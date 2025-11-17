#pragma once

#include "../../app.h"
#include "../../ui/form.h"
#include "../../ui/label.h"
#include "../../ui/image.h"
#include "../../assets/fonts/OpenDyslexic128.h"
#include "../../assets/icons_24.h"



namespace rckid {

    /** Alarm app
     
     */
    class Alarm : public ui::Form<void> {
    public:
    
        String name() const override { return "Alarm"; }

        Alarm(): 
            ui::Form<void>{},
            h_{Rect::XYWH(0, 30, 150, 130), ""},
            m_{Rect::XYWH(170, 30, 150, 130), ""},
            colon_{Rect::XYWH(150, 30, 20, 130), ":" }
        {
            g_.addChild(h_);
            h_.setFont(Font::fromROM<assets::OpenDyslexic128>());
            h_.setHAlign(HAlign::Right);
            g_.addChild(m_);
            m_.setFont(Font::fromROM<assets::OpenDyslexic128>());
            m_.setHAlign(HAlign::Left);
            g_.addChild(colon_);
            colon_.setFont(Font::fromROM<assets::OpenDyslexic128>());

            contextMenu_.add(ui::ActionMenu::Item("Set alarm"));
        }

    protected:


    private:
        ui::Label h_;
        ui::Label m_;
        ui::Label colon_;
        ui::ActionMenu contextMenu_;

    }; // rckid::Alarm



} // namespace rckid