#pragma once
#include "platform/platform.h"

namespace platform { 

    namespace nrf24l01 {
        enum class Speed : uint8_t {
            k250 = 0b00100000,
            m1 = 0b00000000,
            m2 = 0b00001000,
        }; 

        enum class Power : uint8_t {
            dbm0 = 0b11,
            dbm6 = 0b10,
            dbm12 = 0b01,
            dbm18 = 0b00,
        }; 

    } // namespace nrf24l01

    /** Simplified driver for the NRF24L01 chip and its clones. 
     
        GND VCC
        RXTX CS 
        SCK MOSI
        MISO IRQ


        Pipe 0 : TX_ADDR for ACKs
        Pipe 1 : RX_ADDR for direct packets
        Pipe 2 : BCAST for broadcasts


        Receiving messages

        To receive a message, initialize, then enter standby and then enableReceiver. Then either poll the receive() method, or wait for an interrupt, upon which call the receive method which is guaranteed to return a packet then. After each successful call to receive() the clearDataReadyIrq() method should be called to clear the IRQ. If this method returns true, there is still a packet to be received and the process should be repeated. 

        Transmitting messages

        Initialize, then standby. When there are messages to be sent, use the transmit() or transmitNoAck() methods to transfer the payloads to the chip. Start the transmitter by calling enableTransmitter(). This will send the messages and generate interrupts each time either a message is sent, or max number of retransmits has been reached. Either poll, or wait for interrupt to call the checkTransmitIrq() method. 

        !!! Note that the device must not transmit for more than 4ms. After whole tx fifo is transmitted, standby-ii mode is entered. At this point either power down, or add more stuff to tx buffer, but tx buffer can't be filled in while actually transmitting as this could exceed the time. 

        !! Fake NRF chips !!

        They have often a bug with the ACK in the Enhanced Shock-Burst, meaning that dynamic payload sizes cannot be ACKed.  
        
    */
    class NRF24L01 {
    public:

        struct Status {
        public:
            uint8_t raw;

            Status(uint8_t raw): raw{raw} {}

            bool rxDataReadyIrq() const { return raw & STATUS_RX_DR; }
            bool rxDataReady() const { return (raw & STATUS_RX_EMPTY) != STATUS_RX_EMPTY; }
            uint8_t rxDataPipe() const { return (raw & STATUS_RX_EMPTY) >> 1; }
            bool txDataSentIrq() const { return raw & STATUS_TX_DS; }
            bool txDataFailIrq() const { return raw & STATUS_MAX_RT; }
            bool txFifoFull() const { return raw & STATUS_TX_FULL; }

            operator uint8_t () const { return raw; }
        }; // Status

        struct FifoStatus {
            uint8_t raw;

            FifoStatus(uint8_t raw): raw{raw} {}

            bool txFull() const { return raw & FIFO_TX_FULL; }
            bool txEmpty() const { return raw & FIFO_TX_EMPTY; }
            bool rxFull() const { return raw & FIFO_RX_FULL; }
            bool rxEmpty() const { return raw & FIFO_RX_EMPTY; }

        }; // FifoStatus

        enum class TxStatus {
            /** Currently sending a message and no sent or failed message yet.
             */
            InProgress,
            /** Last message has been sent ok, and there are more messages that are being sent now.
             */
            Ok,
            /** Last message sent failed. 
             */
            Fail,
        }; // TxStatus
            

        /** Chip select/device ID for the driver.
         */
        const spi::Device CS;
        /** The RX/TX enable pin to turn the radio on or off. 
         */
        const gpio::Pin RXTX;

        NRF24L01(spi::Device cs, gpio::Pin rxtx):
            CS{cs},
            RXTX{rxtx} {
        }

        /** Initializes the driver in the Enhanced shock-burst and returns true if successful, false is not. 
         */
        bool initialize(char const * rxAddr, char const * txAddr, uint8_t ch = 86) {
            gpio::output(RXTX);
            gpio::low(RXTX);
            gpio::output(CS);
            gpio::high(CS);
            // set channel and rx & tx addresses
            setChannel(ch);
            setTxAddress(txAddr);
            setRxAddress(rxAddr);
            // disable enhanced shock burst
            writeRegister(EN_AA, 0); // no auto-ack
            writeRegister(SETUP_RETR, 0); // no retransmit
            writeRegister(FEATURE, 0); // no advanced features
            // enable largest payload size by default
            setPayloadSize(32);
            // set default rf settings - highest power, slowest speed
            setRfSettings(nrf24l01::Power::dbm0, nrf24l01::Speed::k250);
            // and finally time to initialize the config register
            config_ = CONFIG_CRCO | CONFIG_EN_CRC;
            writeRegister(CONFIG, config_);
            return readRegister(CONFIG) == config_;
        }

