#ifndef KORAMA_VFX_LAB_COMMON_HPP
#define KORAMA_VFX_LAB_COMMON_HPP

#include "canvas2d.hpp"
#include "camera3d.hpp"
#include "fighter.hpp"
#include "physics.hpp"
#include "sprite_atlas.hpp"
#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

// Efeitos visuais dos golpes portados do Combat Lab (src/systems/combat_lab_effects.js).
// Cada lutador tem um módulo com o mesmo estado e as mesmas funções do JS; os nomes de variáveis
// e a ordem dos desenhos foram mantidos para que o visual seja o mesmo.
namespace KoRama::vfx {

using cv::Canvas;
using cv::PI;
using cv::Point3;

inline float rnd() { return cv::rnd(); }
inline float nowMs() { return cv::now(); }

struct Glove {
    float x = 0, y = 0, z = 0;
};

// O objeto "fighter" do Lab: estado, tempo da ação, luvas e corpo do lutador desenhado.
struct LabFighter {
    std::string state = "idle";
    int actionTimer = 0, maxActionTimer = 0;
    bool isLeft = false;
    struct {
        Glove gloveL, gloveR;
    } parts;
    struct {
        float offsetX = 0, offsetY = 0, bodySkew = 0;
        bool spinActive = false;
        float spinAngle = 0;
    } body;
};

// Contexto de cada chamada: câmera, lutador (motor) e a visão (de costas ou de frente).
struct LabContext {
    const Camera3D* camera = nullptr;
    const Fighter* entity = nullptr; // lutador do motor (status, vida, build)
    const Fighter* rival = nullptr;
    bool liveBody = false; // visual novo do elenco: o corpo (e a forma monstruosa) vem das peças
    const SpriteLibrary* sprites = nullptr;
    float bodyLift = 0.0f; // altura da base do sprite do corpo (ex.: -10 no King Frog)
    float bodyW = 110.0f, bodyH = 110.0f;
    LabFighter fighter;
    std::string activeCamera = "back"; // "back" (jogador) ou "front" (oponente)
    bool isPaused = false;             // true quando o quadro do motor ainda não avançou
    uint32_t frame = 0;

    float progress() const {
        return fighter.maxActionTimer > 0 ? 1.0f - static_cast<float>(fighter.actionTimer) / static_cast<float>(fighter.maxActionTimer) : 0.0f;
    }

    // projectFighterLocal do index.html: coordenadas locais do lutador -> tela.
    Point3 project3D(float lx, float ly, float lz) const {
        if (!camera || !entity) return {};
        Vec3 w = Physics3D::localToWorld(lx, ly, lz, entity->worldPos, entity->facing);
        Vec3 l = camera->toLocal(w);
        if (l.z <= 2.0f) return {};
        float scale = 450.0f / l.z;
        return {400.0f + l.x * scale + camera->shake.x, 300.0f - l.y * scale + camera->shake.y, l.z, scale};
    }

    Point3 projectWorldPos(float wx, float wy, float wz) const {
        if (!camera) return {};
        Vec3 l = camera->toLocal({wx, wy, wz});
        if (l.z <= 2.0f) return {};
        float scale = 450.0f / l.z;
        return {400.0f + l.x * scale + camera->shake.x, 300.0f - l.y * scale + camera->shake.y, l.z, scale};
    }

    Point3 projectWorldFloor(float wx, float wz) const { return projectWorldPos(wx, 0.0f, wz); }
};

// Interface de um lutador do Lab. update() roda uma vez por quadro do motor; os draw* seguem as
// camadas do drawEntityCharacter do JS (chão, atrás do corpo, sobre o corpo e à frente).
class LabFx {
public:
    virtual ~LabFx() = default;
    virtual void update(LabContext&) {}
    virtual void drawFloor(Canvas&, LabContext&) {}
    virtual void drawBackground(Canvas&, LabContext&) {}
    virtual void drawForeground(Canvas&, LabContext&) {}
    virtual void drawBodyOverlay(Canvas&, LabContext&, float /*bodyW*/, float /*bodyH*/) {}
    // Estágio da boca/corpo (getMouthStage do JS): "0".."6", "inflated", "mud_spit", "no_crown", ...
    virtual std::string mouthStage(const LabContext&) const { return "0"; }
    virtual std::string clawStage(const LabContext&) const { return "closed"; }
    virtual bool overridesBody(const LabContext&) const { return false; } // ex.: tornado do Cremoso
    virtual void drawBodyReplacement(Canvas&, LabContext&, sf::Vector2f, float, float, float) {}
};

std::unique_ptr<LabFx> makeLabFx(uint32_t charId);

} // namespace KoRama::vfx

#endif // KORAMA_VFX_LAB_COMMON_HPP
