// Pátio da Lady Espinha, em código e no acabamento do vinil, pelo concept arena-concepts/07-lady-espinha-patio.png:
// pátio de adobe terracota; ringue de lona creme com a faixa verde-água e a saia verde-água com os losangos em degrau
// (padrão de tapete do deserto), postes quadrados vermelho e azul com os tampões, cordas creme; em volta muretas e
// floreiras de adobe em degraus, cactos saguaro e palmas floridas, vasos pintados; ao fundo o portal de adobe com o
// sino, os dois arcos dos lados, pedestais com os vasos de buganvília e o painel de azulejo sobre a escadaria;
// bandeirolas verde-água e creme, mesas de pedra vermelha no horizonte e céu azul. Unidades do jogo.
import { THREE, flat, withOutline } from './core.js';
import { RoundedBoxGeometry } from 'three/addons/geometries/RoundedBoxGeometry.js';
import { mergeGeometries } from 'three/addons/utils/BufferGeometryUtils.js';
import { UNIT } from './arena_pernambuco.js';

const u = v => v * UNIT;
const HALF = 270, DECK = 46, POST_TOP = 125, ROPES = [38, 68, 98];
const C = { adobe: '#d9814e', adobeL: '#e49762', adobeD: '#b9653a', teal: '#2fa3a0', tealD: '#1f8584', cream: '#efe2c8', red: '#d8342c', blue: '#2a56c6',
            cactus: '#5f9a3a', cactusD: '#4a7e2c', pad: '#6aa84a', flower: '#e8287a', bell: '#a8873a', mesa: '#b4544a', mesaL: '#c96a58', pot: '#c6683c' };
const ol = (m, w = 0.007) => { const g = withOutline(m, w); g.position.copy(m.position); g.quaternion.copy(m.quaternion); g.scale.copy(m.scale); m.position.set(0, 0, 0); m.quaternion.identity(); m.scale.set(1, 1, 1); return g; };
const box = (w, h, d, x, y, z, col, r = 3) => { const m = new THREE.Mesh(new RoundedBoxGeometry(u(w), u(h), u(d), 2, u(Math.min(r, w / 2 - 0.01, h / 2 - 0.01, d / 2 - 0.01))), typeof col === 'string' ? flat(col) : col); m.position.set(u(x), u(y), u(z)); return m; };
const cyl = (r0, r1, h, x, y, z, col, seg = 20) => { const m = new THREE.Mesh(new THREE.CylinderGeometry(u(r0), u(r1), u(h), seg), typeof col === 'string' ? flat(col) : col); m.position.set(u(x), u(y), u(z)); return m; };
const hash = n => { const v = Math.sin(n * 127.1 + 311.7) * 43758.5453; return v - Math.floor(v); };
const toonTex = tex => new THREE.MeshToonMaterial({ map: tex, gradientMap: flat('#fff').gradientMap });
function canvasTex(w, h, draw, rep) {
  const c = document.createElement('canvas'); c.width = w; c.height = h; draw(c.getContext('2d'), w, h);
  const t = new THREE.CanvasTexture(c); t.colorSpace = THREE.SRGBColorSpace; t.anisotropy = 8;
  if (rep) { t.wrapS = t.wrapT = THREE.RepeatWrapping; t.repeat.set(rep[0], rep[1]); } return t;
}
// losango em degraus (pixel) do tapete: creme por fora, terracota e laranja no miolo
function losango(c, cx, cy, p) {
  const ring = (r, col) => { c.fillStyle = col; for (let y = -r; y <= r; y++) { const w = r - Math.abs(y); c.fillRect(cx - (w + 0.5) * p, cy + (y - 0.5) * p, (2 * w + 1) * p, p); } };
  ring(4, C.cream); ring(3, C.teal); ring(2, C.cream); ring(1, C.adobe); ring(0, '#f0a060');
}

