#include "pico/binary_info.h"

#include "hw_config.h"

#include "tusb.h"
#include "sd.h"

#include <stdio.h>
#include "pico/stdlib.h"
#include "common/config.h"

/* Configuration of RP2040 hardware SPI object */
static spi_t spi = {  
    .hw_inst = RP_SD_SPI,  
    .miso_gpio = RP_PIN_SD_RX,
    .mosi_gpio = RP_PIN_SD_TX,
    .sck_gpio = RP_PIN_SD_SCK,    
    .baud_rate = RP_SD_SPI_BAUDRATE,   
    // if this were ommitted, the DMA would interfere with the display which uses the same DMA irq
    .DMA_IRQ_num = DMA_IRQ_0,
    .use_exclusive_DMA_IRQ_handler = false
};

/* SPI Interface */
static sd_spi_if_t spi_if = {
    .spi = &spi,
    .ss_gpio = RP_PIN_SD_CSN
};

/* Configuration of the SD Card socket object */
static sd_card_t sd_card = {   
    /* "pcName" is the FatFs "logical drive" identifier.
    (See http://elm-chan.org/fsw/ff/doc/filename.html#vol) */
    .pcName = "0:",
    .type = SD_IF_SPI,
    .spi_if_p = &spi_if  // Pointer to the SPI interface driving this card
};

/* ********************************************************************** */

size_t sd_get_num() { return 1; }

sd_card_t *sd_get_by_num(size_t num) {
    if (0 == num) {
        return &sd_card;
    } else {
        return NULL;
    }
}

namespace rckid {

    bool SD::mount() {
        card_ = sd_get_by_num(0);
        FRESULT r = f_mount(&card_->fatfs, card_->pcName, 1);
        return r == FR_OK;
    }

    void SD::unmount() {
        f_unmount(card_->pcName);
        card_ = nullptr;
    }

    uint64_t SD::totalBytes() {
        FATFS * fs = & card_->fatfs;
        uint64_t result = (fs->n_fatent - 2) * fs->csize;
        return result * BYTES_PER_SECTOR;
    }

    uint64_t SD::freeBytes() {
        DWORD freeClusters = 0;
        FATFS * fs = nullptr;
        f_getfree(card_->pcName, & freeClusters, &fs);
        uint64_t freeSectors = freeClusters * fs->csize;
        return freeSectors * BYTES_PER_SECTOR;
    }

    void SD::initialize() {
        spi_init(RP_SD_SPI, 400000);
        gpio_set_function(RP_PIN_SD_SCK, GPIO_FUNC_SPI);
        gpio_set_function(RP_PIN_SD_TX, GPIO_FUNC_SPI);
        gpio_set_function(RP_PIN_SD_RX, GPIO_FUNC_SPI);
        // Make the SPI pins available to picotool
        bi_decl(bi_3pins_with_func(RP_PIN_SD_SCK, RP_PIN_SD_TX, RP_PIN_SD_RX, GPIO_FUNC_SPI));
        // initialize the chip select pin 
        gpio_init(RP_PIN_SD_CSN);
        gpio_set_dir(RP_PIN_SD_CSN, GPIO_OUT);
        gpio_put(RP_PIN_SD_CSN, 1);
        // Make the CS pin available to picotool
        bi_decl(bi_1pin_with_name(RP_PIN_SD_CSN, "SPI CS"));
        // initialize the SD card with SPI mode, this is done by sending the CMD0 (go to idle) while the CSN pin is low as per the datasheet
        gpio_put(RP_PIN_SD_CSN, 0);
        uint8_t resetCmd[] = {0x40, 0, 0, 0, 0, 0x95};
        spi_write_blocking(RP_SD_SPI, resetCmd, sizeof(resetCmd));
        gpio_put(RP_PIN_SD_CSN, 1);
        // initialize the SD card, filing an error if stuff changes
                
    }

    void SD::processEvents() {
        //tud_task();
    }

}

