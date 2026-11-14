#ifndef KORAMA_SPRITE_ATLAS_HPP
#define KORAMA_SPRITE_ATLAS_HPP

#include "live_rig.hpp"
#include <SFML/Graphics.hpp>
#include <filesystem>
#include <future>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <unordered_set>

namespace KoRama {

// Saved skin IDs are stable; never derive them from thumbnail ordering.
inline bool hasIntegratedSkin(uint32_t character) { return character < 10; }
inline std::string fighterRigKey(uint32_t character, uint32_t skin) {
    static const char* keys[9] = {"jack", "frog", "joana", "cremoso", "valeria", "vivaldi", "espinha", "marcov", "boss"};
    if (character == 0) return "jack_skin" + std::to_string(skin < 4 ? skin : 0);
    if (character == 1) return "frog_skin" + std::to_string(skin < 4 ? skin : 0);
    if (character == 2) return "joana_skin" + std::to_string(skin < 4 ? skin : 0);
    if (character == 3) return "cremoso_skin" + std::to_string(skin < 4 ? skin : 0);
    if (character == 4) return "valeria_skin" + std::to_string(skin < 4 ? skin : 0);
    if (character == 5) return "vivaldi_skin" + std::to_string(skin < 4 ? skin : 0);
    if (character == 6) return "espinha_skin" + std::to_string(skin < 4 ? skin : 0);
    if (character == 7) return "marcov_skin" + std::to_string(skin < 4 ? skin : 0);
    if (character == 8) return "boss_skin" + std::to_string(skin < 4 ? skin : 0);
    if (character == 9) return "dummy_skin" + std::to_string(skin < 4 ? skin : 0);
    return character < 9 ? keys[character] : "";
}

struct SpriteFrame {
    const sf::Texture* texture = nullptr;
    sf::IntRect rect;
    // Partes recortadas (esqueleto): onde o recorte ficava no quadro original e o tamanho desse quadro.
    sf::Vector2i trimOffset{0, 0};
    sf::Vector2i fullSize{0, 0};
};

// Spritesheets geradas por tools/export_sprites: <nome>.png + <nome>.atlas.
class SpriteLibrary {
public:
    // Carrega assets/sprites/<sheet>.png/.atlas; devolve false se faltar algum arquivo.
    bool loadSheet(const std::filesystem::path& spritesDir, const std::string& sheet);
    // Várias folhas: decodifica os PNGs em paralelo e só sobe as texturas na thread principal.
    void loadSheets(const std::filesystem::path& spritesDir, const std::vector<std::string>& sheets);
    std::optional<SpriteFrame> find(const std::string& name) const;
    // Visual novo do elenco (live_<lutador>): carregado sob demanda na primeira vez que é desenhado;
    // keepRigs libera os outros (chamado no começo de cada luta, fora do desenho).
    void setRigDir(const std::filesystem::path& spritesDir) { rigDir_ = spritesDir; }
    const std::filesystem::path& rigDirectory() const { return rigDir_; }
    void loadRig(const std::filesystem::path& spritesDir, const std::string& key);
    void keepRigs(const std::vector<std::string>& keys);
    const LiveRig* rig(const std::string& key) const;
    // Começa a carregar em segundo plano (sem travar); rig() devolve nullptr até ficar pronto.
    void requestRig(const std::string& key) const;
    // Sobe para a GPU texturas decodificadas em segundo plano antes da luta começar (limite de 1 por quadro).
    bool pollUpload();
    void loadMenuSheets(const std::filesystem::path& dir);
    void keepFighterSheets(const std::filesystem::path& dir, const std::vector<uint32_t>& characters);
    void requestFighterSheets(const std::filesystem::path& dir, const std::vector<uint32_t>& characters);
    bool fighterSheetsReady() const;
    bool fighterSheetsFailed() const { return !sheetMissing_.empty(); }
    bool rigsReady(const std::vector<std::string>& keys) const;
    bool rigFailed(const std::string& key) const { return rigMissing_.count(key) != 0; }
    bool waitForRigs = false; // ferramentas (snapshot): espera a carga em vez de desenhar o visual antigo
    void dumpDebug(std::ostream& os) const {
        os << "sheetDir: " << sheetDir_ << "\nrigDir: " << rigDir_ << "\n";
        os << "wantedSheets: "; for (auto& s : wantedSheets_) os << s << " "; os << "\n";
        os << "loadedSheets: "; for (auto& s : loadedSheets_) os << s << " "; os << "\n";
        os << "queuedSheets: " << queuedSheets_.size() << " pendingSheets: " << pendingSheets_.size() << " uploadingSheets: " << uploadingSheets_.size() << "\n";
        os << "sheetMissing: "; for (auto& s : sheetMissing_) os << s << " "; os << "\n";
        os << "wantedRigs: "; for (auto& s : wantedRigs_) os << s << " "; os << "\n";
        os << "queuedRigs: " << queuedRigs_.size() << " pendingRigs: " << pending_.size() << " uploadingRigs: " << uploading_.size() << "\n";
        os << "rigMissing: "; for (auto& s : rigMissing_) os << s.first << " "; os << "\n";
        os << "rigs_: "; for (auto& s : rigs_) os << s.first << " "; os << "\n";
    }

private:
    std::vector<std::unique_ptr<sf::Texture>> textures_; // ponteiros estáveis para os SpriteFrame
    std::unordered_map<std::string, SpriteFrame> frames_;
    mutable std::unordered_map<std::string, std::unique_ptr<LiveRig>> rigs_;
    mutable std::unordered_map<std::string, bool> rigMissing_;
    mutable std::unordered_map<std::string, std::future<std::unique_ptr<LiveRig>>> pending_;
    std::unordered_map<const sf::Texture*, std::string> textureSheets_;
    std::unordered_set<std::string> loadedSheets_;
    mutable std::vector<std::string> queuedRigs_;
    mutable std::unordered_map<std::string, std::unique_ptr<LiveRig>> uploading_;
    std::unordered_set<std::string> wantedRigs_;
    bool limitedRigs_ = false;
    bool wanted(const std::string& key) const { return !limitedRigs_ || wantedRigs_.count(key) != 0; }
    void pumpRequests() const;
    bool addSheet(const std::string& sheet, sf::Image&& image, const std::filesystem::path& atlasPath);
    bool registerSheet(const std::string& sheet, std::unique_ptr<sf::Texture> texture, const std::filesystem::path& atlasPath);
    std::vector<std::string> selectFighterSheets(const std::filesystem::path& dir, const std::vector<uint32_t>& characters);
    void pumpSheets();
    bool pollSheets();
    struct SheetUpload { sf::Image image; std::unique_ptr<sf::Texture> texture; unsigned row = 0; };
    std::unordered_map<std::string, std::future<std::optional<sf::Image>>> pendingSheets_;
    std::unordered_map<std::string, SheetUpload> uploadingSheets_;
    std::vector<std::string> queuedSheets_, requiredSheets_;
    std::unordered_set<std::string> sheetMissing_, wantedSheets_;
    std::filesystem::path sheetDir_;
    std::filesystem::path rigDir_;
};

} // namespace KoRama

#endif // KORAMA_SPRITE_ATLAS_HPP
