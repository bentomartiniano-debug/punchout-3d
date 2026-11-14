// app3d/render/stab_fx.hpp: estacas que saem do chão sob o rival e voltam para dentro dele.
//  - Estocada de Lava (Valéria): ela ergue os punhos e bate no chão; a rachadura em brasa caminha dela até o rival, levantando o chão,
//    e as estacas rompem nele no quadro do dano. Estacas de rocha magmática rompem o chão de uma vez (facetadas, com veios de lava
//    acesos), jogando lascas, poeira e brasas; depois afundam de volta levantando poeira.
//  - Inverno Allegro (Vivaldi): ela rege com a mão; a geada se espalha e os cristais se materializam um a um; o central acerta no quadro do dano. Feixe de cristais de gelo cresce em volta do rival (facetas azuis com
//    brilho branco), cintilando; depois os cristais afundam soltando lascas e névoa.
// As peças vão pelo Renderer::props (toon com contorno); veio aceso usa uv.x = -2. Tempo em quadros (o do motor enquanto o golpe dura).
#pragma once
#include "particles.hpp"
#include "fighter.hpp"
#include "skills.hpp"
#include <algorithm>
#include <cmath>
#include <vector>
#include <glm/glm.hpp>
namespace KoRama {
struct StabFx {
    enum Style { kNone = -1, kRock = 0, kIce = 1 };
    static constexpr int kSlam = 6;   // quadro em que a Valéria bate no chão
    struct Look { SkillId id; uint32_t charId; int warn, hit, sink, end; };
    static const Look& look(int s) { static const Look k[2] = {{SkillId::ValeriaLavaKickJab, 4, 3, 11, 44, 60}, {SkillId::VivaldiWinterAllegro, 5, 5, 13, 62, 80}}; return k[std::clamp(s, 0, 1)]; }
    static int styleOf(const Fighter& f) { if (f.state != FIGHTER_SKILL) return kNone; for (int s = 0; s < 2; s++) if (f.activeSkill == look(s).id && f.charId == look(s).charId) return s; return kNone; }
    static bool active(const Fighter& f) { return styleOf(f) != kNone; }

    struct Spike { glm::vec2 off; float r, h, tilt, yaw, delay; uint32_t seed; };
    float t = -1.f; int style = kNone; bool was = false, broke = false, sank = false; glm::vec3 at{0.f}, fwd{0, 0, 1}, from{0.f}, crackHead{0.f}; std::vector<Spike> spikes; uint32_t rng = 7u;
    float rnd(float a, float b) { rng = rng * 1664525u + 1013904223u; return a + (b - a) * float(rng >> 8) / 16777216.f; }
    void reset() { t = -1; was = false; style = kNone; spikes.clear(); }
    bool on() const { return t >= 0 && style != kNone; }
    static float sat(float x) { return std::clamp(x, 0.f, 1.f); }
    static float smooth(float x) { x = sat(x); return x * x * (3.f - 2.f * x); }
    const Look& lk() const { return look(style); }

    // quanto cada estaca está para fora (0 enterrada, 1 inteira); a rocha rompe de uma vez com sobra, o gelo cresce
    float rise(const Spike& s) const {
        // rocha: todas rompem juntas e chegam inteiras logo depois do dano; gelo: o anel se materializa um a um e o cristal do meio acerta no dano
        const bool mid = s.seed == 1u; const float start = style == kRock ? float(lk().hit - 2) + s.delay * 0.5f : mid ? float(lk().hit - 3) : float(lk().warn) + s.delay, grow = style == kRock ? 3.f : mid ? 3.f : 7.f;
        const float e = float(t) - start; if (e <= 0.f) return 0.f;
        float up = style == kRock ? (e < grow ? 1.f - (1.f - e / grow) * (1.f - e / grow) * (1.f - e / grow) : 1.f + 0.12f * std::exp(-(e - grow) * 0.5f) * std::cos((e - grow) * 1.2f))
                                  : smooth(e / grow) * (1.f + 0.07f * std::exp(-(e - grow) * 0.4f) * (e > grow ? 1.f : 0.f));
        const float s2 = float(t - lk().sink) - s.delay * 0.5f; if (s2 > 0.f) { const float k = sat(s2 / float(lk().end - lk().sink - 2)); up *= 1.f - k * k * (style == kRock ? 1.f : 0.9f) - (style == kIce ? 0.1f * k : 0.f); }
        return std::max(0.f, up);
    }

