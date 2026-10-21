#include "Menu.hpp"

#include <cmath>

#include "BufferLogic.hpp"
#include "Settings.hpp"

using namespace geode::prelude;

namespace {
constexpr ccColor3B kCyan = {70, 235, 255};
constexpr ccColor3B kCyanDim = {120, 200, 215};
constexpr ccColor3B kWhite = {235, 252, 255};

CCSprite* makeStepButton(char const* text) {
    auto spr = CCSprite::createWithSpriteFrameName("GJ_button_01.png");
    spr->setColor(kCyan);
    auto label = CCLabelBMFont::create(text, "bigFont.fnt");
    label->setScale(0.9f);
    auto size = spr->getContentSize();
    label->setPosition({size.width / 2, size.height / 2 + 1.f});
    spr->addChild(label);
    return spr;
}
}  // namespace

FluxMenu* FluxMenu::create() {
    auto ret = new FluxMenu();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

FluxMenu::~FluxMenu() {
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

void FluxMenu::toggle() {
    if (s_instance) {
        s_instance->onClose(nullptr);
        return;
    }
    // Pause the level first so nothing happens behind the menu.
    if (auto pl = PlayLayer::get()) {
        if (!pl->m_isPaused) {
            pl->pauseGame(false);
        }
    }
    if (auto menu = FluxMenu::create()) {
        s_instance = menu;
        menu->show();
    }
}

bool FluxMenu::init() {
    // Glass panel: light rounded square, tinted cyan and made translucent.
    if (!Popup::init(300.f, 230.f, "square02b_001.png", CCRect{0, 0, 80, 80})) {
        return false;
    }
    m_noElasticity = true;

    m_bgSprite->setColor(ccc3(20, 170, 205));
    m_bgSprite->setOpacity(150);
    // Lighter dimming than the default so the glass effect reads against the game.
    this->setOpacity(70);

    // Soft inner highlight strip to sell the glass look.
    auto sheen = CCLayerColor::create(ccc4(255, 255, 255, 28), 276.f, 62.f);
    sheen->setPosition({12.f, 230.f - 74.f});
    m_mainLayer->addChild(sheen, 0);

    // --- Branding ---
    auto title = CCLabelBMFont::create("FLUX", "bigFont.fnt");
    title->setScale(1.0f);
    title->setColor(kWhite);
    m_mainLayer->addChildAtPosition(title, Anchor::Top, ccp(0, -30));

    auto tagline = CCLabelBMFont::create("Every frame counts.", "chatFont.fnt");
    tagline->setScale(0.75f);
    tagline->setColor(kCyan);
    m_mainLayer->addChildAtPosition(tagline, Anchor::Top, ccp(0, -56));

    // --- Controls (buttons live in the popup's own button menu) ---
    auto toggleLabel = CCLabelBMFont::create("Input Buffer", "bigFont.fnt");
    toggleLabel->setScale(0.5f);
    toggleLabel->setColor(kWhite);
    toggleLabel->setAnchorPoint({0.f, 0.5f});
    toggleLabel->setPosition({30.f, 230.f - 92.f});
    m_mainLayer->addChild(toggleLabel, 2);

    m_stateLabel = CCLabelBMFont::create("ON", "bigFont.fnt");
    m_stateLabel->setScale(0.45f);
    m_stateLabel->setAnchorPoint({1.f, 0.5f});
    m_stateLabel->setPosition({300.f - 78.f, 230.f - 92.f});
    m_mainLayer->addChild(m_stateLabel, 2);

    m_toggler = CCMenuItemToggler::createWithStandardSprites(
        this, menu_selector(FluxMenu::onToggle), 0.75f
    );
    m_toggler->setPosition({300.f - 48.f, 230.f - 92.f});
    m_buttonMenu->addChild(m_toggler);

    // Frame stepper: [-]  N  [+]
    auto framesLabel = CCLabelBMFont::create("Buffer frames", "bigFont.fnt");
    framesLabel->setScale(0.5f);
    framesLabel->setColor(kWhite);
    framesLabel->setPosition({150.f, 230.f - 128.f});
    m_mainLayer->addChild(framesLabel, 2);

    auto minus = CCMenuItemSpriteExtra::create(
        makeStepButton("-"), this, menu_selector(FluxMenu::onMinus)
    );
    minus->setPosition({90.f, 230.f - 158.f});
    m_buttonMenu->addChild(minus);

    auto plus = CCMenuItemSpriteExtra::create(
        makeStepButton("+"), this, menu_selector(FluxMenu::onPlus)
    );
    plus->setPosition({210.f, 230.f - 158.f});
    m_buttonMenu->addChild(plus);

    m_valueLabel = CCLabelBMFont::create("5", "goldFont.fnt");
    m_valueLabel->setScale(0.95f);
    m_valueLabel->setPosition({150.f, 230.f - 158.f});
    m_mainLayer->addChild(m_valueLabel, 2);

    // Slider 0-20
    m_slider = Slider::create(this, menu_selector(FluxMenu::onSlider), 0.8f);
    m_slider->setPosition({150.f, 230.f - 193.f});
    m_mainLayer->addChild(m_slider, 2);

    auto zero = CCLabelBMFont::create("0", "chatFont.fnt");
    zero->setScale(0.6f);
    zero->setColor(kCyanDim);
    zero->setPosition({52.f, 230.f - 208.f});
    m_mainLayer->addChild(zero, 2);

    auto twenty = CCLabelBMFont::create("20", "chatFont.fnt");
    twenty->setScale(0.6f);
    twenty->setColor(kCyanDim);
    twenty->setPosition({248.f, 230.f - 208.f});
    m_mainLayer->addChild(twenty, 2);

    auto hint = CCLabelBMFont::create("Press Y to close", "chatFont.fnt");
    hint->setScale(0.55f);
    hint->setColor(kCyanDim);
    hint->setOpacity(190);
    m_mainLayer->addChildAtPosition(hint, Anchor::Bottom, ccp(0, 10));

    // Reflect saved state.
    m_toggler->toggle(flux::settings::enabled());
    this->setFrames(flux::settings::frames(), true);
    this->refreshEnabledState();
    return true;
}

void FluxMenu::setFrames(int frames, bool updateSlider) {
    frames = flux::clampFrames(frames);
    flux::settings::setFrames(frames);
    m_valueLabel->setString(std::to_string(frames).c_str());
    if (updateSlider) {
        m_slider->setValue(static_cast<float>(frames) / flux::kMaxFrames);
    }
}

void FluxMenu::refreshEnabledState() {
    bool on = flux::settings::enabled();
    m_stateLabel->setString(on ? "ON" : "OFF");
    m_stateLabel->setColor(on ? kCyan : ccc3(150, 150, 150));
    GLubyte alpha = on ? 255 : 120;
    m_valueLabel->setOpacity(alpha);
}

void FluxMenu::onPlus(CCObject*) {
    this->setFrames(flux::settings::frames() + 1, true);
}

void FluxMenu::onMinus(CCObject*) {
    this->setFrames(flux::settings::frames() - 1, true);
}

void FluxMenu::onSlider(CCObject* sender) {
    auto thumb = static_cast<SliderThumb*>(sender);
    int frames = static_cast<int>(std::lround(thumb->getValue() * flux::kMaxFrames));
    // Do not push the value back into the slider while the user is dragging it.
    this->setFrames(frames, false);
}

void FluxMenu::onToggle(CCObject*) {
    // GD invokes the callback before the toggler flips, so the new state is the inverse.
    bool nowOn = !m_toggler->isToggled();
    flux::settings::setEnabled(nowOn);
    this->refreshEnabledState();
}

void FluxMenu::onClose(CCObject* sender) {
    flux::settings::flush();
    s_instance = nullptr;
    Popup::onClose(sender);
}
