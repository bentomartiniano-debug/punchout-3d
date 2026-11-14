# K.O.-RAMA 3D, etapa 1: Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Criar o executável `korama3d`, em que o usuário joga uma luta completa de Valéria contra Valéria em 3D, usando `korama_core` sem alterá-lo.

**Architecture:** Uma ferramenta no navegador exporta o boneco do vinil para `assets3d/game/valeria.glb`. O C++ carrega esse arquivo em uma árvore de peças, converte cada `Fighter` em transformações (`rig`, testado sem janela) e desenha com toon, retícula e contorno em OpenGL 3.3. A HUD é SFML desenhada por cima.

**Tech Stack:** C++17, clang + CMake + Ninja, SFML 3.1, glad, glm, cgltf; Three.js 0.169 e Python só no exportador.

**Regras deste repositório:** commits só quando o usuário pedir (os passos "Commit" valem como ponto de parada e pedido). Não injetar teclas para testar: conferir por PNG (Task 7). `core.js` tem alterações alheias da arena do Marcov: ao commitar, incluir só a parte desta tarefa.

---

## Arquivos

| Arquivo | Responsabilidade |
|---|---|
| `tools3d/export/serve.py` | Servidor estático local que também grava `.glb` por PUT |
| `tools3d/export/export.html` | Monta o boneco, simplifica materiais e grava o `.glb` |
| `tools3d/viewer/vinil/core.js` (modificar) | Guarda a espessura do contorno em `userData` |
| `engine-cpp/app3d/gl/shader.hpp/.cpp` | Compila programas GLSL |
| `engine-cpp/app3d/model/model.hpp/.cpp` | Carrega o `.glb` em nós, malhas e materiais |
| `engine-cpp/app3d/rig/rig.hpp/.cpp` | `Fighter` para transformações, sem OpenGL |
| `engine-cpp/app3d/render/renderer.hpp/.cpp` | Ringue, toon e contorno |
| `engine-cpp/app3d/game/main.cpp` | Janela, laço, teclado, HUD, modo `--snap` |
| `engine-cpp/tests/rig_tests.cpp` | Testes do `rig` |
| `engine-cpp/CMakeLists.txt` (modificar) | Alvos `korama3d` e `korama3d_rig_tests` |

---

### Task 1: Exportar a Valéria para `.glb`

**Files:**
- Create: `tools3d/export/serve.py`, `tools3d/export/export.html`
- Modify: `tools3d/viewer/vinil/core.js` (função `withOutline`)

- [ ] **Step 1: Guardar a espessura do contorno**

Em `withOutline`, trocar `const hull = new THREE.Mesh(mesh.geometry, m); hull.userData.hull = true;` por:

```js
const hull = new THREE.Mesh(mesh.geometry, m); hull.userData.hull = true; hull.userData.outlineW = w; hull.userData.outlineFade = fadeY;
```

- [ ] **Step 2: Criar o servidor**

```python
# tools3d/export/serve.py: serve o repositório e grava em assets3d/game/ (só nesta pasta).
import http.server, pathlib, sys
ROOT = pathlib.Path(__file__).resolve().parents[2]
OUT = ROOT / 'assets3d' / 'game'
class H(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *a, **k): super().__init__(*a, directory=str(ROOT), **k)
    def do_PUT(self):
        name = pathlib.PurePosixPath(self.path).name
        if not name.endswith('.glb'): self.send_error(400); return
        OUT.mkdir(parents=True, exist_ok=True)
        (OUT / name).write_bytes(self.rfile.read(int(self.headers['Content-Length'])))
        self.send_response(204); self.end_headers()
http.server.ThreadingHTTPServer(('127.0.0.1', int(sys.argv[1]) if len(sys.argv) > 1 else 8770), H).serve_forever()
```

- [ ] **Step 3: Criar a página de exportação**

Regras: nó com `userData.hull` não é exportado e passa `outlineW`/`outlineFade` para a malha irmã. Material vira `MeshStandardMaterial` com cor, textura e cores de vértice. O shader do rosto vira UV calculado pela `faceBox`. O shader do cabelo e o da sombrinha viram cores de vértice. As `extras` marcam `unlit` em `MeshBasicMaterial`.

