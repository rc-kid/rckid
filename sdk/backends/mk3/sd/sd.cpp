
#include "platform.h"
#include "sd.h"
#include "backend_config.h"
#include "rckid/apps/DataSync.h"
#include "../backend_internals.h"

namespace rckid {

    namespace {
        uint32_t sdNumBlocks_ = 0;    
    }; // anonymous namespace


    /** Initializes the SD card into SPI mode. 
     
        The information in the method mostly comes from [1] and associated links, namely [2]. 

        [1] https://electronics.stackexchange.com/questions/602105/how-can-i-initialize-use-sd-cards-with-spi
        [2] http://elm-chan.org/docs/mmc/mmc_e.html#dataxfer
     */
    bool sdInitialize() {
        // to initialize, the SPI baudrate must be between 100-400kHz for the initialization
        spi_init(RP_SD_SPI, 200000);
        gpio_set_function(RP_PIN_SD_SCK, GPIO_FUNC_SPI);
        gpio_set_function(RP_PIN_SD_TX, GPIO_FUNC_SPI);
        gpio_set_function(RP_PIN_SD_RX, GPIO_FUNC_SPI);
        bi_decl(bi_3pins_with_func(RP_PIN_SD_SCK, RP_PIN_SD_TX, RP_PIN_SD_RX, GPIO_FUNC_SPI));
        // initialize the CS for manual control
        gpio::outputHigh(RP_PIN_SD_CSN);
        // create the buffer and fill it with 0xff
        uint8_t buffer[16];
        memset(buffer, 0xff, sizeof(buffer));
        // while CS is high, send at least 74 times 0xff
        spi_write_blocking(RP_SD_SPI, buffer, sizeof(buffer));
        // tell the card to go idle
        gpio::low(RP_PIN_SD_CSN);
        uint8_t status = sdSendCommand(CMD0);
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
        status = sdSendCommand(CMD58, buffer, 4);
        if (status != SD_NO_ERROR) {
            LOG(LL_ERROR, "SD card did not respond to CMD58 after power up, status: " << hex(status));
            return false;
        }
        if (! (buffer[0] & 0x80)) {
            LOG(LL_ERROR, "SD card CMD58 response[0] error: " << hex(buffer[0]));
            return false;
        }
        // increase speed to 20MHz
        spi_init(RP_SD_SPI, 20000000);
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
        spi_write_blocking(RP_SD_SPI, cmd,  6);
        uint8_t result = SD_BUSY;
        while (result == SD_BUSY && maxDelay-- != 0)
            spi_read_blocking(RP_SD_SPI, 0xff, reinterpret_cast<uint8_t*>(& result), 1);
        if (responseSize != 0 && response != nullptr)
            spi_read_blocking(RP_SD_SPI, 0xff, response, responseSize);
        // after reading each response, it is important to send extra one byte to allow the SD crd to "recover"
        uint8_t tmp = 0xff;
        spi_write_blocking(RP_SD_SPI, & tmp, 1);
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
                spi_read_blocking(RP_SD_SPI, 0xff, &res, 1);
            // read the block
            spi_read_blocking(RP_SD_SPI, 0xff, buffer, 512);
            // and read the CRC
            uint16_t crc;
            spi_read_blocking(RP_SD_SPI, 0xff, reinterpret_cast<uint8_t*>(&crc), 2);
            res = 0xff;
            spi_write_blocking(RP_SD_SPI, & res, 1);
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
            spi_write_blocking(RP_SD_SPI, cmd, 2); // 1 byte wait + start data block
            spi_write_blocking(RP_SD_SPI, buffer, 512);
            spi_write_blocking(RP_SD_SPI, cmd, 2); // CRC
            spi_read_blocking(RP_SD_SPI, 0xff, cmd, 1); // get the data response
            // wait for busy
            while (cmd[0] != 0xff)
                spi_read_blocking(RP_SD_SPI, 0xff, cmd, 1);
            ++start;
            buffer += 512;
        }
        return true;
    }

} // namespace rckid