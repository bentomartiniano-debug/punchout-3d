// app3d/render/drips.hpp: as gotas do Cremoso (cremoso.js, makeDrips e handDrips): a própria pele estufa, a gota desce e é absorvida.
// Só as contas; o deslocamento é no shader do corpo e do contorno. Cada gota vira (ângulo, altura, raio, cauda).
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <glm/glm.hpp>
#include <vector>
namespace KoRama {
class DripSet {
public:
    static constexpr int kMax = 12;
    struct Zone { float y0, y1, th0, th1, r0, r1, w; };   // faixa em px da folha, ângulos em graus, raio e peso no sorteio
    glm::vec4 u[kMax]{};
    void initSkin(int n, std::vector<Zone> z, uint32_t seed, float t) { zones_ = std::move(z); hand_ = false; start(n, seed, t); }
    void initHand(int n, uint32_t seed, float t) { hand_ = true; start(n, seed, t); }
    void step(float t) {
        for (size_t i = 0; i < list_.size(); i++) { Drop& d = list_[i]; float p = (t - d.t0) / d.dur; if (p >= 1.f || p < 0.f) { spawn(d, t, false); p = 0.f; }
            if (hand_) { const float size = ss(0.f, 0.16f, p) * (1.f - ss(0.82f, 1.f, p)), q = std::clamp((p - 0.08f) / 0.8f, 0.f, 1.f), vc = d.v0 - (d.v0 + 0.92f) * std::pow(q, 1.6f); u[i] = {d.th, vc, d.R * size, std::min(0.9f, 0.15f + 0.7f * q) * size}; }
            else { const float size = ss(0.f, 0.16f, p) * (1.f - ss(0.78f, 1.f, p)), q = std::clamp((p - 0.08f) / 0.78f, 0.f, 1.f), yc = d.ys + d.travel * std::pow(q, 1.7f); u[i] = {d.th, yc, d.R * size, std::min((16.f + 58.f * q) * size, yc - zones_[size_t(d.zone)].y0)}; } }
    }
private:
    struct Drop { int zone = 0; float th = 0, R = 0, ys = 0, travel = 0, dur = 1, t0 = 0, v0 = 0; };
    std::vector<Drop> list_; std::vector<Zone> zones_; bool hand_ = false; uint32_t rng_ = 1;
    static float ss(float a, float b, float x) { const float k = std::clamp((x - a) / (b - a), 0.f, 1.f); return k * k * (3.f - 2.f * k); }
    float rnd() { rng_ = rng_ * 1664525u + 1013904223u; return float(rng_ >> 8) / 16777216.f; }
    void start(int n, uint32_t seed, float t) { rng_ = seed; list_.assign(size_t(std::min(n, kMax)), Drop{}); for (Drop& d : list_) spawn(d, t, true); }
    void spawn(Drop& d, float t, bool first) {
        if (hand_) { d.th = rnd() * 6.2831853f - 3.14159265f; d.R = 0.22f + rnd() * 0.1f; d.v0 = 0.35f + rnd() * 0.3f; d.dur = 2.6f + rnd() * 2.f; d.t0 = t - (first ? rnd() * d.dur : 0.f); return; }
        float W = 0.f; for (const Zone& z : zones_) W += z.w; float r = rnd() * W; d.zone = 0; for (size_t i = 0; i < zones_.size(); i++) { if (r < zones_[i].w) { d.zone = int(i); break; } r -= zones_[i].w; }
        const Zone& z = zones_[size_t(d.zone)]; d.R = z.r0 + rnd() * (z.r1 - z.r0); d.th = (z.th0 + rnd() * (z.th1 - z.th0)) * 3.14159265f / 180.f; if (d.th > 3.14159265f) d.th -= 6.2831853f;
        d.ys = z.y0 + d.R + 4.f + rnd() * 10.f; d.travel = std::max(0.f, std::min(z.y1 - d.R - d.ys, 40.f + rnd() * 90.f)); d.dur = 2.6f + d.travel / 40.f + rnd() * 1.5f; d.t0 = t - (first ? rnd() * d.dur : 0.f);
    }
};
}