```html
<!doctype html><meta charset="utf-8"><title>Exportar lutador</title><pre id="log"></pre>
<script type="importmap">{"imports":{"three":"https://cdn.jsdelivr.net/npm/three@0.169.0/build/three.module.js","three/addons/":"https://cdn.jsdelivr.net/npm/three@0.169.0/examples/jsm/"}}</script>
<script type="module">
import * as THREE from 'three';
import { GLTFExporter } from 'three/addons/exporters/GLTFExporter.js';
import { units } from '../viewer/vinil/core.js';
const key = new URLSearchParams(location.search).get('p') || 'valeria';
const log = t => { document.getElementById('log').textContent += t + '\n'; };
const char = (await import(`../viewer/vinil/${key}.js`)).default;
const root = new THREE.Group(), scene = new THREE.Scene(); scene.add(root);
const rig = await char.build({ THREE, scene, root, clock: new THREE.Clock(), U: units(...char.units), $: s => document.querySelector(s) });
rig.placeFoot(-1, 0, 0); rig.placeFoot(1, 0, 0); root.updateMatrixWorld(true);
const FACES = (char.faces || []).map(f => f[0]);
// assume a identidade em cor de vértice: cabelo por altura (gradiente da lava), sombrinha pelo atributo gp
const colorize = (mesh, g) => {
  if (g.attributes.gp) { const C = [[1, .55, .05], [1, .29, 0], [.18, .82, .75], [.95, .77, .1]], p = g.attributes.gp, c = new Float32Array(p.count * 3); for (let i = 0; i < p.count; i++) { const v = p.getX(i), q = C[Math.min(3, Math.max(0, Math.floor(v / 2)))], t = v % 1, e = Math.min(t, 1 - t), w = e < .07 ? 1 : 0, col = new THREE.Color().setRGB(q[0], q[1], q[2], THREE.SRGBColorSpace); c.set([col.r + (1 - col.r) * w, col.g + (1 - col.g) * w, col.b + (1 - col.b) * w], i * 3); } g.setAttribute('color', new THREE.BufferAttribute(c, 3)); return true; }
  if (mesh.material.userData?.lava || /lava/i.test(mesh.material.customProgramCacheKey?.() || '')) { g.computeBoundingBox(); const b = g.boundingBox, p = g.attributes.position, c = new Float32Array(p.count * 3), S = [[.55, .04, .10], [.88, .16, .08], [1, .45, .04], [1, .76, .12]]; for (let i = 0; i < p.count; i++) { const t = (p.getY(i) - b.min.y) / (b.max.y - b.min.y), s = t * 3, k = Math.min(2, Math.floor(s)), u = s - k, a = S[k], d = S[k + 1], col = new THREE.Color().setRGB(a[0] + (d[0] - a[0]) * u, a[1] + (d[1] - a[1]) * u, a[2] + (d[2] - a[2]) * u, THREE.SRGBColorSpace); c.set([col.r, col.g, col.b], i * 3); } g.setAttribute('color', new THREE.BufferAttribute(c, 3)); return true; }
  return false;
};
const faceMats = new Map();
const convert = src => {
  const dst = new THREE.Object3D(); dst.name = src.name; dst.position.copy(src.position); dst.quaternion.copy(src.quaternion); dst.scale.copy(src.scale); dst.visible = src.visible;
  const NAMED = new Map([[rig.upper, 'upper'], [rig.neck, 'neck'], [rig.hips, 'hips'], [rig.head, 'head'], [rig.P.legL, 'legL'], [rig.P.legR, 'legR'], [rig.P.bootL, 'bootL'], [rig.P.bootR, 'bootR'], [rig.P.umbL, 'gloveL'], [rig.P.umbR, 'gloveR']]);
  if (NAMED.has(src)) dst.name = NAMED.get(src);
  for (const ch of src.children) {
    if (ch.userData.hull) continue;
    if (ch.isMesh) {
      const hull = src.children.find(o => o.userData.hull && o.geometry === ch.geometry), g = ch.geometry.clone(), mt = ch.material, painted = colorize(ch, g);
      const out = new THREE.MeshStandardMaterial({ color: painted ? 0xffffff : (mt.color || new THREE.Color(1, 1, 1)), map: mt.map?.isTexture && !painted ? mt.map : null, vertexColors: painted || !!mt.vertexColors, side: mt.side === THREE.DoubleSide ? THREE.DoubleSide : THREE.FrontSide, transparent: !!mt.transparent, opacity: mt.opacity ?? 1 });
      out.userData = { unlit: !!mt.isMeshBasicMaterial };
      if (mt === rig.P.faceMat || mt.userData?.flatSkin) { const box = rig.P.faceBox; if (box && !g.attributes.uv) { const p = g.attributes.position, uv = new Float32Array(p.count * 2); for (let i = 0; i < p.count; i++) uv.set([(p.getX(i) - box[0]) / (box[2] - box[0]), (p.getY(i) - box[1]) / (box[3] - box[1])], i * 2); g.setAttribute('uv', new THREE.BufferAttribute(uv, 2)); } faceMats.set(out, mt); out.userData.face = true; }
      const m = new THREE.Mesh(g, out); m.name = ch.name; m.position.copy(ch.position); m.quaternion.copy(ch.quaternion); m.scale.copy(ch.scale); m.visible = ch.visible;
      m.userData = { outlineW: hull?.userData.outlineW ?? 0, outlineFade: hull?.userData.outlineFade ?? null };
      if (ch.children.length) for (const k of ch.children) { const c = convert(k); if (c) m.add(c); }
      dst.add(m);
    } else { const c = convert(ch); if (c) dst.add(c); }
  }
  return dst;
};
const out = convert(root); out.name = 'root';
const legBox = new THREE.Box3().setFromObject(rig.P.legL), legLen = rig.P.legL.getWorldPosition(new THREE.Vector3()).y - legBox.min.y;
out.userData = { rig: { WAIST: rig.WAIST, NECK: rig.NECK, HIPS: rig.HIPS, legLen, scale: rig.scale ?? char.scale ?? 1, faces: FACES } };
log(`nós: ${out.children.map(o => o.name).join(', ')}`);
const buf = await new GLTFExporter().parseAsync(out, { binary: true });
await fetch(`/${key}.glb`, { method: 'PUT', body: buf });
log(`pronto: ${(buf.byteLength / 1048576).toFixed(1)} MB`); document.title = 'OK';
</script>
```

- [ ] **Step 4: Rodar o servidor e a exportação**

Rodar `python tools3d/export/serve.py 8770` em segundo plano e abrir `http://localhost:8770/tools3d/export/export.html?p=valeria` no navegador interno.
Expected: título `OK`, log com `upper, hips, legL, legR, bootL, bootR, gloveL, gloveR`, e o arquivo `assets3d/game/valeria.glb`.

Se `rig.P.faceBox` não existir: expor `faceBox` e `faceMat` no retorno da Valéria com `P.faceBox = B.faceBox; P.faceMat = B.faceMat`, logo depois de criar `faceMat` em `valeria.js:198`, e repetir.

- [ ] **Step 5: Validar as peças**

```bash
node -e "const b=require('fs').readFileSync('assets3d/game/valeria.glb');const n=b.readUInt32LE(12);const j=JSON.parse(b.slice(20,20+n));const s=new Set(j.nodes.map(x=>x.name));for(const r of ['upper','neck','head','hips','legL','legR','bootL','bootR','gloveL','gloveR'])if(!s.has(r))throw 'falta '+r;console.log('ok',j.nodes.length,'nós')"
```
Expected: `ok N nós`.

- [ ] **Step 6: Commit** (`tools3d/export/`, hunk de `withOutline` em `core.js`, `assets3d/game/valeria.glb`), quando o usuário pedir.

---

### Task 2: `rig`, com teste antes

**Files:**
- Create: `engine-cpp/app3d/rig/rig.hpp`, `engine-cpp/app3d/rig/rig.cpp`, `engine-cpp/tests/rig_tests.cpp`
- Modify: `engine-cpp/CMakeLists.txt`

Convenção: o jogo usa x para a direita da tela, z para a frente. O boneco usa destro com +z para a frente. Por isso `x` troca de sinal e a luva `L` do motor é o lado `+1` do boneco, nos dois lutadores.

- [ ] **Step 1: Adicionar dependências e alvo de teste ao CMake**

Depois de `FetchContent_MakeAvailable(SFML)`:

```cmake
# --- Jogo 3D isolado (etapa 1 do docs/superpowers/specs/2026-10-30-korama3d-design.md) ---
FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm.git GIT_TAG 1.0.1 GIT_SHALLOW ON EXCLUDE_FROM_ALL SYSTEM)
FetchContent_Declare(cgltf GIT_REPOSITORY https://github.com/jkuhlmann/cgltf.git GIT_TAG v1.14 GIT_SHALLOW ON)
set(GLAD_PROFILE "core" CACHE STRING "" FORCE)
set(GLAD_API "gl=3.3" CACHE STRING "" FORCE)
FetchContent_Declare(glad GIT_REPOSITORY https://github.com/Dav1dde/glad.git GIT_TAG v0.1.36 GIT_SHALLOW ON EXCLUDE_FROM_ALL SYSTEM)
FetchContent_MakeAvailable(glm cgltf glad)

add_library(korama3d_rig STATIC app3d/rig/rig.cpp)
target_include_directories(korama3d_rig PUBLIC app3d)
target_link_libraries(korama3d_rig PUBLIC korama_core glm::glm)

add_executable(korama3d_rig_tests tests/rig_tests.cpp)
target_link_libraries(korama3d_rig_tests PRIVATE korama3d_rig)
if (MINGW)
    target_link_options(korama3d_rig_tests PRIVATE -static)
endif()
add_test(NAME rig3d COMMAND korama3d_rig_tests)
```

