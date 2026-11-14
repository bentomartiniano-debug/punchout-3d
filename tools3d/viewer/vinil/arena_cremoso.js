// Sorveteria do Cremoso, em código e no acabamento do vinil, pelo concept arena-concepts/06-cremoso-sorveteria.png:
// salão rosa e creme de piso xadrez; ringue de lona creme com o círculo rosa e a casquinha no meio, saia de toldo em
// gomos rosa/creme, postes quadrados alternando rosa e verde-água com bola creme no alto, cordas rosa e creme, escadinhas
// nos cantos da frente; ao fundo o balcão com toldo listrado (máquina de café, dispensers de calda, geladeira), nas
// laterais os balcões-vitrine de sorvete e doces com banquinhos; colunas creme com calda de chocolate escorrendo, arcos
// de janelão com a cidade lá fora, luminárias pendentes, vasos com topiaria e as duas estátuas gigantes (casquinha e
// taça de sundae). Unidades do jogo.
import { THREE, flat, withOutline } from './core.js';
import { RoundedBoxGeometry } from 'three/addons/geometries/RoundedBoxGeometry.js';
import { mergeGeometries } from 'three/addons/utils/BufferGeometryUtils.js';
import { UNIT } from './arena_pernambuco.js';

const u = v => v * UNIT;
const HALF = 270, DECK = 46, POST_TOP = 125, ROPES = [38, 68, 98], ROOM_X = 1150, ROOM_Z = 1150, WALL_H = 620;
const C = { pink: '#e8607a', pinkL: '#f39aaa', pinkD: '#c94a66', cream: '#f6e6cf', creamD: '#e8d2b4', teal: '#5fbfaa', tealD: '#3f9e8c', choco: '#5a3326', chocoL: '#7a4a35',
            ball: '#f7efe2', glass: '#cfeff0', wall: '#f3c9c9', cone: '#d9a35f', coneD: '#b98244', sky: '#8fc8f0', metal: '#b9bfc6', leaf: '#3f8a4a', pot: '#6fc2b0' };
const ol = (m, w = 0.007) => { const g = withOutline(m, w); g.position.copy(m.position); g.quaternion.copy(m.quaternion); g.scale.copy(m.scale); m.position.set(0, 0, 0); m.quaternion.identity(); m.scale.set(1, 1, 1); return g; };
const box = (w, h, d, x, y, z, col, r = 3) => { const m = new THREE.Mesh(new RoundedBoxGeometry(u(w), u(h), u(d), 2, u(Math.min(r, w / 2 - 0.01, h / 2 - 0.01, d / 2 - 0.01))), typeof col === 'string' ? flat(col) : col); m.position.set(u(x), u(y), u(z)); return m; };
const cyl = (r0, r1, h, x, y, z, col, seg = 20) => { const m = new THREE.Mesh(new THREE.CylinderGeometry(u(r0), u(r1), u(h), seg), typeof col === 'string' ? flat(col) : col); m.position.set(u(x), u(y), u(z)); return m; };
const ball = (r, x, y, z, col) => { const m = new THREE.Mesh(new THREE.SphereGeometry(u(r), 20, 14), flat(col)); m.position.set(u(x), u(y), u(z)); return m; };
const toonTex = tex => new THREE.MeshToonMaterial({ map: tex, gradientMap: flat('#fff').gradientMap });
function canvasTex(w, h, draw, rep) {
  const c = document.createElement('canvas'); c.width = w; c.height = h; draw(c.getContext('2d'), w, h);
  const t = new THREE.CanvasTexture(c); t.colorSpace = THREE.SRGBColorSpace; t.anisotropy = 8;
  if (rep) { t.wrapS = t.wrapT = THREE.RepeatWrapping; t.repeat.set(rep[0], rep[1]); } return t;
}

// ================= piso do salão: xadrez rosa e creme em diagonal (como no concept)
function piso() {
  const tex = canvasTex(256, 256, (g, w, h) => { g.fillStyle = C.cream; g.fillRect(0, 0, w, h); g.fillStyle = C.pinkL; g.fillRect(0, 0, w / 2, h / 2); g.fillRect(w / 2, h / 2, w / 2, h / 2); }, [ROOM_X * 2 / 120, ROOM_Z * 2 / 120]);
  const m = new THREE.Mesh(new THREE.PlaneGeometry(u(ROOM_X * 2), u(ROOM_Z * 2)), toonTex(tex)); m.rotation.x = -Math.PI / 2; m.position.y = u(-DECK); return m;
}

