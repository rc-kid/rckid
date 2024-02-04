

#include "rckid/rckid.h"
#include "rckid/app.h"
#include "rckid/graphics/framebuffer.h"

#include "utils/tests.h"

using namespace rckid;

class GBCEmuTests : public App<FrameBuffer<ColorRGB>> {
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

int main(int argc, char ** argv) {
    initialize();
    start(GBCEmuTests{argc, argv});
    std::cout << "KTHXBYE!" << std::endl;
}