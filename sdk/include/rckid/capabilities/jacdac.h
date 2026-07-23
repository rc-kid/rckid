#pragma once

#include <rckid/rckid.h>
#include <rckid/task.h>
#include <rckid/string.h>
#include <rckid/ui/header.h>

namespace rckid {

    /** JACDAC communication capability.
     
        Very primitive implementation of JACDAC SWS protocol to send and receive packets. On the network layer all JACDAC has to do is send and receive packets. We might get fancier later on, but for now packet transmit is blocking and packet receive is handled by the driver where the packet is stored into an internal queue. The application is expected to check the queue periodically and process the messages accordingly.

        TODO or just do a rx callback for now and be done with it? simpler for demoing 

        NOTE this is work in progress and early demnonstration at best.
     */
    class JACDAC : public Task {
    public:

        static JACDAC * instance();

        std::optional<std::pair<TileIcon, uint8_t>> headerIcon() const override {
            // TODO
            return std::nullopt;
        }

        /** Sends JACDAC packet.
         */
        void sendPacket(uint8_t const * data, uint32_t numBytes);

    protected:
        void onTick() override;

        void releaseResources() override {
            delete this;
        }

    }; // rckid::JACDAC

} // namespace rckid