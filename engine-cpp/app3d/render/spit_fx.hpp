// app3d/render/spit_fx.hpp: golpes cuspidos da boca.
//  - Cuspidela de Lama (King Frog): o papo da garganta incha, ele cospe uma bola de lama em arco que chega no rival no quadro do dano
//    (35% do golpe), respinga nele, escorre e cai; no chão vira uma poça que cresce e depois vai sendo absorvida devagar (os 3 s do efeito).
//  - Cereja do Bolo (Cremoso): a cabeça incha em três estágios, ele cospe a cereja gigante (com cabinho) que gira até o rival e estoura em
//    suco; fica uma poça vermelha que some devagar.
// Bola, cereja e poças vão pelo Renderer::props; tempo próprio (começa com o golpe e segue depois dele).
#pragma once
#include "particles.hpp"
#include "throwables.hpp"
#include "fighter.hpp"
#include "skills.hpp"
#include <algorithm>
#include <cmath>
#include <vector>
#include <glm/glm.hpp>
namespace KoRama {
struct SpitFx {
    enum Style { kNone = -1, kMud = 0, kCherry = 1 };
    // stretch: quadros que o jogo 3D acrescenta ao golpe do motor (a carga precisa de tempo; o acerto anda junto, no mesmo % do golpe)
    struct Look { SkillId id; uint32_t charId; int spit, hit, absorb, end; float pR; glm::vec3 pc, pc2; int stretch; };
    static const Look& look(int s) { static const Look k[2] = {
        {SkillId::FrogMudSlime, 1, 14, 19, 150, 245, 0.95f, {0.28f, 0.17f, 0.08f}, {0.42f, 0.28f, 0.14f}, 28},
        {SkillId::CremosoCherryTopping, 3, 40, 51, 110, 190, 0.8f, {0.55f, 0.04f, 0.06f}, {0.86f, 0.2f, 0.24f}, 42} }; return k[std::clamp(s, 0, 1)]; }
    static int styleOf(const Fighter& f) { if (f.state != FIGHTER_SKILL) return kNone; for (int s = 0; s < 2; s++) if (f.activeSkill == look(s).id && f.charId == look(s).charId) return s; return kNone; }
    static bool active(const Fighter& f) { return styleOf(f) != kNone; }

    float t = -1.f; int style = kNone; bool was = false, hit = false, justHit = false; glm::vec3 from{0.f}, to{0.f}, puddle{0.f}, ball{0.f}; float spin = 0.f; uint32_t seed = 1u;
    void reset() { t = -1; was = false; style = kNone; }
    bool on() const { return t >= 0 && style != kNone; }
    static float sat(float x) { return std::clamp(x, 0.f, 1.f); }
    static float smooth(float x) { x = sat(x); return x * x * (3.f - 2.f * x); }
    const Look& lk() const { return look(style); }

    // quanto a garganta (Frog) ou a cabeça (Cremoso) está inchada (0..1)
    float inflate() const {
        if (!on()) return 0.f; const float e = float(t);
        // três golfadas de ar, cada uma com um tranco que passa do ponto e volta (mola), tremendo mais perto de soltar
        const auto stage = [&](float a, float len, float amt) { const float u = (e - a) / len; if (u <= 0.f) return 0.f; const float k = u >= 1.f ? 1.f : 1.f - (1.f - u) * (1.f - u); return amt * (k + 0.25f * std::exp(-u * 3.f) * std::sin(u * 9.f) * (u > 0.2f ? 1.f : 0.f)); };
        if (style == kMud) { if (e < 14.f) return (stage(0.f, 4.f, 0.4f) + stage(5.f, 4.f, 0.3f) + stage(10.f, 4.f, 0.3f)) * (1.f + 0.05f * std::sin(e * 3.1f) * smooth(e / 12.f)); return 1.f - smooth((e - 14.f) / 2.f); }
        if (e < 40.f) return (stage(0.f, 10.f, 0.33f) + stage(13.f, 10.f, 0.33f) + stage(27.f, 10.f, 0.34f)) * (1.f + 0.05f * std::sin(e * 2.3f) * smooth((e - 20.f) / 20.f));
        if (e < 44.f) return 1.f - 0.85f * smooth((e - 40.f) / 4.f);
        return 0.15f * (1.f - smooth((e - 44.f) / 26.f));
    }
    // tamanho da poça (0..1) e quanto já foi absorvida (0..1)
    static constexpr bool kPuddle = false;
    float cheeks() const { return on() && style == kCherry ? inflate() : 0.f; }
    float puddleK() const { if (!on() || t < lk().hit) return 0.f; return smooth(float(t - lk().hit) / 22.f); }
    float absorbed() const { if (!on()) return 1.f; return smooth(float(t - lk().absorb) / float(lk().end - lk().absorb)); }

