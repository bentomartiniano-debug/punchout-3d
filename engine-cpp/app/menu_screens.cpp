#include "menu_screens.hpp"
#include "character_info.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <vector>

// Lógica e layout dos menus; o desenho fica em menu_draw.cpp.
namespace KoRama {

namespace {

using Key = sf::Keyboard::Key;

// Ordem da grade de seleção do index.html; -1 é o card "?" (aleatório).
constexpr std::array<int, 9> kSlotChars = {0, 1, 4, 5, -1, 2, 3, 6, 7}; // o aleatório fica no centro

constexpr int kModeArcade = 0;
constexpr int kModeSolo = 1;
constexpr int kModeTraining = 2;
constexpr int kModeBuild = 3;
constexpr int kModeBoss = 4;
constexpr int kModeBack = 5;
constexpr int kVersusFrames = 170;      // ~2,8 s
constexpr int kVersusSkipAfter = 30;

bool isConfirm(Key k) { return k == Key::Enter || k == Key::Z || k == Key::J || k == Key::Space; }
bool isBack(Key k) { return k == Key::Escape || k == Key::X || k == Key::K || k == Key::Backspace; }
bool isUp(Key k) { return k == Key::Up || k == Key::W; }
bool isDown(Key k) { return k == Key::Down || k == Key::S; }
bool isLeft(Key k) { return k == Key::Left || k == Key::A; }
bool isRight(Key k) { return k == Key::Right || k == Key::D; }

std::vector<uint32_t> playableChars() {
    std::vector<uint32_t> out;
    for (uint32_t i = 0; i < 8; ++i) {
        if (kRoster[i].hasFightSprites) out.push_back(i);
    }
    return out;
}

} // namespace

MenuScreens::MenuScreens(HudFonts fonts, const SpriteLibrary* sprites) : fonts_(fonts), sprites_(sprites) {
    for (uint32_t c = 0; c < loadouts_.size(); ++c) loadouts_[c] = SkillSystem::defaultLoadout(c);
}

void MenuScreens::setLoadout(uint32_t charId, const SkillLoadout& loadout) {
    if (charId < loadouts_.size()) loadouts_[charId] = loadout;
}

bool MenuScreens::takeDirty() {
    bool d = dirty_;
    dirty_ = false;
    return d;
}

void MenuScreens::goTo(Screen screen) {
    if (screen != screen_) prevScreen_ = screen_;
    screen_ = screen;
    screenFrame_ = 0;
    notice_.clear();
    noticeTimer_ = 0;
    modal_ = Modal::None;
    if (screen == Screen::CharSelect) charButtonIndex_ = -1;
    if (screen == Screen::Versus) versusReady_ = false;
}

void MenuScreens::notify(const std::string& message) {
    notice_ = message;
    noticeTimer_ = 150;
}

int MenuScreens::stageFocus() const {
    if (screen_ == Screen::CharSelect) return slotChar(slot_);
    if (screen_ == Screen::ModeSelect) return static_cast<int>(playerChar_);
    return -1;
}

int MenuScreens::slotChar(int slot) const {
    return kSlotChars[static_cast<std::size_t>(std::clamp(slot, 0, kSlots - 1))];
}

float MenuScreens::logoBlend() const {
    // A logo começa grande no centro; ao sair do título ela sobe para o topo com mola
    // (transition 0.85s cubic-bezier(0.34, 1.45, 0.64, 1)).
    if (screen_ == Screen::Title) {
        if (prevScreen_ == Screen::Title || screenFrame_ >= 40) return 0.0f;
        const float t = std::min(1.0f, screenFrame_ / 40.0f);
        return 1.0f - (1.0f + 2.45f * std::pow(t - 1.0f, 3.0f) + 1.45f * std::pow(t - 1.0f, 2.0f));
    }
    if (prevScreen_ != Screen::Title) return 1.0f;
    const float t = std::min(1.0f, screenFrame_ / 51.0f);
    return 1.0f + 2.45f * std::pow(t - 1.0f, 3.0f) + 1.45f * std::pow(t - 1.0f, 2.0f);
}

// ---------------------------------------------------------------------------
// Layout no espaço 1600x900 (o mesmo retângulo serve para desenho e para o mouse)
// ---------------------------------------------------------------------------

sf::FloatRect MenuScreens::cardRect(int slot) const {
    // Faixa única do elenco no rodapé.
    const float w = 120.0f, h = 104.0f, gap = 22.0f;
    return {{172.0f + slot * (w + gap), 762.0f}, {w, h}};
}

sf::FloatRect MenuScreens::titleButton(int i) const {
    return {{600.0f, 400.0f + i * 92.0f}, {400.0f, 72.0f}};
}

sf::FloatRect MenuScreens::charButton(int i) const {
    static const std::array<sf::FloatRect, 3> kRects = {{
        {{28.0f, 22.0f}, {150.0f, 48.0f}}, {{1036.0f, 584.0f}, {296.0f, 64.0f}}, {{1350.0f, 592.0f}, {180.0f, 50.0f}}}};
    return kRects[static_cast<std::size_t>(std::clamp(i, 0, 2))];
}

sf::FloatRect MenuScreens::modeButton(int i) const {
    // Cards à direita do palco do lutador; o VOLTAR é menor.
    if (i == 5) return {{990.0f, 578.0f}, {480.0f, 60.0f}};
    return {{960.0f, 106.0f + i * 94.0f}, {540.0f, 80.0f}};
}

sf::FloatRect MenuScreens::buildCard(int i) const {
    const int col = i % 3, row = i / 3;
    return {{694.0f + col * 178.0f, 270.0f + row * 122.0f}, {168.0f, 110.0f}};
}

sf::FloatRect MenuScreens::buildSlot(int slot) const {
    return {{724.0f + slot * 290.0f, 116.0f}, {272.0f, 120.0f}};
}

sf::FloatRect MenuScreens::buildSaveButton() const {
    return {{1290.0f, 784.0f}, {270.0f, 62.0f}};
}

sf::FloatRect MenuScreens::bracketButton(int i) const {
    static const std::array<sf::FloatRect, 3> kRects = {{
        {{820.0f, 812.0f}, {150.0f, 50.0f}}, {{990.0f, 812.0f}, {200.0f, 50.0f}}, {{1214.0f, 800.0f}, {350.0f, 74.0f}}}};
    return kRects[static_cast<std::size_t>(std::clamp(i, 0, 2))];
}

sf::FloatRect MenuScreens::promptButton(int i) const {
    return {{560.0f, 470.0f + i * 66.0f}, {480.0f, 52.0f}};
}

sf::FloatRect MenuScreens::skinPillRect() const {
    return {{68.0f, 645.0f}, {390.0f, 40.0f}};
}

sf::FloatRect MenuScreens::skinPrevButton() const {
    return {{68.0f, 645.0f}, {52.0f, 40.0f}};
}

sf::FloatRect MenuScreens::skinNextButton() const {
    return {{406.0f, 645.0f}, {52.0f, 40.0f}};
}

void MenuScreens::cycleSkin(uint32_t charId, int dir) {
    if (charId >= skins_.size()) return;
    skins_[charId] = (skins_[charId] + (dir > 0 ? 1 : 3)) % 4;
    dirty_ = true;
    const auto& sk = rosterSkin(charId, skins_[charId]);
    notify("SKIN: " + std::string(sk.name));
}

sf::FloatRect MenuScreens::settingsSlider(int i) const {
    return {{430.0f, 290.0f + i * 110.0f}, {740.0f, 40.0f}};
}

sf::FloatRect MenuScreens::settingsTab(int tab) const {
    return tab == 0 ? sf::FloatRect{{410.0f, 190.0f}, {220.0f, 42.0f}}
                    : sf::FloatRect{{650.0f, 190.0f}, {250.0f, 42.0f}};
}

sf::FloatRect MenuScreens::settingsPreset(int p) const {
    static const std::array<sf::FloatRect, 3> kPresets = {{
        {{410.0f, 252.0f}, {240.0f, 38.0f}},
        {{665.0f, 252.0f}, {180.0f, 38.0f}},
        {{860.0f, 252.0f}, {170.0f, 38.0f}}
    }};
    return kPresets[static_cast<std::size_t>(std::clamp(p, 0, 2))];
}

sf::FloatRect MenuScreens::settingsActionKey(int action, int slot) const {
    const int act = std::clamp(action, 0, 7);
    const int sl = std::clamp(slot, 0, 1);
    if (act < 4) {
        const float x = sl == 0 ? 595.0f : 695.0f;
        const float y = 345.0f + act * 50.0f;
        return {{x, y}, {90.0f, 38.0f}};
    } else {
        const float x = sl == 0 ? 995.0f : 1095.0f;
        const float y = 345.0f + (act - 4) * 50.0f;
        return {{x, y}, {90.0f, 38.0f}};
    }
}

sf::FloatRect MenuScreens::settingsReset() const {
    return {{410.0f, 560.0f}, {240.0f, 38.0f}};
}

sf::FloatRect MenuScreens::settingsOk() const {
    return {{1040.0f, 690.0f}, {150.0f, 50.0f}};
}

sf::FloatRect MenuScreens::exitButton(int i) const {
    return i == 0 ? sf::FloatRect{{700.0f, 512.0f}, {190.0f, 62.0f}} : sf::FloatRect{{910.0f, 512.0f}, {170.0f, 62.0f}};
}

// ---------------------------------------------------------------------------
// Entrada
// ---------------------------------------------------------------------------

MenuAction MenuScreens::modalKey(Key key) {
    if (modal_ == Modal::Settings) {
        // Se estiver gravando nova tecla para uma ação
        if (rebindAction_ >= 0) {
            if (key == Key::Escape) {
                rebindAction_ = -1;
                return {};
            }
            if (key != Key::Unknown) {
                if (rebindSlot_ == 0) {
                    keyBindings_.bindings[static_cast<std::size_t>(rebindAction_)].primary = key;
                } else {
                    keyBindings_.bindings[static_cast<std::size_t>(rebindAction_)].secondary = key;
                }
                dirty_ = true;
                rebindAction_ = -1;
            }
            return {};
        }

        // Tab alterna entre Áudio e Controles
        if (key == Key::Tab) {
            settingsTab_ = 1 - settingsTab_;
            return {};
        }

        if (key == Key::Escape) {
            modal_ = Modal::None;
            return {};
        }

        if (settingsTab_ == 0) {
            if (isUp(key)) modalIndex_ = (modalIndex_ + 2) % 3;
            else if (isDown(key)) modalIndex_ = (modalIndex_ + 1) % 3;
            else if ((isLeft(key) || isRight(key)) && modalIndex_ < 2) {
                int& v = modalIndex_ == 0 ? sfxVolume_ : musicVolume_;
                v = std::clamp(v + (isLeft(key) ? -10 : 10), 0, 100);
            } else if (isConfirm(key) && modalIndex_ == 2) {
                modal_ = Modal::None;
            }
            return {};
        }

        if (settingsTab_ == 1) {
            // controlIndex_:
            // 0..2: Presets (Padrão, WASD, Numpad)
            // 3..18: Grade 4x4 (Linhas 0..3 x Colunas 0..3)
            //        col 0: Act row, Slot 0
            //        col 1: Act row, Slot 1
            //        col 2: Act row+4, Slot 0
            //        col 3: Act row+4, Slot 1
            // 19: Resetar
            // 20: OK
            if (isLeft(key)) {
                if (controlIndex_ >= 0 && controlIndex_ <= 2) {
                    if (controlIndex_ > 0) controlIndex_--;
                } else if (controlIndex_ >= 3 && controlIndex_ <= 18) {
                    const int idx = controlIndex_ - 3;
                    const int col = idx % 4;
                    if (col > 0) controlIndex_--;
                } else if (controlIndex_ == 20) {
                    controlIndex_ = 19;
                }
            } else if (isRight(key)) {
                if (controlIndex_ >= 0 && controlIndex_ <= 2) {
                    if (controlIndex_ < 2) controlIndex_++;
                } else if (controlIndex_ >= 3 && controlIndex_ <= 18) {
                    const int idx = controlIndex_ - 3;
                    const int col = idx % 4;
                    if (col < 3) controlIndex_++;
                } else if (controlIndex_ == 19) {
                    controlIndex_ = 20;
                }
            } else if (isUp(key)) {
                if (controlIndex_ >= 0 && controlIndex_ <= 2) {
                    controlIndex_ = (controlIndex_ < 2) ? 19 : 20;
                } else if (controlIndex_ >= 3 && controlIndex_ <= 18) {
                    const int idx = controlIndex_ - 3;
                    const int row = idx / 4;
                    const int col = idx % 4;
                    if (row > 0) controlIndex_ -= 4;
                    else controlIndex_ = std::min(col, 2);
                } else if (controlIndex_ == 19) {
                    controlIndex_ = 3 + 12; // row 3, col 0
                } else if (controlIndex_ == 20) {
                    controlIndex_ = 3 + 15; // row 3, col 3
                }
            } else if (isDown(key)) {
                if (controlIndex_ >= 0 && controlIndex_ <= 2) {
                    controlIndex_ = 3 + controlIndex_;
                } else if (controlIndex_ >= 3 && controlIndex_ <= 18) {
                    const int idx = controlIndex_ - 3;
                    const int row = idx / 4;
                    const int col = idx % 4;
                    if (row < 3) controlIndex_ += 4;
                    else controlIndex_ = (col < 2) ? 19 : 20;
                } else if (controlIndex_ == 19) {
                    controlIndex_ = 0;
                } else if (controlIndex_ == 20) {
                    controlIndex_ = 2;
                }
            } else if (isConfirm(key)) {
                if (controlIndex_ == 0) { keyBindings_ = KeyBindings::makeDefault(); dirty_ = true; }
                else if (controlIndex_ == 1) { keyBindings_ = KeyBindings::makeWasd(); dirty_ = true; }
                else if (controlIndex_ == 2) { keyBindings_ = KeyBindings::makeNumpad(); dirty_ = true; }
                else if (controlIndex_ >= 3 && controlIndex_ <= 18) {
                    const int idx = controlIndex_ - 3;
                    const int row = idx / 4;
                    const int col = idx % 4;
                    rebindAction_ = (col < 2) ? row : (row + 4);
                    rebindSlot_ = (col % 2);
                } else if (controlIndex_ == 19) {
                    keyBindings_ = KeyBindings::makeDefault();
                    dirty_ = true;
                } else if (controlIndex_ == 20) {
                    modal_ = Modal::None;
                }
            }
            return {};
        }
    } else if (modal_ == Modal::Exit) {
        if (isLeft(key) || isRight(key)) modalIndex_ = 1 - modalIndex_;
        else if (isBack(key)) modal_ = Modal::None;
        else if (isConfirm(key)) {
            if (modalIndex_ == 1) {
                MenuAction quit;
                quit.type = MenuAction::Type::Quit;
                return quit;
            }
            modal_ = Modal::None;
        }
    }
    return {};
}

MenuAction MenuScreens::onKey(Key key) {
    if (modal_ != Modal::None) return modalKey(key);
    switch (screen_) {
    case Screen::Boot:
        goTo(Screen::Title);
        return {};

    case Screen::Title:
        if (isUp(key)) titleIndex_ = (titleIndex_ + 2) % 3;
        else if (isDown(key)) titleIndex_ = (titleIndex_ + 1) % 3;
        else if (isConfirm(key)) return confirmTitle();
        else if (key == Key::Escape) openModal(Modal::Exit);
        break;

    case Screen::CharSelect:
        if (rouletteSteps_ > 0) break; // a roleta está girando
        if (key == Key::Q || key == Key::PageUp) {
            int ch = slotChar(slot_);
            if (ch >= 0) { cycleSkin(static_cast<uint32_t>(ch), -1); break; }
        }
        if (key == Key::E || key == Key::PageDown) {
            int ch = slotChar(slot_);
            if (ch >= 0) { cycleSkin(static_cast<uint32_t>(ch), 1); break; }
        }
        if (isLeft(key)) moveCharCursor(-1, 0);
        else if (isRight(key)) moveCharCursor(1, 0);
        else if (isUp(key)) moveCharCursor(0, -1);
        else if (isDown(key)) moveCharCursor(0, 1);
        else if (isConfirm(key)) return confirmCharSelect();
        else if (isBack(key)) goTo(Screen::Title);
        break;

    case Screen::ModeSelect:
        if (isUp(key)) modeIndex_ = (modeIndex_ + 5) % 6;
        else if (isDown(key)) modeIndex_ = (modeIndex_ + 1) % 6;
        else if (isConfirm(key)) return confirmMode();
        else if (isBack(key)) goTo(Screen::CharSelect);
        break;

    case Screen::Build: {
        if (slotPrompt_) {
            if (isLeft(key) || key == Key::Z || key == Key::J) slotPromptIndex_ = 0;
            else if (isRight(key) || key == Key::X || key == Key::K) slotPromptIndex_ = 1;
            if (key == Key::Z || key == Key::J || key == Key::X || key == Key::K || key == Key::Enter || key == Key::Space) {
                slotPrompt_ = false;
                equip(slotPromptIndex_);
            } else if (key == Key::Escape || key == Key::Backspace) {
                slotPrompt_ = false;
            }
            break;
        }
        if (key == Key::V) {
            previewFront_ = !previewFront_;
            glitchClock_.restart();
            glitchLen_ = 10.0f;
            break;
        }
        const int n = static_cast<int>(buildSkills().size());
        if (n == 0) { if (isBack(key) || key == Key::Enter) goTo(buildReturn_); break; }
        if (isLeft(key)) buildIndex_ = (buildIndex_ + n - 1) % n;
        else if (isRight(key)) buildIndex_ = (buildIndex_ + 1) % n;
        else if (isUp(key)) buildIndex_ = (buildIndex_ + n - 3) % n;
        else if (isDown(key)) buildIndex_ = (buildIndex_ + 3) % n;
        else if (keyBindings_.matches(ActionKey::PunchL, key) || key == Key::Z) equip(0);
        else if (keyBindings_.matches(ActionKey::PunchR, key) || key == Key::X) equip(1);
        else if (key == Key::Enter || keyBindings_.matches(ActionKey::Block, key) || key == Key::Space) equip(-1);
        else if (key == Key::Delete || keyBindings_.matches(ActionKey::Dodge, key) || key == Key::C) {
            const SkillId id = buildSkills()[static_cast<std::size_t>(buildIndex_)];
            const SkillLoadout& l = loadouts_[playerChar_];
            unequip(l.spec1 == id ? 0 : (l.spec2 == id ? 1 : (l.passive == id ? 2 : -1)));
        }
        else if (key == Key::Escape || key == Key::Backspace) goTo(buildReturn_);
        break;
    }

    case Screen::Versus:
        if (screenFrame_ >= kVersusSkipAfter) {
            if (screenFrame_ < 150) {
                screenFrame_ = versusReady_ ? 152 : 150;
            } else if (versusReady_) {
                screen_ = Screen::Fight;
                return startAction();
            }
        }
        break;

    case Screen::Bracket:
        if (isLeft(key)) bracketIndex_ = std::max(0, bracketIndex_ - 1);
        else if (isRight(key)) bracketIndex_ = std::min(2, bracketIndex_ + 1);
        else if (isConfirm(key)) return confirmBracket();
        else if (isBack(key)) goTo(Screen::ModeSelect);
        break;

    case Screen::CampaignPrompt:
        if (isUp(key)) promptIndex_ = (promptIndex_ + 2) % 3;
        else if (isDown(key)) promptIndex_ = (promptIndex_ + 1) % 3;
        else if (isConfirm(key)) return confirmPrompt();
        else if (isBack(key)) goTo(Screen::ModeSelect);
        break;

    case Screen::Fight:
        break;
    }
    return {};
}

void MenuScreens::onMouseMove(sf::Vector2f p) {
    if (modal_ == Modal::Settings) {
        if (settingsTab_ == 0) {
            for (int i = 0; i < 2; ++i) if (settingsSlider(i).contains(p)) modalIndex_ = i;
            if (settingsOk().contains(p)) modalIndex_ = 2;
        } else if (settingsTab_ == 1) {
            for (int i = 0; i < 3; ++i) if (settingsPreset(i).contains(p)) controlIndex_ = i;
            for (int act = 0; act < 8; ++act) {
                for (int sl = 0; sl < 2; ++sl) {
                    if (settingsActionKey(act, sl).contains(p)) {
                        const int r = (act < 4) ? act : (act - 4);
                        const int c = (act < 4) ? sl : (2 + sl);
                        controlIndex_ = 3 + r * 4 + c;
                    }
                }
            }
            if (settingsReset().contains(p)) controlIndex_ = 19;
            if (settingsOk().contains(p)) controlIndex_ = 20;
        }
        return;
    }
    if (modal_ == Modal::Exit) {
        for (int i = 0; i < 2; ++i) if (exitButton(i).contains(p)) modalIndex_ = i;
        return;
    }
    switch (screen_) {
    case Screen::Boot: return;
    case Screen::Title:
        for (int i = 0; i < 3; ++i) if (titleButton(i).contains(p)) titleIndex_ = i;
        break;
    case Screen::CharSelect:
        if (rouletteSteps_ > 0) break;
        for (int s = 0; s < kSlots; ++s) if (cardRect(s).contains(p)) { slot_ = s; charButtonIndex_ = -1; }
        for (int i = 0; i < 3; ++i) if (charButton(i).contains(p)) charButtonIndex_ = i;
        break;
    case Screen::ModeSelect:
        for (int i = 0; i < 6; ++i) if (modeButton(i).contains(p)) modeIndex_ = i;
        break;
    case Screen::Build:
        for (int i = 0; i < static_cast<int>(buildSkills().size()); ++i) if (buildCard(i).contains(p)) buildIndex_ = i;
        break;
    case Screen::Bracket:
        for (int i = 0; i < 3; ++i) if (bracketButton(i).contains(p)) bracketIndex_ = i;
        break;
    case Screen::CampaignPrompt:
        for (int i = 0; i < 3; ++i) if (promptButton(i).contains(p)) promptIndex_ = i;
        break;
    default:
        break;
    }
}

void MenuScreens::onMouseDrag(sf::Vector2f p) {
    if (modal_ != Modal::Settings || settingsTab_ != 0 || modalIndex_ > 1) return;
    const sf::FloatRect r = settingsSlider(modalIndex_);
    if (p.y < r.position.y - 40 || p.y > r.position.y + r.size.y + 30) return;
    (modalIndex_ == 0 ? sfxVolume_ : musicVolume_) = static_cast<int>(std::round(std::clamp((p.x - r.position.x) / r.size.x, 0.0f, 1.0f) * 100.0f));
}

MenuAction MenuScreens::onRightClick() {
    if (screen_ == Screen::Build && slotPrompt_) { slotPrompt_ = false; return {}; }
    return onKey(Key::Escape);
}

MenuAction MenuScreens::onClick(sf::Vector2f p) {
    onMouseMove(p);
    if (modal_ == Modal::Settings) {
        if (sf::FloatRect{{1175.0f, 137.0f}, {40.0f, 40.0f}}.contains(p)) {
            modal_ = Modal::None;
            rebindAction_ = -1;
            return {};
        }
        if (settingsTab(0).contains(p)) { settingsTab_ = 0; rebindAction_ = -1; return {}; }
        if (settingsTab(1).contains(p)) { settingsTab_ = 1; rebindAction_ = -1; return {}; }
        if (settingsOk().contains(p)) {
            modal_ = Modal::None;
            rebindAction_ = -1;
            return {};
        }

        if (settingsTab_ == 0) {
            for (int i = 0; i < 2; ++i) {
                sf::FloatRect r = settingsSlider(i);
                if (r.contains(p)) (i == 0 ? sfxVolume_ : musicVolume_) = static_cast<int>(std::round(std::clamp((p.x - r.position.x) / r.size.x, 0.0f, 1.0f) * 100.0f));
            }
        } else if (settingsTab_ == 1) {
            if (settingsPreset(0).contains(p)) { keyBindings_ = KeyBindings::makeDefault(); dirty_ = true; rebindAction_ = -1; controlIndex_ = 0; return {}; }
            if (settingsPreset(1).contains(p)) { keyBindings_ = KeyBindings::makeWasd(); dirty_ = true; rebindAction_ = -1; controlIndex_ = 1; return {}; }
            if (settingsPreset(2).contains(p)) { keyBindings_ = KeyBindings::makeNumpad(); dirty_ = true; rebindAction_ = -1; controlIndex_ = 2; return {}; }
            if (settingsReset().contains(p)) { keyBindings_ = KeyBindings::makeDefault(); dirty_ = true; rebindAction_ = -1; controlIndex_ = 19; return {}; }

            for (int act = 0; act < 8; ++act) {
                const float ly = (act < 4) ? (345.0f + act * 50.0f) : (345.0f + (act - 4) * 50.0f);
                const float lx = (act < 4) ? 410.0f : 810.0f;
                const sf::FloatRect rowRect{{lx - 6.0f, ly - 3.0f}, {385.0f, 44.0f}};

                for (int slot = 0; slot < 2; ++slot) {
                    if (settingsActionKey(act, slot).contains(p)) {
                        rebindAction_ = act;
                        rebindSlot_ = slot;
                        const int r = (act < 4) ? act : (act - 4);
                        const int c = (act < 4) ? slot : (2 + slot);
                        controlIndex_ = 3 + r * 4 + c;
                        return {};
                    }
                }

                if (rowRect.contains(p)) {
                    rebindAction_ = act;
                    rebindSlot_ = 0;
                    const int r = (act < 4) ? act : (act - 4);
                    controlIndex_ = 3 + r * 4;
                    return {};
                }
            }
        }
        return {};
    }
    if (modal_ == Modal::Exit) {
        for (int i = 0; i < 2; ++i) if (exitButton(i).contains(p)) return modalKey(Key::Enter);
        return {};
    }
    switch (screen_) {
    case Screen::Boot:
        goTo(Screen::Title);
        return {};
    case Screen::Title:
        for (int i = 0; i < 3; ++i) if (titleButton(i).contains(p)) return confirmTitle();
        break;
    case Screen::CharSelect: {
        if (rouletteSteps_ > 0) break;
        int ch = slotChar(slot_);
        if (ch >= 0) {
            if (skinPrevButton().contains(p)) { cycleSkin(static_cast<uint32_t>(ch), -1); return {}; }
            if (skinNextButton().contains(p) || skinPillRect().contains(p)) { cycleSkin(static_cast<uint32_t>(ch), 1); return {}; }
        }
        bool onTarget = false;
        for (int s = 0; s < kSlots; ++s) onTarget |= cardRect(s).contains(p);
        for (int i = 0; i < 3; ++i) onTarget |= charButton(i).contains(p);
        if (onTarget) return confirmCharSelect();
        break;
    }
    case Screen::ModeSelect:
        for (int i = 0; i < 6; ++i) if (modeButton(i).contains(p)) return confirmMode();
        break;
    case Screen::Build:
        if (slotPrompt_) {
            for (int i = 0; i < 2; ++i) if (slotPromptButton(i).contains(p)) { slotPrompt_ = false; equip(i); return {}; }
            slotPrompt_ = false; // clique fora cancela
            return {};
        }
        if (previewArea().contains(p)) {
            previewFront_ = !previewFront_;
            glitchClock_.restart();
            glitchLen_ = 10.0f;
            return {};
        }
        for (int i = 0; i < static_cast<int>(buildSkills().size()); ++i) if (buildCard(i).contains(p)) equip(-1);
        for (int slot = 0; slot < 3; ++slot) if (buildSlot(slot).contains(p)) unequip(slot);
        if (buildSaveButton().contains(p)) goTo(buildReturn_);
        break;
    case Screen::Versus:
        return onKey(Key::Enter);
    case Screen::Bracket:
        for (int i = 0; i < 3; ++i) if (bracketButton(i).contains(p)) return confirmBracket();
        break;
    case Screen::CampaignPrompt:
        for (int i = 0; i < 3; ++i) if (promptButton(i).contains(p)) return confirmPrompt();
        break;
    default:
        break;
    }
    return {};
}

void MenuScreens::moveCharCursor(int dx, int dy) {
    if (charButtonIndex_ >= 0) {
        if (dx != 0) charButtonIndex_ = (charButtonIndex_ + dx + 3) % 3;
        if (dy > 0) charButtonIndex_ = -1; // desce de volta para o elenco
        return;
    }
    if (dx != 0) slot_ = (slot_ + dx + kSlots) % kSlots;
    else if (dy < 0) charButtonIndex_ = 1; // sobe para CONFIRMAR
}

MenuAction MenuScreens::confirmTitle() {
    if (titleIndex_ == 1) { openModal(Modal::Settings); return {}; }
    if (titleIndex_ == 2) { openModal(Modal::Exit); modalIndex_ = 0; return {}; }
    goTo(Screen::CharSelect);
    return {};
}

void MenuScreens::openBuild(Screen returnTo) {
    buildIndex_ = 0;
    buildReturn_ = returnTo;
    goTo(Screen::Build);
}

MenuAction MenuScreens::confirmCharSelect() {
    if (charButtonIndex_ == 0) {
        goTo(Screen::Title);
        return {};
    }
    int c = slotChar(slot_);
    if (c < 0) {
        // Roleta: o cursor passa pelos lutadores desacelerando e para num deles (sem confirmar).
        std::vector<int> slots;
        for (int s = 0; s < kSlots; ++s) if (slotChar(s) >= 0 && kRoster[static_cast<std::size_t>(slotChar(s))].hasFightSprites) slots.push_back(s);
        if (slots.empty()) return {};
        rouletteTarget_ = slots[static_cast<std::size_t>(std::rand()) % slots.size()];
        rouletteSteps_ = 16 + static_cast<int>(slots.size()) + std::rand() % static_cast<int>(slots.size());
        rouletteTimer_ = 0;
        rouletteDelay_ = 2;
        charButtonIndex_ = -1;
        return {};
    }
    if (!kRoster[static_cast<std::size_t>(c)].hasFightSprites) {
        notify(std::string(kRoster[static_cast<std::size_t>(c)].name) + " CHEGA EM BREVE AO RINGUE!");
        return {};
    }
    playerChar_ = static_cast<uint32_t>(c);
    if (charButtonIndex_ == 2) {
        openBuild(Screen::CharSelect);
        return {};
    }
    goTo(Screen::ModeSelect);
    return {};
}

MenuAction MenuScreens::confirmMode() {
    if (modeIndex_ == kModeBack) {
        goTo(Screen::CharSelect);
        return {};
    }
    if (modeIndex_ == kModeBuild) {
        if (!SkillSystem::hasSkills(playerChar_)) {
            notify("HABILIDADES DESTE LUTADOR CHEGAM EM BREVE!");
            return {};
        }
        openBuild(Screen::ModeSelect);
        return {};
    }
    if (modeIndex_ == kModeArcade) {
        // Torneio salvo: pergunta se continua com aquele lutador ou começa outro com o atual.
        if (tournament_.active()) {
            promptIndex_ = 0;
            goTo(Screen::CampaignPrompt);
        } else {
            newCampaign();
        }
        return {};
    }
    arcade_ = false;
    if (modeIndex_ == kModeTraining) {
        enemyChar_ = 9; // O Espantalho
        versusTag_ = "MODO TREINO";
        goTo(Screen::Versus);
        return {};
    }
    if (modeIndex_ == kModeBoss) {
        enemyChar_ = 8; // Batalhator 2000
        versusTag_ = "BOSS FINAL";
        goTo(Screen::Versus);
        return {};
    }
    // Luta Solo: oponente sorteado entre os lutadores prontos, nunca o próprio jogador.
    versusTag_ = "LUTA SOLO";
    std::vector<uint32_t> pool;
    for (uint32_t c : playableChars()) if (c != playerChar_) pool.push_back(c);
    enemyChar_ = pool.empty() ? playerChar_ : pool[static_cast<std::size_t>(std::rand()) % pool.size()];
    goTo(Screen::Versus);
    return {};
}

MenuAction MenuScreens::update() {
    frame_++;
    if (screen_ == Screen::Build) updatePreview();
    else if (previewSkill_ != SkillId::None) { previewVideo_.clear(); previewSkill_ = SkillId::None; previewSkin_ = ~0u; }
    if (rouletteSteps_ > 0 && ++rouletteTimer_ >= rouletteDelay_) {
        rouletteTimer_ = 0;
        --rouletteSteps_;
        if (rouletteSteps_ == 0) {
            slot_ = rouletteTarget_;
            notify(std::string("A SORTE ESCOLHEU ") + kRoster[static_cast<std::size_t>(slotChar(slot_))].name + "!");
        } else {
            do { slot_ = (slot_ + 1) % kSlots; } while (slotChar(slot_) < 0);
            // Desacelera nos últimos passos e para exatamente no sorteado.
            if (rouletteSteps_ < 9) rouletteDelay_ += 2;
            if (rouletteSteps_ == 1) slot_ = rouletteTarget_;
        }
    }
    if (slot_ != lastSlot_) {
        prevSlot_ = lastSlot_;
        lastSlot_ = slot_;
        slotChangeFrame_ = frame_;
    }
    screenFrame_++;
    // Hold the intact VS artwork, before the white transition, until all assets are resident.
    if (screen_ == Screen::Versus && !versusReady_ && screenFrame_ > 150) screenFrame_ = 150;
    modalFrame_++;
    if (noticeTimer_ > 0 && --noticeTimer_ == 0) notice_.clear();
    if (screen_ == Screen::Boot && screenFrame_ >= 200) {
        goTo(Screen::Title);
        return {};
    }
    if (screen_ == Screen::Versus && versusReady_ && screenFrame_ >= kVersusFrames) {
        screen_ = Screen::Fight;
        return startAction();
    }
    return {};
}

MenuAction MenuScreens::startAction() const {
    MenuAction action;
    action.type = MenuAction::Type::StartMatch;
    action.playerChar = playerChar_;
    action.enemyChar = enemyChar_;
    action.playerSkin = playerChar_ < skins_.size() ? skins_[playerChar_] : 0;
    action.enemySkin = enemyChar_ < skins_.size() ? skins_[enemyChar_] : 0;
    action.loadout = playerChar_ < loadouts_.size() ? loadouts_[playerChar_] : SkillLoadout{};
    action.arcade = arcade_;
    action.stage = arcade_ ? tournament_.stage : 1;
    action.training = (enemyChar_ == 9);
    return action;
}

// ---------------------------------------------------------------------------
// Torneio Arcade
// ---------------------------------------------------------------------------

void MenuScreens::newCampaign() {
    tournament_ = Tournament::create(static_cast<int>(playerChar_));
    dirty_ = true;
    showBracket();
}

void MenuScreens::showBracket() {
    bracketIndex_ = 2;
    goTo(Screen::Bracket);
}

void MenuScreens::tournamentWon() {
    tournament_.recordWin();
    dirty_ = true;
}

void MenuScreens::tournamentLost() {
    tournament_ = Tournament{};
    dirty_ = true;
}

void MenuScreens::startBracketMatch() {
    enemyChar_ = static_cast<uint32_t>(tournament_.currentOpponent());
    arcade_ = true;
    static const std::array<const char*, 5> kTags = {"", "QUARTAS", "SEMIFINAL", "FINAL", "BOSS FINAL"};
    versusTag_ = kTags[static_cast<std::size_t>(std::clamp(tournament_.stage, 1, 4))];
    goTo(Screen::Versus);
}

MenuAction MenuScreens::confirmBracket() {
    if (bracketIndex_ == 0) goTo(Screen::ModeSelect);
    else if (bracketIndex_ == 1 || !tournament_.active()) newCampaign();
    else startBracketMatch();
    return {};
}

MenuAction MenuScreens::confirmPrompt() {
    if (promptIndex_ == 0) {
        playerChar_ = static_cast<uint32_t>(tournament_.player);
        showBracket();
    } else if (promptIndex_ == 1) {
        newCampaign();
    } else {
        goTo(Screen::ModeSelect);
    }
    return {};
}

// ---------------------------------------------------------------------------
// Montar Build
// ---------------------------------------------------------------------------

std::vector<SkillId> MenuScreens::buildSkills() const {
    // Especiais primeiro, passivas depois, na ordem do SKILL_DATABASE.
    std::vector<SkillId> specials, passives;
    for (const SkillDef& s : SkillSystem::all()) {
        if (s.charId != playerChar_) continue;
        (s.type == SkillType::Passive ? passives : specials).push_back(s.id);
    }
    specials.insert(specials.end(), passives.begin(), passives.end());
    return specials;
}

void MenuScreens::equip(int slot) {
    std::vector<SkillId> skills = buildSkills();
    if (buildIndex_ < 0 || buildIndex_ >= static_cast<int>(skills.size())) return;
    const SkillId id = skills[static_cast<std::size_t>(buildIndex_)];
    const SkillDef* def = SkillSystem::get(id);
    if (!def) return;
    SkillLoadout& l = loadouts_[playerChar_];

    if (def->type == SkillType::Passive) {
        if (slot == 0 || slot == 1) {
            notify("ESSA É UMA PASSIVA: VAI NO ESPAÇO DE PASSIVA");
            return;
        }
        l.passive = id;
        equipFlash_[2] = frame_;
        notify(std::string(def->name) + " EQUIPADA NA PASSIVA");
    } else {
        if (slot == 2) {
            notify("O ESPAÇO DE PASSIVA SÓ ACEITA PASSIVAS");
            return;
        }
        if (slot < 0) {
            // Clique/Enter: já equipado fica onde está; vai para um encaixe vazio ou pergunta qual trocar.
            if (l.spec1 == id || l.spec2 == id) { notify("ESSE GOLPE JÁ ESTÁ EQUIPADO"); return; }
            if (l.spec1 == SkillId::None) slot = 0;
            else if (l.spec2 == SkillId::None) slot = 1;
            else { slotPrompt_ = true; slotPromptIndex_ = 0; return; }
        }
        SkillId& chosen = slot == 0 ? l.spec1 : l.spec2;
        SkillId& other = slot == 0 ? l.spec2 : l.spec1;
        if (other == id) std::swap(chosen, other); // já estava no outro espaço: troca os dois
        else chosen = id;
        equipFlash_[static_cast<std::size_t>(slot)] = frame_;
        if (other == chosen) {} // (troca já registrada)
        notify(std::string(def->name) + (slot == 0 ? " NO ESPECIAL 1" : " NO ESPECIAL 2"));
    }
    dirty_ = true;
}

void MenuScreens::unequip(int slot) {
    SkillLoadout& l = loadouts_[playerChar_];
    if (slot == 0) l.spec1 = SkillId::None;
    else if (slot == 1) l.spec2 = SkillId::None;
    else if (slot == 2) l.passive = SkillId::None;
    else return;
    equipFlash_[static_cast<std::size_t>(slot)] = frame_;
    notify("ESPAÇO LIBERADO");
    dirty_ = true;
}

// ---------------------------------------------------------------------------
// Prévia ao vivo do golpe (ringue de treino da build)
// ---------------------------------------------------------------------------

void MenuScreens::restartPreview(SkillId id, float glitchFrames) {
    previewSkill_ = id;
    previewSkin_ = skin(playerChar_);
    previewVideoFront_ = previewFront_;
    const auto path = sprites_ ? sprites_->rigDirectory().parent_path() / "previews" /
        ("skill_" + std::to_string(static_cast<unsigned>(id)) + "_skin" +
         std::to_string(previewSkin_) + (previewFront_ ? "_front.kpv" : "_back.kpv"))
        : std::filesystem::path{};
    previewVideo_.open(path);
    previewClock_.restart();
    glitchClock_.restart();
    glitchLen_ = glitchFrames;
}

void MenuScreens::updatePreview() {
    const std::vector<SkillId> skills = buildSkills();
    if (skills.empty()) return;
    const SkillId id = skills[static_cast<std::size_t>(std::clamp(buildIndex_, 0, static_cast<int>(skills.size()) - 1))];
    if (id != previewSkill_ || previewSkin_ != skin(playerChar_) || previewVideoFront_ != previewFront_)
        restartPreview(id, previewSkill_ == SkillId::None ? 0.0f : 8.0f);
}

} // namespace KoRama
