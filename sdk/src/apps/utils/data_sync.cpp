#include <rckid/apps/utils/data_sync.h>

namespace rckid {

    namespace fs::internal {

        uint32_t fatPartitionStart();

        uint32_t fatPartitionSize();

    } // namespace rckid::fs::internal


    DataSync::DataSync() {
        using namespace ui;
        icon_ = addChild(new Image())
            << SetRect(Rect::XYWH(0, 60, 320, 64))
            << SetBitmap(assets::icons_64::pen_drive);
        info_ = addChild(new Label())
            << SetRect(Rect::XYWH(0, 140, 320, 24))
            << SetFont(assets::Iosevka24)
            << SetHAlign(HAlign::Center);
        status_ = addChild(new Label())
            << SetRect(Rect::XYWH(0, 160, 320, 24))
            << SetText("Disconnected")
            << SetFont(assets::Iosevka24)
            << SetHAlign(HAlign::Center);

        ASSERT(instance_ == nullptr);

        fs::mount(fs::Drive::SD);
        // TODO check also the SD card inserted pin
        if (fs::isMounted()) {
            String label = fs::getLabel();
            info_->setText(STR(
                (label.empty() ? "SD card" : label.c_str())
                << ", " << hal::fs::sdCapacityBlocks() / 2 / 1024 << "MB"
            ));
            partitionStart_ = fs::internal::fatPartitionStart();
            partitionSize_ = fs::internal::fatPartitionSize();
            LOG(LL_INFO, "DataSync partition start: " << partitionStart_);
            LOG(LL_INFO, "DataSync partition size:  " << partitionSize_);
            fs::unmount();
            connected_ = false;
            instance_ = this;
        } else {
            info_->setText("No Card");
        }

    }

} // namespace rckid