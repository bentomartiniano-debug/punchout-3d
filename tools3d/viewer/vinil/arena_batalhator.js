// Central do Batalhator, em código e no acabamento do vinil, pelo concept arena-concepts/09-central-batalhator.png e o
// kit (torre/telão industrial, anel suspenso): arena industrial à noite. Ringue de lona clara com as faixas e o alvo
// vermelhos, saia de chapa grafite com placas amarelas e parafusos, degraus amarelos nos cantos da frente, postes
// quadrados vermelho (esquerda) e azul-ciano (direita), cordas vermelha, branca e azul; piso de chapa com faixas de
// perigo e grelhas; arquibancadas em U com guarda-corpo amarelo e a torcida (vermelho/azul/amarelo) pulando; ao fundo
// o portal com o anel suspenso amarelo e preto, estandartes vermelhos com a coroa, telões vermelhos, frisos de luz
// ciano, treliças vermelhas, as torres com os tambores listrados e os refletores. Unidades do jogo.
import { THREE, flat, withOutline } from './core.js';
import { RoundedBoxGeometry } from 'three/addons/geometries/RoundedBoxGeometry.js';
import { mergeGeometries } from 'three/addons/utils/BufferGeometryUtils.js';
import { UNIT } from './arena_pernambuco.js';
import { fundeEstaticos } from './arena_jack.js';

const u = v => v * UNIT;
const HALF = 270, DECK = 50, POST_TOP = 125, ROPES = [38, 68, 98], Y0 = -DECK;
const C = { metal: '#3a3f4a', metalD: '#2a2e36', metalL: '#4c5260', yellow: '#f2b81c', red: '#e0203c', redD: '#a8142c', cyan: '#2ab8e8', white: '#eef0f2',
            canvas: '#e8e6e2', ink: '#16181d', screen: '#f0284a' };
const ol = (m, w = 0.007) => { const g = withOutline(m, w); g.userData.olw = w; g.position.copy(m.position); g.quaternion.copy(m.quaternion); g.scale.copy(m.scale); m.position.set(0, 0, 0); m.quaternion.identity(); m.scale.set(1, 1, 1); return g; };
const box = (w, h, d, x, y, z, col, r = 2) => { const m = new THREE.Mesh(new RoundedBoxGeometry(u(w), u(h), u(d), 2, u(Math.min(r, w / 2 - 0.01, h / 2 - 0.01, d / 2 - 0.01))), typeof col === 'string' ? flat(col) : col); m.position.set(u(x), u(y), u(z)); return m; };
const cyl = (r0, r1, h, x, y, z, col, seg = 20) => { const m = new THREE.Mesh(new THREE.CylinderGeometry(u(r0), u(r1), u(h), seg), typeof col === 'string' ? flat(col) : col); m.position.set(u(x), u(y), u(z)); return m; };
const hash = n => { const v = Math.sin(n * 127.1 + 311.7) * 43758.5453; return v - Math.floor(v); };
const toonTex = tex => new THREE.MeshToonMaterial({ map: tex, gradientMap: flat('#fff').gradientMap });
const glow = (col, op = 1) => new THREE.MeshBasicMaterial({ color: col, transparent: op < 1, opacity: op });
function canvasTex(w, h, draw, rep) {
  const c = document.createElement('canvas'); c.width = w; c.height = h; draw(c.getContext('2d'), w, h);
  const t = new THREE.CanvasTexture(c); t.colorSpace = THREE.SRGBColorSpace; t.anisotropy = 8;
  if (rep) { t.wrapS = t.wrapT = THREE.RepeatWrapping; t.repeat.set(rep[0], rep[1]); } return t;
}
// listra de perigo (amarelo e preto em diagonal)
const perigoTex = rep => canvasTex(128, 32, (c, w, h) => { c.fillStyle = C.yellow; c.fillRect(0, 0, w, h); c.fillStyle = C.ink; for (let x = -h; x < w + h; x += 32) { c.beginPath(); c.moveTo(x, h); c.lineTo(x + 16, h); c.lineTo(x + 16 + h, 0); c.lineTo(x + h, 0); c.fill(); } }, rep);
// coroa amarela com a seta rosa (estandartes e telões)
function coroa(c, x, y, s) { c.fillStyle = C.yellow; c.beginPath(); c.moveTo(x - 40 * s, y + 20 * s); c.lineTo(x - 40 * s, y - 20 * s); c.lineTo(x - 20 * s, y); c.lineTo(x, y - 34 * s); c.lineTo(x + 20 * s, y); c.lineTo(x + 40 * s, y - 20 * s); c.lineTo(x + 40 * s, y + 20 * s); c.fill();
  c.fillStyle = '#ff7a9a'; for (const dy of [44, 74]) { c.beginPath(); c.moveTo(x - 40 * s, y + (dy - 10) * s); c.lineTo(x, y + (dy + 14) * s); c.lineTo(x + 40 * s, y + (dy - 10) * s); c.lineTo(x + 40 * s, y + dy * s); c.lineTo(x, y + (dy + 24) * s); c.lineTo(x - 40 * s, y + dy * s); c.fill(); } }

