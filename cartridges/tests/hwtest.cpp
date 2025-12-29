#include <rckid/rckid.h>
#include <rckid/apps/devel/HardwareStatus.h>

using namespace rckid;

/** Tests various hardware features. 
 */
int main() {
    initialize();

    while (true) {
        App::run<HardwareStatus>();
    }
}
