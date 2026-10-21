#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/utils/Keyboard.hpp>

#include "BufferLogic.hpp"
#include "Menu.hpp"
#include "Settings.hpp"

using namespace geode::prelude;

namespace {

// GD button id 1 is the jump / click button (2 and 3 are platformer left/right).
constexpr int kJumpButton = 1;

// Only one gameplay layer is active at a time, so one shared state is enough.
// `owner` is only compared, never dereferenced; a new layer resets the state.
struct BufferState {
    flux::ClickBuffer buffer;
    double clock = 0.0;  // accumulated gameplay seconds (frozen while paused)
    GJBaseGameLayer const* owner = nullptr;
};

BufferState& state() {
    static BufferState s;
    return s;
}

void adopt(GJBaseGameLayer const* layer) {
    auto& s = state();
    if (s.owner != layer) {
        s.buffer.reset();
        s.clock = 0.0;
        s.owner = layer;
    }
}

// Modes where hold length IS the input (ship, wave, swing, robot) are never buffered:
// extending a hold there would change movement, not just forgive timing.
bool playerBufferable(PlayerObject* player) {
    if (!player) return true;
    return !(player->m_isShip || player->m_isDart || player->m_isSwing || player->m_isRobot);
}

bool modeBufferable(GJBaseGameLayer* layer) {
    if (!playerBufferable(layer->m_player1)) return false;
    if (layer->m_gameState.m_isDualMode && !playerBufferable(layer->m_player2)) return false;
    return true;
}

}  // namespace

class $modify(FluxGameLayer, GJBaseGameLayer) {
    // Called for every raw button press/release before GD queues it.
    void handleButton(bool down, int button, bool isPlayer1) {
        if (button != kJumpButton) {
            GJBaseGameLayer::handleButton(down, button, isPlayer1);
            return;
        }

        adopt(this);
        auto& s = state();
        int channel = isPlayer1 ? 0 : 1;

        if (!flux::settings::enabled()) {
            // Buffer switched off mid-level: don't leave a held-back release behind.
            if (s.buffer.takePending(channel)) {
                GJBaseGameLayer::handleButton(false, kJumpButton, isPlayer1);
            }
            GJBaseGameLayer::handleButton(down, button, isPlayer1);
            return;
        }

        if (down) {
            // Presses always go through immediately; if a release was being held
            // back, send it first so the game sees release -> press in order.
            if (s.buffer.onPress(channel, s.clock)) {
                GJBaseGameLayer::handleButton(false, kJumpButton, isPlayer1);
            }
            GJBaseGameLayer::handleButton(true, kJumpButton, isPlayer1);
            return;
        }

        auto action = s.buffer.onRelease(
            channel, s.clock, flux::settings::frames(), modeBufferable(this)
        );
        if (action == flux::ClickBuffer::ReleaseAction::Forward) {
            GJBaseGameLayer::handleButton(false, kJumpButton, isPlayer1);
        }
        // Defer: swallowed here, delivered from update() once the minimum hold elapsed.
    }

    void update(float dt) {
        adopt(this);
        auto& s = state();
        s.clock += dt;

        if (s.buffer.hasPending()) {
            bool enabled = flux::settings::enabled();
            for (int channel = 0; channel < 2; ++channel) {
                bool send = enabled ? s.buffer.takeDue(channel, s.clock)
                                    : s.buffer.takePending(channel);
                if (send) {
                    GJBaseGameLayer::handleButton(false, kJumpButton, channel == 0);
                }
            }
        }

        GJBaseGameLayer::update(dt);
    }
};

class $modify(FluxPlayLayer, PlayLayer) {
    void resetLevel() {
        PlayLayer::resetLevel();
        // The game just reset its own input state; drop any held-back release.
        state().buffer.reset();
    }
};

// Y toggles the menu. The listener never consumes the key (returns Propagate),
// so normal game input is unaffected whether the menu is open or closed.
$execute {
    KeyboardInputEvent(KEY_Y).listen([](KeyboardInputData& data) {
        if (data.action != KeyboardInputData::Action::Press) return ListenerResult::Propagate;
        if (data.modifiers) return ListenerResult::Propagate;  // ignore Ctrl/Alt/Shift+Y
        // Don't open while the player is typing in a text field.
        if (auto ime = CCIMEDispatcher::sharedDispatcher()) {
            if (ime->hasDelegate()) return ListenerResult::Propagate;
        }
        FluxMenu::toggle();
        return ListenerResult::Propagate;
    }).leak();
}
