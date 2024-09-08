#include <platform/tests.h>
#include "uart_target_transceiver.h"

using namespace rckid;

class TestDeviceTransceiver : public Transceiver<UART> {
public:
    Packet received; 
    size_t messagesReceived = 0;

    TestDeviceTransceiver(DeviceId ownId): Transceiver<UART>{ownId} {}
protected:

    void onMessageReceived(Packet const & msg) override {
        memcpy(received, msg, sizeof(Packet));
        ++messagesReceived;
    }
   
}; // TestDeviceTransceiver


TEST(comms, RxTxEnable) {
    TestDeviceTransceiver x{67};
    EXPECT(x.ownId() == 67);
    EXPECT(x.enabled() == false);
    x.initialize();
    EXPECT(x.enabled() == false);
    x.enable();
    EXPECT(x.enabled() == true);
    x.enable();
    EXPECT(x.enabled() == true);
    x.disable();
    EXPECT(x.enabled() == false);
    x.disable();
    EXPECT(x.enabled() == false);
}