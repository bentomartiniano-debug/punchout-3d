// app3d/render/sting_fx.hpp: Agulhada Rainha do Jack (Ferroada do vinil).
// Recolhe as colmeias e as junta na frente do peito; delas sai um ferrão de LUZ: haste fina, farpas e ponta larga em casca âmbar
// translúcida com núcleo branco, somada à cena (Renderer::glow), com três anéis de choque abrindo em volta. Cresce até passar do rival
// no quadro do dano (45% do golpe) e recolhe; faíscas douradas estouram na perfuração.
#pragma once
#include "particles.hpp"
#include "fighter.hpp"
#include "skills.hpp"
#include <algorithm>
#include <cmath>
#include <vector>
#include <glm/glm.hpp>
namespace KoRama {
struct StingFx {
    static constexpr int kHit = 11, kEnd = 22, kLen = 24;   // kLen: quadros do golpe no motor
    float t = -1.f; bool was = false, hit = false; glm::vec3 base{0.f}, dir{0, 0, 1}; float reachLen = 1.f;
    static bool active(const Fighter& f) { return f.charId == 0 && f.state == FIGHTER_SKILL && f.activeSkill == SkillId::JackStingJab; }
    void reset() { t = -1; was = false; }
    bool on() const { return t >= 0; }
    static float sat(float x) { return std::clamp(x, 0.f, 1.f); }
    static float smooth(float x) { x = sat(x); return x * x * (3.f - 2.f * x); }

    // pose: carga (colmeias para trás e abertas) e as duas colmeias juntas na frente do peito (sem o alcance do soco: quem vai longe é a luz)
    static void pose(Fighter& view, const Fighter& f) {
        if (!active(f)) return; const float e = skillElapsed(f);
        const float j = smooth(e / 5.f), back = j * (1.f - smooth((e - 5.f) / 2.f)), hitk = smooth((e - 5.f) / 3.f) * (1.f - smooth((e - 13.f) / 6.f));
        const float x = 26.f - 18.f * hitk + 8.f * back, z = 10.f - 20.f * back + 2.f * hitk;
        view.gloveL.x = -x; view.gloveR.x = x; view.gloveL.y = view.gloveR.y = 72.f + 4.f * j; view.gloveL.z = view.gloveR.z = z;
        view.offsetZ = -8.f * back + 10.f * hitk; view.bodySkew = 0.05f * hitk;
    }
    // alcance da luz (0..1), como no vinil: acende aos 28% do golpe, máximo aos 40%, recolhe até 74%
    float reach() const { if (t < 0) return 0.f; const float p = float(t) / float(kLen); if (p < 0.28f || p > 0.74f) return 0.f; return p <= 0.4f ? std::sin((p - 0.28f) / 0.12f * 1.5708f) : std::cos((p - 0.4f) / 0.34f * 1.5708f); }

    void tick(const Fighter& f, const glm::vec3& handL, const glm::vec3& handR, const glm::vec3& front, const glm::vec3& target, Particles& P) {
        const bool in = active(f);
        if (in && !was) { t = 0; hit = false; } else if (in) t = std::max(t, skillElapsed(f)); else if (t >= 0) { t += 1.f / skillStretch(f.charId); if (t > kEnd) t = -1; }
        was = in; if (t < 0) return;
        base = (handL + handR) * 0.5f + front * 0.18f; const glm::vec3 to = target - base; dir = glm::normalize(to + glm::vec3(1e-5f)); reachLen = glm::length(to) + 0.45f;   // passa do rival
        if (t >= 3 && t < 8) for (int k = 0; k < 3; k++) { Particle q; q.kind = kGlow; const glm::vec3 o = P.dir() * 0.3f; q.p = base + o; q.v = -o / 6.f; q.size = P.rnd(0.02f, 0.035f); q.life = 7; q.noFloor = true;
            q.c0 = glm::vec4(1.f, 0.97f, 0.7f, 1.f); q.c1 = glm::vec4(1.f, 0.7f, 0.15f, 1.f); P.emit(q); }   // a luz juntando entre as colmeias
        if (!hit && t >= kHit) { hit = true;
            for (int k = 0; k < 24; k++) { Particle q; q.kind = kSpark; q.p = target; const glm::vec3 r = glm::normalize(dir + P.dir() * 0.9f); q.v = r * P.rnd(0.04f, 0.09f); q.size = P.rnd(0.015f, 0.028f); q.gravity = 0.003f; q.drag = 0.97f; q.life = P.rnd(20, 34); P.emit(q); }
            for (int k = 0; k < 12; k++) { Particle q; q.kind = kLine; q.p = target; q.v = P.dir() * P.rnd(0.06f, 0.12f); q.size = P.rnd(0.03f, 0.05f); q.drag = 0.86f; q.life = P.rnd(8, 13); q.noFloor = true;
                q.c0 = glm::vec4(1.f, 0.95f, 0.6f, 1.f); q.c1 = glm::vec4(1.f, 0.6f, 0.1f, 0.f); P.emit(q); } }
    }