    void tick(const Fighter& f, const glm::vec3& casterFeet, const glm::vec3& targetFeet, const glm::vec3& front, Particles& P) {
        const int st = styleOf(f); const bool in = st != kNone; const float t0 = t;
        if (in && (!was || st != style)) { t = 0; style = st; broke = sank = false; at = targetFeet; fwd = front; from = casterFeet + front * 0.35f; crackHead = from; rng = uint32_t(f.timer * 977 + f.maxTimer * 131 + 5);
            spikes.clear();
            if (style == kRock) { spikes.push_back({{0.f, 0.f}, 0.27f, 1.6f, 0.f, rnd(0.f, 6.28f), 0.f, 1u});
                for (int i = 0; i < 5; i++) { const float a = float(i) / 5.f * 6.2832f + rnd(-0.3f, 0.3f), d = rnd(0.28f, 0.42f); spikes.push_back({{std::cos(a) * d, std::sin(a) * d}, rnd(0.14f, 0.19f), rnd(0.75f, 1.1f), rnd(0.25f, 0.45f), a, rnd(0.f, 1.5f), uint32_t(i + 2)}); } }
            else { for (int i = 0; i < 10; i++) { const float a = float(i) / 10.f * 6.2832f * 3.f + rnd(-0.2f, 0.2f), d = rnd(0.28f, 0.45f); spikes.push_back({{std::cos(a) * d, std::sin(a) * d}, rnd(0.09f, 0.14f), rnd(0.8f, 1.6f), rnd(0.18f, 0.42f), a, float(i) * 0.75f, uint32_t(i + 2)}); }   // um a um, pulando de lado
                spikes.push_back({{0.f, 0.f}, 0.15f, 1.9f, 0.f, 0.f, 2.f, 1u}); } }
        else if (in) t = std::max(t, skillElapsed(f));
        else if (t >= 0) { t += 1.f / skillStretch(f.charId); if (t > lk().end) { t = -1; style = kNone; spikes.clear(); } }
        was = in; if (t < 0) return;
        if (t < lk().hit - 2) { const glm::vec3 want = targetFeet - fwd * (style == kRock ? 0.3f : 0.f); at.x += (want.x - at.x) * 0.3f; at.z += (want.z - at.z) * 0.3f; }   // a rocha rompe um pouco à frente do rival
        const auto ring = [&](float a) { return glm::vec3(std::cos(a), 0.f, std::sin(a)); };
        const auto line = [&](const glm::vec3& a, const glm::vec3& b, float w, const glm::vec4& c0, const glm::vec4& c1, float life) { const glm::vec3 d = b - a; const float L = glm::length(d); if (L < 1e-3f) return;
            Particle q; q.kind = kLine; q.p = (a + b) * 0.5f; q.p.y = Particles::kFloor + 0.006f; q.dir = d / L; q.len = L + w * 0.5f; q.size = w; q.hold = true; q.noFloor = true; q.drag = 1.f; q.life = life; q.c0 = c0; q.c1 = c1; P.emit(q); };
        if (style == kRock) {
            // a rachadura sai do soco no chão e caminha até o rival: a ponta em brasa anda em zigue-zague, o chão estufa e solta lascas e poeira
            if (in && t >= kSlam && t <= lk().hit - 2) { const float k = float(t - kSlam + 1) / float(lk().hit - 2 - kSlam + 1); const glm::vec3 side = glm::normalize(glm::cross(glm::vec3(0, 1, 0), fwd) + glm::vec3(1e-5f));
                const glm::vec3 next = from + (at - from) * k + side * (rnd(-0.12f, 0.12f) * (1.f - k)); line(crackHead, next, 0.1f, {1.f, 0.62f, 0.14f, 1.f}, {0.22f, 0.06f, 0.03f, 1.f}, float(lk().end + 20));
                if (rnd(0.f, 1.f) < 0.7f) { const glm::vec3 m = (crackHead + next) * 0.5f, br = m + (side * rnd(-1.f, 1.f) + fwd * rnd(-0.3f, 0.3f)) * rnd(0.12f, 0.25f); line(m, br, 0.05f, {1.f, 0.55f, 0.12f, 1.f}, {0.2f, 0.05f, 0.03f, 1.f}, float(lk().end + 10)); }
                for (int i = 0; i < 5; i++) { Particle q; q.kind = kFlake; q.p = next + glm::vec3(P.rnd(-0.1f, 0.1f), 0.04f, P.rnd(-0.1f, 0.1f)); q.v = glm::vec3(P.rnd(-0.01f, 0.01f), P.rnd(0.03f, 0.06f), P.rnd(-0.01f, 0.01f)); q.size = P.rnd(0.025f, 0.05f); q.gravity = 0.004f; q.drag = 0.98f; q.life = P.rnd(24, 36);
                    q.c0 = glm::vec4(0.3f, 0.22f, 0.19f, 1.f); q.c1 = glm::vec4(0.12f, 0.08f, 0.07f, 1.f); P.emit(q); }   // o chão estufando
                for (int i = 0; i < 3; i++) { Particle q; q.kind = kPuff; q.p = next + glm::vec3(P.rnd(-0.12f, 0.12f), 0.05f, P.rnd(-0.12f, 0.12f)); q.v = glm::vec3(0, P.rnd(0.006f, 0.014f), 0); q.size = P.rnd(0.06f, 0.11f); q.grow = 0.002f; q.life = P.rnd(22, 34); q.drag = 0.94f;
                    q.c0 = glm::vec4(0.6f, 0.52f, 0.45f, 1.f); q.c1 = glm::vec4(0.35f, 0.3f, 0.27f, 1.f); P.emit(q); }
                crackHead = next; }
            if (t0 < float(lk().hit - 2) && t >= float(lk().hit - 2)) for (int r = 0; r < 8; r++) { float a = (float(r) + rnd(-0.3f, 0.3f)) / 8.f * 6.2832f; glm::vec3 p = at; const float reach = rnd(0.45f, 0.8f);   // cratera rachada em volta das estacas
                for (int i = 0; i < 3; i++) { a += rnd(-0.5f, 0.5f); const glm::vec3 q = p + ring(a) * (reach / 3.f); line(p, q, 0.08f * (1.f - 0.2f * float(i)), {1.f, 0.6f, 0.12f, 1.f}, {0.22f, 0.06f, 0.03f, 1.f}, float(lk().end + 20)); p = q; } }
            if (t >= kSlam && t < lk().hit - 2 && P.rnd(0.f, 1.f) < 0.6f) { Particle q; q.kind = kPuff; q.p = at + ring(P.rnd(0.f, 6.28f)) * P.rnd(0.f, 0.4f) + glm::vec3(0, 0.04f, 0); q.v = glm::vec3(0, P.rnd(0.004f, 0.01f), 0); q.size = P.rnd(0.05f, 0.09f); q.grow = 0.002f; q.life = P.rnd(20, 30); q.drag = 0.95f;
                q.c0 = glm::vec4(0.45f, 0.38f, 0.34f, 1.f); q.c1 = glm::vec4(0.25f, 0.21f, 0.2f, 1.f); P.emit(q); }   // fumacinha saindo das rachaduras
            if (!broke && t >= lk().hit - 2) { broke = true;   // rompe: lascas de rocha, poeira e brasas
                for (int i = 0; i < 26; i++) { const glm::vec3 o = ring(P.rnd(0.f, 6.28f)); Particle q; q.kind = kFlake; q.p = at + o * P.rnd(0.05f, 0.3f) + glm::vec3(0, 0.08f, 0); q.v = o * P.rnd(0.01f, 0.04f) + glm::vec3(0, P.rnd(0.05f, 0.11f), 0); q.size = P.rnd(0.035f, 0.07f); q.gravity = 0.0045f; q.drag = 0.985f; q.life = P.rnd(36, 56);
                    q.c0 = glm::vec4(0.28f, 0.2f, 0.18f, 1.f); q.c1 = glm::vec4(0.12f, 0.08f, 0.07f, 1.f); P.emit(q); }
                for (int i = 0; i < 22; i++) { const glm::vec3 o = ring(P.rnd(0.f, 6.28f)); Particle q; q.kind = kPuff; q.p = at + o * P.rnd(0.2f, 0.45f) + glm::vec3(0, 0.06f, 0); q.v = o * P.rnd(0.01f, 0.03f) + glm::vec3(0, P.rnd(0.003f, 0.012f), 0); q.size = P.rnd(0.08f, 0.15f); q.grow = 0.003f; q.drag = 0.93f; q.life = P.rnd(30, 50);
                    q.c0 = glm::vec4(0.6f, 0.52f, 0.45f, 1.f); q.c1 = glm::vec4(0.35f, 0.3f, 0.27f, 1.f); P.emit(q); }
                for (int i = 0; i < 20; i++) { Particle q; q.kind = kGlow; q.p = at + glm::vec3(0, 0.2f, 0) + P.dir() * 0.2f; q.v = P.dir() * P.rnd(0.02f, 0.05f) + glm::vec3(0, 0.03f, 0); q.size = P.rnd(0.015f, 0.03f); q.gravity = 0.002f; q.drag = 0.97f; q.life = P.rnd(24, 40); q.noFloor = true;
                    q.c0 = glm::vec4(1.f, 0.85f, 0.35f, 1.f); q.c1 = glm::vec4(0.95f, 0.25f, 0.05f, 1.f); P.emit(q); } }
            if (t > lk().sink && t < lk().end - 4 && P.rnd(0.f, 1.f) < 0.7f) { Particle q; q.kind = kPuff; q.p = at + ring(P.rnd(0.f, 6.28f)) * P.rnd(0.1f, 0.35f) + glm::vec3(0, 0.05f, 0); q.v = glm::vec3(0, P.rnd(0.002f, 0.008f), 0); q.size = P.rnd(0.06f, 0.11f); q.grow = 0.002f; q.life = P.rnd(24, 36); q.drag = 0.95f;
                q.c0 = glm::vec4(0.58f, 0.5f, 0.44f, 1.f); q.c1 = glm::vec4(0.32f, 0.28f, 0.25f, 1.f); P.emit(q); }   // afundando: poeira na boca do buraco
        } else {
            if (t >= lk().warn && t < lk().hit + 4) { const float k = sat(float(t - lk().warn) / float(lk().hit - lk().warn));   // geada se espalhando
                for (int i = 0; i < 4; i++) { const glm::vec3 o = ring(P.rnd(0.f, 6.28f)); Particle q; q.kind = kFlake; q.p = at + o * P.rnd(0.05f, 0.55f * k + 0.1f) + glm::vec3(0, 0.04f, 0); q.v = glm::vec3(0, 0.001f, 0); q.size = P.rnd(0.02f, 0.035f); q.life = P.rnd(40, 70); q.drag = 0.9f;
                    q.c0 = glm::vec4(0.93f, 0.98f, 1.f, 1.f); q.c1 = glm::vec4(0.6f, 0.82f, 1.f, 1.f); P.emit(q); }
                if (P.rnd(0.f, 1.f) < 0.6f) { Particle q; q.kind = kGlow; q.p = at + ring(P.rnd(0.f, 6.28f)) * P.rnd(0.f, 0.5f) + glm::vec3(0, P.rnd(0.03f, 0.25f), 0); q.v = glm::vec3(0, 0.004f, 0); q.size = P.rnd(0.015f, 0.03f); q.life = P.rnd(12, 22); q.noFloor = true; q.c0 = glm::vec4(1.f); q.c1 = glm::vec4(0.7f, 0.9f, 1.f, 1.f); P.emit(q); } }
            if (t > lk().hit && t < lk().sink && P.rnd(0.f, 1.f) < 0.35f) { Particle q; q.kind = kGlow; q.p = at + ring(P.rnd(0.f, 6.28f)) * P.rnd(0.1f, 0.4f) + glm::vec3(0, P.rnd(0.2f, 1.3f), 0); q.v = glm::vec3(0, 0.002f, 0); q.size = P.rnd(0.02f, 0.04f); q.life = P.rnd(8, 14); q.noFloor = true;
                q.c0 = glm::vec4(1.f); q.c1 = glm::vec4(0.75f, 0.92f, 1.f, 1.f); P.emit(q); }   // cintila
            if (!sank && t >= lk().sink) { sank = true; for (int i = 0; i < 24; i++) { const glm::vec3 o = ring(P.rnd(0.f, 6.28f)); Particle q; q.kind = kFlake; q.p = at + o * P.rnd(0.1f, 0.4f) + glm::vec3(0, P.rnd(0.1f, 0.8f), 0); q.v = o * P.rnd(0.01f, 0.025f) + glm::vec3(0, P.rnd(0.01f, 0.03f), 0); q.size = P.rnd(0.025f, 0.05f); q.gravity = 0.003f; q.drag = 0.98f; q.life = P.rnd(30, 46);
                q.c0 = glm::vec4(0.85f, 0.95f, 1.f, 1.f); q.c1 = glm::vec4(0.5f, 0.75f, 1.f, 1.f); P.emit(q); } }   // lascas de gelo
            if (t > lk().sink && t < lk().end - 6 && P.rnd(0.f, 1.f) < 0.6f) { Particle q; q.kind = kPuff; q.p = at + ring(P.rnd(0.f, 6.28f)) * P.rnd(0.1f, 0.4f) + glm::vec3(0, 0.05f, 0); q.v = glm::vec3(0, P.rnd(0.003f, 0.008f), 0); q.size = P.rnd(0.06f, 0.1f); q.grow = 0.0025f; q.life = P.rnd(26, 40); q.drag = 0.95f;
                q.c0 = glm::vec4(0.92f, 0.97f, 1.f, 1.f); q.c1 = glm::vec4(0.7f, 0.85f, 0.97f, 1.f); P.emit(q); }   // névoa fria
        }
    }

