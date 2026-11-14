// app3d/rig/rig.hpp: estado do lutador no motor -> transformações das peças (metros, destro).
#pragma once
#include "fighter.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
namespace KoRama {
constexpr float kRigPi = 3.14159265358979f;
constexpr float kRigUnit = 0.0125f;   // 1 unidade do motor em metros (Valéria: ~135 unidades = 1,7 m)
constexpr float kWorldUnit = 0.0122f; // posição no ringue e câmera: a escala do jogo (UNIT de arena_pernambuco.js)
constexpr float kGloveX = 0.018f;     // a guarda do vinil é mais aberta que a do motor
constexpr float kGloveZ0 = 0.15f;     // luva de guarda (z 12 no motor) um pouco à frente do corpo
constexpr float kGloveLift = 0.12f;   // a luva de guarda do motor (y 64) fica na altura do peito do boneco
constexpr float kSpread = 0.4f;   // metros a mais de cada lutador até o centro: mais espaço entre os dois para a câmera ver o oponente
constexpr float kCornerClear = 1.65f;   // na queda, o corpo fica a isto do poste: vai para a corda, não para dentro da quina
constexpr float kRopeGap = 0.1f;      // no fim do voo, o centro do corpo fica a isto da linha das cordas
constexpr float kStanceX = 0.1f, kStanceBack = 0.2f, kStanceFront = 0.1f;   // guarda de boxe: abertura e os pés à frente e atrás
constexpr float kGloveDrape = 0.14f;  // metros acima da corda de cima onde a luva apoia na queda (o raio da luva e uma folga)
constexpr float kFootSlack = 0.2f;   // metros: o pé plantado fica no máximo isto longe do lugar de guarda; mais que isso escorrega junto (senão vira perna de borracha na esquiva)
constexpr float kHipIn = 0.55f;        // o topo da perna fica mais perto do centro que o pé: no sprite o quadril é largo, mas em 3D isso fura a saia
constexpr float kGloveYaw = 0.35f;    // giro da guarda no vinil (hand.yaw)
enum class GloveMode { Umbrella, Fist, Saw, Stick, Hive };   // como a luva fica na mão (cada lutador do vinil gira a dele de um jeito)
struct RigRest { float WAIST = 0, NECK = 0, HIPS = 0, legLen = 0; glm::vec3 hip[2]{}, foot[2]{}; float scale = 1, handX = 32 * kGloveX, handY = 64 * kRigUnit + kGloveLift; float gloveZ0 = kGloveZ0; GloveMode glove = GloveMode::Umbrella; bool hasLegs = true, virtualFeet = false; float hipIn = kHipIn, stance = 1.f; };   // virtualFeet: sem pernas soltas, a parte de baixo da malha vai até dois pés que o rig planta (King Frog)   // pernas retas (sem saia): hipIn 1 e guarda menor
struct FighterPose {
    glm::mat4 root{1.f};
    glm::vec3 upperPos{}, hipsPos{}, glove[2]{}, footPos[2]{}, legPos[2]{}, legDirLen[2]{};
    glm::vec3 footShift[2]{};   // quanto cada pé saiu do lugar de descanso (espaço do modelo): leva a malha de baixo de quem tem pés virtuais
    float roll = 0, lean = 0, twist = 0, neckTilt = 0, neckTurn = 0;
    glm::vec3 gloveEuler[2]{}, gloveScale[2]{glm::vec3(1.f), glm::vec3(1.f)};   // giro de mão e a luva amassando no alvo
    glm::quat gloveRot[2]{glm::quat(1, 0, 0, 0), glm::quat(1, 0, 0, 0)};        // orientação final da luva
    float gloveOpen = 0, gloveSpin[2]{};                                        // sombrinha: 0 fechada, 1 aberta; giro em volta do cabo (radianos)
    glm::vec3 headScale{1.f}, bodyScale{1.f};                                   // deformação de quem apanhou
};
// só visual: até onde a luva vai (superfície do alvo) e o quanto o lutador está amassado
struct PoseCtx { float reachLimit = 1e9f; float squash = 0.f; bool hasPos = false; glm::vec3 pos{0.f}; float hangT = 0.f, ropeBounce = 0.f, rise = 0.f; float ropeTop = 98.f * 0.0122f; };   // ropeTop: altura da corda de cima da arena (metros), onde os braços ficam pendurados na queda   // rise: 0..1, o esforço para levantar (Z e X na contagem)   // hangT: quadros, balanço nas cordas; ropeBounce: o tranco ao bater nelas
// deformação de quem apanha: amassa de uma vez no golpe, fica assim durante o congelamento e volta balançando
struct ImpactSpring {
    float s = 0, v = 0;
    void kick(float amount) { s = amount; v = 0; }
    void tick() { v += -0.09f * s; v *= 0.84f; s += v; }
};
// atraso das peças soltas (kLag da Vivaldi no vinil): o corpo acelera e elas ficam para trás, voltando em mola
struct LagSpring {
    glm::vec3 prev{0.f}, vel{0.f}, o{0.f}, v{0.f}; bool ready = false;
    void tick(const glm::vec3& pos) { constexpr float dt = 1.f / 60.f; if (!ready) { prev = pos; ready = true; return; } const glm::vec3 nv = (pos - prev) / dt, a = (nv - vel) / dt; vel = nv; prev = pos; v += (o * -70.f + v * -6.f + a * -1.2f) * dt; o += v * dt; if (glm::length(o) > 0.09f) o = glm::normalize(o) * 0.09f; }
};
struct ClothSim {
    static constexpr int kN = 10;
    glm::vec3 o[kN]{}, v[kN]{}, prev{0.f}, vel{0.f}; float rotPrev = 0.f; bool ready = false;
    void tick(const glm::vec3& pos, const glm::mat4& root, float rot, float t);
};
// só visual: o motor troca a pose das luvas e do corpo de um quadro para o outro; aqui elas deslizam até lá.
// view é uma cópia do lutador com essas posições suavizadas, e é ela que vai para computePose.
struct PoseSmoother {
    Fighter view; bool ready = false; float open = 0, spin[2]{}; glm::vec3 pos{0.f};   // pos: posição suavizada no ringue
    void tick(const Fighter& f);
};
// índice 0 = lado -1 do boneco; 1 = lado +1. down: 0 de pé, 1 nocauteado, escorado nas cordas.
// posição do lutador no ringue 3D: compacta na luta e esticada até a corda quando o motor o empurra para longe (nocaute)
glm::vec3 rigPos(const Fighter& f);
FighterPose computePose(const Fighter& f, const RigRest& rest, float down, const PoseCtx& ctx = {});
// passos: cada pé fica plantado no mundo; quando o corpo se afasta demais de onde ele deveria estar, o pé levanta e vai até lá,
// um de cada vez. Sobrescreve footPos e as pernas da pose. Um tick por quadro de 60 Hz.
struct FootStepper {
    bool ready = false; glm::vec3 planted[2]{}, from[2]{}; float t[2]{-1.f, -1.f}; int next = 0, steps = 0;
    void tick(FighterPose& p, const RigRest& r);
};
}
