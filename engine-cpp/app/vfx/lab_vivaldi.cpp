// Maestra Vivaldi. Os golpes dela no index.html eram pontos coloridos voando até o alvo; aqui cada
// um ganhou uma animação própria: a Cadência Staccato estende uma pauta musical até o rival e as notas
// correm por ela a cada batida (cada vez maiores, em crescendo); o Inverno Allegro congela o rival num
// bloco de cristais que estilhaça; o Modo Quatro Estações gira pétalas, faíscas, folhas e flocos em
// volta dela; o Réquiem de Outono manda arcos sonoros roxos e faz chover folhas sobre o rival; a
// Estocada em Crescendo é uma lança de ectoplasma; e a Sonata de Primavera abre uma flor no chão.
#include "fx_kit.hpp"

namespace KoRama::vfx {

namespace {

using namespace kit;

struct Crystal { float x, y, z, h, w, tilt; };

class VivaldiFx final : public LabFx {
public:
    void update(LabContext& c) override;
    void drawFloor(Canvas& ctx, LabContext& c) override;
    void drawBackground(Canvas& ctx, LabContext& c) override { drawPass(ctx, c, false); }
    void drawForeground(Canvas& ctx, LabContext& c) override { drawPass(ctx, c, true); }

private:
    void drawPass(Canvas& ctx, LabContext& c, bool front);
    void note(float x, float y, float z, float vx, float vy, float vz, float r, sf::Color color);

