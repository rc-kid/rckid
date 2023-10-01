#pragma once

#include "platform/platform.h"

namespace platform {

    /** A simplified driver for the nRF24l01+ radio chip. 

        GND VCC
        RXTX CS 
        SCK MOSI
        MISO IRQ

        Operation notes:

        - when packet is not transmitted successfully (MAX_RT), then no further transmits are possible unless the MAX_RT is cleared *and* the packet is *not* removed from the TX buffer!
        - PA+LNA version does not work well short range, especially on max power settings

        - FEATURES must be 0 on the green module for the ACKs to work... Then it kind of works sometimes... But does not seem reliable at all atm

    */
    class NRF24L01 {
    public:
        const spi::Device CS;
        const gpio::Pin RXTX;

        struct Config {
            bool disableDataReadyIRQ() const { return raw & CONFIG_MASK_RX_DR; }
            bool disableDataSentIRQ() const { return raw & CONFIG_MASK_TX_DS; }
            bool disableMaxRetransmitsIRQ() const { return raw & CONFIG_MASK_MAX_RT; }
            uint8_t crcSize() const {
                if (raw | CONFIG_EN_CRC)
                    return (raw | CONFIG_CRCO) ? 2 : 1;
                else
                    return 0;
            }
            bool powerUp() const { return raw & CONFIG_PWR_UP; }
            bool transmitReady() const { return raw & (CONFIG_PRIM_RX == 0); }
            bool receiveReady() const { return raw & CONFIG_PRIM_RX; }
            uint8_t const raw;
        }; 

        struct Status {
            bool dataReady() const { return raw & STATUS_RX_DR; }
            bool dataSent() const { return raw & STATUS_TX_DS; }
            bool maxRetransmits() const { return raw & STATUS_MAX_RT; }
            bool txFull() const { return raw & STATUS_TX_FULL; }
            bool rxEmpty() const { return dataReadyPipe() == 7; }
            /** Returns the data pipe from which the data is ready. 
             
                0..5 are valid pipe values, 7 is returned when rx pipe is empty. 
            */
            uint8_t dataReadyPipe() const { return (raw > 1) & 7; }

            uint8_t const raw;
        }; // NRF24L01::Status

        struct TxStats {
            uint8_t lostPackets() const { return raw >> 4; }
            uint8_t lastRetransmissions() const { return raw & 0xf; }
            uint8_t const raw;
        }; // NRF24L01::TxStats

        enum class Speed : uint8_t {
            k250 = 0b00100000,
            m1 = 0b00000000,
            m2 = 0b00001000,
        }; 

        enum class Power : uint8_t {
            dbm0 = 0b110,
            dbm6 = 0b100,
            dbm12 = 0b010,
            dbm18 = 0b000,
        }; 

        /** Initializes the control pins of the module. 
         */
        NRF24L01(gpio::Pin CS, gpio::Pin RXTX):
            CS(CS),
            RXTX(RXTX) {
        }

        /** Initializes the chip. 
         
            Sets the tx and rx addresses, channel, power, speed and payload size. Enables auto acknowledgement, auto acknowledgement with payloads, dynamic payloads and non-acked payloads. Enables reading pipes 0 (for auto acknowledgements) and 1 (for receiving). 
        */
        void initialize(const char * rxAddr, const char * txAddr,  uint8_t channel = 76, Speed speed = Speed::k250, Power power = Power::dbm0, uint8_t payloadSize = 32) {
            gpio::output(CS);
            gpio::output(RXTX);
            gpio::high(CS);
            gpio::low(RXTX);
            // set the desired speed and output power
            writeRegister(RF_SETUP, static_cast<uint8_t>(power) | static_cast<uint8_t>(speed));
            // set the channel and tx and rx addresses
            setChannel(channel);
            setTxAddress(txAddr);
            setRxAddress(rxAddr);
            // enable auto ack and enhanced shock-burst
            enableAutoAck();
            // enable read pipes 0 and 1
            writeRegister(EN_RXADDR, 3);
            // set the payload size
            setPayloadSize(payloadSize);
            // reset the chip's status
            writeRegister(STATUS, STATUS_MAX_RT | STATUS_RX_DR | STATUS_TX_DS);
            // flush the tx and rx fifos
            flushTx();
            flushRx();
            // set configuration to powered down, ready to receive, crc to 2 bytes
            // all events will appear on the interrupt pin
            config_ = CONFIG_CRCO | CONFIG_EN_CRC;
            writeRegister(CONFIG, config_);
        }

