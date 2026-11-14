// Desenho dos menus no espaço 1600x900, seguindo o css/menus.css e o hub do index.html: fundo
// noturno com grade retrô em perspectiva, logo K.O.-RAMA em Titan One com extrusão e brilho que
// atravessa as letras, cards do elenco inclinados, painéis que entram com mola, build em card
// grande com prévia no ringue, chaveamento do torneio e os modais de configurações e de saída.
#include "menu_screens.hpp"
#include "canvas2d.hpp"
#include "character_info.hpp"
#include "config.hpp"
#include "ui_draw.hpp"
#include "char_icon.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>

namespace KoRama {

namespace {

using cv::Canvas;
constexpr float kPi = 3.14159265f;

std::string upper(const std::string& s) {
    std::string out;
    for (std::size_t i = 0; i < s.size(); ++i) {
        unsigned char ch = static_cast<unsigned char>(s[i]);
        if (ch == 0xC3 && i + 1 < s.size()) {
            unsigned char nx = static_cast<unsigned char>(s[i + 1]);
            if (nx >= 0xA0 && nx <= 0xBE && nx != 0xB7) nx -= 0x20;
            out += static_cast<char>(ch);
            out += static_cast<char>(nx);
            ++i;
        } else {
            out += static_cast<char>(std::toupper(ch));
        }
    }
    return out;
}

float springOut(float t) { // cubic-bezier(0.34, 1.56, 0.64, 1) aproximado
    t = std::clamp(t, 0.0f, 1.0f);
    return 1.0f + 2.56f * std::pow(t - 1.0f, 3.0f) + 1.56f * std::pow(t - 1.0f, 2.0f);
}

// Ícone do lutador em silhueta (cor = fillStyle atual), usado no lugar dos pontos das retículas.
void charIcon(Canvas& ctx, int charId, float cx, float cy, float s) {
    drawCharInsignia(ctx, charId, cx, cy, s);
}

void inkBox(Canvas& ctx, float x, float y, float w, float h, const cv::Style& fill, float border, float shadow, float radius = 0,
            const char* borderColor = "#000000") {
    if (shadow > 0) {
        ctx.fillStyle = "#000000";
        ctx.beginPath();
        if (radius > 0) ctx.roundRect(x + shadow, y + shadow, w, h, radius); else ctx.rect(x + shadow, y + shadow, w, h);
        ctx.fill();
    }
    ctx.beginPath();
    if (radius > 0) ctx.roundRect(x, y, w, h, radius); else ctx.rect(x, y, w, h);
    ctx.fillStyle = fill;
    ctx.fill();
    if (border > 0) { ctx.strokeStyle = borderColor; ctx.lineWidth = border; ctx.stroke(); }
}

void hatch(Canvas& ctx, float x, float y, float w, float h, float spacing, const char* color, float lw) {
    ctx.hatchRect(x, y, w, h, spacing, cv::Style(color), lw, false);
}

void outlined(Canvas& ctx, const std::string& s, float x, float y, const char* fill, float stroke, float shadow) {
    if (shadow > 0) { ctx.fillStyle = "#000000"; ctx.fillText(s, x + shadow, y + shadow); }
    if (stroke > 0) { ctx.strokeStyle = "#000000"; ctx.lineWidth = stroke * 2; ctx.lineJoin = "round"; ctx.strokeText(s, x, y); }
    ctx.fillStyle = fill;
    ctx.fillText(s, x, y);
}

// Botão com cantos cortados (clip-path do .title-btn) e hachura diagonal.
void notchButton(Canvas& ctx, float x, float y, float w, float h, const char* fill, const char* textColor, const std::string& label,
                 const std::string& sub, bool selected, const char* font = "17.6px 'Titan One'") {
    const float lift = selected ? 3.0f : 0.0f, sh = selected ? 9.0f : 6.0f, c = 10;
    x -= lift; y -= lift;
    auto path = [&](float ox, float oy) {
        ctx.beginPath();
        ctx.moveTo(ox, oy); ctx.lineTo(ox + w - c, oy); ctx.lineTo(ox + w, oy + c); ctx.lineTo(ox + w, oy + h); ctx.lineTo(ox + c, oy + h); ctx.lineTo(ox, oy + h - c);
        ctx.closePath();
    };
    ctx.fillStyle = "#000000"; path(x + sh, y + sh); ctx.fill();
    path(x, y); ctx.fillStyle = fill; ctx.fill();
    ctx.save(); path(x, y); ctx.clip();
    ctx.strokeStyle = "rgba(0,0,0,0.08)"; ctx.lineWidth = 3;
    ctx.beginPath();
    for (float k = -h; k < w + h; k += 7) { ctx.moveTo(x + k, y + h); ctx.lineTo(x + k + h, y); }
    ctx.stroke();
    if (selected) { ctx.fillStyle = "rgba(255,255,255,0.10)"; ctx.fillRect(x, y, w, h); }
    ctx.restore();
    path(x, y); ctx.strokeStyle = selected ? "#ffffff" : "#000000"; ctx.lineWidth = 4; ctx.stroke();
    ctx.fillStyle = textColor; ctx.textAlign = "center"; ctx.textBaseline = "middle";
    if (sub.empty()) {
        ctx.font = font;
        ctx.fillText(label, x + w / 2, y + h / 2 + 1);
    } else {
        ctx.font = font;
        ctx.fillText(label, x + w / 2, y + h / 2 - 7);
        ctx.font = "800 11px 'Outfit ExtraBold'";
        ctx.fillText(sub, x + w / 2, y + h / 2 + 13);
    }
}

void pill(Canvas& ctx, float x, float y, const std::string& text, const char* bg, const char* fg, const char* font, float padX = 8,
          float h = 20, float radius = 3, bool alignRight = false) {
    ctx.font = font;
    const float w = ctx.measureText(text) + padX * 2;
    if (alignRight) x -= w;
    inkBox(ctx, x, y, w, h, cv::Style(bg), 2, 2, radius);
    ctx.fillStyle = fg; ctx.textAlign = "center"; ctx.textBaseline = "middle";
    ctx.fillText(text, x + w / 2, y + h / 2 + 1);
}

// Etiqueta (SWARM, HONEY...) com a cor do tipo de habilidade.
std::string hexColor(sf::Color c) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "#%02x%02x%02x", c.r, c.g, c.b);
    return buf;
}

float commandText(Canvas& ctx, const SkillDef& def, float x, float cy, bool alignRight, const char* bg = "#ffe169",
                  const std::string& punchL = "Z", const std::string& punchR = "X") {
    enum Tok { L, R, U, D, PLUS, PUNCH_L, PUNCH_R, CHAIN2, CHAIN3 };   // mesma ordem do CmdTok (skills.hpp)
    std::vector<Tok> toks;
    for (CmdTok t : SkillSystem::commandTokens(def)) toks.push_back(static_cast<Tok>(t));
    ctx.font = "900 11px Montserrat";
    float contentW = 0.0f;
    for (Tok t : toks) {
        if (t <= D) {
            contentW += 11.0f;
        } else if (t >= CHAIN2) {
            contentW += (t == CHAIN3 ? 3 : 2) * 7.0f + 10.0f;
        } else if (t == PLUS) {
            contentW += 9.0f;
        } else {
            const std::string& lbl = (t == PUNCH_L ? punchL : punchR);
            contentW += std::max(11.0f, ctx.measureText(lbl) + 4.0f);
        }
    }
    const float w = 12.0f + contentW, h = 20.0f;
    const float bx = alignRight ? x - w : x;
    inkBox(ctx, bx, cy - h / 2.0f, w, h, cv::Style(bg), 2, 2, 4);
    ctx.fillStyle = "#000000";
    ctx.textBaseline = "middle";
    float tx = bx + 6.0f;
    for (Tok t : toks) {
        float tokenW = 11.0f;
        if (t <= D) {
            tokenW = 11.0f;
            const float c = tx + tokenW * 0.5f, s = 4.0f;
            ctx.beginPath();
            if (t == L) { ctx.moveTo(c - s, cy); ctx.lineTo(c + s, cy - s); ctx.lineTo(c + s, cy + s); }
            else if (t == R) { ctx.moveTo(c + s, cy); ctx.lineTo(c - s, cy - s); ctx.lineTo(c - s, cy + s); }
            else if (t == U) { ctx.moveTo(c, cy - s); ctx.lineTo(c + s, cy + s); ctx.lineTo(c - s, cy + s); }
            else if (t == D) { ctx.moveTo(c, cy + s); ctx.lineTo(c + s, cy - s); ctx.lineTo(c - s, cy - s); }
            ctx.closePath();
            ctx.fill();
        } else if (t >= CHAIN2) {   // combo de entrada: uma bolinha por soco (jab, direto e, com três, gancho) e depois o finalizador
            const int n = t == CHAIN3 ? 3 : 2;
            tokenW = n * 7.0f + 10.0f;
            for (int i = 0; i < n; i++) { ctx.beginPath(); ctx.arc(tx + 3.5f + i * 7.0f, cy, 2.6f, 0.0f, 6.2832f); ctx.fill(); }
            ctx.textAlign = "center";
            ctx.fillText(">", tx + n * 7.0f + 4.5f, cy + 1.0f);
        } else if (t == PLUS) {
            tokenW = 9.0f;
            const float c = tx + tokenW * 0.5f;
            ctx.textAlign = "center";
            ctx.fillText("+", c, cy + 1.0f);
        } else {
            const std::string& lbl = (t == PUNCH_L ? punchL : punchR);
            tokenW = std::max(11.0f, ctx.measureText(lbl) + 4.0f);
            const float c = tx + tokenW * 0.5f;
            ctx.textAlign = "center";
            ctx.fillText(lbl, c, cy + 1.0f);
        }
        tx += tokenW;
    }
    return w;
}

void wrapText(Canvas& ctx, const std::string& text, float x, float y, float maxW, float lineH) {
    std::string cur, word;
    auto flush = [&](bool force) {
        if (!word.empty()) {
            std::string test = cur.empty() ? word : cur + " " + word;
            if (ctx.measureText(test) > maxW && !cur.empty()) { ctx.fillText(cur, x, y); y += lineH; cur = word; }
            else cur = test;
            word.clear();
        }
        if (force && !cur.empty()) { ctx.fillText(cur, x, y); y += lineH; cur.clear(); }
    };
    for (char ch : text) { if (ch == ' ') flush(false); else word += ch; }
    flush(true);
}

void panelCard(Canvas& ctx, float x, float y, float w, float h, float radius = 14) {
    inkBox(ctx, x, y, w, h, cv::Style("rgba(18, 13, 32, 0.96)"), 4, 8, radius);
    hatch(ctx, x + 4, y + 4, w - 8, h - 8, 7, "rgba(255,255,255,0.025)", 2);
}

void neonFrame(Canvas& ctx, float x, float y, float w, float h, float r, sf::Color c) {
    // Brilho neon suave: contornos cada vez mais largos e transparentes, depois a borda nítida.
    for (int i = 6; i >= 1; --i) {
        ctx.beginPath(); ctx.roundRect(x - i * 3.0f, y - i * 3.0f, w + i * 6.0f, h + i * 6.0f, r + i * 3.0f);
        ctx.strokeStyle = cv::rgba(c.r, c.g, c.b, 0.05f * (7 - i) / 2.0f);
        ctx.lineWidth = 6;
        ctx.stroke();
    }
    ctx.beginPath(); ctx.roundRect(x, y, w, h, r);
    ctx.fillStyle = "#0d0a14"; ctx.fill();
    ctx.strokeStyle = cv::Style(c); ctx.lineWidth = 3.5f; ctx.stroke();
}

} // namespace

// ---------------------------------------------------------------------------
// Fundo e logo
// ---------------------------------------------------------------------------

void MenuScreens::drawBackground(Canvas& ctx) {
    ctx.fillStyle = "#090610";
    ctx.fillRect(0, 0, 1600, 900);
    // Grade retrô em perspectiva (rotateX 60°) andando para a frente: pontos dourados e linhas magenta.
    const float horizon = 230, scroll = std::fmod(frame_ / (60.0f * 8.0f), 1.0f);
    for (int row = 0; row < 26; ++row) {
        const float d = (row + scroll) / 26.0f;             // 0 = horizonte, 1 = perto
        const float y = horizon + std::pow(d, 2.2f) * (900 - horizon + 120);
        if (y > 900) continue;
        const float spread = 0.25f + d * 1.6f;
        ctx.fillStyle = cv::rgba(255, 0, 127, 0.05f + 0.06f * d);
        ctx.fillRect(0, y, 1600, std::max(1.0f, d * 1.5f));
        const float step = 28 * spread;
        ctx.fillStyle = cv::rgba(255, 225, 105, 0.05f + 0.12f * d);
        for (float x = std::fmod(800.0f, step) - step; x < 1600; x += step) {
            const float r = 0.6f + 1.3f * d;
            ctx.fillRect(x - r, y + step * 0.3f - r, r * 2, r * 2);
        }
    }
    // Leve escurecimento no topo: degradê vertical direto em 2 triângulos (sem passar pelo stencil)
    const sf::Color cTop(9, 6, 16, 255), cBot(9, 6, 16, 0);
    const sf::Vertex gradV[6] = {
        {{0.0f, 0.0f}, cTop}, {{1600.0f, 0.0f}, cTop}, {{1600.0f, 420.0f}, cBot},
        {{0.0f, 0.0f}, cTop}, {{1600.0f, 420.0f}, cBot}, {{0.0f, 420.0f}, cBot}
    };
    ctx.enqueue(gradV, 6, sf::BlendAlpha, nullptr);
}

void MenuScreens::drawLogo(Canvas& ctx, float cx, float top, float scale) {
    // SVG original: viewBox 900x420, Titan One, extrusão em 9 camadas e brilho que varre as letras a cada 4,2 s.
    struct Glyph { const char* ch; float x, y, size; bool rama; };
    static const Glyph kGlyphs[] = {{"K", 295, 160, 145, false}, {".", 390, 160, 145, false}, {"O", 465, 160, 145, false},
                                    {"-", 570, 160, 145, false}, {"R", 210, 340, 175, true},  {"A", 340, 340, 175, true},
                                    {"M", 475, 340, 175, true},  {"A", 625, 340, 175, true}};
    static const char* kGold[] = {"#000000", "#3e2800", "#6c4600", "#8a5a00", "#996400", "#a86e00", "#b87800", "#c78200", "#d68c00"};
    static const char* kMag[] = {"#000000", "#2a0016", "#4a0028", "#62003a", "#7a0048", "#920056", "#aa0064", "#c20072", "#d6007c"};
    const float cycle = std::fmod(frame_ / 60.0f, 4.2f) / 4.2f;
    const float sweep = cycle < 0.58f ? cycle / 0.58f : 1.0f;
    const float x1 = -350 + 1500 * sweep;
    ctx.save();
    ctx.translate(cx - 450 * scale, top);
    ctx.scale(scale, scale);
    const float t = frame_ / 60.0f;
    const float tSec = static_cast<float>(screenFrame_) / 60.0f;
    for (int i = 0; i < 8; ++i) {
        const Glyph& g = kGlyphs[i];
        // Entrada em "slam" sincronizada com a fala do narrador no Menu Principal:
        // 0.2s -> "K." (i = 0, 1)
        // 1.4s -> "O-" (i = 2, 3)
        // 2.4s -> "RAMA" (i = 4, 5, 6, 7)
        float appear = 1.0f;
        if (screen_ == Screen::Title) {
            float startTime = 0.0f;
            if (i == 0 || i == 1) startTime = 0.20f;
            else if (i == 2 || i == 3) startTime = 1.40f;
            else startTime = 2.40f + (i - 4) * 0.05f;

            if (tSec < startTime) appear = 0.0f;
            else appear = std::clamp((tSec - startTime) / 0.22f, 0.0f, 1.0f);
        }
        if (appear <= 0) continue;
        float sc = 1, dy = 0, rot = 0;
        if (appear < 1) {
            if (appear < 0.7f) { const float u = appear / 0.7f; sc = 3.0f + (0.92f - 3.0f) * u; dy = 120 + (-10 - 120) * u; }
            else { const float u = (appear - 0.7f) / 0.3f; sc = 0.92f + 0.08f * u; dy = -10 + 10 * u; }
        } else {
            const float ph = t / 3.0f * 2 * kPi - i * 0.42f;
            dy = std::sin(ph) * 5.5f;
            rot = std::sin(ph + 0.6f) * 2.2f * kPi / 180;
        }
        ctx.save();
        ctx.globalAlpha = std::min(1.0f, appear * 1.6f);
        const float gx = g.x + g.size * 0.32f, gy = g.y - g.size * 0.35f;
        ctx.translate(gx, gy + dy);
        ctx.rotate(rot);
        ctx.scale(sc, sc);
        ctx.translate(-gx, -gy);
        ctx.font = std::to_string(static_cast<int>(g.size)) + "px 'Titan One'";
        ctx.textAlign = "left"; ctx.textBaseline = "alphabetic";
        const char** ramp = g.rama ? kMag : kGold;
        for (int layer = 0; layer < 9; ++layer) {
            const float off = 18 - layer * 2;
            ctx.fillStyle = ramp[layer];
            ctx.fillText(g.ch, g.x + off, g.y + off);
        }
        const char* base = g.rama ? "#ff007f" : "#ffe169";
        ctx.strokeStyle = "#000000"; ctx.lineWidth = g.rama ? 12 : 10; ctx.lineJoin = "round";
        ctx.strokeText(g.ch, g.x, g.y);
        ctx.fillStyle = base;
        ctx.fillText(g.ch, g.x, g.y);
        // Brilho branco que varre as letras (o texto não aceita gradiente: sobrepõe branco com alfa).
        const float center = x1 + 125, gc = g.x + g.size * 0.32f;
        const float glint = std::max(0.0f, 1.0f - std::fabs(center - gc) / (g.size * 0.45f));
        if (glint > 0.01f) {
            ctx.save();
            ctx.globalAlpha *= glint * 0.85f;
            ctx.fillStyle = "#ffffff";
            ctx.fillText(g.ch, g.x, g.y);
            ctx.restore();
        }
        ctx.restore();
    }
    ctx.restore();
}

void MenuScreens::drawNotice(Canvas& ctx, float y) {
    if (notice_.empty()) return;
    const float a = std::min(1.0f, noticeTimer_ / 20.0f);
    ctx.save();
    ctx.globalAlpha = a;
    ctx.font = "800 17px 'Outfit ExtraBold'";
    const float w = ctx.measureText(notice_) + 40;
    inkBox(ctx, 800 - w / 2, y, w, 36, cv::Style("#ff6600"), 3, 5, 8);
    ctx.fillStyle = "#000000"; ctx.textAlign = "center"; ctx.textBaseline = "middle";
    ctx.fillText(notice_, 800, y + 19);
    ctx.restore();
}

// ---------------------------------------------------------------------------
// Tela principal
// ---------------------------------------------------------------------------

void MenuScreens::draw(sf::RenderTarget& target) {
    const sf::View prev = target.getView();
    sf::View ui(sf::FloatRect({0.0f, 0.0f}, {1600.0f, 900.0f}));
    ui.setViewport(prev.getViewport());
    target.setView(ui);
    {
        Canvas ctx(target);
        if (screen_ != Screen::Versus) drawBackground(ctx);
        if (screen_ == Screen::Title) {
            drawTitle(target, ctx);
            const float b = logoBlend();
            const float big = 480.0f / 900.0f, small = big * 0.68f;
            drawLogo(ctx, 800, 108 + (4 - 108) * b, big + (small - big) * b);
        }
        switch (screen_) {
        case Screen::Boot: drawBoot(target, ctx); break;
        case Screen::Title: break;
        case Screen::CharSelect: drawCharSelect(target, ctx); break;
        case Screen::ModeSelect: drawModeSelect(target, ctx); break;
        case Screen::Build: drawBuild(target, ctx); break;
        case Screen::Versus: drawVersus(target, ctx); break;
        case Screen::Bracket: drawBracket(target, ctx); break;
        case Screen::CampaignPrompt: drawModeSelect(target, ctx); drawCampaignPrompt(ctx); break;
        case Screen::Fight: break;
        }
        if (modal_ == Modal::Settings) drawSettings(ctx);
        else if (modal_ == Modal::Exit) drawExit(ctx);
        // Scanlines CRT do .crt-overlay em lote único estático
        static const auto s_scanTris = []() {
            std::vector<sf::Vertex> v;
            v.reserve(225 * 6);
            const sf::Color scan(0, 0, 0, 23); // ~0.09 alfa
            for (float y = 2.0f; y < 900.0f; y += 4.0f) {
                v.push_back({{0.0f, y}, scan});
                v.push_back({{1600.0f, y}, scan});
                v.push_back({{1600.0f, y + 2.0f}, scan});
                v.push_back({{0.0f, y}, scan});
                v.push_back({{1600.0f, y + 2.0f}, scan});
                v.push_back({{0.0f, y + 2.0f}, scan});
            }
            return v;
        }();
        ctx.enqueue(s_scanTris.data(), s_scanTris.size(), sf::BlendAlpha, nullptr);
        ctx.flush();
    }
    target.setView(prev);
}

void MenuScreens::warmUp(sf::RenderTarget& target) {
    // Desenha cada tela uma vez fora da vista para criar textos, glifos e texturas antes de o
    // jogador navegar (evita o tranco no primeiro quadro de cada transição).
    const Screen screens[] = {Screen::Title, Screen::CharSelect, Screen::ModeSelect, Screen::Build, Screen::Versus, Screen::Bracket};
    for (Screen sc : screens) {
        goTo(sc);
        for (int i = 0; i < 3; ++i) (void)update();
        draw(target);
        for (int i = 0; i < 80; ++i) (void)update();
        draw(target);
    }
    openModal(Modal::Settings); draw(target);
    openModal(Modal::Exit); draw(target);
    modal_ = Modal::None;
    goTo(Screen::Title);
    prevScreen_ = Screen::Title;
    frame_ = 0;
}

namespace { void modeIcon(Canvas& ctx, int mode, float time, const char* ink); }

