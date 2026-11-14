// Visual novo do elenco (live_rig.hpp, exportado do modo jogo da apresentação): só o corpo por dentro
// (cabeça, chapéu, capa, pernas, rosto) vem da animação; ele ocupa a mesma caixa do sprite inteiro, com a
// mesma inclinação e giro. Luvas, trajetórias, tamanhos e o movimento do corpo inteiro continuam do jogo.
#include "ring_renderer.hpp"
#include "engine.hpp"
#include "vfx/psx_bee.hpp"
#include <algorithm>
#include <cmath>

namespace KoRama {

std::string RingRenderer::liveForm(const Fighter& f, const std::string& bodyName) const {
    auto has = [&](const char* s) { return bodyName.find(s) != std::string::npos; };
    switch (f.charId) {
    case 0: {
        const bool honey = f.loadout.has(SkillId::JackHoneySkin) || has("HoneySkin");
        const bool molasses = f.status.molasses > 0 || (f.state == FIGHTER_SKILL && f.activeSkill == SkillId::JackMolasses) || has("Molasses");
        if (molasses && honey) return "molassesHoney";
        if (molasses) return "molasses";
        if (f.state == FIGHTER_SKILL && f.activeSkill == SkillId::JackRoyalJelly) return "royal";
        if (honey) return "honey";
        return "base";
    }
    case 1: {
        const bool tox = has("Poison") || f.status.poisonSkin > 0 ||
                         (f.state == FIGHTER_SKILL && f.activeSkill == SkillId::FrogPoisonSkin && !f.signatureOnly);
        if (has("Inflated")) return tox ? "toxicInflated" : "inflated";
        if (has("NoCrown")) return tox ? "toxicNoCrown" : "nocrown";
        return tox ? "toxic" : "base";
    }
    case 2: return f.monstrous ? (f.status.shell>0 ? "deepArmor" : "deep") : (f.status.shell>0 ? "armor" : "base");
    case 3: {
        // O sabor vem da passiva (não do nome do sprite, que muda nas reações); a ação vem do sprite.
        const bool frozen = f.loadout.has(SkillId::CremosoFrozenStamina), revenge = f.loadout.has(SkillId::CremosoSweetRevenge);
        if (has("MeltBase") || f.status.melt > 0) return frozen ? "meltFrozen" : revenge ? "meltRevenge" : "melt";
        // Mouth stages are independent of passive flavor; never lose the
        // frozen/revenge surface while inflating or spitting the topping.
        return frozen ? "frozen" : revenge ? "revenge" : "base";
    }
    case 4: return f.coldMagma ? "cold" : "base";
    case 5:
        if (f.status.seasons > 0) { // Quatro Estações: troca de estação a cada 1,5 s
            static const char* kSeasons[4] = {"spring", "summer", "autumn", "winter"};
            const uint32_t frameNow = engine_ ? engine_->frameIndex : frameCounter_;
            return kSeasons[(frameNow / 90) % 4];
        }
        return "base";
    case 6: return f.loadout.has(SkillId::EspinhaVivaCaatinga) || (f.state == FIGHTER_SKILL && !f.signatureOnly && f.activeSkill == SkillId::EspinhaFridaTears) ? "bloom" : "base";
    case 7: return f.state == FIGHTER_SKILL && f.activeSkill == SkillId::MarcovZeroGravity ? "zero" : "base";
    case 8: return f.bossStage == 2 ? "stage2" : f.bossStage == 3 ? "stage3" : "base";
    default: return "base";
    }
}

// Ação do jogo -> animação exportada (nome e progresso 0..1; < 0 = em ciclo pelo relógio).
std::string RingRenderer::liveAction(const Fighter& f, bool groggy, float& prog) const {
    prog = f.maxTimer > 0 ? std::clamp(1.0f - static_cast<float>(f.timer) / static_cast<float>(f.maxTimer), 0.0f, 1.0f) : 0.0f;
    const bool isKnockdownLoser = engine_ && engine_->gameState == STATE_KNOCKDOWN &&
                                  ((f.isPlayer && !engine_->enemyIsDown) || (!f.isPlayer && engine_->enemyIsDown));
    const bool isMatchLoser = engine_ && engine_->isMatchOver() &&
                              (((engine_->result == MatchResult::WinKO || engine_->result == MatchResult::WinTKO || engine_->result == MatchResult::WinDecision) && !f.isPlayer) ||
                               ((engine_->result == MatchResult::LossKO || engine_->result == MatchResult::LossTKO || engine_->result == MatchResult::LossDecision) && f.isPlayer));

    if (f.state == FIGHTER_KNOCKDOWN || isKnockdownLoser || isMatchLoser) {
        prog = -1.0f;
        return "groggy"; // olhos tontos no chão ou cambaleando até as cordas
    }

    const bool isKnockdownWinner = engine_ && engine_->gameState == STATE_KNOCKDOWN &&
                                   ((f.isPlayer && engine_->enemyIsDown) || (!f.isPlayer && !engine_->enemyIsDown));
    const bool signing = f.signatureOnly && f.state == FIGHTER_SKILL;
    if (signing && (f.charId >= 1 && f.charId <= 8)) return "signature"; // starts at contact/reach frame, not an arbitrary global phase
    if (signing || f.state == FIGHTER_VICTORY_POSE || celebrating_ || isKnockdownWinner) { prog = -1.0f; return "signature"; }
    if (f.isPlayer && engine_ && engine_->chargeActive && f.state == FIGHTER_IDLE) {
        prog = engine_->chargeRatio();
        return engine_->chargeLeft ? "charge_L" : "charge_R";
    }
    const std::string hand = f.isLeftPunch ? "_L" : "_R";
    switch (f.state) {
    case FIGHTER_JAB: return "jab" + hand;
    case FIGHTER_CROSS: return "cross" + hand;
    case FIGHTER_HOOK: return "hook" + hand;
    case FIGHTER_BODY_BLOW: return "body" + hand;
    case FIGHTER_UPPER: return "upper" + hand;
    case FIGHTER_BLOCK: return "block_L";
    case FIGHTER_DODGE_L: return "dodgeL_L";
    case FIGHTER_DODGE_R: return "dodgeR_L";
    case FIGHTER_DODGE_BACK: return "dodgeBack_L";
    case FIGHTER_DUCK: return "duck_L";
    case FIGHTER_TELEGRAPH: return "telegraph_L";
    case FIGHTER_HIT: return f.hitKind >= 1 && f.hitKind <= 4 ? "hit" + std::to_string(f.hitKind) : "block_L";
    default:
        prog = -1.0f;
        if (groggy) return "groggy";
        return engine_ && engine_->gameState == STATE_FIGHTING ? "step" : "idle"; // passada durante a luta
    }
}

bool RingRenderer::queueLiveBody(const Fighter& f, const Reaction& rx, const std::string& bodyName, const Vec3& world, float w, float h,
                                 float skew, float layer, sf::Color tint, bool backView, bool& rigGloves) {
    rigGloves = false;
    if ((!liveVisual && !hasIntegratedSkin(f.charId)) || !sprites_ || f.charId > 8) return false;
    const LiveRig* rig = sprites_->rig(fighterRigKey(f.charId, f.skinId));
    if (!rig) return false;
    float prog;
    std::string act = liveAction(f, rx.stars, prog);
    float creamPuddle = 0.0f;
    if (hasIntegratedSkin(f.charId) && rx.sprite.rfind("RxSpin_", 0) == 0) {
        // Antes de tocar as cordas ele ainda está tonto; o giro começa no ricochete.
        act = koRopeFrame_ < 0 ? "groggy" : "spin" + rx.sprite.substr(7);
        prog = koRopeFrame_ < 0 ? -1.0f : 0.0f;
    }
    if (f.charId == 1 && f.state == FIGHTER_SKILL && act != "signature") {
        // The actual lab controller owns mouth/contact timing, not the frame
        // clock or a recreated animation. Keep body movement in world space.
        const auto& fx = lab_[f.isPlayer ? 0 : 1];
        if (fx) { act = "mouth_" + fx->mouthStage(labContext(f, f.isPlayer)); prog = 0.0f; }
    }
    const std::string form = liveForm(f, bodyName);
    if (f.charId == 3 && act != "signature" && act.rfind("spin",0) != 0 && act != "groggy") {
        const auto& fx = lab_[f.isPlayer ? 0 : 1];
        if (fx) {
            const std::string stage = fx->mouthStage(labContext(f,f.isPlayer));
            if(stage.rfind("Puddle",0)==0)creamPuddle=std::clamp(std::stof(stage.substr(6))/8.f,0.f,1.f);
            // The wide pooled surface has ordinary hit/dodge/punch clips too;
            // receiving a punch must not make the floor silhouette vanish.
            if (stage.rfind("Puddle",0)==0 && f.state!=FIGHTER_SKILL && act!="step" && act!="idle") {
                act="pooled_"+act;
            } else if (!stage.empty() && (f.state==FIGHTER_SKILL || act=="step" || act=="idle" || act=="block_L")) {
                act = stage.rfind("Inflated",0)==0 || stage=="CherrySpit" ? "mouth_"+stage : stage;
                prog=-1;
            }
        }
    }
    if (f.charId==2 && act!="signature" && f.state==FIGHTER_SKILL && !f.signatureOnly) {
        const float p=f.maxTimer>0 ? 1.0f-static_cast<float>(f.timer)/f.maxTimer : 0;
        auto ease=[](float x){x=std::clamp(x,0.0f,1.0f);return x*x*(3-2*x);};
        if(f.activeSkill==SkillId::JoanaOceanBounty) {
            const float grow=ease(p/.20f)*(1-ease((p-.72f)/.22f));
            act="balm_"+std::to_string(static_cast<int>(std::round(grow*8)));prog=0;
        } else if(f.activeSkill==SkillId::JoanaCrustaceanShell) {
            act="shell_"+std::to_string(static_cast<int>(std::round(ease(p/.65f)*8)));prog=0;
        }
    }
    const bool clipBack = (act == "signature" || (hasIntegratedSkin(f.charId) && act.rfind("spin", 0) == 0)) ? false : backView;
    const LiveRig::Clip* c = rig->clip(form, clipBack, act);
    if (!c) c = rig->clip(form, clipBack, "idle");
    if (!c) c = rig->clip("base", clipBack, act);
    if (!c) c = rig->clip("base", clipBack, "idle");
    if (!c || c->frames.empty()) return false;
    rigGloves = act == "signature";

    const int n = static_cast<int>(c->frames.size());
    const uint32_t frameNow = engine_ ? engine_->frameIndex : frameCounter_;
    const bool creamShape = f.charId==3 && (act.rfind("mouth_",0)==0 || act.rfind("Melt",0)==0 || act.rfind("Puddle",0)==0 || act.rfind("Vortex",0)==0);
    const uint32_t clipClock=creamShape ? frameNow/4 : frameNow; // body-shape surface at 15 Hz; world motion stays 60 Hz
    int i;
    if (prog < 0.0f) i = c->loop ? static_cast<int>(clipClock % static_cast<uint32_t>(n)) : std::min(n - 1, static_cast<int>(clipClock % static_cast<uint32_t>(n + 30)));
    else i = std::min(n - 1, static_cast<int>(prog * (n - 1) + 0.5f));
    const LiveRig::Frame& fr = c->frames[static_cast<std::size_t>(i)];
    float signatureRecoil = 0.0f;
    if (f.charId == 1 && f.skinId != 2 && act == "signature") {
        const float phase = static_cast<float>(i) / std::max(1, n - 1);
        for (const float contact : {.57f, .71f}) if (phase >= contact && phase < contact + .09f) {
            const float q = (phase-contact)/.09f;
            signatureRecoil += 1.5f * (1-q*q*(3-2*q));
        }
    }

    // Passada contínua durante a defesa: os pés mantêm o ritmo do boxe em vez de travarem
    const LiveRig::Frame* stepFrame = nullptr;
    if (act == "block_L" && engine_ && engine_->gameState == STATE_FIGHTING) {
        const LiveRig::Clip* stepClip = rig->clip(form, backView, "step");
        if (!stepClip) stepClip = rig->clip("base", backView, "step");
        if (stepClip && !stepClip->frames.empty()) {
            const int sn = static_cast<int>(stepClip->frames.size());
            const int si = stepClip->loop ? static_cast<int>(frameNow % static_cast<uint32_t>(sn))
                                          : std::min(sn - 1, static_cast<int>(frameNow % static_cast<uint32_t>(sn + 30)));
            stepFrame = &stepClip->frames[static_cast<std::size_t>(si)];
        }
    }

    // Mesma caixa de queueBody: base no meio dos pés, cisalhamento e giro do corpo.
    auto p = camera_.project(world);
    if (!p) return false;
    const float sw = w * p->scale, sh = h * p->scale;
    const sf::Vector2f base = p->pos;
    auto at = [&](float lx, float ly) { return sf::Vector2f{base.x + lx + skew * ly, base.y + ly}; };
    const sf::Vector2f tl0 = at(-sw / 2, -sh), br0 = at(sw / 2, 0.0f), ctr = (tl0 + br0) * 0.5f;
    const float cs = std::cos(rx.roll), sn = std::sin(rx.roll);
    // O sprite do jogo é o viewBox centrado num quadro quadrado (como foi desenhado no atlas).
    const float side = std::max(c->box[2], c->box[3]);
    const float bx0 = c->box[0] + c->box[2] / 2 - side / 2, by0 = c->box[1] + c->box[3] / 2 - side / 2;
    auto toScreen = [&](float x, float y) {
        // Joana's right claw belongs to the same anatomical side in both cameras.
        // The 3D engine mirrors local X in the frontal camera; mirror the authored
        // body/signature to match it, rather than moving world-space attack paths.
        const float joanaFacing=f.charId==2 && !clipBack ? -1.0f : 1.0f;
        const float lx = ((x - bx0) / side - 0.5f) * sw * joanaFacing * (hasIntegratedSkin(f.charId) && rx.mirror ? -1.0f : 1.0f);
        const float ly = ((y + signatureRecoil - by0) / side - 1.0f) * sh;
        sf::Vector2f v = at(lx, ly);
        if (rx.roll != 0.0f) { const sf::Vector2f d = v - ctr; v = {ctr.x + d.x * cs - d.y * sn, ctr.y + d.x * sn + d.y * cs}; }
        if(creamPuddle>0){
            // The complete apron-to-puddle transition settles onto a shallow
            // world-floor surface, not just the single lowest texture pixel.
            // Keep native recoil on the head, but never roll the floor cream.
            const float settle=creamPuddle*std::clamp((y-92.f)/16.f,0.f,1.f);
            const auto floor=camera_.project({world.x,0,world.z+(116.f-y)*h/side*.4f});
            if(floor){
                const sf::Vector2f flat{floor->pos.x+lx*floor->scale/p->scale,floor->pos.y};
                v+=(flat-v)*settle;
            }
        }
        return v;
    };

    const LiveRig::Inst* torsoInst = nullptr;
    if (f.charId == 1 || f.charId == 0) {
        for (std::uint32_t j = 0; j < fr.count; ++j) {
            const LiveRig::Inst* pIn = &rig->insts[fr.first + j];
            if (pIn->slotKind == LiveRig::SlotTorso || (f.charId == 1 && fighterRigKey(f.charId, f.skinId) == "frog" && (pIn->part == 0 || pIn->part == 359))) {
                torsoInst = pIn;
                break;
            }
        }
    }

    struct Batch { const sf::Texture* tex; std::vector<sf::Vertex> v; };
    std::vector<Batch> batches;
    for (std::uint32_t j = 0; j < fr.count; ++j) {
        const LiveRig::Inst* pIn = &rig->insts[fr.first + j];
        if (stepFrame && (pIn->slotKind == LiveRig::SlotLegL || pIn->slotKind == LiveRig::SlotLegR)) {
            // A leg assembly can contain thigh, boot and trim as separate parts.
            // Keep each part's identity when borrowing the walking pose for guard.
            for (std::uint32_t k = 0; k < stepFrame->count; ++k) {
                const auto* candidate = &rig->insts[stepFrame->first + k];
                if (candidate->part == pIn->part && candidate->slotKind == pIn->slotKind) {
                    pIn = candidate;
                    break;
                }
            }
        }
        LiveRig::Inst in = *pIn;
        const LiveRig::Part& pt = rig->parts[static_cast<std::size_t>(in.part)];
        const sf::Texture* tex = rig->sheets[static_cast<std::size_t>(pt.sheet)].get();
        const float pw = pt.rect.size.x * pt.upp, ph = pt.rect.size.y * pt.upp;
        auto corner = [&](float u, float v, const LiveRig::Inst& it) {
            const float px = pt.ox + u * pw, py = pt.oy + v * ph;
            return toScreen(it.a * px + it.c * py + it.e, it.b * px + it.d * py + it.f);
        };
        if (batches.empty() || batches.back().tex != tex) batches.push_back({tex, {}});
        auto& v = batches.back().v;

        // No King Frog, as peças das pernas contêm tanto o calção (topo, y <= 105 px) quanto as pernas/pés (base, y >= 106 px).
        // 1) As pernas e pés (base) são desenhados PRIMEIRO (camada abaixo do calção), com passo e passada livres.
        // 2) O calção (topo) é desenhado DEPOIS (camada acima das pernas), seguindo rigidamente o tronco (torso) sem pedaço de calção nas pernas.
        // Only legacy Frog atlases combine shorts/legs at the 106-pixel seam.
        // The approved skin rigs have independent vector-owned shorts/legs.
        const bool isFrogLeg = (f.charId == 1 && fighterRigKey(f.charId, f.skinId) == "frog" && torsoInst && (in.slotKind == LiveRig::SlotLegL || in.slotKind == LiveRig::SlotLegR));
        if (isFrogLeg && pt.rect.size.y > 110.0f) {
            const float vSplit = std::clamp(106.0f / pt.rect.size.y, 0.05f, 0.95f);
            const float ySplit = pt.rect.position.y + 106.0f;

            // Extensão das pernas para cima (entrando fundo no calção para não parecer cortada quando o calção inclina)
            constexpr float kExtraUp = 55.0f;
            const float vTop = std::clamp((106.0f - kExtraUp) / pt.rect.size.y, 0.05f, 0.95f);
            const float yLegTop = pt.rect.position.y + 106.0f; // textura pura da perna verde

            const sf::Vector2f t0 = pt.rect.position;
            const sf::Vector2f t1 = {t0.x + pt.rect.size.x, t0.y};
            const sf::Vector2f tm0 = {t0.x, ySplit};
            const sf::Vector2f tm1 = {t0.x + pt.rect.size.x, ySplit};
            const sf::Vector2f tLeg0 = {t0.x, yLegTop};
            const sf::Vector2f tLeg1 = {t0.x + pt.rect.size.x, yLegTop};
            const sf::Vector2f t2 = pt.rect.position + pt.rect.size;
            const sf::Vector2f t3 = {t0.x, t2.y};

            // 1. Camada ABAIXO: Pernas e pés mais compridos para cima (entrando fundo no calção), livres na passada
            sf::Color colLeg = tint;
            colLeg.a = static_cast<std::uint8_t>(std::clamp(tint.a * in.op * fr.opacity, 0.0f, 255.0f));
            const sf::Vector2f ql0 = corner(0, vTop, in);
            const sf::Vector2f ql1 = corner(1, vTop, in);
            const sf::Vector2f ql2 = corner(1, 1, in);
            const sf::Vector2f ql3 = corner(0, 1, in);
            v.push_back({ql0, colLeg, tLeg0}); v.push_back({ql1, colLeg, tLeg1}); v.push_back({ql2, colLeg, t2});
            v.push_back({ql0, colLeg, tLeg0}); v.push_back({ql2, colLeg, t2}); v.push_back({ql3, colLeg, t3});

            // 2. Camada ACIMA: Calção cobrindo o topo das pernas e acompanhando o tronco
            sf::Color colTorso = tint;
            colTorso.a = static_cast<std::uint8_t>(std::clamp(tint.a * torsoInst->op * fr.opacity, 0.0f, 255.0f));
            const sf::Vector2f qs0 = corner(0, 0, *torsoInst);
            const sf::Vector2f qs1 = corner(1, 0, *torsoInst);
            const sf::Vector2f qsm1 = corner(1, vSplit, *torsoInst);
            const sf::Vector2f qsm0 = corner(0, vSplit, *torsoInst);
            v.push_back({qs0, colTorso, t0}); v.push_back({qs1, colTorso, t1}); v.push_back({qsm1, colTorso, tm1});
            v.push_back({qs0, colTorso, t0}); v.push_back({qsm1, colTorso, tm1}); v.push_back({qsm0, colTorso, tm0});
        } else if(creamPuddle>.001f) {
            // Subdivide texture quads so the floor attachment bends below the
            // apron instead of shearing/stretching the whole torso and head.
            sf::Color col=tint;col.a=static_cast<std::uint8_t>(std::clamp(tint.a*in.op*fr.opacity,0.f,255.f));
            for(int strip=0;strip<16;++strip){
                const float lo=strip/16.f,hi=(strip+1)/16.f;
                const auto q0=corner(0,lo,in),q1=corner(1,lo,in),q2=corner(1,hi,in),q3=corner(0,hi,in);
                const sf::Vector2f t0{pt.rect.position.x,pt.rect.position.y+lo*pt.rect.size.y},t1{pt.rect.position.x+pt.rect.size.x,t0.y},t2{t1.x,pt.rect.position.y+hi*pt.rect.size.y},t3{t0.x,t2.y};
                v.push_back({q0,col,t0});v.push_back({q1,col,t1});v.push_back({q2,col,t2});v.push_back({q0,col,t0});v.push_back({q2,col,t2});v.push_back({q3,col,t3});
            }
        } else {
            sf::Color col = tint;
            col.a = static_cast<std::uint8_t>(std::clamp(tint.a * in.op * fr.opacity, 0.0f, 255.0f));
            const sf::Vector2f t0 = pt.rect.position, t1 = {t0.x + pt.rect.size.x, t0.y}, t2 = t0 + pt.rect.size, t3 = {t0.x, t2.y};
            const sf::Vector2f q0 = corner(0, 0, in), q1 = corner(1, 0, in), q2 = corner(1, 1, in), q3 = corner(0, 1, in);
            v.push_back({q0, col, t0}); v.push_back({q1, col, t1}); v.push_back({q2, col, t2});
            v.push_back({q0, col, t0}); v.push_back({q2, col, t2}); v.push_back({q3, col, t3});
        }
    }
    // A própria máscara alfa das peças compõe a sombra, sem elipse genérica.
    // No ringue, a projeção perspectiva literal de um billboard vertical vira
    // uma linha ilegível; comprimimos a pose no chão como decal de quadrinhos.
    // A direção lateral acompanha a luz fixa no mundo quando a câmera gira.
    if (creamPuddle < 0.1f && std::fabs(world.x) < 265.0f && std::fabs(world.z) < 265.0f) {
        const int slot = f.isPlayer ? 0 : 1;
        auto shadow = std::make_shared<ShadowData>();
        const float lateral = 0.10f * (std::cos(camera_.yaw) + std::sin(camera_.yaw));
        const float liftFade = std::clamp(1.0f - std::max(0.0f, f.offsetY) / 190.0f, 0.28f, 1.0f);
        shadow->opacity = static_cast<std::uint8_t>(88.0f * liftFade);
        const float refBottomY = (f.charId == 3) ? (backView ? base.y - sh * 0.17f : base.y - sh * 0.08f) : base.y;
        shadow->baseX = base.x;
        shadow->baseY = refBottomY;
        shadow->footY = (f.charId == 3) ? (refBottomY + 2.0f) : (base.y - sh * 0.10f);
        shadow->height = sh; shadow->lateral = lateral;
        shadow->pieces.reserve(batches.size() + 2);
        for (const Batch& batch : batches) {
            shadow->pieces.push_back({batch.tex, batch.v});
            for (sf::Vertex& vertex : shadow->pieces.back().vertices) {
                const float high = std::clamp(refBottomY - vertex.position.y, 0.0f, sh);
                vertex.position = {base.x + (vertex.position.x - base.x) * 0.78f + high * lateral,
                                   shadow->footY + high * 0.23f};
                vertex.color = sf::Color(255, 255, 255, vertex.color.a);
            }
        }
        pendingShadow_[slot] = shadow;
        queue_.push_back({1.26f, p->depth, [this, slot, shadow](sf::RenderTarget& target) {
            constexpr sf::Vector2u maskSize{640u, 384u};
            if (!shadowMask_[slot]) shadowMask_[slot] = std::make_unique<sf::RenderTexture>();
            auto& mask = *shadowMask_[slot];
            if (mask.getSize() != maskSize && !mask.resize(maskSize)) return;
            float minX = 1600.0f, minY = 900.0f;
            for (const auto& piece : shadow->pieces)
                for (const auto& vertex : piece.vertices) {
                    minX = std::min(minX, vertex.position.x);
                    minY = std::min(minY, vertex.position.y);
                }
            minX = std::floor(minX) - 2.0f;
            minY = std::floor(minY) - 2.0f;
            mask.clear(sf::Color::Transparent);
            for (const auto& piece : shadow->pieces) {
                sf::RenderStates st;
                st.texture = piece.texture;
                st.blendMode = sf::BlendMax; // união alfa: sobreposições não escurecem
                st.transform.translate({-minX, -minY});
                mask.draw(piece.vertices.data(), piece.vertices.size(), sf::PrimitiveType::Triangles, st);
            }
            mask.display();
            sf::Sprite silhouette(mask.getTexture());
            silhouette.setPosition({minX, minY});
            silhouette.setColor(sf::Color(7, 17, 22, shadow->opacity));
            target.draw(silhouette);
        }});
    }
    queue_.push_back({layer, p->depth, [batches = std::move(batches)](sf::RenderTarget& target) {
                          for (const auto& b : batches) {
                              sf::RenderStates st;
                              st.texture = b.tex;
                              target.draw(b.v.data(), b.v.size(), sf::PrimitiveType::Triangles, st);
                          }
                      }});
    // Continuous cigar smoke follows the same torso as the biting mouth.
    if (f.charId == 1 && f.skinId == 1 && !backView && act.rfind("spin", 0) != 0 &&
        act.rfind("hit", 0) != 0 && act != "groggy" && (act.rfind("mouth_",0) != 0 || act == "mouth_0" || act == "mouth_inflated" || act == "mouth_no_crown")) {
        const float seconds = frameNow / 60.0f;
        for (int k=0;k<3;++k) {
            const float age=std::fmod(seconds/2.8f+k/3.0f,1.0f);
            float x=99.0f+std::sin(age*7+k*.4f)*age*3, y=42.5f-age*21;
            if(torsoInst){const float ox=x;x=torsoInst->a*x+torsoInst->c*y+torsoInst->e;y=torsoInst->b*ox+torsoInst->d*y+torsoInst->f;}
            const auto pos=toScreen(x,y);const float radius=(.6f+age*1.7f)*std::abs(sw)/120.0f;
            const auto alpha=static_cast<std::uint8_t>(std::sin(age*3.14159265f)*.65f*tint.a);
            queue_.push_back({layer+.02f,p->depth,[pos,radius,alpha](sf::RenderTarget& target){sf::CircleShape puff(radius,12);puff.setOrigin({radius,radius});puff.setPosition(pos);puff.setFillColor(sf::Color(215,226,219,alpha));target.draw(puff);}});
        }
    }
    // Continuous smoker plumes; never baked into hundreds of textures.
    if (f.charId == 0 && f.skinId == 1 && act.rfind("spin", 0) != 0) {
        const float seconds = frameNow / 60.0f;
        for (int k = 0; k < 4; ++k) {
            const float age = std::fmod(seconds / 2.1f + k / 4.0f, 1.0f);
            float x=34.0f+std::sin(age*7+k*.7f)*age*3.5f, y=58-age*23;
            if(torsoInst) { const float ox=x; x=torsoInst->a*x+torsoInst->c*y+torsoInst->e; y=torsoInst->b*ox+torsoInst->d*y+torsoInst->f; }
            const sf::Vector2f at = toScreen(x,y);
            const float radius = (1.2f + age * 3.8f) * std::abs(sw) / 120.0f;
            const auto alpha = static_cast<std::uint8_t>(std::sin(age * 3.14159265f) * .72f * tint.a);
            queue_.push_back({layer + .02f, p->depth, [at, radius, alpha](sf::RenderTarget& target) {
                sf::CircleShape puff(radius, 14); puff.setOrigin({radius,radius}); puff.setPosition(at);
                puff.setFillColor(sf::Color(248,250,252,alpha)); target.draw(puff);
            }});
        }
    }
    // Approved alternative signatures retain native 3D bees and depth ordering.
    if (f.charId == 0 && f.skinId > 0 && act == "signature") {
        const float phase = static_cast<float>(i) / std::max(1, n - 1), seconds = phase * c->dur;
        auto ease = [](float x) { x = std::clamp(x,0.0f,1.0f); return x*x*(3-2*x); };
        const float reach = ease(phase/.18f)*(1-ease((phase-.96f)/.04f));
        const float drawers = ease((phase-.22f)/.10f)*(1-ease((phase-.91f)/.05f));
        for (int k=0;k<7;++k) {
            const float travel = ease((phase-(f.skinId==1?.35f:.30f)-k*.012f)/.15f)*(1-ease((phase-.73f-k*.008f)/.12f));
            if(travel<=.001f) continue;
            const float a=seconds*2+k*6.2831853f/7, sign=k%2?1.0f:-1.0f;
            const float px=f.skinId==1?(k%2?95.0f:25.0f)+sign*(6*reach+7*drawers):60;
            const float py=f.skinId==1?70.5f-16*reach-4.5f+3*drawers:55;
            const float x=px+(60+std::cos(a)*43-px)*travel, y=py+(47+std::sin(a)*19-py)*travel;
            // Convert authored local points to world coordinates, then project the mesh normally.
            Vec3 at=world; const Vec3 lateral=Physics3D::localToWorld((x-60)*w/120,0,std::sin(a)*14*travel,{},f.facing);
            at.x+=lateral.x;at.z+=lateral.z;at.y+=(120-y)*h/120;
            const Vec3 direction=Physics3D::localToWorld(std::cos(a),.25f,std::sin(a),{},f.facing);
            const float depth=camera_.depthOf(at), beeLayer=layer+(depth<p->depth?.1f:-.1f);
            const uint32_t skin=f.skinId;
            queue_.push_back({beeLayer,depth,[this,at,direction,seconds,k,skin](sf::RenderTarget& target){
                vfx::drawPsxBee(target,camera_,at,direction,5.9f,seconds*65+k,0,1,false,skin);
            }});
        }
    }
    return true;
}

} // namespace KoRama