// ================= ringue
function ringue() {
  const g = new THREE.Group(), E = HALF + 34;
  const lona = canvasTex(1024, 1024, (c, w) => { const k = w / (2 * E), P = v => (v + E) * k;
    c.fillStyle = C.ink; c.fillRect(0, 0, w, w); c.fillStyle = C.canvas; c.fillRect(P(-E + 14), P(-E + 14), (2 * E - 28) * k, (2 * E - 28) * k);
    c.strokeStyle = C.red; c.lineWidth = 8 * k; c.strokeRect(P(-HALF + 20), P(-HALF + 20), (2 * HALF - 40) * k, (2 * HALF - 40) * k);
    c.lineWidth = 5 * k; for (const s of [-1, 1]) for (const t of [-1, 1]) { c.beginPath(); c.moveTo(P(s * (HALF - 70)), P(t * (HALF - 40))); c.lineTo(P(s * (HALF - 130)), P(t * (HALF - 40))); c.stroke();
      c.beginPath(); c.moveTo(P(s * (HALF - 40)), P(t * (HALF - 70))); c.lineTo(P(s * (HALF - 40)), P(t * (HALF - 130))); c.stroke(); }
    c.lineWidth = 9 * k; c.beginPath(); c.arc(P(0), P(0), 70 * k, 0, 7); c.stroke();   // alvo
    c.lineWidth = 7 * k; c.beginPath(); c.moveTo(P(0), P(70)); c.lineTo(P(0), P(HALF - 30)); c.moveTo(P(0), P(-70)); c.lineTo(P(0), P(-110)); c.stroke();
    c.fillStyle = 'rgba(0,0,0,0.05)'; for (let i = 0; i < 300; i++) c.fillRect(Math.random() * w, Math.random() * w, 6, 2); });
  const top = new THREE.Mesh(new THREE.BoxGeometry(u(2 * E), u(8), u(2 * E)), [flat(C.ink), flat(C.ink), toonTex(lona), flat(C.ink), flat(C.ink), flat(C.ink)]); top.position.y = u(-4); g.add(ol(top, 0.004));
  // saia: chapa grafite com a faixa de baixo, placas amarelas e parafusos (textura) e o friso de perigo
  const saia = canvasTex(1024, 128, (c, w, h) => { c.fillStyle = C.metalD; c.fillRect(0, 0, w, h); c.fillStyle = C.metal; c.fillRect(0, 10, w, h - 46);
    c.fillStyle = '#23262d'; c.fillRect(0, h - 36, w, 36); c.strokeStyle = '#1a1c22'; c.lineWidth = 3; for (let x = 0; x < w; x += 128) { c.beginPath(); c.moveTo(x, 10); c.lineTo(x, h - 36); c.stroke(); }
    c.fillStyle = C.yellow; for (const x of [150, 330, 694, 874]) { c.fillRect(x - 34, h - 30, 68, 20); } c.fillRect(w / 2 - 90, h - 32, 180, 24);
    c.fillStyle = C.ink; for (const x of [150, 330, 694, 874]) for (const d of [-22, 22]) { c.beginPath(); c.moveTo(x + d - 6, h - 12); c.lineTo(x + d + 2, h - 12); c.lineTo(x + d + 8, h - 28); c.lineTo(x + d, h - 28); c.fill(); }
    c.fillStyle = '#6a707c'; for (let x = 24; x < w; x += 64) { c.beginPath(); c.arc(x, 20, 3, 0, 7); c.fill(); } });
  for (let s = 0; s < 4; s++) { const sk = new THREE.Mesh(new THREE.BoxGeometry(u(2 * E), u(DECK - 8), u(6)), [flat(C.metalD), flat(C.metalD), flat(C.metalD), flat(C.metalD), toonTex(saia), flat(C.metalD)]);
    const a = s * Math.PI / 2; sk.position.set(u(Math.sin(a) * (E - 3)), u(-8 - (DECK - 8) / 2), u(Math.cos(a) * (E - 3))); sk.rotation.y = a; g.add(ol(sk, 0.004)); }
  g.add(box(2 * E + 10, 8, 2 * E + 10, 0, -10, 0, C.metalL, 1.5));   // lábio de chapa
  // degraus amarelos em escada nos cantos da frente, base grafite dos postes
  for (const s of [-1, 1]) { g.add(ol(box(90, 40, 60, s * (E - 10), Y0 + 20, E + 40, C.yellow, 3), 0.005)); g.add(ol(box(60, 20, 60, s * (E - 25), Y0 + 50, E + 10, C.yellow, 3), 0.004));
    g.add(box(30, 16, 4, s * (E - 10), Y0 + 20, E + 71, C.ink, 1)); }
  const corners = [[-HALF, HALF, C.red], [HALF, HALF, C.cyan], [HALF, -HALF, C.cyan], [-HALF, -HALF, C.red]];
  for (const [x, z, col] of corners) {
    g.add(ol(box(44, 18, 44, x, 4, z, C.ink, 3), 0.004));
    g.add(ol(box(34, POST_TOP + 10, 34, x, POST_TOP / 2 + 4, z, col, 4), 0.006)); g.add(box(8, POST_TOP - 20, 2, x - Math.sign(x) * 0 + 0, POST_TOP / 2, z + Math.sign(z) * 18, C.ink, 1));
    const sx = -Math.sign(x), sz = -Math.sign(z);
    for (const h of ROPES) for (const [dx, dz] of [[sx, 0], [0, sz]]) { g.add(ol(box(dx ? 28 : 30, 26, dz ? 28 : 30, x + dx * 30, h, z + dz * 30, col, 4), 0.004)); g.add(box(dx ? 6 : 14, 10, dz ? 6 : 14, x + dx * 46, h, z + dz * 46, C.ink, 1)); } }
  for (let i = 0; i < 4; i++) { const [ax, az] = corners[i], [bx, bz] = corners[(i + 1) % 4];
    ROPES.forEach((h, k) => { const a = new THREE.Vector3(u(ax), u(h), u(az)), b = new THREE.Vector3(u(bx), u(h), u(bz)), len = a.distanceTo(b);
      if (window.__ropeInfo) { window.__ropeInfo.push({ y: u(h), color: [C.cyan, C.white, C.red][k] }); return; }   // o jogo 3D desenha as cordas (tools3d/export)
      const cg = new THREE.CylinderGeometry(u(4), u(4), len, 10); cg.rotateZ(Math.PI / 2);
      const m = new THREE.Mesh(cg, flat([C.cyan, C.white, C.red][k])); m.position.copy(a).add(b).multiplyScalar(0.5); m.rotation.y = -Math.atan2(b.z - a.z, b.x - a.x); g.add(ol(m, 0.004)); }); }
  return g;
}