void MenuScreens::drawBoot(sf::RenderTarget& /*target*/, Canvas& ctx) {
    // -----------------------------------------------------------------------
    // LOGO KO-RAMA no topo com brilho varrendo
    // -----------------------------------------------------------------------
    drawLogo(ctx, 800, 42, 0.40f);

    ctx.save();
    ctx.font = "900 13px Montserrat";
    ctx.textAlign = "center";
    ctx.textBaseline = "middle";
    ctx.fillStyle = "rgba(255, 225, 105, 0.75)";
    ctx.fillText("ARCADE BOXING REVOLUTION", 800, 214);
    ctx.restore();

    // -----------------------------------------------------------------------
    // CICLO DAS 10 INSÍGNIAS OFICIAIS DOS PERSONAGENS (18 frames cada = 180 frames)
    // -----------------------------------------------------------------------
    const int charId = std::min(9, static_cast<int>(screenFrame_ / 18));
    const int localF = screenFrame_ % 18;
    const CharacterInfo& info = rosterInfo(charId);
    const sf::Color col = info.color;

    const float cx = 800.0f;
    const float cy = 390.0f;

    // FÍSICA E JUICE DA ESTAMPAGEM (Squash & Stretch, Impact Jitter, Overshoot)
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    float jitterX = 0.0f;
    float jitterY = 0.0f;
    float tilt = 0.0f;

    if (localF < 5) {
        // Impacto de entrada com mola e tremor
        const float snap = 1.0f + 0.38f * std::exp(-localF * 0.85f) * std::cos(localF * 1.55f);
        scaleX = scaleY = snap;
        if (localF == 0) { jitterX = 3.5f; jitterY = -2.5f; }
        else if (localF == 1) { jitterX = -2.0f; jitterY = 1.5f; }
    } else if (localF < 15) {
        // Respiração e leve inclinação heroica
        const float breathe = std::sin(localF * 0.45f);
        scaleX = 1.0f + 0.03f * breathe;
        scaleY = 1.0f - 0.03f * breathe;
        tilt = std::sin(localF * 0.35f) * 0.035f;
    } else {
        // Antecipação: achata antes da próxima estampagem
        const float u = static_cast<float>(localF - 15) / 2.0f;
        scaleX = 1.0f + 0.20f * std::sin(u * kPi);
        scaleY = 1.0f - 0.16f * std::sin(u * kPi);
    }

    // 1. Explosão cinética de retículas ao bater na tela (Halftone Impact Burst)
    if (localF < 8) {
        const float burstDist = 110.0f + localF * 16.0f;
        const float burstAlpha = (1.0f - static_cast<float>(localF) / 8.0f) * 0.65f;
        ctx.fillStyle = cv::rgba(col.r, col.g, col.b, burstAlpha);
        for (int i = 0; i < 16; ++i) {
            const float a = i * (2.0f * kPi / 16.0f) + localF * 0.05f;
            const float bx = cx + std::cos(a) * burstDist;
            const float by = cy + std::sin(a) * burstDist;
            const float br = std::max(1.5f, 6.0f * (1.0f - static_cast<float>(localF) / 8.0f));
            ctx.beginPath(); ctx.arc(bx, by, br, 0.0f, 2.0f * kPi); ctx.fill();
        }
    }

    ctx.save();
    ctx.translate(cx + jitterX, cy + jitterY);
    ctx.rotate(tilt);

    // 2. Placa / Selo Heráldico de Quadrinho (Hero Shield Badge)
    const float plateW = 210.0f * scaleX, plateH = 210.0f * scaleY;
    inkBox(ctx, -plateW * 0.5f, -plateH * 0.5f, plateW, plateH, cv::Style("#0f091d"), 4.0f, 10.0f, 26.0f, hexColor(col).c_str());

    // 3. Retícula de meio-tom estilizada dentro do selo (Halftone Dots Grid)
    ctx.fillStyle = cv::rgba(col.r, col.g, col.b, 0.16f);
    for (float dy = -75.0f; dy <= 75.0f; dy += 22.0f) {
        for (float dx = -75.0f; dx <= 75.0f; dx += 22.0f) {
            const float d = std::sqrt(dx * dx + dy * dy);
            if (d < 85.0f) {
                const float dr = 2.0f + 2.5f * (1.0f - d / 85.0f);
                ctx.beginPath(); ctx.arc(dx * scaleX, dy * scaleY, dr, 0.0f, 2.0f * kPi); ctx.fill();
            }
        }
    }

    // 4. A INSÍGNIA OFICIAL E REAL DO LUTADOR (drawCharInsignia fiel do jogo!)
    const float insigniaScale = 82.0f * scaleX;

    // Sombra dura da insígnia
    ctx.fillStyle = "#000000";
    drawCharInsignia(ctx, charId, 6.0f, 6.0f, insigniaScale);

    // Corpo vibrante na cor oficial do personagem
    ctx.fillStyle = hexColor(col);
    drawCharInsignia(ctx, charId, 0.0f, 0.0f, insigniaScale);

    // Brilho especular sutil na borda superior
    ctx.fillStyle = "rgba(255, 255, 255, 0.32)";
    drawCharInsignia(ctx, charId, -2.5f, -2.5f, insigniaScale * 0.96f);

    ctx.restore();

    // -----------------------------------------------------------------------
    // PLACA DO NOME E ARQUÉTIPO DO LUTADOR
    // -----------------------------------------------------------------------
    ctx.save();
    const std::string nameText = upper(info.name);
    const std::string archText = upper(info.archetype);

    ctx.font = "26px 'Titan One'";
    ctx.textAlign = "center";
    ctx.textBaseline = "middle";
    const float nameW = std::max(320.0f, ctx.measureText(nameText) + 50.0f);
    const float nameH = 56.0f;
    const float nameY = 530.0f;

    // Moldura do nome
    inkBox(ctx, cx - nameW * 0.5f, nameY, nameW, nameH, cv::Style("#100a1c"), 3.5f, 6.0f, 8.0f, hexColor(col).c_str());

    // Nome em destaque
    ctx.fillStyle = "#ffffff";
    ctx.fillText(nameText, cx, nameY + 20.0f);

    // Arquétipo / Estilo de luta
    ctx.font = "900 11px Montserrat";
    ctx.fillStyle = hexColor(col);
    ctx.fillText(archText, cx, nameY + 42.0f);
    ctx.restore();

    // -----------------------------------------------------------------------
    // BARRA DE CARREGAMENTO ESTILIZADA DE GIBI (0% a 100%)
    // -----------------------------------------------------------------------
    const float progress = std::min(1.0f, static_cast<float>(screenFrame_) / 180.0f);
    const float barW = 660.0f;
    const float barH = 26.0f;
    const float barX = cx - barW * 0.5f;
    const float barY = 640.0f;

    // Fundo da barra com sombra
    inkBox(ctx, barX, barY, barW, barH, cv::Style("#120d20"), 4.0f, 6.0f, 6.0f);

    // Hachuras escuras dentro da barra não preenchida
    hatch(ctx, barX + 3.0f, barY + 3.0f, barW - 6.0f, barH - 6.0f, 14.0f, "rgba(255, 255, 255, 0.06)", 2.0f);

    // Preenchimento animado da barra
    if (progress > 0.01f) {
        const float fillW = (barW - 6.0f) * progress;
        auto barGrad = ctx.createLinearGradient(barX, 0, barX + barW, 0);
        barGrad->addColorStop(0.0f, sf::Color(0xff, 0xd7, 0x00)); // Dourado
        barGrad->addColorStop(0.5f, sf::Color(0xff, 0x00, 0x7f)); // Magenta
        barGrad->addColorStop(1.0f, sf::Color(0x00, 0xf5, 0xd4)); // Ciano
        ctx.fillStyle = barGrad;
        ctx.beginPath();
        ctx.roundRect(barX + 3.0f, barY + 3.0f, fillW, barH - 6.0f, 4.0f);
        ctx.fill();

        // Linhas de velocidade dentro da barra preenchida
        ctx.fillStyle = "rgba(255, 255, 255, 0.22)";
        const float stripeStep = 20.0f;
        const float stripeOffset = std::fmod(frame_ * 1.5f, stripeStep);
        for (float sx = barX + 3.0f + stripeOffset; sx < barX + 3.0f + fillW; sx += stripeStep) {
            ctx.beginPath();
            ctx.moveTo(sx, barY + 3.0f);
            ctx.lineTo(sx + 8.0f, barY + barH - 3.0f);
            ctx.lineTo(sx + 4.0f, barY + barH - 3.0f);
            ctx.lineTo(sx - 4.0f, barY + 3.0f);
            ctx.closePath();
            ctx.fill();
        }

        // Estrela / Faísca de brilho na ponta de avanço da barra
        const float sparkX = barX + 3.0f + fillW;
        const float sparkY = barY + barH * 0.5f;
        ctx.fillStyle = "#ffffff";
        ctx.beginPath();
        ctx.arc(sparkX, sparkY, 4.5f + 1.5f * std::sin(frame_ * 0.3f), 0.0f, 2.0f * kPi);
        ctx.fill();
    }

    // Porcentagem digital à direita da barra
    ctx.save();
    ctx.font = "11px 'Press Start 2P'";
    ctx.textAlign = "right";
    ctx.textBaseline = "middle";
    const int pct = static_cast<int>(progress * 100.0f);
    ctx.fillStyle = "#ffffff";
    ctx.fillText(std::to_string(pct) + "%", barX + barW, barY - 14.0f);
    ctx.restore();

    // -----------------------------------------------------------------------
    // STATUS DINÂMICO DE PRÉ-CARREGAMENTO DAS ARENAS
    // -----------------------------------------------------------------------
    static const char* kArenaStatus[11] = {
        "AQUECENDO O JARDIM BOTANICO DAS ABELHAS...",
        "DRENANDO O PANTANO REAL DO KING FROG...",
        "ANCORANDO NO PORTO DE PESCA DA CRACA...",
        "CONGELANDO O SALAO DE SORVETES DO CREMOSO...",
        "AQUECENDO O LARGO VULCANICO DE PERNAMBUCO...",
        "AFINANDO A SALA DE CONCERTOS DA VIVALDI...",
        "PREPARANDO O PATIO DE AREIA DA ESPINHA...",
        "CALIBRANDO O OBSERVATORIO ESPACIAL DO MARCOV...",
        "ENERGIZANDO A ARENA SUPREMA DE BATALHA...",
        "REFORCANDO O BONECO DE TREINO DO ESPANTALHO...",
        "TODAS AS ARENAS PRONTAS PARA O K.O.! [CLIQUE OU ESPACO]"
    };

    const int statusIdx = (progress >= 0.98f) ? 10 : std::clamp(charId, 0, 9);
    const char* statusText = kArenaStatus[statusIdx];

    ctx.save();
    ctx.font = "900 15px Montserrat";
    ctx.textAlign = "center";
    ctx.textBaseline = "middle";
    ctx.fillStyle = "#000000";
    ctx.fillText(statusText, cx + 2.0f, barY + 44.0f);
    ctx.fillStyle = (progress >= 0.98f) ? "#00f5d4" : "#ffffff";
    ctx.fillText(statusText, cx, barY + 42.0f);
    ctx.restore();

    // -----------------------------------------------------------------------
    // RODAPÉ: DICA DE PULAR
    // -----------------------------------------------------------------------
    const float blink = 0.40f + 0.60f * std::abs(std::sin(frame_ * 0.06f));
    ctx.save();
    ctx.font = "10px 'Press Start 2P'";
    ctx.textAlign = "center";
    ctx.textBaseline = "middle";
    ctx.fillStyle = cv::rgba(255, 255, 255, blink * 0.55f);
    ctx.fillText("CLIQUE OU PRESSIONE QUALQUER TECLA PARA PULAR", cx, 840.0f);
    ctx.restore();
}

void MenuScreens::drawTitle(sf::RenderTarget& /*target*/, Canvas& ctx) {
    // Raios girando atrás da logo, dois lutadores que se revezam nas laterais em painéis
    // diagonais (com a retícula de ícones), botões com ícone e um letreiro correndo embaixo.
    {
        const float rot = frame_ * 0.0025f;
        for (int i = 0; i < 18; ++i) {
            const float a0 = rot + i * 2 * kPi / 18, a1 = a0 + kPi / 18;
            ctx.beginPath(); ctx.moveTo(800, 240);
            ctx.lineTo(800 + std::cos(a0) * 1400, 240 + std::sin(a0) * 1400);
            ctx.lineTo(800 + std::cos(a1) * 1400, 240 + std::sin(a1) * 1400);
            ctx.closePath();
            ctx.fillStyle = i % 2 ? "rgba(255, 0, 127, 0.07)" : "rgba(0, 245, 212, 0.05)";
            ctx.fill();
        }
        auto glow = ctx.createRadialGradient(800, 240, 20, 800, 240, 520);
        glow->addColorStop(0, "rgba(255, 0, 127, 0.22)");
        glow->addColorStop(1, "rgba(255, 0, 127, 0)");
        ctx.fillStyle = glow; ctx.fillRect(280, 0, 1040, 760);
    }

    // PRESS START com efeito VHS tracking glitch durante a vinheta (3.6s a 5.4s) e firme depois:
    const float tSec = static_cast<float>(screenFrame_) / 60.0f;
    if (tSec >= 3.60f) {
        ctx.save();
        ctx.font = "16px 'Press Start 2P'";
        ctx.textAlign = "center";
        ctx.textBaseline = "middle";

        if (tSec < 5.40f) {
            // Fase da vinheta (3.6s a 5.4s): glitch de fita VHS analógica, jitter de tracking e aberração cromática
            const float glitchProgress = (tSec - 3.60f) / (5.40f - 3.60f); // 0.0 -> 1.0 conforme se aproxima de se firmar
            // Flicker estroboscópico de fita com quedas esporádicas de sinal vertical:
            float vhsAlpha = 0.50f + 0.50f * std::sin(screenFrame_ * 18.5f);
            if ((screenFrame_ % 7 == 0) || (screenFrame_ % 13 == 1)) {
                vhsAlpha *= 0.15f; // drop de frame / ruído rápido
            } else if ((screenFrame_ % 23 >= 19)) {
                vhsAlpha = 0.95f; // surto de brilho
            }
            vhsAlpha = vhsAlpha + (1.0f - vhsAlpha) * (glitchProgress * 0.45f);

            // Jitter horizontal de tracking magnético
            float jitterX = 0.0f;
            if ((screenFrame_ % 6 == 0) || (screenFrame_ % 11 == 2)) {
                jitterX = static_cast<float>((screenFrame_ * 7) % 11 - 5) * 1.8f * (1.0f - glitchProgress * 0.6f);
            }
            const float jitterY = ((screenFrame_ % 19 == 0) ? -2.0f : ((screenFrame_ % 29 == 0) ? 2.0f : 0.0f));

            ctx.globalAlpha = std::clamp(vhsAlpha, 0.0f, 1.0f);

            // Aberração cromática / split RGB (canal ciano para a esquerda, magenta para a direita)
            const float chromaSplit = 3.5f * (1.0f - glitchProgress * 0.7f);
            ctx.fillStyle = "rgba(0, 245, 212, 0.70)";
            ctx.fillText("PRESS START", 800.0f + jitterX - chromaSplit, 362.0f + jitterY);

            ctx.fillStyle = "rgba(255, 0, 127, 0.70)";
            ctx.fillText("PRESS START", 800.0f + jitterX + chromaSplit, 362.0f + jitterY);

            // Camada central amarela com contorno preto
            outlined(ctx, "PRESS START", 800.0f + jitterX, 362.0f + jitterY, "#ffe600", 0, 3);

            // Barra de ruído / tracking horizontal passando pelo texto
            if (screenFrame_ % 14 < 6) {
                const float barY = 352.0f + std::fmod((tSec - 3.60f) * 60.0f, 22.0f);
                ctx.fillStyle = "rgba(255, 255, 255, 0.35)";
                ctx.fillRect(700, barY, 200, 2);
            }
        } else {
            // Firme pós-vinheta (>= 5.4s): pulso clássico arcade dourado suave e firme
            const float blink = 0.40f + 0.60f * (0.5f + 0.5f * std::cos((screenFrame_ - 324) / 60.0f / 1.2f * 2.0f * kPi));
            ctx.globalAlpha = blink;
            outlined(ctx, "PRESS START", 800, 362, "#ffe600", 0, 3);
        }
        ctx.restore();
    }
    static const char* kLabels[] = {"INICIAR JOGO", "CONFIGURAÇÕES", "SAIR"};
    static const char* kFill[] = {"#ff007f", "#00f5d4", "#334155"};
    static const char* kText[] = {"#ffffff", "#000000", "#ffffff"};
    static const int kIcon[] = {1, 2, 4};
    for (int i = 0; i < 3; ++i) {
        const float appear = springOut((static_cast<float>(screenFrame_) - 20 - i * 6) / 36.0f);
        if (appear <= 0.01f) continue;
        sf::FloatRect r = titleButton(i);
        const bool sel = titleIndex_ == i;
        ctx.save();
        ctx.globalAlpha = std::min(1.0f, appear);
        ctx.translate(0, (1 - appear) * 500);
        if (sel) {
            // Setas de seleção piscando dos dois lados
            const float bob = std::sin(frame_ * 0.2f) * 5;
            ctx.fillStyle = "#ffe600";
            for (int k = 0; k < 2; ++k) {
                const float ax = k == 0 ? r.position.x - 26 - bob : r.position.x + r.size.x + 26 + bob, d = k == 0 ? 1.0f : -1.0f;
                ctx.beginPath(); ctx.moveTo(ax, r.position.y + r.size.y / 2); ctx.lineTo(ax - 18 * d, r.position.y + r.size.y / 2 - 16); ctx.lineTo(ax - 18 * d, r.position.y + r.size.y / 2 + 16); ctx.closePath();
                ctx.fill(); ctx.strokeStyle = "#000000"; ctx.lineWidth = 3; ctx.stroke();
            }
        }
        notchButton(ctx, r.position.x, r.position.y, r.size.x, r.size.y, kFill[i], kText[i], kLabels[i], "", sel, i == 0 ? "26px 'Titan One'" : "22px 'Titan One'");
        ctx.save();
        ctx.translate(r.position.x + 48 - (sel ? 3 : 0), r.position.y + r.size.y / 2 + 4 - (sel ? 3 : 0));
        ctx.scale(0.72f, 0.72f);
        modeIcon(ctx, kIcon[i], frame_ / 60.0f, kFill[i]);
        ctx.restore();
        ctx.restore();
    }

    // Letreiro correndo + dica de controles
    ctx.font = "600 13px 'Outfit SemiBold'";
    ctx.fillStyle = "#8c82a0"; ctx.textAlign = "center"; ctx.textBaseline = "middle";
    ctx.fillText("MOUSE ou SETAS escolhem  -  CLIQUE ou ENTER confirma  -  M liga/desliga o som", 800, 722);
    ctx.fillStyle = "#000000"; ctx.fillRect(0, 846, 1600, 54);
    ctx.fillStyle = "#ffe600"; ctx.fillRect(0, 846, 1600, 4);
    const std::string ticker = "TORNEIO ARCADE  *  8 LUTADORES + O CHEFE SUPREMO  *  MONTE SUA BUILD  *  SEGURE A GUARDA, ESQUIVE E CONTRA-ATAQUE  *  ";
    ctx.font = "26px Bangers"; ctx.textAlign = "left";
    const float tw = ctx.measureText(ticker);
    const float off = std::fmod(frame_ * 1.6f, tw);
    for (float tx = -off; tx < 1600; tx += tw) {
        ctx.fillStyle = "#ffffff";
        ctx.fillText(ticker, tx, 875);
    }
}

// Ficha do herói (showcase-card): arquétipo, nível, retrato grande, nome, título e a fala.
void MenuScreens::drawHeroCard(sf::RenderTarget& target, Canvas& ctx, int charId, float x, float y, float w, float h) {
    panelCard(ctx, x, y, w, h);
    if (charId < 0) {
        ctx.font = "120px 'Titan One'"; ctx.textAlign = "center"; ctx.textBaseline = "middle";
        outlined(ctx, "?", x + w / 2, y + h * 0.42f, "#ff007f", 4, 6);
        ctx.font = "28px 'Titan One'";
        outlined(ctx, "ALEATÓRIO", x + w / 2, y + h * 0.72f, "#ffffff", 2, 3);
        ctx.font = "800 13px 'Outfit ExtraBold'"; ctx.fillStyle = "#c4b5fd";
        ctx.fillText("A sorte escolhe o seu lutador!", x + w / 2, y + h * 0.8f);
        return;
    }
    const CharacterInfo& info = rosterInfo(static_cast<uint32_t>(charId));
    const std::string col = hexColor(info.color);
    pill(ctx, x + 20, y + 18, upper(info.archetype), "#9d4edd", "#ffffff", "11px 'Titan One'", 9, 22, 2);
    if (charId == 0) pill(ctx, x + 20, y + 18, upper(info.archetype), "#ffe600", "#000000", "11px 'Titan One'", 9, 22, 2);
    pill(ctx, x + w - 20, y + 18, "LVL 10", "#ff007f", "#ffffff", "11px 'Titan One'", 9, 22, 2, true);
    // Brilho de fundo atrás do retrato
    auto glow = ctx.createRadialGradient(x + w / 2, y + h * 0.4f, 10, x + w / 2, y + h * 0.4f, w * 0.55f);
    sf::Color gc = info.color; gc.a = 60;
    glow->addColorStop(0, gc); gc.a = 0; glow->addColorStop(1, gc);
    ctx.fillStyle = glow; ctx.fillRect(x + 6, y + 50, w - 12, h * 0.6f);
    ctx.flush();
    ui::portrait(target, *sprites_, info.key, {{x + 24, y + 60}, {w - 48, h * 0.5f}}, frame_, true);
    ctx.font = "28px 'Titan One'"; ctx.textAlign = "center"; ctx.textBaseline = "middle";
    outlined(ctx, upper(info.name), x + w / 2, y + h * 0.66f, "#ffffff", 2.5f, 3);
    ctx.font = "900 12px Montserrat"; ctx.fillStyle = col;
    ctx.fillText(upper(info.title), x + w / 2, y + h * 0.66f + 26);
    // Balão de fala
    const float bx = x + 22, by = y + h * 0.66f + 46, bw = w - 44, bh = h - (by - y) - 20;
    inkBox(ctx, bx, by, bw, bh, cv::Style("#e2e8f0"), 3, 5, 8);
    ctx.beginPath(); ctx.moveTo(bx + 30, by + 1); ctx.lineTo(bx + 38, by - 9); ctx.lineTo(bx + 46, by + 1); ctx.closePath();
    ctx.fillStyle = "#e2e8f0"; ctx.fill();
    ctx.font = "800 13px 'Outfit ExtraBold'"; ctx.fillStyle = "#111827"; ctx.textAlign = "center";
    wrapText(ctx, std::string("\"") + info.quote + "\"", x + w / 2, by + 20, bw - 24, 17);
}

