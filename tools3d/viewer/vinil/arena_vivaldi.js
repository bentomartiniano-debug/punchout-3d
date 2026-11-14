// Teatro circular da Maestra Vivaldi (arena da Vivaldi), em código e no acabamento do vinil, seguindo a sala do jogo
// (engine-cpp/app/ring_renderer.cpp queueConcertHall e os estilos "concert"): o ringue é um tablado de palco de madeira
// com a saia de veludo e franja dourada, postes de ouro canelados e cordas vinho/creme; em volta, as arquibancadas em
// quatro degraus com poltronas de veludo vazias, o balcão com balaústres, a ribalta de latão; na parede da sala
// (raio 1300) a cortina de veludo em pregas com o lambrequim recortado e franja de ouro, os vitrais das quatro
// estações (a luz deles cai em manchas coloridas no assoalho), colunas douradas caneladas, quatro órgãos de tubos,
// cimalhas de ouro e a cúpula escura com o lustre. Medidas em unidades do jogo (UNIT m); piso do ringue em y = 0.
import { THREE, flat, withOutline } from './core.js';
import { RoundedBoxGeometry } from 'three/addons/geometries/RoundedBoxGeometry.js';
import { mergeGeometries } from 'three/addons/utils/BufferGeometryUtils.js';
import { UNIT } from './arena_pernambuco.js';

const u = v => v * UNIT;
const HALF = 270, EDGE = 292, APRON = 35, POST_TOP = 125, ROPES = [38, 68, 98], HALL = 1300, FLOOR = -38;
const C = { boardA: '#79513e', boardB: '#885b43', boardLine: '#a67550', boardGold: '#c59460', apron: ['#422737', '#583343'], gold: '#d6a974', goldLight: '#f2cf94',
            goldDark: '#b68762', brass: '#c99a61', ropes: ['#964c61', '#f2d3a2', '#964c61'], pads: ['#e2b374', '#994c65', '#e2b374', '#994c65'], postBase: '#40263a',
            parquet: ['#392634', '#462c38'], parquetLine: '#271926', wall: ['#321b2d', '#422033'], drape: '#86394f', drapeDark: '#5f2638', valance: '#6b2438',
            fringe: '#d5a96e', column: '#b98a61', columnDark: '#3d2939', organWood: '#2d1825', pipeA: '#dcc4a0', pipeB: '#bca8a8', seat: '#8a2e48', seatBack: '#6e2139',
            ceiling: '#2a1626', ink: '#14141c' };
const TIERS = [{ rIn: 270, rOut: 410, h: -35, hPrev: -35, st: [0x5e3942, 0x794552, 0x372232, 0xc29565] }, { rIn: 410, rOut: 510, h: 18, hPrev: -35, st: [0x593342, 0x713d51, 0x321f31, 0xc29565] },
               { rIn: 510, rOut: 620, h: 68, hPrev: 18, st: [0x4a2b3d, 0x62364b, 0x2b1b2c, 0xb68762] }, { rIn: 620, rOut: 730, h: 118, hPrev: 68, st: [0x40263a, 0x59334a, 0x241729, 0x9c705a] }];
const SEASONS = ['#8bbf91', '#f6ca76', '#d87968', '#91c9d8'];   // primavera, verão, outono, inverno (as quatro estações)

