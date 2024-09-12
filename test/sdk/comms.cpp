
#include <thread>
#include <chrono>

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
    FantasyUART::reset();
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

TEST(comms, PingTx) {
    FantasyUART::reset();
    unsigned msgs = 0;
    TestDeviceTransceiver x{67};
    x.enable();
    x.send(BroadcastId, msg::Ping{67, 12, 0x3456}, [&](bool success, Packet const & p) mutable {
        if (success)
            ++msgs;
        EXPECT(p[0] == static_cast<uint8_t>(msg::Ping::ID));
        auto & msg = msg::Ping::fromBuffer(p);
        EXPECT(msg.sender == 67);
        EXPECT(msg.index == 12);
        EXPECT(msg.userId == 0x3456);
        EXPECT(msg.userName[0] == 0);
    });
    EXPECT(msgs == 1);
    Packet p;
    EXPECT(FantasyUART::targetRx(p));
    EXPECT(p[0] == static_cast<uint8_t>(msg::Ping::ID));
    auto & msg = msg::Ping::fromBuffer(p);
    EXPECT(msg.sender == 67);
    EXPECT(msg.index == 12);
    EXPECT(msg.userId == 0x3456);
    EXPECT(msg.userName[0] == 0);
}

TEST(comms, Ack) {
    FantasyUART::reset();
    int acks = 0;
    TestDeviceTransceiver x{67};
    x.enable();
    x.send(68, msg::ConnectionOpen{67, 1, 2}, [&](bool success, Packet const & p) mutable {
        acks += success ? 1 : -1; 
        EXPECT(p[0] == static_cast<uint8_t>(msg::ConnectionOpen::ID));
        auto & msg = msg::ConnectionOpen::fromBuffer(p);
        EXPECT(msg.sender == 67);
        EXPECT(msg.requestId == 1);
        EXPECT(msg.param == 2);
    });
    EXPECT(FantasyUART::targetRxSize() == 1);
    // no ack received yet
    EXPECT(acks == 0);
    // read & verify the sent message
    Packet p;
    EXPECT(FantasyUART::targetRx(p));
    EXPECT(FantasyUART::targetRxSize() == 0);
    EXPECT(p[0] == static_cast<uint8_t>(msg::ConnectionOpen::ID));
    auto & msg = msg::ConnectionOpen::fromBuffer(p);
    EXPECT(msg.sender == 67);
    EXPECT(msg.requestId == 1);
    EXPECT(msg.param == 2);
    // send ack
    new (&p) msg::Ack{};
    FantasyUART::targetTx(p);
    // still no ack received yet
    EXPECT(acks == 0);
    EXPECT(FantasyUART::deviceRxSize() == 1);
    // do the loop to check the messages and then check that finally we have the ack
    x.loop();
    EXPECT(FantasyUART::deviceRxSize() == 0);
    EXPECT(x.messagesReceived == 0);
    EXPECT(acks == 1);
}

TEST(comms, AckTimeout) {
    FantasyUART::reset();
    int acks = 0;
    TestDeviceTransceiver x{67};
    x.enable();
    x.send(68, msg::ConnectionOpen{67, 1, 2}, [&](bool success, Packet const & p) mutable {
        acks += success ? 1 : -1; 
        EXPECT(p[0] == static_cast<uint8_t>(msg::ConnectionOpen::ID));
        auto & msg = msg::ConnectionOpen::fromBuffer(p);
        EXPECT(msg.sender == 67);
        EXPECT(msg.requestId == 1);
        EXPECT(msg.param == 2);
    });
    EXPECT(FantasyUART::targetRxSize() == 1);
    // no ack received yet
    EXPECT(acks == 0);
    // no need to read & verify, just wait long enough
    std::this_thread::sleep_for(std::chrono::microseconds(UART_TX_TIMEOUT_US * 2));
    EXPECT(acks == 0);
    x.loop();    
    EXPECT(acks == -1);
}

TEST(comms, UARTTargetAutoAck) {
    FantasyUART::reset();
    int acks = 0;
    TestDeviceTransceiver x{67};
    UARTTargetTransceiver y{68};
    x.enable();
    x.send(68, msg::ConnectionOpen{67, 1, 2}, [&](bool success, Packet const & p) mutable {
        acks += success ? 1 : -1; 
        EXPECT(p[0] == static_cast<uint8_t>(msg::ConnectionOpen::ID));
        auto & msg = msg::ConnectionOpen::fromBuffer(p);
        EXPECT(msg.sender == 67);
        EXPECT(msg.requestId == 1);
        EXPECT(msg.param == 2);
    });
    EXPECT(FantasyUART::targetRxSize() == 1);
    // no ack received yet
    EXPECT(acks == 0);
    y.loop();
    x.loop();
    
    EXPECT(acks == 1);

}