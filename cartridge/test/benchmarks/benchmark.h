#pragma once

#include "rckid/app.h"
#include "rckid/stats.h"
#include "rckid/graphics/framebuffer.h"

/** Simple benchmarking app. 
 */
class BenchmarkDriver : public App<FrameBuffer<ColorRGB_332>> {
public:
}; // BenchmarkDriver