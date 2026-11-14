// Fundo de Pernambuco (arena da Valéria), em código e no acabamento do vinil, seguindo o concept
// arena-concepts/01-valeria-pernambuco.png: atrás do parapeito a ponte de arcos de pedra; depois o morro em terraços
// amarelos com casinhas e verde, a escadaria subindo até a igreja de duas torres (frontão curvo, cruz, porta turquesa);
// coqueiros no morro e nas laterais; plantas de folha larga em vasos de barro; bandeirolas cruzando por cima; à direita
// o mar com a cidade ao longe e as montanhas azuladas no horizonte. Medidas em unidades do jogo (UNIT m cada).
import { THREE, flat, vcol, withOutline } from './core.js';
const vcolMat = () => vcol();
import { RoundedBoxGeometry } from 'three/addons/geometries/RoundedBoxGeometry.js';
import { UNIT } from './arena_pernambuco.js';
import { casa } from './arena_pernambuco_props.js';

const u = v => v * UNIT;
const APRON = 35;
const C = { white: '#f8f1e2', gold: '#e9b23c', goldDark: '#c48a22', door: '#1aa39a', dark: '#2a3b55', stone: '#cbb89a', stoneDark: '#ad9878',
            terrace: '#e9b44c', terraceTop: '#d9a03e', green: '#4f9a3c', greenLight: '#78b84a', trunk: '#8a5a32', trunkDark: '#6b4224', coconut: '#7a8a2a',
            pot: '#c9652e', potBand: '#1fa7a0', leaf: '#4c9c3a', leafLight: '#6cba4a', sea: '#2f8fd0', seaDark: '#2a7cba', mountain: '#7f9cbc', mountainFar: '#a9bdd4',
            city: '#e8dcc8', string: '#5a3418', flags: ['#1fb0c8', '#e2342a', '#f7c22c', '#f08a3c', '#f7ecd2', '#2aa8a0'] };

const ol = (m, w = 0.007) => { const g = withOutline(m, w); g.position.copy(m.position); g.quaternion.copy(m.quaternion); g.scale.copy(m.scale); m.position.set(0, 0, 0); m.quaternion.identity(); m.scale.set(1, 1, 1); return g; };
const box = (w, h, d, x, y, z, col, r = 1.5) => { const m = new THREE.Mesh(new RoundedBoxGeometry(u(w), u(h), u(d), 2, u(Math.min(r, w / 2 - 0.01, h / 2 - 0.01, d / 2 - 0.01))), flat(col)); m.position.set(u(x), u(y), u(z)); return m; };
const extr = (shape, depth, col) => new THREE.Mesh(new THREE.ExtrudeGeometry(shape, { depth: u(depth), bevelEnabled: false, curveSegments: 16 }), flat(col));
const archHole = (cx, w, h) => { const p = new THREE.Path(); p.moveTo(u(cx - w / 2), u(0)); p.lineTo(u(cx + w / 2), 0); p.lineTo(u(cx + w / 2), u(h)); p.absarc(u(cx), u(h), u(w / 2), 0, Math.PI, false); p.lineTo(u(cx - w / 2), 0); return p; };

