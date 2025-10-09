
#include "platform.h"
#include "sd.h"
#include "backend_config.h"
#include "rckid/apps/DataSync.h"
#include "../backend_internals.h"
#include "sd_spi.pio.h"

/*
#undef RP_PIN_SD_RX
#undef RP_PIN_SD_TX
#undef RP_PIN_SD_SCK
#define RP_PIN_SD_RX 19
#define RP_PIN_SD_TX 18
#define RP_PIN_SD_SCK 17
*/

#define RCKID_SD_SPI_BITBANG 0

namespace rckid {

    namespace {
        uint32_t sdNumBlocks_ = 0;    
        uint spiSm_;
        uint spiOffset_;

        void sd_spi_write_bitbang(uint8_t const * data, uint32_t size) {
            while (size-- > 0) {
                uint8_t d = *data++;
                for (int i = 0; i < 8; ++i) {
                    gpio_put(RP_PIN_SD_SCK, false);
                    gpio_put(RP_PIN_SD_TX, (d & 0x80) != 0);
                    d = d << 1;
                    cpu::delayUs(5);
                    gpio_put(RP_PIN_SD_SCK, true);
                    cpu::delayUs(5);
                }
            }
            // remember to idle at low
            gpio_put(RP_PIN_SD_SCK, false);
        }

        void sd_spi_read_bitbang(uint8_t byteToSend, uint8_t * data, uint32_t size) {
            while (size-- > 0) {
                uint8_t d = 0;
                for (int i = 0; i < 8; ++i) {
                    d = d << 1;
                    gpio_put(RP_PIN_SD_SCK, false);
                    gpio_put(RP_PIN_SD_TX, (byteToSend & 0x80) != 0);
                    byteToSend = byteToSend << 1;
                    cpu::delayUs(5);
                    gpio_put(RP_PIN_SD_SCK, true);
                    cpu::delayUs(5);
                    d = d | (gpio_get(RP_PIN_SD_RX) ? 1 : 0);
                }
                *data++ = d;
            }
            // remember to idle at low
            gpio_put(RP_PIN_SD_SCK, false);
        }

        void sd_spi_write_blocking(uint8_t const * data, uint32_t size) {
#if (RCKID_SD_SPI_BITBANG)
            return sd_spi_write_bitbang(data, size);
#endif
            // this is from the Pico SDK where the io_rw_8 replicates the byte to all 4 bytes from the 32bit FIFO
            // TODO not sure how the read can work
            // TODO would this work with DMA? 
            io_rw_8 *txfifo = (io_rw_8 *) &RCKID_SD_PIO->txf[spiSm_];
            io_rw_8 *rxfifo = (io_rw_8 *) &RCKID_SD_PIO->rxf[spiSm_];            
            uint32_t rxlen = size;
            // Drive out all bytes and drain corresponding RX bytes (dummy data) to keep FIFOs balanced.
            while (size > 0 || rxlen > 0) {
                if (size > 0 && !pio_sm_is_tx_fifo_full(RCKID_SD_PIO, spiSm_)) {
                    *txfifo = *data++;
                    --size;
                }
                if (rxlen > 0 && !pio_sm_is_rx_fifo_empty(RCKID_SD_PIO, spiSm_)) {
                    (void) *rxfifo;
                    --rxlen;
                }
      //          tight_loop_contents();
            }
        }


        void sd_spi_read_blocking(uint8_t byteToSend, uint8_t * data, uint32_t size) {
#if (RCKID_SD_SPI_BITBANG)
            return sd_spi_read_bitbang(byteToSend, data, size);
#endif
            io_rw_8 *txfifo = (io_rw_8 *) &RCKID_SD_PIO->txf[spiSm_];
            io_rw_8 *rxfifo = (io_rw_8 *) &RCKID_SD_PIO->rxf[spiSm_];            
            uint32_t txlen = size;
            // Schedule txlen dummy bytes to clock in size bytes, then drain all RX bytes.
            while (size > 0 || txlen > 0) {
                if (txlen > 0 && !pio_sm_is_tx_fifo_full(RCKID_SD_PIO, spiSm_)) {
                    *txfifo = byteToSend;
                    --txlen;
                }
                if (size > 0 && !pio_sm_is_rx_fifo_empty(RCKID_SD_PIO, spiSm_)) {
                    *data++ = *rxfifo;
                    --size;
                }
//                tight_loop_contents();
            }
        }
    }; // anonymous namespace

    void loopbackVerify() {
        for (uint32_t i = 0; i < 256; ++i) {
            uint8_t toSend = static_cast<uint8_t>(i);
            uint8_t received = 0;
            sd_spi_read_blocking(toSend, & received, 1);
            if (toSend != received) {
                LOG(LL_ERROR, "Loopback verify failed at " << i << ", sent: " << hex((uint32_t)toSend) << ", received: " << hex((uint32_t)received));
                return;
            }
        }
        LOG(LL_INFO, "Loopback verify succeeded");
    }