const hex = n => '#' + n.toString(16).padStart(6, '0');
const ol = (m, w = 0.007) => { const g = withOutline(m, w); g.position.copy(m.position); g.quaternion.copy(m.quaternion); g.scale.copy(m.scale); m.position.set(0, 0, 0); m.quaternion.identity(); m.scale.set(1, 1, 1); return g; };
const box = (w, h, d, x, y, z, col, r = 1.5) => { const m = new THREE.Mesh(new RoundedBoxGeometry(u(w), u(h), u(d), 2, u(Math.min(r, w / 2 - 0.01, h / 2 - 0.01, d / 2 - 0.01))), typeof col === 'string' ? flat(col) : col); m.position.set(u(x), u(y), u(z)); return m; };
const P = (a, r, y) => new THREE.Vector3(u(Math.sin(a) * r), u(y), u(Math.cos(a) * r));   // polar do jogo: ângulo 0 = +z (lado da câmera)
const tick = { t: { value: 0 } };
// contorno para malhas instanciadas: a mesma casca do withOutline, mas instanciada junto
const hullMat = w => withOutline(new THREE.Mesh(new THREE.BoxGeometry(1, 1, 1), flat('#fff')), w).children[1].material;
function instanced(geo, mat, mats, w = 0.004) {
  const m = new THREE.InstancedMesh(geo, mat, mats.length); mats.forEach((M, i) => m.setMatrixAt(i, M));
  const g = new THREE.Group(); g.add(m);
  if (w) { const h = new THREE.InstancedMesh(geo, hullMat(w), mats.length); h.instanceMatrix = m.instanceMatrix; g.add(h); }
  return g;
}
const mat4 = (x, y, z, ry = 0, sx = 1, sy = 1, sz = 1) => new THREE.Matrix4().compose(new THREE.Vector3(u(x), u(y), u(z)), new THREE.Quaternion().setFromAxisAngle(new THREE.Vector3(0, 1, 0), ry), new THREE.Vector3(sx, sy, sz));
// anel (coroa circular) horizontal ou parede cilíndrica, em segmentos
function annulus(rIn, rOut, y, col, seg = 96) { const g = new THREE.RingGeometry(u(rIn), u(rOut), seg, 1); g.rotateX(-Math.PI / 2); g.translate(0, u(y), 0); const m = flat(col); m.side = THREE.DoubleSide; return new THREE.Mesh(g, m); }
function wall(r, y0, y1, col, seg = 96, side = THREE.DoubleSide) { const g = new THREE.CylinderGeometry(u(r), u(r), u(y1 - y0), seg, 1, true); g.translate(0, u((y0 + y1) / 2), 0); const m = flat(col); m.side = side; return new THREE.Mesh(g, m); }

