// Porto flutuante (arena da Joana), em código e no acabamento do vinil, seguindo o concept
// arena-concepts/02-joana-porto.png: o espaço de luta é a própria plataforma de tábuas (sem ringue de cordas), com
// vigas de borda, pontões verdes por baixo, postes de amarração com corda enrolada e pneus pendurados; água turquesa
// animada com a rede clara de reflexos; barcos de pesca grandes e listrados em volta (as mesmas posições do jogo,
// engine-cpp/app/ring_renderer.cpp kHarborBoats), com cabine, mastros, bandeirolas, toldo, barris, defensas e boia;
// ilhas de areia ao fundo com casinhas, coqueiros e uma pontezinha de pedra; nuvens. Medidas em unidades do jogo.
import { THREE, flat, withOutline, SPR } from './core.js';
import { RoundedBoxGeometry } from 'three/addons/geometries/RoundedBoxGeometry.js';
import { UNIT } from './arena_pernambuco.js';
import { coqueiro } from './arena_pernambuco_bg.js';

const u = v => v * UNIT;
const HALF = 270, WATER = -43;   // plataforma: meia largura do jogo; água 43 abaixo do piso (harborWaterHeight)
const C = { wood: ['#8b6a48', '#7a5a3c', '#94704c', '#6f5236', '#86643f'], plankTeal: '#4f8f86', beam: '#5a3f28', post: '#7a5636', rope: '#d0ab6c',
            pontoon: '#3f7f6e', pontoonDark: '#2f6556', tire: '#22252a', water: '#1aa3c8', foam: '#8fe6ef', sand: '#e9c37a', rock: '#c9a467', green: '#4f9a3c',
            canvas: '#efe3c8', mast: '#6b4a2e', barrel: '#8a5a32', crate: '#a77a48', fender: '#2a2d33', white: '#f4f1ea', red: '#d0342c' };
const HULLS = [   // faixas do casco, de baixo para cima: fundo, três faixas, borda
  ['#b5302a', '#f2c12e', '#f4f1ea', '#1f5fae', '#1f5fae'], ['#b5302a', '#1f5fae', '#f4f1ea', '#d0342c', '#f2c12e'], ['#2c6b5e', '#f2c12e', '#d0342c', '#f4f1ea', '#1f8fae'],
  ['#b5302a', '#f4f1ea', '#1f8f7a', '#f2c12e', '#d0342c'], ['#b5302a', '#d0342c', '#f2c12e', '#1f5fae', '#f4f1ea']];
const CABINS = ['#d0342c', '#1f5fae', '#f2c12e', '#2aa8a0', '#e8742c'];
const FLAGS = ['#d0342c', '#1f5fae', '#f2c12e', '#2aa8a0', '#f4f1ea'];
const BOATS = [[-575, -440, 0.25, 390, 100, 3, 2], [0, -625, 1.40, 425, 102, 3, 2], [575, -450, 2.70, 390, 100, 3, 2], [-470, -135, -0.30, 300, 72, 3, 1], [470, -135, 0.40, 300, 72, 3, 1],
  [-545, 605, 0.70, 370, 92, 3, 2], [0, 650, 1.20, 390, 95, 3, 2], [555, 605, -0.80, 365, 90, 3, 2], [-1350, 880, 0.55, 360, 90, 3, 1], [-920, 980, 0.90, 370, 92, 3, 2],
  [-330, 1060, 0.35, 390, 95, 3, 2], [330, 1080, -0.40, 385, 94, 3, 2], [920, 990, -0.85, 370, 92, 3, 2], [1350, 890, -0.60, 360, 90, 3, 1], [-1100, 250, -0.20, 340, 85, 3, 1],
  [1100, 250, 0.25, 340, 85, 3, 1], [-1050, -350, 0.40, 350, 88, 3, 1], [1050, -350, -0.45, 350, 88, 3, 1], [-600, -950, 0.80, 380, 95, 3, 2], [600, -950, -0.75, 380, 95, 3, 2],
  [0, -1120, 1.57, 400, 98, 3, 2], [-1400, -1100, 0.50, 350, 88, 3, 0], [1400, -1100, -0.50, 350, 88, 3, 0], [0, -1650, 1.57, 380, 94, 3, 0]];

const ol = (m, w = 0.007) => { const g = withOutline(m, w); g.position.copy(m.position); g.quaternion.copy(m.quaternion); g.scale.copy(m.scale); m.position.set(0, 0, 0); m.quaternion.identity(); m.scale.set(1, 1, 1); return g; };
const box = (w, h, d, x, y, z, col, r = 1.5) => { const m = new THREE.Mesh(new RoundedBoxGeometry(u(w), u(h), u(d), 2, u(Math.min(r, w / 2 - 0.01, h / 2 - 0.01, d / 2 - 0.01))), typeof col === 'string' ? flat(col) : col); m.position.set(u(x), u(y), u(z)); return m; };
const cyl = (r, h, x, y, z, col, seg = 14) => { const m = new THREE.Mesh(new THREE.CylinderGeometry(u(r), u(r), u(h), seg), flat(col)); m.position.set(u(x), u(y), u(z)); return m; };
const tick = { t: { value: 0 } };
const HAZE = '#ffcf9c';   // cor do horizonte: céu, névoa, pé das serras
const SUN_DIR = new THREE.Vector3(-0.40, 0.17, -0.90).normalize(), SUN_COL = '#ffd9a0';   // sol baixo no terço esquerdo, atrás do adversário
const Y = new THREE.Vector3(0, 1, 0);
const MAX_OBS = 110, obs = { value: Array.from({ length: MAX_OBS }, () => new THREE.Vector3(1e6, 1e6, 0)) };   // bordas na água para a espuma
let nObs = 0; const addObs = (x, z, r) => { if (nObs < MAX_OBS) obs.value[nObs++].set(x, z, r); };   // tempo da água (atualizado no onBeforeRender da água)