    void sdEnterSPIMode() {
        pio_sm_set_enabled(RCKID_SD_PIO, spiSm_, false);
        gpio::outputHigh(RP_PIN_SD_CSN); // no cs
        gpio::outputLow(RP_PIN_SD_SCK); // clock idles low
        gpio::setAsInput(RP_PIN_SD_RX);
        gpio::outputHigh(RP_PIN_SD_TX); // tx is high during the initial state
        for (uint8_t i = 0; i < 80; ++i) {
            cpu::delayUs(5);
            gpio_put(RP_PIN_SD_SCK, true);
            cpu::delayUs(5);
            gpio_put(RP_PIN_SD_SCK, false);
        }
#if (RCKID_SD_SPI_BITBANG)
        //sd_spi_program_init(RCKID_SD_PIO, spiSm_, spiOffset_, RP_PIN_SD_RX, RP_PIN_SD_TX, RP_PIN_SD_SCK);
        pio_sm_set_clock_speed(RCKID_SD_PIO, spiSm_, RCKID_SD_SPI_INIT_SPEED * RCKID_SD_SPI_SPEED_MULTIPLIER);
        pio_sm_set_enabled(RCKID_SD_PIO, spiSm_, true);
#endif
    }

    /** Initializes the SD card system, but does not talk to the card. 
     
        Simply loads the PIO and initializes the card detect pin.
     */
    bool sdInitialize() {
        LOG(LL_INFO, "SD init");
        // initialize the PIO for SPI communication with the SD card, use the same pio as the display driver since its base is already set to 16
        spiSm_ = pio_claim_unused_sm(RCKID_SD_PIO, true);
        spiOffset_ = pio_add_program(RCKID_SD_PIO, & sd_spi_program);
        LOG(LL_INFO, "  spi sm: " << (uint32_t)spiSm_);
        LOG(LL_INFO, "  spi offset: " << (uint32_t)spiOffset_);
        gpio::setAsInput(RP_PIN_SD_CD);
        // TODO enable interrupt??
    }

    /** Returns true if SD card is inserted, false otherwise. 
     */
    bool sdIsInserted() {
        return gpio::read(RP_PIN_SD_CD) == 0;
    }

    /** Initializes the SD card into SPI mode. 
     
        The information in the method mostly comes from [1] and associated links, namely [2]. 

        [1] https://electronics.stackexchange.com/questions/602105/how-can-i-initialize-use-sd-cards-with-spi
        [2] http://elm-chan.org/docs/mmc/mmc_e.html#dataxfer
     */
    bool sdInitializeCard() {
        if (! sdIsInserted())
            return false;

        // make the card go to SPI mode 
        sdEnterSPIMode();

        // the card should now be in SPI mode, tell it to go idle
        uint8_t buffer[16];
        gpio::low(RP_PIN_SD_CSN);
        uint8_t status = SD_IDLE;
        for (uint32_t i = 0; i < 20; ++i) {
            status = sdSendCommand(CMD0);
            if (status != 0xff)
                break;
            LOG(LL_INFO, "init fail, retry " << i);
        }
        if (status != SD_IDLE) {
            LOG(LL_ERROR, "SD card did not respond to CMD0, status: " << hex(status));
            return false;
        }
        // send interface condition to verify the communication. this is not supported by v1 cards so if we get illegal command continue with the initialization process
        status = sdSendCommand(CMD8, buffer, 4);
        if (status != SD_ILLEGAL_COMMAND) {
            if (status != SD_IDLE) {
                LOG(LL_ERROR, "SD card did not respond to CMD8, status: " << hex(status));
                return false;
            }
            if (buffer[3] != 0xaa) {
                LOG(LL_ERROR, "SD card CMD8 response[3] error: " << hex(buffer[3]));
                return false;
            }
        }
        // read the OCR register and check the card supports the 3v3 voltage range. This is likely not necessary as every card should be 3v3 compatible, but just to be sure
        status = sdSendCommand(CMD58, buffer, 4);
        if (status != SD_IDLE) {
            LOG(LL_ERROR, "SD card did not respond to CMD58, status: " << hex(status));
            return false;
        }
        if ((buffer[1] & 0x38) != 0x38) {
            LOG(LL_ERROR, "SD card CMD58 response[1] error: " << hex(buffer[1]));
            return false;
        }
        // and now get to the power on state, this needs to be done in a loop
        unsigned attempts = 0;
        while (true) {
            if (attempts++ > 100) {
                LOG(LL_ERROR, "SD card did not power up timeout");
                return false;
            }
            if (sdSendCommand(CMD55) != SD_IDLE) {
                LOG(LL_ERROR, "SD card did not respond to CMD55, status: " << hex(status));
                return false;
            }
            if (sdSendCommand(ACMD41) == SD_NO_ERROR)
                break;
            cpu::delayMs(10);
        }
        // the card is now ready to be operated, send CMD58 again to verify the card power status bit
        status = sdSendCommand(CMD58, buffer, 2);
        if (status != SD_NO_ERROR) {
            LOG(LL_ERROR, "SD card did not respond to CMD58 after power up, status: " << hex(status));
            return false;
        }
        if (! (buffer[0] & 0x80)) {
            LOG(LL_ERROR, "SD card CMD58 response[0] error: " << hex(buffer[0]));
            return false;
        }
        // increase speed to 20MHz
        //spi_init(RP_SD_SPI, 20000000);
        pio_sm_set_clock_speed(RCKID_SD_PIO, spiSm_, RCKID_SD_SPI_SPEED * RCKID_SD_SPI_SPEED_MULTIPLIER);
        // if the card is not SDHC, set block length to 512 bytes
        if (((buffer[0] & 64) == 0) && sdSendCommand(CMD16) != SD_NO_ERROR) {
            LOG(LL_ERROR, "SD card did not respond to CMD16, status: " << hex(status));
            return false;
        }
        // determine the card capacity by reading the CSD
        if (sdSendCommand(CMD9, buffer, 16) != SD_NO_ERROR) {
            LOG(LL_ERROR, "SD card did not respond to CMD9, status: " << hex(status));
           return false;
        }
        // get number of blocks from the CS card's capacity. For SDXC and SDHC cards, this is (CSIZE + 1) * 512KB, so we divide the CSIZE + 1 by 1024 to get size in 512 byte blocks
        sdNumBlocks_ = ((buffer[8] << 16) + (buffer[9] << 8) + buffer[10] + 1) * 1024;
        LOG(LL_INFO, "SD card initialized, blocks: " << sdNumBlocks_);
        return true;
    }

