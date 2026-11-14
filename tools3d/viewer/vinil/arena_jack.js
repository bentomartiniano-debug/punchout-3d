// Jardim do Jack, em código e no acabamento do vinil, pelo concept arena-concepts/08-jack-jardim.png e o kit (colmeia
// aérea revisada, pérgola revisada). Composição do concept, da câmera para o fundo:
//  - frente: degrau de pedra e moitas enormes de rosas (vermelhas à esquerda, rosa à direita) com margaridas;
//  - ringue: lona creme, saia verde-escura com molduras douradas (folhas e a abelha no meio), postes verde-escuros
//    com a bola dourada, tampões vermelho (frente-esquerda), azul (frente-direita) e creme atrás;
//  - lados: cerquinha de madeira, pilares de pedra clara com vasos floridos, lavandas e dedaleiras altas, canteiros
//    cheios; à esquerda a colmeia grande no poste; à direita a pérgola de roseiras e os girassóis;
//  - fundo: muro/terraço de pedra clara atrás do ringue, caminho de pedra subindo até o chafariz sob o arco de rosas,
//    colmeias em postes, moitas de rosas, ciprestes e árvores redondas;
//  - longe: colinas verdes, o morro de pedra com a vila e a torre à esquerda, serra azulada e nuvens fofas.
// As flores e folhagens são instâncias (uma malha por tipo, cor por instância): milhares de flores, poucas chamadas.
import { THREE, flat, withOutline } from './core.js';
import { RoundedBoxGeometry } from 'three/addons/geometries/RoundedBoxGeometry.js';
import { mergeGeometries } from 'three/addons/utils/BufferGeometryUtils.js';
import { UNIT } from './arena_pernambuco.js';

const u = v => v * UNIT;
const HALF = 270, DECK = 46, POST_TOP = 125, ROPES = [38, 68, 98], Y0 = -DECK;
const C = { green: '#3c6a4c', greenD: '#2c5040', gold: '#d9b44a', cream: '#f0e6cc', red: '#c8322e', blue: '#2c4fb0', stone: '#ece4d0', stoneD: '#d4c8aa',
            wood: '#8a5e36', woodD: '#694626', grass: '#86b84e', path: '#d9c9a2' };
const ol = (m, w = 0.007) => { const g = withOutline(m, w); g.userData.olw = w; g.position.copy(m.position); g.quaternion.copy(m.quaternion); g.scale.copy(m.scale); m.position.set(0, 0, 0); m.quaternion.identity(); m.scale.set(1, 1, 1); return g; };
const box = (w, h, d, x, y, z, col, r = 3) => { const m = new THREE.Mesh(new RoundedBoxGeometry(u(w), u(h), u(d), 2, u(Math.min(r, w / 2 - 0.01, h / 2 - 0.01, d / 2 - 0.01))), typeof col === 'string' ? flat(col) : col); m.position.set(u(x), u(y), u(z)); return m; };
const cyl = (r0, r1, h, x, y, z, col, seg = 20) => { const m = new THREE.Mesh(new THREE.CylinderGeometry(u(r0), u(r1), u(h), seg), typeof col === 'string' ? flat(col) : col); m.position.set(u(x), u(y), u(z)); return m; };
const ball = (r, x, y, z, col, seg = 16) => { const m = new THREE.Mesh(new THREE.SphereGeometry(u(r), seg, Math.round(seg * 0.7)), flat(col)); m.position.set(u(x), u(y), u(z)); return m; };
const hash = n => { const v = Math.sin(n * 127.1 + 311.7) * 43758.5453; return v - Math.floor(v); };
const toonTex = tex => new THREE.MeshToonMaterial({ map: tex, gradientMap: flat('#fff').gradientMap });
function canvasTex(w, h, draw, rep) {
  const c = document.createElement('canvas'); c.width = w; c.height = h; draw(c.getContext('2d'), w, h);
  const t = new THREE.CanvasTexture(c); t.colorSpace = THREE.SRGBColorSpace; t.anisotropy = 8;
  if (rep) { t.wrapS = t.wrapT = THREE.RepeatWrapping; t.repeat.set(rep[0], rep[1]); } return t;
}
// junta as peças estáticas de cor chapada numa malha por cor e as cascas de contorno numa por espessura
// (de ~900 chamadas de desenho para algumas dezenas); peças com textura, transparentes ou animadas ficam como estão
export function fundeEstaticos(root) {
  root.updateMatrixWorld(true);
  const solid = new Map(), hulls = new Map(), dead = [];
  const limpa = (geo, mw) => { let g = geo.index ? geo.toNonIndexed() : geo.clone(); for (const k of Object.keys(g.attributes)) if (k !== 'position' && k !== 'normal') g.deleteAttribute(k); return g.applyMatrix4(mw); };
  root.traverse(o => {
    if (!o.isMesh || o.isInstancedMesh || o.userData.hull || o.userData.keep) return;
    const m = o.material; if (Array.isArray(m) || m.map || m.transparent || !m.isMeshToonMaterial || o.onBeforeRender !== THREE.Object3D.prototype.onBeforeRender) return;
    const key = m.color.getHexString() + '|' + (m.flatShading ? 1 : 0) + '|' + m.side, g = limpa(o.geometry, o.matrixWorld);
    (solid.get(key) || solid.set(key, { m, list: [] }).get(key)).list.push(g); dead.push(o);
    const hull = o.parent && o.parent.children.find(c => c.userData.hull && c.geometry === o.geometry);
    if (hull) { const w = o.parent.userData.olw; (hulls.get(w) || hulls.set(w, []).get(w)).push(g); dead.push(hull); }
  });
  for (const o of dead) o.removeFromParent();
  for (const { m, list } of solid.values()) root.add(new THREE.Mesh(mergeGeometries(list), m));
  for (const [w, list] of hulls) { const geo = mergeGeometries(list); root.add(withOutline(new THREE.Mesh(geo, flat('#fff')), w).children[1]); }
}
const merge = list => mergeGeometries(list.map(g => g.index ? g.toNonIndexed() : g));

