
#include "platform.h"
#include "sd.h"
#include "backend_config.h"
#include "rckid/apps/DataSync.h"
#include "../backend_internals.h"
#include "sd_spi.pio.h"

// debug option to use bitbanging instead of the pio SPI driver - only really useful for debugging the basic protocol
#define RCKID_SD_SPI_BITBANG 0

namespace rckid {

    namespace {

        uint32_t sdNumBlocks_ = 0;    
        uint spiSm_;
        uint spiOffset_;

        /** Bitbanged single byte write. 
         */
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

        /** Bitbanged single byte read.
         */
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

    void sdEnterSPIMode() {
        LOG(LL_INFO, "SD switch to SPI mode");
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
#if (RCKID_SD_SPI_BITBANG == 0)
        sd_spi_program_init(RCKID_SD_PIO, spiSm_, spiOffset_, RP_PIN_SD_RX, RP_PIN_SD_TX, RP_PIN_SD_SCK);
        pio_sm_set_clock_speed(RCKID_SD_PIO, spiSm_, RCKID_SD_SPI_INIT_SPEED * RCKID_SD_SPI_SPEED_MULTIPLIER);
        pio_sm_set_enabled(RCKID_SD_PIO, spiSm_, true);
#endif
    }

    uint8_t sdWriteCommand(uint8_t const (&cmd)[6], uint8_t * response, size_t responseSize, unsigned maxDelay) {
        sd_spi_write_blocking(cmd,  6);
        uint8_t result = SD_BUSY;
        while (result == SD_BUSY && maxDelay-- != 0)
            sd_spi_read_blocking(0xff, reinterpret_cast<uint8_t*>(& result), 1);
        if (responseSize != 0 && response != nullptr)
            sd_spi_read_blocking(0xff, response, responseSize);
        return result;
    }

    uint8_t sdSendCommand(uint8_t const (&cmd)[6], uint8_t * response = nullptr, size_t responseSize = 0, unsigned maxDelay = SD_MAX_DELAY) {
        gpio::low(RP_PIN_SD_CSN);
        uint8_t result = sdWriteCommand(cmd, response, responseSize, maxDelay);
        // after reading each response, it is important to send extra one byte to allow the SD crd to "recover"
        uint8_t tmp = 0xff;
        sd_spi_write_blocking(& tmp, 1);
        gpio::high(RP_PIN_SD_CSN);
        return result;
    }

    uint8_t sdBlockCommand(uint8_t const (&cmd)[6], uint8_t * response, uint32_t responseSize, uint32_t maxDelay = SD_MAX_DELAY) {
        gpio::low(RP_PIN_SD_CSN);
        // write the command and wait for response
        uint8_t result = sdWriteCommand(cmd, nullptr, 0, maxDelay);
        if (result != SD_NO_ERROR) {
            gpio::high(RP_PIN_SD_CSN);
            return result;
        }
        // now wait for the data token (0xfe)
        do {
            sd_spi_read_blocking(0xff, & result, 1);
        } while (result != SD_DATA_TOKEN);
        // read the data
        sd_spi_read_blocking( 0xff, response, responseSize);
        // read the CRC
        uint8_t crc[2];
        sd_spi_read_blocking(0xff, crc, 2);
        // and add one more extra byte to allow the card to recover
        result = 0xff;
        sd_spi_write_blocking(& result, 1);
        gpio::high(RP_PIN_SD_CSN);
        return SD_NO_ERROR;
    }