- [ ] **Step 2: Escrever o teste que falha**

```cpp
// tests/rig_tests.cpp: o rig converte o estado do motor em transformações, sem janela.
#include "rig/rig.hpp"
#include <cmath>
#include <cstdio>
using namespace KoRama;
static int fails = 0;
static void near(const char* what, float a, float b) { if (std::fabs(a - b) > 1e-3f) { std::printf("FALHOU %s: %f != %f\n", what, a, b); fails++; } }
int main() {
    Fighter p(4, true), e(4, false);
    p.worldPos = {0, 0, -125}; p.facing = 0;
    e.worldPos = {0, 0, 125}; e.facing = kRigPi;
    p.gloveL = {-32, 64, 12, 54, 54, 0, 0}; p.gloveR = {32, 64, 100, 54, 54, 0, 0};
    e.gloveL = p.gloveL; e.gloveR = p.gloveR;
    RigRest rest; rest.WAIST = 0.61f; rest.NECK = 0.92f; rest.HIPS = 0.63f; rest.legLen = 0.55f;
    rest.hip[0] = {-0.3f, 0.55f, 0}; rest.hip[1] = {0.3f, 0.55f, 0};
    rest.foot[0] = {-0.3f, 0.08f, 0}; rest.foot[1] = {0.3f, 0.08f, 0};
    const FighterPose a = computePose(p, rest, 0), b = computePose(e, rest, 0);
    near("jogador z", a.root[3][2], -125 * kRigUnit);
    near("oponente z", b.root[3][2], 125 * kRigUnit);
    near("jogador olha +z", (a.root * glm::vec4(0, 0, 1, 0)).z, 1);
    near("oponente olha -z", (b.root * glm::vec4(0, 0, 1, 0)).z, -1);
    near("luva L vai ao lado +1", a.glove[1].x, 32 * kRigUnit);
    near("luva R vai ao lado -1", a.glove[0].x, -32 * kRigUnit);
    near("altura da luva", a.glove[1].y, 64 * kRigUnit + kGloveLift);
    near("soco vai à frente", a.glove[0].z, 100 * kRigUnit);
    p.offsetY = -16; const FighterPose c = computePose(p, rest, 0);
    near("agacha", c.upperPos.y, rest.WAIST - 16 * kRigUnit);
    near("pé fica no chão", c.footPos[0].y, rest.foot[0].y);
    near("perna alcança o pé", c.legDirLen[0].y, -(c.legPos[0].y - (c.footPos[0].y + (rest.hip[0].y - rest.legLen - rest.foot[0].y))));
    const FighterPose d = computePose(p, rest, 1);
    near("caído fica deitado", std::fabs((d.root * glm::vec4(0, 1, 0, 0)).y), 0);
    if (!fails) std::printf("rig ok\n");
    return fails ? 1 : 0;
}
```

- [ ] **Step 3: Conferir que falha**

Run: `cmake --preset release && cmake --build build/release --target korama3d_rig_tests`
Expected: erro de compilação, `rig/rig.hpp` não existe.

- [ ] **Step 4: Implementar**

```cpp
// app3d/rig/rig.hpp: estado do lutador no motor -> transformações das peças (metros, destro).
#pragma once
#include "fighter.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
namespace KoRama {
constexpr float kRigPi = 3.14159265358979f;
constexpr float kRigUnit = 0.0125f;   // 1 unidade do motor em metros (Valéria: ~135 unidades = 1,7 m)
constexpr float kGloveLift = 0.12f;   // a luva de guarda do motor (y 64) fica na altura do peito do boneco
struct RigRest { float WAIST = 0, NECK = 0, HIPS = 0, legLen = 0; glm::vec3 hip[2]{}, foot[2]{}; };
struct FighterPose {
    glm::mat4 root{1.f};
    glm::vec3 upperPos{}, hipsPos{}, glove[2]{}, footPos[2]{}, legPos[2]{}, legDirLen[2]{};
    float roll = 0, lean = 0, twist = 0, neckTilt = 0, neckTurn = 0;
};
// índice 0 = lado -1 do boneco; 1 = lado +1. down: 0 de pé, 1 deitado (queda).
FighterPose computePose(const Fighter& f, const RigRest& rest, float down);
}
```

```cpp
// app3d/rig/rig.cpp
#include "rig/rig.hpp"
#include <algorithm>
namespace KoRama {
FighterPose computePose(const Fighter& f, const RigRest& r, float down) {
    FighterPose p;
    p.root = glm::translate(glm::mat4(1.f), glm::vec3(-f.worldPos.x, f.worldPos.y, f.worldPos.z) * kRigUnit);
    p.root = glm::rotate(p.root, -f.facing, glm::vec3(0, 1, 0));
    if (down > 0) p.root = glm::rotate(p.root, -down * kRigPi / 2, glm::vec3(1, 0, 0));
    const glm::vec3 off(-f.offsetX * kRigUnit, f.offsetY * kRigUnit, f.offsetZ * kRigUnit);
    p.upperPos = glm::vec3(0, r.WAIST, 0) + off;
    p.hipsPos = glm::vec3(0, r.HIPS, 0) + off * 0.8f;
    const auto hand = [&](const PartRenderData& g) { return glm::vec3(-g.x * kRigUnit, g.y * kRigUnit + kGloveLift, g.z * kRigUnit) + off; };
    p.glove[1] = hand(f.gloveL); p.glove[0] = hand(f.gloveR);
    // o tronco acompanha o soco: gira para o lado da luva mais à frente e inclina junto
    const float dz = f.gloveL.z - f.gloveR.z, reach = std::max(f.gloveL.z, f.gloveR.z) - 12.0f;
    p.twist = std::clamp(dz * 0.0035f, -0.45f, 0.45f);
    p.lean = std::clamp(reach * 0.0016f, -0.2f, 0.26f);
    p.roll = f.bodySkew;
    if (f.hitstopTimer > 0 || f.hitKind) { const float k = f.hitDir * (f.hitKind == 2 ? 0.3f : 0.12f); p.neckTurn = f.hitKind ? k : 0; p.neckTilt = f.hitKind == 4 ? -0.3f : 0; }
    for (int i = 0; i < 2; i++) {
        p.footPos[i] = r.foot[i] + glm::vec3(off.x * 0.55f, 0, off.z * 0.55f);
        const glm::vec3 a = r.hip[i] + (p.hipsPos - glm::vec3(0, r.HIPS, 0));
        const glm::vec3 b = p.footPos[i] + glm::vec3(0, r.hip[i].y - r.legLen - r.foot[i].y, 0);
        p.legPos[i] = a; p.legDirLen[i] = b - a;
    }
    return p;
}
}
```

