// app3d/rig/ropes.hpp: as cordas do ringue, com a mola de updateRopePhysics do jogo 2D. Cada lado tem três cordas que
// cedem para fora no ponto em que o corpo encosta. Metros; medidas de arena_pernambuco.js (meia largura 270, alturas 38/68/98).
#pragma once
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
namespace KoRama {
// um corpo encostando nas cordas: centro no chão (x, z), quanto o tronco vai para fora por metro de altura (inclinado para trás) e o raio do tronco
struct RopeBody { glm::vec2 base{0.f}, lean{0.f}; float r = 0.32f; };
struct RingRopes {
    static constexpr float kUnit = 0.0122f, kHalf = 270.f * kUnit, kMaxBulge = 0.62f;
    static constexpr float kHeights[3] = {38.f * kUnit, 68.f * kUnit, 98.f * kUnit};
    static constexpr int kMaxRows = 4;
    int rows = 3; float heights[kMaxRows] = {kHeights[0], kHeights[1], kHeights[2], 0.f};   // por arena
    float d[4][kMaxRows]{}, v[4][kMaxRows]{}, contact[4]{0.5f, 0.5f, 0.5f, 0.5f};
    float weight(int r) const { return rows < 2 ? 1.f : 0.65f + 0.35f * float(r) / float(rows - 1); }   // a de cima cede mais (como no jogo)
    static glm::vec2 corner(int i) { const int k = i % 4; return {k == 1 || k == 2 ? kHalf : -kHalf, k >= 2 ? kHalf : -kHalf}; }
    static glm::vec2 outward(int side) { const glm::vec2 e = glm::normalize(corner(side + 1) - corner(side)); return {e.y, -e.x}; }
    // bodies: centro do corpo (x, z) em metros; bodyR: raio com que ele empurra (corpo de pé, sem inclinar)
    void tick(const glm::vec2* bodies, int n, float bodyR) { RopeBody b[4]; n = std::min(n, 4); for (int i = 0; i < n; i++) { b[i].base = bodies[i]; b[i].r = bodyR; } tick(b, n); }
    // cada corda vê o corpo na altura dela; se o corpo passa da linha, a corda abre na hora (sem esperar a mola) e depois volta devagar
    void tick(const RopeBody* bodies, int n) {
        for (int s = 0; s < 4; s++) {
            const glm::vec2 a = corner(s), e = corner(s + 1) - a; const float len = glm::length(e); const glm::vec2 tan = e / len, out = outward(s);
            float push[kMaxRows]{}, best = 0.f;
            for (int b = 0; b < n; b++) {
                const glm::vec2 w0 = bodies[b].base - a; const float t = glm::dot(w0, tan) / len; if (t < 0.04f || t > 0.96f) continue;
                for (int r = 0; r < rows; r++) { const float reach = glm::dot(w0 + bodies[b].lean * heights[r], out) + bodies[b].r; if (reach > push[r]) push[r] = std::min(kMaxBulge, reach);
                    if (reach > best) { best = reach; contact[s] = std::clamp(t, 0.12f, 0.88f); } }
            }
            for (int r = 0; r < rows; r++) { const float want = std::max(push[r], best > 0.f ? std::min(kMaxBulge, best) * weight(r) * 0.6f : 0.f);   // a corda puxa as vizinhas um pouco
                v[s][r] += (want - d[s][r]) * 0.12f; v[s][r] *= 0.8f; d[s][r] += v[s][r];
                if (d[s][r] < push[r]) { d[s][r] = push[r]; v[s][r] = std::max(v[s][r], 0.f); } }
        }
    }
    // ponto da corda (t de 0 a 1 ao longo do lado): cede em V suavizado, preso nos postes
    glm::vec3 point(int side, int row, float t) const {
        const glm::vec2 a = corner(side), b = corner(side + 1), o = outward(side); const float c = contact[side], x = t < c ? t / c : (1 - t) / (1 - c), sh = x * x * (3 - 2 * x);
        const glm::vec2 p = a + (b - a) * t + o * d[side][row] * sh;
        return {p.x, heights[row], p.y};
    }
};
}
