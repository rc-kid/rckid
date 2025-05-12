#include "app.h"

namespace rckid {

    void App::loop() {
        // wait for the previous display update to finish to avoid interfering with the old app unloading
        displayWaitUpdateDone();
        // set the current app in focus. If there is previous app, it will be blurred. The focus method also updates the parent app so that we can go back with the apps
        focus();
        // now run the app
        while (app_ == this) {
            tick();
            update();
            displayWaitUpdateDone();
            draw();
        }
        // wait for the last display update to finish so that the display routine does not interfere with the app unloading
        displayWaitUpdateDone();
        // we are done, should blur ourselves, and refocus parent (if any)
        blur();
    }
}