void MenuScreens::drawStatsPanel(Canvas& ctx, uint32_t charId, float x, float y, float w, float h) {
    panelCard(ctx, x, y, w, h);
    const CharacterInfo& info = rosterInfo(charId);
    float cy = y + 32;
    ctx.font = "16px 'Titan One'"; ctx.textAlign = "left"; ctx.textBaseline = "middle";
    std::string head = upper(info.name);
    {
        std::string t = upper(info.title);
        auto sp = t.find(' ');
        head += " - " + (t.rfind("DE ", 0) == 0 || sp == std::string::npos ? t : t);
    }
    if (ctx.measureText(head) > w - 150) head = upper(info.name);
    outlined(ctx, head, x + 24, cy, "#ffe169", 0, 2);
    pill(ctx, x + w - 24, cy - 12, "NÍVEL 10", "#ffe169", "#000000", "12px 'Titan One'", 12, 24, 4, true);
    cy += 24;
    ctx.fillStyle = "rgba(255,255,255,0.08)"; ctx.fillRect(x + 22, cy, w - 44, 3);
    cy += 28;
    ctx.font = "900 11.5px Montserrat"; ctx.fillStyle = "#e2e8f0"; ctx.textAlign = "left";
    ctx.fillText("EXPERIÊNCIA", x + 24, cy);
    ctx.fillStyle = "#00f5d4"; ctx.textAlign = "right";
    ctx.fillText("9999 / 9999 XP", x + w - 24, cy);
    cy += 14;
    inkBox(ctx, x + 24, cy, w - 48, 7, cv::Style("#0b0712"), 1.5f, 0, 4);
    ctx.fillStyle = "#00f5d4"; ctx.beginPath(); ctx.roundRect(x + 25, cy + 1, w - 50, 5, 3); ctx.fill();
    cy += 30;
    const std::array<std::pair<const char*, int>, 4> rows = {{{"FORÇA", info.forca}, {"STAMINA", info.stamina}, {"ALCANCE", info.alcance}, {"RESISTÊNCIA", info.resistencia}}};
    static const char* kStatCol[] = {"#ff0055", "#00f5d4", "#ffe169", "#c084fc"};
    for (std::size_t i = 0; i < rows.size(); ++i) {
        ctx.font = "900 11.5px Montserrat"; ctx.fillStyle = "#ffffff"; ctx.textAlign = "left";
        ctx.fillText(rows[i].first, x + 24, cy);
        const float bx = x + 150, bw = w - 174;
        inkBox(ctx, bx, cy - 5, bw, 10, cv::Style("#0b0712"), 1.5f, 0, 5);
        const float grow = springOut((static_cast<float>(screenFrame_) - 8 - i * 4) / 30.0f);
        ctx.fillStyle = kStatCol[i];
        ctx.beginPath(); ctx.roundRect(bx + 1, cy - 4, std::max(0.0f, (bw - 2) * rows[i].second / 100.0f * std::min(1.0f, grow)), 8, 4); ctx.fill();
        cy += 27;
    }
    cy += 6;
    ctx.fillStyle = "rgba(255,255,255,0.08)"; ctx.fillRect(x + 22, cy, w - 44, 3);
    cy += 24;
    ctx.font = "13px 'Titan One'"; ctx.textAlign = "left";
    outlined(ctx, "HABILIDADES EQUIPADAS", x + 24, cy, "#ffe169", 0, 2);
    cy += 18;
    const SkillLoadout& l = loadouts_[charId];
    const std::array<SkillId, 3> slots = {l.spec1, l.spec2, l.passive};
    for (std::size_t i = 0; i < slots.size(); ++i) {
        const float rh = 40;
        inkBox(ctx, x + 22, cy, w - 44, rh, cv::Style("#140d22"), 2, 3, 6);
        const SkillDef* def = SkillSystem::get(slots[i]);
        if (!def) {
            ctx.font = "800 13px 'Outfit ExtraBold'"; ctx.fillStyle = "#6b6380"; ctx.textAlign = "left";
            ctx.fillText(i == 2 ? "Passiva vazia" : "Espaço vazio", x + 36, cy + rh / 2);
        } else {
            const bool passive = def->type == SkillType::Passive;
            const std::string tagCol = passive ? "#c084fc" : hexColor(ui::skillTagColor(def->tag));
            pill(ctx, x + 34, cy + 10, passive ? "PASSIVA" : upper(def->tag), tagCol.c_str(), "#000000", "900 10.5px Montserrat", 8, 20, 3);
            ctx.font = "800 14px 'Outfit ExtraBold'"; ctx.fillStyle = "#ffffff"; ctx.textAlign = "left"; ctx.textBaseline = "middle";
            ctx.fillText(def->name, x + 128, cy + rh / 2 + 1);
            if (passive) pill(ctx, x + w - 34, cy + 10, "Passiva", "#ffe169", "#000000", "900 10.5px Montserrat", 8, 20, 4, true);
            else {
                const std::string pL = keyBindings_.actionName(ActionKey::PunchL, true);
                const std::string pR = keyBindings_.actionName(ActionKey::PunchR, true);
                commandText(ctx, *def, x + w - 34, cy + rh / 2, true, "#ffe169", pL, pR);
            }
        }
        cy += rh + 10;
    }
}

// ---------------------------------------------------------------------------
// Seleção de lutador
// ---------------------------------------------------------------------------

void MenuScreens::drawStageHeader(Canvas& ctx, sf::Color col, float headerEnter, const std::string& headerText, const std::string& bannerPrev,
                                  float bannerFlip) {
    const std::string colHex = hexColor(col);
    // ---------------- Cabeçalho ----------------
    {
        const float drop = -90 * (1 - headerEnter);
        ctx.save();
        ctx.translate(0, drop);
        ctx.fillStyle = "#000000"; ctx.fillRect(0, 0, 1600, 92);
        hatch(ctx, 0, 0, 1600, 92, 10, "rgba(255,255,255,0.035)", 3);
        ctx.fillStyle = colHex; ctx.fillRect(0, 88, 1600, 5);
        // Faixa "ESCOLHA SEU LUTADOR"
        ctx.save();
        ctx.translate(800, 46);
        ctx.transform(1, 0, -0.18f, 1, 0, 0);
        inkBox(ctx, -300, -26, 600, 52, cv::Style("#ffe600"), 4, 7, 0);
        hatch(ctx, -296, -22, 592, 44, 9, "rgba(0,0,0,0.07)", 3);
        ctx.restore();
        // O texto da faixa vira como uma placa ao trocar de tela.
        ctx.save();
        ctx.translate(800, 48);
        ctx.scale(1, std::max(0.02f, std::fabs(std::cos(std::min(1.0f, bannerFlip) * kPi))));
        ctx.font = "34px 'Titan One'"; ctx.textAlign = "center"; ctx.textBaseline = "middle";
        ctx.fillStyle = "#000000"; ctx.fillText(bannerFlip < 0.5f && !bannerPrev.empty() ? bannerPrev : headerText, 0, 0);
        ctx.restore();
        ctx.restore();
    }

}

void MenuScreens::drawFighterStage(sf::RenderTarget& target, Canvas& ctx, int focus, sf::Color col, float swap, float since, float enterLeft,
                                   float headerEnter, const std::string& headerText, const std::string& bannerPrev, float bannerFlip) {
    // Palco compartilhado pela seleção de lutador e pela de modo: faixa diagonal na cor do lutador,
    // lutador gigante, placa do nome e o cabeçalho. Assim a troca entre as duas telas é contínua.
    const float t = frame_ / 60.0f;
    const std::string colHex = hexColor(col);
    // ---------------- Faixa diagonal com retícula e raios ----------------
    ctx.save();
    ctx.translate(-260 * (1 - enterLeft), 0);
    {
        const float x0 = -40, x1 = 700, slant = 170;
        // jogo 3D: a arena do lutador girando por baixo da cor, recortada na faixa
        const bool arena3D = stageBg_ && focus >= 0;
        if (arena3D) {
            ctx.flush();
            const float dx = -260 * (1 - enterLeft), sx = stageBg_->getSize().x / 1600.0f, sy = stageBg_->getSize().y / 900.0f;
            sf::ConvexShape band(4); band.setPoint(0, {x0, 96}); band.setPoint(1, {x1 + slant, 96}); band.setPoint(2, {x1, 744}); band.setPoint(3, {x0, 744});
            band.setPosition({dx, 0}); band.setTexture(stageBg_);
            band.setTextureRect(sf::IntRect({static_cast<int>(x0 * sx), static_cast<int>(96 * sy)}, {static_cast<int>((x1 + slant - x0) * sx), static_cast<int>(648 * sy)}));
            target.draw(band);
        }
        ctx.beginPath(); ctx.moveTo(x0, 96); ctx.lineTo(x1 + slant, 96); ctx.lineTo(x1, 744); ctx.lineTo(x0, 744); ctx.closePath();
        auto g = ctx.createLinearGradient(0, 96, 0, 744);
        g->addColorStop(0, sf::Color(col.r / 3, col.g / 3, col.b / 3, arena3D ? 120 : 255));
        g->addColorStop(1, sf::Color(col.r / 7, col.g / 7, col.b / 7, arena3D ? 190 : 255));
        ctx.fillStyle = g; ctx.fill();
        // Raios de impacto girando atrás do lutador
        ctx.save();
        ctx.beginPath(); ctx.moveTo(x0, 96); ctx.lineTo(x1 + slant, 96); ctx.lineTo(x1, 744); ctx.lineTo(x0, 744); ctx.closePath(); ctx.clip();
        const float cx = 400, cy = 430;
        for (int i = 0; i < 18; ++i) {
            const float a0 = i * kPi * 2 / 18 + t * 0.15f, a1 = a0 + kPi / 30;
            // Cada raio termina na borda inclinada da faixa (o clip do canvas é só retangular).
            auto reach = [&](float a) {
                const float dx = std::cos(a), dy = std::sin(a);
                // Borda: x = x1 + slant * (744 - y) / 648  ->  resolve cx + t*dx = x1 + slant*(744 - cy - t*dy)/648
                const float k = slant / 648.0f;
                const float denom = dx + k * dy;
                float tEdge = 900;
                if (denom > 1e-3f) tEdge = std::min(900.0f, (x1 + k * (744 - cy) - cx) / denom);
                return sf::Vector2f{cx + dx * tEdge, cy + dy * tEdge};
            };
            const sf::Vector2f e0 = reach(a0), e1 = reach(a1);
            ctx.beginPath(); ctx.moveTo(cx, cy); ctx.lineTo(e0.x, e0.y); ctx.lineTo(e1.x, e1.y);
            ctx.closePath();
            ctx.fillStyle = cv::rgba(col.r, col.g, col.b, 0.10f);
            ctx.fill();
        }
        ctx.restore();
        // Retícula com o ícone do lutador em foco (crescem para a base)
        ctx.fillStyle = cv::rgba(col.r, col.g, col.b, 0.30f);
        int row = 0;
        for (float y = 118; y < 744; y += 30, ++row) {
            const float r = 2.5f + 8.5f * (y - 118) / 626.0f;
            const float right = x1 + slant * (744 - y) / 648.0f;
            for (float x = (row % 2) * 15.0f + 8; x < right - r - 6; x += 30) {
                if (focus >= 0) charIcon(ctx, focus, x, y, r); else ctx.fillRect(x - r * 0.4f, y - r * 0.4f, r * 0.8f, r * 0.8f);
            }
        }
        // Borda de nanquim na diagonal + filete na cor
        ctx.beginPath(); ctx.moveTo(x1 + slant, 96); ctx.lineTo(x1, 744);
        ctx.strokeStyle = "#000000"; ctx.lineWidth = 10; ctx.stroke();
        ctx.beginPath(); ctx.moveTo(x1 + slant + 14, 96); ctx.lineTo(x1 + 14, 744);
        ctx.strokeStyle = colHex; ctx.lineWidth = 4; ctx.stroke();
    }
    // Nome gigante carimbado ao fundo
    if (focus >= 0) {
        const std::string bigName = upper(lookFor(static_cast<uint32_t>(focus)).name);
        ctx.save();
        ctx.translate(60 + 120 * (1 - swap), 640);
        ctx.rotate(-8 * kPi / 180);
        ctx.font = "150px 'Titan One'"; ctx.textAlign = "left"; ctx.textBaseline = "alphabetic";
        ctx.globalAlpha = 0.16f * std::min(1.0f, swap * 1.5f);
        ctx.fillStyle = "#ffffff";
        ctx.fillText(bigName, 0, 0);
        ctx.restore();
    }
    ctx.restore();

    // ---------------- Lutador gigante ----------------
    {
        const float slideX = -180 * (1 - swap) - 260 * (1 - enterLeft);
        const sf::FloatRect box{{-10 + slideX, 96}, {790, 652}};
        ctx.flush();
        if (focus >= 0 && stageFighter_) {
            // jogo 3D: o modelo do lutador (quadro inteiro com alfa) e a mesma sombra dura deslocada
            const sf::Vector2u ts = stageFighter_->getSize();
            sf::Sprite fig(*stageFighter_, sf::IntRect({0, static_cast<int>(ts.y)}, {static_cast<int>(ts.x), -static_cast<int>(ts.y)}));
            fig.setScale({1600.0f / ts.x, 900.0f / ts.y});
            fig.setPosition({slideX + 16, 14}); fig.setColor(sf::Color(0, 0, 0, 200)); target.draw(fig);
            fig.setPosition({slideX, 0}); fig.setColor(sf::Color::White); target.draw(fig);
        } else if (focus >= 0) {
            const CharacterInfo& info = rosterInfo(static_cast<uint32_t>(focus));
            const uint32_t currentSkin = skin(static_cast<uint32_t>(focus));
            // Sombra dura: a silhueta preta deslocada
            ui::portrait(target, *sprites_, info.key, {{box.position.x + 16, box.position.y + 14}, box.size}, frame_, true, sf::Color(0, 0, 0, 200), currentSkin);
            ui::portrait(target, *sprites_, info.key, box, frame_, true, sf::Color::White, currentSkin);
        } else {
            ctx.save();
            ctx.translate(380 + slideX, 430);
            ctx.rotate(std::sin(t * 2) * 0.08f);
            ctx.font = "320px 'Titan One'"; ctx.textAlign = "center"; ctx.textBaseline = "middle";
            outlined(ctx, "?", 0, 0, "#ff007f", 8, 14);
            ctx.restore();
        }
    }

    // ---------------- Placa do nome (sobre o lutador) ----------------
    {
        const float slideY = 80 * (1 - springOut(since / 26.0f));
        ctx.save();
        ctx.globalAlpha = std::min(1.0f, headerEnter * 1.4f);
        ctx.translate(70, 612 + slideY);
        ctx.rotate(-3 * kPi / 180);
        const std::string name = focus >= 0 ? upper(rosterInfo(static_cast<uint32_t>(focus)).name) : "ALEATÓRIO";
        const std::string title = focus >= 0 ? upper(rosterInfo(static_cast<uint32_t>(focus)).title) : "QUALQUER UM PODE SAIR!";
        ctx.font = "62px 'Titan One'"; ctx.textAlign = "left"; ctx.textBaseline = "alphabetic";
        static const char* kExt[] = {"#000000", "#1a1a1a", "#333333"};
        for (int l = 0; l < 3; ++l) { ctx.fillStyle = kExt[l]; ctx.fillText(name, 9 - l * 3.0f, 9 - l * 3.0f); }
        ctx.strokeStyle = "#000000"; ctx.lineWidth = 10; ctx.lineJoin = "round"; ctx.strokeText(name, 0, 0);
        ctx.fillStyle = "#ffffff"; ctx.fillText(name, 0, 0);
        ctx.font = "900 14px Montserrat";
        const float tw = ctx.measureText(title) + 24;
        inkBox(ctx, 4, 14, tw, 26, cv::Style(colHex), 2.5f, 4, 2);
        ctx.fillStyle = (col.r * 0.3f + col.g * 0.59f + col.b * 0.11f) > 150 ? "#000000" : "#ffffff";
        ctx.textBaseline = "middle"; ctx.fillText(title, 16, 28);

        // Seletor de Skins interativo
        if (focus >= 0) {
            const uint32_t curSkin = skin(static_cast<uint32_t>(focus));
            const SkinDef& sk = rosterSkin(static_cast<uint32_t>(focus), curSkin);
            const std::string skColorHex = hexColor(sk.color);
            const float pillW = 390.0f, pillH = 34.0f;
            const float pillY = 46.0f;

            // Caixa principal do seletor
            inkBox(ctx, 4, pillY, pillW, pillH, cv::Style("#120d20"), 2.5f, 5, 4, skColorHex.c_str());

            // Botão esquerdo [Q] ◄
            ctx.fillStyle = "#1e1735"; ctx.fillRect(6, pillY + 2, 46, pillH - 4);
            ctx.font = "900 11px Montserrat"; ctx.textAlign = "center"; ctx.textBaseline = "middle";
            ctx.fillStyle = "#00f5d4"; ctx.fillText("[Q] ◄", 29, pillY + pillH * 0.5f);

            // Texto central da Skin: SKIN: NOME (X/4)
            ctx.font = "14px 'Titan One'"; ctx.textAlign = "center"; ctx.fillStyle = "#ffffff";
            const std::string skinLabel = "SKIN: " + std::string(sk.name) + " (" + std::to_string(curSkin + 1) + "/4)";
            ctx.fillText(skinLabel, 4 + pillW * 0.5f, pillY + pillH * 0.5f);

            // Botão direito ► [E]
            ctx.fillStyle = "#1e1735"; ctx.fillRect(4 + pillW - 50, pillY + 2, 46, pillH - 4);
            ctx.font = "900 11px Montserrat"; ctx.textAlign = "center"; ctx.textBaseline = "middle";
            ctx.fillStyle = "#00f5d4"; ctx.fillText("► [E]", 4 + pillW - 27, pillY + pillH * 0.5f);

            // Tagline da skin abaixo do seletor
            ctx.font = "italic 900 11px Montserrat"; ctx.textAlign = "left"; ctx.fillStyle = "#cbd5e1";
            ctx.fillText(sk.tagline, 8, pillY + pillH + 11.0f);
        }
        ctx.restore();
    }

    drawStageHeader(ctx, col, headerEnter, headerText, bannerPrev, bannerFlip);
}

namespace {

void drawCarouselCardFighter(sf::RenderTarget& target, const SpriteLibrary& sprites, const std::string& key,
                             float cx, float cy, float cardW, float cardH, float pop,
                             uint32_t frame, bool selected, sf::Color tint) {
    // Joana's approved remake replaces the legacy carousel body and gloves.
    // Reuse the already-resident HD portrait: no combat rig or extra texture.
    if (key == "joana") {
        const auto hd = sprites.find("portrait_hd_joana_skin0");
        if (hd) {
            const float bob = selected ? std::sin(float(frame) * .08f) * 2.f : 0.f;
            const float breathe = selected ? 1.f + std::sin(float(frame) * .08f) * .015f : 1.f;
            const float ch = cardH * pop;
            const float scale = std::min((cardW * pop - 6.f) / hd->rect.size.x,
                                         (ch - 6.f) / hd->rect.size.y) * breathe;
            sf::Sprite art(*hd->texture, hd->rect);
            art.setScale({scale, scale});art.setColor(tint);
            art.setPosition({cx - hd->rect.size.x * scale * .5f,
                             cy - ch * .5f + 2.5f * pop + bob});
            target.draw(art);
            return;
        }
    }
    auto body = sprites.find("portrait_" + key + "_front");
    if (!body) return;
    (void)cardW;

    // Crop e enquadramento de busto heroico para cada lutador no card do carrossel
    struct CropInfo { const char* key; float topRatio; float cropHRatio; float gloveScale; float gloveYOffset; float gloveSpread; };
    static const CropInfo kCrops[] = {
        {"jack",    0.00f, 0.62f, 0.44f, 0.42f, 0.03f},
        {"frog",    0.00f, 0.60f, 0.44f, 0.42f, 0.04f},
        {"joana",   0.00f, 0.62f, 0.45f, 0.40f, 0.02f},
        {"cremoso", 0.00f, 0.64f, 0.44f, 0.44f, 0.03f},
        {"valeria", 0.00f, 0.62f, 0.44f, 0.42f, 0.03f},
        {"vivaldi", 0.05f, 0.60f, 0.44f, 0.42f, 0.03f},
        {"espinha", 0.00f, 0.62f, 0.44f, 0.42f, 0.02f},
        {"marcov",  0.06f, 0.60f, 0.44f, 0.43f, 0.03f},
        {"boss",    0.05f, 0.62f, 0.46f, 0.44f, 0.03f}
    };
    CropInfo cp{"", 0.00f, 0.62f, 0.44f, 0.42f, 0.03f};
    for (const auto& c : kCrops) {
        if (key == c.key) { cp = c; break; }
    }

    const float f = static_cast<float>(frame);
    const float bob = selected ? std::sin(f * 0.08f) * 2.0f : 0.0f;
    const float breathe = selected ? 1.0f + std::sin(f * 0.08f) * 0.015f : 1.0f;

    const float imgW = static_cast<float>(body->rect.size.x);
    const float imgH = static_cast<float>(body->rect.size.y);
    const float topPx = imgH * cp.topRatio;
    const float cropHPx = imgH * cp.cropHRatio;

    // Queremos que o lutador ocupe quase toda a altura da moldura (94% de cardH)
    const float ch = cardH * pop;
    const float targetH = (ch - 6.0f) * breathe;
    const float scale = targetH / cropHPx;
    const float targetW = imgW * scale;

    const float dx = cx - targetW / 2.0f;
    const float dy = cy - ch / 2.0f + 2.5f * pop + bob;

    // Torso / busto
    sf::Sprite torso(*body->texture, sf::IntRect({body->rect.position.x, static_cast<int>(body->rect.position.y + topPx)},
                                                {body->rect.size.x, static_cast<int>(cropHPx)}));
    torso.setScale({scale, scale});
    torso.setPosition({dx, dy});
    torso.setColor(tint);
    target.draw(torso);

    // Luvas em guarda de combate na frente do peito
    const float gSize = targetW * cp.gloveScale;
    const float gloveY = dy + targetH * (cp.gloveYOffset + 0.04f);
    const float spread = targetW * cp.gloveSpread;

    const std::array<std::pair<const char*, sf::Vector2f>, 2> gloves = {{
        {"_gloveL", {dx + targetW * 0.10f - spread + (selected ? std::cos(f * 0.06f) * 1.5f : 0.0f),
                     gloveY + (selected ? std::sin(f * 0.09f) * 2.5f : 0.0f)}},
        {"_gloveR", {dx + targetW * 0.90f - gSize + spread - (selected ? std::cos(f * 0.06f) * 1.5f : 0.0f),
                     gloveY + (selected ? std::cos(f * 0.09f) * 2.5f : 0.0f)}},
    }};

    for (const auto& [suffix, at] : gloves) {
        auto glove = sprites.find("portrait_" + key + suffix);
        if (!glove) continue;
        sf::Sprite g(*glove->texture, glove->rect);
        g.setScale({gSize / glove->rect.size.x, gSize / glove->rect.size.y});
        g.setPosition(at);
        g.setColor(tint);
        target.draw(g);
    }
}

} // namespace

