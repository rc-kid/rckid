#pragma once

#include "../app.h"
#include "../graphics/bitmap.h"
#include "../graphics/png.h"
#include "../filesystem.h"



#if (defined ARCH_RCKID_2)
#include "tusb.h"
#include "hardware/structs/usb.h"
#endif

namespace rckid {

    /** USB Mass Storage device.  
     
        The app has to be active in order for the mass storage attachment to work. It detaches the SD card from the RCKid async DMA routines and makes it exclusively available to the USB connected PC as a mass storage device. 

        Furthermore, it detects if DC power is applied while the app is on, and if it is, initializes the usb stack. We can'd do this immediately because the ESD protection circuit on the USB leaks the pullups on the dataline to the VCC which then prevents the USB aware devices to connect and charge the RCKid properly. 

        When the app is exitted, the USB device is turned off, which re-enables the DC voltage detection & charging. 

     */
    class DataSync : public BitmapApp<16> {
    public:

        static bool active() { return mscActive_; }

    protected:

        DataSync(): 
            BitmapApp<16>{Arena::allocator()}
            /*icon_{std::move(ARENA(Bitmap<ColorRGB>::fromImage(PNG::fromBuffer(assets::icons64::pen_drive))))} */ {
        }

        void focus() override {
            BitmapApp<16>::focus();
            namespace fs = rckid::filesystem;
            sizeBlocks_ = sdCapacity();
            if (sizeBlocks_ > 0) {
                if (fs::mount()) {
                    format_ = fs::formatToStr(fs::getFormat());
                    label_ = fs::getLabel();
                    fs::unmount();
                }
            }
            mscActive_ = false;
        }

        void blur() override {
            BitmapApp<16>::blur();
            if (mscActive_) {
#if (defined ARCH_RCKID_2)
                // disable USB -- reset so that we can again detect DC charge
                mscActive_ = false;
                // TODO deinit freezes
                tud_deinit(BOARD_TUD_RHPORT);
                memset(reinterpret_cast<uint8_t *>(usb_hw), 0, sizeof(*usb_hw));
#endif
            }
        }

        void update() override {
#if (defined ARCH_RCKID_2)
            if (! mscActive_ && dcPower()) {
                // initialize the USB guest
                tud_init(BOARD_TUD_RHPORT);
                mscActive_ = true;
            } else if (mscActive_ && ! dcPower()) {
                mscActive_ = false;
                // disable USB -- reset so that we can again detect DC charge
                tud_deinit(BOARD_TUD_RHPORT);
                memset(reinterpret_cast<uint8_t *>(usb_hw), 0, sizeof(*usb_hw));
            }
#endif
            App::update();
            /*
            // This is too dangerous to have here
            if (btnPressed(Btn::Start)) {
                filesystem::unmount();
                filesystem::format();
                exit();
            }
            */
        }

        void draw() override {
            // TODO for some reason, STR below does not work with ARENA, not sure why
            //NewArenaScope _;
            /*
            g_.fill();
            Header::drawOn(g_);
            g_.blit(Point{128, 54}, icon_);
            char const * status = mscActive_ ? "Connected" : "Disconnected";
            g_.text(160 - g_.font().textWidth(status) / 2, 120) << status;

            std::string stats = STR("blocks: " << sizeBlocks_ << ", " << (sizeBlocks_ / 2 / 1024) << "MB");
            g_.text(160 - g_.font().textWidth(stats) / 2, 140) << stats;

            if (format_ != nullptr)
                stats = STR(format_ << ", label: " << label_);
            else
                stats = "???";
            g_.text(160 - g_.font().textWidth(stats) / 2, 160) << stats;

            stats = STR("r: " << blocksRead_ << ", w: " << blocksWrite_);
            g_.text(160 - g_.font().textWidth(stats) / 2, 180) << stats;
            */
        }

    private:

        friend bool sdReadBlocks(uint32_t start, uint8_t * buffer, uint32_t numBlocks);
        friend bool sdWriteBlocks(uint32_t start, uint8_t const * buffer, uint32_t numBlocks);
        
        //Bitmap<ColorRGB> icon_;
        uint32_t sizeBlocks_;
        char const * format_ = nullptr;
        String label_;

        inline static bool mscActive_ = false;
        inline static uint32_t blocksRead_ = 0;
        inline static uint32_t blocksWrite_ = 0;

    }; // rckid::DataSync

} // namespace rckid