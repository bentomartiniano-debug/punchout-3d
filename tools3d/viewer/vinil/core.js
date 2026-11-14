// Motor comum do modo vinil: palco, materiais (desenho com faixas de luz, sombra de quadrinho com retícula, contorno
// por casca invertida), sólido por seções a partir do sprite, leitura do atlas, animações (parada, jab, carregar,
// soco carregado), movimentação (inclina, salta, passada) e o laço. Cada personagem (valeria.js, frog.js...) só
// constrói as peças e diz onde ficam as articulações.
import * as THREE from 'three';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';

export { THREE };
export const ss = (a, b, x) => { const t = Math.max(0, Math.min(1, (x - a) / (b - a))); return t * t * (3 - 2 * t); };
export const lin = c => c.map(v => Math.pow(v, 2.2));   // cor de vértice é linear; as cores dos sprites são sRGB
export function rrect(g, x, y, w, h, r) { g.beginPath(); g.moveTo(x + r, y); g.arcTo(x + w, y, x + w, y + h, r); g.arcTo(x + w, y + h, x, y + h, r); g.arcTo(x, y + h, x, y, r); g.arcTo(x, y, x + w, y, r); g.closePath(); }
export const INK = 0x0f1c10, INKS = '#0f1c10';
const $ = s => document.querySelector(s);

// --- materiais: desenho com 3 faixas de luz + contorno grosso (casco invertido)
export const ramp = (() => { const d = new Uint8Array([160, 160, 160, 255, 220, 220, 220, 255, 255, 255, 255, 255]); const t = new THREE.DataTexture(d, 3, 1); t.magFilter = t.minFilter = THREE.NearestFilter; t.needsUpdate = true; return t; })();
export const flat = c => new THREE.MeshToonMaterial({ color: c, gradientMap: ramp });
export const vcol = () => new THREE.MeshToonMaterial({ vertexColors: true, gradientMap: ramp });
// sombra de quadrinho (a mesma do visualizador principal): dois tons com corte duro e retícula em tela, a 45°,
// com pontos que crescem rumo à sombra. Vai em todo material com luz, por cima do que ele já faz.
export const comic = { cell: { value: 5.5 }, shade: { value: 0.73 }, dots: { value: 0.87 }, tint: { value: new THREE.Color(1, 1, 1) } };   // tint: cor da sombra (branco = cinza neutro)
export function comicShader(mat) {
  if (mat.userData.comic) return mat;   // material dividido entre peças: uma vez só
  mat.userData.comic = true;
  const prev = mat.onBeforeCompile, key = mat.customProgramCacheKey();
  mat.onBeforeCompile = (sh, r) => {
    prev.call(mat, sh, r);
    Object.assign(sh.uniforms, { uCell: comic.cell, uShade: comic.shade, uDots: comic.dots, uShadeTint: comic.tint });
    sh.fragmentShader = 'uniform float uCell, uShade, uDots; uniform vec3 uShadeTint;\nfloat gTone = 0.0, extraTone = 0.0, gSh = 1.0;\n' + sh.fragmentShader
      .replace('#include <lights_fragment_begin>', THREE.ShaderChunk.lights_fragment_begin.replace('directLight.color *= ( directLight.visible && receiveShadow ) ? getShadow(', 'gSh = ( directLight.visible && receiveShadow ) ? getShadow('))   // sombra projetada vira um número (gSh), não escurece a luz
      .replace('#include <gradientmap_pars_fragment>', `
        vec3 getGradientIrradiance(vec3 normal, vec3 lightDirection) {
          float d = dot(normal, lightDirection), inShadow = 1.0 - step(0.5, gSh);   // gSh: sombra projetada (1 = sol)
          gTone = max(gTone, max(clamp((0.30 - d) / 0.55, 0.0, 1.0), 0.85 * inShadow));   // 0 = luz, 1 = sombra
          return (d > 0.12 && inShadow < 0.5) ? vec3(1.0) : uShade * uShadeTint;   // corte duro; a sombra pode ter cor (ex.: lilás do sol forte)
        }`)
      .replace('#include <opaque_fragment>', `
        outgoingLight *= mix(vec3(1.0), uShade * uShadeTint, extraTone); gTone = max(gTone, extraTone);   // sombra pedida pela peça (ex.: embaixo do babado)
        vec2 hg = mat2(0.7071, -0.7071, 0.7071, 0.7071) * (gl_FragCoord.xy / uCell);
        float hr = length(fract(hg) - 0.5), dotR = 0.62 * sqrt(gTone);   // pontos crescem rumo à sombra
        float hw = fwidth(hr);
        outgoingLight *= mix(vec3(1.0), uDots * uShadeTint, (1.0 - smoothstep(dotR - hw, dotR + hw, hr)) * step(0.02, gTone));
        #include <opaque_fragment>`);
  };
  mat.customProgramCacheKey = () => key + '|comic';
  mat.needsUpdate = true;
  return mat;
}
export function withOutline(mesh, w = 0.014, fadeY = null) {   // fadeY [y0, y1]: abaixo de y0 a casca some (sem anel preto onde a peça entra em outra)
  const m = new THREE.MeshBasicMaterial({ color: INK, side: THREE.BackSide });
  const k = fadeY ? ' * smoothstep(' + fadeY[0].toFixed(4) + ', ' + fadeY[1].toFixed(4) + ', position.y)' : '';
  m.onBeforeCompile = sh => { sh.vertexShader = sh.vertexShader.replace('#include <begin_vertex>', '#include <begin_vertex>\ntransformed += normal * ' + w.toFixed(4) + k + ';'); };
  if (fadeY) m.customProgramCacheKey = () => 'fade' + fadeY.join();
  const hull = new THREE.Mesh(mesh.geometry, m); hull.userData.hull = true; hull.userData.outlineW = w; hull.userData.outlineFade = fadeY;   // o exportador (tools3d/export) lê a espessura
  const g = new THREE.Group(); g.add(mesh, hull); return g;
}

