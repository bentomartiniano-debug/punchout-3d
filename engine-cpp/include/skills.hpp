#ifndef KORAMA_SKILLS_HPP
#define KORAMA_SKILLS_HPP

#include "bridge_types.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace KoRama {

enum class SkillType { Command, Mode, Passive };

// Sequência de teclas que dispara a habilidade (histórico de 1,2 s, como no JS).
enum class SkillCommand {
    None,           // passiva
    BackForward,    // ← → + soco  (→ → + soco também vale)
    DownUp,         // ↓ ↑ + soco
    BothPunches,    // Z + X juntos
    UpUpRight,      // ↑ ↑ + X
    DownDownLeft,   // ↓ ↓ + Z
    BackBackLeft,   // ← ← + Z
    ForwardForwardRight, // → → + X
    BackForwardLeft,     // ← → + Z
};

enum class SkillId : uint8_t {
    None = 0,
    // Jack
    JackInfestation, JackMolasses, JackHoneyWall, JackStingJab, JackCombBurst, JackRoyalJelly,
    JackBeekeeper, JackHoneySkin, JackQueenBlessing,
    // King Frog
    FrogTongue, FrogPoisonSkin, FrogRoyalLeap, FrogMudSlime, FrogFlyCatcher, FrogSwampSpout,
    FrogJumper, FrogSlipperySkin, FrogAmphibianRage,
    // Joana
    JoanaClawCrush, JoanaHookPull, JoanaCrustaceanShell, JoanaCoralSpout, JoanaAnchorSlam, JoanaOceanBounty,
    JoanaAsymmetricArms, JoanaCalcifiedArmor, JoanaDeepSeaTide,
    // O Cremoso
    CremosoSyrupBlast, CremosoVanillaWhip, CremosoMeltMode, CremosoConePunch, CremosoPuddleTrap, CremosoCherryTopping,
    CremosoCreamCushion, CremosoFrozenStamina, CremosoSweetRevenge,
    // Valéria
    ValeriaMagmaPunch, ValeriaVolcanicErupt, ValeriaHeatwaveGuard, ValeriaLavaKickJab, ValeriaAshCloud, ValeriaPhoenixEmber,
    ValeriaEmberDance, ValeriaColdMagma, ValeriaVolcanicRage,
    // Maestra Vivaldi
    VivaldiCadenceStaccato, VivaldiWinterAllegro, VivaldiFourSeasons, VivaldiCrescendoJab, VivaldiAutumnRequiem, VivaldiSpringSonata,
    VivaldiPerfectRhythm, VivaldiPhantomCloak, VivaldiSymphonicResonance,
    // Lady Espinha
    EspinhaNopalSlap, EspinhaLuchaFlyer, EspinhaThornVeil, EspinhaNeedleJab, EspinhaDesertWind, EspinhaFridaTears,
    EspinhaThornRecoil, EspinhaRebelSpirit, EspinhaVivaCaatinga,
    // Dr. Marcov
    MarcovRocketThrust, MarcovOrbitalLaser, MarcovPlasmaBarrier, MarcovKineticOverdrive, MarcovExhaustCloud, MarcovOxygenRecharge,
    MarcovZeroGravity, MarcovThrusterCharge, MarcovTitaniumSuit,
    // Batalhator 2000 (chefe): dois especiais e uma passiva por fase
    BossDualSaw, BossOverclock, BossTitanium, BossEmp, BossMissiles, BossDynamo, BossHolo, BossApocalypse, BossQuantum,
    Count
};

// Modos temporários (Z + X) dos lutadores com efeito declarativo.
enum class SkillMode { None, Melt, Heatwave, Seasons, ThornVeil, Plasma };