        /** Initializes the driver in the Enhanced shock-burst and returns true if successful, false is not. 
         */
        bool initializeESB(char const * rxAddr, char const * txAddr, uint8_t ch = 86) {
            gpio::output(RXTX);
            gpio::low(RXTX);
            gpio::output(CS);
            gpio::high(CS);
            // set channel and rx & tx addresses
            setChannel(ch);
            setTxAddress(txAddr);
            setRxAddress(rxAddr);
            // initialize auto acking the messages
            initializeEnhancedShockBurst();
            // enable largest payload size by default
            setPayloadSize(32);
            // set default rf settings - highest power, slowest speed
            setRfSettings(nrf24l01::Power::dbm0, nrf24l01::Speed::k250);
            // and finally time to initialize the config register
            config_ = CONFIG_CRCO | CONFIG_EN_CRC;
            writeRegister(CONFIG, config_);
            return readRegister(CONFIG) == config_;
        }

        /** Powers the nrf24l01p off. 
         */
        void powerDown() {
            gpio::low(RXTX);
            config_ &= ~CONFIG_PWR_UP;
            writeRegister(CONFIG, config_);
        }

        /** Enters the standby mode for the radio. 
         
            Once in standby mode receive and transmit functions can be called. The standby mode also enables all interrupts for both receiver and transmitter and flushes the buffers so that the chip is in a fresh state to either transmit or receive.
        */
        void standby() {
            gpio::low(RXTX);
            config_ |= (CONFIG_PWR_UP | CONFIG_PRIM_RX);
            // enable interrupts (ok to enable all of them)
            config_ &= ~(CONFIG_MASK_MAX_RT | CONFIG_MASK_RX_DR | CONFIG_MASK_TX_DS);
            writeRegister(CONFIG, config_);
            cpu::delayMs(3); // startup time by the datasheet is 1.5ms
            // reset the chip state
            flushTx();
            flushRx();
            clearIRQ();
        }

        /** Enables the receiver of the chip. 
         
            The receiver can be on for as long as necessary and the radio will start receiving immediately after the call to the method. 
        */
        void enableReceiver() {
            gpio::low(RXTX);
            // make sure we are on and select the receiver mode
            config_ |= (CONFIG_PWR_UP | CONFIG_PRIM_RX);
            // enable interrupts (ok to enable all of them)
            config_ &= ~(CONFIG_MASK_MAX_RT | CONFIG_MASK_RX_DR | CONFIG_MASK_TX_DS);
            writeRegister(CONFIG, config_);
            // enable the radio
            gpio::high(RXTX);
        }

        /** Enables the transmitter part of the chip. 
         
            Note that the transmitter can only be in TX mode for no more than 4ms. When the automatic ACK 
        */
        void enableTransmitter() {
            gpio::low(RXTX);
            config_ |= CONFIG_PWR_UP;
            config_ &= ~ CONFIG_PRIM_RX;
            writeRegister(CONFIG, config_);
            gpio::high(RXTX);
        }

        void enablePolledTransmitter() {
            gpio::low(RXTX);
            config_ |= CONFIG_PWR_UP;
            config_ &= ~ CONFIG_PRIM_RX;
            config_ |= CONFIG_MASK_MAX_RT | CONFIG_MASK_RX_DR | CONFIG_MASK_TX_DS;
            // disable interrupts
            writeRegister(CONFIG, config_);
            gpio::high(RXTX);
        }

        Status clearIrq() {
            begin();
            Status status = spi::transfer(WRITEREGISTER | STATUS);
            spi::transfer(STATUS_MAX_RT | STATUS_RX_DR | STATUS_TX_DS); // clear the IRQs
            end();
            return status;
        }


        /** Clears the data ready IRQ. 
         
            Returns true if there are more packets ready in the rx fifo, false when no more data is available. 
        */
        bool clearDataReadyIrq() {
            begin();
            Status status = spi::transfer(WRITEREGISTER | STATUS);
            spi::transfer(STATUS_RX_DR); // clear the IRQ
            end();
            return status.rxDataReady();
        }

        /** Receives a message. 
         
            Receives a message. Returns true if the message has been stored in the buffer, false if there was no message ready on the chip. 
        */
        bool receive(uint8_t * buffer, size_t payloadSize) {
            begin();
            Status status = spi::transfer(R_RX_PAYLOAD);
            if (!status.rxDataReady()) {
                end();
                return false;
            } else {
                spi::receive(buffer, payloadSize);
                end();
                return true;
            }
        }

