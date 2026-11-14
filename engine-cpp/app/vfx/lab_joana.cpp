// Joana Cara-de-Craca. Port de combat_lab_effects.js (linhas 4446-7708): Tesoura Esmagadora, Arrastão
// do Harpão com corrente 3D, Barreira de Cracas, Impacto Coralino, Onda da Âncora, Bálsamo de Anêmona e
// as passivas (Armamento Assimétrico, Carapaça Calcária e a Maré das Profundezas com a mutação de siri).
// Diferenças em relação ao JS: o Lab trocava o sinal de Z conforme a câmera; aqui Z local sempre aponta
// para o rival e a passada (atrás/à frente do corpo) é decidida pela profundidade real. O coral nasce
// sob o rival, o gancho voa até ele e a onda da âncora corre até os pés dele.
#include "lab_common.hpp"

namespace KoRama::vfx {

namespace {

struct Ring { float x, y, z, r, maxR, life, decay; };
struct Cross { float x, y, z, size, maxSize, rot, life, decay; };
struct Spark { float x, y, z, vx, vy, vz, r; const char* color; float life, decay; };
struct Puff { float x, y, z, vx, vy, vz, r, maxR, life, decay; };
struct Splash { float x, y, z, scale, life, decay; int dir; };
struct WaveEllipse { float x, zStart, zEnd, delay, progress, maxRx, maxRy, life, decay; };
struct SpeedLine { float x, zStart, zEnd, curT, speed, lenZ, width; const char* color; float life, decay; };
struct GroundWave { float x, z, r, maxR, life, decay, speed; };
struct AuraRing { float x, y, z, scale, life, decay; };
struct Drop { float x, y, z, vy, r, life, decay; };
struct ReachRing { float x, y, z, radius, life; int delay; float decay; };
struct ReachLine { float x1, y1, z1, x2, y2, z2, life, decay; };
struct Debris { float x, y, z, vx, vy, vz, r, rot, rotSpeed; const char* color; float life, decay; };
struct Bandana { float x, y, z, vx, vy, vz, rot, rotSpeed, wavePhase, life, decay; };
struct Crater { float x, z, life, decay; };

constexpr const char* kInk = "#0f1c10";
const char* joanaColor(const LabContext& c,const char* color) {
    if(!c.entity || c.entity->skinId==0 || c.entity->skinId>=4)return color;
    static const char* source[]={"#ff8533","#e65c00","#a32e00","#f4be41","#fde047","#f72585","#ff758f","#ff0054","#cbd2d0"};
    static const char* palettes[3][9]={
        {"#d1f0ee","#829da9","#3b626c","#d3ede7","#ecffff","#97bcc4","#d0ecec","#b7e7f4","#c9e4e9"},
        {"#63dfcf","#176576","#0b2d47","#a7ffee","#cffff0","#217a8a","#5ac5be","#79e5d6","#8acfc6"},
        {"#dabc7f","#a87b4c","#543d30","#f4d18b","#ffedc3","#866c47","#d3b67c","#e7ca84","#b6b8a6"}
    };
    for(int i=0;i<9;++i)if(std::string(color)==source[i])return palettes[c.entity->skinId-1][i];
    return color;
}
std::optional<SpriteFrame> joanaSprite(LabContext& c,const std::string& name) {
    const auto skin=c.entity?c.entity->skinId:0;
    const std::string key="joana_skin"+std::to_string(skin<4?skin:0)+"_"+(name=="joanaHookFlyingSprite"?"flying":name);
    if(auto selected=c.sprites->find(key))return selected;
    return c.sprites->find(name);
}

template <class V>
void dropDead(V& v) {
    v.erase(std::remove_if(v.begin(), v.end(), [](const auto& e) { return e.life <= 0; }), v.end());
}

class JoanaFx final : public LabFx {
public:
    void update(LabContext& c) override;
    void drawFloor(Canvas& ctx, LabContext& c) override {
        drawAnchorFloor(ctx, c);
        drawCoralFloor(ctx, c);
        drawAnemoneGroundWaves(ctx, c);
    }
    void drawBackground(Canvas& ctx, LabContext& c) override { drawAll(ctx, c, false); }
    void drawForeground(Canvas& ctx, LabContext& c) override { drawAll(ctx, c, true); }
    void drawBodyOverlay(Canvas& ctx, LabContext& c, float bW, float bH) override;

private:
    float bodyDepth(const LabContext& c) const {
        Point3 b = c.project3D(c.fighter.body.offsetX, c.fighter.body.offsetY + 60, 0);
        return b.ok() ? b.z : 0.0f;
    }
    float rivalDist(const LabContext& c) const {
        if (!c.rival || !c.entity) return 125.0f;
        return std::hypot(c.rival->worldPos.x - c.entity->worldPos.x, c.rival->worldPos.z - c.entity->worldPos.z);
    }
    bool frontOf(const LabContext& c, const Point3& p) const { return p.z < bodyDepth(c); }

    void drawAll(Canvas& ctx, LabContext& c, bool front) {
        drawHookChain(ctx, c, front);
        drawBarnacle(ctx, c, front);
        drawCoral(ctx, c, front);
        drawClawCrush(ctx, c, front);
        drawAnemone(ctx, c, front);
        drawAsymmetric(ctx, c, front);
        drawCalcified(ctx, c, front);
        if (!c.liveBody) drawDeepSeaTide(ctx, c, front); // cauda e garras antigas: no visual novo a forma já tem as suas
    }

    void updateClawCrush(LabContext& c);
    void drawClawCrush(Canvas& ctx, LabContext& c, bool front);
    void updateHookPull(LabContext& c);
    void drawHookChain(Canvas& ctx, LabContext& c, bool front);
    void updateBarnacle(LabContext& c);
    void drawBarnacle(Canvas& ctx, LabContext& c, bool front);
    void updateCoral(LabContext& c);
    void drawCoralFloor(Canvas& ctx, LabContext& c);
    void drawCoral(Canvas& ctx, LabContext& c, bool front);
    void updateAnchor(LabContext& c);
    void drawAnchorFloor(Canvas& ctx, LabContext& c);
    void updateAnemone(LabContext& c);
    void drawAnemoneGroundWaves(Canvas& ctx, LabContext& c);
    void drawAnemone(Canvas& ctx, LabContext& c, bool front);
    void drawCartoonAnemone(Canvas& ctx, float cx, float cy, float baseScale, float time, float seed, float growth);
    void triggerAsymmetric(LabContext& c, bool hookSide);
    void updateAsymmetric();
    void drawAsymmetric(Canvas& ctx, LabContext& c, bool front);
    void triggerCalcified(LabContext& c);
    void updateCalcified();
    void drawCalcified(Canvas& ctx, LabContext& c, bool front);
    void triggerDeepSeaTide(LabContext& c);
    void updateDeepSeaTide(LabContext& c);
    void drawDeepSeaTide(Canvas& ctx, LabContext& c, bool front);
    void drawSparks(Canvas& ctx, LabContext& c, const std::vector<Spark>& v, bool front, bool scaled);

