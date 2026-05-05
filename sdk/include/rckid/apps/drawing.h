#pragma once

#include <rckid/ui/app.h>
#include <rckid/ui/canvas_view.h>
#include <rckid/ui/focus_rect.h>

namespace rckid {

    /** Simple single image editor. 
     
        The single image editor allows editing arbitrary sized images, one image at a time. It does not contain preview, nor does it contain 
     
     */
    class Drawing : public ui::App<void> {
    public:

        String name() const override { return "Drawing"; }

        Drawing():
            ui::App<void>() 
        {
            using namespace ui;
            edit_ = addChild(new CanvasEdit{})
                << SetRect(Rect::XYWH(105, 25, 210,210))
                << SetCanvas(canvas_.get())
                << SetZoom(4);
            view_ = addChild(new CanvasView{})
                << SetRect(Rect::XYWH(5, 25, 100, 100))
                << SetCanvas(canvas_.get())
                << SetZoom(1);
        }

    private:

        void onFocus() override {
            ui::App<void>::onFocus();
            focusWidget(edit_);
        }
        unique_ptr<Canvas> canvas_{new Canvas{64,64}};
        ui::CanvasView * view_ = nullptr;
        ui::CanvasEdit * edit_ = nullptr;

    }; // rckid::Drawing

} // namespace rckid