// unidades: medidas em px do quadro do sprite; S px = 1 m, FOOT = linha do chão, CX = centro do quadro
export function units(S, FOOT, CX) {
  const U = { S, FOOT, CX, X: x => (x - CX) / S, Y: y => (FOOT - y) / S, D: px => px / S };
  // pinta uma peça por projeção de frente (em px do quadro): canvas da frente onde a normal olha para a câmera, da
  // costas atrás (espelhado, como o sprite de costas), mistura curta nas laterais
  U.paintMaterial = (texF, texB, box) => {
    const m = new THREE.MeshToonMaterial({ color: 0xffffff, gradientMap: ramp });
    m.onBeforeCompile = sh => {
      Object.assign(sh.uniforms, { tPF: { value: texF }, tPB: { value: texB }, uPBox: { value: new THREE.Vector4(...box) }, uPU: { value: new THREE.Vector3(S, FOOT, CX) } });
      sh.vertexShader = 'varying vec3 vPP; varying vec3 vPN;\n' + sh.vertexShader.replace('#include <begin_vertex>', '#include <begin_vertex>\nvPP = position; vPN = normal;');
      sh.fragmentShader = 'uniform sampler2D tPF, tPB; uniform vec4 uPBox; uniform vec3 uPU; varying vec3 vPP; varying vec3 vPN;\n' + sh.fragmentShader
        .replace('#include <map_fragment>', `#include <map_fragment>
          vec2 pp = vec2(vPP.x * uPU.x + uPU.z, uPU.y - vPP.y * uPU.x);           // px do quadro
          vec2 puv = vec2((pp.x - uPBox.x) / (uPBox.z - uPBox.x), 1.0 - (pp.y - uPBox.y) / (uPBox.w - uPBox.y));
          float fr = smoothstep(-0.12, 0.12, normalize(vPN).z);
          diffuseColor.rgb *= mix(texture2D(tPB, vec2(1.0 - puv.x, puv.y)).rgb, texture2D(tPF, puv).rgb, fr);`);
    };
    m.customProgramCacheKey = () => 'paint';
    return m;
  };
  // sólido por seções: cada linha = elipse (meia largura a, profundidade a·depth, centro z) ou superelipse (pw > 2 =
  // mais quadrado). zFn deixa mexer em cada ponto.
  U.loft = (rows, { depth = 0.8, cz = 0, NA = 40, capTop = true, capBot = true, zFn = null, colorFn = null, pw = 2 }) => {
    rows = rows.slice();
    const sp = v => Math.sign(v) * Math.pow(Math.abs(v), 2 / pw);
    const close = (src, dir) => { const a = (src.r - src.l) / 2, c = (src.l + src.r) / 2, h = Math.min(a * depth, a) * 0.9, out = [];
      for (let k = 1; k <= 5; k++) { const f = Math.cos(k / 5 * Math.PI / 2); out.push({ y: src.y + dir * h * Math.sin(k / 5 * Math.PI / 2), l: c - a * Math.max(f, 0.01), r: c + a * Math.max(f, 0.01) }); } return out; };
    if (capTop) rows = [...close(rows[0], -1).reverse(), ...rows];
    if (capBot) rows = [...rows, ...close(rows[rows.length - 1], 1)];
    const pos = [], col = [], idx = [];
    for (const row of rows) {
      const a = (row.r - row.l) / 2, c = (row.l + row.r) / 2;
      for (let j = 0; j <= NA; j++) {
        const th = j / NA * Math.PI * 2, x = c + a * sp(Math.cos(th));
        let z = cz + a * depth * sp(Math.sin(th));
        if (row.zf) z = row.zf(x, row.y, z, Math.sin(th)); else if (zFn) z = zFn(x, row.y, z, Math.sin(th));
        pos.push(U.X(x), U.Y(row.y), U.D(z));
        if (colorFn) col.push(...colorFn(x, row.y, z, Math.sin(th)));
      }
    }
    for (let i = 0; i < rows.length - 1; i++) for (let j = 0; j < NA; j++) { const a = i * (NA + 1) + j, b = a + 1, c = a + NA + 1, d = c + 1; idx.push(a, b, c, b, d, c); }
    const g = new THREE.BufferGeometry(); g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3)); g.setIndex(idx);
    if (colorFn) g.setAttribute('color', new THREE.Float32BufferAttribute(col, 3));
    g.computeVertexNormals(); return g;
  };
  return U;
}