// --- igreja: nave branca com frontão barroco curvo e cruz, duas torres com sineira e cúpula, molduras douradas,
// janelas e porta turquesa
export function igreja() {
  const g = new THREE.Group(), NW = 220, NH = 260, D = 320, TW = 100, TH = 380;
  g.add(ol(box(NW, NH, D, 0, NH / 2, 0, C.white, 3), 0.009));
  // frontão: contorno curvo (volutas) por cima da nave
  { const s = new THREE.Shape(), w = NW / 2;
    s.moveTo(u(-w), 0); s.lineTo(u(-w), u(30)); s.quadraticCurveTo(u(-w * 0.55), u(30), u(-w * 0.45), u(70)); s.quadraticCurveTo(u(-w * 0.3), u(120), 0, u(128));
    s.quadraticCurveTo(u(w * 0.3), u(120), u(w * 0.45), u(70)); s.quadraticCurveTo(u(w * 0.55), u(30), u(w), u(30)); s.lineTo(u(w), 0); s.closePath();
    const f = extr(s, 24, C.white); f.position.set(0, u(NH), u(D / 2 - 24)); g.add(ol(f, 0.008));
    const tr = extr(s, 4, C.gold); tr.scale.set(1.06, 1.06, 1); tr.position.set(0, u(NH - 4), u(D / 2 - 26)); g.add(tr);   // moldura dourada atrás
    const ring = new THREE.Mesh(new THREE.TorusGeometry(u(18), u(4), 8, 24), flat(C.gold)); ring.position.set(0, u(NH + 66), u(D / 2 + 2)); g.add(ol(ring, 0.004));
    g.add(ol(box(10, 50, 10, 0, NH + 150, D / 2 - 12, C.gold, 2), 0.004)); g.add(ol(box(34, 10, 10, 0, NH + 158, D / 2 - 12, C.gold, 2), 0.004)); }
  // molduras douradas da fachada (cunhais e friso), janelas, óculo e porta
  for (const x of [-NW / 2 + 6, NW / 2 - 6]) g.add(ol(box(14, NH, 8, x, NH / 2, D / 2 + 2, C.gold, 2), 0.004));
  g.add(ol(box(NW + 8, 12, 10, 0, NH - 6, D / 2 + 3, C.gold, 2), 0.004));
  for (const x of [-56, 56]) { g.add(ol(box(30, 56, 6, x, NH * 0.62, D / 2 + 2, C.gold, 3), 0.003)); g.add(box(20, 44, 6, x, NH * 0.62, D / 2 + 4, C.dark, 3)); }
  { const s = new THREE.Shape(); s.moveTo(u(-30), 0); s.lineTo(u(30), 0); s.lineTo(u(30), u(80)); s.absarc(0, u(80), u(30), 0, Math.PI, false); s.closePath();
    const fr = extr(s, 6, C.gold); fr.scale.set(1.25, 1.12, 1); fr.position.set(0, 0, u(D / 2)); g.add(ol(fr, 0.004));
    const dr = extr(s, 6, C.door); dr.position.set(0, 0, u(D / 2 + 3)); g.add(ol(dr, 0.004)); }
  // torres: corpo, sineira com arco escuro, cimalha, cúpula em gomo e pináculo
  for (const sx of [-1, 1]) {
    const x = sx * (NW / 2 + TW / 2 - 6);
    g.add(ol(box(TW, TH - 110, TW, x, (TH - 110) / 2, D / 2 - TW / 2, C.white, 3), 0.008));
    for (const xx of [x - TW / 2 + 6, x + TW / 2 - 6]) g.add(ol(box(12, TH - 110, 8, xx, (TH - 110) / 2, D / 2 + 2, C.gold, 2), 0.003));
    g.add(ol(box(TW + 10, 12, TW + 10, x, TH - 114, D / 2 - TW / 2, C.gold, 2), 0.004));
    g.add(ol(box(TW - 14, 80, TW - 14, x, TH - 70, D / 2 - TW / 2, C.white, 3), 0.007));
    const bs = new THREE.Shape(); bs.moveTo(u(-16), 0); bs.lineTo(u(16), 0); bs.lineTo(u(16), u(30)); bs.absarc(0, u(30), u(16), 0, Math.PI, false); bs.closePath();
    for (const [nx, nz] of [[0, 1], [1, 0], [-1, 0], [0, -1]]) { const b = extr(bs, 3, C.dark); b.position.set(u(x + nx * (TW - 14) / 2), u(TH - 100), u(D / 2 - TW / 2 + nz * (TW - 14) / 2)); b.rotation.y = Math.atan2(nx, nz); g.add(b); }
    g.add(ol(box(TW - 4, 10, TW - 4, x, TH - 26, D / 2 - TW / 2, C.gold, 2), 0.004));
    const dome = new THREE.Mesh(new THREE.SphereGeometry(u((TW - 20) / 2), 16, 10, 0, Math.PI * 2, 0, Math.PI / 2), flat(C.white)); dome.scale.y = 1.25; dome.position.set(u(x), u(TH - 21), u(D / 2 - TW / 2)); g.add(ol(dome, 0.006));
    const pin = new THREE.Mesh(new THREE.ConeGeometry(u(7), u(36), 8), flat(C.gold)); pin.position.set(u(x), u(TH + 46), u(D / 2 - TW / 2)); g.add(ol(pin, 0.004));
    for (const y of [TH * 0.25, TH * 0.48]) g.add(box(16, 30, 6, x, y, D / 2 + 3, C.dark, 3));
  }
  // telhado de barro da nave
  { const s = new THREE.Shape(); s.moveTo(u(-NW / 2 - 10), 0); s.lineTo(u(NW / 2 + 10), 0); s.lineTo(0, u(70)); s.closePath();
    const r = extr(s, D - 30, '#c95b2e'); r.position.set(0, u(NH), u(-D / 2)); g.add(ol(r, 0.006)); }
  return g;
}