void MenuScreens::drawCharSelect(sf::RenderTarget& target, Canvas& ctx) {
    // Seleção no estilo "fighter select": lutador gigante à esquerda sobre faixa diagonal na cor dele,
    // ficha em card de gibi à direita e o elenco numa faixa de cards no rodapé.
    // Vindo da seleção de modo (ou da build), o lutador e a faixa já estão no lugar.
    const bool fromHub = prevScreen_ == Screen::ModeSelect || prevScreen_ == Screen::Build || prevScreen_ == Screen::CampaignPrompt;
    const float enter = springOut(screenFrame_ / 34.0f);
    const int focus = slotChar(slot_);
    const float since = static_cast<float>(frame_ - slotChangeFrame_);
    const float swap = springOut(since / 22.0f);           // entrada do novo lutador
    const float t = frame_ / 60.0f;
    const sf::Color accent = focus >= 0 ? rosterInfo(static_cast<uint32_t>(focus)).color : sf::Color(255, 0, 127);
    const sf::Color prevAccent = slotChar(prevSlot_) >= 0 ? rosterInfo(static_cast<uint32_t>(slotChar(prevSlot_))).color : sf::Color(255, 0, 127);
    const float blendC = std::clamp(since / 14.0f, 0.0f, 1.0f);
    auto mix = [&](std::uint8_t a, std::uint8_t b) { return static_cast<std::uint8_t>(a + (b - a) * blendC); };
    const sf::Color col(mix(prevAccent.r, accent.r), mix(prevAccent.g, accent.g), mix(prevAccent.b, accent.b));
    const std::string colHex = hexColor(col);

    drawFighterStage(target, ctx, focus, col, swap, since, fromHub ? 1.0f : enter, fromHub ? 1.0f : enter, "ESCOLHA SEU LUTADOR",
                     fromHub ? "ESCOLHA O MODO" : "", fromHub ? screenFrame_ / 14.0f : 1.0f);

    // ---------------- Ficha (direita) ----------------
    {
        const float slide = 320 * (1 - enter);
        const float X = 1000 + slide, Y = 128, W = 540, H = 420;
        ctx.save();
        ctx.translate(X + W / 2, Y + H / 2); ctx.rotate(0.8f * kPi / 180); ctx.translate(-(X + W / 2), -(Y + H / 2));
        inkBox(ctx, X, Y, W, H, cv::Style("#120d20"), 4.5f, 10, 14);
        hatch(ctx, X + 5, Y + 5, W - 10, H - 10, 8, "rgba(255,255,255,0.025)", 2);
        // Aba superior na cor do lutador
        ctx.save();
        ctx.beginPath(); ctx.roundRect(X, Y, W, 58, 14); ctx.clip();
        ctx.fillStyle = colHex; ctx.fillRect(X, Y, W, 58);
        hatch(ctx, X, Y, W, 58, 9, "rgba(0,0,0,0.12)", 3);
        ctx.restore();
        ctx.fillStyle = "#000000"; ctx.fillRect(X, Y + 56, W, 4);
        const bool lightAccent = (col.r * 0.3f + col.g * 0.59f + col.b * 0.11f) > 150;
        ctx.font = "22px 'Titan One'"; ctx.textAlign = "left"; ctx.textBaseline = "middle";
        ctx.fillStyle = lightAccent ? "#000000" : "#ffffff";
        ctx.fillText(focus >= 0 ? upper(rosterInfo(static_cast<uint32_t>(focus)).archetype) : "SORTE OU AZAR?", X + 22, Y + 30);
        pill(ctx, X + W - 20, Y + 16, "LVL 10", "#ffe600", "#000000", "13px 'Titan One'", 10, 26, 3, true);

        float cy = Y + 92;
        if (focus >= 0) {
            const CharacterInfo& info = rosterInfo(static_cast<uint32_t>(focus));
            // Atributos em barras segmentadas (10 blocos), que enchem a cada troca
            const std::array<std::pair<const char*, int>, 4> rows = {{{"FORÇA", info.forca}, {"STAMINA", info.stamina}, {"ALCANCE", info.alcance}, {"RESISTÊNCIA", info.resistencia}}};
            static const char* kStatCol[] = {"#ff0055", "#00f5d4", "#ffe169", "#c084fc"};
            for (std::size_t i = 0; i < rows.size(); ++i) {
                ctx.font = "900 13px Montserrat"; ctx.fillStyle = "#ffffff"; ctx.textAlign = "left";
                ctx.fillText(rows[i].first, X + 26, cy + 10);
                const float fill = std::clamp((since - i * 3.0f) / 18.0f, 0.0f, 1.0f) * rows[i].second / 10.0f;
                for (int b = 0; b < 10; ++b) {
                    const float bx = X + 170 + b * 32, on = std::clamp(fill - b, 0.0f, 1.0f);
                    ctx.save();
                    ctx.translate(bx, cy);
                    ctx.transform(1, 0, -0.25f, 1, 0, 0);
                    ctx.fillStyle = "#000000"; ctx.fillRect(2, 2, 27, 20);
                    ctx.fillStyle = "#231a36"; ctx.fillRect(0, 0, 27, 20);
                    if (on > 0) { ctx.fillStyle = kStatCol[i]; ctx.fillRect(0, 0, 27 * on, 20); ctx.fillStyle = "rgba(255,255,255,0.35)"; ctx.fillRect(0, 0, 27 * on, 5); }
                    ctx.strokeStyle = "#000000"; ctx.lineWidth = 2; ctx.strokeRect(0, 0, 27, 20);
                    ctx.restore();
                }
                ctx.font = "16px 'Titan One'"; ctx.textAlign = "right"; ctx.fillStyle = kStatCol[i];
                ctx.fillText(std::to_string(rows[i].second), X + W - 22, cy + 11);
                cy += 34;
            }
            cy += 10;
            ctx.font = "15px 'Titan One'"; ctx.textAlign = "left";
            outlined(ctx, "HABILIDADES EQUIPADAS", X + 26, cy, "#ffe169", 0, 2);
            cy += 16;
            const SkillLoadout& l = loadouts_[static_cast<std::size_t>(focus)];
            const std::array<SkillId, 3> slots = {l.spec1, l.spec2, l.passive};
            for (std::size_t i = 0; i < slots.size(); ++i) {
                const SkillDef* def = SkillSystem::get(slots[i]);
                inkBox(ctx, X + 22, cy, W - 44, 40, cv::Style("#1b1430"), 2, 3, 6);
                if (def) {
                    const bool passive = def->type == SkillType::Passive;
                    const std::string tc = passive ? "#c084fc" : hexColor(ui::skillTagColor(def->tag));
                    pill(ctx, X + 34, cy + 10, passive ? "PASSIVA" : upper(def->tag), tc.c_str(), "#000000", "900 10.5px Montserrat", 8, 20, 3);
                    ctx.font = "800 15px 'Outfit ExtraBold'"; ctx.fillStyle = "#ffffff"; ctx.textAlign = "left"; ctx.textBaseline = "middle";
                    ctx.fillText(def->name, X + 130, cy + 21);
                    if (!passive) {
                        const std::string pL = keyBindings_.actionName(ActionKey::PunchL, true);
                        const std::string pR = keyBindings_.actionName(ActionKey::PunchR, true);
                        commandText(ctx, *def, X + W - 34, cy + 20, true, "#ffe169", pL, pR);
                    }
                }
                cy += 48;
            }
        } else {
            ctx.font = "800 17px 'Outfit ExtraBold'"; ctx.fillStyle = "#e2e8f0"; ctx.textAlign = "left"; ctx.textBaseline = "middle";
            wrapText(ctx, "A sorte escolhe entre todos os lutadores prontos para o ringue. Confie no destino e suba para a luta!", X + 26, cy + 10, W - 52, 26);
        }
        ctx.restore();
    }

    // ---------------- Botões ----------------
    {
        const float slide = 320 * (1 - enter);
        static const char* kLabels[] = {"VOLTAR", "CONFIRMAR!", "MONTAR BUILD"};
        static const char* kFill[] = {"#334155", "#00f5d4", "#ff007f"};
        static const char* kText[] = {"#ffffff", "#000000", "#ffffff"};
        for (int i = 0; i < 3; ++i) {
            sf::FloatRect r = charButton(i);
            ctx.save();
            ctx.translate(i == 0 ? 0.0f : slide, i == 0 ? -90 * (1 - enter) : 0.0f);
            ctx.translate(r.position.x + r.size.x / 2, r.position.y + r.size.y / 2);
            ctx.transform(1, 0, -0.14f, 1, 0, 0);
            const bool sel = charButtonIndex_ == i;
            const float pulse = (i == 1 && charButtonIndex_ < 0) ? 1.0f + 0.03f * std::sin(t * 6) : 1.0f;
            ctx.scale(pulse, pulse);
            notchButton(ctx, -r.size.x / 2, -r.size.y / 2, r.size.x, r.size.y, kFill[i], kText[i], kLabels[i], "", sel,
                        i == 1 ? "24px 'Titan One'" : "15px 'Titan One'");
            ctx.restore();
        }
    }

    // ---------------- Elenco (faixa no rodapé) ----------------
    {
        const float rise = 200 * (1 - enter);
        ctx.fillStyle = "#000000"; ctx.fillRect(0, 744 + rise, 1600, 156);
        hatch(ctx, 0, 744 + rise, 1600, 156, 10, "rgba(255,255,255,0.03)", 3);
        ctx.fillStyle = colHex; ctx.fillRect(0, 744 + rise, 1600, 4);
        for (int s = 0; s < kSlots; ++s) {
            sf::FloatRect r = cardRect(s);
            const bool selected = s == slot_;
            const int c = slotChar(s);
            const float pop = selected ? 1.0f + 0.12f * springOut(since / 14.0f) : 1.0f;
            const float lift = selected ? -14.0f * springOut(since / 14.0f) : 0.0f;
            const float w = r.size.x, h = r.size.y;
            const float cxp = r.position.x + w / 2, cyp = r.position.y + h / 2 + lift + rise;
            const sf::Color cc = c >= 0 ? rosterInfo(static_cast<uint32_t>(c)).color : sf::Color(255, 0, 127);

            // 1. Moldura e Fundo do Card com Insígnia Heráldica Grandiosa
            ctx.save();
            ctx.translate(cxp, cyp);
            ctx.scale(pop, pop);
            ctx.transform(1, 0, -0.16f, 1, 0, 0);

            if (selected) {
                // Card selecionado: fundo vibrante, contorno duplo e hachura manga
                inkBox(ctx, -w / 2, -h / 2, w, h, cv::Style(hexColor(cc)), 3.5f, 9, 6, "#ffffff");
                hatch(ctx, -w / 2 + 3, -h / 2 + 3, w - 6, h - 6, 8, "rgba(255,255,255,0.20)", 2.5f);
                if (c >= 0) {
                    ctx.fillStyle = "rgba(0, 0, 0, 0.26)";
                    drawCharInsignia(ctx, c, 2.0f, -12.0f, 42.0f);
                    ctx.fillStyle = "rgba(255, 255, 255, 0.38)";
                    drawCharInsignia(ctx, c, 0.0f, -14.0f, 42.0f);
                }
                // Cantoneiras arcade de mira (corner brackets) nos 4 cantos
                ctx.strokeStyle = "#ffffff"; ctx.lineWidth = 2.5f;
                const float cw = 9.0f, ch_c = 9.0f;
                ctx.beginPath(); ctx.moveTo(-w/2 + 2, -h/2 + 2 + ch_c); ctx.lineTo(-w/2 + 2, -h/2 + 2); ctx.lineTo(-w/2 + 2 + cw, -h/2 + 2); ctx.stroke();
                ctx.beginPath(); ctx.moveTo(w/2 - 2 - cw, -h/2 + 2); ctx.lineTo(w/2 - 2, -h/2 + 2); ctx.lineTo(w/2 - 2, -h/2 + 2 + ch_c); ctx.stroke();
                ctx.beginPath(); ctx.moveTo(-w/2 + 2, h/2 - 2 - ch_c); ctx.lineTo(-w/2 + 2, h/2 - 2); ctx.lineTo(-w/2 + 2 + cw, h/2 - 2); ctx.stroke();
                ctx.beginPath(); ctx.moveTo(w/2 - 2 - cw, h/2 - 2); ctx.lineTo(w/2 - 2, h/2 - 2); ctx.lineTo(w/2 - 2, h/2 - 2 - ch_c); ctx.stroke();
                // Friso luminoso na base interna do card
                ctx.fillStyle = "#ffffff";
                ctx.fillRect(-w / 2 + 4, h / 2 - 4, w - 8, 3.0f);
            } else {
                // Card não selecionado: fundo grafite premium com brilho temático e insígnia nítida
                inkBox(ctx, -w / 2, -h / 2, w, h, cv::Style("#141022"), 2.5f, 5, 6, "#000000");
                ctx.fillStyle = cv::rgba(cc.r, cc.g, cc.b, 0.22f);
                ctx.fillRect(-w / 2 + 2, -h / 2 + 2, w - 4, h - 4);
                hatch(ctx, -w / 2 + 3, -h / 2 + 3, w - 6, h - 6, 8, "rgba(255,255,255,0.04)", 2);
                if (c >= 0) {
                    ctx.fillStyle = "rgba(0, 0, 0, 0.40)";
                    drawCharInsignia(ctx, c, 2.0f, -12.0f, 40.0f);
                    ctx.fillStyle = cv::rgba(cc.r, cc.g, cc.b, 0.48f);
                    drawCharInsignia(ctx, c, 0.0f, -14.0f, 40.0f);
                }
                // Friso temático na base interna do card
                ctx.fillStyle = hexColor(cc);
                ctx.fillRect(-w / 2 + 4, h / 2 - 4, w - 8, 2.5f);
            }

            if (c < 0) {
                const float pulse = 1.0f + (selected ? 0.08f * std::sin(t * 7.0f) : 0.04f * std::sin(t * 4.0f));
                ctx.save();
                ctx.scale(pulse, pulse);
                ctx.font = "62px 'Titan One'"; ctx.textAlign = "center"; ctx.textBaseline = "middle";
                outlined(ctx, "?", 0, -4, selected ? "#ffffff" : "#ff007f", 4, 6);
                ctx.restore();
            }
            ctx.restore();

            // 2. Personagem ampliado em busto heroico preenchendo a moldura de ponta a ponta
            if (c >= 0) {
                ctx.flush();
                const CharacterInfo& info = kRoster[static_cast<std::size_t>(c)];
                drawCarouselCardFighter(target, *sprites_, info.key, cxp, cyp, w, h, pop, frame_, selected,
                                        selected ? sf::Color::White : (info.hasFightSprites ? sf::Color(235, 230, 245) : sf::Color(140, 135, 155)));
            }

            // 3. Nome do lutador logo abaixo do card com badge e tipografia nítida
            const std::string charName = c >= 0 ? upper(lookFor(static_cast<uint32_t>(c)).name) : "ALEATÓRIO";
            const float nameY = r.position.y + h + 15 + rise;
            if (selected) {
                ctx.save();
                ctx.translate(cxp, nameY);
                ctx.transform(1, 0, -0.14f, 1, 0, 0);
                const float nw = std::max(90.0f, static_cast<float>(charName.size() * 11 + 20));
                inkBox(ctx, -nw / 2, -11, nw, 22, cv::Style(hexColor(cc)), 2.0f, 4, 3, "#ffffff");
                ctx.font = "900 12px Montserrat"; ctx.textAlign = "center"; ctx.textBaseline = "middle";
                ctx.fillStyle = (cc.r * 0.3f + cc.g * 0.59f + cc.b * 0.11f) > 150 ? "#000000" : "#ffffff";
                ctx.fillText(charName, 0, 0);
                ctx.restore();
            } else {
                ctx.font = "900 11.5px Montserrat"; ctx.textAlign = "center"; ctx.textBaseline = "middle";
                // Sombra suave do texto
                ctx.fillStyle = "#000000";
                ctx.fillText(charName, cxp + 1.5f, nameY + 1.5f);
                ctx.fillStyle = "#c5bcd9";
                ctx.fillText(charName, cxp, nameY);
            }

            // Cursor P1 quicando
            if (selected && charButtonIndex_ < 0) {
                const float by = r.position.y - 30 + lift + rise - std::fabs(std::sin(t * 5)) * 8;
                ctx.save();
                ctx.translate(cxp, by);
                ctx.beginPath(); ctx.moveTo(-20, -16); ctx.lineTo(20, -16); ctx.lineTo(20, 4); ctx.lineTo(6, 4); ctx.lineTo(0, 13); ctx.lineTo(-6, 4); ctx.lineTo(-20, 4); ctx.closePath();
                ctx.fillStyle = "#000000"; ctx.save(); ctx.translate(3, 3); ctx.fill(); ctx.restore();
                ctx.fillStyle = "#ff007f"; ctx.fill(); ctx.strokeStyle = "#000000"; ctx.lineWidth = 2.5f; ctx.stroke();
                ctx.font = "14px 'Titan One'"; ctx.fillStyle = "#ffffff"; ctx.textAlign = "center"; ctx.textBaseline = "middle";
                ctx.fillText("P1", 0, -6);
                ctx.restore();
            }
        }
    }

    drawNotice(ctx, 640);
    ctx.font = "600 13px 'Outfit SemiBold'"; ctx.fillStyle = "#6b6380"; ctx.textAlign = "right"; ctx.textBaseline = "middle";
    ctx.fillText("SETAS escolhem  ·  CIMA vai aos botões  ·  ENTER confirma  ·  ESC volta", 1580, 46);
}

// ---------------------------------------------------------------------------
// Seleção de modo
// ---------------------------------------------------------------------------

namespace {

// Ícones dos modos desenhados à mão (centro em 0,0, cabem num quadrado de ~64 px).
void modeIcon(Canvas& ctx, int mode, float time, const char* ink) {
    ctx.strokeStyle = "#000000";
    ctx.lineWidth = 4;
    ctx.lineJoin = "round";
    switch (mode) {
    case 0: { // troféu
        ctx.beginPath(); ctx.moveTo(-20, -26); ctx.lineTo(20, -26); ctx.lineTo(16, 2); ctx.quadraticCurveTo(0, 14, -16, 2); ctx.closePath();
        ctx.fillStyle = "#ffe600"; ctx.fill(); ctx.stroke();
        ctx.beginPath(); ctx.arc(-22, -14, 9, kPi * 0.5f, kPi * 1.5f); ctx.stroke();
        ctx.beginPath(); ctx.arc(22, -14, 9, -kPi * 0.5f, kPi * 0.5f); ctx.stroke();
        ctx.fillStyle = "#ffe600";
        ctx.beginPath(); ctx.rect(-5, 8, 10, 12); ctx.fill(); ctx.stroke();
        ctx.beginPath(); ctx.rect(-16, 20, 32, 8); ctx.fill(); ctx.stroke();
        ctx.fillStyle = "#ffffff";
        ctx.beginPath(); ctx.ellipse(-8, -16, 3, 7, 0.3f, 0, kPi * 2); ctx.fill();
        break;
    }
    case 1: { // luva de boxe
        ctx.save(); ctx.rotate(-0.35f + std::sin(time * 8) * 0.06f);
        ctx.beginPath(); ctx.ellipse(0, -6, 22, 20, 0, 0, kPi * 2); ctx.fillStyle = "#ff1e42"; ctx.fill(); ctx.stroke();
        ctx.beginPath(); ctx.ellipse(-16, 2, 9, 11, 0.4f, 0, kPi * 2); ctx.fill(); ctx.stroke();
        ctx.beginPath(); ctx.rect(-12, 12, 26, 14); ctx.fillStyle = "#ffffff"; ctx.fill(); ctx.stroke();
        ctx.fillStyle = "rgba(255,255,255,0.7)"; ctx.beginPath(); ctx.ellipse(6, -14, 6, 4, -0.4f, 0, kPi * 2); ctx.fill();
        ctx.restore();
        break;
    }
    case 2: { // Cabeça do Espantalho / Boneco de Treino
        // Chapéu de palha
        ctx.beginPath(); ctx.moveTo(-28, -6); ctx.lineTo(28, -6); ctx.lineTo(16, -26); ctx.lineTo(-16, -26); ctx.closePath();
        ctx.fillStyle = "#eab308"; ctx.fill(); ctx.stroke();
        // Faixa vermelha no chapéu
        ctx.beginPath(); ctx.rect(-18, -13, 36, 5); ctx.fillStyle = "#ef4444"; ctx.fill();
        // Cabeça de saco de estopa
        ctx.beginPath(); ctx.roundRect(-20, -5, 40, 36, 8); ctx.fillStyle = "#d97706"; ctx.fill(); ctx.stroke();
        // Olhos de botão (X e círculo)
        ctx.strokeStyle = "#000000"; ctx.lineWidth = 3;
        ctx.beginPath(); ctx.moveTo(-13, 4); ctx.lineTo(-7, 10); ctx.moveTo(-7, 4); ctx.lineTo(-13, 10); ctx.stroke();
        ctx.beginPath(); ctx.arc(10, 7, 4, 0, kPi * 2); ctx.fillStyle = "#1e293b"; ctx.fill(); ctx.stroke();
        // Boca costurada em zigue-zague
        ctx.beginPath(); ctx.moveTo(-12, 19); ctx.lineTo(-6, 22); ctx.lineTo(0, 19); ctx.lineTo(6, 22); ctx.lineTo(12, 19); ctx.stroke();
        break;
    }
    case 3: { // engrenagem girando
        ctx.save(); ctx.rotate(time * 1.2f);
        ctx.beginPath();
        for (int i = 0; i < 16; ++i) {
            const float a = i * kPi / 8, r = i % 2 == 0 ? 28.0f : 21.0f;
            const float a0 = a - kPi / 16, a1 = a + kPi / 16;
            if (i == 0) ctx.moveTo(std::cos(a0) * r, std::sin(a0) * r); else ctx.lineTo(std::cos(a0) * r, std::sin(a0) * r);
            ctx.lineTo(std::cos(a1) * r, std::sin(a1) * r);
        }
        ctx.closePath(); ctx.fillStyle = "#cbd5e1"; ctx.fill(); ctx.stroke();
        ctx.beginPath(); ctx.arc(0, 0, 9, 0, kPi * 2); ctx.fillStyle = ink; ctx.fill(); ctx.stroke();
        ctx.restore();
        break;
    }
    case 4: { // cabeça do Batalhator
        ctx.beginPath(); ctx.roundRect(-24, -22, 48, 42, 6); ctx.fillStyle = "#64748b"; ctx.fill(); ctx.stroke();
        ctx.beginPath(); ctx.moveTo(0, -22); ctx.lineTo(0, -34); ctx.stroke();
        ctx.beginPath(); ctx.arc(0, -36, 4, 0, kPi * 2); ctx.fillStyle = std::sin(time * 10) > 0 ? "#ff1e42" : "#7f1d1d"; ctx.fill(); ctx.stroke();
        const float blink = std::fmod(time, 3.0f) < 0.12f ? 1.5f : 7.0f;
        ctx.fillStyle = "#ff1e42";
        ctx.beginPath(); ctx.rect(-16, -10, 12, blink); ctx.fill();
        ctx.beginPath(); ctx.rect(4, -10, 12, blink); ctx.fill();
        ctx.beginPath(); ctx.rect(-14, 6, 28, 6); ctx.fillStyle = "#0f172a"; ctx.fill();
        for (int i = 0; i < 4; ++i) { ctx.fillStyle = "#e2e8f0"; ctx.fillRect(-12 + i * 7, 7, 4, 4); }
        break;
    }
    default: { // seta de voltar
        ctx.beginPath(); ctx.moveTo(-24, 0); ctx.lineTo(-4, -18); ctx.lineTo(-4, -8); ctx.lineTo(22, -8); ctx.lineTo(22, 8); ctx.lineTo(-4, 8); ctx.lineTo(-4, 18); ctx.closePath();
        ctx.fillStyle = "#e2e8f0"; ctx.fill(); ctx.stroke();
        break;
    }
    }
}

} // namespace