    // Tesoura
    std::vector<Ring> clawShockwaves;
    std::vector<Cross> clawCross;
    std::vector<Spark> clawSparks;
    std::vector<Puff> clawPuffs;
    bool lastSnap = false;
    // Gancho
    std::vector<Spark> hookSparks;
    std::vector<Ring> hookRings;
    bool lastClink = false;
    // Cracas
    float barnacleGrowth = 0;
    std::vector<Ring> barnacleBursts;
    std::vector<Puff> barnacleBubbles;
    std::vector<Spark> barnacleSparks;
    bool lastBurst = false;
    // Coral
    struct { bool active = false; float growth = 0, x = 0, z = 0; } coral;
    std::vector<Spark> coralParticles;
    std::vector<Ring> waterRipples;
    std::vector<Splash> waterSplashes;
    bool lastEruption = false, lastSubmerge = false;
    // Âncora
    bool anchorFired = false;
    std::optional<Crater> anchorCrater;
    std::vector<WaveEllipse> waveEllipses;
    std::vector<SpeedLine> speedLines;
    std::vector<Spark> tsunamiDrops;
    // Anêmona
    float anemoneGrowth = 0, absorptionGlow = 0;
    std::vector<GroundWave> groundWaves;
    std::vector<AuraRing> auraRings;
    std::vector<Drop> bioDroplets;
    std::vector<Spark> spores;
    // Passivas
    int asymmetricTimer = 0;
    std::vector<ReachRing> reachRings;
    std::vector<ReachLine> reachLines;
    std::vector<Ring> crushShockwaves;
    std::vector<Spark> heavySparks;
    int calcifiedTimer = 0;
    std::vector<Ring> deflectionArcs;
    std::vector<Debris> calcDebris;
    float mutation = 0;
    bool wasMonstrous = false;
    std::vector<Debris> brutalDebris;
    std::vector<Bandana> bandanas;
    std::string lastState = "idle";
    float lastHp = -1;
};

// =========================================================================================
// TESOURA ESMAGADORA
// =========================================================================================
void JoanaFx::updateClawCrush(LabContext& c) {
    auto& f = c.fighter;
    const bool isCrush = f.state == "joana_claw_crush";
    const float p = isCrush ? c.progress() : 0.0f;
    const int elapsed=f.maxActionTimer-f.actionTimer;
    if (isCrush && elapsed >= 12 && !lastSnap) {
        lastSnap = true;
        const float gx = f.parts.gloveR.x + f.body.offsetX, gy = f.parts.gloveR.y + f.body.offsetY, gz = f.parts.gloveR.z + 25;
        clawShockwaves.push_back({gx, gy, gz, 14, 110, 1.0f, 0.045f});
        clawCross.push_back({gx, gy, gz, 10, 130, PI / 4, 1.0f, 0.05f});
        // Melhoria: um segundo X menor e girado, o "corte duplo" da tesoura.
        clawCross.push_back({gx, gy, gz, 6, 80, 0.0f, 0.85f, 0.06f});
        static const char* cols[] = {"#ff8533", "#fde047", "#ffffff"};
        for (int i = 0; i < 32; i++) {
            const float ang = rnd() * PI * 2, spd = 5.0f + rnd() * 9.5f;
            clawSparks.push_back({gx + (rnd() - 0.5f) * 16, gy + (rnd() - 0.5f) * 16, gz + (rnd() - 0.5f) * 8, std::cos(ang) * spd,
                                  std::sin(ang) * spd, (rnd() - 0.5f) * 4, 2.5f + rnd() * 3.8f, cols[i % 3], 1.0f, 0.035f + rnd() * 0.02f});
        }
        for (int i = 0; i < 8; i++) {
            const float ang = rnd() * PI * 2, spd = 1.2f + rnd() * 2.5f;
            clawPuffs.push_back({gx, gy, gz - 8, std::cos(ang) * spd, std::sin(ang) * spd + 0.5f, (rnd() - 0.5f) * 2, 8 + rnd() * 10,
                                 24 + rnd() * 14, 1.0f, 0.035f});
        }
    } else if (!isCrush || p < 0.30f) {
        lastSnap = false;
    }
    if (c.isPaused) return;
    for (auto& s : clawShockwaves) { s.r += (s.maxR - s.r) * 0.22f; s.life -= s.decay; }
    for (auto& s : clawCross) { s.size += (s.maxSize - s.size) * 0.25f; s.life -= s.decay; }
    for (auto& s : clawSparks) { s.x += s.vx; s.y += s.vy; s.z += s.vz; s.vy -= 0.15f; s.life -= s.decay; }
    for (auto& s : clawPuffs) { s.x += s.vx; s.y += s.vy; s.r += (s.maxR - s.r) * 0.1f; s.life -= s.decay; }
    dropDead(clawShockwaves); dropDead(clawCross); dropDead(clawSparks); dropDead(clawPuffs);
}

void JoanaFx::drawSparks(Canvas& ctx, LabContext& c, const std::vector<Spark>& v, bool front, bool scaled) {
    for (const Spark& sp : v) {
        Point3 p = c.project3D(sp.x, sp.y, sp.z);
        if (!p.ok() || frontOf(c, p) != front) continue;
        ctx.save();
        ctx.globalAlpha = std::clamp(sp.life, 0.0f, 1.0f);
        ctx.fillStyle = joanaColor(c,sp.color);
        ctx.strokeStyle = kInk;
        ctx.lineWidth = std::max(0.6f, 1.2f * p.scale);
        ctx.beginPath();
        ctx.arc(p.x, p.y, std::max(1.0f, sp.r * (scaled ? p.scale : 1.0f) * sp.life), 0, PI * 2);
        ctx.fill();
        ctx.stroke();
        ctx.restore();
    }
}

void JoanaFx::drawClawCrush(Canvas& ctx, LabContext& c, bool front) {
    for (const Puff& pf : clawPuffs) {
        Point3 p = c.project3D(pf.x, pf.y, pf.z);
        if (!p.ok() || frontOf(c, p) != front) continue;
        ctx.save();
        ctx.globalAlpha = pf.life * 0.65f;
        ctx.fillStyle = "#e2e8f0";
        ctx.beginPath();
        ctx.arc(p.x, p.y, pf.r * p.scale, 0, PI * 2);
        ctx.fill();
        ctx.restore();
    }
    for (const Ring& sw : clawShockwaves) {
        Point3 p = c.project3D(sw.x, sw.y, sw.z);
        if (!p.ok() || frontOf(c, p) != front) continue;
        ctx.save();
        ctx.globalAlpha = sw.life * 0.95f;
        ctx.beginPath();
        ctx.arc(p.x, p.y, sw.r * p.scale, 0, PI * 2);
        ctx.strokeStyle = "#e65c00";
        ctx.lineWidth = std::max(3.0f, 7.5f * p.scale * sw.life);
        ctx.stroke();
        ctx.beginPath();
        ctx.arc(p.x, p.y, sw.r * 0.82f * p.scale, 0, PI * 2);
        ctx.strokeStyle = "#ffffff";
        ctx.lineWidth = std::max(1.5f, 3.5f * p.scale * sw.life);
        ctx.stroke();
        ctx.restore();
    }
    for (const Cross& cf : clawCross) {
        Point3 p = c.project3D(cf.x, cf.y, cf.z);
        if (!p.ok() || frontOf(c, p) != front) continue;
        ctx.save();
        ctx.translate(p.x, p.y);
        ctx.rotate(cf.rot);
        ctx.globalAlpha = std::clamp(cf.life, 0.0f, 1.0f);
        const float s = cf.size * p.scale, w = std::max(2.0f, 4.5f * p.scale * cf.life);
        ctx.fillStyle = "#ffffff";
        ctx.strokeStyle = "#ff8533";
        ctx.lineWidth = std::max(1.0f, 2.0f * p.scale);
        ctx.beginPath();
        ctx.moveTo(-s, -w); ctx.lineTo(s, -w); ctx.lineTo(s * 1.3f, 0); ctx.lineTo(s, w); ctx.lineTo(-s, w); ctx.lineTo(-s * 1.3f, 0);
        ctx.closePath();
        ctx.fill(); ctx.stroke();
        ctx.beginPath();
        ctx.moveTo(-w, -s); ctx.lineTo(-w, s); ctx.lineTo(0, s * 1.3f); ctx.lineTo(w, s); ctx.lineTo(w, -s); ctx.lineTo(0, -s * 1.3f);
        ctx.closePath();
        ctx.fill(); ctx.stroke();
        ctx.restore();
    }
    drawSparks(ctx, c, clawSparks, front, true);
}

// =========================================================================================
// ARRASTÃO DO HARPÃO
// =========================================================================================
void JoanaFx::updateHookPull(LabContext& c) {
    auto& f = c.fighter;
    const bool isHook = f.state == "joana_hook_pull";
    const float p = isHook ? c.progress() : 0.0f;
    const float tz = std::max(90.0f, rivalDist(c) * 0.85f);
    if (isHook && p >= 0.44f && p < 0.66f && !c.isPaused && rnd() < 0.5f) {
        const float ang = ((p - 0.44f) / 0.22f) * PI * 4;
        const float hx = 2 + std::cos(ang) * 22, hy = 75 + std::sin(ang) * 14;
        hookSparks.push_back({hx + (rnd() - 0.5f) * 10, hy + (rnd() - 0.5f) * 10, tz + (rnd() - 0.5f) * 10,
                              std::cos(ang + PI / 2) * (3.5f + rnd() * 4.5f), std::sin(ang + PI / 2) * (3.5f + rnd() * 4.5f),
                              (rnd() - 0.5f) * 3, 2.0f + rnd() * 2.8f, rnd() < 0.65f ? "#cbd2d0" : "#f4be41", 1.0f, 0.045f});
    }
    if (isHook && p >= 0.88f && !lastClink) {
        lastClink = true;
        const float gx = f.parts.gloveL.x + f.body.offsetX, gy = f.parts.gloveL.y + f.body.offsetY, gz = f.parts.gloveL.z;
        hookRings.push_back({gx, gy, gz, 8, 55, 1.0f, 0.06f});
        for (int i = 0; i < 20; i++) {
            const float ang = rnd() * PI * 2, spd = 3.5f + rnd() * 6.5f;
            hookSparks.push_back({gx, gy, gz, std::cos(ang) * spd, std::sin(ang) * spd, (rnd() - 0.5f) * 3.5f, 1.8f + rnd() * 2.8f,
                                  (i % 2 == 0) ? "#f4be41" : "#ffffff", 1.0f, 0.038f + rnd() * 0.02f});
        }
    } else if (!isHook || p < 0.30f) {
        lastClink = false;
    }
    if (c.isPaused) return;
    for (auto& r : hookRings) { r.r += (r.maxR - r.r) * 0.24f; r.life -= r.decay; }
    for (auto& s : hookSparks) { s.x += s.vx; s.y += s.vy; s.z += s.vz; s.vy -= 0.12f; s.life -= s.decay; }
    dropDead(hookRings); dropDead(hookSparks);
}

void JoanaFx::drawHookChain(Canvas& ctx, LabContext& c, bool front) {
    auto& f = c.fighter;
    const bool isHook = f.state == "joana_hook_pull";
    const float p = isHook ? c.progress() : 0.0f;
    const float armX = f.parts.gloveL.x + f.body.offsetX, armY = f.parts.gloveL.y + f.body.offsetY, armZ = f.parts.gloveL.z + 25;
    const float tz = std::max(90.0f, rivalDist(c) * 0.85f), ty = 75.0f;
    const float side = c.activeCamera == "back" ? 1.0f : -1.0f;

    if (isHook && p >= 0.24f && p < 0.88f) {
        float hx = armX, hy = armY, hz = armZ, hookRot = 0;
        bool tense = false;
        if (p < 0.44f) {
            const float t = (p - 0.24f) / 0.20f, e = std::pow(t, 1.8f);
            hx = armX + (4 - armX) * e;
            hy = armY + (ty - armY) * e;
            hz = armZ + (tz - armZ) * e;
            hookRot = side * (0.35f + 0.5f * (1 - e));
        } else if (p < 0.66f) {
            const float t = (p - 0.44f) / 0.22f, ang = t * PI * 2;
            hx = 4 + std::cos(ang) * 24;
            hy = ty + std::sin(ang) * 16;
            hz = tz + std::sin(ang) * 6;
            hookRot = side * -ang + PI * 0.5f;
        } else {
            const float t = (p - 0.66f) / 0.22f, e = std::sin(t * PI * 0.5f), sx = 28;
            hx = sx + (armX - sx) * e;
            hy = ty + (armY - ty) * e;
            hz = tz + (armZ - tz) * e;
            hookRot = side * (1 - e) * 0.25f;
            tense = true;
        }
        const int numLinks = 16;
        for (int i = 0; i <= numLinks; i++) {
            const float s = static_cast<float>(i) / numLinks;
            float lx = armX + (hx - armX) * s, ly = armY + (hy - armY) * s, lz = armZ + (hz - armZ) * s;
            if (!tense) {
                const float wave = std::sin(s * PI) * 8 * std::sin(s * 4 + p * 12);
                lx += wave * 0.4f;
                ly -= wave * 0.25f;
            } else {
                // Melhoria: a corrente esticada vibra na retração.
                ly += std::sin(s * PI * 3 + p * 60) * 1.5f * (1 - s);
            }
            Point3 lp = c.project3D(lx, ly, lz);
            if (!lp.ok() || frontOf(c, lp) != front) continue;
            ctx.save();
            ctx.translate(lp.x, lp.y);
            const bool odd = i % 2 == 1;
            const float ew = (odd ? 5.5f : 10.0f) * lp.scale, eh = (odd ? 10.0f : 5.5f) * lp.scale;
            ctx.beginPath();
            ctx.ellipse(0, 0, ew, eh, 0, 0, PI * 2);
            ctx.fillStyle = joanaColor(c,"#cbd2d0");
            ctx.strokeStyle = kInk;
            ctx.lineWidth = std::max(1.2f, 2.4f * lp.scale);
            ctx.fill(); ctx.stroke();
            ctx.beginPath();
            ctx.ellipse(0, 0, ew * 0.42f, eh * 0.42f, 0, 0, PI * 2);
            ctx.fillStyle = "#2d3748";
            ctx.fill(); ctx.stroke();
            ctx.beginPath();
            ctx.ellipse(-ew * 0.22f, -eh * 0.22f, ew * 0.22f, eh * 0.22f, 0, 0, PI * 2);
            ctx.fillStyle = "#ffffff";
            ctx.fill();
            ctx.restore();
        }
        Point3 hp = c.project3D(hx, hy, hz);
        if (hp.ok() && frontOf(c, hp) == front) {
            if (auto img = joanaSprite(c,"joanaHookFlyingSprite")) {
                ctx.save();
                ctx.translate(hp.x, hp.y);
                ctx.rotate(hookRot);
                const float hs = 72 * hp.scale * (c.entity&&c.entity->skinId==1?1.5f:1.0f);
                // Rastro de velocidade do harpão no arremesso.
                if (p < 0.44f) {
                    ctx.globalAlpha = 0.25f;
                    ctx.drawImage(*img, -hs / 2 - 6 * side, -hs / 2 + 4, hs, hs);
                    ctx.globalAlpha = 1.0f;
                }
                ctx.drawImage(*img, -hs / 2, -hs / 2, hs, hs);
                ctx.restore();
            }
        }
    }
    for (const Ring& r : hookRings) {
        Point3 pp = c.project3D(r.x, r.y, r.z);
        if (!pp.ok() || frontOf(c, pp) != front) continue;
        ctx.save();
        ctx.globalAlpha = r.life * 0.9f;
        ctx.beginPath();
        ctx.arc(pp.x, pp.y, r.r * pp.scale, 0, PI * 2);
        ctx.strokeStyle = "#f4be41";
        ctx.lineWidth = std::max(2.0f, 5.0f * pp.scale * r.life);
        ctx.stroke();
        ctx.beginPath();
        ctx.arc(pp.x, pp.y, r.r * 0.8f * pp.scale, 0, PI * 2);
        ctx.strokeStyle = "#ffffff";
        ctx.lineWidth = std::max(1.0f, 2.5f * pp.scale * r.life);
        ctx.stroke();
        ctx.restore();
    }
    drawSparks(ctx, c, hookSparks, front, true);
}

// =========================================================================================
// BARREIRA DE CRACAS
// =========================================================================================
void JoanaFx::updateBarnacle(LabContext& c) {
    auto& f = c.fighter;
    const bool isShell = f.state == "joana_crustacean_shell";
    const float p = isShell ? c.progress() : 0.0f;
    if (isShell && p >= 0.35f && !lastBurst) {
        lastBurst = true;
        const float gx = f.body.offsetX, gy = f.body.offsetY + 70, gz = 0;
        barnacleBursts.push_back({gx, gy, gz, 15, 90, 1.0f, 0.04f});
        for (int i = 0; i < 24; i++) {
            const float ang = rnd() * PI * 2, spd = 2.5f + rnd() * 5.5f;
            barnacleBubbles.push_back({gx + (rnd() - 0.5f) * 20, gy + (rnd() - 0.5f) * 20, gz + (rnd() - 0.5f) * 30, std::cos(ang) * spd,
                                       std::sin(ang) * spd + 0.6f, (rnd() - 0.5f) * 3, 5 + rnd() * 9, 0, 1.0f, 0.025f + rnd() * 0.02f});
        }
        static const char* cols[] = {"#f4be41", "#ba9e7b", "#a8dadc"};
        for (int i = 0; i < 22; i++) {
            const float ang = rnd() * PI * 2, spd = 3.5f + rnd() * 6;
            barnacleSparks.push_back({gx + (rnd() - 0.5f) * 25, gy + (rnd() - 0.5f) * 30, gz + (rnd() - 0.5f) * 20, std::cos(ang) * spd,
                                      std::sin(ang) * spd, (rnd() - 0.5f) * 3.5f, 2.2f + rnd() * 2.8f, cols[i % 3], 1.0f, 0.035f});
        }
    } else if (!isShell || p < 0.20f) {
        lastBurst = false;
    }
    const int shell = c.entity ? c.entity->status.shell : 0;
    if (!c.isPaused) {
        if (shell > 0) {
            const float sTimer = shell / 60.0f;
            if (sTimer > 4.5f) barnacleGrowth = std::min(1.0f, std::sin((5.0f - sTimer) / 0.5f * PI * 0.5f) * 1.05f);
            else if (sTimer > 0.6f) barnacleGrowth = 1.0f;
            else barnacleGrowth = std::max(0.0f, std::sin(sTimer / 0.6f * PI * 0.5f));
            if (rnd() < 0.35f) {
                const float ang = rnd() * PI * 2, dist = 15 + rnd() * 20;
                barnacleBubbles.push_back({f.body.offsetX + std::cos(ang) * dist, f.body.offsetY + 50 + (rnd() - 0.5f) * 25, std::sin(ang) * dist,
                                           (rnd() - 0.5f) * 0.8f, 1.2f + rnd() * 1.5f, (rnd() - 0.5f) * 0.8f, 4 + rnd() * 7, 0, 1.0f, 0.02f});
            }
        } else if (isShell) {
            barnacleGrowth = p < 0.35f ? (p / 0.35f) * 0.5f : 0.5f + ((p - 0.35f) / 0.65f) * 0.5f;
        } else {
            barnacleGrowth = std::max(0.0f, barnacleGrowth - 0.05f);
        }
        for (auto& b : barnacleBursts) { b.r += (b.maxR - b.r) * 0.18f; b.life -= b.decay; }
        for (auto& b : barnacleBubbles) { b.x += b.vx; b.y += b.vy; b.z += b.vz; b.life -= b.decay; }
        for (auto& s : barnacleSparks) { s.x += s.vx; s.y += s.vy; s.z += s.vz; s.vy -= 0.10f; s.life -= s.decay; }
        dropDead(barnacleBursts); dropDead(barnacleBubbles); dropDead(barnacleSparks);
    }
}

void JoanaFx::drawBarnacle(Canvas& ctx, LabContext& c, bool front) {
    auto bubble = joanaSprite(c,"joanaSeaBubbleSprite");
    for (const Puff& bb : barnacleBubbles) {
        Point3 bp = c.project3D(bb.x, bb.y, bb.z);
        if (!bp.ok() || frontOf(c, bp) != front) continue;
        ctx.save();
        ctx.globalAlpha = bb.life * 0.85f * (barnacleGrowth > 0 ? 1.0f : 0.4f);
        const float bs = bb.r * 2 * bp.scale;
        if (bubble) {
            ctx.drawImage(*bubble, bp.x - bs / 2, bp.y - bs / 2, bs, bs);
        } else {
            ctx.beginPath();
            ctx.arc(bp.x, bp.y, bb.r * bp.scale, 0, PI * 2);
            ctx.fillStyle = "rgba(168, 218, 220, 0.4)";
            ctx.strokeStyle = kInk;
            ctx.lineWidth = 1.5f;
            ctx.fill(); ctx.stroke();
        }
        ctx.restore();
    }
    for (const Ring& b : barnacleBursts) {
        Point3 bp = c.project3D(b.x, b.y, b.z);
        if (!bp.ok() || frontOf(c, bp) != front) continue;
        ctx.save();
        ctx.globalAlpha = b.life * 0.85f;
        ctx.beginPath();
        ctx.arc(bp.x, bp.y, b.r * bp.scale, 0, PI * 2);
        ctx.strokeStyle = "#457b9d";
        ctx.lineWidth = std::max(3.0f, 7.0f * bp.scale * b.life);
        ctx.stroke();
        ctx.beginPath();
        ctx.arc(bp.x, bp.y, b.r * 0.82f * bp.scale, 0, PI * 2);
        ctx.strokeStyle = "#f4be41";
        ctx.lineWidth = std::max(1.5f, 3.5f * bp.scale * b.life);
        ctx.stroke();
        ctx.restore();
    }
    drawSparks(ctx, c, barnacleSparks, front, true);
}

// =========================================================================================
// IMPACTO CORALINO (nasce sob o rival)
// =========================================================================================
void JoanaFx::updateCoral(LabContext& c) {
    auto& f = c.fighter;
    const bool isCoral = f.state == "joana_coral_spout";
    const float p = isCoral ? c.progress() : 0.0f;
    if (isCoral) {
        coral.active = true;
        coral.x = f.body.offsetX + 4;
        coral.z = std::max(60.0f, rivalDist(c) - 10.0f);
        if (p < 0.30f) {
            coral.growth = 0;
            lastEruption = lastSubmerge = false;
            // Melhoria: o chão borbulha sob o rival durante a carga.
            if (!c.isPaused && p > 0.1f && rnd() < 0.5f) {
                coralParticles.push_back({coral.x + (rnd() - 0.5f) * 30, 2, coral.z + (rnd() - 0.5f) * 20, (rnd() - 0.5f) * 0.6f,
                                          1.5f + rnd() * 1.5f, 0, 1.2f + rnd() * 1.6f, rnd() < 0.5f ? "#a8dadc" : "#ffffff", 0.8f, 0.04f});
            }
        } else if (p < 0.58f) {
            const float t = (p - 0.30f) / 0.28f;
            coral.growth = std::min(1.0f, std::sin(t * PI * 0.5f) * 1.05f);
            if (!lastEruption && p >= 0.32f) {
                lastEruption = true;
                for (int i = 0; i < 3; i++) waterRipples.push_back({coral.x, 0, coral.z, 8.0f + i * 10, 34.0f + i * 12, 1.0f, 0.026f - i * 0.003f});
                waterSplashes.push_back({coral.x - 18, 0, coral.z, 0.75f, 1.0f, 0.042f, -1});
                waterSplashes.push_back({coral.x + 18, 0, coral.z, 0.75f, 1.0f, 0.042f, 1});
                static const char* cols[] = {"#a8dadc", "#ffffff", "#457b9d", "#f72585", "#ff758f", "#2a9d8f"};
                for (int i = 0; i < 18; i++) {
                    const float ang = rnd() * PI * 2, spd = 2.4f + rnd() * 4.5f;
                    coralParticles.push_back({coral.x + (rnd() - 0.5f) * 16, 4 + rnd() * 12, coral.z + (rnd() - 0.5f) * 16, std::cos(ang) * spd,
                                              3.0f + rnd() * 4.5f, (rnd() - 0.5f) * 2.8f, 1.5f + rnd() * 2, cols[i % 6], 1.0f, 0.028f + rnd() * 0.015f});
                }
            }
        } else if (p < 0.80f) {
            coral.growth = 1.0f;
            if (!c.isPaused && rnd() < 0.3f) {
                coralParticles.push_back({coral.x + (rnd() - 0.5f) * 18, 55 + rnd() * 15, coral.z + (rnd() - 0.5f) * 12, (rnd() - 0.5f),
                                          -(1.2f + rnd() * 2), (rnd() - 0.5f), 1.5f + rnd() * 1.5f, rnd() < 0.6f ? "#a8dadc" : "#ffffff", 0.85f, 0.038f});
            }
        } else {
            const float t = (p - 0.80f) / 0.20f;
            coral.growth = std::max(0.0f, std::cos(t * PI * 0.5f));
            if (!lastSubmerge && p >= 0.82f) {
                lastSubmerge = true;
                waterRipples.push_back({coral.x, 0, coral.z, 10, 38, 1.0f, 0.032f});
            }
        }
    } else {
        coral.active = false;
        coral.growth = 0;
        lastEruption = lastSubmerge = false;
    }
    if (c.isPaused) return;
    for (auto& r : waterRipples) { r.r += (r.maxR - r.r) * 0.14f; r.life -= r.decay; }
    for (auto& s : waterSplashes) s.life -= s.decay;
    for (auto& pt : coralParticles) {
        pt.x += pt.vx; pt.y += pt.vy; pt.z += pt.vz; pt.vy -= 0.22f; pt.life -= pt.decay;
        if (pt.y < 0) pt.life = 0;
    }
    dropDead(waterRipples); dropDead(waterSplashes); dropDead(coralParticles);
}

void JoanaFx::drawCoralFloor(Canvas& ctx, LabContext& c) {
    for (const Ring& r : waterRipples) {
        Point3 bp = c.project3D(r.x, 0, r.z);
        if (!bp.ok()) continue;
        ctx.save();
        ctx.globalAlpha = r.life * 0.9f;
        const float rw = r.r * 2.4f * bp.scale, rh = r.r * 0.85f * bp.scale;
        ctx.beginPath();
        ctx.ellipse(bp.x, bp.y, rw / 2, rh / 2, 0, 0, PI * 2);
        ctx.fillStyle = "rgba(42, 157, 143, 0.40)";
        ctx.strokeStyle = kInk;
        ctx.lineWidth = 1.5f;
        ctx.fill(); ctx.stroke();
        ctx.beginPath();
        ctx.ellipse(bp.x, bp.y, rw * 0.42f, rh * 0.42f, 0, 0, PI * 2);
        ctx.strokeStyle = "#ffffff";
        ctx.lineWidth = std::max(1.5f, 3.0f * r.life);
        ctx.stroke();
        ctx.beginPath();
        ctx.ellipse(bp.x, bp.y, rw * 0.25f, rh * 0.25f, 0, 0, PI * 2);
        ctx.strokeStyle = "#a8dadc";
        ctx.lineWidth = std::max(1.0f, 2.0f * r.life);
        ctx.stroke();
        ctx.restore();
    }
    if (coral.active && coral.growth > 0.01f) {
        Point3 bp = c.project3D(coral.x, 0, coral.z);
        if (bp.ok()) {
            if (auto crack = c.sprites->find("joanaCoralGroundCrackSprite")) {
                const float cw = 68 * bp.scale * 1.3f, ch = 24 * bp.scale * 1.3f;
                ctx.save();
                ctx.globalAlpha = std::min(1.0f, coral.growth * 1.6f);
                ctx.drawImage(*crack, bp.x - cw / 2, bp.y - ch / 2, cw, ch);
                ctx.restore();
            }
        }
    }
}

void JoanaFx::drawCoral(Canvas& ctx, LabContext& c, bool front) {
    Point3 base = c.project3D(coral.x, 0, coral.z);
    const bool coralFront = base.ok() && frontOf(c, base);
    const float g = coral.growth;
    const float time = nowMs() * 0.008f;
    if (base.ok() && coralFront == front && coral.active && g > 0.01f) {
        const float k = base.scale * 1.3f; // o Lab desenhava em pixels fixos; aqui acompanha a perspectiva
        const char* stage = g < 0.35f ? "joanaCoralStage1Sprite" : (g < 0.70f ? "joanaCoralStage2Sprite" : "joanaCoralStage3Sprite");
        if (auto img = joanaSprite(c,stage)) {
            const float wiggleMain = std::sin(time * 3) * (0.04f * g);
            const float wl = std::sin(time * 3.6f + 1.2f) * (0.12f * g), wr = std::sin(time * 3.4f + 2.6f) * (0.12f * g);
            const float pulse = 1.0f + std::sin(time * 4) * (0.03f * g);
            const float fullH = 92 * k, curH = fullH * g, coralW = 72 * pulse * k;
            ctx.save();
            ctx.beginPath();
            ctx.rect(base.x - coralW * 2, base.y - curH - fullH * 0.4f, coralW * 4, curH + fullH * 0.4f + 2);
            ctx.clip();
            ctx.translate(base.x, base.y);
            ctx.rotate(wiggleMain);
            ctx.drawImage(*img, -coralW / 2, -fullH + (fullH - curH), coralW, fullH);
            ctx.translate(0, fullH - curH);
            auto branch = [&](float sx, float sy, float rot, float life, bool big) {
                ctx.save();
                ctx.translate(sx, sy);
                ctx.rotate(rot);
                ctx.scale(life * k, life * k);
                ctx.beginPath();
                const float d = sx < 0 ? -1.0f : 1.0f;
                if (big) {
                    ctx.moveTo(0, 0);
                    ctx.quadraticCurveTo(15 * d, -14, 25 * d, -30);
                    ctx.quadraticCurveTo(10 * d, -18, 0, -8);
                } else {
                    ctx.moveTo(0, 0); ctx.lineTo(14 * d, -15); ctx.lineTo(5 * d, -14);
                }
                ctx.closePath();
                ctx.fillStyle = joanaColor(c,big ? "#f72585" : "#ff758f");
                ctx.strokeStyle = kInk;
                ctx.lineWidth = big ? 1.5f : 1.2f;
                ctx.fill(); ctx.stroke();
                if (big) {
                    ctx.beginPath();
                    ctx.arc(25 * d, -30, 2.8f, 0, PI * 2);
                    ctx.fillStyle = "#ffffff";
                    ctx.fill(); ctx.stroke();
                }
                ctx.restore();
            };
            if (g >= 0.35f) {
                const float bl = std::min(1.0f, (g - 0.35f) / 0.35f);
                branch(-coralW * 0.28f, -fullH * 0.45f, -0.45f * bl + wl, bl, true);
                branch(coralW * 0.28f, -fullH * 0.45f, 0.45f * bl + wr, bl, true);
            }
            if (g >= 0.65f) {
                const float tl = std::min(1.0f, (g - 0.65f) / 0.35f);
                branch(-coralW * 0.15f, -fullH * 0.72f, -0.55f * tl + wl * 1.5f, tl, false);
                branch(coralW * 0.15f, -fullH * 0.72f, 0.55f * tl + wr * 1.5f, tl, false);
                const float glow = 0.25f + 0.25f * std::sin(time * 5);
                ctx.globalAlpha = g * glow;
                ctx.globalCompositeOperation = "lighter";
                ctx.beginPath();
                ctx.arc(0, -fullH * 0.9f, 13 * k, 0, PI * 2);
                ctx.fillStyle = joanaColor(c,"#ff0054");
                ctx.fill();
                ctx.beginPath();
                ctx.arc(0, -fullH * 0.9f, 6 * k, 0, PI * 2);
                ctx.fillStyle = "#ffffff";
                ctx.fill();
            }
            ctx.restore();
        }
        // Crista frontal de espuma por cima da base do coral.
        for (const Ring& r : waterRipples) {
            Point3 bp = c.project3D(r.x, 0, r.z);
            if (!bp.ok()) continue;
            ctx.save();
            ctx.globalAlpha = r.life * 0.95f;
            const float rw = r.r * 2.4f * bp.scale, rh = r.r * 0.85f * bp.scale;
            ctx.beginPath();
            ctx.ellipse(bp.x, bp.y, rw / 2, rh / 2, 0, 0, PI);
            ctx.strokeStyle = "#ffffff";
            ctx.lineWidth = std::max(1.8f, 3.2f * r.life);
            ctx.stroke();
            ctx.beginPath();
            ctx.ellipse(bp.x, bp.y, rw * 0.42f, rh * 0.42f, 0, 0, PI);
            ctx.strokeStyle = "#a8dadc";
            ctx.lineWidth = std::max(1.2f, 2.2f * r.life);
            ctx.stroke();
            ctx.restore();
        }
    }
    if (base.ok() && coralFront == front) {
        if (auto img = joanaSprite(c,"joanaWaterSplashSprite")) {
            for (const Splash& sp : waterSplashes) {
                Point3 bp = c.project3D(sp.x, 0, sp.z);
                if (!bp.ok()) continue;
                ctx.save();
                ctx.globalAlpha = sp.life;
                const float grow = 1.0f + (1.0f - sp.life) * 0.6f;
                const float sw = 32 * sp.scale * bp.scale * 1.4f * grow, sh = 26 * sp.scale * bp.scale * 1.4f * grow;
                ctx.translate(bp.x, bp.y);
                if (sp.dir < 0) ctx.scale(-1, 1);
                ctx.drawImage(*img, -sw / 2, -sh, sw, sh);
                ctx.restore();
            }
        }
    }
    drawSparks(ctx, c, coralParticles, front, true);
}

// =========================================================================================
// ONDA DA ÂNCORA (corre rente ao chão até o rival)
// =========================================================================================
void JoanaFx::updateAnchor(LabContext& c) {
    auto& f = c.fighter;
    const bool isWave = f.state == "joana_anchor_slam";
    const float p = isWave ? c.progress() : 0.0f;
    if (isWave && p >= 0.35f && !anchorFired) {
        anchorFired = true;
        const float impactX = f.body.offsetX + 4, startZ = 20, endZ = std::max(80.0f, rivalDist(c) + 10.0f);
        const Vec3 wImpact = Physics3D::localToWorld(impactX, 0, 25, c.entity->worldPos, c.entity->facing);
        anchorCrater = Crater{wImpact.x, wImpact.z, 1.0f, 0.014f};
        for (int i = 0; i < 4; i++)
            waveEllipses.push_back({impactX, startZ, endZ, i * 0.12f, 0, 65.0f + i * 16, 20.0f + i * 5, 1.0f, 0.024f});
        for (int i = 0; i < 14; i++) {
            const float lat = -60 + (i / 13.0f) * 120 + (rnd() - 0.5f) * 6;
            const char* col = i % 3 == 0 ? "#ffffff" : (i % 3 == 1 ? "#a8dadc" : "#457b9d");
            speedLines.push_back({impactX + lat, startZ, endZ, 0, 1.0f + rnd() * 0.35f, 24 + rnd() * 16, 1.5f + rnd() * 1.6f, col, 1.0f,
                                  0.026f + rnd() * 0.01f});
        }
        for (int i = 0; i < 16; i++) {
            const float ang = (rnd() - 0.5f) * PI * 0.6f, spd = 2.5f + rnd() * 4;
            tsunamiDrops.push_back({impactX + (rnd() - 0.5f) * 30, 2 + rnd() * 6, 20, std::sin(ang) * spd * 0.5f, 0.8f + rnd() * 1.5f, spd * 1.1f,
                                    1.2f + rnd() * 1.6f, i % 2 == 0 ? "#ffffff" : "#a8dadc", 1.0f, 0.032f});
        }
    } else if (!isWave || p < 0.35f) {
        anchorFired = isWave && anchorFired && p >= 0.35f;
        if (!isWave) anchorFired = false;
    }
    if (c.isPaused) return;
    for (auto& e : waveEllipses) {
        if (e.delay > 0) e.delay -= 0.035f;
        else { e.progress += 0.045f; e.life -= e.decay; }
        if (e.progress >= 1.2f) e.life = 0;
    }
    for (auto& s : speedLines) { s.curT += 0.055f * s.speed; s.life -= s.decay; if (s.curT >= 1.2f) s.life = 0; }
    if (anchorCrater) { anchorCrater->life -= anchorCrater->decay; if (anchorCrater->life <= 0) anchorCrater.reset(); }
    for (auto& d : tsunamiDrops) { d.x += d.vx; d.y += d.vy; d.z += d.vz; d.vy -= 0.15f; d.life -= d.decay; if (d.y < 0) d.y = 0; }
    dropDead(waveEllipses); dropDead(speedLines); dropDead(tsunamiDrops);
}

void JoanaFx::drawAnchorFloor(Canvas& ctx, LabContext& c) {
    if (anchorCrater && anchorCrater->life > 0.01f) {
        Point3 bp = c.projectWorldFloor(anchorCrater->x, anchorCrater->z);
        if (bp.ok()) {
            if (auto img = c.sprites->find("joanaAnchorImpactCraterSprite")) {
                ctx.save();
                ctx.globalAlpha = anchorCrater->life * 0.9f;
                const float cw = 70 * bp.scale * 1.4f, ch = 28 * bp.scale * 1.4f;
                ctx.drawImage(*img, bp.x - cw / 2, bp.y - ch / 2, cw, ch);
                ctx.restore();
            }
        }
    }
    for (const SpeedLine& sl : speedLines) {
        if (sl.curT <= 0 || sl.curT >= 1.0f) continue;
        const float t = std::min(1.0f, sl.curT), z = sl.zStart + (sl.zEnd - sl.zStart) * t;
        Point3 p1 = c.project3D(sl.x, 0, z), p2 = c.project3D(sl.x, 0, z + sl.lenZ);
        if (!p1.ok() || !p2.ok()) continue;
        ctx.save();
        ctx.globalAlpha = std::max(0.0f, sl.life * std::sin(t * PI) * 0.95f);
        ctx.lineCap = "round";
        ctx.beginPath(); ctx.moveTo(p1.x, p1.y); ctx.lineTo(p2.x, p2.y);
        ctx.strokeStyle = kInk;
        ctx.lineWidth = std::max(1.8f, (sl.width + 1.2f) * p1.scale);
        ctx.stroke();
        ctx.beginPath(); ctx.moveTo(p1.x, p1.y); ctx.lineTo(p2.x, p2.y);
        ctx.strokeStyle = sl.color;
        ctx.lineWidth = std::max(1.0f, sl.width * p1.scale);
        ctx.stroke();
        ctx.beginPath();
        ctx.arc(p2.x, p2.y, std::max(1.0f, sl.width * 0.9f * p2.scale), 0, PI * 2);
        ctx.fillStyle = "#ffffff";
        ctx.fill();
        ctx.restore();
    }
    for (const WaveEllipse& el : waveEllipses) {
        if (el.delay > 0 || el.progress <= 0) continue;
        const float t = std::min(1.0f, el.progress), z = el.zStart + (el.zEnd - el.zStart) * t;
        Point3 bp = c.project3D(el.x, 0, z);
        if (!bp.ok()) continue;
        ctx.save();
        ctx.globalAlpha = std::max(0.0f, el.life * std::sin(t * PI * 0.9f) * 0.9f);
        const float ex = std::sin(t * PI * 0.5f);
        const float rx = el.maxRx * (0.35f + 0.65f * ex) * bp.scale, ry = el.maxRy * (0.35f + 0.65f * ex) * bp.scale;
        ctx.beginPath();
        ctx.ellipse(bp.x, bp.y, rx, ry, 0, 0, PI * 2);
        ctx.fillStyle = "rgba(42, 157, 143, 0.35)";
        ctx.strokeStyle = kInk;
        ctx.lineWidth = 2.0f;
        ctx.fill(); ctx.stroke();
        ctx.beginPath();
        ctx.ellipse(bp.x, bp.y, rx * 0.92f, ry * 0.92f, 0, 0, PI * 2);
        ctx.strokeStyle = "#ffffff";
        ctx.lineWidth = std::max(1.8f, 3.0f * el.life);
        ctx.stroke();
        ctx.beginPath();
        ctx.ellipse(bp.x, bp.y, rx * 0.55f, ry * 0.55f, 0, 0, PI * 2);
        ctx.strokeStyle = "#a8dadc";
        ctx.lineWidth = std::max(1.0f, 1.8f * el.life);
        ctx.stroke();
        for (float a = -0.8f; a <= 0.8f; a += 0.35f) {
            const float ang = PI * 0.5f + a, ca = std::cos(ang), sa = std::sin(ang);
            ctx.beginPath();
            ctx.moveTo(bp.x + ca * rx * 0.75f, bp.y + sa * ry * 0.75f);
            ctx.lineTo(bp.x + ca * rx * 1.15f, bp.y + sa * ry * 1.15f);
            ctx.strokeStyle = "#ffffff";
            ctx.lineWidth = 1.6f;
            ctx.stroke();
        }
        ctx.restore();
    }
    for (const Spark& d : tsunamiDrops) {
        Point3 bp = c.project3D(d.x, d.y, d.z);
        if (!bp.ok()) continue;
        ctx.save();
        ctx.globalAlpha = d.life;
        ctx.fillStyle = d.color;
        ctx.strokeStyle = kInk;
        ctx.lineWidth = 0.5f;
        ctx.beginPath();
        ctx.arc(bp.x, bp.y, std::max(0.6f, d.r * d.life * bp.scale), 0, PI * 2);
        ctx.fill(); ctx.stroke();
        ctx.restore();
    }
}

// =========================================================================================
// BÁLSAMO DE ANÊMONA
// =========================================================================================
void JoanaFx::updateAnemone(LabContext& c) {
    auto& f = c.fighter;
    const bool isBalm = f.state == "joana_ocean_bounty";
    const float p = isBalm ? c.progress() : 0.0f;
    const float bX = f.body.offsetX, bY = f.body.offsetY;
    if (isBalm) {
        if (p < 0.20f) anemoneGrowth = std::sin((p / 0.20f) * PI * 0.5f);
        else if (p < 0.72f) anemoneGrowth = 1.0f;
        else if (p < 0.94f) anemoneGrowth = std::cos((p - 0.72f) / 0.22f * PI * 0.5f);
        else anemoneGrowth = 0;
        if (!c.isPaused) {
            if (p >= 0.20f && p < 0.75f) absorptionGlow = std::min(1.0f, absorptionGlow + 0.05f);
            else if (p >= 0.75f) absorptionGlow = std::max(0.0f, absorptionGlow - 0.03f);
            if (p >= 0.15f && p <= 0.68f && rnd() < 0.22f) groundWaves.push_back({bX, 0, 8, 85, 1.0f, 0.018f, 1.7f});
            if (p >= 0.20f && p <= 0.70f && rnd() < 0.18f) auraRings.push_back({bX + (rnd() - 0.5f) * 10, bY + 45, 0, 0.4f, 1.0f, 0.028f});
            if (p >= 0.18f && p <= 0.72f && rnd() < 0.35f) {
                const bool left = rnd() < 0.5f;
                bioDroplets.push_back({bX + (left ? -16 : 14) + (rnd() - 0.5f) * 6, bY + 68, -14, -0.8f - rnd() * 1.2f, 2.5f + rnd() * 2, 1.0f, 0.024f});
            }
            if (p >= 0.12f && p <= 0.85f && rnd() < 0.55f) {
                const float ang = rnd() * PI * 2;
                spores.push_back({bX + std::cos(ang) * (15 + rnd() * 20), bY + 15 + rnd() * 45, std::sin(ang) * 15, std::cos(ang) * 0.4f,
                                  1.2f + rnd() * 1.8f, 0, 1.6f + rnd() * 2.4f, rnd() < 0.5f ? "#4cc9f0" : "#f72585", 1.0f, 0.028f});
            }
        }
    } else {
        anemoneGrowth = 0;
        if (!c.isPaused) absorptionGlow = std::max(0.0f, absorptionGlow - 0.04f);
    }
    if (c.isPaused) return;
    for (auto& g : groundWaves) { g.r += g.speed; g.life -= g.decay; if (g.r >= g.maxR) g.life = 0; }
    for (auto& r : auraRings) { r.y += 1.4f; r.scale += 0.022f; r.life -= r.decay; }
    for (auto& d : bioDroplets) { d.y += d.vy; d.life -= d.decay; }
    for (auto& s : spores) { s.x += s.vx; s.y += s.vy; s.life -= s.decay; }
    dropDead(groundWaves); dropDead(auraRings); dropDead(bioDroplets); dropDead(spores);
}

void JoanaFx::drawAnemoneGroundWaves(Canvas& ctx, LabContext& c) {
    for (const GroundWave& gw : groundWaves) {
        Point3 bp = c.project3D(gw.x, 0, gw.z);
        if (!bp.ok()) continue;
        ctx.save();
        ctx.globalAlpha = std::max(0.0f, gw.life * 0.85f);
        const float rx = gw.r * bp.scale * 1.8f, ry = rx * 0.32f;
        ctx.beginPath();
        ctx.ellipse(bp.x, bp.y, rx, ry, 0, 0, PI * 2);
        ctx.fillStyle = "rgba(76, 201, 240, 0.16)";
        ctx.strokeStyle = kInk;
        ctx.lineWidth = 1.8f;
        ctx.fill(); ctx.stroke();
        ctx.beginPath();
        ctx.ellipse(bp.x, bp.y, rx * 0.94f, ry * 0.94f, 0, 0, PI * 2);
        ctx.strokeStyle = "#4cc9f0";
        ctx.lineWidth = std::max(1.0f, 2.0f * gw.life);
        ctx.stroke();
        ctx.beginPath();
        ctx.ellipse(bp.x, bp.y, rx * 0.88f, ry * 0.88f, 0, 0, PI * 2);
        ctx.strokeStyle = "#ffffff";
        ctx.lineWidth = std::max(0.8f, 1.4f * gw.life);
        ctx.stroke();
        ctx.restore();
    }
}

void JoanaFx::drawCartoonAnemone(Canvas& ctx, float cx, float cy, float baseScale, float time, float seed, float growth) {
    if (growth <= 0.01f) return;
    ctx.save();
    ctx.translate(cx, cy);
    const float pulse = std::sin(time * 3 + seed);
    ctx.scale(baseScale * growth * (1 + pulse * 0.05f), baseScale * growth * (1 - pulse * 0.05f));
    const float curl = (1.0f - std::min(1.0f, growth)) * 0.45f;
    struct T { float angle, length, width, freq, phase; const char* color; };
    auto tentacles = [&](const std::vector<T>& list, float swayAmt, float midK, const char* bulb, float bulbBase, float bulbAmp, float bulbFreq) {
        for (const T& t : list) {
            const float sway = std::sin(time * t.freq + t.phase) * (swayAmt * growth);
            const float total = t.angle + sway - PI * 0.5f;
            const float tipX = std::cos(total) * t.length, tipY = std::sin(total) * t.length;
            const float midX = std::cos(total - sway * midK) * t.length * 0.5f, midY = std::sin(total - sway * midK) * t.length * 0.5f;
            ctx.beginPath();
            ctx.moveTo(0, -5);
            ctx.quadraticCurveTo(midX, midY, tipX, tipY);
            ctx.strokeStyle = kInk;
            ctx.lineWidth = std::max(1.2f, (t.width + 2.2f) * growth);
            ctx.lineCap = "round";
            ctx.stroke();
            ctx.strokeStyle = t.color;
            ctx.lineWidth = std::max(0.8f, t.width * growth);
            ctx.stroke();
            const float br = (bulbBase + std::sin(time * bulbFreq + t.phase) * bulbAmp) * growth;
            ctx.beginPath(); ctx.arc(tipX, tipY, br + 1.2f * growth, 0, PI * 2); ctx.fillStyle = kInk; ctx.fill();
            ctx.beginPath(); ctx.arc(tipX, tipY, std::max(0.8f, br), 0, PI * 2); ctx.fillStyle = bulb; ctx.fill();
            ctx.beginPath(); ctx.arc(tipX - 0.9f * growth, tipY - 0.9f * growth, std::max(0.4f, br * 0.45f), 0, PI * 2);
            ctx.fillStyle = "#ffffff"; ctx.fill();
        }
    };
    const float k = 1.0f - curl;
    tentacles({{-0.95f * k, 22 * growth, 3.6f, 2.4f, seed, "#560bad"}, {-0.55f * k, 28 * growth, 4.2f, 2.8f, seed + 1.2f, "#7209b7"},
               {0, 32 * growth, 4.6f, 3.2f, seed + 2.4f, "#7209b7"}, {0.55f * k, 28 * growth, 4.2f, 2.9f, seed + 3.6f, "#7209b7"},
               {0.95f * k, 22 * growth, 3.6f, 2.5f, seed + 4.8f, "#560bad"}},
              0.28f, 0.5f, "#4cc9f0", 3.6f, 0.5f, 3.5f);
    ctx.beginPath();
    ctx.moveTo(-12, 6);
    ctx.bezierCurveTo(-14, -4, -10, -12, 0, -12);
    ctx.bezierCurveTo(10, -12, 14, -4, 12, 6);
    ctx.quadraticCurveTo(0, 9, -12, 6);
    ctx.closePath();
    ctx.fillStyle = "#7209b7";
    ctx.strokeStyle = kInk;
    ctx.lineWidth = std::max(1.2f, 2.2f * growth);
    ctx.fill(); ctx.stroke();
    ctx.beginPath();
    ctx.ellipse(0, -5, 11, 5.5f, 0, 0, PI * 2);
    ctx.fillStyle = "#f72585";
    ctx.lineWidth = std::max(1.0f, 1.6f * growth);
    ctx.fill(); ctx.stroke();
    ctx.beginPath(); ctx.ellipse(0, -5, 6.8f, 3.0f, 0, 0, PI * 2); ctx.fillStyle = "#4cc9f0"; ctx.fill();
    ctx.beginPath(); ctx.ellipse(0, -5, 3.0f, 1.2f, 0, 0, PI * 2); ctx.fillStyle = "#ffffff"; ctx.fill();
    tentacles({{-0.80f * k, 18 * growth, 3.4f, 3.0f, seed + 0.8f, "#f72585"}, {-0.30f * k, 24 * growth, 4.0f, 3.4f, seed + 2.0f, "#f72585"},
               {0.30f * k, 24 * growth, 4.0f, 3.3f, seed + 3.2f, "#f72585"}, {0.80f * k, 18 * growth, 3.4f, 2.8f, seed + 4.4f, "#f72585"}},
              0.32f, 0.6f, "#a8dadc", 3.4f, 0.4f, 4.0f);
    ctx.restore();
}

void JoanaFx::drawAnemone(Canvas& ctx, LabContext& c, bool front) {
    auto& f = c.fighter;
    const float time = nowMs() * 0.006f;
    if (auto ring = joanaSprite(c,"joanaAnemoneAuraRingSprite")) {
        for (const AuraRing& rg : auraRings) {
            Point3 bp = c.project3D(rg.x, rg.y, rg.z);
            if (!bp.ok() || frontOf(c, bp) != front) continue;
            ctx.save();
            ctx.globalAlpha = rg.life * 0.85f;
            const float rw = 55 * rg.scale * bp.scale, rh = 20 * rg.scale * bp.scale;
            ctx.drawImage(*ring, bp.x - rw / 2, bp.y - rh / 2, rw, rh);
            ctx.restore();
        }
    }
    // As anêmonas brotam nas costas e ombros (Z negativo: lado oposto ao rival).
    Point3 back = c.project3D(f.body.offsetX, f.body.offsetY + 60, -14);
    const bool clusterFront = back.ok() && frontOf(c, back);
    if (!c.liveBody && clusterFront == front && anemoneGrowth > 0.01f) {
        const float bX = f.body.offsetX, bY = f.body.offsetY;
        struct Cl { float x, y, z, scale, seed; };
        const Cl clusters[] = {{bX - 18, bY + 66, -14, 0.65f, 0.0f}, {bX + 18, bY + 66, -14, 0.65f, 2.5f}, {bX, bY + 42, -16, 0.72f, 4.8f}};
        for (const Cl& cl : clusters) {
            Point3 bp = c.project3D(cl.x, cl.y, cl.z);
            if (!bp.ok()) continue;
            ctx.save();
            ctx.globalAlpha = std::min(1.0f, anemoneGrowth * 1.25f);
            const float bob = std::sin(time * 3 + cl.seed) * 1.8f * anemoneGrowth;
            drawCartoonAnemone(ctx, bp.x, bp.y + bob, cl.scale * bp.scale, time, cl.seed, anemoneGrowth);
            ctx.restore();
        }
        if (auto drop = c.sprites->find("joanaAnemoneGooDropletSprite")) {
            for (const Drop& dr : bioDroplets) {
                Point3 bp = c.project3D(dr.x, dr.y, dr.z);
                if (!bp.ok()) continue;
                ctx.save();
                ctx.globalAlpha = std::max(0.0f, dr.life * std::min(1.0f, anemoneGrowth * 1.5f) * 0.9f);
                const float curR = dr.r * std::max(0.4f, std::min(1.0f, anemoneGrowth));
                const float dw = 18 * curR * 0.35f * bp.scale, dh = 24 * curR * 0.35f * bp.scale;
                ctx.drawImage(*drop, bp.x - dw / 2, bp.y - dh / 2, dw, dh);
                ctx.restore();
            }
        }
    }
    // Melhoria: halo de cura verde-água pulsando no peito enquanto absorve o bálsamo.
    if (front && absorptionGlow > 0.01f) {
        Point3 chest = c.project3D(f.body.offsetX, f.body.offsetY + 60, 0);
        if (chest.ok()) {
            ctx.save();
            ctx.globalCompositeOperation = "lighter";
            const float r = (48 + std::sin(time * 4) * 6) * chest.scale;
            auto grad = ctx.createRadialGradient(chest.x, chest.y, 0, chest.x, chest.y, r);
            grad->addColorStop(0, cv::rgba(76, 201, 240, 0.35f * absorptionGlow).color);
            grad->addColorStop(1, cv::rgba(76, 201, 240, 0).color);
            ctx.fillStyle = grad;
            ctx.beginPath();
            ctx.arc(chest.x, chest.y, r, 0, PI * 2);
            ctx.fill();
            ctx.restore();
        }
    }
    for (const Spark& sp : spores) {
        Point3 bp = c.project3D(sp.x, sp.y, sp.z);
        if (!bp.ok() || frontOf(c, bp) != front) continue;
        ctx.save();
        ctx.globalAlpha = sp.life * 0.9f;
        ctx.fillStyle = sp.color;
        ctx.strokeStyle = kInk;
        ctx.lineWidth = 0.5f;
        ctx.beginPath();
        ctx.arc(bp.x, bp.y, std::max(0.8f, sp.r * sp.life * bp.scale), 0, PI * 2);
        ctx.fill(); ctx.stroke();
        ctx.fillStyle = "#ffffff";
        ctx.beginPath();
        ctx.arc(bp.x - 0.3f, bp.y - 0.3f, std::max(0.4f, sp.r * 0.4f * sp.life * bp.scale), 0, PI * 2);
        ctx.fill();
        ctx.restore();
    }
}

// =========================================================================================
// PASSIVA: ARMAMENTO ASSIMÉTRICO (gancho com alcance, garra com impacto pesado)
// =========================================================================================
void JoanaFx::triggerAsymmetric(LabContext& c, bool hookSide) {
    asymmetricTimer = 90;
    const float bX = c.fighter.body.offsetX, bY = c.fighter.body.offsetY;
    if (hookSide) {
        for (int i = 0; i < 5; i++) reachRings.push_back({bX - 28, bY + 78, 22.0f + i * 24, 8.0f + i * 5, 1.0f, i * 3, 0.024f});
        reachLines.push_back({bX - 22, bY + 70, 18, bX - 28, bY + 78, 110, 1.0f, 0.022f});
    } else {
        crushShockwaves.push_back({bX + 24, bY + 84, 12, 8, 52, 1.0f, 0.032f});
        static const char* cols[] = {"#f59e0b", "#ff8533", "#ffffff"};
        for (int i = 0; i < 24; i++) {
            const float ang = rnd() * PI * 2, spd = 4.0f + rnd() * 7.5f;
            heavySparks.push_back({bX + 24 + (rnd() - 0.5f) * 10, bY + 84 + (rnd() - 0.5f) * 10, 12, std::cos(ang) * spd, std::sin(ang) * spd,
                                   (rnd() - 0.5f) * 3, 2.2f + rnd() * 3.2f, cols[i % 3], 1.0f, 0.032f});
        }
    }
}

void JoanaFx::updateAsymmetric() {
    if (asymmetricTimer > 0) asymmetricTimer--;
    for (auto& r : reachRings) {
        if (r.delay > 0) r.delay--;
        else { r.z += 1.8f; r.radius += 0.45f; r.life -= r.decay; }
    }
    for (auto& l : reachLines) l.life -= l.decay;
    for (auto& s : crushShockwaves) { s.r += (s.maxR - s.r) * 0.16f; s.life -= s.decay; }
    for (auto& s : heavySparks) { s.x += s.vx; s.y += s.vy; s.z += s.vz; s.life -= s.decay; }
    dropDead(reachRings); dropDead(reachLines); dropDead(crushShockwaves); dropDead(heavySparks);
}

void JoanaFx::drawAsymmetric(Canvas& ctx, LabContext& c, bool front) {
    for (const ReachLine& ln : reachLines) {
        Point3 p1 = c.project3D(ln.x1, ln.y1, ln.z1), p2 = c.project3D(ln.x2, ln.y2, ln.z2);
        if (!p1.ok() || !p2.ok() || frontOf(c, p2) != front) continue;
        ctx.save();
        ctx.globalAlpha = ln.life * 0.9f;
        ctx.lineCap = "round";
        ctx.beginPath(); ctx.moveTo(p1.x, p1.y); ctx.lineTo(p2.x, p2.y);
        ctx.strokeStyle = kInk; ctx.lineWidth = 3.8f; ctx.stroke();
        ctx.beginPath(); ctx.moveTo(p1.x, p1.y); ctx.lineTo(p2.x, p2.y);
        ctx.strokeStyle = "#00f5d4"; ctx.lineWidth = 2.2f; ctx.stroke();
        ctx.restore();
    }
    for (const ReachRing& rg : reachRings) {
        if (rg.delay > 0) continue;
        Point3 bp = c.project3D(rg.x, rg.y, rg.z);
        if (!bp.ok() || frontOf(c, bp) != front) continue;
        ctx.save();
        ctx.globalAlpha = rg.life * 0.85f;
        const float r = rg.radius * bp.scale;
        ctx.beginPath(); ctx.ellipse(bp.x, bp.y, r, r * 0.45f, 0, 0, PI * 2);
        ctx.strokeStyle = kInk; ctx.lineWidth = 2.4f; ctx.stroke();
        ctx.beginPath(); ctx.ellipse(bp.x, bp.y, r, r * 0.45f, 0, 0, PI * 2);
        ctx.strokeStyle = "#4cc9f0"; ctx.lineWidth = 1.6f; ctx.stroke();
        ctx.restore();
    }
    for (const Ring& sw : crushShockwaves) {
        Point3 bp = c.project3D(sw.x, sw.y, sw.z);
        if (!bp.ok() || frontOf(c, bp) != front) continue;
        ctx.save();
        ctx.globalAlpha = sw.life * 0.9f;
        const float r = sw.r * bp.scale;
        ctx.beginPath(); ctx.arc(bp.x, bp.y, r, 0, PI * 2); ctx.strokeStyle = kInk; ctx.lineWidth = 3.2f; ctx.stroke();
        ctx.beginPath(); ctx.arc(bp.x, bp.y, r, 0, PI * 2); ctx.strokeStyle = "#ff8533"; ctx.lineWidth = 2.0f; ctx.stroke();
        ctx.restore();
    }
    for (const Spark& sp : heavySparks) {
        Point3 bp = c.project3D(sp.x, sp.y, sp.z);
        if (!bp.ok() || frontOf(c, bp) != front) continue;
        ctx.save();
        ctx.globalAlpha = sp.life;
        ctx.fillStyle = sp.color;
        ctx.beginPath();
        ctx.arc(bp.x, bp.y, std::max(1.0f, sp.r * bp.scale * sp.life), 0, PI * 2);
        ctx.fill();
        ctx.restore();
    }
}

// =========================================================================================
// PASSIVA: CARAPAÇA CALCÁRIA (arcos de deflexão e cascalho ao absorver golpes)
// =========================================================================================
void JoanaFx::triggerCalcified(LabContext& c) {
    calcifiedTimer = 90;
    const float bX = c.fighter.body.offsetX, bY = c.fighter.body.offsetY;
    deflectionArcs.push_back({bX, bY + 70, 22, 16, 68, 1.0f, 0.03f});
    static const char* cols[] = {"#e2e8f0", "#cbd5e1", "#94a3b8"};
    for (int i = 0; i < 26; i++) {
        const bool left = i % 2 == 0;
        calcDebris.push_back({bX + (left ? -22 : 22) + (rnd() - 0.5f) * 14, bY + 72 + (rnd() - 0.5f) * 12, 16 + (rnd() - 0.5f) * 8,
                              (left ? -1.0f : 1.0f) * (1.2f + rnd() * 2.8f), 1.2f + rnd() * 2.2f, 0, 2.2f + rnd() * 3.8f, rnd() * PI * 2,
                              (rnd() - 0.5f) * 0.25f, cols[i % 3], 1.0f, 0.024f});
    }
}

void JoanaFx::updateCalcified() {
    if (calcifiedTimer > 0) calcifiedTimer--;
    for (auto& a : deflectionArcs) { a.r += (a.maxR - a.r) * 0.16f; a.life -= a.decay; }
    for (auto& d : calcDebris) {
        d.x += d.vx; d.y += d.vy; d.vy -= 0.28f; d.rot += d.rotSpeed; d.life -= d.decay;
        if (d.y < 0) { d.y = 0; d.vy = 0; d.vx *= 0.6f; d.rotSpeed *= 0.5f; } // pousa no tablado
    }
    dropDead(deflectionArcs); dropDead(calcDebris);
}

void JoanaFx::drawCalcified(Canvas& ctx, LabContext& c, bool front) {
    for (const Ring& a : deflectionArcs) {
        Point3 bp = c.project3D(a.x, a.y, a.z);
        if (!bp.ok() || frontOf(c, bp) != front) continue;
        ctx.save();
        ctx.globalAlpha = a.life * 0.9f;
        const float r = a.r * bp.scale;
        ctx.lineCap = "round";
        ctx.beginPath(); ctx.arc(bp.x, bp.y, r, -PI * 0.75f, PI * 0.75f); ctx.strokeStyle = "#0f172a"; ctx.lineWidth = 4.0f; ctx.stroke();
        ctx.beginPath(); ctx.arc(bp.x, bp.y, r, -PI * 0.75f, PI * 0.75f); ctx.strokeStyle = "#e2e8f0"; ctx.lineWidth = 2.4f; ctx.stroke();
        ctx.beginPath(); ctx.arc(bp.x, bp.y, r * 0.9f, -PI * 0.65f, PI * 0.65f); ctx.strokeStyle = "#ffffff"; ctx.lineWidth = 1.5f; ctx.stroke();
        ctx.restore();
    }
    for (const Debris& d : calcDebris) {
        Point3 bp = c.project3D(d.x, d.y, d.z);
        if (!bp.ok() || frontOf(c, bp) != front) continue;
        ctx.save();
        ctx.translate(bp.x, bp.y);
        ctx.rotate(d.rot);
        ctx.globalAlpha = d.life;
        const float r = d.r * bp.scale;
        ctx.fillStyle = d.color;
        ctx.strokeStyle = "#0f172a";
        ctx.lineWidth = 1.0f;
        ctx.beginPath();
        ctx.rect(-r / 2, -r / 2, r, r);
        ctx.fill(); ctx.stroke();
        ctx.restore();
    }
}

// =========================================================================================
// PASSIVA: MARÉ DAS PROFUNDEZAS (metamorfose de siri com vida baixa)
// =========================================================================================
void JoanaFx::triggerDeepSeaTide(LabContext& c) {
    const float bX = c.fighter.body.offsetX, bY = c.fighter.body.offsetY;
    bandanas.push_back({bX, bY + 95, -16, (rnd() > 0.5f ? 1.0f : -1.0f) * (3.5f + rnd() * 3), 6.0f + rnd() * 3.5f, -4.5f - rnd() * 4, 0, 0.18f, 0,
                        1.0f, 0.014f});
    static const char* cols[] = {"#ff8533", "#ea580c", "#fde047"};
    for (int i = 0; i < 36; i++) {
        const float ang = rnd() * PI * 2, spd = 4.5f + rnd() * 9.5f;
        brutalDebris.push_back({bX + (rnd() - 0.5f) * 20, bY + 65 + (rnd() - 0.5f) * 30, (rnd() - 0.5f) * 12, std::cos(ang) * spd,
                                std::sin(ang) * spd + 2.5f, (rnd() - 0.5f) * 5, 2.5f + rnd() * 4.5f, rnd() * PI * 2, (rnd() - 0.5f) * 0.35f, cols[i % 3],
                                1.0f, 0.024f + rnd() * 0.018f});
    }
    // Melhoria: anel de choque abissal na transformação.
    crushShockwaves.push_back({bX, bY + 65, 0, 20, 140, 1.0f, 0.025f});
}

void JoanaFx::updateDeepSeaTide(LabContext& c) {
    const bool monstrous = c.entity && c.entity->monstrous;
    if (monstrous && !wasMonstrous) triggerDeepSeaTide(c);
    wasMonstrous = monstrous;
    if (monstrous) mutation = std::min(1.0f, mutation + 0.10f);
    else mutation = std::max(0.0f, mutation - 0.04f);
    for (auto& d : brutalDebris) { d.x += d.vx; d.y += d.vy; d.vy -= 0.28f; d.z += d.vz; d.rot += d.rotSpeed; d.life -= d.decay; }
    for (auto& b : bandanas) {
        b.x += b.vx; b.y += b.vy; b.vy -= 0.16f; b.z += b.vz; b.rot += b.rotSpeed; b.wavePhase += 0.24f; b.life -= b.decay;
        if (b.y < 2) { b.y = 2; b.vx *= 0.8f; b.vz *= 0.8f; b.vy = 0; b.rotSpeed *= 0.7f; }
    }
    dropDead(brutalDebris); dropDead(bandanas);
}

void JoanaFx::drawBodyOverlay(Canvas& ctx, LabContext& c, float bW, float bH) {
    if (mutation <= 0.01f) return;
    auto& f = c.fighter;
    Point3 base = c.project3D(f.body.offsetX, c.bodyLift + f.body.offsetY, 0);
    if (!base.ok()) return;
    const bool isBack = c.activeCamera == "back";
    const float time = nowMs() / 1000.0f, mut = mutation;
    ctx.save();
    ctx.translate(base.x, base.y);
    ctx.transform(1, 0, f.body.bodySkew, 1, 0, 0);
    ctx.scale(base.scale, base.scale);
    ctx.globalAlpha = mut;

    // 1. Patas de siri nas coxas.
    auto crabLeg = [&](float dir, float hipX, float hipY) {
        ctx.save();
        ctx.translate(hipX, hipY);
        ctx.scale(dir * mut, mut);
        ctx.strokeStyle = kInk;
        ctx.lineWidth = 2.4f;
        ctx.beginPath();
        ctx.moveTo(0, 0); ctx.lineTo(bW * 0.18f, -bH * 0.06f); ctx.lineTo(bW * 0.28f, bH * 0.16f); ctx.lineTo(bW * 0.14f, bH * 0.18f); ctx.lineTo(0, bH * 0.08f);
        ctx.closePath();
        ctx.fillStyle = "#ea580c"; ctx.fill(); ctx.stroke();
        ctx.beginPath();
        ctx.moveTo(bW * 0.18f, -bH * 0.06f); ctx.lineTo(bW * 0.25f, -bH * 0.14f); ctx.lineTo(bW * 0.22f, 0);
        ctx.closePath();
        ctx.fillStyle = "#ff8533"; ctx.fill(); ctx.stroke();
        ctx.beginPath();
        ctx.moveTo(bW * 0.28f, bH * 0.16f); ctx.lineTo(bW * 0.22f, bH * 0.38f); ctx.lineTo(bW * 0.12f, bH * 0.36f); ctx.lineTo(bW * 0.14f, bH * 0.18f);
        ctx.closePath();
        ctx.fillStyle = "#c2410c"; ctx.fill(); ctx.stroke();
        ctx.beginPath();
        ctx.moveTo(bW * 0.22f, bH * 0.38f); ctx.quadraticCurveTo(bW * 0.26f, bH * 0.52f, bW * 0.20f, bH * 0.62f); ctx.lineTo(bW * 0.14f, bH * 0.42f);
        ctx.closePath();
        ctx.fillStyle = "#ff8533"; ctx.fill(); ctx.stroke();
        ctx.beginPath();
        ctx.moveTo(bW * 0.20f, bH * 0.56f); ctx.lineTo(bW * 0.20f, bH * 0.62f); ctx.lineTo(bW * 0.16f, bH * 0.52f);
        ctx.closePath();
        ctx.fillStyle = "#fde047"; ctx.fill();
        ctx.restore();
    };
    crabLeg(-1, -bW * 0.16f, -bH * 0.24f);
    crabLeg(1, bW * 0.16f, -bH * 0.24f);

    // 2. Retalhos do colete estourado.
    const float flapL = std::sin(time * 6) * 1.5f, flapR = std::cos(time * 6) * 1.5f;
    for (int s = -1; s <= 1; s += 2) {
        const float flap = s < 0 ? flapL : flapR;
        ctx.fillStyle = "#1d3557"; ctx.strokeStyle = kInk; ctx.lineWidth = 1.8f;
        ctx.beginPath();
        ctx.moveTo(s * bW * 0.20f, -bH * 0.58f); ctx.lineTo(s * bW * 0.26f + flap, -bH * 0.44f); ctx.lineTo(s * bW * 0.22f + flap, -bH * 0.38f);
        ctx.lineTo(s * bW * 0.25f, -bH * 0.30f); ctx.lineTo(s * bW * 0.16f, -bH * 0.28f);
        ctx.closePath();
        ctx.fill(); ctx.stroke();
        ctx.strokeStyle = "#fde047"; ctx.lineWidth = 1.0f;
        ctx.beginPath(); ctx.moveTo(s * bW * 0.20f, -bH * 0.54f); ctx.lineTo(s * bW * 0.24f + flap, -bH * 0.42f); ctx.stroke();
    }

    // 3. Espigões de carapaça nas costas e ombros.
    auto spike = [&](float x, float y, float angle, float length, float width) {
        ctx.save();
        ctx.translate(x, y);
        ctx.rotate(angle);
        ctx.scale(mut, mut);
        ctx.beginPath(); ctx.moveTo(-width / 2, 0); ctx.lineTo(0, -length); ctx.lineTo(width / 2, 0); ctx.closePath();
        ctx.fillStyle = "#ea580c"; ctx.strokeStyle = kInk; ctx.lineWidth = 2.4f; ctx.fill(); ctx.stroke();
        ctx.beginPath(); ctx.moveTo(0, 0); ctx.lineTo(0, -length * 0.78f); ctx.strokeStyle = "#fde047"; ctx.lineWidth = 2.0f; ctx.stroke();
        ctx.beginPath(); ctx.moveTo(-width * 0.25f, -length * 0.65f); ctx.lineTo(0, -length); ctx.lineTo(width * 0.25f, -length * 0.65f); ctx.closePath();
        ctx.fillStyle = "#fed7aa"; ctx.fill();
        ctx.restore();
    };
    const float breathe = 1.0f + std::sin(time * 5) * 0.04f; // melhoria: os espigões "respiram"
    spike(-bW * 0.24f, -bH * 0.62f, -0.45f, bH * 0.30f * breathe, bW * 0.13f);
    spike(bW * 0.24f, -bH * 0.62f, 0.45f, bH * 0.30f * breathe, bW * 0.13f);
    spike(-bW * 0.14f, -bH * 0.68f, -0.20f, bH * 0.36f * breathe, bW * 0.15f);
    spike(bW * 0.14f, -bH * 0.68f, 0.20f, bH * 0.36f * breathe, bW * 0.15f);

    // 4. Pata extra de siri na costela direita, estalando a pinça.
    {
        ctx.save();
        const float pataBreath = std::sin(time * 6.5f) * 0.18f, snap = std::sin(time * 11) * 0.35f + 0.2f;
        ctx.translate(bW * 0.28f, -bH * 0.44f);
        ctx.scale(mut, mut);
        ctx.beginPath(); ctx.arc(0, 0, bW * 0.085f, 0, PI * 2);
        ctx.fillStyle = "#c2410c"; ctx.strokeStyle = kInk; ctx.lineWidth = 2.4f; ctx.fill(); ctx.stroke();
        ctx.rotate(0.4f + pataBreath);
        ctx.beginPath();
        ctx.moveTo(-bW * 0.04f, 0); ctx.lineTo(bW * 0.25f, -bH * 0.06f); ctx.lineTo(bW * 0.23f, bH * 0.08f); ctx.lineTo(-bW * 0.04f, bH * 0.06f);
        ctx.closePath();
        ctx.fillStyle = "#ea580c"; ctx.lineWidth = 2.2f; ctx.fill(); ctx.stroke();
        ctx.translate(bW * 0.23f, 0);
        ctx.rotate(-0.35f + pataBreath * 0.8f);
        ctx.beginPath(); ctx.ellipse(bW * 0.08f, 0, bW * 0.11f, bH * 0.085f, 0, 0, PI * 2);
        ctx.fillStyle = "#ff8533"; ctx.fill(); ctx.stroke();
        for (int jaw = -1; jaw <= 1; jaw += 2) {
            ctx.save();
            ctx.translate(bW * 0.14f, jaw * bH * 0.03f);
            ctx.rotate(jaw < 0 ? -snap : snap * 0.6f);
            ctx.beginPath();
            ctx.moveTo(0, 0); ctx.quadraticCurveTo(bW * 0.15f, jaw * bH * 0.09f, bW * 0.20f, 0); ctx.lineTo(bW * 0.10f, -jaw * bH * 0.02f);
            ctx.closePath();
            ctx.fillStyle = "#e65c00"; ctx.strokeStyle = kInk; ctx.lineWidth = 2.0f; ctx.fill(); ctx.stroke();
            ctx.fillStyle = "#ffffff";
            ctx.beginPath(); ctx.arc(bW * 0.06f, 0, 2.0f, 0, PI * 2); ctx.fill();
            ctx.beginPath(); ctx.arc(bW * 0.12f, jaw * 1.0f, 2.0f, 0, PI * 2); ctx.fill();
            ctx.restore();
        }
        ctx.restore();
    }

    // 5. Cabeça: dreads e o olho de antena gigante.
    const float headY = -bH * 0.82f;
    ctx.beginPath();
    ctx.ellipse(0, headY - bH * 0.02f, bW * 0.32f, bH * 0.18f, 0, 0, PI * 2);
    ctx.fillStyle = "#1c1917";
    ctx.fill();
    const float wind = std::sin(time * 7.5f);
    auto dread = [&](float dx, float dy, float length, float angle, float width) {
        ctx.save();
        ctx.translate(dx, dy);
        ctx.rotate(angle + wind * 0.12f);
        ctx.beginPath();
        ctx.moveTo(-width / 2, 0);
        ctx.quadraticCurveTo(width * 0.8f + wind * 2.5f, length * 0.5f, 0, length);
        ctx.quadraticCurveTo(-width * 0.8f + wind * 2.5f, length * 0.5f, width / 2, 0);
        ctx.closePath();
        ctx.fillStyle = "#1c1917"; ctx.strokeStyle = kInk; ctx.lineWidth = 1.8f; ctx.fill(); ctx.stroke();
        ctx.fillStyle = "#fde047";
        ctx.fillRect(-width * 0.4f, length * 0.4f, width * 0.8f, 3.0f);
        ctx.restore();
    };
    dread(-bW * 0.24f, headY - bH * 0.08f, bH * 0.24f, -0.70f , bW * 0.09f);
    dread(-bW * 0.15f, headY - bH * 0.12f, bH * 0.28f, -0.40f , bW * 0.09f);
    dread(-bW * 0.05f, headY - bH * 0.14f, bH * 0.30f, -0.12f , bW * 0.10f);
    dread(bW * 0.05f, headY - bH * 0.14f, bH * 0.30f, 0.12f , bW * 0.10f);
    dread(bW * 0.15f, headY - bH * 0.12f, bH * 0.28f, 0.40f , bW * 0.09f);
    dread(bW * 0.24f, headY - bH * 0.08f, bH * 0.24f, 0.70f , bW * 0.09f);
    dread(-bW * 0.30f, headY + bH * 0.04f, bH * 0.34f, -0.85f, bW * 0.10f);
    dread(bW * 0.30f, headY + bH * 0.04f, bH * 0.34f, 0.85f, bW * 0.10f);

    ctx.save();
    ctx.translate(bW * 0.16f, headY - bH * 0.06f);
    ctx.rotate(0.38f + std::sin(time * 6) * 0.08f);
    ctx.beginPath();
    ctx.moveTo(-bW * 0.025f, 0); ctx.lineTo(-bW * 0.02f, -bH * 0.18f); ctx.lineTo(bW * 0.02f, -bH * 0.18f); ctx.lineTo(bW * 0.025f, 0);
    ctx.closePath();
    ctx.fillStyle = "#ea580c"; ctx.strokeStyle = kInk; ctx.lineWidth = 2.0f; ctx.fill(); ctx.stroke();
    ctx.beginPath();
    ctx.moveTo(bW * 0.02f, -bH * 0.08f); ctx.lineTo(bW * 0.06f, -bH * 0.12f); ctx.lineTo(bW * 0.02f, -bH * 0.14f);
    ctx.closePath();
    ctx.fillStyle = "#ff8533"; ctx.fill(); ctx.stroke();
    ctx.translate(0, -bH * 0.20f);
    ctx.rotate(0.18f);
    const float erx = bW * 0.095f, ery = bH * 0.055f;
    if (isBack) {
        ctx.beginPath(); ctx.ellipse(0, 0, erx, ery, 0, 0, PI * 2);
        ctx.fillStyle = "#ea580c"; ctx.lineWidth = 2.4f; ctx.fill(); ctx.stroke();
        ctx.beginPath(); ctx.ellipse(0, 0, erx * 0.7f, ery * 0.7f, 0, 0, PI * 2);
        ctx.fillStyle = "#ff8533"; ctx.lineWidth = 1.6f; ctx.fill(); ctx.stroke();
        ctx.beginPath(); ctx.arc(0, 0, erx * 0.25f, 0, PI * 2); ctx.fillStyle = "#c2410c"; ctx.fill();
    } else {
        ctx.beginPath(); ctx.ellipse(0, 0, erx, ery, 0, 0, PI * 2);
        ctx.fillStyle = "#ff8533"; ctx.lineWidth = 2.4f; ctx.fill(); ctx.stroke();
        ctx.beginPath(); ctx.ellipse(0, 0, erx * 0.72f, ery * 0.72f, 0, 0, PI * 2); ctx.fillStyle = "#fde047"; ctx.fill();
        const float pupil = std::sin(time * 8) * 0.15f + 0.85f;
        ctx.beginPath(); ctx.ellipse(0, 0, erx * 0.22f * pupil, ery * 0.9f, 0, 0, PI * 2); ctx.fillStyle = kInk; ctx.fill();
        ctx.fillStyle = "#ffffff";
        ctx.beginPath(); ctx.arc(-erx * 0.35f, -ery * 0.35f, std::max(1.5f, erx * 0.22f), 0, PI * 2); ctx.fill();
    }
    ctx.restore();

    if (!isBack) {
        ctx.save();
        ctx.globalAlpha = mut * (std::sin(time * 9) * 0.3f + 0.7f);
        ctx.shadowBlur = 10;
        ctx.shadowColor = "#00f5d4";
        for (int s = -1; s <= 1; s += 2) {
            ctx.fillStyle = "#00f5d4";
            ctx.beginPath(); ctx.arc(s * bW * 0.08f, headY + bH * 0.01f, bW * 0.035f, 0, PI * 2); ctx.fill();
            ctx.fillStyle = "#ffffff";
            ctx.beginPath(); ctx.arc(s * bW * 0.08f, headY + bH * 0.01f, bW * 0.018f, 0, PI * 2); ctx.fill();
        }
        ctx.restore();
    }
    ctx.restore();
}

void JoanaFx::drawDeepSeaTide(Canvas& ctx, LabContext& c, bool front) {
    auto& f = c.fighter;
    const float time = nowMs() / 1000.0f, mut = mutation;
    // Cauda articulada saindo da lombar (do lado das costas: Z negativo).
    Point3 bPos = c.project3D(f.body.offsetX, c.bodyLift + f.body.offsetY + 34, -12);
    if (mut > 0.01f && bPos.ok() && frontOf(c, bPos) == front) {
        const float bW = c.bodyW * bPos.scale, bH = c.bodyH * bPos.scale;
        const float sway = std::sin(time * 5.2f) * 0.18f;
        ctx.save();
        ctx.translate(bPos.x, bPos.y);
        ctx.globalAlpha = mut;
        ctx.translate(-bW * 0.18f, -bH * 0.18f);
        ctx.rotate(-0.85f + sway);
        struct Node { float x, y, w, h, rot; };
        std::vector<Node> nodes;
        float sx = 0, sy = 0, sw = bW * 0.22f, sh = bH * 0.09f;
        for (int s = 0; s < 5; s++) {
            nodes.push_back({sx, sy, sw, sh, -0.22f + sway * (s * 0.15f)});
            sx -= sw * 0.45f; sy += sh * 0.85f; sw *= 0.88f; sh *= 0.92f;
        }
        ctx.beginPath();
        ctx.moveTo(nodes[0].x - nodes[0].w * 0.45f, nodes[0].y);
        for (std::size_t s = 1; s < nodes.size(); s++) ctx.lineTo(nodes[s].x - nodes[s].w * 0.45f, nodes[s].y);
        ctx.lineTo(sx, sy);
        for (int s = static_cast<int>(nodes.size()) - 1; s >= 0; s--) ctx.lineTo(nodes[s].x + nodes[s].w * 0.45f, nodes[s].y);
        ctx.closePath();
        ctx.fillStyle = "#c2410c"; ctx.strokeStyle = kInk; ctx.lineWidth = 3.2f; ctx.lineJoin = "round";
        ctx.fill(); ctx.stroke();
        for (std::size_t s = 0; s < nodes.size(); s++) {
            const Node& n = nodes[s];
            ctx.save();
            ctx.translate(n.x, n.y);
            ctx.rotate(n.rot);
            ctx.beginPath(); ctx.ellipse(0, 0, n.w / 2, n.h / 2, 0, 0, PI * 2);
            ctx.fillStyle = s % 2 == 0 ? "#ea580c" : "#ff8533"; ctx.strokeStyle = kInk; ctx.lineWidth = 2.2f; ctx.fill(); ctx.stroke();
            for (int d = -1; d <= 1; d += 2) {
                ctx.beginPath();
                ctx.moveTo(d * n.w / 2, 0); ctx.lineTo(d * n.w * 0.78f, -n.h * 0.5f); ctx.lineTo(d * n.w * 0.45f, n.h * 0.2f);
                ctx.closePath();
                ctx.fillStyle = "#c2410c"; ctx.fill(); ctx.stroke();
            }
            ctx.restore();
        }
        ctx.save();
        ctx.translate(sx, sy);
        ctx.rotate(sway * 2.2f);
        for (int d = -1; d <= 1; d += 2) {
            ctx.beginPath(); ctx.ellipse(d * bW * 0.11f, bH * 0.05f, bW * 0.09f, bH * 0.045f, d * 0.55f, 0, PI * 2);
            ctx.fillStyle = "#ff8533"; ctx.strokeStyle = kInk; ctx.lineWidth = 2.0f; ctx.fill(); ctx.stroke();
        }
        ctx.beginPath(); ctx.moveTo(-bW * 0.07f, 0); ctx.lineTo(0, bH * 0.12f); ctx.lineTo(bW * 0.07f, 0); ctx.closePath();
        ctx.fillStyle = "#ea580c"; ctx.fill(); ctx.stroke();
        ctx.fillStyle = "#fde047";
        ctx.beginPath(); ctx.arc(0, bH * 0.09f, 2.5f, 0, PI * 2); ctx.fill();
        ctx.restore();
        ctx.restore();
    }
    for (const Debris& d : brutalDebris) {
        Point3 bp = c.project3D(d.x, d.y, d.z);
        if (!bp.ok() || frontOf(c, bp) != front) continue;
        ctx.save();
        ctx.translate(bp.x, bp.y);
        ctx.rotate(d.rot);
        ctx.globalAlpha = d.life;
        const float r = d.r * bp.scale;
        ctx.fillStyle = d.color; ctx.strokeStyle = kInk; ctx.lineWidth = 1.0f;
        ctx.beginPath(); ctx.moveTo(-r, -r); ctx.lineTo(r, 0); ctx.lineTo(0, r); ctx.closePath();
        ctx.fill(); ctx.stroke();
        ctx.restore();
    }
    for (const Bandana& b : bandanas) {
        Point3 bp = c.project3D(b.x, b.y, b.z);
        if (!bp.ok() || frontOf(c, bp) != front) continue;
        ctx.save();
        ctx.translate(bp.x, bp.y);
        ctx.rotate(b.rot);
        ctx.globalAlpha = b.life * 0.95f;
        const float bw = 34 * bp.scale, bh = 14 * bp.scale, wave = std::sin(b.wavePhase) * 7 * bp.scale;
        ctx.beginPath();
        ctx.moveTo(-bw / 2, -bh / 2);
        ctx.quadraticCurveTo(0, -bh / 2 + wave, bw / 2, -bh / 2);
        ctx.lineTo(bw / 2, bh / 2);
        ctx.quadraticCurveTo(0, bh / 2 + wave, -bw / 2, bh / 2);
        ctx.closePath();
        ctx.fillStyle = "#dc2626"; ctx.strokeStyle = kInk; ctx.lineWidth = 1.8f; ctx.fill(); ctx.stroke();
        ctx.beginPath();
        ctx.moveTo(bw / 2, -bh / 2); ctx.lineTo(bw / 2 + 12 * bp.scale, -bh / 2 - 5 * bp.scale); ctx.lineTo(bw / 2 + 6 * bp.scale, 0);
        ctx.lineTo(bw / 2 + 14 * bp.scale, bh / 2 + 5 * bp.scale); ctx.lineTo(bw / 2, bh / 2);
        ctx.closePath();
        ctx.fillStyle = "#991b1b"; ctx.fill(); ctx.stroke();
        ctx.restore();
    }
}

// =========================================================================================
void JoanaFx::update(LabContext& c) {
    updateClawCrush(c);
    updateHookPull(c);
    updateBarnacle(c);
    updateCoral(c);
    updateAnchor(c);
    updateAnemone(c);

    // Gatilhos das passivas (no Lab eram botões): Armamento Assimétrico ao lançar gancho/uppercut,
    // Carapaça Calcária ao absorver um golpe, Maré das Profundezas ao entrar na forma monstruosa.
    const std::string& st = c.fighter.state;
    if (c.entity && !c.isPaused) {
        const auto& lo = c.entity->loadout;
        if (st != lastState) {
            if (lo.has(SkillId::JoanaAsymmetricArms) && asymmetricTimer <= 0 && (st == "hook" || st == "upper"))
                triggerAsymmetric(c, st == "hook" && c.fighter.isLeft);
            if (lo.has(SkillId::JoanaCalcifiedArmor) && calcifiedTimer <= 0 && st == "hit") triggerCalcified(c);
        }
        lastHp = c.entity->hp;
    }
    lastState = st;
    if (!c.isPaused) {
        updateAsymmetric();
        updateCalcified();
        updateDeepSeaTide(c);
    }
}

} // namespace

std::unique_ptr<LabFx> makeJoanaFx() { return std::make_unique<JoanaFx>(); }

} // namespace KoRama::vfx
