// Lady Espinha. No index.html os golpes dela eram uma garra 2D e pontos de areia; aqui: a Garra del
// Cacto abre dois cortes verdes em arco com espinhos voando a cada tapa; o Gran Salto Luchador levanta
// poeira na decolagem, deixa um rastro de estrelas no ar e afunda o chão no impacto; o Velo de las
// Espinas gira uma coroa de espinhos; a Estocada de Mandacaru dispara um espinho gigante; o Viento
// del Desierto ergue um redemoinho de areia que vai até o rival; e as Lágrimas de Frida abrem dálias
// em volta dela enquanto lágrimas douradas caem.
#include "fx_kit.hpp"

namespace KoRama::vfx {

namespace {

using namespace kit;

const sf::Color kCactus = col("#16a34a"), kCactusDeep = col("#14532d"), kSand = col("#d6b36a"), kSandDeep = col("#92400e"),
                kDahlia = col("#e11d48"), kMagenta = col("#db2777"), kGold = col("#facc15"), kSpine = col("#fef3c7");

struct Slash { float side, life; };
struct Crack { float x, z, r, life; float a[7]; };

class EspinhaFx final : public LabFx {
public:
    void update(LabContext& c) override;
    void drawFloor(Canvas& ctx, LabContext& c) override;
    void drawBackground(Canvas& ctx, LabContext& c) override { drawPass(ctx, c, false); }
    void drawForeground(Canvas& ctx, LabContext& c) override { drawPass(ctx, c, true); }

private:
    void drawPass(Canvas& ctx, LabContext& c, bool front);
    void dust(float x, float z, int n, float power);