// ================= ringue: lona creme com a faixa verde-água, saia com os losangos, postes vermelho/azul, cordas
function ringue() {
  const g = new THREE.Group(), E = HALF + 30;
  const lona = canvasTex(512, 512, (c, w) => { const k = w / (2 * E); c.fillStyle = C.teal; c.fillRect(0, 0, w, w); c.fillStyle = C.cream; c.fillRect(30 * k, 30 * k, w - 60 * k, w - 60 * k);
    c.strokeStyle = 'rgba(170,140,100,0.25)'; c.lineWidth = 2; for (let i = 1; i < 6; i++) { c.beginPath(); c.moveTo(30 * k, i * w / 6); c.lineTo(w - 30 * k, i * w / 6); c.stroke(); } });
  const top = new THREE.Mesh(new THREE.BoxGeometry(u(2 * E), u(8), u(2 * E)), [flat(C.teal), flat(C.teal), toonTex(lona), flat(C.teal), flat(C.teal), flat(C.teal)]); top.position.y = u(-4); g.add(ol(top, 0.004));
  const saia = canvasTex(1024, 128, (c, w, h) => { c.fillStyle = C.teal; c.fillRect(0, 0, w, h); c.fillStyle = C.tealD; c.fillRect(0, 0, w, 10); c.fillRect(0, h - 10, w, 10);
    for (let i = 0; i < 6; i++) losango(c, (i + 0.5) * w / 6, h / 2, 10); });
  for (let s = 0; s < 4; s++) { const sk = new THREE.Mesh(new THREE.BoxGeometry(u(2 * E), u(DECK - 8), u(4)), [flat(C.tealD), flat(C.tealD), flat(C.tealD), flat(C.tealD), toonTex(saia), flat(C.tealD)]);
    const a = s * Math.PI / 2; sk.position.set(u(Math.sin(a) * (E - 2)), u(-8 - (DECK - 8) / 2), u(Math.cos(a) * (E - 2))); sk.rotation.y = a; g.add(ol(sk, 0.004)); }
  // postes: vermelho à esquerda, azul à direita (como no concept), com os tampões das cordas
  const corners = [[-HALF, HALF, C.red], [HALF, HALF, C.blue], [HALF, -HALF, C.blue], [-HALF, -HALF, C.red]];
  for (const [x, z, col] of corners) {
    g.add(ol(box(28, POST_TOP + 10, 28, x, (POST_TOP - 10) / 2, z, col, 4), 0.006));
    for (const h of ROPES) { const sx = -Math.sign(x), sz = -Math.sign(z);
      for (const [dx, dz] of [[sx, 0], [0, sz]]) { const p = box(dx ? 22 : 26, 26, dz ? 22 : 26, x + dx * 24, h, z + dz * 24, col, 5); g.add(ol(p, 0.004)); } } }
  for (let i = 0; i < 4; i++) { const [ax, az] = corners[i], [bx, bz] = corners[(i + 1) % 4];
    ROPES.forEach(h => { const a = new THREE.Vector3(u(ax), u(h), u(az)), b = new THREE.Vector3(u(bx), u(h), u(bz)), len = a.distanceTo(b);
      const cg = new THREE.CylinderGeometry(u(3.6), u(3.6), len, 10); cg.rotateZ(Math.PI / 2);
      const m = new THREE.Mesh(cg, flat(C.cream)); m.position.copy(a).add(b).multiplyScalar(0.5); m.rotation.y = -Math.atan2(b.z - a.z, b.x - a.x); g.add(ol(m, 0.004)); }); }
  return g;
}

// ================= cactos: saguaro (tronco e braços em cápsula, com as costelas no shader) e palma (raquetes) florida
// tubo de cacto com gomos (raio ondulando em volta) e um leve afinamento; cor por semente
function gomos(r, L, seed) { const g = new THREE.CapsuleGeometry(u(r), u(L), 8, 24), p = g.attributes.position;
  for (let i = 0; i < p.count; i++) { const a = Math.atan2(p.getZ(i), p.getX(i)), k = 1 + 0.09 * Math.cos(a * 10); p.setX(i, p.getX(i) * k); p.setZ(i, p.getZ(i) * k); }
  g.computeVertexNormals(); return g; }
const VERDES = ['#5f9a3a', '#4f8a3e', '#6aa24a', '#58904a', '#73a456'];
function saguaro(h, seed) {
  const g = new THREE.Group(), m = flat(VERDES[Math.floor(hash(seed) * 5)]), r = h * (0.075 + hash(seed + 9) * 0.03);
  const t = new THREE.Mesh(gomos(r, h - 2 * r, seed), m); t.position.y = u(h / 2); g.add(ol(t, 0.006));
  const nb = 1 + Math.floor(hash(seed + 4) * 3);   // 1 a 3 braços, alturas e lados variados
  for (let b = 0; b < nb; b++) { const s = b % 2 ? 1 : -1, y0 = 0.32 + hash(seed + b * 7) * 0.3, len = 0.18 + hash(seed + b * 3) * 0.22, ry = (hash(seed + b) - 0.5) * 1.2;
    const arm = new THREE.Group(); arm.rotation.y = ry; arm.position.y = u(h * y0); g.add(arm);
    const out = new THREE.Mesh(gomos(r * 0.65, r * 1.4, seed + b), m); out.rotation.z = Math.PI / 2; out.position.set(u(s * r * 1.5), 0, 0); arm.add(ol(out, 0.005));
    const upA = new THREE.Mesh(gomos(r * 0.65, h * len, seed + b), m); upA.position.set(u(s * r * 2.5), u(h * len / 2), 0); arm.add(ol(upA, 0.005));
    if (hash(seed + b * 11) < 0.6) arm.add(flor(s * r * 2.5, h * len + r * 0.6, 0, 1.2)); }
  if (hash(seed + 2) < 0.7) for (let k = 0; k < 3; k++) g.add(flor(Math.cos(k * 2.1) * r * 0.5, h - r * 0.2, Math.sin(k * 2.1) * r * 0.5, 1.3));   // coroa de flores
  return g;
}
function orgao(h, seed) {   // cacto-órgão: touceira de tubos de alturas diferentes saindo do mesmo pé
  const g = new THREE.Group(), m = flat(VERDES[(Math.floor(hash(seed) * 5) + 2) % 5]);
  for (let k = 0; k < 7; k++) { const a = k / 7 * Math.PI * 2 + hash(seed + k), d = k ? 16 + hash(seed + k * 3) * 10 : 0, L = h * (0.5 + hash(seed + k * 5) * 0.5), r = 11;
    const t = new THREE.Mesh(gomos(r, L, seed + k), m); t.position.set(u(Math.cos(a) * d), u(L / 2 + r), u(Math.sin(a) * d)); t.rotation.set(Math.sin(a) * d * 0.006, 0, -Math.cos(a) * d * 0.006); g.add(ol(t, 0.004)); }
  return g;
}
function flor(x, y, z, s = 1) { const g = new THREE.Group();
  for (let k = 0; k < 5; k++) { const a = k / 5 * Math.PI * 2, p = new THREE.Mesh(new THREE.SphereGeometry(u(4 * s), 8, 6), flat(C.flower)); p.scale.set(1, 0.6, 1); p.position.set(u(Math.cos(a) * 4 * s), 0, u(Math.sin(a) * 4 * s)); g.add(p); }
  const c = new THREE.Mesh(new THREE.SphereGeometry(u(2.5 * s), 8, 6), flat('#f6d040')); c.position.y = u(1.5 * s); g.add(c); g.position.set(u(x), u(y), u(z)); return g; }
