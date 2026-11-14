// Jack montado por partes (piloto do esqueleto 2D): pernas, capa, tronco, cabeça e rosto trocável,
// cada um girando em volta do próprio ponto de giro. As partes vêm de tools/export_sprites/rig_jack.js
// (folha "jack", nomes jackRig*), recortadas mas com a posição no quadro 120x120 original.
#include "rig_pose.hpp"
#include "skills.hpp"
#include <algorithm>
#include <array>
#include <cmath>

namespace KoRama {

using namespace rig;

bool RingRenderer::queueJackRig(const Fighter& f, const Reaction& rx, const Vec3& world, float w, float h, float skew, float layer,
                                sf::Color tint, bool backView) {
    auto part = [this](const std::string& n) { return sprite("jackRig" + n); };
    if (!part("F_torso") || !part("B_torso")) return false;
    auto p = camera_.project(world);
    if (!p) return false;

    // Quadro do corpo na tela (mesmo do sprite inteiro), girado pela cambalhota.
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
    const sf::Vector2f ex = (tr - tl) / kUnits, ey = (bl - tl) / kUnits;
    auto toScreen = [&](sf::Vector2f u) { return tl + ex * u.x + ey * u.y; };

    const uint32_t frame = engine_ ? engine_->frameIndex : frameCounter_;
    const bool ringTurning = engine_ && engine_->gameState == STATE_FIGHTING;
    const Pose q = makePose(f, rx, backView, frame, celebrating_, h, ringTurning);

    Aff root = around(60, 106, 0, q.bodySx * (q.mirrorAll ? -1.0f : 1.0f), 1);
    const float hipY = q.back ? 80.0f : 76.0f, neckY = q.back ? 46.0f : 50.0f;
    const Aff torso = root * move(q.torsoDx, q.torsoDy) * around(60, 74, q.torsoRot, q.torsoSx, q.torsoSy);
    Aff head = torso * move(q.headDx, q.headDy) * around(60, neckY, q.headRot, q.headSx, q.headSy);
    if (q.headMirror) head = head * around(60, neckY, 0, -1, 1);
    // Pernas: encolhem a partir do quadril e giram nele.
    const Aff legL = root * move(q.legLDx, q.legLDy) * around(q.back ? 48.0f : 49.0f, hipY, q.legLRot, 1, q.legSy);
    const Aff legR = root * move(q.legRDx, q.legRDy) * around(q.back ? 72.0f : 71.0f, hipY, q.legRRot, 1, q.legSy);

    // Variações: Pele de Mel troca as peças de pele (sufixo H); Melaço e Geleia Real somam manchas.
    const bool casting = f.state == FIGHTER_SKILL;
    const bool molasses = f.status.molasses > 0 || (casting && f.activeSkill == SkillId::JackMolasses);
    const bool jelly = casting && f.activeSkill == SkillId::JackRoyalJelly;
    const bool honey = f.loadout.has(SkillId::JackHoneySkin);
    auto skin = [&](const std::string& n) { return honey && part(n + "H") ? n + "H" : n; };

    float lay = layer;
    auto draw = [&](const std::string& name, const Aff& m) {
        auto fr = part(name);
        if (!fr) return;
        const float k = kUnits / static_cast<float>(std::max(1, fr->fullSize.x));
        const float x0 = fr->trimOffset.x * k, y0 = fr->trimOffset.y * k;
        const float x1 = x0 + fr->rect.size.x * k, y1 = y0 + fr->rect.size.y * k;
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

    if (!q.back) {
        draw("F_coat", torso);
        draw(skin("F_legL"), legL);
        draw(skin("F_legR"), legR);
        draw("F_torso", torso);
        if (molasses) draw("F_stainM", torso);
        if (jelly) draw("F_stainJ", torso);
        draw(skin(q.head == 2 ? "F_headSide" : (q.head == 1 ? "F_head3q" : "F_head")), head);

        if (q.head != 2) {
            // No três quartos os traços deslizam para o lado do giro e o olho de trás estreita.
            const Aff face = q.head == 1 ? head * move(3, 0) * around(62, 40, 0, 0.85f, 1) : head;
            if (honey) draw("F_beardH", face);
            if (molasses) draw("F_shadeM", face);
            else draw(skin(std::string("E_") + q.eyes), face);
            draw(std::string("M_") + q.mouth, face);
        }
    } else {
        draw(skin("B_legL"), legL);
        draw(skin("B_legR"), legR);
        draw("B_torso", torso);
        if (molasses) draw("B_stainM", torso);
        if (jelly) draw("B_stainJ", torso);
        draw(skin(q.head == 1 ? "B_head3q" : "B_head"), head);
    }
    return true;
}

} // namespace KoRama
