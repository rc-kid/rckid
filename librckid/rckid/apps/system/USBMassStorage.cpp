#include "pico/unique_id.h"
#include "tusb_config.h"
#include "tusb.h"

#include "USBMassStorage.h"

#include "fonts/Iosevka_Mono6pt7b.h"

namespace rckid {

    void USBMassStorage::update() {
        while (!ST7789::updateDone())
            tud_task();
    }

    void USBMassStorage::draw() {
        Renderer & r = renderer();
        r.fill();
        r.text(0,0);
        r.text() << "USB MSC: " << numEvents_ << " events";
    }

}

using namespace rckid;

//--------------------------------------------------------------------+
// LUN 0
//--------------------------------------------------------------------+
#define README0_CONTENTS \
"LUN0: This is tinyusb's MassStorage Class demo.\r\n\r\n\
If you find any bugs or get any questions, feel free to file an\r\n\
issue at github.com/hathach/tinyusb"


uint8_t msc_disk0[16][512] =
{
  //------------- Block0: Boot Sector -------------//
  // byte_per_sector    = DISK_BLOCK_SIZE; fat12_sector_num_16  = DISK_BLOCK_NUM;
  // sector_per_cluster = 1; reserved_sectors = 1;
  // fat_num            = 1; fat12_root_entry_num = 16;
  // sector_per_fat     = 1; sector_per_track = 1; head_num = 1; hidden_sectors = 0;
  // drive_number       = 0x80; media_type = 0xf8; extended_boot_signature = 0x29;
  // filesystem_type    = "FAT12   "; volume_serial_number = 0x1234; volume_label = "TinyUSB 0  ";
  // FAT magic code at offset 510-511
  {
      0xEB, 0x3C, 0x90, 0x4D, 0x53, 0x44, 0x4F, 0x53, 0x35, 0x2E, 0x30, 0x00, 0x02, 0x01, 0x01, 0x00,
      0x01, 0x10, 0x00, 0x10, 0x00, 0xF8, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x29, 0x34, 0x12, 0x00, 0x00, 'T' , 'i' , 'n' , 'y' , 'U' ,
      'S' , 'B' , ' ' , '0' , ' ' , ' ' , 0x46, 0x41, 0x54, 0x31, 0x32, 0x20, 0x20, 0x20, 0x00, 0x00,

      // Zero up to 2 last bytes of FAT magic code
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55, 0xAA
  },

  //------------- Block1: FAT12 Table -------------//
  {
      0xF8, 0xFF, 0xFF, 0xFF, 0x0F // // first 2 entries must be F8FF, third entry is cluster end of readme file
  },

  //------------- Block2: Root Directory -------------//
  {
      // first entry is volume label
      'T' , 'i' , 'n' , 'y' , 'U' , 'S' , 'B' , ' ' , '0' , ' ' , ' ' , 0x08, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4F, 0x6D, 0x65, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      // second entry is readme file
      'R' , 'E' , 'A' , 'D' , 'M' , 'E' , '0' , ' ' , 'T' , 'X' , 'T' , 0x20, 0x00, 0xC6, 0x52, 0x6D,
      0x65, 0x43, 0x65, 0x43, 0x00, 0x00, 0x88, 0x6D, 0x65, 0x43, 0x02, 0x00,
      sizeof(README0_CONTENTS)-1, 0x00, 0x00, 0x00 // readme's files size (4 Bytes)
  },

  //------------- Block3: Readme Content -------------//
  README0_CONTENTS
};


// tinyusb - device descriptor

tusb_desc_device_t const DESC_DEVICE = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor           = 0xcafe, // 11914, // RaspberryPi 
    .idProduct          = 0x4000 | 2, // 1 MSC device
    .bcdDevice          = 0x0100,

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,

    .bNumConfigurations = 0x01
};

// tinyusb - configuration descriptor

enum {
  ITF_NUM_MSC = 0,
  ITF_NUM_TOTAL
};

#define CONFIG_TOTAL_LEN    (TUD_CONFIG_DESC_LEN + TUD_MSC_DESC_LEN)
#define EPNUM_MSC_OUT   0x01
#define EPNUM_MSC_IN    0x81

uint8_t const desc_fs_configuration[] = {
  // Config number, interface count, string index, total length, attribute, power in mA
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),

  // Interface number, string index, EP Out & EP In address, EP size
  TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 0, EPNUM_MSC_OUT, EPNUM_MSC_IN, 64),
};

// tinyusb - string descriptors

// buffer to hold flash ID
char serial[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1];

// array of pointer to string descriptors
char const * string_desc_arr [] =
{
  (const char[]) { 0x09, 0x04 }, // 0: is supported language is English (0x0409)
  "RCKid",                       // 1: Manufacturer
  "RCKidII",                     // 2: Product
  serial,                        // 3: Serials, uses the flash ID
};

static uint16_t _desc_str[32];


// tinyusb - callbacks

