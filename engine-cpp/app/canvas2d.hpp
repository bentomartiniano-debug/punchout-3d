#ifndef KORAMA_CANVAS2D_HPP
#define KORAMA_CANVAS2D_HPP

#include "sprite_atlas.hpp"
#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

// Canvas 2D sobre SFML, com a mesma API do CanvasRenderingContext2D do navegador.
// Os efeitos do jogo original (Combat Lab, ArcadeFX, efeitos de habilidade) foram escritos para o
// canvas; com esta camada eles são transliterados quase linha a linha e mantêm o mesmo visual:
// caminhos com curvas, preenchimento côncavo (stencil), contornos grossos, gradientes, sombras
// suaves, transparência, modos de mistura, texto e imagens.
namespace KoRama::cv {

constexpr float PI = 3.14159265358979f;

float rnd();                       // Math.random()
float now();                       // Date.now() do jogo (ms, relógio de 60 Hz)
void setClock(float milliseconds); // o renderer avança o relógio uma vez por quadro

struct Gradient {
    bool radial = false;
    float x0 = 0, y0 = 0, r0 = 0, x1 = 0, y1 = 0, r1 = 0;
    std::vector<std::pair<float, sf::Color>> stops;
    void addColorStop(float t, std::string_view css);
    void addColorStop(float t, sf::Color c);
    sf::Color at(sf::Vector2f p) const; // cor no ponto (espaço do usuário)
};
using GradientPtr = std::shared_ptr<Gradient>;

// fillStyle / strokeStyle: cor CSS ou gradiente.
struct Style {
    sf::Color color = sf::Color::Black;
    GradientPtr gradient;
    Style() = default;
    Style(const char* css);
    Style(const std::string& css);
    Style(sf::Color c) : color(c) {}
    Style(GradientPtr g) : gradient(std::move(g)) {}
};

sf::Color parseColor(std::string_view css);
Style rgba(float r, float g, float b, float a = 1.0f);
Style rgb(float r, float g, float b);
Style hsla(float h, float s, float l, float a = 1.0f); // h em graus, s/l em %
sf::Color withAlpha(sf::Color c, float a);

// Fontes conhecidas pelo canvas ("bold 20px Bangers" escolhe pelo nome da família).
void registerFont(const std::string& family, const sf::Font* font);

struct Point3 {
    float x = -9999.0f, y = -9999.0f, z = 0.0f, scale = 0.0f;
    bool ok() const { return z > 0.0f && scale > 0.0f; }
};

class Canvas {
public:
    explicit Canvas(sf::RenderTarget& target);
    ~Canvas();
    Canvas(const Canvas&) = delete;
    Canvas& operator=(const Canvas&) = delete;

    // Os desenhos simples (formas sólidas convexas, contornos opacos e imagens) são agrupados e
    // enviados à GPU numa chamada só. Chame flush() antes de desenhar algo direto no alvo por fora
    // do canvas (sprites do SFML, por exemplo) para manter a ordem.
    void flush();
    void enqueue(const sf::Vertex* v, std::size_t n, const sf::BlendMode& blend = sf::BlendAlpha, const sf::Texture* texture = nullptr);

    // Estado (copiado por save/restore, como no navegador)
    Style fillStyle{sf::Color::Black};
    Style strokeStyle{sf::Color::Black};
    float lineWidth = 1.0f;
    float globalAlpha = 1.0f;
    float shadowBlur = 0.0f;
    float shadowOffsetX = 0.0f, shadowOffsetY = 0.0f;
    Style shadowColor{sf::Color::Transparent};
    float miterLimit = 10.0f;
    std::string lineCap = "butt";
    std::string lineJoin = "miter";
    std::string font = "10px sans-serif";
    std::string textAlign = "start";
    std::string textBaseline = "alphabetic";
    std::string globalCompositeOperation = "source-over";
    std::string filter = "none"; // aceito e ignorado (só "brightness(0)" é usado, para silhuetas)

