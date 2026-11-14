// app3d/render/char_fx.hpp: especiais refeitos em código, no lugar das peças gravadas do vinil que não ficavam boas no jogo:
//  - BeeFx: as abelhas da Infestação do Jack (corpo listrado, asas batendo, ferrão), que saem das colmeias, rodeiam o rival e voltam
//  - MeltFx: Modo Derretido e Poça de Calda do Cremoso (o corpo achata e escorre numa poça ondulada, da cor dele, com volume)
//  - ConeFx: Estocada de Casquinha do Cremoso (a casquinha cresce do peito até o rival)
//  - BubbleFx: Escudo Sputnik do Marcov (bolha com malha de hexágonos pulsando)
//  - LaserFx: Raio Orbital Sputnik do Marcov (o satélite desce, a mira fecha no chão e o feixe cai)
//  - JetFx: as turbinas das costas do Marcov (Propulsão Foguete e Impulso de Reação)
// Tudo sai em vértices de 12 números (posição, normal, cor, uv, contorno) para Renderer::props (peça com luz ou de cor própria) e Renderer::glow.
#pragma once
#include "fighter.hpp"
#include "render/particles.hpp"
#include "rig/rig.hpp"
#include "skills.hpp"
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
namespace KoRama {

// peças simples; triângulos no sentido anti-horário vistos de fora (o contorno de tinta depende disso)
struct FxGeo {
    using V = std::vector<float>; using I = std::vector<unsigned>;
    static constexpr float kLit = -1.f, kUnlit = -2.f;   // u do vértice: com a luz do jogo ou de cor própria
    static float sat(float x) { return std::clamp(x, 0.f, 1.f); }
    static float smooth(float x) { x = sat(x); return x * x * (3.f - 2.f * x); }
    static float prog(const Fighter& f) { return 1.f - float(f.timer) / float(std::max(1, f.maxTimer)); }
    static unsigned put(V& v, const glm::vec3& p, const glm::vec3& n, const glm::vec3& c, float u = kLit, float ink = 0.f) { const float d[12] = {p.x, p.y, p.z, n.x, n.y, n.z, c.r, c.g, c.b, u, -1.f, ink}; v.insert(v.end(), d, d + 12); return unsigned(v.size() / 12) - 1u; }
    // dois eixos de lado para uma direção
    static void basis(const glm::vec3& d, glm::vec3& s, glm::vec3& u) { s = glm::normalize(glm::cross(std::fabs(d.y) > 0.92f ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0), d)); u = glm::cross(d, s); }
    // elipsoide em faixas ao longo de ey (cada faixa com a cor dela, de borda seca)
    template <class ColFn> static void bands(V& v, I& i, const glm::vec3& c, const glm::vec3& ex, const glm::vec3& ey, glm::vec3 ez, int NL, int NJ, ColFn col, float u, float ink) {
        if (glm::dot(glm::cross(ex, ey), ez) > 0.f) ez = -ez;
        const float lx = glm::dot(ex, ex), ly = glm::dot(ey, ey), lz = glm::dot(ez, ez);
        for (int b = 0; b < NL; b++) { const unsigned base = unsigned(v.size() / 12); const glm::vec3 cc = col(b);
            for (int r = 0; r < 2; r++) { const float th = 3.14159f * float(b + r) / float(NL);
                for (int j = 0; j < NJ; j++) { const float ph = 6.28318f * float(j) / float(NJ); const glm::vec3 l(std::sin(th) * std::cos(ph), std::cos(th), std::sin(th) * std::sin(ph));
                    put(v, c + ex * l.x + ey * l.y + ez * l.z, glm::normalize(ex * (l.x / lx) + ey * (l.y / ly) + ez * (l.z / lz) + glm::vec3(1e-6f)), cc, u, ink); } }
            for (int j = 0; j < NJ; j++) { const unsigned a = base + unsigned(j), b2 = base + unsigned((j + 1) % NJ), c2 = a + unsigned(NJ), d2 = b2 + unsigned(NJ); i.insert(i.end(), {a, c2, b2, b2, c2, d2}); } }
    }
    static void ball(V& v, I& i, const glm::vec3& c, const glm::vec3& ex, const glm::vec3& ey, const glm::vec3& ez, const glm::vec3& col, float u, float ink, int NL = 6, int NJ = 9) { bands(v, i, c, ex, ey, ez, NL, NJ, [&](int) { return col; }, u, ink); }
    // peça de torno: anéis (distância ao longo de d, raio) em volta do eixo que sai de c
    struct Ring { float along, r; glm::vec3 col; };
    static void lathe(V& v, I& i, const glm::vec3& c, const glm::vec3& d, const Ring* rings, int n, int NJ, float u, float ink) {
        glm::vec3 s, up; basis(d, s, up); const unsigned base = unsigned(v.size() / 12);
        for (int k = 0; k < n; k++) { const Ring& a = rings[std::max(0, k - 1)]; const Ring& b = rings[std::min(n - 1, k + 1)]; const float da = b.along - a.along, dr = b.r - a.r;
            for (int j = 0; j < NJ; j++) { const float ph = 6.28318f * float(j) / float(NJ); const glm::vec3 rad = s * std::cos(ph) + up * std::sin(ph);
                put(v, c + d * rings[k].along + rad * rings[k].r, glm::normalize(rad * da - d * dr + glm::vec3(1e-6f)), rings[k].col, u, ink); } }
        for (int k = 0; k + 1 < n; k++) for (int j = 0; j < NJ; j++) { const unsigned a = base + unsigned(k * NJ + j), b = base + unsigned(k * NJ + (j + 1) % NJ), c2 = a + unsigned(NJ), d2 = b + unsigned(NJ); i.insert(i.end(), {a, b, c2, b, d2, c2}); }
    }
    // chapa: quatro cantos (frente e verso desenhados; sem contorno)
    static void quad(V& v, I& i, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& d, const glm::vec3& col, float u) { const glm::vec3 n = glm::normalize(glm::cross(b - a, d - a) + glm::vec3(1e-6f)); const unsigned k = put(v, a, n, col, u); put(v, b, n, col, u); put(v, c, n, col, u); put(v, d, n, col, u); i.insert(i.end(), {k, k + 1, k + 2, k, k + 2, k + 3}); }
    // caixa com contorno
    static void box(V& v, I& i, const glm::vec3& c, const glm::vec3& ex, const glm::vec3& ey, const glm::vec3& ez, const glm::vec3& col, float u, float ink) {
        const glm::vec3 ax[3] = {ex, ey, ez};
        for (int f = 0; f < 6; f++) { const glm::vec3 n = ax[f / 2] * (f % 2 ? -1.f : 1.f), a = ax[(f / 2 + 1) % 3], b = ax[(f / 2 + 2) % 3], nn = glm::normalize(n); const float sgn = glm::dot(glm::cross(a, b), n) > 0.f ? 1.f : -1.f;
            const unsigned k = put(v, c + n - a - b * sgn, nn, col, u, ink); put(v, c + n + a - b * sgn, nn, col, u, ink); put(v, c + n + a + b * sgn, nn, col, u, ink); put(v, c + n - a + b * sgn, nn, col, u, ink); i.insert(i.end(), {k, k + 1, k + 2, k, k + 2, k + 3}); }
    }
    // poça com volume: borda em lóbulos que ondulam, miolo que sobe para colar no corpo, ondas correndo para fora
    static void puddle(V& v, I& i, const glm::vec3& c, const glm::vec3& ax, const glm::vec3& az, float thick, float rise, const glm::vec3& col, const glm::vec3& edge, float T, float seed, float ink) {
        constexpr int NR = 7, NJ = 44; const unsigned base = unsigned(v.size() / 12); glm::vec3 P[NR + 1][NJ];
        for (int k = 0; k <= NR; k++) { const float rho = float(k) / float(NR);
            for (int j = 0; j < NJ; j++) { const float a = 6.28318f * float(j) / float(NJ);
                const float w = 1.f + (0.11f * std::sin(3.f * a + T * 1.1f + seed) + 0.075f * std::sin(5.f * a - T * 0.85f + seed * 2.3f) + 0.04f * std::sin(9.f * a + T * 1.7f)) * rho;
                const float mound = 1.f - smooth(rho / 0.6f), rim = std::sqrt(std::max(0.f, 1.f - rho * rho * rho * rho * rho * rho)), wave = std::sin(rho * 12.f - T * 3.6f + 0.6f * std::sin(a * 2.f + seed)) * rho * (1.f - rho) * 4.f;
                P[k][j] = c + (ax * std::cos(a) + az * std::sin(a)) * (rho * w) + glm::vec3(0.f, rise * mound * mound + thick * rim * (1.f + 0.22f * wave), 0.f); } }
        for (int k = 0; k <= NR; k++) for (int j = 0; j < NJ; j++) { const glm::vec3 pr = P[std::min(NR, k + 1)][j] - P[std::max(0, k - 1)][j], pt = P[k][(j + 1) % NJ] - P[k][(j + NJ - 1) % NJ];
            glm::vec3 n = k == 0 ? glm::vec3(0, 1, 0) : glm::normalize(glm::cross(pt, pr) + glm::vec3(0.f, 1e-6f, 0.f)); if (k == NR) n = glm::normalize(n + glm::normalize(P[k][j] - c) * 1.5f);
            put(v, P[k][j], n, k >= NR - 1 ? edge : col, kLit, ink); }
        for (int k = 0; k < NR; k++) for (int j = 0; j < NJ; j++) { const unsigned a = base + unsigned(k * NJ + j), b = base + unsigned(k * NJ + (j + 1) % NJ), c2 = a + unsigned(NJ), d2 = b + unsigned(NJ); i.insert(i.end(), {a, b, c2, b, d2, c2}); }
    }
};

