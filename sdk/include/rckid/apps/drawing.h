#pragma once

#include <rckid/ui/app.h>
#include <rckid/ui/canvas_view.h>
#include <rckid/ui/color_picker.h>
#include <rckid/ui/focus_rect.h>
#include <rckid/ui/panel.h>

namespace rckid {

    /** Simple single image editor. 
     
        The single image editor allows editing arbitrary sized images, one image at a time. It does not contain preview, nor does it contain 
     
     */
    class Drawing : public ui::App<void> {
    public:

        String name() const override { return "Drawing"; }

        Drawing(Canvas * canvas):
            ui::App<void>(),
            canvas_{canvas} 
        {
            using namespace ui;
            edit_ = addChild(new CanvasEdit{})
                << SetRect(Rect::XYWH(105, 25, 210,210))
                << SetCanvas(canvas_)
                << SetZoom(8);
            view_ = addChild(new CanvasView{})
                << SetRect(Rect::XYWH(5, 25, 95, 95))
                << SetHAlign(HAlign::Center)
                << SetVAlign(VAlign::Center)
                << SetCanvas(canvas_)
                << SetZoom(1);
            colorA_ = addChild(new Panel())
                << SetRect(Rect::XYWH(5, 100, 35, 35))
                << SetBg(Color::Black());
            colorB_ = addChild(new Panel())
                << SetRect(Rect::XYWH(60, 100, 35, 35))
                << SetBg(Color::White());
            colorPicker_ = addChild(new ColorPicker{})
                << SetRect(Rect::XYWH(5, 140, 95, 95));
        }

    private:

        void onFocus() override {
            ui::App<void>::onFocus();
            focusWidget(edit_);
        }

        void loop() override {
            ui::App<void>::loop();
            if (btnPressed(Btn::Start)) {
                if (edit_->focused()) {
                    focusWidget(colorPicker_);
                } else {
                    focusWidget(edit_);
                }
            }
            if (btnPressed(Btn::A)) {
                if (edit_->focused()) {
                    canvas_->at(edit_->pos()) = colorA_->bg();
                } else if (colorPicker_->focused()) {
                    colorA_->setBg(colorPicker_->selected());
                }
            }
            if (btnPressed(Btn::B)) {
                if (edit_->focused()) {
                    canvas_->at(edit_->pos()) = colorB_->bg();
                } else if (colorPicker_->focused()) {
                    colorB_->setBg(colorPicker_->selected());
                }
            }
        }

        Canvas * canvas_ = nullptr;
        ui::CanvasView * view_ = nullptr;
        ui::CanvasEdit * edit_ = nullptr;
        ui::ColorPicker * colorPicker_ = nullptr;
        ui::Panel * colorA_ = nullptr;
        ui::Panel * colorB_ = nullptr;

    }; // rckid::Drawing

} // namespace rckid