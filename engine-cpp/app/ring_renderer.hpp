#ifndef KORAMA_RING_RENDERER_HPP
#define KORAMA_RING_RENDERER_HPP

#include "camera3d.hpp"
#include "engine.hpp"
#include "key_bindings.hpp"
#include "sprite_atlas.hpp"
#include "vfx/lab_common.hpp"
#include <array>
#include <memory>
#include <SFML/Graphics.hpp>
#include <functional>
#include <optional>
#include <random>
#include <string>
#include <vector>

namespace KoRama {

// Fontes do HUD: Bangers para nomes e banners, Press Start 2P para o relógio (como no index.html).
struct HudFonts {
    const sf::Font* display = nullptr;
    const sf::Font* digits = nullptr;
};

// Replay do nocaute (sistema de replay do JS): 0 rebobinando a fita, 1 sua visão, 2 visão do oponente.
struct ReplayView {
    int phase = -1;     // -1 = luta normal
    float timer = 0.0f; // relógio da reprodução (timecode do VHS)
};

// Mantém a ordem do pintor, mas agrupa geometria compatível em lotes de desenho.
struct TriangleDraw {
    std::vector<sf::Vertex> vertices;
    const sf::Texture* texture = nullptr;
    void operator()(sf::RenderTarget& target) const {
        sf::RenderStates states;
        states.texture = texture;
        if (!vertices.empty()) target.draw(vertices.data(), vertices.size(), sf::PrimitiveType::Triangles, states);
    }
};

// Desenha a cena da luta: ringue, lutadores com sprites, partículas e HUD.
// Segue o modelo do jogo JS: o ringue fica parado e lutadores + câmera giram pelo ring_angle;
// tudo passa por uma fila ordenada por camada e profundidade.
class RingRenderer {
public:
    RingRenderer(HudFonts fonts, const SpriteLibrary* sprites);

    // showResult = false esconde o placar final enquanto o perdedor voa e o replay não terminou.
    // Tempo acumulado de cada etapa do quadro (ms), para medir o desempenho.
    struct Profile { double build = 0, queue = 0, post = 0, hud = 0; int frames = 0; std::size_t queueItems = 0; };
    Profile profile;
    void draw(sf::RenderTarget& target, const GameEngine& engine, bool paused, const ReplayView& replay = {}, bool showResult = true);
    void resetEffects(); // luta nova: zera partículas e efeitos do Lab
    // jogo 3D: só a HUD da luta (placas, vida, stamina, relógio, especiais, combo, contagem), por cima da cena 3D
    void drawHudOnly(sf::RenderTarget& target, const GameEngine& engine, bool paused) { drawComicHud(target, engine, paused); }
    // só o card de fim de luta (jogo 3D): frames desde que ele começou a entrar
    void drawResultCard(sf::RenderTarget& target, const GameEngine& engine, int frames) { celebFrames_ = frames; drawVictoryCard(target, engine); }
    void startFightFade(int frames = 22) { fightFadeFrames_ = fightFadeMax_ = frames; }
    const KeyBindings& keyBindings() const { return keyBindings_; }
    void setKeyBindings(const KeyBindings& kb) { keyBindings_ = kb; }

