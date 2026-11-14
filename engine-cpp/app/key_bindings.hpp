#ifndef KORAMA_KEY_BINDINGS_HPP
#define KORAMA_KEY_BINDINGS_HPP

#include <SFML/Window/Keyboard.hpp>
#include <string>
#include <array>
#include <sstream>
#include <vector>
#include <algorithm>

namespace KoRama {

enum class ActionKey : std::size_t {
    PunchL = 0, // Jab / Soco Esquerdo
    PunchR = 1, // Cross / Direto Direito
    Dodge  = 2, // Esquiva Rápida
    Block  = 3, // Guarda / Bloqueio
    Up     = 4, // Cima / Uppercut
    Down   = 5, // Baixo / Duck
    Left   = 6, // Esquerda
    Right  = 7, // Direita
    Count  = 8
};

struct KeyConfig {
    sf::Keyboard::Key primary = sf::Keyboard::Key::Unknown;
    sf::Keyboard::Key secondary = sf::Keyboard::Key::Unknown;
};

struct KeyBindings {
    std::array<KeyConfig, static_cast<std::size_t>(ActionKey::Count)> bindings;

    static KeyBindings makeDefault() {
        KeyBindings b;
        using K = sf::Keyboard::Key;
        b.bindings[static_cast<std::size_t>(ActionKey::PunchL)] = {K::Z, K::J};
        b.bindings[static_cast<std::size_t>(ActionKey::PunchR)] = {K::X, K::K};
        b.bindings[static_cast<std::size_t>(ActionKey::Dodge)]  = {K::C, K::L};
        b.bindings[static_cast<std::size_t>(ActionKey::Block)]  = {K::Space, K::Unknown};
        b.bindings[static_cast<std::size_t>(ActionKey::Up)]     = {K::Up, K::W};
        b.bindings[static_cast<std::size_t>(ActionKey::Down)]   = {K::Down, K::S};
        b.bindings[static_cast<std::size_t>(ActionKey::Left)]   = {K::Left, K::A};
        b.bindings[static_cast<std::size_t>(ActionKey::Right)]  = {K::Right, K::D};
        return b;
    }

    static KeyBindings makeWasd() {
        KeyBindings b;
        using K = sf::Keyboard::Key;
        b.bindings[static_cast<std::size_t>(ActionKey::PunchL)] = {K::J, K::Unknown};
        b.bindings[static_cast<std::size_t>(ActionKey::PunchR)] = {K::K, K::Unknown};
        b.bindings[static_cast<std::size_t>(ActionKey::Dodge)]  = {K::L, K::Unknown};
        b.bindings[static_cast<std::size_t>(ActionKey::Block)]  = {K::Space, K::Unknown};
        b.bindings[static_cast<std::size_t>(ActionKey::Up)]     = {K::W, K::Unknown};
        b.bindings[static_cast<std::size_t>(ActionKey::Down)]   = {K::S, K::Unknown};
        b.bindings[static_cast<std::size_t>(ActionKey::Left)]   = {K::A, K::Unknown};
        b.bindings[static_cast<std::size_t>(ActionKey::Right)]  = {K::D, K::Unknown};
        return b;
    }

    static KeyBindings makeNumpad() {
        KeyBindings b;
        using K = sf::Keyboard::Key;
        b.bindings[static_cast<std::size_t>(ActionKey::PunchL)] = {K::Numpad4, K::Unknown};
        b.bindings[static_cast<std::size_t>(ActionKey::PunchR)] = {K::Numpad5, K::Unknown};
        b.bindings[static_cast<std::size_t>(ActionKey::Dodge)]  = {K::Numpad6, K::Unknown};
        b.bindings[static_cast<std::size_t>(ActionKey::Block)]  = {K::Numpad0, K::Space};
        b.bindings[static_cast<std::size_t>(ActionKey::Up)]     = {K::Numpad8, K::Up};
        b.bindings[static_cast<std::size_t>(ActionKey::Down)]   = {K::Numpad2, K::Down};
        b.bindings[static_cast<std::size_t>(ActionKey::Left)]   = {K::Numpad1, K::Left};
        b.bindings[static_cast<std::size_t>(ActionKey::Right)]  = {K::Numpad3, K::Right};
        return b;
    }

    bool matches(ActionKey action, sf::Keyboard::Key key) const {
        if (key == sf::Keyboard::Key::Unknown) return false;
        const auto& c = bindings[static_cast<std::size_t>(action)];
        return c.primary == key || c.secondary == key;
    }

    bool isHeld(ActionKey action) const {
        const auto& c = bindings[static_cast<std::size_t>(action)];
        if (c.primary != sf::Keyboard::Key::Unknown && sf::Keyboard::isKeyPressed(c.primary)) return true;
        if (c.secondary != sf::Keyboard::Key::Unknown && sf::Keyboard::isKeyPressed(c.secondary)) return true;
        return false;
    }

