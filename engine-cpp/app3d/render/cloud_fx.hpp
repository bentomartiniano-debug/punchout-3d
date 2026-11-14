// app3d/render/cloud_fx.hpp: os golpes de nuvem — a nuvem sai de quem lança, envolve o rival e fica nele enquanto dura o efeito, dissipando
// devagar; quem está atrás dela (vista da câmera) vira silhueta escura com hachura (o Renderer recebe a esfera, a densidade e a cor: setCloud).
//  - Explosão de Pólen (Jack): as mãos giram em rodas vazando grãos e soltam uma cortina dourada (nuvens claras, grãos acesos boiando).
//  - Erupção de Cinzas (Valéria): o braço varre de um lado ao outro jogando um rastro de cinza com brasas; nuvem cinza com brasas e fagulhas.
//  - Viento del Desierto (Espinha): rodopia num redemoinho de areia que anda até o rival e vira nuvem de areia.
//  - Fumaça de Exaustão (Marcov): agacha, as turbinas das costas cospem fogo e o escapamento solta fumaça preta que rola até o rival.
// Tempo em quadros de 60 Hz; o do motor enquanto o golpe dura, depois anda sozinho.
#pragma once
#include "particles.hpp"
#include "fighter.hpp"
#include "skills.hpp"
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
namespace KoRama {
struct CloudFx {
    enum Style { kNone = -1, kPollen = 0, kAsh, kExhaust, kSand };
    struct Look { SkillId id; uint32_t charId; int release, end; glm::vec3 light, mid, dark, spark, sil; float rise; };
    static const Look& look(int s) {
        static const Look k[4] = {
            {SkillId::JackCombBurst, 0, 9, 170, {1.f, 0.95f, 0.72f}, {1.f, 0.82f, 0.28f}, {0.96f, 0.62f, 0.12f}, {1.f, 0.9f, 0.5f}, {0.2f, 0.13f, 0.04f}, 0.00006f},
            {SkillId::ValeriaAshCloud, 4, 9, 220, {0.62f, 0.6f, 0.58f}, {0.44f, 0.42f, 0.41f}, {0.26f, 0.25f, 0.25f}, {1.f, 0.45f, 0.08f}, {0.09f, 0.08f, 0.08f}, 0.00012f},
            {SkillId::MarcovExhaustCloud, 7, 11, 210, {0.42f, 0.42f, 0.45f}, {0.27f, 0.27f, 0.3f}, {0.13f, 0.13f, 0.15f}, {1.f, 0.55f, 0.12f}, {0.05f, 0.05f, 0.06f}, 0.0002f},
            {SkillId::EspinhaDesertWind, 6, 11, 170, {0.96f, 0.86f, 0.64f}, {0.85f, 0.69f, 0.44f}, {0.64f, 0.48f, 0.28f}, {1.f, 0.9f, 0.6f}, {0.24f, 0.16f, 0.07f}, 0.00003f} };
        return k[std::clamp(s, 0, 3)];
    }
    static int styleOf(const Fighter& f) { if (f.state != FIGHTER_SKILL) return kNone; for (int s = 0; s < 4; s++) if (f.activeSkill == look(s).id && f.charId == look(s).charId) return s; return kNone; }
    static bool active(const Fighter& f) { return styleOf(f) != kNone; }

    float t = -1.f; int style = kNone; bool was = false, burst = false; glm::vec3 at{0.f}, from{0.f}, fwd{0.f, 0.f, 1.f}; float size = 1.f;
    void reset() { t = -1; was = false; style = kNone; }
    bool on() const { return t >= 0; }
    static float sat(float x) { return std::clamp(x, 0.f, 1.f); }
    static float smooth(float x) { x = sat(x); return x * x * (3.f - 2.f * x); }
    const Look& lk() const { return look(style); }
    glm::vec3 silColor() const { return lk().sil; }