void MenuScreens::drawModeSelect(sf::RenderTarget& target, Canvas& ctx) {
    // O lutador escolhido continua no palco (mesma faixa da seleção); os modos entram pela direita
    // e uma barra de informações sobe no lugar do elenco.
    const bool fromChar = prevScreen_ == Screen::CharSelect;
    const float enter = springOut(screenFrame_ / 30.0f);
    const float t = frame_ / 60.0f;
    const CharacterInfo& info = rosterInfo(playerChar_);
    const sf::Color col = info.color;
    drawFighterStage(target, ctx, static_cast<int>(playerChar_), col, 1.0f, 999.0f, fromChar ? 1.0f : enter, fromChar ? 1.0f : enter,
                     "ESCOLHA O MODO", fromChar ? "ESCOLHA SEU LUTADOR" : "", fromChar ? screenFrame_ / 14.0f : 1.0f);

    struct Mode { const char* label; const char* sub; const char* desc; const char* fill; const char* ink; };
    static const Mode kModes[] = {
        {"TORNEIO ARCADE", "4 STAGES + BOSS", "Quatro lutas eliminatórias e o chefe Batalhator no fim. O progresso fica salvo.", "#ff007f", "#ffffff"},
        {"LUTA SOLO", "1 CONFRONTO DIRETO", "Um confronto direto contra um rival sorteado do elenco.", "#00f5d4", "#000000"},
        {"MODO TREINO", "ESPANTALHO & BUILDS", "Pratique combos e teste habilidades sem limite de tempo contra o Espantalho.", "#d97706", "#ffffff"},
        {"MONTAR BUILD", "UPGRADE & HABILIDADES", "Escolha os dois golpes especiais e a passiva deste lutador.", "#ffe600", "#000000"},
        {"BOSS TEST", "BATALHATOR 2000", "Pule direto para a luta contra o chefe final de três fases.", "#ff5a1f", "#ffffff"},
        {"VOLTAR", "TROCAR DE LUTADOR", "Voltar para a seleção de lutador.", "#334155", "#ffffff"},
    };

    // ---------------- Cards dos modos ----------------
    for (int i = 0; i < 6; ++i) {
        sf::FloatRect r = modeButton(i);
        const bool sel = modeIndex_ == i;
        const float appear = springOut((static_cast<float>(screenFrame_) - i * 3.0f) / 26.0f);
        const float push = sel ? -36.0f : 0.0f;
        const float x = r.position.x + 700 * (1 - appear) + push, y = r.position.y, w = r.size.x, h = r.size.y;
        const Mode& m = kModes[i];
        ctx.save();
        ctx.globalAlpha = std::clamp(appear * 1.3f, 0.0f, 1.0f) * (sel ? 1.0f : 0.78f);
        ctx.translate(x + w / 2, y + h / 2);
        ctx.transform(1, 0, -0.2f, 1, 0, 0);
        const float sc = sel ? 1.0f + 0.02f * std::sin(t * 6) : 1.0f;
        ctx.scale(sc, sc);
        // Corpo do card: faixa colorida à esquerda (ícone) e área escura com o texto
        const float iconW = h + 10;
        ctx.fillStyle = "#000000"; ctx.fillRect(-w / 2 + (sel ? 10 : 6), -h / 2 + (sel ? 10 : 6), w, h);
        ctx.fillStyle = sel ? "#1f1533" : "#140f22"; ctx.fillRect(-w / 2, -h / 2, w, h);
        hatch(ctx, -w / 2, -h / 2, w, h, 9, "rgba(255,255,255,0.03)", 2);
        ctx.fillStyle = m.fill; ctx.fillRect(-w / 2, -h / 2, iconW, h);
        hatch(ctx, -w / 2, -h / 2, iconW, h, 8, "rgba(0,0,0,0.12)", 3);
        if (sel) { ctx.fillStyle = m.fill; ctx.fillRect(-w / 2 + iconW, h / 2 - 6, w - iconW, 6); }
        ctx.strokeStyle = sel ? "#ffffff" : "#000000"; ctx.lineWidth = 4; ctx.strokeRect(-w / 2, -h / 2, w, h);
        ctx.fillStyle = "#000000"; ctx.fillRect(-w / 2 + iconW - 2, -h / 2, 4, h);
        ctx.restore();

        // Ícone e textos (sem o cisalhamento, para ficarem nítidos)
        ctx.save();
        ctx.globalAlpha = std::clamp(appear * 1.3f, 0.0f, 1.0f) * (sel ? 1.0f : 0.72f);
        const float iconCx = x + (h + 10) / 2 + h * 0.1f;
        ctx.translate(iconCx, y + h / 2);
        const float bob = sel ? std::sin(t * 5) * 3 : 0;
        ctx.translate(0, bob);
        const float iconScale = (h / 84.0f) * (sel ? 1.08f : 0.94f);
        ctx.scale(iconScale, iconScale);
        modeIcon(ctx, i, t, m.fill);
        ctx.restore();

        ctx.save();
        ctx.globalAlpha = std::clamp(appear * 1.3f, 0.0f, 1.0f) * (sel ? 1.0f : 0.72f);
        const float tx = x + h + 30 - h * 0.10f;
        ctx.textAlign = "left"; ctx.textBaseline = "middle";
        ctx.font = i == 5 ? "20px 'Titan One'" : "25px 'Titan One'";
        outlined(ctx, m.label, tx, y + h / 2 - (i == 5 ? 5 : 9), sel ? "#ffffff" : "#e2e8f0", 2, 3);
        ctx.font = "900 11.5px Montserrat";
        ctx.fillStyle = sel ? m.fill : "#8c82a0";
        ctx.fillText(m.sub, tx + 2, y + h / 2 + (i == 5 ? 13 : 17));
        if (sel) {
            // Seta indicando o modo selecionado
            const float ax = x + w + 18 + std::sin(t * 8) * 5;
            ctx.beginPath(); ctx.moveTo(ax + 18, y + h / 2 - 16); ctx.lineTo(ax, y + h / 2); ctx.lineTo(ax + 18, y + h / 2 + 16); ctx.closePath();
            ctx.fillStyle = m.fill; ctx.fill(); ctx.strokeStyle = "#000000"; ctx.lineWidth = 3; ctx.stroke();
        }
        ctx.restore();
    }

    // ---------------- Barra de informações (rodapé) ----------------
    {
        const float rise = 170 * (1 - springOut(screenFrame_ / 24.0f));
        const float Y = 744 + rise;
        ctx.fillStyle = "#000000"; ctx.fillRect(0, Y, 1600, 156);
        hatch(ctx, 0, Y, 1600, 156, 10, "rgba(255,255,255,0.03)", 3);
        const Mode& m = kModes[std::clamp(modeIndex_, 0, 5)];
        ctx.fillStyle = m.fill; ctx.fillRect(0, Y, 1600, 5);
        // Etiqueta do modo atual
        ctx.save();
        ctx.translate(150, Y + 52);
        ctx.transform(1, 0, -0.2f, 1, 0, 0);
        inkBox(ctx, -110, -22, 220, 44, cv::Style(m.fill), 3.5f, 6, 0);
        ctx.restore();
        ctx.font = "18px 'Titan One'"; ctx.textAlign = "center"; ctx.textBaseline = "middle"; ctx.fillStyle = m.ink;
        ctx.fillText(m.label, 150, Y + 53);
        ctx.font = "800 20px 'Outfit ExtraBold'"; ctx.fillStyle = "#ffffff"; ctx.textAlign = "left";
        ctx.fillText(m.desc, 300, Y + 50);
        // Torneio salvo
        if (tournament_.active()) {
            const std::string who = upper(lookFor(static_cast<uint32_t>(tournament_.player)).name);
            const std::string text = "TORNEIO SALVO: " + who + " · " + upper(Tournament::stageName(tournament_.stage)) + " (STAGE " +
                                     std::to_string(tournament_.stage) + "/4)";
            ctx.font = "900 13px Montserrat"; ctx.fillStyle = "#ffe600";
            ctx.fillText(text, 300, Y + 86);
        }
        ctx.font = "600 13px 'Outfit SemiBold'"; ctx.fillStyle = "#6b6380"; ctx.textAlign = "right";
        ctx.fillText("CIMA / BAIXO escolhem  ·  ENTER confirma  ·  ESC volta", 1580, Y + 120);
    }
    drawNotice(ctx, 680);
}

// ---------------------------------------------------------------------------
// Montar Build (card grande do index.html)
// ---------------------------------------------------------------------------

void MenuScreens::drawPreview(sf::RenderTarget& target, Canvas& ctx, float reveal) {
    // Reproduz quadros já renderizados; o combate e seus rigs não existem neste menu.
    if (previewSkill_ == SkillId::None) return;
    bool available = livePreview_ != nullptr || previewVideo_.frameCount() != 0;
    if (available && !livePreview_) {
        const auto elapsed = previewClock_.getElapsedTime().asSeconds();
        const auto index = static_cast<std::size_t>(elapsed * previewVideo_.fps()) % previewVideo_.frameCount();
        available = previewVideo_.showFrame(index);
    }
    ctx.flush();
    const sf::FloatRect area = previewArea();
    const sf::Vector2u videoSize = livePreview_ ? livePreview_->getSize() : available ? previewVideo_.texture()->getSize() : sf::Vector2u(500u, 464u);
    sf::IntRect crop({0, 0}, {static_cast<int>(videoSize.x), static_cast<int>(videoSize.y)});
    if (livePreview_) {   // o quadro 3D é a janela inteira: recorta o meio com a proporção da área da prévia
        const int cw = std::min(static_cast<int>(videoSize.x), static_cast<int>(videoSize.y * area.size.x / area.size.y));
        crop = sf::IntRect({(static_cast<int>(videoSize.x) - cw) / 2, 0}, {cw, static_cast<int>(videoSize.y)});
    }
    const float sx = area.size.x / crop.size.x, sy = area.size.y / crop.size.y;
    // A transição dura tempo real, não 8, 20 ou 60 quadros conforme o FPS da máquina.
    const float g = glitchLen_ > 0 ? std::clamp(1.0f - glitchClock_.getElapsedTime().asSeconds() * 60.0f / glitchLen_, 0.0f, 1.0f) : 0.0f;
    std::mt19937 rng(frame_ * 7919u);
    std::uniform_real_distribution<float> u(-1.0f, 1.0f);

    // Se ainda não existe um quadro válido, mostre um estado leve de carga, não ruído por tempo indeterminado.
    if (!available) {
        ctx.fillStyle = "#10101e";
        ctx.fillRect(area.position.x, area.position.y, area.size.x, area.size.y);
        ctx.font = "18px 'Press Start 2P'"; ctx.textAlign = "center"; ctx.textBaseline = "middle";
        outlined(ctx, "PREVIA INDISPONIVEL",
                 area.position.x + area.size.x * 0.5f, area.position.y + area.size.y * 0.5f, "#ffe600", 0, 3);
        ctx.flush();
    } else {
        const sf::Texture& tex = livePreview_ ? *livePreview_ : *previewVideo_.texture();
        const int slices = g > 0.01f ? 18 : 1;
        const float sliceH = crop.size.y / static_cast<float>(slices);
        for (int i = 0; i < slices; ++i) {
            const float off = g > 0.01f ? u(rng) * 70.0f * g * (std::fabs(u(rng)) > 0.5f ? 1.0f : 0.15f) : 0.0f;
            sf::IntRect sub({crop.position.x, crop.position.y + static_cast<int>(i * sliceH)}, {crop.size.x, static_cast<int>(std::ceil(sliceH))});
            sf::Sprite sp(tex, sub);
            sp.setScale({sx, sy});
            sp.setPosition({area.position.x + off, area.position.y + i * sliceH * sy});
            sp.setColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(255 * reveal)));
            target.draw(sp);
            if (g > 0.05f) {
                // Separação de cores (vermelho e ciano deslocados)
                sf::RenderStates add;
                add.blendMode = sf::BlendAdd;
                sf::Sprite r(tex, sub), c(tex, sub);
                r.setScale({sx, sy}); c.setScale({sx, sy});
                r.setPosition({area.position.x + off + 9 * g, area.position.y + i * sliceH * sy});
                c.setPosition({area.position.x + off - 9 * g, area.position.y + i * sliceH * sy});
                r.setColor(sf::Color(255, 0, 0, static_cast<std::uint8_t>(110 * g)));
                c.setColor(sf::Color(0, 255, 255, static_cast<std::uint8_t>(110 * g)));
                target.draw(r, add);
                target.draw(c, add);
            }
        }
    }
    // Chiado, barras de tracking e PLAY
    if (g > 0.01f && available) {
        const float amount = g;
        for (int i = 0; i < static_cast<int>(36 * amount); ++i) {
            const float x = area.position.x + (u(rng) * 0.5f + 0.5f) * area.size.x, y = area.position.y + (u(rng) * 0.5f + 0.5f) * area.size.y;
            const float w = 4 + (u(rng) * 0.5f + 0.5f) * 40, v = 0.5f + 0.5f * u(rng);
            ctx.fillStyle = cv::rgba(static_cast<int>(255 * v), static_cast<int>(255 * v), static_cast<int>(255 * v), 0.5f * amount);
            ctx.fillRect(x, y, w, 2);
        }
        const float band = std::fmod(frame_ * 23.0f, area.size.y);
        ctx.fillStyle = cv::rgba(255, 255, 255, 0.18f * amount);
        ctx.fillRect(area.position.x, area.position.y + band, area.size.x, 26);
    }
    // Moldura de fita: cantos REC e câmera
    ctx.save();
    ctx.globalAlpha = reveal;
    ctx.font = "15px 'Press Start 2P'"; ctx.textAlign = "left"; ctx.textBaseline = "middle";
    if ((frame_ / 30) % 2 == 0) { ctx.beginPath(); ctx.arc(area.position.x + 34, area.position.y + area.size.y - 34, 7, 0, kPi * 2); ctx.fillStyle = "#ff1e42"; ctx.fill(); }
    outlined(ctx, "TREINO", area.position.x + 50, area.position.y + area.size.y - 33, "#ffffff", 0, 3);
    ctx.textAlign = "right";
    outlined(ctx, previewFront_ ? "CAM: FRENTE" : "CAM: COSTAS", area.position.x + area.size.x - 140, area.position.y + area.size.y - 33, "#ffe600", 0, 3);
    ctx.font = "800 13px 'Outfit ExtraBold'";
    outlined(ctx, "clique ou V para virar", area.position.x + area.size.x - 140, area.position.y + area.size.y - 60, "#e2e8f0", 0, 2);
    ctx.restore();
}

void MenuScreens::drawSlotPrompt(Canvas& ctx) {
    ctx.fillStyle = "rgba(3, 1, 10, 0.75)";
    ctx.fillRect(0, 0, 1600, 900);
    const auto skills = buildSkills();
    const SkillDef* def = (buildIndex_ >= 0 && buildIndex_ < static_cast<int>(skills.size())) ? SkillSystem::get(skills[static_cast<std::size_t>(buildIndex_)]) : nullptr;
    const SkillLoadout& l = loadouts_[playerChar_];
    ctx.save();
    ctx.translate(800, 470); ctx.rotate(-0.6f * kPi / 180); ctx.translate(-800, -470);
    inkBox(ctx, 520, 330, 560, 250, cv::Style("#120d20"), 4.5f, 10, 14, "#ffe600");
    ctx.font = "28px 'Titan One'"; ctx.textAlign = "center"; ctx.textBaseline = "middle";
    outlined(ctx, "EM QUAL ENCAIXE?", 800, 372, "#ffe600", 2, 3);
    if (def) {
        ctx.font = "800 16px 'Outfit ExtraBold'"; ctx.fillStyle = "#e2e8f0";
        ctx.fillText(std::string(def->name) + " vai substituir:", 800, 412);
    }
    static const char* kCol[] = {"#ff007f", "#00f5d4"};
    static const char* kInk[] = {"#ffffff", "#000000"};
    const std::array<SkillId, 2> cur = {l.spec1, l.spec2};
    for (int i = 0; i < 2; ++i) {
        sf::FloatRect r = slotPromptButton(i);
        const bool sel = slotPromptIndex_ == i;
        ctx.save();
        if (sel) ctx.translate(-3, -3);
        inkBox(ctx, r.position.x, r.position.y, r.size.x, r.size.y, cv::Style(kCol[i]), 3.5f, sel ? 8 : 5, 6, sel ? "#ffffff" : "#000000");
        ctx.font = "17px 'Titan One'"; ctx.fillStyle = kInk[i]; ctx.textAlign = "center";
        ctx.fillText(i == 0 ? "ESPECIAL 1  (Z)" : "ESPECIAL 2  (X)", r.position.x + r.size.x / 2, r.position.y + 24);
        const SkillDef* c = SkillSystem::get(cur[static_cast<std::size_t>(i)]);
        ctx.font = "800 13px 'Outfit ExtraBold'";
        ctx.fillText(c ? c->name : "vazio", r.position.x + r.size.x / 2, r.position.y + 50);
        ctx.restore();
    }
    ctx.font = "600 13px 'Outfit SemiBold'"; ctx.fillStyle = "#8c82a0";
    ctx.fillText("ESC cancela", 800, 560);
    ctx.restore();
}

