#ifndef KORAMA_MENU_SCREENS_HPP
#define KORAMA_MENU_SCREENS_HPP

#include "ring_renderer.hpp" // HudFonts
#include "skills.hpp"
#include "sprite_atlas.hpp"
#include "preview_video.hpp"
#include "tournament.hpp"
#include "key_bindings.hpp"
#include <array>
#include <memory>
#include <vector>
#include <SFML/Graphics.hpp>
#include <cstdint>
#include <string>

namespace KoRama {

namespace cv { class Canvas; }

enum class Screen { Boot, Title, CharSelect, ModeSelect, Build, Versus, Fight, Bracket, CampaignPrompt };

struct MenuAction {
    enum class Type { None, StartMatch, Quit } type = Type::None;
    uint32_t playerChar = 0;
    uint32_t enemyChar = 1;
    uint32_t playerSkin = 0;
    uint32_t enemySkin = 0;
    SkillLoadout loadout;  // build equipada do jogador
    bool arcade = false;   // luta do Torneio Arcade
    int stage = 1;         // fase do torneio (4 = chefe)
    bool training = false; // Modo Treino contra o Espantalho
};

// Título → seleção de lutador → seleção de modo → tela VS, como o hub do index.html: logo que
// desliza do centro para o topo, painéis que entram com mola, modais de configurações e de saída.
// Tudo é desenhado no espaço de interface 1600×900 (o mouse chega nas mesmas coordenadas).
// Teclado (setas/WASD, Enter/Z confirma, Esc/X volta) e mouse.
class MenuScreens {
public:
    MenuScreens(HudFonts fonts, const SpriteLibrary* sprites);

    Screen screen() const { return screen_; }
    int screenFrame() const { return screenFrame_; }
    void goTo(Screen screen);
    uint32_t selectedChar() const { return playerChar_; }
    void setSelectedChar(uint32_t c) { playerChar_ = c; }
    uint32_t opponentChar() const { return enemyChar_; }
    void setOpponentChar(uint32_t c) { enemyChar_ = c; }
    void setVersusTag(const char* tag) { versusTag_ = tag; }
    void setVersusReady(bool ready) { versusReady_ = ready; }
    MenuAction versusMatch() const { return startAction(); }
    void versusFailure() { notify("Falha ao carregar lutador. ESC para voltar."); }
    int modeIndex() const { return modeIndex_; }
    void setModeIndex(int idx) { modeIndex_ = idx; }

    // Skins de cada lutador (0 a 3).
    uint32_t skin(uint32_t charId) const { return charId < skins_.size() ? skins_[charId] : 0; }
    void setSkin(uint32_t charId, uint32_t s) { if (charId < skins_.size()) skins_[charId] = s % 4; }
    void cycleSkin(uint32_t charId, int dir);
    const std::array<uint32_t, 10>& skins() const { return skins_; }
    sf::FloatRect skinPillRect() const;
    sf::FloatRect skinPrevButton() const;
    sf::FloatRect skinNextButton() const;

    // Builds de cada lutador (índice = char_id). takeDirty() avisa quando o jogador mudou alguma.
    const std::array<SkillLoadout, 10>& loadouts() const { return loadouts_; }
    void setLoadout(uint32_t charId, const SkillLoadout& loadout);
    bool takeDirty();
    void openBuild(Screen returnTo);

    // Volumes da tela de configurações (0 a 100).
    int sfxVolume() const { return sfxVolume_; }
    int musicVolume() const { return musicVolume_; }
    void setVolumes(int sfx, int music) { sfxVolume_ = sfx; musicVolume_ = music; }

    const KeyBindings& keyBindings() const { return keyBindings_; }
    void setKeyBindings(const KeyBindings& kb) { keyBindings_ = kb; }
    int settingsTab() const { return settingsTab_; }
    void setSettingsTab(int tab) { settingsTab_ = tab; }
    int rebindAction() const { return rebindAction_; }
    int rebindSlot() const { return rebindSlot_; }

    // Torneio Arcade em andamento (salvo junto com as builds).
    const Tournament& tournament() const { return tournament_; }
    void setTournament(const Tournament& t) { tournament_ = t; }
    void tournamentWon();   // avança a chave depois da vitória do jogador
    void tournamentLost();  // derrota encerra o torneio, como no JS
    void showBracket();
    void newCampaign();     // novo torneio com o lutador escolhido

