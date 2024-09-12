#pragma once

#include "../rckid.h"

#include "messages.h"

/** \defgroup Communication
 
    RCKid provides a simple interface for sending packets of data, upon which more complex communication protocols are built in the SDK. As the main purpose of the communication libraries is to be used with the radio enabled cartridges (ESP8266 via UART, LoRa and NRF24L01p), the basic communications interface closely follows the lowest common denominator. Packets of up to 32 bytes can be sent to one of 255 devices, or to everyone listening. A packet that is not a broadcast can request acknowledgement so sender can be sure the package has been received by the sender. The API is simple on purpose sacrificing high bandwidth fo ease of use and hardware independence. 

    The Transceiver<HARDWARE> class provides basic interface for handling packet transmission, async & blocking ack reporting, receiving messages and harware initialization & power. To implement new communications hardware, the Transceiver must be specialized to the new HW marker class either via full specialization, or by simply providing the implementations for the necessary methods. 

    Step above the Transceiver sits the connection manager that facilitates multi-packet connection persistence and broadcasting.

 */

namespace rckid {

    /** In its simplest form, the packet is just 32 bytes of data.
     */
    using Packet = uint8_t[32];

    /** The acknowledge callback. Called when the target device either acknowledged, or faled to acknowledge the last sent message. 
     */
    using AckCallback = std::function<void(bool, Packet const &)>;

    /** Message transceiver template. 
     
        Provides the necessary scaffolding for packet transmission and defines basic HW interface that must be implemented for each communication hardware to be compatible with the SDK. This can be done by either specializing the necessary functions (see below), or for more complex communication features specializing the entire class (e.g. when multiple buffers are necessary for more than one messages in flight, etc.)
        
     */
    template<typename HARDWARE> 
    class Transceiver {
    public:

        /** When creating a transceiver, own device id has to be provided. 
         */
        Transceiver(DeviceId ownId): ownId_{ownId} {}

        DeviceId ownId() const { return ownId_; }

        /** Returns whether the communication hardware is enabled or disabled.
         */
        bool enabled() const { return enabled_; }

        /** Enables the transceiver hardware. 
         
            Enables the hardware so that it can receive and send messages. Calling any rx/tx functions before, enabling, or after disabling the HW should result in HW errors (immediately returning false).
         */
        void enable() {
            if (enabled_)
                return;
            enabled_ = enableHardware();
        }

        /** Disables the transceiver hardware. 
         
            Disables the hardware to conserve power. Calling any rx/tx functions before, enabling, or after disabling the HW should result in HW errors (immediately returning false). 
         */
        void disable() {
            if (!enabled_)
                return;
            disableHardware();
            enabled_ = false;
        }

        /** Initializes the transceiver's hardware. 
         
            Specialization of this method must be provided. The function must be called before any other transceiver methods can be executed.  
         */
        void initialize() {}

        /** Implementation of the polling mode. 

            TODO maybe does not have to be provided, if using IRQs but not sure yet how IRQs will be used, if at all? Sync issues, etc.             
         */
        void loop();

        /**
         
           - copy message to buffer
           - if ackable, 
         */
        template<typename MSG>
        bool send(DeviceId target, MSG const & message, AckCallback cb) {
            ASSERT(!ackCb_); // only one message in flight
            // set callback & copy the message to the buffer
            ackCb_ = cb;
            memcpy(buffer_, & message, sizeof(MSG));
            // transmit via HW
            txTimeout_ = uptimeUs() + UART_TX_TIMEOUT_US;
            bool result = transmit(target, buffer_);
            // if local send not successful, no need to wait for the ack
            if (!result) {
                ackCb_ = nullptr;
            }
            else if (ackCb_ && !msg::requiresAck(MSG::ID)) {
                ackCb_(true, buffer_); 
                ackCb_ = nullptr;
            }
            return result;
        }

        /** Transmits given message to the provided device. 
         
            Returns true if sending the message was successful, false if there was hardware error with the local transceiver. 
         */
        template<typename MSG>
        bool send(DeviceId target, MSG const & message) { return send(target, message, nullptr); }

        template<typename MSG>
        bool sendBlocking(DeviceId target, MSG const & message) {
            // TODO 
            return false;
        }
        
    protected:

        /** Callback when a message is received. 
         
            Should be overwritten by the subclases to provide adequate feedback.
         */
        virtual void onMessageReceived(Packet const & msg) = 0;

        /** Transmits the given packet. 
         
            This method must be implemented by any spcialization. 
         */
        bool transmit(DeviceId target, Packet const & packet);

        /** Enables the HW. 
         
            Returns true if successful, false otherwise.
         */
        bool enableHardware();

        void disableHardware(); 

        DeviceId ownId_;
        bool enabled_ = false;
        Packet buffer_; 
        AckCallback ackCb_ = nullptr;
        uint32_t txTimeout_ = 0;

    private:

        void onAckReceived(bool value) {
            if (ackCb_) {
                ackCb_(value, buffer_);
                ackCb_ = nullptr;
            }
        }
    }; 

} // namespace rckid