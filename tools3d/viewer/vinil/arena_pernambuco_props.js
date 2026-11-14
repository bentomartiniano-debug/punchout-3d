// Peças de Pernambuco em volta do ringue (arena da Valéria), em código e no acabamento do vinil, seguindo o kit de
// arena-concepts/pernambuco-kit: sombrinha de frevo, tambor, parapeito de balaústres, arcada colonial de três arcos e
// casa colonial de dois andares com faixa de azulejo. Cada peça é um módulo (frente para +z, base em y = 0, medidas em
// unidades do jogo) repetido com outra cor, tamanho e giro — nenhuma gerada fora. buildPernambucoProps() monta o
// entorno: parapeito atrás e dos lados, arcadas e casario ao fundo e nas laterais, tambores e sombrinhas nas quinas.
import { THREE, flat, withOutline } from './core.js';
import { RoundedBoxGeometry } from 'three/addons/geometries/RoundedBoxGeometry.js';
import { UNIT } from './arena_pernambuco.js';

const u = v => v * UNIT;
const APRON = 35;   // o chão fica 35 abaixo do piso do ringue
const C = { cyan: '#1fb0c8', red: '#e2342a', yellow: '#f7c22c', ivory: '#f7ecd2', handle: '#4a2a16', wood: '#8a5530', woodDark: '#5a3418', teal: '#2aa8a0',
            coral: '#ea7a3e', trim: '#f5d9a8', door: '#1f6fa8', roof: '#c95b2e', roofLine: '#9c3f1c', shutter: '#7a4320', tile: '#f4f1ea', tileBlue: '#2b5fae',
            drumRim: '#6b3b1c', skin: '#f2e3c4', rope: '#efe2c0' };
const HOUSE = ['#f2b632', '#3bb0a6', '#e88aa0', '#5aa0d8', '#f08a3c', '#9fd06a'];

const ol = (m, w = 0.007) => { const g = withOutline(m, w); g.position.copy(m.position); g.quaternion.copy(m.quaternion); g.scale.copy(m.scale); m.position.set(0, 0, 0); m.quaternion.identity(); m.scale.set(1, 1, 1); return g; };
const box = (w, h, d, x, y, z, col, r = 1.5) => { const m = new THREE.Mesh(new RoundedBoxGeometry(u(w), u(h), u(d), 2, u(Math.min(r, w / 2 - 0.01, h / 2 - 0.01, d / 2 - 0.01))), flat(col)); m.position.set(u(x), u(y), u(z)); return m; };
const cyl = (r, h, x, y, z, col, seg = 16) => { const m = new THREE.Mesh(new THREE.CylinderGeometry(u(r), u(r), u(h), seg), flat(col)); m.position.set(u(x), u(y), u(z)); return m; };
// padrão no shader, em unidades do jogo no espaço da peça (vP)
function pattern(mat, glsl, key, uniforms = {}) {
  mat.onBeforeCompile = sh => {
    Object.assign(sh.uniforms, uniforms);
    sh.vertexShader = 'varying vec3 vP;\n' + sh.vertexShader.replace('#include <begin_vertex>', `#include <begin_vertex>\nvP = position / ${UNIT.toFixed(6)};`);
    sh.fragmentShader = 'varying vec3 vP;\n' + Object.keys(uniforms).map(k => `uniform vec3 ${k};\n`).join('') + sh.fragmentShader.replace('#include <map_fragment>', '#include <map_fragment>\n' + glsl);
  };
  mat.customProgramCacheKey = () => key;
  return mat;
}
const col = h => ({ value: new THREE.Color(h) });