// ================= tablado-ringue: piso de palco (tábuas em xadrez, filetes, friso de ouro), saia de veludo com pregas
// e franja dourada, postes canelados de ouro com proteções vinho/ouro, cordas, escadinhas de palco nas quinas da frente
function floorTexture() {
  const N = 2048, c = document.createElement('canvas'); c.width = c.height = N;
  const g = c.getContext('2d'), k = N / (2 * EDGE), Px = v => (v + EDGE) * k;
  g.fillStyle = hex(0x583343); g.fillRect(0, 0, N, N);   // beirada fora do quadrado jogável: veludo escuro
  const T = 54;
  for (let ix = 0; ix < 10; ix++) for (let iz = 0; iz < 10; iz++) {   // tábuas em xadrez (cor do jogo), as de um quadrado em pares
    g.fillStyle = (ix + iz) % 2 ? C.boardB : C.boardA; g.fillRect(Px(-HALF + ix * T), Px(-HALF + iz * T), T * k, T * k);
    g.strokeStyle = 'rgba(40,20,20,0.18)'; g.lineWidth = 1.2 * k; g.beginPath();
    for (let s = 1; s < 4; s++) { if ((ix + iz) % 2) { g.moveTo(Px(-HALF + ix * T + s * T / 4), Px(-HALF + iz * T)); g.lineTo(Px(-HALF + ix * T + s * T / 4), Px(-HALF + (iz + 1) * T)); }
      else { g.moveTo(Px(-HALF + ix * T), Px(-HALF + iz * T + s * T / 4)); g.lineTo(Px(-HALF + (ix + 1) * T), Px(-HALF + iz * T + s * T / 4)); } }
    g.stroke(); }
  g.strokeStyle = C.boardLine; g.lineWidth = 1.6 * k; g.beginPath();
  for (let i = -4; i <= 4; i++) { g.moveTo(Px(i * T), Px(-HALF)); g.lineTo(Px(i * T), Px(HALF)); } g.stroke();   // filetes do jogo
  g.strokeStyle = C.boardGold; g.lineWidth = 2.4 * k; g.beginPath(); for (const z of [-215, 215]) { g.moveTo(Px(-HALF), Px(z)); g.lineTo(Px(HALF), Px(z)); } g.stroke();
  // rosácea discreta no meio (clave de ouro apagada: legível sem disputar atenção com os pés)
  g.strokeStyle = 'rgba(216,170,110,0.55)'; g.lineWidth = 2.2 * k; g.beginPath(); g.arc(Px(0), Px(0), 62 * k, 0, 7); g.stroke();
  g.lineWidth = 1.4 * k; g.beginPath(); g.arc(Px(0), Px(0), 50 * k, 0, 7); g.stroke();
  for (let i = 0; i < 8; i++) { const a = i / 8 * Math.PI * 2, b = a + Math.PI / 8; g.beginPath(); g.moveTo(Px(Math.cos(a) * 50), Px(Math.sin(a) * 50)); g.lineTo(Px(Math.cos(b) * 26), Px(Math.sin(b) * 26)); g.lineTo(Px(Math.cos(a + Math.PI / 4) * 50), Px(Math.sin(a + Math.PI / 4) * 50)); g.stroke(); }
  g.strokeStyle = C.gold; g.lineWidth = 4 * k; g.strokeRect(Px(-HALF), Px(-HALF), 2 * HALF * k, 2 * HALF * k);
  const t = new THREE.CanvasTexture(c); t.colorSpace = THREE.SRGBColorSpace; t.anisotropy = 8; return t;
}
// cortina de veludo em pregas (painel reto ou cilíndrico): faixa com o raio ondulando, as pregas de luz e sombra vêm da
// luz; bottom/top em unidades do jogo; cyl: [raio, a0, a1] ou null para painel (largura w, centrado em x = 0)
function pleated({ w = 0, cyl = null, y0, y1, folds, depth = 7, col, scallop = 0, nSeg = 0 }) {
  const NX = nSeg || Math.max(24, folds * 8), NY = 2, pos = [], idx = [];
  for (let j = 0; j <= NY; j++) for (let i = 0; i <= NX; i++) {
    const t = i / NX, f = Math.sin(t * folds * Math.PI * 2), sc = scallop ? scallop * Math.abs(Math.sin(t * folds * Math.PI)) : 0;
    const y = j === 0 ? y0 + sc : y0 + (y1 - y0) * j / NY;
    if (cyl) { const [R, a0, a1] = cyl, a = a0 + (a1 - a0) * t, r = R - depth * (0.5 + 0.5 * f); pos.push(u(Math.sin(a) * r), u(y), u(Math.cos(a) * r)); }
    else pos.push(u(-w / 2 + w * t), u(y), u(depth * (0.5 + 0.5 * f)));
  }
  for (let j = 0; j < NY; j++) for (let i = 0; i < NX; i++) { const a = j * (NX + 1) + i, b = a + 1, c = a + NX + 1, d = c + 1; idx.push(a, b, c, b, d, c); }
  const g = new THREE.BufferGeometry(); g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3)); g.setIndex(idx); g.computeVertexNormals();
  const m = flat(col); m.side = THREE.DoubleSide; return new THREE.Mesh(g, m);
}
function tablado() {
  const ring = new THREE.Group();
  { const top = new THREE.Mesh(new THREE.BoxGeometry(u(2 * EDGE), u(6), u(2 * EDGE)), [flat(C.apron[1]), flat(C.apron[1]), new THREE.MeshToonMaterial({ map: floorTexture(), gradientMap: flat('#fff').gradientMap }), flat(C.apron[1]), flat(C.apron[1]), flat(C.apron[1])]);
    top.position.y = u(-3); ring.add(ol(top, 0.007)); }
  ring.add(box(2 * EDGE - 6, APRON - 4, 2 * EDGE - 6, 0, -APRON / 2 - 4, 0, C.apron[0], 1));   // miolo (atrás da saia)
  // saia de veludo em pregas nos quatro lados, com o friso de ouro em cima e a franja embaixo
  for (const [nx, nz] of [[0, 1], [1, 0], [0, -1], [-1, 0]]) {
    const q = new THREE.Quaternion().setFromUnitVectors(new THREE.Vector3(0, 0, 1), new THREE.Vector3(nx, 0, nz));
    const sk = pleated({ w: 2 * EDGE + 4, y0: -APRON, y1: -6, folds: 18, depth: 5, col: C.apron[(nx + nz + 2) % 2 ? 1 : 0] });
    sk.quaternion.copy(q); sk.position.set(u(nx * (EDGE - 2)), 0, u(nz * (EDGE - 2))); ring.add(ol(sk, 0.004));
    const trim = box(2 * EDGE + 10, 5, 5, 0, -8, 0, C.gold, 1.5); trim.quaternion.copy(q); trim.position.set(u(nx * (EDGE + 2)), u(-8), u(nz * (EDGE + 2))); ring.add(ol(trim, 0.004));
    const fr = box(2 * EDGE + 8, 4, 3, 0, -APRON + 2, 0, C.fringe, 1); fr.quaternion.copy(q); fr.position.set(u(nx * (EDGE + 4)), u(-APRON + 2), u(nz * (EDGE + 4))); ring.add(fr);
  }
  // postes: base vinho, fuste dourado canelado (16 caneluras), capitel e pinha; proteções nas alturas das cordas
  const corners = [[-HALF, HALF], [HALF, HALF], [HALF, -HALF], [-HALF, -HALF]];
  const fust = (() => { const g = new THREE.CylinderGeometry(u(10), u(11), u(POST_TOP + APRON - 18), 32, 1); const p = g.attributes.position;
    for (let i = 0; i < p.count; i++) { const x = p.getX(i), z = p.getZ(i), a = Math.atan2(z, x), k = 1 - 0.07 * Math.max(0, Math.cos(a * 16)); p.setX(i, x * k); p.setZ(i, z * k); } g.computeVertexNormals(); return g; })();
  corners.forEach(([cx, cz], ci) => {
    ring.add(ol(box(32, 16, 32, cx, -APRON + 8, cz, C.postBase, 2), 0.006));
    const f = new THREE.Mesh(fust, flat(C.gold)); f.position.set(u(cx), u((-APRON + 16 + POST_TOP - 2) / 2), u(cz)); ring.add(ol(f, 0.006));
    ring.add(ol(box(28, 8, 28, cx, POST_TOP + 2, cz, C.goldLight, 2), 0.005));
    const pin = new THREE.Mesh(new THREE.SphereGeometry(u(9), 18, 12), flat(C.goldLight)); pin.position.set(u(cx), u(POST_TOP + 14), u(cz)); ring.add(ol(pin, 0.005));
    const toC = new THREE.Vector3(-cx, 0, -cz).normalize();
    ROPES.forEach(h => { const p = box(16, 16, 14, cx + toC.x * 13, h, cz + toC.z * 13, C.pads[ci], 4); p.rotation.y = Math.atan2(toC.x, toC.z); ring.add(ol(p, 0.005)); });
  });
  for (let i = 0; i < 4; i++) {
    const [ax, az] = corners[i], [bx, bz] = corners[(i + 1) % 4];
    ROPES.forEach((h, r) => {
      const a = new THREE.Vector3(u(ax), u(h), u(az)), b = new THREE.Vector3(u(bx), u(h), u(bz)), len = a.distanceTo(b);
      const g = new THREE.CylinderGeometry(u(2.4), u(2.4), len, 10); g.rotateZ(Math.PI / 2);
      const m = new THREE.Mesh(g, flat(C.ropes[r])); m.position.copy(a).add(b).multiplyScalar(0.5); m.rotation.y = -Math.atan2(b.z - a.z, b.x - a.x); ring.add(ol(m, 0.004));
    });
  }
  // escadinhas de palco nas duas quinas da frente: degraus de madeira com tapete vinho e o focinho dourado
  for (const sx of [-1, 1]) for (let k = 0; k < 3; k++) {
    const h = APRON * (3 - k) / 3, d = 22, z = EDGE + d / 2 + k * d - 2;
    ring.add(ol(box(64, h, d, sx * (HALF - 50), -APRON + h / 2, z, C.apron[0], 1.5), 0.006));
    ring.add(box(58, 2, d - 4, sx * (HALF - 50), -APRON + h + 0.5, z, C.ropes[0], 0.8));
    ring.add(box(64, 3, 3, sx * (HALF - 50), -APRON + h - 1, z + d / 2 - 1, C.gold, 1));
  }
  return ring;
}