    static const char* actionLabel(ActionKey action) {
        switch (action) {
        case ActionKey::PunchL: return "Jab Esquerdo";
        case ActionKey::PunchR: return "Direto Direito";
        case ActionKey::Dodge:  return "Esquiva Rápida";
        case ActionKey::Block:  return "Guarda / Bloqueio";
        case ActionKey::Up:     return "Cima / Uppercut";
        case ActionKey::Down:   return "Baixo / Duck";
        case ActionKey::Left:   return "Esquerda / Gancho";
        case ActionKey::Right:  return "Direita / Cruzado";
        default: return "";
        }
    }

    static std::string keyName(sf::Keyboard::Key key) {
        using K = sf::Keyboard::Key;
        switch (key) {
        case K::A: return "A";
        case K::B: return "B";
        case K::C: return "C";
        case K::D: return "D";
        case K::E: return "E";
        case K::F: return "F";
        case K::G: return "G";
        case K::H: return "H";
        case K::I: return "I";
        case K::J: return "J";
        case K::K: return "K";
        case K::L: return "L";
        case K::M: return "M";
        case K::N: return "N";
        case K::O: return "O";
        case K::P: return "P";
        case K::Q: return "Q";
        case K::R: return "R";
        case K::S: return "S";
        case K::T: return "T";
        case K::U: return "U";
        case K::V: return "V";
        case K::W: return "W";
        case K::X: return "X";
        case K::Y: return "Y";
        case K::Z: return "Z";
        case K::Num0: return "0";
        case K::Num1: return "1";
        case K::Num2: return "2";
        case K::Num3: return "3";
        case K::Num4: return "4";
        case K::Num5: return "5";
        case K::Num6: return "6";
        case K::Num7: return "7";
        case K::Num8: return "8";
        case K::Num9: return "9";
        case K::Numpad0: return "NUM 0";
        case K::Numpad1: return "NUM 1";
        case K::Numpad2: return "NUM 2";
        case K::Numpad3: return "NUM 3";
        case K::Numpad4: return "NUM 4";
        case K::Numpad5: return "NUM 5";
        case K::Numpad6: return "NUM 6";
        case K::Numpad7: return "NUM 7";
        case K::Numpad8: return "NUM 8";
        case K::Numpad9: return "NUM 9";
        case K::Up: return "CIMA";
        case K::Down: return "BAIXO";
        case K::Left: return "ESQ";
        case K::Right: return "DIR";
        case K::Space: return "ESPAÇO";
        case K::Enter: return "ENTER";
        case K::Tab: return "TAB";
        case K::Backspace: return "BACKSPACE";
        case K::Escape: return "ESC";
        case K::LShift: return "L-SHIFT";
        case K::RShift: return "R-SHIFT";
        case K::LControl: return "L-CTRL";
        case K::RControl: return "R-CTRL";
        case K::LAlt: return "L-ALT";
        case K::RAlt: return "R-ALT";
        case K::Semicolon: return ";";
        case K::Comma: return ",";
        case K::Period: return ".";
        case K::Slash: return "/";
        case K::Backslash: return "\\";
        case K::Hyphen: return "-";
        case K::Equal: return "=";
        case K::LBracket: return "[";
        case K::RBracket: return "]";
        case K::Apostrophe: return "'";
        case K::Grave: return "`";
        default: return "-";
        }
    }

    static std::string shortKeyName(sf::Keyboard::Key key) {
        using K = sf::Keyboard::Key;
        if (key >= K::Numpad0 && key <= K::Numpad9) {
            return "N" + std::string(1, static_cast<char>('0' + (static_cast<int>(key) - static_cast<int>(K::Numpad0))));
        }
        if (key == K::Space) return "ESPAÇO";
        if (key == K::LShift || key == K::RShift) return "SHIFT";
        if (key == K::LControl || key == K::RControl) return "CTRL";
        if (key == K::LAlt || key == K::RAlt) return "ALT";
        return keyName(key);
    }

    std::string actionName(ActionKey action, bool useShort = false) const {
        const auto& c = bindings[static_cast<std::size_t>(action)];
        sf::Keyboard::Key k = c.primary != sf::Keyboard::Key::Unknown ? c.primary : c.secondary;
        if (k == sf::Keyboard::Key::Unknown) return "?";
        return useShort ? shortKeyName(k) : keyName(k);
    }

