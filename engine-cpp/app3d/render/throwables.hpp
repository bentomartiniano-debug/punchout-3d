// app3d/render/throwables.hpp: o que a torcida joga na comemoração, em 3D.
// Rosas: modeladas em gomos fofos (miolo, três camadas de pétalas, sépalas, haste e folhas) nas cores do jogo original; voam girando e
// esticadas na direção do voo, quicam, deitam de lado e amassam no toque (as pétalas balançam e voltam).
// Calcinhas e cuecas: peças 3D costuradas como de verdade (painel da frente e de trás unidos nas laterais e no fundo; cueca também no
// entrepernas), com elástico na cintura e lacinho; pano mole com espessura (malha de pontos com molas fracas de dobra) e a estampa do tecido
// de cada peça do jogo original. Voam abrindo e tremulando, dobram por cima das cordas, escorregam para dentro e terminam amassadas no chão.
// Nada sai do ringue: as cordas seguram quem já está dentro, e fora do ringue não tem chão (cai e some).
// Tempo em quadros de 60 Hz, metros.
#pragma once
#include "rig/ropes.hpp"
#include "particles.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/quaternion.hpp>
namespace KoRama {
struct Throwables {
    static constexpr int kRoseKinds = 3, kUnderKinds = 12;
    static constexpr float kG = 0.0035f, kFloor = 0.035f, kRopeR = 0.05f, kRS = 1.5f;   // rosas maiores que de verdade para ler de longe
    static constexpr int kAtlasCell = 256, kAtlasCols = 4, kAtlasSize = 1024, kWhiteCell = 15;   // atlas das estampas: uma célula por peça e uma branca (elástico)

    // tecido de cada peça (as cores e estampas do jogo original, celebration.cpp)
    enum Pattern { kPlain, kDots, kStripes, kBows, kHearts };
    // estampas clássicas de desenho animado, para ler de primeira: calcinha em tom pastel com bolinhas, corações, listras ou lacinhos,
    // babado branco ou rosa nas pernas e na cintura e um laço na frente; cueca samba-canção com corações, listras ou bolinhas e cós liso
    struct ClothDef { const char *main, *elastic, *accent; int pattern; const char *patternColor, *ruffle; bool boxer; };
    static const ClothDef& clothDef(int c) {
        static const ClothDef k[kUnderKinds] = {
            {"#ff9ec7", "#ffffff", "#e8174f", kDots, "#ffffff", "#ffffff", false},     // rosa com bolinhas brancas
            {"#ffffff", "#ffb3d1", "#ff4f8b", kHearts, "#e8174f", "#ffb3d1", false},   // branca com corações vermelhos
            {"#a8dcff", "#ffffff", "#ff6fa5", kStripes, "#ffffff", "#ffffff", false},  // azul-bebê listrada
            {"#d6b8ff", "#ffffff", "#8a4fe0", kBows, "#ffffff", "#ffffff", false},     // lilás com lacinhos
            {"#ffffff", "#ff9ec7", "#e8174f", kDots, "#ff7fb2", "#ff9ec7", false},     // branca com bolinhas rosa
            {"#ff5a7a", "#ffffff", "#ffffff", kHearts, "#ffffff", "#ffffff", false},   // vermelha com corações brancos
            {"#ffffff", "#e8233f", "#e8233f", kHearts, "#e8233f", nullptr, true},      // samba-canção branca de corações (a clássica)
            {"#9fd4f5", "#1f3d7a", "#1f3d7a", kStripes, "#ffffff", nullptr, true},     // azul-clara listrada
            {"#ffffff", "#2f6fd6", "#2f6fd6", kDots, "#2f6fd6", nullptr, true},        // branca de bolinhas azuis
            {"#ffe066", "#e8233f", "#e8233f", kDots, "#e8233f", nullptr, true},        // amarela de bolinhas vermelhas
            {"#ffffff", "#e8233f", "#e8233f", kStripes, "#e8233f", nullptr, true},     // listrada de bala (vermelha e branca)
            {"#ffd1e3", "#e8233f", "#e8233f", kHearts, "#e8233f", nullptr, true} };    // rosinha de corações
        return k[std::clamp(c, 0, kUnderKinds - 1)];
    }
    static glm::vec3 hexLin(const char* h) { unsigned v = 0; for (const char* p = h + 1; *p; p++) v = v * 16u + unsigned(*p <= '9' ? *p - '0' : (*p | 32) - 'a' + 10); return lin(v); }

    struct PV { glm::vec3 p, n, c; float ink, jig; };
    struct Mesh { std::vector<PV> v; std::vector<unsigned> idx; };
    // molde da peça: pontos (frente e costas; os das costuras são os mesmos), molas e a grade de cada painel para desenhar
    struct Link { int a, b; float r, k; };
    struct Panel { float sgn = 1.f; std::vector<int> at; std::vector<std::pair<int, int>> rims; };   // rims: bordas abertas (cintura, pernas), pares de vértices da grade   // at: índice do ponto em cada vértice da grade (-1 fora da peça)
    struct Garment { bool boxer = false; std::vector<float> rowY; int NX = 0, NY = 0; std::vector<glm::vec3> rest; std::vector<glm::vec2> art; std::vector<char> band; std::vector<Link> links; Panel panel[2]; int bow = -1, bowL = -1, bowR = -1, bowD = -1; };
    struct Item {
        int kind = 0, life = 0; bool cloth = false, landed = false;
        glm::vec3 pos{0.f}, vel{0.f}, w{0.f}; glm::quat q{1, 0, 0, 0}; float sq = 0.f, sqV = 0.f, phase = 0.f; int still = 0, bounces = 0, dying = -1;   // rosa; dying: quadros do sumiço (-1 vivo)
        std::vector<glm::vec3> p, o; float crumple = 0.f; bool caught = false, flopped = false;   // pano: Verlet; crumple amassando no chão (0..1)
    };
    std::vector<Item> items; uint32_t rng = 1u; Mesh rose[kRoseKinds]; Garment garment[2]; bool ready = false; const RingRopes* ropes = nullptr;

