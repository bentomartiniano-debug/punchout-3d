#include "game_app.hpp"
#include "ui_fonts.hpp"
#include "ui_draw.hpp"
#include "config.hpp"
#include "character_info.hpp"
#include <algorithm>
#include <array>
#include <fstream>
#include <sstream>
#include <iostream>
#include <chrono>
#include <thread>

namespace KoRama {

namespace {

constexpr std::size_t kReplayFrames = 220;  // ~2 s de luta + o voo do perdedor
constexpr int kReplayDelay = 144;           // 2,4 s de voo do perdedor antes do replay
constexpr int kRewindFrames = 60;           // 1,0 s de rebobinagem rápida estilo fita VHS
constexpr float kReplaySpeed = 0.32f;       // câmera lenta

// Spritesheets já exportadas (tools/export_sprites); lutadores sem sheet usam formas provisórias.
constexpr std::array<const char*, 11> kSpriteSheets = {"jack", "frog", "joana", "cremoso", "valeria", "vivaldi", "espinha", "marcov", "boss", "crowd", "portraits"};

// Fonte do projeto em assets/fonts; sem ela, cai para uma fonte do sistema.
std::optional<sf::Font> loadFont(const std::filesystem::path& projectFont) {
    const std::array<std::filesystem::path, 6> candidates = {
        projectFont,
        "C:/Windows/Fonts/impact.ttf",
        "C:/Windows/Fonts/arialbd.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/TTF/DejaVuSans-Bold.ttf",
        "/System/Library/Fonts/Supplemental/Arial Bold.ttf",
    };
    for (const auto& path : candidates) {
        if (!std::filesystem::exists(path)) continue;
        sf::Font font;
        if (font.openFromFile(path)) return font;
    }
    std::cerr << "[K.O.-RAMA] Fonte nao encontrada: " << projectFont.filename().string() << "; texto desligado.\n";
    return std::nullopt;
}

// Janela 16:9: 1600×900 quando cabe na tela, senão 1280×720.
sf::VideoMode windowMode() {
    const sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    if (desktop.size.x >= 1700 && desktop.size.y >= 1000) return sf::VideoMode({1600u, 900u});
    return sf::VideoMode({1280u, 720u});
}

// Stencil para os preenchimentos côncavos do canvas e antialiasing nas bordas, como no navegador.
sf::ContextSettings contextSettings() {
    sf::ContextSettings s;
    s.stencilBits = 8;
    s.antiAliasingLevel = 0;
    return s;
}

bool isAny(sf::Keyboard::Key key, std::initializer_list<sf::Keyboard::Key> options) {
    return std::find(options.begin(), options.end(), key) != options.end();
}

bool held(std::initializer_list<sf::Keyboard::Key> keys) {
    return std::any_of(keys.begin(), keys.end(), [](sf::Keyboard::Key k) { return sf::Keyboard::isKeyPressed(k); });
}

} // namespace
using Key = sf::Keyboard::Key;

std::filesystem::path GameApp::findAssetRoot(const char* argv0) {
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::path exeDir = argv0 ? fs::absolute(fs::path(argv0), ec).parent_path() : fs::current_path();
    const std::array<fs::path, 5> candidates = {
        exeDir / "assets",
        exeDir.parent_path() / "assets",
        exeDir.parent_path().parent_path() / "assets",
        fs::current_path() / "assets",
        fs::current_path() / "engine-cpp" / "assets",
    };
    for (const auto& dir : candidates) {
        if (fs::is_directory(dir / "sprites", ec)) return dir;
    }
    std::cerr << "[K.O.-RAMA] Pasta assets/ nao encontrada; usando formas provisorias.\n";
    return exeDir / "assets";
}

GameApp::GameApp(const std::filesystem::path& assetRoot)
    : assetRoot_(assetRoot),
      window_(windowMode(), "K.O.-RAMA", sf::Style::Default, sf::State::Windowed, contextSettings()),
      view_(sf::FloatRect({0.0f, 0.0f}, {kLogicalWidth, kLogicalHeight})),
      uiView_(sf::FloatRect({0.0f, 0.0f}, {kUiWidth, kUiHeight})),
      displayFont_(loadFont(assetRoot / "fonts" / "Bangers-Regular.ttf")),
      digitFont_(loadFont(assetRoot / "fonts" / "PressStart2P-Regular.ttf")),
      renderer_(HudFonts{displayFont_ ? &*displayFont_ : nullptr, digitFont_ ? &*digitFont_ : nullptr}, &sprites_),
      menus_(HudFonts{displayFont_ ? &*displayFont_ : nullptr, digitFont_ ? &*digitFont_ : nullptr}, &sprites_) {
    sprites_.loadMenuSheets(assetRoot_ / "sprites");
    sprites_.keepRigs({});
    sprites_.setRigDir(assetRoot_ / "sprites");
    sprites_.waitForRigs = true;
    ui::loadFonts(assetRoot_ / "fonts");
    window_.setVerticalSyncEnabled(false);
    window_.setFramerateLimit(60);
    window_.setKeyRepeatEnabled(false);
    updateLetterbox(window_.getSize());
    loadBuilds();
    warmUpCaches();
    audio_.init(assetRoot_);
    prepareFighters(0, 1);
    menus_.goTo(Screen::Boot);
}

#ifdef _WIN32
extern "C" __declspec(dllimport) const unsigned char* __stdcall glGetString(unsigned int name);
#else
extern "C" const unsigned char* glGetString(unsigned int name);
#endif

int GameApp::run() {
    std::string gpuName = "GPU";
    if (const auto* s = reinterpret_cast<const char*>(glGetString(0x1F01))) {
        gpuName = s;
        if (gpuName.find("GeForce") != std::string::npos) gpuName = "NVIDIA GeForce MX110";
        else if (gpuName.find("Intel") != std::string::npos) gpuName = "Intel UHD";
    }

    sf::Clock fpsClock;
    int frameCount = 0;

    while (window_.isOpen()) {
        processEvents();
        tick();
        render();

        ++frameCount;
        if (fpsClock.getElapsedTime().asSeconds() >= 0.5f) {
            const float dt = fpsClock.restart().asSeconds();
            const int fps = static_cast<int>(std::round(frameCount / dt));
            frameCount = 0;
            window_.setTitle("K.O.-RAMA [" + std::to_string(fps) + " FPS | " + gpuName + " | v2.2]");
        }
    }
    return 0;
}

int GameApp::benchmarkVersus(const std::filesystem::path& outCsv, uint32_t player, uint32_t playerSkin,
                             uint32_t enemy, uint32_t enemySkin, int uploadPolls) {
    if (player >= 10 || enemy >= 10 || playerSkin >= 4 || enemySkin >= 4 || uploadPolls < 1 || uploadPolls > 3)
        return 2;
    std::ofstream csv(outCsv);
    if (!csv) return 3;
    csv << "frame,screen_frame,ready,poll_ms,tick_ms,render_ms,total_ms,screen\n";
    csv.setf(std::ios::fixed); csv.precision(4);
    audio_.setMusicVolume(0.0f);
    audio_.setSfxVolume(0.0f);
    menus_.setSkin(player, playerSkin);
    menus_.setSkin(enemy, enemySkin);
    menus_.takeDirty(); // benchmark must never persist selected skins or builds
    versusUploadPolls_ = uploadPolls;
    // Measure a real menu-to-versus transition, not the first swap of a newly
    // opened window. The latter includes one-time driver/context setup.
    menus_.goTo(Screen::ModeSelect);
    for (int i = 0; i < 15 && window_.isOpen(); ++i) {
        processEvents();
        tick();
        render();
    }
    startMatch(player, enemy, menus_.loadouts()[player]);
    if (menus_.screen() != Screen::Versus) return 4;
    const auto wallStart = std::chrono::steady_clock::now();
    auto measureMs = [](auto a, auto b) { return std::chrono::duration<double, std::milli>(b - a).count(); };
    int frames = 0, spikes = 0, worstFrame = 0, fightFrames = 0;
    double worstMs = 0, pollTotal = 0;
    for (; frames < 900 && window_.isOpen(); ++frames) {
        processEvents();
        const auto t0 = std::chrono::steady_clock::now();
        tick();
        const auto t1 = std::chrono::steady_clock::now();
        render();
        const auto t2 = std::chrono::steady_clock::now();
        const double total = measureMs(t0, t2);
        const bool ready = fightersReady();
        const double pollMs = menus_.screen() == Screen::Versus ? lastVersusUploadMs_ : 0.0;
        csv << frames << ',' << menus_.screenFrame() << ',' << ready << ',' << pollMs << ','
            << measureMs(t0, t1) << ',' << measureMs(t1, t2) << ',' << total << ','
            << (menus_.screen() == Screen::Fight ? "fight" : "versus") << '\n';
        pollTotal += pollMs;
        if (total > 33.333) ++spikes;
        if (total > worstMs) { worstMs = total; worstFrame = frames; }
        if (menus_.screen() == Screen::Fight && ++fightFrames >= 15) { ++frames; break; }
        if (menus_.screen() != Screen::Versus && menus_.screen() != Screen::Fight) { ++frames; break; }
    }
    csv.flush();
    std::cout << "VERSUS player=" << player << "/" << playerSkin << " enemy=" << enemy << "/" << enemySkin
              << " polls=" << uploadPolls << " frames=" << frames << " ready=" << fightersReady()
              << " wall_ms=" << measureMs(wallStart, std::chrono::steady_clock::now())
              << " upload_ms=" << pollTotal << " over33=" << spikes << " worst_ms=" << worstMs
              << " worst_frame=" << worstFrame << " csv=" << outCsv << std::endl;
    window_.close();
    return fightersReady() ? 0 : 5;
}

void GameApp::processEvents() {
    while (const std::optional event = window_.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window_.close();
        } else if (const auto* resized = event->getIf<sf::Event::Resized>()) {
            updateLetterbox(resized->size);
        } else if (const auto* wheel = event->getIf<sf::Event::MouseWheelScrolled>()) {
            if (renderer_.freeCam.enabled) {
                freeCamSpeed_ += wheel->delta * 60.0f;
                freeCamSpeed_ = std::clamp(freeCamSpeed_, 60.0f, 3500.0f);
            }
        } else if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
            onKeyPressed(key->code);
        } else if (const auto* moved = event->getIf<sf::Event::MouseMoved>()) {
            if (renderer_.freeCam.enabled) {
                if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Right) || sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
                    int dx = moved->position.x - freeCamLastMousePos_.x;
                    int dy = moved->position.y - freeCamLastMousePos_.y;
                    renderer_.freeCam.yaw += static_cast<float>(dx) * 0.0035f;
                    renderer_.freeCam.pitch += static_cast<float>(dy) * 0.0035f;
                    renderer_.freeCam.pitch = std::clamp(renderer_.freeCam.pitch, -1.45f, 1.45f);
                    while (renderer_.freeCam.yaw > 3.14159f) renderer_.freeCam.yaw -= 2.0f * 3.14159f;
                    while (renderer_.freeCam.yaw < -3.14159f) renderer_.freeCam.yaw += 2.0f * 3.14159f;
                }
                freeCamLastMousePos_ = moved->position;
            }
            const sf::Vector2f p = window_.mapPixelToCoords(moved->position, uiView_);
            if (menus_.screen() != Screen::Fight) {
                menus_.onMouseMove(p);
                if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) { menus_.onMouseDrag(p); handleMenuAction({}); }
            } else {
                renderer_.uiMouse = p;
            }
        } else if (const auto* click = event->getIf<sf::Event::MouseButtonPressed>()) {
            if (renderer_.freeCam.enabled) {
                freeCamLastMousePos_ = click->position;
            }
            const sf::Vector2f p = window_.mapPixelToCoords(click->position, uiView_);
            if (renderer_.freeCam.enabled) {
                // Durante o modo noclip, cliques de mouse não ativam botões de pausa ou combate
                continue;
            }
            if (menus_.screen() != Screen::Fight) {
                const Screen before = menus_.screen();
                if (click->button == sf::Mouse::Button::Left) handleMenuAction(menus_.onClick(p));
                else if (click->button == sf::Mouse::Button::Right) handleMenuAction(menus_.onRightClick());
                if (menus_.screen() != before) audio_.play(Sfx::Select);
            } else if (click->button == sf::Mouse::Button::Left) {
                onFightClick(p);
            }
        } else if (const auto* joyBtn = event->getIf<sf::Event::JoystickButtonPressed>()) {
            if (joyBtn->button == 7 || joyBtn->button == 6) { // Start / Select / Options
                onKeyPressed(Key::Escape);
            } else if (menus_.screen() != Screen::Fight) {
                if (joyBtn->button == 0 || joyBtn->button == 2) onKeyPressed(Key::Enter); // A / X confirma
                else if (joyBtn->button == 1) onKeyPressed(Key::Escape); // B cancela/volta
            }
        } else if (event->is<sf::Event::FocusLost>()) {
            if (menus_.screen() == Screen::Fight && !engine_.isMatchOver()) setPaused(true);
        }
    }
}

