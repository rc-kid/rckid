#include <rckid/rckid.h>
#include <rckid/hal.h>
#include <rckid/app.h>
#include <rckid/task.h>
#include <rckid/ui/header.h>
#include <rckid/apps/dialogs/info_dialog.h>

namespace rckid {

    hal::State lastState_;
    hal::State state_;

    TinyDateTime now_;

    uint64_t nextSecondUptime_ = 0;

    // device

    void initialize() {
        hal::device::initialize();
        now_ = hal::time::now();
        nextSecondUptime_ = hal::time::uptimeUs() + 1000000;
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
        // check if we need to trigger second tick
        if (hal::time::uptimeUs() >= nextSecondUptime_) {
            nextSecondUptime_ += 1000000;
            now_.inc();
            ui::Header::update();
            if (App::current() != nullptr) {
                if (App::current()->capabilities().consumesBudget && pim::updateBudget(-1) == 0) {
                    InfoDialog::error("Out of budget", "No more budget today to play the game.");
                    App::current()->exit();
                }
            } 
        }
        // run tasks
        Task::runAll();
    }

    // io

    bool btnDown(Btn btn) {
        return state_.button(btn);
    }

    bool btnPressed(Btn btn) {
        return state_.button(btn) && !lastState_.button(btn);
    }

    bool btnReleased(Btn btn) {
        return !state_.button(btn) && lastState_.button(btn);
    }

    void btnClear(Btn btn) {
        // simply ensure last state is identical to current state
        lastState_.setButton(btn, btnDown(btn));
    }

    void btnClearAll() {
        lastState_ = state_;
    }

    namespace power {

        uint32_t vcc() {
            return state_.vcc();
        }

        bool charging() {
            return state_.charging();
        }

        bool dcConnected() {
            return (state_.vcc() >= 450);
        }
    }

    // time

    namespace time {

        TinyDateTime now() {
            return now_;
        }
    }

    // debugging

    // hal layer events

    void onWakeup(uint32_t payload) {

    }

    void onPowerOff() {

    }

    void onFatalError(char const * file, uint32_t line, char const * msg, uint32_t payload) {
        LOG(LL_ERROR, "Fatal error at " << file << ":" << line << "\n" << msg << " (payload " << payload << ")");
        // TODO do the BSOD

        // infinite loop so that we never return
        while (true)
            yield();
    }
}