// --- sombrinha de frevo: copa de oito gomos (ciano, vermelho, amarelo, marfim), borda recortada entre as varetas,
// haste escura com ponteira. pole = altura da haste até a copa
export function sombrinha(pole = 0, phase = 0) {
  const g = new THREE.Group(), R = 72, RISE = 24, N = 8, cols = [C.cyan, C.red, C.yellow, C.ivory].map(h => new THREE.Color(h));
  const cone = new THREE.ConeGeometry(u(R), u(RISE), 96, 1, false).toNonIndexed(), p = cone.attributes.position, ca = [];
  for (let i = 0; i < p.count; i++) {   // borda: entre as varetas o pano sobe um pouco (recorte em meia-lua)
    const x = p.getX(i), z = p.getZ(i), y = p.getY(i);
    if (y < 0 && Math.hypot(x, z) > u(R) * 0.98) { const a = Math.atan2(z, x), f = (a / (Math.PI * 2) * N + 64) % 1; p.setY(i, y + u(7) * Math.sin(Math.PI * f)); }
  }
  for (let t = 0; t < p.count; t += 3) {   // cada triângulo pega a cor do gomo do seu centro
    const cx = (p.getX(t) + p.getX(t + 1) + p.getX(t + 2)) / 3, cz = (p.getZ(t) + p.getZ(t + 1) + p.getZ(t + 2)) / 3;
    const k = Math.floor(((Math.atan2(cz, cx) + phase) / (Math.PI * 2) * N + 64)) % N, c = cols[k % 4];
    for (let j = 0; j < 3; j++) ca.push(c.r, c.g, c.b);
  }
  cone.setAttribute('color', new THREE.Float32BufferAttribute(ca, 3)); cone.computeVertexNormals();
  const canopy = new THREE.Mesh(cone, new THREE.MeshToonMaterial({ vertexColors: true, gradientMap: flat('#fff').gradientMap })); canopy.position.y = u(pole + RISE / 2);
  g.add(ol(canopy, 0.007));
  g.add(ol(cyl(2.4, pole + RISE + 10, 0, (pole + RISE + 10) / 2 - 6, 0, C.handle, 10), 0.004));
  const tip = new THREE.Mesh(new THREE.SphereGeometry(u(4.5), 12, 8), flat(C.yellow)); tip.position.y = u(pole + RISE + 5); g.add(ol(tip, 0.004));
  return g;
}

// --- tambor: corpo de triângulos coloridos, aros de madeira, pele clara em cima e cordas em zigue-zague
export function tambor() {
  const g = new THREE.Group(), R = 34, H = 56;
  const m = pattern(flat(C.red), `
    { float a = atan(vP.z, vP.x) / 6.2831853 * 8.0; float s = floor(a + 8.0); float f = fract(a); float v = (vP.y + ${(H / 2).toFixed(1)}) / ${H.toFixed(1)};
      bool up = v < 1.0 - abs(2.0 * f - 1.0);
      vec3 cA = mod(s, 2.0) < 1.0 ? uYel : uBlue; diffuseColor.rgb = up ? cA : uRed; }`, 'tambor', { uYel: col(C.yellow), uBlue: col(C.cyan), uRed: col(C.red) });
  const body = new THREE.Mesh(new THREE.CylinderGeometry(u(R), u(R), u(H), 40), m); body.position.y = u(H / 2 + 4); g.add(ol(body, 0.007));
  for (const y of [5, H + 3]) { const rim = new THREE.Mesh(new THREE.TorusGeometry(u(R + 0.5), u(3.6), 8, 40), flat(C.drumRim)); rim.rotation.x = Math.PI / 2; rim.position.y = u(y); g.add(ol(rim, 0.004)); }
  const skin = cyl(R - 1, 2, 0, H + 5, 0, C.skin, 40); g.add(skin);
  for (let k = 0; k < 8; k++) {   // cordas: diagonais entre os aros
    const a0 = k / 8 * Math.PI * 2, a1 = (k + 0.5) / 8 * Math.PI * 2;
    const A = new THREE.Vector3(Math.cos(a0) * (R + 1.5), 7, Math.sin(a0) * (R + 1.5)), B = new THREE.Vector3(Math.cos(a1) * (R + 1.5), H + 1, Math.sin(a1) * (R + 1.5));
    const len = A.distanceTo(B), r = new THREE.Mesh(new THREE.CylinderGeometry(u(1.1), u(1.1), u(len), 6), flat(C.rope));
    r.position.copy(A.clone().add(B).multiplyScalar(0.5 * UNIT)); r.quaternion.setFromUnitVectors(new THREE.Vector3(0, 1, 0), B.clone().sub(A).normalize()); g.add(r);
  }
  return g;
}

// --- parapeito: trecho reto de madeira (len de comprimento), pilares nas pontas com capitel vermelho e faixa ciano,
// corrimão, travessa de baixo e balaústres
export function parapeito(len = 150, endPillar = true) {
  const g = new THREE.Group(), H = 60;
  const pillar = x => { g.add(ol(box(18, H, 18, x, H / 2, 0, C.wood, 2), 0.005)); g.add(ol(box(19, 6, 19, x, H - 14, 0, C.teal, 1), 0.003)); g.add(ol(box(23, 8, 23, x, H + 4, 0, C.red, 2), 0.005)); };
  pillar(-len / 2); if (endPillar) pillar(len / 2);
  g.add(ol(box(len, 6, 9, 0, H - 4, 0, C.wood, 1.5), 0.005));
  g.add(ol(box(len, 5, 8, 0, 10, 0, C.woodDark, 1.5), 0.004));
  for (let x = -len / 2 + 16; x <= len / 2 - 15; x += 14) g.add(ol(box(4.5, H - 18, 4.5, x, (H - 4 + 10) / 2, 0, C.wood, 1), 0.003));
  return g;
}