    float rnd(float a, float b) { rng = rng * 1664525u + 1013904223u; return a + (b - a) * float(rng >> 8) / 16777216.f; }
    static glm::vec3 lin(unsigned hex) { const glm::vec3 c(float((hex >> 16) & 255) / 255.f, float((hex >> 8) & 255) / 255.f, float(hex & 255) / 255.f); return glm::pow(c, glm::vec3(2.2f)); }
    void clear() { items.clear(); }
    float topRope() const { return ropes ? ropes->heights[std::max(0, ropes->rows - 1)] : RingRopes::kHeights[2]; }

    // ---------- modelo da rosa ----------
    // gomo: elipsoide nos eixos ex, ey, ez (vetores com o meio-tamanho), com a ponta (ey+) dobrando para ez
    static void blob(Mesh& m, const glm::vec3& c, const glm::vec3& ex, const glm::vec3& ey, const glm::vec3& ez, const glm::vec3& col, float ink, float jig, float flare = 0.f) {
        constexpr int NL = 6, NJ = 9; const unsigned base = unsigned(m.v.size());
        if (glm::dot(glm::cross(ex, ey), ez) > 0.f) { blob(m, c, ex, ey, -ez, col, ink, jig, -flare); return; }   // a ordem dos triângulos espera eixos canhotos (senão o contorno cobre a peça)
        const float lx = glm::dot(ex, ex), ly = glm::dot(ey, ey), lz = glm::dot(ez, ez);
        for (int i = 0; i <= NL; i++) { const float th = 3.14159f * float(i) / float(NL);
            for (int j = 0; j < NJ; j++) { const float ph = 6.28318f * float(j) / float(NJ); const glm::vec3 l(std::sin(th) * std::cos(ph), std::cos(th), std::sin(th) * std::sin(ph));
                const float tip = (l.y + 1.f) * 0.5f; glm::vec3 p = c + ex * l.x + ey * l.y + ez * l.z + glm::normalize(ez) * (flare * tip * tip);
                glm::vec3 n = ex * (l.x / lx) + ey * (l.y / ly) + ez * (l.z / lz); n = glm::normalize(n + glm::vec3(1e-6f));
                m.v.push_back({p, n, col, ink, jig * tip}); } }
        for (int i = 0; i < NL; i++) for (int j = 0; j < NJ; j++) { const unsigned a = base + unsigned(i * NJ + j), b = base + unsigned(i * NJ + (j + 1) % NJ), cc = a + NJ, d = b + NJ; m.idx.insert(m.idx.end(), {a, cc, b, b, cc, d}); }
    }
    static void petals(Mesh& m, int n, float y, float rb, float L, float W, float T, float open, float off, const glm::vec3& col, float jig, float flare) {
        for (int i = 0; i < n; i++) { const float a = 6.28318f * float(i) / float(n) + off; const glm::vec3 d(std::cos(a), 0, std::sin(a)), tg(-std::sin(a), 0, std::cos(a));
            const glm::vec3 up = glm::normalize(d * std::sin(open) + glm::vec3(0, 1, 0) * std::cos(open)), out = glm::normalize(d * std::cos(open) - glm::vec3(0, 1, 0) * std::sin(open));
            blob(m, glm::vec3(0, y, 0) + d * rb + up * (L * 0.5f), tg * (W * 0.5f), up * (L * 0.5f), out * (T * 0.5f), col, 0.005f, jig, flare * L); }
    }
    void buildRoses() {
        static const unsigned kMain[3] = {0xe63946, 0xf4be41, 0x8d212b}, kDark[3] = {0xb71c1c, 0xb3820a, 0x4a0e13}, kLight[3] = {0xff6b6b, 0xffe285, 0xc94451};
        const glm::vec3 stem = lin(0x2a9d8f), stemD = lin(0x264653), leaf = lin(0x2a9d8f);
        for (int k = 0; k < kRoseKinds; k++) { Mesh& m = rose[k]; m = {}; const glm::vec3 mn = lin(kMain[k]), dk = lin(kDark[k]), lt = lin(kLight[k]);
            blob(m, glm::vec3(0, 0.055f, 0), glm::vec3(0.02f, 0, 0), glm::vec3(0, 0.028f, 0), glm::vec3(0, 0, 0.02f), dk, 0.004f, 0.2f);   // miolo
            petals(m, 4, 0.03f, 0.012f, 0.05f, 0.042f, 0.016f, 0.22f, 0.3f, glm::mix(dk, mn, 0.5f), 0.4f, 0.1f);
            petals(m, 5, 0.025f, 0.02f, 0.056f, 0.052f, 0.016f, 0.55f, 1.1f, mn, 0.7f, 0.18f);
            petals(m, 6, 0.02f, 0.026f, 0.058f, 0.058f, 0.015f, 0.95f, 0.2f, glm::mix(mn, lt, 0.35f), 1.f, 0.26f);
            petals(m, 5, 0.018f, 0.01f, 0.036f, 0.016f, 0.007f, 2.0f, 0.6f, stemD, 0.f, 0.f);   // sépalas (contorno fino: ver petals)
            blob(m, glm::vec3(0, -0.11f, 0), glm::vec3(0.0075f, 0, 0), glm::vec3(0, 0.125f, 0), glm::vec3(0, 0, 0.0075f), stem, 0.0016f, 0.f);   // haste
            blob(m, glm::vec3(0.03f, -0.08f, 0), glm::normalize(glm::vec3(1, 0.35f, 0)) * 0.035f, glm::vec3(0, 0, 0.017f), glm::normalize(glm::vec3(-0.35f, 1, 0)) * 0.004f, leaf, 0.0015f, 0.f);
            blob(m, glm::vec3(-0.028f, -0.15f, 0.005f), glm::normalize(glm::vec3(-1, 0.3f, 0.2f)) * 0.033f, glm::vec3(0, 0, 0.016f), glm::normalize(glm::vec3(0.3f, 1, 0)) * 0.004f, leaf, 0.0015f, 0.f); }
        buildGarment(garment[0], false); buildGarment(garment[1], true); ready = true;
    }

