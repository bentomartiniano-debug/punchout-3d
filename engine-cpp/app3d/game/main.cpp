// app3d/game/main.cpp: escolha de lutadores e a luta em 3D. Toda regra vem de GameEngine (korama_core).
#include "render/renderer.hpp"
#include "render/impact_fx.hpp"
#include "render/skill_fx.hpp"
#include "render/fx_clips.hpp"
#include "render/char_fx.hpp"
#include "render/water_fx.hpp"
#include "render/leap_fx.hpp"
#include "render/tongue_fx.hpp"
#include "render/geyser_fx.hpp"
#include "render/throwables.hpp"
#include "render/cloud_fx.hpp"
#include "render/stab_fx.hpp"
#include "render/sting_fx.hpp"
#include "render/spit_fx.hpp"
#include "render/honey_fx.hpp"
#include "celebration.hpp"
#include "canvas2d.hpp"
#include <optional>
#include "engine.hpp"
#include "menu_screens.hpp"
#include "ring_renderer.hpp"
#include "audio_system.hpp"
#include "sprite_atlas.hpp"
#include "ui_fonts.hpp"
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <exception>
#include <array>
#include <deque>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <vector>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
using namespace KoRama;
namespace {
// charId do motor -> arquivos em assets3d/game (o Batalhator tem um modelo por fase; a arena é a do oponente)
const char* const kNames[9] = {"JACK", "KING FROG", "JOANA", "O CREMOSO", "VALERIA", "VIVALDI", "LADY ESPINHA", "DR. MARCOV", "BATALHATOR"};
const char* const kFiles[9] = {"jack", "frog", "joana", "cremoso", "valeria", "vivaldi", "espinha", "marcov", "batalhator"};
const char* const kArenas[9] = {"jack", "frog", "joana", "cremoso", "pernambuco", "vivaldi", "espinha", "marcov", "batalhator"};
constexpr double kTick = 1.0 / 60.0;
uint32_t readInput() {
    using K = sf::Keyboard::Key; const auto h = [](K k){ return sf::Keyboard::isKeyPressed(k); }; uint32_t m = 0;
    if (h(K::Up)) m |= INPUT_UP;
    if (h(K::Down)) m |= INPUT_DOWN;
    if (h(K::Left)) m |= INPUT_LEFT;
    if (h(K::Right)) m |= INPUT_RIGHT;
    if (h(K::Z)) m |= INPUT_PUNCH_L;
    if (h(K::X)) m |= INPUT_PUNCH_R;
    if (h(K::Space)) m |= INPUT_BLOCK;
    if (h(K::C)) m |= INPUT_DODGE_KEY;
    return m;
}
// tecla da partida -> bit do motor (toque mais rápido que um quadro ainda conta, como no jogo 2D)
uint32_t keyBit(sf::Keyboard::Key k) {
    using K = sf::Keyboard::Key;
    switch (k) { case K::Up: return INPUT_UP; case K::Down: return INPUT_DOWN; case K::Left: return INPUT_LEFT; case K::Right: return INPUT_RIGHT; case K::Z: return INPUT_PUNCH_L; case K::X: return INPUT_PUNCH_R; case K::Space: return INPUT_BLOCK; case K::C: return INPUT_DODGE_KEY; default: return 0; }
}
// esquiva: C com a direção (como o jogo 2D); sem direção, abaixa
uint32_t withDodge(uint32_t m) {
    if (m & INPUT_DODGE_KEY) { if (m & INPUT_LEFT) m |= INPUT_DODGE_L; else if (m & INPUT_RIGHT) m |= INPUT_DODGE_R; else if (m & INPUT_DOWN) m |= INPUT_DODGE_B; else m |= INPUT_DUCK; }
    return m;
}
RigRest restOf(const Model& m) {
    RigRest r; r.WAIST = m.WAIST; r.NECK = m.NECK; r.HIPS = m.HIPS; r.legLen = m.legLen; r.scale = m.scale;
    r.handX = m.num("handX", 0.74f) * 0.78f; r.handY = m.num("handY", 0.92f); if (m.num("scale", 1.f) > 1.f) r.gloveZ0 = std::max(kGloveZ0, m.num("handZ", kGloveZ0));   // sem braços e de peito largo, a luva colada no corpo parece ombreira
      // a guarda do motor é mais fechada que a do vinil
    r.glove = m.has("\"gloveMode\":\"umb\"") ? GloveMode::Umbrella : m.has("\"gloveMode\":\"saw\"") ? GloveMode::Saw : m.has("\"gloveMode\":\"stick\"") ? GloveMode::Stick : m.has("\"gloveMode\":\"hive\"") ? GloveMode::Hive : GloveMode::Fist;
    r.hasLegs = m.has("\"hasLegs\":true");
    if (r.hasLegs) { r.hip[0] = m.nodes[m.need("legL")].pos; r.hip[1] = m.nodes[m.need("legR")].pos; r.foot[0] = m.nodes[m.need("bootL")].pos; r.foot[1] = m.nodes[m.need("bootR")].pos;
        // perna reta no modelo (quadril em cima do pé, sem saia para furar): sai do quadril de verdade, e a guarda abre menos para a perna curta não ficar na diagonal
        if (std::fabs(r.hip[0].x - r.foot[0].x) < 0.01f) { r.hipIn = 1.f; r.stance = 0.5f; } }
    // sem pernas soltas mas com a parte de baixo que dobra (King Frog): dois pés virtuais embaixo dela, que o rig planta e faz andar
    if (!r.hasLegs && m.byName.count("lower")) { float lo = 0, hi = 0; for (const Node& n : m.nodes) if (n.lower) for (const Prim& pr : n.prims) { lo = std::min(lo, pr.lo.x); hi = std::max(hi, pr.hi.x); }
        const float fx = 0.28f * (hi - lo); r.virtualFeet = true; r.stance = 0.5f; r.hipIn = 1.f; r.legLen = r.HIPS;
        for (int i = 0; i < 2; i++) { const float sx = i ? fx : -fx; r.foot[i] = glm::vec3(sx, 0.f, 0.f); r.hip[i] = glm::vec3(sx, r.HIPS, 0.f); } }
    return r;
}
void bar(sf::RenderWindow& w, float x, float y, float W, float k, sf::Color c) {
    sf::RectangleShape b({W, 18}); b.setPosition({x, y}); b.setFillColor({20,16,30}); b.setOutlineColor(sf::Color::White); b.setOutlineThickness(2); w.draw(b);
    sf::RectangleShape f({W * std::max(0.f, std::min(1.f, k)), 18}); f.setPosition({x, y}); f.setFillColor(c); w.draw(f);
}
void hud(sf::RenderWindow& w, const sf::Font& font, const GameEngine& e) {
    const float W = float(w.getSize().x);
    bar(w, 24, 20, W*0.36f, e.player.hp/e.player.maxHp, {230,60,60}); bar(w, 24, 42, W*0.26f, e.player.stamina/e.player.maxStamina, {250,200,60});
    bar(w, W-24-W*0.36f, 20, W*0.36f, e.enemy.hp/e.enemy.maxHp, {230,60,60}); bar(w, W-24-W*0.26f, 42, W*0.26f, e.enemy.stamina/e.enemy.maxStamina, {250,200,60});
    const auto text = [&](const std::string& s, unsigned size, float y){ sf::Text t(font, sf::String::fromUtf8(s.begin(), s.end()), size); t.setOutlineColor(sf::Color::Black); t.setOutlineThickness(3); const auto b = t.getLocalBounds(); t.setPosition({(W-b.size.x)/2, y}); w.draw(t); };
    const int sec = e.roundFrames/60; char clock[32]; std::snprintf(clock, sizeof clock, "R%d  %d:%02d", e.round, sec/60, sec%60); text(clock, 30, 14);
    // especiais equipados do jogador: nome e comando, apagado enquanto não há fôlego ou o modo já está ativo
    { const auto cmd = [](SkillCommand c) -> const char* { switch (c) { case SkillCommand::BackForward: return "< > + SOCO"; case SkillCommand::DownUp: return "v ^ + SOCO"; case SkillCommand::BothPunches: return "Z + X"; case SkillCommand::UpUpRight: return "^ ^ + X"; case SkillCommand::DownDownLeft: return "v v + Z"; case SkillCommand::BackBackLeft: return "< < + Z"; case SkillCommand::ForwardForwardRight: return "> > + X"; case SkillCommand::BackForwardLeft: return "< > + Z"; default: return ""; } };
      float y = 64; for (const SkillId id : {e.player.loadout.spec1, e.player.loadout.spec2}) { const SkillDef* d = SkillSystem::get(id); if (!d) continue;
        const bool busy = e.player.state == FIGHTER_SKILL && e.player.activeSkill == id; const std::string line = std::string(d->name) + "   " + cmd(d->command); sf::Text t(font, sf::String::fromUtf8(line.begin(), line.end()), 20); t.setOutlineColor(sf::Color::Black); t.setOutlineThickness(2); t.setFillColor(busy ? sf::Color(255, 210, 80) : sf::Color::White); t.setPosition({24, y}); w.draw(t); y += 24; } }
    if (!e.combat.statusBanner.empty() && e.combat.statusBannerTimer > 0.f) text(e.combat.statusBanner, 44, 96);
    if (e.gameState == STATE_ROUND_INTRO) text("PREPARAR", 72, 240);
    if (e.gameState == STATE_ROUND_OVER) text("FIM DO ROUND", 64, 240);
    if (e.gameState == STATE_KNOCKDOWN) { text(std::to_string(e.refereeCount), 80, 60); if (!e.enemyIsDown) text("APERTE Z E X PARA LEVANTAR!", 34, 640); }
    if (e.isMatchOver()) { const MatchResult r = e.result; const bool win = r==MatchResult::WinKO||r==MatchResult::WinTKO||r==MatchResult::WinDecision; text(r==MatchResult::Draw ? "EMPATE" : win ? "VITORIA" : "DERROTA", 90, 200); text("ENTER: REVANCHE   ESC: ESCOLHER", 28, 330); }
}
// estrela de quadrinho com a palavra, ao lado do golpe para não esconder a reação; o estouro é 3D (render/particles.hpp)
// "!" vermelho sobre o oponente enquanto ele arma um especial (k 0..1): cresce e pisca mais rápido perto do golpe
void drawWarnMark(sf::RenderWindow& w, const sf::Font& font, glm::vec2 at, float k) {
    const float pulse = 0.5f + 0.5f * std::sin(k * 40.f), s = 1.f + 0.25f * k + 0.12f * pulse;
    sf::Text t(font, "!", 64); t.setFillColor(sf::Color(255, 60, 40)); t.setOutlineColor(sf::Color(20, 12, 12)); t.setOutlineThickness(5.f);
    const auto b = t.getLocalBounds(); t.setOrigin({b.position.x + b.size.x * 0.5f, b.position.y + b.size.y}); t.setScale({s, s}); t.setPosition({at.x, at.y}); w.draw(t);
}
// combo de entrada de um especial: uma bolinha por soco, acesa a cada golpe dado (sobre a cabeça de quem está montando o combo)
void drawChainPips(sf::RenderWindow& w, glm::vec2 at, int count, int need, sf::Color on) {
    const float r = 7.f, gap = 21.f, x0 = at.x - gap * float(need - 1) * 0.5f;
    for (int i = 0; i < need; i++) { sf::CircleShape c(r); c.setOrigin({r, r}); c.setPosition({x0 + gap * float(i), at.y});
        c.setFillColor(i < count ? on : sf::Color(20, 12, 12, 140)); c.setOutlineColor(sf::Color(20, 12, 12)); c.setOutlineThickness(3.f); w.draw(c); }
}
void drawImpact(sf::RenderWindow& w, const sf::Font& font, const ImpactFx& fx, glm::vec2 at) {
    const float a = fx.age(), k = fx.scale() * 1.2f; const std::uint8_t al = std::uint8_t(255 * fx.alpha());
    const float side = (fx.seed % 2) ? 1.f : -1.f; sf::Transform tf; tf.translate({at.x + side * 150.f, at.y - 70.f - a * 14.f}); tf.rotate(sf::degrees(float((fx.seed % 5) - 2) * 6.f)); tf.scale({k, k});
    constexpr int kSpikes = 14; sf::ConvexShape star(kSpikes * 2);
    for (int i = 0; i < kSpikes * 2; i++) { const float an = i * 3.14159265f / kSpikes, r0 = i % 2 ? 62.f : 100.f + float((fx.seed * 7 + i * 13) % 5) * 9.f; star.setPoint(i, {std::cos(an) * r0, std::sin(an) * r0 * 0.7f}); }
    const sf::Color fill = fx.kind == 2 ? sf::Color(0, 245, 212, al) : fx.kind == 3 ? sf::Color(255, 0, 85, al) : fx.kind == 4 ? sf::Color(255, 30, 66, al) : sf::Color(255, 230, 0, al);
    sf::Transform shadow = tf; shadow.translate({7, 7}); star.setFillColor({0, 0, 0, al}); w.draw(star, shadow);
    star.setFillColor(fill); star.setOutlineColor({0, 0, 0, al}); star.setOutlineThickness(6); w.draw(star, tf);
    sf::Text t(font, sf::String::fromUtf8(fx.word(), fx.word() + std::strlen(fx.word())), fx.kind == 2 ? 54 : 64); const auto b = t.getLocalBounds(); t.setOrigin({b.position.x + b.size.x / 2, b.position.y + b.size.y / 2});
    t.setFillColor(fx.kind <= 1 ? sf::Color(255, 30, 66, al) : sf::Color(255, 255, 255, al)); t.setOutlineColor({0, 0, 0, al}); t.setOutlineThickness(4); w.draw(t, tf);
}
// tela de escolha: primeiro o seu lutador, depois o oponente (a arena é a dele)
// ---------------- builds: dois especiais e uma passiva por lutador, escolhidos entre os 9 golpes dele ----------------
// Ficam no mesmo korama_save.txt do jogo 2D (linhas "build <lutador> <especial1> <especial2> <passiva>"); as outras linhas do arquivo ficam como estão.
using Builds = std::array<SkillLoadout, 9>;
Builds loadBuilds(const std::filesystem::path& file) {
    Builds b; for (uint32_t c = 0; c < 9; c++) b[c] = SkillSystem::defaultLoadout(c);
    if (b[8].spec1 == SkillId::None) b[8] = SkillLoadout{SkillId::BossDualSaw, SkillId::BossOverclock, SkillId::BossTitanium};   // o chefe não tem padrão fora da campanha: o da fase 1
    std::ifstream in(file); std::string line;
    while (std::getline(in, line)) { std::istringstream ss(line); std::string tag, k1, k2, k3; uint32_t c = 0; if (!(ss >> tag >> c >> k1 >> k2 >> k3) || tag != "build" || c >= 9) continue;
        const auto id = [](const std::string& k) { const SkillDef* d = SkillSystem::getByKey(k); return d ? d->id : SkillId::None; }; b[c] = SkillLoadout{id(k1), id(k2), id(k3)}; }
    return b;
}
void saveBuilds(const std::filesystem::path& file, const Builds& b) {
    std::vector<std::string> keep; { std::ifstream in(file); std::string line; while (std::getline(in, line)) if (line.rfind("build ", 0) != 0) keep.push_back(line); }
    if (keep.empty()) keep.push_back("# K.O.-RAMA - builds equipadas");
    std::ofstream out(file); for (const std::string& l : keep) out << l << '\n';
    const auto key = [](SkillId id) { const SkillDef* d = SkillSystem::get(id); return std::string(d ? d->key : "none"); };
    for (uint32_t c = 0; c < 9; c++) out << "build " << c << ' ' << key(b[c].spec1) << ' ' << key(b[c].spec2) << ' ' << key(b[c].passive) << '\n';
}
// ---------------- replay do nocaute: o que se desenha em cada quadro, gravado durante a luta ----------------
// efeitos de golpe dos dois lutadores (vinil, ondas, gêiseres, língua, cuspidas, mel, ferrão, estacas, nuvens): a luta usa e o replay grava uma cópia por quadro
struct FxSet {
    FxPlayer fxP, fxE; WaveFx waveP, waveE; TongueFx tongueP, tongueE; GeyserFx geyP, geyE; CloudFx cloudP, cloudE; StabFx stabP, stabE; StingFx stingP, stingE; SpitFx spitP, spitE; HoneyFx honeyP, honeyE;
    BeeFx beeP, beeE; MeltFx meltP, meltE; ConeFx coneP, coneE; BubbleFx bubP, bubE; LaserFx lasP, lasE; JetFx jetP, jetE; SyrupFx syrP, syrE; HeatFx heatP, heatE;   // especiais refeitos em código (render/char_fx.hpp)
    float poisonP = 0.f, poisonE = 0.f;   // Pele Venenosa
    glm::vec3 warnAt{0.f}; float warnK = 0.f, warnSize = 1.f;   // especial do oponente sendo armado: marca no chão nos pés do jogador (0..1 = quanto falta)
};
struct ReplayFrame {
    FighterPose pP, pE; glm::vec3 lagP{0.f}, lagE{0.f}; ClothSim cP, cE; RingRopes ropes; std::vector<Particle> parts;
    const char* faceP = "normal"; const char* faceE = "normal"; float t60 = 0.f; ImpactFx impact; std::vector<SoundCue> sounds; bool hit = false; float wobP = 0.f, wobE = 0.f, dirP = 0.f, dirE = 0.f; /* balanço do impacto, só do replay */ FxSet fx;   // hit: o quadro do golpe que nocauteou
};
constexpr size_t kReplayFrames = 300;   // 5 s de luta guardados
constexpr int kRewindFrames = 26;       // a fita voltando antes de passar de novo
constexpr int kShotFrames = 24;         // duração de cada corte de câmera antes do golpe
constexpr int kExtraFreeze = 1;
constexpr int kFinisherFreeze = 24;   // quadros parados no golpe que derruba (quadro de impacto, câmera avançando e linhas radiais)      // quadros de congelamento somados ao hitstop do motor, só visual (o motor fica parado)
constexpr float kBodyFront = 0.12f;   // do centro do alvo à frente do corpo
bool dodging(const Fighter& f) { return f.state == FIGHTER_DODGE_L || f.state == FIGHTER_DODGE_R || f.state == FIGHTER_DODGE_BACK; }
// até onde a luva do atacante vai no eixo dele; alvo esquivando deixa o soco passar
float reachOf(const PoseSmoother& atk, const PoseSmoother& def, float defSquash, float tip) { return dodging(def.view) ? 1e9f : glm::length(atk.pos - def.pos) - kBodyFront - tip + 0.26f * defSquash; }
float approach(float v, float t, float s) { return v < t ? std::min(t, v+s) : std::max(t, v-s); }
const char* faceOf(const Fighter& f, float down, int mouth = 0) { if (down <= 0.5f && mouth) return mouth == 2 ? "tongue2" : "tongue1";   // língua do King Frog: boca aberta pintada no rosto
    return down > 0.5f ? "ko" : (f.state == FIGHTER_HIT && f.hitKind) ? "pain" : isPunchState(f.state) ? "angry" : "normal"; }
}
int main(int argc, char** argv) {
    try {
        const bool snap = argc >= 5 && std::string(argv[1]) == "--snap";   // --snap saida.png quadro mascara
        // sorteios (oponente da Luta Solo, IA, builds, chaveamento do torneio) variam a cada vez que o jogo abre; --snap e --bench ficam repetíveis
        if (!snap && !(argc >= 3 && std::string(argv[1]) == "--bench")) std::srand(static_cast<unsigned>(std::time(nullptr)));
        // sem perfil core: a SFML desenha a HUD com o pipeline de compatibilidade
        // atlas das estampas das roupas jogadas (render/throwables.hpp): o tecido de cada calcinha e cueca do jogo original numa célula, e uma célula branca
        // (desenhado antes de criar a janela: a RenderTexture mexe no estado do OpenGL e a cena 3D sumia)
        sf::Image atlasImg;
        { sf::RenderTexture rt; const float C = float(Throwables::kAtlasCell), k = C / 200.f;
          if (rt.resize({unsigned(Throwables::kAtlasSize), unsigned(Throwables::kAtlasSize)})) { rt.clear(sf::Color::White);
              { cv::Canvas ctx(rt);
                for (int c = 0; c < Throwables::kUnderKinds; c++) { const Throwables::ClothDef& cd = Throwables::clothDef(c); const float x0 = float(c % Throwables::kAtlasCols) * C, y0 = float(c / Throwables::kAtlasCols) * C;
                  ctx.fillStyle = cd.main; ctx.fillRect(x0, y0, C, C); ctx.save(); ctx.translate(x0, y0); ctx.scale(k, k);   // dentro da célula: unidades da arte (200 = a peça inteira)
                  const char* ink = "#2a1a22"; ctx.lineJoin = "round"; ctx.lineCap = "round";
                  const auto heart = [&](float x, float y, float r) { for (int pass = 0; pass < 2; pass++) { const float e = pass ? 0.f : 2.4f; ctx.fillStyle = pass ? cd.patternColor : ink;   // dois círculos e um triângulo; contorno de tinta por baixo
                      for (float sd : {-1.f, 1.f}) { ctx.beginPath(); ctx.arc(x + sd * r * 0.5f, y - r * 0.25f, r * 0.56f + e, 0.f, 6.2832f); ctx.closePath(); ctx.fill(); }
                      ctx.beginPath(); ctx.moveTo(x - r * 1.04f - e * 1.2f, y - r * 0.05f); ctx.lineTo(x + r * 1.04f + e * 1.2f, y - r * 0.05f); ctx.lineTo(x, y + r * 1.05f + e * 1.5f); ctx.closePath(); ctx.fill(); } };
                  const auto dot = [&](float x, float y, float r) { ctx.beginPath(); ctx.arc(x, y, r, 0.f, 6.2832f); ctx.closePath(); ctx.fillStyle = cd.patternColor; ctx.fill(); ctx.strokeStyle = ink; ctx.lineWidth = 1.8f; ctx.stroke(); };
                  const auto bow = [&](float x, float y, float r) { for (float sd : {-1.f, 1.f}) { ctx.beginPath(); ctx.moveTo(x, y); ctx.lineTo(x + sd * r, y - r * 0.6f); ctx.lineTo(x + sd * r, y + r * 0.6f); ctx.closePath(); ctx.fillStyle = cd.patternColor; ctx.fill(); ctx.strokeStyle = ink; ctx.lineWidth = 1.8f; ctx.stroke(); } dot(x, y, r * 0.28f); };
                  if (cd.pattern == Throwables::kStripes) { const bool vertical = cd.boxer; for (float q = -10.f; q < 210.f; q += 30.f) { ctx.fillStyle = cd.patternColor; if (vertical) ctx.fillRect(q, 0.f, 14.f, 200.f); else ctx.fillRect(0.f, q, 200.f, 14.f);
                          ctx.strokeStyle = ink; ctx.lineWidth = 1.5f; ctx.beginPath(); if (vertical) { ctx.moveTo(q, 0.f); ctx.lineTo(q, 200.f); ctx.moveTo(q + 14.f, 0.f); ctx.lineTo(q + 14.f, 200.f); } else { ctx.moveTo(0.f, q); ctx.lineTo(200.f, q); ctx.moveTo(0.f, q + 14.f); ctx.lineTo(200.f, q + 14.f); } ctx.stroke(); } }
                  else if (cd.pattern != Throwables::kPlain) for (int row = 0; row < 7; row++) for (int col = 0; col < 7; col++) { const float x = float(col) * 38.f + (row % 2 ? 19.f : 0.f), y = float(row) * 34.f + 6.f;   // grade intercalada
                      if (cd.pattern == Throwables::kDots) dot(x, y, 8.f); else if (cd.pattern == Throwables::kHearts) heart(x, y, 12.f); else bow(x, y, 10.f); }
                  ctx.restore(); } }
              rt.display(); atlasImg = rt.getTexture().copyToImage(); }
        }
        sf::ContextSettings cs; cs.depthBits = 24; cs.majorVersion = 3; cs.minorVersion = 3; cs.antiAliasingLevel = 0;
        sf::RenderWindow win(sf::VideoMode({1280, 720}), "K.O.-RAMA 3D (teste)", sf::Style::Default, sf::State::Windowed, cs);
        if (snap) win.setVisible(false);
        win.setVerticalSyncEnabled(!snap);
        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(sf::Context::getFunction))) throw std::runtime_error("OpenGL 3.3 indisponível");
        const std::string root = KORAMA_ROOT;
        sf::Font font; if (!font.openFromFile(root + "/engine-cpp/assets/fonts/Bangers-Regular.ttf")) throw std::runtime_error("fonte Bangers não abriu");
        Renderer rend; rend.init();
        unsigned undiesTex = atlasImg.getSize().x ? rend.texture(atlasImg.getPixelsPtr(), int(atlasImg.getSize().x), int(atlasImg.getSize().y)) : 0;   // atlas feito antes da janela (ver acima)
        // fontes e sprites do jogo original: o menu e a HUD da luta usam os mesmos
        const std::filesystem::path assets = std::filesystem::path(root) / "engine-cpp" / "assets";
        SpriteLibrary sprites; sprites.loadMenuSheets(assets / "sprites"); sprites.keepRigs({}); sprites.setRigDir(assets / "sprites"); ui::loadFonts(assets / "fonts");
        sf::Font digits; if (!digits.openFromFile(assets / "fonts" / "PressStart2P-Regular.ttf")) throw std::runtime_error("fonte PressStart2P não abriu");
        RingRenderer hudRenderer(HudFonts{&font, &digits}, &sprites); RingRenderer* hud2d = &hudRenderer;   // a HUD da luta do jogo original
        // ---------------- som: o mesmo AudioSystem do jogo 2D (músicas, narrador, golpes, torcida, menu) ----------------
        const bool audioOn = !snap && !(argc >= 2 && (std::string(argv[1]) == "--bench" || std::string(argv[1]) == "--menu"));
        AudioSystem audio; if (audioOn) audio.init(std::filesystem::path(root) / "engine-cpp" / "assets");
        // música de cada lutador; o Batalhator troca de faixa a cada fase
        const auto charTrack = [](uint32_t c, int bossStage) {
            switch (c) { case 0: return MusicTrack::Jack; case 1: return MusicTrack::KingFrog; case 2: return MusicTrack::Joana; case 3: return MusicTrack::Cremoso; case 4: return MusicTrack::Valeria; case 5: return MusicTrack::Vivaldi; case 6: return MusicTrack::Espinha; case 7: return MusicTrack::Marcov;
            case 8: return bossStage == 3 ? MusicTrack::BossPhase3 : bossStage == 2 ? MusicTrack::BossPhase2 : MusicTrack::BossPhase1; default: return MusicTrack::Fight; } };
        int endFrames = 0; bool celebrated = false;   // fim da luta: a música desacelera e depois entra a do vencedor
        // modelos carregados uma vez e guardados; o mesmo modelo serve aos dois lados (a pose é aplicada na hora de desenhar)
        std::map<std::string, std::unique_ptr<Model>> models;
        const auto modelOf = [&](const std::string& name) -> Model& { auto& m = models[name]; if (!m) { m = std::make_unique<Model>(); m->load(root + "/assets3d/game/" + name + ".glb"); if (name == "frog" || name == "cremoso") m->scale = 1.f;
            if (name == "frog") for (Node& n : m->nodes) for (Prim& pr : n.prims) {   // tinta da Pele Venenosa por peça (como no vinil)
                const auto near = [&](float r, float g, float b) { return glm::length(glm::vec3(pr.color) - glm::vec3(r, g, b)) < 0.06f; };
                pr.pz = pr.faceSlot == 1 || pr.faceSlot == 2 ? 6 : pr.faceSlot == 0 ? 1 : pr.vcol ? 2 : pr.tex ? 1 : near(0.78f, 0.6f, 0.1f) ? 4 : near(0.58f, 0.03f, 0.05f) ? 5 : near(0.05f, 0.13f, 0.05f) || near(0.02f, 0.06f, 0.02f) ? 3 : 0; }
            if (name == "jack") for (Node& n : m->nodes) for (Prim& pr : n.prims) if (pr.faceSlot == 0) pr.pz = 9; else if (glm::length(glm::vec3(pr.color) - glm::vec3(0.79f, 0.72f, 0.52f)) < 0.05f) pr.pz = 7;   /* 9: o rosto (a sombra dele já vem pintada) */   // Pele de Mel: rosto e mãos
            if (name == "jack") m->addFace("molasses", 0, [](unsigned w, unsigned h, std::uint8_t* px) { HoneyFx::paintMolasses(w, h, px); });   // Melaço: o rosto sombreado (misturado por cima do normal, em fade)   // Melaço: sombra nos olhos e olhar ciano
            if (name == "cremoso") for (int k = 1; k <= 3; k++)   // Cereja do Bolo: olhos apertando (1, 2) e a boca aberta da cuspida (3)
                m->addFace("spit" + std::to_string(k), 0, [k](unsigned w, unsigned h, std::uint8_t* px) {
                    // acha os dois olhos (manchas escuras) e a cor da pele
                    int x0[2] = {int(w), int(w)}, x1[2] = {-1, -1}, y0[2] = {int(h), int(h)}, y1[2] = {-1, -1};
                    for (unsigned y = 0; y < h; y++) for (unsigned x = 0; x < w; x++) { const std::uint8_t* p = px + (size_t(y) * w + x) * 4; if (p[0] + p[1] + p[2] < 200 && p[3] > 200) { const int e = x < w / 2 ? 0 : 1; x0[e] = std::min(x0[e], int(x)); x1[e] = std::max(x1[e], int(x)); y0[e] = std::min(y0[e], int(y)); y1[e] = std::max(y1[e], int(y)); } }
                    const std::uint8_t* sk = px + (size_t(h / 8) * w + w / 2) * 4; const std::uint8_t skin[3] = {sk[0], sk[1], sk[2]};
                    const auto put = [&](int x, int y, std::uint8_t r, std::uint8_t g, std::uint8_t b) { if (x < 0 || y < 0 || x >= int(w) || y >= int(h)) return; std::uint8_t* q = px + (size_t(y) * w + size_t(x)) * 4; q[0] = r; q[1] = g; q[2] = b; q[3] = 255; };
                    const float sq = k == 1 ? 0.35f : k == 2 ? 0.6f : 0.72f;   // quanto do olho a pálpebra cobre (de cima e de baixo)
                    for (int e = 0; e < 2; e++) { if (x1[e] < 0) continue; const int H = y1[e] - y0[e], cut = int(float(H) * sq * 0.5f);
                        for (int y = y0[e] - 2; y <= y1[e] + 2; y++) for (int x = x0[e] - 2; x <= x1[e] + 2; x++) { const bool lid = y < y0[e] + cut || y > y1[e] - cut; if (lid) put(x, y, skin[0], skin[1], skin[2]); }
                        for (int x = x0[e] - 3; x <= x1[e] + 3; x++) for (int d = -3; d <= 3; d++) { put(x, y0[e] + cut + d, 40, 26, 20); }   // a linha da pálpebra apertada
                    }
                    if (k == 3 && x1[0] >= 0 && x1[1] >= 0) {   // a boca aberta, abaixo dos olhos (centro entre eles)
                        const float cx = float(x0[0] + x1[1]) * 0.5f, ey = float(std::min(y0[0], y0[1])), H = float(y1[0] - y0[0]), cy = ey - H * 1.15f,   /* a textura é de cabeça para baixo: abaixo do bigode */ rx = H * 0.42f, ry = H * 0.62f;   /* um "O" em pé */
                        for (int y = int(cy - ry - 8); y <= int(cy + ry + 8); y++) for (int x = int(cx - rx - 8); x <= int(cx + rx + 8); x++) { const float u = (float(x) - cx) / rx, v = (float(y) - cy) / ry, d = u * u + v * v;
                            if (d < 1.f) { const bool tongue = false; put(x, y, tongue ? 230 : 70, tongue ? 90 : 16, tongue ? 110 : 24); } else if (d < 1.f + 16.f / rx) put(x, y, 35, 20, 16); } }
                });
            if (name == "frog") { m->addFace("spitO", 0, [](unsigned w, unsigned h, std::uint8_t* px) { TongueFx::paintMouthO(w, h, px); }); for (int sl = 1; sl <= 2; sl++) { const auto a = m->faces.find("angry_" + std::to_string(sl)); if (a != m->faces.end()) m->faces["spitO_" + std::to_string(sl)] = a->second; } }
            if (name == "frog") for (int k = 1; k <= 2; k++) { const std::string f = "tongue" + std::to_string(k); m->addFace(f, 0, [k](unsigned w, unsigned h, std::uint8_t* px) { TongueFx::paintMouth(w, h, px, k == 2 ? 1.f : 0.45f); });
                for (int sl = 1; sl <= 2; sl++) { const auto a = m->faces.find("angry_" + std::to_string(sl)); if (a != m->faces.end()) m->faces[f + "_" + std::to_string(sl)] = a->second; } } /* King Frog e Cremoso na altura dos outros (o .glb pede 1.18) */ } return *m; };
        const auto topOf = [](const Model& m) { return m.headTop(); };
        // raio do tronco para as cordas: metade da largura das peças de cima do corpo (sem as luvas), na escala do modelo
        std::map<const Model*, float> ropeRadii;
        const auto ropeRadius = [&](const Model& m) -> float { auto it = ropeRadii.find(&m); if (it != ropeRadii.end()) return it->second;
            float lo = 1e9f, hi = -1e9f; const auto walk = [&](auto&& self, int i, glm::vec3 off) -> void { const Node& n = m.nodes[size_t(i)]; if (n.name.rfind("glove", 0) == 0) return; off += n.pos;
                for (const Prim& pr : n.prims) { lo = std::min(lo, off.x + pr.lo.x); hi = std::max(hi, off.x + pr.hi.x); } for (const int k : n.kids) self(self, k, off); };
            if (m.byName.count("upper")) walk(walk, m.need("upper"), glm::vec3(0.f));
            const float r = hi > lo ? std::clamp(0.42f * (hi - lo) * m.scale, 0.22f, 0.7f) : 0.32f; ropeRadii[&m] = r; return r; };
        // efeitos dos especiais gravados do vinil (fx_<lutador>.glb + .kfx); sem os arquivos, ficam os efeitos de render/skill_fx.hpp
        std::map<uint32_t, std::unique_ptr<FxLibrary>> fxLibs;
        const auto fxLibOf = [&](uint32_t c) -> const FxLibrary* { c = std::min(c, 8u); auto& lib = fxLibs[c]; if (lib) return lib->model ? lib.get() : nullptr; lib = std::make_unique<FxLibrary>();
            const std::string base = root + "/assets3d/game/fx_" + kFiles[c]; if (!std::filesystem::exists(base + ".glb") || !std::filesystem::exists(base + ".kfx")) return nullptr;
            if (!lib->load(base + ".kfx", modelOf(std::string("fx_") + kFiles[c]))) { lib->model = nullptr; return nullptr; }
            if (c == 3) lib->hide({165, 166, 238, 239, 229, 230, 231, 235, 236}); if (c == 7) lib->hide({162, 10, 11});   /* discos e casquinhas do Cremoso, bolha e chamas brancas do Marcov: refeitos em render/char_fx.hpp */ return lib.get(); };
        std::map<const Model*, RigRest> rests;
        const auto restFor = [&](const Model& m) -> const RigRest& { auto it = rests.find(&m); if (it == rests.end()) it = rests.emplace(&m, restOf(m)).first; return it->second; };
        const auto fighterModel = [&](const Fighter& f) -> Model& { const std::string base = kFiles[std::min(f.charId, 8u)]; return modelOf(f.charId == 8 ? base + std::to_string(std::clamp(f.bossStage, 1, 3)) : base); };
        int charP = 4, charE = 4; if (const char* v = std::getenv("KORAMA3D_P")) charP = std::atoi(v); if (const char* v = std::getenv("KORAMA3D_E")) charE = std::atoi(v);
        const std::filesystem::path buildFile = std::filesystem::absolute(std::filesystem::path(argv[0])).parent_path() / "korama_save.txt"; Builds builds = loadBuilds(buildFile);
        SkillLoadout matchLoadout = builds[size_t(std::clamp(charP, 0, 8))]; bool matchArcade = false, matchTraining = false; int matchStage = 1;   // o que o menu escolheu
        GameEngine engine; Model* arena = nullptr;
        FootStepper feetP, feetE; LagSpring lagP, lagE; ClothSim clothP, clothE; FighterPose poseP, poseE; RingRopes ropes; float pull = 0; ImpactSpring bounceP, bounceE; uint32_t ropePrev = 0; int mashPrev = 0;
        float downP = 0, downE = 0, riseP = 0;
        ImpactSpring wobP, wobE; int hurtP = 0, hurtE = 0;   // wob: mola mais solta que a do impacto, gravada para o replay balançar o corpo; hurt: quadros de rosto de dor depois de um golpe
        const auto faceNow = [&](const Fighter& f, float down, int mouth, int hurt) { const char* c = faceOf(f, down, mouth); return hurt > 0 && down <= 0.5f && !mouth ? "pain" : c; };   // todo golpe que tira vida dói, também o do nocaute e os especiais
        ImpactSpring springP, springE; float hpP = 100, hpE = 100; int freeze = 0, stopPrev = 0; uint32_t pending = 0; ImpactFx impact; Particles sparks; std::deque<ReplayFrame> rec; bool recHit = false; int replayT = -1, replayStart = 0, replayEnd = 0, replayHit = 0, shot = -1, shotT = 0; float replayPos = 0.f; bool replayDone = false;   // replay do nocaute
        int finisher = -1; float shakeExtra = 0.f; bool downPrev = false; float shockProg = 2.f, shockStr = 0.f; glm::vec3 shockAt{0.f}; int kickT = -1; float kickAmp = 0.f; glm::vec3 kickDir{0.f};   // balançada da câmera   // finisher: quadros desde o golpe que derrubou (-1: nenhum)
         SkillFx skillFx; SpeedLines speedLines; Renderer::Smear smP, smE; glm::vec3 prevGlove[2][2]{}; bool smearReady = false; /* rastro de quadrinho */ FxSet fxs, replayFx; FxPlayer &fxP = fxs.fxP, &fxE = fxs.fxE; WaveFx &waveP = fxs.waveP, &waveE = fxs.waveE; std::vector<float> waveV; std::vector<unsigned> waveI; LeapFx leapP, leapE; TongueFx &tongueP = fxs.tongueP, &tongueE = fxs.tongueE; GeyserFx &geyP = fxs.geyP, &geyE = fxs.geyE; CloudFx &cloudP = fxs.cloudP, &cloudE = fxs.cloudE; StabFx &stabP = fxs.stabP, &stabE = fxs.stabE; StingFx &stingP = fxs.stingP, &stingE = fxs.stingE; SpitFx &spitP = fxs.spitP, &spitE = fxs.spitE; HoneyFx &honeyP = fxs.honeyP, &honeyE = fxs.honeyE; std::vector<float> honV, honGV; std::vector<unsigned> honI, honGI; std::vector<float> stabV; std::vector<unsigned> stabI;   /* golpes de nuvem: pólen, cinzas, exaustão */ float &poisonP = fxs.poisonP, &poisonE = fxs.poisonE;   /* Pele Venenosa: entra 0,25 s depois do golpe e sai devagar */
        // comemoração do fim da luta (showVictoryRingCelebration do jogo original): o perdedor sai, o campeão dança no centro do ringue com a câmera
        // girando devagar, a torcida joga rosas (e de vez em quando uma peça íntima) e o card de quadrinho entra pela direita
        
        int celebF = -1; float celebA0 = 0.f; Throwables throws; int throwWait = 0; std::vector<float> propV; std::vector<unsigned> propI; constexpr int kCardDelay = 30;   /* onda da âncora da Joana */ int hitCount = 0; PoseSmoother smoothP, smoothE;
        // prévia da build (tela de montar a build): uma luta de mentira por trás do menu, com o golpe escolhido saindo em laço, desenhada num quadro
        bool previewing = false, previewEnemyCam = false; sf::Texture previewTex; SkillId previewSkillCur = SkillId::None; int previewWait = 0;
        // soco reto: enquanto o oponente está no soco ou no especial, ele fica no lugar e na direção em que começou. Se a jogadora esquiva (o ringue gira
        // com ela e com a câmera), o golpe passa reto e ela aparece de lado para ele; quando o golpe acaba ele gira e volta a ficar de frente. No aviso
        // (antes do soco) ele ainda acompanha. É só desenho: o acerto continua decidido pelo motor.
        float turnLagE = 0.f, ringPrev = 0.f, yawOffP = 0.f, yawOffE = 0.f;   // turnLagE: quanto o oponente ficou para trás do giro do ringue; yawOff: giro de cada um por cima da direção do motor
        const auto tickSmooth = [&](bool reset) {
            const float ring = engine.ringAngle; if (reset) { turnLagE = yawOffP = yawOffE = 0.f; ringPrev = ring; }
            const float d = ring - ringPrev; ringPrev = ring;
            const Fighter& e = engine.enemy; const bool committed = engine.gameState == STATE_FIGHTING && (isPunchState(e.state) || e.state == FIGHTER_SKILL);
            if (committed) turnLagE = std::clamp(turnLagE - d, -1.75f, 1.75f); else { turnLagE *= 0.86f; if (std::fabs(turnLagE) < 0.002f) turnLagE = 0.f; }
            Fighter fp = engine.player, fe = e;
            if (turnLagE != 0.f) { const float r = std::hypot(fe.worldPos.x, fe.worldPos.z), a = std::atan2(fe.worldPos.x, fe.worldPos.z) + turnLagE; fe.worldPos.x = r * std::sin(a); fe.worldPos.z = r * std::cos(a); }
            // cada um olha para onde o outro está de fato; o oponente no golpe mantém a direção que tinha
            const auto wrap = [](float a) { while (a > 3.14159265f) a -= 6.2831853f; while (a < -3.14159265f) a += 6.2831853f; return a; };
            const glm::vec3 pp = rigPos(fp), pe = rigPos(fe); const glm::vec2 dir(pe.x - pp.x, pe.z - pp.z); const bool far = glm::length(dir) > 0.05f;
            const float wantP = far ? wrap(std::atan2(-dir.x, dir.y) - fp.facing) : 0.f, wantE = far ? wrap(std::atan2(dir.x, -dir.y) - fe.facing) : 0.f;
            yawOffP += (wantP - yawOffP) * 0.3f; if (committed) yawOffE = wrap(yawOffE - d); else yawOffE += (wantE - yawOffE) * 0.3f;
            if (turnLagE == 0.f && !committed) { if (std::fabs(yawOffP) < 0.002f) yawOffP = 0.f; if (std::fabs(yawOffE) < 0.002f) yawOffE = 0.f; }
            fp.facing += yawOffP; fe.facing += yawOffE;
            smoothP.tick(fp); smoothE.tick(fe); };
        const auto startFight = [&]() {
            engine.startMatch(uint32_t(charP), uint32_t(charE), matchArcade, matchStage, snap ? nullptr : &matchLoadout, matchTraining); engine.startRoundIntro(324); engine.update(0, 0.f);
            endFrames = 0; celebrated = false;
            if (audioOn && !previewing) { audio.setFightPaused(false); audio.stopVersusSounds(); audio.stopFightSounds(); audio.setMusicPitch(1.f); audio.setMusicDuck(1.f); audio.play(Sfx::Bell); audio.play(Sfx::AnnouncerKorama); audio.setMusic(charTrack(engine.enemy.charId, engine.enemy.bossStage)); }
            if (snap && std::getenv("KORAMA3D_WEAK_PLAYER")) engine.player.hp = 3.f;   // conferência: a jogadora cai no primeiro golpe
            if (const char* ring = snap ? std::getenv("KORAMA3D_RING") : nullptr) engine.ringAngle = float(std::atof(ring));   // conferência: ângulo do ringue (quina em 0,785)
            if (snap && std::getenv("KORAMA3D_WEAK_ENEMY")) engine.enemy.hp = 5.f;
            if (const char* pk = snap ? std::getenv("KORAMA3D_PASSIVE") : nullptr) if (const SkillDef* d = SkillSystem::getByKey(pk)) engine.player.loadout.passive = d->id;   // conferência: passiva do jogador
            if (snap && std::getenv("KORAMA3D_LAST_KD")) engine.enemyKnockdowns = GameEngine::kKnockdownsForTko - 1;   // conferência: a próxima queda encerra a luta   // conferência: derruba a oponente no primeiro golpe
            arena = &modelOf(std::string("arena_") + kArenas[charE]); ropes = {}; rend.setArena(*arena, ropes);
            downP = downE = riseP = pull = 0; impact = {}; sparks.clear(); rec.clear(); recHit = false; replayT = -1; replayDone = false; shot = -1; finisher = -1; shakeExtra = 0.f; downPrev = false; shockProg = 2.f; kickT = -1; skillFx.reset(); speedLines.reset(); smP = {}; smE = {}; smearReady = false; fxP = {}; fxE = {}; waveP = {}; waveE = {}; leapP = {}; leapE = {}; tongueP.reset(); tongueE.reset(); geyP.reset(); geyE.reset(); cloudP.reset(); cloudE.reset(); stabP.reset(); stabE.reset(); stingP.reset(); stingE.reset(); spitP.reset(); spitE.reset(); honeyP = {}; honeyE = {}; fxs.beeP = {}; fxs.beeE = {}; fxs.meltP = {}; fxs.meltE = {}; fxs.coneP = {}; fxs.coneE = {}; fxs.bubP = {}; fxs.bubE = {}; fxs.lasP = {}; fxs.lasE = {}; fxs.jetP = {}; fxs.jetE = {}; fxs.syrP = {}; fxs.syrE = {}; fxs.heatP = {}; fxs.heatE = {}; celebF = -1; throws.clear(); rend.setFlash(glm::vec3(0.f), 0.f); feetP = {}; feetE = {}; lagP = {}; lagE = {}; clothP = {}; clothE = {}; bounceP = {}; bounceE = {}; ropePrev = 0; mashPrev = 0; freeze = 0; stopPrev = 0; pending = 0;
            smoothP.ready = smoothE.ready = false; tickSmooth(true); hpP = engine.player.hp; hpE = engine.enemy.hp; springP = {}; springE = {}; wobP = {}; wobE = {}; hurtP = hurtE = 0;
        };
        const auto step = [&](uint32_t mask) {
            impact.tick(); sparks.tick();   // o golpe anima mesmo com a luta congelada
            if (finisher >= 0 && ++finisher == kFinisherFreeze) shakeExtra = 34.f;   // solta: tranco na câmera e o lutador voa
            shakeExtra *= 0.9f; if (kickT >= 0 && ++kickT > 90) kickT = -1; if (shockProg <= 1.f) shockProg += 0.045f;   // a onda de choque anda mesmo com a luta congelada
            // congelado: guarda o que foi apertado para o motor receber quando voltar
            if (freeze > 0) { freeze--; pending |= mask; return; }
            engine.update(mask | pending, float(kTick)); pending = 0;
            { static bool stretched[2] = {false, false}; int k = 0;   // cuspidas: o 3D dá mais tempo de carga ao golpe (o acerto continua no mesmo % dele)
              for (Fighter* f : {&engine.player, &engine.enemy}) { const int st = SpitFx::styleOf(*f); if (st == SpitFx::kNone) stretched[k] = false; else if (!stretched[k]) { const int add = SpitFx::look(st).stretch; f->maxTimer += add; f->timer += add; stretched[k] = true; } k++; } }
            if (engine.isMatchOver()) endFrames++;
            { const MatchResult r = engine.result; const bool ko = r == MatchResult::WinKO || r == MatchResult::WinTKO || r == MatchResult::LossKO || r == MatchResult::LossTKO;
              if (engine.isMatchOver() && endFrames >= 150 && (replayDone || !ko)) celebF++; else { celebF = -1; throws.clear(); throwWait = 0; } }
            if (celebF >= 0) {   // rosas (e de vez em quando uma calcinha ou cueca) voando da arquibancada em arco (render/throwables.hpp)
                if (!throws.ready) throws.buildRoses(); throws.ropes = &ropes;
                static const int undiesDbg = std::getenv("KORAMA3D_UNDIES") ? std::atoi(std::getenv("KORAMA3D_UNDIES")) : 0; const float undies = undiesDbg ? (undiesDbg >= 2 ? 1.f : 0.5f) : 0.025f;   // KORAMA3D_UNDIES: conferência (1 metade vira roupa, 2 tudo roupa nas cordas)
                if (const char* gd = std::getenv("KORAMA3D_GARMENT")) { if (celebF == 0) { throws.clear(); const int c0 = std::atoi(gd); throws.drop(c0 % 12, glm::vec3(-1.75f, 1.2f, -1.7f), 0.3f); throws.drop((c0 + 6) % 12, glm::vec3(-0.85f, 1.2f, -1.7f), -0.3f); } throwWait = 1 << 30; }   // conferência: uma calcinha e uma cueca caindo
                if (--throwWait <= 0) { throws.spawn(throws.rnd(0.f, 1.f) < undies, undiesDbg >= 2); throwWait = int(throws.rnd(15.f, 33.f)); }   // uma de cada vez, sem parar: sempre tem alguma no ar
                throws.tick(ropes, &sparks); }
            if (audioOn && !previewing) {
                audio.play(engine.sounds);
                if (engine.enemy.charId == 8 && !engine.isMatchOver()) { const MusicTrack bt = charTrack(8, engine.enemy.bossStage); if (audio.currentTrack() != bt) audio.setMusic(bt); }
                if (engine.isMatchOver()) {   // fita desacelerando enquanto o perdedor cai; depois a música do vencedor e a torcida
                    const MatchResult r = engine.result; const bool won = r == MatchResult::WinKO || r == MatchResult::WinTKO || r == MatchResult::WinDecision;
                    if (endFrames < 150) { const float q = float(endFrames) / 150.f; audio.setMusicPitch(std::max(0.28f, 1.f - 0.72f * q * q)); audio.setMusicDuck(std::max(0.18f, 1.f - 0.82f * q)); }
                    else if (!celebrated && (replayDone || !(r == MatchResult::WinKO || r == MatchResult::WinTKO || r == MatchResult::LossKO || r == MatchResult::LossTKO))) { celebrated = true; audio.setMusicPitch(1.f); audio.setMusicDuck(1.f); audio.setMusic(charTrack(won ? engine.player.charId : engine.enemy.charId, engine.enemy.bossStage));
                        if (won) { audio.play(Sfx::CrowdVictory); audio.play(Sfx::CameraFlash); } else if (r == MatchResult::Draw) audio.play(Sfx::CrowdWoah); else audio.play(Sfx::CrowdBoo); }
                    else if (won && endFrames % 240 == 120) audio.play(Sfx::CrowdCheer);
                }
            }
            tickSmooth(false);
            if (engine.hitstopTimer > stopPrev) freeze = kExtraFreeze;
            stopPrev = engine.hitstopTimer;
            const auto hit = [](ImpactSpring& sp, float& prev, float hp) { const float dmg = prev - hp; if (dmg >= 2.f) sp.kick(std::min(1.f, 0.55f + dmg * 0.035f)); prev = hp; return dmg; };
            const float dmgP = hit(springP, hpP, engine.player.hp), dmgE = hit(springE, hpE, engine.enemy.hp);
            if (dmgP >= 2.f) { hurtP = 28; wobP.kick(std::min(1.f, 0.6f + dmgP * 0.04f)); } if (dmgE >= 2.f) { hurtE = 28; wobE.kick(std::min(1.f, 0.6f + dmgE * 0.04f)); }
            // o motor só classifica o golpe do jogador (impactKind); o do oponente vai pelo dano
            if (dmgE >= 2.f) impact.spawn(engine.impactTimer > 0 ? engine.impactKind : (dmgE >= 10.f ? 1 : 0), ++hitCount, true);
            else if (dmgP >= 2.f) impact.spawn(dmgP >= 10.f ? 1 : 0, ++hitCount, false);
            // o golpe que derruba ou nocauteia: congela mais, quadro de impacto, câmera avançando e o estouro grande
            { const MatchResult r = engine.result; const bool ko = engine.isMatchOver() && (r == MatchResult::WinKO || r == MatchResult::WinTKO || r == MatchResult::LossKO || r == MatchResult::LossTKO);
              const bool downNow = engine.gameState == STATE_KNOCKDOWN || ko;
              if (downNow && !downPrev) { const bool enemyFell = engine.gameState == STATE_KNOCKDOWN ? engine.enemyIsDown : (r == MatchResult::WinKO || r == MatchResult::WinTKO);
                  finisher = 0; freeze = kFinisherFreeze; impact.spawn(4, ++hitCount, enemyFell); if (ko) recHit = true; if (audioOn) audio.play(Sfx::CameraFlash); }
              downPrev = downNow; }
            if (engine.hitstopTimer == 0 && freeze == 0) { springP.tick(); springE.tick(); if (hurtP > 0) hurtP--; if (hurtE > 0) hurtE--;
                for (ImpactSpring* w : {&wobP, &wobE}) { w->v += -0.05f * w->s; w->v *= 0.93f; w->s += w->v; } }   // balança umas três vezes antes de parar
            const bool over = engine.isMatchOver(), kd = engine.gameState == STATE_KNOCKDOWN;
            const bool pLost = over && (engine.result==MatchResult::LossKO||engine.result==MatchResult::LossTKO), eLost = over && (engine.result==MatchResult::WinKO||engine.result==MatchResult::WinTKO);
            // apertar para levantar: ela força o corpo para cima e dá um tranco
            const float mash = !engine.enemyIsDown && engine.mashTarget > 0 ? std::min(1.f, float(engine.mashProgress) / float(engine.mashTarget)) : 0.f;
            if (kd && !engine.enemyIsDown && engine.mashProgress > mashPrev) bounceP.kick(0.35f);
            mashPrev = engine.mashProgress;
            riseP = approach(riseP, kd && !engine.enemyIsDown ? (snap && std::getenv("KORAMA3D_RISE") ? 1.f : mash) : 0.f, 0.08f);   // KORAMA3D_RISE: conferência, como se apertasse sem parar   // apertar para levantar vira esforço, não meia-pose de pé
            downP = approach(downP, (kd && !engine.enemyIsDown) || pLost ? 1.f : 0.f, 0.05f); downE = approach(downE, (kd && engine.enemyIsDown) || eLost ? 1.f : 0.f, 0.05f);
            pull = approach(pull, downP > 0.f ? 1.f : 0.f, 0.03f);
            // bateu nas cordas: rebate para a frente e volta balançando
            if (engine.koRopeFrame != 0 && ropePrev == 0) { if (downP >= downE) bounceP.kick(1.f); else bounceE.kick(1.f); sparks.burst((downP >= downE ? smoothP.pos : smoothE.pos) + glm::vec3(0, 0.9f, 0), 1, uint32_t(++hitCount)); shakeExtra = std::max(shakeExtra, 22.f); }
            ropePrev = engine.koRopeFrame; bounceP.tick(); bounceE.tick();
            // quem encosta nas cordas é o tronco; escorado, ele empurra mais
            // cada corda vê o tronco na altura dela: escorado, ele inclina para trás (vai mais para fora em cima); o tranco nas cordas o traz de volta
            const auto torso = [&](const PoseSmoother& f, float d, float rebound, const Model& m) { const glm::vec3 p = f.pos; const glm::vec2 out = glm::normalize(glm::vec2(p.x, p.z) + glm::vec2(1e-4f));
                RopeBody b; b.base = glm::vec2(p.x, p.z) - out * (0.32f * rebound * d); b.lean = out * (0.48f * d); b.r = ropeRadius(m); return b; };
            const RopeBody bodies[2] = {torso(smoothP, downP, bounceP.s, fighterModel(engine.player)), torso(smoothE, downE, bounceE.s, fighterModel(engine.enemy))}; ropes.tick(bodies, 2);
            Model& mP = fighterModel(engine.player); Model& mE = fighterModel(engine.enemy); const RigRest& restP = restFor(mP); const RigRest& restE = restFor(mE);
            PoseCtx cP, cE; cP.reachLimit = reachOf(smoothP, smoothE, springE.s, mP.num("gloveTip", 0.2f) * mP.scale); cE.reachLimit = reachOf(smoothE, smoothP, springP.s, mE.num("gloveTip", 0.2f) * mE.scale); cP.hangT = cE.hangT = float(engine.frameIndex); cP.ropeBounce = bounceP.s; cP.rise = riseP; cE.ropeBounce = bounceE.s; cP.ropeTop = cE.ropeTop = ropes.heights[std::max(0, ropes.rows - 1)]; cP.hasPos = cE.hasPos = true; cP.pos = smoothP.pos; cE.pos = smoothE.pos; cP.squash = springP.s; cE.squash = springE.s;
            tongueP.tick(engine.player); tongueE.tick(engine.enemy);
            { const auto pz = [](float& v, const Fighter& f) { const bool want = f.charId == 1 && (f.status.poisonSkin > 0 && !(f.state == FIGHTER_SKILL && f.activeSkill == SkillId::FrogPoisonSkin && skillElapsed(f) < 15.f)); v += ((want ? 1.f : 0.f) - v) * (want ? 0.04f : 0.03f); };
              pz(poisonP, engine.player); pz(poisonE, engine.enemy); }
            { // saltos: o desenho refaz a altura com preparo e aterrissagem (render/leap_fx.hpp)
              Fighter vP = smoothP.view, vE = smoothE.view; LeapFx::Shape shP, shE; const bool lP = leapP.apply(vP, engine.player, shP), lE = leapE.apply(vE, engine.enemy, shE);
              // língua do King Frog: as luvas ficam na guarda (sem o jab do motor) e o tronco gira levando a chicotada
              CloudFx::hands(vP, engine.player); CloudFx::hands(vE, engine.enemy); StabFx::pose(vP, engine.player); StabFx::pose(vE, engine.enemy); StingFx::pose(vP, engine.player); StingFx::pose(vE, engine.enemy); SpitFx::pose(vP, engine.player); SpitFx::pose(vE, engine.enemy); ConeFx::pose(vP, engine.player); ConeFx::pose(vE, engine.enemy); MaestraFx::pose(vP, engine.player); MaestraFx::pose(vE, engine.enemy);   // estacas: bate no chão / rege   // explosão de pólen: as mãos giram
              if (TongueFx::active(engine.player)) { vP.gloveL = vP.baseGloveL; vP.gloveR = vP.baseGloveR; } if (TongueFx::active(engine.enemy)) { vE.gloveL = vE.baseGloveL; vE.gloveR = vE.baseGloveR; }
              poseP = computePose(vP, restP, downP, cP); poseE = computePose(vE, restE, downE, cE); poseP.twist += tongueP.twist(); poseE.twist += tongueE.twist();
              for (auto [sp, q] : {std::pair<const SpitFx*, FighterPose*>{&spitP, &poseP}, {&spitE, &poseE}}) if (sp->on()) { if (sp->style == SpitFx::kCherry) { const float i = sp->inflate(); q->headScale *= glm::vec3(1.f + 0.14f * i, 1.f + 0.06f * i, 1.f + 0.12f * i); }
                  const SpitFx::Body b = sp->body(); q->lean += b.lean; q->neckTilt += b.neck; q->root = q->root * glm::scale(glm::mat4(1.f), b.scale); }   // o corpo acompanha a carga e a cuspida   // Cremoso: a cabeça incha com a cereja
              fxs.meltP.apply(poseP); fxs.meltE.apply(poseE);   // Cremoso derretido: o corpo achata
              if (lP) poseP.root = poseP.root * glm::scale(glm::mat4(1.f), shP.scale); if (lE) poseE.root = poseE.root * glm::scale(glm::mat4(1.f), shE.scale); } poseP.gloveOpen = smoothP.open; poseE.gloveOpen = smoothE.open; for (int i = 0; i < 2; i++) { poseP.gloveSpin[i] = smoothP.spin[i]; poseE.gloveSpin[i] = smoothE.spin[i]; }
            feetP.tick(poseP, restP); feetE.tick(poseE, restE);
            lagP.tick(glm::vec3(poseP.root * glm::vec4(poseP.upperPos, 1))); lagE.tick(glm::vec3(poseE.root * glm::vec4(poseE.upperPos, 1)));
            { const float t = float(engine.frameIndex) / 60.f; clothP.tick(glm::vec3(poseP.root * glm::vec4(poseP.upperPos, 1)), poseP.root, poseP.twist, t); clothE.tick(glm::vec3(poseE.root * glm::vec4(poseE.upperPos, 1)), poseE.root, poseE.twist, t + 1.3f); }
            // o ponto do golpe é a ponta da luva de quem bateu, no momento do acerto
            if (impact.alive && !impact.placed) { const Fighter& atk = impact.byPlayer ? engine.player : engine.enemy; const FighterPose& pa = impact.byPlayer ? poseP : poseE; const glm::vec3 g = pa.glove[atk.isLeftPunch ? 1 : 0]; impact.at = glm::vec3(pa.root * glm::vec4(g.x, g.y, g.z + (impact.byPlayer ? mP : mE).num("gloveTip", 0.2f), 1)); if (StabFx::active(atk) || SpitFx::active(atk)) impact.at = glm::vec3((impact.byPlayer ? poseE : poseP).root[3]) + glm::vec3(0, 0.9f, 0); /* estacas: o golpe é no rival */ impact.placed = true; sparks.burst(impact.at, impact.kind, uint32_t(impact.seed));
                { const float amp = impact.kind == 0 ? 0.25f : impact.kind == 1 ? 0.8f : impact.kind == 4 ? 1.6f : 1.1f;   // golpe forte: a câmera balança na direção dele
                  if (amp >= kickAmp * std::exp(-0.16f * float(std::max(kickT, 0))) || kickT < 0) { kickT = 0; kickAmp = amp; const glm::vec3 d = smoothE.pos - smoothP.pos; kickDir = glm::normalize(glm::vec3(d.x, 0.f, d.z) + glm::vec3(0, 0, 1e-4f)) * (impact.byPlayer ? 1.f : -1.f); } }
                if (impact.kind >= 2) { shockProg = 0.f; shockStr = impact.kind == 4 ? 1.f : impact.kind == 2 ? 0.85f : 0.65f; shockAt = impact.at; }   // contra-ataque, brecha e queda: onda de choque
                if (impact.kind == 4) for (int i = 0; i < 44; i++) { const glm::vec3 d = sparks.dir(); Particle q; q.kind = kLine; q.p = impact.at + d * 0.15f; q.v = d * sparks.rnd(0.09f, 0.18f); q.dir = d; q.len = sparks.rnd(0.25f, 0.6f); q.size = sparks.rnd(0.03f, 0.06f); q.life = sparks.rnd(10, 18); q.drag = 0.86f; q.noFloor = true; const glm::vec3 c = i % 3 ? glm::vec3(0.16f, 0.1f, 0.1f) : glm::vec3(1.f, 0.96f, 0.6f); q.c0 = glm::vec4(c, 1.f); q.c1 = q.c0; sparks.emit(q); } }
            // especiais: pontos do corpo como desenhado e o efeito de cada um (render/skill_fx.hpp)
            { const auto bodyOf = [](const FighterPose& q, const Model& m, const RigRest& r) { FxBody b; const glm::mat4 base = q.root * glm::scale(glm::mat4(1.f), glm::vec3(m.scale));
                  b.feet = glm::vec3(q.root[3]); b.feet.y = 0.f; b.chest = glm::vec3(base * glm::vec4(q.upperPos + glm::vec3(0, (r.NECK - r.WAIST) * 0.45f, 0), 1)); b.head = glm::vec3(base * glm::vec4(q.upperPos + glm::vec3(0, r.NECK - r.WAIST + 0.16f, 0), 1));
                  for (int i = 0; i < 2; i++) b.glove[i] = glm::vec3(base * glm::vec4(q.glove[i], 1)); b.size = m.scale; return b; };
              FxBody bP = bodyOf(poseP, mP, restP), bE = bodyOf(poseE, mE, restE); const glm::vec3 d = bE.feet - bP.feet; const glm::vec3 fw = glm::normalize(glm::vec3(d.x, 0, d.z) + glm::vec3(0, 0, 1e-4f)); bP.fwd = fw; bE.fwd = -fw;
              // especial com efeito gravado do vinil: o clipe começa com o golpe e segue o progresso dele
              const auto clipFx = [&](FxPlayer& pl, const Fighter& f, const FxBody& me, const FxBody& foe, float scale) {
                  const FxLibrary* lib = f.activeSkill == SkillId::JoanaAnchorSlam || LeapFx::isLeap(f) || TongueFx::active(f) || GeyserFx::active(f) || CloudFx::active(f) || StabFx::active(f) || StingFx::active(f) || SpitFx::active(f) || BeeFx::casting(f) || LaserFx::active(f) || (f.charId == 0 && (f.activeSkill == SkillId::JackMolasses || f.activeSkill == SkillId::JackRoyalJelly || f.activeSkill == SkillId::JackHoneyWall)) ? nullptr : fxLibOf(f.charId); const bool in = f.state == FIGHTER_SKILL && f.activeSkill != SkillId::None;   /* a onda da âncora é feita na hora (water_fx.hpp) */
                  if (in && (!pl.inSkill || pl.skill != f.activeSkill)) { pl.start(lib, f.activeSkill, me.feet, me.fwd, glm::length(glm::vec2(foe.feet.x - me.feet.x, foe.feet.z - me.feet.z)), scale); }
                  if (pl.follow) pl.reframe(me.feet, me.fwd);   // modo: acompanha o lutador
                  if (in) pl.tick(1.f - float(f.timer) / float(std::max(1, f.maxTimer))); else { pl.tick(-1.f, modeActive(f, pl.skill)); pl.inSkill = false; }
                  return in && pl.has(lib, f.activeSkill); };
              // onda da âncora: sai com a batida no chão e quebra no rival no acerto do golpe (40% dele)
              const auto waveOf = [&](WaveFx& wv, const Fighter& f, const FxBody& me, const FxBody& foe) {
                  const bool in = f.state == FIGHTER_SKILL && f.activeSkill == SkillId::JoanaAnchorSlam; const float prog = in ? 1.f - float(f.timer) / float(std::max(1, f.maxTimer)) : 0.f;
                  if (in && !wv.fired && prog >= 0.02f) { wv.start(me.feet, foe.feet, me.size); wv.fired = true; } if (!in) wv.fired = false; wv.tick(foe.feet, sparks); return in; };
              const bool waveInP = waveOf(waveP, engine.player, bP, bE), waveInE = waveOf(waveE, engine.enemy, bE, bP);   /* a onda substitui o efeito genérico do golpe */
              const bool ownP = fxLibOf(engine.player.charId) != nullptr, ownE = fxLibOf(engine.enemy.charId) != nullptr;
              const bool clipP = clipFx(fxP, engine.player, bP, bE, mP.scale), clipE = clipFx(fxE, engine.enemy, bE, bP, mE.scale);
              const auto leapTint = [](const Fighter& f) { return f.charId == 1 ? glm::vec3(0.3f, 0.48f, 0.16f) : glm::vec3(0.84f, 0.7f, 0.42f); };
              leapP.tick(engine.player, bP.feet, bP.size, leapTint(engine.player), sparks); leapE.tick(engine.enemy, bE.feet, bE.size, leapTint(engine.enemy), sparks);   /* rachadura e poeira no toque */
              { const Fighter& e = engine.enemy; const bool w = e.state == FIGHTER_TELEGRAPH && e.queuedSkill != SkillId::None && !e.queuedSignature;   /* aviso do especial do oponente */
                fxs.warnK = w ? 1.f - float(e.timer) / float(std::max(1, e.maxTimer)) : 0.f; if (w) { fxs.warnAt = bP.feet; fxs.warnSize = bP.size; } }
              geyP.tick(engine.player, bE.feet, bP.size, sparks); geyE.tick(engine.enemy, bP.feet, bE.size, sparks);   /* gêiser do pântano: coluna de água nos pés do rival */
              cloudP.tick(engine.player, bP.glove[0], bP.glove[1], bP.chest, bP.fwd, bE.chest, bP.size, sparks); cloudE.tick(engine.enemy, bE.glove[0], bE.glove[1], bE.chest, bE.fwd, bP.chest, bE.size, sparks);   /* golpes de nuvem */
              stabP.tick(engine.player, bP.feet, bE.feet, bP.fwd, sparks); stabE.tick(engine.enemy, bE.feet, bP.feet, bE.fwd, sparks);   /* estacas do chão: rocha e gelo */
              stingP.tick(engine.player, bP.glove[0], bP.glove[1], bP.fwd, bE.chest, sparks); stingE.tick(engine.enemy, bE.glove[0], bE.glove[1], bE.fwd, bP.chest, sparks);   /* ferrão da Agulhada Rainha */
              { const glm::vec3 up(0, 1, 0); spitP.tick(engine.player, bP.head + bP.fwd * 0.2f - up * 0.14f, bE.chest, bE.feet, sparks); spitE.tick(engine.enemy, bE.head + bE.fwd * 0.2f - up * 0.14f, bP.chest, bP.feet, sparks); }   /* lama e cereja cuspidas */
              honeyP.tick(engine.player, bP.glove[0], bP.glove[1], sparks, bP.feet); honeyE.tick(engine.enemy, bE.glove[0], bE.glove[1], sparks, bE.feet);   /* melaço e geleia do Jack */
              fxs.beeP.tick(engine.player, engine.enemy, bP.glove[0], bP.glove[1], bE.head, bE.chest, bP.size, sparks); fxs.beeE.tick(engine.enemy, engine.player, bE.glove[0], bE.glove[1], bP.head, bP.chest, bE.size, sparks);   /* abelhas da Infestação */
              fxs.meltP.tick(engine.player, engine.enemy, bP.feet, bP.fwd, bE.feet, bP.size); fxs.meltE.tick(engine.enemy, engine.player, bE.feet, bE.fwd, bP.feet, bE.size);   /* Cremoso derretido e a poça de calda */
              fxs.coneP.tick(engine.player, bP.glove[1], bP.fwd, bE.chest, bP.size, sparks); fxs.coneE.tick(engine.enemy, bE.glove[1], bE.fwd, bP.chest, bE.size, sparks);   /* casquinha */
              fxs.bubP.tick(engine.player, bP.feet, bP.size); fxs.bubE.tick(engine.enemy, bE.feet, bE.size);   /* escudo do Marcov */
              fxs.lasP.tick(engine.player, bE.feet, bP.fwd, bP.size, sparks); fxs.lasE.tick(engine.enemy, bP.feet, bE.fwd, bE.size, sparks);   /* raio orbital */
              fxs.syrP.tick(engine.player, bP.glove[0], bP.glove[1], bP.fwd, bE.chest, bP.size, sparks); fxs.syrE.tick(engine.enemy, bE.glove[0], bE.glove[1], bE.fwd, bP.chest, bE.size, sparks);   /* calda quente */
              fxs.heatP.tick(engine.player, bP.feet, bP.size, sparks); fxs.heatE.tick(engine.enemy, bE.feet, bE.size, sparks);   /* escudo de calor */
              fxs.jetP.tick(engine.player, bP.chest, bP.fwd, bP.size, sparks); fxs.jetE.tick(engine.enemy, bE.chest, bE.fwd, bE.size, sparks);   /* turbinas */
              for (const SpitFx* sp : {&spitP, &spitE}) if (sp->justHit) { engine.hitstopTimer = std::max(engine.hitstopTimer, 9); shakeExtra = std::max(shakeExtra, 22.f); if (sp->style == SpitFx::kCherry) impact.spawn(1, ++hitCount, sp == &spitP); }   /* a cuspida chega com peso */
              skillFx.tick(0, engine.player, bP, bE, sparks, clipP || waveInP || LeapFx::isLeap(engine.player) || TongueFx::active(engine.player) || GeyserFx::active(engine.player) || CloudFx::active(engine.player) || StabFx::active(engine.player) || StingFx::active(engine.player) || SpitFx::active(engine.player) || BeeFx::casting(engine.player) || LaserFx::active(engine.player) || (engine.player.charId == 0 && engine.player.activeSkill == SkillId::JackHoneyWall), ownP); skillFx.tick(1, engine.enemy, bE, bP, sparks, clipE || waveInE || LeapFx::isLeap(engine.enemy) || TongueFx::active(engine.enemy) || GeyserFx::active(engine.enemy) || CloudFx::active(engine.enemy) || StabFx::active(engine.enemy) || StingFx::active(engine.enemy) || SpitFx::active(engine.enemy) || BeeFx::casting(engine.enemy) || LaserFx::active(engine.enemy) || (engine.enemy.charId == 0 && engine.enemy.activeSkill == SkillId::JackHoneyWall), ownE);
              // rastro de quadrinho (smear): a luva que vai estica para trás; rasgada como labareda (o corpo usa as linhas de movimento)
              // o rastro cresce rápido e some devagar, sem ligar e desligar de um quadro para o outro (quando some, só encolhe na mesma direção)
              const auto smoothstep01 = [](float x) { x = std::clamp(x, 0.f, 1.f); return x * x * (3.f - 2.f * x); };
              const auto ease = [](const glm::vec3& cur, const glm::vec3& want) { return glm::length(want) > 1e-5f ? cur + (want - cur) * 0.35f : cur * 0.8f; };
              const auto smearOf = [&](Renderer::Smear& sm, int w, const Fighter& f, const FxBody& b) {
                  const bool active = isPunchState(f.state) || (f.state == FIGHTER_SKILL && !LeapFx::isLeap(f) && !StingFx::active(f));   /* no salto o corpo todo se move: o rastro das luvas só suja */ sm.gloveR = 0.13f * b.size;
                  for (int g = 0; g < 2; g++) { const glm::vec3 d = smearReady ? b.glove[g] - prevGlove[w][g] : glm::vec3(0.f); const float L = glm::length(d); prevGlove[w][g] = b.glove[g];
                      const bool out = active && glm::dot(d / std::max(L, 1e-5f), glm::normalize(b.glove[g] - b.chest + glm::vec3(1e-4f))) > -0.25f;
                      sm.glove[g] = ease(sm.glove[g], out ? d * smoothstep01((L - 0.02f) / 0.035f) : glm::vec3(0.f)); }
                  };
              smearOf(smP, 0, engine.player, bP); smearOf(smE, 1, engine.enemy, bE); smearReady = true;
              speedLines.tick(0, engine.player, bP, sparks, false); speedLines.tick(1, engine.enemy, bE, sparks, false); }   // o corpo fica com as linhas de movimento clássicas
            // grava o quadro para o replay (a luta e o voo depois do nocaute)
            if ((!snap || std::getenv("KORAMA3D_REPLAY")) && replayT < 0 && (!engine.isMatchOver() || endFrames <= 110)) {
                const auto localLag = [](const LagSpring& l, const FighterPose& q) { return glm::vec3(glm::inverse(q.root) * glm::vec4(l.o, 0)); };
                ReplayFrame fr; fr.pP = poseP; fr.pE = poseE; fr.lagP = localLag(lagP, poseP); fr.lagE = localLag(lagE, poseE); fr.cP = clothP; fr.cE = clothE; fr.ropes = ropes; fr.parts = sparks.list;
                fr.faceP = faceNow(engine.player, downP, tongueP.mouth(), hurtP); fr.faceE = faceNow(engine.enemy, downE, tongueE.mouth(), hurtE);
                fr.wobP = wobP.s * (1.f - downP); fr.wobE = wobE.s * (1.f - downE); fr.dirP = float(engine.player.hitDir); fr.dirE = float(engine.enemy.hitDir); fr.t60 = float(engine.frameIndex) / 60.f; fr.impact = impact; fr.sounds = engine.sounds; fr.fx = fxs; for (HoneyFx* h : {&fr.fx.honeyP, &fr.fx.honeyE}) { h->faceV = {}; h->faceI = {}; } fr.hit = recHit; recHit = false;
                rec.push_back(std::move(fr)); if (rec.size() > kReplayFrames) rec.pop_front(); }
        };
        // lutadores e efeitos de golpe: a luta e o replay desenham pela mesma rotina (antes e depois da arena), cada um com o seu conjunto de efeitos
        const auto drawFighters = [&](FxSet& S, Model& mP, Model& mE, const FighterPose& drawP, const FighterPose& drawE, const glm::vec3& lagPL, const glm::vec3& lagEL, const ClothSim& clothP, const ClothSim& clothE, float t60, const char* faceP, const char* faceE, const Renderer::Smear* smearP, const Renderer::Smear* smearE) {
                FxPlayer &fxP = S.fxP, &fxE = S.fxE; WaveFx &waveP = S.waveP, &waveE = S.waveE; TongueFx &tongueP = S.tongueP, &tongueE = S.tongueE; GeyserFx &geyP = S.geyP, &geyE = S.geyE; StabFx &stabP = S.stabP, &stabE = S.stabE; StingFx &stingP = S.stingP, &stingE = S.stingE; SpitFx &spitP = S.spitP, &spitE = S.spitE; HoneyFx &honeyP = S.honeyP, &honeyE = S.honeyE; const float poisonP = S.poisonP, poisonE = S.poisonE;
                (void)fxP; (void)fxE; (void)waveP; (void)waveE; (void)tongueP; (void)tongueE; (void)geyP; (void)geyE; (void)stabP; (void)stabE; (void)stingP; (void)stingE; (void)spitP; (void)spitE; (void)honeyP; (void)honeyE; (void)poisonP; (void)poisonE;
            mP.setFace(faceP); if (*spitP.face()) mP.setFace(spitP.face()); mP.blendFace("molasses", engine.player.charId == 0 ? honeyP.ma : 0.f); mP.shade = engine.player.charId == 0 ? honeyP.ma : 0.f; if (mP.shade > 0.f) { int nd = -1; glm::vec3 lp, ln; if (mP.uvPointSmooth(0, glm::vec2(0.5f, 0.2f), nd, lp, ln)) mP.shadeY = (glm::inverse(drawP.root * glm::scale(glm::mat4(1.f), glm::vec3(mP.scale))) * mP.nodes[size_t(nd)].world * glm::vec4(lp, 1.f)).y; } if (spitP.on() && spitP.style == SpitFx::kMud && spitP.t >= 11 && spitP.t <= 22) mP.setFace("spitO"); mP.poison = engine.player.charId == 0 ? (engine.player.loadout.passive == SkillId::JackHoneySkin ? 1.f : 0.f) : poisonP;   /* Jack: Pele de Mel */ rend.fighter(mP, drawP, lagPL, clothP, t60, smearP); mE.setFace(faceE); if (*spitE.face()) mE.setFace(spitE.face()); mE.blendFace("molasses", engine.enemy.charId == 0 ? honeyE.ma : 0.f); mE.shade = engine.enemy.charId == 0 ? honeyE.ma : 0.f; if (mE.shade > 0.f) { int nd = -1; glm::vec3 lp, ln; if (mE.uvPointSmooth(0, glm::vec2(0.5f, 0.2f), nd, lp, ln)) mE.shadeY = (glm::inverse(drawE.root * glm::scale(glm::mat4(1.f), glm::vec3(mE.scale))) * mE.nodes[size_t(nd)].world * glm::vec4(lp, 1.f)).y; } if (spitE.on() && spitE.style == SpitFx::kMud && spitE.t >= 11 && spitE.t <= 22) mE.setFace("spitO"); mE.poison = engine.enemy.charId == 0 ? (engine.enemy.loadout.passive == SkillId::JackHoneySkin ? 1.f : 0.f) : poisonE; rend.fighter(mE, drawE, lagEL, clothE, t60 + 1.3f, smearE);
            const auto anchorOf = [&](const FxPlayer& pl, Model& m) { FxPlayer::Anchor a; if (!pl.follow || pl.skill != SkillId::VivaldiFourSeasons) return a;   // os enfeites do cabelo da Vivaldi andam com a cabeça (o resto dos modos gira em volta do corpo)
                const auto rest = [&](float y) { return pl.frame * glm::translate(glm::mat4(1.f), glm::vec3(0.f, y, 0.f)); };
                a.head = m.nodes[size_t(m.need("neck"))].world * glm::inverse(rest(m.NECK)); a.body = m.nodes[size_t(m.need("upper"))].world * glm::inverse(rest(m.WAIST)); a.neckY = m.NECK; a.waistY = m.WAIST; a.on = true; return a; };
            for (const FxPlayer* pl : {&fxP, &fxE}) if (pl->active() && pl->lib && pl->lib->model && pl->pose(*pl->lib->model, anchorOf(*pl, pl == &fxP ? mP : mE))) { rend.effects(*pl->lib->model, t60);   // efeitos gravados do vinil
                }
            for (const WaveFx* wv : {&waveP, &waveE}) if (wv->on) { wv->build(waveV, waveI); rend.water(waveV, waveI, wv->height(), wv->foamy(), t60); } for (const GeyserFx* g : {&geyP, &geyE}) if (g->on()) { g->build(waveV, waveI); rend.water(waveV, waveI, g->height(), g->foamy(), t60, g->lava); }
        };
        const auto drawFxAfter = [&](FxSet& S, Model& mP, Model& mE, const FighterPose& drawP, const FighterPose& drawE) {
                FxPlayer &fxP = S.fxP, &fxE = S.fxE; WaveFx &waveP = S.waveP, &waveE = S.waveE; TongueFx &tongueP = S.tongueP, &tongueE = S.tongueE; GeyserFx &geyP = S.geyP, &geyE = S.geyE; StabFx &stabP = S.stabP, &stabE = S.stabE; StingFx &stingP = S.stingP, &stingE = S.stingE; SpitFx &spitP = S.spitP, &spitE = S.spitE; HoneyFx &honeyP = S.honeyP, &honeyE = S.honeyE; const float poisonP = S.poisonP, poisonE = S.poisonE;
                (void)fxP; (void)fxE; (void)waveP; (void)waveE; (void)tongueP; (void)tongueE; (void)geyP; (void)geyE; (void)stabP; (void)stabE; (void)stingP; (void)stingE; (void)spitP; (void)spitE; (void)honeyP; (void)honeyE; (void)poisonP; (void)poisonE;
                if (S.warnK > 0.f) {   // marca no chão do especial que vem: aro vermelho piscando e o miolo enchendo até a hora do golpe
                    std::vector<float> wv; std::vector<unsigned> wi; const float R = 0.62f * S.warnSize, y = Particles::kFloor + 0.008f, k = S.warnK, blink = 0.75f + 0.25f * std::sin(k * 40.f);
                    const auto put = [&](float x, float z, const glm::vec3& c) { const float d[12] = {S.warnAt.x + x, y, S.warnAt.z + z, 0.f, 1.f, 0.f, c.r, c.g, c.b, -1.f, -1.f, 0.f}; wv.insert(wv.end(), d, d + 12); };
                    const auto ring = [&](float r0, float r1, const glm::vec3& c) { const unsigned b = unsigned(wv.size() / 12); constexpr int N = 40;
                        for (int i = 0; i < N; i++) { const float a = float(i) / N * 6.2832f; put(std::cos(a) * r0, std::sin(a) * r0, c); put(std::cos(a) * r1, std::sin(a) * r1, c); }
                        for (int i = 0; i < N; i++) { const unsigned i0 = b + unsigned(i * 2), i1 = i0 + 1, i2 = b + unsigned(((i + 1) % N) * 2), i3 = i2 + 1; wi.insert(wi.end(), {i0, i1, i2, i1, i3, i2}); } };
                    ring(0.f, R * 0.8f * k, glm::vec3(1.f, 0.3f, 0.05f));                                        // miolo enchendo: cheio = hora do golpe
                    rend.props(wv, wi, undiesTex, 0.45f + 0.3f * k); wv.clear(); wi.clear();
                    ring(R * 0.8f, R, glm::vec3(1.f, 0.05f, 0.03f) * blink);                                     // aro vermelho piscando
                    rend.props(wv, wi, undiesTex, 0.92f); }
            { // língua do King Frog: sai do ponto da boca na malha (já posta pelo rend.fighter) até o peito do rival
              std::vector<Particle> tongue; const auto tongueOf = [&](const TongueFx& tg, Model& m, const FighterPose& foe, const Model& mf) {
                  if (!tg.tongueOut()) return; int node = -1; glm::vec3 lp, ln; if (!m.uvPoint(0, TongueFx::kMouthUV, node, lp, ln)) return;
                  const glm::mat4& w = m.nodes[size_t(node)].world; const glm::vec3 at = glm::vec3(w * glm::vec4(lp, 1.f)), nw = glm::normalize(glm::mat3(w) * ln);
                  const glm::vec3 target = glm::vec3(foe.root[3]) + glm::vec3(0, topOf(mf) * 0.6f, 0);
                  tg.build(at + nw * 0.02f, target, m.scale, tongue); };
              tongueOf(tongueP, mP, drawE, mE); tongueOf(tongueE, mE, drawP, mP);
              // papo do King Frog na cuspida de lama: a bolsa da garganta incha logo abaixo da boca (no ponto da boca na malha)
              { Throwables::Mesh pm; const auto papo = [&](const SpitFx& sp, Model& m) { const float i = sp.on() && sp.style == SpitFx::kMud ? sp.inflate() : 0.f; if (i <= 0.02f) return; int node = -1; glm::vec3 lp, ln; if (!m.uvPoint(0, TongueFx::kMouthUV, node, lp, ln)) return;
                    const glm::mat4& w = m.nodes[size_t(node)].world; const glm::vec3 at = glm::vec3(w * glm::vec4(lp, 1.f)), nw = glm::normalize(glm::mat3(w) * ln), side = glm::normalize(glm::cross(glm::vec3(0, 1, 0), nw) + glm::vec3(1e-5f));
                    Throwables::blob(pm, at - glm::vec3(0, 0.09f + 0.04f * i, 0) + nw * (0.02f + 0.07f * i), side * (0.05f + 0.1f * i), glm::vec3(0, 0.04f + 0.07f * i, 0), nw * (0.03f + 0.07f * i), glm::vec3(0.42f, 0.62f, 0.24f), 0.006f, 0.f); };
                const auto cheeks = [&](const SpitFx& sp, Model& m) { const float i = sp.cheeks(); if (i <= 0.02f) return;
                    for (const glm::vec2 uv : {glm::vec2(0.16f, 0.3f), glm::vec2(0.84f, 0.3f)}) { int node = -1; glm::vec3 lp, ln; if (!m.uvPoint(0, uv, node, lp, ln)) continue;
                        const glm::mat4& w = m.nodes[size_t(node)].world; const glm::vec3 at = glm::vec3(w * glm::vec4(lp, 1.f)), nw = glm::normalize(glm::mat3(w) * ln); const float r = 0.035f + 0.09f * i;
                        const glm::vec3 side = glm::normalize(glm::cross(glm::vec3(0, 1, 0), nw) + glm::vec3(1e-5f));
                        Throwables::blob(pm, at + nw * (r * 0.45f), side * r, glm::vec3(0, r * 0.85f, 0), nw * r, glm::vec3(0.95f, 0.74f, 0.2f), 0.005f, 0.f); } };   // bochechas cheias da Cereja
                papo(spitP, mP); papo(spitE, mE); cheeks(spitP, mP); cheeks(spitE, mE); if (!pm.idx.empty()) { std::vector<float> pv; std::vector<unsigned> pi; const unsigned b = 0; for (const auto& q : pm.v) { const float d[12] = {q.p.x, q.p.y, q.p.z, q.n.x, q.n.y, q.n.z, q.c.r, q.c.g, q.c.b, -1.f, -1.f, q.ink}; pv.insert(pv.end(), d, d + 12); } for (unsigned k : pm.idx) pi.push_back(b + k); rend.props(pv, pi, undiesTex); } } if (!tongue.empty()) rend.particles(tongue); }
            if (honeyP.on() || honeyE.on() || honeyP.wallOn() || honeyE.wallOn()) { honV.clear(); honI.clear(); honGV.clear(); honGI.clear(); honeyP.build(mP, honV, honI, honGV, honGI); honeyE.build(mE, honV, honI, honGV, honGI);
                { static std::vector<float> wfV, weV; static std::vector<unsigned> wfI, weI; wfV.clear(); wfI.clear(); weV.clear(); weI.clear(); honeyP.buildWall(rend.eyePos(), mP.scale, wfV, wfI, weV, weI, honGV, honGI); honeyE.buildWall(rend.eyePos(), mE.scale, wfV, wfI, weV, weI, honGV, honGI); rend.props(wfV, wfI, undiesTex, 0.5f); rend.props(weV, weI, undiesTex, 0.92f); }   /* favos: miolo e borda translúcidos */ rend.props(honV, honI, undiesTex); for (HoneyFx* h : {&honeyP, &honeyE}) rend.props(h->faceV, h->faceI, undiesTex, 0.55f); rend.glow(honGV, honGI); }   // gotas do rosto translúcidas   // mel escorrendo do Jack
            { static std::vector<float> cv, cw, cg, bf, bl; static std::vector<unsigned> ci, cwi, cgi, bfi, bli; cv.clear(); ci.clear(); cw.clear(); cwi.clear(); cg.clear(); cgi.clear();   // especiais refeitos em código (render/char_fx.hpp)
              S.meltP.build(cv, ci); S.meltE.build(cv, ci); S.coneP.build(cv, ci); S.coneE.build(cv, ci); S.beeP.build(cv, ci, cw, cwi); S.beeE.build(cv, ci, cw, cwi); S.lasP.build(cv, ci, cg, cgi); S.lasE.build(cv, ci, cg, cgi); S.jetP.build(cv, ci); S.jetE.build(cv, ci); S.syrP.build(cv, ci); S.syrE.build(cv, ci); S.heatP.build(cv, ci, cg, cgi); S.heatE.build(cv, ci, cg, cgi);
              if (!ci.empty()) rend.props(cv, ci, undiesTex); if (!cwi.empty()) rend.props(cw, cwi, undiesTex, 0.6f);
              for (const BubbleFx* bb : {&S.bubP, &S.bubE}) if (bb->on()) { bf.clear(); bfi.clear(); bl.clear(); bli.clear(); bb->build(rend.eyePos(), bf, bfi, bl, bli, cg, cgi); rend.props(bf, bfi, undiesTex, bb->fillAlpha()); rend.props(bl, bli, undiesTex, bb->lineAlpha()); }
              if (!cgi.empty()) rend.glow(cg, cgi); }
            if (stingP.on() || stingE.on()) { stabV.clear(); stabI.clear(); stingP.build(stabV, stabI); stingE.build(stabV, stabI); rend.glow(stabV, stabI); }   // ferrão de luz da Agulhada Rainha
            if (stabP.on() || stabE.on() || spitP.on() || spitE.on()) { stabV.clear(); stabI.clear(); stabP.build(stabV, stabI); stabE.build(stabV, stabI); spitP.build(stabV, stabI); spitE.build(stabV, stabI); rend.props(stabV, stabI, undiesTex); }   // estacas de rocha e gelo
        };
        const auto setCloudOf = [&](const FxSet& S) { const CloudFx& pc = S.cloudP.density() >= S.cloudE.density() ? S.cloudP : S.cloudE; rend.setCloud(pc.center(), pc.radius(), pc.density(), pc.on() ? pc.silColor() : glm::vec3(0.f)); };   // nuvem: quem está atrás vira silhueta
        const auto render = [&]() { if (!win.setActive(true)) throw std::runtime_error("contexto OpenGL perdido");
            Model& mP = fighterModel(engine.player); Model& mE = fighterModel(engine.enemy); const int W = int(win.getSize().x), H = int(win.getSize().y);
            const float t60 = float(engine.frameIndex) / 60.f; const auto localLag = [](const LagSpring& l, const FighterPose& q) { return glm::vec3(glm::inverse(q.root) * glm::vec4(l.o, 0)); };
            { const float k = impact.alive ? std::max(0.f, 1.f - impact.age() * 2.2f) : 0.f; rend.setFlash(impact.at, k * k * (impact.kind == 0 ? 2.5f : 5.f)); }
            const glm::vec3 lagPL = localLag(lagP, poseP), lagEL = localLag(lagE, poseE);
            FighterPose drawP = poseP, drawE = poseE; const bool celeb = celebF >= 0;
            if (celeb) {
                const MatchResult r = engine.result; const bool champP = !(r == MatchResult::LossKO || r == MatchResult::LossTKO || r == MatchResult::LossDecision || (r == MatchResult::Draw && engine.player.hp < engine.enemy.hp));
                FighterPose& ch = champP ? drawP : drawE; FighterPose& lo = champP ? drawE : drawP; const float top = topOf(champP ? mP : mE);
                const glm::vec3 p0(ch.root[3]); glm::vec3 f0(ch.root[2]); f0.y = 0.f; f0 = glm::length(f0) > 1e-4f ? glm::normalize(f0) : glm::vec3(0, 0, 1);
                if (celebF == 0) celebA0 = std::atan2(f0.x, f0.z);   // a câmera começa de frente para o campeão
                const float a = celebA0 - float(celebF) * 0.0018f, t = float(celebF) / 60.f * 3.f;
                const glm::vec3 c(0.f), toCam(std::sin(a), 0.f, std::cos(a)), right = glm::normalize(glm::cross(toCam, glm::vec3(0, 1, 0)));
                const float D = 3.3f + top * 0.25f; const glm::vec3 eye = c + toCam * D + glm::vec3(0, top * 1.3f + 0.2f, 0), at = c + glm::vec3(0, top * 0.45f, 0) - right * (D * 0.2f);   /* de cima: olha por cima das cordas */   // campeão à esquerda: o card fica à direita
                rend.setCamera(true, eye, at);
                // dança: pulinhos, gingado e, a cada 4 s, um rodopio completo, sempre de frente para a câmera
                const float cycle = std::fmod(t / 3.f, 4.f), twirl = cycle > 3.4f ? (cycle - 3.4f) / 0.6f : 0.f;
                const float yaw = a + std::sin(t * 1.6f) * 0.35f + twirl * 6.2832f - std::atan2(f0.x, f0.z), hop = (std::fabs(std::sin(t * 4.5f)) * 0.06f + std::sin(twirl * 3.14159f) * 0.09f) * top;
                const glm::vec3 sway = glm::cross(glm::vec3(0, 1, 0), toCam) * (std::sin(t * 1.5f) * 0.05f * top);
                ch.root = glm::translate(glm::mat4(1.f), c + sway + glm::vec3(0, hop, 0)) * glm::rotate(glm::mat4(1.f), yaw, glm::vec3(0, 1, 0)) * glm::translate(glm::mat4(1.f), -glm::vec3(p0.x, 0.f, p0.z)) * ch.root;
                lo.root = glm::translate(glm::mat4(1.f), glm::vec3(0, -80.f, 0)) * lo.root;   // o perdedor sai de cena
            }
            if (previewing && previewEnemyCam) {   // prévia vista do lado do rival: atrás dele, um pouco ao lado, olhando para o nosso lutador
                const glm::vec3 pp = smoothP.pos, pe = smoothE.pos; glm::vec3 d = pe - pp; d.y = 0.f; d = glm::length(d) > 1e-3f ? glm::normalize(d) : glm::vec3(0, 0, 1); const glm::vec3 side = glm::normalize(glm::cross(glm::vec3(0, 1, 0), d));
                const float tp = topOf(mP), te = topOf(mE); rend.setCamera(true, pe + d * (1.7f + te * 0.35f) + side * 0.95f + glm::vec3(0, te * 1.0f + 0.35f, 0), pp + glm::vec3(0, tp * 0.5f, 0) - side * 0.35f); }
            setCloudOf(fxs);
            rend.shadowPass(*arena, mP, drawP, clothP, t60, mE, drawE, clothE, t60 + 1.3f);
            { const int f = finisher; const auto ease = [](float x) { x = std::clamp(x, 0.f, 1.f); return x * x * (3.f - 2.f * x); }; const float k = f < 0 ? 0.f : f < kFinisherFreeze ? ease(float(f + 1) / 8.f) : 1.f - ease(float(f - kFinisherFreeze) / 18.f);   /* a câmera avança e volta suave */
              rend.setImpactFrame(f >= 0 && f <= 1 ? 1.f : f >= 2 && f <= 3 ? -1.f : 0.f); /* dois quadros invertidos, dois positivos */ rend.setPunchIn(k, impact.at);
              glm::vec2 su; rend.setShock(shockProg <= 1.f && rend.project(shockAt, W, H, su) ? glm::vec2(su.x / float(W), 1.f - su.y / float(H)) : glm::vec2(0.f), shockProg, shockStr);
              { const float t = float(std::max(kickT, 0)), env = kickT < 0 ? 0.f : kickAmp * std::exp(-0.16f * t);   /* mola amortecida: tranco, volta e assenta */
                rend.setCameraKick(kickDir * (0.1f * std::sin(t * 0.75f) * env) + glm::vec3(0, 0.05f * std::sin(t * 0.95f + 1.1f) * env, 0), 0.035f * std::sin(t * 0.6f) * env); }
              float chroma = std::min(0.022f, (engine.screenShake + shakeExtra) * 0.0009f); if (engine.gameState == STATE_KNOCKDOWN) chroma = std::max(chroma, 0.008f); if (f >= 0 && f < kFinisherFreeze) chroma = std::max(chroma, 0.012f); rend.setChroma(chroma); }
            rend.begin(W, H, engine.ringAngle, engine.player, engine.enemy, mP.scale, mE.scale, topOf(mP), topOf(mE), engine.screenShake + shakeExtra, engine.frameIndex, pull, smoothP.pos); 
            drawFighters(fxs, mP, mE, drawP, drawE, lagPL, lagEL, clothP, clothE, t60, faceNow(engine.player, downP, tongueP.mouth(), hurtP), faceNow(engine.enemy, downE, tongueE.mouth(), hurtE), celeb ? nullptr : &smP, celeb ? nullptr : &smE);
            rend.arena(*arena, t60); rend.ropes(ropes);
            drawFxAfter(fxs, mP, mE, drawP, drawE);
            if (celeb) { throws.build(propV, propI, t60); rend.props(propV, propI, undiesTex); }   // o que a torcida jogou
            rend.particles(sparks.list); rend.finish(W, H); if (celeb || previewing) rend.setCamera(false);
            if (previewing) { glBindVertexArray(0); glUseProgram(0); glDisable(GL_DEPTH_TEST); glDisable(GL_CULL_FACE); if (previewTex.getSize() != win.getSize()) (void)previewTex.resize(win.getSize()); previewTex.update(win); return; }   // a prévia só quer o quadro 3D
            glBindVertexArray(0); glUseProgram(0); glDisable(GL_DEPTH_TEST); glDisable(GL_CULL_FACE);
            win.pushGLStates(); glm::vec2 sp;
            if (finisher >= 0 && finisher < kFinisherFreeze + 14 && rend.project(impact.at, W, H, sp)) {
                // linhas radiais de mangá em volta do golpe: raios tortos que tremem (o zigue-zague muda a cada 3 quadros), feitos de pontos de
                // retícula que diminuem e somem em direção ao golpe; entram vindo da borda e se apagam devagar na soltura
                const auto ease = [](float x) { x = std::clamp(x, 0.f, 1.f); return x * x * (3.f - 2.f * x); };
                const auto hash = [](int v) { uint32_t x = uint32_t(v) * 2654435761u; x ^= x >> 15; x *= 2246822519u; x ^= x >> 13; return float(x & 0xffffu) / 65535.f; };
                const float grow = ease(float(finisher + 1) / 7.f), fade = finisher < kFinisherFreeze ? 1.f : 1.f - ease(float(finisher - kFinisherFreeze) / 14.f);
                constexpr float kDot = 11.f;   // pixels entre os pontos da retícula ao longo do raio
                const int flick = finisher / 3, seed = impact.seed * 977; const sf::Vector2f c(sp.x, sp.y); sf::VertexArray dots(sf::PrimitiveType::Triangles);
                float rOut = 0.f; for (const sf::Vector2f k : {sf::Vector2f(0, 0), sf::Vector2f(float(W), 0), sf::Vector2f(0, float(H)), sf::Vector2f(float(W), float(H))}) rOut = std::max(rOut, std::hypot(k.x - c.x, k.y - c.y));   // começa no canto mais longe: o degradê fica todo na tela
                for (int i = 0; i < 40; i++) {
                    const float ang = (float(i) + 0.6f * (hash(i * 3 + seed) - 0.5f)) / 40.f * 6.2832f, r0 = 210.f + 120.f * hash(i * 7 + seed), rIn = rOut + (r0 - rOut) * grow;
                    const sf::Vector2f dir(std::cos(ang), std::sin(ang)), perp(-dir.y, dir.x);
                    sf::Vector2f pts[9]; for (int j = 0; j < 9; j++) { const float t = float(j) / 8.f, r = rOut + (rIn - rOut) * t, off = (hash(i * 31 + j * 7 + flick * 101 + seed) - 0.5f) * 52.f * (j ? 1.f - 0.45f * t : 0.f); pts[j] = c + dir * r + perp * off; }
                    float walked = 0.f, total = 0.f; for (int j = 0; j < 8; j++) { const sf::Vector2f d = pts[j + 1] - pts[j]; total += std::sqrt(d.x * d.x + d.y * d.y); }
                    for (int j = 0; j < 8; j++) { const sf::Vector2f d = pts[j + 1] - pts[j]; const float L = std::sqrt(d.x * d.x + d.y * d.y); if (L < 1.f) continue; const sf::Vector2f u = d / L, n(-u.y, u.x);
                        for (float s = std::fmod(kDot - std::fmod(walked, kDot), kDot); s < L; s += kDot) { const float t = (walked + s) / std::max(1.f, total), k = std::pow(1.f - t, 0.9f), size = 1.2f + 6.8f * k;   /* retícula: ponto grande na borda, some no golpe */
                            const std::uint8_t al = std::uint8_t(std::clamp(175.f * std::pow(1.f - t, 1.1f) * fade, 0.f, 255.f)); if (al < 4) continue; const sf::Color ink(20, 12, 12, al); const sf::Vector2f p = pts[j] + u * s, A = u * size, B = n * size;
                            dots.append({p + A, ink}); dots.append({p + B, ink}); dots.append({p - A, ink}); dots.append({p - A, ink}); dots.append({p - B, ink}); dots.append({p + A, ink}); }
                        walked += L; }
                }
                win.draw(dots); }
            if (impact.alive && rend.project(impact.at, W, H, sp)) drawImpact(win, font, impact, sp);
            if (fxs.warnK > 0.f && !celeb && rend.project(glm::vec3(drawE.root[3]) + glm::vec3(0, topOf(mE) - 0.12f, 0), W, H, sp)) drawWarnMark(win, font, sp - glm::vec2(0.f, engine.enemy.chainNeed > 0 ? 24.f : 0.f), fxs.warnK);
            if (!celeb) {   // combos de entrada: o do oponente em vermelho (vem especial), o do jogador em amarelo (azul quando a esquiva abriu o fluxo)
                const Fighter& e = engine.enemy; const Fighter& p = engine.player;
                if (e.chainNeed > 0 && rend.project(glm::vec3(drawE.root[3]) + glm::vec3(0, topOf(mE) - 0.12f, 0), W, H, sp)) drawChainPips(win, sp - glm::vec2(0.f, 10.f), e.chainCount, e.chainNeed, sf::Color(255, 60, 40));
                const bool flow = p.flowOpen && p.chainCount < 2;
                if (p.chainNeed > 0 && (p.chainCount > 0 || flow) && rend.project(glm::vec3(drawP.root[3]) + glm::vec3(0, topOf(mP) + 0.05f, 0), W, H, sp)) drawChainPips(win, sp - glm::vec2(0.f, 10.f), flow ? 2 : p.chainCount, p.chainNeed, flow ? sf::Color(0, 229, 255) : sf::Color(255, 225, 0)); }
            if (celeb) {
                if (hud2d && celebF >= kCardDelay) { const sf::Vector2i m = sf::Mouse::getPosition(win); hud2d->uiMouse = {float(m.x) * 1600.f / float(W), float(m.y) * 900.f / float(H)}; hud2d->drawResultCard(win, engine, celebF - kCardDelay); }
            } else if (hud2d) hud2d->drawHudOnly(win, engine, false); else hud(win, font, engine);
            win.popGLStates(); };
        // ---------------- replay do nocaute: a fita volta e o golpe passa de novo, com cortes de câmera rápidos e câmera lenta no impacto ----------------
        const auto koDecisive = [&]() { const MatchResult r = engine.result; return r == MatchResult::WinKO || r == MatchResult::WinTKO || r == MatchResult::LossKO || r == MatchResult::LossTKO; };
        const auto replayFinish = [&]() { replayT = -1; replayDone = true; endFrames = 150; if (audioOn) { audio.setMusicPitch(1.f); audio.setMusicDuck(1.f); } };
        const auto replayBegin = [&]() -> bool {
            int hit = -1; for (int i = int(rec.size()) - 1; i >= 0; i--) if (rec[size_t(i)].hit) { hit = i; break; }
            if (hit < 20) return false;
            replayHit = hit; replayStart = std::max(0, hit - 100); replayEnd = int(rec.size()) - 1; replayPos = float(replayEnd); replayT = 0; shot = -1; shotT = 0;
            if (audioOn) { audio.setMusicDuck(0.25f); audio.setMusicPitch(0.85f); audio.play(Sfx::Rewind); }
            return true; };
        const auto replayTick = [&]() {
            replayT++;
            if (replayT <= kRewindFrames) { const float q = float(replayT) / float(kRewindFrames); replayPos = float(replayEnd) - float(replayEnd - replayStart) * q * q * (3.f - 2.f * q); if (replayT == kRewindFrames) replayPos = float(replayStart); return; }
            // velocidade: normal até perto do golpe, câmera lenta no impacto, um pouco lenta no voo
            const float speed = replayPos < float(replayHit - 16) ? 1.f : replayPos < float(replayHit + 14) ? 0.3f : 0.75f;
            const int before = int(replayPos); replayPos += speed; const int now = std::min(int(replayPos), replayEnd);
            if (audioOn) for (int i = before + 1; i <= now; i++) if (!rec[size_t(i)].sounds.empty()) audio.playReplay(rec[size_t(i)].sounds, speed < 0.5f ? 0.7f : 0.82f);
            const int want = replayPos >= float(replayHit - 16) && replayPos < float(replayHit + 14) ? 4 : replayPos >= float(replayHit + 14) ? 5 : (shot < 0 || shot >= 4 || shotT >= kShotFrames) ? (shot < 0 || shot >= 4 ? 0 : shot + 1) % 4 : shot;
            if (want != shot) { shot = want; shotT = 0; } else shotT++;
            if (replayPos >= float(replayEnd)) replayFinish(); };
        const auto replayCamera = [&](const ReplayFrame& f, glm::vec3& eye, glm::vec3& at) {
            const MatchResult r = engine.result; const bool pLost = r == MatchResult::LossKO || r == MatchResult::LossTKO;
            const glm::vec3 P(f.pP.root[3]), E(f.pE.root[3]), L = pLost ? P : E, Wn = pLost ? E : P, mid = (P + E) * 0.5f;
            const glm::vec3 axis = glm::normalize(glm::vec3(E.x - P.x, 0.f, E.z - P.z) + glm::vec3(0, 0, 1e-4f)), side = glm::cross(axis, glm::vec3(0, 1, 0)), away = glm::normalize(glm::vec3(L.x - Wn.x, 0.f, L.z - Wn.z) + glm::vec3(0, 0, 1e-4f));
            const float t = float(shotT);
            if (replayT <= kRewindFrames) { eye = mid - axis * 5.f + glm::vec3(0, 5.2f, 0); at = mid + glm::vec3(0, 0.6f, 0); return; }   // rebobinando: de cima
            switch (shot) {
            case 0: eye = mid + side * (3.4f + 0.012f * t) + glm::vec3(0, 0.75f, 0); at = mid + glm::vec3(0, 1.05f, 0); break;                          // lateral baixa
            case 1: eye = L + away * 1.9f + side * 0.3f + glm::vec3(0, 1.55f, 0); at = Wn + glm::vec3(0, 1.15f, 0); break;                              // a visão de quem vai cair
            case 2: { const float a = 0.6f + t * 0.022f; eye = mid + glm::vec3(std::cos(a) * 4.6f, 3.6f, std::sin(a) * 4.6f); at = mid + glm::vec3(0, 0.8f, 0); break; }   // do alto, girando
            case 3: eye = Wn + away * 1.7f - side * 0.9f + glm::vec3(0, 0.35f, 0); at = Wn + glm::vec3(0, 1.35f, 0); break;                             // de baixo, o vencedor
            case 4: { const glm::vec3 c = rec[size_t(replayHit)].impact.placed ? rec[size_t(replayHit)].impact.at : mid + glm::vec3(0, 1.2f, 0); const float a = t * 0.03f;   // close no golpe, orbitando
                eye = c + (side * std::cos(a) - axis * std::sin(a) * 0.6f) * 1.55f + glm::vec3(0, 0.25f, 0); at = c; break; }
            default: eye = L + side * 4.5f - away * 1.2f + glm::vec3(0, 1.8f, 0); at = L + glm::vec3(0, 0.9f, 0); break;                               // aberto, o voo até as cordas
            } };
        const auto renderReplay = [&]() { if (!win.setActive(true)) throw std::runtime_error("contexto OpenGL perdido");
            Model& mP = fighterModel(engine.player); Model& mE = fighterModel(engine.enemy); const int W = int(win.getSize().x), H = int(win.getSize().y);
            const ReplayFrame& f = rec[size_t(std::clamp(int(replayPos), 0, int(rec.size()) - 1))]; const bool rewinding = replayT <= kRewindFrames;
            // balanço do impacto: o corpo vai para trás, tomba para o lado do golpe e a cabeça chicoteia, voltando em mola (mais evidente que na luta)
            const auto sway = [](FighterPose q, float w, float dir) { if (w == 0.f) return q; const float d = dir < 0.f ? -1.f : 1.f;
                q.root = glm::translate(q.root, glm::vec3(0.06f * d * w, 0.f, -0.14f * w)); q.lean -= 0.34f * w; q.roll += 0.24f * d * w; q.neckTilt -= 0.4f * w; q.neckTurn += 0.35f * d * w; return q; };
            const FighterPose rpP = sway(f.pP, f.wobP, f.dirP), rpE = sway(f.pE, f.wobE, f.dirE);
            { const float k = f.impact.alive ? std::max(0.f, 1.f - f.impact.age() * 2.2f) : 0.f; rend.setFlash(f.impact.at, k * k * (f.impact.kind == 0 ? 2.5f : 5.f)); }
            rend.setImpactFrame(0.f); rend.setPunchIn(0.f, glm::vec3(0.f)); rend.setCameraKick(glm::vec3(0.f), 0.f); rend.setShock(glm::vec2(0.f), 2.f, 0.f); rend.setChroma(rewinding ? 0.02f : 0.004f);
            setCloudOf(f.fx); rend.shadowPass(*arena, mP, rpP, f.cP, f.t60, mE, rpE, f.cE, f.t60 + 1.3f);
            glm::vec3 eye, at; replayCamera(f, eye, at); rend.setCamera(true, eye, at);
            rend.begin(W, H, engine.ringAngle, engine.player, engine.enemy, mP.scale, mE.scale, topOf(mP), topOf(mE), 0.f, engine.frameIndex, 0.f, smoothP.pos);
            replayFx = f.fx; drawFighters(replayFx, mP, mE, rpP, rpE, f.lagP, f.lagE, f.cP, f.cE, f.t60, f.faceP, f.faceE, nullptr, nullptr);
            rend.arena(*arena, f.t60); rend.ropes(f.ropes); drawFxAfter(replayFx, mP, mE, rpP, rpE); rend.particles(f.parts); rend.finish(W, H); rend.setCamera(false);
            glBindVertexArray(0); glUseProgram(0); glDisable(GL_DEPTH_TEST); glDisable(GL_CULL_FACE);
            win.pushGLStates(); win.setView(sf::View(sf::FloatRect({0, 0}, sf::Vector2f(win.getSize()))));
            glm::vec2 sp; if (f.impact.alive && rend.project(f.impact.at, W, H, sp)) drawImpact(win, font, f.impact, sp);
            if (f.fx.warnK > 0.f && rend.project(glm::vec3(f.pE.root[3]) + glm::vec3(0, topOf(mE) - 0.12f, 0), W, H, sp)) drawWarnMark(win, font, sp, f.fx.warnK);
            // cinema: faixas pretas, REPLAY com a luz vermelha piscando; rebobinando, listras de fita
            const float bar = float(H) * 0.09f; sf::RectangleShape band({float(W), bar}); band.setFillColor(sf::Color::Black); win.draw(band); band.setPosition({0, float(H) - bar}); win.draw(band);
            if (rewinding) for (int i = 0; i < 7; i++) { const float y = std::fmod(float((replayT * 37 + i * 131) % H) + float(i) * 11.f, float(H)); sf::RectangleShape line({float(W), 2.f + float(i % 3)}); line.setPosition({0, y}); line.setFillColor(sf::Color(255, 255, 255, 70)); win.draw(line); }
            if ((replayT / 18) % 2 == 0 || rewinding) { sf::CircleShape dot(9.f); dot.setFillColor(sf::Color(230, 30, 40)); dot.setPosition({28.f, bar * 0.5f - 9.f}); win.draw(dot); }
            { const std::string label = rewinding ? "<< REBOBINANDO" : (replayPos >= float(replayHit - 16) && replayPos < float(replayHit + 14)) ? "REPLAY  -  CAMERA LENTA" : "REPLAY";
              sf::Text t(font, label, 30); t.setFillColor(sf::Color::White); t.setOutlineColor(sf::Color::Black); t.setOutlineThickness(2.f); t.setPosition({56.f, bar * 0.5f - 20.f}); win.draw(t);
              sf::Text skip(font, "ENTER: PULAR", 20); skip.setFillColor(sf::Color(200, 200, 200)); const auto b = skip.getLocalBounds(); skip.setPosition({float(W) - b.size.x - 28.f, float(H) - bar * 0.5f - 14.f}); win.draw(skip); }
            win.popGLStates(); };
        const auto tLoad = std::chrono::steady_clock::now();
        if (snap || (argc >= 3 && std::string(argv[1]) == "--bench")) { startFight(); step(0); }
        const double loadMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - tLoad).count();
        if (argc >= 3 && std::string(argv[1]) == "--bench") {   // --bench quadros: mede o tempo de quadro sem vsync e sem mostrar a janela
            win.setVisible(false); win.setVerticalSyncEnabled(false);
            const int frames = std::stoi(argv[2]); rend.profOn(); const auto t0 = std::chrono::steady_clock::now();
            int painP = 0, painE = 0; for (int i = 0; i < frames; i++) { step(i % 40 < 3 ? INPUT_PUNCH_R : 0); render(); rend.profCollect(); win.display(); if (std::string(faceOf(engine.player, downP)) == "pain") painP++; if (std::string(faceOf(engine.enemy, downE)) == "pain") painE++; }
            glFinish(); const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
            std::printf("%d quadros: %.2f ms por quadro (%.0f FPS) | GL %s\n", frames, ms / frames, 1000.0 * frames / ms, reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
            std::printf("placa, ms: sombra %.2f | cenario contorno %.2f cor %.2f | cordas %.2f | lutadores contorno %.2f cor %.2f | particulas %.2f | acabamento %.2f\n", rend.profMs(0), rend.profMs(1), rend.profMs(2), rend.profMs(3), rend.profMs(4), rend.profMs(5), rend.profMs(6), rend.profMs(7));
            std::printf("rosto de dor: jogador %d%%, oponente %d%%\n", painP * 100 / frames, painE * 100 / frames);
            std::printf("por quadro: %ld chamadas, %ld triangulos | carregar %.0f ms\n", rend.drawCalls / frames, rend.tris / frames, loadMs);
            return 0;
        }
        if (snap) {
            const int frames = std::stoi(argv[3]); const uint32_t mask = uint32_t(std::stoul(argv[4]));
            const bool holdKey = std::getenv("KORAMA3D_HOLD") != nullptr; int i = 0;
            // conferência dos especiais: KORAMA3D_SKILL=chave do especial (skills.cpp), KORAMA3D_SKILL_AT=quadro, KORAMA3D_SKILL_BY=p (jogador) ou e (oponente)
            const char* skill = std::getenv("KORAMA3D_SKILL"); const int skillAt = std::getenv("KORAMA3D_SKILL_AT") ? std::atoi(std::getenv("KORAMA3D_SKILL_AT")) : 360; const bool skillByP = std::getenv("KORAMA3D_SKILL_BY") && std::getenv("KORAMA3D_SKILL_BY")[0] == 'p';
            for (; i < frames; i++) { if (skill && i == skillAt) { engine.player.stamina = engine.enemy.stamina = 100.f; engine.castSkill(skillByP, skill); } step(holdKey || i % 24 < 3 ? mask : 0); }   // aperta e solta: o motor reage ao aperto
            // sexto argumento: continua até o primeiro acerto no oponente e para N quadros depois (conferir o impacto)
            // sétimo "kd": em vez do acerto, espera a primeira queda
            if (argc >= 6) { const bool kd = argc >= 7 && std::string(argv[6]) == "kd"; for (; i < 20000 && (kd ? engine.gameState != STATE_KNOCKDOWN : springE.s <= 0.f); i++) step(i % 24 < 3 ? mask : 0); for (int k = std::stoi(argv[5]); k > 0; k--, i++) step(0); }
            if (const char* cf = std::getenv("KORAMA3D_CELEB")) {   // conferência da comemoração: espera o fim da luta, pula o replay e anda N quadros nela
                for (int k = 0; k < 6000 && !engine.isMatchOver(); k++) step(k % 24 < 3 ? mask : 0); replayDone = true;
                for (int k = 0; k < 4000 && celebF < std::atoi(cf); k++) step(0); std::printf("comemoracao: quadro %d resultado %d rosas %zu\n", celebF, int(engine.result), throws.items.size()); }
            if (const char* rp = std::getenv("KORAMA3D_REPLAY")) {   // conferência do replay: espera o nocaute, entra no replay e anda N quadros nele
                for (int k = 0; k < 3000 && !(engine.isMatchOver() && endFrames >= 100); k++) step(0);
                if (replayBegin()) { for (int k = std::atoi(rp); k > 0 && replayT >= 0; k--) replayTick(); if (replayT >= 0) { renderReplay(); sf::Texture t; if (!t.resize(win.getSize())) return 1; t.update(win); std::printf("replay: quadro %d posicao %.1f golpe %d corte %d\n", replayT, replayPos, replayHit, shot); return t.copyToImage().saveToFile(argv[2]) ? 0 : 1; } }
                std::printf("replay nao comecou: fim %d gravados %zu\n", int(engine.isMatchOver()), rec.size()); }
            std::printf("topos: %.2f %.2f escala %.2f %.2f\n", topOf(fighterModel(engine.player)), topOf(fighterModel(engine.enemy)), fighterModel(engine.player).scale, fighterModel(engine.enemy).scale); std::printf("passos: jogadora %d, oponente %d\n", feetP.steps, feetE.steps);            std::printf("estado %d, fase do oponente %d, resultado %d\n", int(engine.gameState), engine.enemy.bossStage, int(engine.result));
            render();
            sf::Texture t; if (!t.resize(win.getSize())) throw std::runtime_error("textura do snap");
            t.update(win); if (!t.copyToImage().saveToFile(argv[2])) throw std::runtime_error(std::string("não salvou ") + argv[2]);
            return 0;
        }
        using clk = std::chrono::steady_clock; auto last = clk::now(); double acc = 0;
        // ---------------- menu do jogo original (app/menu_screens): título, seleção de lutador, modo, build e VS; a luta é a 3D ----------------
        MenuScreens menus(HudFonts{&font, &digits}, &sprites);
        for (uint32_t c = 0; c < 9; c++) menus.setLoadout(c, builds[c]);
        menus.takeDirty(); menus.goTo(Screen::Boot);
        bool inMenu = true; uint32_t tapLatch = 0; Screen menuScreenPrev = Screen::Fight;
        // ---------------- palco 3D da seleção: a arena do lutador em foco girando no fundo e o modelo dele parado na frente ----------------
        sf::Texture stageBg, stageFig; int stageChar = -1; uint32_t stageFrame = 0; RingRopes stageRopes; ClothSim stageCloth;
        // os modelos do elenco e das arenas vão carregando aos poucos no menu, um por quadro, para a troca de foco não travar
        std::vector<std::string> preload; for (int c = 0; c < 9; c++) { preload.push_back(c == 8 ? "batalhator1" : kFiles[c]); preload.push_back(std::string("arena_") + kArenas[c]); }
        const auto drawStage3D = [&](int focus) -> bool {
            if (focus < 0 || focus > 8) return false;
            rend.setImpactFrame(0.f); rend.setPunchIn(0.f, glm::vec3(0.f)); rend.setCameraKick(glm::vec3(0.f), 0.f); rend.setShock(glm::vec2(0.f), 2.f, 0.f); rend.setChroma(0.f);
            const int W = int(win.getSize().x), H = int(win.getSize().y);
            if (stageBg.getSize() != win.getSize() && !stageBg.resize(win.getSize())) return false;
            if (stageFig.getSize() != win.getSize() && !stageFig.resize(win.getSize())) return false;
            Model& m = modelOf(focus == 8 ? "batalhator1" : kFiles[focus]); Model& ar = modelOf(std::string("arena_") + kArenas[focus]);
            if (focus != stageChar) { stageRopes = {}; rend.setArena(ar, stageRopes); stageChar = focus; stageCloth = {}; }
            stageFrame++; const float t = float(stageFrame) / 60.f, top = topOf(m);
            Fighter f(uint32_t(focus), false); f.facing = kRigPi;   // de frente para a câmera
            PoseCtx c; c.hangT = float(stageFrame); c.hasPos = true; c.pos = glm::vec3(0.f);
            FighterPose pose = computePose(f, restFor(m), 0.f, c); FighterPose away = pose; away.root = glm::translate(glm::mat4(1.f), glm::vec3(0, -80.f, 0)) * pose.root;
            stageCloth.tick(glm::vec3(pose.root * glm::vec4(pose.upperPos, 1)), pose.root, pose.twist, t);
            win.setActive(true);
            // 1) arena: câmera dando a volta no ringue, devagar (panorâmica)
            const float a = t * 0.12f; rend.setTransparent(false); rend.setCamera(true, glm::vec3(std::sin(a) * 6.8f, 2.4f, std::cos(a) * 6.8f), glm::vec3(0, 1.1f, 0));
            rend.shadowPass(ar, m, away, stageCloth, t, m, away, stageCloth, t);
            rend.begin(W, H, 0.f, f, f, m.scale, m.scale, top, top, 0.f, stageFrame, 0.f, glm::vec3(0.f)); rend.arena(ar, t); rend.ropes(stageRopes); rend.finish(W, H);
            stageBg.update(win);
            // 2) só o lutador, com alfa: câmera de frente, com ele na esquerda da tela (onde ficava o retrato)
            const float dist = std::max(2.4f, top * 2.1f), xoff = 0.52f * std::tan(0.39f) * float(W) / float(H) * dist;
            rend.setTransparent(true); rend.setCamera(true, glm::vec3(0, top * 0.6f, -dist), glm::vec3(-xoff, top * 0.47f, 0));
            rend.shadowPass(ar, m, pose, stageCloth, t, m, away, stageCloth, t);
            rend.begin(W, H, 0.f, f, f, m.scale, m.scale, top, top, 0.f, stageFrame, 0.f, glm::vec3(0.f)); rend.fighter(m, pose, glm::vec3(0.f), stageCloth, t);
            rend.finishToTexture(stageFig.getNativeHandle(), W, H);
            rend.setTransparent(false); rend.setCamera(false);
            glBindVertexArray(0); glUseProgram(0); glDisable(GL_DEPTH_TEST); glDisable(GL_CULL_FACE);
            return true;
        };
        const auto drawMenu3D = [&]() {
            const int focus = menus.stageFocus(); const bool ok = drawStage3D(focus);
            menus.setStage3D(ok ? &stageBg : nullptr, ok ? &stageFig : nullptr);
            win.setActive(true); win.pushGLStates(); win.clear(sf::Color::Black); menus.draw(win); win.popGLStates();
        };
        // prévia: troca de golpe ou de lutador recomeça a luta de mentira (sem som, sem a encarada); o rival fica parado e ninguém perde vida
        const auto previewTick = [&]() {
            const SkillId want = menus.livePreviewSkill(); const SkillDef* def = SkillSystem::get(want);
            if (!def) { if (previewing) { previewing = false; stageChar = -1; menus.setLivePreview(nullptr); } return; }
            const int pc = int(std::min(menus.livePreviewChar(), 8u));
            if (previewing && (engine.gameState == STATE_KNOCKDOWN || engine.isMatchOver())) previewSkillCur = SkillId::None;   // alguém caiu: recomeça
            if (!previewing || pc != charP || want != previewSkillCur) {
                previewing = true; charP = pc; charE = pc == 4 ? 0 : 4; matchLoadout = builds[size_t(pc)]; matchArcade = false; matchTraining = false; matchStage = 1; previewSkillCur = want;
                startFight(); for (int k = 0; k < 600 && engine.gameState != STATE_FIGHTING; k++) step(0); previewWait = 12; stageChar = -1; }
            previewEnemyCam = menus.livePreviewFront(); if (def->type == SkillType::Passive) engine.player.loadout.passive = want;   // passiva sob o cursor: aparece equipada
            engine.player.hp = engine.player.maxHp; engine.enemy.hp = engine.enemy.maxHp; engine.player.stamina = engine.enemy.stamina = 100.f; engine.enemy.actionTimer = std::max(engine.enemy.actionTimer, 30);   // o rival não ataca
            if (engine.player.state == FIGHTER_IDLE && engine.gameState == STATE_FIGHTING && --previewWait <= 0) { engine.castSkill(true, def->key); previewWait = 45; }
            step(0);
        };
        // conferência do menu: --menu saida.png "Enter,Right,Enter" quadros  (as teclas vão só para o menu desta foto, de 20 em 20 quadros)
        if (argc >= 3 && std::string(argv[1]) == "--menu") {
            win.setVisible(false); std::vector<sf::Keyboard::Key> keys; { std::istringstream ks(argc >= 4 ? argv[3] : ""); std::string k;
                const std::map<std::string, sf::Keyboard::Key> names = {{"Enter", sf::Keyboard::Key::Enter}, {"Esc", sf::Keyboard::Key::Escape}, {"Left", sf::Keyboard::Key::Left}, {"Right", sf::Keyboard::Key::Right}, {"Up", sf::Keyboard::Key::Up}, {"Down", sf::Keyboard::Key::Down}, {"Z", sf::Keyboard::Key::Z}, {"X", sf::Keyboard::Key::X}, {"C", sf::Keyboard::Key::C}, {"Space", sf::Keyboard::Key::Space}, {"V", sf::Keyboard::Key::V}};
                while (std::getline(ks, k, ',')) if (names.count(k)) keys.push_back(names.at(k)); }
            const int frames = argc >= 5 ? std::atoi(argv[4]) : 240; size_t next = 0;
            for (int i = 0; i < frames; i++) { if (i >= 60 && i % 20 == 0 && next < keys.size()) { const MenuAction a = menus.onKey(keys[next++]); if (a.type == MenuAction::Type::StartMatch) std::printf("comecaria a luta: %u x %u\n", a.playerChar, a.enemyChar); } if (menus.screen() == Screen::Versus) menus.setVersusReady(true); menus.update(); sprites.pollUpload(); previewTick(); }
            for (int i = 0; i < 3; i++) { if (previewing) { render(); menus.setLivePreview(&previewTex); } drawMenu3D(); }   // alguns quadros para as sombras e o tecido assentarem
            std::printf("tela %d\n", int(menus.screen()));
            sf::Texture t; if (!t.resize(win.getSize())) return 1; t.update(win); return t.copyToImage().saveToFile(argv[2]) ? 0 : 1;
        }
        const auto syncBuilds = [&]() { if (!menus.takeDirty()) return; for (uint32_t c = 0; c < 9; c++) builds[c] = menus.loadouts()[c]; saveBuilds(buildFile, builds); };
        const auto toMenu = [&](Screen s) { menus.goTo(s); inMenu = true; last = clk::now(); acc = 0; if (audioOn) { audio.stopFightSounds(); audio.stopVersusSounds(); audio.setMusicPitch(1.f); audio.setMusicDuck(1.f); audio.setMusic(MusicTrack::Menu); } };
        const auto act = [&](const MenuAction& a) {
            if (audioOn) { audio.setSfxVolume(float(menus.sfxVolume())); audio.setMusicVolume(float(menus.musicVolume())); }
            if (a.type == MenuAction::Type::Quit) win.close();
            else if (a.type == MenuAction::Type::StartMatch) {
                charP = int(std::min(a.playerChar, 8u)); charE = int(std::min(a.enemyChar, 8u)); matchLoadout = a.loadout; matchArcade = a.arcade; matchStage = a.stage; matchTraining = a.training;
                menus.setStage3D(nullptr, nullptr); win.setActive(true); win.pushGLStates(); menus.draw(win); sf::Text t(font, "CARREGANDO...", 48); t.setPosition({500, 640}); t.setOutlineColor(sf::Color::Black); t.setOutlineThickness(3); win.draw(t); win.popGLStates(); win.display();
                previewing = false; menus.setLivePreview(nullptr); startFight(); step(0); inMenu = false; stageChar = -1; last = clk::now(); acc = 0;   // a luta troca a arena do renderer: o palco recarrega a dele na volta
            }
            syncBuilds();
        };
        // o menu desenha num espaço de 1600x900 com o mesmo enquadramento da janela
        const auto toUi = [&](sf::Vector2i px) { sf::View ui(sf::FloatRect({0, 0}, {1600, 900})); ui.setViewport(win.getView().getViewport()); return win.mapPixelToCoords(px, ui); };
        while (win.isOpen()) {
            while (auto ev = win.pollEvent()) {
                if (ev->is<sf::Event::Closed>()) win.close();
                if (const auto* r = ev->getIf<sf::Event::Resized>()) win.setView(sf::View(sf::FloatRect({0,0}, sf::Vector2f(r->size))));
                using K = sf::Keyboard::Key;
                if (inMenu) {
                    if (const auto* k = ev->getIf<sf::Event::KeyPressed>()) {
                        if (k->code == K::M) { if (audioOn) audio.toggleMute(); continue; }
                        const Screen before = menus.screen(); act(menus.onKey(k->code));
                        if (audioOn && inMenu) { if (k->code == K::Enter || k->code == K::Space || k->code == K::Z) audio.play(Sfx::Select); else if (k->code == K::Up || k->code == K::Down || k->code == K::Left || k->code == K::Right || menus.screen() != before) audio.play(Sfx::MenuNav); }
                    }
                    else if (const auto* mv = ev->getIf<sf::Event::MouseMoved>()) { const sf::Vector2f q = toUi(mv->position); menus.onMouseMove(q); if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) { menus.onMouseDrag(q); act({}); } }
                    else if (const auto* c = ev->getIf<sf::Event::MouseButtonPressed>()) { const Screen before = menus.screen(); if (c->button == sf::Mouse::Button::Left) act(menus.onClick(toUi(c->position))); else if (c->button == sf::Mouse::Button::Right) act(menus.onRightClick()); if (audioOn && inMenu && menus.screen() != before) audio.play(Sfx::Select); }
                    continue;
                }
                std::optional<K> code; if (const auto* kp = ev->getIf<sf::Event::KeyPressed>()) code = kp->code;
                else if (const auto* c = ev->getIf<sf::Event::MouseButtonPressed>(); c && c->button == sf::Mouse::Button::Left && hud2d && celebF >= kCardDelay) { const int b = hud2d->resultButtonAt(toUi(c->position)); if (b == 0) code = K::Enter; else if (b == 1) code = K::Escape; }   // botões do card
                if (!code) continue; const struct { K code; } kk{*code}; const auto* k = &kk;
                if (replayT >= 0) { if (k->code == K::Enter || k->code == K::Space || k->code == K::Escape) replayFinish(); continue; }
                tapLatch |= keyBit(k->code);
                if (k->code == K::M) { if (audioOn) audio.toggleMute(); continue; }
                if (engine.isMatchOver() && k->code == K::Enter) {
                    if (!matchArcade) { startFight(); step(0); }   // revanche
                    else {   // torneio: a chave avança na vitória e acaba na derrota, como no jogo 2D
                        const MatchResult r = engine.result; if (r == MatchResult::WinKO || r == MatchResult::WinTKO || r == MatchResult::WinDecision) menus.tournamentWon(); else menus.tournamentLost();
                        inMenu = true; if (menus.tournament().active() || menus.tournament().champion()) menus.showBracket(); else menus.newCampaign(); syncBuilds();
                    }
                } else if (k->code == K::Escape) toMenu(engine.isMatchOver() ? Screen::Title : Screen::ModeSelect);
            }
            if (!win.isOpen()) break;
            const auto now = clk::now(); acc += std::min(0.1, std::chrono::duration<double>(now-last).count()); last = now;
            if (inMenu) {
                // o VS do original espera os sprites da luta 2D; aqui os modelos carregam ao começar, então ele pode seguir
                while (acc >= kTick && inMenu) {
                    if (menus.screen() == Screen::Versus) menus.setVersusReady(true);
                    if (audioOn) {
                        const Screen cur = menus.screen();
                        if (cur != menuScreenPrev) { if (menuScreenPrev == Screen::Versus) audio.stopVersusSounds(); if (menuScreenPrev == Screen::Title) audio.stop(Sfx::AnnouncerKorama); if (cur == Screen::Title) audio.play(Sfx::AnnouncerKorama); menuScreenPrev = cur; }
                        audio.setMusic(MusicTrack::Menu);
                        if (cur == Screen::Versus) { const int sf = menus.screenFrame(); if (sf == 1) audio.play(Sfx::VersusWhoosh); else if (sf == 24) { audio.play(Sfx::VersusClash); audio.play(Sfx::CameraFlash); audio.play(Sfx::VersusSlam); } if (sf >= 24) audio.setMusicDuck(0.4f); }
                        else audio.setMusicDuck(1.f);
                    }
                    act(menus.update()); sprites.pollUpload(); if (inMenu) previewTick(); acc -= kTick; }
                if (!inMenu) continue;
                if (!preload.empty()) { modelOf(preload.back()); preload.pop_back(); }
                if (previewing) { render(); menus.setLivePreview(&previewTex); } else menus.setLivePreview(nullptr);
                drawMenu3D(); win.display(); continue;
            }
            if (replayT < 0 && !replayDone && engine.isMatchOver() && koDecisive() && endFrames >= 100) { if (!replayBegin()) replayDone = true; }
            if (replayT >= 0) { while (acc >= kTick && replayT >= 0) { replayTick(); acc -= kTick; } if (replayT >= 0) { renderReplay(); win.display(); continue; } }
            while (acc >= kTick) { step(withDodge((win.hasFocus() ? readInput() : 0) | tapLatch)); tapLatch = 0; acc -= kTick; }
            render(); win.display();
        }
    } catch (const std::exception& e) {
        std::fprintf(stderr, "korama3d: %s\n", e.what());
        return 1;
    }
    return 0;
}
