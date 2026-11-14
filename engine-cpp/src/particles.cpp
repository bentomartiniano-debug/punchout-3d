#include "particles.hpp"
#include <cstdlib>
#include <cmath>

namespace KoRama {

void ParticleManager::update(float dt) {
    for (size_t i = 0; i < particles.size(); ) {
        ParticleRenderData& p = particles[i];
        p.x += p.vx;
        p.y += p.vy;
        p.z += p.vz;
        p.vy -= 0.25f; // Gravidade
        p.life -= 0.035f;

        if (p.life <= 0.0f) {
            particles.erase(particles.begin() + i);
        } else {
            i++;
        }
    }
}

void ParticleManager::spawnImpact(float x, float y, float z, int count, uint32_t colorHex) {
    for (int i = 0; i < count; i++) {
        if (particles.size() >= MAX_PARTICLES) particles.erase(particles.begin());

        float angle = (static_cast<float>(std::rand()) / RAND_MAX) * 6.283185f;
        float speed = 2.0f + (static_cast<float>(std::rand()) / RAND_MAX) * 5.0f;

        ParticleRenderData p;
        p.x = x; p.y = y; p.z = z;
        p.vx = std::cos(angle) * speed;
        p.vy = 1.0f + (static_cast<float>(std::rand()) / RAND_MAX) * 4.0f;
        p.vz = std::sin(angle) * speed;
        p.size = 3.0f + (static_cast<float>(std::rand()) / RAND_MAX) * 4.0f;
        p.life = 1.0f;
        p.color_hex = colorHex;
        p.type = 0; // Impact

        particles.push_back(p);
    }
}

void ParticleManager::spawnBlood(float x, float y, float z, int count, uint32_t colorHex) {
    for (int i = 0; i < count; i++) {
        if (particles.size() >= MAX_PARTICLES) particles.erase(particles.begin());

        float angle = (static_cast<float>(std::rand()) / RAND_MAX) * 6.283185f;
        float speed = 1.5f + (static_cast<float>(std::rand()) / RAND_MAX) * 4.0f;

        ParticleRenderData p;
        p.x = x; p.y = y; p.z = z;
        p.vx = std::cos(angle) * speed;
        p.vy = 2.0f + (static_cast<float>(std::rand()) / RAND_MAX) * 5.0f;
        p.vz = std::sin(angle) * speed;
        p.size = 4.0f + (static_cast<float>(std::rand()) / RAND_MAX) * 5.0f;
        p.life = 1.0f;
        p.color_hex = colorHex;
        p.type = 1; // Blood

        particles.push_back(p);
    }
}

void ParticleManager::spawnStraw(float x, float y, float z, int count) {
    static const uint32_t kStrawColors[] = {
        0xfef08aff, // light golden yellow
        0xfacc15ff, // bright yellow straw
        0xeab308ff, // golden amber straw
        0xd97706ff, // toasted dry straw
        0xd4a373ff  // burlap / hay
    };
    for (int i = 0; i < count; i++) {
        if (particles.size() >= MAX_PARTICLES) particles.erase(particles.begin());

        float angle = (static_cast<float>(std::rand()) / RAND_MAX) * 6.283185f;
        float speed = 2.0f + (static_cast<float>(std::rand()) / RAND_MAX) * 5.5f;

        ParticleRenderData p;
        p.x = x; p.y = y; p.z = z;
        p.vx = std::cos(angle) * speed;
        p.vy = 2.5f + (static_cast<float>(std::rand()) / RAND_MAX) * 6.0f;
        p.vz = std::sin(angle) * speed;
        p.size = 3.5f + (static_cast<float>(std::rand()) / RAND_MAX) * 4.5f;
        p.life = 1.0f;
        p.color_hex = kStrawColors[std::rand() % 5];
        p.type = 0; // Impact

        particles.push_back(p);
    }
}

void ParticleManager::spawnDodgeSmoke(float x, float y, float z) {
    for (int i = 0; i < 6; i++) {
        if (particles.size() >= MAX_PARTICLES) particles.erase(particles.begin());

        ParticleRenderData p;
        p.x = x + (static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * 10.0f;
        p.y = y + (static_cast<float>(std::rand()) / RAND_MAX) * 6.0f;
        p.z = z;
        p.vx = (static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * 1.5f;
        p.vy = 0.5f + (static_cast<float>(std::rand()) / RAND_MAX) * 1.0f;
        p.vz = (static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * 1.5f;
        p.size = 6.0f + (static_cast<float>(std::rand()) / RAND_MAX) * 6.0f;
        p.life = 1.0f;
        p.color_hex = 0xddddddaa;
        p.type = 4; // Smoke

        particles.push_back(p);
    }
}

void ParticleManager::clear() {
    particles.clear();
}

void ParticleManager::fillRenderData(GameRenderState& state) const {
    state.particle_count = static_cast<uint32_t>(std::min(particles.size(), static_cast<size_t>(MAX_PARTICLES)));
    for (uint32_t i = 0; i < state.particle_count; i++) {
        state.particles[i] = particles[i];
    }
}

} // namespace KoRama
