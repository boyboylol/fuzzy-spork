#pragma once

#include <Geode/Geode.hpp>

// The Flux settings menu: a semi-transparent cyan "glass" popup.
class FluxMenu : public geode::Popup {
public:
    static FluxMenu* create();
    ~FluxMenu() override;

    // Opens the menu if it is closed, closes it if it is open (bound to the Y key).
    static void toggle();

protected:
    bool init();
    void onClose(cocos2d::CCObject*) override;

    void onPlus(cocos2d::CCObject*);
    void onMinus(cocos2d::CCObject*);
    void onSlider(cocos2d::CCObject*);
    void onToggle(cocos2d::CCObject*);

    void setFrames(int frames, bool updateSlider);
    void refreshEnabledState();

    cocos2d::CCLabelBMFont* m_valueLabel = nullptr;
    cocos2d::CCLabelBMFont* m_stateLabel = nullptr;
    Slider* m_slider = nullptr;
    CCMenuItemToggler* m_toggler = nullptr;

    inline static FluxMenu* s_instance = nullptr;
};
