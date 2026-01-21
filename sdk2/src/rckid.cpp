#include <rckid/rckid.h>
#include <rckid/hal.h>
#include <rckid/task.h>

namespace rckid {

    hal::State lastState_;
    hal::State state_;

    // device

    void initialize() {
        hal::device::initialize();
        // TODO
    }

    void tick() {
        // move to next state
        lastState_ = state_;
        state_.updateWith(hal::io::state());
        // check state interrupts
        // TODO
        // run hal's on tick & yield (this likely requests new state to be gathered as well)
        hal::device::onTick();
        hal::device::onYield();
        // run tasks
        Task::runAll();
    }

    // io

    bool btnDown(Btn btn) {
        return (state_.buttons() & static_cast<uint16_t>(btn)) != 0;
    }

    bool btnPressed(Btn btn) {
        return ((state_.buttons() & static_cast<uint16_t>(btn)) != 0) &&
               ((lastState_.buttons() & static_cast<uint16_t>(btn)) == 0);
    }

    bool btnReleased(Btn btn) {
        return ((state_.buttons() & static_cast<uint16_t>(btn)) == 0) &&
               ((lastState_.buttons() & static_cast<uint16_t>(btn)) != 0);
    }

    void btnClear(Btn btn) {
        // simply ensure last state is identical to current state
        lastState_.setButtonState(static_cast<uint16_t>(btn), btnDown(btn));
    }


    // time

    // debugging

    // hal layer events

    void onWakeup(uint32_t payload) {

    }

    void onPowerOff() {

    }

    void onSecondTick() {

    }

    void onFatalError(char const * file, uint32_t line, char const * msg, uint32_t payload) {
        LOG(LL_ERROR, "Fatal error at " << file << ":" << line << "\n" << msg << " (payload " << payload << ")");
        // TODO do the BSOD

        // infinite loop so that we never return
        while (true)
            yield();
    }
}
