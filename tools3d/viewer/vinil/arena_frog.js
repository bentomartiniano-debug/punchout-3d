// Brejo do King Frog (arena do Frog), em código e no acabamento do vinil, pelo concept arena-concepts/04-king-frog-brejo.png
// e o kit (árvore de raízes, taboas): o ringue é um tablado de tábuas atravessadas (marrom com manchas de musgo) sobre
// estacas na água, com tocos musgosos nos cantos amarrados com corda e duas cordas claras; em volta a água verde-azulada
// do brejo com vitórias-régias e flores brancas, barrancos de argila laranja com capim no alto, touceiras de taboa,
// árvores grandes de raízes com barba-de-velho pendurada e a mata ao fundo, céu azul de meio-dia. Unidades do jogo.
import { THREE, flat, withOutline } from './core.js';
import { RoundedBoxGeometry } from 'three/addons/geometries/RoundedBoxGeometry.js';
import { mergeGeometries } from 'three/addons/utils/BufferGeometryUtils.js';
import { UNIT } from './arena_pernambuco.js';

const u = v => v * UNIT;
const HALF = 270, WATER = -40, POST_TOP = 125, ROPES = [38, 68, 98];
const C = { wood: ['#8a5a3a', '#7a4e33', '#94633f', '#6e4630'], moss: ['#8db53e', '#7ea83a', '#a2c24a'], beam: '#5b3a25', stump: '#7a5233', mossDark: '#5f8a2c',
            rope: '#efe0bd', ropeDark: '#8a5a30', water: '#2a9fae', clay: ['#d8743a', '#c9652f', '#e0844a'], grass: '#7fb43a', grassDark: '#5f9a2e',
            leaf: ['#6fa63a', '#5f9a32', '#7fb446'], trunk: ['#7a5236', '#8a5e3c', '#6a462e'], lily: ['#7fbf3e', '#6aac36'], flower: '#f6f1e0', cattail: '#7a4a2a' };
const ol = (m, w = 0.007) => { const g = withOutline(m, w); g.position.copy(m.position); g.quaternion.copy(m.quaternion); g.scale.copy(m.scale); m.position.set(0, 0, 0); m.quaternion.identity(); m.scale.set(1, 1, 1); return g; };
const box = (w, h, d, x, y, z, col, r = 1.5) => { const m = new THREE.Mesh(new RoundedBoxGeometry(u(w), u(h), u(d), 2, u(Math.min(r, w / 2 - 0.01, h / 2 - 0.01, d / 2 - 0.01))), typeof col === 'string' ? flat(col) : col); m.position.set(u(x), u(y), u(z)); return m; };
const hash = n => { const v = Math.sin(n * 127.1 + 311.7) * 43758.5453; return v - Math.floor(v); };
const tick = { t: { value: 0 } };
// vento: cada planta balança em volta da própria base (taboas, copas, barba-de-velho, folhas na água)
const flutua = o => { o.userData.anim = { k: 'bob', a: 0.018 }; return o; };
const sway = [], swing = (o, amp, sp) => { o.userData.anim = { k: 'sway', a: amp * 0.6 }; sway.push({ o, amp, sp, ph: Math.random() * 6.28, rx: o.rotation.x, rz: o.rotation.z, y: o.position.y }); return o; };
const facet = c => flat(c);
// bloco facetado (pedra/argila/copa): icosaedro sacudido, escalado
function lump(r, sx, sy, sz, col, seed, detail = 3) {
  const g = new THREE.IcosahedronGeometry(u(r), detail), p = g.attributes.position;
  for (let i = 0; i < p.count; i++) { const X = p.getX(i) / u(r), Y = p.getY(i) / u(r), Z = p.getZ(i) / u(r), k = 1 + 0.1 * Math.sin(X * 4 + seed) * Math.sin(Y * 3 + seed * 1.7) * Math.sin(Z * 4 + seed * 0.6) + 0.06 * Math.sin(X * 7 + Z * 5 + seed); p.setXYZ(i, p.getX(i) * sx * k, p.getY(i) * sy * k, p.getZ(i) * sz * k); }
  g.computeVertexNormals(); return new THREE.Mesh(g, facet(col));
}