function palma(seed, s = 1) {
  const g = new THREE.Group(), pads = [[0, 30, 0, 0], [-22, 64, 4, 0.5], [20, 70, -4, -0.4], [-6, 104, 2, 0.15], [34, 108, 0, -0.7]];
  pads.forEach(([x, y, z, rz], i) => { const p = new THREE.Mesh(new THREE.SphereGeometry(u(22 * s), 14, 10), flat(VERDES[(i + Math.floor(hash(seed) * 5)) % 5])); p.scale.set(0.85, 1.25, 0.32); p.position.set(u(x * s), u(y * s), u(z * s)); p.rotation.set(0, hash(seed + i) * 0.8, rz); g.add(ol(p, 0.004));
    if (i > 0 && hash(seed * 3 + i) < 0.8) g.add(flor(x * s + Math.sin(-rz) * 26 * s, y * s + Math.cos(rz) * 26 * s, z * s, s)); });
  return g;
}
// ================= vaso pintado: torno com a faixa de triângulos e losangos (textura)
const vasoTex = canvasTex(256, 128, (c, w, h) => { c.fillStyle = C.cream; c.fillRect(0, 0, w, h); c.fillStyle = C.pot; c.fillRect(0, 0, w, 26); c.fillRect(0, h - 22, w, 22);
  c.fillStyle = C.teal; for (let i = 0; i < 8; i++) { c.beginPath(); c.moveTo(i * 32, 26); c.lineTo(i * 32 + 16, 56); c.lineTo(i * 32 + 32, 26); c.fill(); }
  c.fillStyle = C.adobeD; for (let i = 0; i < 8; i++) { c.beginPath(); c.moveTo(i * 32 + 16, 64); c.lineTo(i * 32 + 26, 82); c.lineTo(i * 32 + 16, 100); c.lineTo(i * 32 + 6, 82); c.fill(); }
  c.fillStyle = C.tealD; c.fillRect(0, h - 26, w, 4); });
function vaso(s = 1, flores = false) {
  const g = new THREE.Group(), pts = [[14, 0], [24, 8], [32, 26], [33, 40], [26, 56], [18, 64], [20, 70]].map(([r, y]) => new THREE.Vector2(u(r * s), u(y * s)));
  const m = new THREE.Mesh(new THREE.LatheGeometry(pts, 24), toonTex(vasoTex)); g.add(ol(m, 0.005));
  if (flores) { const bush = new THREE.Mesh(new THREE.IcosahedronGeometry(u(40 * s), 2), flat('#3f7a34')); bush.scale.set(1.15, 0.8, 1.15); bush.position.y = u(92 * s); g.add(ol(bush, 0.005));
    for (let k = 0; k < 40; k++) { const a = hash(k * 3.1) * 6.28, e = hash(k * 1.7) * 1.2 - 0.2, r = 42 * s;
      g.add(flor(Math.cos(a) * Math.cos(e) * r * 1.1, 92 * s + Math.sin(e) * r * 0.8, Math.sin(a) * Math.cos(e) * r * 1.1, 1.3 * s)); }
    for (let k = 0; k < 8; k++) { const a = k / 8 * 6.28, d = new THREE.Mesh(new THREE.ConeGeometry(u(10 * s), u(40 * s), 6), flat(C.flower)); d.rotation.x = Math.PI; d.position.set(u(Math.cos(a) * 40 * s), u(56 * s), u(Math.sin(a) * 40 * s)); g.add(d); } }
  return g;
}

