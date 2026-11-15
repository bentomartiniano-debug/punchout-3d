// Testes do motor de luta, sem janela. Rodar: ctest --preset <debug|release> ou build/<preset>/korama_tests.exe
#include "engine.hpp"
#include "tournament.hpp"
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <string>
#include <vector>

using namespace KoRama;

namespace {

int g_failures = 0;

#define CHECK(cond)                                                                 \
    do {                                                                            \
        if (!(cond)) {                                                              \
            std::printf("    FALHOU (linha %d): %s\n", __LINE__, #cond);           \
            ++g_failures;                                                           \
        }                                                                           \
    } while (0)

constexpr float kDt = 1.0f / 60.0f;

void tick(GameEngine& e, uint32_t held = 0, int frames = 1) {
    for (int i = 0; i < frames; ++i) e.update(held, kDt);
}

// Partida Jack x King Frog com o oponente parado colado no jogador e sem atacar.
GameEngine freshMatch() {
    std::srand(42);
    GameEngine e;
    e.startMatch(0, 1, false, 1);
    e.enemy.radius = 50.0f;
    e.enemy.actionTimer = 1000000;
    e.styleTraits = false;     // regras básicas, sem o estilo de cada lutador (testado à parte)
    e.ai.defenseChance = 0.0f; // sem defesa reativa nos testes determinísticos
    e.ai.useSkills = false;    // e sem habilidades da IA
    e.slipperyChance = 0.0f;   // e sem a pele mucosa aleatória
    return e;
}

float tapPunch(GameEngine& e, uint32_t hand, uint32_t direction = 0);

// Partida com a build do jogador escolhida.
GameEngine skillMatch(uint32_t playerChar, SkillLoadout loadout) {
    std::srand(42);
    GameEngine e;
    e.startMatch(playerChar, playerChar == 0 ? 1 : 0, false, 1, &loadout);
    e.enemy.radius = 50.0f;
    e.enemy.actionTimer = 1000000;
    e.styleTraits = false;
    e.ai.defenseChance = 0.0f;
    e.ai.useSkills = false;
    e.slipperyChance = 0.0f;
    tick(e, 0, 2);
    return e;
}

// Combo de entrada dos especiais (PLANO-COMBOS-E-ESQUIVA.md, etapa 3): jab, direto e, com três, gancho. Depois vem o finalizador (seta + soco).
void entryCombo(GameEngine& e, int punches) {
    tapPunch(e, INPUT_PUNCH_L);                                 // jab
    tapPunch(e, INPUT_PUNCH_R);                                 // direto
    if (punches >= 3) tapPunch(e, INPUT_PUNCH_L, INPUT_LEFT);   // gancho
}

// Aperta uma sequência de teclas, um aperto a cada 4 quadros.
void press(GameEngine& e, std::initializer_list<uint32_t> keys) {
    for (uint32_t k : keys) {
        tick(e, k);
        tick(e, 0, 3);
    }
}

void testStingJabCommand() {
    GameEngine e = skillMatch(0, {SkillId::JackStingJab, SkillId::JackRoyalJelly, SkillId::JackBeekeeper});
    e.enemy.startMove(FIGHTER_BLOCK, 1000); // ignora a guarda
    press(e, {INPUT_UP, INPUT_UP, INPUT_PUNCH_R});
    CHECK(e.player.state == FIGHTER_SKILL);
    CHECK(e.player.activeSkill == SkillId::JackStingJab);
    tick(e, 0, 30);
    CHECK(e.enemy.hp == 84.0f);
}

void testRoyalJellyHealsWithCooldown() {
    GameEngine e = skillMatch(0, {SkillId::JackStingJab, SkillId::JackRoyalJelly, SkillId::JackBeekeeper});
    e.player.hp = 50.0f;
    press(e, {INPUT_LEFT, INPUT_LEFT, INPUT_PUNCH_L});
    CHECK(e.player.hp == 70.0f);
    tick(e, 0, 40);
    press(e, {INPUT_LEFT, INPUT_LEFT, INPUT_PUNCH_L}); // ainda em recarga: vira soco, sem cura
    CHECK(e.player.hp == 70.0f);
}

void testInfestationDamageOverTime() {
    GameEngine e = skillMatch(0, {SkillId::JackInfestation, SkillId::JackMolasses, SkillId::JackBeekeeper});
    press(e, {INPUT_LEFT, INPUT_RIGHT, INPUT_PUNCH_R});
    CHECK(e.enemy.status.infested > 0);
    tick(e, 0, 6 * 60);
    CHECK(e.enemy.hp < 82.0f && e.enemy.hp > 79.0f); // ~19 HP em 6 s
}

void testMolassesHalvesDamageAndSlows() {
    GameEngine e = skillMatch(0, {SkillId::JackInfestation, SkillId::JackMolasses, SkillId::JackBeekeeper});
    tick(e, INPUT_PUNCH_L);
    tick(e, INPUT_PUNCH_L | INPUT_PUNCH_R);
    CHECK(e.player.status.molasses > 0);
    tick(e, 0, 40);
    float dmg = tapPunch(e, INPUT_PUNCH_L);
    CHECK(dmg == 3.0f); // jab 6 pela metade
    CHECK(e.enemy.status.slowed > 0);
}

void testHoneyWallAbsorbsThreeHits() {
    GameEngine e = skillMatch(0, {SkillId::JackHoneyWall, SkillId::JackStingJab, SkillId::JackBeekeeper});
    tick(e, 0, 30);
    press(e, {INPUT_DOWN, INPUT_DOWN, INPUT_PUNCH_L});
    CHECK(e.player.status.honeyShield == 3);
    tick(e, 0, 40);
    e.enemy.queuedHeavy = true;
    e.enemy.startMove(FIGHTER_HOOK, 36);
    tick(e, 0, 40);
    CHECK(e.player.hp == 90.0f); // 24 × 0,4 ≈ 10
    CHECK(e.player.status.honeyShield == 2);
}

void testFrogTonguePushesAndSlows() {
    GameEngine e = skillMatch(1, {SkillId::FrogTongue, SkillId::FrogPoisonSkin, SkillId::FrogJumper});
    float radiusBefore = e.enemy.radius;
    press(e, {INPUT_LEFT, INPUT_RIGHT, INPUT_PUNCH_R});
    tick(e, 0, 12);
    CHECK(e.enemy.hp == 82.0f);
    CHECK(e.enemy.radius > radiusBefore + 40.0f);
    CHECK(e.enemy.status.slowed > 0);
}

void testFrogPoisonSkin() {
    GameEngine e = skillMatch(1, {SkillId::FrogTongue, SkillId::FrogPoisonSkin, SkillId::FrogJumper});
    tick(e, INPUT_PUNCH_R);
    tick(e, INPUT_PUNCH_R | INPUT_PUNCH_L);
    CHECK(e.enemy.status.poisoned == 0);   // especial de fluxo: sem o combo (ou uma esquiva) o Z + X não lança
    tick(e, 0, 80);
    entryCombo(e, 2);
    e.enemy.hp = 100.0f;                   // o teste mede só o veneno, não os socos do combo
    tick(e, INPUT_PUNCH_R);
    tick(e, INPUT_PUNCH_R | INPUT_PUNCH_L);
    CHECK(e.enemy.status.poisoned > 0);
    tick(e, 0, 7 * 60);   // até o veneno acabar (os quadros congelados do golpe não contam para ele)
    CHECK(e.enemy.hp < 80.0f && e.enemy.hp > 78.0f); // 3,5 × 6 = 21
}

void testRoyalLeapAirborneAndImpact() {
    GameEngine e = skillMatch(1, {SkillId::FrogRoyalLeap, SkillId::FrogMudSlime, SkillId::FrogSlipperySkin});
    press(e, {INPUT_DOWN, INPUT_UP, INPUT_PUNCH_R});
    CHECK(e.player.activeSkill == SkillId::FrogRoyalLeap);
    tick(e, 0, 8);
    CHECK(e.player.offsetY > 60.0f); // no ar
    // Golpe do oponente passa por baixo enquanto ele está no ar.
    e.enemy.queuedHeavy = false;
    e.enemy.startMove(FIGHTER_JAB, 28);
    tick(e, 0, 14);
    CHECK(e.player.hp == 100.0f);
    tick(e, 0, 20);
    CHECK(e.enemy.hp == 78.0f);
}

void testMudBlocksDodge() {
    GameEngine e = skillMatch(0, {SkillId::JackStingJab, SkillId::JackRoyalJelly, SkillId::JackBeekeeper});
    e.player.status.mud = 300;
    tick(e, INPUT_DOWN | INPUT_LEFT, 2);
    CHECK(e.player.state == FIGHTER_BLOCK);
}

void testBlindMakesAttacksMiss() {
    int misses = 0;
    for (int i = 0; i < 40; ++i) {
        GameEngine e = freshMatch();
        e.player.status.blind = 600;
        std::srand(1000 + i);
        if (tapPunch(e, INPUT_PUNCH_L) == 0.0f) ++misses;
    }
    CHECK(misses >= 8 && misses <= 26); // ~40% de 40
}

void testQueenBlessingHealsOnKnockdown() {
    GameEngine e = skillMatch(0, {SkillId::JackStingJab, SkillId::JackRoyalJelly, SkillId::JackQueenBlessing});
    e.player.hp = 40.0f;
    e.enemy.hp = 3.0f;
    tapPunch(e, INPUT_PUNCH_L);
    CHECK(e.enemyKnockdowns == 1);
    CHECK(e.player.hp >= 65.0f);
}

SkillLoadout joanaBuild(SkillId a, SkillId b, SkillId passive = SkillId::JoanaCalcifiedArmor) {
    return {a, b, passive};
}

void testJoanaClawCrushBreaksGuard() {
    GameEngine e = skillMatch(2, joanaBuild(SkillId::JoanaClawCrush, SkillId::JoanaHookPull));
    e.enemy.startMove(FIGHTER_BLOCK, 1000);
    press(e, {INPUT_RIGHT, INPUT_RIGHT, INPUT_PUNCH_R});
    CHECK(e.player.activeSkill == SkillId::JoanaClawCrush);
    tick(e, 0, 12);
    CHECK(e.enemy.hp == 75.0f);
    CHECK(e.enemy.actionTimer == 90 || e.enemy.state == FIGHTER_IDLE);
}

void testJoanaHookPullDragsIn() {
    GameEngine e = skillMatch(2, joanaBuild(SkillId::JoanaClawCrush, SkillId::JoanaHookPull));
    press(e, {INPUT_LEFT, INPUT_RIGHT, INPUT_PUNCH_L});
    CHECK(e.player.activeSkill != SkillId::JoanaHookPull);   // golpe forte: o comando antigo sozinho não lança mais
    tick(e, 0, 80);
    entryCombo(e, 3);
    e.enemy.radius = 138.0f;
    tick(e, INPUT_RIGHT);
    tick(e, INPUT_RIGHT | INPUT_PUNCH_L);                     // finalizador: → + Z
    CHECK(e.player.activeSkill == SkillId::JoanaHookPull);
    tick(e, 0, 12);
    CHECK(e.enemy.radius < 100.0f);
}

void testJoanaShellReducesAndReflects() {
    GameEngine e = skillMatch(2, joanaBuild(SkillId::JoanaCrustaceanShell, SkillId::JoanaCoralSpout));
    tick(e, INPUT_PUNCH_L);
    tick(e, INPUT_PUNCH_L | INPUT_PUNCH_R);
    CHECK(e.player.status.shell > 0);
    tick(e, 0, 35);
    float enemyBefore = e.enemy.hp;
    e.enemy.queuedHeavy = true;
    e.enemy.queuedAttack = FIGHTER_HOOK;
    e.enemy.startMove(FIGHTER_HOOK, 36);
    tick(e, 0, 40);
    // 24 × 1,25 (Joana IA não, aqui o atacante é o Jack) → 24 × 0,8 (carapaça) = 19; 35% volta = 7; 30% passa = 6.
    CHECK(e.player.hp == 94.0f);
    CHECK(enemyBefore - e.enemy.hp == 7.0f);
}

void testJoanaCoralBleeds() {
    GameEngine e = skillMatch(2, joanaBuild(SkillId::JoanaCrustaceanShell, SkillId::JoanaCoralSpout));
    press(e, {INPUT_DOWN, INPUT_UP, INPUT_PUNCH_R});
    tick(e, 0, 20);
    CHECK(e.enemy.status.bleed > 0);
    CHECK(e.enemy.hp < 82.5f && e.enemy.hp > 80.0f); // 18 + um pouco de sangramento
}

void testJoanaAnchorSlows() {
    GameEngine e = skillMatch(2, joanaBuild(SkillId::JoanaAnchorSlam, SkillId::JoanaOceanBounty));
    tick(e, 0, 30);
    entryCombo(e, 3);
    tick(e, INPUT_DOWN);
    tick(e, INPUT_DOWN | INPUT_PUNCH_L);                      // finalizador: ↓ + Z
    CHECK(e.player.activeSkill == SkillId::JoanaAnchorSlam);
    tick(e, 0, 45);                                           // os especiais duram kSkillStretch vezes a ficha
    CHECK(e.enemy.status.slowed > 0);
}

void testJoanaMonstrousAndReach() {
    GameEngine e = skillMatch(2, joanaBuild(SkillId::JoanaClawCrush, SkillId::JoanaHookPull));
    // Gancho de marujo: jab alcança 215 × 1,4 = 301 (o Jack parado a 138 já está a 263).
    e.enemy.radius = 138.0f;
    tick(e, INPUT_PUNCH_L);
    tick(e);
    e.enemy.radius = 138.0f;
    tick(e, 0, 14);
    CHECK(e.enemy.hp < 100.0f);
    // Forma monstruosa com a vida em 40 ou menos.
    e.player.hp = 40.0f;
    tick(e);
    CHECK(e.player.monstrous);
    CHECK(e.player.isBigChar());
}

// O Jack (oponente padrão nestes testes) acerta um golpe comum no jogador.
void enemyHook(GameEngine& e) {
    e.enemy.queuedHeavy = true;
    e.enemy.queuedAttack = FIGHTER_HOOK;
    e.enemy.startMove(FIGHTER_HOOK, 36);
    tick(e, 0, 40);
}

void testCremosoSyrupAndMelt() {
    GameEngine e = skillMatch(3, {SkillId::CremosoSyrupBlast, SkillId::CremosoMeltMode, SkillId::CremosoCreamCushion});
    press(e, {INPUT_RIGHT, INPUT_RIGHT, INPUT_PUNCH_R});
    tick(e, 0, 20);
    CHECK(e.enemy.hp <= 80.0f && e.enemy.hp > 78.0f);
    CHECK(e.enemy.status.burn > 0);
    tick(e, 0, 20);
    tick(e, INPUT_PUNCH_L);
    tick(e, INPUT_PUNCH_L | INPUT_PUNCH_R);
    CHECK(e.player.status.melt > 0);
    tick(e, 0, 45);
    e.player.hp = 80.0f;
    enemyHook(e); // 24 → derretido 12, e 20% disso (2) volta como cura
    CHECK(e.player.hp == 70.0f);
}

void testCremosoCushionAndRevenge() {
    GameEngine e = skillMatch(3, {SkillId::CremosoSyrupBlast, SkillId::CremosoMeltMode, SkillId::CremosoCreamCushion});
    e.enemy.queuedHeavy = false;
    e.enemy.queuedAttack = FIGHTER_JAB;
    e.enemy.startMove(FIGHTER_JAB, 28);
    tick(e, 0, 30);
    CHECK(e.player.hp == 89.0f); // 14 × 0,75 = 10,5 → 11
    tick(e, 0, 30);
    e.enemy.startMove(FIGHTER_JAB, 28);
    tick(e, INPUT_DOWN | INPUT_LEFT, 30); // esquiva perfeita: +3 HP e próximo soco +15%
    CHECK(e.player.hp == 92.0f);
    CHECK(e.player.revengeBonus > 1.0f);
}

void testValeriaMagmaAndColdMagma() {
    GameEngine e = skillMatch(4, {SkillId::ValeriaMagmaPunch, SkillId::ValeriaHeatwaveGuard, SkillId::ValeriaEmberDance});
    press(e, {INPUT_RIGHT, INPUT_RIGHT, INPUT_PUNCH_R});
    tick(e, 0, 20);
    CHECK(e.enemy.hp <= 78.0f && e.enemy.hp > 76.0f);
    CHECK(e.enemy.status.burnDps == 4.0f);
    // Dança das Brasas: esquiva grátis.
    tick(e, 0, 30);
    float st = e.player.stamina;
    tick(e, INPUT_DOWN | INPUT_RIGHT, 2);
    CHECK(e.player.state == FIGHTER_DODGE_R);
    CHECK(e.player.stamina >= st);
}

void testValeriaHeatwaveBurnsAttacker() {
    GameEngine e = skillMatch(4, {SkillId::ValeriaMagmaPunch, SkillId::ValeriaHeatwaveGuard, SkillId::ValeriaEmberDance});
    tick(e, INPUT_PUNCH_L);
    tick(e, INPUT_PUNCH_L | INPUT_PUNCH_R);
    CHECK(e.player.status.heatwave > 0);
    tick(e, 0, 35);
    int burned = 0;
    for (int i = 0; i < 6; ++i) {
        float before = e.enemy.hp;
        enemyHook(e);
        if (e.enemy.hp == before - 3.0f) ++burned;
        e.player.hp = 100.0f;
        e.player.state = FIGHTER_IDLE;
        tick(e, 0, 5);
    }
    CHECK(burned >= 2); // ~80% dos golpes acertam e queimam o Jack
}

void testVivaldiStaccatoAndSeasons() {
    GameEngine e = skillMatch(5, {SkillId::VivaldiCadenceStaccato, SkillId::VivaldiFourSeasons, SkillId::VivaldiPerfectRhythm});
    press(e, {INPUT_RIGHT, INPUT_RIGHT, INPUT_PUNCH_R});
    tick(e, 0, 45);
    CHECK(e.enemy.hp == 80.0f); // 5 notas de 4
    CHECK(e.comboCount >= 5);
    tick(e, 0, 30);
    tick(e, INPUT_PUNCH_L);
    tick(e, INPUT_PUNCH_L | INPUT_PUNCH_R);
    CHECK(e.player.status.seasons > 0);
    tick(e, 0, 35);
    tick(e, INPUT_PUNCH_L);
    tick(e);
    CHECK(e.player.maxTimer == 21); // jab de 28 quadros, 26% mais curto
}

void testCommandBufferedWhileHit() {
    GameEngine e = skillMatch(5, {SkillId::VivaldiCadenceStaccato, SkillId::VivaldiFourSeasons, SkillId::VivaldiPerfectRhythm});
    e.player.startMove(FIGHTER_HIT, 10);
    tick(e, INPUT_PUNCH_L | INPUT_PUNCH_R);
    CHECK(e.player.status.seasons == 0); // apanhando: ainda não sai
    tick(e, 0, 12);
    CHECK(e.player.status.seasons > 0);  // sai assim que o lutador fica livre
}

void testVivaldiPerfectRhythmStacks() {
    GameEngine e = skillMatch(5, {SkillId::VivaldiCadenceStaccato, SkillId::VivaldiFourSeasons, SkillId::VivaldiPerfectRhythm});
    float first = tapPunch(e, INPUT_PUNCH_L);
    e.enemy.state = FIGHTER_IDLE;
    e.enemy.actionTimer = 1000000; // sem aviso: senão o 2º jab vira contra-ataque
    float second = tapPunch(e, INPUT_PUNCH_L);
    CHECK(first == 6.0f);  // 6 × 1,08 = 6,5 → 6
    CHECK(second == 7.0f); // 6 × 1,16 = 7
}

void testEspinhaNopalAndThorns() {
    GameEngine e = skillMatch(6, {SkillId::EspinhaNopalSlap, SkillId::EspinhaThornVeil, SkillId::EspinhaThornRecoil});
    press(e, {INPUT_RIGHT, INPUT_RIGHT, INPUT_PUNCH_R});
    tick(e, 0, 30);
    CHECK(e.enemy.hp <= 80.0f && e.enemy.hp >= 78.0f); // 21 em dois tapas
    tick(e, 0, 100);
    float before = e.enemy.hp;
    enemyHook(e); // espinhos: 15% de volta
    CHECK(e.player.hp == 76.0f);
    CHECK(before - e.enemy.hp == 4.0f);
}

void testEspinhaLuchaFlyerAirborne() {
    GameEngine e = skillMatch(6, {SkillId::EspinhaLuchaFlyer, SkillId::EspinhaNeedleJab, SkillId::EspinhaRebelSpirit});
    press(e, {INPUT_DOWN, INPUT_UP, INPUT_PUNCH_R});
    CHECK(e.player.activeSkill == SkillId::EspinhaLuchaFlyer);
    tick(e, 0, 8);
    CHECK(e.player.offsetY > 60.0f);
    e.enemy.queuedHeavy = false;
    e.enemy.startMove(FIGHTER_JAB, 28);
    tick(e, 0, 12);
    CHECK(e.player.hp == 100.0f);
    tick(e, 0, 20);
    CHECK(e.enemy.hp == 77.0f);
}

void testMarcovRocketPlasmaZeroG() {
    GameEngine e = skillMatch(7, {SkillId::MarcovRocketThrust, SkillId::MarcovPlasmaBarrier, SkillId::MarcovZeroGravity});
    float radius = e.enemy.radius;
    press(e, {INPUT_RIGHT, INPUT_RIGHT, INPUT_PUNCH_R});
    tick(e, 0, 11);
    CHECK(e.enemy.hp == 77.0f);
    CHECK(e.enemy.radius > radius + 40.0f);
    tick(e, 0, 9);
    tick(e, 0, 30);
    tick(e, INPUT_PUNCH_L);
    tick(e, INPUT_PUNCH_L | INPUT_PUNCH_R);
    CHECK(e.player.status.plasma > 0);
    tick(e, 0, 35);
    e.enemy.radius = 50.0f;
    enemyHook(e); // 24 × 0,75 (titânio) = 18 × 0,45 = 8
    CHECK(e.player.hp == 92.0f);
    // Gravidade Zero: a primeira queda do round vira 1 HP.
    e.player.hp = 2.0f;
    e.player.state = FIGHTER_IDLE;
    tick(e, 0, 5);
    enemyHook(e);
    CHECK(e.gameState == STATE_FIGHTING);
    CHECK(e.player.hp == 1.0f);
}

void testEnemyCastsSkills() {
    std::srand(9);
    GameEngine e;
    e.startMatch(0, 1, false, 1);
    int casts = 0;
    for (int t = 0; t < 60 * 45 && !e.isMatchOver(); ++t) {   // 45 s: com o combo de entrada e os especiais alongados, cada lançamento ocupa mais tempo
        e.update(t % 2 ? INPUT_BLOCK : 0, kDt);
        if (e.enemy.state == FIGHTER_SKILL && e.enemy.timer == e.enemy.maxTimer - 1) ++casts;
    }
    std::printf("    IA lançou %d habilidades em 45 s\n", casts);
    CHECK(casts >= 3);
}

// Aperta e solta um soco rápido; devolve o dano causado.
float tapPunch(GameEngine& e, uint32_t hand, uint32_t direction) {
    float before = e.enemy.hp;
    tick(e, hand | direction);
    tick(e, direction);
    tick(e, direction, 60);
    return before - e.enemy.hp;
}

// Batalhator parado colado no jogador, sem defesa nem habilidades da IA.
GameEngine bossMatch() {
    std::srand(42);
    GameEngine e;
    e.startMatch(0, 8, false, 1);
    e.enemy.radius = 50.0f;
    e.enemy.actionTimer = 1000000;
    e.styleTraits = false;
    e.ai.defenseChance = 0.0f;
    e.ai.useSkills = false;
    e.slipperyChance = 0.0f;
    tick(e, 0, 2);
    return e;
}

// Derruba o chefe com um jab e espera ele levantar.
void knockBossDown(GameEngine& e) {
    e.enemy.hp = 3.0f;
    e.enemy.state = FIGHTER_IDLE;
    e.enemy.actionTimer = 1000000;
    e.enemy.radius = 50.0f;
    e.player.radius = 125.0f;
    tapPunch(e, INPUT_PUNCH_L);
    for (int i = 0; i < 60 * 12 && e.gameState == STATE_KNOCKDOWN; ++i) tick(e);
    e.enemy.radius = 50.0f;
    e.player.radius = 125.0f;
    e.enemy.actionTimer = 1000000;
}

void testBossStagesAndPassives() {
    GameEngine e = bossMatch();
    CHECK(e.enemy.maxHp == 200.0f && e.enemy.hp == 200.0f);
    CHECK(e.enemy.bossStage == 1);
    CHECK(tapPunch(e, INPUT_PUNCH_L) == 4.0f); // jab 6 × 0,65 (Blindagem de Titânio)

    knockBossDown(e); // 1ª queda: robô danificado, remontado com meia vida
    CHECK(e.enemy.bossStage == 2);
    CHECK(e.enemy.loadout.has(SkillId::BossDynamo));
    CHECK(e.gameState == STATE_FIGHTING);
    CHECK(e.enemy.hp == 100.0f);
    float playerBefore = e.player.hp;
    CHECK(tapPunch(e, INPUT_PUNCH_L) == 6.0f);
    CHECK(e.player.hp == playerBefore - 2.0f); // Dínamo: choque mínimo de 2

    knockBossDown(e); // 2ª queda: O Nerd Operador
    CHECK(e.enemy.bossStage == 3);
    CHECK(e.enemy.loadout.has(SkillId::BossQuantum));
    e.enemy.hp = 50.0f;
    enemyHook(e);
    CHECK(e.enemy.hp == 56.0f); // Matriz Quântica: acertar cura 6
    CHECK(e.result == MatchResult::None);
}

void testBossSkills() {
    {
        GameEngine e = bossMatch();
        e.castSkill(false, "batalhator_dual_saw_vortex");
        tick(e, 0, 30);
        CHECK(e.player.hp < 78.0f && e.player.hp > 77.0f); // 22 + sangramento
        CHECK(e.player.status.bleed > 0);
    }
    {
        GameEngine e = bossMatch();
        e.castSkill(false, "batalhator_rocket_barrage");
        tick(e, 0, 50);
        CHECK(e.player.hp == 76.0f); // 4 mísseis de 6
    }
    {
        GameEngine e = bossMatch();
        e.castSkill(false, "batalhator_emp_shockwave");
        tick(e, 0, 17);
        CHECK(e.player.hp == 76.0f);
        CHECK(e.player.state == FIGHTER_HIT); // paralisado
    }
    {
        GameEngine e = bossMatch();
        e.castSkill(false, "batalhator_orbital_apocalypse");
        tick(e, 0, 26);
        CHECK(e.player.hp == 72.0f);
        CHECK(e.player.stamina < 60.0f);
    }
}

void testTournamentBracket() {
    std::srand(3);
    Tournament t = Tournament::create(2);
    CHECK(t.stage == 1 && t.active());
    int seen[8] = {};
    for (const BracketMatch& m : t.quarter) { seen[m.p1]++; seen[m.p2]++; }
    for (int c = 0; c < 8; ++c) CHECK(seen[c] == 1); // os 8 lutadores, uma vez cada
    CHECK(t.quarter[0].p1 == 2 && t.currentOpponent() != 2);

    t.recordWin();
    CHECK(t.stage == 2);
    CHECK(t.semi[0].p1 == 2 && t.semi[0].p2 == t.quarter[1].winner);
    CHECK(t.semi[1].p1 == t.quarter[2].winner && t.semi[1].p2 == t.quarter[3].winner);

    // Salvar e carregar no meio do torneio devolve o mesmo chaveamento.
    auto loaded = Tournament::parse(t.serialize());
    CHECK(loaded.has_value());
    if (loaded) {
        CHECK(loaded->serialize() == t.serialize());
        CHECK(loaded->currentOpponent() == t.currentOpponent());
    }
    CHECK(!Tournament::parse("campaign 2 1 1 2").has_value()); // linha truncada

    t.recordWin();
    CHECK(t.stage == 3 && t.final.p1 == 2 && t.final.p2 == t.semi[1].winner);
    t.recordWin();
    CHECK(t.stage == 4 && t.currentOpponent() == Tournament::kBoss);
    t.recordWin();
    CHECK(t.champion() && !t.active());
    CHECK(t.currentOpponent() == -1);
}

// Junta os sons pedidos pelo motor durante alguns quadros.
std::vector<SoundCue> cuesDuring(GameEngine& e, uint32_t held, int frames) {
    std::vector<SoundCue> all;
    for (int i = 0; i < frames; ++i) {
        e.update(held, kDt);
        all.insert(all.end(), e.sounds.begin(), e.sounds.end());
    }
    return all;
}

bool hasCue(const std::vector<SoundCue>& cues, Sfx id, int param = -1) {
    for (const SoundCue& c : cues) if (c.id == id && (param < 0 || c.param == param)) return true;
    return false;
}

void testSoundCues() {
    GameEngine e = freshMatch();
    auto jab = cuesDuring(e, INPUT_PUNCH_L, 1);
    auto rest = cuesDuring(e, 0, 30);
    jab.insert(jab.end(), rest.begin(), rest.end());
    CHECK(hasCue(jab, Sfx::PunchLight));
    CHECK(hasCue(jab, Sfx::ComboHit, 1));

    e.enemy.hp = 3.0f;
    e.enemy.state = FIGHTER_IDLE;
    auto down = cuesDuring(e, INPUT_PUNCH_L, 1);
    auto count = cuesDuring(e, 0, 90); // a contagem 1 sai 60 quadros depois da queda
    down.insert(down.end(), count.begin(), count.end());
    CHECK(hasCue(down, Sfx::Bell));
    CHECK(hasCue(down, Sfx::CrowdCheer));
    CHECK(hasCue(down, Sfx::RefereeCount, 1));

    GameEngine b = bossMatch();
    b.castSkill(false, "batalhator_rocket_barrage");
    CHECK(hasCue(b.sounds, Sfx::MissileBarrage));
}

void testJabDamage() {
    GameEngine e = freshMatch();
    float dmg = tapPunch(e, INPUT_PUNCH_L);
    // Jab base 6 com carga mínima (1 quadro) arredonda para 6.
    CHECK(dmg == 6.0f);
    CHECK(e.comboCount == 1);
}

void testCrossIsRightHand() {
    GameEngine e = freshMatch();
    tick(e, INPUT_PUNCH_R);
    tick(e);
    CHECK(e.player.state == FIGHTER_CROSS);
}

void testChargedPunchTriplesDamage() {
    GameEngine e = freshMatch();
    tick(e, INPUT_PUNCH_L, GameEngine::kMaxChargeFrames + 2);
    CHECK(e.chargeActive);
    CHECK(e.chargeRatio() == 1.0f);
    float before = e.enemy.hp;
    tick(e);          // solta
    tick(e, 0, 80);   // golpe completo
    CHECK(before - e.enemy.hp == 18.0f); // 6 × (1 + 2 × 1)
}

void testChargeAutoReleases() {
    GameEngine e = freshMatch();
    tick(e, INPUT_PUNCH_L, GameEngine::kMaxChargeFrames + GameEngine::kChargeAutoRelease + 2);
    CHECK(!e.chargeActive);
    CHECK(isPunchState(e.player.state));
}

void testCounterDuringTelegraph() {
    GameEngine e = freshMatch();
    e.enemy.queuedAttack = FIGHTER_JAB;
    e.enemy.queuedHeavy = false;
    e.enemy.startMove(FIGHTER_TELEGRAPH, 1000);
    float dmg = tapPunch(e, INPUT_PUNCH_L);
    CHECK(dmg == 10.0f); // round(6 × 1.6)
    CHECK(e.enemy.state == FIGHTER_HIT || e.enemy.state == FIGHTER_IDLE);
}

void testOutOfReachMisses() {
    GameEngine e = freshMatch();
    e.enemy.radius = 138.0f; // 125 + 138 = 263 > 215
    e.enemy.actionTimer = 1000000;
    float before = e.enemy.hp;
    tick(e, INPUT_PUNCH_L);
    tick(e);
    e.enemy.radius = 138.0f;
    tick(e, 0, 12);
    CHECK(e.enemy.hp == before);
}

void testEnemyBlockChips() {
    GameEngine e = freshMatch();
    e.enemy.startMove(FIGHTER_BLOCK, 1000);
    float dmg = tapPunch(e, INPUT_PUNCH_L);
    CHECK(dmg == 2.0f);
}

void testPlayerBlockAndHeavyHit() {
    GameEngine e = freshMatch();
    // Bloqueio segurado: golpe do oponente tira só 3.
    e.enemy.isLeftPunch = true;
    e.enemy.queuedHeavy = true;
    e.enemy.startMove(FIGHTER_HOOK, 36);
    tick(e, INPUT_BLOCK, 40);
    CHECK(e.player.hp == 97.0f);

    // Sem defesa: gancho pesado tira 24 e corta 8 da stamina máxima.
    GameEngine f = freshMatch();
    f.enemy.queuedHeavy = true;
    f.enemy.startMove(FIGHTER_HOOK, 36);
    tick(f, 0, 40);
    CHECK(f.player.hp == 76.0f);
    CHECK(f.player.maxStamina == 92.0f);
}

void testDoubleTapDodgesBack() {
    // O 1º toque levanta a guarda; a esquiva sai assim que a guarda baixa (como no JS).
    GameEngine e = freshMatch();
    tick(e, INPUT_DOWN);
    tick(e, 0, 3);
    tick(e, INPUT_DOWN);
    bool dodged = false;
    for (int i = 0; i < 20 && !dodged; ++i) {
        tick(e);
        dodged = e.player.state == FIGHTER_DODGE_BACK;
    }
    CHECK(dodged);
}

void testDodgeAvoidsPunch() {
    GameEngine e = freshMatch();
    e.enemy.queuedHeavy = false;
    e.enemy.startMove(FIGHTER_JAB, 28);
    tick(e, INPUT_DOWN | INPUT_LEFT, 2);
    CHECK(e.player.state == FIGHTER_DODGE_L);
    tick(e, 0, 30);
    CHECK(e.player.hp == 100.0f);
}

void testHookDodgeDirection() {
    // Gancho da luva esquerda do oponente vem pela direita da tela: esquivar para a direita apanha.
    for (int way = 0; way < 2; ++way) {
        GameEngine e = freshMatch();
        e.enemy.queuedAttack = FIGHTER_HOOK;
        e.enemy.queuedHeavy = true;
        e.enemy.isLeftPunch = true;
        e.enemy.startMove(FIGHTER_HOOK, 36);
        tick(e, INPUT_DOWN | (way == 0 ? INPUT_LEFT : INPUT_RIGHT), 2);
        tick(e, 0, 40);
        if (way == 0) CHECK(e.player.hp == 100.0f);
        else CHECK(e.player.hp < 100.0f);
    }
}

void testOpponentPatterns() {
    // Jack e King Frog lutam por padrões: golpes com aviso próprio, sequências e brecha.
    for (uint32_t opp = 0; opp <= 7; ++opp) {
        GameEngine e;
        e.startMatch(opp == 2 ? 3 : 2, opp, false, 1);
        e.ai.defenseChance = 0.0f;
        bool sawTell = false, sawOpen = false, sawSkill = false;
        for (int i = 0; i < 60 * 90 && e.gameState == STATE_FIGHTING; ++i) {
            e.player.hp = 100.0f;
            if (i == 60 * 40) e.enemy.hp = 50.0f; // fase 2
            tick(e);
            if (e.enemy.state == FIGHTER_TELEGRAPH && e.enemy.tellKind != 0) sawTell = true;
            if (e.enemy.vulnerable > 0) sawOpen = true;
            if (e.enemy.state == FIGHTER_SKILL) sawSkill = true;
        }
        if (!sawTell || !sawOpen || !sawSkill) std::printf("    oponente %u: aviso %d brecha %d habilidade %d%c", opp, sawTell, sawOpen, sawSkill, 10);
        CHECK(sawOpen);
        CHECK(sawSkill);
    }
}

void testEnemyKnockdownCountAndGetUp() {
    GameEngine e = freshMatch();
    e.enemy.hp = 5.0f;
    tapPunch(e, INPUT_PUNCH_L);
    CHECK(e.enemyKnockdowns == 1);
    CHECK(e.gameState == STATE_KNOCKDOWN || e.gameState == STATE_FIGHTING);
    // Com 1 queda, levanta depois da contagem 3 ou 4 com 40 de vida.
    for (int i = 0; i < 60 * 8 && e.gameState == STATE_KNOCKDOWN; ++i) tick(e);
    CHECK(e.gameState == STATE_FIGHTING);
    CHECK(e.enemy.hp == 40.0f);
    CHECK(e.refereeCount >= 4 && e.refereeCount <= 5);
}

void testThreeKnockdownsIsTko() {
    GameEngine e = freshMatch();
    for (int kd = 0; kd < 3; ++kd) {
        e.enemy.hp = 3.0f;
        e.enemy.radius = 50.0f;
        e.enemy.actionTimer = 1000000;
        e.enemy.state = FIGHTER_IDLE;
        e.player.radius = 125.0f;
        tapPunch(e, INPUT_PUNCH_L);
        for (int i = 0; i < 60 * 12 && e.gameState == STATE_KNOCKDOWN; ++i) tick(e);
    }
    CHECK(e.result == MatchResult::WinTKO);
    CHECK(e.gameState == STATE_VICTORY);
}

void testPlayerMashGetsUp() {
    GameEngine e = freshMatch();
    e.player.hp = 5.0f;
    e.enemy.queuedHeavy = false;
    e.enemy.startMove(FIGHTER_JAB, 28);
    tick(e, 0, 30);
    CHECK(e.gameState == STATE_KNOCKDOWN);
    CHECK(!e.enemyIsDown);
    CHECK(e.mashTarget == 14);
    for (int i = 0; i < 14; ++i) {
        tick(e, INPUT_PUNCH_L);
        tick(e);
    }
    CHECK(e.gameState == STATE_FIGHTING);
    CHECK(e.player.hp == 55.0f); // 30 + 100/1 × 0.25
}

void testPlayerCountedOutIsKo() {
    GameEngine e = freshMatch();
    e.player.hp = 5.0f;
    e.enemy.queuedHeavy = false;
    e.enemy.startMove(FIGHTER_JAB, 28);
    tick(e, 0, 30);
    tick(e, 0, 60 * 11);
    CHECK(e.result == MatchResult::LossKO);
    CHECK(e.gameState == STATE_GAME_OVER);
}

void testRoundsAndDecision() {
    GameEngine e = freshMatch();
    e.player.hp = 60.0f;
    e.enemy.hp = 50.0f;
    e.playerKnockdowns = 1;
    e.roundFrames = 1;
    tick(e);
    CHECK(e.gameState == STATE_ROUND_OVER);
    tick(e, 0, GameEngine::kRoundOverFrames + 1);
    CHECK(e.round == 2);
    CHECK(e.gameState == STATE_FIGHTING);
    CHECK(e.player.hp == 85.0f);
    CHECK(e.enemy.hp == 75.0f);
    CHECK(e.playerKnockdowns == 0);

    e.round = 3;
    e.enemy.hp = 70.0f;
    e.enemy.actionTimer = 1000000;
    e.roundFrames = 1;
    tick(e);
    tick(e, 0, GameEngine::kRoundOverFrames + 1);
    CHECK(e.result == MatchResult::WinDecision);
}

void testComboExpires() {
    GameEngine e = freshMatch();
    tapPunch(e, INPUT_PUNCH_L);
    e.enemy.state = FIGHTER_IDLE;
    e.enemy.actionTimer = 1000000;
    tapPunch(e, INPUT_PUNCH_R);
    CHECK(e.comboCount == 2);
    tick(e, 0, GameEngine::kComboFrames + 5);
    CHECK(e.comboCount == 0);
    CHECK(e.bestCombo == 2);
}

// Lutas inteiras com um "bot" que soca, bloqueia e esquiva: todas precisam terminar com um resultado.
void testFullFightsFinish() {
    int finished = 0, wins = 0;
    for (unsigned seed = 1; seed <= 20; ++seed) {
        std::srand(seed);
        GameEngine e;
        e.startMatch(0, 1, false, 1);
        for (int t = 0; t < 60 * 60 * 12 && !e.isMatchOver(); ++t) {
            uint32_t input = 0;
            if (e.gameState == STATE_KNOCKDOWN) input = (t % 2) ? INPUT_PUNCH_L : 0;
            else if (e.enemy.state == FIGHTER_TELEGRAPH) input = (t % 3 == 0) ? INPUT_PUNCH_R : 0;
            else if (isPunchState(e.enemy.state)) input = INPUT_BLOCK;
            else if (t % 25 == 0) input = INPUT_PUNCH_L;
            e.update(input, kDt);
        }
        if (e.isMatchOver()) ++finished;
        if (e.result == MatchResult::WinKO || e.result == MatchResult::WinTKO || e.result == MatchResult::WinDecision) ++wins;
    }
    std::printf("    20 lutas com bot: %d terminaram, %d vitórias do jogador\n", finished, wins);
    CHECK(finished == 20);
}

void testParryTimingAndAdvantage() {
    GameEngine e = freshMatch();
    e.enemy.isLeftPunch = true;
    e.enemy.queuedHeavy = true;
    e.enemy.startMove(FIGHTER_HOOK, 36);
    // Aguarda até o início do golpe (12 quadros)
    tick(e, 0, 12);
    // Jogador apara exatamente no momento do golpe (bloqueio nos primeiros 5 quadros)
    tick(e, INPUT_BLOCK, 4);
    // Dano 100% anulado (100 HP mantido), oponente desestabilizado e com brecha aberta
    CHECK(e.player.hp == 100.0f);
    CHECK(e.enemy.vulnerable == 25);
    CHECK(e.hitstopTimer > 0);
}

void testSluggishPunchWhenExhausted() {
    GameEngine e = freshMatch();
    e.player.stamina = 0.0f;
    e.player.isExhausted = true;
    float startHp = e.enemy.hp;
    // Jogador com estamina zerada aperta e solta soco: golpe NÃO é bloqueado
    tick(e, INPUT_PUNCH_L);
    tick(e, 0); // solta carga -> desfere o jab
    CHECK(e.player.state == FIGHTER_JAB);
    CHECK(e.player.isSluggish == true);
    // Aguarda o soco conectar (+85% duração)
    tick(e, 0, 45);
    // Dano de soco lento/cansado é reduzido para 30% (2 em vez de 6)
    CHECK(startHp - e.enemy.hp == 2.0f);
}

void testSignatureCleansExhaustionAndStatus() {
    GameEngine e = freshMatch();
    e.player.isExhausted = true;
    e.player.isSluggish = true;
    e.player.stamina = 0.0f;
    e.player.hitKind = 2;
    e.player.status.poisoned = 180;
    e.player.status.blind = 180;

    // Derruba o rival: o jogador fica de pé comemorando com pose triunfante
    e.enemy.hp = 1.0f;
    tick(e, INPUT_PUNCH_L);
    tick(e, 0, 30);
    CHECK(e.gameState == STATE_KNOCKDOWN);
    CHECK(e.enemyIsDown == true);
    CHECK(e.player.isExhausted == false);
    CHECK(e.player.isSluggish == false);
    CHECK(e.player.stamina >= 50.0f);
    CHECK(e.player.hitKind == 0);
    CHECK(e.player.status.poisoned == 0);
    CHECK(e.player.status.blind == 0);
}

void testFighterSizeCategories() {
    // Grandalhões
    Fighter frog(1);
    CHECK(frog.sizeCategory() == FIGHTER_SIZE_GRANDALHAO);
    CHECK(frog.isBigChar());
    CHECK(!frog.isSmallChar());

    Fighter cremoso(3);
    CHECK(cremoso.sizeCategory() == FIGHTER_SIZE_GRANDALHAO);
    CHECK(cremoso.isBigChar());
    CHECK(!cremoso.isSmallChar());

    // Joana: normal na forma base, grandalhão na forma monstruosa
    Fighter joana(2);
    CHECK(joana.sizeCategory() == FIGHTER_SIZE_NORMAL);
    CHECK(joana.isNormalChar());
    CHECK(!joana.isBigChar());
    joana.monstrous = true;
    CHECK(joana.sizeCategory() == FIGHTER_SIZE_GRANDALHAO);
    CHECK(joana.isBigChar());

    // Batalhator: grandalhão nas fases 1 e 2; O Nerd Operador (pequenuxo) na fase 3
    Fighter boss(8);
    boss.bossStage = 1;
    CHECK(boss.sizeCategory() == FIGHTER_SIZE_GRANDALHAO);
    CHECK(boss.isBigChar());
    boss.bossStage = 2;
    CHECK(boss.sizeCategory() == FIGHTER_SIZE_GRANDALHAO);
    CHECK(boss.isBigChar());
    boss.bossStage = 3;
    CHECK(boss.sizeCategory() == FIGHTER_SIZE_PEQUENUXO);
    CHECK(boss.isSmallChar());
    CHECK(!boss.isBigChar());

    // Pequenuxos: Jack Mãos de Colmeia
    Fighter jack(0);
    CHECK(jack.sizeCategory() == FIGHTER_SIZE_PEQUENUXO);
    CHECK(jack.isSmallChar());
    CHECK(!jack.isBigChar());

    // Tamanho normal: Valéria (4), Vivaldi (5), Lady Espinha (6), Marcov (7)
    for (uint32_t id : {4u, 5u, 6u, 7u}) {
        Fighter f(id);
        CHECK(f.sizeCategory() == FIGHTER_SIZE_NORMAL);
        CHECK(f.isNormalChar());
        CHECK(!f.isBigChar());
        CHECK(!f.isSmallChar());
    }
}

} // namespace

// Estilo de cada lutador (include/archetype.hpp): canhão de vidro, enxame, zoneadora e o passo à frente do soco.
void testArchetypes() {
    {   // Valéria bate 25% mais forte: jab de 6 vira 8
        GameEngine e = skillMatch(4, SkillLoadout{}); e.styleTraits = true;
        const float before = e.enemy.hp; tick(e, INPUT_PUNCH_L); tick(e, 0, 20);
        CHECK(before - e.enemy.hp == 8.0f);
    }
    {   // Jack: o soco limpo deixa abelhas no rival
        GameEngine e = freshMatch(); e.styleTraits = true;
        tick(e, INPUT_PUNCH_L); tick(e, 0, 20);
        CHECK(e.enemy.status.infested > 0);
    }
    {   // Vivaldi: o soco limpo afasta o rival (10 a mais)
        GameEngine e = skillMatch(5, SkillLoadout{}); e.styleTraits = true;
        const float r0 = e.enemy.radius; tick(e, INPUT_PUNCH_L); tick(e, 0, 14);
        CHECK(e.enemy.radius >= r0 + 9.0f);
    }
    {   // o soco dá o passo à frente e alcança um rival que ficou 25 além do alcance
        GameEngine e = freshMatch();
        e.enemy.radius = 215.0f - e.player.radius + 25.0f;
        const float before = e.enemy.hp; tick(e, INPUT_PUNCH_L); tick(e, 0, 20);
        CHECK(e.enemy.hp < before);
    }
}

// Medição (não é teste): korama_tests --ai. O que acontece com cada soco isolado, soco martelado e especial do jogador contra cada rival
// (fôlego do jogador sempre cheio e vida sempre reposta, para medir só a reação da IA).
void aiStats() {
    const char* names[8] = {"Jack", "King Frog", "Joana", "Cremoso", "Valeria", "Vivaldi", "Espinha", "Marcov"};
    for (int mode = 0; mode < 3; mode++) {
        std::printf("%s\n", mode == 0 ? "== soco isolado (um jab a cada 70 quadros)" : mode == 1 ? "== martelando o jab" : "== especial do jogador (Soco Magmatico a cada 4 s)");
        int tot[6] = {0, 0, 0, 0, 0, 0};
        for (uint32_t c = 0; c < 8; c++) {
            std::srand(7 + c); GameEngine e; e.startMatch(4, c, false, 1); e.slipperyChance = 0.0f;
            int thrown = 0, clean = 0, counter = 0, open = 0, blocked = 0, dodged = 0, enemyHits = 0; float took = 0.f; bool had = false;
            for (int t = 0; t < 60 * 90; t++) {
                uint32_t held = 0;
                if (mode == 0) held = t % 70 < 3 ? INPUT_PUNCH_L : 0;
                else if (mode == 1) held = t % 10 < 3 ? INPUT_PUNCH_L : 0;   // um jab emendado no outro, sem parar (Z e X alternados rápido viram o comando Z + X)
                else if (t % 240 == 120) { e.castSkill(true, "valeria_magma_punch"); }
                const float php0 = e.player.hp; const FighterActionState es = e.enemy.state; const bool wasOpen = e.enemy.vulnerable > 0;
                e.player.stamina = e.player.maxStamina;
                e.update(held, kDt);
                const bool hitNow = mode == 2 ? (e.player.state == FIGHTER_SKILL && e.player.skillEventDone) : (isPunchState(e.player.state) && e.player.hasHit);
                if (hitNow && !had) { thrown++;
                    if (e.enemy.state == FIGHTER_BLOCK) blocked++; else if (e.enemy.state == FIGHTER_DODGE_L || e.enemy.state == FIGHTER_DODGE_R) dodged++;
                    else if (e.enemy.state == FIGHTER_HIT || e.enemy.hp < e.enemy.maxHp - 0.5f) { clean++; if (wasOpen) open++; else if (es == FIGHTER_TELEGRAPH) counter++; } }
                had = hitNow;
                if (e.player.hp < php0 - 0.5f) { enemyHits++; took += php0 - e.player.hp; }
                e.enemy.hp = e.enemy.maxHp; e.player.hp = e.player.maxHp; e.enemy.status.burn = 0; e.enemy.status.infested = 0; e.enemy.status.poisoned = 0; e.enemy.status.bleed = 0; e.player.status = StatusEffects{};
                if (e.gameState != STATE_FIGHTING) break;
            }
            const int miss = thrown - clean - blocked - dodged;
            std::printf("  %-10s golpes %3d | limpos %3d (%2d%%): no aviso %2d, na brecha %2d | guarda %3d | esquiva %3d | fora %2d | rival acertou %2d vezes (%.0f de dano)\n", names[c], thrown, clean, thrown ? clean * 100 / thrown : 0, counter, open, blocked, dodged, miss, enemyHits, took);
            tot[0] += thrown; tot[1] += clean; tot[2] += blocked; tot[3] += dodged; tot[4] += enemyHits; tot[5] += counter;
        }
        const int n = std::max(1, tot[0]);
        std::printf("  TOTAL: limpos %d%% (no aviso %d%%), guarda %d%%, esquiva %d%% | rival acertou %d vezes em 12 min\n", tot[1] * 100 / n, tot[5] * 100 / n, tot[2] * 100 / n, tot[3] * 100 / n, tot[4]);
    }
}

// Medição (não é teste): korama_tests --estilos. Cada lutador na mão do jogador, com o mesmo roteiro (jab e direto alternados a cada 36
// quadros, sem defender), contra o mesmo rival; e cada lutador como rival do mesmo jogador. Vida reposta a cada quadro, fôlego cheio.
void styleStats(bool traits) {
    const char* names[8] = {"Jack", "King Frog", "Joana", "Cremoso", "Valeria", "Vivaldi", "Espinha", "Marcov"};
    for (int side = 0; side < 2; side++) {
        std::printf("%s\n", side == 0 ? "== na mao do jogador (rival: Cremoso; o Cremoso enfrenta o Jack)" : "== como rival (jogador: Joana; a Joana enfrenta o Jack)");
        for (uint32_t c = 0; c < 8; c++) {
            const uint32_t pc = side == 0 ? c : (c == 2 ? 0u : 2u), ec = side == 0 ? (c == 3 ? 0u : 3u) : c;
            float dealt = 0.f, taken = 0.f; int landed = 0, thrown = 0, whiff = 0; bool had = false;
            for (int seed = 0; seed < 3; seed++) { std::srand(31 + seed * 7 + c); GameEngine e; e.startMatch(pc, ec, false, 1); e.slipperyChance = 0.0f; e.styleTraits = traits;
                for (int t = 0; t < 60 * 60; t++) {
                    const uint32_t held = t % 36 < 3 ? ((t / 36) % 2 ? INPUT_PUNCH_R : INPUT_PUNCH_L) : 0;
                    e.player.stamina = e.player.maxStamina; const float eh = e.enemy.hp, ph = e.player.hp;
                    e.update(held, kDt);
                    const bool hitNow = isPunchState(e.player.state) && e.player.hasHit;
                    if (hitNow && !had) { thrown++; if (e.enemy.hp < eh - 0.5f) landed++; else if (e.enemy.state != FIGHTER_BLOCK && e.enemy.state != FIGHTER_DODGE_L && e.enemy.state != FIGHTER_DODGE_R) whiff++; }
                    had = hitNow; dealt += std::max(0.f, eh - e.enemy.hp); taken += std::max(0.f, ph - e.player.hp);
                    e.enemy.hp = e.enemy.maxHp; e.player.hp = e.player.maxHp;
                    if (e.gameState != STATE_FIGHTING) break;
                } }
            std::printf("  %-10s causou %4.0f | levou %4.0f | saldo %+5.0f | socos limpos %2d%% | no vazio %2d%%\n", names[c], dealt / 3.f, taken / 3.f, (dealt - taken) / 3.f, thrown ? landed * 100 / thrown : 0, thrown ? whiff * 100 / thrown : 0);
        }
    }
}

int main(int argc, char** argv) {
    if (argc > 1 && std::string(argv[1]) == "--ai") { aiStats(); return 0; }
    if (argc > 1 && std::string(argv[1]) == "--estilos") { styleStats(!(argc > 2 && std::string(argv[2]) == "off")); return 0; }   // "off": sem os estilos, para comparar
    const std::vector<std::pair<const char*, std::function<void()>>> tests = {
        {"jab causa 6 e soma combo", testJabDamage},
        {"estilos: canhão de vidro, enxame, zoneadora e passo à frente", testArchetypes},
        {"soco direito é cross", testCrossIsRightHand},
        {"soco carregado triplica o dano", testChargedPunchTriplesDamage},
        {"carga solta sozinha", testChargeAutoReleases},
        {"contra-ataque no aviso ×1,6", testCounterDuringTelegraph},
        {"fora de alcance não acerta", testOutOfReachMisses},
        {"bloqueio do oponente tira só 2", testEnemyBlockChips},
        {"bloqueio do jogador e golpe pesado", testPlayerBlockAndHeavyHit},
        {"↓↓ esquiva para trás", testDoubleTapDodgesBack},
        {"esquiva lateral evita o golpe", testDodgeAvoidsPunch},
        {"gancho: esquiva para o lado certo", testHookDodgeDirection},
        {"oponentes lutam por padrões", testOpponentPatterns},
        {"queda do oponente: contagem e levanta", testEnemyKnockdownCountAndGetUp},
        {"3 quedas no round = TKO", testThreeKnockdownsIsTko},
        {"jogador levanta martelando Z/X", testPlayerMashGetsUp},
        {"jogador contado até 10 = KO", testPlayerCountedOutIsKo},
        {"rounds e decisão por pontos", testRoundsAndDecision},
        {"combo expira", testComboExpires},
        {"lutas completas terminam", testFullFightsFinish},
        {"Agulhada Rainha (↑↑+X) fura a guarda", testStingJabCommand},
        {"Geleia Real cura 20 com recarga", testRoyalJellyHealsWithCooldown},
        {"Infestação: dano por segundo", testInfestationDamageOverTime},
        {"Modo Melaço: metade do dano e lentidão", testMolassesHalvesDamageAndSlows},
        {"Favo Protetor absorve 3 golpes", testHoneyWallAbsorbsThreeHits},
        {"Golpe de Língua empurra e deixa lento", testFrogTonguePushesAndSlows},
        {"Pele Venenosa envenena", testFrogPoisonSkin},
        {"Salto Real: invulnerável no ar e 22 de dano", testRoyalLeapAirborneAndImpact},
        {"lama impede esquiva", testMudBlocksDodge},
        {"cegueira erra ~40%", testBlindMakesAttacksMiss},
        {"Bênção da Rainha cura no KD", testQueenBlessingHealsOnKnockdown},
        {"IA usa habilidades", testEnemyCastsSkills},
        {"Joana: Tesoura quebra a guarda (25)", testJoanaClawCrushBreaksGuard},
        {"Joana: Arrastão puxa o oponente", testJoanaHookPullDragsIn},
        {"Joana: Cracas seguram 70% e refletem 35%", testJoanaShellReducesAndReflects},
        {"Joana: Coral sangra", testJoanaCoralBleeds},
        {"Joana: Onda da Âncora deixa lento", testJoanaAnchorSlows},
        {"Joana: alcance do gancho e forma monstruosa", testJoanaMonstrousAndReach},
        {"Cremoso: calda queima e Modo Derretido", testCremosoSyrupAndMelt},
        {"Cremoso: amortecedor e Vingança Doce", testCremosoCushionAndRevenge},
        {"Valéria: Soco Magmático e Dança das Brasas", testValeriaMagmaAndColdMagma},
        {"Valéria: Escudo de Calor queima quem bate", testValeriaHeatwaveBurnsAttacker},
        {"Vivaldi: Staccato e Quatro Estações", testVivaldiStaccatoAndSeasons},
        {"Vivaldi: Ritmo Perfeito acumula", testVivaldiPerfectRhythmStacks},
        {"comando fechado apanhando sai depois", testCommandBufferedWhileHit},
        {"Batalhator: 3 fases e passivas", testBossStagesAndPassives},
        {"Batalhator: habilidades", testBossSkills},
        {"torneio: chaves, avanço e save", testTournamentBracket},
        {"sons pedidos pelo motor", testSoundCues},
        {"Espinha: Garra del Cacto e espinhos", testEspinhaNopalAndThorns},
        {"Espinha: Salto Luchador no ar", testEspinhaLuchaFlyerAirborne},
        {"Marcov: foguete, Escudo Sputnik e Gravidade Zero", testMarcovRocketPlasmaZeroG},
        {"Parry: bloqueio perfeito anula dano e abre o oponente", testParryTimingAndAdvantage},
        {"Baixa estamina: soco 85% mais lento e 30% dano sem travar input", testSluggishPunchWhenExhausted},
        {"Assinatura: limpa exaustão e status para pose heroica limpa", testSignatureCleansExhaustionAndStatus},
        {"categorias de tamanho dos lutadores", testFighterSizeCategories},
    };
    for (const auto& [name, fn] : tests) {
        int before = g_failures;
        fn();
        std::printf("%s %s\n", g_failures == before ? "[ok]  " : "[FALHA]", name);
    }
    std::printf("\n%s: %d falha(s)\n", g_failures == 0 ? "SUCESSO" : "ERRO", g_failures);
    return g_failures == 0 ? 0 : 1;
}
