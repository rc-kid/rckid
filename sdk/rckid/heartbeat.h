#pragma once

#include "ui/header.h"
#include "task.h"
#include "assets/tiles/System16.h"

namespace rckid {

    class HeartbeatTask : public Task {
    public:

        HeartbeatTask() = default;  

    protected: 

        void tick() override {
            if (--ticks_ == 0)
                delete this;
        }

        Coord updateHeader(ui::Header & header, Coord offset) override { 
            header.at(--offset, 0).setPaletteOffset(ui::Header::PALETTE_ACCENT + 1) = assets::SYSTEM16_MESSAGE_RIGHT;
            return offset;
        }

        uint32_t ticks_ = 20 * 60;
    };

} // namespace rckid