    // pose (no lugar do soco do motor): Valéria ergue os dois punhos e bate no chão; Vivaldi rege com a mão direita em oito
    static void pose(Fighter& view, const Fighter& f) {
        const int st = styleOf(f); if (st == kNone) return; const float e = skillElapsed(f);
        const auto mix = [](const PartRenderData& a, float x, float y, float z, float k) { PartRenderData r = a; r.x += (x - a.x) * k; r.y += (y - a.y) * k; r.z += (z - a.z) * k; return r; };
        if (st == kRock) {
            float y, z, oy;
            if (e < 5.f) { const float k = smooth(e / 5.f); y = 70.f + 55.f * k; z = 10.f; oy = 6.f * k; }
            else if (e < 7.f) { const float k = smooth((e - 5.f) / 2.f); y = 125.f - 121.f * k; z = 10.f + 25.f * k; oy = 6.f - 30.f * k; }
            else { y = 4.f; z = 35.f; oy = -24.f + std::sin(e * 2.3f) * 1.5f * (e < 12.f ? 1.f : 0.f); }
            const float back = smooth((e - 14.f) / 10.f);
            view.gloveL = mix(mix(view.baseGloveL, -20.f, y, z, 1.f), view.baseGloveL.x, view.baseGloveL.y, view.baseGloveL.z, back); view.gloveR = mix(mix(view.baseGloveR, 20.f, y, z, 1.f), view.baseGloveR.x, view.baseGloveR.y, view.baseGloveR.z, back);
            view.offsetY = oy * (1.f - back); view.bodySkew = 0.f;
        } else {
            const float env = smooth(e / 4.f) * (1.f - smooth((e - 22.f) / 6.f)), beat = smooth((e - 10.f) / 2.f) * (1.f - smooth((e - 14.f) / 4.f));   // o compasso forte no dano
            view.gloveR = mix(view.baseGloveR, 24.f + 16.f * std::sin(e * 0.45f), 92.f + 16.f * std::sin(e * 0.9f), 28.f + 6.f * std::cos(e * 0.45f) + 22.f * beat, env);
            view.gloveL = mix(view.baseGloveL, -28.f, 76.f + 5.f * std::sin(e * 0.45f + 1.f), 18.f, env);
            view.bodySkew = 0.05f * std::sin(e * 0.45f) * env; view.offsetY = 0.f;
        }
    }