    Kit k;
    Once staccatoHits[5], winterHit, jabHit, requiemWave, sonataBloom;
    float staff = 0;      // pauta estendida (0..1)
    float ice = 0;        // bloco de gelo no rival
    int iceT = 0;
    std::vector<Crystal> crystals;
    float flower = 0;     // flor da Sonata
    float seasons = 0;
    float spear = 0;      // lança de ectoplasma
    float rivalD = 180;
    // Geometry, depth sorting and native contact times remain unchanged.
    sf::Color kGhost=col("#5eead4"),kGhostDeep=col("#0f766e"),kPurple=col("#a855f7"),kPurpleDeep=col("#581c87"),
              kIce=col("#bae6fd"),kIceDeep=col("#0284c7"),kPink=col("#f9a8d4"),kGold=col("#facc15"),kLeaf=col("#ea580c"),kGreen=col("#4ade80"),
              crystalLight=col("#e0f2fe"),crystalMid=col("#7dd3fc"),crystalDark=col("#0ea5e9"),crystalEdge=col("#075985");
};

void VivaldiFx::note(float x, float y, float z, float vx, float vy, float vz, float r, sf::Color color) {
    P n; n.kind = Kind::Note; n.x = x; n.y = y; n.z = z; n.vx = vx; n.vy = vy; n.vz = vz; n.r = r; n.decay = 0.02f;
    n.c1 = color; n.c0 = sf::Color::White; n.seed = rnd() * 10; n.drag = 0.97f;
    k.add(n);
}

void VivaldiFx::update(LabContext& c) {
    auto& f = c.fighter;
    const std::string& st = f.state;
    const float p = c.progress();
    const float bX = f.body.offsetX, bY = f.body.offsetY;
    const bool paused = c.isPaused;
    rivalD = rivalDistOf(c);
    const float chestY = 85;
    const unsigned skin=c.entity?c.entity->skinId:0;
    if(skin==1){
        kGhost=col("#f5d67b");kGhostDeep=col("#8e4c28");kPurple=col("#c56952");kPurpleDeep=col("#672b34");
        kIce=col("#e4eee7");kIceDeep=col("#659a9d");kPink=col("#e6b58f");kGold=col("#ca8a04");kLeaf=col("#ad662f");kGreen=col("#a6b36a");
        crystalLight=col("#f5f2dc");crystalMid=col("#bad6ce");crystalDark=col("#659a9d");crystalEdge=col("#385b62");
    }else if(skin==2){
        kGhost=col("#d54460");kGhostDeep=col("#591c36");kPurple=col("#b67ba5");kPurpleDeep=col("#362437");
        kIce=col("#bfaec5");kIceDeep=col("#50475e");kPink=col("#de6786");kGold=col("#ccb17d");kLeaf=col("#98506c");kGreen=col("#7b9b83");
        crystalLight=col("#d6c5da");crystalMid=col("#8d7998");crystalDark=col("#50475e");crystalEdge=col("#211c30");
    }else if(skin==3){
        kGhost=col("#b3f4ff");kGhostDeep=col("#2c7ea6");kPurple=col("#c4a3ef");kPurpleDeep=col("#594a9e");
        kIce=col("#c9f7ff");kIceDeep=col("#3986bb");kPink=col("#c4a3ef");kGold=col("#d9efab");kLeaf=col("#a2dca6");kGreen=col("#72e8bb");
        crystalLight=col("#edffff");crystalMid=col("#91e3ed");crystalDark=col("#3986bb");crystalEdge=col("#2e4b81");
    }else{
        kGhost=col("#5eead4");kGhostDeep=col("#0f766e");kPurple=col("#a855f7");kPurpleDeep=col("#581c87");
        kIce=col("#bae6fd");kIceDeep=col("#0284c7");kPink=col("#f9a8d4");kGold=col("#facc15");kLeaf=col("#ea580c");kGreen=col("#4ade80");
        crystalLight=col("#e0f2fe");crystalMid=col("#7dd3fc");crystalDark=col("#0ea5e9");crystalEdge=col("#075985");
    }

    // ---------------- CADÊNCIA STACCATO ----------------
    const bool staccato = st == "vivaldi_cadence_staccato";
    if (!paused) staff += ((staccato && p < 0.92f ? 1.0f : 0.0f) - staff) * 0.15f;
    for (int i = 0; i < 5; i++) {
        if (staccatoHits[i](staccato, p, 0.2f + i * 0.15f)) {
            const float grow = 1.0f + i * 0.25f; // crescendo
            for (int n = 0; n < 3 + i; n++)
                note(bX + rr(-10, 10), bY + 80 + rr(-6, 12), 20, rr(-1, 1), rr(-0.5f, 0.8f), rivalD / rr(10, 14), rr(5, 7) * grow, i % 2 ? kPurple : kGhost);
            burst(k, rr(-10, 10), chestY + rr(-10, 15), rivalD - 15, kGhost, kPurple, 6 + i * 2, 0.6f + i * 0.15f);
        }
    }

    // ---------------- INVERNO ALLEGRO ----------------
    const bool winter = st == "vivaldi_winter_allegro";
    if (winter && !paused && p < 0.5f) {
        // Nevasca em espiral subindo pela luva do uppercut
        const Glove& g = f.isLeft ? f.parts.gloveL : f.parts.gloveR;
        const float a = p * 40;
        P fl; fl.kind = Kind::Flake; fl.x = g.x + bX + std::cos(a) * 18; fl.y = g.y + bY + rr(-10, 10); fl.z = g.z + std::sin(a) * 18;
        fl.vy = rr(1, 3); fl.r = rr(3, 6); fl.rotSpeed = 0.1f; fl.decay = 0.03f; fl.c0 = sf::Color::White; fl.c2 = kIceDeep;
        k.add(fl);
    }
    if (winterHit(winter, p, 0.45f)) {
        ice = 0.01f; iceT = 0;
        crystals.clear();
        for (int i = 0; i < 9; i++) {
            const float a = i * PI * 2 / 9 + rr(-0.2f, 0.2f);
            crystals.push_back({std::cos(a) * rr(16, 30), rr(0, 20), rivalD + std::sin(a) * rr(10, 20), rr(60, 130), rr(14, 22), std::cos(a) * 0.35f});
        }
        burst(k, 0, 100, rivalD, kIce, kIceDeep, 18, 1.2f);
        P fr; fr.kind = Kind::FloorRing; fr.z = rivalD; fr.r = 20; fr.maxR = 110; fr.decay = 0.02f; fr.c1 = kIce; fr.c0 = sf::Color::White; fr.width = 5;
        k.add(fr);
    }
    if (ice > 0 && !paused) {
        iceT++;
        ice = iceT < 8 ? iceT / 8.0f : (iceT < 70 ? 1.0f : 0.0f);
        if (iceT == 70) {
            // Estilhaça
            for (int i = 0; i < 26; i++) {
                P s; s.kind = Kind::Shard; s.x = rr(-30, 30); s.y = rr(10, 120); s.z = rivalD + rr(-20, 20);
                s.vx = rr(-4, 4); s.vy = rr(2, 7); s.vz = rr(-4, 4); s.gravity = 0.35f; s.r = rr(3, 7); s.rotSpeed = rr(-0.3f, 0.3f);
                s.decay = 0.022f; s.c1 = rnd() < 0.5f ? kIce : crystalLight; s.floorBounce = true;
                k.add(s);
            }
            ice = 0;
        }
    }

    // ---------------- MODO QUATRO ESTAÇÕES ----------------
    const bool seasonOn = (c.entity && c.entity->status.seasons > 0) || st == "vivaldi_four_seasons_mode";
    if (!paused) seasons += ((seasonOn ? 1.0f : 0.0f) - seasons) * 0.08f;
    if (seasons > 0.05f && !paused && rnd() < 0.6f * seasons) {
        const int season = (c.frame / 90) % 4; // same 1.5s native clock as the selected body form
        const float a = rnd() * PI * 2, rad = rr(40, 60);
        P q; q.x = bX + std::cos(a) * rad; q.y = bY + rr(10, 120); q.z = std::sin(a) * rad;
        q.vx = -std::sin(a) * 1.6f; q.vz = std::cos(a) * 1.6f; q.decay = 0.022f; q.rotSpeed = rr(-0.15f, 0.15f); q.seed = rnd() * 10;
        switch (season) {
        case 0: q.kind = Kind::Leaf; q.c1 = kPink; q.c2 = col("#be185d"); q.r = rr(4, 6); q.vy = rr(-0.4f, 0.4f); break;  // primavera
        case 1: q.kind = Kind::Glow; q.c1 = kGold; q.r = rr(2, 4); q.vy = rr(0.5f, 1.5f); break;                          // verão
        case 2: q.kind = Kind::Leaf; q.c1 = kLeaf; q.c2 = col("#7c2d12"); q.r = rr(5, 7); q.vy = -rr(0.4f, 1.0f); break;  // outono
        default: q.kind = Kind::Flake; q.c0 = sf::Color::White; q.c2 = kIceDeep; q.r = rr(3, 5); q.vy = -rr(0.3f, 0.8f); break; // inverno
        }
        k.add(q);
    }

    // ---------------- ESTOCADA EM CRESCENDO ----------------
    const bool jab = st == "vivaldi_crescendo_jab";
    if (!paused) spear += ((jab && p > 0.2f && p < 0.7f ? 1.0f : 0.0f) - spear) * 0.35f;
    if (jabHit(jab, p, 0.42f)) {
        burst(k, 0, chestY, rivalD - 15, kGhost, kGhostDeep, 14, 1.0f);
        for (int i = 0; i < 8; i++) {
            P w; w.kind = Kind::Glow; w.x = rr(-15, 15); w.y = chestY + rr(-15, 15); w.z = rivalD - 15; w.vx = rr(-2, 2); w.vy = rr(0, 3); w.vz = rr(-2, 2);
            w.r = rr(4, 7); w.decay = 0.03f; w.c1 = kGhost; w.drag = 0.94f;
            k.add(w);
        }
    }

    // ---------------- RÉQUIEM DE OUTONO ----------------
    const bool requiem = st == "vivaldi_autumn_requiem";
    if (requiem && !paused && p < 0.6f && static_cast<int>(p * 200) % 5 == 0) {
        P r; r.kind = Kind::Ring; r.x = bX; r.y = bY + 80; r.z = 20; r.vz = rivalD / 26; r.r = 12; r.growth = 0.9f; r.decay = 0.035f;
        r.c1 = kPurple; r.c0 = col("#e9d5ff"); r.width = 3.5f;
        k.add(r);
        note(bX + rr(-20, 20), bY + 90, 10, rr(-1, 1), rr(0.5f, 1.5f), rivalD / 40, rr(4, 6), kPurple);
    }
    if (requiemWave(requiem, p, 0.4f)) burst(k, 0, chestY, rivalD, col("#e9d5ff"), kPurple, 12, 1.1f);
    if (c.rival && c.rival->status.slowed > 0 && !paused && rnd() < 0.18f) {
        // Folhas de outono caindo sem parar sobre o rival lento
        P l; l.kind = Kind::Leaf; l.x = rr(-45, 45); l.y = rr(140, 170); l.z = rivalD + rr(-25, 25); l.vx = rr(-0.6f, 0.6f); l.vy = -rr(0.6f, 1.2f);
        l.r = rr(5, 8); l.rotSpeed = rr(-0.1f, 0.1f); l.decay = 0.008f; l.c1 = rnd() < 0.5f ? kLeaf : col("#b45309"); l.c2 = col("#7c2d12"); l.seed = rnd() * 10;
        k.add(l);
    }

    // ---------------- SONATA DE PRIMAVERA ----------------
    const bool sonata = st == "vivaldi_spring_sonata";
    if (!paused) flower += ((sonata && p < 0.9f ? 1.0f : 0.0f) - flower) * 0.1f;
    if (sonata && !paused && rnd() < 0.5f) {
        note(bX + rr(-40, 40), rr(5, 20), rr(-30, 30), 0, rr(1.2f, 2.2f), 0, rr(4, 6), rnd() < 0.5f ? kGreen : kPink);
        P pe; pe.kind = Kind::Leaf; pe.x = bX + rr(-50, 50); pe.y = rr(60, 150); pe.z = rr(-30, 30); pe.vy = -0.8f; pe.vx = rr(-0.5f, 0.5f);
        pe.r = rr(3, 5); pe.rotSpeed = 0.12f; pe.decay = 0.015f; pe.c1 = kPink; pe.c2 = col("#be185d"); pe.seed = rnd() * 10;
        k.add(pe);
    }
    if (sonataBloom(sonata, p, 0.5f)) burst(k, bX, bY + 70, 0, kGreen, kPink, 12, 1.0f);

    // Manto Fantasma: rastros ectoplásmicos quando a esquiva atravessa o golpe.
    if (c.entity && c.entity->loadout.has(SkillId::VivaldiPhantomCloak) && !paused && rnd() < 0.08f) {
        P w; w.kind = Kind::Glow; w.x = bX + rr(-30, 30); w.y = bY + rr(20, 110); w.z = rr(-10, 10); w.vy = 0.6f; w.r = rr(2, 3.5f);
        w.decay = 0.02f; w.c1 = kGhost;
        k.add(w);
    }

    k.update(paused);
}

void VivaldiFx::drawFloor(Canvas& ctx, LabContext& c) {
    auto& f = c.fighter;
    const float time = nowMs() / 1000.0f;
    // Flor da Sonata de Primavera desabrochando aos pés dela.
    if (flower > 0.02f) {
        Point3 q = c.project3D(f.body.offsetX, 0, 0);
        if (q.ok()) {
            ctx.save();
            ctx.translate(q.x, q.y);
            ctx.scale(q.scale * flower, q.scale * flower * 0.38f);
            ctx.rotate(time * 0.6f);
            for (int ring = 0; ring < 2; ring++) {
                const int n = ring == 0 ? 10 : 7;
                for (int i = 0; i < n; i++) {
                    ctx.save();
                    ctx.rotate(i * PI * 2 / n + ring * 0.3f);
                    const float L = ring == 0 ? 90 : 55;
                    ctx.beginPath(); ctx.moveTo(0, 0); ctx.quadraticCurveTo(L * 0.5f, -L * 0.28f, L, 0); ctx.quadraticCurveTo(L * 0.5f, L * 0.28f, 0, 0);
                    ctx.closePath();
                    ctx.fillStyle = ring == 0 ? cv::Style(kPink) : cv::Style(sf::Color((kPink.r+255)/2,(kPink.g+255)/2,(kPink.b+255)/2)); ctx.strokeStyle = cv::Style(kPurpleDeep); ctx.lineWidth = 2.0f;
                    ctx.fill(); ctx.stroke();
                    ctx.restore();
                }
            }
            ctx.beginPath(); ctx.arc(0, 0, 18, 0, PI * 2); ctx.fillStyle = cv::Style(kGold); ctx.strokeStyle = cv::Style(kGhostDeep); ctx.fill(); ctx.stroke();
            ctx.restore();
        }
    }
    // Relógio das estações no chão
    if (seasons > 0.05f) {
        Point3 q = c.project3D(f.body.offsetX, 0, 0);
        if (q.ok()) {
            ctx.save();
            ctx.translate(q.x, q.y);
            ctx.scale(q.scale, q.scale * 0.36f);
            ctx.globalAlpha = 0.8f * seasons;
            const sf::Color cols[] = {kPink,kGold,kLeaf,kIce};
            for (int i = 0; i < 4; i++) {
                ctx.beginPath(); ctx.arc(0, 0, 70, i * PI / 2 + time, (i + 1) * PI / 2 + time - 0.1f);
                ctx.strokeStyle = "#0f1c10"; ctx.lineWidth = 9; ctx.stroke();
                ctx.strokeStyle = cv::Style(cols[i]); ctx.lineWidth = 6; ctx.stroke();
            }
            ctx.beginPath(); ctx.moveTo(0, 0); ctx.lineTo(std::cos(time * 3) * 60, std::sin(time * 3) * 60);
            ctx.strokeStyle = "#ffffff"; ctx.lineWidth = 4; ctx.stroke();
            ctx.restore();
        }
    }
    k.drawFloor(ctx, c);
}

void VivaldiFx::drawPass(Canvas& ctx, LabContext& c, bool front) {
    auto& f = c.fighter;
    const float bodyZ = bodyDepthOf(c);
    const float time = nowMs() / 1000.0f;

    // Pauta musical da Cadência Staccato: cinco linhas onduladas da batuta até o rival
    if (staff > 0.02f) {
        const float len = rivalD * staff;
        Point3 mid = c.project3D(f.body.offsetX, 90, len * 0.5f);
        if (mid.ok() && (mid.z < bodyZ) == front) {
            ctx.save();
            ctx.lineCap = "round";
            for (int line = 0; line < 5; line++) {
                for (int pass = 0; pass < 2; pass++) {
                    ctx.beginPath();
                    bool first = true;
                    for (int i = 0; i <= 16; i++) {
                        const float u = i / 16.0f, z = 20 + (len - 20) * u;
                        const float y = 80 + line * 5 + std::sin(u * 7 - time * 8) * 6 * u;
                        Point3 q = c.project3D(f.body.offsetX + std::sin(u * 3) * 8, y, z);
                        if (!q.ok()) continue;
                        if (first) { ctx.moveTo(q.x, q.y); first = false; } else ctx.lineTo(q.x, q.y);
                    }
                    ctx.strokeStyle = pass == 0 ? cv::Style(kPurpleDeep) : cv::Style(line == 2 ? kGhost : col("#e9d5ff"));
                    ctx.lineWidth = pass == 0 ? 3.4f : 1.6f;
                    ctx.globalAlpha = staff;
                    ctx.stroke();
                }
            }
            ctx.restore();
        }
    }

    // Lança de ectoplasma da Estocada em Crescendo
    if (spear > 0.02f) {
        Point3 a = c.project3D(f.body.offsetX, 85, 20), b = c.project3D(0, 85, 20 + (rivalD - 30) * spear);
        if (a.ok() && b.ok() && (b.z < bodyZ) == front) {
            ctx.save();
            ctx.lineCap = "round";
            ctx.globalCompositeOperation = "lighter";
            for (int i = 0; i < 3; i++) {
                ctx.beginPath(); ctx.moveTo(a.x, a.y); ctx.lineTo(b.x, b.y);
                ctx.strokeStyle = i == 0 ? alpha(kGhostDeep,0.5f*spear) : (i == 1 ? alpha(kGhost,0.8f*spear) : alpha(sf::Color::White,spear));
                ctx.lineWidth = (i == 0 ? 22 : (i == 1 ? 11 : 4)) * b.scale;
                ctx.stroke();
            }
            ctx.restore();
            ctx.save();
            ctx.translate(b.x, b.y);
            ctx.rotate(std::atan2(b.y - a.y, b.x - a.x));
            const float s = b.scale;
            ctx.beginPath(); ctx.moveTo(24 * s, 0); ctx.lineTo(-6 * s, -10 * s); ctx.lineTo(0, 0); ctx.lineTo(-6 * s, 10 * s); ctx.closePath();
            ctx.fillStyle = cv::Style(kGhost); ctx.strokeStyle = cv::Style(kGhostDeep); ctx.lineWidth = 2.0f; ctx.fill(); ctx.stroke();
            ctx.restore();
        }
    }

    // Bloco de cristais de gelo em volta do rival
    if (ice > 0.01f) {
        for (const Crystal& cr : crystals) {
            Point3 base = c.project3D(cr.x, cr.y, cr.z);
            if (!base.ok() || (base.z < bodyZ) != front) continue;
            const float s = base.scale * ice;
            ctx.save();
            ctx.translate(base.x, base.y);
            ctx.rotate(cr.tilt);
            ctx.globalAlpha = 0.85f;
            ctx.beginPath();
            ctx.moveTo(-cr.w * s, 0); ctx.lineTo(-cr.w * 0.8f * s, -cr.h * 0.8f * s); ctx.lineTo(0, -cr.h * s); ctx.lineTo(cr.w * 0.8f * s, -cr.h * 0.8f * s);
            ctx.lineTo(cr.w * s, 0);
            ctx.closePath();
            auto g = ctx.createLinearGradient(-cr.w * s, 0, cr.w * s, 0);
            g->addColorStop(0,crystalLight); g->addColorStop(0.45f,crystalMid); g->addColorStop(1,crystalDark);
            ctx.fillStyle = g; ctx.strokeStyle = cv::Style(crystalEdge); ctx.lineWidth = 2.2f;
            ctx.fill(); ctx.stroke();
            ctx.beginPath(); ctx.moveTo(-cr.w * 0.4f * s, -4 * s); ctx.lineTo(-cr.w * 0.3f * s, -cr.h * 0.7f * s);
            ctx.strokeStyle = "#ffffff"; ctx.lineWidth = 2.0f * s; ctx.stroke();
            ctx.restore();
        }
        // Brilho gelado cintilando
        if (front) {
            Point3 q = c.project3D(0, 100, rivalD);
            if (q.ok()) {
                ctx.save();
                ctx.globalCompositeOperation = "lighter";
                for (int i = 0; i < 4; i++) {
                    const float a = time * 2 + i * 1.57f, x = q.x + std::cos(a) * 40 * q.scale, y = q.y + std::sin(a * 1.3f) * 50 * q.scale;
                    const float r = (6 + 3 * std::sin(time * 9 + i)) * q.scale;
                    ctx.beginPath();
                    ctx.moveTo(x, y - r * 2); ctx.lineTo(x + r * 0.4f, y); ctx.lineTo(x, y + r * 2); ctx.lineTo(x - r * 0.4f, y); ctx.closePath();
                    ctx.moveTo(x - r * 2, y); ctx.lineTo(x, y + r * 0.4f); ctx.lineTo(x + r * 2, y); ctx.lineTo(x, y - r * 0.4f); ctx.closePath();
                    ctx.fillStyle = "rgba(255, 255, 255, 0.85)";
                    ctx.fill();
                }
                ctx.restore();
            }
        }
    }
    k.draw(ctx, c, front, bodyZ);
}

} // namespace

std::unique_ptr<LabFx> makeVivaldiFx() { return std::make_unique<VivaldiFx>(); }

} // namespace KoRama::vfx
