// Resto do elenco montado por partes: as peças saem recortadas da própria arte de cada lutador
// (tools/export_sprites/rig_auto.js), no quadro 0..1000, uma por quadro de animação do sprite
// original. Variações e poses especiais de skill que não têm peças continuam com o sprite inteiro.
#include "rig_pose.hpp"
#include <array>

namespace KoRama {

using namespace rig;

namespace {

struct RigSpec {
    const char* prefix;
    const char* front;   // nome do sprite-base de frente (com _k se tiver quadros)
    const char* back;
    float cx, hw;        // centro e meia largura da caixa da cabeça
    float neck, waist, hip, legLen; // hip = 0: sem pernas separadas
    std::array<float, 8> shift;     // sobe-e-desce da cabeça por quadro (de frente)
};

const std::array<RigSpec, 8> kRigSpecs = {{
    {nullptr, nullptr, nullptr, 0, 0, 0, 0, 0, 0, {}}, // Jack tem montagem própria
    {"frog", "frogFrontSpriteFrames", "frogBackSprite", 500, 330, 470, 700, 0, 0, {}},
    {"joana", "joanaFrontSprite", "joanaBackSprite", 505, 255, 380, 650, 800, 170, {}},
    {"cremoso", "cremosoFrontFrames", "cremosoBackFrames", 500, 225, 460, 700, 0, 0, {}},
    {"valeria", "valeriaFrontFrames", "valeriaBackFrames", 500, 230, 410, 650, 770, 200, {}},
    {"vivaldi", "vivaldiFrontFrames", "vivaldiBackFrames", 500, 300, 480, 700, 0, 0, {}},
    {"espinha", "espinhaFrontFrames", "espinhaBackFrames", 500, 215, 440, 620, 700, 280, {0, -8, 0, 8}},
    {"marcov", "marcovFrontFrames", "marcovBackFrames", 500, 275, 400, 560, 640, 300, {0, -19, 0, 19}},
}};

// Quadro k se o nome for um sprite-base do lutador (senão -1).
int baseFrame(const std::string& name, const char* base) {
    const std::string b = base;
    if (name == b) return 0;
    if (name.size() > b.size() + 1 && name.compare(0, b.size(), b) == 0 && name[b.size()] == '_') {
        const std::string tail = name.substr(b.size() + 1);
        if (!tail.empty() && tail.find_first_not_of("0123456789") == std::string::npos) return std::stoi(tail);
    }
    return -1;
}

} // namespace

bool RingRenderer::rigBase(uint32_t charId, const std::string& bodyName, bool backView) {
    if (charId < 1 || charId > 7) return false;
    const RigSpec& sp = kRigSpecs[charId];
    return baseFrame(bodyName, backView ? sp.back : sp.front) >= 0;
}

bool RingRenderer::queueFighterRig(const Fighter& f, const Reaction& rx, const std::string& bodyName, const Vec3& world, float w, float h,
                                   float skew, float layer, sf::Color tint, bool backView) {
    if (f.charId < 1 || f.charId > 7) return false;
    const RigSpec& sp = kRigSpecs[f.charId];
    const bool spin = rx.sprite.rfind("RxSpin_", 0) == 0;
    // Corpo tem de ser um sprite-base (as reações viram pose aqui; variações usam o sprite inteiro).
    int k = baseFrame(bodyName, backView ? sp.back : sp.front);
    if (k < 0) {
        const bool rxName = bodyName.find("Rx") != std::string::npos;
        if (!rxName) return false;
        k = 0;
    }
    const std::string pre = std::string(sp.prefix) + "Rig";
    auto part = [this, &pre](const std::string& n) { return sprite(pre + n); };
    if (!part("F0_torso")) return false;
    auto p = camera_.project(world);
    if (!p) return false;

    constexpr float U = 1000.0f;
    const float s = U / 120.0f; // poses vêm em unidades do Jack
    const float sw = w * p->scale, sh = h * p->scale;
    const sf::Vector2f base = p->pos;
    auto at = [&](float lx, float ly) { return sf::Vector2f{base.x + lx + skew * ly, base.y + ly}; };
    sf::Vector2f tl = at(-sw / 2, -sh), tr = at(sw / 2, -sh), br = at(sw / 2, 0.0f), bl = at(-sw / 2, 0.0f);
    if (rx.roll != 0.0f) {
        const sf::Vector2f c = (tl + br) * 0.5f;
        const float cs = std::cos(rx.roll), sn = std::sin(rx.roll);
        for (sf::Vector2f* v : {&tl, &tr, &br, &bl}) {
            const sf::Vector2f d = *v - c;
            *v = {c.x + d.x * cs - d.y * sn, c.y + d.x * sn + d.y * cs};
        }
    }
    const sf::Vector2f ex = (tr - tl) / U, ey = (bl - tl) / U;
    auto toScreen = [&](sf::Vector2f u) { return tl + ex * u.x + ey * u.y; };

    const uint32_t frame = engine_ ? engine_->frameIndex : frameCounter_;
    const bool ringTurning = engine_ && engine_->gameState == STATE_FIGHTING;
    const Pose q = makePose(f, rx, backView, frame, celebrating_, h, ringTurning, sp.hip > 0 ? sp.legLen / s : 31.0f);
    if (spin || q.back != backView) k = 0;
    const std::string v = (q.back ? "B" : "F") + std::to_string(k);
    if (!part(v + "_torso")) return false;

    const float cx = sp.cx;
    const Aff root = around(cx, U, 0, q.bodySx * (q.mirrorAll ? -1.0f : 1.0f), 1);
    const Aff torso = root * move(q.torsoDx * s, q.torsoDy * s) * around(cx, sp.waist, q.torsoRot, q.torsoSx, q.torsoSy);
    Aff head = torso * move(q.headDx * s, q.headDy * s) * around(cx, sp.neck, q.headRot, q.headSx, q.headSy);
    if (q.headMirror) head = head * around(cx, sp.neck, 0, -1, 1);
    const Aff legL = root * move(q.legLDx * s, q.legLDy * s) * around(cx - 80, sp.hip, q.legLRot, 1, q.legSy);
    const Aff legR = root * move(q.legRDx * s, q.legRDy * s) * around(cx + 80, sp.hip, q.legRRot, 1, q.legSy);

    float lay = layer;
    auto draw = [&](const std::string& name, const Aff& m) {
        auto fr = part(name);
        if (!fr) return;
        const float kk = U / static_cast<float>(std::max(1, fr->fullSize.x));
        const float x0 = fr->trimOffset.x * kk, y0 = fr->trimOffset.y * kk;
        const float x1 = x0 + fr->rect.size.x * kk, y1 = y0 + fr->rect.size.y * kk;
        const std::array<sf::Vector2f, 4> pts = {toScreen(m({x0, y0})), toScreen(m({x1, y0})), toScreen(m({x1, y1})), toScreen(m({x0, y1}))};
        const sf::FloatRect r(sf::Vector2f(fr->rect.position), sf::Vector2f(fr->rect.size));
        const sf::Vector2f t0 = r.position, t1{r.position.x + r.size.x, r.position.y}, t2 = r.position + r.size, t3{r.position.x, r.position.y + r.size.y};
        const std::array<sf::Vertex, 6> vx = {{{pts[0], tint, t0}, {pts[1], tint, t1}, {pts[2], tint, t2}, {pts[0], tint, t0}, {pts[2], tint, t2}, {pts[3], tint, t3}}};
        const sf::Texture* texture = fr->texture;
        queue_.push_back({lay, p->depth, [vx, texture](sf::RenderTarget& tg) {
                              sf::RenderStates states;
                              states.texture = texture;
                              tg.draw(vx.data(), vx.size(), sf::PrimitiveType::Triangles, states);
                          }});
        lay += 0.0005f;
    };

    if (sp.hip > 0) { draw(v + "_legL", legL); draw(v + "_legR", legR); }
    draw(v + "_torso", torso);
    if (!q.back) {
        draw(v + (q.head == 2 ? "_headSide" : (q.head == 1 ? "_head3q" : "_head")), head);
        if (q.head != 2) {
            // No três quartos o rosto acompanha o achatamento da cabeça.
            const Aff face = q.head == 1 ? head * move(sp.hw * 0.1f, 0) * around(cx, sp.neck, 0, 0.8f, 1) : head;
            const Aff gen = face * move(0, sp.shift[static_cast<std::size_t>(k) % 8]);
            const std::string e = q.eyes;
            if (e == "normal" || e == "angry" || e == "tired") {
                draw(v + "_eyesN", face);
                if (e == "angry") draw("E_brows", gen);
                if (e == "tired") draw("E_lids", gen);
            } else {
                draw("E_" + e, gen);
            }
            const std::string m = q.mouth;
            if (m == "normal") draw(v + "_mouthN", face);
            else draw("M_" + m, gen);
        }
    } else {
        draw(v + (q.head == 1 ? "_head3q" : "_head"), head);
    }
    return true;
}

} // namespace KoRama
