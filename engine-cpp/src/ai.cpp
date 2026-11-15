#include "ai.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>

namespace KoRama {

namespace {

constexpr float kPi = 3.14159265f;

float random01() {
    return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
}

// Especiais que atacam o jogador (golpe, gêiser, raio, estaca...) não saem do nada: o oponente arma antes (aviso no corpo e no chão).
// Curas, modos, escudos e o enxame só mexem em quem lança ou já pegam na hora, então saem direto.
constexpr int kSkillWarnFrames = 30;
bool needsWarning(SkillId id) {
    const SkillDef* d = SkillSystem::get(id);
    if (!d || d->type != SkillType::Command || !d->tag) return false;
    for (const char* t : {"HEAL", "SHIELD", "SWARM"}) if (std::string(d->tag) == t) return false;
    return true;
}
// arma o especial (FIGHTER_TELEGRAPH com o especial na fila); false = esse especial não pede aviso e sai na hora
bool warnSkill(Fighter& enemy, SkillId id) {
    if (id == SkillId::None || !needsWarning(id)) return false;
    enemy.queuedSkill = id; enemy.queuedSignature = false; enemy.tellKind = 13;
    enemy.startMove(FIGHTER_TELEGRAPH, kSkillWarnFrames);
    return true;
}

} // namespace

void EnemyAI::reset(uint32_t charId) {
    (void)charId;
    skillCooldown = 220 + std::rand() % 140; // primeira habilidade entre 4 e 6 s
    rolledDefenseForPunch = false;
    queue.clear();
    combo.clear();
    pendingRecovery = 0;
    afterSkill = false;
    castPoison = false;
    feintActive = false;
    tellBeats = 1;
    rockets = 0;
    lastPunchTimer = 0; sawPunch = false; rolledSkillDodge = false; defKind = defDelay = defHold = 0;
    guardSaw = dodgeSaw = countering = inHit = false; hitsTaken = blocksInRow = calm = 0; sinceDodge = 100000;
}

// ---------------------------------------------------------------- defesa reativa
// Escolhe entre guarda e esquiva para o golpe que chega em hitIn quadros e marca a reação (com um tempo de reação curto).
// Golpe pesado ou carregado pede mais esquiva; quem está apanhando em sequência fecha a guarda; especial só se evita esquivando.
void EnemyAI::planDefense(const Fighter& enemy, const Fighter& player, int hitIn, bool canBlock) {
    const bool heavy = player.state == FIGHTER_HOOK || player.state == FIGHTER_UPPER || player.chargeRatio > 0.3f;
    float dodge = !canBlock ? 1.0f : (heavy ? 0.6f : 0.4f);
    if (canBlock && hitsTaken >= 2) dodge *= 0.5f;
    if (canBlock && sinceDodge < 150) dodge *= 0.25f; // acabou de esquivar: agora prefere a guarda (duas esquivas seguidas giram o ringue demais)
    if (enemy.status.mud > 0) dodge = 0.0f; // na lama não dá para esquivar
    if (random01() < dodge) {
        defKind = 2;
        defDelay = std::max(hitIn - 11, std::min(3, hitIn - 2)); // a esquiva dura 22 quadros: sai para o acerto cair no meio dela
    } else if (canBlock) {
        defKind = 1;
        defDelay = std::min(2 + std::rand() % 4, std::max(0, hitIn - 3));
        defHold = hitIn - std::max(0, defDelay) + 9; // segura até o soco bater e mais um pouco
    } else {
        defKind = 0;
    }
    defDelay = std::max(0, defDelay);
}

void EnemyAI::startDefense(Fighter& enemy, uint32_t& audioEvents) {
    if (defKind == 1) {
        enemy.startMove(FIGHTER_BLOCK, std::max(14, defHold));
    } else if (defKind == 2) {
        enemy.startMove(random01() > 0.5f ? FIGHTER_DODGE_L : FIGHTER_DODGE_R, 22);
        audioEvents |= AUDIO_SFX_DODGE;
    }
    if (defKind == 2) sinceDodge = 0;
    defKind = 0; guardSaw = dodgeSaw = false;
}

// Resposta rápida depois de defender: aviso curto (a luva recua) e um golpe. Depois da guarda vem um soco leve; depois da esquiva, um gancho.
void EnemyAI::startCounter(Fighter& enemy, bool heavy) {
    queue.clear(); pendingRecovery = 0; feintActive = false;
    countering = true; blocksInRow = 0;
    enemy.queuedAttack = heavy ? FIGHTER_HOOK : (random01() > 0.5f ? FIGHTER_JAB : FIGHTER_CROSS);
    enemy.queuedHeavy = heavy;
    enemy.isLeftPunch = random01() > 0.5f;
    enemy.tellKind = 1;
    enemy.queuedSkill = SkillId::None; enemy.queuedSignature = false;
    enemy.startMove(FIGHTER_TELEGRAPH, heavy ? 14 : 10); // curto: precisa chegar antes do próximo soco de quem martela
}

// ---------------------------------------------------------------- padrões de luta (Jack e King Frog)
// Fases pela vida: 1 acima de 60%, 2 de 60% a 30%, 3 abaixo de 30% (mais rápido, mais armadilhas).
namespace {
int phaseOf(const Fighter& f) {
    const float r = f.maxHp > 0 ? f.hp / f.maxHp : 1.0f;
    return r > 0.6f ? 1 : (r > 0.3f ? 2 : 3);
}
} // namespace

void EnemyAI::choosePattern(Fighter& enemy) {
    queue.clear();
    pendingRecovery = 0;
    const int phase = phaseOf(enemy);
    const float speed = phase == 3 ? 0.85f : 1.0f; // avisos 15% mais curtos na reta final
    auto t = [&](int frames) { return std::max(18, static_cast<int>(frames * speed)); };
    const bool hand = random01() > 0.5f;
    struct Option { int weight; int id; };
    std::vector<Option> opts;
    if (enemy.charId == 0) { // Jack
        opts = {{3, 0}, {2, 1}};
        if (phase >= 2) opts.push_back({2, 2});
        const int total = [&] { int s = 0; for (auto& o : opts) s += o.weight; return s; }();
        int roll = std::rand() % total, pick = 0;
        for (auto& o : opts) { if (roll < o.weight) { pick = o.id; break; } roll -= o.weight; }
        if (pick == 0) { // Ferroada: aba do chapéu baixa, jab rápido (dupla na fase 3)
            queue.push_back({FIGHTER_JAB, hand, t(26), 1});
            if (phase == 3) queue.push_back({FIGHTER_JAB, !hand, 0, 1});
        } else if (pick == 1) { // Colmeia Dupla: bate as colmeias, gancho de um lado e do outro; brecha depois
            const bool feint = random01() < (phase == 3 ? 0.5f : 0.15f);
            Step a{FIGHTER_HOOK, true, t(44), 2};
            a.feint = feint;
            queue.push_back(a);
            queue.push_back({FIGHTER_HOOK, false, 0, 2});
            pendingRecovery = 72;
        } else { // Enxame + uppercut
            Step s1{FIGHTER_SKILL, hand, 0, 0};
            s1.skill = SkillId::JackInfestation;
            queue.push_back(s1);
            queue.push_back({FIGHTER_UPPER, hand, t(32), 3});
        }
        return;
    }
    auto skill = [&](SkillId id) { Step s1{FIGHTER_SKILL, hand, 0, 0}; s1.skill = id; return s1; };
    auto pickOf = [&](std::vector<Option> o) {
        int tot = 0;
        for (auto& x : o) tot += x.weight;
        int r = std::rand() % std::max(1, tot);
        for (auto& x : o) { if (r < x.weight) return x.id; r -= x.weight; }
        return o.empty() ? 0 : o.front().id;
    };
    tellBeats = 1;
    switch (enemy.charId) {
    case 2: { // Joana: garra pesada, arrastão do harpão (emenda um jab na fase 2+), âncora com brecha longa
        const int pick = pickOf({{3, 0}, {2, 1}, {2, 2}, {phase >= 2 ? 2 : 0, 3}});
        if (pick == 0) { queue.push_back(skill(SkillId::JoanaClawCrush)); pendingRecovery = 54; }
        else if (pick == 1) { queue.push_back(skill(SkillId::JoanaHookPull)); if (phase >= 2) queue.push_back({FIGHTER_JAB, hand, 0, 0}); }
        else if (pick == 2) queue.push_back({FIGHTER_JAB, hand, t(30), 8}); // jab de marujo: recolhe o gancho
        else { queue.push_back(skill(SkillId::JoanaAnchorSlam)); pendingRecovery = 80; }
        return;
    }
    case 3: { // O Cremoso: casquinha, calda; derrete e sobe da poça com uppercut (brecha ao se remontar); finge derreter
        const int pick = pickOf({{2, 0}, {2, 1}, {phase >= 2 ? 2 : 0, 2}, {phase == 3 ? 3 : 1, 3}, {2, 4}});
        if (pick == 0) queue.push_back(skill(SkillId::CremosoConePunch));
        else if (pick == 1) queue.push_back(skill(SkillId::CremosoSyrupBlast));
        else if (pick == 2) { queue.push_back(skill(SkillId::CremosoMeltMode)); queue.push_back({FIGHTER_UPPER, hand, t(40), 3}); pendingRecovery = 80; }
        else if (pick == 3) queue.push_back({FIGHTER_HOOK, hand, t(40), 9}); // finta: amolece como se fosse derreter e solta um gancho
        else queue.push_back({FIGHTER_CROSS, hand, t(34), 0});
        return;
    }
    case 4: { // Valéria: combos de frevo no ritmo (3 golpes), brecha no fim da dança; fora do tempo na fase 3
        const int pick = pickOf({{3, 0}, {2, 1}, {1, 2}});
        if (pick == 0) {
            const int beat = 14, late = (phase == 3 && random01() < 0.5f) ? 28 : beat;
            queue.push_back({FIGHTER_JAB, true, t(30), 10});
            queue.push_back({FIGHTER_JAB, false, late, 10});
            queue.push_back({FIGHTER_HOOK, true, beat, 10});
            pendingRecovery = 70;
        } else if (pick == 1) queue.push_back(skill(SkillId::ValeriaMagmaPunch));
        else queue.push_back(skill(SkillId::ValeriaLavaKickJab));
        return;
    }
    case 5: { // Vivaldi: a batuta conta os golpes (1 a 3 batidas = 1 a 3 socos); brecha depois do finale de 3
        const int pick = pickOf({{4, 0}, {phase >= 2 ? 2 : 0, 1}, {phase >= 2 ? 1 : 0, 2}});
        if (pick == 0) {
            const int n = 1 + std::rand() % (phase == 1 ? 2 : 3);
            tellBeats = n;
            for (int i = 0; i < n; ++i) queue.push_back({i == n - 1 && n == 3 ? FIGHTER_HOOK : FIGHTER_JAB, i % 2 == 0, i == 0 ? t(24 + 14 * n) : 0, 11});
            if (n == 3) pendingRecovery = 66;
        } else if (pick == 1) queue.push_back(skill(SkillId::VivaldiCadenceStaccato));
        else queue.push_back(skill(SkillId::VivaldiWinterAllegro));
        return;
    }
    case 6: { // Lady Espinha: espinhos (socar dói), voadora de lucha com brecha, garra do cacto
        const int pick = pickOf({{2, 0}, {2, 1}, {phase >= 2 ? 2 : 1, 2}, {phase >= 2 ? 1 : 0, 3}, {2, 4}});
        if (pick == 0) queue.push_back(skill(SkillId::EspinhaNopalSlap));
        else if (pick == 1) queue.push_back(skill(SkillId::EspinhaNeedleJab));
        else if (pick == 2) { queue.push_back(skill(SkillId::EspinhaLuchaFlyer)); pendingRecovery = 90; }
        else if (pick == 3) queue.push_back(skill(SkillId::EspinhaThornVeil));
        else queue.push_back({FIGHTER_HOOK, hand, t(36), 0});
        return;
    }
    case 7: { // Dr. Marcov: investida de foguete (chama cresce); superaquece depois da 3ª = brecha
        const int pick = pickOf({{4, 0}, {2, 1}, {phase >= 2 && !castPoison ? 1 : 0, 2}, {2, 3}});
        if (pick == 0) {
            queue.push_back({FIGHTER_CROSS, hand, t(phase == 3 ? 24 : 32), 12});
            if (++rockets >= 3) { rockets = 0; pendingRecovery = 72; }
        } else if (pick == 1) queue.push_back(skill(SkillId::MarcovKineticOverdrive));
        else if (pick == 2) { castPoison = true; queue.push_back(skill(SkillId::MarcovPlasmaBarrier)); }
        else queue.push_back({FIGHTER_JAB, hand, t(28), 0});
        return;
    }
    default: break;
    }
    // King Frog
    if (phase == 3 && !castPoison) {
        castPoison = true;
        Step s1{FIGHTER_SKILL, hand, 0, 0};
        s1.skill = SkillId::FrogPoisonSkin;
        queue.push_back(s1);
        return;
    }
    opts = {{2, 0}, {3, 1}, {phase == 3 ? 3 : 1, 2}};
    if (phase >= 2) opts.push_back({2, 3});
    int total = 0;
    for (auto& o : opts) total += o.weight;
    int roll = std::rand() % total, pick = 0;
    for (auto& o : opts) { if (roll < o.weight) { pick = o.id; break; } roll -= o.weight; }
    if (pick == 0) { Step s1{FIGHTER_SKILL, hand, 0, 0}; s1.skill = SkillId::FrogTongue; queue.push_back(s1); }
    else if (pick == 1) queue.push_back({FIGHTER_BODY_BLOW, hand, t(32), 5});  // Tapa de Lama: pega lama do chão
    else if (pick == 2) queue.push_back({FIGHTER_JAB, hand, t(38), 7});        // finta: infla e solta um jab
    else { Step s1{FIGHTER_SKILL, hand, 0, 0}; s1.skill = SkillId::FrogRoyalLeap; queue.push_back(s1); pendingRecovery = 90; }
}

AiAction EnemyAI::nextStep(Fighter& enemy) {
    const Step st = queue.front();
    queue.erase(queue.begin());
    if (st.move == FIGHTER_SKILL) {
        enemy.isLeftPunch = st.left;
        return beginSkill(enemy, st.skill);
    }
    enemy.queuedAttack = st.move;
    enemy.queuedHeavy = (st.move == FIGHTER_HOOK || st.move == FIGHTER_UPPER || st.move == FIGHTER_CROSS);
    enemy.isLeftPunch = st.left;
    enemy.tellKind = st.tellKind;
    enemy.queuedSkill = SkillId::None; enemy.queuedSignature = false;
    feintActive = st.feint;
    if (st.tell <= 0) enemy.startMove(st.move, (st.move == FIGHTER_JAB || st.move == FIGHTER_CROSS) ? 26 : 32);
    else enemy.startMove(FIGHTER_TELEGRAPH, st.tell);
    return {};
}

// Especial do oponente: curas, modos e escudos saem na hora; os outros pedem o combo de entrada antes (o combo é o aviso para o jogador).
AiAction EnemyAI::beginSkill(Fighter& enemy, SkillId id) {
    const SkillDef* d = SkillSystem::get(id);
    const int need = d ? SkillSystem::chainNeed(*d) : 0;
    if (need == 0) { AiAction a; a.cast = id; afterSkill = id != SkillId::None; return a; }
    const bool lead = enemy.isLeftPunch;
    combo.clear();
    // cada rival monta o combo do jeito dele (golpe, mão trocada ou não, quadros de aviso); o terceiro golpe só entra nos especiais fortes
    struct Beat { FighterActionState move; bool swap; int tell; };
    static const Beat kCombo[9][3] = {
        {{FIGHTER_JAB, false, 18}, {FIGHTER_JAB, true, 13}, {FIGHTER_HOOK, false, 18}},        // Jack: dois jabs rápidos e gancho
        {{FIGHTER_BODY_BLOW, false, 24}, {FIGHTER_HOOK, true, 18}, {FIGHTER_UPPER, false, 24}}, // King Frog: por baixo, de lado e por cima
        {{FIGHTER_CROSS, false, 24}, {FIGHTER_HOOK, true, 20}, {FIGHTER_HOOK, false, 20}},      // Joana: direto e duas garras
        {{FIGHTER_JAB, false, 24}, {FIGHTER_BODY_BLOW, true, 20}, {FIGHTER_CROSS, false, 22}},  // Cremoso: devagar, no corpo
        {{FIGHTER_JAB, false, 16}, {FIGHTER_CROSS, true, 13}, {FIGHTER_UPPER, false, 18}},      // Valéria: curta e agressiva
        {{FIGHTER_JAB, false, 18}, {FIGHTER_JAB, true, 18}, {FIGHTER_CROSS, false, 18}},        // Vivaldi: no compasso
        {{FIGHTER_HOOK, false, 22}, {FIGHTER_HOOK, true, 18}, {FIGHTER_BODY_BLOW, false, 20}},  // Lady Espinha: tapas de lado
        {{FIGHTER_CROSS, false, 26}, {FIGHTER_BODY_BLOW, true, 22}, {FIGHTER_UPPER, false, 26}},// Marcov: pesado e lento
        {{FIGHTER_JAB, false, 22}, {FIGHTER_CROSS, true, 16}, {FIGHTER_HOOK, false, 20}},       // chefe: o combo padrão
    };
    const Beat* beat = kCombo[enemy.charId < 9 ? enemy.charId : 8];
    for (int i = 0; i < std::min(need, 3); i++) combo.push_back({beat[i].move, beat[i].swap ? !lead : lead, beat[i].tell, 0});
    Step last; last.move = FIGHTER_SKILL; last.left = lead; last.skill = id;
    combo.push_back(last);
    enemy.chainNeed = need; enemy.chainCount = 0;
    return nextCombo(enemy);
}

AiAction EnemyAI::nextCombo(Fighter& enemy) {
    const Step st = combo.front();
    combo.erase(combo.begin());
    if (st.move == FIGHTER_SKILL) {   // combo completo: agora o especial (com o aviso da etapa 1, se ele pede)
        if (warnSkill(enemy, st.skill)) return {};
        AiAction a; a.cast = st.skill; afterSkill = true; return a;
    }
    enemy.queuedAttack = st.move;
    enemy.queuedHeavy = st.move != FIGHTER_JAB;
    enemy.isLeftPunch = st.left;
    enemy.tellKind = 0;
    enemy.queuedSkill = SkillId::None; enemy.queuedSignature = false;
    feintActive = false;
    enemy.chainCount++;
    enemy.startMove(FIGHTER_TELEGRAPH, st.tell);
    return {};
}

AiAction EnemyAI::update(Fighter& enemy, const Fighter& player, uint32_t frame, uint32_t& audioEvents) {
    if (enemy.staminaRegenDelay > 0) enemy.staminaRegenDelay--;
    else enemy.stamina = std::min(enemy.maxStamina, enemy.stamina + 0.4f * (enemy.loadout.has(SkillId::JoanaDeepSeaTide) && enemy.hp < 40.0f ? 1.5f : 1.0f));

    enemy.resetParts();
    enemy.telegraphFlash = false;
    if (enemy.state != FIGHTER_TELEGRAPH) { enemy.queuedSkill = SkillId::None; enemy.queuedSignature = false; }   // apanhou ou foi interrompido armando o especial: ele não sai
    if (enemy.vulnerable > 0) enemy.vulnerable--;
    // combo de entrada: apanhar (ou levar um aparo, que também desequilibra) quebra a sequência
    if (enemy.state == FIGHTER_HIT || enemy.state == FIGHTER_KNOCKDOWN) combo.clear();
    if (combo.empty() && enemy.queuedSkill == SkillId::None && enemy.state != FIGHTER_TELEGRAPH && !isPunchState(enemy.state)) enemy.chainCount = enemy.chainNeed = 0;

    const bool playerPunching = isPunchState(player.state);
    // cada soco do jogador é um sorteio novo, mesmo emendado no anterior (antes, martelar passava direto pela defesa)
    if (!playerPunching || !sawPunch || player.timer > lastPunchTimer) rolledDefenseForPunch = false;
    sawPunch = playerPunching; lastPunchTimer = player.timer;
    const bool playerCasting = player.state == FIGHTER_SKILL && needsWarning(player.activeSkill);
    if (!playerCasting) rolledSkillDodge = false;
    const bool reactive = defenseChance > 0.0f;
    // em quantos quadros o golpe do jogador chega (-1: nada vindo); especial não se segura na guarda
    int hitIn = -1; bool canBlock = true;
    if (playerPunching && !player.hasHit) hitIn = static_cast<int>(std::ceil(0.40f * player.maxTimer)) - (player.maxTimer - player.timer) + 1;
    else if (playerCasting && !player.skillEventDone) {
        const SkillDef* pd = SkillSystem::get(player.activeSkill);
        hitIn = static_cast<int>(std::ceil((pd ? pd->fx.at : 0.45f) * player.maxTimer)) - (player.maxTimer - player.timer) + 1;
        canBlock = false;
    }
    if (enemy.state != FIGHTER_HIT) inHit = false;
    if (sinceDodge < 100000) sinceDodge++;
    if (playerPunching || playerCasting) calm = 0; else if (++calm > 90) { hitsTaken = 0; blocksInRow = 0; }

    switch (enemy.state) {
    case FIGHTER_IDLE: {
        // Avança até o meio do ringue enquanto está parado.
        enemy.radius += (enemy.idleRadius() - enemy.radius) * 0.045f;
        if (std::fabs(enemy.radius - enemy.idleRadius()) < 0.5f) enemy.radius = enemy.idleRadius();
        enemy.applyIdleBob(frame);

        if (hasPattern(enemy)) {
            if (afterSkill) { // a habilidade da sequência acabou
                afterSkill = false;
                if (queue.empty() && pendingRecovery > 0) { enemy.vulnerable = pendingRecovery; pendingRecovery = 0; }
                enemy.actionTimer = queue.empty() ? 30 : 8;
            }
            if (enemy.vulnerable > 0) { // brecha: cansado, de guarda baixa, sem reagir
                enemy.gloveL.y -= 22.0f; enemy.gloveR.y -= 22.0f;
                enemy.gloveL.x -= 8.0f; enemy.gloveR.x += 8.0f;
                enemy.offsetY = -6.0f + std::sin(frame * 0.15f) * 3.0f;
                enemy.bodySkew = std::sin(frame * 0.07f) * 0.06f;
                return {};
            }
        }

        // montando o combo de um especial: emenda o próximo golpe, sem parar para defender
        if (!combo.empty()) {
            if (enemy.vulnerable > 0) return {};
            enemy.actionTimer = std::min(enemy.actionTimer, 6);
            if (--enemy.actionTimer <= 0) return nextCombo(enemy);
            return {};
        }

        // defesa marcada: espera o tempo de reação (ou a hora certa da esquiva) e sai
        if (defKind != 0) {
            if (hitIn < 0) defKind = 0; // o golpe não vem mais
            else { if (--defDelay <= 0 || hitIn <= 2) startDefense(enemy, audioEvents); return {}; }
        }
        // Reação ao golpe do jogador, sorteada uma vez por soco ou especial. Soco: defenseChance (quase certo se já apanhou duas vezes seguidas).
        // Especial: só perto do acerto, e só esquivando.
        if (reactive && hitIn > 0 && enemy.vulnerable <= 0) {
            bool& rolled = canBlock ? rolledDefenseForPunch : rolledSkillDodge;
            if (!rolled && (canBlock || hitIn <= 16)) {
                rolled = true;
                const float chance = canBlock ? (hitsTaken >= 2 ? std::max(defenseChance, 0.95f) : defenseChance) : skillDodgeChance;
                if (random01() < chance) {
                    planDefense(enemy, player, hitIn, canBlock);
                    if (defKind != 0) { if (defDelay <= 0) startDefense(enemy, audioEvents); return {}; }
                }
            }
        }

        // Habilidades da build sorteada: uma a cada 5–8 s (60% especial 1, 40% especial 2).
        if (useSkills && !hasPattern(enemy) && (enemy.loadout.spec1 != SkillId::None || enemy.loadout.spec2 != SkillId::None) && --skillCooldown <= 0) {
            skillCooldown = 320 + std::rand() % 160;
            const float firstChance = enemy.bossStage > 0 ? 0.5f : 0.6f; // o chefe sorteia meio a meio
            SkillId pick = (random01() < firstChance && enemy.loadout.spec1 != SkillId::None) ? enemy.loadout.spec1 : enemy.loadout.spec2;
            if (pick == SkillId::None) pick = enemy.loadout.spec1;
            return beginSkill(enemy, pick);
        }

        if (hasPattern(enemy) && --enemy.actionTimer <= 0) {
            if (queue.empty()) choosePattern(enemy);
            return nextStep(enemy);
        }
        if (!hasPattern(enemy) && --enemy.actionTimer <= 0) {
            // Golpe-assinatura (32%): animação própria do lutador, mas dano de golpe comum. Arma antes, como um soco pesado comum.
            if (useSkills && random01() < signatureChance) {
                AiAction action;
                action.signature = true;
                if (enemy.charId == 0) action.cast = SkillId::JackStingJab;
                else if (enemy.charId >= 3 && enemy.charId <= 7) {
                    // Golpes de ataque da ficha do lutador (→→ + X e ↑↑ + X).
                    static const SkillId kSig[8][2] = {{}, {}, {},
                        {SkillId::CremosoConePunch, SkillId::CremosoSyrupBlast},
                        {SkillId::ValeriaMagmaPunch, SkillId::ValeriaLavaKickJab},
                        {SkillId::VivaldiCrescendoJab, SkillId::VivaldiWinterAllegro},
                        {SkillId::EspinhaNopalSlap, SkillId::EspinhaNeedleJab},
                        {SkillId::MarcovRocketThrust, SkillId::MarcovKineticOverdrive}};
                    action.cast = kSig[enemy.charId][std::rand() % 2];
                }
                else if (enemy.charId == 2) {
                    static const SkillId kJoana[] = {SkillId::JoanaClawCrush, SkillId::JoanaHookPull, SkillId::JoanaAnchorSlam};
                    action.cast = kJoana[std::rand() % 3];
                }
                else if (enemy.charId == 1) {
                    static const SkillId kFrog[] = {SkillId::FrogTongue, SkillId::FrogMudSlime, SkillId::FrogRoyalLeap};
                    action.cast = kFrog[std::rand() % 3];
                }
                if (action.cast != SkillId::None) {
                    enemy.isLeftPunch = random01() > 0.5f;
                    enemy.queuedSkill = action.cast; enemy.queuedSignature = true; enemy.queuedHeavy = true; enemy.tellKind = 0;
                    enemy.startMove(FIGHTER_TELEGRAPH, 36);
                    return {};
                }
            }
            static const FighterActionState kMoves[] = {FIGHTER_JAB, FIGHTER_CROSS, FIGHTER_HOOK, FIGHTER_UPPER, FIGHTER_BODY_BLOW};
            FighterActionState move = kMoves[std::rand() % 5];
            enemy.queuedAttack = move;
            enemy.queuedHeavy = (move == FIGHTER_HOOK || move == FIGHTER_UPPER || move == FIGHTER_CROSS);
            enemy.isLeftPunch = enemy.bossStage == 2 ? enemy.queuedHeavy : random01() > 0.5f;
            enemy.startMove(FIGHTER_TELEGRAPH, enemy.queuedHeavy ? 48 : 36);
        }
        return {};
    }

    case FIGHTER_BLOCK:
        enemy.timer--;
        enemy.gloveL.x = -26.0f; enemy.gloveL.y = 85.0f; enemy.gloveL.z = 12.0f;
        enemy.gloveR.x = 26.0f;  enemy.gloveR.y = 85.0f; enemy.gloveR.z = 12.0f;
        if (playerPunching && player.hasHit && !guardSaw) { guardSaw = true; enemy.timer = std::min(enemy.timer, 3); } // a guarda segurou um soco: abre logo, para dar tempo de responder
        // o jogador emendou outro soco: quase sempre mantém a guarda fechada em vez de abrir
        if (reactive && playerPunching && !rolledDefenseForPunch && hitIn > 0) {
            rolledDefenseForPunch = true;
            if (random01() < 0.85f) enemy.timer = std::max(enemy.timer, hitIn + 9);
        }
        if (enemy.timer <= 0) {
            enemy.state = FIGHTER_IDLE; enemy.actionTimer = 32;
            if (guardSaw) { // segurou: às vezes responde na hora (mais ainda se o jogador insiste em bater na guarda)
                guardSaw = false; hitsTaken = 0; blocksInRow++;
                if (reactive && enemy.vulnerable <= 0 && combo.empty() && random01() < counterChance + (blocksInRow >= 2 ? 0.25f : 0.0f)) startCounter(enemy, false);
            }
        }
        return {};

    case FIGHTER_DODGE_L:
    case FIGHTER_DODGE_R: {
        enemy.timer--;
        const float p = 1.0f - static_cast<float>(enemy.timer) / std::max(1, enemy.maxTimer);
        const float swing = std::sin(p * kPi);
        const float dir = (enemy.state == FIGHTER_DODGE_L) ? -1.0f : 1.0f;
        const float turnDir = (enemy.state == FIGHTER_DODGE_L) ? 1.0f : -1.0f;
        enemy.offsetX = dir * 54.0f * swing;
        enemy.offsetY = 18.0f * swing;
        enemy.bodySkew = -dir * 0.28f * swing;

        const float prevP = 1.0f - static_cast<float>(enemy.timer + 1) / std::max(1, enemy.maxTimer);
        const float stepFrac = (p * p * (3.0f - 2.0f * p)) - (prevP * prevP * (3.0f - 2.0f * prevP));

        AiAction act;
        act.ringTurnDelta = turnDir * (kPi * 0.5f) * stepFrac;
        if (p < 0.3f && !enemy.spawnedDodgeSmoke) {
            enemy.spawnedDodgeSmoke = true;
            act.dodgeSmoke = true;
            act.smokeX = enemy.offsetX;
        }

        if ((playerPunching && player.hasHit) || (playerCasting && player.skillEventDone)) dodgeSaw = true; // a esquiva evitou um golpe de verdade
        if (enemy.timer <= 0) {
            enemy.state = FIGHTER_IDLE;
            enemy.resetPose();
            enemy.actionTimer = 30;
            if (dodgeSaw) { // o jogador bateu no vazio: às vezes volta com um gancho
                dodgeSaw = false; hitsTaken = 0;
                if (reactive && enemy.vulnerable <= 0 && combo.empty() && random01() < counterChance + 0.1f) startCounter(enemy, true);
            }
        }
        return act;
    }

    case FIGHTER_TELEGRAPH: {
        // Lê o golpe do jogador ainda no começo do aviso: larga o próprio golpe e defende (só golpe comum; especial armado e combo de
        // entrada vão até o fim). Na metade final do aviso já está comprometido, e aí o soco do jogador entra como contra-ataque.
        if (reactive && hitIn > 0 && combo.empty() && enemy.queuedSkill == SkillId::None && !feintActive) {
            bool& rolled = canBlock ? rolledDefenseForPunch : rolledSkillDodge;
            if (!rolled && (canBlock || hitIn <= 16)) {
                rolled = true;
                const float prog0 = 1.0f - static_cast<float>(enemy.timer) / std::max(1, enemy.maxTimer);
                const int myHit = enemy.timer + 12; // quando o meu golpe chegaria
                // o contra-ataque só continua se chegar antes do soco do jogador; senão volta a defender
                if (myHit > hitIn && (countering ? random01() < 0.85f : (prog0 < 0.55f && random01() < tellCancelChance))) {
                    countering = false;
                    queue.clear(); pendingRecovery = 0; enemy.tellKind = 0;
                    enemy.state = FIGHTER_IDLE; enemy.resetPose(); enemy.actionTimer = 30;
                    planDefense(enemy, player, hitIn, canBlock);
                    if (defKind != 0 && defDelay <= 0) startDefense(enemy, audioEvents);
                    return {};
                }
            }
        }
        enemy.timer--;
        enemy.telegraphFlash = enemy.timer % 6 < 3;
        float prog = 1.0f - static_cast<float>(enemy.timer) / enemy.maxTimer;
        enemy.resetPose();
        if (enemy.bossStage == 1 && enemy.queuedHeavy) {
            // Batalhator: arma as duas serras juntas.
            enemy.gloveL.y += 20.0f * prog; enemy.gloveR.y += 20.0f * prog;
            enemy.gloveL.z += 18.0f * prog; enemy.gloveR.z += 18.0f * prog;
            enemy.offsetY = -6.0f * prog;
        } else if (enemy.bossStage == 2) {
            // Danificado: pesado com a cabeça-punho (esquerda), leve com a serra (direita).
            if (enemy.queuedHeavy) {
                enemy.gloveL.z += 24.0f * prog;
                enemy.gloveL.x -= 14.0f * prog;
                enemy.bodySkew = 0.12f * prog;
            } else {
                enemy.gloveR.z += 18.0f * prog;
                enemy.offsetY = 4.0f * prog;
                enemy.bodySkew = -0.05f * prog;
            }
        } else {
            PartRenderData& gl = enemy.isLeftPunch ? enemy.gloveL : enemy.gloveR;
            const PartRenderData& base = enemy.isLeftPunch ? enemy.baseGloveL : enemy.baseGloveR;
            const float side = enemy.isLeftPunch ? -1.0f : 1.0f;
            switch (enemy.tellKind) {
            case 1: // Ferroada: puxa a luva para trás e para baixo, rápido
                gl.z = base.z - 16.0f * prog; gl.y = base.y - 10.0f * prog;
                enemy.bodySkew = side * 0.08f * prog;
                break;
            case 2: { // Colmeia Dupla: bate as colmeias uma na outra
                const float bang = std::fabs(std::sin(prog * kPi * 2.0f));
                enemy.gloveL.x = enemy.baseGloveL.x * (1.0f - 0.85f * bang);
                enemy.gloveR.x = enemy.baseGloveR.x * (1.0f - 0.85f * bang);
                enemy.gloveL.y += 12.0f * bang; enemy.gloveR.y += 12.0f * bang;
                enemy.offsetY = 3.0f * bang;
                break;
            }
            case 3: // uppercut: agacha e leva a luva lá embaixo
                enemy.offsetY = -10.0f * prog;
                gl.y = base.y - 30.0f * prog; gl.z = base.z + 6.0f * prog;
                break;
            case 5: // Tapa de Lama: desce a mão até o chão
                gl.y = base.y - 60.0f * prog; gl.z = base.z + 22.0f * prog;
                gl.x = base.x + side * 8.0f * prog;
                enemy.bodySkew = side * 0.16f * prog;
                enemy.offsetY = -6.0f * prog;
                break;
            case 7: // finta: infla (abre os braços e sobe)
                enemy.gloveL.x = enemy.baseGloveL.x * (1.0f + 0.6f * prog);
                enemy.gloveR.x = enemy.baseGloveR.x * (1.0f + 0.6f * prog);
                enemy.offsetY = 8.0f * prog;
                break;
            case 8: // Joana: recolhe o braço do gancho bem para fora e para trás
                gl.x = base.x + side * 46.0f * prog; gl.z = base.z - 26.0f * prog; gl.y = base.y + 24.0f * prog;
                enemy.bodySkew = -side * 0.18f * prog;
                break;
            case 9: // Cremoso: amolece (afunda e as mãos caem), como se fosse derreter
                enemy.offsetY = -16.0f * prog;
                enemy.gloveL.y -= 24.0f * prog; enemy.gloveR.y -= 24.0f * prog;
                break;
            case 10: // Valéria: pulinho do frevo
                enemy.offsetY = 10.0f * std::fabs(std::sin(prog * kPi * 2.0f));
                gl.y = base.y + 10.0f * prog;
                enemy.bodySkew = side * 0.1f * std::sin(prog * kPi * 2.0f);
                break;
            case 11: { // Vivaldi: a batuta bate uma vez por golpe que vem
                const float tap = std::fabs(std::sin(prog * kPi * static_cast<float>(tellBeats)));
                enemy.gloveR.y = enemy.baseGloveR.y + 30.0f * tap;
                enemy.gloveR.x = enemy.baseGloveR.x + 6.0f;
                break;
            }
            case 13: { // especial vindo: as duas luvas recuam e sobem, agacha e treme cada vez mais
                const float shake = std::sin(prog * 70.0f) * 4.0f * prog;
                enemy.gloveL.z = enemy.baseGloveL.z - 22.0f * prog; enemy.gloveR.z = enemy.baseGloveR.z - 22.0f * prog;
                enemy.gloveL.y = enemy.baseGloveL.y + 26.0f * prog; enemy.gloveR.y = enemy.baseGloveR.y + 26.0f * prog;
                enemy.gloveL.x = enemy.baseGloveL.x * (1.0f + 0.35f * prog) + shake; enemy.gloveR.x = enemy.baseGloveR.x * (1.0f + 0.35f * prog) - shake;
                enemy.offsetY = -12.0f * prog; enemy.offsetX = shake;
                break;
            }
            case 12: // Marcov: foguetes acendem, sobe e treme cada vez mais
                enemy.offsetY = 18.0f * prog;
                enemy.offsetX = std::sin(prog * 90.0f) * 5.0f * prog;
                enemy.gloveL.z -= 16.0f * prog; enemy.gloveR.z -= 16.0f * prog;
                enemy.gloveL.y -= 10.0f * prog; enemy.gloveR.y -= 10.0f * prog;
                break;
            default: // aviso comum: arma a luva do golpe, sobe, avança e abre, com o tronco inclinado
                gl.y = base.y + 15.0f * prog;
                gl.z = base.z + 18.0f * prog;
                gl.x = base.x + side * 10.0f * prog;
                enemy.bodySkew = -side * 0.12f * prog;
                break;
            }
        }
        if (enemy.timer <= 0 && enemy.queuedSkill != SkillId::None) {   // especial armado: agora sai
            AiAction action; action.cast = enemy.queuedSkill; action.signature = enemy.queuedSignature; if (!action.signature) afterSkill = true; enemy.queuedSkill = SkillId::None; enemy.queuedSignature = false; enemy.tellKind = 0;
            enemy.state = FIGHTER_IDLE; enemy.resetPose(); return action;
        }
        if (enemy.timer <= 0) {
            if (feintActive) { // aviso falso: arma de novo antes de soltar
                feintActive = false;
                enemy.startMove(FIGHTER_TELEGRAPH, std::max(20, enemy.maxTimer * 2 / 3));
                return {};
            }
            FighterActionState move = enemy.queuedAttack;
            enemy.startMove(move, (move == FIGHTER_JAB || move == FIGHTER_CROSS) ? 28 : 36);
        }
        return {};
    }

    case FIGHTER_JAB:
    case FIGHTER_CROSS:
    case FIGHTER_HOOK:
    case FIGHTER_BODY_BLOW:
    case FIGHTER_UPPER: {
        enemy.timer--;
        float p = 1.0f - static_cast<float>(enemy.timer) / enemy.maxTimer;
        enemy.resetPose();
        const float swing = std::sin(p * kPi);
        if (enemy.bossStage == 1 && enemy.queuedHeavy) {
            enemy.gloveL.z += 70.0f * swing; enemy.gloveR.z += 70.0f * swing;
            enemy.gloveL.y += 12.0f * swing; enemy.gloveR.y += 12.0f * swing;
            enemy.offsetY = 6.0f * swing;
        } else if (enemy.bossStage == 2 && enemy.queuedHeavy) {
            enemy.gloveL.z += 80.0f * swing; enemy.gloveL.y += 10.0f * swing; enemy.gloveL.x += 12.0f * swing;
            enemy.bodySkew = -0.10f * swing;
        } else if (enemy.bossStage == 2) {
            enemy.gloveR.z += 60.0f * swing; enemy.gloveR.y += 8.0f * swing;
            enemy.offsetY = 4.0f * swing;
            enemy.bodySkew = 0.06f * swing;
        } else {
            enemy.animateCommonPunch(enemy.state, p, enemy.isLeftPunch);
        }
        AiAction action;
        if (p > 0.40f && p < 0.68f && !enemy.hasHit) {
            enemy.hasHit = true;
            action.checkHit = true;
        }
        if (enemy.hasHit) hitsTaken = 0; // voltou a atacar: a sequência do jogador acabou
        if (enemy.timer <= 0) {
            countering = false;
            enemy.state = FIGHTER_IDLE;
            enemy.resetPose();
            enemy.actionTimer = 35 + static_cast<int>(random01() * 45.0f);
            if (hasPattern(enemy)) {
                if (!queue.empty()) enemy.actionTimer = 4; // emenda o próximo golpe da sequência
                else {
                    if (pendingRecovery > 0) { enemy.vulnerable = pendingRecovery; pendingRecovery = 0; }
                    const int phase = phaseOf(enemy);
                    enemy.actionTimer = (phase == 3 ? 22 : 34) + static_cast<int>(random01() * (phase == 3 ? 24.0f : 40.0f));
                }
            }
        }
        return action;
    }

    case FIGHTER_HIT: {
        if (!inHit) { inHit = true; hitsTaken++; blocksInRow = 0; countering = false; defKind = 0; } // mais um golpe limpo na sequência
        enemy.timer--;
        float p = 1.0f - static_cast<float>(enemy.timer) / enemy.maxTimer;
        enemy.bodySkew = std::sin(p * kPi) * 0.3f;
        enemy.offsetX = (random01() - 0.5f) * 18.0f;
        if (enemy.timer <= 0) {
            enemy.state = FIGHTER_IDLE;
            enemy.resetPose();
            enemy.actionTimer = 35;
        }
        // Apanhou: a sequência e a brecha pendente se perdem.
        queue.clear(); pendingRecovery = 0; feintActive = false; enemy.tellKind = 0;
        return {};
    }

    default:
        return {};
    }
}

void EnemyAI::finishAction(Fighter& enemy) {
    if (enemy.state == FIGHTER_IDLE || enemy.state == FIGHTER_KNOCKDOWN) {
        enemy.resetPose();
        return;
    }
    enemy.timer--;
    float p = 1.0f - static_cast<float>(enemy.timer) / std::max(1, enemy.maxTimer);
    enemy.resetPose();
    if (isPunchState(enemy.state)) enemy.animateCommonPunch(enemy.state, p, enemy.isLeftPunch);
    if (enemy.timer <= 0) {
        enemy.state = FIGHTER_IDLE;
        enemy.resetPose();
        enemy.actionTimer = 55;
    }
}

} // namespace KoRama
