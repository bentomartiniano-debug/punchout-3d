// app3d/render/geyser_fx.hpp: o Gêiser do Pântano do King Frog com a água da onda da Joana (shader water_ do Renderer).
// Antes do golpe o chão borbulha nos pés do rival; a coluna dispara e acerta no quadro do motor (45% de 28); lá em cima a água abre numa copa
// que pinga; no fim a coluna desaba em espuma e espirra em anel. Malha refeita a cada quadro: corpo (face clara, riscos subindo),
// copa (lábio rasgado com espuma na crista) e tampa. Tempo em quadros de 60 Hz; o do motor enquanto o golpe dura.
#pragma once
#include "particles.hpp"
#include "fighter.hpp"
#include "skills.hpp"
#include <algorithm>
#include <cmath>
#include <vector>
#include <glm/glm.hpp>
namespace KoRama {
struct GeyserFx {
    static constexpr int kRise = 7, kHit = 13, kFall = 34, kEnd = 58;
    // lava: a Erupção Solavanco da Valéria é a mesma coluna em lava (o shader troca a paleta; a espuma vira fogo, lava, pedra e fumaça)
    float t = -1.f; bool was = false, burst = false, splash = false, lava = false; glm::vec3 at{0.f}; float size = 1.f;
    static bool active(const Fighter& f) { return f.state == FIGHTER_SKILL && ((f.charId == 1 && f.activeSkill == SkillId::FrogSwampSpout) || (f.charId == 4 && f.activeSkill == SkillId::ValeriaVolcanicErupt)); }
    void reset() { t = -1; was = false; }
    bool on() const { return t >= 0; }
    static float sat(float x) { return std::clamp(x, 0.f, 1.f); }
    static float smooth(float x) { x = sat(x); return x * x * (3.f - 2.f * x); }
    float ft() const { return float(t); }
    // altura da coluna: dispara com sobra, assenta, e desaba
    float height() const {
        const float f = ft(), Hm = 2.7f * size; if (t < kRise) return 0.f;
        if (f <= kHit) { const float a = (f - kRise) / float(kHit - kRise); return Hm * 1.08f * (1.f - (1.f - a) * (1.f - a) * (1.f - a)); }
        if (f <= kHit + 6) return Hm * (1.08f - 0.08f * smooth((f - kHit) / 6.f));
        if (f <= kFall) return Hm * (1.f + 0.03f * std::sin(f * 0.7f));
        return Hm * (1.f - 0.85f * smooth((f - kFall) / float(kEnd - kFall - 4)));
    }
    float foamy() const { return t < kFall + 4 ? 0.f : smooth((ft() - kFall - 4) / float(kEnd - kFall - 6)); }
    float bloom() const { return smooth((ft() - kRise - 3) / 8.f); }   // a copa abre depois que a coluna chega lá em cima