// ================= tablado: tábuas atravessadas (de um lado ao outro), emendas desencontradas, umas tábuas de musgo;
// vigas de borda, travessas e estacas descendo na água; tocos musgosos nos cantos com a corda enrolada; duas cordas
function tablado() {
  const g = new THREE.Group(), W = 2 * HALF + 26, PW = 42, N = Math.ceil(W / PW);
  for (let i = 0; i < N; i++) {
    const z = -W / 2 + PW * (i + 0.5), cuts = [0.3 + 0.2 * hash(i), 0.62 + 0.18 * hash(i + 9)].filter((c, k) => k === 0 || hash(i + 3) > 0.4);
    let a = -W / 2;
    [...cuts, 1].forEach((c, k) => { const b = -W / 2 + W * c, len = b - a - 3;
      g.add(ol(box(len, 8, PW - 3, (a + b) / 2, -4, z, C.wood[(i * 3 + k) % 4], 1.2), 0.004));
      if (hash(i * 5 + k * 11) < 0.45) { const ml = len * (0.25 + 0.45 * hash(i + k * 13)), mx = a + 2 + (len - ml) * hash(i * 3 + k);   // mancha de musgo cobrindo parte da tábua
        g.add(box(ml, 1, PW - 6, mx + ml / 2, 0.3, z, C.moss[(i + k) % 3], 0.4)); }
      a = b; });
  }
  const E = HALF + 14;
  for (const s of [-1, 1]) { g.add(ol(box(24, 18, 2 * E + 24, s * E, -15, 0, C.beam, 2), 0.006)); g.add(ol(box(2 * E + 24, 18, 24, 0, -15, s * E, C.beam, 2), 0.006)); }
  for (const x of [-E, -90, 90, E]) for (const z of [-E, -90, 90, E]) g.add(ol(box(20, 70, 20, x, -45, z, C.beam, 2), 0.005));   // estacas
  // tocos dos cantos: tronco facetado, coroa de musgo e duas voltas de corda
  for (const [cx, cz] of [[-HALF, HALF], [HALF, HALF], [HALF, -HALF], [-HALF, -HALF]]) {
    const tg = new THREE.CylinderGeometry(u(24), u(30), u(POST_TOP + 40), 14, 4), p = tg.attributes.position;
    for (let i = 0; i < p.count; i++) { const k = 1 + 0.12 * (hash(cx + cz * 3 + i) - 0.5); p.setX(i, p.getX(i) * k); p.setZ(i, p.getZ(i) * k); }
    tg.computeVertexNormals(); const t = new THREE.Mesh(tg, facet(C.stump)); t.position.set(u(cx), u((POST_TOP - 40) / 2), u(cz)); g.add(ol(t, 0.006));
    const top = lump(30, 1, 0.55, 1, C.moss[0], cx * 0.3 + cz); top.position.set(u(cx), u(POST_TOP + 2), u(cz)); g.add(ol(top, 0.006));
    for (const y of [20, 80]) { const m = lump(31, 1, 0.9, 1, C.mossDark, cx + y + cz * 2, 2); m.position.set(u(cx), u(y), u(cz)); m.scale.y = 0.6; g.add(m); }
    for (const h of ROPES) for (const dy of [-5, 5]) { const r = new THREE.Mesh(new THREE.TorusGeometry(u(27), u(4), 6, 18), flat(C.ropeDark)); r.rotation.x = Math.PI / 2; r.position.set(u(cx), u(h + dy), u(cz)); g.add(ol(r, 0.003)); }
  }
  const corners = [[-HALF, HALF], [HALF, HALF], [HALF, -HALF], [-HALF, -HALF]];
  for (let i = 0; i < 4; i++) { const [ax, az] = corners[i], [bx, bz] = corners[(i + 1) % 4];
    ROPES.forEach(h => { const a = new THREE.Vector3(u(ax), u(h), u(az)), b = new THREE.Vector3(u(bx), u(h), u(bz)), len = a.distanceTo(b);
      const cg = new THREE.CylinderGeometry(u(3.4), u(3.4), len, 8); cg.rotateZ(Math.PI / 2);
      const m = new THREE.Mesh(cg, flat(C.rope)); m.position.copy(a).add(b).multiplyScalar(0.5); m.rotation.y = -Math.atan2(b.z - a.z, b.x - a.x); g.add(ol(m, 0.004)); }); }
  return g;
}

