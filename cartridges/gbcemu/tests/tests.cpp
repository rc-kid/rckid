

#include "rckid/rckid.h"
#include "rckid/app.h"
#include "rckid/graphics/framebuffer.h"

#include "utils/tests.h"

using namespace rckid;

class GBCEmuTests : public FrameBufferApp<ColorRGB> {
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

void rckid_main(int argc, char ** argv) {
    GBCEmuTests{argc, argv}.run();
    std::cout << "KTHXBYE!" << std::endl;
}