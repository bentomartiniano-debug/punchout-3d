#ifndef KORAMA_SOUND_CUES_HPP
#define KORAMA_SOUND_CUES_HPP

#include <cstdint>

namespace KoRama {

// Sons do jogo, um por função do SoundFX do JS (src/graphics/sprites.js). O motor só emite os
// pedidos; quem toca (e sintetiza) é o app.
enum class Sfx : uint8_t {
    PunchLight, PunchHeavy,       // param = carga 0..10
    Block, Dodge, ChargeTick,     // ChargeTick: param = carga 0..10
    Bell, ComboHit,               // ComboHit: param = tamanho do combo
    Impact, Zap, Rebuild,
    CrowdCheer, CrowdBoo, CrowdGasp,
    RefereeCount,                 // param = número da contagem
    Whip, Slurp, ClawSnap, HarpoonChain, SyrupBoil, CreamWhip,
    SawBuzz, ReactorPulse, EmpShock, MissileBarrage, ArcadeCombo, ApocalypseLaser,
    RocketThrust, OrbitalLaser, PlasmaShield, CactusSlap, LuchaImpact, SpikePierce,
    StaccatoViolin, WinterViolin, Crescendo, MagmaExplosion, VolcanoEruption,
    Crunch, TidalWave, Geyser,
    Select, MenuNav,
    Parry,
    AnnouncerKorama,
    CrowdVictory,
    CrowdCount,
    CrowdImpact,
    CrowdWoah,
    CameraFlash,
    Rewind,
    VersusWhoosh,
    VersusClash,
    VersusSlam,
    Count
};

struct SoundCue {
    Sfx id;
    int param = 0;
};

} // namespace KoRama

#endif // KORAMA_SOUND_CUES_HPP