    // The skin changes the cosmetic material, not the world-space trajectories.
    sf::Color kCactus=col("#16a34a"),kCactusDeep=col("#14532d"),kSand=col("#d6b36a"),kSandDeep=col("#92400e"),kDahlia=col("#e11d48"),kMagenta=col("#db2777"),kGold=col("#facc15"),kSpine=col("#fef3c7");
    Kit k;
    Once slap1, slap2, takeoff, impact, needle, windGust, fridaBloom;
    std::vector<Slash> slashes;
    std::vector<Crack> cracks;
    float veil = 0, frida = 0, tornado = 0;
    float rivalD = 180;
};

void EspinhaFx::dust(float x, float z, int n, float power) {
    for (int i = 0; i < n; i++) {
        const float a = rnd() * PI * 2;
        P s; s.kind = Kind::Smoke; s.x = x + std::cos(a) * 10; s.y = rr(2, 10); s.z = z + std::sin(a) * 10;
        s.vx = std::cos(a) * rr(1.5f, 4) * power; s.vz = std::sin(a) * rr(1.5f, 4) * power; s.vy = rr(0.3f, 1.4f); s.drag = 0.92f;
        s.r = rr(8, 14) * power; s.growth = 0.35f; s.decay = rr(0.02f, 0.035f); s.rot = rnd() * PI * 2;
        s.c1 = kSand; s.c0 = col("#fde68a"); s.c2 = kSandDeep;
        k.add(s);
    }
}

void EspinhaFx::update(LabContext& c) {
    auto& f = c.fighter;
    const unsigned skin=c.entity?std::min(3u,c.entity->skinId):0;
    static const char* materials[4][8]={
        {"#16a34a","#14532d","#d6b36a","#92400e","#e11d48","#db2777","#facc15","#fef3c7"},
        {"#dfb54c","#594527","#eed38b","#927335","#c7384b","#ffdb73","#fff0a0","#f9dfa0"},
        {"#ed861f","#743f32","#eab46a","#8b5341","#ed751e","#ffb234","#ffe36d","#fff1d7"},
        {"#d79b52","#5b4937","#d7b181","#875a3b","#de6554","#eeb85d","#efcc6f","#d8c1a0"}};
    sf::Color* palette[]={&kCactus,&kCactusDeep,&kSand,&kSandDeep,&kDahlia,&kMagenta,&kGold,&kSpine};
    for(unsigned i=0;i<8;++i)*palette[i]=col(materials[skin][i]);
    const std::string& st = f.state;
    const float p = c.progress();
    const float bX = f.body.offsetX, bY = f.body.offsetY;
    const bool paused = c.isPaused;
    rivalD = rivalDistOf(c);
    const float hitZ = rivalD - 20, chest = 85;

    // ---------------- LA GARRA DEL CACTO (dois tapas) ----------------
    const bool slap = st == "espinha_nopal_slap";
    auto slapHit = [&](float side) {
        slashes.push_back({side, 1.0f});
        burst(k, side * 10, chest, hitZ, kSpine, kCactus, 10, 0.9f);
        for (int i = 0; i < 12; i++) {
            P s; s.kind = Kind::Spine; s.x = side * 10; s.y = chest + rr(-10, 10); s.z = hitZ;
            const float a = rnd() * PI * 2, sp = rr(4, 9);
            s.vx = std::cos(a) * sp; s.vy = std::sin(a) * sp * 0.7f + 1; s.vz = rr(-3, 3); s.gravity = 0.2f; s.r = rr(7, 11); s.decay = 0.03f;
            s.c1 = kSpine;
            k.add(s);
        }
    };
    if (slap1(slap, p, 0.35f)) slapHit(-1);
    if (slap2(slap, p, 0.58f)) slapHit(1);

    // ---------------- GRAN SALTO LUCHADOR ----------------
    const bool lucha = st == "espinha_lucha_flyer";
    if (takeoff(lucha, p, 0.12f)) {
        dust(bX, 0, 12, 1.0f);
        P fr; fr.kind = Kind::FloorRing; fr.x = bX; fr.r = 10; fr.maxR = 80; fr.decay = 0.04f; fr.c1 = kSand; fr.c0 = col("#fef3c7"); fr.width = 4;
        k.add(fr);
    }
    if (lucha && !paused && p > 0.15f && p < 0.8f) {
        // Rastro de estrelas e confete de lucha libre no ar
        P s; s.kind = Kind::Star; s.x = bX + rr(-15, 15); s.y = bY + rr(40, 110); s.z = rr(-10, 10); s.vy = rr(-0.5f, 0.5f); s.r = rr(4, 7);
        s.rotSpeed = rr(-0.2f, 0.2f); s.decay = 0.035f; s.c1 = rnd() < 0.5f ? kGold : kMagenta;
        k.add(s);
        if (rnd() < 0.6f) {
            P cf; cf.kind = Kind::Shard; cf.x = bX + rr(-20, 20); cf.y = bY + rr(30, 120); cf.z = rr(-15, 15); cf.vx = rr(-1, 1); cf.vy = rr(-0.5f, 1);
            cf.gravity = 0.05f; cf.r = rr(2, 3.5f); cf.rotSpeed = rr(-0.3f, 0.3f); cf.decay = 0.02f;
            const sf::Color pal[] = {col("#22c55e"), col("#ffffff"), col("#ef4444"), col("#3b82f6"), kGold};
            cf.c1 = pal[static_cast<int>(rnd() * 5) % 5];
            k.add(cf);
        }
    }
    if (impact(lucha, p, 0.82f)) {
        Vec3 impW = Physics3D::localToWorld(0, 0, rivalD, c.entity->worldPos, c.entity->facing);
        Crack cr{impW.x, impW.z, 0, 1.0f, {}};
        for (float& a : cr.a) a = rnd() * PI * 2;
        cracks.push_back(cr);
        dust(0, rivalD, 18, 1.5f);
        burst(k, 0, 60, rivalD, kGold, kMagenta, 20, 1.5f);
        P fr; fr.kind = Kind::FloorRing; fr.z = rivalD; fr.r = 15; fr.maxR = 140; fr.decay = 0.025f; fr.c1 = kGold; fr.c0 = sf::Color::White; fr.width = 6;
        k.add(fr);
        for (int i = 0; i < 5; i++) {
            P s; s.kind = Kind::Star; s.x = rr(-30, 30); s.y = rr(100, 140); s.z = rivalD + rr(-20, 20); s.vy = rr(1, 3); s.vx = rr(-2, 2);
            s.r = rr(7, 10); s.rotSpeed = 0.2f; s.decay = 0.02f; s.gravity = 0.08f; s.c1 = kGold;
            k.add(s);
        }
    }

    // ---------------- VELO DE LAS ESPINAS ----------------
    const bool veilOn = (c.entity && c.entity->status.thornVeil > 0) || st == "espinha_thorn_veil";
    if (!paused) veil += ((veilOn ? 1.0f : 0.0f) - veil) * 0.1f;

    // ---------------- ESTOCADA DE MANDACARU ----------------
    const bool jab = st == "espinha_needle_jab";
    if (needle(jab, p, 0.3f)) {
        P s; s.kind = Kind::Spine; s.x = bX; s.y = chest; s.z = 25; s.vz = (hitZ - 25) / 9.0f; s.r = 34; s.decay = 1.0f / 10.5f; s.c1 = kSpine;
        k.add(s);
        P g; g.kind = Kind::Glow; g.x = bX; g.y = chest; g.z = 25; g.vz = s.vz; g.r = 10; g.decay = s.decay; g.c1 = kCactus;
        k.add(g);
    }
    if (jab && !paused && p > 0.4f && p < 0.45f) burst(k, 0, chest, hitZ, kSpine, kCactus, 12, 1.0f);

    // ---------------- VIENTO DEL DESIERTO ----------------
    const bool wind = st == "espinha_desert_wind";
    if (!paused) tornado += ((wind && p < 0.85f ? 1.0f : 0.0f) - tornado) * 0.15f;
    if (wind && !paused) {
        // Redemoinho de areia girando em volta dela e avançando ao rival depois do golpe
        const float centerZ = p < 0.4f ? 0.0f : (rivalD * std::min(1.0f, (p - 0.4f) / 0.3f));
        for (int i = 0; i < 3; i++) {
            const float a = rnd() * PI * 2, h = rr(0, 120), rad = 15 + h * 0.35f;
            P s; s.kind = rnd() < 0.7f ? Kind::Drop : Kind::Leaf; s.x = (p < 0.4f ? bX : 0) + std::cos(a) * rad; s.y = h; s.z = centerZ + std::sin(a) * rad;
            s.vx = -std::sin(a) * 4; s.vz = std::cos(a) * 4; s.vy = rr(0.5f, 2); s.r = rr(1.5f, 3); s.decay = 0.05f;
            s.c1 = kSand; s.c0 = col("#fef3c7"); s.c2 = kSandDeep; s.seed = rnd() * 10; s.rotSpeed = 0.3f;
            k.add(s);
        }
    }
    if (windGust(wind, p, 0.4f)) dust(0, rivalD, 14, 1.3f);

    // ---------------- LÁGRIMAS DE FRIDA ----------------
    const bool tears = st == "espinha_frida_tears";
    if (!paused) frida += ((tears && p < 0.9f ? 1.0f : 0.0f) - frida) * 0.1f;
    if (tears && !paused && rnd() < 0.5f) {
        P d; d.kind = Kind::Drop; d.x = bX + rr(-8, 8); d.y = bY + 108; d.z = 12; d.vx = rr(-0.6f, 0.6f); d.vy = 0.5f; d.gravity = 0.12f;
        d.r = rr(2, 3); d.decay = 0.02f; d.c1 = kGold; d.c0 = sf::Color::White;
        k.add(d);
        P pe; pe.kind = Kind::Leaf; pe.x = bX + rr(-50, 50); pe.y = bY + rr(60, 140); pe.z = rr(-30, 30); pe.vy = -0.6f;
        pe.r = rr(4, 6); pe.rotSpeed = 0.1f; pe.decay = 0.015f; pe.c1 = rnd() < 0.5f ? kDahlia : kMagenta; pe.c2 = col("#831843"); pe.seed = rnd() * 10;
        k.add(pe);
    }
    if (fridaBloom(tears, p, 0.5f)) burst(k, bX, bY + 80, 0, kGold, kDahlia, 12, 1.0f);

    // Espinas de Lucha: espinhos espetam quem bate nela (cada vez que ela sofre um golpe).
    if (c.entity && c.entity->loadout.has(SkillId::EspinhaThornRecoil) && st == "hit" && !paused && rnd() < 0.4f) {
        P s; s.kind = Kind::Spine; s.x = bX + rr(-20, 20); s.y = bY + rr(50, 100); s.z = 15; s.vz = rr(3, 6); s.vx = rr(-2, 2); s.vy = rr(-1, 2);
        s.r = rr(6, 9); s.decay = 0.05f; s.c1 = kSpine;
        k.add(s);
    }

    if (!paused) {
        for (auto& s : slashes) s.life -= 0.07f;
        for (auto& cr : cracks) { cr.r += (1.0f - cr.r) * 0.3f; cr.life -= 0.008f; }
        slashes.erase(std::remove_if(slashes.begin(), slashes.end(), [](const Slash& s) { return s.life <= 0; }), slashes.end());
        cracks.erase(std::remove_if(cracks.begin(), cracks.end(), [](const Crack& s) { return s.life <= 0; }), cracks.end());
    }
    k.update(paused);
}

void EspinhaFx::drawFloor(Canvas& ctx, LabContext& c) {
    for (const Crack& cr : cracks) {
        Point3 q = c.projectWorldFloor(cr.x, cr.z);
        if (!q.ok()) continue;
        ctx.save();
        ctx.translate(q.x, q.y);
        ctx.scale(q.scale, q.scale * 0.38f);
        ctx.globalAlpha = std::min(1.0f, cr.life * 2);

        // 1. Cratera cel-shaded exterior
        ctx.beginPath(); ctx.ellipse(0, 0, 68 * cr.r, 68 * cr.r, 0, 0, PI * 2);
        ctx.fillStyle = "rgba(18, 9, 3, 0.70)"; ctx.fill();
        ctx.strokeStyle = "#080402"; ctx.lineWidth = 3.5f; ctx.stroke();

        // 2. Fissura abissal central
        ctx.beginPath(); ctx.ellipse(0, 0, 36 * cr.r, 36 * cr.r, 0, 0, PI * 2);
        ctx.fillStyle = "#020101"; ctx.fill();

        ctx.lineCap = "round";
        for (int pass = 0; pass < 3; pass++) {
            if (pass == 0) {
                ctx.strokeStyle = "#050201";
                ctx.lineWidth = 7.5f;
            } else if (pass == 1) {
                ctx.strokeStyle = "#27150a";
                ctx.lineWidth = 4.0f;
            } else {
                ctx.strokeStyle = "#f59e0b"; // realce dourado cel-shaded
                ctx.lineWidth = 1.8f;
            }
            for (float a : cr.a) {
                ctx.beginPath(); ctx.moveTo(0, 0);
                ctx.lineTo(std::cos(a) * 45 * cr.r, std::sin(a) * 45 * cr.r);
                ctx.lineTo(std::cos(a + 0.25f) * 85 * cr.r, std::sin(a + 0.25f) * 85 * cr.r);
                ctx.stroke();
            }
        }
        ctx.restore();
    }
    k.drawFloor(ctx, c);
}

void EspinhaFx::drawPass(Canvas& ctx, LabContext& c, bool front) {
    auto& f = c.fighter;
    const float bodyZ = bodyDepthOf(c);
    const float time = nowMs() / 1000.0f;

    // Cortes verdes em arco da Garra del Cacto
    for (const Slash& s : slashes) {
        Point3 q = c.project3D(s.side * 10, 85, rivalD - 20);
        if (!q.ok() || (q.z < bodyZ) != front) continue;
        ctx.save();
        ctx.translate(q.x, q.y);
        ctx.rotate(s.side * 0.6f);
        ctx.globalAlpha = s.life;
        const float R = 55 * q.scale * (1.2f - s.life * 0.2f);
        for (int i = 0; i < 3; i++) {
            const float off = (i - 1) * 12 * q.scale;
            ctx.beginPath();
            ctx.arc(off, 0, R, -PI * 0.85f, -PI * 0.15f);
            ctx.arc(off, 6 * q.scale, R * 0.9f, -PI * 0.2f, -PI * 0.8f, true);
            ctx.closePath();
            ctx.fillStyle = cv::Style(i == 1 ? sf::Color::White : kCactus); ctx.strokeStyle = cv::Style(kCactusDeep); ctx.lineWidth = 1.6f;
            ctx.fill(); ctx.stroke();
        }
        ctx.restore();
    }

    // Coroa de espinhos girando do Velo de las Espinas
    if (veil > 0.03f) {
        for (int i = 0; i < 14; i++) {
            const float a = i * PI * 2 / 14 + time * 1.6f;
            const float h = 60 + std::sin(time * 3 + i) * 10;
            Point3 base = c.project3D(f.body.offsetX + std::cos(a) * 48, f.body.offsetY + h, std::sin(a) * 34);
            Point3 tip = c.project3D(f.body.offsetX + std::cos(a) * 70, f.body.offsetY + h + 6, std::sin(a) * 50);
            if (!base.ok() || !tip.ok() || (base.z < bodyZ) != front) continue;
            const float w = 6 * base.scale * veil;
            const float dx = tip.x - base.x, dy = tip.y - base.y, len = std::max(1.0f, std::hypot(dx, dy)), nx = -dy / len * w, ny = dx / len * w;
            ctx.save();
            ctx.globalAlpha = veil;
            ctx.beginPath(); ctx.moveTo(base.x + nx, base.y + ny); ctx.lineTo(base.x + dx * veil, base.y + dy * veil); ctx.lineTo(base.x - nx, base.y - ny);
            ctx.closePath();
            ctx.fillStyle = cv::Style(i % 2 ? kSpine : col("#bbf7d0")); ctx.strokeStyle = cv::Style(kCactusDeep); ctx.lineWidth = 1.4f;
            ctx.fill(); ctx.stroke();
            ctx.beginPath(); ctx.arc(base.x, base.y, w * 0.9f, 0, PI * 2); ctx.fillStyle = cv::Style(kCactus); ctx.fill(); ctx.stroke();
            ctx.restore();
        }
    }

    // Dálias da Frida em volta da cabeça
    if (frida > 0.03f) {
        for (int i = 0; i < 6; i++) {
            const float a = i * PI / 3 + time * 0.5f;
            Point3 q = c.project3D(f.body.offsetX + std::cos(a) * 45, f.body.offsetY + 115 + std::sin(a * 2) * 6, std::sin(a) * 30);
            if (!q.ok() || (q.z < bodyZ) != front) continue;
            ctx.save();
            ctx.translate(q.x, q.y);
            ctx.rotate(time + i);
            const float s = q.scale * frida;
            for (int layer = 0; layer < 2; layer++) {
                const int n = layer == 0 ? 10 : 7;
                const float L = (layer == 0 ? 13 : 8) * s;
                for (int j = 0; j < n; j++) {
                    ctx.save();
                    ctx.rotate(j * PI * 2 / n + layer * 0.3f);
                    ctx.beginPath(); ctx.moveTo(0, 0); ctx.quadraticCurveTo(L * 0.5f, -L * 0.35f, L, 0); ctx.quadraticCurveTo(L * 0.5f, L * 0.35f, 0, 0);
                    ctx.fillStyle = layer == 0 ? cv::Style(i % 2 ? kDahlia : kMagenta) : cv::Style(col("#fda4af"));
                    ctx.strokeStyle = "#4c0519"; ctx.lineWidth = 1.0f;
                    ctx.fill(); ctx.stroke();
                    ctx.restore();
                }
            }
            ctx.beginPath(); ctx.arc(0, 0, 3.5f * s, 0, PI * 2); ctx.fillStyle = cv::Style(kGold); ctx.fill();
            ctx.restore();
        }
    }

    // Funil do redemoinho de areia
    if (tornado > 0.03f && !front) {
        Point3 b = c.project3D(f.body.offsetX, 0, 0), t = c.project3D(f.body.offsetX, 130, 0);
        if (b.ok() && t.ok()) {
            ctx.save();
            ctx.globalAlpha = 0.45f * tornado;
            for (int i = 0; i < 6; i++) {
                const float u = i / 5.0f, y = b.y + (t.y - b.y) * u, rx = (18 + 45 * u) * b.scale, ry = rx * 0.3f;
                ctx.beginPath(); ctx.ellipse(b.x + std::sin(time * 6 + i) * 5 * b.scale, y, rx, ry, 0, time * 4 + i, time * 4 + i + PI * 1.4f);
                ctx.strokeStyle = cv::Style(kSand); ctx.lineWidth = 5 * b.scale; ctx.stroke();
            }
            ctx.restore();
        }
    }
    k.draw(ctx, c, front, bodyZ);
}

} // namespace

std::unique_ptr<LabFx> makeEspinhaFx() { return std::make_unique<EspinhaFx>(); }

} // namespace KoRama::vfx