// ---------------- Jack: Infestação de Abelhas ----------------
struct BeeFx : FxGeo {
    static constexpr int kN = 9;
    struct Bee { glm::vec3 p{0.f}, f{0.f, 0.f, 1.f}; float k = 0.f; bool out = false; };
    Bee b[kN]; int t = -1; float T = 0.f; bool was = false; float size = 1.f; int stung = 0;
    static bool casting(const Fighter& f) { return f.charId == 0 && f.state == FIGHTER_SKILL && f.activeSkill == SkillId::JackInfestation; }
    void reset() { *this = BeeFx{}; }
    bool on() const { return t >= 0; }
    // hive: as duas colmeias (luvas); head/chest: do rival
    void tick(const Fighter& me, const Fighter& foe, const glm::vec3& hiveL, const glm::vec3& hiveR, const glm::vec3& head, const glm::vec3& chest, float sz, Particles& P) {
        const bool in = casting(me); if (in && !was) { t = 0; for (int i = 0; i < kN; i++) { b[i] = Bee{}; b[i].p = i % 2 ? hiveL : hiveR; } } was = in;
        if (t < 0) return; t++; T += 1.f / 60.f; size = sz; const bool stay = in || (me.charId == 0 && foe.status.infested > 0); bool any = false;
        for (int i = 0; i < kN; i++) { Bee& q = b[i]; const glm::vec3 home = i % 2 ? hiveL : hiveR, prev = q.p; const float fi = float(i);
            if (stay && !q.out && t >= 5 + i * 2) q.out = true; if (!stay) q.out = false;
            glm::vec3 goal = home;
            if (q.out) { const float a = T * (2.6f + 0.35f * std::sin(fi * 1.7f)) * (i % 2 ? 1.f : -1.f) + fi * 0.9f; const float dive = std::pow(std::max(0.f, std::sin(T * 2.3f + fi * 2.1f)), 8.f);
                const glm::vec3 c = glm::mix(head, chest, 0.5f + 0.5f * std::sin(fi * 2.4f)); const float r = (0.34f + 0.1f * std::sin(fi * 1.9f + T)) * sz * (1.f - 0.75f * dive);
                goal = c + glm::vec3(std::cos(a) * r, (0.12f * std::sin(fi * 3.1f) + 0.07f * std::sin(T * 5.f + fi)) * sz, std::sin(a) * r); }
            const glm::vec3 d = goal - q.p; const float L = glm::length(d), step = std::min(L, std::max(0.035f * sz, L * (q.out ? 0.2f : 0.12f))); if (L > 1e-4f) q.p += d / L * step;
            if (q.out) q.p += glm::vec3(std::sin(T * 31.f + fi * 5.f), std::cos(T * 27.f + fi * 3.f), std::sin(T * 23.f + fi)) * (0.004f * sz);   // o tremido do voo
            const glm::vec3 mv = q.p - prev; if (glm::length(mv) > 1e-4f) q.f = glm::normalize(glm::mix(q.f, glm::normalize(mv), 0.35f) + glm::vec3(1e-6f));
            const bool show = q.out || L > 0.1f * sz; q.k = std::clamp(q.k + (show && (q.out || q.k > 0.f) ? 0.12f : -0.15f), 0.f, 1.f); any |= q.k > 0.f || q.out; }
        // a ferroada: quando o enxame tira vida, um estalo pequeno no rival
        if (me.charId == 0 && foe.status.infested > 0 && foe.status.infested % 20 == 0 && foe.status.infested != stung) { stung = foe.status.infested;
            for (int i = 0; i < 4; i++) { Particle s; s.kind = kLine; s.p = glm::mix(head, chest, P.rnd(0.f, 1.f)) + P.dir() * 0.15f * sz; s.v = P.dir() * P.rnd(0.02f, 0.04f); s.dir = s.v; s.len = 0.09f; s.size = 0.025f; s.life = P.rnd(7, 11); s.drag = 0.85f; s.noFloor = true; s.c0 = s.c1 = glm::vec4(1.f, 0.93f, 0.4f, 1.f); P.emit(s); } }
        if (!any && !stay) t = -1;
    }
    // body: peças com luz e contorno; wings: asas translúcidas (desenhar com opacidade)
    void build(V& body, I& bi, V& wings, I& wi) const {
        if (!on()) return; const glm::vec3 yel(1.f, 0.78f, 0.1f), dark(0.17f, 0.11f, 0.07f), fuzz(0.98f, 0.66f, 0.1f), wingC(0.9f, 0.97f, 1.f);
        for (int i = 0; i < kN; i++) { const Bee& q = b[i]; if (q.k <= 0.02f) continue; const float L = 0.05f * size * q.k, ink = 0.0045f * size;
            const glm::vec3 f = q.f, sd = glm::normalize(glm::cross(glm::vec3(0, 1, 0), f) + glm::vec3(1e-5f, 0, 0)), up = glm::cross(f, sd);
            bands(body, bi, q.p - f * (0.75f * L), sd * (0.66f * L), -f * (1.05f * L), up * (0.66f * L), 5, 8, [&](int k) { return k % 2 ? yel : dark; }, kLit, ink);   // abdômen listrado (a ponta de trás escura)
            ball(body, bi, q.p + f * (0.42f * L), sd * (0.56f * L), up * (0.56f * L), f * (0.56f * L), fuzz, kLit, ink, 5, 8);                                     // tórax
            ball(body, bi, q.p + f * (1.05f * L), sd * (0.42f * L), up * (0.42f * L), f * (0.4f * L), dark, kLit, ink, 5, 8);                                     // cabeça
            { const Ring st[2] = {{0.f, 0.13f * L, dark}, {0.5f * L, 0.f, dark}}; lathe(body, bi, q.p - f * (1.72f * L), -f, st, 2, 6, kLit, 0.f); }                // ferrão
            const float flap = 0.25f + 0.75f * std::sin(T * 75.f + float(i) * 2.f);   // asas batendo
            for (int s = -1; s <= 1; s += 2) { const glm::vec3 root = q.p + f * (0.35f * L) + up * (0.45f * L) + sd * (float(s) * 0.2f * L), out = glm::normalize(sd * float(s) * std::cos(flap) + up * (0.35f + std::sin(flap)) - f * 0.35f);
                const glm::vec3 w = glm::normalize(glm::cross(out, up) + glm::vec3(1e-5f)) * (0.5f * L), tip = root + out * (1.9f * L), mid = root + out * (1.05f * L);
                const unsigned k = put(wings, root, up, wingC, kUnlit); put(wings, mid + w, up, wingC, kUnlit); put(wings, tip, up, wingC, kUnlit); put(wings, mid - w, up, wingC, kUnlit); wi.insert(wi.end(), {k, k + 1, k + 2, k, k + 2, k + 3}); } }
    }
};