    static std::string keyToToken(sf::Keyboard::Key key) {
        using K = sf::Keyboard::Key;
        if (key >= K::A && key <= K::Z) {
            return std::string(1, static_cast<char>('A' + (static_cast<int>(key) - static_cast<int>(K::A))));
        }
        if (key >= K::Num0 && key <= K::Num9) {
            return "Num" + std::string(1, static_cast<char>('0' + (static_cast<int>(key) - static_cast<int>(K::Num0))));
        }
        if (key >= K::Numpad0 && key <= K::Numpad9) {
            return "Np" + std::string(1, static_cast<char>('0' + (static_cast<int>(key) - static_cast<int>(K::Numpad0))));
        }
        switch (key) {
        case K::Up: return "Up";
        case K::Down: return "Down";
        case K::Left: return "Left";
        case K::Right: return "Right";
        case K::Space: return "Space";
        case K::Enter: return "Enter";
        case K::Tab: return "Tab";
        case K::Backspace: return "Bksp";
        case K::Escape: return "Esc";
        case K::LShift: return "LShift";
        case K::RShift: return "RShift";
        case K::LControl: return "LCtrl";
        case K::RControl: return "RCtrl";
        case K::LAlt: return "LAlt";
        case K::RAlt: return "RAlt";
        case K::Semicolon: return "Semi";
        case K::Comma: return "Comma";
        case K::Period: return "Period";
        case K::Slash: return "Slash";
        case K::Backslash: return "Bslash";
        case K::Hyphen: return "Minus";
        case K::Equal: return "Equal";
        case K::LBracket: return "LBrack";
        case K::RBracket: return "RBrack";
        case K::Apostrophe: return "Apos";
        case K::Grave: return "Grave";
        default: return "-";
        }
    }

    static sf::Keyboard::Key tokenToKey(const std::string& token) {
        using K = sf::Keyboard::Key;
        if (token.empty() || token == "-") return K::Unknown;
        if (token.size() == 1 && token[0] >= 'A' && token[0] <= 'Z') {
            return static_cast<K>(static_cast<int>(K::A) + (token[0] - 'A'));
        }
        if (token.size() == 1 && token[0] >= 'a' && token[0] <= 'z') {
            return static_cast<K>(static_cast<int>(K::A) + (token[0] - 'a'));
        }
        if (token.rfind("Num", 0) == 0 && token.size() == 4 && token[3] >= '0' && token[3] <= '9') {
            return static_cast<K>(static_cast<int>(K::Num0) + (token[3] - '0'));
        }
        if (token.rfind("Np", 0) == 0 && token.size() == 3 && token[2] >= '0' && token[2] <= '9') {
            return static_cast<K>(static_cast<int>(K::Numpad0) + (token[2] - '0'));
        }
        if (token == "Up") return K::Up;
        if (token == "Down") return K::Down;
        if (token == "Left") return K::Left;
        if (token == "Right") return K::Right;
        if (token == "Space") return K::Space;
        if (token == "Enter") return K::Enter;
        if (token == "Tab") return K::Tab;
        if (token == "Bksp") return K::Backspace;
        if (token == "Esc") return K::Escape;
        if (token == "LShift") return K::LShift;
        if (token == "RShift") return K::RShift;
        if (token == "LCtrl") return K::LControl;
        if (token == "RCtrl") return K::RControl;
        if (token == "LAlt") return K::LAlt;
        if (token == "RAlt") return K::RAlt;
        if (token == "Semi") return K::Semicolon;
        if (token == "Comma") return K::Comma;
        if (token == "Period") return K::Period;
        if (token == "Slash") return K::Slash;
        if (token == "Bslash") return K::Backslash;
        if (token == "Minus") return K::Hyphen;
        if (token == "Equal") return K::Equal;
        if (token == "LBrack") return K::LBracket;
        if (token == "RBrack") return K::RBracket;
        if (token == "Apos") return K::Apostrophe;
        if (token == "Grave") return K::Grave;
        return K::Unknown;
    }

    std::string serialize() const {
        std::string s;
        for (std::size_t i = 0; i < static_cast<std::size_t>(ActionKey::Count); ++i) {
            if (i > 0) s += ' ';
            s += keyToToken(bindings[i].primary);
            s += ' ';
            s += keyToToken(bindings[i].secondary);
        }
        return s;
    }

    static KeyBindings deserialize(const std::string& line) {
        KeyBindings b = makeDefault();
        std::istringstream ss(line);
        std::string tok1, tok2;
        for (std::size_t i = 0; i < static_cast<std::size_t>(ActionKey::Count); ++i) {
            if (ss >> tok1 >> tok2) {
                b.bindings[i].primary = tokenToKey(tok1);
                b.bindings[i].secondary = tokenToKey(tok2);
            }
        }
        return b;
    }
};

} // namespace KoRama

#endif // KORAMA_KEY_BINDINGS_HPP
