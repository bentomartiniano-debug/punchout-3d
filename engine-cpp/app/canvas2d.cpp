#include <memory>
#include "canvas2d.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <map>
#include <iostream>
#include <random>
#include <unordered_map>

namespace KoRama::cv {

namespace {

std::mt19937& rng() {
    static std::mt19937 g(20261003u);
    return g;
}

float g_clock = 0.0f;
std::map<std::string, const sf::Font*>& fontRegistry() {
    static std::map<std::string, const sf::Font*> fonts;
    return fonts;
}

std::uint8_t clampByte(float v) {
    return static_cast<std::uint8_t>(std::clamp(std::lround(v), 0L, 255L));
}

float hueToRgb(float p, float q, float t) {
    if (t < 0) t += 1;
    if (t > 1) t -= 1;
    if (t < 1.0f / 6) return p + (q - p) * 6 * t;
    if (t < 1.0f / 2) return q;
    if (t < 2.0f / 3) return p + (q - p) * (2.0f / 3 - t) * 6;
    return p;
}

sf::Color hslToColor(float h, float s, float l, float a) {
    h = std::fmod(std::fmod(h, 360.0f) + 360.0f, 360.0f) / 360.0f;
    s = std::clamp(s / 100.0f, 0.0f, 1.0f);
    l = std::clamp(l / 100.0f, 0.0f, 1.0f);
    float r, g, b;
    if (s == 0) {
        r = g = b = l;
    } else {
        float q = l < 0.5f ? l * (1 + s) : l + s - l * s, p = 2 * l - q;
        r = hueToRgb(p, q, h + 1.0f / 3);
        g = hueToRgb(p, q, h);
        b = hueToRgb(p, q, h - 1.0f / 3);
    }
    return sf::Color(clampByte(r * 255), clampByte(g * 255), clampByte(b * 255), clampByte(a * 255));
}

// Lê os números de "rgba(1, 2, 3, 0.5)" / "hsl(120, 50%, 40%)".
std::vector<float> numbersIn(std::string_view s) {
    std::vector<float> out;
    std::string cur;
    for (char ch : s) {
        if (std::isdigit(static_cast<unsigned char>(ch)) || ch == '.' || ch == '-' || ch == 'e') cur += ch;
        else if (!cur.empty()) { out.push_back(std::strtof(cur.c_str(), nullptr)); cur.clear(); }
    }
    if (!cur.empty()) out.push_back(std::strtof(cur.c_str(), nullptr));
    return out;
}

sf::Color parseColorUncached(std::string_view css) {
    std::string s;
    for (char ch : css) if (!std::isspace(static_cast<unsigned char>(ch))) s += static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    if (s.empty() || s == "transparent" || s == "none") return sf::Color::Transparent;
    if (s[0] == '#') {
        auto hex = [&](std::size_t i, std::size_t n) { return static_cast<std::uint8_t>(std::stoul(s.substr(i, n), nullptr, 16)); };
        try {
            if (s.size() == 4) return sf::Color(hex(1, 1) * 17, hex(2, 1) * 17, hex(3, 1) * 17);
            if (s.size() == 5) return sf::Color(hex(1, 1) * 17, hex(2, 1) * 17, hex(3, 1) * 17, hex(4, 1) * 17);
            if (s.size() == 7) return sf::Color(hex(1, 2), hex(3, 2), hex(5, 2));
            if (s.size() == 9) return sf::Color(hex(1, 2), hex(3, 2), hex(5, 2), hex(7, 2));
        } catch (...) {
        }
        return sf::Color::Magenta;
    }
    if (s.rfind("rgb", 0) == 0) {
        auto n = numbersIn(s);
        if (n.size() >= 3) return sf::Color(clampByte(n[0]), clampByte(n[1]), clampByte(n[2]), clampByte((n.size() > 3 ? n[3] : 1.0f) * 255));
    }
    if (s.rfind("hsl", 0) == 0) {
        auto n = numbersIn(s);
        if (n.size() >= 3) return hslToColor(n[0], n[1], n[2], n.size() > 3 ? n[3] : 1.0f);
    }
    static const std::unordered_map<std::string, sf::Color> kNamed = {
        {"black", sf::Color::Black}, {"white", sf::Color::White}, {"red", sf::Color::Red}, {"green", sf::Color(0, 128, 0)},
        {"blue", sf::Color::Blue}, {"yellow", sf::Color::Yellow}, {"gold", sf::Color(255, 215, 0)}, {"orange", sf::Color(255, 165, 0)},
        {"cyan", sf::Color::Cyan}, {"magenta", sf::Color::Magenta}, {"gray", sf::Color(128, 128, 128)}, {"grey", sf::Color(128, 128, 128)},
        {"pink", sf::Color(255, 192, 203)}, {"purple", sf::Color(128, 0, 128)}, {"lime", sf::Color(0, 255, 0)},
        {"brown", sf::Color(165, 42, 42)}, {"silver", sf::Color(192, 192, 192)}, {"crimson", sf::Color(220, 20, 60)},
        {"darkred", sf::Color(139, 0, 0)}, {"hotpink", sf::Color(255, 105, 180)}, {"skyblue", sf::Color(135, 206, 235)},
    };
    auto it = kNamed.find(s);
    return it != kNamed.end() ? it->second : sf::Color::Magenta;
}

float cross(sf::Vector2f o, sf::Vector2f a, sf::Vector2f b) {
    return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x);
}

bool isConvex(const std::vector<sf::Vector2f>& p) {
    if (p.size() < 4) return true;
    int sign = 0;
    const std::size_t n = p.size();
    for (std::size_t i = 0; i < n; ++i) {
        float c = cross(p[i], p[(i + 1) % n], p[(i + 2) % n]);
        if (std::fabs(c) < 1e-4f) continue;
        int s = c > 0 ? 1 : -1;
        if (sign == 0) sign = s;
        else if (s != sign) return false;
    }
    return true;
}

float transformScale(const sf::Transform& t) {
    const float* m = t.getMatrix();
    float sx = std::hypot(m[0], m[1]), sy = std::hypot(m[4], m[5]);
    return std::sqrt(std::max(1e-6f, sx * sy));
}

void appendFan(std::vector<sf::Vertex>& out, const std::vector<sf::Vector2f>& poly, sf::Color c) {
    if (poly.size() < 3) return;
    for (std::size_t i = 1; i + 1 < poly.size(); ++i) {
        out.push_back({poly[0], c});
        out.push_back({poly[i], c});
        out.push_back({poly[i + 1], c});
    }
}

void appendDisc(std::vector<sf::Vertex>& out, sf::Vector2f c, float r, sf::Color col) {
    const int n = std::clamp(static_cast<int>(r * 1.5f), 6, 24);
    for (int i = 0; i < n; ++i) {
        float a0 = 2 * PI * i / n, a1 = 2 * PI * (i + 1) / n;
        out.push_back({c, col});
        out.push_back({{c.x + std::cos(a0) * r, c.y + std::sin(a0) * r}, col});
        out.push_back({{c.x + std::cos(a1) * r, c.y + std::sin(a1) * r}, col});
    }
}

sf::FloatRect boundsOf(const std::vector<sf::Vertex>& v) {
    float x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f;
    for (const auto& p : v) {
        x0 = std::min(x0, p.position.x); y0 = std::min(y0, p.position.y);
        x1 = std::max(x1, p.position.x); y1 = std::max(y1, p.position.y);
    }
    return {{x0 - 1, y0 - 1}, {x1 - x0 + 2, y1 - y0 + 2}};
}

} // namespace