    uint8_t sdSendCommand(uint8_t const (&cmd)[6], uint8_t * response, size_t responseSize, unsigned maxDelay) {
        //gpio::low(RP_PIN_SD_CSN);
        sd_spi_write_blocking(cmd,  6);
        uint8_t result = SD_BUSY;
        while (result == SD_BUSY && maxDelay-- != 0)
            sd_spi_read_blocking(0xff, reinterpret_cast<uint8_t*>(& result), 1);
        if (responseSize != 0 && response != nullptr)
            sd_spi_read_blocking(0xff, response, responseSize);
        // after reading each response, it is important to send extra one byte to allow the SD crd to "recover"
        uint8_t tmp = 0xff;
        sd_spi_write_blocking(& tmp, 1);
        //gpio::high(RP_PIN_SD_CSN);
        return result;
    }

    // rckid API functions

    uint32_t sdCapacity() {
        return sdNumBlocks_;
    }

    bool sdReadBlocks(uint32_t start, uint8_t * buffer, uint32_t numBlocks) {
        while (numBlocks-- != 0) {
            //gpio::low(RP_PIN_SD_CSN);
            uint8_t cmd[] = { 
                0x51, 
                static_cast<uint8_t>((start >> 24) & 0xff), 
                static_cast<uint8_t>((start >> 16) & 0xff), 
                static_cast<uint8_t>((start >> 8) & 0xff), 
                static_cast<uint8_t>(start & 0xff),
                0x01
            };
            //uint8_t cmd[] = { 0x51, 0, 0, 0, 0, 0x01 };
            //spi_write_blocking(RP_SD_SPI, cmd, 6);
            //spi_read_blocking(RP_SD_SPI, 0xff, buffer, 2048);
            //gpio::high(RP_PIN_SD_CSN);
            if (sdSendCommand(cmd) != 0)
                return false;
            // wait for the block start
            uint8_t res = 0xff;
            while (res != 0xfe)
                sd_spi_read_blocking(0xff, &res, 1);
            // read the block
            sd_spi_read_blocking(0xff, buffer, 512);
            // and read the CRC
            uint16_t crc;
            sd_spi_read_blocking(0xff, reinterpret_cast<uint8_t*>(&crc), 2);
            res = 0xff;
            sd_spi_write_blocking(& res, 1);
            // verify the CRC
            ++start;
            buffer += 512;
        }
        return true;
    }

    bool sdWriteBlocks(uint32_t start, uint8_t const * buffer, uint32_t numBlocks) {
        while (numBlocks-- != 0) {
            uint8_t cmd[] = { 
                0x58,
                static_cast<uint8_t>((start >> 24) & 0xff), 
                static_cast<uint8_t>((start >> 16) & 0xff), 
                static_cast<uint8_t>((start >> 8) & 0xff),
                static_cast<uint8_t>(start & 0xff),
                0x01
            };
            if (sdSendCommand(cmd) != 0)
                return false;
            cmd[0] = 0xff;
            cmd[1] = 0xfe; // start of data block
            sd_spi_write_blocking(cmd, 2); // 1 byte wait + start data block
            sd_spi_write_blocking(buffer, 512);
            sd_spi_write_blocking(cmd, 2); // CRC
            sd_spi_read_blocking(0xff, cmd, 1); // get the data response
            // wait for busy
            while (cmd[0] != 0xff)
                sd_spi_read_blocking(0xff, cmd, 1);
            ++start;
            buffer += 512;
        }
        return true;
    }

} // namespace rckid