// ================= água do brejo: verde-azulada, rede clara de reflexos andando devagar, mais clara junto às bordas
function agua() {
  const g = new THREE.PlaneGeometry(u(14000), u(14000), 120, 120); g.rotateX(-Math.PI / 2);
  const m = flat(C.water);
  m.onBeforeCompile = sh => {
    sh.uniforms.uT = tick.t; sh.uniforms.uLine = { value: new THREE.Color('#7fd8d0') }; sh.uniforms.uDeep = { value: new THREE.Color('#1f7f94') };
    sh.vertexShader = 'varying vec2 vW;\n' + sh.vertexShader.replace('#include <begin_vertex>', '#include <begin_vertex>\nvW = position.xz / ' + UNIT.toFixed(6) + ';');
    sh.fragmentShader = 'uniform float uT; uniform vec3 uLine, uDeep; varying vec2 vW;\n' + `
      vec2 h2(vec2 p) { p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3))); return fract(sin(p) * 43758.5453); }
      float cellEdge(vec2 p, float t) { vec2 i = floor(p), f = fract(p); float m1 = 8.0, m2 = 8.0;
        for (int y = -1; y <= 1; y++) for (int x = -1; x <= 1; x++) { vec2 gg = vec2(x, y), o = 0.5 + 0.42 * sin(t * 0.5 + 6.2831 * h2(i + gg)); vec2 r = gg + o - f; float d = dot(r, r);
          if (d < m1) { m2 = m1; m1 = d; } else if (d < m2) m2 = d; }
        return sqrt(m2) - sqrt(m1); }
    ` + sh.fragmentShader.replace('#include <map_fragment>', `#include <map_fragment>
      { float r = length(vW); diffuseColor.rgb = mix(diffuseColor.rgb, uDeep, smoothstep(500.0, 2500.0, r) * 0.5);
        float e = cellEdge(vW / 120.0 + vec2(uT * 0.03, uT * 0.02), uT), w = fwidth(e) * 1.2;
        diffuseColor.rgb = mix(diffuseColor.rgb, uLine, (1.0 - smoothstep(0.03 - w, 0.03 + w, e)) * 0.22 * (1.0 - smoothstep(1200.0, 3000.0, r))); }`)
      .replace('#include <opaque_fragment>', 'outgoingLight = mix(outgoingLight, diffuseColor.rgb, 0.3);\n#include <opaque_fragment>');
  };
  m.customProgramCacheKey = () => 'agua-brejo';
  const w = new THREE.Mesh(g, m); w.position.y = u(WATER); w.onBeforeRender = () => { const t = tick.t.value = performance.now() / 1000;
    for (const s of sway) { const a = t * s.sp + s.ph; s.o.rotation.x = s.rx + Math.sin(a) * s.amp; s.o.rotation.z = s.rz + Math.sin(a * 0.8 + 1.7) * s.amp * 0.7; } };
  return w;
}