// ================= protótipos (em unidades do jogo, "tamanho 1"), virados para +y
const P = {
  // rosa: botão no meio e três voltas de pétalas em concha, abrindo para fora (raio ~7); low-poly: ~260 triângulos
  rosa: (() => { const L = []; const bud = new THREE.IcosahedronGeometry(u(2.6), 0); bud.scale(1, 1.15, 1); bud.translate(0, u(1.4), 0); L.push(bud);
    [[3, 2.8, 2.4, 0.5, 1.6], [4, 3.8, 4.2, 0.9, 1.0], [5, 4.6, 6.0, 1.25, 0.2]].forEach(([n, r, d, tilt, y], k) => { for (let i = 0; i < n; i++) {
      const g = new THREE.SphereGeometry(u(r), 6, 3); g.scale(1, 0.42, 0.75); g.rotateZ(tilt); g.translate(u(d), u(y), 0); g.rotateY(i / n * Math.PI * 2 + k * 0.5); L.push(g); } });
    return merge(L); })(),
  // margarida: pétalas compridas em volta (o miolo é outra instância)
  marg: (() => { const L = []; for (let i = 0; i < 10; i++) { const g = new THREE.SphereGeometry(u(2.3), 5, 3); g.scale(2.6, 0.35, 1); g.translate(u(5.4), 0, 0); g.rotateY(i / 10 * Math.PI * 2); L.push(g); } return merge(L); })(),
  miolo: (() => { const g = new THREE.SphereGeometry(u(2.6), 6, 4); g.scale(1, 0.55, 1); return g; })(),
  // espiga (lavanda/dedaleira): florzinhas empilhadas afinando, altura 1 = 40 unidades
  espiga: (() => { const L = []; for (let i = 0; i < 9; i++) { const t = i / 8, r = 4.2 - t * 2.6; for (let k = 0; k < 3; k++) { const a = k / 3 * Math.PI * 2 + i * 0.9, g = new THREE.IcosahedronGeometry(u(r * 0.85), 0);
      g.translate(u(Math.cos(a) * r * 0.5), u(t * 40), u(Math.sin(a) * r * 0.5)); L.push(g); } } return merge(L); })(),
  haste: new THREE.CylinderGeometry(u(0.9), u(1.2), u(1), 5).translate(0, u(0.5), 0),
  folha: (() => { const g = new THREE.SphereGeometry(u(6), 5, 3); g.scale(1.6, 0.22, 0.6); g.translate(u(9), 0, 0); return g; })(),
  girP: (() => { const L = []; for (let i = 0; i < 14; i++) { const g = new THREE.SphereGeometry(u(3), 5, 3); g.scale(2.4, 0.35, 0.9); g.translate(u(13), 0, 0); g.rotateY(i / 14 * Math.PI * 2); L.push(g); } return merge(L); })(),
  girC: (() => { const g = new THREE.CylinderGeometry(u(9), u(9), u(3), 16); return g; })(),
  // folhagem: bloco facetado (moitas, sebes, copas)
  blob: (() => { const g = new THREE.IcosahedronGeometry(u(1), 1), p = g.attributes.position; for (let i = 0; i < p.count; i++) { const k = 1 + 0.12 * Math.sin(p.getX(i) * 9 + p.getY(i) * 7) * Math.cos(p.getZ(i) * 8); p.setXYZ(i, p.getX(i) * k, p.getY(i) * k, p.getZ(i) * k); } g.computeVertexNormals(); return g; })(),
};
const INST = new Map();   // nome -> lista de { m: Matrix4, c: Color }
const _q = new THREE.Quaternion(), _e = new THREE.Euler(), _up = new THREE.Vector3(0, 1, 0), _n = new THREE.Vector3(), _s = new THREE.Vector3();
function put(kind, x, y, z, s, col, normal, spin = 0) {
  if (normal) { _q.setFromUnitVectors(_up, _n.copy(normal).normalize()); _q.multiply(new THREE.Quaternion().setFromAxisAngle(_up, spin)); } else _q.setFromEuler(_e.set(0, spin, 0));
  if (typeof s === 'number') _s.set(s, s, s); else _s.set(...s);
  (INST.get(kind) || INST.set(kind, []).get(kind)).push({ m: new THREE.Matrix4().compose(new THREE.Vector3(u(x), u(y), u(z)), _q.clone(), _s.clone()), c: new THREE.Color(col) });
}
const VERDES = ['#3f7a34', '#4a8a3a', '#356a2e', '#5a9a40', '#2f5f2c'];
const verde = k => VERDES[Math.floor(hash(k) * VERDES.length)];
const varia = (col, k, a = 0.08) => { const c = new THREE.Color(col); c.offsetHSL(0, 0, (hash(k) - 0.5) * a); return '#' + c.getHexString(); };

// ================= plantas
// moita florida: vários blocos de folhagem sobrepostos e as flores voltadas para fora na casca
function moita(x, z, r, tipo, cols, seed, y0 = Y0, densidade = 1) {
  const nb = 5 + Math.floor(hash(seed) * 3), cy = y0 + r * 0.7;
  for (let i = 0; i < nb; i++) { const a = i / nb * Math.PI * 2 + hash(seed + i), d = r * 0.45, rr = r * (0.55 + hash(seed + i * 3) * 0.2);
    put('blob', x + Math.cos(a) * d, cy + (hash(seed + i * 5) - 0.4) * r * 0.4, z + Math.sin(a) * d, [rr * 1.1, rr * 0.85, rr * 1.1], verde(seed + i), null, a); }
  put('blob', x, cy + r * 0.25, z, [r * 0.7, r * 0.6, r * 0.7], verde(seed + 99));
  const longe = Math.hypot(x, z - 300) > 800 ? 0.45 : 1, n = Math.round(r * r / 22 * densidade * longe), fs = (tipo === 'rosa' ? r / 22 : r / 26) * (longe < 1 ? 1.25 : 1);
  for (let i = 0; i < n; i++) { const a = hash(seed * 7 + i) * Math.PI * 2, e = -0.15 + hash(seed * 3 + i * 7) * 1.5, R = r * 0.98;
    _n.set(Math.cos(a) * Math.cos(e), Math.sin(e), Math.sin(a) * Math.cos(e));
    const fx = x + _n.x * R * 1.08, fy = cy + _n.y * R * 0.82 + r * 0.1, fz = z + _n.z * R * 1.08, col = cols[i % cols.length], s = Math.max(0.8, fs * (0.85 + hash(seed + i) * 0.4));
    const nv = _n.clone();
    if (tipo === 'rosa') put('rosa', fx, fy, fz, s, varia(col, seed + i), nv, i);
    else { put('marg', fx, fy, fz, s * 0.9, varia(col, seed + i, 0.04), nv, i); put('miolo', fx + nv.x * 1.2 * s, fy + nv.y * 1.2 * s, fz + nv.z * 1.2 * s, s * 0.9, '#f2b82a', nv); } }
  for (let i = 0; i < Math.round(n / 3); i++) { const a = hash(seed + i * 11) * Math.PI * 2; put('folha', x + Math.cos(a) * r * 0.9, cy - r * 0.2, z + Math.sin(a) * r * 0.9, r / 18, verde(seed + i * 2), new THREE.Vector3(0, 1, 0), -a); }
}
// touceira de espigas: lavanda roxa ou dedaleira branca/rosa
function espigas(x, z, n, h, col, seed, y0 = Y0) {
  for (let i = 0; i < n; i++) { const a = hash(seed + i) * 6.28, d = hash(seed * 2 + i) * 18, hh = h * (0.75 + hash(seed + i * 3) * 0.45), tilt = new THREE.Vector3(Math.cos(a) * 0.12, 1, Math.sin(a) * 0.12);
    const px = x + Math.cos(a) * d, pz = z + Math.sin(a) * d;
    put('haste', px, y0, pz, [1, hh * 0.55, 1], verde(seed + i), tilt);
    put('espiga', px + tilt.x * hh * 0.5, y0 + hh * 0.5, pz + tilt.z * hh * 0.5, [hh / 60, hh / 40 * 0.55, hh / 60], varia(col, seed + i, 0.1), tilt); }
  for (let k = 0; k < 6; k++) put('folha', x, y0 + 4, z, 1.2, verde(seed + k), new THREE.Vector3(0, 1, 0), k);
}
function girassol(x, z, h, seed, y0 = Y0) {
  put('haste', x, y0, z, [2.2, h, 2.2], '#4a7a30');
  for (let k = 0; k < 3; k++) put('folha', x, y0 + h * (0.3 + k * 0.2), z, 2.2, verde(seed + k), new THREE.Vector3(Math.cos(k * 2) * 0.3, 1, Math.sin(k * 2) * 0.3), k * 2.1);
  const face = new THREE.Vector3(0.15 * (hash(seed) - 0.5), 0.35, 1);
  put('girP', x, y0 + h, z + 2, 1, varia('#f6c21c', seed, 0.06), face); put('girC', x + face.x * 1.5, y0 + h + 0.5, z + 3.5, 1, '#6a3a18', face);
}

