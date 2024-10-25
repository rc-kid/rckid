#include <rckid/app.h>
#include <rckid/graphics/bitmap.h>
#include <rckid/graphics/color.h>
#include <rckid/utils/fixedint.h>

namespace rckid {

    /** Atari Pong Clone
     
        Consists of two players and a ball. The left player is always the current player, while the right one can either be someone else in multiplayer mode, or the machine itself in single player mode. 

        - controllable game speed
        - and size of the paddles
        - allow customizing the paddle color, to make kids happy 
        - allow changing the ball angle based on the angle and speed of the paddle


     */
    class Pong : public GraphicsApp<RenderableBitmap<ColorRGB>> {
    public:
        static void run() {
            Pong p{};
            p.loop();
        }
        
    protected:
        Pong(): 
            GraphicsApp{RenderableBitmap<Color>{320, 240}}
        {
            resetGame();
        }

        void update() override {
            if (btnPressed(Btn::B))
                exit();
            // move the ball
            ball_ = ball_ + (deltaBall_ * speed_);
            // update the pad's position & movement speed
            // delta controlled by the keys & press length
            /*
            if (down(Btn::Up)) {
                deltaLeft_ *= FixedInt{1, 32};
                deltaLeft_.clipInRange(-15, -1);
                left_ += deltaLeft_;
            } else if (deltaLeft_ < 0) {
                deltaLeft_ = 0;
            }
            if (down(Btn::Down)) {
                deltaLeft_ *= FixedInt{1, 32};
                deltaLeft_.clipInRange(1, 15);
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
           // position controlled by the accelerometer
            left_ = 120 + accelY() * 200 / 16384;

            left_.clipInRange(20, 220);
            // update the rigfht paddle to always center the ball
            // TODO do this only when single player mode (!)
            right_ = ball_.y;
            right_.clipInRange(20, 220);
            // check ball position
            if (ball_.y < 5) {
                ball_.y = 5 + (5 - ball_.y);
                deltaBall_.y = deltaBall_.y * -1; 
            }
            if (ball_.y >= 235) {
                ball_.y =470 - ball_.y;
                deltaBall_.y = deltaBall_.y * -1; 
            }
            // check the collision with paddles
            if (!fail_) {
                if (ball_.x >= 300) {
                    ball_.x = 600 - ball_.x;
                    deltaBall_.x = deltaBall_.x * -1;
                    
                    //setRumbler(RumblerEffect::Nudge());
                    // TODO pong
                }
                if (ball_.x <= 20) {
                    if (ball_.y.inRange(left_ - 20, left_ + 20)) {
                        ball_.x = 40 - ball_.x;
                        deltaBall_.x = deltaBall_.x * -1;
                        //setRumbler(RumblerEffect::Nudge());
                        // TODO pong
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

        void draw() {
            // clear the scrceen and draw the net
            g_.fill(color::Black);
            for (int i = 10; i < 240; i += 20)
                g_.fill(color::DarkGray, Rect::XYWH(159, i, 2, 10));
            // draw the player score
            drawScore(scoreLeft_, 120);
            drawScore(scoreRight_, 200 - 20);
            // draw player paddles
            g_.fill(color::White, Rect::XYWH(5, left_ - 20, 10, 40));
            g_.fill(color::White, Rect::XYWH(305, right_ - 20, 10, 40));
            // draw the ball
            FixedPoint ballPos = ball_ - Point{5,5};
            g_.fill(color::White, Rect::XYWH(ballPos.x, ballPos.y, 10, 10));
        }

        void resetBall() {
            // TODO effects
            ball_ = FixedPoint{160, 120};
            speed_ = 1;
            deltaBall_ = FixedPoint{FixedInt{1, 128},1};
            deltaLeft_ = 0;
            fail_ = false;
            //setRumbler(RumblerEffect::OK());

        }

        void resetGame() {
            // TODO effects
            resetBall();
            scoreLeft_ = 0;
            scoreRight_ = 0;

        }

        void drawScore(unsigned score, int fromX) {
            uint8_t const * digit = digits_[score];
            for (int x = 0; x < 4; ++x) {
                uint8_t col = *(digit++);
                for (int y = 0; y < 8; ++y) {
                    if (col & (1 << (7 - y)))
                        g_.fill(color::Gray, Rect::XYWH(fromX + (3 - x) * 5, 10 + y * 5, 5, 5));
                }
            }
        }

    private:

        FixedInt left_ = 120_fi;
        FixedInt deltaLeft_ = 0_fi;

        FixedInt right_ = 120_fi;
        FixedInt deltaRight_ = 0_fi;

        FixedPoint ball_;
        FixedPoint deltaBall_;   

        FixedInt speed_ = 1_fi;     

        unsigned scoreLeft_ = 0;
        unsigned scoreRight_ = 0;

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