void MenuScreens::drawBuild(sf::RenderTarget& target, Canvas& ctx) {
    // Arsenal: o lutador desliza para o canto esquerdo do mesmo palco; os três encaixes ficam no topo,
    // os nove golpes numa grade e o detalhe do golpe em foco à direita.
    const CharacterInfo& info = rosterInfo(playerChar_);
    const SkillLoadout& l = loadouts_[playerChar_];
    const std::vector<SkillId> skills = buildSkills();
    const float enter = springOut(screenFrame_ / 28.0f);
    const float t = frame_ / 60.0f;
    const bool fromHub = prevScreen_ == Screen::ModeSelect || prevScreen_ == Screen::CharSelect;
    // O ringue de treino ocupa o palco: aqui só o cabeçalho é desenhado (a faixa e o lutador ficariam escondidos).
    drawStageHeader(ctx, info.color, fromHub ? 1.0f : enter, "MONTAR BUILD",
                    fromHub ? (prevScreen_ == Screen::ModeSelect ? "ESCOLHA O MODO" : "ESCOLHA SEU LUTADOR") : "",
                    fromHub ? screenFrame_ / 14.0f : 1.0f);
    // O lutador "vira fita": a imagem falha como VHS e cai no ringue de treino com o golpe em foco.
    drawPreview(target, ctx, 1.0f);
    // Área de trabalho à direita, separada do ringue por uma diagonal de nanquim
    ctx.fillStyle = "#0c0816";
    ctx.beginPath(); ctx.moveTo(700, 96); ctx.lineTo(1600, 96); ctx.lineTo(1600, 744); ctx.lineTo(560, 744); ctx.closePath(); ctx.fill();
    hatch(ctx, 560, 96, 1040, 648, 10, "rgba(255,255,255,0.02)", 3);
    ctx.beginPath(); ctx.moveTo(700, 96); ctx.lineTo(560, 744); ctx.strokeStyle = "#000000"; ctx.lineWidth = 10; ctx.stroke();
    ctx.beginPath(); ctx.moveTo(714, 96); ctx.lineTo(574, 744); ctx.strokeStyle = hexColor(info.color); ctx.lineWidth = 4; ctx.stroke();

    auto tagColor = [](const SkillDef* d) {
        return d->type == SkillType::Passive ? std::string("#c084fc") : hexColor(ui::skillTagColor(d->tag));
    };
    auto slotOf = [&](SkillId id) { return l.spec1 == id ? 0 : (l.spec2 == id ? 1 : (l.passive == id ? 2 : -1)); };

    const std::string pL = keyBindings_.actionName(ActionKey::PunchL, true);
    const std::string pR = keyBindings_.actionName(ActionKey::PunchR, true);
    const std::string dKey = keyBindings_.actionName(ActionKey::Dodge, true);

    // ---------------- Encaixes equipados ----------------
    static const char* kSlotName[] = {"ESPECIAL 1", "ESPECIAL 2", "PASSIVA"};
    const std::string kSlotKey[] = {pL, pR, "ENTER"};
    static const char* kSlotCol[] = {"#ff007f", "#00f5d4", "#c084fc"};
    const std::array<SkillId, 3> equipped = {l.spec1, l.spec2, l.passive};
    for (int s = 0; s < 3; ++s) {
        sf::FloatRect r = buildSlot(s);
        const float appear = springOut((static_cast<float>(screenFrame_) - 4 - s * 3.0f) / 24.0f);
        const float flash = std::max(0.0f, 1.0f - (frame_ - equipFlash_[static_cast<std::size_t>(s)]) / 18.0f);
        const float pop = 1.0f + 0.08f * flash;
        ctx.save();
        ctx.globalAlpha = std::clamp(appear * 1.3f, 0.0f, 1.0f);
        ctx.translate(r.position.x + r.size.x / 2, r.position.y + r.size.y / 2 - 120 * (1 - appear));
        ctx.scale(pop, pop);
        ctx.transform(1, 0, -0.12f, 1, 0, 0);
        const float w = r.size.x, h = r.size.y;
        ctx.fillStyle = "#000000"; ctx.fillRect(-w / 2 + 7, -h / 2 + 7, w, h);
        ctx.fillStyle = "#140f22"; ctx.fillRect(-w / 2, -h / 2, w, h);
        hatch(ctx, -w / 2, -h / 2, w, h, 9, "rgba(255,255,255,0.03)", 2);
        ctx.fillStyle = kSlotCol[s]; ctx.fillRect(-w / 2, -h / 2, w, 30);
        ctx.strokeStyle = flash > 0.01f ? "#ffffff" : "#000000"; ctx.lineWidth = 4; ctx.strokeRect(-w / 2, -h / 2, w, h);
        ctx.fillStyle = "#000000"; ctx.fillRect(-w / 2, -h / 2 + 28, w, 3);
        if (flash > 0.01f) { ctx.fillStyle = cv::rgba(255, 255, 255, 0.35f * flash); ctx.fillRect(-w / 2, -h / 2, w, h); }
        ctx.restore();
        // Textos sem cisalhamento
        ctx.save();
        ctx.globalAlpha = std::clamp(appear * 1.3f, 0.0f, 1.0f);
        const float x = r.position.x, y = r.position.y - 120 * (1 - appear);
        ctx.font = "15px 'Titan One'"; ctx.textAlign = "left"; ctx.textBaseline = "middle";
        ctx.fillStyle = s == 1 ? "#000000" : "#ffffff";
        ctx.fillText(kSlotName[s], x + 18, y + 16);
        pill(ctx, x + r.size.x - 14, y + 5, kSlotKey[s], "#ffe600", "#000000", "900 10px Montserrat", 7, 18, 3, true);
        const SkillDef* def = SkillSystem::get(equipped[static_cast<std::size_t>(s)]);
        if (def) {
            const std::string tc = tagColor(def);
            pill(ctx, x + 16, y + 42, def->type == SkillType::Passive ? "PASSIVA" : upper(def->tag), tc.c_str(), "#000000", "12px 'Titan One'", 9, 22, 3);
            ctx.font = "19px 'Titan One'"; ctx.fillStyle = "#ffffff"; ctx.textAlign = "left";
            outlined(ctx, def->name, x + 16, y + 88, "#ffffff", 0, 2);
            if (def->type != SkillType::Passive) commandText(ctx, *def, x + r.size.x - 16, y + 53, true, "#ffe169", pL, pR);
        } else {
            ctx.font = "800 15px 'Outfit ExtraBold'"; ctx.fillStyle = "#6b6380"; ctx.textAlign = "center";
            ctx.fillText("ENCAIXE VAZIO", x + r.size.x / 2, y + 72);
        }
        ctx.restore();
    }

    // ---------------- Grade de golpes ----------------
    for (int i = 0; i < static_cast<int>(skills.size()) && i < 9; ++i) {
        const SkillDef* def = SkillSystem::get(skills[static_cast<std::size_t>(i)]);
        if (!def) continue;
        sf::FloatRect r = buildCard(i);
        const bool focus = i == buildIndex_;
        const int eq = slotOf(def->id);
        const float appear = springOut((static_cast<float>(screenFrame_) - 8 - i * 2.0f) / 24.0f);
        const float lift = focus ? -6.0f : 0.0f;
        const std::string tc = tagColor(def);
        ctx.save();
        ctx.globalAlpha = std::clamp(appear * 1.3f, 0.0f, 1.0f) * (focus || eq >= 0 ? 1.0f : 0.85f);
        ctx.translate(r.position.x + r.size.x / 2 + 400 * (1 - appear), r.position.y + r.size.y / 2 + lift);
        if (focus) ctx.scale(1.04f, 1.04f);
        const float w = r.size.x, h = r.size.y;
        inkBox(ctx, -w / 2, -h / 2, w, h, cv::Style(focus ? "#251a3d" : "#171126"), 3.5f, focus ? 9 : 5, 8, focus ? "#ffffff" : (eq >= 0 ? kSlotCol[eq] : "#000000"));
        ctx.fillStyle = tc; ctx.fillRect(-w / 2 + 2, -h / 2 + 2, 10, h - 4);
        hatch(ctx, -w / 2 + 12, -h / 2 + 2, w - 14, h - 4, 9, "rgba(255,255,255,0.025)", 2);
        ctx.restore();

        ctx.save();
        ctx.globalAlpha = std::clamp(appear * 1.3f, 0.0f, 1.0f) * (focus || eq >= 0 ? 1.0f : 0.8f);
        const float x = r.position.x + 400 * (1 - appear), y = r.position.y + lift;
        pill(ctx, x + 22, y + 10, def->type == SkillType::Passive ? "PASSIVA" : upper(def->tag), tc.c_str(), "#000000", "11px 'Titan One'", 8, 20, 3);
        if (eq >= 0) {
            static const char* kBadge[] = {"E1", "E2", "P"};
            ctx.beginPath(); ctx.arc(x + r.size.x - 18, y + 20, 13, 0, kPi * 2);
            ctx.fillStyle = kSlotCol[eq]; ctx.fill(); ctx.strokeStyle = "#000000"; ctx.lineWidth = 2.5f; ctx.stroke();
            ctx.font = "11px 'Titan One'"; ctx.fillStyle = eq == 1 ? "#000000" : "#ffffff"; ctx.textAlign = "center"; ctx.textBaseline = "middle";
            ctx.fillText(kBadge[eq], x + r.size.x - 18, y + 21);
        }
        ctx.font = std::string(def->name).size() > 18 ? "800 13.5px 'Outfit ExtraBold'" : "800 15px 'Outfit ExtraBold'";
        ctx.fillStyle = "#ffffff"; ctx.textAlign = "left"; ctx.textBaseline = "middle";
        ctx.fillText(def->name, x + 22, y + 52);
        if (def->type != SkillType::Passive) commandText(ctx, *def, x + 22, y + 82, false, "#00f5d4", pL, pR);
        ctx.font = "900 9.5px Montserrat"; ctx.fillStyle = "#8c82a0"; ctx.textAlign = "right";
        ctx.fillText(def->type == SkillType::Passive ? "PASSIVA" : (def->type == SkillType::Mode ? "MODO" : "COMANDO"), x + r.size.x - 12, y + 84);
        ctx.restore();
    }

    // ---------------- Detalhe do golpe em foco ----------------
    {
        const float appear = springOut((static_cast<float>(screenFrame_) - 10) / 26.0f);
        const float X = 1236 + 420 * (1 - appear), Y = 270, W = 330, H = 356;
        ctx.save();
        ctx.globalAlpha = std::clamp(appear * 1.3f, 0.0f, 1.0f);
        ctx.translate(X + W / 2, Y + H / 2); ctx.rotate(0.6f * kPi / 180); ctx.translate(-(X + W / 2), -(Y + H / 2));
        inkBox(ctx, X, Y, W, H, cv::Style("#120d20"), 4.5f, 9, 12);
        const SkillDef* def = (buildIndex_ >= 0 && buildIndex_ < static_cast<int>(skills.size())) ? SkillSystem::get(skills[static_cast<std::size_t>(buildIndex_)]) : nullptr;
        if (def) {
            const std::string tc = tagColor(def);
            ctx.save();
            ctx.beginPath(); ctx.roundRect(X, Y, W, 52, 12); ctx.clip();
            ctx.fillStyle = tc; ctx.fillRect(X, Y, W, 52);
            hatch(ctx, X, Y, W, 52, 9, "rgba(0,0,0,0.12)", 3);
            ctx.restore();
            ctx.fillStyle = "#000000"; ctx.fillRect(X, Y + 50, W, 4);
            ctx.font = "13px 'Titan One'"; ctx.fillStyle = "#000000"; ctx.textAlign = "left"; ctx.textBaseline = "middle";
            ctx.fillText(def->type == SkillType::Passive ? "PASSIVA · SEMPRE ATIVA" : (def->type == SkillType::Mode ? "ESPECIAL · MODO TEMPORÁRIO" : "ESPECIAL · COMANDO"), X + 18, Y + 26);
            ctx.font = "26px 'Titan One'";
            float cy = Y + 88;
            // Nome em uma ou duas linhas
            {
                std::string name = upper(def->name);
                if (ctx.measureText(name) > W - 36) {
                    auto sp = name.find(' ', name.size() / 2 - 2);
                    if (sp == std::string::npos) sp = name.rfind(' ');
                    outlined(ctx, name.substr(0, sp), X + 18, cy, "#ffffff", 2, 3);
                    cy += 30;
                    outlined(ctx, name.substr(sp + 1), X + 18, cy, "#ffffff", 2, 3);
                } else {
                    outlined(ctx, name, X + 18, cy, "#ffffff", 2, 3);
                }
            }
            cy += 30;
            if (def->type != SkillType::Passive) {
                ctx.save();
                ctx.translate(X + 18, cy);
                ctx.scale(1.4f, 1.4f);
                commandText(ctx, *def, 0, 0, false, "#ffe600", pL, pR);
                ctx.restore();
                cy += 30;
            }
            ctx.font = "600 15px 'Outfit SemiBold'"; ctx.fillStyle = "#e2e8f0"; ctx.textAlign = "left";
            wrapText(ctx, def->desc, X + 18, cy + 4, W - 36, 21);
            const int eq = slotOf(def->id);
            const std::string status = eq == 0 ? "EQUIPADO NO ESPECIAL 1" : (eq == 1 ? "EQUIPADO NO ESPECIAL 2" : (eq == 2 ? "PASSIVA EQUIPADA" : "PRONTO PARA EQUIPAR"));
            pill(ctx, X + 18, Y + H - 44, status, eq >= 0 ? kSlotCol[eq] : "#00f5d4", eq == 1 || eq < 0 ? "#000000" : "#ffffff", "900 11px Montserrat", 10, 26, 4);
        }
        ctx.restore();
    }

    // ---------------- Rodapé ----------------
    {
        const float rise = 170 * (1 - springOut(screenFrame_ / 24.0f));
        const float Y = 744 + rise;
        ctx.fillStyle = "#000000"; ctx.fillRect(0, Y, 1600, 156);
        hatch(ctx, 0, Y, 1600, 156, 10, "rgba(255,255,255,0.03)", 3);
        ctx.fillStyle = hexColor(info.color); ctx.fillRect(0, Y, 1600, 5);
        // Teclas
        struct Hint { std::string key; const char* what; };
        const Hint kHints[] = {{"SETAS", "navegar"}, {pL, "especial 1"}, {pR, "especial 2"}, {"ENTER", "equipar"}, {dKey, "desequipar"}, {"ESC", "salvar e sair"}};
        float hx = 40;
        for (const Hint& h : kHints) {
            ctx.font = "900 12px Montserrat";
            const float kw = ctx.measureText(h.key) + 18;
            inkBox(ctx, hx, Y + 40, kw, 28, cv::Style("#ffe600"), 2.5f, 3, 4);
            ctx.fillStyle = "#000000"; ctx.textAlign = "center"; ctx.textBaseline = "middle";
            ctx.fillText(h.key, hx + kw / 2, Y + 55);
            ctx.font = "800 15px 'Outfit ExtraBold'"; ctx.fillStyle = "#e2e8f0"; ctx.textAlign = "left";
            ctx.fillText(h.what, hx + kw + 10, Y + 55);
            hx += kw + 10 + ctx.measureText(h.what) + 30;
        }
        sf::FloatRect save = buildSaveButton();
        ctx.save();
        ctx.translate(save.position.x + save.size.x / 2, save.position.y + save.size.y / 2 + rise);
        ctx.transform(1, 0, -0.14f, 1, 0, 0);
        notchButton(ctx, -save.size.x / 2, -save.size.y / 2, save.size.x, save.size.y, "#ff007f", "#ffffff", "SALVAR & FECHAR", "", false, "20px 'Titan One'");
        ctx.restore();
    }
    drawNotice(ctx, 684);
    if (slotPrompt_) drawSlotPrompt(ctx);
    (void)t;
}

// ---------------------------------------------------------------------------
// Tela VS
// ---------------------------------------------------------------------------

void MenuScreens::drawVersus(sf::RenderTarget& target, Canvas& ctx) {
    // Tela VS: Colisão Titânica entre as duas metades com aproximação sincronizada,
    // recuo elástico, dentes de impacto comic, raios de alta tensão, estrela de impacto mangá,
    // ondas de choque duplas, faíscas direcionadas e o slam sísmico do emblema VS.
    const float f = static_cast<float>(screenFrame_);
    const float t = frame_ / 60.0f;
    std::mt19937 rng(frame_ * 31u);
    std::uniform_real_distribution<float> u(-1.0f, 1.0f);

    // 1. Aproximação sincronizada e aceleração de alta velocidade das duas metades
    const float pIn = std::clamp(f / 24.0f, 0.0f, 1.0f);
    const float inSpeed = std::pow(pIn, 2.4f);

    // 2. Colisão e recuo elástico (recoil): no instante do choque (f >= 24), as duas metades
    // se chocam violentamente no centro, comprimem-se e vibram em oscilação elástica amortecida.
    float recoil = 0.0f;
    float shake = 0.0f;
    if (f >= 24.0f) {
        const float dt = f - 24.0f;
        recoil = std::exp(-dt * 0.15f) * std::sin(dt * 0.62f) * 28.0f;
        shake = (dt < 30.0f) ? std::exp(-dt * 0.13f) : 0.0f;
    }
    const float shx = (((static_cast<int>(f) % 2 == 0) ? 24.0f : -24.0f) * shake) + u(rng) * 6.0f * shake;
    const float shy = (((static_cast<int>(f) % 3 == 0) ? 10.0f : -10.0f) * shake) + u(rng) * 4.0f * shake;

    // Deslocamento horizontal de cada metade
    const float oLeft = -1380.0f * (1.0f - inSpeed) + recoil;
    const float oRight = 1380.0f * (1.0f - inSpeed) - recoil;

    ctx.fillStyle = "#05030a"; ctx.fillRect(0, 0, 1600, 900);
    ctx.save();
    ctx.translate(shx, shy);

    auto side = [&](uint32_t charId, bool left, float o) {
        const CharacterInfo& info = rosterInfo(charId);
        const sf::Color c = info.color;
        ctx.save();
        ctx.translate(o, 0);

        // Painel diagonal com degradê temático
        const bool panelVisible = left ? o + 880.0f >= -32.0f : o + 720.0f <= 1632.0f;
        if (panelVisible) {
            ctx.beginPath();
            if (left) {
                ctx.moveTo(-60, 0); ctx.lineTo(880, 0); ctx.lineTo(720, 900); ctx.lineTo(-60, 900);
            } else {
                ctx.moveTo(880, 0); ctx.lineTo(1660, 0); ctx.lineTo(1660, 900); ctx.lineTo(720, 900);
            }
            ctx.closePath();
            auto g = ctx.createLinearGradient(left ? 0.0f : 1600.0f, 0, 800, 0);
            g->addColorStop(0, sf::Color(c.r / 2, c.g / 2, c.b / 2));
            g->addColorStop(1, sf::Color(c.r / 7, c.g / 7, c.b / 7));
            ctx.fillStyle = g; ctx.fill();

            // Retícula temática: evita construir ícones fora da janela durante a entrada.
            ctx.fillStyle = cv::rgba(c.r, c.g, c.b, 0.22f);
            int row = 0;
            for (float y = 24; y < 900; y += 32, ++row) {
                const float edge = 880 - 160 * y / 900;
                const float r = 3.0f + 9.0f * (left ? 1 - y / 900 : y / 900);
                for (float x = (left ? 10.0f : edge + 20) + (row % 2) * 16.0f; left ? x < edge - r - 6 : x < 1600; x += 32)
                    if (x + o + r >= -32.0f && x + o - r <= 1632.0f)
                        charIcon(ctx, static_cast<int>(charId), x, y, r);
            }
        }

        // Linhas de velocidade cinéticas: na aproximação (f < 24) voam em alta velocidade em direção ao centro
        const int numLines = (f < 24) ? 24 : 12;
        for (int i = 0; i < numLines; ++i) {
            const float y = std::fmod(i * 79.0f + frame_ * (left ? 22.0f : -22.0f) + 900, 900.0f);
            const float len = (f < 24) ? (200 + (i % 4) * 140) : (110 + (i % 3) * 70);
            const float x0 = left ? std::fmod(i * 173.0f + frame_ * 28.0f, 800.0f) : 800 + std::fmod(i * 173.0f - frame_ * 28.0f, 800.0f);
            ctx.fillStyle = (f < 24) ? cv::rgba(255, 255, 255, 0.18f) : cv::rgba(255, 255, 255, 0.06f);
            ctx.fillRect(x0, y, len, (f < 24) ? 4.5f : 2.5f);
        }
        ctx.restore();

        // Lutador grande com sombra dura e hitstop / escala pulsante no choque
        const float pulse = (f >= 24 && f < 42) ? (1.0f + 0.09f * std::exp(-(f - 24) * 0.20f) * std::sin((f - 24) * 0.65f)) : 1.0f;
        const float cx = (left ? 400.0f : 1200.0f) + o * 1.15f;
        ctx.flush();
        const sf::FloatRect box{{cx - 330 * pulse, 90 - (pulse - 1.0f) * 160.0f}, {660 * pulse, 600 * pulse}};
        const uint32_t sId = left ? skin(playerChar_) : skin(enemyChar_);
        if (box.position.x + box.size.x < -80.0f || box.position.x > 1680.0f) return;
        ui::portrait(target, *sprites_, info.key, {{box.position.x + (left ? 18.0f : -18.0f), box.position.y + 14}, box.size}, frame_, true, sf::Color(0, 0, 0, 190), sId);
        ui::portrait(target, *sprites_, info.key, box, frame_, true, sf::Color::White, sId);

        // Placa estilizada do nome
        ctx.save();
        ctx.translate(cx, 700);
        ctx.rotate((left ? -3.0f : 3.0f) * kPi / 180);
        const std::string name = upper(charId == 8 ? std::string("BATALHATOR 2000") : std::string(info.name));
        ctx.font = "64px 'Titan One'"; ctx.textAlign = "center"; ctx.textBaseline = "alphabetic";
        for (int l = 0; l < 3; ++l) { ctx.fillStyle = l == 0 ? "#000000" : (l == 1 ? "#1a1a1a" : "#333333"); ctx.fillText(name, 9 - l * 3.0f, 9 - l * 3.0f); }
        ctx.strokeStyle = "#000000"; ctx.lineWidth = 10; ctx.lineJoin = "round"; ctx.strokeText(name, 0, 0);
        ctx.fillStyle = "#ffffff"; ctx.fillText(name, 0, 0);
        ctx.font = "900 14px Montserrat";
        const std::string title = upper(info.title);
        const float tw = ctx.measureText(title) + 26;
        inkBox(ctx, -tw / 2, 14, tw, 26, cv::Style(hexColor(c)), 2.5f, 4, 2);
        ctx.fillStyle = (c.r * 0.3f + c.g * 0.59f + c.b * 0.11f) > 150 ? "#000000" : "#ffffff";
        ctx.textBaseline = "middle"; ctx.fillText(title, 0, 28);
        if (sId > 0) {
            const SkinDef& sk = rosterSkin(charId, sId);
            pill(ctx, 0, 48, sk.name, hexColor(sk.color).c_str(), "#000000", "900 11px Montserrat", 6, 20, 2, true);
        }
        ctx.restore();
    };
    side(playerChar_, true, oLeft);
    side(enemyChar_, false, oRight);

    // Epicentro do impacto da colisão
    const float cx = 800.0f, cy = 380.0f;

    // 3. Efeitos de Colisão na Divisória (f >= 24)
    if (f >= 24.0f) {
        // A. Dentes de impacto comic (serrilhado que morde e tritura a linha divisória com o choque)
        const float toothAmp = std::exp(-(f - 24.0f) * 0.09f) * 26.0f;
        ctx.beginPath();
        for (float y = 0; y <= 900; y += 30.0f) {
            const float baseEdge = 880.0f - 160.0f * (y / 900.0f);
            const float tooth = ((static_cast<int>(y) / 30) % 2 == 0 ? toothAmp : -toothAmp) * (1.0f - std::abs(y - cy) / 600.0f);
            if (y == 0) ctx.moveTo(baseEdge + tooth, y);
            else ctx.lineTo(baseEdge + tooth, y);
        }
        ctx.strokeStyle = "#000000"; ctx.lineWidth = 26; ctx.lineJoin = "round"; ctx.stroke();
        ctx.strokeStyle = "#ffffff"; ctx.lineWidth = 6; ctx.stroke();

        // B. Raios de alta tensão elétrica crepitando ao longo da fratura
        if (f < 54.0f) {
            const float lightningAlpha = std::clamp((54.0f - f) / 30.0f, 0.0f, 1.0f);
            for (int bolt = 0; bolt < 2; ++bolt) {
                ctx.beginPath();
                std::mt19937 boltRng(frame_ * 97u + bolt * 313u);
                std::uniform_real_distribution<float> bDist(-1.0f, 1.0f);
                float curX = 880.0f, curY = 0.0f;
                ctx.moveTo(curX, curY);
                while (curY < 900.0f) {
                    curY += 36.0f + bDist(boltRng) * 16.0f;
                    const float targetEdge = 880.0f - 160.0f * (curY / 900.0f);
                    curX = targetEdge + bDist(boltRng) * (42.0f * lightningAlpha);
                    ctx.lineTo(curX, curY);
                }
                ctx.strokeStyle = (bolt == 0) ? cv::rgba(0, 240, 255, 0.92f * lightningAlpha) : cv::rgba(255, 230, 0, 0.95f * lightningAlpha);
                ctx.lineWidth = 4.0f;
                ctx.stroke();
            }
        }

        // C. Estrela de Impacto Comic / Manga Starburst (picos pontiagudos explodindo do ponto de contato)
        if (f < 48.0f) {
            const float starK = (f - 24.0f) / 24.0f;
            const float starAlpha = 1.0f - starK;
            const int numSpikes = 16;
            const float rOuter = 50.0f + 290.0f * std::pow(starK, 0.4f);
            const float rInner = 20.0f + 75.0f * starK;
            ctx.beginPath();
            for (int i = 0; i < numSpikes * 2; ++i) {
                const float angle = i * kPi / numSpikes + (frame_ % 4) * 0.05f;
                const float rad = (i % 2 == 0) ? rOuter : rInner;
                const float px = cx + std::cos(angle) * rad;
                const float py = cy + std::sin(angle) * rad * 0.85f;
                if (i == 0) ctx.moveTo(px, py);
                else ctx.lineTo(px, py);
            }
            ctx.closePath();
            ctx.fillStyle = cv::rgba(255, 255, 255, 0.85f * starAlpha);
            ctx.fill();
            ctx.strokeStyle = cv::rgba(255, 230, 0, 0.95f * starAlpha);
            ctx.lineWidth = 5.0f;
            ctx.stroke();
        }

        // D. Ondas de choque radiais expansivas duplas
        if (f < 56.0f) {
            const float k = (f - 24.0f) / 32.0f;
            // Onda externa branca
            ctx.beginPath();
            ctx.ellipse(cx, cy, 60.0f + 560.0f * k, 45.0f + 420.0f * k, -12 * kPi / 180, 0, kPi * 2);
            ctx.strokeStyle = cv::rgba(255, 255, 255, 0.9f * (1.0f - k));
            ctx.lineWidth = 18.0f * (1.0f - k) + 2.0f;
            ctx.stroke();

            // Onda interna dourada de energia concentrada
            ctx.beginPath();
            ctx.ellipse(cx, cy, 30.0f + 380.0f * k, 22.0f + 280.0f * k, -12 * kPi / 180, 0, kPi * 2);
            ctx.strokeStyle = cv::rgba(255, 230, 0, 0.95f * (1.0f - k));
            ctx.lineWidth = 10.0f * (1.0f - k) + 1.5f;
            ctx.stroke();
        }

        // E. Faíscas e estilhaços direcionais de impacto
        if (f < 62.0f) {
            const float spkK = (f - 24.0f);
            const CharacterInfo& pInfo = rosterInfo(playerChar_);
            const CharacterInfo& eInfo = rosterInfo(enemyChar_);
            for (int i = 0; i < 32; ++i) {
                const bool toLeft = (i % 2 == 0);
                const float baseAngle = toLeft ? (kPi + (i % 5 - 2) * 0.35f) : ((i % 5 - 2) * 0.35f);
                const float speed = 12.0f + (i % 7) * 4.5f;
                const float dist = spkK * speed;
                const float px = cx + std::cos(baseAngle) * dist;
                const float py = cy + std::sin(baseAngle) * dist * 0.9f + (i % 3 - 1) * 60.0f;
                const float sparkLen = std::max(6.0f, 28.0f - spkK * 0.6f);
                const float sparkAlpha = std::clamp(1.0f - spkK / 36.0f, 0.0f, 1.0f);

                ctx.save();
                ctx.translate(px, py);
                ctx.rotate(baseAngle);
                sf::Color sc = toLeft ? pInfo.color : eInfo.color;
                if (i % 3 == 0) sc = sf::Color(255, 240, 50);
                else if (i % 4 == 0) sc = sf::Color(255, 255, 255);
                ctx.fillStyle = cv::rgba(sc.r, sc.g, sc.b, sparkAlpha);
                ctx.fillRect(-sparkLen / 2, -3.0f, sparkLen, 6.0f);
                ctx.restore();
            }
        }
    }

    // 4. Emblema "VS" cravando na tela com impacto e relevo 3D
    if (f >= 24.0f) {
        const float dt = f - 24.0f;
        const float slamK = std::clamp(dt / 8.0f, 0.0f, 1.0f);
        const float pop = springOut(slamK);
        const float sc = (dt < 8.0f) ? (3.6f - 2.6f * pop) : (1.0f + 0.03f * std::sin(t * 6.0f));

        ctx.save();
        ctx.translate(cx, cy);
        ctx.rotate(-7 * kPi / 180 + std::sin(t * 3) * 0.02f);
        ctx.scale(sc, sc);
        ctx.font = "220px 'Titan One'"; ctx.textAlign = "center"; ctx.textBaseline = "middle";
        static const char* kExt[] = {"#000000", "#3a0020", "#62003a", "#8a0050", "#b20066", "#d6007c", "#ff007f"};
        for (int l = 0; l < 7; ++l) { ctx.fillStyle = kExt[l]; ctx.fillText("VS", 18 - l * 3.0f, 18 - l * 3.0f); }
        ctx.strokeStyle = "#000000"; ctx.lineWidth = 14; ctx.lineJoin = "round"; ctx.strokeText("VS", 0, 0);
        ctx.fillStyle = "#ffe600"; ctx.fillText("VS", 0, 0);
        ctx.fillStyle = "rgba(255,255,255,0.65)"; ctx.fillText("VS", -3, -5);
        ctx.fillStyle = "#ffe600"; ctx.fillText("VS", 0, 0);
        ctx.restore();
    }
    ctx.restore();

    // 5. Etiqueta da luta e comparativo de atributos frente a frente
    if (f > 30) {
        const float a = std::clamp((f - 30) / 14.0f, 0.0f, 1.0f);
        ctx.save();
        ctx.globalAlpha = a;
        ctx.translate(800, 520);
        ctx.rotate(-3 * kPi / 180);
        ctx.font = "24px 'Titan One'";
        const float w = ctx.measureText(versusTag_) + 50;
        ctx.transform(1, 0, -0.18f, 1, 0, 0);
        inkBox(ctx, -w / 2, -24, w, 48, cv::Style("#ffe600"), 4, 7, 0);
        ctx.fillStyle = "#000000"; ctx.textAlign = "center"; ctx.textBaseline = "middle";
        ctx.fillText(versusTag_, 0, 2);
        ctx.restore();

        const CharacterInfo& p = rosterInfo(playerChar_);
        const CharacterInfo& e = rosterInfo(enemyChar_);
        const std::array<std::pair<const char*, std::pair<int, int>>, 4> rows = {{{"FORÇA", {p.forca, e.forca}}, {"STAMINA", {p.stamina, e.stamina}},
                                                                                   {"ALCANCE", {p.alcance, e.alcance}}, {"RESISTÊNCIA", {p.resistencia, e.resistencia}}}};
        const float by = 770;
        ctx.save();
        ctx.globalAlpha = a;
        inkBox(ctx, 470, by - 18, 660, 128, cv::Style("rgba(8, 5, 14, 0.92)"), 3.5f, 6, 10);
        for (std::size_t i = 0; i < rows.size(); ++i) {
            const float y = by + i * 28;
            const float grow = std::clamp((f - 34 - i * 4) / 20.0f, 0.0f, 1.0f);
            const int pv = rows[i].second.first, ev = rows[i].second.second;
            ctx.font = "900 12px Montserrat"; ctx.fillStyle = "#ffffff"; ctx.textAlign = "center"; ctx.textBaseline = "middle";
            ctx.fillText(rows[i].first, 800, y);
            const float lw = 200 * pv / 100.0f * grow, rw = 200 * ev / 100.0f * grow;
            ctx.fillStyle = "#1e1730"; ctx.fillRect(500, y - 7, 220, 14); ctx.fillRect(880, y - 7, 220, 14);
            ctx.fillStyle = hexColor(p.color); ctx.fillRect(720 - lw, y - 7, lw, 14);
            ctx.fillStyle = hexColor(e.color); ctx.fillRect(880, y - 7, rw, 14);
            ctx.font = "13px 'Titan One'";
            ctx.fillStyle = pv >= ev ? "#ffe600" : "#8c82a0"; ctx.textAlign = "right"; ctx.fillText(std::to_string(pv), 492, y + 1);
            ctx.fillStyle = ev >= pv ? "#ffe600" : "#8c82a0"; ctx.textAlign = "left"; ctx.fillText(std::to_string(ev), 1108, y + 1);
        }
        ctx.restore();
    }

    // 6. Clarão potente de impacto (whiteout de colisão) no momento exato do choque
    if (f >= 24 && f < 32) {
        const float flashK = (f - 24.0f) / 8.0f;
        const float alpha = (f == 24.0f) ? 0.92f : (0.75f * (1.0f - flashK));
        ctx.fillStyle = cv::rgba(255, 255, 255, alpha);
        ctx.fillRect(0, 0, 1600, 900);
    }
    if (f > 150) {
        const float k = (f - 150) / 20.0f;
        ctx.fillStyle = cv::rgba(255, 255, 255, std::min(1.0f, k));
        ctx.fillRect(0, 0, 1600, 900);
    }
    if (f >= 30 && (frame_ / 20) % 2 == 0) {
        ctx.font = "800 15px 'Outfit ExtraBold'"; ctx.fillStyle = "#e2e8f0"; ctx.textAlign = "center"; ctx.textBaseline = "middle";
        ctx.fillText("ENTER para lutar", 800, 880);
    }
    if (!notice_.empty()) drawNotice(ctx, 840);
}