    // Jogo 3D: o palco da seleção (e da escolha de modo) mostra a arena do lutador girando no fundo e o modelo 3D no lugar do retrato.
    // background: o quadro da arena (orientação normal); fighter: só o lutador com alfa, de cabeça para baixo (saída do OpenGL). Nulos = visual 2D.
    void setStage3D(const sf::Texture* background, const sf::Texture* fighter) { stageBg_ = background; stageFighter_ = fighter; }
    int stageFocus() const;   // lutador no palco agora; -1 = aleatório ou tela sem palco
    // Jogo 3D: a prévia da build mostra o golpe de verdade (motor e efeitos da luta) num quadro da janela (orientação normal); nulo = vídeo gravado.
    // O jogo pergunta qual golpe, qual lutador e qual vista (V alterna: false = nossas costas, true = do lado do rival).
    void setLivePreview(const sf::Texture* frame) { livePreview_ = frame; }
    SkillId livePreviewSkill() const { return screen_ == Screen::Build ? previewSkill_ : SkillId::None; }
    uint32_t livePreviewChar() const { return playerChar_; }
    bool livePreviewFront() const { return previewFront_; }

    MenuAction onKey(sf::Keyboard::Key key);
    void onMouseMove(sf::Vector2f p);
    MenuAction onClick(sf::Vector2f p);
    void onMouseDrag(sf::Vector2f p); // botão esquerdo segurado (barras de volume)
    MenuAction onRightClick();         // clique direito = voltar (Esc)
    MenuAction update(); // um tick de 60 Hz (animações e fim da tela VS)
    void draw(sf::RenderTarget& target);
    void warmUp(sf::RenderTarget& target); // pré-carrega textos e texturas de todas as telas

    // Modais do título.
    enum class Modal { None, Settings, Exit };
    Modal modal() const { return modal_; }
    void openModal(Modal m) { modal_ = m; modalFrame_ = 0; modalIndex_ = m == Modal::Exit ? 0 : 0; }

private:
    // Seleção de lutador: 9 posições, a 3ª é "aleatório".
    static constexpr int kSlots = 9;
    static constexpr int kRandomSlot = 4;
    int slotChar(int slot) const;           // -1 = aleatório
    sf::FloatRect cardRect(int slot) const;
    sf::FloatRect titleButton(int i) const;
    sf::FloatRect charButton(int i) const;
    sf::FloatRect modeButton(int i) const;
    sf::FloatRect buildCard(int i) const;
    sf::FloatRect buildSlot(int slot) const;
    sf::FloatRect buildSaveButton() const;
    sf::FloatRect bracketButton(int i) const;
    sf::FloatRect promptButton(int i) const;
    sf::FloatRect settingsSlider(int i) const;
    sf::FloatRect settingsTab(int tab) const;
    sf::FloatRect settingsPreset(int p) const;
    sf::FloatRect settingsActionKey(int action, int slot) const;
    sf::FloatRect settingsReset() const;
    sf::FloatRect settingsOk() const;
    sf::FloatRect exitButton(int i) const;
    std::vector<SkillId> buildSkills() const;
    void equip(int slot); // 0 especial 1, 1 especial 2, 2 passiva, -1 automático
    void unequip(int slot);

    MenuAction confirmTitle();
    MenuAction confirmCharSelect();
    MenuAction confirmMode();
    MenuAction confirmBracket();
    MenuAction confirmPrompt();
    MenuAction modalKey(sf::Keyboard::Key key);
    MenuAction startAction() const;
    void startBracketMatch();
    void moveCharCursor(int dx, int dy);
    void notify(const std::string& message);

    // Desenho (menu_draw.cpp)
    void drawBackground(cv::Canvas& ctx);
    void drawLogo(cv::Canvas& ctx, float cx, float top, float scale);
    void drawBoot(sf::RenderTarget& target, cv::Canvas& ctx);
    void drawTitle(sf::RenderTarget& target, cv::Canvas& ctx);
    void drawCharSelect(sf::RenderTarget& target, cv::Canvas& ctx);
    void drawStageHeader(cv::Canvas& ctx, sf::Color col, float headerEnter, const std::string& headerText, const std::string& bannerPrev,
                         float bannerFlip);
    void drawFighterStage(sf::RenderTarget& target, cv::Canvas& ctx, int focus, sf::Color col, float swap, float since, float enterLeft,
                          float headerEnter, const std::string& headerText, const std::string& bannerPrev, float bannerFlip);
    void drawModeSelect(sf::RenderTarget& target, cv::Canvas& ctx);
    void drawVersus(sf::RenderTarget& target, cv::Canvas& ctx);
    void drawBuild(sf::RenderTarget& target, cv::Canvas& ctx);
    void drawBracket(sf::RenderTarget& target, cv::Canvas& ctx);
    void drawCampaignPrompt(cv::Canvas& ctx);
    void drawSettings(cv::Canvas& ctx);
    void drawExit(cv::Canvas& ctx);
    void drawHeroCard(sf::RenderTarget& target, cv::Canvas& ctx, int charId, float x, float y, float w, float h);
    void drawStatsPanel(cv::Canvas& ctx, uint32_t charId, float x, float y, float w, float h);
    void drawMatchCard(sf::RenderTarget& target, cv::Canvas& ctx, float x, float y, const BracketMatch& m, const char* label,
                       bool current, bool locked, bool bossCard);
    void drawNotice(cv::Canvas& ctx, float y);
    float logoBlend() const; // 0 = logo grande no centro (título), 1 = logo pequena no topo