// ================= vitória-régia (disco com o corte) e flor branca de pétalas em ponta
function lilyPad(r, seed) {
  const sh = new THREE.Shape(), a0 = 0.25; sh.moveTo(0, 0); sh.absarc(0, 0, u(r), a0, Math.PI * 2 - 0.05, false); sh.lineTo(0, 0);
  const g = new THREE.ExtrudeGeometry(sh, { depth: u(2), bevelEnabled: false, curveSegments: 14 }); g.rotateX(-Math.PI / 2);
  const m = new THREE.Mesh(g, flat(C.lily[seed % 2])); m.rotation.y = hash(seed) * 6.28; return m;
}
function flower(seed) {
  const g = new THREE.Group();
  for (let ring = 0; ring < 2; ring++) for (let k = 0; k < 6; k++) { const a = (k + ring * 0.5) / 6 * Math.PI * 2;
    const pg = new THREE.ConeGeometry(u(5), u(16 - ring * 4), 4); pg.translate(0, u(8), 0); const p = new THREE.Mesh(pg, flat(C.flower));
    p.position.set(u(Math.cos(a) * 3), u(2), u(Math.sin(a) * 3)); p.quaternion.setFromUnitVectors(new THREE.Vector3(0, 1, 0), new THREE.Vector3(Math.cos(a), 1.2 + ring * 0.8, Math.sin(a)).normalize()); g.add(p); }
  const c = new THREE.Mesh(new THREE.SphereGeometry(u(4), 8, 6), flat('#f2c94c')); c.position.y = u(5); g.add(c);
  return g;
}
// ================= taboa: touceira de folhas em lâmina e hastes com a espiga marrom
function taboas(n, seed, s = 1) {
  const g = new THREE.Group(), leaves = [], spikes = [];
  for (let i = 0; i < n; i++) { const a = hash(seed + i) * 6.28, r = hash(seed + i * 2) * 26 * s, h = (90 + hash(seed + i * 3) * 90) * s, lean = 0.15 + hash(seed + i * 5) * 0.35;
    const lg = new THREE.ConeGeometry(u(5 * s), u(h), 3); lg.scale(1, 1, 0.3); lg.translate(0, u(h / 2), 0);
    lg.applyMatrix4(new THREE.Matrix4().makeRotationFromEuler(new THREE.Euler(Math.cos(a) * lean, a, Math.sin(a) * lean))); lg.translate(u(Math.cos(a) * r), 0, u(Math.sin(a) * r)); leaves.push(lg);
    if (i % 2 === 0) { const sh = (130 + hash(seed + i * 7) * 70) * s, sx = Math.cos(a) * r * 0.6, sz = Math.sin(a) * r * 0.6;
      const st = new THREE.CylinderGeometry(u(1.4), u(1.6), u(sh), 5); st.translate(u(sx), u(sh / 2), u(sz)); leaves.push(st);
      const sp = new THREE.CapsuleGeometry(u(5 * s), u(24 * s), 4, 8); sp.translate(u(sx), u(sh - 6), u(sz)); spikes.push(sp); } }
  g.add(ol(new THREE.Mesh(mergeGeometries(leaves.map(x => x.index ? x.toNonIndexed() : x)), flat(C.leaf[seed % 3])), 0.003));
  if (spikes.length) g.add(ol(new THREE.Mesh(mergeGeometries(spikes.map(x => x.index ? x.toNonIndexed() : x)), flat(C.cattail)), 0.003));
  swing(g, 0.05, 1.3 + hash(seed) * 0.8);
  return g;
}
// ================= barranco de argila: bloco facetado laranja com a tampa de capim e umas touceiras
function barranco(w, h, d, seed) {
  // como no concept: paredão de argila de contorno orgânico, lados levemente abaulados com um lábio de grama no alto
  // e a tampa levemente ondulada; malha lisa (anéis), e a cor vem no shader pela altura: capim descendo em ondas pela
  // borda, argila com camadas (estratos) mais escuras. Nada de triângulo pintado.
  const A = 72, S = 14, T = 7, base = a => 0.86 + 0.08 * Math.sin(a * 2 + seed) + 0.05 * Math.sin(a * 5 + seed * 1.3) + 0.025 * Math.sin(a * 11 + seed * 2.1);
  const pos = [], idx = [];
  for (let r = 0; r <= S + T; r++) for (let k = 0; k < A; k++) { const a = k / A * Math.PI * 2, b = base(a); let rad, y;
    if (r <= S) { const t = r / S; y = t * h; rad = b * (0.97 + 0.03 * Math.sin(t * 7 + a * 3 + seed) + 0.05 * Math.max(0, t - 0.82) / 0.18 - 0.04 * (1 - t)); }
    else { const t = (r - S) / T; rad = b * 1.02 * (1 - t) ** 0.8; y = h + (1 - (1 - t) ** 2) * 10 + 4 * Math.sin(a * 3 + t * 4 + seed) * t; }
    pos.push(u(Math.cos(a) * rad * w / 2), u(y), u(Math.sin(a) * rad * d / 2)); }
  for (let r = 0; r < S + T; r++) for (let k = 0; k < A; k++) { const a = r * A + k, b = r * A + (k + 1) % A, c = a + A, e = b + A; idx.push(a, c, b, b, c, e); }
  const geo = new THREE.BufferGeometry(); geo.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3)); geo.setIndex(idx); geo.computeVertexNormals();
  const m = flat('#ffffff');
  m.onBeforeCompile = sh => {
    Object.assign(sh.uniforms, { uH: { value: h }, uSeed: { value: seed }, uClay: { value: new THREE.Color(C.clay[seed % 3]) }, uClayD: { value: new THREE.Color('#b0532a') }, uGrass: { value: new THREE.Color(C.grass) }, uGrassD: { value: new THREE.Color(C.grassDark) } });
    sh.vertexShader = 'varying vec3 vL;\n' + sh.vertexShader.replace('#include <begin_vertex>', '#include <begin_vertex>\nvL = position / ' + UNIT.toFixed(6) + ';');
    sh.fragmentShader = 'uniform float uH, uSeed; uniform vec3 uClay, uClayD, uGrass, uGrassD; varying vec3 vL;\n' + sh.fragmentShader.replace('#include <map_fragment>', `#include <map_fragment>
      { float a = atan(vL.z, vL.x), y = vL.y;
        float drip = uH * 0.8 + uH * 0.07 * sin(a * 6.0 + uSeed) + uH * 0.05 * sin(a * 17.0 + uSeed * 2.0) - uH * 0.06 * pow(abs(sin(a * 9.0 + uSeed)), 6.0);
        float g = smoothstep(drip - 0.6, drip + 0.6, y);
        float band = fract(y / 26.0 + 0.15 * sin(a * 4.0 + uSeed));
        vec3 clay = mix(uClay, uClayD, step(0.78, band) * 0.6);
        float tuft = step(0.6, fract(sin(dot(floor(vL.xz / 18.0), vec2(12.9, 78.2))) * 43758.5)) * step(uH + 2.0, y);
        diffuseColor.rgb = mix(clay, mix(uGrass, uGrassD, tuft * 0.5), g); }`);
  };
  m.customProgramCacheKey = () => 'barranco-brejo';
  const g = new THREE.Group(); g.add(ol(new THREE.Mesh(geo, m), 0.006));
  for (let k = 0; k < 3; k++) { const t = taboas(5, seed * 3 + k, 0.8); t.position.set(u((hash(seed + k) - 0.5) * w * 0.6), u(h + 6), u((hash(seed + k * 2) - 0.5) * d * 0.6)); g.add(t); }
  for (let k = 0; k < 2; k++) { const f = samambaia(seed * 2 + k); f.position.set(u((hash(seed + k * 4) - 0.5) * w * 0.5), u(h + 6), u((hash(seed + k * 6) - 0.5) * d * 0.5)); g.add(f); }
  // pedras arredondadas no pé do paredão, meio enterradas na água
  for (let k = 0; k < 4; k++) { const a = hash(seed * 9 + k) * 6.28, b = base(a) * 1.02, st = lump(18 + hash(seed + k) * 16, 1.2, 0.7, 1, '#8a7a6a', seed * 4 + k, 2);
    st.position.set(u(Math.cos(a) * b * w / 2), u(30), u(Math.sin(a) * b * d / 2)); g.add(ol(st, 0.004)); }
  return g;
}
// ================= samambaia: folhas largas em leque, abrindo do centro
function samambaia(seed) {
  const parts = [];
  for (let k = 0; k < 8; k++) { const a = k / 8 * Math.PI * 2 + hash(seed + k), L = 50 + hash(seed + k * 3) * 30, lg = new THREE.ConeGeometry(u(12), u(L), 4);
    lg.scale(1, 1, 0.25); lg.translate(0, u(L / 2), 0); lg.rotateX(0.9 + hash(seed * 2 + k) * 0.4); lg.rotateY(a); parts.push(lg.toNonIndexed()); }
  return swing(ol(new THREE.Mesh(mergeGeometries(parts), facet(C.leaf[seed % 3])), 0.003), 0.04, 1.1);
}
// ================= árvore de raízes do kit: tronco grosso facetado que abre em raízes, galhos tortos, copas
// facetadas em guarda-chuva e a barba-de-velho caindo das copas
function arvore(s, seed) {
  const g = new THREE.Group(), tc = C.trunk[seed % 3], q = v => u(v * s);
  const tg = new THREE.CylinderGeometry(q(28), q(46), q(320), 16, 8), p = tg.attributes.position;
  for (let i = 0; i < p.count; i++) { const y = p.getY(i) / q(320) + 0.5, k = 1 + 0.25 * (hash(seed + i * 1.3) - 0.5) + 0.6 * Math.max(0, 0.25 - y) * 4; p.setX(i, p.getX(i) * k + Math.sin(y * 3 + seed) * q(14)); p.setZ(i, p.getZ(i) * k); }
  tg.computeVertexNormals(); const t = new THREE.Mesh(tg, facet(tc)); t.position.y = q(160); g.add(ol(t, 0.006));
  for (let k = 0; k < 5; k++) { const a = k / 5 * 6.28 + seed, rg = new THREE.ConeGeometry(q(16), q(110), 10); rg.translate(0, q(55), 0);
    const r = new THREE.Mesh(rg, facet(tc)); r.position.set(q(Math.cos(a) * 30), q(40), q(Math.sin(a) * 30)); r.quaternion.setFromUnitVectors(new THREE.Vector3(0, 1, 0), new THREE.Vector3(Math.cos(a), -0.55, Math.sin(a)).normalize()); g.add(ol(r, 0.004)); }
  const tops = [[0, 380, 0, 140], [-150, 320, 40, 105], [140, 300, -30, 100], [40, 330, 120, 95]];
  tops.forEach(([x, y, z, r], k) => {
    const A = new THREE.Vector3(0, q(260), 0), B = new THREE.Vector3(q(x * 0.8), q(y - 20), q(z * 0.8)), len = A.distanceTo(B);
    if (k) { const br = new THREE.Mesh(new THREE.CylinderGeometry(q(9), q(14), len, 10), facet(tc)); br.position.copy(A).add(B).multiplyScalar(0.5); br.quaternion.setFromUnitVectors(new THREE.Vector3(0, 1, 0), B.clone().sub(A).normalize()); g.add(ol(br, 0.004)); }
    const pv = new THREE.Group(); pv.position.set(q(x), q(y - r * 0.3), q(z)); g.add(swing(pv, 0.035, 0.7 + hash(seed + k) * 0.4));
    const cp = lump(r, 1.15, 0.5, 1.15, C.leaf[(seed + k) % 3], seed * 7 + k, 3); cp.position.set(0, q(r * 0.3), 0); cp.scale.multiplyScalar(s); pv.add(ol(cp, 0.006));
    const moss = []; for (let m = 0; m < 9; m++) { const a = hash(seed + k * 11 + m) * 6.28, rr = r * (0.4 + 0.55 * hash(seed + m * 3)), L = 60 + hash(seed + k + m * 5) * 90;
      const mg = new THREE.ConeGeometry(q(8), q(L), 4); mg.rotateX(Math.PI); mg.translate(q(Math.cos(a) * rr * 1.1), q(-L / 2), q(Math.sin(a) * rr * 1.1)); moss.push(mg); }
    pv.add(new THREE.Mesh(mergeGeometries(moss.map(x => x.index ? x.toNonIndexed() : x)), facet('#9cc23f')));
  });
  return g;
}

