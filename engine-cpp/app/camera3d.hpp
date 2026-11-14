#ifndef KORAMA_CAMERA3D_HPP
#define KORAMA_CAMERA3D_HPP

#include "bridge_types.h"
#include <SFML/System/Vector2.hpp>
#include <optional>

namespace KoRama {

struct ScreenPoint {
    sf::Vector2f pos;
    float scale;
    float depth;
};

// Câmera de perspectiva igual à do jogo JS (updateCameraTrig/getLocal3D/projectLocal no index.html).
class Camera3D {
public:
    Vec3 pos{0.0f, 220.0f, -260.0f};
    float yaw = 0.0f;
    float pitch = 0.45f;
    sf::Vector2f shake{0.0f, 0.0f};

    float depthOf(const Vec3& world) const;
    std::optional<ScreenPoint> project(const Vec3& world) const;
    Vec3 toLocal(const Vec3& world) const;                          // x/y na tela, z = profundidade
    std::optional<ScreenPoint> projectLocal(const Vec3& local) const;
};

} // namespace KoRama

#endif // KORAMA_CAMERA3D_HPP