// ================= ringue: lona com o logo, saia de toldo em gomos, postes rosa/verde-água, cordas, escadinhas
function ringue() {
  const g = new THREE.Group(), E = HALF + 30;
  const lona = canvasTex(1024, 1024, (c, w) => { const k = w / (2 * E), P = v => (v + E) * k;
    c.fillStyle = C.cream; c.fillRect(0, 0, w, w); c.strokeStyle = C.pinkL; c.lineWidth = 6 * k; c.strokeRect(P(-HALF + 8), P(-HALF + 8), (2 * HALF - 16) * k, (2 * HALF - 16) * k);
    c.lineWidth = 9 * k; c.beginPath(); c.ellipse(P(0), P(0), 125 * k, 125 * k, 0, 0, 7); c.stroke();
    // casquinha: cone com a grade e a bola rosa
    c.fillStyle = C.cone; c.beginPath(); c.moveTo(P(-48), P(-4)); c.lineTo(P(48), P(-4)); c.lineTo(P(0), P(80)); c.closePath(); c.fill();
    c.strokeStyle = C.coneD; c.lineWidth = 2.5 * k; for (let i = -3; i <= 3; i++) { c.beginPath(); c.moveTo(P(i * 16 - 30), P(-4)); c.lineTo(P(i * 16 + 30), P(80)); c.moveTo(P(i * 16 + 30), P(-4)); c.lineTo(P(i * 16 - 30), P(80)); c.stroke(); }
    c.globalCompositeOperation = 'destination-over'; c.globalCompositeOperation = 'source-over';
    c.fillStyle = C.pink; c.beginPath(); c.ellipse(P(0), P(-14), 56 * k, 36 * k, 0, 0, 7); c.fill();
    c.fillStyle = C.pinkL; c.beginPath(); c.ellipse(P(-14), P(-26), 20 * k, 10 * k, -0.3, 0, 7); c.fill(); });
  const top = new THREE.Mesh(new THREE.BoxGeometry(u(2 * E), u(8), u(2 * E)), [flat(C.cream), flat(C.cream), toonTex(lona), flat(C.cream), flat(C.cream), flat(C.cream)]);
  top.position.y = u(-4); g.add(ol(top, 0.004));
  // saia: gomos de toldo (painel reto + meia-lua embaixo), alternando rosa e creme
  const GW = 46, N = Math.round(2 * E / GW);
  for (let s = 0; s < 4; s++) { const grp = new THREE.Group(); grp.rotation.y = s * Math.PI / 2;
    for (let i = 0; i < N; i++) { const x = -E + GW * (i + 0.5), col = i % 2 ? C.cream : C.pink, sh = new THREE.Shape();
      sh.moveTo(u(-GW / 2), u(0)); sh.lineTo(u(GW / 2), u(0)); sh.lineTo(u(GW / 2), u(-24)); sh.absarc(0, u(-24), u(GW / 2), 0, -Math.PI, true); sh.lineTo(u(-GW / 2), u(0));
      const m = new THREE.Mesh(new THREE.ExtrudeGeometry(sh, { depth: u(4), bevelEnabled: false, curveSegments: 10 }), flat(col)); m.position.set(u(x), u(-8), u(E)); grp.add(m); }
    grp.add(box(2 * E, DECK - 8, 4, 0, -8 - (DECK - 8) / 2, E - 3, C.pinkD, 1)); g.add(grp); }
  // postes quadrados: rosa nos cantos de trás-esquerda/frente-esquerda, verde-água nos outros (como no concept)
  const corners = [[-HALF, HALF, C.pink], [HALF, HALF, C.teal], [HALF, -HALF, C.pink], [-HALF, -HALF, C.teal]];
  for (const [x, z, col] of corners) {
    g.add(ol(box(30, POST_TOP, 30, x, POST_TOP / 2, z, col, 4), 0.006)); g.add(ol(box(36, 12, 36, x, 6, z, col, 3), 0.005));
    g.add(ol(ball(14, x, POST_TOP + 12, z, C.ball), 0.005));
    for (const h of ROPES) { const sx = -Math.sign(x), sz = -Math.sign(z);   // tampões das cordas (cilindrinhos creme saindo para dentro)
      for (const [dx, dz] of [[sx, 0], [0, sz]]) { const t = cyl(7, 7, 22, x + dx * 22, h, z + dz * 22, C.ball, 14); t.rotation.set(dz ? Math.PI / 2 : 0, 0, dx ? Math.PI / 2 : 0); g.add(ol(t, 0.004)); } } }
  for (let i = 0; i < 4; i++) { const [ax, az] = corners[i], [bx, bz] = corners[(i + 1) % 4];
    ROPES.forEach((h, k) => { const a = new THREE.Vector3(u(ax), u(h), u(az)), b = new THREE.Vector3(u(bx), u(h), u(bz)), len = a.distanceTo(b);
      const cg = new THREE.CylinderGeometry(u(4), u(4), len, 10); cg.rotateZ(Math.PI / 2);
      const m = new THREE.Mesh(cg, flat(k === 1 ? C.cream : C.pink)); m.position.copy(a).add(b).multiplyScalar(0.5); m.rotation.y = -Math.atan2(b.z - a.z, b.x - a.x); g.add(ol(m, 0.004)); }); }
  // escadinhas nos dois cantos da frente (rosa à esquerda, verde-água à direita)
  for (const [s, col] of [[-1, C.pink], [1, C.teal]]) for (let k = 0; k < 3; k++)
    g.add(ol(box(70, DECK / 3 * (3 - k), 34, s * (E - 35), -DECK + DECK / 3 * (3 - k) / 2, E + 20 + k * 30, col, 2), 0.005));
  return g;
}

// ================= coluna creme com a calda de chocolate escorrendo do capitel (gotas compridas e curtas)
function coluna(x, z) {
  const g = new THREE.Group(), H = WALL_H - 60;
  g.add(ol(cyl(46, 52, H, 0, H / 2, 0, C.cream, 28), 0.006));
  g.add(ol(cyl(62, 62, 26, 0, H - 13, 0, C.choco, 28), 0.006));
  for (let i = 0; i < 12; i++) { const a = i / 12 * Math.PI * 2, L = 60 + (i % 3) * 55 + (i % 2) * 30;
    const d = new THREE.Mesh(new THREE.CapsuleGeometry(u(11), u(L), 4, 10), flat(C.choco)); d.position.set(u(Math.cos(a) * 48), u(H - 26 - L / 2), u(Math.sin(a) * 48)); g.add(d); }
  g.add(ol(cyl(60, 60, 30, 0, 15, 0, C.creamD, 28), 0.006));
  g.position.set(u(x), u(-DECK), u(z)); return g;
}