// ---------------- Cremoso: Modo Derretido e Poça de Calda ----------------
struct MeltFx : FxGeo {
    float k = 0.f, pk = 0.f, stick = 0.f, T = 0.f, size = 1.f, dist = 1.8f; glm::vec3 at{0.f}, fwd{0.f, 0.f, 1.f}, foeAt{0.f};
    void reset() { *this = MeltFx{}; }
    bool on() const { return k > 0.01f || pk > 0.01f || stick > 0.01f; }
    void tick(const Fighter& f, const Fighter& foe, const glm::vec3& feet, const glm::vec3& fw, const glm::vec3& foeFeet, float sz) {
        T += 1.f / 60.f; const bool mine = f.charId == 3, cast = mine && f.state == FIGHTER_SKILL;
        const bool melt = mine && (f.status.melt > 0 || (cast && f.activeSkill == SkillId::CremosoMeltMode && prog(f) > 0.15f));
        k += ((melt ? 1.f : 0.f) - k) * (melt ? 0.07f : 0.05f); if (k < 0.004f && !melt) k = 0.f;
        const bool pd = cast && f.activeSkill == SkillId::CremosoPuddleTrap; const float p = pd ? prog(f) : 0.f, want = pd ? smooth(p / 0.38f) * (1.f - smooth((p - 0.72f) / 0.28f)) : 0.f;
        pk += (want - pk) * 0.25f; if (pk < 0.004f && !pd) pk = 0.f;
        const bool st = mine && foe.status.mud > 0; stick += ((st ? 1.f : 0.f) - stick) * 0.08f; if (stick < 0.004f && !st) stick = 0.f;
        at = glm::vec3(feet.x, 0.f, feet.z); fwd = fw; foeAt = glm::vec3(foeFeet.x, 0.f, foeFeet.z); dist = glm::length(foeAt - at); size = sz;
    }
    float squash() const { return std::max(k, pk * 0.55f); }
    // o corpo achata e alarga; a cabeça e as luvas descem junto mas guardam o tamanho
    void apply(FighterPose& q) const {
        const float m = squash(); if (m <= 0.002f) return; const float wob = 1.f + 0.035f * m * std::sin(T * 6.5f), sx = (1.f + 0.24f * m) * wob, sy = (1.f - 0.5f * m) / wob;
        q.root = q.root * glm::scale(glm::mat4(1.f), glm::vec3(sx, sy, sx));
        const glm::vec3 keep(1.f / sx, 1.f / sy, 1.f / sx); q.headScale *= keep * glm::vec3(1.f + 0.05f * m, 1.f - 0.1f * m, 1.f + 0.05f * m); for (int i = 0; i < 2; i++) q.gloveScale[i] *= keep;
    }
    void build(V& v, I& i) const {
        if (!on()) return; const float y = Particles::kFloor + 0.004f; const glm::vec3 side = glm::normalize(glm::cross(glm::vec3(0, 1, 0), fwd) + glm::vec3(1e-5f, 0, 0));
        const glm::vec3 cream(0.99f, 0.78f, 0.13f), creamEdge(0.93f, 0.6f, 0.05f), syrup(0.86f, 0.47f, 0.03f), syrupEdge(0.62f, 0.27f, 0.02f);
        if (k > 0.01f) { const float R = size * (0.3f + 0.42f * k);   // derretido: o próprio corpo escorrendo em volta dos pés
            puddle(v, i, at + glm::vec3(0, y, 0), side * R, fwd * R, 0.05f * size * (0.4f + 0.6f * k), 0.34f * size * k, cream, creamEdge, T, 0.7f, 0.007f * size); }
        if (pk > 0.01f) { const float R = size * (0.28f + 0.3f * pk), reach = 0.5f * dist * pk;   // poça de calda: corre do Cremoso até debaixo do rival
            puddle(v, i, at + fwd * reach + glm::vec3(0, y + 0.002f, 0), side * R, fwd * (R + reach), 0.04f * size, 0.12f * size * pk, syrup, syrupEdge, T * 1.3f, 2.1f, 0.007f * size); }
        if (stick > 0.01f) { const float R = size * 0.46f * stick;   // a calda que prende os pés do rival
            puddle(v, i, foeAt + glm::vec3(0, y + 0.001f, 0), side * R, fwd * R, 0.04f * size, 0.1f * size * stick, syrup, syrupEdge, T * 1.2f, 4.4f, 0.007f * size); }
    }
};

// ---------------- Cremoso: Estocada de Casquinha ----------------
// O soco revestido de casquinha: a casquinha se forma em volta da luva direita enquanto o braço arma para trás (lascas girando para dentro),
// dispara até o rival, segura um instante e se estilhaça. Fica do lado do corpo, então dá para ver de trás.
struct ConeFx : FxGeo {
    int t = -1; bool was = false, done = false, broke = false; float form = 0.f, thrust = 0.f, gone = 0.f, size = 1.f, len = 1.f, T = 0.f; glm::vec3 base{0.f}, dir{0.f, 0.f, 1.f};
    static bool active(const Fighter& f) { return f.charId == 3 && f.state == FIGHTER_SKILL && f.activeSkill == SkillId::CremosoConePunch; }
    void reset() { *this = ConeFx{}; }
    bool on() const { return t >= 0 && form > 0.01f && gone < 0.99f; }
    static float formK(float p) { return smooth(p / 0.24f); }
    static float thrustK(float p) { return p < 0.3f ? 0.f : p < 0.45f ? std::sin((p - 0.3f) / 0.15f * 1.5708f) : p < 0.66f ? 1.f : 1.f - smooth((p - 0.66f) / 0.3f); }
    // o braço direito arma para trás e para cima, e desce estocando; o corpo vai junto
    static void pose(Fighter& view, const Fighter& f) {
        if (!active(f)) return; const float p = prog(f), w = smooth(p / 0.2f) * (1.f - smooth((p - 0.26f) / 0.1f)), k = thrustK(p), on = smooth(p / 0.1f) * (1.f - smooth((p - 0.9f) / 0.1f)); view.isLeftPunch = false;   // quem estoca é sempre a direita
        const float x = 30.f + 8.f * w - 14.f * k, y = 70.f + 16.f * w - 2.f * k, z = 10.f - 26.f * w + 62.f * k;
        view.gloveR.x += (x - view.gloveR.x) * on; view.gloveR.y += (y - view.gloveR.y) * on; view.gloveR.z += (z - view.gloveR.z) * on;
        view.gloveL.x += (-24.f - view.gloveL.x) * on; view.gloveL.y += (66.f - view.gloveL.y) * on; view.gloveL.z += (6.f - 8.f * k - view.gloveL.z) * on;
        view.offsetZ += -8.f * w + 14.f * k; view.bodySkew += 0.07f * w - 0.09f * k;
    }
    void tick(const Fighter& f, const glm::vec3& glove, const glm::vec3& fw, const glm::vec3& foeChest, float sz, Particles& P) {
        const bool in = active(f); if (in && !was) { t = 0; done = broke = false; gone = 0.f; } was = in; if (t < 0) return; if (!in) { t = -1; form = 0.f; return; }
        t++; T += 1.f / 60.f; const float p = prog(f); form = formK(p); thrust = thrustK(p); size = sz; gone = smooth((p - 0.66f) / 0.1f);
        const glm::vec3 d = foeChest - glove; const float L = glm::length(d); dir = L > 1e-3f ? glm::normalize(glm::mix(glm::normalize(fw + glm::vec3(0, 0.25f, 0)), d / L, smooth((p - 0.2f) / 0.15f))) : fw;
        base = glove - dir * (0.16f * sz); len = glm::mix(0.62f * sz, std::max(0.62f * sz, L + 0.1f * sz), thrust) * form;
        const glm::vec3 waf(0.87f, 0.62f, 0.29f), wafD(0.62f, 0.38f, 0.14f);
        if (p < 0.26f && t % 2 == 0) { const glm::vec3 o = P.dir() * (0.4f * sz); Particle q; q.kind = kFlake; q.p = glove + o; q.v = -o / 11.f; q.size = P.rnd(0.025f, 0.045f); q.life = 11; q.noFloor = true; q.drag = 1.f; q.c0 = q.c1 = glm::vec4(t % 4 ? waf : wafD, 1.f); P.emit(q); }   // lascas juntando na luva
        const glm::vec3 tip = base + dir * len;
        if (f.skillEventDone && !done) { done = true;   // o acerto: lascas e riscos
            for (int i = 0; i < 14; i++) { Particle q; q.kind = kFlake; q.p = tip + P.dir() * 0.06f; q.v = P.dir() * P.rnd(0.03f, 0.08f) + glm::vec3(0, 0.02f, 0) - dir * 0.02f; q.size = P.rnd(0.03f, 0.06f); q.life = P.rnd(26, 42); q.gravity = 0.003f; q.drag = 0.97f; q.c0 = q.c1 = glm::vec4(i % 3 ? waf : wafD, 1.f); P.emit(q); }
            for (int i = 0; i < 10; i++) { Particle q; q.kind = kLine; q.p = tip; q.v = P.dir() * P.rnd(0.06f, 0.11f); q.dir = q.v; q.len = 0.2f; q.size = 0.045f; q.life = P.rnd(9, 14); q.drag = 0.86f; q.noFloor = true; q.c0 = q.c1 = glm::vec4(1.f, 0.9f, 0.6f, 1.f); P.emit(q); } }
        if (p >= 0.66f && !broke) { broke = true;   // a casquinha se estilhaça inteira, ao longo do comprimento
            for (int i = 0; i < 34; i++) { const float u = P.rnd(0.f, 1.f); Particle q; q.kind = kFlake; q.p = base + dir * (len * u) + P.dir() * (0.12f * sz * (1.f - u)); q.v = P.dir() * P.rnd(0.02f, 0.06f) + glm::vec3(0, 0.03f, 0); q.size = P.rnd(0.035f, 0.075f); q.life = P.rnd(30, 50); q.gravity = 0.0035f; q.drag = 0.975f; q.c0 = q.c1 = glm::vec4(i % 3 ? waf : wafD, 1.f); P.emit(q); } }
    }
    void build(V& v, I& i) const {
        if (!on()) return; const glm::vec3 waf(0.87f, 0.62f, 0.29f), wafLite(0.95f, 0.74f, 0.4f), groove(0.6f, 0.36f, 0.13f); const float keep = 1.f - gone, R = 0.21f * size * (0.3f + 0.7f * form) * keep, L = len * keep;
        const float tw = T * 3.f * (1.f - form);   // gira enquanto se forma
        { const Ring r[7] = {{-0.05f * size, R * 0.86f, waf}, {0.f, R * 1.1f, wafLite}, {0.05f * size, R, waf}, {L * 0.3f, R * 0.71f, waf}, {L * 0.6f, R * 0.41f, waf}, {L * 0.85f, R * 0.16f, waf}, {L, 0.f, waf}}; lathe(v, i, base, dir, r, 7, 18, kLit, 0.008f * size); }
        // o xadrez da casquinha: frisos em espiral nos dois sentidos, um pouco acima da casca
        glm::vec3 s, up; basis(dir, s, up); constexpr int kTurns = 7, kSeg = 10; const float x0 = 0.06f * size;
        for (int h = 0; h < kTurns * 2; h++) { const float a0 = 6.28318f * float(h / 2) / float(kTurns) + tw, sgn = h % 2 ? 1.f : -1.f;
            for (int q = 0; q < kSeg; q++) { const auto at = [&](float u2, float off) { const float al = x0 + (L - x0) * u2, rr = R * (1.f - al / std::max(L, 1e-3f)) + 0.004f * size, a = a0 + sgn * u2 * 2.4f + off / std::max(rr, 0.02f); return base + dir * al + (s * std::cos(a) + up * std::sin(a)) * rr; };
                const float u0 = float(q) / kSeg, u1 = float(q + 1) / kSeg, w0 = 0.011f * size * (1.f - u0 * 0.7f), w1 = 0.011f * size * (1.f - u1 * 0.7f);
                quad(v, i, at(u0, -w0), at(u0, w0), at(u1, w1), at(u1, -w1), groove, kLit); } }
    }
};

