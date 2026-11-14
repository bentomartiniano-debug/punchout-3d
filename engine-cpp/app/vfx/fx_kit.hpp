#ifndef KORAMA_VFX_FX_KIT_HPP
#define KORAMA_VFX_FX_KIT_HPP

#include "lab_common.hpp"

// Kit de partículas no estilo do Combat Lab (cel-shading com contorno escuro, brilho aditivo e
// física com Y para cima). Os módulos da Valéria, Vivaldi, Lady Espinha, Dr. Marcov e Batalhator
// usam o kit para montar golpes novos, mais trabalhados que os originais do index.html.
namespace KoRama::vfx::kit {

constexpr const char* kInk = "#0f1c10";

enum class Kind {
    Glow,   // ponto de luz aditivo (núcleo branco + cor)
    Spark,  // risco de faísca alinhado à velocidade
    Smoke,  // nuvem cel-shaded de quatro lóbulos
    Drop,   // gota com contorno e brilho
    Shard,  // lasca triangular girando
    Note,   // nota musical
    Star,   // estrela de cinco pontas
    Leaf,   // folha / pétala
    Flake,  // floco de neve de seis braços
    Ring,   // anel de choque voltado para a câmera
    FloorRing, // anel deitado no chão
    Bolt,   // raio elétrico em zigue-zague até (tx, ty, tz)
    Spine,  // espinho de cacto
    Bubble, // bolha translúcida
};

struct P {
    Kind kind = Kind::Glow;
    float x = 0, y = 0, z = 0, vx = 0, vy = 0, vz = 0;
    float gravity = 0, drag = 1.0f;
    float r = 4, growth = 0, maxR = 0;
    float rot = 0, rotSpeed = 0;
    float life = 1.0f, decay = 0.03f;
    sf::Color c0 = sf::Color::White, c1 = sf::Color(250, 204, 21), c2 = sf::Color(234, 88, 12);
    float width = 2.0f;
    float tx = 0, ty = 0, tz = 0; // alvo do raio
    bool floorBounce = false;
    float seed = 0;
};

inline sf::Color col(const char* css) { return cv::parseColor(css); }
inline float rr(float a, float b) { return a + (b - a) * rnd(); }

inline cv::Style alpha(sf::Color c, float a) {
    c.a = static_cast<std::uint8_t>(std::clamp(a, 0.0f, 1.0f) * c.a);
    return cv::Style(c);
}

class Kit {
public:
    std::vector<P> ps;

    P& add(const P& p) {
        ps.push_back(p);
        return ps.back();
    }

    void update(bool paused) {
        if (paused) return;
        for (P& p : ps) {
            p.vx *= p.drag; p.vz *= p.drag;
            if (p.kind != Kind::Ring && p.kind != Kind::FloorRing) p.vy *= p.drag;
            p.vy -= p.gravity;
            p.x += p.vx; p.y += p.vy; p.z += p.vz;
            if (p.y < 0 && p.kind != Kind::FloorRing) {
                if (p.floorBounce && p.vy < -1.0f) { p.y = 0; p.vy *= -0.35f; p.vx *= 0.6f; p.vz *= 0.6f; }
                else if (p.gravity > 0) { p.y = 0; p.vy = 0; p.vx *= 0.7f; p.vz *= 0.7f; }
            }
            if (p.maxR > 0) p.r += (p.maxR - p.r) * 0.2f;
            else p.r += p.growth;
            p.rot += p.rotSpeed;
            p.life -= p.decay;
        }
        ps.erase(std::remove_if(ps.begin(), ps.end(), [](const P& p) { return p.life <= 0 || p.r <= 0; }), ps.end());
    }

    void clear() { ps.clear(); }