    // ---------- molde das roupas ----------
    static bool inPoly(const std::vector<glm::vec2>& P, float x, float y) { bool in = false; for (size_t i = 0, j = P.size() - 1; i < P.size(); j = i++) if ((P[i].y > y) != (P[j].y > y) && x < (P[j].x - P[i].x) * (y - P[i].y) / (P[j].y - P[i].y) + P[i].x) in = !in; return in; }
    static float segDist(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b) { const glm::vec2 ab = b - a; const float t = std::clamp(glm::dot(p - a, ab) / glm::dot(ab, ab), 0.f, 1.f); return glm::length(p - (a + ab * t)); }
    // molde paramétrico (como as roupas dos personagens: bordas lisas e volume): cada painel é uma grade de colunas (de um lado ao outro)
    // e linhas (da cintura até o fundo), com a largura seguindo o contorno da peça original (coordenadas da arte 200x200, y para baixo).
    // Calcinha: painéis da frente e de trás costurados nas laterais de cima e no fundo; as aberturas das pernas ficam livres.
    // Cueca: duas pernas abaixo do gancho; costura nas laterais e no entrepernas; cintura e barras livres.
    static constexpr float kS = 0.0036f, kD = 0.055f;   // metros por unidade da arte; meio volume (frente/costas)
    static void buildGarment(Garment& g, bool boxer) {
        g = {}; g.boxer = boxer; g.NX = boxer ? 18 : 17; g.NY = 12; const int NX = g.NX, NY = g.NY, H = NX / 2; const float yC = 102.f;
        for (int j = 0; j < NY; j++) { const float v = float(j) / float(NY - 1); g.rowY.push_back(boxer ? 42.f + 98.f * v : 46.f + 104.f * v); }
        const auto hw = [&](float y) { if (boxer) return 68.f + (y - 42.f) / 98.f * 5.f; if (y <= 76.f) return 70.f + (y - 46.f) / 30.f * 2.f; const float t = (y - 76.f) / 74.f; return 13.f + 59.f * (1.f - std::pow(t, 2.2f)); };
        const auto gap = [&](float y) { return y <= yC ? 0.f : 20.f * std::pow((y - yC) / 38.f, 0.8f); };
        // posição na arte e quanto volume cada vértice da grade tem
        const auto artOf = [&](int i, int j, float& bulge) { const float y = g.rowY[size_t(j)], w = hw(y);
            if (!boxer) { const float s = -1.f + 2.f * float(i) / float(NX - 1); bulge = std::sqrt(std::max(0.f, 1.f - s * s)); return glm::vec2(100.f + s * w, y); }
            const bool right = i >= H; const float u = right ? float(i - H) / float(H - 1) : float(i) / float(H - 1), gp = gap(y);
            const float x = right ? gp + u * (w - gp) : -w + u * (w - gp);
            if (y <= yC) { const float s = x / w; bulge = std::sqrt(std::max(0.f, 1.f - s * s)); } else bulge = std::sin(3.14159f * u);
            return glm::vec2(100.f + x, y); };
        const auto shared = [&](int i, int j) { const float y = g.rowY[size_t(j)];   // costura: o mesmo ponto na frente e atrás
            if (!boxer) return ((i == 0 || i == NX - 1) && y <= 77.f) || j == NY - 1;
            return i == 0 || i == NX - 1 || ((i == H - 1 || i == H) && y > yC); };
        for (int f = 0; f < 2; f++) { Panel& pn = g.panel[f]; pn.sgn = f == 0 ? -1.f : 1.f; pn.at.assign(size_t(NX * NY), -1);
            for (int j = 0; j < NY; j++) for (int i = 0; i < NX; i++) { const size_t k = size_t(j * NX + i);
                if (boxer && i == H && g.rowY[size_t(j)] <= yC) { pn.at[k] = pn.at[k - 1]; continue; }   // acima do gancho as duas pernas são um pano só
                if (f == 1 && shared(i, j)) { pn.at[k] = g.panel[0].at[k]; continue; }
                float bulge = 0.f; const glm::vec2 a = artOf(i, j, bulge); const bool sh = shared(i, j); pn.at[k] = int(g.rest.size());
                const float z = sh ? 0.f : (f == 0 ? 1.f : -1.f) * kD * std::max(0.3f, bulge) * (boxer ? 1.f : 1.f - 0.45f * float(j) / float(NY - 1));
                g.rest.push_back(glm::vec3((a.x - 100.f) * kS, (100.f - a.y) * kS, z)); g.art.push_back(a); g.band.push_back(j <= 1 ? 1 : 0); } }
        const auto split = [&](int i0, int i1, int j) { return boxer && std::min(i0, i1) == H - 1 && std::max(i0, i1) == H && g.rowY[size_t(j)] > yC; };   // não liga as duas pernas
        for (int f = 0; f < 2; f++) { Panel& pn = g.panel[f]; const auto A = [&](int i, int j) { return (i < 0 || j < 0 || i >= NX || j >= NY) ? -1 : pn.at[size_t(j * NX + i)]; };
            const auto L = [&](int a, int b, float k) { if (a < 0 || b < 0 || a == b) return; g.links.push_back({a, b, glm::length(g.rest[size_t(a)] - g.rest[size_t(b)]), k}); };
            for (int j = 0; j < NY; j++) for (int i = 0; i < NX; i++) { const int a = A(i, j); if (a < 0) continue; const bool bd = j <= 1;
                if (!split(i, i + 1, j)) L(a, A(i + 1, j), 1.f); L(a, A(i, j + 1), 1.f);
                if (!split(i, i + 1, j) && !split(i, i + 1, std::min(j + 1, NY - 1))) { L(a, A(i + 1, j + 1), 0.8f); L(A(i + 1, j), A(i, j + 1), 0.8f); }
                if (!split(i, i + 1, j) && !split(i + 1, i + 2, j)) L(a, A(i + 2, j), bd ? 0.8f : 0.4f); L(a, A(i, j + 2), 0.4f); }   // dobra: o cós é firme, o pano dobra macio
            // bordas abertas: cintura, aberturas das pernas (calcinha) e barras (cueca)
            for (int i = 0; i + 1 < NX; i++) if (A(i, 0) != A(i + 1, 0)) pn.rims.push_back({i, i + 1});
            if (!boxer) { for (int jj = 0; jj + 1 < NY; jj++) for (int i : {0, NX - 1}) if (!(shared(i, jj) && shared(i, jj + 1))) pn.rims.push_back({jj * NX + i, (jj + 1) * NX + i}); }
            else for (int i = 0; i + 1 < NX; i++) if (!split(i, i + 1, NY - 1)) pn.rims.push_back({(NY - 1) * NX + i, (NY - 1) * NX + i + 1}); }
        // lacinho (calcinha) ou etiqueta (cueca) na frente da cintura
        { const Panel& pn = g.panel[0]; const int ci = boxer ? H : NX / 2, cj = boxer ? 0 : 1; g.bow = pn.at[size_t(cj * NX + ci)]; g.bowL = pn.at[size_t(cj * NX + ci - 2)]; g.bowR = pn.at[size_t(cj * NX + ci + 1)]; g.bowD = pn.at[size_t((cj + 1) * NX + ci)]; }
    }

