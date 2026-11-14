// Poses das habilidades. As de Jack, Joana, Cremoso e King Frog são o animateSpecialSkillMove do
// index.html; os demais golpes (e os que no JS caíam numa pose genérica) ganharam poses próprias,
// sincronizadas com os efeitos de app/vfx: o soco magmático arma girando antes de explodir, a
// erupção crava os dois punhos no chão, a Vivaldi rege com a batuta, a Lady Espinha dá os dois
// tapas de cacto, o Dr. Marcov chama o satélite com o braço erguido, e assim por diante.
#include "skill_poses.hpp"
#include <algorithm>
#include <cmath>

namespace KoRama {

namespace {
constexpr float kPi = 3.14159265f;
float ease(float t) { t = std::clamp(t, 0.0f, 1.0f); return std::sin(t * kPi * 0.5f); }
float window(float p, float a, float b) { return std::clamp((p - a) / (b - a), 0.0f, 1.0f); }
} // namespace

bool animateSkillPose(Fighter& f, SkillId id, float p) {
    const float swing = std::sin(p * kPi);
    PartRenderData& L = f.gloveL;
    PartRenderData& R = f.gloveR;
    const float baseY = f.guardGloveY();

    switch (id) {
    // ------------------------------------------------------------------ Jack (index.html)
    case SkillId::JackStingJab:
        L.x = -8; R.x = 8;
        L.y = R.y = 70 + 8 * swing;
        L.z = R.z = 18 + 85 * swing;
        f.bodySkew = 0.08f * swing;
        f.offsetZ = 14 * swing;
        return true;
    case SkillId::JackHoneyWall:
        L.x = -26 - 12 * swing; R.x = 26 + 12 * swing;
        L.y = R.y = 70;
        L.z = R.z = 18 + 48 * swing;
        return true;
    case SkillId::JackMolasses:
        if (p < 0.35f) { const float t = p / 0.35f; L.x = -20 - 35 * t; R.x = 20 + 35 * t; }
        else { const float slam = ease((p - 0.35f) / 0.65f); L.x = -55 + 47 * slam; R.x = 55 - 47 * slam; }
        L.y = R.y = 70;
        return true;
    case SkillId::JackCombBurst:
        L.x = -24 + 14 * swing; R.x = 24 + 20 * swing;
        L.z = R.z = 18 + 65 * swing;
        L.y = R.y = 70;
        f.bodySkew = 0.14f * swing;
        return true;
    case SkillId::JackRoyalJelly:
        L.y = R.y = 70 + 32 * swing;
        L.x = -16; R.x = 16;
        f.offsetY = 6 * swing;
        return true;
    case SkillId::JackInfestation:
        L.x = -20 - 35 * swing; R.x = 20 + 35 * swing;
        L.y = R.y = 70 + 20 * swing;
        L.z = R.z = 18 + 25 * swing;
        return true;

    // ------------------------------------------------------------------ Joana (index.html + novas)
    case SkillId::JoanaClawCrush:
        R.x = 35 + 12 * swing; R.y = 68 + 8 * swing; R.z = 15 + 85 * swing;
        L.x = -35; L.y = 68; L.z = 15;
        f.bodySkew = 0.22f * swing;
        f.offsetZ = 10 * swing;
        return true;
    case SkillId::JoanaHookPull:
        L.x = -35; L.y = 68 + 8 * swing; L.z = 15 + 90 * swing;
        R.x = 35; R.y = 68; R.z = 15;
        f.bodySkew = -0.18f * swing;
        return true;
    case SkillId::JoanaAnchorSlam:
        L.y = R.y = 68 + 45 * swing;
        L.z = R.z = 15 + 60 * swing;
        f.offsetY = -15 * swing;
        return true;
    case SkillId::JoanaCrustaceanShell:
        L.x = -20; R.x = 20;
        L.y = R.y = 80;
        L.z = R.z = 18;
        f.bodySkew = 0.04f * std::sin(p * kPi * 10) * swing; // treme enquanto as cracas brotam
        return true;
    case SkillId::JoanaCoralSpout: {
        // Nova: ergue as duas mãos e soca o chão; o coral irrompe sob o rival.
        const float raise = ease(window(p, 0.0f, 0.28f)), slam = ease(window(p, 0.28f, 0.36f)), back = window(p, 0.75f, 1.0f);
        const float y = 68 + 60 * raise - 110 * slam;
        L.x = -26; R.x = 26;
        L.y = R.y = std::max(6.0f, y + (68 - std::max(6.0f, y)) * back);
        L.z = R.z = 15 + 30 * slam * (1 - back);
        f.offsetY = -16 * slam * (1 - back) + 6 * raise * (1 - slam);
        return true;
    }
    case SkillId::JoanaOceanBounty: {
        // Nova: a garra estala no peito e o gancho embala as anêmonas.
        const float sway = std::sin(p * kPi * 3);
        R.x = 22; R.y = 86 + 6 * sway; R.z = 22;
        L.x = -30 - 6 * sway; L.y = 74 + 10 * swing; L.z = 10;
        f.bodySkew = 0.06f * sway;
        f.offsetY = 4 * swing;
        return true;
    }

    // ------------------------------------------------------------------ O Cremoso (index.html + novas)
    case SkillId::CremosoConePunch:
        R.x = 35 - 8 * swing; R.y = 68 + 10 * swing; R.z = 15 + 88 * swing;
        L.x = -35; L.y = 68; L.z = 15;
        f.bodySkew = 0.20f * swing;
        return true;
    case SkillId::CremosoSyrupBlast:
        L.x = -22; R.x = 22;
        L.y = R.y = 68 + 8 * swing;
        L.z = R.z = 15 + 68 * swing;
        return true;
    case SkillId::CremosoCherryTopping:
        if (p < 0.48f) {
            const float prep = ease(p / 0.48f);
            L.x = -35 - 15 * prep; R.x = 35 + 15 * prep;
            L.z = R.z = 15 - 10 * prep;
            L.y = R.y = 68 + 6 * prep;
            f.offsetY = 6 * prep;
        } else {
            const float recoil = std::sin((p - 0.48f) / 0.52f * kPi);
            L.x = -50 + 15 * (1 - recoil); R.x = 50 - 15 * (1 - recoil);
            L.z = R.z = 5 + 10 * (1 - recoil);
            L.y = R.y = 68;
            f.offsetY = -12 * recoil;
            f.bodySkew = -0.10f * recoil;
        }
        return true;
    case SkillId::CremosoVanillaWhip:
        L.x = -35 - 15 * std::cos(p * kPi * 4); R.x = 35 + 15 * std::cos(p * kPi * 4);
        L.z = 15 + 40 * std::sin(p * kPi * 4); R.z = 15 - 40 * std::sin(p * kPi * 4);
        L.y = R.y = 68;
        f.bodySkew = 0.25f * std::sin(p * kPi * 4);
        return true;
    case SkillId::CremosoMeltMode:
        // Nova: derrete, as luvas escorregam e o corpo balança feito gelatina.
        L.x = -38; R.x = 38;
        L.y = R.y = 68 - 26 * swing;
        L.z = R.z = 10;
        f.offsetY = -18 * swing;
        f.bodySkew = 0.12f * std::sin(p * kPi * 6) * swing;
        return true;
    case SkillId::CremosoPuddleTrap:
        // Nova: braços erguidos enquanto a base derrete em poça fervente.
        L.x = -30 - 6 * swing; R.x = 30 + 6 * swing;
        L.y = R.y = 68 + 50 * swing;
        L.z = R.z = 12;
        f.offsetY = -10 * swing;
        f.bodySkew = 0.05f * std::sin(p * kPi * 8) * swing;
        return true;

    // ------------------------------------------------------------------ King Frog (index.html)
    case SkillId::FrogSwampSpout:
        R.x = 35; R.y = 65 + 95 * swing; R.z = 65 * swing;
        f.bodySkew = -0.25f * swing;
        return true;
    case SkillId::FrogTongue:
        L.x = -42 - 20 * swing; R.x = 42 + 20 * swing;
        L.z = R.z = -15 * swing;
        L.y = R.y = 45 - 5 * swing;
        f.offsetY = -8 * swing;
        f.bodySkew = 0.22f * std::sin(p * kPi * 2);
        return true;
    case SkillId::FrogMudSlime:
        L.x = -40; R.x = 40;
        L.z = R.z = 35 * swing;
        L.y = R.y = baseY;
        return true;
    case SkillId::FrogFlyCatcher:
        // Nova: abre os braços e estica o pescoço para cima caçando as moscas.
        L.x = -46 - 10 * swing; R.x = 46 + 10 * swing;
        L.y = R.y = 45 + 20 * swing;
        L.z = R.z = -6 * swing;
        f.offsetY = 10 * swing;
        f.bodySkew = 0.15f * std::sin(p * kPi * 3);
        return true;
    case SkillId::FrogPoisonSkin:
        L.x = -24; R.x = 24;
        L.y = R.y = 52 + 6 * swing;
        L.z = R.z = 20;
        f.bodySkew = 0.05f * std::sin(p * kPi * 12) * swing;
        f.offsetY = -6 * swing;
        return true;

    // ------------------------------------------------------------------ Valéria (novas)
    case SkillId::ValeriaMagmaPunch: {
        // Arma girando o braço (o guarda-chuva acelera) e solta um direto magmático no 0,42.
        const float wind = ease(window(p, 0.0f, 0.42f)), hit = window(p, 0.42f, 0.62f), rec = window(p, 0.62f, 1.0f);
        const float thrust = std::sin(hit * kPi * 0.5f) * (1 - rec);
        const float spin = p * kPi * 6 * (1 - hit);
        R.x = 40 + 14 * wind + std::cos(spin) * 10 * wind - 30 * thrust;
        R.y = baseY - 10 * wind + std::sin(spin) * 10 * wind + 12 * thrust;
        R.z = 12 - 26 * wind * (1 - thrust) + 140 * thrust;
        L.x = -30; L.y = baseY + 6; L.z = 18;
        f.bodySkew = -0.25f * wind * (1 - thrust) + 0.35f * thrust;
        f.offsetZ = 26 * thrust;
        f.offsetX = 10 * thrust;
        return true;
    }
    case SkillId::ValeriaVolcanicErupt: {
        // Ergue os dois punhos e crava no chão: o pilar sobe sob o rival.
        const float raise = ease(window(p, 0.0f, 0.45f)), slam = ease(window(p, 0.45f, 0.55f)), rec = window(p, 0.8f, 1.0f);
        const float y = baseY + 62 * raise - 120 * slam;
        L.x = -24; R.x = 24;
        L.y = R.y = std::max(4.0f, y) + (baseY - std::max(4.0f, y)) * rec;
        L.z = R.z = 12 + 34 * slam * (1 - rec);
        f.offsetY = (8 * raise - 24 * slam) * (1 - rec);
        f.bodySkew = 0.04f * std::sin(p * kPi * 14) * slam * (1 - rec);
        return true;
    }
    case SkillId::ValeriaHeatwaveGuard:
        L.x = -50 * swing - 20 * (1 - swing); R.x = 50 * swing + 20 * (1 - swing);
        L.y = R.y = baseY + 24 * swing;
        L.z = R.z = 14;
        f.bodySkew = 0.05f * std::sin(p * kPi * 9) * swing;
        f.offsetY = -6 * swing;
        return true;
    case SkillId::ValeriaLavaKickJab: {
        // Inclina para trás e estoca baixo com a perna (luva direita vai junto na diagonal).
        const float s = std::sin(window(p, 0.15f, 1.0f) * kPi);
        R.x = 20; R.y = baseY - 34 * s; R.z = 12 + 120 * s;
        L.x = -34; L.y = baseY + 14 * s; L.z = 4;
        f.bodySkew = -0.3f * s;
        f.offsetY = -10 * s;
        f.offsetZ = 16 * s;
        return true;
    }
    case SkillId::ValeriaAshCloud: {
        // Varre o braço de um lado ao outro lançando as cinzas.
        const float t = window(p, 0.1f, 0.5f);
        R.x = 60 - 110 * t; R.y = baseY + 10; R.z = 20 + 60 * std::sin(t * kPi);
        L.x = -36; L.y = baseY; L.z = 10;
        f.bodySkew = 0.3f - 0.6f * t;
        return true;
    }
    case SkillId::ValeriaPhoenixEmber:
        L.x = -30 - 34 * swing; R.x = 30 + 34 * swing;
        L.y = R.y = baseY + 52 * swing;
        L.z = R.z = 6;
        f.offsetY = 14 * swing;
        return true;

    // ------------------------------------------------------------------ Maestra Vivaldi (novas)
    case SkillId::VivaldiCadenceStaccato: {
        // Cinco jabs curtos alternados, cada um mais longo (crescendo).
        const float local = std::fmod(p * 5.0f, 1.0f);
        const int beat = std::min(4, static_cast<int>(p * 5.0f));
        const bool left = beat % 2 == 1;
        PartRenderData& g = left ? L : R;
        PartRenderData& o = left ? R : L;
        const float s = std::sin(local * kPi), reach = 90 + beat * 18.0f;
        g.x = left ? -12 : 12; g.y = baseY + 20 * s; g.z = 14 + reach * s;
        o.x = left ? 30 : -30; o.y = baseY; o.z = 12;
        f.bodySkew = (left ? -0.18f : 0.18f) * s;
        f.offsetZ = 10 * s;
        return true;
    }
    case SkillId::VivaldiWinterAllegro: {
        const float prep = ease(window(p, 0.0f, 0.3f)), up = std::sin(window(p, 0.3f, 1.0f) * kPi);
        R.x = 14; R.y = baseY - 26 * prep * (1 - up) + 110 * up; R.z = 12 + 120 * up;
        L.x = -32; L.y = baseY; L.z = 10;
        f.bodySkew = -0.3f * up;
        f.offsetY = -10 * prep * (1 - up) + 24 * up;
        return true;
    }
    case SkillId::VivaldiFourSeasons: {
        // Rege a orquestra: a batuta (luva direita) desenha um círculo, a esquerda marca o compasso.
        const float a = p * kPi * 4;
        R.x = 30 + 26 * std::cos(a); R.y = baseY + 26 + 24 * std::sin(a); R.z = 20;
        L.x = -34; L.y = baseY + 16 * std::fabs(std::sin(a * 0.5f)); L.z = 14;
        f.bodySkew = 0.08f * std::sin(a);
        f.offsetY = 6 * swing;
        return true;
    }
    case SkillId::VivaldiCrescendoJab: {
        const float s = std::sin(window(p, 0.2f, 1.0f) * kPi);
        R.x = 6; R.y = baseY + 18 * s; R.z = 12 + 170 * s;
        L.x = -30; L.y = baseY; L.z = 6;
        f.bodySkew = 0.25f * s;
        f.offsetZ = 34 * s;
        return true;
    }
    case SkillId::VivaldiAutumnRequiem: {
        const float a = std::sin(p * kPi * 2);
        L.x = -40 + 20 * a; R.x = 40 + 20 * a;
        L.y = R.y = baseY + 40 * swing;
        L.z = R.z = 20;
        f.bodySkew = 0.12f * a;
        return true;
    }
    case SkillId::VivaldiSpringSonata:
        L.x = -28 - 26 * swing; R.x = 28 + 26 * swing;
        L.y = R.y = baseY + 44 * swing;
        L.z = R.z = 10;
        f.offsetY = 16 * swing;
        return true;

    // ------------------------------------------------------------------ Lady Espinha (novas)
    case SkillId::EspinhaNopalSlap: {
        // Dois tapas de cacto em gancho: esquerda no 0,35 e direita no 0,58.
        const float s1 = std::sin(window(p, 0.18f, 0.48f) * kPi), s2 = std::sin(window(p, 0.42f, 0.74f) * kPi);
        L.x = -52 + 80 * window(p, 0.18f, 0.48f) * (s1 > 0 ? 1.0f : 0.0f); L.y = baseY + 20 * s1; L.z = 12 + 120 * s1;
        R.x = 52 - 80 * window(p, 0.42f, 0.74f) * (s2 > 0 ? 1.0f : 0.0f); R.y = baseY + 20 * s2; R.z = 12 + 120 * s2;
        f.bodySkew = 0.4f * s1 - 0.4f * s2;
        f.offsetZ = 14 * std::max(s1, s2);
        return true;
    }
    case SkillId::EspinhaThornVeil: {
        const float burst = ease(window(p, 0.4f, 0.6f));
        const float cross = 1 - burst;
        L.x = -16 * cross - 56 * burst; R.x = 16 * cross + 56 * burst;
        L.y = R.y = baseY + 12;
        L.z = R.z = 22 * cross + 6 * burst;
        f.offsetY = -10 * cross * swing + 6 * burst * swing;
        return true;
    }
    case SkillId::EspinhaNeedleJab: {
        const float s = std::sin(window(p, 0.15f, 1.0f) * kPi);
        R.x = 10; R.y = baseY + 12 * s; R.z = 12 + 165 * s;
        L.x = -38; L.y = baseY - 6; L.z = 4;
        f.bodySkew = 0.3f * s;
        f.offsetZ = 30 * s;
        f.offsetY = -6 * s;
        return true;
    }
    case SkillId::EspinhaDesertWind: {
        // Rodopio rasteiro: as luvas orbitam o corpo agachado.
        const float a = p * kPi * 6;
        L.x = -46 * std::cos(a); R.x = 46 * std::cos(a);
        L.z = 20 + 40 * std::sin(a); R.z = 20 - 40 * std::sin(a);
        L.y = R.y = baseY - 20;
        f.offsetY = -16 * swing;
        f.bodySkew = 0.3f * std::sin(a);
        return true;
    }
    case SkillId::EspinhaFridaTears: {
        // Mãos no rosto (lágrimas) e depois abertas para o céu.
        const float open = ease(window(p, 0.5f, 0.75f));
        L.x = -14 - 46 * open; R.x = 14 + 46 * open;
        L.y = R.y = baseY + 34 + 20 * open;
        L.z = R.z = 24 * (1 - open) + 8;
        f.offsetY = 10 * open * swing;
        f.bodySkew = 0.05f * std::sin(p * kPi * 6) * (1 - open);
        return true;
    }

    // ------------------------------------------------------------------ Dr. Marcov (novas)
    case SkillId::MarcovRocketThrust: {
        const float crouch = ease(window(p, 0.0f, 0.3f)), dash = std::sin(window(p, 0.3f, 1.0f) * kPi);
        R.x = 14; R.y = baseY - 10 * crouch * (1 - dash) + 14 * dash; R.z = 12 - 20 * crouch * (1 - dash) + 160 * dash;
        L.x = -36; L.y = baseY; L.z = 6;
        f.offsetY = -14 * crouch * (1 - dash);
        f.offsetZ = 60 * dash;
        f.bodySkew = 0.3f * dash;
        return true;
    }
    case SkillId::MarcovOrbitalLaser: {
        // Ergue o braço chamando o satélite e baixa no disparo.
        const float up = ease(window(p, 0.0f, 0.3f)), fire = ease(window(p, 0.55f, 0.65f));
        R.x = 16; R.y = baseY + 90 * up * (1 - fire) + 30 * fire; R.z = 14 + 40 * fire;
        L.x = -30; L.y = baseY + 10; L.z = 14;
        f.bodySkew = -0.1f * up + 0.15f * fire;
        f.offsetY = 8 * up * (1 - fire);
        return true;
    }
    case SkillId::MarcovPlasmaBarrier:
        L.x = -30 - 14 * swing; R.x = 30 + 14 * swing;
        L.y = R.y = baseY + 10;
        L.z = R.z = 14 + 50 * swing;
        f.offsetY = 4 * swing;
        return true;
    case SkillId::MarcovKineticOverdrive: {
        const float pull = ease(window(p, 0.0f, 0.3f)), s = std::sin(window(p, 0.3f, 1.0f) * kPi);
        R.x = 34 - 24 * s; R.y = baseY + 10 * s; R.z = 12 - 30 * pull * (1 - s) + 170 * s;
        L.x = -32; L.y = baseY; L.z = 6;
        f.bodySkew = -0.2f * pull * (1 - s) + 0.35f * s;
        f.offsetZ = 26 * s;
        return true;
    }
    case SkillId::MarcovExhaustCloud:
        L.x = -44; R.x = 44;
        L.y = R.y = baseY - 20 * swing;
        L.z = R.z = -10 * swing;
        f.bodySkew = 0.2f * std::sin(p * kPi * 2);
        f.offsetY = -8 * swing;
        return true;
    case SkillId::MarcovOxygenRecharge:
        L.x = -18; R.x = 18;
        L.y = R.y = baseY + 26 + 6 * std::sin(p * kPi * 4);
        L.z = R.z = 18;
        f.offsetY = 5 * std::sin(p * kPi * 2);
        return true;

    default:
        return false;
    }
}

} // namespace KoRama
