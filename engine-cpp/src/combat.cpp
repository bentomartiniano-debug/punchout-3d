#include "combat.hpp"
#include <cstring>
#include <cstdio>
#include <algorithm>

namespace KoRama {

void CombatSystem::update(float dt) {
    for (size_t i = 0; i < floatingTexts.size(); ) {
        FloatingTextItem& item = floatingTexts[i];
        item.y += 0.8f;
        item.life -= 0.025f;

        if (item.life <= 0.0f) {
            floatingTexts.erase(floatingTexts.begin() + i);
        } else {
            i++;
        }
    }

    if (statusBannerTimer > 0.0f) {
        statusBannerTimer -= dt;
        if (statusBannerTimer <= 0.0f) {
            statusBanner.clear();
        }
    }
}

void CombatSystem::clear() {
    floatingTexts.clear();
    statusBanner.clear();
    statusBannerTimer = 0.0f;
}

void CombatSystem::spawnFloatingText(const std::string& text, uint32_t colorHex, float x, float y, float z, bool isCrit, uint32_t side) {
    if (floatingTexts.size() >= MAX_FLOATING_TEXTS) floatingTexts.erase(floatingTexts.begin());

    FloatingTextItem item;
    item.x = x; item.y = y; item.z = z;
    item.life = 1.0f;
    item.text = text;
    item.colorHex = colorHex;
    item.isCrit = isCrit;
    item.side = side;
    floatingTexts.push_back(item);
}

void CombatSystem::triggerStatusBanner(const std::string& msg) {
    statusBanner = msg;
    statusBannerTimer = 2.5f;
}

void CombatSystem::fillRenderData(GameRenderState& state) const {
    state.floating_text_count = static_cast<uint32_t>(std::min(floatingTexts.size(), static_cast<size_t>(MAX_FLOATING_TEXTS)));
    for (uint32_t i = 0; i < state.floating_text_count; i++) {
        const FloatingTextItem& item = floatingTexts[i];
        state.floating_texts[i].x = item.x;
        state.floating_texts[i].y = item.y;
        state.floating_texts[i].z = item.z;
        state.floating_texts[i].life = item.life;
        state.floating_texts[i].color_hex = item.colorHex;
        state.floating_texts[i].is_crit = item.isCrit ? 1 : 0;
        state.floating_texts[i].side = item.side;
        std::strncpy(state.floating_texts[i].text, item.text.c_str(), MAX_TEXT_LEN - 1);
        state.floating_texts[i].text[MAX_TEXT_LEN - 1] = '\0';
    }

    if (!statusBanner.empty()) {
        std::strncpy(state.status_banner, statusBanner.c_str(), MAX_TEXT_LEN - 1);
        state.status_banner[MAX_TEXT_LEN - 1] = '\0';
        state.status_banner_timer = statusBannerTimer;
    } else {
        state.status_banner[0] = '\0';
        state.status_banner_timer = 0.0f;
    }
}

} // namespace KoRama