    // Uso no menu (prévia de golpes na build): sem HUD e com a câmera opcionalmente do lado do rival.
    bool hideHud = false;
    // Mouse no espaço de interface 1600x900 (destaque dos botões da pausa e do card de resultado).
    sf::Vector2f uiMouse{-1.0f, -1.0f};
    static sf::FloatRect pauseButton() { return {{768.0f, 832.0f}, {64.0f, 50.0f}}; }
    static sf::FloatRect pausePlate(int i, bool training = false) {
        if (training) return {{800.0f - 300.0f + (i % 2 ? 20.0f : -20.0f), 380.0f + i * 68.0f}, {600.0f, 54.0f}};
        return {{800.0f - 280.0f + (i % 2 ? 26.0f : -26.0f), 448.0f + i * 82.0f}, {560.0f, 62.0f}};
    }
    int resultButtonAt(sf::Vector2f p) const {
        for (std::size_t i = 0; i < resultButtons_.size(); ++i) if (resultButtons_[i].contains(p)) return static_cast<int>(i);
        return -1;
    }
    bool reverseCamera = false;
    bool hideEnemy = false;
    // Captura para a apresentação (korama_snapshot jacktrack): caixa do corpo e luvas na tela, por lutador.
    struct FighterCapture {
        bool valid = false;
        float bodyX = 0, bodyY = 0, bodyW = 0, bodyH = 0, skew = 0; // base (meio dos pés), tamanho na tela
        struct Glove { float x = 0, y = 0, size = 0, rot = 0, skew = 0; bool front = true; std::string name; } glove[2];
    };
    FighterCapture capture[2]; // 0 = jogador, 1 = oponente
    static inline bool liveVisual = false; // visual novo do elenco (F9 liga/desliga)
    bool hidePlayer = false; // só para capturas de teste das poses do oponente // a câmera do lado do rival olharia através dele
    bool hideCrowd = false;
    bool disableBackdropShader = false;
    bool hideArena = false;
    bool hideRing = false;
    bool hidePostFx = false;
    std::optional<int> overrideTheme;

    // Câmera livre / noclip para inspeção 3D dos cenários
    struct FreeCam {
        bool enabled = false;
        Vec3 pos{0.0f, 220.0f, -260.0f};
        float yaw = 0.0f;
        float pitch = 0.45f;
    };
    FreeCam freeCam;
    const Camera3D& camera() const { return camera_; }

private:
    struct DrawItem {
        float layer;
        float depth;
        std::function<void(sf::RenderTarget&)> draw;
    };
    struct BatchCollector {
        TriangleDraw td;
        float depthSum = 0.0f;
        int count = 0;
        float layer = 0.0f;

        BatchCollector(float lyr = 0.0f) : layer(lyr) {}

        void addPoly(RingRenderer& rr, std::initializer_list<Vec3> list, sf::Color fill, sf::Color stroke = sf::Color::Transparent) {
            float d = 0.0f;
            if (rr.appendWorldPolygon(td, d, list, fill, stroke)) {
                depthSum += d;
                count++;
            }
        }
        void addPoly(RingRenderer& rr, const Vec3* verts, std::size_t n, sf::Color fill, sf::Color stroke = sf::Color::Transparent) {
            float d = 0.0f;
            if (rr.appendWorldPolygon(td, d, verts, n, fill, stroke)) {
                depthSum += d;
                count++;
            }
        }
        void addPoly(RingRenderer& rr, const std::vector<Vec3>& v, sf::Color fill, sf::Color stroke = sf::Color::Transparent) {
            float d = 0.0f;
            if (rr.appendWorldPolygon(td, d, v, fill, stroke)) {
                depthSum += d;
                count++;
            }
        }
        void addLine(RingRenderer& rr, const Vec3& a, const Vec3& b, sf::Color col, float w) {
            float d = 0.0f;
            if (rr.appendWorldLine(td, d, a, b, col, w)) {
                depthSum += d;
                count++;
            }
        }
        void flush(std::vector<DrawItem>& q) {
            if (count > 0 && !td.vertices.empty()) {
                q.push_back({layer, depthSum / static_cast<float>(count), std::move(td)});
                td = {};
                depthSum = 0.0f;
                count = 0;
            }
        }
    };
    struct ShadowPiece {
        const sf::Texture* texture = nullptr;
        std::vector<sf::Vertex> vertices;
    };
    struct ShadowData {
        std::vector<ShadowPiece> pieces;
        float baseX = 0.0f, baseY = 0.0f, footY = 0.0f, height = 1.0f, lateral = 0.0f;
        std::uint8_t opacity = 0;
    };
    std::array<std::shared_ptr<ShadowData>, 2> pendingShadow_;
    std::array<std::unique_ptr<sf::RenderTexture>, 2> shadowMask_;
    void addShadowGlove(const SpriteFrame& frame, const Vec3& world, float w, float h, float skew, float rotation, int slot);

