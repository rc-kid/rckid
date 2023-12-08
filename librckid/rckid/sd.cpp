#include "hw_config.h"

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

}

void sdtest() {
    sd_card_t *pSD = sd_get_by_num(0);
    

    FRESULT fr = f_mount(&pSD->fatfs, pSD->pcName, 1);
    if (FR_OK != fr) panic("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
    FIL fil;
    const char* const filename = "audio/test2.txt";
    fr = f_open(&fil, filename, FA_OPEN_APPEND | FA_WRITE);
    if (FR_OK != fr && FR_EXIST != fr)
        panic("f_open(%s) error: %s (%d)\n", filename, FRESULT_str(fr), fr);
    if (f_printf(&fil, "Hello, world!\n") < 0) {
        printf("f_printf failed\n");
    }
    fr = f_close(&fil);
    if (FR_OK != fr) {
        printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
    }
    f_unmount(pSD->pcName);

}