// ---------------- Marcov: Escudo Sputnik ----------------
struct BubbleFx : FxGeo {
    float a = 0.f, T = 0.f, hitK = 0.f, hp = -1.f, size = 1.f; glm::vec3 c{0.f};
    void reset() { *this = BubbleFx{}; }
    bool on() const { return a > 0.01f; }
    void tick(const Fighter& f, const glm::vec3& feet, float sz) {
        T += 1.f / 60.f; const bool want = f.charId == 7 && (f.status.plasma > 0 || (f.state == FIGHTER_SKILL && f.activeSkill == SkillId::MarcovPlasmaBarrier && prog(f) > 0.2f));
        a += ((want ? 1.f : 0.f) - a) * (want ? 0.12f : 0.08f); if (a < 0.004f && !want) a = 0.f;
        if (want && hp >= 0.f && f.hp < hp - 0.5f) hitK = 1.f; hp = f.hp; hitK *= 0.9f;   // o golpe que o escudo segura acende a malha
        size = sz; c = feet + glm::vec3(0, 0.95f * sz, 0);
    }
    float fillAlpha() const { return (0.12f + 0.2f * hitK) * a; }
    float lineAlpha() const { return std::min(1.f, 0.72f + 0.3f * hitK) * a; }
    // fill: a casca azul (bem translúcida); lines: a malha de hexágonos (translúcida); glow: o brilho da borda
    void build(const glm::vec3& eye, V& fill, I& fi, V& lines, I& li, V& glowV, I& gi) const {
        if (!on()) return; const float R = 0.98f * size * (0.82f + 0.18f * a); const glm::vec3 cA(0.22f, 0.74f, 0.97f), cB(0.78f, 0.93f, 1.f); const glm::vec3 view = glm::normalize(eye - c);
        ball(fill, fi, c, glm::vec3(R, 0, 0), glm::vec3(0, R, 0), glm::vec3(0, 0, R), cA, kUnlit, 0.f, 8, 14);
        // borda acesa: mais forte onde a casca fica de lado para a câmera
        { constexpr int NL = 10, NJ = 18; const unsigned base = unsigned(glowV.size() / 12);
          for (int r = 0; r <= NL; r++) { const float th = 3.14159f * float(r) / NL; for (int j = 0; j < NJ; j++) { const float ph = 6.28318f * float(j) / NJ; const glm::vec3 n(std::sin(th) * std::cos(ph), std::cos(th), std::sin(th) * std::sin(ph));
              const float fr = 1.f - std::fabs(glm::dot(n, glm::normalize(eye - (c + n * R)))); put(glowV, c + n * R, n, cB * ((0.04f + 0.42f * fr * fr * fr + 0.12f * hitK) * a), kUnlit); } }
          for (int r = 0; r < NL; r++) for (int j = 0; j < NJ; j++) { const unsigned p0 = base + unsigned(r * NJ + j), p1 = base + unsigned(r * NJ + (j + 1) % NJ), p2 = p0 + NJ, p3 = p1 + NJ; gi.insert(gi.end(), {p0, p2, p1, p1, p2, p3}); } }
        // malha: fileiras de hexágonos deitados na casca, só do lado da câmera, girando devagar e pulsando
        constexpr int kRows = 8; const float dLat = 0.36f;
        for (int row = 0; row < kRows; row++) { const float lat = -1.05f + dLat * float(row), cl = std::cos(lat); const int n = std::max(5, int(std::round(17.f * cl)));
            for (int j = 0; j < n; j++) { const float lon = 6.28318f * (float(j) + (row % 2 ? 0.5f : 0.f)) / float(n) + T * 0.22f; const glm::vec3 d(cl * std::cos(lon), std::sin(lat), cl * std::sin(lon)); const float face = glm::dot(d, view); if (face < -0.05f) continue;
                const glm::vec3 e1 = glm::normalize(glm::cross(glm::vec3(0, 1, 0), d) + glm::vec3(1e-5f, 0, 0)), e2 = glm::cross(d, e1); const float hr = 0.205f * R, pulse = 0.8f + 0.2f * std::sin(T * 5.f + float(row) * 0.9f + float(j) * 0.5f), in = 1.f - 0.17f * pulse - 0.1f * hitK;
                const glm::vec3 col = glm::mix(cA, cB, sat(1.25f - face) * 0.8f + 0.2f * pulse); const unsigned base = unsigned(lines.size() / 12);
                for (int s = 0; s < 6; s++) { const float g = 6.28318f * float(s) / 6.f + 0.5236f; const glm::vec3 o = e1 * std::cos(g) + e2 * std::sin(g); put(lines, c + glm::normalize(d + o * (hr / R)) * R, d, col, kUnlit); put(lines, c + glm::normalize(d + o * (hr * in / R)) * R, d, col, kUnlit); }
                for (int s = 0; s < 6; s++) { const unsigned i0 = base + unsigned(s * 2), i1 = i0 + 1, i2 = base + unsigned(((s + 1) % 6) * 2), i3 = i2 + 1; li.insert(li.end(), {i0, i1, i2, i1, i3, i2}); } } }
    }
};

