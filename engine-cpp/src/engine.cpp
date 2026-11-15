#include "engine.hpp"
#include "archetype.hpp"
#include "skill_poses.hpp"
#include "physics.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>

namespace KoRama {

namespace {

constexpr float kPi = 3.14159265f;

float random01() {
    return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
}

// Ponto à frente do lutador (em direção ao oponente), na altura y.
Vec3 frontOf(const Fighter& f, float y, float forward = 20.0f) {
    return Physics3D::localToWorld(0.0f, y, forward, f.worldPos, f.facing);
}

// Características próprias de cada lutador que valem sem habilidade equipada (passivas intrínsecas do JS).
// Por enquanto só Jack e King Frog, a dupla da fatia vertical; o resto do elenco entra na fase 5.
float applyDefenderTraits(Fighter& defender, float damage, CombatSystem& combat, float slipperyChance, bool heavy, bool jab) {
    if (defender.loadout.has(SkillId::JackHoneySkin) && defender.hp <= 45.0f) {
        damage = std::round(damage * 0.65f); // Jack: Pele de Mel reduz 35% de dano
        defender.stamina = std::min(defender.maxStamina, defender.stamina + 2.5f); // recupera fôlego sob pressão
    }
    float slipRate = (slipperyChance <= 0.0f) ? 0.0f : (defender.loadout.has(SkillId::FrogSlipperySkin) ? 0.30f : slipperyChance);
    if (defender.loadout.has(SkillId::FrogSlipperySkin) && random01() < slipRate) {
        damage = std::round(damage * 0.50f); // King Frog: pele mucosa absorve metade
        defender.slipperyProcs++;            // o visual da deflexão aquosa reage a isso
        defender.stamina = std::min(defender.maxStamina, defender.stamina + 12.0f); // Mucosa restaura fôlego!
        combat.triggerStatusBanner(defender.isPlayer ? "¡PELE MUCOSA! (+STAMINA)" : "¡KING FROG: PELE MUCOSA!");
    }
    if (defender.loadout.has(SkillId::JoanaCalcifiedArmor)) {
        damage = std::round(damage * 0.80f); // Joana: Carapaça Calcária (-20%)
    }
    if (defender.loadout.has(SkillId::CremosoCreamCushion) && defender.state == FIGHTER_BLOCK) {
        damage = std::round(damage * 0.50f); // Cremoso: Amortecedor reduz chip damage pela metade
        defender.stamina = std::min(defender.maxStamina, defender.stamina + 6.0f); // e converte em stamina!
        combat.triggerStatusBanner(defender.isPlayer ? "¡AMORTECEDOR (+STAMINA)!" : "¡CREMOSO: AMORTECEDOR!");
    } else if (defender.loadout.has(SkillId::CremosoCreamCushion) && jab) {
        damage = std::round(damage * 0.75f); // Cremoso: Amortecedor contra jabs
    }
    if (defender.loadout.has(SkillId::MarcovTitaniumSuit) && heavy) {
        damage = std::round(damage * 0.75f); // Marcov: Traje de Titânio (-25% pesados)
    }
    if (defender.coldMagma) damage = std::round(damage * 0.50f); // Valéria: Magma Frio
    if (defender.loadout.has(SkillId::BossTitanium)) damage = std::round(damage * 0.65f); // Batalhator fase 1
    return damage;
}

float applyAttackerTraits(const Fighter& attacker, float damage, FighterActionState move) {
    if (attacker.loadout.has(SkillId::FrogAmphibianRage) && attacker.stamina < 30.0f) {
        damage += 3.0f; // King Frog: Fúria Anfíbia (+3 dano)
    }
    if (attacker.loadout.has(SkillId::JoanaAsymmetricArms)) {
        // Joana: Armamento Assimétrico (+25% em ganchos, uppers e direto da garra)
        if (move == FIGHTER_HOOK || move == FIGHTER_UPPER || move == FIGHTER_CROSS) damage = std::round(damage * 1.25f);
    }
    if (attacker.loadout.has(SkillId::ValeriaVolcanicRage) && attacker.hp <= 35.0f) {
        damage = std::round(damage * 1.30f); // Valéria: Fúria Vulcânica (+30%)
    }
    return damage;
}

float dodgeCostFor(const Fighter& f) {
    if (f.loadout.has(SkillId::ValeriaEmberDance)) return 0.0f;  // Valéria: Dança das Brasas (grátis)
    if (f.loadout.has(SkillId::FrogJumper)) return 3.0f;        // King Frog: Saltador (-50% custo)
    if (f.loadout.has(SkillId::JackBeekeeper)) return 4.5f;     // Apicultor
    return 6.0f;                                               // Base: 6.0f (sustentável para combate defensivo)
}

constexpr uint32_t kDirectionBits = INPUT_UP | INPUT_DOWN | INPUT_LEFT | INPUT_RIGHT;
constexpr uint32_t kPunchBits = INPUT_PUNCH_L | INPUT_PUNCH_R;
constexpr int kCommandWindow = 72;       // 1,2 s de histórico para comandos
constexpr int kBothPunchesWindow = 24;   // Z + X em até 400 ms
constexpr int kChainGap = 40;            // quadros entre o fim de um soco do combo e o próximo antes de o combo esfriar
constexpr float kChainPunchCost = 0.6f;  // fração do fôlego que custa o soco que carrega um espaço do combo
// (o passo à frente máximo de cada lutador fica na ficha de arquétipo: include/archetype.hpp)
constexpr float kLungeSlack = 6.0f;     // fecha até esta folga dentro do alcance
constexpr int kFlowWindow = 60;          // quadros, depois de uma esquiva bem feita, em que um especial de fluxo sai sem combo
constexpr int kHealCooldown = 8 * 60;

} // namespace

GameEngine::GameEngine() {
    init();
}

void GameEngine::init() {
    frameIndex = 0;
    gameState = STATE_TITLE;
    matchTime = 0.0f;
    ringAngle = 0.0f;
    screenShake = 0.0f;

    SkillSystem::init();
    player.initCharacter(0, true);
    enemy.initCharacter(1, false);

    combat.clear();
    particles.clear();
    std::memset(&renderState, 0, sizeof(renderState));
}

void GameEngine::startMatch(uint32_t playerChar, uint32_t enemyChar, bool arcade, int stage, const SkillLoadout* playerLoadout, bool training) {
    trainingMode = training || (enemyChar == 9);
    player.initCharacter(playerChar, true);
    enemy.initCharacter(trainingMode ? 9 : enemyChar, false);
    player.loadout = playerLoadout ? *playerLoadout : SkillSystem::defaultLoadout(playerChar);
    enemy.loadout = trainingMode ? SkillLoadout{} : SkillSystem::randomLoadout(enemyChar);
    if (trainingMode) {
        enemy.name = "ESPANTALHO";
        enemy.hp = enemy.maxHp = 100.0f;
        enemy.radius = 50.0f;
    } else if (enemyChar == 8) {
        // Batalhator: 200 de vida, abre rápido e troca de build a cada queda.
        enemy.hp = enemy.maxHp = 200.0f;
        enemy.loadout = SkillSystem::bossLoadout(1);
    }
    commandHistory_.clear();
    healBannerShown_ = false;

    isArcadeMode = arcade && !trainingMode;
    arcadeStage = stage;
    isFinalBoss = !trainingMode && (enemyChar == 8);

    round = 1;
    roundFrames = kRoundFrames;
    roundOverTimer = 0;
    playerKnockdowns = enemyKnockdowns = 0;
    enemyIsDown = false;
    refereeCount = refereeTimer = 0;
    mashProgress = mashTarget = 0;
    result = MatchResult::None;
    playerDamageTaken = 0.0f;
    prevPlayerHp_ = player.hp;
    comboCount = comboTimer = bestCombo = 0;
    chain_ = chainTimer_ = flowWindow_ = 0;
    hitstopTimer = 0;
    cancelCharge();

    prevInput_ = 0;
    framesSinceDownTap_ = 1000;
    pendingDodgeBack_ = false;
    bufferedHand_ = -1;
    bufferedAge_ = 0;

    matchTime = 0.0f;
    ringAngle = 0.0f;
    gameState = STATE_FIGHTING;
    screenShake = 0.0f;

    combat.clear();
    particles.clear();
    ai.reset(enemyChar);
    if (enemyChar == 8) enemy.actionTimer = 15;
    sounds.clear();
    cue(Sfx::Bell);
    placeFighters();
}

float GameEngine::chargeRatio() const {
    return chargeActive ? std::min(1.0f, static_cast<float>(chargeFrames) / kMaxChargeFrames) : 0.0f;
}

bool GameEngine::isPerfectVictory() const {
    const bool won = (result == MatchResult::WinKO || result == MatchResult::WinTKO || result == MatchResult::WinDecision);
    if (!won) return false;
    if (playerKnockdowns > 0) return false;
    if (playerDamageTaken > 0.001f) return false;
    if (player.hp < player.maxHp - 0.001f) return false;
    return true;
}

void GameEngine::startRoundIntro(int frames) {
    gameState = STATE_ROUND_INTRO;
    roundIntroDuration = frames;
    roundIntroTimer = frames;
}

void GameEngine::update(uint32_t inputMask, float dt) {
    frameIndex++;
    sounds.clear();
    uint32_t audio = AUDIO_NONE;

    const uint32_t pressed = inputMask & ~prevInput_;
    const uint32_t released = prevInput_ & ~inputMask;
    prevInput_ = inputMask;

    framesSinceDownTap_ = std::min(framesSinceDownTap_ + 1, 1000);
    bufferedAge_ = std::min(bufferedAge_ + 1, 1000);
    if (screenShake > 0.0f) screenShake = std::max(0.0f, screenShake - 1.2f);
    if (impactTimer > 0) impactTimer--;
    if (hurtTimer > 0) hurtTimer--;

    switch (gameState) {
    case STATE_ROUND_INTRO:
        if (--roundIntroTimer <= 0) {
            gameState = STATE_FIGHTING;
            cue(Sfx::Bell);
        }
        break;
    case STATE_FIGHTING:
        if (hitstopTimer > 0) hitstopTimer--; // congela a ação por alguns quadros no impacto
        else updateFighting(inputMask, pressed, released, audio);
        break;
    case STATE_KNOCKDOWN:
        updateKnockdown(pressed, audio);
        break;
    case STATE_ROUND_OVER:
        if (--roundOverTimer <= 0) startNextRound(audio);
        break;
    case STATE_VICTORY:
    case STATE_GAME_OVER: {
        // O perdedor por nocaute escorrega para as cordas.
        Fighter& loser = (result == MatchResult::WinKO || result == MatchResult::WinTKO) ? enemy : player;
        if (loser.state == FIGHTER_KNOCKDOWN && loser.radius < 232.0f) loser.radius = std::min(232.0f, loser.radius + 4.0f); // arremessado: chega às cordas mais rápido que numa queda comum
        if (loser.state == FIGHTER_KNOCKDOWN && loser.radius >= 231.0f && koRopeFrame == 0) {
            koRopeFrame = frameIndex;
            cue(Sfx::CrowdGasp);
        }
        break;
    }
    default:
        break;
    }

    combat.update(dt);
    particles.update(dt);

    if (player.hp < prevPlayerHp_) {
        playerDamageTaken += (prevPlayerHp_ - player.hp);
    }
    prevPlayerHp_ = player.hp;

    matchTime = static_cast<float>(kRoundFrames - roundFrames) / 60.0f;
    placeFighters();
    fillRenderState(audio);
}

// ---------------------------------------------------------------------------
// Luta em andamento
// ---------------------------------------------------------------------------

void GameEngine::updateFighting(uint32_t held, uint32_t pressed, uint32_t released, uint32_t& audio) {
    ringAngle -= 0.0016f; // o ringue gira devagar durante a luta

    // combo de entrada dos especiais: esfria com o tempo e quebra quando o jogador apanha
    if (chain_ > 0 && (--chainTimer_ <= 0 || player.state == FIGHTER_HIT || player.state == FIGHTER_KNOCKDOWN)) chain_ = 0;
    if (flowWindow_ > 0) flowWindow_--;

    if (pressed & INPUT_DOWN) {
        if (framesSinceDownTap_ < kDoubleTapFrames) pendingDodgeBack_ = true;
        framesSinceDownTap_ = 0;
    }

    if (pressed & INPUT_DODGE_KEY) {
        uint32_t d = INPUT_DUCK;
        if (held & INPUT_LEFT) d = INPUT_DODGE_L;
        else if (held & INPUT_RIGHT) d = INPUT_DODGE_R;
        else if (held & INPUT_DOWN) d = INPUT_DODGE_B;
        if (player.state != FIGHTER_IDLE && player.state != FIGHTER_BLOCK) {
            bufferedDodge_ = d;
            bufferedDodgeAge_ = 0;
        }
        player.dodgeLock = false;
    } else if (pressed & (INPUT_DODGE_L | INPUT_DODGE_R | INPUT_DODGE_B | INPUT_DUCK)) {
        if (player.state != FIGHTER_IDLE && player.state != FIGHTER_BLOCK) {
            bufferedDodge_ = pressed & (INPUT_DODGE_L | INPUT_DODGE_R | INPUT_DODGE_B | INPUT_DUCK);
            bufferedDodgeAge_ = 0;
        }
        player.dodgeLock = false;
    }
    if (bufferedDodge_ != 0) {
        if (++bufferedDodgeAge_ >= kBufferFrames) {
            bufferedDodge_ = 0;
        }
    }

    // Comandos especiais: histórico de apertos dos últimos 1,2 s.
    commandHistory_.erase(std::remove_if(commandHistory_.begin(), commandHistory_.end(),
                                         [&](const InputEvent& e) { return frameIndex - e.frame > kCommandWindow; }),
                          commandHistory_.end());
    uint32_t consumed = 0;
    for (uint32_t bit : {INPUT_UP, INPUT_DOWN, INPUT_LEFT, INPUT_RIGHT, INPUT_PUNCH_L, INPUT_PUNCH_R}) {
        if (!(pressed & bit)) continue;
        commandHistory_.push_back({bit, frameIndex});
        if (tryPlayerCommand(held, audio)) consumed = pressed; // o aperto que fechou o comando não vira soco
    }

    // Aperto: parado começa a carregar; ocupado guarda o soco no buffer.
    for (int hand = 0; hand < 2; ++hand) {
        uint32_t bit = hand == 0 ? INPUT_PUNCH_L : INPUT_PUNCH_R;
        if ((pressed & bit) && !(consumed & bit)) {
            if (player.state == FIGHTER_IDLE) {
                chargeActive = true;
                chargeLeft = hand == 0;
                chargeFrames = 0;
            } else {
                bufferedHand_ = hand;
                bufferedAge_ = 0;
            }
        }
        if (released & bit) {
            if (chargeActive && chargeLeft == (hand == 0)) releaseCharge(held);
            else if (player.state != FIGHTER_IDLE && player.state != FIGHTER_SKILL) { bufferedHand_ = hand; bufferedAge_ = 0; }
        }
    }

    // Lentidão (-35%): o lutador perde 1 de cada 3 quadros.
    const bool playerSlowedNow = player.status.slowed > 0 && frameIndex % 3 == 0;
    const bool enemySlowedNow = enemy.status.slowed > 0 && frameIndex % 3 == 0;

    if (!playerSlowedNow) updatePlayer(held, audio);
    if (gameState != STATE_FIGHTING) return;

    if (trainingMode || enemy.charId == 9) {
        // Espantalho (Modo Treino): fica no centro do ringue e reage aos golpes sem atacar.
        if (enemy.state == FIGHTER_HIT) {
            if (--enemy.timer <= 0) {
                enemy.state = FIGHTER_IDLE;
                enemy.resetPose();
            }
        }
        enemy.radius = 50.0f;
    } else if (!enemySlowedNow) {
        if (enemy.state == FIGHTER_SKILL) {
            updateSkillMove(enemy, player, audio, true);
        } else {
            const int wasOpen = enemy.vulnerable;
            auto applyAiActionMotion = [&](const AiAction& act) {
                if (act.ringTurnDelta != 0.0f) ringAngle += act.ringTurnDelta;
                if (act.dodgeSmoke) {
                    Vec3 at = Physics3D::localToWorld(act.smokeX, 5.0f, 0.0f, enemy.worldPos, enemy.facing);
                    particles.spawnImpact(at.x, at.y, at.z, 12, 0xddddddff);
                }
            };
            AiAction action = ai.update(enemy, player, frameIndex, audio);
            applyAiActionMotion(action);
            // especial do oponente sendo armado: tique de carga subindo, para o jogador ouvir que vem coisa
            if (enemy.state == FIGHTER_TELEGRAPH && enemy.queuedSkill != SkillId::None && !enemy.queuedSignature && enemy.timer % 6 == 0)
                cue(Sfx::ChargeTick, std::clamp(10 - enemy.timer * 10 / std::max(1, enemy.maxTimer), 0, 10));
            if (wasOpen == 0 && enemy.vulnerable > 0) // brecha do padrão de luta: avisa o jogador
                combat.spawnFloatingText("BRECHA!", 0x00f0ffff, enemy.worldPos.x, enemy.worldPos.y + 110.0f, enemy.worldPos.z, true, 0);
            // Quatro Estações: a IA ganha um quadro extra a cada três.
            if (!action.checkHit && action.cast == SkillId::None && enemy.status.seasons > 0 && frameIndex % 3 == 0 &&
                enemy.state != FIGHTER_SKILL) {
                AiAction extraAction = ai.update(enemy, player, frameIndex, audio);
                applyAiActionMotion(extraAction);
                action = extraAction;
            }
            // o soco do oponente também dá o passo à frente quando o jogador ficou um pouco além do alcance (até o limite do estilo dele)
            if (!isPunchState(enemy.state)) enemyLungeDone_ = 0.0f;
            else if (!enemy.hasHit && !action.checkHit && styleTraits) {
                const float reachE = 210.0f + arch(enemy.charId).reach + ((player.isBigChar() || enemy.isBigChar()) ? 25.0f : 0.0f);
                const float gap = (player.radius + enemy.radius) - reachE + kLungeSlack, lungeMax = arch(enemy.charId).lunge * 0.5f;   // metade do passo do jogador: o oponente ainda erra quando é empurrado para longe
                if (gap > 0.0f && enemyLungeDone_ < lungeMax) { const float step = std::min({6.0f, gap, lungeMax - enemyLungeDone_, enemy.radius - 20.0f}); if (step > 0.0f) { enemy.radius -= step; enemyLungeDone_ += step; } }
            }
            if (action.checkHit) checkHitOnPlayer(enemy.queuedHeavy, audio);
            else if (action.cast != SkillId::None) castSkill(enemy, player, action.cast, action.signature, audio);
        }
    }
    if (gameState != STATE_FIGHTING) return;

    tickStatus(player, audio);
    if (gameState != STATE_FIGHTING) return;
    tickStatus(enemy, audio);
    if (gameState != STATE_FIGHTING) return;

    if (comboTimer > 0 && --comboTimer == 0) resetCombo();

    // Modo Treino: quando o combo termina e o jogador para de atacar, a vida regenera 100% suavemente
    if ((trainingMode || enemy.charId == 9) && comboTimer == 0 && player.state == FIGHTER_IDLE) {
        if (enemy.hp < enemy.maxHp) {
            enemy.hp = std::min(enemy.maxHp, enemy.hp + 2.0f);
        }
    }

    if (!trainingMode) {
        if (--roundFrames <= 0) {
            roundFrames = 0;
            gameState = STATE_ROUND_OVER;
            roundOverTimer = kRoundOverFrames;
            audio |= AUDIO_SFX_GONG;
            cue(Sfx::Bell);
            cancelCharge();
            player.state = enemy.state = FIGHTER_IDLE;
            player.resetPose();
            enemy.resetPose();
            combat.triggerStatusBanner("FIM DO ROUND " + std::to_string(round) + "!");
        }
    } else {
        roundFrames = kRoundFrames; // no modo treino o cronômetro é infinito
    }
}

void GameEngine::updatePlayer(uint32_t held, uint32_t& audio) {
    Fighter& f = player;

    if (f.staminaRegenDelay > 0) f.staminaRegenDelay--;
    else {
        float regen = 0.35f;
        if (f.loadout.has(SkillId::JoanaDeepSeaTide) && f.hp < 40.0f) regen *= 1.5f;
        if (f.loadout.has(SkillId::CremosoFrozenStamina)) regen *= 1.35f;
        if (f.loadout.has(SkillId::EspinhaVivaCaatinga)) regen *= 2.0f - f.hp / f.maxHp;
        f.stamina = std::min(f.maxStamina, f.stamina + regen);
    }
    if (f.isExhausted && f.stamina >= 50.0f) f.isExhausted = false;
    else if (f.stamina <= 0.0f) {
        if (f.loadout.has(SkillId::ValeriaColdMagma) && !f.internalIgnitionUsed) {
            f.internalIgnitionUsed = true;
            f.isExhausted = false;
            f.stamina = 30.0f;
            f.staminaRegenDelay = 0;
            enemy.hp = std::max(0.0f, enemy.hp - 8.0f);
            combat.triggerStatusBanner("¡IGNIÇÃO INTERNA! (+30 STAMINA)");
            Vec3 bPos = frontOf(enemy, 60.0f, 0.0f);
            particles.spawnImpact(bPos.x, bPos.y, bPos.z, 20, 0xff4500ff);
            combat.spawnFloatingText("-8 EXPLOSÃO!", 0xff4500ff, enemy.worldPos.x, enemy.worldPos.y + 90.0f, enemy.worldPos.z, false, 0);
        } else {
            f.isExhausted = true;
        }
    }

    f.resetParts();

    if (chargeActive) {
        f.offsetX *= 0.85f;
        f.offsetY *= 0.85f;
        f.bodySkew *= 0.85f;
        chargeFrames++;
        if (chargeFrames % 6 == 0) cue(Sfx::ChargeTick, static_cast<int>(chargeRatio() * 10.0f));
        if (chargeFrames >= kMaxChargeFrames + kChargeAutoRelease) {
            releaseCharge(held);
        } else {
            f.animateChargingPose(chargeLeft, chargeRatio(), random01() - 0.5f, random01() - 0.5f, random01() - 0.5f);
        }
        return;
    }

    if (f.state == FIGHTER_IDLE) {
        // Comando fechado enquanto apanhava ou golpeava: sai assim que o lutador fica livre, como o buffer de soco.
        if (tryBufferedCommand(held, audio)) return;
        if (bufferedDodge_ != 0 && bufferedDodgeAge_ < kBufferFrames) {
            uint32_t d = bufferedDodge_;
            bufferedDodge_ = 0;
            f.dodgeLock = false;
            playerMovementAndBlock(held | d, audio);
            if (f.state != FIGHTER_IDLE) return;
        }
        if (bufferedHand_ >= 0 && bufferedAge_ < kBufferFrames) {
            bool left = bufferedHand_ == 0;
            bufferedHand_ = -1;
            executePunch(left, 0.0f, held);
            return;
        }
        f.radius += (f.idleRadius() - f.radius) * 0.045f;
        if (std::fabs(f.radius - f.idleRadius()) < 0.5f) f.radius = f.idleRadius();
        f.applyIdleBob(frameIndex);
        playerMovementAndBlock(held, audio);
    } else if (f.state == FIGHTER_SKILL) {
        updateSkillMove(player, enemy, audio, true);
    } else {
        playerHandleAction(held, audio, true);
    }
}

void GameEngine::playerMovementAndBlock(uint32_t held, uint32_t& audio) {
    Fighter& f = player;
    if (held & INPUT_DODGE_KEY) {
        if (held & INPUT_LEFT) held |= INPUT_DODGE_L;
        else if (held & INPUT_RIGHT) held |= INPUT_DODGE_R;
        else if (held & INPUT_DOWN) held |= INPUT_DODGE_B;
        else held |= INPUT_DUCK;
    }
    // Segurar o direcional para inclinar o lutador (Esquerda / Direita) NUNCA bloqueia esquivas.
    // O dodgeLock só é mantido enquanto o gatilho da esquiva estiver ativamente segurado (C, botões dedicados ou Baixo+Lado).
    const bool holdingDodgeInput = (held & (INPUT_DODGE_KEY | INPUT_DODGE_L | INPUT_DODGE_R | INPUT_DODGE_B | INPUT_DUCK)) ||
                                   ((held & INPUT_DOWN) && (held & (INPUT_LEFT | INPUT_RIGHT)));
    if (!holdingDodgeInput) {
        f.dodgeLock = false;
    }
    const float cost = dodgeCostFor(f);

    auto startDodge = [&](FighterActionState move, int baseDuration) {
        int duration = baseDuration;
        if (f.stamina < cost || f.isExhausted) {
            f.isSluggish = true;
            f.stamina = 0.0f;
            f.staminaRegenDelay = 60;
            duration = static_cast<int>(std::round(baseDuration * 1.75f));
        } else {
            f.isSluggish = false;
            f.stamina -= cost;
            f.staminaRegenDelay = f.loadout.has(SkillId::ValeriaEmberDance) ? 0 : 12;
        }
        f.dodgeLock = true;
        f.startMove(move, duration);
        audio |= AUDIO_SFX_DODGE;
        cue(Sfx::Dodge);
    };

    // Esquiva dedicada / por botão (Tecla C, direções ou bits INPUT_DODGE_*)
    if (held & (INPUT_DODGE_L | INPUT_DODGE_R | INPUT_DODGE_B | INPUT_DUCK)) {
        if (!f.dodgeLock && f.status.mud == 0) {
            if (held & INPUT_DODGE_L) startDodge(FIGHTER_DODGE_L, 22);
            else if (held & INPUT_DODGE_R) startDodge(FIGHTER_DODGE_R, 22);
            else if (held & INPUT_DODGE_B) startDodge(FIGHTER_DODGE_BACK, 24);
            else if (held & INPUT_DUCK) startDodge(FIGHTER_DUCK, 22);
            return;
        }
    }

    if (pendingDodgeBack_) {
        pendingDodgeBack_ = false;
        if (f.status.mud == 0) { // na lama não dá para esquivar
            startDodge(FIGHTER_DODGE_BACK, 24);
            return;
        }
    }

    // Esquiva clássica (Baixo + Esquerda / Direita) ou Bloqueio
    if (held & INPUT_DOWN) {
        if (held & (INPUT_LEFT | INPUT_RIGHT)) {
            if (!f.dodgeLock) {
                if (f.status.mud == 0) { // na lama não dá para esquivar
                    startDodge((held & INPUT_LEFT) ? FIGHTER_DODGE_L : FIGHTER_DODGE_R, 22);
                } else {
                    f.startMove(FIGHTER_BLOCK, 14);
                }
            }
        } else {
            f.startMove(FIGHTER_BLOCK, 14);
        }
    } else if (held & INPUT_BLOCK) {
        f.startMove(FIGHTER_BLOCK, 14);
    } else if (held & INPUT_LEFT) {
        f.gloveL.x -= 8.0f; f.gloveL.z += 8.0f;
        f.bodySkew = 0.15f;
    } else if (held & INPUT_RIGHT) {
        f.gloveR.x += 8.0f; f.gloveR.z += 8.0f;
        f.bodySkew = -0.15f;
    } else if (held & INPUT_UP) {
        f.gloveL.y += 8.0f; f.gloveR.y += 8.0f;
    }
}

void GameEngine::playerHandleAction(uint32_t held, uint32_t& audio, bool allowHits) {
    Fighter& f = player;
    f.timer--;
    const float p = 1.0f - static_cast<float>(f.timer) / std::max(1, f.maxTimer);
    const float swing = std::sin(p * kPi);

    PartRenderData& other = f.isLeftPunch ? f.gloveR : f.gloveL;
    const float sign = f.isLeftPunch ? -1.0f : 1.0f;
    other.x = -sign * (f.charId == 1 ? 20.0f : 14.0f);
    other.y = f.guardGloveY();
    other.z = 10.0f;

    auto dodgeSmoke = [&](float x) {
        if (p < 0.3f && !f.spawnedDodgeSmoke) {
            f.spawnedDodgeSmoke = true;
            Vec3 at = Physics3D::localToWorld(x, 5.0f, 0.0f, f.worldPos, f.facing);
            particles.spawnImpact(at.x, at.y, at.z, 12, 0xddddddff);
        }
    };

    switch (f.state) {
    case FIGHTER_DODGE_L:
    case FIGHTER_DODGE_R: {
        const float dir = (f.state == FIGHTER_DODGE_L) ? -1.0f : 1.0f;
        const float turnDir = (f.state == FIGHTER_DODGE_L) ? 1.0f : -1.0f;
        f.offsetX = dir * 54.0f * swing;
        f.offsetY = 18.0f * swing;
        f.bodySkew = -dir * 0.28f * swing;

        const float prevP = 1.0f - static_cast<float>(f.timer + 1) / std::max(1, f.maxTimer);
        const float stepFrac = (p * p * (3.0f - 2.0f * p)) - (prevP * prevP * (3.0f - 2.0f * prevP));
        ringAngle += turnDir * (kPi * 0.5f) * stepFrac;
        dodgeSmoke(f.offsetX);
        break;
    }
    case FIGHTER_DODGE_BACK:
        f.offsetY = 16.0f * swing;
        f.bodySkew = 0.08f * swing;
        f.radius = std::min(215.0f, f.radius + 14.0f * swing);
        dodgeSmoke(0.0f);
        break;
    case FIGHTER_DUCK:
        f.offsetY = -30.0f * swing;
        break;
    case FIGHTER_BLOCK: {
        if ((held & (INPUT_DODGE_KEY | INPUT_DODGE_L | INPUT_DODGE_R | INPUT_DODGE_B | INPUT_DUCK)) || bufferedDodge_ != 0) {
            f.state = FIGHTER_IDLE;
            f.resetPose();
            f.dodgeLock = false;
            playerMovementAndBlock(held, audio);
            return;
        }
        float blockY = f.charId == 1 ? 55.0f : 85.0f;
        f.gloveL.x = -24.0f; f.gloveL.y = blockY; f.gloveL.z = 18.0f;
        f.gloveR.x = 24.0f;  f.gloveR.y = blockY; f.gloveR.z = 18.0f;
        if (f.parryWindowTimer > 0) f.parryWindowTimer--;
        if (held & (INPUT_BLOCK | INPUT_DOWN)) f.timer = 2; // segura a guarda enquanto a tecla estiver apertada
        break;
    }
    case FIGHTER_HIT:
        f.offsetX = (random01() - 0.5f) * 12.0f;
        break;
    default:
        if (isPunchState(f.state)) f.animateCommonPunch(f.state, p, f.isLeftPunch);
        break;
    }

    // o soco dá um passo à frente sozinho se o rival ficou só um pouco além do alcance (empurrado por um golpe, por exemplo): antes o soco errava por poucos pontos
    const float lungeMax = arch(f.charId).lunge;
    if (allowHits && isPunchState(f.state) && p <= 0.40f && !f.hasHit && lungeDone_ < lungeMax && enemy.state != FIGHTER_DODGE_L && enemy.state != FIGHTER_DODGE_R && !isAirborne(enemy)) {
        const float gap = (f.radius + enemy.radius) - moveReach(f.state) + kLungeSlack;
        if (gap > 0.0f) { const float step = std::min({6.0f, gap, lungeMax - lungeDone_, f.radius - 90.0f}); if (step > 0.0f) { f.radius -= step; lungeDone_ += step; } }
    }
    if (allowHits && isPunchState(f.state) && p > 0.40f && p < 0.70f && !f.hasHit) {
        f.hasHit = true;
        checkHitOnEnemy(f.state, f.chargeRatio, audio);
        if (gameState != STATE_FIGHTING) return;
    }

    if (f.timer <= 0) {
        f.state = FIGHTER_IDLE;
        f.resetPose();
        f.isSluggish = false;
        if (bufferedDodge_ != 0 && bufferedDodgeAge_ < kBufferFrames) {
            uint32_t d = bufferedDodge_;
            bufferedDodge_ = 0;
            f.dodgeLock = false;
            playerMovementAndBlock(held | d, audio);
            if (f.state != FIGHTER_IDLE) return;
        }
        if (allowHits && tryBufferedCommand(held, audio)) return;
        if (allowHits && bufferedHand_ >= 0 && bufferedAge_ < kBufferFrames) {
            bool left = bufferedHand_ == 0;
            bufferedHand_ = -1;
            executePunch(left, 0.0f, held);
        }
    }
}

void GameEngine::executePunch(bool left, float charge, uint32_t held) {
    Fighter& f = player;
    FighterActionState move = left ? FIGHTER_JAB : FIGHTER_CROSS;
    if (held & INPUT_UP) move = FIGHTER_UPPER;
    else if (held & INPUT_DOWN) move = FIGHTER_BODY_BLOW;
    else if (held & (INPUT_LEFT | INPUT_RIGHT)) move = FIGHTER_HOOK;

    float baseCost = 22.0f;
    int baseDuration = 30;
    switch (move) {
    case FIGHTER_JAB:       baseCost = 18.0f; baseDuration = 28; break;
    case FIGHTER_CROSS:     baseCost = 22.0f; baseDuration = 32; break;
    case FIGHTER_HOOK:      baseCost = 30.0f; baseDuration = 38; break;
    case FIGHTER_BODY_BLOW: baseCost = 28.0f; baseDuration = 34; break;
    case FIGHTER_UPPER:     baseCost = 40.0f; baseDuration = 48; break;
    default: break;
    }
    float cost = baseCost * (1.0f + charge * 0.5f);
    if (f.status.seasons > 0) cost *= 0.7f; // Quatro Estações
    if (f.loadout.has(SkillId::EspinhaVivaCaatinga)) cost *= 0.7f + 0.3f * (f.hp / f.maxHp); // mais barato com pouca vida
    int duration = static_cast<int>(std::round(baseDuration * (1.0f + charge * 0.5f)));
    if (f.status.seasons > 0) duration = static_cast<int>(std::round(duration * 0.74f)); // +35% de velocidade
    if (f.loadout.has(SkillId::FrogAmphibianRage) && f.stamina < 30.0f) {
        duration = static_cast<int>(std::round(duration * 0.75f)); // Fúria Anfíbia: 25% mais rápido
    }

    // Variar compensa: o soco que carrega um espaço do combo (diferente dos que já estão nele) custa 40% menos fôlego.
    // Repetir o mesmo soco paga o preço cheio.
    const uint32_t chainBit = 1u << ((static_cast<uint32_t>(move) & 15u) * 2u + (left ? 1u : 0u));
    const bool chainFresh = chain_ == 0 || !(chainSeen_ & chainBit);
    if (chainFresh) cost *= kChainPunchCost;

    // Não bloqueia mais socos com baixa estamina / exaustão!
    // Se a estamina for insuficiente, o soco sai como "sluggish" (+85% mais lento e 30% do dano).
    if (f.stamina < cost || f.isExhausted) {
        if (f.loadout.has(SkillId::ValeriaColdMagma) && !f.internalIgnitionUsed) {
            f.internalIgnitionUsed = true;
            f.isExhausted = false;
            f.stamina = 30.0f;
            f.staminaRegenDelay = 0;
            enemy.hp = std::max(0.0f, enemy.hp - 8.0f);
            combat.triggerStatusBanner("¡IGNIÇÃO INTERNA! (+30 STAMINA)");
            Vec3 bPos = frontOf(enemy, 60.0f, 0.0f);
            particles.spawnImpact(bPos.x, bPos.y, bPos.z, 20, 0xff4500ff);
            combat.spawnFloatingText("-8 EXPLOSÃO!", 0xff4500ff, enemy.worldPos.x, enemy.worldPos.y + 90.0f, enemy.worldPos.z, false, 0);
            f.isSluggish = false;
            f.stamina = std::max(0.0f, f.stamina - cost);
            f.staminaRegenDelay = 35;
        } else {
            f.isSluggish = true;
            f.stamina = 0.0f;
            f.staminaRegenDelay = 60;
            duration = static_cast<int>(std::round(duration * 1.85f));
        }
    } else {
        f.isSluggish = false;
        f.stamina -= cost;
        f.staminaRegenDelay = 35;
    }
    if (f.loadout.has(SkillId::JoanaDeepSeaTide) && f.hp <= 40.0f) {
        f.stamina = std::min(f.maxStamina, f.stamina + 5.0f);
    }
    f.isLeftPunch = left;
    f.chargeRatio = charge;
    // combo de entrada: cada soco diferente dos que já estão no combo carrega um espaço (o mesmo golpe com a outra mão já conta como diferente).
    // Vale mesmo sem acertar; repetir um soco não carrega, só mantém o combo quente.
    {
        const uint32_t bit = chainBit;
        if (chain_ == 0) chainSeen_ = 0;
        if (!(chainSeen_ & bit)) { chainSeen_ |= bit; chain_ = std::min(chain_ + 1, 3); }
        chainTimer_ = duration + kChainGap;
    }
    lungeDone_ = 0.0f;
    f.startMove(move, duration);
}

void GameEngine::releaseCharge(uint32_t held) {
    if (!chargeActive) return;
    float ratio = chargeRatio();
    bool left = chargeLeft;
    cancelCharge();
    executePunch(left, ratio, held);
}

void GameEngine::cancelCharge() {
    chargeActive = false;
    chargeFrames = 0;
}

// ---------------------------------------------------------------------------
// Acertos
// ---------------------------------------------------------------------------

float GameEngine::moveReach(FighterActionState move) const {
    float reach = 220.0f;
    switch (move) {
    case FIGHTER_JAB:       reach = 215.0f; break;
    case FIGHTER_CROSS:     reach = 220.0f; break;
    case FIGHTER_HOOK:      reach = 215.0f; break;
    case FIGHTER_BODY_BLOW: reach = 215.0f; break;
    case FIGHTER_UPPER:     reach = 200.0f; break;
    default: break;
    }
    reach += arch(player.charId).reach; // braço de cada estilo
    if (player.isBigChar()) reach += 20.0f; // compensa o porte maior e idleRadius recuado (138 vs 115..125)
    if (enemy.isBigChar())  reach += 15.0f; // oponente grandalhão tem silhueta/hurtbox mais ampla e próxima
    if (enemy.state == FIGHTER_HIT || enemy.vulnerable > 0) reach += 30.0f; // brecha e hitstun: combos conectam com firmeza
    if ((player.charId == 2 || player.loadout.has(SkillId::JoanaAsymmetricArms)) && (player.isLeftPunch || move == FIGHTER_JAB || move == FIGHTER_BODY_BLOW)) reach *= 1.40f; // gancho de marujo
    return reach;
}

void GameEngine::checkHitOnEnemy(FighterActionState move, float charge, uint32_t& audio) {
    const float distance = player.radius + enemy.radius;

    float baseDamage = 8.0f, baseStun = 20.0f;
    switch (move) {
    case FIGHTER_JAB:       baseDamage = 6.0f;  baseStun = 16.0f; break;
    case FIGHTER_CROSS:     baseDamage = 8.0f;  baseStun = 20.0f; break;
    case FIGHTER_HOOK:      baseDamage = 10.0f; baseStun = 24.0f; break;
    case FIGHTER_BODY_BLOW: baseDamage = 10.0f; baseStun = 28.0f; break;
    case FIGHTER_UPPER:     baseDamage = 14.0f; baseStun = 38.0f; break;
    default: break;
    }
    const float reach = moveReach(move);

    const bool enemyAirborne = isAirborne(enemy);
    const bool enemyDodging = (distance > reach || enemy.state == FIGHTER_DODGE_L || enemy.state == FIGHTER_DODGE_R || enemyAirborne);
    if (enemyDodging || attackMisses(player, enemy)) {
        audio |= AUDIO_SFX_DODGE; // soco no ar
        cue(Sfx::Dodge);
        if (enemy.state == FIGHTER_DODGE_L || enemy.state == FIGHTER_DODGE_R) {
            cue(Sfx::CrowdWoah); // a plateia reage à esquiva do oponente
            dodgeReward(enemy, player);
        }
        if (player.isSluggish) player.vulnerable = 28;
        if (enemyDodging && enemy.loadout.has(SkillId::JackBeekeeper)) {
            player.hp = std::max(0.0f, player.hp - 3.0f);
            Vec3 at = frontOf(player, 50.0f);
            particles.spawnImpact(at.x, at.y + 40.0f, at.z, 15, 0xffd700ff);
            combat.spawnFloatingText("-3 ABELHA!", 0xffd700ff, player.worldPos.x, player.worldPos.y + 90.0f, player.worldPos.z, false, 0);
        }
        return;
    }

    if (enemy.state == FIGHTER_BLOCK) {
        audio |= AUDIO_SFX_BLOCK;
        cue(Sfx::Block);
        screenShake = std::max(screenShake, 5.0f);
        if (player.isSluggish) {
            // Golpe desferido em exaustão é defletido pela guarda: gera brecha e atrasa recuperação
            player.vulnerable = 25;
            player.staminaRegenDelay = 75;
            combat.spawnFloatingText("DEFLETIDO!", 0xf43f5eff, player.worldPos.x, player.worldPos.y + 85.0f, player.worldPos.z, true, 0);
        } else {
            float blockChip = 2.0f;
            if (enemy.loadout.has(SkillId::CremosoCreamCushion)) {
                blockChip *= 0.5f;
                enemy.stamina = std::min(enemy.maxStamina, enemy.stamina + 4.0f);
            }
            enemy.hp = std::max(0.0f, enemy.hp - blockChip);
        }
        Vec3 at = frontOf(enemy, 70.0f);
        particles.spawnImpact(at.x, at.y, at.z, 5, 0xccccccff);
        if (enemy.hp <= 0.0f) triggerKnockdown(true, audio);
        return;
    }

    float damage = std::round(baseDamage * (1.0f + charge * 2.0f));
    int stun = static_cast<int>(std::round(baseStun * (1.0f + charge * 0.8f)));

    if (player.isSluggish) {
        damage = std::max(1.0f, std::round(damage * 0.30f)); // Dano reduzido por exaustão (30% do normal)
        stun = std::max(6, static_cast<int>(stun * 0.40f));
    }

    const bool isCounter = enemy.state == FIGHTER_TELEGRAPH;
    if (enemy.vulnerable > 0) { damage = std::round(damage * 1.25f); stun = static_cast<int>(stun * 1.3f); } // brecha
    if (isCounter) {
        float counterMult = (player.loadout.has(SkillId::EspinhaRebelSpirit) ? 1.85f : (move == FIGHTER_UPPER ? 1.35f : 1.60f));
        damage = std::round(damage * counterMult);
        stun += 12;
        audio |= AUDIO_SFX_COUNTER_HIT;
    }

    if (move == FIGHTER_UPPER) {
        if (!trainingMode && enemy.charId != 9) enemy.radius = std::min(235.0f, enemy.radius + 40.0f);
        else enemy.radius = 50.0f;
        enemy.staminaRegenDelay = 60;
        hitstopTimer = 4;
    } else if (move == FIGHTER_BODY_BLOW) {
        if (!player.isSluggish) {
            enemy.stamina = std::max(0.0f, enemy.stamina - 20.0f);
            enemy.staminaRegenDelay = 45;
        } else {
            // Golpe em exaustão não tem pressão suficiente para quebrar o fôlego do adversário
            enemy.staminaRegenDelay = 15;
        }
        hitstopTimer = 3;
    }

    const bool isHeavy = charge > 0.5f || move == FIGHTER_UPPER || isCounter;
    {
        // Peso do impacto / Hitstop Dinâmico: congelamento cirúrgico de quadros proporcional ao peso, carga e counter
        int stop = move == FIGHTER_JAB ? 3 : (move == FIGHTER_BODY_BLOW ? 5 : (move == FIGHTER_UPPER ? 9 : 6));
        if (charge > 0.3f) stop += static_cast<int>(std::round(charge * 5.0f)); // até +5 frames em carga cheia
        if (isCounter) stop = std::max(stop, 12); // counter hit pesado congela 12 frames
        const bool open = enemy.vulnerable > 0;
        if (open) stop += 3;
        hitstopTimer = std::max(hitstopTimer, stop);
        if (!trainingMode && enemy.charId != 9) {
            if (isHeavy || move == FIGHTER_HOOK || move == FIGHTER_CROSS) {
                enemy.radius = std::min(235.0f, enemy.radius + (isHeavy ? 16.0f : 8.0f) * arch(player.charId).pushMul);
                if (enemy.radius >= 225.0f) {
                    cue(Sfx::CrowdGasp); // mandou o oponente para as cordas! Suspiro tenso ("Ooooh!")
                }
            }
        } else {
            enemy.radius = 50.0f;
        }
        impactKind = isCounter ? 2 : (open ? 3 : ((move == FIGHTER_UPPER || charge > 0.6f) ? 2 : (isHeavy || move == FIGHTER_HOOK || move == FIGHTER_CROSS ? 1 : 0)));
        impactTimer = impactKind == 0 ? 16 : 28;
        impactSeed = static_cast<int>(frameIndex);
        impactAt = frontOf(enemy, move == FIGHTER_BODY_BLOW ? 45.0f : 85.0f);
    }
    audio |= isHeavy ? AUDIO_SFX_PUNCH_HEAVY : AUDIO_SFX_PUNCH_LIGHT;
    if (charge > 0.7f) audio |= AUDIO_SFX_PUNCH_CRIT;
    cue(isHeavy ? Sfx::PunchHeavy : Sfx::PunchLight, static_cast<int>(charge * 10.0f));
    if (isCounter || move == FIGHTER_UPPER || charge > 0.6f || (enemy.vulnerable > 0 && isHeavy)) {
        cue(Sfx::Crunch); // CRACK brusco de ossada/maxilar
        cue(Sfx::CrowdImpact); // ovação potente da torcida com o impacto
    }
    if (isHeavy) cue(Sfx::CrowdCheer); // a torcida vibra com golpe pesado
    screenShake = std::max(screenShake, isHeavy ? 24.0f : 8.0f);

    Vec3 at = frontOf(enemy, move == FIGHTER_BODY_BLOW ? 45.0f : 85.0f);
    if (enemy.isDummy()) {
        particles.spawnStraw(at.x, at.y + 10.0f, at.z, isHeavy ? 26 : 14);
    } else {
        uint32_t sparkColor = charge > 0.7f ? 0xff0055ff : (isCounter ? 0x00e5ffff : 0xffd700ff);
        particles.spawnImpact(at.x, at.y, at.z, isHeavy ? 25 : 12, sparkColor);
        if (isHeavy) particles.spawnBlood(at.x, at.y + 10.0f, at.z, 10, 0x8a0f1fff);
    }

    damage = resolveDamage(player, enemy, damage, move, charge > 0.5f || move == FIGHTER_UPPER, move == FIGHTER_JAB);
    if (arch(player.charId).punchPush > 0.0f && !trainingMode && enemy.charId != 9 && !enemy.loadout.has(SkillId::MarcovZeroGravity))
        enemy.radius = std::min(235.0f, enemy.radius + arch(player.charId).punchPush); // zoneadora: todo soco limpo afasta o rival
    if (charge > 0.5f && player.loadout.has(SkillId::MarcovThrusterCharge)) player.stamina = std::min(player.maxStamina, player.stamina + 20.0f);
    if (player.charId == 7) player.stamina = std::min(player.maxStamina, player.stamina + 3.0f); // Marcov: recarga cinética
    if (move == FIGHTER_UPPER && enemy.loadout.has(SkillId::MarcovZeroGravity)) enemy.radius -= 40.0f; // imune a empurrão
    if (trainingMode || enemy.charId == 9) {
        enemy.hp = std::max(1.0f, enemy.hp - damage);
    } else {
        enemy.hp = std::max(0.0f, enemy.hp - damage);
    }

    uint32_t textColor = isCounter ? 0x00f0ffff : (charge > 0.7f ? 0xff1e42ff : (isHeavy ? 0xff6600ff : 0xffde00ff));
    combat.spawnFloatingText("-" + std::to_string(static_cast<int>(damage)), textColor, enemy.worldPos.x,
                             enemy.worldPos.y + 70.0f, enemy.worldPos.z, isHeavy || isCounter, player.isLeftPunch ? 1 : 0);
    if (enemy.vulnerable > 0) {
        combat.spawnFloatingText("PUNISH!", 0x00f5d4ff, enemy.worldPos.x, enemy.worldPos.y + 95.0f, enemy.worldPos.z, true, player.isLeftPunch ? 1 : 0);
    } else if (isCounter) {
        combat.spawnFloatingText("COUNTER!", 0x00e5ffff, enemy.worldPos.x, enemy.worldPos.y + 95.0f, enemy.worldPos.z, true, player.isLeftPunch ? 1 : 0);
    }
    registerComboHit();

    const int kind = move == FIGHTER_BODY_BLOW ? 3 : (move == FIGHTER_UPPER ? 4 : ((move == FIGHTER_HOOK || move == FIGHTER_CROSS || isHeavy) ? 2 : 1));
    if (trainingMode || enemy.charId == 9) {
        enemy.startMove(FIGHTER_HIT, stun);
        enemy.hitKind = kind;
        enemy.hitDir = player.isLeftPunch ? 1 : -1;
        enemy.telegraphFlash = false;
    } else if (enemy.hp <= 0.0f) {
        enemy.hitKind = kind;
        enemy.hitDir = player.isLeftPunch ? 1 : -1;
        triggerKnockdown(true, audio);
    } else {
        enemy.startMove(FIGHTER_HIT, stun);
        enemy.hitKind = kind;
        enemy.hitDir = player.isLeftPunch ? 1 : -1;
        enemy.telegraphFlash = false;
    }
}

void GameEngine::checkHitOnPlayer(bool heavy, uint32_t& audio) {
    float maxReach = 210.0f + arch(enemy.charId).reach;
    if (player.isBigChar() || enemy.isBigChar()) maxReach += 25.0f;
    if (player.radius + enemy.radius > maxReach) return;

    bool playerDodging = player.state == FIGHTER_DODGE_L || player.state == FIGHTER_DODGE_R ||
                         player.state == FIGHTER_DODGE_BACK || player.state == FIGHTER_DUCK;
    // Resposta certa: gancho se esquiva para o lado oposto ao da luva que vem; uppercut só abaixando
    // ou recuando (esquivar para o lado não tira a cabeça do caminho).
    {
        const FighterActionState m = enemy.queuedAttack;
        const bool fromScreenRight = enemy.isLeftPunch; // a luva esquerda do oponente vem pela direita da tela
        if (m == FIGHTER_HOOK && ((fromScreenRight && player.state == FIGHTER_DODGE_R) || (!fromScreenRight && player.state == FIGHTER_DODGE_L)))
            playerDodging = false;
        if (m == FIGHTER_UPPER && (player.state == FIGHTER_DODGE_L || player.state == FIGHTER_DODGE_R)) playerDodging = false;
    }
    if (playerDodging || isAirborne(player) || attackMisses(enemy, player)) {
        if (playerDodging) flowWindow_ = kFlowWindow;   // esquiva bem feita abre os especiais de fluxo
        audio |= AUDIO_SFX_DODGE;
        cue(Sfx::Dodge);
        if (playerDodging) {
            cue(Sfx::CrowdWoah);
            enemy.vulnerable = std::max(enemy.vulnerable, heavy ? 28 : 20);
            combat.spawnFloatingText("BRECHA!", 0x00f0ffff, enemy.worldPos.x, enemy.worldPos.y + 110.0f, enemy.worldPos.z, true, 0);
        }
        if (playerDodging) dodgeReward(player, enemy);
        // O Cremoso: Vingança Doce — esquiva perfeita cura 2 e carrega +15% no próximo soco.
        if (playerDodging && (player.charId == 3 || player.loadout.has(SkillId::CremosoSweetRevenge))) {
            heal(player, 2.0f);
            player.revengeBonus = 1.15f;
        }
        // Jack Mãos de Colmeia: Apicultor — esquivas soltam abelha que causa dano residual
        if (playerDodging && player.loadout.has(SkillId::JackBeekeeper)) {
            enemy.hp = std::max(0.0f, enemy.hp - 3.0f);
            Vec3 at = frontOf(enemy, 50.0f);
            particles.spawnImpact(at.x, at.y + 40.0f, at.z, 15, 0xffd700ff);
            combat.spawnFloatingText("-3 ABELHA!", 0xffd700ff, enemy.worldPos.x, enemy.worldPos.y + 90.0f, enemy.worldPos.z, false, 0);
        }
        return;
    }

    if (player.state == FIGHTER_BLOCK) {
        if (player.parryWindowTimer > 0) {
            // PARRY PERFEITO (aparada nos primeiros 10 quadros de guarda = 166ms)
            audio |= AUDIO_SFX_COUNTER_HIT;
            cue(Sfx::Parry);
            cue(Sfx::CrowdCheer);
            screenShake = std::max(screenShake, 18.0f);
            hitstopTimer = 12; // Congelamento dramático de quadros
            impactKind = 2;   // Dispara Shockwave Ripple e Aberração Cromática no shader GLSL
            impactTimer = 30;
            impactSeed = static_cast<int>(frameIndex);
            impactAt = frontOf(player, 65.0f);

            // Recompensa de estamina e quebra da exaustão
            player.stamina = std::min(player.maxStamina, player.stamina + 20.0f);
            player.staminaRegenDelay = 0;
            player.isExhausted = false;

            // Desequilibra o oponente com vantagem de quadros e brecha aberta
            enemy.startMove(FIGHTER_HIT, 24);
            enemy.hitKind = 0;
            enemy.vulnerable = 30; // Brecha de 30 frames para punição imediata

            combat.spawnFloatingText("PARRY!", 0x00f5d4ff, player.worldPos.x, player.worldPos.y + 75.0f, player.worldPos.z, true, 0);
            Vec3 at = frontOf(player, 65.0f);
            particles.spawnImpact(at.x, at.y, at.z, 28, 0x00f5d4ff);
            return;
        }

        audio |= AUDIO_SFX_BLOCK;
        cue(Sfx::Block);
        screenShake = std::max(screenShake, 6.0f);
        float blockChip = 3.0f;
        if (player.loadout.has(SkillId::CremosoCreamCushion)) {
            blockChip *= 0.5f;
            player.stamina = std::min(player.maxStamina, player.stamina + 4.0f);
        }
        player.hp = std::max(0.0f, player.hp - blockChip);
        player.startMove(FIGHTER_HIT, 8);
        player.hitKind = 0;
        player.staminaRegenDelay = 40;
        if (player.loadout.has(SkillId::JackHoneySkin)) {
            player.stamina = std::min(player.maxStamina, player.stamina + 4.0f);
        }
        if (player.hp <= 0.0f) triggerKnockdown(false, audio);
        return;
    }

    // Golpes pesados (cross, gancho e uppercut) causam 14 de dano base e cortam stamina máxima; jabs e leves causam 8.
    float damage = heavy ? 14.0f : 8.0f;
    damage = resolveDamage(enemy, player, damage, enemy.queuedAttack, heavy, !heavy);

    audio |= AUDIO_SFX_PUNCH_HEAVY;
    cue(Sfx::PunchHeavy, 6); // sfx.playPunch(true, 0.6)
    Vec3 at = frontOf(player, 70.0f);
    const bool heavyImpact = heavy || enemy.queuedAttack == FIGHTER_UPPER;
    if (heavyImpact) {
        cue(Sfx::Crunch); // CRACK brusco ao sofrer golpe pesado
        cue(Sfx::CrowdImpact); // A plateia reage com choque/rugido ao golpe pesado que pegou no jogador!
        cue(Sfx::CrowdGasp);   // Choque e apreensão coletiva da arena ("Ooooh!")
        impactKind = 2;
        impactTimer = 28;
        impactSeed = static_cast<int>(frameIndex);
        impactAt = at;
    } else {
        cue(Sfx::CrowdGasp); // Soco limpo do oponente faz a plateia reagir imediatamente!
    }
    if (heavyImpact || enemy.queuedAttack == FIGHTER_HOOK || enemy.queuedAttack == FIGHTER_CROSS) {
        const float push = (heavy ? 20.0f : 12.0f) * arch(enemy.charId).pushMul;
        player.radius = std::min(235.0f, player.radius + push);
        if (player.radius >= 170.0f || push >= 15.0f) {
            cue(Sfx::CrowdGasp); // Mandou o jogador em direção às cordas! Suspiro tenso da arena ("Ooooh!")
        }
    }
    if (arch(enemy.charId).punchPush > 0.0f) player.radius = std::min(235.0f, player.radius + arch(enemy.charId).punchPush); // zoneadora: todo soco limpo afasta
    screenShake = std::max(screenShake, 25.0f);
    // Hitstop escalonado: 8 frames para golpes normais, 12 para pesados
    hitstopTimer = std::max(hitstopTimer, heavy ? 11 : 6);
    hurtTimer = heavy ? 26 : 18;
    particles.spawnImpact(at.x, at.y, at.z, 18, 0xff3333ff);
    particles.spawnBlood(at.x, at.y + 15.0f, at.z, heavy ? 12 : 6, 0x8a0f1fff);

    player.hp = std::max(0.0f, player.hp - damage);
    combat.spawnFloatingText("-" + std::to_string(static_cast<int>(damage)) + " HP", 0xff1e42ff, player.worldPos.x,
                             player.worldPos.y + 65.0f, player.worldPos.z, heavy, enemy.isLeftPunch ? 1 : 0);
    resetCombo();
    if (enemy.loadout.has(SkillId::BossQuantum)) heal(enemy, 4.0f); // Matriz Quântica: cada acerto recupera 4

    player.startMove(FIGHTER_HIT, 22);
    {
        const FighterActionState m = enemy.queuedAttack;
        player.hitKind = m == FIGHTER_BODY_BLOW ? 3 : (m == FIGHTER_UPPER ? 4 : (m == FIGHTER_JAB ? 1 : 2));
        player.hitDir = enemy.isLeftPunch ? -1 : 1;
    }
    if (player.status.honeyShield == 0) {
        player.stamina = std::max(0.0f, player.stamina - 25.0f);
    }
    if (heavy && !player.loadout.has(SkillId::CremosoFrozenStamina)) {
        player.maxStamina = std::max(30.0f, player.maxStamina - 8.0f);
        player.stamina = std::min(player.stamina, player.maxStamina);
    }
    cancelCharge();

    if (player.hp <= 0.0f) triggerKnockdown(false, audio);
}

void GameEngine::registerComboHit() {
    comboCount++;
    cue(Sfx::ComboHit, comboCount);
    if (comboCount % 5 == 0 && player.loadout.has(SkillId::VivaldiSymphonicResonance)) {
        player.stamina = std::min(player.maxStamina, player.stamina + 25.0f);
    }
    comboTimer = kComboFrames;
    bestCombo = std::max(bestCombo, comboCount);
}

void GameEngine::resetCombo() {
    comboCount = 0;
    comboTimer = 0;
}

// ---------------------------------------------------------------------------
// Quedas, juiz, rounds e fim de luta
// ---------------------------------------------------------------------------

void GameEngine::triggerKnockdown(bool enemyDown, uint32_t& audio) {
    Fighter& falling = enemyDown ? enemy : player;
    if (falling.loadout.has(SkillId::MarcovZeroGravity) && !falling.zeroGravityUsed) {
        // Gravidade Zero: flutua e escapa da primeira queda do round.
        falling.zeroGravityUsed = true;
        falling.hp = 1.0f;
        combat.triggerStatusBanner(falling.isPlayer ? "¡GRAVIDADE ZERO! (QUEDA EVITADA)" : "¡DR. MARCOV: GRAVIDADE ZERO!");
        return;
    }
    Fighter& standing = enemyDown ? player : enemy;
    koBySkill = standing.state == FIGHTER_SKILL;
    falling.signatureOnly = false; // queda interrompe a assinatura antes da reação nas cordas
    hitstopTimer = 0; // a queda já para a luta (a pausa do soco não pode sobrar para a volta)
    impactKind = 4; impactTimer = 40; impactSeed = static_cast<int>(frameIndex);
    impactAt = frontOf(falling, 85.0f);
    // Limpa exaustão, status e olho tonto de quem ficou de pé para a comemoração/pose triunfante
    standing.status = StatusEffects{};
    standing.isExhausted = false;
    standing.staminaRegenDelay = 0;
    standing.stamina = std::max(standing.stamina, 50.0f);
    standing.isSluggish = false;
    standing.hitKind = 0;
    if (standing.loadout.has(SkillId::VivaldiSymphonicResonance)) {
        standing.stamina = std::min(standing.maxStamina, standing.stamina + 25.0f);
    }
    gameState = STATE_KNOCKDOWN;
    enemyIsDown = enemyDown;
    koRopeFrame = 0;
    cue(Sfx::Bell);
    cue(enemyDown ? Sfx::CrowdCheer : Sfx::CrowdBoo); // a torcida reage à queda
    cue(Sfx::CrowdImpact); // o estrondo da queda no tablado (seja oponente ou jogador)
    if (!enemyDown) cue(Sfx::CrowdGasp); // o jogador caiu! Choque coletivo da arena
    refereeCount = 0;
    refereeTimer = 60;
    audio |= AUDIO_SFX_KNOCKDOWN | AUDIO_SFX_GONG;
    cancelCharge();
    bufferedHand_ = -1;

    if (enemyDown) {
        enemy.state = FIGHTER_KNOCKDOWN;
        enemy.telegraphFlash = false;
        enemyKnockdowns++;
        if (player.loadout.has(SkillId::JackQueenBlessing)) {
            heal(player, 16.0f);
            combat.triggerStatusBanner("¡BÊNÇÃO DA RAINHA! (+16 HP NO KD)");
        }
        if (enemyKnockdowns >= kKnockdownsForTko) {
            endMatch(MatchResult::WinTKO, audio);
        } else if (enemy.bossStage > 0 && enemy.bossStage < 3) {
            // Batalhator: cada queda o desmonta na forma seguinte (robô danificado, depois O Nerd Operador).
            enemy.bossStage++;
            enemy.loadout = SkillSystem::bossLoadout(enemy.bossStage);
            enemy.status = StatusEffects{};
            const Vec3 at = frontOf(enemy, 70.0f, 0.0f);
            particles.spawnImpact(at.x, at.y, at.z, 45, 0x94a3b8ff);
            particles.spawnImpact(at.x, at.y + 20.0f, at.z, 30, enemy.bossStage == 3 ? 0x00f5d4ff : 0xffbe0bff);
            screenShake = std::max(screenShake, 30.0f);
            cue(Sfx::Zap);
            combat.triggerStatusBanner(enemy.bossStage == 2 ? "¡BATALHATOR DANIFICADO: MODO FÚRIA!" : "¡FASE FINAL: O NERD OPERADOR!");
        } else {
            combat.triggerStatusBanner("¡KNOCKDOWN!");
        }
    } else {
        player.state = FIGHTER_KNOCKDOWN;
        playerKnockdowns++;
        if (enemy.loadout.has(SkillId::JackQueenBlessing)) heal(enemy, 16.0f);
        if (playerKnockdowns >= kKnockdownsForTko) {
            endMatch(MatchResult::LossTKO, audio);
        } else {
            mashProgress = 0;
            mashTarget = playerKnockdowns * 14;
        }
    }
}

void GameEngine::updateKnockdown(uint32_t pressed, uint32_t& audio) {
    Fighter& downed = enemyIsDown ? enemy : player;
    downed.resetPose();
    if (downed.radius < 232.0f) downed.radius += 2.5f;
    if (downed.radius >= 231.0f && koRopeFrame == 0) {
        koRopeFrame = frameIndex;
        cue(Sfx::CrowdGasp); // o lutador caído escorrega nas cordas
    }
    if (enemyIsDown) {
        // Cambaleia tonto no chão.
        float t = frameIndex / 6.6f;
        enemy.bodySkew = std::sin(t) * 0.2f;
        enemy.offsetY = -std::fabs(std::cos(t * 0.7f)) * 6.0f;
        enemy.offsetX = std::sin(t * 0.5f) * 8.0f;
        enemy.gloveL.y = enemy.gloveR.y = 75.0f;
        // O jogador termina o golpe que estava dando, sem acertar de novo.
        player.resetParts();
        if (player.state == FIGHTER_SKILL) updateSkillMove(player, enemy, audio, false);
        else if (player.state != FIGHTER_IDLE) playerHandleAction(0, audio, false);
        else player.resetPose();
    } else {
        if (enemy.state == FIGHTER_SKILL) updateSkillMove(enemy, player, audio, false);
        else ai.finishAction(enemy);
        if (pressed & (INPUT_PUNCH_L | INPUT_PUNCH_R | INPUT_BLOCK)) {
            mashProgress++; // MASH Z / X para levantar
            audio |= AUDIO_SFX_DODGE;
            cue(Sfx::Dodge);
        }
    }
    handleReferee(pressed, audio);
}

void GameEngine::handleReferee(uint32_t pressed, uint32_t& audio) {
    (void)pressed;
    refereeTimer--;

    // Quem está de pé recua e recupera fôlego enquanto o juiz conta.
    if (enemyIsDown) {
        if (player.radius < 180.0f) player.radius += 1.2f;
        player.stamina = std::min(player.maxStamina, player.stamina + 0.5f);
    } else {
        if (enemy.radius > 60.0f && enemy.radius < 180.0f) enemy.radius += 1.2f;
        enemy.stamina = std::min(enemy.maxStamina, enemy.stamina + 0.5f);
    }

    // O Batalhator se remonta fazendo barulho de oficina.
    if (enemyIsDown && enemy.bossStage > 0 && (refereeTimer == 30 || refereeTimer == 58)) cue(Sfx::Rebuild);

    if (refereeTimer <= 0) {
        refereeCount++;
        refereeTimer = 60;
        cue(Sfx::RefereeCount, refereeCount);
        cue(Sfx::CrowdCount, refereeCount);
        if (refereeCount >= 10) {
            endMatch(enemyIsDown ? MatchResult::WinKO : MatchResult::LossKO, audio);
            return;
        }
        if (enemyIsDown) {
            int getUpAfter = enemyKnockdowns * 3 + std::rand() % 2;
            if (refereeCount > getUpAfter) {
                recover(true);
                return;
            }
        }
    }

    if (!enemyIsDown && mashProgress >= mashTarget) recover(false);
}

void GameEngine::recover(bool enemyRecovers) {
    gameState = STATE_FIGHTING;
    koRopeFrame = 0;
    // Valéria: Magma Frio ao levantar da 2ª queda (o cabelo vira basalto).
    Fighter& rising = enemyRecovers ? enemy : player;
    const int falls = enemyRecovers ? enemyKnockdowns : playerKnockdowns;
    if (falls >= 2 && rising.loadout.has(SkillId::ValeriaColdMagma) && !rising.coldMagma) {
        rising.coldMagma = true;
        combat.triggerStatusBanner(rising.isPlayer ? "¡MODO MAGMA FRIO! (+50% RESISTÊNCIA)" : "¡VALÉRIA: MAGMA FRIO!");
    }
    if (enemyRecovers) {
        // O Batalhator se remonta com meia vida (no JS volta com 40 de 200, e as fases 2 e 3 acabavam rápido demais).
        enemy.hp = enemy.bossStage > 0 ? enemy.maxHp * 0.5f : 40.0f;
        enemy.actionTimer = 55;
    } else {
        player.hp = 30.0f + (100.0f / playerKnockdowns) * 0.25f;
        player.stamina = player.maxStamina;
        player.isExhausted = false;
    }
    player.state = enemy.state = FIGHTER_IDLE;
    player.resetPose();
    enemy.resetPose();
}

void GameEngine::startNextRound(uint32_t& audio) {
    if (round >= kRounds) {
        if (player.hp > enemy.hp) endMatch(MatchResult::WinDecision, audio);
        else if (player.hp < enemy.hp) endMatch(MatchResult::LossDecision, audio);
        else endMatch(MatchResult::Draw, audio);
        return;
    }
    round++;
    roundFrames = kRoundFrames;
    player.hp = std::min(player.maxHp, player.hp + 25.0f);
    enemy.hp = std::min(enemy.maxHp, enemy.hp + 25.0f);
    playerKnockdowns = enemyKnockdowns = 0;
    player.zeroGravityUsed = enemy.zeroGravityUsed = false;
    player.internalIgnitionUsed = enemy.internalIgnitionUsed = false;
    player.state = enemy.state = FIGHTER_IDLE;
    player.resetPose();
    enemy.resetPose();
    player.radius = player.idleRadius();
    enemy.radius = enemy.idleRadius();
    enemy.actionTimer = 70;
    gameState = STATE_FIGHTING;
    audio |= AUDIO_SFX_GONG;
    cue(Sfx::Bell);
    combat.triggerStatusBanner("ROUND " + std::to_string(round) + "!");
}

void GameEngine::endMatch(MatchResult outcome, uint32_t& audio) {
    result = outcome;
    matchEndFrame = frameIndex;
    koRopeFrame = 0;
    const bool won = outcome == MatchResult::WinKO || outcome == MatchResult::WinTKO || outcome == MatchResult::WinDecision;
    gameState = won ? STATE_VICTORY : STATE_GAME_OVER;
    cancelCharge();
    audio |= AUDIO_SFX_GONG;
    cue(Sfx::Impact);
    if (won) {
        cue(Sfx::CrowdVictory); // celebração apoteótica da arena com palmas e ovação
        cue(Sfx::CameraFlash);  // flashes e disparos dos repórteres no ringue
        if (isPerfectVictory()) {
            cue(Sfx::CrowdCheer);
            cue(Sfx::CameraFlash);
        }
    } else {
        cue(Sfx::CrowdBoo);
    }
    combat.statusBanner.clear();
    combat.statusBannerTimer = 0.0f;

    Fighter& winner = won ? player : enemy;
    Fighter& loser = won ? enemy : player;
    loser.signatureOnly = false; // vale também para derrota por pontos
    winner.status = StatusEffects{};
    winner.isExhausted = false;
    winner.staminaRegenDelay = 0;
    winner.stamina = winner.maxStamina;
    winner.isSluggish = false;
    winner.hitKind = 0;

    if (outcome == MatchResult::WinKO || outcome == MatchResult::WinTKO) {
        enemy.state = FIGHTER_KNOCKDOWN;
        player.state = FIGHTER_VICTORY_POSE;
        player.resetPose();
    } else if (outcome == MatchResult::LossKO || outcome == MatchResult::LossTKO) {
        player.state = FIGHTER_KNOCKDOWN;
        enemy.state = FIGHTER_VICTORY_POSE;
        enemy.resetPose();
    } else {
        if (won) {
            player.state = FIGHTER_VICTORY_POSE;
            enemy.state = FIGHTER_IDLE;
        } else {
            enemy.state = FIGHTER_VICTORY_POSE;
            player.state = FIGHTER_IDLE;
        }
        player.resetPose();
        enemy.resetPose();
    }
}

// ---------------------------------------------------------------------------
// Habilidades
// ---------------------------------------------------------------------------

namespace {

const char* const kBannerNames[] = {"JACK", "KING FROG", "JOANA", "O CREMOSO", "VALÉRIA", "VIVALDI", "LADY ESPINHA",
                                    "DR. MARCOV", "BATALHATOR"};

// Maiúsculas em UTF-8 para ASCII e para as letras acentuadas do português (à–þ → À–Þ).
std::string utf8Upper(const std::string& s) {
    std::string out = s;
    for (size_t i = 0; i < out.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(out[i]);
        if (c >= 'a' && c <= 'z') out[i] = static_cast<char>(c - 32);
        else if (c == 0xC3 && i + 1 < out.size()) {
            unsigned char n = static_cast<unsigned char>(out[i + 1]);
            if (n >= 0xA0 && n <= 0xBE && n != 0xB7) out[i + 1] = static_cast<char>(n - 0x20);
            ++i;
        }
    }
    return out;
}

// Ordem em que o JS testa os comandos de cada lutador (o primeiro que casar vence).
const std::vector<SkillId>& commandOrder(uint32_t charId) {
    static const std::vector<SkillId> jack = {SkillId::JackMolasses, SkillId::JackInfestation, SkillId::JackHoneyWall,
                                              SkillId::JackStingJab, SkillId::JackRoyalJelly, SkillId::JackCombBurst};
    static const std::vector<SkillId> frog = {SkillId::FrogTongue, SkillId::FrogPoisonSkin, SkillId::FrogRoyalLeap,
                                              SkillId::FrogMudSlime, SkillId::FrogFlyCatcher, SkillId::FrogSwampSpout};
    static const std::vector<SkillId> joana = {SkillId::JoanaClawCrush, SkillId::JoanaHookPull, SkillId::JoanaCrustaceanShell,
                                               SkillId::JoanaCoralSpout, SkillId::JoanaAnchorSlam, SkillId::JoanaOceanBounty};
    static const std::vector<SkillId> none;
    if (charId == 0) return jack;
    if (charId == 1) return frog;
    if (charId == 2) return joana;
    // Demais lutadores: a ordem da tabela (→→X, ↓↑X, Z+X, ↑↑X, ↓↓Z, ←←Z), igual à do JS.
    static std::vector<std::vector<SkillId>> others(8);
    if (charId < others.size()) {
        if (others[charId].empty()) {
            for (const SkillDef& s : SkillSystem::all()) {
                if (s.charId == charId && s.type != SkillType::Passive) others[charId].push_back(s.id);
            }
        }
        return others[charId];
    }
    return none;
}

// Som de cada habilidade (o sfx.playX chamado no activateX do JS). Modos e curas, mudos no JS,
// ganham o zumbido do escudo de plasma para o jogador sentir que o comando saiu.
Sfx skillSound(SkillId id) {
    switch (id) {
    case SkillId::JackInfestation: case SkillId::FrogRoyalLeap: case SkillId::ValeriaAshCloud:
    case SkillId::VivaldiAutumnRequiem: case SkillId::EspinhaDesertWind: case SkillId::MarcovExhaustCloud:
        return Sfx::Dodge;
    case SkillId::JackCombBurst: case SkillId::FrogPoisonSkin: return Sfx::Zap;
    case SkillId::JackStingJab: case SkillId::JoanaCoralSpout: case SkillId::ValeriaLavaKickJab:
    case SkillId::MarcovKineticOverdrive:
        return Sfx::Impact;
    case SkillId::FrogTongue: return Sfx::Whip;
    case SkillId::FrogFlyCatcher: case SkillId::FrogMudSlime: return Sfx::Slurp;
    case SkillId::FrogSwampSpout: return Sfx::Geyser;
    case SkillId::JoanaClawCrush: return Sfx::ClawSnap;
    case SkillId::JoanaHookPull: return Sfx::HarpoonChain;
    case SkillId::JoanaAnchorSlam: return Sfx::TidalWave;
    case SkillId::CremosoSyrupBlast: case SkillId::CremosoPuddleTrap: return Sfx::SyrupBoil;
    case SkillId::CremosoVanillaWhip: return Sfx::CreamWhip;
    case SkillId::CremosoConePunch: return Sfx::Crunch;
    case SkillId::ValeriaMagmaPunch: return Sfx::MagmaExplosion;
    case SkillId::ValeriaVolcanicErupt: return Sfx::VolcanoEruption;
    case SkillId::VivaldiCadenceStaccato: return Sfx::StaccatoViolin;
    case SkillId::VivaldiWinterAllegro: return Sfx::WinterViolin;
    case SkillId::VivaldiCrescendoJab: return Sfx::Crescendo;
    case SkillId::EspinhaNopalSlap: return Sfx::CactusSlap;
    case SkillId::EspinhaLuchaFlyer: return Sfx::LuchaImpact;
    case SkillId::EspinhaNeedleJab: return Sfx::SpikePierce;
    case SkillId::MarcovRocketThrust: return Sfx::RocketThrust;
    case SkillId::MarcovOrbitalLaser: return Sfx::OrbitalLaser;
    case SkillId::BossDualSaw: return Sfx::SawBuzz;
    case SkillId::BossOverclock: return Sfx::ReactorPulse;
    case SkillId::BossEmp: return Sfx::EmpShock;
    case SkillId::BossMissiles: return Sfx::MissileBarrage;
    case SkillId::BossHolo: return Sfx::ArcadeCombo;
    case SkillId::BossApocalypse: return Sfx::ApocalypseLaser;
    default: return Sfx::PlasmaShield;
    }
}

bool isHealSkill(SkillId id) {
    if (id == SkillId::JackRoyalJelly || id == SkillId::FrogFlyCatcher || id == SkillId::JoanaOceanBounty) return true;
    const SkillDef* def = SkillSystem::get(id);
    return def && def->fx.heal > 0.0f;
}

} // namespace

std::string GameEngine::skillBanner(const Fighter& caster, SkillId id) const {
    const SkillDef* def = SkillSystem::get(id);
    std::string name = def ? utf8Upper(def->name) : "";
    if (caster.isPlayer) return "¡" + name + "!";
    return std::string("¡") + kBannerNames[std::min<uint32_t>(caster.charId, 8)] + ": " + name + "!";
}

bool GameEngine::commandEntered(SkillCommand command, uint32_t held) const {
    const auto& h = commandHistory_;
    auto count = [&](uint32_t bit) {
        int n = 0;
        for (const InputEvent& e : h) n += e.bit == bit ? 1 : 0;
        return n;
    };
    switch (command) {
    case SkillCommand::BackForward: {
        // ← → ou → →, e um soco depois da seta para a frente.
        bool sawLeft = false, leftThenRight = false, punchAfterRight = false;
        int rights = 0;
        for (const InputEvent& e : h) {
            if (e.bit == INPUT_LEFT) sawLeft = true;
            else if (e.bit == INPUT_RIGHT) { leftThenRight |= sawLeft; rights++; }
            else if ((e.bit & kPunchBits) && rights > 0) punchAfterRight = true;
        }
        return punchAfterRight && (rights >= 2 || leftThenRight);
    }
    case SkillCommand::DownUp: {
        bool down = false, up = false, punchAfterUp = false;
        for (const InputEvent& e : h) {
            if (e.bit == INPUT_DOWN) down = true;
            else if (e.bit == INPUT_UP && down) up = true;
            else if ((e.bit & kPunchBits) && up) punchAfterUp = true;
        }
        return up && (punchAfterUp || (held & kPunchBits));
    }
    case SkillCommand::BothPunches: {
        bool z = held & INPUT_PUNCH_L, x = held & INPUT_PUNCH_R;
        for (const InputEvent& e : h) {
            if (frameIndex - e.frame > kBothPunchesWindow) continue;
            z |= e.bit == INPUT_PUNCH_L;
            x |= e.bit == INPUT_PUNCH_R;
        }
        return z && x;
    }
    case SkillCommand::UpUpRight:
        return count(INPUT_UP) >= 2 && (count(INPUT_PUNCH_R) > 0 || (held & INPUT_PUNCH_R));
    case SkillCommand::DownDownLeft:
        return count(INPUT_DOWN) >= 2 && (count(INPUT_PUNCH_L) > 0 || (held & INPUT_PUNCH_L));
    case SkillCommand::BackBackLeft:
        return count(INPUT_LEFT) >= 2 && (count(INPUT_PUNCH_L) > 0 || (held & INPUT_PUNCH_L));
    case SkillCommand::ForwardForwardRight: {
        int rights = 0;
        bool punchAfterTwo = false;
        for (const InputEvent& e : h) {
            if (e.bit == INPUT_RIGHT) rights++;
            else if ((e.bit & kPunchBits) && rights >= 2) punchAfterTwo = true;
        }
        return punchAfterTwo;
    }
    case SkillCommand::BackForwardLeft: {
        bool sawLeft = false, leftThenRight = false, zAfter = false;
        for (const InputEvent& e : h) {
            if (e.bit == INPUT_LEFT) sawLeft = true;
            else if (e.bit == INPUT_RIGHT && sawLeft) leftThenRight = true;
            else if (e.bit == INPUT_PUNCH_L && leftThenRight) zAfter = true;
        }
        return zAfter;
    }
    case SkillCommand::None:
        break;
    }
    return false;
}

bool GameEngine::chainReady(SkillEntry entry) const {
    if (entry == SkillEntry::Full) return chain_ >= 3;
    if (entry == SkillEntry::Flow) return chain_ >= 2 || flowWindow_ > 0;
    return true;
}

bool GameEngine::finisherEntered(const SkillDef& def, uint32_t held) const {
    const SkillFinisher f = SkillSystem::finisherOf(def);
    if (f.both) return commandEntered(SkillCommand::BothPunches, held);
    // o soco mais recente, apertado há pouco, tem de ser o do finalizador
    const InputEvent* punch = nullptr;
    for (auto it = commandHistory_.rbegin(); it != commandHistory_.rend(); ++it) if (it->bit & kPunchBits) { punch = &*it; break; }
    if (!punch || frameIndex - punch->frame >= static_cast<uint32_t>(kBufferFrames)) return false;
    if (!((f.z && punch->bit == INPUT_PUNCH_L) || (f.x && punch->bit == INPUT_PUNCH_R))) return false;
    if (held & f.dir) return true;
    for (const InputEvent& e : commandHistory_) if (e.bit == f.dir && frameIndex - e.frame < 24u) return true;   // seta apertada logo antes do soco
    return false;
}

bool GameEngine::tryBufferedCommand(uint32_t held, uint32_t& audio) {
    return !commandHistory_.empty() && frameIndex - commandHistory_.back().frame < kBufferFrames &&
           tryPlayerCommand(held, audio);
}

bool GameEngine::tryPlayerCommand(uint32_t held, uint32_t& audio) {
    // Pode lançar parado (inclusive carregando um soco), na guarda ou cancelando a esquiva para trás,
    // porque ↓↓ (do comando ↓↓+Z) já dispara essa esquiva antes do soco chegar.
    // Um soco que começou há no máximo 6 quadros também pode virar comando (Z e X quase juntos, por exemplo).
    const bool punchStartup = isPunchState(player.state) && player.maxTimer - player.timer <= 6;
    if (player.state != FIGHTER_IDLE && player.state != FIGHTER_BLOCK && player.state != FIGHTER_DODGE_BACK && !punchStartup) {
        return false;
    }

    for (SkillId id : commandOrder(player.charId)) {
        if (!player.loadout.canCast(id)) continue;
        const SkillDef* def = SkillSystem::get(id);
        if (!def) continue;
        // curas, modos e escudos: o comando simples de sempre. Os outros: combo (ou esquiva, nos de fluxo) e o finalizador.
        const SkillEntry entry = SkillSystem::entryOf(*def);
        if (entry == SkillEntry::Quick ? !commandEntered(def->command, held) : !(chainReady(entry) && finisherEntered(*def, held))) continue;

        // Efeito já ativo: o comando não repete (como no JS).
        if ((id == SkillId::JackInfestation && enemy.status.infested > 0) ||
            (id == SkillId::JackMolasses && player.status.molasses > 0) ||
            (id == SkillId::JackHoneyWall && player.status.honeyShield > 0) ||
            (id == SkillId::FrogPoisonSkin && player.status.poisonSkin > 0) ||
            (id == SkillId::JoanaCrustaceanShell && player.status.shell > 0) ||
            (def->fx.mode != SkillMode::None && (player.status.melt > 0 || player.status.heatwave > 0 || player.status.seasons > 0 ||
                                                 player.status.thornVeil > 0 || player.status.plasma > 0))) {
            continue;
        }
        if (isHealSkill(id) && player.status.healCooldown > 0) {
            if (!healBannerShown_) {
                int secs = (player.status.healCooldown + 59) / 60;
                combat.triggerStatusBanner(utf8Upper(def->name) + " EM RECARGA (" + std::to_string(secs) + "s)");
                healBannerShown_ = true;
            }
            continue;
        }

        chainCast_ = entry != SkillEntry::Quick;
        const bool cast = castSkill(player, enemy, id, false, audio);
        chainCast_ = false;
        if (cast) {
            if (entry != SkillEntry::Quick) { chain_ = 0; flowWindow_ = 0; }
            commandHistory_.clear();
            cancelCharge();
            pendingDodgeBack_ = false;
            bufferedHand_ = -1;
            healBannerShown_ = false;
            return true;
        }
    }
    return false;
}

bool GameEngine::castSkill(Fighter& caster, Fighter& target, SkillId id, bool signature, uint32_t& audio) {
    const SkillDef* def = SkillSystem::get(id);
    if (!def || def->type == SkillType::Passive) return false;

    // Custo estratégico de recurso para habilidades (Stamina & Combo Meter)
    float skillCost = 25.0f;
    if (isHealSkill(id)) skillCost = 20.0f;
    if (def->type == SkillType::Mode) skillCost = 22.0f;

    if (caster.isPlayer && chainCast_) {
        // lançado pelo combo de entrada (ou pela esquiva que abre o fluxo): os socos já pagaram, o especial não cobra de novo nem falha por falta de fôlego
        caster.staminaRegenDelay = 30;
    } else if (caster.isPlayer) {
        if (comboCount >= 3) {
            skillCost = 12.0f; // Bônus de Combo: custo reduzido de estamina!
            combat.spawnFloatingText("COMBO SKILL!", 0xffe600ff, caster.worldPos.x, caster.worldPos.y + 70.0f, caster.worldPos.z, true, 0);
        }
        if (caster.isExhausted || caster.stamina < 15.0f) {
            combat.triggerStatusBanner("FALTA ESTAMINA PARA HABILIDADE!");
            return false;
        }
        caster.stamina = std::max(0.0f, caster.stamina - skillCost);
        caster.staminaRegenDelay = 45;
    } else {
        caster.stamina = std::max(0.0f, caster.stamina - skillCost * 0.5f);
    }

    caster.startMove(FIGHTER_SKILL, std::max(1, static_cast<int>(std::lround(def->moveFrames * skillStretch(caster.charId)))));
    caster.activeSkill = id;
    caster.signatureOnly = signature;
    caster.skillEventDone = false;
    caster.telegraphFlash = false;
    audio |= AUDIO_SFX_SKILL_ACTIVATE;
    cue(skillSound(id));
    if (signature) {
        // Limpa status negativos, exaustão e olho tonto para a assinatura sair limpa e heroica
        caster.status = StatusEffects{};
        caster.isExhausted = false;
        caster.staminaRegenDelay = 0;
        caster.stamina = std::max(caster.stamina, 40.0f);
        caster.isSluggish = false;
        caster.hitKind = 0;
        return true;
    }

    combat.triggerStatusBanner(skillBanner(caster, id));
    const Vec3 at = frontOf(target, 70.0f, 0.0f);
    switch (id) {
    case SkillId::JackInfestation:
        target.status.infested = 6 * 60;
        particles.spawnImpact(at.x, at.y, at.z, 18, 0xffd700ff);
        break;
    case SkillId::JackMolasses:
        caster.status.molasses = 6 * 60;
        break;
    case SkillId::JackHoneyWall:
        caster.status.honeyShield = 3;
        break;
    case SkillId::JackRoyalJelly:
        heal(caster, 12.0f);
        caster.status.healCooldown = kHealCooldown;
        break;
    case SkillId::FrogPoisonSkin: {
        caster.status.poisonSkin = 6 * 60;
        target.status.poisoned = 6 * 60;
        Vec3 c = frontOf(caster, 60.0f, 0.0f);
        particles.spawnImpact(c.x, c.y, c.z, 20, 0x10b981ff);
        break;
    }
    case SkillId::FrogFlyCatcher:
        heal(caster, 12.0f);
        caster.status.burn = 0;
        caster.status.bleed = 0;
        caster.status.slowed = 0;
        caster.status.healCooldown = kHealCooldown;
        break;
    case SkillId::JoanaCrustaceanShell: {
        caster.status.shell = 5 * 60;
        Vec3 c = frontOf(caster, 60.0f, 0.0f);
        particles.spawnImpact(c.x, c.y, c.z, 20, 0x38bdf8ff);
        break;
    }
    case SkillId::JoanaOceanBounty:
        heal(caster, 12.0f);
        caster.status.healCooldown = kHealCooldown;
        break;
    default: {
        // Ficha declarativa: cura e modo saem na hora; o resto no impacto (updateSkillMove).
        const SkillEffect& fx = def->fx;
        if (fx.heal > 0.0f) {
            heal(caster, fx.heal);
            caster.status.healCooldown = kHealCooldown;
        }
        switch (fx.mode) {
        case SkillMode::Melt: caster.status.melt = fx.modeFrames; break;
        case SkillMode::Heatwave: caster.status.heatwave = fx.modeFrames; break;
        case SkillMode::Seasons: caster.status.seasons = fx.modeFrames; break;
        case SkillMode::ThornVeil: caster.status.thornVeil = fx.modeFrames; break;
        case SkillMode::Plasma: caster.status.plasma = fx.modeFrames; break;
        case SkillMode::None: break;
        }
        if (fx.mode != SkillMode::None) {
            Vec3 c = frontOf(caster, 60.0f, 0.0f);
            particles.spawnImpact(c.x, c.y, c.z, 22, 0x93c5fdff);
        }
        break;
    }
    }
    caster.skillHitsDone = 0;
    if (caster.loadout.has(SkillId::MarcovThrusterCharge)) caster.stamina = std::min(caster.maxStamina, caster.stamina + 20.0f);
    return true;
}

void GameEngine::updateSkillMove(Fighter& caster, Fighter& target, uint32_t& audio, bool allowEffects) {
    caster.timer--;
    const float elapsed = skillElapsed(caster);   // em quadros da ficha (o especial dura kSkillStretch vezes isso)
    const float p = 1.0f - static_cast<float>(caster.timer) / std::max(1, caster.maxTimer);
    const float swing = std::sin(p * kPi);
    const SkillId id = caster.activeSkill;

    // Pose da habilidade (skill_poses.cpp); sem pose dedicada, cai nas genéricas abaixo.
    caster.resetPose();
    if (caster.bossStage == 0 && animateSkillPose(caster, id, p)) {
    } else switch (id) {
    case SkillId::JackStingJab:
    case SkillId::FrogTongue:
    case SkillId::FrogMudSlime:
        caster.animateCommonPunch(FIGHTER_JAB, p, caster.isLeftPunch);
        break;
    case SkillId::FrogSwampSpout:
        caster.animateCommonPunch(FIGHTER_UPPER, p, caster.isLeftPunch);
        break;
    case SkillId::JoanaClawCrush:
        caster.animateCommonPunch(FIGHTER_CROSS, p, false); // a garra é a mão direita
        break;
    case SkillId::JoanaCoralSpout:
        caster.animateCommonPunch(FIGHTER_UPPER, p, false);
        break;
    case SkillId::JoanaHookPull:
        caster.animateCommonPunch(FIGHTER_JAB, std::min(p, 0.55f), true); // o gancho é a mão esquerda
        break;
    case SkillId::JoanaAnchorSlam:
        // Ergue o gancho e crava no chão.
        caster.gloveL.y = p < 0.4f ? caster.baseGloveL.y + 60.0f * (p / 0.4f)
                                   : std::max(5.0f, caster.baseGloveL.y + 60.0f - 140.0f * (p - 0.4f) / 0.2f);
        caster.gloveL.z += 30.0f * swing;
        caster.offsetY = (p > 0.4f && p < 0.7f) ? -10.0f : 0.0f;
        break;
    case SkillId::JackInfestation:
    case SkillId::JackCombBurst:
        caster.gloveL.z += 70.0f * swing; caster.gloveR.z += 70.0f * swing;
        caster.gloveL.y += 20.0f * swing; caster.gloveR.y += 20.0f * swing;
        caster.bodySkew = 0.1f * std::sin(p * kPi * 4.0f);
        break;
    case SkillId::FrogRoyalLeap:
        if (elapsed <= 14) caster.offsetY = std::sin((elapsed / 14.0f) * kPi * 0.5f) * 220.0f;
        else if (elapsed <= 22) caster.offsetY = 220.0f;
        else if (elapsed <= 27) caster.offsetY = 220.0f * (1.0f - (elapsed - 22) / 5.0f);
        else caster.offsetY = 0.0f;
        caster.gloveL.y += 40.0f; caster.gloveR.y += 40.0f;
        break;
    default:
        if (caster.bossStage > 0) {
            // Batalhator: ergue serras e controles e treme, sem avançar as luvas (enormes perto da câmera).
            caster.gloveL.y += 25.0f * swing; caster.gloveR.y += 25.0f * swing;
            caster.gloveL.x -= 10.0f * swing; caster.gloveR.x += 10.0f * swing;
            caster.offsetY = -8.0f * swing;
            caster.bodySkew = 0.05f * std::sin(p * kPi * 6.0f);
            break;
        }
        if (const SkillDef* def = SkillSystem::get(id); def && def->fx.airborne) {
            // Salto de lucha libre: sobe, paira e cai em cima do alvo.
            float up = std::min(1.0f, p / 0.4f), down = p > 0.7f ? std::min(1.0f, (p - 0.7f) / 0.12f) : 0.0f;
            caster.offsetY = 200.0f * std::sin(up * kPi * 0.5f) * (1.0f - down);
            caster.gloveL.y += 40.0f; caster.gloveR.y += 40.0f;
            break;
        } else if (def && def->fx.damage > 0.0f) {
            // Golpes da ficha: soco de direita, ou uppercut nos ↓↑ + X, alternando as mãos nos rítmicos.
            bool left = def->fx.hits > 1 ? (caster.skillHitsDone % 2 == 1) : false;
            FighterActionState pose = def->command == SkillCommand::DownUp ? FIGHTER_UPPER : FIGHTER_CROSS;
            float local = def->fx.hits > 1 ? std::fmod(p * def->fx.hits, 1.0f) : p;
            caster.animateCommonPunch(pose, local, left);
            break;
        }
        // modos e curas: luvas no peito, corpo pulsando
        caster.gloveL.x = -24.0f; caster.gloveR.x = 24.0f;
        caster.gloveL.y += 18.0f * swing; caster.gloveR.y += 18.0f * swing;
        caster.offsetY = -6.0f * swing;
        break;
    }

    if (allowEffects && !caster.skillEventDone) {
        const bool targetUp = target.state != FIGHTER_KNOCKDOWN;
        if (caster.signatureOnly) {
            // Golpe-assinatura da IA: acerta como um golpe comum.
            if (p > 0.40f && p < 0.65f) {
                caster.skillEventDone = true;
                if (&caster == &enemy) checkHitOnPlayer(false, audio);
                else checkHitOnEnemy(FIGHTER_JAB, 0.0f, audio);
            }
        } else if (targetUp) {
            switch (id) {
            case SkillId::JackStingJab:
                if (p >= 0.45f) { caster.skillEventDone = true; if (skillDamage(caster, target, 15.0f, "", audio)) target.status.infested = std::max(target.status.infested, 150); }   // a Agulhada marca o alvo: o enxame vai atrás
                break;
            case SkillId::FrogSwampSpout:
                if (p >= 0.45f) {
                    caster.skillEventDone = true;
                    if (skillDamage(caster, target, 12.0f, "", audio)) {
                        hitstopTimer = std::max(hitstopTimer, 12);
                        screenShake = std::max(screenShake, 24.0f);
                        impactKind = 2; // Shockwave ripple tremenda
                        impactTimer = 28;
                        impactSeed = static_cast<int>(frameIndex);
                        impactAt = frontOf(target, 60.0f);
                        Vec3 g = frontOf(target, 10.0f, 0.0f);
                        particles.spawnImpact(g.x, g.y, g.z, 35, 0x38bdf8ff);
                        cue(Sfx::Crunch); // CRACK brusco do gêiser
                        if (gameState == STATE_FIGHTING && target.state != FIGHTER_KNOCKDOWN && target.state != FIGHTER_BLOCK) {
                            target.startMove(FIGHTER_HIT, 28);
                            target.hitKind = 2;
                            target.vulnerable = 35; // Abre brecha no chão (stagger firme sem juggle)
                        }
                    }
                }
                break;
            case SkillId::FrogTongue:
                if (elapsed >= 10) {
                    caster.skillEventDone = true;
                    if (skillDamage(caster, target, 14.0f, "", audio)) {
                        if (!trainingMode && target.charId != 9) {
                            target.radius = std::min(&target == &enemy ? 220.0f : 215.0f, target.radius + (&target == &enemy ? 55.0f : 35.0f));
                        }
                        target.status.slowed = 4 * 60;
                    }
                }
                break;
            case SkillId::FrogRoyalLeap:
                if (elapsed >= 28) {
                    caster.skillEventDone = true;
                    if (skillDamage(caster, target, 16.0f, "", audio)) {
                        hitstopTimer = std::max(hitstopTimer, 15); // 15 quadros de freeze frame épico no impacto da queda
                        screenShake = std::max(screenShake, 30.0f);
                        impactKind = 2; // Shockwave ripple tremenda no impacto
                        impactTimer = 32;
                        impactSeed = static_cast<int>(frameIndex);
                        impactAt = frontOf(target, 65.0f);
                        Vec3 g = frontOf(target, 20.0f, 0.0f);
                        particles.spawnImpact(g.x, g.y, g.z, 30, 0x10b981ff); // impacto verde-anfíbio
                        cue(Sfx::Crunch); // CRACK brusco do salto esmagador
                        if (gameState == STATE_FIGHTING && target.state != FIGHTER_KNOCKDOWN && target.state != FIGHTER_BLOCK) {
                            if (&target == &enemy) { enemy.state = FIGHTER_IDLE; enemy.resetPose(); enemy.actionTimer = 60; }
                            else player.startMove(FIGHTER_HIT, 30);
                        }
                    }
                }
                break;
            case SkillId::ValeriaMagmaPunch:
                if (p >= 0.45f) {
                    caster.skillEventDone = true;
                    if (skillDamage(caster, target, 14.0f, "", audio)) {
                        hitstopTimer = std::max(hitstopTimer, 14); // 14 quadros de freeze frame vulcânico
                        screenShake = std::max(screenShake, 28.0f);
                        impactKind = 2; // Shockwave Ripple + Aberração cromática dramática
                        impactTimer = 30;
                        impactSeed = static_cast<int>(frameIndex);
                        impactAt = frontOf(target, 70.0f);
                        Vec3 g = frontOf(target, 40.0f, 0.0f);
                        particles.spawnImpact(g.x, g.y, g.z, 32, 0xff4500ff); // chamas/magma vivo
                        target.status.burn = 4 * 60;
                        target.status.burnDps = 2.0f;
                        cue(Sfx::Crunch); // CRACK brusco vulcânico
                    }
                }
                break;
            case SkillId::EspinhaLuchaFlyer:
                if (elapsed >= 26) {
                    caster.skillEventDone = true;
                    if (skillDamage(caster, target, 16.0f, "", audio)) {
                        hitstopTimer = std::max(hitstopTimer, 15); // 15 quadros de freeze frame de lucha livre
                        screenShake = std::max(screenShake, 30.0f);
                        impactKind = 2; // Shockwave ripple
                        impactTimer = 32;
                        impactSeed = static_cast<int>(frameIndex);
                        impactAt = frontOf(target, 65.0f);
                        Vec3 g = frontOf(target, 20.0f, 0.0f);
                        particles.spawnImpact(g.x, g.y, g.z, 30, 0xf43f5eff); // impacto rosa lucha
                        cue(Sfx::Crunch); // CRACK brusco da queda da terceira corda
                        if (gameState == STATE_FIGHTING && target.state != FIGHTER_KNOCKDOWN && target.state != FIGHTER_BLOCK) {
                            if (&target == &enemy) { enemy.state = FIGHTER_IDLE; enemy.resetPose(); enemy.actionTimer = 55; }
                            else player.startMove(FIGHTER_HIT, 28);
                        }
                    }
                }
                break;
            case SkillId::JoanaClawCrush:
                if (elapsed >= 12) {
                    caster.skillEventDone = true;
                    if (skillDamage(caster, target, 16.0f, "", audio)) {
                        hitstopTimer = std::max(hitstopTimer, 14);
                        screenShake = std::max(screenShake, 26.0f);
                        impactKind = 2;
                        impactTimer = 30;
                        impactSeed = static_cast<int>(frameIndex);
                        impactAt = frontOf(target, 70.0f);
                        cue(Sfx::Crunch); // CRACK brusco do fechamento da garra
                        if (gameState == STATE_FIGHTING && target.state != FIGHTER_KNOCKDOWN) {
                            if (&target == &enemy) { enemy.state = FIGHTER_IDLE; enemy.resetPose(); enemy.actionTimer = 50; }
                            else player.startMove(FIGHTER_HIT, 25);
                        }
                    }
                }
                break;
            case SkillId::JoanaHookPull:
                if (elapsed >= 12) {
                    caster.skillEventDone = true;
                    if (skillDamage(caster, target, 15.0f, "", audio)) {
                        if (!trainingMode && target.charId != 9) {
                            target.radius = std::max(&target == &enemy ? 30.0f : 80.0f, target.radius - 55.0f);
                        }
                        if (gameState == STATE_FIGHTING && target.hp > 0.0f && target.state != FIGHTER_KNOCKDOWN) {
                            if (&target == &enemy) {
                                enemy.state = FIGHTER_IDLE;
                                enemy.resetPose();
                                enemy.actionTimer = 35; // vantagem de frames / stun para punição
                            } else {
                                player.startMove(FIGHTER_HIT, 20);
                            }
                        }
                        Vec3 g = frontOf(target, 80.0f);
                        particles.spawnImpact(g.x, g.y, g.z, 12, 0xcbd5e1ff);
                        screenShake = std::max(screenShake, 8.0f);
                    }
                }
                break;
            case SkillId::JoanaCoralSpout:
                if (p >= 0.45f) {
                    caster.skillEventDone = true;
                    if (skillDamage(caster, target, 14.0f, "", audio)) {
                        target.status.bleed = 5 * 60;
                    }
                }
                break;
            case SkillId::JoanaAnchorSlam:
                if (elapsed >= 18) {
                    caster.skillEventDone = true;
                    if (skillDamage(caster, target, 14.0f, "", audio)) {
                        target.status.slowed = 4 * 60; // a onda desestabiliza
                        screenShake = std::max(screenShake, 14.0f);
                        Vec3 g = frontOf(target, 5.0f, 0.0f);
                        particles.spawnImpact(g.x, g.y, g.z, 24, 0x38bdf8ff);
                    }
                }
                break;
            case SkillId::FrogMudSlime:
                if (elapsed >= 14) {
                    caster.skillEventDone = true;
                    if (skillDamage(caster, target, 12.0f, "", audio)) {
                        target.status.mud = 3 * 60;
                        target.status.slowed = 3 * 60;
                        Vec3 g = frontOf(target, 5.0f, 0.0f);
                        particles.spawnImpact(g.x, g.y, g.z, 20, 0x7a5230ff);
                    }
                }
                break;
            default: {
                const SkillDef* def = SkillSystem::get(id);
                if (!def) { caster.skillEventDone = true; break; }
                const SkillEffect& fx = def->fx;
                if (fx.hits > 1) {
                    // Golpes rítmicos espalhados entre "at" e 90% da animação.
                    float span = (0.9f - fx.at) / static_cast<float>(fx.hits - 1);
                    while (caster.skillHitsDone < fx.hits && p >= fx.at + span * caster.skillHitsDone) {
                        caster.skillHitsDone++;
                        bool last = caster.skillHitsDone == fx.hits;
                        applySkillEffect(caster, target, last ? fx : SkillEffect{}, fx.damage / fx.hits, audio);
                        if (gameState != STATE_FIGHTING) break;
                    }
                    if (caster.skillHitsDone >= fx.hits) caster.skillEventDone = true;
                } else if (p >= fx.at) {
                    caster.skillEventDone = true;
                    if (fx.at > 0.0f) applySkillEffect(caster, target, fx, fx.damage, audio);
                }
                break;
            }
            }
        }
    }

    if (caster.timer <= 0 && caster.state == FIGHTER_SKILL) {
        caster.state = FIGHTER_IDLE;
        caster.resetPose();
        caster.activeSkill = SkillId::None;
        caster.signatureOnly = false;
        if (&caster == &enemy) enemy.actionTimer = 35 + std::rand() % 45;
    }
}

bool GameEngine::skillDamage(Fighter& caster, Fighter& target, float damage, const char* label, uint32_t& audio) {
    (void)label;
    const SkillDef* def = SkillSystem::get(caster.activeSkill);
    bool isGuardBreak = (caster.activeSkill == SkillId::JoanaClawCrush ||
                         caster.activeSkill == SkillId::EspinhaNopalSlap ||
                         (def && def->tag && std::strcmp(def->tag, "BREAK") == 0));
    bool isGuardSting = (caster.activeSkill == SkillId::JackStingJab ||
                         caster.activeSkill == SkillId::CremosoConePunch ||
                         caster.activeSkill == SkillId::ValeriaLavaKickJab ||
                         caster.activeSkill == SkillId::VivaldiCrescendoJab ||
                         caster.activeSkill == SkillId::EspinhaNeedleJab ||
                         caster.activeSkill == SkillId::MarcovKineticOverdrive ||
                         (def && def->tag && std::strcmp(def->tag, "STING") == 0));

    bool isDodging = target.state == FIGHTER_DODGE_L || target.state == FIGHTER_DODGE_R ||
                     target.state == FIGHTER_DODGE_BACK || target.state == FIGHTER_DUCK;

    if (isDodging || isAirborne(target) || attackMisses(caster, target)) {
        if (isDodging && target.isPlayer) flowWindow_ = kFlowWindow;   // esquiva bem feita abre os especiais de fluxo
        audio |= AUDIO_SFX_DODGE;
        cue(Sfx::Dodge);
        cue(Sfx::CrowdWoah);
        combat.spawnFloatingText("ESQUIVOU!", 0x38bdf8ff, target.worldPos.x, target.worldPos.y + 110.0f, target.worldPos.z, false, 0);

        if (&caster == &enemy) {
            enemy.vulnerable = std::max(enemy.vulnerable, 26);
            combat.spawnFloatingText("BRECHA!", 0x00f0ffff, enemy.worldPos.x, enemy.worldPos.y + 110.0f, enemy.worldPos.z, true, 0);
        }

        if (isDodging) dodgeReward(target, caster);
        if (isDodging && (target.charId == 3 || target.loadout.has(SkillId::CremosoSweetRevenge))) {
            heal(target, 2.0f);
            target.revengeBonus = 1.15f;
        }
        if (isDodging && target.loadout.has(SkillId::JackBeekeeper)) {
            caster.hp = std::max(0.0f, caster.hp - 3.0f);
            Vec3 at = frontOf(caster, 50.0f);
            particles.spawnImpact(at.x, at.y + 40.0f, at.z, 15, 0xffd700ff);
            combat.spawnFloatingText("-3 ABELHA!", 0xffd700ff, caster.worldPos.x, caster.worldPos.y + 90.0f, caster.worldPos.z, false, 0);
        }
        return false;
    }

    bool blocked = (target.state == FIGHTER_BLOCK);

    if (blocked) {
        if (isGuardBreak) {
            // Quebra de guarda: rompe o bloqueio e abre brecha
            target.startMove(FIGHTER_HIT, 28);
            target.hitKind = 2;
            target.vulnerable = 25;
            audio |= AUDIO_SFX_COUNTER_HIT;
            cue(Sfx::Crunch); // CRACK brusco de quebra de guarda!
            cue(Sfx::Impact);
            combat.spawnFloatingText("GUARD BREAK!", 0xffa500ff, target.worldPos.x, target.worldPos.y + 110.0f, target.worldPos.z, true, 0);
        } else if (target.isPlayer && target.parryWindowTimer > 0) {
            // PARRY PERFEITO CONTRA HABILIDADE ESPECIAL! (aparada nos primeiros 10 quadros de guarda = 166ms)
            audio |= AUDIO_SFX_COUNTER_HIT;
            cue(Sfx::Parry);
            cue(Sfx::CrowdCheer);
            screenShake = std::max(screenShake, 22.0f);
            hitstopTimer = 14; // Congelamento de impacto
            impactKind = 2;   // Shockwave Ripple + Aberração Cromática
            impactTimer = 32;
            impactSeed = static_cast<int>(frameIndex);
            impactAt = frontOf(target, 65.0f);

            target.stamina = std::min(target.maxStamina, target.stamina + 25.0f);
            target.staminaRegenDelay = 0;
            target.isExhausted = false;

            caster.startMove(FIGHTER_HIT, 28);
            caster.hitKind = 2;
            caster.vulnerable = 35; // Brecha de 35 frames contra especial aparado

            combat.spawnFloatingText("PARRY!", 0x00f5d4ff, target.worldPos.x, target.worldPos.y + 85.0f, target.worldPos.z, true, 0);
            Vec3 at = frontOf(target, 65.0f);
            particles.spawnImpact(at.x, at.y, at.z, 30, 0x00f5d4ff);
            return false;
        } else if (isGuardSting) {
            // Golpe perfurante: ignora guarda como prometido na descrição
            combat.spawnFloatingText("PERFUROU!", 0x38bdf8ff, target.worldPos.x, target.worldPos.y + 110.0f, target.worldPos.z, true, 0);
        } else {
            // Especial comum bloqueado: reduz para 45% (chip damage) e não atordoa
            damage = std::round(damage * 0.45f);
            audio |= AUDIO_SFX_BLOCK;
            cue(Sfx::Block);
            screenShake = std::max(screenShake, 8.0f);
            combat.spawnFloatingText("BLOQUEADO (-" + std::to_string(static_cast<int>(damage)) + ")", 0x94a3b8ff,
                                     target.worldPos.x, target.worldPos.y + 90.0f, target.worldPos.z, false, 0);
            Vec3 at = frontOf(target, 70.0f);
            particles.spawnImpact(at.x, at.y, at.z, 8, 0x94a3b8ff);
        }
    }

    // estocadas, cada uma com o seu efeito: a do Cremoso pune quem está na guarda; a da Vivaldi cresce com a sequência de acertos
    if (caster.activeSkill == SkillId::CremosoConePunch && blocked) damage += 6.0f;
    if (caster.activeSkill == SkillId::VivaldiCrescendoJab) damage = 11.0f + 2.0f * static_cast<float>(std::min(4, caster.isPlayer ? comboCount : caster.rhythmStacks));
    damage = std::round(damage * arch(caster.charId).dealt * arch(target.charId).taken); // estilo de cada um (canhão de vidro)
    damage = applyStatusDefense(target, caster, damage);
    if (trainingMode || target.charId == 9) {
        target.hp = std::max(1.0f, target.hp - damage);
    } else {
        target.hp = std::max(0.0f, target.hp - damage);
    }
    if (!blocked || isGuardBreak || isGuardSting) {
        audio |= AUDIO_SFX_PUNCH_HEAVY;
        int hitstop = (damage >= 14.0f ? 12 : (damage >= 10.0f ? 8 : 4));
        hitstopTimer = std::max(hitstopTimer, hitstop);
        screenShake = std::max(screenShake, damage >= 14.0f ? 24.0f : 16.0f);
        Vec3 at = frontOf(target, 80.0f);
        if (target.isDummy()) {
            particles.spawnStraw(at.x, at.y + 10.0f, at.z, 22);
        } else {
            particles.spawnImpact(at.x, at.y, at.z, 20, 0xef4444ff);
        }
        if (damage >= 10.0f || isGuardBreak || isGuardSting) {
            impactKind = 2; // Shockwave ripple + Contact flash
            impactTimer = 28;
            impactSeed = static_cast<int>(frameIndex);
            impactAt = at;
            cue(Sfx::Crunch); // CRACK brusco de impacto contundente!
            cue(Sfx::CrowdImpact); // ovação potente da plateia com habilidade pesada
            if (&target == &player) cue(Sfx::CrowdGasp); // suspiro tenso ao ver o jogador atingido por especial
        }
        combat.spawnFloatingText("-" + std::to_string(static_cast<int>(damage)), 0xef4444ff, target.worldPos.x,
                                 target.worldPos.y + 90.0f, target.worldPos.z, true, caster.isLeftPunch ? 1 : 0);
    }
    if (&caster == &player) registerComboHit();
    else resetCombo();
    if (!trainingMode && target.charId != 9 && target.hp <= 0.0f) triggerKnockdown(&target == &enemy, audio);
    return true;
}

void GameEngine::applySkillEffect(Fighter& caster, Fighter& target, const SkillEffect& fx, float damage, uint32_t& audio) {
    bool hit = true;
    if (damage > 0.0f) hit = skillDamage(caster, target, std::round(damage), "", audio);
    if (!hit || gameState != STATE_FIGHTING || target.state == FIGHTER_KNOCKDOWN) return;
    const SkillDef* def = SkillSystem::get(caster.activeSkill);
    bool isGuardBreak = (caster.activeSkill == SkillId::JoanaClawCrush ||
                         caster.activeSkill == SkillId::EspinhaNopalSlap ||
                         (def && def->tag && std::strcmp(def->tag, "BREAK") == 0));
    bool blocked = (target.state == FIGHTER_BLOCK && !isGuardBreak);
    StatusEffects& s = target.status;
    if (fx.burn > 0) { s.burn = blocked ? fx.burn / 2 : fx.burn; s.burnDps = fx.burnDps; }
    if (fx.slow > 0) s.slowed = std::max(s.slowed, blocked ? fx.slow / 2 : fx.slow);
    if (fx.blind > 0 && !blocked) s.blind = std::max(s.blind, fx.blind);
    if (fx.bleed > 0 && !blocked) s.bleed = std::max(s.bleed, fx.bleed);
    if (fx.mud > 0 && !blocked) s.mud = std::max(s.mud, fx.mud);
    if (fx.staminaDrain > 0.0f) target.stamina = std::max(0.0f, target.stamina - (blocked ? fx.staminaDrain * 0.4f : fx.staminaDrain));
    if (fx.push > 0.0f && !target.loadout.has(SkillId::MarcovZeroGravity) && !trainingMode && target.charId != 9) {
        target.radius = std::min(235.0f, target.radius + (blocked ? fx.push * 0.5f : fx.push));
        if (target.radius >= 170.0f || fx.push >= 25.0f) {
            cue(Sfx::CrowdGasp); // empurrão contra as cordas!
        }
    }
    if (fx.stun > 0 && !blocked && gameState == STATE_FIGHTING && target.hp > 0.0f && target.state != FIGHTER_KNOCKDOWN) {
        if (&target == &enemy) { enemy.state = FIGHTER_IDLE; enemy.resetPose(); enemy.actionTimer = fx.stun; }
        else player.startMove(FIGHTER_HIT, std::min(50, fx.stun / 2));
    }
    const Vec3 at = frontOf(target, 60.0f, 0.0f);
    if (fx.slow > 0 || fx.blind > 0 || fx.mud > 0) particles.spawnImpact(at.x, at.y, at.z, 18, 0xa8a29eff);
}

void GameEngine::heal(Fighter& f, float amount) {
    f.hp = std::min(f.maxHp, f.hp + amount);
    combat.spawnFloatingText("+" + std::to_string(static_cast<int>(amount)) + " HP", 0x10b981ff, f.worldPos.x,
                             f.worldPos.y + 90.0f, f.worldPos.z, false, 0);
    Vec3 at = frontOf(f, 70.0f, 0.0f);
    particles.spawnImpact(at.x, at.y, at.z, 14, 0x10b981ff);
}

void GameEngine::tickStatus(Fighter& f, uint32_t& audio) {
    StatusEffects& s = f.status;
    if (s.infested > 0) {
        s.infested--;
        if (s.infested % 20 == 0) {
            f.hp = std::max(0.0f, f.hp - 1.0f);
            Vec3 at = frontOf(f, 40.0f, 0.0f);
            particles.spawnImpact(at.x, at.y + 35.0f, at.z, 8, 0xffd700ff); // Faísca de ferroada de abelha
            combat.spawnFloatingText("-1", 0xffd700ff, f.worldPos.x + (random01() - 0.5f) * 16.0f,
                                     f.worldPos.y + 75.0f, f.worldPos.z, false, 0);
            screenShake = std::max(screenShake, 1.8f);
        }
    }
    if (s.poisoned > 0) {
        s.poisoned--;
        f.hp -= 3.5f / 60.0f;
        Fighter& poisoner = (&f == &player) ? enemy : player;
        if (poisoner.charId == 1 || poisoner.loadout.has(SkillId::FrogPoisonSkin)) {
            poisoner.hp = std::min(poisoner.maxHp, poisoner.hp + (1.5f / 60.0f));
        }
    }
    if (s.bleed > 0) { s.bleed--; f.hp -= 3.0f / 60.0f; }
    if (s.burn > 0) { s.burn--; f.hp -= s.burnDps / 60.0f; }
    for (int* t : {&s.poisonSkin, &s.molasses, &s.slowed, &s.blind, &s.mud, &s.healCooldown, &s.shell,
                   &s.melt, &s.heatwave, &s.seasons, &s.thornVeil, &s.plasma}) {
        if (*t > 0) --*t;
    }
    if (f.hp <= 0.0f && f.state != FIGHTER_KNOCKDOWN) {
        f.hp = 0.0f;
        triggerKnockdown(&f == &enemy, audio);
    }

    // Joana com vida <= 40 e com a passiva Maré das Profundezas: forma monstruosa contínua (maior, carapaça e câmera de personagem grande).
    const bool monstrous = f.charId == 2 && f.hp > 0.0f && f.hp <= 40.0f && f.loadout.has(SkillId::JoanaDeepSeaTide);
    if (monstrous && !f.monstrous) {
        combat.triggerStatusBanner(f.isPlayer ? "¡MODO MONSTRUOSO!" : "¡JOANA: MODO MONSTRUOSO!");
        screenShake = std::max(screenShake, 12.0f);
    }
    f.monstrous = monstrous;
}

bool GameEngine::attackMisses(Fighter& attacker, const Fighter& defender) {
    const bool heat = defender.status.heatwave > 0 && random01() < 0.20f;
    // Maestra Vivaldi - Manto Fantasma: 3 acertos no ritmo concedem 1 esquiva automática garantida
    if (defender.loadout.has(SkillId::VivaldiPhantomCloak) && defender.rhythmStacks >= 3) {
        const_cast<Fighter&>(defender).rhythmStacks -= 3;
        combat.spawnFloatingText("MANTO FANTASMA!", 0xc084fcff, defender.worldPos.x, defender.worldPos.y + 80.0f,
                                 defender.worldPos.z, true, 0);
        return true;
    }
    if (heat) {
        combat.spawnFloatingText("ERROU!", 0xcbd5e1ff, defender.worldPos.x, defender.worldPos.y + 80.0f,
                                 defender.worldPos.z, false, 0);
        return true;
    }
    if (defender.loadout.has(SkillId::BossQuantum) && random01() < 0.35f) {
        combat.triggerStatusBanner("¡ESQUIVA QUÂNTICA (GLITCH DIGITAL)!");
        const Vec3 at = frontOf(defender, 70.0f, 0.0f);
        particles.spawnImpact(at.x, at.y, at.z, 14, 0x818cf8ff);
        return true;
    }
    if (attacker.status.blind > 0) {
        combat.spawnFloatingText("CEGO!", 0xfacc15ff, defender.worldPos.x, defender.worldPos.y + 80.0f, defender.worldPos.z,
                                 false, 0);
        return true;
    }
    return false;
}

float GameEngine::applyStatusDefense(Fighter& defender, Fighter& attacker, float damage) {
    StatusEffects& s = defender.status;
    auto bounce = [&](float amount, const char* label, uint32_t color) {
        attacker.hp = std::max(0.0f, attacker.hp - amount);
        combat.spawnFloatingText("-" + std::to_string(static_cast<int>(amount)) + " " + label, color, attacker.worldPos.x,
                                 attacker.worldPos.y + 95.0f, attacker.worldPos.z, false, 0);
    };
    if (s.honeyShield > 0) {
        s.honeyShield--;
        damage = std::round(damage * 0.40f);
        defender.stamina = std::min(defender.maxStamina, defender.stamina + 10.0f);
        attacker.status.slowed = std::max(attacker.status.slowed, 2 * 60);
        combat.spawnFloatingText("ABSORVIDO!", 0xffd700ff, defender.worldPos.x, defender.worldPos.y + 100.0f, defender.worldPos.z, false, 0);
    }
    if (s.shell > 0) { // Barreira de Cracas: devolve 35% e segura 70%
        bounce(std::max(3.0f, std::round(damage * 0.35f)), "CRACAS", 0x0284c7ff);
        damage = std::round(damage * 0.30f);
    }
    if (s.melt > 0) { // Modo Derretido: metade do dano e 20% vira cura
        damage = std::round(damage * 0.50f);
        float h = std::round(damage * 0.20f);
        if (h > 0.0f) defender.hp = std::min(defender.maxHp, defender.hp + h);
    }
    if (s.thornVeil > 0) damage = std::round(damage * 0.50f);
    if (s.plasma > 0) { // Escudo Sputnik
        damage = std::round(damage * 0.45f);
        defender.stamina = std::min(defender.maxStamina, defender.stamina + 15.0f);
    }
    if (s.heatwave > 0) {
        bounce(3.0f, "CALOR", 0xff4500ff); // queima quem encosta
        attacker.stamina = std::max(0.0f, attacker.stamina - 10.0f); // calor drena o fôlego do agressor
    }
    if (defender.loadout.has(SkillId::EspinhaThornRecoil)) {
        // Espinas de Lucha: 15% volta para o agressor (o dobro com o Velo).
        float recoil = std::max(1.0f, std::round(damage * 0.15f)) * (s.thornVeil > 0 ? 2.0f : 1.0f);
        bounce(recoil, "ESPINHOS", 0x10b981ff);
    }
    if (defender.loadout.has(SkillId::BossDynamo)) { // Dínamo Enfurecido: choque de volta em quem bate
        bounce(std::max(2.0f, std::round(damage * 0.15f)), "CHOQUE", 0x00f5d4ff);
    }
    defender.rhythmStacks = 0; // apanhar quebra o Ritmo Perfeito
    return damage;
}

float GameEngine::resolveDamage(Fighter& attacker, Fighter& defender, float damage, FighterActionState move, bool heavy, bool jab) {
    damage = applyAttackerTraits(attacker, damage, move);
    damage = applyDefenderTraits(defender, damage, combat, slipperyChance, heavy, jab);
    damage = std::round(damage * arch(attacker.charId).dealt * arch(defender.charId).taken); // estilo de cada um (canhão de vidro)
    if (arch(attacker.charId).beePerPunch > 0 && !defender.isDummy()) // enxame: cada soco limpo deixa abelhas no rival, até o teto
        defender.status.infested = std::min(std::max(kBeeCap, defender.status.infested), defender.status.infested + arch(attacker.charId).beePerPunch);
    if (attacker.status.molasses > 0) {
        // Modo Melaço: removeu corte de dano próprio; aplica slow cumulativo no rival a cada golpe
        defender.status.slowed = std::min(6 * 60, defender.status.slowed + 60);
    }
    const bool perfectRhythm = attacker.loadout.has(SkillId::VivaldiPerfectRhythm);
    if (perfectRhythm || attacker.loadout.has(SkillId::VivaldiPhantomCloak)) {
        // Manto Rítmico and Ritmo Perfeito are mutually exclusive passives, but
        // both need the consecutive-hit counter. Previously only the latter
        // accumulated it, leaving Manto's automatic dodge unreachable.
        attacker.rhythmStacks = std::min(5, attacker.rhythmStacks + 1);
        if (perfectRhythm)
            damage = std::round(damage * (1.0f + attacker.rhythmStacks * 0.08f));
    }
    if (attacker.revengeBonus > 1.0f) {
        damage = std::round(damage * attacker.revengeBonus);
        attacker.revengeBonus = 1.0f;
        combat.triggerStatusBanner(attacker.charId == 2 ? "¡CONTRA-GOLPE DA GARRA! (+25% DANO)" : "¡VINGANÇA DOCE! (+15% DANO)");
    }
    damage = applyStatusDefense(defender, attacker, damage);
    if (attacker.hp <= 0.0f && attacker.state != FIGHTER_KNOCKDOWN && gameState == STATE_FIGHTING) {
        uint32_t audio = AUDIO_NONE;
        triggerKnockdown(&attacker == &enemy, audio); // morreu nos espinhos/calor do outro
    }
    return damage;
}

// O que cada lutador ganha ao esquivar de um golpe (Jack e Cremoso já tinham a deles: abelha do Apicultor e Vingança Doce).
void GameEngine::dodgeReward(Fighter& dodger, Fighter& attacker) {
    if (!styleTraits) return;
    const bool mine = dodger.isPlayer;   // a IA esquiva muito mais vezes que o jogador: a recompensa dela é menor
    const auto say = [&](const char* txt, uint32_t color) { combat.spawnFloatingText(txt, color, dodger.worldPos.x, dodger.worldPos.y + 128.0f, dodger.worldPos.z, false, 0); };
    switch (dodger.charId) {
    case 1: // King Frog (armadilheiro): quem erra nele sai lento
        attacker.status.slowed = std::max(attacker.status.slowed, mine ? 120 : 45); say("VISGO!", 0x9be33aff); break;
    case 2: // Joana (pesada): o próximo soco bate +25%
        dodger.revengeBonus = 1.25f; say("GARRA ARMADA!", 0xff4b3eff); break;
    case 4: // Valéria (canhão de vidro): a brasa pega em quem errou
        attacker.status.burn = std::max(attacker.status.burn, mine ? 90 : 60); attacker.status.burnDps = std::max(attacker.status.burnDps, 2.0f); say("BRASA!", 0xff6a14ff); break;
    case 5: // Vivaldi (zoneadora): abre espaço, empurrando o rival
        if (!attacker.loadout.has(SkillId::MarcovZeroGravity) && !trainingMode && attacker.charId != 9) attacker.radius = std::min(235.0f, attacker.radius + 50.0f);
        say("ESPAÇO!", 0xb48cffff); break;
    case 6: // Lady Espinha (retaliadora): quem erra nela se fura
        if (!attacker.isDummy()) attacker.hp = std::max(1.0f, attacker.hp - (mine ? 4.0f : 2.0f));
        combat.spawnFloatingText(mine ? "-4 ESPINHO!" : "-2 ESPINHO!", 0x16a34aff, attacker.worldPos.x, attacker.worldPos.y + 90.0f, attacker.worldPos.z, false, 0); break;
    case 7: // Marcov (controlador): recupera fôlego
        dodger.stamina = std::min(dodger.maxStamina, dodger.stamina + 15.0f); dodger.staminaRegenDelay = 0; say("+FÔLEGO", 0x4fc3ffff); break;
    default: break;
    }
}

bool GameEngine::isAirborne(const Fighter& f) const {
    if (f.state != FIGHTER_SKILL || f.offsetY <= 60.0f) return false;
    if (f.activeSkill == SkillId::FrogRoyalLeap) return true;
    const SkillDef* def = SkillSystem::get(f.activeSkill);
    return def && def->fx.airborne;
}

// ---------------------------------------------------------------------------
// Posições e estado de renderização
// ---------------------------------------------------------------------------

void GameEngine::placeFighters() {
    // Os dois no eixo do ringue, frente a frente; o ring angle gira a dupla e a câmera.
    const float s = std::sin(ringAngle), c = std::cos(ringAngle);
    player.worldPos = {-player.radius * s, 0.0f, -player.radius * c};
    player.facing = ringAngle;
    enemy.worldPos = {enemy.radius * s, 0.0f, enemy.radius * c};
    enemy.facing = ringAngle + kPi;
}

void GameEngine::castSkill(bool isPlayerFighter, const std::string& skillId) {
    // API da ponte WASM: lança pelo id do SKILL_DATABASE do JS.
    const SkillDef* def = SkillSystem::getByKey(skillId);
    if (!def || gameState != STATE_FIGHTING) return;
    uint32_t audio = AUDIO_NONE;
    if (isPlayerFighter) castSkill(player, enemy, def->id, false, audio);
    else castSkill(enemy, player, def->id, false, audio);
}

void GameEngine::reset() {
    init();
}

const GameRenderState* GameEngine::getRenderState() {
    return &renderState;
}

void GameEngine::fillRenderState(uint32_t audioEvents) {
    player.comboCount = comboCount;
    player.chainCount = chain_;
    player.flowOpen = flowWindow_ > 0;
    player.chainNeed = 0;
    for (SkillId id : {player.loadout.spec1, player.loadout.spec2})
        if (const SkillDef* d = SkillSystem::get(id)) player.chainNeed = std::max(player.chainNeed, SkillSystem::chainNeed(*d));
    player.chargeRatio = chargeActive ? chargeRatio() : player.chargeRatio;

    renderState.frame_index = frameIndex;
    renderState.game_state = static_cast<uint32_t>(gameState);
    renderState.match_time = matchTime;
    renderState.ring_angle = ringAngle;
    renderState.cam_pos = camPos;
    renderState.cam_pitch = camPitch;
    renderState.cam_yaw = camYaw;
    renderState.screen_shake = screenShake;
    renderState.audio_events = audioEvents;

    player.fillRenderData(renderState.player);
    enemy.fillRenderData(renderState.enemy);
    particles.fillRenderData(renderState);
    combat.fillRenderData(renderState);
}

} // namespace KoRama
