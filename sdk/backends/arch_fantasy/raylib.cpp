/** \page raylib_backend Raylib backend
 
    A fantasy console backend that uses RayLib for the graphics, audio and other aspects of the device. Should work anywhere raylib does. 
 */

#ifndef ARCH_FANTASY
#error "You are building fantasy (RayLib) backend without the indicator macro"
#endif


#include <cstdlib>
#include <iostream>

#include <raylib.h>

#include "rckid/rckid.h"

namespace rckid {

    void displayDraw();

    void initialize() {
        InitWindow(640, 480, "RCKid");
        SetTargetFPS(60);
    }

    void tick() {
        if (WindowShouldClose())
            std::exit(-1);
        displayDraw();
    }

    Writer debugWrite() {
        return Writer([](char c) {
            std::cout << c;
            if (c == '\n')
                std::cout.flush();
        });
    }

    namespace {
        DisplayMode displayMode_ = DisplayMode::Off;
        DisplayUpdateCallback displayCallback_;
        uint8_t displayBrightness_ = 255;
        size_t displayUpdating_ = 0; 
        Rect displayRect_ = Rect::WH(320, 240);
        size_t displayIdx_ = 0;
        size_t displayMax_ = 320 * 240;
        ColorRGB framebuffer_[320 * 240];
    }

    void drawPixel(int x, int y, ColorRGB c) {
        Color rc;
        rc.a = 255;
        rc.r = c.r();
        rc.g = c.g();
        rc.b = c.b();
        DrawRectangle(x * 2, y * 2, 2, 2, rc);
    }

    void displayDraw() {
        BeginDrawing();
        ColorRGB * c = framebuffer_;
        switch (displayMode_) {
            case DisplayMode::Native: {
                for (int x = displayRect_.w - 1; x >= 0; --x)
                    for (int y = 0; y < displayRect_.h; ++y)
                        drawPixel(displayRect_.x + x, displayRect_.y + y, *c++);
                break;
            }
            case DisplayMode::NativeDouble:
            case DisplayMode::Natural:
            case DisplayMode::NaturalDouble:
                UNIMPLEMENTED;
            default:
                UNREACHABLE;
        }
        EndDrawing();
    }

    DisplayMode displayMode() { return displayMode_; }

    void displaySetMode(DisplayMode mode) {
        displayMode_ = mode;
        // TODO if display is off, mark it as off 
    }

    uint8_t displayBrightness() { return displayBrightness_; }

    void displaySetBrightness(uint8_t value) { displayBrightness_ = value; }

    Rect displayUpdateRegion() { return displayRect_; }

    void displaySetUpdateRegion(Rect region) { 
        displayRect_ = region; 
        displayMax_ = region.w * region.h;
        ASSERT(region.w > 0 && region.h > 0 && region.x >= 0 && region.y >= 0);
        ASSERT(region.bottom() <= 240 && region.right() <= 320);
    }

    bool displayUpdateActive() { return displayUpdating_ > 0; }

    void displayUpdate(ColorRGB const * pixels, uint32_t numPixels) {
        ++displayUpdating_;
        // update the pixels
        while (numPixels != 0) {
            framebuffer_[displayIdx_++] = *(pixels++);
            if (displayIdx_ >= displayMax_)
                displayIdx_ = 0;
            --numPixels;
        }
        // check if this is the first update call, in which case call all the other updates (as long as the callback generates a new update) and when no more updates are scheduled, actually redraw the display. Note that if the update is not the first, no callbacks are called
        if (displayUpdating_ == 1) {
            while (true) {
                size_t updatingOld = displayUpdating_;
                if (displayCallback_)
                    displayCallback_();
                if (updatingOld == displayUpdating_)
                    break;
            }
            displayUpdating_ = 0;
            displayDraw();
        }
    }

    void displayUpdate(ColorRGB const * pixels, uint32_t numPixels, DisplayUpdateCallback callback) {
        displayCallback_ = callback;
        displayUpdate(pixels, numPixels);
    }
}