void GameApp::onFightClick(sf::Vector2f p) {
    // Mouse na luta: botão de pausa, opções da pausa e botões do card de resultado viram as teclas.
    renderer_.uiMouse = p;
    if (engine_.isMatchOver()) {
        if (replayActive_) { onKeyPressed(Key::Enter); return; }
        if (!resultVisible()) return;
        const int b = renderer_.resultButtonAt(p);
        if (b == 0) onKeyPressed(Key::Enter);
        else if (b == 1) onKeyPressed(Key::Escape);
        return;
    }
    if (paused_) {
        if (engine_.trainingMode) {
            static const Key kTrainKeys[] = {Key::Escape, Key::Tab, Key::B, Key::R, Key::Q, Key::M};
            for (int i = 0; i < 6; ++i) {
                if (RingRenderer::pausePlate(i, true).contains(p)) {
                    audio_.play(Sfx::Select);
                    onKeyPressed(kTrainKeys[i]);
                    return;
                }
            }
            return;
        }
        static const Key kKeys[] = {Key::Escape, Key::R, Key::Q, Key::M};
        for (int i = 0; i < 4; ++i) if (RingRenderer::pausePlate(i, false).contains(p)) { audio_.play(Sfx::Select); onKeyPressed(kKeys[i]); return; }
        return;
    }
    if (RingRenderer::pauseButton().contains(p)) { audio_.play(Sfx::Select); onKeyPressed(Key::Escape); }
}

void GameApp::warmUpCaches() {
    // Desenha menus e um quadro de luta fora da tela no carregamento para criar textos, glifos e
    // texturas antes do primeiro uso (sem isso, a primeira vez de cada tela dá um tranco).
    sf::ContextSettings gl;
    gl.stencilBits = 8;
    sf::RenderTexture scratch;
    if (!scratch.resize({1600u, 900u}, gl)) return;
    scratch.setView(view_);
    {
        MenuScreens warm(HudFonts{displayFont_ ? &*displayFont_ : nullptr, digitFont_ ? &*digitFont_ : nullptr}, &sprites_);
        warm.warmUp(scratch);
    }
    GameEngine dummy;
    renderer_.hidePlayer = renderer_.hideEnemy = true;
    for (uint32_t opp = 0; opp < 9; ++opp) {
        dummy.startMatch(0, opp, false, 1);
        dummy.update(0, static_cast<float>(kTickSeconds));
        renderer_.draw(scratch, dummy, false);
    }
    renderer_.hidePlayer = renderer_.hideEnemy = false;
    renderer_.resetEffects();

    // Aquece todos os retratos HD fora da tela (em scratch) para garantir que os drivers
    // de GPU compilem shaders e façam upload de texturas sem piscar nada na janela principal.
    for (uint32_t c = 0; c < 10; ++c) {
        for (uint32_t s = 0; s < 4; ++s) {
            ui::portrait(scratch, sprites_, rosterInfo(c).key, sf::FloatRect({0.0f, 0.0f}, {660.0f, 600.0f}), 0, false, sf::Color(0, 0, 0, 190), s);
            ui::portrait(scratch, sprites_, rosterInfo(c).key, sf::FloatRect({0.0f, 0.0f}, {660.0f, 600.0f}), 0, false, sf::Color::White, s);
        }
    }
    scratch.display();

    audio_.musicBuffer(MusicTrack::Fight);
    audio_.musicBuffer(MusicTrack::Boss);
}

