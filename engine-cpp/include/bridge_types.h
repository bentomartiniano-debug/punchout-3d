#ifndef KORAMA_BRIDGE_TYPES_H
#define KORAMA_BRIDGE_TYPES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_PARTICLES 128
#define MAX_FLOATING_TEXTS 16
#define MAX_NAME_LEN 32
#define MAX_TEXT_LEN 48

// Máscara de Bits de Entrada (Input Bitmask)
typedef enum {
    INPUT_UP        = (1 << 0),
    INPUT_DOWN      = (1 << 1),
    INPUT_LEFT      = (1 << 2),
    INPUT_RIGHT     = (1 << 3),
    INPUT_PUNCH_L   = (1 << 4), // Tecla Z
    INPUT_PUNCH_R   = (1 << 5), // Tecla X
    INPUT_BLOCK     = (1 << 6), // Tecla Espaço / Down
    INPUT_DODGE_L   = (1 << 7),
    INPUT_DODGE_R   = (1 << 8),
    INPUT_DODGE_B   = (1 << 9),
    INPUT_SKILL_1   = (1 << 10),
    INPUT_SKILL_2   = (1 << 11),
    INPUT_DUCK      = (1 << 12),
    INPUT_DODGE_KEY = (1 << 13) // Tecla C / L / Botão dedicado de esquiva
} InputFlags;

// Estados Globais de Jogo
typedef enum {
    STATE_TITLE = 0,
    STATE_CHAR_SELECT,
    STATE_MODE_SELECT,
    STATE_BRACKET,
    STATE_VS_SCREEN,
    STATE_FIGHTING,
    STATE_KNOCKDOWN,
    STATE_EMPEROR_CUTSCENE,
    STATE_VICTORY,
    STATE_GAME_OVER,
    STATE_PAUSED,
    STATE_ROUND_OVER,       // intervalo entre rounds (novo no fim para não mudar os valores antigos)
    STATE_ROUND_INTRO       // encarada pré-luta estilo mangá / face-off dinâmico
} GameStateEnum;

// Estados de Ação do Lutador
typedef enum {
    FIGHTER_IDLE = 0,
    FIGHTER_JAB,
    FIGHTER_HOOK,
    FIGHTER_BODY_BLOW,
    FIGHTER_UPPER,
    FIGHTER_BLOCK,
    FIGHTER_DODGE_L,
    FIGHTER_DODGE_R,
    FIGHTER_DODGE_BACK,
    FIGHTER_DUCK,
    FIGHTER_TELEGRAPH,
    FIGHTER_HIT,
    FIGHTER_KNOCKDOWN,
    FIGHTER_VICTORY_POSE,
    FIGHTER_CROSS,          // direto de direita (novo no fim para não mudar os valores antigos)
    FIGHTER_SKILL           // animação de habilidade especial (qual habilidade fica em Fighter::activeSkill)
} FighterActionState;

// Categorias de Tamanho dos Lutadores
typedef enum {
    FIGHTER_SIZE_NORMAL = 0,
    FIGHTER_SIZE_PEQUENUXO,
    FIGHTER_SIZE_GRANDALHAO
} FighterSizeCategory;

// Eventos de Áudio (Bitmask para a Web Audio API tocar os SFX)
typedef enum {
    AUDIO_NONE              = 0,
    AUDIO_SFX_PUNCH_LIGHT   = (1 << 0),
    AUDIO_SFX_PUNCH_HEAVY   = (1 << 1),
    AUDIO_SFX_PUNCH_CRIT    = (1 << 2),
    AUDIO_SFX_BLOCK         = (1 << 3),
    AUDIO_SFX_DODGE         = (1 << 4),
    AUDIO_SFX_KNOCKDOWN     = (1 << 5),
    AUDIO_SFX_COUNTER_HIT   = (1 << 6),
    AUDIO_SFX_SKILL_ACTIVATE= (1 << 7),
    AUDIO_SFX_SPECIAL_ZAP   = (1 << 8),
    AUDIO_SFX_BLOOD_SPLAT   = (1 << 9),
    AUDIO_SFX_GONG          = (1 << 10)
} AudioEventFlags;

// Vetor 3D
typedef struct {
    float x, y, z;
} Vec3;

// Dados de Renderização de uma Parte do Lutador (Luva/Corpo)
typedef struct {
    float x, y, z;
    float w, h;
    float rot2d;
    float skew;
} PartRenderData;

// Dados de Renderização do Lutador
typedef struct {
    uint32_t char_id; // 0=jack, 1=frog, 2=joana, 3=cremoso, 4=valeria, 5=vivaldi, 6=espinha, 7=marcov, 8=boss
    uint32_t action_state;
    float hp, max_hp;
    float stamina, max_stamina;
    uint32_t is_exhausted;
    uint32_t is_blocking;
    uint32_t is_left_punch;
    float charge_ratio;
    uint32_t combo_count;
    
    // Coordenadas Espaciais no Mundo 3D
    Vec3 world_pos;
    float radius;
    float facing; // Rotação Y
    float offset_x, offset_y, offset_z;
    float body_skew;

    // Partes Corporais (Luva Esquerda, Luva Direita)
    PartRenderData glove_l;
    PartRenderData glove_r;

    // Status / Telegraph
    uint32_t telegraph_flash;
    uint32_t hitstop_timer;
    uint32_t size_category; // FighterSizeCategory: 0=Normal, 1=Pequenuxo, 2=Grandalhão
} FighterRenderData;

// Dados de Renderização de Partículas
typedef struct {
    float x, y, z;
    float vx, vy, vz;
    float size;
    float life; // 1.0 -> 0.0
    uint32_t color_hex; // RGBA em 0xRRGGBBAA
    uint32_t type; // 0=impact, 1=blood, 2=sweat, 3=spark, 4=smoke, 5=ring_stain
} ParticleRenderData;

// Dados de Textos Flutuantes (Damage Popups / Status)
typedef struct {
    float x, y, z;
    float life; // 1.0 -> 0.0
    char text[MAX_TEXT_LEN];
    uint32_t color_hex;
    uint32_t is_crit;
    uint32_t side; // 0=left, 1=right
} FloatingTextData;

// Estrutura Principal de Renderização Compartilhada (Zero-Copy com JS)
typedef struct {
    uint32_t frame_index;
    uint32_t game_state;
    float match_time; // Segundos de luta
    float ring_angle;

    // Lutadores
    FighterRenderData player;
    FighterRenderData enemy;

    // Câmera 3D
    Vec3 cam_pos;
    float cam_pitch, cam_yaw;
    float screen_shake;

    // Partículas
    uint32_t particle_count;
    ParticleRenderData particles[MAX_PARTICLES];

    // Textos Flutuantes
    uint32_t floating_text_count;
    FloatingTextData floating_texts[MAX_FLOATING_TEXTS];

    // Eventos de Áudio Disparados neste Frame
    uint32_t audio_events;

    // Banner de Status da Luta (Ex: "¡CONTRA-ATAQUE!", "¡OVERCLOCK!")
    char status_banner[MAX_TEXT_LEN];
    float status_banner_timer;
} GameRenderState;

#ifdef __cplusplus
}
#endif

#endif // KORAMA_BRIDGE_TYPES_H
