#pragma once

#include "../app.h"
#include "../filesystem.h"
#include "../ui/form.h"
#include "../ui/label.h"
#include "../ui/image.h"

#if (defined ARCH_RCKID_2)
#include "tusb.h"
#include "hardware/structs/usb.h"
#endif

extern "C" {
    bool tud_msc_start_stop_cb(uint8_t, uint8_t, bool, bool);
    int32_t tud_msc_read10_cb(uint8_t, uint32_t, uint32_t, void *, uint32_t);
    int32_t tud_msc_write10_cb(uint8_t, uint32_t, uint32_t, uint8_t*, uint32_t);
}

namespace rckid {

    /** SD Card as USB MSC
     
        At this point it only works if the USB is already connected when the app is started. Disconnect & connect detection does not work properly. Only tested on mk2 so far.
     */
    class DataSync : public ui::App<void> {
    public:

        String name() const override { return "DataSync"; }

        DataSync():
            ui::App<void>{},
            icon_{Icon{assets::icons_64::pen_drive}},
            info_{Rect::XYWH(0, 150, 320, 20), "SD card not found"},
            status_{Rect::XYWH(0, 180, 320, 20), "Disconnected"} 
        {
            icon_.setTransparent(true);
            icon_.setPos(160 - icon_.width() / 2, 60);
            status_.setHAlign(HAlign::Center);
            g_.addChild(icon_);
            g_.addChild(info_);
            g_.addChild(status_);
            fs::mount(fs::Drive::SD);
            if (fs::isMounted()) {
                info_.setText(STR("SD card: " << sdCapacity() / 2 / 1024 << "MB, label " << fs::getLabel()));
                fs::unmount();
                mscStatus_ = Status::Disconnected;
            }

        }

        void update() override {
            ui::App<void>::update();
            if (btnPressed(Btn::B) || btnPressed(Btn::Down)) {
                btnClear(Btn::B);
                btnClear(Btn::Down);
                exit();
            }
        }

        void draw() override {

            switch (mscStatus_) {
                case Status::Inactive:
                    status_.setText("Inactive");
                    break;
                case Status::Disconnected:
                    status_.setText("Disconnected");
                    break;
                case Status::Connected:
                    status_.setText(STR("Connected, r: " << blocksRead_ << ", w: " << blocksWrite_));
                    break;
                default:
                    UNREACHABLE;
            }



            ui::App<void>::draw();
        }

        static bool active() {
            return mscStatus_ != Status::Inactive;
        }


    private:
        friend bool ::tud_msc_start_stop_cb(uint8_t, uint8_t, bool, bool);
        friend int32_t ::tud_msc_read10_cb(uint8_t, uint32_t, uint32_t, void *, uint32_t);
        friend int32_t ::tud_msc_write10_cb(uint8_t, uint32_t, uint32_t, uint8_t*, uint32_t);

        ui::Image icon_;
        ui::Label info_;
        ui::Label status_;

        static void connect() {
            ASSERT(mscStatus_ == Status::Disconnected);
            fs::unmount(fs::Drive::SD);
            mscStatus_ = Status::Connected;
            blocksRead_ = 0;
            blocksWrite_ = 0;
        }

        static void disconnect() {
            if (active())
                mscStatus_ = Status::Disconnected;
        }

        enum class Status {
            Inactive, 
            Disconnected, 
            Connected,
        }; 

        static inline Status mscStatus_ = Status::Inactive;
        static inline uint32_t blocksRead_ = 0;
        static inline uint32_t blocksWrite_ = 0;

    }; // DataSync

#ifdef FOO

    /** USB Mass Storage device.  
     
        The app has to be active in order for the mass storage attachment to work. It detaches the SD card from the RCKid async DMA routines and makes it exclusively available to the USB connected PC as a mass storage device. 

        Furthermore, it detects if DC power is applied while the app is on, and if it is, initializes the usb stack. We can'd do this immediately because the ESD protection circuit on the USB leaks the pullups on the dataline to the VCC which then prevents the USB aware devices to connect and charge the RCKid properly. 

        When the app is exitted, the USB device is turned off, which re-enables the DC voltage detection & charging. 

     */
    class DataSync : public CanvasApp<ColorRGB> {
    public:

        static bool active() { return mscActive_; }

    protected:

        DataSync(): 
            CanvasApp<ColorRGB>{}
            /*icon_{std::move(ARENA(Bitmap<ColorRGB>::fromImage(PNG::fromBuffer(assets::icons_64::pen_drive))))} */ {
        }

        void focus() override {
            CanvasApp<ColorRGB>::focus();
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
            CanvasApp<ColorRGB>::blur();
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
            CanvasApp<ColorRGB>::update();
            /*
            // This is too dangerous to have here
            if (btnPressed(Btn::Start)) {
                fs::unmount();
                fs::format();
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

#endif

} // namespace rckid