// --- plataforma: tábuas ao comprido (12 de 45, duas pintadas de verde-água), vigas de borda, pontões, postes, pneus
function plataforma() {
  const g = new THREE.Group(), PW = 45, N = 12, L = 2 * HALF + 24;
  for (let i = 0; i < N; i++) {
    const x = -HALF + PW * (i + 0.5);
    // cada tábua em 2-3 pedaços (emendas desencontradas), um ou outro pintado de verde-água como no concept
    const cuts = i % 3 === 0 ? [0.42] : i % 3 === 1 ? [0.25, 0.7] : [0.58];
    let a = -L / 2;
    [...cuts, 1].forEach((c, k) => {
      const b = -L / 2 + L * c, len = b - a - 2, teal = (i === 3 && k === 1) || (i === 9 && k === 0);
      g.add(ol(box(PW - 3, 8, len, x, -4, (a + b) / 2, teal ? C.plankTeal : C.wood[(i * 3 + k) % C.wood.length], 1.2), 0.004));
      a = b;
    });
  }
  const E = HALF + 12;
  for (const sx of [-1, 1]) g.add(ol(box(22, 20, 2 * E + 22, sx * E, -12, 0, C.beam, 2), 0.006));
  for (const sz of [-1, 1]) g.add(ol(box(2 * E + 22, 20, 22, 0, -12, sz * E, C.beam, 2), 0.006));
  for (const x of [-180, 0, 180]) g.add(ol(box(14, 14, 2 * E, x, -27, 0, C.beam, 1.5), 0.004));   // travessas por baixo
  for (const x of [-180, 0, 180]) for (const z of [-180, 0, 180]) {   // pontões verdes meio afundados
    g.add(ol(box(150, 32, 150, x, -40, z, C.pontoon, 4), 0.006));
    g.add(box(152, 5, 152, x, -32, z, C.pontoonDark, 2));
  }
  // postes de amarração: quinas, meio das bordas e terços; corda enrolada em dois anéis
  const posts = [];
  for (const sx of [-1, 1]) for (const sz of [-1, 1]) posts.push([sx * HALF, sz * HALF]);
  for (const t of [-1 / 3, 1 / 3]) { posts.push([t * E * 2, E], [t * E * 2, -E], [E, t * E * 2], [-E, t * E * 2]); }
  posts.forEach(([x, z], i) => {
    const h = i < 4 ? 173 : 72;   // quinas sobem até 125: seguram as cordas (38, 68, 98)
    g.add(ol(box(26, h, 26, x, -48 + h / 2, z, C.post, 3), 0.006));
    g.add(ol(box(30, 6, 30, x, -48 + h + 2, z, C.beam, 2), 0.004));
    for (const y of [h - 66, h - 56]) { const r = new THREE.Mesh(new THREE.TorusGeometry(u(16), u(3.2), 8, 18), flat(C.rope)); r.rotation.x = Math.PI / 2; r.position.set(u(x), u(-48 + y), u(z)); g.add(ol(r, 0.003)); }
  });
  // cordas de sisal entre os postes das quinas, nas alturas do jogo; trançado em espiral pintado no shader
  const corners = [[-HALF, HALF], [HALF, HALF], [HALF, -HALF], [-HALF, -HALF]];
  const sisal = flat('#c9a663');
  sisal.onBeforeCompile = sh => {
    sh.vertexShader = 'varying vec2 vR;' + String.fromCharCode(10) + sh.vertexShader.replace('#include <begin_vertex>', '#include <begin_vertex>' + String.fromCharCode(10) + 'vR = uv;');
    sh.fragmentShader = 'varying vec2 vR;' + String.fromCharCode(10) + sh.fragmentShader.replace('#include <map_fragment>', '#include <map_fragment>' + String.fromCharCode(10) +
      '{ float k = fract(vR.y * 70.0 + vR.x * 2.0); diffuseColor.rgb *= k < 0.35 ? 0.72 : (k < 0.45 ? 0.86 : 1.0); }');
  };
  sisal.customProgramCacheKey = () => 'sisal';
  for (let i = 0; i < 4; i++) { const [ax, az] = corners[i], [bx, bz] = corners[(i + 1) % 4];
    for (const h of [38, 68, 98]) { const a = new THREE.Vector3(u(ax), u(h), u(az)), b = new THREE.Vector3(u(bx), u(h), u(bz)), m = new THREE.Mesh(new THREE.CylinderGeometry(u(3.4), u(3.4), a.distanceTo(b), 10, 1), sisal);
      m.position.copy(a).add(b).multiplyScalar(0.5); m.quaternion.setFromUnitVectors(new THREE.Vector3(0, 1, 0), b.clone().sub(a).normalize()); g.add(ol(m, 0.004)); } }
  // pneus pendurados nas bordas, de pé, virados para fora
  for (const [x, z, ry] of [[-150, E + 14, 0], [150, E + 14, 0], [-150, -E - 14, 0], [150, -E - 14, 0], [E + 14, -120, Math.PI / 2], [E + 14, 120, Math.PI / 2], [-E - 14, -120, Math.PI / 2], [-E - 14, 120, Math.PI / 2]]) {
    const t = new THREE.Mesh(new THREE.TorusGeometry(u(22), u(9), 10, 22), flat(C.tire)); t.position.set(u(x), u(-24), u(z)); t.rotation.y = ry; g.add(ol(t, 0.005));
    const tie = new THREE.Mesh(new THREE.CylinderGeometry(u(2), u(2), u(22), 6), flat(C.rope)); tie.position.set(u(x), u(-4), u(z)); g.add(tie);
  }
  return g;
}

// --- água: plano grande, ondas suaves perto da plataforma e a rede clara de reflexos (células animadas)
function agua() {
  const N = 30000, g = new THREE.PlaneGeometry(u(N), u(N), 240, 240); g.rotateX(-Math.PI / 2);   // 366 m: a borda fica além da névoa cheia (150 m), o mar some no horizonte
  const m = flat(C.water);
  m.onBeforeCompile = sh => {
    sh.uniforms.uT = tick.t; sh.uniforms.uFoam = { value: new THREE.Color(C.foam) }; sh.uniforms.uObs = obs; sh.uniforms.uShallow = { value: new THREE.Color('#5fd0d8') }; sh.uniforms.uWhite = { value: new THREE.Color('#f4fbfb') }; sh.uniforms.uSunDir = { value: SUN_DIR }; sh.uniforms.uSunC = { value: new THREE.Color(SUN_COL) }; sh.uniforms.uHaze = { value: new THREE.Color(HAZE) };
    sh.vertexShader = 'uniform float uT; varying vec2 vW;\n' + sh.vertexShader.replace('#include <begin_vertex>', `#include <begin_vertex>
      vec2 wp = position.xz / ${UNIT.toFixed(6)}; vW = wp;
      float near = 1.0 - smoothstep(600.0, 2600.0, length(wp));
      transformed.y += ${UNIT.toFixed(6)} * near * (3.0 * sin(wp.x * 0.015 + uT * 1.9) + 2.0 * sin(wp.y * 0.020 - uT * 1.4));`);
    sh.fragmentShader = 'uniform float uT; uniform vec3 uFoam, uShallow, uWhite, uSunDir, uSunC, uHaze; uniform vec3 uObs[' + MAX_OBS + ']; varying vec2 vW; float gGlint = 0.0;\n' + `
      vec2 h2(vec2 p) { p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3))); return fract(sin(p) * 43758.5453); }
      float vn(vec2 p) { vec2 i = floor(p), f = fract(p); f = f * f * (3.0 - 2.0 * f);
        return mix(mix(h2(i).x, h2(i + vec2(1.0, 0.0)).x, f.x), mix(h2(i + vec2(0.0, 1.0)).x, h2(i + vec2(1.0, 1.0)).x, f.x), f.y); }
      float cellEdge(vec2 p, float t) { vec2 i = floor(p), f = fract(p); float m1 = 8.0, m2 = 8.0;
        for (int y = -1; y <= 1; y++) for (int x = -1; x <= 1; x++) { vec2 gg = vec2(x, y), o = 0.5 + 0.42 * sin(t * 0.7 + 6.2831 * h2(i + gg)); vec2 r = gg + o - f; float d = dot(r, r);
          if (d < m1) { m2 = m1; m1 = d; } else if (d < m2) m2 = d; }
        return sqrt(m2) - sqrt(m1); }
    ` + sh.fragmentShader.replace('#include <map_fragment>', `#include <map_fragment>
      { vec2 p = vW / 150.0 + vec2(uT * 0.04, uT * 0.025);
        float e = cellEdge(p, uT), w = fwidth(e) * 1.2;
        float line = 1.0 - smoothstep(0.035 - w, 0.035 + w, e);
        float fade = 1.0 - smoothstep(1500.0, 3200.0, length(vW));
        diffuseColor.rgb = mix(diffuseColor.rgb, uFoam, line * 0.55 * fade);
        // espuma reativa: distância até a borda mais perto (plataforma ou círculos dos cascos e ilhas)
        vec2 q = abs(vW) - vec2(300.0); float d = length(max(q, 0.0)) + min(max(q.x, q.y), 0.0);
        float ph = 0.0;
        for (int i = 0; i < ${MAX_OBS}; i++) { float di = length(vW - uObs[i].xy) - uObs[i].z; if (di < d) { d = di; ph = float(i) * 1.7; } }
        // água mais clara no raso perto das bordas
        diffuseColor.rgb = mix(diffuseColor.rgb, uShallow, (1.0 - smoothstep(0.0, 90.0, d)) * 0.55);
        // a onda bate e volta, mas cada trecho da borda no seu tempo e alcance (ruído que anda devagar); a espuma se
        // quebra em retalhos, a segunda linha aparece só em pedaços, e uns flocos soltos boiam perto da borda
        float n1 = vn(vW * 0.012 + vec2(uT * 0.12, -uT * 0.09));
        float n2 = vn(vW * 0.05 + vec2(-uT * 0.30, uT * 0.25));
        float n3 = vn(vW * 0.11 + vec2(uT * 0.55, uT * 0.40));
        float swell = 0.5 + 0.5 * sin(uT * (1.0 + 0.9 * n1) + ph + n1 * 6.2831);
        float o = 2.0 + 34.0 * n1 * swell + 10.0 * n2;
        float foam = (1.0 - smoothstep(o - 2.0, o + 2.5, d)) * smoothstep(0.30, 0.48, n3 + 0.35 * (1.0 - clamp(d / max(o, 1.0), 0.0, 1.0)));
        float o2 = o + 12.0 + 22.0 * n2;
        foam = max(foam, (1.0 - smoothstep(1.5, 4.0, abs(d - o2))) * step(0.52, n3) * (0.45 + 0.55 * (1.0 - n1)));
        foam = max(foam, step(0.86, vn(vW * 0.22 + vec2(uT * 0.7, -uT * 0.5))) * (1.0 - smoothstep(10.0, 70.0, d)) * 0.8);
        diffuseColor.rgb = mix(diffuseColor.rgb, uWhite, foam * step(-40.0, d));
        // caminho de brilho do sol: o olhar refletido nas ondinhas (normal sacudida por ruído) bate no sol
        vec3 wp = vec3(vW.x * ${UNIT.toFixed(6)}, ${(WATER * UNIT).toFixed(5)}, vW.y * ${UNIT.toFixed(6)}), V = normalize(wp - cameraPosition);
        vec2 rn = vec2(vn(vW * 0.045 + vec2(uT * 0.9, uT * 0.4)), vn(vW * 0.06 + vec2(-uT * 0.5, uT * 1.1))) - 0.5;
        float sg = dot(reflect(V, normalize(vec3(rn.x * 0.55, 1.0, rn.y * 0.55))), uSunDir), s0 = dot(reflect(V, vec3(0.0, 1.0, 0.0)), uSunDir);
        gGlint = smoothstep(0.990, 0.995, sg) * 1.3 + pow(max(s0, 0.0), 30.0) * 0.4; }`)
      .replace('#include <opaque_fragment>', 'outgoingLight = mix(outgoingLight, diffuseColor.rgb, 0.35) + uSunC * gGlint;\n' +   // água brilha um pouco por conta própria
        'outgoingLight = mix(outgoingLight, uHaze, pow(smoothstep(2400.0, 9600.0, length(vW)), 0.75));\n#include <opaque_fragment>');   // vira névoa aos poucos (30 m a 117 m): faixa macia na tela até o pé das serras
  };
  m.customProgramCacheKey = () => 'agua-joana';
  const w = new THREE.Mesh(g, m); w.position.y = u(WATER);
  w.onBeforeRender = () => { tick.t.value = performance.now() / 1000; };
  return w;
}

