#include "platform.h"

#include "pico/unique_id.h"
#include "tusb_config.h"
#include "tusb.h"


#include "rckid/apps/DataSync.h"

using namespace rckid;

/** \name USB Interface
    
 */
//@{

/* A combination of interfaces must have a unique product id, since PC will save device driver after the first plug.
 * Same VID/PID with different interface e.g MSC (first), then CDC (later) will possibly cause system error on PC.
 *
 * Auto ProductID layout's Bitmap:
 *   [MSB]         HID | MSC | CDC          [LSB]
 */
#define _PID_MAP(itf, n)  ( (CFG_TUD_##itf) << (n) )
#define USB_PID           (0x4000 | _PID_MAP(CDC, 0) | _PID_MAP(MSC, 1) | _PID_MAP(HID, 2) | \
                           _PID_MAP(MIDI, 3) | _PID_MAP(VENDOR, 4) )

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
    .idProduct          = USB_PID,
    .bcdDevice          = 0x0100,

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,

    .bNumConfigurations = 0x01
};

// tinyusb - configuration descriptor

enum {
  ITF_NUM_CDC = 0,
  ITF_NUM_CDC_DATA,
  ITF_NUM_MSC,
  ITF_NUM_TOTAL
};

#define CONFIG_TOTAL_LEN    (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_MSC_DESC_LEN)
  #define EPNUM_CDC_NOTIF   0x81
  #define EPNUM_CDC_OUT     0x02
  #define EPNUM_CDC_IN      0x82

  #define EPNUM_MSC_OUT     0x03
  #define EPNUM_MSC_IN      0x83

uint8_t const desc_fs_configuration[] = {
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),

    // Interface number, string index, EP notification address and size, EP data address (out, in) and size.
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4, EPNUM_CDC_NOTIF, 8, EPNUM_CDC_OUT, EPNUM_CDC_IN, 64),

    // Interface number, string index, EP Out & EP In address, EP size
    TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 5, EPNUM_MSC_OUT, EPNUM_MSC_IN, 64),
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

//@}

/** \name CDC (USB Serial Adapter)
 * 
 */
//@{
extern "C" {

    // Invoked when cdc when line state changed e.g connected/disconnected
    void tud_cdc_line_state_cb([[maybe_unused]] uint8_t itf, bool dtr, [[maybe_unused]] bool rts) {
        // TODO set some indicator
        if (dtr) {
            // Terminal connected
        } else {
            // Terminal disconnected
        }
    }

    // Invoked when CDC interface received data from host
    void tud_cdc_rx_cb([[maybe_unused]] uint8_t itf) {
        // TODO - do we want to support reading? 
    }

}

//@}

/** \name USB Mass Storage
 */
//@{

extern "C" {

    uint8_t const * tud_descriptor_device_cb(void) {
        return (uint8_t const *)(& DESC_DEVICE);
    }

    uint8_t const * tud_descriptor_configuration_cb([[maybe_unused]] uint8_t index) {
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

        In theory, we can have also the LittleFS that is mounted in the flash as a LUN here, but LittleFS is not easily understood by all operating systems, and the flash storage is not expected to be user accessible anyways. 
    */
    uint8_t tud_msc_get_maxlun_cb(void) {
        return sdCapacity() > 0 ? 1 : 0; 
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
        
        Return true allowing host to read/write this LUN e.g SD card inserted. Only returns true if the DataSync app is active, otherwise the disk is not ready as it may be used by RCKid device apps. 
    */
    bool tud_msc_test_unit_ready_cb([[maybe_unused]] uint8_t lun) {
        return rckid::DataSync::active();
    }

    /** Invoked when received SCSI_CMD_READ_CAPACITY_10 and SCSI_CMD_READ_FORMAT_CAPACITY to determine the disk size

        Application update block count and block size. 
    */
    void tud_msc_capacity_cb([[maybe_unused]] uint8_t lun, uint32_t* block_count, uint16_t* block_size) {
        *block_count = sdCapacity();
        *block_size  = 512;
    }

    /** Invoked when received Start Stop Unit command
     
        - Start = 0 : stopped power mode, if load_eject = 1 : unload disk storage
        - Start = 1 : active mode, if load_eject = 1 : load disk storage
    */
    bool tud_msc_start_stop_cb([[maybe_unused]] uint8_t lun, [[maybe_unused]] uint8_t power_condition, bool start, bool load_eject) {
        if (start)
            DataSync::connect();
        else if (load_eject)
            DataSync::disconnect();
        //if (load_eject) 
        //    start ? DataSync::connect() : DataSync::disconnect();
        return true;
    }

    /** The SD card allows writing. 
     */
    bool tud_msc_is_writable_cb ([[maybe_unused]] uint8_t lun) { return true; }

    /** Callback invoked when received READ10 command.

        Copy disk's data to buffer (up to bufsize) and return number of copied bytes.
    */
    int32_t tud_msc_read10_cb([[maybe_unused]] uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize) {
        // out of disk
        if (lba >= sdCapacity()) 
            return -1;
        if (bufsize != 512)
            ERROR(rckid::error::USBMSCRead);

        sdReadBlocks(lba, reinterpret_cast<uint8_t*>(buffer), 1);
        ++rckid::DataSync::blocksRead_;
        return (int32_t) bufsize;
    }

    /** Callback invoked when received WRITE10 command.
        
        Process data in buffer to disk's storage and return number of written bytes
    */
    int32_t tud_msc_write10_cb([[maybe_unused]] uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize) {
        // out of ramdisk
        if (lba >= sdCapacity()) 
            return -1;
        if (bufsize != 512)
            ERROR(rckid::error::USBMSCWrite);

        sdWriteBlocks(lba, buffer, 1);
        ++rckid::DataSync::blocksWrite_;
        //uint8_t* addr = msc_disk0[lba]  + offset;
        //memcpy(addr, buffer, bufsize);

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
//@}