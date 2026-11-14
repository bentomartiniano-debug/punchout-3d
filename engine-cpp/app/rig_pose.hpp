// Esqueleto 2D: transformação afim e poses por estado, compartilhadas pelo Jack (jack_rig.cpp)
// e pelo resto do elenco (fighter_rig.cpp). Poses em unidades do quadro 120x120 do Jack.
#ifndef KORAMA_RIG_POSE_HPP
#define KORAMA_RIG_POSE_HPP

#include "ring_renderer.hpp"
#include "skills.hpp"
#include <algorithm>
#include <cmath>
#include <string>

namespace KoRama::rig {

constexpr float kPi = 3.14159265f;
constexpr float kUnits = 120.0f; // quadro do SVG original

// Transformação afim 2D: x' = a x + c y + e, y' = b x + d y + f.
struct Aff {
    float a = 1, b = 0, c = 0, d = 1, e = 0, f = 0;
    sf::Vector2f operator()(sf::Vector2f p) const { return {a * p.x + c * p.y + e, b * p.x + d * p.y + f}; }
};
inline Aff operator*(const Aff& m, const Aff& n) {
    return {m.a * n.a + m.c * n.b, m.b * n.a + m.d * n.b, m.a * n.c + m.c * n.d, m.b * n.c + m.d * n.d,
            m.a * n.e + m.c * n.f + m.e, m.b * n.e + m.d * n.f + m.f};
}
inline Aff move(float x, float y) { return {1, 0, 0, 1, x, y}; }
inline Aff turn(float r) { const float c = std::cos(r), s = std::sin(r); return {c, s, -s, c, 0, 0}; }
inline Aff scale(float x, float y) { return {x, 0, 0, y, 0, 0}; }
// Gira e escala em volta de um ponto de giro.
inline Aff around(float px, float py, float rot, float sx = 1, float sy = 1) { return move(px, py) * turn(rot) * scale(sx, sy) * move(-px, -py); }

struct Pose {
    bool back = false;
    int head = 0;            // 0 normal, 1 três quartos, 2 perfil
    bool headMirror = false; // cabeça virada para a esquerda
    bool mirrorAll = false;  // segunda metade do giro do nocaute
    float bodySx = 1;        // corpo achatado (perfil no giro)
    float torsoRot = 0, torsoSx = 1, torsoSy = 1, torsoDx = 0, torsoDy = 0;
    float headRot = 0, headDx = 0, headDy = 0, headSx = 1, headSy = 1;
    float legLRot = 0, legRRot = 0, legSy = 1;
    float legLDx = 0, legLDy = 0, legRDx = 0, legRDy = 0; // passada lateral
    const char* eyes = "normal";
    const char* mouth = "normal";
};

inline float bump(float p) { return std::sin(std::clamp(p, 0.0f, 1.0f) * kPi); }

inline Pose makePose(const Fighter& f, const RingRenderer::Reaction& rx, bool backView, uint32_t frame, bool celebrating, float bodyH, bool ringTurning,
                     float legLen = 31.0f) {
    Pose q;
    q.back = backView;
    const float t = frame / 60.0f;
    const float p = f.maxTimer > 0 ? std::clamp(1.0f - static_cast<float>(f.timer) / static_cast<float>(f.maxTimer), 0.0f, 1.0f) : 0.0f;
    const float side = f.isLeftPunch ? -1.0f : 1.0f;

    // Respiração e piscada
    q.torsoSy = 1 + 0.012f * std::sin(t * 3.0f);
    q.headDy = 0.5f * std::sin(t * 3.0f + 0.6f);
    if (frame % 200 < 7) q.eyes = "blink";
    if (f.isExhausted) { q.eyes = "tired"; q.mouth = "pant"; q.headDy += 1.5f; q.torsoSy -= 0.02f; }

    switch (f.state) {
    case FIGHTER_JAB: case FIGHTER_CROSS: case FIGHTER_HOOK: case FIGHTER_UPPER: case FIGHTER_BODY_BLOW: {
        const float k = bump(p);
        q.torsoRot = side * 0.09f * k;
        q.headRot = side * 0.06f * k;
        q.headDx = side * 1.2f * k;
        q.legLRot = -0.05f * k; q.legRRot = 0.05f * k;
        if (f.state == FIGHTER_UPPER) { q.torsoSy += 0.04f * k; q.headDy -= 2.0f * k; }
        if (f.state == FIGHTER_BODY_BLOW) { q.torsoSy -= 0.05f * k; q.headDy += 2.0f * k; }
        q.eyes = "angry"; q.mouth = "grit";
        break;
    }
    case FIGHTER_BLOCK:
        q.headDy += 2.5f; q.headSy = 0.95f; q.torsoSy -= 0.04f;
        q.legLRot = 0.04f * std::sin(t * 6.0f);
        q.legRRot = -0.04f * std::sin(t * 6.0f);
        q.legLDy = -1.8f * std::fabs(std::sin(t * 6.0f));
        q.legRDy = -1.8f * std::fabs(std::cos(t * 6.0f));
        q.eyes = "angry"; q.mouth = "grit";
        break;
    case FIGHTER_DODGE_L: case FIGHTER_DODGE_R: {
        const float dir = f.state == FIGHTER_DODGE_L ? -1.0f : 1.0f, k = bump(p);
        q.torsoRot = dir * 0.2f * k; q.headRot = dir * 0.3f * k; q.headDx = dir * 2.0f * k;
        q.legLRot = q.legRRot = dir * 0.12f * k;
        q.eyes = "angry";
        break;
    }
    case FIGHTER_DODGE_BACK:
        q.torsoSy -= 0.05f * bump(p); q.headDy += 1.5f * bump(p);
        q.eyes = "angry";
        break;
    case FIGHTER_DUCK: {
        // O motor abaixa o corpo inteiro; as pernas encolhem para os pés ficarem no chão.
        const float depth = -f.offsetY * kUnits / bodyH;
        q.legSy = std::max(0.3f, 1 - depth / legLen);
        q.headDy += 2; q.torsoSy -= 0.03f;
        q.eyes = "angry";
        break;
    }
    case FIGHTER_TELEGRAPH:
        q.torsoRot = 0.05f * std::sin(t * 9); q.headDy -= 1;
        q.eyes = "angry"; q.mouth = "grin";
        break;
    case FIGHTER_SKILL:
        q.eyes = "angry"; q.mouth = "grin";
        break;
    case FIGHTER_VICTORY_POSE:
        q.eyes = "happy"; q.mouth = "grin"; q.headRot = 0.1f * std::sin(t * 4);
        break;
    default:
        break;
    }
    if (celebrating || f.signatureOnly) { q.eyes = "happy"; q.mouth = "grin"; q.headRot = 0.12f * std::sin(t * 5); }

    // Passada lateral acompanhando o giro do ringue: o pé da frente abre de lado, o de trás vem
    // atrás e os dois deslizam de volta enquanto o ringue gira (sem cruzar as pernas).
    const bool stance = f.state == FIGHTER_IDLE || f.state == FIGHTER_TELEGRAPH || f.state == FIGHTER_BLOCK;
    if (ringTurning && stance && !rx.stars && rx.rope <= 0) {
        const float u = std::fmod(t / 0.9f, 1.0f);
        const float step = 9.0f, lift = 5.5f;
        const float dir = backView ? -1.0f : 1.0f; // o ringue gira para o mesmo lado da tela nas duas vistas
        float lead, leadLift = 0, trail = 0, trailLift = 0;
        if (u < 0.25f) { const float k = u / 0.25f; lead = step * k * k * (3 - 2 * k); leadLift = std::sin(k * kPi) * lift; }
        else if (u < 0.5f) lead = step;
        else lead = step * (1 - (u - 0.5f) / 0.5f);
        if (u >= 0.5f && u < 0.75f) { const float k = (u - 0.5f) / 0.25f; trail = step * 0.5f * k; trailLift = std::sin(k * kPi) * lift; }
        else if (u >= 0.75f) trail = step * 0.5f * (1 - (u - 0.75f) / 0.25f);
        // Pé da frente é o do lado do passo.
        float& leadDx = dir > 0 ? q.legRDx : q.legLDx;
        float& leadDy = dir > 0 ? q.legRDy : q.legLDy;
        float& trailDx = dir > 0 ? q.legLDx : q.legRDx;
        float& trailDy = dir > 0 ? q.legLDy : q.legRDy;
        leadDx = dir * lead; leadDy = -leadLift;
        trailDx = dir * trail; trailDy = -trailLift;
        q.torsoDx += dir * (lead + trail) * 0.25f;
        q.torsoDy -= (leadLift + trailLift) * 0.25f;
    }

    // Reações (golpe levado, tontura, cordas, giro do nocaute)
    const bool turned = rx.sprite.find("Turn") != std::string::npos;
    const bool up = rx.sprite.find("Up") != std::string::npos;
    if (f.state == FIGHTER_HIT || f.state == FIGHTER_KNOCKDOWN || rx.stars || rx.face) {
        q.headRot += rx.headRot * 0.9f;
        q.headDx += rx.headShiftX * kUnits * 0.5f;
        q.headDy += rx.headShiftY * kUnits;
    }
    if (turned) {
        q.head = 1; q.headMirror = rx.dir < 0;
        q.torsoRot -= rx.dir * 0.08f;
        q.eyes = "pain"; q.mouth = "pain";
    } else if (up) {
        q.headDy -= 4; q.headSy = 0.88f; q.headRot = rx.dir * 0.1f; q.torsoSy += 0.02f;
        q.eyes = "up"; q.mouth = "oh";
    } else if (rx.face == 1) {
        q.eyes = "pain"; q.mouth = "pain";
    } else if (rx.face == 2) {
        q.eyes = "pain"; q.mouth = "grit";
    }
    if (rx.clutch > 0) { q.torsoSy -= 0.08f * rx.clutch; q.headDy += 3 * rx.clutch; }
    if (rx.face == 3) {
        static const char* kSpiral[3] = {"spiral0", "spiral1", "spiral2"};
        q.eyes = kSpiral[(frame / 6) % 3]; q.mouth = "wavy";
    }
    if (rx.rope > 0) {
        q.legLRot = 0.18f * rx.rope; q.legRRot = -0.18f * rx.rope;
        q.torsoSy -= 0.04f * rx.rope;
    }
    // Squash & Stretch dinâmico de impacto com preservação de volume 2D
    if (rx.squash != 1.0f) {
        const float sq = std::clamp(rx.squash, 0.4f, 1.8f);
        q.torsoSy *= sq;
        q.torsoSx *= 1.0f / std::sqrt(sq);
    }
    // Deformação facial / mandíbula de impacto
    if (rx.headScaleX != 1.0f) {
        const float hs = std::clamp(rx.headScaleX, 0.4f, 1.8f);
        q.headSx *= hs;
        q.headSy *= (2.0f - hs); // estica na vertical quando achata na horizontal
    }
    if (rx.sprite.rfind("RxSpin_", 0) == 0) {
        const int k = rx.sprite.back() - '0';
        q.eyes = "x"; q.mouth = "wavy";
        q.mirrorAll = rx.mirror;
        q.headRot = 0; q.headDx = 0;
        switch (k) {
        case 0: q.back = false; break;
        case 1: q.back = false; q.head = 1; q.bodySx = 0.8f; break;
        case 2: q.back = false; q.head = 2; q.bodySx = 0.55f; break;
        case 3: q.back = true; q.head = 1; q.bodySx = 0.8f; break;
        default: q.back = true; break;
        }
    }
    return q;
}


} // namespace KoRama::rig

#endif