// --- arcada colonial: parede coral com três arcos (molduras claras, portas azuis no fundo), cornija e telhado de barro
export function arcada(wall = C.coral) {
  const g = new THREE.Group(), W = 420, H = 300, D = 40, OW = 96, OH = 150, N = 3;
  const xs = [-1, 0, 1].map(i => i * (W / N));
  const s = new THREE.Shape(); s.moveTo(u(-W / 2), 0); s.lineTo(u(W / 2), 0); s.lineTo(u(W / 2), u(H)); s.lineTo(u(-W / 2), u(H)); s.closePath();
  const arch = (cx, w, h) => { const p = new THREE.Path(); p.moveTo(u(cx - w / 2), 0); p.lineTo(u(cx + w / 2), 0); p.lineTo(u(cx + w / 2), u(h)); p.absarc(u(cx), u(h), u(w / 2), 0, Math.PI, false); p.lineTo(u(cx - w / 2), 0); return p; };
  for (const x of xs) s.holes.push(arch(x, OW, OH));
  const wg = new THREE.ExtrudeGeometry(s, { depth: u(D), bevelEnabled: false, curveSegments: 16 }); wg.translate(0, 0, u(-D / 2));
  g.add(ol(new THREE.Mesh(wg, flat(wall)), 0.008));
  for (const x of xs) {   // moldura do arco (anel) e porta no fundo
    const fr = new THREE.Shape(); fr.moveTo(u(x - OW / 2 - 11), 0); fr.lineTo(u(x - OW / 2 - 11), u(OH)); fr.absarc(u(x), u(OH), u(OW / 2 + 11), Math.PI, 0, true); fr.lineTo(u(x + OW / 2 + 11), 0); fr.lineTo(u(x + OW / 2), 0); fr.lineTo(u(x + OW / 2), u(OH)); fr.absarc(u(x), u(OH), u(OW / 2), 0, Math.PI, false); fr.lineTo(u(x - OW / 2), 0); fr.closePath();
    const fg = new THREE.ExtrudeGeometry(fr, { depth: u(6), bevelEnabled: false, curveSegments: 16 }); fg.translate(0, 0, u(D / 2));
    g.add(ol(new THREE.Mesh(fg, flat(C.trim)), 0.005));
    const dr = new THREE.Shape(); dr.moveTo(u(x - 30), 0); dr.lineTo(u(x + 30), 0); dr.lineTo(u(x + 30), u(110)); dr.absarc(u(x), u(110), u(30), 0, Math.PI, false); dr.closePath();
    const dg = new THREE.ExtrudeGeometry(dr, { depth: u(4), bevelEnabled: false, curveSegments: 12 }); dg.translate(0, 0, u(-D / 2 - 30));
    g.add(ol(new THREE.Mesh(dg, flat(C.door)), 0.004));
  }
  g.add(ol(box(W, OH + 60, 4, 0, (OH + 60) / 2, -D / 2 - 34, '#c96a36', 1), 0.004));   // fundo do alpendre
  g.add(ol(box(W, 4, 64, 0, 2, -D / 2 - 12, C.trim, 1), 0.003));                       // piso do alpendre
  for (const x of [-W / 2, W / 2]) g.add(ol(box(18, H, D + 8, x, H / 2, 0, C.trim, 2), 0.005));
  g.add(ol(box(W + 24, 12, D + 16, 0, H + 6, 0, C.trim, 2), 0.006));                   // cornija
  // telhado de duas águas, telhas em linhas
  const rs = new THREE.Shape(); rs.moveTo(u(-(D + 70) / 2), 0); rs.lineTo(u((D + 70) / 2), 0); rs.lineTo(0, u(62)); rs.closePath();
  const rg = new THREE.ExtrudeGeometry(rs, { depth: u(W + 40), bevelEnabled: false }); rg.translate(0, 0, u(-(W + 40) / 2)); rg.rotateY(Math.PI / 2);
  const rm = pattern(flat(C.roof), 'if (fract(vP.x / 18.0) < 0.12 || fract((vP.y + abs(vP.z) * 0.9) / 14.0) < 0.14) diffuseColor.rgb = uLine;', 'telha', { uLine: col(C.roofLine) });
  const roof = new THREE.Mesh(rg, rm); roof.position.y = u(H + 12); g.add(ol(roof, 0.006));
  return g;
}