void GameApp::handleMenuAction(const MenuAction& action) {
    // Volumes e controles da tela de configurações.
    audio_.setSfxVolume(static_cast<float>(menus_.sfxVolume()));
    audio_.setMusicVolume(static_cast<float>(menus_.musicVolume()));
    keyBindings_ = menus_.keyBindings();
    renderer_.setKeyBindings(keyBindings_);
    if (action.type == MenuAction::Type::Quit) window_.close();
    else if (action.type == MenuAction::Type::StartMatch) {
        const MenuAction match = pendingMatch_.value_or(action);
        if (pendingTrainingSwap_) {
            matchPlayer_ = match.playerChar;
            matchLoadout_ = match.loadout;
            engine_.player.initCharacter(matchPlayer_, true);
            engine_.player.skinId = match.playerSkin;
            engine_.player.loadout = matchLoadout_;
            engine_.combat.triggerStatusBanner("LUTADOR: " + std::string(kRoster[matchPlayer_].name));
            engine_.update(0, 0.f);
            pendingTrainingSwap_ = loadingFighters_ = false;
            pendingMatch_.reset();
            menus_.goTo(Screen::Fight);
            audio_.setMusic(currentMatchTrack());
        } else startMatch(match.playerChar, match.enemyChar, match.loadout, match.arcade, match.stage, match.training);
    }
    if (menus_.takeDirty()) saveBuilds();
}

void GameApp::onKeyPressed(Key key) {
    if (menus_.screen() == Screen::Versus && key == Key::Escape) {
        const bool arcade = pendingMatch_.value_or(menus_.versusMatch()).arcade;
        leaveFight();
        menus_.goTo(arcade ? Screen::Bracket : Screen::ModeSelect);
        return;
    }
    if (menus_.modal() == MenuScreens::Modal::Settings && menus_.rebindAction() >= 0) {
        handleMenuAction(menus_.onKey(key));
        audio_.play(Sfx::Select);
        return;
    }
    if (key == Key::M) { // liga/desliga o som em qualquer tela
        audio_.toggleMute();
        return;
    }
    if (key == Key::F7) { // Alterna modo Noclip / Câmera Livre na luta
        renderer_.freeCam.enabled = !renderer_.freeCam.enabled;
        if (renderer_.freeCam.enabled) {
            renderer_.freeCam.pos = {0.0f, 220.0f, -260.0f};
            renderer_.freeCam.pitch = 0.45f;
            renderer_.freeCam.yaw = 0.0f;
            audio_.play(Sfx::Select);
        } else {
            audio_.play(Sfx::Select);
        }
        return;
    }
    if (renderer_.freeCam.enabled && menus_.screen() == Screen::Fight) {
        if (key == Key::F) {
            renderer_.hidePlayer = !renderer_.hidePlayer;
            renderer_.hideEnemy = renderer_.hidePlayer;
            return;
        }
        if (key == Key::G) {
            renderer_.hideCrowd = !renderer_.hideCrowd;
            return;
        }
        if (key == Key::R) {
            renderer_.freeCam.pos = {0.0f, 220.0f, -260.0f};
            renderer_.freeCam.pitch = 0.45f;
            renderer_.freeCam.yaw = 0.0f;
            return;
        }
        if (key == Key::T) {
            renderer_.freeCam.pos = {0.0f, 1500.0f, 0.0f};
            renderer_.freeCam.pitch = 1.55f;
            renderer_.freeCam.yaw = 0.0f;
            return;
        }
        if (key == Key::Y) {
            renderer_.freeCam.pos = {-580.0f, 420.0f, -580.0f};
            renderer_.freeCam.pitch = 0.48f;
            renderer_.freeCam.yaw = 0.785f;
            return;
        }
        if (key == Key::U) {
            renderer_.freeCam.pos = {0.0f, 40.0f, -160.0f};
            renderer_.freeCam.pitch = 0.05f;
            renderer_.freeCam.yaw = 0.0f;
            return;
        }
        if (key == Key::I) {
            renderer_.freeCam.pos = {0.0f, 520.0f, -1450.0f};
            renderer_.freeCam.pitch = 0.26f;
            renderer_.freeCam.yaw = 0.0f;
            return;
        }
    }
    if (key == Key::F8) { // Alterna entre 1280x720 e 1600x900 para notebooks
        const auto sz = window_.getSize();
        if (sz.x >= 1500) window_.setSize({1280u, 720u});
        else window_.setSize({1600u, 900u});
        updateLetterbox(window_.getSize());
        return;
    }
    if (key == Key::F9) { // visual novo do elenco (onde já existe) liga/desliga, em qualquer tela
        RingRenderer::liveVisual = !RingRenderer::liveVisual;
        saveBuilds();
        return;
    }
    if (menus_.screen() != Screen::Fight) {
        const Screen before = menus_.screen();
        handleMenuAction(menus_.onKey(key));
        if (isAny(key, {Key::Enter, Key::Space, Key::Z, Key::J})) {
            audio_.play(Sfx::Select);
        }
        else if (isAny(key, {Key::Up, Key::Down, Key::Left, Key::Right, Key::W, Key::A, Key::S, Key::D}) || menus_.screen() != before) {
            audio_.play(Sfx::MenuNav);
        }
        return;
    }

    const auto state = engine_.gameState;
    const bool matchOver = engine_.isMatchOver() && (state == STATE_VICTORY || state == STATE_GAME_OVER);

    if (matchOver) {
        if (replayActive_) { // pula o replay
            if (isAny(key, {Key::Space, Key::Enter, Key::Escape})) {
                replayActive_ = false;
                replayDone_ = true;
                lastReplayFrame_ = -1;
                audio_.setMusic(MusicTrack::None);
            }
            return;
        }
        if (!resultVisible()) return; // o perdedor ainda está voando
        if (key == Key::Escape) {
            leaveFight(); // volta para a seleção de modo
        } else if (key == Key::Enter) {
            if (!matchArcade_) startMatch(matchPlayer_, matchEnemy_, matchLoadout_); // revanche
            else if (menus_.tournament().active() || menus_.tournament().champion()) menus_.showBracket(); // próxima chave
            else menus_.newCampaign(); // derrota encerra o torneio: começa outro
            if (menus_.takeDirty()) saveBuilds();
        }
        return;
    }
    if (engine_.trainingMode) {
        if (key == Key::Tab) {
            cycleTrainingFighter(!held({Key::LShift, Key::RShift}));
            return;
        }
        if (key == Key::B) {
            setPaused(false);
            menus_.openBuild(Screen::Fight);
            audio_.play(Sfx::Select);
            return;
        }
    }
    if (key == Key::Escape || key == Key::P) {
        setPaused(!paused_);
        return;
    }
    if (paused_) {
        if (engine_.trainingMode) {
            if (key == Key::Tab) {
                cycleTrainingFighter(!held({Key::LShift, Key::RShift}));
                return;
            }
            if (key == Key::B) {
                setPaused(false);
                menus_.openBuild(Screen::Fight);
                audio_.play(Sfx::Select);
                return;
            }
        }
        if (key == Key::R) startMatch(matchPlayer_, matchEnemy_, matchLoadout_, matchArcade_, matchStage_, matchTraining_);
        else if (key == Key::Q) leaveFight();
        return;
    }

    // Toques mais rápidos que um quadro ainda contam: a tecla fica "segurada" por pelo menos um tick.
    if (keyBindings_.matches(ActionKey::PunchL, key)) tapLatch_ |= INPUT_PUNCH_L;
    if (keyBindings_.matches(ActionKey::PunchR, key)) tapLatch_ |= INPUT_PUNCH_R;
    if (keyBindings_.matches(ActionKey::Down, key))   tapLatch_ |= INPUT_DOWN;
    if (keyBindings_.matches(ActionKey::Up, key))     tapLatch_ |= INPUT_UP;
    if (keyBindings_.matches(ActionKey::Left, key))   tapLatch_ |= INPUT_LEFT;
    if (keyBindings_.matches(ActionKey::Right, key))  tapLatch_ |= INPUT_RIGHT;
    if (keyBindings_.matches(ActionKey::Block, key))  tapLatch_ |= INPUT_BLOCK;
    if (keyBindings_.matches(ActionKey::Dodge, key))  tapLatch_ |= INPUT_DODGE_KEY;
}

