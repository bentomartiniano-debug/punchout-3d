#include "ui_draw.hpp"
#include "canvas2d.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <sstream>
#include <vector>

namespace KoRama::ui {

void text(sf::RenderTarget& target, const sf::Font* font, const std::string& utf8, sf::Vector2f pos, unsigned size,
          sf::Color color, float anchorX, float outline) {
    if (!font || utf8.empty()) return;
    sf::Text t(*font, sf::String::fromUtf8(utf8.begin(), utf8.end()), size);
    t.setFillColor(color);
    sf::Color ink = kInk;
    ink.a = color.a;
    t.setOutlineColor(ink);
    t.setOutlineThickness(outline >= 0.0f ? outline : std::max(2.0f, size / 10.0f));
    sf::FloatRect b = t.getLocalBounds();
    t.setPosition({std::round(pos.x - (b.position.x + b.size.x) * anchorX), std::round(pos.y)});
    target.draw(t);
}

void wrappedText(sf::RenderTarget& target, const sf::Font* font, const std::string& utf8, sf::Vector2f pos,
                 unsigned size, sf::Color color, float maxWidth, float lineHeight, float anchorX) {
    if (!font) return;
    std::istringstream words(utf8);
    std::string word, line;
    std::vector<std::string> lines;
    auto width = [&](const std::string& s) {
        sf::Text t(*font, sf::String::fromUtf8(s.begin(), s.end()), size);
        return t.getLocalBounds().size.x;
    };
    while (words >> word) {
        std::string candidate = line.empty() ? word : line + " " + word;
        if (!line.empty() && width(candidate) > maxWidth) {
            lines.push_back(line);
            line = word;
        } else {
            line = candidate;
        }
    }
    if (!line.empty()) lines.push_back(line);
    for (std::size_t i = 0; i < lines.size(); ++i) {
        text(target, font, lines[i], {pos.x, pos.y + i * lineHeight}, size, color, anchorX, 0.0f);
    }
}

void panel(sf::RenderTarget& target, sf::FloatRect r, sf::Color fill, sf::Color border, float borderWidth, float shadow,
           float slant) {
    auto shape = [&](sf::Vector2f offset) {
        sf::ConvexShape s(4);
        s.setPoint(0, {r.position.x + slant + offset.x, r.position.y + offset.y});
        s.setPoint(1, {r.position.x + r.size.x + offset.x, r.position.y + offset.y});
        s.setPoint(2, {r.position.x + r.size.x - slant + offset.x, r.position.y + r.size.y + offset.y});
        s.setPoint(3, {r.position.x + offset.x, r.position.y + r.size.y + offset.y});
        return s;
    };
    if (shadow > 0.0f) {
        sf::ConvexShape s = shape({shadow, shadow});
        s.setFillColor(sf::Color(0, 0, 0, std::min<std::uint8_t>(fill.a, 200)));
        target.draw(s);
    }
    sf::ConvexShape s = shape({0.0f, 0.0f});
    s.setFillColor(fill);
    s.setOutlineColor(border);
    s.setOutlineThickness(borderWidth);
    target.draw(s);
}

void bar(sf::RenderTarget& target, sf::FloatRect r, float ratio, sf::Color fill, sf::Color back) {
    ratio = std::clamp(ratio, 0.0f, 1.0f);
    sf::RectangleShape bg(r.size);
    bg.setPosition(r.position);
    bg.setFillColor(back);
    bg.setOutlineColor(kInk);
    bg.setOutlineThickness(2.0f);
    target.draw(bg);
    sf::RectangleShape fg({r.size.x * ratio, r.size.y});
    fg.setPosition(r.position);
    fg.setFillColor(fill);
    target.draw(fg);
}

void dotBackground(sf::RenderTarget& target, sf::Vector2f size, uint32_t frame) {
    sf::RectangleShape bg(size);
    bg.setFillColor(kNight);
    target.draw(bg);
    // Malha de pontinhos que desce devagar.
    const float spacing = 22.0f;
    float drift = std::fmod(frame * 0.15f, spacing);
    std::vector<sf::Vertex> dots;
    const sf::Color dot(0x1e, 0x18, 0x2c);
    for (float y = -spacing + drift; y < size.y; y += spacing) {
        for (float x = 6.0f; x < size.x; x += spacing) {
            float s = 1.4f;
            sf::Vector2f c{x, y};
            dots.push_back({c + sf::Vector2f{-s, -s}, dot});
            dots.push_back({c + sf::Vector2f{s, -s}, dot});
            dots.push_back({c + sf::Vector2f{s, s}, dot});
            dots.push_back({c + sf::Vector2f{-s, -s}, dot});
            dots.push_back({c + sf::Vector2f{s, s}, dot});
            dots.push_back({c + sf::Vector2f{-s, s}, dot});
        }
    }
    target.draw(dots.data(), dots.size(), sf::PrimitiveType::Triangles);
}

namespace {

using cv::Canvas;

void drawEspantalhoPortrait(sf::RenderTarget& target, sf::FloatRect box, uint32_t frame, bool animated, sf::Color tint) {
    Canvas ctx(target);
    const float f = static_cast<float>(frame);
    const float bob = animated ? std::sin(f * 0.05f) * 3.0f : 0.0f;
    const float breathe = animated ? 1.0f + std::sin(f * 0.05f) * 0.012f : 1.0f;

    const float w = box.size.x, h = box.size.y;
    // O busto do Espantalho é desenhado com a exata geometria do modelo in-game (queueScarecrow),
    // recortado do peito para cima e alinhado na base inferior da caixa exatamente como os outros lutadores.
    const float chestBottomY = -26.0f; // linha de corte do busto (no peito)
    const float hatCrownTop = -153.0f; // topo da copa do chapéu
    const float modelHeight = -hatCrownTop + chestBottomY; // 153 - 26 = 127 unidades de altura

    // Para ficar na mesma altura dos outros lutadores (Jack, Frog, Marcov: altura do busto ~405px em caixa de 600px):
    const float refBustH = 405.0f;
    const float sc = (h / 600.0f) * (refBustH / modelHeight) * breathe;

    // Alinhamento horizontal no centro da caixa e base inferior na mesma linha dos outros lutadores:
    const float cx = box.position.x + w / 2.0f;
    const float bottomY = box.position.y + h - 2.0f + bob;

    const bool isShadow = (tint.r == 0 && tint.g == 0 && tint.b == 0 && tint.a > 0);
    const sf::Color ink = isShadow ? tint : kInk;

    auto col = [&](sf::Color normal) -> sf::Color {
        if (isShadow) return tint;
        if (tint == sf::Color::White) return normal;
        return sf::Color(static_cast<std::uint8_t>(normal.r * tint.r / 255),
                         static_cast<std::uint8_t>(normal.g * tint.g / 255),
                         static_cast<std::uint8_t>(normal.b * tint.b / 255),
                         static_cast<std::uint8_t>(normal.a * tint.a / 255));
    };

    ctx.save();
    // A coordenada model Y = chestBottomY mapeia exatamente para bottomY na tela:
    ctx.translate(cx, bottomY - chestBottomY * sc);

    // 1. Mastro vertical de madeira (termina exatamente na linha de corte do busto)
    const float poleTop = -135.0f;
    const float poleBottom = chestBottomY;
    ctx.beginPath();
    ctx.roundRect(-6.0f * sc, poleTop * sc, 12.0f * sc, (poleBottom - poleTop) * sc, 2.0f * sc);
    ctx.fillStyle = col(sf::Color(115, 75, 42));
    ctx.fill();
    ctx.strokeStyle = ink;
    ctx.lineWidth = 2.5f * sc;
    ctx.stroke();
    if (!isShadow) {
        ctx.beginPath();
        ctx.moveTo(-3.0f * sc, poleBottom * sc);
        ctx.lineTo(-3.0f * sc, (poleTop + 2.0f) * sc);
        ctx.strokeStyle = sf::Color(150, 102, 60);
        ctx.lineWidth = 1.5f * sc;
        ctx.stroke();
    }

    // 2. Travessão horizontal de madeira dos ombros
    ctx.beginPath();
    ctx.roundRect(-52.0f * sc, -74.0f * sc, 104.0f * sc, 10.0f * sc, 2.0f * sc);
    ctx.fillStyle = col(sf::Color(115, 75, 42));
    ctx.fill();
    ctx.strokeStyle = ink;
    ctx.lineWidth = 2.5f * sc;
    ctx.stroke();

    // 3. Tufos de palha saindo das pontas dos ombros
    auto drawStrawTuft = [&](float tx, float ty, float dirX) {
        ctx.fillStyle = col(sf::Color(250, 212, 60));
        ctx.beginPath();
        ctx.moveTo(tx, ty - 4.0f * sc);
        ctx.lineTo(tx + dirX * 14.0f * sc, ty - 6.0f * sc);
        ctx.lineTo(tx + dirX * 10.0f * sc, ty);
        ctx.lineTo(tx + dirX * 16.0f * sc, ty + 4.0f * sc);
        ctx.lineTo(tx, ty + 5.0f * sc);
        ctx.closePath();
        ctx.fill();
        ctx.strokeStyle = ink;
        ctx.lineWidth = 1.8f * sc;
        ctx.stroke();
    };
    drawStrawTuft(-52.0f * sc, -69.0f * sc, -1.0f);
    drawStrawTuft(52.0f * sc, -69.0f * sc, 1.0f);

    // 4. Amarras em X de corda na junção dos mastros
    ctx.strokeStyle = isShadow ? ink : sf::Color(218, 186, 122);
    ctx.lineWidth = 3.5f * sc;
    ctx.beginPath();
    ctx.moveTo(-7.0f * sc, -76.0f * sc); ctx.lineTo(7.0f * sc, -62.0f * sc);
    ctx.moveTo(7.0f * sc, -76.0f * sc); ctx.lineTo(-7.0f * sc, -62.0f * sc);
    ctx.stroke();
    if (!isShadow) {
        ctx.strokeStyle = ink;
        ctx.lineWidth = 1.0f * sc;
        ctx.beginPath();
        ctx.moveTo(-7.0f * sc, -76.0f * sc); ctx.lineTo(7.0f * sc, -62.0f * sc);
        ctx.moveTo(7.0f * sc, -76.0f * sc); ctx.lineTo(-7.0f * sc, -62.0f * sc);
        ctx.stroke();
    }

    // 5. Luvas de boxe penduradas por cordas (antigas, couro vermelho)
    auto drawBoxingGlove = [&](float hangX, float hangY, bool isLeft) {
        ctx.save();
        ctx.translate(hangX, hangY);

        ctx.strokeStyle = isShadow ? ink : sf::Color(200, 170, 110);
        ctx.lineWidth = 2.0f * sc;
        ctx.beginPath();
        ctx.moveTo(0.0f, 0.0f);
        ctx.lineTo(0.0f, 22.0f * sc);
        ctx.stroke();

        ctx.translate(0.0f, 22.0f * sc);

        // Munhequeira branca/fita
        ctx.beginPath();
        ctx.roundRect(-8.0f * sc, -4.0f * sc, 16.0f * sc, 7.0f * sc, 2.0f * sc);
        ctx.fillStyle = col(sf::Color(235, 235, 230));
        ctx.fill();
        ctx.strokeStyle = ink;
        ctx.lineWidth = 2.0f * sc;
        ctx.stroke();

        // Luva de couro vermelha
        ctx.beginPath();
        ctx.roundRect(-12.0f * sc, 2.0f * sc, 24.0f * sc, 22.0f * sc, 8.0f * sc);
        ctx.fillStyle = col(sf::Color(185, 36, 42));
        ctx.fill();

        if (!isShadow) {
            // Sombra da luva
            ctx.fillStyle = sf::Color(122, 20, 26);
            ctx.beginPath();
            ctx.roundRect(isLeft ? -12.0f * sc : 2.0f * sc, 2.0f * sc, 10.0f * sc, 22.0f * sc, 6.0f * sc);
            ctx.fill();
        }

        // Contorno da luva
        ctx.beginPath();
        ctx.roundRect(-12.0f * sc, 2.0f * sc, 24.0f * sc, 22.0f * sc, 8.0f * sc);
        ctx.strokeStyle = ink;
        ctx.lineWidth = 2.5f * sc;
        ctx.stroke();

        // Polegar da luva
        float thumbX = isLeft ? 10.0f * sc : -10.0f * sc;
        ctx.beginPath();
        ctx.ellipse(thumbX, 10.0f * sc, 5.0f * sc, 8.0f * sc, (isLeft ? 0.3f : -0.3f), 0.0f, 2.0f * 3.14159265f);
        ctx.fillStyle = col(sf::Color(185, 36, 42));
        ctx.fill();
        ctx.strokeStyle = ink;
        ctx.lineWidth = 2.0f * sc;
        ctx.stroke();

        ctx.restore();
    };
    drawBoxingGlove(-40.0f * sc, -70.0f * sc, true);
    drawBoxingGlove(40.0f * sc, -70.0f * sc, false);

    // 6. Tronco de saco de estopa (peito / busto)
    ctx.beginPath();
    ctx.moveTo(-16.0f * sc, -82.0f * sc);
    ctx.bezierCurveTo(-30.0f * sc, -70.0f * sc, -34.0f * sc, -50.0f * sc, -30.0f * sc, chestBottomY * sc);
    ctx.lineTo(30.0f * sc, chestBottomY * sc);
    ctx.bezierCurveTo(34.0f * sc, -50.0f * sc, 30.0f * sc, -70.0f * sc, 16.0f * sc, -82.0f * sc);
    ctx.closePath();
    ctx.fillStyle = col(sf::Color(212, 168, 92));
    ctx.fill();

    if (!isShadow) {
        // Sombra suave na lateral direita
        ctx.fillStyle = sf::Color(156, 116, 52, 175);
        ctx.beginPath();
        ctx.moveTo(4.0f * sc, -82.0f * sc);
        ctx.lineTo(16.0f * sc, -82.0f * sc);
        ctx.bezierCurveTo(30.0f * sc, -70.0f * sc, 34.0f * sc, -50.0f * sc, 30.0f * sc, chestBottomY * sc);
        ctx.lineTo(4.0f * sc, chestBottomY * sc);
        ctx.closePath();
        ctx.fill();

        // Brilho suave na lateral esquerda
        ctx.fillStyle = sf::Color(238, 204, 140, 130);
        ctx.beginPath();
        ctx.moveTo(-8.0f * sc, -82.0f * sc);
        ctx.lineTo(-16.0f * sc, -82.0f * sc);
        ctx.bezierCurveTo(-30.0f * sc, -70.0f * sc, -34.0f * sc, -50.0f * sc, -30.0f * sc, chestBottomY * sc);
        ctx.lineTo(-8.0f * sc, chestBottomY * sc);
        ctx.closePath();
        ctx.fill();
    }

    // Contorno do tronco
    ctx.beginPath();
    ctx.moveTo(-16.0f * sc, -82.0f * sc);
    ctx.bezierCurveTo(-30.0f * sc, -70.0f * sc, -34.0f * sc, -50.0f * sc, -30.0f * sc, chestBottomY * sc);
    ctx.lineTo(30.0f * sc, chestBottomY * sc);
    ctx.bezierCurveTo(34.0f * sc, -50.0f * sc, 30.0f * sc, -70.0f * sc, 16.0f * sc, -82.0f * sc);
    ctx.closePath();
    ctx.strokeStyle = ink;
    ctx.lineWidth = 3.2f * sc;
    ctx.stroke();

    if (!isShadow) {
        // Remendo jeans azul no peito (direito de quem olha)
        ctx.beginPath();
        ctx.roundRect(7.0f * sc, -62.0f * sc, 16.0f * sc, 15.0f * sc, 2.0f * sc);
        ctx.fillStyle = sf::Color(52, 98, 162);
        ctx.fill();
        ctx.strokeStyle = ink;
        ctx.lineWidth = 1.8f * sc;
        ctx.stroke();
        ctx.strokeStyle = sf::Color(240, 240, 240);
        ctx.lineWidth = 1.4f * sc;
        for (float px : {9.0f, 15.0f, 21.0f}) {
            ctx.beginPath();
            ctx.moveTo((px - 1.5f) * sc, -62.0f * sc); ctx.lineTo((px + 1.5f) * sc, -62.0f * sc);
            ctx.moveTo((px - 1.5f) * sc, -47.0f * sc); ctx.lineTo((px + 1.5f) * sc, -47.0f * sc);
            ctx.stroke();
        }

        // Remendo vermelho nas costelas inferiores
        ctx.beginPath();
        ctx.roundRect(-22.0f * sc, -38.0f * sc, 13.0f * sc, 12.0f * sc, 2.0f * sc);
        ctx.fillStyle = sf::Color(185, 45, 45);
        ctx.fill();
        ctx.strokeStyle = ink;
        ctx.lineWidth = 1.8f * sc;
        ctx.stroke();

        // Costura central do saco de estopa
        ctx.strokeStyle = ink;
        ctx.lineWidth = 2.2f * sc;
        ctx.beginPath();
        ctx.moveTo(0.0f, -78.0f * sc);
        ctx.lineTo(0.0f, chestBottomY * sc);
        ctx.stroke();
        for (float sy = -74.0f; sy <= chestBottomY; sy += 7.0f) {
            ctx.beginPath();
            ctx.moveTo(-4.5f * sc, sy * sc);
            ctx.lineTo(4.5f * sc, sy * sc);
            ctx.stroke();
        }
    }

    // 7. Gola de palha ao redor do pescoço
    ctx.fillStyle = col(sf::Color(250, 212, 60));
    ctx.strokeStyle = ink;
    ctx.lineWidth = 1.8f * sc;
    ctx.beginPath();
    ctx.moveTo(-18.0f * sc, -82.0f * sc);
    for (float nx = -15.0f; nx <= 15.0f; nx += 5.0f) {
        ctx.lineTo(nx * sc, -74.0f * sc);
        ctx.lineTo((nx + 2.5f) * sc, -83.0f * sc);
    }
    ctx.closePath();
    ctx.fill();
    ctx.stroke();

    // 8. Corda amarrada no pescoço
    ctx.beginPath();
    ctx.roundRect(-12.0f * sc, -86.0f * sc, 24.0f * sc, 5.0f * sc, 2.5f * sc);
    ctx.fillStyle = col(sf::Color(218, 186, 122));
    ctx.fill();
    ctx.strokeStyle = ink;
    ctx.lineWidth = 2.0f * sc;
    ctx.stroke();

    // 9. Cabeça de saco de estopa (oval)
    const float headCY = -108.0f * sc;
    ctx.beginPath();
    ctx.ellipse(0.0f, headCY, 21.0f * sc, 23.0f * sc, 0.0f, 0.0f, 2.0f * 3.14159265f);
    ctx.fillStyle = col(sf::Color(218, 172, 98));
    ctx.fill();

    if (!isShadow) {
        // Sombra lateral direita da cabeça
        ctx.fillStyle = sf::Color(162, 120, 56, 160);
        ctx.beginPath();
        ctx.arc(0.0f, headCY, 21.0f * sc, -0.45f * 3.14159265f, 0.45f * 3.14159265f);
        ctx.closePath();
        ctx.fill();
    }

    ctx.beginPath();
    ctx.ellipse(0.0f, headCY, 21.0f * sc, 23.0f * sc, 0.0f, 0.0f, 2.0f * 3.14159265f);
    ctx.strokeStyle = ink;
    ctx.lineWidth = 3.2f * sc;
    ctx.stroke();

    // Franzido / nó no topo da cabeça
    ctx.beginPath();
    ctx.ellipse(0.0f, headCY - 22.0f * sc, 5.0f * sc, 3.5f * sc, 0.0f, 0.0f, 2.0f * 3.14159265f);
    ctx.fillStyle = col(sf::Color(190, 148, 76));
    ctx.fill();
    ctx.strokeStyle = ink;
    ctx.lineWidth = 1.8f * sc;
    ctx.stroke();

    if (!isShadow) {
        // Bochechas coradas
        ctx.fillStyle = sf::Color(235, 95, 95, 115);
        ctx.beginPath();
        ctx.ellipse(-12.0f * sc, headCY + 5.0f * sc, 4.5f * sc, 3.0f * sc, 0.0f, 0.0f, 2.0f * 3.14159265f);
        ctx.fill();
        ctx.beginPath();
        ctx.ellipse(12.0f * sc, headCY + 5.0f * sc, 4.5f * sc, 3.0f * sc, 0.0f, 0.0f, 2.0f * 3.14159265f);
        ctx.fill();

        // Olho esquerdo: botão de madeira com X de linha amarela
        ctx.beginPath();
        ctx.arc(-9.0f * sc, headCY, 5.5f * sc, 0.0f, 2.0f * 3.14159265f);
        ctx.fillStyle = sf::Color(45, 30, 20);
        ctx.fill();
        ctx.strokeStyle = ink;
        ctx.lineWidth = 2.0f * sc;
        ctx.stroke();
        ctx.strokeStyle = sf::Color(250, 240, 180);
        ctx.lineWidth = 1.6f * sc;
        ctx.beginPath();
        ctx.moveTo(-11.5f * sc, headCY - 2.5f * sc); ctx.lineTo(-6.5f * sc, headCY + 2.5f * sc);
        ctx.moveTo(-6.5f * sc, headCY - 2.5f * sc); ctx.lineTo(-11.5f * sc, headCY + 2.5f * sc);
        ctx.stroke();

        // Olho direito: botão preto com ponto de brilho
        ctx.beginPath();
        ctx.arc(8.5f * sc, headCY - 0.5f * sc, 4.2f * sc, 0.0f, 2.0f * 3.14159265f);
        ctx.fillStyle = sf::Color(20, 22, 25);
        ctx.fill();
        ctx.strokeStyle = ink;
        ctx.lineWidth = 1.8f * sc;
        ctx.stroke();
        ctx.beginPath();
        ctx.arc(7.2f * sc, headCY - 1.8f * sc, 1.2f * sc, 0.0f, 2.0f * 3.14159265f);
        ctx.fillStyle = sf::Color(255, 255, 255);
        ctx.fill();

        // Nariz triangular de remendo
        ctx.beginPath();
        ctx.moveTo(0.0f, headCY + 1.0f * sc);
        ctx.lineTo(-3.5f * sc, headCY + 6.0f * sc);
        ctx.lineTo(3.5f * sc, headCY + 6.0f * sc);
        ctx.closePath();
        ctx.fillStyle = sf::Color(190, 75, 55);
        ctx.fill();
        ctx.strokeStyle = ink;
        ctx.lineWidth = 1.5f * sc;
        ctx.stroke();

        // Boca costurada
        ctx.strokeStyle = ink;
        ctx.lineWidth = 2.4f * sc;
        ctx.beginPath();
        ctx.moveTo(-13.0f * sc, headCY + 11.0f * sc);
        ctx.quadraticCurveTo(0.0f, headCY + 17.0f * sc, 13.0f * sc, headCY + 11.0f * sc);
        ctx.stroke();
        for (float mx = -10.0f; mx <= 10.0f; mx += 5.0f) {
            float my = headCY + 13.5f * sc - (1.0f - std::fabs(mx) / 10.0f) * 1.5f * sc;
            ctx.beginPath();
            ctx.moveTo(mx * sc, (my - 2.5f) * sc);
            ctx.lineTo(mx * sc, (my + 2.5f) * sc);
            ctx.stroke();
        }
    }

    // 10. O Chapéu de Palha com fita vermelha
    const float hatBaseY = headCY - 18.0f * sc;

    // Cabelo de palha sob a aba do chapéu
    ctx.fillStyle = col(sf::Color(250, 212, 60));
    ctx.strokeStyle = ink;
    ctx.lineWidth = 1.8f * sc;
    ctx.beginPath();
    ctx.moveTo(-18.0f * sc, hatBaseY);
    ctx.lineTo(-26.0f * sc, hatBaseY - 8.0f * sc);
    ctx.lineTo(-16.0f * sc, hatBaseY - 4.0f * sc);
    ctx.lineTo(-10.0f * sc, hatBaseY - 12.0f * sc);
    ctx.lineTo(-5.0f * sc, hatBaseY - 5.0f * sc);
    ctx.lineTo(3.0f * sc, hatBaseY - 13.0f * sc);
    ctx.lineTo(8.0f * sc, hatBaseY - 4.0f * sc);
    ctx.lineTo(18.0f * sc, hatBaseY - 10.0f * sc);
    ctx.lineTo(17.0f * sc, hatBaseY);
    ctx.closePath();
    ctx.fill();
    ctx.stroke();

    // Aba do chapéu
    ctx.beginPath();
    ctx.ellipse(0.0f, hatBaseY, 32.0f * sc, 8.5f * sc, 0.0f, 0.0f, 2.0f * 3.14159265f);
    ctx.fillStyle = col(sf::Color(228, 188, 82));
    ctx.fill();
    ctx.strokeStyle = ink;
    ctx.lineWidth = 3.0f * sc;
    ctx.stroke();

    if (!isShadow) {
        // Rasgo na borda da aba
        ctx.beginPath();
        ctx.moveTo(-22.0f * sc, hatBaseY + 2.0f * sc);
        ctx.lineTo(-19.0f * sc, hatBaseY - 2.0f * sc);
        ctx.lineTo(-16.0f * sc, hatBaseY + 2.0f * sc);
        ctx.fillStyle = sf::Color(168, 128, 45);
        ctx.fill();
        ctx.strokeStyle = ink;
        ctx.lineWidth = 1.5f * sc;
        ctx.stroke();
    }

    // Copa do chapéu
    ctx.beginPath();
    ctx.moveTo(-16.0f * sc, hatBaseY - 2.0f * sc);
    ctx.lineTo(-11.0f * sc, hatBaseY - 24.0f * sc);
    ctx.bezierCurveTo(-5.0f * sc, hatBaseY - 27.0f * sc, 5.0f * sc, hatBaseY - 27.0f * sc, 11.0f * sc, hatBaseY - 24.0f * sc);
    ctx.lineTo(16.0f * sc, hatBaseY - 2.0f * sc);
    ctx.closePath();
    ctx.fillStyle = col(sf::Color(228, 188, 82));
    ctx.fill();

    if (!isShadow) {
        // Sombra lateral da copa
        ctx.fillStyle = sf::Color(168, 128, 45, 150);
        ctx.beginPath();
        ctx.moveTo(3.0f * sc, hatBaseY - 2.0f * sc);
        ctx.lineTo(5.0f * sc, hatBaseY - 25.0f * sc);
        ctx.bezierCurveTo(8.0f * sc, hatBaseY - 26.0f * sc, 10.0f * sc, hatBaseY - 25.0f * sc, 11.0f * sc, hatBaseY - 24.0f * sc);
        ctx.lineTo(16.0f * sc, hatBaseY - 2.0f * sc);
        ctx.closePath();
        ctx.fill();
    }

    ctx.beginPath();
    ctx.moveTo(-16.0f * sc, hatBaseY - 2.0f * sc);
    ctx.lineTo(-11.0f * sc, hatBaseY - 24.0f * sc);
    ctx.bezierCurveTo(-5.0f * sc, hatBaseY - 27.0f * sc, 5.0f * sc, hatBaseY - 27.0f * sc, 11.0f * sc, hatBaseY - 24.0f * sc);
    ctx.lineTo(16.0f * sc, hatBaseY - 2.0f * sc);
    ctx.closePath();
    ctx.strokeStyle = ink;
    ctx.lineWidth = 3.0f * sc;
    ctx.stroke();

    // Fita vermelha na copa
    ctx.beginPath();
    ctx.roundRect(-14.5f * sc, hatBaseY - 9.0f * sc, 29.0f * sc, 7.0f * sc, 2.0f * sc);
    ctx.fillStyle = col(sf::Color(220, 38, 38));
    ctx.fill();
    ctx.strokeStyle = ink;
    ctx.lineWidth = 1.8f * sc;
    ctx.stroke();

    // Tufinho de palha no topo da copa
    ctx.beginPath();
    ctx.moveTo(-2.0f * sc, hatBaseY - 26.5f * sc);
    ctx.lineTo(2.0f * sc, hatBaseY - 33.0f * sc);
    ctx.lineTo(1.0f * sc, hatBaseY - 26.5f * sc);
    ctx.closePath();
    ctx.fillStyle = col(sf::Color(250, 212, 60));
    ctx.fill();
    ctx.strokeStyle = ink;
    ctx.lineWidth = 1.5f * sc;
    ctx.stroke();

    ctx.restore();
    ctx.flush();
}

} // namespace

void portrait(sf::RenderTarget& target, const SpriteLibrary& sprites, const std::string& key, sf::FloatRect box,
              uint32_t frame, bool animated, sf::Color tint, uint32_t skinId) {
    const auto hd = sprites.find("portrait_hd_" + key + "_skin" + std::to_string(skinId < 4 ? skinId : 0));
    if (hd) {
        const float bob = animated ? std::sin(float(frame) * .05f) * 3.f : 0.f;
        const float breathe = animated ? 1.f + std::sin(float(frame) * .05f) * .012f : 1.f;
        // Optional lower-body bleed does not change the portrait's framing/scale.
        const float frameHeight = std::min(float(hd->rect.size.y), 768.f);
        const float scale = std::min((box.size.x - 20.f) / hd->rect.size.x, (box.size.y - 10.f) / frameHeight) * breathe;
        sf::Sprite art(*hd->texture, hd->rect);
        art.setScale({scale, scale});art.setColor(tint);
        // Raise Cremoso's bust above the name plate in every menu/skin.
        const float lift = key == "cremoso" ? box.size.y * .10f : 0.f;
        art.setPosition({box.position.x + (box.size.x - hd->rect.size.x * scale) * .5f, box.position.y + box.size.y - frameHeight * scale - 2.f + bob - lift});
        target.draw(art);return;
    }
    if (key == "dummy" && skinId == 0) {
        drawEspantalhoPortrait(target, box, frame, animated, tint);
        return;
    }
    const std::string portraitKey = (key == "jack" || key == "frog" || key == "joana" || key == "cremoso" || key == "marcov" || key == "valeria" || key == "vivaldi" || key == "boss" || key == "dummy" || key == "espinha") ? key + "_skin" + std::to_string(skinId < 4 ? skinId : 0) : key;
    auto body = sprites.find("portrait_" + portraitKey + "_front");
    if (!body) body = sprites.find("portrait_" + key + "_front");
    if (!body) return;
    const float w = box.size.x, h = box.size.y;
    const float f = static_cast<float>(frame);
    const float bob = animated ? std::sin(f * 0.05f) * 3.0f : 0.0f;
    const float breathe = animated ? 1.0f + std::sin(f * 0.05f) * 0.012f : 1.0f;

    const float imgW = static_cast<float>(body->rect.size.x), imgH = static_cast<float>(body->rect.size.y);
    const float cropH = imgH * 0.68f;
    const float scale = std::min((w - 20.0f) / imgW, (h - 10.0f) / cropH) * breathe;
    const float targetW = imgW * scale, targetH = cropH * scale;
    const float dx = box.position.x + (w - targetW) / 2.0f;
    const float dy = box.position.y + h - targetH - 2.0f + bob;

    sf::Sprite torso(*body->texture, sf::IntRect(body->rect.position, {body->rect.size.x, static_cast<int>(cropH)}));
    torso.setScale({scale, scale});
    torso.setPosition({dx, dy});
    torso.setColor(tint);
    target.draw(torso);

    const bool espinha = key == "espinha";
    const float gSize = targetW * (espinha ? 0.52f : 0.54f);
    const float gloveY = dy + targetH * (espinha ? 0.35f : 0.32f);
    const float spread = targetW * (espinha ? 0.07f : 0.12f);
    const std::array<std::pair<const char*, sf::Vector2f>, 2> gloves = {{
        {"_gloveL", {dx - spread + (animated ? std::cos(f * 0.05f) * 2.0f : 0.0f), gloveY + (animated ? std::sin(f * 0.08f) * 5.5f : 0.0f)}},
        {"_gloveR", {dx + targetW - gSize + spread - (animated ? std::cos(f * 0.05f) * 2.0f : 0.0f), gloveY + (animated ? std::cos(f * 0.08f) * 5.5f : 0.0f)}},
    }};
    for (const auto& [suffix, at] : gloves) {
        auto glove = sprites.find("portrait_" + portraitKey + suffix);
        if (!glove) glove = sprites.find("portrait_" + key + suffix);
        if (!glove) continue;
        sf::Sprite g(*glove->texture, glove->rect);
        g.setScale({gSize / glove->rect.size.x, gSize / glove->rect.size.y});
        g.setPosition(at);
        g.setColor(tint);
        target.draw(g);
    }
}

void portraitFace(sf::RenderTarget& target, const SpriteLibrary& sprites, const std::string& key, sf::FloatRect box,
                  uint32_t frame, bool animated, sf::Color tint, float rotationDeg, float zoom) {
    if (key == "dummy") {
        Canvas ctx(target);
        const float sc = std::min(box.size.x, box.size.y) / 78.0f * zoom;
        const float cx = box.position.x + box.size.x / 2.0f;
        const float cy = box.position.y + box.size.y / 2.0f + (animated ? std::sin(frame * 0.05f) * 1.5f : 0.0f);
        ctx.save();
        ctx.translate(cx, cy);
        ctx.rotate(rotationDeg * (3.14159265f / 180.0f));
        const float headCY = 6.0f * sc;
        const float hatBaseY = headCY - 18.0f * sc;
        const sf::Color ink = kInk;

        // Corda e gola
        ctx.beginPath();
        ctx.roundRect(-12.0f * sc, headCY + 22.0f * sc, 24.0f * sc, 5.0f * sc, 2.5f * sc);
        ctx.fillStyle = sf::Color(218, 186, 122); ctx.fill();
        ctx.strokeStyle = ink; ctx.lineWidth = 1.8f * sc; ctx.stroke();

        // Cabeça de estopa
        ctx.beginPath();
        ctx.ellipse(0.0f, headCY, 21.0f * sc, 23.0f * sc, 0.0f, 0.0f, 2.0f * 3.14159265f);
        ctx.fillStyle = sf::Color(218, 172, 98); ctx.fill();
        ctx.strokeStyle = ink; ctx.lineWidth = 2.8f * sc; ctx.stroke();

        // Bochechas
        ctx.fillStyle = sf::Color(235, 95, 95, 120);
        ctx.beginPath(); ctx.ellipse(-12.0f * sc, headCY + 5.0f * sc, 4.5f * sc, 3.0f * sc, 0, 0, 2*3.14159265f); ctx.fill();
        ctx.beginPath(); ctx.ellipse(12.0f * sc, headCY + 5.0f * sc, 4.5f * sc, 3.0f * sc, 0, 0, 2*3.14159265f); ctx.fill();

        // Olhos
        ctx.beginPath(); ctx.arc(-9.0f * sc, headCY, 5.5f * sc, 0.0f, 2.0f * 3.14159265f);
        ctx.fillStyle = sf::Color(45, 30, 20); ctx.fill();
        ctx.strokeStyle = ink; ctx.lineWidth = 1.8f * sc; ctx.stroke();
        ctx.strokeStyle = sf::Color(250, 240, 180); ctx.lineWidth = 1.5f * sc;
        ctx.beginPath(); ctx.moveTo(-11.5f * sc, headCY - 2.5f * sc); ctx.lineTo(-6.5f * sc, headCY + 2.5f * sc);
        ctx.moveTo(-6.5f * sc, headCY - 2.5f * sc); ctx.lineTo(-11.5f * sc, headCY + 2.5f * sc); ctx.stroke();

        ctx.beginPath(); ctx.arc(8.5f * sc, headCY - 0.5f * sc, 4.2f * sc, 0.0f, 2.0f * 3.14159265f);
        ctx.fillStyle = sf::Color(20, 22, 25); ctx.fill();
        ctx.strokeStyle = ink; ctx.lineWidth = 1.6f * sc; ctx.stroke();
        ctx.beginPath(); ctx.arc(7.2f * sc, headCY - 1.8f * sc, 1.2f * sc, 0.0f, 2.0f * 3.14159265f);
        ctx.fillStyle = sf::Color(255, 255, 255); ctx.fill();

        // Nariz
        ctx.beginPath(); ctx.moveTo(0.0f, headCY + 1.0f * sc); ctx.lineTo(-3.5f * sc, headCY + 6.0f * sc); ctx.lineTo(3.5f * sc, headCY + 6.0f * sc); ctx.closePath();
        ctx.fillStyle = sf::Color(190, 75, 55); ctx.fill(); ctx.strokeStyle = ink; ctx.lineWidth = 1.4f * sc; ctx.stroke();

        // Boca
        ctx.strokeStyle = ink; ctx.lineWidth = 2.0f * sc;
        ctx.beginPath(); ctx.moveTo(-13.0f * sc, headCY + 11.0f * sc); ctx.quadraticCurveTo(0.0f, headCY + 17.0f * sc, 13.0f * sc, headCY + 11.0f * sc); ctx.stroke();

        // Chapéu
        ctx.beginPath(); ctx.ellipse(0.0f, hatBaseY, 32.0f * sc, 8.5f * sc, 0.0f, 0.0f, 2.0f * 3.14159265f);
        ctx.fillStyle = sf::Color(228, 188, 82); ctx.fill(); ctx.strokeStyle = ink; ctx.lineWidth = 2.6f * sc; ctx.stroke();

        ctx.beginPath(); ctx.moveTo(-16.0f * sc, hatBaseY - 2.0f * sc); ctx.lineTo(-11.0f * sc, hatBaseY - 24.0f * sc);
        ctx.bezierCurveTo(-5.0f * sc, hatBaseY - 27.0f * sc, 5.0f * sc, hatBaseY - 27.0f * sc, 11.0f * sc, hatBaseY - 24.0f * sc);
        ctx.lineTo(16.0f * sc, hatBaseY - 2.0f * sc); ctx.closePath();
        ctx.fillStyle = sf::Color(228, 188, 82); ctx.fill(); ctx.strokeStyle = ink; ctx.lineWidth = 2.6f * sc; ctx.stroke();

        ctx.beginPath(); ctx.roundRect(-14.5f * sc, hatBaseY - 9.0f * sc, 29.0f * sc, 7.0f * sc, 2.0f * sc);
        ctx.fillStyle = sf::Color(220, 38, 38); ctx.fill(); ctx.strokeStyle = ink; ctx.lineWidth = 1.6f * sc; ctx.stroke();

        ctx.restore();
        ctx.flush();
        return;
    }
    auto pic = sprites.find("portrait_" + key + "_front");
    if (!pic) return;
    struct Crop { const char* key; float cx, top, size; };
    static const Crop kCrop[] = {{"jack", 0.5f, 0.0f, 0.6f}, {"frog", 0.5f, 0.0f, 0.58f}, {"joana", 0.5f, 0.0f, 0.5f},
                                 {"cremoso", 0.5f, 0.0f, 0.6f}, {"valeria", 0.5f, 0.0f, 0.5f}, {"vivaldi", 0.5f, 0.1f, 0.44f},
                                 {"espinha", 0.5f, 0.0f, 0.5f}, {"marcov", 0.5f, 0.12f, 0.48f}, {"boss", 0.5f, 0.1f, 0.52f}};
    Crop c{"", 0.5f, 0.0f, 0.55f};
    for (const Crop& k : kCrop) if (key == k.key) c = k;
    const float iw = static_cast<float>(pic->rect.size.x), ih = static_cast<float>(pic->rect.size.y), sz = c.size * iw;
    sf::Sprite bust(*pic->texture, sf::IntRect({pic->rect.position.x + static_cast<int>(c.cx * iw - sz / 2), pic->rect.position.y + static_cast<int>(c.top * ih)},
                                               {static_cast<int>(sz), static_cast<int>(sz)}));
    const float k = std::min(box.size.x, box.size.y) / sz * zoom;
    const float bob = animated ? std::sin(frame * 0.05f) * 1.5f : 0.0f;
    bust.setOrigin({sz / 2, sz / 2});
    bust.setScale({k, k});
    bust.setRotation(sf::degrees(rotationDeg));
    bust.setPosition({box.position.x + box.size.x / 2, box.position.y + box.size.y / 2 + bob});
    bust.setColor(tint);
    target.draw(bust);
}

namespace {
sf::Color hex(uint32_t rgb) {
    return sf::Color(static_cast<std::uint8_t>(rgb >> 16), static_cast<std::uint8_t>(rgb >> 8), static_cast<std::uint8_t>(rgb));
}
} // namespace

sf::Color skillTagColor(const std::string& tag) {
    if (tag == "SWARM") return hex(0xfbbf24);
    if (tag == "HONEY") return hex(0xf59e0b);
    if (tag == "SHIELD") return hex(0x38bdf8);
    if (tag == "STING") return hex(0xef4444);
    if (tag == "POLLEN") return hex(0xfacc15);
    if (tag == "HEAL") return hex(0x10b981);
    if (tag == "REACH") return hex(0xfb7185);
    if (tag == "VENOM") return hex(0xa3e635);
    if (tag == "IMPACT") return hex(0xf97316);
    if (tag == "SLOW") return hex(0xb45309);
    if (tag == "BURST") return hex(0x22d3ee);
    if (tag == "BREAK") return hex(0xdc2626);
    if (tag == "STUN") return hex(0x0ea5e9);
    if (tag == "BURN") return hex(0xf97316);
    if (tag == "ABSORB") return hex(0xfde68a);
    if (tag == "KNOCKDOWN") return hex(0xb91c1c);
    if (tag == "RHYTHM") return hex(0xc084fc);
    if (tag == "BOOST") return hex(0x34d399);
    if (tag == "DRAIN") return hex(0xa855f7);
    if (tag == "ROCKET") return hex(0xf43f5e);
    if (tag == "BARRIER") return hex(0x22d3ee);
    return hex(0xa78bfa); // PASSIVA
}

void commandHint(sf::RenderTarget& target, const sf::Font* font, SkillCommand cmd, float rightX, float centerY, sf::Color color,
                 const std::string& punchL, const std::string& punchR) {
    enum Tok { L, R, U, D, PLUS, PUNCH_L, PUNCH_R };
    std::vector<Tok> toks;
    switch (cmd) {
    case SkillCommand::BackForward: toks = {L, R, PLUS, PUNCH_R}; break;
    case SkillCommand::DownUp: toks = {D, U, PLUS, PUNCH_R}; break;
    case SkillCommand::BothPunches: toks = {PUNCH_L, PLUS, PUNCH_R}; break;
    case SkillCommand::UpUpRight: toks = {U, U, PLUS, PUNCH_R}; break;
    case SkillCommand::DownDownLeft: toks = {D, D, PLUS, PUNCH_L}; break;
    case SkillCommand::BackBackLeft: toks = {L, L, PLUS, PUNCH_L}; break;
    case SkillCommand::ForwardForwardRight: toks = {R, R, PLUS, PUNCH_R}; break;
    case SkillCommand::BackForwardLeft: toks = {L, R, PLUS, PUNCH_L}; break;
    case SkillCommand::None: return;
    }
    const float step = 13.0f;
    float x = rightX - step * static_cast<float>(toks.size());
    for (Tok tk : toks) {
        const float cx = x + step * 0.5f;
        if (tk <= D) {
            sf::ConvexShape tri(3);
            const float s = 5.0f;
            if (tk == L) { tri.setPoint(0, {cx - s, centerY}); tri.setPoint(1, {cx + s, centerY - s}); tri.setPoint(2, {cx + s, centerY + s}); }
            if (tk == R) { tri.setPoint(0, {cx + s, centerY}); tri.setPoint(1, {cx - s, centerY - s}); tri.setPoint(2, {cx - s, centerY + s}); }
            if (tk == U) { tri.setPoint(0, {cx, centerY - s}); tri.setPoint(1, {cx + s, centerY + s}); tri.setPoint(2, {cx - s, centerY + s}); }
            if (tk == D) { tri.setPoint(0, {cx, centerY + s}); tri.setPoint(1, {cx + s, centerY - s}); tri.setPoint(2, {cx - s, centerY - s}); }
            tri.setFillColor(color);
            tri.setOutlineColor(kInk);
            tri.setOutlineThickness(1.5f);
            target.draw(tri);
        } else if (font) {
            const std::string label = tk == PLUS ? "+" : (tk == PUNCH_L ? punchL : punchR);
            sf::Text t(*font, label, 14);
            t.setFillColor(color);
            t.setOutlineColor(kInk);
            t.setOutlineThickness(1.5f);
            sf::FloatRect b = t.getLocalBounds();
            t.setPosition({std::round(cx - (b.position.x + b.size.x / 2.0f)), std::round(centerY - 10.0f)});
            target.draw(t);
        }
        x += step;
    }
}


} // namespace KoRama::ui
