#include <rckid/rckid.h>

using namespace rckid;

/** Tests that assertion failures end up being blue screens of death. 
 
    This should be one of the first tests to run as it verifies the assertion mechanism used extensively in the SDK code is working as intended.
 */
int main() {
    initialize();
    ASSERT(false);
    while (true)
        yield();
}