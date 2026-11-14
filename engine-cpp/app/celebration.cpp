#include "celebration.hpp"
#include <cmath>

namespace KoRama::celebration {

namespace {

using cv::Canvas;
constexpr const char* kInk = "#0f1c10";
constexpr float kSw = 5.5f;

void poly(Canvas& ctx, std::initializer_list<float> pts, bool close = true) {
    ctx.beginPath();
    auto it = pts.begin();
    ctx.moveTo(it[0], it[1]);
    for (std::size_t i = 2; i + 1 < pts.size(); i += 2) ctx.lineTo(it[i], it[i + 1]);
    if (close) ctx.closePath();
}

void fillStroke(Canvas& ctx, const char* fill, float sw) {
    ctx.fillStyle = fill;
    ctx.fill();
    if (sw > 0) {
        ctx.strokeStyle = kInk;
        ctx.lineWidth = sw;
        ctx.lineJoin = "round";
        ctx.stroke();
    }
}

struct RoseCols { const char *main, *dark, *light; };
const RoseCols kRoses[] = {{"#e63946", "#b71c1c", "#ff6b6b"}, {"#f4be41", "#b3820a", "#ffe285"}, {"#8d212b", "#4a0e13", "#c94451"}};

void drawRose(Canvas& ctx, const RoseCols& c) {
    const char *stemMain = "#2a9d8f", *stemLight = "#48cae4", *stemDark = "#264653";
    ctx.lineCap = "round";
    poly(ctx, {100, 90, 94, 115, 104, 145, 96, 185}, false);
    ctx.strokeStyle = kInk; ctx.lineWidth = kSw * 2.2f; ctx.lineJoin = "round"; ctx.stroke();
    ctx.strokeStyle = stemMain; ctx.lineWidth = kSw * 1.2f; ctx.stroke();
    poly(ctx, {94, 115, 80, 110, 88, 122}); fillStroke(ctx, stemDark, kSw * 0.7f);
    poly(ctx, {102, 145, 116, 140, 108, 152}); fillStroke(ctx, stemDark, kSw * 0.7f);
    poly(ctx, {94, 125, 65, 115, 35, 130, 55, 155, 92, 135}); fillStroke(ctx, stemMain, kSw);
    poly(ctx, {94, 125, 65, 115, 35, 130, 60, 132}); fillStroke(ctx, stemLight, 0);
    poly(ctx, {94, 125, 65, 130, 35, 130}, false); ctx.strokeStyle = kInk; ctx.lineWidth = kSw * 0.7f; ctx.stroke();
    poly(ctx, {102, 148, 135, 138, 165, 152, 145, 175, 100, 158}); fillStroke(ctx, stemMain, kSw);
    poly(ctx, {102, 148, 135, 138, 165, 152, 138, 152}); fillStroke(ctx, stemLight, 0);
    poly(ctx, {102, 148, 135, 152, 165, 152}, false); ctx.strokeStyle = kInk; ctx.lineWidth = kSw * 0.7f; ctx.stroke();
    poly(ctx, {80, 85, 62, 105, 78, 92}); fillStroke(ctx, stemDark, kSw * 0.7f);
    poly(ctx, {120, 85, 138, 105, 122, 92}); fillStroke(ctx, stemDark, kSw * 0.7f);
    poly(ctx, {85, 85, 100, 110, 115, 85}); fillStroke(ctx, stemMain, kSw * 0.9f);
    poly(ctx, {60, 65, 50, 38, 80, 22, 100, 28, 68, 52}); fillStroke(ctx, c.dark, kSw);
    poly(ctx, {140, 65, 150, 38, 120, 22, 100, 28, 132, 52}); fillStroke(ctx, c.dark, kSw);
    poly(ctx, {50, 60, 40, 78, 70, 92, 98, 88, 65, 70}); fillStroke(ctx, c.main, kSw);
    poly(ctx, {50, 60, 70, 92, 65, 70}); fillStroke(ctx, c.light, 0);
    poly(ctx, {150, 60, 160, 78, 130, 92, 102, 88, 135, 70}); fillStroke(ctx, c.main, kSw);
    poly(ctx, {150, 60, 130, 92, 135, 70}); fillStroke(ctx, c.dark, 0);
    poly(ctx, {65, 50, 60, 75, 88, 90, 100, 90, 112, 90, 140, 75, 135, 50, 100, 32}); fillStroke(ctx, c.main, kSw);
    poly(ctx, {65, 50, 100, 32, 100, 90, 60, 75}); fillStroke(ctx, c.light, 0);
    poly(ctx, {135, 50, 100, 32, 100, 90, 140, 75}); fillStroke(ctx, c.dark, 0);
    poly(ctx, {75, 48, 70, 32, 90, 26, 100, 30, 110, 26, 130, 32, 125, 48, 100, 60}); fillStroke(ctx, c.main, kSw * 0.8f);
    poly(ctx, {75, 48, 100, 30, 100, 60}); fillStroke(ctx, c.light, 0);
    poly(ctx, {125, 48, 100, 30, 100, 60}); fillStroke(ctx, c.dark, 0);
    poly(ctx, {85, 42, 100, 34, 115, 42, 100, 52}); fillStroke(ctx, c.dark, kSw * 0.7f);
    poly(ctx, {85, 42, 100, 34, 100, 52}); fillStroke(ctx, c.light, 0);
}

enum class Pattern { Plain, Dots, Stripes, Bows, Hearts };

struct Cloth { const char *main, *dark, *light, *elastic, *accent; Pattern pattern; const char* patternColor; bool boxer; };
const Cloth kCloths[] = {
    {"#ff2a6d", "#ab003c", "#ff70a0", "#05d9e8", "#f4be41", Pattern::Dots, "#ffffff", false},
    {"#05d9e8", "#008ba3", "#6effff", "#ff2a6d", "#f4be41", Pattern::Stripes, "#ffffff", false},
    {"#fcd93b", "#b59700", "#ffea75", "#e65c00", "#ffffff", Pattern::Bows, "#ff2a6d", false},
    {"#7c3aed", "#5018a8", "#a77bf3", "#340c75", "#f4be41", Pattern::Hearts, "#ffffff", false},
    {"#d92b4b", "#99152c", "#f06e85", "#6e0d1e", "#f4be41", Pattern::Dots, "#fcd93b", false},
    {"#238c53", "#11522f", "#53bd82", "#0a361e", "#f4be41", Pattern::Stripes, "#ffffff", false},
    {"#e65c00", "#993d00", "#ff8533", "#22252a", "#f4be41", Pattern::Dots, "#ffffff", true},
    {"#05d9e8", "#008ba3", "#6effff", "#1d3557", "#f4be41", Pattern::Stripes, "#ffffff", true},
    {"#ff2a6d", "#ab003c", "#ff70a0", "#22252a", "#f4be41", Pattern::Hearts, "#ffffff", true},
    {"#2b5cb8", "#173678", "#638ee0", "#122654", "#f4be41", Pattern::Bows, "#ffffff", true},
    {"#334155", "#1e293b", "#64748b", "#0f172a", "#d92b4b", Pattern::Plain, "#ffffff", true},
    {"#d99e2b", "#946610", "#f5cc6e", "#593e09", "#ffffff", Pattern::Dots, "#0f1c10", true},
};

// Contorno das peças em polígono (as curvas Q do SVG achatadas) para testar se a estampa cai dentro.
std::vector<sf::Vector2f> pantiesOutline() {
    std::vector<sf::Vector2f> pts{{34, 58}, {28, 76}};
    auto quad = [&](sf::Vector2f a, sf::Vector2f c, sf::Vector2f b) {
        for (int i = 1; i <= 8; i++) {
            const float t = i / 8.0f, u = 1 - t;
            pts.push_back({u * u * a.x + 2 * u * t * c.x + t * t * b.x, u * u * a.y + 2 * u * t * c.y + t * t * b.y});
        }
    };
    quad({28, 76}, {28, 98}, {62, 124});
    quad({62, 124}, {82, 144}, {100, 152});
    quad({100, 152}, {118, 144}, {138, 124});
    quad({138, 124}, {172, 98}, {172, 76});
    pts.push_back({166, 58});
    return pts;
}

const std::vector<sf::Vector2f> kBoxerOutline{{32, 42}, {168, 42}, {172, 132}, {118, 142}, {100, 102}, {82, 142}, {28, 132}};

bool inside(const std::vector<sf::Vector2f>& poly, float x, float y) {
    bool in = false;
    for (std::size_t i = 0, j = poly.size() - 1; i < poly.size(); j = i++) {
        if ((poly[i].y > y) != (poly[j].y > y) && x < (poly[j].x - poly[i].x) * (y - poly[i].y) / (poly[j].y - poly[i].y) + poly[i].x) in = !in;
    }
    return in;
}

void pathOf(Canvas& ctx, const std::vector<sf::Vector2f>& pts) {
    ctx.beginPath();
    ctx.moveTo(pts[0].x, pts[0].y);
    for (std::size_t i = 1; i < pts.size(); i++) ctx.lineTo(pts[i].x, pts[i].y);
    ctx.closePath();
}

void drawPattern(Canvas& ctx, const Cloth& c, const std::vector<sf::Vector2f>& outline) {
    if (c.pattern == Pattern::Plain) return;
    ctx.fillStyle = c.patternColor;
    if (c.pattern == Pattern::Stripes) {
        // Listras diagonais: segmentos de reta recortados pela silhueta em passos pequenos.
        ctx.strokeStyle = c.patternColor;
        ctx.lineWidth = 6;
        ctx.lineCap = "butt";
        for (float k = -200; k < 400; k += 20) {
            bool open = false;
            ctx.beginPath();
            for (float s = 0; s <= 260; s += 3) {
                const float x = k + s * 0.707f, y = s * 0.707f;
                const bool in = inside(outline, x, y);
                if (in && !open) { ctx.moveTo(x, y); open = true; }
                else if (in) ctx.lineTo(x, y);
                else open = false;
            }
            ctx.stroke();
        }
        return;
    }
    const float step = c.pattern == Pattern::Dots ? 20.0f : 24.0f;
    for (float y = 0; y < 200; y += step) {
        for (float x = 0; x < 200; x += step) {
            if (c.pattern == Pattern::Dots) {
                for (auto [ox, oy] : {std::pair{5.0f, 5.0f}, std::pair{15.0f, 15.0f}}) {
                    if (!inside(outline, x + ox, y + oy)) continue;
                    poly(ctx, {x + ox, y + oy - 3, x + ox + 3, y + oy, x + ox, y + oy + 3, x + ox - 3, y + oy});
                    ctx.fill();
                }
            } else if (!inside(outline, x + 12, y + 12)) {
                continue;
            } else if (c.pattern == Pattern::Bows) {
                poly(ctx, {x + 12, y + 10, x + 6, y + 6, x + 8, y + 12, x + 6, y + 18, x + 12, y + 14, x + 18, y + 18, x + 16, y + 12, x + 18, y + 6});
                ctx.fill();
            } else {
                poly(ctx, {x + 12, y + 17, x + 6, y + 11, x + 6, y + 7, x + 9, y + 5, x + 12, y + 7, x + 15, y + 5, x + 18, y + 7, x + 18, y + 11});
                ctx.fill();
            }
        }
    }
}

void drawPanties(Canvas& ctx, const Cloth& c) {
    static const std::vector<sf::Vector2f> outline = pantiesOutline();
    pathOf(ctx, outline);
    fillStroke(ctx, c.main, kSw);
    drawPattern(ctx, c, outline);
    // Sombra e luz nas metades
    ctx.globalAlpha *= 0.3f;
    ctx.beginPath(); ctx.moveTo(34, 58); ctx.lineTo(28, 76); ctx.quadraticCurveTo(28, 98, 62, 124); ctx.quadraticCurveTo(82, 144, 100, 152); ctx.lineTo(100, 58);
    ctx.closePath(); ctx.fillStyle = c.light; ctx.fill();
    ctx.globalAlpha /= 0.3f;
    ctx.globalAlpha *= 0.4f;
    ctx.beginPath(); ctx.moveTo(166, 58); ctx.lineTo(172, 76); ctx.quadraticCurveTo(172, 98, 138, 124); ctx.quadraticCurveTo(118, 144, 100, 152); ctx.lineTo(100, 58);
    ctx.closePath(); ctx.fillStyle = c.dark; ctx.fill();
    ctx.globalAlpha /= 0.4f;
    pathOf(ctx, outline);
    ctx.strokeStyle = kInk; ctx.lineWidth = kSw; ctx.stroke();
    // Elástico
    ctx.beginPath();
    ctx.moveTo(28, 45); ctx.lineTo(172, 45); ctx.bezierCurveTo(176, 45, 178, 48, 176, 53); ctx.lineTo(166, 62); ctx.lineTo(34, 62); ctx.lineTo(24, 53);
    ctx.bezierCurveTo(22, 48, 24, 45, 28, 45);
    ctx.closePath();
    fillStroke(ctx, c.elastic, kSw);
    ctx.strokeStyle = c.light; ctx.lineWidth = kSw * 0.7f;
    for (float x = 36; x < 164; x += 12) { ctx.beginPath(); ctx.moveTo(x, 53.5f); ctx.lineTo(std::min(164.0f, x + 8), 53.5f); ctx.stroke(); }
    ctx.lineCap = "round";
    ctx.beginPath(); ctx.moveTo(28, 76); ctx.quadraticCurveTo(64, 112, 88, 140); ctx.strokeStyle = kInk; ctx.lineWidth = kSw * 0.8f; ctx.stroke();
    ctx.beginPath(); ctx.moveTo(172, 76); ctx.quadraticCurveTo(136, 112, 112, 140); ctx.stroke();
    // Lacinho
    ctx.save();
    ctx.translate(100, 53.5f);
    poly(ctx, {0, 0, -9, -7, -15, 0, -6, 5}); fillStroke(ctx, c.accent, kSw * 0.6f);
    poly(ctx, {0, 0, 9, -7, 15, 0, 6, 5}); fillStroke(ctx, c.accent, kSw * 0.6f);
    ctx.beginPath(); ctx.roundRect(-3.5f, -3.5f, 7, 7, 1.5f); fillStroke(ctx, c.accent, kSw * 0.6f);
    ctx.restore();
}

void drawBoxer(Canvas& ctx, const Cloth& c) {
    pathOf(ctx, kBoxerOutline);
    fillStroke(ctx, c.main, kSw);
    drawPattern(ctx, c, kBoxerOutline);
    ctx.globalAlpha *= 0.25f;
    poly(ctx, {32, 42, 100, 42, 100, 102, 82, 142, 28, 132}); ctx.fillStyle = c.light; ctx.fill();
    ctx.globalAlpha /= 0.25f;
    ctx.globalAlpha *= 0.35f;
    poly(ctx, {168, 42, 100, 42, 100, 102, 118, 142, 172, 132}); ctx.fillStyle = c.dark; ctx.fill();
    ctx.globalAlpha /= 0.35f;
    pathOf(ctx, kBoxerOutline);
    ctx.strokeStyle = kInk; ctx.lineWidth = kSw; ctx.lineJoin = "round"; ctx.stroke();
    poly(ctx, {32, 42, 168, 42, 168, 58, 32, 58}); fillStroke(ctx, c.elastic, kSw);
    ctx.beginPath(); ctx.moveTo(33, 50); ctx.lineTo(167, 50); ctx.strokeStyle = c.light; ctx.lineWidth = kSw * 0.6f; ctx.stroke();
    ctx.strokeStyle = kInk; ctx.lineWidth = kSw * 0.8f;
    const float seam[] = {78, 58, 80, 95, 100, 102, 120, 95, 122, 58};
    for (int i = 0; i < 4; i++) {
        const float ax = seam[i * 2], ay = seam[i * 2 + 1], bx = seam[i * 2 + 2], by = seam[i * 2 + 3];
        const float len = std::hypot(bx - ax, by - ay);
        for (float d = 0; d < len; d += 8) {
            const float e = std::min(len, d + 5);
            ctx.beginPath(); ctx.moveTo(ax + (bx - ax) * d / len, ay + (by - ay) * d / len); ctx.lineTo(ax + (bx - ax) * e / len, ay + (by - ay) * e / len);
            ctx.stroke();
        }
    }
    ctx.lineWidth = kSw * 0.9f;
    ctx.beginPath(); ctx.moveTo(28, 132); ctx.lineTo(82, 142); ctx.stroke();
    ctx.beginPath(); ctx.moveTo(172, 132); ctx.lineTo(118, 142); ctx.stroke();
    poly(ctx, {88, 38, 112, 38, 112, 54, 88, 54}); fillStroke(ctx, c.accent, kSw * 0.7f);
    ctx.beginPath(); ctx.moveTo(92, 46); ctx.lineTo(108, 46); ctx.strokeStyle = kInk; ctx.lineWidth = kSw * 0.7f; ctx.stroke();
}

} // namespace

void drawThrowable(Canvas& ctx, int kind, float size) {
    ctx.save();
    const float s = size / 200.0f;
    ctx.scale(s, s);
    ctx.translate(-100, -100);
    if (kind < kRoseKinds) drawRose(ctx, kRoses[kind]);
    else {
        const Cloth& c = kCloths[(kind - kRoseKinds) % kUnderwearKinds];
        if (c.boxer) drawBoxer(ctx, c);
        else drawPanties(ctx, c);
    }
    ctx.restore();
}

void drawVictoryCard(Canvas& ctx, const VictoryCard& card, float appear, float time, std::vector<sf::FloatRect>* hits, sf::Vector2f mouse) {
    // Card de quadrinho: palavra gigante sobre uma explosão estrelada, painel inclinado com a fita
    // no topo, título, selos (XP e combo), descrição e botões em placas com a tecla.
    const float t = std::clamp(appear, 0.0f, 1.0f);
    const float ease = 1 + 2.70158f * std::pow(t - 1, 3.0f) + 1.70158f * std::pow(t - 1, 2.0f);
    const char* accent = card.perfect ? "#ffe600" : (card.won ? "#ffe600" : (card.draw ? "#00f5d4" : "#ff1e42"));
    const char* strip = card.perfect ? "#f59e0b" : (card.won ? "#ff007f" : (card.draw ? "#334155" : "#475569"));
    const float w = 470;
    const float h = 286 + card.buttons.size() * 66.0f;
    const float x = 1600 - 110 - w + (1 - ease) * 120, y = 470 - h / 2;

    auto wrap = [&](const std::string& text, float maxW) {
        std::vector<std::string> out;
        std::string cur, word;
        auto flush = [&]() {
            if (word.empty()) return;
            std::string test = cur.empty() ? word : cur + " " + word;
            if (ctx.measureText(test) > maxW && !cur.empty()) { out.push_back(cur); cur = word; }
            else cur = test;
            word.clear();
        };
        for (char ch : text) { if (ch == ' ') flush(); else word += ch; }
        flush();
        if (!cur.empty()) out.push_back(cur);
        return out;
    };
    auto outlined = [&](const std::string& s, float tx, float ty, const char* fill, float stroke, float shadow) {
        if (shadow > 0) { ctx.fillStyle = "#000000"; ctx.fillText(s, tx + shadow, ty + shadow); }
        if (stroke > 0) { ctx.strokeStyle = "#000000"; ctx.lineWidth = stroke * 2; ctx.lineJoin = "round"; ctx.strokeText(s, tx, ty); }
        ctx.fillStyle = fill; ctx.fillText(s, tx, ty);
    };

    ctx.save();
    ctx.globalAlpha = t;
    ctx.translate(x + w / 2, y + h / 2);
    ctx.rotate(-1.2f * cv::PI / 180);
    ctx.scale(0.9f + 0.1f * ease, 0.9f + 0.1f * ease);
    ctx.translate(-w / 2, -h / 2);

    // Painel
    ctx.fillStyle = "#000000"; ctx.fillRect(12, 12, w, h);
    ctx.fillStyle = "#120d20"; ctx.fillRect(0, 0, w, h);
    ctx.hatchRect(0, 0, w, h, 14, cv::Style("rgba(255,255,255,0.03)"), 3, true);
    ctx.fillStyle = strip; ctx.fillRect(0, 0, w, 46);
    ctx.hatchRect(0, 0, w, 46, 10, cv::Style("rgba(0,0,0,0.15)"), 4, false);
    ctx.fillStyle = "#000000"; ctx.fillRect(0, 46, w, 5);
    ctx.strokeStyle = "#000000"; ctx.lineWidth = 5; ctx.strokeRect(0, 0, w, h);
    ctx.font = "19px 'Titan One'"; ctx.textAlign = "right"; ctx.textBaseline = "middle";
    ctx.fillStyle = card.perfect ? "#ffe600" : "#ffffff"; ctx.fillText(card.ribbon, w - 22, 25);

    // Explosão + palavra gigante saltando sobre o canto do painel
    {
        const float pop = std::clamp((appear * 24 - 6) / 12.0f, 0.0f, 1.0f);
        const float sp = 1 + 2.56f * std::pow(pop - 1, 3.0f) + 1.56f * std::pow(pop - 1, 2.0f);
        ctx.save();
        ctx.translate(150, -40);
        ctx.rotate(-7 * cv::PI / 180);
        ctx.scale(sp, sp);
        const int spikes = 14;
        for (int pass = 0; pass < 2; ++pass) {
            ctx.beginPath();
            for (int i = 0; i < spikes * 2; ++i) {
                const float a = i * cv::PI / spikes + time * 0.4f;
                const float r = (i % 2 == 0 ? 120.0f : 78.0f) * (pass == 0 ? 1.0f : 0.92f);
                const float px = std::cos(a) * r * 1.5f + (pass == 0 ? 7 : 0), py = std::sin(a) * r * 0.62f + (pass == 0 ? 7 : 0);
                if (i == 0) ctx.moveTo(px, py); else ctx.lineTo(px, py);
            }
            ctx.closePath();
            ctx.fillStyle = pass == 0 ? "#000000" : accent; ctx.fill();
            if (pass == 1) { ctx.strokeStyle = "#000000"; ctx.lineWidth = 4; ctx.stroke(); }
        }
        ctx.font = card.perfect ? "68px 'Titan One'" : "76px 'Titan One'"; ctx.textAlign = "center"; ctx.textBaseline = "middle";
        const float bob = std::sin(time * 3) * 2;
        for (int i = 6; i >= 1; --i) { ctx.fillStyle = card.perfect ? "#ff007f" : (card.won ? "#ff007f" : "#000000"); ctx.fillText(card.headline, i, bob + i); }
        outlined(card.headline, 0, bob, card.perfect ? "#ffe600" : "#ffffff", 3.5f, 0);
        ctx.restore();
    }

    float cy = 116;
    ctx.textAlign = "left";
    ctx.font = "30px 'Titan One'";
    for (const std::string& ln : wrap(card.title, w - 56)) { outlined(ln, 28, cy, accent, 2.5f, 3); cy += 36; }
    cy += 10;

    // Selos
    auto chip = [&](float cx0, const std::string& text, const char* bg, const char* fg) {
        ctx.font = "12px 'Press Start 2P'";
        const float cw = ctx.measureText(text) + 26;
        if (cx0 + cw > w - 24) return 0.0f; // Salvaguarda: nunca vaza do card
        ctx.fillStyle = "#000000"; ctx.fillRect(cx0 + 4, cy + 4, cw, 32);
        ctx.fillStyle = bg; ctx.fillRect(cx0, cy, cw, 32);
        ctx.strokeStyle = "#000000"; ctx.lineWidth = 3; ctx.strokeRect(cx0, cy, cw, 32);
        ctx.fillStyle = fg; ctx.textAlign = "center"; ctx.fillText(text, cx0 + cw / 2, cy + 17);
        return cw;
    };
    float chipX = 28;
    float wBadge = chip(chipX, card.badge, card.perfect ? "#ffe600" : (card.won ? "#00f5d4" : "#94a3b8"), "#000000");
    if (wBadge > 0) chipX += wBadge + 14;
    if (card.perfect && card.combo == 0) {
        float wLife = chip(chipX, "100% VIDA", "#ff007f", "#ffffff");
        if (wLife > 0) chipX += wLife + 14;
    }
    if (card.combo > 0) {
        chip(chipX, "COMBO x" + std::to_string(card.combo), "#9d4edd", "#ffffff");
    }
    cy += 56;

    ctx.font = "800 16px 'Outfit ExtraBold'"; ctx.fillStyle = "#e2e8f0"; ctx.textAlign = "left";
    for (const std::string& ln : wrap(card.description, w - 56)) { ctx.fillText(ln, 28, cy); cy += 22; }
    cy = std::max(cy + 14, h - card.buttons.size() * 66.0f - 10);

    // Botões em placas inclinadas: tecla à esquerda, ação à direita; o principal pulsa.
    for (std::size_t i = 0; i < card.buttons.size(); i++) {
        const bool primary = i == 0;
        const float bx = 24, bw = w - 48, bh = 52;
        const sf::FloatRect hit({x + bx, y + cy}, {bw, bh});
        if (hits) hits->push_back(hit);
        const bool hover = hit.contains(mouse);
        const float lift = hover ? 4.0f : (primary ? std::max(0.0f, std::sin(time * 4)) * 3 : 0);
        auto plate = [&](float px, float py, float pw) {
            ctx.beginPath(); ctx.moveTo(px, py); ctx.lineTo(px + pw, py); ctx.lineTo(px + pw - 16, py + bh); ctx.lineTo(px, py + bh); ctx.closePath();
        };
        ctx.fillStyle = "#000000"; plate(bx + 6, cy + 6, bw); ctx.fill();
        plate(bx - lift, cy - lift, bw); ctx.fillStyle = primary ? "#00f5d4" : "#334155"; ctx.fill();
        ctx.strokeStyle = primary || hover ? "#ffffff" : "#000000"; ctx.lineWidth = 4; ctx.stroke();
        ctx.fillStyle = "#000000"; ctx.fillRect(bx - lift + 10, cy - lift + 10, 104, bh - 20);
        ctx.font = "12px 'Press Start 2P'"; ctx.textAlign = "center"; ctx.fillStyle = "#ffe600";
        ctx.fillText(card.buttons[i].first, bx - lift + 62, cy - lift + bh / 2 + 1);
        ctx.font = "22px 'Titan One'"; ctx.textAlign = "left";
        ctx.fillStyle = primary ? "#000000" : "#ffffff";
        ctx.fillText(card.buttons[i].second, bx - lift + 132, cy - lift + bh / 2 + 1);
        cy += bh + 14;
    }
    ctx.restore();
}

} // namespace KoRama::celebration
