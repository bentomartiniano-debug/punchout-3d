#include <algorithm>
#include <array>
#include <cmath>
// app3d/model/model.cpp
#define CGLTF_IMPLEMENTATION
#include "model/model.hpp"
#include <SFML/Graphics/Image.hpp>
#include <cgltf.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstdlib>
#include <stdexcept>
namespace KoRama {
static std::string extras(const cgltf_data* d, const cgltf_extras& e) {
    if (!e.start_offset) return {};
    return std::string(d->json + e.start_offset, e.end_offset - e.start_offset);
}
static float jsonNum(const std::string& s, const char* key, float def = 0) {
    const auto p = s.find(std::string("\"") + key + "\""); if (p == std::string::npos) return def;
    const auto c = s.find(':', p); if (c == std::string::npos) return def;
    const char* b = s.c_str() + c + 1; if (*b == 'n') return def; return std::strtof(b, nullptr);
}
// folhas pintadas de frente e de costas (projeção): as faces laterais do modelo ficam espremidas na beirada da pintura e liam o fundo
// branco e a franja do contorno, e aparecia uma listra clara vertical dos lados. Os texels usados pelas faces viradas para a frente ou
// para trás (prot) ficam como estão; o resto do fundo (ligado à borda da folha, com tolerância para levar a franja, mais 2 pixels de beirada
// fora das faces) recebe a cor das partes vizinhas, espalhada até cobrir tudo (a leitura desfocada das laterais pega níveis bem borrados).
static void padSheet(sf::Image& im, const std::vector<std::uint8_t>& prot) {
    const unsigned w = im.getSize().x, h = im.getSize().y; if (w < 8 || h < 8) return;
    std::uint8_t* px = const_cast<std::uint8_t*>(im.getPixelsPtr()); const size_t N = size_t(w) * h;
    const auto at = [&](size_t i) { return px + i * 4; };
    const std::uint8_t* c0 = at(0); const int bg[4] = {c0[0], c0[1], c0[2], c0[3]};
    for (size_t i : {size_t(w - 1), size_t(h - 1) * w, N - 1}) { const std::uint8_t* c = at(i); for (int k = 0; k < 4; k++) if (std::abs(int(c[k]) - bg[k]) > 2) return; }   // cantos diferentes: não é folha com fundo
    const bool clear = bg[3] < 8;   // fundo transparente: só a cor é espalhada, o alfa fica
    const auto far = [&](size_t i) { const std::uint8_t* c = at(i); if (clear) return 255 - int(c[3]); const int dr = c[0] - bg[0], dg = c[1] - bg[1], db = c[2] - bg[2]; return int(std::sqrt(float(dr * dr + dg * dg + db * db))); };   // distância ao fundo (menor = mais fundo)
    std::vector<std::uint8_t> mark(N); std::vector<size_t> q; q.reserve(N / 4);
    const auto flood = [&](int tol) { std::fill(mark.begin(), mark.end(), 0); q.clear();
        const auto seed = [&](size_t i) { if (!mark[i] && !prot[i] && (clear ? 255 - far(i) : far(i)) <= (clear ? 255 - tol : tol)) { mark[i] = 1; q.push_back(i); } };
        for (unsigned x = 0; x < w; x++) { seed(x); seed(size_t(h - 1) * w + x); } for (unsigned y = 0; y < h; y++) { seed(size_t(y) * w); seed(size_t(y) * w + w - 1); }
        for (size_t k = 0; k < q.size(); k++) { const size_t i = q[k]; const unsigned x = unsigned(i % w), y = unsigned(i / w);
            if (x > 0) seed(i - 1); if (x + 1 < w) seed(i + 1); if (y > 0) seed(i - w); if (y + 1 < h) seed(i + w); }
        return q.size(); };
    // transparente: fundo = alfa até 200; opaco: cor a até 110 do fundo (leva a franja junto)
    if (clear) flood(255 - 200); else flood(110);
    // a beirada de 2 pixels das partes (o antialiasing com o fundo, mais clara) também vira fundo: senão era ela que se espalhava
    for (int pass = 0; pass < 2; pass++) { for (size_t i = 0; i < N; i++) if (!mark[i] && !prot[i]) { const unsigned x = unsigned(i % w), y = unsigned(i / w);
            if ((x > 0 && mark[i - 1] == 1) || (x + 1 < w && mark[i + 1] == 1) || (y > 0 && mark[i - w] == 1) || (y + 1 < h && mark[i + w] == 1)) mark[i] = 4; }
        for (std::uint8_t& m : mark) if (m == 4) m = 1; }
    // camadas de 1 pixel a partir das partes, até cobrir o fundo: cada pixel de fundo vizinho de uma parte (ou de uma camada já pintada) leva a média deles
    std::vector<size_t> front; for (size_t i = 0; i < N; i++) if (mark[i] == 1) { const unsigned x = unsigned(i % w), y = unsigned(i / w);
        if ((x > 0 && !mark[i - 1]) || (x + 1 < w && !mark[i + 1]) || (y > 0 && !mark[i - w]) || (y + 1 < h && !mark[i + w])) front.push_back(i); }
    std::vector<std::array<int, 4>> col; std::vector<size_t> next;
    while (!front.empty()) { col.assign(front.size(), {-1, 0, 0, 0});
        for (size_t f = 0; f < front.size(); f++) { const unsigned x = unsigned(front[f] % w), y = unsigned(front[f] / w); int sum[4] = {0, 0, 0, 0}, n = 0;
            for (int dy = -1; dy <= 1; dy++) for (int dx = -1; dx <= 1; dx++) { const int xx = int(x) + dx, yy = int(y) + dy; if (xx < 0 || yy < 0 || xx >= int(w) || yy >= int(h)) continue; const size_t j = size_t(yy) * w + size_t(xx); if (mark[j] == 1) continue;
                const std::uint8_t* c = at(j); for (int k = 0; k < 4; k++) sum[k] += c[k]; n++; }
            if (n) col[f] = {sum[0] / n, sum[1] / n, sum[2] / n, sum[3] / n}; }
        next.clear();
        for (size_t f = 0; f < front.size(); f++) { if (col[f][0] < 0) continue; std::uint8_t* c = at(front[f]); c[0] = std::uint8_t(col[f][0]); c[1] = std::uint8_t(col[f][1]); c[2] = std::uint8_t(col[f][2]); if (!clear) c[3] = std::uint8_t(col[f][3]); mark[front[f]] = 2; }
        for (size_t f = 0; f < front.size(); f++) { if (col[f][0] < 0) continue; const size_t i = front[f]; const unsigned x = unsigned(i % w), y = unsigned(i / w);
            const auto add = [&](size_t j) { if (mark[j] == 1) { mark[j] = 3; next.push_back(j); } }; if (x > 0) add(i - 1); if (x + 1 < w) add(i + 1); if (y > 0) add(i - w); if (y + 1 < h) add(i + w); }
        for (size_t j : next) mark[j] = 1; front.swap(next); }
}
// textura de um material: o nome do GL sai na hora e a imagem sobe no fim do load, quando já se sabe quais texels as faces de frente e de
// costas das peças projetadas usam (padSheet). O wrap segue o sampler do glTF (as folhas pedem CLAMP_TO_EDGE).
struct PendingTex { const cgltf_texture* tx = nullptr; GLuint id = 0; bool proj = false; std::vector<glm::vec2> tris; };
static void uploadTex(const PendingTex& pt) {
    const cgltf_image* img = pt.tx->image; if (!img || !img->buffer_view) return;
    sf::Image im; const auto* bytes = cgltf_buffer_view_data(img->buffer_view);
    if (!im.loadFromMemory(bytes, img->buffer_view->size)) throw std::runtime_error("textura inválida");
    if (pt.proj) { const unsigned w = im.getSize().x, h = im.getSize().y; std::vector<std::uint8_t> prot(size_t(w) * h, 0);
        for (size_t t = 0; t + 2 < pt.tris.size(); t += 3) { glm::vec2 a = pt.tris[t] * glm::vec2(w, h), b = pt.tris[t + 1] * glm::vec2(w, h), c = pt.tris[t + 2] * glm::vec2(w, h);
            const float area = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x); if (std::fabs(area) < 1e-6f) continue;
            const int x0 = std::max(0, int(std::floor(std::min({a.x, b.x, c.x})))), x1 = std::min(int(w) - 1, int(std::ceil(std::max({a.x, b.x, c.x})))), y0 = std::max(0, int(std::floor(std::min({a.y, b.y, c.y})))), y1 = std::min(int(h) - 1, int(std::ceil(std::max({a.y, b.y, c.y}))));
            for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) { const glm::vec2 p(float(x) + 0.5f, float(y) + 0.5f);
                const float e0 = ((b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x)) / area, e1 = ((c.x - b.x) * (p.y - b.y) - (c.y - b.y) * (p.x - b.x)) / area, e2 = ((a.x - c.x) * (p.y - c.y) - (a.y - c.y) * (p.x - c.x)) / area;
                if (e0 >= 0.f && e1 >= 0.f && e2 >= 0.f) prot[size_t(y) * w + size_t(x)] = 1; } }
        padSheet(im, prot); }
    glBindTexture(GL_TEXTURE_2D, pt.id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, im.getSize().x, im.getSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, im.getPixelsPtr());
    glGenerateMipmap(GL_TEXTURE_2D); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    const GLint ws = pt.tx->sampler && pt.tx->sampler->wrap_s ? GLint(pt.tx->sampler->wrap_s) : GL_REPEAT, wt = pt.tx->sampler && pt.tx->sampler->wrap_t ? GLint(pt.tx->sampler->wrap_t) : GL_REPEAT;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, ws); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wt);
}
static void attrib(GLuint slot, const cgltf_accessor* a, int comps) {
    GLuint vbo; glGenBuffers(1, &vbo); glBindBuffer(GL_ARRAY_BUFFER, vbo);
    std::vector<float> d(a->count * comps); cgltf_accessor_unpack_floats(a, d.data(), d.size());
    glBufferData(GL_ARRAY_BUFFER, d.size() * sizeof(float), d.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(slot); glVertexAttribPointer(slot, comps, GL_FLOAT, GL_FALSE, 0, nullptr);
}
namespace {
// cor sRGB -> tipo de superfície. Brancos muito claros (lona do ringue) e cores fortes ficam pintados
int surfaceOf(const glm::vec3& c) {
    const float mx = std::max({c.r, c.g, c.b}), mn = std::min({c.r, c.g, c.b}), v = mx, s = mx > 0 ? (mx - mn) / mx : 0;
    float h = 0; if (mx > mn) { const float d = mx - mn; h = mx == c.r ? 60.f * std::fmod((c.g - c.b) / d + 6.f, 6.f) : mx == c.g ? 60.f * ((c.b - c.r) / d + 2.f) : 60.f * ((c.r - c.g) / d + 4.f); }
    if (h >= 75 && h <= 170 && s > 0.25f && v > 0.2f) return 4;
    if (h >= 8 && h <= 45 && s > 0.35f && v < 0.62f && v > 0.12f) return 1;
    if (s < 0.28f && v > 0.35f && v < 0.95f) return 2;
    if (h >= 25 && h <= 55 && s >= 0.28f && s <= 0.75f && v >= 0.62f) return 3;
    return 0;
}
}
void Model::load(const std::string& path) {
    const auto fail = [&](const char* why) { throw std::runtime_error(path + ": " + why); };
    cgltf_options opt{}; cgltf_data* d = nullptr;
    if (cgltf_parse_file(&opt, path.c_str(), &d) != cgltf_result_success) fail("não abriu ou não é .glb");
    if (cgltf_load_buffers(&opt, d, path.c_str()) != cgltf_result_success) fail("buffers inválidos");
    nodes.resize(d->nodes_count); std::vector<size_t> opens;
    std::map<const cgltf_image*, PendingTex> texs;
    // expressões (face_<nome>_<slot>): a folha da expressão troca a da peça do mesmo slot, então recebe o mesmo preenchimento das laterais
    std::map<int, std::vector<glm::vec2>> slotTris; std::map<const cgltf_image*, int> faceSheet;
    const auto texOf = [&](const cgltf_texture* tx) -> GLuint { if (!tx->image || !tx->image->buffer_view) return 0; PendingTex& pt = texs[tx->image]; if (!pt.id) { pt.tx = tx; glGenTextures(1, &pt.id); } return pt.id; };
    for (size_t i = 0; i < d->nodes_count; i++) {
        const cgltf_node& s = d->nodes[i]; Node& n = nodes[i];
        n.name = s.name ? s.name : "";
        // o GLTFExporter grava matriz, não TRS: decompõe (escala pode ser negativa nas peças espelhadas)
        float mm[16]; cgltf_node_transform_local(&s, mm); const glm::mat4 m = glm::make_mat4(mm);
        n.pos = glm::vec3(m[3]);
        glm::vec3 X(m[0]), Y(m[1]), Z(m[2]);
        n.scale = {glm::length(X), glm::length(Y), glm::length(Z)};
        if (glm::determinant(glm::mat3(m)) < 0) { n.scale.x = -n.scale.x; X = -X; }
        n.rot = glm::quat_cast(glm::mat3(glm::normalize(X), glm::normalize(Y), glm::normalize(Z))); n.rot0 = n.rot;
        n.parent = s.parent ? int(s.parent - d->nodes) : -1;
        for (size_t k = 0; k < s.children_count; k++) n.kids.push_back(int(s.children[k] - d->nodes));
        if (!n.name.empty()) byName[n.name] = int(i);
        const std::string ex = extras(d, s.extras);
        n.lower = n.name == "lower"; n.drip = n.name == "dripBody" ? 1 : n.name == "dripHead" ? 2 : n.name == "dripHand" ? 3 : 0;
        if (n.name == "root") { rootNode = int(i); meta = ex; WAIST = jsonNum(ex, "WAIST"); NECK = jsonNum(ex, "NECK"); HIPS = jsonNum(ex, "HIPS"); legLen = jsonNum(ex, "legLen"); scale = jsonNum(ex, "scale", 1); }
        n.outlineW = jsonNum(ex, "outlineW");
        const auto f = ex.find("\"outlineFade\":["); if (f != std::string::npos) { n.fade = true; const char* b = ex.c_str() + f + 15; char* e2; n.fadeA = std::strtof(b, &e2); n.fadeB = std::strtof(e2 + 1, nullptr); }
        if (!s.mesh) continue;
        if (n.name.rfind("face_", 0) == 0) {   // plano de transporte da textura de uma expressão
            n.visible = false;
            const auto* mat = s.mesh->primitives[0].material;
            if (mat && mat->pbr_metallic_roughness.base_color_texture.texture) { const cgltf_texture* ft = mat->pbr_metallic_roughness.base_color_texture.texture; faces[n.name.substr(5)] = texOf(ft);
                const auto us = n.name.rfind('_'); if (ft->image && us != std::string::npos) faceSheet[ft->image] = std::atoi(n.name.c_str() + us + 1); }
            continue;
        }
        if (n.name == "canopyOpen") { opens.push_back(i); continue; }   // forma aberta: vira atributos da sombrinha irmã
        for (size_t p = 0; p < s.mesh->primitives_count; p++) {
            const cgltf_primitive& pr = s.mesh->primitives[p]; Prim o;
            glGenVertexArrays(1, &o.vao); glBindVertexArray(o.vao); size_t verts = 0;
            for (size_t a = 0; a < pr.attributes_count; a++) { const auto& at = pr.attributes[a];
                if (at.type == cgltf_attribute_type_position) { attrib(0, at.data, 3); verts = at.data->count; if (at.data->has_min && at.data->has_max) { o.lo = glm::make_vec3(at.data->min); o.hi = glm::make_vec3(at.data->max); } }
                else if (at.type == cgltf_attribute_type_normal) attrib(1, at.data, 3);
                else if (at.type == cgltf_attribute_type_texcoord) attrib(at.index == 1 ? 6 : 2, at.data, 2);
                else if (at.type == cgltf_attribute_type_color) { attrib(3, at.data, 3); o.vcol = true; } }
            if (pr.indices) { std::vector<uint32_t> ix(pr.indices->count); for (size_t k = 0; k < ix.size(); k++) ix[k] = uint32_t(cgltf_accessor_read_index(pr.indices, k)); GLuint ebo; glGenBuffers(1, &ebo); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo); glBufferData(GL_ELEMENT_ARRAY_BUFFER, ix.size() * 4, ix.data(), GL_STATIC_DRAW); o.count = GLsizei(ix.size()); o.indexed = true; } else o.count = GLsizei(verts);
            if (pr.material) { const auto& pm = pr.material->pbr_metallic_roughness; o.color = {pm.base_color_factor[0], pm.base_color_factor[1], pm.base_color_factor[2], pm.base_color_factor[3]}; o.twoSided = pr.material->double_sided; o.blend = pr.material->alpha_mode == cgltf_alpha_mode_blend; const std::string mx = extras(d, pr.material->extras); o.unlit = mx.find("\"unlit\":true") != std::string::npos; o.faceSlot = int(jsonNum(mx, "faceSlot", -1));
                if (pr.material->emissive_texture.texture) o.tex2 = texOf(pr.material->emissive_texture.texture);
                if (pm.base_color_texture.texture) o.tex0 = o.tex = texOf(pm.base_color_texture.texture);
                // peça projetada (pintura de frente e de costas): guarda os triângulos de UV das faces viradas para a frente ou para trás
                if (o.tex && o.tex2) { const cgltf_accessor *uvA = nullptr, *nA = nullptr; for (size_t a = 0; a < pr.attributes_count; a++) { const auto& at = pr.attributes[a]; if (at.type == cgltf_attribute_type_texcoord && at.index == 0) uvA = at.data; else if (at.type == cgltf_attribute_type_normal) nA = at.data; }
                    if (uvA && nA) { const size_t nt = pr.indices ? pr.indices->count : verts; std::vector<glm::vec2> tris;
                        for (size_t k = 0; k + 2 < nt; k += 3) { float nz = 0.f; glm::vec2 uv[3]; for (int c = 0; c < 3; c++) { const size_t v = pr.indices ? cgltf_accessor_read_index(pr.indices, k + size_t(c)) : k + size_t(c); float f[3] = {0, 0, 0}; cgltf_accessor_read_float(nA, v, f, 3); nz += f[2] / 3.f; cgltf_accessor_read_float(uvA, v, f, 2); uv[c] = glm::vec2(f[0], f[1]); }
                            if (std::fabs(nz) > 0.35f) tris.insert(tris.end(), uv, uv + 3); }
                        for (const cgltf_texture* t : {pm.base_color_texture.texture, pr.material->emissive_texture.texture}) { PendingTex& pt = texs[t->image]; pt.proj = true; pt.tris.insert(pt.tris.end(), tris.begin(), tris.end()); }
                        if (o.faceSlot >= 0) { auto& st = slotTris[o.faceSlot]; st.insert(st.end(), tris.begin(), tris.end()); } } } }
            if (o.faceSlot >= 0) {   // guarda a malha e a imagem do rosto
                FaceMesh fm; fm.node = int(i); fm.slot = o.faceSlot;
                for (size_t a = 0; a < pr.attributes_count; a++) { const auto& at = pr.attributes[a]; float v[3];
                    if (at.type == cgltf_attribute_type_position) for (size_t k = 0; k < at.data->count; k++) { cgltf_accessor_read_float(at.data, k, v, 3); fm.pos.push_back(glm::make_vec3(v)); }
                    else if (at.type == cgltf_attribute_type_normal) for (size_t k = 0; k < at.data->count; k++) { cgltf_accessor_read_float(at.data, k, v, 3); fm.nrm.push_back(glm::make_vec3(v)); }
                    else if (at.type == cgltf_attribute_type_texcoord && at.index == 0) for (size_t k = 0; k < at.data->count; k++) { cgltf_accessor_read_float(at.data, k, v, 2); fm.uv.push_back(glm::vec2(v[0], v[1])); } }
                if (pr.indices) for (size_t k = 0; k < pr.indices->count; k++) fm.idx.push_back(uint32_t(cgltf_accessor_read_index(pr.indices, k)));
                else for (uint32_t k = 0; k < uint32_t(fm.pos.size()); k++) fm.idx.push_back(k);   // sem índices: triângulos em sequência
                faceMesh.push_back(std::move(fm));
                const cgltf_texture* tx = pr.material ? pr.material->pbr_metallic_roughness.base_color_texture.texture : nullptr;
                if (tx && tx->image && tx->image->buffer_view && !faceImg.count(o.faceSlot)) { sf::Image im; if (im.loadFromMemory(cgltf_buffer_view_data(tx->image->buffer_view), tx->image->buffer_view->size)) {
                    FaceImg fi; fi.w = im.getSize().x; fi.h = im.getSize().y; fi.px.assign(im.getPixelsPtr(), im.getPixelsPtr() + size_t(fi.w) * fi.h * 4); faceImg[o.faceSlot] = std::move(fi); } } }
            // tipo de superfície pela cor (as arenas não dão nome aos materiais): madeira, pedra, areia ou folhagem ganham relevo no shader
            { glm::vec3 col(o.color); bool known = !o.tex;
              if (o.vcol) { known = false; for (size_t a = 0; a < pr.attributes_count; a++) if (pr.attributes[a].type == cgltf_attribute_type_color) { const cgltf_accessor* ac = pr.attributes[a].data; glm::vec3 sum(0.f); const size_t step = std::max<size_t>(1, ac->count / 64); int k = 0; for (size_t v = 0; v < ac->count; v += step, k++) { float c[4] = {1, 1, 1, 1}; cgltf_accessor_read_float(ac, v, c, 4); sum += glm::vec3(c[0], c[1], c[2]); } if (k) { col *= sum / float(k); known = true; } } }
              if (known) o.surf = surfaceOf(glm::pow(glm::clamp(col, 0.f, 1.f), glm::vec3(1.f / 2.2f)));
              const glm::vec3 e = o.hi - o.lo; o.grain = e.x >= e.y && e.x >= e.z ? glm::vec3(1, 0, 0) : e.y >= e.z ? glm::vec3(0, 1, 0) : glm::vec3(0, 0, 1); }
            n.prims.push_back(o);
        }
    }
    for (const size_t i : opens) {
        const cgltf_primitive& op = d->nodes[i].mesh->primitives[0];
        for (const int k : nodes[nodes[i].parent].kids) if (nodes[k].name == "canopy" && !nodes[k].prims.empty()) { Prim& pr = nodes[k].prims[0]; glBindVertexArray(pr.vao);
            for (size_t a = 0; a < op.attributes_count; a++) { if (op.attributes[a].type == cgltf_attribute_type_position) attrib(4, op.attributes[a].data, 3); else if (op.attributes[a].type == cgltf_attribute_type_normal) attrib(5, op.attributes[a].data, 3); }
            pr.morph = true; }
    }
    for (auto& [im, pt] : texs) { const auto fs = faceSheet.find(im); if (fs == faceSheet.end()) continue; const auto st = slotTris.find(fs->second);
        if (st != slotTris.end() && !st->second.empty()) { pt.proj = true; pt.tris.insert(pt.tris.end(), st->second.begin(), st->second.end()); } }
    for (const auto& [im, pt] : texs) uploadTex(pt);
    glBindVertexArray(0); cgltf_free(d);
    if (rootNode < 0) fail("sem o nó root");
}
int Model::need(const std::string& name) const { const auto it = byName.find(name); if (it == byName.end()) throw std::runtime_error("peça obrigatória faltando: " + name); return it->second; }
float Model::num(const char* key, float def) const { return jsonNum(meta, key, def); }
void Model::blendFace(const std::string& face, float k) {
    for (Node& n : nodes) for (Prim& p : n.prims) if (p.faceSlot >= 0) { const auto it = k > 0.f ? faces.find(face + "_" + std::to_string(p.faceSlot)) : faces.end(); p.texB = it != faces.end() ? it->second : 0; p.texMix = p.texB ? k : 0.f; }
}
void Model::setFace(const std::string& face) {
    for (Node& n : nodes) for (Prim& p : n.prims) if (p.faceSlot >= 0) { const auto it = faces.find(face + "_" + std::to_string(p.faceSlot)); p.tex = it != faces.end() ? it->second : p.tex0; }
}
void Model::setMorphUnder(int node, float amount) { nodes[node].morph = amount; for (const int k : nodes[node].kids) setMorphUnder(k, amount); }
void Model::addFace(const std::string& name, int slot, const std::function<void(unsigned w, unsigned h, std::uint8_t* px)>& paint) {
    const auto it = faceImg.find(slot); if (it == faceImg.end()) return; std::vector<std::uint8_t> px = it->second.px; paint(it->second.w, it->second.h, px.data());
    GLuint t; glGenTextures(1, &t); glBindTexture(GL_TEXTURE_2D, t);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, GLsizei(it->second.w), GLsizei(it->second.h), 0, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    glGenerateMipmap(GL_TEXTURE_2D); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    faces[name + "_" + std::to_string(slot)] = t;
}
bool Model::uvPointSmooth(int slot, const glm::vec2& uv, int& node, glm::vec3& pos, glm::vec3& nrm) const {
    struct C { float d; int node; glm::vec3 p, n; }; C best[4]; int nb = 0;
    for (const FaceMesh& fm : faceMesh) if (fm.slot == slot) for (size_t k = 0; k < std::min(fm.pos.size(), fm.uv.size()); k++) { const float d = glm::length(fm.uv[k] - uv); const C c{d, fm.node, fm.pos[k], k < fm.nrm.size() ? fm.nrm[k] : glm::vec3(0, 0, 1)};
        if (nb < 4) { best[nb++] = c; } else { int w = 0; for (int i = 1; i < 4; i++) if (best[i].d > best[w].d) w = i; if (d < best[w].d) best[w] = c; } }
    // primeiro: o triângulo que contém o ponto da textura (interpola de verdade); senão, a média dos mais perto
    for (const FaceMesh& fm : faceMesh) if (fm.slot == slot) for (size_t k = 0; k + 2 < fm.idx.size(); k += 3) { const uint32_t a = fm.idx[k], b = fm.idx[k + 1], c = fm.idx[k + 2]; if (a >= fm.uv.size() || b >= fm.uv.size() || c >= fm.uv.size()) continue;
        const glm::vec2 A = fm.uv[a], B = fm.uv[b], C = fm.uv[c]; const float d = (B.y - C.y) * (A.x - C.x) + (C.x - B.x) * (A.y - C.y); if (std::fabs(d) < 1e-12f) continue;
        const float wa = ((B.y - C.y) * (uv.x - C.x) + (C.x - B.x) * (uv.y - C.y)) / d, wb = ((C.y - A.y) * (uv.x - C.x) + (A.x - C.x) * (uv.y - C.y)) / d, wc = 1.f - wa - wb;
        if (wa < -1e-4f || wb < -1e-4f || wc < -1e-4f) continue; node = fm.node; pos = fm.pos[a] * wa + fm.pos[b] * wb + fm.pos[c] * wc;
        const glm::vec3 na = a < fm.nrm.size() ? fm.nrm[a] : glm::vec3(0, 0, 1), nb2 = b < fm.nrm.size() ? fm.nrm[b] : na, nc = c < fm.nrm.size() ? fm.nrm[c] : na; nrm = glm::normalize(na * wa + nb2 * wb + nc * wc + glm::vec3(1e-6f)); return true; }
    if (nb == 0) return false; node = best[0].node; glm::vec3 p(0.f), n(0.f); float W = 0.f; float dmin = 1e9f;
    for (int i = 0; i < nb; i++) { if (best[i].node != node) continue; const float w = 1.f / (best[i].d + 1e-4f); p += best[i].p * w; n += best[i].n * w; W += w; dmin = std::min(dmin, best[i].d); }
    pos = p / W; nrm = glm::normalize(n + glm::vec3(1e-6f)); return dmin < 0.1f;
}
bool Model::uvPoint(int slot, const glm::vec2& uv, int& node, glm::vec3& pos, glm::vec3& nrm) const {
    float best = 1e9f;
    for (const FaceMesh& fm : faceMesh) if (fm.slot == slot) for (size_t k = 0; k < std::min(fm.pos.size(), fm.uv.size()); k++) { const float d = glm::length(fm.uv[k] - uv); if (d < best) { best = d; node = fm.node; pos = fm.pos[k]; nrm = k < fm.nrm.size() ? fm.nrm[k] : glm::vec3(0, 0, 1); } }
    return best < 0.1f;
}
float Model::headTop() const {
    float top = 0;
    const auto go = [&](auto&& self, int i, const glm::mat4& w) -> void {
        const Node& n = nodes[i]; if (!n.visible || n.name.rfind("face_", 0) == 0) return;
        const glm::mat4 me = w * glm::translate(glm::mat4(1.f), n.pos) * glm::mat4_cast(n.rot) * glm::scale(glm::mat4(1.f), n.scale);
        for (const Prim& p : n.prims) for (int c = 0; c < 8; c++) top = std::max(top, (me * glm::vec4(c & 1 ? p.hi.x : p.lo.x, c & 2 ? p.hi.y : p.lo.y, c & 4 ? p.hi.z : p.lo.z, 1)).y);
        for (int k : n.kids) self(self, k, me);
    };
    for (size_t i = 0; i < nodes.size(); i++) if (nodes[i].name == "neck") { for (int k : nodes[i].kids) go(go, k, glm::mat4(1.f)); for (const Prim& p : nodes[i].prims) top = std::max(top, p.hi.y); break; }
    return (NECK + top) * scale;
}
void Model::updateWorld(const glm::mat4& base) {
    const auto go = [&](auto&& self, int i, const glm::mat4& w) -> void { Node& n = nodes[i]; n.world = glm::scale(glm::translate(w, n.pos) * glm::mat4_cast(n.rot), n.scale); for (int k : n.kids) self(self, k, n.world); };
    go(go, rootNode, base);
}
}
