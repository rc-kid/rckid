#pragma once

#include <rckid/ui/app.h>
#include <rckid/ui/label.h>
#include <rckid/ui/image.h>
#include <assets/Baloo128.h>

namespace rckid {

    /** Simple splashscreen showing the kidDo wink. 
     
        Shows the kid:Do text in white, then calls the callback function for hardware initialization. After HW initialization is finished, shows the wink animation with fade from white to green for the ;D and then fades the whole logo out and exits. 
     */
    class SplashScreen : public ui::App<void> {
    public:

        /** Callback for the HW initializer. 
         */
        typedef void (*InitCallback)();

        String name() const override { return "SplashScreen"; }

        /** Creates the splashscreen application with the given callback for initialization routine after the initial fade in.
         */
        SplashScreen(InitCallback cb):
            cb_{cb}
        {
            using namespace ui;
            with(root_)
                << SetBg(Color::Black())
                << UseHeader(Header::Visibility::Never)
                << UseBackgroundImage(false);
            k_ = addChild(new ui::Label{})
                << SetText("k")
                << SetFont(assets::Baloo128)
                << SetRect(Rect::XYWH(44, 56, 44, 128));
            i_ = addChild(new ui::Label{})
                << SetText("i")
                << SetFont(assets::Baloo128)
                << SetRect(Rect::XYWH(88, 56, 22, 128));
            d_ = addChild(new ui::Label{})
                << SetText("d")
                << SetFont(assets::Baloo128)
                << SetRect(Rect::XYWH(110, 56, 47, 128));
            // comma for the wink, needs to appear before the colon for proper rendering
            comma_ = addChild(new ui::Label{})
                << SetText(",")
                << SetFg(Color::Black())
                << SetFont(assets::Baloo128)
                << SetRect(Rect::XYWH(158, 56, 19, 128));
            colon_ = addChild(new ui::Label{})
                << SetText(":")
                << SetFont(assets::Baloo128)
                << SetUseAlpha(true)
                << SetRect(Rect::XYWH(157, 56, 19, 128));
            D_ = addChild(new ui::Label{})
                << SetText("D")
                << SetFont(assets::Baloo128)
                << SetRect(Rect::XYWH(176, 62, 53, 128));
            o_ = addChild(new ui::Label{})
                << SetText("o")
                << SetFont(assets::Baloo128)
                << SetRect(Rect::XYWH(229, 56, 47, 128));
        }

    protected:

        void onLoopStart() override {
            ui::App<void>::onLoopStart();
            animate()
                << ChangeFg(k_, Color::Black(), Color::White())
                << ChangeFg(i_, Color::Black(), Color::White())
                << ChangeFg(d_, Color::Black(), Color::White())
                << ChangeFg(colon_, Color::Black(), Color::White())
                << ChangeFg(D_, Color::Black(), Color::White())
                << ChangeFg(o_, Color::Black(), Color::White());
            hal::display::setBrightness(128);
        }

        void loop() override {
            // fade in
            waitUntilIdle();
            // call the rest if the initialization
            if (cb_ != nullptr)
                cb_();
            // wink
            // TODO make the wink use accent color of the device so that its identical to the buttons
            animate()
                << ChangeFg(comma_, Color::Black(), Color::Green())
                << ChangeFg(colon_, Color::White(), Color::Green())
                << ChangeFg(D_, Color::White(), Color::Green());
            waitUntilIdle();
            animate()
                << ChangeFg(comma_, Color::Green(), Color::Black())
                << ChangeFg(colon_, Color::Green(), Color::White())
                << ChangeFg(D_, Color::Green(), Color::White());
            waitUntilIdle();
            // fade out
            animate()
                << ChangeFg(k_, Color::White(), Color::Black())
                << ChangeFg(i_, Color::White(), Color::Black())
                << ChangeFg(d_, Color::White(), Color::Black())
                << ChangeFg(colon_, Color::White(), Color::Black())
                << ChangeFg(D_, Color::White(), Color::Black())
                << ChangeFg(o_, Color::White(), Color::Black());
            waitUntilIdle();
            exit();
        }

    private:

        InitCallback cb_;

        ui::Label * k_;
        ui::Label * i_;
        ui::Label * d_;
        ui::Label * colon_;
        ui::Label * D_;
        ui::Label * o_;
        ui::Label * comma_;

    }; // rckid::SplashScreen

} // namespace rckid