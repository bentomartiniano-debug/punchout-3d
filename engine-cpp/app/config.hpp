#ifndef KORAMA_APP_CONFIG_HPP
#define KORAMA_APP_CONFIG_HPP

#include <SFML/Graphics/Color.hpp>
#include <array>
#include <cstdint>

namespace KoRama {

// Resolução lógica herdada do canvas original; a janela escala com letterbox.
// A luta é desenhada em 800×600 e esticada para a tela 16:9, como o canvas do jogo original;
// HUD e menus usam 1600×900 (o tamanho em que o HTML original foi desenhado).
constexpr float kLogicalWidth = 800.0f;
constexpr float kLogicalHeight = 600.0f;
constexpr float kUiWidth = 1600.0f;
constexpr float kUiHeight = 900.0f;
constexpr float kFov = 450.0f; // valor em uso no index.html (src/config/constants.js está desatualizado)

constexpr int kTickRate = 60;
constexpr double kTickSeconds = 1.0 / kTickRate;
constexpr int kMaxTicksPerFrame = 5;


struct CharacterLook {
    const char* name;
    sf::Color color;
};

// Índice = char_id do motor (0 jack ... 8 boss); cores de CHAR_INFO no index.html.
inline const std::array<CharacterLook, 10> kCharacters = {{
    {"JACK", sf::Color(0xff, 0xd7, 0x00)},
    {"KING FROG", sf::Color(0x2e, 0xcc, 0x71)},
    {"JOANA", sf::Color(0xe6, 0x5c, 0x00)},
    {"O CREMOSO", sf::Color(0xfe, 0xf0, 0x8a)},
    {"VALÉRIA", sf::Color(0xff, 0x45, 0x00)},
    {"VIVALDI", sf::Color(0x9d, 0x4e, 0xdd)},
    {"LADY ESPINHA", sf::Color(0x00, 0xf5, 0xd4)},
    {"DR. MARCOV", sf::Color(0xd9, 0x04, 0x29)},
    {"BATALHATOR 2000", sf::Color(0xff, 0x00, 0x55)},
    {"O ESPANTALHO", sf::Color(0xd9, 0x77, 0x06)},
}};

inline const CharacterLook& lookFor(uint32_t charId) {
    return kCharacters[charId < kCharacters.size() ? charId : 0];
}

inline sf::Color colorFromRgba(uint32_t rgba) {
    return sf::Color(static_cast<std::uint8_t>(rgba >> 24), static_cast<std::uint8_t>(rgba >> 16),
                     static_cast<std::uint8_t>(rgba >> 8), static_cast<std::uint8_t>(rgba));
}

} // namespace KoRama

#endif // KORAMA_APP_CONFIG_HPP