extern "C" {

    uint8_t const * tud_descriptor_device_cb(void) {
        return (uint8_t const *)(& DESC_DEVICE);
    }

    uint8_t const * tud_descriptor_configuration_cb(uint8_t index) {
    (void) index; // for multiple configurations
    return desc_fs_configuration;
    }

    uint16_t const * tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
        uint16_t resultSize = 0;
        switch (index) {
            case 0:
                memcpy(&_desc_str[1], string_desc_arr[0], 2);
                resultSize = 1;
                break;
            case 3:
                pico_get_unique_board_id_string(serial, sizeof(serial));
                // fallthrough
            default: {
                if ( !(index < sizeof(string_desc_arr)/sizeof(string_desc_arr[0])) ) 
                    return nullptr;
                const char* str = string_desc_arr[index];

                // Cap at max char
                resultSize = strlen(str);
                if ( resultSize > 31 ) 
                    resultSize = 31;

                // Convert ASCII string into UTF-16
                for(unsigned i = 0; i < resultSize; ++i)
                    _desc_str[1+i] = str[i];
            }
        }
        // first byte is length (including header), second byte is string type
        _desc_str[0] = (TUSB_DESC_STRING << 8 ) | (2 * resultSize + 2);
        return _desc_str;
    }

    /** Number of LUNs available. For now we have just one (the SD card). 
     
        TODO: return 0 if there is no SD card. 
        TODO: We can in theory have two LUNs, the second being the FATFS in the flash chip. 
    */
    uint8_t tud_msc_get_maxlun_cb(void) {
        return 1; 
    }

    /** Invoked when received SCSI_CMD_INQUIRY
     
    Application fill vendor id, product id and revision with string up to 8, 16, 4 characters respectively
    */
    void tud_msc_inquiry_cb([[maybe_unused]] uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4]) {

        const char vid[] = "RCKidII";
        const char pid[] = "SD";
        const char rev[] = "1.0";

        memcpy(vendor_id  , vid, strlen(vid));
        memcpy(product_id , pid, strlen(pid));
        memcpy(product_rev, rev, strlen(rev));
    }

    /** Invoked when received Test Unit Ready command.
        
        Return true allowing host to read/write this LUN e.g SD card inserted
    */
    bool tud_msc_test_unit_ready_cb([[maybe_unused]] uint8_t lun) {
        return USBMassStorage::available(); 
    }

    /** Invoked when received SCSI_CMD_READ_CAPACITY_10 and SCSI_CMD_READ_FORMAT_CAPACITY to determine the disk size

        Application update block count and block size. 
    */
    void tud_msc_capacity_cb([[maybe_unused]] uint8_t lun, uint32_t* block_count, uint16_t* block_size) {
        *block_count = USBMassStorage::numBlocks();
        *block_size  = USBMassStorage::blockSize();
    }

    /** Invoked when received Start Stop Unit command
     
        - Start = 0 : stopped power mode, if load_eject = 1 : unload disk storage
        - Start = 1 : active mode, if load_eject = 1 : load disk storage
    */
    bool tud_msc_start_stop_cb([[maybe_unused]] uint8_t lun, [[maybe_unused]] uint8_t power_condition, bool start, bool load_eject) {

        if (load_eject) {
            if (start) {
                // load disk storage
            } else {
                // unload disk storage
            }
        }

        return true;
    }

    /** The SD card allows writing. 
     */
    bool tud_msc_is_writable_cb ([[maybe_unused]] uint8_t lun) { return true; }

    /** Callback invoked when received READ10 command.

        Copy disk's data to buffer (up to bufsize) and return number of copied bytes.
    */
    int32_t tud_msc_read10_cb([[maybe_unused]] uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize) {
        // out of ramdisk
        if (lba >= USBMassStorage::numBlocks()) 
            return -1;

        uint8_t const* addr = msc_disk0[lba] + offset;
        memcpy(buffer, addr, bufsize);

        return (int32_t) bufsize;
    }

    /** Callback invoked when received WRITE10 command.
        
        Process data in buffer to disk's storage and return number of written bytes
    */
    int32_t tud_msc_write10_cb([[maybe_unused]] uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize) {
        // out of ramdisk
        if (lba >= USBMassStorage::numBlocks()) 
            return -1;

        uint8_t* addr = msc_disk0[lba]  + offset;
        memcpy(addr, buffer, bufsize);

        return (int32_t) bufsize;
    }

    /** Callback invoked when received an SCSI command not in built-in list below
        
        - READ_CAPACITY10, READ_FORMAT_CAPACITY, INQUIRY, MODE_SENSE6, REQUEST_SENSE
        - READ10 and WRITE10 has their own callbacks
    */
    int32_t tud_msc_scsi_cb ([[maybe_unused]] uint8_t lun, uint8_t const scsi_cmd[16], void* buffer, uint16_t bufsize) {
        // read10 & write10 has their own callback and MUST not be handled here
        void const* response = nullptr;
        int32_t resplen = 0;

        // most scsi handled is input
        bool in_xfer = true;

        switch (scsi_cmd[0]) {
            default:
                // Set Sense = Invalid Command Operation
                tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);
                // negative means error -> tinyusb could stall and/or response with failed status
                resplen = -1;
            break;
        }

        // return resplen must not larger than bufsize
        if ( resplen > bufsize ) 
            resplen = bufsize;

        if (response && (resplen > 0)) {
            if (in_xfer) {
                memcpy(buffer, response, (size_t) resplen);
            } else {
                // SCSI output
            }
        }

        return resplen;
    }

}

