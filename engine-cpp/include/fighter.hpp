#ifndef KORAMA_FIGHTER_HPP
#define KORAMA_FIGHTER_HPP

#include "bridge_types.h"
#include "skills.hpp"
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>

namespace KoRama {

struct FighterStats {
    int forca = 50;
    int stamina = 50;
    int alcance = 50;
    int resistencia = 50;
};

// Efeitos de habilidades em andamento, em quadros de 60 Hz (exceto honeyShield, em golpes).
struct StatusEffects {
    int infested = 0;      // enxame de abelhas: 3,2 HP/s
    int poisoned = 0;      // veneno: 3,5 HP/s
    int poisonSkin = 0;    // Pele Venenosa ativa (visual)
    int molasses = 0;      // Modo Melaço ativo: dano -50% e socos deixam o alvo lento
    int slowed = 0;        // -35% de velocidade
    int blind = 0;         // -40% de precisão
    int mud = 0;           // preso na lama: não esquiva
    int honeyShield = 0;   // próximos golpes com -60% de dano
    int healCooldown = 0;  // recarga das curas
    int bleed = 0;         // sangramento de coral: 3 HP/s
    int shell = 0;         // Barreira de Cracas: -70% de dano e reflete 35%
    int burn = 0;          // queimadura (calda ou lava)
    float burnDps = 0.0f;
    int melt = 0;          // Modo Derretido: -50% e cura 20% do dano recebido
    int heatwave = 0;      // Escudo de Calor: atacante erra 20% e se queima
    int seasons = 0;       // Quatro Estações: socos 35% mais rápidos e mais baratos
    int thornVeil = 0;     // Velo de Espinas: -50% e espinhos em dobro
    int plasma = 0;        // Escudo Sputnik: -55% e recarga de stamina
};

// Golpes básicos de boxe compartilhados por todos os lutadores.
inline bool isPunchState(FighterActionState s) {
    return s == FIGHTER_JAB || s == FIGHTER_CROSS || s == FIGHTER_HOOK || s == FIGHTER_BODY_BLOW || s == FIGHTER_UPPER;
}

// Lutador (jogador ou oponente). Regras, tempos e poses portados do Player/Enemy do index.html.
class Fighter {
public:
    uint32_t charId = 0; // 0=jack, 1=frog, 2=joana, 3=cremoso, 4=valeria, 5=vivaldi, 6=espinha, 7=marcov, 8=boss
    uint32_t skinId = 0; // 0=padrão, 1=skin 1, 2=skin 2, 3=skin 3
    std::string name;
    bool isPlayer = false;

    // Vida e Estamina
    float hp = 100.0f;
    float maxHp = 100.0f;
    float stamina = 100.0f;
    float maxStamina = 100.0f;
    int staminaRegenDelay = 0;
    bool isExhausted = false;
    bool isSluggish = false;       // Soco com pouca/nenhuma estamina: 50% mais lento e 50% de dano
    int parryWindowTimer = 0;      // Janela ativa (10 quadros = 166ms) para parry perfeito ao bloquear

    // Estado e Timers (em quadros de 60 Hz)
    FighterActionState state = FIGHTER_IDLE;
    int timer = 0;
    int maxTimer = 0;
    bool hasHit = false;
    bool isLeftPunch = false;
    float chargeRatio = 0.0f;
    int comboCount = 0;
    // combo de entrada dos especiais (etapa 3): socos já dados em ordem e quantos o especial mais exigente da build pede (0 = nenhum pede)
    int chainCount = 0, chainNeed = 0;
    bool flowOpen = false;   // esquiva bem feita: os especiais de fluxo saem sem combo por alguns quadros
    int hitstopTimer = 0;
    // Reação visual ao último golpe levado (só o renderer usa): 0 nenhuma/bloqueio, 1 rosto leve,
    // 2 gancho/cruzado (vira o rosto), 3 corpo (mão na barriga), 4 uppercut. hitDir: lado da tela para onde a cabeça vai.
    int hitKind = 0;
    int hitDir = 1;

    // Transformações 3D e Posição (worldPos e facing vêm do raio + ring angle, calculados pelo motor)
    Vec3 worldPos = {0.0f, 0.0f, 0.0f};
    float radius = 125.0f;
    float facing = 0.0f;
    float offsetX = 0.0f, offsetY = 0.0f, offsetZ = 0.0f;
    float bodySkew = 0.0f;

