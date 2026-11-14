#include "ring_renderer.hpp"
#include "joana_visual.hpp"
#include "vfx/psx_bee.hpp"
#include "celebration.hpp"
#include <chrono>
#ifdef _WIN32
extern "C" __declspec(dllimport) void __stdcall glFinish();
#else
extern "C" void glFinish();
#endif

namespace {
// Espera a GPU terminar (só para medir com precisão quando KORAMA_PROFILE está definido).
void glFinishHack(sf::RenderTarget&) {
    static const bool on = std::getenv("KORAMA_PROFILE") != nullptr;
    if (on) glFinish();
}
} // namespace
#include "config.hpp"
#include "physics.hpp"
#include "ui_draw.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>
#include <vector>

namespace KoRama {

namespace {

constexpr float kPi = 3.14159265f;

// Geometria do ringue em buildRenderQueue() do index.html.
constexpr float kRingHalf = 270.0f;
constexpr float kTileStep = 54.0f;
constexpr float kApronDepth = 35.0f;
constexpr float kPostTop = 125.0f;
constexpr std::array<float, 3> kRopeHeights = {38.0f, 68.0f, 98.0f};
constexpr float kNearDepth = 250.0f; // mais perto que isso da câmera = desenhado na frente dos lutadores

struct HarborBoat { float x, z, angle, length, beam; int columns, rows; };
constexpr std::array<HarborBoat, 31> kHarborBoats = {{
    {-575.0f, -440.0f, 0.25f, 390.0f, 100.0f, 3, 2},
    {0.0f, -625.0f, 1.40f, 425.0f, 102.0f, 3, 2},
    {575.0f, -450.0f, 2.70f, 390.0f, 100.0f, 3, 2},
    {-470.0f, -135.0f, -0.30f, 300.0f, 72.0f, 3, 1},
    {470.0f, -135.0f, 0.40f, 300.0f, 72.0f, 3, 1},
    {-545.0f, 605.0f, 0.70f, 370.0f, 92.0f, 3, 2},
    {0.0f, 650.0f, 1.20f, 390.0f, 95.0f, 3, 2},
    {555.0f, 605.0f, -0.80f, 365.0f, 90.0f, 3, 2},
    {-1350.0f, 880.0f, 0.55f, 360.0f, 90.0f, 3, 1},
    {-920.0f, 980.0f, 0.90f, 370.0f, 92.0f, 3, 2},
    {-330.0f, 1060.0f, 0.35f, 390.0f, 95.0f, 3, 2},
    {330.0f, 1080.0f, -0.40f, 385.0f, 94.0f, 3, 2},
    {920.0f, 990.0f, -0.85f, 370.0f, 92.0f, 3, 2},
    {1350.0f, 890.0f, -0.60f, 360.0f, 90.0f, 3, 1},
    {-1100.0f, 250.0f, -0.20f, 340.0f, 85.0f, 3, 1},
    {1100.0f, 250.0f, 0.25f, 340.0f, 85.0f, 3, 1},
    {-1050.0f, -350.0f, 0.40f, 350.0f, 88.0f, 3, 1},
    {1050.0f, -350.0f, -0.45f, 350.0f, 88.0f, 3, 1},
    {-600.0f, -950.0f, 0.80f, 380.0f, 95.0f, 3, 2},
    {600.0f, -950.0f, -0.75f, 380.0f, 95.0f, 3, 2},
    {0.0f, -1120.0f, 1.57f, 400.0f, 98.0f, 3, 2},
    {-1750.0f, 1450.0f, 0.65f, 350.0f, 88.0f, 3, 0},
    {-1150.0f, 1580.0f, 0.85f, 370.0f, 92.0f, 3, 1},
    {-580.0f, 1680.0f, 0.20f, 360.0f, 90.0f, 3, 1},
    {0.0f, 1750.0f, 1.30f, 390.0f, 96.0f, 3, 1},
    {580.0f, 1690.0f, -0.30f, 360.0f, 90.0f, 3, 1},
    {1180.0f, 1590.0f, -0.75f, 370.0f, 92.0f, 3, 1},
    {1750.0f, 1460.0f, -0.60f, 350.0f, 88.0f, 3, 0},
    {-1400.0f, -1100.0f, 0.50f, 350.0f, 88.0f, 3, 0},
    {1400.0f, -1100.0f, -0.50f, 350.0f, 88.0f, 3, 0},
    {0.0f, -1650.0f, 1.57f, 380.0f, 94.0f, 3, 0}
}};

float harborWaterHeight(float x, float z, float frame) {
    return -43.0f + 3.0f * std::sin(x * 0.015f + frame * 0.031f)
                  + 2.0f * std::sin(z * 0.020f - frame * 0.023f);
}

Vec3 harborLandingPoint(const Vec3& start, float frame) {
    const float len = std::hypot(start.x, start.z);
    const float dx = len > 1.0f ? start.x / len : 0.0f;
    const float dz = len > 1.0f ? start.z / len : 1.0f;
    // Do lado distante, mira primeiro a lateral da plataforma: a borda de madeira
    // não pode cobrir a espuma na câmera de combate. Na frente, evita a lateral
    // extrema para que o impacto não saia do enquadramento.
    constexpr std::array<float, 11> farOffsets = {0.95f, -0.95f, 0.82f, -0.82f, 0.62f, -0.62f,
                                                    0.43f, -0.43f, 0.22f, -0.22f, 0.0f};
    constexpr std::array<float, 11> nearOffsets = {0.43f, -0.43f, 0.62f, -0.62f, 0.22f, -0.22f,
                                                     0.82f, -0.82f, 0.0f, 1.02f, -1.02f};
    const auto& offsets = dz > 0.0f ? farOffsets : nearOffsets;
    for (float offset : offsets) {
        const float co = std::cos(offset), si = std::sin(offset);
        const float nx = dx * co - dz * si, nz = dx * si + dz * co;
        const float shore = (kRingHalf + 52.0f) / std::max(std::fabs(nx), std::fabs(nz));
        for (float extra : {0.0f, 36.0f, 72.0f}) {
            const float radius = std::max(shore, len + 90.0f) + extra;
            const float x = nx * radius, z = nz * radius;
            bool underBoat = false;
            for (const HarborBoat& boat : kHarborBoats) {
                const float bx = x - boat.x, bz = z - boat.z;
                const float ca = std::cos(boat.angle), sa = std::sin(boat.angle);
                const float u = bx * ca + bz * sa, v = -bx * sa + bz * ca;
                if (u > -boat.length * 0.50f - 25.0f && u < boat.length * 0.55f + 25.0f &&
                    std::fabs(v) < boat.beam + 25.0f) { underBoat = true; break; }
            }
            if (!underBoat) return {x, harborWaterHeight(x, z, frame), z};
        }
    }
    // Todas as rotas próximas ocupadas: cai além da frota, nunca dentro de um casco.
    const float x = dx * 2200.0f, z = dz * 2200.0f;
    return {x, harborWaterHeight(x, z, frame), z};
}

// Camadas da fila de desenho (mesma escala do renderQueue do jogo JS).
constexpr float kLayerApronFar = 0.8f;
constexpr float kLayerFloor = 1.0f;
constexpr float kLayerBorder = 2.0f;
constexpr float kLayerPostFar = 2.9f;
constexpr float kLayerRopeFar = 3.0f;
constexpr float kLayerParticles = 4.0f;
constexpr float kLayerApronNear = 4.9f;
constexpr float kLayerRopeNear = 5.0f;
constexpr float kLayerPostNear = 5.1f;

const sf::Color kInk(0, 0, 0);

// GLSL Fragment Shader de Alta Performance:
// 1. Shockwave Ripple: Distorção radial UV atrelada a Counters, Especiais e Golpes Críticos.
// 2. Chromatic Aberration: Separação de canais RGB atrelada ao screen shake e knockdowns.
// 3. Comic Book Halftone / Retícula Spider-Verse: Pontos de nanquim rotacionados a 45° nas áreas de sombra.
static const char* kPostFxFragShader = R"(
uniform sampler2D u_texture;
uniform vec2 u_resolution;
uniform vec2 u_shockCenter;
uniform float u_shockProgress;
uniform float u_shockStrength;
uniform float u_chromaOffset;
uniform float u_halftoneEnabled;
uniform float u_halftoneScale;

void main() {
    vec2 texCoord = gl_TexCoord[0].xy;

    // 1. Shockwave Ripple
    if (u_shockProgress >= 0.0 && u_shockProgress <= 1.0 && u_shockStrength > 0.0) {
        float aspect = u_resolution.x / u_resolution.y;
        vec2 diff = (texCoord - u_shockCenter);
        diff.x *= aspect;
        float dist = length(diff);
        float currentRadius = u_shockProgress * 0.9;
        float ringWidth = 0.12;
        if (dist > currentRadius - ringWidth && dist < currentRadius + ringWidth) {
            float fade = 1.0 - u_shockProgress;
            float wave = sin((dist - currentRadius) / ringWidth * 3.14159265);
            vec2 normDiff = normalize(vec2(diff.x / aspect, diff.y));
            float offset = wave * u_shockStrength * fade * 0.045;
            texCoord += normDiff * offset;
        }
    }

    // 2. Chromatic Aberration Dinâmica
    vec4 col;
    if (u_chromaOffset > 0.0005) {
        vec2 dir = texCoord - vec2(0.5, 0.5);
        vec2 shift = dir * u_chromaOffset;
        col.r = texture2D(u_texture, texCoord + shift).r;
        col.g = texture2D(u_texture, texCoord).g;
        col.b = texture2D(u_texture, texCoord - shift).b;
        col.a = texture2D(u_texture, texCoord).a;
    } else {
        col = texture2D(u_texture, texCoord);
    }

    // 3. Spider-Verse Comic Halftone (Retícula de Pontos Ben-Day em tons de sombra)
    if (u_halftoneEnabled > 0.5) {
        float lum = dot(col.rgb, vec3(0.299, 0.587, 0.114));
        if (lum < 0.72) {
            float angle = 0.785398; // 45 graus
            vec2 p = gl_FragCoord.xy;
            mat2 rot = mat2(cos(angle), -sin(angle), sin(angle), cos(angle));
            vec2 rotP = rot * p;
            float scale = max(4.0, u_halftoneScale);
            vec2 cell = fract(rotP / scale) - vec2(0.5, 0.5);
            float distDot = length(cell);
            float maxR = (0.72 - lum) * 0.75;
            if (distDot < maxR) {
                // Mistura suave de retícula estilo nanquim de história em quadrinhos
                col.rgb = mix(col.rgb, col.rgb * 0.55, 0.45);
            }
        }
    }

    // 4. Contact Flash Brusco (Clarão focal no ponto de impacto nos primeiros frames)
    if (u_shockProgress < 0.18 && u_shockStrength > 0.7) {
        float aspect = u_resolution.x / u_resolution.y;
        vec2 fdiff = texCoord - u_shockCenter;
        fdiff.x *= aspect;
        float flashDist = length(fdiff);
        if (flashDist < 0.28) {
            float flash = (1.0 - flashDist / 0.28) * (1.0 - u_shockProgress / 0.18) * 0.50;
            col.rgb += vec3(flash);
        }
    }

    gl_FragColor = col;
}
)";

// Acabamento impresso apenas no céu, antes de ringue e lutadores: não custa
// outra passagem de tela cheia nem altera as silhuetas usadas no combate.
static const char* kComicSkyFragShader = R"(
void main() {
    vec3 base = gl_Color.rgb;
    vec3 inked = floor(base * 7.0 + 0.5) / 7.0;
    vec3 color = mix(base, inked, 0.30);
    vec2 dots = mod(vec2(gl_FragCoord.x + gl_FragCoord.y * 0.5,
                         gl_FragCoord.y), 9.0) - 4.5;
    float shadow = 1.0 - dot(base, vec3(0.299, 0.587, 0.114));
    float dotInk = (1.0 - smoothstep(1.3, 2.0, length(dots))) *
                   max(0.0, shadow - 0.30) * 0.09;
    gl_FragColor = vec4(color * (1.0 - dotInk), gl_Color.a);
}
)";

// Luz de impressão aplicada só aos planos atrás do ringue. As três faixas de
// Luz de impressão aplicada suavemente às bordas do cenário. Vinheta orgânica
// contínua onde a retícula Ben-Day encolhe até dissolver no céu, sem cortes retos ou degraus.
static const char* kComicBackdropFragShader = R"(
uniform vec2 u_resolution;
uniform vec3 u_ink;
uniform vec3 u_light;
uniform float u_time;

void main() {
    vec2 uv = gl_FragCoord.xy / u_resolution;

    // Vinheta elíptica suave e contínua nos cantos externos (sem cortes retos ou degraus)
    vec2 delta = uv - vec2(0.5, 0.44);
    delta.x *= 1.25;
    float dist = length(delta);
    float vignette = smoothstep(0.32, 0.90, dist);

    // Retícula Ben-Day rotacionada a 45 graus
    vec2 dotPos = mod(vec2(gl_FragCoord.x + gl_FragCoord.y * 0.5, gl_FragCoord.y), 10.0) - vec2(5.0);
    float dotDist = length(dotPos);

    // O raio de cada ponto encolhe organicamente até evaporar rumo ao centro do céu
    float dotRadius = clamp((vignette - 0.12) * 3.2, 0.0, 2.4);
    float inkDot = (dotRadius > 0.1) ? (1.0 - smoothstep(max(0.0, dotRadius - 0.5), dotRadius + 0.4, dotDist)) : 0.0;

    // Tinta gráfica de quadrinhos com leve calor atmosférico
    vec3 color = mix(u_ink, u_light, 0.12 + 0.08 * sin(u_time * 0.5));

    // Alpha orgânico e suave: desaparece totalmente no céu aberto
    float alpha = vignette * 0.12 + inkDot * 0.10 * vignette;
    gl_FragColor = vec4(color, alpha);
}
)";

sf::Color withAlpha(sf::Color c, float alpha) {
    c.a = static_cast<std::uint8_t>(std::clamp(alpha, 0.0f, 1.0f) * c.a);
    return c;
}

sf::Color hex(uint32_t rgb) {
    return sf::Color(static_cast<std::uint8_t>(rgb >> 16), static_cast<std::uint8_t>(rgb >> 8), static_cast<std::uint8_t>(rgb));
}

void drawThickLine(sf::RenderTarget& target, sf::Vector2f a, sf::Vector2f b, float thickness, sf::Color color) {
    sf::Vector2f d = b - a;
    float len = std::hypot(d.x, d.y);
    if (len < 0.001f) return;
    sf::Vector2f n{-d.y / len * thickness * 0.5f, d.x / len * thickness * 0.5f};
    std::array<sf::Vertex, 6> v = {{{a + n, color}, {b + n, color}, {b - n, color},
                                    {a + n, color}, {b - n, color}, {a - n, color}}};
    target.draw(v.data(), v.size(), sf::PrimitiveType::Triangles);
}

TriangleDraw lineTriangles(sf::Vector2f a, sf::Vector2f b, float thickness, sf::Color color) {
    const sf::Vector2f d = b - a;
    const float len = std::hypot(d.x, d.y);
    if (len < .001f) return {};
    const sf::Vector2f n{-d.y / len * thickness * .5f, d.x / len * thickness * .5f};
    return {{{a+n,color},{b+n,color},{b-n,color},{a+n,color},{b-n,color},{a-n,color}}, nullptr};
}

bool isBig(uint32_t charId) {
    return charId == 1 || charId == 3 || charId == 8;
}

} // namespace

float Camera3D::depthOf(const Vec3& world) const {
    float cosYaw = std::cos(yaw), sinYaw = std::sin(yaw); // +yaw como no index.html (camera.js está desatualizado)
    float cosPitch = std::cos(-pitch), sinPitch = std::sin(-pitch);
    float dx = world.x - pos.x, dy = world.y - pos.y, dz = world.z - pos.z;
    float rz = dx * sinYaw + dz * cosYaw;
    return dy * sinPitch + rz * cosPitch;
}

Vec3 Camera3D::toLocal(const Vec3& world) const {
    float cosYaw = std::cos(yaw), sinYaw = std::sin(yaw); // +yaw como no index.html (camera.js está desatualizado)
    float cosPitch = std::cos(-pitch), sinPitch = std::sin(-pitch);
    float dx = world.x - pos.x, dy = world.y - pos.y, dz = world.z - pos.z;
    float rx = dx * cosYaw - dz * sinYaw;
    float rz = dx * sinYaw + dz * cosYaw;
    return {rx, dy * cosPitch - rz * sinPitch, dy * sinPitch + rz * cosPitch};
}

std::optional<ScreenPoint> Camera3D::projectLocal(const Vec3& local) const {
    if (local.z <= 10.0f) return std::nullopt;
    float scale = kFov / local.z;
    return ScreenPoint{{kLogicalWidth / 2.0f + local.x * scale + shake.x, kLogicalHeight / 2.0f - local.y * scale + shake.y},
                       scale, local.z};
}

std::optional<ScreenPoint> Camera3D::project(const Vec3& world) const {
    return projectLocal(toLocal(world));
}

RingRenderer::RingRenderer(HudFonts fonts, const SpriteLibrary* sprites) : fonts_(fonts), sprites_(sprites) {
    // 64 torcedores em 4 degraus intercalados, sem repetir visual na vizinhança (CrowdManager do JS).
    struct Tier { float dist, height, angleOffset; int skinStride; };
    const std::array<Tier, 4> tiers = {{{360.0f, -30.0f, 0.0f, 0}, {465.0f, 18.0f, kPi / 16.0f, 7},
                                        {565.0f, 68.0f, kPi / 32.0f, 13}, {665.0f, 118.0f, 3.0f * kPi / 32.0f, 2}}};
    for (int t = 0; t < 4; ++t) {
        for (int i = 0; i < 16; ++i) {
            float jitter = std::sin(i * 3.7f + t * 1.5f) * 0.025f;
            float angle = (i / 16.0f) * 2.0f * kPi + tiers[static_cast<std::size_t>(t)].angleOffset + jitter;
            float d = tiers[static_cast<std::size_t>(t)].dist;
            crowd_.push_back({{std::sin(angle) * d, tiers[static_cast<std::size_t>(t)].height, std::cos(angle) * d}, angle,
                              (i * 3 + tiers[static_cast<std::size_t>(t)].skinStride) % 19, std::fmod(i * 0.95f + t * 1.4f, 2.0f * kPi)});
        }
    }
}

std::optional<SpriteFrame> RingRenderer::sprite(const std::string& name) const {
    return sprites_ ? sprites_->find(name) : std::nullopt;
}

void RingRenderer::draw(sf::RenderTarget& target, const GameEngine& engine, bool paused, const ReplayView& replay, bool showResult) {
    using Clock = std::chrono::steady_clock;
    auto ms = [](Clock::time_point a, Clock::time_point b) { return std::chrono::duration<double, std::milli>(b - a).count(); };
    target.setActive(true);
    const auto tStart = Clock::now();
    const GameRenderState& state = engine.renderState;
    frameCounter_ = engine.frameIndex;
    engine_ = &engine;
    // A arena acompanha o adversário; no treino, acompanha o lutador escolhido (ou override se definido).
    const uint32_t hostChar = overrideTheme.has_value()
                                  ? static_cast<uint32_t>(*overrideTheme)
                                  : (engine.enemy.charId == 9 ? engine.player.charId : engine.enemy.charId);
    arenaTheme_ = hostChar == 0 ? ArenaTheme::JackGarden
                : hostChar == 1 ? ArenaTheme::FrogSwamp
                : hostChar == 2 ? ArenaTheme::JoanaHarbor
                : hostChar == 3 ? ArenaTheme::CremosoParlor
                : hostChar == 4 ? ArenaTheme::ValeriaPernambuco
                : hostChar == 5 ? ArenaTheme::VivaldiConcertHall
                : hostChar == 6 ? ArenaTheme::EspinhaCourtyard
                : hostChar == 7 ? ArenaTheme::MarcovObservatory : ArenaTheme::Central;
    cv::setClock(static_cast<float>(engine.frameIndex) * (1000.0f / 60.0f));

    if (state.screen_shake > 0.0f && !freeCam.enabled) {
        std::uniform_real_distribution<float> jitter(-1.0f, 1.0f);
        camera_.shake = {jitter(rng_) * state.screen_shake * 0.35f, jitter(rng_) * state.screen_shake * 0.35f};
    } else {
        camera_.shake = {0.0f, 0.0f};
    }

    const float ringAngle = state.ring_angle;
    const bool reverse = replay.phase == 2 || reverseCamera;
    reverseView_ = reverse;

    // Transição suave de zoom ao ir para as cordas / knockdown (zoom out) e voltar ao jogo (zoom in)
    const bool onRopes = engine.isMatchOver() ||
                         engine.gameState == STATE_KNOCKDOWN ||
                         engine.player.state == FIGHTER_KNOCKDOWN ||
                         engine.enemy.state == FIGHTER_KNOCKDOWN ||
                         (engine.player.radius > 200.0f && engine.player.state != FIGHTER_DODGE_BACK) ||
                         engine.enemy.radius > 200.0f;
    const float targetZoom = onRopes ? 1.0f : 0.0f;
    const float zoomSpeed = (targetZoom > ropeZoom_) ? 0.08f : 0.10f;
    ropeZoom_ += (targetZoom - ropeZoom_) * zoomSpeed;

    if (freeCam.enabled) {
        camera_.pos = freeCam.pos;
        camera_.yaw = freeCam.yaw;
        camera_.pitch = freeCam.pitch;
    } else {
        if (reverse) placeReverseCamera(engine.enemy, ringAngle);
        else {
            Fighter camFighter = engine.player;
            if (ropeZoom_ > 0.001f) {
                camFighter.radius = engine.player.idleRadius();
                camFighter.offsetX = 0.0f;
            }
            placeCamera(camFighter, ringAngle);
        }
        // Na derrota marítima, recuo discreto para enquadrar a queda na água sem
        // afastar o ringue nem transformar o barco do primeiro plano no foco.
        if (arenaTheme_ == ArenaTheme::JoanaHarbor && !reverse && engine.isMatchOver() &&
            (engine.result == MatchResult::LossKO || engine.result == MatchResult::LossTKO) &&
            engine.koRopeFrame != 0 && engine.frameIndex >= engine.koRopeFrame) {
            const float f = static_cast<float>(engine.frameIndex - engine.koRopeFrame);
            const float t = std::clamp((f - 19.0f) / 16.0f, 0.0f, 1.0f);
            const float ease = t * t * (3.0f - 2.0f * t);
            camera_.pos.x -= std::sin(ringAngle) * 140.0f * ease;
            camera_.pos.z -= std::cos(ringAngle) * 140.0f * ease;
            camera_.pos.y += (150.0f - camera_.pos.y) * ease;
            camera_.pitch += (0.59f - camera_.pitch) * ease;
        }
    }

    // Celebração (showVictoryRingCelebration): depois do replay o perdedor sai de cena e o campeão
    // comemora de frente para a torcida enquanto a câmera gira devagar em volta do ringue.
    const bool celebrate = engine.isMatchOver() && showResult && replay.phase < 0;
    if (celebrate && !celebrating_) {
        celebrating_ = true;
        celebFrames_ = 0;
        celebAngle_ = ringAngle;
        celebFrameIdx_ = engine.frameIndex;
    } else if (!celebrate) {
        celebrating_ = false;
    }
    Fighter champion;
    if (celebrating_) {
        while (celebFrameIdx_ < engine.frameIndex) { ++celebFrameIdx_; ++celebFrames_; celebAngle_ -= 0.0018f; }
        const MatchResult r = engine.result;
        const bool championIsPlayer = !(r == MatchResult::LossKO || r == MatchResult::LossTKO || r == MatchResult::LossDecision ||
                                        (r == MatchResult::Draw && engine.player.hp < engine.enemy.hp));
        champion = championIsPlayer ? engine.player : engine.enemy;
        const float a = celebAngle_, t = celebFrames_ / 60.0f * 3.0f; // Date.now() * 0.003
        const bool big = champion.isBigChar();
        const bool small = champion.isSmallChar();
        const float camR = big ? -330.0f : (small ? -215.0f : -245.0f), lateral = 35.0f;
        if (!freeCam.enabled) {
            camera_.pos = {camR * std::sin(a) + lateral * std::cos(a), big ? 250.0f : (small ? 185.0f : 205.0f), camR * std::cos(a) - lateral * std::sin(a)};
            camera_.pitch = 0.40f;
            camera_.yaw = a;
        }
        champion.state = FIGHTER_IDLE;
        champion.worldPos = {-45.0f * std::sin(a), 0.0f, -45.0f * std::cos(a)};
        // Dança: pulinhos, gingado de quadril e, a cada 4 s, um rodopio completo.
        const float cycle = std::fmod(t / 3.0f, 4.0f);
        const float twirl = cycle > 3.4f ? (cycle - 3.4f) / 0.6f : 0.0f;
        champion.facing = a + kPi + std::sin(t * 1.6f) * 0.35f + twirl * 2.0f * kPi;
        champion.offsetY = std::fabs(std::sin(t * 4.5f)) * 12.0f + std::sin(twirl * kPi) * 18.0f;
        champion.offsetX = std::sin(t * 1.5f) * 10.0f;
        champion.bodySkew = std::sin(t * 3.0f) * 0.08f;
        champion.gloveL.y = 92.0f + std::sin(t * 5.5f) * 18.0f;
        champion.gloveR.y = 92.0f + std::cos(t * 5.5f) * 18.0f;
        champion.gloveL.x = -22.0f + std::sin(t * 2.8f) * 5.0f;
        champion.gloveR.x = 22.0f - std::sin(t * 2.8f) * 5.0f;
        champion.gloveL.z = champion.gloveR.z = 20.0f;
    }

    // Posições e orientação vêm do motor (raio + ring angle, como no gameLoop do JS).
    Placement playerPlace{engine.player.worldPos, engine.player.facing};
    Placement enemyPlace{engine.enemy.worldPos, engine.enemy.facing};

    // Efeitos do Lab: um módulo por lutador, atualizado uma vez por quadro do motor (parado no replay).
    for (int slot = 0; slot < 2; ++slot) {
        const Fighter& f = slot == 0 ? engine.player : engine.enemy;
        if (labChar_[static_cast<std::size_t>(slot)] != f.charId) {
            lab_[static_cast<std::size_t>(slot)] = vfx::makeLabFx(f.charId);
            labChar_[static_cast<std::size_t>(slot)] = f.charId;
        }
    }
    labStep_ = replay.phase < 0 && engine.frameIndex != labFrame_;
    if (labStep_) {
        labFrame_ = engine.frameIndex;
        for (int slot = 0; slot < 2; ++slot) {
            if (!lab_[static_cast<std::size_t>(slot)]) continue;
            const Fighter& f = slot == 0 ? engine.player : engine.enemy;
            vfx::LabContext ctx = labContext(f, slot == 0);
            lab_[static_cast<std::size_t>(slot)]->update(ctx);
        }
    }

    queue_.clear();
    stepCrowdEffects(engine);
    if (!hideArena) {
        if (arenaTheme_ == ArenaTheme::JoanaHarbor) queueHarbor(engine);
    else {
        if (arenaTheme_ == ArenaTheme::Central) queueCentral(engine);
        if (arenaTheme_ == ArenaTheme::ValeriaPernambuco) queuePernambuco();
        if (arenaTheme_ == ArenaTheme::MarcovObservatory) queueObservatory();
        if (arenaTheme_ == ArenaTheme::FrogSwamp) queueSwamp();
        if (arenaTheme_ == ArenaTheme::VivaldiConcertHall) queueConcertHall();
        if (arenaTheme_ == ArenaTheme::CremosoParlor) queueParlor();
        if (arenaTheme_ == ArenaTheme::EspinhaCourtyard) queueEspinhaCourtyard();
        if (arenaTheme_ == ArenaTheme::JackGarden) queueJackGarden();
        queueStands();
        if (!hideCrowd) queueCrowd(engine);
    }
    }
    // Uma única película de tinta sobre o FUNDO, antes de desenhar piso,
    // lutadores, cordas e golpes. O recorte central mantém o oponente legível.
    const ArenaTheme printedTheme = arenaTheme_;
    const float printTime = static_cast<float>(frameCounter_) / 90.0f;
    if (!disableBackdropShader && !hideArena) {
        queue_.push_back({0.79f, 0.0f, [printedTheme, printTime](sf::RenderTarget& backdrop) {
            static std::unique_ptr<sf::Shader> shader = [] {
                if (!sf::Shader::isAvailable()) return std::unique_ptr<sf::Shader>{};
                auto candidate = std::make_unique<sf::Shader>();
                if (!candidate->loadFromMemory(kComicBackdropFragShader, sf::Shader::Type::Fragment))
                    return std::unique_ptr<sf::Shader>{};
                return candidate;
            }();
            if (!shader) return;
            sf::Glsl::Vec3 ink{0.10f, 0.10f, 0.16f};
            sf::Glsl::Vec3 light{0.87f, 0.76f, 0.56f};
            switch (printedTheme) {
            case ArenaTheme::JackGarden:          ink = {0.15f, 0.08f, 0.17f}; light = {1.00f, 0.72f, 0.38f}; break;
            case ArenaTheme::FrogSwamp:           ink = {0.05f, 0.16f, 0.15f}; light = {0.76f, 0.82f, 0.42f}; break;
            case ArenaTheme::JoanaHarbor:         ink = {0.04f, 0.16f, 0.23f}; light = {0.84f, 0.84f, 0.62f}; break;
            case ArenaTheme::CremosoParlor:       ink = {0.26f, 0.10f, 0.23f}; light = {1.00f, 0.78f, 0.62f}; break;
            case ArenaTheme::ValeriaPernambuco:   ink = {0.20f, 0.08f, 0.24f}; light = {1.00f, 0.72f, 0.35f}; break;
            case ArenaTheme::VivaldiConcertHall:  ink = {0.12f, 0.06f, 0.21f}; light = {0.93f, 0.76f, 0.52f}; break;
            case ArenaTheme::EspinhaCourtyard:    ink = {0.29f, 0.10f, 0.15f}; light = {0.98f, 0.73f, 0.43f}; break;
            case ArenaTheme::MarcovObservatory:   ink = {0.04f, 0.11f, 0.24f}; light = {0.48f, 0.86f, 0.94f}; break;
            case ArenaTheme::Central:             break;
            }
            shader->setUniform("u_resolution", sf::Vector2f(backdrop.getSize()));
            shader->setUniform("u_ink", ink);
            shader->setUniform("u_light", light);
            shader->setUniform("u_time", printTime);
            const sf::Color white = sf::Color::White;
            const std::array<sf::Vertex, 6> quad = {{
                {{0.0f, 0.0f}, white}, {{kLogicalWidth, 0.0f}, white}, {{kLogicalWidth, kLogicalHeight}, white},
                {{0.0f, 0.0f}, white}, {{kLogicalWidth, kLogicalHeight}, white}, {{0.0f, kLogicalHeight}, white},
            }};
            sf::RenderStates states;
            states.shader = shader.get();
            states.blendMode = sf::BlendAlpha;
            backdrop.draw(quad.data(), quad.size(), sf::PrimitiveType::Triangles, states);
        }});
    }
    if (!hideRing) {
        updateRopePhysics(engine);
        queueRing();
        queueImpactWord(engine);
    }
    // Na visão do oponente os papéis trocam: ele de costas para a câmera, o jogador de frente.
    const bool enemyDown = engine.enemy.state == FIGHTER_KNOCKDOWN, playerDown = engine.player.state == FIGHTER_KNOCKDOWN;
    // Nocaute decisivo: antes do replay o perdedor rodopia até as cordas e é arremessado.
    const MatchResult res = engine.result;
    const bool koEnd = engine.isMatchOver() && !celebrating_ &&
                       (res == MatchResult::WinKO || res == MatchResult::WinTKO || res == MatchResult::LossKO || res == MatchResult::LossTKO);
    if (!koEnd) { koFrames_ = -1; koRopeFrame_ = -1; koStar_ = {-1.0f, -1.0f}; koSparks_ = false; }
    if (koEnd && !celebrating_) {
        // Tudo em função do quadro do motor (igual ao vivo e no replay, que reproduz os mesmos estados).
        koFrames_ = static_cast<int>(engine.frameIndex - engine.matchEndFrame);
        koRopeFrame_ = (engine.koRopeFrame != 0 && engine.frameIndex >= engine.koRopeFrame) ? static_cast<int>(engine.koRopeFrame - engine.matchEndFrame) : -1;
        const bool enemyLoses = res == MatchResult::WinKO || res == MatchResult::WinTKO;
        Fighter loser = enemyLoses ? engine.enemy : engine.player;
        const Fighter& winner = enemyLoses ? engine.player : engine.enemy;
        const float k = static_cast<float>(koFrames_);
        if (koRopeFrame_ < 0) koSparks_ = false;
        if (koRopeFrame_ >= 0 && !koSparks_) {
            koSparks_ = true;
            // Faíscas do impacto nas cordas
            std::uniform_real_distribution<float> u(-1.0f, 1.0f);
            for (int i = 0; i < 18; ++i)
                flashes_.push_back({{loser.worldPos.x + u(rng_) * 30.0f, 70.0f + u(rng_) * 40.0f, loser.worldPos.z + u(rng_) * 30.0f}, 1.0f, 0.05f, 30.0f + u(rng_) * 10.0f});

            // Impulso elástico nas cordas do lado atingido
            int bestSide = 0;
            float maxDot = -999.0f;
            const float lLen = std::max(1.0f, std::hypot(loser.worldPos.x, loser.worldPos.z));
            const sf::Vector2f dir = {loser.worldPos.x / lLen, loser.worldPos.z / lLen};
            const std::array<sf::Vector2f, 4> sideNormals = {{{0.0f, -1.0f}, {1.0f, 0.0f}, {0.0f, 1.0f}, {-1.0f, 0.0f}}};
            for (int s = 0; s < 4; ++s) {
                float d = dir.x * sideNormals[s].x + dir.y * sideNormals[s].y;
                if (d > maxDot) { maxDot = d; bestSide = s; }
            }
            ropes_[bestSide].deflect = {15.0f, 22.0f, 28.0f};
            ropes_[bestSide].velocity = {-120.0f, -180.0f, -240.0f};
        }
        Vec3 pos = loser.worldPos;
        float facing = loser.facing;
        loser.state = FIGHTER_HIT;
        loser.offsetY = (koRopeFrame_ < 0) ? std::fabs(std::sin(k * 0.5f)) * 6.0f : 0.0f;
        bool visible = true;
        if (koRopeFrame_ >= 0) {
            const float f = k - static_cast<float>(koRopeFrame_);
            const float len = std::max(1.0f, std::hypot(pos.x, pos.z));
            const float dx = pos.x / len, dz = pos.z / len;
            const float harborImpact = enemyLoses ? 52.0f : 35.0f;
            const Vec3 harborLanding = arenaTheme_ == ArenaTheme::JoanaHarbor
                ? harborLandingPoint(loser.worldPos, static_cast<float>(engine.koRopeFrame) + harborImpact)
                : Vec3{};
            auto harborFlight = [&](float launchSpeed) {
                const float t = std::clamp(f / harborImpact, 0.0f, 1.0f);
                pos.x = loser.worldPos.x + (harborLanding.x - loser.worldPos.x) * t;
                pos.z = loser.worldPos.z + (harborLanding.z - loser.worldPos.z) * t;
                pos.y = launchSpeed * f + (harborLanding.y - launchSpeed * harborImpact) * t * t;
                visible = f < harborImpact;
            };
            if (enemyLoses) {
                // Oponente: salto arqueado que passa limpo por cima da corda superior (topo Y=98.0f),
                // pairando no ar diante da torcida antes de acelerar rumo aos refletores até virar estrelinha.
                if (arenaTheme_ == ArenaTheme::JoanaHarbor) {
                    harborFlight(17.5f);
                } else {
                    const float vault = 110.0f * std::sin(std::min(1.5707963f, f * 0.10f));
                    const float climb = (f > 15.0f) ? (f - 15.0f) * 4.8f + (f - 15.0f) * (f - 15.0f) * 0.08f : 0.0f;
                    pos = {pos.x + dx * f * 3.8f, vault + climb, pos.z + dz * f * 3.8f};
                    if (f > 55.0f) visible = false;
                    if (auto pr = camera_.project(pos)) koStar_ = {std::clamp(pr->pos.x, 30.0f, 770.0f), std::clamp(pr->pos.y, 30.0f, 570.0f)};
                }
            } else {
                // Jogador: ricocheteia nas cordas e é arremessado em arco parabólico alto por cima
                // da corda superior (ápice Y ~138.0f em f=16), caindo fora do ringue em direção à câmera.
                if (arenaTheme_ == ArenaTheme::JoanaHarbor) harborFlight(17.0f);
                else {
                    const float vaultY = 17.0f * f - 0.525f * f * f;
                    pos = {pos.x + dx * f * 3.2f, vaultY, pos.z + dz * f * 3.2f};
                    if (f > 55.0f) visible = false;
                }
            }
        }
        queueFighter(winner, {winner.worldPos, winner.facing}, false);
        // Cambaleia tonto até as cordas; o giro de ejeção começa só no quique.
        const float spinDir = loser.hitDir >= 0 ? 1.0f : -1.0f;
        const float preRopeWobble = 0.08f * std::sin((koRopeFrame_ < 0 ? k : static_cast<float>(koRopeFrame_)) * 0.3f);
        const float koRoll = spinDir * (preRopeWobble + (koRopeFrame_ < 0 ? 0.0f : (k - koRopeFrame_) * 0.42f));
        if (visible) queueFighter(loser, {pos, facing}, (!enemyLoses) != reverse, koRoll, true);
        if (arenaTheme_ == ArenaTheme::JoanaHarbor && koRopeFrame_ >= 0) {
            const float f = k - static_cast<float>(koRopeFrame_);
            const float impact = enemyLoses ? 52.0f : 35.0f;
            const float elapsed = f - impact;
            if (elapsed >= 0.0f && elapsed < 38.0f) {
                const Vec3 landing = harborLandingPoint(loser.worldPos, static_cast<float>(engine.koRopeFrame) + impact);
                if (auto projected = camera_.project(landing)) {
                    const sf::Vector2f center = projected->pos;
                    const float scale = std::clamp(projected->scale, 0.45f, 1.75f);
                    // Silhueta vetorial 2D como os gêiseres dos golpes. A camada fica
                    // acima do mar/barcos, mas abaixo do convés: nunca pinta a madeira.
                    queue_.push_back({0.72f, projected->depth, [center, scale, elapsed](sf::RenderTarget& target) {
                        cv::Canvas ctx(target);
                        ctx.save();
                        ctx.translate(center.x, center.y);
                        const float fade = 1.0f - elapsed / 38.0f;
                        const float radius = std::min((34.0f + elapsed * 2.7f) * scale, 68.0f);
                        const float burst = std::sin(std::min(elapsed / 22.0f, 1.0f) * kPi);
                        // Perto da câmera a coluna não pode sumir atrás do convés.
                        const float height = std::min(66.0f * scale * burst, 48.0f);
                        ctx.globalAlpha = fade;
                        ctx.fillStyle = sf::Color(120, 211, 209);
                        ctx.strokeStyle = sf::Color(19, 91, 105);
                        ctx.lineWidth = 3.0f * scale;
                        ctx.beginPath();
                        ctx.ellipse(0.0f, 0.0f, radius, radius * 0.24f, 0.0f, 0.0f, 2.0f * kPi);
                        ctx.fill(); ctx.stroke();
                        ctx.fillStyle = sf::Color(221, 252, 243);
                        ctx.beginPath();
                        ctx.ellipse(0.0f, -2.0f * scale, radius * 0.72f, radius * 0.12f,
                                    0.0f, 0.0f, 2.0f * kPi);
                        ctx.fill();
                        if (height > 2.0f) {
                            const float w = std::min(radius * 0.55f, height * 0.52f);
                            ctx.fillStyle = sf::Color(77, 178, 190);
                            ctx.strokeStyle = sf::Color(19, 91, 105);
                            ctx.lineWidth = 3.0f * scale;
                            ctx.beginPath();
                            ctx.moveTo(-w, -3.0f * scale);
                            ctx.bezierCurveTo(-w * 1.15f, -height * 0.32f, -w * 0.75f, -height * 0.70f,
                                              -w * 0.18f, -height);
                            ctx.quadraticCurveTo(0.0f, -height * 1.12f, w * 0.17f, -height * 0.94f);
                            ctx.bezierCurveTo(w * 0.72f, -height * 0.67f, w * 1.12f, -height * 0.31f,
                                              w, -3.0f * scale);
                            ctx.closePath(); ctx.fill(); ctx.stroke();
                            ctx.fillStyle = sf::Color(222, 252, 245);
                            ctx.beginPath();
                            ctx.moveTo(-w * 0.37f, -height * 0.12f);
                            ctx.quadraticCurveTo(-w * 0.57f, -height * 0.57f, -w * 0.05f, -height * 0.91f);
                            ctx.quadraticCurveTo(w * 0.16f, -height * 0.54f, w * 0.30f, -height * 0.13f);
                            ctx.closePath(); ctx.fill();
                            for (int i = 0; i < 7; ++i) {
                                const float side = i % 2 == 0 ? -1.0f : 1.0f;
                                const float spread = (0.45f + 0.12f * i) * radius;
                                const float x = side * spread;
                                const float y = -height * (0.42f + 0.08f * (i % 3));
                                const float drop = (4.0f + (i % 3) * 1.8f) * scale;
                                ctx.fillStyle = sf::Color(224, 251, 243);
                                ctx.strokeStyle = sf::Color(21, 101, 114);
                                ctx.lineWidth = 1.5f * scale;
                                ctx.beginPath();
                                ctx.ellipse(x, y, drop, drop * 1.35f, side * 0.24f, 0.0f, 2.0f * kPi);
                                ctx.fill(); ctx.stroke();
                            }
                        }
                        ctx.restore();
                    }});
                }
            }
        }
        if (koRopeFrame_ >= 0 && k - koRopeFrame_ > 40.0f && enemyLoses && koStar_.x >= 0.0f) {
            const sf::Vector2f c = koStar_;
            const float blink = 0.5f + 0.5f * std::sin(k * 0.6f);
            queue_.push_back({6.0f, 0.0f, [c, blink](sf::RenderTarget& t) {
                                  cv::Canvas ctx(t);
                                  ctx.globalCompositeOperation = "lighter";
                                  const float r = 10.0f + 8.0f * blink;
                                  ctx.beginPath();
                                  ctx.moveTo(c.x, c.y - r * 2); ctx.lineTo(c.x + r * 0.35f, c.y - r * 0.35f); ctx.lineTo(c.x + r * 2, c.y);
                                  ctx.lineTo(c.x + r * 0.35f, c.y + r * 0.35f); ctx.lineTo(c.x, c.y + r * 2); ctx.lineTo(c.x - r * 0.35f, c.y + r * 0.35f);
                                  ctx.lineTo(c.x - r * 2, c.y); ctx.lineTo(c.x - r * 0.35f, c.y - r * 0.35f);
                                  ctx.closePath();
                                  ctx.fillStyle = cv::rgba(255, 255, 220, 0.6f + 0.4f * blink);
                                  ctx.fill();
                              }});
        }
        queueParticles(state);
    } else if (celebrating_) {
        queueFighter(champion, {champion.worldPos, champion.facing}, false);
    } else {
        // No chão o oponente fica de barriga para cima (rosto de nocaute à mostra).
        if (!hideEnemy) queueFighter(engine.enemy, enemyPlace, reverse ? !enemyDown : false);
        if (!hidePlayer) queueFighter(engine.player, playerPlace, reverse ? playerDown : !playerDown);
        queueParticles(state);
        queueSkillEffects(engine);
    }

    // Inicialização do Shader de Hardware GLSL
    if (!shaderInitialized_) {
        shaderInitialized_ = true;
        if (sf::Shader::isAvailable()) {
            postFxShader_ = std::make_unique<sf::Shader>();
            if (!postFxShader_->loadFromMemory(kPostFxFragShader, sf::Shader::Type::Fragment)) {
                postFxShader_.reset();
            }
        }
    }

    // Inicialização / Redimensionamento do Render Target Intermediário da Cena
    const unsigned int rtW = static_cast<unsigned int>(kLogicalWidth);
    const unsigned int rtH = static_cast<unsigned int>(kLogicalHeight);
    if (!sceneRt_ || sceneRt_->getSize().x != rtW || sceneRt_->getSize().y != rtH) {
        sceneRt_ = std::make_unique<sf::RenderTexture>();
        sf::ContextSettings gl;
        gl.stencilBits = 8;
        if (!sceneRt_->resize({rtW, rtH}, gl)) {
            sceneRt_.reset();
        }
    }

    // Rastreamento e Propagação da Shockwave (Onda de Choque Radial + Contact Flash)
    if (engine.impactTimer > 0 && (engine.impactKind == 2 || engine.impactKind == 3 || engine.impactKind == 4)) {
        if (engine.impactSeed != lastImpactSeed_) {
            lastImpactSeed_ = engine.impactSeed;
            auto pr = camera_.project(engine.impactAt);
            if (pr) {
                shockCenter_ = {pr->pos.x / kLogicalWidth, pr->pos.y / kLogicalHeight};
                shockProgress_ = 0.0f;
                shockStrength_ = engine.impactKind == 4 ? 1.0f : (engine.impactKind == 2 ? 0.85f : 0.65f);
            }
        }
    }
    if (shockProgress_ < 1.0f) {
        shockProgress_ += 0.045f;
        if (shockProgress_ > 1.0f) shockProgress_ = 1.0f;
    }

    sf::RenderTarget& sceneTarget = sceneRt_ ? static_cast<sf::RenderTarget&>(*sceneRt_) : target;
    const sf::View mainView = target.getView();

    if (sceneRt_) {
        sceneRt_->clear(sf::Color::Transparent, sf::StencilValue{0u});
        sf::View sceneView(sf::FloatRect({0.0f, 0.0f}, {kLogicalWidth, kLogicalHeight}));
        sceneRt_->setView(sceneView);
    }

    // Horizonte das arenas, atrás da geometria 3D.
    if (!hideArena) {
        const bool harbor = arenaTheme_ == ArenaTheme::JoanaHarbor;
        const bool pernambuco = arenaTheme_ == ArenaTheme::ValeriaPernambuco;
        const bool observatory = arenaTheme_ == ArenaTheme::MarcovObservatory;
        const bool swamp = arenaTheme_ == ArenaTheme::FrogSwamp;
        const bool concert = arenaTheme_ == ArenaTheme::VivaldiConcertHall;
        const bool parlor = arenaTheme_ == ArenaTheme::CremosoParlor;
        const bool courtyard = arenaTheme_ == ArenaTheme::EspinhaCourtyard;
        const bool garden = arenaTheme_ == ArenaTheme::JackGarden;
        const sf::Color skyTop = harbor ? hex(0x68b9ce) : pernambuco ? hex(0x514b91)
                                     : observatory ? hex(0x090e2c) : swamp ? hex(0x173a35)
                                     : concert ? hex(0x26162f) : parlor ? hex(0x794961)
                                     : courtyard ? hex(0x55456b) : garden ? hex(0x3e355c) : hex(0x100a20);
        const sf::Color skyBottom = harbor ? hex(0x145c73) : pernambuco ? hex(0xf6a05e)
                                        : observatory ? hex(0x263b63) : swamp ? hex(0x79875b)
                                        : concert ? hex(0x513048) : parlor ? hex(0xf0b58c)
                                        : courtyard ? hex(0xd57c6a) : garden ? hex(0xf79a45) : hex(0x040508);
        std::array<sf::Vertex, 6> sky = {{
            {{0.0f, 0.0f}, skyTop}, {{kLogicalWidth, 0.0f}, skyTop}, {{kLogicalWidth, kLogicalHeight}, skyBottom},
            {{0.0f, 0.0f}, skyTop}, {{kLogicalWidth, kLogicalHeight}, skyBottom}, {{0.0f, kLogicalHeight}, skyBottom},
        }};
        // O shader colore somente a geometria já existente do céu. Se GLSL
        // não estiver disponível, o gradiente vetorial original permanece.
        static std::unique_ptr<sf::Shader> comicSkyShader = [] {
            if (!sf::Shader::isAvailable()) return std::unique_ptr<sf::Shader>{};
            auto shader = std::make_unique<sf::Shader>();
            if (!shader->loadFromMemory(kComicSkyFragShader, sf::Shader::Type::Fragment))
                return std::unique_ptr<sf::Shader>{};
            return shader;
        }();
        sf::RenderStates skyStates;
        skyStates.shader = comicSkyShader.get();
        sceneTarget.draw(sky.data(), sky.size(), sf::PrimitiveType::Triangles, skyStates);
        if (harbor) {
            const float day = std::fmod(static_cast<float>(frameCounter_) / 3600.0f, 1.0f);
            const float sunX = 95.0f + day * 610.0f;
            const float sunY = 92.0f - 45.0f * std::sin(day * kPi);
            sf::CircleShape glow(49.0f);
            glow.setPosition({sunX - 49.0f, sunY - 49.0f});
            glow.setFillColor(sf::Color(255, 224, 128, 45));
            sceneTarget.draw(glow);
            sf::CircleShape sun(29.0f);
            sun.setPosition({sunX - 29.0f, sunY - 29.0f});
            sun.setFillColor(hex(0xffdc82));
            sceneTarget.draw(sun);
            // As ilhas têm âncoras no mundo. A projeção acompanha a câmera quando
            // ela gira; só a silhueta e os coqueiros são desenhados em 2D.
            auto drawIsland = [&](const Vec3& world, float worldWidth, float worldRise, int palms) {
                const auto projected = camera_.project(world);
                if (!projected || projected->depth <= 10.0f) return;
                const float cx = projected->pos.x;
                const float width = worldWidth * projected->scale;
                const float rise = worldRise * projected->scale;
                if (cx + width < 0.0f || cx - width > kLogicalWidth) return;
                const float shore = projected->pos.y + 3.0f;
                queue_.push_back({0.16f, projected->depth, [cx, width, rise, palms, shore](sf::RenderTarget& target) {
                cv::Canvas islandArt(target);
                islandArt.save();
                islandArt.translate(cx, 0.0f);
                islandArt.fillStyle = sf::Color(184, 137, 65);
                islandArt.strokeStyle = sf::Color(41, 100, 101);
                islandArt.lineWidth = 2.5f;
                islandArt.beginPath();
                islandArt.moveTo(-width * 0.58f, shore);
                islandArt.quadraticCurveTo(-width * 0.43f, shore - rise * 0.78f,
                                           -width * 0.18f, shore - rise);
                islandArt.quadraticCurveTo(0.0f, shore - rise * 1.35f,
                                           width * 0.21f, shore - rise * 0.91f);
                islandArt.quadraticCurveTo(width * 0.47f, shore - rise * 0.58f,
                                           width * 0.58f, shore);
                islandArt.closePath(); islandArt.fill(); islandArt.stroke();
                islandArt.fillStyle = sf::Color(245, 207, 105);
                islandArt.beginPath();
                islandArt.moveTo(-width * 0.46f, shore - 3.0f);
                islandArt.quadraticCurveTo(-width * 0.22f, shore - rise * 1.01f,
                                           width * 0.05f, shore - rise * 1.04f);
                islandArt.quadraticCurveTo(width * 0.32f, shore - rise * 0.93f,
                                           width * 0.48f, shore - 3.0f);
                islandArt.closePath(); islandArt.fill();
                for (int p = 0; p < palms; ++p) {
                    const float rootX = palms == 1 ? -width * 0.04f : (p == 0 ? -width * 0.22f : width * 0.24f);
                    const float size = (p == 0 ? 35.0f : 27.0f) * (width / 112.0f);
                    const float lean = p == 0 ? -size * 0.15f : size * 0.20f;
                    const float rootY = shore - rise * 0.75f;
                    const float crownX = rootX + lean, crownY = rootY - size;
                    islandArt.strokeStyle = sf::Color(55, 75, 54);
                    islandArt.lineWidth = std::max(2.3f, size * 0.15f);
                    islandArt.beginPath();
                    islandArt.moveTo(rootX, rootY);
                    islandArt.quadraticCurveTo(rootX - lean * 0.22f, rootY - size * 0.54f,
                                               crownX, crownY);
                    islandArt.stroke();
                    islandArt.strokeStyle = sf::Color(161, 117, 57);
                    islandArt.lineWidth = std::max(1.3f, size * 0.065f);
                    islandArt.stroke();
                    for (int leaf = -2; leaf <= 2; ++leaf) {
                        const float tipX = crownX + leaf * size * 0.37f;
                        const float tipY = crownY - size * 0.13f + std::fabs(static_cast<float>(leaf)) * size * 0.22f;
                        islandArt.fillStyle = sf::Color(38, 111, 85);
                        islandArt.strokeStyle = sf::Color(28, 83, 71);
                        islandArt.lineWidth = 1.2f;
                        islandArt.beginPath();
                        islandArt.moveTo(crownX, crownY);
                        islandArt.quadraticCurveTo(crownX + leaf * size * 0.23f,
                                                   crownY - size * 0.34f, tipX, tipY);
                        islandArt.quadraticCurveTo(crownX + leaf * size * 0.18f,
                                                   crownY - size * 0.06f, crownX, crownY);
                        islandArt.closePath(); islandArt.fill(); islandArt.stroke();
                    }
                }
                islandArt.restore();
                }});
            };
            drawIsland({-2250.0f, -43.0f, 4300.0f}, 1180.0f, 170.0f, 2);
            drawIsland({  720.0f, -43.0f, 5000.0f},  770.0f, 140.0f, 1);
            drawIsland({ 3350.0f, -43.0f, 4500.0f}, 1350.0f, 180.0f, 2);
            drawIsland({ 3200.0f, -43.0f,-4300.0f}, 1000.0f, 160.0f, 1);
            drawIsland({ -380.0f, -43.0f,-5200.0f}, 1300.0f, 190.0f, 2);
            drawIsland({-2900.0f, -43.0f,-4600.0f},  800.0f, 130.0f, 1);
            drawIsland({ 4700.0f, -43.0f,  700.0f},  900.0f, 150.0f, 1);
            drawIsland({-4800.0f, -43.0f, -450.0f}, 1100.0f, 170.0f, 2);
            // A linha d'água oscila: não há corte horizontal imóvel entre céu e mar.
            std::vector<sf::Vertex> seaEdge;
            seaEdge.reserve(80 * 12);
            for (int x = 0; x < static_cast<int>(kLogicalWidth); x += 10) {
                auto crest = [this](float px) {
                    return 160.0f + 3.5f * std::sin(px * 0.030f + frameCounter_ * 0.020f)
                                  + 1.8f * std::sin(px * 0.077f - frameCounter_ * 0.032f);
                };
                const float xa = static_cast<float>(x), xb = xa + 10.0f;
                const float ya = crest(xa), yb = crest(xb);
                const sf::Color water = hex(0x176b7d);
                const sf::Color glint(215, 245, 226, 66);
                for (const auto& p : std::array<sf::Vector2f, 6>{{{xa, ya}, {xb, yb}, {xb, kLogicalHeight},
                                                                    {xa, ya}, {xb, kLogicalHeight}, {xa, kLogicalHeight}}})
                    seaEdge.push_back({p, water});
                for (const auto& p : std::array<sf::Vector2f, 6>{{{xa, ya}, {xb, yb}, {xb, yb + 2.0f},
                                                                    {xa, ya}, {xb, yb + 2.0f}, {xa, ya + 2.0f}}})
                    seaEdge.push_back({p, glint});
            }
            sceneTarget.draw(seaEdge.data(), seaEdge.size(), sf::PrimitiveType::Triangles);
        }
        // O céu e as superfícies já carregam a iluminação de cada arena.
        // Os antigos dois fachos fixos cruzavam o centro e competiam com os lutadores.
    }

    std::stable_sort(queue_.begin(), queue_.end(), [](const DrawItem& a, const DrawItem& b) {
        if (a.layer != b.layer) return a.layer < b.layer;
        return a.depth > b.depth; // mais longe primeiro
    });
    glFinishHack(sceneTarget);
    const auto tBuilt = Clock::now();
    // Never merge across a callback: it can change view, blend, shader or
    // scissor state. Texture switches also flush without reordering geometry.
    TriangleDraw pending;
    std::size_t submitted = 0, merged = 0;
    auto flush = [&] {
        if (pending.vertices.empty()) return;
        pending(sceneTarget);
        pending.vertices.clear();
        ++submitted;
    };
    const bool noBatch = std::getenv("KORAMA_DISABLE_RING_BATCH") != nullptr;
    for (const DrawItem& item : queue_) {
        const auto* triangles = noBatch ? nullptr : item.draw.target<TriangleDraw>();
        if (!triangles) { flush(); item.draw(sceneTarget); ++submitted; continue; }
        if (!pending.vertices.empty() && pending.texture != triangles->texture) flush();
        pending.texture = triangles->texture;
        pending.vertices.insert(pending.vertices.end(), triangles->vertices.begin(), triangles->vertices.end());
        ++merged;
    }
    flush();
    if (std::getenv("KORAMA_RENDER_STATS") && frameCounter_ % 120 == 0)
        std::fprintf(stderr,"Render queue: %zu items, %zu submissions, %zu batchable\n",queue_.size(),submitted,merged);
    glFinishHack(sceneTarget);
    const auto tQueue = Clock::now();

    // Se renderizou no sceneRt_, transfere para a target principal aplicando os shaders GLSL
    if (sceneRt_) {
        sceneRt_->display();
        target.setView(mainView);

        sf::Sprite sceneSprite(sceneRt_->getTexture());
        sceneSprite.setPosition({0.0f, 0.0f});
        sceneSprite.setScale({1.0f, 1.0f});

        // Aberração cromática dinâmica proporcional ao tremor e impacto
        float chroma = (state.screen_shake > 0.0f) ? std::min(0.022f, state.screen_shake * 0.0009f) : 0.0f;
        if (engine.gameState == STATE_KNOCKDOWN) chroma = std::max(chroma, 0.008f);
        const bool needShader = postFxShader_ && (shockProgress_ < 1.0f || chroma > 0.0005f);

        if (needShader) {
            postFxShader_->setUniform("u_texture", sf::Shader::CurrentTexture);
            postFxShader_->setUniform("u_resolution", sf::Vector2f(kLogicalWidth, kLogicalHeight));
            postFxShader_->setUniform("u_shockCenter", shockCenter_);
            postFxShader_->setUniform("u_shockProgress", shockProgress_);
            postFxShader_->setUniform("u_shockStrength", shockStrength_);
            postFxShader_->setUniform("u_chromaOffset", chroma);
            postFxShader_->setUniform("u_halftoneEnabled", 0.0f); // desligado para eliminar sobrecarga de fillrate em GPUs integradas
            postFxShader_->setUniform("u_halftoneScale", 5.0f);

            sf::RenderStates shaderStates;
            shaderStates.shader = postFxShader_.get();
            shaderStates.blendMode = sf::BlendAlpha;
            target.draw(sceneSprite, shaderStates);
        } else {
            target.draw(sceneSprite);
        }
    }

    // Acabamento do canvas original (renderFrame): brilho do holofote central, flash de impacto,
    // scanlines CRT a cada 3 px e vinheta nos cantos, no espaço 1600x900.
    {
        const sf::View fightView = target.getView();
        sf::View uiView(sf::FloatRect({0.0f, 0.0f}, {1600.0f, 900.0f}));
        uiView.setViewport(fightView.getViewport());
        target.setView(uiView);
        if (!postFx_) {
            // Pré-desenhado em textura (alfa pré-multiplicado): antes eram dois degradês de tela cheia e
            // 300 faixas por quadro.
            postFx_ = std::make_unique<sf::RenderTexture>();
            sf::ContextSettings gl;
            gl.stencilBits = 8;
            if (postFx_->resize({1600u, 900u}, gl)) {
                postFx_->clear(sf::Color::Transparent, sf::StencilValue{0u});
                sf::RenderTexture& rt = *postFx_;
                {
                    cv::Canvas ctx(rt);
                    auto glow = ctx.createRadialGradient(800, 900 * 0.58f, 40, 800, 900 * 0.58f, 900 * 0.55f);
                    glow->addColorStop(0, "rgba(255, 230, 150, 0.08)");
                    glow->addColorStop(0.6f, "rgba(255, 200, 100, 0.03)");
                    glow->addColorStop(1, "rgba(0, 0, 0, 0)");
                    ctx.fillStyle = glow;
                    ctx.fillRect(0, 0, 1600, 900);
                }
                sf::VertexArray lines(sf::PrimitiveType::Triangles);
                const sf::Color scan(0, 0, 0, 15);
                for (float y = 0.0f; y < 900.0f; y += 3.0f) {
                    lines.append({{0.0f, y}, scan}); lines.append({{1600.0f, y}, scan}); lines.append({{1600.0f, y + 1.0f}, scan});
                    lines.append({{0.0f, y}, scan}); lines.append({{1600.0f, y + 1.0f}, scan}); lines.append({{0.0f, y + 1.0f}, scan});
                }
                rt.draw(lines);
                {
                    cv::Canvas ctx(rt);
                    auto vig = ctx.createRadialGradient(800, 450, 1600 * 0.35f, 800, 450, 1600 * 0.75f);
                    vig->addColorStop(0, "rgba(0,0,0,0)");
                    vig->addColorStop(1, "rgba(0,0,0,0.45)");
                    ctx.fillStyle = vig;
                    ctx.fillRect(0, 0, 1600, 900);
                }
                rt.display();
            }
        }
        if (postFx_ && !postFxHud_ && combineHudVignette_ && !paused && !hideHud && !celebrating_ && replay.phase < 0) {
            auto combined = std::make_unique<sf::RenderTexture>();
            sf::ContextSettings gl;
            gl.stencilBits = 8;
            if (combined->resize({1600u, 900u}, gl)) {
                combined->clear(sf::Color::Transparent, sf::StencilValue{0u});
                sf::Sprite base(postFx_->getTexture());
                sf::RenderStates premul;
                premul.blendMode = sf::BlendMode(sf::BlendMode::Factor::One, sf::BlendMode::Factor::OneMinusSrcAlpha);
                combined->draw(base, premul);
                drawHudVignette(*combined);
                combined->display();
                postFxHud_ = std::move(combined);
            }
        }
        if (postFx_ && !hidePostFx) {
            const bool hudVignette = combineHudVignette_ && postFxHud_ && !paused && !hideHud &&
                                     !celebrating_ && replay.phase < 0;
            sf::Sprite overlay((hudVignette ? postFxHud_ : postFx_)->getTexture());
            sf::RenderStates st;
            st.blendMode = sf::BlendMode(sf::BlendMode::Factor::One, sf::BlendMode::Factor::OneMinusSrcAlpha);
            target.draw(overlay, st);
        }
        if (state.screen_shake > 7.0f && replay.phase < 0) {
            sf::RectangleShape flash({1600.0f, 900.0f});
            flash.setFillColor(sf::Color(255, 255, 240, 56));
            target.draw(flash);
        }
        target.setView(fightView);
    }

    if (replay.phase >= 0) {
        drawReplayOverlay(target, replay);
        return;
    }
    glFinishHack(target);
    const auto tPost = Clock::now();
    if (celebrating_) {
        drawVictoryCard(target, engine);
    } else {
        drawFloatingTexts(target, state);
        if (!hideHud) drawHud(target, engine, paused, showResult);
    }
    if (fightFadeFrames_ > 0) {
        const float alpha = static_cast<float>(fightFadeFrames_) / static_cast<float>(fightFadeMax_);
        --fightFadeFrames_;
        sf::View currentView = target.getView();
        target.setView(sf::View(sf::FloatRect({0.0f, 0.0f}, {1600.0f, 900.0f})));
        sf::RectangleShape fadeRect(sf::Vector2f(1600.0f, 900.0f));
        fadeRect.setFillColor(sf::Color(255, 255, 255, static_cast<uint8_t>(std::clamp(alpha * 255.0f, 0.0f, 255.0f))));
        target.draw(fadeRect);
        target.setView(currentView);
    }
    glFinishHack(target);
    const auto tEnd = Clock::now();
    profile.build += ms(tStart, tBuilt); profile.queue += ms(tBuilt, tQueue); profile.post += ms(tQueue, tPost); profile.hud += ms(tPost, tEnd);
    profile.queueItems += queue_.size();
    profile.frames++;
}

void RingRenderer::drawVictoryCard(sf::RenderTarget& target, const GameEngine& engine) {
    const MatchResult r = engine.result;
    const bool won = r == MatchResult::WinKO || r == MatchResult::WinTKO || r == MatchResult::WinDecision;
    const bool perfect = engine.isPerfectVictory();
    celebration::VictoryCard card;
    card.won = won;
    card.perfect = perfect;
    card.draw = r == MatchResult::Draw;
    card.combo = engine.bestCombo;
    if (won) {
        const bool champion = engine.isArcadeMode && engine.arcadeStage >= 4;
        if (perfect) {
            card.ribbon = champion ? "CAMPEAO (100% VIDA)" : "PERFEITO (100% VIDA)";
            card.headline = "PERFEITO!";
            card.title = champion ? "CAMPEÃO SUPREMO (INVENCÍVEL!)"
                                  : (engine.isArcadeMode ? "PERFEITO NO STAGE " + std::to_string(engine.arcadeStage) + "/4" : "VITÓRIA PERFEITA (FLAWLESS!)");
            if (engine.enemy.charId == 8) card.title = "BOSS FINAL DESTRUÍDO SEM LEVAR DANO!";
            card.badge = "+1500 XP (PERFEITO)";
            card.description = "LUTA IMPECÁVEL! Você dominou o ringue sem levar nenhum soco. A torcida enlouquece com a perfeição técnica!";
        } else {
            card.ribbon = champion ? "CAMPEÃO ABSOLUTO" : "VITÓRIA NO ROUND";
            card.headline = champion ? "CAMPEÃO!" : (r == MatchResult::WinKO ? "K.O.!" : "VITÓRIA!");
            card.title = champion ? "CAMPEÃO SUPREMO DO TORNEIO"
                                  : (engine.isArcadeMode ? "VITÓRIA NO STAGE " + std::to_string(engine.arcadeStage) + "/4" : "VITÓRIA ESPETACULAR");
            if (engine.enemy.charId == 8) card.title = r == MatchResult::WinKO ? "BOSS FINAL NOCAUTEADO!" : "BOSS FINAL DESTRUÍDO!";
            card.badge = "+500 XP DE COMBATE";
            card.description = "O lutador recebe a ovação calorosa da torcida e uma gloriosa chuva de rosas no centro do ringue!";
            if (r == MatchResult::WinDecision) card.description = "Vitória nos pontos depois dos 3 assaltos! A torcida joga rosas no ringue.";
        }
        if (engine.isArcadeMode) card.buttons = {{"ENTER", champion ? "VER O CHAVEAMENTO" : "AVANÇAR NO TORNEIO"}, {"ESC", "MENU"}};
        else card.buttons = {{"ENTER", "JOGAR NOVAMENTE"}, {"ESC", "SELEÇÃO DE MODO"}};
    } else if (r == MatchResult::Draw) {
        card.ribbon = "FIM DOS 3 ASSALTOS";
        card.headline = "EMPATE!";
        card.title = "EMPATE TÉCNICO";
        card.badge = "+100 XP DE COMBATE";
        card.description = "Ninguém caiu! Os juízes não conseguiram separar os dois lutadores.";
        card.buttons = {{"ENTER", "REVANCHE"}, {"ESC", "SELEÇÃO DE MODO"}};
    } else {
        card.ribbon = "DERROTA NO COMBATE";
        card.headline = "DERROTA";
        card.title = r == MatchResult::LossDecision ? "DERROTA NOS PONTOS" : "NOCAUTEADO NA LONA";
        card.badge = "0 XP GANHO";
        card.description = r == MatchResult::LossDecision ? "Os juízes deram a luta ao oponente, que comemora no ringue sob aplausos!"
                                                           : "Você caiu na contagem do árbitro enquanto o oponente comemora no ringue sob aplausos!";
        if (engine.isArcadeMode) card.buttons = {{"ENTER", "NOVO TORNEIO"}, {"ESC", "MENU PRINCIPAL"}};
        else card.buttons = {{"ENTER", "TENTAR NOVAMENTE"}, {"ESC", "SELEÇÃO DE MODO"}};
    }

    // O card vive no espaço de interface 1600x900, na mesma área da vista da luta.
    const sf::View fightView = target.getView();
    sf::View ui(sf::FloatRect({0.0f, 0.0f}, {1600.0f, 900.0f}));
    ui.setViewport(fightView.getViewport());
    target.setView(ui);
    {
        cv::Canvas ctx(target);
        resultButtons_.clear();
        celebration::drawVictoryCard(ctx, card, std::min(1.0f, celebFrames_ / 24.0f), celebFrames_ / 60.0f, &resultButtons_, uiMouse);
    }
    target.setView(fightView);
}

void RingRenderer::resetEffects() {
    lab_[0].reset();
    lab_[1].reset();
    labChar_ = {{99u, 99u}};
    flashes_.clear();
    throwables_.clear();
    mudPuddleActive_ = {false, false};
    joanaCoralCrackActive_ = false;
    ropes_ = {};
    lastRopeFrame_ = 0;
    ropeZoom_ = 0.0f;
    fightFadeFrames_ = 0;
    cameraInitialized_ = false;
}

namespace {

// Nome do estado no vocabulário do JS (os efeitos do Lab comparam por texto).
std::string labStateName(const Fighter& f) {
    switch (f.state) {
    case FIGHTER_IDLE: return "idle";
    case FIGHTER_JAB: return "jab";
    case FIGHTER_HOOK: return "hook";
    case FIGHTER_BODY_BLOW: return "body_blow";
    case FIGHTER_UPPER: return "upper";
    case FIGHTER_BLOCK: return "block";
    case FIGHTER_DODGE_L: return "dodgeL";
    case FIGHTER_DODGE_R: return "dodgeR";
    case FIGHTER_DODGE_BACK: return "dodgeBack";
    case FIGHTER_DUCK: return "duck";
    case FIGHTER_TELEGRAPH: return "telegraph";
    case FIGHTER_HIT: return "hit";
    case FIGHTER_KNOCKDOWN: return "knockdown";
    case FIGHTER_VICTORY_POSE: return "victory";
    case FIGHTER_CROSS: return "cross";
    case FIGHTER_SKILL:
        if (const SkillDef* def = SkillSystem::get(f.activeSkill)) return def->key;
        return "idle";
    }
    return "idle";
}

} // namespace

vfx::LabContext RingRenderer::labContext(const Fighter& f, bool isPlayerSlot) const {
    vfx::LabContext c;
    c.camera = &camera_;
    c.entity = &f;
    if (engine_) c.rival = isPlayerSlot ? &engine_->enemy : &engine_->player;
    c.frame = frameCounter_;
    if((f.charId==3 || f.charId==5) && engine_)c.frame=engine_->frameIndex;
    c.sprites = sprites_;
    // Medidas do corpo de cada lutador (as mesmas do queueFighter / drawEntityCharacter do JS).
    const bool pl = f.isPlayer;
    switch (f.charId) {
    case 0: c.bodyW = 110; c.bodyH = 110; c.bodyLift = 34; break;
    case 1: c.bodyW = pl ? 115.0f : 140.0f; c.bodyH = pl ? 140.0f : 170.0f; c.bodyLift = -10; break;
    case 2: c.bodyW = (pl ? 115.0f : 135.0f) * (f.monstrous ? 1.15f : 1.0f); c.bodyH = (pl ? 120.0f : 145.0f) * (f.monstrous ? 1.32f : 1.0f);
            c.bodyLift = f.monstrous ? 26.0f : 22.0f; break;
    case 3: c.bodyW = pl ? 140.0f : 164.0f; c.bodyH = pl ? 144.0f : 172.0f; c.bodyLift = 18.0f; break;
    case 4: c.bodyW = pl ? 115.0f : 135.0f; c.bodyH = pl ? 120.0f : 145.0f; c.bodyLift = 22; break;
    case 5: c.bodyW = pl ? 122.0f : 142.0f; c.bodyH = pl ? 132.0f : 158.0f; c.bodyLift = 20; break;
    case 6: c.bodyW = pl ? 120.0f : 140.0f; c.bodyH = pl ? 130.0f : 155.0f; c.bodyLift = 20; break;
    case 7: c.bodyW = pl ? 120.0f : 142.0f; c.bodyH = pl ? 130.0f : 158.0f; c.bodyLift = 18; break;
    default: c.bodyW = f.bossStage == 3 ? 135.0f : 210.0f; c.bodyH = f.bossStage == 3 ? 165.0f : 235.0f; c.bodyLift = f.bossStage == 3 ? 10.0f : 25.0f; break;
    }
    c.fighter.state = labStateName(f);
    if ((f.charId >= 1 && f.charId <= 7) && f.signatureOnly && f.state == FIGHTER_SKILL) c.fighter.state = "victory";
    c.fighter.actionTimer = f.timer;
    c.fighter.maxActionTimer = f.maxTimer;
    c.fighter.isLeft = f.isLeftPunch;
    c.fighter.parts.gloveL = {f.gloveL.x, f.gloveL.y, f.gloveL.z};
    c.fighter.parts.gloveR = {f.gloveR.x, f.gloveR.y, f.gloveR.z};
    c.fighter.body.offsetX = f.offsetX;
    c.fighter.body.offsetY = f.offsetY;
    c.fighter.body.bodySkew = f.bodySkew;
    if(f.charId==5 && f.skinId==1 && !celebrating_ && (f.state==FIGHTER_IDLE || f.state==FIGHTER_BLOCK))
        c.fighter.body.offsetX=c.fighter.body.offsetY=c.fighter.body.bodySkew=0;
    if (joanaCrushing(f)) {
        const Fighter visual=joanaVisualPose(f);
        c.fighter.parts.gloveR={visual.gloveR.x,visual.gloveR.y,visual.gloveR.z};
        c.fighter.body.offsetX=visual.offsetX;c.fighter.body.offsetY=visual.offsetY;c.fighter.body.bodySkew=visual.bodySkew;
    }
    const bool down = f.state == FIGHTER_KNOCKDOWN;
    c.activeCamera = (f.isPlayer ? !down : down) ? "back" : "front";
    c.liveBody = liveVisual || hasIntegratedSkin(f.charId);
    c.isPaused = !labStep_;
    return c;
}

void RingRenderer::queueLabLayers(const Fighter& f, bool backView, float bodyLayer, const Vec3& bodyWorld, float bodyW, float bodyH) {
    const std::size_t slot = f.isPlayer ? 0 : 1;
    if (!lab_[slot]) return;
    const float depth = camera_.depthOf(bodyWorld);
    auto push = [&](float layer, float d, int kind) {
        // Visual novo: a Joana monstruosa já vem desenhada nas peças; a mutação antiga por cima duplicaria.
        if (kind == 2 && f.charId == 2) return;
        queue_.push_back({layer, d, [this, slot, kind, backView, bodyW, bodyH](sf::RenderTarget& t) {
                              if (!engine_ || !lab_[slot]) return;
                              const Fighter& fighter = slot == 0 ? engine_->player : engine_->enemy;
                              vfx::LabContext ctx = labContext(fighter, slot == 0);
                              ctx.activeCamera = backView ? "back" : "front";
                              cv::Canvas canvas(t);
                              if (kind == 0) lab_[slot]->drawFloor(canvas, ctx);
                              else if (kind == 1) lab_[slot]->drawBackground(canvas, ctx);
                              else if (kind == 2) lab_[slot]->drawBodyOverlay(canvas, ctx, bodyW, bodyH);
                              else lab_[slot]->drawForeground(canvas, ctx);
                          }});
    };
    push(1.5f, depth - 50.0f, 0);
    push(bodyLayer - 0.25f, depth + 5.0f, 1);
    push(bodyLayer + 0.05f, depth, 2);
    push(bodyLayer + 0.25f, depth - 10.0f, 3);
}

void RingRenderer::placeReverseCamera(const Fighter& enemy, float ringAngle) {
    // Sem o rival na tela (prévia da build), a câmera chega mais perto para enquadrar o lutador de frente.
    const float reverse = ringAngle + kPi;
    const bool big = enemy.isBigChar();
    const bool small = enemy.isSmallChar();
    const float camRadius = hideEnemy ? (small ? -80.0f : (big ? -120.0f : -95.0f))
                                      : (-260.0f + (big ? -65.0f : (small ? 20.0f : 0.0f)));
    const float camY = hideEnemy ? (small ? 160.0f : (big ? 195.0f : 175.0f))
                                 : (big ? 245.0f : (small ? 205.0f : 220.0f));
    const float pitch = hideEnemy ? (small ? 0.28f : (big ? 0.36f : 0.32f))
                                  : (big ? 0.48f : (small ? 0.43f : 0.45f));
    const float lateral = hideEnemy ? 0.0f : enemy.offsetX * 0.6f;
    camera_.pos = {camRadius * std::sin(reverse) + lateral * std::cos(reverse), camY,
                   camRadius * std::cos(reverse) - lateral * std::sin(reverse)};
    camera_.pitch = pitch;
    camera_.yaw = reverse;
    cameraInitialized_ = false;
}

void RingRenderer::drawReplayOverlay(sf::RenderTarget& target, const ReplayView& replay) {
    overlayTick_++;
    const float t = static_cast<float>(overlayTick_);
    const float fx = 18.0f, fy = 18.0f, fw = kLogicalWidth - 36.0f, fh = kLogicalHeight - 36.0f;
    auto rect = [&](float x, float y, float w, float h, sf::Color c) {
        sf::RectangleShape r({w, h});
        r.setPosition({x, y});
        r.setFillColor(c);
        target.draw(r);
    };
    auto scanlines = [&](std::uint8_t alpha, float thickness) {
        std::vector<sf::Vertex> v;
        for (float y = 0.0f; y < kLogicalHeight; y += 3.0f) {
            sf::Color c(0, 0, 0, alpha);
            v.push_back({{0.0f, y}, c}); v.push_back({{kLogicalWidth, y}, c}); v.push_back({{kLogicalWidth, y + thickness}, c});
            v.push_back({{0.0f, y}, c}); v.push_back({{kLogicalWidth, y + thickness}, c}); v.push_back({{0.0f, y + thickness}, c});
        }
        target.draw(v.data(), v.size(), sf::PrimitiveType::Triangles);
    };
    std::uniform_real_distribution<float> u(0.0f, 1.0f);

    if (replay.phase == 0) {
        // Rebobinando a fita: tela escura, faixas de tracking, scanlines e as setas ◀◀.
        rect(0, 0, kLogicalWidth, kLogicalHeight, sf::Color(10, 10, 10, 115));
        for (int i = 0; i < 8; ++i) {
            float bandY = std::fmod(t * 30.0f + i * 85.0f, kLogicalHeight + 40.0f) - 20.0f;
            rect(0, bandY, kLogicalWidth, 6.0f + (i % 4) * 5.0f, sf::Color(255, 255, 255, 46));
        }
        scanlines(90, 1.5f);
        rect((u(rng_) - 0.5f) * 50.0f, std::fmod(t * 30.0f, kLogicalHeight), kLogicalWidth, 14.0f, sf::Color(255, 255, 255, 64));
        const float cx = kLogicalWidth / 2.0f, cy = kLogicalHeight / 2.0f, a = 48.0f;
        const std::uint8_t alpha = static_cast<std::uint8_t>(255 * (0.26f + std::sin(t * 0.25f) * 0.08f));
        for (float tip : {cx - 12.0f - a * 1.15f, cx + a * 1.05f - a * 1.15f}) {
            sf::ConvexShape arrow(3);
            arrow.setPoint(0, {tip + a * 1.15f, cy - a});
            arrow.setPoint(1, {tip, cy});
            arrow.setPoint(2, {tip + a * 1.15f, cy + a});
            arrow.setFillColor(sf::Color(235, 25, 55, alpha));
            arrow.setOutlineColor(sf::Color(255, 60, 90, static_cast<std::uint8_t>(std::min(255, alpha + 38))));
            arrow.setOutlineThickness(3.5f);
            target.draw(arrow);
        }
        const bool blink = (overlayTick_ / 8) % 2 == 0;
        drawText(target, "<< REW 16X", {fx + 16.0f, fy + 12.0f}, 13u, blink ? sf::Color(255, 255, 255, 230) : sf::Color(200, 200, 200, 128), 0.0f, fonts_.digits);
        drawText(target, "TAPE 01  SP -0:02:45", {fx + fw - 16.0f, fy + 14.0f}, 11u, sf::Color(255, 255, 255, 190), 1.0f, fonts_.digits);
    } else {
        // Reprodução em câmera lenta com moldura de TV, REC piscando e OSD do videocassete.
        sf::RectangleShape frame({fw, fh});
        frame.setPosition({fx, fy});
        frame.setFillColor(sf::Color::Transparent);
        frame.setOutlineColor(sf::Color(255, 50, 80, 190));
        frame.setOutlineThickness(4.0f);
        target.draw(frame);
        const float b = 22.0f;
        const sf::Color gold = hex(0xffd700);
        for (auto [x, y, dx, dy] : std::array<std::array<float, 4>, 4>{{{fx, fy, 1, 1}, {fx + fw, fy, -1, 1}, {fx, fy + fh, 1, -1}, {fx + fw, fy + fh, -1, -1}}}) {
            drawThickLine(target, {x, y}, {x + dx * b, y}, 3.0f, gold);
            drawThickLine(target, {x, y}, {x, y + dy * b}, 3.0f, gold);
        }
        const bool mine = replay.phase == 1;
        const std::string badge = mine ? "REPLAY (SUA VISÃO)" : "REPLAY (VISÃO DO INIMIGO)";
        const sf::Color badgeColor = mine ? gold : hex(0xff3355);
        float textW = static_cast<float>(badge.size()) * 10.0f;
        if (fonts_.digits) textW = sf::Text(*fonts_.digits, sf::String::fromUtf8(badge.begin(), badge.end()), 11u).getLocalBounds().size.x;
        const float bw = textW + 38.0f, bx = fx + fw - bw - 12.0f, by = fy + 12.0f;
        rect(bx, by, bw, 32.0f, sf::Color(0, 0, 0, 216));
        sf::RectangleShape edge({bw, 32.0f});
        edge.setPosition({bx, by});
        edge.setFillColor(sf::Color::Transparent);
        edge.setOutlineColor(badgeColor);
        edge.setOutlineThickness(2.0f);
        target.draw(edge);
        sf::ConvexShape play(3);
        play.setPoint(0, {bx + 10.0f, by + 10.0f}); play.setPoint(1, {bx + 20.0f, by + 16.0f}); play.setPoint(2, {bx + 10.0f, by + 22.0f});
        play.setFillColor(badgeColor);
        target.draw(play);
        drawText(target, badge, {bx + 26.0f, by + 9.0f}, 11u, badgeColor, 0.0f, fonts_.digits);

        if ((overlayTick_ / 24) % 2 == 0) { // REC piscando
            sf::CircleShape rec(6.0f);
            rec.setOrigin({6.0f, 6.0f});
            rec.setPosition({fx + 20.0f, fy + 24.0f});
            rec.setFillColor(hex(0xff0055));
            target.draw(rec);
        }
        char timecode[40];
        std::snprintf(timecode, sizeof(timecode), "SP PLAY 00:03:%02d:%02d", static_cast<int>(replay.timer * 0.05f) % 100,
                      static_cast<int>(std::fmod(replay.timer, 30.0f)));
        drawText(target, timecode, {fx + 34.0f, fy + 18.0f}, 11u, sf::Color::White, 0.0f, fonts_.digits);
        drawText(target, "AUTO TRACKING: ON", {fx + 16.0f, fy + fh - 24.0f}, 9u, sf::Color(255, 255, 255, 180), 0.0f, fonts_.digits);
        drawText(target, "CH 03 STEREO HI-FI", {fx + fw - 16.0f, fy + fh - 24.0f}, 9u, sf::Color(255, 255, 255, 180), 1.0f, fonts_.digits);
        scanlines(36, 1.0f);
        for (int i = 0; i < 3; ++i) rect(0, std::fmod(t * 4.0f + i * 220.0f, kLogicalHeight + 40.0f) - 20.0f, kLogicalWidth, 2.0f, sf::Color(255, 255, 255, 10));
        if (u(rng_) < 0.18f) rect(0, u(rng_) * kLogicalHeight, kLogicalWidth, 3.0f + u(rng_) * 8.0f, sf::Color(255, 255, 255, 38));
    }
    drawText(target, "PULAR REPLAY  [ESPAÇO]", {400.0f, kLogicalHeight - 58.0f}, 16u, sf::Color(255, 255, 255, 200), 0.5f);
}

void RingRenderer::placeCamera(const Fighter& player, float ringAngle) {
    const bool playerBig = player.isBigChar();
    const bool playerSmall = player.isSmallChar();
    const bool enemyBig = engine_ && engine_->enemy.isBigChar();
    const bool enemySmall = engine_ && engine_->enemy.isSmallChar();

    float baseRadius = -260.0f - (player.radius - 60.0f);
    if (playerBig) baseRadius -= 65.0f;
    else if (playerSmall) baseRadius += 20.0f;
    if (enemyBig) baseRadius -= 25.0f;
    else if (enemySmall) baseRadius += 15.0f;

    float normalCamRadius = baseRadius;
    float normalCamY = (playerBig ? 245.0f : (playerSmall ? 205.0f : 220.0f)) + (enemyBig ? 14.0f : (enemySmall ? -10.0f : 0.0f));
    float normalPitch = (playerBig ? 0.48f : (playerSmall ? 0.43f : 0.45f)) + (enemyBig ? 0.015f : (enemySmall ? -0.015f : 0.0f));
    float normalLateral = player.offsetX * 0.6f;

    // Câmera panorâmica das cordas: mesma distância, altura e ângulo para TODOS os lutadores
    // Permite ver com clareza as 3 cordas e a deformação elástica (zoom out nas cordas, zoom in no combate)
    constexpr float kRopeCamRadius = -425.0f;
    constexpr float kRopeCamY = 260.0f;
    constexpr float kRopePitch = 0.51f;

    float camRadius = normalCamRadius * (1.0f - ropeZoom_) + kRopeCamRadius * ropeZoom_;
    float camY = normalCamY * (1.0f - ropeZoom_) + kRopeCamY * ropeZoom_;
    float pitch = normalPitch * (1.0f - ropeZoom_) + kRopePitch * ropeZoom_;
    float lateral = normalLateral * (1.0f - ropeZoom_);

    const Vec3 targetPos = {camRadius * std::sin(ringAngle) + lateral * std::cos(ringAngle),
                            camY,
                            camRadius * std::cos(ringAngle) - lateral * std::sin(ringAngle)};
    const float targetPitch = pitch;
    const float targetYaw = ringAngle;

    if (!cameraInitialized_) {
        camera_.pos = targetPos;
        camera_.pitch = targetPitch;
        camera_.yaw = targetYaw;
        cameraInitialized_ = true;
    } else {
        // Suavidade ao recalcular a posição da câmera (amortecimento exponencial fluido a 60 FPS)
        constexpr float kCamPosLerp = 0.16f;
        constexpr float kCamRotLerp = 0.16f;
        camera_.pos.x += (targetPos.x - camera_.pos.x) * kCamPosLerp;
        camera_.pos.y += (targetPos.y - camera_.pos.y) * kCamPosLerp;
        camera_.pos.z += (targetPos.z - camera_.pos.z) * kCamPosLerp;
        camera_.pitch += (targetPitch - camera_.pitch) * kCamRotLerp;
        camera_.yaw += (targetYaw - camera_.yaw) * kCamRotLerp;
    }
}

void RingRenderer::updateRopePhysics(const GameEngine& engine) {
    const std::array<sf::Vector2f, 4> corners = {
        sf::Vector2f{-kRingHalf, -kRingHalf}, {kRingHalf, -kRingHalf}, {kRingHalf, kRingHalf}, {-kRingHalf, kRingHalf}};

    std::array<std::array<float, 3>, 4> targetDeflect{};
    for (auto& s : targetDeflect) s.fill(0.0f);

    const MatchResult res = engine.result;
    const bool isKoEnd = engine.isMatchOver() &&
                         (res == MatchResult::WinKO || res == MatchResult::WinTKO || res == MatchResult::LossKO || res == MatchResult::LossTKO);
    const bool enemyLoses = isKoEnd && (res == MatchResult::WinKO || res == MatchResult::WinTKO);
    const bool playerLoses = isKoEnd && (res == MatchResult::LossKO || res == MatchResult::LossTKO);

    const Fighter* fighters[2] = {&engine.player, &engine.enemy};
    for (int slot = 0; slot < 2; ++slot) {
        // Se o lutador está sendo ejetado da arena (3ª queda TKO ou voo de KO), não deforma as cordas!
        if (isKoEnd) {
            if ((slot == 1 && enemyLoses) || (slot == 0 && playerLoses)) continue;
        }
        if (slot == 1 && engine.enemyKnockdowns >= GameEngine::kKnockdownsForTko && engine.enemy.state == FIGHTER_KNOCKDOWN) continue;
        if (slot == 0 && engine.playerKnockdowns >= GameEngine::kKnockdownsForTko && engine.player.state == FIGHTER_KNOCKDOWN) continue;

        const Fighter& f = *fighters[slot];
        Placement place{f.worldPos, f.facing};
        Reaction rx = computeReaction(f, slot, 0.0f, false, slot == 0);
        const float ox = f.offsetX + rx.dx;
        const float oy = f.offsetY + rx.dy;
        const float oz = f.offsetZ + rx.dz;
        const float bodyLift = f.isBigChar() ? 26.0f : (f.isSmallChar() ? 15.0f : 20.0f);
        Vec3 bodyWorld = Physics3D::localToWorld(ox, bodyLift + oy, oz, place.pos, place.facing);

        const float bodyR = f.isBigChar() ? 34.0f : (f.isSmallChar() ? 19.0f : 25.0f);
        const float extraLean = rx.rope * 18.0f;
        const float totalR = bodyR + extraLean;

        for (std::size_t side = 0; side < 4; ++side) {
            sf::Vector2f from = corners[side], to = corners[(side + 1) % 4];
            sf::Vector2f edge = to - from;
            float len = std::sqrt(edge.x * edge.x + edge.y * edge.y);
            sf::Vector2f tangent = edge / len;
            sf::Vector2f normal{tangent.y, -tangent.x}; // normal externa ao ringue

            sf::Vector2f v{bodyWorld.x - from.x, bodyWorld.z - from.y};
            float proj = v.x * tangent.x + v.y * tangent.y;
            float t = proj / len;
            if (t < 0.04f || t > 0.96f) continue;

            float distOut = v.x * normal.x + v.y * normal.y;
            float reach = distOut + totalR;
            if (reach > 0.0f) {
                constexpr float kMaxDeflect = 24.0f;
                float P = std::min(kMaxDeflect, reach);
                float wTop = rx.rope > 0.0f ? 1.0f : 0.92f;
                float wMid = 0.88f;
                float wBot = 0.65f;
                if (P * wTop > targetDeflect[side][2]) {
                    targetDeflect[side][2] = P * wTop;
                    targetDeflect[side][1] = P * wMid;
                    targetDeflect[side][0] = P * wBot;
                    ropes_[side].contactT = std::clamp(t, 0.12f, 0.88f);
                    ropes_[side].contactWidth = (f.isBigChar() ? 50.0f : (f.isSmallChar() ? 26.0f : 36.0f)) / len;
                }
            }
        }
    }

    const bool frameAdvanced = engine.frameIndex != lastRopeFrame_;
    if (frameAdvanced) {
        lastRopeFrame_ = engine.frameIndex;
        constexpr float dt = 1.0f / 60.0f;
        constexpr float kSpring = 280.0f; // rigidez elástica da corda
        constexpr float cDamp = 16.0f;    // amortecimento de ricochete
        for (std::size_t s = 0; s < 4; ++s) {
            for (int r = 0; r < 3; ++r) {
                float target = targetDeflect[s][r];
                if (target > 0.0f) {
                    ropes_[s].deflect[r] += (target - ropes_[s].deflect[r]) * 0.40f;
                    ropes_[s].velocity[r] = (target - ropes_[s].deflect[r]) * 10.0f;
                } else {
                    float force = -kSpring * ropes_[s].deflect[r] - cDamp * ropes_[s].velocity[r];
                    ropes_[s].velocity[r] += force * dt;
                    ropes_[s].deflect[r] += ropes_[s].velocity[r] * dt;
                    if (std::fabs(ropes_[s].deflect[r]) < 0.05f && std::fabs(ropes_[s].velocity[r]) < 0.1f) {
                        ropes_[s].deflect[r] = 0.0f;
                        ropes_[s].velocity[r] = 0.0f;
                    }
                }
            }
        }
    }
}

void RingRenderer::queueRing() {
    const bool harbor = arenaTheme_ == ArenaTheme::JoanaHarbor;
    const bool pernambuco = arenaTheme_ == ArenaTheme::ValeriaPernambuco;
    const bool observatory = arenaTheme_ == ArenaTheme::MarcovObservatory;
    const bool swamp = arenaTheme_ == ArenaTheme::FrogSwamp;
    const bool concert = arenaTheme_ == ArenaTheme::VivaldiConcertHall;
    const bool parlor = arenaTheme_ == ArenaTheme::CremosoParlor;
    const bool courtyard = arenaTheme_ == ArenaTheme::EspinhaCourtyard;
    const bool garden = arenaTheme_ == ArenaTheme::JackGarden;
    // A física continua no mesmo plano: só as superfícies do ringue mudam.
    auto floor = std::make_shared<std::vector<sf::Vertex>>();
    if (harbor) {
        constexpr float plankWidth = 45.0f;
        const std::array<uint32_t, 4> wood = {0x59412e, 0x67492f, 0x705138, 0x5f4733};
        for (int col = 0; col < 12; ++col) {
            const float x = -kRingHalf + col * plankWidth;
            // Recorta a tábua no plano próximo. Descartar o quadrilátero inteiro
            // fazia o piso terminar em uma linha reta antes da borda da tela.
            const std::array<Vec3, 4> world = {{{x, 0.0f, -kRingHalf}, {x + plankWidth, 0.0f, -kRingHalf},
                                                 {x + plankWidth, 0.0f, kRingHalf}, {x, 0.0f, kRingHalf}}};
            std::array<Vec3, 8> clipped{};
            std::size_t count = 0;
            for (std::size_t edge = 0; edge < world.size(); ++edge) {
                const Vec3 a = camera_.toLocal(world[edge]);
                const Vec3 b = camera_.toLocal(world[(edge + 1) % world.size()]);
                const bool aIn = a.z >= 12.0f, bIn = b.z >= 12.0f;
                if (aIn) clipped[count++] = a;
                if (aIn != bIn) {
                    const float t = (12.0f - a.z) / (b.z - a.z);
                    clipped[count++] = {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, 12.0f};
                }
            }
            if (count < 3) continue;
            const sf::Color color = hex(wood[static_cast<std::size_t>(col % 4)]);
            const auto first = camera_.projectLocal(clipped[0]);
            if (!first) continue;
            for (std::size_t tri = 1; tri + 1 < count; ++tri) {
                const auto b = camera_.projectLocal(clipped[tri]);
                const auto c = camera_.projectLocal(clipped[tri + 1]);
                if (!b || !c) continue;
                floor->push_back({first->pos, color});
                floor->push_back({b->pos, color});
                floor->push_back({c->pos, color});
            }
        }
    } else {
        for (float tx = -kRingHalf; tx < kRingHalf; tx += kTileStep) {
            for (float tz = -kRingHalf; tz < kRingHalf; tz += kTileStep) {
                auto a = camera_.project({tx, 0.0f, tz});
                auto b = camera_.project({tx + kTileStep, 0.0f, tz});
                auto c = camera_.project({tx + kTileStep, 0.0f, tz + kTileStep});
                auto d = camera_.project({tx, 0.0f, tz + kTileStep});
                if (!a || !b || !c || !d) continue;
                int ix = static_cast<int>((tx + kRingHalf) / kTileStep), iz = static_cast<int>((tz + kRingHalf) / kTileStep);
                sf::Color color = pernambuco
                    ? ((ix + iz) % 2 == 0 ? hex(0xc68e69) : hex(0xd8a578))
                    : observatory ? ((ix + iz) % 2 == 0 ? hex(0x1d3042) : hex(0x26394a))
                    : swamp ? ((ix + iz) % 2 == 0 ? hex(0x52645e) : hex(0x62746c))
                    : concert ? ((ix + iz) % 2 == 0 ? hex(0x79513e) : hex(0x885b43))
                    : parlor ? ((ix + iz) % 2 == 0 ? hex(0xd4a574) : hex(0xe7bd8c))
                    : courtyard ? ((ix + iz) % 2 == 0 ? hex(0x8d493e) : hex(0xa95b48))
                    : garden ? ((ix + iz) % 2 == 0 ? hex(0x947852) : hex(0xa6885f))
                    : ((ix + iz) % 2 == 0 ? hex(0x0d2a1b) : hex(0x174429));
                for (const auto* p : {&a, &b, &c, &a, &c, &d}) floor->push_back({(*p)->pos, color});
            }
        }
    }
    queue_.push_back({kLayerFloor, 0.0f, TriangleDraw{std::move(*floor), nullptr}});
    if (garden) {
        // Friso de madeira nobre e ouro na borda interna do tablado
        for (float edge : {-224.0f, 224.0f}) {
            queueWorldLine({edge, 0.6f, -224.0f}, {edge, 0.6f, 224.0f},
                           hex(0xd4a849), 2.2f, kLayerFloor + 0.04f);
            queueWorldLine({-224.0f, 0.6f, edge}, {224.0f, 0.6f, edge},
                           hex(0xd4a849), 2.2f, kLayerFloor + 0.04f);
        }
        // Brasão geométrico de favo de mel no centro do tablado do Jack
        constexpr int hexSegs = 6;
        for (int i = 0; i < hexSegs; ++i) {
            const float a = i * 2.0f * kPi / hexSegs;
            const float b = (i + 1) * 2.0f * kPi / hexSegs;
            // Hexágono externo (raio 68)
            queueWorldLine({std::sin(a) * 68.0f, 0.5f, std::cos(a) * 68.0f},
                           {std::sin(b) * 68.0f, 0.5f, std::cos(b) * 68.0f},
                           hex(0xd4a849), 2.4f, kLayerFloor + 0.04f);
            // Hexágono interno (raio 38)
            queueWorldLine({std::sin(a) * 38.0f, 0.5f, std::cos(a) * 38.0f},
                           {std::sin(b) * 38.0f, 0.5f, std::cos(b) * 38.0f},
                           hex(0xdfb455), 1.6f, kLayerFloor + 0.05f);
            // Raios do favo conectando os vértices ao centro
            queueWorldLine({std::sin(a) * 38.0f, 0.5f, std::cos(a) * 38.0f},
                           {0.0f, 0.5f, 0.0f},
                           hex(0xb88934), 1.4f, kLayerFloor + 0.05f);
        }
    }
    if (observatory) {
        // Trilhas orbitais gravadas no metal, discretas e sempre sob os pés.
        for (float radius : {105.0f, 205.0f}) {
            for (int segment = 0; segment < 28; ++segment) {
                const float a = segment * 2.0f * kPi / 28.0f;
                const float b = (segment + 1) * 2.0f * kPi / 28.0f;
                queueWorldLine({std::sin(a) * radius, 0.6f, std::cos(a) * radius},
                               {std::sin(b) * radius, 0.6f, std::cos(b) * radius},
                               hex(0x426b78), 1.5f, kLayerFloor + 0.06f);
            }
        }
    }
    if (concert) {
        // Tablado com filetes finos: legível sem disputar atenção com os pés.
        for (int i = -4; i <= 4; ++i) {
            const float x = i * 54.0f;
            queueWorldLine({x, 0.4f, -kRingHalf}, {x, 0.4f, kRingHalf},
                           hex(0xa67550), 1.4f, kLayerFloor + 0.04f);
        }
        for (float z : {-215.0f, 215.0f})
            queueWorldLine({-kRingHalf, 0.5f, z}, {kRingHalf, 0.5f, z},
                           hex(0xc59460), 2.0f, kLayerFloor + 0.05f);
    }
    if (courtyard) {
        // Friso de azulejo na borda, longe dos pés e sem clarear o centro do ringue.
        for (float edge : {-226.0f, 226.0f}) {
            queueWorldLine({edge, 0.6f, -226.0f}, {edge, 0.6f, 226.0f},
                           hex(0x3d9d9a), 2.0f, kLayerFloor + 0.04f);
            queueWorldLine({-226.0f, 0.6f, edge}, {226.0f, 0.6f, edge},
                           hex(0x3d9d9a), 2.0f, kLayerFloor + 0.04f);
        }
    }
    if (harbor) {
        for (int col = 0; col < 12; ++col) {
            const float x = -kRingHalf + col * 45.0f;
            queueWorldLine({x, 0.4f, -kRingHalf}, {x, 0.4f, kRingHalf}, hex(0x243234), 2.3f, kLayerFloor + 0.04f);
            queueWorldLine({x + 3.0f, 0.4f, -kRingHalf}, {x + 3.0f, 0.4f, kRingHalf}, hex(0x947150), 0.8f, kLayerFloor + 0.05f);
            for (float z : {-kRingHalf + 17.0f, kRingHalf - 17.0f})
                queueWorldLine({x + 19.0f, 0.5f, z}, {x + 24.0f, 0.5f, z}, hex(0x293031), 2.0f, kLayerFloor + 0.06f);
        }
    }

    const std::array<sf::Vector2f, 4> corners = {
        sf::Vector2f{-kRingHalf, -kRingHalf}, {kRingHalf, -kRingHalf}, {kRingHalf, kRingHalf}, {-kRingHalf, kRingHalf}};

    auto queueLine = [this](Vec3 a, Vec3 b, sf::Color color, float width, float layer) {
        auto pa = camera_.project(a), pb = camera_.project(b);
        if (!pa || !pb) return;
        float thick = std::max(1.0f, width * (pa->scale + pb->scale) * 0.5f);
        sf::Vector2f p0 = pa->pos, p1 = pb->pos;
        queue_.push_back({layer, (pa->depth + pb->depth) * 0.5f, lineTriangles(p0,p1,thick,color)});
    };
    // Os holofotes ficam fora do ringue: cordas e postes projetam sombras
    // curtas para dentro, sem pintar água, arquibancada ou o lado do tablado.
    auto queueRingShadow = [this](Vec3 a, Vec3 b, float width, std::uint8_t alpha) {
        auto inside = [](const Vec3& p) {
            return std::fabs(p.x) < kRingHalf - 3.0f && std::fabs(p.z) < kRingHalf - 3.0f;
        };
        if (!inside(a) || !inside(b)) return;
        auto pa = camera_.project(a), pb = camera_.project(b);
        if (!pa || !pb) return;
        const float thick = std::max(1.0f, width * (pa->scale + pb->scale) * 0.5f);
        queue_.push_back({kLayerFloor + 0.14f, (pa->depth + pb->depth) * 0.5f,
                          lineTriangles(pa->pos, pb->pos, thick, sf::Color(8, 14, 19, alpha))});
    };

    // Curva de deformação elástica da corda quando encostada pelo lutador
    auto ropeCurve = [](float t, float tc, float hw) {
        if (t <= 0.0f || t >= 1.0f) return 0.0f;
        float tent = (t < tc) ? (t / std::max(0.01f, tc)) : ((1.0f - t) / std::max(0.01f, 1.0f - tc));
        float d = std::fabs(t - tc);
        float crown = std::exp(-(d * d) / (2.0f * hw * hw));
        float sinArch = std::sin(t * kPi);
        return std::clamp(tent * (0.60f + 0.40f * crown) * (0.45f + 0.55f * sinArch), 0.0f, 1.0f);
    };

    for (std::size_t side = 0; side < corners.size(); ++side) {
        sf::Vector2f from = corners[side], to = corners[(side + 1) % corners.size()];
        sf::Vector2f edge = to - from;
        float len = std::sqrt(edge.x * edge.x + edge.y * edge.y);
        sf::Vector2f tangent = edge / len;
        sf::Vector2f normal{tangent.y, -tangent.x}; // normal externa

        // Saia do ringue e borda dourada fixa no chão (12 segmentos)
        for (int i = 0; i < 12; ++i) {
            float t1 = i / 12.0f, t2 = (i + 1) / 12.0f;
            sf::Vector2f a = from + edge * t1, b = from + edge * t2;
            float midDepth = camera_.depthOf({(a.x + b.x) * 0.5f, 0.0f, (a.y + b.y) * 0.5f});
            bool near = midDepth < kNearDepth;

            auto q0 = camera_.project({a.x, 0.0f, a.y}), q1 = camera_.project({b.x, 0.0f, b.y});
            auto q2 = camera_.project({b.x, -kApronDepth, b.y}), q3 = camera_.project({a.x, -kApronDepth, a.y});
            if (q0 && q1 && q2 && q3) {
                sf::Color color = harbor ? (i % 2 == 0 ? hex(0x352b2b) : hex(0x45332c))
                                         : pernambuco ? (i % 2 == 0 ? hex(0x754b50) : hex(0x8b5b55))
                                         : observatory ? (i % 2 == 0 ? hex(0x142b3b) : hex(0x1b3343))
                                         : swamp ? (i % 2 == 0 ? hex(0x273d34) : hex(0x314b3a))
                                         : concert ? (i % 2 == 0 ? hex(0x422737) : hex(0x583343))
                                         : parlor ? (i % 2 == 0 ? hex(0x895568) : hex(0xa76b70))
                                         : courtyard ? (i % 2 == 0 ? hex(0x633846) : hex(0x75424a))
                                         : garden ? (i % 2 == 0 ? hex(0x694e30) : hex(0x7c5d39))
                                         : (i % 2 == 0 ? hex(0x09131d) : hex(0x0c1a27));
                std::array<sf::Vertex, 6> quad = {{{q0->pos, color}, {q1->pos, color}, {q2->pos, color},
                                                   {q0->pos, color}, {q2->pos, color}, {q3->pos, color}}};
                queue_.push_back({near ? kLayerApronNear : kLayerApronFar, midDepth, [quad](sf::RenderTarget& t) {
                                      t.draw(quad.data(), quad.size(), sf::PrimitiveType::Triangles);
                                  }});
            }

            queueLine({a.x, 0.0f, a.y}, {b.x, 0.0f, b.y}, harbor ? hex(0xc69b64)
                                                                     : pernambuco ? hex(0xf8cf70)
                                                                     : observatory ? hex(0x68c9d5)
                                                                     : swamp ? hex(0xb4bc72)
                                                                     : concert ? hex(0xe2b374)
                                                                     : parlor ? hex(0xfbe2b6)
                                                                     : courtyard ? hex(0xd4a75e) : garden ? hex(0xe4b84e) : hex(0xffd700), 8.0f, kLayerBorder);
        }

        // Cordas elásticas deformáveis (32 segmentos para curvatura suave)
        constexpr int kRopeSegments = 32;
        for (std::size_t r = 0; r < kRopeHeights.size(); ++r) {
            if (harbor && r != 1) continue;
            float h = kRopeHeights[r];
            float def = ropes_[side].deflect[r];
            float tc = ropes_[side].contactT;
            float hw = ropes_[side].contactWidth;
            sf::Color ropeColor = harbor ? hex(0xd9c29a)
                                         : pernambuco ? (r == 0 ? hex(0xf26d56) : r == 1 ? hex(0xffe4ab) : hex(0x3bbbb2))
                                         : observatory ? (r == 1 ? hex(0xccecf0) : hex(0x5a9aaf))
                                         : swamp ? (r == 1 ? hex(0xcbd3a4) : hex(0x779b74))
                                         : concert ? (r == 1 ? hex(0xf2d3a2) : hex(0x964c61))
                                         : parlor ? (r == 1 ? hex(0xf8e0be) : hex(0xda8095))
                                         : courtyard ? (r == 1 ? hex(0xe2bd84) : hex(0x48a8a4))
                                         : garden ? (r == 1 ? hex(0xe7cf7e) : hex(0x547c4d))
                                         : (r == 1 ? hex(0xffffff) : hex(0xe63946));

            for (int k = 0; k < kRopeSegments; ++k) {
                float ta = k / static_cast<float>(kRopeSegments);
                float tb = (k + 1) / static_cast<float>(kRopeSegments);

                sf::Vector2f pa2d = from + edge * ta;
                sf::Vector2f pb2d = from + edge * tb;
                if (std::fabs(def) > 0.001f) {
                    pa2d += normal * (def * ropeCurve(ta, tc, hw));
                    pb2d += normal * (def * ropeCurve(tb, tc, hw));
                }

                Vec3 pa3d{pa2d.x, h, pa2d.y};
                Vec3 pb3d{pb2d.x, h, pb2d.y};
                float midDepth = camera_.depthOf({(pa3d.x + pb3d.x) * 0.5f, h, (pa3d.z + pb3d.z) * 0.5f});
                bool ropeNear = midDepth < kNearDepth;
                queueLine(pa3d, pb3d, ropeColor, 6.0f, ropeNear ? kLayerRopeNear : kLayerRopeFar);
                const float inward = h * 0.29f;
                const float along = h * 0.055f;
                queueRingShadow({pa2d.x - normal.x * inward + tangent.x * along, 0.7f,
                                 pa2d.y - normal.y * inward + tangent.y * along},
                                {pb2d.x - normal.x * inward + tangent.x * along, 0.7f,
                                 pb2d.y - normal.y * inward + tangent.y * along},
                                3.5f, static_cast<std::uint8_t>(r == 1 ? 36 : 26));
            }
        }
    }

    // Postes de canto com almofadas coloridas.
    const std::array<sf::Color, 4> padColors = harbor
        ? std::array<sf::Color, 4>{hex(0xd28c4d), hex(0x48aaa5), hex(0xd28c4d), hex(0x48aaa5)}
        : pernambuco ? std::array<sf::Color, 4>{hex(0xf1c650), hex(0x24a9a1), hex(0xef775f), hex(0x24a9a1)}
        : observatory ? std::array<sf::Color, 4>{hex(0x8bd6e2), hex(0xe87269), hex(0x8bd6e2), hex(0xe87269)}
        : swamp ? std::array<sf::Color, 4>{hex(0xc1c97b), hex(0x5d9f78), hex(0xc1c97b), hex(0x5d9f78)}
        : concert ? std::array<sf::Color, 4>{hex(0xe2b374), hex(0x994c65), hex(0xe2b374), hex(0x994c65)}
        : parlor ? std::array<sf::Color, 4>{hex(0xa8d5b9), hex(0xf1b1a3), hex(0xa8d5b9), hex(0xf1b1a3)}
        : courtyard ? std::array<sf::Color, 4>{hex(0xd7aa62), hex(0x51aca6), hex(0xd7aa62), hex(0x51aca6)}
        : garden ? std::array<sf::Color, 4>{hex(0xe4b84e), hex(0x6b995f), hex(0xe4b84e), hex(0x6b995f)}
        : std::array<sf::Color, 4>{hex(0xe63946), hex(0x3b82f6), hex(0xcbd5e1), hex(0xcbd5e1)};
    for (std::size_t i = 0; i < corners.size(); ++i) {
        float cx = corners[i].x, cz = corners[i].y;
        const float sx = std::copysign(1.0f, cx), sz = std::copysign(1.0f, cz);
        queueRingShadow({cx - sx * 5.0f, 0.8f, cz - sz * 5.0f},
                        {cx - sx * 35.0f, 0.8f, cz - sz * 35.0f}, 13.0f, 53);
        float layer = camera_.depthOf({cx, 60.0f, cz}) < kNearDepth ? kLayerPostNear : kLayerPostFar;
        queueLine({cx, -kApronDepth, cz}, {cx, kPostTop, cz}, harbor ? hex(0x493729)
                                                       : pernambuco ? hex(0x533a46)
                                                       : observatory ? hex(0x172c3c)
                                                       : swamp ? hex(0x28382d)
                                                       : concert ? hex(0x40263a)
                                                       : parlor ? hex(0x75495c)
                                                       : courtyard ? hex(0x724e5a) : garden ? hex(0x453c2c) : hex(0x334155), 14.0f, layer);
        queueLine({cx, -kApronDepth, cz}, {cx, kPostTop, cz}, harbor ? hex(0x9c784b)
                                                       : pernambuco ? hex(0xe4b66f)
                                                       : observatory ? hex(0x6aaebf)
                                                       : swamp ? hex(0x779377)
                                                       : concert ? hex(0xd6a974)
                                                       : parlor ? hex(0xf7d1a8)
                                                       : courtyard ? hex(0xe8b777) : garden ? hex(0xc59a50) : hex(0xe2e8f0), 4.0f, layer + 0.02f);
        if (harbor) {
            queueLine({cx, 63.0f, cz}, {cx, 74.0f, cz}, hex(0xe8c88d), 13.0f, layer + 0.03f);
        } else {
            for (float h : kRopeHeights) queueLine({cx, h - 8.0f, cz}, {cx, h + 8.0f, cz}, padColors[i], 20.0f, layer + 0.03f);
        }
        queueLine({cx, kPostTop - 5.0f, cz}, {cx, kPostTop + 5.0f, cz}, harbor ? hex(0x172d34)
                                                    : pernambuco ? hex(0x493440)
                                                    : observatory ? hex(0x102635)
                                                    : swamp ? hex(0x1c332b)
                                                    : concert ? hex(0x372238)
                                                    : parlor ? hex(0x633e54)
                                                    : courtyard ? hex(0x61445a) : garden ? hex(0x3f4831) : hex(0x0f172a), 16.0f, layer + 0.05f);
    }
}

// ---------------------------------------------------------------------------
// Arena: arquibancada, torcida, flashes e rosas
// ---------------------------------------------------------------------------

bool RingRenderer::appendWorldPolygon(TriangleDraw& td, float& outDepth, const Vec3* world, std::size_t nIn, sf::Color fill, sf::Color stroke) {
    constexpr float kNear = 12.0f;
    if (nIn < 3) return false;

    // Caminho otimizado em stack para polígonos usuais (até 14 vértices); fallback dinâmico para casos raros.
    constexpr std::size_t kMaxStackVerts = 16;
    std::array<Vec3, kMaxStackVerts> inBuf;
    std::array<Vec3, kMaxStackVerts> outBuf;
    std::vector<Vec3> inDyn, outDyn;
    Vec3* in = inBuf.data();
    Vec3* out = outBuf.data();
    std::size_t maxCapacity = kMaxStackVerts;

    if (nIn > 14) {
        inDyn.resize(nIn);
        outDyn.resize(nIn * 2);
        in = inDyn.data();
        out = outDyn.data();
        maxCapacity = outDyn.size();
    }

    for (std::size_t i = 0; i < nIn; ++i) in[i] = camera_.toLocal(world[i]);

    std::size_t outCount = 0;
    for (std::size_t i = 0; i < nIn; ++i) {
        const Vec3& a = in[i];
        const Vec3& b = in[(i + 1) % nIn];
        const bool aIn = a.z >= kNear, bIn = b.z >= kNear;
        if (aIn && outCount < maxCapacity) out[outCount++] = a;
        if (aIn != bIn && outCount < maxCapacity) {
            const float t = (kNear - a.z) / (b.z - a.z);
            out[outCount++] = {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, kNear};
        }
    }
    if (outCount < 3) return false;

    std::array<sf::Vector2f, kMaxStackVerts> ptsBuf;
    std::vector<sf::Vector2f> ptsDyn;
    sf::Vector2f* pts = ptsBuf.data();
    if (outCount > kMaxStackVerts) {
        ptsDyn.resize(outCount);
        pts = ptsDyn.data();
    }

    float depthSum = 0.0f;
    float minX = std::numeric_limits<float>::max(), minY = minX;
    float maxX = std::numeric_limits<float>::lowest(), maxY = maxX;
    for (std::size_t i = 0; i < outCount; ++i) {
        auto p = camera_.projectLocal({out[i].x, out[i].y, std::max(out[i].z, 10.5f)});
        if (!p) return false;
        pts[i] = p->pos;
        depthSum += p->depth;
        minX = std::min(minX, p->pos.x); maxX = std::max(maxX, p->pos.x);
        minY = std::min(minY, p->pos.y); maxY = std::max(maxY, p->pos.y);
    }
    // Descartar só faces inteiramente fora da janela, com margem para outlines.
    // A geometria permanece no mundo e volta a aparecer assim que a câmera gira.
    constexpr float kScreenMargin = 32.0f;
    if (maxX < -kScreenMargin || minX > kLogicalWidth + kScreenMargin ||
        maxY < -kScreenMargin || minY > kLogicalHeight + kScreenMargin) return false;

    const std::size_t triVerts = (outCount - 2) * 3;
    const std::size_t strokeVerts = (stroke.a > 0) ? outCount * 6 : 0;
    td.vertices.reserve(td.vertices.size() + triVerts + strokeVerts);

    for (std::size_t i = 1; i + 1 < outCount; ++i) {
        td.vertices.push_back({pts[0], fill});
        td.vertices.push_back({pts[i], fill});
        td.vertices.push_back({pts[i + 1], fill});
    }

    if (stroke.a > 0) {
        for (std::size_t i = 0; i < outCount; ++i) {
            const auto lt = lineTriangles(pts[i], pts[(i + 1) % outCount], 1.0f, stroke);
            td.vertices.insert(td.vertices.end(), lt.vertices.begin(), lt.vertices.end());
        }
    }

    outDepth = depthSum / static_cast<float>(outCount);
    return true;
}

void RingRenderer::queueWorldPolygon(const Vec3* world, std::size_t nIn, sf::Color fill, sf::Color stroke, float layer) {
    TriangleDraw td;
    td.texture = nullptr;
    float depth = 0.0f;
    if (appendWorldPolygon(td, depth, world, nIn, fill, stroke)) {
        queue_.push_back({layer, depth, std::move(td)});
    }
}

bool RingRenderer::appendWorldLine(TriangleDraw& td, float& outDepth, const Vec3& a, const Vec3& b, sf::Color color, float width) {
    constexpr float kNear = 12.0f;
    Vec3 la = camera_.toLocal(a), lb = camera_.toLocal(b);
    if (la.z < kNear && lb.z < kNear) return false;
    if (la.z < kNear) { float t = (kNear - la.z) / (lb.z - la.z); la = {la.x + (lb.x - la.x) * t, la.y + (lb.y - la.y) * t, kNear}; }
    if (lb.z < kNear) { float t = (kNear - lb.z) / (la.z - lb.z); lb = {lb.x + (la.x - lb.x) * t, lb.y + (la.y - lb.y) * t, kNear}; }
    auto pa = camera_.projectLocal(la), pb = camera_.projectLocal(lb);
    if (!pa || !pb) return false;
    sf::Vector2f p0 = pa->pos, p1 = pb->pos;
    float thick = std::max(1.0f, width * std::min(2.0f, (pa->scale + pb->scale) * 0.5f));
    const float margin = std::max(32.0f, thick);
    if (std::max(p0.x, p1.x) < -margin || std::min(p0.x, p1.x) > kLogicalWidth + margin ||
        std::max(p0.y, p1.y) < -margin || std::min(p0.y, p1.y) > kLogicalHeight + margin) return false;

    const sf::Vector2f d = p1 - p0;
    const float len = std::hypot(d.x, d.y);
    if (len >= 0.001f) {
        const sf::Vector2f n{-d.y / len * thick * 0.5f, d.x / len * thick * 0.5f};
        td.vertices.reserve(td.vertices.size() + 6);
        td.vertices.push_back({p0 + n, color});
        td.vertices.push_back({p1 + n, color});
        td.vertices.push_back({p1 - n, color});
        td.vertices.push_back({p0 + n, color});
        td.vertices.push_back({p1 - n, color});
        td.vertices.push_back({p0 - n, color});
    }
    outDepth = (pa->depth + pb->depth) * 0.5f;
    return true;
}

void RingRenderer::queueWorldLine(const Vec3& a, const Vec3& b, sf::Color color, float width, float layer) {
    TriangleDraw td;
    float depth = 0.0f;
    if (appendWorldLine(td, depth, a, b, color, width)) {
        queue_.push_back({layer, depth, std::move(td)});
    }
}

void RingRenderer::queueHarbor(const GameEngine& engine) {
    // A plataforma ocupa exatamente o plano da luta; toda a água e os barcos
    // vivem fora dele. Assim, golpes e colisões continuam sendo os do jogo.
    constexpr float waterY = -43.0f;
    const float time = static_cast<float>(engine.frameIndex);
    const sf::Color sea = hex(0x176b7d); // mesma cor do mar desenhado até o horizonte
    constexpr int seaSectors = 16;
    constexpr std::array<float, 4> seaRadii = {0.0f, 800.0f, 2800.0f, 16000.0f};

    BatchCollector seaBc(0.10f);
    BatchCollector wakeBc(0.11f);
    BatchCollector foamBc(0.19f);
    BatchCollector boatHullBc(0.45f);

    auto waterPoint = [time](float radius, float angle) {
        const float x = radius * std::sin(angle), z = radius * std::cos(angle);
        const float wave = radius > 7000.0f ? 0.0f
            : 3.0f * std::sin(x * 0.015f + time * 0.031f)
            + 2.0f * std::sin(z * 0.020f - time * 0.023f);
        return Vec3{x, waterY + wave, z};
    };
    std::array<std::array<Vec3, seaSectors>, 4> waterGrid;
    for (std::size_t ring = 0; ring < seaRadii.size(); ++ring) {
        for (int sector = 0; sector < seaSectors; ++sector) {
            const float a = sector * 2.0f * kPi / seaSectors;
            waterGrid[ring][sector] = waterPoint(seaRadii[ring], a);
        }
    }
    // Anéis circulares chegam além do horizonte de qualquer câmera da arena;
    // não há canto quadrado de malha aparecendo durante a luta ou a vitória.
    for (std::size_t ring = 0; ring + 1 < seaRadii.size(); ++ring) {
        for (int sector = 0; sector < seaSectors; ++sector) {
            const int next = (sector + 1) % seaSectors;
            seaBc.addPoly(*this, {waterGrid[ring][sector], waterGrid[ring + 1][sector],
                                  waterGrid[ring + 1][next], waterGrid[ring][next]},
                          sea, sf::Color::Transparent);
        }
    }

    // Ondulação e espuma de arrebentação ao redor da plataforma de madeira do cais
    constexpr int kPlatformSectors = 24;
    for (int s = 0; s < kPlatformSectors; ++s) {
        const float a = s * 2.0f * kPi / static_cast<float>(kPlatformSectors);
        const float b = (s + 1) * 2.0f * kPi / static_cast<float>(kPlatformSectors);
        const float waveA = 3.5f * std::sin(time * 0.042f + s * 0.65f);
        const float waveB = 3.5f * std::sin(time * 0.042f + (s + 1) * 0.65f);
        auto pRipple = [](float ang, float r, float y) {
            return Vec3{std::sin(ang) * r, y, std::cos(ang) * r};
        };
        // Faixa de sombra de contato da água sob o cais
        wakeBc.addPoly(*this, {pRipple(a, 305.0f, waterY + 0.4f), pRipple(b, 305.0f, waterY + 0.4f),
                               pRipple(b, 326.0f + waveB, waterY + 0.4f), pRipple(a, 326.0f + waveA, waterY + 0.4f)},
                      hex(0x135564), sf::Color::Transparent);
        // Friso de espuma ondulante
        wakeBc.addLine(*this, pRipple(a, 326.0f + waveA, waterY + 0.5f),
                              pRipple(b, 326.0f + waveB, waterY + 0.5f),
                              hex(0xc4ede6), 3.0f);
    }

    // Pequenas linhas de espuma derivam em duas velocidades, sem texturas nem uploads por quadro.
    for (int i = 0; i < 42; ++i) {
        const float x = std::sin(i * 17.17f) * 1180.0f;
        const float z = std::cos(i * 11.71f) * 1180.0f;
        if (std::fabs(x) < 325.0f && std::fabs(z) < 325.0f) continue;
        const float drift = std::sin(time * 0.025f + i * 2.1f) * 17.0f;
        const float w = 17.0f + (i % 4) * 11.0f;
        foamBc.addLine(*this, {x + drift - w, waterY + 1.0f, z}, {x + drift + w, waterY + 1.0f, z},
                       sf::Color(191, 235, 223, static_cast<uint8_t>(70 + i % 3 * 28)), 2.5f);
    }

    // A queda e os barcos usam a mesma frota para manter livre a área do splash.
    const auto& boats = kHarborBoats;
    struct Paint {
        uint32_t hull, hullLight, hullShade, keel, cream, accent, wood, woodLight, deck, shadow, brass;
    };
    static const std::array<Paint, 4> paints = {{
        // Turquesa Joana
        {0x1e8389, 0x39a4aa, 0x125459, 0x183642, 0xf5dfb8, 0xe26b48, 0x885c3b, 0xba8558, 0x473d32, 0x243037, 0xf6c167},
        // Traineira Vermelha
        {0xa33b2e, 0xc65344, 0x68241c, 0x361e1b, 0xf6ece1, 0x2b587a, 0x724b33, 0x9e6d4c, 0x3b3633, 0x2d2222, 0xf6c167},
        // Baleeira Cobalto
        {0x26557e, 0x3d74a4, 0x173652, 0x142536, 0xf4c768, 0xe58e37, 0x80593b, 0xb08157, 0x3e3a38, 0x1c2b38, 0xf6c167},
        // Pântano / Craca
        {0x556b46, 0x748e62, 0x37472d, 0x232e1d, 0xe3dcce, 0xcb7a48, 0x66533f, 0x8f775e, 0x35332e, 0x2b3225, 0xe4b669}
    }};
    static const std::array<const char*, 8> fan = {"weirdoFan", "punkCyclops", "crazyGoose", "jonas",
        "angryGranny", "zombieFan", "hotdogGuy", "cactomenFan"};
    for (std::size_t i = 0; i < boats.size(); ++i) {
        const HarborBoat& b = boats[i];

        // Frustum culling rápido por barco: descarta barcos atrás da câmera ou fora da tela
        const Vec3 localPos = camera_.toLocal({b.x, waterY, b.z});
        const float radius = b.length * 0.55f;
        if (localPos.z + radius < 12.0f) continue;
        if (std::fabs(localPos.x) > localPos.z * 1.6f + radius) continue;
        const bool isFar = (b.rows == 0) || (b.x * b.x + b.z * b.z > 1300.0f * 1300.0f);

        const float ca = std::cos(b.angle), sa = std::sin(b.angle);
        const float bob = std::sin(time * 0.035f + i * 1.7f) * 2.6f;
        auto at = [&](float u, float v, float y) { return Vec3{b.x + u * ca - v * sa, y + bob, b.z + u * sa + v * ca}; };
        auto shadowPt = [&](float u, float v) { return Vec3{b.x + u * ca - v * sa, waterY + 0.35f, b.z + u * sa + v * ca}; };
        const float half = b.length * 0.5f, beam = b.beam;
        const Paint& paint = paints[i % paints.size()];

        // Sombra de contato do casco projetada diretamente sobre a superfície da água
        wakeBc.addPoly(*this, {
            shadowPt(-0.92f * half, 0.0f),
            shadowPt(-0.75f * half, -0.68f * beam),
            shadowPt( 0.00f * half, -0.88f * beam),
            shadowPt( 0.72f * half, -0.58f * beam),
            shadowPt( 1.04f * half, 0.0f),
            shadowPt( 0.72f * half,  0.58f * beam),
            shadowPt( 0.00f * half,  0.88f * beam),
            shadowPt(-0.75f * half,  0.68f * beam)
        }, hex(0x0c3742), sf::Color::Transparent);

        if (!isFar) {
            // Marola e reflexo curto na linha d'água ao redor do casco
            const float wakeOffset = beam * (0.95f + 0.05f * std::sin(time * 0.035f + i * 1.7f));
            wakeBc.addLine(*this, shadowPt(-0.65f * half, -wakeOffset), shadowPt(0.60f * half, -wakeOffset),
                           hex(0x288796), 3.0f);
            wakeBc.addLine(*this, shadowPt(-0.65f * half,  wakeOffset), shadowPt(0.60f * half,  wakeOffset),
                           hex(0x288796), 3.0f);

            // Espuma suave na roda de proa (corte da água)
            wakeBc.addLine(*this, shadowPt(1.02f * half, 0.0f), shadowPt(1.14f * half, -0.22f * beam),
                           hex(0xc8f0ea), 2.5f);
            wakeBc.addLine(*this, shadowPt(1.02f * half, 0.0f), shadowPt(1.14f * half,  0.22f * beam),
                           hex(0xc8f0ea), 2.5f);
        }

        struct Rib { float u, w, keel, chine, top; };
        const std::array<Rib, 8> ribs = {{
            {-0.94f * half, 0.48f * beam, waterY - 4.5f,  waterY + 11.5f, waterY + 52.2f},
            {-0.74f * half, 0.80f * beam, waterY - 10.5f, waterY + 2.7f,  waterY + 43.4f},
            {-0.43f * half, 0.96f * beam, waterY - 12.6f, waterY - 1.6f,  waterY + 37.9f},
            { 0.00f * half, 1.00f * beam, waterY - 13.2f, waterY - 2.7f,  waterY + 35.7f},
            { 0.43f * half, 0.92f * beam, waterY - 10.5f, waterY + 1.6f,  waterY + 41.2f},
            { 0.74f * half, 0.64f * beam, waterY - 1.6f,  waterY + 13.7f, waterY + 54.4f},
            { 0.96f * half, 0.22f * beam, waterY + 17.0f, waterY + 32.4f, waterY + 69.8f},
            { 1.05f * half, 0.00f * beam, waterY + 34.6f, waterY + 51.1f, waterY + 81.9f}
        }};
        const Rib& r0 = ribs[0];
        const Rib& r6 = ribs[6];
        const Rib& r7 = ribs[7];

        // 1. Casco exterior em 8 estações (ambos os bordos)
        for (float side : {-1.0f, 1.0f}) {
            for (std::size_t s = 0; s + 1 < ribs.size(); ++s) {
                const Rib& a = ribs[s];
                const Rib& b = ribs[s + 1];
                const float za = a.w * side, zb = b.w * side;

                // Fundo: quilha ao chine
                boatHullBc.addPoly(*this, {at(a.u, 0.0f, a.keel), at(b.u, 0.0f, b.keel),
                                           at(b.u, zb * 0.56f, b.chine), at(a.u, za * 0.56f, a.chine)},
                                  hex(paint.keel), sf::Color::Transparent);

                // Lateral inferior do casco
                const uint32_t hullShadeCol = s % 2 == 0 ? paint.hullShade : paint.hull;
                boatHullBc.addPoly(*this, {at(a.u, za * 0.56f, a.chine), at(b.u, zb * 0.56f, b.chine),
                                           at(b.u, zb * 0.90f, b.top - 20.0f), at(a.u, za * 0.90f, a.top - 20.0f)},
                                  hex(hullShadeCol), sf::Color::Transparent);

                // Lateral superior do casco
                const uint32_t hullMainCol = s % 2 == 0 ? paint.hull : paint.hullLight;
                boatHullBc.addPoly(*this, {at(a.u, za * 0.90f, a.top - 20.0f), at(b.u, zb * 0.90f, b.top - 20.0f),
                                           at(b.u, zb * 0.98f, b.top - 6.0f), at(a.u, za * 0.98f, a.top - 6.0f)},
                                  hex(hullMainCol), sf::Color::Transparent);

                // Faixa clara tradicional (sheer strake)
                boatHullBc.addPoly(*this, {at(a.u, za * 0.98f, a.top - 6.0f), at(b.u, zb * 0.98f, b.top - 6.0f),
                                           at(b.u, zb, b.top), at(a.u, za, a.top)},
                                  hex(paint.cream), sf::Color::Transparent);

                // Friso protetor saliente (rub rail)
                boatHullBc.addLine(*this, at(a.u, za * 0.91f, a.top - 19.0f),
                                   at(b.u, zb * 0.91f, b.top - 19.0f), hex(paint.accent), 4.0f);

                // Borda superior maciça (cap rail)
                boatHullBc.addLine(*this, at(a.u, za, a.top), at(b.u, zb, b.top), hex(paint.woodLight), 5.5f);
            }
        }

        // 2. Painel de popa (transom) com leme náutico
        boatHullBc.addPoly(*this, {at(r0.u, -r0.w, r0.top), at(r0.u, r0.w, r0.top),
                                   at(r0.u, r0.w * 0.56f, r0.chine), at(r0.u, -r0.w * 0.56f, r0.chine)},
                          hex(paint.wood), sf::Color::Transparent);
        boatHullBc.addLine(*this, at(r0.u, -r0.w, r0.top), at(r0.u, r0.w, r0.top), hex(paint.cream), 5.5f);
        const float rx = r0.u - 2.0f;
        boatHullBc.addPoly(*this, {at(rx, 0.0f, r0.top + 2.0f), at(rx - 14.0f, 0.0f, r0.top + 3.0f),
                                   at(rx - 12.0f, 0.0f, r0.keel + 6.0f), at(rx, 0.0f, r0.keel - 4.0f)},
                          hex(paint.woodLight), sf::Color::Transparent);
        boatHullBc.addLine(*this, at(rx - 14.0f, 0.0f, r0.top + 3.0f), at(rx - 12.0f, 0.0f, r0.keel + 6.0f),
                           hex(0x18242b), 2.5f);
        boatHullBc.addLine(*this, at(rx - 13.0f, 0.0f, r0.top + 3.0f), at(r0.u + 18.0f, 0.0f, r0.top + 3.0f),
                           hex(paint.cream), 3.0f);

        // 3. Roda de proa erguida, deck, argola e lampião
        boatHullBc.addPoly(*this, {at(r6.u, 0.0f, r6.keel), at(r7.u, 0.0f, r7.keel),
                                   at(r7.u, 0.0f, r7.top + 8.0f), at(r6.u, 0.0f, r6.top)},
                          hex(paint.woodLight), sf::Color::Transparent);
        boatHullBc.addLine(*this, at(r6.u, 0.0f, r6.top), at(r7.u, 0.0f, r7.top + 8.0f), hex(paint.woodLight), 4.5f);
        boatHullBc.addLine(*this, at(r6.u, 0.0f, r6.keel), at(r7.u, 0.0f, r7.keel), hex(paint.keel), 3.5f);

        boatHullBc.addPoly(*this, {at(ribs[5].u, -ribs[5].w * 0.80f, ribs[5].top - 12.0f),
                                   at(r6.u, -r6.w * 0.70f, r6.top - 12.0f),
                                   at(r6.u, r6.w * 0.70f, r6.top - 12.0f),
                                   at(ribs[5].u, ribs[5].w * 0.80f, ribs[5].top - 12.0f)},
                          hex(paint.wood), sf::Color::Transparent);
        boatHullBc.addLine(*this, at(r7.u + 1.0f, -2.0f, r7.top + 4.0f), at(r7.u + 1.0f, 2.0f, r7.top + 4.0f),
                           hex(paint.brass), 3.5f);
        boatHullBc.addLine(*this, at(r7.u, 0.0f, r7.top + 4.0f), at(r7.u + 5.0f, 0.0f, r7.top + 2.0f),
                           hex(paint.brass), 2.5f);
        boatHullBc.addPoly(*this, {at(r7.u + 4.0f, -2.5f, r7.top - 1.0f), at(r7.u + 7.0f, -2.5f, r7.top - 1.0f),
                                   at(r7.u + 7.0f, 2.5f, r7.top - 1.0f), at(r7.u + 4.0f, 2.5f, r7.top - 1.0f)},
                          hex(paint.brass), sf::Color::Transparent);
        const float lanternGlow = 0.8f + 0.2f * std::sin(time * 0.09f + i);
        boatHullBc.addLine(*this, at(r7.u + 5.5f, -3.0f, r7.top + 1.5f), at(r7.u + 5.5f, 3.0f, r7.top + 1.5f),
                           sf::Color(255, 220, 110, static_cast<uint8_t>(200 * lanternGlow)), 4.5f);

        // 4. Estrado de madeira no interior (deck afundado)
        const float floorY = waterY + 20.3f;
        if (isFar) {
            boatHullBc.addPoly(*this, {at(ribs[0].u, -ribs[0].w * 0.82f, floorY), at(ribs[5].u, -ribs[5].w * 0.82f, floorY),
                                       at(ribs[5].u, ribs[5].w * 0.82f, floorY), at(ribs[0].u, ribs[0].w * 0.82f, floorY)},
                              hex(paint.deck), sf::Color::Transparent);
        } else {
            for (std::size_t s = 0; s < 5; ++s) {
                const Rib& a = ribs[s];
                const Rib& b = ribs[s + 1];
                boatHullBc.addPoly(*this, {at(a.u, -a.w * 0.82f, floorY), at(b.u, -b.w * 0.82f, floorY),
                                           at(b.u, b.w * 0.82f, floorY), at(a.u, a.w * 0.82f, floorY)},
                                  hex(s % 2 == 0 ? paint.deck : paint.shadow), sf::Color::Transparent);
            }
        }

        // 5. Bancos transversais de madeira maciça (thwarts)
        const float benchY = waterY + 29.0f;
        const std::array<float, 3> thwartU = {{-0.54f * half, -0.03f * half, 0.48f * half}};
        const std::array<float, 3> thwartW = {{0.85f * beam, 0.90f * beam, 0.82f * beam}};
        for (std::size_t k = 0; k < 3; ++k) {
            const float tu = thwartU[k], tw = thwartW[k];
            boatHullBc.addPoly(*this, {at(tu - 7.0f, -tw, benchY), at(tu + 7.0f, -tw, benchY),
                                       at(tu + 7.0f, tw, benchY), at(tu - 7.0f, tw, benchY)},
                              hex(paint.woodLight), sf::Color::Transparent);
            boatHullBc.addPoly(*this, {at(tu + 7.0f, -tw, benchY), at(tu + 7.0f, tw, benchY),
                                       at(tu + 7.0f, tw, benchY - 6.0f), at(tu + 7.0f, -tw, benchY - 6.0f)},
                              hex(paint.wood), sf::Color::Transparent);
            boatHullBc.addLine(*this, at(tu, -tw * 0.95f, benchY + 1.0f), at(tu, tw * 0.95f, benchY + 1.0f),
                               hex(paint.cream), 2.0f);
        }

        // 6. Torcida integrada: sentada com a cintura flush nos bancos, sem vãos vazios
        struct Seat { float u, v; int fanIdx; };
        const std::array<Seat, 3> seats1 = {{{thwartU[0], 0.0f, 0}, {thwartU[1], 0.0f, 1}, {thwartU[2], 0.0f, 2}}};
        const std::array<Seat, 8> seats2 = {{
            {thwartU[0], -thwartW[0] * 0.38f, 3},
            {thwartU[0],  thwartW[0] * 0.38f, 5},
            {thwartU[1], -thwartW[1] * 0.42f, 2},
            {thwartU[1],  thwartW[1] * 0.42f, 6},
            {thwartU[2], -thwartW[2] * 0.36f, 0},
            {thwartU[2],  thwartW[2] * 0.36f, 7},
            {-0.72f * half, -thwartW[0] * 0.48f, 1},
            {-0.72f * half,  thwartW[0] * 0.48f, 4}
        }};
        auto drawSeat = [&](const Seat& s) {
            const bool cheer = celebrating_ || engine.gameState == STATE_KNOCKDOWN;
            const float jump = cheer ? std::fabs(std::sin(time * 0.15f + i * 2.0f + s.fanIdx)) * 8.0f : 0.0f;
            const float sprW = b.rows == 1 ? 72.0f : 78.0f;
            const float sprH = b.rows == 1 ? 92.0f : 100.0f;
            const float seatY = benchY - sprH * 0.38f + jump;
            const Vec3 seatPos = at(s.u, s.v, seatY);
            const std::string name = std::string(fan[(i * 3 + s.fanIdx) % fan.size()]) + "FrontFrames_" +
                                     std::to_string((engine.frameIndex / 9) % 4);
            if (auto spr = sprite(name)) {
                queueSprite(*spr, seatPos, sprW, sprH, 0.0f, 0.45f, sf::Color(225, 230, 210), 0.0f, 0.0f);
            }
        };
        if (b.rows == 1) {
            for (const auto& s : seats1) drawSeat(s);
        } else if (b.rows >= 2) {
            for (const auto& s : seats2) drawSeat(s);
        }

        // 7. Acessórios náuticos: toletes de latão, boia e cracas na linha d'água (somente em barcos próximos/médios)
        if (!isFar) {
            boatHullBc.addLine(*this, at(-0.08f * half, -beam, waterY + 36.0f),
                               at(-0.08f * half, -beam, waterY + 40.0f), hex(paint.brass), 3.0f);
            boatHullBc.addLine(*this, at(-0.08f * half, beam, waterY + 36.0f),
                               at(-0.08f * half, beam, waterY + 40.0f), hex(paint.brass), 3.0f);

            boatHullBc.addLine(*this, at(-0.15f * half - 5.0f, beam + 2.0f, waterY + 28.0f),
                               at(-0.15f * half + 5.0f, beam + 2.0f, waterY + 28.0f), hex(paint.cream), 9.0f);
            boatHullBc.addLine(*this, at(-0.15f * half - 2.5f, beam + 2.0f, waterY + 28.0f),
                               at(-0.15f * half + 2.5f, beam + 2.0f, waterY + 28.0f), hex(paint.accent), 4.5f);

            boatHullBc.addLine(*this, at(-0.55f * half, beam * 0.90f, waterY + 3.0f),
                               at(-0.50f * half, beam * 0.90f, waterY + 5.0f), sf::Color(230, 238, 230), 2.5f);
            boatHullBc.addLine(*this, at(0.20f * half, beam * 0.85f, waterY + 4.0f),
                               at(0.25f * half, beam * 0.85f, waterY + 2.0f), sf::Color(230, 238, 230), 2.5f);
            boatHullBc.addLine(*this, at(-0.55f * half, -beam * 0.90f, waterY + 3.0f),
                               at(-0.50f * half, -beam * 0.90f, waterY + 5.0f), sf::Color(230, 238, 230), 2.5f);
            boatHullBc.addLine(*this, at(0.20f * half, -beam * 0.85f, waterY + 4.0f),
                               at(0.25f * half, -beam * 0.85f, waterY + 2.0f), sf::Color(230, 238, 230), 2.5f);
        }
    }

    seaBc.flush(queue_);
    wakeBc.flush(queue_);
    foamBc.flush(queue_);
    boatHullBc.flush(queue_);

    // Barbatana distante: aparece ocasionalmente e cruza a água sem se aproximar da luta.
    const float sharkPhase = std::fmod(time, 1500.0f);
    if (sharkPhase > 520.0f && sharkPhase < 760.0f) {
        const float x = -970.0f + (sharkPhase - 520.0f) * 7.4f;
        const float z = 1050.0f;
        queueWorldPolygon({{x - 42, waterY + 2, z}, {x + 12, waterY + 43, z},
                           {x + 5, waterY + 12, z}, {x + 42, waterY + 2, z}},
                          hex(0x263e46), hex(0x17323a), 0.34f);
        queueWorldLine({x - 51, waterY + 1, z}, {x + 53, waterY + 1, z}, hex(0xb6dad7), 3.0f, 0.35f);
    }

    if (celebrating_) {
        // Os peixes pulam em arcos escalonados apenas na comemoração.
        for (int i = 0; i < 4; ++i) {
            const float phase = std::fmod(celebFrames_ * 0.045f + i * 1.4f, 2.7f);
            if (phase > 1.0f) continue;
            const float x = (i % 2 ? 1.0f : -1.0f) * (350.0f + i * 20.0f) + (phase - 0.5f) * 68.0f;
            const float z = (i / 2 ? 1.0f : -1.0f) * (350.0f + i * 20.0f);
            const float y = waterY + 12.0f + 77.0f * std::sin(phase * kPi);
            queueWorldPolygon({{x - 17, y, z}, {x + 10, y + 8, z}, {x + 17, y - 2, z},
                               {x + 9, y - 7, z}, {x - 17, y, z}},
                              hex(i % 2 ? 0xf3b96c : 0xc7d9a6), hex(0x37585a), 3.20f);
            queueWorldLine({x - 15, y, z}, {x - 25, y + 10, z}, hex(0xe9d9a0), 4.0f, 3.21f);
        }
    }
}

void RingRenderer::queuePernambuco() {
    // Pátio de festa inspirado nas ruas de Recife e Olinda. Tudo existe no
    // espaço da arena: fachadas e enfeites acompanham os giros da câmera.
    constexpr int kHouses = 16;
    constexpr float kStreetRadius = 960.0f;
    constexpr std::array<uint32_t, 8> walls = {
        0xf0b46e, 0x72a8a0, 0xea8b72, 0xe9cc8f,
        0x8da3c5, 0xc87f82, 0xb0b982, 0xe0a573
    };
    constexpr std::array<uint32_t, 6> flags = {
        0xf8d768, 0x3cb7b0, 0xf27361, 0x648ac1, 0xefabca, 0xf3e2a7
    };
    const float time = static_cast<float>(frameCounter_) * 0.025f;
    auto polar = [](float a, float r, float y) {
        return Vec3{std::sin(a) * r, y, std::cos(a) * r};
    };

    BatchCollector bcStreet(0.08f);
    BatchCollector bcStreetLight(0.085f);
    BatchCollector bcHouses(0.125f);
    BatchCollector bcMoldings(0.132f);
    BatchCollector bcWindows(0.140f);
    BatchCollector bcLanterns(0.150f);
    BatchCollector bcBunting(0.182f);

    // 1. Calçamento de pedras portuguesas e meio-fio de granito
    for (int sector = 0; sector < 32; ++sector) {
        const float a = sector * 2.0f * kPi / 32.0f;
        const float b = (sector + 1) * 2.0f * kPi / 32.0f;
        // Lajes do calçamento colonial
        bcStreet.addPoly(*this, {polar(a, 705.0f, -38.0f), polar(b, 705.0f, -38.0f),
                                polar(b, 1110.0f, -38.0f), polar(a, 1110.0f, -38.0f)},
                         hex(sector % 2 ? 0x977c70 : 0xb5957c));
        // Juntas radiais e concêntricas de paralelepípedos
        bcStreet.addLine(*this, polar(a, 705.0f, -37.9f), polar(a, 1110.0f, -37.9f), hex(0x5a423a), 2.5f);
        bcStreet.addLine(*this, polar(a, 910.0f, -37.9f), polar(b, 910.0f, -37.9f), hex(0x5a423a), 2.0f);

        // Meio-fio de cantaria de granito na borda do ringue
        bcStreet.addLine(*this, polar(a, 706.0f, -37.5f), polar(b, 706.0f, -37.5f), hex(0xd2c0a8), 5.0f);
        bcStreet.addLine(*this, polar(a, 704.0f, -37.8f), polar(b, 704.0f, -37.8f), hex(0x3e2c26), 2.5f);
    }

    // 2. Casario colonial de Recife e Olinda
    const float lampPulse = 0.85f + 0.15f * std::sin(frameCounter_ * 0.065f);
    for (int i = 0; i < kHouses; ++i) {
        const float angle = (i + 0.5f) * 2.0f * kPi / kHouses;
        const float width = 260.0f + static_cast<float>(i % 3) * 19.0f;
        const float height = 245.0f + static_cast<float>((i * 7) % 4) * 22.0f;
        const float half = width * 0.5f;

        // u percorre a frente da casa; depth aproxima a fachada do ringue.
        auto at = [angle](float u, float y, float depth = 0.0f) {
            const float r = kStreetRadius - depth;
            return Vec3{std::sin(angle) * r + std::cos(angle) * u,
                        y, std::cos(angle) * r - std::sin(angle) * u};
        };

        const sf::Color wall = hex(walls[static_cast<std::size_t>(i % walls.size())]);
        const sf::Color shade = hex(i % 2 ? 0x684d5b : 0x735768);

        // Fachada frontal
        bcHouses.addPoly(*this, {at(-half, -34), at(half, -34), at(half, height), at(-half, height)},
                         wall, hex(0x543c4b));
        // Lateral em perspectiva da casa
        bcHouses.addPoly(*this, {at(half, -34), at(half, height),
                                at(half, height, -52.0f), at(half, -34, -52.0f)},
                         shade);

        // Beiral de telhas coloniais de barro (terracota) no topo da fachada
        bcHouses.addPoly(*this, {at(-half - 8, height, -6), at(half + 8, height, -6),
                                at(half + 12, height + 16, 12), at(-half - 12, height + 16, 12)},
                         hex(0xb84e36), hex(0x56231c));
        bcHouses.addLine(*this, at(-half - 12, height + 16, 12), at(half + 12, height + 16, 12),
                         hex(0xd9684c), 3.5f);

        // Platibanda, cornija clássica e faixa de azulejos decorados
        bcMoldings.addPoly(*this, {at(-half - 5, height - 13, 2), at(half + 5, height - 13, 2),
                                  at(half + 5, height + 5, 2), at(-half - 5, height + 5, 2)},
                           hex(i % 2 ? 0xf0d7a8 : 0xf8e5bd), hex(0x684654));
        bcMoldings.addPoly(*this, {at(-half + 5, 86, 3), at(half - 5, 86, 3),
                                  at(half - 5, 104, 3), at(-half + 5, 104, 3)},
                           hex(i % 2 ? 0x4b9ea4 : 0x5b82a5));
        bcMoldings.addLine(*this, at(-half + 6, 106, 4), at(half - 6, 106, 4),
                           hex(0xf5d7a4), 5.0f);

        // Janelas e portas coloniais com vergas em arco
        for (int column = -1; column <= 1; ++column) {
            const float x = static_cast<float>(column) * 74.0f;
            const float y0 = column == 0 ? -32.0f : 133.0f;
            const float y1 = column == 0 ? 76.0f : 206.0f;
            const float hw = column == 0 ? 30.0f : 25.0f;

            // Guarnição de estuque branco
            bcWindows.addPoly(*this, {at(x - hw - 5, y0 - 5, 6), at(x + hw + 5, y0 - 5, 6),
                                     at(x + hw + 5, y1 + 5, 6), at(x - hw - 5, y1 + 5, 6)},
                              hex(0xf5d9ad));
            // Abertura escura da janela/porta
            bcWindows.addPoly(*this, {at(x - hw, y0, 7), at(x + hw, y0, 7),
                                     at(x + hw, y1, 7), at(x - hw, y1, 7)},
                              column == 0 ? hex(0x584250) : hex(0x325b69));

            if (column != 0) {
                // Caixilho de madeira colonial
                bcWindows.addLine(*this, at(x, y0 + 3, 8), at(x, y1 - 3, 8), hex(0xeacb9a), 4.0f);
                bcWindows.addLine(*this, at(x - hw, y0 + 37, 8), at(x + hw, y0 + 37, 8), hex(0xeacb9a), 4.0f);
                // Peitoril da janela
                bcWindows.addLine(*this, at(x - hw - 11, y0 - 11, 10), at(x + hw + 11, y0 - 11, 10), hex(0x634855), 7.0f);
                // Verga em arco superior colonial (moldura arqueada de estuque)
                bcWindows.addLine(*this, at(x - hw - 4, y1 + 6, 7), at(x, y1 + 16, 7), hex(0xf8e5bd), 4.0f);
                bcWindows.addLine(*this, at(x, y1 + 16, 7), at(x + hw + 4, y1 + 6, 7), hex(0xf8e5bd), 4.0f);
            }
        }

        // Lampião colonial de ferro forjado entre as fachadas
        const float lampA = i * 2.0f * kPi / kHouses;
        const Vec3 lampBase = polar(lampA, kStreetRadius - 12.0f, 138.0f);
        const Vec3 lampTip = polar(lampA, kStreetRadius - 38.0f, 155.0f);
        const Vec3 lampHead = polar(lampA, kStreetRadius - 38.0f, 147.0f);

        // Braço curvo de ferro fundido
        bcLanterns.addLine(*this, lampBase, lampTip, hex(0x2a1c26), 4.5f);
        // Cúpula do lampião
        bcLanterns.addLine(*this, lampTip, lampHead, hex(0x3e2836), 8.0f);
        // Chama quente amarela-dourada piscando suavemente
        const sf::Color flameCol(static_cast<uint8_t>(255 * lampPulse),
                                 static_cast<uint8_t>(212 * lampPulse),
                                 static_cast<uint8_t>(105 * lampPulse));
        bcLanterns.addLine(*this, {lampHead.x, lampHead.y - 2.0f, lampHead.z},
                           {lampHead.x, lampHead.y + 4.0f, lampHead.z}, flameCol, 7.5f);

        // Mancha suave de luz quente do lampião no calçamento
        const Vec3 spotFloor = polar(lampA, kStreetRadius - 40.0f, -37.5f);
        const sf::Color spotCol(245, 195, 95, static_cast<uint8_t>(48 * lampPulse));
        bcStreetLight.addLine(*this, {spotFloor.x - 22.0f, spotFloor.y, spotFloor.z},
                              {spotFloor.x + 22.0f, spotFloor.y, spotFloor.z}, spotCol, 28.0f);
    }

    // 3. Cordões e bandeirolas de festa junina / carnaval em volta do pátio
    for (int i = 0; i < kHouses; ++i) {
        const float a = i * 2.0f * kPi / kHouses;
        const float b = (i + 1) * 2.0f * kPi / kHouses;
        const Vec3 left = polar(a, 855.0f, 327.0f);
        const Vec3 right = polar(b, 855.0f, 327.0f);
        bcBunting.addLine(*this, left, right, hex(0xf7d99c), 2.0f);

        for (int pennant = 1; pennant <= 4; ++pennant) {
            const float t = static_cast<float>(pennant) / 5.0f;
            const float x = left.x + (right.x - left.x) * t;
            const float z = left.z + (right.z - left.z) * t;
            const float flutter = 5.0f * std::sin(time + i * 1.7f + pennant * 0.8f);
            const float dx = (right.x - left.x) * 0.045f;
            const float dz = (right.z - left.z) * 0.045f;
            bcBunting.addPoly(*this, {{x - dx, 327.0f, z - dz}, {x + dx, 327.0f, z + dz},
                                      {x + flutter, 299.0f + flutter * 0.35f, z - flutter * 0.4f}},
                              hex(flags[static_cast<std::size_t>((i + pennant) % flags.size())]));
        }
    }

    // Flush dos lotes agrupados
    bcStreet.flush(queue_);
    bcStreetLight.flush(queue_);
    bcHouses.flush(queue_);
    bcMoldings.flush(queue_);
    bcWindows.flush(queue_);
    bcLanterns.flush(queue_);
    bcBunting.flush(queue_);

    // 4. Sombrinhas de frevo em volta do público (mantendo rotação e projeção estável)
    for (int i = 0; i < 12; ++i) {
        const float angle = (i + 0.25f) * 2.0f * kPi / 12.0f;
        const Vec3 stem = polar(angle, 735.0f, 166.0f);
        const Vec3 top = polar(angle, 735.0f, 244.0f);
        queueWorldLine(stem, top, hex(0x63424e), 5.0f, 0.43f);
        const auto p = camera_.project(top);
        if (!p) continue;
        const sf::Vector2f center = p->pos;
        const float radius = std::clamp(55.0f * p->scale, 4.0f, 31.0f);
        const float rot = time * 0.14f + i * 0.6f;
        const float depth = p->depth;
        queue_.push_back({0.44f, depth, [center, radius, rot](sf::RenderTarget& target) {
            constexpr std::array<uint32_t, 8> colors = {
                0xfbd463, 0x2eafa7, 0xf17058, 0x6e8ed1,
                0xf4d981, 0x3cb8ae, 0xe77c70, 0x6685c4
            };
            for (int wedge = 0; wedge < 8; ++wedge) {
                const float a = rot + wedge * 2.0f * kPi / 8.0f;
                const float b = rot + (wedge + 1) * 2.0f * kPi / 8.0f;
                const std::array<sf::Vertex, 3> triangle = {{
                    {center, hex(colors[static_cast<std::size_t>(wedge)])},
                    {{center.x + std::cos(a) * radius, center.y + std::sin(a) * radius * 0.65f}, hex(colors[static_cast<std::size_t>(wedge)])},
                    {{center.x + std::cos(b) * radius, center.y + std::sin(b) * radius * 0.65f}, hex(colors[static_cast<std::size_t>(wedge)])}
                }};
                target.draw(triangle.data(), triangle.size(), sf::PrimitiveType::Triangles);
            }
            sf::CircleShape hub(std::max(2.0f, radius * 0.13f));
            hub.setPosition({center.x - hub.getRadius(), center.y - hub.getRadius()});
            hub.setFillColor(hex(0x5b4050));
            target.draw(hub);
        }});
    }
}

void RingRenderer::queueObservatory() {
    // =========================================================================
    // OBSERVATÓRIO ESPACIAL DO DR. MARCOV (DARK PILOT)
    // Modelado em 3D sólido cel-shaded, ancorado em coordenadas globais.
    // Brilhos de borda controlados em metal, vidro e consoles com pulso lento.
    // =========================================================================
    auto polar = [](float a, float r, float y) {
        return Vec3{std::sin(a) * r, y, std::cos(a) * r};
    };

    auto isCulled = [&](const Vec3& pos, float radius) -> bool {
        const Vec3 local = camera_.toLocal(pos);
        if (local.z + radius < 15.0f) return true;
        if (std::fabs(local.x) > local.z * 1.6f + radius) return true;
        if (std::fabs(local.y) > local.z * 1.3f + radius) return true;
        return false;
    };

    // -------------------------------------------------------------------------
    // 1. PISO CIRCULAR & MARCADORES CELESTES NA BORDA DA PLATAFORMA
    // -------------------------------------------------------------------------
    BatchCollector floorBc(0.08f);

    // Setores do deck (r = 710 a 1350)
    for (int sector = 0; sector < 32; ++sector) {
        const float a = sector * 2.0f * kPi / 32.0f;
        const float b = (sector + 1) * 2.0f * kPi / 32.0f;
        const sf::Color tileCol = hex(sector % 2 ? 0x243748 : 0x2b4154);
        floorBc.addPoly(*this, {polar(a, 710.0f, -38.0f), polar(b, 710.0f, -38.0f),
                                polar(b, 1350.0f, -38.0f), polar(a, 1350.0f, -38.0f)},
                        tileCol, sf::Color::Transparent);

        // Guia metálica do perímetro interno (chanfro entre a torcida e o deck)
        floorBc.addPoly(*this, {polar(a, 706.0f, -38.0f), polar(b, 706.0f, -38.0f),
                                polar(b, 712.0f, -36.5f), polar(a, 712.0f, -36.5f)},
                        hex(0x385268), hex(0x527892));

        // Balizas luminosas de navegação estelar embutidas no piso (pulso suave)
        const float pulse = 0.72f + 0.28f * std::sin(frameCounter_ * 0.05f + sector * 0.45f);
        const sf::Color diodeCol(static_cast<uint8_t>(45 * pulse),
                                 static_cast<uint8_t>(215 * pulse),
                                 static_cast<uint8_t>(245 * pulse));
        const float midA = (a + b) * 0.5f;
        const float da = 0.014f;
        floorBc.addPoly(*this, {polar(midA, 708.5f, -36.3f),
                                polar(midA - da, 710.5f, -36.3f),
                                polar(midA, 712.5f, -36.3f),
                                polar(midA + da, 710.5f, -36.3f)},
                        diodeCol, hex(0xacf6ff));

        // Guarda-corpo do perímetro externo (parapeito voltado ao abismo espacial)
        floorBc.addPoly(*this, {polar(a, 1344.0f, -38.0f), polar(b, 1344.0f, -38.0f),
                                polar(b, 1354.0f, -16.0f), polar(a, 1354.0f, -16.0f)},
                        hex(sector % 2 ? 0x1d2c3b : 0x25394b), hex(0x405c74));
        // Corrimão superior do parapeito com reflexo estelar
        floorBc.addLine(*this, polar(a, 1354.0f, -16.0f), polar(b, 1354.0f, -16.0f),
                        hex(0x6a92aa), 3.0f);
    }
    floorBc.flush(queue_);

    // -------------------------------------------------------------------------
    // 2. CÚPULAS DO OBSERVATÓRIO COM FENDAS DE ABERTURA, VIDRO E ANEL ROTATIVO
    // -------------------------------------------------------------------------
    struct Dome { float angle, distance, radius, towerHeight, slitAngle; };
    constexpr std::array<Dome, 4> domes = {{{0.0f, 1080.0f, 206.0f, 210.0f, 0.42f},
                                             {1.62f, 1120.0f, 145.0f, 170.0f, 0.35f},
                                             {3.05f, 1140.0f, 180.0f, 195.0f, -0.38f},
                                             {4.68f, 1100.0f, 145.0f, 175.0f, 0.30f}}};
    constexpr int kSegments = 16;
    BatchCollector domeBc(0.12f);
    BatchCollector domeGlassBc(0.15f);

    for (std::size_t d = 0; d < domes.size(); ++d) {
        const Dome& dome = domes[d];
        const Vec3 center = polar(dome.angle, dome.distance, 0.0f);
        if (isCulled(center, dome.radius + 60.0f)) continue;

        auto ring = [&](float angle, float radius, float y) {
            return Vec3{center.x + std::sin(angle) * radius, y,
                        center.z + std::cos(angle) * radius};
        };

        for (int segment = 0; segment < kSegments; ++segment) {
            const float a = segment * 2.0f * kPi / kSegments;
            const float b = (segment + 1) * 2.0f * kPi / kSegments;
            const float midA = (a + b) * 0.5f;

            // Iluminação direcional cel-shade nas faces cilíndricas
            const float lightDot = std::cos(midA - 0.85f);
            const sf::Color towerCol = lightDot > 0.35f ? hex(0x446074)
                                     : (lightDot > -0.30f ? hex(0x30475b) : hex(0x223444));
            domeBc.addPoly(*this, {ring(a, dome.radius, -35.0f), ring(b, dome.radius, -35.0f),
                                   ring(b, dome.radius, dome.towerHeight), ring(a, dome.radius, dome.towerHeight)},
                           towerCol, hex(0x182634));

            // Anel metálico de rotação da cúpula com chanfro superior iluminado
            domeBc.addPoly(*this, {ring(a, dome.radius + 6.0f, dome.towerHeight - 11.0f),
                                   ring(b, dome.radius + 6.0f, dome.towerHeight - 11.0f),
                                   ring(b, dome.radius + 6.0f, dome.towerHeight + 8.0f),
                                   ring(a, dome.radius + 6.0f, dome.towerHeight + 8.0f)},
                           hex(0x5a7e93), hex(0x223646));
            // Friso de borda superior refletindo o luar
            domeBc.addLine(*this, ring(a, dome.radius + 6.0f, dome.towerHeight + 8.0f),
                           ring(b, dome.radius + 6.0f, dome.towerHeight + 8.0f),
                           hex(0xa4cee0), 2.5f);

            // Cúpula hemisférica em 4 patamares esféricos
            constexpr std::array<float, 5> domeR = {1.0f, 0.88f, 0.65f, 0.36f, 0.08f};
            constexpr std::array<float, 5> domeY = {8.0f, 40.0f, 74.0f, 98.0f, 114.0f};

            // Determina se este segmento é a fenda de abertura do telescópio
            float slitDiff = std::fabs(midA - dome.slitAngle);
            while (slitDiff > kPi) slitDiff = std::fabs(slitDiff - 2.0f * kPi);
            const bool isSlit = (slitDiff < (kPi / static_cast<float>(kSegments)) * 1.05f);

            for (std::size_t band = 0; band + 1 < domeR.size(); ++band) {
                const Vec3 v0 = ring(a, dome.radius * domeR[band], dome.towerHeight + domeY[band]);
                const Vec3 v1 = ring(b, dome.radius * domeR[band], dome.towerHeight + domeY[band]);
                const Vec3 v2 = ring(b, dome.radius * domeR[band + 1], dome.towerHeight + domeY[band + 1]);
                const Vec3 v3 = ring(a, dome.radius * domeR[band + 1], dome.towerHeight + domeY[band + 1]);

                if (isSlit && band >= 1) {
                    // Interior escuro da fenda de observação
                    domeBc.addPoly(*this, {ring(a, dome.radius * domeR[band] * 0.82f, dome.towerHeight + domeY[band]),
                                           ring(b, dome.radius * domeR[band] * 0.82f, dome.towerHeight + domeY[band]),
                                           ring(b, dome.radius * domeR[band + 1] * 0.82f, dome.towerHeight + domeY[band + 1]),
                                           ring(a, dome.radius * domeR[band + 1] * 0.82f, dome.towerHeight + domeY[band + 1])},
                                   hex(0x101a24), hex(0x182430));

                    // Silhueta do telescópio interno espiando o céu estrelado
                    if (band == 2) {
                        const Vec3 scopeEye = ring(midA, dome.radius * 0.70f, dome.towerHeight + 65.0f);
                        const Vec3 scopeTip = ring(midA, dome.radius * 0.86f, dome.towerHeight + 84.0f);
                        domeBc.addLine(*this, scopeEye, scopeTip, hex(0x283e50), 12.0f);
                        domeBc.addLine(*this, scopeTip, scopeTip, hex(0x68def0), 8.0f);
                    }
                    // Portas corrediças de aço nas laterais da fenda com contorno brilhante
                    domeBc.addLine(*this, v0, v3, hex(0xc0e2f2), 3.0f);
                    domeBc.addLine(*this, v1, v2, hex(0xc0e2f2), 3.0f);
                } else {
                    const sf::Color bandCol = lightDot > 0.35f ? hex(0xbed5dc)
                                            : (lightDot > -0.30f ? hex(0x82a5b6) : hex(0x567788));
                    domeBc.addPoly(*this, {v0, v1, v2, v3}, bandCol, hex(0x263a48));
                }
            }

            // Nervuras estruturais meridianas com reflexo de quadrinho
            if (segment % 4 == 0) {
                domeBc.addLine(*this, ring(a, dome.radius * 0.98f, dome.towerHeight + 10.0f),
                               ring(a, dome.radius * 0.10f, dome.towerHeight + 114.0f),
                               hex(0x9ac0d2), 3.5f);
            }
        }

        // Pináculo do ápice com sinalizador astronômico
        const Vec3 apexBase = ring(0.0f, 0.0f, dome.towerHeight + 114.0f);
        const Vec3 apexTip = ring(0.0f, 0.0f, dome.towerHeight + 148.0f);
        domeBc.addLine(*this, apexBase, apexTip, hex(0x6a8fa2), 4.0f);
        domeBc.addLine(*this, apexTip, apexTip, hex(0x65ebfa), 6.5f);

        // Galeria envidraçada sob cada cúpula com montantes verticais
        for (int segment = 0; segment < kSegments; segment += 2) {
            const float a = (segment + 0.15f) * 2.0f * kPi / kSegments;
            const float b = (segment + 0.85f) * 2.0f * kPi / kSegments;
            const sf::Color glassCol = (d + segment) % 3 == 0 ? hex(0x6ab8c8) : hex(0x8ae0ea);
            domeGlassBc.addPoly(*this, {ring(a, dome.radius + 1.2f, 114.0f),
                                        ring(b, dome.radius + 1.2f, 114.0f),
                                        ring(b, dome.radius + 1.2f, 144.0f),
                                        ring(a, dome.radius + 1.2f, 144.0f)},
                                glassCol, hex(0x203646));
        }
    }
    domeBc.flush(queue_);
    domeGlassBc.flush(queue_);

    // -------------------------------------------------------------------------
    // 3. TELESCÓPIO EQUATORIAL GIGANTE COM ANÉIS DE LATÃO, BASE E LENTE
    // -------------------------------------------------------------------------
    const Vec3 scope = polar(0.42f, 810.0f, 0.0f);
    if (!isCulled(scope, 240.0f)) {
        BatchCollector scopeBc(0.17f);

        // Base/pilar do telescópio na plataforma lateral
        const Vec3 pierBase{scope.x - 39.0f, 95.0f, scope.z};
        const Vec3 pierTop{scope.x - 39.0f, 192.0f, scope.z};
        scopeBc.addLine(*this, pierBase, pierTop, hex(0x1e3042), 26.0f);
        scopeBc.addLine(*this, pierBase, pierTop, hex(0x3a566d), 16.0f);

        // Bloco mecânico equatorial e cabeçote do eixo polar
        const Vec3 headCenter{scope.x - 28.0f, 196.0f, scope.z - 5.0f};
        scopeBc.addLine(*this, headCenter, {headCenter.x - 12.0f, headCenter.y + 16.0f, headCenter.z - 8.0f},
                        hex(0x50748a), 32.0f);

        // Haste do contrapeso e massas de equilíbrio
        const Vec3 cweightEnd{headCenter.x + 48.0f, headCenter.y - 42.0f, headCenter.z + 24.0f};
        scopeBc.addLine(*this, headCenter, cweightEnd, hex(0x2c4052), 8.0f);
        scopeBc.addLine(*this, {headCenter.x + 28.0f, headCenter.y - 25.0f, headCenter.z + 14.0f},
                        cweightEnd, hex(0x769aa8), 24.0f);
        scopeBc.addLine(*this, {headCenter.x + 36.0f, headCenter.y - 32.0f, headCenter.z + 18.0f},
                        cweightEnd, hex(0x203444), 20.0f);

        // Tubo óptico principal (OTA)
        const Vec3 scopeBase{scope.x - 10.0f, 192.0f, scope.z + 5.0f};
        const Vec3 scopeTip{scope.x - 130.0f, 296.0f, scope.z - 52.0f};

        // Célula do espelho primário (traseira espessa)
        scopeBc.addLine(*this, scopeBase,
                        {scopeBase.x - 24.0f, scopeBase.y + 20.0f, scopeBase.z - 10.0f},
                        hex(0x1a2a3a), 48.0f);

        // Tubo cilíndrico principal
        scopeBc.addLine(*this, scopeBase, scopeTip, hex(0x283e50), 38.0f);
        scopeBc.addLine(*this, scopeBase, scopeTip, hex(0x3a5468), 26.0f);

        // Filetes de latão astronômico (estética clássica de instrumento celeste)
        const auto lerpScope = [&](float t) {
            return Vec3{scopeBase.x + (scopeTip.x - scopeBase.x) * t,
                        scopeBase.y + (scopeTip.y - scopeBase.y) * t,
                        scopeBase.z + (scopeTip.z - scopeBase.z) * t};
        };
        scopeBc.addLine(*this, lerpScope(0.28f), lerpScope(0.33f), hex(0xd0a43a), 40.0f);
        scopeBc.addLine(*this, lerpScope(0.58f), lerpScope(0.63f), hex(0xd0a43a), 39.0f);
        scopeBc.addLine(*this, lerpScope(0.29f), lerpScope(0.32f), hex(0xf8dc76), 42.0f);
        scopeBc.addLine(*this, lerpScope(0.59f), lerpScope(0.62f), hex(0xf8dc76), 41.0f);

        // Linha de brilho branco-azulado de quadrinho no topo do tubo
        const Vec3 rimOffset{-4.0f, 15.0f, 0.0f};
        scopeBc.addLine(*this, {scopeBase.x + rimOffset.x, scopeBase.y + rimOffset.y, scopeBase.z + rimOffset.z},
                        {scopeTip.x + rimOffset.x, scopeTip.y + rimOffset.y, scopeTip.z + rimOffset.z},
                        hex(0xc8e6f5), 4.5f);

        // Coifa frontal do telescópio (dew shield)
        scopeBc.addLine(*this, lerpScope(0.85f), scopeTip, hex(0x324d62), 42.0f);
        scopeBc.addLine(*this, lerpScope(0.96f), scopeTip, hex(0x80aabf), 44.0f);

        // Lente objetiva com reflexo estelar brilhante
        scopeBc.addLine(*this, scopeTip, scopeTip, hex(0x184054), 36.0f);
        scopeBc.addLine(*this, scopeTip, scopeTip, hex(0x6ee6f8), 24.0f);
        scopeBc.addLine(*this, {scopeTip.x - 3.0f, scopeTip.y + 4.0f, scopeTip.z},
                        {scopeTip.x - 3.0f, scopeTip.y + 4.0f, scopeTip.z},
                        hex(0xffffff), 9.0f);

        // Luneta buscadora (finderscope) montada sobre o tubo principal
        const Vec3 finderBase{scopeBase.x - 12.0f, scopeBase.y + 24.0f, scopeBase.z - 4.0f};
        const Vec3 finderTip{scopeTip.x + 22.0f, scopeTip.y + 12.0f, scopeTip.z + 10.0f};
        scopeBc.addLine(*this, finderBase, finderTip, hex(0x1c2e3e), 12.0f);
        scopeBc.addLine(*this, finderBase, finderTip, hex(0xd0a43a), 6.0f);
        scopeBc.addLine(*this, finderTip, finderTip, hex(0x60e4f6), 9.0f);

        scopeBc.flush(queue_);
    }

    // -------------------------------------------------------------------------
    // 4. CONSOLES DE CONTROLE & RADAR ESTELAR DE COMANDO
    // -------------------------------------------------------------------------
    BatchCollector consoleBc(0.385f);
    const float pulse = 0.74f + 0.26f * std::sin(frameCounter_ * 0.045f);

    for (int i = 0; i < 8; ++i) {
        const float a = (i + 0.5f) * 2.0f * kPi / 8.0f;
        const float s = std::sin(a), c = std::cos(a);
        auto panel = [=](float u, float y, float radius) {
            return Vec3{s * radius + c * u, y, c * radius - s * u};
        };

        if (isCulled(panel(0, 170, 700), 75.0f)) continue;

        // Pedestal e chassi da estação de controle
        consoleBc.addPoly(*this, {panel(-44, 132, 702), panel(44, 132, 702),
                                  panel(44, 212, 702), panel(-44, 212, 702)},
                          hex(0x162636), hex(0x38546b));

        // Moldura do monitor chanfrada com quebra-luz superior
        consoleBc.addPoly(*this, {panel(-42, 136, 700), panel(42, 136, 700),
                                  panel(42, 210, 700), panel(-42, 210, 700)},
                          hex(0x1c2f42), hex(0x56778e));
        consoleBc.addPoly(*this, {panel(-42, 206, 700), panel(42, 206, 700),
                                  panel(38, 215, 694), panel(-38, 215, 694)},
                          hex(0x12202c), hex(0x405e74));

        // Tela CRT do radar astronômico
        consoleBc.addPoly(*this, {panel(-35, 145, 696), panel(35, 145, 696),
                                  panel(35, 203, 696), panel(-35, 203, 696)},
                          hex(0x0c202a), hex(0x224a56));

        // Radar circular estelar
        const float radarY = 174.0f;
        constexpr int kRadarPts = 8;
        for (int r = 0; r < kRadarPts; ++r) {
            const float ra = r * 2.0f * kPi / kRadarPts;
            const float rb = (r + 1) * 2.0f * kPi / kRadarPts;
            consoleBc.addLine(*this,
                              panel(std::sin(ra) * 19.0f, radarY + std::cos(ra) * 19.0f, 695),
                              panel(std::sin(rb) * 19.0f, radarY + std::cos(rb) * 19.0f, 695),
                              hex(0x1f5c68), 1.5f);
        }

        // Linha de varredura ativa do radar
        const float sweepA = frameCounter_ * 0.045f + i * 0.785f;
        consoleBc.addLine(*this, panel(0, radarY, 694),
                          panel(std::sin(sweepA) * 18.0f, radarY + std::cos(sweepA) * 18.0f, 694),
                          sf::Color(static_cast<uint8_t>(75 * pulse),
                                    static_cast<uint8_t>(245 * pulse),
                                    static_cast<uint8_t>(230 * pulse)), 2.5f);

        // Traço de dados / osciloscópio estelar inferior
        consoleBc.addLine(*this, panel(-28, 149, 694), panel(-10, 149, 694), hex(0x38d898), 2.0f);
        consoleBc.addLine(*this, panel(-10, 149, 694), panel(-5, 153, 694), hex(0x5cf2b8), 2.0f);
        consoleBc.addLine(*this, panel(-5, 153, 694), panel(0, 146, 694), hex(0x5cf2b8), 2.0f);
        consoleBc.addLine(*this, panel(0, 146, 694), panel(6, 151, 694), hex(0x5cf2b8), 2.0f);
        consoleBc.addLine(*this, panel(6, 151, 694), panel(28, 149, 694), hex(0x38d898), 2.0f);

        // Teclado/bancada com fileira de diodos de status
        consoleBc.addPoly(*this, {panel(-38, 130, 701), panel(38, 130, 701),
                                  panel(36, 138, 694), panel(-36, 138, 694)},
                          hex(0x223648), hex(0x3a566d));

        // Diodos coloridos piscando em sincronia cósmica
        const sf::Color leds[4] = {hex(0x3cd8f0), hex(0xffaa30), hex(0x3cd070), hex(0xf04878)};
        for (int l = 0; l < 4; ++l) {
            const float uLed = -22.0f + l * 14.5f;
            const float ledP = 0.65f + 0.35f * std::sin(frameCounter_ * 0.08f + i + l * 1.5f);
            const sf::Color col(static_cast<uint8_t>(leds[l].r * ledP),
                                static_cast<uint8_t>(leds[l].g * ledP),
                                static_cast<uint8_t>(leds[l].b * ledP));
            consoleBc.addLine(*this, panel(uLed - 2.5f, 134.0f, 693),
                              panel(uLed + 2.5f, 134.0f, 693), col, 3.5f);
        }
    }
    consoleBc.flush(queue_);

    // -------------------------------------------------------------------------
    // 5. CAMPO PROFUNDO: NEBULOSAS, CONSTELAÇÃO E CAMPO DE ESTRELAS
    // -------------------------------------------------------------------------
    // Nebulosas cósmicas no infinito (camada 0.035f)
    TriangleDraw nebulae;
    nebulae.texture = nullptr;
    auto addNebulaQuad = [&](float a0, float a1, float y0, float y1, sf::Color col) {
        const auto p0 = camera_.project(polar(a0, 5600.0f, y0));
        const auto p1 = camera_.project(polar(a1, 5600.0f, y0));
        const auto p2 = camera_.project(polar(a1, 5600.0f, y1));
        const auto p3 = camera_.project(polar(a0, 5600.0f, y1));
        if (p0 && p1 && p2 && p3) {
            nebulae.vertices.push_back({p0->pos, col});
            nebulae.vertices.push_back({p1->pos, col});
            nebulae.vertices.push_back({p2->pos, col});
            nebulae.vertices.push_back({p0->pos, col});
            nebulae.vertices.push_back({p2->pos, col});
            nebulae.vertices.push_back({p3->pos, col});
        }
    };
    addNebulaQuad(0.1f, 0.9f, 650.0f, 1150.0f, hex(0x161834));
    addNebulaQuad(3.2f, 4.0f, 700.0f, 1200.0f, hex(0x1a1838));
    if (!nebulae.vertices.empty()) queue_.push_back({0.035f, 0.0f, std::move(nebulae)});

    // Estrelas e constelações projetadas via câmera 3D (camada 0.04f)
    TriangleDraw stars;
    stars.texture = nullptr;
    std::vector<sf::Vector2f> constellationPts;

    for (int i = 0; i < 84; ++i) {
        const float a = i * 2.399963f;
        const float y = 520.0f + static_cast<float>((i * 97) % 720);
        const auto p = camera_.project(polar(a, 5500.0f, y));
        if (!p || p->pos.x < -6.0f || p->pos.x > kLogicalWidth + 6.0f ||
            p->pos.y < -6.0f || p->pos.y > 275.0f) continue;

        const float twinkle = 0.75f + 0.25f * std::sin(frameCounter_ * 0.06f + i * 1.3f);
        const float size = (i % 6 == 0 ? 2.2f : (i % 3 == 0 ? 1.5f : 1.0f)) * twinkle;

        sf::Color color;
        if (i % 7 == 0) color = hex(0x9ee8ff);      // Azul celeste brilhante
        else if (i % 5 == 0) color = hex(0xffe6a2); // Dourado quente
        else color = hex(0xe4eaf0);                 // Branco estelar puro

        const float x = p->pos.x, sy = p->pos.y;
        for (const auto& v : std::array<sf::Vector2f, 6>{{{x-size, sy-size}, {x+size, sy-size}, {x+size, sy+size},
                                                           {x-size, sy-size}, {x+size, sy+size}, {x-size, sy+size}}}) {
            stars.vertices.push_back({v, color});
        }

        // Picos de difração em cruz para estrelas brilhantes (estética de quadrinho)
        if (i % 7 == 0 && size > 1.8f) {
            const float arm = size * 2.2f;
            const sf::Color spikeCol(color.r, color.g, color.b, 160);
            for (const auto& v : std::array<sf::Vector2f, 6>{{{x-arm, sy-0.5f}, {x+arm, sy-0.5f}, {x+arm, sy+0.5f},
                                                               {x-arm, sy-0.5f}, {x+arm, sy+0.5f}, {x-arm, sy+0.5f}}}) {
                stars.vertices.push_back({v, spikeCol});
            }
            for (const auto& v : std::array<sf::Vector2f, 6>{{{x-0.5f, sy-arm}, {x+0.5f, sy-arm}, {x+0.5f, sy+arm},
                                                               {x-0.5f, sy-arm}, {x+0.5f, sy+arm}, {x-0.5f, sy+arm}}}) {
                stars.vertices.push_back({v, spikeCol});
            }
        }

        // Guarda estrelas do topo para desenhar a constelação
        if (i < 30 && constellationPts.size() < 5 && p->pos.y < 120.0f) {
            constellationPts.push_back(p->pos);
        }
    }

    // Linhas sutis de constelação estelar
    if (constellationPts.size() >= 4) {
        const sf::Color lineCol = hex(0x325874);
        auto addConstellationLine = [&](const sf::Vector2f& p0, const sf::Vector2f& p1) {
            const sf::Vector2f d = p1 - p0;
            const float len = std::hypot(d.x, d.y);
            if (len < 1.0f || len > 220.0f) return;
            const sf::Vector2f n(-d.y / len * 0.75f, d.x / len * 0.75f);
            stars.vertices.push_back({p0 - n, lineCol});
            stars.vertices.push_back({p1 - n, lineCol});
            stars.vertices.push_back({p1 + n, lineCol});
            stars.vertices.push_back({p0 - n, lineCol});
            stars.vertices.push_back({p1 + n, lineCol});
            stars.vertices.push_back({p0 + n, lineCol});
        };
        for (std::size_t c = 0; c + 1 < constellationPts.size(); ++c) {
            addConstellationLine(constellationPts[c], constellationPts[c + 1]);
        }
    }

    if (!stars.vertices.empty()) queue_.push_back({0.04f, 0.0f, std::move(stars)});
}

void RingRenderer::queueSwamp() {
    // =========================================================================
    // O PÂNTANO DO REI FROG (SWAMP ARENA)
    // Modelado em 3D sólido cel-shaded, ancorado em coordenadas globais.
    // Brilho de umidade nas margens de terra, sombras sob as vitórias-régias
    // e taboas firmemente enraizadas no solo do brejo.
    // =========================================================================
    auto polar = [](float a, float r, float y) {
        return Vec3{std::sin(a) * r, y, std::cos(a) * r};
    };

    BatchCollector waterBc(0.075f);
    BatchCollector bankBc(0.084f);
    BatchCollector padBc(0.105f);
    BatchCollector vegetationBc(0.150f);
    BatchCollector reedBc(0.155f);

    constexpr int kWaterSegments = 36;
    for (int i = 0; i < kWaterSegments; ++i) {
        const float a = i * 2.0f * kPi / kWaterSegments;
        const float b = (i + 1) * 2.0f * kPi / kWaterSegments;
        const float wave = 2.2f * std::sin(frameCounter_ * 0.035f + a * 4.0f);
        waterBc.addPoly(*this, {polar(a, 267.0f, -73.0f), polar(b, 267.0f, -73.0f),
                                polar(b, 1560.0f, -73.0f + wave), polar(a, 1560.0f, -73.0f + wave)},
                        hex(i % 3 == 0 ? 0x315c54 : 0x284e49), sf::Color::Transparent);
    }

    // Faixa de terra firme depois da água com beira irregular orgânica
    auto bankRadius = [](float a) {
        return 858.0f + 27.0f * std::sin(a * 5.0f + 0.4f)
                      + 13.0f * std::sin(a * 11.0f - 0.7f);
    };
    for (int i = 0; i < kWaterSegments; ++i) {
        const float a = i * 2.0f * kPi / kWaterSegments;
        const float b = (i + 1) * 2.0f * kPi / kWaterSegments;
        const float ra = bankRadius(a), rb = bankRadius(b);

        // Faixa de lama úmida / arrebentação lamacenta na beira da água
        bankBc.addPoly(*this, {polar(a, ra - 26.0f, -71.0f), polar(b, rb - 26.0f, -71.0f),
                               polar(b, rb, -67.0f), polar(a, ra, -67.0f)},
                       hex(0x233328), sf::Color::Transparent);

        // Reflexo de umidade na margem molhada (brilho especular de quadrinho na lama)
        const float wetPulse = 0.78f + 0.22f * std::sin(frameCounter_ * 0.045f + a * 4.0f);
        bankBc.addLine(*this, polar(a, ra - 8.0f, -68.0f), polar(b, rb - 8.0f, -68.0f),
                       sf::Color(static_cast<uint8_t>(90 * wetPulse),
                                 static_cast<uint8_t>(148 * wetPulse),
                                 static_cast<uint8_t>(122 * wetPulse)), 3.0f);

        // Terra firme e platôs de vegetação
        bankBc.addPoly(*this, {polar(a, ra, -67.0f), polar(b, rb, -67.0f),
                               polar(b, 1115.0f, -60.0f), polar(a, 1115.0f, -60.0f)},
                       hex(i % 3 == 0 ? 0x96884f : 0x887d49), sf::Color::Transparent);
        bankBc.addPoly(*this, {polar(a, 1115.0f, -60.0f), polar(b, 1115.0f, -60.0f),
                               polar(b, 2050.0f, -43.0f), polar(a, 2050.0f, -43.0f)},
                       hex(i % 2 == 0 ? 0x587447 : 0x637d4e), sf::Color::Transparent);
        bankBc.addLine(*this, polar(a, ra, -65.0f), polar(b, rb, -65.0f),
                       hex(0xc3ba75), 3.0f);
    }

    // A torcida do pântano fica sobre folhas gigantes ancoradas na água
    for (const Spectator& s : crowd_) {
        if (s.pos.y != 18.0f && s.pos.y != 68.0f) continue;
        if (s.skin % 2 == 0) continue;
        const Vec3 center{s.pos.x, -58.0f, s.pos.z};
        const float radius = s.pos.y == 18.0f ? 69.0f : 75.0f;

        // Sombra de contato da folha sob a água
        std::vector<Vec3> shadowPad;
        shadowPad.reserve(13);
        shadowPad.push_back({center.x, center.y - 1.2f, center.z});
        for (int j = 0; j <= 11; ++j) {
            const float a = s.angle + 0.22f + j * (2.0f * kPi - 0.44f) / 11.0f;
            shadowPad.push_back({center.x + std::sin(a) * (radius + 4.0f), center.y - 1.2f,
                                 center.z + std::cos(a) * (radius + 4.0f)});
        }
        padBc.addPoly(*this, shadowPad, hex(0x163026), sf::Color::Transparent);

        // Folha da torcida
        std::vector<Vec3> pad;
        pad.reserve(13);
        pad.push_back(center);
        for (int j = 0; j <= 11; ++j) {
            const float a = s.angle + 0.22f + j * (2.0f * kPi - 0.44f) / 11.0f;
            pad.push_back({center.x + std::sin(a) * radius, center.y,
                           center.z + std::cos(a) * radius});
        }
        padBc.addPoly(*this, pad, hex(s.pos.y == 18.0f ? 0x709c59 : 0x5e8954), hex(0x294b3b));
        padBc.addLine(*this, {center.x, center.y + 1.0f, center.z},
                      {center.x + std::sin(s.angle + 0.6f) * 48.0f, center.y + 1.0f,
                       center.z + std::cos(s.angle + 0.6f) * 48.0f},
                      hex(0x426b47), 2.0f);
    }

    // Pequenas ondulações suaves na lâmina d'água
    for (int i = 0; i < 18; ++i) {
        const float a = (i + 0.35f) * 2.0f * kPi / 18.0f;
        const float r = 590.0f + static_cast<float>((i * 91) % 520);
        const float sway = std::sin(frameCounter_ * 0.045f + i * 0.8f) * 5.0f;
        for (int j = 0; j < 3; ++j) {
            const float ang0 = a + (j - 1.5f) * 0.055f;
            const float ang1 = a + (j - 0.5f) * 0.055f;
            waterBc.addLine(*this, polar(ang0, r + sway, -69.0f), polar(ang1, r + sway, -69.0f),
                           hex(0x75a68a), 1.6f);
        }
    }

    // Vitórias-régias flutuantes com sombras de oclusão na água
    for (int i = 0; i < 13; ++i) {
        const float a = (i + 0.2f) * 2.0f * kPi / 13.0f;
        const float distance = 570.0f + static_cast<float>((i * 53) % 330);
        const Vec3 c = polar(a, distance, -62.0f);
        const float r = 43.0f + static_cast<float>((i * 17) % 25);

        // Sombra de contato da vitória-régia sobre a água
        std::vector<Vec3> shadowPad;
        shadowPad.reserve(13);
        shadowPad.push_back({c.x, c.y - 1.4f, c.z});
        for (int j = 0; j <= 11; ++j) {
            const float edge = a + 0.23f + j * (2.0f * kPi - 0.46f) / 11.0f;
            shadowPad.push_back({c.x + std::sin(edge) * (r + 3.0f), c.y - 1.4f, c.z + std::cos(edge) * (r + 3.0f)});
        }
        padBc.addPoly(*this, shadowPad, hex(0x163026), sf::Color::Transparent);

        // Folha da vitória-régia
        std::vector<Vec3> pad;
        pad.reserve(13);
        pad.push_back(c);
        for (int j = 0; j <= 11; ++j) {
            const float edge = a + 0.23f + j * (2.0f * kPi - 0.46f) / 11.0f;
            pad.push_back({c.x + std::sin(edge) * r, c.y, c.z + std::cos(edge) * r});
        }
        padBc.addPoly(*this, pad, hex(i % 2 ? 0x5a8751 : 0x74a563), hex(0x294e3c));

        // Nervura central da vitória-régia
        padBc.addLine(*this, c, {c.x + std::sin(a + 1.2f) * r * 0.85f, c.y + 0.5f, c.z + std::cos(a + 1.2f) * r * 0.85f},
                      hex(0x386445), 1.5f);

        if (i % 3 == 0) {
            // Broto / flor do brejo no centro da vitória-régia
            padBc.addPoly(*this, {{c.x - 9, c.y + 2, c.z}, {c.x, c.y + 19, c.z - 5},
                                  {c.x + 9, c.y + 2, c.z}},
                          hex(0xe0d496), sf::Color::Transparent);
        }
    }

    // Árvores do pântano com raízes aéreas e copas com sombreamento cel-shade
    const float rightX = std::cos(camera_.yaw), rightZ = -std::sin(camera_.yaw);
    for (int i = 0; i < 11; ++i) {
        const float a = (i + 0.48f) * 2.0f * kPi / 11.0f;
        const Vec3 root = polar(a, 945.0f + static_cast<float>((i * 79) % 250), -61.0f);
        const float height = 205.0f + static_cast<float>((i * 37) % 65);
        const Vec3 crown{root.x + (i % 2 ? 32.0f : -30.0f), height, root.z};
        const sf::Color bark = hex(i % 2 ? 0x344b35 : 0x3f5136);
        auto leaf = [&](float dx, float dy) {
            return Vec3{crown.x + rightX * dx, crown.y + dy, crown.z + rightZ * dx};
        };

        // Tronco maciço com contorno e galhos
        vegetationBc.addLine(*this, root, crown, hex(0x172e2a), 37.0f);
        vegetationBc.addLine(*this, root, crown, bark, 26.0f);
        for (int branch = -2; branch <= 2; ++branch) {
            if (branch == 0) continue;
            const float reach = branch * 47.0f;
            const Vec3 tip{crown.x + rightX * reach, height + 45.0f - std::abs(branch) * 14.0f,
                           crown.z + rightZ * reach};
            vegetationBc.addLine(*this, {crown.x, height - 48.0f, crown.z}, tip, bark, 12.0f);
            vegetationBc.addLine(*this, tip, {tip.x, tip.y - 35.0f, tip.z}, hex(0x5a7550), 3.0f);
        }
        vegetationBc.addPoly(*this, {leaf(-98, -4), leaf(-75, 44), leaf(-35, 68), leaf(0, 55),
                                     leaf(47, 75), leaf(91, 37), leaf(105, -8), leaf(58, -27),
                                     leaf(2, -17), leaf(-47, -34)},
                             hex(0x254c3b), hex(0x18372f));
        vegetationBc.addPoly(*this, {leaf(-64, 2), leaf(-32, 33), leaf(0, 23), leaf(38, 39),
                                     leaf(72, 4), leaf(35, -13), leaf(-12, -10)},
                             hex(0x4b7550), sf::Color::Transparent);
    }

    // Taboas do brejo (cattails): brotam da lama em montes enraizados firmes
    for (int clump = 0; clump < 13; ++clump) {
        const float a = (clump + 0.34f) * 2.0f * kPi / 13.0f;
        const Vec3 center = polar(a, 770.0f + static_cast<float>((clump * 47) % 135), -66.0f);
        const float tx = std::cos(a), tz = -std::sin(a);

        // Monte de terra/musgo na base do tufo de taboas (ancoragem visual ao solo)
        reedBc.addPoly(*this, {
            {center.x - tx * 36.0f, center.y - 2.0f, center.z - tz * 36.0f},
            {center.x + tx * 36.0f, center.y - 2.0f, center.z + tz * 36.0f},
            {center.x + tx * 18.0f, center.y + 6.0f, center.z + tz * 18.0f},
            {center.x - tx * 18.0f, center.y + 6.0f, center.z - tz * 18.0f}
        }, hex(0x273523), hex(0x182416));

        for (int stalk = -2; stalk <= 2; ++stalk) {
            const float offset = stalk * 17.0f;
            const Vec3 root{center.x + tx * offset, center.y + 2.0f,
                            center.z + tz * offset};
            const float height = 142.0f + static_cast<float>((clump * 11 + stalk * 7 + 35) % 5) * 15.0f;
            const float bend = 7.0f * std::sin(frameCounter_ * 0.026f + clump + stalk * 0.65f);
            const Vec3 top{root.x + tx * bend, root.y + height,
                           root.z + tz * bend};

            // Haste verde-oliva
            reedBc.addLine(*this, root, top, hex(stalk % 2 == 0 ? 0xa2b762 : 0x7d9a58), 4.0f);
            if (stalk % 2 == 0) {
                reedBc.addPoly(*this, {root,
                                       {root.x + tx * 20.0f, root.y + 91.0f, root.z + tz * 20.0f},
                                       {root.x + tx * 8.0f, root.y + 20.0f, root.z + tz * 8.0f}},
                               hex(0x8cae61), sf::Color::Transparent);
            }
            if (stalk == -1 || stalk == 1 || clump % 3 == 0) {
                auto head = [&](float side, float y) {
                    return Vec3{top.x + tx * side, y, top.z + tz * side};
                };
                // Espiga cilíndrica aveludada de taboa
                reedBc.addPoly(*this, {head(0.0f, top.y - 43.0f), head(-7.0f, top.y - 38.0f),
                                       head(-8.0f, top.y - 10.0f), head(-5.0f, top.y - 3.0f),
                                       head(5.0f, top.y - 3.0f), head(8.0f, top.y - 10.0f),
                                       head(7.0f, top.y - 38.0f)},
                               hex((clump + stalk) % 2 == 0 ? 0x99502c : 0x784324),
                               hex(0x563b29));
                reedBc.addLine(*this, head(-3.0f, top.y - 32.0f), head(-3.0f, top.y - 11.0f),
                               hex(0xc37a3e), 2.0f);
            }
        }
    }

    waterBc.flush(queue_);
    bankBc.flush(queue_);
    padBc.flush(queue_);
    vegetationBc.flush(queue_);
    reedBc.flush(queue_);

    // Vaga-lumes pulsantes agrupados num único lote de triângulos
    TriangleDraw fireflies;
    fireflies.texture = nullptr;
    for (int i = 0; i < 42; ++i) {
        const float a = i * 2.399963f;
        const float r = 800.0f + static_cast<float>((i * 137) % 650);
        const float y = 115.0f + static_cast<float>((i * 73) % 300)
                      + 9.0f * std::sin(frameCounter_ * 0.045f + i);
        const auto p = camera_.project(polar(a, r, y));
        if (!p || p->pos.x < 0.0f || p->pos.x > kLogicalWidth ||
            p->pos.y < 0.0f || p->pos.y > kLogicalHeight) continue;
        const float size = (i % 5 == 0 ? 2.5f : 1.4f) *
                           (0.7f + 0.3f * std::sin(frameCounter_ * 0.08f + i * 2.1f));
        const sf::Color glow = i % 4 == 0 ? hex(0xf2e596) : hex(0xc7e894);
        const float x = p->pos.x, py = p->pos.y;
        for (const sf::Vector2f v : std::array<sf::Vector2f, 6>{{{x, py - size}, {x + size, py}, {x, py + size},
                                                                 {x, py - size}, {x, py + size}, {x - size, py}}})
            fireflies.vertices.push_back({v, glow});
    }
    if (!fireflies.vertices.empty()) queue_.push_back({0.18f, 0.0f, std::move(fireflies)});
}

void RingRenderer::queueConcertHall() {
    // Teatro circular da Maestra: o salão, os vitrais e as cortinas pertencem
    // ao espaço 3D, não ao HUD. A zona de combate e suas colisões não mudam.
    auto polar = [](float a, float r, float y) {
        return Vec3{std::sin(a) * r, y, std::cos(a) * r};
    };
    constexpr int bays = 24;
    constexpr float step = 2.0f * kPi / bays;
    const std::array<uint32_t, 4> seasons = {0x8bbf91, 0xf6ca76, 0xd87968, 0x91c9d8};

    BatchCollector bcFloor(0.077f);
    BatchCollector bcLightSpots(0.078f);
    BatchCollector bcWalls(0.10f);
    BatchCollector bcDrapes(0.115f);
    BatchCollector bcWindows(0.125f);
    BatchCollector bcColumns(0.131f);
    BatchCollector bcOrgan(0.136f);
    BatchCollector bcBalcony(0.190f);
    BatchCollector bcFootlights(0.192f);

    // Madeira circular fora do quadrado jogável com juntas de parquete e manchas de vitral
    for (int i = 0; i < bays; ++i) {
        const float a = i * step, b = (i + 1) * step;
        // Tabulado de madeira nobre do palco
        bcFloor.addPoly(*this, {polar(a, 710.0f, -38.0f), polar(b, 710.0f, -38.0f),
                               polar(b, 1390.0f, -38.0f), polar(a, 1390.0f, -38.0f)},
                        hex(i % 2 ? 0x392634 : 0x462c38));
        // Juntas radiais e anéis concêntricos do parquete
        bcFloor.addLine(*this, polar(a, 710.0f, -37.9f), polar(a, 1390.0f, -37.9f), hex(0x271926), 2.5f);
        bcFloor.addLine(*this, polar(a, 1050.0f, -37.9f), polar(b, 1050.0f, -37.9f), hex(0x271926), 2.0f);

        // Rodapé de jacarandá e parede base de veludo
        bcWalls.addPoly(*this, {polar(a, 1300.0f, -37.0f), polar(b, 1300.0f, -37.0f),
                                polar(b, 1300.0f, 530.0f), polar(a, 1300.0f, 530.0f)},
                        hex(i % 2 ? 0x321b2d : 0x422033));

        // Cortinas de veludo com dois tons (luz e sombra em pregas facetadas de quadrinho)
        const float midA = (a + b) * 0.5f;
        const float a0 = a + 0.012f, a1 = midA, a2 = b - 0.012f;
        // Prega lado luz
        bcDrapes.addPoly(*this, {polar(a0, 1297.0f, 30.0f), polar(a1, 1297.0f, 30.0f),
                                 polar(a1, 1297.0f, 335.0f), polar(a0, 1297.0f, 335.0f)},
                         hex(i % 2 ? 0x7e354c : 0x8e3f53));
        // Prega lado sombra
        bcDrapes.addPoly(*this, {polar(a1, 1297.0f, 30.0f), polar(a2, 1297.0f, 30.0f),
                                 polar(a2, 1297.0f, 335.0f), polar(a1, 1297.0f, 335.0f)},
                         hex(i % 2 ? 0x5a2336 : 0x6a2b3e));

        // Pregas verticais profundas
        for (int fold = 1; fold <= 3; ++fold) {
            const float fa = a + (b - a) * fold / 4.0f;
            bcDrapes.addLine(*this, polar(fa, 1292.0f, 35.0f), polar(fa, 1292.0f, 328.0f),
                             hex(0x3e1927), 6.5f);
            // Filete sutil de brilho no topo da crista da prega
            bcDrapes.addLine(*this, polar(fa + 0.008f, 1291.0f, 40.0f), polar(fa + 0.008f, 1291.0f, 325.0f),
                             hex(0xa94b63), 2.0f);
        }

        // Lambrequim recortado (valance) de veludo com franja dourada no topo
        bcDrapes.addPoly(*this, {polar(a, 1293.0f, 326.0f), polar(b, 1293.0f, 326.0f),
                                 polar(midA, 1293.0f, 306.0f)},
                         hex(0x6b2438));
        bcDrapes.addLine(*this, polar(a, 1293.0f, 326.0f), polar(midA, 1293.0f, 306.0f), hex(0xd5a96e), 3.5f);
        bcDrapes.addLine(*this, polar(midA, 1293.0f, 306.0f), polar(b, 1293.0f, 326.0f), hex(0xd5a96e), 3.5f);

        // Cimalhas e frisos dourados da sala sinfônica
        bcDrapes.addLine(*this, polar(a, 1290.0f, 351.0f), polar(b, 1290.0f, 351.0f),
                         hex(0xc99a61), 8.0f);
        bcDrapes.addLine(*this, polar(a, 1290.0f, 376.0f), polar(b, 1290.0f, 376.0f),
                         hex(0x9b6b4c), 3.0f);
        bcDrapes.addLine(*this, polar(a, 1285.0f, -37.0f), polar(b, 1285.0f, -37.0f),
                         hex(0x9b6b4c), 9.0f);

        if (i % 3 == 1) {
            // Vitrais largos: quatro cores sazonais repetidas no salão.
            const float left = a + step * 0.19f, right = b - step * 0.19f;
            const uint32_t light = seasons[static_cast<std::size_t>((i / 3) % 4)];
            bcWindows.addPoly(*this, {polar(left, 1287.0f, 142.0f), polar(right, 1287.0f, 142.0f),
                                     polar(right, 1287.0f, 315.0f), polar(left, 1287.0f, 315.0f)},
                              hex(light), hex(0xd5a96e));
            bcWindows.addLine(*this, polar(left, 1283.0f, 226.0f), polar(right, 1283.0f, 226.0f),
                              hex(0x67445d), 5.0f);
            bcWindows.addLine(*this, polar((left + right) * 0.5f, 1283.0f, 143.0f),
                              polar((left + right) * 0.5f, 1283.0f, 314.0f),
                              hex(0x67445d), 5.0f);
            bcWindows.addPoly(*this, {polar(left, 1282.0f, 316.0f), polar((left + right) * 0.5f, 1282.0f, 357.0f),
                                     polar(right, 1282.0f, 316.0f)}, hex(light), hex(0xd5a96e));

            // Manchas de luz do vitral no chão de madeira do palco (fora do ringue de combate)
            const uint8_t lr = static_cast<uint8_t>((light >> 16) & 0xff);
            const uint8_t lg = static_cast<uint8_t>((light >> 8) & 0xff);
            const uint8_t lb = static_cast<uint8_t>(light & 0xff);
            const float wIn = step * 0.28f;
            const float wOut = step * 0.42f;
            const sf::Color spotDiffuse(lr, lg, lb, 65);
            const sf::Color spotCore(static_cast<uint8_t>(std::min(255, lr + 50)),
                                     static_cast<uint8_t>(std::min(255, lg + 50)),
                                     static_cast<uint8_t>(std::min(255, lb + 50)), 95);
            // Mancha trapezoidal suave de luz colorida
            bcLightSpots.addPoly(*this, {polar(midA - wIn, 770.0f, -37.6f),
                                         polar(midA + wIn, 770.0f, -37.6f),
                                         polar(midA + wOut, 1180.0f, -37.6f),
                                         polar(midA - wOut, 1180.0f, -37.6f)},
                                 spotDiffuse);
            // Núcleo losangular mais iluminado no centro da mancha
            bcLightSpots.addPoly(*this, {polar(midA, 820.0f, -37.5f),
                                         polar(midA + wIn * 0.55f, 960.0f, -37.5f),
                                         polar(midA, 1110.0f, -37.5f),
                                         polar(midA - wIn * 0.55f, 960.0f, -37.5f)},
                                 spotCore);
        }
    }

    // Colunata dourada canelada e capitéis clássicos
    for (int i = 0; i < 12; ++i) {
        const float a = i * 2.0f * kPi / 12.0f;
        bcColumns.addLine(*this, polar(a, 1260.0f, -35.0f), polar(a, 1260.0f, 470.0f),
                          hex(0x3d2939), 51.0f);
        bcColumns.addLine(*this, polar(a, 1257.0f, -35.0f), polar(a, 1257.0f, 470.0f),
                          hex(0xb98a61), 17.0f);
        bcColumns.addLine(*this, polar(a, 1252.0f, 18.0f), polar(a, 1252.0f, 420.0f),
                          hex(0xe5c28a), 3.0f);
        // Friso canelado de reflexo
        bcColumns.addLine(*this, polar(a + 0.005f, 1252.0f, 18.0f), polar(a + 0.005f, 1252.0f, 420.0f),
                          hex(0xfff1d6), 1.8f);
        // Capitel clássico esculpido no topo da coluna
        bcColumns.addLine(*this, polar(a, 1250.0f, 470.0f), polar(a, 1250.0f, 505.0f),
                          hex(0xd7ad70), 62.0f);
        bcColumns.addLine(*this, polar(a, 1248.0f, 470.0f), polar(a, 1248.0f, 478.0f),
                          hex(0xf2cf94), 70.0f);
        bcColumns.addLine(*this, polar(a, 1248.0f, 498.0f), polar(a, 1248.0f, 505.0f),
                          hex(0xf2cf94), 68.0f);
    }

    // Quatro fachadas do Grande Órgão com tubos metálicos polidos, labium e pedestal nobre
    for (int group = 0; group < 4; ++group) {
        const float a = group * kPi * 0.5f;
        const float tx = std::cos(a), tz = -std::sin(a);
        const Vec3 center = polar(a, 1250.0f, 0.0f);

        // Pedestal e console de jacarandá do órgão
        const Vec3 pL{center.x - tx * 115.0f, -36.0f, center.z - tz * 115.0f};
        const Vec3 pR{center.x + tx * 115.0f, -36.0f, center.z + tz * 115.0f};
        const Vec3 cL{center.x - tx * 115.0f, 92.0f, center.z - tz * 115.0f};
        const Vec3 cR{center.x + tx * 115.0f, 92.0f, center.z + tz * 115.0f};
        bcOrgan.addPoly(*this, {pL, pR, cR, cL}, hex(0x2d1825), hex(0x8a5933));
        // Frisos de latão dourado no pedestal
        bcOrgan.addLine(*this, {center.x - tx * 112.0f, 15.0f, center.z - tz * 112.0f},
                        {center.x + tx * 112.0f, 15.0f, center.z + tz * 112.0f}, hex(0xc99a61), 5.0f);
        bcOrgan.addLine(*this, {center.x - tx * 112.0f, 85.0f, center.z - tz * 112.0f},
                        {center.x + tx * 112.0f, 85.0f, center.z + tz * 112.0f}, hex(0xdfba7c), 6.5f);

        // Tubos de órgão
        for (int pipe = -3; pipe <= 3; ++pipe) {
            const float offset = pipe * 28.0f;
            const float height = 155.0f + (3 - std::abs(pipe)) * 26.0f;
            const Vec3 foot{center.x + tx * offset, 92.0f, center.z + tz * offset};
            const Vec3 crown{foot.x, height + 92.0f, foot.z};

            // Contorno do tubo
            bcOrgan.addLine(*this, foot, crown, hex(0x423040), 23.0f);
            // Corpo metálico cilíndrico
            bcOrgan.addLine(*this, {foot.x, foot.y + 10.0f, foot.z},
                            {crown.x, crown.y - 7.0f, crown.z},
                            hex(pipe % 2 == 0 ? 0xdcc4a0 : 0xbca8a8), 15.0f);
            // Especular vertical longo (reflexo comic no metal polido)
            bcOrgan.addLine(*this, {foot.x + tx * 2.0f, foot.y + 36.0f, foot.z + tz * 2.0f},
                            {crown.x + tx * 2.0f, crown.y - 12.0f, crown.z + tz * 2.0f},
                            hex(0xfffaea), 3.5f);
            // Labium (boca chanfrada do tubo de órgão)
            const float lipY = foot.y + 30.0f;
            bcOrgan.addLine(*this, {foot.x, lipY, foot.z}, {foot.x, lipY + 3.0f, foot.z},
                            hex(0x221320), 14.0f);
            bcOrgan.addLine(*this, {foot.x, lipY - 3.0f, foot.z}, {foot.x, lipY - 1.0f, foot.z},
                            hex(0xf5d398), 15.0f);
            // Pé cônico de latão dourado
            bcOrgan.addLine(*this, {foot.x, foot.y + 12.0f, foot.z},
                            {foot.x, foot.y + 24.0f, foot.z},
                            hex(0xd9a66f), 18.0f);
            // Glint dourado na coroa superior chanfrada do tubo
            bcOrgan.addLine(*this, {crown.x, crown.y - 6.0f, crown.z},
                            {crown.x, crown.y, crown.z},
                            hex(0xffeec9), 12.0f);
        }
    }

    // Balcões com balaústres entalhados
    for (int i = 0; i < 32; ++i) {
        const float a = i * 2.0f * kPi / 32.0f;
        const float b = (i + 1) * 2.0f * kPi / 32.0f;
        bcBalcony.addLine(*this, polar(a, 840.0f, 175.0f), polar(b, 840.0f, 175.0f),
                          hex(0xd8aa70), 8.0f);
        bcBalcony.addLine(*this, polar(a, 840.0f, 205.0f), polar(b, 840.0f, 205.0f),
                          hex(0x7c5260), 4.0f);
        // Balaústres elegantes
        bcBalcony.addLine(*this, polar(a, 840.0f, 175.0f), polar(a, 840.0f, 205.0f),
                          hex(0xb88160), 5.0f);
        const float midA = (a + b) * 0.5f;
        bcBalcony.addLine(*this, polar(midA, 840.0f, 178.0f), polar(midA, 840.0f, 202.0f),
                          hex(0xa26d52), 3.5f);
    }

    // Ribalta do palco com luminárias de latão e pulso de luz quente
    const float pulse = 0.83f + 0.17f * std::sin(frameCounter_ * 0.052f);
    const sf::Color glow(static_cast<uint8_t>(246 * pulse),
                         static_cast<uint8_t>(202 * pulse),
                         static_cast<uint8_t>(139 * pulse));
    for (int i = 0; i < 20; ++i) {
        const float a = (i + 0.5f) * 2.0f * kPi / 20.0f;
        // Concha de latão da luminária
        bcFootlights.addLine(*this, polar(a, 815.0f, 126.0f), polar(a, 815.0f, 134.0f),
                             hex(0x8c5b36), 14.0f);
        // Filamento radiante
        bcFootlights.addLine(*this, polar(a, 815.0f, 130.0f), polar(a, 815.0f, 145.0f),
                             glow, 12.0f);
    }

    // Submissão unificada por lote
    bcFloor.flush(queue_);
    bcLightSpots.flush(queue_);
    bcWalls.flush(queue_);
    bcDrapes.flush(queue_);
    bcWindows.flush(queue_);
    bcColumns.flush(queue_);
    bcOrgan.flush(queue_);
    bcBalcony.flush(queue_);
    bcFootlights.flush(queue_);
}

void RingRenderer::queueCentral(const GameEngine& engine) {
    // Arena Central (Coliseu Industrial do Batalhator 2000):
    // Estrutura de aço pesado, vigas com treliças em X e rebites,
    // mega-telões industriais suspensos com telemetria que responde às 3 fases do chefe,
    // sinalizadores e refletores industriais cel-shaded.
    auto polar = [](float a, float r, float y) {
        return Vec3{std::sin(a) * r, y, std::cos(a) * r};
    };

    const int stage = (engine.enemy.charId == 8) ? engine.enemy.bossStage
                     : (engine.player.charId == 8 ? engine.player.bossStage : 1);

    // Cores temáticas por fase do Batalhator 2000:
    // Fase 1: Âmbar/Laranja Industrial (Titânio e Serras)
    // Fase 2: Ciano/Azul Elétrico (Dínamo e PEM)
    // Fase 3: Carmesim/Vermelho Alerta Crítico (Matriz Quântica e Laser)
    const sf::Color screenBg = (stage >= 3) ? hex(0x220b16)
                             : (stage == 2) ? hex(0x0a1c28) : hex(0x1c150c);
    const sf::Color screenGrid = (stage >= 3) ? hex(0x991b3b)
                               : (stage == 2) ? hex(0x0e7490) : hex(0xb45309);
    const sf::Color screenGlow = (stage >= 3) ? hex(0xf43f5e)
                               : (stage == 2) ? hex(0x22d3ee) : hex(0xfbbf24);
    const sf::Color beaconCol = (stage >= 3) ? hex(0xff2244)
                              : (stage == 2) ? hex(0x00f0ff) : hex(0xffaa00);

    BatchCollector bcWalls(0.080f);
    BatchCollector bcGirders(0.110f);
    BatchCollector bcScreens(0.140f);
    BatchCollector bcMonitors(0.150f);
    BatchCollector bcLights(0.170f);

    constexpr int bays = 16;
    constexpr float step = 2.0f * kPi / bays;

    // 1. Paredes industriais circulares e vigas mestras
    for (int i = 0; i < bays; ++i) {
        const float a = i * step, b = (i + 1) * step;

        // Placas de blindagem de aço fosco na muralha do perímetro
        bcWalls.addPoly(*this, {polar(a, 1380.0f, -38.0f), polar(b, 1380.0f, -38.0f),
                               polar(b, 1380.0f, 490.0f), polar(a, 1380.0f, 490.0f)},
                        hex(i % 2 ? 0x141b24 : 0x18222e), hex(0x0b1017));

        // Frisos metálicos horizontais na muralha
        bcWalls.addLine(*this, polar(a, 1375.0f, 180.0f), polar(b, 1375.0f, 180.0f), hex(0x253346), 4.0f);
        bcWalls.addLine(*this, polar(a, 1375.0f, 360.0f), polar(b, 1375.0f, 360.0f), hex(0x253346), 4.0f);

        // Piso de serviço externo além das arquibancadas
        bcWalls.addPoly(*this, {polar(a, 730.0f, -38.0f), polar(b, 730.0f, -38.0f),
                               polar(b, 1380.0f, -38.0f), polar(a, 1380.0f, -38.0f)},
                        hex(i % 2 ? 0x10151f : 0x131a26));
        bcWalls.addLine(*this, polar(a, 730.0f, -37.8f), polar(a, 1380.0f, -37.8f), hex(0x0c1119), 2.5f);

        // Pilares colossais em H de sustentação
        const Vec3 basePillar = polar(a, 1360.0f, -37.0f);
        const Vec3 topPillar = polar(a, 1360.0f, 500.0f);
        bcGirders.addLine(*this, basePillar, topPillar, hex(0x0d121a), 36.0f);
        bcGirders.addLine(*this, basePillar, topPillar, hex(0x28364a), 26.0f);
        bcGirders.addLine(*this, polar(a, 1358.0f, -37.0f), polar(a, 1358.0f, 500.0f), hex(0x475973), 3.0f);

        // Treliças diagonais industriais em X entre pilares
        bcGirders.addLine(*this, polar(a, 1365.0f, 180.0f), polar(b, 1365.0f, 360.0f), hex(0x1e2838), 6.0f);
        bcGirders.addLine(*this, polar(a, 1365.0f, 360.0f), polar(b, 1365.0f, 180.0f), hex(0x1e2838), 6.0f);
        bcGirders.addLine(*this, polar(a, 1365.0f, 360.0f), polar(b, 1365.0f, 480.0f), hex(0x1e2838), 6.0f);
        bcGirders.addLine(*this, polar(a, 1365.0f, 480.0f), polar(b, 1365.0f, 360.0f), hex(0x1e2838), 6.0f);

        // Faixa zebrada de segurança industrial (hazard tape amarela/preta) na passarela superior
        if (i % 2 == 0) {
            bcGirders.addLine(*this, polar(a, 1350.0f, 182.0f), polar(b, 1350.0f, 182.0f), hex(0xd97706), 5.0f);
            for (float fz = 0.1f; fz < 0.95f; fz += 0.2f) {
                const float az = a + step * fz;
                bcGirders.addLine(*this, polar(az, 1348.0f, 179.0f), polar(az + step * 0.08f, 1348.0f, 185.0f), hex(0x111827), 4.0f);
            }
        }
    }

    // 2. Quatro Telões Gigantes (Jumbotrons) nos eixos do estádio
    const float rightX = std::cos(camera_.yaw), rightZ = -std::sin(camera_.yaw);
    const float animTime = static_cast<float>(frameCounter_) * 0.12f;
    const float beaconPulse = 0.65f + 0.35f * std::sin(static_cast<float>(frameCounter_) * 0.25f);

    for (int sc = 0; sc < 4; ++sc) {
        const float angle = sc * (kPi * 0.5f);
        const Vec3 screenCenter = polar(angle, 1220.0f, 310.0f);

        // Haste/cabo de sustentação do telão descendo do teto
        bcGirders.addLine(*this, {screenCenter.x - 70.0f * rightX, 480.0f, screenCenter.z - 70.0f * rightZ},
                          {screenCenter.x - 70.0f * rightX, screenCenter.y + 65.0f, screenCenter.z - 70.0f * rightZ},
                          hex(0x334155), 4.0f);
        bcGirders.addLine(*this, {screenCenter.x + 70.0f * rightX, 480.0f, screenCenter.z + 70.0f * rightZ},
                          {screenCenter.x + 70.0f * rightX, screenCenter.y + 65.0f, screenCenter.z + 70.0f * rightZ},
                          hex(0x334155), 4.0f);

        auto spoint = [&](float ox, float oy) {
            return Vec3{screenCenter.x + rightX * ox, screenCenter.y + oy, screenCenter.z + rightZ * ox};
        };

        // Moldura externa chanfrada do telão
        bcScreens.addPoly(*this, {spoint(-102.0f, -62.0f), spoint(102.0f, -62.0f),
                                 spoint(102.0f, 62.0f), spoint(-102.0f, 62.0f)},
                          hex(0x0f172a), hex(0x334155));

        // Superfície luminosa do monitor
        bcMonitors.addPoly(*this, {spoint(-94.0f, -54.0f), spoint(94.0f, -54.0f),
                                  spoint(94.0f, 54.0f), spoint(-94.0f, 54.0f)},
                           screenBg, hex(0x030712));

        // Grade de varredura (scanlines / matrix grid)
        for (float gy = -40.0f; gy <= 40.0f; gy += 20.0f) {
            bcMonitors.addLine(*this, spoint(-90.0f, gy), spoint(90.0f, gy), screenGrid, 1.5f);
        }

        // Gráfico de onda / osciloscópio animado no centro do telão
        for (int pt = 0; pt < 7; ++pt) {
            const float px0 = -80.0f + pt * 23.0f;
            const float px1 = px0 + 23.0f;
            const float py0 = std::sin(animTime + pt * 1.1f + sc) * 18.0f;
            const float py1 = std::sin(animTime + (pt + 1) * 1.1f + sc) * 18.0f;
            bcMonitors.addLine(*this, spoint(px0, py0), spoint(px1, py1), screenGlow, 2.8f);
        }

        // Barra de status / telemetria digital inferior
        bcMonitors.addPoly(*this, {spoint(-86.0f, -44.0f), spoint(86.0f, -44.0f),
                                  spoint(86.0f, -36.0f), spoint(-86.0f, -36.0f)},
                           hex(0x0f172a), screenGrid);
        const float fillW = -84.0f + 168.0f * (stage >= 3 ? 0.95f : stage == 2 ? 0.65f : 0.35f);
        bcMonitors.addPoly(*this, {spoint(-84.0f, -43.0f), spoint(fillW, -43.0f),
                                  spoint(fillW, -37.0f), spoint(-84.0f, -37.0f)},
                           screenGlow);

        // Luz de alerta / sinalizador estroboscópico piscando no topo do telão
        const sf::Color activeBeacon(static_cast<uint8_t>(beaconCol.r * beaconPulse),
                                     static_cast<uint8_t>(beaconCol.g * beaconPulse),
                                     static_cast<uint8_t>(beaconCol.b * beaconPulse));
        bcLights.addLine(*this, spoint(-30.0f, 65.0f), spoint(-30.0f, 75.0f), activeBeacon, 9.0f);
        bcLights.addLine(*this, spoint(30.0f, 65.0f), spoint(30.0f, 75.0f), activeBeacon, 9.0f);
        bcLights.addLine(*this, spoint(-30.0f, 68.0f), spoint(-30.0f, 73.0f), hex(0xffffff), 4.0f);
        bcLights.addLine(*this, spoint(30.0f, 68.0f), spoint(30.0f, 73.0f), hex(0xffffff), 4.0f);
    }

    // 3. Bancadas de Refletores Industriais Angulados
    for (int i = 0; i < 8; ++i) {
        const float a = (i + 0.5f) * 2.0f * kPi / 8.0f;
        const Vec3 lampWorld = polar(a, 940.0f, 385.0f);

        // Suporte de montagem em treliça
        bcLights.addLine(*this, polar(a, 960.0f, 440.0f), lampWorld, hex(0x1e293b), 5.0f);

        // Chassi dos refletores (três refletores industriais quadrados)
        const Vec3 lampL{lampWorld.x - 14.0f * rightX, lampWorld.y, lampWorld.z - 14.0f * rightZ};
        const Vec3 lampR{lampWorld.x + 14.0f * rightX, lampWorld.y, lampWorld.z + 14.0f * rightZ};
        bcLights.addLine(*this, lampL, lampR, hex(0x0f172a), 11.0f);
        bcLights.addLine(*this, lampL, lampR, hex(0x475569), 3.0f);

        // Emissores de luz com brilho cel-shaded
        const sf::Color lampGlow = (stage >= 3) ? hex(0xfecdd3) : hex(0xfef08a);
        bcLights.addLine(*this, {lampWorld.x, lampWorld.y - 4.0f, lampWorld.z},
                         {lampWorld.x, lampWorld.y + 4.0f, lampWorld.z}, lampGlow, 8.0f);
    }

    // Flush dos lotes agrupados
    bcWalls.flush(queue_);
    bcGirders.flush(queue_);
    bcScreens.flush(queue_);
    bcMonitors.flush(queue_);
    bcLights.flush(queue_);
}

void RingRenderer::queueParlor() {
    // Sorveteria vintage do Cremoso: vitrine com brilho de vidro cel-shaded,
    // balcão anular com sombras de contato no piso de parquet, rodapés de chocolate,
    // frisos dourados e toldo parisiense festivo.
    auto polar = [](float a, float r, float y) {
        return Vec3{std::sin(a) * r, y, std::cos(a) * r};
    };
    constexpr int bays = 24;
    constexpr float step = 2.0f * kPi / bays;
    const std::array<uint32_t, 4> flavor = {0xf3d394, 0xe9879c, 0xa4d6ba, 0xb3b9e9};

    BatchCollector bcGround(0.076f);
    BatchCollector bcWall(0.10f);
    BatchCollector bcPanels(0.115f);
    BatchCollector bcAwning(0.140f);
    BatchCollector bcDisplays(0.152f);
    BatchCollector bcCounter(0.165f);
    BatchCollector bcVitrine(0.178f);

    // 1. Piso de Parquet Creme e Caramelo com Sombras de Contato
    for (int i = 0; i < bays; ++i) {
        const float a = i * step, b = (i + 1) * step;
        const float midA = (a + b) * 0.5f;

        // Ladrilhos radiais em creme suave e doce de leite / caramelo
        bcGround.addPoly(*this, {polar(a, 700.0f, -38.0f), polar(b, 700.0f, -38.0f),
                                polar(b, 1450.0f, -38.0f), polar(a, 1450.0f, -38.0f)},
                         hex(i % 2 ? 0xe2b88e : 0xf0c9a0));

        // Juntas finas de rejunte de confeitaria
        bcGround.addLine(*this, polar(a, 700.0f, -37.9f), polar(a, 1450.0f, -37.9f), hex(0xb0805c), 2.0f);
        bcGround.addLine(*this, polar(a, 1050.0f, -37.9f), polar(b, 1050.0f, -37.9f), hex(0xb0805c), 1.8f);

        // Parede creme pastel com iluminação suave
        bcWall.addPoly(*this, {polar(a, 1320.0f, -37.0f), polar(b, 1320.0f, -37.0f),
                              polar(b, 1320.0f, 460.0f), polar(a, 1320.0f, 460.0f)},
                       hex(i % 2 ? 0xf7d7b1 : 0xf1cba8));

        // Lambril / Boiserie de madeira chocolate
        bcPanels.addPoly(*this, {polar(a, 1315.0f, -37.0f), polar(b, 1315.0f, -37.0f),
                                polar(b, 1315.0f, 105.0f), polar(a, 1315.0f, 105.0f)},
                         hex(i % 2 ? 0x84545b : 0x754854));

        // Friso entalhado central nos painéis de chocolate
        bcPanels.addPoly(*this, {polar(a + step * 0.15f, 1313.0f, -15.0f),
                                polar(b - step * 0.15f, 1313.0f, -15.0f),
                                polar(b - step * 0.15f, 1313.0f, 85.0f),
                                polar(a + step * 0.15f, 1313.0f, 85.0f)},
                         hex(i % 2 ? 0x724249 : 0x65373e), hex(0x995c64));

        // Rodapé escuro na base da parede
        bcPanels.addLine(*this, polar(a, 1314.0f, -35.0f), polar(b, 1314.0f, -35.0f), hex(0x48242c), 4.0f);

        // Moldura turquesa estilo Art Déco acima do lambril
        bcPanels.addLine(*this, polar(a, 1310.0f, 104.0f), polar(b, 1310.0f, 104.0f), hex(0x54a99f), 6.0f);
        bcPanels.addLine(*this, polar(a, 1310.0f, 108.0f), polar(b, 1310.0f, 108.0f), hex(0x7ecbc2), 2.0f);

        // Cornija superior rosa vintage no teto
        bcPanels.addLine(*this, polar(a, 1310.0f, 445.0f), polar(b, 1310.0f, 445.0f), hex(0xb26775), 6.0f);
        bcPanels.addLine(*this, polar(a, 1310.0f, 448.0f), polar(b, 1310.0f, 448.0f), hex(0xd48795), 2.5f);

        // Cartela decorativa pastel nos módulos pares da parede
        if (i % 2 == 0) {
            bcWall.addPoly(*this, {polar(midA - step * 0.28f, 1318.0f, 150.0f),
                                  polar(midA + step * 0.28f, 1318.0f, 150.0f),
                                  polar(midA + step * 0.28f, 1318.0f, 250.0f),
                                  polar(midA - step * 0.28f, 1318.0f, 250.0f)},
                           sf::Color(255, 245, 230, 90), hex(0xd9a87d));
        }
    }

    // 2. Balcão Anular de Sorvetes (Raio 595 - 710) com Sombras de Contato e Vitrine
    for (int i = 0; i < 32; ++i) {
        const float a = i * 2.0f * kPi / 32.0f;
        const float b = (i + 1) * 2.0f * kPi / 32.0f;
        const float midA = (a + b) * 0.5f;

        // Sombra de contato do balcão projetada no chão
        bcGround.addPoly(*this, {polar(a, 578.0f, -37.5f), polar(b, 578.0f, -37.5f),
                                polar(b, 597.0f, -37.5f), polar(a, 597.0f, -37.5f)},
                         sf::Color(35, 18, 22, 135));

        // Frente do balcão: painéis verticais listrados (framboesa e chantilly)
        bcCounter.addPoly(*this, {polar(a, 595.0f, -35.0f), polar(b, 595.0f, -35.0f),
                                 polar(b, 595.0f, 68.0f), polar(a, 595.0f, 68.0f)},
                          hex(i % 2 ? 0xb86672 : 0xf1d9bd));

        // Filete de contorno entre as frentes
        bcCounter.addLine(*this, polar(a, 594.0f, -35.0f), polar(a, 594.0f, 68.0f), hex(0x753a44), 2.0f);

        // Tampo de mármore creme do balcão
        bcCounter.addPoly(*this, {polar(a, 595.0f, 68.0f), polar(b, 595.0f, 68.0f),
                                 polar(b, 710.0f, 68.0f), polar(a, 710.0f, 68.0f)},
                          hex(i % 2 ? 0xeacb9f : 0xf9dfb8));

        // Borda frontal em latão/turquesa
        bcCounter.addLine(*this, polar(a, 590.0f, 68.0f), polar(b, 590.0f, 68.0f), hex(0x5aa99c), 5.5f);
        bcCounter.addLine(*this, polar(a, 590.0f, 70.0f), polar(b, 590.0f, 70.0f), hex(0x82c8bd), 2.0f);

        // Vitrine de vidro cel-shaded sobre o balcão (guarda de vidro dos sorvetes)
        // Vidro translúcido com brilho de borda
        bcVitrine.addPoly(*this, {polar(a, 608.0f, 71.0f), polar(b, 608.0f, 71.0f),
                                 polar(b, 608.0f, 96.0f), polar(a, 608.0f, 96.0f)},
                          sf::Color(220, 245, 255, 65));

        // Armação cromada/latão no topo do vidro
        bcVitrine.addLine(*this, polar(a, 608.0f, 96.0f), polar(b, 608.0f, 96.0f), hex(0xbddcd6), 2.5f);

        // Brilhos e reflexos diagonais de vidro em quadrinhos
        if (i % 3 == 0) {
            bcVitrine.addLine(*this, polar(a + 0.02f, 607.0f, 74.0f),
                             polar(a + 0.08f, 607.0f, 93.0f), sf::Color(255, 255, 255, 140), 2.2f);
            bcVitrine.addLine(*this, polar(a + 0.05f, 607.0f, 74.0f),
                             polar(a + 0.09f, 607.0f, 87.0f), sf::Color(255, 255, 255, 110), 1.5f);
        }
    }

    // 3. Toldo Parisiense com Babado e Frisos Dourados
    for (int i = 0; i < 24; ++i) {
        const float a = i * step, b = (i + 1) * step;
        const sf::Color stripe = hex(i % 2 ? 0xfbe7c7 : 0xdc8192);

        // Caimento do toldo
        bcAwning.addPoly(*this, {polar(a, 1298.0f, 362.0f), polar(b, 1298.0f, 362.0f),
                                polar(b, 1178.0f, 305.0f), polar(a, 1178.0f, 305.0f)},
                         stripe, hex(0x8f5565));

        // Babado pendente
        bcAwning.addPoly(*this, {polar(a, 1178.0f, 305.0f), polar(b, 1178.0f, 305.0f),
                                polar(b, 1178.0f, 276.0f), polar(a, 1178.0f, 276.0f)},
                         stripe);

        // Barra inferior e friso dourado no babado
        bcAwning.addLine(*this, polar(a, 1178.0f, 276.0f), polar(b, 1178.0f, 276.0f), hex(0x8f5565), 3.0f);
        bcAwning.addLine(*this, polar(a, 1178.0f, 280.0f), polar(b, 1178.0f, 280.0f), hex(0xe8c17b), 1.5f);
    }

    // 4. Cones e Luminárias de Sorvete Suspensos
    const float rightX = std::cos(camera_.yaw), rightZ = -std::sin(camera_.yaw);
    for (int i = 0; i < 8; ++i) {
        const float a = (i + 0.48f) * 2.0f * kPi / 8.0f;
        const Vec3 root = polar(a, 1015.0f, 0.0f);
        const uint32_t scoop = flavor[static_cast<std::size_t>(i % 4)];
        auto point = [&](float x, float y) {
            return Vec3{root.x + rightX * x, y, root.z + rightZ * x};
        };

        // Haste metálica / corrente dourada de sustentação conectada ao toldo/teto
        bcDisplays.addLine(*this, point(0.0f, 355.0f), point(0.0f, 440.0f), hex(0xb08b51), 2.2f);
        // Pequena cúpula/remate decorativo no topo da haste
        bcDisplays.addPoly(*this, {point(-8.0f, 355.0f), point(8.0f, 355.0f),
                                  point(6.0f, 363.0f), point(-6.0f, 363.0f)},
                           hex(0xd9b36c), hex(0x6b4f23));

        // Casquinha de waffle
        bcDisplays.addPoly(*this, {point(0.0f, 67.0f), point(-74.0f, 215.0f), point(74.0f, 215.0f)},
                           hex(0xc9834f), hex(0x5c3b3b));
        for (int waffle = -2; waffle <= 2; ++waffle) {
            const float y = 135.0f + waffle * 16.0f;
            const float span = (y - 67.0f) * 0.38f;
            bcDisplays.addLine(*this, point(-span, y), point(span, y), hex(0xe9ae70), 2.0f);
        }

        // Bola de sorvete volumosa
        bcDisplays.addPoly(*this, {point(-87.0f, 225.0f), point(-76.0f, 270.0f),
                                  point(-47.0f, 303.0f), point(0.0f, 314.0f),
                                  point(47.0f, 303.0f), point(76.0f, 270.0f),
                                  point(87.0f, 225.0f), point(54.0f, 201.0f),
                                  point(0.0f, 194.0f), point(-54.0f, 201.0f)},
                           hex(scoop), hex(0x5c3b4d));

        // Brilho cel-shaded na bola de sorvete
        bcDisplays.addPoly(*this, {point(-49.0f, 266.0f), point(-29.0f, 286.0f),
                                  point(-5.0f, 294.0f), point(25.0f, 288.0f)},
                           sf::Color(255, 255, 230, 115));

        // Cabinho da cereja
        bcDisplays.addLine(*this, point(0.0f, 313.0f), point(0.0f, 334.0f), hex(0x5b7e63), 3.0f);

        // Cereja brilhante
        bcDisplays.addPoly(*this, {point(-15.0f, 339.0f), point(-10.0f, 351.0f),
                                  point(8.0f, 352.0f), point(17.0f, 340.0f),
                                  point(9.0f, 330.0f), point(-10.0f, 330.0f)},
                           hex(0xb94f61), hex(0x613949));
    }

    // Flush dos lotes agrupados
    bcGround.flush(queue_);
    bcWall.flush(queue_);
    bcPanels.flush(queue_);
    bcAwning.flush(queue_);
    bcDisplays.flush(queue_);
    bcCounter.flush(queue_);
    bcVitrine.flush(queue_);
}

void RingRenderer::queueEspinhaCourtyard() {
    // Pátio colonial de lucha da Lady Espinha: contraste quente/frio entre terracota e turquesa.
    // Adobe em blocos, nichos com azulejos talavera, mandacarus em vasos de barro com sombras de contato no piso.
    auto polar = [](float a, float r, float y) {
        return Vec3{std::sin(a) * r, y, std::cos(a) * r};
    };
    constexpr int bays = 24;
    constexpr float step = 2.0f * kPi / bays;
    const std::array<uint32_t, 4> flags = {0xe5ae5b, 0x49ada8, 0xc75b74, 0xf0d3a1};

    BatchCollector bcGround(0.076f);
    BatchCollector bcTiles(0.078f);
    BatchCollector bcWallBase(0.10f);
    BatchCollector bcNiches(0.120f);
    BatchCollector bcButtress(0.130f);
    BatchCollector bcBunting(0.145f);
    BatchCollector bcPots(0.150f);
    BatchCollector bcCactus(0.153f);

    // 1. Piso de lajotas de terracota e mosaicos de azulejos turquesa
    for (int i = 0; i < bays; ++i) {
        const float a = i * step, b = (i + 1) * step;
        const float midA = (a + b) * 0.5f;

        // Lajotas de terracota cozida
        bcGround.addPoly(*this, {polar(a, 700.0f, -38.0f), polar(b, 700.0f, -38.0f),
                                polar(b, 1500.0f, -38.0f), polar(a, 1500.0f, -38.0f)},
                         hex(i % 2 ? 0x884839 : 0x9a503c));

        // Juntas de argamassa escura entre as lajotas
        bcGround.addLine(*this, polar(a, 700.0f, -37.9f), polar(a, 1500.0f, -37.9f), hex(0x4a231b), 2.5f);
        bcGround.addLine(*this, polar(a, 860.0f, -37.9f), polar(b, 860.0f, -37.9f), hex(0x4a231b), 2.0f);

        // Mosaico de azulejos vidrados coloniais (turquesa contrastando com a terracota)
        if (i % 2 == 0) {
            bcTiles.addPoly(*this, {polar(a + step * 0.16f, 846.0f, -36.5f),
                                   polar(b - step * 0.16f, 846.0f, -36.5f),
                                   polar(b - step * 0.16f, 874.0f, -36.5f),
                                   polar(a + step * 0.16f, 874.0f, -36.5f)},
                            hex(0x227b82), hex(0xdba566));
            // Losango central no azulejo
            bcTiles.addPoly(*this, {polar(midA, 849.0f, -36.4f),
                                   polar(b - step * 0.28f, 860.0f, -36.4f),
                                   polar(midA, 871.0f, -36.4f),
                                   polar(a + step * 0.28f, 860.0f, -36.4f)},
                            hex(0x42b4ab), hex(0xf4dfa8));
        }

        // 2. Muro de adobe em blocos largos com base de tijolo colonial
        bcWallBase.addPoly(*this, {polar(a, 1340.0f, -38.0f), polar(b, 1340.0f, -38.0f),
                                  polar(b, 1340.0f, 390.0f), polar(a, 1340.0f, 390.0f)},
                           hex(i % 2 ? 0x97483d : 0xa95340));
        // Beiral de cumeeira em telhas cerâmicas curvas no topo do muro
        bcWallBase.addPoly(*this, {polar(a, 1334.0f, 388.0f), polar(b, 1334.0f, 388.0f),
                                  polar(b, 1330.0f, 404.0f), polar(a, 1330.0f, 404.0f)},
                           hex(0xb84b38), hex(0x56231c));
        bcWallBase.addLine(*this, polar(a, 1330.0f, 404.0f), polar(b, 1330.0f, 404.0f), hex(0xd9684c), 3.0f);

        // Linhas de bloco de adobe
        for (float y : {174.0f, 286.0f})
            bcWallBase.addLine(*this, polar(a, 1338.0f, y), polar(b, 1338.0f, y), hex(0x773d3d), 2.0f);
        if (i % 3 != 1) {
            bcWallBase.addLine(*this, polar(midA, 1337.0f, 177.0f), polar(midA, 1337.0f, 284.0f), hex(0x773d3d), 2.0f);
        }

        // Rodapé de tijolo queimado e cornija dourada
        bcWallBase.addPoly(*this, {polar(a, 1336.0f, -38.0f), polar(b, 1336.0f, -38.0f),
                                  polar(b, 1336.0f, 84.0f), polar(a, 1336.0f, 84.0f)},
                           hex(i % 2 ? 0x59323c : 0x693943));
        bcWallBase.addLine(*this, polar(a, 1330.0f, 88.0f), polar(b, 1330.0f, 88.0f), hex(0xc79059), 7.0f);
        bcWallBase.addLine(*this, polar(a, 1328.0f, 378.0f), polar(b, 1328.0f, 378.0f), hex(0xcf9a5a), 9.0f);

        // Ladrilhos ornamentais na cimalha
        bcWallBase.addPoly(*this, {polar(midA, 1326.0f, 366.0f),
                                  polar(b - step * 0.22f, 1326.0f, 343.0f),
                                  polar(midA, 1326.0f, 320.0f),
                                  polar(a + step * 0.22f, 1326.0f, 343.0f)},
                           hex(i % 2 ? 0x3e9b98 : 0xc77871), hex(0x593c47));

        // 3. Nichos arqueados coloniais com azulejos Talavera
        if (i % 3 == 1) {
            const float left = a + step * 0.15f, right = b - step * 0.15f;
            bcNiches.addPoly(*this, {polar(left, 1325.0f, 95.0f), polar(right, 1325.0f, 95.0f),
                                    polar(right, 1325.0f, 275.0f), polar(left, 1325.0f, 275.0f)},
                             hex(0x226a73), hex(0xdaa766));
            bcNiches.addPoly(*this, {polar(left, 1322.0f, 277.0f),
                                    polar((left + right) * 0.5f, 1322.0f, 322.0f),
                                    polar(right, 1322.0f, 277.0f)},
                             hex(0x226a73), hex(0xdaa766));
            bcNiches.addLine(*this, polar(left, 1320.0f, 151.0f), polar(right, 1320.0f, 151.0f),
                             hex(0x7fc6ae), 3.0f);
            // Roseta geométrica de azulejo no centro do nicho
            const float midN = (left + right) * 0.5f;
            bcNiches.addPoly(*this, {polar(midN, 1318.0f, 263.0f),
                                    polar(right - step * 0.10f, 1318.0f, 211.0f),
                                    polar(midN, 1318.0f, 159.0f),
                                    polar(left + step * 0.10f, 1318.0f, 211.0f)},
                             hex(0x73b9a8), hex(0x183f53));
            bcNiches.addPoly(*this, {polar(midN, 1316.0f, 243.0f),
                                    polar(right - step * 0.17f, 1316.0f, 211.0f),
                                    polar(midN, 1316.0f, 179.0f),
                                    polar(left + step * 0.17f, 1316.0f, 211.0f)},
                             hex(0xd9ac67));
        }
    }

    // 4. Contrafortes e tocheiros coloniais de ferro com chamas vivas
    const float torchPulse = 0.82f + 0.18f * std::sin(frameCounter_ * 0.08f);
    for (int i = 0; i < 8; ++i) {
        const float a = i * 2.0f * kPi / 8.0f;
        bcButtress.addLine(*this, polar(a, 1300.0f, -35.0f), polar(a, 1300.0f, 385.0f),
                           hex(0x63343c), 53.0f);
        bcButtress.addLine(*this, polar(a, 1295.0f, 16.0f), polar(a, 1295.0f, 367.0f),
                           hex(0xc48657), 16.0f);

        // Tocheiro de ferro fundido montado no contraforte
        const Vec3 tBase = polar(a, 1288.0f, 205.0f);
        const Vec3 tTip = polar(a, 1272.0f, 222.0f);
        bcButtress.addLine(*this, tBase, tTip, hex(0x26161c), 5.0f);
        // Cesto da tocha
        bcButtress.addLine(*this, {tTip.x, tTip.y - 4.0f, tTip.z}, {tTip.x, tTip.y + 8.0f, tTip.z},
                           hex(0x382028), 9.0f);
        // Fogo vivo ardente da tocha colonial
        const sf::Color flameCol(static_cast<uint8_t>(255 * torchPulse),
                                 static_cast<uint8_t>(178 * torchPulse),
                                 static_cast<uint8_t>(52 * torchPulse));
        bcButtress.addLine(*this, {tTip.x, tTip.y + 4.0f, tTip.z}, {tTip.x, tTip.y + 19.0f, tTip.z},
                           flameCol, 9.0f);
        bcButtress.addLine(*this, {tTip.x, tTip.y + 6.0f, tTip.z}, {tTip.x, tTip.y + 15.0f, tTip.z},
                           hex(0xfffaea), 4.5f);
    }

    // 5. Papel picado tradicional em cordões suspensos
    for (int i = 0; i < 32; ++i) {
        const float a = i * 2.0f * kPi / 32.0f;
        const float b = (i + 1) * 2.0f * kPi / 32.0f;
        const float middle = (a + b) * 0.5f;
        bcBunting.addLine(*this, polar(a, 1100.0f, 310.0f), polar(b, 1100.0f, 310.0f),
                          hex(0x684b59), 2.0f);
        bcBunting.addPoly(*this, {polar(a + 0.012f, 1095.0f, 305.0f),
                                 polar(b - 0.012f, 1095.0f, 305.0f),
                                 polar(middle, 1095.0f, 258.0f)},
                          hex(flags[static_cast<std::size_t>(i % 4)]),
                          hex(0x56394a));
    }

    // 6. Vasos de barro coloniais com sombras de contato e mandacarus floridos
    const float rightX = std::cos(camera_.yaw), rightZ = -std::sin(camera_.yaw);
    for (int i = 0; i < 10; ++i) {
        const float a = (i + 0.52f) * 2.0f * kPi / 10.0f;
        const Vec3 root = polar(a, 890.0f + static_cast<float>((i * 43) % 95), -35.0f);

        // Sombra de contato recortada sob o vaso de barro no chão
        for (int s = 0; s < 8; ++s) {
            const float sa = s * 2.0f * kPi / 8.0f;
            const float sb = (s + 1) * 2.0f * kPi / 8.0f;
            bcGround.addPoly(*this, {{root.x, -37.5f, root.z},
                                     {root.x + std::sin(sa) * 54.0f, -37.5f, root.z + std::cos(sa) * 45.0f},
                                     {root.x + std::sin(sb) * 54.0f, -37.5f, root.z + std::cos(sb) * 45.0f}},
                             hex(0x281412));
        }

        auto point = [&](float x, float y) {
            return Vec3{root.x + rightX * x, y, root.z + rightZ * x};
        };

        // Vaso de cerâmica terracota torneada
        bcPots.addPoly(*this, {point(-48.0f, -25.0f), point(48.0f, -25.0f),
                              point(39.0f, 36.0f), point(-39.0f, 36.0f)},
                       hex(0xb36c55), hex(0x693f48));
        // Bojo sombreado do vaso
        bcPots.addPoly(*this, {point(-46.0f, -23.0f), point(46.0f, -23.0f),
                              point(43.0f, -3.0f), point(-43.0f, -3.0f)},
                       hex(0x8a4635));
        // Borda rolada do vaso com brilho de cerâmica
        bcPots.addLine(*this, point(-45.0f, 38.0f), point(45.0f, 38.0f), hex(0xe3a56d), 11.0f);
        bcPots.addLine(*this, point(-43.0f, 40.0f), point(43.0f, 40.0f), hex(0xffdeb2), 3.0f);

        // Mandacaru: caule central canelado
        const float height = 232.0f + static_cast<float>((i * 29) % 52);
        bcCactus.addLine(*this, point(0.0f, 35.0f), point(0.0f, height), hex(0x244b49), 53.0f);
        bcCactus.addLine(*this, point(0.0f, 42.0f), point(0.0f, height - 3.0f),
                         hex(i % 2 ? 0x4f9a73 : 0x429373), 42.0f);

        // Braços laterais do cacto
        for (int side : {-1, 1}) {
            const float y = 112.0f + (side == 1 ? 36.0f : 0.0f);
            bcCactus.addLine(*this, point(0.0f, y), point(side * 61.0f, y), hex(0x244b49), 31.0f);
            bcCactus.addLine(*this, point(side * 61.0f, y), point(side * 61.0f, y + 69.0f), hex(0x244b49), 31.0f);
            bcCactus.addLine(*this, point(side * 61.0f, y), point(side * 61.0f, y + 65.0f), hex(0x55a67b), 23.0f);
            // Espinhos dos braços
            bcCactus.addLine(*this, point(side * 61.0f - 4.0f, y + 25.0f), point(side * 61.0f + 4.0f, y + 25.0f), hex(0xf0e698), 2.0f);
            bcCactus.addLine(*this, point(side * 61.0f - 4.0f, y + 50.0f), point(side * 61.0f + 4.0f, y + 50.0f), hex(0xf0e698), 2.0f);
        }

        // Nervuras claras com espinhos amarelos
        for (float rib : {-11.0f, 11.0f}) {
            bcCactus.addLine(*this, point(rib, 58.0f), point(rib, height - 14.0f), hex(0x86bb78), 2.5f);
            for (float sy = 75.0f; sy < height - 20.0f; sy += 36.0f) {
                bcCactus.addLine(*this, point(rib - 3.0f, sy), point(rib + 3.0f, sy), hex(0xf5ed9a), 2.0f);
            }
        }

        // Flores de pitaya cor-de-rosa/magenta no topo dos cactos
        if (i % 2 == 0) {
            bcCactus.addPoly(*this, {point(-22.0f, height + 3.0f), point(-10.0f, height + 19.0f),
                                    point(8.0f, height + 24.0f), point(23.0f, height + 4.0f),
                                    point(0.0f, height - 4.0f)},
                             hex(0xea4f7c), hex(0x7e2c46));
            // Núcleo dourado da flor
            bcCactus.addPoly(*this, {point(-6.0f, height + 6.0f), point(6.0f, height + 6.0f),
                                    point(0.0f, height + 15.0f)},
                             hex(0xffd850));
        }
    }

    // Flush dos lotes agrupados
    bcGround.flush(queue_);
    bcTiles.flush(queue_);
    bcWallBase.flush(queue_);
    bcNiches.flush(queue_);
    bcButtress.flush(queue_);
    bcBunting.flush(queue_);
    bcPots.flush(queue_);
    bcCactus.flush(queue_);
}

void RingRenderer::queueJackGarden() {
    // =========================================================================
    // O JARDIM BOTÂNICO REAL & APIÁRIO VICTORIANO DO JACK ("THE CRYSTAL HIVE")
    // Modelado inteiramente em 3D sólido cel-shaded a partir de referências.
    // Geometria rica em coordenadas globais (estável em todos os ângulos do noclip).
    // =========================================================================

    auto polar = [](float a, float r, float y) {
        return Vec3{std::sin(a) * r, y, std::cos(a) * r};
    };

    // Usa o BatchCollector compartilhado para acumular múltiplos polígonos/linhas por camada.

    // Culling frustum e proximidade: descarta objetos estritamente atrás da câmera ou fora da visão
    auto isCulled = [&](const Vec3& pos, float radius) -> bool {
        const Vec3 local = camera_.toLocal(pos);
        if (local.z + radius < 15.0f) return true; // atrás da câmera / plano próximo
        if (std::fabs(local.x) > local.z * 1.6f + radius) return true; // fora horizontal
        if (std::fabs(local.y) > local.z * 1.3f + radius) return true; // fora vertical
        return false;
    };

    // -------------------------------------------------------------------------
    // HELPER 1: Caixa sólida 3D com rotação yaw e sombreamento cel-shade nas faces
    // -------------------------------------------------------------------------
    auto drawSolidBoxBatch = [&](BatchCollector& bc, const Vec3& center, const Vec3& size, float yaw,
                                 sf::Color topCol, sf::Color frontCol, sf::Color sideCol,
                                 sf::Color strokeCol) {
        const float hx = size.x * 0.5f;
        const float hy = size.y * 0.5f;
        const float hz = size.z * 0.5f;
        const float cy = std::cos(yaw), sy = std::sin(yaw);

        auto corner = [&](float dx, float dy, float dz) {
            return Vec3{center.x + dx * cy - dz * sy,
                        center.y + dy,
                        center.z + dx * sy + dz * cy};
        };

        const Vec3 p0 = corner(-hx, -hy, -hz);
        const Vec3 p1 = corner( hx, -hy, -hz);
        const Vec3 p2 = corner( hx, -hy,  hz);
        const Vec3 p3 = corner(-hx, -hy,  hz);
        const Vec3 p4 = corner(-hx,  hy, -hz);
        const Vec3 p5 = corner( hx,  hy, -hz);
        const Vec3 p6 = corner( hx,  hy,  hz);
        const Vec3 p7 = corner(-hx,  hy,  hz);

        // Face Superior (Top)
        if (topCol.a > 0) {
            bc.addPoly(*this, {p4, p5, p6, p7}, topCol, strokeCol);
        }
        // Face Frontal (+Z local)
        if (frontCol.a > 0) {
            bc.addPoly(*this, {p3, p2, p6, p7}, frontCol, strokeCol);
        }
        // Face Traseira (-Z local - levemente sombreada)
        const sf::Color backCol(static_cast<uint8_t>(frontCol.r * 0.82f),
                                static_cast<uint8_t>(frontCol.g * 0.82f),
                                static_cast<uint8_t>(frontCol.b * 0.82f), frontCol.a);
        if (backCol.a > 0) {
            bc.addPoly(*this, {p1, p0, p4, p5}, backCol, strokeCol);
        }
        // Face Direita (+X local)
        if (sideCol.a > 0) {
            bc.addPoly(*this, {p2, p1, p5, p6}, sideCol, strokeCol);
        }
        // Face Esquerda (-X local - levemente sombreada)
        const sf::Color leftCol(static_cast<uint8_t>(sideCol.r * 0.85f),
                                static_cast<uint8_t>(sideCol.g * 0.85f),
                                static_cast<uint8_t>(sideCol.b * 0.85f), sideCol.a);
        if (leftCol.a > 0) {
            bc.addPoly(*this, {p0, p3, p7, p4}, leftCol, strokeCol);
        }
    };

    auto drawSolidBox = [&](const Vec3& center, const Vec3& size, float yaw,
                            sf::Color topCol, sf::Color frontCol, sf::Color sideCol,
                            sf::Color strokeCol, float layer = 0.12f) {
        BatchCollector bc(layer);
        drawSolidBoxBatch(bc, center, size, yaw, topCol, frontCol, sideCol, strokeCol);
        bc.flush(queue_);
    };

    // -------------------------------------------------------------------------
    // HELPER 2: Cilindro/Prisma facetado em 3D (para colunas, postes e vasos)
    // -------------------------------------------------------------------------
    auto drawSolidCylinderBatch = [&](BatchCollector& bc, const Vec3& base, float rBot, float rTop, float height,
                                      int sides, float yawOffset, sf::Color topCol,
                                      sf::Color sideColLight, sf::Color sideColDark,
                                      sf::Color strokeCol) {
        if (sides < 3 || sides > 16) return;
        std::array<Vec3, 16> botRing;
        std::array<Vec3, 16> topRing;
        const float step = 2.0f * kPi / sides;
        for (int i = 0; i < sides; ++i) {
            const float a = i * step + yawOffset;
            const float sa = std::sin(a), ca = std::cos(a);
            botRing[i] = Vec3{base.x + sa * rBot, base.y, base.z + ca * rBot};
            topRing[i] = Vec3{base.x + sa * rTop, base.y + height, base.z + ca * rTop};
        }
        for (int i = 0; i < sides; ++i) {
            const int next = (i + 1) % sides;
            const float midA = (i + 0.5f) * step + yawOffset;
            const float lit = 0.5f + 0.5f * std::cos(midA - 0.7f);
            const sf::Color col(
                static_cast<uint8_t>(sideColDark.r + (sideColLight.r - sideColDark.r) * lit),
                static_cast<uint8_t>(sideColDark.g + (sideColLight.g - sideColDark.g) * lit),
                static_cast<uint8_t>(sideColDark.b + (sideColLight.b - sideColDark.b) * lit),
                sideColLight.a
            );
            bc.addPoly(*this, {botRing[i], botRing[next], topRing[next], topRing[i]}, col, strokeCol);
        }
        if (topCol.a > 0) {
            bc.addPoly(*this, topRing.data(), sides, topCol, strokeCol);
        }
    };

    auto drawSolidCylinder = [&](const Vec3& base, float rBot, float rTop, float height,
                                 int sides, float yawOffset, sf::Color topCol,
                                 sf::Color sideColLight, sf::Color sideColDark,
                                 sf::Color strokeCol, float layer = 0.12f) {
        BatchCollector bc(layer);
        drawSolidCylinderBatch(bc, base, rBot, rTop, height, sides, yawOffset, topCol, sideColLight, sideColDark, strokeCol);
        bc.flush(queue_);
    };

    // -------------------------------------------------------------------------
    // HELPER 3: Barril 3D de mel e carvalho com aros de ferro
    // -------------------------------------------------------------------------
    auto draw3DHoneyBarrel = [&](const Vec3& root, float radius, float height, float yaw) {
        if (isCulled(root, radius * 1.5f)) return;
        constexpr int sides = 8;
        const float midR = radius * 1.15f;
        const float halfH = height * 0.5f;
        const float step = 2.0f * kPi / sides;
        BatchCollector barrelBc(0.11f);
        for (int s = 0; s < sides; ++s) {
            const float a0 = s * step + yaw, a1 = (s + 1) * step + yaw;
            const float sa0 = std::sin(a0), ca0 = std::cos(a0);
            const float sa1 = std::sin(a1), ca1 = std::cos(a1);

            const Vec3 b0{root.x + sa0 * radius, root.y, root.z + ca0 * radius};
            const Vec3 b1{root.x + sa1 * radius, root.y, root.z + ca1 * radius};
            const Vec3 m0{root.x + sa0 * midR, root.y + halfH, root.z + ca0 * midR};
            const Vec3 m1{root.x + sa1 * midR, root.y + halfH, root.z + ca0 * midR};
            const Vec3 t0{root.x + sa0 * radius, root.y + height, root.z + ca0 * radius};
            const Vec3 t1{root.x + sa1 * radius, root.y + height, root.z + ca1 * radius};

            const sf::Color woodCol = (s % 2 == 0) ? hex(0x86603a) : hex(0x73502f);
            barrelBc.addPoly(*this, {b0, b1, m1, m0}, woodCol, hex(0x2d1f14));
            barrelBc.addPoly(*this, {m0, m1, t1, t0}, woodCol, hex(0x2d1f14));
        }

        std::array<Vec3, sides> lid;
        for (int s = 0; s < sides; ++s) {
            const float a = s * step + yaw;
            lid[s] = Vec3{root.x + std::sin(a) * radius * 0.95f, root.y + height, root.z + std::cos(a) * radius * 0.95f};
        }
        barrelBc.addPoly(*this, lid.data(), sides, hex(0x614427), hex(0x2d1f14));

        for (float hoopH : {height * 0.22f, height * 0.78f}) {
            const float rHoop = radius + (midR - radius) * (1.0f - std::fabs(hoopH - halfH) / halfH);
            for (int s = 0; s < sides; ++s) {
                const float a0 = s * step + yaw, a1 = (s + 1) * step + yaw;
                barrelBc.addLine(*this, {root.x + std::sin(a0) * (rHoop + 1.0f), root.y + hoopH, root.z + std::cos(a0) * (rHoop + 1.0f)},
                                 {root.x + std::sin(a1) * (rHoop + 1.0f), root.y + hoopH, root.z + std::cos(a1) * (rHoop + 1.0f)},
                                 hex(0x212226), 4.5f);
            }
        }
        barrelBc.flush(queue_);
    };

    // -------------------------------------------------------------------------
    // HELPER 4: Colmeia Langstroth 3D completa com caixas empilhadas, telhado e abelhas
    // -------------------------------------------------------------------------
    auto draw3DBeehive = [&](const Vec3& root, float yaw, int hiveSeed) {
        if (isCulled(root, 75.0f)) return;
        BatchCollector hiveBc(0.11f);

        drawSolidBoxBatch(hiveBc, {root.x, root.y + 0.8f, root.z}, {68.0f, 1.2f, 56.0f}, yaw,
                          hex(0x251c14), hex(0x251c14), hex(0x251c14), sf::Color::Transparent);

        const float legH = 22.0f;
        const float cy = std::cos(yaw), sy = std::sin(yaw);
        auto legPos = [&](float lx, float lz) {
            return Vec3{root.x + lx * cy - lz * sy, root.y + legH * 0.5f, root.z + lx * sy + lz * cy};
        };
        auto cornerPt = [&](float lx, float ly, float lz) {
            return Vec3{root.x + lx * cy - lz * sy, root.y + ly, root.z + lx * sy + lz * cy};
        };

        for (float lx : {-22.0f, 22.0f}) {
            for (float lz : {-17.0f, 17.0f}) {
                drawSolidBoxBatch(hiveBc, legPos(lx, lz), {6.0f, legH, 6.0f}, yaw,
                                  hex(0x563e27), hex(0x493420), hex(0x3e2c1a), hex(0x23180e));
            }
        }
        drawSolidBoxBatch(hiveBc, {root.x, root.y + legH + 2.0f, root.z}, {58.0f, 4.0f, 48.0f}, yaw,
                          hex(0x846039), hex(0x6a4a28), hex(0x583c1e), hex(0x281b0e));

        const Vec3 rampPos = legPos(0.0f, 27.0f);
        drawSolidBoxBatch(hiveBc, {rampPos.x, root.y + legH + 1.0f, rampPos.z}, {42.0f, 3.0f, 14.0f}, yaw,
                          hex(0x9d7241), hex(0x7c572e), hex(0x624320), hex(0x2b1c0e));

        float curY = root.y + legH + 4.0f;
        constexpr int boxCount = 3;
        const std::array<uint32_t, 3> boxTones = {0xd69a4d, 0xcaa053, 0xdfab5a};
        for (int b = 0; b < boxCount; ++b) {
            const float boxH = 28.0f;
            const float boxCenterY = curY + boxH * 0.5f;
            const uint32_t col = boxTones[static_cast<std::size_t>((b + hiveSeed) % 3)];
            drawSolidBoxBatch(hiveBc, {root.x, boxCenterY, root.z}, {52.0f, boxH, 44.0f}, yaw,
                              hex(col), hex(col), hex(col - 0x15120a), hex(0x3e2b16));

            const Vec3 frontNotch = legPos(0.0f, 22.5f);
            drawSolidBoxBatch(hiveBc, {frontNotch.x, boxCenterY, frontNotch.z}, {18.0f, 4.5f, 1.2f}, yaw,
                              hex(0x3a2512), hex(0x291809), hex(0x291809), sf::Color::Transparent);

            drawSolidBoxBatch(hiveBc, {root.x, curY + boxH, root.z}, {53.5f, 1.8f, 45.5f}, yaw,
                              hex(0x5c3c1e), hex(0x4a2f16), hex(0x422811), hex(0x241508));

            if (b == 0) {
                const Vec3 slitPos = legPos(0.0f, 22.3f);
                drawSolidBoxBatch(hiveBc, {slitPos.x, curY + 2.5f, slitPos.z}, {26.0f, 3.2f, 1.0f}, yaw,
                                  hex(0x1a110a), hex(0x150d07), hex(0x150d07), sf::Color::Transparent);
            }
            curY += boxH + 1.8f;
        }

        const float roofH = 16.0f;
        drawSolidBoxBatch(hiveBc, {root.x, curY + 3.0f, root.z}, {58.0f, 5.0f, 50.0f}, yaw,
                          hex(0x527c6a), hex(0x3d6152), hex(0x335245), hex(0x1d3229));

        const Vec3 peak0 = cornerPt(0.0f, curY + 5.0f + roofH, -26.0f);
        const Vec3 peak1 = cornerPt(0.0f, curY + 5.0f + roofH,  26.0f);
        const Vec3 eaveL0 = cornerPt(-31.0f, curY + 4.0f, -26.0f);
        const Vec3 eaveL1 = cornerPt(-31.0f, curY + 4.0f,  26.0f);
        const Vec3 eaveR0 = cornerPt( 31.0f, curY + 4.0f, -26.0f);
        const Vec3 eaveR1 = cornerPt( 31.0f, curY + 4.0f,  26.0f);

        hiveBc.addPoly(*this, {eaveL0, eaveL1, peak1, peak0}, hex(0x5f8d79), hex(0x1d3229));
        hiveBc.addPoly(*this, {peak0, peak1, eaveR1, eaveR0}, hex(0x4c7563), hex(0x1d3229));
        hiveBc.addPoly(*this, {eaveL0, peak0, eaveR0}, hex(0x6b4c2b), hex(0x2d1e0f));
        hiveBc.addPoly(*this, {eaveR1, peak1, eaveL1}, hex(0x573d21), hex(0x2d1e0f));
        hiveBc.addLine(*this, peak0, peak1, hex(0xdfb448), 4.0f);
        hiveBc.flush(queue_);

        const float tBase = static_cast<float>(frameCounter_) * 0.048f + hiveSeed * 1.9f;
        for (int b = 0; b < 3; ++b) {
            const float tb = tBase + b * 2.094f;
            const float bx = std::cos(tb) * (42.0f + b * 14.0f);
            const float by = curY * 0.65f + std::sin(tb * 2.1f) * 16.0f;
            const float bz = std::sin(tb) * (36.0f + b * 10.0f);
            const Vec3 beePos{root.x + bx * cy - bz * sy, by, root.z + bx * sy + bz * cy};
            const Vec3 beeDir{-std::sin(tb) * cy - std::cos(tb) * sy,
                              0.3f * std::cos(tb * 2.1f),
                              -std::sin(tb) * sy + std::cos(tb) * cy};
            if (auto pr = camera_.project(beePos)) {
                const float flap = std::fmod(static_cast<float>(frameCounter_) * 1.4f + hiveSeed * 2 + b, 1.0f);
                queue_.push_back({0.45f, pr->depth, [this, beePos, beeDir, flap](sf::RenderTarget& tg) {
                    vfx::drawPsxBee(tg, camera_, beePos, beeDir, 4.2f, flap, 0.0f, 0.95f, false, 0);
                }});
            }
        }
    };

    // -------------------------------------------------------------------------
    // HELPER 5: Girassol Gigante em 3D autêntico (caule articulado, folhas e coroa)
    // -------------------------------------------------------------------------
    auto draw3DSunflower = [&](const Vec3& root, float height, float tiltAngle, float faceYaw, float scale, int seed) {
        if (isCulled(root, 75.0f * scale)) return;

        BatchCollector flowerBc(0.130f);

        drawSolidBoxBatch(flowerBc, {root.x, root.y + 0.5f, root.z}, {28.0f * scale, 1.0f, 28.0f * scale}, 0.0f,
                          hex(0x201a12), hex(0x201a12), hex(0x201a12), sf::Color::Transparent);

        const float sway = std::sin(static_cast<float>(frameCounter_) * 0.032f + seed * 1.25f) * (4.5f * scale);
        const float cyF = std::cos(faceYaw), syF = std::sin(faceYaw);

        const Vec3 p0 = root;
        const Vec3 p1{root.x + syF * (12.0f * scale) + sway * 0.3f, root.y + height * 0.40f, root.z + cyF * (12.0f * scale)};
        const Vec3 p2{root.x + syF * (26.0f * scale) + sway * 0.7f, root.y + height * 0.75f, root.z + cyF * (26.0f * scale)};
        const Vec3 crown{root.x + syF * (38.0f * scale) + sway, root.y + height, root.z + cyF * (38.0f * scale)};

        flowerBc.addLine(*this, p0, p1, hex(0x2b5e32), 8.5f * scale);
        flowerBc.addLine(*this, p1, p2, hex(0x356f3a), 7.5f * scale);
        flowerBc.addLine(*this, p2, crown, hex(0x3e7e42), 6.5f * scale);

        const float leafYaw = faceYaw + kPi * 0.5f;
        const float cL = std::cos(leafYaw), sL = std::sin(leafYaw);
        const Vec3 lf0 = p1;
        const Vec3 lf1{p1.x - sL * (28.0f * scale), p1.y + 12.0f * scale, p1.z - cL * (28.0f * scale)};
        const Vec3 lfTip{p1.x - sL * (48.0f * scale), p1.y - 4.0f * scale, p1.z - cL * (48.0f * scale)};
        const Vec3 lf2{p1.x - sL * (22.0f * scale) + syF * (14.0f * scale), p1.y + 2.0f * scale, p1.z - cL * (22.0f * scale) + cyF * (14.0f * scale)};

        const Vec3 rf0 = p2;
        const Vec3 rf1{p2.x + sL * (30.0f * scale), p2.y + 14.0f * scale, p2.z + cL * (30.0f * scale)};
        const Vec3 rfTip{p2.x + sL * (52.0f * scale), p2.y - 2.0f * scale, p2.z + cL * (52.0f * scale)};
        const Vec3 rf2{p2.x + sL * (24.0f * scale) - syF * (12.0f * scale), p2.y + 4.0f * scale, p2.z + cL * (24.0f * scale) - cyF * (12.0f * scale)};

        flowerBc.addPoly(*this, {lf0, lf1, lfTip, lf2}, hex(0x3f7d38), hex(0x1f471e));
        flowerBc.addPoly(*this, {rf0, rf1, rfTip, rf2}, hex(0x488a40), hex(0x1f471e));

        const float st = std::sin(tiltAngle), ct = std::cos(tiltAngle);
        const Vec3 normal{syF * ct, -st, cyF * ct};
        Vec3 u{-normal.z, 0.0f, normal.x};
        const float uLen = std::sqrt(u.x * u.x + u.z * u.z);
        if (uLen > 0.001f) { u.x /= uLen; u.z /= uLen; }
        else { u = {1.0f, 0.0f, 0.0f}; }
        const Vec3 v{u.y * normal.z - u.z * normal.y,
                     u.z * normal.x - u.x * normal.z,
                     u.x * normal.y - u.y * normal.x};

        const float headR = 46.0f * scale;

        constexpr int sepalCount = 6;
        for (int s = 0; s < sepalCount; ++s) {
            const float a0 = s * 2.0f * kPi / sepalCount;
            const float a1 = (s + 1) * 2.0f * kPi / sepalCount;
            const float aMid = (a0 + a1) * 0.5f;
            const Vec3 pA{crown.x + (u.x * std::cos(a0) + v.x * std::sin(a0)) * (headR * 0.45f) - normal.x * (6.0f * scale),
                          crown.y + (u.y * std::cos(a0) + v.y * std::sin(a0)) * (headR * 0.45f) - normal.y * (6.0f * scale),
                          crown.z + (u.z * std::cos(a0) + v.z * std::sin(a0)) * (headR * 0.45f) - normal.z * (6.0f * scale)};
            const Vec3 pB{crown.x + (u.x * std::cos(a1) + v.x * std::sin(a1)) * (headR * 0.45f) - normal.x * (6.0f * scale),
                          crown.y + (u.y * std::cos(a1) + v.y * std::sin(a1)) * (headR * 0.45f) - normal.y * (6.0f * scale),
                          crown.z + (u.z * std::cos(a1) + v.z * std::sin(a1)) * (headR * 0.45f) - normal.z * (6.0f * scale)};
            const Vec3 tip{crown.x + (u.x * std::cos(aMid) + v.x * std::sin(aMid)) * (headR * 0.65f) - normal.x * (10.0f * scale),
                           crown.y + (u.y * std::cos(aMid) + v.y * std::sin(aMid)) * (headR * 0.65f) - normal.y * (10.0f * scale),
                           crown.z + (u.z * std::cos(aMid) + v.z * std::sin(aMid)) * (headR * 0.65f) - normal.z * (10.0f * scale)};
            flowerBc.addPoly(*this, {pA, tip, pB}, hex(0x275529), hex(0x163618));
        }

        constexpr int petalCount = 12;
        const float innerR = headR * 0.40f;
        const float petalLen = headR - innerR;
        const float petalStep = 2.0f * kPi / petalCount;
        for (int p = 0; p < petalCount; ++p) {
            const float a = p * petalStep;
            const float ca = std::cos(a), sa = std::sin(a);
            const float px = -sa, py = ca;

            const float wBase = innerR * 0.22f;
            const Vec3 b0{crown.x + (u.x * (ca * innerR - px * wBase) + v.x * (sa * innerR - py * wBase)),
                          crown.y + (u.y * (ca * innerR - px * wBase) + v.y * (sa * innerR - py * wBase)),
                          crown.z + (u.z * (ca * innerR - px * wBase) + v.z * (sa * innerR - py * wBase))};
            const Vec3 b1{crown.x + (u.x * (ca * innerR + px * wBase) + v.x * (sa * innerR + py * wBase)),
                          crown.y + (u.y * (ca * innerR + px * wBase) + v.y * (sa * innerR + py * wBase)),
                          crown.z + (u.z * (ca * innerR + px * wBase) + v.z * (sa * innerR + py * wBase))};

            const float rMid = innerR + petalLen * 0.55f;
            const float wMid = petalLen * 0.38f;
            const Vec3 m0{crown.x + (u.x * (ca * rMid - px * wMid) + v.x * (sa * rMid - py * wMid)) + normal.x * (2.0f * scale),
                          crown.y + (u.y * (ca * rMid - px * wMid) + v.y * (sa * rMid - py * wMid)) + normal.y * (2.0f * scale),
                          crown.z + (u.z * (ca * rMid - px * wMid) + v.z * (sa * rMid - py * wMid)) + normal.z * (2.0f * scale)};
            const Vec3 m1{crown.x + (u.x * (ca * rMid + px * wMid) + v.x * (sa * rMid + py * wMid)) + normal.x * (2.0f * scale),
                          crown.y + (u.y * (ca * rMid + px * wMid) + v.y * (sa * rMid + py * wMid)) + normal.y * (2.0f * scale),
                          crown.z + (u.z * (ca * rMid + px * wMid) + v.z * (sa * rMid + py * wMid)) + normal.z * (2.0f * scale)};

            const Vec3 tip{crown.x + (u.x * ca * headR + v.x * sa * headR) + normal.x * (3.5f * scale),
                           crown.y + (u.y * ca * headR + v.y * sa * headR) + normal.y * (3.5f * scale),
                           crown.z + (u.z * ca * headR + v.z * sa * headR) + normal.z * (3.5f * scale)};

            const sf::Color petCol = (p % 2 == 0) ? hex(0xfac032) : hex(0xf2a61a);
            flowerBc.addPoly(*this, {b0, m0, tip, m1, b1}, petCol, hex(0x824b10));
        }

        constexpr int discSegs = 8;
        std::array<Vec3, 8> disc;
        for (int d = 0; d < discSegs; ++d) {
            const float a = d * 2.0f * kPi / discSegs;
            disc[d] = Vec3{crown.x + (u.x * std::cos(a) + v.x * std::sin(a)) * innerR + normal.x * (5.5f * scale),
                           crown.y + (u.y * std::cos(a) + v.y * std::sin(a)) * innerR + normal.y * (5.5f * scale),
                           crown.z + (u.z * std::cos(a) + v.z * std::sin(a)) * innerR + normal.z * (5.5f * scale)};
        }
        flowerBc.addPoly(*this, disc.data(), discSegs, hex(0x351f12), hex(0x56381e));

        constexpr int innerSegs = 6;
        std::array<Vec3, 6> innerDisc;
        for (int d = 0; d < innerSegs; ++d) {
            const float a = d * 2.0f * kPi / innerSegs;
            innerDisc[d] = Vec3{crown.x + (u.x * std::cos(a) + v.x * std::sin(a)) * (innerR * 0.52f) + normal.x * (7.5f * scale),
                                crown.y + (u.y * std::cos(a) + v.y * std::sin(a)) * (innerR * 0.52f) + normal.y * (7.5f * scale),
                                crown.z + (u.z * std::cos(a) + v.z * std::sin(a)) * (innerR * 0.52f) + normal.z * (7.5f * scale)};
        }
        flowerBc.addPoly(*this, innerDisc.data(), innerSegs, hex(0x633e1c), hex(0xb38634));
        flowerBc.flush(queue_);
    };

    // -------------------------------------------------------------------------
    // HELPER 6: Roseiras Reais 3D (massa orgânica facetada e rosas imperiais)
    // -------------------------------------------------------------------------
    auto draw3DRoseBush = [&](const Vec3& root, float radius, float height, int roseCount, int seed) {
        if (isCulled(root, radius * 1.5f + height)) return;
        BatchCollector bushBc(0.128f);

        drawSolidBoxBatch(bushBc, {root.x, root.y + 0.6f, root.z}, {radius * 2.1f, 1.2f, radius * 2.1f}, 0.0f,
                          hex(0x281e14), hex(0x281e14), hex(0x281e14), sf::Color::Transparent);

        constexpr int foliageMounds = 4;
        for (int m = 0; m < foliageMounds; ++m) {
            const float a = m * 2.0f * kPi / foliageMounds + seed * 0.7f;
            const float dist = (m == 0 ? 0.0f : radius * 0.55f);
            const Vec3 mCenter{root.x + std::sin(a) * dist,
                               root.y + (m == 0 ? height * 0.58f : height * 0.42f),
                               root.z + std::cos(a) * dist};
            const float mR = (m == 0 ? radius * 0.70f : radius * 0.55f);
            const sf::Color folCol = (m % 2 == 0) ? hex(0x2d5f32) : hex(0x38733c);
            drawSolidCylinderBatch(bushBc, Vec3{mCenter.x, mCenter.y - mR * 0.6f, mCenter.z},
                                   mR * 0.7f, mR * 0.85f, mR * 1.2f, 5, a,
                                   folCol, folCol, hex(0x1e4423), hex(0x143118));
        }

        for (int r = 0; r < roseCount; ++r) {
            const float a = (r + 0.35f) * 2.0f * kPi / roseCount + seed * 1.4f;
            const float dist = radius * (0.45f + 0.45f * ((r * 7) % 5) / 5.0f);
            const float ry = root.y + height * (0.45f + 0.45f * ((r * 11) % 6) / 6.0f);
            const Vec3 rosePos{root.x + std::sin(a) * dist, ry, root.z + std::cos(a) * dist};

            const sf::Color outerCol = (r % 2 == 0) ? hex(0xd92644) : hex(0xec3f5e);
            const sf::Color innerCol = (r % 2 == 0) ? hex(0xf46580) : hex(0xfa8498);
            const float roseSize = 18.0f + static_cast<float>((r * 13) % 8);

            constexpr int petSides = 5;
            std::array<Vec3, 5> petals;
            for (int p = 0; p < petSides; ++p) {
                const float pa = p * 2.0f * kPi / petSides + a;
                petals[p] = Vec3{rosePos.x + std::sin(pa) * (roseSize * 0.5f),
                                 rosePos.y + std::cos(pa) * (roseSize * 0.4f),
                                 rosePos.z + std::sin(pa * 1.3f) * 4.0f};
            }
            bushBc.addPoly(*this, petals.data(), petSides, outerCol, hex(0x4d111d));

            constexpr int innerSides = 4;
            std::array<Vec3, 4> innerPet;
            for (int p = 0; p < innerSides; ++p) {
                const float pa = p * 2.0f * kPi / innerSides + a + 0.6f;
                innerPet[p] = Vec3{rosePos.x + std::sin(pa) * (roseSize * 0.28f),
                                   rosePos.y + std::cos(pa) * (roseSize * 0.22f),
                                   rosePos.z + 3.0f};
            }
            bushBc.addPoly(*this, innerPet.data(), innerSides, innerCol, hex(0x611322));

            bushBc.addLine(*this, rosePos, Vec3{rosePos.x, rosePos.y + 1.0f, rosePos.z + 4.5f},
                           hex(0xffd560), 5.5f);
        }
        bushBc.flush(queue_);
    };

    // -------------------------------------------------------------------------
    // HELPER 7: Lanterna Vitoriana Hexagonal de Mel (Chama de néctar e vidro)
    // -------------------------------------------------------------------------
    auto drawHexLantern = [&](const Vec3& center, float r, float h, float flicker, float layer = 0.225f) {
        constexpr int sides = 6;
        const float step = 2.0f * kPi / sides;
        const float halfH = h * 0.5f;

        std::vector<Vec3> bot(sides), top(sides);
        for (int i = 0; i < sides; ++i) {
            const float a = i * step;
            bot[i] = Vec3{center.x + std::sin(a) * r, center.y - halfH, center.z + std::cos(a) * r};
            top[i] = Vec3{center.x + std::sin(a) * r, center.y + halfH, center.z + std::cos(a) * r};
        }

        BatchCollector lanternBc(layer);
        const Vec3 peak{center.x, center.y + halfH + r * 0.75f, center.z};
        for (int i = 0; i < sides; ++i) {
            const int next = (i + 1) % sides;
            lanternBc.addPoly(*this, {top[i], top[next], peak}, hex(0x272c30), hex(0xd4a849));
        }
        lanternBc.addLine(*this, peak, Vec3{peak.x, peak.y + 14.0f, peak.z}, hex(0xe4bb52), 4.0f);
        lanternBc.addPoly(*this, {Vec3{peak.x - 10.0f, peak.y + 10.0f, peak.z},
                                  Vec3{peak.x, peak.y + 16.0f, peak.z},
                                  Vec3{peak.x + 10.0f, peak.y + 10.0f, peak.z}},
                          hex(0xf8d458), hex(0x76541c));

        const uint8_t glassAlpha = static_cast<uint8_t>(std::clamp(180 + flicker * 4.0f, 120.0f, 255.0f));
        for (int i = 0; i < sides; ++i) {
            const int next = (i + 1) % sides;
            lanternBc.addPoly(*this, {bot[i], bot[next], top[next], top[i]},
                              sf::Color(255, 195, 55, glassAlpha), hex(0x24282c));
        }

        const Vec3 botPeak{center.x, center.y - halfH - r * 0.40f, center.z};
        for (int i = 0; i < sides; ++i) {
            const int next = (i + 1) % sides;
            lanternBc.addPoly(*this, {bot[next], bot[i], botPeak}, hex(0x24282c), hex(0xd4a849));
        }

        const float flameSize = (r * 0.42f) + flicker * 0.5f;
        drawSolidBoxBatch(lanternBc, Vec3{center.x, center.y + flicker * 0.6f, center.z},
                          {flameSize, flameSize * 1.5f, flameSize},
                          flicker * 0.2f,
                          sf::Color(255, 250, 195, 245),
                          sf::Color(255, 245, 160, 240),
                          sf::Color(255, 215, 75, 230),
                          sf::Color::Transparent);
        lanternBc.flush(queue_);
    };

    // -------------------------------------------------------------------------
    // HELPER 8: Moinho de Vento Holandês Tradicional (Stellingmolen) 3D
    // Base octogonal de tijolo, galeria/varanda de madeira, corpo cônico de palha,
    // cúpula/capô e 4 pás com treliça em rotação animada.
    // -------------------------------------------------------------------------
    auto drawDutchWindmill = [&](const Vec3& root, float scale, float yaw, int millSeed) {
        if (isCulled(root, 180.0f * scale)) return;

        BatchCollector millBc(0.062f);

        // Base octogonal de alvenaria/tijolos holandeses terracota
        const float baseR = 48.0f * scale;
        const float baseH = 75.0f * scale;
        drawSolidCylinderBatch(millBc, root, baseR, baseR * 0.92f, baseH, 8, yaw,
                               hex(0x6e372b), hex(0x8a4536), hex(0x54261d), hex(0x28100b));

        // Galeria de vigamento / Passarela suspensa (Stelling)
        const float galleryY = root.y + baseH;
        const float galleryR = baseR * 1.35f;
        drawSolidCylinderBatch(millBc, Vec3{root.x, galleryY, root.z}, galleryR, galleryR, 6.0f * scale, 8, yaw,
                               hex(0x3b2a1a), hex(0x2b1e13), hex(0x1a120b), hex(0x120c07));
        // Guarda-corpo da galeria
        for (int s = 0; s < 8; ++s) {
            const float a0 = s * 2.0f * kPi / 8.0f + yaw;
            const float a1 = (s + 1) * 2.0f * kPi / 8.0f + yaw;
            const Vec3 r0{root.x + std::sin(a0) * galleryR, galleryY + 2.0f * scale, root.z + std::cos(a0) * galleryR};
            const Vec3 r1{root.x + std::sin(a1) * galleryR, galleryY + 2.0f * scale, root.z + std::cos(a1) * galleryR};
            const Vec3 t0{r0.x, galleryY + 16.0f * scale, r0.z};
            const Vec3 t1{r1.x, galleryY + 16.0f * scale, r1.z};
            millBc.addLine(*this, r0, t0, hex(0x24180e), 3.0f * scale);
            millBc.addLine(*this, t0, t1, hex(0x3e2c1c), 3.0f * scale);
        }

        // Corpo cônico de madeira/palha escura (Smock)
        const float smockBaseY = galleryY + 6.0f * scale;
        const float smockH = 150.0f * scale;
        const float smockTopR = baseR * 0.52f;
        drawSolidCylinderBatch(millBc, Vec3{root.x, smockBaseY, root.z}, baseR * 0.88f, smockTopR, smockH, 8, yaw,
                               hex(0x4a3d31), hex(0x56473a), hex(0x362c23), hex(0x1a1511));

        // Capô / Cúpula superior móvel (Kap)
        const float capBaseY = smockBaseY + smockH;
        const float capH = 34.0f * scale;
        const float capR = smockTopR * 1.15f;
        drawSolidCylinderBatch(millBc, Vec3{root.x, capBaseY, root.z}, capR, capR * 0.40f, capH, 8, yaw,
                               hex(0x242e2b), hex(0x2f3d39), hex(0x1c2422), hex(0x101514));

        // Eixo do moinho (Windshaft) projetado para a frente
        const float cy = std::cos(yaw), sy = std::sin(yaw);
        const float shaftLen = 28.0f * scale;
        const Vec3 hubPos{root.x + sy * shaftLen, capBaseY + capH * 0.55f, root.z + cy * shaftLen};
        drawSolidBoxBatch(millBc, hubPos, {12.0f * scale, 12.0f * scale, 12.0f * scale}, yaw,
                          hex(0x3a281a), hex(0x2d1e13), hex(0x20150d), hex(0x120c07));

        // Rotação contínua e suave das 4 pás
        const float sailRot = static_cast<float>(frameCounter_) * 0.0075f + millSeed * 1.57f;
        const float sailLen = 135.0f * scale;
        const float sailW = 24.0f * scale;

        // Vetores tangentes para as pás no plano perpendicular ao yaw
        const Vec3 rightAxis{cy, 0.0f, -sy};
        const Vec3 upAxis{0.0f, 1.0f, 0.0f};

        for (int p = 0; p < 4; ++p) {
            const float a = sailRot + p * (kPi * 0.5f);
            const float ca = std::cos(a), sa = std::sin(a);
            // Direção radial da pá
            const Vec3 dir{rightAxis.x * ca + upAxis.x * sa,
                           rightAxis.y * ca + upAxis.y * sa,
                           rightAxis.z * ca + upAxis.z * sa};
            // Direção perpendicular para a largura da vela de treliça
            const Vec3 perp{-rightAxis.x * sa + upAxis.x * ca,
                            -rightAxis.y * sa + upAxis.y * ca,
                            -rightAxis.z * sa + upAxis.z * ca};

            const Vec3 sparTip{hubPos.x + dir.x * sailLen,
                               hubPos.y + dir.y * sailLen,
                               hubPos.z + dir.z * sailLen};
            // Viga principal de madeira escura da pá
            millBc.addLine(*this, hubPos, sparTip, hex(0x2d1e13), 5.5f * scale);

            // Pano de vela / Treliça tradicional (Sailcloth & lattice)
            const float sailStart = 25.0f * scale;
            const float sailEnd = sailLen * 0.95f;
            const Vec3 v0{hubPos.x + dir.x * sailStart, hubPos.y + dir.y * sailStart, hubPos.z + dir.z * sailStart};
            const Vec3 v1{hubPos.x + dir.x * sailEnd,   hubPos.y + dir.y * sailEnd,   hubPos.z + dir.z * sailEnd};
            const Vec3 v2{v1.x + perp.x * sailW,        v1.y + perp.y * sailW,        v1.z + perp.z * sailW};
            const Vec3 v3{v0.x + perp.x * sailW,        v0.y + perp.y * sailW,        v0.z + perp.z * sailW};

            // Pano de linho cru com brilho de pôr do sol
            const sf::Color sailCol = (p % 2 == 0) ? hex(0xf4ead8) : hex(0xe8dcc6);
            millBc.addPoly(*this, {v0, v1, v2, v3}, sailCol, hex(0x3a2c1e));

            // Travessas da treliça da vela (3 ripas horizontais e 1 longitudinal)
            for (float frac : {0.25f, 0.50f, 0.75f}) {
                const Vec3 lA{v0.x + (v1.x - v0.x) * frac, v0.y + (v1.y - v0.y) * frac, v0.z + (v1.z - v0.z) * frac};
                const Vec3 lB{v3.x + (v2.x - v3.x) * frac, v3.y + (v2.y - v3.y) * frac, v3.z + (v2.z - v3.z) * frac};
                millBc.addLine(*this, lA, lB, hex(0x44301d), 2.2f * scale);
            }
            const Vec3 midA{ (v0.x + v3.x) * 0.5f, (v0.y + v3.y) * 0.5f, (v0.z + v3.z) * 0.5f };
            const Vec3 midB{ (v1.x + v2.x) * 0.5f, (v1.y + v2.y) * 0.5f, (v1.z + v2.z) * 0.5f };
            millBc.addLine(*this, midA, midB, hex(0x44301d), 2.2f * scale);
        }
        millBc.flush(queue_);
    };

    // -------------------------------------------------------------------------
    // HELPER 9: Árvores Holandesas 3D (Choupos esguios e Carvalhos copados)
    // -------------------------------------------------------------------------
    auto drawDutchTree = [&](const Vec3& root, float trunkH, float crownR, float crownH, int style, int seed) {
        if (isCulled(root, crownR * 1.5f + crownH)) return;
        BatchCollector treeBc(0.076f);
        drawSolidCylinderBatch(treeBc, root, crownR * 0.22f, crownR * 0.14f, trunkH, 6, seed * 0.6f,
                               hex(0x38281b), hex(0x4a3727), hex(0x302115), hex(0x191009));
        const Vec3 crownBase{root.x, root.y + trunkH * 0.85f, root.z};

        if (style == 0) {
            // Choupo Holandês (Poplar): silhueta esguia e vertical em múltiplos andares
            constexpr int tiers = 3;
            for (int t = 0; t < tiers; ++t) {
                const float frac = static_cast<float>(t) / tiers;
                const float tY = crownBase.y + frac * crownH;
                const float tR = crownR * (1.0f - frac * 0.65f);
                const float tH = (crownH / tiers) * 1.35f;
                const sf::Color folCol = (t % 2 == 0) ? hex(0x365427) : hex(0x456932);
                drawSolidCylinderBatch(treeBc, Vec3{crownBase.x, tY, crownBase.z}, tR, tR * 0.55f, tH, 6,
                                       frac * 1.4f + seed, folCol, folCol, hex(0x233a18), hex(0x12210a));
            }
        } else {
            // Carvalho Copado (Oak): copa arredondada e densa com massas de folhagem
            constexpr int clusters = 4;
            for (int c = 0; c < clusters; ++c) {
                const float ca = c * 2.0f * kPi / clusters + seed * 1.1f;
                const float cDist = (c == 0 ? 0.0f : crownR * 0.45f);
                const float cY = crownBase.y + (c == 0 ? crownH * 0.45f : crownH * 0.25f);
                const Vec3 cPos{crownBase.x + std::sin(ca) * cDist, cY, crownBase.z + std::cos(ca) * cDist};
                const float cR = (c == 0 ? crownR * 0.75f : crownR * 0.58f);
                const sf::Color folCol = (c % 2 == 0) ? hex(0x3b5c28) : hex(0x4d7534);
                drawSolidCylinderBatch(treeBc, cPos, cR, cR * 0.70f, crownH * 0.55f, 5, ca,
                                       folCol, folCol, hex(0x253d19), hex(0x14230c));
            }
        }
        treeBc.flush(queue_);
    };

    // -------------------------------------------------------------------------
    // HELPER 10: Campo / Parterre de Lavanda 3D (fileiras roxas e perfumadas)
    // -------------------------------------------------------------------------
    auto drawLavenderField = [&](const Vec3& center, float width, float length, float yaw, int rows, float curbH = 0.0f) {
        const float maxR = std::max(width, length) * 0.65f;
        if (isCulled(center, maxR)) return;

        BatchCollector lavBc(0.067f);
        if (curbH > 0.0f) {
            // Jardineira de cantaria de pedra clássica
            drawSolidBoxBatch(lavBc, Vec3{center.x, center.y + curbH * 0.5f, center.z},
                              {width + 12.0f, curbH, length + 12.0f}, yaw,
                              hex(0x9d8d78), hex(0x867764), hex(0x6a5b4a), hex(0x352b22));
        }
        // Solo fértil escuro do canteiro
        const float soilY = center.y + curbH + 0.4f;
        drawSolidBoxBatch(lavBc, Vec3{center.x, soilY, center.z}, {width, 1.0f, length}, yaw,
                          hex(0x271e16), hex(0x271e16), hex(0x271e16), sf::Color::Transparent);

        const float cy = std::cos(yaw), sy = std::sin(yaw);
        const float rowSpacing = width / (rows + 1);
        for (int r = 0; r < rows; ++r) {
            const float rx = -width * 0.5f + (r + 1) * rowSpacing;
            const float rBaseY = soilY + 1.0f;
            constexpr int bumps = 4;
            for (int b = 0; b < bumps; ++b) {
                const float rz = -length * 0.45f + b * (length * 0.90f / (bumps - 1));
                const Vec3 bumpCenter{center.x + rx * cy - rz * sy, rBaseY, center.z + rx * sy + rz * cy};
                const sf::Color lavCol = (b % 2 == 0) ? hex(0x7b4cb2) : hex(0x8d5ec8);
                drawSolidCylinderBatch(lavBc, bumpCenter, rowSpacing * 0.46f, rowSpacing * 0.30f, 22.0f, 5, yaw + b * 0.4f,
                                       hex(0xad7beb), lavCol, hex(0x563480), hex(0x311a4d));
                // Espigas florais roxas destacadas
                const Vec3 tipP{bumpCenter.x, bumpCenter.y + 22.0f, bumpCenter.z};
                lavBc.addLine(*this, tipP, Vec3{tipP.x + std::sin(b * 1.6f) * 4.0f, tipP.y + 14.0f, tipP.z + std::cos(b * 1.6f) * 4.0f},
                              hex(0xc898fa), 3.5f);
            }
        }
        lavBc.flush(queue_);
    };

    // =========================================================================
    // 1. HORIZONTE HOLANDÊS: MONTANHAS, COLINAS E NUVENS AO PÔR DO SOL
    // Cumes distantes com iluminação de borda dourada contra o céu crepuscular
    // =========================================================================
    constexpr int kMountainSectors = 36;
    constexpr float kMtnStep = 2.0f * kPi / kMountainSectors;
    const float mtnR0 = 9500.0f;
    const float mtnR1 = 11500.0f;

    // Cordilheira distante - Púrpura crepuscular com topo âmbar
    BatchCollector mtnFar(0.055f);
    BatchCollector mtnFarLine(0.056f);
    for (int i = 0; i < kMountainSectors; ++i) {
        const float a0 = i * kMtnStep, a1 = (i + 1) * kMtnStep;
        const float h0 = 550.0f + 280.0f * std::sin(a0 * 3.0f) + 160.0f * std::cos(a0 * 7.0f);
        const float h1 = 550.0f + 280.0f * std::sin(a1 * 3.0f) + 160.0f * std::cos(a1 * 7.0f);

        const Vec3 b0 = polar(a0, mtnR0, -40.0f);
        const Vec3 b1 = polar(a1, mtnR0, -40.0f);
        const Vec3 t1 = polar(a1, mtnR1, h1);
        const Vec3 t0 = polar(a0, mtnR1, h0);

        const sf::Color mtnCol = (i % 2 == 0) ? hex(0x3b2d4f) : hex(0x433359);
        mtnFar.addPoly(*this, {b0, b1, t1, t0}, mtnCol, sf::Color::Transparent);
        mtnFarLine.addLine(*this, t0, t1, hex(0xf89c46), 4.5f);
    }
    mtnFar.flush(queue_);
    mtnFarLine.flush(queue_);

    // Colinas onduladas intermediárias
    const float hillR0 = 7400.0f;
    const float hillR1 = 8800.0f;
    BatchCollector hillNear(0.058f);
    BatchCollector hillNearLine(0.059f);
    for (int i = 0; i < kMountainSectors; ++i) {
        const float a0 = i * kMtnStep, a1 = (i + 1) * kMtnStep;
        const float h0 = 260.0f + 140.0f * std::cos(a0 * 4.0f + 1.2f) + 90.0f * std::sin(a0 * 6.0f);
        const float h1 = 260.0f + 140.0f * std::cos(a1 * 4.0f + 1.2f) + 90.0f * std::sin(a1 * 6.0f);

        const Vec3 b0 = polar(a0, hillR0, -38.0f);
        const Vec3 b1 = polar(a1, hillR0, -38.0f);
        const Vec3 t1 = polar(a1, hillR1, h1);
        const Vec3 t0 = polar(a0, hillR1, h0);

        const sf::Color hillCol = (i % 2 == 0) ? hex(0x2f3630) : hex(0x363e37);
        hillNear.addPoly(*this, {b0, b1, t1, t0}, hillCol, sf::Color::Transparent);
        hillNearLine.addLine(*this, t0, t1, hex(0xdfa052), 3.0f);
    }
    hillNear.flush(queue_);
    hillNearLine.flush(queue_);

    // Nuvens estilizadas no crepúsculo
    BatchCollector clouds(0.065f);
    for (int i = 0; i < 9; ++i) {
        const float a = (i + 0.28f) * 2.0f * kPi / 9.0f;
        const Vec3 root = polar(a, 2500.0f, 380.0f + static_cast<float>((i % 3) * 25));
        const float w = 180.0f + static_cast<float>((i % 3) * 35);
        const float cosA = std::cos(a), sinA = std::sin(a);
        auto cloudPt = [&](float u, float v) {
            return Vec3{root.x - cosA * u, root.y + v, root.z + sinA * u};
        };
        clouds.addPoly(*this, {cloudPt(-w, -18.0f), cloudPt(-w * 0.75f, 15.0f),
                               cloudPt(-w * 0.45f, 22.0f), cloudPt(-w * 0.25f, 48.0f),
                               cloudPt(0.0f, 62.0f), cloudPt(w * 0.28f, 35.0f),
                               cloudPt(w * 0.58f, 40.0f), cloudPt(w * 0.85f, 12.0f),
                               cloudPt(w, -18.0f)},
                       sf::Color(255, 235, 210, 210), sf::Color::Transparent);
    }
    clouds.flush(queue_);

    // =========================================================================
    // 2. A GRANDE ESTUFA VITORIANA ("THE CRYSTAL HIVE PALACE")
    // Cúpulas monumentais trazidas para junto do jardim botânico (sem vazio)
    // =========================================================================
    auto drawConservatoryDome = [&](const Vec3& center, float radius, float wallH, float domeH, bool isMain) {
        if (isCulled(center, radius * 1.25f + domeH)) return;

        BatchCollector domeBc(0.068f);

        // Base / Fundação de cantaria de pedra moldurada
        drawSolidCylinderBatch(domeBc, Vec3{center.x, -35.0f, center.z}, radius * 1.06f, radius * 1.02f,
                               wallH * 0.28f, 8, 0.392f,
                               hex(0x6b5b4c), hex(0x56483b), hex(0x44372c), hex(0x271e17));
        // Friso moldurado superior da fundação
        drawSolidCylinderBatch(domeBc, Vec3{center.x, -35.0f + wallH * 0.28f, center.z}, radius * 1.04f, radius * 1.01f,
                               8.0f, 8, 0.392f,
                               hex(0x8a7764), hex(0x736150), hex(0x564738), hex(0x2d241c));

        // Galeria de paredes de vidro com arcos de ferro forjado e luz dourada interna
        constexpr int octSides = 8;
        const float octStep = 2.0f * kPi / octSides;
        const float galleryBaseY = -35.0f + wallH * 0.28f + 8.0f;
        const float galleryTopY = galleryBaseY + wallH * 0.68f;

        std::array<Vec3, 8> wallBot;
        std::array<Vec3, 8> wallTop;
        for (int i = 0; i < octSides; ++i) {
            const float a = i * octStep + 0.392f;
            wallBot[i] = Vec3{center.x + std::sin(a) * radius, galleryBaseY, center.z + std::cos(a) * radius};
            wallTop[i] = Vec3{center.x + std::sin(a) * radius, galleryTopY, center.z + std::cos(a) * radius};
        }

        for (int i = 0; i < octSides; ++i) {
            const int next = (i + 1) % octSides;
            // Fundo luminoso interno da estufa (luz solar e lâmpadas quentes de mel)
            domeBc.addPoly(*this, {wallBot[i], wallBot[next], wallTop[next], wallTop[i]},
                           sf::Color(255, 232, 168, 120), sf::Color::Transparent);
            // Vidro translúcido da fachada com reflexo verde-água cel-shaded
            domeBc.addPoly(*this, {wallBot[i], wallBot[next], wallTop[next], wallTop[i]},
                           sf::Color(218, 246, 240, 165), hex(0x21322b));

            // Colunas de ferro forjado nas quinas da estufa
            domeBc.addLine(*this, wallBot[i], wallTop[i], hex(0x1e2e28), 6.5f);
            domeBc.addLine(*this, wallBot[i], wallTop[i], hex(0xd4aa4c), 2.2f);

            // Arco interno e bandeira envidraçada de cada baía
            const Vec3 midBot{ (wallBot[i].x + wallBot[next].x) * 0.5f, galleryBaseY, (wallBot[i].z + wallBot[next].z) * 0.5f };
            const Vec3 midTop{ (wallTop[i].x + wallTop[next].x) * 0.5f, galleryTopY, (wallTop[i].z + wallTop[next].z) * 0.5f };
            domeBc.addLine(*this, midBot, midTop, hex(0x283d34), 2.5f);
            const Vec3 midH{ midBot.x, galleryBaseY + (galleryTopY - galleryBaseY) * 0.65f, midBot.z };
            domeBc.addLine(*this, Vec3{wallBot[i].x, midH.y, wallBot[i].z}, Vec3{wallBot[next].x, midH.y, wallBot[next].z},
                           hex(0x283d34), 2.5f);
        }

        // Cornija e entablamento clássico moldurado no topo da parede
        drawSolidCylinderBatch(domeBc, Vec3{center.x, galleryTopY, center.z}, radius * 1.03f, radius * 1.05f,
                               12.0f, 8, 0.392f,
                               hex(0x8a7764), hex(0x736150), hex(0x524436), hex(0x251e18));

        // Cúpula abobadada de vidro em 3D (nervuras curvas e calota facetada)
        const float domeBaseY = galleryTopY + 12.0f;
        const int domeRings = isMain ? 4 : 3;
        const int domeSlices = isMain ? 12 : 8;
        const float sliceStep = 2.0f * kPi / domeSlices;

        for (int r = 0; r < domeRings; ++r) {
            const float t0 = static_cast<float>(r) / domeRings;
            const float t1 = static_cast<float>(r + 1) / domeRings;
            const float r0 = radius * std::cos(t0 * kPi * 0.5f);
            const float r1 = radius * std::cos(t1 * kPi * 0.5f);
            const float y0 = domeBaseY + std::sin(t0 * kPi * 0.5f) * domeH;
            const float y1 = domeBaseY + std::sin(t1 * kPi * 0.5f) * domeH;

            for (int s = 0; s < domeSlices; ++s) {
                const float a0 = s * sliceStep, a1 = (s + 1) * sliceStep;
                const Vec3 q0{center.x + std::sin(a0) * r0, y0, center.z + std::cos(a0) * r0};
                const Vec3 q1{center.x + std::sin(a1) * r0, y0, center.z + std::cos(a1) * r0};
                const Vec3 q2{center.x + std::sin(a1) * r1, y1, center.z + std::cos(a1) * r1};
                const Vec3 q3{center.x + std::sin(a0) * r1, y1, center.z + std::cos(a0) * r1};

                // Vidro facetado da abóbada
                const sf::Color domeGlass = (r % 2 == 0) ? sf::Color(228, 252, 246, 155)
                                                         : sf::Color(215, 245, 238, 145);
                domeBc.addPoly(*this, {q0, q1, q2, q3}, domeGlass, hex(0x23372f));
            }
        }

        // Nervuras estruturais de ferro forjado e aros dourados da cúpula
        for (int s = 0; s < domeSlices; ++s) {
            const float a = s * sliceStep;
            for (int r = 0; r < domeRings; ++r) {
                const float t0 = static_cast<float>(r) / domeRings;
                const float t1 = static_cast<float>(r + 1) / domeRings;
                const float r0 = radius * std::cos(t0 * kPi * 0.5f);
                const float r1 = radius * std::cos(t1 * kPi * 0.5f);
                const float y0 = domeBaseY + std::sin(t0 * kPi * 0.5f) * domeH;
                const float y1 = domeBaseY + std::sin(t1 * kPi * 0.5f) * domeH;

                const sf::Color ribCol = (s % 3 == 0) ? hex(0xdfaf3c) : hex(0x1a2822);
                const float ribThick = (isMain ? ((s % 3 == 0) ? 6.0f : 4.0f) : ((s % 3 == 0) ? 4.5f : 3.0f));
                domeBc.addLine(*this, Vec3{center.x + std::sin(a) * r0, y0, center.z + std::cos(a) * r0},
                               Vec3{center.x + std::sin(a) * r1, y1, center.z + std::cos(a) * r1},
                               ribCol, ribThick);
            }
        }

        // Aros horizontais estruturais da cúpula em cada anel
        for (int r = 0; r <= domeRings; ++r) {
            const float t = static_cast<float>(r) / domeRings;
            const float rRing = radius * std::cos(t * kPi * 0.5f);
            const float yRing = domeBaseY + std::sin(t * kPi * 0.5f) * domeH;
            const sf::Color ringCol = (r == 0 || r == 2) ? hex(0xdfaf3c) : hex(0x1a2822);
            const float ringThick = (r == 0) ? (isMain ? 7.0f : 5.0f) : (isMain ? 4.5f : 3.2f);
            for (int s = 0; s < domeSlices; ++s) {
                const float a0 = s * sliceStep, a1 = (s + 1) * sliceStep;
                domeBc.addLine(*this, Vec3{center.x + std::sin(a0) * rRing, yRing, center.z + std::cos(a0) * rRing},
                               Vec3{center.x + std::sin(a1) * rRing, yRing, center.z + std::cos(a1) * rRing},
                               ringCol, ringThick);
            }
        }

        // Lanterna de topo / Cúpula menor com pináculo real
        const float lanternY = domeBaseY + domeH;
        drawSolidCylinderBatch(domeBc, Vec3{center.x, lanternY, center.z}, radius * 0.18f, radius * 0.16f,
                               (isMain ? 38.0f : 22.0f), 6, 0.0f,
                               hex(0xd4a849), hex(0x2a3d34), hex(0x1a2923), hex(0xd4a849));

        // Pináculo de ouro e brasão real da abelha
        const float spireH = isMain ? 68.0f : 36.0f;
        const Vec3 spireBase{center.x, lanternY + (isMain ? 38.0f : 22.0f), center.z};
        const Vec3 spireTop{center.x, spireBase.y + spireH, center.z};
        domeBc.addLine(*this, spireBase, spireTop, hex(0xf0c652), 6.5f);

        if (isMain) {
            // Escultura 3D da Abelha Rainha Dourada no topo da cúpula central
            const Vec3 beeCrest{center.x, spireTop.y + 12.0f, center.z};
            drawSolidBoxBatch(domeBc, beeCrest, {26.0f, 16.0f, 32.0f}, 0.0f,
                              hex(0xfad65c), hex(0xdfaf3c), hex(0xc4952c), hex(0x6e4e16));
            // Asas douradas estendidas
            domeBc.addPoly(*this, {Vec3{beeCrest.x - 8.0f, beeCrest.y + 4.0f, beeCrest.z},
                                   Vec3{beeCrest.x - 38.0f, beeCrest.y + 22.0f, beeCrest.z - 12.0f},
                                   Vec3{beeCrest.x - 32.0f, beeCrest.y + 28.0f, beeCrest.z + 12.0f},
                                   Vec3{beeCrest.x - 8.0f, beeCrest.y + 8.0f, beeCrest.z + 8.0f}},
                           hex(0xffe682), hex(0x8c6620));
            domeBc.addPoly(*this, {Vec3{beeCrest.x + 8.0f, beeCrest.y + 4.0f, beeCrest.z},
                                   Vec3{beeCrest.x + 38.0f, beeCrest.y + 22.0f, beeCrest.z - 12.0f},
                                   Vec3{beeCrest.x + 32.0f, beeCrest.y + 28.0f, beeCrest.z + 12.0f},
                                   Vec3{beeCrest.x + 8.0f, beeCrest.y + 8.0f, beeCrest.z + 8.0f}},
                           hex(0xffe682), hex(0x8c6620));
            // Coroa imperial no topo da abelha
            domeBc.addLine(*this, Vec3{beeCrest.x, beeCrest.y + 8.0f, beeCrest.z},
                           Vec3{beeCrest.x, beeCrest.y + 24.0f, beeCrest.z},
                           hex(0xffe888), 4.5f);
        }
        domeBc.flush(queue_);
    };

    // Palácio de vidro: Cúpula central monumental e pavilhões laterais conectados trazidos para perto do jardim
    drawConservatoryDome(Vec3{0.0f, 0.0f, 1180.0f}, 250.0f, 150.0f, 170.0f, true);
    drawConservatoryDome(Vec3{-440.0f, 0.0f, 1100.0f}, 165.0f, 115.0f, 115.0f, false);
    drawConservatoryDome(Vec3{ 440.0f, 0.0f, 1100.0f}, 165.0f, 115.0f, 115.0f, false);
    drawConservatoryDome(Vec3{-820.0f, 0.0f, 1020.0f}, 130.0f, 90.0f, 85.0f, false);
    drawConservatoryDome(Vec3{ 820.0f, 0.0f, 1020.0f}, 130.0f, 90.0f, 85.0f, false);

    // Galerias de ligação envidraçadas entre a cúpula central e os pavilhões
    BatchCollector gallBc(0.067f);
    for (float side : {-1.0f, 1.0f}) {
        const float x0 = side * 190.0f, x1 = side * 350.0f;
        const float zGall = 1135.0f;
        drawSolidBoxBatch(gallBc, Vec3{(x0 + x1) * 0.5f, 35.0f, zGall}, {std::fabs(x1 - x0), 75.0f, 80.0f}, 0.0f,
                          sf::Color(220, 248, 242, 160), sf::Color(255, 230, 160, 110),
                          hex(0x21322b), hex(0x1a2822));
        gallBc.addLine(*this, Vec3{x0, 72.0f, zGall}, Vec3{x1, 72.0f, zGall}, hex(0xd4ab4b), 3.5f);
    }
    gallBc.flush(queue_);

    // Portal de Entrada envidraçado monumental integrado na fachada da Cúpula Central (edifício isolado no jardim)
    const float domeFrontZ = 949.0f;
    const float portalBaseY = 15.0f;
    const float portalH = 68.0f;
    BatchCollector portalBc(0.082f);
    // Moldura vitoriana em arco dourado e ferro forjado em torno das portas de vidro
    portalBc.addLine(*this, Vec3{-52.0f, portalBaseY, domeFrontZ - 1.0f}, Vec3{-52.0f, portalBaseY + portalH, domeFrontZ - 1.0f}, hex(0xdfaf3c), 5.5f);
    portalBc.addLine(*this, Vec3{ 52.0f, portalBaseY, domeFrontZ - 1.0f}, Vec3{ 52.0f, portalBaseY + portalH, domeFrontZ - 1.0f}, hex(0xdfaf3c), 5.5f);
    portalBc.addLine(*this, Vec3{-52.0f, portalBaseY + portalH, domeFrontZ - 1.0f}, Vec3{ 52.0f, portalBaseY + portalH, domeFrontZ - 1.0f}, hex(0xdfaf3c), 5.5f);
    // Portas de vidro integradas na fachada com luz quente de mel no interior da estufa
    drawSolidBoxBatch(portalBc, Vec3{0.0f, portalBaseY + portalH * 0.5f, domeFrontZ - 2.0f}, {100.0f, portalH, 2.0f}, 0.0f,
                      sf::Color(255, 238, 175, 170), hex(0x21322b), hex(0x182620), hex(0xdfaf3c));
    // Divisórias das portas de vidro
    portalBc.addLine(*this, Vec3{0.0f, portalBaseY, domeFrontZ - 3.0f}, Vec3{0.0f, portalBaseY + portalH, domeFrontZ - 3.0f}, hex(0xdfaf3c), 3.5f);
    portalBc.addLine(*this, Vec3{-50.0f, portalBaseY + portalH * 0.5f, domeFrontZ - 3.0f}, Vec3{50.0f, portalBaseY + portalH * 0.5f, domeFrontZ - 3.0f}, hex(0x283d34), 2.5f);
    // Escadaria frontal de cantaria conectando a soleira dos domos ao gramado do jardim (-35)
    for (int step = 0; step < 4; ++step) {
        const float stZ0 = domeFrontZ - 3.0f - (step + 1) * 7.0f;
        const float stZ1 = stZ0 + 7.0f;
        const float stY = -35.0f + (3 - step) * 12.5f;
        portalBc.addPoly(*this, {Vec3{-65.0f, stY, stZ0}, Vec3{65.0f, stY, stZ0},
                                 Vec3{65.0f, stY, stZ1}, Vec3{-65.0f, stY, stZ1}},
                         hex(step % 2 ? 0x9d8d78 : 0x8a7764), hex(0x352b22));
        portalBc.addLine(*this, Vec3{-65.0f, stY, stZ1}, Vec3{65.0f, stY, stZ1}, hex(0xbfa679), 2.5f);
    }
    portalBc.flush(queue_);

    // Lanternas de latão dourado ladeando o portal de entrada dos domos
    for (float side : {-1.0f, 1.0f}) {
        const Vec3 lPos{side * 60.0f, portalBaseY + 22.0f, domeFrontZ - 2.0f};
        drawSolidBox(Vec3{lPos.x, portalBaseY + 12.0f, lPos.z}, {8.0f, 20.0f, 8.0f}, 0.0f,
                     hex(0x8a7764), hex(0x736150), hex(0x564738), hex(0x271e17), 0.084f);
        drawHexLantern(Vec3{lPos.x, portalBaseY + 28.0f, lPos.z}, 9.0f, 16.0f, 1.5f, 0.085f);
    }

    // =========================================================================
    // 3. O TERRAÇO REAL, BALAUSTRADA VITORIANA E VASOS DE FLORES
    // A plataforma nobre que envolve o ringue de boxe
    // =========================================================================
    constexpr int kSectors = 32;
    constexpr float kSeg = 2.0f * kPi / kSectors;

    // Piso de cantaria de pedra polida ao redor do ringue (raio 230 a 430)
    BatchCollector terraceFloor(0.092f);
    for (int i = 0; i < kSectors; ++i) {
        const float a0 = i * kSeg, a1 = (i + 1) * kSeg;
        const sf::Color stoneCol = (i % 2 == 0) ? hex(0x8e7e6a) : hex(0x9d8d78);
        terraceFloor.addPoly(*this, {polar(a0, 230.0f, -35.0f), polar(a1, 230.0f, -35.0f),
                                     polar(a1, 430.0f, -35.0f), polar(a0, 430.0f, -35.0f)},
                             stoneCol, hex(0x4a3f33));
    }
    terraceFloor.flush(queue_);

    // Degrau e terraço superior (Belvedere) (raio 430 a 510, altura -15)
    BatchCollector stepRiser(0.095f);
    BatchCollector stepTrim(0.096f);
    BatchCollector walkFloor(0.097f);
    for (int i = 0; i < kSectors; ++i) {
        const float a0 = i * kSeg, a1 = (i + 1) * kSeg;
        // Espelho do degrau
        stepRiser.addPoly(*this, {polar(a0, 430.0f, -35.0f), polar(a1, 430.0f, -35.0f),
                                  polar(a1, 430.0f, -15.0f), polar(a0, 430.0f, -15.0f)},
                         hex(0x5e4f40), hex(0x2a221b));
        // Friso do degrau
        stepTrim.addLine(*this, polar(a0, 428.0f, -14.0f), polar(a1, 428.0f, -14.0f), hex(0xb39e87), 3.0f);

        // Piso do passeio superior
        const sf::Color walkCol = (i % 2 == 0) ? hex(0x998873) : hex(0xa69580);
        walkFloor.addPoly(*this, {polar(a0, 430.0f, -15.0f), polar(a1, 430.0f, -15.0f),
                                  polar(a1, 510.0f, -15.0f), polar(a0, 510.0f, -15.0f)},
                          walkCol, hex(0x463b2f));
    }
    stepRiser.flush(queue_);
    stepTrim.flush(queue_);
    walkFloor.flush(queue_);

    // Balaustrada clássica de pedra moldurada (raio 505) contornando todo o terraço do ringue
    BatchCollector balBase(0.100f);
    BatchCollector balPlinth(0.101f);
    BatchCollector balusters(0.102f);
    BatchCollector handrail(0.103f);
    BatchCollector handrailLine(0.104f);
    BatchCollector urnsBatch(0.106f);

    for (int i = 0; i < kSectors; ++i) {
        const float a0 = i * kSeg, a1 = (i + 1) * kSeg;
        const Vec3 mid = polar((a0 + a1) * 0.5f, 505.0f, 0.0f);
        if (isCulled(mid, 75.0f)) continue;

        // Rodapé da balaustrada
        balBase.addPoly(*this, {polar(a0, 498.0f, -15.0f), polar(a1, 498.0f, -15.0f),
                                polar(a1, 498.0f, -2.0f), polar(a0, 498.0f, -2.0f)},
                        hex(0x6e5c4a), hex(0x2d241c));
        balPlinth.addPoly(*this, {polar(a0, 498.0f, -2.0f), polar(a1, 498.0f, -2.0f),
                                  polar(a1, 512.0f, -2.0f), polar(a0, 512.0f, -2.0f)},
                          hex(0x8f7c67), hex(0x2d241c));

        // Balaústres torneados de pedra (mini-colunas verticais)
        for (float sub : {0.25f, 0.75f}) {
            const float bA = a0 + kSeg * sub;
            const Vec3 bRoot = polar(bA, 505.0f, -2.0f);
            drawSolidCylinderBatch(balusters, bRoot, 4.0f, 3.5f, 22.0f, 6, 0.0f,
                                   hex(0xa6947e), hex(0x91806c), hex(0x645443), hex(0x32281e));
        }

        // Peitoril / Corrimão moldurado superior
        handrail.addPoly(*this, {polar(a0, 496.0f, 20.0f), polar(a1, 496.0f, 20.0f),
                                 polar(a1, 514.0f, 20.0f), polar(a0, 514.0f, 20.0f)},
                         hex(0xa89680), hex(0x382c20));
        handrail.addPoly(*this, {polar(a0, 496.0f, 20.0f), polar(a1, 496.0f, 20.0f),
                                 polar(a1, 496.0f, 26.0f), polar(a0, 496.0f, 26.0f)},
                         hex(0x766350), hex(0x382c20));
        handrailLine.addLine(*this, polar(a0, 495.0f, 26.0f), polar(a1, 495.0f, 26.0f), hex(0xbfae99), 3.0f);

        // Pilares mestres e vasos ornamentais a cada 45 graus
        if (i % 4 == 0) {
            const Vec3 pilRoot = polar(a0, 505.0f, -15.0f);
            drawSolidBox(Vec3{pilRoot.x, 9.0f, pilRoot.z}, {24.0f, 48.0f, 24.0f}, a0,
                         hex(0xa89680), hex(0x897764), hex(0x6e5c4a), hex(0x2f241a), 0.105f);

            // Vaso ornamental de pedra clássico com flores
            const Vec3 urnPos{pilRoot.x, 33.0f, pilRoot.z};
            drawSolidCylinderBatch(urnsBatch, urnPos, 8.5f, 14.0f, 18.0f, 8, a0,
                                   hex(0x3c2a1c), hex(0x9d8b76), hex(0x685745), hex(0x2b2118));
            // Rosas vermelhas e folhagens caindo do vaso sobre a balaustrada
            draw3DRoseBush(Vec3{urnPos.x, urnPos.y + 16.0f, urnPos.z}, 16.0f, 18.0f, 4, i);
        }
    }
    balBase.flush(queue_);
    balPlinth.flush(queue_);
    balusters.flush(queue_);
    handrail.flush(queue_);
    handrailLine.flush(queue_);
    urnsBatch.flush(queue_);

    // =========================================================================
    // 4. A GRANDE ESPLANADA REAL, BALAUSTRADAS E A AVENIDA DA ESTUFA
    // Começa logo atrás do ringue (z = 210) a y = -2, com balustrada e vasos
    // =========================================================================
    BatchCollector lawnBatch(0.052f);
    for (int i = 0; i < kSectors; ++i) {
        const float a0 = i * kSeg, a1 = (i + 1) * kSeg;
        // Anel 1: Passeio ao redor do terraço
        lawnBatch.addPoly(*this, {polar(a0, 510.0f, -35.0f), polar(a1, 510.0f, -35.0f),
                                  polar(a1, 950.0f, -35.0f), polar(a0, 950.0f, -35.0f)},
                          hex(i % 2 ? 0x436c34 : 0x4d7a3c), sf::Color::Transparent);
        // Anel 2: Jardins intermediários e campos de flores
        lawnBatch.addPoly(*this, {polar(a0, 950.0f, -35.0f), polar(a1, 950.0f, -35.0f),
                                  polar(a1, 1850.0f, -35.0f), polar(a0, 1850.0f, -35.0f)},
                          hex(i % 2 ? 0x4a7538 : 0x54823f), sf::Color::Transparent);
        // Anel 3: Pôlderes e prados campestres
        lawnBatch.addPoly(*this, {polar(a0, 1850.0f, -35.0f), polar(a1, 1850.0f, -35.0f),
                                  polar(a1, 4200.0f, -35.0f), polar(a0, 4200.0f, -35.0f)},
                          hex(i % 2 ? 0x426632 : 0x4b7238), sf::Color::Transparent);
        // Anel 4: Planície holandesa distante até os pés das colinas (r = 12000)
        lawnBatch.addPoly(*this, {polar(a0, 4200.0f, -35.0f), polar(a1, 4200.0f, -35.0f),
                                  polar(a1, 12000.0f, -35.0f), polar(a0, 12000.0f, -35.0f)},
                          hex(i % 2 ? 0x39552c : 0x405f32), sf::Color::Transparent);
    }
    lawnBatch.flush(queue_);

    // Sebes de buxinho francês recortado emoldurando os canteiros do jardim
    for (float side : {-1.0f, 1.0f}) {
        const float hx = side * 220.0f;
        const Vec3 hedgePos{hx, -25.0f, 680.0f};
        if (!isCulled(hedgePos, 140.0f)) {
            drawSolidBox(hedgePos, {20.0f, 20.0f, 240.0f}, 0.0f,
                         hex(0x244b22), hex(0x1a3d19), hex(0x1a3d19), hex(0x112710), 0.066f);
        }
    }

    // =========================================================================
    // 5. AS QUATRO LAMPARINAS VITORIANAS NOS CANTOS DO RINGUE
    // =========================================================================
    const std::array<sf::Vector2f, 4> lampCorners = {{{-365.0f, -365.0f},
                                                      { 365.0f, -365.0f},
                                                      { 365.0f,  365.0f},
                                                      {-365.0f,  365.0f}}};
    for (std::size_t c = 0; c < lampCorners.size(); ++c) {
        const Vec3 lRoot{lampCorners[c].x, -35.0f, lampCorners[c].y};
        if (isCulled(lRoot, 50.0f)) continue;
        const float signX = std::copysign(1.0f, lRoot.x);
        const float signZ = std::copysign(1.0f, lRoot.z);

        // Pedestal e base octogonal de ferro fundido
        drawSolidCylinder(lRoot, 18.0f, 14.0f, 22.0f, 8, 0.0f,
                          hex(0x2b3136), hex(0x202528), hex(0x15181a), hex(0x0e1012), 0.218f);

        // Coluna canelada com anéis de latão torneado
        drawSolidCylinder(Vec3{lRoot.x, -13.0f, lRoot.z}, 6.5f, 5.0f, 195.0f, 8, 0.0f,
                          hex(0xd4a849), hex(0x2b3136), hex(0x181c1e), hex(0x0e1012), 0.220f);
        for (float ringY : {25.0f, 95.0f, 168.0f}) {
            drawSolidCylinder(Vec3{lRoot.x, ringY, lRoot.z}, 8.0f, 8.0f, 6.0f, 8, 0.0f,
                              hex(0xf0c558), hex(0xdfab3c), hex(0xa67c24), hex(0x4a340b), 0.221f);
        }

        // Braço em voluta curvada sustentando a lanterna em direção ao ringue
        const Vec3 armEnd{lRoot.x - signX * 32.0f, 195.0f, lRoot.z - signZ * 32.0f};
        queueWorldLine(Vec3{lRoot.x, 180.0f, lRoot.z}, Vec3{lRoot.x, 212.0f, lRoot.z}, hex(0x202528), 6.0f, 0.222f);
        queueWorldLine(Vec3{lRoot.x, 212.0f, lRoot.z}, Vec3{armEnd.x, 206.0f, armEnd.z}, hex(0x202528), 5.5f, 0.222f);
        queueWorldLine(Vec3{armEnd.x, 206.0f, armEnd.z}, armEnd, hex(0x202528), 5.0f, 0.222f);

        // Lanterna hexagonal de mel suspensa
        const float flicker = std::sin(static_cast<float>(frameCounter_) * 0.16f + c * 1.8f) * 3.5f;
        drawHexLantern(armEnd, 17.0f, 32.0f, flicker, 0.225f);
    }

    // =========================================================================
    // 6. AS ESTAÇÕES DE APICULTURA REAL NAS LATERAIS (FORA DAS ARQUIBANCADAS)
    // Localizadas no gramado botânico externo, sem ocupar nem invadir o lugar da plateia.
    // =========================================================================
    // Apiário da lateral esquerda (Oeste, no jardim botânico além das arquibancadas)
    if (!isCulled(Vec3{-590.0f, -34.0f, 35.0f}, 160.0f)) {
        drawSolidBox(Vec3{-590.0f, -34.0f, 35.0f}, {80.0f, 3.0f, 220.0f}, 0.12f,
                     hex(0x5a3c22), hex(0x442c18), hex(0x442c18), hex(0x22140a), 0.082f);
        draw3DBeehive(Vec3{-580.0f, -32.0f, 85.0f}, -1.30f, 101);
        draw3DBeehive(Vec3{-595.0f, -32.0f, 20.0f}, -1.20f, 102);
        draw3DBeehive(Vec3{-580.0f, -32.0f, -45.0f}, -1.35f, 103);
        draw3DHoneyBarrel(Vec3{-610.0f, -32.0f, 125.0f}, 14.0f, 28.0f, -0.4f);
        draw3DHoneyBarrel(Vec3{-610.0f, -32.0f, -75.0f}, 14.0f, 28.0f, 0.4f);
    }

    // Apiário da lateral direita (Leste, no jardim botânico além das arquibancadas)
    if (!isCulled(Vec3{ 590.0f, -34.0f, 35.0f}, 160.0f)) {
        drawSolidBox(Vec3{ 590.0f, -34.0f, 35.0f}, {80.0f, 3.0f, 220.0f}, -0.12f,
                     hex(0x5a3c22), hex(0x442c18), hex(0x442c18), hex(0x22140a), 0.082f);
        draw3DBeehive(Vec3{ 580.0f, -32.0f, 85.0f}, 1.30f, 104);
        draw3DBeehive(Vec3{ 595.0f, -32.0f, 20.0f}, 1.20f, 105);
        draw3DBeehive(Vec3{ 580.0f, -32.0f, -45.0f}, 1.35f, 106);
        draw3DHoneyBarrel(Vec3{ 610.0f, -32.0f, 125.0f}, 14.0f, 28.0f, 0.4f);
        draw3DHoneyBarrel(Vec3{ 610.0f, -32.0f, -75.0f}, 14.0f, 28.0f, -0.4f);
    }

    // Estações de colmeias nos campos botânicos amplos
    const std::array<float, 4> hiveAngles = {{-0.75f, 0.75f, -1.25f, 1.25f}};
    for (std::size_t i = 0; i < hiveAngles.size(); ++i) {
        const float a = hiveAngles[i];
        const float radius = 1100.0f + static_cast<float>((i % 2) * 80);
        const Vec3 hRoot = polar(a, radius, -35.0f);
        const float hiveYaw = a + kPi + ((i % 2 == 0) ? 0.25f : -0.25f);

        draw3DBeehive(hRoot, hiveYaw, static_cast<int>(i + 20));
        const float sideSign = (i % 2 == 0) ? 1.0f : -1.0f;
        const Vec3 bRoot{hRoot.x + std::cos(hiveYaw) * (sideSign * 48.0f),
                         -35.0f,
                         hRoot.z - std::sin(hiveYaw) * (sideSign * 48.0f)};
        draw3DHoneyBarrel(bRoot, 14.0f, 28.0f, hiveYaw * 0.7f);
    }

    // =========================================================================
    // 7. OS GIRASSÓIS GIGANTES (CAMPOS DOURADOS EM 3D COM PÉTALAS ARREDONDADAS)
    // =========================================================================
    // Girassóis do campo oeste (esquerda)
    for (int i = 0; i < 11; ++i) {
        const float a = -0.45f - static_cast<float>(i) * 0.12f;
        const float r = 880.0f + static_cast<float>((i * 47) % 280);
        const Vec3 root = polar(a, r, -35.0f);
        const float h = 145.0f + static_cast<float>((i * 19) % 50);
        const float faceYaw = a + kPi + 0.15f * std::sin(static_cast<float>(i));
        draw3DSunflower(root, h, 0.22f, faceYaw, 1.0f, i);
    }
    // Girassóis do campo leste (direita)
    for (int i = 0; i < 11; ++i) {
        const float a = 0.45f + static_cast<float>(i) * 0.12f;
        const float r = 880.0f + static_cast<float>((i * 47) % 280);
        const Vec3 root = polar(a, r, -35.0f);
        const float h = 145.0f + static_cast<float>((i * 19) % 50);
        const float faceYaw = a + kPi - 0.15f * std::sin(static_cast<float>(i));
        draw3DSunflower(root, h, 0.22f, faceYaw, 1.0f, i + 12);
    }
    // Girassóis do campo sul (em direção aos moinhos de vento)
    for (int i = 0; i < 8; ++i) {
        const float a = kPi - 0.65f + static_cast<float>(i) * 0.18f;
        const float r = 1150.0f + static_cast<float>((i * 31) % 220);
        const Vec3 root = polar(a, r, -35.0f);
        const float h = 150.0f + static_cast<float>((i * 23) % 40);
        const float faceYaw = a + 0.2f * std::cos(static_cast<float>(i));
        draw3DSunflower(root, h, 0.20f, faceYaw, 1.05f, i + 24);
    }
    // Girassóis margeando a Grande Alameda Real (afastados lateralmente para emoldurar a esplanada sem bloquear a visão)
    draw3DSunflower(Vec3{-210.0f, -35.0f, 520.0f}, 175.0f, 0.20f,  0.42f, 1.05f, 101);
    draw3DSunflower(Vec3{ 210.0f, -35.0f, 520.0f}, 175.0f, 0.20f, -0.42f, 1.05f, 102);
    draw3DSunflower(Vec3{-225.0f, -35.0f, 780.0f}, 180.0f, 0.22f,  0.32f, 1.10f, 103);
    draw3DSunflower(Vec3{ 225.0f, -35.0f, 780.0f}, 180.0f, 0.22f, -0.32f, 1.10f, 104);

    // =========================================================================
    // 8. AS ROSEIRAS REAIS E CANTEIROS BOTÂNICOS
    // =========================================================================
    // Roseiras ao longo da balaustrada do jardim
    for (int i = 0; i < 8; ++i) {
        const float a = (i + 0.5f) * 2.0f * kPi / 8.0f;
        const Vec3 rRoot = polar(a, 550.0f, -35.0f);
        draw3DRoseBush(rRoot, 32.0f, 48.0f, 7, i * 3);
    }

    // =========================================================================
    // 9. OS MOINHOS DE VENTO HOLANDESES (STELLINGMOLENS 3D)
    // =========================================================================
    // Moinhos ao Norte nas colinas holandesas (visíveis em combate flanqueando os domos)
    drawDutchWindmill(Vec3{-1050.0f, -35.0f, 1420.0f}, 1.55f,  0.28f, 6);
    drawDutchWindmill(Vec3{ 1050.0f, -35.0f, 1420.0f}, 1.55f, -0.28f, 7);
    drawDutchWindmill(Vec3{-1680.0f, -35.0f, 1950.0f}, 1.25f,  0.45f, 8);
    drawDutchWindmill(Vec3{ 1680.0f, -35.0f, 1950.0f}, 1.25f, -0.45f, 9);

    // Moinho 1: O Moinho Real Central (imponente ao Sul, visível ao olhar para trás / replay)
    drawDutchWindmill(Vec3{0.0f, -35.0f, -1350.0f}, 1.65f, 0.05f, 1);

    // Moinho 2: Moinho a Sudoeste
    drawDutchWindmill(Vec3{-560.0f, -35.0f, -1650.0f}, 1.38f, -0.22f, 2);

    // Moinho 3: Moinho a Sudeste
    drawDutchWindmill(Vec3{ 580.0f, -35.0f, -1720.0f}, 1.42f, 0.25f, 3);

    // Moinhos distantes nas colinas
    drawDutchWindmill(Vec3{-1250.0f, -35.0f, -2350.0f}, 1.10f, -0.45f, 4);
    drawDutchWindmill(Vec3{ 1280.0f, -35.0f, -2450.0f}, 1.10f,  0.45f, 5);

    // =========================================================================
    // 10. OS ARVOREDOS HOLANDESES (CHOUPOS ESGUIOS & CARVALHOS COPADOS)
    // =========================================================================
    // Bosque emoldurando os moinhos ao Sul
    const std::array<sf::Vector2f, 8> southTrees = {{{-220.0f, -1250.0f},
                                                     { 240.0f, -1280.0f},
                                                     {-420.0f, -1520.0f},
                                                     { 450.0f, -1580.0f},
                                                     {-780.0f, -1850.0f},
                                                     { 820.0f, -1920.0f},
                                                     {-1050.0f, -2100.0f},
                                                     { 1100.0f, -2200.0f}}};
    for (std::size_t t = 0; t < southTrees.size(); ++t) {
        const int style = (t % 2 == 0) ? 0 : 1; // 0 = Choupo, 1 = Carvalho
        const float trH = (style == 0) ? 90.0f : 75.0f;
        const float cR = (style == 0) ? 38.0f : 55.0f;
        const float cH = (style == 0) ? 140.0f : 95.0f;
        drawDutchTree(Vec3{southTrees[t].x, -35.0f, southTrees[t].y}, trH, cR, cH, style, static_cast<int>(t));
    }

    // Bosque emoldurando a Estufa ao Norte
    const std::array<sf::Vector2f, 6> northTrees = {{{-620.0f, 1260.0f},
                                                     { 620.0f, 1260.0f},
                                                     {-980.0f, 1180.0f},
                                                     { 980.0f, 1180.0f},
                                                     {-1150.0f, 1350.0f},
                                                     { 1150.0f, 1350.0f}}};
    for (std::size_t t = 0; t < northTrees.size(); ++t) {
        const int style = (t % 3 == 0) ? 1 : 0;
        const float trH = (style == 0) ? 95.0f : 80.0f;
        const float cR = (style == 0) ? 40.0f : 58.0f;
        const float cH = (style == 0) ? 145.0f : 100.0f;
        drawDutchTree(Vec3{northTrees[t].x, -35.0f, northTrees[t].y}, trH, cR, cH, style, static_cast<int>(t + 20));
    }

    // Fileiras laterais de choupos (pôlderes holandeses a Leste e Oeste)
    for (float side : {-1.0f, 1.0f}) {
        for (int p = 0; p < 5; ++p) {
            const float px = side * (1250.0f + p * 40.0f);
            const float pz = -600.0f + p * 300.0f;
            drawDutchTree(Vec3{px, -35.0f, pz}, 85.0f, 35.0f, 130.0f, 0, static_cast<int>(p + (side > 0 ? 50 : 60)));
        }
    }

    // =========================================================================
    // 11. OS VIBRANTES CAMPOS DE LAVANDA (PARTERRES ROXOS 3D NO JARDIM EXTERNO)
    // Localizados inteiramente fora das arquibancadas, nos gramados dos jardins botânicos
    // =========================================================================
    // 1. Parterres reais de lavanda nos jardins ao norte flanqueando a Grande Alameda
    drawLavenderField(Vec3{-360.0f, -35.0f, 680.0f}, 120.0f, 300.0f, 0.05f, 4);
    drawLavenderField(Vec3{ 360.0f, -35.0f, 680.0f}, 120.0f, 300.0f, -0.05f, 4);

    // 2. Parterres laterais no gramado externo ao lado das colmeias (completamente fora da arquibancada)
    drawLavenderField(Vec3{-640.0f, -35.0f, 60.0f}, 110.0f, 240.0f, 0.0f, 4);
    drawLavenderField(Vec3{ 640.0f, -35.0f, 60.0f}, 110.0f, 240.0f, 0.0f, 4);

    // 3. Campos de lavanda amplos no horizonte sul (ao lado dos moinhos de vento)
    drawLavenderField(Vec3{-480.0f, -35.0f, -1080.0f}, 220.0f, 360.0f, -0.22f, 5);
    drawLavenderField(Vec3{ 480.0f, -35.0f, -1080.0f}, 220.0f, 360.0f, 0.22f, 5);
}

void RingRenderer::queueStands() {
    // 4 degraus em anel de 32 segmentos: piso, faixa de assento, espelho do degrau, friso, grade e refletores.
    const bool harbor = arenaTheme_ == ArenaTheme::JoanaHarbor;
    const bool pernambuco = arenaTheme_ == ArenaTheme::ValeriaPernambuco;
    const bool observatory = arenaTheme_ == ArenaTheme::MarcovObservatory;
    const bool swamp = arenaTheme_ == ArenaTheme::FrogSwamp;
    const bool concert = arenaTheme_ == ArenaTheme::VivaldiConcertHall;
    const bool parlor = arenaTheme_ == ArenaTheme::CremosoParlor;
    const bool courtyard = arenaTheme_ == ArenaTheme::EspinhaCourtyard;
    const bool garden = arenaTheme_ == ArenaTheme::JackGarden;
    if (swamp || garden) return; // O jardim e o pântano modelam seus próprios terraços e plataformas.
    struct Geo { float rIn, rOut, h, hPrev; };
    struct Style { uint32_t platform, seat, riser, trim; };
    const std::array<Geo, 4> geo = {{{270, 410, -35, -35}, {410, 510, 18, -35}, {510, 620, 68, 18}, {620, 730, 118, 68}}};
    const std::array<Style, 4> centralStyle = {{{0x18202c, 0x222c3c, 0x0c1018, 0x334155}, {0x131b26, 0x1c2534, 0x090d13, 0x283547},
                                                {0x0d1118, 0x131a24, 0x05070a, 0x1e293b}, {0x080c12, 0x0e141c, 0x030508, 0x151d2a}}};
    const std::array<Style, 4> harborStyle = {{{0x263d3d, 0x405452, 0x172b31, 0x98724d}, {0x293c3b, 0x49605a, 0x14282b, 0xa27e54},
                                               {0x223332, 0x3d5651, 0x102328, 0x8b6a46}, {0x1a2b2d, 0x344a48, 0x0a1d24, 0x715a40}}};
    const std::array<Style, 4> pernambucoStyle = {{{0x775555, 0x996b5d, 0x503b4b, 0xf1bb72},
                                                   {0x624955, 0x7b5960, 0x413547, 0xe9ad6b},
                                                   {0x504455, 0x694e61, 0x342f45, 0xd79866},
                                                   {0x413b52, 0x544460, 0x2c2b40, 0xc78961}}};
    const std::array<Style, 4> observatoryStyle = {{{0x223849, 0x35566a, 0x142634, 0x67b8c8},
                                                    {0x1c3044, 0x2d4a60, 0x102332, 0x5ea7bc},
                                                    {0x192a3e, 0x294258, 0x0e1c2a, 0x5296ae},
                                                    {0x142235, 0x23374b, 0x0b1625, 0x487d9a}}};
    const std::array<Style, 4> swampStyle = {{{0x455a43, 0x667757, 0x293c31, 0xa5b26d},
                                             {0x3c513d, 0x596e50, 0x22382d, 0x93ab6a},
                                             {0x334b39, 0x4b6347, 0x1c3027, 0x829e61},
                                             {0x2b4034, 0x405b41, 0x182b24, 0x6f905b}}};
    const std::array<Style, 4> concertStyle = {{{0x5e3942, 0x794552, 0x372232, 0xc29565},
                                               {0x593342, 0x713d51, 0x321f31, 0xc29565},
                                               {0x4a2b3d, 0x62364b, 0x2b1b2c, 0xb68762},
                                               {0x40263a, 0x59334a, 0x241729, 0x9c705a}}};
    const std::array<Style, 4> parlorStyle = {{{0xe3b38a, 0xf1d4b1, 0x9a6370, 0x5cafa0},
                                              {0xd59f83, 0xe6bda5, 0x855766, 0x65b7a8},
                                              {0xc68c79, 0xdba995, 0x784c5d, 0x5aa99d},
                                              {0xb57a72, 0xcf9b8d, 0x6c4357, 0x52998f}}};
    const std::array<Style, 4> courtyardStyle = {{{0x834438, 0x9d5747, 0x552f3a, 0xc18b54},
                                                  {0x784039, 0x914d43, 0x4b2b37, 0xb98150},
                                                  {0x6d3b39, 0x864742, 0x452936, 0xae794e},
                                                  {0x613638, 0x79413f, 0x3e2734, 0xa06e49}}};
    const std::array<Style, 4> gardenStyle = {{{0x3d6032, 0x867352, 0x241d17, 0xd4a849},
                                               {0x4c4338, 0x6f604b, 0x201c18, 0xb88e44},
                                               {0x423830, 0x60503f, 0x1c1916, 0x9e7a3b},
                                               {0x383028, 0x504334, 0x171513, 0x886632}}};
    const auto& style = harbor ? harborStyle : pernambuco ? pernambucoStyle
                        : observatory ? observatoryStyle : swamp ? swampStyle
                        : concert ? concertStyle : parlor ? parlorStyle
                        : courtyard ? courtyardStyle : garden ? gardenStyle : centralStyle;
    constexpr int kSegs = 32;
    const float seg = 2.0f * kPi / kSegs;
    for (std::size_t t = 0; t < geo.size(); ++t) {
        // Duas fileiras de pedra bastam; as arquibancadas altas escondiam o brejo.
        if ((swamp || parlor || courtyard || garden) && t >= 2) break;
        const Geo& g = geo[t];
        const Style& st = style[t];
        for (int s = 0; s < kSegs; ++s) {
            const float a = s * seg, n = (s + 1) * seg;
            const float sa = std::sin(a), ca = std::cos(a), sn = std::sin(n), cn = std::cos(n);
            auto ring = [&](float r, float y, float ang) { return Vec3{std::sin(ang) * r, y, std::cos(ang) * r}; };
            const sf::Color platCol = (garden && t == 0) ? hex(s % 2 ? 0x3e6232 : 0x486f3a) : hex(st.platform);
            queueWorldPolygon({ring(g.rIn, g.h, a), ring(g.rIn, g.h, n), ring(g.rOut, g.h, n), ring(g.rOut, g.h, a)}, platCol, hex(0x080a0f), 0.2f);
            if (t > 0) {
                const float r = g.rIn + 18.0f;
                queueWorldPolygon({ring(r, g.h + 1, a), ring(r, g.h + 1, n), ring(r + 26, g.h + 1, n), ring(r + 26, g.h + 1, a)}, hex(st.seat), hex(0x0c1018), 0.21f);
            }
            if (g.h != g.hPrev) {
                queueWorldPolygon({ring(g.rIn, g.hPrev, a), ring(g.rIn, g.hPrev, n), ring(g.rIn, g.h, n), ring(g.rIn, g.h, a)}, hex(st.riser), hex(0x05070a), 0.2f);
            }
            queueWorldLine({sa * g.rIn, g.h, ca * g.rIn}, {sn * g.rIn, g.h, cn * g.rIn}, hex(st.trim), 2.0f, 0.22f);
            if (t == 1) {
                queueWorldLine({sa * g.rIn, g.h + 14, ca * g.rIn}, {sn * g.rIn, g.h + 14, cn * g.rIn},
                               harbor ? hex(0x96764d) : pernambuco ? hex(0xe5a86d)
                               : observatory ? hex(0x6aaebe) : swamp ? hex(0xa7bd7c)
                               : concert ? hex(0xd6a974)
                               : parlor ? hex(0x6bbbae)
                               : courtyard ? hex(0xc5915c) : garden ? hex(0xb99853) : hex(0x253042), 3.0f, 0.24f);
                queueWorldLine({sa * g.rIn, g.h, ca * g.rIn}, {sa * g.rIn, g.h + 14, ca * g.rIn},
                               harbor ? hex(0x54432e) : pernambuco ? hex(0x634754)
                               : observatory ? hex(0x294454) : swamp ? hex(0x3b573e)
                               : concert ? hex(0x76475a)
                               : parlor ? hex(0x8e5d68)
                               : courtyard ? hex(0x714146) : garden ? hex(0x526540) : hex(0x1d2636), 2.0f, 0.24f);
            }
            if (t == geo.size() - 1 && s % 8 == 0) {
                const float rx = sa * g.rOut, rz = ca * g.rOut;
                queueWorldLine({rx, g.h, rz}, {rx, g.h + 45, rz}, harbor ? hex(0x61482e)
                                                                    : pernambuco ? hex(0x6d4b4d)
                                                                    : observatory ? hex(0x3c6477)
                                                                    : swamp ? hex(0x4d6b4d)
                                                                    : concert ? hex(0xb68762)
                                                                    : parlor ? hex(0x6bbbae)
                                                                    : courtyard ? hex(0xd69d68) : hex(0x2b3648), 4.0f, 0.25f);
                if (auto p = camera_.project({rx, g.h + 45, rz})) {
                    sf::Vector2f c = p->pos;
                    float w = 16.0f * p->scale, h = 12.0f * p->scale;
                    const sf::Color lampColor = harbor ? hex(0xffbb6c)
                                                 : pernambuco ? hex(0xffcf89)
                                                 : observatory ? hex(0x8de5ef)
                                                 : swamp ? hex(0xdaf2a0)
                                                 : concert ? hex(0xf6d29c)
                                                 : parlor ? hex(0xffe4bf)
                                                 : courtyard ? hex(0xffd896) : hex(0xfef08a);
                    queue_.push_back({0.26f, p->depth, [c, w, h, lampColor](sf::RenderTarget& tg) {
                                          sf::RectangleShape lamp({w, h});
                                          lamp.setPosition({c.x - w / 2.0f, c.y - h});
                                          lamp.setFillColor(lampColor);
                                          tg.draw(lamp);
                                      }});
                }
            }
        }
    }
    if (harbor) {
        // Caixas de pesca amarradas ao cais e boias nas pontas da arquibancada.
        // Tudo fica fora do quadrado jogável e atrás dos lutadores.
        for (float sx : {-1.0f, 1.0f}) for (float sz : {-1.0f, 1.0f}) {
            const float x = sx * 348.0f, z = sz * 348.0f;
            queueWorldPolygon({{x - 24, -35, z - 20}, {x + 24, -35, z - 20},
                               {x + 24, 18, z - 20}, {x - 24, 18, z - 20}},
                              hex(0x725038), hex(0x211f20), 0.38f);
            queueWorldPolygon({{x - 24, 18, z - 20}, {x + 24, 18, z - 20},
                               {x + 24, 18, z + 20}, {x - 24, 18, z + 20}},
                              hex(0xa47a4b), hex(0x362923), 0.39f);
            queueWorldLine({x - 24, -35, z - 20}, {x + 24, 18, z - 20}, hex(0xb28a54), 3.0f, 0.40f);
            queueWorldLine({x + 24, -35, z - 20}, {x - 24, 18, z - 20}, hex(0xb28a54), 3.0f, 0.40f);

            const Vec3 buoyWorld{sx * 685.0f, 190.0f, sz * 120.0f};
            if (auto p = camera_.project(buoyWorld)) {
                const sf::Vector2f center = p->pos;
                const float radius = std::clamp(19.0f * p->scale, 4.0f, 34.0f);
                queue_.push_back({0.42f, p->depth, [center, radius](sf::RenderTarget& t) {
                    sf::CircleShape buoy(radius);
                    buoy.setPosition({center.x - radius, center.y - radius});
                    buoy.setFillColor(sf::Color::Transparent);
                    buoy.setOutlineThickness(std::max(2.0f, radius * 0.38f));
                    buoy.setOutlineColor(hex(0xe98245));
                    t.draw(buoy);
                }});
            }
        }
    }
}

void RingRenderer::stepCrowdEffects(const GameEngine& engine) {
    // Os efeitos andam no relógio do motor (60 Hz); luta nova ou salto grande zera tudo.
    const uint32_t now = engine.frameIndex;
    if (arenaTheme_ == ArenaTheme::JoanaHarbor) {
        // O cais não usa a antiga plateia do estádio nem objetos lançados por ela.
        flashes_.clear();
        throwables_.clear();
        effectsFrame_ = now;
        return;
    }
    if (now < effectsFrame_ || now - effectsFrame_ > 30) {
        flashes_.clear();
        throwables_.clear();
        effectsFrame_ = now;
    }
    const bool matchOver = engine.isMatchOver();
    const bool jumping = engine.gameState == STATE_KNOCKDOWN || matchOver;
    const float flashChance = matchOver ? 0.35f : (engine.gameState == STATE_KNOCKDOWN ? 0.08f : 0.015f);
    std::uniform_real_distribution<float> u(0.0f, 1.0f);
    for (; effectsFrame_ < now; ++effectsFrame_) {
        const float time = effectsFrame_ / 12.0f; // Date.now() / 200
        for (const Spectator& s : crowd_) {
            if (u(rng_) >= flashChance) continue;
            float jump = jumping ? std::fabs(std::sin(time * 3.0f + s.phase)) * 22.0f : std::sin(time + s.phase) * 2.0f;
            flashes_.push_back({{s.pos.x + (u(rng_) - 0.5f) * 20.0f, s.pos.y + jump + 25.0f + u(rng_) * 20.0f, s.pos.z + (u(rng_) - 0.5f) * 20.0f},
                                1.0f, 0.2f + u(rng_) * 0.1f, 15.0f + u(rng_) * 25.0f});
        }
        for (Flash& f : flashes_) f.life -= f.decay;
        flashes_.erase(std::remove_if(flashes_.begin(), flashes_.end(), [](const Flash& f) { return f.life <= 0.0f; }), flashes_.end());

        // Chuva de rosas sobre quem venceu (spawnThrowable): de vez em quando, até 10 no ar.
        if (matchOver && throwables_.size() < 14 && u(rng_) < (celebrating_ ? 0.06f : 0.03f)) {
            float start = u(rng_) * 2.0f * kPi, dist = 180.0f + u(rng_) * 80.0f;
            Vec3 from{std::sin(start) * dist, 140.0f + u(rng_) * 60.0f, std::cos(start) * dist};
            Vec3 to{(u(rng_) - 0.5f) * 160.0f, 0.0f, (u(rng_) - 0.5f) * 160.0f};
            float frames = 40.0f + std::floor(u(rng_) * 25.0f);
            // Roupas íntimas são uma piada rara (~2.5%, 1 em 40 itens); a quase totalidade são rosas.
            int kind = u(rng_) < 0.025f ? celebration::kRoseKinds + static_cast<int>(u(rng_) * celebration::kUnderwearKinds) % celebration::kUnderwearKinds
                                        : static_cast<int>(u(rng_) * 3.0f) % 3;
            throwables_.push_back({from, {(to.x - from.x) / frames, 2.0f + u(rng_) * 4.0f, (to.z - from.z) / frames},
                                   u(rng_) * 2.0f * kPi, (u(rng_) - 0.5f) * 0.15f, 180, false, kind});
        }
        for (Throwable& t : throwables_) {
            if (!t.landed) {
                t.pos.x += t.vel.x; t.pos.y += t.vel.y; t.pos.z += t.vel.z;
                t.vel.y -= 0.25f;
                t.rot += t.spin;
                if (t.pos.y <= 2.0f) { t.pos.y = 2.0f; t.landed = true; }
            } else {
                t.life--;
            }
        }
        throwables_.erase(std::remove_if(throwables_.begin(), throwables_.end(), [](const Throwable& t) { return t.life <= 0; }), throwables_.end());
    }
}

void RingRenderer::queueCrowd(const GameEngine& engine) {
    if (hideCrowd) return;
    static const std::array<const char*, 19> kSkins = {
        "weirdoFan", "punkCyclops", "crazyGoose", "hotdogGuy", "jonas", "angryGranny", "zombieFan", "vuvuzelaGuy", "radioUncle",
        "tvBoobs", "caramelDog", "coolerGuy", "flareGuy", "drumMaster", "jugglerFan", "cactomenFan", "ghostFan", "dinoMascot", "influencerFan"};
    const bool jumping = engine.gameState == STATE_KNOCKDOWN || engine.isMatchOver();
    const float time = engine.frameIndex / 12.0f;
    const uint32_t anim = (engine.frameIndex / 9) % 4; // quadros de 150 ms
    for (const Spectator& s : crowd_) {
        const bool harbor = arenaTheme_ == ArenaTheme::JoanaHarbor;
        // No cais a torcida se espalha pelas plataformas, deixando ver o porto ao fundo.
        if (harbor && s.pos.y >= 68.0f && s.skin % 3 != 0) continue;
        if (arenaTheme_ == ArenaTheme::MarcovObservatory && s.skin % 3 == 0) continue;
        if (arenaTheme_ == ArenaTheme::VivaldiConcertHall && s.skin % 4 == 0) continue;
        if (arenaTheme_ == ArenaTheme::CremosoParlor && s.pos.y >= 68.0f) continue;
        if (arenaTheme_ == ArenaTheme::EspinhaCourtyard && s.pos.y >= 68.0f) continue;
        if (arenaTheme_ == ArenaTheme::JackGarden) {
            if (s.pos.y >= 68.0f) continue;
            // Desobstrui apenas o corredor norte por onde a Grande Alameda passa em direção aos domos
            if (s.pos.z > 160.0f && std::fabs(s.pos.x) < 130.0f) continue;
        }
        if (arenaTheme_ == ArenaTheme::FrogSwamp &&
            ((s.pos.y != 18.0f && s.pos.y != 68.0f) || s.skin % 2 == 0)) continue;
        Vec3 local = camera_.toLocal(s.pos);
        // Torcedores colados na lente (câmera recuada dos lutadores grandes) cobririam a tela toda.
        if (local.z < 150.0f || local.z > 2200.0f) continue;
        float jump = jumping ? std::fabs(std::sin(time * 3.0f + s.phase)) * 22.0f : std::sin(time + s.phase) * 2.0f;
        // De frente, de lado ou de costas conforme o ângulo em relação à câmera.
        float a = std::fmod(s.angle - camera_.yaw + 4.0f * kPi, 2.0f * kPi);
        const char* view = "Front";
        bool back = false;
        if (a > kPi * 0.35f && a < kPi * 0.65f) view = "Side";
        else if (a >= kPi * 0.65f && a <= kPi * 1.35f) { view = "Back"; back = true; }
        else if (a > kPi * 1.35f && a < kPi * 1.65f) view = "Side";
        std::string name = std::string(kSkins[static_cast<std::size_t>(s.skin)]) + view + "Frames_" + std::to_string(anim);
        auto spr = sprite(name);
        if (!spr) continue;
        const float layer = (local.z < 250.0f && back) ? 5.5f : 0.6f;
        // Plateia apagada (escura e azulada) para os lutadores saltarem; mantém escura e atmosférica na vitória!
        const sf::Color crowdTint = harbor
            ? ((jumping && !celebrating_ && !engine.isMatchOver()) ? sf::Color(188, 213, 205) : sf::Color(95, 139, 142))
            : arenaTheme_ == ArenaTheme::ValeriaPernambuco
              ? ((jumping && !celebrating_ && !engine.isMatchOver()) ? sf::Color(255, 219, 170) : sf::Color(183, 136, 126))
              : arenaTheme_ == ArenaTheme::MarcovObservatory
                ? ((jumping && !celebrating_ && !engine.isMatchOver()) ? sf::Color(190, 225, 239) : sf::Color(99, 139, 159))
              : arenaTheme_ == ArenaTheme::FrogSwamp
                ? ((jumping && !celebrating_ && !engine.isMatchOver()) ? sf::Color(221, 232, 179) : sf::Color(116, 151, 106))
              : arenaTheme_ == ArenaTheme::VivaldiConcertHall
                ? ((jumping && !celebrating_ && !engine.isMatchOver()) ? sf::Color(240, 210, 207) : sf::Color(156, 116, 142))
              : arenaTheme_ == ArenaTheme::CremosoParlor
                ? ((jumping && !celebrating_ && !engine.isMatchOver()) ? sf::Color(255, 230, 213) : sf::Color(166, 136, 137))
              : arenaTheme_ == ArenaTheme::EspinhaCourtyard
                ? ((jumping && !celebrating_ && !engine.isMatchOver()) ? sf::Color(255, 228, 188) : sf::Color(178, 128, 117))
              : arenaTheme_ == ArenaTheme::JackGarden
                ? ((jumping && !celebrating_ && !engine.isMatchOver()) ? sf::Color(239, 233, 221) : sf::Color(188, 183, 177))
              : ((jumping && !celebrating_ && !engine.isMatchOver()) ? sf::Color(200, 190, 225) : sf::Color(115, 105, 145));
        const float groundY = arenaTheme_ == ArenaTheme::FrogSwamp ? -50.0f : s.pos.y;
        // A plateia continua colorida nas laterais, mas recua visualmente no
        // corredor de leitura atrás do adversário. O cálculo usa a projeção
        // real do torcedor, portanto acompanha os giros sem um painel fixo.
        sf::Color stagedTint = crowdTint;
        const bool busyStands = arenaTheme_ == ArenaTheme::ValeriaPernambuco ||
                                arenaTheme_ == ArenaTheme::VivaldiConcertHall ||
                                arenaTheme_ == ArenaTheme::CremosoParlor ||
                                arenaTheme_ == ArenaTheme::EspinhaCourtyard ||
                                arenaTheme_ == ArenaTheme::JackGarden;
        if (busyStands && !engine.isMatchOver()) {
            const auto screen = camera_.projectLocal(local);
            if (screen) {
                const float distance = std::fabs(screen->pos.x - kLogicalWidth * 0.5f);
                const float focus = std::clamp((265.0f - distance) / 185.0f, 0.0f, 1.0f);
                const float light = 0.94f - focus * 0.19f;
                stagedTint.r = static_cast<std::uint8_t>(stagedTint.r * light);
                stagedTint.g = static_cast<std::uint8_t>(stagedTint.g * light);
                stagedTint.b = static_cast<std::uint8_t>(stagedTint.b * light);
            }
        }
        queueSprite(*spr, {s.pos.x, groundY + jump, s.pos.z}, 140.0f, 170.0f, 0.0f, layer, stagedTint);
    }

    for (const Flash& f : flashes_) {
        auto p = camera_.project(f.pos);
        if (!p) continue;
        float sz = std::max(3.0f, f.size * p->scale * 0.4f);
        sf::Vector2f c = p->pos;
        float alpha = f.life;
        queue_.push_back({4.0f, p->depth, [c, sz, alpha](sf::RenderTarget& t) {
                              // Estrela de 4 pontas em pixel art.
                              sf::RectangleShape h({sz * 3.6f, sz * 0.8f}), v({sz * 0.8f, sz * 3.6f}), core({sz * 1.2f, sz * 1.2f});
                              h.setPosition({c.x - sz * 1.8f, c.y - sz * 0.4f});
                              v.setPosition({c.x - sz * 0.4f, c.y - sz * 1.8f});
                              core.setPosition({c.x - sz * 0.6f, c.y - sz * 0.6f});
                              h.setFillColor(withAlpha(sf::Color::White, alpha));
                              v.setFillColor(withAlpha(sf::Color::White, alpha));
                              core.setFillColor(withAlpha(sf::Color(255, 255, 187), alpha));
                              t.draw(h); t.draw(v); t.draw(core);
                          }});
    }

    for (const Throwable& th : throwables_) {
        auto p = camera_.project(th.pos);
        if (!p) continue;
        float alpha = th.landed && th.life < 40 ? th.life / 40.0f : 1.0f;
        float size = 45.0f * p->scale;
        sf::Vector2f c = p->pos;
        float rot = th.rot;
        int kind = th.kind;
        queue_.push_back({1.5f, p->depth, [c, size, rot, alpha, kind](sf::RenderTarget& t) {
                              cv::Canvas ctx(t);
                              ctx.globalAlpha = alpha;
                              ctx.translate(c.x, c.y);
                              ctx.rotate(rot);
                              celebration::drawThrowable(ctx, kind, size);
                          }});
    }
}

void RingRenderer::queueSprite(const SpriteFrame& frame, const Vec3& world, float w, float h, float skew, float layer,
                               sf::Color tint, float rotation, float bottomCrop) {
    auto p = camera_.project(world);
    if (!p) return;
    float sw = w * p->scale, sh = h * p->scale;
    // Sprite recortado: ocupa só a sua parte do quadro original (sem esticar).
    float u0 = 0, v0 = 0, u1 = 1, v1 = 1;
    if (frame.fullSize.x > 0 && frame.fullSize.y > 0 && (frame.rect.size.x != frame.fullSize.x || frame.rect.size.y != frame.fullSize.y)) {
        u0 = frame.trimOffset.x / static_cast<float>(frame.fullSize.x);
        v0 = frame.trimOffset.y / static_cast<float>(frame.fullSize.y);
        u1 = u0 + frame.rect.size.x / static_cast<float>(frame.fullSize.x);
        v1 = v0 + frame.rect.size.y / static_cast<float>(frame.fullSize.y);
    }
    bottomCrop = std::clamp(bottomCrop, 0.0f, 0.9f);
    v1 = v0 + (v1 - v0) * (1.0f - bottomCrop);
    sf::Vector2f base = p->pos;
    // Ancorado pela base, com cisalhamento horizontal como ctx.transform(1, 0, skew, 1, 0, 0).
    auto at = [&](float lx, float ly) { return sf::Vector2f{base.x + lx + skew * ly, base.y + ly}; };
    sf::Vector2f tl = at(-sw / 2 + sw * u0, -sh + sh * v0), tr = at(-sw / 2 + sw * u1, -sh + sh * v0),
                 br = at(-sw / 2 + sw * u1, -sh + sh * v1), bl = at(-sw / 2 + sw * u0, -sh + sh * v1);
    if (rotation != 0.0f) {
        const sf::Vector2f c = (tl + br) * 0.5f;
        const float cs = std::cos(rotation), sn = std::sin(rotation);
        for (sf::Vector2f* v : {&tl, &tr, &br, &bl}) {
            sf::Vector2f d = *v - c;
            *v = {c.x + d.x * cs - d.y * sn, c.y + d.x * sn + d.y * cs};
        }
    }

    sf::FloatRect r(sf::Vector2f(frame.rect.position), sf::Vector2f(frame.rect.size));
    sf::Vector2f t0 = r.position, t1 = {r.position.x + r.size.x, r.position.y},
                 t2 = {r.position.x + r.size.x, r.position.y + r.size.y * (1.0f - bottomCrop)},
                 t3 = {r.position.x, r.position.y + r.size.y * (1.0f - bottomCrop)};
    std::array<sf::Vertex, 6> v = {{{tl, tint, t0}, {tr, tint, t1}, {br, tint, t2}, {tl, tint, t0}, {br, tint, t2}, {bl, tint, t3}}};
    const sf::Texture* texture = frame.texture;
    queue_.push_back({layer, p->depth, TriangleDraw{{v.begin(),v.end()},texture}});
}

void RingRenderer::addShadowGlove(const SpriteFrame& frame, const Vec3& world, float w, float h,
                                  float skew, float rotation, int slot) {
    const auto shadow = pendingShadow_[slot];
    const auto p = camera_.project(world);
    if (!shadow || !p || !frame.texture) return;
    const float sw = w * p->scale, sh = h * p->scale;
    float u0 = 0, v0 = 0, u1 = 1, v1 = 1;
    if (frame.fullSize.x > 0 && frame.fullSize.y > 0 &&
        (frame.rect.size.x != frame.fullSize.x || frame.rect.size.y != frame.fullSize.y)) {
        u0 = frame.trimOffset.x / static_cast<float>(frame.fullSize.x);
        v0 = frame.trimOffset.y / static_cast<float>(frame.fullSize.y);
        u1 = u0 + frame.rect.size.x / static_cast<float>(frame.fullSize.x);
        v1 = v0 + frame.rect.size.y / static_cast<float>(frame.fullSize.y);
    }
    auto at = [&](float u, float v) {
        const float ly = -sh + sh * v;
        return sf::Vector2f{p->pos.x - sw * 0.5f + sw * u + skew * ly, p->pos.y + ly};
    };
    sf::Vector2f pts[4] = {at(u0,v0), at(u1,v0), at(u1,v1), at(u0,v1)};
    if (rotation != 0.0f) {
        const sf::Vector2f center = (pts[0] + pts[2]) * 0.5f;
        const float cs = std::cos(rotation), sn = std::sin(rotation);
        for (auto& pt : pts) {
            const sf::Vector2f d = pt - center;
            pt = {center.x + d.x * cs - d.y * sn, center.y + d.x * sn + d.y * cs};
        }
    }
    for (auto& pt : pts) {
        const float high = std::clamp(shadow->baseY - pt.y, 0.0f, shadow->height);
        pt = {shadow->baseX + (pt.x - shadow->baseX) * 0.78f + high * shadow->lateral,
              shadow->footY + high * 0.23f};
    }
    const sf::Vector2f tex = sf::Vector2f(frame.rect.position);
    const sf::Vector2f size = sf::Vector2f(frame.rect.size);
    const sf::Vector2f uv[4] = {tex, {tex.x+size.x,tex.y}, tex+size, {tex.x,tex.y+size.y}};
    constexpr sf::Color white = sf::Color::White;
    shadow->pieces.push_back({frame.texture, {{pts[0],white,uv[0]}, {pts[1],white,uv[1]}, {pts[2],white,uv[2]},
                                               {pts[0],white,uv[0]}, {pts[2],white,uv[2]}, {pts[3],white,uv[3]}}});
}

void RingRenderer::queueFighter(const Fighter& fIn, const Placement& placeIn, bool backView, float roll, bool koFlight) {
    if (fIn.charId == 9 || fIn.isDummy()) {
        queueScarecrow(fIn, placeIn, backView);
        return;
    }

    const bool isPlayer = fIn.isPlayer;
    pendingShadow_[isPlayer ? 0 : 1].reset();

    const bool isKnockdownWinner = engine_ && engine_->gameState == STATE_KNOCKDOWN &&
                                   ((fIn.isPlayer && engine_->enemyIsDown) || (!fIn.isPlayer && !engine_->enemyIsDown));
    const bool isMatchWinner = engine_ && engine_->isMatchOver() &&
                               (((engine_->result == MatchResult::WinKO || engine_->result == MatchResult::WinTKO || engine_->result == MatchResult::WinDecision) && fIn.isPlayer) ||
                                ((engine_->result == MatchResult::LossKO || engine_->result == MatchResult::LossTKO || engine_->result == MatchResult::LossDecision) && !fIn.isPlayer));
    const bool isKnockdownLoser = engine_ && engine_->gameState == STATE_KNOCKDOWN &&
                                  ((fIn.isPlayer && !engine_->enemyIsDown) || (!fIn.isPlayer && engine_->enemyIsDown));
    const bool isMatchLoser = engine_ && engine_->isMatchOver() && !isMatchWinner;

    const bool isHeroic = !koFlight && !isKnockdownLoser && !isMatchLoser &&
                          ((fIn.signatureOnly && fIn.state == FIGHTER_SKILL) || celebrating_ || fIn.state == FIGHTER_VICTORY_POSE || isKnockdownWinner || isMatchWinner);

    Placement place = placeIn;
    Fighter f = joanaVisualPose(fIn);
    if(f.charId==5 && f.skinId==1 && !celebrating_ && (f.state==FIGHTER_IDLE || f.state==FIGHTER_BLOCK))
        f.offsetX=f.offsetY=f.bodySkew=0;
    if (isHeroic && !koFlight) {
        backView = false;
        place.facing = std::atan2(camera_.pos.x - place.pos.x, camera_.pos.z - place.pos.z);
        f.facing = place.facing;
        if (f.charId >= 1 && f.charId <= 7 && !celebrating_) {
            // The approved gesture owns the two chest impacts. Do not inherit
            // the old skill's premature shake or lift from the simulation pose.
            // On the victory screen the authored gesture rides on top of the
            // ring celebration's world-space hops and sway, like original Jack.
            f.offsetX = f.offsetY = f.offsetZ = f.bodySkew = 0.0f;
        }
    }

    // Nomes dos sprites e medidas como em drawEntityCharacter() do index.html.
    std::string bodyName, gloveL, gloveR;
    float bodyW = 110.0f, bodyH = 110.0f, bodyLift = 34.0f, gloveSize = 58.0f, skewL = 0.0f, skewR = 0.0f;
    float sizeMulL = 1.0f, sizeMulR = 1.0f, liftL = 0.0f, liftR = 0.0f, rotL = 0.0f, rotR = 0.0f;
    Reaction rx = computeReaction(f, f.isPlayer ? 0 : 1, roll, koFlight, backView);
    const bool down = rx.xxFace && !koFlight; // no voo o esqueleto faz o rosto de nocaute

    const bool casting = f.state == FIGHTER_SKILL;
    if (f.charId == 0) { // Jack — variações de getCharacterFrontImage/BackImage do JS
        const bool molasses = f.status.molasses > 0 || (casting && f.activeSkill == SkillId::JackMolasses);
        const bool jelly = casting && f.activeSkill == SkillId::JackRoyalJelly;
        const bool honeySkin = f.loadout.has(SkillId::JackHoneySkin);
        std::string variant;
        if (molasses && honeySkin) variant = "MolassesHoney";
        else if (molasses) variant = "Molasses";
        else if (jelly) variant = "RoyalJelly";
        else if (honeySkin) variant = "HoneySkin";
        bodyName = backView ? "jackBackSprite" + variant : (down ? "jackFrontSpriteGameOver" : "jackFrontSprite" + variant);
        gloveL = gloveR = molasses ? "beehiveGloveSpriteMolasses" : "beehiveGloveSprite"; // Geleia Real usa as colmeias padrão
    } else if (f.charId == 1) { // King Frog
        const bool poison = f.status.poisonSkin > 0 || (casting && f.activeSkill == SkillId::FrogPoisonSkin);
        // Boca e coroa seguem o getMouthStage do Lab (língua, cuspe, salto sem coroa, papo inflado...).
        std::string stage = "0";
        if (const auto& fx = lab_[f.isPlayer ? 0 : 1]) stage = fx->mouthStage(labContext(f, f.isPlayer));
        const std::string pz = poison ? "Poison" : "";
        if (backView) {
            const bool noCrown = stage == "no_crown" || stage == "squash_no_crown";
            bodyName = noCrown ? "frogBackSprite" + pz + "NoCrown" : (poison ? "frogBackSpritePoisonSkin" : "frogBackSprite");
        } else if (down) {
            bodyName = "frogFrontSpriteGameOver";
        } else if (stage == "inflated") bodyName = "frogFrontSprite" + pz + "Inflated";
        else if (stage == "mud_spit") bodyName = "frogFrontSprite" + pz + "MudSpit";
        else if (stage == "no_crown") bodyName = "frogFrontSprite" + pz + "NoCrown";
        else if (stage == "squash_no_crown") bodyName = "frogFrontSprite" + pz + "SquashNoCrown";
        else if (stage == "charge" || f.state == FIGHTER_TELEGRAPH) bodyName = "frogFrontSprite" + pz + "Charge";
        else if (stage != "0") bodyName = "frogFrontSprite" + pz + "Mouth" + stage;
        else bodyName = poison ? "frogFrontSpritePoisonSkin" : "frogFrontSpriteFrames_" + std::to_string((frameCounter_ / 9) % 4);
        bodyW = isPlayer ? 115.0f : 140.0f;
        bodyH = isPlayer ? 140.0f : 170.0f;
        bodyLift = -10.0f;
        gloveSize = isPlayer ? 54.0f : 85.0f;
        skewL = -0.15f;
        skewR = 0.15f;
        // A luva da mão que vai bater fica laranja durante o aviso; de frente as luvas são espelhadas.
        bool flashL = f.telegraphFlash && !f.isLeftPunch, flashR = f.telegraphFlash && f.isLeftPunch;
        std::string left = "frogGloveLeftSprite", right = "frogGloveRightSprite";
        gloveL = backView ? left : right;
        gloveR = backView ? right : left;
        if (flashL) gloveL += "Orange";
        if (flashR) gloveR += "Orange";
    }

    std::string armorName;
    sf::Color gloveTint = sf::Color::White;
    {
        const uint32_t frame = frameCounter_;
        const float p = f.maxTimer > 0 ? 1.0f - static_cast<float>(f.timer) / static_cast<float>(f.maxTimer) : 0.0f;
        // Quadro animado "nome_i" com recuo para "nome_0" quando a animação tem menos quadros.
        auto frameName = [&](const std::string& base, uint32_t idx) {
            std::string n = base + "_" + std::to_string(idx);
            return sprite(n) ? n : base + "_0";
        };
        const bool blocking = f.state == FIGHTER_BLOCK;
        const bool flash = f.telegraphFlash;
        const char* side = backView ? "Back" : "Front";
        if (f.charId >= 3 && f.charId <= 7) skewL = -0.1f, skewR = 0.1f;

        if (f.charId == 3) { // O Cremoso: skin pela passiva, ação pelo estado (frames de 140 ms)
            const std::string skin = f.loadout.has(SkillId::CremosoFrozenStamina) ? "Frozen"
                                   : f.loadout.has(SkillId::CremosoSweetRevenge)  ? "Revenge" : "";
            std::string action;
            if (const auto& fx = lab_[f.isPlayer ? 0 : 1]) {
                action = fx->mouthStage(labContext(f, f.isPlayer)); // inflando, cuspindo a cereja, base derretida
            } else if (f.status.melt > 0 || (casting && f.activeSkill == SkillId::CremosoMeltMode)) action = "MeltBase";
            else if (casting && f.activeSkill == SkillId::CremosoCherryTopping) action = "CherrySpit";
            if (!action.empty() && !sprite(frameName(std::string("cremoso") + side + action + skin + "Frames", 0))) action.clear();
            if (casting && f.activeSkill == SkillId::CremosoVanillaWhip && sprite("cremosoVanillaTornadoBodySprite")) {
                bodyName = "cremosoVanillaTornadoBodySprite";
            } else if (down && !backView) {
                bodyName = frameName("cremosoFrontGameOverFrames", 0);
            } else {
                bodyName = frameName(std::string("cremoso") + side + action + skin + "Frames", (frame / 8) % 4);
            }
            bodyW = isPlayer ? 140.0f : 164.0f; bodyH = isPlayer ? 144.0f : 172.0f; bodyLift = 18.0f;
            gloveSize = isPlayer ? 76.0f : 94.0f;
            std::string glove = skin == "Frozen" ? "cremosoGloveFrozenSprite" : (skin == "Revenge" ? "cremosoGloveRevengeSprite" : "cremosoGloveSprite");
            if (casting && f.activeSkill == SkillId::CremosoSyrupBlast) glove = "cremosoGloveSyrupBoilingSprite";
            gloveL = gloveR = glove;
            if (flash) gloveL = gloveR = "cremosoGloveSpriteOrange";
        } else if (f.charId == 4) { // Valéria: frevo animado, luvas giratórias, Magma Frio
            const std::string cold = f.coldMagma ? "ColdMagma" : "";
            const uint32_t body = (frame / 9) % 4;
            bodyName = backView ? frameName("valeriaBackFrames" + cold, body)
                                : (down ? frameName("valeriaFrontGameOverFrames" + cold, body) : frameName("valeriaFrontFrames" + cold, body));
            bodyW = isPlayer ? 115.0f : 135.0f; bodyH = isPlayer ? 120.0f : 145.0f; bodyLift = 22.0f;
            gloveSize = isPlayer ? 66.0f : 82.0f;
            if (blocking) {
                gloveL = gloveR = backView ? "valeriaGloveBlockBack" : "valeriaGloveBlockFront";
            } else {
                const bool busy = isPunchState(f.state) || casting;
                const uint32_t spin = busy ? (frame / 4) % 8 : (frame / 7) % 8;
                const bool magma = (casting && f.activeSkill == SkillId::ValeriaMagmaPunch) || f.status.heatwave > 0;
                const std::string kind = magma ? "Magma" : "";
                const std::string orange = (flash && !magma) ? "Orange" : "";
                if (backView) {
                    gloveL = frameName("valeriaGlove" + kind + "BackFrames" + orange, spin);
                    gloveR = frameName("valeriaGlove" + kind + "BackRightFrames" + orange, spin);
                } else {
                    gloveL = gloveR = frameName("valeriaGlove" + kind + "FrontFrames" + orange, spin);
                }
            }
        } else if (f.charId == 5) { // Maestra Vivaldi: fantasma em 8 quadros
            const uint32_t body = (frame / 7) % 8;
            bodyName = backView ? frameName("vivaldiBackFrames", body)
                                : (down ? frameName("vivaldiFrontGameOverFrames", body) : frameName("vivaldiFrontFrames", body));
            bodyW = isPlayer ? 122.0f : 142.0f; bodyH = isPlayer ? 132.0f : 158.0f; bodyLift = 20.0f;
            gloveSize = isPlayer ? 80.0f : 102.0f;
            const std::string kind = blocking ? std::string("Block") + side : std::string(side);
            gloveL = "vivaldiGloveLeft" + kind;
            gloveR = "vivaldiGloveRight" + kind;
        } else if (f.charId == 6) { // Lady Espinha
            const uint32_t body = (frame / 8) % 4;
            bodyName = backView ? frameName("espinhaBackFrames", body)
                                : (down ? frameName("espinhaFrontGameOverFrames", body) : frameName("espinhaFrontFrames", body));
            bodyW = isPlayer ? 120.0f : 140.0f; bodyH = isPlayer ? 130.0f : 155.0f; bodyLift = 20.0f;
            gloveSize = isPlayer ? 58.0f : 72.0f;
            if (blocking) gloveL = gloveR = backView ? "espinhaGloveBlockBack" : "espinhaGloveBlockFront";
            else if (backView) { gloveL = "espinhaGloveLeftBack"; gloveR = "espinhaGloveRightBack"; }
            else { gloveL = "espinhaGloveRightFront"; gloveR = "espinhaGloveLeftFront"; } // de frente as mãos espelham
        } else if (f.charId == 7) { // Dr. Marcov
            const uint32_t body = (frame / 8) % 4;
            bodyName = backView ? frameName("marcovBackFrames", body)
                                : (down ? frameName("marcovFrontGameOverFrames", body) : frameName("marcovFrontFrames", body));
            bodyW = isPlayer ? 120.0f : 142.0f; bodyH = isPlayer ? 130.0f : 158.0f; bodyLift = 18.0f;
            gloveSize = isPlayer ? 74.0f : 94.0f;
            if (blocking) gloveL = gloveR = backView ? "marcovGloveBlockBack" : "marcovGloveBlockFront";
            else { gloveL = std::string("marcovGloveLeft") + side; gloveR = std::string("marcovGloveRight") + side; }
        }
        if (f.charId == 8) { // Batalhator 2000: robô, robô danificado e O Nerd Operador
            const int stage = std::max(1, f.bossStage);
            const bool nerd = stage == 3;
            bodyName = frameName("batalhator" + std::to_string(stage) + side + "Frames", (frame / 3) % 8);
            bodyW = nerd ? 135.0f : 210.0f; bodyH = nerd ? 165.0f : 235.0f; bodyLift = nerd ? 10.0f : 25.0f;
            gloveSize = 100.0f;
            const float spin = static_cast<float>(frame) * 0.42f; // serras a ~25 rad/s, como no JS
            if (stage == 1) {
                const bool launched = casting && f.activeSkill == SkillId::BossDualSaw && !f.signatureOnly;
                gloveL = launched ? "batalhatorWristSocket" : "batalhatorSawLeft";
                gloveR = launched ? "batalhatorWristSocket" : "batalhatorSawRight";
                if (!launched) { rotL = spin; rotR = -spin; }
                liftL = liftR = 65.0f;
            } else if (stage == 2) {
                gloveL = "batalhatorHeadLeft";
                gloveR = "batalhatorSawRight";
                sizeMulL = 0.82f; skewL = -0.1f; rotR = -spin; liftR = 65.0f;
            } else {
                gloveL = "nerdGloveLeft";
                gloveR = "nerdGloveRight";
                sizeMulL = sizeMulR = 0.75f; skewL = -0.1f; skewR = 0.1f;
            }
            if (flash) gloveTint = sf::Color(255, 170, 170);
        }
        // Sem luva laranja própria: o aviso pisca em tom quente.
        if (flash && f.charId >= 5 && f.charId <= 7) gloveTint = sf::Color(255, 190, 110);
    }
    if (f.charId == 0 && f.telegraphFlash) {
        gloveTint = sf::Color(255, 170, 170);
    }
    if (f.charId == 2) { // Joana: gancho de marujo na esquerda, garra de siri na direita
        const float sizeMul = f.monstrous ? 1.15f : 1.0f, heightMul = f.monstrous ? 1.32f : 1.0f;
        bodyName = backView ? "joanaBackSprite" : (down ? "joanaFrontSpriteGameOver" : "joanaFrontSprite");
        bodyW = (isPlayer ? 115.0f : 135.0f) * sizeMul;
        bodyH = (isPlayer ? 120.0f : 145.0f) * heightMul;
        bodyLift = f.monstrous ? 26.0f : 22.0f;
        gloveSize = isPlayer ? 66.0f : 82.0f;
        skewL = -0.1f;
        skewR = 0.1f;

        // Estágio da garra (getClawStage do Combat Lab).
        const float p = f.maxTimer > 0 ? 1.0f - static_cast<float>(f.timer) / static_cast<float>(f.maxTimer) : 0.0f;
        std::string claw = "Closed";
        if (casting && f.activeSkill == SkillId::JoanaClawCrush) {
            claw = p < 0.16f ? "Open1" : p < 0.32f ? "Open2" : p < 0.52f ? "Open3" : p < 0.66f ? "Clamp" : "Closed";
        } else if (casting && f.activeSkill == SkillId::JoanaOceanBounty && p >= 0.18f && p < 0.78f) {
            float cycle = std::sin(((p - 0.18f) / 0.60f) * kPi * 6.0f);
            claw = cycle > 0.45f ? "Open2" : (cycle > -0.15f ? "Open1" : "Closed");
        }
        const bool hookThrown = casting && f.activeSkill == SkillId::JoanaHookPull && p >= 0.22f && p <= 0.85f;
        const bool flashL = f.telegraphFlash && !f.isLeftPunch, flashR = f.telegraphFlash && f.isLeftPunch;
        gloveL = hookThrown ? "joanaHookWristSocketSprite" : (backView ? "joanaHookBackSprite" : "joanaHookSprite");
        gloveR = backView ? "joanaClawBack" + claw + "Sprite" : "joanaClaw" + claw + "Sprite";
        if ((flashL || (f.monstrous && !liveVisual)) && !hookThrown) gloveL = "joanaHookSpriteOrange";
        if ((flashR || (f.monstrous && !liveVisual)) && claw == "Closed") gloveR = "joanaClawSpriteOrange";
        if (f.status.shell > 0) armorName = backView ? "joanaBackBarnacleArmorSprite" : "joanaFrontBarnacleArmorSprite";
    }

    // Jack montado por partes (sem variações de pele, que ainda usam o sprite inteiro).
    const bool jackRig = f.charId == 0 && (bodyName.rfind("jackFrontSprite", 0) == 0 || bodyName.rfind("jackBackSprite", 0) == 0) &&
                         sprite("jackRigF_torso").has_value();
    static const char* kRigTorso[8] = {"", "frogRigF0_torso", "joanaRigF0_torso", "cremosoRigF0_torso", "valeriaRigF0_torso",
                                       "vivaldiRigF0_torso", "espinhaRigF0_torso", "marcovRigF0_torso"};
    const bool castRig = f.charId >= 1 && f.charId <= 7 && rigBase(f.charId, bodyName, backView) && sprite(kRigTorso[f.charId]).has_value();
    const bool isSpinRx = koFlight || rx.sprite.rfind("RxSpin_", 0) == 0;
    // Quadros novos de reação (cabeça virada, olhando para cima, giro do nocaute) quando existirem.
    if ((isSpinRx || (!jackRig && !castRig)) && !rx.sprite.empty() && f.charId < 8 && !casting) {
        static const char* kRxPrefix[8] = {"jack", "frog", "joana", "cremoso", "valeria", "vivaldi", "espinha", "marcov"};
        const std::string rxName = std::string(kRxPrefix[f.charId]) + rx.sprite;
        if (sprite(rxName)) {
            bodyName = rxName;
            rx.headRot = rx.headShiftX = rx.headShiftY = 0.0f;
            rx.headScaleX = 1.0f;
            if (rx.face != 3) rx.face = 0;
            rx.drops = false;
        }
    }
    if (f.charId == 0) {
        const std::string form = liveForm(f, bodyName), skin = "jack_skin" + std::to_string(f.skinId < 4 ? f.skinId : 0);
        const std::string view = backView ? "back" : "front";
        if (sprite(skin + "_" + form + "_" + view + "_0")) {
            gloveL = skin + "_" + form + "_" + view + "_0";
            gloveR = skin + "_" + form + "_" + view + "_1";
            if (f.skinId == 1 && casting && f.activeSkill == SkillId::JackInfestation) {
                auto ease=[](float x){x=std::clamp(x,0.0f,1.0f);return x*x*(3-2*x);};
                const float progress=f.maxTimer>0?1.0f-static_cast<float>(f.timer)/f.maxTimer:0.0f;
                const int drawer=static_cast<int>(std::round(8*ease((progress-.12f)/.18f)*(1-ease((progress-.85f)/.15f))));
                const std::string suffix=drawer>0?"_drawer"+std::to_string(drawer):"";
                if (sprite(gloveL+suffix)) gloveL+=suffix;
                if (sprite(gloveR+suffix)) gloveR+=suffix;
            }
        }
    }
    if (f.charId == 1) {
        const std::string stem = fighterRigKey(f.charId, f.skinId) + "_" + liveForm(f, bodyName) + "_" + (backView ? "back" : "front") + "_";
        const std::string left = stem + (backView ? "0" : "1"), right = stem + (backView ? "1" : "0");
        if (sprite(left) && sprite(right)) { gloveL = left; gloveR = right; }
        if (f.telegraphFlash) gloveTint = sf::Color(255, 190, 110);
    }
    if (f.charId == 2) {
        const std::string stem=fighterRigKey(f.charId,f.skinId)+"_"+liveForm(f,bodyName)+"_"+(backView?"back":"front")+"_";
        const float p=f.maxTimer>0?1.0f-static_cast<float>(f.timer)/f.maxTimer:0;
        const bool released=casting && !f.signatureOnly && f.activeSkill==SkillId::JoanaHookPull && p>=.24f && p<.88f;
        const std::string left=stem+(released?"released":"0"),right=stem+"jaw"+std::to_string(joanaJawFrame(f));
        if(sprite(left)&&sprite(right)){gloveL=left;gloveR=right;armorName.clear();}
        // Full harpoon silhouette, with the same-sized gripping hand. Its atlas
        // is padded around the hand so the spear tip can never be clipped.
        if(f.skinId==1)sizeMulL*=1.5f;
        if(f.telegraphFlash)gloveTint=sf::Color(255,190,110);
    }
    if (f.charId == 7) {
        const std::string stem=fighterRigKey(f.charId,f.skinId)+"_"+liveForm(f,bodyName)+"_"+(backView?"back":"front")+"_";
        const std::string left=stem+(backView?"0":"1"),right=stem+(backView?"1":"0");
        if(sprite(left)&&sprite(right)){gloveL=left;gloveR=right;}
    }
    if (f.charId == 4) {
        const std::string stem=fighterRigKey(f.charId,f.skinId)+"_"+liveForm(f,bodyName)+"_"+(backView?"back":"front")+"_";
        const float p=f.maxTimer>0?std::clamp(1.f-static_cast<float>(f.timer)/f.maxTimer,0.f,1.f):0;
        auto ease=[](float x){x=std::clamp(x,0.f,1.f);return x*x*(3-2*x);};
        const bool guard=f.state==FIGHTER_BLOCK || f.status.heatwave>0;
        std::string suffix;
        if(f.skinId==3){
            const float open=guard?1.f:casting&&!f.signatureOnly&&f.activeSkill==SkillId::ValeriaHeatwaveGuard?ease(p/.15f):0;
            suffix="_fan"+std::to_string(static_cast<int>(std::round(open*8)));
        } else suffix=guard?"_block":"_"+std::to_string((frameCounter_/((isPunchState(f.state)||casting)?4:7))%8);
        const std::string left=stem+(backView?"0":"1")+suffix,right=stem+(backView?"1":"0")+suffix;
        if(sprite(left)&&sprite(right)){gloveL=left;gloveR=right;}
        // Padding around ribbon ends/fan tips, without changing hand proportions.
        sizeMulL*=1.5f;sizeMulR*=1.5f;
        if(f.telegraphFlash)gloveTint=sf::Color(255,190,110);
    }
    if (f.charId == 3) {
        const std::string stem=fighterRigKey(3,f.skinId)+"_"+liveForm(f,bodyName)+"_"+(backView?"back":"front")+"_";
        const unsigned clock=engine_?engine_->frameIndex:frameCounter_;
        const bool boiling=casting&&!f.signatureOnly&&f.activeSkill==SkillId::CremosoSyrupBlast;
        const std::string suffix=(boiling?"_boiling_":"_")+std::to_string((clock/12)%4);
        const std::string left=stem+(backView?"0":"1")+suffix,right=stem+(backView?"1":"0")+suffix;
        if(sprite(left)&&sprite(right)){gloveL=left;gloveR=right;}
        sizeMulL*=1.5f;sizeMulR*=1.5f;
        if(casting&&!f.signatureOnly&&f.activeSkill==SkillId::CremosoVanillaWhip){
            const float p=1.f-static_cast<float>(f.timer)/std::max(1,f.maxTimer);
            auto ease=[](float x){x=std::clamp(x,0.f,1.f);return x*x*(3-2*x);};
            const float vortex=ease(p/.10f)*(1-ease((p-.88f)/.12f));
            gloveTint.a=static_cast<uint8_t>(255*(1-vortex));
        }
        if(f.telegraphFlash)gloveTint=sf::Color(255,195,115);
    }
    if (f.charId == 5) {
        const std::string stem=fighterRigKey(f.charId,f.skinId)+"_"+liveForm(f,bodyName)+"_"+(backView?"back":"front")+"_";
        const unsigned clock=engine_?engine_->frameIndex:frameCounter_;
        const std::string suffix="_"+std::to_string(f.skinId==1?0:(clock*7/60)%24);
        // The alternative SVGs mirror their equipment in the rear view.
        const bool firstOnLeft=backView&&f.skinId==0;
        const std::string left=stem+(firstOnLeft?"0":"1")+suffix,right=stem+(firstOnLeft?"1":"0")+suffix;
        if(sprite(left)&&sprite(right)){gloveL=left;gloveR=right;}
        sizeMulL*=1.5f;sizeMulR*=1.5f;
        if(f.telegraphFlash)gloveTint=sf::Color(255,215,150);
    }
    if(f.charId==6 && hasIntegratedSkin(6)) {
        const std::string stem=fighterRigKey(6,f.skinId)+"_"+liveForm(f,bodyName)+"_"+(backView?"back":"front")+"_";
        const unsigned clock=engine_?engine_->frameIndex:frameCounter_;
        const std::string suffix="_"+std::to_string((clock/4)%8);
        gloveL=stem+(backView?"0":"1")+suffix;gloveR=stem+(backView?"1":"0")+suffix;
        sizeMulL*=1.5f;sizeMulR*=1.5f;
        if(f.telegraphFlash)gloveTint=sf::Color(255,210,140);
    }
    if(f.charId==8 && hasIntegratedSkin(8)) {
        const std::string stem=fighterRigKey(8,f.skinId)+"_"+liveForm(f,bodyName)+"_"+(backView?"back":"front")+"_";
        const unsigned clock=engine_?engine_->frameIndex:frameCounter_;
        const std::string suffix="_"+std::to_string((clock/3)%8);
        const bool launched=casting && !f.signatureOnly && f.activeSkill==SkillId::BossDualSaw;
        const std::string socket=fighterRigKey(8,f.skinId)+"_socket_"+(backView?"back":"front")+"_";
        gloveL=launched?socket+"0":stem+"0"+suffix;
        gloveR=launched?socket+"1":stem+"1"+suffix;
        // The approved art already spins around the native weapon center.
        rotL=rotR=0;sizeMulL*=1.5f;sizeMulR*=1.5f;
    }
    auto body = bodyName.empty() ? std::nullopt : sprite(bodyName);
    auto spriteL = gloveL.empty() ? std::nullopt : sprite(gloveL);
    auto spriteR = gloveR.empty() ? std::nullopt : sprite(gloveR);
    if (!body || !spriteL || !spriteR) {
        queuePlaceholderFighter(f, place, backView);
        return;
    }

    auto toWorld = [&](float lx, float ly, float lz) {
        Vec3 w = Physics3D::localToWorld(lx, ly, lz, place.pos, place.facing);
        if (!koFlight) {
            // Barreira física das cordas ("sem atravessar"):
            const float bodyThickness = f.isBigChar() ? 34.0f : (f.isSmallChar() ? 18.0f : 25.0f);
            const float maxLimit = kRingHalf - bodyThickness + 6.0f;
            w.x = std::clamp(w.x, -maxLimit, maxLimit);
            w.z = std::clamp(w.z, -maxLimit, maxLimit);
        }
        return w;
    };
    const float ox = f.offsetX + rx.dx, oy = f.offsetY + rx.dy, oz = f.offsetZ + rx.dz, skewB = f.bodySkew + rx.skew;
    const float bodyHs = bodyH * rx.squash;
    if (rx.dust) {
        std::uniform_real_distribution<float> u(-1.0f, 1.0f);
        for (int i = 0; i < 12; ++i) {
            const Vec3 w = toWorld(ox + u(rng_) * 60.0f, 6.0f + std::fabs(u(rng_)) * 20.0f, oz + u(rng_) * 30.0f);
            flashes_.push_back({w, 0.8f, 0.04f, 24.0f + u(rng_) * 8.0f});
        }
    }

    // Golpe em andamento: a luva cresce até 42% no auge, como no JS.
    const bool attacking = isPunchState(f.state);
    float swingL = 0.0f, swingR = 0.0f;
    if (attacking && f.maxTimer > 0) {
        float p = 1.0f - static_cast<float>(f.timer) / static_cast<float>(f.maxTimer);
        if (p >= 0.16f) {
            float sw = std::sin((p - 0.16f) / 0.84f * kPi);
            (f.isLeftPunch ? swingL : swingR) = sw;
        }
    }
    const bool punchL = attacking && f.isLeftPunch, punchR = attacking && !f.isLeftPunch;

    const bool isNearFighter = f.isPlayer != reverseView_;
    // Quando o lutador próximo é ejetado (koFlight), ele é catapultado por cima das cordas
    // da frente em direção à câmera, devendo ser desenhado na frente das cordas (camada 5.4 > 5.1).
    const bool overNearRopes = isNearFighter && koFlight && koRopeFrame_ >= 0;
    float bodyLayer = overNearRopes ? 5.4f : (isNearFighter ? 4.4f : 3.5f);
    float layerL, layerR;
    if (backView) {
        // De costas: luvas paradas atrás do corpo; só a que soca passa na frente.
        layerL = punchL ? bodyLayer + 0.2f : bodyLayer - 0.2f;
        layerR = punchR ? bodyLayer + 0.2f : bodyLayer - 0.2f;
    } else {
        float rest = down ? bodyLayer - 0.1f : bodyLayer + 0.1f;
        layerL = punchL ? bodyLayer + 0.2f : rest;
        layerR = punchR ? bodyLayer + 0.2f : rest;
    }

    sf::Color tint = sf::Color::White;
    if (f.state == FIGHTER_HIT) tint = sf::Color(255, 195, 195);
    else if (f.isExhausted && !isHeroic) tint = sf::Color(190, 190, 210);

    if (f.monstrous && f.charId != 2 && f.state != FIGHTER_HIT) tint = sf::Color(255, 205, 185); // approved monster palettes must not be recolored
    queueLabLayers(f, backView, bodyLayer, toWorld(ox, bodyLift + oy, oz), bodyW, bodyHs);
    // Corpo substituído por desenho procedural do Lab (ex.: Tornado de Baunilha do Cremoso, sem luvas).
    if (const auto& fx = lab_[f.isPlayer ? 0 : 1]; fx && fx->overridesBody(labContext(f, f.isPlayer))) {
        const Vec3 at = toWorld(ox, bodyLift + oy, oz);
        if (auto pr = camera_.project(at)) {
            const std::size_t slot = f.isPlayer ? 0 : 1;
            const sf::Vector2f pos{pr->pos.x, pr->pos.y - 10.0f * pr->scale};
            const float sc = pr->scale;
            queue_.push_back({bodyLayer, pr->depth, [this, slot, pos, sc, bodyW, bodyH](sf::RenderTarget& t) {
                                  if (!engine_ || !lab_[slot]) return;
                                  const Fighter& who = slot == 0 ? engine_->player : engine_->enemy;
                                  vfx::LabContext ctx = labContext(who, slot == 0);
                                  cv::Canvas canvas(t);
                                  lab_[slot]->drawBodyReplacement(canvas, ctx, pos, sc, bodyW, bodyH);
                              }});
        }
        return;
    }
    bool liveGloves = false;
    if ((!isSpinRx || hasIntegratedSkin(f.charId)) && queueLiveBody(f, rx, bodyName, toWorld(ox, bodyLift + oy, oz), bodyW, bodyHs, skewB, bodyLayer, tint, backView, liveGloves)) {
        // Rosto e cabeça vêm das peças; do sprite antigo só estrelas e gotas por cima.
        Reaction over = rx;
        over.face = 0;
        over.headRot = over.headShiftX = over.headShiftY = 0.0f;
        over.headScaleX = 1.0f;
        queueBody(*body, toWorld(ox, bodyLift + oy, oz), bodyW, bodyHs, skewB, bodyLayer, tint, over, f.charId, backView, true, false);
        if (liveGloves) return; // na assinatura as luvas fazem parte da animação
    } else if (!isSpinRx && ((jackRig && queueJackRig(f, rx, toWorld(ox, bodyLift + oy, oz), bodyW, bodyHs, skewB, bodyLayer, tint, backView)) ||
        (castRig && queueFighterRig(f, rx, bodyName, toWorld(ox, bodyLift + oy, oz), bodyW, bodyHs, skewB, bodyLayer, tint, backView)))) {
        // Só estrelas e gotas por cima; o rosto já vem das partes.
        Reaction over = rx;
        over.face = 0;
        over.headRot = over.headShiftX = over.headShiftY = 0.0f;
        over.headScaleX = 1.0f;
        queueBody(*body, toWorld(ox, bodyLift + oy, oz), bodyW, bodyHs, skewB, bodyLayer, tint, over, f.charId, backView, true, false);
    } else {
        queueBody(*body, toWorld(ox, bodyLift + oy, oz), rx.mirror ? -bodyW : bodyW, bodyHs, skewB, bodyLayer, tint, rx, f.charId, backView, true);
    }
    if (!isSpinRx && !armorName.empty()) {
        if (auto armor = sprite(armorName)) {
            queueBody(*armor, toWorld(ox, bodyLift + oy, oz), bodyW, bodyHs, skewB, bodyLayer + 0.05f, sf::Color::White, rx, f.charId, backView, false);
        }
    }

    PartRenderData gl = f.gloveL, gr = f.gloveR;
    // Reação: luvas sobem/abrem no impacto; no golpe no corpo uma delas segura a barriga.
    float sxLocal = 1.0f; // +1 se o x local do lutador aponta para a direita da tela
    if (auto a = camera_.project(toWorld(ox, bodyLift, oz)); a) {
        if (auto b = camera_.project(toWorld(ox + 20.0f, bodyLift, oz)); b) sxLocal = b->pos.x >= a->pos.x ? 1.0f : -1.0f;
    }
    gl.y += rx.gloveLift; gr.y += rx.gloveLift;
    gl.x += (gl.x < 0 ? -1.0f : 1.0f) * rx.gloveSpread + rx.gloveShift * sxLocal;
    gr.x += (gr.x < 0 ? -1.0f : 1.0f) * rx.gloveSpread + rx.gloveShift * sxLocal;
    if (rx.clutch > 0.0f) {
        PartRenderData& hold = rx.clutchLeft ? gl : gr;
        PartRenderData& loose = rx.clutchLeft ? gr : gl;
        const float side = rx.clutchLeft ? -1.0f : 1.0f;
        hold.x += (side * 6.0f - hold.x) * rx.clutch;
        hold.y += (bodyLift + bodyHs * 0.08f - hold.y) * rx.clutch;
        (rx.clutchLeft ? sizeMulL : sizeMulR) *= 1.0f - 0.2f * rx.clutch;
        hold.z += (24.0f - hold.z) * rx.clutch;
        loose.y -= 26.0f * rx.clutch;
        loose.x += (loose.x < 0 ? -1.0f : 1.0f) * 12.0f * rx.clutch;
        (rx.clutchLeft ? rotL : rotR) += side * 0.5f * rx.clutch;
    }
    if (rx.rope > 0.0f) {
        // Agarrado nas cordas: braços abertos para trás, luvas na corda de cima.
        const float reach = std::fabs(bodyW) * 0.62f;
        gl.x += (-reach - gl.x) * rx.rope; gr.x += (reach - gr.x) * rx.rope;
        gl.y += (kRopeHeights[2] - 34.0f - oy - gl.y) * rx.rope; gr.y += (kRopeHeights[2] - 34.0f - oy - gr.y) * rx.rope;
        gl.z += (-8.0f - gl.z) * rx.rope; gr.z += (-8.0f - gr.z) * rx.rope;
        rotL += -0.5f * rx.rope; rotR += 0.5f * rx.rope;
    }
    if (koFlight) {
        // As luvas giram junto, em volta do eixo vertical; as que passam para trás ficam atrás do corpo.
        const float cs = std::cos(rx.yaw), sn = std::sin(rx.yaw);
        for (PartRenderData* g : {&gl, &gr}) {
            const float x = g->x, z = g->z;
            g->x = x * cs - z * sn;
            g->z = x * sn + z * cs;
        }
        layerL = gl.z < 0.0f ? bodyLayer - 0.05f : bodyLayer + 0.1f;
        layerR = gr.z < 0.0f ? bodyLayer - 0.05f : bodyLayer + 0.1f;
    }
    if (rx.roll != 0.0f) {
        // Gira as luvas junto com o corpo, em volta do centro do sprite (sinal conforme o lado da tela).
        const float cy = bodyLift + bodyHs * 0.5f;
        const float ang = -rx.roll * sxLocal, cs = std::cos(ang), sn = std::sin(ang);
        for (PartRenderData* g : {&gl, &gr}) {
            const float dx = g->x, dy = g->y - cy;
            g->x = dx * cs - dy * sn;
            g->y = cy + dx * sn + dy * cs;
        }
        rotL += rx.roll; rotR += rx.roll;
    }
    float sizeL = gloveSize * sizeMulL * (1.0f + 0.42f * swingL), sizeR = gloveSize * sizeMulR * (1.0f + 0.42f * swingR);
    if (f.charId == 1 && hasIntegratedSkin(1)) {
        // The approved gloves fill 76.5% of their atlas square, compared with
        // 58.3% in the native sprite. Undo that export-padding size inflation
        // for all four skins, keeping the hand centre and native attack reach.
        constexpr float paddingCompensation = .762f;
        liftL += sizeL * (1-paddingCompensation) * .5f;
        liftR += sizeR * (1-paddingCompensation) * .5f;
        sizeL *= paddingCompensation;
        sizeR *= paddingCompensation;
    }
    if(f.charId==3 || f.charId==5 || f.charId==6 || f.charId==8) {
        // 180-unit padded equipment replaces the native 120-unit frame. Keep
        // its centre at the native hand, including the punch's size multiplier.
        liftL-=sizeL/6.0f;liftR-=sizeR/6.0f;
    }
    {
        FighterCapture& cap = capture[isPlayer ? 0 : 1];
        cap.valid = false;
        if (auto pb = camera_.project(toWorld(ox, bodyLift + oy, oz))) {
            cap.valid = true;
            cap.bodyX = pb->pos.x; cap.bodyY = pb->pos.y;
            cap.bodyW = bodyW * pb->scale; cap.bodyH = bodyHs * pb->scale; cap.skew = skewB;
            const Vec3 wl = toWorld(gl.x + ox, gl.y + oy + liftL, gl.z + oz), wr = toWorld(gr.x + ox, gr.y + oy + liftR, gr.z + oz);
            const float sz[2] = {sizeL, sizeR}, rt[2] = {rotL, rotR}, sk[2] = {skewL, skewR}, ly[2] = {layerL, layerR};
            const Vec3 w[2] = {wl, wr};
            for (int k = 0; k < 2; ++k)
                if (auto pg = camera_.project(w[k]))
                    cap.glove[k] = {pg->pos.x, pg->pos.y, sz[k] * pg->scale, rt[k], sk[k], ly[k] > bodyLayer, k ? gloveR : gloveL};
        }
    }
    const bool espinhaSig = (f.charId == 6 && isHeroic && !hasIntegratedSkin(6));
    if (espinhaSig) {
        gl.x -= 24.0f;
        gr.x += 24.0f;
    }
    const float widthL = espinhaSig ? -sizeL : sizeL;
    const float widthR = espinhaSig ? -sizeR : sizeR;
    const Vec3 gloveWorldL = toWorld(gl.x + ox, gl.y + oy + liftL, gl.z + oz);
    const Vec3 gloveWorldR = toWorld(gr.x + ox, gr.y + oy + liftR, gr.z + oz);
    queueSprite(*spriteL, gloveWorldL, widthL, sizeL, skewL, layerL, gloveTint, rotL);
    queueSprite(*spriteR, gloveWorldR, widthR, sizeR, skewR, layerR, gloveTint, rotR);
    addShadowGlove(*spriteL, gloveWorldL, widthL, sizeL, skewL, rotL, isPlayer ? 0 : 1);
    addShadowGlove(*spriteR, gloveWorldR, widthR, sizeR, skewR, rotR, isPlayer ? 0 : 1);

    // Batalhator danificado: mira laser vermelha da cabeça-punho ao armar o golpe pesado.
    if (f.bossStage == 2 && f.queuedHeavy && (f.state == FIGHTER_TELEGRAPH || isPunchState(f.state))) {
        if (auto from = camera_.project(toWorld(gl.x + ox, gl.y + oy, gl.z + oz))) {
            std::uniform_real_distribution<float> jitter(-40.0f, 40.0f);
            sf::Vector2f a = from->pos, b{kLogicalWidth / 2.0f + jitter(rng_), kLogicalHeight - 100.0f};
            float thick = std::max(2.0f, 4.0f * from->scale);
            queue_.push_back({layerL + 0.05f, from->depth, [a, b, thick](sf::RenderTarget& tg) {
                                  drawThickLine(tg, a, b, thick, sf::Color(255, 0, 68, 205));
                              }});
        }
    }
}

void RingRenderer::queuePlaceholderFighter(const Fighter& f, const Placement& place, bool backView) {
    // Lutadores ainda sem spritesheet exportada: tronco e luvas em formas simples.
    const CharacterLook& look = lookFor(f.charId);
    const float bodyW = isBig(f.charId) ? 140.0f : 110.0f;
    const float bodyH = isBig(f.charId) ? 170.0f : 150.0f;
    auto toWorld = [&](float lx, float ly, float lz) { return Physics3D::localToWorld(lx, ly, lz, place.pos, place.facing); };

    auto bottom = camera_.project(toWorld(f.offsetX, f.offsetY, f.offsetZ));
    auto top = camera_.project(toWorld(f.offsetX + f.bodySkew * bodyH * 0.5f, f.offsetY + bodyH, f.offsetZ));
    if (bottom && top) {
        float wb = bodyW * 0.38f * bottom->scale, wt = bodyW * 0.5f * top->scale;
        sf::Vector2f b = bottom->pos, tp = top->pos;
        sf::Color fill = look.color;
        bool flash = f.telegraphFlash;
        queue_.push_back({backView ? 4.4f : 3.5f, bottom->depth, [b, tp, wb, wt, fill, flash](sf::RenderTarget& t) {
                              sf::ConvexShape torso(4);
                              torso.setPoint(0, {tp.x - wt, tp.y});
                              torso.setPoint(1, {tp.x + wt, tp.y});
                              torso.setPoint(2, {b.x + wb, b.y});
                              torso.setPoint(3, {b.x - wb, b.y});
                              torso.setFillColor(fill);
                              torso.setOutlineColor(flash ? sf::Color::White : kInk);
                              torso.setOutlineThickness(flash ? 5.0f : 2.5f);
                              t.draw(torso);
                          }});
    }
    for (const PartRenderData* g : {&f.gloveL, &f.gloveR}) {
        auto p = camera_.project(toWorld(g->x + f.offsetX, g->y + f.offsetY, g->z + f.offsetZ));
        if (!p) continue;
        float r = g->w * 0.5f * p->scale;
        sf::Vector2f c = p->pos;
        queue_.push_back({backView ? 4.2f : 3.6f, p->depth, [c, r](sf::RenderTarget& t) {
                              sf::CircleShape glove(r);
                              glove.setOrigin({r, r});
                              glove.setPosition(c);
                              glove.setFillColor(sf::Color(0xd6, 0x28, 0x28));
                              glove.setOutlineColor(kInk);
                              glove.setOutlineThickness(3.0f);
                              t.draw(glove);
                          }});
    }
}

void RingRenderer::queueScarecrow(const Fighter& f, const Placement& place, bool backView) {
    auto toWorld = [&](float lx, float ly, float lz) {
        return Physics3D::localToWorld(lx, ly, lz, place.pos, place.facing);
    };

    auto groundPt = camera_.project(toWorld(f.offsetX, 0.0f, f.offsetZ));
    if (!groundPt) return;

    const float sc = groundPt->scale;
    const sf::Vector2f groundPos = groundPt->pos;
    const float depth = groundPt->depth;
    const float bodyLayer = backView ? 4.4f : 3.5f;
    // Original remains its exact procedural renderer. The alternatives use the
    // approved vector bodies and the same normalized native spring reactions.
    // No movement, timer, collision or training rule is changed here.
    if (f.skinId > 0 && sprites_) {
        const auto* rig = sprites_->rig(fighterRigKey(9,f.skinId));
        std::string action="idle";
        const bool hit=f.state==FIGHTER_HIT && f.maxTimer>0;
        if(hit){
            if(f.hitKind>=1&&f.hitKind<=4)action=f.hitDir>=0?"hit"+std::to_string(f.hitKind):"dummy_hit"+std::to_string(f.hitKind)+"_R";
            else action=f.hitDir>=0?"dummy_hit0_L":"dummy_hit0_R";
        }
        const auto* clip=rig?rig->clip("base",backView,action):nullptr;
        if(clip&&!clip->frames.empty()){
            const float p=hit?std::clamp(1.f-float(f.timer)/f.maxTimer,0.f,1.f):std::fmod(float(frameCounter_)/60.f,clip->dur)/clip->dur;
            const auto index=std::min(clip->frames.size()-1,std::size_t(std::round(p*(clip->frames.size()-1))));
            const auto& fr=clip->frames[index];
            struct DummyBatch{const sf::Texture* texture;std::vector<sf::Vertex> vertices;};
            std::vector<DummyBatch> batches;
            for(unsigned j=0;j<fr.count;++j){
                const auto& in=rig->insts[fr.first+j];const auto& pt=rig->parts[in.part];const auto* texture=rig->sheets[pt.sheet].get();
                if(batches.empty()||batches.back().texture!=texture)batches.push_back({texture,{}});
                auto& vertices=batches.back().vertices;
                const float pw=pt.rect.size.x*pt.upp,ph=pt.rect.size.y*pt.upp;
                const sf::Color color(255,255,255,std::uint8_t(std::clamp(in.op*fr.opacity,0.f,1.f)*255));
                auto vertex=[&](float u,float v){const float x=pt.ox+u*pw,y=pt.oy+v*ph;return sf::Vertex{groundPos+sf::Vector2f{(in.a*x+in.c*y+in.e)*sc,(in.b*x+in.d*y+in.f)*sc},color,pt.rect.position+sf::Vector2f{u*pt.rect.size.x,v*pt.rect.size.y}};};
                const auto a=vertex(0,0),b=vertex(1,0),c=vertex(1,1),d=vertex(0,1);vertices.insert(vertices.end(),{a,b,c,a,c,d});
            }
            for(auto& batch:batches)queue_.push_back({bodyLayer,depth,[batch=std::move(batch)](sf::RenderTarget& target){sf::RenderStates states;states.texture=batch.texture;target.draw(batch.vertices.data(),batch.vertices.size(),sf::PrimitiveType::Triangles,states);}});
            auto& cap=capture[f.isPlayer?0:1];cap.valid=true;
            return;
        }
    }

    const float frame = static_cast<float>(frameCounter_);

    // Reações físicas de mola e oscilação
    float wobbleAngle = 0.0f;
    float wobbleX = 0.0f;
    float wobbleY = 0.0f;
    float squash = 1.0f;
    float stretch = 1.0f;
    float hatPop = 0.0f;
    float hatTilt = 0.0f;
    bool hurtEyes = false;
    bool shockMouth = false;
    float springVibrate = 0.0f;

    if (f.state == FIGHTER_HIT && f.maxTimer > 0) {
        const float p = 1.0f - static_cast<float>(f.timer) / static_cast<float>(f.maxTimer);
        const float d = f.hitDir >= 0 ? 1.0f : -1.0f;
        hurtEyes = true;
        shockMouth = (f.hitKind == 2 || f.hitKind == 4);
        const float decay = std::pow(1.0f - p, 1.3f);

        switch (f.hitKind) {
        case 1: // Jab
            wobbleAngle = -d * 0.12f * decay * std::sin(p * kPi * 5.0f);
            wobbleX = -d * 8.0f * decay * std::sin(p * kPi * 5.0f);
            springVibrate = decay * std::sin(p * kPi * 8.0f) * 4.0f;
            break;
        case 2: // Gancho / Cruzado
            wobbleAngle = -d * 0.28f * decay * std::sin(p * kPi * 3.5f);
            wobbleX = -d * 20.0f * decay * std::sin(p * kPi * 3.5f);
            springVibrate = decay * std::sin(p * kPi * 6.0f) * 6.0f;
            hatTilt = d * 0.22f * decay;
            hatPop = 8.0f * std::sin(p * kPi);
            break;
        case 3: // Golpe no corpo
            squash = 1.0f + 0.14f * std::sin(p * kPi);
            stretch = 1.0f - 0.20f * std::sin(p * kPi);
            wobbleY = 12.0f * std::sin(p * kPi);
            springVibrate = decay * 5.0f;
            break;
        case 4: // Uppercut (salto do chapéu no ar)
            stretch = 1.0f + 0.24f * std::sin(p * kPi);
            squash = 1.0f - 0.12f * std::sin(p * kPi);
            wobbleY = -22.0f * std::sin(p * kPi);
            hatPop = 12.0f * std::sin(p * kPi);
            hatTilt = (std::fmod(frame, 2.0f) == 0 ? 0.14f : -0.14f) * std::sin(p * kPi);
            break;
        default:
            wobbleAngle = -d * 0.15f * decay * std::sin(p * kPi * 4.0f);
            wobbleX = -d * 12.0f * decay;
            break;
        }
    } else {
        const float idleWave = std::sin(frame * 0.05f);
        wobbleAngle = idleWave * 0.02f;
        wobbleX = idleWave * 1.5f;
        wobbleY = std::sin(frame * 0.08f) * 1.0f;
    }

    const float gloveSwing = (wobbleAngle * -42.0f) + std::sin(frame * 0.06f) * 3.5f;
    const bool flash = f.telegraphFlash;

    queue_.push_back({bodyLayer, depth, [=](sf::RenderTarget& t) {
        cv::Canvas ctx(t);

        // 1. Sombra circular no piso do ringue
        ctx.save();
        ctx.translate(groundPos.x, groundPos.y);
        ctx.beginPath();
        ctx.ellipse(0.0f, 0.0f, 46.0f * sc, 15.0f * sc, 0.0f, 0.0f, 2.0f * kPi);
        ctx.fillStyle = sf::Color(0, 0, 0, 90);
        ctx.fill();

        // 2. Base de madeira em cruz no chão (estacionária)
        ctx.beginPath();
        ctx.moveTo(-10.0f * sc, 4.0f * sc);
        ctx.lineTo(-6.0f * sc, -14.0f * sc);
        ctx.lineTo(6.0f * sc, -14.0f * sc);
        ctx.lineTo(10.0f * sc, 4.0f * sc);
        ctx.closePath();
        ctx.fillStyle = sf::Color(78, 48, 24);
        ctx.fill();
        ctx.strokeStyle = kInk;
        ctx.lineWidth = 2.5f * sc;
        ctx.stroke();

        ctx.beginPath();
        ctx.roundRect(-44.0f * sc, -8.0f * sc, 88.0f * sc, 10.0f * sc, 2.0f * sc);
        ctx.fillStyle = sf::Color(115, 75, 42);
        ctx.fill();
        ctx.strokeStyle = kInk;
        ctx.lineWidth = 2.5f * sc;
        ctx.stroke();

        ctx.beginPath();
        ctx.moveTo(-40.0f * sc, -5.0f * sc);
        ctx.lineTo(40.0f * sc, -5.0f * sc);
        ctx.strokeStyle = sf::Color(150, 102, 60);
        ctx.lineWidth = 1.5f * sc;
        ctx.stroke();

        for (float bx : {-32.0f, 32.0f}) {
            ctx.beginPath();
            ctx.arc(bx * sc, -3.0f * sc, 2.5f * sc, 0.0f, 2.0f * kPi);
            ctx.fillStyle = sf::Color(50, 52, 58);
            ctx.fill();
            ctx.strokeStyle = kInk;
            ctx.lineWidth = 1.0f * sc;
            ctx.stroke();
        }

        // Colar de ferro de montagem da mola
        ctx.beginPath();
        ctx.roundRect(-14.0f * sc, -14.0f * sc, 28.0f * sc, 8.0f * sc, 2.0f * sc);
        ctx.fillStyle = sf::Color(70, 75, 82);
        ctx.fill();
        ctx.strokeStyle = kInk;
        ctx.lineWidth = 2.0f * sc;
        ctx.stroke();

        // Mola espiral metálica de impacto
        const float springBaseY = -14.0f * sc;
        const float springTopY = -46.0f * sc + wobbleY * sc * 0.3f;
        const float springTopX = wobbleX * sc * 0.3f + springVibrate * sc;
        const int kCoils = 6;
        ctx.beginPath();
        ctx.moveTo(0.0f, springBaseY);
        for (int i = 0; i < kCoils; ++i) {
            float t0 = static_cast<float>(i) / kCoils;
            float t1 = static_cast<float>(i + 1) / kCoils;
            float cy0 = springBaseY + (springTopY - springBaseY) * (t0 + 0.5f / kCoils);
            float cx = (i % 2 == 0 ? -12.0f : 12.0f) * sc + (springTopX * t0);
            ctx.quadraticCurveTo(cx, cy0, springTopX * t1, springBaseY + (springTopY - springBaseY) * t1);
        }
        ctx.strokeStyle = sf::Color(55, 60, 68);
        ctx.lineWidth = 7.0f * sc;
        ctx.stroke();
        ctx.strokeStyle = sf::Color(175, 185, 195);
        ctx.lineWidth = 2.4f * sc;
        ctx.stroke();
        ctx.restore();

        // 3. Estrutura Articulada do Espantalho (Mastro, Tronco, Braços, Luvas, Cabeça, Chapéu)
        ctx.save();
        ctx.translate(groundPos.x + wobbleX * sc, groundPos.y - 46.0f * sc + wobbleY * sc);
        ctx.rotate(wobbleAngle);
        ctx.scale(squash, stretch);

        // Mastro vertical de madeira
        ctx.beginPath();
        ctx.roundRect(-6.0f * sc, -135.0f * sc, 12.0f * sc, 135.0f * sc, 2.0f * sc);
        ctx.fillStyle = sf::Color(115, 75, 42);
        ctx.fill();
        ctx.strokeStyle = flash ? sf::Color::White : kInk;
        ctx.lineWidth = 2.5f * sc;
        ctx.stroke();
        ctx.beginPath();
        ctx.moveTo(-3.0f * sc, 0.0f);
        ctx.lineTo(-3.0f * sc, -133.0f * sc);
        ctx.strokeStyle = sf::Color(150, 102, 60);
        ctx.lineWidth = 1.5f * sc;
        ctx.stroke();

        // Travessão horizontal dos ombros/braços
        ctx.beginPath();
        ctx.roundRect(-52.0f * sc, -74.0f * sc, 104.0f * sc, 10.0f * sc, 2.0f * sc);
        ctx.fillStyle = sf::Color(115, 75, 42);
        ctx.fill();
        ctx.strokeStyle = flash ? sf::Color::White : kInk;
        ctx.lineWidth = 2.5f * sc;
        ctx.stroke();

        // Tufos de palha saindo das pontas dos braços
        auto drawStrawTuft = [&](float tx, float ty, float dirX) {
            ctx.fillStyle = sf::Color(250, 212, 60);
            ctx.strokeStyle = kInk;
            ctx.lineWidth = 1.8f * sc;
            ctx.beginPath();
            ctx.moveTo(tx, ty - 4.0f * sc);
            ctx.lineTo(tx + dirX * 14.0f * sc, ty - 6.0f * sc);
            ctx.lineTo(tx + dirX * 10.0f * sc, ty);
            ctx.lineTo(tx + dirX * 16.0f * sc, ty + 4.0f * sc);
            ctx.lineTo(tx, ty + 5.0f * sc);
            ctx.closePath();
            ctx.fill();
            ctx.stroke();
        };
        drawStrawTuft(-52.0f * sc, -69.0f * sc, -1.0f);
        drawStrawTuft(52.0f * sc, -69.0f * sc, 1.0f);

        // Amarras em X de corda na união dos mastros
        ctx.strokeStyle = sf::Color(218, 186, 122);
        ctx.lineWidth = 3.5f * sc;
        ctx.beginPath();
        ctx.moveTo(-7.0f * sc, -76.0f * sc);
        ctx.lineTo(7.0f * sc, -62.0f * sc);
        ctx.moveTo(7.0f * sc, -76.0f * sc);
        ctx.lineTo(-7.0f * sc, -62.0f * sc);
        ctx.stroke();
        ctx.strokeStyle = kInk;
        ctx.lineWidth = 1.0f * sc;
        ctx.stroke();

        // Luvas de boxe antigas penduradas (balançam com física de inércia)
        auto drawBoxingGlove = [&](float hangX, float hangY, float swing, bool isLeft) {
            ctx.save();
            ctx.translate(hangX, hangY);
            ctx.rotate(swing * (kPi / 180.0f));

            ctx.strokeStyle = sf::Color(200, 170, 110);
            ctx.lineWidth = 2.0f * sc;
            ctx.beginPath();
            ctx.moveTo(0.0f, 0.0f);
            ctx.lineTo(0.0f, 22.0f * sc);
            ctx.stroke();

            ctx.translate(0.0f, 22.0f * sc);

            // Munhequeira branca/fita
            ctx.beginPath();
            ctx.roundRect(-8.0f * sc, -4.0f * sc, 16.0f * sc, 7.0f * sc, 2.0f * sc);
            ctx.fillStyle = sf::Color(235, 235, 230);
            ctx.fill();
            ctx.strokeStyle = kInk;
            ctx.lineWidth = 2.0f * sc;
            ctx.stroke();

            // Luva de couro vermelha
            ctx.beginPath();
            ctx.roundRect(-12.0f * sc, 2.0f * sc, 24.0f * sc, 22.0f * sc, 8.0f * sc);
            ctx.fillStyle = sf::Color(185, 36, 42);
            ctx.fill();
            // Sombra da luva
            ctx.fillStyle = sf::Color(122, 20, 26);
            ctx.beginPath();
            ctx.roundRect(isLeft ? -12.0f * sc : 2.0f * sc, 2.0f * sc, 10.0f * sc, 22.0f * sc, 6.0f * sc);
            ctx.fill();
            // Contorno
            ctx.beginPath();
            ctx.roundRect(-12.0f * sc, 2.0f * sc, 24.0f * sc, 22.0f * sc, 8.0f * sc);
            ctx.strokeStyle = flash ? sf::Color::White : kInk;
            ctx.lineWidth = 2.5f * sc;
            ctx.stroke();

            // Polegar da luva
            float thumbX = isLeft ? 10.0f * sc : -10.0f * sc;
            ctx.beginPath();
            ctx.ellipse(thumbX, 10.0f * sc, 5.0f * sc, 8.0f * sc, (isLeft ? 0.3f : -0.3f), 0.0f, 2.0f * kPi);
            ctx.fillStyle = sf::Color(185, 36, 42);
            ctx.fill();
            ctx.strokeStyle = flash ? sf::Color::White : kInk;
            ctx.lineWidth = 2.0f * sc;
            ctx.stroke();

            ctx.restore();
        };
        drawBoxingGlove(-40.0f * sc, -70.0f * sc, gloveSwing - 6.0f, true);
        drawBoxingGlove(40.0f * sc, -70.0f * sc, gloveSwing + 6.0f, false);

        // Palha saindo de baixo do tronco
        ctx.fillStyle = sf::Color(250, 212, 60);
        ctx.strokeStyle = kInk;
        ctx.lineWidth = 1.8f * sc;
        ctx.beginPath();
        ctx.moveTo(-20.0f * sc, -14.0f * sc);
        for (float sx = -18.0f; sx <= 18.0f; sx += 6.0f) {
            ctx.lineTo((sx + 3.0f) * sc, (-8.0f - std::fabs(std::fmod(sx, 4.0f))) * sc);
            ctx.lineTo((sx + 6.0f) * sc, -14.0f * sc);
        }
        ctx.fill();
        ctx.stroke();

        // Tronco de saco de estopa (burlap sack)
        ctx.beginPath();
        ctx.moveTo(-16.0f * sc, -82.0f * sc);
        ctx.bezierCurveTo(-30.0f * sc, -70.0f * sc, -34.0f * sc, -45.0f * sc, -28.0f * sc, -20.0f * sc);
        ctx.bezierCurveTo(-26.0f * sc, -12.0f * sc, -18.0f * sc, -14.0f * sc, 0.0f, -14.0f * sc);
        ctx.bezierCurveTo(18.0f * sc, -14.0f * sc, 26.0f * sc, -12.0f * sc, 28.0f * sc, -20.0f * sc);
        ctx.bezierCurveTo(34.0f * sc, -45.0f * sc, 30.0f * sc, -70.0f * sc, 16.0f * sc, -82.0f * sc);
        ctx.closePath();
        ctx.fillStyle = sf::Color(212, 168, 92);
        ctx.fill();

        // Sombra suave na lateral direita do estopa (seguindo a curvatura real do tronco)
        ctx.fillStyle = sf::Color(156, 116, 52, 175);
        ctx.beginPath();
        ctx.moveTo(4.0f * sc, -82.0f * sc);
        ctx.lineTo(16.0f * sc, -82.0f * sc);
        ctx.bezierCurveTo(30.0f * sc, -70.0f * sc, 34.0f * sc, -45.0f * sc, 28.0f * sc, -20.0f * sc);
        ctx.bezierCurveTo(26.0f * sc, -12.0f * sc, 18.0f * sc, -14.0f * sc, 4.0f * sc, -14.0f * sc);
        ctx.closePath();
        ctx.fill();

        // Brilho suave na lateral esquerda do estopa (seguindo a curvatura real do tronco)
        ctx.fillStyle = sf::Color(238, 204, 140, 130);
        ctx.beginPath();
        ctx.moveTo(-8.0f * sc, -82.0f * sc);
        ctx.lineTo(-16.0f * sc, -82.0f * sc);
        ctx.bezierCurveTo(-30.0f * sc, -70.0f * sc, -34.0f * sc, -45.0f * sc, -28.0f * sc, -20.0f * sc);
        ctx.bezierCurveTo(-26.0f * sc, -12.0f * sc, -18.0f * sc, -14.0f * sc, -8.0f * sc, -14.0f * sc);
        ctx.closePath();
        ctx.fill();

        // Contorno do tronco
        ctx.strokeStyle = flash ? sf::Color::White : kInk;
        ctx.lineWidth = 3.2f * sc;
        ctx.beginPath();
        ctx.moveTo(-16.0f * sc, -82.0f * sc);
        ctx.bezierCurveTo(-30.0f * sc, -70.0f * sc, -34.0f * sc, -45.0f * sc, -28.0f * sc, -20.0f * sc);
        ctx.bezierCurveTo(-26.0f * sc, -12.0f * sc, -18.0f * sc, -14.0f * sc, 0.0f, -14.0f * sc);
        ctx.bezierCurveTo(18.0f * sc, -14.0f * sc, 26.0f * sc, -12.0f * sc, 28.0f * sc, -20.0f * sc);
        ctx.bezierCurveTo(34.0f * sc, -45.0f * sc, 30.0f * sc, -70.0f * sc, 16.0f * sc, -82.0f * sc);
        ctx.closePath();
        ctx.stroke();

        // Remendo jeans azul no peito
        ctx.beginPath();
        ctx.roundRect(7.0f * sc, -62.0f * sc, 16.0f * sc, 15.0f * sc, 2.0f * sc);
        ctx.fillStyle = sf::Color(52, 98, 162);
        ctx.fill();
        ctx.strokeStyle = kInk;
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
        ctx.strokeStyle = kInk;
        ctx.lineWidth = 1.8f * sc;
        ctx.stroke();

        // Costura central do saco de estopa
        ctx.strokeStyle = kInk;
        ctx.lineWidth = 2.2f * sc;
        ctx.beginPath();
        ctx.moveTo(-1.0f * sc, -78.0f * sc);
        ctx.lineTo(1.0f * sc, -26.0f * sc);
        ctx.stroke();
        for (float sy = -74.0f; sy <= -28.0f; sy += 7.0f) {
            ctx.beginPath();
            ctx.moveTo(-4.5f * sc, (sy - 1.0f) * sc);
            ctx.lineTo(4.5f * sc, (sy + 1.0f) * sc);
            ctx.stroke();
        }

        // Cinturão de corda com nó e pontas esfiapadas
        ctx.beginPath();
        ctx.roundRect(-26.0f * sc, -25.0f * sc, 52.0f * sc, 6.0f * sc, 3.0f * sc);
        ctx.fillStyle = sf::Color(218, 186, 122);
        ctx.fill();
        ctx.strokeStyle = kInk;
        ctx.lineWidth = 2.2f * sc;
        ctx.stroke();
        ctx.beginPath();
        ctx.arc(0.0f, -22.0f * sc, 4.5f * sc, 0.0f, 2.0f * kPi);
        ctx.fillStyle = sf::Color(218, 186, 122);
        ctx.fill();
        ctx.stroke();
        ctx.beginPath();
        ctx.moveTo(-2.0f * sc, -20.0f * sc);
        ctx.quadraticCurveTo(-6.0f * sc, -15.0f * sc, -5.0f * sc, -9.0f * sc);
        ctx.moveTo(2.0f * sc, -20.0f * sc);
        ctx.quadraticCurveTo(5.0f * sc, -14.0f * sc, 3.0f * sc, -7.0f * sc);
        ctx.lineWidth = 2.4f * sc;
        ctx.strokeStyle = sf::Color(218, 186, 122);
        ctx.stroke();

        // Gola de palha ao redor do pescoço
        ctx.fillStyle = sf::Color(250, 212, 60);
        ctx.strokeStyle = kInk;
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

        // Corda amarrada no pescoço
        ctx.beginPath();
        ctx.roundRect(-12.0f * sc, -86.0f * sc, 24.0f * sc, 5.0f * sc, 2.5f * sc);
        ctx.fillStyle = sf::Color(218, 186, 122);
        ctx.fill();
        ctx.strokeStyle = kInk;
        ctx.lineWidth = 2.0f * sc;
        ctx.stroke();

        // Cabeça de saco de estopa
        const float headCY = -108.0f * sc;
        ctx.beginPath();
        ctx.ellipse(0.0f, headCY, 21.0f * sc, 23.0f * sc, 0.0f, 0.0f, 2.0f * kPi);
        ctx.fillStyle = sf::Color(218, 172, 98);
        ctx.fill();

        // Sombra lateral direita da cabeça
        ctx.fillStyle = sf::Color(162, 120, 56, 160);
        ctx.beginPath();
        ctx.arc(0.0f, headCY, 21.0f * sc, -0.45f * kPi, 0.45f * kPi);
        ctx.closePath();
        ctx.fill();

        ctx.beginPath();
        ctx.ellipse(0.0f, headCY, 21.0f * sc, 23.0f * sc, 0.0f, 0.0f, 2.0f * kPi);
        ctx.strokeStyle = flash ? sf::Color::White : kInk;
        ctx.lineWidth = 3.2f * sc;
        ctx.stroke();

        // Franzido / nó no topo da cabeça de estopa (visível se o chapéu se mover)
        ctx.beginPath();
        ctx.ellipse(0.0f, headCY - 22.0f * sc, 5.0f * sc, 3.5f * sc, 0.0f, 0.0f, 2.0f * kPi);
        ctx.fillStyle = sf::Color(190, 148, 76);
        ctx.fill();
        ctx.strokeStyle = kInk;
        ctx.lineWidth = 1.8f * sc;
        ctx.stroke();

        // Bochechas coradas
        ctx.fillStyle = "rgba(235, 95, 95, 0.45)";
        ctx.beginPath();
        ctx.ellipse(-12.0f * sc, headCY + 5.0f * sc, 4.5f * sc, 3.0f * sc, 0.0f, 0.0f, 2.0f * kPi);
        ctx.fill();
        ctx.beginPath();
        ctx.ellipse(12.0f * sc, headCY + 5.0f * sc, 4.5f * sc, 3.0f * sc, 0.0f, 0.0f, 2.0f * kPi);
        ctx.fill();

        if (!backView) {
            // Rosto de Frente: Olhos, Nariz e Boca
            if (hurtEyes) {
                // Olhos em X quando atingido
                ctx.strokeStyle = kInk;
                ctx.lineWidth = 3.0f * sc;
                ctx.beginPath();
                ctx.moveTo(-13.0f * sc, headCY - 4.0f * sc); ctx.lineTo(-5.0f * sc, headCY + 4.0f * sc);
                ctx.moveTo(-5.0f * sc, headCY - 4.0f * sc); ctx.lineTo(-13.0f * sc, headCY + 4.0f * sc);
                ctx.moveTo(5.0f * sc, headCY - 4.0f * sc); ctx.lineTo(13.0f * sc, headCY + 4.0f * sc);
                ctx.moveTo(13.0f * sc, headCY - 4.0f * sc); ctx.lineTo(5.0f * sc, headCY + 4.0f * sc);
                ctx.stroke();
            } else {
                // Olho esquerdo: botão de madeira grande com X de linha amarela
                ctx.beginPath();
                ctx.arc(-9.0f * sc, headCY, 5.5f * sc, 0.0f, 2.0f * kPi);
                ctx.fillStyle = sf::Color(45, 30, 20);
                ctx.fill();
                ctx.strokeStyle = kInk;
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
                ctx.arc(8.5f * sc, headCY - 0.5f * sc, 4.2f * sc, 0.0f, 2.0f * kPi);
                ctx.fillStyle = sf::Color(20, 22, 25);
                ctx.fill();
                ctx.strokeStyle = kInk;
                ctx.lineWidth = 1.8f * sc;
                ctx.stroke();
                ctx.beginPath();
                ctx.arc(7.2f * sc, headCY - 1.8f * sc, 1.2f * sc, 0.0f, 2.0f * kPi);
                ctx.fillStyle = sf::Color(255, 255, 255);
                ctx.fill();
            }

            // Nariz triangular de remendo
            ctx.beginPath();
            ctx.moveTo(0.0f, headCY + 1.0f * sc);
            ctx.lineTo(-3.5f * sc, headCY + 6.0f * sc);
            ctx.lineTo(3.5f * sc, headCY + 6.0f * sc);
            ctx.closePath();
            ctx.fillStyle = sf::Color(190, 75, 55);
            ctx.fill();
            ctx.strokeStyle = kInk;
            ctx.lineWidth = 1.5f * sc;
            ctx.stroke();

            // Boca costurada / boca aberta no choque
            if (shockMouth) {
                ctx.beginPath();
                ctx.ellipse(0.0f, headCY + 14.0f * sc, 6.0f * sc, 7.0f * sc, 0.0f, 0.0f, 2.0f * kPi);
                ctx.fillStyle = sf::Color(55, 20, 20);
                ctx.fill();
                ctx.strokeStyle = kInk;
                ctx.lineWidth = 2.4f * sc;
                ctx.stroke();
                ctx.strokeStyle = sf::Color(250, 212, 60);
                ctx.lineWidth = 2.0f * sc;
                ctx.beginPath();
                ctx.moveTo(1.0f * sc, headCY + 14.0f * sc);
                ctx.lineTo(7.0f * sc, headCY + 18.0f * sc);
                ctx.stroke();
            } else {
                ctx.strokeStyle = kInk;
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
        } else {
            // Costura traseira da cabeça
            ctx.strokeStyle = kInk;
            ctx.lineWidth = 2.0f * sc;
            ctx.beginPath();
            ctx.moveTo(0.0f, headCY - 16.0f * sc);
            ctx.lineTo(0.0f, headCY + 16.0f * sc);
            ctx.stroke();
            for (float sy = headCY - 12.0f * sc; sy <= headCY + 12.0f * sc; sy += 6.0f * sc) {
                ctx.beginPath();
                ctx.moveTo(-3.0f * sc, sy); ctx.lineTo(3.0f * sc, sy);
                ctx.stroke();
            }
        }

        // O Chapéu de Palha com fita vermelha (inclina no gancho e salta no ar no uppercut)
        const float hatBaseY = headCY - 18.0f * sc;
        ctx.save();
        ctx.translate(0.0f, hatBaseY - hatPop * sc);
        if (std::fabs(hatTilt) > 0.001f) {
            ctx.rotate(hatTilt);
        }
        ctx.translate(0.0f, -hatBaseY);

        // Cabelo de palha espetada sob a aba do chapéu (pertence ao conjunto do chapéu)
        ctx.fillStyle = sf::Color(250, 212, 60);
        ctx.strokeStyle = kInk;
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
        ctx.ellipse(0.0f, hatBaseY, 32.0f * sc, 8.5f * sc, 0.0f, 0.0f, 2.0f * kPi);
        ctx.fillStyle = sf::Color(228, 188, 82);
        ctx.fill();
        ctx.strokeStyle = flash ? sf::Color::White : kInk;
        ctx.lineWidth = 3.0f * sc;
        ctx.stroke();

        // Rasgo na borda da aba
        ctx.beginPath();
        ctx.moveTo(-22.0f * sc, hatBaseY + 2.0f * sc);
        ctx.lineTo(-19.0f * sc, hatBaseY - 2.0f * sc);
        ctx.lineTo(-16.0f * sc, hatBaseY + 2.0f * sc);
        ctx.fillStyle = sf::Color(168, 128, 45);
        ctx.fill();
        ctx.strokeStyle = kInk;
        ctx.lineWidth = 1.5f * sc;
        ctx.stroke();

        // Copa do chapéu
        ctx.beginPath();
        ctx.moveTo(-16.0f * sc, hatBaseY - 2.0f * sc);
        ctx.lineTo(-11.0f * sc, hatBaseY - 24.0f * sc);
        ctx.bezierCurveTo(-5.0f * sc, hatBaseY - 27.0f * sc, 5.0f * sc, hatBaseY - 27.0f * sc, 11.0f * sc, hatBaseY - 24.0f * sc);
        ctx.lineTo(16.0f * sc, hatBaseY - 2.0f * sc);
        ctx.closePath();
        ctx.fillStyle = sf::Color(228, 188, 82);
        ctx.fill();

        // Sombra lateral da copa
        ctx.fillStyle = sf::Color(168, 128, 45, 150);
        ctx.beginPath();
        ctx.moveTo(3.0f * sc, hatBaseY - 26.5f * sc);
        ctx.bezierCurveTo(5.0f * sc, hatBaseY - 27.0f * sc, 8.0f * sc, hatBaseY - 26.0f * sc, 11.0f * sc, hatBaseY - 24.0f * sc);
        ctx.lineTo(16.0f * sc, hatBaseY - 2.0f * sc);
        ctx.lineTo(3.0f * sc, hatBaseY - 2.0f * sc);
        ctx.closePath();
        ctx.fill();

        ctx.beginPath();
        ctx.moveTo(-16.0f * sc, hatBaseY - 2.0f * sc);
        ctx.lineTo(-11.0f * sc, hatBaseY - 24.0f * sc);
        ctx.bezierCurveTo(-5.0f * sc, hatBaseY - 27.0f * sc, 5.0f * sc, hatBaseY - 27.0f * sc, 11.0f * sc, hatBaseY - 24.0f * sc);
        ctx.lineTo(16.0f * sc, hatBaseY - 2.0f * sc);
        ctx.closePath();
        ctx.strokeStyle = flash ? sf::Color::White : kInk;
        ctx.lineWidth = 3.0f * sc;
        ctx.stroke();

        // Fita vermelha na copa
        ctx.beginPath();
        ctx.moveTo(-16.0f * sc, hatBaseY - 2.0f * sc);
        ctx.lineTo(-14.5f * sc, hatBaseY - 8.0f * sc);
        ctx.bezierCurveTo(-5.0f * sc, hatBaseY - 9.5f * sc, 5.0f * sc, hatBaseY - 9.5f * sc, 14.5f * sc, hatBaseY - 8.0f * sc);
        ctx.lineTo(16.0f * sc, hatBaseY - 2.0f * sc);
        ctx.bezierCurveTo(6.0f * sc, hatBaseY - 3.5f * sc, -6.0f * sc, hatBaseY - 3.5f * sc, -16.0f * sc, hatBaseY - 2.0f * sc);
        ctx.closePath();
        ctx.fillStyle = sf::Color(225, 35, 50);
        ctx.fill();
        ctx.strokeStyle = kInk;
        ctx.lineWidth = 2.0f * sc;
        ctx.stroke();

        // Palha saindo por um furinho na copa
        ctx.strokeStyle = sf::Color(250, 212, 60);
        ctx.lineWidth = 2.0f * sc;
        ctx.beginPath();
        ctx.moveTo(3.0f * sc, hatBaseY - 24.0f * sc);
        ctx.lineTo(7.0f * sc, hatBaseY - 30.0f * sc);
        ctx.stroke();

        ctx.restore(); // Restaura posição do chapéu

        // Efeito de palha voando no impacto do golpe
        if (f.state == FIGHTER_HIT) {
            const float p = 1.0f - static_cast<float>(f.timer) / static_cast<float>(f.maxTimer);
            ctx.strokeStyle = sf::Color(254, 238, 120);
            ctx.lineWidth = 2.2f * sc;
            static const float kStrawAngles[] = { -0.8f, -0.3f, 0.4f, 1.1f, -1.4f };
            for (int i = 0; i < 5; ++i) {
                float a = kStrawAngles[i] + (i * 0.2f);
                float dist = (12.0f + i * 6.0f) * sc + p * 32.0f * sc;
                float len = 10.0f * sc;
                float sx0 = std::cos(a) * dist;
                float sy0 = headCY + 10.0f * sc + std::sin(a) * dist;
                ctx.beginPath();
                ctx.moveTo(sx0, sy0);
                ctx.lineTo(sx0 + std::cos(a) * len, sy0 + std::sin(a) * len);
                ctx.stroke();
            }
        }

        ctx.restore(); // Restaura corpo articulado
    }});
}

void RingRenderer::queueParticles(const GameRenderState& state) {
    uint32_t count = std::min<uint32_t>(state.particle_count, MAX_PARTICLES);
    for (uint32_t i = 0; i < count; ++i) {
        const ParticleRenderData& p = state.particles[i];
        auto sp = camera_.project({p.x, p.y, p.z});
        if (!sp) continue;
        // Faísca em losango com contorno preto, como as partículas de impacto do JS.
        float size = std::max(3.0f, p.size * sp->scale * p.life);
        sf::Vector2f c = sp->pos;
        sf::Color color = withAlpha(colorFromRgba(p.color_hex), std::min(1.0f, p.life * 1.2f));
        queue_.push_back({kLayerParticles, sp->depth, [c, size, color](sf::RenderTarget& t) {
                              auto diamond = [&](float grow, sf::Color fill) {
                                  sf::ConvexShape d(4);
                                  d.setPoint(0, {c.x, c.y - size * 1.5f - grow});
                                  d.setPoint(1, {c.x + size * 0.45f + grow * 0.8f, c.y});
                                  d.setPoint(2, {c.x, c.y + size * 1.5f + grow});
                                  d.setPoint(3, {c.x - size * 0.45f - grow * 0.8f, c.y});
                                  d.setFillColor(fill);
                                  t.draw(d);
                              };
                              diamond(1.5f, withAlpha(kInk, color.a / 255.0f));
                              diamond(0.0f, color);
                          }});
    }
}

void RingRenderer::drawFloatingTexts(sf::RenderTarget& target, const GameRenderState& state) {
    uint32_t count = std::min<uint32_t>(state.floating_text_count, MAX_FLOATING_TEXTS);
    for (uint32_t i = 0; i < count; ++i) {
        const FloatingTextData& t = state.floating_texts[i];
        auto sp = camera_.project({t.x, t.y, t.z});
        if (!sp) continue;

        const bool isNumber = (t.text[0] == '-' || t.text[0] == '+');
        const float dir = (t.side == 1 ? -1.0f : 1.0f);
        const float elapsed = 1.0f - t.life;
        const sf::Font* comicFont = fonts_.display ? fonts_.display : fonts_.digits;

        if (isNumber) {
            // Número de dano/cura com pop dinâmico sutil no impacto, compacto para não cobrir o combate
            const float pop = 1.0f + 0.20f * std::exp(-elapsed * 9.0f);
            const float offsetX = dir * (10.0f + elapsed * 8.0f) * sp->scale;
            const float offsetY = -14.0f * sp->scale - elapsed * 18.0f * sp->scale;
            const sf::Vector2f pos = {sp->pos.x + offsetX, sp->pos.y + offsetY};

            unsigned sz = static_cast<unsigned>((t.is_crit ? 21 : 16) * sp->scale * pop);
            sz = std::clamp(sz, 12u, 26u);

            if (comicFont) {
                sf::Text text(*comicFont, sf::String::fromUtf8(t.text, t.text + std::strlen(t.text)), sz);
                sf::FloatRect bounds = text.getLocalBounds();
                const sf::Vector2f origin = {bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f};
                text.setOrigin(origin);

                // Sombra sutil de tinta
                const float shadowOff = std::max(1.5f, 2.0f * sp->scale);
                text.setPosition({pos.x + shadowOff, pos.y + shadowOff});
                text.setFillColor(withAlpha(sf::Color(10, 8, 18), t.life));
                text.setOutlineColor(withAlpha(sf::Color(10, 8, 18), t.life));
                text.setOutlineThickness(std::max(1.2f, sz / 10.0f));
                target.draw(text);

                // Texto principal com preenchimento vibrante e contorno fino
                sf::Color fillCol = colorFromRgba(t.color_hex);
                if (t.is_crit) fillCol = sf::Color(255, 230, 0); // Ouro elétrico em acerto crítico
                else if (t.text[0] == '+') fillCol = sf::Color(76, 217, 100); // Verde vibrante em cura

                text.setPosition(pos);
                text.setFillColor(withAlpha(fillCol, t.life));
                text.setOutlineColor(withAlpha(kInk, t.life));
                text.setOutlineThickness(std::max(1.2f, sz / 10.0f));
                target.draw(text);
            }
        } else {
            // Balão de ação compacto (PUNISH!, COUNTER!, PARRY!, BRECHA!, etc.)
            const float offsetX = -dir * (30.0f + elapsed * 6.0f) * sp->scale;
            const float offsetY = -8.0f * sp->scale - elapsed * 10.0f * sp->scale;
            const sf::Vector2f pos = {sp->pos.x + offsetX, sp->pos.y + offsetY};

            if (comicFont) {
                unsigned sz = static_cast<unsigned>((t.is_crit ? 13 : 11) * sp->scale);
                sz = std::clamp(sz, 10u, 16u);
                sf::Text text(*comicFont, sf::String::fromUtf8(t.text, t.text + std::strlen(t.text)), sz);
                sf::FloatRect bounds = text.getLocalBounds();

                // Fundo em cápsula escura sutil e compacta
                const float padX = 5.0f, padY = 2.0f;
                const sf::Vector2f boxSize = {bounds.size.x + padX * 2.0f, bounds.size.y + padY * 2.0f};
                sf::RectangleShape balloon(boxSize);
                balloon.setOrigin({boxSize.x * 0.5f, boxSize.y * 0.5f});
                balloon.setPosition({pos.x, pos.y + bounds.size.y * 0.5f});
                balloon.setFillColor(sf::Color(14, 10, 22, static_cast<uint8_t>(200 * t.life)));
                balloon.setOutlineColor(withAlpha(colorFromRgba(t.color_hex), t.life));
                balloon.setOutlineThickness(1.0f);
                target.draw(balloon);

                // Texto da ação dentro do balão
                text.setFillColor(withAlpha(sf::Color::White, t.life));
                text.setOutlineColor(withAlpha(kInk, t.life));
                text.setOutlineThickness(1.0f);
                text.setPosition({std::round(pos.x - (bounds.position.x + bounds.size.x) * 0.5f),
                                  std::round(pos.y - bounds.position.y * 0.5f)});
                target.draw(text);
            }
        }
    }
}

void RingRenderer::drawHud(sf::RenderTarget& target, const GameEngine& engine, bool paused, bool /*showResult*/) {
    // O HUD inteiro (barras, relógio, quedas, habilidades, combo, contagem e pausa) vive no
    // espaço 1600x900 e segue o css/hud.css do jogo original.
    drawComicHud(target, engine, paused);
}

void RingRenderer::drawKnockdownDots(sf::RenderTarget& target, sf::Vector2f pos, int count, bool alignRight) {
    drawText(target, "QUEDAS", {alignRight ? pos.x - 62.0f : pos.x, pos.y - 2.0f}, 13u, sf::Color(0x8c, 0x82, 0xa0),
             alignRight ? 1.0f : 0.0f);
    for (int i = 0; i < GameEngine::kKnockdownsForTko; ++i) {
        float x = alignRight ? pos.x - 8.0f - i * 18.0f : pos.x + 60.0f + i * 18.0f;
        sf::CircleShape dot(6.0f);
        dot.setOrigin({6.0f, 6.0f});
        dot.setPosition({x, pos.y + 8.0f});
        dot.setFillColor(i < count ? sf::Color(0xff, 0x1e, 0x42) : sf::Color(0x14, 0x0d, 0x22));
        dot.setOutlineColor(kInk);
        dot.setOutlineThickness(2.0f);
        target.draw(dot);
    }
}

void RingRenderer::drawBar(sf::RenderTarget& target, sf::Vector2f pos, sf::Vector2f size, float ratio, sf::Color fill,
                           bool alignRight) {
    ratio = std::clamp(ratio, 0.0f, 1.0f);
    sf::RectangleShape back(size);
    back.setPosition(pos);
    back.setFillColor(sf::Color(0x14, 0x0d, 0x22));
    back.setOutlineColor(kInk);
    back.setOutlineThickness(3.0f);
    target.draw(back);

    sf::RectangleShape bar({size.x * ratio, size.y});
    bar.setPosition(alignRight ? sf::Vector2f{pos.x + size.x * (1.0f - ratio), pos.y} : pos);
    bar.setFillColor(fill);
    target.draw(bar);
}

void RingRenderer::drawText(sf::RenderTarget& target, const std::string& utf8, sf::Vector2f pos, unsigned size,
                            sf::Color color, float anchorX, const sf::Font* font) {
    if (!font) font = fonts_.display;
    if (!font) font = fonts_.digits;
    if (!font) return;
    sf::Text text(*font, sf::String::fromUtf8(utf8.begin(), utf8.end()), size);
    text.setFillColor(color);
    text.setOutlineColor(withAlpha(kInk, color.a / 255.0f));
    text.setOutlineThickness(std::max(2.0f, size / 10.0f));
    sf::FloatRect bounds = text.getLocalBounds();
    text.setPosition({std::round(pos.x - (bounds.position.x + bounds.size.x) * anchorX), std::round(pos.y)});
    target.draw(text);
}

// ---------------------------------------------------------------------------
// Efeitos das habilidades e painel de habilidades
// ---------------------------------------------------------------------------

namespace {

// Pequenos círculos com contorno usados pelos efeitos.
void dot(sf::RenderTarget& tg, sf::Vector2f c, float r, sf::Color fill, sf::Color outline, float outlineWidth = 1.5f,
         std::size_t points = 30) {
    sf::CircleShape s(r, points);
    s.setOrigin({r, r});
    s.setPosition(c);
    s.setFillColor(fill);
    s.setOutlineColor(outline);
    s.setOutlineThickness(outlineWidth);
    tg.draw(s);
}

} // namespace

void RingRenderer::queueFighterStatus(const Fighter& f, uint32_t frame) {
    const float bodyLayer = f.isPlayer ? 4.4f : 3.5f;
    const Vec3 fighterCenter = Physics3D::localToWorld(f.offsetX, 0.0f, f.offsetZ, f.worldPos, f.facing);
    const float fighterDepth = camera_.depthOf(fighterCenter);
    const float headY = (f.charId == 9 || f.isDummy()) ? 152.0f : (f.isBigChar() ? 128.0f : (f.isSmallChar() ? 92.0f : 115.0f));
    const float t = static_cast<float>(frame);
    auto layerFor = [&](float depth) { return bodyLayer + (depth < fighterDepth ? 0.12f : -0.12f); };
    auto around = [&](float angle, float radius, float y) {
        return Vec3{f.worldPos.x + std::cos(angle) * radius, y, f.worldPos.z + std::sin(angle) * radius};
    };

    // Poça de lama no chão (100% ancorada na arena, sem seguir os passos do lutador)
    int fIdx = f.isPlayer ? 0 : 1;
    const Fighter* mudSource=engine_?(f.isPlayer?&engine_->enemy:&engine_->player):nullptr;
    const bool creamMud=mudSource&&mudSource->charId==3&&hasIntegratedSkin(3);
    if (f.status.mud > 0 && (creamMud || !(engine_ && (engine_->player.charId == 1 || engine_->enemy.charId == 1)))) {
        if (!mudPuddleActive_[fIdx]) {
            mudPuddleActive_[fIdx] = true;
            mudPuddleFloorPos_[fIdx] = {f.worldPos.x, 0.0f, f.worldPos.z};
        }
        const Vec3 floorPos = mudPuddleFloorPos_[fIdx];
        if(creamMud){
            // Preserve the real slow/snare location and lifetime. The old
            // generic brown mud marker is not the caster's merged silhouette.
            static const sf::Color colors[]={hex(0xfef08a),hex(0xa2c65c),hex(0xf3bbca),hex(0xc5e9ef)};
            sf::Color fill=colors[std::min(3u,mudSource->skinId)];
            if(mudSource->loadout.has(SkillId::CremosoFrozenStamina))fill=hex(0xe3f7ff);
            else if(mudSource->loadout.has(SkillId::CremosoSweetRevenge))fill=hex(0xbe123c);
            fill.a=static_cast<uint8_t>(220*std::clamp(f.status.mud/20.f,0.f,1.f));
            const auto center=camera_.project({floorPos.x,1,floorPos.z});
            if(center){
                std::vector<sf::Vertex> mesh;const float phase=std::floor(t/12.f)*.16f;
                for(int i=0;i<40;++i){
                    auto edge=[&](int j){const float a=j*2*kPi/40.f,r=75.f*(1+.035f*std::sin(a*5+phase));return camera_.project({floorPos.x+std::cos(a)*r,1,floorPos.z+std::sin(a)*r*.48f});};
                    const auto a=edge(i),b=edge(i+1);if(a&&b){mesh.push_back({center->pos,fill});mesh.push_back({a->pos,fill});mesh.push_back({b->pos,fill});}
                }
                queue_.push_back({1.5f,center->depth,[mesh=std::move(mesh)](sf::RenderTarget& tg){tg.draw(mesh.data(),mesh.size(),sf::PrimitiveType::Triangles);}});
            }
        } else if (auto p = camera_.project({floorPos.x, 1.0f, floorPos.z})) {
            float rx = 75.0f * p->scale, ry = 25.0f * p->scale;
            sf::Vector2f c = p->pos;
            queue_.push_back({1.5f, p->depth, [c, rx, ry](sf::RenderTarget& tg) {
                                  // 1. Respingo escuro de base com borda cel-shading de nanquim
                                  sf::CircleShape base(rx * 1.08f, 28);
                                  base.setScale({1.0f, ry / rx});
                                  base.setOrigin({rx * 1.08f, rx * 1.08f});
                                  base.setPosition(c);
                                  base.setFillColor(sf::Color(0x1a, 0x0c, 0x04));
                                  base.setOutlineColor(sf::Color::Black);
                                  base.setOutlineThickness(2.5f);
                                  tg.draw(base);

                                  // 2. Corpo viscoso de lama rica em tom chocolate escuro
                                  sf::CircleShape mud(rx, 24);
                                  mud.setScale({1.0f, ry / rx});
                                  mud.setOrigin({rx, rx});
                                  mud.setPosition(c);
                                  mud.setFillColor(sf::Color(0x5c, 0x34, 0x18));
                                  tg.draw(mud);

                                  // 3. Camada intermediária mais clara (cel-shading cartoon)
                                  sf::CircleShape inner(rx * 0.72f, 20);
                                  inner.setScale({1.0f, ry / rx * 0.95f});
                                  inner.setOrigin({rx * 0.72f, rx * 0.72f});
                                  inner.setPosition(c + sf::Vector2f{0.0f, -ry * 0.08f});
                                  inner.setFillColor(sf::Color(0x78, 0x48, 0x22));
                                  tg.draw(inner);

                                  // 4. Brilho especular cartoon líquido
                                  sf::CircleShape spec(rx * 0.42f, 16);
                                  spec.setScale({1.0f, (ry * 0.35f) / (rx * 0.42f)});
                                  spec.setOrigin({rx * 0.42f, rx * 0.42f});
                                  spec.setPosition(c + sf::Vector2f{-rx * 0.22f, -ry * 0.25f});
                                  spec.setFillColor(sf::Color(255, 255, 255, 140));
                                  tg.draw(spec);

                                  // 5. Gotas de respingo cartoon em volta da poça
                                  for (int d = 0; d < 4; ++d) {
                                      float da = d * 1.57f + 0.3f;
                                      sf::Vector2f dp = c + sf::Vector2f{std::cos(da) * rx * 1.15f, std::sin(da) * ry * 1.15f};
                                      dot(tg, dp, std::max(1.8f, rx * 0.08f), sf::Color(0x5c, 0x34, 0x18), sf::Color::Black, 1.2f);
                                  }
                              }});
        }
    } else {
        mudPuddleActive_[fIdx] = false;
    }

    // Enxame de abelhas 3D estilo cel-shaded orbitando a cabeça e o tronco
    if (f.status.infested > 0) {
        Reaction rx = computeReaction(f, f.isPlayer ? 0 : 1, 0.0f, false, f.isPlayer);
        const float ox = f.offsetX + rx.dx;
        const float oy = f.offsetY + rx.dy;
        const float oz = f.offsetZ + rx.dz;
        const Vec3 orbitCenter = Physics3D::localToWorld(ox, headY + oy, oz, f.worldPos, f.facing);
        const float orbitCenterDepth = camera_.depthOf(orbitCenter);

        constexpr int kBeeCount = 10;
        for (int i = 0; i < kBeeCount; ++i) {
            float a = t * 0.082f + static_cast<float>(i) * (6.283185f / static_cast<float>(kBeeCount));
            float radX = 32.0f + static_cast<float>(i % 3) * 3.5f;
            float radZ = 30.0f + static_cast<float>((i + 1) % 3) * 3.5f;
            float beeY = headY + oy + (static_cast<float>(i % 4) - 1.5f) * 7.5f + std::sin(a * 2.0f + static_cast<float>(i) * 1.4f) * 6.0f;

            float lx = ox + std::cos(a) * radX;
            float lz = oz + std::sin(a) * radZ;
            float ly = beeY;
            Vec3 wPos = Physics3D::localToWorld(lx, ly, lz, f.worldPos, f.facing);

            float dlx = -std::sin(a) * radX;
            float dlz =  std::cos(a) * radZ;
            float dly =  std::cos(a * 2.0f + static_cast<float>(i) * 1.4f) * 12.0f;
            float cosF = std::cos(f.facing), sinF = std::sin(f.facing);
            Vec3 wDir = { dlx * cosF + dlz * sinF, dly, -dlx * sinF + dlz * cosF };

            float rollAngle = -0.32f;
            float beeDepth = camera_.depthOf(wPos);
            float flapPhase = t * 0.65f + static_cast<float>(i) * 1.7f;
            float beeSize = 6.2f + static_cast<float>(i % 3) * 0.8f; // Mais compacta, vibrante e cel-shaded

            // Oclusão 3D e passagem frente/trás:
            // orbitCenterDepth é medido no centro da cabeça do lutador.
            // Quando beeDepth > orbitCenterDepth: a abelha está na metade de trás da órbita.
            // Camada: bodyLayer - 0.20f (3.30f para oponente). O corpo do oponente (3.50f)
            // é desenhado POR CIMA, ocultando a abelha quando ela passa atrás dele!
            // Quando beeDepth <= orbitCenterDepth: a abelha está na metade da frente da órbita.
            // Camada: bodyLayer + 0.20f (3.70f para oponente). A abelha é desenhada NA FRENTE do rosto e do peito.
            float beeLayer = bodyLayer + (beeDepth < orbitCenterDepth ? 0.20f : -0.20f);

            const Fighter* source = engine_ ? (f.isPlayer ? &engine_->enemy : &engine_->player) : nullptr;
            const uint32_t beeSkin = source && source->charId == 0 ? source->skinId : 0;
            queue_.push_back({beeLayer, beeDepth, [this, wPos, wDir, beeSize, flapPhase, rollAngle, beeSkin](sf::RenderTarget& tg) {
                vfx::drawPsxBee(tg, camera_, wPos, wDir, beeSize, flapPhase, rollAngle, 1, false, beeSkin);
            }});
        }
    }

    // Bolhas de veneno cartoon com brilho especular e rastro tóxico
    if (f.status.poisoned > 0) {
        for (int i = 0; i < 7; ++i) {
            float rise = std::fmod(t * 1.4f + static_cast<float>(i) * 17.0f, 120.0f);
            float a = static_cast<float>(i) * 0.9f + t * 0.02f;
            auto p = camera_.project(around(a, 34.0f, 30.0f + rise));
            if (!p) continue;
            float r = std::max(2.5f, (5.0f + static_cast<float>(i % 3) * 1.5f) * p->scale);
            float alpha = 1.0f - rise / 120.0f;
            sf::Vector2f c = p->pos;
            queue_.push_back({layerFor(p->depth), p->depth, [c, r, alpha](sf::RenderTarget& tg) {
                                  sf::CircleShape bubble(r, 16);
                                  bubble.setOrigin({r, r});
                                  bubble.setPosition(c);
                                  bubble.setFillColor(withAlpha(hex(0xa3e635), 0.75f * alpha));
                                  bubble.setOutlineColor(withAlpha(hex(0x14532d), 0.95f * alpha));
                                  bubble.setOutlineThickness(std::max(1.0f, 1.5f * (r / 6.0f)));
                                  tg.draw(bubble);

                                  // Brilho especular cartoon
                                  sf::CircleShape shine(r * 0.32f, 10);
                                  shine.setOrigin({r * 0.32f, r * 0.32f});
                                  shine.setPosition(c + sf::Vector2f{-r * 0.35f, -r * 0.35f});
                                  shine.setFillColor(withAlpha(sf::Color::White, 0.9f * alpha));
                                  tg.draw(shine);
                              }});
        }
    }

    // Queimadura (calda ou lava): labaredas cartoon ascendentes
    if (f.status.burn > 0) {
        for (int i = 0; i < 7; ++i) {
            float rise = std::fmod(t * 2.2f + static_cast<float>(i) * 14.0f, 75.0f);
            float a = static_cast<float>(i) * 1.05f + t * 0.035f;
            auto p = camera_.project(around(a, 28.0f, 40.0f + rise));
            if (!p) continue;
            float r = std::max(2.5f, (7.0f - rise / 18.0f) * p->scale);
            float alpha = 1.0f - rise / 75.0f;
            sf::Color c1 = f.charId == 3 ? hex(0xfb7185) : hex(0xf97316);
            sf::Vector2f c = p->pos;
            queue_.push_back({layerFor(p->depth) + 0.2f, p->depth, [c, r, alpha, c1](sf::RenderTarget& tg) {
                                  // Labareda cartoon: gota pontuda estilizada
                                  sf::ConvexShape flame(5);
                                  flame.setPoint(0, c + sf::Vector2f{0.0f, -r * 1.8f}); // ponta superior
                                  flame.setPoint(1, c + sf::Vector2f{r * 0.9f, -r * 0.4f});
                                  flame.setPoint(2, c + sf::Vector2f{r * 0.6f, r * 0.8f});
                                  flame.setPoint(3, c + sf::Vector2f{-r * 0.6f, r * 0.8f});
                                  flame.setPoint(4, c + sf::Vector2f{-r * 0.9f, -r * 0.4f});
                                  flame.setFillColor(withAlpha(c1, alpha));
                                  flame.setOutlineColor(withAlpha(hex(0x450a0a), alpha));
                                  flame.setOutlineThickness(1.2f);
                                  tg.draw(flame);

                                  // Núcleo brilhante amarelo
                                  sf::CircleShape core(r * 0.45f, 12);
                                  core.setOrigin({r * 0.45f, r * 0.45f});
                                  core.setPosition(c + sf::Vector2f{0.0f, r * 0.1f});
                                  core.setFillColor(withAlpha(hex(0xfef08a), alpha * 0.9f));
                                  tg.draw(core);
                              }});
        }
    }

    // Modos: aura de cada um em volta do corpo.
    struct Aura { int frames; sf::Color fill; sf::Color edge; std::size_t points; float radius; };
    const std::array<Aura, 5> auras = {{
        {f.status.heatwave, sf::Color(0xff, 0x6a, 0x1a, 70), hex(0xff4500), 30, 70.0f},
        {f.status.seasons, sf::Color(0x34, 0xd3, 0x99, 50), hex(0x9d4edd), 30, 64.0f},
        {f.status.thornVeil, sf::Color(0x10, 0xb9, 0x81, 55), hex(0x065f46), 12, 66.0f},
        {f.status.plasma, sf::Color(0x22, 0xd3, 0xee, 60), hex(0x0891b2), 40, 72.0f},
        {f.status.melt, sf::Color(0xfe, 0xf0, 0x8a, 60), hex(0xca8a04), 30, 68.0f},
    }};
    // Com módulo de efeitos (Cremoso em diante) as auras e as estações vêm do próprio módulo.
    const bool moduleAuras = f.charId >= 3 && f.charId <= 7 && lab_[f.isPlayer ? 0 : 1];
    for (const Aura& aura : auras) {
        if (aura.frames <= 0 || moduleAuras) continue;
        auto p = camera_.project(Physics3D::localToWorld(0.0f, 70.0f, 10.0f, f.worldPos, f.facing));
        if (!p) continue;
        float r = aura.radius * p->scale * (1.0f + 0.05f * std::sin(t * 0.2f));
        sf::Vector2f c = p->pos;
        Aura a = aura;
        queue_.push_back({bodyLayer + 0.18f, p->depth - 1.0f, [c, r, a](sf::RenderTarget& tg) {
                              dot(tg, c, r, a.fill, withAlpha(a.edge, 0.8f), 3.0f, a.points);
                          }});
    }
    // Quatro Estações: quatro orbes coloridos em órbita.
    if (f.status.seasons > 0 && !moduleAuras) {
        const std::array<uint32_t, 4> colors = {0x4ade80, 0xfacc15, 0xf97316, 0x60a5fa};
        for (int i = 0; i < 4; ++i) {
            float a = t * 0.08f + static_cast<float>(i) * 1.5708f;
            auto p = camera_.project(around(a, 52.0f, headY - 20.0f));
            if (!p) continue;
            float r = std::max(3.0f, 7.0f * p->scale);
            sf::Vector2f c = p->pos;
            sf::Color col = hex(colors[static_cast<std::size_t>(i)]);
            queue_.push_back({layerFor(p->depth), p->depth, [c, r, col](sf::RenderTarget& tg) { dot(tg, c, r, col, kInk, 2.0f); }});
        }
    }

    // Sangramento de coral: gotas vermelhas cartoon escorrendo/pingando
    if (f.status.bleed > 0) {
        for (int i = 0; i < 5; ++i) {
            float fall = std::fmod(t * 1.8f + static_cast<float>(i) * 23.0f, 90.0f);
            float a = static_cast<float>(i) * 1.3f;
            auto p = camera_.project(around(a, 22.0f, headY - 10.0f - fall));
            if (!p) continue;
            float r = std::max(2.5f, 4.5f * p->scale);
            sf::Vector2f c = p->pos;
            queue_.push_back({layerFor(p->depth) + 0.2f, p->depth, [c, r](sf::RenderTarget& tg) {
                                  // Gota pontuda cartoon com borda forte
                                  sf::ConvexShape drop(4);
                                  drop.setPoint(0, c + sf::Vector2f{0.0f, -r * 1.5f});
                                  drop.setPoint(1, c + sf::Vector2f{r, 0.0f});
                                  drop.setPoint(2, c + sf::Vector2f{0.0f, r * 1.2f});
                                  drop.setPoint(3, c + sf::Vector2f{-r, 0.0f});
                                  drop.setFillColor(hex(0xdc2626));
                                  drop.setOutlineColor(hex(0x450a0a));
                                  drop.setOutlineThickness(1.2f);
                                  tg.draw(drop);

                                  // Brilho especular
                                  sf::CircleShape shine(r * 0.28f, 8);
                                  shine.setPosition(c + sf::Vector2f{-r * 0.35f, -r * 0.4f});
                                  shine.setFillColor(sf::Color(255, 210, 210, 230));
                                  tg.draw(shine);
                              }});
        }
    }

    // Efeito de ofuscamento/cegueira cartoon (poeira/esporos/atordoamento cel-shaded, sem amarelo turvo)
    if (f.status.blind > 0) {
        for (int i = 0; i < 7; ++i) {
            float a = t * 0.05f + static_cast<float>(i) * 0.95f;
            auto p = camera_.project(around(a, 32.0f, headY - 5.0f + std::sin(t * 0.07f + static_cast<float>(i)) * 8.0f));
            if (!p) continue;
            float r = std::max(5.0f, 16.0f * p->scale);
            sf::Vector2f c = p->pos;
            queue_.push_back({layerFor(p->depth) + 0.2f, p->depth, [c, r, i, t](sf::RenderTarget& tg) {
                                  // Puff cartoon cel-shaded de poeira/fumaça (branco giz com sombreado ardósia e contorno ink)
                                  sf::CircleShape puff(r, 16);
                                  puff.setOrigin({r, r});
                                  puff.setPosition(c);
                                  puff.setFillColor(sf::Color(248, 250, 252, 205));
                                  puff.setOutlineColor(sf::Color(15, 23, 42, 220));
                                  puff.setOutlineThickness(1.5f);
                                  tg.draw(puff);

                                  // Sombreado interno inferior em meia-lua (cel-shade)
                                  sf::CircleShape puffShadow(r * 0.72f, 14);
                                  puffShadow.setOrigin({r * 0.72f, r * 0.72f});
                                  puffShadow.setPosition(c + sf::Vector2f{0.0f, r * 0.22f});
                                  puffShadow.setFillColor(sf::Color(203, 213, 225, 210));
                                  tg.draw(puffShadow);

                                  // Brilho especular superior
                                  sf::CircleShape shine(std::max(1.2f, r * 0.26f), 8);
                                  shine.setOrigin({r * 0.26f, r * 0.26f});
                                  shine.setPosition(c + sf::Vector2f{-r * 0.28f, -r * 0.30f});
                                  shine.setFillColor(sf::Color(255, 255, 255, 250));
                                  tg.draw(shine);

                                  // Estrela giratória de tontura/ofuscamento estilo quadrinho
                                  const float starRot = t * 0.12f + static_cast<float>(i) * 1.2f;
                                  const float sr = std::max(2.2f, r * 0.55f);
                                  const sf::Vector2f starPos = c + sf::Vector2f{std::cos(starRot) * r * 0.85f, std::sin(starRot) * r * 0.65f};
                                  const sf::Color starCol = (i % 2 == 0) ? sf::Color(0, 245, 212) : sf::Color(255, 255, 255);

                                  // Cruz/estrela de 4 pontas
                                  sf::RectangleShape armH({sr * 2.2f, sr * 0.55f});
                                  armH.setOrigin({sr * 1.1f, sr * 0.275f});
                                  armH.setPosition(starPos);
                                  armH.setRotation(sf::radians(starRot * 2.0f));
                                  armH.setFillColor(starCol);
                                  armH.setOutlineColor(sf::Color(15, 23, 42, 230));
                                  armH.setOutlineThickness(1.0f);
                                  tg.draw(armH);

                                  sf::RectangleShape armV({sr * 0.55f, sr * 2.2f});
                                  armV.setOrigin({sr * 0.275f, sr * 1.1f});
                                  armV.setPosition(starPos);
                                  armV.setRotation(sf::radians(starRot * 2.0f));
                                  armV.setFillColor(starCol);
                                  armV.setOutlineColor(sf::Color(15, 23, 42, 230));
                                  armV.setOutlineThickness(1.0f);
                                  tg.draw(armV);
                              }});
        }
    }

    // Escudo de favos cartoon: hexágonos dourados com borda cel-shaded e relevo interno
    // Favos e melaço do Jack vêm do módulo do Lab (lab_jack.cpp).
    if (f.status.honeyShield > 0 && f.charId != 0) {
        const int count = f.status.honeyShield * 2;
        for (int i = 0; i < count; ++i) {
            float a = t * 0.03f + static_cast<float>(i) * (6.2832f / static_cast<float>(count));
            Vec3 w = Physics3D::localToWorld(std::cos(a) * 42.0f, 70.0f + std::sin(a) * 34.0f, 26.0f, f.worldPos, f.facing);
            auto p = camera_.project(w);
            if (!p) continue;
            float r = std::max(6.0f, 16.0f * p->scale);
            sf::Vector2f c = p->pos;
            queue_.push_back({bodyLayer + 0.15f, p->depth, [c, r](sf::RenderTarget& tg) {
                                  // Hexágono exterior
                                  sf::CircleShape hexOuter(r, 6);
                                  hexOuter.setOrigin({r, r});
                                  hexOuter.setPosition(c);
                                  hexOuter.setFillColor(sf::Color(0xf5, 0x9e, 0x0b, 175));
                                  hexOuter.setOutlineColor(hex(0x78350f));
                                  hexOuter.setOutlineThickness(2.2f);
                                  tg.draw(hexOuter);

                                  // Hexágono interior iluminado
                                  sf::CircleShape hexInner(r * 0.65f, 6);
                                  hexInner.setOrigin({r * 0.65f, r * 0.65f});
                                  hexInner.setPosition(c);
                                  hexInner.setFillColor(sf::Color(0xfd, 0xba, 0x74, 215));
                                  tg.draw(hexInner);
                              }});
        }
    }

    // Melaço escorrendo das luvas.
    if (f.status.molasses > 0 && f.charId != 0) {
        for (const PartRenderData* g : {&f.gloveL, &f.gloveR}) {
            float drop = std::fmod(t * 1.2f + std::fabs(g->x), 40.0f);
            auto p = camera_.project(Physics3D::localToWorld(g->x + f.offsetX, g->y + f.offsetY - drop, g->z + f.offsetZ,
                                                             f.worldPos, f.facing));
            if (!p) continue;
            float r = std::max(2.0f, 5.0f * p->scale);
            sf::Vector2f c = p->pos;
            queue_.push_back({bodyLayer + 0.3f, p->depth, [c, r](sf::RenderTarget& tg) { dot(tg, c, r, hex(0xd97706), hex(0x451a03)); }});
        }
    }
}

void RingRenderer::queueSkillEffects(const GameEngine& engine) {
    queueFighterStatus(engine.player, engine.frameIndex);
    queueFighterStatus(engine.enemy, engine.frameIndex);
    queueBossEffects(engine);

    for (const Fighter* caster : {&engine.player, &engine.enemy}) {
        if (caster->state != FIGHTER_SKILL) continue;
        // Joana em diante: os golpes vêm inteiros dos módulos de efeitos (app/vfx).
        if (caster->charId >= 2 && caster->charId <= 7 && lab_[caster->isPlayer ? 0 : 1]) continue;
        const Fighter& target = caster == &engine.player ? engine.enemy : engine.player;
        const int elapsed = caster->maxTimer - caster->timer;
        const float p = 1.0f - static_cast<float>(caster->timer) / static_cast<float>(std::max(1, caster->maxTimer));
        const float layer = caster->isPlayer ? 4.5f : 3.65f;
        const float headY = caster->isBigChar() ? 105.0f : (caster->isSmallChar() ? 75.0f : 90.0f);
        const Vec3 mouth = Physics3D::localToWorld(0.0f, headY + caster->offsetY, 25.0f, caster->worldPos, caster->facing);

        auto tongue = [&](Vec3 to, float extend) {
            if (extend <= 0.01f) return;
            Vec3 tip{mouth.x + (to.x - mouth.x) * extend, mouth.y + (to.y - mouth.y) * extend, mouth.z + (to.z - mouth.z) * extend};
            auto a = camera_.project(mouth), b = camera_.project(tip);
            if (!a || !b) return;
            float thick = std::max(4.0f, 16.0f * (a->scale + b->scale) * 0.5f);
            sf::Vector2f pa = a->pos, pb = b->pos;
            queue_.push_back({layer, b->depth, [pa, pb, thick](sf::RenderTarget& tg) {
                                  drawThickLine(tg, pa, pb, thick + 4.0f, kInk);
                                  drawThickLine(tg, pa, pb, thick, hex(0xf472b6));
                                  dot(tg, pb, thick * 0.8f, hex(0xec4899), kInk, 2.0f);
                              }});
        };

        switch (caster->activeSkill) {
        case SkillId::FrogTongue: {
            if (lab_[caster->isPlayer ? 0 : 1]) break;
            float extend = elapsed <= 10 ? static_cast<float>(elapsed) / 10.0f : std::max(0.0f, 1.0f - static_cast<float>(elapsed - 10) / 14.0f);
            tongue({target.worldPos.x, target.worldPos.y + 80.0f, target.worldPos.z}, extend);
            break;
        }
        case SkillId::FrogFlyCatcher:
            if (lab_[caster->isPlayer ? 0 : 1]) break;
            tongue({mouth.x, mouth.y + 130.0f, mouth.z}, std::sin(p * kPi));
            break;
        case SkillId::FrogMudSlime:
            if (lab_[caster->isPlayer ? 0 : 1]) break;
            if (elapsed < 14) {
                float u = static_cast<float>(elapsed) / 14.0f;
                Vec3 to{target.worldPos.x, 10.0f, target.worldPos.z};
                Vec3 ball{mouth.x + (to.x - mouth.x) * u, mouth.y + (to.y - mouth.y) * u + std::sin(u * kPi) * 40.0f,
                          mouth.z + (to.z - mouth.z) * u};
                if (auto b = camera_.project(ball)) {
                    float r = std::max(4.0f, 14.0f * b->scale);
                    sf::Vector2f c = b->pos;
                    queue_.push_back({layer, b->depth, [c, r](sf::RenderTarget& tg) { dot(tg, c, r, hex(0x7a5230), kInk, 2.0f); }});
                }
            }
            break;
        case SkillId::FrogSwampSpout:
            if (lab_[caster->isPlayer ? 0 : 1]) break;
            if (p >= 0.45f && !caster->signatureOnly) {
                float grow = std::min(1.0f, (p - 0.45f) / 0.2f) * (1.0f - std::max(0.0f, p - 0.8f) / 0.2f);
                auto base = camera_.project({target.worldPos.x, 0.0f, target.worldPos.z});
                auto top = camera_.project({target.worldPos.x, 220.0f * grow, target.worldPos.z});
                if (base && top && grow > 0.02f) {
                    float w = 34.0f * base->scale;
                    sf::Vector2f b0 = base->pos, t0 = top->pos;
                    float wobble = std::sin(static_cast<float>(engine.frameIndex) * 0.6f) * 4.0f;
                    queue_.push_back({target.isPlayer ? 4.6f : 3.75f, base->depth, [b0, t0, w, wobble](sf::RenderTarget& tg) {
                                          sf::ConvexShape column(4);
                                          column.setPoint(0, {b0.x - w, b0.y});
                                          column.setPoint(1, {b0.x + w, b0.y});
                                          column.setPoint(2, {t0.x + w * 0.6f + wobble, t0.y});
                                          column.setPoint(3, {t0.x - w * 0.6f + wobble, t0.y});
                                          column.setFillColor(sf::Color(0x38, 0xbd, 0xf8, 200));
                                          column.setOutlineColor(hex(0x0c4a6e));
                                          column.setOutlineThickness(3.0f);
                                          tg.draw(column);
                                          dot(tg, t0 + sf::Vector2f{wobble, 0.0f}, w * 0.9f, sf::Color(0xe0, 0xf2, 0xfe), hex(0x0c4a6e), 2.0f);
                                      }});
                }
            }
            break;
        case SkillId::JoanaHookPull: {
            if (lab_[caster->isPlayer ? 0 : 1]) break;
            // Gancho voando preso numa corrente: vai até o alvo e volta puxando.
            float extend = elapsed <= 10 ? static_cast<float>(elapsed) / 10.0f : std::max(0.0f, 1.0f - static_cast<float>(elapsed - 10) / 16.0f);
            if (extend <= 0.01f) break;
            const PartRenderData& g = caster->gloveL;
            Vec3 from = Physics3D::localToWorld(g.x + caster->offsetX, g.y + caster->offsetY, g.z, caster->worldPos, caster->facing);
            Vec3 to{target.worldPos.x, target.worldPos.y + 75.0f, target.worldPos.z};
            Vec3 tip{from.x + (to.x - from.x) * extend, from.y + (to.y - from.y) * extend + std::sin(extend * kPi) * 25.0f,
                     from.z + (to.z - from.z) * extend};
            auto a = camera_.project(from), b = camera_.project(tip);
            if (a && b) {
                float thick = std::max(2.0f, 6.0f * (a->scale + b->scale) * 0.5f);
                sf::Vector2f pa = a->pos, pb = b->pos;
                queue_.push_back({layer, b->depth + 1.0f, [pa, pb, thick](sf::RenderTarget& tg) {
                                      drawThickLine(tg, pa, pb, thick + 3.0f, kInk);
                                      drawThickLine(tg, pa, pb, thick, hex(0x94a3b8));
                                  }});
            }
            if (auto hook = sprite("joanaHookFlyingSprite")) queueSprite(*hook, {tip.x, tip.y - 25.0f, tip.z}, 60.0f, 60.0f, 0.0f, layer);
            break;
        }
        case SkillId::JoanaCoralSpout:
            if (lab_[caster->isPlayer ? 0 : 1]) break;
            if (p >= 0.45f && !caster->signatureOnly) {
                if (!joanaCoralCrackActive_) {
                    joanaCoralCrackPos_ = target.worldPos;
                    joanaCoralCrackActive_ = true;
                }
                const char* stage = p < 0.6f ? "joanaCoralStage1Sprite" : (p < 0.75f ? "joanaCoralStage2Sprite" : "joanaCoralStage3Sprite");
                if (auto coral = sprite(stage)) {
                    queueSprite(*coral, {joanaCoralCrackPos_.x, 0.0f, joanaCoralCrackPos_.z}, 120.0f, 120.0f, 0.0f, target.isPlayer ? 4.6f : 3.75f);
                }
                if (auto crack = sprite("joanaCoralGroundCrackSprite")) {
                    queueSprite(*crack, {joanaCoralCrackPos_.x, 2.0f, joanaCoralCrackPos_.z}, 150.0f, 50.0f, 0.0f, 1.6f);
                }
            } else {
                joanaCoralCrackActive_ = false;
            }
            break;
        case SkillId::JoanaAnchorSlam: {
            if (lab_[caster->isPlayer ? 0 : 1]) break;
            if (elapsed >= 8) {
                Vec3 craterAt = Physics3D::localToWorld(-20.0f, 1.0f, 40.0f, caster->worldPos, caster->facing);
                if (auto crater = sprite("joanaAnchorImpactCraterSprite")) queueSprite(*crater, craterAt, 140.0f, 50.0f, 0.0f, 1.6f);
                float u = std::min(1.0f, static_cast<float>(elapsed - 8) / 10.0f);
                Vec3 to{target.worldPos.x, 0.0f, target.worldPos.z};
                Vec3 wave{craterAt.x + (to.x - craterAt.x) * u, 0.0f, craterAt.z + (to.z - craterAt.z) * u};
                if (auto front = sprite("joanaTsunamiWaveFrontSprite")) {
                    float fade = elapsed > 22 ? std::max(0.0f, 1.0f - (elapsed - 22) / 10.0f) : 1.0f;
                    queueSprite(*front, wave, 150.0f, 100.0f * fade + 1.0f, 0.0f, layer + 0.1f, sf::Color(255, 255, 255, static_cast<std::uint8_t>(230 * fade)));
                }
            }
            break;
        }
        case SkillId::JoanaOceanBounty:
            if (lab_[caster->isPlayer ? 0 : 1]) break;
            if (auto anemone = sprite("joanaAnemoneClusterSprite")) {
                float pulse = 1.0f + 0.1f * std::sin(static_cast<float>(engine.frameIndex) * 0.3f);
                queueSprite(*anemone, {mouth.x, mouth.y + 30.0f, mouth.z}, 80.0f * pulse, 80.0f * pulse, 0.0f, layer + 0.1f);
            }
            break;
        case SkillId::JoanaCrustaceanShell:
            if (lab_[caster->isPlayer ? 0 : 1]) break;
            if (auto shield = sprite("joanaBarnacleShieldSprite")) {
                Vec3 at = Physics3D::localToWorld(0.0f, 40.0f, 35.0f, caster->worldPos, caster->facing);
                queueSprite(*shield, at, 110.0f * std::sin(p * kPi) + 1.0f, 110.0f * std::sin(p * kPi) + 1.0f, 0.0f, layer + 0.1f);
            }
            break;
        case SkillId::ValeriaVolcanicErupt:
        case SkillId::MarcovOrbitalLaser:
        case SkillId::BossApocalypse: {
            if(caster->charId==7 && lab_[caster->isPlayer?0:1])break; // native themed controller owns beam/depth, no duplicate foreground pillar
            // Pilar de lava saindo do chão / laser do satélite (ou do Raio Apocalíptico) caindo do céu.
            const bool apocalypse = caster->activeSkill == SkillId::BossApocalypse;
            const bool laser = caster->activeSkill == SkillId::MarcovOrbitalLaser || apocalypse;
            if (p < 0.4f || p > 0.85f || caster->signatureOnly) break;
            float grow = std::min(1.0f, (p - 0.4f) / 0.12f);
            auto base = camera_.project({target.worldPos.x, 0.0f, target.worldPos.z});
            auto top = camera_.project({target.worldPos.x, laser ? 600.0f : 200.0f * grow, target.worldPos.z});
            if (!base || !top) break;
            float w = (apocalypse ? 34.0f : laser ? 22.0f : 34.0f) * base->scale;
            sf::Vector2f b0 = base->pos, t0 = top->pos;
            sf::Color fill = apocalypse ? sf::Color(0xef, 0x44, 0x44, 215)
                           : laser      ? sf::Color(0x7d, 0xd3, 0xfc, 210) : sf::Color(0xff, 0x6a, 0x1a, 220);
            sf::Color edge = apocalypse ? hex(0x7f1d1d) : laser ? hex(0x0369a1) : hex(0x7c2d12);
            const unsigned beamSkin=caster->skinId<4?caster->skinId:0;
            if(apocalypse){
                fill=beamSkin==1?sf::Color(98,215,229,210):beamSkin==2?sf::Color(230,170,94,205):beamSkin==3?sf::Color(113,228,242,215):fill;
                edge=beamSkin==1?hex(0xdc7cda):beamSkin==2?hex(0x7a4329):beamSkin==3?hex(0xf6c65f):edge;
                w*=grow*(.93f+.07f*std::sin(engine.frameIndex*.8f));
            }
            const float beamPulse=std::fmod(p*18.f,1.f);
            queue_.push_back({target.isPlayer ? 4.6f : 3.75f, base->depth, [b0, t0, w, fill, edge, apocalypse, beamSkin, beamPulse](sf::RenderTarget& tg) {
                                  sf::ConvexShape col(4);
                                  col.setPoint(0, {b0.x - w, b0.y});
                                  col.setPoint(1, {b0.x + w, b0.y});
                                  col.setPoint(2, {t0.x + w * 0.7f, t0.y});
                                  col.setPoint(3, {t0.x - w * 0.7f, t0.y});
                                  col.setFillColor(fill);
                                  col.setOutlineColor(edge);
                                  col.setOutlineThickness(3.0f);
                                  tg.draw(col);
                                  if(apocalypse){
                                      sf::ConvexShape core(4);core.setPoint(0,{b0.x-w*.30f,b0.y});core.setPoint(1,{b0.x+w*.30f,b0.y});core.setPoint(2,{t0.x+w*.20f,t0.y});core.setPoint(3,{t0.x-w*.20f,t0.y});core.setFillColor(sf::Color(255,246,220,225));tg.draw(core);
                                      for(int j=0;j<7;++j){const float u=std::fmod(beamPulse+j/7.f,1.f),y=t0.y+(b0.y-t0.y)*u,x=t0.x+(b0.x-t0.x)*u,r=w*(.7f+.3f*u);sf::RectangleShape band({r*2,beamSkin==2?5.f:2.f});band.setPosition({x-r,y});band.setFillColor(edge);tg.draw(band);}
                                  }
                              }});
            break;
        }
        case SkillId::CremosoPuddleTrap:
            if (lab_[caster->isPlayer ? 0 : 1]) break;
            if (p >= 0.5f) {
                const std::string skin = caster->loadout.has(SkillId::CremosoFrozenStamina) ? "Frozen"
                                       : caster->loadout.has(SkillId::CremosoSweetRevenge)  ? "Revenge" : "";
                if (auto puddle = sprite("cremosoSyrupPuddle" + skin + "Sprite")) {
                    queueSprite(*puddle, {target.worldPos.x, 1.0f, target.worldPos.z}, 150.0f, 60.0f, 0.0f, 1.6f);
                }
            }
            break;
        case SkillId::ValeriaAshCloud:
        case SkillId::EspinhaDesertWind:
        case SkillId::MarcovExhaustCloud:
        case SkillId::VivaldiAutumnRequiem:
        case SkillId::VivaldiCadenceStaccato:
        case SkillId::VivaldiWinterAllegro: {
            if((caster->charId==5 || caster->charId==7) && lab_[caster->isPlayer?0:1])break;
            // Nuvens de cinza/areia/fumaça, ondas sonoras e notas: pontos que vão até o alvo.
            sf::Color color = hex(0x9ca3af);
            if (caster->activeSkill == SkillId::EspinhaDesertWind) color = hex(0xd6b36a);
            else if (caster->activeSkill == SkillId::VivaldiAutumnRequiem) color = hex(0xa855f7);
            else if (caster->activeSkill == SkillId::VivaldiCadenceStaccato) color = hex(0xc084fc);
            else if (caster->activeSkill == SkillId::VivaldiWinterAllegro) color = hex(0xbae6fd);
            const float big = caster->activeSkill == SkillId::VivaldiCadenceStaccato || caster->activeSkill == SkillId::VivaldiWinterAllegro ? 5.0f : 13.0f;
            for (int i = 0; i < 7; ++i) {
                float u = std::fmod(p * 1.8f + static_cast<float>(i) * 0.14f, 1.0f);
                Vec3 from = Physics3D::localToWorld(0.0f, 70.0f, 30.0f, caster->worldPos, caster->facing);
                Vec3 to{target.worldPos.x, target.worldPos.y + 85.0f, target.worldPos.z};
                Vec3 at{from.x + (to.x - from.x) * u + std::sin(u * 7.0f + static_cast<float>(i)) * 18.0f,
                        from.y + (to.y - from.y) * u + std::cos(u * 5.0f + static_cast<float>(i)) * 10.0f,
                        from.z + (to.z - from.z) * u};
                auto pp = camera_.project(at);
                if (!pp) continue;
                float r = std::max(2.0f, big * pp->scale);
                sf::Vector2f c = pp->pos;
                queue_.push_back({layer, pp->depth, [c, r, color](sf::RenderTarget& tg) { dot(tg, c, r, withAlpha(color, 0.85f), withAlpha(kInk, 0.6f)); }});
            }
            break;
        }
        case SkillId::MarcovRocketThrust:
        case SkillId::CremosoSyrupBlast:
        case SkillId::ValeriaMagmaPunch: {
            if(caster->charId==7 && lab_[caster->isPlayer?0:1])break;
            if (caster->activeSkill == SkillId::CremosoSyrupBlast && lab_[caster->isPlayer ? 0 : 1]) break;
            // Rastro de fogo / calda atrás da luva que soca.
            sf::Color color = caster->activeSkill == SkillId::CremosoSyrupBlast ? hex(0xe11d48) : hex(0xfb923c);
            const PartRenderData& g = caster->gloveR;
            for (int i = 0; i < 5; ++i) {
                Vec3 at = Physics3D::localToWorld(g.x + caster->offsetX, g.y + caster->offsetY + 10.0f, g.z + caster->offsetZ - 14.0f * i,
                                                  caster->worldPos, caster->facing);
                auto pp = camera_.project(at);
                if (!pp) continue;
                float r = std::max(2.0f, (10.0f - i * 1.5f) * pp->scale);
                sf::Vector2f c = pp->pos;
                float alpha = 1.0f - i * 0.18f;
                queue_.push_back({layer + 0.05f, pp->depth, [c, r, color, alpha](sf::RenderTarget& tg) { dot(tg, c, r, withAlpha(color, alpha), withAlpha(kInk, alpha)); }});
            }
            break;
        }
        case SkillId::JackInfestation:
        case SkillId::JackCombBurst: {
            if (lab_[caster->isPlayer ? 0 : 1]) break; // efeito completo vem do Lab
            // Abelhas / pólen voando das luvas até o alvo.
            sf::Color color = caster->activeSkill == SkillId::JackInfestation ? hex(0xffd700) : hex(0xfacc15);
            for (int i = 0; i < 8; ++i) {
                float u = std::fmod(p * 2.2f + static_cast<float>(i) * 0.125f, 1.0f);
                const PartRenderData& g = (i % 2) ? caster->gloveL : caster->gloveR;
                Vec3 from = Physics3D::localToWorld(g.x, g.y, g.z, caster->worldPos, caster->facing);
                Vec3 to{target.worldPos.x, target.worldPos.y + 90.0f, target.worldPos.z};
                Vec3 at{from.x + (to.x - from.x) * u + std::sin(u * 9.0f + static_cast<float>(i)) * 14.0f, from.y + (to.y - from.y) * u,
                        from.z + (to.z - from.z) * u};
                auto pp = camera_.project(at);
                if (!pp) continue;
                float r = std::max(2.0f, 5.0f * pp->scale);
                sf::Vector2f c = pp->pos;
                queue_.push_back({layer, pp->depth, [c, r, color](sf::RenderTarget& tg) { dot(tg, c, r, color, kInk); }});
            }
            break;
        }
        default:
            break;
        }
    }
}

void RingRenderer::queueBossEffects(const GameEngine& engine) {
    auto renderBoss = [&](const Fighter& boss, const Fighter& target) {
    if (boss.bossStage == 0) return;
    const float t = static_cast<float>(engine.frameIndex);
    const unsigned skin=boss.skinId<4?boss.skinId:0;
    const sf::Color accent=hex(skin==1?0x62d7e5:skin==2?0xe6aa5e:skin==3?0x71e4f2:0xffd700);

    // Cortina de fumaça enquanto o robô se desmonta e remonta na contagem (some na contagem 5).
    if (!boss.isPlayer && engine.gameState == STATE_KNOCKDOWN && engine.enemyIsDown && engine.refereeCount < 5) {
        float alpha = engine.refereeCount >= 4 ? std::max(0.0f, 1.0f - static_cast<float>(60 - engine.refereeTimer) / 60.0f) : 0.98f;
        if (auto c = camera_.project({boss.worldPos.x, 100.0f, boss.worldPos.z})) {
            for (int s = 0; s < 7; ++s) {
                float fs = static_cast<float>(s);
                sf::Vector2f at = c->pos + sf::Vector2f{std::sin(t / 9.0f + fs) * 90.0f * c->scale, std::cos(t / 9.0f + fs * 2.0f) * 50.0f * c->scale - 20.0f};
                float r = (90.0f + std::sin(fs * 3.0f) * 30.0f) * c->scale;
                queue_.push_back({4.95f, c->depth - fs, [at, r, alpha](sf::RenderTarget& tg) {
                                      dot(tg, at, r, sf::Color(30, 26, 44, static_cast<std::uint8_t>(245 * alpha)), sf::Color::Transparent, 0.0f, 40);
                                      dot(tg, at, r * 0.6f, sf::Color(14, 12, 22, static_cast<std::uint8_t>(250 * alpha)), sf::Color::Transparent, 0.0f, 30);
                                  }});
            }
        }
    }

    if (boss.state != FIGHTER_SKILL || boss.signatureOnly) return;
    const SkillDef* def = SkillSystem::get(boss.activeSkill);
    if (!def) return;
    const float p = 1.0f - static_cast<float>(boss.timer) / static_cast<float>(std::max(1, boss.maxTimer));
    const float since = (p - def->fx.at) * static_cast<float>(boss.maxTimer); // quadros desde o impacto
    const float layer = 4.55f;
    auto toBoss = [&](float x, float y, float z) { return Physics3D::localToWorld(x, y, z, boss.worldPos, boss.facing); };

    switch (boss.activeSkill) {
    case SkillId::BossDualSaw: {
        // Serras desacopladas voam até o alvo e voltam aos pulsos.
        for (int side = 0; side < 2; ++side) {
            const PartRenderData& g = side == 0 ? boss.gloveL : boss.gloveR;
            Vec3 wrist = toBoss(g.x + boss.offsetX, g.y + boss.offsetY + 65.0f, g.z);
            Vec3 aimSide = Physics3D::localToWorld(side == 0 ? 26.0f : -26.0f, 60.0f, 0.0f, target.worldPos, target.facing);
            float u = p <= 0.5f ? std::sin(p / 0.5f * kPi * 0.5f) : 1.0f - std::sin((p - 0.5f) / 0.5f * kPi * 0.5f);
            float arc = std::sin(u * kPi) * 16.0f;
            Vec3 at{wrist.x + (aimSide.x - wrist.x) * u, wrist.y + (aimSide.y - wrist.y) * u + arc, wrist.z + (aimSide.z - wrist.z) * u};
            const bool back=boss.isPlayer;
            const std::string name=fighterRigKey(8,skin)+"_base_"+(back?"back":"front")+"_"+std::to_string(side)+"_"+std::to_string((engine.frameIndex/3)%8);
            if (auto saw = sprite(name)) {
                queueSprite(*saw, {at.x, at.y - 69.0f, at.z}, 138.0f, 138.0f, 0.0f, layer, sf::Color::White);
            }
            if (auto sp = camera_.project(at)) {
                std::uniform_real_distribution<float> jitter(-1.0f, 1.0f);
                for (int s = 0; s < 4; ++s) {
                    sf::Vector2f c = sp->pos + sf::Vector2f{jitter(rng_) * 60.0f * sp->scale, jitter(rng_) * 60.0f * sp->scale};
                    float r = std::max(1.5f, (2.5f + std::fabs(jitter(rng_)) * 3.5f) * sp->scale);
                    queue_.push_back({layer + 0.01f, sp->depth, [c, r, accent](sf::RenderTarget& tg) { dot(tg, c, r, accent, hex(0xef4444), 1.0f); }});
                }
            }
        }
        break;
    }
    case SkillId::BossOverclock:
    case SkillId::BossEmp: {
        // Onda no chão em volta do alvo: ruptura térmica vermelha ou pulso eletromagnético ciano.
        if (since < 0.0f) break;
        const bool emp = boss.activeSkill == SkillId::BossEmp;
        float radius = 10.0f + since * (emp ? 6.5f : 5.5f);
        float life = std::max(0.0f, 1.0f - since * (emp ? 0.028f : 0.03f));
        if (auto c = camera_.project({target.worldPos.x, 2.0f, target.worldPos.z})) {
            float rx = radius * c->scale, ry = rx * 0.4f;
            sf::Vector2f at = c->pos;
            sf::Color edge = skin ? accent : emp ? hex(0x00f5d4) : hex(0xef4444), inner = skin==1 ? hex(0xdc7cda) : skin==2 ? hex(0xf7d49a) : skin==3 ? hex(0xf6c65f) : emp ? hex(0xa5f3fc) : hex(0xf59e0b);
            queue_.push_back({1.45f, c->depth, [at, rx, ry, life, edge, inner](sf::RenderTarget& tg) {
                                  for (int ring = 0; ring < 2; ++ring) {
                                      float k = ring == 0 ? 1.0f : 0.72f;
                                      sf::CircleShape e(rx * k, 48);
                                      e.setOrigin({rx * k, rx * k});
                                      e.setScale({1.0f, ry / rx});
                                      e.setPosition(at);
                                      e.setFillColor(ring == 0 ? withAlpha(inner, 0.25f * life) : sf::Color::Transparent);
                                      e.setOutlineColor(withAlpha(ring == 0 ? edge : inner, life));
                                      e.setOutlineThickness(ring == 0 ? 6.0f : 3.0f);
                                      tg.draw(e);
                                  }
                              }});
        }
        break;
    }
    case SkillId::BossMissiles: {
        // Quatro micromísseis em arco, cada um chegando no seu golpe.
        const float span = (0.9f - def->fx.at) / 3.0f;
        for (int m = 0; m < 4; ++m) {
            float hit = def->fx.at + span * static_cast<float>(m), launch = std::max(0.02f, hit - 0.3f);
            if (p < launch || p > hit) continue;
            float u = (p - launch) / (hit - launch);
            Vec3 from = toBoss(m % 2 == 0 ? -32.0f : 32.0f, 95.0f + m * 8.0f, 26.0f);
            Vec3 to = Physics3D::localToWorld(m % 2 == 0 ? 18.0f : -18.0f, 55.0f + (m - 1.5f) * 10.0f, 0.0f, target.worldPos, target.facing);
            for (int trail = 0; trail < 5; ++trail) {
                float v = std::max(0.0f, u - trail * 0.06f);
                Vec3 at{from.x + (to.x - from.x) * v, from.y + (to.y - from.y) * v + std::sin(v * kPi) * 30.0f, from.z + (to.z - from.z) * v};
                auto sp = camera_.project(at);
                if (!sp) continue;
                float r = std::max(2.0f, (trail == 0 ? 9.0f : 7.0f - trail) * sp->scale);
                sf::Vector2f c = sp->pos;
                bool head = trail == 0;
                float alpha = 1.0f - trail * 0.18f;
                queue_.push_back({layer, sp->depth - (head ? 1.0f : 0.0f), [c, r, head, alpha, skin, accent](sf::RenderTarget& tg) {
                                      if (head) {
                                          dot(tg, c, r, hex(0xe2e8f0), kInk, 2.0f);
                                          dot(tg, c, r * 0.5f, skin?accent:hex(0xef4444), sf::Color::Transparent, 0.0f);
                                      } else {
                                          dot(tg, c, r, sf::Color(0xcb, 0xd5, 0xe1, static_cast<std::uint8_t>(170 * alpha)), sf::Color::Transparent, 0.0f);
                                      }
                                  }});
            }
        }
        break;
    }
    case SkillId::BossHolo: {
        // Clones holográficos do Nerd em volta do alvo, piscando.
        if (p < 0.25f) break;
        for (int h = 0; h < 3; ++h) {
            auto clone = sprite(fighterRigKey(8,skin)+"_clone_"+(boss.isPlayer?"back":"front")+"_"+std::to_string((engine.frameIndex/3+h*3)%12));
            if(!clone)continue;
            bool on = (engine.frameIndex / 2 + static_cast<uint32_t>(h)) % 3 != 0;
            if (!on) continue;
            Vec3 at = Physics3D::localToWorld((h - 1) * 48.0f, 0.0f, 34.0f, target.worldPos, target.facing);
            float fade = 1.0f - std::max(0.0f, p - 0.85f) / 0.15f;
            // Clone artwork uses a 350-unit padded canvas (body ground y=146).
            // Preserve its native body height and floor contact, not the padding.
            at.y-=104.f*128.f/146.f;
            queueSprite(*clone, at, 350.f*128.f/146.f, 350.f*128.f/146.f, (h - 1) * 0.08f, layer, sf::Color(120, 200, 255, static_cast<std::uint8_t>(150 * fade)));
        }
        break;
    }
    default:
        break;
    }
    };
    renderBoss(engine.enemy,engine.player);
    renderBoss(engine.player,engine.enemy);
}

void RingRenderer::drawSkillDeck(sf::RenderTarget& target, const Fighter& f, bool alignRight) {
    if (!SkillSystem::hasSkills(f.charId) && f.bossStage == 0) return;
    // O painel do oponente mostra a build sorteada dele; o do jogador, a build equipada.
    const std::array<SkillId, 3> slots = {f.loadout.spec1, f.loadout.spec2, f.loadout.passive};
    const float w = 252.0f, h = 26.0f, gap = 5.0f;
    const float x = alignRight ? kLogicalWidth - 16.0f - w : 16.0f;
    float y = kLogicalHeight - 16.0f - 3.0f * h - 2.0f * gap;

    for (SkillId id : slots) {
        const SkillDef* def = SkillSystem::get(id);
        if (!def) { y += h + gap; continue; }
        const sf::Color tc = ui::skillTagColor(def->tag);
        const bool casting = f.state == FIGHTER_SKILL && f.activeSkill == id && !f.signatureOnly;

        sf::RectangleShape row({w, h});
        row.setPosition({x, y});
        row.setFillColor(sf::Color(0x0f, 0x17, 0x2a, 225));
        row.setOutlineColor(casting ? tc : kInk);
        row.setOutlineThickness(casting ? 3.0f : 2.0f);
        target.draw(row);

        // Recarga das curas: barra escura que esvazia.
        if ((id == SkillId::JackRoyalJelly || id == SkillId::FrogFlyCatcher) && f.status.healCooldown > 0) {
            sf::RectangleShape cd({w * static_cast<float>(f.status.healCooldown) / (8.0f * 60.0f), h});
            cd.setPosition({x, y});
            cd.setFillColor(sf::Color(0, 0, 0, 150));
            target.draw(cd);
        }

        sf::RectangleShape tag({58.0f, h - 8.0f});
        tag.setPosition({x + 4.0f, y + 4.0f});
        tag.setFillColor(tc);
        target.draw(tag);
        ui::text(target, fonts_.display, def->tag, {x + 33.0f, y + 5.0f}, 12u, sf::Color(0x0f, 0x17, 0x2a), 0.5f, 0.0f);
        // Nomes longos ("Reagrupamento das Brasas") encolhem para não invadir o comando.
        const bool longName = std::string(def->name).size() > 22;
        ui::text(target, fonts_.display, def->name, {x + 68.0f, y + (longName ? 6.0f : 4.0f)}, longName ? 11u : 13u, sf::Color::White, 0.0f, 1.5f);
        if (def->type != SkillType::Passive) {
            const std::string pL = keyBindings_.actionName(ActionKey::PunchL, true);
            const std::string pR = keyBindings_.actionName(ActionKey::PunchR, true);
            ui::commandHint(target, fonts_.display, def->command, x + w - 6.0f, y + h / 2.0f, hex(0xffe169), pL, pR);
        }
        y += h + gap;
    }
}

void RingRenderer::drawStatusChips(sf::RenderTarget& target, const Fighter& f, sf::Vector2f pos, bool alignRight) {
    struct Chip { const char* label; sf::Color color; };
    std::vector<Chip> chips;
    if (f.status.infested > 0) chips.push_back({"ENXAME", hex(0xfbbf24)});
    if (f.status.poisoned > 0) chips.push_back({"VENENO", hex(0xa3e635)});
    if (f.status.slowed > 0) chips.push_back({"LENTO", hex(0x93c5fd)});
    if (f.status.blind > 0) chips.push_back({"CEGO", hex(0xfacc15)});
    if (f.status.mud > 0) chips.push_back({"NA LAMA", hex(0xb45309)});
    if (f.status.molasses > 0) chips.push_back({"MELAÇO", hex(0xf59e0b)});
    if (f.status.poisonSkin > 0) chips.push_back({"PELE TÓXICA", hex(0x10b981)});
    if (f.status.bleed > 0) chips.push_back({"SANGRANDO", hex(0xf87171)});
    if (f.status.shell > 0) chips.push_back({"CRACAS", hex(0x0284c7)});
    if (f.monstrous) chips.push_back({"MONSTRUOSA", hex(0xfb923c)});
    if (f.status.burn > 0) chips.push_back({"QUEIMANDO", hex(0xfb923c)});
    if (f.status.melt > 0) chips.push_back({"DERRETIDO", hex(0xfde68a)});
    if (f.status.heatwave > 0) chips.push_back({"CALOR", hex(0xff7a45)});
    if (f.status.seasons > 0) chips.push_back({"4 ESTAÇÕES", hex(0x34d399)});
    if (f.status.thornVeil > 0) chips.push_back({"ESPINHOS", hex(0x10b981)});
    if (f.status.plasma > 0) chips.push_back({"PLASMA", hex(0x22d3ee)});
    if (f.coldMagma) chips.push_back({"MAGMA FRIO", hex(0x94a3b8)});
    if (f.bossStage == 1) chips.push_back({"BLINDADO", hex(0x94a3b8)});
    if (f.bossStage == 2) chips.push_back({"FÚRIA", hex(0x00f5d4)});
    if (f.bossStage == 3) chips.push_back({"QUÂNTICO", hex(0x818cf8)});
    if (f.status.honeyShield > 0) {
        chips.push_back({f.status.honeyShield >= 3 ? "FAVO x3" : (f.status.honeyShield == 2 ? "FAVO x2" : "FAVO x1"), hex(0x38bdf8)});
    }
    float x = pos.x;
    for (const Chip& c : chips) {
        const float w = 74.0f;
        sf::RectangleShape box({w, 18.0f});
        box.setPosition({alignRight ? x - w : x, pos.y});
        box.setFillColor(c.color);
        box.setOutlineColor(kInk);
        box.setOutlineThickness(2.0f);
        target.draw(box);
        ui::text(target, fonts_.display, c.label, {box.getPosition().x + w / 2.0f, pos.y + 1.0f}, 13u, sf::Color(0x0f, 0x17, 0x2a), 0.5f, 0.0f);
        x += alignRight ? -(w + 6.0f) : (w + 6.0f);
    }
}

} // namespace KoRama
