#include <rckid/ui/app.h>

int main() {
    rckid::initialize();
    rckid::ui::App<void> app;
    app.run();
}