// ================= ringue
function ringue() {
  const g = new THREE.Group(), E = HALF + 30;
  const lona = canvasTex(512, 512, (c, w) => { const k = w / (2 * E); c.fillStyle = C.green; c.fillRect(0, 0, w, w); c.fillStyle = C.cream; c.fillRect(18 * k, 18 * k, w - 36 * k, w - 36 * k); });
  const top = new THREE.Mesh(new THREE.BoxGeometry(u(2 * E), u(8), u(2 * E)), [flat(C.green), flat(C.green), toonTex(lona), flat(C.green), flat(C.green), flat(C.green)]); top.position.y = u(-4); g.add(ol(top, 0.004));
  const saia = canvasTex(1536, 192, (c, w, h) => { c.fillStyle = C.green; c.fillRect(0, 0, w, h); c.fillStyle = C.greenD; c.fillRect(0, h - 14, w, 14);
    const pw = (w - 60) / 3; c.strokeStyle = C.gold; c.lineWidth = 6; for (let i = 0; i < 3; i++) c.strokeRect(30 + i * pw + 14, 26, pw - 28, h - 58);
    c.fillStyle = C.gold;
    const raminho = (x, y, f) => { c.save(); c.translate(x, y); c.scale(f, 1); c.beginPath(); c.ellipse(0, 6, 12, 28, 0.25, 0, 7); c.fill(); c.beginPath(); c.ellipse(-22, -6, 10, 22, -0.6, 0, 7); c.fill(); c.beginPath(); c.ellipse(22, -6, 10, 22, 0.6, 0, 7); c.fill(); c.restore(); };
    raminho(30 + pw / 2, h / 2 - 4, 1); raminho(w - 30 - pw / 2, h / 2 - 4, 1);
    for (const f of [-1, 1]) { c.save(); c.translate(w / 2 + f * 150, h / 2); c.scale(f, 1); c.beginPath(); c.ellipse(0, 0, 10, 26, 0.6, 0, 7); c.fill(); c.restore(); }
    const bx = w / 2, by = h / 2 - 2;   // abelha
    c.beginPath(); c.ellipse(bx - 44, by - 14, 40, 18, -0.3, 0, 7); c.fill(); c.beginPath(); c.ellipse(bx + 44, by - 14, 40, 18, 0.3, 0, 7); c.fill();
    c.beginPath(); c.ellipse(bx - 34, by + 14, 26, 11, 0.4, 0, 7); c.fill(); c.beginPath(); c.ellipse(bx + 34, by + 14, 26, 11, -0.4, 0, 7); c.fill();
    c.beginPath(); c.ellipse(bx, by + 8, 14, 30, 0, 0, 7); c.fill(); c.beginPath(); c.arc(bx, by - 28, 11, 0, 7); c.fill();
    c.fillStyle = C.green; for (const y of [0, 14]) c.fillRect(bx - 14, by + y, 28, 5); });
  for (let s = 0; s < 4; s++) { const sk = new THREE.Mesh(new THREE.BoxGeometry(u(2 * E), u(DECK - 8), u(4)), [flat(C.greenD), flat(C.greenD), flat(C.greenD), flat(C.greenD), toonTex(saia), flat(C.greenD)]);
    const a = s * Math.PI / 2; sk.position.set(u(Math.sin(a) * (E - 2)), u(-8 - (DECK - 8) / 2), u(Math.cos(a) * (E - 2))); sk.rotation.y = a; g.add(ol(sk, 0.004)); }
  g.add(box(2 * E + 6, 5, 2 * E + 6, 0, -9, 0, C.gold, 1.5));   // filete dourado na borda da lona
  const corners = [[-HALF, HALF, C.red], [HALF, HALF, C.blue], [HALF, -HALF, C.cream], [-HALF, -HALF, C.cream]];
  for (const [x, z, pad] of corners) {
    g.add(ol(box(28, POST_TOP + 12, 28, x, (POST_TOP - 12) / 2, z, C.green, 3), 0.006)); g.add(ol(box(34, 10, 34, x, POST_TOP + 2, z, C.gold, 2), 0.004));
    g.add(ol(ball(12, x, POST_TOP + 16, z, C.gold), 0.004));
    const sx = -Math.sign(x), sz = -Math.sign(z); g.add(ol(box(36, ROPES[2] - ROPES[0] + 34, 36, x + sx * 18, (ROPES[0] + ROPES[2]) / 2, z + sz * 18, pad, 6), 0.005));
    for (const h of ROPES) g.add(ol(box(40, 6, 40, x + sx * 18, h, z + sz * 18, C.gold, 1.5), 0.003)); }
  for (let i = 0; i < 4; i++) { const [ax, az] = corners[i], [bx, bz] = corners[(i + 1) % 4];
    ROPES.forEach((h, k) => { const a = new THREE.Vector3(u(ax), u(h), u(az)), b = new THREE.Vector3(u(bx), u(h), u(bz)), len = a.distanceTo(b);
      if (window.__ropeInfo) { window.__ropeInfo.push({ y: u(h), color: C.cream }); return; }   // o jogo 3D desenha as cordas (tools3d/export)
      const cg = new THREE.CylinderGeometry(u(3.8), u(3.8), len, 10); cg.rotateZ(Math.PI / 2);
      const col = i === 0 && k === 1 ? C.red : i === 1 && k === 2 ? C.blue : C.cream;   // frente: corda do meio vermelha; direita: a de cima azul
      const m = new THREE.Mesh(cg, flat(col)); m.position.copy(a).add(b).multiplyScalar(0.5); m.rotation.y = -Math.atan2(b.z - a.z, b.x - a.x); g.add(ol(m, 0.004)); }); }
  return g;
}

