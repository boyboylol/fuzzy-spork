// Standalone unit test for the buffering logic.
//   g++ -std=c++20 -I../src test_buffer.cpp -o test_buffer && ./test_buffer
#include "BufferLogic.hpp"

#include <cstdio>
#include <cstdlib>

using flux::ClickBuffer;
using R = flux::ClickBuffer::ReleaseAction;

static int g_checks = 0;
#define CHECK(cond) do { ++g_checks; if (!(cond)) { \
    std::printf("FAIL line %d: %s\n", __LINE__, #cond); std::exit(1); } } while (0)

static constexpr double F = flux::kFrameSeconds;

int main() {
    // Short tap is extended to the minimum hold.
    {
        ClickBuffer b;
        CHECK(!b.onPress(0, 0.0));
        CHECK(b.onRelease(0, 1 * F, 5, true) == R::Defer);
        CHECK(b.hasPending());
        CHECK(!b.takeDue(0, 4 * F));
        CHECK(b.takeDue(0, 5 * F));
        CHECK(!b.hasPending());
        CHECK(!b.takeDue(0, 6 * F));  // only fires once
    }
    // Long hold: release forwarded immediately.
    {
        ClickBuffer b;
        b.onPress(0, 0.0);
        CHECK(b.onRelease(0, 8 * F, 5, true) == R::Forward);
        CHECK(!b.hasPending());
    }
    // Exactly at the threshold forwards immediately.
    {
        ClickBuffer b;
        b.onPress(0, 0.0);
        CHECK(b.onRelease(0, 5 * F, 5, true) == R::Forward);
    }
    // 0 frames and non-bufferable modes never defer.
    {
        ClickBuffer b;
        b.onPress(0, 0.0);
        CHECK(b.onRelease(0, 1 * F, 0, true) == R::Forward);
        b.onPress(0, 2 * F);
        CHECK(b.onRelease(0, 3 * F, 5, false) == R::Forward);
    }
    // A press during a deferred release flushes the release first (order kept).
    {
        ClickBuffer b;
        b.onPress(0, 0.0);
        CHECK(b.onRelease(0, 1 * F, 5, true) == R::Defer);
        CHECK(b.onPress(0, 2 * F));   // caller must send release, then press
        CHECK(!b.hasPending());
        // The new press starts a fresh hold window.
        CHECK(b.onRelease(0, 3 * F, 5, true) == R::Defer);
        CHECK(b.takeDue(0, 7 * F));
    }
    // Duplicate release while deferred stays deferred and does not re-arm.
    {
        ClickBuffer b;
        b.onPress(0, 0.0);
        b.onRelease(0, 1 * F, 5, true);
        CHECK(b.onRelease(0, 2 * F, 5, true) == R::Defer);
        CHECK(b.takeDue(0, 5 * F));
    }
    // Release with no recorded press is forwarded untouched.
    {
        ClickBuffer b;
        CHECK(b.onRelease(0, 1.0, 5, true) == R::Forward);
    }
    // Players are independent.
    {
        ClickBuffer b;
        b.onPress(0, 0.0);
        b.onPress(1, 0.0);
        CHECK(b.onRelease(0, 1 * F, 5, true) == R::Defer);
        CHECK(b.onRelease(1, 9 * F, 5, true) == R::Forward);
        CHECK(!b.takeDue(1, 9 * F));
        CHECK(b.takeDue(0, 9 * F));
    }
    // Disable flush and reset.
    {
        ClickBuffer b;
        b.onPress(0, 0.0);
        b.onRelease(0, 1 * F, 5, true);
        CHECK(b.takePending(0));
        CHECK(!b.takePending(0));
        b.onPress(1, 0.0);
        b.onRelease(1, 1 * F, 5, true);
        b.reset();
        CHECK(!b.hasPending());
    }
    // Clamp.
    CHECK(flux::clampFrames(-3) == 0);
    CHECK(flux::clampFrames(99) == 20);
    CHECK(flux::clampFrames(5) == 5);

    std::printf("OK: %d checks passed\n", g_checks);
    return 0;
}
