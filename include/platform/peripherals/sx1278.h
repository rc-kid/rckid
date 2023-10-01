#pragma once
#include "platform/platform.h"

namespace platform { 

    namespace sx1278 {

        enum class Power : uint8_t {
            dbm2 = 0,
            dbm3,
            dbm4,
            dbm5,
            dbm6,
            dbm7,
            dbm8,
            dbm9,
            dbm10,
            dbm11,
            dbm12,
            dbm13,
            dbm14,
            dbm15,
            dbm16,
            dbm17,
        }; 

    } // namespace sx1278


    /** A very simple LoRa driver for SX1278 & compatible chips. 
     
        The chips themselves are way more powerful than the driver. 
    */

    class SX1278 {
    public:

        const spi::Device CS;

        SX1278(spi::Device cs):
            CS{cs} {
        }

        bool initialize(uint16_t freq = 43400, sx1278::Power power = sx1278::Power::dbm17) {
            gpio::output(CS);
            gpio::high(CS);
            // the REG_VERSION must contain 0x12. If that's not the case either the chip is wrong, or SPI is not working
            if (readRegister(REG_VERSION) != 0x12)
                return false;
            // start in sleep mode where we can access all registers
            sleep();
            // the tx and rx modes can use the whole fifo for themselves
            writeRegister(REG_FIFO_TX_BASE_ADDR, 0);
            writeRegister(REG_FIFO_RX_BASE_ADDR, 0);            
            // set the default parameters
            setPayloadSize(32);
            // enable implicit header mode, smallest coding and large bandwidth
            writeRegister(REG_MODEM_CONFIG_1, CONFIG_IMPLICIT_HEADER_MODE | CONFIG_CODING_4_5 | CONFIG_BANDWIDTH_250k);
            // use smallest spreading factor for fastest baudrate, disable crc 
            writeRegister(REG_MODEM_CONFIG_2, 0x60);
            // as per the datasheet, for the smallest spreading factor we need to se the following too (and the implicit header)
            writeRegister(REG_DETECTION_OPTIMIZE, 0xc5);
            writeRegister(REG_DETECTION_THRESHOLD, 0x0c);
            // set autogain for the LNA
            writeRegister(REG_MODEM_CONFIG_3, 0x04);
            // set maximum LNA gain & enable boost (150% of LNA current)
            writeRegister(REG_LNA, 0b00100011);        
            // set the transmitter power and frequency
            setTxPower(power);
            setFrequency(freq);
            // move to standby for quicker tx and rx times
            standby();
            return true;
        }

        void sleep() {
            writeRegister(REG_OP_MODE, MODE_LORA | MODE_SLEEP);
        }

        void standby() {
            writeRegister(REG_OP_MODE, MODE_LORA | MODE_STANDBY);
        }

        /** Enables the continous receiver, puts RxDone on DIO0 and starts listening. 
         */
        void enableReceiver() {
            writeRegister(REG_DIO_MAPPING_1, 0b00000000); // enable RXdone on DIO0
            writeRegister(REG_OP_MODE, MODE_LORA | MODE_RX_CONTINUOUS);
        }

        /** Returns true if there was a message ready to be copied to the buffer. False otherwise. 
         */
        bool receive(uint8_t * buffer, size_t payloadSize) {
            if (readRegister(REG_RX_NB_BYTES) < payloadSize)
                return false;        
            writeRegister(REG_FIFO_ADDR_PTR, readRegister(REG_FIFO_RX_CURRENT_ADDR));
            begin();
            spi::transfer(REG_FIFO);
            spi::receive(buffer, payloadSize);
            end();
            return true;
        }

        uint8_t packetRssi() {
            return readRegister(REG_PKT_RSSI_VALUE);
        }

        /** Enables the transmitter. 
         
            Enters standby mode and makes sure that DIO0 will go from 0 to 1 when TxDone interrupt happens. 
        */
        void enableTransmitter() {
            standby();
            writeRegister(REG_DIO_MAPPING_1, 0b01000000); // enable TXdone on DIO0
        }

        /** Transmits the given payload as a single packet.
         */
        void transmit(uint8_t const * buffer, size_t payloadSize) {
            //standby();
            // reset the fifo. Is this necessary? 
            writeRegister(REG_FIFO_ADDR_PTR, 0);
            // upload the packet
            begin();
            spi::transfer(REG_FIFO | 0x80);
            spi::send(buffer, payloadSize);
            end();
            // enter tx mode
            writeRegister(REG_OP_MODE, MODE_LORA | MODE_TX);
        }

        void clearIrq() {
            writeRegister(REG_IRQ_FLAGS, 0xff);
        }


        /** Sets the frequency (channel). 
         
            43400
        */
        void setFrequency(uint16_t freq) {
            uint64_t f = static_cast<uint64_t>(freq) << 19 / 100 / 32000000;
            writeRegister(REG_FRF_MSB, (f >> 16) & 0xff);
            writeRegister(REG_FRF_MID, (f >> 8) & 0xff);
            writeRegister(REG_FRF_LSB, f & 0xff);
        }

        void setPayloadSize(uint8_t size) {
            writeRegister(REG_PAYLOAD_LENGTH, size);
        }

        uint8_t getPayloadSize() {
            return readRegister(REG_PAYLOAD_LENGTH);
        }

        void setTxPower(sx1278::Power power) {
            writeRegister(REG_PA_DAC, 0x84);
            writeRegister(REG_PA_CONFIG, PA_CONFIG_BOOST | static_cast<uint8_t>(power));
        }