// --- ponte de pedra: arcos repetidos, pilares, passeio e mureta por cima
export function ponte(n = 5, span = 150, low = [], vines = []) {
  // vão a vão: cada um com seu arco, fiadas de pedra, passeio e mureta. Os vãos em "low" são mais baixos e arruinados
  // (sem a mureta inteira, pedras caídas na frente): deixam a escadaria do morro aparecer por cima. "vines" = pilares
  // com buganvília subindo
  const g = new THREE.Group(), W = n * span, D = 70;
  const stoneMat = () => { const m = flat(C.stone);
    m.onBeforeCompile = sh => {   // fiadas de pedra
      sh.vertexShader = 'varying vec3 vP;' + String.fromCharCode(10) + sh.vertexShader.replace('#include <begin_vertex>', '#include <begin_vertex>' + String.fromCharCode(10) + 'vP = position / ' + UNIT.toFixed(6) + ';');
      sh.fragmentShader = 'varying vec3 vP;' + String.fromCharCode(10) + sh.fragmentShader.replace('#include <map_fragment>', '#include <map_fragment>' + String.fromCharCode(10) +
        '{ float row = floor(vP.y / 22.0); float x = vP.x / 40.0 + 0.5 * mod(row, 2.0); if (fract(vP.y / 22.0) < 0.08 || fract(x) < 0.05) diffuseColor.rgb *= 0.88; }');
    };
    m.customProgramCacheKey = () => 'pedra'; return m; };
  const mat = stoneMat();
  for (let i = 0; i < n; i++) {
    const cx = -W / 2 + span * (i + 0.5), isLow = low.includes(i), H = isLow ? 118 : 190, OW = span - 34, OH = isLow ? 34 : 80;
    const sh = new THREE.Shape(); sh.moveTo(u(cx - span / 2 - 0.5), 0); sh.lineTo(u(cx + span / 2 + 0.5), 0);
    if (isLow) {   // borda de cima quebrada
      sh.lineTo(u(cx + span / 2 + 0.5), u(H)); sh.lineTo(u(cx + span * 0.25), u(H - 14)); sh.lineTo(u(cx + span * 0.05), u(H + 6)); sh.lineTo(u(cx - span * 0.2), u(H - 18)); sh.lineTo(u(cx - span / 2 - 0.5), u(H - 4));
    } else { sh.lineTo(u(cx + span / 2 + 0.5), u(H)); sh.lineTo(u(cx - span / 2 - 0.5), u(H)); }
    sh.closePath(); sh.holes.push(archHole(cx, OW, OH));
    const wall = new THREE.Mesh(new THREE.ExtrudeGeometry(sh, { depth: u(D), bevelEnabled: false, curveSegments: 16 }), mat); wall.position.z = u(-D / 2); g.add(ol(wall, 0.008));
    if (!isLow) {
      g.add(ol(box(span + 1, 14, D + 14, cx, H + 7, 0, C.stoneDark, 2), 0.005));
      for (const z of [-D / 2 + 4, D / 2 - 4]) for (let x = cx - span / 2 + 15; x <= cx + span / 2 - 14; x += 30) g.add(ol(box(18, 26, 10, x, H + 27, z, C.stone, 2), 0.003));
    } else {       // pedras caídas no chão, na frente do vão
      for (const [dx, dz, s, r] of [[-30, 60, 22, 0.3], [18, 78, 16, -0.4], [44, 52, 13, 0.9], [-58, 90, 12, 0.2]]) { const b = box(s * 1.5, s, s * 1.2, cx + dx, s / 2, dz, C.stoneDark, 2); b.rotation.y = r; g.add(ol(b, 0.004)); }
    }
  }
  // buganvília: cachos rosa e folhas subindo pela frente de alguns pilares e caindo por cima da mureta
  for (const i of vines) {
    const px = -W / 2 + span * i;
    for (let k = 0; k < 16; k++) {
      const y = 10 + k * 13 + ((k * 7) % 5), x = px + Math.sin(k * 1.9) * 16, pink = k % 3 !== 0;
      const b = new THREE.Mesh(new THREE.IcosahedronGeometry(u(pink ? 11 : 9), 0), flat(pink ? (k % 2 ? '#e0479e' : '#c9327f') : C.green));
      b.position.set(u(x), u(y), u(D / 2 + 6)); g.add(ol(b, 0.003));
    }
  }
  return g;
}