    // Anéis de chão (desenhados na camada do tablado).
    void drawFloor(Canvas& ctx, const LabContext& c) const {
        for (const P& p : ps) {
            if (p.kind != Kind::FloorRing) continue;
            Point3 q = c.project3D(p.x, 0, p.z);
            if (!q.ok()) continue;
            const float rx = p.r * q.scale, ry = rx * 0.36f;
            ctx.save();
            ctx.globalAlpha = std::clamp(p.life, 0.0f, 1.0f);
            ctx.beginPath(); ctx.ellipse(q.x, q.y, rx, ry, 0, 0, PI * 2);
            ctx.strokeStyle = kInk; ctx.lineWidth = (p.width + 2.5f) * q.scale * 0.6f; ctx.stroke();
            ctx.strokeStyle = cv::Style(p.c1); ctx.lineWidth = p.width * q.scale * 0.6f; ctx.stroke();
            ctx.beginPath(); ctx.ellipse(q.x, q.y, rx * 0.86f, ry * 0.86f, 0, 0, PI * 2);
            ctx.strokeStyle = cv::Style(p.c0); ctx.lineWidth = std::max(1.0f, p.width * 0.4f * q.scale * 0.6f); ctx.stroke();
            ctx.restore();
        }
    }

    // front: só as partículas mais perto da câmera que o corpo (depth < bodyDepth).
    void draw(Canvas& ctx, const LabContext& c, bool front, float bodyDepth) const {
        const float time = nowMs() / 1000.0f;
        for (const P& p : ps) {
            if (p.kind == Kind::FloorRing) continue;
            Point3 q = c.project3D(p.x, p.y, p.z);
            if (!q.ok() || (q.z < bodyDepth) != front) continue;
            const float a = std::clamp(p.life, 0.0f, 1.0f), s = q.scale, R = std::max(0.6f, p.r * s);
            ctx.save();
            switch (p.kind) {
            case Kind::Glow: {
                ctx.globalCompositeOperation = "lighter";
                auto g = ctx.createRadialGradient(q.x, q.y, 0, q.x, q.y, R * 2.2f);
                sf::Color c0 = p.c0, c1 = p.c1, c2 = p.c1;
                c0.a = static_cast<std::uint8_t>(230 * a); c1.a = static_cast<std::uint8_t>(170 * a); c2.a = 0;
                g->addColorStop(0, c0); g->addColorStop(0.35f, c1); g->addColorStop(1, c2);
                ctx.fillStyle = g;
                ctx.beginPath(); ctx.arc(q.x, q.y, R * 2.2f, 0, PI * 2); ctx.fill();
                break;
            }
            case Kind::Spark: {
                Point3 q2 = c.project3D(p.x - p.vx * 2.2f, p.y - p.vy * 2.2f, p.z - p.vz * 2.2f);
                if (!q2.ok()) break;
                ctx.globalAlpha = a;
                ctx.lineCap = "round";
                ctx.beginPath(); ctx.moveTo(q2.x, q2.y); ctx.lineTo(q.x, q.y);
                ctx.strokeStyle = cv::Style(p.c1); ctx.lineWidth = std::max(1.5f, p.width * 1.8f * s); ctx.stroke();
                ctx.beginPath(); ctx.moveTo(q2.x, q2.y); ctx.lineTo(q.x, q.y);
                ctx.strokeStyle = cv::Style(p.c0); ctx.lineWidth = std::max(0.8f, p.width * 0.8f * s); ctx.stroke();
                break;
            }
            case Kind::Smoke: {
                ctx.globalAlpha = std::min(1.0f, a * 1.3f);
                ctx.translate(q.x, q.y); ctx.rotate(p.rot);
                ctx.beginPath();
                ctx.arc(0, 0, R * 0.8f, 0, PI * 2);
                ctx.arc(-R * 0.4f, -R * 0.3f, R * 0.6f, 0, PI * 2);
                ctx.arc(R * 0.4f, -R * 0.2f, R * 0.65f, 0, PI * 2);
                ctx.arc(R * 0.2f, R * 0.35f, R * 0.55f, 0, PI * 2);
                ctx.fillStyle = cv::Style(p.c1); ctx.strokeStyle = cv::Style(p.c2); ctx.lineWidth = 1.8f * s;
                ctx.fill(); ctx.stroke();
                ctx.fillStyle = cv::Style(p.c0);
                ctx.beginPath(); ctx.arc(-R * 0.1f, -R * 0.2f, R * 0.42f, 0, PI * 2); ctx.fill();
                break;
            }
            case Kind::Drop: {
                ctx.globalAlpha = std::min(1.0f, a * 1.4f);
                ctx.beginPath(); ctx.arc(q.x, q.y, R, 0, PI * 2);
                ctx.fillStyle = cv::Style(p.c1); ctx.strokeStyle = kInk; ctx.lineWidth = std::max(0.8f, 1.2f * s);
                ctx.fill(); ctx.stroke();
                ctx.fillStyle = cv::Style(p.c0);
                ctx.beginPath(); ctx.arc(q.x - R * 0.3f, q.y - R * 0.3f, R * 0.35f, 0, PI * 2); ctx.fill();
                break;
            }
            case Kind::Shard: {
                ctx.globalAlpha = a;
                ctx.translate(q.x, q.y); ctx.rotate(p.rot);
                ctx.beginPath(); ctx.moveTo(-R, -R * 0.7f); ctx.lineTo(R, 0); ctx.lineTo(-R * 0.3f, R);
                ctx.closePath();
                ctx.fillStyle = cv::Style(p.c1); ctx.strokeStyle = kInk; ctx.lineWidth = 1.0f;
                ctx.fill(); ctx.stroke();
                break;
            }
            case Kind::Note: {
                ctx.globalAlpha = a;
                ctx.translate(q.x, q.y); ctx.rotate(std::sin(time * 4 + p.seed) * 0.25f);
                const float h = R * 2.6f;
                ctx.fillStyle = cv::Style(p.c1); ctx.strokeStyle = kInk; ctx.lineWidth = 1.4f * s;
                ctx.beginPath(); ctx.ellipse(0, 0, R, R * 0.72f, -0.4f, 0, PI * 2); ctx.fill(); ctx.stroke();
                ctx.beginPath(); ctx.moveTo(R * 0.85f, -R * 0.2f); ctx.lineTo(R * 0.85f, -h);
                ctx.strokeStyle = kInk; ctx.lineWidth = 2.6f * s; ctx.stroke();
                ctx.strokeStyle = cv::Style(p.c1); ctx.lineWidth = 1.2f * s; ctx.stroke();
                ctx.beginPath(); ctx.moveTo(R * 0.85f, -h); ctx.quadraticCurveTo(R * 2.0f, -h * 0.75f, R * 1.6f, -h * 0.4f);
                ctx.strokeStyle = kInk; ctx.lineWidth = 2.6f * s; ctx.stroke();
                ctx.strokeStyle = cv::Style(p.c1); ctx.lineWidth = 1.2f * s; ctx.stroke();
                ctx.fillStyle = cv::Style(p.c0);
                ctx.beginPath(); ctx.arc(-R * 0.3f, -R * 0.25f, R * 0.25f, 0, PI * 2); ctx.fill();
                break;
            }
            case Kind::Star: {
                ctx.globalAlpha = a;
                ctx.translate(q.x, q.y); ctx.rotate(p.rot);
                ctx.beginPath();
                for (int i = 0; i < 10; i++) {
                    const float ang = i * PI / 5 - PI / 2, rad = i % 2 == 0 ? R : R * 0.45f;
                    if (i == 0) ctx.moveTo(std::cos(ang) * rad, std::sin(ang) * rad);
                    else ctx.lineTo(std::cos(ang) * rad, std::sin(ang) * rad);
                }
                ctx.closePath();
                ctx.fillStyle = cv::Style(p.c1); ctx.strokeStyle = kInk; ctx.lineWidth = 1.2f * s;
                ctx.fill(); ctx.stroke();
                break;
            }
            case Kind::Leaf: {
                ctx.globalAlpha = a;
                ctx.translate(q.x, q.y); ctx.rotate(p.rot);
                ctx.scale(1.0f, 0.55f + 0.45f * std::sin(time * 6 + p.seed)); // folha girando no ar
                ctx.beginPath(); ctx.moveTo(-R, 0); ctx.quadraticCurveTo(0, -R * 0.8f, R, 0); ctx.quadraticCurveTo(0, R * 0.8f, -R, 0);
                ctx.closePath();
                ctx.fillStyle = cv::Style(p.c1); ctx.strokeStyle = kInk; ctx.lineWidth = 1.0f * s;
                ctx.fill(); ctx.stroke();
                ctx.beginPath(); ctx.moveTo(-R * 0.8f, 0); ctx.lineTo(R * 0.8f, 0);
                ctx.strokeStyle = cv::Style(p.c2); ctx.lineWidth = 0.8f * s; ctx.stroke();
                break;
            }
            case Kind::Flake: {
                ctx.globalAlpha = a;
                ctx.translate(q.x, q.y); ctx.rotate(p.rot);
                ctx.lineCap = "round";
                for (int pass = 0; pass < 2; pass++) {
                    ctx.strokeStyle = pass == 0 ? cv::Style(p.c2) : cv::Style(p.c0);
                    ctx.lineWidth = (pass == 0 ? 2.6f : 1.2f) * s;
                    for (int i = 0; i < 6; i++) {
                        const float ang = i * PI / 3, cx = std::cos(ang), sy = std::sin(ang);
                        ctx.beginPath(); ctx.moveTo(0, 0); ctx.lineTo(cx * R, sy * R);
                        ctx.moveTo(cx * R * 0.55f, sy * R * 0.55f); ctx.lineTo(std::cos(ang + 0.6f) * R * 0.8f, std::sin(ang + 0.6f) * R * 0.8f);
                        ctx.moveTo(cx * R * 0.55f, sy * R * 0.55f); ctx.lineTo(std::cos(ang - 0.6f) * R * 0.8f, std::sin(ang - 0.6f) * R * 0.8f);
                        ctx.stroke();
                    }
                }
                break;
            }
            case Kind::Ring: {
                ctx.globalAlpha = a;
                ctx.beginPath(); ctx.arc(q.x, q.y, R, 0, PI * 2);
                ctx.strokeStyle = kInk; ctx.lineWidth = (p.width + 3.0f) * s * a; ctx.stroke();
                ctx.strokeStyle = cv::Style(p.c1); ctx.lineWidth = p.width * s * a; ctx.stroke();
                ctx.strokeStyle = cv::Style(p.c0); ctx.lineWidth = p.width * 0.4f * s * a; ctx.stroke();
                break;
            }
            case Kind::Bolt: {
                Point3 t = c.project3D(p.tx, p.ty, p.tz);
                if (!t.ok()) break;
                ctx.globalAlpha = a;
                ctx.lineCap = "round"; ctx.lineJoin = "round";
                const int segs = 7;
                std::vector<sf::Vector2f> pts;
                const float jitter = 14 * s;
                for (int i = 0; i <= segs; i++) {
                    const float u = static_cast<float>(i) / segs;
                    float off = (i == 0 || i == segs) ? 0 : std::sin(time * 60 + i * 7.3f + p.seed) * jitter;
                    pts.push_back({q.x + (t.x - q.x) * u + off, q.y + (t.y - q.y) * u + off * 0.6f});
                }
                for (int pass = 0; pass < 3; pass++) {
                    ctx.beginPath(); ctx.moveTo(pts[0].x, pts[0].y);
                    for (std::size_t i = 1; i < pts.size(); i++) ctx.lineTo(pts[i].x, pts[i].y);
                    ctx.strokeStyle = pass == 0 ? cv::Style(kInk) : (pass == 1 ? cv::Style(p.c1) : cv::Style(p.c0));
                    ctx.lineWidth = (pass == 0 ? 5.0f : (pass == 1 ? 3.2f : 1.2f)) * std::max(0.6f, s);
                    ctx.stroke();
                }
                break;
            }
            case Kind::Spine: {
                ctx.globalAlpha = a;
                Point3 q2 = c.project3D(p.x - p.vx * 3, p.y - p.vy * 3, p.z - p.vz * 3);
                if (!q2.ok()) break;
                const float dx = q.x - q2.x, dy = q.y - q2.y, len = std::max(1.0f, std::hypot(dx, dy)), nx = -dy / len, ny = dx / len, w = R * 0.35f;
                ctx.beginPath(); ctx.moveTo(q.x, q.y); ctx.lineTo(q2.x + nx * w, q2.y + ny * w); ctx.lineTo(q2.x - nx * w, q2.y - ny * w);
                ctx.closePath();
                ctx.fillStyle = cv::Style(p.c1); ctx.strokeStyle = kInk; ctx.lineWidth = 1.0f;
                ctx.fill(); ctx.stroke();
                break;
            }
            case Kind::Bubble: {
                ctx.globalAlpha = a * 0.9f;
                ctx.beginPath(); ctx.arc(q.x, q.y, R, 0, PI * 2);
                sf::Color f = p.c1; f.a = 90;
                ctx.fillStyle = cv::Style(f); ctx.strokeStyle = cv::Style(p.c2); ctx.lineWidth = 1.4f * s;
                ctx.fill(); ctx.stroke();
                ctx.fillStyle = cv::Style(p.c0);
                ctx.beginPath(); ctx.arc(q.x - R * 0.35f, q.y - R * 0.35f, R * 0.25f, 0, PI * 2); ctx.fill();
                break;
            }
            default: break;
            }
            ctx.restore();
        }
    }
};

// Utilidades de cena comuns aos módulos.
inline float bodyDepthOf(const LabContext& c) {
    Point3 b = c.project3D(c.fighter.body.offsetX, c.fighter.body.offsetY + 60, 0);
    return b.ok() ? b.z : 0.0f;
}

inline float rivalDistOf(const LabContext& c, float fallback = 180.0f) {
    if (!c.rival || !c.entity) return fallback;
    return std::hypot(c.rival->worldPos.x - c.entity->worldPos.x, c.rival->worldPos.z - c.entity->worldPos.z);
}

// Faz uma fase disparar uma única vez por golpe (p cruzou o limiar).
struct Once {
    bool fired = false;
    bool operator()(bool active, float p, float at) {
        if (!active || p < at - 0.001f) { if (!active || p < at - 0.15f) fired = false; return false; }
        if (fired) return false;
        fired = true;
        return true;
    }
};

// Explosão radial pronta: anel, faíscas, brilho e fumaça.
inline void burst(Kit& k, float x, float y, float z, sf::Color hot, sf::Color mid, int sparks, float power = 1.0f) {
    P ring; ring.kind = Kind::Ring; ring.x = x; ring.y = y; ring.z = z; ring.r = 8; ring.maxR = 70 * power; ring.decay = 0.045f;
    ring.c0 = sf::Color::White; ring.c1 = mid; ring.width = 6;
    k.add(ring);
    P glow; glow.kind = Kind::Glow; glow.x = x; glow.y = y; glow.z = z; glow.r = 18 * power; glow.growth = 1.2f; glow.decay = 0.07f; glow.c1 = hot;
    k.add(glow);
    for (int i = 0; i < sparks; i++) {
        const float a = rnd() * PI * 2, e = rr(-0.8f, 0.8f), sp = rr(4, 10) * power;
        P s; s.kind = Kind::Spark; s.x = x; s.y = y; s.z = z;
        s.vx = std::cos(a) * std::cos(e) * sp; s.vy = std::sin(e) * sp + 1.5f; s.vz = std::sin(a) * std::cos(e) * sp;
        s.gravity = 0.18f; s.drag = 0.95f; s.decay = rr(0.035f, 0.06f); s.c0 = sf::Color::White; s.c1 = i % 2 ? hot : mid; s.width = 1.6f;
        k.add(s);
    }
}

} // namespace KoRama::vfx::kit

#endif