// ================= pedra, madeira, colmeia, pérgola, arco, chafariz
function pilarVaso(x, z, h, tipo, cols, seed) {   // pilar quadrado de pedra clara com o vaso-taça de pedra e flores
  const g = new THREE.Group(); g.add(ol(box(44, h, 44, x, Y0 + h / 2, z, C.stone, 3), 0.005)); g.add(ol(box(52, 8, 52, x, Y0 + h + 4, z, C.stoneD, 2), 0.004));
  const pts = [[14, 0], [16, 4], [9, 9], [8, 18], [18, 24], [26, 34], [28, 40], [25, 42]].map(([r, y]) => new THREE.Vector2(u(r), u(y)));
  const v = new THREE.Mesh(new THREE.LatheGeometry(pts, 20), flat(C.stone)); v.position.set(u(x), u(Y0 + h + 8), u(z)); g.add(ol(v, 0.005));
  moita(x, z, 26, tipo, cols, seed, Y0 + h + 30); return g;
}
function cerca(x0, z0, x1, z1) {
  const g = new THREE.Group(), L = Math.hypot(x1 - x0, z1 - z0), n = Math.max(2, Math.round(L / 36)), a = Math.atan2(z1 - z0, x1 - x0);
  for (let i = 0; i <= n; i++) { const t = i / n, p = box(9, 56, 9, x0 + (x1 - x0) * t, Y0 + 28, z0 + (z1 - z0) * t, i % 2 ? C.wood : C.woodD, 1.5); g.add(ol(p, 0.003)); }
  for (const h of [20, 46]) { const r = box(L, 7, 5, (x0 + x1) / 2, Y0 + h, (z0 + z1) / 2, C.wood, 1.5); r.rotation.y = -a; g.add(ol(r, 0.003)); }
  return g;
}
function colmeia(x, z, h, s = 1) {   // colmeia aérea do kit
  const g = new THREE.Group(), q = v => v * s;
  g.add(ol(box(q(16), h, q(16), 0, h / 2, 0, C.wood, 2), 0.004));
  for (const d of [-1, 1]) { const b = box(q(7), q(60), q(7), d * q(20), h - q(32), 0, C.woodD, 1); b.rotation.z = d * 0.75; g.add(ol(b, 0.003)); }
  g.add(ol(box(q(84), q(7), q(70), 0, h + q(3), 0, C.woodD, 1.5), 0.004));
  ['#f4e9cc', '#f2c43a', '#f4e9cc', '#f4e9cc', '#f2c43a', '#f4e9cc'].forEach((c, i) => g.add(ol(box(q(70), q(16), q(58), 0, h + q(15) + i * q(16), 0, c, 2), 0.004)));
  g.add(box(q(26), q(4), q(8), 0, h + q(10), q(32), C.woodD, 1)); g.add(box(q(18), q(5), q(2), 0, h + q(15), q(29.5), '#2a1e12', 1));
  for (const d of [-1, 1]) { const r = box(q(52), q(6), q(78), d * q(21), h + q(122), 0, '#8a7462', 1.5); r.rotation.z = d * -0.55; g.add(ol(r, 0.004)); }
  g.add(ol(box(q(10), q(10), q(80), 0, h + q(134), 0, '#6e5a48', 2), 0.003));
  g.position.set(u(x), u(Y0), u(z)); return g;
}
function pergola(x, z, W, D, H) {
  const g = new THREE.Group();
  for (const sx of [-1, 1]) for (const sz of [-1, 1]) g.add(ol(box(18, H, 18, sx * W / 2, H / 2, sz * D / 2, C.wood, 2), 0.005));
  for (const sz of [-1, 1]) g.add(ol(box(W + 60, 16, 12, 0, H + 8, sz * D / 2, C.woodD, 2), 0.004));
  for (let i = 0; i < 10; i++) g.add(ol(box(10, 10, D + 50, -W / 2 - 20 + i * (W + 40) / 9, H + 21, 0, C.wood, 1.5), 0.003));
  g.position.set(u(x), u(Y0), u(z));
  const cols = [['#d8283a', '#b81e30'], ['#f07a9a', '#f6a8c0'], ['#ffffff', '#f6e4ec']];
  for (const sx of [-1, 1]) for (const sz of [-1, 1]) for (let k = 0; k < 5; k++) moita(x + sx * W / 2, z + sz * D / 2, 20, 'rosa', cols[(k + (sx > 0)) % 3], 300 + sx * 30 + sz * 7 + k, Y0 + k * H / 5.5);
  for (let i = 0; i < 14; i++) moita(x - W / 2 + hash(i + 5) * W, z - D / 2 + hash(i * 3) * D, 22, 'rosa', cols[i % 3], 340 + i, Y0 + H + 6, 1.2);
  for (const sz of [-1, 1]) for (let i = 0; i < 6; i++) moita(x - W / 2 + i * W / 5, z + sz * D / 2, 16, 'rosa', cols[(i + 1) % 3], 380 + i + sz * 9, Y0 + H - 20);   // cachos pendendo das vigas
  return g;
}
function arcoRosas(x, z, R) {
  const g = new THREE.Group(); g.add(ol(new THREE.Mesh(new THREE.TorusGeometry(u(R), u(5), 8, 28, Math.PI), flat(C.woodD)), 0.003));
  for (const s of [-1, 1]) g.add(ol(box(10, 110, 10, s * R, -55, 0, C.woodD, 1), 0.003));
  g.position.set(u(x), u(Y0 + 110), u(z));
  for (let i = 0; i <= 14; i++) { const a = i / 14 * Math.PI; moita(x + Math.cos(a) * R, z, 18, 'rosa', i % 3 ? ['#f07a9a', '#f6a8c0'] : ['#d8283a'], 400 + i, Y0 + 92 + Math.sin(a) * R); }
  for (const s of [-1, 1]) for (let k = 0; k < 3; k++) moita(x + s * R, z, 18, 'rosa', ['#f07a9a', '#ffffff'], 420 + k + s * 5, Y0 + k * 36);
  return g;
}
function chafariz(x, z) {
  const g = new THREE.Group(), agua = flat('#8fd4e8');
  g.add(ol(cyl(80, 86, 26, 0, 13, 0, C.stone, 32), 0.005)); g.add(cyl(72, 72, 2, 0, 24, 0, agua, 32));
  g.add(ol(cyl(12, 18, 70, 0, 56, 0, C.stone, 16), 0.004)); g.add(ol(cyl(40, 24, 12, 0, 94, 0, C.stone, 24), 0.004)); g.add(cyl(36, 36, 2, 0, 99, 0, agua, 24));
  g.add(ol(cyl(6, 9, 30, 0, 114, 0, C.stone, 12), 0.003)); g.add(ol(cyl(18, 12, 8, 0, 132, 0, C.stone, 16), 0.003));
  for (let k = 0; k < 8; k++) { const a = k / 8 * Math.PI * 2, j = new THREE.Mesh(new THREE.TorusGeometry(u(22), u(1.6), 5, 12, Math.PI * 0.6), new THREE.MeshToonMaterial({ color: '#cfeefa', transparent: true, opacity: 0.8, gradientMap: flat('#fff').gradientMap }));
    j.rotation.set(0, -a, Math.PI * 0.2); j.position.set(u(Math.cos(a) * 20), u(118), u(Math.sin(a) * 20)); g.add(j); }
  const queda = (tex, op) => new THREE.MeshToonMaterial({ map: tex, color: '#e6f8ff', transparent: true, opacity: op, depthWrite: false, side: THREE.DoubleSide, gradientMap: flat('#fff').gradientMap });
  const cortina = new THREE.Mesh(new THREE.CylinderGeometry(u(37), u(46), u(74), 32, 1, true), queda(AGUA.cair, 0.7)); cortina.userData.anim = { k: 'scroll', a: 1.4 }; cortina.position.y = u(62); g.add(cortina);
  const jato = new THREE.Mesh(new THREE.CylinderGeometry(u(2.5), u(4), u(36), 10, 1, true), queda(AGUA.subir, 0.85)); jato.userData.anim = { k: 'scroll', a: -2.2 }; jato.position.y = u(152); g.add(jato);
  const topo = new THREE.Mesh(new THREE.CylinderGeometry(u(18), u(24), u(26), 20, 1, true), queda(AGUA.cair, 0.6)); topo.userData.anim = { k: 'scroll', a: 1.4 }; topo.position.y = u(123); g.add(topo);   // borda do pratinho de cima
  for (let k = 0; k < 3; k++) { const o = new THREE.Mesh(new THREE.RingGeometry(u(0.8), u(1), 32), new THREE.MeshBasicMaterial({ color: '#ffffff', transparent: true, opacity: 0.6, depthWrite: false }));
    o.rotation.x = -Math.PI / 2; o.position.y = u(25.5); o.userData.ph = k / 3; AGUA.ondas.push(o); g.add(o); }
  g.position.set(u(x), u(Y0), u(z)); return g;
}
// texturas da água correndo (listras claras com transparência) e as ondinhas da bacia: animadas no relógio da cena
const AGUA = (() => { const t = () => canvasTex(64, 128, (c, w, h) => { c.clearRect(0, 0, w, h); for (let i = 0; i < 14; i++) { const x = (i * 37) % w, y = (i * 53) % h; c.fillStyle = i % 3 ? 'rgba(255,255,255,0.55)' : 'rgba(160,220,240,0.8)'; c.fillRect(x, y, 3 + (i % 2) * 2, 30 + (i % 4) * 14); }
    c.fillStyle = 'rgba(190,235,250,0.45)'; c.fillRect(0, 0, w, h); }, [6, 1]);
  return { cair: t(), subir: t(), ondas: [] }; })();
function cipreste(x, z, h, y0 = Y0) { const m = new THREE.Mesh(new THREE.CapsuleGeometry(u(h * 0.13), u(h * 0.74), 8, 14), flat('#2e6236'));
  const p = m.geometry.attributes.position; for (let i = 0; i < p.count; i++) { const y = p.getY(i) / u(h * 0.5), k = (1 - Math.max(0, y) * 0.6) * (1 + 0.06 * Math.sin(p.getY(i) / u(h) * 40)); p.setX(i, p.getX(i) * k); p.setZ(i, p.getZ(i) * k); }
  m.geometry.computeVertexNormals(); m.position.set(u(x), u(y0 + h * 0.5), u(z)); return ol(m, 0.005); }