    // pose: joga o corpo para trás enchendo (o Frog levanta o queixo) e dá o tranco para a frente na cuspida
    static void pose(Fighter& view, const Fighter& f) {
        const int st = styleOf(f); if (st == kNone) return; const Look& L = look(st); const float e = skillElapsed(f), sp = float(L.spit) - 3.f;
        const float load = smooth(e / sp) * (1.f - smooth((e - sp) / 2.f)), kick = smooth((e - sp) / 2.f) * (1.f - smooth((e - sp - 4.f) / 10.f));
        view.offsetZ = -14.f * load + 18.f * kick; view.offsetY = (st == kMud ? 5.f : 3.f) * load - 6.f * kick; view.bodySkew = 0.03f * std::sin(e * 2.9f) * load;
        view.gloveL.x = view.baseGloveL.x - 10.f * load; view.gloveR.x = view.baseGloveR.x + 10.f * load; view.gloveL.y = view.gloveR.y = view.baseGloveL.y - 6.f * load + 8.f * kick; view.gloveL.z = view.gloveR.z = view.baseGloveL.z - 8.f * load + 10.f * kick;
    }
    // o corpo inteiro: cada golfada estica o corpo para cima e arqueia para trás (cabeça para trás), tremendo mais perto de soltar;
    // na cuspida o corpo amassa e chicoteia para a frente com a cabeça junto, e volta em mola passando do ponto
    struct Body { float lean = 0.f, neck = 0.f; glm::vec3 scale{1.f}; };
    Body body() const {
        Body b; if (!on()) return b; const float e = float(t), sp = float(lk().spit), i = std::min(1.f, inflate());
        const float sw = sp - 4.f;   // começa a vir para a frente antes: a cuspida sai no meio do movimento
        if (e < sw) { const float shake = 0.04f * std::sin(e * 4.3f) * smooth((e - sw * 0.5f) / (sw * 0.5f)); const float k = std::min(1.f, std::max(i, smooth(e / sw)));
            b.lean = -0.38f * k + shake; b.neck = -0.45f * k; b.scale = glm::vec3(1.f - 0.06f * k + 0.5f * shake, 1.f + 0.12f * k, 1.f - 0.06f * k + 0.5f * shake); return b; }
        const float u = e - sw, snap = 1.f - smooth(u / 6.f);   // de trás para a frente em 6 quadros, a cuspida no meio
        const float spring = std::exp(-u * 0.22f) * std::cos(u * 0.55f);   // depois balança e assenta
        b.lean = -0.38f * snap + 0.5f * spring * (1.f - snap); b.neck = -0.45f * snap + 0.5f * spring * (1.f - snap);
        const float sq = 0.2f * spring * (1.f - snap); b.scale = glm::vec3(1.f + sq, 1.f - sq * 1.1f, 1.f + sq) * (1.f - snap) + glm::vec3(0.94f, 1.12f, 0.94f) * snap;
        return b; }
    // rosto do Cremoso: olhos apertando enquanto incha, boca aberta na cuspida ("" = o normal)
    const char* face() const { if (!on() || style != kCherry) return ""; const float e = float(t);
        if (e >= 37.f && e < 50.f) return "spit3"; if (e >= 50.f && e < 58.f) return "spit2"; if (e < 39.f) { const float i = inflate(); return i > 0.6f ? "spit2" : i > 0.2f ? "spit1" : ""; } return ""; }
    void tick(const Fighter& f, const glm::vec3& mouth, const glm::vec3& targetChest, const glm::vec3& targetFeet, Particles& P) { justHit = false;
        const int st = styleOf(f); const bool in = st != kNone;
        if (in && (!was || st != style)) { t = 0; style = st; hit = false; seed = uint32_t(f.timer * 131 + 7); }
        else if (in) t = std::max(t, skillElapsed(f)); else if (t >= 0) { t += 1.f / skillStretch(f.charId); if (t > lk().end) { t = -1; style = kNone; } }
        was = in; if (t < 0) return;
        const Look& L = lk();
        if (t <= L.spit) { from = mouth; to = targetChest; puddle = glm::vec3(targetFeet.x, 0.f, targetFeet.z) + glm::normalize(glm::vec3(mouth.x - targetFeet.x, 0.f, mouth.z - targetFeet.z) + glm::vec3(1e-5f)) * 0.3f; }   // um pouco à frente do rival (escorre para a frente)
        if (t >= L.spit && t < L.hit) { const float u = float(t - L.spit + 1) / float(L.hit - L.spit + 1); ball = from + (to - from) * u + glm::vec3(0, std::sin(u * 3.14159f) * (style == kMud ? 0.18f : 0.3f), 0); spin += 0.6f;
            for (int k = 0; k < 2; k++) { Particle q; q.kind = style == kMud ? kEnergy : kGlow; q.p = ball + P.dir() * 0.05f; q.v = P.dir() * 0.005f + glm::vec3(0, -0.006f, 0); q.size = P.rnd(0.025f, 0.045f); q.gravity = 0.003f; q.life = P.rnd(14, 22); q.noFloor = false;
                q.c0 = glm::vec4(style == kMud ? glm::vec3(0.36f, 0.22f, 0.1f) : glm::vec3(0.95f, 0.15f, 0.2f), 1.f); q.c1 = glm::vec4(style == kMud ? glm::vec3(0.2f, 0.12f, 0.05f) : glm::vec3(1.f, 0.75f, 0.8f), 1.f); P.emit(q); } }   // rastro
        if (!hit && t >= L.hit) { hit = true; justHit = true;   // respingo: bolotas grudam e escorrem, gotas caem no chão
            const glm::vec3 c0 = style == kMud ? glm::vec3(0.38f, 0.24f, 0.11f) : glm::vec3(0.92f, 0.1f, 0.16f), c1 = style == kMud ? glm::vec3(0.2f, 0.12f, 0.05f) : glm::vec3(0.55f, 0.03f, 0.06f);
            for (int k = 0; k < 36; k++) { Particle q; q.kind = kEnergy; q.p = to + P.dir() * 0.12f; const glm::vec3 d = P.dir(); q.v = glm::vec3(d.x, std::fabs(d.y) * 0.6f, d.z) * P.rnd(0.02f, 0.06f); q.size = P.rnd(0.03f, 0.07f); q.gravity = 0.0035f; q.drag = 0.96f; q.life = P.rnd(28, 46);
                q.c0 = glm::vec4(c0, 1.f); q.c1 = glm::vec4(c1, 1.f); P.emit(q); }
            for (int k = 0; k < 14; k++) { Particle q; q.kind = kEnergy; q.p = to + glm::vec3(P.rnd(-0.18f, 0.18f), P.rnd(-0.2f, 0.15f), P.rnd(-0.12f, 0.12f)); q.v = glm::vec3(0, -P.rnd(0.003f, 0.008f), 0); q.size = P.rnd(0.04f, 0.07f); q.gravity = 0.0012f; q.drag = 0.9f; q.life = P.rnd(30, 50);
                q.c0 = glm::vec4(c0, 1.f); q.c1 = glm::vec4(c1, 1.f); P.emit(q); }   // escorrendo pelo corpo
            if (style == kCherry) for (int k = 0; k < 24; k++) { Particle q; q.kind = kGlow; q.p = to; q.v = P.dir() * P.rnd(0.03f, 0.07f); q.size = P.rnd(0.02f, 0.035f); q.drag = 0.92f; q.life = P.rnd(14, 24); q.noFloor = true;
                q.c0 = glm::vec4(1.f, 0.8f, 0.85f, 1.f); q.c1 = glm::vec4(0.95f, 0.2f, 0.3f, 1.f); P.emit(q); } }
        if (kPuddle && hit && t < L.hit + 30 && P.rnd(0.f, 1.f) < 0.5f) { Particle q; q.kind = kEnergy; q.p = to + glm::vec3(P.rnd(-0.15f, 0.15f), P.rnd(-0.3f, 0.f), P.rnd(-0.1f, 0.1f)); q.v = glm::vec3(0, -0.004f, 0); q.size = P.rnd(0.025f, 0.045f); q.gravity = 0.0025f; q.life = 30;
            q.c0 = glm::vec4(L.pc2, 1.f); q.c1 = glm::vec4(L.pc, 1.f); P.emit(q); }   // pingando do rival na poça
    }