// ================= parede de janelões em arco com a cidade lá fora (painel pintado) e a moldura creme
function cidadeTex() {
  return canvasTex(1024, 512, (c, w, h) => {
    const sky = c.createLinearGradient(0, 0, 0, h); sky.addColorStop(0, '#5fa8e8'); sky.addColorStop(1, '#bfe3f7'); c.fillStyle = sky; c.fillRect(0, 0, w, h);
    c.fillStyle = '#ffffff'; for (const [x, y, r] of [[120, 90, 40], [170, 80, 50], [600, 70, 38], [650, 60, 46], [880, 110, 34]]) { c.beginPath(); c.arc(x, y, r, 0, 7); c.fill(); }
    let s = 7; const rnd = () => (s = (s * 9301 + 49297) % 233280) / 233280;
    for (const [col, base, hmin, hmax] of [['#b9a4d8', 330, 90, 220], ['#9b84c8', 380, 60, 170], ['#c99ac8', 430, 30, 110]])
      for (let x = -20; x < w; ) { const bw = 40 + rnd() * 70, bh = hmin + rnd() * (hmax - hmin); c.fillStyle = col; c.fillRect(x, base - bh, bw, h - base + bh);
        if (rnd() < 0.3) { c.beginPath(); c.arc(x + bw / 2, base - bh, bw / 2.4, Math.PI, 0); c.fill(); }
        c.fillStyle = 'rgba(255,255,255,0.35)'; for (let wy = base - bh + 12; wy < base; wy += 18) for (let wx = x + 8; wx < x + bw - 8; wx += 14) c.fillRect(wx, wy, 6, 8);
        x += bw + 4; }
    c.fillStyle = '#7fb06a'; for (let x = 0; x < w; x += 46) { c.beginPath(); c.arc(x + 20, 450, 26, 0, 7); c.fill(); }
  });
}
function parede(len, rotY, x, z, tex) {
  const g = new THREE.Group(), N = Math.round(len / 330), W = len / N;
  if (tex) { const back = new THREE.Mesh(new THREE.PlaneGeometry(u(len), u(WALL_H - 120)), new THREE.MeshBasicMaterial({ map: tex })); back.position.set(0, u(-DECK + 70 + (WALL_H - 120) / 2), u(-30)); g.add(back); }   // sem tex: janela aberta para a paisagem 3D lá fora
  // moldura: mureta embaixo, pilares entre os arcos, arcos e a faixa rosa no alto
  g.add(ol(box(len, 120, 32, 0, -DECK + 60, 0, C.pinkL, 3), 0.006));   // mais fina que os pilares (sem faces coincidentes)
  g.add(ol(box(len, 14, 50, 0, -DECK + 124, 0, C.cream, 3), 0.005));
  for (let i = 0; i <= N; i++) g.add(ol(box(46, WALL_H, 40, -len / 2 + i * W, -DECK + WALL_H / 2, 0, C.cream, 4), 0.006));
  for (let i = 0; i < N; i++) { const cx = -len / 2 + W * (i + 0.5), sh = new THREE.Shape(), R = (W - 46) / 2, y0 = WALL_H - 140 - R;
    sh.moveTo(u(-W / 2), u(WALL_H)); sh.lineTo(u(W / 2), u(WALL_H)); sh.lineTo(u(W / 2), u(y0)); sh.absarc(0, u(y0), u(R), 0, Math.PI, false); sh.lineTo(u(-W / 2), u(WALL_H));
    const m = new THREE.Mesh(new THREE.ExtrudeGeometry(sh, { depth: u(28), bevelEnabled: false, curveSegments: 24 }), flat(i % 2 ? C.pinkL : C.cream)); m.position.set(u(cx), u(-DECK), u(-14)); g.add(ol(m, 0.005));
    for (let k = 1; k < 3; k++) g.add(box(6, WALL_H - 260, 6, cx - R + k * R * 2 / 3, -DECK + 130 + (WALL_H - 260) / 2, -8, C.cream, 1)); }   // caixilhos
  g.add(ol(box(len, 40, 56, 0, -DECK + WALL_H, 0, C.pink, 4), 0.006));
  g.rotation.y = rotY; g.position.set(u(x), 0, u(z)); return g;
}