    // ---------- lançamento ----------
    // de um ponto da torcida, em arco, até um ponto no ringue; algumas roupas vão parar em cima das cordas
    void spawn(bool undies, bool onRope = false) {
        { int alive = 0; for (const Item& o : items) if (o.dying < 0) alive++;   // fluxo contínuo: a mais antiga some (com efeito, ver kPop)
          static const int cap = std::getenv("KORAMA3D_POPCAP") ? std::atoi(std::getenv("KORAMA3D_POPCAP")) : 70;   // KORAMA3D_POPCAP: conferência do sumiço
          if (alive >= cap) for (Item& o : items) if (o.dying < 0) { o.dying = 0; break; } }
        Item it; it.cloth = undies; it.kind = undies ? kRoseKinds + int(rnd(0.f, float(kUnderKinds) - 0.01f)) : int(rnd(0.f, float(kRoseKinds) - 0.01f)); it.life = 100000; it.phase = rnd(0.f, 6.28f);
        glm::vec3 from, to; const float T = rnd(50.f, 72.f);
        if (undies && (onRope || rnd(0.f, 1.f) < 0.45f)) {   // nas cordas: vem do lado de fora daquele lado
            const int s = int(rnd(0.f, 3.99f)); const glm::vec2 a = RingRopes::corner(s), b = RingRopes::corner(s + 1), on = a + (b - a) * rnd(0.2f, 0.8f), out = RingRopes::outward(s);
            to = glm::vec3(on.x, topRope() + 0.16f, on.y) - glm::vec3(out.x, 0, out.y) * 0.2f; const glm::vec2 f = on + out * rnd(2.5f, 3.5f) + glm::vec2(-out.y, out.x) * rnd(-1.f, 1.f); from = glm::vec3(f.x, rnd(2.f, 2.8f), f.y);
        } else { const float a = rnd(0.f, 6.2832f), ra = rnd(0.f, 6.2832f), rr = rnd(0.4f, 2.2f); from = glm::vec3(std::cos(a) * 6.5f, rnd(2.2f, 3.2f), std::sin(a) * 6.5f); to = glm::vec3(std::cos(ra) * rr, 0.12f, std::sin(ra) * rr); }
        // arco alto (passa por cima das cordas) chegando no alvo em T quadros
        const glm::vec3 v((to.x - from.x) / T, (to.y - from.y) / T + 0.5f * kG * T, (to.z - from.z) / T);
        const glm::quat q = glm::normalize(glm::quat(rnd(-1.f, 1.f), rnd(-1.f, 1.f), rnd(-1.f, 1.f), rnd(-1.f, 1.f)));
        if (!undies) { it.pos = from; it.vel = v; it.q = q; it.w = glm::normalize(glm::vec3(rnd(-1.f, 1.f), rnd(-1.f, 1.f), rnd(-1.f, 1.f))) * rnd(0.12f, 0.26f); }
        else { const Garment& g = garment[clothDef(it.kind - kRoseKinds).boxer ? 1 : 0]; const glm::vec3 w = glm::normalize(glm::vec3(rnd(-1.f, 1.f), rnd(-1.f, 1.f), rnd(-1.f, 1.f))) * rnd(0.05f, 0.1f);   // gira rodopiando
            it.p.resize(g.rest.size()); it.o.resize(g.rest.size());
            for (size_t k = 0; k < g.rest.size(); k++) { const glm::vec3 r = q * g.rest[k]; it.p[k] = from + r; it.o[k] = from + r - (v + glm::cross(w, r)); } }
        items.push_back(std::move(it));
    }

