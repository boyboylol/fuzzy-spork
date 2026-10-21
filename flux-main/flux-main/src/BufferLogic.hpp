#pragma once

// Pure C++ scheduling logic for Flux. It deliberately has NO Geode / cocos2d
// dependency so it can be unit-tested on any machine (see tests/test_buffer.cpp).
//
// Model: "click extension" buffer.
//   * Presses are always forwarded to the game immediately (zero added latency).
//   * A release that arrives sooner than `frames` display-frames (1/60 s each)
//     after its press is deferred until the press has lasted that long.
//   * A release arriving after the minimum hold time is forwarded immediately.
//   * A new press while a release is still deferred first flushes that release,
//     so event order is always preserved (release, then press).

#include <algorithm>
#include <array>

namespace flux {

inline constexpr int kMinFrames = 0;
inline constexpr int kMaxFrames = 20;
inline constexpr int kDefaultFrames = 5;
inline constexpr double kFrameSeconds = 1.0 / 60.0;

inline int clampFrames(int frames) {
    return std::clamp(frames, kMinFrames, kMaxFrames);
}

struct Channel {
    bool held = false;            // physical button state as seen by the mod
    bool pendingRelease = false;  // a release is being held back
    double pressTime = 0.0;
    double releaseDue = 0.0;
};

class ClickBuffer {
public:
    enum class ReleaseAction { Forward, Defer };

    // Channel 0 = player 1, channel 1 = player 2.

    // Returns true if a deferred release must be sent BEFORE forwarding this press.
    bool onPress(int channel, double now) {
        auto& c = m_channels[channel];
        bool flushRelease = c.pendingRelease;
        c.pendingRelease = false;
        c.held = true;
        c.pressTime = now;
        return flushRelease;
    }

    ReleaseAction onRelease(int channel, double now, int frames, bool bufferable) {
        auto& c = m_channels[channel];
        if (c.pendingRelease) {
            return ReleaseAction::Defer;  // duplicate release, already held back
        }
        bool wasHeld = c.held;
        c.held = false;
        if (!wasHeld || !bufferable || frames <= 0) {
            return ReleaseAction::Forward;
        }
        double due = c.pressTime + clampFrames(frames) * kFrameSeconds;
        if (now >= due) {
            return ReleaseAction::Forward;
        }
        c.pendingRelease = true;
        c.releaseDue = due;
        return ReleaseAction::Defer;
    }

    // True (once) when a deferred release has become due and must be sent now.
    bool takeDue(int channel, double now) {
        auto& c = m_channels[channel];
        if (c.pendingRelease && now >= c.releaseDue) {
            c.pendingRelease = false;
            return true;
        }
        return false;
    }

    // True (once) if a release is pending; used to flush when the mod is disabled.
    bool takePending(int channel) {
        auto& c = m_channels[channel];
        bool was = c.pendingRelease;
        c.pendingRelease = false;
        return was;
    }

    bool hasPending() const {
        return m_channels[0].pendingRelease || m_channels[1].pendingRelease;
    }

    void reset() { m_channels = {}; }

private:
    std::array<Channel, 2> m_channels{};
};

}  // namespace flux
