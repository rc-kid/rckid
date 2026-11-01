#pragma once

#include <rckid/ui/app.h>
#include <rckid/ui/tile_grid.h>
#include <rckid/dolang/num.h>

namespace rckid {

    /** Benchmark of the doLang rational numbers (dolang::num) against int32_t and float.

        Runs the same templated workloads for all three types and displays the time each took in microseconds. The workloads are written for rationals, so for int32_t the fractions truncate (e.g. 1/60 becomes 0) and the computed values are meaningless. Only the timings are of interest, as they show the cost of the same sequence of operations in each type.

        The workloads are:

        - whole: arithmetic on whole numbers only, which exercises the num's integer fast path
        - add/sub: adding and subtracting small fractions (denominators up to 7), exact for num
        - mul/div: multiplication and division of fractions, exact for num
        - compare: comparisons of fractions
        - physics: bouncing ball with dt = 1/60, denominators explode, so num constantly has to approximate
        - harmonic: sum of 1/i, worst case for num as denominators grow very fast

        The table is filled one cell per frame so that progress is visible. Press A to run again, B to exit.
     */
    class DoLangNumBenchmark : public ui::App<void> {
    public:
        String name() const override { return "NumBench"; }

        DoLangNumBenchmark() {
            using namespace ui;
            text_ = addChild(new ui::TileGrid{40, 15, Palette256})
                << SetRect(Rect::WH(320, 240));
            tg_ = & text_->contents();
            tg_->text(0, 0) << "doLang num benchmark [us]";
            tg_->text(0, 2) << "workload";
            tg_->text(COL_WIDTH * 1 + NAME_WIDTH - COL_WIDTH, 2) << "";
            for (uint32_t t = 0; t < NUM_TYPES; ++t)
                tg_->text(NAME_WIDTH + t * COL_WIDTH, 2) << TYPE_NAMES[t];
            for (uint32_t w = 0; w < NUM_WORKLOADS; ++w)
                tg_->text(0, FIRST_ROW + w) << WORKLOAD_NAMES[w];
            tg_->text(0, 13) << "A: run again   B: exit";
        }

    protected:

        void loop() override {
            ui::App<void>::loop();
            if (btnPressed(Btn::B))
                return exit();
            if (btnPressed(Btn::A))
                next_ = 0;
            if (next_ < NUM_WORKLOADS * NUM_TYPES) {
                uint32_t w = next_ / NUM_TYPES;
                uint32_t t = next_ % NUM_TYPES;
                if (t == 0)
                    for (uint32_t i = 0; i < NUM_TYPES; ++i)
                        tg_->text(NAME_WIDTH + i * COL_WIDTH, FIRST_ROW + w) << "...      ";
                uint32_t us = 0;
                switch (t) {
                    case 0: us = runWorkload<int32_t>(w); break;
                    case 1: us = runWorkload<float>(w); break;
                    case 2: us = runWorkload<dolang::num>(w); break;
                }
                tg_->text(NAME_WIDTH + t * COL_WIDTH, FIRST_ROW + w) << us << "        ";
                ++next_;
            }
        }

    private:

        static constexpr uint32_t NUM_TYPES = 3;
        static constexpr uint32_t NUM_WORKLOADS = 6;
        static constexpr uint32_t NAME_WIDTH = 10;
        static constexpr uint32_t COL_WIDTH = 10;
        static constexpr uint32_t FIRST_ROW = 4;

        inline static constexpr char const * TYPE_NAMES[NUM_TYPES] = { "int32", "float", "num" };
        inline static constexpr char const * WORKLOAD_NAMES[NUM_WORKLOADS] = { "whole", "add/sub", "mul/div", "compare", "physics", "harmonic" };

        /** Number of input values per workload (kept small so that the num arrays do not take too much RAM).
         */
        static constexpr uint32_t K = 64;

        /** How many times the inner loop over the inputs is repeated.
         */
        static constexpr uint32_t REPS = 100;

        // type helpers, so that the workloads can be written once for all types

        template<typename T>
        static T make(int32_t n, int32_t d = 1);

        template<typename T>
        static int32_t checksum(T x);

