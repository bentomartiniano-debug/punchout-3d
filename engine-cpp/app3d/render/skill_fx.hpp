// app3d/render/skill_fx.hpp: os efeitos dos golpes especiais em peças 3D com cel shading.
// Observa o lutador do motor (qual especial, o progresso, o quadro do acerto, os golpes rítmicos, os modos e os estados) e solta
// partículas no mesmo Particles do estouro do soco. Cada especial tem um estilo (projétil, golpe, onda no chão, nuvem, cura, aura...)
// e uma paleta; o desenho de cada um segue os efeitos do jogo 2D (app/vfx/lab_*.cpp), refeitos em volume.
#pragma once
#include "fighter.hpp"
#include "render/particles.hpp"
#include "skills.hpp"
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <vector>
namespace KoRama {

// Pontos do corpo no mundo (metros), tirados da pose desenhada
struct FxBody { glm::vec3 feet{0.f}, chest{0.f}, head{0.f}, glove[2]{}, fwd{0.f, 0.f, 1.f}; float size = 1.f; };

enum class FxStyle { None, Melee, Projectile, Spear, Tongue, Pull, Ground, Beam, Cloud, Swarm, Tornado, Heal, Mode, Wall, Leap, Rhythm, Rocket, Saw, Pulse, Missiles };
struct FxDef { FxStyle style = FxStyle::None; glm::vec3 a{1.f}, b{1.f}, c{1.f}; int kind = kPuff; };

inline glm::vec3 fxHex(uint32_t h) { return glm::vec3(float((h >> 16) & 255u), float((h >> 8) & 255u), float(h & 255u)) / 255.f; }

inline FxDef fxDefOf(SkillId id) {
    using S = FxStyle; using I = SkillId; const auto D = [](S s, uint32_t a, uint32_t b, uint32_t c, int k) { return FxDef{s, fxHex(a), fxHex(b), fxHex(c), k}; };
    switch (id) {
    // Jack: abelhas, mel e pólen
    case I::JackInfestation: return D(S::Swarm, 0xffd21f, 0x2b1d0e, 0xfff3a0, kGlow);
    case I::JackMolasses: return D(S::Mode, 0xf2a20c, 0xb3620a, 0xffd36b, kEnergy);
    case I::JackHoneyWall: return D(S::Wall, 0xf7b21b, 0xc77a06, 0xffe08a, kFlake);
    case I::JackStingJab: return D(S::Spear, 0xffd21f, 0x2b1d0e, 0xfff3a0, kGlow);
    case I::JackCombBurst: return D(S::Cloud, 0xffe45c, 0xf5b400, 0xfff6c2, kFlake);
    case I::JackRoyalJelly: return D(S::Heal, 0xfff1b8, 0xffc93c, 0xffffff, kGlow);
    // King Frog: língua, veneno, lama e água do brejo
    case I::FrogTongue: return D(S::Tongue, 0xff6f9c, 0xc23a6b, 0xffc2d6, kEnergy);
    case I::FrogPoisonSkin: return D(S::Mode, 0x9be33a, 0x7b2fbf, 0xd8ff8a, kGlow);
    case I::FrogRoyalLeap: return D(S::Leap, 0x7a5a2e, 0x4c7a2a, 0xffd34d, kPuff);
    case I::FrogMudSlime: return D(S::Projectile, 0x6b4a24, 0x3f2a12, 0x8b6a3a, kPuff);
    case I::FrogFlyCatcher: return D(S::Heal, 0x7ddc5a, 0x2d2d2d, 0xd8ff8a, kGlow);
    case I::FrogSwampSpout: return D(S::Ground, 0x5fd3c8, 0x2f8f6b, 0x6b4a24, kEnergy);
    // Joana: garra, harpão, cracas, coral e mar
    case I::JoanaClawCrush: return D(S::Melee, 0xff4b3e, 0xffffff, 0xff9a7a, kLine);
    case I::JoanaHookPull: return D(S::Pull, 0x9aa7b4, 0x4a5560, 0xd9e2ea, kLine);
    case I::JoanaCrustaceanShell: return D(S::Mode, 0xe9dcc2, 0x5bb8b0, 0xfff7e6, kFlake);
    case I::JoanaCoralSpout: return D(S::Ground, 0xff7a6b, 0xff4fa0, 0xffd0b8, kPuff);
    case I::JoanaAnchorSlam: return D(S::Ground, 0x3fa9f5, 0x1d5f9e, 0xd8f1ff, kEnergy);
    case I::JoanaOceanBounty: return D(S::Heal, 0x6be3d4, 0xff8fc8, 0xffffff, kGlow);
    // O Cremoso: calda quente, baunilha, creme, casquinha e cereja
    case I::CremosoSyrupBlast: return D(S::Projectile, 0x7a3b12, 0xd77a1d, 0xffc06b, kEnergy);
    case I::CremosoVanillaWhip: return D(S::Tornado, 0xfff4d6, 0xf3d9a4, 0xffffff, kPuff);
    case I::CremosoMeltMode: return D(S::Mode, 0xfff0d0, 0xf6c6d8, 0xffffff, kEnergy);
    case I::CremosoConePunch: return D(S::Melee, 0xd99a4e, 0x8a5222, 0xffe2b0, kFlake);
    case I::CremosoPuddleTrap: return D(S::Ground, 0xc9761f, 0x7a3b12, 0xffc06b, kEnergy);
    case I::CremosoCherryTopping: return D(S::Heal, 0xe3243b, 0xfff4d6, 0xffffff, kGlow);
    // Valéria: magma, lava, calor e cinza
    case I::ValeriaMagmaPunch: return D(S::Melee, 0xff6a14, 0xffd23f, 0x5a2a1a, kFire);
    case I::ValeriaVolcanicErupt: return D(S::Ground, 0xff6a14, 0x3a2a24, 0xffd23f, kFire);
    case I::ValeriaHeatwaveGuard: return D(S::Mode, 0xff8a1f, 0xffd23f, 0xff4b1f, kFire);
    case I::ValeriaLavaKickJab: return D(S::Projectile, 0xff5a12, 0xffc23a, 0x5a2a1a, kEnergy);
    case I::ValeriaAshCloud: return D(S::Cloud, 0x6b625e, 0x3a3532, 0xff6a14, kPuff);
    case I::ValeriaPhoenixEmber: return D(S::Heal, 0xffb23a, 0xff5a12, 0xfff1a0, kGlow);
    // Vivaldi: notas, inverno, estações, outono e primavera
    case I::VivaldiCadenceStaccato: return D(S::Rhythm, 0xb48cff, 0xffd84a, 0xffffff, kGlow);
    case I::VivaldiWinterAllegro: return D(S::Ground, 0xd8f1ff, 0x7cc4ff, 0xffffff, kFlake);
    case I::VivaldiFourSeasons: return D(S::Mode, 0xff9fc8, 0x8be36b, 0xffb23a, kFlake);
    case I::VivaldiCrescendoJab: return D(S::Projectile, 0xb48cff, 0xffd84a, 0xffffff, kGlow);
    case I::VivaldiAutumnRequiem: return D(S::Cloud, 0xe0702a, 0x8a4a1c, 0xffc23a, kFlake);
    case I::VivaldiSpringSonata: return D(S::Heal, 0xffa6cf, 0x9be36b, 0xffffff, kFlake);
    // Lady Espinha: cacto, espinhos, areia, estrelas de lucha e dálias
    case I::EspinhaNopalSlap: return D(S::Melee, 0x16a34a, 0xfef3c7, 0x14532d, kLine);
    case I::EspinhaLuchaFlyer: return D(S::Leap, 0xd6b36a, 0x92400e, 0xfacc15, kPuff);
    case I::EspinhaThornVeil: return D(S::Mode, 0xfef3c7, 0x16a34a, 0xe11d48, kLine);
    case I::EspinhaNeedleJab: return D(S::Spear, 0xfef3c7, 0x16a34a, 0xfacc15, kLine);
    case I::EspinhaDesertWind: return D(S::Tornado, 0xd6b36a, 0x92400e, 0xfde68a, kPuff);
    case I::EspinhaFridaTears: return D(S::Heal, 0xe11d48, 0xfacc15, 0xdb2777, kGlow);
    // Dr. Marcov: foguete, laser orbital, plasma, reação, fumaça e oxigênio
    case I::MarcovRocketThrust: return D(S::Rocket, 0xff7a1a, 0x6b6b70, 0xffd23f, kFire);
    case I::MarcovOrbitalLaser: return D(S::Beam, 0x4fe3ff, 0xffffff, 0x1a8cff, kGlow);
    case I::MarcovPlasmaBarrier: return D(S::Mode, 0x4fc3ff, 0xb8f0ff, 0x1a6cff, kGlow);
    case I::MarcovKineticOverdrive: return D(S::Rocket, 0x4fc3ff, 0x9aa7b4, 0xffffff, kGlow);
    case I::MarcovExhaustCloud: return D(S::Cloud, 0x4a4a52, 0x24242a, 0xff7a1a, kPuff);
    case I::MarcovOxygenRecharge: return D(S::Heal, 0x9fe8ff, 0xffffff, 0x4fc3ff, kGlow);
    // Batalhator: serras, reator, PEM, mísseis, holograma e o raio do fim
    case I::BossDualSaw: return D(S::Saw, 0xc8ced6, 0xff8a1f, 0xffffff, kLine);
    case I::BossOverclock: return D(S::Pulse, 0xff3b3b, 0xffb23a, 0xffffff, kGlow);
    case I::BossEmp: return D(S::Pulse, 0x4fe3ff, 0xffffff, 0x1a6cff, kGlow);
    case I::BossMissiles: return D(S::Missiles, 0xb0b6bf, 0xff7a1a, 0xffd23f, kPuff);
    case I::BossHolo: return D(S::Rhythm, 0x4fffe0, 0xff4fd8, 0xffffff, kGlow);
    case I::BossApocalypse: return D(S::Beam, 0xff3b3b, 0xffe0e0, 0xff8a1f, kGlow);
    default: return {};
    }
}

class SkillFx {
public:
    void reset() { t_[0] = Track{}; t_[1] = Track{}; }
    // who: 0 jogador, 1 oponente. Uma vez por quadro do motor, depois das poses
    // clipped: o especial tem efeito gravado do vinil (render/fx_clips.hpp); aqui ficam só os modos e estados
    // ownFx: o lutador tem os efeitos gravados do vinil; aí as auras dos modos daqui também saem (ficam só os estados de quem apanha)
    void tick(int who, const Fighter& f, const FxBody& me, const FxBody& foe, Particles& P, bool clipped = false, bool ownFx = false) {
        Track& k = t_[who]; k.frame++;
        const bool in = !clipped && f.state == FIGHTER_SKILL && f.activeSkill != SkillId::None;
        if (in && (k.state != FIGHTER_SKILL || k.skill != f.activeSkill)) { k.skill = f.activeSkill; k.def = fxDefOf(k.skill); k.event = f.skillEventDone; k.hits = f.skillHitsDone; k.launched = 0; k.prevGlove[0] = me.glove[0]; k.prevGlove[1] = me.glove[1]; start(k, me, foe, P); }
        if (in) { const float p = 1.f - float(f.timer) / float(std::max(1, f.maxTimer));
            during(k, f, p, me, foe, P);
            if (f.skillEventDone && !k.event) hit(k, me, foe, P);
            if (f.skillHitsDone > k.hits && k.def.style != FxStyle::Missiles) hit(k, me, foe, P); }
        flights(k, foe, P);
        if (k.beam > 0) { k.beam--; beamFrame(k, P); }
        k.state = f.state; k.event = f.skillEventDone; k.hits = f.skillHitsDone; k.prevGlove[0] = me.glove[0]; k.prevGlove[1] = me.glove[1];
        statuses(k, f, me, P, ownFx);
    }
private:
    struct Flight { glm::vec3 p{0.f}, v{0.f}; int left = 0; FxDef def; bool spear = false; };
    struct Track { SkillId skill = SkillId::None; FxDef def; int state = -1, hits = 0, frame = 0, launched = 0, beam = 0; bool event = false; glm::vec3 prevGlove[2]{}, beamAt{0.f}; std::vector<Flight> flights; };
    Track t_[2];

