#pragma once

#include "ui/header.h"
#include "task.h"
#include "assets/tiles/System16.h"
#include "apps/utils/Steps.h"

#include "filesystem.h"

namespace rckid {

    class HeartbeatTask : public Task {
    public:

        HeartbeatTask() {
            // add current time to some file
            fs::FileWrite f{fs::fileAppend("/heartbeat_log.txt")};
            if (f.good()) {
                TinyDateTime now = timeNow();
                f.writer() << now.date.day() << "/" << now.date.month() << "/" << now.date.year() << " "
                           << now.time.hour() << ":" << now.time.minute() << ":" << now.time.second() << "\n";
            }
            // clear pedometer
            Steps::onHeartbeat();
        }

    protected: 

        void tick() override {
            if (--ticks_ == 0)
                delete this;
        }

        Coord updateHeader(ui::Header & header, Coord offset) override { 
            header.at(--offset, 0).setPaletteOffset(ui::Header::PALETTE_ACCENT + 1) = assets::SYSTEM16_MESSAGE_RIGHT;
            return offset;
        }

        uint32_t ticks_ = 60 * 60;
    };

} // namespace rckid