    // pólen: mãos girando em rodas na frente do peito, acelerando; na soltura empurram para a frente (os outros usam a pose do motor)
    static void hands(Fighter& view, const Fighter& f) {
        if (styleOf(f) != kPollen) return; const float e = skillElapsed(f); const int R = look(kPollen).release;
        if (e < R) { const float k = e / float(R), ang = 1.2f * e + 0.9f * e * k, r = 14.f + 6.f * k;
            view.gloveL.x = -22.f + std::cos(ang) * r * 0.5f; view.gloveL.y = 72.f + std::sin(ang) * r; view.gloveL.z = 26.f + std::cos(ang) * r;
            view.gloveR.x = 22.f - std::cos(ang + 3.14159f) * r * 0.5f; view.gloveR.y = 72.f + std::sin(ang + 3.14159f) * r; view.gloveR.z = 26.f + std::cos(ang + 3.14159f) * r;
        } else { const float k = smooth((e - R) / 5.f) * (1.f - smooth((e - 16.f) / 10.f));
            view.gloveL.x = -30.f; view.gloveR.x = 30.f; view.gloveL.y = view.gloveR.y = 74.f; view.gloveL.z = view.gloveR.z = 26.f + 70.f * k; }
    }
    // densidade (0..1), raio e centro da nuvem agora
    float density() const { if (t < 0 || t < lk().release) return 0.f; const float e = float(t - lk().release), life = float(lk().end - lk().release); return smooth(e / 7.f) * (1.f - smooth((e - life * 0.45f) / (life * 0.5f))); }
    float radius() const { if (t < 0 || t < lk().release) return 0.f; const float e = float(t - lk().release); return (0.55f + 0.45f * smooth(e / 12.f) + e * 0.0018f) * size * (style == kExhaust ? 1.15f : 1.f); }
    glm::vec3 center() const { if (t < 0 || t < lk().release) return from; const float e = float(t - lk().release), k = smooth(e / 9.f); return from + (at - from) * k + glm::vec3(0, e * lk().rise * 2.f, 0); }