    static Particle mk(int kind, const glm::vec3& p, const glm::vec3& v, float size, float life, const glm::vec3& c0, const glm::vec3& c1) { Particle q; q.kind = kind; q.p = p; q.v = v; q.size = size; q.life = life; q.c0 = glm::vec4(c0, 1.f); q.c1 = glm::vec4(c1, 1.f); q.drag = 0.95f; return q; }
    // peça que vive só um quadro (língua, corrente, feixe): nasce já crescida
    static Particle now(Particle q) { q.hold = true; q.noFloor = true; q.age = 4.f; q.life = 5.f; q.drag = 1.f; return q; }
    static glm::vec3 side(const glm::vec3& f) { return glm::normalize(glm::cross(f, glm::vec3(0, 1, 0)) + glm::vec3(1e-5f)); }
    static int activeGlove(const FxBody& me) { return glm::dot(me.glove[0] - me.chest, me.fwd) >= glm::dot(me.glove[1] - me.chest, me.fwd) ? 0 : 1; }

    void line(Particles& P, const glm::vec3& a, const glm::vec3& b, float w, const glm::vec3& c) { const glm::vec3 d = b - a; const float L = glm::length(d); if (L < 1e-3f) return; Particle q = now(mk(kLine, (a + b) * 0.5f, glm::vec3(0.f), w, 5, c, c)); q.len = L; q.dir = d / L; P.emit(q); }
    void burst(Particles& P, const glm::vec3& at, const FxDef& d, float s) {
        for (int i = 0; i < int(16 * s); i++) { Particle q = mk(kLine, at, P.dir() * P.rnd(0.05f, 0.12f) * s, P.rnd(0.04f, 0.08f) * s, P.rnd(12, 22), d.a, d.b); q.drag = 0.9f; P.emit(q); }
        for (int i = 0; i < int(10 * s); i++) { Particle q = mk(d.kind == kLine || d.kind == kFlake ? kEnergy : d.kind, at + P.dir() * 0.08f, P.dir() * P.rnd(0.015f, 0.045f) * s, P.rnd(0.07f, 0.14f) * s, P.rnd(16, 28), d.a, d.c); q.gravity = d.kind == kFire ? -0.0008f : 0.0006f; q.grow = 0.002f; P.emit(q); }
        for (int i = 0; i < int(6 * s); i++) { Particle q = mk(kFlake, at, P.dir() * P.rnd(0.03f, 0.07f), P.rnd(0.025f, 0.045f), P.rnd(24, 40), d.c, d.b); q.gravity = 0.0025f; q.drag = 0.97f; P.emit(q); }
    }
    void floorRing(Particles& P, const glm::vec3& at, const FxDef& d, float s) {
        for (int i = 0; i < int(28 * s); i++) { const float a = float(i) / (28.f * s) * 6.2832f + P.rnd(-0.1f, 0.1f); const glm::vec3 r(std::cos(a), 0, std::sin(a)); Particle q = mk(kLine, glm::vec3(at.x, 0.05f, at.z) + r * 0.2f, r * P.rnd(0.07f, 0.11f) * s, P.rnd(0.05f, 0.08f), P.rnd(14, 20), d.a, d.b); q.drag = 0.9f; q.noFloor = true; P.emit(q); }
        for (int i = 0; i < int(12 * s); i++) { const float a = P.rnd(0.f, 6.2832f); const glm::vec3 r(std::cos(a), 0, std::sin(a)); Particle q = mk(kPuff, glm::vec3(at.x, 0.08f, at.z) + r * P.rnd(0.2f, 0.5f), r * P.rnd(0.01f, 0.03f) + glm::vec3(0, 0.008f, 0), P.rnd(0.1f, 0.2f) * s, P.rnd(28, 46), glm::mix(d.c, glm::vec3(0.94f, 0.9f, 0.82f), 0.5f), glm::vec3(0.86f, 0.82f, 0.76f)); q.grow = 0.002f; P.emit(q); }
    }
    void column(Particles& P, const glm::vec3& at, const FxDef& d, float s) {
        for (int i = 0; i < int(20 * s); i++) { Particle q = mk(d.kind == kLine ? kEnergy : d.kind, glm::vec3(at.x, 0.05f, at.z) + glm::vec3(P.rnd(-0.25f, 0.25f), 0, P.rnd(-0.25f, 0.25f)), glm::vec3(P.rnd(-0.01f, 0.01f), P.rnd(0.05f, 0.13f) * s, P.rnd(-0.01f, 0.01f)), P.rnd(0.1f, 0.2f) * s, P.rnd(28, 44), d.a, d.b); q.gravity = 0.003f; q.drag = 0.985f; P.emit(q); }
        for (int i = 0; i < int(8 * s); i++) { Particle q = mk(kFlake, glm::vec3(at.x, 0.1f, at.z), glm::vec3(P.rnd(-0.04f, 0.04f), P.rnd(0.06f, 0.12f), P.rnd(-0.04f, 0.04f)), P.rnd(0.04f, 0.07f), P.rnd(30, 46), d.c, d.b); q.gravity = 0.004f; q.drag = 0.98f; P.emit(q); }
    }
    void launch(Track& k, const glm::vec3& from, const glm::vec3& to, int frames, bool spear) { Flight fl; fl.p = from; fl.v = (to - from) / float(frames); fl.left = frames; fl.def = k.def; fl.spear = spear; k.flights.push_back(fl); }

