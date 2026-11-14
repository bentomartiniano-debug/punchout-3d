// King Frog — O Rei da Lama. Port de combat_lab_effects.js (linhas 2163-4445): Golpe de Língua em
// grande arco, Cuspe de Lama com poças, Gêiser do Pântano, Captura de Moscas, Salto Real com coroa
// voadora e rachaduras, e as passivas (Saltador, Pele Mucosa, Fúria Anfíbia).
// Correções em relação ao JS: lama, poeira, detritos e faíscas usavam o eixo Y invertido e afundavam
// no chão; aqui sobem e caem. As gotas da Pele Venenosa (código morto no JS) agora escorrem no peito.
#include "lab_common.hpp"

namespace KoRama::vfx {

namespace {

// Visual anchors use the approved 120-unit drawing; simulation/damage stay
// untouched. All native world-space trajectories still run in this module.
float frogMouthY(const LabContext& c, float svgY = 51.0f) {
    return c.bodyLift + c.fighter.body.offsetY + (1.0f-svgY/120.0f)*c.bodyH;
}
std::string frogTone(const LabContext& c, const char* color) {
    const unsigned skin = c.entity ? c.entity->skinId : 0;
    const std::string s = color;
    if (skin == 1) {
        if (s=="#f43f5e") return "#d4ac42";
        if (s=="#be123c") return "#5d4b2b";
        if (s=="#e11d48") return "#b78c30";
        if (s=="#fda4af") return "#efd77b";
        if (s=="#fb7185") return "#e2bd55";
        if (s=="#881337") return "#66552a";
    } else if (skin == 3) {
        if (s=="#f43f5e") return "#37b9d3";
        if (s=="#be123c") return "#195876";
        if (s=="#e11d48") return "#268ba7";
        if (s=="#fda4af") return "#b3eece";
        if (s=="#fb7185") return "#e8d564";
        if (s=="#881337") return "#155c78";
    }
    return s;
}

struct TrailPt { float tipX, tipY, tipZ, mouthX, mouthY, mouthZ, life; };
struct Shockwave { float x, y, z, r, maxR, life, rot; };
struct ActionLine { float startX, startY, startZ, endX, endY, endZ, width, life; };
struct Particle { float x, y, z, vx, vy, vz, radius; const char* color; float life, decay; };
struct MudBomb { float x, y, z, targetX, targetZ, vx, vy, vz, gravity, rot, rotSpeed, r, life, maxFlight; };
struct MudSplash { float x, y, z, vx, vy, vz, gravity, r, life, decay; };
struct Bubble { float relX, relZ, r, maxR, grow; };
struct Droplet { float relX, relZ, r; };
struct MudPuddle { float x, z, curR, maxR; int life, maxLife; std::vector<Bubble> bubbles; std::vector<Droplet> droplets; };
struct GeyserBubble { float x, y, z, vy, r, life, decay; };
struct GeyserDrop { float x, y, z, vx, vy, r, life, decay; };
struct Ripple { float x, z, r, maxR, life, decay; };
struct Fly { int id; float angle, speed, rx, ry, rz, baseHeight; bool caught; float caughtP, flapPhase, x, y, z; };
struct Dust { float x, y, z, vx, vy, vz, radius, maxRadius, life, decay; };
struct CrackSeg { float x, z, width; };
struct Crack { float x, z, curR, maxR; int life, maxLife; std::vector<std::vector<CrackSeg>> branches; };
struct Debris { float x, y, z, vx, vy, vz, gravity, size, rot, rotSpeed, life, decay; };
struct Ring { float x, z, r, maxR, life, decay; };
struct Deflect { float x, y, z, radius, maxR, life, decay; };
struct Flame { float x, y, z, vx, vy, r, maxR; const char* color; float life, decay; };
struct PoisonDrip { float x, startY, curLen, maxLen, speed, phase, width, r; };

class FrogFx final : public LabFx {
public:
    void update(LabContext& c) override;
    void drawFloor(Canvas& ctx, LabContext& c) override {
        drawFrogGroundCracks(ctx, c);
        drawFrogMudPuddles(ctx, c);
        drawFrogDustPuffs(ctx, c);
        drawFrogDebris(ctx, c);
    }
    void drawBackground(Canvas& ctx, LabContext& c) override {
        draw3DFrogTongue(ctx, c, false);
        drawFrogJumperEffect(ctx, c, false);
        drawFrogSlipperySkinEffect(ctx, c, false);
        drawFrogAmphibianRageEffect(ctx, c, false);
    }
    void drawForeground(Canvas& ctx, LabContext& c) override {
        draw3DFrogTongue(ctx, c, true);
        drawFrogMudAirProjectiles(ctx, c);
        drawFrogFlyingCrown(ctx, c);
        drawFrogCrownSparks(ctx, c);
        drawFrogFlies(ctx, c);
        drawFrogFlyCatchTongue(ctx, c);
        drawFrogFlyCatchEffects(ctx, c);
        drawFrogSwampGeyser(ctx, c);
        drawFrogJumperEffect(ctx, c, true);
        drawFrogSlipperySkinEffect(ctx, c, true);
        drawFrogAmphibianRageEffect(ctx, c, true);
    }
    void drawBodyOverlay(Canvas& ctx, LabContext& c, float bW, float bH) override;
    std::string mouthStage(const LabContext& c) const override;

private:
    void updateFrogTongue(LabContext& c);
    void draw3DFrogTongue(Canvas& ctx, LabContext& c, bool isFrontPass);
    void spawnFrogMudBomb(LabContext& c);
    void spawnFrogMudPuddle(float landX, float landZ);
    void updateFrogMudSpit(LabContext& c);
    void drawFrogMudPuddles(Canvas& ctx, LabContext& c);
    void drawFrogMudAirProjectiles(Canvas& ctx, LabContext& c);
    void updateFrogSwampGeyser(LabContext& c);
    void drawFrogSwampGeyser(Canvas& ctx, LabContext& c);
    void updateFrogFlyCatch(LabContext& c);
    void drawFrogFlies(Canvas& ctx, LabContext& c);
    void drawFrogFlyCatchTongue(Canvas& ctx, LabContext& c);
    void drawFrogFlyCatchEffects(Canvas& ctx, LabContext& c);
    void triggerFrogLeapTakeoff(float x, float z);
    void triggerFrogLeapImpact(float x, float z);
    void updateFrogRoyalLeap(LabContext& c);
    void drawFrogGroundCracks(Canvas& ctx, LabContext& c);
    void drawFrogDustPuffs(Canvas& ctx, LabContext& c);
    void drawFrogDebris(Canvas& ctx, LabContext& c);
    void drawFrogFlyingCrown(Canvas& ctx, LabContext& c);
    void drawFrogCrownSparks(Canvas& ctx, LabContext& c);
    void triggerFrogJumperEffect(LabContext& c);
    void updateFrogJumper(LabContext& c);
    void drawFrogJumperEffect(Canvas& ctx, LabContext& c, bool isFrontPass);
    void triggerFrogSlipperySkinEffect(LabContext& c);
    void updateFrogSlipperySkin(LabContext& c);
    void drawFrogSlipperySkinEffect(Canvas& ctx, LabContext& c, bool isFrontPass);
    void updateFrogAmphibianRage(LabContext& c);
    void drawFrogAmphibianRageEffect(Canvas& ctx, LabContext& c, bool isFrontPass);
    void initFrogPoisonDrips(float bW, float bH);

