#ifndef KORAMA_VFX_PSX_BEE_HPP
#define KORAMA_VFX_PSX_BEE_HPP

#include "camera3d.hpp"
#include "physics.hpp"
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace KoRama::vfx {

// Renderizador de abelhas 3D vibrantes estilo anime/cel-shaded:
// Modelo 3D com orientação física completa (yaw, pitch, roll nas curvas),
// faixas listradas amarelo ouro vibrante e preto nanquim com iluminação cel-shaded,
// olhos expressivos ciano elétrico brilhante, asas translúcidas com reflexo especular
// cartoon e proporções limpas perfeitamente integradas ao estilo visual do KO-RAMA.
inline void drawPsxBee(sf::RenderTarget& target,
                       const Camera3D& camera,
                       const Vec3& worldPos,
                       const Vec3& flightDir,
                       float sizeScale,
                       float flapPhase,
                       float rollAngle = 0.0f,
                       float alpha = 1.0f,
                       bool isRoyal = false,
                       uint32_t skinId = 0) {
    if (alpha <= 0.01f || sizeScale <= 0.1f) return;

    Vec3 localCenter = camera.toLocal(worldPos);
    if (localCenter.z <= 6.0f) return;

    // Escala mundial do modelo
    const float s = sizeScale * 0.42f;

    // Cálculo do referencial ortonormal 3D (Forward, Right, Up) com Pitch, Yaw e Roll
    Vec3 fwd = flightDir;
    float len = std::sqrt(fwd.x * fwd.x + fwd.y * fwd.y + fwd.z * fwd.z);
    if (len > 0.001f) {
        fwd.x /= len; fwd.y /= len; fwd.z /= len;
    } else {
        fwd = {0.0f, 0.0f, 1.0f};
    }

    float pitch = std::asin(std::clamp(fwd.y, -0.999f, 0.999f));
    float yaw = std::atan2(fwd.x, fwd.z);

    Vec3 right0{std::cos(yaw), 0.0f, -std::sin(yaw)};
    Vec3 up0{-std::sin(pitch) * std::sin(yaw), std::cos(pitch), -std::sin(pitch) * std::cos(yaw)};

    float cosR = std::cos(rollAngle), sinR = std::sin(rollAngle);
    Vec3 right{right0.x * cosR + up0.x * sinR, right0.y * cosR + up0.y * sinR, right0.z * cosR + up0.z * sinR};
    Vec3 up{-right0.x * sinR + up0.x * cosR, -right0.y * sinR + up0.y * cosR, -right0.z * sinR + up0.z * cosR};

    auto toWorld = [&](float lx, float ly, float lz) -> Vec3 {
        return {
            worldPos.x + right.x * lx + up.x * ly + fwd.x * lz,
            worldPos.y + right.y * lx + up.y * ly + fwd.y * lz,
            worldPos.z + right.z * lx + up.z * ly + fwd.z * lz
        };
    };

    // Cores Cel-Shaded Ultra Vibrantes
    auto col = [&](std::uint8_t r, std::uint8_t g, std::uint8_t b, float a = 1.0f) {
        float finalA = std::clamp(a * alpha, 0.0f, 1.0f);
        return sf::Color(r, g, b, static_cast<std::uint8_t>(finalA * 255.0f));
    };

    sf::Color cGoldTop, cGoldMid, cGoldShade;
    sf::Color cBlackTop, cBlackShade;
    sf::Color cHeadFill, cHeadShade;
    sf::Color cEyeGlow, cEyeSpec, cEyeBorder;
    sf::Color cWingFill, cWingEdge, cWingGlint;
    sf::Color cStinger, cAntenna;

    if (!isRoyal) {
        // Amarelo solar super saturado, laranja âmbar quente e preto nanquim limpo
        cGoldTop   = col(255, 238, 0);    // #ffee00 (amarelo ouro vivo)
        cGoldMid   = col(251, 191, 36);   // #fbbf24
        cGoldShade = col(234, 88, 12);    // #ea580c (sombra quente)

        cBlackTop   = col(24, 24, 27);    // #18181b (listra escura)
        cBlackShade = col(9, 9, 11);      // #09090b

        cHeadFill  = col(245, 158, 11);   // #f59e0b
        cHeadShade = col(194, 65, 12);    // #c2410c

        cEyeGlow   = col(24, 24, 27);     // #18181b preto carvão limpo
        cEyeBorder = col(9, 9, 11);       // #09090b preto nanquim profundo
        cEyeSpec   = col(255, 255, 255, 0.85f);
        cStinger   = col(9, 9, 11);
        cAntenna   = col(24, 24, 27);

        cWingFill  = col(230, 245, 255, 0.65f);
        cWingEdge  = col(15, 23, 42, 0.85f);
        cWingGlint = col(255, 255, 255, 0.95f);
    } else {
        // Paleta Rainha: Esmeralda cintilante, dourado nobre e asas de cristal
        cGoldTop   = col(167, 243, 208);  // #a7f3d0 menta vibrante
        cGoldMid   = col(52, 211, 153);   // #34d399
        cGoldShade = col(5, 150, 105);    // #059669

        cBlackTop   = col(6, 78, 59);     // #064e3b
        cBlackShade = col(2, 44, 34);

        cHeadFill  = col(251, 191, 36);   // Ouro imperial
        cHeadShade = col(180, 83, 9);

        cEyeGlow   = col(20, 45, 35);     // Esmeralda ônix profundo
        cEyeBorder = col(4, 28, 20);
        cEyeSpec   = col(255, 255, 255, 0.85f);
        cStinger   = col(4, 47, 46);
        cAntenna   = col(6, 78, 59);

        cWingFill  = col(209, 250, 229, 0.80f);
        cWingEdge  = col(6, 78, 59, 0.90f);
        cWingGlint = col(255, 255, 255, 0.95f);
    }

    if (skinId == 1 && !isRoyal) cGoldTop = col(244,213,94);
    if (skinId == 2 && !isRoyal) {
        cGoldTop=col(217,187,101);cGoldShade=col(143,116,62);
        cHeadFill=col(173,172,155);cHeadShade=col(104,112,121);
    }
    if (skinId == 3) {
        if (!isRoyal) { cGoldTop=col(232,203,36);cGoldShade=col(140,110,24); }
        cBlackTop=col(52,70,83);cHeadFill=col(0,191,174);cHeadShade=col(25,99,107);
        cEyeGlow=col(0,245,212);cEyeBorder=col(9,52,52);
        cWingFill=cWingEdge=cWingGlint=sf::Color::Transparent;
    }

    struct CelPart {
        std::array<sf::Vector2f, 4> pts;
        std::size_t numPts = 3;
        sf::Color fill;
        float depth = 0.0f;
    };

    std::vector<CelPart> parts;
    parts.reserve(32);

    auto addQuad = [&](const Vec3& v0, const Vec3& v1, const Vec3& v2, const Vec3& v3,
                       sf::Color fill) {
        auto p0 = camera.project(v0);
        auto p1 = camera.project(v1);
        auto p2 = camera.project(v2);
        auto p3 = camera.project(v3);
        if (!p0 || !p1 || !p2 || !p3) return;

        // Finitude
        if (!std::isfinite(p0->pos.x) || !std::isfinite(p0->pos.y) ||
            !std::isfinite(p1->pos.x) || !std::isfinite(p1->pos.y) ||
            !std::isfinite(p2->pos.x) || !std::isfinite(p2->pos.y) ||
            !std::isfinite(p3->pos.x) || !std::isfinite(p3->pos.y)) return;

        // Área 2D projetada (shoelace) para descartar faces degeneradas de perfil
        float area2D = 0.5f * std::abs(
            (p0->pos.x * p1->pos.y - p1->pos.x * p0->pos.y) +
            (p1->pos.x * p2->pos.y - p2->pos.x * p1->pos.y) +
            (p2->pos.x * p3->pos.y - p3->pos.x * p2->pos.y) +
            (p3->pos.x * p0->pos.y - p0->pos.x * p3->pos.y)
        );
        if (area2D < 0.6f) return;

        float avgDepth = (p0->depth + p1->depth + p2->depth + p3->depth) * 0.25f;
        parts.push_back({ {p0->pos, p1->pos, p2->pos, p3->pos}, 4, fill, avgDepth });
    };

    auto addTri = [&](const Vec3& v0, const Vec3& v1, const Vec3& v2,
                      sf::Color fill) {
        auto p0 = camera.project(v0);
        auto p1 = camera.project(v1);
        auto p2 = camera.project(v2);
        if (!p0 || !p1 || !p2) return;

        if (!std::isfinite(p0->pos.x) || !std::isfinite(p0->pos.y) ||
            !std::isfinite(p1->pos.x) || !std::isfinite(p1->pos.y) ||
            !std::isfinite(p2->pos.x) || !std::isfinite(p2->pos.y)) return;

        float area2D = 0.5f * std::abs(
            (p0->pos.x * (p1->pos.y - p2->pos.y) +
             p1->pos.x * (p2->pos.y - p0->pos.y) +
             p2->pos.x * (p0->pos.y - p1->pos.y))
        );
        if (area2D < 0.5f) return;

        float avgDepth = (p0->depth + p1->depth + p2->depth) * 0.33333f;
        parts.push_back({ {p0->pos, p1->pos, p2->pos, sf::Vector2f{}}, 3, fill, avgDepth });
    };

    auto addRibbon = [&](const Vec3& va, const Vec3& vb, float width, sf::Color col, float depthOffset = -0.05f) {
        auto pa = camera.project(va);
        auto pb = camera.project(vb);
        if (!pa || !pb) return;
        if (!std::isfinite(pa->pos.x) || !std::isfinite(pa->pos.y) ||
            !std::isfinite(pb->pos.x) || !std::isfinite(pb->pos.y)) return;

        sf::Vector2f delta = pb->pos - pa->pos;
        float dLen = std::sqrt(delta.x * delta.x + delta.y * delta.y);
        if (dLen < 0.5f) return;
        sf::Vector2f perp{-delta.y / dLen * width, delta.x / dLen * width};
        float avgDepth = (pa->depth + pb->depth) * 0.5f + depthOffset;
        parts.push_back({ {pa->pos - perp, pb->pos - perp, pb->pos + perp, pa->pos + perp},
                          4, col, avgDepth });
    };

    // ==========================================
    // GEOMETRIA CEL-SHADED DA ABELHA
    // ==========================================

    // 1. Abdômen (Forma arredondada e fofa listrada)
    // Faixa 3 (Traseira cônica com ferrão)
    Vec3 pStingerTip = toWorld( 0.0f,         -0.08f * s, -1.80f * s);
    Vec3 pAb3Top     = toWorld( 0.0f,          0.38f * s, -1.30f * s);
    Vec3 pAb3Btm     = toWorld( 0.0f,         -0.34f * s, -1.30f * s);
    Vec3 pAb3L       = toWorld(-0.40f * s,     0.02f * s, -1.30f * s);
    Vec3 pAb3R       = toWorld( 0.40f * s,     0.02f * s, -1.30f * s);

    // Ferrão afiado
    addTri(pStingerTip, pAb3L, pAb3Top, cStinger);
    addTri(pStingerTip, pAb3Top, pAb3R, cStinger);
    addTri(pStingerTip, pAb3R, pAb3Btm, cStinger);
    addTri(pStingerTip, pAb3Btm, pAb3L, cStinger);

    // Faixa 2 (Amarelo ouro vibrante)
    Vec3 pAb2Top     = toWorld( 0.0f,          0.56f * s, -0.80f * s);
    Vec3 pAb2Btm     = toWorld( 0.0f,         -0.48f * s, -0.80f * s);
    Vec3 pAb2L       = toWorld(-0.62f * s,     0.04f * s, -0.80f * s);
    Vec3 pAb2R       = toWorld( 0.62f * s,     0.04f * s, -0.80f * s);

    // Conecta Faixa 3 (Preto) -> Faixa 2
    addQuad(pAb3Top, pAb3L, pAb2L, pAb2Top, cBlackTop);
    addQuad(pAb3Top, pAb2Top, pAb2R, pAb3R, cBlackTop);
    addQuad(pAb3L, pAb3Btm, pAb2Btm, pAb2L, cBlackShade);
    addQuad(pAb3R, pAb2R, pAb2Btm, pAb3Btm, cBlackShade);

    // Faixa 1 (Preto nanquim)
    Vec3 pAb1Top     = toWorld( 0.0f,          0.65f * s, -0.32f * s);
    Vec3 pAb1Btm     = toWorld( 0.0f,         -0.54f * s, -0.32f * s);
    Vec3 pAb1L       = toWorld(-0.68f * s,     0.05f * s, -0.32f * s);
    Vec3 pAb1R       = toWorld( 0.68f * s,     0.05f * s, -0.32f * s);

    // Conecta Faixa 2 (Amarelo Ouro) -> Faixa 1
    addQuad(pAb2Top, pAb2L, pAb1L, pAb1Top, cGoldTop);
    addQuad(pAb2Top, pAb1Top, pAb1R, pAb2R, cGoldTop);
    addQuad(pAb2L, pAb2Btm, pAb1Btm, pAb1L, cGoldShade);
    addQuad(pAb2R, pAb1R, pAb1Btm, pAb2Btm, cGoldShade);

    // 2. Tórax (Segmento frontal arredondado amarelo vivo)
    Vec3 pThxTop     = toWorld( 0.0f,          0.66f * s,  0.22f * s);
    Vec3 pThxBtm     = toWorld( 0.0f,         -0.50f * s,  0.22f * s);
    Vec3 pThxL       = toWorld(-0.66f * s,     0.06f * s,  0.22f * s);
    Vec3 pThxR       = toWorld( 0.66f * s,     0.06f * s,  0.22f * s);

    // Conecta Faixa 1 (Preto) -> Tórax
    addQuad(pAb1Top, pAb1L, pThxL, pThxTop, cBlackTop);
    addQuad(pAb1Top, pThxTop, pThxR, pAb1R, cBlackTop);
    addQuad(pAb1L, pAb1Btm, pThxBtm, pThxL, cBlackShade);
    addQuad(pAb1R, pThxR, pThxBtm, pAb1Btm, cBlackShade);

    // 3. Cabeça Cartoon
    Vec3 pHeadNose   = toWorld( 0.0f,         -0.04f * s,  1.18f * s);
    Vec3 pHeadTop    = toWorld( 0.0f,          0.44f * s,  0.72f * s);
    Vec3 pHeadBtm    = toWorld( 0.0f,         -0.34f * s,  0.72f * s);
    Vec3 pHeadL      = toWorld(-0.46f * s,     0.05f * s,  0.72f * s);
    Vec3 pHeadR      = toWorld( 0.46f * s,     0.05f * s,  0.72f * s);

    // Conecta Tórax -> Cabeça
    addQuad(pThxTop, pThxL, pHeadL, pHeadTop, cGoldTop);
    addQuad(pThxTop, pHeadTop, pHeadR, pThxR, cGoldTop);
    addQuad(pThxL, pThxBtm, pHeadBtm, pHeadL, cGoldShade);
    addQuad(pThxR, pHeadR, pHeadBtm, pThxBtm, cGoldShade);

    // Focinho da cabeça
    addTri(pHeadTop, pHeadL, pHeadNose, cHeadFill);
    addTri(pHeadTop, pHeadNose, pHeadR, cHeadFill);
    addTri(pHeadBtm, pHeadNose, pHeadL, cHeadShade);
    addTri(pHeadBtm, pHeadR, pHeadNose, cHeadShade);

    // 4. Olhos Cartoon Expressivos (com Backface Culling 3D e renderização 100% imune a linhas/miters)
    Vec3 pEyeLc = toWorld(-0.36f * s, 0.24f * s, 0.95f * s);
    Vec3 pEyeLt = toWorld(-0.38f * s, 0.38f * s, 0.90f * s);
    Vec3 pEyeLf = toWorld(-0.24f * s, 0.24f * s, 1.05f * s);

    Vec3 pEyeRc = toWorld( 0.36f * s, 0.24f * s, 0.95f * s);
    Vec3 pEyeRt = toWorld( 0.38f * s, 0.38f * s, 0.90f * s);
    Vec3 pEyeRf = toWorld( 0.24f * s, 0.24f * s, 1.05f * s);

    auto renderSafeEye = [&](const Vec3& pCenter, const Vec3& pTop, const Vec3& pFwd, const Vec3& nEye) {
        // Backface Culling 3D em relação à câmera para nunca desenhar o olho oculto de trás da cabeça
        Vec3 vCam = { camera.pos.x - pCenter.x, camera.pos.y - pCenter.y, camera.pos.z - pCenter.z };
        float distCam = std::sqrt(vCam.x * vCam.x + vCam.y * vCam.y + vCam.z * vCam.z);
        if (distCam < 0.001f) return;
        float cosView = (nEye.x * vCam.x + nEye.y * vCam.y + nEye.z * vCam.z) / distCam;
        if (cosView <= 0.06f) return; // Olho virado para longe ou em ângulo rasante

        auto pc = camera.project(pCenter);
        if (!pc || !std::isfinite(pc->pos.x) || !std::isfinite(pc->pos.y)) return;

        auto pt = camera.project(pTop);
        auto pf = camera.project(pFwd);

        sf::Vector2f u = pt ? (pt->pos - pc->pos) : sf::Vector2f{0.0f, -0.8f * pc->scale};
        sf::Vector2f v = pf ? (pf->pos - pc->pos) : sf::Vector2f{0.8f * pc->scale, 0.0f};

        float lenU = std::sqrt(u.x * u.x + u.y * u.y);
        float lenV = std::sqrt(v.x * v.x + v.y * v.y);
        // Raio base do olho estritamente proporcional à escala física da abelha no espaço
        float baseR = std::clamp((0.15f * s) * pc->scale, 0.55f, 2.2f);

        if (lenU < 0.2f || lenV < 0.2f) {
            u = { -0.15f * baseR, -0.90f * baseR };
            v = {  0.85f * baseR,  0.15f * baseR };
        } else {
            float rMaj = std::clamp(lenU, baseR * 0.65f, baseR * 1.35f);
            float rMin = std::clamp(lenV, baseR * 0.45f, baseR * 1.10f);
            u = (u / lenU) * rMaj;
            v = (v / lenV) * rMin;
        }

        const sf::Vector2f c = pc->pos;

        // 1. Olho preto nanquim cel-shaded clássico (proporcional, sutil e natural)
        sf::Vector2f o0 = c + u;
        sf::Vector2f o1 = c + v;
        sf::Vector2f o2 = c - u;
        sf::Vector2f o3 = c - v;
        parts.push_back({ {o0, o1, o2, o3}, 4, cEyeGlow, pc->depth - 0.20f });

        // 2. Micro-brilho especular cartoon
        sf::Vector2f specC = c + u * 0.30f + v * 0.20f;
        float specR = std::clamp(baseR * 0.35f, 0.35f, 0.95f);
        parts.push_back({ {specC + sf::Vector2f{-specR, -specR},
                           specC + sf::Vector2f{ specR, -specR},
                           specC + sf::Vector2f{ specR,  specR},
                           specC + sf::Vector2f{-specR,  specR}}, 4, cEyeSpec, pc->depth - 0.24f });
    };

    // Vetores normais dos olhos orientados para fora da face
    Vec3 nEyeL = {
        right.x * (-0.80f) + up.x * 0.22f + fwd.x * 0.55f,
        right.y * (-0.80f) + up.y * 0.22f + fwd.y * 0.55f,
        right.z * (-0.80f) + up.z * 0.22f + fwd.z * 0.55f
    };
    Vec3 nEyeR = {
        right.x * ( 0.80f) + up.x * 0.22f + fwd.x * 0.55f,
        right.y * ( 0.80f) + up.y * 0.22f + fwd.y * 0.55f,
        right.z * ( 0.80f) + up.z * 0.22f + fwd.z * 0.55f
    };

    renderSafeEye(pEyeLc, pEyeLt, pEyeLf, nEyeL);
    renderSafeEye(pEyeRc, pEyeRt, pEyeRf, nEyeR);

    // 5. Antenas Cartoon
    Vec3 pAntL0 = toWorld(-0.15f * s, 0.42f * s, 0.96f * s);
    Vec3 pAntL1 = toWorld(-0.26f * s, 0.70f * s, 1.20f * s);
    Vec3 pAntR0 = toWorld( 0.15f * s, 0.42f * s, 0.96f * s);
    Vec3 pAntR1 = toWorld( 0.26f * s, 0.70f * s, 1.20f * s);

    addRibbon(pAntL0, pAntL1, 0.85f, cAntenna);
    addRibbon(pAntR0, pAntR1, 0.85f, cAntenna);

    // 6. Asas Cartoon Translúcidas com Borda Estrutural e Brilho Especular
    float flapAngle = std::sin(flapPhase) * 0.82f;
    float cosF = std::cos(flapAngle), sinF = std::sin(flapAngle);
    float wingSpan = 1.55f * s;
    float wingLen  = 1.10f * s;

    Vec3 pWingL_root = toWorld(-0.16f * s, 0.58f * s, 0.15f * s);
    Vec3 pWingL_mid  = toWorld(-0.16f * s - cosF * (wingSpan * 0.65f), 0.58f * s + sinF * (wingSpan * 0.65f),  0.15f * s + wingLen * 0.35f);
    Vec3 pWingL_tip  = toWorld(-0.16f * s - cosF * wingSpan,           0.58f * s + sinF * wingSpan,            0.15f * s - wingLen * 0.35f);
    Vec3 pWingL_rear = toWorld(-0.16f * s - cosF * (wingSpan * 0.40f), 0.58f * s + sinF * (wingSpan * 0.40f),  0.15f * s - wingLen * 0.65f);

    Vec3 pWingR_root = toWorld( 0.16f * s, 0.58f * s, 0.15f * s);
    Vec3 pWingR_mid  = toWorld( 0.16f * s + cosF * (wingSpan * 0.65f), 0.58f * s + sinF * (wingSpan * 0.65f),  0.15f * s + wingLen * 0.35f);
    Vec3 pWingR_tip  = toWorld( 0.16f * s + cosF * wingSpan,           0.58f * s + sinF * wingSpan,            0.15f * s - wingLen * 0.35f);
    Vec3 pWingR_rear = toWorld( 0.16f * s + cosF * (wingSpan * 0.40f), 0.58f * s + sinF * (wingSpan * 0.40f),  0.15f * s - wingLen * 0.65f);

    // Membrana da asa
    addQuad(pWingL_root, pWingL_mid, pWingL_tip, pWingL_rear, cWingFill);
    addQuad(pWingR_root, pWingR_rear, pWingR_tip, pWingR_mid, cWingFill);

    // Borda estrutural escura cel-shaded (ribbon 2D seguro sem miter outlines)
    addRibbon(pWingL_root, pWingL_mid, 0.75f, cWingEdge, -0.06f);
    addRibbon(pWingL_mid,  pWingL_tip, 0.75f, cWingEdge, -0.06f);
    addRibbon(pWingR_root, pWingR_mid, 0.75f, cWingEdge, -0.06f);
    addRibbon(pWingR_mid,  pWingR_tip, 0.75f, cWingEdge, -0.06f);

    // Reflexo especular branco cartoon nas asas (ribbon 2D seguro)
    addRibbon(pWingL_root, pWingL_tip, 0.55f, cWingGlint, -0.12f);
    addRibbon(pWingR_root, pWingR_tip, 0.55f, cWingGlint, -0.12f);

    // Skin geometry is world-space, sorted together with the original bee mesh.
    if (skinId == 2) {
        const Vec3 a=toWorld(-.52f*s,.13f*s,.47f*s),b=toWorld(.52f*s,.13f*s,.47f*s),
                   c=toWorld(.52f*s,.85f*s,.47f*s),d=toWorld(-.52f*s,.85f*s,.47f*s),
                   e=toWorld(-.52f*s,.13f*s,1.12f*s),f=toWorld(.52f*s,.13f*s,1.12f*s),
                   g=toWorld(.52f*s,.85f*s,1.12f*s),h=toWorld(-.52f*s,.85f*s,1.12f*s);
        const sf::Color metal=col(155,168,181);
        addQuad(a,b,c,d,metal);addQuad(e,h,g,f,metal);addQuad(a,e,f,b,metal);
        addQuad(d,c,g,h,metal);addQuad(b,f,g,c,metal);addQuad(a,d,h,e,metal);
        addRibbon(toWorld(-.44f*s,.28f*s,1.14f*s),toWorld(.44f*s,.28f*s,1.14f*s),.7f,col(26,36,49));
        addRibbon(toWorld(.73f*s,.28f*s,-.3f*s),toWorld(.73f*s,.28f*s,2.1f*s),.5f,col(215,183,80));
        addTri(toWorld(.73f*s,.28f*s,2.5f*s),toWorld(.52f*s,.28f*s,1.9f*s),toWorld(.94f*s,.28f*s,1.9f*s),col(212,220,228));
    } else if (skinId == 3) {
        for (float x : {-1.25f,1.25f}) for(float z : {-.65f,.65f}) {
            addRibbon(toWorld(x*.35f*s,.55f*s,z*.35f*s),toWorld(x*s,.55f*s,z*s),.6f,col(135,156,167));
            for(int j=0;j<8;++j) {
                const float a=j*6.2831853f/8,b=(j+1)*6.2831853f/8;
                addRibbon(toWorld((x+std::cos(a)*.6f)*s,.55f*s,(z+std::sin(a)*.6f)*s),
                          toWorld((x+std::cos(b)*.6f)*s,.55f*s,(z+std::sin(b)*.6f)*s),.45f,col(39,159,156));
            }
            for(int j=0;j<2;++j) {
                const float a=flapPhase*1.7f+z+j*1.5707963f;
                addRibbon(toWorld((x-std::cos(a)*.55f)*s,.55f*s,(z-std::sin(a)*.55f)*s),
                          toWorld((x+std::cos(a)*.55f)*s,.55f*s,(z+std::sin(a)*.55f)*s),.6f,col(87,232,223));
            }
        }
    }

    // 7. Brilho Glossy Cel-Shaded no Topo
    Vec3 pGloss = toWorld(0.0f, 0.68f * s, 0.20f * s);
    if (auto pg = camera.project(pGloss)) {
        if (std::isfinite(pg->pos.x) && std::isfinite(pg->pos.y)) {
            float rG = std::max(1.0f, 1.5f * pg->scale);
            parts.push_back({ {pg->pos + sf::Vector2f{-rG, -rG},
                               pg->pos + sf::Vector2f{ rG, -rG},
                               pg->pos + sf::Vector2f{ rG,  rG},
                               pg->pos + sf::Vector2f{-rG,  rG}}, 4, sf::Color(255, 255, 255, static_cast<std::uint8_t>(210 * alpha)), pg->depth - 0.2f });
        }
    }

    // Ordenação de profundidade (Back-to-Front)
    std::stable_sort(parts.begin(), parts.end(), [](const CelPart& a, const CelPart& b) {
        return a.depth > b.depth;
    });

    // All faces are untextured flat colors. Submit one ordered triangle batch
    // per bee instead of one SFML shape/draw call per face (especially drones).
    std::vector<sf::Vertex> vertices;
    vertices.reserve(parts.size()*6);
    for (const auto& p : parts) {
        if (p.numPts < 3) continue;

        // Firewall estrito de coordenadas: dropa qualquer vértice NaN/Inf ou fora do viewport
        bool valid = true;
        for (std::size_t i = 0; i < p.numPts; ++i) {
            if (!std::isfinite(p.pts[i].x) || !std::isfinite(p.pts[i].y) ||
                p.pts[i].x < -500.0f || p.pts[i].x > 1800.0f ||
                p.pts[i].y < -500.0f || p.pts[i].y > 1500.0f) {
                valid = false;
                break;
            }
        }
        if (!valid) continue;

        for(std::size_t i=1;i+1<p.numPts;++i) {
            vertices.push_back({p.pts[0],p.fill});vertices.push_back({p.pts[i],p.fill});vertices.push_back({p.pts[i+1],p.fill});
        }
    }
    if(!vertices.empty())target.draw(vertices.data(),vertices.size(),sf::PrimitiveType::Triangles);
}

} // namespace KoRama::vfx

#endif // KORAMA_VFX_PSX_BEE_HPP
