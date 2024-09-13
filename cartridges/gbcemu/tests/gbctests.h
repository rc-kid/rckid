#pragma once

#include <platform/tests.h>

#include "../lib/assembler.h"
#include "../lib/gbc.h"

#define Z (1 << 7)
#define N (1 << 6)
#define H (1 << 5)
#define C (1 << 4)

#define RUN(...) do { uint8_t pgm[] = { __VA_ARGS__ STOP(0) }; gbc.runTest(pgm, sizeof(pgm)); } while (false)
#define EXPECT_FLAGS(...) EXPECT((int)gbc.state().f(), (static_cast<int>(__VA_ARGS__)))