// ================= balcão do fundo: base verde-água com frisos rosa, tampo, toldo listrado de gomos, máquinas
function balcaoFundo(z) {
  const g = new THREE.Group(), W = 640, T0 = -DECK + 112, glassM = () => new THREE.MeshToonMaterial({ color: C.glass, transparent: true, opacity: 0.35, gradientMap: flat('#fff').gradientMap });
  // balcão: rodapé rosa escuro, frente em painéis almofadados verde-água com friso creme, tampo rosa com borda creme
  g.add(ol(box(W, 14, 76, 0, -DECK + 7, 4, C.pinkD, 3), 0.005));
  g.add(ol(box(W, 90, 80, 0, -DECK + 59, 0, C.teal, 4), 0.006));
  const NP = 8, PW = W / NP;
  for (let i = 0; i < NP; i++) { const x = -W / 2 + PW * (i + 0.5); g.add(ol(box(PW - 18, 56, 6, x, -DECK + 58, 41, C.tealD, 4), 0.004)); g.add(box(PW - 30, 44, 4, x, -DECK + 58, 44, C.teal, 3)); }
  for (const y of [-DECK + 22, -DECK + 94]) g.add(box(W + 2, 6, 84, 0, y, 0, C.cream, 2));
  g.add(ol(box(W + 30, 10, 96, 0, T0 - 6, 2, C.pinkL, 4), 0.006)); g.add(box(W + 34, 4, 100, 0, T0 - 12, 2, C.cream, 2));
  // parede de trás: azulejo verde-água (textura), duas prateleiras com potes e taças
  const tile = canvasTex(128, 128, (c, w) => { c.fillStyle = C.tealD; c.fillRect(0, 0, w, w); c.fillStyle = C.teal; c.fillRect(3, 3, w / 2 - 6, w / 2 - 6); c.fillRect(w / 2 + 3, w / 2 + 3, w / 2 - 6, w / 2 - 6); c.fillStyle = '#56b39f'; c.fillRect(w / 2 + 3, 3, w / 2 - 6, w / 2 - 6); c.fillRect(3, w / 2 + 3, w / 2 - 6, w / 2 - 6); }, [14, 5]);
  const back = new THREE.Mesh(new THREE.BoxGeometry(u(W - 40), u(230), u(20)), [flat(C.tealD), flat(C.tealD), flat(C.tealD), flat(C.tealD), toonTex(tile), flat(C.tealD)]); back.position.set(0, u(-DECK + 220), u(-75)); g.add(ol(back, 0.006));
  const taca = (x, y, z) => { const t = new THREE.Mesh(new THREE.LatheGeometry([[1.5, 0], [1.5, 10], [4, 12], [10, 22], [11, 24]].map(([r, h]) => new THREE.Vector2(u(r), u(h))), 14), flat(C.pinkL)); t.position.set(u(x), u(y), u(z)); return ol(t, 0.003); };
  for (const [x0, x1] of [[-280, -100], [-80, 150]]) for (const y of [-DECK + 210, -DECK + 260]) {
    g.add(ol(box(x1 - x0, 6, 26, (x0 + x1) / 2, y, -56, C.cream, 2), 0.004));
    for (let x = x0 + 14, k = 0; x < x1 - 10; x += 26, k++) { const col = [C.pink, '#f3e0a0', '#8fd8c8', C.chocoL][(k + (y > -DECK + 230 ? 1 : 0)) % 4];
      if (k % 2) { g.add(ol(cyl(9, 9, 22, x, y + 14, -56, glassM(), 12), 0.003)); g.add(cyl(8, 8, 12, x, y + 9, -56, col, 12)); g.add(cyl(9.5, 9.5, 5, x, y + 26, -56, C.cream, 12)); }   // pote de cobertura
      else g.add(taca(x, y + 3, -56)); } }
  // toldo listrado inclinado com os gomos, a barra e os dois pés
  const T = 26, n = Math.round(W / T);
  for (let i = 0; i < n; i++) { const x = -W / 2 + T * (i + 0.5), col = i % 2 ? C.cream : C.pink;
    const st = box(T, 4, 110, x, -DECK + 360, -30, col, 1); st.rotation.x = 0.35; g.add(st);
    const sh = new THREE.Shape(); sh.moveTo(u(-T / 2), 0); sh.lineTo(u(T / 2), 0); sh.lineTo(u(T / 2), u(-14)); sh.absarc(0, u(-14), u(T / 2), 0, -Math.PI, true);
    const gm = new THREE.Mesh(new THREE.ExtrudeGeometry(sh, { depth: u(3), bevelEnabled: false, curveSegments: 8 }), flat(col)); gm.position.set(u(x), u(-DECK + 342), u(22)); g.add(gm); }
  g.add(ol(box(W + 10, 8, 8, 0, -DECK + 342, 24, C.cream, 2), 0.004));
  for (const x of [-W / 2 + 6, W / 2 - 6]) g.add(ol(box(10, 230, 10, x, T0 + 115, 22, C.cream, 2), 0.004));
  // máquina de espresso: corpo verde-água, frente cromada, dois grupos com porta-filtro e xícara, manômetros, vaporizador, xícaras em cima
  { const X = -220, Y = T0;
    g.add(ol(box(110, 72, 56, X, Y + 36, -16, C.teal, 10), 0.005)); g.add(ol(box(100, 30, 4, X, Y + 50, 13, C.metal, 3), 0.004));
    g.add(ol(box(116, 8, 60, X, Y + 76, -16, C.metal, 3), 0.004));
    for (const dx of [-26, 26]) { g.add(ol(cyl(9, 9, 10, X + dx, Y + 32, 18, C.metal, 14), 0.003)); g.add(box(30, 5, 5, X + dx, Y + 26, 34, '#2a2222', 2));
      g.add(ol(cyl(6, 5, 9, X + dx, Y + 5, 20, C.cream, 12), 0.003)); g.add(box(26, 3, 18, X + dx, Y + 1.5, 20, C.metal, 1)); }
    for (const dx of [-14, 14]) { const m = cyl(9, 9, 3, X + dx, Y + 62, 16, C.cream, 16); m.rotation.x = Math.PI / 2; g.add(ol(m, 0.003)); const p = box(1.5, 7, 1, X + dx, Y + 63, 18, '#c0283a', 0.5); p.rotation.z = 0.6; g.add(p); }
    const w = cyl(2, 2, 40, X + 58, Y + 30, 10, C.metal, 8); w.rotation.z = 0.2; g.add(w);
    for (let k = 0; k < 4; k++) g.add(ol(cyl(6, 5, 8, X - 36 + k * 24, Y + 84, -16, k % 2 ? C.pinkL : C.cream, 12), 0.003)); }
  // dispensers de calda: base de inox com as torneiras, jarros de vidro com a calda dentro e a bomba em cima
  { const Y = T0; g.add(ol(box(140, 26, 50, -10, Y + 13, -14, C.metal, 4), 0.005)); g.add(box(150, 4, 30, -10, Y + 2, 14, C.metal, 1));
    ['#7a3a20', '#e0a040', '#c0283a'].forEach((col, i) => { const x = -50 + i * 40;
      g.add(ol(cyl(15, 15, 54, x, Y + 53, -14, glassM(), 18), 0.004)); g.add(cyl(13, 13, 40, x, Y + 46, -14, col, 18));
      g.add(ol(cyl(16, 16, 8, x, Y + 84, -14, C.metal, 18), 0.003)); g.add(ol(cyl(3, 3, 18, x, Y + 96, -14, C.metal, 8), 0.003)); g.add(ol(box(18, 4, 6, x + 6, Y + 105, -14, C.metal, 1.5), 0.003));
      g.add(ol(cyl(4, 3, 14, x, Y + 13, 14, C.metal, 8), 0.003)); }); }
  // copos empilhados, casquinhas no suporte, porta-guardanapo, caixa registradora e taças de sundae prontas
  for (const x of [-140, 90]) for (let k = 0; k < 4; k++) g.add(ol(cyl(9, 7, 12, x, T0 + 6 + k * 9, 0, k % 2 ? C.cream : C.pinkL, 14), 0.003));
  g.add(ol(box(50, 10, 20, 135, T0 + 5, 12, C.pinkL, 2), 0.003));
  for (let k = 0; k < 3; k++) { const cg = new THREE.ConeGeometry(u(6), u(20), 10); cg.rotateX(Math.PI); const c = new THREE.Mesh(cg, flat(C.cone)); c.position.set(u(120 + k * 15), u(T0 + 18), u(12)); g.add(ol(c, 0.003)); }
  g.add(ol(box(28, 22, 14, -110, T0 + 11, 22, C.metal, 3), 0.003)); g.add(box(22, 14, 10, -110, T0 + 15, 22, C.cream, 1));
  { const X = -290; g.add(ol(box(46, 30, 40, X, T0 + 15, 0, C.pink, 5), 0.004)); const tp = box(40, 4, 22, X, T0 + 34, 6, C.cream, 1.5); tp.rotation.x = -0.4; g.add(ol(tp, 0.003)); g.add(ol(box(30, 18, 4, X, T0 + 42, -16, '#3a2a2a', 2), 0.003)); }
  for (const x of [40, 70]) { g.add(taca(x, T0, 22)); g.add(ol(ball(9, x, T0 + 27, 22, C.cream), 0.003)); g.add(ball(3.5, x, T0 + 37, 22, '#d0203a')); }
  // geladeira de bolos: corpo rosa, interior creme, prateleiras com bolos, porta de vidro e puxador
  { const X = 230, Y = T0;
    g.add(ol(box(130, 190, 64, X, Y + 95, -28, C.pinkL, 8), 0.006)); g.add(ol(box(136, 12, 70, X, Y + 196, -28, C.pink, 4), 0.004));
    g.add(box(110, 150, 50, X, Y + 92, -24, C.cream, 4));
    for (let k = 0; k < 3; k++) { const y = Y + 30 + k * 50; g.add(box(106, 3, 46, X, y, -18, C.metal, 1));
      for (let j = 0; j < 3; j++) { const x = X - 34 + j * 34, col = [C.pink, C.chocoL, '#f3e0a0'][(k + j) % 3];
        g.add(ol(cyl(13, 13, 14, x, y + 9, -10, col, 16), 0.003)); g.add(cyl(13.5, 13.5, 3, x, y + 17, -10, C.cream, 16)); g.add(ball(3, x, y + 21, -10, '#d0203a')); } }
    g.add(box(112, 154, 3, X, Y + 92, 6, glassM(), 1)); g.add(ol(box(5, 50, 5, X + 48, Y + 100, 10, C.metal, 2), 0.003)); }
  // banquinhos na frente do balcão
  for (let k = 0; k < 6; k++) { const x = -250 + k * 100; g.add(cyl(4, 4, 70, x, -DECK + 35, 80, C.metal, 10)); g.add(cyl(18, 18, 4, x, -DECK + 2, 80, C.metal, 16)); g.add(ol(cyl(20, 20, 12, x, -DECK + 74, 80, C.pink, 20), 0.004)); }
  g.position.z = u(z); return g;
}
// ================= balcão-vitrine lateral: base rosa, tampo creme, vitrine de vidro com as cubas de sorvete, doces e banquinhos
function balcaoLado(sx) {
  const g = new THREE.Group(), L = 760, X = sx * 880;
  g.add(ol(box(80, 96, L, X, -DECK + 48, -150, C.pink, 4), 0.006)); g.add(ol(box(96, 10, L + 20, X, -DECK + 101, -150, C.cream, 3), 0.005));
  for (let k = 0; k < 3; k++) { const z = -150 - L / 2 + 130 + k * 250;
    const vid = new THREE.Mesh(new RoundedBoxGeometry(u(70), u(52), u(200), 2, u(5)), new THREE.MeshToonMaterial({ color: C.glass, transparent: true, opacity: 0.45, gradientMap: flat('#fff').gradientMap }));
    vid.position.set(u(X), u(-DECK + 132), u(z)); g.add(vid);
    for (let j = 0; j < 5; j++) { const col = ['#f5b8c8', '#6a3a2a', '#f3e0a0', '#8fd8c8', C.pink][(j + k) % 5];
      g.add(box(56, 10, 34, X, -DECK + 112, z - 80 + j * 40, C.metal, 1)); g.add(ol(box(50, 12, 30, X, -DECK + 120, z - 80 + j * 40, col, 5), 0.003)); } }
  // doces no tampo (cupcakes) e campânulas
  for (let k = 0; k < 6; k++) { const z = -150 - L / 2 + 60 + k * 130;
    g.add(cyl(10, 7, 12, X + sx * 10, -DECK + 112, z, C.chocoL, 12)); g.add(ol(ball(11, X + sx * 10, -DECK + 124, z, k % 2 ? C.pinkL : C.cream), 0.003)); g.add(ball(4, X + sx * 10, -DECK + 136, z, '#d0203a')); }
  // banquinhos do lado do ringue
  for (let k = 0; k < 6; k++) { const z = -150 - L / 2 + 70 + k * 125, x = X - sx * 90;
    g.add(cyl(4, 4, 70, x, -DECK + 35, z, C.metal, 10)); g.add(cyl(18, 18, 4, x, -DECK + 2, z, C.metal, 16)); g.add(ol(cyl(20, 20, 12, x, -DECK + 74, z, C.pink, 20), 0.004)); }
  return g;
}