    HudFonts fonts_;
    const SpriteLibrary* sprites_;
    Screen screen_ = Screen::Boot;
    Screen prevScreen_ = Screen::Boot;
    uint32_t frame_ = 0;
    uint32_t screenFrame_ = 0;

    int titleIndex_ = 0;       // 0 iniciar, 1 configurações, 2 sair
    int slot_ = 0;
    int prevSlot_ = 0, lastSlot_ = 0;   // troca de lutador na seleção (anima a entrada do novo)
    uint32_t slotChangeFrame_ = 0;
    int rouletteSteps_ = 0, rouletteTarget_ = 0, rouletteTimer_ = 0, rouletteDelay_ = 2; // roleta do aleatório
    int charButtonIndex_ = -1; // -1 = foco na grade; 0 menu principal; 1 confirmar; 2 montar build
    int modeIndex_ = 1;        // começa em LUTA SOLO (índice 1 de kModes)
    const char* versusTag_ = "LUTA SOLO"; // etiqueta no meio da tela de VS
    bool versusReady_ = false;
    uint32_t playerChar_ = 0;
    uint32_t enemyChar_ = 1;

    std::array<SkillLoadout, 10> loadouts_{};
    const sf::Texture* stageBg_ = nullptr; const sf::Texture* livePreview_ = nullptr;
    const sf::Texture* stageFighter_ = nullptr;
    std::array<uint32_t, 10> skins_{};
    int buildIndex_ = 0;
    std::array<uint32_t, 3> equipFlash_{{0u, 0u, 0u}};
    // A build reproduz somente o clipe pré-renderizado da habilidade/skin/câmera.
    PreviewVideo previewVideo_;
    SkillId previewSkill_ = SkillId::None;
    uint32_t previewSkin_ = ~0u;
    bool previewVideoFront_ = false;
    bool previewFront_ = false;
    sf::Clock previewClock_;
    sf::Clock glitchClock_;
    float glitchLen_ = 0.0f;
    bool slotPrompt_ = false;   // "em qual encaixe?" quando os dois especiais estão ocupados
    int slotPromptIndex_ = 0;
    void restartPreview(SkillId id, float glitchFrames);
    void updatePreview();
    void drawPreview(sf::RenderTarget& target, cv::Canvas& ctx, float reveal);
    void drawSlotPrompt(cv::Canvas& ctx);
    sf::FloatRect previewArea() const { return {{0.0f, 96.0f}, {700.0f, 648.0f}}; }
    sf::FloatRect slotPromptButton(int i) const { return {{560.0f + i * 250.0f, 470.0f}, {230.0f, 70.0f}}; } // quadro do último equipar em cada encaixe (pisca)
    Screen buildReturn_ = Screen::ModeSelect;
    bool dirty_ = false;

    std::string notice_;
    int noticeTimer_ = 0;

    Tournament tournament_;
    bool arcade_ = false;     // a próxima luta (tela VS) é do torneio
    int bracketIndex_ = 2;    // 0 voltar, 1 novo torneio, 2 entrar no ringue
    int promptIndex_ = 0;     // 0 continuar, 1 novo, 2 cancelar

    Modal modal_ = Modal::None;
    uint32_t modalFrame_ = 0;
    int modalIndex_ = 0;      // configurações: 0 efeitos, 1 música, 2 ok · saída: 0 cancelar, 1 fechar
    int sfxVolume_ = 80, musicVolume_ = 80;
    int dragSlider_ = -1;

    int settingsTab_ = 0;       // 0 áudio, 1 controles
    int rebindAction_ = -1;     // -1 normal, 0..7 remapeando ação
    int rebindSlot_ = 0;        // 0 primária, 1 secundária
    int controlIndex_ = 0;      // foco na navegação por teclado da aba controles
    KeyBindings keyBindings_ = KeyBindings::makeDefault();
};

} // namespace KoRama

#endif // KORAMA_MENU_SCREENS_HPP
