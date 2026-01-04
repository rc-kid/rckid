#pragma once

#include "../../ui/form.h"
#include "../../ui/geometry.h"
#include "../../ui/tilemap.h"
#include "../../assets/tiles/System24.h"

namespace rckid {

    /** Popup menu. 
     
        The popup menu is a simple menu visualizer. 
     */
    template<typename PAYLOAD>
    class PopupMenu : public ui::Form<PAYLOAD> {
    public:

        /** It's ok that all popup menus have the same name as they do not use any of the name associated functionality. 
         */
        String name() const override { return "PopupMenu"; }

        static constexpr Coord TileWidth = 12;
        static constexpr Coord TileHeight = 24;

        static bool runAndExecute(ui::Menu<PAYLOAD> * menu) {
            auto result = App::run<PopupMenu<PAYLOAD>>(menu);
            if (result.has_value()) {
                auto & action = result.value();
                if (action != nullptr) {
                    action();
                    return true;
                }
            }
            return false;
        }

        PopupMenu(ui::Menu<PAYLOAD> * menu):
            ui::Form<PAYLOAD>{Rect::XYWH(0, 240 - getNumRows(menu) * TileHeight - 4, getLongestText(menu) * TileWidth + 4, getNumRows(menu) * TileHeight + 4), /* raw */ true},
            menu_{menu} {
            using namespace ui;
            g_.setBg(ui::Style::bg());
            selectedRect_ = g_.addChild(new Rectangle{Rect::XYWH(2, 2, g_.width() - 4, TileHeight)});
            selectedRect_->setColor(ui::Style::accentBg());
            selectedRect_->setFill(true);
            tm_ = g_.addChild(new Tilemap<Tile<12, 24, Color16>>{getLongestText(menu), getNumRows(menu), assets::System24, palette_});
            tm_->setPos(2,2);
            fillText();
        }

        void update() override {
            // if back button is selected, do return nullopt
            if (btnPressed(Btn::B))
                exit();
            if (btnPressed(Btn::A)) {
                // return the selected item
                auto & item = (*menu_)[selected_];
                ASSERT(item.isAction()); // generators not supported yet
                exit(item.action());
            }
            if (btnPressed(Btn::Up)) {
                if (selected_ > 0)
                    --selected_;
                else 
                    selected_ = menu_->size() - 1;
                updateSelection();
            }
            if (btnPressed(Btn::Down)) {
                if (selected_ < menu_->size() - 1)
                    ++selected_;
                else 
                    selected_ = 0;
                updateSelection();
            }
        }

    private:

        using ui::Form<PAYLOAD>::g_;
        using ui::Form<PAYLOAD>::exit;
        using ui::Form<PAYLOAD>::parent;

        static Coord getLongestText(ui::Menu<PAYLOAD> * menu) {
            uint32_t longest = 0;
            for (auto const & item : *menu) {
                if (item.text.size() > longest)
                    longest = item.text.size();
            }
            return longest;
        }

        static Coord getNumRows(ui::Menu<PAYLOAD> * menu) {
            return menu->size() > 6 ? 6 : menu->size(); 
        }

        void fillText() {
            tm_->clear();
            for (Coord i = 0; i < getNumRows(menu_); ++i) {
                tm_->text(0, i) << (*menu_)[i + top_].text;
            }
        }

        void updateSelection() {
            if (selected_ < top_) {
                top_ = selected_;
                fillText();
            } else if (selected_ >= top_ + getNumRows(menu_)) {
                top_ = selected_ - getNumRows(menu_) + 1;
                fillText();
            }
            selectedRect_->setPos(2, 2 + (selected_ - top_) * TileHeight);
        }

        ui::Menu<PAYLOAD> * menu_; 
        ui::Rectangle * selectedRect_; 
        ui::Tilemap<Tile<12, 24, Color16>> * tm_;
        uint32_t top_ = 0;
        uint32_t selected_ = 0;

        static constexpr uint16_t palette_[] = {
            // gray
            ColorRGB{0x00, 0x00, 0x00}.toRaw(), 
            ColorRGB{0x11, 0x11, 0x11}.toRaw(), 
            ColorRGB{0x22, 0x22, 0x22}.toRaw(), 
            ColorRGB{0x33, 0x33, 0x33}.toRaw(), 
            ColorRGB{0x44, 0x44, 0x44}.toRaw(), 
            ColorRGB{0x55, 0x55, 0x55}.toRaw(), 
            ColorRGB{0x66, 0x66, 0x66}.toRaw(), 
            ColorRGB{0x77, 0x77, 0x77}.toRaw(), 
            ColorRGB{0x88, 0x88, 0x88}.toRaw(), 
            ColorRGB{0x99, 0x99, 0x99}.toRaw(), 
            ColorRGB{0xaa, 0xaa, 0xaa}.toRaw(), 
            ColorRGB{0xbb, 0xbb, 0xbb}.toRaw(), 
            ColorRGB{0xcc, 0xcc, 0xcc}.toRaw(), 
            ColorRGB{0xdd, 0xdd, 0xdd}.toRaw(), 
            ColorRGB{0xee, 0xee, 0xee}.toRaw(), 
            ColorRGB{0xff, 0xff, 0xff}.toRaw(), 
            0, 
            ColorRGB{0xff, 0xff, 0xff}.toRaw(), 
            ColorRGB{0x00, 0xff, 0x00}.toRaw(),
        };

    }; // rckid::PopupMenu



} // namespace rckid