    void save();
    void restore();

    void translate(float x, float y);
    void rotate(float angle);
    void scale(float sx, float sy);
    void transform(float a, float b, float c, float d, float e, float f);
    void setTransform(float a, float b, float c, float d, float e, float f);
    void resetTransform();

    void beginPath();
    void moveTo(float x, float y);
    void lineTo(float x, float y);
    void quadraticCurveTo(float cpx, float cpy, float x, float y);
    void bezierCurveTo(float c1x, float c1y, float c2x, float c2y, float x, float y);
    void arc(float x, float y, float r, float a0, float a1, bool ccw = false);
    void ellipse(float x, float y, float rx, float ry, float rotation, float a0, float a1, bool ccw = false);
    void arcTo(float x1, float y1, float x2, float y2, float r);
    void rect(float x, float y, float w, float h);
    void roundRect(float x, float y, float w, float h, float r);
    void closePath();

    void fill();
    void fillFan(const sf::Vector2f* pts, std::size_t n, sf::Color c);
    void stroke();
    void clip();
    void fillRect(float x, float y, float w, float h);
    void strokeRect(float x, float y, float w, float h);
    void clearRect(float, float, float, float) {}
    // Hachura diagonal (45°) recortada no retângulo, agrupada numa única chamada. Equivale a um
    // rect + clip + várias linhas com stroke, mas sem passar pelo stencil.
    void hatchRect(float x, float y, float w, float h, float spacing, const Style& color, float lineWidth, bool rising);

    void fillText(const std::string& text, float x, float y);
    void strokeText(const std::string& text, float x, float y);
    float measureText(const std::string& text);

    void drawImage(const SpriteFrame& image, float dx, float dy, float dw, float dh);
    void drawImage(const SpriteFrame& image, float dx, float dy);

    GradientPtr createLinearGradient(float x0, float y0, float x1, float y1);
    GradientPtr createRadialGradient(float x0, float y0, float r0, float x1, float y1, float r1);

    sf::RenderTarget& target() { return target_; }
    sf::Color solid(const Style& s, float extraAlpha = 1.0f) const;

private:
    struct State {
        Style fillStyle, strokeStyle, shadowColor;
        float lineWidth, globalAlpha, shadowBlur, shadowOffsetX, shadowOffsetY, miterLimit;
        std::string lineCap, lineJoin, font, textAlign, textBaseline, globalCompositeOperation, filter;
        sf::Transform transform;
        bool clipping;
        std::vector<std::vector<sf::Vector2f>> clipPath;
    };
    struct SubPath {
        std::vector<sf::Vector2f> points; // já transformados (espaço do alvo)
        bool closed = false;
    };

    sf::Vector2f map(float x, float y) const { return transform_.transformPoint({x, y}); }
    SubPath& current();
    void ensureSubPath(sf::Vector2f p);
    sf::BlendMode blend() const;
    void coverPaths(const std::vector<std::vector<sf::Vector2f>>& polys, const Style& style, float extraAlpha, sf::Vector2f offset);
    void drawStrokeGeometry(std::vector<sf::Vertex>& tris, float width, sf::Vector2f offset) const;
    void paintCover(const std::vector<sf::Vertex>& stencilTris, sf::FloatRect bounds, const Style& style, float extraAlpha,
                    bool evenOdd);
    void text(const std::string& str, float x, float y, bool strokeOnly);
    void applyClip(sf::RenderStates& states) const;

    sf::RenderTarget& target_;
    std::vector<sf::Vertex> batch_;
    sf::BlendMode batchBlend_ = sf::BlendAlpha;
    const sf::Texture* batchTexture_ = nullptr;
    sf::Transform transform_;
    std::vector<SubPath> path_;
    std::vector<State> stack_;
    bool clipping_ = false;
    std::vector<std::vector<sf::Vector2f>> clipPath_;
};

} // namespace KoRama::cv

#endif // KORAMA_CANVAS2D_HPP
