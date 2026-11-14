// app3d/render/water_fx.hpp: a onda da âncora da Joana (Onda da Âncora), feita na hora em vez das ondas extrudadas do vinil.
// Uma lâmina de água que nasce do lado da Joana, corre na diagonal até o rival (para a câmera ver o perfil) e quebra por cima: costas,
// crista e o lábio que enrola (o tubo), mais a face da frente côncava. A cada quadro a malha é montada de novo e o Renderer::water
// pinta em faixas de quadrinho (azul fundo, azul-piscina, água clara, crista e riscos de espuma, retícula e nanquim na silhueta).
// A espuma e os respingos são partículas kFoam (nuvens com contorno). Tempo em segundos de 60 quadros.
#pragma once
#include "particles.hpp"
#include "fighter.hpp"
#include <algorithm>
#include <cmath>
#include <vector>
#include <glm/glm.hpp>
namespace KoRama {

struct WaveFx {
    static constexpr int kNU = 72, kNS = 40, kNF = 14;            // divisões: ao longo da onda, no perfil (costas+lábio), na face da frente
    static constexpr float kRise = 0.06f, kCrash = 0.19f, kEnd = 0.55f;   // sobe, quebra no rival (no acerto do golpe: a pausa do impacto congela a quebra), acaba
    bool on = false, fired = false, crashed = false; float t = 0.f, size = 1.f; glm::vec3 from{0.f}, to{0.f}, dir{0.f, 0.f, 1.f}, side{1.f, 0.f, 0.f};
    static constexpr float kSlant = -0.75f;   // a onda entra na diagonal (rad): da câmera atrás do rival se vê o perfil dela enrolando

    void start(const glm::vec3& casterFeet, const glm::vec3& targetFeet, float sz) {
        on = true; crashed = false; t = 0.f; from = casterFeet; to = targetFeet; size = sz;
        glm::vec3 b = to - from; b.y = 0.f; b = glm::length(b) > 1e-4f ? glm::normalize(b) : glm::vec3(0, 0, 1); const float c = std::cos(kSlant), s = std::sin(kSlant);
        dir = glm::vec3(b.x * c + b.z * s, 0.f, -b.x * s + b.z * c); side = glm::normalize(glm::cross(glm::vec3(0, 1, 0), dir)); }
    static float ss(float a, float b, float x) { x = std::clamp((x - a) / (b - a), 0.f, 1.f); return x * x * (3.f - 2.f * x); }
    // o momento: altura da crista, distância até o rival, quanto o lábio enrolou e a abertura do arco (rad)
    float height() const { return 1.6f * size * ss(0.f, kRise, t) * (1.f - 0.95f * ss(kCrash + 0.04f, kCrash + 0.32f, t)); }
    float radius() const { return glm::mix(2.2f * size, 1.25f * size, ss(0.f, kCrash, t)); }   // distância da frente até o rival
    float foamy() const { return ss(kCrash - 0.02f, kCrash + 0.3f, t); }   // ao quebrar, a água vira espuma
    float curl() const { return ss(0.02f, kCrash, t) + 0.3f * ss(kCrash, kEnd, t); }
    glm::vec3 back() const { return -dir; }

    // perfil em alturas da crista (z para o rival, y para cima). s 0..0,55: costas subindo até a crista; 0,55..1: o lábio enrolando para a frente e para baixo
    static glm::vec2 profile(float s, float c) {
        const float lead = 0.55f * c;   // o lábio se projeta para a frente, por cima do rival
        if (s < 0.55f) { const float q = s / 0.55f; return {-1.1f + (1.1f + lead) * q, 1.f - (1.f - q) * (1.f - q)}; }
        const float q = (s - 0.55f) / 0.45f, r = 0.42f, a = q * c * 3.0f; return {lead + r * std::sin(a), 1.f - r + r * std::cos(a)}; }
    // face da frente: do pé (que avança no chão) até embaixo da crista, rasa no pé e em pé no alto
    static glm::vec2 face(float k, float c) { const glm::vec2 F(0.55f + 0.15f * c, 0.f), U(0.2f * c, 0.78f - 0.3f * c); return {F.x + (U.x - F.x) * k, U.y * k * k}; }
    // altura e enrolar de cada ponto ao longo da onda: as pontas baixam e o meio quebra primeiro
    void local(float u, float H, float c, float& h, float& cu) const { h = H * (1.f - ss(0.68f, 1.f, std::fabs(u))) * (0.86f + 0.14f * std::sin(u * 9.f + t * 5.f));    cu = std::clamp(c * 1.2f - std::fabs(u) * 0.4f, 0.f, 1.45f); }
    // frente quase reta, de 2,8 m, com as pontas um pouco atrasadas (abraça o rival ao quebrar)
    glm::vec3 place(float u, const glm::vec2& zy, float h, float R) const {
        return to - dir * (R - zy.x * h + 0.55f * size * u * u) + side * (u * 1.4f * size) + glm::vec3(0.f, zy.y * h, 0.f); }

