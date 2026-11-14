#ifndef KORAMA_AI_HPP
#define KORAMA_AI_HPP

#include "fighter.hpp"

namespace KoRama {

// O que a IA pede ao motor neste quadro.
struct AiAction {
    bool checkHit = false;            // golpe comum chegou na janela de acerto
    SkillId cast = SkillId::None;     // habilidade a lançar
    bool signature = false;           // golpe-assinatura: animação da habilidade + golpe comum
    float ringTurnDelta = 0.0f;       // rotação angular do ringue durante esquiva do oponente
    bool dodgeSmoke = false;          // disparo de partículas de fumaça sob os pés
    float smokeX = 0.0f;              // coordenada horizontal local da fumaça
};

// IA do oponente portada da classe Enemy do index.html.
class EnemyAI {
public:
    int skillCooldown = 220;
    bool rolledDefenseForPunch = false; // a reação de defesa é sorteada uma vez por soco do jogador
    float defenseChance = 0.80f;        // chance de bloquear/esquivar soco seco no neutro (80% para exigir leitura de brechas)
    float signatureChance = 0.32f;      // chance de trocar o soco comum pelo golpe-assinatura
    bool useSkills = true;
    // Defesa reativa (tudo desligado quando defenseChance é 0, como nos testes determinísticos):
    float tellCancelChance = 0.5f;      // chance de largar o aviso de um golpe comum, ainda no começo, para defender o soco que vem
    float skillDodgeChance = 0.5f;      // chance de esquivar de um especial do jogador na hora do acerto
    float counterChance = 0.45f;        // chance de responder com um golpe rápido depois de segurar ou esquivar um soco
    int lastPunchTimer = 0; bool sawPunch = false;   // para saber quando o jogador começou outro soco (mesmo emendado no anterior)
    bool rolledSkillDodge = false;      // a esquiva do especial é sorteada uma vez por especial
    int defKind = 0, defDelay = 0, defHold = 0;      // defesa marcada: 1 guarda, 2 esquiva; quadros de reação; quanto segurar a guarda
    bool guardSaw = false, dodgeSaw = false;         // a guarda segurou / a esquiva evitou um golpe de verdade
    bool countering = false, inHit = false;          // o golpe atual é um contra-ataque; estava apanhando no quadro anterior
    int sinceDodge = 100000;                         // quadros desde a última esquiva
    int hitsTaken = 0, blocksInRow = 0, calm = 0;    // golpes limpos seguidos que levou; socos seguidos na guarda; quadros sem o jogador atacar
    void planDefense(const Fighter& enemy, const Fighter& player, int hitIn, bool canBlock);
    void startDefense(Fighter& enemy, uint32_t& audioEvents);
    void startCounter(Fighter& enemy, bool heavy);

    // Avança um quadro de luta. FIGHTER_SKILL é animado pelo motor, não aqui.
    AiAction update(Fighter& enemy, const Fighter& player, uint32_t frame, uint32_t& audioEvents);

    // Termina a animação em andamento enquanto o juiz conta (sem checar acertos).
    void finishAction(Fighter& enemy);

    void reset(uint32_t charId);

    // Padrões de luta (docs/padroes_luta.md): sequência de golpes com aviso próprio, brecha no fim.
    struct Step {
        FighterActionState move = FIGHTER_JAB; // FIGHTER_SKILL = lança `skill`
        bool left = true;
        int tell = 36;      // quadros de aviso (0 = emenda sem aviso)
        int tellKind = 0;
        SkillId skill = SkillId::None;
        bool feint = false; // aviso falso: repete o aviso uma vez antes de soltar
    };
    std::vector<Step> queue;
    // combo de entrada de um especial (etapa 3): jab, direto e, nos fortes, gancho; o último passo é o especial. Apanhar ou levar aparo esvazia.
    std::vector<Step> combo;
    AiAction beginSkill(Fighter& enemy, SkillId id);
    AiAction nextCombo(Fighter& enemy);
    int pendingRecovery = 0;  // brecha a abrir quando a sequência terminar
    bool afterSkill = false;  // a sequência estava numa habilidade (o motor anima)
    bool castPoison = false;  // King Frog: pele venenosa na reta final (uma vez)
    bool feintActive = false; // o aviso atual é falso (arma de novo)
    int tellBeats = 1;        // Vivaldi: batidas da batuta = golpes que vêm
    int rockets = 0;          // Marcov: investidas desde o último superaquecimento
    bool hasPattern(const Fighter& enemy) const { return enemy.bossStage == 0 && enemy.charId <= 7; }
    void choosePattern(Fighter& enemy);
    AiAction nextStep(Fighter& enemy);
};

} // namespace KoRama

#endif // KORAMA_AI_HPP
