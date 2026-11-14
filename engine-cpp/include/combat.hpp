#ifndef KORAMA_COMBAT_HPP
#define KORAMA_COMBAT_HPP

#include "fighter.hpp"
#include "particles.hpp"
#include <string>
#include <vector>

namespace KoRama {

struct FloatingTextItem {
    float x, y, z;
    float life = 1.0f;
    std::string text;
    uint32_t colorHex = 0xffffffff;
    bool isCrit = false;
    uint32_t side = 0; // 0=left, 1=right
};

// Números de dano flutuantes e banner de status da luta.
// As regras de acerto ficam no GameEngine, que conhece rounds, quedas e combos.
class CombatSystem {
public:
    std::vector<FloatingTextItem> floatingTexts;
    std::string statusBanner;
    float statusBannerTimer = 0.0f;

    void update(float dt);
    void clear();

    void spawnFloatingText(const std::string& text, uint32_t colorHex, float x, float y, float z, bool isCrit, uint32_t side);
    void triggerStatusBanner(const std::string& msg);

    void fillRenderData(GameRenderState& state) const;
};

} // namespace KoRama

#endif // KORAMA_COMBAT_HPP
