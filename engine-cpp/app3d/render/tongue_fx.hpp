// app3d/render/tongue_fx.hpp: o Golpe de Língua do King Frog feito na hora, preso à cabeça.
// A boca aberta é uma expressão pintada na própria textura do rosto (some a boca fechada e acompanha a cabeça no mesmo quadro);
// a língua é um tubo rosa de bolas encadeadas que sai do ponto da boca na malha e vai até o peito do rival, com a ponta mais grossa.
// O motor acerta no quadro 10 dos 24 do golpe: a língua chega lá nesse quadro, segura um pouco e volta.
#pragma once
#include "particles.hpp"
#include "fighter.hpp"
#include "skills.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>
#include <glm/glm.hpp>
namespace KoRama {
struct TongueFx {
    // tempo da língua em quadros: enquanto o golpe dura é o do motor (acerta no quadro 10, e o hitstop congela junto);
    // depois o recolhimento continua sozinho até engolir
    static constexpr int kHit = 10, kStick = 16, kBack = 36, kEnd = 42;
    static constexpr glm::vec2 kMouthUV{0.5f, 0.53f};   // meio da boca aberta na textura do rosto (peça 0)
    static bool active(const Fighter& f) { return f.charId == 1 && f.state == FIGHTER_SKILL && f.activeSkill == SkillId::FrogTongue; }
    float t = -1.f; float s = 1.f; bool was = false;
    void reset() { t = -1; was = false; }
    void tick(const Fighter& f) {   // uma vez por quadro do motor
        const bool in = active(f);
        if (in && !was) { t = 0; s = f.isLeftPunch ? -1.f : 1.f; }   // o sentido alterna com a mão que o motor escolhe
        else if (in) t = std::max(t, skillElapsed(f));
        else if (t >= 0) { t += 1.f / skillStretch(f.charId); if (t > kEnd) t = -1; }
        was = in;
    }
    bool on() const { return t >= 0; }
    static float sat(float x) { return std::clamp(x, 0.f, 1.f); }
    static float smooth(float x) { x = sat(x); return x * x * (3.f - 2.f * x); }
    // quanto a língua está para fora (0..1)
    float ext() const {
        const float e = float(t); if (t < 0 || e < 2.f) return 0.f;
        if (e <= kHit) { const float a = (e - 2.f) / float(kHit - 2); return 1.f - (1.f - a) * (1.f - a); }   // sai acelerada e chega cheia
        if (e <= kStick) return 1.f;
        const float a = sat((e - kStick) / float(kBack - kStick)); return 1.f - a * a * (3.f - 2.f * a) * (0.55f + 0.45f * a);   // recolhe devagar e engole no fim
    }
    // ângulo da ponta em volta do eixo vertical, em relação à linha até o rival: vem de lado e chega reta no acerto
    float ang() const { const float e = float(t), a0 = -s * 1.05f; if (e <= 2.f) return a0; if (e >= kHit) return 0.f; return a0 * (1.f - smooth((e - 2.f) / float(kHit - 2))); }
    // giro do tronco: arma para o lado contrário, joga a língua com o corpo e volta
    float twist() const {
        const float e = float(t); if (t < 0) return 0.f; const float wind = s * 0.5f;
        if (e <= 3.f) return wind * smooth(e / 3.f);
        if (e <= kHit + 2) { const float a = smooth((e - 3.f) / float(kHit - 1)); return wind + (-s * 0.32f - wind) * a; }   // passa um pouco do rival
        return -s * 0.32f * (1.f - smooth((e - kHit - 2) / 14.f));
    }
    // boca: 0 fechada, 1 meio aberta, 2 aberta (engole no fim)
    int mouth() const { if (t < 0) return 0; return t < 1 ? 1 : t <= kBack ? 2 : t <= kBack + 3 ? 1 : 0; }
    bool tongueOut() const { return on() && ext() > 0.01f; }