// ================= estátuas gigantes: casquinha de três bolas e taça de sundae, ambas com a cereja e o canudo de wafer
function casquinha() {
  const g = new THREE.Group(), cg = new THREE.ConeGeometry(u(70), u(170), 16); cg.rotateX(Math.PI); const c = new THREE.Mesh(cg, flat(C.cone)); c.position.y = u(85); g.add(ol(c, 0.006));
  g.add(ol(cyl(76, 74, 22, 0, 172, 0, C.coneD, 24), 0.005));
  [[0, 210, 0, 62, '#6a3a2a'], [-34, 250, 10, 50, '#8fd8c8'], [34, 252, -6, 52, C.pinkL], [0, 300, 0, 50, C.cream]].forEach(([x, y, z, r, col]) => g.add(ol(ball(r, x, y, z, col), 0.006)));
  g.add(ol(ball(16, 0, 360, 0, '#d0203a'), 0.004)); const w = cyl(7, 7, 110, -30, 360, 0, '#e8b070', 10); w.rotation.z = 0.35; g.add(ol(w, 0.004));
  return g;
}
function sundae() {
  const g = new THREE.Group();
  g.add(ol(cyl(60, 70, 20, 0, 10, 0, C.cream, 24), 0.005)); g.add(ol(cyl(14, 18, 80, 0, 60, 0, C.cream, 16), 0.005));
  const pts = []; for (let i = 0; i <= 10; i++) { const t = i / 10; pts.push(new THREE.Vector2(u(20 + 70 * Math.sin(t * 1.4)), u(t * 130))); }
  const taca = new THREE.Mesh(new THREE.LatheGeometry(pts, 28), flat(C.pinkL)); taca.position.y = u(100); g.add(ol(taca, 0.006));
  g.add(ol(cyl(84, 84, 30, 0, 240, 0, C.choco, 28), 0.006));
  g.add(ol(ball(70, 0, 270, 0, C.cream), 0.006)); g.add(ol(ball(48, 0, 320, 0, C.cream), 0.006));
  g.add(ol(ball(16, 0, 372, 0, '#d0203a'), 0.004)); const w = cyl(7, 7, 110, 34, 360, 0, '#e8b070', 10); w.rotation.z = -0.35; g.add(ol(w, 0.004));
  return g;
}
// ================= vaso verde-água com topiaria (cone de copa arredondada)
function vaso(x, z, y = -DECK) {
  const g = new THREE.Group(); g.add(ol(cyl(26, 20, 44, 0, 22, 0, C.pot, 16), 0.005));
  const t = new THREE.Mesh(new THREE.CapsuleGeometry(u(26), u(50), 6, 14), flat(C.leaf)); t.position.y = u(100); t.userData.anim = { k: 'sway', a: 0.018 }; g.add(ol(t, 0.005));
  g.position.set(u(x), u(y), u(z)); return g;
}
// ================= luminária pendente: fio, cúpula rosa em meia-esfera e a lâmpada creme
function luminaria(x, z, y = 420) {
  const g = new THREE.Group(); g.add(cyl(1.5, 1.5, WALL_H - y, 0, (WALL_H - y) / 2, 0, '#5a4a4a', 6));
  const cp = new THREE.Mesh(new THREE.SphereGeometry(u(40), 24, 12, 0, Math.PI * 2, 0, Math.PI / 2), flat(C.pink)); g.add(ol(cp, 0.005));
  g.add(ball(22, 0, -6, 0, '#fff4dc')); g.position.set(u(x), u(y - DECK), u(z)); g.userData.anim = { k: 'sway', a: 0.025, hang: true }; return g;
}