uint32_t GameApp::buildInputMask() const {
    if (renderer_.freeCam.enabled) return 0;
    // O motor recebe as teclas seguradas e detecta sozinho aperto, soltura, duplo toque e buffer.
    uint32_t mask = tapLatch_;
    if (!window_.hasFocus()) return mask;
    if (keyBindings_.isHeld(ActionKey::Up))     mask |= INPUT_UP;
    if (keyBindings_.isHeld(ActionKey::Down))   mask |= INPUT_DOWN;
    if (keyBindings_.isHeld(ActionKey::Left))   mask |= INPUT_LEFT;
    if (keyBindings_.isHeld(ActionKey::Right))  mask |= INPUT_RIGHT;
    if (keyBindings_.isHeld(ActionKey::Block))  mask |= INPUT_BLOCK;
    if (keyBindings_.isHeld(ActionKey::PunchL)) mask |= INPUT_PUNCH_L;
    if (keyBindings_.isHeld(ActionKey::PunchR)) mask |= INPUT_PUNCH_R;
    if (keyBindings_.isHeld(ActionKey::Dodge))  mask |= INPUT_DODGE_KEY;

    // Suporte a controle/gamepad (Xbox, PlayStation, arcade stick USB)
    if (sf::Joystick::isConnected(0)) {
        const float ax = sf::Joystick::getAxisPosition(0, sf::Joystick::Axis::X);
        const float ay = sf::Joystick::getAxisPosition(0, sf::Joystick::Axis::Y);
        const float px = sf::Joystick::getAxisPosition(0, sf::Joystick::Axis::PovX);
        const float py = sf::Joystick::getAxisPosition(0, sf::Joystick::Axis::PovY);

        if (ax < -45.0f || px < -45.0f) mask |= INPUT_LEFT;
        if (ax > 45.0f || px > 45.0f) mask |= INPUT_RIGHT;
        if (ay < -45.0f || py > 45.0f) mask |= INPUT_UP;
        if (ay > 45.0f || py < -45.0f) mask |= INPUT_DOWN;

        // Botões de soco: A/Cross (0) ou X/Square (2) = Soco Esquerdo
        if (sf::Joystick::isButtonPressed(0, 0) || sf::Joystick::isButtonPressed(0, 2)) mask |= INPUT_PUNCH_L;
        // Y/Triangle (3) = Soco Direito
        if (sf::Joystick::isButtonPressed(0, 3)) mask |= INPUT_PUNCH_R;
        // B/Circle (1) ou RB (5): Esquiva dedicada
        if (sf::Joystick::isButtonPressed(0, 1) || sf::Joystick::isButtonPressed(0, 5)) mask |= INPUT_DODGE_KEY;
        // LB (4) ou Gatilhos analógicos = Bloqueio
        if (sf::Joystick::isButtonPressed(0, 4)) mask |= INPUT_BLOCK;
        const float z = sf::Joystick::getAxisPosition(0, sf::Joystick::Axis::Z);
        if (std::fabs(z) > 40.0f) mask |= INPUT_BLOCK;
    }

    if (mask & INPUT_DODGE_KEY) {
        if (mask & INPUT_LEFT) mask |= INPUT_DODGE_L;
        else if (mask & INPUT_RIGHT) mask |= INPUT_DODGE_R;
        else if (mask & INPUT_DOWN) mask |= INPUT_DODGE_B;
        else mask |= INPUT_DUCK;
    }
    return mask;
}