        /** Sets static payload size for all receiving pipes. 
         
            Technically only affects pipe 1, as others are disabled and pipe 0 has dynamic payload size enabled for auto ack payloads to work. 
        */
        void setPayloadSize(uint8_t value) {
            for (uint8_t i = 0; i < 6; ++i)
                writeRegister(RX_PW_P0 + i, value);        
        }

        /** Enables or disables the auto acknowledgement feature. 
        
            Also enables the dynamic payload length and ack payload features so that ack packages can return non critical data. 
        */
        void enableAutoAck(bool enable = true) {
            if (enable) {
                // auto retransmit count to 15, auto retransmit delay to 1500us, which is the minimum for the worst case of 32bytes long payload and 250kbps speed
                writeRegister(SETUP_RETR, 0x80);
                // enable automatic acknowledge on input pipe 1 & 0
                writeRegister(EN_AA, 3);
                // disable dynamic payloads on all input pipes except pipe 0 used for ack payloads
                writeRegister(DYNPD, 0xff);
                // enables the enhanced shock-burst features, dynamic payload size and transmit of packages without ACKs
                writeRegister(FEATURE, 0);
                //writeRegister(FEATURE,  EN_DPL | EN_ACK_PAY | EN_DYN_ACK );
            } else {
                writeRegister(EN_AA, 0); // disable auto ack
                writeRegister(FEATURE, 0); // disable dynamic payload and ack payload features
                writeRegister(DYNPD, 0); // disable dynamic payload on all pipes
                writeRegister(SETUP_RETR, 0); // disable auto retransmit
            }
        }

        /** Sets the channel. 
         */
        void setChannel(uint8_t value) {
            writeRegister(RF_CH, value);
        }
        
        /** Returns the current channel. 
         */
        uint8_t channel() {
            return readRegister(RF_CH & 0x7f);
        }

        /** Clears the status flags for interrupt events. 
         */
        void clearIRQ() {
            writeRegister(STATUS, STATUS_RX_DR | STATUS_TX_DS | STATUS_MAX_RT); // clear interrupt flags
        }

        /** Flushes the trasmitter's buffer. 
         */
        void flushTx() {
            begin();
            spi::transfer(FLUSH_TX);
            end();
        }
        
        /** Flushes the receiver's buffer. 
         */
        void flushRx() {
            begin();
            spi::transfer(FLUSH_RX);
            end();
        }

        /** Reads the transmitter's address into the given buffer. The buffer must be at least 5 bytes long. 
         */
        void txAddress(char * addr) {
            begin();
            spi::transfer(READREGISTER + TX_ADDR);
            receive(reinterpret_cast<uint8_t*>(addr), 5);
            end();
        }

        /** Sets the transmitter's address (package target). The address must be 5 bytes long. 
         
            The address is also set for the 0th receiving pipe for auto acknowledgements.
        */
        void setTxAddress(const char * addr) {
            begin();
            spi::transfer(WRITEREGISTER | RX_ADDR_P0);
            spi::send(reinterpret_cast<uint8_t const*>(addr), 5);
            end();
            begin();
            spi::transfer(WRITEREGISTER | TX_ADDR);
            spi::send(reinterpret_cast<uint8_t const*>(addr), 5);
            end();
        }
        
        /** Reads the receiver's address into the given buffer. The buffer must be at least 5 bytes long. 
         */
        void rxAddress(char * addr) {
            begin();
            spi::transfer(READREGISTER | RX_ADDR_P1);
            receive(reinterpret_cast<uint8_t*>(addr), 5);
            end();
        }

        /** Sets the receiver's address. The address must be 5 bytes long. 

            For now only pipe 1 is supported as pipe 1 can have a full address specified, the other pipes differ only it the last byte. Also, as of now, I can't imagine a situation where I would like to receive on two addresses. 
        */
        void setRxAddress(const char * addr) {
            begin();
            spi::transfer(WRITEREGISTER | RX_ADDR_P1);
            spi::send(reinterpret_cast<uint8_t const*>(addr), 5);
            end();
        }

        /** Enters the power down mode. 
         */
        void powerDown() {
            gpio::low(RXTX);
            config_ &= ~CONFIG_PWR_UP;
            writeRegister(CONFIG, config_);
        }