// ================= arquibancada em degraus (de frente para o ringue), com o guarda-corpo amarelo; devolve os lugares
function arquibancada(cx, cz, L, ry, rows, y0, seats) {
  const g = new THREE.Group(), RH = 26, RD = 34;
  for (let r = 0; r < rows; r++) { g.add(box(L, RH * (r + 1), RD, 0, y0 + RH * (r + 1) / 2, -r * RD, r % 2 ? C.metal : C.metalD, 1));
    for (let x = -L / 2 + 16; x < L / 2 - 10; x += 30) if (Math.abs(x) > 40) seats.push({ g, x, y: y0 + RH * (r + 1), z: -r * RD - 4, ry, cx, cz }); }
  // escada do meio (vão sem gente) e guarda-corpo na frente
  g.add(box(70, 4, rows * RD, 0, y0 + 2, -(rows - 1) * RD / 2, C.yellow, 1));
  for (let x = -L / 2; x <= L / 2; x += 60) g.add(box(5, 40, 5, x, y0 + 20, 24, C.yellow, 1));
  g.add(box(L, 6, 6, 0, y0 + 40, 24, C.yellow, 1)); g.add(box(L, 4, 4, 0, y0 + 22, 24, C.yellow, 1));
  g.add(box(L, y0 + DECK, 20, 0, (y0 - DECK) / 2, 12, C.metalD, 1));
  g.add(box(L, 10, 4, 0, y0 - 14, 23, C.red, 1));   // friso vermelho
  g.position.set(u(cx), 0, u(cz)); g.rotation.y = ry; return g;
}
// treliça: duas barras e as diagonais em zigue-zague entre A e B
function trelica(a, b, h, col, n = 10) {
  const g = new THREE.Group(), A = new THREE.Vector3(...a), B = new THREE.Vector3(...b), up = new THREE.Vector3(0, h, 0), bar = (p, q, r) => { const L = p.distanceTo(q), m = new THREE.Mesh(new THREE.BoxGeometry(u(r), L * UNIT, u(r)), flat(col));
    m.position.copy(p).add(q).multiplyScalar(0.5 * UNIT); m.quaternion.setFromUnitVectors(new THREE.Vector3(0, 1, 0), q.clone().sub(p).normalize()); g.add(ol(m, 0.003)); };
  bar(A, B, 10); bar(A.clone().add(up), B.clone().add(up), 10);
  for (let i = 0; i < n; i++) { const p = A.clone().lerp(B, i / n), q = A.clone().lerp(B, (i + 1) / n); bar(i % 2 ? p.clone().add(up) : p, i % 2 ? q : q.clone().add(up), 6); }
  return g;
}
// tambor listrado de perigo (torres e anel)
function tambor(r, h, x, y, z) { const g = new THREE.Group(), t = perigoTex([Math.round(r / 6), 1]);
  const m = new THREE.Mesh(new THREE.CylinderGeometry(u(r), u(r), u(h), 32, 1, true), toonTex(t)); m.material.side = THREE.DoubleSide; g.add(m);
  for (const s of [-1, 1]) g.add(ol(cyl(r + 6, r + 6, 10, 0, s * (h / 2 + 4), 0, C.ink, 32), 0.004));
  g.position.set(u(x), u(y), u(z)); return g; }

