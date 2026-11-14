#include "fighter.hpp"
#include <cmath>

namespace KoRama {

namespace {
constexpr float kPi = 3.14159265f;
}

Fighter::Fighter(uint32_t id, bool player) {
    initCharacter(id, player);
}

void Fighter::initCharacter(uint32_t id, bool asPlayer) {
    charId = id;
    isPlayer = asPlayer;
    hp = maxHp = 100.0f;
    stamina = maxStamina = 100.0f;
    staminaRegenDelay = 0;
    isExhausted = false;
    isSluggish = false;
    parryWindowTimer = 0;
    state = FIGHTER_IDLE;
    timer = maxTimer = 0;
    comboCount = 0;
    chargeRatio = 0.0f;
    hasHit = false;
    telegraphFlash = false;
    dodgeLock = false;
    actionTimer = 70;
    status = StatusEffects{};
    activeSkill = SkillId::None;
    signatureOnly = false;
    skillEventDone = false;
    monstrous = false;
    skillHitsDone = 0;
    rhythmStacks = 0;
    revengeBonus = 1.0f;
    coldMagma = false;
    internalIgnitionUsed = false;
    zeroGravityUsed = false;
    bossStage = id == 8 ? 1 : 0;

    // Luvas base de applyCharacterConfig() no index.html.
    float x = 35.0f, y = 68.0f, z = 15.0f, size = 58.0f;
    if (id == 0) { x = 32.0f; y = 64.0f; z = 12.0f; size = 54.0f; }       // Jack
    else if (id == 1) { x = 42.0f; y = 65.0f; z = 0.0f; size = 85.0f; }   // King Frog
    else if (id == 3) { size = 60.0f; }                                   // O Cremoso
    else if (id == 7) { x = 36.0f; size = 60.0f; }                       // Dr. Marcov
    else if (id == 8) { x = 44.0f; y = 72.0f; z = 0.0f; size = 98.0f; }   // Batalhator
    baseGloveL = {-x, y, z, size, size, 0.0f, 0.0f};
    baseGloveR = { x, y, z, size, size, 0.0f, 0.0f};

    radius = idleRadius();
    facing = isPlayer ? 0.0f : kPi;
    resetPose();
}

FighterSizeCategory Fighter::sizeCategory() const {
    // Grandalhões: Cremoso, King Frog, Joana Monstro, Batalhator nas 2 primeiras formas
    if (charId == 1 || charId == 3 || (charId == 2 && monstrous) || (charId == 8 && bossStage < 3)) {
        return FIGHTER_SIZE_GRANDALHAO;
    }
    // Pequenuxos: Jack Mãos de Colmeia, O Nerd Operador (estágio 3 do Batalhator)
    if (charId == 0 || (charId == 8 && bossStage == 3)) {
        return FIGHTER_SIZE_PEQUENUXO;
    }
    // Tamanho Normal: os demais
    return FIGHTER_SIZE_NORMAL;
}

bool Fighter::isBigChar() const {
    return sizeCategory() == FIGHTER_SIZE_GRANDALHAO;
}

bool Fighter::isSmallChar() const {
    return sizeCategory() == FIGHTER_SIZE_PEQUENUXO;
}

bool Fighter::isNormalChar() const {
    return sizeCategory() == FIGHTER_SIZE_NORMAL;
}

const char* Fighter::sizeCategoryName() const {
    switch (sizeCategory()) {
    case FIGHTER_SIZE_GRANDALHAO: return "GRANDALHÃO";
    case FIGHTER_SIZE_PEQUENUXO:  return "PEQUENUXO";
    case FIGHTER_SIZE_NORMAL:
    default:                      return "NORMAL";
    }
}

float Fighter::idleRadius() const {
    if (!isPlayer) return 50.0f; // o oponente avança até o meio do ringue
    switch (sizeCategory()) {
    case FIGHTER_SIZE_GRANDALHAO: return 138.0f;
    case FIGHTER_SIZE_PEQUENUXO:  return 115.0f;
    case FIGHTER_SIZE_NORMAL:
    default:                      return 125.0f;
    }
}

float Fighter::guardGloveY() const {
    if (charId == 1) return 38.0f;
    if (charId == 2 || charId == 3 || charId == 4) return 68.0f;
    return 70.0f;
}

void Fighter::resetPose() {
    offsetX = offsetY = offsetZ = 0.0f;
    bodySkew = 0.0f;
    resetParts();
}

void Fighter::resetParts() {
    gloveL = baseGloveL;
    gloveR = baseGloveR;
}

void Fighter::startMove(FighterActionState move, int duration) {
    if (move == FIGHTER_HIT) hitKind = 2; // padrão (habilidades); os socos comuns ajustam logo depois
    if (move != FIGHTER_SKILL) signatureOnly = false; // assinatura interrompida não pode sobreviver a acerto/defesa
    state = move;
    timer = duration;
    maxTimer = duration;
    hasHit = false;
    spawnedDodgeSmoke = false;
    parryWindowTimer = (move == FIGHTER_BLOCK) ? 10 : 0;
}

void Fighter::applyIdleBob(uint32_t frame) {
    float t = frame / 10.8f; // Date.now() / 180 a 60 FPS
    float bob = std::sin(t) * 3.0f;
    // Gingado próprio: frevo da Valéria, flutuação fantasma da Vivaldi, gravidade zero do Marcov.
    float targetX = 0.0f, targetY = bob, targetSkew = 0.0f;
    if (charId == 4) {
        float ft = frame / 9.0f;
        targetX = std::sin(ft) * 14.0f;
        targetY = -std::fabs(std::sin(ft * 2.0f)) * 8.0f;
        targetSkew = std::cos(ft) * 0.12f;
    } else if (charId == 5) {
        float gt = frame / 12.0f;
        targetX = std::sin(gt * 0.6f) * 6.0f;
        targetY = std::sin(gt) * 10.0f;
        targetSkew = std::cos(gt * 0.8f) * 0.06f;
    } else if (charId == 8) {
        float mt = frame / 6.0f; // balanço mecânico do Batalhator
        targetX = std::sin(mt * 0.5f) * 5.0f;
        targetY = std::cos(mt) * 6.0f - 3.0f;
        targetSkew = std::sin(mt * 0.3f) * 0.05f;
    } else if (charId == 7) {
        float zt = frame / 10.8f;
        targetX = std::sin(zt * 0.7f) * 8.0f;
        targetY = std::cos(zt) * 6.0f - 3.0f;
        targetSkew = std::sin(zt * 0.5f) * 0.06f;
    }
    offsetX += (targetX - offsetX) * 0.2f;
    bodySkew += (targetSkew - bodySkew) * 0.2f;
    offsetY += (targetY - offsetY) * 0.2f;
    gloveL.y = baseGloveL.y + bob + std::cos(t) * 2.0f;
    gloveR.y = baseGloveR.y + bob - std::cos(t) * 2.0f;
}

void Fighter::animateChargingPose(bool left, float ratio, float jitterX, float jitterY, float jitterZ) {
    PartRenderData& gl = left ? gloveL : gloveR;
    const PartRenderData& base = left ? baseGloveL : baseGloveR;
    float sign = left ? -1.0f : 1.0f;
    // Puxa a luva para trás na altura da costela e treme mais quanto maior a carga.
    gl.z = base.z - (10.0f + ratio * 16.0f);
    gl.y = 56.0f - ratio * 8.0f;
    gl.x = sign * (35.0f + ratio * 8.0f);
    bodySkew = -sign * (0.08f + ratio * 0.16f);
    offsetY = -2.0f * ratio;
    offsetZ = -4.0f * ratio;
    float shake = 1.0f + ratio * 3.5f;
    gl.x += jitterX * shake;
    gl.y += jitterY * shake;
    gl.z += jitterZ * shake;
}

void Fighter::animateCommonPunch(FighterActionState move, float p, bool left) {
    PartRenderData& gl = left ? gloveL : gloveR;
    PartRenderData& other = left ? gloveR : gloveL;
    const float sign = left ? -1.0f : 1.0f;
    const float baseY = guardGloveY();

    other.x = -sign * (charId == 1 ? 20.0f : 14.0f);
    other.y = baseY + 4.0f;
    other.z = 10.0f;

    auto prepOf = [p](float until) { return std::sin((p / until) * kPi * 0.5f); };
    auto swingOf = [p](float from) { return std::sin(((p - from) / (1.0f - from)) * kPi); };

    switch (move) {
    case FIGHTER_JAB:
        if (p < 0.18f) {
            float prep = prepOf(0.18f);
            gl.z -= 14.0f * prep;
            gl.y = baseY - 6.0f * prep;
            bodySkew = -sign * 0.08f * prep;
        } else {
            float s = swingOf(0.18f);
            gl.x = (left ? -20.0f : 20.0f) + (left ? 14.0f : -14.0f) * s;
            gl.z += 175.0f * s;
            gl.y = baseY + 46.0f * s;
            bodySkew = sign * 0.28f * s;
            offsetX = sign * 8.0f * s;
            offsetZ = 20.0f * s;
        }
        break;
    case FIGHTER_CROSS:
        if (p < 0.20f) {
            float prep = prepOf(0.20f);
            gl.z -= 18.0f * prep;
            gl.y = baseY - 6.0f * prep;
            bodySkew = -sign * 0.12f * prep;
        } else {
            float s = swingOf(0.20f);
            gl.x = (left ? -20.0f : 20.0f) + (left ? 26.0f : -26.0f) * s;
            gl.z += 190.0f * s;
            gl.y = baseY + 50.0f * s;
            bodySkew = sign * 0.38f * s;
            offsetX = sign * 14.0f * s;
            offsetZ = 28.0f * s;
        }
        break;
    case FIGHTER_HOOK: {
        float startX = left ? -52.0f : 52.0f, endX = left ? 28.0f : -28.0f;
        if (p < 0.22f) {
            float prep = prepOf(0.22f);
            gl.x = startX + (left ? -16.0f : 16.0f) * prep;
            gl.z -= 12.0f * prep;
            bodySkew = (left ? -0.15f : 0.15f) * prep;
        } else {
            float strikeP = (p - 0.22f) / 0.78f;
            float s = std::sin(strikeP * kPi);
            gl.x = startX + (endX - startX) * std::min(1.0f, strikeP * 1.3f);
            const float reachZ = isBigChar() ? 180.0f : 150.0f;
            const float fwdZ = isBigChar() ? 26.0f : 18.0f;
            const float hookLift = (charId == 1) ? 56.0f : 36.0f;
            gl.z += reachZ * s;
            gl.y = baseY + hookLift * s;
            bodySkew = (left ? 1.0f : -1.0f) * 0.45f * s;
            offsetX = (left ? 16.0f : -16.0f) * s;
            offsetZ = fwdZ * s;
        }
        break;
    }
    case FIGHTER_UPPER:
        if (p < 0.24f) {
            float prep = prepOf(0.24f);
            gl.y = baseY - 28.0f * prep;
            gl.z -= 16.0f * prep;
            gl.x = left ? -16.0f : 16.0f;
            bodySkew = sign * 0.15f * prep;
            offsetY = -12.0f * prep;
        } else {
            float s = swingOf(0.24f);
            gl.x = (left ? -10.0f : 10.0f) + (left ? 6.0f : -6.0f) * s;
            const float reachZ = isBigChar() ? 175.0f : 145.0f;
            const float fwdZ = isBigChar() ? 26.0f : 18.0f;
            const float upperLift = (charId == 1) ? 120.0f : 95.0f;
            gl.z += reachZ * s;
            gl.y = baseY + upperLift * s;
            bodySkew = -sign * 0.35f * s;
            offsetY = 16.0f * s;
            offsetZ = fwdZ * s;
        }
        break;
    case FIGHTER_BODY_BLOW:
        if (p < 0.22f) {
            float prep = prepOf(0.22f);
            gl.y = baseY - 16.0f * prep;
            gl.z -= 14.0f * prep;
            bodySkew = -sign * 0.10f * prep;
            offsetY = -8.0f * prep;
        } else {
            float s = swingOf(0.22f);
            gl.x = (left ? -20.0f : 20.0f) + (left ? 14.0f : -14.0f) * s;
            gl.z += 180.0f * s;
            gl.y = baseY - 16.0f * s;
            bodySkew = sign * 0.26f * s;
            offsetY = -14.0f * s;
            offsetZ = 24.0f * s;
        }
        break;
    default:
        break;
    }
}

void Fighter::fillRenderData(FighterRenderData& data) const {
    data.char_id = charId;
    data.action_state = static_cast<uint32_t>(state);
    data.hp = hp;
    data.max_hp = maxHp;
    data.stamina = stamina;
    data.max_stamina = maxStamina;
    data.is_exhausted = isExhausted ? 1 : 0;
    data.is_blocking = (state == FIGHTER_BLOCK) ? 1 : 0;
    data.is_left_punch = isLeftPunch ? 1 : 0;
    data.charge_ratio = chargeRatio;
    data.combo_count = comboCount;
    data.world_pos = worldPos;
    data.radius = radius;
    data.facing = facing;
    data.offset_x = offsetX;
    data.offset_y = offsetY;
    data.offset_z = offsetZ;
    data.body_skew = bodySkew;
    data.glove_l = gloveL;
    data.glove_r = gloveR;
    data.telegraph_flash = telegraphFlash ? 1 : 0;
    data.hitstop_timer = hitstopTimer;
    data.size_category = static_cast<uint32_t>(sizeCategory());
}

} // namespace KoRama