// --- casa colonial (sobrado)
export function casa(wall = HOUSE[0], W = 200, H = 300) {
  // sobrado colonial: barra de azulejo, cunhais de pedra, friso entre os andares, cornija; em cima janelas altas de
  // verga em arco com venezianas e sacada de grade; embaixo porta em arco e janelas; telhado de duas águas com beiral
  const g = new THREE.Group(), D = 170, F = D / 2, IRON = '#2b2622', SH = '#3f6e4e';
  const m = pattern(flat(wall), `
    if (vP.y < 34.0 && abs(vP.z) > ${(F - 1).toFixed(1)}) { vec2 q = vec2(vP.x, vP.y) / 11.0; vec2 f = fract(q) - 0.5;
      diffuseColor.rgb = (abs(f.x) + abs(f.y) < 0.3) ? uBlue : uTile; }`, 'casa' + wall, { uBlue: col(C.tileBlue), uTile: col(C.tile) });
  g.add(ol(new THREE.Mesh(new THREE.BoxGeometry(u(W), u(H), u(D)).translate(0, u(H / 2), 0), m), 0.008));
  // cunhais: blocos de pedra alternados nas quinas da frente
  for (const sx of [-1, 1]) for (let y = 0, k = 0; y < H - 8; y += 26, k++) { const w = k % 2 ? 14 : 22; g.add(ol(box(w, 24, 6, sx * (W / 2 - w / 2 + 1), y + 13, F + 2, C.trim, 1), 0.003)); }
  g.add(ol(box(W + 6, 9, 8, 0, H * 0.5, F + 3, C.trim, 1.5), 0.004));                       // friso entre os andares
  g.add(ol(box(W + 14, 9, D + 14, 0, H - 4, 0, C.trim, 1.5), 0.005));                       // cornija
  g.add(ol(box(W + 22, 7, D + 22, 0, H + 3, 0, C.trim, 1.5), 0.005));
  const arched = (w, h) => { const s = new THREE.Shape(); s.moveTo(u(-w / 2), 0); s.lineTo(u(w / 2), 0); s.lineTo(u(w / 2), u(h - w * 0.28)); s.quadraticCurveTo(0, u(h + w * 0.12), u(-w / 2), u(h - w * 0.28)); s.closePath(); return s; };
  const plate = (shape, depth, color, x, y, z) => { const e = new THREE.Mesh(new THREE.ExtrudeGeometry(shape, { depth: u(depth), bevelEnabled: false, curveSegments: 10 }), flat(color)); e.position.set(u(x), u(y), u(z)); return ol(e, 0.003); };
  const win = (x, y, w, h, balcony) => {
    g.add(plate(arched(w + 12, h + 10), 4, C.trim, x, y - 5, F));
    for (const s of [-1, 1]) g.add(plate(arched(w / 2 - 1, h), 3, SH, x + s * w / 4, y, F + 3));   // duas folhas de veneziana
    if (balcony) {
      g.add(ol(box(w + 26, 5, 24, x, y - 4, F + 12, C.trim, 1.5), 0.003));
      g.add(ol(box(w + 26, 3, 3, x, y + 30, F + 23, IRON, 1), 0.002));
      for (let k = 0; k <= 6; k++) g.add(box(2, 32, 2, x - (w + 22) / 2 + k * (w + 22) / 6, y + 14, F + 23, IRON, 0.5));
    }
  };
  const n = W >= 190 ? 3 : 2, step = W / (n + 0.4);
  for (let i = 0; i < n; i++) win(-step * (n - 1) / 2 + i * step, H * 0.58, 30, 64, true);
  const doorX = n === 3 ? 0 : step / 2;
  g.add(plate(arched(46, 92), 4, C.trim, doorX, 0, F)); g.add(plate(arched(36, 86), 3, C.shutter, doorX, 0, F + 3));
  for (let i = 0; i < n; i++) { const x = -step * (n - 1) / 2 + i * step; if (Math.abs(x - doorX) > 30) win(x, H * 0.16, 28, 50, false); }
  // telhado de duas águas: oitões na cor da parede e as duas águas de telha com beiral
  const RH = 64, gs = new THREE.Shape(); gs.moveTo(u(-F), 0); gs.lineTo(u(F), 0); gs.lineTo(0, u(RH)); gs.closePath();
  const gable = new THREE.Mesh(new THREE.ExtrudeGeometry(gs, { depth: u(W), bevelEnabled: false }).translate(0, 0, u(-W / 2)).rotateY(Math.PI / 2), flat(wall));
  gable.position.y = u(H + 6); g.add(ol(gable, 0.004));
  const slope = Math.atan2(RH, F), len = Math.hypot(F, RH) + 22;
  const tm = pattern(flat(C.roof), 'if (fract(vP.x / 16.0) < 0.12 || fract(vP.z / 13.0) < 0.12) diffuseColor.rgb = uLine;', 'telhaC', { uLine: col(C.roofLine) });
  for (const s of [-1, 1]) {
    const slab = new THREE.Mesh(new THREE.BoxGeometry(u(W + 24), u(7), u(len)), tm);
    slab.rotation.x = s * slope; slab.position.set(0, u(H + 6 + RH / 2 + 2), u(s * (F / 2 + 4))); g.add(ol(slab, 0.005));
  }
  return g;
}

