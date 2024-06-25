#pragma once

#include "rckid/app.h"
#include "rckid/stats.h"
#include "rckid/graphics/framebuffer.h"
#include "rckid/ui/header.h"
#include "rckid/audio/tone.h"
#include "rckid/audio/music.h"

namespace rckid {

    class Melody : public App<FrameBuffer<ColorRGB>> {
    public:
        static Melody * create() { return new Melody{}; }

    protected:

        void onFocus() override {
            App::onFocus();
            music_.setTempo(Tempo::Vivace); 
            audio::play(&music_);
        }

        void onBlur() override {
            App::onBlur();
            audio::stop();
            
        }

        void update() override {
            App::update();
        }

        void draw() override {
            driver_.fill();
            header_.drawOn(driver_, Rect::WH(320, 20));
        }

    private:

        Music<Tone<SineWave>> music_{melody_};
        Header<Color> header_;


        static constexpr NoteInfo melody_[] = {
            NOTE_2(E4), 
            NOTE_4(E4),
            NOTE_4(D4),
            NOTE_2(C4), 
            NOTE_4(C4),
            REST_4,
            NOTE_4(D4),
            NOTE_4(D4),
            NOTE_4(E4),
            NOTE_4(F4),
            NOTE_4(E4), 
            NOTE_4(D4),
            NOTE_4(C4),
            REST_4,
            NOTE_4(E4), 
            NOTE_4(G4), 
            NOTE_4(E4),
            NOTE_4(D4),
            NOTE_4(C4),
            NOTE_4(B3),
            NOTE_4(A3),
            REST_8,
            NOTE_8(C4),
            NOTE_4(D4),
            NOTE_4(C4),
            NOTE_4(E4),
            NOTE_4(D4),
            NOTE_2L(C4),
            REST_4,
            NOTE_4(E4),
            NOTE_4(E4),
            NOTE_4(E4),
            NOTE_4(D4),
            NOTE_2(C4),
            NOTE_4(C4),
            NOTE_4(C4),
            NOTE_4(D4),
            NOTE_4(D4),
            NOTE_4(D4),
            NOTE_4(F4),
            NOTE_4(E4),
            NOTE_4(D4),
            NOTE_4(C4),
            REST_4,
            NOTE_4(E4),
            NOTE_4(G4),
            NOTE_4(E4),
            NOTE_4(D4),
            NOTE_4(C4),
            NOTE_4(B3),
            NOTE_4(A3),
            REST_8,
            NOTE_8(C4),
            NOTE_4(D4),
            NOTE_4(D4),
            NOTE_4(E4),
            NOTE_4(D4),
            NOTE_2L(C4),
            REST_4,

            NOTE_2(E4), 
            NOTE_4(E4),
            NOTE_4(D4),
            NOTE_2(C4), 
            NOTE_4(C4),
            REST_4,
            NOTE_4(D4),
            NOTE_4(D4),
            NOTE_4(E4),
            NOTE_4(F4),
            NOTE_4(E4), 
            NOTE_4(D4),
            NOTE_4(C4),
            REST_4,
            NOTE_4(E4), 
            NOTE_4(G4), 
            NOTE_4(E4),
            NOTE_4(D4),
            NOTE_4(C4),
            NOTE_4(B3),
            NOTE_4(A3),
            REST_8,
            NOTE_8(C4),
            NOTE_4(D4),
            NOTE_4(C4),
            NOTE_4(E4),
            NOTE_4(D4),
            NOTE(C4),
            
            REST,

        };
    }; // rckid::Melody


} // namespace rckid