        template<typename T>
        uint32_t runWorkload(uint32_t w) {
            // the inputs are filled at runtime from a volatile seed, so the compiler cannot precompute the workloads
            int32_t seed = seed_;
            T a[K];
            T b[K];
            switch (w) {
                case 0: // whole
                    for (uint32_t i = 0; i < K; ++i) {
                        a[i] = make<T>((i + seed) % 17 - 8);
                        b[i] = make<T>((i * 7 + seed) % 13 - 6);
                    }
                    break;
                case 1: // add/sub
                case 3: // compare
                    for (uint32_t i = 0; i < K; ++i) {
                        a[i] = make<T>((i + seed) % 13 - 6, 1 + i % 7);
                        b[i] = make<T>((i * 5 + seed) % 11 - 5, 1 + (i + 3) % 7);
                    }
                    break;
                case 2: // mul/div (all values >= 1 so that integer division never divides by zero)
                    for (uint32_t i = 0; i < K; ++i) {
                        a[i] = make<T>(7 + (i + seed) % 5, 1 + i % 4);
                        b[i] = make<T>(7 + (i * 3 + seed) % 5, 1 + (i + 1) % 4);
                    }
                    break;
                default:
                    break;
            }
            uint64_t start = time::uptimeUs();
            int32_t result = 0;
            switch (w) {
                case 0: result = whole(a, b); break;
                case 1: result = addSub(a, b); break;
                case 2: result = mulDiv(a, b); break;
                case 3: result = compare(a, b); break;
                case 4: result = physics<T>(); break;
                case 5: result = harmonic<T>(); break;
            }
            uint32_t elapsed = static_cast<uint32_t>(time::uptimeUs() - start);
            sink_ = sink_ + result;
            return elapsed;
        }

        /** Sum of products of whole numbers, the sum is reset every rep to stay in range.
         */
        template<typename T>
        static int32_t whole(T const * a, T const * b) {
            int32_t check = 0;
            for (uint32_t r = 0; r < REPS; ++r) {
                T sum = make<T>(0);
                for (uint32_t i = 0; i < K; ++i)
                    sum = sum + a[i] * b[i] - a[(i + 1) % K];
                check += checksum(sum);
            }
            return check;
        }

        /** Adds all a's and subtracts all b's, the sum is reset every rep.
         */
        template<typename T>
        static int32_t addSub(T const * a, T const * b) {
            int32_t check = 0;
            for (uint32_t r = 0; r < REPS; ++r) {
                T sum = make<T>(0);
                for (uint32_t i = 0; i < K; ++i)
                    sum = sum + a[i] - b[i];
                check += checksum(sum);
            }
            return check;
        }

        /** x = x * a / b, where the values are reset every few steps so that the magnitude stays bounded.
         */
        template<typename T>
        static int32_t mulDiv(T const * a, T const * b) {
            int32_t check = 0;
            for (uint32_t r = 0; r < REPS; ++r) {
                for (uint32_t i = 0; i < K; i += 4) {
                    T x = make<T>(1);
                    for (uint32_t j = i; j < i + 4; ++j)
                        x = x * a[j] / b[j];
                    check += checksum(x);
                }
            }
            return check;
        }

        template<typename T>
        static int32_t compare(T const * a, T const * b) {
            int32_t count = 0;
            for (uint32_t r = 0; r < REPS; ++r) {
                for (uint32_t i = 0; i < K; ++i) {
                    if (a[i] < b[i])
                        ++count;
                    if (a[i] >= b[(i + 1) % K])
                        ++count;
                }
            }
            return count;
        }

        /** Ball falling from height 100 under gravity and bouncing off the floor, losing 1/10 of its speed at each bounce.
         */
        template<typename T>
        static int32_t physics() {
            T const dt = make<T>(1, 60);
            T const g = make<T>(98, 10);
            T const restitution = make<T>(9, 10);
            T const zero = make<T>(0);
            T x = make<T>(100);
            T v = make<T>(0);
            for (uint32_t i = 0; i < REPS * K / 4; ++i) {
                v = v - g * dt;
                x = x + v * dt;
                if (x < zero) {
                    x = -x;
                    v = -v * restitution;
                }
            }
            return checksum(x) + checksum(v);
        }

        /** Sums 1/i for i = 1..K*4, repeated.
         */
        template<typename T>
        static int32_t harmonic() {
            int32_t check = 0;
            for (uint32_t r = 0; r < REPS / 4; ++r) {
                T sum = make<T>(0);
                for (uint32_t i = 1; i <= K * 4; ++i)
                    sum = sum + make<T>(1, i);
                check += checksum(sum);
            }
            return check;
        }

        ui::TileGrid * text_;
        TileGrid * tg_;

        uint32_t next_ = 0;

        volatile int32_t seed_ = 3;
        volatile int32_t sink_ = 0;

    }; // rckid::DoLangNumBenchmark

    template<> inline int32_t DoLangNumBenchmark::make<int32_t>(int32_t n, int32_t d) { return n / d; }
    template<> inline float DoLangNumBenchmark::make<float>(int32_t n, int32_t d) { return static_cast<float>(n) / static_cast<float>(d); }
    template<> inline dolang::num DoLangNumBenchmark::make<dolang::num>(int32_t n, int32_t d) { return dolang::num{n, d}; }

    template<> inline int32_t DoLangNumBenchmark::checksum<int32_t>(int32_t x) { return x; }
    template<> inline int32_t DoLangNumBenchmark::checksum<float>(float x) { return static_cast<int32_t>(x * 1000); }
    template<> inline int32_t DoLangNumBenchmark::checksum<dolang::num>(dolang::num x) { return x.n() ^ x.d(); }

} // namespace rckid
