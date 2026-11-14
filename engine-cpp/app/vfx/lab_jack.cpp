// Jack — Mãos de Colmeia. Port de combat_lab_effects.js (linhas 99-2160): abelhas da Infestação,
// mel do Modo Melaço, escudo de favos, Agulhada Rainha, cortina de pólen, Geleia Real e as passivas
// (Apicultor, Pele de Mel, Bênção da Rainha).
#include "lab_common.hpp"
#include "psx_bee.hpp"

namespace KoRama::vfx {

namespace {

struct Bee {
    float x, y, z, vx, vy, vz, orbitRadius, orbitSpeed, orbitAngle, size, life, decay, wingPhase, wingSpeed, angle;
};
struct HoneyDrip { float x, y, z, vy, length, width, life; };
struct HoneyPuddle { float x, z, r, maxR, life, decay, alpha; };
struct HoneySplash { float x, y, z, vx, vy, vz, size, life, decay; };
struct OrbitBee {
    int gloveIdx;
    float startX, startY, startZ, burstVx, burstVy, burstVz;
    float orbitRadiusX, orbitRadiusZ, orbitHeight, orbitSpeed, orbitAngle, size, wingPhase, wingSpeed;
    float spawnAge, maxSpawnAge;
};
struct HexCell { int ring; float baseAngle, heightOffset, radius, hexRadius, pulseOffset; };
struct PollenPuff { float x, y, z, vx, vy, vz, radius, maxRadius, growthSpeed, alpha, life, decay, rot, rotSpeed, hue; };
struct PollenMote { float x, y, z, vx, vy, vz, size, twinkleSpeed, twinklePhase, life, decay; };
struct GooDrip { float x, y, z, radius, length, sparklePhase, sparkleSpeed; };
struct Drift { float x, y, z, vx, vy, vz, radius, life, decay; };
struct Spark { float x, y, z, vx, vy, vz, radius; const char* color; float life, decay; };

// Abelha pronta para desenhar (com posição 3D, velocidade e rolagem).
struct BeeDraw {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    float size = 15.0f, life = 1.0f, wingPhase = 0.0f;
    float angle = 0.0f;
    float vx = 0.0f, vy = 0.0f, vz = 1.0f;
    float roll = 0.0f;
};

constexpr int MOLASSES_MAX_TIMER = 480;
constexpr int SHIELD_MAX_TIMER = 420;
constexpr int ROYAL_JELLY_DURATION = 420;
constexpr int HONEY_SKIN_MAX_TIMER = 450;
constexpr int QUEEN_BLESSING_MAX_TIMER = 180;

class JackFx final : public LabFx {
public:
    JackFx() { initHoneycombCells(); }

    void update(LabContext& c) override;
    void drawBackground(Canvas& ctx, LabContext& c) override {
        drawHoneyEffects(ctx, c);
        updateAndRenderHoneycombShield(ctx, c, false);
        draw3DQueenStinger(ctx, c, false);
        updateAndRenderPollenSmoke(ctx, c, false);
        drawRoyalJellyCoating(ctx, c, false);
        updateAndRenderRoyalBees(ctx, c, false);
        updateAndRenderBeekeeperEffects(ctx, c, false);
        drawQueenBlessingEffect(ctx, c, false);
        updateAndRenderOrbitBees(ctx, c, false);
    }
    void drawForeground(Canvas& ctx, LabContext& c) override {
        updateAndRenderOrbitBees(ctx, c, true);
        updateAndRenderHoneycombShield(ctx, c, true);
        draw3DQueenStinger(ctx, c, true);
        updateAndRenderPollenSmoke(ctx, c, true);
        drawRoyalJellyCoating(ctx, c, true);
        updateAndRenderRoyalBees(ctx, c, true);
        updateAndRenderBeekeeperEffects(ctx, c, true);
        drawQueenBlessingEffect(ctx, c, true);
        updateAndRenderBees(ctx, c);
    }
    void drawBodyOverlay(Canvas& ctx, LabContext& c, float bW, float bH) override;

    // Brilho da Pele de Mel e da Geleia Real sobre o corpo (o renderer desenha o sprite em overlay).
    float honeySkinAlpha = 0.0f;
    float royalJellyAbsorption = 0.0f;

private:
    void resetMolassesOrbitBees(LabContext& c, bool fromClash);
    void initHoneycombCells();
    void spawnPollenSmokeBurst(LabContext& c);
    void spawnRoyalJellyBees(LabContext& c);
    void spawnBeekeeperBonusBees(float gx, float gy, float gz, bool isLeft);
    void spawnBeekeeperDodgePollen(float bodyX, float bodyY, bool isLeft);
    void spawnInfestationBees(LabContext& c);

    void draw3DBee(Canvas& ctx, LabContext& c, const BeeDraw& bee);
    void updateAndRenderBees(Canvas& ctx, LabContext& c);
    void drawHoneyEffects(Canvas& ctx, LabContext& c);
    void updateAndRenderOrbitBees(Canvas& ctx, LabContext& c, bool isFrontPass);
    void draw3DHexagon(Canvas& ctx, LabContext& c, float x, float y, float z, float hexRadius, float alpha, float rotAngle, float screenDx=0, float screenDy=0);
    void updateAndRenderHoneycombShield(Canvas& ctx, LabContext& c, bool isFrontPass);
    void drawArrowGeometry(Canvas& ctx, float length, float shaftW, float barbW, float barbMidW, float scale, float reachProgress);
    void draw3DQueenStinger(Canvas& ctx, LabContext& c, bool isFrontPass);
    void drawCartoonCloud(Canvas& ctx, float r, float alpha);
    void updateAndRenderPollenSmoke(Canvas& ctx, LabContext& c, bool isFrontPass);
    void draw3DRoyalBee(Canvas& ctx, LabContext& c, const BeeDraw& b);
    void updateAndRenderRoyalBees(Canvas& ctx, LabContext& c, bool isFrontPass);
    void drawRoyalJellyCoating(Canvas& ctx, LabContext& c, bool isFrontPass);
    void updateAndRenderBeekeeperEffects(Canvas& ctx, LabContext& c, bool isFrontPass);
    void drawQueenBlessingEffect(Canvas& ctx, LabContext& c, bool isFrontPass);

    std::vector<Bee> activeBees;
    std::vector<HoneyDrip> activeHoneyDrips;
    std::vector<HoneyPuddle> activeHoneyPuddles;
    std::vector<HoneySplash> honeySplashes;
    bool isMolassesModeActive = false;
    int molassesTimer = 0;
    bool lastMolassesSlamProgress = false;
    std::vector<OrbitBee> molassesOrbitBees;

    bool isShieldActive = false;
    int shieldTimer = 0;
    float shieldAngle = 0;
    float shieldSpawnAge = 0;
    std::vector<HexCell> honeycombCells;

    std::vector<PollenPuff> activePollenPuffs;
    std::vector<PollenMote> activePollenMotes;
    bool lastPollenBurstProgress = false;

    bool isRoyalJellyActive = false;
    int royalJellyTimer = 0;
    std::vector<OrbitBee> royalJellyBees;
    std::vector<GooDrip> royalJellyGooDrips;
    bool lastRoyalJellyTrigger = false;

    std::vector<Bee> beekeeperBees;
    std::vector<Drift> beekeeperPollenDrifts;
    std::vector<Spark> beekeeperImpactSparks;
    bool lastHitFrameTriggered = false;

