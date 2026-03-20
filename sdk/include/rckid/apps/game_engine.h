#pragma once

#include <rckid/game/engine.h>

namespace rckid {

    class GameEngine : public game::Engine {
    public:
        GameEngine():
            game::Engine("Cat") {

        }

    }; // rckid::GameEngine
}