    // Língua
    float frogTongueExt = 0, frogTongueArcP = 0;
    std::vector<MudSplash> frogTongueSaliva;
    std::vector<Particle> frogTongueImpactSparks;
    std::vector<TrailPt> frogTongueTrail;
    std::vector<Shockwave> frogTongueShockwaves;
    std::vector<ActionLine> frogTongueActionLines;
    bool lastFrogTongueHit = false;
    // Pele Venenosa
    float frogPoisonProgress = 0;
    std::vector<PoisonDrip> frogPoisonDrips;
    // Lama
    std::vector<MudBomb> frogMudBombs;
    std::vector<MudPuddle> frogMudPuddles;
    std::vector<MudSplash> frogMudSplashes;
    bool frogMudSpitFired = false;
    // Gêiser
    struct {
        bool active = false;
        float height = 0, width = 0, baseX = 0, baseZ = 0;
        bool erupted = false;
        std::vector<GeyserBubble> bubbles;
        std::vector<GeyserDrop> droplets;
        std::vector<Ripple> ripples;
    } frogGeyser;
    // Moscas
    std::vector<Fly> frogFlies;
    std::vector<Particle> frogFlyCatchSparks;
    bool frogFlySwarmSpawned = false, frogFlyHealTriggered = false;
    // Salto Real
    std::vector<Dust> frogDustPuffs;
    std::vector<Crack> frogGroundCracks;
    std::vector<Debris> frogDebrisFragments;
    std::vector<Particle> frogCrownSparks;
    bool frogLeapDustSpawned = false, frogLeapImpactSpawned = false;
    struct { bool active = false; float x = 0, y = 0, z = 0, rot = 0; } frogFlyingCrown;
    // Passivas
    int frogJumperTimer = 0;
    std::vector<Ring> frogJumperSpringRings;
    std::vector<Particle> frogJumperSparks;
    std::string lastState;
    int frogSlipperyTimer = 0;
    std::vector<Deflect> frogSlipperyDeflections;
    std::vector<Particle> frogSlipperyDrops;
    int lastSlipperyProcs = 0;
    bool isFrogRageActive = false;
    int frogRageTimer = 0;
    std::vector<Flame> frogRageFlames;
};

// ---------------------------------------------------------------------------
// Controlador
// ---------------------------------------------------------------------------

void FrogFx::update(LabContext& c) {
    updateFrogTongue(c);
    updateFrogMudSpit(c);
    updateFrogRoyalLeap(c);
    updateFrogFlyCatch(c);
    updateFrogSwampGeyser(c);

    // Saltador: molas e faíscas a cada esquiva. Pele Mucosa: deflexão aquosa quando absorve um golpe.
    const std::string& st = c.fighter.state;
    const bool dodge = st == "dodgeL" || st == "dodgeR" || st == "dodgeBack";
    const bool wasDodge = lastState == "dodgeL" || lastState == "dodgeR" || lastState == "dodgeBack";
    if (dodge && !wasDodge) triggerFrogJumperEffect(c);
    lastState = st;
    if (c.entity && c.entity->slipperyProcs != lastSlipperyProcs) {
        lastSlipperyProcs = c.entity->slipperyProcs;
        triggerFrogSlipperySkinEffect(c);
    }
    updateFrogJumper(c);
    updateFrogSlipperySkin(c);
    updateFrogAmphibianRage(c);

    // Veneno: a mancha cresce enquanto a pele está ativa e some devagar.
    const bool poison = c.entity && (c.entity->status.poisonSkin > 0 || st == "frog_poison_skin");
    if (poison) {
        if (frogPoisonDrips.empty()) initFrogPoisonDrips(115, 140);
        frogPoisonProgress = std::min(1.0f, frogPoisonProgress + 0.04f);
    } else {
        frogPoisonProgress = std::max(0.0f, frogPoisonProgress - 0.03f);
        if (frogPoisonProgress <= 0) frogPoisonDrips.clear();
    }
    for (PoisonDrip& d : frogPoisonDrips) {
        d.curLen += d.speed;
        if (d.curLen > d.maxLen) {
            d.curLen = 2.0f + rnd() * 3.0f; // a gota cai e outra começa a escorrer
            d.x = (rnd() - 0.5f) * 115 * 0.34f;
        }
    }
}

std::string FrogFx::mouthStage(const LabContext& c) const {
    const float p = c.progress();
    const std::string& st = c.fighter.state;
    if (st == "frog_tongue") {
        if (p < 0.08f) return "0";
        if (p < 0.16f) return "1";
        if (p < 0.24f) return "2";
        if (p < 0.32f) return "3";
        if (p <= 0.68f) return "4";
        if (p < 0.78f) return "5";
        if (p < 0.88f) return "6";
        return "0";
    }
    if (st == "frog_mud_slime") {
        if (p < 0.22f) return "inflated";
        if (p < 0.32f) return "2";
        if (p < 0.44f) return "3";
        if (p < 0.72f) return "mud_spit";
        if (p < 0.84f) return "5";
        if (p < 0.92f) return "6";
        return "0";
    }
    if (st == "frog_royal_leap") {
        if (p < 0.35f) return "0";
        if (p < 0.48f) return "no_crown";
        if (p < 0.68f) return "squash_no_crown";
        if (p < 0.84f) return "no_crown";
        return "0";
    }
    if (st == "frog_fly_catcher") {
        if (p < 0.14f) return "1";
        if (p < 0.26f) return "2";
        if (p < 0.38f) return "3";
        if (p < 0.68f) return "4";
        if (p < 0.82f) return "inflated";
        if (p < 0.92f) return "5";
        return "charge";
    }
    if (st == "frog_swamp_spout") {
        if (p < 0.25f) return "charge";
        if (p < 0.38f) return "3";
        if (p < 0.68f) return "4";
        if (p < 0.82f) return "5";
        return "0";
    }
    if (st == "jab" || st == "cross" || st == "hook" || st == "upper" || st == "body_blow") {
        return (p >= 0.18f && p <= 0.65f) ? "1" : "0";
    }
    if (st == "telegraph") return "charge";
    return "0";
}

// ---------------------------------------------------------------------------
// Golpe de Língua
// ---------------------------------------------------------------------------

void FrogFx::updateFrogTongue(LabContext& c) {
    auto& f = c.fighter;
    if (f.state == "frog_tongue") {
        const float p = c.progress();
        if (p >= 0.22f && p <= 0.74f) {
            const float tArc = (p - 0.22f) / 0.52f;
            frogTongueArcP = tArc;
            const float easeArc = std::sin((tArc - 0.5f) * PI) * 0.5f + 0.5f;
            if (tArc < 0.16f) {
                frogTongueExt = std::sin((tArc / 0.16f) * PI * 0.5f);
            } else if (tArc <= 0.84f) {
                frogTongueExt = 1.0f;
                if (tArc >= 0.48f && !lastFrogTongueHit) {
                    lastFrogTongueHit = true;
                    const float hitX = f.body.offsetX, hitY = f.body.offsetY + 48, hitZ = 280;
                    frogTongueShockwaves.push_back({hitX, hitY, hitZ, 12, 95, 1.0f, PI / 12});
                    for (int l = 0; l < 8; ++l) {
                        const float ang = (l / 8.0f) * PI * 2 + (rnd() - 0.5f) * 0.2f;
                        frogTongueActionLines.push_back({hitX + std::cos(ang) * 15, hitY + std::sin(ang) * 15, hitZ,
                                                         hitX + std::cos(ang) * (65 + rnd() * 30), hitY + std::sin(ang) * (65 + rnd() * 30),
                                                         hitZ + (rnd() - 0.5f) * 40, 3.5f + rnd() * 2.0f, 1.0f});
                    }
                    for (int s = 0; s < 28; ++s) {
                        const float ang = rnd() * PI * 2, spd = 5.0f + rnd() * 8.0f;
                        frogTongueImpactSparks.push_back({hitX + (rnd() - 0.5f) * 20, hitY + (rnd() - 0.5f) * 20, hitZ + (rnd() - 0.5f) * 30,
                                                          std::cos(ang) * spd + 3.0f, std::sin(ang) * spd, (rnd() - 0.5f) * 4, 4.0f + rnd() * 5.0f,
                                                          s % 3 == 0 ? "#ffffff" : (s % 3 == 1 ? "#f43f5e" : "#facc15"), 1.0f, 0.035f + rnd() * 0.025f});
                    }
                }
            } else {
                frogTongueExt = std::cos((tArc - 0.84f) / 0.16f * PI * 0.5f);
            }
            const float mouthX = f.body.offsetX, mouthY = frogMouthY(c), mouthZ = 4;
            const float tipX = mouthX + (-170 + easeArc * 340) * frogTongueExt;
            const float tipZ = 20 + std::sin(tArc * PI) * 280 * frogTongueExt;
            const float tipY = mouthY - std::sin(tArc * PI) * 22 + std::cos(tArc * PI * 2) * 10;
            if (frogTongueExt > 0.25f) frogTongueTrail.push_back({tipX, tipY, tipZ, mouthX, mouthY, mouthZ, 1.0f});
            if (rnd() < 0.75f) {
                frogTongueSaliva.push_back({tipX + (rnd() - 0.5f) * 16, tipY + (rnd() - 0.5f) * 14, tipZ + (rnd() - 0.5f) * 20,
                                            5.0f + rnd() * 6.0f, (rnd() - 0.45f) * 4.0f, (rnd() - 0.5f) * 3.5f, 0, 2.5f + rnd() * 3.5f, 1.0f,
                                            0.03f + rnd() * 0.02f});
            }
        } else {
            frogTongueExt = 0;
            lastFrogTongueHit = false;
        }
    } else {
        frogTongueExt = 0;
        lastFrogTongueHit = false;
    }
    for (int i = static_cast<int>(frogTongueTrail.size()) - 1; i >= 0; --i) {
        if ((frogTongueTrail[i].life -= 0.075f) <= 0) frogTongueTrail.erase(frogTongueTrail.begin() + i);
    }
    for (int i = static_cast<int>(frogTongueShockwaves.size()) - 1; i >= 0; --i) {
        Shockwave& sw = frogTongueShockwaves[i];
        sw.r += (sw.maxR - sw.r) * 0.18f;
        if ((sw.life -= 0.05f) <= 0) frogTongueShockwaves.erase(frogTongueShockwaves.begin() + i);
    }
    for (int i = static_cast<int>(frogTongueActionLines.size()) - 1; i >= 0; --i) {
        if ((frogTongueActionLines[i].life -= 0.08f) <= 0) frogTongueActionLines.erase(frogTongueActionLines.begin() + i);
    }
    for (int i = static_cast<int>(frogTongueSaliva.size()) - 1; i >= 0; --i) {
        MudSplash& sl = frogTongueSaliva[i];
        sl.x += sl.vx; sl.y += sl.vy; sl.z += sl.vz;
        if ((sl.life -= sl.decay) <= 0) frogTongueSaliva.erase(frogTongueSaliva.begin() + i);
    }
    for (int i = static_cast<int>(frogTongueImpactSparks.size()) - 1; i >= 0; --i) {
        Particle& sp = frogTongueImpactSparks[i];
        sp.x += sp.vx; sp.y += sp.vy; sp.z += sp.vz;
        if ((sp.life -= sp.decay) <= 0) frogTongueImpactSparks.erase(frogTongueImpactSparks.begin() + i);
    }
}

void FrogFx::draw3DFrogTongue(Canvas& ctx, LabContext& c, bool isFrontPass) {
    if (frogTongueExt <= 0.01f) return;
    if (c.activeCamera == "back" && isFrontPass) return;
    if (c.activeCamera != "back" && !isFrontPass) return;
    auto& f = c.fighter;
    Point3 bPos = c.project3D(f.body.offsetX, f.body.offsetY, 0);
    const float mouthX = f.body.offsetX, mouthY = frogMouthY(c), mouthZ = 4;
    const float tArc = frogTongueArcP;
    const float easeArc = std::sin((tArc - 0.5f) * PI) * 0.5f + 0.5f;
    const float tipX = mouthX + (-170 + easeArc * 340) * frogTongueExt;
    const float tipZ = 20 + std::sin(tArc * PI) * 280 * frogTongueExt;
    const float tipY = mouthY - std::sin(tArc * PI) * 22 + std::cos(tArc * PI * 2) * 10;

    // 1. Lâmina de vácuo em leque atrás da ponta.
    if (frogTongueTrail.size() > 2) {
        ctx.save();
        for (std::size_t i = 0; i + 1 < frogTongueTrail.size(); ++i) {
            const TrailPt& a = frogTongueTrail[i];
            const TrailPt& b = frogTongueTrail[i + 1];
            Point3 pTipA = c.project3D(a.tipX, a.tipY, a.tipZ), pTipB = c.project3D(b.tipX, b.tipY, b.tipZ);
            Point3 pBaseA = c.project3D(a.mouthX, a.mouthY, a.mouthZ), pBaseB = c.project3D(b.mouthX, b.mouthY, b.mouthZ);
            if (pTipA.scale <= 0 || pTipB.scale <= 0 || pBaseA.scale <= 0 || pBaseB.scale <= 0) continue;
            const float alpha = std::min(a.life, b.life);
            ctx.beginPath();
            ctx.moveTo(pBaseA.x, pBaseA.y);
            ctx.lineTo(pTipA.x, pTipA.y);
            ctx.lineTo(pTipB.x, pTipB.y);
            ctx.lineTo(pBaseB.x, pBaseB.y);
            ctx.closePath();
            auto g = ctx.createLinearGradient(pBaseA.x, pBaseA.y, (pTipA.x + pTipB.x) * 0.5f, (pTipA.y + pTipB.y) * 0.5f);
            g->addColorStop(0.0f, "rgba(225, 29, 72, 0.0)");
            g->addColorStop(0.4f, cv::rgba(225, 29, 72, alpha * 0.25f).color);
            g->addColorStop(0.8f, cv::rgba(244, 63, 94, alpha * 0.55f).color);
            g->addColorStop(1.0f, cv::rgba(255, 228, 230, alpha * 0.85f).color);
            ctx.fillStyle = g;
            ctx.fill();
            ctx.beginPath();
            ctx.moveTo(pTipA.x, pTipA.y);
            ctx.lineTo(pTipB.x, pTipB.y);
            ctx.strokeStyle = cv::rgba(255, 255, 255, alpha * 0.95f);
            ctx.lineWidth = std::max(2.0f, 6 * pTipB.scale);
            ctx.stroke();
            ctx.strokeStyle = cv::rgba(251, 113, 133, alpha * 0.75f);
            ctx.lineWidth = std::max(4.0f, 12 * pTipB.scale);
            ctx.stroke();
        }
        ctx.restore();
    }

    // 2. Nós da língua com onda de chicote.
    const int numNodes = 16;
    std::vector<Point3> nodes;
    for (int i = 0; i <= numNodes; ++i) {
        const float s = static_cast<float>(i) / numNodes;
        const float wave = std::sin((tArc - s * 0.38f) * PI * 3.0f) * 38 * std::sin(s * PI) * frogTongueExt;
        const float vert = std::sin(s * PI) * 16 * frogTongueExt;
        nodes.push_back(c.project3D(mouthX + (tipX - mouthX) * s + wave, mouthY + (tipY - mouthY) * s + vert, mouthZ + (tipZ - mouthZ) * s));
    }
    for (const Point3& n : nodes) if (n.scale <= 0) return;
    ctx.save();
    // 3. Sombra no chão.
    ctx.beginPath();
    for (int i = 0; i <= numNodes; ++i) {
        Point3 sh = c.project3D(mouthX + (tipX - mouthX) * (i / static_cast<float>(numNodes)), 0, mouthZ + (tipZ - mouthZ) * (i / static_cast<float>(numNodes)));
        if (i == 0) ctx.moveTo(sh.x, sh.y);
        else ctx.lineTo(sh.x, sh.y);
    }
    ctx.strokeStyle = "rgba(0, 0, 0, 0.26)";
    ctx.lineWidth = std::max(4.0f, 10 * bPos.scale * frogTongueExt);
    ctx.stroke();
    // 4. Corpo da língua.
    const Point3& pStart = nodes[0];
    const Point3& pEnd = nodes[numNodes];
    const float wBase = 18 * pStart.scale, wTip = 30 * pEnd.scale;
    ctx.beginPath();
    ctx.ellipse(pStart.x, pStart.y, wBase * 0.70f, wBase * 0.48f, 0, 0, PI * 2);
    ctx.fillStyle = frogTone(c, "#f43f5e");
    ctx.fill();
    std::vector<sf::Vector2f> left, right;
    for (int i = 0; i <= numNodes; ++i) {
        const float s = static_cast<float>(i) / numNodes;
        const float currW = (wBase + (wTip - wBase) * s) * 0.5f;
        const Point3& cur = nodes[i];
        float vx, vy;
        if (i < numNodes) { vx = nodes[i + 1].x - cur.x; vy = nodes[i + 1].y - cur.y; }
        else { vx = cur.x - nodes[i - 1].x; vy = cur.y - nodes[i - 1].y; }
        float len = std::hypot(vx, vy);
        if (len < 1e-4f) len = 1;
        const float dx = -vy / len, dy = vx / len;
        left.push_back({cur.x + dx * currW, cur.y + dy * currW});
        right.push_back({cur.x - dx * currW, cur.y - dy * currW});
    }
    ctx.beginPath();
    ctx.moveTo(left[0].x, left[0].y);
    for (std::size_t i = 1; i < left.size(); ++i) ctx.lineTo(left[i].x, left[i].y);
    for (int i = static_cast<int>(right.size()) - 1; i >= 0; --i) ctx.lineTo(right[i].x, right[i].y);
    ctx.closePath();
    auto gT = ctx.createLinearGradient(pStart.x, pStart.y, pEnd.x, pEnd.y);
    gT->addColorStop(0.0f, frogTone(c, "#f43f5e"));
    gT->addColorStop(0.20f, frogTone(c, "#be123c"));
    gT->addColorStop(0.65f, frogTone(c, "#e11d48"));
    gT->addColorStop(1.0f, frogTone(c, "#fda4af"));
    ctx.fillStyle = gT;
    ctx.fill();
    ctx.strokeStyle = "#0f1c10";
    ctx.lineWidth = std::max(1.4f, 2.8f * pEnd.scale);
    ctx.stroke();
    ctx.beginPath();
    ctx.moveTo(nodes[0].x, nodes[0].y);
    for (int i = 1; i <= numNodes; ++i) ctx.lineTo(nodes[i].x, nodes[i].y);
    ctx.strokeStyle = "rgba(254, 240, 138, 0.90)";
    ctx.lineWidth = std::max(1.2f, 3.8f * pEnd.scale);
    ctx.stroke();
    ctx.beginPath();
    ctx.arc(pEnd.x, pEnd.y, wTip * 0.58f, 0, PI * 2);
    ctx.fillStyle = frogTone(c, "#fb7185");
    ctx.fill();
    ctx.strokeStyle = "#0f1c10";
    ctx.lineWidth = std::max(1.4f, 2.8f * pEnd.scale);
    ctx.stroke();
    ctx.fillStyle = "#ffffff";
    ctx.beginPath();
    ctx.arc(pEnd.x - wTip * 0.18f, pEnd.y - wTip * 0.18f, std::max(1.5f, 4.5f * pEnd.scale), 0, PI * 2);
    ctx.fill();
    ctx.restore();

    for (const Shockwave& sw : frogTongueShockwaves) {
        Point3 p = c.project3D(sw.x, sw.y, sw.z);
        if (p.scale <= 0 || sw.life <= 0) continue;
        ctx.save();
        ctx.translate(p.x, p.y);
        ctx.rotate(sw.rot);
        ctx.beginPath();
        ctx.ellipse(0, 0, sw.r * p.scale * 1.5f, sw.r * p.scale * 0.65f, 0, 0, PI * 2);
        ctx.strokeStyle = cv::rgba(255, 255, 255, sw.life * 0.95f);
        ctx.lineWidth = std::max(2.0f, 6 * p.scale * sw.life);
        ctx.stroke();
        ctx.beginPath();
        ctx.ellipse(0, 0, (sw.r + 8) * p.scale * 1.5f, (sw.r + 8) * p.scale * 0.65f, 0, 0, PI * 2);
        ctx.strokeStyle = cv::rgba(244, 63, 94, sw.life * 0.70f);
        ctx.lineWidth = std::max(1.5f, 3.5f * p.scale * sw.life);
        ctx.stroke();
        ctx.restore();
    }
    for (const ActionLine& al : frogTongueActionLines) {
        Point3 a = c.project3D(al.startX, al.startY, al.startZ), b = c.project3D(al.endX, al.endY, al.endZ);
        if (a.scale <= 0 || b.scale <= 0 || al.life <= 0) continue;
        ctx.save();
        ctx.beginPath();
        ctx.moveTo(a.x, a.y);
        ctx.lineTo(b.x, b.y);
        ctx.strokeStyle = cv::rgba(255, 255, 255, al.life * 0.90f);
        ctx.lineWidth = std::max(1.2f, al.width * b.scale * al.life);
        ctx.lineCap = "round";
        ctx.stroke();
        ctx.restore();
    }
    for (const MudSplash& sl : frogTongueSaliva) {
        Point3 p = c.project3D(sl.x, sl.y, sl.z);
        if (p.scale <= 0 || sl.life <= 0) continue;
        ctx.fillStyle = cv::rgba(254, 240, 138, sl.life * 0.90f);
        ctx.strokeStyle = cv::rgba(15, 28, 16, sl.life * 0.90f);
        ctx.lineWidth = std::max(0.6f, 1.0f * p.scale);
        ctx.beginPath();
        ctx.arc(p.x, p.y, sl.r * p.scale, 0, PI * 2);
        ctx.fill();
        ctx.stroke();
    }
    for (const Particle& sp : frogTongueImpactSparks) {
        Point3 p = c.project3D(sp.x, sp.y, sp.z);
        if (p.scale <= 0 || sp.life <= 0) continue;
        ctx.fillStyle = sp.color;
        ctx.strokeStyle = "#0f1c10";
        ctx.lineWidth = std::max(0.8f, 1.4f * p.scale);
        ctx.beginPath();
        ctx.arc(p.x, p.y, sp.radius * p.scale * sp.life, 0, PI * 2);
        ctx.fill();
        ctx.stroke();
    }
}

// ---------------------------------------------------------------------------
// Pele Venenosa: gotas escorrendo no peito (novo — no JS a camada nunca aparecia)
// ---------------------------------------------------------------------------

void FrogFx::initFrogPoisonDrips(float bW, float bH) {
    (void)bW; (void)bH;
    frogPoisonDrips.clear();
}

void FrogFx::drawBodyOverlay(Canvas& ctx, LabContext& c, float bW, float bH) {
    (void)ctx; (void)c; (void)bW; (void)bH;
    // Gotas sobressalentes removidas a pedido do design para deixar a pele envenenada limpa e cartoon
}

// ---------------------------------------------------------------------------
// Cuspe de Lama
// ---------------------------------------------------------------------------

void FrogFx::spawnFrogMudBomb(LabContext& c) {
    auto& f = c.fighter;
    // A bomba sai da boca (altura 48 acima da base do corpo) e cai no pé do rival.
    Vec3 s = Physics3D::localToWorld(f.body.offsetX, frogMouthY(c, 48), 14, c.entity->worldPos, c.entity->facing);
    Vec3 t = c.rival ? c.rival->worldPos : s;
    const float flightFrames = 22, gravity = 0.52f;
    MudBomb b;
    b.x = s.x; b.y = s.y; b.z = s.z;
    b.targetX = t.x; b.targetZ = t.z;
    b.vx = (t.x - s.x) / flightFrames;
    b.vz = (t.z - s.z) / flightFrames;
    b.vy = (0 - s.y + 0.5f * gravity * flightFrames * flightFrames) / flightFrames;
    b.gravity = gravity;
    b.rot = 0;
    b.rotSpeed = 0.16f + rnd() * 0.10f;
    b.r = 15;
    b.life = 0;
    b.maxFlight = flightFrames;
    frogMudBombs.push_back(b);
}

void FrogFx::spawnFrogMudPuddle(float landX, float landZ) {
    MudPuddle p{landX, landZ, 12, 58 + rnd() * 16, 380, 380, {}, {}};
    for (int i = 0; i < 8; ++i) {
        const float ang = rnd() * PI * 2, dist = p.maxR * (0.65f + rnd() * 0.50f);
        p.droplets.push_back({std::cos(ang) * dist, std::sin(ang) * dist * 0.45f, 3 + rnd() * 4.5f});
    }
    frogMudPuddles.push_back(p);
    for (int i = 0; i < 22; ++i) {
        const float ang = rnd() * PI * 2, spd = 3.2f + rnd() * 6.0f;
        frogMudSplashes.push_back({landX, 0, landZ, std::cos(ang) * spd, 4.0f + rnd() * 5.0f, std::sin(ang) * spd * 0.7f, 0.48f,
                                   3.2f + rnd() * 4.2f, 1.0f, 0.035f + rnd() * 0.025f});
    }
}

void FrogFx::updateFrogMudSpit(LabContext& c) {
    if (c.fighter.state == "frog_mud_slime") {
        const float p = c.progress();
        if (p >= 0.44f && p < 0.65f && !frogMudSpitFired) {
            frogMudSpitFired = true;
            spawnFrogMudBomb(c);
        }
    } else {
        frogMudSpitFired = false;
    }
    for (int i = static_cast<int>(frogMudBombs.size()) - 1; i >= 0; --i) {
        MudBomb& b = frogMudBombs[i];
        b.x += b.vx; b.y += b.vy; b.z += b.vz;
        b.vy -= b.gravity;
        b.rot += b.rotSpeed;
        b.life += 1;
        if (static_cast<int>(b.life) % 2 == 0) {
            frogMudSplashes.push_back({b.x + (rnd() - 0.5f) * 6, b.y + (rnd() - 0.5f) * 6, b.z, (rnd() - 0.5f) * 1.5f, -1.0f + rnd() * 2.0f,
                                       b.vz * 0.2f, 0.3f, 2.2f + rnd() * 2.0f, 1.0f, 0.05f});
        }
        if (b.life >= b.maxFlight || b.y <= 0) {
            spawnFrogMudPuddle(b.life >= b.maxFlight ? b.targetX : b.x, b.life >= b.maxFlight ? b.targetZ : b.z);
            frogMudBombs.erase(frogMudBombs.begin() + i);
        }
    }
    for (int i = static_cast<int>(frogMudSplashes.size()) - 1; i >= 0; --i) {
        MudSplash& s = frogMudSplashes[i];
        s.x += s.vx; s.y += s.vy; s.z += s.vz;
        s.vy -= s.gravity;
        s.life -= s.decay;
        if (s.y <= 0) { s.y = 0; s.vx *= 0.5f; s.vz *= 0.5f; s.vy = 0; }
        if (s.life <= 0) frogMudSplashes.erase(frogMudSplashes.begin() + i);
    }
    for (int i = static_cast<int>(frogMudPuddles.size()) - 1; i >= 0; --i) {
        MudPuddle& p = frogMudPuddles[i];
        p.life -= 1;
        if (p.curR < p.maxR) p.curR += (p.maxR - p.curR) * 0.20f;
        if (p.life > 60 && rnd() < 0.25f) {
            const float ang = rnd() * PI * 2, dist = p.curR * rnd() * 0.75f;
            p.bubbles.push_back({std::cos(ang) * dist, std::sin(ang) * dist * 0.45f, 1.5f, 3.5f + rnd() * 4.0f, 0.25f + rnd() * 0.20f});
        }
        for (int b = static_cast<int>(p.bubbles.size()) - 1; b >= 0; --b) {
            p.bubbles[b].r += p.bubbles[b].grow;
            if (p.bubbles[b].r >= p.bubbles[b].maxR) p.bubbles.erase(p.bubbles.begin() + b);
        }
        if (p.life <= 0) frogMudPuddles.erase(frogMudPuddles.begin() + i);
    }
}

void FrogFx::drawFrogMudPuddles(Canvas& ctx, LabContext& c) {
    for (const MudPuddle& p : frogMudPuddles) {
        Point3 pc = c.projectWorldFloor(p.x, p.z);
        if (pc.scale <= 0) continue;
        const float pAlpha = p.life > 60 ? 1.0f : p.life / 60.0f;
        const float rx = p.curR * pc.scale, ry = rx * 0.36f;
        ctx.save();
        ctx.globalAlpha = pAlpha;
        ctx.beginPath();
        ctx.ellipse(pc.x, pc.y, rx, ry, 0, 0, PI * 2);
        ctx.fillStyle = "#221208";
        ctx.fill();
        ctx.strokeStyle = "#0d0603";
        ctx.lineWidth = std::max(1.0f, 2.0f * pc.scale);
        ctx.stroke();
        auto g = ctx.createRadialGradient(pc.x, pc.y, 2, pc.x, pc.y, rx * 0.88f);
        g->addColorStop(0.0f, "#5a341b");
        g->addColorStop(0.55f, "#3b2010");
        g->addColorStop(1.0f, "#221208");
        ctx.beginPath();
        ctx.ellipse(pc.x, pc.y, rx * 0.86f, ry * 0.86f * 0.92f, 0, 0, PI * 2);
        ctx.fillStyle = g;
        ctx.fill();
        for (const Droplet& d : p.droplets) {
            Point3 pd = c.projectWorldFloor(p.x + d.relX, p.z + d.relZ);
            const float drx = d.r * pd.scale;
            ctx.beginPath();
            ctx.ellipse(pd.x, pd.y, drx, drx * 0.38f, 0, 0, PI * 2);
            ctx.fillStyle = "#3b2010";
            ctx.fill();
            ctx.strokeStyle = "#0d0603";
            ctx.lineWidth = std::max(0.6f, 1.0f * pd.scale);
            ctx.stroke();
        }
        for (const Bubble& bub : p.bubbles) {
            Point3 pb = c.projectWorldFloor(p.x + bub.relX, p.z + bub.relZ);
            const float brx = bub.r * pb.scale, bry = brx * 0.65f;
            ctx.beginPath();
            ctx.ellipse(pb.x, pb.y - bry * 0.5f, brx, bry, 0, 0, PI * 2);
            ctx.fillStyle = "#6b3f22";
            ctx.fill();
            ctx.strokeStyle = "#0d0603";
            ctx.lineWidth = std::max(0.6f, 1.2f * pb.scale);
            ctx.stroke();
            ctx.fillStyle = "rgba(255, 255, 255, 0.65)";
            ctx.beginPath();
            ctx.arc(pb.x - brx * 0.3f, pb.y - bry * 0.7f, std::max(0.5f, brx * 0.28f), 0, PI * 2);
            ctx.fill();
        }
        ctx.strokeStyle = "rgba(255, 255, 255, 0.32)";
        ctx.lineWidth = std::max(0.8f, 1.6f * pc.scale);
        ctx.beginPath();
        ctx.ellipse(pc.x - rx * 0.22f, pc.y - ry * 0.15f, rx * 0.44f, ry * 0.32f, -0.15f, 0, PI * 0.85f);
        ctx.stroke();
        ctx.restore();
    }
}

void FrogFx::drawFrogMudAirProjectiles(Canvas& ctx, LabContext& c) {
    for (const MudBomb& b : frogMudBombs) {
        Point3 p = c.projectWorldPos(b.x, b.y, b.z);
        if (p.scale <= 0) continue;
        const float r = b.r * p.scale;
        ctx.save();
        ctx.translate(p.x, p.y);
        ctx.rotate(b.rot);
        ctx.beginPath();
        ctx.ellipse(0, 0, r * 1.15f, r * 0.88f, 0, 0, PI * 2);
        auto g = ctx.createRadialGradient(-r * 0.3f, -r * 0.3f, 1, 0, 0, r * 1.2f);
        g->addColorStop(0.0f, "#7c4826");
        g->addColorStop(0.4f, "#522e17");
        g->addColorStop(0.85f, "#331c0d");
        g->addColorStop(1.0f, "#1a0e06");
        ctx.fillStyle = g;
        ctx.fill();
        ctx.strokeStyle = "#0d0603";
        ctx.lineWidth = std::max(1.2f, 2.4f * p.scale);
        ctx.stroke();
        ctx.beginPath();
        ctx.arc(r * 0.72f, r * 0.45f, r * 0.32f, 0, PI * 2);
        ctx.moveTo(-r * 0.78f + r * 0.26f, -r * 0.25f);
        ctx.arc(-r * 0.78f, -r * 0.25f, r * 0.26f, 0, PI * 2);
        ctx.fillStyle = "#522e17";
        ctx.fill();
        ctx.stroke();
        ctx.fillStyle = "rgba(255, 255, 255, 0.75)";
        ctx.beginPath();
        ctx.arc(-r * 0.35f, -r * 0.35f, std::max(0.8f, r * 0.22f), 0, PI * 2);
        ctx.fill();
        ctx.restore();
    }
    for (const MudSplash& s : frogMudSplashes) {
        Point3 p = c.projectWorldPos(s.x, s.y, s.z);
        if (p.scale <= 0) continue;
        ctx.save();
        ctx.globalAlpha = std::min(1.0f, s.life * 1.2f);
        ctx.beginPath();
        ctx.arc(p.x, p.y, std::max(0.8f, s.r * p.scale * s.life), 0, PI * 2);
        ctx.fillStyle = "#4a2a16";
        ctx.fill();
        ctx.strokeStyle = "#0d0603";
        ctx.lineWidth = std::max(0.6f, 1.0f * p.scale);
        ctx.stroke();
        ctx.restore();
    }
}

// ---------------------------------------------------------------------------
// Gêiser do Pântano
// ---------------------------------------------------------------------------

void FrogFx::updateFrogSwampGeyser(LabContext& c) {
    auto& f = c.fighter;
    auto& g = frogGeyser;
    if (f.state == "frog_swamp_spout") {
        const float p = c.progress();
        // O gêiser nasce sob o rival (a 280 à frente), não sob o próprio sapo.
        const float targetZ = c.rival ? std::hypot(c.rival->worldPos.x - c.entity->worldPos.x, c.rival->worldPos.z - c.entity->worldPos.z) : 200.0f;
        if (p < 0.35f) {
            g.erupted = false;
            g.active = false;
            g.height = g.width = 0;
            if (rnd() < 0.6f) {
                const float ang = rnd() * PI * 2, dist = rnd() * 32;
                g.bubbles.push_back({f.body.offsetX + 8 + std::cos(ang) * dist, 0, targetZ + std::sin(ang) * dist, 0.8f + rnd() * 1.5f,
                                     2.0f + rnd() * 3.5f, 1.0f, 0.05f});
            }
        } else if (p < 0.60f) {
            g.active = true;
            g.baseX = f.body.offsetX + 8;
            g.baseZ = targetZ;
            const float subida = std::sin((p - 0.35f) / 0.25f * PI * 0.5f);
            g.height = subida * 480;
            g.width = 68 * subida;
            if (!g.erupted) {
                g.erupted = true;
                g.ripples.push_back({g.baseX, g.baseZ, 10, 95, 1.0f, 0.025f});
            }
            for (int d = 0; d < 4; ++d) {
                const float ang = rnd() * PI * 2, dist = rnd() * g.width * 0.5f;
                g.droplets.push_back({g.baseX + std::cos(ang) * dist, g.height * (0.4f + rnd() * 0.6f), g.baseZ + std::sin(ang) * dist,
                                      std::cos(ang) * (2.0f + rnd() * 4.0f), 4.0f + rnd() * 6.0f, 2.2f + rnd() * 3.0f, 1.0f, 0.025f});
            }
        } else if (p < 0.85f) {
            g.active = true;
            const float descida = 1.0f - std::pow((p - 0.60f) / 0.25f, 1.6f);
            g.height = descida * 480;
            g.width = 68 * std::max(0.1f, descida);
            if (rnd() < 0.25f) g.ripples.push_back({g.baseX, g.baseZ, 5, 80, 1.0f, 0.03f});
        } else {
            g.active = false;
            g.height = g.width = 0;
        }
    } else {
        g.active = false;
        g.height = g.width = 0;
        g.erupted = false;
    }
    for (int i = static_cast<int>(g.bubbles.size()) - 1; i >= 0; --i) {
        g.bubbles[i].y += g.bubbles[i].vy;
        if ((g.bubbles[i].life -= g.bubbles[i].decay) <= 0) g.bubbles.erase(g.bubbles.begin() + i);
    }
    for (int i = static_cast<int>(g.droplets.size()) - 1; i >= 0; --i) {
        GeyserDrop& d = g.droplets[i];
        d.x += d.vx; d.y += d.vy;
        d.vy -= 0.38f;
        d.life -= d.decay;
        if (d.y <= 0 || d.life <= 0) g.droplets.erase(g.droplets.begin() + i);
    }
    for (int i = static_cast<int>(g.ripples.size()) - 1; i >= 0; --i) {
        Ripple& r = g.ripples[i];
        r.r += (r.maxR - r.r) * 0.08f;
        if ((r.life -= r.decay) <= 0) g.ripples.erase(g.ripples.begin() + i);
    }
}

void FrogFx::drawFrogSwampGeyser(Canvas& ctx, LabContext& c) {
    auto& g = frogGeyser;
    for (const GeyserBubble& b : g.bubbles) {
        Point3 p = c.project3D(b.x, b.y, b.z);
        if (p.scale <= 0) continue;
        ctx.save();
        ctx.globalAlpha = std::min(1.0f, b.life * 1.2f);
        ctx.beginPath();
        ctx.arc(p.x, p.y, std::max(1.0f, b.r * p.scale), 0, PI * 2);
        ctx.fillStyle = "rgba(165, 243, 252, 0.75)";
        ctx.fill();
        ctx.strokeStyle = "#0891b2";
        ctx.lineWidth = std::max(0.6f, 1.2f * p.scale);
        ctx.stroke();
        ctx.fillStyle = "#ffffff";
        ctx.beginPath();
        ctx.arc(p.x - b.r * 0.3f * p.scale, p.y - b.r * 0.3f * p.scale, std::max(0.6f, b.r * 0.3f * p.scale), 0, PI * 2);
        ctx.fill();
        ctx.restore();
    }
    for (const Ripple& rip : g.ripples) {
        Point3 p = c.project3D(rip.x, 0, rip.z);
        if (p.scale <= 0) continue;
        ctx.save();
        ctx.globalAlpha = std::min(1.0f, rip.life * 1.3f);
        ctx.beginPath();
        ctx.ellipse(p.x, p.y, rip.r * p.scale, rip.r * 0.35f * p.scale, 0, 0, PI * 2);
        ctx.strokeStyle = "rgba(6, 182, 212, 0.85)";
        ctx.lineWidth = std::max(1.2f, 3.0f * p.scale);
        ctx.stroke();
        ctx.beginPath();
        ctx.ellipse(p.x, p.y, rip.r * 0.85f * p.scale, rip.r * 0.30f * p.scale, 0, 0, PI * 2);
        ctx.strokeStyle = "rgba(255, 255, 255, 0.9)";
        ctx.lineWidth = std::max(0.8f, 1.8f * p.scale);
        ctx.stroke();
        ctx.restore();
    }
    if (g.active && g.height > 5) {
        Point3 pBase = c.project3D(g.baseX, 0, g.baseZ), pTop = c.project3D(g.baseX, g.height, g.baseZ);
        if (pBase.scale > 0 && pTop.scale > 0) {
            const float baseW = g.width * 1.2f * pBase.scale, topW = g.width * 0.85f * pTop.scale;
            const float time = nowMs() / 1000.0f;
            ctx.save();
            auto grad = ctx.createLinearGradient(0, pBase.y, 0, pTop.y);
            grad->addColorStop(0.0f, "rgba(15, 118, 110, 0.88)");
            grad->addColorStop(0.3f, "rgba(6, 182, 212, 0.85)");
            grad->addColorStop(0.7f, "rgba(103, 232, 249, 0.90)");
            grad->addColorStop(1.0f, "rgba(240, 253, 250, 0.95)");
            ctx.beginPath();
            ctx.moveTo(pBase.x - baseW / 2, pBase.y);
            const int segments = 12;
            for (int s = 1; s <= segments; ++s) {
                const float prog = static_cast<float>(s) / segments;
                const float sy = pBase.y + (pTop.y - pBase.y) * prog, currW = baseW + (topW - baseW) * prog;
                ctx.lineTo(pBase.x - currW / 2 + std::sin(time * 18 + prog * 10) * 3.5f * pBase.scale, sy);
            }
            ctx.quadraticCurveTo(pTop.x, pTop.y - 18 * pTop.scale, pBase.x + topW / 2, pTop.y);
            for (int s = segments - 1; s >= 0; --s) {
                const float prog = static_cast<float>(s) / segments;
                const float sy = pBase.y + (pTop.y - pBase.y) * prog, currW = baseW + (topW - baseW) * prog;
                ctx.lineTo(pBase.x + currW / 2 + std::sin(time * 18 + prog * 10 + PI) * 3.5f * pBase.scale, sy);
            }
            ctx.closePath();
            ctx.fillStyle = grad;
            ctx.fill();
            ctx.strokeStyle = "#083344";
            ctx.lineWidth = std::max(1.5f, 3.2f * pBase.scale);
            ctx.stroke();
            ctx.strokeStyle = "rgba(255, 255, 255, 0.75)";
            ctx.lineWidth = std::max(1.0f, 2.2f * pBase.scale);
            for (int l = -2; l <= 2; ++l) {
                const float off = l * baseW * 0.18f, wave = std::sin(time * 24 + l) * 4 * pBase.scale;
                ctx.beginPath();
                ctx.moveTo(pBase.x + off, pBase.y);
                ctx.lineTo(pTop.x + off * 0.7f + wave, pTop.y);
                ctx.stroke();
            }
            for (int r = 0; r < 4; ++r) {
                const float ringProg = std::fmod(time * 1.8f + r / 4.0f, 1.0f);
                const float ringY = pBase.y + (pTop.y - pBase.y) * ringProg;
                const float ringW = (baseW + (topW - baseW) * ringProg) * 1.1f;
                ctx.beginPath();
                ctx.ellipse(pBase.x, ringY, ringW * 0.5f, 5 * pBase.scale, 0, 0, PI * 2);
                ctx.fillStyle = "rgba(255, 255, 255, 0.85)";
                ctx.fill();
                ctx.strokeStyle = "#67e8f9";
                ctx.lineWidth = std::max(0.8f, 1.6f * pBase.scale);
                ctx.stroke();
            }
            ctx.beginPath();
            ctx.ellipse(pTop.x, pTop.y - 4 * pTop.scale, topW * 0.7f, 10 * pTop.scale, 0, 0, PI * 2);
            ctx.fillStyle = "#ffffff";
            ctx.fill();
            ctx.strokeStyle = "#38bdf8";
            ctx.lineWidth = std::max(1.2f, 2.2f * pTop.scale);
            ctx.stroke();
            ctx.restore();
        }
    }
    for (const GeyserDrop& d : g.droplets) {
        Point3 p = c.project3D(d.x, d.y, d.z);
        if (p.scale <= 0) continue;
        ctx.save();
        ctx.globalAlpha = std::min(1.0f, d.life * 1.3f);
        ctx.beginPath();
        ctx.arc(p.x, p.y, std::max(1.2f, d.r * p.scale), 0, PI * 2);
        ctx.fillStyle = "#67e8f9";
        ctx.fill();
        ctx.strokeStyle = "#0e7490";
        ctx.lineWidth = std::max(0.6f, 1.2f * p.scale);
        ctx.stroke();
        ctx.fillStyle = "#ffffff";
        ctx.beginPath();
        ctx.arc(p.x - d.r * 0.3f * p.scale, p.y - d.r * 0.3f * p.scale, std::max(0.6f, d.r * 0.35f * p.scale), 0, PI * 2);
        ctx.fill();
        ctx.restore();
    }
}

// ---------------------------------------------------------------------------
// Captura de Moscas
// ---------------------------------------------------------------------------

void FrogFx::updateFrogFlyCatch(LabContext& c) {
    auto& f = c.fighter;
    if (f.state == "frog_fly_catcher") {
        const float p = c.progress();
        if (!frogFlySwarmSpawned) {
            frogFlySwarmSpawned = true;
            frogFlyHealTriggered = false;
            frogFlies = {{1, 0.5f, 0.088f, 70, 20, 35, 80, false, 0.35f, 0, 0, 0, 0},
                         {2, 2.3f, -0.080f, 80, 25, 42, 90, false, 0.46f, 1.8f, 0, 0, 0},
                         {3, 4.1f, 0.095f, 60, 18, 30, 74, false, 0.57f, 3.2f, 0, 0, 0},
                         {4, 5.5f, -0.085f, 75, 22, 36, 85, false, 0.68f, 4.5f, 0, 0, 0}};
        }
        const float mouthX = f.body.offsetX, mouthY = frogMouthY(c), mouthZ = 12;
        for (Fly& fl : frogFlies) {
            fl.flapPhase += 0.95f;
            fl.angle += fl.speed;
            if (!fl.caught) {
                fl.x = mouthX + std::cos(fl.angle) * fl.rx;
                fl.y = f.body.offsetY + fl.baseHeight + std::sin(fl.angle * 2.2f) * fl.ry;
                fl.z = std::sin(fl.angle) * fl.rz;
                if (p >= fl.caughtP) {
                    fl.caught = true;
                    for (int s = 0; s < 8; ++s) {
                        const float ang = rnd() * PI * 2;
                        frogFlyCatchSparks.push_back({fl.x, fl.y, fl.z, std::cos(ang) * 3.0f, std::sin(ang) * 3.0f, 0, 2.5f + rnd() * 2.0f, "#f43f5e", 1.0f, 0.07f});
                    }
                }
            } else {
                fl.x += (mouthX - fl.x) * 0.55f;
                fl.y += (mouthY - fl.y) * 0.55f;
                fl.z += (mouthZ - fl.z) * 0.55f;
            }
        }
        if (p >= 0.70f && !frogFlyHealTriggered) {
            frogFlyHealTriggered = true;
            for (int s = 0; s < 24; ++s) {
                const float ang = rnd() * PI * 2, dist = rnd() * 34;
                frogFlyCatchSparks.push_back({f.body.offsetX + std::cos(ang) * dist, f.body.offsetY + 30 + rnd() * 65, 10 + (rnd() - 0.5f) * 20,
                                              (rnd() - 0.5f) * 1.8f, 2.2f + rnd() * 2.8f, 0, 3.2f + rnd() * 3.5f,
                                              s % 3 == 0 ? "#4ade80" : (s % 3 == 1 ? "#86efac" : "#facc15"), 1.0f, 0.032f});
            }
        }
    } else {
        frogFlySwarmSpawned = false;
        frogFlyHealTriggered = false;
        frogFlies.clear();
    }
    for (int i = static_cast<int>(frogFlyCatchSparks.size()) - 1; i >= 0; --i) {
        Particle& s = frogFlyCatchSparks[i];
        s.x += s.vx; s.y += s.vy;
        if ((s.life -= s.decay) <= 0) frogFlyCatchSparks.erase(frogFlyCatchSparks.begin() + i);
    }
}

void FrogFx::drawFrogFlies(Canvas& ctx, LabContext& c) {
    for (const Fly& fl : frogFlies) {
        Point3 p = c.project3D(fl.x, fl.y, fl.z);
        if (p.scale <= 0) continue;
        const float s = std::max(2.5f, 6.0f * p.scale);
        ctx.save();
        ctx.translate(p.x, p.y);
        ctx.save();
        ctx.rotate(std::sin(fl.flapPhase) * 0.75f);
        ctx.beginPath();
        ctx.ellipse(-s * 0.6f, -s * 0.8f, s * 0.9f, s * 0.45f, -0.4f, 0, PI * 2);
        ctx.ellipse(s * 0.6f, -s * 0.8f, s * 0.9f, s * 0.45f, 0.4f, 0, PI * 2);
        ctx.fillStyle = "rgba(224, 242, 254, 0.8)";
        ctx.fill();
        ctx.strokeStyle = "#38bdf8";
        ctx.lineWidth = std::max(0.6f, 1.0f * p.scale);
        ctx.stroke();
        ctx.restore();
        ctx.beginPath();
        ctx.ellipse(0, 0, s * 0.8f, s * 0.55f, 0, 0, PI * 2);
        ctx.fillStyle = "#0f172a";
        ctx.fill();
        ctx.strokeStyle = "#020617";
        ctx.lineWidth = std::max(0.8f, 1.5f * p.scale);
        ctx.stroke();
        ctx.strokeStyle = "#475569";
        ctx.lineWidth = std::max(0.6f, 1.0f * p.scale);
        ctx.beginPath();
        ctx.moveTo(-s * 0.2f, -s * 0.4f); ctx.lineTo(-s * 0.2f, s * 0.4f);
        ctx.moveTo(s * 0.2f, -s * 0.4f); ctx.lineTo(s * 0.2f, s * 0.4f);
        ctx.stroke();
        ctx.fillStyle = "#ef4444";
        ctx.beginPath();
        ctx.arc(-s * 0.55f, -s * 0.15f, s * 0.26f, 0, PI * 2);
        ctx.moveTo(s * 0.55f + s * 0.26f, -s * 0.15f);
        ctx.arc(s * 0.55f, -s * 0.15f, s * 0.26f, 0, PI * 2);
        ctx.fill();
        ctx.strokeStyle = "#7f1d1d";
        ctx.lineWidth = std::max(0.5f, 0.8f * p.scale);
        ctx.stroke();
        ctx.restore();
    }
}

void FrogFx::drawFrogFlyCatchTongue(Canvas& ctx, LabContext& c) {
    auto& f = c.fighter;
    if (f.state != "frog_fly_catcher") return;
    const float p = c.progress();
    if (p < 0.25f || p >= 0.70f) return;
    // A língua sai da boca (em espaço 3D, ao contrário do JS que usava a altura do sprite na tela).
    Point3 mouth = c.project3D(f.body.offsetX + f.body.bodySkew * 14, frogMouthY(c), 14);
    if (mouth.scale <= 0) return;
    const float tStrike = (p - 0.25f) / 0.45f;
    const float subStrike = std::fmod(tStrike * 4.0f, 1.0f);
    const int strikeIndex = std::min(3, static_cast<int>(tStrike * 4.0f));
    if (strikeIndex >= static_cast<int>(frogFlies.size())) return;
    const Fly& target = frogFlies[static_cast<std::size_t>(strikeIndex)];
    const float strike = std::sin(subStrike * PI);
    if (strike <= 0.02f) return;
    Point3 pFly = c.project3D(target.x, target.y, target.z);
    if (pFly.scale <= 0) return;
    const float sc = mouth.scale;
    const float startX = mouth.x, startY = mouth.y;
    const float endX = startX + (pFly.x - startX) * strike, endY = startY + (pFly.y - startY) * strike;
    ctx.save();
    const float sideCurve = (strikeIndex % 2 == 0 ? -28.0f : 28.0f) * sc * strike;
    const float ctrlX = (startX + endX) / 2 + sideCurve, ctrlY = std::min(startY, endY) - 18 * sc * strike;
    Point3 sStart = c.project3D(f.body.offsetX, 0, 0);
    Point3 sEnd = c.project3D(f.body.offsetX + (target.x - f.body.offsetX) * strike, 0, target.z * strike);
    ctx.beginPath();
    ctx.moveTo(sStart.x, sStart.y);
    ctx.lineTo(sEnd.x, sEnd.y);
    ctx.strokeStyle = "rgba(0, 0, 0, 0.22)";
    ctx.lineWidth = std::max(2.0f, 6 * sc * strike);
    ctx.stroke();
    ctx.lineCap = "round";
    ctx.beginPath();
    ctx.moveTo(startX, startY);
    ctx.quadraticCurveTo(ctrlX, ctrlY, endX, endY);
    ctx.strokeStyle = frogTone(c, "#881337");
    ctx.lineWidth = std::max(4.5f, 9.5f * sc);
    ctx.stroke();
    ctx.beginPath();
    ctx.moveTo(startX, startY);
    ctx.quadraticCurveTo(ctrlX, ctrlY, endX, endY);
    ctx.strokeStyle = frogTone(c, "#f43f5e");
    ctx.lineWidth = std::max(2.5f, 6.5f * sc);
    ctx.stroke();
    ctx.beginPath();
    ctx.moveTo(startX, startY);
    ctx.quadraticCurveTo(ctrlX, ctrlY - 2 * sc, endX, endY);
    ctx.strokeStyle = frogTone(c, "#fda4af");
    ctx.lineWidth = std::max(1.0f, 2.2f * sc);
    ctx.stroke();
    const float tipR = std::max(4.5f, 9.5f * sc);
    ctx.beginPath();
    ctx.arc(endX, endY, tipR, 0, PI * 2);
    ctx.fillStyle = frogTone(c, "#fb7185");
    ctx.fill();
    ctx.strokeStyle = frogTone(c, "#881337");
    ctx.lineWidth = std::max(1.2f, 2.2f * sc);
    ctx.stroke();
    ctx.fillStyle = "#ffffff";
    ctx.beginPath();
    ctx.arc(endX - 2.5f * sc, endY - 2.5f * sc, std::max(1.2f, 2.8f * sc), 0, PI * 2);
    ctx.fill();
    for (int gIdx = 0; gIdx < 3; ++gIdx) {
        const float ang = (gIdx / 3.0f) * PI * 2 + subStrike * 4;
        const float dist = tipR * (1.2f + 0.5f * std::sin(subStrike * PI));
        ctx.beginPath();
        ctx.arc(endX + std::cos(ang) * dist, endY + std::sin(ang) * dist, std::max(0.8f, 1.6f * sc), 0, PI * 2);
        ctx.fillStyle = "rgba(255, 255, 255, 0.85)";
        ctx.fill();
    }
    ctx.restore();
}

void FrogFx::drawFrogFlyCatchEffects(Canvas& ctx, LabContext& c) {
    for (const Particle& s : frogFlyCatchSparks) {
        Point3 p = c.project3D(s.x, s.y, s.z);
        if (p.scale <= 0) continue;
        ctx.save();
        ctx.globalAlpha = std::min(1.0f, s.life * 1.3f);
        ctx.fillStyle = s.color;
        ctx.beginPath();
        ctx.arc(p.x, p.y, std::max(1.0f, s.radius * p.scale * s.life), 0, PI * 2);
        ctx.fill();
        ctx.strokeStyle = "#ffffff";
        ctx.lineWidth = std::max(0.5f, 0.9f * p.scale);
        ctx.stroke();
        ctx.restore();
    }
}

// ---------------------------------------------------------------------------
// Salto Real
// ---------------------------------------------------------------------------

void FrogFx::triggerFrogLeapTakeoff(float x, float z) {
    for (int i = 0; i < 16; ++i) {
        const float ang = (i / 16.0f) * PI * 2 + (rnd() - 0.5f) * 0.2f, spd = 3.6f + rnd() * 3.4f;
        frogDustPuffs.push_back({x + std::cos(ang) * 12, 0, z + std::sin(ang) * 10, std::cos(ang) * spd, 0.6f + rnd() * 1.5f,
                                 std::sin(ang) * spd * 0.6f, 14 + rnd() * 12, 28 + rnd() * 16, 1.0f, 0.038f + rnd() * 0.02f});
    }
}

void FrogFx::triggerFrogLeapImpact(float x, float z) {
    Crack crack{x, z, 10, 82 + rnd() * 18, 360, 360, {}};
    for (int b = 0; b < 7; ++b) {
        const float baseAng = (b / 7.0f) * PI * 2 + (rnd() - 0.5f) * 0.4f;
        std::vector<CrackSeg> segs;
        float curDist = 0, curAngle = baseAng;
        const int numSegs = 4 + static_cast<int>(rnd() * 3);
        const float segLen = crack.maxR / numSegs;
        for (int s = 0; s < numSegs; ++s) {
            curDist += segLen * (0.8f + rnd() * 0.4f);
            curAngle += (rnd() - 0.5f) * 0.6f;
            segs.push_back({std::cos(curAngle) * curDist, std::sin(curAngle) * curDist * 0.42f, std::max(1.2f, (numSegs - s) * 0.9f)});
        }
        crack.branches.push_back(segs);
    }
    frogGroundCracks.push_back(crack);
    for (int i = 0; i < 20; ++i) {
        const float ang = rnd() * PI * 2, spd = 4.2f + rnd() * 5.0f;
        frogDustPuffs.push_back({x, 0, z, std::cos(ang) * spd, 1.2f + rnd() * 2.8f, std::sin(ang) * spd * 0.7f, 12 + rnd() * 10,
                                 30 + rnd() * 14, 1.0f, 0.04f + rnd() * 0.025f});
    }
    for (int i = 0; i < 18; ++i) {
        const float ang = rnd() * PI * 2, spd = 3.5f + rnd() * 6.5f;
        frogDebrisFragments.push_back({x, 0, z, std::cos(ang) * spd, 4.5f + rnd() * 6.5f, std::sin(ang) * spd * 0.6f, 0.55f,
                                       3.5f + rnd() * 4.5f, rnd() * PI * 2, (rnd() - 0.5f) * 0.3f, 1.0f, 0.025f});
    }
}

void FrogFx::updateFrogRoyalLeap(LabContext& c) {
    auto& f = c.fighter;
    if (f.state == "frog_royal_leap") {
        const float p = c.progress();
        if (p >= 0.20f && !frogLeapDustSpawned) {
            frogLeapDustSpawned = true;
            triggerFrogLeapTakeoff(f.body.offsetX, 0);
        }
        if (p >= 0.58f && !frogLeapImpactSpawned) {
            frogLeapImpactSpawned = true;
            Vec3 wImp = Physics3D::localToWorld(f.body.offsetX, 0.0f, 0.0f, c.entity->worldPos, c.entity->facing);
            triggerFrogLeapImpact(wImp.x, wImp.z);
        }
        if (p >= 0.44f && p < 0.84f) {
            frogFlyingCrown.active = true;
            frogFlyingCrown.x = f.body.offsetX;
            frogFlyingCrown.z = 2;
            const float t = (p - 0.44f) / 0.40f;
            frogFlyingCrown.y = 480 - 480 * std::pow(t, 2.2f);
            frogFlyingCrown.rot = std::sin(t * PI * 4.0f) * 0.18f * (1 - t);
        } else if (p >= 0.84f) {
            if (frogFlyingCrown.active) {
                frogFlyingCrown.active = false;
                for (int s = 0; s < 20; ++s) {
                    const float ang = rnd() * PI * 2, spd = 3.2f + rnd() * 5.0f;
                    frogCrownSparks.push_back({f.body.offsetX, 118, 6, std::cos(ang) * spd, std::sin(ang) * spd + 1.2f, 0, 3.5f + rnd() * 3.5f,
                                               s % 3 == 0 ? "#facc15" : (s % 3 == 1 ? "#e11d48" : "#38bdf8"), 1.0f, 0.038f});
                }
            }
        } else {
            frogFlyingCrown.active = false;
        }
    } else {
        frogLeapDustSpawned = false;
        frogLeapImpactSpawned = false;
        frogFlyingCrown.active = false;
    }
    for (int i = static_cast<int>(frogDustPuffs.size()) - 1; i >= 0; --i) {
        Dust& d = frogDustPuffs[i];
        d.x += d.vx; d.y += d.vy; d.z += d.vz;
        d.radius += (d.maxRadius - d.radius) * 0.10f;
        if ((d.life -= d.decay) <= 0) frogDustPuffs.erase(frogDustPuffs.begin() + i);
    }
    for (int i = static_cast<int>(frogGroundCracks.size()) - 1; i >= 0; --i) {
        Crack& cr = frogGroundCracks[i];
        cr.life -= 1;
        if (cr.curR < cr.maxR) cr.curR += (cr.maxR - cr.curR) * 0.35f;
        if (cr.life <= 0) frogGroundCracks.erase(frogGroundCracks.begin() + i);
    }
    for (int i = static_cast<int>(frogDebrisFragments.size()) - 1; i >= 0; --i) {
        Debris& d = frogDebrisFragments[i];
        d.x += d.vx; d.y += d.vy; d.z += d.vz;
        d.vy -= d.gravity;
        d.rot += d.rotSpeed;
        d.life -= d.decay;
        if (d.y <= 0) { d.y = 0; d.vy = 0; d.vx *= 0.6f; d.vz *= 0.6f; }
        if (d.life <= 0) frogDebrisFragments.erase(frogDebrisFragments.begin() + i);
    }
    for (int i = static_cast<int>(frogCrownSparks.size()) - 1; i >= 0; --i) {
        Particle& s = frogCrownSparks[i];
        s.x += s.vx; s.y += s.vy;
        if ((s.life -= s.decay) <= 0) frogCrownSparks.erase(frogCrownSparks.begin() + i);
    }
}

void FrogFx::drawFrogGroundCracks(Canvas& ctx, LabContext& c) {
    for (const Crack& cr : frogGroundCracks) {
        Point3 pc = c.projectWorldFloor(cr.x, cr.z);
        if (pc.scale <= 0) continue;
        ctx.save();
        ctx.globalAlpha = cr.life > 60 ? 1.0f : cr.life / 60.0f;
        // 1. Base da cratera: void escuro cel-shaded
        ctx.beginPath();
        ctx.ellipse(pc.x, pc.y, cr.curR * 0.40f * pc.scale, cr.curR * 0.18f * pc.scale, 0, 0, PI * 2);
        ctx.fillStyle = "#03060a";
        ctx.fill();
        ctx.strokeStyle = "#000000";
        ctx.lineWidth = std::max(1.5f, 3.2f * pc.scale);
        ctx.stroke();

        // 2. Fissura profunda central
        ctx.beginPath();
        ctx.ellipse(pc.x, pc.y, cr.curR * 0.22f * pc.scale, cr.curR * 0.10f * pc.scale, 0, 0, PI * 2);
        ctx.fillStyle = "#000000";
        ctx.fill();

        // 3. Ramificações profundas cel-shaded (3 passadas: abismo preto, bisel da rocha, realce comic)
        const float frac = std::min(1.0f, cr.curR / cr.maxR);
        for (const auto& branch : cr.branches) {
            if (branch.size() < 2) continue;
            for (int pass = 0; pass < 3; ++pass) {
                ctx.beginPath();
                ctx.moveTo(pc.x, pc.y);
                for (const CrackSeg& seg : branch) {
                    Point3 ps = c.projectWorldFloor(cr.x + seg.x * frac, cr.z + seg.z * frac);
                    ctx.lineTo(ps.x, ps.y);
                }
                if (pass == 0) {
                    ctx.strokeStyle = "#000000";
                    ctx.lineWidth = std::max(2.2f, 4.4f * pc.scale);
                } else if (pass == 1) {
                    ctx.strokeStyle = "#27170b";
                    ctx.lineWidth = std::max(1.4f, 2.6f * pc.scale);
                } else {
                    ctx.strokeStyle = "#d97706";
                    ctx.lineWidth = std::max(0.8f, 1.4f * pc.scale);
                }
                ctx.stroke();
            }
        }
        ctx.restore();
    }
}

void FrogFx::drawFrogDustPuffs(Canvas& ctx, LabContext& c) {
    for (const Dust& d : frogDustPuffs) {
        Point3 p = c.project3D(d.x, d.y, d.z);
        if (p.scale <= 0) continue;
        const float r = d.radius * p.scale;
        ctx.save();
        ctx.globalAlpha = std::min(0.85f, d.life * 1.2f);
        ctx.translate(p.x, p.y);
        ctx.beginPath();
        ctx.arc(0, 0, r, 0, PI * 2);
        ctx.moveTo(-r * 0.45f + r * 0.7f, -r * 0.25f);
        ctx.arc(-r * 0.45f, -r * 0.25f, r * 0.7f, 0, PI * 2);
        ctx.moveTo(r * 0.45f + r * 0.7f, -r * 0.25f);
        ctx.arc(r * 0.45f, -r * 0.25f, r * 0.7f, 0, PI * 2);
        ctx.fillStyle = "#e2d9c8";
        ctx.fill();
        ctx.strokeStyle = "#44403c";
        ctx.lineWidth = std::max(0.8f, 1.6f * p.scale);
        ctx.stroke();
        ctx.restore();
    }
}

void FrogFx::drawFrogDebris(Canvas& ctx, LabContext& c) {
    for (const Debris& d : frogDebrisFragments) {
        Point3 p = c.projectWorldPos(d.x, d.y, d.z);
        if (p.scale <= 0) continue;
        const float s = d.size * p.scale;
        ctx.save();
        ctx.globalAlpha = std::min(1.0f, d.life * 1.5f);
        ctx.translate(p.x, p.y);
        ctx.rotate(d.rot);
        ctx.beginPath();
        ctx.rect(-s / 2, -s / 2, s, s * 0.7f);
        ctx.fillStyle = "#92400e";
        ctx.fill();
        ctx.strokeStyle = "#1c1917";
        ctx.lineWidth = std::max(0.6f, 1.2f * p.scale);
        ctx.stroke();
        ctx.restore();
    }
}

void FrogFx::drawFrogFlyingCrown(Canvas& ctx, LabContext& c) {
    if (!frogFlyingCrown.active || !c.sprites) return;
    Point3 p = c.project3D(frogFlyingCrown.x, c.bodyLift + frogFlyingCrown.y, frogFlyingCrown.z);
    if (p.scale <= 0) return;
    const bool poison = c.entity && c.entity->status.poisonSkin > 0;
    const std::string key = fighterRigKey(1, c.entity ? c.entity->skinId : 0) + "_" + (poison ? "toxic" : "base") + "_" + c.activeCamera + "_hat";
    auto crown = c.sprites->find(key);
    if (!crown) crown = c.sprites->find(poison ? "frogCrownPoisonSprite" : "frogCrownSprite");
    const float bW = c.bodyW * p.scale, bH = c.bodyH * p.scale;
    ctx.save();
    ctx.translate(p.x, p.y);
    ctx.rotate(frogFlyingCrown.rot);
    if (crown) ctx.drawImage(*crown, -bW / 2, -bH, bW, bH);
    ctx.strokeStyle = "rgba(255, 255, 255, 0.85)";
    ctx.lineWidth = std::max(1.0f, 1.8f * p.scale);
    ctx.beginPath();
    ctx.arc(0, -bH * 0.86f, std::max(1.0f, 2.5f * p.scale), 0, PI * 2);
    ctx.stroke();
    ctx.restore();
}

void FrogFx::drawFrogCrownSparks(Canvas& ctx, LabContext& c) {
    for (const Particle& s : frogCrownSparks) {
        Point3 p = c.project3D(s.x, s.y, s.z);
        if (p.scale <= 0) continue;
        ctx.save();
        ctx.globalAlpha = std::min(1.0f, s.life * 1.4f);
        ctx.fillStyle = s.color;
        ctx.beginPath();
        ctx.arc(p.x, p.y, std::max(1.0f, s.radius * p.scale * s.life), 0, PI * 2);
        ctx.fill();
        ctx.strokeStyle = "#ffffff";
        ctx.lineWidth = std::max(0.6f, 1.0f * p.scale);
        ctx.stroke();
        ctx.restore();
    }
}

// ---------------------------------------------------------------------------
// Passivas
// ---------------------------------------------------------------------------

void FrogFx::triggerFrogJumperEffect(LabContext& c) {
    auto& f = c.fighter;
    frogJumperTimer = 120;
    frogJumperSpringRings.push_back({f.body.offsetX - 25, 0, 6, 38 + rnd() * 12, 1.0f, 0.045f});
    frogJumperSpringRings.push_back({f.body.offsetX + 25, 0, 6, 38 + rnd() * 12, 1.0f, 0.045f});
    for (int i = 0; i < 18; ++i) {
        const float ang = rnd() * PI * 2, spd = 2.5f + rnd() * 4.5f;
        frogJumperSparks.push_back({f.body.offsetX + (rnd() - 0.5f) * 35, 4, (rnd() - 0.5f) * 20, std::cos(ang) * spd, 1.0f + rnd() * 4.0f,
                                    std::sin(ang) * spd * 0.6f, 2.5f + rnd() * 3.0f, i % 2 == 0 ? "#4ade80" : "#facc15", 1.0f, 0.04f});
    }
}

void FrogFx::updateFrogJumper(LabContext&) {
    if (frogJumperTimer > 0) frogJumperTimer--;
    for (int i = static_cast<int>(frogJumperSpringRings.size()) - 1; i >= 0; --i) {
        Ring& r = frogJumperSpringRings[i];
        r.r += (r.maxR - r.r) * 0.15f;
        if ((r.life -= r.decay) <= 0) frogJumperSpringRings.erase(frogJumperSpringRings.begin() + i);
    }
    for (int i = static_cast<int>(frogJumperSparks.size()) - 1; i >= 0; --i) {
        Particle& s = frogJumperSparks[i];
        s.x += s.vx; s.y += s.vy; s.z += s.vz;
        s.vy -= 0.25f;
        if (s.y < 0) { s.y = 0; s.vy = 0; }
        if ((s.life -= s.decay) <= 0) frogJumperSparks.erase(frogJumperSparks.begin() + i);
    }
}

void FrogFx::drawFrogJumperEffect(Canvas& ctx, LabContext& c, bool isFrontPass) {
    if (!isFrontPass) return;
    for (const Ring& s : frogJumperSpringRings) {
        Point3 p = c.project3D(s.x, 0, s.z);
        if (p.scale <= 0) continue;
        ctx.save();
        ctx.globalAlpha = s.life * 0.9f;
        ctx.beginPath();
        ctx.ellipse(p.x, p.y, s.r * p.scale * 1.4f, s.r * p.scale * 0.45f, 0, 0, PI * 2);
        ctx.strokeStyle = "#4ade80";
        ctx.lineWidth = std::max(1.5f, 3.5f * p.scale * s.life);
        ctx.stroke();
        ctx.beginPath();
        ctx.ellipse(p.x, p.y, s.r * 0.6f * p.scale * 1.4f, s.r * 0.6f * p.scale * 0.45f, 0, 0, PI * 2);
        ctx.strokeStyle = "#facc15";
        ctx.lineWidth = std::max(1.0f, 2.0f * p.scale * s.life);
        ctx.stroke();
        ctx.restore();
    }
    for (const Particle& sp : frogJumperSparks) {
        Point3 p = c.project3D(sp.x, sp.y, sp.z);
        if (p.scale <= 0) continue;
        ctx.save();
        ctx.globalAlpha = sp.life;
        ctx.fillStyle = sp.color;
        ctx.strokeStyle = "#0f1c10";
        ctx.lineWidth = std::max(0.6f, 1.0f * p.scale);
        ctx.beginPath();
        ctx.arc(p.x, p.y, std::max(1.0f, sp.radius * p.scale * sp.life), 0, PI * 2);
        ctx.fill();
        ctx.stroke();
        ctx.restore();
    }
}

void FrogFx::triggerFrogSlipperySkinEffect(LabContext& c) {
    auto& f = c.fighter;
    frogSlipperyTimer = 120;
    frogSlipperyDeflections.push_back({f.body.offsetX, f.body.offsetY + 65, 15, 12, 52, 1.0f, 0.04f});
    for (int i = 0; i < 16; ++i) {
        const float ang = -PI * 0.2f + (rnd() - 0.5f) * 1.2f, spd = 3.5f + rnd() * 5.0f;
        frogSlipperyDrops.push_back({f.body.offsetX + (rnd() - 0.5f) * 15, f.body.offsetY + 65 + (rnd() - 0.5f) * 15, 15, std::cos(ang) * spd,
                                     std::sin(ang) * spd, (rnd() - 0.5f) * 2, 2.2f + rnd() * 3.0f, "#7dd3fc", 1.0f, 0.035f});
    }
}

void FrogFx::updateFrogSlipperySkin(LabContext&) {
    if (frogSlipperyTimer > 0) frogSlipperyTimer--;
    for (int i = static_cast<int>(frogSlipperyDeflections.size()) - 1; i >= 0; --i) {
        Deflect& d = frogSlipperyDeflections[i];
        d.radius += (d.maxR - d.radius) * 0.16f;
        if ((d.life -= d.decay) <= 0) frogSlipperyDeflections.erase(frogSlipperyDeflections.begin() + i);
    }
    for (int i = static_cast<int>(frogSlipperyDrops.size()) - 1; i >= 0; --i) {
        Particle& d = frogSlipperyDrops[i];
        d.x += d.vx; d.y += d.vy; d.z += d.vz;
        if ((d.life -= d.decay) <= 0) frogSlipperyDrops.erase(frogSlipperyDrops.begin() + i);
    }
}

void FrogFx::drawFrogSlipperySkinEffect(Canvas& ctx, LabContext& c, bool isFrontPass) {
    if (!isFrontPass) return;
    for (const Deflect& d : frogSlipperyDeflections) {
        Point3 p = c.project3D(d.x, d.y, d.z);
        if (p.scale <= 0) continue;
        ctx.save();
        ctx.globalAlpha = d.life * 0.95f;
        ctx.lineCap = "round";
        ctx.beginPath();
        ctx.arc(p.x, p.y, d.radius * p.scale, -PI * 0.8f, PI * 0.3f);
        ctx.strokeStyle = "#38bdf8";
        ctx.lineWidth = std::max(2.5f, 6.0f * p.scale * d.life);
        ctx.stroke();
        ctx.beginPath();
        ctx.arc(p.x, p.y, d.radius * p.scale, -PI * 0.8f, PI * 0.3f);
        ctx.strokeStyle = "#ffffff";
        ctx.lineWidth = std::max(1.0f, 2.5f * p.scale * d.life);
        ctx.stroke();
        ctx.restore();
    }
    for (const Particle& d : frogSlipperyDrops) {
        Point3 p = c.project3D(d.x, d.y, d.z);
        if (p.scale <= 0) continue;
        ctx.save();
        ctx.globalAlpha = d.life;
        ctx.fillStyle = "#7dd3fc";
        ctx.strokeStyle = "#0284c7";
        ctx.lineWidth = std::max(0.6f, 1.0f * p.scale);
        ctx.beginPath();
        ctx.arc(p.x, p.y, std::max(1.0f, d.radius * p.scale), 0, PI * 2);
        ctx.fill();
        ctx.stroke();
        ctx.restore();
    }
}

void FrogFx::updateFrogAmphibianRage(LabContext& c) {
    // Fúria Anfíbia: com stamina abaixo de 30 o sapo bate mais forte — e agora pega fogo também.
    const bool raging = c.entity && c.entity->stamina < 30.0f && c.entity->state != FIGHTER_KNOCKDOWN;
    if (raging) {
        isFrogRageActive = true;
        frogRageTimer = std::max(frogRageTimer, 40);
    }
    if (isFrogRageActive || frogRageTimer > 0) {
        if (--frogRageTimer <= 0) {
            isFrogRageActive = false;
            frogRageTimer = 0;
        }
        if (rnd() < 0.70f) {
            const float bH = c.bodyH;
            frogRageFlames.push_back({c.fighter.body.offsetX + (rnd() - 0.5f) * 55, c.bodyLift + c.fighter.body.offsetY + rnd() * bH * 0.8f,
                                      (rnd() - 0.5f) * 20, (rnd() - 0.5f) * 1.5f, 2.5f + rnd() * 3.5f, 4.0f + rnd() * 5.5f, 9.0f + rnd() * 6.0f,
                                      rnd() < 0.5f ? "#ef4444" : "#f97316", 1.0f, 0.035f + rnd() * 0.02f});
        }
    }
    for (int i = static_cast<int>(frogRageFlames.size()) - 1; i >= 0; --i) {
        Flame& f = frogRageFlames[i];
        f.x += f.vx; f.y += f.vy;
        f.r += (f.maxR - f.r) * 0.06f;
        if ((f.life -= f.decay) <= 0) frogRageFlames.erase(frogRageFlames.begin() + i);
    }
}

void FrogFx::drawFrogAmphibianRageEffect(Canvas& ctx, LabContext& c, bool isFrontPass) {
    if (!isFrogRageActive && frogRageTimer <= 0 && frogRageFlames.empty()) return;
    if (!isFrontPass) return;
    const float time = nowMs() / 1000.0f;
    const float alpha = std::max(0.3f, std::min(1.0f, frogRageTimer / 40.0f));
    for (const Flame& f : frogRageFlames) {
        Point3 p = c.project3D(f.x, f.y, f.z);
        if (p.scale <= 0) continue;
        ctx.save();
        ctx.globalAlpha = f.life * alpha * 0.85f;
        ctx.fillStyle = f.color;
        ctx.beginPath();
        ctx.arc(p.x, p.y, f.r * p.scale * f.life, 0, PI * 2);
        ctx.fill();
        ctx.fillStyle = "#fef08a";
        ctx.beginPath();
        ctx.arc(p.x, p.y, f.r * 0.45f * p.scale * f.life, 0, PI * 2);
        ctx.fill();
        ctx.restore();
    }
    if (!isFrogRageActive || c.activeCamera == "back") return; // os olhos só aparecem de frente
    auto& fb = c.fighter.body;
    for (float side : {-1.0f, 1.0f}) {
        Point3 eye = c.project3D(fb.offsetX + side * 24 + fb.bodySkew * 10, c.bodyLift + fb.offsetY + 108, 12);
        if (eye.scale <= 0) continue;
        ctx.save();
        const float w = (5.0f + std::sin(time * 18) * 2.0f) * eye.scale;
        ctx.fillStyle = "rgba(239, 68, 68, 0.85)";
        ctx.shadowColor = "#ef4444";
        ctx.shadowBlur = 12 * eye.scale;
        ctx.beginPath();
        ctx.arc(eye.x, eye.y, w, 0, PI * 2);
        ctx.fill();
        ctx.shadowBlur = 0;
        ctx.fillStyle = "#fef08a";
        ctx.beginPath();
        ctx.arc(eye.x, eye.y, w * 0.5f, 0, PI * 2);
        ctx.fill();
        ctx.restore();
    }
}

} // namespace

std::unique_ptr<LabFx> makeFrogFx() { return std::make_unique<FrogFx>(); }

} // namespace KoRama::vfx