// --- barco da plateia (o do estudo tools/barco_estudo_3d.html, agora em 3D de verdade): casco por 8 costelas da popa
// à proa, com as faixas do estudo (fundo escuro, lateral baixa e alta alternando o tom a cada gomo, faixa creme, friso
// colorido e borda de madeira), popa reta de madeira com leme, roda de proa erguida com argola e lampião, estrado,
// três bancos, remos, boia e cracas na linha d'água; torcedores do atlas do jogo sentados nos bancos. Medidas do
// estudo (popa x = -330 até a ponta da proa x = 368); escala s; frente para +x
const THEMES = [
  { hull: '#1e8389', hullLight: '#39a4aa', hullShade: '#125459', keel: '#183642', cream: '#f5dfb8', accent: '#e26b48', wood: '#885c3b', woodLight: '#ba8558', deck: '#473d32', inner: '#2f4b4e', brass: '#f6c167', rope: '#e8cb90' },
  { hull: '#a33b2e', hullLight: '#c65344', hullShade: '#68241c', keel: '#361e1b', cream: '#f6ece1', accent: '#2b587a', wood: '#724b33', woodLight: '#9e6d4c', deck: '#3b3633', inner: '#522f2b', brass: '#f6c167', rope: '#e8cb90' },
  { hull: '#26557e', hullLight: '#3d74a4', hullShade: '#173652', keel: '#142536', cream: '#f4c768', accent: '#e58e37', wood: '#80593b', woodLight: '#b08157', deck: '#3e3a38', inner: '#284157', brass: '#f6c167', rope: '#e8cb90' },
  { hull: '#556b46', hullLight: '#748e62', hullShade: '#37472d', keel: '#232e1d', cream: '#e3dcce', accent: '#cb7a48', wood: '#66533f', woodLight: '#8f775e', deck: '#35332e', inner: '#3f4937', brass: '#e4b669', rope: '#d7c79e' }];
const RIBS = [{ x: -330, w: 74, top: 130, chine: 56, keel: 26 }, { x: -260, w: 124, top: 114, chine: 40, keel: 16 }, { x: -150, w: 148, top: 104, chine: 32, keel: 12 }, { x: 0, w: 154, top: 100, chine: 30, keel: 11 },
  { x: 150, w: 142, top: 110, chine: 38, keel: 16 }, { x: 260, w: 98, top: 134, chine: 60, keel: 32 }, { x: 335, w: 34, top: 162, chine: 94, keel: 66 }, { x: 368, w: 0, top: 184, chine: 128, keel: 98 }];
