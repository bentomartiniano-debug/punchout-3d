// O Cremoso. Port de combat_lab_effects.js (linhas 7708-7965 e 8600-11660): Explosão de Calda Quente,
// Tornado de Baunilha (o corpo vira um vórtice procedural), Modo Derretido, Estocada de Casquinha,
// Poça de Calda Fervente, Cereja do Bolo e as auras das passivas (Vingança Doce e Stamina Gelada).
// Diferenças em relação ao JS: as torrentes de calda e a cereja vão até a distância real do rival; o
// Lab desenhava objetos de chão 26-28 px abaixo do ponto projetado (aqui ficam no chão); as gotas da
// borda derretida (código morto no JS) agora escorrem do corpo no Modo Derretido; e as auras das
// passivas, que no jogo nunca acendiam, ficam sempre visíveis com a skin equipada.
#include "lab_common.hpp"

namespace KoRama::vfx {

namespace {

constexpr const char* kInk = "#0f1c10";

template <class V>
void dropDead(V& v) {
    v.erase(std::remove_if(v.begin(), v.end(), [](const auto& e) { return e.life <= 0; }), v.end());
}

enum class Skin { Default, Frozen, Revenge };

struct Steam { float x, y, z, vx, vy, vz, scale, growth, life, decay, rot, rotSpeed; };
struct Spark { float x, y, z, vx, vy, vz, r; const char* color; float life, decay; };
struct Stream { int side; float startX, startY, startZ, targetX, targetY, targetZ, progress, life, decay; };
struct Splash { float x, y, z, vx, vy, vz, scale, rot, rotSpeed, life, decay; };
struct Puddle { float x, z, r, maxR, life, decay; };
struct Blob { float x, y, z, vx, vy, vz, scale, rot, rotSpeed, life, decay; const char* color; };
struct TRing { float x, y, z, scale, maxScale, rot, rotSpeed, life, decay; };
struct Star { float orbitAngle, orbitRadius, orbitSpeed, life, decay; };
struct Sat { float dx, dz, r; };
struct MeltSplash { float x, z, r, maxR, life, decay, alpha; Skin skin; Sat sats[2]; };
struct Vapor { float x, y, z, vx, vy, vz, r, maxR, life, decay; const char* color; };
struct Crumb { float x, y, z, vx, vy, vz, size, rot, rotSpeed, life, decay; const char* color; };
struct Ring { float x, y, z, r, maxR, life, decay; const char* color; };
struct Bubble { float x, y, z, r, maxR, growSpeed, wobblePhase, life, decay; const char* color; };
struct Cherry { float x, y, z, vx, vy, vz, r, stemAngle, stemWobble, spinAngle, life, maxZ; };
struct Trail { float x, y, z, r, life, decay; const char* color; };

const char* pick(Skin s, const char* def, const char* frozen, const char* revenge) {
    return s == Skin::Frozen ? frozen : (s == Skin::Revenge ? revenge : def);
}

const char* creamColor(Skin s) {
    if (s == Skin::Frozen) return rnd() > 0.4f ? "#f8fafc" : (rnd() > 0.5f ? "#bae6fd" : "#3b1a08");
    if (s == Skin::Revenge) return rnd() > 0.4f ? "#e11d48" : (rnd() > 0.5f ? "#be123c" : "#fb7185");
    return rnd() > 0.3f ? "#fffde7" : "#fef08a";
}

class CremosoFx final : public LabFx {
public:
    void update(LabContext& c) override;
    void drawFloor(Canvas& ctx, LabContext& c) override;
    void drawBackground(Canvas& ctx, LabContext& c) override { drawPass(ctx, c, false); }
    void drawForeground(Canvas& ctx, LabContext& c) override { drawPass(ctx, c, true); }
    void drawBodyOverlay(Canvas& ctx, LabContext& c, float bW, float bH) override;
    std::string mouthStage(const LabContext& c) const override;
    bool overridesBody(const LabContext& c) const override { return !c.liveBody && c.fighter.state == "cremoso_vanilla_whip"; }
    void drawBodyReplacement(Canvas& ctx, LabContext& c, sf::Vector2f pos, float scale, float bW, float bH) override {
        drawVortex(ctx, pos, scale, bW, bH, nowMs() / 45.0f, skinOf(c));
    }

private:
    unsigned variant = 0;
    // Stable storage for particle colors: particles keep these pointers after
    // their spawning frame. Passive cold/revenge coloring remains independent.
    const char* pick(Skin s,const char* def,const char* frozen,const char* revenge) const {
        if(s!=Skin::Default || !variant)return KoRama::vfx::pick(s,def,frozen,revenge);
        static const char* main[]={"#fef08a","#a2c65c","#f3bbca","#c5e9ef"};
        static const char* dark[]={"#ca8a04","#668239","#9c6d4c","#538dac"};
        static const char* light[]={"#fffde7","#d0df97","#fff0dc","#fff6df"};
        static const char* sauce[]={"#991b1b","#597538","#a95976","#7b443b"};
        const std::string d(def);
        if(d=="#fef08a"||d=="#fde047"||d=="#fef9c3"||d=="#eab308")return main[variant];
        if(d=="#ca8a04"||d=="#78350f")return dark[variant];
        if(d=="#fffde7"||d=="#f8fafc")return light[variant];
        if(d=="#991b1b"||d=="#dc2626"||d=="#7f1d1d")return sauce[variant];
        return def;
    }
    const char* creamColor(Skin s) const {
        if(!variant)return KoRama::vfx::creamColor(s);
        return rnd()>.4f?pick(s,"#fef08a","#bae6fd","#e11d48"):pick(s,"#fffde7","#f8fafc","#fb7185");
    }
    static Skin skinOf(const LabContext& c) {
        if (!c.entity) return Skin::Default;
        if (c.entity->loadout.has(SkillId::CremosoFrozenStamina)) return Skin::Frozen;
        if (c.entity->loadout.has(SkillId::CremosoSweetRevenge)) return Skin::Revenge;
        return Skin::Default;
    }
    float bodyDepth(const LabContext& c) const {
        Point3 b = c.project3D(c.fighter.body.offsetX, c.fighter.body.offsetY + 60, 0);
        return b.ok() ? b.z : 0.0f;
    }
    bool frontOf(const LabContext& c, const Point3& p) const { return p.z < bodyDepth(c); }
    float rivalDist(const LabContext& c) const {
        if (!c.rival || !c.entity) return 175.0f;
        return std::hypot(c.rival->worldPos.x - c.entity->worldPos.x, c.rival->worldPos.z - c.entity->worldPos.z);
    }

    void drawPass(Canvas& ctx, LabContext& c, bool front);
    void updateSyrup(LabContext& c);
    void drawSyrup(Canvas& ctx, LabContext& c, bool front);
    void updateTornado(LabContext& c);
    void drawTornado(Canvas& ctx, LabContext& c, bool front);
    void drawVortex(Canvas& ctx, sf::Vector2f pos, float scale, float bW, float bH, float spin, Skin skin);
    void updateMelt(LabContext& c);
    void drawMelt(Canvas& ctx, LabContext& c, bool front);
    void updateCone(LabContext& c);
    void drawCone(Canvas& ctx, LabContext& c, bool front);
    void drawSpikeCone(Canvas& ctx, LabContext& c, float ox, float oy, float oz, float angle, float maxLen, float maxRadius, float p);
    void drawChestCone(Canvas& ctx, LabContext& c, float ox, float oy, float oz, float p, bool deep);
    void drawApprovedCones(Canvas& ctx, LabContext& c, bool front);
    void updatePuddle(LabContext& c);
    void drawPuddleFloor(Canvas& ctx, LabContext& c);
    void drawPuddle(Canvas& ctx, LabContext& c, bool front);
    void updateCherry(LabContext& c);
    void drawCherry(Canvas& ctx, LabContext& c, bool front);
    void updatePassives(LabContext& c);
    void drawPassives(Canvas& ctx, LabContext& c, bool front);
    void bubble(Canvas& ctx, float x, float y, float r, const char* c0, const char* c1, const char* c2, const char* c3, float lw);