// --- o entorno: posições em unidades do jogo (o ringue tem meia largura 270; a câmera do jogo fica do lado +z)
export function buildPernambucoProps() {
  const all = new THREE.Group(); all.position.y = u(-APRON);
  const put = (obj, x, z, ry = 0, s = 1) => { obj.position.set(u(x), 0, u(z)); obj.rotation.y = ry; obj.scale.setScalar(s); all.add(obj); return obj; };
  const sink = obj => { obj.position.y = u(-3); return obj; };   // piso do alpendre não coincide com o chão (sem piscar)
  // parapeito: atrás e dos lados, em trechos de 146
  const L = 146, R0 = 440;
  for (let i = 0; i < 6; i++) put(parapeito(L, i === 5), -R0 + L / 2 + i * L, -R0);
  for (const sx of [-1, 1]) for (let i = 0; i < 4; i++) put(parapeito(L, i === 3), sx * R0, -R0 + L / 2 + i * L, Math.PI / 2);
  // arcadas: fundo e laterais (de frente para o ringue)
  for (const sx of [-1, 1]) for (const z of [-340, 30]) sink(put(arcada(z > 0 ? '#e9663a' : C.coral), sx * 820, z, -sx * Math.PI / 2, 0.88));
  // casario atrás das arcadas, alturas e cores variadas
  // (o fundo é aberto: ponte, morro e igreja vêm de arena_pernambuco_bg.js)
  // lado da câmera (+z): arcadas viradas para o ringue, sobrados atrás, e casas fechando as quinas
  for (const x of [-420, 0, 420]) sink(put(arcada(x === 0 ? '#e9663a' : C.coral), x, 860, Math.PI, 0.88));
  [-760, -500, -250, 0, 250, 500, 760].forEach((x, i) => put(casa(HOUSE[(i + 2) % HOUSE.length], 200 + (i % 3) * 15, 320 + ((i * 37) % 3) * 40), x, 1180, Math.PI));
  for (const sx of [-1, 1]) put(casa(HOUSE[sx > 0 ? 4 : 1], 210, 360), sx * 860, 700, Math.PI + sx * Math.PI / 4);
  for (const sx of [-1, 1]) [-560, -300, -40, 220].forEach((z, i) => put(casa(HOUSE[(i + (sx > 0 ? 3 : 1)) % HOUSE.length], 200, 360 + (i % 2) * 70), sx * 1180, z, -sx * Math.PI / 2));
  // tambores nas quinas (na frente, ao lado das escadas; atrás, dentro do parapeito)
  for (const [x, z, s] of [[-370, 330, 1], [370, 330, 1], [-360, -360, 1.15], [360, -360, 0.9], [-280, -400, 0.8]]) put(tambor(), x, z, x * 0.01, s);
  // sombrinhas grandes em hastes nas quinas e nas laterais
  for (const [x, z, ph, s] of [[-540, -400, 0, 1.25], [540, -400, 0.4, 1.25], [-580, 160, 0.8, 1.1], [580, 160, 0.2, 1.1]]) {
    const sb = put(sombrinha(230, ph), x, z, ph, s); sb.rotation.z = (x > 0 ? -1 : 1) * 0.08;
  }
  return all;
}
