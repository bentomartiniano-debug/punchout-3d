// app3d/render/honey_fx.hpp: Modo Melaço e Geleia Real do Jack, como no vinil.
//  - Melaço (enquanto o modo dura): mel âmbar escorre da frente da roupa e das duas colmeias, as colmeias racham em zigue-zague, o rosto
//    ganha a sombra dura sobre os olhos com o olhar ciano (expressão "molasses" pintada no rosto) e duas luzinhas ciano pulsando; pingos
//    caem das colmeias.
//  - Geleia Real (cura): o mesmo escorrido em geleia clara, crescendo e sumindo em ~2,8 s, com brilhos subindo.
// As gotas ficam presas no tronco e nas colmeias (nós "upper" e "gloveL/R" do modelo) e penduram para baixo; a superfície vem da malha.
#pragma once
#include "particles.hpp"
#include "throwables.hpp"
#include "model/model.hpp"
#include "fighter.hpp"
#include "skills.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
namespace KoRama {
struct HoneyFx {
    float ma = 0.f, ja = 0.f, t = 0.f; int tj = -1; bool wasJ = false;
    // Pele de Mel (passiva): a pele fica laranja e gotas de mel estufam nela, escorrem e somem (como as gotas do Cremoso)
    bool skin = false; struct Drop { float u = 0, v = 0, t0 = 0, dur = 1, r = 0.03f, a = 0; int where = 0; }; Drop drops[12]; uint32_t rng = 9u;
    float rnd(float a, float b) { rng = rng * 1664525u + 1013904223u; return a + (b - a) * float(rng >> 8) / 16777216.f; }
    void respawn(Drop& d, bool first) { d.where = rnd(0.f, 1.f) < 0.65f ? 0 : (rnd(0.f, 1.f) < 0.5f ? 1 : 2); d.u = rnd(0.f, 1.f) < 0.5f ? rnd(0.06f, 0.17f) : (rnd(0.f, 1.f) < 0.5f ? rnd(0.39f, 0.61f) : rnd(0.83f, 0.94f)); d.v = rnd(0.42f, 0.85f); d.a = rnd(-1.4f, 1.4f); d.r = d.where == 0 ? rnd(0.014f, 0.024f) : rnd(0.03f, 0.045f); /* a maior parte no rosto, na parte que o capuz deixa ver */ d.dur = rnd(2.2f, 3.6f); d.t0 = t - (first ? rnd(0.f, d.dur) : 0.f); }
    // Favo Protetor (como no vinil): 16 favos de luz em dois anéis orbitando o Jack, sempre de frente para a câmera, pulsando; cada golpe
    // absorvido apaga um terço deles aos poucos (estágio 0, 1, 2), e quando o escudo acaba some o resto
    float wallT = -1.f, wallStage[3] = {0.f, 0.f, 0.f}; int shield = 0; glm::vec3 wallAt{0.f};
    // medidas da malha (no espaço do nó, repouso): frente do tronco por altura e raio das colmeias
    struct Shape { bool ready = false; float torsoFront[8]{}; float torsoY0 = 0.f, torsoY1 = 1.f; float hiveR = 0.25f; glm::vec3 hiveC{0.f}; } shape;
    static float sat(float x) { return std::clamp(x, 0.f, 1.f); }
    static float ss(float a, float b, float x) { const float k = sat((x - a) / (b - a)); return k * k * (3.f - 2.f * k); }