- [ ] **Step 5: Rodar**

Run: `cmake --build build/release --target korama3d_rig_tests && build/release/korama3d_rig_tests`
Expected: `rig ok`.

- [ ] **Step 6: Commit**, quando o usuário pedir.

---

### Task 3: Carregar o modelo

**Files:**
- Create: `engine-cpp/app3d/gl/shader.hpp`, `engine-cpp/app3d/gl/shader.cpp`, `engine-cpp/app3d/model/model.hpp`, `engine-cpp/app3d/model/model.cpp`

- [ ] **Step 1: Shader**

```cpp
// app3d/gl/shader.hpp
#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
namespace KoRama {
class Shader {
public:
    void build(const char* vs, const char* fs);   // lança std::runtime_error com o log
    void use() const { glUseProgram(id_); }
    GLint loc(const char* n) const { return glGetUniformLocation(id_, n); }
    void set(const char* n, const glm::mat4& v) const;
    void set(const char* n, const glm::vec4& v) const;
    void set(const char* n, const glm::vec3& v) const;
    void set(const char* n, float v) const;
    void set(const char* n, int v) const;
private:
    GLuint id_ = 0;
};
}
```

```cpp
// app3d/gl/shader.cpp
#include "gl/shader.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>
namespace KoRama {
static GLuint compile(GLenum t, const char* src) {
    GLuint s = glCreateShader(t); glShaderSource(s, 1, &src, nullptr); glCompileShader(s);
    GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) { char log[2048]; glGetShaderInfoLog(s, sizeof log, nullptr, log); throw std::runtime_error(std::string("shader: ") + log); }
    return s;
}
void Shader::build(const char* vs, const char* fs) {
    GLuint a = compile(GL_VERTEX_SHADER, vs), b = compile(GL_FRAGMENT_SHADER, fs);
    id_ = glCreateProgram(); glAttachShader(id_, a); glAttachShader(id_, b); glLinkProgram(id_);
    GLint ok = 0; glGetProgramiv(id_, GL_LINK_STATUS, &ok);
    if (!ok) { char log[2048]; glGetProgramInfoLog(id_, sizeof log, nullptr, log); throw std::runtime_error(std::string("link: ") + log); }
    glDeleteShader(a); glDeleteShader(b);
}
void Shader::set(const char* n, const glm::mat4& v) const { glUniformMatrix4fv(loc(n), 1, GL_FALSE, glm::value_ptr(v)); }
void Shader::set(const char* n, const glm::vec4& v) const { glUniform4fv(loc(n), 1, glm::value_ptr(v)); }
void Shader::set(const char* n, const glm::vec3& v) const { glUniform3fv(loc(n), 1, glm::value_ptr(v)); }
void Shader::set(const char* n, float v) const { glUniform1f(loc(n), v); }
void Shader::set(const char* n, int v) const { glUniform1i(loc(n), v); }
}
```

- [ ] **Step 2: Modelo**

```cpp
// app3d/model/model.hpp: árvore de peças do .glb, com malhas na GPU.
#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <map>
#include <string>
#include <vector>
namespace KoRama {
struct Prim { GLuint vao = 0; GLsizei count = 0; GLenum indexType = GL_UNSIGNED_INT; bool indexed = false; glm::vec4 color{1.f}; GLuint tex = 0; bool unlit = false, vcol = false, twoSided = false, blend = false, face = false; };
struct Node { std::string name; glm::vec3 pos{0.f}, scale{1.f}; glm::quat rot{1, 0, 0, 0}; bool visible = true; int parent = -1; std::vector<int> kids; std::vector<Prim> prims; float outlineW = 0, fadeA = 0, fadeB = 0; bool fade = false; glm::mat4 world{1.f}; };
struct Model {
    std::vector<Node> nodes; int rootNode = -1; std::map<std::string, int> byName;
    float WAIST = 0, NECK = 0, HIPS = 0, legLen = 0, scale = 1;
    void load(const std::string& path);   // lança std::runtime_error com o arquivo e o motivo
    int need(const std::string& name) const;   // lança citando a peça que falta
    void updateWorld(const glm::mat4& base);
};
}
```

