#ifndef KORAMA_PHYSICS_HPP
#define KORAMA_PHYSICS_HPP

#include "bridge_types.h"
#include <cmath>

namespace KoRama {

class Physics3D {
public:
    static constexpr float RING_RADIUS_MAX = 215.0f;
    static constexpr float RING_RADIUS_DEFAULT = 125.0f;
    static constexpr float DEG2RAD = 3.14159265358979323846f / 180.0f;

    static Vec3 localToWorld(float lx, float ly, float lz, const Vec3& origin, float facing) {
        float cosF = std::cos(facing);
        float sinF = std::sin(facing);
        return {
            origin.x + (lx * cosF + lz * sinF),
            origin.y + ly,
            origin.z + (-lx * sinF + lz * cosF)
        };
    }

    static Vec3 worldToLocal(float wx, float wy, float wz, const Vec3& origin, float facing) {
        float dx = wx - origin.x;
        float dy = wy - origin.y;
        float dz = wz - origin.z;
        float cosF = std::cos(facing);
        float sinF = std::sin(facing);
        return {
            dx * cosF - dz * sinF,
            dy,
            dx * sinF + dz * cosF
        };
    }

    static float distance3D(const Vec3& a, const Vec3& b) {
        float dx = a.x - b.x;
        float dy = a.y - b.y;
        float dz = a.z - b.z;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    static float clamp(float v, float minV, float maxV) {
        return (v < minV) ? minV : (v > maxV ? maxV : v);
    }
};

} // namespace KoRama

#endif // KORAMA_PHYSICS_HPP