    struct Placement {
        Vec3 pos;
        float facing;
    };

    void placeCamera(const Fighter& player, float ringAngle);
    void placeReverseCamera(const Fighter& enemy, float ringAngle); // replay pela visão do oponente
    void drawReplayOverlay(sf::RenderTarget& target, const ReplayView& replay);
    void queueRing();
    void updateRopePhysics(const GameEngine& engine);
    // roll: giro do corpo inteiro na tela (voo do nocaute); koFlight usa o rosto de nocaute e estrelas.
    void queueFighter(const Fighter& fighter, const Placement& place, bool backView, float roll = 0.0f, bool koFlight = false);
    // Reação ao golpe / queda / tontura calculada a cada quadro (fighter_pose.cpp).
public:
    struct Reaction {
        float headRot = 0, headShiftX = 0, headShiftY = 0, headScaleX = 1;
        float dx = 0, dy = 0, dz = 0, skew = 0, squash = 1, roll = 0;
        float gloveLift = 0, gloveSpread = 0, gloveShift = 0, clutch = 0; // gloveShift em direção de tela
        bool clutchLeft = false;
        int face = 0;          // 1 olhos apertados, 2 dentes cerrados, 3 zonzo
        bool xxFace = false;   // usa o sprite de nocaute (olhos X)
        bool stars = false, drops = false, dust = false, sweat = false;
        float dir = 1, hitP = 0, time = 0;
        std::string sprite;    // quadro novo de reação (sufixo: RxFrontTurnR, RxBackUp, RxSpin_2...)
        bool mirror = false;   // espelha o quadro (segunda metade do giro)
        float yaw = 0;         // giro em torno do eixo vertical (voo do nocaute)
        float rope = 0;        // agarrado nas cordas (0..1)
    };
private:
    Reaction computeReaction(const Fighter& f, int slot, float roll, bool koFlight, bool backView);
    void queueBody(const SpriteFrame& frame, const Vec3& world, float w, float h, float skew, float layer, sf::Color tint,
                   const Reaction& rx, uint32_t charId, bool backView, bool overlays, bool drawSprite = true);
    // Jack por partes (jack_rig.cpp); false se as partes não foram exportadas.
    bool queueJackRig(const Fighter& f, const Reaction& rx, const Vec3& world, float w, float h, float skew, float layer,
                      sf::Color tint, bool backView);
    // Demais lutadores por partes (fighter_rig.cpp); false = usa o sprite inteiro (variações e skills).
    bool queueFighterRig(const Fighter& f, const Reaction& rx, const std::string& bodyName, const Vec3& world, float w, float h,
                         float skew, float layer, sf::Color tint, bool backView);
    static bool rigBase(uint32_t charId, const std::string& bodyName, bool backView);
    // Visual novo (live_fighter.cpp): corpo animado por dentro na caixa do sprite; luvas e movimento do jogo.
    std::string liveForm(const Fighter& f, const std::string& bodyName) const;
    std::string liveAction(const Fighter& f, bool groggy, float& prog) const;
    bool queueLiveBody(const Fighter& f, const Reaction& rx, const std::string& bodyName, const Vec3& world, float w, float h,
                       float skew, float layer, sf::Color tint, bool backView, bool& rigGloves);
    int downFrames_[2] = {-1, -1};
    uint32_t downIdx_[2] = {0, 0};
    int downDir_[2] = {1, 1};
    void queuePlaceholderFighter(const Fighter& fighter, const Placement& place, bool backView);
    void queueScarecrow(const Fighter& f, const Placement& place, bool backView);
    void queueParticles(const GameRenderState& state);
    void queueSkillEffects(const GameEngine& engine);
    void queueFighterStatus(const Fighter& f, uint32_t frame);
    // Efeitos do Combat Lab nas camadas do drawEntityCharacter (chão, atrás, sobre e à frente do corpo).
    void queueLabLayers(const Fighter& f, bool backView, float bodyLayer, const Vec3& bodyWorld, float bodyW, float bodyH);
    vfx::LabContext labContext(const Fighter& f, bool backView) const;
    void drawSkillDeck(sf::RenderTarget& target, const Fighter& f, bool alignRight);
    void drawStatusChips(sf::RenderTarget& target, const Fighter& f, sf::Vector2f pos, bool alignRight);
    // rotation em radianos, em volta do centro do sprite (serras do Batalhator).
    void queueSprite(const SpriteFrame& frame, const Vec3& world, float w, float h, float skew, float layer,
                     sf::Color tint = sf::Color::White, float rotation = 0.0f, float bottomCrop = 0.0f);
    void queueBossEffects(const GameEngine& engine);
    // Arquibancada em 4 degraus, torcida animada, flashes de câmera e rosas jogadas no campeão.
    void queueStands();
    void queueCentral(const GameEngine& engine);
    void queueHarbor(const GameEngine& engine);
    void queuePernambuco();
    void queueObservatory();
    void queueSwamp();
    void queueConcertHall();
    void queueParlor();
    void queueEspinhaCourtyard();
    void queueJackGarden();
    void queueCrowd(const GameEngine& engine);
    void stepCrowdEffects(const GameEngine& engine);
    bool appendWorldPolygon(TriangleDraw& td, float& outDepth, const Vec3* world, std::size_t nIn, sf::Color fill, sf::Color stroke);
    bool appendWorldPolygon(TriangleDraw& td, float& outDepth, std::initializer_list<Vec3> list, sf::Color fill, sf::Color stroke) {
        return appendWorldPolygon(td, outDepth, list.begin(), list.size(), fill, stroke);
    }
    bool appendWorldPolygon(TriangleDraw& td, float& outDepth, const std::vector<Vec3>& vec, sf::Color fill, sf::Color stroke) {
        return appendWorldPolygon(td, outDepth, vec.data(), vec.size(), fill, stroke);
    }