export async function buildBatalhatorArena() {
  const all = new THREE.Group();
  all.add(ringue());
  all.userData.look = { tint: [0.92, 0.95, 1.08], horizon: '#232733', zenith: '#101219', fog: [90, 220],
    sun: { color: '#f4f6ff', int: 2.0, pos: [0.6, 3.6, 2.4] }, hemi: { sky: '#9aa6c8', ground: '#2a2a34', int: 1.1 } };
  // ---- piso de chapa: placas com juntas, faixas de perigo amarelas e grelhas
  const chapa = canvasTex(256, 256, (c, w) => { c.fillStyle = '#2c3038'; c.fillRect(0, 0, w, w); c.strokeStyle = '#1e2127'; c.lineWidth = 4; c.strokeRect(2, 2, w - 4, w - 4); c.strokeRect(2, 2, w / 2 - 2, w - 4);
    c.fillStyle = '#353a44'; for (let i = 0; i < 40; i++) { const x = (i * 53) % w, y = (i * 97) % w; c.fillRect(x, y, 10, 3); c.fillRect(x + 4, y - 4, 3, 10); } }, [20, 20]);
  const piso = new THREE.Mesh(new THREE.PlaneGeometry(u(3000), u(3000)), toonTex(chapa)); piso.rotation.x = -Math.PI / 2; piso.position.y = u(Y0); piso.userData.keep = true; all.add(piso);
  for (const s of [-1, 1]) { all.add(box(260, 2, 40, s * 260, Y0 + 1, 420, C.yellow, 0.5)); all.add(box(40, 2, 240, s * 400, Y0 + 1, 160, C.yellow, 0.5));
    const gr = new THREE.Mesh(new THREE.PlaneGeometry(u(260), u(40)), toonTex(canvasTex(128, 32, (c, w, h) => { c.fillStyle = '#16181d'; c.fillRect(0, 0, w, h); c.fillStyle = '#4a505c'; for (let x = 4; x < w; x += 8) c.fillRect(x, 4, 3, h - 8); })));
    gr.rotation.x = -Math.PI / 2; gr.position.set(u(s * 560), u(Y0 + 1.5), u(420)); all.add(gr); }
  // ---- arquibancadas em U (fundo e laterais) e a torcida
  const seats = [];
  all.add(arquibancada(0, -760, 1200, 0, 12, 30, seats));
  all.add(arquibancada(-760, -60, 1100, Math.PI / 2, 12, 30, seats), arquibancada(760, -60, 1100, -Math.PI / 2, 12, 30, seats));
  all.add(arquibancada(-700, 620, 700, Math.PI * 0.75, 10, 30, seats), arquibancada(700, 620, 700, -Math.PI * 0.75, 10, 30, seats));
  all.add(arquibancada(0, 820, 900, Math.PI, 10, 30, seats));
  // ---- portal do fundo: pilares com os frisos de luz ciano, estandartes com a coroa, anel suspenso, treliças vermelhas
  const ZB = -1250;
  const estandarte = canvasTex(256, 512, (c, w, h) => { c.fillStyle = C.red; c.fillRect(0, 0, w, h); c.fillStyle = C.redD; c.fillRect(0, 0, 16, h); c.fillRect(w - 16, 0, 16, h); coroa(c, w / 2, h * 0.38, 1.6); });
  for (const s of [-1, 1]) {
    all.add(ol(box(120, 820, 90, s * 330, Y0 + 410, ZB, C.metal, 4), 0.006)); all.add(ol(box(140, 40, 110, s * 330, Y0 + 520, ZB, C.yellow, 3), 0.005));
    const est = new THREE.Mesh(new THREE.PlaneGeometry(u(130), u(260), 4, 10), toonTex(estandarte)); est.userData.anim = { k: 'flag', a: 0.07 }; est.position.set(u(s * 210), u(Y0 + 560), u(ZB + 40)); all.add(est);
    for (const dx of [-70, 70]) all.add(box(10, 160, 4, s * 330 + dx, Y0 + 640, ZB + 47, glow(C.cyan)));
    all.add(trelica([s * 390, 820, ZB], [s * 1100, 600, ZB + 300], 50, C.red, 12));
  }
  all.add(ol(box(560, 90, 90, 0, Y0 + 270, ZB - 20, C.metalD, 3), 0.006)); all.add(ol(box(560, 20, 100, 0, Y0 + 320, ZB - 20, C.yellow, 2), 0.004));
  for (let x = -230; x <= 230; x += 92) { const v = box(50, 50, 6, x, Y0 + 270, ZB + 26, C.yellow, 1); v.rotation.z = Math.PI / 4; all.add(v); }   // os "V" amarelos da viga
  all.add(box(420, 300, 10, 0, Y0 + 470, ZB - 60, '#15171d', 1));   // vão escuro do portal
  const anel = tambor(220, 70, 0, Y0 + 780, ZB + 20); anel.userData.anim = { k: 'spin', axis: [0, 1, 0], sp: 0.15 }; all.add(anel);
  for (let k = 0; k < 8; k++) { const a = k / 8 * Math.PI * 2; all.add(box(26, 10, 6, Math.cos(a) * 226, Y0 + 744, ZB + 20 + Math.sin(a) * 226, glow('#ffffff'))); }
  for (const s of [-1, 1]) all.add(cyl(4, 4, 260, s * 200, Y0 + 940, ZB + 20, C.ink, 6));   // cabos do anel
  // ---- torres dos lados com os tambores listrados e os refletores; telões vermelhos inclinados
  const tela = canvasTex(512, 256, (c, w, h) => { c.fillStyle = C.screen; c.fillRect(0, 0, w, h); c.fillStyle = '#ff5a74'; for (let i = 0; i < 4; i++) { c.beginPath(); c.moveTo(60 + i * 90, 40); c.lineTo(160 + i * 90, 128); c.lineTo(60 + i * 90, 216); c.lineTo(110 + i * 90, 216); c.lineTo(210 + i * 90, 128); c.lineTo(110 + i * 90, 40); c.fill(); }
    c.fillStyle = 'rgba(0,0,0,0.12)'; for (let y = 0; y < h; y += 4) c.fillRect(0, y, w, 1); }, [1, 1]);
  tela.wrapS = THREE.RepeatWrapping;
  for (const s of [-1, 1]) {
    const tx = s * 820, tz = -1050;
    all.add(ol(box(140, 1000, 140, tx, Y0 + 500, tz, C.metal, 4), 0.006));
    all.add(tambor(110, 60, tx, Y0 + 860, tz)); all.add(tambor(90, 40, tx, Y0 + 700, tz));
    for (const dy of [300, 460]) all.add(trelica([tx - 70, dy, tz + 70], [tx + 70, dy, tz + 70], 40, C.red, 4));
    const tv = new THREE.Mesh(new THREE.BoxGeometry(u(420), u(230), u(14)), [flat(C.ink), flat(C.ink), flat(C.ink), flat(C.ink), new THREE.MeshBasicMaterial({ map: tela }), flat(C.ink)]);
    tv.userData.anim = { k: 'scroll', a: 0.15, x: true }; tv.position.set(u(s * 1080), u(Y0 + 720), u(-650)); tv.rotation.y = -s * 1.0; all.add(ol(tv, 0.005));
    const tv2 = new THREE.Mesh(new THREE.BoxGeometry(u(300), u(160), u(12)), [flat(C.ink), flat(C.ink), flat(C.ink), flat(C.ink), new THREE.MeshBasicMaterial({ map: tela }), flat(C.ink)]);
    tv2.userData.anim = { k: 'scroll', a: 0.15, x: true }; tv2.position.set(u(s * 560), u(Y0 + 520), u(-1150)); all.add(ol(tv2, 0.005));
    for (let k = 0; k < 4; k++) all.add(box(30, 16, 8, s * (700 + k * 120), Y0 + 980 - k * 40, -900 + k * 120, glow('#ffffff')));   // refletores
    all.add(box(8, 220, 6, s * 470, Y0 + 640, ZB + 47, glow(C.cyan)));
  }
  // anel de camarotes escuros em cima das arquibancadas (fecha o horizonte, a arena é coberta)
  for (let i = 0; i < 28; i++) { const a = i / 28 * Math.PI * 2, R = 1500; const p = box(360, 260, 40, Math.sin(a) * R, Y0 + 560, -Math.cos(a) * R, C.metalD, 2); p.rotation.y = -a; all.add(p);
    const l = box(300, 8, 4, Math.sin(a) * (R - 22), Y0 + 640, -Math.cos(a) * (R - 22), glow(i % 3 ? '#e8eef8' : C.cyan)); l.rotation.y = -a; all.add(l); }
  fundeEstaticos(all);
  // ---- torcida: bonequinhos (corpo e cabeça) nas cores do concept, em instâncias, pulando em ondas
  const corpo = new THREE.CapsuleGeometry(u(8), u(14), 3, 8).translate(0, u(15), 0), cabeca = new THREE.SphereGeometry(u(7), 8, 6).translate(0, u(36), 0);
  const pessoa = mergeGeometries([corpo.toNonIndexed(), cabeca.toNonIndexed()]);
  const cores = ['#e0283c', '#2a6ad8', '#f2b81c', '#e85a78', '#3aa0e0', '#c8322e', '#f6d050'];
  const lugares = seats.filter((_, i) => hash(i * 1.3) < 0.88);
  const torcida = new THREE.InstancedMesh(pessoa, flat('#ffffff'), lugares.length), M = new THREE.Matrix4(), v = new THREE.Vector3(), q = new THREE.Quaternion();
  lugares.forEach((s, i) => { s.g.updateMatrixWorld(true); v.set(u(s.x), u(s.y), u(s.z)).applyMatrix4(s.g.matrixWorld); q.setFromEuler(new THREE.Euler(0, s.ry + (hash(i) - 0.5) * 0.4, 0));
    M.compose(v, q, new THREE.Vector3(1, 0.9 + hash(i * 7) * 0.25, 1)); torcida.setMatrixAt(i, M); torcida.setColorAt(i, new THREE.Color(cores[Math.floor(hash(i * 3) * cores.length)])); });
  const relogio = { value: 0 };
  torcida.material.onBeforeCompile = sh => { sh.uniforms.uT = relogio; sh.vertexShader = 'uniform float uT;\n' + sh.vertexShader.replace('#include <project_vertex>', `vec4 mvPosition = vec4( transformed, 1.0 );
      mvPosition = instanceMatrix * mvPosition;
      { vec3 ip = instanceMatrix[3].xyz; float w = sin(uT * 2.2 - atan(ip.x, -ip.z) * 3.0); mvPosition.y += max(w, 0.0) * max(w, 0.0) * 0.07 + abs(sin(uT * 7.0 + ip.x * 11.0 + ip.z * 7.0)) * 0.012; }
      mvPosition = modelViewMatrix * mvPosition;
      gl_Position = projectionMatrix * mvPosition;`); };
  torcida.material.customProgramCacheKey = () => 'torcida';
  torcida.instanceMatrix.needsUpdate = true; torcida.instanceColor.needsUpdate = true; torcida.frustumCulled = false; torcida.userData.anim = { k: 'crowd' }; all.add(torcida);
  // relógio: torcida em ola, telões correndo, anel girando devagar
  piso.onBeforeRender = () => { const t = performance.now() / 1000; relogio.value = t; tela.offset.x = (t * 0.15) % 1; anel.rotation.y = t * 0.15; };
  return all;
}