    // Partes Corporais
    PartRenderData baseGloveL = {-32.0f, 64.0f, 12.0f, 54.0f, 54.0f, 0.0f, 0.0f};
    PartRenderData baseGloveR = { 32.0f, 64.0f, 12.0f, 54.0f, 54.0f, 0.0f, 0.0f};
    PartRenderData gloveL;
    PartRenderData gloveR;

    // IA / Telegraph
    bool telegraphFlash = false;
    int actionTimer = 70;
    FighterActionState queuedAttack = FIGHTER_JAB;
    bool queuedHeavy = false;   // cross, gancho e uppercut do oponente contam como pesados
    int tellKind = 0;           // aviso próprio do golpe (padrões de luta; 0 = aviso comum)
    int vulnerable = 0;         // brecha: quadros em que o oponente está aberto (+25% de dano, não defende)
    bool dodgeLock = false;
    bool spawnedDodgeSmoke = false;

    // Habilidades
    SkillLoadout loadout;
    StatusEffects status;
    SkillId activeSkill = SkillId::None;   // habilidade sendo animada em FIGHTER_SKILL
    SkillId queuedSkill = SkillId::None;   // especial do oponente sendo armado em FIGHTER_TELEGRAPH (sai quando o aviso acaba)
    bool queuedSignature = false;          // o especial armado é um golpe-assinatura (dano de soco comum; avisa como soco, sem marca no chão)
    bool signatureOnly = false;            // golpe-assinatura da IA: só animação + golpe comum
    bool skillEventDone = false;           // efeito principal da habilidade já aplicado
    int skillHitsDone = 0;                 // golpes rítmicos já aplicados (Cadência Staccato etc.)
    int rhythmStacks = 0;                  // Ritmo Perfeito da Vivaldi (até 5)
    float revengeBonus = 1.0f;             // Vingança Doce do Cremoso: próximo soco +15%
    bool coldMagma = false;                // Valéria em Magma Frio (depois da 2ª queda)
    bool internalIgnitionUsed = false;     // Valéria: Ignição Interna (1x por round)
    bool zeroGravityUsed = false;          // Gravidade Zero já salvou uma queda neste round
    bool monstrous = false;                // Joana com vida <= 40: forma monstruosa (maior e mais forte visualmente)
    int slipperyProcs = 0;                 // vezes que a pele mucosa absorveu golpes (efeito visual)
    int bossStage = 0;                     // Batalhator: 1 robô, 2 robô danificado, 3 O Nerd Operador (0 = não é o chefe)

    // Stats
    FighterStats stats;

    Fighter(uint32_t id = 0, bool player = false);

    void initCharacter(uint32_t id, bool asPlayer);
    void resetPose();
    void resetParts();
    FighterSizeCategory sizeCategory() const;
    bool isBigChar() const;        // Grandalhão
    bool isSmallChar() const;      // Pequenuxo
    bool isNormalChar() const;     // Tamanho normal
    bool isDummy() const { return charId == 9; } // O Espantalho (Modo Treino)
    const char* sizeCategoryName() const;
    float idleRadius() const;      // distância de descanso do centro do ringue
    float guardGloveY() const;     // altura da luva de guarda usada nas animações de soco

    void startMove(FighterActionState move, int duration);

    // Pose do soco/esquiva/bloqueio no progresso atual (sem efeitos de jogo).
    void animateCommonPunch(FighterActionState move, float p, bool left);
    void animateChargingPose(bool left, float ratio, float jitterX, float jitterY, float jitterZ);
    void applyIdleBob(uint32_t frame);

    void fillRenderData(FighterRenderData& data) const;
};

// Os especiais duram kSkillStretch vezes o tempo da ficha (moveFrames), para dar para ver cada um; os do chefe ficam como estão.
// Os tempos escritos em quadros (no motor e nos efeitos do app3d) continuam na escala da ficha: use skillElapsed em vez de maxTimer - timer.
constexpr float kSkillStretch = 1.7f;
inline float skillStretch(uint32_t charId) { return charId == 8 ? 1.0f : kSkillStretch; }
inline float skillElapsed(const Fighter& f) { return static_cast<float>(f.maxTimer - f.timer) / skillStretch(f.charId); }

} // namespace KoRama

#endif // KORAMA_FIGHTER_HPP