    // malha da luz (para Renderer::glow): torno do perfil farpado do vinil (casca e núcleo) e os anéis
    void build(std::vector<float>& v, std::vector<unsigned>& idx) const {
        const float R = reach(); if (R <= 0.02f) return;
        const auto put = [&](const glm::vec3& p, const glm::vec3& c) { const float d[12] = {p.x, p.y, p.z, 0, 1, 0, c.r, c.g, c.b, -2.f, -1.f, 0.f}; v.insert(v.end(), d, d + 12); };
        const glm::vec3 s1 = glm::normalize(glm::cross(dir, glm::vec3(0, 1, 0)) + glm::vec3(1e-5f)), s2 = glm::cross(dir, s1);
        static const float prof[7][2] = {{0.f, 0.f}, {0.05f, 0.04f}, {0.36f, 0.05f}, {0.42f, 0.11f}, {0.5f, 0.06f}, {0.64f, 0.15f}, {1.f, 0.f}};   // (ao longo, raio)
        const float L = reachLen * R, W = (0.5f + 0.5f * R) * 1.35f;   // um pouco mais grossa que no vinil: a câmera da luta é mais longe
        const auto lathe = [&](float k, const glm::vec3& col) { constexpr int N = 16; const unsigned b0 = unsigned(v.size() / 12);
            for (int i = 0; i < 7; i++) for (int j = 0; j < N; j++) { const float a = float(j) / N * 6.2832f; put(base + dir * (prof[i][0] * L) + (s1 * std::cos(a) + s2 * std::sin(a)) * (prof[i][1] * k * W), col); }
            for (int i = 0; i < 6; i++) for (int j = 0; j < N; j++) { const unsigned a = b0 + unsigned(i * N + j), b = b0 + unsigned(i * N + (j + 1) % N), c = a + N, d = b + N; idx.insert(idx.end(), {a, b, c, b, d, c}); } };
        lathe(1.f, glm::vec3(0.96f, 0.62f, 0.04f) * 0.55f);      // casca âmbar
        lathe(0.45f, glm::vec3(1.f, 0.97f, 0.76f) * 0.95f);     // núcleo branco
        for (int r = 0; r < 3; r++) { const float q = float(r + 1) / 3.f, e = (0.1f + 0.2f * q) * (1.f + q) * W, a = (1.f - q * 0.8f) * R * 0.9f; const glm::vec3 c = base + dir * (L * (0.3f + 0.25f * float(r))); constexpr int N = 32; const unsigned b0 = unsigned(v.size() / 12);
            for (int j = 0; j < N; j++) { const float an = float(j) / N * 6.2832f; const glm::vec3 o = s1 * std::cos(an) + s2 * std::sin(an); put(c + o * (e * 0.86f), glm::vec3(1.f, 0.92f, 0.4f) * a); put(c + o * e, glm::vec3(1.f, 0.92f, 0.4f) * a); }
            for (int j = 0; j < N; j++) { const unsigned i0 = b0 + unsigned(j * 2), i1 = i0 + 1, i2 = b0 + unsigned(((j + 1) % N) * 2), i3 = i2 + 1; idx.insert(idx.end(), {i0, i1, i2, i1, i3, i2}); } }   // anéis de choque
    }
};
}
