// app3d/model/model.hpp: árvore de peças do .glb, com malhas na GPU.
#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>
namespace KoRama {
struct Prim { glm::vec3 lo{0.f}, hi{0.f}; GLuint vao = 0; GLsizei count = 0; bool indexed = false; glm::vec4 color{1.f}; GLuint tex = 0, tex0 = 0, tex2 = 0; int faceSlot = -1; int pz = 0; GLuint texB = 0; float texMix = 0.f;   /* texB/texMix: uma segunda expressão misturada por cima (fade) */   /* pz: tinta da Pele Venenosa (1 pele com espiral, 2 pele, 3 luva, 4 coroa, 5 pedra, 6 olhos) */ bool unlit = false, vcol = false, twoSided = false, blend = false, morph = false; int surf = 0; glm::vec3 grain{1.f, 0.f, 0.f}; };   // surf: 0 pintado, 1 madeira, 2 pedra, 3 areia, 4 folhagem (relevo do cenário); grain: eixo comprido da peça
struct Node { std::string name; glm::vec3 pos{0.f}, scale{1.f}; glm::quat rot{1, 0, 0, 0}, rot0{1, 0, 0, 0}; bool visible = true; int parent = -1; std::vector<int> kids; std::vector<Prim> prims; float outlineW = 0, fadeA = 0, fadeB = 0; bool fade = false; glm::mat4 world{1.f}; float morph = 0; bool lower = false; int drip = 0; glm::vec4 tint{-1.f}; float ghost = 1.f; };   // ghost: transparência extra (cópias fantasmas do rastro)   // tint: cor que substitui a do material (a < 0: nenhuma); efeitos gravados do vinil   // drip: 1 corpo, 2 cabeça, 3 mão
struct Model {
    std::vector<Node> nodes; int rootNode = -1; std::map<std::string, int> byName;
    std::map<std::string, GLuint> faces;   // "expressão_peça" -> textura
    // peças do rosto (faceSlot) com a malha e a imagem guardadas: para pintar expressões novas e achar no corpo um ponto da textura (a boca)
    struct FaceMesh { int node = -1, slot = -1; std::vector<glm::vec3> pos, nrm; std::vector<glm::vec2> uv; std::vector<uint32_t> idx; };
    struct FaceImg { unsigned w = 0, h = 0; std::vector<std::uint8_t> px; };
    std::vector<FaceMesh> faceMesh; std::map<int, FaceImg> faceImg;
    // nova expressão "name" para a peça slot: copia a textura original e deixa paint pintar por cima (RGBA, w x h)
    void addFace(const std::string& name, int slot, const std::function<void(unsigned w, unsigned h, std::uint8_t* px)>& paint);
    // ponto da malha do rosto mais perto da coordenada de textura uv (0..1, origem em cima): nó, posição e normal locais
    bool uvPoint(int slot, const glm::vec2& uv, int& node, glm::vec3& pos, glm::vec3& nrm) const;
    bool uvPointSmooth(int slot, const glm::vec2& uv, int& node, glm::vec3& pos, glm::vec3& nrm) const;   // média dos vértices mais perto (para algo andar pela pele sem pular)
    std::string meta;                      // extras do root: medidas, clima da arena, cordas
    float num(const char* key, float def = 0) const;
    bool has(const char* text) const { return meta.find(text) != std::string::npos; }
    float WAIST = 0, NECK = 0, HIPS = 0, legLen = 0, scale = 1;
    float poison = 0.f;   // Pele Venenosa ligada (0..1), posta pelo jogo antes de desenhar
    float shade = 0.f, shadeY = 0.f;   // sombra do capuz do Jack (melaço) nas peças de pele acima de shadeY (altura no modelo)
    void load(const std::string& path);   // lança std::runtime_error com o arquivo e o motivo
    int need(const std::string& name) const;   // lança citando a peça que falta
    void setFace(const std::string& face);
    void blendFace(const std::string& face, float k);   // mistura outra expressão por cima da atual (k 0..1; "" ou 0 desliga)
    void setMorphUnder(int node, float amount);   // mistura as peças com forma alternativa (sombrinha) abaixo deste nó
    void updateWorld(const glm::mat4& base);
    float headTop() const;   // topo da cabeça (com chapéu) em metros do mundo, da malha de verdade
};
}