// --- coqueiro: tronco curvo em anéis, copa de folhas compridas caindo, cocos
export function coqueiro(h = 300, lean = 0.25, seed = 0) {
  const g = new THREE.Group(), segs = 9;
  const pt = t => new THREE.Vector3(Math.sin(lean) * h * t * t * 0.9, h * t, 0);
  for (let i = 0; i < segs; i++) {
    const a = pt(i / segs), b = pt((i + 1) / segs), len = a.distanceTo(b);
    const m = new THREE.Mesh(new THREE.CylinderGeometry(u(11 - i * 0.6), u(12 - i * 0.6), u(len + 2), 10), flat(i % 2 ? C.trunk : C.trunkDark));
    m.position.copy(a.clone().add(b).multiplyScalar(0.5 * UNIT)); m.quaternion.setFromUnitVectors(new THREE.Vector3(0, 1, 0), b.clone().sub(a).normalize()); g.add(ol(m, 0.004));
  }
  const top = pt(1);
  for (let k = 0; k < 9; k++) {   // folha: forma de pena extrudada, curvada para baixo
    const a = (k / 9) * Math.PI * 2 + seed, L = 120 + (k % 3) * 18;
    const s = new THREE.Shape(); s.moveTo(0, 0); s.quadraticCurveTo(u(L * 0.5), u(26), u(L), 0); s.quadraticCurveTo(u(L * 0.5), u(-10), 0, 0);
    const geo = new THREE.ExtrudeGeometry(s, { depth: u(2), bevelEnabled: false, curveSegments: 10 }); geo.rotateX(-Math.PI / 2);   // folha deitada, ao longo de +x
    const p = geo.attributes.position;
    for (let i = 0; i < p.count; i++) { const x = p.getX(i) / u(L); p.setY(i, p.getY(i) + u(L) * (0.25 * x - 0.75 * x * x)); }   // sobe um pouco e cai na ponta
    geo.computeVertexNormals();
    const lf = new THREE.Mesh(geo, flat(k % 2 ? C.leaf : C.leafLight)); lf.userData.anim = { k: 'sway', a: 0.035, rad: true }; lf.position.set(u(top.x), u(top.y), u(top.z)); lf.rotation.y = a;
    g.add(ol(lf, 0.004));
  }
  for (let k = 0; k < 3; k++) { const c = new THREE.Mesh(new THREE.SphereGeometry(u(9), 10, 8), flat(C.coconut)); const a = k * 2.1 + seed; c.position.set(u(top.x + Math.cos(a) * 10), u(top.y - 10), u(top.z + Math.sin(a) * 10)); g.add(ol(c, 0.003)); }
  return g;
}

// --- canteiro de coqueiro no calçamento: anel baixo de pedra com areia dentro (o tronco sai da areia)
export function plantado(palm) {
  const ring = new THREE.Mesh(new THREE.CylinderGeometry(u(46), u(50), u(16), 20), flat('#c9a77c')); ring.position.y = u(8); palm.add(ol(ring, 0.005));
  const sand = new THREE.Mesh(new THREE.CylinderGeometry(u(38), u(38), u(2), 20), flat(C.terrace)); sand.position.y = u(15.5); palm.add(sand);
  return palm;
}

// --- planta em vaso: vaso de barro com faixa turquesa, folhas largas saindo em leque
export function plantaVaso(s = 1, seed = 0) {
  const g = new THREE.Group();
  const prof = [[0, 0], [26, 0], [30, 8], [36, 44], [40, 50], [40, 58], [34, 58]].map(([r, y]) => new THREE.Vector2(u(r * s), u(y * s)));
  g.add(ol(new THREE.Mesh(new THREE.LatheGeometry(prof, 8), flat(C.pot)), 0.006));
  const band = new THREE.Mesh(new THREE.CylinderGeometry(u(41 * s), u(41 * s), u(9 * s), 8), flat(C.potBand)); band.position.y = u(52 * s); g.add(ol(band, 0.004));
  for (let k = 0; k < 9; k++) {
    const a = (k / 9) * Math.PI * 2 + seed, L = (70 + (k % 3) * 22) * s, up = 0.6 + (k % 2) * 0.35;
    const sh = new THREE.Shape(); sh.moveTo(0, 0); sh.quadraticCurveTo(u(L * 0.45), u(L * 0.22), u(L), 0); sh.quadraticCurveTo(u(L * 0.45), u(-L * 0.22), 0, 0);
    const geo = new THREE.ExtrudeGeometry(sh, { depth: u(2), bevelEnabled: false, curveSegments: 10 }); geo.rotateX(-Math.PI / 2);
    const p = geo.attributes.position;
    for (let i = 0; i < p.count; i++) { const x = p.getX(i) / u(L); p.setY(i, p.getY(i) + u(L) * up * (1.1 * x - 0.8 * x * x)); }   // folha sobe em arco e pende na ponta
    geo.computeVertexNormals();
    const lf = new THREE.Mesh(geo, flat(k % 2 ? C.leaf : C.leafLight)); lf.userData.anim = { k: 'sway', a: 0.05, rad: true }; lf.position.y = u(56 * s); lf.rotation.y = a; g.add(ol(lf, 0.004));
  }
  return g;
}