    // mede a frente do tronco (z máximo por faixa de altura) e a colmeia (centro e raio) a partir das caixas das peças, uma vez
    void measure(const Model& m) {
        if (shape.ready) return; shape.ready = true;
        const auto walk = [&](auto&& self, int i, const glm::mat4& M, auto&& fn) -> void { const Node& n = m.nodes[size_t(i)]; const glm::mat4 W = M * glm::translate(glm::mat4(1.f), n.pos) * glm::mat4_cast(n.rot0) * glm::scale(glm::mat4(1.f), n.scale);
            if (n.name.rfind("face_", 0) == 0) return; for (const Prim& p : n.prims) for (int c = 0; c < 8; c++) fn(glm::vec3(W * glm::vec4(c & 1 ? p.hi.x : p.lo.x, c & 2 ? p.hi.y : p.lo.y, c & 4 ? p.hi.z : p.lo.z, 1.f)));
            for (int k : n.kids) if (m.nodes[size_t(k)].name != "neck" && m.nodes[size_t(k)].name.rfind("glove", 0) != 0) self(self, k, W, fn); };
        const auto local = [&](int i) { const Node& n = m.nodes[size_t(i)]; return glm::inverse(glm::translate(glm::mat4(1.f), n.pos) * glm::mat4_cast(n.rot0) * glm::scale(glm::mat4(1.f), n.scale)); };   // espaço do próprio nó
        const auto it = m.byName.find("upper"); if (it != m.byName.end()) { std::vector<glm::vec3> pts; walk(walk, it->second, local(it->second), [&](const glm::vec3& p) { pts.push_back(p); });
            for (const glm::vec3& p : pts) { shape.torsoY0 = std::min(shape.torsoY0, p.y); shape.torsoY1 = std::max(shape.torsoY1, p.y); }
            for (const glm::vec3& p : pts) { const int b = std::clamp(int((p.y - shape.torsoY0) / std::max(1e-3f, shape.torsoY1 - shape.torsoY0) * 8.f), 0, 7); if (std::fabs(p.x) < 0.25f) shape.torsoFront[b] = std::max(shape.torsoFront[b], p.z); } }
        const auto g = m.byName.find("gloveL"); if (g != m.byName.end()) { glm::vec3 lo(1e9f), hi(-1e9f); walk(walk, g->second, local(g->second), [&](const glm::vec3& p) { lo = glm::min(lo, p); hi = glm::max(hi, p); });
            shape.hiveC = (lo + hi) * 0.5f; shape.hiveR = std::max(0.08f, std::min(hi.x - lo.x, hi.z - lo.z) * 0.5f); }
    }
    float frontAt(float y) const { const int b = std::clamp(int((y - shape.torsoY0) / std::max(1e-3f, shape.torsoY1 - shape.torsoY0) * 8.f), 0, 7); return shape.torsoFront[b]; }

