#ifndef KORAMA_GAME_APP_HPP
#define KORAMA_GAME_APP_HPP

#include "audio_system.hpp"
#include "engine.hpp"
#include "menu_screens.hpp"
#include "ring_renderer.hpp"
#include "sprite_atlas.hpp"
#include <SFML/Graphics.hpp>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <optional>

namespace KoRama {

// Janela, loop fixo de 60 ticks/s e tradução do teclado para a máscara de entrada do motor.
class GameApp {
public:
    explicit GameApp(const std::filesystem::path& assetRoot);
    int run();
    int benchmarkVersus(const std::filesystem::path& outCsv, uint32_t player, uint32_t playerSkin,
                        uint32_t enemy, uint32_t enemySkin, int uploadPolls);
    void dumpTestFrame(const std::filesystem::path& outPng, bool training = false, int punchTick = 0);
    void dumpMenuFrame(const std::filesystem::path& outPng, int modeIndex = 2);
    void dumpBuildFrame(const std::filesystem::path& outPng, uint32_t charId = 3, uint32_t skinId = 0);
    void dumpBootFrames();

    // Procura a pasta assets/ ao lado do executável, acima dele (build/<preset>/) ou no diretório atual.
    static std::filesystem::path findAssetRoot(const char* argv0);

private:
    void processEvents();
    void onKeyPressed(sf::Keyboard::Key key);
    uint32_t buildInputMask() const;
    void tick();
    void render();
    void updateLetterbox(sf::Vector2u windowSize);
    void warmUpCaches();
    void handleMenuAction(const MenuAction& action);
    void startMatch(uint32_t playerChar, uint32_t enemyChar, const SkillLoadout& loadout, bool arcade = false, int stage = 1, bool training = false);
    void cycleTrainingFighter(bool forward);
    void prepareFighters(uint32_t playerChar, uint32_t enemyChar);
    bool fightersReady() const;
    std::filesystem::path savePath() const;
    void loadBuilds();
    void saveBuilds();
    void leaveFight();
    void stepReplay();
    bool resultVisible() const;
    void onFightClick(sf::Vector2f p);
    void setPaused(bool paused);
    MusicTrack charTrack(uint32_t charId) const;
    MusicTrack currentMatchTrack() const;

    std::filesystem::path assetRoot_;
    sf::RenderWindow window_;
    sf::View view_;    // cena da luta: 800×600 esticado em 16:9
    sf::View uiView_;  // HUD e menus: 1600×900
    std::optional<sf::Font> displayFont_;
    std::optional<sf::Font> digitFont_;
    SpriteLibrary sprites_;
    RingRenderer renderer_;
    MenuScreens menus_;
    GameEngine engine_;
    AudioSystem audio_;
    Screen lastScreen_ = Screen::Fight;

    std::vector<std::string> fightRigKeys_;
    bool loadingFighters_ = false;
    uint32_t charSelectPreloadChar_ = ~0u;
    uint32_t charSelectPreloadSkin_ = ~0u;
    bool announceLoadedFight_ = false;
    int versusUploadPolls_ = 1; // spread GPU uploads across the versus animation
    double lastVersusUploadMs_ = 0.0;
    std::optional<MenuAction> pendingMatch_;
    bool pendingTrainingSwap_ = false;
    bool paused_ = false;
    float freeCamSpeed_ = 600.0f;
    sf::Vector2i freeCamLastMousePos_{};
    uint32_t tapLatch_ = 0; // teclas apertadas desde o último tick
    KeyBindings keyBindings_ = KeyBindings::makeDefault();
    uint32_t matchPlayer_ = 0;
    uint32_t matchEnemy_ = 1;
    SkillLoadout matchLoadout_;
    bool matchArcade_ = false;
    int matchStage_ = 1;
    bool matchTraining_ = false;
    bool arcadeResultApplied_ = false; // vitória/derrota do torneio já registrada nesta luta
    bool matchMusicStopped_ = false;
    sf::Time knockoutMusicOffset_ = sf::Time::Zero;

    // Replay do nocaute: os últimos 3 s da luta (cópias do motor) e o estado da fita.
    std::deque<GameEngine> replay_;
    int endFrames_ = 0;          // quadros desde o fim da luta
    bool replayActive_ = false;
    bool replayDone_ = false;
    int replayPhase_ = 0;        // 0 rebobinando, 1 sua visão, 2 visão do oponente
    int rewindTimer_ = 0;
    float replayTimer_ = 0.0f;
    int lastReplayFrame_ = -1;
    bool celebrationAudioStarted_ = false;
};

} // namespace KoRama

#endif // KORAMA_GAME_APP_HPP