    private:

        static constexpr uint8_t REG_FIFO = 0x00;
        static constexpr uint8_t REG_OP_MODE = 0x01;
        static constexpr uint8_t REG_FRF_MSB = 0x06;
        static constexpr uint8_t REG_FRF_MID = 0x07;
        static constexpr uint8_t REG_FRF_LSB = 0x08;
        static constexpr uint8_t REG_PA_CONFIG = 0x09;
        static constexpr uint8_t REG_OCP = 0x0b;
        static constexpr uint8_t REG_LNA = 0x0c;
        static constexpr uint8_t REG_FIFO_ADDR_PTR = 0x0d;
        static constexpr uint8_t REG_FIFO_TX_BASE_ADDR = 0x0e;
        static constexpr uint8_t REG_FIFO_RX_BASE_ADDR = 0x0f;
        static constexpr uint8_t REG_FIFO_RX_CURRENT_ADDR = 0x10;
        static constexpr uint8_t REG_IRQ_FLAGS = 0x12;
        static constexpr uint8_t REG_RX_NB_BYTES = 0x13;
        static constexpr uint8_t REG_PKT_SNR_VALUE = 0x19;
        static constexpr uint8_t REG_PKT_RSSI_VALUE = 0x1a;
        static constexpr uint8_t REG_RSSI_VALUE = 0x1b;
        static constexpr uint8_t REG_MODEM_CONFIG_1 = 0x1d;
        static constexpr uint8_t REG_MODEM_CONFIG_2 = 0x1e;
        static constexpr uint8_t REG_PREAMBLE_MSB = 0x20;
        static constexpr uint8_t REG_PREAMBLE_LSB = 0x21;
        static constexpr uint8_t REG_PAYLOAD_LENGTH = 0x22;
        static constexpr uint8_t REG_MODEM_CONFIG_3 = 0x26;
        static constexpr uint8_t REG_FREQ_ERROR_MSB = 0x28;
        static constexpr uint8_t REG_FREQ_ERROR_MID = 0x29;
        static constexpr uint8_t REG_FREQ_ERROR_LSB = 0x2a;
        static constexpr uint8_t REG_RSSI_WIDEBAND = 0x2c;
        static constexpr uint8_t REG_DETECTION_OPTIMIZE = 0x31;
        static constexpr uint8_t REG_INVERTIQ = 0x33;
        static constexpr uint8_t REG_DETECTION_THRESHOLD = 0x37;
        static constexpr uint8_t REG_SYNC_WORD = 0x39;
        static constexpr uint8_t REG_INVERTIQ2 = 0x3b;
        static constexpr uint8_t REG_DIO_MAPPING_1 = 0x40;
        static constexpr uint8_t REG_VERSION = 0x42;
        static constexpr uint8_t REG_PA_DAC = 0x4d;

        // modes
        static constexpr uint8_t MODE_LORA = 0x80;
        static constexpr uint8_t MODE_SLEEP = 0x00;
        static constexpr uint8_t MODE_STANDBY = 0x01;
        static constexpr uint8_t MODE_TX = 0x03;
        static constexpr uint8_t MODE_RX_CONTINUOUS = 0x05;
        static constexpr uint8_t MODE_RX_SINGLE = 0x06;

        // config 1
        static constexpr uint8_t CONFIG_BANDWIDTH_7k8 = 0;
        static constexpr uint8_t CONFIG_BANDWIDTH_10k4 = 0x10;
        static constexpr uint8_t CONFIG_BANDWIDTH_15k6 = 0x20;
        static constexpr uint8_t CONFIG_BANDWIDTH_20k8 = 0x30;
        static constexpr uint8_t CONFIG_BANDWIDTH_31k25 = 0x40;
        static constexpr uint8_t CONFIG_BANDWIDTH_41k7 = 0x50;
        static constexpr uint8_t CONFIG_BANDWIDTH_62k5 = 0x60;
        static constexpr uint8_t CONFIG_BANDWIDTH_125k = 0x70;
        static constexpr uint8_t CONFIG_BANDWIDTH_250k = 0x80;
        static constexpr uint8_t CONFIG_BANDWIDTH_500k = 0x90;
        static constexpr uint8_t CONFIG_CODING_4_5 = 0b0010;
        static constexpr uint8_t CONFIG_CODING_4_6 = 0b0100;
        static constexpr uint8_t CONFIG_CODING_4_7 = 0b0110;
        static constexpr uint8_t CONFIG_CODING_4_8 = 0b1000;
        static constexpr uint8_t CONFIG_IMPLICIT_HEADER_MODE = 1;

        // config 2
        static constexpr uint8_t CONFIG_PAYLOAD_CRC = 0b100;

        // PA config
        static constexpr uint8_t PA_CONFIG_BOOST = 0x80;

        void begin() {
            spi::begin(CS);
            cpu::delayUs(2);
        }

        void end() {
            spi::end(CS);
            cpu::delayUs(2);
        }

        void writeRegister(uint8_t reg, uint8_t value) {
            begin();
            spi::transfer(reg | 0x80);
            spi::transfer(value);
            end();
        }

        uint8_t readRegister(uint8_t reg) {
            begin();
            spi::transfer(reg & 0x7f);
            uint8_t x = spi::transfer(0x00);
            end();
            return x;
        }


    }; // SX1278

} // namespace platform 