// ---------------------------------------------------------------------------
// Torneio
// ---------------------------------------------------------------------------

void MenuScreens::drawMatchCard(sf::RenderTarget& target, Canvas& ctx, float x, float y, const BracketMatch& m, const char* label,
                                bool current, bool locked, bool bossCard) {
    // Confronto em duas placas empilhadas: retrato, nome e a cor do lutador; perdedor riscado.
    const float w = 250, ph = 50, gap = 4;
    const float alpha = locked ? 0.35f : 1.0f;
    const float t = frame_ / 60.0f;
    ctx.save();
    ctx.globalAlpha = alpha;
    ctx.font = "900 10.5px Montserrat"; ctx.textAlign = "left"; ctx.textBaseline = "middle";
    ctx.fillStyle = bossCard ? "#ff1e42" : (current ? "#ffe600" : "#8c82a0");
    ctx.fillText(label, x + 4, y - 12);
    if (current) {
        const float pulse = 0.5f + 0.5f * std::sin(t * 6);
        ctx.strokeStyle = cv::rgba(255, 230, 0, 0.4f + 0.6f * pulse); ctx.lineWidth = 4;
        ctx.beginPath(); ctx.roundRect(x - 7, y - 7, w + 14, ph * 2 + gap + 14, 10); ctx.stroke();
        pill(ctx, x + w - 4, y - 22, "SUA LUTA", "#ffe600", "#000000", "900 10px Montserrat", 8, 18, 3, true);
    }
    auto plate = [&](int c, float py) {
        const bool won = m.winner >= 0 && m.winner == c;
        const bool lost = m.winner >= 0 && c >= 0 && m.winner != c;
        const bool hidden = bossCard && c == Tournament::kBoss && m.winner < 0;
        const bool you = c >= 0 && c == tournament_.player;
        if (c < 0) {
            inkBox(ctx, x, py, w, ph, cv::Style("#120d20"), 2.5f, 3, 6, "#2a2340");
            ctx.font = "34px 'Titan One'"; ctx.fillStyle = "#2a2340"; ctx.textAlign = "center";
            ctx.fillText("?", x + ph / 2 + 2, py + ph / 2 + 2);
            ctx.font = "900 12px Montserrat"; ctx.fillStyle = "#4a425c"; ctx.textAlign = "left";
            ctx.fillText("A DEFINIR", x + ph + 14, py + ph / 2);
            return;
        }
        const CharacterInfo& info = rosterInfo(static_cast<uint32_t>(c));
        const std::string col = hidden ? std::string("#ff1e42") : hexColor(info.color);
        inkBox(ctx, x, py, w, ph, cv::Style(won ? "#2a1f45" : "#171126"), 3, 4, 6, won ? "#ffe600" : "#000000");
        ctx.fillStyle = lost ? "#334155" : col; ctx.fillRect(x + 2, py + 2, ph - 2, ph - 4);
        ctx.fillStyle = "#000000"; ctx.fillRect(x + ph, py + 2, 3, ph - 4);
        ctx.flush();
        sf::Color tint = hidden ? sf::Color(20, 0, 10) : (lost ? sf::Color(90, 90, 105) : sf::Color::White);
        tint.a = static_cast<std::uint8_t>(255 * alpha);
        ui::portraitFace(target, *sprites_, info.key, {{x + 4, py + 2}, {ph - 6, ph - 4}}, frame_, false, tint);
        if (hidden) {
            ctx.fillStyle = cv::rgba(255, 30, 66, 0.25f + 0.2f * std::sin(t * 4));
            ctx.fillRect(x + 2, py + 2, ph - 2, ph - 4);
        }
        const std::string name = hidden ? "SUPREMO BOSS" : upper(lookFor(static_cast<uint32_t>(c)).name);
        ctx.font = "15px 'Titan One'"; ctx.textAlign = "left"; ctx.textBaseline = "middle";
        ctx.fillStyle = lost ? "#64748b" : "#ffffff";
        ctx.fillText(name, x + ph + 14, py + ph / 2 - (you ? 6 : 0));
        if (you) { ctx.font = "900 9.5px Montserrat"; ctx.fillStyle = "#ffe600"; ctx.fillText("VOCÊ", x + ph + 14, py + ph / 2 + 11); }
        if (won) {
            ctx.beginPath(); ctx.arc(x + w - 20, py + ph / 2, 11, 0, kPi * 2);
            ctx.fillStyle = "#ffe600"; ctx.fill(); ctx.strokeStyle = "#000000"; ctx.lineWidth = 2.5f; ctx.stroke();
            ctx.beginPath(); ctx.moveTo(x + w - 26, py + ph / 2); ctx.lineTo(x + w - 21, py + ph / 2 + 5); ctx.lineTo(x + w - 14, py + ph / 2 - 5);
            ctx.strokeStyle = "#000000"; ctx.lineWidth = 3; ctx.stroke();
        }
        if (lost) {
            ctx.beginPath(); ctx.moveTo(x + ph + 8, py + ph / 2 + 1); ctx.lineTo(x + w - 12, py + ph / 2 + 1);
            ctx.strokeStyle = "#ff1e42"; ctx.lineWidth = 3; ctx.stroke();
        }
    };
    plate(m.p1, y);
    plate(m.p2, y + ph + gap);
    ctx.restore();
}

void MenuScreens::drawBracket(sf::RenderTarget& target, Canvas& ctx) {
    const Tournament& t = tournament_;
    const float f = static_cast<float>(screenFrame_);
    const float time = frame_ / 60.0f;
    drawStageHeader(ctx, sf::Color(255, 0, 127), springOut(f / 24.0f), "TORNEIO ARCADE", "", 1.0f);

    // Trilha de progresso
    {
        static const char* kSteps[] = {"QUARTAS", "SEMIFINAL", "FINAL", "BOSS", "CAMPEÃO"};
        const float y = 124, x0 = 420, step = 190;
        for (int i = 0; i < 5; ++i) {
            const bool done = t.stage > i + 1 || (i == 4 && t.champion());
            const bool now = t.stage == i + 1;
            if (i < 4) {
                ctx.fillStyle = done ? "#ffe600" : "#2a2340";
                ctx.fillRect(x0 + i * step + 16, y - 3, step - 32, 6);
            }
            ctx.beginPath(); ctx.arc(x0 + i * step, y, now ? 14 : 11, 0, kPi * 2);
            ctx.fillStyle = done ? "#ffe600" : (now ? "#ff007f" : "#1a1428"); ctx.fill();
            ctx.strokeStyle = now ? "#ffffff" : "#000000"; ctx.lineWidth = 3; ctx.stroke();
            ctx.font = "900 11px Montserrat"; ctx.textAlign = "center"; ctx.textBaseline = "middle";
            ctx.fillStyle = now ? "#ffffff" : (done ? "#ffe600" : "#4a425c");
            ctx.fillText(kSteps[i], x0 + i * step, y + 26);
        }
    }

    // Árvore
    const float colX[] = {60, 420, 780, 1140};
    const float qfY[] = {184, 330, 476, 622};
    const float sfY[] = {257, 549};
    const float finY = 403, cardW = 250, cardH = 104;
    const float pulse = std::fmod(time * 0.9f, 1.0f);
    auto link = [&](float x0, float y0, float x1, float y1, bool lit) {
        const float mx = (x0 + x1) / 2;
        auto seg = [&](float ax, float ay, float bx, float by) {
            ctx.beginPath(); ctx.moveTo(ax, ay); ctx.lineTo(bx, by);
            ctx.strokeStyle = "#000000"; ctx.lineWidth = lit ? 9 : 6; ctx.stroke();
            ctx.strokeStyle = lit ? "#ffe600" : "#2a2340"; ctx.lineWidth = lit ? 4 : 3; ctx.stroke();
        };
        seg(x0, y0, mx, y0); seg(mx, y0, mx, y1); seg(mx, y1, x1, y1);
        if (lit) {
            // Pulso de luz percorrendo o caminho do jogador
            const float total = std::fabs(mx - x0) + std::fabs(y1 - y0) + std::fabs(x1 - mx);
            float d = pulse * total, px, py;
            if (d < mx - x0) { px = x0 + d; py = y0; }
            else if ((d -= mx - x0) < std::fabs(y1 - y0)) { px = mx; py = y0 + (y1 > y0 ? d : -d); }
            else { d -= std::fabs(y1 - y0); px = mx + d; py = y1; }
            auto g = ctx.createRadialGradient(px, py, 0, px, py, 16);
            g->addColorStop(0, "rgba(255,255,255,0.95)"); g->addColorStop(1, "rgba(255,230,0,0)");
            ctx.fillStyle = g; ctx.beginPath(); ctx.arc(px, py, 16, 0, kPi * 2); ctx.fill();
        }
    };
    for (int i = 0; i < 4; ++i) link(colX[0] + cardW + 8, qfY[i] + cardH / 2, colX[1] - 8, sfY[i / 2] + cardH / 2, i == 0 && t.stage > 1);
    for (int i = 0; i < 2; ++i) link(colX[1] + cardW + 8, sfY[i] + cardH / 2, colX[2] - 8, finY + cardH / 2, i == 0 && t.stage > 2);
    link(colX[2] + cardW + 8, finY + cardH / 2, colX[3] - 8, finY + cardH / 2, t.stage > 3);
    link(colX[3] + cardW + 8, finY + cardH / 2, 1450, finY + cardH / 2, t.champion());

    static const char* kQf[] = {"LUTA 1", "LUTA 2", "LUTA 3", "LUTA 4"};
    auto appearAt = [&](int col) { return springOut((f - col * 5.0f) / 22.0f); };
    for (int i = 0; i < 4; ++i) {
        ctx.save(); ctx.translate(-200 * (1 - appearAt(0)), 0);
        drawMatchCard(target, ctx, colX[0], qfY[i], t.quarter[static_cast<std::size_t>(i)], kQf[i], t.stage == 1 && i == 0, false, false);
        ctx.restore();
    }
    for (int i = 0; i < 2; ++i) {
        ctx.save(); ctx.translate(0, 60 * (1 - appearAt(1)));
        drawMatchCard(target, ctx, colX[1], sfY[i], t.semi[static_cast<std::size_t>(i)], i == 0 ? "SEMIFINAL 1" : "SEMIFINAL 2", t.stage == 2 && i == 0, t.stage < 2, false);
        ctx.restore();
    }
    ctx.save(); ctx.translate(0, 60 * (1 - appearAt(2)));
    drawMatchCard(target, ctx, colX[2], finY, t.final, "GRANDE FINAL", t.stage == 3, t.stage < 3, false);
    ctx.restore();
    ctx.save(); ctx.translate(200 * (1 - appearAt(3)), 0);
    drawMatchCard(target, ctx, colX[3], finY, t.boss, "DESAFIO BOSS", t.stage == 4, t.stage < 4, true);
    ctx.restore();

    // Troféu do campeão
    {
        const float cx = 1500, cy = finY + cardH / 2;
        ctx.save();
        ctx.translate(cx, cy + std::sin(time * 2) * 4);
        ctx.globalAlpha = t.champion() ? 1.0f : 0.45f;
        if (t.champion()) {
            for (int i = 0; i < 12; ++i) {
                const float a = i * kPi / 6 + time * 0.5f;
                ctx.beginPath(); ctx.moveTo(0, 0); ctx.lineTo(std::cos(a) * 90, std::sin(a) * 90); ctx.lineTo(std::cos(a + 0.15f) * 90, std::sin(a + 0.15f) * 90); ctx.closePath();
                ctx.fillStyle = "rgba(255, 230, 0, 0.18)"; ctx.fill();
            }
        }
        ctx.scale(1.6f, 1.6f);
        ctx.strokeStyle = "#000000"; ctx.lineWidth = 4; ctx.lineJoin = "round";
        ctx.beginPath(); ctx.moveTo(-20, -26); ctx.lineTo(20, -26); ctx.lineTo(16, 2); ctx.quadraticCurveTo(0, 14, -16, 2); ctx.closePath();
        ctx.fillStyle = "#ffe600"; ctx.fill(); ctx.stroke();
        ctx.beginPath(); ctx.arc(-22, -14, 9, kPi * 0.5f, kPi * 1.5f); ctx.stroke();
        ctx.beginPath(); ctx.arc(22, -14, 9, -kPi * 0.5f, kPi * 0.5f); ctx.stroke();
        ctx.fillStyle = "#ffe600";
        ctx.beginPath(); ctx.rect(-5, 8, 10, 12); ctx.fill(); ctx.stroke();
        ctx.beginPath(); ctx.rect(-16, 20, 32, 8); ctx.fill(); ctx.stroke();
        ctx.restore();
        ctx.font = "900 11px Montserrat"; ctx.textAlign = "center"; ctx.fillStyle = t.champion() ? "#ffe600" : "#4a425c";
        ctx.fillText(t.champion() ? "CAMPEÃO!" : "CAMPEÃO", cx, cy + 70);
    }

    // Rodapé: próxima luta e botões
    {
        const float rise = 170 * (1 - springOut(f / 24.0f));
        const float Y = 770 + rise;
        ctx.fillStyle = "#000000"; ctx.fillRect(0, Y, 1600, 130);
        hatch(ctx, 0, Y, 1600, 130, 10, "rgba(255,255,255,0.03)", 3);
        ctx.fillStyle = "#ff007f"; ctx.fillRect(0, Y, 1600, 5);
        const int opp = t.currentOpponent();
        if (t.active() && opp >= 0 && t.player >= 0) {
            ctx.font = "900 11px Montserrat"; ctx.textAlign = "left"; ctx.textBaseline = "middle"; ctx.fillStyle = "#ffe600";
            ctx.fillText("PRÓXIMA LUTA · " + upper(Tournament::stageName(t.stage)), 40, Y + 30);
            ctx.flush();
            ui::portraitFace(target, *sprites_, rosterInfo(static_cast<uint32_t>(t.player)).key, {{40, Y + 42}, {80, 76}}, frame_, true);
            ui::portraitFace(target, *sprites_, rosterInfo(static_cast<uint32_t>(opp)).key, {{300, Y + 42}, {80, 76}}, frame_, true,
                         t.stage == 4 ? sf::Color(40, 0, 20) : sf::Color::White);
            ctx.font = "16px 'Titan One'"; ctx.fillStyle = "#ffffff";
            ctx.fillText(upper(lookFor(static_cast<uint32_t>(t.player)).name), 126, Y + 80);
            ctx.font = "30px 'Titan One'"; ctx.textAlign = "center";
            outlined(ctx, "VS", 255, Y + 80, "#ff007f", 2, 3);
            ctx.font = "16px 'Titan One'"; ctx.textAlign = "left"; ctx.fillStyle = t.stage == 4 ? "#ff1e42" : "#ffffff";
            ctx.fillText(t.stage == 4 ? "???" : upper(lookFor(static_cast<uint32_t>(opp)).name), 386, Y + 80);
        } else if (t.champion()) {
            ctx.font = "28px 'Titan One'"; ctx.textAlign = "left"; ctx.textBaseline = "middle";
            outlined(ctx, "VOCÊ É O CAMPEÃO SUPREMO!", 40, Y + 66, "#ffe600", 2, 3);
        }
        static const char* kStageGo[] = {"", "ENTRAR NO RINGUE!", "ENTRAR NO RINGUE!", "ENTRAR NA FINAL!", "ENFRENTAR O BOSS!"};
        const std::string go = t.champion() ? "NOVO TORNEIO" : kStageGo[std::clamp(t.stage, 1, 4)];
        auto skewBtn = [&](sf::FloatRect r, const char* fill, const char* fg, const std::string& label, const char* font, bool sel) {
            ctx.save();
            ctx.translate(r.position.x + r.size.x / 2, r.position.y + r.size.y / 2 + rise);
            ctx.transform(1, 0, -0.16f, 1, 0, 0);
            if (sel) ctx.scale(1.04f, 1.04f);
            notchButton(ctx, -r.size.x / 2, -r.size.y / 2, r.size.x, r.size.y, fill, fg, label, "", sel, font);
            ctx.restore();
        };
        skewBtn(bracketButton(0), "#334155", "#ffffff", "VOLTAR", "15px 'Titan One'", bracketIndex_ == 0);
        skewBtn(bracketButton(1), "#e11d48", "#ffffff", "NOVO TORNEIO", "15px 'Titan One'", bracketIndex_ == 1);
        skewBtn(bracketButton(2), t.stage == 4 ? "#ff007f" : "#00f5d4", t.stage == 4 ? "#ffffff" : "#000000", go, "24px 'Titan One'", bracketIndex_ == 2);
    }
}