    // feet: pés de quem lança; targetFeet: pés do rival; uma vez por quadro do motor
    void tick(const Fighter& f, const glm::vec3& targetFeet, float sz, Particles& P) {
        const bool in = active(f);
        if (in && !was) { t = 0; burst = splash = false; at = targetFeet; size = sz; lava = f.charId == 4; }
        else if (in) t = std::max(t, skillElapsed(f));
        else if (t >= 0) { t += 1.f / skillStretch(f.charId); if (t > kEnd) t = -1; }
        was = in; if (t < 0) return;
        if (t < kRise) { at.x += (targetFeet.x - at.x) * 0.3f; at.z += (targetFeet.z - at.z) * 0.3f; }
        const auto foam = [&](const glm::vec3& p, const glm::vec3& vel, float s, float life, float grav) { Particle q; q.p = p; q.v = vel; q.size = s; q.life = life; q.gravity = grav; q.drag = 0.95f;
            if (!lava) { q.kind = kFoam; q.c0 = glm::vec4(1.f); q.c1 = glm::vec4(0.88f, 0.96f, 1.f, 1.f); P.emit(q); return; }
            const float r = P.rnd(0.f, 1.f);   // lava: bola de fogo, gota de lava ou fumaça escura subindo
            if (r < 0.45f) { q.kind = kFire; q.c0 = glm::vec4(1.f, 0.55f, 0.1f, 1.f); q.c1 = glm::vec4(0.35f, 0.22f, 0.2f, 1.f); q.gravity = -0.0005f; }
            else if (r < 0.8f) { q.kind = kEnergy; q.size *= 0.6f; q.c0 = glm::vec4(1.f, 0.42f, 0.06f, 1.f); q.c1 = glm::vec4(0.6f, 0.08f, 0.03f, 1.f); q.gravity = std::max(grav, 0.002f); }
            else { q.kind = kPuff; q.c0 = glm::vec4(0.35f, 0.3f, 0.29f, 1.f); q.c1 = glm::vec4(0.16f, 0.14f, 0.14f, 1.f); q.gravity = -0.0006f; q.grow = 0.003f; q.life *= 1.6f; }
            P.emit(q); };
        const auto drop = [&](const glm::vec3& p, const glm::vec3& vel, float s, float life) { Particle q; q.kind = kGlow; q.p = p; q.v = vel; q.size = s; q.life = life; q.gravity = 0.0032f; q.drag = 0.985f;
            if (lava) { q.c0 = glm::vec4(1.f, 0.85f, 0.35f, 1.f); q.c1 = glm::vec4(0.95f, 0.25f, 0.05f, 1.f); }   // brasas
            else { q.c0 = glm::vec4(0.92f, 0.98f, 1.f, 1.f); q.c1 = glm::vec4(0.6f, 0.86f, 0.96f, 1.f); }
            P.emit(q); };
        // lava: antes de estourar o chão racha brilhando em volta dos pés do rival; no estouro voam pedras
        if (lava && t == 0) { for (int r = 0; r < 9; r++) { float a = (float(r) + P.rnd(-0.3f, 0.3f)) / 9.f * 6.2832f; glm::vec3 p = at; const float reach = P.rnd(0.6f, 1.1f) * size;
                for (int i = 0; i < 4; i++) { a += P.rnd(-0.5f, 0.5f); const glm::vec3 q = p + glm::vec3(std::cos(a), 0, std::sin(a)) * (reach / 4.f); const glm::vec3 d = q - p; const float L = glm::length(d);
                    Particle c; c.kind = kLine; c.p = (p + q) * 0.5f; c.p.y = Particles::kFloor + 0.006f; c.dir = d / L; c.len = L; c.size = 0.09f * size * (1.f - 0.18f * float(i)); c.hold = true; c.noFloor = true; c.drag = 1.f; c.life = float(kEnd + 30);
                    c.c0 = glm::vec4(1.f, 0.62f, 0.12f, 1.f); c.c1 = glm::vec4(0.25f, 0.06f, 0.03f, 1.f); P.emit(c); p = q; } } }
        if (lava && !burst && t >= kRise) for (int i = 0; i < 18; i++) { const float a = P.rnd(0.f, 6.2832f); Particle q; q.kind = kFlake; q.p = at + glm::vec3(std::cos(a), 0, std::sin(a)) * 0.2f * size + glm::vec3(0, 0.1f, 0);
            q.v = glm::vec3(std::cos(a) * P.rnd(0.01f, 0.03f), P.rnd(0.06f, 0.12f), std::sin(a) * P.rnd(0.01f, 0.03f)); q.size = P.rnd(0.04f, 0.08f) * size; q.gravity = 0.004f; q.drag = 0.985f; q.life = P.rnd(40, 60);
            q.c0 = glm::vec4(0.3f, 0.2f, 0.18f, 1.f); q.c1 = glm::vec4(0.12f, 0.08f, 0.07f, 1.f); P.emit(q); }   // pedras
        const auto ring = [&](float a) { return glm::vec3(std::cos(a), 0.f, std::sin(a)); };
        // borbulha: o chão ferve nos pés do rival, cada vez mais
        if (t < kRise) { const float k = (ft() + 1.f) / float(kRise); for (int i = 0; i < 1 + int(k * 3.f); i++) { const glm::vec3 o = ring(P.rnd(0.f, 6.2832f)) * P.rnd(0.f, 0.45f) * size;
                foam(at + o + glm::vec3(0, 0.03f, 0), glm::vec3(0, P.rnd(0.004f, 0.012f), 0), P.rnd(0.05f, 0.11f) * size * (0.5f + k), P.rnd(10, 18), 0.f); }
            if (P.rnd(0.f, 1.f) < 0.5f) drop(at + ring(P.rnd(0.f, 6.2832f)) * 0.2f * size + glm::vec3(0, 0.05f, 0), glm::vec3(P.rnd(-0.01f, 0.01f), P.rnd(0.02f, 0.04f), P.rnd(-0.01f, 0.01f)), 0.03f * size, 20); }
        // estouro: anel de espuma que corre pelo chão e jatos de gotas
        if (!burst && t >= kRise) { burst = true;
            for (int i = 0; i < 40; i++) { const float a = P.rnd(0.f, 6.2832f); const glm::vec3 o = ring(a);
                foam(at + o * P.rnd(0.25f, 0.5f) * size + glm::vec3(0, P.rnd(0.03f, 0.2f), 0), o * P.rnd(0.02f, 0.05f) * size + glm::vec3(0, P.rnd(0.005f, 0.02f), 0), P.rnd(0.12f, 0.26f) * size, P.rnd(26, 44), 0.0006f); }
            for (int i = 0; i < 30; i++) { const glm::vec3 o = ring(P.rnd(0.f, 6.2832f)); drop(at + o * 0.3f * size + glm::vec3(0, 0.1f, 0), o * P.rnd(0.01f, 0.03f) + glm::vec3(0, P.rnd(0.06f, 0.11f), 0), P.rnd(0.03f, 0.055f) * size, P.rnd(30, 46)); } }
        // subindo: cabeça de espuma estourando no topo e gotas espirrando para cima
        if (t >= kRise && t <= kHit + 2) { const float H = height(); for (int i = 0; i < 5; i++) { const glm::vec3 o = ring(P.rnd(0.f, 6.2832f));
                foam(at + o * P.rnd(0.f, 0.3f) * size + glm::vec3(0, H + P.rnd(-0.1f, 0.15f) * size, 0), o * P.rnd(0.005f, 0.025f) + glm::vec3(0, P.rnd(0.01f, 0.03f), 0), P.rnd(0.1f, 0.2f) * size, P.rnd(14, 24), 0.0015f); }
            for (int i = 0; i < 3; i++) { const glm::vec3 o = ring(P.rnd(0.f, 6.2832f)); drop(at + o * 0.15f * size + glm::vec3(0, H, 0), o * P.rnd(0.01f, 0.03f) + glm::vec3(0, P.rnd(0.04f, 0.08f), 0), P.rnd(0.025f, 0.045f) * size, P.rnd(26, 40)); } }
        // de pé: espuma no pé e a copa pingando em volta
        if (t >= kRise && t < kFall + 8) { const float H = height(), bl = bloom();
            for (int i = 0; i < 2; i++) { const glm::vec3 o = ring(P.rnd(0.f, 6.2832f)); foam(at + o * P.rnd(0.3f, 0.55f) * size + glm::vec3(0, 0.04f, 0), o * P.rnd(0.004f, 0.015f), P.rnd(0.1f, 0.2f) * size, P.rnd(18, 30), 0.f); }
            if (bl > 0.3f) for (int i = 0; i < 3; i++) { const glm::vec3 o = ring(P.rnd(0.f, 6.2832f)); drop(at + o * (0.33f + 0.5f * bl) * size + glm::vec3(0, H - 0.25f * size, 0), o * P.rnd(0.005f, 0.02f), P.rnd(0.025f, 0.05f) * size, P.rnd(30, 44)); }
            if (bl > 0.3f && P.rnd(0.f, 1.f) < 0.5f) { const glm::vec3 o = ring(P.rnd(0.f, 6.2832f)); foam(at + o * (0.35f + 0.45f * bl) * size + glm::vec3(0, H + 0.05f * size, 0), o * 0.01f + glm::vec3(0, 0.008f, 0), P.rnd(0.06f, 0.12f) * size, P.rnd(14, 22), 0.002f); } }
        // desaba: a água bate no chão e espirra
        if (!splash && t >= kFall + 10) { splash = true;
            for (int i = 0; i < 46; i++) { const glm::vec3 o = ring(P.rnd(0.f, 6.2832f)); foam(at + o * P.rnd(0.3f, 0.8f) * size + glm::vec3(0, P.rnd(0.03f, 0.25f), 0), o * P.rnd(0.01f, 0.035f) + glm::vec3(0, P.rnd(0.f, 0.012f), 0), P.rnd(0.14f, 0.3f) * size, P.rnd(30, 50), 0.0004f); } }
    }