// --- sprite: atlas do jogo (engine-cpp/assets/sprites/<nome>.png + .atlas) e ferramentas para ler o desenho
export const SPR = '../../engine-cpp/assets/sprites/';
export async function loadSprite(name, size) {
  const img = await new Promise((ok, bad) => { const i = new Image(); i.onload = () => ok(i); i.onerror = bad; i.src = SPR + name + '.png'; });
  const atlas = {};
  for (const l of (await (await fetch(SPR + name + '.atlas')).text()).split('\n')) {
    const a = l.trim().split(/\s+/); if (a.length < 5 || a[0].startsWith('#')) continue;
    const n = a.slice(1).map(Number); atlas[a[0]] = { x: n[0], y: n[1], w: n[2], h: n[3], ox: n[4] || 0, oy: n[5] || 0 };
  }
  const N = size;
  const frameCanvas = names => {
    const c = document.createElement('canvas'); c.width = c.height = N; const g = c.getContext('2d', { willReadFrequently: true });
    for (const n of [].concat(names)) { const a = atlas[n]; if (a) g.drawImage(img, a.x, a.y, a.w, a.h, a.ox, a.oy, a.w, a.h); }
    return c;
  };
  const pix = c => c.getContext('2d', { willReadFrequently: true }).getImageData(0, 0, N, N).data;
  // limpa a cor: tira o que não passa no teste e estende as cores vizinhas por cima (sem contorno preto nem rosto no cabelo)
  const cleanFill = (c, keep, passes = 60) => {
    const d = pix(c), p = new Uint8ClampedArray(d);
    for (let i = 0; i < p.length; i += 4) if (!keep(p[i], p[i + 1], p[i + 2], p[i + 3])) p[i + 3] = 0;
    for (let k = 0; k < passes; k++) {
      const q = new Uint8ClampedArray(p);
      for (let y = 1; y < N - 1; y++) for (let x = 1; x < N - 1; x++) {
        const i = (y * N + x) * 4; if (q[i + 3]) continue;
        for (const j of [i - 4, i + 4, i - 4 * N, i + 4 * N]) if (q[j + 3]) { p[i] = q[j]; p[i + 1] = q[j + 1]; p[i + 2] = q[j + 2]; p[i + 3] = 255; break; }
      }
    }
    return p;
  };
  const rowsOf = (c, test, y0 = 0, y1 = N) => {
    const d = pix(c), rows = [];
    for (let y = y0; y < y1; y += 3) { let l = -1, r = -1; for (let x = 0; x < N; x++) { const i = (y * N + x) * 4; if (test(d[i], d[i + 1], d[i + 2], d[i + 3])) { if (l < 0) l = x; r = x; } } if (l >= 0 && r - l > 4) rows.push({ y, l, r }); }
    for (let k = 1; k < rows.length - 1; k++) { rows[k].L = (rows[k - 1].l + 2 * rows[k].l + rows[k + 1].l) / 4; rows[k].R = (rows[k - 1].r + 2 * rows[k].r + rows[k + 1].r) / 4; }
    for (const r of rows) { r.l = r.L ?? r.l; r.r = r.R ?? r.r; }
    return rows;
  };
  return { img, atlas, frameCanvas, pix, cleanFill, rowsOf };
}

// --- pernas presas no quadril: a perna vai do ponto do quadril (preso no grupo do quadril) até o tornozelo, onde está o pé
const DOWN = new THREE.Vector3(0, -1, 0);
export function legPlacer({ legs, feet, hipX, top, bot, hips, hipsY }) {
  window.__legPlacers?.push({ legs, feet, top, bot });   // tools3d/export lê as pernas daqui
  const F0 = { [-1]: feet[-1].position.clone(), [1]: feet[1].position.clone() };
  return (side, dx, lift) => {
    const leg = legs[side], foot = feet[side], f0 = F0[side];
    foot.position.set(f0.x + dx, f0.y + lift, f0.z);
    const a = new THREE.Vector3(hipX[side], top - hipsY, 0).applyMatrix4(hips.matrix), b = new THREE.Vector3(hipX[side] + dx, bot + lift, 0);
    const dir = b.clone().sub(a), len = dir.length();
    leg.position.copy(a); leg.quaternion.setFromUnitVectors(DOWN, dir.divideScalar(len)); leg.scale.y = len / (top - bot);
  };
}