// ---------------- Marcov: Raio Orbital Sputnik ----------------
struct LaserFx : FxGeo {
    int t = -1, beam = 0, away = 0; bool was = false, fired = false; float p = 0.f, T = 0.f, size = 1.f; glm::vec3 target{0.f}, side{1.f, 0.f, 0.f};
    static constexpr int kBeam = 26, kAway = 24;
    static bool active(const Fighter& f) { return f.charId == 7 && f.state == FIGHTER_SKILL && f.activeSkill == SkillId::MarcovOrbitalLaser; }
    void reset() { *this = LaserFx{}; }
    bool on() const { return t >= 0; }
    void tick(const Fighter& f, const glm::vec3& foeFeet, const glm::vec3& fw, float sz, Particles& P) {
        const bool in = active(f); if (in && !was) { t = 0; fired = false; beam = 0; away = 0; target = glm::vec3(foeFeet.x, 0.f, foeFeet.z); } was = in; if (t < 0) return;
        t++; T += 1.f / 60.f; size = sz; side = glm::normalize(glm::cross(glm::vec3(0, 1, 0), fw) + glm::vec3(1e-5f, 0, 0));
        if (in) { p = prog(f); if (!fired) target += (glm::vec3(foeFeet.x, 0.f, foeFeet.z) - target) * 0.25f; }   // a mira persegue o rival até o disparo
        if (in && f.skillEventDone && !fired) { fired = true; beam = kBeam; const glm::vec3 ice(0.73f, 0.9f, 0.99f), pl(0.22f, 0.74f, 0.97f);
            for (int i = 0; i < 30; i++) { const float a = float(i) / 30.f * 6.2832f; const glm::vec3 r(std::cos(a), 0, std::sin(a)); Particle q; q.kind = kLine; q.p = target + glm::vec3(0, 0.06f, 0) + r * 0.2f; q.v = r * P.rnd(0.08f, 0.13f); q.dir = r; q.len = 0.3f; q.size = P.rnd(0.05f, 0.08f); q.life = P.rnd(12, 18); q.drag = 0.9f; q.noFloor = true; q.c0 = q.c1 = glm::vec4(i % 2 ? ice : pl, 1.f); P.emit(q); }
            for (int i = 0; i < 16; i++) { Particle q; q.kind = kGlow; q.p = target + glm::vec3(P.rnd(-0.2f, 0.2f), P.rnd(0.1f, 1.2f), P.rnd(-0.2f, 0.2f)); q.v = P.dir() * P.rnd(0.02f, 0.05f) + glm::vec3(0, 0.02f, 0); q.size = P.rnd(0.05f, 0.09f); q.life = P.rnd(18, 30); q.noFloor = true; q.drag = 0.95f; q.c0 = glm::vec4(ice, 1.f); q.c1 = glm::vec4(pl, 1.f); P.emit(q); } }
        if (beam > 0) { beam--; if (beam % 2 == 0) { Particle s; s.kind = kSpark; s.p = target + glm::vec3(0, 0.1f, 0); s.v = glm::vec3(P.rnd(-0.07f, 0.07f), P.rnd(0.03f, 0.09f), P.rnd(-0.07f, 0.07f)); s.size = 0.02f; s.life = 30; s.gravity = 0.003f; s.drag = 0.985f; s.c0 = s.c1 = glm::vec4(0.73f, 0.9f, 0.99f, 1.f); P.emit(s); } }
        if (!in && beam == 0) { if (++away > kAway) t = -1; }   // o satélite vai embora
        if (!in && !fired && away > 6) t = -1;
    }
    glm::vec3 satPos() const { const float down = smooth(float(t) / 16.f), up = smooth(float(away) / float(kAway)); return target + glm::vec3(0, (2.75f + 2.6f * (1.f - down) + 3.f * up * up) * size, 0) + side * (0.05f * std::sin(T * 2.f) * size); }
    void build(V& v, I& i, V& gv, I& gi) const {
        if (!on()) return; const glm::vec3 sp = satPos(), up(0, 1, 0), fw = glm::cross(side, up); const float s = size, ink = 0.008f * s;
        const glm::vec3 metal(0.8f, 0.84f, 0.88f), panel(0.13f, 0.25f, 0.6f), cell(0.38f, 0.65f, 0.98f), darkC(0.08f, 0.1f, 0.08f), red(0.94f, 0.2f, 0.2f), ice(0.73f, 0.9f, 0.99f), pl(0.22f, 0.74f, 0.97f), deep(0.01f, 0.41f, 0.63f);
        // o Sputnik: esfera de metal, a luz vermelha embaixo, os dois painéis azuis e as quatro antenas para trás
        ball(v, i, sp, side * (0.17f * s), up * (0.17f * s), fw * (0.17f * s), metal, kLit, ink, 7, 12);
        ball(v, i, sp - up * (0.15f * s), side * (0.055f * s), up * (0.055f * s), fw * (0.055f * s), red * (0.75f + 0.25f * std::sin(T * 14.f)), kUnlit, 0.f, 5, 8);
        for (int sd = -1; sd <= 1; sd += 2) { const glm::vec3 pc = sp + side * (float(sd) * 0.37f * s); box(v, i, pc, side * (0.18f * s), up * (0.1f * s), fw * (0.012f * s), panel, kLit, ink * 0.75f);
            for (int k = -1; k <= 1; k++) for (int fz = -1; fz <= 1; fz += 2) { const glm::vec3 lc = pc + side * (float(k) * 0.09f * s) + fw * (float(fz) * 0.014f * s); quad(v, i, lc - side * (0.005f * s) - up * (0.09f * s), lc + side * (0.005f * s) - up * (0.09f * s), lc + side * (0.005f * s) + up * (0.09f * s), lc - side * (0.005f * s) + up * (0.09f * s), cell, kUnlit); } }
        for (int k = 0; k < 4; k++) { const float a = 0.785f + 1.5708f * float(k); const glm::vec3 d = glm::normalize(up * 0.75f + (side * std::cos(a) + fw * std::sin(a)) * 0.9f); const Ring r[2] = {{0.f, 0.009f * s, darkC}, {0.4f * s, 0.005f * s, darkC}}; lathe(v, i, sp + d * (0.15f * s), d, r, 2, 5, kLit, 0.f); }
        // a mira no chão: dois aros vermelhos e quatro traços, fechando até o disparo
        if (!fired || beam > 0) { const float y = Particles::kFloor + 0.01f, k = fired ? 1.f : 1.f + 1.1f * (1.f - smooth(p / 0.5f)), blink = fired ? 1.f : 0.65f + 0.35f * std::sin(T * 22.f); const glm::vec3 c = target + glm::vec3(0, y, 0), col = red * blink; constexpr int N = 40;
            for (const float R0 : {0.68f, 0.37f}) { const float R = R0 * s * k, w = 0.03f * s; const unsigned b = unsigned(v.size() / 12);
                for (int j = 0; j < N; j++) { const float a = 6.28318f * float(j) / N + T * (R0 > 0.5f ? 0.8f : -1.1f); const glm::vec3 r(std::cos(a), 0, std::sin(a)); put(v, c + r * (R - w), up, col, kUnlit); put(v, c + r * (R + w), up, col, kUnlit); }
                for (int j = 0; j < N; j++) { const unsigned i0 = b + unsigned(j * 2), i1 = i0 + 1, i2 = b + unsigned(((j + 1) % N) * 2), i3 = i2 + 1; i.insert(i.end(), {i0, i2, i1, i1, i2, i3}); } }
            for (int j = 0; j < 4; j++) { const float a = 1.5708f * float(j) + T * 0.8f; const glm::vec3 r(std::cos(a), 0, std::sin(a)), tn(-r.z, 0, r.x); const float r0 = 0.52f * s * k, r1 = 0.86f * s * k, w = 0.03f * s; quad(v, i, c + r * r0 - tn * w, c + r * r1 - tn * w, c + r * r1 + tn * w, c + r * r0 + tn * w, col, kUnlit); } }
        // a carga embaixo do satélite e o feixe: três tubos de luz, o de dentro branco, e o clarão no chão
        const float ch = fired ? 0.f : smooth((p - 0.15f) / 0.35f);
        if (ch > 0.01f) { const float r = (0.04f + 0.13f * ch) * s * (0.9f + 0.1f * std::sin(T * 40.f)); ball(gv, gi, sp - up * (0.24f * s), side * r, up * r, fw * r, pl * 0.8f, kUnlit, 0.f, 6, 10); ball(gv, gi, sp - up * (0.24f * s), side * (r * 0.5f), up * (r * 0.5f), fw * (r * 0.5f), glm::vec3(0.8f), kUnlit, 0.f, 5, 8); }
        if (beam > 0) { const float w = std::min(1.f, float(beam) / 7.f) * std::min(1.f, float(kBeam - beam + 1) / 3.f) * (0.85f + 0.15f * std::sin(T * 60.f)), H = sp.y - 0.2f * s; const glm::vec3 foot(target.x, Particles::kFloor + 0.01f, target.z);
            const struct { glm::vec3 c; float k; } layer[3] = {{deep * 0.6f, 1.f}, {pl * 0.75f, 0.62f}, {glm::vec3(0.85f), 0.28f}};
            for (const auto& ly : layer) { const Ring r[2] = {{0.f, 0.42f * s * ly.k * w, ly.c}, {H, 0.1f * s * ly.k * w, ly.c}}; lathe(gv, gi, foot, up, r, 2, 20, kUnlit, 0.f); }
            { const unsigned b = put(gv, foot, up, ice * (0.8f * w), kUnlit); constexpr int N = 28; for (int j = 0; j < N; j++) { const float a = 6.28318f * float(j) / N; put(gv, foot + glm::vec3(std::cos(a), 0, std::sin(a)) * (0.75f * s * w), up, glm::vec3(0.f), kUnlit); } for (int j = 0; j < N; j++) gi.insert(gi.end(), {b, b + 1 + unsigned(j), b + 1 + unsigned((j + 1) % N)}); } }
    }
};