    // malha da água: v = (pos, normal, u, s, parte) por vértice; parte 0 é lábio (borda rasgada e crista), 1 é face
    void build(std::vector<float>& v, std::vector<unsigned>& idx) const {
        v.clear(); idx.clear(); const float H = height(); if (!on() || H < 0.02f) return;
        const float T = ft() / 60.f, bl = bloom(), rTop = 0.3f * size, lean = 0.06f * size * std::sin(T * 4.f);
        constexpr int NU = 48;
        const float Hn = std::max(0.3f, H / size);   // as bolhas de volume sobem pelo corpo (a água correndo para cima)
        const auto wob = [&](float u, float s) { const float y = s * Hn, a = u * 3.14159f;
            return 1.f + 0.14f * std::sin(a * 3.f + T * 10.f + s * 7.f) + 0.07f * std::sin(a * 7.f - T * 15.f + s * 11.f)
                 + 0.2f * std::pow(std::max(0.f, std::sin(y * 4.2f - T * 26.f + std::sin(a * 2.f) * 0.8f)), 3.f); };
        const auto place = [&](float u, float r, float y) { const float a = (u + 1.f) * 3.14159f; return at + glm::vec3(std::cos(a) * r + lean * (y / std::max(0.1f, H)), y, std::sin(a) * r); };
        const float Htop = H + 0.12f * size * bl;
        const auto body = [&](float u, float s) { const float r = (0.52f + (0.27f - 0.52f) * smooth(s / 0.22f) + 0.05f * smooth((s - 0.75f) / 0.25f)) * size * wob(u, s); return place(u, r, s * Htop); };
        const auto crown = [&](float u, float s) { const float r = rTop * wob(u, 1.f) + s * 0.55f * size * bl, y = Htop + (0.16f * std::sin(s * 3.14159f * 0.75f) - 0.62f * s * s) * size * bl; return place(u, r, y); };
        const auto cap = [&](float u, float s) { const float r = s * rTop * wob(u, 1.f); return place(u, r, Htop + 0.07f * size * (1.f - s * s)); };
        const auto grid = [&](int NS, int part, auto&& fn, int sgnMode) {
            std::vector<glm::vec3> P(size_t(NU * NS)); for (int i = 0; i < NU; i++) for (int j = 0; j < NS; j++) P[size_t(i * NS + j)] = fn(-1.f + 2.f * float(i) / float(NU - 1), float(j) / float(NS - 1));
            const auto pt = [&](int i, int j) { if (i < 0) i += NU - 1; if (i >= NU) i -= NU - 1; return P[size_t(i * NS + std::clamp(j, 0, NS - 1))]; };
            const unsigned base = unsigned(v.size() / 9);
            for (int i = 0; i < NU; i++) for (int j = 0; j < NS; j++) { const glm::vec3 p = pt(i, j); glm::vec3 n = glm::cross(pt(i, j + 1) - pt(i, j - 1), pt(i + 1, j) - pt(i - 1, j)); const float L = glm::length(n); n = L > 1e-9f ? n / L : glm::vec3(0, 1, 0);
                const glm::vec3 out = glm::vec3(p.x - at.x, 0.f, p.z - at.z); if ((sgnMode == 0 ? glm::dot(n, out) : n.y) < 0.f) n = -n;
                const float d[9] = {p.x, p.y, p.z, n.x, n.y, n.z, -1.f + 2.f * float(i) / float(NU - 1), float(j) / float(NS - 1), float(part)}; v.insert(v.end(), d, d + 9); }
            for (int i = 0; i + 1 < NU; i++) for (int j = 0; j + 1 < NS; j++) { const unsigned a = base + unsigned(i * NS + j), b = a + unsigned(NS); idx.insert(idx.end(), {a, b, a + 1, a + 1, b, b + 1}); } };
        grid(30, 1, body, 0); grid(4, 1, cap, 1); if (bl > 0.02f) grid(14, 0, crown, 1);
    }
};
}