// ================================================================ palco, HUD, animações e laço
// char: { title, size, units: [S, FOOT, CX], faces: [[chave, rótulo]], hudExtra, sliders, scale (tamanho no ringue; o rig pode trocar com rig.scale), build(ctx) }
export async function runVinil(char) {
  const hud = $('#hud');
  const sliders = [['cell', 'Retícula (tamanho)', 3, 14, 0.5, comic.cell], ['dots', 'Pontos (força)', 0, 1, 0.01, comic.dots], ['shade', 'Sombra (tom)', 0.3, 1, 0.01, comic.shade], ...(char.sliders || [])];
  // painel do lutador (a troca de lutador e de cenário fica na barra de cima, vinil.html)
  hud.innerHTML = `<b>${char.title}</b>
    <div class="row"><b>Rosto</b>${(char.faces || []).map(([k, l]) => `<button data-face="${k}">${l}</button>`).join('')}</div>
    ${char.hudExtra ? `<div class="row">${char.hudExtra}</div>` : ''}
    <div class="row"><b>Golpes</b><button data-jab="-1">Jab E</button><button data-jab="1">Jab D</button><button id="charge">Carregar (segure)</button></div>
    ${char.skills ? `<div class="row"><b>Especiais</b>${char.skills.map(([k, l]) => `<button data-skill="${k}">${l}</button>`).join('')}</div>` : ''}
    <div class="row"><b>Inclina</b><button data-dodge="L:lean">◀</button><button data-dodge="B:lean">trás</button><button data-dodge="R:lean">▶</button>
      <b style="min-width:0;margin-left:6px">Salta</b><button data-dodge="L:hop">◀</button><button data-dodge="B:hop">trás</button><button data-dodge="R:hop">▶</button></div>
    <div class="row"><b>Passos</b><button data-step="-1">◀ Passo</button><button data-step="1">Passo ▶</button><button id="stepLoop">Vai e volta</button></div>
    <div class="row" id="camRow"><b>Câmera</b><button data-cam="0,1.0,4.2">Frente</button><button data-cam="3,1.2,3">3/4</button><button data-cam="4.2,1.0,0">Lado</button><button data-cam="-2,1.2,-3.6">Costas</button></div>
    <details id="look"><summary>Visual (retícula)</summary>
    ${sliders.map(([id, l, a, b, st, u]) => `<label>${l} <input id="${id}" type="range" min="${a}" max="${b}" step="${st}" value="${u.value}"><span></span></label>`).join('<br>')}
    </details><div id="status">Carregando…</div>`;

  $('#status').textContent = 'Iniciando renderização 3D…';
  const CAM = (new URLSearchParams(location.search).get('cam') || '3,1.2,3').split(',').map(Number);
  const renderer = new THREE.WebGLRenderer({ antialias: true });
  renderer.setPixelRatio(Math.min(2, devicePixelRatio)); renderer.setSize(innerWidth, innerHeight);
  renderer.outputColorSpace = THREE.SRGBColorSpace; document.body.appendChild(renderer.domElement);
  const scene = new THREE.Scene(); scene.background = new THREE.Color(0x1d1626);
  const camera = new THREE.PerspectiveCamera(32, innerWidth / innerHeight, 0.05, 50); camera.position.set(...CAM);
  const controls = new OrbitControls(camera, renderer.domElement); controls.target.set(0, 0.85, 0); controls.update();
  addEventListener('resize', () => { renderer.setSize(innerWidth, innerHeight); camera.aspect = innerWidth / innerHeight; camera.updateProjectionMatrix(); });
  const hemi = new THREE.HemisphereLight(0xffffff, 0x584868, 1.7); scene.add(hemi);
  const sun = new THREE.DirectionalLight(0xffffff, 1.7); sun.position.set(1.2, 3, 3); scene.add(sun);
  const floor = new THREE.Mesh(new THREE.CircleGeometry(1.1, 48), new THREE.MeshBasicMaterial({ color: 0x2c6b4a })); floor.rotation.x = -Math.PI / 2; scene.add(floor);
  // ?arena=<nome>: a arena em volta do personagem (piso do ringue/plataforma em y = 0); câmera "Ringue" como a do jogo
  let arenaShadows = false;
  const ctxSkin = { light: null };   // luz da arena para a pele chapada dos rostos
  // sem ?arena= cada lutador abre na arena dele (char.arena), se já tiver uma; ?arena=nenhuma mostra só o disco
  const ARENA = new URLSearchParams(location.search).get('arena') ?? char.arena;
  const ARENAS = {
    pernambuco: async () => {
      const { buildPernambucoRing } = await import('./arena_pernambuco.js');
      const { buildPernambucoProps } = await import('./arena_pernambuco_props.js');
      const { buildPernambucoBackground } = await import('./arena_pernambuco_bg.js');
      return [buildPernambucoRing(), buildPernambucoProps(), buildPernambucoBackground()];
    },
    joana: async () => [await (await import('./arena_joana.js')).buildJoanaArena()],
    marcov: async () => [await (await import('./arena_marcov.js')).buildMarcovArena()],
    vivaldi: async () => [await (await import('./arena_vivaldi.js')).buildVivaldiArena()],
    frog: async () => [await (await import('./arena_frog.js')).buildFrogArena()],
    cremoso: async () => [await (await import('./arena_cremoso.js')).buildCremosoArena()],
    espinha: async () => [await (await import('./arena_espinha.js')).buildEspinhaArena()],
    jack: async () => [await (await import('./arena_jack.js')).buildJackArena()],
    batalhator: async () => [await (await import('./arena_batalhator.js')).buildBatalhatorArena()],
  };
  if (ARENAS[ARENA]) {
    $('#status').textContent = 'Montando arena…';
    // (sombra projetada do sol testada e desligada: pesava e não acrescentava; o shader de quadrinho continua aceitando)
    let objs;
    try { objs = await ARENAS[ARENA](); }
    catch (error) { $('#status').textContent = 'Erro na arena: ' + (error?.message || String(error)); throw error; }
    scene.add(...objs); floor.visible = false; camera.far = 200; camera.updateProjectionMatrix();
    // cada arena pode trazer o seu clima em userData.look (cor da sombra, céu, sol, luz do céu e o disco do sol no céu);
    // sem isso fica o meio-dia de festa: sombra lilás, céu do azul fundo ao creme no horizonte. Névoa na cor do
    // horizonte só a partir de 40 m (dissolve morros e mar ao longe; não toca os lutadores)
    const look = Object.assign({ tint: [0.9, 0.84, 1.1], horizon: '#f6ead2', zenith: '#2f8ee0' }, objs[0].userData.look);
    comic.tint.value.set(...look.tint);
    if (look.sun) { sun.color.set(look.sun.color); sun.intensity = look.sun.int; sun.position.set(...look.sun.pos); }
    if (look.hemi) { hemi.color.set(look.hemi.sky); hemi.groundColor.set(look.hemi.ground); hemi.intensity = look.hemi.int; }
    const HORIZON = new THREE.Color(look.horizon), ZENITH = new THREE.Color(look.zenith);
    scene.background = HORIZON.clone(); scene.fog = new THREE.Fog(HORIZON, ...(look.fog || [40, 150]));
    const sky = new THREE.Mesh(new THREE.SphereGeometry(180, 32, 16), new THREE.ShaderMaterial({ side: THREE.BackSide, depthWrite: false, fog: false,
      uniforms: { uH: { value: HORIZON }, uZ: { value: ZENITH }, uS: { value: new THREE.Vector3(...(look.disc || [0, 1, 0])).normalize() }, uSC: { value: new THREE.Color(look.discColor || '#ffffff') }, uSD: { value: look.disc ? 1 : 0 } },
      vertexShader: 'varying vec3 vD; void main() { vD = normalize(position); vec4 p = modelViewMatrix * vec4(position, 1.0); gl_Position = projectionMatrix * p; }',
      fragmentShader: `uniform vec3 uH, uZ, uS, uSC; uniform float uSD; varying vec3 vD;
        void main() { vec3 d = normalize(vD); float t = smoothstep(-0.02, 0.45, d.y); vec3 c = mix(uH, uZ, pow(t, 0.8));
          float s = max(dot(d, uS), 0.0) * uSD;   // sol: halo largo e disco de borda dura
          c = mix(c, uSC, pow(s, 14.0) * 0.55); c = mix(c, vec3(1.0, 0.98, 0.9), smoothstep(0.99962, 0.99974, s));
          gl_FragColor = vec4(c, 1.0); }` }));
    sky.renderOrder = -1; scene.add(sky);
    // a pele chapada (sem luz) dos rostos acompanha a luz da arena: a luz do céu mais a do sol, comparadas com as do
    // visualizador neutro (as duas a 1,7), um pouco puxada para o branco para o rosto continuar legível
    const skinLight = hemi.color.clone().multiplyScalar(hemi.intensity).add(sun.color.clone().multiplyScalar(sun.intensity)).multiplyScalar(1 / 3.4);
    skinLight.lerp(new THREE.Color(1, 1, 1), 0.25); skinLight.setRGB(Math.min(1, skinLight.r), Math.min(1, skinLight.g), Math.min(1, skinLight.b)); ctxSkin.light = skinLight;   // nunca clareia   // cúpula de 180 m em volta da cena (a câmera fica sempre perto do meio)
    const ringCam = () => { camera.fov = 50; camera.updateProjectionMatrix(); camera.position.set(0, 2.4, 8.6); controls.target.set(0, 2.6, -8); controls.update(); };   // como no concept: ringue embaixo, ponte, morro e igreja no alto
    const b = document.createElement('button'); b.textContent = 'Ringue'; b.onclick = ringCam;
    document.querySelector('[data-cam]').before(b);
    ringCam();
  }
  document.querySelectorAll('[data-cam]').forEach(b => b.onclick = () => { camera.position.set(...b.dataset.cam.split(',').map(Number)); controls.target.set(0, 0.85, 0); controls.update(); });

  const root = new THREE.Group(); scene.add(root);
  const clock = new THREE.Clock();
  const U = units(...char.units);
  const ctx = { THREE, scene, root, clock, U, $ };
  // rig: { P, upper, WAIST, neck, NECK, hips, HIPS, hands: {-1, 1}, hand: {x, y, z, yaw}, placeFoot, setFace,
  //        updateHand?(u, sx, hp, c), onFrame?(c) }
  const rig = await char.build(ctx);
  if (ctxSkin.light) root.traverse(o => { const ms = o.material ? [].concat(o.material) : []; for (const m of ms) if (m.userData && m.userData.flatSkin && m.color) m.color.copy(ctxSkin.light).multiply(new THREE.Color(m.color.r, m.color.g, m.color.b)); });
  const { upper, WAIST, neck, NECK, hips, HIPS } = rig;

  // --- animações: parada (respira, ginga), jab de cada lado (estocada), carregando (puxa a mão para trás, treme,
  // calor sobe) e soco carregado ao soltar
  let anim = { kind: 'idle', t0: 0, side: 1, power: 0 }, faceNow = 'normal';
  const setFace = f => { if (f === faceNow) return; faceNow = f; rig.setFace(f); };
  document.querySelectorAll('[data-face]').forEach(b => b.onclick = () => setFace(b.dataset.face));
  const toIdle = t => { anim = { kind: 'idle', t0: t, side: 1, power: 0 }; };
  const startJab = side => { if (anim.kind === 'idle' || anim.kind === 'jab') anim = { kind: 'jab', t0: clock.elapsedTime, side, power: 0 }; };
  const startCharge = () => { if (anim.kind !== 'charge' && anim.kind !== 'magma') anim = { kind: 'charge', t0: clock.elapsedTime, side: 1, power: 0 }; };
  const release = () => { if (anim.kind === 'charge') anim = { kind: 'magma', t0: clock.elapsedTime, side: anim.side, power: Math.max(0.3, anim.power) }; };
  // especiais do lutador: rig.skills[id] = { dur, start(t), pose(k, pose) -> { twist, lean, drop, tilt } }
  const startSkill = id => { if (anim.kind === 'idle' && rig.skills?.[id]) { anim = { kind: 'skill', id, t0: clock.elapsedTime, side: 1, power: 0 }; rig.skills[id].start?.(clock.elapsedTime); } };
  document.querySelectorAll('[data-skill]').forEach(b => b.onclick = () => startSkill(b.dataset.skill));
  document.querySelectorAll('[data-jab]').forEach(b => b.onclick = () => startJab(+b.dataset.jab));
  const cb = $('#charge'); cb.onpointerdown = startCharge; cb.onpointerup = cb.onpointerleave = release;
  addEventListener('keydown', e => { if (e.repeat) return; if (e.key === 'q') startJab(-1); if (e.key === 'e') startJab(1); if (e.key === ' ') startCharge(); });
  addEventListener('keyup', e => { if (e.key === ' ') release(); });
  // --- movimentação (lados vistos da câmera de frente: ◀ = esquerda da tela). Tempos do jogo (engine.cpp startDodge):
  //  inclina = 22 quadros, pés plantados, só o tronco e a cabeça saem da linha do golpe;
  //  salta   = 40 quadros: agacha, sai do chão, amortece e volta para o centro;
  //  passada = 22 quadros: o pé do lado vai primeiro, o corpo vai atrás e o outro pé fecha
  const DODGE = { lean: 22 / 60, hop: 40 / 60 }, STEP_T = 22 / 60, STEP_D = 0.32, STEP_MAX = 0.65;
  let move = null, baseX = 0, stepLoop = false, stepDir = 1;
  const startDodge = spec => { if (move) return; const [dir, mode] = spec.split(':'); move = { kind: mode, dir, t0: clock.elapsedTime, dur: DODGE[mode] }; };
  const startStep = dir => {
    if (move) return;
    if (Math.abs(baseX + dir * STEP_D) > STEP_MAX) dir = -dir;   // não sai do tapete: volta
    move = { kind: 'step', sx: dir, t0: clock.elapsedTime, dur: STEP_T, x0: baseX };
  };
  document.querySelectorAll('[data-dodge]').forEach(b => b.onclick = () => startDodge(b.dataset.dodge));
  document.querySelectorAll('[data-step]').forEach(b => b.onclick = () => { stepLoop = false; $('#stepLoop').style.outline = ''; startStep(+b.dataset.step); });
  $('#stepLoop').onclick = () => { stepLoop = !stepLoop; $('#stepLoop').style.outline = stepLoop ? '2px solid #fc6' : ''; };
  addEventListener('keydown', e => { if (e.repeat) return; const m = { a: 'L:lean', d: 'R:lean', s: 'B:lean', z: 'L:hop', c: 'R:hop', x: 'B:hop' }[e.key]; if (m) startDodge(m); if (e.key === 'ArrowLeft') startStep(-1); if (e.key === 'ArrowRight') startStep(1); });

  $('#status').textContent = 'Pronto · arraste para girar';
  scene.traverse(o => { if (o.isMesh && o.material.isMeshToonMaterial) { comicShader(o.material); if (arenaShadows) { o.castShadow = true; o.receiveShadow = true; } } });
  for (const [id, , , , , u] of sliders) {
    const el = $('#' + id), sp = el.nextElementSibling, f = () => { u.value = +el.value; sp.textContent = el.value; };
    el.oninput = f; f();
  }
  window.VIN = { comic, THREE, scene, camera, renderer, controls, P: rig.P, rig, root, upper, neck, hips, skirt: hips, clock, get anim() { return anim; }, get move() { return move; }, startJab, startCharge, release, startSkill, startDodge, startStep };

  const easeO = k => 1 - (1 - k) ** 3;
  const M = px => px / 245;   // distâncias do movimento em metros (px do quadro da Valéria): igual para todos os personagens
  const H = rig.hand;
  // mão no lugar: base do personagem + avanço/subida/para o meio do golpe, acompanhando o tronco (inclina/desloca)
  const placeHand = (u, sx, hp, c, thr) => {
    const [, up, mid] = hp;
    const ry = H.yaw * sx * (1 - mid), UH = H.y - WAIST;
    u.rotation.set(-up * 1.2, ry, 0);
    u.position.set(sx * H.x * (1 - 0.45 * mid) + Math.sin(ry) * thr + c.shX - Math.sin(c.roll) * UH, H.y + c.ph[sx] * 0.02 + up + c.bob * 0.5 - c.drop * 0.7, H.z + Math.cos(ry) * thr + c.shZ + Math.sin(c.lean) * UH);
  };
  const updateHand = rig.updateHand || ((u, sx, hp, c) => c.placeHand(u, sx, hp, c, hp[0]));
  // um quadro; VIN.frame(dt) avança na mão (testes e fotos com a aba em segundo plano, onde o navegador para o laço)
  const frame = (dtIn = null) => {
    let dt = Math.min(0.05, clock.getDelta()); if (dtIn != null) { clock.elapsedTime += dtIn; dt = dtIn; }
    const t = clock.elapsedTime;
    // pose do quadro: tronco (torce / inclina / agacha), cabeça, mãos e calor
    let twist = 0, lean = 0, drop = 0, tilt = 0, heat = 0;
    const pose = { [-1]: [0, 0, 0, 0, 0], [1]: [0, 0, 0, 0, 0] };   // por lado: avanço, subida, para o meio, abrir, giro
    const k = t - anim.t0;
    if (anim.kind === 'jab') {                              // 0,42 s: recua um tico, estoca, segura, volta
      const T = 0.42, s = anim.side;
      const wind = ss(0, 0.07, k) * (1 - ss(0.07, 0.13, k)), hit = ss(0.07, 0.14, k) * (1 - ss(0.22, T, k));
      pose[s] = [-0.08 * wind + 0.5 * easeO(hit), 0.03 * wind + 0.04 * hit, 0.35 * hit, 0, 0];
      twist = -s * (0.08 * wind - 0.28 * hit); lean = 0.1 * hit; tilt = s * 0.06 * hit;
      if (k > T) toIdle(t);
    } else if (anim.kind === 'charge') {                    // enche em 1,2 s
      const c = Math.min(1, k / 1.2), s = anim.side, e = easeO(c); anim.power = c;
      const shake = Math.sin(t * 45) * 0.006 * c;
      pose[s] = [-0.28 * e + shake, 0.14 * e, -0.1 * c, 0, 0.12 * c];
      pose[-s] = [0.05 * c, 0.02 * c, 0.1 * c, 0, 0];
      twist = s * 0.35 * e + shake * 3; lean = -0.06 * c; drop = M(10) * e; heat = c; tilt = -s * 0.05 * c;
      setFace('angry');
    } else if (anim.kind === 'magma') {                     // solta: avança mais quanto mais carregou, abre e gira na batida
      const T = 0.85, s = anim.side, p = anim.power;
      const hit = ss(0, 0.1, k) * (1 - ss(0.45, T, k)), pre = 1 - ss(0, 0.08, k);
      pose[s] = [-0.28 * pre + (0.45 + 0.35 * p) * easeO(hit), 0.14 * pre + 0.06 * hit, 0.4 * hit, ss(0.06, 0.16, k) * (1 - ss(0.5, 0.8, k)), 0.35 * hit];
      twist = s * 0.35 * pre - s * 0.45 * hit; lean = 0.18 * hit; drop = M(10) * pre; heat = p * (1 - ss(0.1, 0.7, k));
      if (k > T) { toIdle(t); setFace('normal'); }
    }
    if (anim.kind === 'skill') {
      const sk = rig.skills[anim.id], o = sk.pose(k, pose) || {};
      twist = o.twist || 0; lean = o.lean || 0; drop = o.drop || 0; tilt = o.tilt || 0;
      if (k > sk.dur) toIdle(t);
    }
    // movimentação: inclina (roll / para trás), salto e passada
    let roll = 0, shX = 0, shZ = 0, lift = 0, rootX = baseX, rootZ = 0;
    const feet = { [-1]: [0, 0], [1]: [0, 0] };              // por pé: desloca x, sobe
    if (!move && stepLoop) { startStep(stepDir); if (move) stepDir = -move.sx; }
    if (move) {
      const p = Math.min(1, (t - move.t0) / move.dur), arc = x => Math.sin(Math.PI * Math.min(1, Math.max(0, x)));
      const sx = move.dir === 'L' ? -1 : move.dir === 'R' ? 1 : 0, back = move.dir === 'B';
      if (move.kind === 'lean') {                             // sai rápido (35%), segura e volta macio
        const e = p < 0.35 ? ss(0, 0.35, p) : p < 0.55 ? 1 : 1 - ss(0.55, 1, p);
        if (back) { lean -= 0.34 * e; drop += M(10) * e; shZ = -0.08 * e; }
        else { roll = -sx * 0.42 * e; shX = sx * 0.12 * e; drop += M(12) * e; tilt -= sx * 0.12 * e; }
      } else if (move.kind === 'hop') {
        const dist = back ? 0.45 : 0.5;
        const crouch = ss(0, 0.2, p) * (1 - ss(0.2, 0.3, p)), air = ss(0.24, 0.3, p) * (1 - ss(0.52, 0.58, p));
        const land = ss(0.55, 0.62, p) * (1 - ss(0.62, 0.78, p)), out = ss(0.22, 0.58, p) * (1 - ss(0.72, 1, p));
        lift = 0.2 * Math.sin(Math.PI * Math.min(1, Math.max(0, (p - 0.24) / 0.32)));
        rootX += sx * dist * out; rootZ = back ? -dist * out : 0;
        drop += M(22) * crouch + M(16) * land;
        shX = sx * 0.05 * crouch;
        for (const f of [-1, 1]) feet[f][1] = M(18) * air;     // encolhe os pés no ar
        if (back) lean += 0.12 * crouch - 0.16 * air + 0.08 * land; else roll = -sx * (0.14 * crouch + 0.2 * air - 0.07 * land);
      } else {                                                // passada: pé da frente, corpo, pé de trás
        const s = move.sx, ease = x => { x = Math.min(1, Math.max(0, x)); return x * x * (3 - 2 * x); };
        const body = ease((p - 0.15) / 0.7), lead = ss(0, 0.6, p), trail = ss(0.5, 0.95, p);
        rootX = move.x0 + s * STEP_D * body;
        feet[s] = [s * STEP_D * (lead - body), M(34) * arc(p / 0.6)];
        feet[-s] = [s * STEP_D * (trail - body), M(14) * arc((p - 0.5) / 0.45)];
        roll = s * 0.07 * arc(p / 0.6) - s * 0.04 * arc((p - 0.55) / 0.45);   // pende para longe do pé no ar
        drop += M(8) * arc((p - 0.5) / 0.35);
        twist += s * 0.08 * arc(p);
      }
      if (p >= 1) { if (move.kind === 'step') baseX = move.x0 + move.sx * STEP_D; move = null; }
    }
    root.position.set(rootX, lift, rootZ); root.scale.setScalar(rig.scale ?? char.scale ?? 1);   // grandalhões (Cremoso, King Frog, Batalhator 1 e 2) maiores que os demais
    // parada: respira, ginga do quadril e o pulinho de luta
    const bob = Math.abs(Math.sin(t * 3.2)) * M(6), br = Math.sin(t * 1.6);
    upper.position.set(shX, WAIST + bob - drop, shZ); upper.rotation.set(lean, twist, roll + br * 0.02);
    upper.scale.set(1 + 0.01 * br, 1 + 0.008 * br, 1 + 0.01 * br);
    neck.position.y = NECK - WAIST + bob * 0.2; neck.rotation.set(-lean * 0.5 + Math.sin(t * 1.6 + 1) * 0.02, -twist * 0.4, tilt + Math.sin(t * 1.1) * 0.03);
    hips.position.set(shX, HIPS + bob * 0.9 - drop * 0.8, shZ); hips.rotation.set(lean * 0.45, twist * 0.5, roll * 0.55 + Math.sin(t * 3.2) * 0.025 * (rig.hipSway ?? 1));
    hips.updateMatrix(); rig.placeFoot(-1, ...feet[-1]); rig.placeFoot(1, ...feet[1]);
    const c = { t, dt, skill: anim.kind === 'skill' ? anim.id : null, bob, drop, shX, shZ, roll, lean, twist, heat, pose, ph: { [-1]: Math.sin(t * 3.2), [1]: Math.cos(t * 3.2) }, placeHand };
    for (const sx of [-1, 1]) updateHand(rig.hands[sx], sx, pose[sx], c);
    if (rig.onFrame) rig.onFrame(c);
    renderer.render(scene, camera);
  };
  VIN.frame = frame;
  (function loop() { requestAnimationFrame(loop); frame(); })();
}