// ---------------- Marcov: turbinas das costas ----------------
struct JetFx : FxGeo {
    float pw = 0.f, T = 0.f, size = 1.f; glm::vec3 chest{0.f}, fwd{0.f, 0.f, 1.f};
    static bool active(const Fighter& f) { return f.charId == 7 && f.state == FIGHTER_SKILL && (f.activeSkill == SkillId::MarcovRocketThrust || f.activeSkill == SkillId::MarcovKineticOverdrive); }
    void reset() { *this = JetFx{}; }
    bool on() const { return pw > 0.02f; }
    void tick(const Fighter& f, const glm::vec3& ch, const glm::vec3& fw, float sz, Particles& P) {
        T += 1.f / 60.f; const bool in = active(f); const float p = in ? prog(f) : 0.f, want = in ? smooth(p / 0.12f) * (1.f - smooth((p - 0.62f) / 0.3f)) : 0.f;
        pw += (want - pw) * 0.3f; if (pw < 0.01f && !in) pw = 0.f; chest = ch; fwd = fw; size = sz;
        if (pw > 0.3f) { const glm::vec3 sd = glm::normalize(glm::cross(glm::vec3(0, 1, 0), fw) + glm::vec3(1e-5f, 0, 0));
            for (int s = -1; s <= 1; s += 2) { const glm::vec3 at = nozzle(sd, s) - fw * (0.45f * sz * pw);
                Particle q; q.kind = kGlow; q.p = at + P.dir() * 0.03f; q.v = -fw * P.rnd(0.03f, 0.06f) + P.dir() * 0.008f - glm::vec3(0, 0.008f, 0); q.size = P.rnd(0.03f, 0.05f); q.life = P.rnd(10, 16); q.noFloor = true; q.drag = 0.96f; const bool hot = P.rnd(0.f, 1.f) < 0.5f; q.c0 = glm::vec4(hot ? glm::vec3(0.98f, 0.45f, 0.09f) : glm::vec3(0.22f, 0.74f, 0.97f), 1.f); q.c1 = q.c0; P.emit(q);
                if (P.rnd(0.f, 1.f) < 0.35f) { Particle sm; sm.kind = kPuff; sm.p = at - glm::vec3(0, 0.08f, 0); sm.v = -fw * P.rnd(0.01f, 0.025f) + glm::vec3(0, 0.004f, 0); sm.size = P.rnd(0.07f, 0.12f); sm.life = P.rnd(26, 40); sm.grow = 0.003f; sm.c0 = glm::vec4(0.62f, 0.64f, 0.68f, 1.f); sm.c1 = glm::vec4(0.86f, 0.84f, 0.8f, 1.f); P.emit(sm); } } }
    }
    glm::vec3 nozzle(const glm::vec3& sd, int s) const { return chest - fwd * (0.27f * size) + sd * (float(s) * 0.19f * size) + glm::vec3(0, 0.04f * size, 0); }
    // chama de quadrinho: azul com contorno, miolo branco, a ponta tremendo
    void build(V& v, I& i) const {
        if (!on()) return; const glm::vec3 sd = glm::normalize(glm::cross(glm::vec3(0, 1, 0), fwd) + glm::vec3(1e-5f, 0, 0)), blue(0.22f, 0.74f, 0.97f), lite(0.73f, 0.9f, 0.99f), white(1.f);
        for (int s = -1; s <= 1; s += 2) { const float fl = 0.82f + 0.18f * std::sin(T * 48.f + float(s) * 1.3f), L = 0.58f * size * pw * fl, R = 0.085f * size * (0.6f + 0.4f * pw);
            const glm::vec3 d = glm::normalize(-fwd - glm::vec3(0, 0.22f, 0) + sd * (0.05f * std::sin(T * 31.f + float(s)))), at = nozzle(sd, s);
            const Ring r[6] = {{0.f, R * 0.7f, white}, {L * 0.14f, R, white}, {L * 0.3f, R * 0.92f, lite}, {L * 0.55f, R * 0.66f, blue}, {L * 0.85f, R * 0.26f, blue}, {L, 0.f, blue}}; lathe(v, i, at, d, r, 6, 12, kUnlit, 0.007f * size); }
    }
};

// ---------------- Cremoso: Explosão de Calda Quente ----------------
// A calda ferve nas duas luvas, sai em dois jorros grossos que fazem um arco até o rival e escorre dele depois do acerto.
struct SyrupFx : FxGeo {
    int t = -1; bool was = false, done = false; float p = 0.f, T = 0.f, size = 1.f; glm::vec3 hand[2]{}, target{0.f}, side{1.f, 0.f, 0.f};
    static bool active(const Fighter& f) { return f.charId == 3 && f.state == FIGHTER_SKILL && f.activeSkill == SkillId::CremosoSyrupBlast; }
    void reset() { *this = SyrupFx{}; }
    bool on() const { return t >= 0; }
    float head() const { return sat((p - 0.2f) / 0.25f); }
    float tail() const { return smooth((p - 0.52f) / 0.33f); }
    glm::vec3 at(int s, float q) const { const glm::vec3 a = hand[s], b = target + side * ((s ? 1.f : -1.f) * 0.1f * size); return glm::mix(a, b, q) + glm::vec3(0, std::sin(q * 3.14159f) * 0.34f * size, 0); }
    void tick(const Fighter& f, const glm::vec3& hL, const glm::vec3& hR, const glm::vec3& fw, const glm::vec3& foeChest, float sz, Particles& P) {
        const bool in = active(f); if (in && !was) { t = 0; done = false; } was = in; if (t < 0) return; if (!in) { t = -1; return; }
        t++; T += 1.f / 60.f; p = prog(f); size = sz; hand[0] = hL + fw * (0.1f * sz); hand[1] = hR + fw * (0.1f * sz); target = foeChest; side = glm::normalize(glm::cross(glm::vec3(0, 1, 0), fw) + glm::vec3(1e-5f, 0, 0));
        const glm::vec3 hot(0.98f, 0.62f, 0.08f), syr(0.82f, 0.42f, 0.03f);
        if (t % 3 == 0 && p < 0.8f) { const int s = (t / 3) % 2; const float q = p < 0.2f ? 0.f : P.rnd(tail(), head()); Particle st; st.kind = kPuff; st.p = at(s, q) + glm::vec3(0, 0.06f, 0); st.v = glm::vec3(P.rnd(-0.004f, 0.004f), P.rnd(0.01f, 0.02f), P.rnd(-0.004f, 0.004f)); st.size = P.rnd(0.04f, 0.07f); st.grow = 0.002f; st.life = P.rnd(22, 34); st.noFloor = true; st.c0 = glm::vec4(1.f, 0.97f, 0.9f, 1.f); st.c1 = glm::vec4(0.95f, 0.9f, 0.8f, 1.f); P.emit(st); }   // o vapor da calda quente
        if (p >= 0.2f && p < 0.75f && t % 2 == 0) { const int s = (t / 2) % 2; Particle d; d.kind = kEnergy; d.p = at(s, P.rnd(tail(), head())); d.v = glm::vec3(P.rnd(-0.008f, 0.008f), -0.004f, P.rnd(-0.008f, 0.008f)); d.size = P.rnd(0.025f, 0.045f); d.gravity = 0.0025f; d.life = P.rnd(20, 30); d.drag = 0.99f; d.c0 = glm::vec4(hot, 1.f); d.c1 = glm::vec4(syr, 1.f); P.emit(d); }   // pingos caindo do jorro
        if (f.skillEventDone && !done) { done = true;   // o acerto: a calda espirra
            for (int i = 0; i < 26; i++) { Particle q; q.kind = kEnergy; q.p = target + P.dir() * 0.08f; q.v = P.dir() * P.rnd(0.03f, 0.08f) + glm::vec3(0, 0.03f, 0); q.size = P.rnd(0.04f, 0.09f); q.gravity = 0.0035f; q.life = P.rnd(26, 44); q.drag = 0.97f; q.c0 = glm::vec4(hot, 1.f); q.c1 = glm::vec4(syr, 1.f); P.emit(q); } }
        if (done && p < 0.95f && t % 3 == 0) { Particle d; d.kind = kEnergy; d.p = target + glm::vec3(P.rnd(-0.22f, 0.22f), P.rnd(-0.3f, 0.15f), P.rnd(-0.12f, 0.12f)) * sz; d.v = glm::vec3(0, -0.003f, 0); d.size = P.rnd(0.03f, 0.05f); d.gravity = 0.0016f; d.life = 34; d.drag = 0.99f; d.c0 = glm::vec4(hot, 1.f); d.c1 = glm::vec4(syr, 1.f); P.emit(d); }   // escorrendo do rival
    }
    void build(V& v, I& i) const {
        if (!on()) return; const glm::vec3 syr(0.84f, 0.45f, 0.03f), lite(0.98f, 0.66f, 0.12f); const float ink = 0.007f * size, h = head(), tl = tail();
        const auto blob = [&](const glm::vec3& c, float r, const glm::vec3& col) { ball(v, i, c, glm::vec3(r, 0, 0), glm::vec3(0, r, 0), glm::vec3(0, 0, r), col, kLit, ink, 5, 8); };
        for (int s = 0; s < 2; s++) {
            const float boil = smooth(p / 0.18f) * (1.f - smooth((p - 0.6f) / 0.2f));   // a bola de calda fervendo na luva
            if (boil > 0.02f) { const float r = (0.1f + 0.02f * std::sin(T * 22.f + float(s) * 2.f)) * size * boil; blob(hand[s], r, syr); blob(hand[s] + glm::vec3(0.4f * r, 0.75f * r * (0.8f + 0.3f * std::sin(T * 17.f + float(s))), 0.f), r * 0.42f, lite); }
            if (h <= tl + 0.01f || p < 0.2f) continue; constexpr int N = 15;
            for (int k = 0; k < N; k++) { const float u = float(k) / float(N - 1), q = tl + (h - tl) * u, r = (k == N - 1 ? 0.095f : 0.058f + 0.018f * std::sin(u * 9.f + T * 20.f + float(s))) * size * (0.55f + 0.45f * smooth(u * 4.f + 0.3f)); blob(at(s, q), r, k % 4 == 3 ? lite : syr); } }
    }
};