    // pinta a boca aberta por cima da fechada (a textura tem o rosto de cabeça para baixo: o queixo fica para cima na imagem)
    // boca em "O" (elipse em pé) para cuspir: a boca fechada some sob o contorno, miolo escuro e um brilho de baba em baixo
    static void paintMouthO(unsigned w, unsigned h, std::uint8_t* px) {
        const float sx = float(w) / 1024.f, sy = float(h) / 1024.f, cx = 512.f * sx, cy = 572.f * sy, rx = 175.f * sx, ry = 105.f * sy, ink = 13.f * sx;
        // tampa a linha da boca fechada (de ponta a ponta) com a cor da pele logo acima dela
        const std::uint8_t* sk = px + (size_t(680.f * sy) * w + size_t(cx)) * 4; const std::uint8_t skin[3] = {sk[0], sk[1], sk[2]};
        for (int y = int(580.f * sy); y <= int(625.f * sy); y++) for (int x = int(180.f * sx); x <= int(850.f * sx); x++) { std::uint8_t* p = px + (size_t(y) * w + size_t(x)) * 4; p[0] = skin[0]; p[1] = skin[1]; p[2] = skin[2]; }
        for (int y = int(cy - ry - ink); y <= int(cy + ry + ink); y++) for (int x = int(cx - rx - ink); x <= int(cx + rx + ink); x++) { if (x < 0 || y < 0 || x >= int(w) || y >= int(h)) continue;
            const float u = (float(x) - cx) / rx, v = (float(y) - cy) / ry, d = std::sqrt(u * u + v * v); std::uint8_t* p = px + (size_t(y) * w + size_t(x)) * 4;
            if (d < 1.f - ink / rx) { const bool shine = (v < -0.45f && std::fabs(u) < 0.4f && d > 0.6f); p[0] = shine ? 150 : 60; p[1] = shine ? 60 : 14; p[2] = shine ? 70 : 22; } else if (d < 1.f + ink / rx) { p[0] = 16; p[1] = 22; p[2] = 12; } }
    }
    static void paintMouth(unsigned w, unsigned h, std::uint8_t* px, float open) {
        const float sx = float(w) / 1024.f, sy = float(h) / 1024.f, cx = 512.f * sx, lip = 624.f * sy, rx = 345.f * sx, depth = (44.f + 150.f * open) * sy, ink = 11.f * sx;
        const auto put = [&](int x, int y, std::uint8_t r, std::uint8_t g, std::uint8_t b) { if (x < 0 || y < 0 || x >= int(w) || y >= int(h)) return; std::uint8_t* p = px + (size_t(y) * w + size_t(x)) * 4; p[0] = r; p[1] = g; p[2] = b; p[3] = 255; };
        for (int x = int(cx - rx - ink); x <= int(cx + rx + ink); x++) {
            const float u = (float(x) - cx) / rx; if (std::fabs(u) > 1.f + ink / rx) continue; const float uu = std::min(1.f, std::fabs(u));
            const float top = lip - 10.f * sy * uu * uu, bot = top - depth * std::sqrt(std::max(0.f, 1.f - uu * uu * uu * uu)) - 4.f * sy;
            for (int y = int(bot - ink); y <= int(top + ink); y++) {
                const float fy = float(y); const bool inside = std::fabs(u) <= 1.f && fy >= bot && fy <= top;
                const float edge = std::min({fy - bot, top - fy, (1.f - std::fabs(u)) * rx});
                if (!inside || edge < ink) { put(x, y, 16, 22, 12); continue; }   // contorno de tinta
                // fundo da boca escuro; a base da língua rosa embaixo (perto do queixo) com sombra no meio
                const float tu = u / 0.62f, tv = (fy - bot) / std::max(1.f, (top - bot) * 0.55f);
                if (tu * tu + (tv - 0.35f) * (tv - 0.35f) / 0.5f < 1.f) { const bool mid = std::fabs(u) < 0.035f && tv > 0.15f; put(x, y, mid ? 190 : 236, mid ? 70 : 108, mid ? 98 : 138); }
                else put(x, y, 78, 18, 30);
            }
        }
    }

    // a língua em bolas que vivem um quadro: da boca (pos no mundo) ao alvo
    // chicote: na ida a ponta vem atrasada e o meio da língua vai na frente (arco côncavo no sentido do movimento);
    // no acerto ela gruda e vibra; na volta afrouxa e cai um pouco
    void build(const glm::vec3& mouthAt, const glm::vec3& target, float size, std::vector<Particle>& out) const {
        const float e = ext(); if (e <= 0.01f) return; const float ft = float(t);
        const auto rotY = [](const glm::vec3& v, float a) { const float c = std::cos(a), sn = std::sin(a); return glm::vec3(c * v.x + sn * v.z, v.y, -sn * v.x + c * v.z); };
        const glm::vec3 d = target - mouthAt, dh(d.x, 0.f, d.z); const float A = ang();
        const glm::vec3 tip = mouthAt + (rotY(dh, A) + glm::vec3(0, d.y, 0)) * e; const float L = glm::length(tip - mouthAt);
        const float lead = ft < kHit ? -A * 0.55f : 0.f;   // o meio vai à frente da ponta, para o lado do movimento
        const float sag = ft > kStick ? 0.1f * L * sat((ft - kStick) / 6.f) : 0.f;
        const glm::vec3 ctrl = mouthAt + rotY(dh, A + lead) * (0.5f * e) + glm::vec3(0, d.y * 0.5f * e + 0.05f * L - sag, 0);
        const glm::vec3 side = glm::normalize(glm::cross(glm::normalize(tip - mouthAt + glm::vec3(1e-5f)), glm::vec3(0, 1, 0)) + glm::vec3(1e-5f));
        const float wob = ft >= kHit ? std::sin((ft - kHit) * 1.9f) * 0.09f * L * std::exp(-(ft - kHit) * 0.22f) : 0.f;   // vibra ao grudar
        const auto at = [&](float u) { const float a = 1.f - u; return a * a * mouthAt + 2.f * a * u * ctrl + u * u * tip + side * (wob * std::sin(u * 3.14159f)); };
        const glm::vec4 pink(1.f, 0.45f, 0.56f, 1.f), deep(0.86f, 0.28f, 0.4f, 1.f);
        const auto ball = [&](const glm::vec3& p, float r, const glm::vec4& c) { Particle q; q.kind = kPuff; q.p = p; q.size = r; q.hold = true; q.noFloor = true; q.age = 4.f; q.life = 5.f; q.drag = 1.f; q.c0 = c; q.c1 = c; out.push_back(q); };
        const float r0 = 0.06f * size, r1 = 0.045f * size; float u = 0.f;
        while (u < 1.f) { const float r = r0 + (r1 - r0) * u; ball(at(u), r, pink); u += std::max(0.01f, r * 0.55f / std::max(0.05f, L)); }
        const float squash = ft >= kHit && ft < kHit + 6 ? 1.f + 0.35f * std::sin((ft - kHit) / 6.f * 3.14159f) : 1.f;   // a ponta esmaga no rival
        ball(tip, 0.1f * size * squash, deep);
    }
};
}