    // conferência: solta uma peça de roupa (kind 0..11) parada, de pé, na altura dada
    void drop(int c, const glm::vec3& at, float yaw) {
        Item it; it.cloth = true; it.kind = kRoseKinds + c; it.life = 100000; const Garment& g = garment[clothDef(c).boxer ? 1 : 0]; const glm::quat q = glm::angleAxis(yaw, glm::vec3(0, 1, 0));
        it.p.resize(g.rest.size()); it.o.resize(g.rest.size()); for (size_t k = 0; k < g.rest.size(); k++) { it.p[k] = at + q * g.rest[k]; it.o[k] = it.p[k]; } items.push_back(std::move(it));
    }
    // ---------- física ----------
    // sumiço: incha um pouco, gira subindo e encolhe até sumir num brilho de estrelinhas (nada some de repente)
    static constexpr int kPop = 26;
    static float popScale(int d) { if (d < 0) return 1.f; const float t = float(d) / float(kPop); if (t < 0.22f) return 1.f + 0.22f * std::sin(t / 0.22f * 1.5708f); const float u = (t - 0.22f) / 0.78f; return 1.22f * (1.f - u * u * (3.f - 2.f * u)); }
    static glm::vec3 center(const Item& it) { if (!it.cloth) return it.pos; glm::vec3 c(0.f); for (const glm::vec3& q : it.p) c += q; return c / float(std::max<size_t>(1, it.p.size())); }
    void tick(const RingRopes& r, Particles* fx = nullptr) { ropes = &r;
        for (Item& it : items) { it.life--; if (it.life <= 0 && it.dying < 0) it.dying = 0; if (it.dying >= 0) it.dying++; if (it.cloth) cloth(it, fx); else roseStep(it, fx);
            if (it.dying == kPop && fx) { const glm::vec3 c = center(it) + glm::vec3(0, 0.25f, 0);   // estrelinhas
                for (int k = 0; k < 10; k++) { Particle q; q.kind = kGlow; q.p = c; const glm::vec3 d = fx->dir(); q.v = d * fx->rnd(0.012f, 0.03f); q.size = fx->rnd(0.025f, 0.045f); q.life = fx->rnd(14, 24); q.drag = 0.9f; q.noFloor = true;
                    q.c0 = glm::vec4(1.f, 0.97f, 0.8f, 1.f); q.c1 = glm::vec4(1.f, 0.7f, 0.85f, 1.f); fx->emit(q); }
                for (int k = 0; k < 8; k++) { Particle q; q.kind = kLine; q.p = c; const glm::vec3 d = fx->dir(); q.v = d * fx->rnd(0.04f, 0.07f); q.size = fx->rnd(0.02f, 0.035f); q.life = fx->rnd(8, 12); q.drag = 0.85f; q.noFloor = true;
                    q.c0 = glm::vec4(1.f, 1.f, 0.9f, 1.f); q.c1 = glm::vec4(1.f, 0.8f, 0.9f, 0.f); fx->emit(q); } } }
        items.erase(std::remove_if(items.begin(), items.end(), [](const Item& it) { return it.dying >= kPop || (it.cloth ? it.p[0].y < -3.f : it.pos.y < -3.f); }), items.end());
    }
    static bool onRing(const glm::vec3& p) { return std::max(std::fabs(p.x), std::fabs(p.z)) < RingRopes::kHalf + 0.35f; }
    // quem já está dentro do ringue não passa pelas cordas (até um pouco acima da de cima): bate e volta
    bool wall(glm::vec3& p, glm::vec3& vel, const glm::vec3& prev) const {
        const float H = RingRopes::kHalf - 0.03f; if (std::max(std::fabs(prev.x), std::fabs(prev.z)) > H || p.y > topRope() + 0.06f) return false; bool hit = false;
        if (std::fabs(p.x) > H) { p.x = std::copysign(H, p.x); vel.x = -vel.x * 0.35f; hit = true; }
        if (std::fabs(p.z) > H) { p.z = std::copysign(H, p.z); vel.z = -vel.z * 0.35f; hit = true; }
        return hit;
    }
    static void puff(Particles* fx, const glm::vec3& at, int n, float s) {
        if (!fx) return; for (int i = 0; i < n; i++) { const float a = fx->rnd(0.f, 6.2832f); const glm::vec3 d(std::cos(a), 0.f, std::sin(a)); Particle q; q.kind = kPuff; q.p = glm::vec3(at.x, kFloor + 0.03f, at.z) + d * 0.05f;
            q.v = d * fx->rnd(0.006f, 0.016f) * s + glm::vec3(0, fx->rnd(0.002f, 0.006f), 0); q.size = fx->rnd(0.05f, 0.09f) * s; q.grow = 0.002f; q.drag = 0.92f; q.life = fx->rnd(18, 28); q.c0 = glm::vec4(0.95f, 0.92f, 0.86f, 1.f); q.c1 = glm::vec4(0.84f, 0.8f, 0.74f, 1.f); fx->emit(q); }
    }
    // pontos da rosa que encostam no chão (local)
    void roseStep(Item& it, Particles* fx) {
        it.sqV += -0.2f * it.sq - 0.14f * it.sqV; it.sq += it.sqV;   // mola do amasso
        if (it.still > 40) return;
        const glm::vec3 prev = it.pos; it.vel.y -= kG; it.pos += it.vel; wall(it.pos, it.vel, prev);
        const float wl = glm::length(it.w); if (wl > 1e-5f) it.q = glm::normalize(glm::angleAxis(wl, it.w / wl) * it.q);
        if (!onRing(it.pos)) return;   // fora do ringue não tem chão
        static const glm::vec3 pts[] = {{0, 0.085f, 0}, {0.05f, 0.04f, 0}, {-0.05f, 0.04f, 0}, {0, 0.04f, 0.05f}, {0, 0.04f, -0.05f}, {0, -0.235f, 0}, {0.03f, -0.08f, 0}};
        float lo = 1e9f; for (const glm::vec3& l : pts) lo = std::min(lo, (it.q * (l * kRS)).y + it.pos.y);
        if (lo < kFloor) { it.pos.y += kFloor - lo;
            if (it.vel.y < -0.006f) {   // quica: amassa, solta poeira e tomba
                it.sqV += std::min(0.6f, -it.vel.y * 7.f); it.vel.y = -it.vel.y * (it.bounces == 0 ? 0.42f : 0.22f); it.bounces++;
                it.w = it.w * 0.6f + glm::vec3(it.vel.z, 0.f, -it.vel.x) * 6.f; }
            else it.vel.y = 0.f;
            it.vel.x *= 0.78f; it.vel.z *= 0.78f; it.w *= 0.85f; it.landed = true;
            // deita de lado: a haste vai ficando na horizontal
            glm::vec3 ax = it.q * glm::vec3(0, 1, 0); glm::vec3 hz(ax.x, 0.f, ax.z); if (glm::length(hz) < 1e-3f) hz = glm::vec3(1, 0, 0); hz = glm::normalize(hz) + glm::vec3(0, 0.08f, 0);
            const glm::quat lie = glm::rotation(glm::normalize(ax), glm::normalize(hz)) * it.q; it.q = glm::normalize(glm::slerp(it.q, lie, 0.12f));
            if (glm::length(it.vel) < 0.003f && it.bounces > 0) it.still++; }
    }
    void cloth(Item& it, Particles* fx) {
        if (it.still > 90) return;
        const Garment& g = garment[clothDef(it.kind - kRoseKinds).boxer ? 1 : 0];
        // no chão o pano vai encolhendo e levanta calombos que desabam em dobras: termina amassado
        int down = 0; for (const glm::vec3& q : it.p) if (q.y < kFloor + 0.025f) down++;
        if (down > int(it.p.size()) / 4 && it.crumple < 1.f) { it.flopped = true;
            it.crumple = std::min(1.f, it.crumple + 0.04f); }
        // Verlet: voando quase não freia (o arco foi calculado sem ar) e tremula; depois de encostar, o ar segura
        const float damp = it.caught || it.crumple > 0.f ? 0.985f : 0.9995f; float moving = 0.f; const bool flying = !it.caught && it.crumple <= 0.f;
        for (size_t k = 0; k < it.p.size(); k++) { glm::vec3 v = (it.p[k] - it.o[k]) * damp; if (flying) v += glm::vec3(rnd(-1.f, 1.f), rnd(-1.f, 1.f), rnd(-1.f, 1.f)) * 0.0007f;
            it.o[k] = it.p[k]; it.p[k] += v + glm::vec3(0, -kG, 0); moving = std::max(moving, glm::length(v)); }
        const float sh = 1.f;   // no chão só dobra macio, não encolhe
        for (int iter = 0; iter < 4; iter++) {
            for (const Link& l : g.links) { glm::vec3& a = it.p[size_t(l.a)]; glm::vec3& b = it.p[size_t(l.b)]; const glm::vec3 d = b - a; const float L = glm::length(d); if (L < 1e-6f) continue; const float rest = l.r * sh;
                const glm::vec3 c = d * ((L - rest) / L * 0.5f * l.k); a += c; b -= c; }   // estica pouco, encolhe fácil: é isso que amassa
            for (size_t k = 0; k < it.p.size(); k++) { const bool air = it.p[k].y > kFloor + 0.002f; if (collide(it.p[k], it.o[k]) && !it.caught) { it.caught = true; for (size_t m = 0; m < it.p.size(); m++) it.o[m] = it.p[m] - (it.p[m] - it.o[m]) * 0.15f; }   // a corda segura: o pano para e fica pendurado
                if (iter == 0 && air && it.p[k].y <= kFloor + 0.001f) { const float a = rnd(0.f, 6.2832f), m = rnd(0.0005f, 0.0015f); it.o[k] -= glm::vec3(std::cos(a) * m, -rnd(0.f, 0.003f), std::sin(a) * m); } }   // bateu no chão: dobrinhas macias
        }
        if (moving < 0.0012f) it.still++; else it.still = 0;
    }
    // chão (com atrito, só no ringue), cordas (cilindros entre os cantos) e a parede das cordas; devolve se encostou numa corda
    bool collide(glm::vec3& p, glm::vec3& o) const {
        { glm::vec3 v = p - o; if (wall(p, v, o)) o = p - v; }
        if (onRing(p) && p.y < kFloor) { p.y = kFloor; o.x += (p.x - o.x) * 0.55f; o.z += (p.z - o.z) * 0.55f; o.y = p.y; }
        if (std::fabs(p.x) > RingRopes::kHalf + 0.3f || std::fabs(p.z) > RingRopes::kHalf + 0.3f || p.y > 1.8f) return false; bool hit = false;
        for (int s = 0; s < 4; s++) { const glm::vec2 a2 = RingRopes::corner(s), b2 = RingRopes::corner(s + 1), out = RingRopes::outward(s);
            for (int row = 0; row < (ropes ? ropes->rows : 3); row++) { const float h = ropes ? ropes->heights[row] : RingRopes::kHeights[row];
                const glm::vec3 a(a2.x, h, a2.y), b(b2.x, h, b2.y), ab = b - a; const float t = std::clamp(glm::dot(p - a, ab) / glm::dot(ab, ab), 0.f, 1.f);
                const glm::vec3 c = a + ab * t, d = p - c; const float L = glm::length(d); if (L >= kRopeR || L < 1e-6f) continue;
                hit = true; p = c + d / L * kRopeR; const glm::vec3 v = p - o, nrm = d / L; o = p - (v - nrm * std::min(0.f, glm::dot(v, nrm))) * 0.9f;   // tira a velocidade contra a corda e freia um pouco
                o += glm::vec3(out.x, 0.f, out.y) * 0.0016f; } }   // empurrãozinho para dentro: vai escorregando para o ringue
        // pendurado do lado de fora das cordas: escorrega para dentro do ringue
        const float ex = std::max(std::fabs(p.x), std::fabs(p.z)) - RingRopes::kHalf; if (ex > -0.05f && p.y > kFloor + 0.05f) { const glm::vec2 in = -glm::normalize(std::fabs(p.x) > std::fabs(p.z) ? glm::vec2(p.x, 0.f) : glm::vec2(0.f, p.z)); o -= glm::vec3(in.x, 0.f, in.y) * 0.0012f; }
        return hit;
    }

