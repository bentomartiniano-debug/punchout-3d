#ifndef KORAMA_CHAR_ICON_HPP
#define KORAMA_CHAR_ICON_HPP

#include "canvas2d.hpp"
#include <cmath>
#include <initializer_list>

namespace KoRama {

// Desenha a insígnia heráldica/reticular em silhueta de cada lutador.
// Usa o ctx.fillStyle atual do Canvas.
inline void drawCharInsignia(cv::Canvas& ctx, int charId, float cx, float cy, float s) {
    constexpr float kPi = 3.14159265358979323846f;
    auto poly = [&](std::initializer_list<sf::Vector2f> pts) {
        ctx.beginPath();
        bool first = true;
        for (const auto& p : pts) {
            if (first) ctx.moveTo(cx + p.x * s, cy + p.y * s); else ctx.lineTo(cx + p.x * s, cy + p.y * s);
            first = false;
        }
        ctx.closePath(); ctx.fill();
    };
    auto disc = [&](float x, float y, float r, float a0 = 0.0f, float a1 = 2.0f * kPi) {
        ctx.beginPath(); ctx.arc(cx + x * s, cy + y * s, r * s, a0, a1); ctx.closePath(); ctx.fill();
    };
    auto rect = [&](float x, float y, float w, float h) { ctx.fillRect(cx + x * s, cy + y * s, w * s, h * s); };

    // Caminho ultra-rápido para retículas halftone de menus (s <= 14.0f):
    // Cada lutador tem uma silhueta convexa de desenho único, que é agrupada diretamente no lote de vértices
    // sem passar pelo stencil do OpenGL, reduzindo milhares de chamadas de desenho a apenas 1.
    if (s <= 14.0f) {
        const sf::Color col = ctx.solid(ctx.fillStyle);
        switch (charId) {
        case 0: { // Jack: favo de mel hexagonal único
            static const float kHex[6][2] = {
                {0.8660254f, 0.5f}, {0.0f, 1.0f}, {-0.8660254f, 0.5f},
                {-0.8660254f, -0.5f}, {0.0f, -1.0f}, {0.8660254f, -0.5f}
            };
            const float rad = s * 0.95f;
            sf::Vector2f pts[6];
            for (int i = 0; i < 6; ++i) pts[i] = {cx + kHex[i][0] * rad, cy + kHex[i][1] * rad};
            ctx.fillFan(pts, 6, col);
            return;
        }
        case 1: { // King Frog: pentágono de coroa real
            static const float kPts[5][2] = {{-0.75f, 0.65f}, {-0.85f, -0.4f}, {0.0f, -0.85f}, {0.85f, -0.4f}, {0.75f, 0.65f}};
            sf::Vector2f pts[5];
            for (int i = 0; i < 5; ++i) pts[i] = {cx + kPts[i][0] * s, cy + kPts[i][1] * s};
            ctx.fillFan(pts, 5, col);
            return;
        }
        case 2: { // Joana: carapaça de siri (elipse de 8 vértices)
            static const float kCos[8] = {1.f, 0.7071f, 0.f, -0.7071f, -1.f, -0.7071f, 0.f, 0.7071f};
            static const float kSin[8] = {0.f, 0.7071f, 1.f, 0.7071f, 0.f, -0.7071f, -1.f, -0.7071f};
            sf::Vector2f pts[8];
            for (int i = 0; i < 8; ++i) pts[i] = {cx + kCos[i] * 0.88f * s, cy + kSin[i] * 0.65f * s};
            ctx.fillFan(pts, 8, col);
            return;
        }
        case 3: { // O Cremoso: diamante de morango (6 vértices)
            static const float kPts[6][2] = {{0.0f, -0.85f}, {0.75f, -0.3f}, {0.55f, 0.5f}, {0.0f, 0.85f}, {-0.55f, 0.5f}, {-0.75f, -0.3f}};
            sf::Vector2f pts[6];
            for (int i = 0; i < 6; ++i) pts[i] = {cx + kPts[i][0] * s, cy + kPts[i][1] * s};
            ctx.fillFan(pts, 6, col);
            return;
        }
        case 4: { // Valéria: sombrinha (semicírculo de 7 vértices)
            static const float kPts[7][2] = {{-0.85f, 0.2f}, {-0.736f, -0.225f}, {-0.425f, -0.536f}, {0.0f, -0.65f}, {0.425f, -0.536f}, {0.736f, -0.225f}, {0.85f, 0.2f}};
            sf::Vector2f pts[7];
            for (int i = 0; i < 7; ++i) pts[i] = {cx + kPts[i][0] * s, cy + kPts[i][1] * s};
            ctx.fillFan(pts, 7, col);
            return;
        }
        case 5: { // Vivaldi: nota musical inclinada (8 vértices)
            static const float kCos[8] = {1.f, 0.7071f, 0.f, -0.7071f, -1.f, -0.7071f, 0.f, 0.7071f};
            static const float kSin[8] = {0.f, 0.7071f, 1.f, 0.7071f, 0.f, -0.7071f, -1.f, -0.7071f};
            sf::Vector2f pts[8];
            for (int i = 0; i < 8; ++i) {
                const float ex = kCos[i] * 0.75f * s, ey = kSin[i] * 0.55f * s;
                pts[i] = {cx + ex * 0.90f + ey * 0.435f, cy - ex * 0.435f + ey * 0.90f};
            }
            ctx.fillFan(pts, 8, col);
            return;
        }
        case 6: { // Espinha: cacto saguaro hexagonal (6 vértices)
            static const float kPts[6][2] = {{-0.35f, -0.85f}, {0.35f, -0.85f}, {0.6f, 0.0f}, {0.35f, 0.85f}, {-0.35f, 0.85f}, {-0.6f, 0.0f}};
            sf::Vector2f pts[6];
            for (int i = 0; i < 6; ++i) pts[i] = {cx + kPts[i][0] * s, cy + kPts[i][1] * s};
            ctx.fillFan(pts, 6, col);
            return;
        }
        case 7: { // Dr. Marcov: arco convexo em curva de banana (6 vértices)
            static const float kPts[6][2] = {{-0.6f, -0.7f}, {0.2f, -0.6f}, {0.75f, 0.1f}, {0.5f, 0.8f}, {0.0f, 0.7f}, {-0.4f, 0.1f}};
            sf::Vector2f pts[6];
            for (int i = 0; i < 6; ++i) pts[i] = {cx + kPts[i][0] * s, cy + kPts[i][1] * s};
            ctx.fillFan(pts, 6, col);
            return;
        }
        case 8: { // Batalhator: disco circular (8 vértices)
            static const float kCos[8] = {1.f, 0.7071f, 0.f, -0.7071f, -1.f, -0.7071f, 0.f, 0.7071f};
            static const float kSin[8] = {0.f, 0.7071f, 1.f, 0.7071f, 0.f, -0.7071f, -1.f, -0.7071f};
            sf::Vector2f pts[8];
            for (int i = 0; i < 8; ++i) pts[i] = {cx + kCos[i] * 0.85f * s, cy + kSin[i] * 0.85f * s};
            ctx.fillFan(pts, 8, col);
            return;
        }
        default: { // Espantalho / Treino: triângulo de chapéu (3 vértices)
            const sf::Vector2f pts[3] = {{cx - 0.85f * s, cy + 0.5f * s}, {cx, cy - 0.75f * s}, {cx + 0.85f * s, cy + 0.5f * s}};
            ctx.fillFan(pts, 3, col);
            return;
        }
        }
    }

    switch (charId) {
    case 0: { // Jack Mãos de Colmeia: favo de mel triplo de hexágonos elegantes
        auto hex = [&](float hx, float hy, float hr) {
            ctx.beginPath();
            for (int i = 0; i < 6; ++i) {
                const float a = kPi / 6.0f + i * kPi / 3.0f;
                const float x = cx + (hx + std::cos(a) * hr) * s;
                const float y = cy + (hy + std::sin(a) * hr) * s;
                if (i == 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
            }
            ctx.closePath(); ctx.fill();
        };
        // Célula superior e duas inferiores interligadas como colmeia autêntica
        hex(0.0f, -0.42f, 0.48f);
        hex(-0.44f, 0.35f, 0.48f);
        hex(0.44f, 0.35f, 0.48f);
        break;
    }
    case 1: // King Frog: coroa real com faixa de base e 3 pontas
        rect(-0.9f, 0.2f, 1.8f, 0.55f);
        poly({{-0.9f, 0.2f}, {-0.9f, -0.75f}, {-0.35f, 0.2f}});
        poly({{-0.4f, 0.2f}, {0.0f, -0.95f}, {0.4f, 0.2f}});
        poly({{0.35f, 0.2f}, {0.9f, -0.75f}, {0.9f, 0.2f}});
        break;
    case 2: // Joana: carapaça de siri náutico com pinças
        ctx.beginPath(); ctx.ellipse(cx, cy + 0.25f * s, 0.62f * s, 0.42f * s, 0, 0, 2.0f * kPi); ctx.closePath(); ctx.fill();
        disc(-0.72f, -0.5f, 0.3f);
        disc(0.72f, -0.5f, 0.3f);
        rect(-0.9f, 0.35f, 0.26f, 0.12f); rect(0.64f, 0.35f, 0.26f, 0.12f);
        rect(-0.86f, 0.6f, 0.24f, 0.12f); rect(0.62f, 0.6f, 0.24f, 0.12f);
        break;
    case 3: // O Cremoso: morango estilizado com copa e folhas
        poly({{-0.7f, -0.35f}, {0.7f, -0.35f}, {0.62f, 0.15f}, {0.3f, 0.65f}, {0.0f, 0.95f}, {-0.3f, 0.65f}, {-0.62f, 0.15f}});
        poly({{-0.55f, -0.45f}, {0.0f, -0.75f}, {0.55f, -0.45f}, {0.0f, -0.4f}});
        rect(-0.07f, -1.0f, 0.14f, 0.3f);
        break;
    case 4: // Valéria: sombrinha / guarda-chuva de frevo
        disc(0.0f, -0.05f, 0.95f, kPi, 2.0f * kPi);
        rect(-0.07f, -0.05f, 0.14f, 0.85f);
        rect(-0.42f, 0.7f, 0.42f, 0.14f);
        rect(-0.42f, 0.5f, 0.14f, 0.3f);
        break;
    case 5: // Vivaldi: nota musical clássica com cabeça inclinada e bandeira
        ctx.beginPath(); ctx.ellipse(cx - 0.25f * s, cy + 0.6f * s, 0.38f * s, 0.28f * s, -0.4f, 0, 2.0f * kPi); ctx.closePath(); ctx.fill();
        rect(0.02f, -0.95f, 0.16f, 1.5f);
        poly({{0.18f, -0.95f}, {0.75f, -0.45f}, {0.6f, -0.2f}, {0.18f, -0.55f}});
        break;
    case 6: // Espinha: cacto saguaro luchador com tronco e dois braços
        rect(-0.2f, -0.95f, 0.4f, 1.9f);
        rect(-0.62f, -0.05f, 0.42f, 0.22f); rect(-0.62f, -0.6f, 0.22f, 0.55f);
        rect(0.2f, 0.2f, 0.42f, 0.22f); rect(0.4f, -0.35f, 0.22f, 0.55f);
        break;
    case 7: { // Dr. Marcov: Banana inconfundível com cabinho e arco clássico
        // Corpo principal em curva graciosa de banana
        ctx.beginPath();
        ctx.moveTo(cx - 0.42f * s, cy - 0.65f * s);
        // Dorso externo convexo
        ctx.bezierCurveTo(cx + 0.12f * s, cy - 0.62f * s, cx + 0.82f * s, cy - 0.12f * s, cx + 0.78f * s, cy + 0.36f * s);
        ctx.bezierCurveTo(cx + 0.74f * s, cy + 0.68f * s, cx + 0.46f * s, cy + 0.90f * s, cx + 0.22f * s, cy + 0.92f * s);
        // Pontinha inferior da banana
        ctx.lineTo(cx + 0.12f * s, cy + 0.96f * s);
        ctx.lineTo(cx + 0.14f * s, cy + 0.84f * s);
        // Ventre interno côncavo
        ctx.bezierCurveTo(cx + 0.38f * s, cy + 0.62f * s, cx + 0.46f * s, cy + 0.30f * s, cx + 0.42f * s, cy + 0.08f * s);
        ctx.bezierCurveTo(cx + 0.38f * s, cy - 0.18f * s, cx + 0.16f * s, cy - 0.44f * s, cx - 0.40f * s, cy - 0.63f * s);
        ctx.closePath();
        ctx.fill();

        // Cabinho no topo (pedúnculo chanfrado da penca)
        poly({{-0.42f, -0.63f}, {-0.52f, -0.90f}, {-0.36f, -0.94f}, {-0.28f, -0.65f}});
        // Pontinha escura no bico da fruta
        poly({{0.10f, 0.84f}, {0.08f, 0.98f}, {0.22f, 0.94f}});
        break;
    }
    case 8: // Batalhator: serra circular dentada mecânica
        disc(0.0f, 0.0f, 0.68f);
        for (int i = 0; i < 8; ++i) {
            const float a = i * kPi / 4.0f, b = a + kPi / 8.0f;
            poly({{std::cos(a) * 0.62f, std::sin(a) * 0.62f},
                  {std::cos(a + 0.05f) * 1.0f, std::sin(a + 0.05f) * 1.0f},
                  {std::cos(b + 0.25f) * 0.62f, std::sin(b + 0.25f) * 0.62f}});
        }
        break;
    default: // 9 = Espantalho (Modo Treino): chapéu de palha com aba larga e remendo
        disc(0.0f, -0.1f, 0.70f);
        rect(-0.95f, 0.35f, 1.9f, 0.28f);
        poly({{-0.3f, -0.75f}, {0.0f, -0.95f}, {0.3f, -0.75f}});
        break;
    }
}

} // namespace KoRama

#endif // KORAMA_CHAR_ICON_HPP