    // hands: luvas de quem lança; body: peito dele; fwd: para o rival; target: peito do rival
    void tick(const Fighter& f, const glm::vec3& handL, const glm::vec3& handR, const glm::vec3& body, const glm::vec3& front, const glm::vec3& target, float sz, Particles& P) {
        const int st = styleOf(f); const bool in = st != kNone;
        if (in && (!was || st != style)) { t = 0; burst = false; size = sz; style = st; }
        else if (in) t = std::max(t, skillElapsed(f));
        else if (t >= 0) { t += 1.f / skillStretch(f.charId); if (t > lk().end) { t = -1; style = kNone; } }
        was = in; if (t < 0) return;
        const Look& L = lk(); const glm::vec3 side = glm::normalize(glm::cross(glm::vec3(0, 1, 0), front) + glm::vec3(1e-5f));
        const auto mote = [&](const glm::vec3& p, const glm::vec3& v, float s, float life, const glm::vec3& c0, const glm::vec3& c1, int kind = kGlow) { Particle q; q.kind = kind; q.p = p; q.v = v; q.size = s; q.life = life; q.drag = 0.96f; q.jitter = 0.0005f; q.noFloor = true;
            q.gravity = kind == kFire ? -0.0004f : -0.00003f; q.c0 = glm::vec4(c0, 1.f); q.c1 = glm::vec4(c1, 1.f); P.emit(q); };
        const auto puff = [&](const glm::vec3& p, const glm::vec3& v, float s, float life, float grow) { Particle q; q.kind = kPuff; q.p = p; q.v = v; q.size = s; q.life = life; q.grow = grow; q.drag = 0.94f; q.jitter = 0.00025f; q.gravity = -L.rise; q.noFloor = true;
            const float w = P.rnd(0.f, 1.f); q.c0 = glm::vec4(glm::mix(L.light, L.mid, w), 1.f); q.c1 = glm::vec4(glm::mix(L.mid, L.dark, w), 1.f); P.emit(q); };
        const auto spark = [&](const glm::vec3& p, const glm::vec3& v) { mote(p, v, P.rnd(0.01f, 0.02f) * size, P.rnd(20, 40), glm::vec3(1.f, 0.85f, 0.4f), L.spark); };
        if (t < L.release) {   // antes de soltar
            if (in) { fwd = front; at = target; }
            if (style == kPollen) { from = (handL + handR) * 0.5f + front * 0.15f; for (const glm::vec3& h : {handL, handR}) for (int k = 0; k < 2; k++) mote(h + P.dir() * 0.06f, P.dir() * 0.006f + fwd * 0.004f, P.rnd(0.012f, 0.022f) * size, P.rnd(18, 30), L.light, L.dark); }
            else if (style == kAsh) { from = handR + front * 0.1f;   // o braço varrendo: rastro de cinza e brasas saindo da mão
                for (int k = 0; k < 3; k++) puff(handR + P.dir() * 0.05f, fwd * P.rnd(0.01f, 0.03f) + P.dir() * 0.006f, P.rnd(0.035f, 0.07f) * size, P.rnd(25, 45), 0.0015f);
                for (int k = 0; k < 2; k++) spark(handR + P.dir() * 0.05f, fwd * P.rnd(0.02f, 0.05f) + P.dir() * 0.015f); }
            else if (style == kSand) { from = body - glm::vec3(0, 0.4f * size, 0); const glm::vec3 feet = body - glm::vec3(0, body.y, 0);   // rodopio: grãos de areia girando em volta dela, subindo
                for (int k = 0; k < 6; k++) { Particle q; q.kind = k % 3 ? kGlow : kPuff; q.p = feet; q.axis = feet; q.v = glm::vec3(0.f); q.orbitA = P.rnd(0.f, 6.2832f); q.orbitR = P.rnd(0.25f, 0.6f) * size; q.orbitV = P.rnd(0.22f, 0.34f); q.orbitY = P.rnd(0.05f, 1.6f) * size; q.drag = 1.f; q.noFloor = true;
                    q.size = k % 3 ? P.rnd(0.012f, 0.022f) : P.rnd(0.05f, 0.09f); q.life = P.rnd(16, 26); q.c0 = glm::vec4(L.light, 1.f); q.c1 = glm::vec4(L.dark, 1.f); P.emit(q); } }
            else { from = body - glm::vec3(0, 0.25f * size, 0) + front * 0.2f;   // agachado: turbinas das costas acesas e o escapamento começando a soltar
                const glm::vec3 back = body - front * 0.35f * size + glm::vec3(0, 0.15f * size, 0);
                for (int k = 0; k < 2; k++) { const glm::vec3 nz = back + side * (k ? 0.18f : -0.18f) * size; mote(nz, -front * P.rnd(0.03f, 0.06f) + glm::vec3(0, -0.01f, 0), P.rnd(0.05f, 0.09f) * size, P.rnd(10, 16), glm::vec3(1.f, 0.85f, 0.4f), L.spark, kFire); }
                if (t > 3) for (int k = 0; k < 2; k++) puff(from + P.dir() * 0.08f, fwd * P.rnd(0.008f, 0.02f) + glm::vec3(0, 0.004f, 0), P.rnd(0.05f, 0.09f) * size, P.rnd(25, 40), 0.002f); }
            return; }
        if (!burst && style == kSand) { const glm::vec3 feet = from - glm::vec3(0, from.y, 0), tgt = at - glm::vec3(0, at.y, 0);   // o redemoinho anda até o rival
            for (int k = 0; k < 90; k++) { Particle q; q.kind = k % 3 ? kGlow : kPuff; q.p = feet; q.axis = feet; q.v = (tgt - feet) / 9.f; q.orbitA = P.rnd(0.f, 6.2832f); q.orbitR = P.rnd(0.15f, 0.5f) * size; q.orbitG = 0.004f; q.orbitV = P.rnd(0.25f, 0.4f); q.orbitY = P.rnd(0.05f, 1.7f) * size;
                q.drag = 1.f; q.noFloor = true; q.size = k % 3 ? P.rnd(0.012f, 0.024f) : P.rnd(0.06f, 0.11f); q.life = P.rnd(9, 12); q.c0 = glm::vec4(L.light, 1.f); q.c1 = glm::vec4(L.mid, 1.f); P.emit(q); } }
        if (!burst) { burst = true;   // a soltura: a parede de nuvem sai na direção do rival e abre
            const int n = style == kPollen ? 34 : 44; const float sp = style == kExhaust ? 0.06f : 0.05f;
            for (int k = 0; k < n; k++) { const float u = P.rnd(-1.f, 1.f), v = P.rnd(-0.6f, 1.f);
                const glm::vec3 p = from + side * (u * 0.18f) + glm::vec3(0, v * 0.15f, 0); const glm::vec3 vel = fwd * P.rnd(sp * 0.6f, sp * 1.3f) + side * (u * P.rnd(0.01f, 0.03f)) + glm::vec3(0, v * 0.018f, 0);
                puff(p, vel, P.rnd(0.04f, 0.1f) * size * (style == kPollen ? 1.f : 1.3f), P.rnd(50, 90), P.rnd(0.0008f, 0.002f)); }
            const int motes = style == kPollen ? 220 : 90;
            for (int k = 0; k < motes; k++) { const glm::vec3 v = fwd * P.rnd(0.03f, 0.08f) + P.dir() * 0.025f;
                if (style == kPollen) mote(from + P.dir() * 0.12f, v, P.rnd(0.01f, 0.024f) * size, P.rnd(60, 140), L.light, L.dark); else spark(from + P.dir() * 0.12f, v); }
            for (int k = 0; k < 30; k++) { Particle q; q.kind = kFlake; q.p = from + P.dir() * 0.1f; q.v = fwd * P.rnd(0.03f, 0.07f) + P.dir() * 0.02f; q.size = P.rnd(0.012f, 0.022f) * size; q.life = P.rnd(60, 110); q.drag = 0.95f; q.jitter = 0.0004f; q.gravity = 0.0001f;
                q.c0 = glm::vec4(style == kPollen ? glm::vec3(1.f, 0.9f, 0.45f) : L.mid, 1.f); q.c1 = glm::vec4(L.dark, 1.f); P.emit(q); } }   // lascas: pólen brilhando, flocos de cinza, fuligem
        // a nuvem fica no rival e vai se desfazendo: repõe tufos na casca (dá para ver através) e grãos/brasas dentro, cada vez menos
        const float d = density(), R = radius(); const glm::vec3 c = center(); at = glm::mix(at, target, 0.05f);
        if (d > 0.05f) {
            if (P.rnd(0.f, 1.f) < (style == kPollen ? 0.45f : 0.6f) * d) { const glm::vec3 o = P.dir(); puff(c + o * (R * P.rnd(0.85f, 1.05f)), o * 0.003f, P.rnd(0.04f, 0.1f) * size * (style == kPollen ? 1.f : 1.25f) * (0.6f + 0.4f * d), P.rnd(40, 70), P.rnd(0.0006f, 0.0015f)); }
            const int n = int((style == kPollen ? 9.f : 3.f) * d) + 1;
            for (int k = 0; k < n; k++) { const glm::vec3 p = c + P.dir() * (R * P.rnd(0.f, 1.1f));
                if (style == kPollen) mote(p, P.dir() * 0.003f + glm::vec3(0, 0.0015f, 0), P.rnd(0.008f, 0.02f) * size, P.rnd(40, 80), L.light, L.dark);
                else if (P.rnd(0.f, 1.f) < 0.5f) spark(p, P.dir() * 0.004f + glm::vec3(0, 0.003f, 0)); } }
    }
};
}
