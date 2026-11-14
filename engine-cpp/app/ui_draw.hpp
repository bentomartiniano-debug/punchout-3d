#ifndef KORAMA_UI_DRAW_HPP
#define KORAMA_UI_DRAW_HPP

#include "skills.hpp"
#include "sprite_atlas.hpp"
#include <SFML/Graphics.hpp>
#include <string>

namespace KoRama::ui {

// Paleta HQ do jogo (css/hud.css e css/menus.css).
inline const sf::Color kInk(0, 0, 0);
inline const sf::Color kYellow(0xff, 0xd2, 0x3f);
inline const sf::Color kMagenta(0xff, 0x00, 0x7f);
inline const sf::Color kCyan(0x00, 0xf5, 0xd4);
inline const sf::Color kOrange(0xff, 0x66, 0x00);
inline const sf::Color kRed(0xff, 0x1e, 0x42);
inline const sf::Color kSlate(0x33, 0x41, 0x55);
inline const sf::Color kNight(0x0b, 0x09, 0x10);
inline const sf::Color kPanel(0x17, 0x12, 0x22);
inline const sf::Color kPanelEdge(0x2d, 0x24, 0x42);
inline const sf::Color kMuted(0x8c, 0x82, 0xa0);

// Texto UTF-8 com contorno de nanquim. anchorX: 0 = esquerda, 0,5 = centro, 1 = direita.
void text(sf::RenderTarget& target, const sf::Font* font, const std::string& utf8, sf::Vector2f pos, unsigned size,
          sf::Color color, float anchorX = 0.0f, float outline = -1.0f);

// Quebra o texto em linhas que caibam em maxWidth e desenha a partir de pos.
void wrappedText(sf::RenderTarget& target, const sf::Font* font, const std::string& utf8, sf::Vector2f pos,
                 unsigned size, sf::Color color, float maxWidth, float lineHeight, float anchorX = 0.0f);

// Painel com borda preta e sombra dura deslocada (sem desfoque), como os cards do HTML.
void panel(sf::RenderTarget& target, sf::FloatRect rect, sf::Color fill, sf::Color border = kInk,
           float borderWidth = 3.0f, float shadow = 6.0f, float slant = 0.0f);

void bar(sf::RenderTarget& target, sf::FloatRect rect, float ratio, sf::Color fill, sf::Color back = sf::Color(0x14, 0x0d, 0x22));

// Fundo escuro com malha de pontinhos (halftone).
void dotBackground(sf::RenderTarget& target, sf::Vector2f size, uint32_t frame);

// Cor da etiqueta de cada tipo de habilidade (skill-tag-* do index.html, mais as do King Frog).
sf::Color skillTagColor(const std::string& tag);

// Setas e teclas de um comando (← → + X etc.), alinhadas à direita em rightX.
void commandHint(sf::RenderTarget& target, const sf::Font* font, SkillCommand cmd, float rightX, float centerY, sf::Color color,
                 const std::string& punchL = "Z", const std::string& punchR = "X");

// Retrato como renderCharacterPortrait() do JS: corpo cortado em 68% da altura + luvas flutuando.
void portrait(sf::RenderTarget& target, const SpriteLibrary& sprites, const std::string& key, sf::FloatRect box,
              uint32_t frame, bool animated, sf::Color tint = sf::Color::White, uint32_t skinId = 0);

// Close do rosto (cabeça e ombros do retrato) preenchendo o quadro; usado na HUD e no chaveamento.
void portraitFace(sf::RenderTarget& target, const SpriteLibrary& sprites, const std::string& key, sf::FloatRect box,
                  uint32_t frame, bool animated, sf::Color tint = sf::Color::White, float rotationDeg = 0.0f, float zoom = 1.0f);

} // namespace KoRama::ui

#endif // KORAMA_UI_DRAW_HPP