```cpp
// app3d/model/model.cpp
#define CGLTF_IMPLEMENTATION
#include "model/model.hpp"
#include <SFML/Graphics/Image.hpp>
#include <cgltf.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cstring>
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
static GLuint uploadTex(const cgltf_image* img) {
    if (!img || !img->buffer_view) return 0;
    sf::Image im; const auto* bytes = cgltf_buffer_view_data(img->buffer_view);
    if (!im.loadFromMemory(bytes, img->buffer_view->size)) throw std::runtime_error("textura inválida");
    GLuint t; glGenTextures(1, &t); glBindTexture(GL_TEXTURE_2D, t);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, im.getSize().x, im.getSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, im.getPixelsPtr());
    glGenerateMipmap(GL_TEXTURE_2D); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return t;
}
static void attrib(GLuint slot, const cgltf_accessor* a, int comps) {
    GLuint vbo; glGenBuffers(1, &vbo); glBindBuffer(GL_ARRAY_BUFFER, vbo);
    std::vector<float> d(a->count * comps); cgltf_accessor_unpack_floats(a, d.data(), d.size());
    glBufferData(GL_ARRAY_BUFFER, d.size() * sizeof(float), d.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(slot); glVertexAttribPointer(slot, comps, GL_FLOAT, GL_FALSE, 0, nullptr);
}
void Model::load(const std::string& path) {
    const auto fail = [&](const char* why) { throw std::runtime_error(path + ": " + why); };
    cgltf_options opt{}; cgltf_data* d = nullptr;
    if (cgltf_parse_file(&opt, path.c_str(), &d) != cgltf_result_success) fail("não abriu ou não é .glb");
    if (cgltf_load_buffers(&opt, d, path.c_str()) != cgltf_result_success) fail("buffers inválidos");
    nodes.resize(d->nodes_count);
    std::map<const cgltf_image*, GLuint> texs;
    for (size_t i = 0; i < d->nodes_count; i++) {
        const cgltf_node& s = d->nodes[i]; Node& n = nodes[i];
        n.name = s.name ? s.name : "";
        if (s.has_translation) n.pos = {s.translation[0], s.translation[1], s.translation[2]};
        if (s.has_scale) n.scale = {s.scale[0], s.scale[1], s.scale[2]};
        if (s.has_rotation) n.rot = glm::quat(s.rotation[3], s.rotation[0], s.rotation[1], s.rotation[2]);
        n.parent = s.parent ? int(s.parent - d->nodes) : -1;
        for (size_t k = 0; k < s.children_count; k++) n.kids.push_back(int(s.children[k] - d->nodes));
        if (!n.name.empty()) byName[n.name] = int(i);
        const std::string ex = extras(d, s.extras);
        if (n.name == "root") { rootNode = int(i); WAIST = jsonNum(ex, "WAIST"); NECK = jsonNum(ex, "NECK"); HIPS = jsonNum(ex, "HIPS"); legLen = jsonNum(ex, "legLen"); scale = jsonNum(ex, "scale", 1); }
        n.outlineW = jsonNum(ex, "outlineW");
        const auto f = ex.find("\"outlineFade\":["); if (f != std::string::npos) { n.fade = true; const char* b = ex.c_str() + f + 15; char* e2; n.fadeA = std::strtof(b, &e2); n.fadeB = std::strtof(e2 + 1, nullptr); }
        if (!s.mesh) continue;
        for (size_t p = 0; p < s.mesh->primitives_count; p++) {
            const cgltf_primitive& pr = s.mesh->primitives[p]; Prim o;
            glGenVertexArrays(1, &o.vao); glBindVertexArray(o.vao); size_t verts = 0;
            for (size_t a = 0; a < pr.attributes_count; a++) { const auto& at = pr.attributes[a];
                if (at.type == cgltf_attribute_type_position) { attrib(0, at.data, 3); verts = at.data->count; }
                else if (at.type == cgltf_attribute_type_normal) attrib(1, at.data, 3);
                else if (at.type == cgltf_attribute_type_texcoord) attrib(2, at.data, 2);
                else if (at.type == cgltf_attribute_type_color) { attrib(3, at.data, 3); o.vcol = true; } }
            if (pr.indices) { std::vector<uint32_t> ix(pr.indices->count); for (size_t k = 0; k < ix.size(); k++) ix[k] = uint32_t(cgltf_accessor_read_index(pr.indices, k)); GLuint ebo; glGenBuffers(1, &ebo); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo); glBufferData(GL_ELEMENT_ARRAY_BUFFER, ix.size() * 4, ix.data(), GL_STATIC_DRAW); o.count = GLsizei(ix.size()); o.indexed = true; } else o.count = GLsizei(verts);
            if (pr.material) { const auto& pm = pr.material->pbr_metallic_roughness; o.color = {pm.base_color_factor[0], pm.base_color_factor[1], pm.base_color_factor[2], pm.base_color_factor[3]}; o.twoSided = pr.material->double_sided; o.blend = pr.material->alpha_mode == cgltf_alpha_mode_blend; const std::string mx = extras(d, pr.material->extras); o.unlit = mx.find("\"unlit\":true") != std::string::npos; o.face = mx.find("\"face\":true") != std::string::npos;
                if (pm.base_color_texture.texture) { const auto* im = pm.base_color_texture.texture->image; auto it = texs.find(im); o.tex = it != texs.end() ? it->second : (texs[im] = uploadTex(im)); } }
            n.prims.push_back(o);
        }
    }
    glBindVertexArray(0); cgltf_free(d);
    if (rootNode < 0) fail("sem o nó root");
}
int Model::need(const std::string& name) const { const auto it = byName.find(name); if (it == byName.end()) throw std::runtime_error("peça obrigatória faltando: " + name); return it->second; }
void Model::updateWorld(const glm::mat4& base) {
    const auto go = [&](auto&& self, int i, const glm::mat4& w) -> void { Node& n = nodes[i]; n.world = glm::scale(glm::translate(w, n.pos) * glm::mat4_cast(n.rot), n.scale); for (int k : n.kids) self(self, k, n.world); };
    go(go, rootNode, base);
}
}
```

- [ ] **Step 3: Compilar** junto com a Task 4 (o primeiro alvo que usa estes arquivos é `korama3d`).

---

### Task 4: Renderizador

**Files:**
- Create: `engine-cpp/app3d/render/renderer.hpp`, `engine-cpp/app3d/render/renderer.cpp`

- [ ] **Step 1: Interface**

```cpp
// app3d/render/renderer.hpp
#pragma once
#include "gl/shader.hpp"
#include "model/model.hpp"
#include "rig/rig.hpp"
namespace KoRama {
class Renderer {
public:
    void init();
    void begin(int w, int h, float ringAngle, const Fighter& player);
    void ring();
    void fighter(Model& m, const FighterPose& pose);
private:
    void apply(Model& m, const FighterPose& p);
    void draw(const Node& n, bool hull);
    Shader toon_, hull_, flat_;
    glm::mat4 view_{1.f}, proj_{1.f};
    glm::vec3 sun_{0.3f, 0.75f, -0.6f};
    GLuint ringVao_ = 0, cubeVao_ = 0; GLsizei ringCount_ = 0;
};
}
```

- [ ] **Step 2: Implementação**

A câmera repete a do jogo (`Camera3D` + `placeCamera`): 220 de altura, `-260 - (radius - 60)` de raio, pitch 0,45, em unidades do motor convertidas para metros. O toon repete o `comicShader` do vinil: corte duro em 0,12, sombra 0,73, retícula de célula 5,5 a 45° e força 0,87.

