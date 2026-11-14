#ifndef KORAMA_ENGINE_HPP
#define KORAMA_ENGINE_HPP

#include "bridge_types.h"
#include "fighter.hpp"
#include "combat.hpp"
#include "particles.hpp"
#include "ai.hpp"
#include "skills.hpp"
#include "sound_cues.hpp"
#include <string>
#include <vector>

namespace KoRama {

enum class MatchResult {
    None,
    WinKO,          // oponente não levantou até 10
    WinTKO,         // 3 quedas do oponente no mesmo round
    WinDecision,    // mais vida ao fim do 3º round
    LossKO,
    LossTKO,
    LossDecision,
    Draw,
};

// Partida de boxe completa: entrada do jogador, IA, acertos, quedas com contagem do juiz,
// rounds, combo e resultado. Regras portadas do jogo JS original (removido na fase 7; ver histórico do git).
class GameEngine {
public:
    static constexpr int kRoundFrames = 180 * 60;   // 3:00 por round
    static constexpr int kRounds = 3;
    static constexpr int kKnockdownsForTko = 3;
    static constexpr int kMaxChargeFrames = 72;     // 1,2 s para carga total
    static constexpr int kChargeAutoRelease = 24;   // solta sozinho 0,4 s depois de cheio
    static constexpr int kBufferFrames = 19;        // soco apertado durante outra ação vale por 320 ms
    static constexpr int kDoubleTapFrames = 17;     // ↓↓ em menos de 280 ms = esquiva para trás
    static constexpr int kComboFrames = 150;        // ~2,5 s para manter o combo
    static constexpr int kRoundOverFrames = 210;    // pausa de 3,5 s entre rounds

    uint32_t frameIndex = 0;
    // Nocaute final: quadro do fim da luta e quadro em que o perdedor bateu nas cordas (0 = ainda não).
    // Ficam no estado para o voo do perdedor sair igual ao vivo e no replay.
    uint32_t matchEndFrame = 0, koRopeFrame = 0;
    GameStateEnum gameState = STATE_TITLE;
    float matchTime = 0.0f;
    float ringAngle = 0.0f;

    Fighter player;
    Fighter enemy;

    CombatSystem combat;
    ParticleManager particles;
    EnemyAI ai;

    // Câmera & Shake
    Vec3 camPos = {0.0f, 220.0f, -260.0f};
    float camPitch = 0.45f;
    float camYaw = 0.0f;
    float screenShake = 0.0f;

    // Modo de Jogo & Torneio
    bool isArcadeMode = false;
    int arcadeStage = 1;
    bool isFinalBoss = false;
    bool trainingMode = false;

    // Rounds, quedas e juiz
    int round = 1;
    int roundFrames = kRoundFrames;
    int roundOverTimer = 0;
    int playerKnockdowns = 0;
    int enemyKnockdowns = 0;
    bool enemyIsDown = false;    // quem está no chão durante STATE_KNOCKDOWN
    int refereeCount = 0;
    int refereeTimer = 0;
    int mashProgress = 0;
    int mashTarget = 0;
    MatchResult result = MatchResult::None;
    float playerDamageTaken = 0.0f;
    float prevPlayerHp_ = 100.0f;

    // Combo do jogador
    int comboCount = 0;
    int comboTimer = 0;
    int bestCombo = 0;

    // Soco carregado
    bool chargeActive = false;
    bool chargeLeft = false;
    int chargeFrames = 0;

    int hitstopTimer = 0;
    int roundIntroDuration = 324;
    int roundIntroTimer = 0;   // quadros da encarada estilo gibi antes do gongo
    // Impacto (só visual): onomatopeia no ponto do soco e tela vermelha quando o jogador apanha.
    int impactTimer = 0;       // quadros restantes da onomatopeia
    int impactKind = 0;        // 0 leve, 1 pesado, 2 contra-ataque, 3 brecha, 4 queda
    int impactSeed = 0;
    Vec3 impactAt{};
    int hurtTimer = 0;         // borda vermelha quando o jogador leva um golpe
    float slipperyChance = 0.20f; // Pele Mucosa do King Frog (os testes zeram para ter dano exato)

    GameRenderState renderState;
    std::vector<SoundCue> sounds; // sons pedidos no último update (o app toca e o motor limpa no próximo)

    GameEngine();