    void start(Track& k, const FxBody& me, const FxBody& foe, Particles& P) {
        const FxDef& d = k.def; (void)foe;
        switch (d.style) {
        case FxStyle::Mode: case FxStyle::Pulse:   // ativação: uma casca que se abre em volta do corpo
            for (int i = 0; i < 26; i++) { Particle q = mk(d.kind == kLine ? kGlow : d.kind, me.chest + P.dir() * 0.15f * me.size, P.dir() * P.rnd(0.03f, 0.06f) * me.size, P.rnd(0.05f, 0.1f), P.rnd(18, 30), d.a, d.b); q.drag = 0.9f; q.noFloor = true; P.emit(q); }
            floorRing(P, me.feet, d, 0.7f); break;
        case FxStyle::Heal:
            for (int i = 0; i < 24; i++) { Particle q = mk(kGlow, me.feet, glm::vec3(0, P.rnd(0.004f, 0.01f), 0), P.rnd(0.06f, 0.09f), P.rnd(40, 60), d.a, d.b); q.axis = me.feet + glm::vec3(0, 0.05f, 0); q.orbitA = float(i) / 18.f * 6.2832f; q.orbitR = 0.55f * me.size; q.orbitV = 0.07f; q.drag = 1.f; q.noFloor = true; P.emit(q); }
            break;
        case FxStyle::Leap: for (int i = 0; i < 16; i++) { const float a = P.rnd(0.f, 6.2832f); const glm::vec3 r(std::cos(a), 0, std::sin(a)); Particle q = mk(kPuff, glm::vec3(me.feet.x, 0.08f, me.feet.z) + r * 0.3f, r * P.rnd(0.02f, 0.05f) + glm::vec3(0, 0.01f, 0), P.rnd(0.1f, 0.18f), P.rnd(26, 40), d.a, d.c); P.emit(q); } break;
        case FxStyle::Swarm:   // as abelhas saem do corpo e vão zumbir na cabeça do rival
            for (int i = 0; i < 18; i++) { Particle q = mk(kGlow, me.chest, glm::vec3(0.f), P.rnd(0.055f, 0.075f), P.rnd(120, 170), i % 3 ? d.a : d.b, i % 3 ? d.a : d.b); q.hold = true; q.noFloor = true; q.axis = me.chest + P.dir() * 0.2f; q.v = (foe.head - q.axis) / 14.f * P.rnd(0.85f, 1.15f); q.drag = 0.93f; q.orbitA = P.rnd(0.f, 6.2832f); q.orbitR = P.rnd(0.18f, 0.38f); q.orbitV = P.rnd(0.18f, 0.32f) * (i % 2 ? 1.f : -1.f); q.orbitY = P.rnd(-0.15f, 0.2f); q.jitter = 0.0008f; P.emit(q); }
            break;
        default: break;
        }
    }