```cpp
// app3d/render/renderer.cpp
#include "render/renderer.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
namespace KoRama {
static const char* kToonVS = R"(#version 330 core
layout(location=0) in vec3 aP; layout(location=1) in vec3 aN; layout(location=2) in vec2 aU; layout(location=3) in vec3 aC;
uniform mat4 uVP, uM; out vec3 vN; out vec2 vU; out vec3 vC;
void main(){ vN = mat3(transpose(inverse(uM))) * aN; vU = aU; vC = aC; gl_Position = uVP * uM * vec4(aP,1.0); })";
static const char* kToonFS = R"(#version 330 core
in vec3 vN; in vec2 vU; in vec3 vC; out vec4 o;
uniform vec4 uColor; uniform vec3 uSun; uniform sampler2D uTex; uniform int uHasTex, uVCol, uUnlit;
void main(){ vec4 a = uColor; if(uHasTex==1) a *= texture(uTex,vU); if(uVCol==1) a.rgb *= vC; if(a.a<0.05) discard;
  vec3 c = a.rgb;
  if(uUnlit==0){ float d = dot(normalize(vN), uSun); float tone = clamp((0.30-d)/0.55,0.0,1.0);
    if(d<=0.12) c *= 0.73;
    vec2 g = mat2(0.7071,-0.7071,0.7071,0.7071) * (gl_FragCoord.xy/5.5); float r = length(fract(g)-0.5), R = 0.62*sqrt(tone), w = fwidth(r);
    c *= mix(1.0, 0.87, (1.0-smoothstep(R-w,R+w,r))*step(0.02,tone)); }
  o = vec4(pow(c, vec3(1.0/2.2)), a.a); })";
static const char* kHullVS = R"(#version 330 core
layout(location=0) in vec3 aP; layout(location=1) in vec3 aN; uniform mat4 uVP, uM; uniform float uW, uFadeA, uFadeB; uniform int uFade;
void main(){ float k = uFade==1 ? smoothstep(uFadeA,uFadeB,aP.y) : 1.0; gl_Position = uVP * uM * vec4(aP + aN*uW*k, 1.0); })";
static const char* kHullFS = R"(#version 330 core
out vec4 o; void main(){ o = vec4(0.05,0.04,0.07,1.0); })";
static const char* kFlatVS = R"(#version 330 core
layout(location=0) in vec3 aP; uniform mat4 uVP, uM; void main(){ gl_Position = uVP*uM*vec4(aP,1.0); })";
static const char* kFlatFS = R"(#version 330 core
out vec4 o; uniform vec3 uColor; void main(){ o = vec4(uColor,1.0); })";
void Renderer::init() {
    toon_.build(kToonVS, kToonFS); hull_.build(kHullVS, kHullFS); flat_.build(kFlatVS, kFlatFS);
    glEnable(GL_DEPTH_TEST); glEnable(GL_CULL_FACE);
    sun_ = glm::normalize(sun_);
    // lona: quadrado; cubo unitário: poste e cordas
    const float q[] = {-1,0,-1, 1,0,-1, 1,0,1, -1,0,-1, 1,0,1, -1,0,1};
    GLuint vbo; glGenVertexArrays(1,&ringVao_); glBindVertexArray(ringVao_); glGenBuffers(1,&vbo); glBindBuffer(GL_ARRAY_BUFFER,vbo); glBufferData(GL_ARRAY_BUFFER,sizeof q,q,GL_STATIC_DRAW); glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,nullptr); ringCount_ = 6;
    std::vector<float> c; const float s[6][3][3] = {{{1,0,0},{0,1,0},{0,0,1}},{{-1,0,0},{0,0,1},{0,1,0}},{{0,1,0},{0,0,1},{1,0,0}},{{0,-1,0},{1,0,0},{0,0,1}},{{0,0,1},{1,0,0},{0,1,0}},{{0,0,-1},{0,1,0},{1,0,0}}};
    for (auto& f : s) { auto v=[&](float a,float b){ for(int i=0;i<3;i++) c.push_back(f[0][i]+a*f[1][i]+b*f[2][i]); }; v(-1,-1);v(1,-1);v(1,1);v(-1,-1);v(1,1);v(-1,1); }
    glGenVertexArrays(1,&cubeVao_); glBindVertexArray(cubeVao_); glGenBuffers(1,&vbo); glBindBuffer(GL_ARRAY_BUFFER,vbo); glBufferData(GL_ARRAY_BUFFER,c.size()*4,c.data(),GL_STATIC_DRAW); glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,nullptr);
    glBindVertexArray(0);
}
void Renderer::begin(int w, int h, float a, const Fighter& pl) {
    glViewport(0,0,w,h); glClearColor(0.11f,0.09f,0.15f,1); glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    const float R = (-260.0f - (pl.radius - 60.0f)) * kRigUnit, Y = 220.0f * kRigUnit, lat = pl.offsetX * 0.6f * kRigUnit;
    // mesmas contas do jogo, com x trocado de sinal (ver rig.hpp)
    const glm::vec3 eye(-(R*std::sin(a) + lat*std::cos(a)), Y, R*std::cos(a) - lat*std::sin(a));
    const glm::vec3 fwd = glm::normalize(glm::vec3(-std::sin(a)*std::cos(0.45f), -std::sin(0.45f), std::cos(a)*std::cos(0.45f)));
    view_ = glm::lookAt(eye, eye + fwd, glm::vec3(0,1,0));
    proj_ = glm::perspective(glm::radians(50.0f), float(w)/float(h), 0.05f, 100.0f);
}
void Renderer::ring() {
    glDisable(GL_CULL_FACE); flat_.use(); flat_.set("uVP", proj_*view_);
    const float E = Physics3D_RING_METERS;
    auto box=[&](glm::vec3 c, glm::vec3 s, glm::vec3 col){ flat_.set("uM", glm::scale(glm::translate(glm::mat4(1.f),c),s)); flat_.set("uColor",col); glBindVertexArray(cubeVao_); glDrawArrays(GL_TRIANGLES,0,36); };
    flat_.set("uM", glm::scale(glm::mat4(1.f), glm::vec3(E,1,E))); flat_.set("uColor", glm::vec3(0.17f,0.42f,0.29f)); glBindVertexArray(ringVao_); glDrawArrays(GL_TRIANGLES,0,ringCount_);
    for (int sx : {-1,1}) for (int sz : {-1,1}) box({sx*E,0.7f,sz*E},{0.09f,0.7f,0.09f},{0.85f,0.75f,0.25f});
    for (float y : {0.55f,0.9f,1.25f}) for (int s : {-1,1}) { box({0,y,s*E},{E,0.025f,0.025f},{0.85f,0.2f,0.25f}); box({s*E,y,0},{0.025f,0.025f,E},{0.85f,0.2f,0.25f}); }
    glEnable(GL_CULL_FACE); glBindVertexArray(0);
}
void Renderer::apply(Model& m, const FighterPose& p) {
    Node& up = m.nodes[m.need("upper")]; up.pos = p.upperPos; up.rot = glm::quat(glm::vec3(p.lean, p.twist, p.roll));
    Node& ne = m.nodes[m.need("neck")]; ne.pos = glm::vec3(0, m.NECK - m.WAIST, 0); ne.rot = glm::quat(glm::vec3(-p.lean*0.5f + p.neckTilt, -p.twist*0.4f + p.neckTurn, 0));
    Node& hi = m.nodes[m.need("hips")]; hi.pos = p.hipsPos; hi.rot = glm::quat(glm::vec3(p.lean*0.45f, p.twist*0.5f, p.roll*0.55f));
    const char* LEG[2] = {"legL","legR"}, *BOOT[2] = {"bootL","bootR"}, *GL_[2] = {"gloveL","gloveR"};
    for (int i=0;i<2;i++) {
        Node& g = m.nodes[m.need(GL_[i])]; g.pos = p.glove[i];
        Node& b = m.nodes[m.need(BOOT[i])]; b.pos = p.footPos[i];
        Node& l = m.nodes[m.need(LEG[i])]; l.pos = p.legPos[i]; const float len = glm::length(p.legDirLen[i]);
        l.rot = glm::rotation(glm::vec3(0,-1,0), p.legDirLen[i]/len); l.scale = {1, len/m.legLen, 1};
    }
    m.updateWorld(p.root * glm::scale(glm::mat4(1.f), glm::vec3(m.scale)));
}
void Renderer::draw(const Node& n, bool hull) {
    for (const Prim& pr : n.prims) { glBindVertexArray(pr.vao);
        if (hull) { hull_.set("uM", n.world); hull_.set("uW", n.outlineW); hull_.set("uFade", n.fade?1:0); hull_.set("uFadeA", n.fadeA); hull_.set("uFadeB", n.fadeB); }
        else { toon_.set("uM", n.world); toon_.set("uColor", pr.color); toon_.set("uHasTex", pr.tex?1:0); toon_.set("uVCol", pr.vcol?1:0); toon_.set("uUnlit", pr.unlit?1:0); if (pr.tex) { glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, pr.tex); }
            if (pr.twoSided) glDisable(GL_CULL_FACE); else glEnable(GL_CULL_FACE); if (pr.blend) { glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA); glDepthMask(GL_FALSE); } }
        if (pr.indexed) glDrawElements(GL_TRIANGLES, pr.count, GL_UNSIGNED_INT, nullptr); else glDrawArrays(GL_TRIANGLES, 0, pr.count);
        if (!hull && pr.blend) { glDisable(GL_BLEND); glDepthMask(GL_TRUE); } }
}
void Renderer::fighter(Model& m, const FighterPose& p) {
    apply(m, p); const auto vis = [&](auto&& self, int i, auto&& fn) -> void { const Node& n = m.nodes[i]; if (!n.visible) return; fn(n); for (int k : n.kids) self(self, k, fn); };
    hull_.use(); hull_.set("uVP", proj_*view_); glCullFace(GL_FRONT); vis(vis, m.rootNode, [&](const Node& n){ if (n.outlineW > 0) draw(n, true); }); glCullFace(GL_BACK);
    toon_.use(); toon_.set("uVP", proj_*view_); toon_.set("uSun", sun_); toon_.set("uTex", 0); vis(vis, m.rootNode, [&](const Node& n){ draw(n, false); });
    glBindVertexArray(0);
}
}
```