    bool appendWorldLine(TriangleDraw& td, float& outDepth, const Vec3& a, const Vec3& b, sf::Color color, float width);

    void queueWorldPolygon(const Vec3* world, std::size_t nIn, sf::Color fill, sf::Color stroke, float layer);
    void queueWorldPolygon(std::initializer_list<Vec3> list, sf::Color fill, sf::Color stroke, float layer) {
        queueWorldPolygon(list.begin(), list.size(), fill, stroke, layer);
    }
    void queueWorldPolygon(const std::vector<Vec3>& vec, sf::Color fill, sf::Color stroke, float layer) {
        queueWorldPolygon(vec.data(), vec.size(), fill, stroke, layer);
    }
    void queueWorldLine(const Vec3& a, const Vec3& b, sf::Color color, float width, float layer);

    void drawFloatingTexts(sf::RenderTarget& target, const GameRenderState& state);
    void drawHud(sf::RenderTarget& target, const GameEngine& engine, bool paused, bool showResult);
    void drawComicHud(sf::RenderTarget& target, const GameEngine& engine, bool paused); // fight_hud.cpp
    void drawHudVignette(sf::RenderTarget& target); // static gradient shared with the cached HUD overlay
    float hudTrail_[2] = {1.0f, 1.0f}; // rastro laranja do dano na barra de vida
    void queueImpactWord(const GameEngine& engine); // fight_hud.cpp
    void drawKnockdownDots(sf::RenderTarget& target, sf::Vector2f pos, int count, bool alignRight);
    void drawBar(sf::RenderTarget& target, sf::Vector2f pos, sf::Vector2f size, float ratio, sf::Color fill, bool alignRight);
    void drawText(sf::RenderTarget& target, const std::string& utf8, sf::Vector2f pos, unsigned size, sf::Color color,
                  float anchorX = 0.0f, const sf::Font* font = nullptr);

    std::optional<SpriteFrame> sprite(const std::string& name) const;