void GameApp::tick() {
    const Screen curScreen = menus_.screen();
    if (curScreen != lastScreen_) {
        if (lastScreen_ == Screen::Versus) {
            audio_.stopVersusSounds();
        }
        if (lastScreen_ == Screen::Fight) {
            audio_.stopFightSounds();
        }
        if (curScreen == Screen::Fight) {
            prepareFighters(matchPlayer_, matchEnemy_);
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
            while (!fightersReady() && std::chrono::steady_clock::now() < deadline) {
                sprites_.pollUpload();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
        if (lastScreen_ == Screen::Title) {
            audio_.stop(Sfx::AnnouncerKorama);
        }
        if (lastScreen_ == Screen::Build) {
            if (curScreen != Screen::Fight && curScreen != Screen::CharSelect) {
                fightRigKeys_.clear();
                sprites_.keepRigs({});
                sprites_.keepFighterSheets(assetRoot_ / "sprites", {});
                loadingFighters_ = false;
                charSelectPreloadChar_ = ~0u;
                charSelectPreloadSkin_ = ~0u;
            }
        }
        if (lastScreen_ == Screen::CharSelect && curScreen != Screen::Build && curScreen != Screen::Versus && curScreen != Screen::Fight && curScreen != Screen::ModeSelect) {
            fightRigKeys_.clear();
            sprites_.keepRigs({});
            sprites_.keepFighterSheets(assetRoot_ / "sprites", {});
            loadingFighters_ = false;
            charSelectPreloadChar_ = ~0u;
            charSelectPreloadSkin_ = ~0u;
        }
        if (curScreen == Screen::CharSelect) {
            // Em vez de descartar os lutadores na tela de seleção, pré-carrega em segundo plano
            // o lutador e oponente selecionados para que a entrada na tela de Versus seja instantânea!
            const uint32_t p = menus_.selectedChar();
            const uint32_t e = menus_.opponentChar();
            charSelectPreloadChar_ = p;
            charSelectPreloadSkin_ = e;
            prepareFighters(p, e);
        }
        if (curScreen == Screen::Bracket) {
            // No modo Arcade, pré-carrega o próximo oponente da chave
            const uint32_t p = menus_.selectedChar();
            const int opp = menus_.tournament().currentOpponent();
            const uint32_t e = (opp >= 0) ? static_cast<uint32_t>(opp) : 1u;
            charSelectPreloadChar_ = p;
            charSelectPreloadSkin_ = e;
            prepareFighters(p, e);
        }
        if (curScreen == Screen::Title) {
            audio_.play(Sfx::AnnouncerKorama);
        }
        lastScreen_ = curScreen;
    }

    if (curScreen != Screen::Fight) {
        audio_.setMusic(MusicTrack::Menu); // A música do menu continua tocando normalmente na tela de Versus
        if (curScreen == Screen::Versus) {
            if (!pendingMatch_) pendingMatch_ = menus_.versusMatch();
            prepareFighters(pendingMatch_->playerChar, pendingMatch_->enemyChar);
            const int sf = menus_.screenFrame();
            const auto uploadStart = std::chrono::steady_clock::now();
            for (int i = 0; i < versusUploadPolls_ && sprites_.pollUpload(); ++i) {}
            lastVersusUploadMs_ = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - uploadStart).count();
            menus_.setVersusReady(fightersReady());
            bool failed = sprites_.fighterSheetsFailed();
            for (const auto& key : fightRigKeys_) failed = failed || sprites_.rigFailed(key);
            if (failed) menus_.versusFailure();
            if (sf == 1) {
                audio_.play(Sfx::VersusWhoosh);
            } else if (sf == 24) {
                audio_.play(Sfx::VersusClash);
                audio_.play(Sfx::CameraFlash);
                audio_.play(Sfx::VersusSlam);
            }
            // Duck da música do menu durante a colisão para destacar o estrondo e o eco estéreo:
            if (sf >= 24) {
                audio_.setMusicDuck(0.40f);
            }
        } else if (curScreen == Screen::Build) {
            sprites_.pollUpload(); // reap a previous fight's completed worker, if any
        } else if (curScreen == Screen::CharSelect) {
            const uint32_t p = menus_.selectedChar();
            const uint32_t e = menus_.opponentChar();
            if (p != charSelectPreloadChar_ || e != charSelectPreloadSkin_) {
                charSelectPreloadChar_ = p;
                charSelectPreloadSkin_ = e;
                prepareFighters(p, e);
            }
            audio_.setMusicDuck(1.0f);
        } else {
            audio_.setMusicDuck(1.0f);
        }
        // Retém o upload contínuo fora do combate para que tudo chegue pronto na hora da luta:
        if (curScreen != Screen::Versus && curScreen != Screen::Build) sprites_.pollUpload();
        handleMenuAction(menus_.update());
        return;
    }
    if (paused_) return;
    if (renderer_.freeCam.enabled) {
        const float dt = static_cast<float>(kTickSeconds);
        float speed = freeCamSpeed_;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift)) {
            speed *= 3.0f;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RControl)) {
            speed *= 0.2f;
        }

        const float cy = std::cos(renderer_.freeCam.yaw), sy = std::sin(renderer_.freeCam.yaw);
        const float cp = std::cos(renderer_.freeCam.pitch), sp = std::sin(renderer_.freeCam.pitch);
        const Vec3 fwd3D = {sy * cp, -sp, cy * cp};
        const Vec3 right = {cy, 0.0f, -sy};

        Vec3 move{0.0f, 0.0f, 0.0f};
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) {
            move.x += fwd3D.x; move.y += fwd3D.y; move.z += fwd3D.z;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) {
            move.x -= fwd3D.x; move.y -= fwd3D.y; move.z -= fwd3D.z;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
            move.x += right.x; move.z += right.z;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
            move.x -= right.x; move.z -= right.z;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::E)) {
            move.y += 1.0f;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::C) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Q)) {
            move.y -= 1.0f;
        }

        float moveLen = std::sqrt(move.x * move.x + move.y * move.y + move.z * move.z);
        if (moveLen > 0.0001f) {
            renderer_.freeCam.pos.x += (move.x / moveLen) * speed * dt;
            renderer_.freeCam.pos.y += (move.y / moveLen) * speed * dt;
            renderer_.freeCam.pos.z += (move.z / moveLen) * speed * dt;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) renderer_.freeCam.yaw -= 1.9f * dt;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) renderer_.freeCam.yaw += 1.9f * dt;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) renderer_.freeCam.pitch -= 1.9f * dt;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) renderer_.freeCam.pitch += 1.9f * dt;
        renderer_.freeCam.pitch = std::clamp(renderer_.freeCam.pitch, -1.45f, 1.45f);
        while (renderer_.freeCam.yaw > 3.14159f) renderer_.freeCam.yaw -= 2.0f * 3.14159f;
        while (renderer_.freeCam.yaw < -3.14159f) renderer_.freeCam.yaw += 2.0f * 3.14159f;

        return; // não avança a simulação do combate enquanto voa livre
    }
    if (engine_.trainingMode) {
        engine_.player.loadout = menus_.loadouts()[matchPlayer_];
    }
    engine_.update(buildInputMask(), static_cast<float>(kTickSeconds));
    tapLatch_ = 0;
    audio_.play(engine_.sounds);

    // Batalhator: acompanha dinamicamente a fase do robô e atualiza a música para a respectiva fase
    if (matchEnemy_ == 8 && !engine_.isMatchOver()) {
        MusicTrack bossTrack = currentMatchTrack();
        if (audio_.currentTrack() != bossTrack) {
            audio_.setMusic(bossTrack);
        }
    }

    if (engine_.isMatchOver()) {
        if (!matchMusicStopped_) {
            matchMusicStopped_ = true;
            knockoutMusicOffset_ = audio_.musicPlayingOffset();
        }
    }

    // Grava a luta para o replay (durante o combate e apenas os primeiros 35 quadros do knockdown para registrar
    // a queda na lona e o impacto inicial, sem gravar os 10 segundos de contagem na lona que apagariam a luta).
    const bool recordKnockdown = (engine_.gameState == STATE_KNOCKDOWN && engine_.refereeCount == 0 && engine_.refereeTimer >= 25);
    const bool fighting = engine_.gameState == STATE_FIGHTING || recordKnockdown;
    if ((!engine_.isMatchOver() && fighting) || (engine_.isMatchOver() && endFrames_ <= 95)) {
        replay_.push_back(engine_);
        if (replay_.size() > kReplayFrames) replay_.pop_front();
    }
    if (engine_.isMatchOver()) {
        ++endFrames_;
        const MatchResult r = engine_.result;
        const bool decisive = r == MatchResult::WinKO || r == MatchResult::WinTKO || r == MatchResult::LossKO || r == MatchResult::LossTKO;

        // Desaceleração suave da música (efeito tape stop de fita desacelerando pós-nocaute enquanto o lutador voa/cai)
        // e ducking gradual de volume:
        if (endFrames_ < kReplayDelay && !replayActive_) {
            const float p = static_cast<float>(endFrames_) / static_cast<float>(kReplayDelay);
            const float pitch = std::max(0.28f, 1.0f - 0.72f * (p * p));
            audio_.setMusicPitch(pitch);
            const float duck = std::max(0.18f, 1.0f - 0.82f * p);
            audio_.setMusicDuck(duck);
        }

        if (endFrames_ == kReplayDelay && decisive && replay_.size() >= 10 && !replayDone_) {
            replayActive_ = true;
            replayPhase_ = 0;
            rewindTimer_ = 0;
            replayTimer_ = 0.0f;
            lastReplayFrame_ = -1;
            audio_.setMusicDuck(0.08f); // Abaixa profundamente o volume da música para o efeito de rewind sobressair!
            audio_.play(Sfx::Rewind);
        }
        if (replayActive_) stepReplay();

        // Celebração sonoro-musical na tela de resultado (após o replay ou contagem da decisão):
        if (resultVisible() && !celebrationAudioStarted_) {
            celebrationAudioStarted_ = true;
            const bool won = (r == MatchResult::WinKO || r == MatchResult::WinTKO || r == MatchResult::WinDecision);
            const uint32_t winnerChar = won ? matchPlayer_ : matchEnemy_;
            audio_.setMusicDuck(1.0f);
            audio_.setMusic(charTrack(winnerChar));
            if (won) {
                audio_.play(Sfx::CrowdVictory);
                audio_.play(Sfx::CameraFlash);
            } else if (r == MatchResult::Draw) {
                audio_.play(Sfx::CrowdWoah);
            } else {
                audio_.play(Sfx::CrowdBoo);
            }
        }
        if (resultVisible() && celebrationAudioStarted_) {
            const bool won = (r == MatchResult::WinKO || r == MatchResult::WinTKO || r == MatchResult::WinDecision);
            if (won && (endFrames_ % 240 == 120)) {
                audio_.play(Sfx::CrowdCheer);
            }
        }
    }

    // Fim de luta do torneio: registra uma vez (vitória avança a chave, derrota encerra) e salva.
    if (matchArcade_ && engine_.isMatchOver() && !arcadeResultApplied_) {
        arcadeResultApplied_ = true;
        const MatchResult r = engine_.result;
        if (r == MatchResult::WinKO || r == MatchResult::WinTKO || r == MatchResult::WinDecision) menus_.tournamentWon();
        else menus_.tournamentLost();
        if (menus_.takeDirty()) saveBuilds();
    }
}