    void tick(const Fighter& f, const glm::vec3& handL, const glm::vec3& handR, Particles& P, const glm::vec3& feet = glm::vec3(0.f)) {
        t += 1.f / 60.f; if (f.charId != 0) { ma = ja = 0.f; tj = -1; wallT = -1.f; return; }
        { const int sh = f.status.honeyShield; if (sh > 0 && shield == 0) wallT = 0.f; shield = sh; wallAt = feet;
          if (wallT >= 0.f) { wallT += 1.f / 60.f; bool any = false; for (int s = 0; s < 3; s++) { const bool lit = sh > s; wallStage[s] = lit ? std::min(1.f, wallStage[s] + 1.f / 24.f) : std::max(0.f, wallStage[s] - 1.f / 21.f); any |= wallStage[s] > 0.f; } if (!any && sh == 0) wallT = -1.f; } }
        skin = f.loadout.passive == SkillId::JackHoneySkin; if (skin) for (Drop& d : drops) if (d.dur <= 1.f || t - d.t0 > d.dur) respawn(d, d.dur <= 1.f);
        const bool mol = f.status.molasses > 0; ma = mol ? std::min(1.f, ma + 1.f / 40.f) : std::max(0.f, ma - 1.f / 24.f);
        const bool jel = f.state == FIGHTER_SKILL && f.activeSkill == SkillId::JackRoyalJelly; if (jel && !wasJ) tj = 0; wasJ = jel; if (tj >= 0) { tj++; if (tj > 175) tj = -1; }
        ja = tj < 0 ? 0.f : ss(6.f, 30.f, float(tj)) * (1.f - ss(126.f, 168.f, float(tj)));
        if (ma > 0.6f && P.rnd(0.f, 1.f) < 0.35f) { const glm::vec3 h = P.rnd(0.f, 1.f) < 0.5f ? handL : handR; Particle q; q.kind = kEnergy; q.p = h + glm::vec3(P.rnd(-0.1f, 0.1f), -shape.hiveR * 0.9f, P.rnd(0.f, 0.12f)); q.v = glm::vec3(0, -0.004f, 0); q.size = P.rnd(0.025f, 0.04f); q.gravity = 0.004f; q.life = 40;
            q.c0 = glm::vec4(1.f, 0.62f, 0.08f, 1.f); q.c1 = glm::vec4(0.85f, 0.4f, 0.02f, 1.f); P.emit(q); }   // pingos
        if (ja > 0.3f && P.rnd(0.f, 1.f) < 0.5f) { Particle q; q.kind = kGlow; q.p = (handL + handR) * 0.5f + P.dir() * 0.4f; q.v = glm::vec3(0, P.rnd(0.008f, 0.016f), 0); q.size = P.rnd(0.02f, 0.035f); q.life = P.rnd(24, 40); q.noFloor = true;
            q.c0 = glm::vec4(1.f, 1.f, 0.8f, 1.f); q.c1 = glm::vec4(1.f, 0.9f, 0.4f, 1.f); P.emit(q); }   // brilhos da geleia
    }
    bool on() const { return ma > 0.01f || ja > 0.01f || skin; }
    bool wallOn() const { return wallT >= 0.f; }
    // favos virados para a câmera (eye). O miolo (fv) e as bordas (ev) são peças translúcidas de cor própria (Renderer::props), para o favo
    // continuar âmbar em qualquer ringue; a luz somada (gv, Renderer::glow) fica só como um brilho fraco, senão estoura em branco no ringue claro
    void buildWall(const glm::vec3& eye, float size, std::vector<float>& fv0, std::vector<unsigned>& fi0, std::vector<float>& ev0, std::vector<unsigned>& ei0, std::vector<float>& gv0, std::vector<unsigned>& gi0) const {
        std::vector<float>* pv = &gv0; std::vector<unsigned>* pi = &gi0;
#define gv (*pv)
#define gi (*pi)
        if (!wallOn()) return; const float sp = ss(0.f, 0.4f, wallT);
        const auto put = [&](const glm::vec3& p, const glm::vec3& c) { const float d[12] = {p.x, p.y, p.z, 0, 1, 0, c.r, c.g, c.b, -2.f, -1.f, 0.f}; gv.insert(gv.end(), d, d + 12); };
        for (int i = 0; i < 16; i++) { const bool up = i < 8; const int stage = i % 3; const float a = sp * wallStage[stage]; if (a <= 0.01f) continue;
            const float an = (float(up ? i : i - 8) + (up ? 0.f : 0.5f)) / 8.f * 6.2832f + wallT * 1.44f, r = 0.78f * size * (0.35f + 0.65f * sp), ph = float(i) * 0.4f, pu = 0.75f + 0.25f * std::sin(t * 4.2f + ph);
            const glm::vec3 c = wallAt + glm::vec3(std::cos(an) * r, (up ? 1.12f : 0.74f) * size + std::sin(t * 3.5f + ph) * 0.02f, std::sin(an) * r);
            const glm::vec3 toCam = glm::normalize(eye - c), right = glm::normalize(glm::cross(glm::vec3(0, 1, 0), toCam) + glm::vec3(1e-5f)), upv = glm::cross(toCam, right); float hs = 0.17f * size * (0.4f + 0.6f * sp);
            const auto hexAt = [&](float k, float ang) { return c + (right * std::cos(ang) + upv * std::sin(ang)) * (hs * k); };
            const auto ring = [&](float r0, float r1, const glm::vec3& col) { const unsigned b = unsigned(gv.size() / 12); for (int s = 0; s < 6; s++) { const float g = float(s) / 6.f * 6.2832f; put(hexAt(r0, g), col); put(hexAt(r1, g), col); }
                for (int s = 0; s < 6; s++) { const unsigned i0 = b + unsigned(s * 2), i1 = i0 + 1, i2 = b + unsigned(((s + 1) % 6) * 2), i3 = i2 + 1; gi.insert(gi.end(), {i0, i1, i2, i1, i3, i2}); } };
            const auto fill = [&](const glm::vec3& col, const glm::vec3& mid) { const unsigned b = unsigned(gv.size() / 12); put(c, mid); for (int s = 0; s < 6; s++) put(hexAt(0.9f, float(s) / 6.f * 6.2832f), col);
              for (int s = 0; s < 6; s++) gi.insert(gi.end(), {b, b + 1 + unsigned(s), b + 1 + unsigned((s + 1) % 6)}); };
            const float k = a * (0.4f + 0.6f * sp);   // as peças translúcidas não têm opacidade por vértice: o favo entra e sai crescendo
            pv = &fv0; pi = &fi0; { const float keep = hs; hs = keep * k; fill(glm::vec3(0.93f, 0.56f, 0.04f), glm::vec3(1.f, 0.78f, 0.2f) * (0.85f + 0.15f * pu));   // miolo âmbar
            pv = &ev0; pi = &ei0; ring(0.88f, 1.f, glm::vec3(1.f, 0.86f, 0.3f)); ring(0.5f, 0.58f, glm::vec3(1.f, 0.93f, 0.55f));   // borda e hexágono de dentro
            pv = &gv0; pi = &gi0; ring(0.9f, 1.f, glm::vec3(1.f, 0.9f, 0.4f) * (0.2f * a * pu)); hs = keep; }   // brilho fraco na borda
        }
#undef gv
#undef gi
    }

