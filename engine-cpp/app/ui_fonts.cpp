#include "ui_fonts.hpp"
#include "canvas2d.hpp"
#include <array>
#include <optional>

namespace KoRama::ui {

namespace {
std::array<std::optional<sf::Font>, static_cast<std::size_t>(Font::Count)>& store() {
    static std::array<std::optional<sf::Font>, static_cast<std::size_t>(Font::Count)> fonts;
    return fonts;
}
} // namespace

void loadFonts(const std::filesystem::path& dir) {
    struct Entry { Font id; const char* file; const char* family; };
    const Entry entries[] = {
        {Font::Titan, "TitanOne-Regular.ttf", "titan one"},
        {Font::Outfit, "Outfit-Black.ttf", "outfit"},
        {Font::OutfitBold, "Outfit-ExtraBold.ttf", "outfit extrabold"},
        {Font::OutfitSemi, "Outfit-SemiBold.ttf", "outfit semibold"},
        {Font::Montserrat, "Montserrat-Black.ttf", "montserrat"},
        {Font::Bangers, "Bangers-Regular.ttf", "bangers"},
        {Font::Pixel, "PressStart2P-Regular.ttf", "press start 2p"},
    };
    for (const Entry& e : entries) {
        auto& slot = store()[static_cast<std::size_t>(e.id)];
        if (slot) continue;
        sf::Font f;
        if (!f.openFromFile(dir / e.file)) continue;
        slot = std::move(f);
        cv::registerFont(e.family, &*slot);
    }
}

const sf::Font* font(Font which) {
    auto& slot = store()[static_cast<std::size_t>(which)];
    return slot ? &*slot : nullptr;
}

} // namespace KoRama::ui
