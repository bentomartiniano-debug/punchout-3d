// app3d/render/leap_fx.hpp: os saltos (Salto Real do King Frog, Gran Salto da Lady Espinha) com preparo de verdade.
// O motor sobe o corpo já no primeiro quadro; aqui o desenho refaz a altura pelo tempo do golpe: agacha com esforço (tremendo, achatado),
// dispara esticado, paira, cai acelerando e toca o chão no quadro do acerto, amassando. Rachadura e poeira só nesse toque.
// Só o visual: o motor continua decidindo dano, invulnerabilidade e tempo.
#pragma once
#include "particles.hpp"
#include "fighter.hpp"
#include "skills.hpp"
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
namespace KoRama {
struct LeapFx {
    bool landed = false; SkillId skill = SkillId::None; float lastE = -1.f;
    // quadro (desde o início do golpe) em que toca o chão: um antes do acerto do motor
    static int landFrame(SkillId id) { return id == SkillId::FrogRoyalLeap ? 27 : 25; }
    static float peak(SkillId id) { return id == SkillId::FrogRoyalLeap ? 220.f : 200.f; }
    static bool isLeap(const Fighter& f) { if (f.state != FIGHTER_SKILL || f.activeSkill == SkillId::None) return false; const SkillDef* d = SkillSystem::get(f.activeSkill); return d && d->fx.airborne; }
    static constexpr int kPrep = 10; static constexpr float kCrouch = 30.f;
    static float sat(float x) { return std::clamp(x, 0.f, 1.f); }

    // altura (unidades do motor) e deformação do corpo nesse quadro
    struct Shape { float y = 0.f; glm::vec3 scale{1.f}; float gloveY = 0.f, gloveZ = 0.f; };
    static Shape shape(SkillId id, float e) {
        const int L = landFrame(id), F = L - 5; const float H = peak(id); Shape s;
        if (e < kPrep) {   // agacha: desce rápido e segura tremendo de esforço, cada vez mais achatado
            const float a = sat(float(e) / float(kPrep - 3)), k = 1.f - (1.f - a) * (1.f - a), hold = sat(float(e - (kPrep - 4)) / 4.f);
            s.y = -kCrouch * k + std::sin(float(e) * 2.9f) * 2.2f * hold;
            s.scale = glm::vec3(1.f + 0.13f * k + 0.03f * hold, 1.f - 0.15f * k - 0.03f * hold, 1.f + 0.13f * k + 0.03f * hold);
            s.gloveY = -18.f * k - 40.f; s.gloveZ = -14.f * k;   // as luvas descem junto (o motor sobe 40 no golpe todo)
        } else if (e < kPrep + 6) {   // dispara: sai do chão de uma vez, esticado
            const float a = float(e - kPrep) / 6.f, k = 1.f - std::pow(1.f - a, 2.6f);
            s.y = -kCrouch + (H + kCrouch) * k; const float st = 0.2f * (1.f - a);
            s.scale = glm::vec3(1.f - st * 0.5f, 1.f + st, 1.f - st * 0.5f); s.gloveY = 10.f * (1.f - a);
        } else if (e < F) {   // paira
            s.y = H + std::sin(float(e - kPrep - 6) * 0.45f) * 4.f; s.gloveY = 6.f;
        } else if (e < L) {   // cai acelerando, esticado, luvas para cima para esmagar
            const float a = float(e - F) / float(L - F); s.y = H * (1.f - a * a); const float st = 0.14f * a;
            s.scale = glm::vec3(1.f - st * 0.5f, 1.f + st, 1.f - st * 0.5f); s.gloveY = 24.f * a;
        } else {   // tocou: amassa e volta
            const float a = sat(float(e - L) / 7.f), k = (1.f - a) * (1.f - a);
            s.y = -kCrouch * 0.85f * k; s.scale = glm::vec3(1.f + 0.22f * k, 1.f - 0.22f * k, 1.f + 0.22f * k); s.gloveY = -30.f * k;
        }
        return s;
    }

    // aplica no Fighter que vai para o rig; devolve se é um salto (o chamador multiplica a deformação na pose)
    bool apply(Fighter& view, const Fighter& f, Shape& out) {
        if (!isLeap(f)) { skill = SkillId::None; landed = false; lastE = -1; return false; }
        if (f.activeSkill != skill) { skill = f.activeSkill; landed = false; lastE = -1; }
        const float e = skillElapsed(f); out = shape(skill, e);
        view.offsetY = out.y; view.gloveL.y += out.gloveY; view.gloveR.y += out.gloveY; view.gloveL.z += out.gloveZ; view.gloveR.z += out.gloveZ;
        return true;
    }