const FANS = [[1940, 1164], [2716, 1358], [1358, 970], [776, 0], [1940, 388], [3104, 0], [776, 776], [1358, 194]];   // torcedores no atlas crowd.png (quadros de 192)
const SEATS = [[3, -285, -35], [5, -285, 35], [1, -190, -55], [4, -190, 55], [2, -10, -60], [6, -10, 60], [0, 170, -48], [7, 170, 48]];
let crowdImg = null;
const fanTex = {};
function fanTexture(i) {
  if (fanTex[i]) return fanTex[i];
  const c = document.createElement('canvas'); c.width = c.height = 192; const g = c.getContext('2d'); g.imageSmoothingEnabled = false;
  if (crowdImg) g.drawImage(crowdImg, FANS[i][0], FANS[i][1], 192, 192, 0, 0, 192, 192);
  const t = new THREE.CanvasTexture(c); t.colorSpace = THREE.SRGBColorSpace; t.magFilter = THREE.NearestFilter; return (fanTex[i] = t);
}
function barcoPlateia(s, theme, nFans) {
  const T = THEMES[theme % THEMES.length], g = new THREE.Group(), q = v => u(v * s);
  const P = (x, y, z) => [q(x), q(y), q(z)];
  // casco: grade de costelas x 9 pontos (borda, -12, -38, quina, quilha, e o espelho do outro lado); "band" escolhe a faixa
  const prof = r => [[r.top, -r.w], [r.top - 12, -r.w * 0.98], [r.top - 38, -r.w * 0.9], [r.chine, -r.w * 0.56], [r.keel, 0], [r.chine, r.w * 0.56], [r.top - 38, r.w * 0.9], [r.top - 12, r.w * 0.98], [r.top, r.w]];
  const pos = [];
  RIBS.forEach(r => prof(r).forEach(([y, z]) => pos.push(...P(r.x, y, z))));
  // faixas: cada quadrado da grade vai para o grupo da sua cor (creme, lateral alta clara/escura, lateral baixa, fundo)
  const NJ = 9, groups = [[], [], [], [], []];   // 0 creme, 1 casco, 2 casco claro, 3 sombra, 4 fundo
  for (let i = 0; i < RIBS.length - 1; i++) for (let j = 0; j < NJ - 1; j++) {
    const a = i * NJ + j, b = a + 1, c = a + NJ, d = c + 1, k = j > 3 ? 7 - j : j, even = i % 2 === 0;
    const m = k === 0 ? 0 : k === 1 ? (even ? 1 : 2) : k === 2 ? (even ? 3 : 1) : 4;
    groups[m].push(a, c, b, b, c, d);   // faces para fora
  }
  const hg = new THREE.BufferGeometry(); hg.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3));
  const idx = []; groups.forEach((gr, m) => { hg.addGroup(idx.length, gr.length, m); idx.push(...gr); });
  hg.setIndex(idx); hg.computeVertexNormals();
  const mats = [T.cream, T.hull, T.hullLight, T.hullShade, T.keel].map(c => { const m = flat(c); m.side = THREE.DoubleSide; m.polygonOffset = true; m.polygonOffsetFactor = -2; m.polygonOffsetUnits = -6; return m; });   // ganha do contorno vendo por dentro
  g.add(ol(new THREE.Mesh(hg, mats), 0.008));
  // popa de madeira (fecha a costela de trás)
  { const r = RIBS[0], pts = prof(r), sh = new THREE.Shape(); pts.forEach(([y, z], j) => (j ? sh.lineTo(q(z), q(y)) : sh.moveTo(q(z), q(y)))); sh.closePath();
    const m = new THREE.Mesh(new THREE.ShapeGeometry(sh), flat(T.wood)); m.material.side = THREE.DoubleSide; m.rotation.y = -Math.PI / 2; m.position.x = q(r.x); g.add(ol(m, 0.004)); }
  // friso colorido (top-36) e borda de madeira (top) dos dois lados, como tubos
  const rail = (yOf, wOf, r0, col) => { for (const side of [-1, 1]) { const pts = RIBS.map(r => new THREE.Vector3(q(r.x), q(yOf(r)), q(side * wOf(r)))); g.add(new THREE.Mesh(new THREE.TubeGeometry(new THREE.CatmullRomCurve3(pts), 40, q(r0), 6), flat(col))); } };
  rail(r => r.top - 36, r => r.w * 0.91, 2.6, T.accent);
  rail(r => r.top + 2, r => r.w, 4.5, T.woodLight);
  // roda de proa erguida, argola e lampião
  { const a = RIBS[6], b = RIBS[7], sh = new THREE.Shape();
    sh.moveTo(q(a.x), q(a.keel)); sh.lineTo(q(b.x), q(b.keel)); sh.lineTo(q(b.x + 8), q(b.top + 16)); sh.lineTo(q(b.x - 6), q(b.top + 16)); sh.lineTo(q(a.x), q(a.top)); sh.closePath();
    const m = new THREE.Mesh(new THREE.ExtrudeGeometry(sh, { depth: q(10), bevelEnabled: false }), flat(T.woodLight)); m.position.z = -q(5); g.add(ol(m, 0.004));
    const ring = new THREE.Mesh(new THREE.TorusGeometry(q(6), q(1.8), 6, 14), flat(T.brass)); ring.position.set(q(b.x + 2), q(b.top + 8), q(6)); g.add(ring);
    const lamp = new THREE.Mesh(new THREE.BoxGeometry(q(8), q(12), q(8)), new THREE.MeshBasicMaterial({ color: '#ffd27a' })); lamp.position.set(q(b.x + 16), q(b.top + 4), 0); g.add(ol(lamp, 0.003)); }
  // leme
  { const r = RIBS[0], sh = new THREE.Shape(); sh.moveTo(q(r.x), q(r.keel - 15)); sh.lineTo(q(r.x - 28), q(r.keel + 10)); sh.lineTo(q(r.x - 32), q(r.top + 8)); sh.lineTo(q(r.x), q(r.top + 5)); sh.closePath();
    const m = new THREE.Mesh(new THREE.ExtrudeGeometry(sh, { depth: q(6), bevelEnabled: false }), flat(T.woodLight)); m.position.z = -q(3); g.add(ol(m, 0.004)); }
  // estrado afundado e bancos
  { const sh = new THREE.Shape(), rs = RIBS.slice(0, 6); rs.forEach((r, i) => (i ? sh.lineTo(q(r.x), q(r.w * 0.82)) : sh.moveTo(q(r.x), q(r.w * 0.82)))); rs.slice().reverse().forEach(r => sh.lineTo(q(r.x), -q(r.w * 0.82))); sh.closePath();
    const m = new THREE.Mesh(new THREE.ShapeGeometry(sh), flat(T.deck)); m.material.side = THREE.DoubleSide; m.rotation.x = Math.PI / 2; m.position.y = q(72); g.add(m); }
  for (const [bx, bw] of [[-190, 140], [-10, 152], [170, 136]]) { const m = new THREE.Mesh(new RoundedBoxGeometry(q(28), q(12), q(bw * 1.76), 2, q(2)), flat(T.woodLight)); m.position.set(q(bx), q(82), 0); g.add(ol(m, 0.004)); }
  // remos apoiados nos toletes, de pá colorida
  for (const side of [-1, 1]) {
    const a = new THREE.Vector3(q(-160), q(102), q(side * 100)), b = new THREE.Vector3(q(210), q(120), q(side * 174)), len = a.distanceTo(b);
    const oar = new THREE.Mesh(new THREE.CylinderGeometry(q(2.5), q(2.5), len, 6), flat(T.woodLight)); oar.position.copy(a).add(b).multiplyScalar(0.5); oar.quaternion.setFromUnitVectors(new THREE.Vector3(0, 1, 0), b.clone().sub(a).normalize()); g.add(oar);
    const blade = new THREE.Mesh(new THREE.BoxGeometry(q(50), q(2), q(14)), flat(T.accent)); blade.position.copy(b).lerp(a, 0.07); blade.lookAt(b.clone().add(b.clone().sub(a))); blade.rotateY(Math.PI / 2); g.add(ol(blade, 0.003));
  }
  // boia salva-vidas na amurada
  { const r = new THREE.Mesh(new THREE.TorusGeometry(q(17), q(5), 8, 20), flat(T.cream)); r.position.set(q(-60), q(84), q(153)); g.add(ol(r, 0.004));
    for (let k = 0; k < 4; k++) { const a = new THREE.Mesh(new THREE.TorusGeometry(q(17), q(5.3), 8, 6, Math.PI / 4), flat(T.accent)); a.position.copy(r.parent ? r.parent.position : r.position); a.rotation.z = k * Math.PI / 2 + 0.2; g.add(a); } }
  // cracas na linha d'água
  for (const [cx, cy, side] of [[-200, 48, -1], [-80, 38, -1], [60, 42, -1], [180, 52, -1], [-220, 48, 1], [-60, 38, 1], [70, 44, 1], [190, 54, 1]]) for (let k = 0; k < 3; k++) {
    const c = new THREE.Mesh(new THREE.ConeGeometry(q(4.5), q(5), 7), flat('#e8eee6')); c.rotation.x = side * Math.PI / 2; c.position.set(q(cx + (k - 1) * 8), q(cy + (k % 2) * 6), q(side * 136 * 0.72 + side * 2)); g.add(c);
  }
  // torcida: sprites sentados nos bancos (a cintura do quadro, a 62% da altura, encosta no banco)
  SEATS.slice(0, nFans).forEach(([f, sx, sz]) => {
    const sp = new THREE.Sprite(new THREE.SpriteMaterial({ map: fanTexture(f), alphaTest: 0.5 })); const h = q(148);
    sp.scale.set(h, h, 1); sp.position.set(q(sx), q(88) + h * 0.12, q(sz)); g.add(sp);
  });
  return g;
}