// ================= assoalho da sala: parquete em gomos (duas madeiras), juntas radiais e anéis
function assoalho() {
  const g = new THREE.CircleGeometry(u(1400), 96); g.rotateX(-Math.PI / 2); g.translate(0, u(FLOOR), 0);
  const m = flat(C.parquet[0]);
  m.onBeforeCompile = sh => {
    sh.vertexShader = 'varying vec2 vG;\n' + sh.vertexShader.replace('#include <begin_vertex>', '#include <begin_vertex>\nvG = position.xz / ' + UNIT.toFixed(5) + ';');
    sh.fragmentShader = 'uniform vec3 uB, uL;\nvarying vec2 vG;\n' + sh.fragmentShader.replace('#include <map_fragment>', `#include <map_fragment>
      { float a = atan(vG.x, vG.y) / 6.2831853 * 24.0, r = length(vG);
        if (mod(floor(a), 2.0) > 0.5) diffuseColor.rgb = uB;
        float wa = fwidth(a) * 1.2, wr = fwidth(r) * 1.2;
        if (abs(fract(a) - 0.5) > 0.5 - wa || abs(mod(r, 170.0) - 85.0) > 85.0 - wr * 1.5) diffuseColor.rgb = uL; } }`);
    sh.uniforms.uB = { value: new THREE.Color(C.parquet[1]) }; sh.uniforms.uL = { value: new THREE.Color(C.parquetLine) };
  };
  m.customProgramCacheKey = () => 'vivParquet';
  return new THREE.Mesh(g, m);
}