    void during(Track& k, const Fighter& f, float p, const FxBody& me, const FxBody& foe, Particles& P) {
        const FxDef& d = k.def; const int g = activeGlove(me); const glm::vec3 glove = me.glove[g], gv = glove - k.prevGlove[g];
        switch (d.style) {
        case FxStyle::Melee: case FxStyle::Rhythm: case FxStyle::Saw:   // rastro da luva no golpe
            if (glm::length(gv) > 0.015f) { Particle q = mk(kLine, glove, gv * 0.4f, 0.05f, 10, d.a, d.b); q.drag = 0.85f; P.emit(q); if (k.frame % 2 == 0) { Particle b = mk(d.kind == kLine ? kGlow : d.kind, glove, gv * 0.15f + P.dir() * 0.004f, 0.06f, 14, d.a, d.c); P.emit(b); } }
            if (d.style == FxStyle::Saw && p > 0.2f && p < 0.85f) for (int i = 0; i < 3; i++) { Particle s = mk(kSpark, foe.chest - me.fwd * 0.2f, glm::vec3(P.rnd(-0.06f, 0.06f), P.rnd(0.02f, 0.08f), P.rnd(-0.06f, 0.06f)), P.rnd(0.016f, 0.026f), P.rnd(20, 36), d.b, d.b); s.gravity = 0.0035f; s.drag = 0.985f; P.emit(s); }
            break;
        case FxStyle::Projectile: case FxStyle::Spear:
            if (!k.launched && p < 0.3f && k.frame % 2 == 0) { const glm::vec3 o = P.dir() * 0.35f; Particle q = mk(kGlow, glove + o, -o / 10.f, 0.04f, 10, d.a, d.c); q.noFloor = true; P.emit(q); }   // junta energia na mão
            if (!k.launched && p >= 0.3f) { k.launched = 1; launch(k, glove, foe.chest, d.style == FxStyle::Spear ? 8 : 12, d.style == FxStyle::Spear); }
            break;
        case FxStyle::Missiles:
            if (p >= 0.2f && k.launched < 4 && k.frame % 6 == 0) { k.launched++; const glm::vec3 from = me.head + glm::vec3(0, 0.25f, 0) + side(me.fwd) * (k.launched % 2 ? 0.3f : -0.3f); launch(k, from, foe.chest + P.dir() * 0.15f, 16, false); }
            break;
        case FxStyle::Tongue: case FxStyle::Pull: {   // estica até o rival e volta
            const float ext = std::clamp(p < 0.45f ? p / 0.45f : 1.f - (p - 0.45f) / 0.35f, 0.f, 1.f); if (ext <= 0.01f) break;
            const glm::vec3 from = d.style == FxStyle::Tongue ? me.head - glm::vec3(0, 0.12f * me.size, 0) + me.fwd * 0.15f : glove, tip = from + (foe.chest - from) * ext;
            line(P, from, tip, d.style == FxStyle::Tongue ? 0.09f : 0.055f, d.style == FxStyle::Pull ? d.b : d.a);
            if (d.style == FxStyle::Pull) { line(P, tip, tip - me.fwd * 0.22f + glm::vec3(0, 0.16f, 0), 0.08f, d.c); line(P, tip, tip - me.fwd * 0.22f - glm::vec3(0, 0.16f, 0), 0.08f, d.c); }
            else { Particle q = now(mk(kEnergy, tip, glm::vec3(0.f), 0.12f, 5, d.a, d.a)); P.emit(q); }
            break; }
        case FxStyle::Cloud: if (p > 0.15f && p < 0.85f) for (int i = 0; i < 2; i++) { const glm::vec3 from = me.chest + me.fwd * 0.3f + P.dir() * 0.15f; Particle q = mk(d.kind, from, (foe.head - from) / 45.f + P.dir() * 0.006f, P.rnd(0.09f, 0.18f), P.rnd(46, 64), d.a, d.b); q.grow = 0.003f; q.drag = 0.99f; q.noFloor = true; if (d.kind == kFlake) { q.size *= 0.4f; q.gravity = 0.0004f; } P.emit(q); }
            break;
        case FxStyle::Tornado: if (p > 0.12f && p < 0.9f) for (int i = 0; i < 3; i++) { Particle q = mk(d.kind, me.feet, glm::vec3(0.f), P.rnd(0.06f, 0.13f), P.rnd(28, 40), d.a, d.b); q.axis = me.feet + me.fwd * 0.4f; q.v = (foe.feet - q.axis) / 36.f; q.drag = 1.f; q.orbitA = P.rnd(0.f, 6.2832f); q.orbitR = P.rnd(0.08f, 0.3f); q.orbitG = 0.006f; q.orbitV = 0.32f; q.orbitY = P.rnd(0.f, 1.7f) * me.size; q.noFloor = true; P.emit(q); }
            break;
        case FxStyle::Heal: for (int h = 0; h < 2; h++) { const float a = P.rnd(0.f, 6.2832f), r = P.rnd(0.1f, 0.55f) * me.size; Particle q = mk(h ? kGlow : kFlake, me.feet + glm::vec3(std::cos(a) * r, P.rnd(0.1f, 0.6f), std::sin(a) * r), glm::vec3(0, P.rnd(0.018f, 0.032f), 0), P.rnd(0.06f, 0.1f), P.rnd(34, 50), h ? d.a : d.b, d.c); q.drag = 0.99f; q.noFloor = true; P.emit(q); }
            break;
        case FxStyle::Leap: if (f.offsetY > 25.f) { Particle q = mk(kGlow, me.chest + P.dir() * 0.15f, P.dir() * 0.006f, P.rnd(0.04f, 0.07f), 22, d.c, d.a); q.noFloor = true; P.emit(q); }   // estrelas de lucha no ar
            break;
        case FxStyle::Rocket: if (p > 0.08f && p < 0.85f) { const glm::vec3 back = me.chest - me.fwd * 0.35f * me.size;
                for (int i = 0; i < 2; i++) { Particle q = mk(d.kind, back + P.dir() * 0.05f, -me.fwd * P.rnd(0.03f, 0.06f) + P.dir() * 0.01f, P.rnd(0.08f, 0.14f), P.rnd(12, 18), d.a, d.c); q.noFloor = true; P.emit(q); }
                if (k.frame % 2 == 0) { Particle s = mk(kPuff, back - me.fwd * 0.2f, -me.fwd * 0.015f + glm::vec3(0, 0.006f, 0), P.rnd(0.1f, 0.18f), P.rnd(30, 44), d.b, glm::vec3(0.86f, 0.82f, 0.76f)); s.grow = 0.003f; P.emit(s); } }
            break;
        case FxStyle::Beam: if (p < 0.5f && k.frame % 2 == 0) { Particle q = mk(kGlow, foe.feet, glm::vec3(0.f), 0.05f, 12, d.a, d.b); q.axis = glm::vec3(foe.feet.x, 0.06f, foe.feet.z); q.orbitA = P.rnd(0.f, 6.2832f); q.orbitR = 0.7f; q.orbitG = -0.04f; q.orbitV = 0.12f; q.drag = 1.f; q.noFloor = true; P.emit(q); }   // a mira se fecha no chão
            break;
        case FxStyle::Pulse: if (p < 0.6f && k.frame % 2 == 0) { const glm::vec3 o = P.dir() * 0.7f * me.size; Particle q = mk(kGlow, me.chest + o, -o / 12.f, 0.05f, 12, d.a, d.c); q.noFloor = true; P.emit(q); }
            break;
        case FxStyle::Wall: if (!k.launched && p > 0.3f) { k.launched = 1; wall(k, me, P); } break;
        default: break;
        }
    }