// --- ilha facetada (como a imagem de referência): blocos de rocha dourada em prisma, um bloco grande com outro em cima
// e outros menores em volta descendo até a água, moitas no alto dos blocos, coqueiros em alguns topos, base de areia
function rocha(r, h, seed, col) {   // prisma irregular de faces chapadas; o contorno usa a malha soldada (sem rasgos)
  const seg = 5 + (seed % 3), geo = new THREE.CylinderGeometry(u(r * 0.72), u(r), u(h), seg, 2), p = geo.attributes.position;
  const hsh = n => { const v = Math.sin(n * 91.7 + seed * 13.1) * 43758.5453; return v - Math.floor(v); };
  for (let i = 0; i < p.count; i++) { const x = p.getX(i), y = p.getY(i), z = p.getZ(i), a = Math.atan2(z, x), k = 1 + 0.18 * (hsh(Math.round(a * 10) + Math.round(y * 50)) - 0.5);
    p.setXYZ(i, x * k, y + (y > 0 ? u(h * 0.08) * (hsh(Math.round(a * 7)) - 0.5) : 0), z * k); }
  geo.translate(0, u(h / 2), 0); geo.computeVertexNormals();
  const m = new THREE.Mesh(geo, flat(col)), g = withOutline(m, 0.007);
  m.geometry = geo.toNonIndexed(); m.geometry.computeVertexNormals();   // faces chapadas (facetado)
  return g;
}
function ilha(w, h, d, houses, palms, seed = 0) {
  const g = new THREE.Group(), hsh = n => { const v = Math.sin(n * 57.3 + seed * 31.7) * 43758.5453; return v - Math.floor(v); };
  const cols = ['#e8b85a', '#d9a648', '#f0c46a'];
  const tops = [];
  const add = (x, z, r, hh, c, y0 = 0) => { const b = rocha(r, hh, seed * 7 + tops.length, c); b.position.set(u(x), u(y0), u(z)); b.rotation.y = hsh(tops.length) * 6; g.add(b); tops.push([x, y0 + hh, z, r]); };
  add(0, 0, w * 0.48, h * 0.62, cols[0]);
  add(w * 0.06, d * -0.05, w * 0.3, h * 0.42, cols[2], h * 0.62);          // bloco de cima
  const n = 4 + (seed % 3);
  for (let i = 0; i < n; i++) { const a = i / n * Math.PI * 2 + hsh(i + 10) * 0.8, rr = 0.55 + hsh(i + 20) * 0.25;
    add(Math.cos(a) * w * rr, Math.sin(a) * d * rr, w * (0.18 + hsh(i + 30) * 0.14), h * (0.22 + hsh(i + 40) * 0.38), cols[i % 3]); }
  // base de areia irregular na linha d'água
  { const geo = new THREE.CylinderGeometry(u(w * 0.98), u(w * 1.06), u(16), 9), p = geo.attributes.position;
    for (let i = 0; i < p.count; i++) { const a = Math.atan2(p.getZ(i), p.getX(i)), k = 1 + 0.12 * Math.sin(a * 3 + seed) + 0.06 * Math.sin(a * 7); p.setX(i, p.getX(i) * k); p.setZ(i, p.getZ(i) * k * d / w); }
    geo.computeVertexNormals(); const b = new THREE.Mesh(geo, flat(C.sand)); b.position.y = u(3); g.add(ol(b, 0.005)); }
  // moitas no alto dos blocos e coqueiros em alguns topos
  tops.forEach(([x, y, z, r], i) => { for (let k = 0; k < 3; k++) { const s = r * (0.28 + hsh(i * 5 + k) * 0.14), a = hsh(i * 3 + k) * 6.28;
      const b = new THREE.Mesh(new THREE.IcosahedronGeometry(u(s), 0), flat(k % 2 ? C.green : '#3f8a3a')); b.position.set(u(x + Math.cos(a) * r * 0.35), u(y + s * 0.4), u(z + Math.sin(a) * r * 0.35)); g.add(ol(b, 0.004)); } });
  palms.forEach((pl, i) => { const t = tops[(i * 2 + 1) % tops.length]; const pm = coqueiro(220 + (i % 2) * 60, pl[2] || 0.2, seed + i); pm.position.set(u(t[0]), u(t[1] - 4), u(t[2])); g.add(pm); });
  g.userData.r = w * 1.02;   // raio na linha d'água (espuma)
  return g;
}

// --- peças de porto (medidas em unidades do jogo; y = 0 no piso da plataforma)
const rod = (a, b, r, col, w = 0.008) => { const A = new THREE.Vector3(...a).multiplyScalar(UNIT), B = new THREE.Vector3(...b).multiplyScalar(UNIT);
  const m = new THREE.Mesh(new THREE.CylinderGeometry(u(r), u(r), A.distanceTo(B), 8), typeof col === 'string' ? flat(col) : col);
  m.position.copy(A).add(B).multiplyScalar(0.5); m.quaternion.setFromUnitVectors(Y, B.clone().sub(A).normalize()); return w ? ol(m, w) : m; };