Definir no topo de `renderer.cpp`, depois dos includes: `static constexpr float Physics3D_RING_METERS = 215.0f * kRigUnit * 1.15f;`, além de `#include <glm/gtx/quaternion.hpp>` com `#define GLM_ENABLE_EXPERIMENTAL` antes, por causa de `glm::rotation`, e `#include <cmath>`.

---

### Task 5: Jogo: laço, teclado, fluxo e HUD

**Files:**
- Create: `engine-cpp/app3d/game/main.cpp`
- Modify: `engine-cpp/CMakeLists.txt`

- [ ] **Step 1: Alvo**

```cmake
add_executable(korama3d app3d/game/main.cpp app3d/gl/shader.cpp app3d/model/model.cpp app3d/render/renderer.cpp)
target_include_directories(korama3d PRIVATE ${cgltf_SOURCE_DIR})
target_link_libraries(korama3d PRIVATE korama3d_rig SFML::Graphics SFML::Window SFML::System glad)
target_compile_definitions(korama3d PRIVATE KORAMA_ROOT="${CMAKE_SOURCE_DIR}/..")
if (MINGW)
    target_link_options(korama3d PRIVATE -static)
endif()
```

- [ ] **Step 2: `main.cpp`**

Teclas: setas movem e esquivam, `Z` e `X` socam, `Espaço` bloqueia, `C` esquiva, `Enter` revanche no fim, `Esc` sai. Passo fixo de 60 Hz, como `GameApp`. `--snap <saída> <quadro> <máscara>` simula sem teclado e grava PNG (Task 7).