    void hit(Track& k, const FxBody& me, const FxBody& foe, Particles& P) {
        const FxDef& d = k.def; const glm::vec3 at = foe.chest - me.fwd * 0.15f;
        switch (d.style) {
        case FxStyle::Melee: case FxStyle::Rocket: burst(P, at, d, 1.f); break;
        case FxStyle::Rhythm: case FxStyle::Saw: burst(P, at, d, 0.6f); break;
        case FxStyle::Tongue: case FxStyle::Pull: burst(P, at, d, 0.7f); break;
        case FxStyle::Ground: floorRing(P, foe.feet, d, 1.f); column(P, foe.feet, d, 1.f); break;
        case FxStyle::Leap: floorRing(P, foe.feet, d, 1.4f); column(P, foe.feet, d, 0.6f); break;
        case FxStyle::Cloud: for (int i = 0; i < 14; i++) { Particle q = mk(d.kind, foe.head + P.dir() * 0.25f, P.dir() * 0.01f, P.rnd(0.12f, 0.22f) * (d.kind == kFlake ? 0.4f : 1.f), P.rnd(40, 60), d.a, d.b); q.grow = 0.003f; q.noFloor = true; P.emit(q); } break;
        case FxStyle::Tornado: burst(P, foe.chest, d, 0.7f); break;
        case FxStyle::Heal: for (int i = 0; i < 20; i++) { Particle q = mk(kGlow, me.chest + P.dir() * 0.2f, P.dir() * P.rnd(0.02f, 0.04f) + glm::vec3(0, 0.01f, 0), P.rnd(0.04f, 0.07f), P.rnd(26, 40), d.a, d.c); q.noFloor = true; P.emit(q); } break;
        case FxStyle::Beam: k.beam = 26; k.beamAt = foe.feet; floorRing(P, foe.feet, d, 1.2f); burst(P, foe.chest, d, 0.8f); break;
        case FxStyle::Pulse: for (int i = 0; i < 36; i++) { Particle q = mk(kGlow, me.chest, glm::vec3(0.f), 0.06f, 26, d.a, d.b); q.axis = me.chest; q.orbitA = float(i) / 36.f * 6.2832f; q.orbitR = 0.2f; q.orbitG = 0.09f; q.orbitV = 0.02f; q.orbitY = P.rnd(-0.1f, 0.1f); q.drag = 1.f; q.noFloor = true; P.emit(q); }
            floorRing(P, me.feet, d, 1.3f); break;
        case FxStyle::Wall: if (!k.launched) { k.launched = 1; wall(k, me, P); } break;
        default: break;   // projéteis estouram quando chegam
        }
    }