// ---------------- Valéria: Escudo de Calor ----------------
// Enquanto o modo dura: um aro de brasa pulsando no chão, labaredas subindo em volta do corpo e riscos de ar quente.
struct HeatFx : FxGeo {
    float k = 0.f, T = 0.f, size = 1.f; glm::vec3 at{0.f}; int n = 0;
    void reset() { *this = HeatFx{}; }
    bool on() const { return k > 0.01f; }
    void tick(const Fighter& f, const glm::vec3& feet, float sz, Particles& P) {
        T += 1.f / 60.f; n++; const bool want = f.charId == 4 && (f.status.heatwave > 0 || (f.state == FIGHTER_SKILL && f.activeSkill == SkillId::ValeriaHeatwaveGuard && prog(f) > 0.2f));
        k += ((want ? 1.f : 0.f) - k) * (want ? 0.1f : 0.06f); if (k < 0.004f && !want) k = 0.f; at = glm::vec3(feet.x, 0.f, feet.z); size = sz; if (k < 0.3f) return;
        if (n % 2 == 0) { const float a = P.rnd(0.f, 6.2832f), r = P.rnd(0.42f, 0.56f) * sz; Particle q; q.kind = kFire; q.p = at + glm::vec3(std::cos(a) * r, 0.05f, std::sin(a) * r); q.v = glm::vec3(0, P.rnd(0.018f, 0.034f), 0); q.size = P.rnd(0.07f, 0.12f); q.life = P.rnd(22, 34); q.gravity = -0.0004f; q.drag = 0.99f; q.noFloor = true; q.c0 = glm::vec4(1.f); q.c1 = glm::vec4(1.f); P.emit(q); }
        if (n % 5 == 0) { const float a = P.rnd(0.f, 6.2832f), r = P.rnd(0.5f, 0.7f) * sz; Particle l; l.kind = kLine; l.p = at + glm::vec3(std::cos(a) * r, P.rnd(0.2f, 0.9f) * sz, std::sin(a) * r); l.v = glm::vec3(0, 0.028f, 0); l.dir = glm::vec3(0, 1, 0); l.len = P.rnd(0.18f, 0.3f); l.size = 0.03f; l.life = P.rnd(14, 20); l.noFloor = true; l.drag = 1.f; l.c0 = l.c1 = glm::vec4(1.f, 0.8f, 0.35f, 1.f); P.emit(l); }
    }
    void build(V& v, I& i, V& gv, I& gi) const {
        if (!on()) return; const glm::vec3 c = at + glm::vec3(0, Particles::kFloor + 0.008f, 0), up(0, 1, 0), ember(1.f, 0.42f, 0.06f), core(1.f, 0.8f, 0.25f); constexpr int N = 44; const float pulse = 0.9f + 0.1f * std::sin(T * 7.f), R = 0.66f * size * k * pulse;
        // aro de brasa com a borda de dentro serrilhada (labareda deitada)
        { const unsigned b = unsigned(v.size() / 12); for (int j = 0; j < N; j++) { const float a = 6.28318f * float(j) / N + T * 0.6f, tooth = j % 2 ? 0.8f : 0.92f - 0.06f * std::sin(T * 11.f + float(j)); const glm::vec3 r(std::cos(a), 0, std::sin(a)); put(v, c + r * (R * tooth), up, core, kUnlit); put(v, c + r * R, up, ember, kUnlit); }
          for (int j = 0; j < N; j++) { const unsigned i0 = b + unsigned(j * 2), i1 = i0 + 1, i2 = b + unsigned(((j + 1) % N) * 2), i3 = i2 + 1; i.insert(i.end(), {i0, i2, i1, i1, i2, i3}); } }
        { const unsigned b = put(gv, c, up, ember * (0.22f * k), kUnlit); for (int j = 0; j < N; j++) { const float a = 6.28318f * float(j) / N; put(gv, c + glm::vec3(std::cos(a), 0, std::sin(a)) * (R * 1.25f), up, glm::vec3(0.f), kUnlit); } for (int j = 0; j < N; j++) gi.insert(gi.end(), {b, b + 1 + unsigned(j), b + 1 + unsigned((j + 1) % N)}); }
    }
};