// ================= arquitetura de adobe
// reboco de adobe: base terracota com manchas mais claras/escuras e trincas finas (textura repetida)
const reboco = (col, rep = 2) => { const t = canvasTex(256, 256, (c, w) => { c.fillStyle = col; c.fillRect(0, 0, w, w); let sd = 3; const rnd = () => (sd = (sd * 9301 + 49297) % 233280) / 233280;
    for (let i = 0; i < 60; i++) { c.fillStyle = rnd() < 0.5 ? 'rgba(255,230,200,0.10)' : 'rgba(120,50,20,0.08)'; c.beginPath(); c.ellipse(rnd() * w, rnd() * w, 8 + rnd() * 30, 6 + rnd() * 18, rnd() * 3, 0, 7); c.fill(); }
    for (let i = 0; i < 3; i++) { const bx = rnd() * w * 0.8, by = rnd() * w * 0.8, bw = 50 + rnd() * 50, bh = 30 + rnd() * 30;   // reboco caído: tijolos de adobe à mostra
      c.save(); c.beginPath(); c.ellipse(bx + bw / 2, by + bh / 2, bw / 2, bh / 2, rnd() - 0.5, 0, 7); c.clip(); c.fillStyle = 'rgba(120,55,30,0.55)'; c.fillRect(bx, by, bw, bh);
      c.fillStyle = 'rgba(200,110,70,0.9)'; for (let yy = by; yy < by + bh; yy += 10) for (let xx = bx - ((yy / 10) % 2) * 11; xx < bx + bw; xx += 22) c.fillRect(xx + 1, yy + 1, 19, 8); c.restore();
      c.strokeStyle = 'rgba(255,225,190,0.5)'; c.lineWidth = 2; c.beginPath(); c.ellipse(bx + bw / 2, by + bh / 2, bw / 2, bh / 2, 0, 0, 7); c.stroke(); }
    for (let i = 0; i < 400; i++) { c.fillStyle = rnd() < 0.5 ? 'rgba(255,240,215,0.18)' : 'rgba(90,40,20,0.14)'; c.fillRect(rnd() * w, rnd() * w, 1.5, 1.5); }   // grão do reboco
    c.strokeStyle = 'rgba(110,50,25,0.35)'; c.lineWidth = 1.2; for (let i = 0; i < 5; i++) { let x = rnd() * w, y = rnd() * w; c.beginPath(); c.moveTo(x, y); for (let k = 0; k < 5; k++) { x += (rnd() - 0.5) * 30; y += rnd() * 14; c.lineTo(x, y); } c.stroke(); } }, [rep, rep]);
  return toonTex(t); };
const ADOBE = reboco(C.adobe), ADOBE_L = reboco(C.adobeL), ADOBE_ARCO = reboco(C.adobe, 0.45);   // arcos: UV da extrusão em metros
// detalhes soltos do pátio: agave (leque de folhas pontudas), cacto-bola com gomos, pedras, banco de madeira, potes empilhados
function agave(s = 1) { const parts = []; for (let k = 0; k < 14; k++) { const a = k / 14 * Math.PI * 2 + (k % 2) * 0.2, L = (44 + (k % 3) * 10) * s, g = new THREE.ConeGeometry(u(7 * s), u(L), 4);
    g.scale(1, 1, 0.35); g.translate(0, u(L / 2), 0); g.rotateX(0.5 + (k % 2) * 0.35); g.rotateY(a); parts.push(g.toNonIndexed()); }
  const ag = ol(new THREE.Mesh(mergeGeometries(parts), flat('#7aa07a')), 0.003); ag.userData.anim = { k: 'sway', a: 0.03 }; return ag; }
function bola(s = 1) { const g = new THREE.SphereGeometry(u(18 * s), 24, 14), p = g.attributes.position;
  for (let i = 0; i < p.count; i++) { const a = Math.atan2(p.getZ(i), p.getX(i)), k = 1 + 0.07 * Math.cos(a * 12); p.setX(i, p.getX(i) * k); p.setZ(i, p.getZ(i) * k); p.setY(i, p.getY(i) * 0.85); }
  g.computeVertexNormals(); const grp = new THREE.Group(), m = new THREE.Mesh(g, flat(C.cactusD)); m.position.y = u(14 * s); grp.add(ol(m, 0.004));
  if (s > 0.8) grp.add(flor(0, 30 * s, 0, 0.9)); return grp; }
function pedra(s, seed) { const g = new THREE.IcosahedronGeometry(u(s), 1), p = g.attributes.position; for (let i = 0; i < p.count; i++) p.setY(i, p.getY(i) * (0.55 + 0.2 * hash(seed + i)));
  g.computeVertexNormals(); const m = new THREE.Mesh(g, flat(hash(seed) < 0.5 ? '#c9a07a' : '#b8876a')); m.material.flatShading = true; return ol(m, 0.004); }
function banco() { const g = new THREE.Group(); g.add(ol(box(120, 8, 30, 0, 34, 0, '#8a5a34', 2), 0.004)); g.add(ol(box(120, 8, 6, 0, 60, -12, '#8a5a34', 2), 0.004));
  for (const x of [-50, 50]) for (const z of [-10, 10]) g.add(ol(box(6, 34, 6, x, 17, z, '#6e4426', 1), 0.003)); return g; }