```cpp
// app3d/game/main.cpp: Valéria contra Valéria em 3D. Toda regra vem de GameEngine (korama_core).
#include "render/renderer.hpp"
#include "engine.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <chrono>
#include <cstdio>
#include <exception>
#include <string>
using namespace KoRama;
namespace {
constexpr uint32_t kCharId = 4;   // Valéria
constexpr double kTick = 1.0 / 60.0;
uint32_t readInput() {
    using K = sf::Keyboard::Key; const auto h = [](K k){ return sf::Keyboard::isKeyPressed(k); }; uint32_t m = 0;
    if (h(K::Up)) m |= INPUT_UP; if (h(K::Down)) m |= INPUT_DOWN; if (h(K::Left)) m |= INPUT_LEFT; if (h(K::Right)) m |= INPUT_RIGHT;
    if (h(K::Z)) m |= INPUT_PUNCH_L; if (h(K::X)) m |= INPUT_PUNCH_R; if (h(K::Space)) m |= INPUT_BLOCK; if (h(K::C)) m |= INPUT_DODGE_KEY;
    return m;
}
RigRest restOf(const Model& m) {
    RigRest r; r.WAIST = m.WAIST; r.NECK = m.NECK; r.HIPS = m.HIPS; r.legLen = m.legLen;
    r.hip[0] = m.nodes[m.need("legL")].pos; r.hip[1] = m.nodes[m.need("legR")].pos;
    r.foot[0] = m.nodes[m.need("bootL")].pos; r.foot[1] = m.nodes[m.need("bootR")].pos;
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
    const auto text = [&](const std::string& s, unsigned size, float y){ sf::Text t(font, s, size); t.setOutlineColor(sf::Color::Black); t.setOutlineThickness(3); const auto b = t.getLocalBounds(); t.setPosition({(W-b.size.x)/2, y}); w.draw(t); };
    const int sec = e.roundFrames/60; char clock[32]; std::snprintf(clock, sizeof clock, "R%d  %d:%02d", e.round, sec/60, sec%60); text(clock, 30, 14);
    if (e.gameState == STATE_ROUND_INTRO) text("PREPARAR", 72, 240);
    if (e.gameState == STATE_ROUND_OVER) text("FIM DO ROUND", 64, 240);
    if (e.gameState == STATE_KNOCKDOWN) { text(std::to_string(e.refereeCount), 110, 200); if (!e.enemyIsDown) text("APERTE Z E X PARA LEVANTAR!", 34, 340); }
    if (e.isMatchOver()) { const MatchResult r = e.result; const bool win = r==MatchResult::WinKO||r==MatchResult::WinTKO||r==MatchResult::WinDecision; text(r==MatchResult::Draw ? "EMPATE" : win ? "VITORIA" : "DERROTA", 90, 200); text("ENTER: REVANCHE   ESC: SAIR", 28, 330); }
}
void startFight(GameEngine& e) { e.startMatch(kCharId, kCharId, false, 1); e.startRoundIntro(324); e.update(0, 0.f); }
float approach(float v, float t, float s) { return v < t ? std::min(t, v+s) : std::max(t, v-s); }
}
int main(int argc, char** argv) {
    try {
        sf::ContextSettings cs; cs.depthBits = 24; cs.majorVersion = 3; cs.minorVersion = 3; cs.attributeFlags = sf::ContextSettings::Core; cs.antiAliasingLevel = 4;
        sf::RenderWindow win(sf::VideoMode({1280, 720}), "K.O.-RAMA 3D (teste)", sf::Style::Default, sf::State::Windowed, cs);
        win.setVerticalSyncEnabled(true);
        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(sf::Context::getFunction))) throw std::runtime_error("OpenGL 3.3 indisponível");
        const std::string root = KORAMA_ROOT;
        sf::Font font; if (!font.openFromFile(root + "/engine-cpp/assets/fonts/Bangers-Regular.ttf")) throw std::runtime_error("fonte Bangers não abriu");
        Model mP, mE; mP.load(root + "/assets3d/game/valeria.glb"); mE.load(root + "/assets3d/game/valeria.glb");
        const RigRest rest = restOf(mP);
        Renderer rend; rend.init();
        GameEngine engine; startFight(engine);
        float downP = 0, downE = 0;
        const auto step = [&](uint32_t mask) { engine.update(mask, float(kTick));
            const bool over = engine.isMatchOver(), kd = engine.gameState == STATE_KNOCKDOWN;
            const bool pLost = over && (engine.result==MatchResult::LossKO||engine.result==MatchResult::LossTKO), eLost = over && (engine.result==MatchResult::WinKO||engine.result==MatchResult::WinTKO);
            downP = approach(downP, (kd && !engine.enemyIsDown) || pLost ? 1.f : 0.f, 0.06f); downE = approach(downE, (kd && engine.enemyIsDown) || eLost ? 1.f : 0.f, 0.06f); };
        const auto render = [&]() { win.setActive(true);
            glEnable(GL_DEPTH_TEST); rend.begin(int(win.getSize().x), int(win.getSize().y), engine.ringAngle, engine.player); rend.ring();
            rend.fighter(mP, computePose(engine.player, rest, downP)); rend.fighter(mE, computePose(engine.enemy, rest, downE));
            glBindVertexArray(0); glUseProgram(0); glDisable(GL_DEPTH_TEST); glDisable(GL_CULL_FACE);
            win.pushGLStates(); hud(win, font, engine); win.popGLStates(); };
        if (argc >= 5 && std::string(argv[1]) == "--snap") {   // --snap saida.png quadro mascara
            const int frames = std::stoi(argv[3]); const uint32_t mask = uint32_t(std::stoul(argv[4]));
            for (int i = 0; i < frames; i++) step(mask);
            render(); win.display(); render();
            sf::Texture t; if (!t.resize(win.getSize())) throw std::runtime_error("textura do snap"); t.update(win); if (!t.copyToImage().saveToFile(argv[2])) throw std::runtime_error(std::string("não salvou ") + argv[2]);
            return 0;
        }
        using clk = std::chrono::steady_clock; auto last = clk::now(); double acc = 0;
        while (win.isOpen()) {
            while (auto ev = win.pollEvent()) {
                if (ev->is<sf::Event::Closed>()) win.close();
                if (const auto* k = ev->getIf<sf::Event::KeyPressed>()) { if (k->code == sf::Keyboard::Key::Escape) win.close(); if (k->code == sf::Keyboard::Key::Enter && engine.isMatchOver()) { startFight(engine); downP = downE = 0; } }
                if (const auto* r = ev->getIf<sf::Event::Resized>()) win.setView(sf::View(sf::FloatRect({0,0}, sf::Vector2f(r->size))));
            }
            const auto now = clk::now(); acc += std::min(0.1, std::chrono::duration<double>(now-last).count()); last = now;
            while (acc >= kTick) { step(win.hasFocus() ? readInput() : 0); acc -= kTick; }
            render(); win.display();
        }
    } catch (const std::exception& e) {
        std::fprintf(stderr, "korama3d: %s\n", e.what());
        sf::RenderWindow box(sf::VideoMode({10,10}), "erro"); box.close();
        return 1;
    }
    return 0;
}
```

- [ ] **Step 3: Compilar**

Run: `cmake --preset release && cmake --build build/release --target korama3d`
Expected: `korama3d.exe` em `engine-cpp/build/release/`. Corrigir erros de compilação das Tasks 3 a 5 aqui.

---

### Task 6: Conferir que a lógica não mudou

- [ ] **Step 1: Rodar os testes**

Run: `cmake --build build/release --target korama_tests korama3d_rig_tests && ctest --test-dir build/release --output-on-failure`
Expected: `engine_rules` e `rig3d` passam.

- [ ] **Step 2: Conferir o diff**

Run: `git status --short engine-cpp/src engine-cpp/include`
Expected: nada novo em relação ao início desta etapa.

---

### Task 7: Conferência visual e calibragem

Sem teclas injetadas: tudo por `--snap`. Máscaras: nenhuma = 0; soco esquerdo (`INPUT_PUNCH_L`) = 16; bloqueio = 64.

- [ ] **Step 1: Gerar os quadros**

```bash
build/release/korama3d --snap snap3d_guarda.png 400 0
build/release/korama3d --snap snap3d_soco.png 420 16
build/release/korama3d --snap snap3d_bloqueio.png 420 64
build/release/korama3d --snap snap3d_longo.png 9000 16
```

- [ ] **Step 2: Olhar cada PNG e comparar com o vinil**

Conferir: jogador de costas e oponente de frente, os dois com os pés na lona, sombrinhas na altura do peito, rosto certo, contorno presente, cabelo e sombrinha com as cores do vinil, HUD legível. O `snap3d_longo.png` deve mostrar a luta evoluída (vida baixa, queda ou resultado).

- [ ] **Step 3: Calibrar**

Ajustar só constantes: `kRigUnit`, `kGloveLift`, sol, FOV da câmera, com novo snap a cada ajuste. O que estiver muito diferente do vinil por causa de shader perdido vira correção no `colorize` do exportador, com nova exportação.

- [ ] **Step 4: Medir o desempenho**

Abrir o `korama3d.exe`, registrar o tempo médio de quadro em log por 600 quadros (acrescentar `std::fprintf` temporário em `render`).
Expected: média abaixo de 16,6 ms. Remover o log.

- [ ] **Step 5: Entregar**

Passar ao usuário o caminho do executável, as teclas e os PNGs, listando o que ficou diferente do vinil. Commit dos arquivos desta etapa quando o usuário pedir. A decisão de continuar para a etapa 2 é dele.
