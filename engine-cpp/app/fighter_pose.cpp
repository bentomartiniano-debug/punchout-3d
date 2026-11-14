// Reações dos lutadores desenhadas por cima dos sprites: cabeça que vira no gancho, careta de dor
// com a mão na barriga, tontura com estrelinhas, queda do knockdown e o giro do voo de nocaute.
// A cabeça é um recorte do próprio sprite (acima do pescoço) que gira e encolhe na horizontal; a
// careta é pintada sobre o rosto com a cor da pele de cada lutador.
#include "ring_renderer.hpp"
#include "canvas2d.hpp"
#include "physics.hpp"
#include <algorithm>
#include <array>
#include <cmath>

namespace KoRama {

namespace {

constexpr float kPi = 3.14159265f;

// Rosto de frente de cada lutador, em coordenadas normalizadas do sprite (0..1).
struct FaceSpec {
    float neck;            // altura do pescoço (corte da cabeça)
    float eyeLx, eyeRx, eyeY, eyeR;
    float mouthX, mouthY, mouthW;
    sf::Color skin;
};

const std::array<FaceSpec, 9> kFaces = {{
    {0.38f, 0.445f, 0.555f, 0.320f, 0.030f, 0.500f, 0.405f, 0.070f, sf::Color(0xdc, 0xed, 0xc1)}, // Jack
    {0.46f, 0.360f, 0.640f, 0.290f, 0.045f, 0.500f, 0.445f, 0.200f, sf::Color(0x6f, 0x96, 0x4c)}, // King Frog
    {0.38f, 0.425f, 0.525f, 0.310f, 0.026f, 0.480f, 0.370f, 0.060f, sf::Color(0xb8, 0x74, 0x4c)}, // Joana
    {0.46f, 0.390f, 0.600f, 0.310f, 0.045f, 0.500f, 0.440f, 0.110f, sf::Color(0xfe, 0xf0, 0x8a)}, // O Cremoso
    {0.41f, 0.430f, 0.550f, 0.340f, 0.034f, 0.490f, 0.395f, 0.070f, sf::Color(0x8a, 0x4a, 0x2a)}, // Valéria
    {0.48f, 0.450f, 0.570f, 0.380f, 0.032f, 0.510f, 0.455f, 0.060f, sf::Color(0xd4, 0xec, 0xdc)}, // Vivaldi
    {0.44f, 0.420f, 0.580f, 0.310f, 0.032f, 0.500f, 0.385f, 0.070f, sf::Color(0x1f, 0x9e, 0x76)}, // Lady Espinha
    {0.40f, 0.420f, 0.580f, 0.330f, 0.032f, 0.500f, 0.385f, 0.070f, sf::Color(0xc4, 0x8e, 0x62)}, // Dr. Marcov
    {0.40f, 0.440f, 0.560f, 0.300f, 0.030f, 0.500f, 0.380f, 0.070f, sf::Color(0x9c, 0xa3, 0xaf)}, // Batalhator
}};

float smooth01(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return t * t * (3 - 2 * t);
}


// Mapeia (u, v) do sprite para a tela, com a cabeça (v < pescoço) girada em volta do pescoço.
struct BodyMap {
    sf::Vector2f tl, tr, bl;
    sf::Vector2f ax, ay;   // eixos unitários do sprite na tela
    float w = 1, h = 1;    // tamanho do sprite na tela
    float neck = 0.4f;
    float headRot = 0, headScaleX = 1;
    sf::Vector2f headShift;
    sf::Vector2f flat(float u, float v) const { return tl + (tr - tl) * u + (bl - tl) * v; }
    sf::Vector2f head(float u, float v) const {
        const sf::Vector2f n = flat(0.5f, neck);
        sf::Vector2f d = flat(u, v) - n;
        const float along = d.x * ax.x + d.y * ax.y;
        d += ax * (along * (headScaleX - 1));
        const float c = std::cos(headRot), s = std::sin(headRot);
        return n + sf::Vector2f{d.x * c - d.y * s, d.x * s + d.y * c} + headShift;
    }
    float angle() const { return std::atan2(ax.y, ax.x) + headRot; }
};

void star(cv::Canvas& ctx, float x, float y, float r, float rot) {
    ctx.beginPath();
    for (int i = 0; i < 10; ++i) {
        const float a = rot + i * kPi / 5 - kPi / 2, rr = i % 2 == 0 ? r : r * 0.45f;
        if (i == 0) ctx.moveTo(x + std::cos(a) * rr, y + std::sin(a) * rr); else ctx.lineTo(x + std::cos(a) * rr, y + std::sin(a) * rr);
    }
    ctx.closePath();
}

// Careta: 1 olhos apertados + boca aberta, 2 olhos apertados + dentes cerrados, 3 olhos em espiral.
void drawFace(cv::Canvas& ctx, const BodyMap& m, const FaceSpec& fs, int face, float time) {
    const float s = m.w;
    const float ang = m.angle();
    auto atFace = [&](float u, float v) {
        const sf::Vector2f p = m.head(u, v);
        ctx.save();
        ctx.translate(p.x, p.y);
        ctx.rotate(ang);
        ctx.scale(m.headScaleX, 1.0f);
    };
    const char* ink = "#0b0712";
    const float er = fs.eyeR * s;
    for (int e = 0; e < 2; ++e) {
        const float ex = e == 0 ? fs.eyeLx : fs.eyeRx;
        atFace(ex, fs.eyeY);
        ctx.fillStyle = cv::rgba(fs.skin.r, fs.skin.g, fs.skin.b, 1.0f);
        ctx.beginPath(); ctx.ellipse(0, 0, er * 1.45f, er * 1.25f, 0, 0, 2 * kPi); ctx.fill();
        ctx.strokeStyle = ink; ctx.lineWidth = std::max(1.5f, er * 0.42f); ctx.lineJoin = "round"; ctx.lineCap = "round";
        if (face == 3) {
            // Espiral girando
            ctx.beginPath();
            for (int k = 0; k <= 22; ++k) {
                const float t = k / 22.0f, a = t * 4.2f * kPi + time * 8 * (e == 0 ? 1 : -1), r = er * 1.05f * t;
                if (k == 0) ctx.moveTo(std::cos(a) * r, std::sin(a) * r); else ctx.lineTo(std::cos(a) * r, std::sin(a) * r);
            }
            ctx.stroke();
        } else {
            // > <  (apontando para o nariz)
            const float d = e == 0 ? 1.0f : -1.0f;
            ctx.beginPath();
            ctx.moveTo(-er * 0.8f * d, -er * 0.75f); ctx.lineTo(er * 0.7f * d, 0); ctx.lineTo(-er * 0.8f * d, er * 0.75f);
            ctx.stroke();
        }
        ctx.restore();
    }
    const float mw = fs.mouthW * s;
    atFace(fs.mouthX, fs.mouthY);
    ctx.lineJoin = "round";
    if (face == 2) {
        // Dentes cerrados
        const float bw = mw, bh = std::max(4.0f, mw * 0.42f);
        ctx.fillStyle = "#ffffff";
        ctx.beginPath(); ctx.roundRect(-bw / 2, -bh / 2, bw, bh, bh * 0.3f); ctx.fill();
        ctx.strokeStyle = ink; ctx.lineWidth = std::max(1.5f, s * 0.012f); ctx.stroke();
        ctx.beginPath(); ctx.moveTo(-bw / 2, 0); ctx.lineTo(bw / 2, 0);
        for (int k = 1; k < 4; ++k) { const float x = -bw / 2 + bw * k / 4; ctx.moveTo(x, -bh / 2); ctx.lineTo(x, bh / 2); }
        ctx.lineWidth = std::max(1.0f, s * 0.008f); ctx.stroke();
    } else if (face == 1) {
        // Boca aberta torta, com língua
        ctx.fillStyle = "#2a0a12";
        ctx.beginPath(); ctx.ellipse(0, 0, mw * 0.42f, mw * 0.32f, -0.2f, 0, 2 * kPi); ctx.fill();
        ctx.strokeStyle = ink; ctx.lineWidth = std::max(1.5f, s * 0.012f); ctx.stroke();
        ctx.fillStyle = "#ff5c7a";
        ctx.beginPath(); ctx.ellipse(mw * 0.06f, mw * 0.14f, mw * 0.2f, mw * 0.12f, 0, 0, 2 * kPi); ctx.fill();
    } else {
        // Boca ondulada de zonzo
        ctx.strokeStyle = ink; ctx.lineWidth = std::max(1.5f, s * 0.012f); ctx.lineCap = "round";
        ctx.beginPath();
        for (int k = 0; k <= 8; ++k) {
            const float x = -mw / 2 + mw * k / 8, y = std::sin(k * 1.6f + time * 6) * mw * 0.1f;
            if (k == 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
        }
        ctx.stroke();
    }
    ctx.restore();
    // Marca de dor (veia) na testa e gota de suor
    if (face != 3) {
        atFace(fs.eyeRx + fs.eyeR * 2.2f, fs.eyeY - fs.eyeR * 3.0f);
        const float r = er * 1.1f;
        ctx.strokeStyle = "#ff1e42"; ctx.lineWidth = std::max(1.5f, er * 0.45f); ctx.lineCap = "round";
        ctx.beginPath();
        for (int q = 0; q < 4; ++q) {
            const float a = q * kPi / 2 + kPi / 4;
            ctx.moveTo(std::cos(a) * r * 0.3f, std::sin(a) * r * 0.3f);
            ctx.lineTo(std::cos(a) * r, std::sin(a) * r);
        }
        ctx.stroke();
        ctx.restore();
        atFace(fs.eyeLx - fs.eyeR * 2.4f, fs.eyeY - fs.eyeR * 0.5f);
        ctx.fillStyle = "#9be7ff"; ctx.strokeStyle = ink; ctx.lineWidth = std::max(1.0f, er * 0.25f);
        ctx.beginPath(); ctx.moveTo(0, -er * 1.4f); ctx.quadraticCurveTo(er * 0.9f, 0, 0, er * 0.7f); ctx.quadraticCurveTo(-er * 0.9f, 0, 0, -er * 1.4f);
        ctx.fill(); ctx.stroke();
        ctx.restore();
    }
}

} // namespace

RingRenderer::Reaction RingRenderer::computeReaction(const Fighter& f, int slot, float roll, bool koFlight, bool backView) {
    Reaction r;
    const uint32_t frame = engine_ ? engine_->frameIndex : frameCounter_;
    r.time = frame / 60.0f;
    const char* view = backView ? "Back" : "Front";

    if (koFlight) {
        // Gira em torno do próprio eixo trocando os ângulos (frente, 3/4, perfil, 3/4 de costas, costas),
        // como os torcedores; a segunda metade da volta usa os mesmos quadros espelhados.
        float a = std::fmod(roll, 2 * kPi);
        if (a < 0) a += 2 * kPi;
        if (backView) a = std::fmod(a + kPi, 2 * kPi);
        const int sector = static_cast<int>(std::floor(a / (kPi / 4) + 0.5f)) % 8;
        static const int kFrame[8] = {0, 1, 2, 3, 4, 3, 2, 1};
        r.sprite = "RxSpin_" + std::to_string(kFrame[sector]);
        r.mirror = sector > 4;
        r.yaw = a;
        r.xxFace = true;
        r.stars = true;
        r.roll = roll * 0.55f; // cambalhota (gira na tela) junto do giro no próprio eixo
        return r;
    }

    // Knockdown: o golpe vira a cabeça, ele recua cambaleando até as cordas e fica agarrado nelas,
    // zonzo (olhos em espiral e estrelinhas), até levantar.
    if (f.state == FIGHTER_KNOCKDOWN) {
        if (downFrames_[slot] < 0) { downFrames_[slot] = 0; downIdx_[slot] = frame; downDir_[slot] = f.hitDir >= 0 ? 1 : -1; }
        while (downIdx_[slot] < frame) { ++downIdx_[slot]; ++downFrames_[slot]; }
        const float n = static_cast<float>(downFrames_[slot]);
        const float d = static_cast<float>(downDir_[slot]);
        const float back = smooth01((n - 6) / 34.0f);           // recuo até as cordas
        const float maxBack = f.isBigChar() ? 236.0f : (f.isSmallChar() ? 250.0f : 244.0f);
        const float ropeDist = std::max(0.0f, maxBack - f.radius);
        r.dz = -ropeDist * back;
        r.dir = d;
        if (n < 14) {
            r.sprite = std::string("Rx") + view + "Turn" + (d > 0 ? "R" : "L");
            r.headRot = d * 0.5f; r.headShiftX = d * 0.1f; r.headScaleX = 0.72f; // sem sprite novo (chefe)
            r.skew = d * 0.15f;
            r.gloveSpread = 18; r.gloveLift = -10;
            r.face = 1;
        } else {
            const float wob = std::sin(r.time * 2.6f);
            r.rope = smooth01((n - 14) / 18.0f);
            r.roll = d * 0.06f * (1 - r.rope) + 0.05f * wob * r.rope;
            r.dy = -14 * r.rope + std::fabs(std::sin(r.time * 1.3f)) * 3;
            r.headRot = 0.16f * wob; r.headShiftY = 0.02f;
            r.face = 3;
            r.stars = true;
        }
        return r;
    }
    downFrames_[slot] = -1;

    if (f.state == FIGHTER_HIT && f.maxTimer > 0 && f.hitKind > 0) {
        const float p = 1.0f - static_cast<float>(f.timer) / static_cast<float>(f.maxTimer);
        const float env = smooth01(p < 0.15f ? p / 0.15f : 1 - (p - 0.15f) / 0.85f);
        const float hold = smooth01(p < 0.12f ? p / 0.12f : 1 - (p - 0.65f) / 0.35f); // mão na barriga fica mais tempo
        const float d = f.hitDir >= 0 ? 1.0f : -1.0f;
        r.dir = d;
        r.hitP = p;
        const std::string turn = std::string("Rx") + view + "Turn" + (d > 0 ? "R" : "L");
        switch (f.hitKind) {
        case 1:
            if (env > 0.45f) r.sprite = turn;
            r.headRot = d * 0.24f * env; r.headShiftX = d * 0.04f * env; r.headScaleX = 1 - 0.14f * env;
            r.gloveSpread = 12 * env; r.gloveLift = -10 * env;
            r.face = env > 0.12f ? 1 : 0;
            break;
        case 2:
            if (env > 0.1f) r.sprite = turn;
            r.headRot = d * 0.52f * env; r.headShiftX = d * 0.11f * env; r.headScaleX = 1 - 0.38f * env;
            r.skew = d * 0.16f * env;
            r.gloveShift = -d * 12 * env; r.gloveSpread = 16 * env; r.gloveLift = -16 * env;
            r.face = env > 0.1f ? 1 : 0;
            r.drops = p < 0.7f;
            break;
        case 3:
            r.squash = 1 - 0.1f * hold;
            r.dy = -10 * hold;
            r.headShiftY = 0.035f * hold; r.headRot = -d * 0.1f * hold;
            r.clutch = hold;
            r.clutchLeft = d < 0;
            r.face = hold > 0.1f ? 2 : 0;
            break;
        case 4:
            if (env > 0.1f) r.sprite = std::string("Rx") + view + "Up";
            r.headShiftY = -0.04f * env; r.headRot = d * 0.16f * env; r.headScaleX = 1 - 0.1f * env;
            r.dy = 6 * env;
            r.gloveLift = 20 * env;
            r.gloveSpread = 20 * env; // abre os braços
            r.face = env > 0.1f ? 1 : 0;
            break;
        default:
            break;
        }
        return r;
    }

    const bool isKnockdownWinner = engine_ && engine_->gameState == STATE_KNOCKDOWN &&
                                   ((f.isPlayer && engine_->enemyIsDown) || (!f.isPlayer && !engine_->enemyIsDown));
    const bool isMatchWinner = engine_ && engine_->isMatchOver() &&
                               (((engine_->result == MatchResult::WinKO || engine_->result == MatchResult::WinTKO || engine_->result == MatchResult::WinDecision) && f.isPlayer) ||
                                ((engine_->result == MatchResult::LossKO || engine_->result == MatchResult::LossTKO || engine_->result == MatchResult::LossDecision) && !f.isPlayer));
    const bool isKnockdownLoser = engine_ && engine_->gameState == STATE_KNOCKDOWN &&
                                  ((f.isPlayer && !engine_->enemyIsDown) || (!f.isPlayer && engine_->enemyIsDown));
    const bool isMatchLoser = engine_ && engine_->isMatchOver() && !isMatchWinner;

    const bool isHeroic = !koFlight && !isKnockdownLoser && !isMatchLoser &&
                          (f.signatureOnly || f.state == FIGHTER_VICTORY_POSE || (f.state == FIGHTER_SKILL && f.hp > 0.0f) || isKnockdownWinner || isMatchWinner);

    // Quase nocauteado: balança, cabeça mole e estrelinhas em volta.
    if (f.hp > 0 && f.hp < f.maxHp * 0.22f && !isHeroic) {
        r.headRot = std::sin(r.time * 2.3f) * 0.13f;
        r.headShiftY = 0.012f;
        r.skew = std::sin(r.time * 1.6f) * 0.07f;
        r.stars = true;
        if (f.state == FIGHTER_IDLE || f.state == FIGHTER_BLOCK || f.state == FIGHTER_TELEGRAPH) r.face = 3;
    }
    // Exausto: ofegante e gota de suor cartoon escorrendo
    if (f.isExhausted && f.hp > 0 && !isHeroic && f.state != FIGHTER_KNOCKDOWN) {
        r.headShiftY += 0.018f + 0.008f * std::sin(r.time * 4.5f);
        r.squash *= 1.0f - 0.02f * std::sin(r.time * 4.5f);
        r.sweat = true;
    }
    if (isHeroic) {
        r.stars = false;
        r.face = 0;
        r.sweat = false;
    }
    return r;
}

void RingRenderer::queueBody(const SpriteFrame& frame, const Vec3& world, float w, float h, float skew, float layer, sf::Color tint,
                             const Reaction& rx, uint32_t charId, bool backView, bool overlays, bool drawSprite) {
    auto p = camera_.project(world);
    if (!p) return;
    const float sw = w * p->scale, sh = h * p->scale;
    const sf::Vector2f base = p->pos;
    auto at = [&](float lx, float ly) { return sf::Vector2f{base.x + lx + skew * ly, base.y + ly}; };
    sf::Vector2f tl = at(-sw / 2, -sh), tr = at(sw / 2, -sh), br = at(sw / 2, 0.0f), bl = at(-sw / 2, 0.0f);
    if (rx.roll != 0.0f) {
        const sf::Vector2f c = (tl + br) * 0.5f;
        const float cs = std::cos(rx.roll), sn = std::sin(rx.roll);
        for (sf::Vector2f* v : {&tl, &tr, &br, &bl}) {
            sf::Vector2f d = *v - c;
            *v = {c.x + d.x * cs - d.y * sn, c.y + d.x * sn + d.y * cs};
        }
    }
    const FaceSpec& fs = kFaces[std::min<std::size_t>(charId, kFaces.size() - 1)];
    BodyMap m;
    m.tl = tl; m.tr = tr; m.bl = bl;
    const sf::Vector2f ex = tr - tl, ey = bl - tl;
    m.w = std::hypot(ex.x, ex.y); m.h = std::hypot(ey.x, ey.y);
    m.ax = m.w > 0 ? ex / m.w : sf::Vector2f{1, 0};
    m.ay = m.h > 0 ? ey / m.h : sf::Vector2f{0, 1};
    m.neck = fs.neck;
    // De costas o giro da cabeça aparece espelhado (vemos a nuca).
    m.headRot = rx.headRot;
    m.headScaleX = rx.headScaleX;
    m.headShift = m.ax * (rx.headShiftX * m.w) + m.ay * (rx.headShiftY * m.h);

    const sf::FloatRect r(sf::Vector2f(frame.rect.position), sf::Vector2f(frame.rect.size));
    const sf::Texture* texture = frame.texture;
    auto quad = [&](std::array<sf::Vector2f, 4> pts, float v0, float v1, float lay) {
        const float ty0 = r.position.y + r.size.y * v0, ty1 = r.position.y + r.size.y * v1;
        const sf::Vector2f t0{r.position.x, ty0}, t1{r.position.x + r.size.x, ty0}, t2{r.position.x + r.size.x, ty1}, t3{r.position.x, ty1};
        std::array<sf::Vertex, 6> v = {{{pts[0], tint, t0}, {pts[1], tint, t1}, {pts[2], tint, t2}, {pts[0], tint, t0}, {pts[2], tint, t2}, {pts[3], tint, t3}}};
        queue_.push_back({lay, p->depth, [v, texture](sf::RenderTarget& t) {
                              sf::RenderStates states;
                              states.texture = texture;
                              t.draw(v.data(), v.size(), sf::PrimitiveType::Triangles, states);
                          }});
    };
    const bool headMoves = std::fabs(rx.headRot) > 0.001f || std::fabs(rx.headScaleX - 1) > 0.001f || rx.headShiftX != 0 || rx.headShiftY != 0;
    if (!drawSprite) {
    } else if (!headMoves) {
        quad({m.flat(0, 0), m.flat(1, 0), m.flat(1, 1), m.flat(0, 1)}, 0, 1, layer);
    } else {
        // Tronco inteiro embaixo; a cabeça (com um pouco do pescoço) desenhada por cima, girada.
        const float n = fs.neck, ext = std::min(1.0f, n + 0.12f);
        quad({m.flat(0, n), m.flat(1, n), m.flat(1, 1), m.flat(0, 1)}, n, 1, layer);
        quad({m.head(0, 0), m.head(1, 0), m.head(1, ext), m.head(0, ext)}, 0, ext, layer + 0.002f);
    }
    if (!overlays) return;
    const float time = rx.time;
    if (rx.face > 0 && !backView && !rx.xxFace) {
        const int face = rx.face;
        FaceSpec tinted = fs;
        tinted.skin = sf::Color(fs.skin.r * tint.r / 255, fs.skin.g * tint.g / 255, fs.skin.b * tint.b / 255);
        queue_.push_back({layer + 0.004f, p->depth, [m, fs = tinted, face, time](sf::RenderTarget& t) {
                              cv::Canvas ctx(t);
                              drawFace(ctx, m, fs, face, time);
                          }});
    }
    if (rx.drops) {
        // Gotas de suor/saliva voando para o lado do golpe
        const float hp = rx.hitP, d = rx.dir;
        queue_.push_back({layer + 0.006f, p->depth, [m, hp, d](sf::RenderTarget& t) {
                              cv::Canvas ctx(t);
                              const sf::Vector2f o = m.head(0.5f + d * 0.12f, m.neck - 0.05f);
                              for (int i = 0; i < 5; ++i) {
                                  const float k = hp * (1.0f + i * 0.35f);
                                  const float x = o.x + d * k * m.w * (0.6f + 0.15f * i), y = o.y - k * m.w * 0.35f * (1 - k) + k * k * m.w * 0.5f - i * 3;
                                  const float r = std::max(1.5f, m.w * (0.018f - i * 0.002f));
                                  ctx.globalAlpha = std::clamp(1.2f - hp * 1.4f, 0.0f, 1.0f);
                                  ctx.fillStyle = i % 2 ? "#ffffff" : "#9be7ff";
                                  ctx.beginPath(); ctx.arc(x, y, r, 0, 2 * kPi); ctx.fill();
                              }
                          }});
    }
    if (rx.stars) {
        queue_.push_back({layer + 0.008f, p->depth, [m, time](sf::RenderTarget& t) {
                              cv::Canvas ctx(t);
                              const sf::Vector2f top = m.head(0.5f, 0.02f);
                              const float rx2 = m.w * 0.32f, ry = m.w * 0.08f;
                              for (int i = 0; i < 3; ++i) {
                                  const float a = time * 4.5f + i * 2 * kPi / 3;
                                  const float x = top.x + std::cos(a) * rx2, y = top.y - m.w * 0.06f + std::sin(a) * ry;
                                  const float r = m.w * 0.045f * (0.8f + 0.25f * std::sin(a));
                                  star(ctx, x, y, r, a);
                                  ctx.fillStyle = "#ffe600"; ctx.fill();
                                  ctx.strokeStyle = "#0b0712"; ctx.lineWidth = std::max(1.0f, r * 0.25f); ctx.stroke();
                              }
                          }});
    }
    if (rx.sweat) {
        // Gota de suor cartoon estilo HQ escorrendo na têmpora quando exausto
        queue_.push_back({layer + 0.007f, p->depth, [m, time](sf::RenderTarget& t) {
                              cv::Canvas ctx(t);
                              const sf::Vector2f headPos = m.head(0.72f, 0.22f);
                              const float drip = std::fmod(time * 1.6f, 1.0f);
                              const float sx = headPos.x, sy = headPos.y + drip * m.w * 0.07f;
                              const float r = std::max(2.5f, m.w * 0.022f * (1.0f - drip * 0.25f));
                              ctx.fillStyle = "#38bdf8";
                              ctx.beginPath();
                              ctx.arc(sx, sy, r, 0, 2 * kPi);
                              ctx.fill();
                              if (drip > 0.35f) {
                                  ctx.fillStyle = "rgba(56, 189, 248, 0.65)";
                                  ctx.beginPath();
                                  ctx.arc(sx, sy - r * 1.6f, r * 0.55f, 0, 2 * kPi);
                                  ctx.fill();
                              }
                              ctx.fillStyle = "#ffffff";
                              ctx.beginPath();
                              ctx.arc(sx - r * 0.3f, sy - r * 0.3f, r * 0.35f, 0, 2 * kPi);
                              ctx.fill();
                          }});
    }
}

} // namespace KoRama
