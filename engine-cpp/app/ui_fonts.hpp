#ifndef KORAMA_APP_UI_FONTS_HPP
#define KORAMA_APP_UI_FONTS_HPP

#include <SFML/Graphics/Font.hpp>
#include <filesystem>

// Fontes da interface do jogo original: Titan One (logo e títulos), Outfit (texto), Montserrat Black,
// Bangers (HUD em itálico) e Press Start 2P (números e botões retrô). Carregadas uma vez e
// registradas no Canvas2D com os nomes CSS ("Titan One", "Outfit", ...).
namespace KoRama::ui {

enum class Font { Titan, Outfit, OutfitBold, OutfitSemi, Montserrat, Bangers, Pixel, Count };

void loadFonts(const std::filesystem::path& fontDir);
const sf::Font* font(Font which); // nullptr se a fonte não existir

} // namespace KoRama::ui

#endif
