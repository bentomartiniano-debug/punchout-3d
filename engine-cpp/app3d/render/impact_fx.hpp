// app3d/render/impact_fx.hpp: estrela e onomatopeia do golpe, com os tempos e palavras do jogo 2D (fight_hud.cpp).
// Só o estado e as contas; quem desenha é o laço do jogo.
#pragma once
#include <algorithm>
#include <glm/glm.hpp>
namespace KoRama {
struct ImpactFx {
    bool alive = false, placed = false;   // placed: o ponto do golpe já foi calculado pela pose
    int kind = 0, seed = 0, timer = 0, dur = 0;   // kind: 0 leve, 1 pesado, 2 contra-ataque, 3 brecha, 4 queda
    bool byPlayer = true;                  // quem bateu: é da luva dele que sai o ponto
    glm::vec3 at{0.f};
    void spawn(int k, int s, bool player) { alive = true; placed = false; kind = k; seed = s; byPlayer = player; dur = timer = k == 4 ? 40 : (k == 0 ? 16 : 26); }
    void tick() { if (alive && --timer <= 0) alive = false; }
    float age() const { return dur ? 1.f - float(timer) / float(dur) : 1.f; }
    float scale() const { const float a = age(), base = kind == 0 ? 0.3f : (kind == 4 ? 0.8f : 0.5f); return base * (a < 0.18f ? 0.5f + a / 0.18f * 0.7f : 1.2f - std::min(0.2f, (a - 0.18f) * 0.6f)); }
    float alpha() const { const float a = age(); return a > 0.75f ? std::max(0.f, 1.f - (a - 0.75f) / 0.25f) : 1.f; }
    const char* word() const {
        static const char* const kLight[] = {"PÁ!", "TUM!", "PLAFT!"};
        static const char* const kHeavy[] = {"POW!", "BAM!", "WHAM!", "SOCK!"};
        return kind == 0 ? kLight[seed % 3] : kind == 1 ? kHeavy[seed % 4] : kind == 2 ? "KA-POW!" : kind == 3 ? "CRACK!" : "K.O.!";
    }
};
}
