#pragma once

#include <rckid/graphics/canvas.h>

namespace rckid {

    /** Pong clone.
     
        Consists of two players and a ball. The left player is always the current player, while the right one can either be someone else in multiplayer mode, or the machine itself in single player mode. 

        The paddle is controlled by tilting the device. As the fantasy console has no accelerometer, the dpad can be used to nudge the paddle up and down as well.

        - controllable game speed
        - and size of the paddles
        - allow customizing the paddle color, to make kids happy 
        - allow changing the ball angle based on the angle and speed of the paddle
     */
    class Pong : public CanvasApp<void> {
    public:

        String name() const override { return "Pong"; }

        Capabilities capabilities() const override {
            return Capabilities {
                .canPersistState = true,
                .consumesBudget = true,
            };
        }

        static constexpr uint8_t VERSION = 1;

        Pong(): CanvasApp<void>{} {
            resetGame();
        }

    protected:

        void saveState(RandomWriteStream & into) const override {
            into.binaryWriter()
                << VERSION
                << scoreLeft_
                << scoreRight_
                << ball_.x
                << ball_.y
                << deltaBall_.x
                << deltaBall_.y
                << speed_
                << fail_;
        }

        bool loadState(RandomReadStream & from) override {
            auto rd = from.binaryReader();
            if (read<uint8_t>(rd) != VERSION) {
                LOG(LL_WARN, "Unsupported save version, skipping");
                return false;
            }
            rd  >> scoreLeft_
                >> scoreRight_
                >> ball_.x
                >> ball_.y
                >> deltaBall_.x
                >> deltaBall_.y
                >> speed_
                >> fail_;
            return true;
        }

        void loop() override {
            CanvasApp<void>::loop();
            if (btnPressed(Btn::B)) {
                btnClear(Btn::B);
                return exit();
            }
            // move the ball
            ball_ = ball_ + (deltaBall_ * speed_);
            // update the pad's position & movement speed
            // delta controlled by the keys & press length
            /*
            if (down(Btn::Up)) {
                deltaLeft_ *= FixedInt{1, 32};
                deltaLeft_.clamp(-15, -1);
                left_ += deltaLeft_;
            } else if (deltaLeft_ < 0) {
                deltaLeft_ = 0;
            }
            if (down(Btn::Down)) {
                deltaLeft_ *= FixedInt{1, 32};
                deltaLeft_.clamp(1, 15);
                left_ += deltaLeft_;
            } else if (deltaLeft_ > 0) {
                deltaLeft_ = 0;
            }
            */
            // -- delta controlled by the acceperometer
            /*  
            deltaLeft_ = accelY() * 32 / 16384;
            if (deltaLeft_ > 32)
                deltaLeft_ = 32;
            if (deltaLeft_ < -32)
                deltaLeft_ = -32;
            left_ += deltaLeft_;
            */
            // position controlled by the accelerometer, nudged by the dpad
            if (btnDown(Btn::Up))
                dpadOffset_ = (dpadOffset_ - 4).clamp(-DPAD_RANGE, DPAD_RANGE);
            if (btnDown(Btn::Down))
                dpadOffset_ = (dpadOffset_ + 4).clamp(-DPAD_RANGE, DPAD_RANGE);
            left_ = 120 + accel().y * 200 / 16384 + dpadOffset_;

            left_ = left_.clamp(20, 220);
            // update the right paddle to always center the ball
            // TODO do this only when single player mode (!)
            right_ = ball_.y;
            right_ = right_.clamp(20, 220);
            // check ball position
            if (ball_.y < 5) {
                ball_.y = 5 + (5 - ball_.y);
                deltaBall_.y = deltaBall_.y * -1; 
            }
            if (ball_.y >= 235) {
                ball_.y = 470 - ball_.y;
                deltaBall_.y = deltaBall_.y * -1; 
            }
            // check the collision with paddles
            if (!fail_) {
                if (ball_.x >= 300) {
                    ball_.x = 600 - ball_.x;
                    deltaBall_.x = deltaBall_.x * -1;
                    rumbler::nudge();
                }
                if (ball_.x <= 20) {
                    if (ball_.y.inRange(left_ - 20, left_ + 20)) {
                        ball_.x = 40 - ball_.x;
                        deltaBall_.x = deltaBall_.x * -1;
                        rumbler::nudge();
                    } else {
                        fail_ = true;
                    }
                }
            }

            // oops, player 1 looses
            if (ball_.x <= 0) {
                if (++scoreRight_ == 10)
                    resetGame();
                else 
                    resetBall();
            }
            // oops player 2 looses
            if (ball_.x >= 320) {
                if (++scoreLeft_ == 10)
                    resetGame();
                else 
                    resetBall();
            }
        }

