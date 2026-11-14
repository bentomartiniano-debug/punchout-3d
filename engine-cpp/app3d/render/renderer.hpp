// app3d/render/renderer.hpp
#pragma once
#include "gl/shader.hpp"
#include "model/model.hpp"
#include "rig/rig.hpp"
#include "render/drips.hpp"
#include "render/particles.hpp"
#include "rig/ropes.hpp"
#include <vector>
namespace KoRama {
class Renderer {
public:
    void init();
    void begin(int w, int h, float ringAngle, const Fighter& player, const Fighter& enemy, float pScale, float eScale, float pTop, float eTop, float shake, uint32_t frame, float pullback, const glm::vec3& focus);
    bool project(const glm::vec3& world, int w, int h, glm::vec2& out) const;
    glm::vec3 eyePos() const { return eye_; }   // onde a câmera está (último quadro)
    void setArena(const Model& m, RingRopes& r);   // clima e cordas da arena
    void arena(Model& m, float t);
    void ropes(const RingRopes& r);
    // rastro de quadrinho (smear): quanto cada luva e o corpo andaram no último quadro (mundo); zero = sem rastro
    struct Smear { glm::vec3 glove[2]{}; float gloveR = 0.13f; };
    void fighter(Model& m, const FighterPose& pose, const glm::vec3& lag, const ClothSim& cloth, float t, const Smear* smear = nullptr);
    void shadowPass(Model& arena, Model& a, const FighterPose& pa, const ClothSim& clothA, float tA, Model& b, const FighterPose& pb, const ClothSim& clothB, float tB);   // antes de begin
    void particles(const std::vector<Particle>& list);
    void setFlash(const glm::vec3& at, float strength) { flash_ = glm::vec4(at, strength); }   // clarão do golpe, ilumina quem está perto
    // --bench: tempo de cada etapa na placa de vídeo; quem chama marca o começo de cada uma
    void profOn() { prof_ = true; glGenQueries(kProf, profQ_); }
    void profMark(int i) { if (!prof_) return; if (profOpen_) glEndQuery(GL_TIME_ELAPSED); glBeginQuery(GL_TIME_ELAPSED, profQ_[i]); profOpen_ = true; profUsed_[i] = true; }
    void profCollect() { if (!prof_) return; if (profOpen_) { glEndQuery(GL_TIME_ELAPSED); profOpen_ = false; } for (int i = 0; i < kProf; i++) if (profUsed_[i]) { GLuint64 ns = 0; glGetQueryObjectui64v(profQ_[i], GL_QUERY_RESULT, &ns); profNs_[i] += double(ns); profUsed_[i] = false; } profN_++; }
    double profMs(int i) const { return profN_ ? profNs_[i] / 1e6 / profN_ : 0; }
    static constexpr int kProf = 8;
    long drawCalls = 0, tris = 0;
    // água da onda (render/water_fx.hpp): vértices posição, normal, (u, s, parte); H a altura da crista agora
    void water(const std::vector<float>& verts, const std::vector<unsigned>& idx, float H, float foamy, float t, bool lava = false);   // foamy 0..1: a onda se desmanchando em espuma
    // peças jogadas na comemoração (render/throwables.hpp): vértices posição, normal, cor, uv no atlas (u < 0: sem textura) e contorno; tex o atlas das roupas
    void props(const std::vector<float>& verts, const std::vector<unsigned>& idx, unsigned tex, float alpha = 1.f);   // alpha < 1: translúcida, sem contorno
    unsigned texture(const unsigned char* rgba, int w, int h);
    void glow(const std::vector<float>& verts, const std::vector<unsigned>& idx, bool overlay = false);   // overlay: por cima de tudo (a estrelinha no céu, mesmo com teto)   // luz translúcida somada à cena (mesmo formato das props; a cor já vem com a opacidade)   // textura sRGB com mipmaps (para atlas feitos na hora)
    void effects(Model& m, float t);   // efeitos dos especiais gravados do vinil (render/fx_clips.hpp), depois dos lutadores
    void finish(int w, int h);
    // palco do menu (seleção de lutador): câmera escolhida por fora, fundo transparente (só o lutador, sem céu) e o quadro numa textura com alfa
    void setCamera(bool on, const glm::vec3& eye = glm::vec3(0.f), const glm::vec3& at = glm::vec3(0.f)) { camOn_ = on; camEye_ = eye; camAt_ = at; }
    void setTransparent(bool on) { transparent_ = on; }
    // golpe que derruba: quadro de impacto invertido (0..1) e a câmera avançando para o ponto do golpe (0..1)
    void setImpactFrame(float k) { impact_ = k; }
    void setPunchIn(float k, const glm::vec3& at) { punch_ = k; punchAt_ = at; }
    // onda de choque (centro em uv da tela, progresso 0..1, força) e aberração cromática, como no jogo 2D
    void setShock(const glm::vec2& uv, float progress, float strength) { shock_ = glm::vec4(uv, progress, strength); }
    void setChroma(float k) { chroma_ = k; }
    void setCloud(const glm::vec3& center, float radius, float density, const glm::vec3& sil) { cloud_ = glm::vec4(center, std::max(radius, 0.01f)); cloudK_ = density; cloudSil_ = sil; }   // nuvem que vira silhueta quem está atrás
    // balançada da câmera no golpe forte: deslocamento (metros) e inclinação (radianos) que o laço do jogo amortece
    void setCameraKick(const glm::vec3& offset, float roll) { kick_ = offset; kickRoll_ = roll; }
    // nocaute para a estratosfera: a câmera da luta fica onde está e vira o olhar para o ponto (k 0..1)
    void setSkyLook(const glm::vec3& at, float k) { skyAt_ = at; skyK_ = k; }
    glm::vec3 skyAt_{0.f}; float skyK_ = 0.f;
    void finishToTexture(unsigned tex, int w, int h);   // resolve o quadro (sem bloom nem acabamento) direto na textura GL tex   // brilho e acabamento, na tela; depois disso a HUD
private:
    void apply(Model& m, const FighterPose& p);
    void drawPrim(const Node& n, const Prim& pr, bool hull);
    void drawTree(Model& m, int from = -1);   // from: só a subárvore deste nó (-1: o modelo inteiro)
    void drawDepth(Model& m);
    void ensureTargets(int w, int h);
    Shader toon_, hull_, flat_, sky_, depth_, bright_, blur_, post_, part_, water_, prop_; unsigned waterVao_ = 0, waterVbo_ = 0, waterIbo_ = 0, propVao_ = 0, propVbo_ = 0, propIbo_ = 0;
    glm::mat4 view_{1.f}, proj_{1.f};
    glm::vec3 sun_{0.42f, 0.84f, 0.2f}, eye_{0.f}, tint_{0.9f, 0.84f, 1.1f}, horizon_{0.965f, 0.918f, 0.824f}, zenith_{0.184f, 0.557f, 0.878f}, ropeCol_[RingRopes::kMaxRows]{};
    glm::vec2 fogRange_{40.f, 150.f}; glm::mat4 bend_{1.f}, base_{1.f}; glm::vec3 footShift_[2]{}; const glm::vec3* sect_ = nullptr; float time_ = 0.f;
    GLuint emptyVao_ = 0, cubeVao_ = 0, blobVao_ = 0, shardVao_ = 0, instVbo_ = 0, shFbo_[2]{}, shTex_[2]{}, msFbo_ = 0, msColor_ = 0, msDepth_ = 0, resFbo_ = 0, resTex_ = 0, blurFbo_[2]{}, blurTex_[2]{};
    bool prof_ = false, profOpen_ = false, profUsed_[kProf]{}; GLuint profQ_[kProf]{}; double profNs_[kProf]{}; int profN_ = 0;
    struct DrawItem { const Node* n; const Prim* pr; float dist; }; std::vector<DrawItem> drawList_; std::vector<float> ropeInst_; GLuint ropeVbo_ = 0;
    // o que já está valendo na placa, para não mandar de novo
    struct DrawState { const Node* node = nullptr; glm::mat4 world{0.f}; glm::vec4 color{-1.f}; int cw = 0, pz = -1, surf = -1; float mix = 0.f; int  drip = -1, bend = -1, fade = -1, hasTex = -1, hasBack = -1, vcol = -1, unlit = -1, two = -1; float morph = -1.f, w = -1.f; GLuint tex = 0, tex2 = 0; } st_;
    DripSet dripBody_, dripHead_, dripHand_; glm::vec3 dripK_{0.f}; bool dripReady_ = false, indoor_ = false; float depthMaxY_ = 1e9f;
    GLuint noiseTex_ = 0; glm::vec4 frustum_[6]{}; GLsizei blobVerts_ = 0, shardVerts_ = 0; int scene_ = 0, fbW_ = 0, fbH_ = 0; bool staticDirty_ = true; float rim_ = 0.f, paint_ = 0.f, hatchR_ = 0.f; float pz_ = 0.f, shade_ = 0.f, shadeY_ = 0.f; glm::mat4 pzInv_{1.f}; glm::vec4 cloud_{0.f, 0.f, 0.f, 1.f}; float cloudK_ = 0.f; glm::vec3 cloudSil_{0.2f, 0.13f, 0.04f}; glm::vec4 smA_{0.f}, smB_{0.f}; float smC_ = -1.f, push_ = 0.f;   /* rastro rasgado: ponto da frente e comprimento; direção do rabo e 1/largura de cada língua */ glm::vec4 flash_{0.f}; bool camOn_ = false, transparent_ = false; float impact_ = 0.f, punch_ = 0.f, chroma_ = 0.f; glm::vec4 shock_{0.f, 0.f, 2.f, 0.f}; glm::vec3 kick_{0.f}; float kickRoll_ = 0.f; glm::vec3 punchAt_{0.f}; glm::vec3 camEye_{0.f}, camAt_{0.f}; glm::mat4 lightS_{1.f}, lightD_{1.f};
};
}