// ================= Paris lá fora (do lado da câmera): a torre Eiffel modelada nas proporções reais (base quadrada de
// 125 m, quatro pernas em curva que se juntam no 2º andar a 116 m, fuste afinando até 276 m, antena até 330 m; arcos
// entre as pernas, 1º andar a 57 m, treliça em X nas faces), longe da janela; atrás dela a cidade pintada e o gramado
function torreEiffel(H) {
  const k = u(H) / 330, beams = [], up = new THREE.Vector3(0, 1, 0);
  const beam = (a, b, r) => { const A = new THREE.Vector3(...a).multiplyScalar(k), B = new THREE.Vector3(...b).multiplyScalar(k), L = A.distanceTo(B); if (L < 1e-6) return;
    const g = new THREE.BoxGeometry(r * k * 2, L, r * k * 2); g.applyQuaternion(new THREE.Quaternion().setFromUnitVectors(up, B.clone().sub(A).normalize())); g.translate((A.x + B.x) / 2, (A.y + B.y) / 2, (A.z + B.z) / 2); beams.push(g); };
  // meia-largura externa da torre na altura h (m): curva côncava da base ao 2º andar, depois afina reto até o topo
  const half = h => h <= 116 ? 10 + 52.5 * Math.pow(1 - h / 116, 1.7) : 10 - 7 * Math.pow((h - 116) / 160, 0.9);
  const legW = h => h <= 116 ? 4 + 14 * Math.pow(1 - h / 116, 1.3) : 2.2;   // largura de cada perna
  const C4 = [[1, 1], [-1, 1], [-1, -1], [1, -1]], hs = [];
  for (let h = 0; h <= 276; h += h < 116 ? 6 : 10) hs.push(Math.min(h, 276));
  // pernas: em cada canto, os dois banzos (externo e interno) da perna em treliça
  for (const [sx, sz] of C4) for (let i = 0; i < hs.length - 1; i++) { const h0 = hs[i], h1 = hs[i + 1];
    const o0 = half(h0), o1 = half(h1), n0 = Math.max(o0 - legW(h0), 0.5), n1 = Math.max(o1 - legW(h1), 0.5);
    beam([sx * o0, h0, sz * o0], [sx * o1, h1, sz * o1], 1.4);
    if (h1 <= 116) { beam([sx * n0, h0, sz * o0], [sx * n1, h1, sz * o1], 0.9); beam([sx * o0, h0, sz * n0], [sx * o1, h1, sz * n1], 0.9);
      beam([sx * o0, h0, sz * o0], [sx * n1, h1, sz * o1], 0.4); beam([sx * o0, h0, sz * o0], [sx * o1, h1, sz * n1], 0.4); } }   // diagonais da perna
  // fuste acima do 2º andar: treliça em X nas quatro faces
  for (let i = 0; i < hs.length - 1; i++) { const h0 = hs[i], h1 = hs[i + 1]; if (h0 < 116) continue; const a = half(h0), b = half(h1);
    for (let f = 0; f < 4; f++) { const [x0, z0] = C4[f], [x1, z1] = C4[(f + 1) % 4];
      beam([x0 * a, h0, z0 * a], [x1 * b, h1, z1 * b], 0.35); beam([x1 * a, h0, z1 * a], [x0 * b, h1, z0 * b], 0.35); beam([x0 * a, h0, z0 * a], [x1 * a, h0, z1 * a], 0.5); } }
  // arcos entre as pernas na base (decorativos, como os de verdade) e a cinta entre as pernas até o 1º andar
  for (let f = 0; f < 4; f++) { const [x0, z0] = C4[f], [x1, z1] = C4[(f + 1) % 4], P = [];
    for (let i = 0; i <= 16; i++) { const t = i / 16, h = 18 + 21 * Math.sin(t * Math.PI), w = half(h) - legW(h) * 0.5, s = -1 + 2 * t;
      const ex = x0 === x1 ? x0 * w : s * w * (x1 > x0 ? 1 : -1), ez = z0 === z1 ? z0 * w : s * w * (z1 > z0 ? 1 : -1); P.push([ex, h, ez]); }
    for (let i = 0; i < 16; i++) beam(P[i], P[i + 1], 1.1);
    for (const h of [57, 50]) { const w = half(h); beam([x0 * w, h, z0 * w], [x1 * w, h, z1 * w], h === 57 ? 1.6 : 0.8); }
    for (let i = 0; i < 6; i++) { const t0 = i / 6, t1 = (i + 1) / 6, w = half(50), y0 = 50, y1 = 57, X = t => x0 * w + (x1 - x0) * w * t, Z = t => z0 * w + (z1 - z0) * w * t;
      beam([X(t0), y0, Z(t0)], [X(t1), y1, Z(t1)], 0.4); beam([X(t1), y0, Z(t1)], [X(t0), y1, Z(t0)], 0.4); } }
  const g = new THREE.Group(), m = flat('#7d6074'); m.fog = false;
  g.add(new THREE.Mesh(mergeGeometries(beams), m));
  // andares: plataforma do 1º (com a galeria), do 2º, cabine do topo, antena
  const plat = (h, w, t) => { const b = new THREE.Mesh(new THREE.BoxGeometry(w * 2 * k, t * k, w * 2 * k), m); b.position.y = h * k; g.add(b); };
  plat(57, half(57) + 3, 4); plat(64, half(64) + 1, 3); plat(116, half(116) + 4, 3); plat(122, half(122) + 2, 2.5); plat(276, 6, 6); plat(283, 4.5, 8);
  const ant = new THREE.Mesh(new THREE.CylinderGeometry(0.6 * k, 1.4 * k, 44 * k, 8), m); ant.position.y = 309 * k; g.add(ant);
  return g;
}
function paris(cityTex) {
  const g = new THREE.Group(), Z = ROOM_Z;
  const chao = new THREE.Mesh(new THREE.PlaneGeometry(u(12000), u(8000)), flat('#9cc98a')); chao.rotation.x = -Math.PI / 2; chao.position.set(0, u(-DECK - 2), u(Z + 4000)); g.add(chao);
  // fundo: céu azul alto com a cidade baixinha na faixa de baixo (prédios na altura dos das outras janelas)
  const pt = canvasTex(2048, 1024, (c, w, h) => { const sky = c.createLinearGradient(0, 0, 0, h); sky.addColorStop(0, '#4f9be6'); sky.addColorStop(1, '#bfe3f7'); c.fillStyle = sky; c.fillRect(0, 0, w, h);
    let sd = 11; const rnd = () => (sd = (sd * 9301 + 49297) % 233280) / 233280;
    // nuvens fofas: grupos de bolas brancas com a base reta e um sombreado azulado embaixo
    for (let n = 0; n < 22; n++) { const cx = rnd() * w, cy = 720 + rnd() * 150, sc = 0.6 + rnd() * 0.9, k = 4 + Math.floor(rnd() * 4);
      for (const [col, dy] of [['#d6e9f8', 10], ['#ffffff', 0]]) { c.fillStyle = col; for (let i = 0; i < k; i++) { const x = cx + (i - k / 2) * 34 * sc, r = (26 + rnd() * 26) * sc; c.beginPath(); c.arc(x, cy + dy - r * 0.4, r, 0, 7); c.fill(); }
        c.fillRect(cx - k / 2 * 34 * sc - 20 * sc, cy + dy - 16 * sc, k * 34 * sc + 20 * sc, 16 * sc); } }
    // prédios em três planos, alturas bem variadas (uns baixinhos, umas torres), com janelinhas e cúpulas
    for (const [col, maxH, minH] of [['#b9a4d8', 0.12, 0.035], ['#a48ccc', 0.085, 0.02], ['#c99ac8', 0.05, 0.012]])
      for (let x = -20; x < w; ) { const bw = 30 + rnd() * 60, r = rnd(), bh = h * (minH + (maxH - minH) * (r < 0.15 ? 1 : r * r)), top = h - bh;
        c.fillStyle = col; c.fillRect(x, top, bw, bh);
        if (rnd() < 0.2) { c.beginPath(); c.arc(x + bw / 2, top, bw / 2.4, Math.PI, 0); c.fill(); } else if (rnd() < 0.15) c.fillRect(x + bw / 2 - 2, top - 30, 4, 30);
        c.fillStyle = 'rgba(255,255,255,0.35)'; for (let wy = top + 10; wy < h - 8; wy += 16) for (let wx = x + 7; wx < x + bw - 7; wx += 12) c.fillRect(wx, wy, 5, 7);
        x += bw + 3 + rnd() * 10; }
  });
  const pano = new THREE.Mesh(new THREE.PlaneGeometry(u(7000), u(4000)), new THREE.MeshBasicMaterial({ map: pt, fog: false })); pano.position.set(0, u(-DECK - 60 + 2000), u(Z + 1700)); pano.rotation.y = Math.PI; g.add(pano);
  const t = torreEiffel(1150); t.position.set(u(-250), u(-DECK), u(Z + 1100)); t.rotation.y = 0.5; g.add(t);
  return g;
}