        void render() override {
            // clear the screen and draw the net
            canvas().fill(Color::Black());
            for (Coord i = 10; i < 240; i += 20)
                canvas().fill(Rect::XYWH(159, i, 2, 10), Color::DarkGray());
            // draw the player score
            drawScore(scoreLeft_, 120);
            drawScore(scoreRight_, 200 - 20);
            // draw player paddles
            canvas().fill(Rect::XYWH(5, (left_ - 20).round(), 10, 40), Color::White());
            canvas().fill(Rect::XYWH(305, (right_ - 20).round(), 10, 40), Color::White());
            // draw the ball
            FixedPoint ballPos = ball_ - Point{5, 5};
            canvas().fill(Rect::XYWH(ballPos.x.round(), ballPos.y.round(), 10, 10), Color::White());
            CanvasApp<void>::render();
        }

        void resetBall() {
            // TODO effects
            ball_ = FixedPoint{160, 120};
            speed_ = 1;
            deltaBall_ = FixedPoint{FixedInt{1, 8}, 1};
            deltaLeft_ = 0;
            dpadOffset_ = 0;
            fail_ = false;
            rumbler::success();
        }

        void resetGame() {
            // TODO effects
            resetBall();
            scoreLeft_ = 0;
            scoreRight_ = 0;
        }

        void drawScore(uint32_t score, Coord fromX) {
            uint8_t const * digit = digits_[score];
            for (Coord x = 0; x < 4; ++x) {
                uint8_t col = *(digit++);
                for (Coord y = 0; y < 8; ++y) {
                    if (col & (1 << (7 - y)))
                        canvas().fill(Rect::XYWH(fromX + (3 - x) * 5, 10 + y * 5, 5, 5), Color::Gray());
                }
            }
        }

    private:

        /** How far from the center the dpad alone can move the paddle. 
         */
        static constexpr int DPAD_RANGE = 100;

        FixedInt left_ = 120;
        FixedInt deltaLeft_ = 0;
        FixedInt dpadOffset_ = 0;

        FixedInt right_ = 120;
        FixedInt deltaRight_ = 0;

        FixedPoint ball_;
        FixedPoint deltaBall_;   

        FixedInt speed_ = 1;     

        uint32_t scoreLeft_ = 0;
        uint32_t scoreRight_ = 0;

        bool fail_ = false;

        // simple digits we have 
        static constexpr uint8_t digits_[][4] = {
            {
                0b11111110,
                0b10000010,
                0b10000010,
                0b11111110,
            },
            {
                0b00000000,
                0b00000000,
                0b11111110,
                0b00000000,
            },
            {
                0b11110010,
                0b10010010,
                0b10010010,
                0b10011110,
            },
            {
                0b11111110,
                0b10010010,
                0b10010010,
                0b10000010,
            },
            {
                0b11111110,
                0b00010000,
                0b00010000,
                0b11110000,
            },
            {
                0b10011110,
                0b10010010,
                0b10010010,
                0b11110010,
            },
            {
                0b00011110,
                0b00010010,
                0b00010010,
                0b11111110,
            },
            {
                0b11111110,
                0b10000000,
                0b10000000,
                0b10000000,
            },
            {
                0b11111110,
                0b10010010,
                0b10010010,
                0b11111110,
            },
            {
                0b11111110,
                0b10010000,
                0b10010000,
                0b11110000,
            },
        };

    }; // rckid::Pong

} // namespace rckid