    void init();
    // playerLoadout nulo = build padrão do lutador; o oponente sempre recebe uma build sorteada.
    void startMatch(uint32_t playerChar, uint32_t enemyChar, bool arcade, int stage, const SkillLoadout* playerLoadout = nullptr, bool training = false);
    void startRoundIntro(int frames = 324);
    // inputMask = teclas seguradas neste quadro (InputFlags); bordas de aperto/soltura são detectadas aqui.
    void update(uint32_t inputMask, float dt);
    void castSkill(bool isPlayer, const std::string& skillId);
    void reset();

    float chargeRatio() const;
    bool isMatchOver() const { return result != MatchResult::None; }
    bool isPerfectVictory() const;

    const GameRenderState* getRenderState();

private:
    void updateFighting(uint32_t held, uint32_t pressed, uint32_t released, uint32_t& audio);
    void updateKnockdown(uint32_t pressed, uint32_t& audio);
    void updatePlayer(uint32_t held, uint32_t& audio);
    void playerMovementAndBlock(uint32_t held, uint32_t& audio);
    void playerHandleAction(uint32_t held, uint32_t& audio, bool allowHits);
    void executePunch(bool left, float charge, uint32_t held);
    void releaseCharge(uint32_t held);
    void cancelCharge();

    void checkHitOnEnemy(FighterActionState move, float charge, uint32_t& audio);
    void checkHitOnPlayer(bool heavy, uint32_t& audio);
    void triggerKnockdown(bool enemyDown, uint32_t& audio);
    void handleReferee(uint32_t pressed, uint32_t& audio);
    void recover(bool enemyRecovers);
    void startNextRound(uint32_t& audio);
    void endMatch(MatchResult outcome, uint32_t& audio);
    void registerComboHit();

    // Habilidades
    bool tryBufferedCommand(uint32_t held, uint32_t& audio);
    bool tryPlayerCommand(uint32_t held, uint32_t& audio);
    bool commandEntered(SkillCommand command, uint32_t held) const;
    bool finisherEntered(const SkillDef& def, uint32_t held) const;   // seta + soco que fecha o combo de um especial
    bool chainReady(SkillEntry entry) const;
    bool castSkill(Fighter& caster, Fighter& target, SkillId id, bool signature, uint32_t& audio);
    void updateSkillMove(Fighter& caster, Fighter& target, uint32_t& audio, bool allowEffects);
    bool skillDamage(Fighter& caster, Fighter& target, float damage, const char* label, uint32_t& audio);
    void heal(Fighter& f, float amount);
    void tickStatus(Fighter& f, uint32_t& audio);
    bool attackMisses(Fighter& attacker, const Fighter& defender);
    float applyStatusDefense(Fighter& defender, Fighter& attacker, float damage);
    // Todos os modificadores de um golpe (características, modos, passivas) para qualquer lado da luta.
    float resolveDamage(Fighter& attacker, Fighter& defender, float damage, FighterActionState move, bool heavy, bool jab);
    bool isAirborne(const Fighter& f) const;
    void applySkillEffect(Fighter& caster, Fighter& target, const SkillEffect& fx, float damage, uint32_t& audio);
    std::string skillBanner(const Fighter& caster, SkillId id) const;
    void resetCombo();
    void cue(Sfx id, int param = 0) { sounds.push_back({id, param}); }

    void placeFighters();
    void fillRenderState(uint32_t audioEvents);

    uint32_t prevInput_ = 0;
    int framesSinceDownTap_ = 1000;
    bool pendingDodgeBack_ = false;
    int bufferedHand_ = -1; // -1 nenhum, 0 esquerda, 1 direita
    // combo de entrada dos especiais: jab, direto e gancho, nessa ordem (PLANO-COMBOS-E-ESQUIVA.md, etapa 3)
    int chain_ = 0;        // socos do combo já dados
    uint32_t chainSeen_ = 0;   // quais socos (golpe + mão) já entraram no combo
    int chainTimer_ = 0;   // quadros até o combo esfriar
    int flowWindow_ = 0;   // depois de uma esquiva bem feita, os especiais de fluxo saem sem combo
    int bufferedAge_ = 0;
    uint32_t bufferedDodge_ = 0;
    int bufferedDodgeAge_ = 0;

    struct InputEvent {
        uint32_t bit;
        uint32_t frame;
    };
    std::vector<InputEvent> commandHistory_; // apertos dos últimos 1,2 s, para comandos especiais
    bool healBannerShown_ = false;
};

} // namespace KoRama

#endif // KORAMA_ENGINE_HPP