    // ---------- malha do quadro ----------
    // por vértice: pos, normal, cor, uv (no atlas; -1 sem textura), contorno
    void build(std::vector<float>& v, std::vector<unsigned>& idx, float t) const {
        v.clear(); idx.clear();
        const auto put = [&](const glm::vec3& p, const glm::vec3& n, const glm::vec3& c, float u, float uv, float ink) { const float d[12] = {p.x, p.y, p.z, n.x, n.y, n.z, c.r, c.g, c.b, u, uv, ink}; v.insert(v.end(), d, d + 12); };
        const auto putMesh = [&](const Mesh& m) { const unsigned base = unsigned(v.size() / 12); for (const PV& pv : m.v) put(pv.p, pv.n, pv.c, -1.f, -1.f, pv.ink); for (unsigned k : m.idx) idx.push_back(base + k); };
        for (const Item& it : items) {
            if (!it.cloth) { const unsigned base = unsigned(v.size() / 12); const Mesh& m = rose[it.kind % kRoseKinds]; const glm::mat3 R = glm::mat3_cast(it.q);
                // voando: estica na direção do voo; no chão: amassa a partir do ponto mais baixo e as pétalas balançam
                const float sp = glm::length(it.vel), st = !it.landed && sp > 0.01f ? 1.f + std::min(0.45f, sp * 3.5f) : 1.f; const glm::vec3 dir = sp > 1e-5f ? it.vel / sp : glm::vec3(0, 1, 0);
                const float sq = std::clamp(it.sq, -0.3f, 0.5f), jig = sq * 0.7f + 0.03f * std::sin(t * 2.f + it.phase) * (it.landed ? 0.f : 1.f);
                const float ps = popScale(it.dying), pt = it.dying < 0 ? 0.f : float(it.dying) / float(kPop); const glm::quat spin = glm::angleAxis(pt * pt * 9.f, glm::vec3(0, 1, 0)); const glm::vec3 lift(0.f, 0.35f * pt * pt, 0.f);
                for (const PV& pv : m.v) { const glm::vec3 l = pv.p + pv.n * (pv.jig * jig * 0.035f * std::sin(pv.p.x * 90.f + pv.p.z * 70.f + t * 9.f) + pv.jig * sq * 0.014f);
                    glm::vec3 w = R * (l * kRS); if (st > 1.f) { const float a = glm::dot(w, dir); w = (w - dir * a) / std::sqrt(st) + dir * (a * st); }
                    const float y = w.y + it.pos.y - kFloor; w.y -= sq * y * 0.55f; w.x *= 1.f + sq * 0.32f; w.z *= 1.f + sq * 0.32f;
                    if (it.dying >= 0) w = spin * (w * ps) + lift;
                    put(it.pos + w, (it.dying >= 0 ? spin * (R * pv.n) : R * pv.n), pv.c, -1.f, -1.f, pv.ink); }
                for (unsigned k : m.idx) idx.push_back(base + k); continue; }
            // roupa: cada painel com espessura (camada de fora e de dentro, bordas fechadas), estampa do tecido e o cós mais grosso
            const int c = it.kind - kRoseKinds; const ClothDef& cd = clothDef(c); const Garment& g = garment[cd.boxer ? 1 : 0];
            const float cu = float((c % kAtlasCols) * kAtlasCell), cv = float((c / kAtlasCols) * kAtlasCell), wu = float((kWhiteCell % kAtlasCols) * kAtlasCell + kAtlasCell / 2) / float(kAtlasSize), wv = float((kWhiteCell / kAtlasCols) * kAtlasCell + kAtlasCell / 2) / float(kAtlasSize);
            const glm::vec3 elastic = hexLin(cd.elastic); constexpr float kT = 0.008f, kTB = 0.016f, kInk = 0.005f; const int H = g.NX / 2;
            const float ps = popScale(it.dying), pt = it.dying < 0 ? 0.f : float(it.dying) / float(kPop); const glm::vec3 cen = center(it); const glm::quat spin = glm::angleAxis(pt * pt * 9.f, glm::vec3(0, 1, 0));
            const auto tf = [&](const glm::vec3& q) { return it.dying < 0 ? q : cen + spin * ((q - cen) * ps) + glm::vec3(0.f, 0.35f * pt * pt, 0.f); };
            const auto tn = [&](const glm::vec3& n) { return it.dying < 0 ? n : spin * n; };
            const unsigned firstV = unsigned(v.size() / 12);
            for (int f = 0; f < 2; f++) { const Panel& pn = g.panel[f];
                const auto A = [&](int i, int j) { return (i < 0 || j < 0 || i >= g.NX || j >= g.NY) ? -1 : pn.at[size_t(j * g.NX + i)]; };
                std::vector<int> outV(pn.at.size(), -1), inV(pn.at.size(), -1);
                for (int j = 0; j < g.NY; j++) for (int i = 0; i < g.NX; i++) { const int a = A(i, j); if (a < 0) continue; const glm::vec3 P = it.p[size_t(a)];
                    const bool legL = g.boxer && g.rowY[size_t(j)] > 102.f && i == H - 1, legR = g.boxer && g.rowY[size_t(j)] > 102.f && i == H;   // na beira da perna não olha a outra perna
                    const int xl = legR ? -1 : A(i - 1, j), xr = legL ? -1 : A(i + 1, j), yu = A(i, j - 1), yd = A(i, j + 1);
                    const glm::vec3 dI = (xr >= 0 ? it.p[size_t(xr)] : P) - (xl >= 0 ? it.p[size_t(xl)] : P), dJ = (yd >= 0 ? it.p[size_t(yd)] : P) - (yu >= 0 ? it.p[size_t(yu)] : P);
                    glm::vec3 n = glm::cross(dI, dJ) * pn.sgn; const float L = glm::length(n); n = L > 1e-9f ? n / L : glm::vec3(0, 1, 0);
                    const glm::vec2 art = g.art[size_t(a)]; const bool band = g.band[size_t(a)] != 0; const float th = band ? kTB : kT;
                    const float u = band ? wu : (cu + art.x / 200.f * float(kAtlasCell)) / float(kAtlasSize), vv = band ? wv : (cv + art.y / 200.f * float(kAtlasCell)) / float(kAtlasSize);
                    const glm::vec3 col = band ? elastic : glm::vec3(1.f);
                    outV[size_t(j * g.NX + i)] = int(v.size() / 12); put(tf(P + n * th * ps), tn(n), col, u, vv, kInk);
                    inV[size_t(j * g.NX + i)] = int(v.size() / 12); put(tf(P - n * th * ps), tn(-n), col * 0.95f, u, vv, 0.f); }   // o contorno é só da camada de fora (o de dentro apareceria pela cintura)
                for (int j = 0; j + 1 < g.NY; j++) for (int i = 0; i + 1 < g.NX; i++) {
                    if (g.boxer && i == H - 1 && g.rowY[size_t(j + 1)] > 102.f) continue;   // entre as pernas
                    const size_t ka = size_t(j * g.NX + i), kb = ka + 1, kc = ka + size_t(g.NX), kd = kc + 1; if (outV[ka] < 0 || outV[kb] < 0 || outV[kc] < 0 || outV[kd] < 0) continue;
                    const auto tri = [&](const std::vector<int>& Lv, bool ccw) { const unsigned a = unsigned(Lv[ka]), b = unsigned(Lv[kb]), cc = unsigned(Lv[kc]), d = unsigned(Lv[kd]);
                        if (ccw) idx.insert(idx.end(), {a, b, cc, b, d, cc}); else idx.insert(idx.end(), {a, cc, b, b, cc, d}); };
                    tri(outV, pn.sgn > 0.f); tri(inV, pn.sgn < 0.f); }
                // bordas fechadas: uma faixa liga a camada de fora com a de dentro (o tecido tem corpo, como nas roupas dos personagens)
                for (const auto& [ka, kb] : pn.rims) { if (outV[size_t(ka)] < 0 || outV[size_t(kb)] < 0) continue; const unsigned a = unsigned(outV[size_t(ka)]), b = unsigned(outV[size_t(kb)]), ai = unsigned(inV[size_t(ka)]), bi = unsigned(inV[size_t(kb)]);
                    idx.insert(idx.end(), {a, b, bi, a, bi, ai}); }
                if (cd.ruffle) { Mesh m; const glm::vec3 rc = hexLin(cd.ruffle); int k = 0;
                    for (const auto& [ka, kb] : pn.rims) { const int pa = pn.at[size_t(ka)], pb = pn.at[size_t(kb)]; if (pa < 0 || pb < 0 || pa == pb) continue;
                        const int ia = ka % g.NX, ja = ka / g.NX, ib = kb % g.NX, jb = kb / g.NX; const bool waist = ja == 0 && jb == 0;
                        const int qa = waist ? A(ia, 1) : A(ia == 0 ? 1 : ia - 1, ja), qb = waist ? A(ib, 1) : A(ib == 0 ? 1 : ib - 1, jb); if (qa < 0 || qb < 0) continue;   // um ponto para dentro do pano
                        const glm::vec3 PA = it.p[size_t(pa)], PB = it.p[size_t(pb)], mid = (PA + PB) * 0.5f, inner = (it.p[size_t(qa)] + it.p[size_t(qb)]) * 0.5f;
                        const glm::vec3 t = PB - PA, e = glm::normalize(mid - inner + glm::vec3(1e-5f)), nn = glm::normalize(glm::cross(t, e) + glm::vec3(1e-5f)); const float L = glm::length(t), wv = (k++ % 2 ? 1.f : -1.f);
                        blob(m, tf(mid + e * 0.024f * ps + nn * 0.006f * wv * ps), t * (0.62f * ps), e * (0.026f * ps), nn * (0.014f * ps), rc, 0.0012f, 0.f); (void)L; }
                    putMesh(m); } }
            // lacinho (calcinha) ou etiqueta (cueca) presos na frente da cintura
            if (g.bow >= 0 && g.bowL >= 0 && g.bowR >= 0 && g.bowD >= 0) { const glm::vec3 P = it.p[size_t(g.bow)], r = glm::normalize(it.p[size_t(g.bowR)] - it.p[size_t(g.bowL)] + glm::vec3(1e-5f));
                const glm::vec3 n = glm::normalize(-glm::cross(r, it.p[size_t(g.bowD)] - P) + glm::vec3(1e-5f));   // para fora do painel da frente
                const glm::vec3 up = glm::normalize(glm::cross(n, r) + glm::vec3(1e-5f)), ac = hexLin(cd.accent), base = tf(P + n * (kTB + 0.012f) * ps); const glm::vec3 R = tn(r) * ps, U = tn(up) * ps, N = tn(n) * ps; Mesh m;
                if (!cd.boxer) { blob(m, base + R * 0.04f, R * 0.038f, U * 0.026f, N * 0.014f, ac, 0.0035f, 0.f); blob(m, base - R * 0.04f, R * 0.038f, U * 0.026f, N * 0.014f, ac, 0.0035f, 0.f); blob(m, base, R * 0.015f, U * 0.015f, N * 0.016f, ac, 0.0035f, 0.f);
                    blob(m, base - U * 0.035f + R * 0.015f, R * 0.01f, U * 0.025f, N * 0.008f, ac, 0.0035f, 0.f); blob(m, base - U * 0.035f - R * 0.015f, R * 0.01f, U * 0.025f, N * 0.008f, ac, 0.0035f, 0.f); }   // laço com as pontinhas
                else { blob(m, base - U * 0.06f, R * 0.012f, U * 0.012f, N * 0.008f, ac, 0.0035f, 0.f); blob(m, base - U * 0.11f, R * 0.012f, U * 0.012f, N * 0.008f, ac, 0.0035f, 0.f); }   // botões da braguilha
                putMesh(m); }
        }
    }
};
}
