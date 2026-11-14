// app3d/render/particles.hpp: o estouro do golpe em peças 3D com cel shading (bola de fogo, faíscas, lascas, riscos e a fumaça no chão).
// Só o estado e as contas, em metros por quadro de 60 Hz; quem desenha é o Renderer.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <glm/glm.hpp>
#include <vector>
namespace KoRama {
// kPuff bola com luz em três faixas (cor c0->c1); kFire bola de fogo; kSpark faísca com rastro; kFlake lasca girando; kLine risco/lasca de cor chapada
// kGlow bola acesa (sem sombra, brilha no bloom); kEnergy bola em faixas pelo ângulo com a câmera, na cor dada (magia, calda, lava)
// kFoam espuma de água: nuvem com contorno, branca na luz, azul-clara e azul com retícula na sombra
enum ParticleKind { kPuff = 0, kSpark = 2, kFlake = 3, kFire = 4, kLine = 5, kGlow = 6, kEnergy = 8, kFoam = 9 };
struct Particle { glm::vec3 p{0.f}, v{0.f}; glm::vec4 c0{1.f}, c1{1.f}; float age = 0, life = 1, size = 0.1f, grow = 0, gravity = 0, drag = 1, seed = 0; int kind = kPuff; static constexpr int kTrail = 9; glm::vec3 trail[kTrail]{}; int trailN = 0;
    // especiais: hold mantém o tamanho até o fim (projétil); órbita em volta de "axis" (que anda com v); len fixa o comprimento do risco na direção "dir" (raio, feixe);
    // jitter sacode a velocidade (abelha); noFloor atravessa o chão
    bool hold = false, noFloor = false; glm::vec3 axis{0.f}, dir{0.f}; float orbitA = 0, orbitV = 0, orbitR = 0, orbitG = 0, orbitY = 0, len = 0, jitter = 0; };
class Particles {
public:
    std::vector<Particle> list;
    void clear() { list.clear(); }
    void tick() {
        for (Particle& q : list) { q.age += 1.f;
            if (q.jitter > 0.f) { q.v += dir() * q.jitter; }
            if (q.orbitV != 0.f) { q.v.y -= q.gravity; q.v *= q.drag; q.axis += q.v; q.orbitA += q.orbitV; q.orbitR = std::max(0.f, q.orbitR + q.orbitG); q.p = q.axis + glm::vec3(std::cos(q.orbitA) * q.orbitR, q.orbitY, std::sin(q.orbitA) * q.orbitR); q.size += q.grow; continue; } if (q.kind == kSpark) { for (int i = Particle::kTrail - 1; i > 0; i--) q.trail[i] = q.trail[i - 1]; q.trail[0] = q.p; q.trailN = std::min(q.trailN + 1, Particle::kTrail); if (q.p.y < kFloor) q.age = q.life; } q.v.y -= q.gravity; q.v *= q.drag; q.p += q.v; q.size += q.grow; if (!q.noFloor && q.kind != kFire && q.kind != kSpark && q.p.y < kFloor) { q.p.y = kFloor; q.v.y *= -0.4f; } }
        list.erase(std::remove_if(list.begin(), list.end(), [](const Particle& q) { return q.age >= q.life; }), list.end());
    }
    // kind é o do ImpactFx: 0 leve, 1 pesado, 2 contra-ataque, 3 brecha, 4 queda
    void burst(const glm::vec3& at, int kind, uint32_t seed) {
        rng_ = seed * 2654435761u + 1u;
        const float s = kind == 0 ? 0.55f : kind == 4 ? 1.6f : 1.f; const auto n = [&](int k) { return int(float(k) * s); };
        { Particle f; f.kind = kFire; f.p = at; f.size = 0.2f * s; f.grow = 0.013f * s; f.drag = 0.9f; f.life = 18; f.c0 = {1.f, 0.5f, 0.1f, 1.f}; f.c1 = {0.42f, 0.3f, 0.26f, 1.f}; add(f); }
        for (int i = 0; i < n(7); i++) { Particle f; f.kind = kFire; f.p = at + dir() * 0.24f * s; f.v = dir() * rnd(0.008f, 0.022f); f.size = rnd(0.08f, 0.15f) * s; f.grow = 0.005f; f.gravity = -0.0005f; f.drag = 0.93f; f.life = rnd(14, 24); f.c0 = {1.f, 0.5f, 0.1f, 1.f}; f.c1 = {0.42f, 0.3f, 0.26f, 1.f}; add(f); }
        for (int i = 0; i < n(9); i++) { Particle f; f.kind = kLine; f.p = at + dir() * 0.3f * s; f.v = dir() * rnd(0.04f, 0.08f); f.size = rnd(0.03f, 0.05f); f.drag = 0.93f; f.life = rnd(8, 15); f.c0 = {0.12f, 0.07f, 0.06f, 1.f}; f.c1 = {0.12f, 0.07f, 0.06f, 0.f}; add(f); }
        for (int i = 0; i < n(22); i++) { Particle f; f.kind = kSpark; f.p = at; const glm::vec3 d = dir(); f.v = glm::vec3(d.x, std::fabs(d.y) * 0.8f + 0.25f, d.z) * rnd(0.035f, 0.085f); f.size = rnd(0.016f, 0.03f); f.gravity = rnd(0.0028f, 0.0044f); f.drag = 0.985f; f.life = rnd(30, 56); add(f); }
        for (int i = 0; i < n(12); i++) { Particle f; f.kind = kFlake; f.p = at; f.v = dir() * rnd(0.03f, 0.07f); f.size = rnd(0.02f, 0.04f); f.gravity = 0.0022f; f.drag = 0.97f; f.life = rnd(30, 52); f.c0 = i % 2 ? glm::vec4(0.98f, 0.45f, 0.14f, 1.f) : glm::vec4(0.93f, 0.2f, 0.12f, 1.f); f.c1 = f.c0; f.c1.a = 0.f; add(f); }
        const float mainDir = rnd(0.f, 6.283f), spread = rnd(0.7f, 3.14f); const int clumps = 2 + int(rnd(0.f, 3.99f)), puffs = n(int(rnd(14.f, 32.f)));
        float clumpA[6], clumpR[6]; for (int j = 0; j < clumps; j++) { clumpA[j] = mainDir + rnd(-spread, spread); clumpR[j] = rnd(0.12f, 0.7f); }
        for (int i = 0; i < puffs; i++) { const int j = i % clumps; const float a = clumpA[j] + rnd(-0.45f, 0.45f), r = std::max(0.05f, clumpR[j] + rnd(-0.16f, 0.2f)); Particle f; f.kind = kPuff; f.p = glm::vec3(at.x + std::cos(a) * r, kFloor + rnd(0.03f, 0.32f), at.z + std::sin(a) * r); f.v = glm::vec3(std::cos(a), rnd(0.1f, 0.6f), std::sin(a)) * rnd(0.01f, 0.034f); f.size = (rnd(0.f, 1.f) < 0.2f ? rnd(0.17f, 0.26f) : rnd(0.05f, 0.15f)) * s; f.grow = rnd(0.001f, 0.0035f); f.gravity = -0.00012f; f.drag = 0.965f; f.life = rnd(24, 56); f.c0 = {0.94f, 0.9f, 0.82f, 1.f}; f.c1 = {0.86f, 0.82f, 0.76f, 1.f}; add(f); }
    }
    // para quem monta efeitos (os especiais): sorteio e entrada direta
    void emit(Particle q) { add(q); }
    float rnd(float a, float b) { rng_ = rng_ * 1664525u + 1013904223u; return a + (b - a) * float(rng_ >> 8) / 16777216.f; }
    glm::vec3 dir() { const float y = rnd(-0.6f, 1.f), a = rnd(0.f, 6.283f), r = std::sqrt(std::max(0.f, 1.f - y * y)); return {std::cos(a) * r, y, std::sin(a) * r}; }
    static constexpr float kFloor = 0.03f;
private:
    static constexpr size_t kMax = 1400;
    uint32_t rng_ = 1;
    void add(Particle q) { q.seed = rnd(0.f, 6.283f); if (list.size() < kMax) list.push_back(q); }
};
}