// ---------------------------------------------------------------------------
// Modais
// ---------------------------------------------------------------------------

void MenuScreens::drawCampaignPrompt(Canvas& ctx) {
    ctx.fillStyle = "rgba(5, 0, 13, 0.75)";
    ctx.fillRect(0, 0, 1600, 900);
    const float a = springOut(screenFrame_ / 20.0f);
    ctx.save();
    ctx.translate(800, 470); ctx.scale(0.9f + 0.1f * a, 0.9f + 0.1f * a); ctx.translate(-800, -470);
    neonFrame(ctx, 520, 290, 560, 400, 16, sf::Color(255, 225, 105));
    ctx.font = "28px 'Titan One'"; ctx.textAlign = "center"; ctx.textBaseline = "middle";
    outlined(ctx, "TORNEIO EM ANDAMENTO!", 800, 336, "#ffe169", 2.5f, 3);
    const std::string saved = upper(rosterInfo(static_cast<uint32_t>(tournament_.player)).name);
    const std::string current = upper(rosterInfo(playerChar_).name);
    ctx.font = "800 15px 'Outfit ExtraBold'"; ctx.fillStyle = "#e2e8f0";
    wrapText(ctx, "Existe um torneio salvo com " + saved + " em " + Tournament::stageName(tournament_.stage) + ". Continuar com " + saved +
                      " ou começar um novo torneio com " + current + "?",
             800, 384, 480, 21);
    static const char* kFill[] = {"#00f5d4", "#ff007f", "#334155"};
    static const char* kText[] = {"#000000", "#ffffff", "#ffffff"};
    const std::string labels[] = {"CONTINUAR COM " + saved, "NOVO TORNEIO COM " + current, "CANCELAR"};
    for (int i = 0; i < 3; ++i) {
        sf::FloatRect r = promptButton(i);
        notchButton(ctx, r.position.x, r.position.y, r.size.x, r.size.y, kFill[i], kText[i], labels[i], i == 0 ? std::string(Tournament::stageName(tournament_.stage)) : "",
                    promptIndex_ == i, "15px 'Titan One'");
    }
    ctx.restore();
}

namespace {
// Painel de modal: sombra dura, corpo escuro com hachura, faixa preta no topo com a bandeira
// colorida inclinada (mesmo cabeçalho das outras telas).
void modalPanel(Canvas& ctx, float x, float y, float w, float h, const char* accent, const std::string& title, float appear) {
    ctx.fillStyle = "#000000"; ctx.fillRect(x + 12, y + 12, w, h);
    ctx.fillStyle = "#120d20"; ctx.fillRect(x, y, w, h);
    ctx.hatchRect(x, y + 64, w, h - 64, 14, cv::Style("rgba(255,255,255,0.025)"), 3, true);
    ctx.fillStyle = "#05030a"; ctx.fillRect(x, y, w, 64);
    ctx.fillStyle = accent; ctx.fillRect(x, y + 64, w, 5);
    ctx.strokeStyle = "#000000"; ctx.lineWidth = 5; ctx.strokeRect(x, y, w, h);
    ctx.font = "34px 'Titan One'";
    const float bw = std::max(300.0f, ctx.measureText(title) + 90), bx = x + 34 - (1 - appear) * 60;
    ctx.save();
    ctx.globalAlpha *= std::clamp(appear * 1.4f, 0.0f, 1.0f);
    ctx.beginPath(); ctx.moveTo(bx + 22, y - 16); ctx.lineTo(bx + bw + 8, y - 16); ctx.lineTo(bx + bw - 14, y + 48); ctx.lineTo(bx, y + 48); ctx.closePath();
    ctx.fillStyle = "#000000"; ctx.fill();
    ctx.beginPath(); ctx.moveTo(bx + 16, y - 22); ctx.lineTo(bx + bw, y - 22); ctx.lineTo(bx + bw - 22, y + 42); ctx.lineTo(bx - 6, y + 42); ctx.closePath();
    ctx.fillStyle = accent; ctx.fill(); ctx.strokeStyle = "#000000"; ctx.lineWidth = 4; ctx.stroke();
    ctx.textAlign = "left"; ctx.textBaseline = "middle";
    outlined(ctx, title, bx + 30, y + 11, "#ffffff", 2.5f, 4);
    ctx.restore();
}

void keyCap(Canvas& ctx, float x, float cy, const std::string& key, float minW = 34) {
    ctx.font = "11px 'Press Start 2P'";
    const float w = std::max(minW, ctx.measureText(key) + 16);
    ctx.fillStyle = "#000000"; ctx.fillRect(x + 3, cy - 9, w, 26);
    ctx.fillStyle = "#ffe600"; ctx.fillRect(x, cy - 13, w, 26);
    ctx.fillStyle = "rgba(0,0,0,0.18)"; ctx.fillRect(x, cy + 8, w, 5);
    ctx.strokeStyle = "#000000"; ctx.lineWidth = 2.5f; ctx.strokeRect(x, cy - 13, w, 26);
    ctx.fillStyle = "#000000"; ctx.textAlign = "center"; ctx.textBaseline = "middle";
    ctx.fillText(key, x + w / 2, cy);
}

void speakerIcon(Canvas& ctx, float cx, float cy, float s, const char* col) {
    ctx.fillStyle = col;
    ctx.fillRect(cx - s, cy - s * 0.4f, s * 0.7f, s * 0.8f);
    ctx.beginPath(); ctx.moveTo(cx - s * 0.3f, cy - s * 0.4f); ctx.lineTo(cx + s * 0.3f, cy - s); ctx.lineTo(cx + s * 0.3f, cy + s); ctx.lineTo(cx - s * 0.3f, cy + s * 0.4f); ctx.closePath(); ctx.fill();
    ctx.strokeStyle = col; ctx.lineWidth = s * 0.22f;
    for (int i = 0; i < 2; ++i) { ctx.beginPath(); ctx.arc(cx + s * 0.3f, cy, s * (0.7f + i * 0.45f), -0.8f, 0.8f); ctx.stroke(); }
}
} // namespace

void MenuScreens::drawSettings(Canvas& ctx) {
    ctx.fillStyle = "rgba(3, 1, 10, 0.82)";
    ctx.fillRect(0, 0, 1600, 900);
    const float a = springOut(modalFrame_ / 18.0f);
    const float x = 370, y = 120, w = 860, h = 650;
    ctx.save();
    ctx.globalAlpha = std::min(1.0f, a * 1.5f);
    ctx.translate(800, 450); ctx.scale(0.9f + 0.1f * a, 0.9f + 0.1f * a); ctx.translate(-800, -450);
    modalPanel(ctx, x, y, w, h, "#00f5d4", "CONFIGURAÇÕES", a);
    inkBox(ctx, x + w - 58, y + 12, 40, 40, cv::Style("#ff007f"), 3, 3, 4);
    ctx.font = "20px 'Titan One'"; ctx.fillStyle = "#ffffff"; ctx.textAlign = "center"; ctx.textBaseline = "middle";
    ctx.fillText("X", x + w - 38, y + 33);

    // ---------------- ABAS (ÁUDIO / CONTROLES) ----------------
    const sf::FloatRect t0 = settingsTab(0);
    const sf::FloatRect t1 = settingsTab(1);
    notchButton(ctx, t0.position.x, t0.position.y, t0.size.x, t0.size.y,
                settingsTab_ == 0 ? "#00f5d4" : "#1a162b",
                settingsTab_ == 0 ? "#000000" : "#94a3b8",
                "AUDIO", "", settingsTab_ == 0, "18px 'Titan One'");
    notchButton(ctx, t1.position.x, t1.position.y, t1.size.x, t1.size.y,
                settingsTab_ == 1 ? "#ff007f" : "#1a162b",
                settingsTab_ == 1 ? "#ffffff" : "#94a3b8",
                "CONTROLES", "", settingsTab_ == 1, "18px 'Titan One'");

    if (settingsTab_ == 0) {
        // ================= ABA: ÁUDIO =================
        static const char* kLabel[] = {"EFEITOS SONOROS", "MÚSICA"};
        static const char* kCol[] = {"#00f5d4", "#ff007f"};
        static const sf::Color kRgb[] = {sf::Color(0, 245, 212), sf::Color(255, 0, 127)};
        for (int i = 0; i < 2; ++i) {
            const int v = i == 0 ? sfxVolume_ : musicVolume_;
            const bool sel = modalIndex_ == i;
            const sf::FloatRect r = settingsSlider(i);
            const float ly = r.position.y - 24;
            if (sel) {
                ctx.fillStyle = cv::rgba(kRgb[i].r, kRgb[i].g, kRgb[i].b, 0.10f);
                ctx.fillRect(x + 30, ly - 22, w - 60, 98);
                ctx.fillStyle = kCol[i]; ctx.fillRect(x + 30, ly - 22, 6, 98);
            }
            if (i == 0) speakerIcon(ctx, r.position.x + 14, ly, 13, kCol[i]);
            else { ctx.fillStyle = kCol[i]; charIcon(ctx, 5, r.position.x + 14, ly, 15); }
            ctx.font = "22px 'Titan One'"; ctx.textAlign = "left"; ctx.textBaseline = "middle";
            outlined(ctx, kLabel[i], r.position.x + 42, ly, sel ? "#ffffff" : "#cbd5e1", 0, 3);
            ctx.font = "18px 'Press Start 2P'"; ctx.textAlign = "right";
            outlined(ctx, std::to_string(v) + "%", r.position.x + r.size.x, ly, v == 0 ? "#64748b" : "#ffe600", 0, 3);

            const int segs = 20, lit = static_cast<int>(std::round(v / 5.0f));
            const float gap = 5, sw = (r.size.x - gap * (segs - 1)) / segs;
            for (int k = 0; k < segs; ++k) {
                const float sx = r.position.x + k * (sw + gap);
                const float sh = 14 + 22 * k / (segs - 1.0f), sy = r.position.y + r.size.y - sh;
                ctx.beginPath(); ctx.moveTo(sx + 4, sy); ctx.lineTo(sx + sw + 4, sy); ctx.lineTo(sx + sw, sy + sh); ctx.lineTo(sx, sy + sh); ctx.closePath();
                ctx.fillStyle = k < lit ? kCol[i] : "#241c38"; ctx.fill();
                if (k == lit - 1 && sel) { ctx.strokeStyle = "#ffffff"; ctx.lineWidth = 2.5f; ctx.stroke(); }
            }
        }

        // Cartão de dicas de áudio
        const float cy0 = y + 400;
        ctx.fillStyle = "#0c0817"; ctx.fillRect(x + 40, cy0, w - 80, 140);
        ctx.strokeStyle = "#2a2540"; ctx.lineWidth = 2; ctx.strokeRect(x + 40, cy0, w - 80, 140);
        pill(ctx, x + 60, cy0 - 13, "DICAS DE ÁUDIO", "#9d4edd", "#ffffff", "14px 'Titan One'", 12, 26, 2);
        ctx.font = "800 15px 'Outfit ExtraBold'"; ctx.fillStyle = "#e2e8f0"; ctx.textAlign = "left"; ctx.textBaseline = "middle";
        ctx.fillText("• Tecla [M]: Silencia e reativa todo o som a qualquer momento no jogo.", x + 65, cy0 + 35);
        ctx.fillText("• Trilha sonora e efeitos têm volumes separados para máxima clareza nos golpes.", x + 65, cy0 + 70);
        ctx.fillText("• Sons de impacto, esquiva e habilidades usam sintetizador SFML dinâmico.", x + 65, cy0 + 105);
    } else {
        // ================= ABA: CONTROLES =================
        // 1. Presets Rápidos
        ctx.font = "800 14px 'Outfit ExtraBold'"; ctx.fillStyle = "#94a3b8"; ctx.textAlign = "left"; ctx.textBaseline = "middle";
        ctx.fillText("PRESETS:", 410, 268);
        const sf::FloatRect p0 = settingsPreset(0);
        const sf::FloatRect p1 = settingsPreset(1);
        const sf::FloatRect p2 = settingsPreset(2);
        notchButton(ctx, p0.position.x, p0.position.y, p0.size.x, p0.size.y, controlIndex_ == 0 ? "#00f5d4" : "#251d38", controlIndex_ == 0 ? "#000000" : "#00f5d4", "PADRAO (ZXC+SETAS)", "", controlIndex_ == 0, "13px 'Titan One'");
        notchButton(ctx, p1.position.x, p1.position.y, p1.size.x, p1.size.y, controlIndex_ == 1 ? "#ff007f" : "#251d38", controlIndex_ == 1 ? "#ffffff" : "#ff007f", "WASD + JKL", "", controlIndex_ == 1, "13px 'Titan One'");
        notchButton(ctx, p2.position.x, p2.position.y, p2.size.x, p2.size.y, controlIndex_ == 2 ? "#facc15" : "#251d38", controlIndex_ == 2 ? "#000000" : "#facc15", "TECLADO NUM.", "", controlIndex_ == 2, "13px 'Titan One'");

        // 2. Títulos das Colunas da Grade
        ctx.font = "700 12px 'Outfit Bold'"; ctx.fillStyle = "#64748b"; ctx.textAlign = "left";
        ctx.fillText("AÇÃO DE COMBATE", 410, 325);
        ctx.fillText("TECLA 1", 615, 325);
        ctx.fillText("TECLA 2", 715, 325);
        ctx.fillText("AÇÃO DE COMBATE", 810, 325);
        ctx.fillText("TECLA 1", 1015, 325);
        ctx.fillText("TECLA 2", 1115, 325);

        // 3. Grade das 8 Ações
        static const char* kActionCols[] = {
            "#00f5d4", "#ff8500", "#ff007f", "#38bdf8",
            "#34d399", "#a78bfa", "#f43f5e", "#fbbf24"
        };
        for (int act = 0; act < 8; ++act) {
            const ActionKey ak = static_cast<ActionKey>(act);
            const float ly = (act < 4) ? (345.0f + act * 50.0f) : (345.0f + (act - 4) * 50.0f);
            const float lx = (act < 4) ? 410.0f : 810.0f;

            // Fundo da linha
            ctx.fillStyle = "#0c0817";
            ctx.fillRect(lx - 6, ly - 3, 385, 44);
            ctx.strokeStyle = "#1e1830"; ctx.lineWidth = 1.2f;
            ctx.strokeRect(lx - 6, ly - 3, 385, 44);

            // Indicador de cor da ação
            ctx.fillStyle = kActionCols[act];
            ctx.fillRect(lx - 6, ly - 3, 4, 44);

            // Nome da Ação
            ctx.font = "800 14px 'Outfit ExtraBold'";
            ctx.fillStyle = "#f1f5f9"; ctx.textAlign = "left"; ctx.textBaseline = "middle";
            ctx.fillText(KeyBindings::actionLabel(ak), lx + 8, ly + 19);

            // Slots de Teclas (Primária e Secundária)
            for (int sl = 0; sl < 2; ++sl) {
                const sf::FloatRect rk = settingsActionKey(act, sl);
                const sf::Keyboard::Key k = (sl == 0 ? keyBindings_.bindings[act].primary : keyBindings_.bindings[act].secondary);
                const bool isRec = (rebindAction_ == act && rebindSlot_ == sl);
                const int r = (act < 4) ? act : (act - 4);
                const int c = (act < 4) ? sl : (2 + sl);
                const bool isFocused = (controlIndex_ == 3 + r * 4 + c);

                if (isRec) {
                    const float pulse = 0.5f + 0.5f * std::sin(modalFrame_ * 0.35f);
                    ctx.fillStyle = cv::rgba(255, 234, 0, 0.25f + 0.35f * pulse);
                    ctx.fillRect(rk.position.x, rk.position.y, rk.size.x, rk.size.y);
                    ctx.strokeStyle = "#ffea00"; ctx.lineWidth = 3.0f;
                    ctx.strokeRect(rk.position.x, rk.position.y, rk.size.x, rk.size.y);

                    ctx.font = "11px 'Press Start 2P'";
                    ctx.fillStyle = "#ffffff"; ctx.textAlign = "center"; ctx.textBaseline = "middle";
                    ctx.fillText("...", rk.position.x + rk.size.x / 2, rk.position.y + rk.size.y / 2);
                } else {
                    ctx.fillStyle = isFocused ? "#2a1f42" : "#1e1830";
                    ctx.fillRect(rk.position.x, rk.position.y, rk.size.x, rk.size.y);
                    ctx.strokeStyle = isFocused ? "#00f5d4" : (k == sf::Keyboard::Key::Unknown ? "#2c2445" : "#4a3e6f");
                    ctx.lineWidth = isFocused ? 2.5f : 1.5f;
                    ctx.strokeRect(rk.position.x, rk.position.y, rk.size.x, rk.size.y);

                    std::string kname = KeyBindings::keyName(k);
                    ctx.font = kname.size() > 4 ? "700 11px 'Outfit Bold'" : "800 14px 'Outfit ExtraBold'";
                    ctx.fillStyle = isFocused ? "#ffffff" : (k == sf::Keyboard::Key::Unknown ? "#64748b" : "#00f5d4");
                    ctx.textAlign = "center"; ctx.textBaseline = "middle";
                    ctx.fillText(kname, rk.position.x + rk.size.x / 2, rk.position.y + rk.size.y / 2);
                }
            }
        }

        // 4. Botão Restaurar Padrões
        const sf::FloatRect rst = settingsReset();
        notchButton(ctx, rst.position.x, rst.position.y, rst.size.x, rst.size.y, controlIndex_ == 19 ? "#ff007f" : "#3b1122", controlIndex_ == 19 ? "#ffffff" : "#fca5a5", "RESTAURAR PADRÕES", "", controlIndex_ == 19, "13px 'Titan One'");

        // 5. Banner de Status / Dica de Controle
        if (rebindAction_ >= 0) {
            pill(ctx, 675, 560, "PRESSIONE A NOVA TECLA NO TECLADO (ESC CANCELA)", "#ffea00", "#000000", "13px 'Titan One'", 14, 38, 2);
        } else {
            ctx.font = "600 13px 'Outfit SemiBold'"; ctx.fillStyle = "#94a3b8"; ctx.textAlign = "left"; ctx.textBaseline = "middle";
            ctx.fillText("DICA: Clique na tecla ou selecione com as Setas e Enter para redefinir!", 675, 579);
        }
    }

    // Rodapé
    ctx.font = "600 13px 'Outfit SemiBold'"; ctx.fillStyle = "#64748b"; ctx.textAlign = "left"; ctx.textBaseline = "middle";
    ctx.fillText("TAB alterna abas  -  ESC fecha", x + 34, y + h - 35);
    const sf::FloatRect ok = settingsOk();
    const bool okFocus = (settingsTab_ == 0 && modalIndex_ == 2) || (settingsTab_ == 1 && controlIndex_ == 20);
    notchButton(ctx, ok.position.x, ok.position.y, ok.size.x, ok.size.y, okFocus ? "#00f5d4" : "#134e4a", okFocus ? "#000000" : "#2dd4bf", "OK!", "", okFocus, "22px 'Titan One'");
    ctx.restore();
}

void MenuScreens::drawExit(Canvas& ctx) {
    ctx.fillStyle = "rgba(3, 1, 10, 0.80)";
    ctx.fillRect(0, 0, 1600, 900);
    const float a = springOut(modalFrame_ / 18.0f);
    const float x = 480, y = 280, w = 640, h = 330;
    ctx.save();
    ctx.globalAlpha = std::min(1.0f, a * 1.5f);
    ctx.translate(800, 450); ctx.scale(0.9f + 0.1f * a, 0.9f + 0.1f * a); ctx.translate(-800, -450);
    modalPanel(ctx, x, y, w, h, "#ff007f", "SAIR DO JOGO?", a);
    // Luva caída de lado, suando
    const float gx = x + 120, gy = y + 160;
    ctx.save(); ctx.translate(gx, gy); ctx.rotate(-0.35f + 0.05f * std::sin(frame_ * 0.08f));
    ctx.fillStyle = "#000000"; ctx.beginPath(); ctx.arc(6, 6, 46, 0, 2 * kPi); ctx.fill();
    ctx.fillStyle = "#ff1e42"; ctx.beginPath(); ctx.arc(0, 0, 46, 0, 2 * kPi); ctx.fill();
    ctx.strokeStyle = "#000000"; ctx.lineWidth = 4; ctx.stroke();
    ctx.beginPath(); ctx.arc(-38, 8, 18, 0, 2 * kPi); ctx.fill(); ctx.stroke();
    ctx.fillStyle = "#f8fafc"; ctx.fillRect(-28, 36, 56, 22); ctx.strokeRect(-28, 36, 56, 22);
    ctx.fillStyle = "rgba(255,255,255,0.35)"; ctx.beginPath(); ctx.arc(14, -20, 12, 0, 2 * kPi); ctx.fill();
    ctx.restore();
    const float drop = std::fmod(frame_ * 0.6f, 30.0f);
    ctx.globalAlpha *= 1 - drop / 30.0f;
    ctx.fillStyle = "#7dd3fc"; ctx.beginPath(); ctx.moveTo(gx + 56, gy - 62 + drop); ctx.lineTo(gx + 64, gy - 46 + drop); ctx.arc(gx + 56, gy - 46 + drop, 8, 0, kPi); ctx.closePath(); ctx.fill();
    ctx.globalAlpha = std::min(1.0f, a * 1.5f);

    ctx.font = "40px 'Titan One'"; ctx.textAlign = "left"; ctx.textBaseline = "middle";
    outlined(ctx, "JÁ VAI EMBORA?", x + 220, y + 130, "#ffe600", 2.5f, 4);
    ctx.font = "800 17px 'Outfit ExtraBold'"; ctx.fillStyle = "#e2e8f0";
    ctx.fillText("O ringue vai sentir sua falta.", x + 222, y + 176);
    ctx.font = "600 14px 'Outfit SemiBold'"; ctx.fillStyle = "#8c82a0";
    ctx.fillText("Builds e torneio ficam salvos.", x + 222, y + 202);
    static const char* kFill[] = {"#334155", "#ff007f"};
    static const char* kLabel[] = {"CONTINUAR", "SAIR"};
    for (int i = 0; i < 2; ++i) {
        const sf::FloatRect r = exitButton(i);
        notchButton(ctx, r.position.x, r.position.y, r.size.x, r.size.y, kFill[i], "#ffffff", kLabel[i], i == 0 ? "ESC" : "ENTER", modalIndex_ == i, "22px 'Titan One'");
    }
    ctx.restore();
}

} // namespace KoRama