float rnd() {
    static std::uniform_real_distribution<float> d(0.0f, 1.0f);
    return d(rng());
}

float now() { return g_clock; }
void setClock(float ms) { g_clock = ms; }

sf::Color parseColor(std::string_view css) {
    static std::unordered_map<std::string, sf::Color> cache;
    auto it = cache.find(std::string(css));
    if (it != cache.end()) return it->second;
    sf::Color c = parseColorUncached(css);
    if (cache.size() < 4096) cache.emplace(std::string(css), c);
    return c;
}

Style::Style(const char* css) : color(parseColor(css)) {}
Style::Style(const std::string& css) : color(parseColor(css)) {}

Style rgba(float r, float g, float b, float a) {
    return Style(sf::Color(clampByte(r), clampByte(g), clampByte(b), clampByte(a * 255.0f)));
}
Style rgb(float r, float g, float b) { return rgba(r, g, b, 1.0f); }
Style hsla(float h, float s, float l, float a) { return Style(hslToColor(h, s, l, a)); }

sf::Color withAlpha(sf::Color c, float a) {
    c.a = clampByte(c.a * std::clamp(a, 0.0f, 1.0f));
    return c;
}

void registerFont(const std::string& family, const sf::Font* font) {
    std::string key;
    for (char ch : family) key += static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    fontRegistry()[key] = font;
}

// ---------------------------------------------------------------------------
// Gradientes
// ---------------------------------------------------------------------------

void Gradient::addColorStop(float t, std::string_view css) { addColorStop(t, parseColor(css)); }

