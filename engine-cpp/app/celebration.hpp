#ifndef KORAMA_APP_CELEBRATION_HPP
#define KORAMA_APP_CELEBRATION_HPP

#include <SFML/Graphics/Rect.hpp>
#include "canvas2d.hpp"
#include <string>
#include <vector>

// Celebração de fim de luta (showVictoryRingCelebration do index.html): arte vetorial das rosas e das
// peças íntimas que a torcida joga no ringue e o card de vitória em estilo gibi.
namespace KoRama::celebration {

constexpr int kRoseKinds = 3;
constexpr int kUnderwearKinds = 12; // 6 calcinhas + 6 cuecas samba-canção
constexpr int kThrowableKinds = kRoseKinds + kUnderwearKinds;

// Desenha a peça centrada em (0,0) ocupando size x size (o SVG original tinha viewBox 200x200).
void drawThrowable(cv::Canvas& ctx, int kind, float size);

struct VictoryCard {
    std::string ribbon, title, badge, description;
    std::vector<std::pair<std::string, std::string>> buttons; // tecla, ação
    std::string headline = "VITÓRIA!"; // palavra gigante sobre a explosão
    int combo = 0;
    bool won = true;
    bool draw = false;
    bool perfect = false;
};

// Card no espaço de interface 1600x900, deslizando da direita (appear: 0..1 da animação de entrada).
// hits recebe os retângulos dos botões (espaço 1600x900); mouse destaca o botão sob o cursor.
void drawVictoryCard(cv::Canvas& ctx, const VictoryCard& card, float appear, float time,
                     std::vector<sf::FloatRect>* hits = nullptr, sf::Vector2f mouse = {-1.0f, -1.0f});

} // namespace KoRama::celebration

#endif
