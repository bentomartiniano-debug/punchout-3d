#ifndef KORAMA_PARTICLES_HPP
#define KORAMA_PARTICLES_HPP

#include "bridge_types.h"
#include <vector>

namespace KoRama {

class ParticleManager {
public:
    std::vector<ParticleRenderData> particles;

    void update(float dt);
    void spawnImpact(float x, float y, float z, int count, uint32_t colorHex);
    void spawnBlood(float x, float y, float z, int count, uint32_t colorHex);
    void spawnStraw(float x, float y, float z, int count);
    void spawnDodgeSmoke(float x, float y, float z);
    void clear();

    void fillRenderData(GameRenderState& state) const;
};

} // namespace KoRama

#endif // KORAMA_PARTICLES_HPP