    // efeitos: na hora do toque, rachadura em volta dos pés, poeira que corre pelo chão e lascas
    void tick(const Fighter& f, const glm::vec3& feet, float size, const glm::vec3& tint, Particles& P) {
        if (!isLeap(f) || f.activeSkill != skill) return;
        const float e = skillElapsed(f); if (e == lastE) return; lastE = e;
        if (!landed && e >= landFrame(skill)) { landed = true; land(glm::vec3(feet.x, 0.f, feet.z), size, tint, P); }
    }
    static void land(const glm::vec3& at, float s, const glm::vec3& tint, Particles& P) {
        // rachadura: raios quebrados em zigue-zague saindo do centro, com galhos
        const glm::vec3 ink(0.1f, 0.07f, 0.06f), dark(0.22f, 0.15f, 0.11f);
        const auto seg = [&](const glm::vec3& a, const glm::vec3& b, float w) { const glm::vec3 d = b - a; const float L = glm::length(d); if (L < 1e-3f) return;
            Particle q; q.kind = kLine; q.p = (a + b) * 0.5f; q.p.y = Particles::kFloor + 0.004f; q.dir = d / L; q.len = L + w * 0.6f; q.size = w; q.hold = true; q.noFloor = true; q.drag = 1.f; q.life = P.rnd(150, 190); q.c0 = glm::vec4(ink, 1.f); q.c1 = glm::vec4(dark, 1.f); P.emit(q); };
        const int rays = 11;
        for (int r = 0; r < rays; r++) {
            float a = (float(r) + P.rnd(-0.3f, 0.3f)) / float(rays) * 6.2832f; glm::vec3 p = at + glm::vec3(std::cos(a), 0, std::sin(a)) * 0.08f * s;
            const float reach = P.rnd(1.1f, 1.9f) * s; const int n = 5 + int(P.rnd(0.f, 2.99f)); float w = 0.2f * s;
            for (int i = 0; i < n; i++) { a += P.rnd(-0.5f, 0.5f); const glm::vec3 q = p + glm::vec3(std::cos(a), 0, std::sin(a)) * (reach / float(n)); seg(p, q, w);
                if (i >= 1 && P.rnd(0.f, 1.f) < 0.45f) { const float b = a + (P.rnd(0.f, 1.f) < 0.5f ? 1.f : -1.f) * P.rnd(0.5f, 0.9f); seg(q, q + glm::vec3(std::cos(b), 0, std::sin(b)) * reach * P.rnd(0.15f, 0.3f), w * 0.6f); }
                p = q; w *= 0.8f; }
        }
        // anel de rachaduras curtas em volta do centro (o chão afundou)
        for (int i = 0; i < 12; i++) { const float a0 = float(i) / 12.f * 6.2832f, a1 = a0 + 6.2832f / 12.f * P.rnd(0.6f, 0.95f), r = 0.55f * s * P.rnd(0.85f, 1.15f);
            seg(at + glm::vec3(std::cos(a0), 0, std::sin(a0)) * r, at + glm::vec3(std::cos(a1), 0, std::sin(a1)) * r * P.rnd(0.9f, 1.1f), 0.11f * s); }
        // poeira: um anel que corre rente ao chão e cresce, e uma nuvem baixa que sobe devagar
        const glm::vec3 d0(0.95f, 0.91f, 0.83f), d1(0.8f, 0.74f, 0.66f);
        for (int i = 0; i < 34; i++) { const float a = float(i) / 34.f * 6.2832f + P.rnd(-0.08f, 0.08f); const glm::vec3 r(std::cos(a), 0, std::sin(a));
            Particle q; q.kind = kPuff; q.p = at + r * 0.35f * s + glm::vec3(0, P.rnd(0.05f, 0.14f), 0); q.v = r * P.rnd(0.05f, 0.085f) * s + glm::vec3(0, P.rnd(0.002f, 0.008f), 0); q.size = P.rnd(0.12f, 0.22f) * s; q.grow = 0.004f; q.drag = 0.9f; q.gravity = -0.0001f; q.life = P.rnd(34, 54); q.c0 = glm::vec4(d0, 1.f); q.c1 = glm::vec4(d1, 1.f); P.emit(q); }
        for (int i = 0; i < 14; i++) { const float a = P.rnd(0.f, 6.2832f), rr = P.rnd(0.1f, 0.5f) * s; const glm::vec3 r(std::cos(a), 0, std::sin(a));
            Particle q; q.kind = kPuff; q.p = at + r * rr + glm::vec3(0, P.rnd(0.1f, 0.35f), 0); q.v = r * P.rnd(0.008f, 0.02f) + glm::vec3(0, P.rnd(0.01f, 0.02f), 0); q.size = P.rnd(0.18f, 0.3f) * s; q.grow = 0.003f; q.drag = 0.95f; q.gravity = -0.00005f; q.life = P.rnd(40, 64); q.c0 = glm::vec4(glm::mix(d0, tint, 0.15f), 1.f); q.c1 = glm::vec4(d1, 1.f); P.emit(q); }
        // lascas do chão pulando e riscos de choque saindo rente
        for (int i = 0; i < 18; i++) { const float a = P.rnd(0.f, 6.2832f); const glm::vec3 r(std::cos(a), 0, std::sin(a));
            Particle q; q.kind = kFlake; q.p = at + r * 0.25f * s + glm::vec3(0, 0.05f, 0); q.v = r * P.rnd(0.02f, 0.05f) + glm::vec3(0, P.rnd(0.05f, 0.1f), 0); q.size = P.rnd(0.03f, 0.06f) * s; q.gravity = 0.004f; q.drag = 0.98f; q.life = P.rnd(34, 50); q.c0 = glm::vec4(dark * 1.6f, 1.f); q.c1 = glm::vec4(dark, 1.f); P.emit(q); }
        for (int i = 0; i < 22; i++) { const float a = float(i) / 22.f * 6.2832f; const glm::vec3 r(std::cos(a), 0, std::sin(a));
            Particle q; q.kind = kLine; q.p = at + r * 0.3f * s + glm::vec3(0, 0.06f, 0); q.v = r * P.rnd(0.1f, 0.15f) * s; q.size = P.rnd(0.04f, 0.06f); q.drag = 0.88f; q.noFloor = true; q.life = P.rnd(10, 15); q.c0 = glm::vec4(1.f, 0.98f, 0.9f, 1.f); q.c1 = glm::vec4(tint, 0.f); P.emit(q); }
    }
};
}