// floreira/mureta em degrau: bloco de adobe com o friso verde-água e uma faixa de terra no alto
function mureta(w, h, d, x, z, ry = 0) {
  const g = new THREE.Group(); g.add(ol(box(w, h, d, 0, h / 2, 0, ADOBE, 4), 0.006)); g.add(ol(box(w + 6, 8, d + 6, 0, h + 4, 0, C.adobeL, 3), 0.004)); g.add(box(w + 2, 6, d + 2, 0, h - 12, 0, C.teal, 2));
  g.position.set(u(x), u(-DECK), u(z)); g.rotation.y = ry; return g;
}
// portal: arco de adobe com o sino pendurado, a moldura verde-água em cima e as lanterninhas
function arco(W, H, x, z, sino) {
  const g = new THREE.Group(), T = 40, sh = new THREE.Shape(), R = W / 2 - T;
  sh.moveTo(u(-W / 2), 0); sh.lineTo(u(-W / 2), u(H)); sh.lineTo(u(W / 2), u(H)); sh.lineTo(u(W / 2), 0); sh.lineTo(u(R), 0); sh.lineTo(u(R), u(H - T - R)); sh.absarc(0, u(H - T - R), u(R), 0, Math.PI, false); sh.lineTo(u(-R), 0); sh.lineTo(u(-W / 2), 0);
  const m = new THREE.Mesh(new THREE.ExtrudeGeometry(sh, { depth: u(40), bevelEnabled: true, bevelSize: u(3), bevelThickness: u(3), bevelSegments: 2, curveSegments: 24 }), ADOBE_ARCO); m.position.z = u(-20); g.add(ol(m, 0.006));
  if (sino) { // empena com a moldura verde-água em arco
    const cap = new THREE.Shape(); cap.moveTo(u(-W / 2 - 10), 0); cap.quadraticCurveTo(0, u(90), u(W / 2 + 10), 0); cap.lineTo(u(-W / 2 - 10), 0);
    const cm = new THREE.Mesh(new THREE.ExtrudeGeometry(cap, { depth: u(46), bevelEnabled: false, curveSegments: 20 }), ADOBE_ARCO); cm.position.set(0, u(H), u(-23)); g.add(ol(cm, 0.006));
    const rim = new THREE.Shape(); rim.moveTo(u(-W / 2 - 18), 0); rim.quadraticCurveTo(0, u(104), u(W / 2 + 18), 0); rim.lineTo(u(W / 2 + 4), 0); rim.quadraticCurveTo(0, u(80), u(-W / 2 - 4), 0);
    const rm = new THREE.Mesh(new THREE.ExtrudeGeometry(rim, { depth: u(50), bevelEnabled: false, curveSegments: 20 }), flat(C.teal)); rm.position.set(0, u(H + 4), u(-25)); g.add(ol(rm, 0.005));
    // sino: corpo em tronco de cone, cúpula em cima, borda dobrada embaixo e o badalo
    const by = H - T - 10; g.add(ol(cyl(13, 24, 40, 0, by - 22, 0, C.bell, 24), 0.005));
    const dome = new THREE.Mesh(new THREE.SphereGeometry(u(13), 20, 10, 0, Math.PI * 2, 0, Math.PI / 2), flat(C.bell)); dome.position.y = u(by - 2); g.add(ol(dome, 0.005));
    const lip = new THREE.Mesh(new THREE.TorusGeometry(u(24), u(3), 8, 28), flat(C.bell)); lip.rotation.x = Math.PI / 2; lip.position.y = u(by - 42); g.add(ol(lip, 0.004));
    g.add(ol(new THREE.Mesh(new THREE.SphereGeometry(u(5), 10, 8), flat('#6a5420')).translateY(u(by - 46)), 0.003));
    g.add(cyl(1.5, 1.5, 12, 0, H - T + 2, 0, '#3a3a3a', 6)); }
  for (const s of [-1, 1]) { const lx = s * (W / 2 - T / 2), lan = box(16, 22, 10, lx, H * 0.55, 24, C.teal, 3); g.add(ol(lan, 0.003)); g.add(box(8, 12, 4, lx, H * 0.55, 30, '#f6d58a', 1)); }
  g.position.set(u(x), u(-DECK), u(z)); return g;
}
// casa de adobe dos lados: bloco com a arcada e o friso verde-água no alto
function casa(w, h, d, x, z, ry) {
  const g = new THREE.Group(); g.add(ol(box(w, h, d, 0, h / 2, 0, ADOBE_L, 6), 0.006)); g.add(ol(box(w + 10, 14, d + 10, 0, h + 4, 0, C.teal, 3), 0.005));
  for (let i = 0; i < Math.round(w / 45); i++) { const v = cyl(6, 6, 40, -w / 2 + 22 + i * 45, h - 24, d / 2 + 10, '#6e4426', 8); v.rotation.x = Math.PI / 2; g.add(ol(v, 0.003)); }   // vigas saindo da parede
  for (const wx of [-w / 4, w / 4]) { g.add(box(34, 40, 4, wx, h * 0.62, d / 2 + 1, '#3a2a28', 1)); for (const s of [-1, 1]) g.add(ol(box(16, 44, 4, wx + s * 26, h * 0.62, d / 2 + 3, C.teal, 1.5), 0.003));
    g.add(ol(box(46, 6, 14, wx, h * 0.62 - 24, d / 2 + 6, C.adobeD, 1.5), 0.003)); }
  const vr = vaso(0.6); vr.position.set(u(w / 3), u(h + 10), 0); g.add(vr);
  const n = 1;
  for (let i = 0; i < n; i++) { const sh = new THREE.Shape(), R = 36, x0 = -w / 2 + w / n * (i + 0.5);
    sh.moveTo(u(-R), 0); sh.lineTo(u(-R), u(90)); sh.absarc(0, u(90), u(R), Math.PI, 0, true); sh.lineTo(u(R), 0);
    const door = new THREE.Mesh(new THREE.ShapeGeometry(sh, 16), flat('#7a3a24')); door.position.set(u(x0), 0, u(d / 2 + 0.5)); g.add(door); }
  g.position.set(u(x), u(-DECK), u(z)); g.rotation.y = ry; return g;
}
// bandeirolas: cordão em catenária com triângulos verde-água e creme
function bandeirolas(a, b, n = 18) {
  const g = new THREE.Group(), A = new THREE.Vector3(...a), B = new THREE.Vector3(...b), P = t => A.clone().lerp(B, t).add(new THREE.Vector3(0, -Math.sin(t * Math.PI) * 40, 0));
  const pts = []; for (let i = 0; i <= 24; i++) pts.push(P(i / 24).multiplyScalar(UNIT));
  g.add(new THREE.Mesh(new THREE.TubeGeometry(new THREE.CatmullRomCurve3(pts), 40, u(0.8), 4), flat('#5a4a3a')));
  const dir = B.clone().sub(A).normalize();
  for (let i = 1; i < n; i++) { const p = P(i / n), sh = new THREE.Shape(); sh.moveTo(u(-9), 0); sh.lineTo(u(9), 0); sh.lineTo(0, u(-22)); sh.lineTo(u(-9), 0);
    const f = new THREE.Mesh(new THREE.ShapeGeometry(sh), new THREE.MeshToonMaterial({ color: i % 2 ? C.teal : C.cream, side: THREE.DoubleSide, gradientMap: flat('#fff').gradientMap }));
    f.userData.anim = { k: 'flag', a: 0.05 }; f.position.copy(p).multiplyScalar(UNIT); f.rotation.y = -Math.atan2(dir.z, dir.x); g.add(f); }
  return g;
}
// mesa de pedra no horizonte: platô facetado em camadas
function mesa(w, h, d, seed) {
  const parts = []; let y = 0;
  for (let k = 0; k < 3; k++) { const hh = h * [0.45, 0.35, 0.2][k], s = 1 - k * 0.18, gg = new THREE.CylinderGeometry(u(w / 2 * s * 0.92), u(w / 2 * s), u(hh), 7, 1); gg.scale(1, 1, d / w); gg.rotateY(hash(seed + k)); gg.translate(0, u(y + hh / 2), 0); parts.push(gg.toNonIndexed()); y += hh; }
  const m = flat(C.mesa); m.flatShading = true; return new THREE.Mesh(mergeGeometries(parts), m);
}