export async function buildCremosoArena() {
  const all = new THREE.Group(), city = cidadeTex();
  all.add(piso(), ringue());
  all.userData.look = { indoor: true, tint: [1.05, 0.98, 1.0], horizon: '#f7d6dc', zenith: '#f1b9c4', fog: [140, 420],
    sun: { color: '#fff1e6', int: 1.8, pos: [1.2, 3.6, 2.4] }, hemi: { sky: '#fff0f2', ground: '#d890a0', int: 1.3 } };
  // paredes de janelões nos quatro lados e o forro rosa claro
  all.add(parede(ROOM_X * 2, 0, 0, -ROOM_Z, city), parede(ROOM_Z * 2, Math.PI / 2, -ROOM_X, 0, city), parede(ROOM_Z * 2, -Math.PI / 2, ROOM_X, 0, city), parede(ROOM_X * 2, Math.PI, 0, ROOM_Z, null));
  const teto = new THREE.Mesh(new THREE.PlaneGeometry(u(ROOM_X * 2), u(ROOM_Z * 2)), flat(C.cream)); teto.rotation.x = Math.PI / 2; teto.position.y = u(WALL_H - DECK + 20); all.add(teto);
  // colunas de chocolate e luminárias
  for (const [x, z] of [[-620, -720], [620, -720], [-620, 720], [620, 720], [-1000, -300], [1000, -300], [-1000, 400], [1000, 400]]) all.add(coluna(x, z));
  for (const x of [-420, -160, 160, 420]) all.add(luminaria(x, -560));
  for (const z of [-200, 300]) for (const x of [-700, 700]) all.add(luminaria(x, z, 440));
  all.add(balcaoFundo(-820), balcaoLado(-1), balcaoLado(1));
  // estátuas gigantes nos cantos do fundo, sobre pedestal rosa
  for (const [x, f] of [[-960, casquinha], [960, sundae]]) { const s = f(); s.position.set(u(x), u(-DECK + 90), u(-960)); s.scale.setScalar(1.15); all.add(s); all.add(ol(box(220, 90, 220, x, -DECK + 45, -960, C.pink, 6), 0.006)); }
  // topiarias
  for (const [x, z] of [[-470, -800], [470, -800], [-760, -650], [760, -650], [-760, 500], [760, 500], [-400, 900], [400, 900]]) all.add(vaso(x, z));
  all.add(paris(city));
  return all;
}