void GameApp::render() {
    window_.clear(sf::Color::Black, sf::StencilValue{0u});
    window_.setView(view_);
    if (menus_.screen() != Screen::Fight) {
        menus_.draw(window_);
    } else if (replayActive_ && !replay_.empty()) {
        const std::size_t last = replay_.size() - 1;
        std::size_t idx;
        if (replayPhase_ == 0) {
            float p = static_cast<float>(rewindTimer_) / kRewindFrames;
            idx = static_cast<std::size_t>(std::max(0.0f, (1.0f - p) * static_cast<float>(last)));
        } else {
            idx = std::min(last, static_cast<std::size_t>(replayTimer_));
        }
        renderer_.draw(window_, replay_[idx], false, ReplayView{replayPhase_, replayTimer_}, false);
    } else {
        renderer_.draw(window_, engine_, paused_, ReplayView{}, resultVisible());
    }
    if (renderer_.freeCam.enabled && menus_.screen() == Screen::Fight) {
        sf::RectangleShape banner({1600.0f - 40.0f, 44.0f});
        banner.setPosition({20.0f, 16.0f});
        banner.setFillColor(sf::Color(14, 18, 28, 230));
        banner.setOutlineColor(sf::Color(255, 215, 0));
        banner.setOutlineThickness(2.0f);
        window_.draw(banner);

        if (displayFont_) {
            const std::string bannerMsg = "MODO NOCLIP ATIVO | WASD: VOAR | ESPACO/C: SUBIR/DESCER | ARRASTE MOUSE: OLHAR | F7: VOLTAR";
            sf::Text t(*displayFont_, sf::String::fromUtf8(bannerMsg.begin(), bannerMsg.end()), 18u);
            t.setPosition({34.0f, 25.0f});
            t.setFillColor(sf::Color(255, 220, 60));
            window_.draw(t);
        }
    }
    window_.display();
}

// Rebobina (45 quadros) e passa a fita duas vezes em câmera lenta: sua visão e a do oponente.
void GameApp::stepReplay() {
    if (replayPhase_ == 0) {
        if (++rewindTimer_ >= kRewindFrames) {
            replayPhase_ = 1;
            replayTimer_ = 0.0f;
            lastReplayFrame_ = -1;
            audio_.setMusicDuck(1.0f);
            const float replaySec = static_cast<float>(replay_.size()) / 60.0f;
            const float seekSec = std::max(0.0f, knockoutMusicOffset_.asSeconds() - replaySec);
            audio_.setReplayMusic(currentMatchTrack(), 0.82f, sf::seconds(seekSec));
        }
        return;
    }

    // Dispara os sons gravados no deque no tempo exato da fita em câmera lenta (pitch 0.78x e abafado)
    const int prevFrame = lastReplayFrame_;
    replayTimer_ += kReplaySpeed;
    const int curFrame = static_cast<int>(replayTimer_);
    if (curFrame > prevFrame && curFrame < static_cast<int>(replay_.size())) {
        for (int f = std::max(0, prevFrame + 1); f <= curFrame && f < static_cast<int>(replay_.size()); ++f) {
            const auto& cues = replay_[static_cast<std::size_t>(f)].sounds;
            if (!cues.empty()) {
                audio_.playReplay(cues, 0.78f);
            }
        }
        lastReplayFrame_ = curFrame;
    }

    if (static_cast<std::size_t>(replayTimer_) >= replay_.size()) {
        if (replayPhase_ == 1) {
            replayPhase_ = 2;
            replayTimer_ = 0.0f;
            lastReplayFrame_ = -1;
        } else {
            replayActive_ = false;
            replayDone_ = true;
            lastReplayFrame_ = -1;
            audio_.setMusic(MusicTrack::None);
        }
    }
}

MusicTrack GameApp::charTrack(uint32_t charId) const {
    if (charId == 8) {
        const int bStage = engine_.enemy.bossStage;
        return (bStage == 2) ? MusicTrack::BossPhase2 :
               (bStage == 3) ? MusicTrack::BossPhase3 : MusicTrack::BossPhase1;
    }
    switch (charId) {
    case 0: return MusicTrack::Jack;
    case 1: return MusicTrack::KingFrog;
    case 2: return MusicTrack::Joana;
    case 3: return MusicTrack::Cremoso;
    case 4: return MusicTrack::Valeria;
    case 5: return MusicTrack::Vivaldi;
    case 6: return MusicTrack::Espinha;
    case 7: return MusicTrack::Marcov;
    default: return MusicTrack::Fight;
    }
}

MusicTrack GameApp::currentMatchTrack() const {
    return charTrack(matchEnemy_);
}

bool GameApp::resultVisible() const {
    return !engine_.isMatchOver() || (endFrames_ >= kReplayDelay && !replayActive_);
}

void GameApp::updateLetterbox(sf::Vector2u windowSize) {
    if (windowSize.x == 0 || windowSize.y == 0) return;
    // As duas vistas ocupam a mesma área 16:9, com faixas pretas se a janela tiver outra proporção.
    const float windowRatio = static_cast<float>(windowSize.x) / static_cast<float>(windowSize.y);
    const float targetRatio = kUiWidth / kUiHeight;
    sf::FloatRect viewport({0.0f, 0.0f}, {1.0f, 1.0f});
    if (windowRatio > targetRatio) {
        viewport.size.x = targetRatio / windowRatio;
        viewport.position.x = (1.0f - viewport.size.x) / 2.0f;
    } else {
        viewport.size.y = windowRatio / targetRatio;
        viewport.position.y = (1.0f - viewport.size.y) / 2.0f;
    }
    view_.setViewport(viewport);
    uiView_.setViewport(viewport);
}

void GameApp::prepareFighters(uint32_t playerChar, uint32_t enemyChar) {
    sprites_.requestFighterSheets(assetRoot_ / "sprites", {playerChar, enemyChar});
    std::vector<std::string> targetKeys = {fighterRigKey(playerChar, menus_.skin(playerChar)), fighterRigKey(enemyChar, menus_.skin(enemyChar))};
    if (fightRigKeys_ == targetKeys && loadingFighters_) {
        return;
    }
    fightRigKeys_ = std::move(targetKeys);
    sprites_.keepRigs(fightRigKeys_);
    for (const auto& key : fightRigKeys_) sprites_.requestRig(key);
    loadingFighters_ = true;
}

bool GameApp::fightersReady() const {
    return sprites_.fighterSheetsReady() && sprites_.rigsReady(fightRigKeys_);
}

