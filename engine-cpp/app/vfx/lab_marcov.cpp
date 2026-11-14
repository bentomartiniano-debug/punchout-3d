// Dr. Marcov. Os golpes do cosmonauta ganharam animações novas: a Propulsão Foguete acende chamas
// azuis de turbina nas costas com rastro de fumaça e linhas de velocidade; o Raio Orbital Sputnik
// mostra o satélite no céu, uma mira girando no chão sob o rival e o feixe descendo; o Escudo
// Sputnik é uma bolha de plasma hexagonal com dois satélites em órbita; o Impulso de Reação deixa
// um rastro de chama cinética na luva; a Fumaça de Exaustão sai das turbinas até o rival; e a Célula
// de Oxigênio solta bolhas e anéis de pressão.
#include "fx_kit.hpp"

namespace KoRama::vfx {

namespace {

using namespace kit;

const sf::Color kPlasma = col("#38bdf8"), kPlasmaDeep = col("#0369a1"), kFlame = col("#93c5fd"), kHotWhite = sf::Color::White,
                kOrange = col("#fb923c"), kSmoke = col("#6b7280"), kSmokeDark = col("#374151"), kRed = col("#ef4444"), kO2 = col("#7dd3fc");

unsigned skinOf(const LabContext& c) { return c.entity && c.entity->skinId<4 ? c.entity->skinId : 0; }
struct Theme { sf::Color light, dark, accent, smoke, deepSmoke; };
Theme themeOf(const LabContext& c) {
    if(skinOf(c)==1)return {col("#ead19a"),col("#6f4c31"),col("#bd9d70"),col("#b49d7a"),col("#695841")};
    if(skinOf(c)==3)return {col("#e5ddca"),col("#647c7c"),col("#c95d41"),col("#c8cdc1"),col("#72847e")};
    return {kPlasma,kPlasmaDeep,kOrange,kSmoke,kSmokeDark};
}

class MarcovFx final : public LabFx {
public:
    void update(LabContext& c) override;
    void drawFloor(Canvas& ctx, LabContext& c) override;
    void drawBackground(Canvas& ctx, LabContext& c) override { drawPass(ctx, c, false); }
    void drawForeground(Canvas& ctx, LabContext& c) override { drawPass(ctx, c, true); }

private:
    void drawPass(Canvas& ctx, LabContext& c, bool front);
    void drawSatellite(Canvas& ctx, LabContext& c, float x, float y, float s, float rot);
    void thruster(LabContext& c, float power);