// ================= arquibancadas: quatro degraus em anel (piso, espelho, friso), poltronas de veludo nos degraus de
// cima (vazias: a plateia fica de fora do vinil) e o parapeito com a ribalta de latão no último
const hash = n => { const v = Math.sin(n * 127.1 + 311.7) * 43758.5453; return v - Math.floor(v); };
function arquibancadas() {
  const g = new THREE.Group();
  TIERS.forEach((T, t) => {
    const [plat, seat, riser, trim] = T.st.map(hex);
    g.add(annulus(T.rIn, T.rOut, T.h, plat));
    if (T.h !== T.hPrev) g.add(ol(wall(T.rIn, T.hPrev, T.h, riser), 0.004));
    const fr = new THREE.Mesh(new THREE.TorusGeometry(u(T.rIn), u(2.2), 6, 120), flat(hex(T.st[3]))); fr.rotation.x = Math.PI / 2; fr.position.y = u(T.h + 1); g.add(fr);
    if (t > 0) g.add(annulus(T.rIn + 18, T.rIn + 44, T.h + 0.5, seat));   // faixa do tapete das poltronas
  });
  // parede do fundo das arquibancadas (fecha embaixo do balcão) e parapeito com a ribalta
  g.add(ol(wall(730, -35, 140, hex(0x372232)), 0.004));
  { const top = new THREE.Mesh(new THREE.TorusGeometry(u(730), u(5), 8, 140), flat(C.gold)); top.rotation.x = Math.PI / 2; top.position.y = u(140); g.add(top); }
  // poltronas de veludo (encosto e assento) nos três degraus de cima, instanciadas
  const backs = [], seats = [];
  TIERS.slice(1).forEach((T, k) => {
    const r = T.rIn + 34, n = Math.floor(2 * Math.PI * r / 46);
    for (let i = 0; i < n; i++) { const a = (i + (k % 2) * 0.5) / n * Math.PI * 2, x = Math.sin(a) * r, z = Math.cos(a) * r;
      if (z > 380 && Math.abs(x) < 460) continue;   // corredor da câmera (ela fica no alto do último degrau)
      backs.push(mat4(Math.sin(a) * (r + 13), T.h + 22, Math.cos(a) * (r + 13), a)); seats.push(mat4(x, T.h + 8, z, a));
      }
  });
  g.add(instanced(new RoundedBoxGeometry(u(36), u(34), u(8), 2, u(3)), flat(C.seatBack), backs), instanced(new RoundedBoxGeometry(u(36), u(10), u(26), 2, u(3)), flat(C.seat), seats));
  // ribalta: 20 luminárias de latão no parapeito, com a chama quente pulsando
  const lampM = new THREE.MeshBasicMaterial({ color: '#f6d29c' }), shells = [], lamps = [];
  for (let i = 0; i < 20; i++) { const a = (i + 0.5) / 20 * Math.PI * 2; shells.push(mat4(Math.sin(a) * 728, 146, Math.cos(a) * 728, a)); lamps.push(mat4(Math.sin(a) * 728, 154, Math.cos(a) * 728, a)); }
  const lampG = instanced(new THREE.SphereGeometry(u(6), 12, 8), lampM, lamps, 0);
  g.add(instanced(new THREE.CylinderGeometry(u(8), u(5), u(10), 12), flat('#8c5b36'), shells), lampG);
  // chama pulsando devagar (o mesmo pulso do jogo)
  const base = new THREE.Color('#f6ca8b'), clock = new THREE.Clock();
  lampG.children[0].onBeforeRender = () => { lampM.color.copy(base).multiplyScalar(0.98 + 0.17 * Math.sin(clock.getElapsedTime() * 3.1)); };
  return g;
}

// ================= balcão: laje em anel no alto (sobre as arquibancadas de fora), balaustrada de ouro na frente,
// colunas finas por baixo
function balcao() {
  const g = new THREE.Group(), R0 = 840, R1 = 1180, Y = 175;
  g.add(ol(annulus(R0, R1, Y, hex(0x4a2b3d)), 0.004), ol(wall(R0, Y - 16, Y, hex(0x6b3a52)), 0.004));
  { const m = new THREE.Mesh(new THREE.RingGeometry(u(R0), u(R1), 96, 1), flat(hex(0x2b1b2c))); m.rotation.x = Math.PI / 2; m.position.y = u(Y - 16); m.material.side = THREE.DoubleSide; g.add(m); }   // forro por baixo
  for (const [y, r, col] of [[Y + 30, 4, C.gold], [Y + 3, 3, C.goldDark], [Y - 16, 3, C.gold]]) { const t = new THREE.Mesh(new THREE.TorusGeometry(u(R0 - 2), u(r), 6, 160), flat(col)); t.rotation.x = Math.PI / 2; t.position.y = u(y); g.add(t); }
  const bal = [], cols = [];
  for (let i = 0; i < 96; i++) { const a = (i + 0.5) / 96 * Math.PI * 2; bal.push(mat4(Math.sin(a) * (R0 - 2), Y + 16, Math.cos(a) * (R0 - 2), a)); }
  for (let i = 0; i < 16; i++) { const a = i / 16 * Math.PI * 2; cols.push(mat4(Math.sin(a) * (R0 + 22), (FLOOR + Y - 16) / 2, Math.cos(a) * (R0 + 22), a, 1, (Y - 16 - FLOOR) / 100, 1)); }
  const bg = new THREE.LatheGeometry([[0, -13], [3.4, -13], [3.4, -10], [2, -7], [4.6, 0], [2, 7], [3, 10], [3, 13], [0, 13]].map(([r, y]) => new THREE.Vector2(u(r), u(y))), 10);
  g.add(instanced(bg, flat(C.goldDark), bal, 0.003), instanced(new THREE.CylinderGeometry(u(7), u(8), u(100), 12), flat(C.gold), cols));
  return g;
}