const craca = (x, y, z, dir, r = 3.5) => { const c = new THREE.Mesh(new THREE.ConeGeometry(u(r), u(r * 1.3), 6), flat('#e8eee6')); c.position.set(u(x), u(y), u(z)); c.quaternion.setFromUnitVectors(Y, dir.clone().normalize()); return c; };
// cabo de amarração caído entre dois pontos (catenária simples)
function cabo(a, b, sag, r = 2.2) {
  const A = new THREE.Vector3(...a), B = new THREE.Vector3(...b), pts = [];
  for (let i = 0; i <= 16; i++) { const t = i / 16, p = A.clone().lerp(B, t); p.y -= sag * 4 * t * (1 - t); pts.push(p.multiplyScalar(UNIT)); }
  return ol(new THREE.Mesh(new THREE.TubeGeometry(new THREE.CatmullRomCurve3(pts), 24, u(r), 6), flat(C.rope)), 0.003);
}
// caixote de peixe de ripas (corpo + duas ripas escuras em volta), peixes por cima se fish
function caixote(w, h, d, x, y, z, fish = false) {
  const g = new THREE.Group();
  g.add(ol(box(w, h, d, x, y + h / 2, z, C.crate, 1.5), 0.003));
  for (const f of [0.3, 0.72]) g.add(box(w + 1.5, h * 0.12, d + 1.5, x, y + h * f, z, '#7d5634', 0.5));
  if (fish) for (let k = 0; k < 4; k++) { const f = new THREE.Mesh(new THREE.SphereGeometry(u(w * 0.16), 10, 6), flat(k % 2 ? '#c9d4d8' : '#aebdc4'));
    f.scale.set(2.2, 0.5, 0.8); f.position.set(u(x + (k % 2 - 0.5) * w * 0.4), u(y + h + 2), u(z + (k - 1.5) * d * 0.2)); f.rotation.y = k * 0.7; g.add(ol(f, 0.002)); }
  return g;
}
// âncora velha coberta de cracas, pendurada pela argola (y = 0 na argola, pendurada para baixo); s = escala
function ancora(s) {
  const g = new THREE.Group(), iron = '#3d4249', q = v => v * s;
  const ring = new THREE.Mesh(new THREE.TorusGeometry(u(q(9)), u(q(2.6)), 8, 16), flat(iron)); ring.position.y = u(q(-8)); g.add(ol(ring, 0.003));
  g.add(rod([0, q(-16), 0], [0, q(-112), 0], q(4.5), iron, 0.004));                                         // haste
  g.add(rod([0, q(-26), q(-30)], [0, q(-26), q(30)], q(3), iron, 0.004));                                   // cepo
  const arm = new THREE.Mesh(new THREE.TorusGeometry(u(q(32)), u(q(4.5)), 8, 18, Math.PI), flat(iron)); arm.rotation.z = Math.PI; arm.position.y = u(q(-80)); g.add(ol(arm, 0.004));
  for (const sx of [-1, 1]) { const f = new THREE.Mesh(new THREE.ConeGeometry(u(q(10)), u(q(22)), 4), flat(iron)); f.scale.z = 0.4; f.position.set(u(q(sx * 32)), u(q(-72)), 0); f.rotation.z = -sx * 0.35; g.add(ol(f, 0.003)); }
  // cracas espalhadas pelos braços e pela haste (é a Cara-de-Craca)
  for (let k = 0; k < 16; k++) { const a = Math.PI + (k / 15) * Math.PI, onArm = k % 3 !== 0;
    const x = onArm ? Math.cos(a) * 32 : 0, y = onArm ? -80 + Math.sin(a) * 32 : -40 - k * 3.5, dir = onArm ? new THREE.Vector3(Math.cos(a), Math.sin(a), (k % 2 ? 1 : -1) * 0.6) : new THREE.Vector3(k % 2 ? 1 : -1, 0, 0.5);
    g.add(craca(q(x + dir.x * 4), q(y + dir.y * 4), q(dir.z * 4), dir, q(4 + (k % 3) * 1.5))); }
  return g;
}
// rede de pesca secando, pendurada por cima da viga de um lado (de zA a zB), com boias de cortiça laranja na borda
function rede(x0, zA, zB, drop) {
  const W = zB - zA, geo = new THREE.PlaneGeometry(1, 1, 30, 8), p = geo.attributes.position, uv = geo.attributes.uv;
  for (let i = 0; i < p.count; i++) { const sP = uv.getX(i), t = 1 - uv.getY(i), sag = 10 * Math.sin(Math.PI * sP);
    const x = x0 - 16 * Math.min(1, t / 0.15) - 6 * t - 4 * Math.sin(sP * 9) * t, y = 30 - sag - t * drop * (1 - 0.18 * Math.sin(Math.PI * sP)) + 6 * Math.sin(sP * 13) * t * t, z = zA + W * sP;
    p.setXYZ(i, u(x), u(y), u(z)); }
  geo.computeVertexNormals();
  const m = flat('#355f58'); m.side = THREE.DoubleSide;
  m.onBeforeCompile = sh => {   // malha em losango: o que não é fio é furo
    sh.vertexShader = 'varying vec2 vN;\n' + sh.vertexShader.replace('#include <begin_vertex>', '#include <begin_vertex>\nvN = uv * vec2(' + (W / 11).toFixed(1) + ', ' + (drop / 11).toFixed(1) + ');');
    sh.fragmentShader = 'varying vec2 vN;\n' + sh.fragmentShader.replace('#include <clipping_planes_fragment>', `#include <clipping_planes_fragment>
      { vec2 a = vec2(vN.x + vN.y, vN.x - vN.y); vec2 f = abs(fract(a) - 0.5); float e = 0.5 - max(f.x, f.y), w = fwidth(e) * 1.5;
        if (e > 0.07 + w) discard; }`);
  };
  m.customProgramCacheKey = () => 'rede';
  const g = new THREE.Group(); g.add(new THREE.Mesh(geo, m));
  for (let k = 0; k <= 8; k++) { const sP = k / 8, b = new THREE.Mesh(new THREE.SphereGeometry(u(5), 8, 6), flat('#e8742c')); b.scale.set(0.8, 1, 1.3);
    b.position.set(u(x0 - 6), u(31 - 10 * Math.sin(Math.PI * sP)), u(zA + W * sP)); g.add(ol(b, 0.002)); }
  return g;
}
// torre de vigia de palafita (a da imagem de referência): quatro pernas inclinadas com travas em X, limo e cracas na
// linha d'água, piso com guarda-corpo, toldo listrado vermelho e creme num mastro, bandeira vermelha da Joana, escada
// de frente para o ringue até um pontãozinho com caixotes e cabo enrolado, lampião aceso embaixo do toldo
function torre() {
  const g = new THREE.Group(), B = WATER - 40, H = WATER + 330, S = 54, T = 44, wood = '#7a5636', lite = '#a07a50';
  const leg = (sx, sz, y) => { const k = (y - B) / (H - B), r = S + (T - S) * k; return [sx * r, y, sz * r]; };
  const L = [[-1, -1], [1, -1], [1, 1], [-1, 1]];
  L.forEach(([sx, sz]) => {
    g.add(rod(leg(sx, sz, B), leg(sx, sz, H), 7, wood, 0.01));
    g.add(rod(leg(sx, sz, B), leg(sx, sz, WATER + 24), 8.6, '#55703f', 0));   // limo na linha d'água
    for (let k = 0; k < 7; k++) { const a = k / 7 * 6.283 + sx + sz, [x, , z] = leg(sx, sz, WATER + 8); g.add(craca(x + Math.cos(a) * 9, WATER + 6 + (k % 3) * 6, z + Math.sin(a) * 9, new THREE.Vector3(Math.cos(a), 0.3, Math.sin(a)), 3.2)); }
  });
  for (let i = 0; i < 4; i++) { const [ax, az] = L[i], [bx, bz] = L[(i + 1) % 4];
    for (const [y1, y2] of [[WATER + 30, WATER + 170], [WATER + 170, H - 8]]) { g.add(rod(leg(ax, az, y1), leg(bx, bz, y2), 3.4, wood, 0.006)); g.add(rod(leg(ax, az, y2), leg(bx, bz, y1), 3.4, wood, 0.006)); }
    g.add(rod(leg(ax, az, WATER + 170), leg(bx, bz, WATER + 170), 3.8, wood, 0.006)); }
  const D = T + 22;
  g.add(ol(box(2 * D, 10, 2 * D, 0, H + 5, 0, C.wood[0], 2), 0.01));
  // guarda-corpo (com vão na frente, onde chega a escada)
  for (const [sx, sz] of L) g.add(rod([sx * D, H + 10, sz * D], [sx * D, H + 52, sz * D], 3, lite, 0.006));
  for (const y of [H + 30, H + 50]) {
    g.add(rod([-D, y, -D], [D, y, -D], 2.4, lite, 0.005)); g.add(rod([-D, y, -D], [-D, y, D], 2.4, lite, 0.005)); g.add(rod([D, y, -D], [D, y, D], 2.4, lite, 0.005));
    g.add(rod([-D, y, D], [-20, y, D], 2.4, lite, 0.005)); g.add(rod([20, y, D], [D, y, D], 2.4, lite, 0.005)); }
  // toldo: mastros nas quinas e telhado de lona em gomos vermelho e creme
  for (const [sx, sz] of L) g.add(rod([sx * (D - 6), H + 10, sz * (D - 6)], [sx * (D - 6), H + 118, sz * (D - 6)], 2.6, lite, 0.005));
  { const geo = new THREE.ConeGeometry(u(D * 1.5), u(58), 8, 1).toNonIndexed(), p = geo.attributes.position, col = [];
    for (let i = 0; i < p.count; i += 3) { const cx = (p.getX(i) + p.getX(i + 1) + p.getX(i + 2)) / 3, cz = (p.getZ(i) + p.getZ(i + 1) + p.getZ(i + 2)) / 3;
      const seg = Math.floor((Math.atan2(cz, cx) + Math.PI) / (2 * Math.PI) * 8 + 0.5) % 8, c = new THREE.Color(seg % 2 ? '#d0342c' : '#f2e6cc');
      for (let k = 0; k < 3; k++) col.push(c.r, c.g, c.b); }
    geo.setAttribute('color', new THREE.Float32BufferAttribute(col, 3)); geo.computeVertexNormals();
    const roof = new THREE.Mesh(geo, new THREE.MeshToonMaterial({ vertexColors: true, gradientMap: flat('#fff').gradientMap })); roof.position.y = u(H + 145); roof.rotation.y = Math.PI / 8;
    const og = withOutline(roof, 0.01); og.position.copy(roof.position); og.rotation.copy(roof.rotation); roof.position.set(0, 0, 0); roof.rotation.set(0, 0, 0); g.add(og); }
  // mastro e bandeira vermelha (balança no vento: vértices mexidos a cada quadro)
  g.add(rod([0, H + 170, 0], [0, H + 285, 0], 2.6, lite, 0.005));
  const fg = new THREE.PlaneGeometry(u(84), u(50), 12, 1); fg.translate(u(42), 0, 0);
  const flag = new THREE.Mesh(fg, flat('#d0342c')); flag.material.side = THREE.DoubleSide; flag.position.y = u(H + 255); flag.userData.anim = { k: 'flag', a: 0.14, rad: true, L: u(84) }; g.add(flag);
  g.userData.flag = { geo: fg, base: Float32Array.from(fg.attributes.position.array) };
  // lampião aceso embaixo do toldo, na frente
  g.add(rod([0, H + 118, D - 8], [0, H + 98, D - 8], 0.8, '#222', 0));
  g.add(ol(new THREE.Mesh(new THREE.BoxGeometry(u(10), u(14), u(10)), new THREE.MeshBasicMaterial({ color: '#ffd27a' })), 0.003)); g.children[g.children.length - 1].position.set(0, u(H + 90), u(D - 8));
  // escada de frente para o ringue, do pontão ao piso
  const lad = (x, y) => [x, y, S + 16 + (T - S) * (y - B) / (H - B)];
  for (const x of [-16, 16]) g.add(rod(lad(x, WATER - 4), lad(x, H + 10), 2.6, lite, 0.005));
  for (let y = WATER + 22; y < H; y += 26) g.add(rod(lad(-16, y), lad(16, y), 1.8, lite, 0));
  // pontãozinho na base, com caixotes e cabo enrolado
  g.add(ol(box(130, 8, 80, 0, WATER + 3, S + 62, C.wood[1], 1.5), 0.006));
  g.add(caixote(34, 22, 28, -38, WATER + 7, S + 60, true), caixote(30, 20, 26, -36, WATER + 29, S + 58));
  for (let k = 0; k < 3; k++) { const r = new THREE.Mesh(new THREE.TorusGeometry(u(14 - k * 2), u(3), 6, 16), flat(C.rope)); r.rotation.x = Math.PI / 2; r.position.set(u(36), u(WATER + 10 + k * 5), u(S + 66)); g.add(ol(r, 0.002)); }
  g.userData.legs = L.map(([sx, sz]) => [sx * S, sz * S]); g.userData.dock = [0, S + 62];
  return g;
}