    // malha: cada estaca é um prisma facetado e torto (anel da base, anel do meio e ponta), com uma face por cor (luz dura de quadrinho)
    void build(std::vector<float>& v, std::vector<unsigned>& idx) const {
        if (!on()) return;
        const auto put = [&](const glm::vec3& p, const glm::vec3& n, const glm::vec3& c, float flag, float ink) { const float d[12] = {p.x, p.y, p.z, n.x, n.y, n.z, c.r, c.g, c.b, flag, -1.f, ink}; v.insert(v.end(), d, d + 12); };
        const auto tri = [&](const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& col, float flag, float ink, const glm::vec3& center) {
            glm::vec3 n = glm::normalize(glm::cross(b - a, c - a) + glm::vec3(1e-7f)); const glm::vec3 m = (a + b + c) / 3.f; const bool flip = glm::dot(n, m - center) < 0.f; if (flip) n = -n;
            const unsigned base = unsigned(v.size() / 12); put(a, n, col, flag, ink); put(flip ? c : b, n, col, flag, ink); put(flip ? b : c, n, col, flag, ink); idx.insert(idx.end(), {base, base + 1, base + 2}); };
        for (const Spike& s : spikes) {
            const float up = rise(s); if (up <= 0.001f) continue;
            uint32_t h = s.seed * 2654435761u + 17u; const auto hr = [&](float a, float b) { h = h * 1664525u + 1013904223u; return a + (b - a) * float(h >> 8) / 16777216.f; };
            const int N = style == kRock ? 5 : 6; const float H = s.h, R = s.r;
            // eixo inclinado para fora; a estaca sobe ao longo dele e entra no chão (abaixo do piso o ringue esconde)
            const glm::vec3 out(std::cos(s.yaw), 0.f, std::sin(s.yaw)); const glm::vec3 axis = glm::normalize(glm::vec3(0, 1, 0) * std::cos(s.tilt) + out * std::sin(s.tilt));
            const glm::vec3 base = at + glm::vec3(s.off.x, 0.f, s.off.y) - axis * (H * (1.f - std::min(up, 1.f))) - axis * 0.08f;
            const float L = H * std::max(up, 1.f);   // a sobra do tranco estica a ponta
            const glm::vec3 side1 = glm::normalize(glm::cross(axis, glm::vec3(0.31f, 0.f, 0.95f)) + glm::vec3(1e-5f)), side2 = glm::cross(axis, side1);
            std::vector<glm::vec3> b0, b1; const glm::vec3 tip = base + axis * (L + 0.08f) + side1 * hr(-0.03f, 0.03f) + side2 * hr(-0.03f, 0.03f);
            for (int i = 0; i < N; i++) { const float a = float(i) / float(N) * 6.2832f + hr(-0.2f, 0.2f); const glm::vec3 d = side1 * std::cos(a) + side2 * std::sin(a);
                b0.push_back(base + d * R * hr(0.85f, 1.15f)); b1.push_back(base + axis * ((L + 0.08f) * hr(0.45f, 0.6f)) + d * R * hr(0.55f, 0.8f)); }
            const glm::vec3 center = base + axis * (L * 0.4f);
            for (int i = 0; i < N; i++) { const int j = (i + 1) % N;
                glm::vec3 col; float flag = -1.f;
                if (style == kRock) { const float shade = hr(0.f, 1.f); col = glm::mix(glm::vec3(0.16f, 0.11f, 0.1f), glm::vec3(0.3f, 0.21f, 0.18f), shade); }
                else { const int k = (i + int(s.seed)) % 3; col = k == 0 ? glm::vec3(0.86f, 0.96f, 1.f) : k == 1 ? glm::vec3(0.45f, 0.76f, 1.f) : glm::vec3(0.62f, 0.86f, 1.f); }
                tri(b0[size_t(i)], b0[size_t(j)], b1[size_t(j)], col, flag, 0.006f, center); tri(b0[size_t(i)], b1[size_t(j)], b1[size_t(i)], col, flag, 0.006f, center);
                const glm::vec3 col2 = style == kRock ? col * 0.9f : glm::mix(col, glm::vec3(1.f), 0.35f); tri(b1[size_t(i)], b1[size_t(j)], tip, col2, flag, 0.006f, center);
                // rocha: veio de lava aceso em algumas faces (uma faixa fina no meio da face)
                if (style == kRock && (i + int(s.seed)) % 2 == 0) { const glm::vec3 fa = (b0[size_t(i)] + b0[size_t(j)]) * 0.5f, fb = (b1[size_t(i)] + b1[size_t(j)]) * 0.5f, fn = glm::normalize(fa - (base) + glm::vec3(1e-5f)) * 0.016f, w = (b0[size_t(j)] - b0[size_t(i)]) * 0.17f;
                    const glm::vec3 hot(1.f, 0.55f, 0.12f); tri(fa - w + fn, fa + w + fn, fb + fn + w * 0.4f, hot, -2.f, 0.f, center); tri(fa - w + fn, fb + fn + w * 0.4f, fb + fn - w * 0.4f, hot, -2.f, 0.f, center); } }
        }
    }
};
}