void Gradient::addColorStop(float t, sf::Color c) {
    stops.emplace_back(std::clamp(t, 0.0f, 1.0f), c);
    std::stable_sort(stops.begin(), stops.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
}

sf::Color Gradient::at(sf::Vector2f p) const {
    if (stops.empty()) return sf::Color::Transparent;
    float t = 0.0f;
    if (!radial) {
        float dx = x1 - x0, dy = y1 - y0, len2 = dx * dx + dy * dy;
        t = len2 > 0 ? ((p.x - x0) * dx + (p.y - y0) * dy) / len2 : 0.0f;
    } else {
        // Cone entre os dois círculos (especificação do canvas): o maior t com r(t) >= 0.
        float cdx = x1 - x0, cdy = y1 - y0, dr = r1 - r0;
        float pdx = p.x - x0, pdy = p.y - y0;
        float a = cdx * cdx + cdy * cdy - dr * dr;
        float b = pdx * cdx + pdy * cdy + r0 * dr;
        float c = pdx * pdx + pdy * pdy - r0 * r0;
        if (std::fabs(a) < 1e-6f) {
            t = std::fabs(b) > 1e-6f ? c / (2 * b) : 0.0f;
        } else {
            float disc = b * b - a * c;
            if (disc < 0) return sf::Color::Transparent;
            float sq = std::sqrt(disc);
            float t1 = (b + sq) / a, t2 = (b - sq) / a;
            t = (r0 + t1 * dr >= 0) ? std::max(t1, (r0 + t2 * dr >= 0) ? t2 : t1) : t2;
        }
    }
    t = std::clamp(t, 0.0f, 1.0f);
    if (t <= stops.front().first) return stops.front().second;
    if (t >= stops.back().first) return stops.back().second;
    for (std::size_t i = 1; i < stops.size(); ++i) {
        if (t <= stops[i].first) {
            const auto& [ta, ca] = stops[i - 1];
            const auto& [tb, cb] = stops[i];
            float u = tb > ta ? (t - ta) / (tb - ta) : 0.0f;
            auto mix = [u](std::uint8_t a, std::uint8_t b) { return clampByte(a + (b - a) * u); };
            return sf::Color(mix(ca.r, cb.r), mix(ca.g, cb.g), mix(ca.b, cb.b), mix(ca.a, cb.a));
        }
    }
    return stops.back().second;
}

// ---------------------------------------------------------------------------
// Canvas
// ---------------------------------------------------------------------------

Canvas::Canvas(sf::RenderTarget& target) : target_(target) { batch_.reserve(4096); }

Canvas::~Canvas() { flush(); }

void Canvas::flush() {
    if (batch_.empty()) return;
    sf::RenderStates st;
    st.blendMode = batchBlend_;
    st.texture = batchTexture_;
    target_.draw(batch_.data(), batch_.size(), sf::PrimitiveType::Triangles, st);
    batch_.clear();
}

void Canvas::enqueue(const sf::Vertex* v, std::size_t n, const sf::BlendMode& blend, const sf::Texture* texture) {
    if (n == 0) return;
    if (!batch_.empty() && (!(blend == batchBlend_) || texture != batchTexture_)) flush();
    batchBlend_ = blend;
    batchTexture_ = texture;
    batch_.insert(batch_.end(), v, v + n);
}

void Canvas::save() {
    stack_.push_back({fillStyle, strokeStyle, shadowColor, lineWidth, globalAlpha, shadowBlur, shadowOffsetX, shadowOffsetY, miterLimit,
                      lineCap, lineJoin, font, textAlign, textBaseline, globalCompositeOperation, filter, transform_, clipping_, clipPath_});
}

void Canvas::restore() {
    if (stack_.empty()) return;
    const State& s = stack_.back();
    fillStyle = s.fillStyle; strokeStyle = s.strokeStyle; shadowColor = s.shadowColor;
    lineWidth = s.lineWidth; globalAlpha = s.globalAlpha; shadowBlur = s.shadowBlur;
    shadowOffsetX = s.shadowOffsetX; shadowOffsetY = s.shadowOffsetY; miterLimit = s.miterLimit;
    lineCap = s.lineCap; lineJoin = s.lineJoin; font = s.font; textAlign = s.textAlign; textBaseline = s.textBaseline;
    globalCompositeOperation = s.globalCompositeOperation; filter = s.filter;
    transform_ = s.transform;
    clipping_ = s.clipping;
    clipPath_ = s.clipPath;
    stack_.pop_back();
}

void Canvas::translate(float x, float y) { transform_.translate({x, y}); }
void Canvas::rotate(float angle) { transform_.rotate(sf::radians(angle)); }
void Canvas::scale(float sx, float sy) { transform_.scale({sx, sy}); }
void Canvas::transform(float a, float b, float c, float d, float e, float f) {
    transform_.combine(sf::Transform(a, c, e, b, d, f, 0, 0, 1));
}
void Canvas::setTransform(float a, float b, float c, float d, float e, float f) { transform_ = sf::Transform(a, c, e, b, d, f, 0, 0, 1); }
void Canvas::resetTransform() { transform_ = sf::Transform::Identity; }

void Canvas::beginPath() { path_.clear(); }

Canvas::SubPath& Canvas::current() {
    if (path_.empty()) path_.emplace_back();
    return path_.back();
}

void Canvas::ensureSubPath(sf::Vector2f p) {
    if (path_.empty() || path_.back().closed) {
        path_.emplace_back();
        path_.back().points.push_back(p);
    }
}

void Canvas::moveTo(float x, float y) {
    path_.emplace_back();
    path_.back().points.push_back(map(x, y));
}

void Canvas::lineTo(float x, float y) {
    sf::Vector2f p = map(x, y);
    if (path_.empty() || path_.back().closed) { ensureSubPath(p); return; }
    current().points.push_back(p);
}

void Canvas::quadraticCurveTo(float cpx, float cpy, float x, float y) {
    if (path_.empty() || current().points.empty()) { moveTo(cpx, cpy); }
    sf::Vector2f p0 = current().points.back(), c = map(cpx, cpy), p1 = map(x, y);
    float len = std::hypot(c.x - p0.x, c.y - p0.y) + std::hypot(p1.x - c.x, p1.y - c.y);
    int n = std::clamp(static_cast<int>(len / 4.0f), 4, 48);
    for (int i = 1; i <= n; ++i) {
        float t = static_cast<float>(i) / n, u = 1 - t;
        current().points.push_back({u * u * p0.x + 2 * u * t * c.x + t * t * p1.x, u * u * p0.y + 2 * u * t * c.y + t * t * p1.y});
    }
}

void Canvas::bezierCurveTo(float c1x, float c1y, float c2x, float c2y, float x, float y) {
    if (path_.empty() || current().points.empty()) { moveTo(c1x, c1y); }
    sf::Vector2f p0 = current().points.back(), a = map(c1x, c1y), b = map(c2x, c2y), p1 = map(x, y);
    float len = std::hypot(a.x - p0.x, a.y - p0.y) + std::hypot(b.x - a.x, b.y - a.y) + std::hypot(p1.x - b.x, p1.y - b.y);
    int n = std::clamp(static_cast<int>(len / 4.0f), 6, 64);
    for (int i = 1; i <= n; ++i) {
        float t = static_cast<float>(i) / n, u = 1 - t;
        float w0 = u * u * u, w1 = 3 * u * u * t, w2 = 3 * u * t * t, w3 = t * t * t;
        current().points.push_back({w0 * p0.x + w1 * a.x + w2 * b.x + w3 * p1.x, w0 * p0.y + w1 * a.y + w2 * b.y + w3 * p1.y});
    }
}

void Canvas::ellipse(float x, float y, float rx, float ry, float rotation, float a0, float a1, bool ccw) {
    rx = std::fabs(rx);
    ry = std::fabs(ry);
    float sweep;
    const float tau = 2 * PI;
    if (!ccw) {
        if (a1 - a0 >= tau) sweep = tau;
        else { sweep = std::fmod(a1 - a0, tau); if (sweep < 0) sweep += tau; }
    } else {
        if (a0 - a1 >= tau) sweep = -tau;
        else { sweep = -std::fmod(a0 - a1, tau); if (sweep > 0) sweep -= tau; }
    }
    const float scr = transformScale(transform_) * std::max(rx, ry);
    int n = std::clamp(static_cast<int>(std::fabs(sweep) * scr / 3.0f), 6, 160);
    const float cr = std::cos(rotation), sr = std::sin(rotation);
    for (int i = 0; i <= n; ++i) {
        float a = a0 + sweep * i / n;
        float ex = std::cos(a) * rx, ey = std::sin(a) * ry;
        sf::Vector2f p = map(x + ex * cr - ey * sr, y + ex * sr + ey * cr);
        if (i == 0) {
            if (path_.empty() || path_.back().closed || path_.back().points.empty()) {
                path_.emplace_back();
                path_.back().points.push_back(p);
            } else {
                current().points.push_back(p);
            }
        } else {
            current().points.push_back(p);
        }
    }
}

void Canvas::arc(float x, float y, float r, float a0, float a1, bool ccw) { ellipse(x, y, r, r, 0.0f, a0, a1, ccw); }

void Canvas::arcTo(float x1, float y1, float x2, float y2, float r) {
    // Aproximação: canto arredondado entre os dois segmentos.
    if (path_.empty() || current().points.empty()) { moveTo(x1, y1); return; }
    sf::Transform inv = transform_.getInverse();
    sf::Vector2f p0 = inv.transformPoint(current().points.back());
    sf::Vector2f d0{p0.x - x1, p0.y - y1}, d1{x2 - x1, y2 - y1};
    float l0 = std::hypot(d0.x, d0.y), l1 = std::hypot(d1.x, d1.y);
    if (l0 < 1e-3f || l1 < 1e-3f || r <= 0) { lineTo(x1, y1); return; }
    d0 /= l0; d1 /= l1;
    float cosA = d0.x * d1.x + d0.y * d1.y;
    float half = std::acos(std::clamp(cosA, -1.0f, 1.0f)) / 2.0f;
    float dist = r / std::tan(std::max(1e-3f, half));
    sf::Vector2f t0{x1 + d0.x * dist, y1 + d0.y * dist}, t1{x1 + d1.x * dist, y1 + d1.y * dist};
    lineTo(t0.x, t0.y);
    quadraticCurveTo(x1, y1, t1.x, t1.y);
}

void Canvas::rect(float x, float y, float w, float h) {
    moveTo(x, y);
    lineTo(x + w, y);
    lineTo(x + w, y + h);
    lineTo(x, y + h);
    closePath();
}

void Canvas::roundRect(float x, float y, float w, float h, float r) {
    r = std::min({r, std::fabs(w) / 2, std::fabs(h) / 2});
    moveTo(x + r, y);
    lineTo(x + w - r, y);
    arc(x + w - r, y + r, r, -PI / 2, 0);
    lineTo(x + w, y + h - r);
    arc(x + w - r, y + h - r, r, 0, PI / 2);
    lineTo(x + r, y + h);
    arc(x + r, y + h - r, r, PI / 2, PI);
    lineTo(x, y + r);
    arc(x + r, y + r, r, PI, 1.5f * PI);
    closePath();
}

void Canvas::closePath() {
    if (path_.empty()) return;
    SubPath& s = path_.back();
    s.closed = true;
    // O próximo comando começa no primeiro ponto deste caminho (como no canvas).
    if (!s.points.empty()) {
        sf::Vector2f first = s.points.front();
        path_.emplace_back();
        path_.back().points.push_back(first);
    }
}

sf::BlendMode Canvas::blend() const {
    if (globalCompositeOperation == "lighter" || globalCompositeOperation == "screen" || globalCompositeOperation == "color-dodge") {
        return sf::BlendMode(sf::BlendMode::Factor::SrcAlpha, sf::BlendMode::Factor::One, sf::BlendMode::Equation::Add);
    }
    if (globalCompositeOperation == "multiply") return sf::BlendMultiply;
    return sf::BlendAlpha;
}

sf::Color Canvas::solid(const Style& s, float extraAlpha) const {
    return withAlpha(s.color, globalAlpha * extraAlpha);
}

void Canvas::applyClip(sf::RenderStates& states) const {
    (void)states;
}

// Desenha triângulos no stencil e cobre a área marcada com a cor ou o gradiente.
void Canvas::paintCover(const std::vector<sf::Vertex>& stencilTris, sf::FloatRect bounds, const Style& style, float extraAlpha,
                        bool evenOdd) {
    if (stencilTris.empty()) return;
    flush();
    sf::View saved = target_.getView();
    bool scissored = false;
    if (clipping_ && !clipPath_.empty()) {
        float x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f;
        for (const auto& poly : clipPath_) for (auto p : poly) { x0 = std::min(x0, p.x); y0 = std::min(y0, p.y); x1 = std::max(x1, p.x); y1 = std::max(y1, p.y); }
        sf::View v = saved;
        const sf::Vector2f size = saved.getSize(), center = saved.getCenter();
        const float left = center.x - size.x / 2, top = center.y - size.y / 2;
        float sx = std::clamp((x0 - left) / size.x, 0.0f, 1.0f), sy = std::clamp((y0 - top) / size.y, 0.0f, 1.0f);
        float ex = std::clamp((x1 - left) / size.x, 0.0f, 1.0f), ey = std::clamp((y1 - top) / size.y, 0.0f, 1.0f);
        v.setScissor(sf::FloatRect({sx, sy}, {std::max(0.0f, ex - sx), std::max(0.0f, ey - sy)}));
        target_.setView(v);
        scissored = true;
    }

    sf::RenderStates mark;
    mark.stencilMode = sf::StencilMode{sf::StencilComparison::Always,
                                       evenOdd ? sf::StencilUpdateOperation::Invert : sf::StencilUpdateOperation::Replace,
                                       sf::StencilValue{1u}, sf::StencilValue{1u}, true};
    target_.draw(stencilTris.data(), stencilTris.size(), sf::PrimitiveType::Triangles, mark);

    std::vector<sf::Vertex> cover;
    if (!style.gradient) {
        sf::Color c = solid(style, extraAlpha);
        sf::Vector2f a = bounds.position, b = bounds.position + bounds.size;
        cover = {{{a.x, a.y}, c}, {{b.x, a.y}, c}, {{b.x, b.y}, c}, {{a.x, a.y}, c}, {{b.x, b.y}, c}, {{a.x, b.y}, c}};
    } else {
        // Malha sobre a caixa; cada vértice recebe a cor do gradiente no espaço do usuário.
        const sf::Transform inv = transform_.getInverse();
        // Malha proporcional ao tamanho: 2 células em brilhos pequenos, até 28 em telas cheias.
        const float span = std::max(bounds.size.x, bounds.size.y);
        const int n = std::clamp(static_cast<int>(span / 14.0f), 2, 28);
        auto colorAt = [&](sf::Vector2f p) {
            return withAlpha(style.gradient->at(inv.transformPoint(p)), globalAlpha * extraAlpha);
        };
        std::vector<sf::Color> grid((n + 1) * (n + 1));
        std::vector<sf::Vector2f> pos((n + 1) * (n + 1));
        for (int j = 0; j <= n; ++j) {
            for (int i = 0; i <= n; ++i) {
                sf::Vector2f p{bounds.position.x + bounds.size.x * i / n, bounds.position.y + bounds.size.y * j / n};
                pos[j * (n + 1) + i] = p;
                grid[j * (n + 1) + i] = colorAt(p);
            }
        }
        cover.reserve(n * n * 6);
        for (int j = 0; j < n; ++j) {
            for (int i = 0; i < n; ++i) {
                int a = j * (n + 1) + i, b = a + 1, c = a + n + 1, d = c + 1;
                for (int k : {a, b, d, a, d, c}) cover.push_back({pos[k], grid[k]});
            }
        }
    }
    sf::RenderStates paint;
    paint.blendMode = blend();
    paint.stencilMode = sf::StencilMode{sf::StencilComparison::NotEqual, sf::StencilUpdateOperation::Zero, sf::StencilValue{0u},
                                        sf::StencilValue{1u}, false};
    target_.draw(cover.data(), cover.size(), sf::PrimitiveType::Triangles, paint);
    if (scissored) target_.setView(saved);
}

void Canvas::coverPaths(const std::vector<std::vector<sf::Vector2f>>& polys, const Style& style, float extraAlpha, sf::Vector2f offset) {
    std::vector<sf::Vertex> tris;
    for (const auto& poly : polys) {
        if (poly.size() < 3) continue;
        std::vector<sf::Vector2f> moved = poly;
        for (auto& p : moved) p += offset;
        appendFan(tris, moved, sf::Color::White);
    }
    if (tris.empty()) return;
    paintCover(tris, boundsOf(tris), style, extraAlpha, true);
}

void Canvas::fill() {
    std::vector<std::vector<sf::Vector2f>> polys;
    for (const SubPath& s : path_) if (s.points.size() >= 3) polys.push_back(s.points);
    if (polys.empty()) return;

    // Sombra/brilho (shadowBlur): camadas cada vez maiores e mais fracas em volta da forma.
    sf::Color sc = withAlpha(shadowColor.color, globalAlpha);
    if (shadowBlur > 0.5f && sc.a > 0) {
        const float blurPx = shadowBlur * transformScale(transform_) * 0.5f;
        for (int pass = 3; pass >= 1; --pass) {
            std::vector<sf::Vertex> tris;
            const float w = blurPx * pass / 1.5f;
            for (const auto& poly : polys) {
                std::vector<sf::Vector2f> moved = poly;
                for (auto& p : moved) p += sf::Vector2f{shadowOffsetX, shadowOffsetY};
                appendFan(tris, moved, sf::Color::White);
                std::vector<sf::Vertex> ring;
                std::vector<sf::Vector2f> closed = moved;
                closed.push_back(moved.front());
                for (std::size_t i = 0; i + 1 < closed.size(); ++i) {
                    sf::Vector2f a = closed[i], b = closed[i + 1], d = b - a;
                    float len = std::hypot(d.x, d.y);
                    if (len < 1e-4f) continue;
                    sf::Vector2f nrm{-d.y / len * w, d.x / len * w};
                    for (sf::Vector2f p : {a + nrm, b + nrm, b - nrm, a + nrm, b - nrm, a - nrm}) tris.push_back({p, sf::Color::White});
                }
                for (auto p : moved) appendDisc(tris, p, w, sf::Color::White);
            }
            paintCover(tris, boundsOf(tris), Style(sc), 0.22f, false);
        }
    }

    // Caminho único e convexo com cor sólida: leque direto (o caso mais comum: círculos, elipses, retângulos).
    if (polys.size() == 1 && !fillStyle.gradient && isConvex(polys[0]) && !clipping_) {
        std::vector<sf::Vertex> tris;
        appendFan(tris, polys[0], solid(fillStyle));
        enqueue(tris.data(), tris.size(), blend(), nullptr);
        return;
    }
    // Regra "nonzero" do canvas: várias formas convexas com a mesma orientação (lóbulos de nuvem,
    // círculos sobrepostos) se unem; orientações misturadas (anel com furo) e formas côncavas usam par/ímpar.
    bool allConvex = true;
    int orientation = 0;
    bool mixed = false;
    for (const auto& poly : polys) {
        if (!isConvex(poly)) allConvex = false;
        float area = 0;
        for (std::size_t i = 0; i < poly.size(); ++i) {
            const auto& a2 = poly[i];
            const auto& b2 = poly[(i + 1) % poly.size()];
            area += a2.x * b2.y - b2.x * a2.y;
        }
        int o = area >= 0 ? 1 : -1;
        if (orientation == 0) orientation = o;
        else if (o != orientation) mixed = true;
    }
    std::vector<sf::Vertex> tris;
    for (const auto& poly : polys) appendFan(tris, poly, sf::Color::White);
    if (tris.empty()) return;
    paintCover(tris, boundsOf(tris), fillStyle, 1.0f, !(allConvex && !mixed && polys.size() > 1));
}

void Canvas::fillFan(const sf::Vector2f* pts, std::size_t n, sf::Color c) {
    if (n < 3) return;
    if (batchTexture_ != nullptr || !(batchBlend_ == blend())) flush();
    batchBlend_ = blend();
    batchTexture_ = nullptr;
    const sf::Vector2f p0 = transform_.transformPoint(pts[0]);
    for (std::size_t i = 1; i + 1 < n; ++i) {
        batch_.push_back({p0, c});
        batch_.push_back({transform_.transformPoint(pts[i]), c});
        batch_.push_back({transform_.transformPoint(pts[i + 1]), c});
    }
}

void Canvas::drawStrokeGeometry(std::vector<sf::Vertex>& tris, float width, sf::Vector2f offset) const {
    const float hw = width / 2.0f;
    const bool roundJoin = lineJoin == "round", roundCap = lineCap == "round", squareCap = lineCap == "square";
    const sf::Color w = sf::Color::White;
    for (const SubPath& s : path_) {
        if (s.points.size() < 2) continue;
        std::vector<sf::Vector2f> pts;
        for (auto p : s.points) if (pts.empty() || std::hypot(p.x - pts.back().x, p.y - pts.back().y) > 0.01f) pts.push_back(p + offset);
        if (pts.size() < 2) continue;
        const bool closed = s.closed;
        if (closed) pts.push_back(pts.front());
        if (squareCap && !closed) {
            sf::Vector2f d0 = pts[1] - pts[0], d1 = pts.back() - pts[pts.size() - 2];
            float l0 = std::hypot(d0.x, d0.y), l1 = std::hypot(d1.x, d1.y);
            if (l0 > 0) pts.front() -= d0 / l0 * hw;
            if (l1 > 0) pts.back() += d1 / l1 * hw;
        }
        for (std::size_t i = 0; i + 1 < pts.size(); ++i) {
            sf::Vector2f a = pts[i], b = pts[i + 1], d = b - a;
            float len = std::hypot(d.x, d.y);
            if (len < 1e-4f) continue;
            sf::Vector2f n{-d.y / len * hw, d.x / len * hw};
            for (sf::Vector2f p : {a + n, b + n, b - n, a + n, b - n, a - n}) tris.push_back({p, w});
            // Juntas: arredondadas ou um triângulo que tapa a fresta (bisel); curvas finas ficam lisas.
            if (i + 2 < pts.size() || closed) {
                sf::Vector2f c = pts[(i + 2) < pts.size() ? i + 2 : 1];
                sf::Vector2f d2 = c - b;
                float len2 = std::hypot(d2.x, d2.y);
                if (len2 < 1e-4f) continue;
                if (roundJoin || hw > 2.5f) {
                    if (roundJoin) appendDisc(tris, b, hw, w);
                    else {
                        sf::Vector2f n2{-d2.y / len2 * hw, d2.x / len2 * hw};
                        for (sf::Vector2f p : {b, b + n, b + n2, b, b - n, b - n2}) tris.push_back({p, w});
                    }
                }
            }
        }
        if (roundCap && !closed) {
            appendDisc(tris, pts.front(), hw, w);
            appendDisc(tris, pts.back(), hw, w);
        }
    }
}

void Canvas::stroke() {
    const float width = std::max(0.6f, lineWidth * transformScale(transform_));
    sf::Color sc = withAlpha(shadowColor.color, globalAlpha);
    if (shadowBlur > 0.5f && sc.a > 0) {
        const float blurPx = shadowBlur * transformScale(transform_) * 0.5f;
        for (int pass = 3; pass >= 1; --pass) {
            std::vector<sf::Vertex> tris;
            drawStrokeGeometry(tris, width + blurPx * pass / 1.5f * 2.0f, {shadowOffsetX, shadowOffsetY});
            if (!tris.empty()) paintCover(tris, boundsOf(tris), Style(sc), 0.22f, false);
        }
    }
    std::vector<sf::Vertex> tris;
    drawStrokeGeometry(tris, width, {0, 0});
    if (tris.empty()) return;
    sf::Color c = solid(strokeStyle);
    // Segmento reto único (sem juntas que se sobreponham): pode ir direto para o lote mesmo translúcido.
    if (!strokeStyle.gradient && !clipping_ && path_.size() == 1 && path_[0].points.size() == 2 && lineCap != "round") {
        for (auto& v : tris) v.color = c;
        enqueue(tris.data(), tris.size(), blend(), nullptr);
        return;
    }
    if (!strokeStyle.gradient && c.a == 255 && blend() == sf::BlendAlpha && !clipping_) {
        for (auto& v : tris) v.color = c;
        enqueue(tris.data(), tris.size(), sf::BlendAlpha, nullptr);
        return;
    }
    // Contorno translúcido: marca no stencil para não escurecer onde os segmentos se cruzam.
    paintCover(tris, boundsOf(tris), strokeStyle, 1.0f, false);
}

void Canvas::clip() {
    clipping_ = true;
    clipPath_.clear();
    for (const SubPath& s : path_) if (s.points.size() >= 3) clipPath_.push_back(s.points);
}

void Canvas::fillRect(float x, float y, float w, float h) {
    const sf::Color sc = withAlpha(shadowColor.color, globalAlpha);
    if (!fillStyle.gradient && (shadowBlur <= 0.5f || sc.a == 0) && !clipping_) {
        const sf::Vector2f p0 = map(x, y);
        const sf::Vector2f p1 = map(x + w, y);
        const sf::Vector2f p2 = map(x + w, y + h);
        const sf::Vector2f p3 = map(x, y + h);
        const sf::Color c = solid(fillStyle);
        const sf::Vertex v[6] = {{p0, c}, {p1, c}, {p2, c}, {p0, c}, {p2, c}, {p3, c}};
        enqueue(v, 6, blend(), nullptr);
        return;
    }
    std::vector<SubPath> saved = std::move(path_);
    path_.clear();
    rect(x, y, w, h);
    fill();
    path_ = std::move(saved);
}

void Canvas::hatchRect(float x, float y, float w, float h, float spacing, const Style& color, float lw, bool rising) {
    if (w <= 0 || h <= 0 || spacing <= 0) return;
    const sf::Color c = solid(color);
    if (c.a == 0) return;
    std::vector<sf::Vertex> tris;
    tris.reserve(static_cast<std::size_t>((w + h) / spacing + 2) * 6);
    const float hw = lw * 0.5f * 0.7071f; // meia espessura projetada nos eixos (linha a 45°)
    for (float k = -h; k < w + h; k += spacing) {
        // Ponto na reta: (x + k + t*h, rising ? y + h - t*h : y + t*h), t em [0,1], recortado em x.
        float t0 = std::max(0.0f, -k / h), t1 = std::min(1.0f, (w - k) / h);
        if (t1 <= t0) continue;
        auto at = [&](float t) { return sf::Vector2f{x + k + t * h, rising ? y + h - t * h : y + t * h}; };
        const sf::Vector2f a = at(t0), b = at(t1);
        const sf::Vector2f n = rising ? sf::Vector2f{hw, hw} : sf::Vector2f{hw, -hw};
        const sf::Vector2f q[4] = {map(a.x - n.x, a.y - n.y), map(b.x - n.x, b.y - n.y), map(b.x + n.x, b.y + n.y), map(a.x + n.x, a.y + n.y)};
        for (int i : {0, 1, 2, 0, 2, 3}) tris.push_back({q[i], c});
    }
    enqueue(tris.data(), tris.size(), blend(), nullptr);
}

void Canvas::strokeRect(float x, float y, float w, float h) {
    std::vector<SubPath> saved = std::move(path_);
    path_.clear();
    rect(x, y, w, h);
    stroke();
    path_ = std::move(saved);
}

// ---------------------------------------------------------------------------
// Texto e imagens
// ---------------------------------------------------------------------------

namespace {

struct FontSpec {
    const sf::Font* font = nullptr;
    float size = 10.0f;
    bool bold = false, italic = false;
};

FontSpec parseFontUncached(const std::string& spec);

FontSpec parseFont(const std::string& spec) {
    static std::unordered_map<std::string, FontSpec> cache;
    static std::size_t registrySize = 0;
    if (registrySize != fontRegistry().size()) { cache.clear(); registrySize = fontRegistry().size(); }
    if (auto it = cache.find(spec); it != cache.end()) return it->second;
    FontSpec f = parseFontUncached(spec);
    cache[spec] = f;
    return f;
}

FontSpec parseFontUncached(const std::string& spec) {
    FontSpec f;
    std::string lower;
    for (char ch : spec) lower += static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    auto px = lower.find("px");
    if (px != std::string::npos) {
        std::size_t start = px;
        while (start > 0 && (std::isdigit(static_cast<unsigned char>(lower[start - 1])) || lower[start - 1] == '.')) --start;
        f.size = std::strtof(lower.substr(start, px - start).c_str(), nullptr);
    }
    f.bold = lower.find("bold") != std::string::npos || lower.find("900") != std::string::npos || lower.find("800") != std::string::npos;
    f.italic = lower.find("italic") != std::string::npos;
    auto& reg = fontRegistry();
    std::size_t best = 0; // a família mais longa vence ("outfit semibold" antes de "outfit")
    for (const auto& [family, font] : reg) {
        if (family.size() > best && lower.find(family) != std::string::npos) { f.font = font; best = family.size(); }
    }
    if (f.font) return f;
    // Sem família conhecida: Outfit (texto) se houver, senão a primeira registrada.
    auto it = reg.find("outfit");
    f.font = it != reg.end() ? it->second : (reg.empty() ? nullptr : reg.begin()->second);
    return f;
}

} // namespace

float Canvas::measureText(const std::string& str) {
    // Medidas repetem muito (HUD e menus a cada quadro): guarda por fonte + texto.
    static std::unordered_map<std::string, float> cache;
    const std::string key = font + '' + str;
    if (auto it = cache.find(key); it != cache.end()) return it->second;
    FontSpec f = parseFont(font);
    if (!f.font) return str.size() * f.size * 0.6f;
    sf::Text t(*f.font, sf::String::fromUtf8(str.begin(), str.end()), static_cast<unsigned>(std::max(1.0f, f.size)));
    const float w = t.getLocalBounds().size.x + t.getLocalBounds().position.x;
    if (cache.size() > 4096) cache.clear();
    cache[key] = w;
    return w;
}

void Canvas::text(const std::string& str, float x, float y, bool strokeOnly) {
    FontSpec f = parseFont(font);
    if (!f.font) return;
    flush();
    const float sc = transformScale(transform_);
    // Tamanho real arredondado para degraus de ~10% (texto que cresce/encolhe numa animação não
    // obriga a fonte a rasterizar um tamanho novo a cada quadro); o resto da escala vai na transformação.
    const float wanted = std::clamp(f.size * sc, 4.0f, 200.0f);
    const float step = std::round(std::log(wanted) / std::log(1.1f));
    const unsigned px = static_cast<unsigned>(std::clamp(std::round(std::pow(1.1f, step)), 4.0f, 200.0f));
    const float fit = wanted / static_cast<float>(px);
    const float outline = strokeOnly ? std::max(1.0f, std::round(lineWidth * sc * 0.5f / fit * 2.0f) / 2.0f) : 0.0f;
    // Cache de textos montados: a geometria do sf::Text (sobretudo com contorno) é cara e o HUD e os
    // menus repetem os mesmos textos a cada quadro. Só cor, origem e transformação mudam.
    struct Entry { std::unique_ptr<sf::Text> text; sf::FloatRect bounds; uint32_t lastUse = 0; };
    static std::unordered_map<std::string, Entry> cache;
    static uint32_t useClock = 0;
    ++useClock;
    std::string key;
    key.reserve(str.size() + 32);
    key.append(reinterpret_cast<const char*>(&f.font), sizeof(f.font));
    key += static_cast<char>(px & 0xff); key += static_cast<char>(px >> 8);
    key += f.italic ? 'i' : 'n';
    key += std::to_string(static_cast<int>(outline * 4));
    key += '';
    key += str;
    auto it = cache.find(key);
    if (it == cache.end()) {
        if (cache.size() > 2048) {
            // Descarta os menos usados recentemente.
            for (auto e = cache.begin(); e != cache.end();) e = (useClock - e->second.lastUse > 600) ? cache.erase(e) : std::next(e);
            if (cache.size() > 2048) cache.clear();
        }
        Entry e;
        e.text = std::make_unique<sf::Text>(*f.font, sf::String::fromUtf8(str.begin(), str.end()), px);
        if (f.italic) e.text->setStyle(sf::Text::Italic);
        if (strokeOnly) {
            e.text->setFillColor(sf::Color::Transparent);
            e.text->setOutlineThickness(outline);
        }
        e.bounds = e.text->getLocalBounds();
        it = cache.emplace(std::move(key), std::move(e)).first;
    }
    Entry& entry = it->second;
    entry.lastUse = useClock;
    sf::Text& t = *entry.text;
    const sf::FloatRect b = entry.bounds;
    float ox = 0;
    if (textAlign == "center") ox = b.position.x + b.size.x / 2;
    else if (textAlign == "right" || textAlign == "end") ox = b.position.x + b.size.x;
    float oy = px * 0.8f; // alphabetic
    if (textBaseline == "middle") oy = px * 0.55f;
    else if (textBaseline == "top" || textBaseline == "hanging") oy = px * 0.12f;
    else if (textBaseline == "bottom") oy = px * 1.0f;
    t.setOrigin({ox, oy});
    sf::Transform tf = transform_;
    tf.translate({x, y});
    tf.scale({fit / sc, fit / sc});
    if (strokeOnly) t.setOutlineColor(solid(strokeStyle));
    else t.setFillColor(solid(fillStyle));
    sf::RenderStates st;
    st.transform = tf;
    st.blendMode = blend();
    target_.draw(t, st);
}

void Canvas::fillText(const std::string& str, float x, float y) { text(str, x, y, false); }
void Canvas::strokeText(const std::string& str, float x, float y) { text(str, x, y, true); }

void Canvas::drawImage(const SpriteFrame& image, float dx, float dy, float dw, float dh) {
    if (!image.texture) return;
    sf::Color c = withAlpha(sf::Color::White, globalAlpha);
    if (filter.find("brightness(0)") != std::string::npos) c = withAlpha(sf::Color::Black, globalAlpha);
    sf::FloatRect r(sf::Vector2f(image.rect.position), sf::Vector2f(image.rect.size));
    sf::Vector2f t0 = r.position, t1 = {r.position.x + r.size.x, r.position.y}, t2 = r.position + r.size, t3 = {r.position.x, r.position.y + r.size.y};
    sf::Vector2f p0 = map(dx, dy), p1 = map(dx + dw, dy), p2 = map(dx + dw, dy + dh), p3 = map(dx, dy + dh);
    std::array<sf::Vertex, 6> v = {{{p0, c, t0}, {p1, c, t1}, {p2, c, t2}, {p0, c, t0}, {p2, c, t2}, {p3, c, t3}}};
    // A textura usa coordenadas em pixels (como sf::Sprite): converte para o modo normalizado do lote.
    enqueue(v.data(), v.size(), blend(), image.texture);
}

void Canvas::drawImage(const SpriteFrame& image, float dx, float dy) {
    drawImage(image, dx, dy, static_cast<float>(image.rect.size.x), static_cast<float>(image.rect.size.y));
}

GradientPtr Canvas::createLinearGradient(float x0, float y0, float x1, float y1) {
    auto g = std::make_shared<Gradient>();
    g->x0 = x0; g->y0 = y0; g->x1 = x1; g->y1 = y1;
    return g;
}

GradientPtr Canvas::createRadialGradient(float x0, float y0, float r0, float x1, float y1, float r1) {
    auto g = std::make_shared<Gradient>();
    g->radial = true;
    g->x0 = x0; g->y0 = y0; g->r0 = r0; g->x1 = x1; g->y1 = y1; g->r1 = r1;
    return g;
}

} // namespace KoRama::cv