    // gotas (props), rachaduras (props) e o brilho dos olhos (glow), com o modelo já posto na pose
    std::vector<float> faceV; std::vector<unsigned> faceI;   // gotas do rosto (translúcidas, desenhadas à parte)
    void build(Model& m, std::vector<float>& v, std::vector<unsigned>& idx, std::vector<float>& gv, std::vector<unsigned>& gi) {
        faceV.clear(); faceI.clear();
        if (!on()) return; measure(m);
        Throwables::Mesh skinMesh, faceMesh;
        const auto putMesh = [&](const Throwables::Mesh& mm, std::vector<float>& V, std::vector<unsigned>& I, float flag) { const unsigned base = unsigned(V.size() / 12); for (const Throwables::PV& pv : mm.v) { const float d[12] = {pv.p.x, pv.p.y, pv.p.z, pv.n.x, pv.n.y, pv.n.z, pv.c.r, pv.c.g, pv.c.b, flag, -1.f, pv.ink}; V.insert(V.end(), d, d + 12); } for (unsigned k : mm.idx) I.push_back(base + k); };
        Throwables::Mesh mesh;
        // gota pendurada: um fio esticado para baixo do ponto (no mundo) e a bolota na ponta
        const auto drip = [&](const glm::vec3& at, const glm::vec3& out, float w, float len, const glm::vec3& col) { if (len < 0.004f) return; const glm::vec3 down(0, -1, 0), side = glm::normalize(glm::cross(down, out) + glm::vec3(1e-5f));
            Throwables::blob(mesh, at + down * (len * 0.5f) + out * (w * 0.15f), side * (w * 0.55f), down * (len * 0.5f), out * (w * 0.3f), col, 0.004f, 0.f);
            Throwables::blob(mesh, at + down * len + out * (w * 0.2f), side * (w * 0.75f), down * (w * 0.9f), out * (w * 0.6f), col, 0.004f, 0.f);
            Throwables::blob(mesh, at + down * (len - w * 0.25f) + out * (w * 0.75f) - side * (w * 0.25f), side * (w * 0.2f), down * (w * 0.3f), out * (w * 0.1f), glm::vec3(1.f, 0.97f, 0.85f), 0.f, 0.f); };   // brilho molhado
        const auto layer = [&](float a, const glm::vec3& col, float dx, float dy) {
            if (a <= 0.01f) return;
            if (const auto it = m.byName.find("upper"); it != m.byName.end()) { const glm::mat4& W = m.nodes[size_t(it->second)].world; const glm::vec3 fwd = glm::normalize(glm::vec3(W[2]));
                static const float coat[5][4] = {{-0.17f, 0.7f, 0.2f, 0.06f}, {0.14f, 0.76f, 0.17f, 0.05f}, {-0.02f, 0.55f, 0.21f, 0.065f}, {0.2f, 0.48f, 0.13f, 0.045f}, {-0.24f, 0.33f, 0.12f, 0.045f}};   // x, altura (fração do tronco), comprimento, largura
                for (const auto& c : coat) { const float y = shape.torsoY0 + (shape.torsoY1 - shape.torsoY0) * (c[1] + dy), x = c[0] + dx; drip(glm::vec3(W * glm::vec4(x, y, frontAt(y) + 0.025f, 1.f)), fwd, c[3], c[2] * a, col); } }
            for (const char* gn : {"gloveL", "gloveR"}) { const auto it = m.byName.find(gn); if (it == m.byName.end()) continue; const glm::mat4& W = m.nodes[size_t(it->second)].world;
                static const float dirs[5][4] = {{-0.5f, 0.25f, 0.85f, 0.2f}, {0.45f, 0.3f, 0.85f, 0.18f}, {0.f, 0.5f, 0.85f, 0.24f}, {-0.8f, -0.1f, 0.6f, 0.13f}, {0.8f, 0.f, 0.6f, 0.12f}};
                for (const auto& d : dirs) { const glm::vec3 n = glm::normalize(glm::vec3(d[0], d[1], d[2])), lp = shape.hiveC + n * shape.hiveR, at = glm::vec3(W * glm::vec4(lp, 1.f)), out = glm::normalize(glm::mat3(W) * n);
                    drip(at, out, 0.05f, d[3] * a, col); } }
        };
        layer(ma, glm::vec3(0.93f, 0.5f, 0.03f), 0.f, 0.f);            // mel âmbar
        layer(ja, glm::vec3(1.f, 0.95f, 0.58f), 0.04f, -0.03f);        // geleia clara
        // rachaduras nas colmeias (melaço): zigue-zague escuro descendo pela superfície
        if (ma > 0.02f) for (const char* gn : {"gloveL", "gloveR"}) { const auto it = m.byName.find(gn); if (it == m.byName.end()) continue; const glm::mat4& W = m.nodes[size_t(it->second)].world;
            for (const float a0 : {-0.95f, 0.9f, -1.45f, 1.4f}) { glm::vec3 prev; for (int k = 0; k <= 8; k++) { const float a = a0 + (k % 2 ? 0.13f : -0.13f), y = 0.75f - 1.4f * float(k) / 8.f * std::min(1.f, ma * 1.5f);
                    const glm::vec3 n = glm::normalize(glm::vec3(std::sin(a), y, std::cos(a))), p = glm::vec3(W * glm::vec4(shape.hiveC + n * (shape.hiveR * 0.98f), 1.f));
                    if (k > 0) { const glm::vec3 d = p - prev, out = glm::normalize(glm::mat3(W) * n), side = glm::normalize(glm::cross(d, out) + glm::vec3(1e-5f)); Throwables::blob(mesh, (p + prev) * 0.5f, d * 0.55f, side * 0.012f, out * 0.008f, glm::vec3(0.06f, 0.04f, 0.03f), 0.f, 0.f); }
                    prev = p; } } }
        // gotas da Pele de Mel: no rosto (pela textura, descendo para o queixo) e nas colmeias (descendo pela superfície)
        if (skin) for (const Drop& d : drops) { const float p = (t - d.t0) / d.dur; if (p < 0.f || p > 1.f) continue; const float size = ss(0.f, 0.16f, p) * (1.f - ss(0.78f, 1.f, p)), q = std::clamp((p - 0.08f) / 0.78f, 0.f, 1.f), slide = std::pow(q, 1.7f);
            glm::vec3 at, nw; if (d.where == 0) { int node = -1; glm::vec3 lp, ln; if (!m.uvPointSmooth(0, glm::vec2(d.u, d.v - 0.38f * slide), node, lp, ln)) continue; const glm::mat4& W = m.nodes[size_t(node)].world; at = glm::vec3(W * glm::vec4(lp, 1.f)); nw = glm::normalize(glm::mat3(W) * ln);
                if (const auto nk = m.byName.find("neck"); nk != m.byName.end()) { const glm::vec3 hc = glm::vec3(m.nodes[size_t(nk->second)].world[3]) + glm::vec3(0, 0.12f, 0); if (glm::dot(nw, at - hc) < 0.f) nw = -nw; } }   // a normal para fora da cabeça
            else { const auto it = m.byName.find(d.where == 1 ? "gloveL" : "gloveR"); if (it == m.byName.end()) continue; const glm::mat4& W = m.nodes[size_t(it->second)].world; const float y = 0.7f - 1.3f * (d.v - 0.42f) / 0.48f * 0.3f - 1.1f * slide;
                const glm::vec3 n = glm::normalize(glm::vec3(std::sin(d.a), y, std::cos(d.a))); at = glm::vec3(W * glm::vec4(shape.hiveC + n * shape.hiveR, 1.f)); nw = glm::normalize(glm::mat3(W) * n); }
            Throwables::Mesh& dm = d.where == 0 ? faceMesh : skinMesh; const float r = d.r * size; if (r < 0.004f) continue; const glm::vec3 down = glm::normalize(glm::vec3(0, -1, 0) - nw * glm::dot(glm::vec3(0, -1, 0), nw) + glm::vec3(1e-5f)), side = glm::normalize(glm::cross(down, nw));
            Throwables::blob(dm, at + nw * (r * 0.35f) + down * (r * 0.3f), side * r, down * (r * 1.35f), nw * (r * 0.55f), glm::vec3(0.95f, 0.5f, 0.04f), 0.0035f, 0.f);   // a gota estufando a pele, comprida para baixo
            if (d.where != 0) Throwables::blob(dm, at + nw * (r * 0.8f) - side * (r * 0.3f) - down * (r * 0.1f), side * (r * 0.22f), down * (r * 0.35f), nw * (r * 0.12f), glm::vec3(1.f, 0.96f, 0.82f), 0.f, 0.f); }   // brilho
        putMesh(skinMesh, v, idx, -1.f); putMesh(faceMesh, faceV, faceI, -1.f);
        putMesh(mesh, v, idx, -1.f);
        // olhar ciano pulsando (melaço): luz somada na frente dos olhos
        if (ma > 0.02f) { Throwables::Mesh g; for (const glm::vec2 uv : {glm::vec2(0.27f, 0.6f), glm::vec2(0.73f, 0.6f)}) { int node = -1; glm::vec3 lp, ln; if (!m.uvPoint(0, uv, node, lp, ln)) continue;
                const glm::mat4& W = m.nodes[size_t(node)].world; const glm::vec3 at = glm::vec3(W * glm::vec4(lp, 1.f)), nw = glm::normalize(glm::mat3(W) * ln), side = glm::normalize(glm::cross(glm::vec3(0, 1, 0), nw) + glm::vec3(1e-5f));
                const float k = ma * (0.45f + 0.25f * std::sin(t * 6.f)); Throwables::blob(g, at + nw * 0.02f, side * 0.06f, glm::vec3(0, 0.026f, 0), nw * 0.02f, glm::vec3(0.f, 0.96f, 0.83f) * k, 0.f, 0.f); }
            putMesh(g, gv, gi, -2.f); }
    }
    // rosto do melaço (sombra dura nos olhos e olhar ciano): pinta por cima do rosto normal (a textura é de cabeça para baixo)
    static void paintMolasses(unsigned w, unsigned h, std::uint8_t* px, float k = 1.f) {   // k: quanto da sombra (os estágios da transição)
        const auto put = [&](int x, int y, std::uint8_t r, std::uint8_t g, std::uint8_t b, float a) { if (x < 0 || y < 0 || x >= int(w) || y >= int(h)) return; std::uint8_t* p = px + (size_t(y) * w + size_t(x)) * 4; p[0] = std::uint8_t(p[0] * (1.f - a) + r * a); p[1] = std::uint8_t(p[1] * (1.f - a) + g * a); p[2] = std::uint8_t(p[2] * (1.f - a) + b * a); };
        const float sx = float(w) / 1024.f, sy = float(h) / 1024.f;
        for (int x = 0; x < int(w); x++) { const float u = float(x) / float(w) - 0.5f, bow = 40.f * sy * (1.f - 4.f * u * u); const int ya = int(200.f * sy + bow);   // a sombra do capuz: da metade do nariz para cima (nariz, olhos, testa)
            for (int y = ya; y < int(h); y++) put(x, y, 15, 23, 42, 0.92f * k); }
        for (const float cx : {277.f, 747.f}) for (int y = int(585.f * sy); y <= int(645.f * sy); y++) for (int x = int((cx - 62.f) * sx); x <= int((cx + 62.f) * sx); x++) put(x, y, 0, 245, 212, std::clamp(k * 1.4f - 0.4f, 0.f, 1.f));   // olhar ciano
    }
};
}