// ---------------- Nocaute final ----------------
// O golpe que encerra a luta arremessa quem perdeu: vai pelo ar deitado para trás, com os pés para cima, até bater nas cordas.
// Se o golpe foi um especial, ele sai girando para o céu, some de tão longe e vira uma estrelinha que pisca.
struct KoFx : FxGeo {
    static constexpr float kFly = 54.f, kStar = 44.f;   // quadros do voo para o céu e da estrela
    int mode = 0; float t = 0.f, u = 0.f, r0 = 125.f, size = 1.f; glm::vec3 from{0.f}, to{0.f}, side{1.f, 0.f, 0.f}; bool justStar = false;
    void reset() { *this = KoFx{}; }
    bool blast() const { return mode == 2; }
    bool on() const { return mode != 0; }
    // from: os pés de quem perdeu; eye: a câmera (o voo para o céu vai para longe dela, para ficar na tela)
    void start(bool bySkill, const Fighter& f, const glm::vec3& feet, const glm::vec3& eye, float sz) {
        reset(); mode = bySkill ? 2 : 1; r0 = f.radius; size = sz; from = feet;
        glm::vec3 d(feet.x - eye.x, 0.f, feet.z - eye.z); const float L = glm::length(d); d = L > 0.05f ? d / L : glm::vec3(0, 0, -1);
        const float yaw = f.isPlayer ? 0.36f : 0.26f;   /* um pouco para o lado: no meio do alto da tela fica o relógio do round */ const glm::vec3 dir(d.x * std::cos(yaw) - d.z * std::sin(yaw), 0.f, d.x * std::sin(yaw) + d.z * std::cos(yaw));
        to = feet + dir * 30.f + glm::vec3(0, 17.f, 0); side = glm::normalize(glm::cross(glm::vec3(0, 1, 0), dir));
    }
    float flyK() const { return std::pow(sat(t / kFly), 1.7f); }
    // a câmera acompanha o voo e a estrela, e volta para o ringue no fim
    float lookK() const { return mode == 2 ? smooth(t / 18.f) * (1.f - smooth((t - (kFly + kStar - 8.f)) / 14.f)) : 0.f; }
    // mira abaixo dele: quem voa (e depois a estrela) fica no alto da tela, fora do letreiro do resultado que ocupa o meio
    glm::vec3 lookAt() const { const float e = flyK(); return (t >= kFly ? to : pos() + glm::vec3(0, 0.85f * size * (1.f - 0.78f * e), 0)) - glm::vec3(0, 8.f * e, 0); }
    glm::vec3 pos() const { const float e = flyK(); return from + (to - from) * e + glm::vec3(0, 1.3f * std::sin(e * 3.14159f), 0); }
    void tick(const Fighter& f, Particles& P) {
        justStar = false; if (mode == 0) return;
        if (f.state != FIGHTER_KNOCKDOWN) { reset(); return; }   // a luta recomeçou
        if (mode == 1) { u = sat((f.radius - r0) / std::max(1.f, 232.f - r0)); return; }
        const float t0 = t; t += 1.f; if (t0 < kFly && t >= kFly) justStar = true;
        if (t0 == 0.f) for (int i = 0; i < 26; i++) { const float a = float(i) / 26.f * 6.2832f; const glm::vec3 r(std::cos(a), 0, std::sin(a)); Particle q; q.kind = kLine; q.p = from + glm::vec3(0, 0.1f, 0) + r * 0.25f; q.v = r * P.rnd(0.08f, 0.13f); q.dir = r; q.len = 0.35f; q.size = 0.06f; q.life = P.rnd(12, 18); q.drag = 0.9f; q.noFloor = true; q.c0 = q.c1 = glm::vec4(1.f, 0.95f, 0.7f, 1.f); P.emit(q); }
        if (t < kFly) { const float k = 1.f - 0.78f * flyK(); const glm::vec3 c = pos() + glm::vec3(0, 0.85f * size * k, 0);   // rastro de fumaça e riscos
            Particle s; s.kind = kPuff; s.p = c + P.dir() * (0.12f * k); s.v = P.dir() * 0.006f; s.size = P.rnd(0.14f, 0.22f) * (0.5f + 0.5f * k); s.grow = 0.004f; s.life = P.rnd(26, 40); s.noFloor = true; s.c0 = glm::vec4(1.f, 0.98f, 0.92f, 1.f); s.c1 = glm::vec4(0.88f, 0.86f, 0.82f, 1.f); P.emit(s);
            if (int(t) % 2 == 0) { const glm::vec3 d = glm::normalize(to - from); Particle l; l.kind = kLine; l.p = c - d * 0.3f + P.dir() * (0.25f * k); l.v = glm::vec3(0.f); l.dir = d; l.len = P.rnd(0.5f, 0.9f); l.size = 0.035f; l.life = P.rnd(6, 10); l.noFloor = true; l.drag = 1.f; l.c0 = l.c1 = glm::vec4(1.f, 0.98f, 0.92f, 1.f); P.emit(l); } }
    }
    void apply(FighterPose& q) const {
        if (mode == 1) {   // pelo ar até as cordas: sobe, deita para trás (os pés vão para cima e para a frente) e desce na corda
            const float k = smooth(u / 0.18f) * (1.f - smooth((u - 0.8f) / 0.2f)), arc = std::sin(u * 3.14159f); const glm::vec3 c(0, 0.85f * size, 0);
            q.root = q.root * glm::translate(glm::mat4(1.f), glm::vec3(0, 0.5f * arc * size, 0) + c) * glm::rotate(glm::mat4(1.f), -1.05f * k, glm::vec3(1, 0, 0)) * glm::translate(glm::mat4(1.f), -c);
            q.neckTilt -= 0.5f * k; return; }
        if (mode != 2) return;
        if (t >= kFly) { q.root = glm::translate(glm::mat4(1.f), glm::vec3(0, -300.f, 0)); return; }   // já virou estrela
        const float e = flyK(), k = 1.f - 0.78f * e, spin = t * 0.5f; glm::mat4 R = q.root; R[3] = glm::vec4(0, 0, 0, 1); const glm::vec3 c(0, 0.85f * size, 0);
        q.root = glm::translate(glm::mat4(1.f), pos() + c * k) * glm::rotate(glm::mat4(1.f), -spin, side) * glm::rotate(glm::mat4(1.f), spin * 0.37f, glm::vec3(0, 1, 0)) * glm::scale(glm::mat4(1.f), glm::vec3(k)) * glm::translate(glm::mat4(1.f), -c) * R;
    }
    // a estrelinha: quatro pontas de luz viradas para a câmera, acendendo de uma vez e piscando até sumir
    void build(const glm::vec3& eye, V& gv, I& gi) const {
        if (mode != 2 || t < kFly - 2.f || t > kFly + kStar) return; const float s = (t - kFly + 2.f) / (kStar + 2.f), amp = std::pow(std::sin(3.14159f * s), 0.6f) * (0.75f + 0.25f * std::sin(s * 42.f));
        const glm::vec3 c = to + glm::vec3(0, 0.85f * size * 0.22f, 0), toCam = glm::normalize(eye - c), right = glm::normalize(glm::cross(glm::vec3(0, 1, 0), toCam)), up = glm::cross(toCam, right), col = glm::vec3(1.f, 0.97f, 0.8f) * amp;
        const auto ray = [&](const glm::vec3& d, const glm::vec3& w, float L, float W) { const unsigned b = put(gv, c, toCam, col, kUnlit); put(gv, c + d * L, toCam, glm::vec3(0.f), kUnlit); put(gv, c + w * W, toCam, glm::vec3(0.f), kUnlit); put(gv, c - d * L, toCam, glm::vec3(0.f), kUnlit); put(gv, c - w * W, toCam, glm::vec3(0.f), kUnlit); gi.insert(gi.end(), {b, b + 1, b + 2, b, b + 2, b + 3, b, b + 3, b + 4, b, b + 4, b + 1}); };
        const float L = 5.4f * amp, rot = 0.25f * std::sin(s * 9.f); const glm::vec3 a = right * std::cos(rot) + up * std::sin(rot), b2 = up * std::cos(rot) - right * std::sin(rot), d1 = glm::normalize(a + b2), d2 = glm::normalize(a - b2);
        ray(a, b2, L, 0.42f * amp); ray(b2, a, L, 0.42f * amp); ray(d1, d2, L * 0.45f, 0.3f * amp); ray(d2, d1, L * 0.45f, 0.3f * amp);
        ball(gv, gi, c, right * (0.45f * amp), up * (0.45f * amp), toCam * (0.45f * amp), col * 0.9f, kUnlit, 0.f, 5, 8);
    }
};

// ---------------- Vivaldi: a regência dos especiais que não tinham gesto próprio ----------------
struct MaestraFx : FxGeo {
    static void pose(Fighter& v, const Fighter& f) {
        if (f.charId != 5 || f.state != FIGHTER_SKILL) return; const float p = prog(f);
        const auto to = [](auto& g, float x, float y, float z, float k) { g.x += (x - g.x) * k; g.y += (y - g.y) * k; g.z += (z - g.z) * k; };
        switch (f.activeSkill) {
        case SkillId::VivaldiFourSeasons: {   // abre os braços num gesto largo e a batuta desenha um círculo no alto
            const float k = smooth(p / 0.22f) * (1.f - smooth((p - 0.78f) / 0.22f)), a = p * 6.2832f * 1.5f;
            to(v.gloveL, -44.f, 94.f + 5.f * std::sin(a), 12.f, k); to(v.gloveR, 34.f + 10.f * std::cos(a), 106.f + 10.f * std::sin(a), 18.f, k); v.offsetY += 3.f * k * std::sin(a * 2.f); v.bodySkew += 0.04f * k * std::sin(a); break; }
        case SkillId::VivaldiSpringSonata: {   // as mãos sobem juntas e se abrem como flor, o corpo balança no compasso
            const float k = smooth(p / 0.18f) * (1.f - smooth((p - 0.82f) / 0.18f)), up = smooth(p / 0.4f), open = smooth((p - 0.4f) / 0.3f), sway = std::sin(p * 12.566f) * 5.f;
            const float x = 9.f + 32.f * open, y = 74.f + 40.f * up - 10.f * open;
            to(v.gloveL, -x + sway, y, 14.f, k); to(v.gloveR, x + sway, y, 14.f, k); v.bodySkew += 0.05f * k * std::sin(p * 12.566f); v.offsetY += 2.5f * k * std::fabs(std::sin(p * 12.566f)); break; }
        case SkillId::VivaldiCrescendoJab: {   // crescendo: a mão livre aponta o rival e a batuta sobe para trás antes da estocada
            const float k = smooth(p / 0.16f) * (1.f - smooth((p - 0.24f) / 0.1f));
            to(v.gloveL, -12.f, 78.f, 30.f, k); to(v.gloveR, 32.f, 96.f, -14.f, k); v.offsetZ -= 6.f * k; break; }
        default: break;
        }
    }
};
}
