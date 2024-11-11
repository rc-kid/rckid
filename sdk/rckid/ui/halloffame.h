#pragma once

#include <string>
#include <vector>

#include "../rckid.h"
#include "../utils/stream.h"
#include "../graphics/canvas.h"

#include "../assets/fonts/OpenDyslexic48.h"
#include "../assets/fonts/Iosevka24.h"

namespace rckid {

    /** A simple hall of fame table. 
     
        Table of names & high scores that supports basic hall of fame operations and serialization/deserialization for storage. 
     */
    class HallOfFame {
    public:

        /** Initializes the hall of fame with empty table. 
         */
        HallOfFame() = default;

        bool add(std::string const & name , uint32_t score) {
            size_t iAt = table_.size();
            while (iAt > 0) {
                if (table_[iAt - 1].score > score)
                    break;
                --iAt;
            }
            // try harder next time
            if (iAt >= maxEntries_)
                return false;
            table_.insert(table_.begin() + iAt, Record{name, score});
            // ensure the table is not infinite
            while (table_.size() > maxEntries_)
                table_.pop_back();
            return true;
        }

        bool isHighEnough(uint32_t score) {
            size_t iAt = table_.size();
            while (iAt > 0) {
                if (table_[iAt - 1].score > score)
                    break;
                --iAt;
            }
            return iAt < maxEntries_;
        }

        /** Serializes the current hall of fame entries to the given stream. 
         */
        void serializeTo(WriteStream & s) {
            s.serialize<uint32_t>(table_.size());
            for (auto const & i : table_) {
                s.serialize(i.name);
                s.serialize(i.score);
            }
        }

        /** Deserializes the hall of fame entries from given stream. 
         */
        void deserializeFrom(ReadStream & s) {
            uint32_t n = s.deserialize<uint32_t>();
            table_.clear();
            for (; n > 0; --n) {
                std::string name = s.deserialize<std::string>();
                uint32_t score = s.deserialize<uint32_t>();
                table_.push_back(Record{name, score});
            }
            // ensure the table is not infinite
            while (table_.size() > maxEntries_)
                table_.pop_back();
        }

        /** Draws the hall of fame on the provided canvas. 
         */
        void drawOn(Canvas<ColorRGB> & g) {
            g.text(160 - titleFont_->textWidth(title_.c_str()) / 2, 20, *titleFont_, titleColor_) << title_;
            int y = 20 + titleFont_->size + 10;
            for (auto const & i : table_) {
                g.text(20, y, *tableFont_, tableColor_) << i.name;
                g.text(260, y, *tableFont_, tableColor_) << i.score;
                y += tableFont_->size + 5;
            }
        }

    private:

        struct Record {
            std::string name;
            uint32_t score;
        }; 

        Font const * titleFont_ = & assets::font::OpenDyslexic48::font;
        ColorRGB titleColor_ = color::White;

        Font const * tableFont_ = & assets::font::Iosevka24::font;
        ColorRGB tableColor_ = color::White;

        uint32_t maxEntries_ = 5;
        std::string title_{"Hall of Fame"};
        std::vector<Record> table_;

    }; // rckid::HallOfFame

} // namespace rckid