// ================================================================ todos lado a lado (?p=todos)
// list: [[chave, módulo]]. Cada personagem monta no seu próprio grupo, em fila, parado na guarda (respira e ginga, cada
// um no seu tempo). Os botões com o nome levam a câmera até ele.
export async function runLineup(list) {
  const hud = $('#hud');
  const mods = list.map(([k, m]) => [k, m.default]);
  hud.innerHTML = `<b>K.O.-RAMA · vinil · todos</b><br><span id="names"></span><br><button id="all">Ver todos</button>
    <div id="extras" style="display:none">${mods.map(([, c]) => c.hudExtra || '').join('')}</div><div id="status">Carregando…</div>`;
  const renderer = new THREE.WebGLRenderer({ antialias: true });
  renderer.setPixelRatio(Math.min(2, devicePixelRatio)); renderer.setSize(innerWidth, innerHeight);
  renderer.outputColorSpace = THREE.SRGBColorSpace; document.body.appendChild(renderer.domElement);
  const scene = new THREE.Scene(); scene.background = new THREE.Color(0x1d1626);
  const camera = new THREE.PerspectiveCamera(32, innerWidth / innerHeight, 0.05, 200);
  const controls = new OrbitControls(camera, renderer.domElement);
  addEventListener('resize', () => { renderer.setSize(innerWidth, innerHeight); camera.aspect = innerWidth / innerHeight; camera.updateProjectionMatrix(); });
  scene.add(new THREE.HemisphereLight(0xffffff, 0x584868, 1.7));
  const sun = new THREE.DirectionalLight(0xffffff, 1.7); sun.position.set(1.2, 3, 3); scene.add(sun);
  const GAP = 1.75, W = GAP * mods.length;
  const floor = new THREE.Mesh(new THREE.PlaneGeometry(W + 2, 3), new THREE.MeshBasicMaterial({ color: 0x2c6b4a })); floor.rotation.x = -Math.PI / 2; scene.add(floor);
  const clock = new THREE.Clock(), M = px => px / 245, rigs = [];
  for (let i = 0; i < mods.length; i++) {
    const [key, char] = mods[i], x = (i - (mods.length - 1) / 2) * GAP;
    $('#status').textContent = `Carregando ${char.title}…`;
    const root = new THREE.Group(); root.position.x = x; scene.add(root);
    const rig = await char.build({ THREE, scene, root, clock, U: units(...char.units), $ });
    rigs.push({ key, char, rig, root, x, ph: i * 0.73 });
    const b = document.createElement('button'); b.textContent = char.title; b.onclick = () => look(x, 3.6); $('#names').appendChild(b);
  }
  const look = (x, d) => { camera.position.set(x + d * 0.35, 1.25, d); controls.target.set(x, 0.85, 0); controls.update(); };
  const all = () => { const d = (W / 2 + 0.6) / (Math.tan(THREE.MathUtils.degToRad(camera.fov / 2)) * camera.aspect); camera.position.set(0, 1.4, Math.max(6, d)); controls.target.set(0, 0.85, 0); controls.update(); };   // a fila inteira cabe na largura da tela
  $('#all').onclick = all; all();
  scene.traverse(o => { if (o.isMesh && o.material.isMeshToonMaterial) comicShader(o.material); });
  $('#status').textContent = 'Pronto · arraste para girar';
  const pose = { [-1]: [0, 0, 0, 0, 0], [1]: [0, 0, 0, 0, 0] };
  const frame = (dtIn = null) => {
    let dt = Math.min(0.05, clock.getDelta()); if (dtIn != null) { clock.elapsedTime += dtIn; dt = dtIn; }
    for (const R of rigs) {
      const { rig, root } = R, t = clock.elapsedTime + R.ph, { upper, neck, hips, WAIST, NECK, HIPS } = rig, H = rig.hand;
      root.position.set(R.x, 0, 0); root.scale.setScalar(rig.scale ?? R.char.scale ?? 1);
      const bob = Math.abs(Math.sin(t * 3.2)) * M(6), br = Math.sin(t * 1.6);
      upper.position.set(0, WAIST + bob, 0); upper.rotation.set(0, 0, br * 0.02); upper.scale.set(1 + 0.01 * br, 1 + 0.008 * br, 1 + 0.01 * br);
      neck.position.y = NECK - WAIST + bob * 0.2; neck.rotation.set(Math.sin(t * 1.6 + 1) * 0.02, 0, Math.sin(t * 1.1) * 0.03);
      hips.position.set(0, HIPS + bob * 0.9, 0); hips.rotation.set(0, 0, Math.sin(t * 3.2) * 0.025 * (rig.hipSway ?? 1));
      hips.updateMatrix(); rig.placeFoot(-1, 0, 0); rig.placeFoot(1, 0, 0);
      const ph = { [-1]: Math.sin(t * 3.2), [1]: Math.cos(t * 3.2) };
      const placeHand = (u, sx, hp, c, thr) => {
        const [, up, mid] = hp, ry = H.yaw * sx * (1 - mid);
        u.rotation.set(-up * 1.2, ry, 0);
        u.position.set(sx * H.x * (1 - 0.45 * mid) + Math.sin(ry) * thr, H.y + ph[sx] * 0.02 + up + bob * 0.5, H.z + Math.cos(ry) * thr);
      };
      const c = { t, dt, bob, drop: 0, shX: 0, shZ: 0, roll: 0, lean: 0, twist: 0, heat: 0, pose, ph, placeHand };
      for (const sx of [-1, 1]) (rig.updateHand || ((u, s, hp, cc) => cc.placeHand(u, s, hp, cc, hp[0])))(rig.hands[sx], sx, pose[sx], c);
      if (rig.onFrame) rig.onFrame(c);
    }
    renderer.render(scene, camera);
  };
  window.VIN = { THREE, scene, camera, controls, rigs, clock, frame, look, all };
  (function loop() { requestAnimationFrame(loop); frame(); })();
}