    // malha do quadro: por vértice posição, normal (para fora: costas para cima, face para o rival) e (u, s, parte)
    void build(std::vector<float>& v, std::vector<unsigned>& idx) const {
        v.clear(); idx.clear(); const float H = height(); if (!on || H < 0.01f) return;
        const float R = radius(), c = curl();
        const auto grid = [&](int NS, int part) {
            std::vector<glm::vec3> P(size_t(kNU * NS)); std::vector<float> U(kNU);
            for (int i = 0; i < kNU; i++) { const float u = -1.f + 2.f * float(i) / float(kNU - 1); U[size_t(i)] = u; float h, cu; local(u, H, c, h, cu);
                for (int j = 0; j < NS; j++) { const float s = float(j) / float(NS - 1); glm::vec2 zy = part == 0 ? profile(s, cu) : face(s, cu);
                    zy += glm::vec2(0.03f * std::sin(u * 17.f - s * 6.f + t * 11.f), 0.035f * std::sin(u * 21.f + s * 8.f - t * 13.f)) * (part == 0 ? 1.f : 0.6f) * std::min(1.f, s * 4.f);   // marola na superfície
                    P[size_t(i * NS + j)] = place(u, zy, h, R); } }
            const auto at = [&](int i, int j) { return P[size_t(std::clamp(i, 0, kNU - 1) * NS + std::clamp(j, 0, NS - 1))]; };
            // sentido da normal: confere num ponto do meio (costas olham para cima; face olha para o rival)
            const int mi = kNU / 2, mj = part == 0 ? NS / 4 : NS / 2; const glm::vec3 n0 = glm::cross(at(mi + 1, mj) - at(mi - 1, mj), at(mi, mj + 1) - at(mi, mj - 1));
            const float sg = part == 0 ? (n0.y >= 0.f ? 1.f : -1.f) : (glm::dot(n0, -back()) >= 0.f ? 1.f : -1.f);
            const unsigned base = unsigned(v.size() / 9);
            for (int i = 0; i < kNU; i++) for (int j = 0; j < NS; j++) { glm::vec3 n = glm::cross(at(i + 1, j) - at(i - 1, j), at(i, j + 1) - at(i, j - 1)) * sg; const float L = glm::length(n); n = L > 1e-9f ? n / L : glm::vec3(0, 1, 0);
                const glm::vec3 p = at(i, j); const float d[9] = {p.x, p.y, p.z, n.x, n.y, n.z, U[size_t(i)], float(j) / float(NS - 1), float(part)}; v.insert(v.end(), d, d + 9); }
            for (int i = 0; i + 1 < kNU; i++) for (int j = 0; j + 1 < NS; j++) { const unsigned a = base + unsigned(i * NS + j), b = a + unsigned(NS); idx.insert(idx.end(), {a, b, a + 1, a + 1, b, b + 1}); } };
        grid(kNS, 0); grid(kNF, 1);
    }

