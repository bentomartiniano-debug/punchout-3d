#ifndef KORAMA_LIVE_RIG_HPP
#define KORAMA_LIVE_RIG_HPP

// Esqueleto ao vivo do elenco (tools/export_sprites/lab2/export_rig.mjs): cada peça do lutador é uma
// imagem só; cada quadro de cada animação (60 fps) diz quais peças desenhar, em que ordem, com que
// matriz (unidades do palco da apresentação: viewBox -30 -42 180 160, pés em 60,108) e opacidade.
#include <SFML/Graphics.hpp>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace KoRama {

struct LiveRig {
    enum SlotKind : uint8_t {
        SlotOther = 0,
        SlotGloveL = 1,
        SlotGloveR = 2,
        SlotLegL = 3,
        SlotLegR = 4,
        SlotTorso = 5
    };
    struct Part { int sheet; sf::FloatRect rect; float ox, oy, upp; };
    struct Inst { int part; float a, b, c, d, e, f, op; int glove; uint8_t slotKind = SlotOther; }; // glove: 1 = esquerda, 2 = direita, 0 = outra peça
    struct Frame { float opacity; std::uint32_t first, count; };
    struct Clip { bool loop = false; float dur = 0; float box[4] = {0, 0, 120, 120}; std::vector<Frame> frames; }; // box: viewBox do sprite inteiro
    int fps = 60;
    std::vector<std::unique_ptr<sf::Texture>> sheets;
    std::vector<Part> parts;
    std::vector<Inst> insts;
    std::unordered_map<std::string, Clip> clips; // "forma|F|animação"

    bool load(const std::filesystem::path& dir, const std::string& key) { return parse(dir, key) && upload(); }
    // parse lê a trilha e decodifica as folhas (pode rodar em outra thread); upload cria as texturas.
    bool parse(const std::filesystem::path& dir, const std::string& key);
    bool upload();
    // -1 failure, 0 more work, 1 ready. Used only outside combat drawing.
    int uploadStep(std::size_t byteBudget = 4u * 1024u * 1024u);
    std::size_t uploadPage = 0;
    unsigned uploadRow = 0;
    std::vector<sf::Image> images; // folhas decodificadas, esperando o upload
    const Clip* clip(const std::string& form, bool back, const std::string& name) const;
    bool loadBinary(const std::filesystem::path& rigbPath, uint64_t expectedTrackSize, uint64_t expectedTrackModTime, int& outNSheets);
    bool saveBinary(const std::filesystem::path& rigbPath, uint64_t trackFileSize, uint64_t trackModTime, int nSheets) const;
};

} // namespace KoRama

#endif