// Efeito declarativo de uma habilidade. Jack, King Frog e Joana têm lógica própria no motor;
// os demais lutadores são descritos só por esta ficha, aplicada no quadro "at" da animação.
struct SkillEffect {
    float damage = 0.0f;       // dano limpo (ignora a guarda)
    float at = 0.45f;          // progresso da animação em que o golpe acerta
    int hits = 1;              // golpes rítmicos: o dano é dividido entre eles
    int stun = 0;              // quadros de atordoamento no alvo
    int slow = 0;              // quadros de lentidão
    int burn = 0;              // quadros de queimadura
    float burnDps = 0.0f;
    int blind = 0;             // quadros de cegueira
    int bleed = 0;             // quadros de sangramento (3 HP/s)
    int mud = 0;               // quadros sem esquiva
    float staminaDrain = 0.0f;
    float push = 0.0f;         // empurra o alvo para trás
    float heal = 0.0f;         // cura imediata (com recarga de 8 s)
    SkillMode mode = SkillMode::None;
    int modeFrames = 0;
    bool airborne = false;     // salta: invulnerável no ar, como o Salto Real
};

struct SkillDef {
    SkillId id;
    const char* key;      // id do SKILL_DATABASE do JS
    uint32_t charId;
    const char* name;
    SkillType type;
    SkillCommand command;
    const char* tag;      // etiqueta do HUD (SWARM, HONEY, ...)
    const char* desc;
    int moveFrames;       // duração da animação de quem lança (0 = passiva)
    SkillEffect fx{};     // efeito declarativo (Cremoso, Valéria, Vivaldi, Espinha, Marcov)
};

// Três espaços equipados: dois especiais (comando ou modo) e uma passiva.
struct SkillLoadout {
    SkillId spec1 = SkillId::None;
    SkillId spec2 = SkillId::None;
    SkillId passive = SkillId::None;

    bool has(SkillId id) const { return id != SkillId::None && (spec1 == id || spec2 == id || passive == id); }
    // allSpecials: os 6 especiais do lutador valem na luta (jogo 3D); a passiva continua sendo só uma
    bool allSpecials = false;
    bool canCast(SkillId id) const;
};

// Como o especial entra na luta (PLANO-COMBOS-E-ESQUIVA.md, etapa 3): nenhum golpe forte sai do nada.
//  Quick: curas, modos e escudos. Só mexem em quem lança, então saem na hora pelo comando simples de sempre.
//  Flow:  debuffs e dano passivo. Pedem ritmo: jab e direto antes (ou uma esquiva bem feita), e então o finalizador.
//  Full:  golpes fortes. Pedem o combo inteiro (jab, direto, gancho), e então o finalizador. O combo é o aviso.
enum class SkillEntry { Quick, Flow, Full };
// Finalizador: uma direção segurada mais um soco, tirado da última tecla do comando antigo de cada especial.
struct SkillFinisher { uint32_t dir = 0; bool z = false, x = false, both = false; };   // dir: INPUT_UP / INPUT_DOWN / INPUT_RIGHT (0 = nenhuma); both: Z e X juntos
// Peças do comando para desenhar no HUD e nas telas (a ordem é a mesma dos desenhos de menu_draw.cpp e fight_hud.cpp)
enum class CmdTok : uint8_t { Left, Right, Up, Down, Plus, PunchL, PunchR, Chain2, Chain3 };

class SkillSystem {
public:
    static SkillEntry entryOf(const SkillDef& def);
    static int chainNeed(const SkillDef& def);             // socos do combo antes do finalizador: 0, 2 ou 3
    static SkillFinisher finisherOf(const SkillDef& def);
    static std::vector<CmdTok> commandTokens(const SkillDef& def);
    static void init(); // mantido para a ponte WASM; a tabela é estática
    static const SkillDef* get(SkillId id);
    static const SkillDef* getByKey(const std::string& key);
    static const std::array<SkillDef, static_cast<size_t>(SkillId::Count) - 1>& all();

    // Build padrão do jogo JS (getDefaultProgression).
    static SkillLoadout defaultLoadout(uint32_t charId);
    // Build aleatória do oponente (setupEnemyBuild): 2 especiais distintos + 1 passiva.
    static SkillLoadout randomLoadout(uint32_t charId);
    // Build do Batalhator em cada fase (1 robô, 2 robô danificado, 3 O Nerd Operador).
    static SkillLoadout bossLoadout(int stage);
    static bool hasSkills(uint32_t charId);
};

} // namespace KoRama

#endif // KORAMA_SKILLS_HPP
