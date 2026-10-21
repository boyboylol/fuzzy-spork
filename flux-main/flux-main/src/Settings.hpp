#pragma once

#include <Geode/Geode.hpp>
#include "BufferLogic.hpp"

// Persistent Flux settings, stored with Geode's saved-value API
// (written to Flux's save file in the Geode config directory, so they survive restarts).
namespace flux::settings {

inline bool enabled() {
    return geode::Mod::get()->getSavedValue<bool>("enabled", true);
}

inline int frames() {
    return clampFrames(geode::Mod::get()->getSavedValue<int>("buffer-frames", kDefaultFrames));
}

inline void setEnabled(bool value) {
    geode::Mod::get()->setSavedValue<bool>("enabled", value);
}

inline void setFrames(int value) {
    geode::Mod::get()->setSavedValue<int>("buffer-frames", clampFrames(value));
}

// Flush to disk right away so a crash does not lose the change.
inline void flush() {
    auto result = geode::Mod::get()->saveData();
    (void)result;
}

}  // namespace flux::settings