    int honeySkinTimer = 0;
    bool isQueenBlessingActive = false;
    int queenBlessingTimer = 0;
    int lastRivalKnockdowns = 0;
};

void JackFx::resetMolassesOrbitBees(LabContext& c, bool fromClash) {
    molassesOrbitBees.clear();
    if (!fromClash) return;
    auto& f = c.fighter;
    // Seis abelhas saem de dentro das colmeias (3 da esquerda, 3 da direita).
    for (int i = 0; i < 6; ++i) {
        const bool isLeft = i % 2 == 0;
        const Glove& gl = isLeft ? f.parts.gloveL : f.parts.gloveR;
        const float sideDir = isLeft ? -1.0f : 1.0f;
        molassesOrbitBees.push_back({isLeft ? 0 : 1, gl.x + f.body.offsetX + (rnd() - 0.5f) * 4, gl.y + f.body.offsetY + (rnd() - 0.5f) * 4,
                                     gl.z + 15, sideDir * (2.4f + rnd() * 2.8f), 1.8f + rnd() * 2.6f, (rnd() - 0.5f) * 3.5f,
                                     52.0f + (i % 3) * 10, 38.0f + (i % 2) * 12, 46.0f + i * 9,
                                     0.048f + (i * 0.007f) * (i % 2 == 0 ? 1 : -1), (i / 6.0f) * PI * 2, 6.5f + (i % 3) * 0.8f,
                                     rnd() * PI * 2, 1.1f + rnd() * 0.4f, 0, 38});
    }
}

void JackFx::initHoneycombCells() {
    honeycombCells.clear();
    const int countPerRing = 8;
    const float radius = 64;
    for (int i = 0; i < countPerRing; ++i) honeycombCells.push_back({0, (i / static_cast<float>(countPerRing)) * PI * 2, 74, radius, 17, i * 0.4f});
    for (int i = 0; i < countPerRing; ++i) honeycombCells.push_back({1, ((i + 0.5f) / countPerRing) * PI * 2, 46, radius, 17, (i + 0.5f) * 0.4f});
}

void JackFx::spawnPollenSmokeBurst(LabContext& c) {
    auto& f = c.fighter;
    for (int i = 0; i < 36; ++i) {
        const float ang = rnd() * PI * 2, spread = 0.5f + rnd() * 2.8f, spdZ = 3.5f + rnd() * 7.0f;
        activePollenPuffs.push_back({f.body.offsetX + (rnd() - 0.5f) * 26, f.body.offsetY + 70 + (rnd() - 0.5f) * 28, 18 + (rnd() - 0.5f) * 10,
                                     std::cos(ang) * spread, (rnd() - 0.35f) * 2.8f, spdZ, 18 + rnd() * 22, 55 + rnd() * 45,
                                     0.8f + rnd() * 1.2f, 0.65f + rnd() * 0.25f, 1.0f, 0.012f + rnd() * 0.008f, rnd() * PI * 2,
                                     (rnd() - 0.5f) * 0.03f, (rnd() > 0.4f ? 45.0f : 38.0f) + rnd() * 12});
    }
    for (int i = 0; i < 48; ++i) {
        const float ang = rnd() * PI * 2, spd = 2.0f + rnd() * 6.5f;
        activePollenMotes.push_back({f.body.offsetX + (rnd() - 0.5f) * 30, f.body.offsetY + 70 + (rnd() - 0.5f) * 32, 18,
                                     std::cos(ang) * spd * 0.8f, std::sin(ang) * spd * 0.8f + 0.4f, 4.5f + rnd() * 8.0f,
                                     3.5f + rnd() * 4.5f, 0.2f + rnd() * 0.3f, rnd() * PI * 2, 1.0f, 0.011f + rnd() * 0.012f});
    }
}

void JackFx::spawnRoyalJellyBees(LabContext& c) {
    auto& f = c.fighter;
    royalJellyBees.clear();
    isRoyalJellyActive = true;
    royalJellyTimer = ROYAL_JELLY_DURATION;
    royalJellyAbsorption = 1.0f;
    for (int i = 0; i < 6; ++i) {
        const bool isLeft = i % 2 == 0;
        const Glove& gl = isLeft ? f.parts.gloveL : f.parts.gloveR;
        const float sideDir = isLeft ? -1.0f : 1.0f;
        royalJellyBees.push_back({isLeft ? 0 : 1, gl.x + f.body.offsetX, gl.y + f.body.offsetY, gl.z + 14, sideDir * (2.4f + rnd() * 2.2f),
                                  1.8f + rnd() * 2.0f, (rnd() - 0.5f) * 3.5f, 58.0f + (i % 3) * 10, 44.0f + (i % 2) * 12, 38.0f + i * 9,
                                  0.048f * (isLeft ? 1 : -1), (i / 6.0f) * PI * 2, 8.0f + (i % 2) * 1.5f, rnd() * PI * 2, 1.25f, 0, 25});
    }
    royalJellyGooDrips.clear();
    for (int d = 0; d < 24; ++d) {
        royalJellyGooDrips.push_back({(rnd() - 0.5f) * 42, 30 + rnd() * 65, 14 + (rnd() - 0.5f) * 12, 4.0f + rnd() * 4.5f, 8 + rnd() * 14,
                                      rnd() * PI * 2, 0.15f + rnd() * 0.15f});
    }
}

void JackFx::spawnBeekeeperBonusBees(float gx, float gy, float gz, bool isLeft) {
    for (int i = 0; i < 3; ++i) {
        const float spreadAngle = (i - 1) * 0.38f;
        beekeeperBees.push_back({gx + (i - 1) * 10.0f, gy + (rnd() - 0.5f) * 8, gz + 10, std::sin(spreadAngle) * 3.2f + (isLeft ? -1.8f : 1.8f),
                                 (rnd() - 0.5f) * 2.5f, 8.8f + rnd() * 4.5f, 0, 0, 0, 6.2f + rnd() * 1.5f, 1.0f, 0.022f + rnd() * 0.008f,
                                 rnd() * PI * 2, 1.6f, 0});
    }
    for (int s = 0; s < 12; ++s) {
        const float sparkAngle = rnd() * PI * 2, sparkSpd = 3.5f + rnd() * 4.5f;
        beekeeperImpactSparks.push_back({gx, gy, gz + 12, std::cos(sparkAngle) * sparkSpd, std::sin(sparkAngle) * sparkSpd,
                                         (rnd() - 0.5f) * 3.0f, 3.5f + rnd() * 3.0f, rnd() > 0.4f ? "#fde047" : "#ea580c", 1.0f,
                                         0.05f + rnd() * 0.03f});
    }
}

void JackFx::spawnBeekeeperDodgePollen(float bodyX, float bodyY, bool isLeft) {
    for (int i = 0; i < 16; ++i) {
        const float ang = rnd() * PI * 2, spd = 2.0f + rnd() * 4.0f;
        beekeeperPollenDrifts.push_back({bodyX + (rnd() - 0.5f) * 28, bodyY + 30 + rnd() * 50, 10 + (rnd() - 0.5f) * 20,
                                         (isLeft ? 1 : -1) * (2.2f + rnd() * 3.0f) + std::cos(ang) * spd * 0.5f, std::sin(ang) * spd * 0.6f,
                                         (rnd() - 0.5f) * 3.0f, 4.5f + rnd() * 5.5f, 1.0f, 0.026f + rnd() * 0.015f});
    }
    for (int b = 0; b < 3; ++b) {
        beekeeperBees.push_back({bodyX + (isLeft ? -22.0f : 22.0f) + (rnd() - 0.5f) * 14, bodyY + 45 + (rnd() - 0.5f) * 16, 14 + (rnd() - 0.5f) * 10,
                                 (isLeft ? 1.8f : -1.8f) * (2.2f + rnd() * 2.5f), (rnd() - 0.5f) * 3.2f, 4.8f + rnd() * 4.2f, 0, 0, 0,
                                 6.0f + rnd() * 1.2f, 1.0f, 0.020f + rnd() * 0.008f, rnd() * PI * 2, 1.5f, 0});
    }
}

void JackFx::spawnInfestationBees(LabContext& c) {
    auto& f = c.fighter;
    const Glove* gloves[2] = {&f.parts.gloveL, &f.parts.gloveR};
    for (int idx = 0; idx < 2; ++idx) {
        const Glove& g = *gloves[idx];
        const float sx = g.x + f.body.offsetX + (rnd() - 0.5f) * 12, sy = g.y + f.body.offsetY + (rnd() - 0.5f) * 12, sz = g.z + (rnd() - 0.5f) * 10;
        const float angle = rnd() * PI * 2;
        const float dirX = (idx == 0 ? -1.0f : 1.0f) * (2.0f + rnd() * 3.0f);
        activeBees.push_back({sx, sy, sz, dirX + std::cos(angle) * 2.2f, (rnd() - 0.45f) * 3.0f, 4.5f + rnd() * 5.0f, 15 + rnd() * 30,
                              (rnd() > 0.5f ? 1.0f : -1.0f) * (0.12f + rnd() * 0.08f), rnd() * PI * 2, 6.5f + rnd() * 2.0f, 1.0f,
                              0.016f + rnd() * 0.016f, rnd() * PI * 2, 0.8f + rnd() * 0.4f, (rnd() - 0.5f) * 0.6f});
    }
}

// Controlador (CombatLabEffects.update + os gatilhos do updateAnimationProgress do Lab).
void JackFx::update(LabContext& c) {
    auto& f = c.fighter;
    const float p = c.progress();
    const Fighter* e = c.entity;

    if (f.state == "jack_infestation" && p >= 0.22f && p <= 0.78f) spawnInfestationBees(c);

    if (f.state == "jack_comb_burst") {
        if (p >= 0.36f && !lastPollenBurstProgress) {
            lastPollenBurstProgress = true;
            spawnPollenSmokeBurst(c);
        }
        if (p < 0.30f) lastPollenBurstProgress = false;
    }
    if (f.state == "jack_royal_jelly") {
        if (p >= 0.30f && !lastRoyalJellyTrigger) {
            lastRoyalJellyTrigger = true;
            spawnRoyalJellyBees(c);
        }
        if (p < 0.25f) lastRoyalJellyTrigger = false;
    }
    if (f.state == "jack_honey_wall" && p >= 0.38f && (!isShieldActive || shieldTimer < SHIELD_MAX_TIMER - 30)) {
        isShieldActive = true;
        shieldTimer = SHIELD_MAX_TIMER;
        shieldSpawnAge = 0;
    }
    if (f.state == "jack_molasses") {
        if (p >= 0.35f && !lastMolassesSlamProgress) {
            lastMolassesSlamProgress = true;
            isMolassesModeActive = true;
            molassesTimer = MOLASSES_MAX_TIMER;
            resetMolassesOrbitBees(c, true);
            for (int s = 0; s < 24; ++s) {
                const float ang = rnd() * PI * 2, spd = 3.2f + rnd() * 5.5f;
                honeySplashes.push_back({(rnd() - 0.5f) * 8, 74 + (rnd() - 0.5f) * 8, 32 + (rnd() - 0.5f) * 8, std::cos(ang) * spd,
                                         std::sin(ang) * spd + 1.2f, (rnd() - 0.5f) * 4.0f, 4.5f + rnd() * 4.5f, 1.0f, 0.035f + rnd() * 0.02f});
            }
        }
        if (p < 0.30f) lastMolassesSlamProgress = false;
        if (p >= 0.35f && rnd() < 0.8f) {
            const bool isL = rnd() > 0.5f;
            const Glove& gl = isL ? f.parts.gloveL : f.parts.gloveR;
            activeHoneyDrips.push_back({gl.x + f.body.offsetX + (rnd() - 0.5f) * 14, gl.y + f.body.offsetY - 8, gl.z + (rnd() - 0.5f) * 8,
                                        1.2f + rnd() * 1.5f, 6 + rnd() * 8, 2.5f + rnd() * 2.0f, 1.0f});
        }
    } else if (isMolassesModeActive) {
        if (rnd() < 0.35f) {
            const bool isL = rnd() > 0.5f;
            const Glove& gl = isL ? f.parts.gloveL : f.parts.gloveR;
            activeHoneyDrips.push_back({gl.x + f.body.offsetX + (rnd() - 0.5f) * 14, gl.y + f.body.offsetY - 8, gl.z + (rnd() - 0.5f) * 8,
                                        1.2f + rnd() * 1.5f, 5 + rnd() * 7, 2.2f + rnd() * 1.8f, 1.0f});
        }
        if (rnd() < 0.30f) {
            activeHoneyDrips.push_back({f.body.offsetX + (rnd() - 0.5f) * 28, f.body.offsetY + 70 + rnd() * 20, (rnd() - 0.5f) * 12,
                                        1.0f + rnd() * 1.4f, 4 + rnd() * 6, 2.0f + rnd() * 1.6f, 1.0f});
        }
    }

    // Passiva Apicultor: abelhas bônus no golpe e redemoinho de pólen na esquiva.
    const bool beekeeper = e && (e->loadout.has(SkillId::JackBeekeeper));
    const bool punching = f.state == "jab" || f.state == "cross" || f.state == "hook" || f.state == "upper" || f.state == "body_blow";
    const bool dodging = f.state == "dodgeL" || f.state == "dodgeR" || f.state == "dodgeBack";
    if (beekeeper && punching) {
        if (p >= 0.45f && p <= 0.65f && !lastHitFrameTriggered) {
            lastHitFrameTriggered = true;
            const Glove& gl = f.isLeft ? f.parts.gloveL : f.parts.gloveR;
            spawnBeekeeperBonusBees(gl.x + f.body.offsetX, gl.y + f.body.offsetY, gl.z, f.isLeft);
        }
        if (p < 0.25f) lastHitFrameTriggered = false;
    } else if (beekeeper && dodging) {
        if (p >= 0.35f && p <= 0.55f && !lastHitFrameTriggered) {
            lastHitFrameTriggered = true;
            spawnBeekeeperDodgePollen(f.body.offsetX, f.body.offsetY, f.state == "dodgeL");
        }
        if (p < 0.25f) lastHitFrameTriggered = false;
    } else {
        lastHitFrameTriggered = false;
    }

    // Pele de Mel: brilho âmbar ao bloquear (e fixo com a passiva equipada).
    const bool honeySkin = e && e->loadout.has(SkillId::JackHoneySkin);
    if (f.state == "block") honeySkinTimer = std::max(honeySkinTimer, honeySkin ? HONEY_SKIN_MAX_TIMER : 20);
    if (honeySkinTimer > 0) honeySkinTimer--;
    honeySkinAlpha = honeySkinTimer > 120 ? 1.0f : honeySkinTimer / 120.0f;

    // Bênção da Rainha: coroa celestial quando o rival cai.
    if (e && c.rival) {
        if (e->loadout.has(SkillId::JackQueenBlessing) && c.rival->state == FIGHTER_KNOCKDOWN && lastRivalKnockdowns == 0) {
            isQueenBlessingActive = true;
            queenBlessingTimer = QUEEN_BLESSING_MAX_TIMER;
        }
        lastRivalKnockdowns = c.rival->state == FIGHTER_KNOCKDOWN ? 1 : 0;
    }
    if (queenBlessingTimer > 0 && --queenBlessingTimer == 0) isQueenBlessingActive = false;

    // Tempos ligados ao motor: o melaço dura o status, o escudo os golpes restantes, a geleia se absorve.
    if (e) {
        if (isMolassesModeActive && f.state != "jack_molasses") {
            molassesTimer = std::min(molassesTimer, std::max(e->status.molasses, 0));
            if (molassesTimer <= 0) {
                isMolassesModeActive = false;
                resetMolassesOrbitBees(c, false);
            }
        }
        if (isShieldActive && f.state != "jack_honey_wall" && e->status.honeyShield <= 0 && --shieldTimer <= 0) isShieldActive = false;
    }
    if (isRoyalJellyActive) {
        royalJellyTimer--;
        royalJellyAbsorption = royalJellyTimer < 150 ? std::max(0.0f, royalJellyTimer / 150.0f) : 1.0f;
        if (royalJellyTimer <= 0) {
            isRoyalJellyActive = false;
            royalJellyBees.clear();
        }
    }
}

void JackFx::draw3DBee(Canvas& ctx, LabContext& c, const BeeDraw& bee) {
    if (!c.camera || !c.entity) return;

    // Converte a posição local do lutador para coordenadas mundiais
    Vec3 wBee = Physics3D::localToWorld(bee.x, bee.y, bee.z, c.entity->worldPos, c.entity->facing);
    float beeDepth = c.camera->depthOf(wBee);

    // Oclusão 3D pelo rival ("sumindo quando o inimigo estiver na frente delas"):
    if (c.rival) {
        float rivalHeadY = (c.rival->charId == 9 || c.rival->isDummy()) ? 122.0f : (c.rival->isBigChar() ? 122.0f : (c.rival->isSmallChar() ? 85.0f : 108.0f));
        Vec3 wRivalCenter = Physics3D::localToWorld(c.rival->offsetX, bee.y, c.rival->offsetZ, c.rival->worldPos, c.rival->facing);
        float rivalDepth = c.camera->depthOf(wRivalCenter);

        // Se a abelha está atrás do oponente em relação à câmera:
        if (beeDepth > rivalDepth) {
            Point3 pRival = c.projectWorldPos(wRivalCenter.x, wRivalCenter.y, wRivalCenter.z);
            Point3 pBee = c.projectWorldPos(wBee.x, wBee.y, wBee.z);
            if (pRival.ok() && pBee.ok()) {
                const float rivalHalfW = 46.0f * pRival.scale;
                const float rivalTop = pRival.y - (rivalHeadY - bee.y + 40.0f) * pRival.scale;
                const float rivalBottom = pRival.y + (bee.y + 12.0f) * pRival.scale;
                if (std::abs(pBee.x - pRival.x) < rivalHalfW && pBee.y >= rivalTop && pBee.y <= rivalBottom) {
                    return; // Oponente está na frente da abelha: abelha é ocultada!
                }
            }
        }
    }

    float cosF = std::cos(c.entity->facing), sinF = std::sin(c.entity->facing);
    Vec3 wDir = { bee.vx * cosF + bee.vz * sinF, bee.vy, -bee.vx * sinF + bee.vz * cosF };
    if (std::abs(wDir.x) < 0.001f && std::abs(wDir.y) < 0.001f && std::abs(wDir.z) < 0.001f) {
        float a = bee.angle;
        wDir = { std::sin(a) * cosF + std::cos(a) * sinF, 0.0f, -std::sin(a) * sinF + std::cos(a) * cosF };
    }

    ctx.flush();
    drawPsxBee(ctx.target(), *c.camera, wBee, wDir, bee.size, bee.wingPhase, bee.roll, bee.life, false, c.entity->skinId);
}

void JackFx::updateAndRenderBees(Canvas& ctx, LabContext& c) {
    if (activeBees.empty()) return;
    std::sort(activeBees.begin(), activeBees.end(), [](const Bee& a, const Bee& b) { return b.z < a.z; });
    for (int i = static_cast<int>(activeBees.size()) - 1; i >= 0; --i) {
        Bee& b = activeBees[static_cast<std::size_t>(i)];
        if (!c.isPaused) {
            b.orbitAngle += b.orbitSpeed;
            b.x += b.vx + std::cos(b.orbitAngle) * 1.6f;
            b.y += b.vy + std::sin(b.orbitAngle) * 1.3f;
            b.z += b.vz;
            b.wingPhase += b.wingSpeed;
            b.life -= b.decay;
        }
        float curVx = b.vx - std::sin(b.orbitAngle) * 1.6f * b.orbitSpeed;
        float curVy = b.vy + std::cos(b.orbitAngle) * 1.3f * b.orbitSpeed;
        float curVz = b.vz;
        float roll = -std::sin(b.orbitAngle) * 0.45f;
        draw3DBee(ctx, c, {b.x, b.y, b.z, b.size, b.life, b.wingPhase, b.angle, curVx, curVy, curVz, roll});
        if (b.life <= 0 || b.z > 340) activeBees.erase(activeBees.begin() + i);
    }
}

void JackFx::drawHoneyEffects(Canvas& ctx, LabContext& c) {
    const bool isClosing = !isMolassesModeActive || molassesTimer <= 90;
    for (int i = static_cast<int>(activeHoneyPuddles.size()) - 1; i >= 0; --i) {
        HoneyPuddle& pud = activeHoneyPuddles[static_cast<std::size_t>(i)];
        if (!c.isPaused) {
            if (pud.r < pud.maxR && !isClosing) pud.r += 0.35f;
            if (isClosing) {
                pud.life -= 0.016f;
                pud.alpha = std::max(0.0f, pud.alpha - 0.012f);
            } else {
                pud.life -= pud.decay;
            }
        }
        Point3 p = c.projectWorldFloor(pud.x, pud.z);
        const float rw = pud.r * p.scale, rh = pud.r * 0.40f * p.scale;
        const float alpha = std::min(0.85f, pud.life * pud.alpha);
        if (alpha > 0.01f && p.ok()) {
            ctx.save();
            ctx.translate(p.x, p.y);
            ctx.globalAlpha = alpha;
            ctx.fillStyle = "#92400e";
            ctx.beginPath();
            ctx.ellipse(0, 0, rw + 1.2f, rh + 1.0f, 0, 0, PI * 2);
            ctx.fill();
            auto grad = ctx.createRadialGradient(0, -rh * 0.2f, rw * 0.1f, 0, 0, rw);
            grad->addColorStop(0, "#fef08a");
            grad->addColorStop(0.4f, "#f59e0b");
            grad->addColorStop(1, "#b45309");
            ctx.fillStyle = grad;
            ctx.beginPath();
            ctx.ellipse(0, 0, rw, rh, 0, 0, PI * 2);
            ctx.fill();
            ctx.fillStyle = "rgba(255, 255, 255, 0.75)";
            ctx.beginPath();
            ctx.ellipse(-rw * 0.28f, -rh * 0.28f, rw * 0.24f, rh * 0.20f, -0.2f, 0, PI * 2);
            ctx.fill();
            ctx.restore();
        }
        if (pud.life <= 0 || pud.alpha <= 0.01f) activeHoneyPuddles.erase(activeHoneyPuddles.begin() + i);
    }

    for (int i = static_cast<int>(activeHoneyDrips.size()) - 1; i >= 0; --i) {
        HoneyDrip& d = activeHoneyDrips[static_cast<std::size_t>(i)];
        if (!c.isPaused) {
            d.y -= d.vy;
            d.vy += 0.42f;
        }
        if (d.y <= 0) {
            Vec3 wPud = Physics3D::localToWorld(d.x, 0, d.z, c.entity->worldPos, c.entity->facing);
            activeHoneyPuddles.push_back({wPud.x, wPud.z, 4, 14 + rnd() * 8, 1.0f, 0.003f, 0.8f});
            activeHoneyDrips.erase(activeHoneyDrips.begin() + i);
            continue;
        }
        Point3 p = c.project3D(d.x, d.y, d.z);
        if (!p.ok()) continue;
        const float w = d.width * p.scale, h = d.length * p.scale;
        ctx.save();
        ctx.translate(p.x, p.y);
        ctx.fillStyle = "#f59e0b";
        ctx.strokeStyle = "#92400e";
        ctx.lineWidth = 0.8f * p.scale;
        ctx.beginPath();
        ctx.moveTo(0, -h);
        ctx.bezierCurveTo(-w * 0.8f, -h * 0.3f, -w, 0, 0, w * 0.8f);
        ctx.bezierCurveTo(w, 0, w * 0.8f, -h * 0.3f, 0, -h);
        ctx.closePath();
        ctx.fill();
        ctx.stroke();
        ctx.fillStyle = "rgba(255, 255, 255, 0.8)";
        ctx.beginPath();
        ctx.arc(w * 0.2f, -h * 0.3f, w * 0.25f, 0, PI * 2);
        ctx.fill();
        ctx.restore();
    }

    for (int i = static_cast<int>(honeySplashes.size()) - 1; i >= 0; --i) {
        HoneySplash& sp = honeySplashes[static_cast<std::size_t>(i)];
        if (!c.isPaused) {
            sp.x += sp.vx;
            sp.y += sp.vy;
            sp.z += sp.vz;
            sp.vy -= 0.32f;
            sp.life -= sp.decay;
        }
        Point3 p = c.project3D(sp.x, sp.y, sp.z);
        if (p.ok()) {
            const float sz = sp.size * p.scale;
            ctx.save();
            ctx.translate(p.x, p.y);
            ctx.globalAlpha = std::min(1.0f, sp.life * 1.5f);
            ctx.fillStyle = "#f59e0b";
            ctx.strokeStyle = "#b45309";
            ctx.lineWidth = 1 * p.scale;
            ctx.beginPath();
            ctx.arc(0, 0, sz, 0, PI * 2);
            ctx.fill();
            ctx.stroke();
            ctx.restore();
        }
        if (sp.life <= 0 || sp.y <= 0) {
            if (sp.y <= 0) {
                Vec3 wPud = Physics3D::localToWorld(sp.x, 0, sp.z, c.entity->worldPos, c.entity->facing);
                activeHoneyPuddles.push_back({wPud.x, wPud.z, 3, 10 + rnd() * 6, 1.0f, 0.004f, 0.75f});
            }
            honeySplashes.erase(honeySplashes.begin() + i);
        }
    }
}

void JackFx::updateAndRenderOrbitBees(Canvas& ctx, LabContext& c, bool isFrontPass) {
    if (molassesOrbitBees.empty()) return;
    auto& f = c.fighter;
    const float pMolasses = f.state == "jack_molasses" ? c.progress() : 0.0f;
    if (f.state == "jack_molasses" && pMolasses < 0.35f) return;
    const bool isReturning = isMolassesModeActive && molassesTimer <= 60;
    const float returnProgress = isReturning ? std::min(1.0f, (60 - molassesTimer) / 52.0f) : 0.0f;
    Point3 bPos = c.project3D(f.body.offsetX, f.body.offsetY, 0);

    for (OrbitBee& b : molassesOrbitBees) {
        if (!c.isPaused) {
            b.spawnAge = std::min(45.0f, b.spawnAge + 1);
            b.orbitAngle += b.orbitSpeed * (isReturning ? 2.2f : 1.0f);
            b.wingPhase += b.wingSpeed * (isReturning ? 1.8f : 1.0f);
        }
        const float spawnProgress = std::min(1.0f, b.spawnAge / b.maxSpawnAge);
        float bx, by, bz;
        const Glove& gl = b.gloveIdx == 0 ? f.parts.gloveL : f.parts.gloveR;
        if (!isReturning) {
            const float finalX = f.body.offsetX + std::cos(b.orbitAngle) * b.orbitRadiusX;
            const float finalY = f.body.offsetY + b.orbitHeight + std::sin(b.orbitAngle * 2.5f) * 8;
            const float finalZ = std::sin(b.orbitAngle) * b.orbitRadiusZ;
            if (spawnProgress < 1.0f) {
                const float easeArc = (1 - std::cos(spawnProgress * PI)) * 0.5f;
                const float popX = gl.x + f.body.offsetX + b.burstVx * (1 - easeArc) * 8;
                const float popY = gl.y + f.body.offsetY + b.burstVy * (1 - easeArc) * 8;
                const float popZ = gl.z + 10 + b.burstVz * (1 - easeArc) * 5;
                bx = popX * (1 - easeArc) + finalX * easeArc;
                by = popY * (1 - easeArc) + finalY * easeArc;
                bz = popZ * (1 - easeArc) + finalZ * easeArc;
            } else {
                bx = finalX; by = finalY; bz = finalZ;
            }
        } else {
            const float targetX = gl.x + f.body.offsetX, targetY = gl.y + f.body.offsetY, targetZ = gl.z + 10;
            const float orbitX = f.body.offsetX + std::cos(b.orbitAngle) * b.orbitRadiusX * (1 - returnProgress);
            const float orbitY = f.body.offsetY + b.orbitHeight * (1 - returnProgress) + targetY * returnProgress;
            const float orbitZ = std::sin(b.orbitAngle) * b.orbitRadiusZ * (1 - returnProgress);
            bx = orbitX * (1 - returnProgress) + targetX * returnProgress;
            by = orbitY * (1 - returnProgress) + targetY * returnProgress;
            bz = orbitZ * (1 - returnProgress) + targetZ * returnProgress;
        }
        Point3 ptBee = c.project3D(bx, by, bz);
        const bool isFront = ptBee.z <= bPos.z;
        if (isFront != isFrontPass) continue;
        float beeSize = b.size, beeAlpha = 1.0f;
        if (spawnProgress < 1.0f) {
            beeSize = b.size * (0.25f + spawnProgress * 0.75f);
            beeAlpha = std::min(1.0f, spawnProgress * 2.0f);
        } else if (isReturning) {
            beeSize = b.size * std::max(0.1f, 1.0f - returnProgress * 0.9f);
            beeAlpha = std::max(0.0f, 1.0f - returnProgress);
        }
        float curVx = -std::sin(b.orbitAngle) * b.orbitRadiusX * b.orbitSpeed;
        float curVz = std::cos(b.orbitAngle) * b.orbitRadiusZ * b.orbitSpeed;
        float curVy = std::cos(b.orbitAngle * 2.5f) * 8.0f * b.orbitSpeed * 2.5f;
        float roll = -0.40f;
        if (beeAlpha > 0.02f) draw3DBee(ctx, c, {bx, by, bz, beeSize, beeAlpha, b.wingPhase, std::cos(b.orbitAngle) * 0.35f, curVx, curVy, curVz, roll});
    }
}

void JackFx::draw3DHexagon(Canvas& ctx, LabContext& c, float x, float y, float z, float hexRadius, float alpha, float rotAngle, float screenDx, float screenDy) {
    Point3 pt = c.project3D(x, y, z);
    if (pt.scale <= 0) return;
    const float r = hexRadius * pt.scale;
    ctx.save();
    ctx.translate(pt.x+screenDx, pt.y+screenDy);
    ctx.rotate(rotAngle);
    ctx.beginPath();
    for (int i = 0; i < 6; ++i) {
        const float a = (i * PI / 3) - PI / 6;
        if (i == 0) ctx.moveTo(std::cos(a) * r, std::sin(a) * r);
        else ctx.lineTo(std::cos(a) * r, std::sin(a) * r);
    }
    ctx.closePath();
    const bool cyber = c.entity && c.entity->skinId == 3;
    ctx.fillStyle = cyber ? cv::rgba(32,188,166,alpha*.36f) : cv::rgba(255, 210, 30, alpha * 0.36f);
    ctx.fill();
    ctx.strokeStyle = cyber ? cv::rgba(116,255,224,alpha*.88f) : cv::rgba(255, 248, 140, alpha * 0.88f);
    ctx.lineWidth = std::max(1.0f, 2.2f * pt.scale);
    ctx.stroke();
    ctx.beginPath();
    for (int i = 0; i < 6; ++i) {
        const float a = (i * PI / 3) - PI / 6;
        if (i == 0) ctx.moveTo(std::cos(a) * r * 0.58f, std::sin(a) * r * 0.58f);
        else ctx.lineTo(std::cos(a) * r * 0.58f, std::sin(a) * r * 0.58f);
    }
    ctx.closePath();
    ctx.strokeStyle = cyber ? cv::rgba(219,255,243,alpha*.50f) : cv::rgba(255, 255, 220, alpha * 0.50f);
    ctx.lineWidth = std::max(0.7f, 1.2f * pt.scale);
    ctx.stroke();
    ctx.restore();
}

void JackFx::updateAndRenderHoneycombShield(Canvas& ctx, LabContext& c, bool isFrontPass) {
    auto& f = c.fighter;
    const bool engineShield = c.entity && c.entity->status.honeyShield > 0;
    const bool hasShield = engineShield || f.state == "jack_honey_wall" || isShieldActive;
    if (!hasShield) {
        isShieldActive = false;
        shieldSpawnAge = 0;
        return;
    }
    const float pShield = f.state == "jack_honey_wall" ? c.progress() : 0.0f;
    if (f.state == "jack_honey_wall" && pShield < 0.25f && !engineShield) return;
    if (f.state == "jack_honey_wall" && pShield < 0.15f) shieldSpawnAge = 0;
    if (!c.isPaused) {
        shieldAngle += 0.024f;
        shieldSpawnAge = std::min(45.0f, shieldSpawnAge + 1);
    }
    const float spawnProgress = std::min(1.0f, shieldSpawnAge / 24.0f);
    const float easeSpawn = (1 - std::cos(spawnProgress * PI)) * 0.5f;
    // O escudo some favo a favo conforme os golpes são absorvidos (3 golpes = cheio).
    float fadeAlpha = 1.0f;
    if (!engineShield && f.state != "jack_honey_wall") fadeAlpha = std::clamp(shieldTimer / 30.0f, 0.0f, 1.0f);
    const int hitsLeft = c.entity ? c.entity->status.honeyShield : 3;
    const float time = nowMs() / 1000.0f;
    Point3 bPos = c.project3D(f.body.offsetX, f.body.offsetY, 0);
    // Cada golpe absorvido apaga um pouco o domo (3 golpes = cheio).
    if (engineShield) fadeAlpha *= hitsLeft >= 3 ? 1.0f : (hitsLeft == 2 ? 0.8f : 0.6f);
    for (const HexCell& cell : honeycombCells) {
        const float curAngle = shieldAngle + cell.baseAngle;
        const float curRadius = cell.radius * (0.35f + 0.65f * easeSpawn);
        const float curX = f.body.offsetX + std::cos(curAngle) * curRadius;
        const float curZ = std::sin(curAngle) * curRadius;
        const float curY = f.body.offsetY + cell.heightOffset + std::sin(time * 3.5f + cell.pulseOffset) * 2.5f;
        Point3 ptHex = c.project3D(curX, curY, curZ);
        const bool isFront = ptHex.z <= bPos.z;
        if (isFront != isFrontPass) continue;
        float hexAlpha = (0.75f + 0.25f * std::sin(time * 4.2f + cell.pulseOffset)) * easeSpawn * fadeAlpha;
        const float hexR = cell.hexRadius * (0.4f + 0.6f * easeSpawn);
        // Preserve all sixteen cells; displace colliding cells, never delete them.
        float tx=bPos.x+(ptHex.x-bPos.x)*1.35f, ty=ptHex.y+12*ptHex.scale;
        for(int pass=0;pass<4;++pass)for (const Glove* glove : {&f.parts.gloveL,&f.parts.gloveR}) {
            const Point3 hand=c.project3D(glove->x+f.body.offsetX,glove->y+f.body.offsetY,glove->z);
            if(!hand.ok())continue;
            const float dx=tx-hand.x,dy=ty-hand.y,minDistance=hexR*ptHex.scale+23*hand.scale;
            if(std::hypot(dx,dy)<minDistance) {
                if(std::abs(tx-bPos.x)<14*ptHex.scale)ty=hand.y+std::sqrt(std::max(0.0f,minDistance*minDistance-dx*dx));
                else tx=hand.x+(tx<bPos.x?-1.0f:1.0f)*std::sqrt(std::max(0.0f,minDistance*minDistance-dy*dy));
            }
        }
        draw3DHexagon(ctx, c, curX, curY, curZ, hexR, hexAlpha, std::sin(curAngle) * 0.15f,tx-ptHex.x,ty-ptHex.y);
    }
}

void JackFx::drawArrowGeometry(Canvas& ctx, float length, float shaftW, float barbW, float barbMidW, float scale, float reachProgress) {
    for (int s = 1; s <= 3; ++s) {
        const float frac = (s / 3.0f) * reachProgress;
        const float ringDist = length * frac, ringRadius = (10 + (1 - frac) * 16) * scale;
        ctx.strokeStyle = cv::rgba(255, 235, 100, (1 - frac) * reachProgress * 0.6f);
        ctx.lineWidth = std::max(1.0f, 2.2f * scale);
        ctx.beginPath();
        ctx.ellipse(ringDist, 0, ringRadius * 0.35f, ringRadius, 0, 0, PI * 2);
        ctx.stroke();
    }
    ctx.beginPath();
    ctx.moveTo(0, -shaftW * 0.6f);
    ctx.lineTo(length * 0.35f, -shaftW);
    ctx.lineTo(length * 0.42f, -barbMidW);
    ctx.lineTo(length * 0.52f, -shaftW * 1.2f);
    ctx.lineTo(length * 0.65f, -barbW);
    ctx.lineTo(length, 0);
    ctx.lineTo(length * 0.65f, barbW);
    ctx.lineTo(length * 0.52f, shaftW * 1.2f);
    ctx.lineTo(length * 0.42f, barbMidW);
    ctx.lineTo(length * 0.35f, shaftW);
    ctx.lineTo(0, shaftW * 0.6f);
    ctx.closePath();
    auto grad = ctx.createLinearGradient(0, 0, length, 0);
    grad->addColorStop(0.0f, "#1c1106");
    grad->addColorStop(0.25f, "#d97706");
    grad->addColorStop(0.55f, "#291807");
    grad->addColorStop(0.80f, "#fbbf24");
    grad->addColorStop(1.0f, "#fffbeb");
    ctx.fillStyle = grad;
    ctx.shadowColor = "#fbbf24";
    ctx.shadowBlur = 14 * scale;
    ctx.fill();
    ctx.shadowBlur = 0;
    ctx.strokeStyle = "#fef08a";
    ctx.lineWidth = std::max(1.2f, 2.4f * scale);
    ctx.stroke();
    ctx.beginPath();
    ctx.moveTo(0, 0);
    ctx.lineTo(length - 4 * scale, 0);
    ctx.strokeStyle = "#ffffff";
    ctx.lineWidth = std::max(1.0f, 1.8f * scale);
    ctx.stroke();
    const float flareR = (12 + rnd() * 6) * scale * reachProgress;
    ctx.fillStyle = "rgba(255, 255, 255, 0.95)";
    ctx.beginPath();
    ctx.arc(length, 0, flareR * 0.45f, 0, PI * 2);
    ctx.fill();
    ctx.strokeStyle = "rgba(255, 240, 150, 0.85)";
    ctx.lineWidth = 1.8f * scale;
    ctx.beginPath();
    ctx.moveTo(length - flareR, 0); ctx.lineTo(length + flareR, 0);
    ctx.moveTo(length, -flareR); ctx.lineTo(length, flareR);
    ctx.stroke();
}

void JackFx::draw3DQueenStinger(Canvas& ctx, LabContext& c, bool isFrontPass) {
    auto& f = c.fighter;
    if (f.state != "jack_sting_jab") return;
    const float p = c.progress();
    if (p < 0.28f || p > 0.74f) return;
    float reachProgress = p <= 0.40f ? std::sin((p - 0.28f) / 0.12f * PI * 0.5f) : std::cos((p - 0.40f) / 0.34f * PI * 0.5f);
    if (reachProgress <= 0.02f) return;
    const float startX = f.body.offsetX, startY = f.body.offsetY + 74, startZ = f.parts.gloveL.z + 18;
    Point3 pBase = c.project3D(startX, startY, startZ);
    if (pBase.scale <= 0) return;
    const bool isStingerInFront = c.activeCamera != "back";
    if (isStingerInFront != isFrontPass) return;
    ctx.save();
    const float scale = pBase.scale;
    if (c.activeCamera == "front") {
        ctx.translate(pBase.x, pBase.y);
        for (int s = 1; s <= 3; ++s) {
            const float frac = (s / 3.0f) * reachProgress;
            const float rShock = (18 + frac * 48) * scale;
            ctx.strokeStyle = cv::rgba(255, 235, 100, (1 - frac) * reachProgress * 0.7f);
            ctx.lineWidth = std::max(1.2f, 2.8f * scale);
            ctx.beginPath();
            ctx.ellipse(0, 0, rShock, rShock * 0.7f, 0, 0, PI * 2);
            ctx.stroke();
        }
        const float fW = (28 + 24 * reachProgress) * scale, fH = (14 + 10 * reachProgress) * scale, coreR = (12 + 10 * reachProgress) * scale;
        ctx.beginPath();
        ctx.moveTo(0, -coreR * 0.8f);
        ctx.lineTo(-fW * 0.5f, -fH * 0.6f);
        ctx.lineTo(-fW, -fH);
        ctx.lineTo(-fW * 0.7f, 0);
        ctx.lineTo(-fW * 0.9f, fH * 0.8f);
        ctx.lineTo(-coreR * 0.6f, coreR * 0.5f);
        ctx.lineTo(0, coreR * 0.8f);
        ctx.lineTo(coreR * 0.6f, coreR * 0.5f);
        ctx.lineTo(fW * 0.9f, fH * 0.8f);
        ctx.lineTo(fW * 0.7f, 0);
        ctx.lineTo(fW, -fH);
        ctx.lineTo(fW * 0.5f, -fH * 0.6f);
        ctx.closePath();
        auto g = ctx.createRadialGradient(0, 0, 2, 0, 0, fW);
        g->addColorStop(0.0f, "#fffbeb");
        g->addColorStop(0.35f, "#fbbf24");
        g->addColorStop(0.70f, "#d97706");
        g->addColorStop(1.0f, "#1c1106");
        ctx.fillStyle = g;
        ctx.shadowColor = "#fbbf24";
        ctx.shadowBlur = 18 * scale;
        ctx.fill();
        ctx.shadowBlur = 0;
        ctx.strokeStyle = "#fef08a";
        ctx.lineWidth = std::max(1.5f, 2.6f * scale);
        ctx.stroke();
        const float flareR = (16 + rnd() * 8) * scale * reachProgress;
        ctx.fillStyle = "rgba(255, 255, 255, 0.98)";
        ctx.beginPath();
        ctx.arc(0, 0, flareR * 0.4f, 0, PI * 2);
        ctx.fill();
        ctx.strokeStyle = "rgba(255, 245, 160, 0.9)";
        ctx.lineWidth = 2.2f * scale;
        ctx.beginPath();
        ctx.moveTo(-flareR * 1.3f, 0); ctx.lineTo(flareR * 1.3f, 0);
        ctx.moveTo(0, -flareR * 0.9f); ctx.lineTo(0, flareR * 0.9f);
        ctx.stroke();
    } else {
        // De costas: a flecha some para o fundo, na direção do rival. Melhorada: a ponta avança em
        // perspectiva até o oponente, com o corpo da flecha visto em diagonal.
        const float endZ = startZ + 140 * reachProgress;
        Point3 pTip = c.project3D(startX, startY + 6, endZ);
        if (pTip.scale > 0) {
            const float dx = pTip.x - pBase.x, dy = pTip.y - pBase.y;
            const float length = std::max(10.0f * scale, std::hypot(dx, dy));
            ctx.translate(pBase.x, pBase.y);
            ctx.rotate(std::atan2(dy, dx));
            drawArrowGeometry(ctx, length, 6 * scale, (16 + 12 * reachProgress) * scale, (10 + 8 * reachProgress) * scale, scale * 0.8f, reachProgress);
        } else {
            ctx.translate(pBase.x, pBase.y);
        }
        ctx.restore();
        ctx.save();
        ctx.translate(pBase.x, pBase.y);
        for (int s = 1; s <= 3; ++s) {
            const float frac = (s / 3.0f) * reachProgress;
            const float rShock = (16 + (1 - frac) * 20) * scale * 0.8f;
            ctx.strokeStyle = cv::rgba(255, 235, 100, (1 - frac) * reachProgress * 0.5f);
            ctx.lineWidth = std::max(1.0f, 2 * scale);
            ctx.beginPath();
            ctx.ellipse(0, 0, rShock, rShock * 0.6f, 0, 0, PI * 2);
            ctx.stroke();
        }
    }
    ctx.restore();
}

void JackFx::drawCartoonCloud(Canvas& ctx, float r, float alpha) {
    if (r <= 0.1f) return;
    ctx.save();
    struct Lobe { float x, y, rad; };
    const Lobe lobes[] = {{0, 0, r * 0.95f}, {-r * 0.42f, -r * 0.15f, r * 0.65f}, {r * 0.42f, -r * 0.15f, r * 0.65f},
                          {-r * 0.25f, r * 0.35f, r * 0.60f}, {r * 0.25f, r * 0.35f, r * 0.60f}, {0, -r * 0.45f, r * 0.55f}};
    ctx.beginPath();
    for (const Lobe& l : lobes) {
        ctx.moveTo(l.x + l.rad, l.y);
        ctx.arc(l.x, l.y, l.rad, 0, PI * 2);
    }
    ctx.fillStyle = cv::rgba(248, 250, 252, alpha);
    ctx.fill();
    ctx.beginPath();
    ctx.arc(0, r * 0.25f, r * 0.65f, 0, PI);
    ctx.arc(-r * 0.25f, r * 0.35f, r * 0.45f, 0, PI);
    ctx.arc(r * 0.25f, r * 0.35f, r * 0.45f, 0, PI);
    ctx.fillStyle = cv::rgba(203, 213, 225, alpha * 0.95f);
    ctx.fill();
    ctx.strokeStyle = cv::rgba(15, 23, 42, alpha);
    ctx.lineWidth = std::max(1.5f, 2.6f * (r / 35));
    for (const Lobe& l : lobes) {
        ctx.beginPath();
        ctx.arc(l.x, l.y, l.rad, 0, PI * 2);
        ctx.stroke();
    }
    ctx.strokeStyle = cv::rgba(148, 163, 184, alpha * 0.85f);
    ctx.lineWidth = std::max(1.0f, 1.8f * (r / 35));
    ctx.beginPath();
    ctx.arc(-r * 0.1f, -r * 0.1f, r * 0.3f, 0.2f, PI * 1.2f);
    ctx.stroke();
    ctx.restore();
}

void JackFx::updateAndRenderPollenSmoke(Canvas& ctx, LabContext& c, bool isFrontPass) {
    if (activePollenPuffs.empty() && activePollenMotes.empty()) return;
    auto& f = c.fighter;
    Point3 bPos = c.project3D(f.body.offsetX, f.body.offsetY, 0);
    for (int i = static_cast<int>(activePollenPuffs.size()) - 1; i >= 0; --i) {
        PollenPuff& p = activePollenPuffs[static_cast<std::size_t>(i)];
        if (!c.isPaused) {
            p.x += p.vx; p.y += p.vy; p.z += p.vz;
            p.radius = std::min(p.maxRadius, p.radius + p.growthSpeed);
            p.rot += p.rotSpeed;
            p.life -= p.decay;
        }
        Point3 pt = c.project3D(p.x, p.y, p.z);
        if (pt.scale > 0 && p.life > 0) {
            const bool isFront = c.activeCamera == "front" ? true : pt.z <= bPos.z;
            if (isFront == isFrontPass) {
                ctx.save();
                ctx.translate(pt.x, pt.y);
                ctx.rotate(p.rot);
                const float popScale = p.life > 0.85f ? 1.0f - (p.life - 0.85f) / 0.15f : (p.life < 0.20f ? p.life / 0.20f : 1.0f);
                drawCartoonCloud(ctx, p.radius * pt.scale * std::max(0.2f, popScale), p.alpha * std::min(1.0f, p.life * 2.5f));
                ctx.restore();
            }
        }
        if (p.life <= 0 || p.z > 360) activePollenPuffs.erase(activePollenPuffs.begin() + i);
    }
    for (int i = static_cast<int>(activePollenMotes.size()) - 1; i >= 0; --i) {
        PollenMote& m = activePollenMotes[static_cast<std::size_t>(i)];
        if (!c.isPaused) {
            m.x += m.vx; m.y += m.vy; m.z += m.vz;
            m.twinklePhase += m.twinkleSpeed;
            m.life -= m.decay;
        }
        Point3 pt = c.project3D(m.x, m.y, m.z);
        if (pt.scale > 0 && m.life > 0) {
            const bool isFront = c.activeCamera == "front" ? true : pt.z <= bPos.z;
            if (isFront == isFrontPass) {
                const float mSize = m.size * pt.scale * std::min(1.0f, m.life * 2.0f);
                ctx.save();
                ctx.translate(pt.x, pt.y);
                ctx.fillStyle = "#ffffff";
                ctx.strokeStyle = "#00f5d4";
                ctx.lineWidth = 1.2f * pt.scale;
                ctx.beginPath();
                ctx.arc(0, 0, mSize, 0, PI * 2);
                ctx.fill();
                ctx.stroke();
                ctx.strokeStyle = "#00f5d4";
                ctx.lineWidth = 1.5f * pt.scale;
                ctx.beginPath();
                ctx.moveTo(-mSize * 1.6f, 0); ctx.lineTo(mSize * 1.6f, 0);
                ctx.moveTo(0, -mSize * 1.6f); ctx.lineTo(0, mSize * 1.6f);
                ctx.stroke();
                ctx.restore();
            }
        }
        if (m.life <= 0 || m.z > 360) activePollenMotes.erase(activePollenMotes.begin() + i);
    }
}

void JackFx::draw3DRoyalBee(Canvas& ctx, LabContext& c, const BeeDraw& b) {
    if (!c.camera || !c.entity) return;
    Vec3 wBee = Physics3D::localToWorld(b.x, b.y, b.z, c.entity->worldPos, c.entity->facing);
    float cosF = std::cos(c.entity->facing), sinF = std::sin(c.entity->facing);
    Vec3 wDir = { b.vx * cosF + b.vz * sinF, b.vy, -b.vx * sinF + b.vz * cosF };
    if (std::abs(wDir.x) < 0.001f && std::abs(wDir.y) < 0.001f && std::abs(wDir.z) < 0.001f) {
        float a = b.angle;
        wDir = { std::sin(a) * cosF + std::cos(a) * sinF, 0.0f, -std::sin(a) * sinF + std::cos(a) * cosF };
    }
    ctx.flush();
    drawPsxBee(ctx.target(), *c.camera, wBee, wDir, b.size, b.wingPhase, b.roll, b.life, true, c.entity->skinId);
}

void JackFx::updateAndRenderRoyalBees(Canvas& ctx, LabContext& c, bool isFrontPass) {
    if (!isRoyalJellyActive || royalJellyBees.empty()) return;
    auto& f = c.fighter;
    const bool isReturning = royalJellyTimer <= 200;
    const float returnProgress = isReturning ? std::min(1.0f, (200 - royalJellyTimer) / 50.0f) : 0.0f;
    if (returnProgress >= 1.0f) {
        royalJellyBees.clear();
        return;
    }
    Point3 bPos = c.project3D(f.body.offsetX, f.body.offsetY, 0);
    for (OrbitBee& b : royalJellyBees) {
        if (!c.isPaused) {
            b.spawnAge = std::min(25.0f, b.spawnAge + 1);
            b.orbitAngle += b.orbitSpeed * (isReturning ? 2.2f : 1.0f);
            b.wingPhase += b.wingSpeed * (isReturning ? 1.8f : 1.0f);
        }
        const float spawnProgress = std::min(1.0f, b.spawnAge / b.maxSpawnAge);
        const Glove& gl = b.gloveIdx == 0 ? f.parts.gloveL : f.parts.gloveR;
        float bx, by, bz;
        if (!isReturning) {
            const float finalX = f.body.offsetX + std::cos(b.orbitAngle) * b.orbitRadiusX;
            const float finalY = f.body.offsetY + b.orbitHeight + std::sin(b.orbitAngle * 2.0f) * 10;
            const float finalZ = std::sin(b.orbitAngle) * b.orbitRadiusZ;
            if (spawnProgress < 1.0f) {
                const float easeArc = (1 - std::cos(spawnProgress * PI)) * 0.5f;
                const float popX = gl.x + f.body.offsetX + b.burstVx * (1 - easeArc) * 6;
                const float popY = gl.y + f.body.offsetY + b.burstVy * (1 - easeArc) * 6;
                const float popZ = gl.z + 10 + b.burstVz * (1 - easeArc) * 4;
                bx = popX * (1 - easeArc) + finalX * easeArc;
                by = popY * (1 - easeArc) + finalY * easeArc;
                bz = popZ * (1 - easeArc) + finalZ * easeArc;
            } else {
                bx = finalX; by = finalY; bz = finalZ;
            }
        } else {
            const float targetX = gl.x + f.body.offsetX, targetY = gl.y + f.body.offsetY, targetZ = gl.z + 10;
            const float orbitX = f.body.offsetX + std::cos(b.orbitAngle) * b.orbitRadiusX * (1 - returnProgress);
            const float orbitY = f.body.offsetY + b.orbitHeight * (1 - returnProgress) + targetY * returnProgress;
            const float orbitZ = std::sin(b.orbitAngle) * b.orbitRadiusZ * (1 - returnProgress);
            bx = orbitX * (1 - returnProgress) + targetX * returnProgress;
            by = orbitY * (1 - returnProgress) + targetY * returnProgress;
            bz = orbitZ * (1 - returnProgress) + targetZ * returnProgress;
        }
        Point3 pt = c.project3D(bx, by, bz);
        if ((pt.z <= bPos.z) != isFrontPass) continue;
        float beeSize = b.size, beeAlpha = 1.0f;
        if (spawnProgress < 1.0f) {
            beeSize = b.size * (0.3f + spawnProgress * 0.7f);
            beeAlpha = std::min(1.0f, spawnProgress * 2.0f);
        } else if (isReturning) {
            beeSize = b.size * std::max(0.1f, 1.0f - returnProgress * 0.9f);
            beeAlpha = std::max(0.0f, 1.0f - returnProgress);
        }
        float curVx = -std::sin(b.orbitAngle) * b.orbitRadiusX * b.orbitSpeed;
        float curVz = std::cos(b.orbitAngle) * b.orbitRadiusZ * b.orbitSpeed;
        float curVy = std::cos(b.orbitAngle * 2.0f) * 10.0f * b.orbitSpeed * 2.0f;
        float roll = -0.40f;
        if (beeAlpha > 0.02f) draw3DRoyalBee(ctx, c, {bx, by, bz, beeSize, beeAlpha, b.wingPhase, std::cos(b.orbitAngle) * 0.35f, curVx, curVy, curVz, roll});
    }
}

void JackFx::drawRoyalJellyCoating(Canvas& ctx, LabContext& c, bool isFrontPass) {
    if (!isRoyalJellyActive || royalJellyAbsorption <= 0.01f) return;
    auto& f = c.fighter;
    Point3 bPos = c.project3D(f.body.offsetX, f.body.offsetY, 0);
    const float time = nowMs() / 1000.0f;
    int i = 0;
    for (const GooDrip& d : royalJellyGooDrips) {
        const float gx = f.body.offsetX + d.x, gy = f.body.offsetY + d.y + std::sin(time * 2.0f + i) * 1.5f, gz = d.z;
        Point3 pt = c.project3D(gx, gy, gz);
        if (pt.scale > 0 && (pt.z <= bPos.z) == isFrontPass) {
            const float r = d.radius * pt.scale, len = d.length * pt.scale;
            const float alpha = royalJellyAbsorption * (0.80f + 0.20f * std::sin(time * 3 + d.sparklePhase));
            ctx.save();
            ctx.translate(pt.x, pt.y);
            ctx.beginPath();
            ctx.moveTo(-r * 0.6f, 0);
            ctx.quadraticCurveTo(-r * 0.8f, len * 0.6f, 0, len);
            ctx.quadraticCurveTo(r * 0.8f, len * 0.6f, r * 0.6f, 0);
            ctx.closePath();
            auto g = ctx.createLinearGradient(0, 0, 0, len);
            g->addColorStop(0.0f, cv::rgba(255, 255, 240, alpha * 0.95f).color);
            g->addColorStop(0.5f, cv::rgba(253, 224, 71, alpha * 0.90f).color);
            g->addColorStop(1.0f, cv::rgba(217, 119, 6, alpha * 0.80f).color);
            ctx.fillStyle = g;
            ctx.fill();
            ctx.strokeStyle = cv::rgba(255, 255, 220, alpha * 0.95f);
            ctx.lineWidth = std::max(0.8f, 1.4f * pt.scale);
            ctx.stroke();
            ctx.fillStyle = cv::rgba(255, 255, 255, alpha * 0.95f);
            ctx.beginPath();
            ctx.arc(-r * 0.2f, len * 0.3f, std::max(1.0f, 1.8f * pt.scale), 0, PI * 2);
            ctx.fill();
            const float sparkleY = -std::fmod(time * 30 + i * 15, 40.0f) * pt.scale;
            const float sparkleAlpha = std::max(0.0f, 1.0f - (-sparkleY / (40 * pt.scale))) * royalJellyAbsorption;
            if (sparkleAlpha > 0.05f) {
                ctx.fillStyle = cv::rgba(110, 231, 183, sparkleAlpha * 0.8f);
                ctx.beginPath();
                ctx.arc(r * 0.3f, sparkleY, 2.2f * pt.scale, 0, PI * 2);
                ctx.fill();
            }
            ctx.restore();
        }
        ++i;
    }
}

void JackFx::updateAndRenderBeekeeperEffects(Canvas& ctx, LabContext& c, bool isFrontPass) {
    auto& f = c.fighter;
    Point3 bPos = c.project3D(f.body.offsetX, f.body.offsetY, 0);
    for (int i = static_cast<int>(beekeeperBees.size()) - 1; i >= 0; --i) {
        Bee& b = beekeeperBees[static_cast<std::size_t>(i)];
        if (!c.isPaused) {
            b.x += b.vx; b.y += b.vy; b.z += b.vz;
            b.wingPhase += b.wingSpeed;
            b.life -= b.decay;
        }
        Point3 pt = c.project3D(b.x, b.y, b.z);
        if (pt.scale > 0 && b.life > 0 && (pt.z <= bPos.z) == isFrontPass) {
            draw3DBee(ctx, c, {b.x, b.y, b.z, b.size, b.life, b.wingPhase, std::atan2(b.vy, b.vx) * 0.4f, b.vx, b.vy, b.vz, -0.25f});
        }
        if (b.life <= 0 || b.z > 360) beekeeperBees.erase(beekeeperBees.begin() + i);
    }
    for (int i = static_cast<int>(beekeeperPollenDrifts.size()) - 1; i >= 0; --i) {
        Drift& p = beekeeperPollenDrifts[static_cast<std::size_t>(i)];
        if (!c.isPaused) {
            p.x += p.vx; p.y += p.vy; p.z += p.vz;
            p.life -= p.decay;
        }
        Point3 pt = c.project3D(p.x, p.y, p.z);
        if (pt.scale > 0 && p.life > 0 && (pt.z <= bPos.z) == isFrontPass) {
            ctx.save();
            ctx.translate(pt.x, pt.y);
            ctx.fillStyle = cv::rgba(253, 224, 71, p.life * 0.85f);
            ctx.strokeStyle = cv::rgba(180, 83, 9, p.life * 0.95f);
            ctx.lineWidth = std::max(0.8f, 1.4f * pt.scale);
            ctx.beginPath();
            ctx.arc(0, 0, p.radius * pt.scale * p.life, 0, PI * 2);
            ctx.fill();
            ctx.stroke();
            ctx.restore();
        }
        if (p.life <= 0) beekeeperPollenDrifts.erase(beekeeperPollenDrifts.begin() + i);
    }
    for (int i = static_cast<int>(beekeeperImpactSparks.size()) - 1; i >= 0; --i) {
        Spark& s = beekeeperImpactSparks[static_cast<std::size_t>(i)];
        if (!c.isPaused) {
            s.x += s.vx; s.y += s.vy; s.z += s.vz;
            s.life -= s.decay;
        }
        Point3 pt = c.project3D(s.x, s.y, s.z);
        if (pt.scale > 0 && s.life > 0 && (pt.z <= bPos.z) == isFrontPass) {
            ctx.save();
            ctx.translate(pt.x, pt.y);
            ctx.fillStyle = s.color;
            ctx.strokeStyle = "#000000";
            ctx.lineWidth = std::max(0.6f, 1.0f * pt.scale);
            ctx.beginPath();
            ctx.arc(0, 0, s.radius * pt.scale * s.life, 0, PI * 2);
            ctx.fill();
            ctx.stroke();
            ctx.restore();
        }
        if (s.life <= 0) beekeeperImpactSparks.erase(beekeeperImpactSparks.begin() + i);
    }
}

void JackFx::drawQueenBlessingEffect(Canvas& ctx, LabContext& c, bool isFrontPass) {
    if (!isQueenBlessingActive && queenBlessingTimer <= 0) return;
    if (!isFrontPass) return;
    auto& f = c.fighter;
    Point3 bPos = c.project3D(f.body.offsetX, f.body.offsetY + 118, 12);
    if (bPos.scale <= 0) return;
    const float time = nowMs() / 1000.0f;
    float alpha = 1.0f;
    if (queenBlessingTimer > 130) alpha = std::max(0.0f, (QUEEN_BLESSING_MAX_TIMER - queenBlessingTimer) / 50.0f);
    else if (queenBlessingTimer <= 60) alpha = std::max(0.0f, queenBlessingTimer / 60.0f);
    if (alpha <= 0.01f) return;
    const float rise = (1.0f - alpha) * 14 * bPos.scale;
    ctx.save();
    ctx.translate(bPos.x, bPos.y - rise);
    ctx.globalAlpha = alpha;
    ctx.strokeStyle = cv::rgba(110, 231, 183, alpha * 0.45f);
    ctx.lineWidth = 1.8f * bPos.scale;
    for (int r = 0; r < 8; ++r) {
        const float rayAng = (r / 8.0f) * PI * 2 + time * 1.5f;
        ctx.beginPath();
        ctx.moveTo(0, 0);
        ctx.lineTo(std::cos(rayAng) * 32 * bPos.scale, std::sin(rayAng) * 32 * bPos.scale);
        ctx.stroke();
    }
    const float cW = 26 * bPos.scale, cH = 16 * bPos.scale;
    ctx.fillStyle = "#fde047";
    ctx.strokeStyle = "#ca8a04";
    ctx.lineWidth = 2.0f * bPos.scale;
    ctx.shadowColor = "#6ee7b7";
    ctx.shadowBlur = 12 * bPos.scale * alpha;
    ctx.beginPath();
    ctx.moveTo(-cW * 0.5f, 0);
    ctx.lineTo(-cW * 0.5f, -cH * 0.8f);
    ctx.lineTo(-cW * 0.25f, -cH * 0.35f);
    ctx.lineTo(0, -cH);
    ctx.lineTo(cW * 0.25f, -cH * 0.35f);
    ctx.lineTo(cW * 0.5f, -cH * 0.8f);
    ctx.lineTo(cW * 0.5f, 0);
    ctx.closePath();
    ctx.fill();
    ctx.shadowBlur = 0;
    ctx.stroke();
    ctx.fillStyle = "#6ee7b7";
    ctx.beginPath();
    ctx.arc(0, -cH * 0.5f, 2.5f * bPos.scale, 0, PI * 2);
    ctx.fill();
    ctx.beginPath();
    ctx.arc(-cW * 0.32f, -cH * 0.45f, 2.0f * bPos.scale, 0, PI * 2);
    ctx.fill();
    ctx.beginPath();
    ctx.arc(cW * 0.32f, -cH * 0.45f, 2.0f * bPos.scale, 0, PI * 2);
    ctx.fill();
    for (int p = 0; p < 4; ++p) {
        const float partY = -std::fmod(time * 25 + p * 12, 35.0f) * bPos.scale;
        const float partX = std::sin(time * 3 + p) * 12 * bPos.scale;
        ctx.fillStyle = cv::rgba(110, 231, 183, alpha * 0.85f);
        ctx.beginPath();
        ctx.arc(partX, partY, 2.0f * bPos.scale, 0, PI * 2);
        ctx.fill();
    }
    ctx.restore();
}

void JackFx::drawBodyOverlay(Canvas&, LabContext&, float, float) {
    // As camadas de Pele de Mel e Geleia Real são sprites do próprio Jack; o renderer as desenha
    // por cima do corpo com honeySkinAlpha / royalJellyAbsorption.
}

} // namespace

std::unique_ptr<LabFx> makeJackFx() { return std::make_unique<JackFx>(); }

// Leitura dos alfas de overlay do Jack (usados pelo renderer).
float jackHoneySkinAlpha(const LabFx& fx) { return static_cast<const JackFx&>(fx).honeySkinAlpha; }
float jackRoyalJellyAlpha(const LabFx& fx) { return static_cast<const JackFx&>(fx).royalJellyAbsorption; }

} // namespace KoRama::vfx
