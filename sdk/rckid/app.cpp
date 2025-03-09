#include "app.h"

namespace rckid {

    void App::run() {
        // set the current app in focus. If there is previous app, it will be blurred. The focus method also updates the parent app so that we can go back with the apps
        focus();
        // now run the app
        while (app_ == this) {
            tick();
            update();
            displayWaitUpdateDone();
            draw();
        }
        // we are done, should blur ourselves, and refocus parent (if any)
        blur();
    }
}