    // malha: a bola de lama / a cereja voando e a poça no chão (borda escura por baixo no lugar do contorno: chapa reta não leva casco)
    void build(std::vector<float>& v, std::vector<unsigned>& idx) const {
        if (!on()) return; const Look& L = lk();
        const auto putMesh = [&](const Throwables::Mesh& m) { const unsigned base = unsigned(v.size() / 12); for (const Throwables::PV& pv : m.v) { const float d[12] = {pv.p.x, pv.p.y, pv.p.z, pv.n.x, pv.n.y, pv.n.z, pv.c.r, pv.c.g, pv.c.b, -1.f, -1.f, pv.ink}; v.insert(v.end(), d, d + 12); } for (unsigned k : m.idx) idx.push_back(base + k); };
        if (t >= L.spit && t < L.hit) { Throwables::Mesh m; const glm::vec3 X(std::cos(spin), 0, std::sin(spin)), Z(-std::sin(spin), 0, std::cos(spin));
            if (style == kMud) { Throwables::blob(m, ball, X * 0.19f, glm::vec3(0, 0.16f, 0), Z * 0.19f, L.pc2, 0.007f, 0.f); Throwables::blob(m, ball + glm::vec3(-0.07f, 0.08f, 0.06f), X * 0.07f, glm::vec3(0, 0.07f, 0), Z * 0.07f, L.pc, 0.005f, 0.f); Throwables::blob(m, ball + glm::vec3(0.08f, -0.05f, -0.03f), X * 0.06f, glm::vec3(0, 0.05f, 0), Z * 0.06f, L.pc, 0.005f, 0.f); }
            else { Throwables::blob(m, ball, X * 0.2f, glm::vec3(0, 0.2f, 0), Z * 0.2f, glm::vec3(0.86f, 0.06f, 0.08f), 0.007f, 0.f); Throwables::blob(m, ball + glm::vec3(-0.07f, 0.1f, -0.08f), X * 0.045f, glm::vec3(0, 0.045f, 0), Z * 0.045f, glm::vec3(1.f), 0.f, 0.f);
                Throwables::blob(m, ball + glm::vec3(0.04f, 0.3f, 0.f), glm::vec3(0.014f, 0, 0), glm::vec3(0.05f, 0.11f, 0) * 0.9f, glm::vec3(0, 0, 0.014f), glm::vec3(0.06f, 0.5f, 0.12f), 0.004f, 0.f); }   // brilho e cabinho
            putMesh(m); }
        if (!kPuddle) return;   // sem poça no chão (pedido: só o respingo no rival)
        const float pk = puddleK(), ab = absorbed(); if (pk <= 0.01f || ab >= 0.99f) return;
        const float R = L.pR * pk * (1.f - 0.55f * ab); constexpr int N = 30; uint32_t h = seed;
        float wob[N]; for (int i = 0; i < N; i++) { h = h * 1664525u + 1013904223u; wob[i] = 0.82f + 0.3f * float(h >> 8) / 16777216.f; }
        const auto disc = [&](float r, float y, const glm::vec3& col) { const unsigned c = unsigned(v.size() / 12); const float d0[12] = {puddle.x, y, puddle.z, 0, 1, 0, col.r, col.g, col.b, -1.f, -1.f, 0.f}; v.insert(v.end(), d0, d0 + 12);
            for (int i = 0; i < N; i++) { const float a = float(i) / N * 6.2832f, rr = r * (wob[i] * 0.7f + 0.3f * wob[(i + 1) % N]); const float d[12] = {puddle.x + std::cos(a) * rr, y, puddle.z + std::sin(a) * rr, 0, 1, 0, col.r, col.g, col.b, -1.f, -1.f, 0.f}; v.insert(v.end(), d, d + 12); }
            for (int i = 0; i < N; i++) idx.insert(idx.end(), {c, c + 1 + unsigned((i + 1) % N), c + 1 + unsigned(i)}); };
        const float y = Particles::kFloor + 0.004f; const glm::vec3 base = glm::mix(L.pc, L.pc * 0.55f, ab);
        disc(R + 0.025f, y, glm::vec3(0.05f, 0.03f, 0.02f));             // borda de tinta
        disc(R, y + 0.002f, base);                                          // a poça
        disc(R * 0.55f * (1.f - ab), y + 0.004f, glm::mix(L.pc2, base, ab));   // o brilho molhado (some primeiro)
    }
};
}