void GameApp::startMatch(uint32_t playerChar, uint32_t enemyChar, const SkillLoadout& loadout, bool arcade, int stage, bool training) {
    training = training || (enemyChar == 9);
    prepareFighters(playerChar, enemyChar);
    if (!fightersReady()) {
        MenuAction match;
        match.type = MenuAction::Type::StartMatch;
        match.playerChar = playerChar; match.enemyChar = enemyChar;
        match.playerSkin = menus_.skin(playerChar); match.enemySkin = menus_.skin(enemyChar);
        match.loadout = loadout; match.arcade = arcade; match.stage = stage; match.training = training;
        pendingMatch_ = match;
        menus_.setSelectedChar(playerChar); menus_.setOpponentChar(enemyChar);
        menus_.setVersusTag(training ? "MODO TREINO" : arcade ? "TORNEIO ARCADE" : "LUTA SOLO");
        menus_.goTo(Screen::Versus);
        return;
    }
    loadingFighters_ = false;
    pendingMatch_.reset();
    matchPlayer_ = playerChar;
    matchEnemy_ = enemyChar;
    matchLoadout_ = loadout;
    matchArcade_ = arcade;
    matchStage_ = stage;
    matchTraining_ = training;
    arcadeResultApplied_ = false;
    replay_.clear();
    renderer_.resetEffects();
    renderer_.startFightFade(22);
    endFrames_ = 0;
    replayActive_ = false;
    replayDone_ = false;
    lastReplayFrame_ = -1;
    matchMusicStopped_ = false;
    knockoutMusicOffset_ = sf::Time::Zero;
    celebrationAudioStarted_ = false;
    audio_.setFightPaused(false);
    audio_.stopVersusSounds();
    audio_.stopFightSounds();
    audio_.setMusicPitch(1.0f);
    audio_.setMusicDuck(1.0f);
    setPaused(false);
    tapLatch_ = 0;
    menus_.goTo(Screen::Fight);
    engine_.startMatch(playerChar, enemyChar, arcade, stage, &matchLoadout_, matchTraining_);
    engine_.player.skinId = menus_.skin(playerChar);
    engine_.enemy.skinId = menus_.skin(enemyChar);
    engine_.startRoundIntro(324); // contagem regressiva sincronizada com narrador (0.2s K, 1.4s O, 2.4s Rama, 3.6s-5.4s Luta/vinheta)
    engine_.update(0, 0.0f); // preenche o estado de render antes do primeiro tick
    audio_.play(Sfx::Bell);
    audio_.play(Sfx::AnnouncerKorama);
    audio_.setMusic(currentMatchTrack());
}

void GameApp::cycleTrainingFighter(bool forward) {
    if (!engine_.trainingMode) return;
    std::vector<uint32_t> pool;
    for (uint32_t i = 0; i < 8; ++i) {
        if (kRoster[i].hasFightSprites) pool.push_back(i);
    }
    if (pool.empty()) return;
    int curIdx = 0;
    for (std::size_t i = 0; i < pool.size(); ++i) {
        if (pool[i] == matchPlayer_) { curIdx = static_cast<int>(i); break; }
    }
    int nextIdx = forward ? (curIdx + 1) % static_cast<int>(pool.size())
                          : (curIdx - 1 + static_cast<int>(pool.size())) % static_cast<int>(pool.size());
    const uint32_t next = pool[static_cast<std::size_t>(nextIdx)];
    MenuAction match;
    match.type = MenuAction::Type::StartMatch;
    match.playerChar = next; match.enemyChar = matchEnemy_;
    match.playerSkin = menus_.skin(next); match.enemySkin = menus_.skin(matchEnemy_);
    match.loadout = menus_.loadouts()[next]; match.training = true;
    pendingMatch_ = match; pendingTrainingSwap_ = true;
    setPaused(false);
    menus_.setSelectedChar(next); menus_.setOpponentChar(matchEnemy_);
    menus_.setVersusTag("MODO TREINO"); menus_.goTo(Screen::Versus);
    prepareFighters(next, matchEnemy_);
    announceLoadedFight_ = false;
    audio_.play(Sfx::Select);
}

void GameApp::leaveFight() {
    loadingFighters_ = announceLoadedFight_ = false;
    pendingMatch_.reset(); pendingTrainingSwap_ = false;
    fightRigKeys_.clear();sprites_.keepRigs({});sprites_.keepFighterSheets(assetRoot_ / "sprites", {});
    setPaused(false);
    audio_.setFightPaused(false);
    audio_.stopFightSounds();
    audio_.stopVersusSounds();
    celebrationAudioStarted_ = false;
    audio_.setMusicPitch(1.0f);
    audio_.setMusicDuck(1.0f);
    audio_.setMusic(MusicTrack::Menu);
    menus_.goTo(Screen::Title);
}

void GameApp::setPaused(bool paused) {
    if (paused_ == paused) return;
    paused_ = paused;
    if (menus_.screen() == Screen::Fight) {
        audio_.setFightPaused(paused_);
    }
}

// Builds salvas em texto: "build <char_id> <especial1> <especial2> <passiva>" (ids do SKILL_DATABASE, "-" = vazio),
// mais o torneio em andamento numa linha "campaign ..." (Tournament::serialize).
std::filesystem::path GameApp::savePath() const {
    return assetRoot_.parent_path() / "korama_save.txt";
}

void GameApp::loadBuilds() {
    std::ifstream in(savePath());
    std::string line;
    while (std::getline(in, line)) {
        if (line == "visual novo") { RingRenderer::liveVisual = true; continue; }
        if (line.rfind("keys ", 0) == 0) {
            keyBindings_ = KeyBindings::deserialize(line.substr(5));
            menus_.setKeyBindings(keyBindings_);
            renderer_.setKeyBindings(keyBindings_);
            continue;
        }
        if (line.rfind("volume ", 0) == 0) {
            int sfx = 80, mus = 80;
            if (std::sscanf(line.c_str(), "volume %d %d", &sfx, &mus) == 2) {
                menus_.setVolumes(sfx, mus);
                audio_.setSfxVolume(static_cast<float>(sfx));
                audio_.setMusicVolume(static_cast<float>(mus));
            }
            continue;
        }
        if (auto campaign = Tournament::parse(line)) {
            menus_.setTournament(*campaign);
            continue;
        }
        if (line.rfind("skins ", 0) == 0) {
            std::istringstream ss(line.substr(6));
            uint32_t s = 0, c = 0;
            while (ss >> s && c < 10) {
                menus_.setSkin(c++, s);
            }
            continue;
        }
        std::istringstream ss(line);
        std::string tag, k1, k2, k3;
        uint32_t charId = 0;
        if (!(ss >> tag >> charId >> k1 >> k2 >> k3) || tag != "build") continue;
        auto idOf = [](const std::string& key) {
            const SkillDef* def = SkillSystem::getByKey(key);
            return def ? def->id : SkillId::None;
        };
        menus_.setLoadout(charId, SkillLoadout{idOf(k1), idOf(k2), idOf(k3)});
    }
}

void GameApp::saveBuilds() {
    std::ofstream out(savePath());
    if (!out) return;
    out << "# K.O.-RAMA - configuracoes e builds equipadas\n";
    out << "volume " << menus_.sfxVolume() << ' ' << menus_.musicVolume() << '\n';
    out << "keys " << keyBindings_.serialize() << '\n';
    out << "skins";
    for (uint32_t s : menus_.skins()) out << ' ' << s;
    out << '\n';
    const auto& loadouts = menus_.loadouts();
    for (uint32_t c = 0; c < loadouts.size(); ++c) {
        if (!SkillSystem::hasSkills(c)) continue;
        auto keyOf = [](SkillId id) {
            const SkillDef* def = SkillSystem::get(id);
            return std::string(def ? def->key : "-");
        };
        out << "build " << c << ' ' << keyOf(loadouts[c].spec1) << ' ' << keyOf(loadouts[c].spec2) << ' '
            << keyOf(loadouts[c].passive) << '\n';
    }
    if (menus_.tournament().active()) out << menus_.tournament().serialize() << '\n';
    if (RingRenderer::liveVisual) out << "visual novo\n";
}