    void wall(Track& k, const FxBody& me, Particles& P) {   // favo de mel: placas na frente do corpo
        const glm::vec3 s = side(me.fwd), c = me.chest + me.fwd * 0.6f * me.size;
        for (int r = 0; r < 4; r++) for (int i = 0; i < 5; i++) { Particle q = mk(kFlake, c + s * ((float(i) - 2.f + (r % 2) * 0.5f) * 0.2f) + glm::vec3(0, (float(r) - 1.5f) * 0.19f, 0), glm::vec3(0.f), 0.1f, P.rnd(70, 90), k.def.a, k.def.b); q.hold = true; q.noFloor = true; q.drag = 1.f; P.emit(q); }
    }

    void flights(Track& k, const FxBody& foe, Particles& P) {
        for (Flight& fl : k.flights) {
            if (fl.left <= 0) continue;
            fl.p += fl.v; fl.left--;
            const FxDef& d = fl.def;
            if (fl.spear) { Particle q = now(mk(kLine, fl.p, glm::vec3(0.f), 0.07f, 5, d.a, d.a)); q.len = 0.6f; q.dir = fl.v; P.emit(q); Particle t = now(mk(kLine, fl.p + glm::normalize(fl.v) * 0.33f, glm::vec3(0.f), 0.05f, 5, d.c, d.c)); t.len = 0.12f; t.dir = fl.v; P.emit(t); }
            else { Particle q = now(mk(d.kind == kLine || d.kind == kFlake ? kEnergy : d.kind, fl.p, glm::vec3(0.f), 0.15f, 5, d.a, d.a)); P.emit(q); }
            Particle tr = mk(d.kind == kFire || d.style == FxStyle::Missiles ? kFire : kGlow, fl.p, -fl.v * 0.1f + P.dir() * 0.004f, P.rnd(0.04f, 0.08f), P.rnd(10, 16), d.c, d.b); tr.noFloor = true; P.emit(tr);   // rastro
            if (fl.left == 0) burst(P, fl.p, d, d.style == FxStyle::Missiles ? 0.6f : 1.f);
        }
        k.flights.erase(std::remove_if(k.flights.begin(), k.flights.end(), [](const Flight& f) { return f.left <= 0; }), k.flights.end());
        (void)foe;
    }