function arvore(x, z, s, seed, y0 = Y0) { const g = new THREE.Group(); g.add(ol(cyl(9 * s, 14 * s, 110 * s, x, y0 + 55 * s, z, C.woodD, 8), 0.004));
  for (let k = 0; k < 7; k++) { const a = k / 7 * Math.PI * 2, r = (36 + hash(seed + k) * 18) * s; put('blob', x + Math.cos(a) * 40 * s, y0 + (130 + hash(seed * 3 + k) * 40) * s, z + Math.sin(a) * 30 * s, [r * 1.15, r * 0.9, r * 1.15], verde(seed + k)); }
  put('blob', x, y0 + 175 * s, z, [55 * s, 45 * s, 55 * s], verde(seed + 50)); return g; }

export async function buildJackArena() {
  INST.clear();
  const all = new THREE.Group();
  all.add(ringue());
  all.userData.look = { tint: [1.03, 1.0, 0.93], horizon: '#d8ecf6', zenith: '#4a98e8', fog: [80, 210],
    sun: { color: '#fff2d6', int: 2.1, pos: [1.6, 3.4, 2.0] }, hemi: { sky: '#eaf4ff', ground: '#6a8a44', int: 1.45 } };
  // ---- chão: gramado, lajeado em volta do ringue e o caminho até o chafariz
  const grama = new THREE.Mesh(new THREE.PlaneGeometry(u(9000), u(9000)), flat(C.grass)); grama.rotation.x = -Math.PI / 2; grama.position.y = u(Y0 - 1); grama.userData.keep = true; all.add(grama);   // fica solta: leva o relógio das abelhas
  const lajes = canvasTex(256, 256, (c, w) => { c.fillStyle = '#b4a27e'; c.fillRect(0, 0, w, w); let sd = 5; const rnd = () => (sd = (sd * 9301 + 49297) % 233280) / 233280;
    for (let y = 0; y < w; y += 42) for (let x = -((y / 42) % 2) * 30; x < w; x += 60) { c.fillStyle = ['#e4d6b6', '#dacaa6', '#ecdcc0'][Math.floor(rnd() * 3)]; c.fillRect(x + 3, y + 3, 54, 36); } }, [6, 6]);
  const piso = new THREE.Mesh(new THREE.PlaneGeometry(u(760), u(900)), toonTex(lajes)); piso.rotation.x = -Math.PI / 2; piso.position.set(0, u(Y0), u(40)); all.add(piso);
  const caminho = new THREE.Mesh(new THREE.PlaneGeometry(u(150), u(700)), toonTex(canvasTex(256, 256, (c, w) => { c.fillStyle = '#b4a27e'; c.fillRect(0, 0, w, w); for (let y = 0; y < w; y += 32) for (let x = -((y / 32) % 2) * 20; x < w; x += 44) { c.fillStyle = y % 64 ? '#e4d6b6' : '#dccca8'; c.fillRect(x + 2, y + 2, 40, 28); } }, [1, 5])));
  caminho.rotation.x = -Math.PI / 2; caminho.position.set(u(-20), u(Y0 + 0.5), u(-860)); all.add(caminho);
  // ---- fundo do ringue: terraço de pedra (muro baixo com capa, escada no meio), pilares com vasos e a balaustrada
  for (const s of [-1, 1]) { all.add(ol(box(330, 30, 110, s * 255, Y0 + 15, -480, C.stone, 3), 0.005)); all.add(ol(box(340, 6, 118, s * 255, Y0 + 32, -480, C.stoneD, 2), 0.004));
    for (let k = 0; k < 5; k++) moita(s * (130 + k * 60), -490, 30 + hash(k + s) * 8, ['rosa', 'marg', 'rosa', 'peq', 'rosa'][k], [['#d8283a'], ['#ffffff'], ['#f07a9a'], ['#f6d23a'], ['#f6f0f0']][k], 1100 + k + s * 9, Y0 + 34);
    for (let k = 0; k < 3; k++) espigas(s * (160 + k * 110), -455, 5, 80, k % 2 ? '#f6f0ff' : '#8a6ad8', 1150 + k + s * 9, Y0 + 34); }
  for (let k = 0; k < 4; k++) all.add(ol(box(160, 10 * (k + 1), 30, -20, Y0 + 5 * (k + 1), -400 - k * 30, k % 2 ? C.stoneD : C.stone, 2), 0.003));
  for (const s of [-1, 1]) { all.add(pilarVaso(s * 90, -440, 70, s < 0 ? 'rosa' : 'marg', s < 0 ? ['#d8283a'] : ['#ffffff'], 500 + s));
    for (let k = 0; k < 6; k++) all.add(ol(cyl(6, 8, 30, s * (140 + k * 28), Y0 + 63, -440, C.stone, 10), 0.003));   // balaústres
    all.add(ol(box(170, 8, 16, s * 210, Y0 + 82, -440, C.stone, 2), 0.003)); }
  // ---- lado esquerdo (como no concept): cerca, pilares com vasos, colmeia grande, lavandas e dedaleiras, canteiro de rosas e margaridas
  all.add(cerca(-410, 330, -410, -380)); all.add(cerca(-410, -380, -180, -380));
  all.add(pilarVaso(-460, -150, 90, 'rosa', ['#f6f0f0', '#f6c8e0'], 520)); all.add(pilarVaso(-440, 340, 60, 'marg', ['#ffffff'], 521));
  all.add(colmeia(-720, -260, 330, 1.25)); all.add(colmeia(-330, -720, 190, 0.9));
  [[-520, 150, '#8a6ad8'], [-560, 40, '#7a5ac8'], [-470, 260, '#f6f0ff'], [-520, -40, '#f0a8d0'], [-620, 220, '#8a6ad8']].forEach(([x, z, c], i) => espigas(x, z, 7, 100 + hash(i) * 40, c, 600 + i * 13));
  [[-640, -60, 44, 'rosa', ['#d8283a', '#c41e2e']], [-760, 120, 48, 'rosa', ['#d8283a']], [-640, 320, 40, 'marg', ['#ffffff']], [-820, -120, 46, 'peq', ['#f6d23a', '#f2a02a']],
   [-560, -300, 38, 'rosa', ['#f6f0f0']], [-880, 260, 50, 'marg', ['#f6d23a']], [-700, -420, 44, 'rosa', ['#d8283a']], [-920, -380, 52, 'rosa', ['#f07a9a']]]
    .forEach(([x, z, r, t, c], i) => moita(x, z, r, t, c, 640 + i * 17));
  for (const s of [-1, 1]) for (let i = 0; i < 7; i++) { const z = -330 + i * 100;
    if (i % 2) espigas(s * 365, z, 6, 90 + hash(i + s) * 50, i % 4 === 1 ? '#8a6ad8' : '#f0a8d0', 1200 + i + s * 20);
    else moita(s * 370, z, 30 + hash(i * 3 + s) * 10, i % 4 ? 'rosa' : 'marg', i % 4 ? (s < 0 ? ['#d8283a'] : ['#f07a9a']) : ['#ffffff', '#f6d23a'], 1230 + i + s * 20); }
  // ---- lado direito: cerca, pérgola de roseiras, girassóis, vasos de pedra com flores, lavandas
  all.add(cerca(410, 330, 410, -380)); all.add(cerca(410, -380, 180, -380));
  all.add(pergola(600, -300, 230, 220, 260));
  for (let i = 0; i < 9; i++) girassol(780 + (i % 3) * 40 + hash(i) * 20, -60 + Math.floor(i / 3) * 50, 150 + hash(i * 3) * 50, 700 + i);
  all.add(pilarVaso(460, 120, 70, 'rosa', ['#f07a9a', '#f6a8c0'], 720)); all.add(pilarVaso(470, 360, 40, 'marg', ['#ffffff', '#f6d23a'], 721));
  [[540, 260, '#8a6ad8'], [580, 380, '#7a5ac8'], [640, 160, '#f6f0ff']].forEach(([x, z, c], i) => espigas(x, z, 7, 100 + hash(i * 5) * 40, c, 740 + i * 13));
  [[620, 60, 40, 'rosa', ['#d8283a']], [760, 260, 46, 'peq', ['#f6d23a']], [880, -260, 50, 'rosa', ['#f07a9a']], [700, 420, 42, 'marg', ['#ffffff']]].forEach(([x, z, r, t, c], i) => moita(x, z, r, t, c, 780 + i * 17));
  all.add(colmeia(820, -560, 300, 1.1));
  // ---- frente: moitas enormes de rosas nos cantos (vermelhas à esquerda, rosa à direita) com margaridas e vasos
  moita(-470, 520, 70, 'rosa', ['#d8283a', '#c41e2e', '#e0303e'], 900, Y0, 0.9); moita(-330, 600, 46, 'marg', ['#ffffff'], 901); moita(-620, 420, 50, 'peq', ['#f6d23a'], 902);
  moita(470, 520, 70, 'rosa', ['#f07a9a', '#f6a8c0', '#e85a8a'], 910, Y0, 0.9); moita(330, 600, 44, 'peq', ['#f6d23a', '#ffffff'], 911); moita(620, 420, 48, 'marg', ['#ffffff'], 912);
  espigas(600, 560, 8, 120, '#8a6ad8', 913); espigas(-600, 600, 6, 110, '#f6f0ff', 914);
  // ---- fundo: chafariz sob o arco de rosas, moitas e sebes dos dois lados do caminho, colmeias, árvores
  all.add(chafariz(-20, -1080)); all.add(arcoRosas(-20, -960, 120));
  for (const s of [-1, 1]) for (let i = 0; i < 6; i++) moita(-20 + s * (130 + hash(i) * 70), -600 - i * 80, 38 + hash(i * 3) * 12, i % 2 ? 'rosa' : 'marg', i % 2 ? ['#d8283a', '#f07a9a'] : ['#ffffff', '#f6d23a'], 960 + i + s * 20);
  for (const s of [-1, 1]) for (let i = 0; i < 5; i++) espigas(-20 + s * (300 + hash(i * 7) * 200), -620 - i * 90, 6, 90 + hash(i) * 40, i % 2 ? '#8a6ad8' : '#f6f0ff', 1000 + i + s * 40);
  for (const s of [-1, 1]) for (let i = 0; i < 6; i++) moita(-20 + s * (380 + hash(i * 3) * 300), -560 - i * 100, 44, ['rosa', 'peq', 'marg'][i % 3], [['#f07a9a'], ['#f6d23a', '#f2a02a'], ['#ffffff']][i % 3], 1040 + i + s * 30);
  for (const s of [-1, 1]) for (let i = 0; i < 8; i++) put('blob', s * (330 + i * 70), Y0 + 80, -1050 - hash(i) * 60, [60, 50, 50], verde(990 + i + s));   // sebe alta do fundo
  all.add(colmeia(-250, -1000, 230, 0.9)); all.add(colmeia(260, -980, 200, 0.85));
  // ---- ciprestes e árvores redondas
  for (const [x, z, h] of [[-980, -380, 520], [-860, -760, 460], [-560, -1250, 420], [-380, -1450, 380], [420, -1350, 440], [980, -900, 500], [1100, -300, 480], [1200, 200, 420], [-1150, 200, 440], [220, -1600, 360], [-140, -1650, 340]]) all.add(cipreste(x, z, h));
  [[-700, -1150, 1.4, 1], [650, -1150, 1.5, 2], [-1150, -950, 1.6, 3], [1150, -1300, 1.5, 4], [60, -1450, 1.3, 5]].forEach(([x, z, s, sd]) => all.add(arvore(x, z, s, sd * 31)));
  // ---- atrás da câmera: canteiros, cerca e árvores fechando o jardim
  all.add(cerca(-760, 760, 760, 760));
  for (let i = 0; i < 9; i++) moita(-700 + i * 175, 860, 50, i % 2 ? 'rosa' : 'marg', i % 2 ? ['#d8283a', '#f07a9a'] : ['#ffffff'], 1100 + i);
  for (const x of [-900, -400, 300, 850]) all.add(cipreste(x, 1050, 420));
  // ---- longe: colinas verdes em camadas, o morro de pedra com a vila e a torre, a serra azulada
  const colina = (R, base, amp, col, seed, arc = [-1.1, 1.1]) => { const pos = [], idx = [], N = 120;
    for (let i = 0; i <= N; i++) { const a = arc[0] + (arc[1] - arc[0]) * i / N, hh = base + amp * (0.5 + 0.3 * Math.sin(a * 4 + seed) + 0.2 * Math.sin(a * 9 + seed * 2)), x = Math.sin(a) * R, z = -Math.cos(a) * R;
      pos.push(u(x), u(Y0 - 4), u(z), u(x), u(Y0 + hh), u(z)); if (i < N) { const b = i * 2; idx.push(b, b + 2, b + 1, b + 1, b + 2, b + 3); } }
    const g = new THREE.BufferGeometry(); g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3)); g.setIndex(idx); g.computeVertexNormals(); const m = flat(col); m.side = THREE.DoubleSide; return new THREE.Mesh(g, m); };
  all.add(colina(2200, 90, 200, '#7fae4e', 1, [-Math.PI, Math.PI]), colina(2700, 160, 300, '#6e9e4e', 3, [-Math.PI, Math.PI]), colina(3600, 150, 260, '#a6c4d8', 6, [-Math.PI, Math.PI]));
  for (let i = 0; i < 30; i++) { const a = (hash(i) - 0.5) * 2.4, r = 2350 + hash(i * 3) * 300; all.add(cipreste(Math.sin(a) * r, -Math.cos(a) * r, 160 + hash(i * 7) * 120, Y0 + 120)); }
  // ---- o morro de pedra com a vila (à esquerda, como no concept): penhasco em três camadas facetadas com faixas de
  // estrato e pedras soltas no pé, capim na borda; em cima a muralha com ameias, as casinhas de telhado de duas águas
  // (janelas, portas, chaminés) em dois patamares, a igreja com o campanário e a flecha, a torre da cúpula e ciprestes
  const hill1 = a => 90 + 200 * (0.5 + 0.3 * Math.sin(a * 4 + 1) + 0.2 * Math.sin(a * 9 + 2));   // altura da 1ª colina no ângulo a
  const telhado = (w, d, h, x, y, z, ry, col) => { const sh = new THREE.Shape(); sh.moveTo(u(-w / 2 - 5), 0); sh.lineTo(0, u(h)); sh.lineTo(u(w / 2 + 5), 0); sh.lineTo(u(-w / 2 - 5), 0);
    const g = new THREE.ExtrudeGeometry(sh, { depth: u(d + 8), bevelEnabled: false }); g.translate(0, 0, -u(d + 8) / 2); const m = new THREE.Mesh(g, flat(col)); m.position.set(u(x), u(y), u(z)); m.rotation.y = ry; return ol(m, 0.003); };
  const casinha = (x, y, z, w, d, h, ry, seed) => { const g = new THREE.Group(), parede = ['#f3e6c8', '#efd9b0', '#f6ead4', '#e8cfa4'][seed % 4];
    g.add(ol(box(w, h, d, 0, h / 2, 0, parede, 2), 0.004)); g.add(telhado(w, d, h * 0.45, 0, h, 0, 0, ['#c8644a', '#b85a3e', '#d0704e'][seed % 3]));
    for (const fx of w > 50 ? [-w / 4, w / 4] : [0]) { g.add(box(9, 12, 2, fx, h * 0.62, d / 2 + 0.5, '#4a3a3a', 1)); g.add(box(13, 3, 4, fx, h * 0.62 - 7, d / 2 + 1, parede, 1)); }   // janelas com peitoril
    if (hash(seed) < 0.6) g.add(box(10, 18, 2, (hash(seed * 3) - 0.5) * w * 0.4, 9, d / 2 + 0.5, '#6a4026', 1));   // porta
    if (hash(seed * 5) < 0.5) g.add(ol(box(9, 22, 9, w * 0.25, h + h * 0.3, -d * 0.15, parede, 1.5), 0.003));   // chaminé
    g.position.set(u(x), u(y), u(z)); g.rotation.y = ry; return g; };
  { const vx = -1000, vz = -2380, pedra = ['#c9b998', '#bcab88', '#d4c4a4'];
    [[430, 330, 150, 0], [370, 300, 140, 150], [320, 270, 110, 290]].forEach(([r0, r1, h, y], k) => {   // camadas do penhasco
      const g = new THREE.CylinderGeometry(u(r1), u(r0), u(h), 11, 2), p = g.attributes.position;
      for (let i = 0; i < p.count; i++) { const a = Math.atan2(p.getZ(i), p.getX(i)), f = 1 + 0.08 * Math.sin(a * 5 + k) + 0.05 * Math.sin(a * 13 + k * 2); p.setX(i, p.getX(i) * f); p.setZ(i, p.getZ(i) * f * 0.75); }
      g.computeVertexNormals(); const m = new THREE.Mesh(g, flat(pedra[k])); m.material.flatShading = true; m.position.set(u(vx + k * 12), u(Y0 + y + h / 2), u(vz)); all.add(ol(m, 0.005));
      const faixa = new THREE.Mesh(new THREE.CylinderGeometry(u(r1 * 1.01), u(r1 * 1.01), u(8), 11), flat('#a8957a')); faixa.scale.z = 0.75; faixa.position.set(u(vx + k * 12), u(Y0 + y + h - 6), u(vz)); all.add(faixa); });
    for (let i = 0; i < 9; i++) { const a = -0.3 + i / 8 * 3.4, b = new THREE.Mesh(new THREE.IcosahedronGeometry(u(40 + hash(i) * 40), 0), flat(pedra[i % 3])); b.material.flatShading = true;
      b.scale.y = 0.7; b.position.set(u(vx + Math.cos(a) * 440), u(Y0 + 25), u(vz + Math.sin(a) * 330)); all.add(ol(b, 0.004)); }   // pedras soltas no pé
    const top = Y0 + 400;
    for (let i = 0; i < 26; i++) { const a = i / 26 * Math.PI * 2; put('blob', vx + 24 + Math.cos(a) * 250, top - 2, vz + Math.sin(a) * 185, [30, 12, 30], verde(1500 + i)); }   // capim na borda, dentro do topo da pedra
    for (let i = 0; i < 22; i++) { const a = i / 22 * Math.PI * 2, x = vx + 24 + Math.cos(a) * 300, z = vz + Math.sin(a) * 222;   // muralha com ameias
      const seg = box(92, 34, 12, x, top + 17, z, '#e2d2ae', 1.5); seg.rotation.y = -a + Math.PI / 2; all.add(ol(seg, 0.003));
      const am = box(14, 12, 14, x, top + 40, z, '#e2d2ae', 1); am.rotation.y = -a; all.add(ol(am, 0.003)); }
    // casinhas: patamar de baixo na frente, de cima atrás (mais alto), viradas para o lado do jardim
    for (let i = 0; i < 9; i++) { const x = vx - 190 + i * 46 + hash(i) * 8, z = vz + 120 - hash(i * 3) * 30; all.add(casinha(x, top, z, 40 + hash(i * 5) * 16, 36, 36 + hash(i * 7) * 22, (hash(i * 9) - 0.5) * 0.3, i)); }
    all.add(ol(box(500, 20, 220, vx + 24, top + 10, vz - 10, '#d8c8a2', 2), 0.003));   // patamar de cima
    for (let i = 0; i < 7; i++) { const x = vx - 160 + i * 56 + hash(i + 20) * 10, z = vz + 20 - hash(i * 4) * 50; all.add(casinha(x, top + 20, z, 48 + hash(i * 6) * 14, 42, 46 + hash(i * 8) * 30, (hash(i * 11) - 0.5) * 0.4, i + 10)); }
    // igreja: nave com telhado e rosácea, campanário com os vãos do sino e a flecha
    { const cx = vx + 70, cz = vz - 70; all.add(ol(box(120, 80, 70, cx, top + 60, cz, '#f2e4c4', 2), 0.004)); all.add(telhado(70, 120, 40, cx, top + 100, cz, 0, '#c8644a'));
      const ros = cyl(12, 12, 3, cx, top + 80, cz + 36, '#8a3a2a', 14); ros.rotation.x = Math.PI / 2; all.add(ol(ros, 0.003));
      all.add(ol(box(46, 200, 46, cx + 80, top + 120, cz - 10, '#eadcbc', 2), 0.005)); all.add(ol(box(54, 10, 54, cx + 80, top + 222, cz - 10, '#d8c8a2', 2), 0.003));
      for (const [dx, dz] of [[0, 23.5], [23.5, 0], [0, -23.5], [-23.5, 0]]) all.add(box(dz ? 16 : 2, 24, dz ? 2 : 16, cx + 80 + dx, top + 196, cz - 10 + dz, '#3a2a2a', 1));
      const fl = new THREE.Mesh(new THREE.ConeGeometry(u(36), u(80), 4), flat('#b85a3e')); fl.rotation.y = Math.PI / 4; fl.position.set(u(cx + 80), u(top + 267), u(cz - 10)); all.add(ol(fl, 0.004));
      all.add(ol(box(3, 24, 3, cx + 80, top + 318, cz - 10, '#3a3030', 0.5), 0.002)); all.add(ol(box(14, 3, 3, cx + 80, top + 322, cz - 10, '#3a3030', 0.5), 0.002)); }
    // torre da cúpula (a do concept) e ciprestes entre as casas
    { const tx = vx - 150, tz = vz - 60; all.add(ol(cyl(30, 34, 150, tx, top + 95, tz, '#eadcbc', 14), 0.005)); all.add(ol(cyl(36, 36, 10, tx, top + 172, tz, '#d8c8a2', 14), 0.003));
      for (let k = 0; k < 6; k++) { const a = k / 6 * Math.PI * 2, jn = box(8, 20, 3, tx + Math.cos(a) * 31, top + 140, tz + Math.sin(a) * 31, '#3a2a2a', 1); jn.rotation.y = -a + Math.PI / 2; all.add(jn); }
      const dm = new THREE.Mesh(new THREE.SphereGeometry(u(34), 16, 8, 0, Math.PI * 2, 0, Math.PI / 2), flat('#c8644a')); dm.position.set(u(tx), u(top + 177), u(tz)); all.add(ol(dm, 0.004)); all.add(ol(ball(6, tx, top + 216, tz, '#d9b44a'), 0.002)); }
    for (const [x, z, h] of [[vx - 220, vz + 60, 130], [vx + 200, vz + 80, 150], [vx - 60, vz + 150, 110], [vx + 170, vz - 120, 160]]) all.add(cipreste(x, z, h, top));
  }
  // ---- moinhos de vento no lado oposto (colinas da direita e atrás): torre branca afunilada, telhado cônico, porta e
  // janelinhas, as quatro pás de treliça girando com o vento, viradas para o jardim
  const pas = [];
  const moinho = (a, R, s, seed) => { const x = Math.sin(a) * R, z = -Math.cos(a) * R, y = Y0 + hill1(a) - 30, g = new THREE.Group(), q = v => v * s;
    g.add(ol(cyl(q(40), q(56), q(190), 0, q(95), 0, '#f4ecdc', 16), 0.005)); g.add(ol(cyl(q(44), q(44), q(10), 0, q(192), 0, '#8a5e36', 16), 0.004));
    const tel = new THREE.Mesh(new THREE.ConeGeometry(u(q(52)), u(q(70)), 16), flat('#b85a3e')); tel.position.y = u(q(232)); g.add(ol(tel, 0.004));
    g.add(box(q(26), q(40), q(4), 0, q(20), q(54), '#6a4026', 2)); for (const yy of [90, 140]) g.add(box(q(14), q(18), q(4), 0, q(yy), q(48 - yy * 0.06), '#4a3a3a', 1));
    const eixo = cyl(q(9), q(9), q(30), 0, q(200), q(52), '#6a4026', 10); eixo.rotation.x = Math.PI / 2; g.add(ol(eixo, 0.003));
    g.position.set(u(x), u(y), u(z)); g.lookAt(0, u(y), 0); all.add(g);
    const sail = new THREE.Group(); sail.position.set(u(x), u(y + q(200)), u(z)); sail.lookAt(0, u(y + q(200)), 0); sail.translateZ(u(q(70))); sail.add(ol(ball(q(12), 0, 0, 0, '#6a4026', 10), 0.003));
    const rot = new THREE.Group(); sail.add(rot);
    for (let k = 0; k < 4; k++) { const arm = new THREE.Group(); arm.rotation.z = k * Math.PI / 2; rot.add(arm);
      arm.add(ol(box(q(6), q(170), q(5), 0, q(90), 0, '#6a4026', 1), 0.003));
      arm.add(box(q(36), q(130), q(2), q(20), q(104), q(-3), '#efe6d0', 1));   // a lona da pá
      for (let j = 0; j < 6; j++) arm.add(box(q(40), q(3), q(4), q(18), q(44 + j * 24), 0, '#8a5e36', 0.8));   // ripas da treliça
      arm.add(box(q(3), q(130), q(4), q(38), q(104), 0, '#8a5e36', 0.8)); }
    rot.rotation.z = hash(seed) * 6.28; rot.userData.sp = 0.5 + hash(seed * 3) * 0.4; rot.userData.anim = { k: 'spin', axis: [0, 0, 1], sp: rot.userData.sp }; rot.traverse(o => { o.userData.keep = true; }); pas.push(rot); return sail; };
  const velas = [[0.62, 2330, 1.1, 1], [0.95, 2380, 1.25, 2], [1.3, 2340, 1.0, 3], [1.75, 2360, 1.15, 4], [2.6, 2350, 1.1, 5], [-2.7, 2350, 1.0, 6]].map(([a, R, s, sd]) => moinho(a, R, s, sd));
  // ---- nuvens fofas (creme, sem névoa)
  const nuvemM = new THREE.MeshBasicMaterial({ color: '#fff8ea', fog: false });
  for (let i = 0; i < 11; i++) { const a = (i / 11 - 0.5) * Math.PI * 1.8, r = 3300, cg = new THREE.Group();
    for (let k = 0; k < 6; k++) { const b = new THREE.Mesh(new THREE.IcosahedronGeometry(u(55 + hash(i * 5 + k) * 55), 1), nuvemM); b.position.set(u((k - 2.5) * 70), u(Math.sin(k * 1.3) * 25 + (k % 2) * 20), 0); b.scale.y = 0.75; cg.add(b); }
    cg.position.set(u(Math.sin(a) * r), u(700 + hash(i) * 500), u(-Math.cos(a) * r)); cg.lookAt(0, cg.position.y, 0); all.add(cg); }
  fundeEstaticos(all);
  for (const v of velas) all.add(v);   // pás giram: fora da fusão
  // ---- abelhas voando
  const abelhas = [];
  for (let i = 0; i < 16; i++) { const b = new THREE.Group(), body = new THREE.Mesh(new THREE.CapsuleGeometry(u(4.5), u(7), 4, 8), flat('#f2c43a')); body.rotation.z = Math.PI / 2; b.add(ol(body, 0.003));
    for (const x of [-2.5, 2.5]) { const st = cyl(4.8, 4.8, 2.2, x, 0, 0, '#2a2018', 10); st.rotation.z = Math.PI / 2; b.add(st); }
    const wm = new THREE.MeshToonMaterial({ color: '#ffffff', transparent: true, opacity: 0.75, gradientMap: flat('#fff').gradientMap }), w1 = new THREE.Mesh(new THREE.SphereGeometry(u(5), 8, 6), wm); w1.scale.set(0.6, 0.2, 1); w1.position.set(0, u(6), u(4)); b.add(w1); const w2 = w1.clone(); w2.position.z = u(-4); b.add(w2);
    const home = [[-720, -260], [-330, -720], [820, -560], [-250, -1000], [260, -980]][i % 5];
    b.userData = { cx: home[0], cz: home[1], cy: 200 + hash(i * 7) * 200, r: 60 + hash(i * 9) * 120, sp: 0.5 + hash(i * 2) * 0.8, ph: hash(i * 4) * 6.28, w: [w1, w2] }; b.position.set(u(b.userData.cx), u(Y0 + b.userData.cy), u(b.userData.cz)); b.userData.anim = { k: 'bee', r: u(b.userData.r), ph: b.userData.ph / 6.2832 }; b.traverse(o => { o.userData.keep = true; }); abelhas.push(b); all.add(b); }
  // ---- instâncias: uma malha por tipo, cor por instância
  const vento = { value: 0 };
  const balanca = (mat, k) => { mat.onBeforeCompile = sh => { sh.uniforms.uVento = vento;
      sh.vertexShader = 'uniform float uVento;\n' + sh.vertexShader.replace('#include <project_vertex>', `vec4 mvPosition = vec4( transformed, 1.0 );
        #ifdef USE_INSTANCING
          mvPosition = instanceMatrix * mvPosition;
        #endif
        { float hgt = max(mvPosition.y - (${u(Y0).toFixed(4)}), 0.0), ph = uVento * 1.6 + mvPosition.x * 1.3 + mvPosition.z * 0.9;
          mvPosition.x += sin(ph) * hgt * ${k.toFixed(3)}; mvPosition.z += sin(ph * 0.8 + 1.7) * hgt * ${(k * 0.6).toFixed(3)}; }
        mvPosition = modelViewMatrix * mvPosition;
        gl_Position = projectionMatrix * mvPosition;`); };
    mat.customProgramCacheKey = () => 'vento' + k; return mat; };
  for (const [kind, list] of INST) { const mat = balanca(flat('#ffffff'), kind === 'blob' ? 0.022 : 0.03), im = new THREE.InstancedMesh(P[kind], mat, list.length); im.userData.anim = { k: 'sway', a: kind === 'blob' ? 0.022 : 0.03, ground: u(Y0) };
    list.forEach((e, i) => { im.setMatrixAt(i, e.m); im.setColorAt(i, e.c); }); im.instanceMatrix.needsUpdate = true; im.instanceColor.needsUpdate = true; im.computeBoundingSphere(); im.frustumCulled = false; all.add(im); }
  grama.onBeforeRender = () => { const t = performance.now() / 1000; vento.value = t;
    AGUA.cair.offset.y = t * 1.4; AGUA.subir.offset.y = -t * 2.2;
    for (const o of AGUA.ondas) { const f = (t * 0.5 + o.userData.ph) % 1; o.scale.setScalar(u(8 + f * 60) / u(1)); o.material.opacity = 0.6 * (1 - f); }
    for (const p of pas) p.rotation.z = t * p.userData.sp;
    for (const b of abelhas) { const d = b.userData, a = t * d.sp + d.ph; b.position.set(u(d.cx + Math.sin(a) * d.r), u(Y0 + d.cy + Math.sin(a * 2.3) * 14), u(d.cz + Math.sin(a * 2) * d.r * 0.5));
      b.rotation.y = -Math.atan2(Math.cos(a * 2) * d.r, Math.cos(a) * d.r); for (const w of d.w) w.rotation.x = Math.sin(t * 60) * 0.6; } };
  return all;
}
