// Valéria Vulcão. O Soco Magmático é o port do Combat Lab (combat_lab_effects.js 7966-8600, com a
// física corrigida para Y para cima e o chão em Y = 0); os outros golpes eram pontos simples no
// index.html e ganharam animações novas: erupção com coluna de lava e bombas vulcânicas, pico de
// obsidiana sob o rival, aura de calor com línguas de fogo, nuvem de cinzas que persegue o rival e
// asas de fênix na cura. Fúria Vulcânica e Magma Frio têm brasas e vapor no corpo.
#include "fx_kit.hpp"

namespace KoRama::vfx {

namespace {

using namespace kit;

const sf::Color kWhite = sf::Color::White;

struct Scorch { float x, z, r, life, decay; float bubbleT[3]; float bx[3], bz[3]; };
struct Pillar { float x, z, height, maxHeight, life; int t; };
struct Spike { float x, z, grow, life; int t; float seed; };

class ValeriaFx final : public LabFx {
public:
    void update(LabContext& c) override;
    void drawFloor(Canvas& ctx, LabContext& c) override;
    void drawBackground(Canvas& ctx, LabContext& c) override { drawPass(ctx, c, false); }
    void drawForeground(Canvas& ctx, LabContext& c) override { drawPass(ctx, c, true); }

private:
    void drawPass(Canvas& ctx, LabContext& c, bool front);
    void spawnEmber(float x, float y, float z, bool fast);
    void spawnDrop(float x, float y, float z, float dir, bool burstDrop);
    void spawnSmoke(float x, float y, float z, bool big, bool ash = false);
    void drawFlame(Canvas& ctx, float x, float y, float w, float h, float t, float a);