    /** Initializes the SD card system, but does not talk to the card. 
     
        Simply loads the PIO and initializes the card detect pin.
     */
    void sdInitialize() {
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

        // the card should now be in SPI mode, tell it to go idle via CMD0, we'll repeat this 10 times in case the card takes long time (maybe not necessary)
        uint8_t buffer[16];
        gpio::low(RP_PIN_SD_CSN);
        uint8_t status = SD_IDLE;
        for (uint32_t i = 0; i < 10; ++i) {
            status = sdSendCommand(CMD0);
            if (status != 0xff)
                break;
            LOG(LL_INFO, "init fail, retry " << i);
        }
        if (status != SD_IDLE) {
            LOG(LL_ERROR, "SD card did not respond to CMD0, status: " << hex(status));
            return false;
        }
        LOG(LL_INFO, "  idle (CMD0)");
        // determine if the card is v2 or v1 by sending CMD8 with the pattern 0x1aa. As v1 cards only support up to 2GB, we do not really care about them now so illegal command is failed initiliazation. We can also verify the communication is ok, by checking the pattern (0xaa) is echoed back
        status = sdSendCommand(CMD8, buffer, 4);
        if (status != SD_IDLE) {
            LOG(LL_ERROR, "  v1 not supported, or invalid status: "  << hex(status));
            return false;
        }
        if (buffer[3] != 0xaa) {
            LOG(LL_ERROR, "  echo mismatch: " << hex(buffer[3]));
            return false;
        }
        // now repeatedly send ACMD41 (which is CMD55 followed by CMD41) until the card goes to ready state (not idle anymore)
        for (uint32_t i = 0; i < 100; ++i) {
            status = sdSendCommand(CMD55);
            if (status != SD_IDLE) {
                LOG(LL_ERROR, "  cmd55 error: " << hex(status));
                return false;
            }
            status = sdSendCommand(ACMD41);
            if (status == SD_NO_ERROR)
                break;
            yield();
            cpu::delayMs(10);
        }
        if (status != SD_NO_ERROR) {
            LOG(LL_ERROR, "  power up timeout: " << hex(status));
            return false;
        }
        // read the OCR register, this tells us whether the card supports 3.3V (they all do) and whether it is standard capacity (below 2GB, byte addressable, or SDHC/SDXC which are both block addressable). As we have previously discarded cards below 2GB anyways, we can safely only support block addressable cards
        status = sdSendCommand(CMD58, buffer, 4);
        if (status != SD_NO_ERROR) {
            LOG(LL_ERROR, "  ocr error: " << hex(status));
            return false;
        }
        if (buffer[0] & 0x40 != 0x40) {
            LOG(LL_ERROR, "  byte addressable not supported");
        }
        // now the card is powered up and we can increase the speed
        pio_sm_set_enabled(RCKID_SD_PIO, spiSm_, false);
        pio_sm_set_clock_speed(RCKID_SD_PIO, spiSm_, RCKID_SD_SPI_SPEED * RCKID_SD_SPI_SPEED_MULTIPLIER);
        pio_sm_set_enabled(RCKID_SD_PIO, spiSm_, true);
        // and determine the card capacity by reading the CSD register via CMD9
        status = sdBlockCommand(CMD9, buffer, 16);
        if (status != SD_NO_ERROR) {
            LOG(LL_ERROR, "  csd error: " << hex(status));
            return false;
        }
        for (uint32_t i = 0; i < 16; ++i)
            LOG(LL_INFO, "    csd[" << i << "]: " << hex(buffer[i]));

        // get number of blocks from the CS card's capacity. For SDXC and SDHC cards, this is (CSIZE + 1) * 512KB, so we divide the CSIZE + 1 by 1024 to get size in 512 byte blocks
        // as the CSIZE is stored in bits 48-69
        // [ 0] 120
        // [ 1] 112
        // [ 2] 104    
        // [ 3]  96
        // [ 4]  88
        // [ 5]  80
        // [ 6]  72
        // [ 7]  64   xxxxxx
        // [ 8]  56 xxxxxxxx
        // [ 9]  48 xxxxxxxx
        // [10]  40
        // [11]  32
        // [12]  24
        // [13]  16
        // [14]  8
        // [15]  0
        sdNumBlocks_ = buffer[9] + (buffer[8] << 8) + ((buffer[7] & 0x3f) << 16) + 1;
        sdNumBlocks_ = sdNumBlocks_ * 1024; // convert to number of 512 byte blocks
        LOG(LL_INFO, "  SD blocks: " << sdNumBlocks_);
        return true;
    }

    // rckid API functions

    uint32_t sdCapacity() {
        return sdNumBlocks_;
    }

    bool sdReadBlocks(uint32_t start, uint8_t * buffer, uint32_t numBlocks) {
        while (numBlocks-- != 0) {
            uint8_t cmd[] = { 
                0x51, 
                static_cast<uint8_t>((start >> 24) & 0xff), 
                static_cast<uint8_t>((start >> 16) & 0xff), 
                static_cast<uint8_t>((start >> 8) & 0xff), 
                static_cast<uint8_t>(start & 0xff),
                0x01
            };
            if (sdBlockCommand(cmd, buffer, 512) != SD_NO_ERROR)
                return false;
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
            gpio::low(RP_PIN_SD_CSN);
            uint8_t status = sdWriteCommand(cmd, nullptr, 0, SD_MAX_DELAY);
            if (status != SD_NO_ERROR) {
                gpio::high(RP_PIN_SD_CSN);
                return false;
            }
            // send one extra byte, then data token and the the buffer itself
            cmd[0] = 0xff;
            cmd[1] = 0xfe; // start of data block
            sd_spi_write_blocking(cmd, 2); // 1 byte wait + start data block
            sd_spi_write_blocking(buffer, 512);
            sd_spi_write_blocking(cmd, 2); // CRC -- we ignore the CRC
            sd_spi_read_blocking(0xff, & status, 1); // get the data response
            // wait for busy
            while (status != SD_BUSY)
                sd_spi_read_blocking(0xff, & status, 1);
            ++start;
            buffer += 512;
        }
        return true;
    }

} // namespace rckid