    void beamFrame(Track& k, Particles& P) {   // o raio desce do céu
        const glm::vec3 top = k.beamAt + glm::vec3(0, 7.f, 0), bot = glm::vec3(k.beamAt.x, 0.02f, k.beamAt.z); const float w = 0.55f * std::min(1.f, float(k.beam) / 8.f);
        line(P, top, bot, w * 1.6f, k.def.a); line(P, top, bot, w * 0.7f, k.def.b);
        if (k.frame % 2 == 0) { Particle s = mk(kSpark, bot + glm::vec3(0, 0.1f, 0), glm::vec3(P.rnd(-0.06f, 0.06f), P.rnd(0.03f, 0.08f), P.rnd(-0.06f, 0.06f)), 0.02f, 30, k.def.a, k.def.a); s.gravity = 0.003f; s.drag = 0.985f; P.emit(s); }
    }

    // modos e estados que duram: um pouco por quadro, em volta do corpo
    void statuses(Track& k, const Fighter& f, const FxBody& me, Particles& P, bool ownFx) {
        const StatusEffects& s = f.status; const int n = k.frame;
        const auto aura = [&](SkillId id, int every) { if (n % every) return; const FxDef d = fxDefOf(id); Particle q = mk(d.kind, me.feet, glm::vec3(0.f), P.rnd(0.07f, 0.11f), P.rnd(34, 48), d.a, d.b); q.axis = me.feet; q.orbitA = P.rnd(0.f, 6.2832f); q.orbitR = P.rnd(0.42f, 0.58f) * me.size; q.orbitV = P.rnd(0.04f, 0.07f) * (n % 2 ? 1.f : -1.f); q.orbitY = P.rnd(0.25f, 1.55f) * me.size; q.drag = 1.f; q.noFloor = true;
            if (d.kind == kLine) { q.len = 0.2f; q.dir = glm::vec3(std::cos(q.orbitA), 0.3f, std::sin(q.orbitA)); q.hold = true; q.size = 0.07f; }
            P.emit(q); };
        const auto drip = [&](const glm::vec3& c0, const glm::vec3& c1, int every) { if (n % every) return; Particle q = mk(kEnergy, me.chest + glm::vec3(P.rnd(-0.2f, 0.2f), P.rnd(-0.2f, 0.1f), P.rnd(-0.2f, 0.2f)) * me.size, glm::vec3(0.f), P.rnd(0.03f, 0.05f), 40, c0, c1); q.gravity = 0.0012f; q.drag = 0.99f; P.emit(q); };
        const auto rise = [&](int kind, const glm::vec3& at, float spread, const glm::vec3& c0, const glm::vec3& c1, int every, float sz) { if (n % every) return; Particle q = mk(kind, at + glm::vec3(P.rnd(-spread, spread), 0, P.rnd(-spread, spread)), glm::vec3(0, P.rnd(0.008f, 0.02f), 0), sz * P.rnd(0.7f, 1.2f), P.rnd(24, 36), c0, c1); q.noFloor = true; q.drag = 0.99f; P.emit(q); };
        if (!ownFx) {
        if (s.molasses > 0) { aura(SkillId::JackMolasses, 2); drip(fxHex(0xf2a20c), fxHex(0xb3620a), 7); }
        if (s.honeyShield > 0) aura(SkillId::JackHoneyWall, 5);
        if (s.poisonSkin > 0) aura(SkillId::FrogPoisonSkin, 2);
        if (s.shell > 0) aura(SkillId::JoanaCrustaceanShell, 2);
        if (s.melt > 0) { aura(SkillId::CremosoMeltMode, 3); drip(fxHex(0xfff0d0), fxHex(0xf6c6d8), 5); }
        if (s.heatwave > 0) { aura(SkillId::ValeriaHeatwaveGuard, 2); }
        if (s.seasons > 0) aura(SkillId::VivaldiFourSeasons, 2);
        if (s.thornVeil > 0) aura(SkillId::EspinhaThornVeil, 1);
        if (s.plasma > 0) aura(SkillId::MarcovPlasmaBarrier, 1);
        }
        // quem sofre
        if (s.burn > 0) rise(kFire, me.chest, 0.22f * me.size, glm::vec3(1.f), glm::vec3(1.f), 4, 0.07f);
        if (s.poisoned > 0) rise(kGlow, me.chest, 0.25f * me.size, fxHex(0x9be33a), fxHex(0x7b2fbf), 6, 0.04f);
        // (a Infestação tem as abelhas de verdade em render/char_fx.hpp)
        if (s.slowed > 0) drip(fxHex(0xc9761f), fxHex(0x7a3b12), 9);
        if (s.blind > 0) rise(kPuff, me.head, 0.18f * me.size, fxHex(0x4a4a52), fxHex(0x24242a), 5, 0.1f);
        if (s.mud > 0) rise(kPuff, me.feet + glm::vec3(0, 0.05f, 0), 0.2f * me.size, fxHex(0x6b4a24), fxHex(0x3f2a12), 8, 0.08f);
        if (s.bleed > 0) drip(fxHex(0xe3243b), fxHex(0x8a1020), 8);
    }
};

// Linhas de velocidade de quadrinho: atrás da luva que vai (soco ou especial; a luva voltando para o corpo não deixa rastro) e atrás do
// corpo quando ele anda rápido para o lado (esquiva, passo). Riscos finos paralelos ao movimento, que afinam e somem em poucos quadros.
class SpeedLines {
public:
    void reset() { ready_[0] = ready_[1] = false; }
    // um risco do rastro: nasce atrás do ponto, na direção do movimento
    static void streak(Particles& P, const glm::vec3& at, const glm::vec3& dir, float len, bool white) { const glm::vec3 c = white ? glm::vec3(1.f, 0.98f, 0.92f) : glm::vec3(0.16f, 0.1f, 0.1f); Particle q; q.kind = kLine; q.p = at - dir * len * 0.5f; q.len = len; q.dir = dir; q.size = white ? 0.05f : 0.035f; q.life = P.rnd(6, 9); q.c0 = glm::vec4(c, 1.f); q.c1 = q.c0; q.noFloor = true; q.drag = 1.f; P.emit(q); }
    // gloves: false quando as luvas já deixam o rastro esticado (Renderer::Smear); aí só o corpo ganha riscos
    // body: false quando o corpo só anda porque o ringue girou na esquiva do outro (na tela ele fica parado: sem riscos)
    void tick(int who, const Fighter& f, const FxBody& me, Particles& P, bool gloves = true, bool body = true) {
        const bool active = gloves && (isPunchState(f.state) || f.state == FIGHTER_SKILL);
        for (int g = 0; g < 2; g++) {
            const glm::vec3 now = me.glove[g], prev = ready_[who] ? prev_[who][g] : now; prev_[who][g] = now;
            const glm::vec3 d = now - prev; const float speed = glm::length(d);
            if (!active || speed < kMinSpeed) continue;
            if (glm::dot(d / speed, glm::normalize(now - me.chest + glm::vec3(1e-4f))) < -0.25f) continue;   // voltando para o corpo
            const glm::vec3 dir = d / speed, side = glm::normalize(glm::cross(dir, std::fabs(dir.y) > 0.9f ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0))), up = glm::cross(side, dir);
            const float r = kGloveR * me.size, len = std::min(0.55f, speed * 3.2f) * me.size;
            for (int k = 0; k < kLines; k++) {
                const float a = (float(k) + P.rnd(-0.3f, 0.3f)) / float(kLines) * 6.2832f, rr = r * P.rnd(0.55f, 1.05f), l = len * P.rnd(0.6f, 1.15f);
                const glm::vec3 base = now - dir * r * 0.4f + (side * std::cos(a) + up * std::sin(a)) * rr;
                streak(P, base, dir, l, k == 0 || (k == 3 && speed > 0.08f));
            }
        }
        // corpo andando rápido para o lado: riscos ao longo da altura dele, do lado de onde saiu (inclusive voando para as cordas depois do golpe que derruba)
        { const glm::vec3 prev = ready_[who] ? body_[who] : me.chest; body_[who] = me.chest; glm::vec3 d = me.chest - prev; d.y = 0.f; const float speed = glm::length(d);
          if (body && speed > kBodySpeed) { const glm::vec3 dir = d / speed, side = glm::cross(dir, glm::vec3(0, 1, 0)); const float h = glm::max(0.3f, me.head.y - me.feet.y), w = kBodyR * me.size, len = std::min(0.9f, speed * 5.f) * me.size;
            for (int k = 0; k < kBodyLines; k++) { const float y = me.feet.y + h * P.rnd(0.12f, 1.f), sw = P.rnd(-1.f, 1.f) * w; const glm::vec3 base = glm::vec3(me.chest.x, y, me.chest.z) - dir * w * 0.9f + side * sw;
              streak(P, base, dir, len * P.rnd(0.6f, 1.2f), k == 0); } } }
        ready_[who] = true;
    }
private:
    static constexpr float kMinSpeed = 0.035f, kGloveR = 0.11f;   // metros por quadro a partir dos quais a luva deixa rastro; raio da luva
    static constexpr int kLines = 5, kBodyLines = 4;
    static constexpr float kBodySpeed = 0.022f, kBodyR = 0.32f;   // metros por quadro do corpo de lado; meia largura do corpo
    glm::vec3 prev_[2][2]{}, body_[2]{}; bool ready_[2]{false, false};
};
}