export async function buildEspinhaArena() {
  const all = new THREE.Group();
  all.add(ringue());
  all.userData.look = { tint: [1.02, 0.97, 0.92], horizon: '#bfe0f4', zenith: '#3f8fe6', fog: [70, 190],
    sun: { color: '#fff0d8', int: 2.1, pos: [1.4, 3.4, 2.2] }, hemi: { sky: '#e4f2ff', ground: '#c47a52', int: 1.4 } };
  // chão do pátio: adobe terracota em lajotas grandes (textura) e o deserto além
  const laj = canvasTex(256, 256, (c, w) => { c.fillStyle = C.adobe; c.fillRect(0, 0, w, w); c.strokeStyle = C.adobeD; c.lineWidth = 4; c.strokeRect(2, 2, w / 2 - 4, w / 2 - 4); c.strokeRect(w / 2 + 2, 2, w / 2 - 4, w / 2 - 4); c.strokeRect(2, w / 2 + 2, w - 4, w / 2 - 4); }, [16, 16]);
  const chao = new THREE.Mesh(new THREE.PlaneGeometry(u(2200), u(2000)), toonTex(laj)); chao.rotation.x = -Math.PI / 2; chao.position.y = u(-DECK); all.add(chao);
  const deserto = new THREE.Mesh(new THREE.PlaneGeometry(u(9000), u(9000)), flat('#d99a6a')); deserto.rotation.x = -Math.PI / 2; deserto.position.y = u(-DECK - 2); all.add(deserto);
  // laterais abertas: muretas baixas longe do ringue (dá para ver o deserto por cima), cactos atrás delas, detalhes
  // soltos no chão do pátio (agaves, cactos-bola, pedras, bancos, potes) sem fechar a vista
  for (const s of [-1, 1]) {
    all.add(mureta(90, 40, 900, s * 780, -40), mureta(110, 70, 500, s * 920, -200));
    for (let k = 0; k < 5; k++) { const p = palma(k + (s > 0 ? 10 : 0), 0.7); p.position.set(u(s * 780), u(-DECK + 46), u(-400 + k * 190)); all.add(p); }
    for (let k = 0; k < 2; k++) { const v = vaso(0.8, k === 0); v.position.set(u(s * 920), u(-DECK + 76), u(-360 + k * 300)); all.add(v); }
    const sg = saguaro(420, s > 0 ? 3 : 7); sg.position.set(u(s * 1050), u(-DECK), u(-300)); sg.rotation.y = s > 0 ? Math.PI : 0; all.add(sg);
    const sg2 = saguaro(300, s > 0 ? 5 : 9); sg2.position.set(u(s * 1000), u(-DECK), u(300)); all.add(sg2);
    const b = banco(); b.position.set(u(s * 560), u(-DECK), u(-160)); b.rotation.y = s * -Math.PI / 2; all.add(b);
    [[470, 260, 'a'], [520, 120, 'b'], [600, -340, 'a'], [480, -420, 'p'], [640, 380, 'b'], [560, 40, 'p']].forEach(([x, z, t], k) => {
      const o = t === 'a' ? agave(0.9 + hash(k) * 0.4) : t === 'b' ? bola(0.7 + hash(k * 3) * 0.5) : pedra(16 + hash(k) * 14, k * 7 + (s > 0 ? 50 : 0)); o.position.set(u(s * x), u(-DECK), u(z)); all.add(o); });
    for (let k = 0; k < 3; k++) { const v = vaso(0.45 + k * 0.08); v.position.set(u(s * (500 + k * 30)), u(-DECK), u(420 + (k % 2) * 30)); all.add(v); }
    { const o = orgao(160, s > 0 ? 3 : 8); o.position.set(u(s * 1000), u(-DECK), u(-650)); all.add(o); }
    const pf = palma(s > 0 ? 21 : 22, 1.3); pf.position.set(u(s * 520), u(-DECK), u(560)); all.add(pf);
  }
  // fundo: escadaria, painel de azulejo, pedestais com vasos de buganvília, portal com o sino e os dois arcos
  const Z0 = -470;
  for (let k = 0; k < 4; k++) all.add(ol(box(420, 22 * (k + 1), 40, 0, -DECK + 11 * (k + 1), Z0 - k * 40, k % 2 ? ADOBE_L : ADOBE, 2), 0.004));
  const azul = canvasTex(512, 128, (c, w, h) => { c.fillStyle = C.teal; c.fillRect(0, 0, w, h); for (let i = 0; i < 7; i++) losango(c, (i + 0.5) * w / 7, h / 2, 6); for (let i = 0; i < 7; i++) losango(c, i * w / 7, h / 2 + 50, 4); });
  const painel = new THREE.Mesh(new THREE.BoxGeometry(u(300), u(90), u(10)), [flat(C.tealD), flat(C.tealD), flat(C.tealD), flat(C.tealD), toonTex(azul), flat(C.tealD)]);
  painel.position.set(0, u(-DECK + 88 + 45), u(Z0 - 170)); all.add(ol(painel, 0.004));
  all.add(ol(box(420, 88, 60, 0, -DECK + 44, Z0 - 180, ADOBE, 3), 0.005));
  for (const s of [-1, 1]) { all.add(ol(box(120, 190, 110, s * 230, -DECK + 95, Z0 - 170, ADOBE, 4), 0.006)); all.add(ol(box(130, 10, 120, s * 230, -DECK + 195, Z0 - 170, C.adobeL, 3), 0.004));
    const v = vaso(1.2, true); v.position.set(u(s * 230), u(-DECK + 200), u(Z0 - 170)); all.add(v); }
  all.add(arco(260, 380, 0, Z0 - 280, true));
  for (const s of [-1, 1]) { all.add(arco(220, 260, s * 330, Z0 - 300, false)); all.add(ol(box(160, 200, 40, s * 520, -DECK + 100, Z0 - 300, ADOBE, 3), 0.005)); }
  // casas de adobe nos lados do fundo e um saguaro em cada canto
  for (const s of [-1, 1]) { all.add(casa(280, 280, 220, s * 1250, -300, s * -Math.PI / 2)); all.add(casa(220, 220, 200, s * 1000, -820, 0)); all.add(casa(240, 180, 200, s * 1300, 300, s * -Math.PI / 2));
    const sg = saguaro(380, s > 0 ? 11 : 13); sg.position.set(u(s * 480), u(-DECK), u(-700)); all.add(sg); }
  // bandeirolas dos telhados ao portal
  all.add(bandeirolas([-1140, 290, -300], [-150, 320, -760]), bandeirolas([1140, 290, -300], [150, 320, -760]));
  if (!window.__ropeInfo) all.add(bandeirolas([-1140, 290, -200], [1140, 290, -200], 40));   // este passa por cima do ringue: fora do jogo 3D (tools3d/export)
  // atrás da câmera: muro de adobe com arcos e cactos
  all.add(mureta(1400, 50, 60, 0, 950)); for (const k of [-2, 2]) all.add(arco(200, 230, k * 300, 960, false));
  for (const x of [-700, 700]) { const sg = saguaro(340, x > 0 ? 17 : 19); sg.position.set(u(x), u(-DECK), u(820)); all.add(sg); }
  // deserto: palmas e saguaros soltos, mesas de pedra vermelha no horizonte
  for (let i = 0; i < 20; i++) { const a = (i / 20) * Math.PI * 2, r = 1500 + hash(i) * 700; const q = hash(i * 3), c = q < 0.4 ? saguaro(260 + hash(i * 5) * 200, i + 30) : q < 0.7 ? orgao(180 + hash(i) * 120, i) : palma(i + 40, 1.3); c.position.set(u(Math.sin(a) * r), u(-DECK), u(-Math.cos(a) * r)); all.add(c); }
  for (let i = 0; i < 9; i++) { const a = (i / 9 - 0.5) * Math.PI * 1.3, r = 3000 + hash(i * 2) * 400, m = mesa(500 + hash(i) * 500, 350 + hash(i * 7) * 450, 300, i);
    m.position.set(u(Math.sin(a) * r), u(-DECK), u(-Math.cos(a) * r)); all.add(m); }
  for (let i = 0; i < 6; i++) { const a = Math.PI + (i / 6 - 0.5) * Math.PI, r = 3000, m = mesa(600, 300 + hash(i * 9) * 300, 300, i + 20); m.position.set(u(Math.sin(a) * r), u(-DECK), u(-Math.cos(a) * r)); all.add(m); }
  // (1) floreiras altas de adobe nos dois cantos da frente, cheias de palmas floridas
  for (const s of [-1, 1]) { all.add(mureta(200, 70, 160, s * 470, 560));
    for (let k = 0; k < 3; k++) { const p = palma(60 + k + (s > 0 ? 5 : 0), 0.9 + k * 0.15); p.position.set(u(s * (430 + k * 40)), u(-DECK + 76), u(540 + (k % 2) * 40)); p.rotation.y = k; all.add(p);
      for (let f = 0; f < 4; f++) all.add(flor(s * (400 + hash(k * 9 + f) * 120), -DECK + 100 + hash(f + k) * 90, 520 + hash(f * 3 + k) * 80, 1.4)); } }
  // (2) nichos com lanterninhas verde-água nas muretas e nos lados da escadaria
  const nicho = (x, y, z, ry) => { const g = new THREE.Group(), sh = new THREE.Shape(); sh.moveTo(u(-12), 0); sh.lineTo(u(-12), u(18)); sh.absarc(0, u(18), u(12), Math.PI, 0, true); sh.lineTo(u(12), 0);
    const n = new THREE.Mesh(new THREE.ShapeGeometry(sh, 10), flat('#7a3a24')); g.add(n);
    g.add(ol(box(12, 16, 8, 0, 10, 4, C.teal, 2), 0.003)); g.add(box(6, 8, 2, 0, 10, 8.5, '#f6d58a', 1)); g.add(box(14, 3, 10, 0, 19, 4, C.tealD, 1));
    g.position.set(u(x), u(y), u(z)); g.rotation.y = ry; all.add(g); };
  for (const s of [-1, 1]) { for (let k = 0; k < 4; k++) nicho(s * (780 - s * -45.5), -DECK + 10, -320 + k * 210, s * -Math.PI / 2);
    nicho(s * 160, -DECK + 30, Z0 - 179.5 + 30.5, 0); nicho(s * 230, -DECK + 100, Z0 - 170 + 55.5, 0); }
  // (3) touceiras de palma florida ao pé do portal e dos pedestais
  for (let k = 0; k < 8; k++) { const s = k % 2 ? 1 : -1, p = palma(80 + k, 0.6 + hash(k) * 0.3); p.position.set(u(s * (150 + hash(k * 3) * 230)), u(-DECK + 88), u(Z0 - 130 - hash(k * 5) * 30)); p.rotation.y = k; all.add(p); }
  for (let k = 0; k < 16; k++) all.add(flor((hash(k) - 0.5) * 760, -DECK + 92 + hash(k * 2) * 60, Z0 - 120 - hash(k * 4) * 40, 1.5));
  // (4) terraços em degrau dos lados, atrás das muretas, com vaso pintado em cada degrau
  for (const s of [-1, 1]) for (let k = 0; k < 3; k++) { const h = 50 + k * 45, x = s * (880 + k * 70);
    all.add(mureta(70, h, 360, x, 420)); const v = vaso(0.55 + k * 0.1, k === 2); v.position.set(u(x), u(-DECK + h + 6), u(420 + (k - 1) * 90)); all.add(v); }
  // (5) mesas de pedra vermelha próximas, aparecendo nos vãos dos arcos do fundo
  [[-560, -1500, 520, 420], [600, -1600, 600, 520], [0, -2000, 700, 380], [-1300, -1700, 500, 600], [1350, -1650, 540, 560]].forEach(([x, z, w, h], i) => { const m = mesa(w, h, 260, i + 60); m.position.set(u(x), u(-DECK), u(z)); all.add(m); });
  // horizonte recortado: anel de dunas baixas onduladas e, atrás, a serra lilás contínua com picos e platôs
  const anel = (R, base, amp, col, seed, N = 180) => { const pos = [], idx = [];
    for (let i = 0; i <= N; i++) { const a = i / N * Math.PI * 2, hgt = base + amp * (0.5 + 0.3 * Math.sin(a * 5 + seed) + 0.2 * Math.sin(a * 13 + seed * 2) + 0.15 * Math.sin(a * 29 + seed * 3)) * (hash(Math.floor(i / 6) + seed) < 0.25 ? 1.5 : 1);
      const x = Math.sin(a) * R, z = -Math.cos(a) * R; pos.push(u(x), u(-DECK - 4), u(z), u(x * 1.02), u(-DECK + hgt), u(z * 1.02)); if (i < N) { const b = i * 2; idx.push(b, b + 2, b + 1, b + 1, b + 2, b + 3); } }
    const g = new THREE.BufferGeometry(); g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3)); g.setIndex(idx); g.computeVertexNormals();
    const m = flat(col); m.side = THREE.DoubleSide; return new THREE.Mesh(g, m); };
  all.add(anel(2300, 40, 120, '#d9915e', 1), anel(2900, 80, 260, '#c97a5a', 4), anel(3700, 200, 700, '#b07a9a', 7, 240));
  return all;
}