export async function buildFrogArena() {
  const all = new THREE.Group();
  all.add(tablado(), agua()); all.children[1].userData.anim = { k: 'water', a: 0.02 };
  // meio-dia claro: sol alto de frente-direita, céu azul, sombra azulada
  all.userData.look = { tint: [0.8, 0.9, 1.12], horizon: '#cfe9f2', zenith: '#2e8fe0', fog: [45, 170],
    sun: { color: '#fff4d8', int: 2.0, pos: [1.6, 3.4, 1.8] }, hemi: { sky: '#d8eeff', ground: '#3f6a3a', int: 1.5 } };
  const put = (o, x, z, ry = 0, y = WATER) => { o.position.set(u(x), u(y), u(z)); o.rotation.y = ry; all.add(o); return o; };
  // vitórias-régias e flores espalhadas na água (longe do tablado e fora do corredor da câmera)
  for (let n = 0, k = 0; n < 500 && k < 90; n++) { const a = hash(n * 1.7) * 6.28, r = 340 + hash(n * 2.3) * 1300, x = Math.sin(a) * r, z = -Math.cos(a) * r;
    if (Math.abs(x) < HALF + 40 && Math.abs(z) < HALF + 40) continue;
    flutua(swing(put(lilyPad(18 + hash(n) * 22, n), x, z, 0, WATER + 0.5), 0.02, 0.9)); if (hash(n * 9.1) < 0.18) flutua(swing(put(flower(n), x, z, 0, WATER + 2), 0.06, 0.9)); k++; }
  // barrancos de argila em volta, abrindo um canal de água no fundo (como no concept), touceiras de taboa na beira
  const banks = [[-900, -700, 520, 90, 380], [-1350, -150, 600, 110, 520], [-1100, 600, 500, 90, 420], [900, -760, 560, 95, 400], [1350, -100, 620, 110, 520],
                 [1150, 650, 520, 90, 420], [-1900, -1500, 900, 80, 500], [1950, -1500, 900, 80, 520], [-600, -1900, 520, 70, 300], [650, -1950, 560, 70, 320],
                 [-1100, 1500, 620, 100, 460], [1150, 1550, 640, 100, 460], [-1900, 900, 700, 90, 520], [1950, 950, 700, 90, 520], [0, 2300, 900, 80, 500]];
  banks.forEach(([x, z, w, h, d], i) => put(barranco(w * 1.2, h * 1.7, d * 1.2, i + 1), x, z, hash(i) * 0.8 - 0.4, WATER - 8));
  for (let i = 0; i < 26; i++) { const a = hash(i * 5.3) * 6.28, r = 420 + hash(i * 1.9) * 600, x = Math.sin(a) * r, z = -Math.cos(a) * r; if (z > 380 && Math.abs(x) < 500) continue; put(taboas(9, i + 40), x, z); }
  // árvores grandes nos barrancos dos lados e ao fundo, mata baixa no horizonte
  [[-1250, -350, 1.5, 1], [1300, -420, 1.6, 2], [-1150, 750, 1.3, 3], [1200, 760, 1.35, 4], [-2000, -1450, 1.4, 5], [2050, -1450, 1.5, 6], [-700, -2000, 1.1, 7], [720, -2060, 1.15, 8], [-1150, 1600, 1.4, 9], [1200, 1650, 1.5, 10], [-2000, 1000, 1.3, 11], [2050, 1050, 1.35, 12], [150, 2400, 1.4, 13]]
    .forEach(([x, z, s, sd]) => { put(barranco(260 * s, 90, 260 * s, sd + 30), x, z, 0, WATER - 30); put(arvore(s, sd), x, z, hash(sd) * 6.28, WATER + 55); });   // cada árvore no seu monte de argila
  for (let i = 0; i < 46; i++) { const a = (i / 46 - 0.5) * Math.PI * 2, r = 3300 + hash(i) * 600, x = Math.sin(a) * r, z = -Math.cos(a) * r; if (Math.abs(a) < 0.3) continue;   // canal aberto até o horizonte, como no concept
    const t = arvore(1.6 + hash(i * 2) * 0.8, i + 20); put(t, x, z, hash(i) * 6.28, WATER); }
  // mata distante azulada, faixa contínua no horizonte
  for (let i = 0; i < 110; i++) { const a = (i / 110 - 0.5) * Math.PI * 2, r = 5200 + hash(i * 3) * 500, b = lump(260 + hash(i) * 160, 1.3, 0.8, 1, ['#4f8f86', '#5b9a8c', '#467f80'][i % 3], i + 200, 2);
    put(b, Math.sin(a) * r, -Math.cos(a) * r, 0, WATER + 120 + hash(i * 7) * 120); }
  return all;
}