    Kit k;
    Once rocketHit, laserFire, kineticHit, exhaustBlast, o2Burst;
    float jets = 0, laserCharge = 0, beam = 0, shield = 0, kinetic = 0;
    float rivalD = 180;
    float airTime = -1;
    bool wasLaser = false;
};

void MarcovFx::thruster(LabContext& c, float power) {
    auto& f = c.fighter;
    const auto theme=themeOf(c);const bool natural=skinOf(c)==1||skinOf(c)==3;
    for (int side = -1; side <= 1; side += 2) {
        P fl; fl.kind = Kind::Glow; fl.x = f.body.offsetX + side * 22; fl.y = f.body.offsetY + 70; fl.z = -22;
        fl.vx = side * rr(0, 0.6f); fl.vy = -rr(1.5f, 3) * power; fl.vz = -rr(2, 5) * power; fl.r = rr(5, 8) * power; fl.decay = 0.09f;
        fl.c1 = rnd() < 0.5f ? kPlasma : kFlame; fl.drag = 0.9f;
        if(natural){fl.kind=Kind::Smoke;fl.y=f.body.offsetY+8;fl.c0=theme.light;fl.c1=theme.smoke;fl.c2=theme.deepSmoke;fl.vy=rr(.5f,1.5f);fl.r*=1.5f;fl.decay=.06f;}
        k.add(fl);
        if (rnd() < 0.4f) {
            P s; s.kind = Kind::Smoke; s.x = fl.x; s.y = fl.y - 10; s.z = -30; s.vy = -rr(0.5f, 1.5f); s.vz = -rr(1, 2.5f); s.r = rr(6, 10);
            s.growth = 0.5f; s.decay = 0.03f; s.rot = rnd() * PI * 2; s.c1 = kSmoke; s.c0 = col("#9ca3af"); s.c2 = kSmokeDark;
            if(natural){s.y=f.body.offsetY+6;s.c0=theme.light;s.c1=theme.smoke;s.c2=theme.deepSmoke;}
            k.add(s);
        }
    }
}

void MarcovFx::update(LabContext& c) {
    auto& f = c.fighter;
    const std::string& st = f.state;
    const float p = c.progress();
    const float bX = f.body.offsetX, bY = f.body.offsetY;
    const bool paused = c.isPaused;
    rivalD = rivalDistOf(c);
    const float chest = 85;
    const auto theme=themeOf(c);const sf::Color kPlasma=theme.light,kPlasmaDeep=theme.dark,kOrange=theme.accent,kSmoke=theme.smoke,kSmokeDark=theme.deepSmoke,kO2=skinOf(c)==1?col("#92a46b"):skinOf(c)==3?theme.light:col("#7dd3fc");

    // ---------------- PROPULSÃO FOGUETE ----------------
    const bool rocket = st == "marcov_rocket_thrust";
    if (!paused) jets += ((rocket && p < 0.75f ? 1.0f : 0.0f) - jets) * 0.25f;
    if (jets > 0.05f && !paused) {
        thruster(c, 0.6f + jets * 0.8f);
        if (rnd() < 0.7f) {
            // Linhas de velocidade passando por ela
            P l; l.kind = Kind::Spark; l.x = bX + rr(-60, 60); l.y = bY + rr(10, 130); l.z = rr(40, 80); l.vz = -rr(10, 16);
            l.decay = 0.12f; l.c0 = sf::Color::White; l.c1 = kPlasma; l.width = 1.0f;
            k.add(l);
        }
    }
    if (rocketHit(rocket, p, 0.5f)) {
        burst(k, 0, chest, rivalD - 20, kHotWhite, kPlasma, 18, 1.4f);
        P r; r.kind = Kind::Ring; r.y = chest; r.z = rivalD - 20; r.r = 10; r.maxR = 110; r.decay = 0.04f; r.c1 = kOrange; r.c0 = sf::Color::White; r.width = 5;
        k.add(r);
    }

    // ---------------- RAIO ORBITAL SPUTNIK ----------------
    const bool laser = st == "marcov_orbital_laser";
    if(!paused){if(laser&&!wasLaser)airTime=0;else if(airTime>=0){airTime+=1.f/60;if(airTime>1.65f)airTime=-1;}wasLaser=laser;}
    if (!paused) {
        laserCharge += ((laser && p < 0.55f ? 1.0f : 0.0f) - laserCharge) * 0.12f;
        beam = laser && p >= 0.55f && p < 0.85f ? std::min(1.0f, beam + 0.35f) : std::max(0.0f, beam - 0.15f);
    }
    if (laserFire(laser, p, 0.55f)) {
        burst(k, 0, 10, rivalD, kHotWhite, kPlasma, 26, 1.6f);
        P fr; fr.kind = Kind::FloorRing; fr.z = rivalD; fr.r = 15; fr.maxR = 130; fr.decay = 0.025f; fr.c1 = kPlasma; fr.c0 = sf::Color::White; fr.width = 6;
        k.add(fr);
    }
    if (beam > 0.3f && !paused) {
        for (int i = 0; i < 2; i++) {
            P s; s.kind = Kind::Spark; s.x = rr(-10, 10); s.y = rr(2, 8); s.z = rivalD + rr(-10, 10);
            const float a = rnd() * PI * 2; s.vx = std::cos(a) * rr(3, 7); s.vz = std::sin(a) * rr(3, 7); s.vy = rr(2, 6); s.gravity = 0.3f;
            s.decay = 0.05f; s.c0 = sf::Color::White; s.c1 = kPlasma; s.width = 1.4f;
            k.add(s);
        }
    }

    // ---------------- ESCUDO SPUTNIK ----------------
    const bool shieldOn = (c.entity && c.entity->status.plasma > 0) || st == "marcov_plasma_barrier";
    if (!paused) shield += ((shieldOn ? 1.0f : 0.0f) - shield) * 0.1f;
    if (shield > 0.05f && !paused && c.entity && c.entity->state == FIGHTER_HIT && rnd() < 0.6f) {
        // Golpe absorvido: faíscas de plasma pulam na superfície da bolha
        const float a = rr(-0.8f, 0.8f);
        P b; b.kind = Kind::Bolt; b.x = bX + std::sin(a) * 50; b.y = bY + rr(50, 110); b.z = std::cos(a) * 40;
        b.tx = bX + std::sin(a + 0.6f) * 50; b.ty = b.y + rr(-30, 30); b.tz = std::cos(a + 0.6f) * 40; b.decay = 0.15f; b.c1 = kPlasma; b.seed = rnd() * 10;
        k.add(b);
    }

    // ---------------- IMPULSO DE REAÇÃO ----------------
    const bool kin = st == "marcov_kinetic_overdrive";
    if (!paused) kinetic += ((kin && p < 0.6f ? 1.0f : 0.0f) - kinetic) * 0.3f;
    if (kinetic > 0.05f && !paused) {
        const Glove& g = f.isLeft ? f.parts.gloveL : f.parts.gloveR;
        for (int i = 0; i < 2; i++) {
            P fl; fl.kind = Kind::Glow; fl.x = g.x + bX + rr(-6, 6); fl.y = g.y + bY + rr(-6, 6); fl.z = g.z - 10; fl.vz = -rr(3, 6); fl.vy = rr(-0.5f, 0.5f);
            fl.r = rr(5, 9); fl.decay = 0.1f; fl.c1 = rnd() < 0.5f ? kOrange : kPlasma;
            k.add(fl);
        }
    }
    if (kineticHit(kin, p, 0.4f)) burst(k, 0, chest, rivalD - 20, kOrange, kPlasma, 16, 1.1f);

    // ---------------- FUMAÇA DE EXAUSTÃO ----------------
    const bool exhaust = st == "marcov_exhaust_cloud";
    if (exhaust && !paused && p < 0.5f) {
        thruster(c, 0.7f);
        P s; s.kind = Kind::Smoke; s.x = bX + rr(-20, 20); s.y = bY + rr(40, 90); s.z = 10; s.vz = rivalD / rr(25, 35); s.vy = rr(-0.3f, 0.4f);
        s.r = rr(10, 16); s.growth = 0.5f; s.decay = 0.02f; s.rot = rnd() * PI * 2; s.rotSpeed = rr(-0.05f, 0.05f);
        s.c1 = kSmoke; s.c0 = col("#9ca3af"); s.c2 = kSmokeDark;
        k.add(s);
    }
    if (exhaustBlast(exhaust, p, 0.4f)) {
        for (int i = 0; i < 12; i++) {
            P s; s.kind = Kind::Smoke; s.x = rr(-35, 35); s.y = rr(40, 130); s.z = rivalD + rr(-25, 25); s.vy = rr(0, 0.8f);
            s.r = rr(16, 26); s.growth = 0.4f; s.decay = 0.012f; s.rot = rnd() * PI * 2; s.c1 = kSmoke; s.c0 = col("#9ca3af"); s.c2 = kSmokeDark;
            k.add(s);
        }
    }

    // ---------------- CÉLULA DE OXIGÊNIO ----------------
    const bool o2 = st == "marcov_oxygen_recharge";
    if (o2 && !paused && rnd() < 0.6f) {
        P b; b.kind = Kind::Bubble; b.x = bX + rr(-35, 35); b.y = bY + rr(20, 80); b.z = rr(-20, 20); b.vy = rr(1, 2.2f); b.vx = rr(-0.3f, 0.3f);
        b.r = rr(3, 7); b.decay = 0.02f; b.c0 = sf::Color::White; b.c1 = kO2; b.c2 = kPlasmaDeep;
        if(skinOf(c)==1){b.kind=Kind::Leaf;b.c1=col("#92a46b");b.c0=col("#d4ce92");b.rot=rnd()*6;b.rotSpeed=.04f;}
        k.add(b);
    }
    if (o2Burst(o2, p, 0.5f)) {
        for (int i = 0; i < 3; i++) {
            P r; r.kind = Kind::Ring; r.x = bX; r.y = bY + 60 + i * 15; r.r = 20; r.maxR = 80 + i * 15; r.decay = 0.035f + i * 0.005f;
            r.c1 = kO2; r.c0 = sf::Color::White; r.width = 3;
            k.add(r);
        }
    }

    k.update(paused);
}

void MarcovFx::drawFloor(Canvas& ctx, LabContext& c) {
    const float time = nowMs() / 1000.0f;
    const auto theme=themeOf(c);const sf::Color kPlasmaDeep=theme.dark,kRed=theme.accent;
    // Mira do satélite girando sob o rival
    if (laserCharge > 0.03f || beam > 0.03f) {
        Point3 q = c.project3D(0, 0, rivalD);
        if (q.ok()) {
            ctx.save();
            ctx.translate(q.x, q.y);
            ctx.scale(q.scale, q.scale * 0.38f);
            const float a = std::max(laserCharge, beam), R = 70 - 25 * laserCharge;
            ctx.globalAlpha = a;
            ctx.rotate(time * 3);
            for (int pass = 0; pass < 2; pass++) {
                ctx.strokeStyle = pass == 0 ? cv::Style(kPlasmaDeep) : cv::Style(kRed);
                ctx.lineWidth = pass == 0 ? 7 : 3.5f;
                ctx.beginPath(); ctx.arc(0, 0, R, 0, PI * 2); ctx.stroke();
                ctx.beginPath(); ctx.arc(0, 0, R * 0.55f, 0, PI * 2); ctx.stroke();
                for (int i = 0; i < 4; i++) {
                    ctx.save(); ctx.rotate(i * PI / 2);
                    ctx.beginPath(); ctx.moveTo(R * 0.7f, 0); ctx.lineTo(R * 1.25f, 0); ctx.stroke();
                    ctx.restore();
                }
            }
            ctx.restore();
        }
    }
    k.drawFloor(ctx, c);
}

void MarcovFx::drawSatellite(Canvas& ctx, LabContext& c, float x, float y, float s, float rot) {
    std::string key="marcov_skin"+std::to_string(skinOf(c))+"_orbital";
    if(skinOf(c)==3)key+="_"+std::to_string((c.frame/2)%8);
    if(auto img=c.sprites->find(key)){
        ctx.save();ctx.translate(x,y);ctx.rotate(rot);ctx.drawImage(*img,-40*s,-40*s,80*s,80*s);ctx.restore();return;
    }
    ctx.save();
    ctx.translate(x, y);
    ctx.rotate(rot);
    // Painéis solares
    for (int side = -1; side <= 1; side += 2) {
        ctx.fillStyle = "#1e3a8a"; ctx.strokeStyle = "#0f1c10"; ctx.lineWidth = 1.6f;
        ctx.fillRect(side * 14 * s - (side < 0 ? 26 * s : 0), -7 * s, 26 * s, 14 * s);
        ctx.strokeRect(side * 14 * s - (side < 0 ? 26 * s : 0), -7 * s, 26 * s, 14 * s);
        ctx.strokeStyle = "#60a5fa"; ctx.lineWidth = 1.0f;
        for (int i = 1; i < 3; i++) {
            const float xx = side * 14 * s - (side < 0 ? 26 * s : 0) + i * 26 * s / 3;
            ctx.beginPath(); ctx.moveTo(xx, -7 * s); ctx.lineTo(xx, 7 * s); ctx.stroke();
        }
    }
    // Corpo esférico com antenas (Sputnik)
    ctx.beginPath(); ctx.arc(0, 0, 13 * s, 0, PI * 2);
    auto g = ctx.createRadialGradient(-4 * s, -4 * s, 1, 0, 0, 13 * s);
    g->addColorStop(0, "#ffffff"); g->addColorStop(0.5f, "#cbd5e1"); g->addColorStop(1, "#64748b");
    ctx.fillStyle = g; ctx.strokeStyle = "#0f1c10"; ctx.lineWidth = 2.0f; ctx.fill(); ctx.stroke();
    ctx.strokeStyle = "#0f1c10"; ctx.lineWidth = 1.6f;
    for (int i = 0; i < 4; i++) {
        const float a = PI * 0.6f + i * 0.25f;
        ctx.beginPath(); ctx.moveTo(std::cos(a) * 12 * s, std::sin(a) * 12 * s); ctx.lineTo(std::cos(a) * 34 * s, std::sin(a) * 34 * s); ctx.stroke();
    }
    ctx.beginPath(); ctx.arc(0, 0, 4 * s, 0, PI * 2); ctx.fillStyle = "#ef4444"; ctx.fill();
    ctx.restore();
}

void MarcovFx::drawPass(Canvas& ctx, LabContext& c, bool front) {
    auto& f = c.fighter;
    const float bodyZ = bodyDepthOf(c);
    const float time = nowMs() / 1000.0f;
    const auto theme=themeOf(c);const sf::Color kPlasmaDeep=theme.dark;
    const bool stone=skinOf(c)==1,aviator=skinOf(c)==3;

    // Air raid remains in world space through recovery. Its camera-side pass
    // is behind the player in the rear view, never painted over his body.
    if(aviator&&airTime>=0&&front==(c.activeCamera=="front")){
        const float u=std::clamp(airTime/1.55f,0.f,1.f),a=std::min(1.f,airTime/.10f)*std::clamp((1.65f-airTime)/.18f,0.f,1.f);
        Point3 plane=c.project3D(-180+360*u,175+std::sin(u*PI)*10,-20);
        if(plane.ok()){ctx.save();ctx.globalAlpha=a;
            ctx.strokeStyle=alpha(theme.smoke,.7f);ctx.lineWidth=4*plane.scale;ctx.beginPath();ctx.moveTo(plane.x-90*plane.scale,plane.y+4);ctx.lineTo(plane.x-12*plane.scale,plane.y);ctx.stroke();
            drawSatellite(ctx,c,plane.x,plane.y,1.6f*plane.scale,0);ctx.restore();}
        for(int i=0;i<6;i++)for(int j=0;j<2;j++){
            const float start=.275f+i*.105f+j*.025f,v=(airTime-start)/.58f;if(v<=0||v>=1)continue;
            const float x=-180+360*std::clamp(start/1.55f,0.f,1.f),tx=(i-2.5f)*16+(j?8:-8),old=std::max(0.f,v-.14f);
            Point3 tip=c.project3D(x+(tx-x)*v,175*(1-v),-20+(rivalD+20)*v),tail=c.project3D(x+(tx-x)*old,175*(1-old),-20+(rivalD+20)*old);
            if(!tip.ok()||!tail.ok())continue;ctx.save();ctx.globalAlpha=std::sin(v*PI);
            ctx.strokeStyle=alpha(col("#eee1ae"),.85f);ctx.lineWidth=3*tip.scale;ctx.beginPath();ctx.moveTo(tail.x,tail.y);ctx.lineTo(tip.x,tip.y);ctx.stroke();
            ctx.fillStyle="#e7c379";ctx.strokeStyle="#211714";ctx.lineWidth=1.2f;ctx.beginPath();ctx.ellipse(tip.x,tip.y,3.5f*tip.scale,6.5f*tip.scale,0,0,2*PI);ctx.fill();ctx.stroke();ctx.restore();
        }
    }

    // Satélite e feixe do Raio Orbital (sempre na frente: vem do céu)
    if (!aviator && front==(c.activeCamera=="front") && (laserCharge > 0.03f || beam > 0.03f)) {
        Point3 sky = c.project3D(0, 420, rivalD), ground = c.project3D(0, 0, rivalD);
        if (sky.ok() && ground.ok()) {
            const float sy = std::max(30.0f, sky.y);
            if(!stone)drawSatellite(ctx,c,sky.x,sy,std::clamp(sky.scale*1.4f,.7f,1.1f),std::sin(time*2)*.2f);
            if (laserCharge > 0.05f && !stone) {
                // Carga: brilho pulsando na antena
                ctx.save();
                ctx.globalCompositeOperation = "lighter";
                const float r = (10 + 20 * laserCharge + std::sin(time * 30) * 4);
                auto g = ctx.createRadialGradient(sky.x, sy + 12, 0, sky.x, sy + 12, r);
                g->addColorStop(0, cv::rgba(255, 255, 255, laserCharge).color);
                g->addColorStop(0.4f, cv::rgba(56, 189, 248, 0.8f * laserCharge).color);
                g->addColorStop(1, cv::rgba(56, 189, 248, 0).color);
                ctx.fillStyle = g;
                ctx.beginPath(); ctx.arc(sky.x, sy + 12, r, 0, PI * 2); ctx.fill();
                ctx.restore();
            }
            if(stone&&beam>.03f){ctx.save();ctx.globalAlpha=beam;for(int pass=0;pass<2;pass++){
                ctx.strokeStyle=pass?cv::Style(col("#f7df8c")):cv::Style(theme.dark);ctx.lineWidth=(pass?4.f:8.f)*ground.scale;
                ctx.beginPath();ctx.moveTo(sky.x,sy);for(int i=1;i<=7;i++){const float p=i/7.f;ctx.lineTo(sky.x+(ground.x-sky.x)*p+(i==7?0:std::sin(i*2.8f+std::floor(time*12))*14*ground.scale),sy+(ground.y-sy)*p);}ctx.stroke();}ctx.restore();}
            if (beam > 0.03f && !stone) {
                ctx.save();
                ctx.globalCompositeOperation = "lighter";
                const float w = 40 * ground.scale * beam;
                const float jit = std::sin(time * 50) * 3;
                for (int i = 0; i < 3; i++) {
                    const float k2 = 1.0f - i * 0.35f;
                    ctx.beginPath();
                    ctx.moveTo(sky.x - w * 0.25f * k2, sy + 12); ctx.lineTo(sky.x + w * 0.25f * k2, sy + 12);
                    ctx.lineTo(ground.x + w * k2 + jit, ground.y); ctx.lineTo(ground.x - w * k2 - jit, ground.y);
                    ctx.closePath();
                    ctx.fillStyle = i == 0 ? cv::rgba(3, 105, 161, 0.55f * beam) : (i == 1 ? cv::rgba(56, 189, 248, 0.8f * beam) : cv::rgba(255, 255, 255, beam));
                    ctx.fill();
                }
                ctx.beginPath(); ctx.ellipse(ground.x, ground.y, w * 1.6f, w * 0.55f, 0, 0, PI * 2);
                ctx.fillStyle = cv::rgba(186, 230, 253, 0.8f * beam); ctx.fill();
                ctx.restore();
            }
        }
    }

    // Chamas das turbinas nas costas (lado oposto ao rival)
    if (jets > 0.05f && !stone && !aviator) {
        for (int side = -1; side <= 1; side += 2) {
            Point3 q = c.project3D(f.body.offsetX + side * 22, f.body.offsetY + 70, -20);
            Point3 t = c.project3D(f.body.offsetX + side * 24, f.body.offsetY + 40, -(20 + 55 * jets + std::sin(time * 40 + side) * 8));
            if (!q.ok() || !t.ok() || (q.z < bodyZ) != front) continue;
            ctx.save();
            ctx.globalCompositeOperation = "lighter";
            const float w = 12 * q.scale;
            for (int i = 0; i < 3; i++) {
                const float k2 = 1.0f - i * 0.3f;
                ctx.beginPath(); ctx.moveTo(q.x - w * k2, q.y); ctx.quadraticCurveTo(t.x, (q.y + t.y) / 2, t.x, t.y); ctx.quadraticCurveTo(t.x, (q.y + t.y) / 2, q.x + w * k2, q.y);
                ctx.closePath();
                ctx.fillStyle = i == 0 ? cv::rgba(3, 105, 161, 0.7f) : (i == 1 ? cv::rgba(56, 189, 248, 0.85f) : cv::rgba(255, 255, 255, 0.95f));
                ctx.fill();
            }
            ctx.restore();
        }
    }

    // Bolha de plasma hexagonal + satélites em órbita
    if (shield > 0.03f) {
        Point3 center = c.project3D(f.body.offsetX, f.body.offsetY + 70, 0);
        if (center.ok()) {
            const float R = 78 * center.scale;
            if (front && !stone && !aviator) {
                ctx.save();
                ctx.globalAlpha = 0.85f * shield;
                auto g = ctx.createRadialGradient(center.x, center.y, R * 0.6f, center.x, center.y, R);
                g->addColorStop(0, cv::rgba(56, 189, 248, 0.0f).color);
                g->addColorStop(0.85f, cv::rgba(56, 189, 248, 0.25f).color);
                g->addColorStop(1, cv::rgba(186, 230, 253, 0.6f).color);
                ctx.fillStyle = g;
                ctx.beginPath(); ctx.arc(center.x, center.y, R, 0, PI * 2); ctx.fill();
                // Malha hexagonal pulsando
                ctx.strokeStyle = cv::rgba(186, 230, 253, 0.55f + 0.25f * std::sin(time * 5));
                ctx.lineWidth = 1.4f;
                const float hs = 16 * center.scale;
                for (int row = -4; row <= 4; row++) {
                    for (int col = -4; col <= 4; col++) {
                        const float hx = center.x + col * hs * 1.5f, hy = center.y + row * hs * 1.73f + (col % 2 ? hs * 0.86f : 0);
                        if (std::hypot(hx - center.x, hy - center.y) > R * 0.92f) continue;
                        ctx.beginPath();
                        for (int v = 0; v < 6; v++) {
                            const float a = v * PI / 3;
                            if (v == 0) ctx.moveTo(hx + std::cos(a) * hs * 0.9f, hy + std::sin(a) * hs * 0.9f);
                            else ctx.lineTo(hx + std::cos(a) * hs * 0.9f, hy + std::sin(a) * hs * 0.9f);
                        }
                        ctx.closePath();
                        ctx.stroke();
                    }
                }
                ctx.beginPath(); ctx.arc(center.x, center.y, R, 0, PI * 2);
                ctx.strokeStyle = cv::Style(kPlasmaDeep); ctx.lineWidth = 3.0f; ctx.stroke();
                ctx.strokeStyle = "#ffffff"; ctx.lineWidth = 1.2f; ctx.stroke();
                ctx.restore();
            }
            if(aviator){const auto wind=c.project3D(f.body.offsetX,f.body.offsetY+85,0);ctx.save();ctx.globalAlpha=shield*.7f;ctx.strokeStyle=cv::Style(theme.light);ctx.lineWidth=2.5f;
                for(int ring=0;ring<3;ring++){const float rx=(72+ring*4)*wind.scale,ry=(78+ring*4)*wind.scale;ctx.beginPath();ctx.ellipse(wind.x,wind.y,rx,ry,0,front?0:PI,front?PI:2*PI);ctx.stroke();}ctx.restore();}
            const int count=stone?6:aviator?1:2;
            for (int i = 0; i < count; i++) {
                const float a = time * 2.2f + i * 2*PI/count;
                Point3 sp = c.project3D(f.body.offsetX + std::cos(a) * 85, f.body.offsetY + 90 + std::sin(a * 2) * 12, std::sin(a) * 60);
                if (!sp.ok() || (sp.z < bodyZ) != front) continue;
                drawSatellite(ctx,c,sp.x,sp.y,(stone?.42f:aviator?.8f:.45f)*sp.scale*shield,aviator?a+PI/2:a);
            }
        }
    }
    k.draw(ctx, c, front, bodyZ);
}

} // namespace

std::unique_ptr<LabFx> makeMarcovFx() { return std::make_unique<MarcovFx>(); }

} // namespace KoRama::vfx