    Skin skin = Skin::Default;
    // Calda
    std::vector<Stream> streams;
    std::vector<Splash> splashes;
    std::vector<Puddle> syrupPuddles;
    std::vector<Steam> steam;
    std::vector<Spark> heatSparks;
    // Tornado
    std::vector<Blob> blobs;
    std::vector<TRing> tRings;
    std::vector<Puddle> groundSplashes;
    std::vector<Star> stars;
    // Derretido
    float meltProgress = 0;
    std::vector<Spark> meltDrops;
    std::vector<MeltSplash> meltSplashes;
    std::vector<Vapor> meltVapors;
    // Casquinha
    std::vector<Crumb> crumbs;
    std::vector<Ring> coneShocks;
    bool coneBurst = false;
    // Poça fervente
    std::vector<Puddle> boilPuddles;
    std::vector<Bubble> boilBubbles, footBubbles;
    std::vector<Vapor> boilVapors;
    std::vector<Spark> pops;
    float puddleBase = 0;
    // Cereja
    std::vector<Cherry> cherries;
    std::vector<Spark> juice;
    std::vector<Trail> trail;
    std::vector<Ring> cherryShocks;
    std::vector<Puddle> cherryPuddles;
    // Passivas
    std::vector<Bubble> passiveBubbles;
    std::vector<Vapor> passiveVapors;
};

// =========================================================================================
// EXPLOSÃO DE CALDA QUENTE
// =========================================================================================
void CremosoFx::updateSyrup(LabContext& c) {
    auto& f = c.fighter;
    const float p = c.progress();
    const bool active = f.state == "cremoso_syrup_blast";
    if (active && !c.isPaused) {
        const Glove gl = f.parts.gloveL, gr = f.parts.gloveR;
        if (p < 0.35f) {
            if (rnd() < 0.85f) {
                for (int s = -1; s <= 1; s += 2) {
                    const Glove& g = s < 0 ? gl : gr;
                    steam.push_back({g.x + (rnd() - 0.5f) * 14, g.y + 10 + (rnd() - 0.5f) * 8, g.z + 5, s * (0.8f + rnd() * 0.8f), 2.4f + rnd() * 2.2f,
                                     (rnd() - 0.5f), 0.6f + rnd() * 0.5f, 0.035f, 1.0f, 0.03f, rnd() * PI * 2, (rnd() - 0.5f) * 0.06f});
                }
            }
            if (rnd() < 0.7f) {
                for (int s = -1; s <= 1; s += 2) {
                    const Glove& g = s < 0 ? gl : gr;
                    heatSparks.push_back({g.x + (rnd() - 0.5f) * 16, g.y + (rnd() - 0.5f) * 16, g.z + 8, s * (1.5f + rnd() * 2), (rnd() - 0.5f) * 2.2f + 1.2f,
                                          (rnd() - 0.5f) * 2, 2.0f + rnd() * 3, rnd() > 0.5f ? "#f43f5e" : "#fde047", 1.0f, 0.045f});
                }
            }
        }
        if (p >= 0.35f && p <= 0.65f) {
            if (p >= 0.36f && streams.empty()) {
                const float tz = std::max(120.0f, rivalDist(c) - 10.0f);
                streams.push_back({-1, gl.x, gl.y, gl.z, -12, 70, tz, 0, 1.0f, 0.032f});
                streams.push_back({1, gr.x, gr.y, gr.z, 12, 70, tz, 0, 1.0f, 0.032f});
                // Approved attack throws syrup through the air. It no longer
                // creates a second persistent floor trap as a visual byproduct.
            }
            for (int i = 0; i < 4; i++) {
                const bool left = i % 2 == 0;
                const Glove& g = left ? gl : gr;
                const float ang = (rnd() - 0.5f) * 0.8f, spd = 3.5f + rnd() * 6.5f;
                splashes.push_back({g.x + (rnd() - 0.5f) * 14, g.y + (rnd() - 0.5f) * 12, g.z + rnd() * 15, std::sin(ang) * spd + (left ? 1.0f : -1.0f),
                                    (rnd() - 0.2f) * 5 + 1.5f, 4.5f + rnd() * 6, 0.5f + rnd() * 0.65f, rnd() * PI * 2, (rnd() - 0.5f) * 0.35f, 1.0f, 0.025f});
            }
            steam.push_back({(rnd() - 0.5f) * 35, 70 + (rnd() - 0.5f) * 16, 40 + rnd() * 70, (rnd() - 0.5f) * 1.5f, 2.5f + rnd() * 3, 2.5f + rnd() * 3,
                             0.7f + rnd() * 0.7f, 0.04f, 1.0f, 0.026f, rnd() * PI * 2, (rnd() - 0.5f) * 0.08f});
        }
    }
    if (!active && !c.isPaused && p == 0.0f) streams.clear();
    if (c.isPaused) return;
    for (auto& s : streams) { s.progress = std::min(1.0f, s.progress + 0.12f); s.life -= s.decay; }
    for (auto& s : splashes) {
        s.x += s.vx; s.y += s.vy; s.vy -= 0.24f; s.z += s.vz; s.rot += s.rotSpeed; s.life -= s.decay;
        if (s.y < 0) s.life = 0;
    }
    for (auto& pd : syrupPuddles) {
        pd.r += (pd.maxR - pd.r) * 0.1f;
        pd.life -= pd.decay;
        if (rnd() < 0.2f) {
            steam.push_back({pd.x + (rnd() - 0.5f) * pd.r * 0.7f, 10, pd.z + (rnd() - 0.5f) * 15, (rnd() - 0.5f) * 0.6f, 1.8f + rnd() * 2, (rnd() - 0.5f) * 0.6f,
                             0.5f + rnd() * 0.6f, 0.03f, 1.0f, 0.025f, rnd() * PI * 2, (rnd() - 0.5f) * 0.05f});
        }
    }
    for (auto& s : steam) { s.x += s.vx; s.y += s.vy; s.z += s.vz; s.scale += s.growth; s.rot += s.rotSpeed; s.life -= s.decay; }
    for (auto& s : heatSparks) { s.x += s.vx; s.y += s.vy; s.z += s.vz; s.life -= s.decay; }
    dropDead(streams); dropDead(splashes); dropDead(syrupPuddles); dropDead(steam); dropDead(heatSparks);
}

void CremosoFx::drawSyrup(Canvas& ctx, LabContext& c, bool front) {
    const char* sfx = pick(skin, "", "Frozen", "Revenge");
    const std::string sk = sfx;
    static const char* outerColors[]={"#975336","#65863b","#a95976","#7b443b"};
    static const char* innerColors[]={"#e6ba78","#c6db85","#f1b0c1","#f4bf99"};
    const char* outer = skin==Skin::Frozen?"#0284c7":skin==Skin::Revenge?"#450a0a":outerColors[variant];
    const char* inner = skin==Skin::Frozen?"#bae6fd":skin==Skin::Revenge?"#e11d48":innerColors[variant];
    auto streamImg = c.sprites->find("cremosoSyrupStream" + sk + "Sprite");
    for (const Stream& st : streams) {
        const float cx = st.startX + (st.targetX - st.startX) * st.progress, cy = st.startY + (st.targetY - st.startY) * st.progress,
                    cz = st.startZ + (st.targetZ - st.startZ) * st.progress;
        Point3 ps = c.project3D(st.startX, st.startY, st.startZ), pc = c.project3D(cx, cy, cz);
        if (!ps.ok() || !pc.ok() || frontOf(c, pc) != front) continue;
        ctx.save();
        ctx.globalAlpha = std::min(1.0f, st.life * 1.4f);
        const float ang = std::atan2(pc.y - ps.y, pc.x - ps.x), w = 28 * pc.scale;
        const float mx = (ps.x + pc.x) / 2, top = std::min(ps.y, pc.y);
        ctx.beginPath();
        ctx.moveTo(ps.x, ps.y - w * 0.4f);
        ctx.quadraticCurveTo(mx, top - 15 * pc.scale, pc.x, pc.y);
        ctx.quadraticCurveTo(mx, top + 15 * pc.scale, ps.x, ps.y + w * 0.4f);
        ctx.closePath();
        ctx.fillStyle = outer; ctx.strokeStyle = kInk; ctx.lineWidth = 3.0f * pc.scale;
        ctx.fill(); ctx.stroke();
        ctx.beginPath();
        ctx.moveTo(ps.x, ps.y - w * 0.2f);
        ctx.quadraticCurveTo(mx, top - 10 * pc.scale, pc.x - 5 * pc.scale, pc.y);
        ctx.quadraticCurveTo(mx, top + 10 * pc.scale, ps.x, ps.y + w * 0.2f);
        ctx.closePath();
        ctx.fillStyle = inner;
        ctx.fill();
        if (streamImg && !c.liveBody) {
            ctx.translate(pc.x, pc.y);
            ctx.rotate(ang + (st.side < 0 ? 0.15f : -0.15f));
            const float k = 0.6f + 0.4f * st.progress, sW = 160 * pc.scale * k, sH = 72 * pc.scale * k;
            ctx.drawImage(*streamImg, -sW * 0.4f, -sH / 2, sW, sH);
        }
        ctx.restore();
    }
    if (auto img = c.sprites->find("cremosoSyrupSplash" + sk + "Sprite")) {
        for (const Splash& sp : splashes) {
            Point3 bp = c.project3D(sp.x, sp.y, sp.z);
            if (!bp.ok() || frontOf(c, bp) != front) continue;
            ctx.save();
            ctx.translate(bp.x, bp.y);
            ctx.rotate(sp.rot);
            ctx.globalAlpha = std::min(1.0f, sp.life * 1.3f);
            const float s = 45 * bp.scale * sp.scale;
            if(!c.liveBody)ctx.drawImage(*img, -s / 2, -s / 2, s, s);
            else {
                ctx.beginPath();ctx.moveTo(-s*.48f,0);ctx.quadraticCurveTo(-s*.2f,-s*.4f,0,-s*.18f);ctx.quadraticCurveTo(s*.4f,-s*.3f,s*.5f,0);ctx.quadraticCurveTo(s*.1f,s*.4f,-s*.48f,0);ctx.closePath();
                ctx.fillStyle=outer;ctx.strokeStyle=kInk;ctx.lineWidth=1.5f*bp.scale;ctx.fill();ctx.stroke();
                ctx.beginPath();ctx.moveTo(-s*.3f,0);ctx.quadraticCurveTo(0,s*.15f,s*.3f,0);ctx.strokeStyle=inner;ctx.lineWidth=2*bp.scale;ctx.stroke();
            }
            ctx.restore();
        }
    }
    if (auto img = c.sprites->find("cremosoSweetSteam" + sk + "Sprite")) {
        for (const Steam& st : steam) {
            Point3 bp = c.project3D(st.x, st.y, st.z);
            if (!bp.ok() || frontOf(c, bp) != front) continue;
            ctx.save();
            ctx.translate(bp.x, bp.y);
            ctx.rotate(st.rot);
            ctx.globalAlpha = st.life * 0.75f;
            const float s = 60 * bp.scale * st.scale;
            ctx.drawImage(*img, -s / 2, -s / 2, s, s);
            ctx.restore();
        }
    }
    for (const Spark& s : heatSparks) {
        Point3 bp = c.project3D(s.x, s.y, s.z);
        if (!bp.ok() || frontOf(c, bp) != front) continue;
        ctx.save();
        ctx.globalAlpha = s.life;
        ctx.fillStyle = s.color; ctx.strokeStyle = kInk; ctx.lineWidth = 1.0f;
        ctx.beginPath(); ctx.arc(bp.x, bp.y, s.r * bp.scale, 0, PI * 2); ctx.fill(); ctx.stroke();
        ctx.restore();
    }
}

// =========================================================================================
// TORNADO DE BAUNILHA
// =========================================================================================
void CremosoFx::updateTornado(LabContext& c) {
    auto& f = c.fighter;
    const float p = c.progress();
    const bool active = f.state == "cremoso_vanilla_whip";
    const float bX = f.body.offsetX, bY = f.body.offsetY;
    if (active && !c.isPaused) {
        if (p < 0.25f) {
            if (rnd() < 0.35f) {
                const float ang = rnd() * PI * 2, spd = 2 + rnd() * 3.5f;
                blobs.push_back({bX + std::cos(ang) * 18, 70 + bY + (rnd() - 0.5f) * 10, std::sin(ang) * 18, std::cos(ang) * spd, (rnd() - 0.2f) * 2.5f + 1,
                                 std::sin(ang) * spd, 0.35f + rnd() * 0.3f, rnd() * PI * 2, (rnd() - 0.5f) * 0.25f, 1.0f, 0.025f, rnd() > 0.4f ? "#fffde7" : "#fef08a"});
            }
        } else if (p < 0.75f) {
            if (rnd() < 0.55f) {
                const float ang = rnd() * PI * 2, spd = 4 + rnd() * 5.5f;
                blobs.push_back({bX + std::cos(ang) * 14, 75 + bY + (rnd() - 0.5f) * 20, std::sin(ang) * 14, std::cos(ang) * spd, (rnd() - 0.3f) * 4.5f + 1.5f,
                                 std::sin(ang) * spd, 0.45f + rnd() * 0.45f, rnd() * PI * 2, (rnd() - 0.5f) * 0.35f, 1.0f, 0.018f, creamColor(skin)});
            }
            if (rnd() < 0.25f) tRings.push_back({bX, 35 + bY + rnd() * 50, (rnd() - 0.5f) * 8, 0.7f, 1.4f, rnd() * PI * 2, 0.18f, 1.0f, 0.035f});
        } else if (stars.empty()) {
            for (int s = 0; s < 4; s++) stars.push_back({s / 4.0f * PI * 2, 26, 0.1f, 1.0f, 0.015f});
        }
    } else if (!active) {
        stars.clear();
    }
    if (c.isPaused) return;
    for (auto& b : blobs) {
        b.x += b.vx; b.y += b.vy; b.vy -= 0.28f; b.z += b.vz; b.rot += b.rotSpeed; b.life -= b.decay;
        if (b.y <= 5 && b.vy < 0) {
            groundSplashes.push_back({b.x, b.z, 6 * b.scale, 14 * b.scale, 1.0f, 0.02f});
            b.life = 0;
        }
    }
    for (auto& r : tRings) { r.scale += (r.maxScale - r.scale) * 0.15f; r.rot += r.rotSpeed; r.y += 2; r.life -= r.decay; }
    for (auto& s : groundSplashes) { s.r += (s.maxR - s.r) * 0.12f; s.life -= s.decay; }
    for (auto& s : stars) { s.orbitAngle += s.orbitSpeed; s.life -= s.decay; }
    dropDead(blobs); dropDead(tRings); dropDead(groundSplashes); dropDead(stars);
}

void CremosoFx::drawTornado(Canvas& ctx, LabContext& c, bool front) {
    auto& f = c.fighter;
    auto blobImg = c.sprites->find("cremosoCreamBlobSprite");
    for (const Blob& bl : blobs) {
        Point3 bp = c.project3D(bl.x, bl.y, bl.z);
        if (!bp.ok() || frontOf(c, bp) != front) continue;
        ctx.save();
        ctx.translate(bp.x, bp.y);
        ctx.rotate(bl.rot);
        ctx.globalAlpha = std::min(1.0f, bl.life * 1.3f);
        const float s = 32 * bp.scale * bl.scale;
        if (blobImg && skin == Skin::Default && !variant) {
            ctx.drawImage(*blobImg, -s / 2, -s / 2, s, s);
        } else {
            // Melhoria: com skin, o creme voa na cor da skin (o sprite do Lab era sempre baunilha).
            ctx.beginPath(); ctx.arc(0, 0, s / 2, 0, PI * 2);
            ctx.fillStyle = bl.color; ctx.strokeStyle = kInk; ctx.lineWidth = 1.4f;
            ctx.fill(); ctx.stroke();
            ctx.beginPath(); ctx.arc(-s * 0.15f, -s * 0.15f, s * 0.12f, 0, PI * 2); ctx.fillStyle = "#ffffff"; ctx.fill();
        }
        ctx.restore();
    }
    if (!front) return;
    if (auto ring = c.sprites->find("cremosoCreamSpiralRingSprite")) {
        for (const TRing& rg : tRings) {
            Point3 rp = c.project3D(rg.x, rg.y, rg.z);
            if (!rp.ok()) continue;
            ctx.save();
            ctx.translate(rp.x, rp.y);
            ctx.rotate(rg.rot);
            ctx.globalAlpha = rg.life * 0.75f;
            const float w = 130 * rp.scale * rg.scale, h = 45 * rp.scale * rg.scale;
            ctx.drawImage(*ring, -w / 2, -h / 2, w, h);
            ctx.restore();
        }
    }
    if (!stars.empty() && f.state == "cremoso_vanilla_whip" && c.progress() > 0.78f) {
        Point3 head = c.project3D(f.body.offsetX, 110 + f.body.offsetY, 0);
        if (head.ok()) {
            for (const Star& st : stars) {
                ctx.save();
                ctx.translate(head.x + std::cos(st.orbitAngle) * st.orbitRadius * head.scale, head.y + std::sin(st.orbitAngle) * st.orbitRadius * 0.35f * head.scale);
                ctx.globalAlpha = st.life;
                ctx.fillStyle = "#facc15"; ctx.strokeStyle = kInk; ctx.lineWidth = 1.2f;
                const float r = 5.5f * head.scale;
                ctx.beginPath();
                for (int i = 0; i < 5; i++) {
                    const float a = i * 4 * PI / 5 - PI / 2;
                    if (i == 0) ctx.moveTo(std::cos(a) * r, std::sin(a) * r);
                    else ctx.lineTo(std::cos(a) * r, std::sin(a) * r);
                }
                ctx.closePath();
                ctx.fill(); ctx.stroke();
                ctx.restore();
            }
        }
    }
}

void CremosoFx::drawVortex(Canvas& ctx, sf::Vector2f pos, float s, float, float, float spin, Skin sk) {
    ctx.save();
    ctx.translate(pos.x, pos.y);
    ctx.rotate(std::sin(spin * 1.5f) * 0.06f);
    const char *colOutline = "#0f1c10", *colBase = "#ca8a04", *colTier = "#eab308", *colLight = "#fef08a", *colHi = "#fffde7",
               *colApron = "#f8fafc", *colApronStroke = "#0f1c10", *colWind = "#ffffff";
    if (sk == Skin::Frozen) {
        colOutline = "#0284c7"; colBase = "#0284c7"; colTier = "#38bdf8"; colLight = "#bae6fd"; colHi = "#f8fafc";
        colApron = "#f1f5f9"; colApronStroke = "#0284c7"; colWind = "#bae6fd";
    } else if (sk == Skin::Revenge) {
        colOutline = "#450a0a"; colBase = "#7f1d1d"; colTier = "#be123c"; colLight = "#e11d48"; colHi = "#fb7185";
        colApron = "#f8fafc"; colApronStroke = "#450a0a"; colWind = "#fb7185";
    }
    // Base pontiaguda no chão
    ctx.beginPath();
    ctx.moveTo(-18 * s, -10 * s); ctx.quadraticCurveTo(-14 * s, 18 * s, 0, 32 * s); ctx.quadraticCurveTo(14 * s, 18 * s, 18 * s, -10 * s);
    ctx.closePath();
    ctx.fillStyle = colBase; ctx.strokeStyle = colOutline; ctx.lineWidth = 2.4f * s; ctx.fill(); ctx.stroke();
    ctx.beginPath();
    ctx.moveTo(-14 * s, -10 * s); ctx.quadraticCurveTo(-10 * s, 14 * s, 0, 26 * s); ctx.quadraticCurveTo(10 * s, 14 * s, 14 * s, -10 * s);
    ctx.closePath();
    ctx.fillStyle = colLight; ctx.fill();
    auto tier = [&](float w, float top, float bottom, float dip, float lw, const char* fill, bool stroke) {
        ctx.beginPath();
        ctx.moveTo(-w * s, top * s);
        ctx.quadraticCurveTo(-(w - 2) * s, (top + (bottom - top) * 0.7f) * s, 0, bottom * s);
        ctx.quadraticCurveTo((w - 2) * s, (top + (bottom - top) * 0.7f) * s, w * s, top * s);
        ctx.quadraticCurveTo(0, dip * s, -w * s, top * s);
        ctx.closePath();
        ctx.fillStyle = fill;
        if (stroke) { ctx.strokeStyle = colOutline; ctx.lineWidth = lw * s; }
        ctx.fill();
        if (stroke) ctx.stroke();
    };
    tier(36, -42, 0, -32, 2.6f, colTier, true);
    tier(30, -40, -4, -34, 0, colLight, false);
    ctx.beginPath();
    ctx.moveTo(-16 * s, -34 * s); ctx.lineTo(16 * s, -34 * s); ctx.lineTo(10 * s, -14 * s); ctx.lineTo(-10 * s, -14 * s);
    ctx.closePath();
    ctx.fillStyle = colApron; ctx.strokeStyle = colApronStroke; ctx.lineWidth = 2.0f * s; ctx.fill(); ctx.stroke();
    tier(52, -78, -34, -68, 2.8f, colTier, true);
    tier(45, -76, -38, -70, 0, colLight, false);
    tier(35, -74, -42, -72, 0, colHi, false);
    ctx.beginPath(); ctx.ellipse(0, -82 * s, 50 * s, 14 * s, 0, 0, PI * 2);
    ctx.fillStyle = colBase; ctx.strokeStyle = colOutline; ctx.lineWidth = 2.8f * s; ctx.fill(); ctx.stroke();
    ctx.beginPath(); ctx.ellipse(0, -84 * s, 45 * s, 11 * s, 0, 0, PI * 2); ctx.fillStyle = colLight; ctx.fill();
    ctx.beginPath(); ctx.ellipse(0, -85 * s, 35 * s, 8 * s, 0, 0, PI * 2); ctx.fillStyle = colHi; ctx.fill();

    const float rot = spin * 2.5f;
    if (sk == Skin::Frozen) {
        ctx.beginPath(); ctx.ellipse(0, -86 * s, 40 * s, 9 * s, 0, 0, PI * 2);
        ctx.fillStyle = "#3b1a08"; ctx.strokeStyle = kInk; ctx.lineWidth = 1.6f * s; ctx.fill(); ctx.stroke();
        for (int t = 0; t < 2; t++) {
            const float a = rot + t * PI;
            ctx.save();
            ctx.translate(std::cos(a) * 40 * s, -86 * s + std::sin(a) * 9 * s);
            ctx.rotate(t == 0 ? rot * 0.8f - 0.4f : rot * 0.8f + PI * 0.5f);
            ctx.fillStyle = "#78350f"; ctx.strokeStyle = kInk; ctx.lineWidth = 1.4f * s;
            ctx.fillRect(-3.5f * s, -16 * s, 7 * s, 24 * s);
            ctx.strokeRect(-3.5f * s, -16 * s, 7 * s, 24 * s);
            ctx.fillStyle = "#451a03";
            ctx.fillRect(-3.5f * s, -10 * s, 7 * s, 3 * s);
            ctx.fillRect(-3.5f * s, -2 * s, 7 * s, 3 * s);
            ctx.restore();
        }
        static const char* sprinkles[] = {"#ef4444", "#3b82f6", "#eab308", "#22c55e", "#ec4899"};
        for (int i = 0; i < 8; i++) {
            const float a = rot * 1.5f + i * PI / 4, d = (28.0f + (i % 3) * 12) * s;
            ctx.fillStyle = sprinkles[i % 5];
            ctx.fillRect(std::cos(a) * d - 2 * s, -84 * s + std::sin(a) * d * 0.35f - s, 4 * s, 2.2f * s);
        }
    } else if (sk == Skin::Revenge) {
        ctx.beginPath(); ctx.ellipse(0, -86 * s, 42 * s, 10 * s, 0, 0, PI * 2);
        ctx.fillStyle = "#be123c"; ctx.strokeStyle = "#450a0a"; ctx.lineWidth = 1.8f * s; ctx.fill(); ctx.stroke();
        ctx.beginPath(); ctx.ellipse(0, -87 * s, 34 * s, 7 * s, 0, 0, PI * 2); ctx.fillStyle = "#e11d48"; ctx.fill();
        for (int t = 0; t < 2; t++) {
            const float a = rot + t * PI;
            ctx.save();
            ctx.translate(std::cos(a) * 44 * s, -85 * s + std::sin(a) * 10 * s);
            ctx.fillStyle = "#d97706"; ctx.strokeStyle = kInk; ctx.lineWidth = 1.0f * s;
            ctx.fillRect(-1.2f * s, -16 * s, 2.4f * s, 12 * s);
            ctx.strokeRect(-1.2f * s, -16 * s, 2.4f * s, 12 * s);
            ctx.beginPath(); ctx.arc(0, 0, 7.5f * s, 0, PI * 2);
            ctx.fillStyle = "#be123c"; ctx.strokeStyle = "#450a0a"; ctx.lineWidth = 1.8f * s; ctx.fill(); ctx.stroke();
            ctx.beginPath(); ctx.arc(0, 0, 6 * s, 0, PI * 2); ctx.fillStyle = "#e11d48"; ctx.fill();
            ctx.beginPath(); ctx.arc(-2.2f * s, -2.5f * s, 2.2f * s, 0, PI * 2); ctx.fillStyle = "#ffffff"; ctx.fill();
            ctx.restore();
        }
    } else {
        for (int t = 0; t < 2; t++) {
            const float a = rot + t * PI, x = std::cos(a) * 44 * s, y = -84 * s + std::sin(a) * 10 * s;
            ctx.beginPath(); ctx.arc(x, y, 7 * s, 0, PI * 2);
            ctx.fillStyle = "#dc2626"; ctx.strokeStyle = kInk; ctx.lineWidth = 1.8f * s; ctx.fill(); ctx.stroke();
            ctx.beginPath(); ctx.arc(x - 2 * s, y - 2 * s, 2 * s, 0, PI * 2); ctx.fillStyle = "#fef08a"; ctx.fill();
        }
    }
    // Chapéu de chef rodopiando
    ctx.save();
    ctx.translate(0, -88 * s);
    ctx.rotate(std::sin(spin * 3) * 0.18f);
    ctx.beginPath();
    ctx.moveTo(-16 * s, 0);
    ctx.bezierCurveTo(-26 * s, -18 * s, -8 * s, -28 * s, 0, -18 * s);
    ctx.bezierCurveTo(8 * s, -28 * s, 26 * s, -18 * s, 16 * s, 0);
    ctx.closePath();
    ctx.fillStyle = colApron; ctx.strokeStyle = colApronStroke; ctx.lineWidth = 2.2f * s; ctx.fill(); ctx.stroke();
    ctx.fillRect(-16 * s, -2 * s, 32 * s, 7 * s);
    ctx.strokeRect(-16 * s, -2 * s, 32 * s, 7 * s);
    ctx.restore();
    // Fitas de vento
    const float wp = std::fmod(spin * 2.0f, PI * 2);
    ctx.strokeStyle = colWind; ctx.lineWidth = 2.4f * s; ctx.lineCap = "round";
    ctx.beginPath(); ctx.arc(0, -60 * s, 42 * s, wp, wp + 1.2f); ctx.stroke();
    ctx.beginPath(); ctx.arc(0, -28 * s, 30 * s, wp + 1.5f, wp + 2.7f); ctx.stroke();
    ctx.beginPath(); ctx.arc(0, 4 * s, 18 * s, wp + 3.0f, wp + 4.2f); ctx.stroke();
    // Melhoria: segunda volta de fitas do outro lado, o vórtice parece girar de verdade.
    ctx.globalAlpha = 0.6f;
    ctx.beginPath(); ctx.arc(0, -60 * s, 46 * s, wp + PI, wp + PI + 0.9f); ctx.stroke();
    ctx.beginPath(); ctx.arc(0, -28 * s, 34 * s, wp + PI + 1.5f, wp + PI + 2.4f); ctx.stroke();
    ctx.restore();
}

// =========================================================================================
// MODO DERRETIDO
// =========================================================================================
void CremosoFx::updateMelt(LabContext& c) {
    const int melt = c.entity ? c.entity->status.melt : 0;
    if (!c.isPaused) {
        if (melt > 0) meltProgress = melt <= 40 ? std::max(0.0f, melt / 40.0f) : std::min(1.0f, meltProgress + 0.08f);
        else meltProgress = std::max(0.0f, meltProgress - 0.04f);
    }
    if(c.liveBody){meltDrops.clear();meltSplashes.clear();meltVapors.clear();return;}
    if (c.isPaused) return;
    auto& f = c.fighter;
    const float bX = f.body.offsetX, bY = f.body.offsetY;
    if (meltProgress > 0.01f) {
        if (rnd() < 0.65f * meltProgress) {
            struct P { float x, y, z; };
            const P pts[] = {{bX, 70 + bY, 8}, {bX - 32, 85 + bY, 0}, {bX + 32, 85 + bY, 0}, {bX - 16, 28 + bY, 4}, {bX + 16, 28 + bY, 4}, {bX, 24 + bY, 6},
                             {f.parts.gloveL.x + bX, f.parts.gloveL.y + bY - 8, f.parts.gloveL.z}, {f.parts.gloveR.x + bX, f.parts.gloveR.y + bY - 8, f.parts.gloveR.z}};
            const P& pt = pts[static_cast<int>(rnd() * 8) % 8];
            meltDrops.push_back({pt.x + (rnd() - 0.5f) * 6, pt.y, pt.z + (rnd() - 0.5f) * 6, (rnd() - 0.5f) * 0.8f, -(0.5f + rnd()), (rnd() - 0.5f) * 0.8f,
                                 2.2f + rnd() * 2.6f, creamColor(skin), 1.0f, 0.008f});
        }
        if (rnd() < 0.35f * meltProgress) {
            meltVapors.push_back({bX + (rnd() - 0.5f) * 36, 35 + bY + rnd() * 60, 10 + (rnd() - 0.5f) * 16, (rnd() - 0.5f) * 0.6f, 0.8f + rnd() * 1.2f,
                                  (rnd() - 0.5f) * 0.6f, 5 + rnd() * 6, 0, 1.0f, 0.024f,
                                  pick(skin, "rgba(254, 240, 138, 0.35)", "rgba(186, 230, 253, 0.45)", "rgba(225, 29, 72, 0.45)")});
        }
    }
    for (auto& d : meltDrops) {
        d.x += d.vx; d.y += d.vy; d.z += d.vz; d.vy -= 0.3f; d.life -= d.decay;
        if (d.y <= 0) {
            MeltSplash ms{d.x, d.z, d.r * 1.6f, d.r * (3.0f + rnd() * 2), 1.0f, 0.012f, 0.85f, skin, {}};
            ms.sats[0] = {(rnd() - 0.5f) * 12, (rnd() - 0.5f) * 8, d.r * 0.5f};
            ms.sats[1] = {(rnd() - 0.5f) * 12, (rnd() - 0.5f) * 8, d.r * 0.4f};
            meltSplashes.push_back(ms);
            d.life = 0;
        }
    }
    for (auto& s : meltSplashes) { if (s.r < s.maxR) s.r += (s.maxR - s.r) * 0.16f; s.life -= s.decay; }
    for (auto& v : meltVapors) { v.x += v.vx; v.y += v.vy; v.z += v.vz; v.r += 0.12f; v.life -= v.decay; }
    dropDead(meltDrops); dropDead(meltSplashes); dropDead(meltVapors);
}

void CremosoFx::drawMelt(Canvas& ctx, LabContext& c, bool front) {
    for (const Spark& dr : meltDrops) {
        Point3 p = c.project3D(dr.x, dr.y, dr.z);
        if (!p.ok() || frontOf(c, p) != front) continue;
        const float r = dr.r * p.scale, h = r * 2.2f;
        ctx.save();
        ctx.translate(p.x, p.y);
        ctx.globalAlpha = std::min(1.0f, dr.life * 1.2f);
        ctx.beginPath();
        ctx.moveTo(0, -h);
        ctx.quadraticCurveTo(-r * 0.9f, -h * 0.2f, -r, 0);
        ctx.arc(0, 0, r, PI, 0, true);
        ctx.quadraticCurveTo(r * 0.9f, -h * 0.2f, 0, -h);
        ctx.closePath();
        ctx.fillStyle = dr.color; ctx.strokeStyle = kInk; ctx.lineWidth = 1.4f;
        ctx.fill(); ctx.stroke();
        ctx.fillStyle = "#ffffff";
        ctx.beginPath(); ctx.arc(-r * 0.3f, -r * 0.2f, std::max(0.5f, r * 0.32f), 0, PI * 2); ctx.fill();
        ctx.restore();
    }
    if (!front) return;
    for (const Vapor& v : meltVapors) {
        Point3 p = c.project3D(v.x, v.y, v.z);
        if (!p.ok()) continue;
        ctx.save();
        ctx.globalAlpha = v.life * 0.55f;
        ctx.fillStyle = v.color;
        ctx.beginPath(); ctx.arc(p.x, p.y, v.r * p.scale, 0, PI * 2); ctx.fill();
        ctx.restore();
    }
}

// =========================================================================================
// ESTOCADA DE CASQUINHA
// =========================================================================================
void CremosoFx::updateCone(LabContext& c) {
    auto& f = c.fighter;
    const float p = c.progress();
    const bool active = f.state == "cremoso_cone_punch";
    const float bX = f.body.offsetX, bY = f.body.offsetY;
    if (active) {
        if (!c.isPaused && p >= 0.25f && p < 0.65f && !coneBurst) {
            coneBurst = true;
            for (int i = 0; i < 24; i++) {
                const float ang = rnd() * PI * 2, spd = 3.5f + rnd() * 6;
                crumbs.push_back({bX + std::cos(ang) * 6, 98 + bY + std::sin(ang) * 6, 10 + rnd() * 35, std::cos(ang) * spd, (rnd() - 0.2f) * 3.5f + 1.5f,
                                  2 + rnd() * 5, 2.8f + rnd() * 3.8f, rnd() * PI * 2, (rnd() - 0.5f) * 0.35f, 1.0f, 0.022f + rnd() * 0.012f,
                                  pick(skin,rnd() > 0.4f ? "#ca8a04" : "#eab308","#bae6fd","#be123c")});
            }
            for (int i = 0; i < 3; i++) coneShocks.push_back({bX, 98 + bY, 12.0f + i * 35, 10.0f + i * 8, 38.0f + i * 14, 1.0f, 0.045f, pick(skin,"#fde047","#bae6fd","#fb7185")});
        }
    } else {
        coneBurst = false;
    }
    if (c.isPaused) return;
    for (auto& cb : crumbs) { cb.x += cb.vx; cb.y += cb.vy; cb.z += cb.vz; cb.vy -= 0.25f; cb.rot += cb.rotSpeed; cb.life -= cb.decay; if (cb.y < 0) cb.life = 0; }
    for (auto& s : coneShocks) { s.r += (s.maxR - s.r) * 0.22f; s.life -= s.decay; }
    dropDead(crumbs); dropDead(coneShocks);
}

static float coneExtension(float p) {
    if (p < 0.20f) return 0;
    if (p < 0.58f) return std::sin((p - 0.20f) / 0.38f * PI * 0.5f);
    if (p < 0.76f) return 1;
    return std::max(0.0f, 1.0f - (p - 0.76f) / 0.24f);
}

void CremosoFx::drawSpikeCone(Canvas& ctx, LabContext& c, float ox, float oy, float oz, float angle, float maxLen, float maxRadius, float p) {
    Point3 b = c.project3D(ox, oy, oz);
    if (!b.ok()) return;
    const float ext = coneExtension(p);
    if (ext <= 0.02f) return;
    const float len = maxLen * ext * b.scale, rad = maxRadius * (0.5f + 0.5f * ext) * b.scale;
    const float tx = b.x + std::cos(angle) * len, ty = b.y + std::sin(angle) * len;
    const float px = std::cos(angle + PI / 2) * rad, py = std::sin(angle + PI / 2) * rad;
    const float ax = b.x - std::cos(angle) * rad * 0.45f, ay = b.y - std::sin(angle) * rad * 0.45f;
    ctx.save();
    ctx.beginPath();
    ctx.moveTo(b.x - px, b.y - py); ctx.lineTo(tx, ty); ctx.lineTo(b.x + px, b.y + py); ctx.quadraticCurveTo(ax, ay, b.x - px, b.y - py);
    ctx.closePath();
    auto g = ctx.createLinearGradient(b.x - px, b.y - py, b.x + px, b.y + py);
    g->addColorStop(0, "#78350f"); g->addColorStop(0.35f, "#ca8a04"); g->addColorStop(0.7f, "#fde047"); g->addColorStop(1, "#ca8a04");
    ctx.fillStyle = g; ctx.strokeStyle = kInk; ctx.lineWidth = std::max(1.4f, 2.0f * b.scale);
    ctx.fill(); ctx.stroke();
    for (int i = 1; i <= 3; i++) {
        const float t = i / 4.0f, rx = b.x + (tx - b.x) * t, ry = b.y + (ty - b.y) * t, rr = rad * (1 - t * 0.9f);
        ctx.beginPath();
        ctx.moveTo(rx - std::cos(angle + PI / 2) * rr, ry - std::sin(angle + PI / 2) * rr);
        ctx.lineTo(rx + std::cos(angle + PI / 2) * rr, ry + std::sin(angle + PI / 2) * rr);
        ctx.strokeStyle = "rgba(120, 53, 15, 0.85)"; ctx.lineWidth = std::max(1.0f, 1.4f * b.scale);
        ctx.stroke();
    }
    ctx.fillStyle = "#ffffff";
    ctx.beginPath(); ctx.arc(tx, ty, std::max(1.5f, 2.4f * b.scale), 0, PI * 2); ctx.fill();
    ctx.restore();
}

void CremosoFx::drawChestCone(Canvas& ctx, LabContext& c, float ox, float oy, float oz, float p, bool deep) {
    Point3 b = c.project3D(ox, oy, oz);
    if (!b.ok()) return;
    const float ext = coneExtension(p);
    if (ext <= 0.01f) return;
    ctx.save();
    if (!deep) {
        // Vista de frente: o cone aponta para a câmera (círculo com vértice no centro).
        const float R = (8 + 17 * ext) * b.scale, cx = b.x, cy = b.y;
        ctx.beginPath(); ctx.arc(cx, cy, R, 0, PI * 2);
        auto g = ctx.createRadialGradient(cx, cy, b.scale, cx, cy, R);
        g->addColorStop(0, "#fffde7"); g->addColorStop(0.25f, "#fde047"); g->addColorStop(0.65f, "#eab308"); g->addColorStop(0.9f, "#ca8a04");
        g->addColorStop(1, "#78350f");
        ctx.fillStyle = g; ctx.strokeStyle = kInk; ctx.lineWidth = std::max(1.8f, 2.4f * b.scale);
        ctx.fill(); ctx.stroke();
        for (int r = 1; r <= 3; r++) {
            const float rr = R * (r / 3.4f);
            ctx.beginPath(); ctx.arc(cx, cy, rr, 0, PI * 2);
            ctx.strokeStyle = "rgba(120, 53, 15, 0.75)"; ctx.lineWidth = std::max(1.0f, 1.4f * b.scale); ctx.stroke();
            ctx.beginPath(); ctx.arc(cx, cy - 0.4f * b.scale, rr * 0.96f, PI * 0.9f, PI * 2.1f);
            ctx.strokeStyle = "rgba(255, 253, 231, 0.65)"; ctx.lineWidth = std::max(0.7f, 0.9f * b.scale); ctx.stroke();
        }
        for (int s = 0; s < 8; s++) {
            const float a = s * PI / 4 + PI / 8;
            ctx.beginPath(); ctx.moveTo(cx + std::cos(a) * R * 0.98f, cy + std::sin(a) * R * 0.98f); ctx.lineTo(cx, cy);
            ctx.strokeStyle = s % 2 == 0 ? kInk : "rgba(120, 53, 15, 0.80)";
            ctx.lineWidth = s % 2 == 0 ? std::max(1.3f, 1.8f * b.scale) : std::max(0.9f, 1.2f * b.scale);
            ctx.stroke();
        }
        ctx.beginPath(); ctx.arc(cx, cy, std::max(2.0f, 3.4f * b.scale), 0, PI * 2); ctx.fillStyle = kInk; ctx.fill();
        ctx.fillStyle = "#ffffff";
        ctx.beginPath(); ctx.arc(cx - 0.6f * b.scale, cy - 0.6f * b.scale, std::max(1.0f, 1.6f * b.scale), 0, PI * 2); ctx.fill();
        if (ext > 0.4f) {
            const float sr = std::max(2.8f, 5.5f * b.scale * ext);
            ctx.beginPath();
            ctx.moveTo(cx, cy - sr); ctx.lineTo(cx + sr * 0.35f, cy - sr * 0.35f); ctx.lineTo(cx + sr, cy); ctx.lineTo(cx + sr * 0.35f, cy + sr * 0.35f);
            ctx.lineTo(cx, cy + sr); ctx.lineTo(cx - sr * 0.35f, cy + sr * 0.35f); ctx.lineTo(cx - sr, cy); ctx.lineTo(cx - sr * 0.35f, cy - sr * 0.35f);
            ctx.closePath();
            ctx.fill();
        }
    } else {
        // Vista de costas: o cone perfura para o fundo, em direção ao rival.
        const float baseR = 22 * b.scale;
        Point3 tip = c.project3D(ox, oy + 2, oz + std::min(260.0f, rivalDist(c)) * ext);
        if (tip.ok()) {
            const float dx = tip.x - b.x, dy = tip.y - b.y, ang = std::atan2(dy, dx);
            const float px = std::cos(ang + PI / 2), py = std::sin(ang + PI / 2);
            const float mx = b.x + dx * 0.5f, my = b.y + dy * 0.5f, mr = baseR * 0.6f + 1.5f * b.scale;
            ctx.beginPath();
            ctx.moveTo(b.x - px * baseR, b.y - py * baseR);
            ctx.quadraticCurveTo(mx - px * mr, my - py * mr, tip.x, tip.y);
            ctx.quadraticCurveTo(mx + px * mr, my + py * mr, b.x + px * baseR, b.y + py * baseR);
            ctx.ellipse(b.x, b.y, baseR, baseR * 0.38f, ang, PI / 2, -PI / 2, true);
            ctx.closePath();
            auto g = ctx.createLinearGradient(b.x - px * baseR, b.y - py * baseR, b.x + px * baseR, b.y + py * baseR);
            g->addColorStop(0, "#78350f"); g->addColorStop(0.18f, "#ca8a04"); g->addColorStop(0.48f, "#fde047"); g->addColorStop(0.75f, "#fffde7");
            g->addColorStop(1, "#ca8a04");
            ctx.fillStyle = g; ctx.strokeStyle = kInk; ctx.lineWidth = std::max(1.8f, 2.4f * b.scale);
            ctx.fill(); ctx.stroke();
            for (int i = 1; i <= 8; i++) {
                const float t = i / 9.0f, r = baseR * (1 - t * 0.94f);
                ctx.beginPath(); ctx.ellipse(b.x + dx * t, b.y + dy * t, r, r * 0.4f, ang + PI / 2, 0, PI * 2);
                ctx.strokeStyle = "rgba(120, 53, 15, 0.85)"; ctx.lineWidth = std::max(1.2f, 1.6f * b.scale); ctx.stroke();
            }
            ctx.beginPath(); ctx.arc(tip.x, tip.y, std::max(2.4f, 4.2f * tip.scale), 0, PI * 2);
            ctx.fillStyle = "#fffde7"; ctx.strokeStyle = kInk; ctx.lineWidth = 1.6f; ctx.fill(); ctx.stroke();
        }
    }
    ctx.restore();
}

void CremosoFx::drawApprovedCones(Canvas& ctx, LabContext& c, bool front) {
    const float ext=coneExtension(c.progress());if(ext<=.01f)return;
    const bool back=c.activeCamera=="back";
    const float bx=c.fighter.body.offsetX,by=c.fighter.body.offsetY;
    static const char* cones[]={"#d5a45e","#7f9a46","#d7b17a","#f3d4a7"};
    static const char* shades[]={"#86532f","#3f652f","#805536","#ba6b91"};
    static const char* lights[]={"#f5d79a","#d3e2a2","#ffe6b9","#ffeada"};
    auto side=[&](float x,float y,float z,float tx,float ty,float tz,float radius){
        const Point3 a=c.project3D(bx+x,by+y,z),b=c.project3D(bx+x+(tx-x)*ext,by+y+(ty-y)*ext,z+(tz-z)*ext);
        if(!a.ok()||!b.ok())return;
        const float r=radius*a.scale*ext;
        ctx.save();ctx.globalAlpha=ext;
        ctx.beginPath();ctx.moveTo(a.x-r,a.y);ctx.lineTo(b.x,b.y);ctx.lineTo(a.x+r,a.y);ctx.closePath();
        ctx.fillStyle=cones[variant];ctx.strokeStyle=kInk;ctx.lineWidth=std::max(1.5f,2*a.scale);ctx.fill();ctx.stroke();
        ctx.beginPath();
        for(int i=1;i<7;++i){const float q=i/7.f,cx=a.x+(b.x-a.x)*q,cy=a.y+(b.y-a.y)*q,rr=r*(1-q);ctx.moveTo(cx-rr,cy);ctx.lineTo(cx+rr,cy);ctx.moveTo(a.x-r+2*r*q,a.y);ctx.lineTo(b.x,b.y);}
        ctx.strokeStyle=shades[variant];ctx.lineWidth=std::max(.8f,a.scale);ctx.stroke();
        if(variant==3){ctx.beginPath();ctx.moveTo(a.x-r*.55f,a.y);ctx.lineTo(b.x,b.y);ctx.strokeStyle="#ed8eb3";ctx.lineWidth=2*a.scale;ctx.stroke();}
        ctx.beginPath();ctx.moveTo(b.x-a.scale,b.y+5*b.scale);ctx.lineTo(b.x,b.y+2*b.scale);ctx.strokeStyle=lights[variant];ctx.lineWidth=1.4f*b.scale;ctx.stroke();ctx.restore();
    };
    if(back==front){
        // Exactly two triangular cones grow out of the back. Camera-local
        // projection retains the native depth pass; neither becomes a disk.
        side(-24,98,-8,-40,157,-38,9);side(24,98,-8,40,157,-38,9);
    }
    if(back!=front){
        if(back)side(0,98,12,0,110,std::min(260.f,rivalDist(c)),22);
        else {
            const Point3 p=c.project3D(bx,98+by,12);if(!p.ok())return;
            const float r=20*ext*p.scale;
            ctx.save();ctx.beginPath();ctx.arc(p.x,p.y,r,0,PI*2);ctx.fillStyle=cones[variant];ctx.strokeStyle=kInk;ctx.lineWidth=std::max(1.6f,2*p.scale);ctx.fill();ctx.stroke();
            ctx.beginPath();
            for(int i=-4;i<=4;++i){const float d=i*r/5.f;if(std::fabs(d)>=r)continue;const float half=std::sqrt(r*r-d*d);ctx.moveTo(p.x-half,p.y+d);ctx.lineTo(p.x+half,p.y+d);ctx.moveTo(p.x+d,p.y-half);ctx.lineTo(p.x+d,p.y+half);}
            ctx.strokeStyle=shades[variant];ctx.lineWidth=std::max(.8f,p.scale);ctx.stroke();
            ctx.beginPath();ctx.moveTo(p.x,p.y);ctx.lineTo(p.x+r*.65f,p.y-r*.65f);ctx.lineTo(p.x+r*.75f,p.y+r*.4f);ctx.closePath();ctx.fillStyle=shades[variant];ctx.globalAlpha=.45f;ctx.fill();ctx.globalAlpha=1;
            ctx.beginPath();ctx.arc(p.x-r*.1f,p.y-r*.1f,2*p.scale,0,PI*2);ctx.fillStyle=lights[variant];ctx.fill();ctx.restore();
        }
    }
}

void CremosoFx::drawCone(Canvas& ctx, LabContext& c, bool front) {
    auto& f = c.fighter;
    const bool active = f.state == "cremoso_cone_punch";
    const float p = c.progress(), bX = f.body.offsetX, bY = f.body.offsetY;
    const bool isBack = c.activeCamera == "back";
    if(active&&c.liveBody)drawApprovedCones(ctx,c,front);
    if (active && !c.liveBody) {
        // O cone do peito aponta para o rival; os espinhos dos ombros e das costas, para o lado da câmera de costas.
        if (isBack != front) drawChestCone(ctx, c, bX, 54 + bY, 12, p, isBack);
        if (isBack == front) {
            drawSpikeCone(ctx, c, bX - 28, 72 + bY, -4, -PI * 0.74f, 46, 11, p);
            drawSpikeCone(ctx, c, bX + 28, 72 + bY, -4, -PI * 0.26f, 46, 11, p);
            drawSpikeCone(ctx, c, bX - 17, 64 + bY, -16, -PI * 0.72f, 37, 10, p);
            drawSpikeCone(ctx, c, bX + 17, 64 + bY, -16, -PI * 0.28f, 37, 10, p);
            drawSpikeCone(ctx, c, bX, 82 + bY, -14, -PI / 2, 43, 11, p);
        }
    }
    for (const Crumb& cb : crumbs) {
        Point3 bp = c.project3D(cb.x, cb.y, cb.z);
        if (!bp.ok() || frontOf(c, bp) != front) continue;
        const float sz = cb.size * bp.scale;
        ctx.save();
        ctx.translate(bp.x, bp.y);
        ctx.rotate(cb.rot);
        ctx.globalAlpha = std::min(1.0f, cb.life * 1.3f);
        ctx.beginPath(); ctx.moveTo(0, -sz); ctx.lineTo(sz * 0.85f, sz * 0.7f); ctx.lineTo(-sz * 0.85f, sz * 0.7f); ctx.closePath();
        ctx.fillStyle = cb.color; ctx.strokeStyle = kInk; ctx.lineWidth = 1.0f;
        ctx.fill(); ctx.stroke();
        ctx.restore();
    }
    for (const Ring& sw : coneShocks) {
        Point3 bp = c.project3D(sw.x, sw.y, sw.z);
        if (!bp.ok() || frontOf(c, bp) != front) continue;
        ctx.save();
        ctx.globalAlpha = sw.life * 0.75f;
        ctx.strokeStyle = sw.color; ctx.lineWidth = 2.5f * bp.scale;
        ctx.beginPath(); ctx.ellipse(bp.x, bp.y, sw.r * bp.scale, sw.r * 0.45f * bp.scale, 0, 0, PI * 2); ctx.stroke();
        ctx.restore();
    }
}

// =========================================================================================
// POÇA DE CALDA FERVENTE
// =========================================================================================
void CremosoFx::updatePuddle(LabContext& c) {
    auto& f = c.fighter;
    const float p = c.progress();
    const bool active = f.state == "cremoso_puddle_trap";
    const float bX = f.body.offsetX;
    if (c.isPaused) return;
    if (!boilPuddles.empty()) {
        float maxLife = 0;
        for (auto& pd : boilPuddles) maxLife = std::max(maxLife, pd.life);
        const float target = maxLife > 0.15f ? 1.0f : maxLife / 0.15f;
        puddleBase += (target - puddleBase) * (maxLife > 0.15f ? 0.14f : 0.12f);
    } else {
        puddleBase += (0.0f - puddleBase) * 0.12f;
        if (puddleBase < 0.005f) puddleBase = 0;
    }
    if (active) {
        if (p >= 0.15f && boilPuddles.empty()) {
            const Vec3 wb = Physics3D::localToWorld(bX, 0, 14, c.entity->worldPos, c.entity->facing);
            boilPuddles.push_back({wb.x, wb.z, 12, 54, 1.0f, 0.0032f});
        }
        if (p >= 0.20f && p <= 0.80f) {
            if (rnd() < 0.75f) {
                const float ang = rnd() * PI * 2, d = rnd() * 38;
                boilBubbles.push_back({bX + std::cos(ang) * d, 2, 14 + std::sin(ang) * d * 0.7f, 1.5f, 3.5f + rnd() * 5.5f, 0.24f + rnd() * 0.32f, rnd() * PI * 2,
                                       1.0f, 0.035f, creamColor(skin)});
            }
            if (rnd() < 0.55f) {
                const bool left = rnd() > 0.5f;
                footBubbles.push_back({bX + (left ? -14 : 14) + (rnd() - 0.5f) * 8, 2 + rnd() * 10, 14 + (rnd() - 0.5f) * 6, 1.2f, 3.0f + rnd() * 4,
                                       0.2f + rnd() * 0.28f, 0, 1.0f, 0.04f, pick(skin, "#fffde7", "#bae6fd", "#e11d48")});
            }
            if (rnd() < 0.4f) {
                const float ang = rnd() * PI * 2, d = rnd() * 30;
                boilVapors.push_back({bX + std::cos(ang) * d, 6 + rnd() * 12, 14 + std::sin(ang) * d * 0.7f, (rnd() - 0.5f) * 0.6f, 1.6f + rnd() * 2,
                                      (rnd() - 0.5f) * 0.6f, 6 + rnd() * 8, 18 + rnd() * 12, 1.0f, 0.025f, pick(skin, "#fef9c3", "#bae6fd", "#fda4af")});
            }
        }
    }
    for (auto& pd : boilPuddles) {
        pd.r += (pd.maxR - pd.r) * 0.12f;
        pd.life -= pd.decay;
        if (rnd() < 0.2f) {
            const float ang = rnd() * PI * 2, d = rnd() * pd.r * 0.6f;
            boilVapors.push_back({pd.x + std::cos(ang) * d, 6, pd.z + std::sin(ang) * d * 0.7f, (rnd() - 0.5f) * 0.4f, 1.2f + rnd() * 1.5f, (rnd() - 0.5f) * 0.4f,
                                  5 + rnd() * 6, 16 + rnd() * 10, 1.0f, 0.024f, pick(skin, "#fef9c3", "#bae6fd", "#fda4af")});
        }
    }
    auto popAt = [&](const Bubble& b, int n, float y) {
        for (int k = 0; k < n; k++) {
            const float a = rnd() * PI * 2, s = 1.5f + rnd() * 2.6f;
            pops.push_back({b.x, y, b.z, std::cos(a) * s, 1.6f + rnd() * 2.8f, std::sin(a) * s, 1.0f + rnd() * 1.4f, b.color, 1.0f, 0.045f});
        }
    };
    for (auto& b : boilBubbles) {
        b.r += b.growSpeed; b.wobblePhase += 0.25f;
        if (b.r >= b.maxR) { popAt(b, 4, 6 + b.r * 0.5f); b.life = 0; continue; }
        b.life -= b.decay;
    }
    for (auto& b : footBubbles) {
        b.r += b.growSpeed; b.y += 0.4f;
        if (b.r >= b.maxR) { popAt(b, 3, b.y); b.life = 0; continue; }
        b.life -= b.decay;
    }
    for (auto& s : pops) { s.x += s.vx; s.y += s.vy; s.z += s.vz; s.vy -= 0.22f; s.life -= s.decay; if (s.y < 0) s.life = 0; }
    for (auto& v : boilVapors) { v.x += v.vx; v.y += v.vy; v.z += v.vz; v.r += (v.maxR - v.r) * 0.05f; v.life -= v.decay; }
    dropDead(boilPuddles); dropDead(boilBubbles); dropDead(footBubbles); dropDead(pops); dropDead(boilVapors);
}

void CremosoFx::bubble(Canvas& ctx, float x, float y, float r, const char* c0, const char* c1, const char* c2, const char* c3, float lw) {
    ctx.beginPath(); ctx.arc(x, y, r, 0, PI * 2);
    auto g = ctx.createRadialGradient(x - r * 0.3f, y - r * 0.3f, r * 0.1f, x, y, r);
    g->addColorStop(0, "#ffffff"); g->addColorStop(0.4f, c0); g->addColorStop(0.75f, c1); g->addColorStop(0.92f, c2); g->addColorStop(1, c3);
    ctx.fillStyle = g; ctx.strokeStyle = kInk; ctx.lineWidth = lw;
    ctx.fill(); ctx.stroke();
    ctx.fillStyle = "#ffffff";
    ctx.beginPath(); ctx.arc(x - r * 0.33f, y - r * 0.33f, std::max(0.6f, r * 0.3f), 0, PI * 2); ctx.fill();
}

void CremosoFx::drawPuddleFloor(Canvas& ctx, LabContext& c) {
    if(c.liveBody)return; // approved pooled silhouette already owns the seam
    const float time = nowMs() / 260.0f;
    for (const Puddle& pd : boilPuddles) {
        Point3 pc = c.projectWorldFloor(pd.x, pd.z);
        if (!pc.ok()) continue;
        ctx.save();
        ctx.translate(pc.x, pc.y);
        ctx.globalAlpha = std::min(1.0f, pd.life * 1.5f);
        const float rx = pd.r * pc.scale * 2.0f, ry = rx * 0.38f;
        auto wobbly = [&](float ex, float ey) {
            ctx.beginPath();
            bool first = true;
            for (float a = 0; a <= PI * 2 + 0.15f; a += 0.12f) {
                const float w = std::sin(a * 5 + time) * 0.08f + std::cos(a * 3 - time * 1.4f) * 0.06f;
                const float px = std::cos(a) * ex * (1 + w), py = std::sin(a) * ey * (1 + w);
                if (first) { ctx.moveTo(px, py); first = false; } else ctx.lineTo(px, py);
            }
            ctx.closePath();
        };
        wobbly(rx + 2.5f, ry + 1.8f);
        ctx.fillStyle = pick(skin, "#ca8a04", "#0284c7", "#450a0a"); ctx.strokeStyle = kInk; ctx.lineWidth = std::max(1.8f, 2.6f * pc.scale);
        ctx.fill(); ctx.stroke();
        wobbly(rx, ry);
        auto g = ctx.createRadialGradient(0, -ry * 0.25f, rx * 0.1f, 0, 0, rx);
        if (skin == Skin::Frozen) { g->addColorStop(0, "#ffffff"); g->addColorStop(0.35f, "#f8fafc"); g->addColorStop(0.7f, "#bae6fd"); g->addColorStop(0.9f, "#38bdf8"); g->addColorStop(1, "#0284c7"); }
        else if (skin == Skin::Revenge) { g->addColorStop(0, "#ffffff"); g->addColorStop(0.35f, "#fb7185"); g->addColorStop(0.7f, "#e11d48"); g->addColorStop(0.9f, "#be123c"); g->addColorStop(1, "#7f1d1d"); }
        else { g->addColorStop(0, "#fffde7"); g->addColorStop(0.35f, "#fef08a"); g->addColorStop(0.7f, "#fde047"); g->addColorStop(0.9f, "#eab308"); g->addColorStop(1, "#ca8a04"); }
        ctx.fillStyle = g;
        ctx.fill();
        for (int ring = 1; ring <= 3; ring++) {
            const float t = ring / 3.8f, k = 1 + 0.1f * std::sin(time * 2.2f + ring);
            ctx.beginPath();
            ctx.ellipse(std::sin(time + ring) * 2.5f, std::cos(time + ring) * 1.2f, rx * t * k, ry * t * k, 0, 0, PI * 2);
            ctx.strokeStyle = ring == 2 ? pick(skin, "rgba(15, 28, 16, 0.45)", "rgba(2, 132, 199, 0.45)", "rgba(69, 10, 10, 0.45)")
                                        : pick(skin, "rgba(202, 138, 4, 0.85)", "rgba(56, 189, 248, 0.85)", "rgba(190, 18, 60, 0.85)");
            ctx.lineWidth = std::max(1.2f, 1.8f * pc.scale);
            ctx.stroke();
        }
        const sf::Color wave = skin == Skin::Frozen ? sf::Color(186, 230, 253) : (skin == Skin::Revenge ? sf::Color(251, 113, 133) : sf::Color(255, 253, 231));
        for (int w = 1; w <= 2; w++) {
            const float wr = std::fmod(time * 0.8f + w * 0.5f, 1.0f) * rx * 0.88f, wa = std::max(0.0f, 1 - wr / rx);
            ctx.beginPath(); ctx.ellipse(0, 0, wr, wr * 0.38f, 0, 0, PI * 2);
            ctx.strokeStyle = cv::rgba(wave.r, wave.g, wave.b, wa * 0.7f);
            ctx.lineWidth = std::max(1.0f, 1.5f * pc.scale);
            ctx.stroke();
        }
        ctx.fillStyle = "rgba(255, 255, 255, 0.92)";
        ctx.beginPath(); ctx.ellipse(-rx * 0.28f, -ry * 0.28f, rx * 0.2f, ry * 0.22f, -0.2f, 0, PI * 2); ctx.fill();
        ctx.restore();
    }
}

void CremosoFx::drawPuddle(Canvas& ctx, LabContext& c, bool front) {
    for (const Bubble& b : boilBubbles) {
        Point3 p = c.project3D(b.x, 2, b.z);
        if (!p.ok() || frontOf(c, p) != front) continue;
        const float r = b.r * p.scale;
        ctx.save();
        ctx.globalAlpha = std::min(1.0f, b.life * 1.4f);
        if (skin == Skin::Revenge) bubble(ctx, p.x, p.y - r * 0.4f, r + std::sin(b.wobblePhase) * r * 0.12f, "#fb7185", "#e11d48", "#be123c", "#7f1d1d", std::max(1.2f, 1.8f * p.scale));
        else if (skin == Skin::Frozen) bubble(ctx, p.x, p.y - r * 0.4f, r + std::sin(b.wobblePhase) * r * 0.12f, "#f8fafc", "#bae6fd", "#38bdf8", "#0284c7", std::max(1.2f, 1.8f * p.scale));
        else bubble(ctx, p.x, p.y - r * 0.4f, r + std::sin(b.wobblePhase) * r * 0.12f, b.color, "#fde047", "#eab308", "#ca8a04", std::max(1.2f, 1.8f * p.scale));
        ctx.restore();
    }
    for (const Bubble& b : footBubbles) {
        Point3 p = c.project3D(b.x, b.y, b.z);
        if (!p.ok() || frontOf(c, p) != front) continue;
        ctx.save();
        ctx.globalAlpha = std::min(1.0f, b.life * 1.4f);
        bubble(ctx, p.x, p.y, b.r * p.scale, pick(skin, "#fffde7", "#f8fafc", "#fb7185"), pick(skin, "#fde047", "#bae6fd", "#e11d48"),
               pick(skin, "#eab308", "#38bdf8", "#be123c"), pick(skin, "#ca8a04", "#0284c7", "#7f1d1d"), std::max(1.0f, 1.5f * p.scale));
        ctx.restore();
    }
    for (const Spark& s : pops) {
        Point3 p = c.project3D(s.x, s.y, s.z);
        if (!p.ok() || frontOf(c, p) != front) continue;
        ctx.save();
        ctx.globalAlpha = std::min(1.0f, s.life * 1.3f);
        ctx.beginPath(); ctx.arc(p.x, p.y, std::max(1.0f, s.r * p.scale), 0, PI * 2);
        ctx.fillStyle = s.color; ctx.strokeStyle = kInk; ctx.lineWidth = 0.9f; ctx.fill(); ctx.stroke();
        ctx.restore();
    }
    for (const Vapor& v : boilVapors) {
        Point3 p = c.project3D(v.x, v.y, v.z);
        if (!p.ok() || frontOf(c, p) != front) continue;
        ctx.save();
        ctx.globalAlpha = v.life * 0.35f;
        ctx.fillStyle = v.color;
        ctx.beginPath(); ctx.arc(p.x, p.y, v.r * p.scale, 0, PI * 2); ctx.fill();
        ctx.restore();
    }
}

// =========================================================================================
// CEREJA DO BOLO
// =========================================================================================
void CremosoFx::updateCherry(LabContext& c) {
    auto& f = c.fighter;
    const float p = c.progress();
    const bool active = f.state == "cremoso_cherry_topping";
    const float bX = f.body.offsetX, bY = f.body.offsetY;
    if (active && !c.isPaused && p < 0.44f && rnd() < 0.35f) {
        const float ang = rnd() * PI * 2, d = 20 + rnd() * 25;
        trail.push_back({bX + std::cos(ang) * d, bY + 50 + std::sin(ang) * d * 0.7f, 18 + (rnd() - 0.5f) * 8, 2.0f + rnd() * 2.2f, 0.8f, 0.05f,
                         rnd() > 0.4f ? "#fecdd3" : "#ffffff"});
    }
    if (active && !c.isPaused && p >= 0.48f && p <= 0.56f && cherries.empty() && cherryShocks.empty()) {
        const float maxZ = std::max(120.0f, rivalDist(c) - 10.0f);
        cherries.push_back({bX, bY + 70, 22, 0, 0.2f, 14.2f, 18.5f, -0.35f, 0, 0, 1.0f, maxZ});
        cherryShocks.push_back({bX, bY + 70, 22, 6, 48, 1.0f, 0.08f, "#ff4d6d"});
        for (int i = 0; i < 24; i++) {
            const float ang = rnd() * PI * 2, spd = 3 + rnd() * 6.5f;
            juice.push_back({bX, bY + 70, 22, std::cos(ang) * spd, std::sin(ang) * spd * 0.7f, 6 + rnd() * 8, 2.2f + rnd() * 3.4f,
                             rnd() > 0.3f ? "#dc2626" : "#f87171", 1.0f, 0.032f});
        }
    }
    if (c.isPaused) return;
    for (auto& ch : cherries) {
        ch.x += ch.vx; ch.y += ch.vy; ch.z += ch.vz;
        ch.stemWobble += 0.3f; ch.stemAngle = std::sin(ch.stemWobble) * 0.45f; ch.spinAngle += 0.12f;
        if (rnd() < 0.9f) trail.push_back({ch.x + (rnd() - 0.5f) * 8, ch.y + (rnd() - 0.5f) * 8, ch.z - 4, 3.5f + rnd() * 4, 1.0f, 0.045f, rnd() > 0.4f ? "#dc2626" : "#f43f5e"});
        if (ch.z >= ch.maxZ) {
            cherryShocks.push_back({ch.x, ch.y, ch.z, 10, 85, 1.0f, 0.05f, "#ff4d6d"});
            for (int k = 0; k < 42; k++) {
                const float ang = rnd() * PI * 2, spd = 4 + rnd() * 9;
                juice.push_back({ch.x, ch.y, ch.z, std::cos(ang) * spd, std::sin(ang) * spd * 0.85f, (rnd() - 0.5f) * 6, 2.5f + rnd() * 5,
                                 rnd() > 0.5f ? "#991b1b" : (rnd() > 0.3f ? "#dc2626" : "#ef4444"), 1.0f, 0.022f});
            }
            // Melhoria: a calda da cereja respinga no chão aos pés do rival.
            const Vec3 wc = Physics3D::localToWorld(ch.x, 0, ch.z, c.entity->worldPos, c.entity->facing);
            cherryPuddles.push_back({wc.x, wc.z, 6, 30, 1.0f, 0.008f});
            ch.life = 0;
        }
    }
    for (auto& t : trail) { t.life -= t.decay; t.r *= 0.96f; }
    for (auto& s : juice) { s.x += s.vx; s.y += s.vy; s.z += s.vz; s.vy -= 0.15f; s.life -= s.decay; if (s.y < 0) s.life = 0; }
    for (auto& s : cherryShocks) { s.r += (s.maxR - s.r) * 0.22f; s.life -= s.decay; }
    for (auto& pd : cherryPuddles) { pd.r += (pd.maxR - pd.r) * 0.12f; pd.life -= pd.decay; }
    dropDead(cherries); dropDead(trail); dropDead(juice); dropDead(cherryShocks); dropDead(cherryPuddles);
}

void CremosoFx::drawCherry(Canvas& ctx, LabContext& c, bool front) {
    for (const Ring& sw : cherryShocks) {
        Point3 p = c.project3D(sw.x, sw.y, sw.z);
        if (!p.ok() || frontOf(c, p) != front) continue;
        ctx.save();
        ctx.globalAlpha = sw.life * 0.85f;
        const float r = sw.r * p.scale;
        ctx.beginPath(); ctx.arc(p.x, p.y, r, 0, PI * 2);
        ctx.strokeStyle = sw.color; ctx.lineWidth = std::max(1.8f, 3.5f * p.scale * sw.life); ctx.stroke();
        ctx.beginPath(); ctx.arc(p.x, p.y, r * 0.85f, 0, PI * 2);
        ctx.fillStyle = cv::rgba(255, 255, 255, 0.35f * sw.life); ctx.fill();
        ctx.restore();
    }
    for (const Trail& t : trail) {
        Point3 p = c.project3D(t.x, t.y, t.z);
        if (!p.ok() || frontOf(c, p) != front) continue;
        ctx.save();
        ctx.globalAlpha = t.life * 0.85f;
        ctx.beginPath(); ctx.arc(p.x, p.y, std::max(1.0f, t.r * p.scale), 0, PI * 2);
        ctx.fillStyle = t.color; ctx.fill();
        ctx.restore();
    }
    for (const Cherry& ch : cherries) {
        Point3 p = c.project3D(ch.x, ch.y, ch.z);
        if (!p.ok() || frontOf(c, p) != front) continue;
        const float R = ch.r * p.scale, fs = p.scale;
        static const char* fruitMain[]={"#dc2626","#701d36","#70482e","#d75e9b"};
        static const char* fruitLight[]={"#ff4d6d","#ba6085","#b58661","#f5b2d2"};
        static const char* fruitDark[]={"#450a0a","#341b30","#352116","#7e2f65"};
        ctx.save();
        ctx.translate(p.x, p.y);
        ctx.rotate(ch.spinAngle * 0.3f);
        ctx.beginPath(); ctx.arc(0, 0, R, 0, PI * 2);
        auto g = ctx.createRadialGradient(-R * 0.32f, -R * 0.32f, R * 0.12f, 0, 0, R * 1.1f);
        g->addColorStop(0, fruitLight[variant]); g->addColorStop(0.35f, fruitMain[variant]); g->addColorStop(0.75f, fruitMain[variant]); g->addColorStop(0.95f, fruitDark[variant]); g->addColorStop(1, fruitDark[variant]);
        ctx.fillStyle = g; ctx.strokeStyle = kInk; ctx.lineWidth = std::max(1.8f, 2.6f * fs);
        ctx.fill(); ctx.stroke();
        ctx.beginPath(); ctx.ellipse(0, -R * 0.65f, R * 0.32f, R * 0.16f, 0, 0, PI * 2);
        ctx.fillStyle = "#7f1d1d"; ctx.lineWidth = std::max(1.0f, 1.4f * fs); ctx.fill(); ctx.stroke();
        ctx.fillStyle = "rgba(255, 255, 255, 0.94)";
        ctx.beginPath(); ctx.ellipse(-R * 0.38f, -R * 0.36f, R * 0.28f, R * 0.18f, -0.35f, 0, PI * 2); ctx.fill();
        ctx.beginPath(); ctx.arc(R * 0.35f, R * 0.35f, R * 0.12f, 0, PI * 2); ctx.fill();
        if(variant==1 || variant==2){
            ctx.beginPath();ctx.moveTo(-R*.4f,-R*.65f);ctx.quadraticCurveTo(R*.2f,-R*.15f,-R*.2f,R*.7f);
            ctx.strokeStyle=fruitDark[variant];ctx.lineWidth=std::max(1.f,1.7f*fs);ctx.stroke();
            if(variant==2){for(int i=0;i<7;++i){const float a=i*2.399f;ctx.beginPath();ctx.arc(std::cos(a)*R*.55f,std::sin(a)*R*.55f,1.3f*fs,0,PI*2);ctx.fillStyle=fruitLight[variant];ctx.fill();}}
        } else {
        ctx.save();
        ctx.translate(0, -R * 0.65f);
        ctx.rotate(ch.stemAngle);
        const float stem = R * 1.6f;
        ctx.lineCap = "round";
        ctx.beginPath(); ctx.moveTo(0, 0); ctx.quadraticCurveTo(R * 0.45f, -stem * 0.6f, -R * 0.25f, -stem);
        ctx.strokeStyle = "#15803d"; ctx.lineWidth = std::max(2.0f, 3.2f * fs); ctx.stroke();
        ctx.beginPath(); ctx.moveTo(0, 0); ctx.quadraticCurveTo(R * 0.45f, -stem * 0.6f, -R * 0.25f, -stem);
        ctx.strokeStyle = "#22c55e"; ctx.lineWidth = std::max(1.0f, 1.6f * fs); ctx.stroke();
        ctx.fillStyle = "#166534"; ctx.strokeStyle = kInk; ctx.lineWidth = std::max(0.8f, 1.2f * fs);
        ctx.beginPath(); ctx.arc(-R * 0.25f, -stem, std::max(1.5f, 2.5f * fs), 0, PI * 2); ctx.fill(); ctx.stroke();
        ctx.translate(-R * 0.25f, -stem);
        ctx.rotate(-0.5f + std::sin(ch.stemWobble * 1.5f) * 0.2f);
        const float lw = R * 0.65f, lh = R * 0.32f;
        ctx.beginPath(); ctx.moveTo(0, 0); ctx.quadraticCurveTo(lw * 0.5f, -lh * 1.2f, lw, 0); ctx.quadraticCurveTo(lw * 0.5f, lh * 1.2f, 0, 0);
        ctx.closePath();
        ctx.fillStyle = "#22c55e"; ctx.lineWidth = std::max(1.0f, 1.4f * fs); ctx.fill(); ctx.stroke();
        ctx.beginPath(); ctx.moveTo(0, 0); ctx.lineTo(lw * 0.85f, 0); ctx.strokeStyle = "#15803d"; ctx.lineWidth = std::max(0.8f, fs); ctx.stroke();
        ctx.restore();
        }
        ctx.restore();
    }
    for (const Spark& s : juice) {
        Point3 p = c.project3D(s.x, s.y, s.z);
        if (!p.ok() || frontOf(c, p) != front) continue;
        ctx.save();
        ctx.globalAlpha = std::min(1.0f, s.life * 1.4f);
        const float r = std::max(1.0f, s.r * p.scale);
        ctx.beginPath(); ctx.arc(p.x, p.y, r, 0, PI * 2);
        ctx.fillStyle = s.color; ctx.strokeStyle = kInk; ctx.lineWidth = 0.8f; ctx.fill(); ctx.stroke();
        ctx.fillStyle = "#ffffff";
        ctx.beginPath(); ctx.arc(p.x - r * 0.3f, p.y - r * 0.3f, std::max(0.4f, r * 0.3f), 0, PI * 2); ctx.fill();
        ctx.restore();
    }
}

// =========================================================================================
// AURAS DAS PASSIVAS
// =========================================================================================
void CremosoFx::updatePassives(LabContext& c) {
    if (c.isPaused) return;
    auto& f = c.fighter;
    const float bX = f.body.offsetX, bY = f.body.offsetY;
    if (skin == Skin::Revenge) {
        // A Vingança Doce ferve mais forte com a vida baixa.
        const float heat = c.entity && c.entity->hp < 50.0f ? 1.0f : 0.35f;
        if (rnd() < 0.65f * heat) {
            struct P { float x, y, z; };
            const P locs[] = {{bX - 26, 55 + bY, -12}, {bX + 26, 55 + bY, -12}, {bX, 40 + bY, 16}, {bX - 16, 15 + bY, 10}, {bX + 16, 15 + bY, 10}};
            const P& l = locs[static_cast<int>(rnd() * 5) % 5];
            passiveBubbles.push_back({l.x + (rnd() - 0.5f) * 12, l.y + (rnd() - 0.5f) * 8, l.z + (rnd() - 0.5f) * 8, 1.2f, 2.8f + rnd() * 3.5f,
                                      0.8f + rnd() * 1.4f, 0, 1.0f, 0.035f, rnd() > 0.4f ? "#e11d48" : (rnd() > 0.5f ? "#be123c" : "#fb7185")});
        }
        if (rnd() < 0.35f * heat) {
            passiveVapors.push_back({bX + (rnd() - 0.5f) * 36, 25 + bY + rnd() * 45, (rnd() - 0.5f) * 16, (rnd() - 0.5f) * 0.5f, 1.2f + rnd() * 1.8f,
                                     (rnd() - 0.5f) * 0.5f, 5 + rnd() * 7, 16 + rnd() * 10, 1.0f, 0.026f, "rgba(225, 29, 72, 0.45)"});
        }
    } else if (skin == Skin::Frozen && !c.liveBody && rnd() < 0.3f) {
        passiveVapors.push_back({bX + (rnd() - 0.5f) * 36, 20 + bY + rnd() * 50, (rnd() - 0.5f) * 16, (rnd() - 0.5f) * 0.4f, 0.8f + rnd() * 1.2f,
                                 (rnd() - 0.5f) * 0.4f, 5 + rnd() * 6, 15 + rnd() * 8, 1.0f, 0.024f, "rgba(186, 230, 253, 0.45)"});
    }
    // growSpeed guarda a velocidade de subida das bolhas.
    for (auto& b : passiveBubbles) { b.y += b.growSpeed; if (b.r < b.maxR) b.r += 0.22f; b.life -= b.decay; }
    for (auto& v : passiveVapors) { v.x += v.vx; v.y += v.vy; v.z += v.vz; v.r += (v.maxR - v.r) * 0.05f; v.life -= v.decay; }
    dropDead(passiveBubbles); dropDead(passiveVapors);
}

void CremosoFx::drawPassives(Canvas& ctx, LabContext& c, bool front) {
    for (const Bubble& b : passiveBubbles) {
        Point3 p = c.project3D(b.x, b.y, b.z);
        if (!p.ok() || frontOf(c, p) != front) continue;
        ctx.save();
        ctx.globalAlpha = std::min(1.0f, b.life * 1.5f);
        bubble(ctx, p.x, p.y, b.r * p.scale, b.color, b.color, "#7f1d1d", "#450a0a", std::max(0.8f, 1.2f * p.scale));
        ctx.restore();
    }
    for (const Vapor& v : passiveVapors) {
        Point3 p = c.project3D(v.x, v.y, v.z);
        if (!p.ok() || frontOf(c, p) != front) continue;
        ctx.save();
        ctx.globalAlpha = v.life * 0.4f;
        ctx.fillStyle = v.color;
        ctx.beginPath(); ctx.arc(p.x, p.y, v.r * p.scale, 0, PI * 2); ctx.fill();
        ctx.restore();
    }
}

// =========================================================================================
void CremosoFx::update(LabContext& c) {
    variant=c.entity?std::min(3u,c.entity->skinId):0;
    skin = skinOf(c);
    updateSyrup(c);
    updateTornado(c);
    updateMelt(c);
    updateCone(c);
    updatePuddle(c);
    updateCherry(c);
    updatePassives(c);
}

void CremosoFx::drawFloor(Canvas& ctx, LabContext& c) {
    const std::string sk = pick(skin, "", "Frozen", "Revenge");
    if (auto img = c.sprites->find("cremosoSyrupPuddle" + sk + "Sprite")) {
        for (const Puddle& pd : syrupPuddles) {
            Point3 pc = c.projectWorldFloor(pd.x, pd.z);
            if (!pc.ok()) continue;
            ctx.save();
            ctx.globalAlpha = std::min(1.0f, pd.life * 1.6f);
            const float w = pd.r * pc.scale * 2.4f, h = w * 0.38f;
            ctx.drawImage(*img, pc.x - w / 2, pc.y - h / 2, w, h);
            ctx.restore();
        }
    }
    for (const Puddle& sp : groundSplashes) {
        Point3 pc = c.projectWorldFloor(sp.x, sp.z);
        if (!pc.ok()) continue;
        ctx.save();
        ctx.globalAlpha = std::min(1.0f, sp.life * 1.5f);
        const float rx = sp.r * pc.scale * 1.8f;
        ctx.beginPath(); ctx.ellipse(pc.x, pc.y, rx, rx * 0.4f, 0, 0, PI * 2);
        ctx.fillStyle = pick(skin, "#fffde7", "#f8fafc", "#e11d48"); ctx.strokeStyle = kInk; ctx.lineWidth = 1.4f;
        ctx.fill(); ctx.stroke();
        ctx.restore();
    }
    for (const MeltSplash& sp : meltSplashes) {
        Point3 pc = c.project3D(sp.x, 0, sp.z);
        if (!pc.ok()) continue;
        ctx.save();
        ctx.translate(pc.x, pc.y);
        ctx.globalAlpha = std::min(0.92f, sp.life * sp.alpha);
        const float rx = sp.r * pc.scale * 1.8f, ry = rx * 0.38f;
        ctx.beginPath(); ctx.ellipse(0, 0, rx + 1.4f, ry + 1.0f, 0, 0, PI * 2);
        ctx.fillStyle = pick(sp.skin, "#ca8a04", "#0284c7", "#450a0a"); ctx.strokeStyle = kInk; ctx.lineWidth = 1.4f;
        ctx.fill(); ctx.stroke();
        auto g = ctx.createRadialGradient(0, -ry * 0.2f, rx * 0.1f, 0, 0, rx);
        if (sp.skin == Skin::Revenge) { g->addColorStop(0, "#ffffff"); g->addColorStop(0.3f, "#fb7185"); g->addColorStop(0.7f, "#e11d48"); g->addColorStop(1, "#be123c"); }
        else if (sp.skin == Skin::Frozen) { g->addColorStop(0, "#ffffff"); g->addColorStop(0.35f, "#f8fafc"); g->addColorStop(0.7f, "#bae6fd"); g->addColorStop(1, "#38bdf8"); }
        else { g->addColorStop(0, "#ffffff"); g->addColorStop(0.4f, "#fffde7"); g->addColorStop(0.8f, "#fef08a"); g->addColorStop(1, "#eab308"); }
        ctx.fillStyle = g;
        ctx.beginPath(); ctx.ellipse(0, 0, rx, ry, 0, 0, PI * 2); ctx.fill();
        for (const Sat& st : sp.sats) {
            const float r = st.r * pc.scale;
            ctx.beginPath(); ctx.ellipse(st.dx * pc.scale, st.dz * pc.scale * 0.38f, r * 1.4f, r * 0.6f, 0, 0, PI * 2);
            ctx.fillStyle = pick(sp.skin, "#fffde7", "#bae6fd", "#e11d48"); ctx.strokeStyle = kInk; ctx.lineWidth = 1.0f;
            ctx.fill(); ctx.stroke();
        }
        ctx.fillStyle = "rgba(255, 255, 255, 0.90)";
        ctx.beginPath(); ctx.ellipse(-rx * 0.28f, -ry * 0.25f, rx * 0.22f, ry * 0.22f, -0.2f, 0, PI * 2); ctx.fill();
        ctx.restore();
    }
    drawPuddleFloor(ctx, c);
    for (const Puddle& pd : cherryPuddles) {
        Point3 pc = c.projectWorldFloor(pd.x, pd.z);
        if (!pc.ok()) continue;
        ctx.save();
        ctx.translate(pc.x, pc.y);
        ctx.globalAlpha = std::min(1.0f, pd.life * 1.5f);
        const float rx = pd.r * pc.scale * 1.8f, ry = rx * 0.38f;
        ctx.beginPath(); ctx.ellipse(0, 0, rx, ry, 0, 0, PI * 2);
        auto g = ctx.createRadialGradient(0, -ry * 0.2f, rx * 0.1f, 0, 0, rx);
        g->addColorStop(0, "#ef4444"); g->addColorStop(0.6f, "#dc2626"); g->addColorStop(1, "#991b1b");
        ctx.fillStyle = g; ctx.strokeStyle = kInk; ctx.lineWidth = std::max(1.2f, 1.8f * pc.scale);
        ctx.fill(); ctx.stroke();
        ctx.fillStyle = "rgba(255, 255, 255, 0.85)";
        ctx.beginPath(); ctx.ellipse(-rx * 0.3f, -ry * 0.3f, rx * 0.22f, ry * 0.24f, -0.2f, 0, PI * 2); ctx.fill();
        ctx.restore();
    }
    // Sombra da cereja voando
    for (const Cherry& ch : cherries) {
        Point3 ps = c.project3D(ch.x, 0, ch.z);
        if (!ps.ok()) continue;
        const float alt = std::max(0.25f, 1.0f - ch.y / 140.0f), w = ch.r * ps.scale * 1.6f * alt;
        ctx.save();
        ctx.globalAlpha = 0.55f * alt;
        ctx.fillStyle = "#000000";
        ctx.beginPath(); ctx.ellipse(ps.x, ps.y, w, w * 0.36f, 0, 0, PI * 2); ctx.fill();
        ctx.restore();
    }
}

void CremosoFx::drawPass(Canvas& ctx, LabContext& c, bool front) {
    drawSyrup(ctx, c, front);
    drawTornado(ctx, c, front);
    drawMelt(ctx, c, front);
    drawCone(ctx, c, front);
    drawPuddle(ctx, c, front);
    drawCherry(ctx, c, front);
    drawPassives(ctx, c, front);
}

void CremosoFx::drawBodyOverlay(Canvas& ctx, LabContext& c, float bW, float bH) {
    if (c.liveBody || meltProgress <= 0.01f || overridesBody(c)) return;
    auto& f = c.fighter;
    Point3 base = c.project3D(f.body.offsetX, c.bodyLift + f.body.offsetY, 0);
    if (!base.ok()) return;
    const float time = nowMs() / 1000.0f;
    ctx.save();
    ctx.translate(base.x, base.y);
    ctx.transform(1, 0, f.body.bodySkew, 1, 0, 0);
    ctx.scale(base.scale, base.scale);
    ctx.globalAlpha = std::min(1.0f, meltProgress * 1.35f) * 0.55f;
    struct D { float x, y, len, r, phase; };
    const float chin = -bH * 0.44f, apron = -bH * 0.12f;
    const D pts[] = {{0, chin, bH * 0.12f, bW * 0.038f, 0}, {-bW * 0.32f, -bH * 0.65f, bH * 0.1f, bW * 0.032f, 1.5f},
                     {bW * 0.32f, -bH * 0.65f, bH * 0.1f, bW * 0.032f, 3.0f}, {-bW * 0.22f, apron, bH * 0.14f, bW * 0.034f, 4.2f},
                     {0, apron + 4, bH * 0.16f, bW * 0.04f, 2.1f}, {bW * 0.22f, apron, bH * 0.14f, bW * 0.034f, 5.4f}};
    for (const D& d : pts) {
        const float osc = std::sin(time * 4.8f + d.phase), len = d.len * (0.8f + 0.45f * osc), tip = d.y + len, r = d.r * (1 - 0.15f * osc);
        ctx.beginPath();
        ctx.moveTo(d.x - r * 0.8f, d.y);
        ctx.quadraticCurveTo(d.x - r * 0.85f, d.y + len * 0.65f, d.x, tip);
        ctx.quadraticCurveTo(d.x + r * 0.85f, d.y + len * 0.65f, d.x + r * 0.8f, d.y);
        ctx.closePath();
        auto g = ctx.createLinearGradient(d.x, d.y, d.x, tip);
        const char* tipCol;
        if (skin == Skin::Revenge) {
            g->addColorStop(0, "rgba(190, 18, 60, 0.0)"); g->addColorStop(0.35f, "rgba(225, 29, 72, 0.50)");
            g->addColorStop(0.7f, "rgba(251, 113, 133, 0.75)"); g->addColorStop(1, "rgba(255, 228, 230, 0.90)");
            tipCol = "rgba(225, 29, 72, 0.90)";
        } else if (skin == Skin::Frozen) {
            g->addColorStop(0, "rgba(2, 132, 199, 0.0)"); g->addColorStop(0.35f, "rgba(56, 189, 248, 0.50)");
            g->addColorStop(0.7f, "rgba(186, 230, 253, 0.75)"); g->addColorStop(1, "rgba(248, 250, 252, 0.90)");
            tipCol = "rgba(186, 230, 253, 0.90)";
        } else {
            g->addColorStop(0, "rgba(202, 138, 4, 0.0)"); g->addColorStop(0.35f, "rgba(234, 179, 8, 0.45)");
            g->addColorStop(0.7f, "rgba(254, 240, 138, 0.70)"); g->addColorStop(1, "rgba(255, 253, 231, 0.88)");
            tipCol = "rgba(255, 253, 231, 0.85)";
        }
        ctx.fillStyle = g;
        ctx.fill();
        ctx.strokeStyle = pick(skin, "rgba(15, 28, 16, 0.25)", "rgba(2, 132, 199, 0.30)", "rgba(69, 10, 10, 0.30)");
        ctx.lineWidth = 0.8f;
        ctx.stroke();
        ctx.beginPath(); ctx.arc(d.x, tip, r, 0, PI * 2); ctx.fillStyle = tipCol; ctx.fill(); ctx.stroke();
        ctx.fillStyle = "rgba(255, 255, 255, 0.55)";
        ctx.beginPath(); ctx.arc(d.x - r * 0.3f, tip - r * 0.3f, std::max(0.5f, r * 0.32f), 0, PI * 2); ctx.fill();
    }
    ctx.restore();
}

std::string CremosoFx::mouthStage(const LabContext& c) const {
    const auto& st = c.fighter.state;
    const float p = c.progress();
    if(c.liveBody){
        auto ease=[](float x){x=std::clamp(x,0.f,1.f);return x*x*(3-2*x);};
        if(st=="cremoso_vanilla_whip")return "Vortex"+std::to_string(static_cast<int>(std::round(8*ease(p/.10f)*(1-ease((p-.88f)/.12f)))));
    }
    if (st == "cremoso_cherry_topping") {
        if (p < 0.16f) return "Inflated1";
        if (p < 0.32f) return "Inflated2";
        if (p < 0.48f) return "Inflated";
        if (p < 0.78f) return "CherrySpit";
        if (p < 0.90f) return "Inflated1";
        return "";
    }
    if(c.liveBody && (puddleBase>.005f || st=="cremoso_puddle_trap"))return "Puddle"+std::to_string(static_cast<int>(std::round(8*std::clamp(puddleBase,0.f,1.f))));
    if (c.liveBody && (st=="cremoso_melt_mode" || meltProgress>.005f))return "Melt"+std::to_string(static_cast<int>(std::round(8*meltProgress)));
    if (st == "cremoso_melt_mode" || (c.entity && c.entity->status.melt > 0) || puddleBase > 0.3f) return "MeltBase";
    return "";
}

} // namespace

std::unique_ptr<LabFx> makeCremosoFx() { return std::make_unique<CremosoFx>(); }

} // namespace KoRama::vfx
