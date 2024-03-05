

#include "rckid/rckid.h"
#include "rckid/app.h"
#include "rckid/graphics/framebuffer.h"

#include "utils/tests.h"

using namespace rckid;

class GBCEmuTests : public FBApp<FrameBuffer<ColorRGB>> {
public:
    GBCEmuTests(int argc, char ** argv): argc_{argc}, argv_{argv} {}
private:

    void update() override {
        status_ = Test::RunAll(argc_, argv_);
        exit();
    }

    void draw() override {
    }

    int argc_;
    char ** argv_;
    int status_ = 0;
}; // GBCEmuTests

void rckid_main() {
    // argc, argv
    GBCEmuTests{0, nullptr}.run();
    std::cout << "KTHXBYE!" << std::endl;
}