    HudFonts fonts_;
    const SpriteLibrary* sprites_;
    Camera3D camera_;
    std::vector<DrawItem> queue_;
    std::mt19937 rng_{1234u};
    uint32_t frameCounter_ = 0; // quadro do motor, para as animações em ciclo
    enum class ArenaTheme { Central, JoanaHarbor, ValeriaPernambuco, MarcovObservatory, FrogSwamp, VivaldiConcertHall, CremosoParlor, EspinhaCourtyard, JackGarden };
    ArenaTheme arenaTheme_ = ArenaTheme::Central;

    struct Spectator {
        Vec3 pos;
        float angle;
        int skin;
        float phase;
    };
    struct Flash {
        Vec3 pos;
        float life, decay, size;
    };
    struct Throwable {
        Vec3 pos, vel;
        float rot, spin;
        int life;
        bool landed;
        int kind; // 0..2 rosas, 3..14 calcinhas e cuecas (celebration.hpp)
    };
    std::vector<Spectator> crowd_;
    std::vector<Flash> flashes_;
    std::vector<Throwable> throwables_;
    uint32_t effectsFrame_ = 0;
    // Celebração da vitória: o campeão dança no centro do ringue com a câmera girando em volta.
    // Voo do perdedor no nocaute: gira até as cordas, quica nelas e sai voando (vira estrela) ou cai da tela.
    int koFrames_ = -1, koRopeFrame_ = -1;
    bool koSparks_ = false;
    uint32_t koFrameIdx_ = 0;
    sf::Vector2f koStar_{-1.0f, -1.0f};
    std::unique_ptr<sf::RenderTexture> postFx_; // brilho do holofote + scanlines + vinheta, desenhados uma vez
    std::unique_ptr<sf::RenderTexture> postFxHud_; // same overlay with the static HUD vignette precomposed
    bool combineHudVignette_ = true; // diagnostic fallback keeps the original separate pass
    std::unique_ptr<sf::RenderTexture> sceneRt_; // alvo intermediário para pós-processamento de GPU
    std::unique_ptr<sf::Shader> postFxShader_;  // Halftone, Shockwave Ripple e Chromatic Aberration
    bool shaderInitialized_ = false;
    sf::Vector2f shockCenter_{0.5f, 0.5f};
    float shockProgress_ = 1.0f;
    float shockStrength_ = 0.0f;
    int lastImpactSeed_ = -1;
    bool celebrating_ = false;
    int celebFrames_ = 0;
    float celebAngle_ = 0.0f;
    uint32_t celebFrameIdx_ = 0;
    void drawVictoryCard(sf::RenderTarget& target, const GameEngine& engine);
    std::vector<sf::FloatRect> resultButtons_;
    uint32_t overlayTick_ = 0; // animação da moldura do replay (o motor congelado não anda)
    int fightFadeFrames_ = 0;
    int fightFadeMax_ = 22;

    const GameEngine* engine_ = nullptr;
    std::array<std::unique_ptr<vfx::LabFx>, 2> lab_; // [0] jogador, [1] oponente
    std::array<uint32_t, 2> labChar_{{99u, 99u}};
    bool labStep_ = false;  // o quadro do motor avançou: os efeitos andam neste desenho
    uint32_t labFrame_ = 0;

    std::array<Vec3, 2> mudPuddleFloorPos_{};
    std::array<bool, 2> mudPuddleActive_{false, false};
    Vec3 joanaCoralCrackPos_{};
    bool joanaCoralCrackActive_ = false;

    struct RopeSideState {
        std::array<float, 3> deflect{0.0f, 0.0f, 0.0f};  // deformação elástica (unidades de mundo)
        std::array<float, 3> velocity{0.0f, 0.0f, 0.0f}; // velocidade da mola
        float contactT = 0.5f;                           // posição normalizada do contato (0..1)
        float contactWidth = 0.12f;                      // largura do apoio (fração do lado)
    };
    std::array<RopeSideState, 4> ropes_{};
    uint32_t lastRopeFrame_ = 0;
    bool reverseView_ = false;
    float ropeZoom_ = 0.0f;
    bool cameraInitialized_ = false;
    KeyBindings keyBindings_ = KeyBindings::makeDefault();
};

} // namespace KoRama

#endif // KORAMA_RING_RENDERER_HPP