// --- bandeirolas: linha em catenária entre dois pontos, bandeirinhas de ponta recortada penduradas
export function bandeirolas(a, b, sag = 60, n = 18) {
  const g = new THREE.Group(), A = new THREE.Vector3(...a), B = new THREE.Vector3(...b);
  const at = t => A.clone().lerp(B, t).add(new THREE.Vector3(0, -sag * 4 * t * (1 - t), 0));
  const pts = []; for (let i = 0; i <= 40; i++) pts.push(at(i / 40).multiplyScalar(UNIT));
  g.add(new THREE.Mesh(new THREE.TubeGeometry(new THREE.CatmullRomCurve3(pts), 60, u(1), 5), flat(C.string)));
  const dir = B.clone().sub(A).setY(0).normalize(), ry = Math.atan2(-dir.z, dir.x);
  const fs = new THREE.Shape(); fs.moveTo(u(-9), 0); fs.lineTo(u(9), 0); fs.lineTo(u(9), u(-26)); fs.lineTo(0, u(-18)); fs.lineTo(u(-9), u(-26)); fs.closePath();
  const fg = new THREE.ExtrudeGeometry(fs, { depth: u(0.8), bevelEnabled: false });
  for (let i = 1; i < n; i++) {
    const t = i / n, p = at(t), f = new THREE.Mesh(fg, flat(C.flags[i % C.flags.length])); f.material.side = THREE.DoubleSide; f.userData.anim = { k: 'flag', a: 0.05 };
    f.position.copy(p.multiplyScalar(UNIT)); f.rotation.y = ry; f.rotation.x = 0.08 * Math.sin(i * 1.7); g.add(ol(f, 0.003));
  }
  return g;
}

// --- montanha ao longe: pico baixo facetado
const montanha = (w, h, col) => { const g = new THREE.IcosahedronGeometry(1, 1); g.scale(u(w), u(h), u(w * 0.55)); const m = new THREE.Mesh(g, flat(col)); return m; };   // morro arredondado (metade afundada no mar)