        /** Enters the standby mode
         
            Note that standby mode has to be entered before transmitting or receiving or the radio might not work from the very beginning.
        */
        void standby() {
            gpio::low(RXTX);
            config_ |= CONFIG_PWR_UP | CONFIG_PRIM_RX;
            writeRegister(CONFIG, config_);
            cpu::delayMs(3); // startup time by the datasheet is 1.5ms
        }

        /** Starts the receiver. 
         
            To stop the receiver, call either standby() or powerDown(). 
        */
        void startReceiver() {
            config_ |= (CONFIG_PWR_UP | CONFIG_PRIM_RX);
            writeRegister(CONFIG, config_);
            gpio::high(RXTX);
        }

        /** Receives a single message from the TX FIFO.
         
            The caller must make sure that the message is of the specified size, and that a message exists. Otherwise the protocol will stop working. 
        */
        void receive(uint8_t * buffer, uint8_t size) {
            begin();
            spi::transfer(R_RX_PAYLOAD);
            spi::receive(buffer, size);
            end();
        }

        /** Downloads a variable length message returning its length. 
         
            Returns 0 if there is no new message. 
        */
        uint8_t receive(uint8_t * buffer) {
            begin();
            Status status{spi::transfer(R_RX_PL_WID)};
            uint8_t len = 0;
            if (status.dataReady()) {
                len = spi::transfer(0);
                end();
                begin();
                spi::transfer(R_RX_PAYLOAD);
                spi::receive(buffer, len);
            }
            end();
            return len;
        }

        /** Transmits given message immediately. 
         
            ACK will be expected on the message unless auto ack feature is disabled.
        */
        void transmit(uint8_t * buffer, uint8_t size) {
            gpio::low(RXTX); // disable power to allow switching, or sending a pulse
            config_ &= ~CONFIG_PRIM_RX;
            config_ |= CONFIG_PWR_UP; 
            writeRegister(CONFIG, config_);
            // transmit the payload
            begin();
            spi::transfer(W_TX_PAYLOAD);
            spi::send(buffer, size);
            end();
            // send RXTX pulse to initiate the transmission, datasheet requires 10 us delay
            gpio::high(RXTX);
            cpu::delayUs(15); // some margin to 10us required by the datasheet
            gpio::low(RXTX);
        }

        /** Transmits the given payload not requiring an ACK from the receiver. 
         
            Only works in auto ack mode.
        */
        void transmitNoAck(uint8_t * buffer, uint8_t size) {
            gpio::low(RXTX); // disable power to allow switching, or sending a pulse
            config_ &= ~CONFIG_PRIM_RX;
            config_ |= CONFIG_PWR_UP; 
            writeRegister(CONFIG, config_);
            // transmit the payload
            begin();
            spi::transfer(W_TX_PAYLOAD_NO_ACK);
            spi::send(buffer, size);
            end();
            // send RXTX pulse to initiate the transmission, datasheet requires 10 us delay
            gpio::high(RXTX);
            cpu::delayUs(15); // some margin
            gpio::low(RXTX);
        }

        /** Sends given payload as next ack's payload. 
         
            Only works in auto ack mode. 
        */
        void transmitAckPayload(uint8_t const * payload, uint8_t size) {
            begin();
            spi::transfer(W_ACK_PAYLOAD | 1); // for pipe 1, since we do not use other pipes
            spi::send(payload, size);
            end();
        }

        /** Returns the contents of the config register on the device. This is for debugging purposes only. 
         */
        Config config() {
            return Config{readRegister(CONFIG)};
        }
        
        /** Returns the contents of the status register. For debugging purposes only. 
         */
        Status status() {
            begin();
            Status result{spi::transfer(NOP)};
            end();
            return result;
        }
        
        /** Returns the observe TX register. For debugging purposes only. 
         */
        TxStats observeTX() {
            return TxStats{readRegister(OBSERVE_TX)};
        }


        /** Returns the fifo status register. For debugging purposes only. 
         */
        uint8_t fifoStatus() {
            return readRegister(FIFO_STATUS);
        }


        uint8_t features() {
            return readRegister(FEATURE);
        }

        
    private:

        void begin() {
            spi::begin(CS);
            cpu::delayUs(2);
        }

        void end() {
            spi::end(CS);
            cpu::delayUs(2);
        }

        uint8_t readRegister(uint8_t reg) {
            begin();
            spi::transfer(READREGISTER | reg);
            uint8_t result = spi::transfer(0);
            end();
            return result;
        }
        