// --- serras em silhueta no horizonte, em duas camadas (a de trás mais clara, quase da cor do céu): faixa fechada em
// volta de tudo, crista por soma de senos, sem luz nem contorno (perspectiva atmosférica)
// o pé da serra some na mesma névoa cor do horizonte em que o mar some (bruma subindo da água), a crista fica nítida
function serra(R, base, amp, col, seed, mist) {
  const n = 360, pos = [], idx = [];
  for (let i = 0; i <= n; i++) { const a = i / n * Math.PI * 2;
    const k = Math.abs(0.55 * Math.sin(a * 3 + seed) + 0.3 * Math.sin(a * 7 + seed * 2.1) + 0.15 * Math.sin(a * 19 + seed * 0.7));   // frequências inteiras: a volta fecha sem degrau
    const h = base + amp * Math.pow(k, 1.3), x = Math.sin(a) * R, z = -Math.cos(a) * R;
    pos.push(u(x), u(WATER - 200), u(z), u(x), u(WATER + h), u(z)); }
  for (let i = 0; i < n; i++) { const a = i * 2; idx.push(a, a + 2, a + 1, a + 1, a + 2, a + 3); }
  const g = new THREE.BufferGeometry(); g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3)); g.setIndex(idx);
  return new THREE.Mesh(g, new THREE.ShaderMaterial({ side: THREE.DoubleSide, fog: false,
    uniforms: { uC: { value: new THREE.Color(col) }, uH: { value: new THREE.Color(HAZE) } },
    vertexShader: `varying float vY; void main() { vY = position.y / ${UNIT.toFixed(6)} - (${WATER.toFixed(1)}); gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0); }`,
    fragmentShader: `uniform vec3 uC, uH; varying float vY; void main() { gl_FragColor = vec4(mix(uH, uC, smoothstep(-20.0, ${mist.toFixed(1)}, vY)), 1.0); }` }));
}