// ================= paredes da sala: rodapé, veludo, cortina em pregas com lambrequim recortado e franja, cimalhas
// de ouro, vitrais das quatro estações (com a mancha de luz no assoalho), colunas caneladas, órgãos e a cúpula
function sala() {
  const g = new THREE.Group(), BAYS = 24, step = Math.PI * 2 / BAYS;
  g.add(wall(HALL, FLOOR, 540, C.wall[0], 96, THREE.BackSide));
  g.add(ol(wall(HALL - 8, FLOOR, -8, hex(0x3a2030)), 0.004));   // rodapé de jacarandá
  for (const [y, r, col] of [[-6, 6, C.brass], [351, 9, hex(0xc99a61)], [376, 4, hex(0x9b6b4c)], [532, 10, C.gold]]) { const t = new THREE.Mesh(new THREE.TorusGeometry(u(HALL - 10), u(r), 8, 200), flat(col)); t.rotation.x = Math.PI / 2; t.position.y = u(y); g.add(t); }
  // cortina: um anel inteiro de pregas, e o lambrequim de meias-luas por cima (franja de ouro na barra)
  { const cortina = pleated({ cyl: [HALL - 4, 0, Math.PI * 2], y0: 30, y1: 338, folds: BAYS * 4, depth: 12, col: C.drape, nSeg: BAYS * 4 * 10 }); cortina.userData.anim = { k: 'sway', a: 0.035, hang: true }; g.add(cortina); }
  g.add(pleated({ cyl: [HALL - 20, 0, Math.PI * 2], y0: 300, y1: 346, folds: BAYS, depth: 5, col: C.valance, scallop: 24, nSeg: BAYS * 24 }));
  { const pts = []; for (let i = 0; i <= BAYS * 24; i++) { const a = i / (BAYS * 24) * Math.PI * 2, t = (i / 24) % 1, y = 300 + 24 * Math.abs(Math.sin(t * Math.PI)); pts.push(P(a, HALL - 23, y)); }
    g.add(new THREE.Mesh(new THREE.TubeGeometry(new THREE.CatmullRomCurve3(pts, true), BAYS * 24, u(3.4), 6, true), flat(C.fringe))); }
  // vitrais: oito janelas (uma a cada três vãos), em arco, quatro estações; a luz deles no assoalho
  const spots = [];
  for (let k = 0; k < 8; k++) {
    const a = (k * 3 + 1.5) * step, col = SEASONS[k % 4], W = 2 * Math.sin(step * 0.62 / 2) * HALL;
    const sh = new THREE.Shape(); sh.moveTo(u(-W / 2), u(142)); sh.lineTo(u(W / 2), u(142)); sh.lineTo(u(W / 2), u(310)); sh.absarc(0, u(310), u(W / 2), 0, Math.PI, false); sh.closePath();
    const fr = new THREE.Group(); fr.position.copy(P(a, HALL - 30, 0)); fr.rotation.y = a + Math.PI; g.add(fr);
    // vidro em losangos: cor da estação em dois tons e o chumbo, pintados num canvas e acesos (não recebem sombra)
    const c = document.createElement('canvas'); c.width = 128; c.height = 256; const cg = c.getContext('2d');
    cg.fillStyle = col; cg.fillRect(0, 0, 128, 256);
    for (let j = 0; j < 12; j++) for (let i = 0; i < 6; i++) { if ((i + j) % 3 === 0) { cg.fillStyle = 'rgba(255,255,255,0.35)'; cg.fillRect(i * 22, j * 22, 22, 22); } }
    cg.strokeStyle = '#4e3346'; cg.lineWidth = 3; for (let d = -256; d < 256; d += 26) { cg.beginPath(); cg.moveTo(d, 0); cg.lineTo(d + 256, 256); cg.moveTo(d + 256, 0); cg.lineTo(d, 256); cg.stroke(); }
    cg.lineWidth = 7; cg.beginPath(); cg.moveTo(64, 0); cg.lineTo(64, 256); cg.moveTo(0, 150); cg.lineTo(128, 150); cg.stroke();
    const tx = new THREE.CanvasTexture(c); tx.colorSpace = THREE.SRGBColorSpace;
    const sg = new THREE.ShapeGeometry(sh, 16), p = sg.attributes.position, uv = sg.attributes.uv;
    for (let i = 0; i < p.count; i++) uv.setXY(i, (p.getX(i) / u(W) + 0.5), (p.getY(i) - u(142)) / u(310 + W / 2 - 142));
    const glass = new THREE.Mesh(sg, new THREE.MeshBasicMaterial({ map: tx, side: THREE.DoubleSide })); glass.position.z = u(-1); fr.add(glass);
    // moldura dourada em arco
    const hole = new THREE.Path(); hole.moveTo(u(-W / 2), u(142)); hole.lineTo(u(W / 2), u(142)); hole.lineTo(u(W / 2), u(310)); hole.absarc(0, u(310), u(W / 2), 0, Math.PI, false); hole.closePath();
    const out = new THREE.Shape(); out.moveTo(u(-W / 2 - 12), u(130)); out.lineTo(u(W / 2 + 12), u(130)); out.lineTo(u(W / 2 + 12), u(310)); out.absarc(0, u(310), u(W / 2 + 12), 0, Math.PI, false); out.closePath(); out.holes.push(hole);
    const fm = new THREE.Mesh(new THREE.ExtrudeGeometry(out, { depth: u(6), bevelEnabled: false, curveSegments: 16 }), flat(C.gold)); fm.position.z = u(-2); fr.add(ol(fm, 0.005));
    spots.push([a, col]);
  }
  // manchas de luz colorida no assoalho (entre a arquibancada e a parede, sob o balcão), um trapézio suave e o losango
  { const mk = (pts, col, op) => { const sh = new THREE.Shape(pts.map(v => new THREE.Vector2(v.x, -v.z))); const sg = new THREE.ShapeGeometry(sh); sg.rotateX(-Math.PI / 2);
      const m = new THREE.Mesh(sg, new THREE.MeshBasicMaterial({ color: col, transparent: true, opacity: op, depthWrite: false, blending: THREE.AdditiveBlending })); m.position.y = u(FLOOR + 0.6); return m; };
    for (const [a, col] of spots) { const wI = step * 0.28, wO = step * 0.42;
      g.add(mk([P(a - wI, 770, 0), P(a + wI, 770, 0), P(a + wO, 1180, 0), P(a - wO, 1180, 0)], col, 0.22));
      g.add(mk([P(a, 820, 0), P(a + wI * 0.55, 960, 0), P(a, 1110, 0), P(a - wI * 0.55, 960, 0)], col, 0.3)); } }
  // colunata dourada canelada: plinto escuro, fuste com caneluras, capitel em três anéis
  { const fust = new THREE.CylinderGeometry(u(17), u(19), u(450), 32, 1); const p = fust.attributes.position;
    for (let i = 0; i < p.count; i++) { const x = p.getX(i), z = p.getZ(i), a = Math.atan2(z, x), k = 1 - 0.09 * Math.max(0, Math.cos(a * 20)); p.setX(i, x * k); p.setZ(i, z * k); } fust.computeVertexNormals();
    const fm = [], pl = [], c1 = [], c2 = [];
    for (let i = 0; i < 12; i++) { const a = (i + 0.5) / 12 * Math.PI * 2, x = Math.sin(a) * 1240, z = Math.cos(a) * 1240;
      fm.push(mat4(x, 20 + 225, z)); pl.push(mat4(x, FLOOR + 29, z)); c1.push(mat4(x, 474, z)); c2.push(mat4(x, 496, z)); }
    g.add(instanced(fust, flat(C.column), fm, 0.006), instanced(new RoundedBoxGeometry(u(54), u(58), u(54), 2, u(4)), flat(C.columnDark), pl, 0.006),
      instanced(new RoundedBoxGeometry(u(50), u(12), u(50), 2, u(3)), flat(hex(0xd7ad70)), c1, 0.005), instanced(new RoundedBoxGeometry(u(62), u(12), u(62), 2, u(3)), flat(C.goldLight), c2, 0.005)); }
  // quatro órgãos de tubos: console de jacarandá com frisos de latão, sete tubos (o do meio mais alto) com a boca
  for (let k = 0; k < 4; k++) {
    const a = k * Math.PI / 2, og = new THREE.Group(); og.position.copy(P(a, 1225, 0)); og.rotation.y = a + Math.PI; g.add(og);
    og.add(ol(box(250, 128, 60, 0, FLOOR + 64, 0, C.organWood, 3), 0.007));
    for (const y of [15, 85]) og.add(box(240, 5, 4, 0, y, 31, y > 50 ? hex(0xdfba7c) : C.brass, 1));
    og.add(ol(box(270, 10, 70, 0, 92, 0, C.brass, 2), 0.005));
    for (let p = -3; p <= 3; p++) {
      const h = 155 + (3 - Math.abs(p)) * 26, x = p * 30, col = p % 2 === 0 ? C.pipeA : C.pipeB;
      const pipe = new THREE.Mesh(new THREE.CylinderGeometry(u(10), u(10), u(h), 18), flat(col)); pipe.position.set(u(x), u(97 + 20 + h / 2), 0); og.add(ol(pipe, 0.005));
      const foot = new THREE.Mesh(new THREE.CylinderGeometry(u(10), u(4), u(20), 18), flat(hex(0xd9a66f))); foot.position.set(u(x), u(107), 0); og.add(ol(foot, 0.004));
      og.add(box(14, 7, 3, x, 147, 10, hex(0x221320), 1));   // a boca (lábio) do tubo
    }
  }
  // cúpula: forro em cone raso escuro com os nervos de ouro e o lustre de velas no meio
  { const dome = new THREE.Mesh(new THREE.ConeGeometry(u(HALL), u(620), 64, 1, true), flat(C.ceiling)); dome.material.side = THREE.BackSide; dome.position.y = u(540 + 310); g.add(dome);
    for (let i = 0; i < 12; i++) { const a = i / 12 * Math.PI * 2, A = P(a, HALL - 20, 545), B = new THREE.Vector3(0, u(1150), 0), len = A.distanceTo(B);
      const r = new THREE.Mesh(new THREE.CylinderGeometry(u(5), u(5), len, 6), flat(C.goldDark)); r.position.copy(A).add(B).multiplyScalar(0.5); r.quaternion.setFromUnitVectors(new THREE.Vector3(0, 1, 0), B.clone().sub(A).normalize()); g.add(r); }
    const ch = new THREE.Group(); ch.position.y = u(820); g.add(ch);
    const rod = new THREE.Mesh(new THREE.CylinderGeometry(u(3), u(3), u(330), 6), flat(C.goldDark)); rod.position.y = u(165); ch.add(rod);
    for (const [r, y] of [[150, 0], [95, 60]]) { const t = new THREE.Mesh(new THREE.TorusGeometry(u(r), u(6), 8, 48), flat(C.gold)); t.rotation.x = Math.PI / 2; t.position.y = u(y); ch.add(ol(t, 0.005));
      const n = Math.round(r / 12), cm = new THREE.MeshBasicMaterial({ color: '#ffe2a8' });
      for (let i = 0; i < n; i++) { const a = i / n * Math.PI * 2, cd = new THREE.Mesh(new THREE.CylinderGeometry(u(3), u(3), u(18), 6), flat('#f4ead6')); cd.position.set(u(Math.sin(a) * r), u(y + 12), u(Math.cos(a) * r)); ch.add(cd);
        const fl = new THREE.Mesh(new THREE.SphereGeometry(u(4.5), 8, 6), cm); fl.scale.y = 1.6; fl.position.set(u(Math.sin(a) * r), u(y + 26), u(Math.cos(a) * r)); ch.add(fl); } }
    const drop = new THREE.Mesh(new THREE.OctahedronGeometry(u(26), 0), new THREE.MeshBasicMaterial({ color: '#f7e6c4' })); drop.position.y = u(-40); ch.add(ol(drop, 0.004)); }
  return g;
}

export async function buildVivaldiArena() {
  const all = new THREE.Group();
  all.add(tablado(), assoalho(), arquibancadas(), balcao(), sala());
  // sala de concerto: luz de palco quente vindo do alto da frente, sombra lilás-vinho, "céu" ameixa (só aparece pelas
  // frestas: a cúpula fecha a sala); sem disco de sol
  all.userData.look = { indoor: true, tint: [0.9, 0.8, 1.08], horizon: '#513048', zenith: '#26162f', fog: [60, 200],
    sun: { color: '#ffe0b0', int: 1.9, pos: [0.8, 3.2, 2.6] }, hemi: { sky: '#f0c8d8', ground: '#4a2440', int: 1.25 } };
  return all;
}