        /** Uploads the given message to the tx fifo. 
         
            Returns true if the message was uploaded successfully or false if the tx fifo is full. When sent, the message is expected to be acked. 

            Note that calling this actually does not transmit the message. To do so, the startTransmitter() method must be called when the tx fifo is filled with messages to be sent. 
        */
        bool transmit(uint8_t const * buffer, size_t payloadSize) {
            begin();
            Status status = spi::transfer(W_TX_PAYLOAD);
            if (status.txFifoFull()) {
                end();
                return false;
            } else {
                spi::send(buffer, payloadSize);
                end();
                return true;
            }
        }

        /** Uploads a message that should not be acked to the tx fifo. Otherwise works exactly as the transmit() method. 
         */
        bool transmitNoAck(uint8_t const * buffer, size_t payloadSize) {
            begin();
            Status status = spi::transfer(W_TX_PAYLOAD_NO_ACK);
            if (status.txFifoFull()) {
                end();
                return false;
            } else {
                spi::send(buffer, payloadSize);
                end();
                return true;
            }
        }

        /** Clears the TX irqs (failure or sent). 
         */
        Status clearTxIrqs() {
            begin();
            Status status = spi::transfer(WRITEREGISTER | STATUS);
            uint8_t response = status & (STATUS_TX_DS | STATUS_MAX_RT);
            if (response != 0)
                spi::transfer(response);
            end();
            return status;
        }

        /** Checks the last transmission and returns its status.
         */
        TxStatus checkTransmitIrq(bool stopOnFailure = true) {
            begin();
            Status status = spi::transfer(WRITEREGISTER | STATUS);
            TxStatus result = TxStatus::InProgress;
            if (status.txDataSentIrq())
                result = TxStatus::Ok;
            if (status.txDataFailIrq()) {
                result = TxStatus::Fail;
                if (stopOnFailure) {
                    end();
                    standby();
                    return result;
                }
            }
            if (result != TxStatus::InProgress) {
                spi::transfer(status); // this clears all IRQs
            }
            end();
            return result;
        }

        /** Returns the current status register contents. 
         */
        Status getStatus() {
            return readRegister(STATUS);
        }

        /** Returns the status of the tx and rx fifos. 
         */
        FifoStatus getFifoStatus() {
            return readRegister(FIFO_STATUS);
        }

        /** RF Settings, which is the power and speed. 
         */
        std::pair<nrf24l01::Power, nrf24l01::Speed> rfSettings() {
            uint8_t x = readRegister(RF_SETUP);
            auto p = static_cast<nrf24l01::Power>(x & 0b110);
            auto s = static_cast<nrf24l01::Speed>(x & 0b101000);
            return std::make_pair(p, s);
        }

        void setRfSettings(nrf24l01::Power power, nrf24l01::Speed speed) {
            writeRegister(RF_SETUP, static_cast<uint8_t>(power) | static_cast<uint8_t>(speed));        
        }

        /** Channel selection. 
         */

        uint8_t channel() {
            return readRegister(RF_CH);
        }

        void setChannel(uint8_t value) {
            writeRegister(RF_CH, value);
        }

        /** Payload size. 
         */
        uint8_t payloadSize() {
            return readRegister(RX_PW_P1);
        }

        void setPayloadSize(uint8_t size) {
            writeRegister(RX_PW_P1, size);
        }

        /** Transmittingh Address. 
         
            This is the address to which the radio will transmit. 
        */

        void txAddress(char * addr) {
            begin();
            spi::transfer(READREGISTER + TX_ADDR);
            spi::receive(reinterpret_cast<uint8_t*>(addr), 5);
            end();
        }

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

        /** Receiving address.
         */
        void rxAddress(char * addr) {
            begin();
            spi::transfer(READREGISTER | RX_ADDR_P1);
            spi::receive(reinterpret_cast<uint8_t*>(addr), 5);
            end();
        }

        void setRxAddress(const char * addr) {
            begin();
            spi::transfer(WRITEREGISTER | RX_ADDR_P1);
            spi::send(reinterpret_cast<uint8_t const*>(addr), 5);
            end();
        }




    //private:

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

        void initializeEnhancedShockBurst() {
            // auto retransmit count to 15, auto retransmit delay to 1500us, which is the minimum for the worst case of 32bytes long payload and 250kbps speed
            writeRegister(SETUP_RETR, 0x80);
            // enable automatic acknowledge on input pipe 1 & 0
            writeRegister(EN_AA, 3);
            // disable dynamic payloads on all input pipes
            writeRegister(DYNPD, 0);
            // enables per message control of ACK (via the W_TX_PAYLOAD_NO_ACK command)
            writeRegister(FEATURE, EN_DYN_ACK);
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


        /** Local cache of the config register.
         */
        uint8_t config_;

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
        static constexpr uint8_t STATUS_RX_EMPTY = 0b1110;
        
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

    };

} // namespace platform