export async function buildJoanaArena() {
  crowdImg = await new Promise(ok => { const i = new Image(); i.onload = () => ok(i); i.onerror = () => ok(null); i.src = SPR + 'crowd.png'; });
  const all = new THREE.Group();
  all.add(plataforma(), agua()); all.children[1].userData.anim = { k: 'water', a: 0.035 };
  // fim de tarde: sol baixo dourado de lado e por trás (contraluz: as bordas acendem), sombra fria azul, céu de pêssego
  // no horizonte até azul fundo, e o disco do sol no terço esquerdo, atrás do adversário (o brilho na água vem dele)
  all.userData.look = { tint: [0.78, 0.86, 1.2], horizon: HAZE, fog: [35, 122],   // o mar fica todo cor de névoa antes do pé das serras (137 m)
    zenith: '#3b6cb4', sun: { color: '#ffc98e', int: 2.2, pos: [-3.2, 2.0, -2.4] },
    hemi: { sky: '#ffe0bc', ground: '#2f6f86', int: 1.45 }, disc: SUN_DIR.toArray(), discColor: SUN_COL };
  const E = HALF + 12;
  // barcos em volta, balançando na água
  const boats = [];
  // barcos espalhados à toa em volta (sem fileira): posições sorteadas com espaço mínimo entre eles, longe da
  // plataforma e fora do corredor da câmera; tamanhos e rumos variados. Maiores e mais altos que no estudo, sem plateia
  const hash = n => { const v = Math.sin(n * 127.1 + 311.7) * 43758.5453; return v - Math.floor(v); };
  const placed = [];
  // dois barcos amarrados nas laterais do ringue (a plateia que chegou pelo mar): cabos dos postes até a proa e a popa,
  // caixotes de peixe no do lado esquerdo, a âncora velha cheia de cracas pendurada na proa do da direita
  const TALL = 1.35;
  const keep = o => { const cg = new THREE.Group(); cg.scale.y = 1 / TALL; o.add(cg); return cg; };   // filhos sem o esticão do casco
  [[-505, -30, Math.PI / 2 + 0.05, 0.92, 3], [505, 20, -Math.PI / 2 - 0.04, 0.9, 0]].forEach(([x, z, ry, sc, th], i) => {
    const b = barcoPlateia(sc, th, 0), q = v => u(v * sc); b.scale.y = TALL; b.position.set(u(x), u(WATER - 40 * sc * TALL), u(z)); b.rotation.y = ry; all.add(b);
    b.userData.anim = { k: 'bob', a: u(4) * 0.3 }; boats.push({ b, ph: i * 2.1, y0: b.position.y, ry, amp: 0.3 }); placed.push([x, z, sc]);
    const ax = Math.cos(ry), az = -Math.sin(ry);
    for (const f of [-0.32, 0, 0.3]) addObs(x + ax * f * 700 * sc, z + az * f * 700 * sc, 130 * sc * (f > 0.2 ? 0.75 : 1));
    b.updateMatrixWorld(true);
    const at = (lx, ly) => b.localToWorld(new THREE.Vector3(q(lx), q(ly), 0)).divideScalar(UNIT).toArray();
    const side = Math.sign(x), post = z2 => [side * E, 30, z2];
    const bow = at(368, 190), stern = at(-318, 132);   // cada cabo vai ao poste do lado mais perto
    all.add(cabo(post(Math.sign(bow[2] - z) * 188), bow, 26), cabo(post(Math.sign(stern[2] - z) * 188), stern, 30));
    if (i === 0) { const cg = keep(b); cg.position.y = q(72);
      cg.add(caixote(56 * sc, 34 * sc, 46 * sc, -100 * sc, 0, -38 * sc), caixote(56 * sc, 34 * sc, 46 * sc, -100 * sc, 0, 30 * sc, true), caixote(52 * sc, 32 * sc, 44 * sc, -96 * sc, 34 * sc, -6 * sc),
        caixote(54 * sc, 32 * sc, 44 * sc, 80 * sc, 0, 10 * sc, true));
      for (let k = 0; k < 3; k++) { const r = new THREE.Mesh(new THREE.TorusGeometry(q(20 - k * 3), q(3.5), 6, 16), flat(C.rope)); r.rotation.x = Math.PI / 2; r.position.set(q(-260), q(6 + k * 6), 0); cg.add(ol(r, 0.002)); } }
    else { const cg = keep(b); cg.position.set(q(392), q(176), 0); const an = ancora(sc); an.rotation.y = Math.PI / 2; cg.add(an);   // braços abertos para quem olha a proa
      for (let k = 0; k < 4; k++) { const t = k / 4, l = new THREE.Mesh(new THREE.TorusGeometry(q(5), q(1.6), 5, 10), flat('#3d4249')); l.position.set(0, q(2 + t * 34), 0); l.rotation.y = k % 2 ? Math.PI / 2 : 0; cg.add(l); }
      cg.add(caixote(56 * sc, 34 * sc, 46 * sc, -500 * sc, -140 * sc, 0, true)); }   // no estrado (72 * TALL)
  });
  all.add(rede(-E - 12, -170, 170, 66));
  // torre de vigia no terço direito, de frente para a câmera; espuma em volta das pernas e do pontão
  const TX = 430, TZ = -1150, TR = Math.atan2(-TX, 705 - TZ), tw = torre(); tw.position.set(u(TX), 0, u(TZ)); tw.rotation.y = TR; all.add(tw);
  const tws = ([lx, lz]) => [TX + lx * Math.cos(TR) + lz * Math.sin(TR), TZ - lx * Math.sin(TR) + lz * Math.cos(TR)];
  tw.userData.legs.forEach(l => addObs(...tws(l), 9)); addObs(...tws(tw.userData.dock), 55);
  placed.push([TX, TZ, 1.2]);
  for (let n = 0, k = 0; n < 400 && placed.length < 20; n++) {
    const a = hash(n) * Math.PI * 2, r = 650 + Math.pow(hash(n + 0.5), 0.8) * 1500, x = Math.sin(a) * r, z = -Math.cos(a) * r;
    const len = 330 + hash(n + 0.25) * 140, sc = len / 700 * 1.55;
    if (z > 200 && Math.abs(x) < 900 && r < 1800) continue;                          // corredor da câmera
    if (Math.abs(x) < 520 && Math.abs(z) < 520) continue;                              // plataforma
    if (z < 0 && Math.abs(Math.atan2(x, 705 - z)) < 0.15) continue;                     // mar aberto atrás do adversário
    if (placed.some(p => Math.hypot(p[0] - x, p[1] - z) < (p[2] + sc) * 380)) continue;   // espaço entre os barcos
    placed.push([x, z, sc, hash(n + 0.75) * Math.PI * 2, k++]);
  }
  placed.filter(p => p.length > 3).forEach(([x, z, sc, ang, k]) => {   // (os 3 primeiros só reservam lugar: amarrados e torre)
    const b = barcoPlateia(sc, k, 0); b.scale.y = TALL;
    for (const f of [-0.32, 0, 0.3]) addObs(x + Math.cos(ang) * f * 700 * sc, z + Math.sin(ang) * f * 700 * sc, 130 * sc * (f > 0.2 ? 0.75 : 1));
    b.position.set(u(x), u(WATER - 40 * sc * TALL), u(z)); b.rotation.y = -ang; all.add(b); b.userData.anim = { k: 'bob', a: u(4) }; boats.push({ b, ph: k * 1.37, y0: b.position.y, ry: -ang, amp: 1 });   // linha d'água na altura 40 do estudo
  });
  // ilhas sem casas em volta de todo o horizonte (inclusive do lado da câmera); a do fundo com a pontezinha de pedra
  const put = (o, x, z, ry = 0) => { o.position.set(u(x), u(WATER - 6), u(z)); o.rotation.y = ry; all.add(o); if (o.userData.r) addObs(x, z, o.userData.r); return o; };
  put(ilha(420, 260, 300, [], [[-260, 140, 0.25], [230, 120, -0.2], [60, -160, 0.1], [-60, 40, -0.15]], 1), -1150, -2550);
  put(ilha(240, 150, 200, [], [[-120, 80, 0.2], [120, 60, -0.25]], 3), -470, -2700);
  { const br = new THREE.Group(); const sh = new THREE.Shape(); sh.moveTo(u(-90), 0); sh.lineTo(u(90), 0); sh.lineTo(u(90), u(70)); sh.lineTo(u(-90), u(70)); sh.closePath();
    const hole = new THREE.Path(); hole.moveTo(u(-45), 0); hole.lineTo(u(45), 0); hole.absarc(0, 0, u(45), 0, Math.PI, false); sh.holes.push(hole);
    const m = new THREE.Mesh(new THREE.ExtrudeGeometry(sh, { depth: u(40), bevelEnabled: false, curveSegments: 12 }), flat('#cbb89a')); br.add(ol(m, 0.006)); put(br, -740, -2630); }
  const ring = [[-62, 2500, 200], [-35, 2900, 320], [-12, 3100, 160], [20, 2800, 260], [48, 2600, 180], [75, 3000, 300], [105, 2700, 210], [135, 3200, 340], [160, 2600, 190],
                [180, 3000, 280], [205, 2700, 230], [232, 3100, 320], [258, 2800, 200], [290, 3000, 260], [318, 2650, 180]];   // [ângulo a partir do fundo, distância, tamanho]
  ring.forEach(([deg, d, sz], i) => { const a = deg * Math.PI / 180, x = Math.sin(a) * d, z = -Math.cos(a) * d; if (Math.abs(deg) < 8) return;
    const palms = []; for (let k = 0; k < 1 + (i % 3); k++) palms.push([(k - 1) * sz * 0.45, ((k * 37) % 3 - 1) * sz * 0.3, (k % 2 ? -1 : 1) * 0.22]);
    put(ilha(sz, sz * (0.55 + (i % 3) * 0.12), sz * 0.8, [], palms, 9 + i), x, z, i); });
  all.add(serra(13600, 260, 1300, '#f3b994', 1.7, 700), serra(11200, 120, 900, '#dc9a92', 4.2, 520));   // 166 m e 137 m
  // nuvens em volta do céu
  for (let k = 0; k < 22; k++) {
    const cl = new THREE.Group(), n = 4 + (k % 3);
    for (let i = 0; i < n; i++) {
      const r = 70 + ((k * 7 + i * 13) % 5) * 18, b = new THREE.Mesh(new THREE.IcosahedronGeometry(u(r), 1), flat(i % 2 ? '#ffffff' : '#eef6fb'));
      b.position.set(u((i - (n - 1) / 2) * 95 + ((i * 31) % 40)), u(((i * 17) % 3) * 22), u((i % 2) * 40)); b.scale.y = 0.62; cl.add(ol(b, 0.006));
    }
    const ang = k / 22 * Math.PI * 2 + ((k * 0.37) % 0.2), dist = 3200 + (k % 4) * 450;
    cl.position.set(u(Math.sin(ang) * dist), u(1100 + ((k * 211) % 5) * 160), u(-Math.cos(ang) * dist)); cl.lookAt(0, cl.position.y, 0); cl.scale.setScalar(1 + (k % 3) * 0.35); all.add(cl);
  }
  // balanço dos barcos (sobe, desce e joga um pouco), no mesmo relógio da água
  const water = all.children[1], prev = water.onBeforeRender;
  water.onBeforeRender = (...a) => { prev(...a); const t = tick.t.value; for (const { b, ph, y0, ry, amp } of boats) { b.position.y = y0 + u(4) * amp * Math.sin(t * 1.3 + ph); b.rotation.set(0.03 * amp * Math.sin(t * 1.1 + ph), ry, 0.025 * amp * Math.sin(t * 0.9 + ph * 0.7)); }
    const { geo, base } = tw.userData.flag, fp = geo.attributes.position;
    for (let i = 0; i < fp.count; i++) { const x = base[i * 3], k = x / u(84); fp.setZ(i, u(9) * k * Math.sin(x / u(84) * 7 - t * 5.5) ); fp.setY(i, base[i * 3 + 1] - u(5) * k * k); }
    fp.needsUpdate = true; geo.computeVertexNormals(); };
  return all;
}