    // um quadro: anda o tempo, segue o rival até quebrar e solta espuma, respingos e brilhos
    void tick(const glm::vec3& targetFeet, Particles& P) {
        if (!on) return; t += 1.f / 60.f / kSkillStretch; if (t >= kEnd) { on = false; return; }   // no ritmo do especial alongado
        if (t < kCrash) { to.x += (targetFeet.x - to.x) * 0.25f; to.z += (targetFeet.z - to.z) * 0.25f; }
        const float H = height(), R = radius(), c = curl();
        const auto foam = [&](const glm::vec3& p, const glm::vec3& vel, float sz, float life, float grav) { Particle q; q.kind = kFoam; q.p = p; q.v = vel; q.size = sz; q.life = life; q.gravity = grav; q.drag = 0.95f;
            q.c0 = glm::vec4(1.f); q.c1 = glm::vec4(0.88f, 0.96f, 1.f, 1.f); P.emit(q); };
        const auto inward = [&](float) { return dir; };
        if (t < kCrash + 0.06f) {
            // respingo saindo da ponta do lábio
            for (int k = 0; k < 6; k++) { const float u = P.rnd(-0.85f, 0.85f); float h, cu; local(u, H, c, h, cu); const glm::vec3 tip = place(u, profile(1.f, cu), h, R);
                foam(tip, inward(u) * P.rnd(0.01f, 0.03f) + glm::vec3(0, P.rnd(0.008f, 0.025f), 0), P.rnd(0.04f, 0.09f) * size, P.rnd(14, 24), 0.0012f); }
            // espuma no pé da onda, que vai rolando junto
            for (int k = 0; k < 4; k++) { const float u = P.rnd(-0.9f, 0.9f); float h, cu; local(u, H, c, h, cu); const glm::vec3 ft = place(u, face(0.05f, cu), h, R);
                foam(ft + glm::vec3(0, 0.04f, 0), inward(u) * P.rnd(0.008f, 0.02f), P.rnd(0.1f, 0.19f) * size, P.rnd(16, 26), 0.f); }
            // brilhos na água
            if (P.rnd(0.f, 1.f) < 0.6f) { const float u = P.rnd(-0.8f, 0.8f); float h, cu; local(u, H, c, h, cu); Particle q; q.kind = kGlow; q.p = place(u, profile(P.rnd(0.3f, 0.6f), cu), h, R) - inward(u) * 0.03f;
                q.v = glm::vec3(0, 0.004f, 0); q.size = P.rnd(0.025f, 0.045f); q.life = P.rnd(8, 14); q.c0 = glm::vec4(1.f); q.c1 = glm::vec4(0.7f, 0.95f, 1.f, 1.f); q.noFloor = true; P.emit(q); }
        }
        if (!crashed && t >= kCrash) {
            // a quebra: anel de espuma em volta do rival, respingos para cima e para fora
            crashed = true;
            for (int k = 0; k < 46; k++) { const float a = P.rnd(0.f, 6.2832f), r = P.rnd(0.3f, 1.f) * size; const glm::vec3 o(std::cos(a), 0.f, std::sin(a));
                foam(to + o * r + glm::vec3(0, P.rnd(0.04f, 0.45f) * size, 0), o * P.rnd(0.004f, 0.02f) + glm::vec3(0, P.rnd(0.f, 0.015f), 0), P.rnd(0.16f, 0.32f) * size, P.rnd(30, 52), 0.0003f); }
            for (int k = 0; k < 34; k++) { const float a = P.rnd(0.f, 6.2832f); const glm::vec3 o(std::cos(a), 0.f, std::sin(a)); Particle q; q.kind = kGlow;   // gotas: pingos claros acesos
                q.p = to + o * P.rnd(0.2f, 0.6f) * size + glm::vec3(0, P.rnd(0.3f, 1.1f) * size, 0); q.v = o * P.rnd(0.015f, 0.04f) + glm::vec3(0, P.rnd(0.03f, 0.075f), 0); q.size = P.rnd(0.03f, 0.06f) * size; q.life = P.rnd(26, 40); q.gravity = 0.003f; q.drag = 0.98f;
                q.c0 = glm::vec4(0.92f, 0.98f, 1.f, 1.f); q.c1 = glm::vec4(0.6f, 0.86f, 0.96f, 1.f); P.emit(q); }
        }
        if (crashed && P.rnd(0.f, 1.f) < 0.7f) { const float a = P.rnd(0.f, 6.2832f), r = P.rnd(0.4f, 1.1f) * size; const glm::vec3 o(std::cos(a), 0.f, std::sin(a));   // a onda desmancha em espuma
            foam(to + o * r + glm::vec3(0, 0.05f, 0), o * P.rnd(0.003f, 0.012f), P.rnd(0.12f, 0.24f) * size, P.rnd(24, 40), 0.f); }
    }
};
}
