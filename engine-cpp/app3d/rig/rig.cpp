// app3d/rig/rig.cpp
#define GLM_ENABLE_EXPERIMENTAL
#include "rig/rig.hpp"
#include <glm/gtx/quaternion.hpp>
#include "rig/ropes.hpp"
#include <algorithm>
#include <cmath>
#include <glm/gtc/quaternion.hpp>
namespace KoRama {
static float smooth(float a, float b, float x) { const float t = std::clamp((x - a) / (b - a), 0.f, 1.f); return t * t * (3 - 2 * t); }
// giro de mão por golpe (pitch, yaw, roll da luva); side: +1 ou -1 do boneco. Pitch positivo aponta para baixo.
static glm::vec3 gloveTurn(const Fighter& f, bool isLeftGlove, float side) {
    glm::vec3 e(0, -kGloveYaw * side, 0);
    if (!isPunchState(f.state) || f.isLeftPunch != isLeftGlove || f.maxTimer <= 0) return e;
    const float t = 1.f - float(f.timer) / float(f.maxTimer), fade = smooth(0, 0.15f, t) * (1 - smooth(0.8f, 1, t)), turn = fade * smooth(0.15f, 0.5f, t);
    switch (f.state) {
    case FIGHTER_JAB: e.z = side * 1.4f * turn; break;
    case FIGHTER_CROSS: e.z = side * 2.2f * turn; e.y *= 1 - fade; break;
    // gancho: a ponta vem de fora e varre para dentro; a palma vira junto
    case FIGHTER_HOOK: e.y = (1 - fade) * e.y + fade * side * (0.9f - 1.9f * smooth(0.22f, 0.7f, t)); e.z = side * 1.6f * turn; break;
    // uppercut: sai apontando para baixo e sobe
    case FIGHTER_UPPER: e.x = fade * (0.8f - 1.9f * smooth(0.2f, 0.6f, t)); e.z = side * 0.8f * turn; break;
    case FIGHTER_BODY_BLOW: e.x = 0.45f * fade; e.z = side * 1.2f * turn; break;
    default: break;
    }
    return e;
}
glm::vec3 rigPos(const Fighter& f) {
    const float r = std::hypot(f.worldPos.x, f.worldPos.z); if (r < 1e-3f) return {0, f.worldPos.y * kRigUnit, 0};
    constexpr float kFight = 125.f, kRopeR = 232.f;   // raio de luta e o raio em que o motor marca que chegou às cordas
    const float dx = -f.worldPos.x / r, dz = f.worldPos.z / r, limit = RingRopes::kHalf / std::max(std::fabs(dx), std::fabs(dz)) - kRopeGap / std::max(std::fabs(dx), std::fabs(dz));
    // kSpread: cada lutador fica um pouco mais longe do centro que no motor, para a câmera ver o oponente; a luva compensa no soco (computePose)
    const float m = kSpread * std::min(1.f, r / kFight) + ((r <= kFight || f.state != FIGHTER_KNOCKDOWN) ? r * kWorldUnit : kFight * kWorldUnit + (limit - kSpread - kFight * kWorldUnit) * std::min(1.f, (r - kFight) / (kRopeR - kFight)));
    float x = dx * m, z = dz * m;
    if (f.state == FIGHTER_KNOCKDOWN && r > kFight) { const float side = RingRopes::kHalf - kCornerClear; if (std::fabs(x) >= std::fabs(z)) z = std::clamp(z, -side, side); else x = std::clamp(x, -side, side); }
    return {x, f.worldPos.y * kRigUnit, z};
}
namespace {
// o pé em relação ao lugar de descanso dele na malha (guarda inteira, de pé)
void shiftFeet(FighterPose& p, const RigRest& r) { for (int i = 0; i < 2; i++) p.footShift[i] = p.footPos[i] - (r.foot[i] + (i ? glm::vec3(kStanceX, 0, -kStanceBack) : glm::vec3(-kStanceX, 0, kStanceFront)) * r.stance); }
}
void FootStepper::tick(FighterPose& p, const RigRest& r) {
    if (!r.hasLegs && !r.virtualFeet) return;
    constexpr float kRate = 1.f / 11.f, kLift = 0.13f;   // passo de 11 quadros
    // metros: quanto o pé fica para trás antes de dar o passo; perna reta e curta (sem saia escondendo) passa a diagonal logo, então passo mais cedo
    const float tight = r.stance < 1.f ? 0.55f : 1.f, kStart = 0.17f * tight, kFar = 0.42f * tight, slack = kFootSlack * tight;
    const glm::mat4 toLocal = glm::inverse(p.root);
    glm::vec3 want[2];
    for (int i = 0; i < 2; i++) want[i] = glm::vec3(p.root * glm::vec4(p.footPos[i], 1));
    if (!ready) { planted[0] = want[0]; planted[1] = want[1]; ready = true; }
    const auto far = [&](int i) { return glm::length(glm::vec2(want[i].x - planted[i].x, want[i].z - planted[i].z)); };
    // começa o passo: o pé mais longe primeiro; os dois juntos só se o corpo foi longe demais (voo, esquiva)
    const int a = far(1) > far(0) ? 1 : (far(0) > far(1) ? 0 : next), b = 1 - a;
    for (const int i : {a, b}) if (t[i] < 0 && far(i) > kStart && (t[1 - i] < 0 || far(i) > kFar)) { from[i] = planted[i]; t[i] = 0; next = 1 - i; steps++; }
    for (int i = 0; i < 2; i++) {
        glm::vec3 w = planted[i];
        if (t[i] >= 0) { t[i] += kRate; const float k = std::min(1.f, t[i]), e = k * k * (3 - 2 * k); w = glm::mix(from[i], want[i], e); w.y += kLift * std::sin(k * kRigPi); if (t[i] >= 1) { planted[i] = want[i]; t[i] = -1; } }
        else w.y = want[i].y;
        // no ar (salto) o pé vai junto com o corpo, sem ficar plantado
        if (p.footPos[i].y - r.foot[i].y > 0.06f) { planted[i] = want[i]; t[i] = -1; w = want[i]; }   // footPos aqui ainda é o lugar de guarda, com a subida do salto
        const glm::vec3 home = p.footPos[i]; p.footPos[i] = glm::vec3(toLocal * glm::vec4(w, 1));
        // longe demais do lugar de guarda (esquiva rápida): o pé escorrega junto em vez de a perna esticar na diagonal
        { const glm::vec2 d(p.footPos[i].x - home.x, p.footPos[i].z - home.z); const float L = glm::length(d);
          if (L > slack) { const glm::vec2 c = d * (slack / L); p.footPos[i].x = home.x + c.x; p.footPos[i].z = home.z + c.y; const glm::vec3 nw(p.root * glm::vec4(p.footPos[i], 1)); if (t[i] < 0) planted[i] = glm::vec3(nw.x, planted[i].y, nw.z); else { from[i].x = nw.x; from[i].z = nw.z; } } }
        const float legBottom = r.hip[i].y - r.legLen + (p.footPos[i].y - r.foot[i].y);
        p.legDirLen[i] = glm::vec3(p.footPos[i].x, legBottom, p.footPos[i].z) - p.legPos[i];
    }
    shiftFeet(p, r);
}
void ClothSim::tick(const glm::vec3& pos, const glm::mat4& root, float rot, float t) {
    constexpr float dt = 1.f / 60.f, k = 38.f, kMax = 0.12f;
    if (!ready) { prev = pos; rotPrev = rot; ready = true; return; }
    const glm::vec3 nv = (pos - prev) / dt; glm::vec3 a = glm::mat3(glm::inverse(root)) * ((nv - vel) / dt); vel = nv; prev = pos;
    const float al = glm::length(a); if (al > 30.f) a *= 30.f / al;
    const float spin = (rot - rotPrev) / dt; rotPrev = rot;
    for (int j = 0; j < kN; j++) { const float an = (float(j) + 0.5f) / float(kN) * 2.f * kRigPi - kRigPi; const glm::vec3 tan(-std::sin(an), 0, std::cos(an));
        v[j] += -k * dt * o[j] - 5.f * dt * v[j] - 0.9f * dt * a - spin * 0.6f * dt * tan;
        v[j].x += std::sin(t * 1.3f + float(j)) * 0.012f * dt * k; v[j].z += std::cos(t * 1.1f + float(j) * 1.7f) * 0.012f * dt * k;   // brisa
        o[j] += v[j] * dt; const float L = glm::length(o[j]); if (L > kMax) o[j] *= kMax / L; }
}
void PoseSmoother::tick(const Fighter& f) {
    constexpr float kHand = 0.2f, kPunch = 0.55f, kBody = 0.25f;   // fração do caminho por quadro (60 Hz)
    if (!ready) { view = f; pos = rigPos(f); ready = true; return; }
    pos += (rigPos(f) - pos) * 0.15f;
    // a sombrinha gira em volta do cabo só durante o soco, na mão que soca (updateHand do vinil)
    if (isPunchState(f.state) && f.maxTimer > 0) { const int i = f.isLeftPunch ? 1 : 0; spin[i] += (i ? 1.f : -1.f) * 0.42f * std::sin(kRigPi * (1.f - float(f.timer) / float(f.maxTimer))); }
    open += ((f.state == FIGHTER_BLOCK ? 1.f : 0.f) - open) * 0.28f; if (open < 0.002f) open = 0; if (open > 0.998f) open = 1;
    const PartRenderData gl = view.gloveL, gr = view.gloveR; const float ox = view.offsetX, oy = view.offsetY, oz = view.offsetZ, sk = view.bodySkew;
    view = f;
    const auto slide = [](float from, float to, float k) { return from + (to - from) * k; };
    const auto hand = [&](PartRenderData& out, const PartRenderData& was, bool punching) { const float k = punching ? kPunch : kHand; out.x = slide(was.x, out.x, k); out.y = slide(was.y, out.y, k); out.z = slide(was.z, out.z, k); };
    const bool punch = isPunchState(f.state);
    hand(view.gloveL, gl, punch && f.isLeftPunch); hand(view.gloveR, gr, punch && !f.isLeftPunch);
    view.offsetX = slide(ox, f.offsetX, kBody); view.offsetY = slide(oy, f.offsetY, kBody); view.offsetZ = slide(oz, f.offsetZ, kBody); view.bodySkew = slide(sk, f.bodySkew, kBody);
}
FighterPose computePose(const Fighter& f, const RigRest& r, float down, const PoseCtx& ctx) {
    FighterPose p;
    p.root = glm::translate(glm::mat4(1.f), ctx.hasPos ? ctx.pos : rigPos(f));
    p.root = glm::rotate(p.root, -f.facing, glm::vec3(0, 1, 0));
    // nocauteada: inclina para trás a partir dos pés e desce um pouco, escorada nas cordas
    if (down > 0) p.root = glm::rotate(p.root, -0.34f * down, glm::vec3(1, 0, 0));
    // nas cordas o balanço é daqui, igual para os dois (o motor só balança o oponente; a jogadora ficava parada)
    const float T = ctx.hangT, sway = std::sin(T * 0.055f), breath = std::sin(T * 0.16f), t60 = T / 60.f, up = 1 - down;
    // de pé: o pulinho de luta do vinil, o corpo sobe e desce no ritmo, e o quadril ginga
    const float bob = std::fabs(std::sin(t60 * 3.2f)) * 0.0245f * up;
    p.root = glm::translate(p.root, glm::vec3(0, 0, 0.32f * ctx.ropeBounce * down));
    const glm::vec3 off = glm::vec3(-f.offsetX * kRigUnit, f.offsetY * kRigUnit, f.offsetZ * kRigUnit) * (1 - down) + glm::vec3(sway * 0.04f, -0.1f + breath * 0.014f, 0) * down;
    p.upperPos = glm::vec3(0, r.WAIST + bob, 0) + off;
    // o quadril acompanha 80% do balanço do tronco, mas a subida do salto inteira: senão o corpo se divide no ar
    p.hipsPos = glm::vec3(0, r.HIPS + bob * 0.9f + std::max(0.f, off.y) * 0.2f, 0) + off * 0.8f;
    // no soco a luva vai mais longe na mesma medida que os dois foram afastados (kSpread de cada lado)
    const auto hand = [&](const PartRenderData& g, const PartRenderData& base) { const float out = std::clamp((g.z - 12.0f) * kRigUnit / 0.3f, 0.f, 1.f) * 2.f * kSpread / r.scale; return glm::vec3(-g.x * r.handX / 32.f, r.handY + (g.y - base.y) * kRigUnit, r.gloveZ0 + (g.z - 12.0f) * kRigUnit + out) + off; };
    p.glove[1] = hand(f.gloveL, f.baseGloveL) + glm::vec3(0, bob * 0.5f + std::sin(t60 * 3.2f) * 0.02f * up, 0); p.glove[0] = hand(f.gloveR, f.baseGloveR) + glm::vec3(0, bob * 0.5f + std::cos(t60 * 3.2f) * 0.02f * up - 0.02f * up * std::cos(0.f), 0);
    // os braços vão para trás e ficam jogados por cima da corda de cima
    // a luva fica em cima da corda de cima da arena: a altura dela no mundo já com o corpo inclinado para trás e a escala do lutador
    { const float a = -0.34f * down, z = -0.32f, lift = kGloveDrape, y = ((ctx.ropeTop + lift) / std::max(0.3f, r.scale) + z * std::sin(a)) / std::max(0.5f, std::cos(a));
      for (int i = 0; i < 2; i++) { const float s = i ? 1.f : -1.f; p.glove[i] += (glm::vec3(s * (0.72f + 0.03f * std::sin(T * 0.08f)), y + 0.025f * s * std::sin(T * 0.11f), z) - p.glove[i]) * down; } }
    p.gloveEuler[1] = gloveTurn(f, true, 1.f); p.gloveEuler[0] = gloveTurn(f, false, -1.f);
    // a luva para na superfície do alvo e amassa em vez de atravessar
    const float reachLocal = ctx.reachLimit / r.scale;
    for (int i = 0; i < 2; i++) if (p.glove[i].z >= reachLocal) { p.glove[i].z = reachLocal; p.gloveScale[i] = {1.25f, 1.25f, 0.7f}; }
    // orientação: giro de mão por cima de como a luva fica na guarda ou no soco (onFrame de cada lutador no vinil)
    for (int i = 0; i < 2; i++) { const float s = i ? 1.f : -1.f, k = std::clamp(((i ? f.gloveL.z : f.gloveR.z) - 12.f) * kRigUnit / 0.3f, 0.f, 1.f); const glm::vec3 e = p.gloveEuler[i], UP(0, 1, 0);
        if (r.glove == GloveMode::Umbrella) { p.gloveRot[i] = glm::quat(e); continue; }
        // colmeia (jack.js): na guarda a boca para a frente; no golpe a boca aponta para baixo e o alto do cesto vai à frente
        if (r.glove == GloveMode::Hive) { const glm::vec3 dir = glm::normalize(glm::mix(glm::vec3(s * 0.25f, 0.05f, 1), glm::vec3(0, -1, 0.08f), k)), ref = glm::normalize(glm::mix(UP, glm::vec3(0, 0, 1), k)), yA = glm::normalize(ref - dir * glm::dot(ref, dir)); p.gloveRot[i] = glm::quat_cast(glm::mat3(glm::cross(yA, dir), yA, dir)); continue; }
        glm::quat base(1, 0, 0, 0);
        if (r.glove == GloveMode::Fist) { const glm::vec3 dir = glm::normalize(glm::mix(glm::vec3(0, 0.8f, 0.6f), glm::vec3(0, -0.1f, 1), k)); glm::vec3 zr = glm::mix(glm::vec3(0, 0, -1), glm::vec3(0, -1, 0), k); zr = glm::normalize(zr - dir * glm::dot(zr, dir)); base = glm::quat_cast(glm::mat3(glm::cross(dir, zr), dir, zr)); }
        else if (r.glove == GloveMode::Saw) base = glm::rotation(UP, glm::normalize(glm::vec3(s * 0.1f, 0.15f, 1)));
        else base = glm::rotation(UP, glm::normalize(glm::vec3(s * 0.1f, 1, 0.25f + 0.6f * k)));
        p.gloveRot[i] = glm::quat(glm::vec3(e.x, r.glove == GloveMode::Fist ? e.y + kGloveYaw * s : e.y, 0)) * base * glm::angleAxis(e.z, UP); }
    // o tronco acompanha o soco: gira levando à frente o lado da luva que avança e inclina junto
    const float dz = f.gloveL.z - f.gloveR.z, reach = std::max(f.gloveL.z, f.gloveR.z) - 12.0f;
    p.twist = std::clamp(-dz * 0.0035f, -0.45f, 0.45f);
    p.lean = std::clamp(reach * 0.0016f, -0.2f, 0.26f);
    p.roll = f.bodySkew * (1 - down) + sway * 0.07f * down + std::sin(t60 * 3.2f) * 0.025f * up;
    // quem apanha: a deformação vai para o corpo no golpe na barriga e para a cabeça nos outros; recua e a cabeça acompanha a mola
    const float q = ctx.squash, body = f.hitKind == 3 ? 1.f : 0.25f, head = f.hitKind == 3 ? 0.3f : 1.f;
    p.headScale = {1 + 0.32f * q * head, 1 - 0.34f * q * head, 1 + 0.12f * q * head};
    p.bodyScale = {1 + 0.26f * q * body, 1 - 0.3f * q * body, 1 + 0.26f * q * body};
    p.root = glm::translate(p.root, glm::vec3(0, 0, -0.26f * q));
    p.neckTurn = f.hitDir * (f.hitKind == 2 ? 0.55f : 0.2f) * q;
    p.neckTilt = (f.hitKind == 4 ? -0.6f : f.hitKind == 3 ? 0.35f : -0.2f) * q;
    p.lean += (f.hitKind == 3 ? 0.3f : -0.18f) * q;
    // cabeça caída para a frente e para o lado
    p.lean -= 0.3f * down; p.neckTilt += (0.72f + 0.14f * std::sin(T * 0.07f) + 0.5f * ctx.ropeBounce) * down; p.neckTurn += 0.32f * sway * down;
    // tentando levantar: continua escorada, mas dá trancos para cima, ergue a cabeça e se inclina para a frente (em vez de ficar meio de pé e dura)
    if (ctx.rise > 0.f && down > 0.f) { const float pulse = std::pow(std::max(0.f, std::sin(T * 0.45f)), 3.f), k = ctx.rise * down;
        p.root = glm::translate(p.root, glm::vec3(0, (0.03f + 0.07f * pulse) * k, -0.05f * pulse * k)); p.neckTilt -= (0.35f + 0.25f * pulse) * k; p.lean += 0.18f * k * pulse; }
    for (int i = 0; i < 2; i++) {
        // guarda de boxe: pés abertos, um à frente e outro atrás (o de trás aparece sob a saia para a câmera); nas cordas escorregam para a frente
        const glm::vec3 stance = (i ? glm::vec3(kStanceX, 0, -kStanceBack) : glm::vec3(-kStanceX, 0, kStanceFront)) * (r.stance * (1 - down));
        // no salto (o motor sobe o corpo pelo offsetY) os pés sobem junto e um pouco mais: as pernas encolhem como num pulo
        const float lift = std::max(0.f, off.y - 0.05f), feetUp = lift + std::min(lift, 0.3f) * 0.3f;
        p.footPos[i] = r.foot[i] + stance + glm::vec3(off.x * 0.8f, feetUp, off.z * 0.8f) + glm::vec3(0, 0, 0.32f * down);
        // a perna sai do quadril como ele está, girado (legPlacer do vinil): senão ela fura a saia
        const glm::vec3 a = p.hipsPos + glm::quat(glm::vec3(p.lean * 0.45f, p.twist * 0.5f, p.roll * 0.55f)) * (glm::vec3(r.hip[i].x * r.hipIn, r.hip[i].y, r.hip[i].z) - glm::vec3(0, r.HIPS, 0));
        const glm::vec3 b(p.footPos[i].x, r.hip[i].y - r.legLen + feetUp, p.footPos[i].z);
        p.legPos[i] = a; p.legDirLen[i] = b - a;
    }
    shiftFeet(p, r);
    return p;
}
}