    Kit k;
    std::vector<Scorch> scorches;
    std::vector<Pillar> pillars;
    std::vector<Spike> spikes;
    Once magmaHit, eruptRumble, eruptBlast, spikeHit, ashThrow, phoenixFlare;
    float flashAlpha = 0;
    float heat = 0;      // aquecimento da luva no Soco Magmático
    float wings = 0;     // abertura das asas de fênix
    float heatwave = 0;  // intensidade da aura do Escudo de Calor
    float magmaGx = 0, magmaGy = 0, magmaGz = 0;
    // Native geometry/timing stays shared; pigments follow the selected costume.
    sf::Color kYellow=col("#facc15"),kOrange=col("#ea580c"),kRed=col("#dc2626"),kDeep=col("#991b1b"),
              kAsh=col("#3f3f46"),kAshDark=col("#1c1917"),kAshLight=col("#71717a"),kHot=col("#ff4500");
};

void ValeriaFx::spawnEmber(float x, float y, float z, bool fast) {
    P e; e.kind = Kind::Glow;
    e.x = x + rr(-22, 22); e.y = y + rr(-18, 18); e.z = z + rr(-20, 20);
    e.vx = rr(-1, 1) * (fast ? 2.5f : 1.0f); e.vy = rr(2.4f, fast ? 8.4f : 5.9f) * 0.6f; e.vz = rr(-1, 1) * (fast ? 2.2f : 1.1f);
    e.r = rr(1.4f, 4.2f); e.decay = rr(0.02f, 0.045f); e.c0 = kWhite; e.c1 = rnd() < 0.5f ? kYellow : kHot; e.drag = 0.98f;
    k.add(e);
}

void ValeriaFx::spawnDrop(float x, float y, float z, float dir, bool burstDrop) {
    P d; d.kind = Kind::Drop;
    const float spd = burstDrop ? rr(3, 7.5f) : rr(0.8f, 3);
    d.x = x + rr(-10, 10); d.y = y + rr(-8, 8); d.z = z + rr(-10, 10);
    d.vx = dir * spd * (burstDrop ? rr(-0.2f, 0.8f) : 1.0f); d.vy = (burstDrop ? 2.0f : -0.5f) + rr(0, 2.5f); d.vz = (burstDrop ? 4.5f : 1.0f) + rr(0, 4.5f);
    d.r = (burstDrop ? 3.8f : 2.8f) + rr(0, 3.2f); d.gravity = 0.44f; d.decay = rr(0.012f, 0.025f);
    d.c0 = kWhite; d.c1 = kOrange; d.seed = 1; // seed 1 = gota de lava (vira poça ao tocar o chão)
    k.add(d);
}

void ValeriaFx::spawnSmoke(float x, float y, float z, bool big, bool ash) {
    P s; s.kind = Kind::Smoke;
    const float spd = big ? 3.2f : 1.4f;
    s.x = x + rr(-1, 1) * (big ? 18 : 10); s.y = y + rr(-1, 1) * (big ? 14 : 8); s.z = z + rr(-1, 1) * (big ? 18 : 10);
    s.vx = rr(-0.5f, 0.5f) * spd; s.vy = rr(1.4f, big ? 5.2f : 3.4f) * 0.5f; s.vz = rr(-0.5f, 0.5f) * spd;
    s.r = (big ? 20 : 12) + rr(0, big ? 20 : 10); s.growth = big ? 0.75f : 0.4f; s.rot = rnd() * PI * 2; s.rotSpeed = rr(-0.04f, 0.04f);
    s.decay = big ? rr(0.014f, 0.024f) : rr(0.022f, 0.036f); s.drag = 0.97f;
    const bool dark = rnd() > 0.3f;
    s.c1 = dark ? kAshDark : kAsh; s.c0 = dark ? col("#44403c") : kAshLight; s.c2 = col("#09090b");
    if (ash) { s.c1 = rnd() < 0.5f ? kAsh : col("#52525b"); s.c0 = kAshLight; }
    k.add(s);
}

void ValeriaFx::update(LabContext& c) {
    auto& f = c.fighter;
    const std::string& st = f.state;
    const float p = c.progress();
    const float bX = f.body.offsetX, bY = f.body.offsetY;
    const float dist = rivalDistOf(c);
    const bool paused = c.isPaused;
    const unsigned skin=c.entity?c.entity->skinId:0;
    if(skin==2){kYellow=col("#ffd56b");kOrange=col("#e34d6d");kRed=col("#b83274");kDeep=col("#5b194e");kHot=col("#ff708e");kAsh=col("#49304c");kAshDark=col("#241c2e");kAshLight=col("#8f6e99");}
    else if(skin==3){kYellow=col("#ffbd63");kOrange=col("#df3036");kRed=col("#ad1633");kDeep=col("#50172a");kHot=col("#f55b42");kAsh=col("#33232d");kAshDark=col("#17151e");kAshLight=col("#7f6474");}
    else {kYellow=col("#facc15");kOrange=col("#ea580c");kRed=col("#dc2626");kDeep=col("#991b1b");kHot=col("#ff4500");kAsh=col("#3f3f46");kAshDark=col("#1c1917");kAshLight=col("#71717a");}

    // ---------------- SOCO MAGMÁTICO (Combat Lab) ----------------
    const bool magma = st == "valeria_magma_punch";
    {
        const Glove& g = f.isLeft ? f.parts.gloveL : f.parts.gloveR;
        magmaGx = g.x + bX; magmaGy = g.y + bY; magmaGz = g.z + 25;
        const float dir = f.isLeft ? -1.0f : 1.0f;
        if (magma && !paused) {
            if (p < 0.42f) {
                const float hi = p / 0.42f;
                heat = hi;
                if (p > 0.1f && rnd() < 0.85f * hi) {
                    P s; s.kind = Kind::Spark; s.x = magmaGx + rr(-12, 12); s.y = magmaGy + rr(-12, 12); s.z = magmaGz + rr(-12, 12);
                    const float a = rnd() * PI * 2, sp = rr(4.5f, 13);
                    s.vx = std::cos(a) * sp; s.vy = std::sin(a) * sp + 1.8f; s.vz = rr(-0.5f, 0.5f) * sp; s.decay = rr(0.04f, 0.07f);
                    s.c0 = kWhite; s.c1 = kYellow; s.width = 1.4f; s.drag = 0.92f;
                    k.add(s);
                }
                if (rnd() < 0.9f * hi) spawnEmber(magmaGx, magmaGy, magmaGz, false);
                if (rnd() < 0.5f * hi) spawnDrop(magmaGx, magmaGy, magmaGz, dir, false);
                if (p > 0.2f && rnd() < 0.35f) spawnSmoke(magmaGx, magmaGy, magmaGz, false);
            } else if (p < 0.68f) {
                heat = 1;
                if (rnd() < 0.75f) { spawnSmoke(magmaGx, magmaGy, magmaGz, false); spawnDrop(magmaGx, magmaGy, magmaGz, dir, false); }
            } else {
                heat = std::max(0.0f, 1.0f - (p - 0.68f) / 0.32f);
                if (rnd() < 0.45f) spawnSmoke(magmaGx, magmaGy, magmaGz, false);
                if (rnd() < 0.35f) spawnEmber(magmaGx, magmaGy, magmaGz, false);
            }
        }
        if (!magma && !paused) heat = std::max(0.0f, heat - 0.08f);
        if (magmaHit(magma, p, 0.42f)) {
            flashAlpha = 0.85f;
            P r1; r1.kind = Kind::Ring; r1.x = magmaGx; r1.y = magmaGy; r1.z = magmaGz; r1.r = 14; r1.maxR = 130; r1.decay = 0.038f;
            r1.c1 = kOrange; r1.c0 = kWhite; r1.width = 6;
            P r2 = r1; r2.r = 8; r2.maxR = 90; r2.decay = 0.045f; r2.c1 = kYellow; r2.c0 = col("#ffedd5"); r2.width = 4.5f;
            k.add(r1); k.add(r2);
            for (int i = 0; i < 9; i++) spawnSmoke(magmaGx, magmaGy, magmaGz, true);
            burst(k, magmaGx, magmaGy, magmaGz, kYellow, kOrange, 14, 1.2f);
            for (int i = 0; i < 6; i++) { spawnDrop(magmaGx, magmaGy, magmaGz, dir, true); spawnEmber(magmaGx, magmaGy, magmaGz, true); }
        }
    }

    // ---------------- ERUPÇÃO SOLAVANCO ----------------
    const bool erupt = st == "valeria_volcanic_erupt";
    if (erupt && !paused && p < 0.55f) {
        // O chão sob o rival racha e fumega antes de explodir.
        if (rnd() < 0.5f) spawnSmoke(rr(-30, 30), 4, dist + rr(-25, 25), false);
        if (rnd() < 0.6f) spawnEmber(0, 4, dist, false);
        if (static_cast<int>(p * 100) % 9 == 0 && rnd() < 0.5f) {
            P fr; fr.kind = Kind::FloorRing; fr.x = 0; fr.z = dist; fr.r = 10; fr.maxR = 70; fr.decay = 0.04f; fr.c1 = kOrange; fr.c0 = kYellow; fr.width = 4;
            k.add(fr);
        }
    }
    if (eruptRumble(erupt, p, 0.12f)) {
        Vec3 w = Physics3D::localToWorld(0, 0, dist, c.entity->worldPos, c.entity->facing);
        scorches.push_back({w.x, w.z, 10, 1.0f, 0.004f, {0.8f, 0.6f, 0.4f}, {rr(-14, 14), rr(-14, 14), rr(-14, 14)}, {rr(-6, 6), rr(-6, 6), rr(-6, 6)}});
    }
    if (eruptBlast(erupt, p, 0.55f)) {
        Vec3 w = Physics3D::localToWorld(0, 0, dist, c.entity->worldPos, c.entity->facing);
        pillars.push_back({w.x, w.z, 0, 240, 1.0f, 0});
        burst(k, 0, 20, dist, kYellow, kOrange, 26, 1.6f);
        for (int i = 0; i < 16; i++) {
            // Bombas vulcânicas em parábola
            P b; b.kind = Kind::Drop; b.x = rr(-10, 10); b.y = rr(40, 140); b.z = dist + rr(-10, 10);
            const float a = rnd() * PI * 2, sp = rr(2.5f, 6);
            b.vx = std::cos(a) * sp; b.vz = std::sin(a) * sp; b.vy = rr(4, 9); b.gravity = 0.38f; b.r = rr(4, 8); b.decay = 0.012f;
            b.c0 = kYellow; b.c1 = kOrange; b.seed = 1;
            k.add(b);
        }
        for (int i = 0; i < 10; i++) spawnSmoke(rr(-20, 20), rr(10, 60), dist, true);
    }

    // ---------------- ESTOCADA DE LAVA (pico de obsidiana sob o rival) ----------------
    const bool kick = st == "valeria_lava_kick_jab";
    if (spikeHit(kick, p, 0.38f)) {
        Vec3 w = Physics3D::localToWorld(0, 0, dist - 8, c.entity->worldPos, c.entity->facing);
        spikes.push_back({w.x, w.z, 0, 1.0f, 0, rnd() * 10});
        burst(k, 0, 40, dist - 8, kYellow, kRed, 18, 1.1f);
        for (int i = 0; i < 14; i++) {
            P s; s.kind = Kind::Shard; s.x = rr(-12, 12); s.y = rr(5, 50); s.z = dist - 8 + rr(-12, 12);
            s.vx = rr(-4, 4); s.vy = rr(3, 8); s.vz = rr(-4, 4); s.gravity = 0.35f; s.r = rr(3, 6); s.rotSpeed = rr(-0.3f, 0.3f);
            s.decay = 0.02f; s.c1 = rnd() < 0.5f ? col("#27272a") : col("#52525b"); s.floorBounce = true;
            k.add(s);
        }
    }
    if (kick && !paused && p < 0.38f && rnd() < 0.7f) {
        // A perna aquece e deixa um rastro de brasas.
        spawnEmber(bX + rr(-10, 10), 20 + bY, 10, false);
    }

    // ---------------- ESCUDO DE CALOR ----------------
    const int hw = c.entity ? c.entity->status.heatwave : 0;
    if (!paused) heatwave += ((hw > 0 || st == "valeria_heatwave_guard" ? 1.0f : 0.0f) - heatwave) * 0.1f;
    if (heatwave > 0.05f && !paused) {
        if (rnd() < 0.5f * heatwave) spawnEmber(bX + rr(-35, 35), bY + rr(0, 60), rr(-20, 20), false);
        if (rnd() < 0.08f * heatwave) {
            P fr; fr.kind = Kind::FloorRing; fr.x = bX; fr.z = 0; fr.r = 20; fr.maxR = 95; fr.decay = 0.03f; fr.c1 = kHot; fr.c0 = kYellow; fr.width = 3.5f;
            k.add(fr);
        }
        if (rnd() < 0.06f * heatwave) {
            P r; r.kind = Kind::Ring; r.x = bX; r.y = bY + 60; r.z = 0; r.r = 30; r.maxR = 85; r.decay = 0.05f; r.c1 = col("#fb923c"); r.c0 = col("#fef3c7"); r.width = 2.5f;
            k.add(r);
        }
    }

    // ---------------- CORTINA DE CINZAS ----------------
    const bool ash = st == "valeria_ash_cloud";
    if (ash && !paused && p < 0.45f && rnd() < 0.8f) {
        // Rajada de cinzas saindo das mãos em direção ao rival
        P s; s.kind = Kind::Smoke; s.x = bX + rr(-10, 10); s.y = bY + 70 + rr(-10, 10); s.z = 20;
        s.vx = rr(-0.6f, 0.6f); s.vy = rr(-0.2f, 0.6f); s.vz = dist / rr(24, 34); s.r = rr(8, 14); s.growth = 0.45f; s.decay = 0.022f;
        s.rot = rnd() * PI * 2; s.rotSpeed = rr(-0.05f, 0.05f); s.c1 = kAsh; s.c0 = kAshLight; s.c2 = col("#09090b");
        k.add(s);
        if (rnd() < 0.5f) spawnEmber(bX, bY + 70, 20 + rnd() * dist * 0.5f, false);
    }
    if (ashThrow(ash, p, 0.42f)) for (int i = 0; i < 14; i++) spawnSmoke(rr(-30, 30), rr(70, 130), dist + rr(-20, 20), true, true);
    if (c.rival && c.rival->status.blind > 0 && !paused && rnd() < 0.25f) {
        // Enquanto o rival está cego, cinzas giram em volta da cabeça dele.
        const float a = rnd() * PI * 2;
        P s; s.kind = Kind::Smoke; s.x = std::cos(a) * 28; s.y = rr(95, 135); s.z = dist + std::sin(a) * 20;
        s.vx = -std::sin(a) * 0.8f; s.vz = std::cos(a) * 0.8f; s.vy = 0.3f; s.r = rr(8, 14); s.growth = 0.2f; s.decay = 0.03f;
        s.c1 = kAsh; s.c0 = kAshLight; s.c2 = col("#09090b"); s.rot = rnd() * PI * 2;
        k.add(s);
    }

    // ---------------- REAGRUPAMENTO DAS BRASAS (asas de fênix) ----------------
    const bool phoenix = st == "valeria_phoenix_ember";
    if (!paused) wings += ((phoenix && p < 0.85f ? 1.0f : 0.0f) - wings) * 0.12f;
    if (phoenix && !paused && p < 0.75f) {
        // Brasas do ringue espiralam para dentro do corpo.
        for (int i = 0; i < 2; i++) {
            const float a = rnd() * PI * 2, rad = rr(70, 120);
            P e; e.kind = Kind::Glow; e.x = bX + std::cos(a) * rad; e.y = rr(5, 40); e.z = std::sin(a) * rad;
            e.vx = -std::cos(a) * rad / 28; e.vz = -std::sin(a) * rad / 28; e.vy = (bY + 70 - e.y) / 28;
            e.r = rr(2, 4); e.decay = 1.0f / 30; e.c1 = rnd() < 0.5f ? kYellow : kHot;
            k.add(e);
        }
    }
    if (phoenixFlare(phoenix, p, 0.6f)) {
        burst(k, bX, bY + 70, 0, kYellow, kHot, 20, 1.3f);
        P fr; fr.kind = Kind::FloorRing; fr.x = bX; fr.z = 0; fr.r = 20; fr.maxR = 130; fr.decay = 0.025f; fr.c1 = kHot; fr.c0 = kYellow; fr.width = 5;
        k.add(fr);
    }

    // ---------------- PASSIVAS ----------------
    if (c.entity && !paused) {
        const auto& lo = c.entity->loadout;
        if (lo.has(SkillId::ValeriaVolcanicRage) && c.entity->hp <= 35.0f && c.entity->hp > 0 && rnd() < 0.35f)
            spawnEmber(bX + rr(-30, 30), bY + rr(10, 110), rr(-10, 10), false);
        if (c.entity->coldMagma && rnd() < 0.05f) {
            P s; s.kind = Kind::Smoke; s.x = bX + rr(-20, 20); s.y = bY + rr(90, 120); s.z = rr(-10, 10); s.vy = 1.2f; s.r = 6; s.growth = 0.35f;
            s.decay = 0.03f; s.c1 = col("#cbd5e1"); s.c0 = col("#f8fafc"); s.c2 = col("#64748b");
            k.add(s);
        }
    }

    // ---------------- FÍSICA ----------------
    if (paused) return;
    flashAlpha = std::max(0.0f, flashAlpha - 0.06f);
    for (P& pp : k.ps) {
        if (pp.kind == Kind::Drop && pp.seed == 1 && pp.y <= 0.5f && pp.vy <= 0 && pp.life > 0) {
            Vec3 w = c.entity ? Physics3D::localToWorld(pp.x, 0.0f, pp.z, c.entity->worldPos, c.entity->facing) : Vec3{pp.x, 0.0f, pp.z};
            scorches.push_back({w.x, w.z, rr(10, 18), 1.0f, rr(0.012f, 0.02f), {0.6f, 0.4f, 0.2f}, {rr(-7, 7), rr(-7, 7), 0}, {rr(-3, 3), rr(-3, 3), 0}});
            pp.life = 0;
        }
    }
    k.update(false);
    for (auto& s : scorches) { s.life -= s.decay; }
    for (auto& pl : pillars) {
        pl.t++;
        if (pl.t < 12) pl.height = pl.maxHeight * std::sin(pl.t / 12.0f * PI * 0.5f);
        else if (pl.t > 34) { pl.height *= 0.86f; pl.life -= 0.06f; }
        if (pl.t % 2 == 0) {
            P d; d.kind = Kind::Drop;
            Vec3 l = c.entity ? Physics3D::worldToLocal(pl.x, 0.0f, pl.z, c.entity->worldPos, c.entity->facing) : Vec3{0, 0, 0};
            d.x = l.x + rr(-14, 14); d.y = pl.height * rr(0.6f, 1.0f); d.z = l.z + rr(-14, 14);
            const float a = rnd() * PI * 2; d.vx = std::cos(a) * rr(1, 3.5f); d.vz = std::sin(a) * rr(1, 3.5f); d.vy = rr(1, 4);
            d.gravity = 0.4f; d.r = rr(3, 6); d.decay = 0.02f; d.c0 = kYellow; d.c1 = kOrange; d.seed = 1;
            k.add(d);
        }
    }
    for (auto& sp : spikes) {
        sp.t++;
        sp.grow = sp.t < 6 ? sp.t / 6.0f : (sp.t > 40 ? std::max(0.0f, 1.0f - (sp.t - 40) / 14.0f) : 1.0f);
        if (sp.t > 54) sp.life = 0;
    }
    scorches.erase(std::remove_if(scorches.begin(), scorches.end(), [](const Scorch& s) { return s.life <= 0; }), scorches.end());
    pillars.erase(std::remove_if(pillars.begin(), pillars.end(), [](const Pillar& p) { return p.life <= 0; }), pillars.end());
    spikes.erase(std::remove_if(spikes.begin(), spikes.end(), [](const Spike& s) { return s.life <= 0; }), spikes.end());
}

void ValeriaFx::drawFloor(Canvas& ctx, LabContext& c) {
    const float time = nowMs() / 1000.0f;
    for (const Scorch& s : scorches) {
        Point3 q = c.projectWorldFloor(s.x, s.z);
        if (!q.ok()) continue;
        const float rx = s.r * q.scale * 1.8f, ry = rx * 0.38f;
        ctx.save();
        ctx.translate(q.x, q.y);
        ctx.globalAlpha = std::min(1.0f, s.life * 1.5f);
        ctx.fillStyle = cv::Style(kOrange);
        ctx.beginPath(); ctx.ellipse(0, 0, rx * 1.3f, ry * 1.3f, 0, 0, PI * 2); ctx.fill();
        ctx.fillStyle = "#09090b"; ctx.strokeStyle = "#09090b"; ctx.lineWidth = 1.8f;
        ctx.beginPath(); ctx.ellipse(0, 0, rx, ry, 0, 0, PI * 2); ctx.fill(); ctx.stroke();
        // Rachaduras incandescentes
        ctx.strokeStyle = cv::Style(kOrange); ctx.lineWidth = 1.6f; ctx.lineCap = "round";
        for (int i = 0; i < 5; i++) {
            const float a = i * 1.26f + s.x * 0.1f;
            ctx.beginPath(); ctx.moveTo(0, 0);
            ctx.lineTo(std::cos(a) * rx * 0.55f, std::sin(a) * ry * 0.55f);
            ctx.lineTo(std::cos(a + 0.3f) * rx * 0.95f, std::sin(a + 0.3f) * ry * 0.95f);
            ctx.stroke();
        }
        ctx.fillStyle = cv::Style(kRed); ctx.beginPath(); ctx.ellipse(0, 0, rx * 0.6f, ry * 0.6f, 0, 0, PI * 2); ctx.fill();
        ctx.fillStyle = cv::Style(kYellow); ctx.beginPath(); ctx.ellipse(0, 0, rx * 0.35f, ry * 0.35f, 0, 0, PI * 2); ctx.fill();
        ctx.fillStyle = "#ffffff"; ctx.beginPath(); ctx.ellipse(0, 0, rx * 0.15f, ry * 0.15f, 0, 0, PI * 2); ctx.fill();
        for (int b = 0; b < 3; b++) {
            if (s.life < s.bubbleT[b]) continue;
            const float br = (2.4f + b * 0.5f) * q.scale * (0.8f + 0.2f * std::sin(time * 8 + b));
            ctx.fillStyle = cv::Style(kHot); ctx.strokeStyle = "#09090b"; ctx.lineWidth = 1.0f;
            ctx.beginPath(); ctx.arc(s.bx[b] * q.scale, s.bz[b] * q.scale * 0.38f - br, br, 0, PI * 2); ctx.fill(); ctx.stroke();
        }
        ctx.restore();
    }
    k.drawFloor(ctx, c);
}

void ValeriaFx::drawFlame(Canvas& ctx, float x, float y, float w, float h, float t, float a) {
    // Língua de fogo cel-shaded em três camadas.
    const float sway = std::sin(t) * w * 0.35f;
    const sf::Color cols[] = {kDeep,kOrange,kYellow,col("#fff7ed")};
    for (int i = 0; i < 4; i++) {
        const float k = 1.0f - i * 0.22f;
        ctx.globalAlpha = a;
        ctx.beginPath();
        ctx.moveTo(x - w * k, y);
        ctx.quadraticCurveTo(x - w * k * 0.9f, y - h * k * 0.55f, x + sway * k, y - h * k);
        ctx.quadraticCurveTo(x + w * k * 0.9f, y - h * k * 0.55f, x + w * k, y);
        ctx.quadraticCurveTo(x, y + w * k * 0.4f, x - w * k, y);
        ctx.closePath();
        ctx.fillStyle = cv::Style(cols[i]);
        if (i == 0) { ctx.strokeStyle = "#09090b"; ctx.lineWidth = 1.6f; ctx.fill(); ctx.stroke(); }
        else ctx.fill();
    }
}

void ValeriaFx::drawPass(Canvas& ctx, LabContext& c, bool front) {
    auto& f = c.fighter;
    const float bodyZ = bodyDepthOf(c);
    const float time = nowMs() / 1000.0f;

    // Asas de fênix (atrás do corpo, em ambas as câmeras: ficam nas costas)
    if (wings > 0.02f) {
        Point3 back = c.project3D(f.body.offsetX, f.body.offsetY + 75, -12);
        if (back.ok() && (back.z < bodyZ) == front) {
            ctx.save();
            ctx.translate(back.x, back.y);
            const float s = back.scale * wings, flap = std::sin(time * 7) * 0.12f;
            for (int side = -1; side <= 1; side += 2) {
                ctx.save();
                ctx.scale(static_cast<float>(side), 1.0f);
                ctx.rotate(-0.25f + flap);
                for (int feather = 0; feather < 6; feather++) {
                    const float ang = -0.2f - feather * 0.22f, len = (125 - feather * 9) * s;
                    ctx.save();
                    ctx.rotate(ang);
                    auto g = ctx.createLinearGradient(0, 0, len, 0);
                    g->addColorStop(0, cv::rgba(255, 247, 237, 0.9f).color);
                    g->addColorStop(0.4f, alpha(kYellow,.85f).color);
                    g->addColorStop(0.8f, alpha(kOrange,.7f).color);
                    g->addColorStop(1, alpha(kDeep,0).color);
                    ctx.fillStyle = g;
                    ctx.beginPath();
                    ctx.moveTo(0, 0);
                    ctx.quadraticCurveTo(len * 0.5f, -16 * s - std::sin(time * 10 + feather) * 4 * s, len, 0);
                    ctx.quadraticCurveTo(len * 0.5f, 12 * s, 0, 0);
                    ctx.fill();
                    ctx.restore();
                }
                ctx.restore();
            }
            ctx.restore();
        }
    }

    // Aura do Escudo de Calor: línguas de fogo em volta da base
    if (heatwave > 0.05f) {
        for (int i = 0; i < 8; i++) {
            const float a = i * PI / 4 + time * 0.8f;
            Point3 q = c.project3D(f.body.offsetX + std::cos(a) * 40, 0, std::sin(a) * 26);
            if (!q.ok() || (q.z < bodyZ) != front) continue;
            ctx.save();
            const float h = (26 + 10 * std::sin(time * 9 + i * 1.7f)) * q.scale * heatwave;
            drawFlame(ctx, q.x, q.y, 9 * q.scale, h, time * 10 + i, 0.85f * heatwave);
            ctx.restore();
        }
    }

    // Coluna de lava da erupção
    for (const Pillar& pl : pillars) {
        Point3 b = c.projectWorldFloor(pl.x, pl.z), t = c.projectWorldPos(pl.x, pl.height, pl.z);
        if (!b.ok() || !t.ok() || (b.z < bodyZ) != front) continue;
        const float w = 34 * b.scale, wt = 24 * t.scale, wob = std::sin(time * 30) * 3 * b.scale;
        ctx.save();
        ctx.globalAlpha = std::clamp(pl.life, 0.0f, 1.0f);
        ctx.beginPath();
        ctx.moveTo(b.x - w, b.y);
        ctx.bezierCurveTo(b.x - w * 0.7f + wob, (b.y + t.y) / 2, t.x - wt * 1.2f - wob, t.y + 30 * t.scale, t.x - wt, t.y);
        ctx.lineTo(t.x + wt, t.y);
        ctx.bezierCurveTo(t.x + wt * 1.2f + wob, t.y + 30 * t.scale, b.x + w * 0.7f - wob, (b.y + t.y) / 2, b.x + w, b.y);
        ctx.closePath();
        auto g = ctx.createLinearGradient(b.x - w, 0, b.x + w, 0);
        g->addColorStop(0,kDeep);g->addColorStop(0.25f,kOrange);g->addColorStop(0.5f,kYellow);g->addColorStop(0.75f,kOrange);g->addColorStop(1,kDeep);
        ctx.fillStyle = g; ctx.strokeStyle = "#09090b"; ctx.lineWidth = 3.0f;
        ctx.fill(); ctx.stroke();
        // Veios de magma descendo
        ctx.strokeStyle = "#fff7ed"; ctx.lineWidth = 2.0f * b.scale; ctx.lineCap = "round";
        for (int i = 0; i < 3; i++) {
            const float u = std::fmod(time * 1.8f + i * 0.33f, 1.0f);
            const float x0 = b.x + (i - 1) * w * 0.4f, y0 = t.y + (b.y - t.y) * u;
            ctx.beginPath(); ctx.moveTo(x0, y0); ctx.lineTo(x0 + std::sin(u * 9) * 3, y0 + 22 * b.scale); ctx.stroke();
        }
        // Topo borbulhante
        ctx.fillStyle = cv::Style(kYellow); ctx.strokeStyle = "#09090b"; ctx.lineWidth = 2.0f;
        for (int i = 0; i < 5; i++) {
            const float bx = t.x + (i - 2) * wt * 0.5f, by = t.y - std::abs(std::sin(time * 12 + i)) * 10 * t.scale;
            ctx.beginPath(); ctx.arc(bx, by, (8 + (i % 2) * 4) * t.scale, 0, PI * 2); ctx.fill(); ctx.stroke();
        }
        ctx.restore();
    }

    // Pico de obsidiana
    for (const Spike& sp : spikes) {
        Point3 b = c.projectWorldFloor(sp.x, sp.z);
        if (!b.ok() || (b.z < bodyZ) != front || sp.grow <= 0.01f) continue;
        ctx.save();
        ctx.translate(b.x, b.y);
        const float s = b.scale * sp.grow;
        struct Sp { float dx, h, w, tilt; };
        const Sp list[] = {{0, 120, 26, 0}, {-22, 70, 18, -0.35f}, {22, 80, 18, 0.3f}, {-38, 40, 12, -0.6f}, {36, 46, 12, 0.55f}};
        for (const Sp& e : list) {
            ctx.save();
            ctx.translate(e.dx * b.scale, 0);
            ctx.rotate(e.tilt);
            ctx.beginPath(); ctx.moveTo(-e.w * s, 0); ctx.lineTo(-e.w * 0.15f * s, -e.h * s); ctx.lineTo(e.w * 0.25f * s, -e.h * 0.95f * s); ctx.lineTo(e.w * s, 0);
            ctx.closePath();
            ctx.fillStyle = "#27272a"; ctx.strokeStyle = "#09090b"; ctx.lineWidth = 2.4f;
            ctx.fill(); ctx.stroke();
            ctx.beginPath(); ctx.moveTo(-e.w * 0.5f * s, 0); ctx.lineTo(-e.w * 0.1f * s, -e.h * 0.9f * s); ctx.lineTo(0, 0); ctx.closePath();
            ctx.fillStyle = "#52525b"; ctx.fill();
            // Veios de lava brilhando
            ctx.strokeStyle = "#f97316"; ctx.lineWidth = 2.0f * b.scale;
            ctx.beginPath(); ctx.moveTo(0, -e.h * 0.1f * s); ctx.lineTo(e.w * 0.2f * s, -e.h * 0.45f * s); ctx.lineTo(-e.w * 0.05f * s, -e.h * 0.75f * s); ctx.stroke();
            ctx.restore();
        }
        ctx.restore();
    }

    // Aura incandescente da luva e flash do Soco Magmático (frente)
    if (front && (heat > 0.02f || flashAlpha > 0.01f)) {
        Point3 g = c.project3D(magmaGx, magmaGy, magmaGz);
        if (g.ok()) {
            ctx.save();
            ctx.globalCompositeOperation = "lighter";
            if (heat > 0.02f) {
                const float rad = (55 + std::sin(time * 40) * 7) * g.scale * heat;
                auto gr = ctx.createRadialGradient(g.x, g.y, rad * 0.1f, g.x, g.y, rad);
                gr->addColorStop(0, cv::rgba(255, 255, 255, 0.9f * heat).color);
                gr->addColorStop(0.25f,alpha(kYellow,.75f*heat).color);
                gr->addColorStop(0.6f,alpha(kHot,.5f*heat).color);
                gr->addColorStop(1,alpha(kDeep,0).color);
                ctx.fillStyle = gr;
                ctx.beginPath(); ctx.arc(g.x, g.y, rad, 0, PI * 2); ctx.fill();
            }
            if (flashAlpha > 0.01f) {
                auto gr = ctx.createRadialGradient(g.x, g.y, 0, g.x, g.y, 95 * g.scale);
                gr->addColorStop(0, cv::rgba(255, 255, 255, 0.95f * flashAlpha).color);
                gr->addColorStop(0.35f,alpha(kYellow,.8f*flashAlpha).color);
                gr->addColorStop(0.7f,alpha(kOrange,.5f*flashAlpha).color);
                gr->addColorStop(1,alpha(kRed,0).color);
                ctx.fillStyle = gr;
                ctx.beginPath(); ctx.arc(g.x, g.y, 95 * g.scale, 0, PI * 2); ctx.fill();
            }
            ctx.restore();
        }
    }
    k.draw(ctx, c, front, bodyZ);
}

} // namespace

std::unique_ptr<LabFx> makeValeriaFx() { return std::make_unique<ValeriaFx>(); }

} // namespace KoRama::vfx