void GameApp::dumpTestFrame(const std::filesystem::path& outPng, bool training, int punchTick) {
    (void)window_.setActive(true);
    std::ofstream dbg("dump_log.txt");
    dbg << "Entering dumpTestFrame with outPng=" << outPng << " punchTick=" << punchTick << std::endl;
    if (training) {
        startMatch(0, 9, SkillLoadout{}, false, 1, true);
    } else if (punchTick == 8) {
        startMatch(0, 2, SkillLoadout{}, false, 1, false);
    } else {
        startMatch(0, 1, SkillLoadout{}, false, 1, false);
    }
    dbg << "startMatch finished, screen=" << static_cast<int>(menus_.screen()) << std::endl;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(60);
    while (menus_.screen() == Screen::Versus) {
        tick();
        if (std::chrono::steady_clock::now() >= deadline) {
            dbg << "Asset preparation timed out!\n";
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    dbg << "Passed Versus screen, ticking 155 frames...\n";
    for (int i = 0; i < 155; ++i) {
        tick();
    }
    dbg << "Ticked 155 frames\n";
    if (punchTick == 1) {
        engine_.player.state = FIGHTER_HOOK;
        engine_.player.timer = 10;
        engine_.player.maxTimer = 16;
        engine_.player.hitDir = 1;
        engine_.enemy.state = FIGHTER_HIT;
        engine_.enemy.timer = 12;
        engine_.enemy.maxTimer = 18;
        engine_.enemy.hitDir = -1;
        engine_.enemy.hitKind = 2; // Hook reaction!
        engine_.enemy.hp = 84.0f;
        engine_.comboCount = 3;
        engine_.comboTimer = 75;
        engine_.particles.spawnStraw(engine_.enemy.worldPos.x, 70.0f, engine_.enemy.worldPos.z, 22);
    } else if (punchTick == 2) {
        engine_.player.state = FIGHTER_UPPER;
        engine_.player.timer = 8;
        engine_.player.maxTimer = 20;
        engine_.player.hitDir = 1;
        engine_.enemy.state = FIGHTER_HIT;
        engine_.enemy.timer = 9;
        engine_.enemy.maxTimer = 18;
        engine_.enemy.hitDir = 1;
        engine_.enemy.hitKind = 4; // Uppercut reaction!
        engine_.enemy.hp = 70.0f;
        engine_.comboCount = 5;
        engine_.comboTimer = 75;
        engine_.particles.spawnStraw(engine_.enemy.worldPos.x, 90.0f, engine_.enemy.worldPos.z, 28);
    } else if (punchTick == 3) {
        engine_.enemy.status.infested = 180;
    } else if (punchTick == 4) {
        engine_.player.state = FIGHTER_SKILL;
        engine_.player.activeSkill = SkillId::JackInfestation;
        engine_.player.timer = 15;
        engine_.player.maxTimer = 30;
        engine_.enemy.status.infested = 180;
    } else if (punchTick == 5) {
        engine_.enemy.status.blind = 180;
        engine_.combat.triggerStatusBanner("¡INFESTAÇÃO!");
    } else if (punchTick == 6) {
        engine_.gameState = STATE_KNOCKDOWN;
        engine_.player.state = FIGHTER_KNOCKDOWN;
        engine_.enemyIsDown = false;
        engine_.refereeCount = 5;
        engine_.mashTarget = 20;
        engine_.mashProgress = 13;
    } else if (punchTick == 7) {
        engine_.result = MatchResult::WinKO;
    }
    engine_.roundIntroTimer = 0;
    if (punchTick != 6) {
        engine_.gameState = STATE_FIGHTING;
    }
    renderer_.resetEffects();
    sf::ContextSettings gl;
    gl.stencilBits = 8;
    sf::RenderTexture rt;
    if (rt.resize({1600u, 900u}, gl)) {
        rt.clear(sf::Color::Black, sf::StencilValue{0u});
        rt.setView(view_);
        renderer_.draw(rt, engine_, paused_, ReplayView{}, punchTick == 7 ? true : resultVisible());
        rt.display();
        bool ok = rt.getTexture().copyToImage().saveToFile(outPng.string());
        dbg << "[DUMP] Saved " << outPng.string() << " : " << (ok ? "SUCCESS" : "FAILED") << std::endl;
        std::cout << "[DUMP] Saved " << outPng.string() << " : " << (ok ? "SUCCESS" : "FAILED") << std::endl;
    } else {
        dbg << "[DUMP] rt.resize failed!" << std::endl;
        std::cout << "[DUMP] rt.resize failed!" << std::endl;
    }
}

void GameApp::dumpMenuFrame(const std::filesystem::path& outPng, int modeIndex) {
    (void)window_.setActive(true);
    menus_.goTo(Screen::ModeSelect);
    menus_.setSelectedChar(0);
    menus_.setModeIndex(modeIndex);
    for (int i = 0; i < 45; ++i) {
        menus_.update();
    }
    sf::ContextSettings gl;
    gl.stencilBits = 8;
    sf::RenderTexture rt;
    if (rt.resize({1600u, 900u}, gl)) {
        rt.clear(sf::Color::Black, sf::StencilValue{0u});
        rt.setView(view_);
        menus_.draw(rt);
        rt.display();
        bool ok = rt.getTexture().copyToImage().saveToFile(outPng.string());
        std::cout << "[DUMP] Saved " << outPng.string() << " : " << (ok ? "SUCCESS" : "FAILED") << std::endl;
    } else {
        std::cout << "[DUMP] rt.resize failed!" << std::endl;
    }
}

void GameApp::dumpBuildFrame(const std::filesystem::path& outPng, uint32_t charId, uint32_t skinId) {
    (void)window_.setActive(true);
    menus_.goTo(Screen::CharSelect);
    menus_.setSelectedChar(charId);
    menus_.setSkin(charId, skinId);
    menus_.openBuild(Screen::CharSelect);
    tick();
    // Build previews are prerecorded and deliberately do not load combat rigs.
    for (int i = 0; i < 45; ++i) {
        tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    {
        std::ofstream dbg("build_debug.txt");
        dbg << "build preview uses prerecorded video; combat rigs are not requested\n";
        sprites_.dumpDebug(dbg);
    }
    sf::ContextSettings gl;
    gl.stencilBits = 8;
    sf::RenderTexture rt;
    if (rt.resize({1600u, 900u}, gl)) {
        rt.clear(sf::Color::Black, sf::StencilValue{0u});
        rt.setView(view_);
        menus_.draw(rt);
        rt.display();
        bool ok = rt.getTexture().copyToImage().saveToFile(outPng.string());
        std::cout << "[DUMP] Saved " << outPng.string() << " : " << (ok ? "SUCCESS" : "FAILED") << std::endl;
    } else {
        std::cout << "[DUMP] rt.resize failed!" << std::endl;
    }
}

void GameApp::dumpBootFrames() {
    (void)window_.setActive(true);
    sf::ContextSettings gl;
    gl.stencilBits = 8;
    sf::RenderTexture rt;
    if (!rt.resize({1600u, 900u}, gl)) return;

    struct Capture { int frame; const char* file; };
    const Capture caps[] = {
        {9, "snap_boot_0_jack.png"},
        {27, "snap_boot_1_frog.png"},
        {45, "snap_boot_2_joana.png"},
        {63, "snap_boot_3_cremoso.png"},
        {81, "snap_boot_4_valeria.png"},
        {99, "snap_boot_5_vivaldi.png"},
        {117, "snap_boot_6_espinha.png"},
        {135, "snap_boot_7_marcov.png"},
        {153, "snap_boot_8_batalhator.png"},
        {171, "snap_boot_9_espantalho.png"},
        {190, "snap_boot_10_complete.png"},
    };

    menus_.goTo(Screen::Boot);
    int curTarget = 0;
    for (int f = 0; f <= 195 && curTarget < 11; ++f) {
        if (f == caps[curTarget].frame) {
            rt.clear(sf::Color::Black, sf::StencilValue{0u});
            rt.setView(view_);
            menus_.draw(rt);
            rt.display();
            bool ok = rt.getTexture().copyToImage().saveToFile(caps[curTarget].file);
            std::cout << "[DUMP] " << caps[curTarget].file << " : " << (ok ? "SUCCESS" : "FAILED") << std::endl;
            ++curTarget;
        }
        menus_.update();
    }
}

} // namespace KoRama