        void writeRegister(uint8_t reg, uint8_t value) {
            //printf("Writing register %u, value %u\n", reg, value);
            begin();
            spi::transfer(WRITEREGISTER | reg);
            spi::transfer(value);
            end();
        }
        
        // commands
        
        static constexpr uint8_t READREGISTER = 0;
        static constexpr uint8_t WRITEREGISTER = 0b00100000;
        static constexpr uint8_t R_RX_PAYLOAD = 0b01100001;
        static constexpr uint8_t W_TX_PAYLOAD = 0b10100000;
        static constexpr uint8_t FLUSH_TX = 0b11100001;
        static constexpr uint8_t FLUSH_RX = 0b11100010;
        static constexpr uint8_t REUSE_TX_PL = 0b11100011;
        static constexpr uint8_t R_RX_PL_WID = 0b01100000;
        static constexpr uint8_t W_ACK_PAYLOAD = 0b10101000;
        static constexpr uint8_t W_TX_PAYLOAD_NO_ACK = 0b10110000;
        static constexpr uint8_t NOP = 0b11111111;
        
        // registers
        
        static constexpr uint8_t CONFIG = 0;
        static constexpr uint8_t EN_AA = 1;
        static constexpr uint8_t EN_RXADDR = 2;
        static constexpr uint8_t SETUP_AW = 3;
        static constexpr uint8_t SETUP_RETR = 4;
        static constexpr uint8_t RF_CH = 5;
        static constexpr uint8_t RF_SETUP = 6;
        static constexpr uint8_t STATUS = 7;
        static constexpr uint8_t OBSERVE_TX = 8;
        static constexpr uint8_t RPD = 9;
        static constexpr uint8_t RX_ADDR_P0 = 0x0a; // 5 bytes
        static constexpr uint8_t RX_ADDR_P1 = 0x0b; // 5 bytes
        static constexpr uint8_t RX_ADDR_P2 = 0x0c;
        static constexpr uint8_t RX_ADDR_P3 = 0x0d;
        static constexpr uint8_t RX_ADDR_P4 = 0x0e;
        static constexpr uint8_t RX_ADDR_P5 = 0x0f;
        static constexpr uint8_t TX_ADDR = 0x10; // 5 bytes
        static constexpr uint8_t RX_PW_P0 = 0x11;
        static constexpr uint8_t RX_PW_P1 = 0x12;
        static constexpr uint8_t RX_PW_P2 = 0x13;
        static constexpr uint8_t RX_PW_P3 = 0x14;
        static constexpr uint8_t RX_PW_P4 = 0x15;
        static constexpr uint8_t RX_PW_P5 = 0x16;
        static constexpr uint8_t FIFO_STATUS = 0x17;
        static constexpr uint8_t DYNPD = 0x1c;
        static constexpr uint8_t FEATURE = 0x1d;
        
        // config register values
        static constexpr uint8_t CONFIG_MASK_RX_DR = 1 << 6;
        static constexpr uint8_t CONFIG_MASK_TX_DS = 1 << 5;
        static constexpr uint8_t CONFIG_MASK_MAX_RT = 1 << 4;
        static constexpr uint8_t CONFIG_EN_CRC = 1 << 3;
        static constexpr uint8_t CONFIG_CRCO = 1 << 2;
        static constexpr uint8_t CONFIG_PWR_UP = 1 << 1;
        static constexpr uint8_t CONFIG_PRIM_RX = 1 << 0;
        
        // status register values
        static constexpr uint8_t STATUS_RX_DR = 1 << 6;
        static constexpr uint8_t STATUS_TX_DS = 1 << 5;
        static constexpr uint8_t STATUS_MAX_RT = 1 << 4;
        static constexpr uint8_t STATUS_TX_FULL = 1 << 0;
        
        // features
        static constexpr uint8_t EN_DPL = 1 << 2;
        static constexpr uint8_t EN_ACK_PAY = 1 << 1;
        static constexpr uint8_t EN_DYN_ACK = 1 << 0;

        // fifo status register values
        static constexpr uint8_t FIFO_TX_REUSE = 1 << 6;
        static constexpr uint8_t FIFO_TX_FULL = 1 << 5;
        static constexpr uint8_t FIFO_TX_EMPTY = 1 << 4;
        static constexpr uint8_t FIFO_RX_FULL = 1 << 1;
        static constexpr uint8_t FIFO_RX_EMPTY = 1 << 0;
        
        uint8_t config_;
        
    };

} // namespace platform