export function buildPernambucoBackground() {
  const all = new THREE.Group(); all.position.y = u(-APRON);
  const put = (o, x, y, z, ry = 0, s = 1) => { o.position.set(u(x), u(y), u(z)); o.rotation.y = ry; o.scale.multiplyScalar(s); all.add(o); return o; };
  // mar em volta de tudo (o calçamento é uma ilha), mais escuro ao longe
  { const sea = new THREE.Mesh(new THREE.PlaneGeometry(u(14000), u(14000)), flat(C.sea)); sea.rotation.x = -Math.PI / 2; sea.position.y = u(-18); all.add(sea); }
  // borda da praça (x ±1400, z -760..1500): mureta de pedra baixa e, por fora, a praia de contorno irregular que desce
  // até a água em volta de toda a ilha
  { const X0 = -1400, X1 = 1400, Z0 = -760, Z1 = 1500, CZ = (Z0 + Z1) / 2;
    for (const [x, z, w, d] of [[0, Z1, X1 - X0 + 20, 16], [X0, CZ, 16, Z1 - Z0], [X1, CZ, 16, Z1 - Z0]]) all.add(ol(box(w, 16, d, x, 6, z, '#c9a77c', 3), 0.004));
    const toRect = th => { const c = Math.cos(th), s = Math.sin(th), tx = c ? (c > 0 ? X1 : X0) / c : 1e9, tz = s ? (s > 0 ? Z1 - CZ : Z0 - CZ) / s : 1e9; return Math.min(Math.abs(tx), Math.abs(tz)); };
    const NA = 220, pos = [], idx = [], ca = [], cIn = new THREE.Color(C.terrace), cOut = new THREE.Color(C.terrace), cWet = new THREE.Color(C.terrace);   // mesmo tom da terra do morro
    const rings = [[0, -1, cIn], [0.35, -6, cIn], [0.75, -14, cOut], [1, -24, cWet]];
    for (let j = 0; j <= NA; j++) {
      const th = j / NA * Math.PI * 2, r0 = toRect(th) - 30, wide = 700 + 260 * Math.sin(th * 3 + 0.7) * Math.sin(th * 5 + 1.9) + 120 * Math.sin(th * 11);   // faixa larga de areia
      for (const [f, y, c] of rings) { const r = r0 + Math.max(380, wide) * f; pos.push(u(Math.cos(th) * r), u(y), u(CZ + Math.sin(th) * r)); ca.push(c.r, c.g, c.b); }
    }
    const R = rings.length;
    for (let j = 0; j < NA; j++) for (let k = 0; k < R - 1; k++) { const a = j * R + k, b = a + 1, c = a + R, d = c + 1; idx.push(a, b, c, b, d, c); }
    const g = new THREE.BufferGeometry(); g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3)); g.setAttribute('color', new THREE.Float32BufferAttribute(ca, 3)); g.setIndex(idx); g.computeVertexNormals();
    const beach = new THREE.Mesh(g, vcol()); beach.material.side = THREE.DoubleSide; all.add(beach); }
  // ponte atravessando o cenário de um lado ao outro, atrás do parapeito; à direita, o cais com casas atrás dela
  put(ponte(25, 146, [9, 10], [4, 7, 18, 21]), 0, -6, -720);   // vãos 9 e 10 baixos: a escadaria aparece por cima; buganvílias longe do centro (zona calma atrás dos lutadores)   // afundada 6: a base não coincide com o chão (sem piscar)
  { const R = 830;
    put(ol(box(520, 40, 520, 0, 0, 0, '#cfae86', 3), 0.007), R + 230, -18, -990);                           // cais (direita)
    for (const [c, x, z, ry] of [['#e88aa0', R + 120, -930, -0.25], ['#5aa0d8', R + 330, -1010, -0.35], ['#f2b632', R + 200, -1150, -0.15]]) put(casa(c, 190, 300), x, 2, z, ry, 0.9);
    put(plantado(coqueiro(300, -0.25, 9)), R + 430, 2, -850);
  }
  // morro da igreja na lateral esquerda, girado para o centro (a igreja aparece em três-quartos, não de frente):
  // relevo contínuo (sem degraus de terraplanagem) com o topo aplanado para a igreja; casas, escadaria, verde e
  // coqueiros pousam na altura do terreno. Montado num grupo próprio (frente = +z)
  const hill = new THREE.Group();
  const PEAK = [0, -600], TOP = 420;
  const hash = (x, z) => { const s = Math.sin(x * 12.9898 + z * 78.233) * 43758.5453; return s - Math.floor(s); };
  const bump = (x, z) => 22 * Math.sin(x * 0.011 + 1.3) * Math.sin(z * 0.013 + 0.4) + 14 * Math.sin(x * 0.027 - z * 0.019) + 8 * Math.sin(x * 0.05 + z * 0.043);
  const dist = (x, z) => { const dx = (x - PEAK[0]) / 1150, dz = (z - PEAK[1]) / (z > PEAK[1] ? 780 : 900); return Math.hypot(dx, dz); };   // encosta larga e suave
  const H = (x, z) => {   // altura do terreno (unidades): planalto no alto, encosta suave, pé do morro na água
    const d = dist(x, z), t = Math.min(1, Math.max(0, (d - 0.24) / 0.76)), s = t * t * (3 - 2 * t);
    // costa ondulada (como a praia da praça): perto dela o terreno vira uma praia rasa; depois afunda de vez no mar
    const th = Math.atan2(z - PEAK[1], x - PEAK[0]), shore = 0.98 + 0.07 * Math.sin(th * 3 + 0.7) * Math.sin(th * 5 + 1.9) + 0.035 * Math.sin(th * 11);
    const base = TOP * (1 - s) - 260 * Math.max(0, d - shore);
    return base + bump(x, z) * Math.min(1, Math.max(0, (d - 0.32) * 3)) * (1 - 0.6 * s);
  };
  { const W = 2900, D = 2300, NX = 128, NZ = 100, g = new THREE.PlaneGeometry(u(W), u(D), NX, NZ); g.rotateX(-Math.PI / 2); g.translate(0, 0, u(PEAK[1] + 80));
    const p = g.attributes.position, ca = [], cG = new THREE.Color(C.green), cL = new THREE.Color(C.greenLight), cE = new THREE.Color(C.terrace), cT = new THREE.Color('#efe4cf'), c = new THREE.Color();
    for (let i = 0; i < p.count; i++) {
      const x = p.getX(i) / UNIT, z = p.getZ(i) / UNIT, h = H(x, z); p.setY(i, u(h));
      const d = dist(x, z), n = Math.sin(x * 0.009 + 2.1) * Math.sin(z * 0.011) + 0.6 * Math.sin(x * 0.023 + z * 0.017);
      c.copy(d < 0.24 ? cT : cE);                          // terra lisa; o adro da igreja mais claro (o verde vem das moitas)
      ca.push(c.r, c.g, c.b);
    }
    g.setAttribute('color', new THREE.Float32BufferAttribute(ca, 3)); g.computeVertexNormals();
    const tm = new THREE.Mesh(g, vcolMat());   // malha soldada: a casca do contorno não abre rasgos
    hill.add(ol(tm, 0.008)); }
  // casa assentada no terreno: base amarela do ponto mais baixo do lote até a casa (sem casa flutuando nem enterrada)
  const onGround = (obj, x, z, w, d, ry = 0, s = 1) => {
    const hs = [[-w / 2, -d / 2], [w / 2, -d / 2], [w / 2, d / 2], [-w / 2, d / 2]].map(([a, b]) => H(x + a * s, z + b * s));
    const top = Math.max(...hs), low = Math.min(...hs);
    obj.position.set(u(x), u(top), u(z)); obj.rotation.y = ry; obj.scale.setScalar(s); hill.add(obj);
    if (top - low > 4) hill.add(ol(box((w + 16) * s, top - low + 20, (d + 16) * s, x, (top + low) / 2 - 10, z, C.terrace, 3), 0.005));
  };
  for (const [c, x, z, ry] of [['#f2b632', -420, 20, 0.15], ['#e88aa0', 330, -10, -0.2], ['#5aa0d8', 520, -120, -0.35], ['#9fd06a', -330, -260, 0.25], ['#3bb0a6', 300, -300, -0.15], ['#f08a3c', -560, -180, 0.4]])
    onGround(casa(c, 150, 220), x, z, 150, 180, ry, 0.8);
  // escadaria: degraus seguindo a encosta, do pé do morro até o adro
  for (let z = 120; z > -380; z -= 30) { const y = H(0, z); hill.add(ol(box(120, 46, 32, 0, y - 20, z, Math.round(z / 30) % 2 ? '#efe4cf' : '#e2d4b8', 2), 0.004)); }
  { const ig = igreja(); ig.position.set(0, u(H(0, -660) - 6), u(-660)); hill.add(ig); }
  // verde: moitas e arbustos espalhados pela encosta (fora da escada e do adro)
  for (let k = 0; k < 70; k++) {
    const x = (hash(k, 1) - 0.5) * 2000, z = PEAK[1] + 80 + (hash(k, 2) - 0.5) * 1500, d = dist(x, z);
    if (d < 0.3 || d > 1.0 || Math.abs(x) < 90) continue;
    const r = 20 + hash(k, 3) * 26, b = new THREE.Mesh(new THREE.IcosahedronGeometry(u(r), 0), flat(k % 2 ? C.green : C.greenLight));
    b.position.set(u(x), u(H(x, z) + r * 0.45), u(z)); b.scale.y = 0.8; hill.add(ol(b, 0.004));
  }
  const hillPalms = [];
  [[-470, -60, 0.2], [-220, -360, -0.25], [260, -470, 0.3], [-280, -520, 0.15], [470, -60, -0.2]].forEach(([x, z, l], i) => { const h = 260 + (i % 2) * 60, p = coqueiro(h, l, i); p.position.set(u(x), u(H(x, z) - 4), u(z)); p.rotation.y = i; hill.add(p); hillPalms.push({ p, h, l }); });
  put(hill, -400, 0, -1120, 0.38);
  // ponto de amarrar no tronco de um coqueiro do morro (a 80% da altura), em unidades no espaço do cenário
  all.updateMatrixWorld(true);
  const tie = i => { const { p, h, l } = hillPalms[i], t = 0.8, w = p.localToWorld(new THREE.Vector3(u(Math.sin(l) * h * t * t * 0.9), u(h * t), 0)); return all.worldToLocal(w).divideScalar(UNIT).toArray(); };
  // coqueiros nas laterais da praça
  [[700, 0, -980, -0.2]].forEach(([x, y, z, l], i) => put(coqueiro(300, l, i + 7), x, y, z, i));
  [[-980, 0, 420, 0.3], [990, 0, 380, -0.3], [-1050, 0, -700, 0.2], [1060, 0, -760, -0.25]].forEach(([x, y, z, l], i) => put(plantado(coqueiro(320, l, i + 3)), x, y, z, i * 2));
  // lado da câmera: coqueiros e vasos na frente das arcadas
  [[-700, 640, 0.25], [700, 640, -0.25], [-260, 720, 0.15], [300, 730, -0.2]].forEach(([x, z, l], i) => put(plantado(coqueiro(300 + (i % 2) * 40, l, i + 11)), x, 0, z, i));
  [[-520, 700], [-120, 720], [140, 720], [540, 700]].forEach(([x, z], i) => put(plantaVaso(1.1, i + 5), x, 0, z, i));
  // plantas em vaso nas laterais da praça, perto das arcadas
  [[-640, 260], [-640, -120], [-640, -520], [640, 260], [640, -120], [640, -520], [-380, 470], [380, 470]].forEach(([x, z], i) => put(plantaVaso(1 + (i % 3) * 0.15, i), x, 0, z, i));
  // cidade ao longe na beira do mar (à direita) e montanhas azuladas no horizonte
  for (let k = 0; k < 26; k++) { const w = 40 + (k * 37) % 50, h = 30 + (k * 53) % 90; put(box(w, h, 40, 0, 0, 0, k % 3 ? C.city : '#d9c7a8', 1), 900 + k * 70, h / 2 - 10, -2300 - (k % 3) * 40); }
  // montanhas em volta de todo o horizonte (duas fileiras: a de trás mais clara)
  for (let k = 0; k < 30; k++) {
    const far = k % 2, th = k / 30 * Math.PI * 2 + far * 0.1, r = (far ? 5600 : 4700) + ((k * 337) % 5) * 120;
    const w = 700 + ((k * 211) % 4) * 220, h = (far ? 520 : 380) + ((k * 97) % 5) * 110;
    const m = put(montanha(w, h, far ? C.mountainFar : C.mountain), Math.cos(th) * r, -18, 370 + Math.sin(th) * r); m.lookAt(0, m.position.y, u(370));
  }
  // bandeirolas: dos prédios da esquerda aos da direita, passando por cima do fundo, e algumas mais perto
  for (const [a, b, sag] of [[[-820, 420, -380], [820, 420, -380], 110], [[-820, 380, 60], [820, 380, 60], 90], [[-820, 400, -380], tie(3), 60], [[820, 400, -380], tie(2), 90]].slice(0, window.__ropeInfo ? 1 : 4)) all.add(bandeirolas(a, b, sag, 26));   // o jogo 3D só fica com o varal do fundo: os outros passam por cima do ringue (tools3d/export)
  // nuvens: tufos achatados de bolas brancas, altas e ao longe, espalhadas pelo céu
  for (let k = 0; k < 22; k++) {   // o céu todo, em volta da cena
    const cl = new THREE.Group(), n = 4 + (k % 3);
    for (let i = 0; i < n; i++) {
      const r = 70 + ((k * 7 + i * 13) % 5) * 18, b = new THREE.Mesh(new THREE.IcosahedronGeometry(u(r), 1), flat(i % 2 ? '#ffffff' : '#eef6fb'));
      b.position.set(u((i - (n - 1) / 2) * 95 + ((i * 31) % 40)), u(((i * 17) % 3) * 22), u((i % 2) * 40)); b.scale.y = 0.62; cl.add(ol(b, 0.006));
    }
    const ang = k / 22 * Math.PI * 2 + ((k * 0.37) % 0.2), dist = 3200 + (k % 4) * 450;
    cl.position.set(u(Math.sin(ang) * dist), u(1100 + ((k * 211) % 5) * 160), u(-Math.cos(ang) * dist)); cl.lookAt(0, cl.position.y, 0); cl.scale.setScalar(1 + (k % 3) * 0.35);
    all.add(cl);
  }
  return all;
}
