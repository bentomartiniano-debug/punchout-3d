// King Frog no modo vinil, modelado em código com o modelo 3D dele (assets3d/frog/clean/body_frog.glb) como molde:
// as medidas saíram de fatias do modelo do Meshy a cada 2 cm (largura, costas e frente de cada altura, sem os braços),
// alisadas e espelhadas. Superfície lisa, cores chapadas, sem braços (como no jogo: só as luvas flutuam), cinturão de
// campeão com fivela, short, pernas, pés de três dedos, coroa de cinco pontas e o rosto 2.5D do jogo pintado na frente.
// Medidas em cm; px do quadro = cm (S = 100), FOOT = 200: a altura h vira y = 200 - h.
import { THREE, ss, ramp, INK, INKS, flat, withOutline, legPlacer } from './core.js';
import { RoundedBoxGeometry } from 'three/addons/geometries/RoundedBoxGeometry.js';
import { meshyLower } from './frog_lower.js';
import { VALERIA, bodyOpts, valeriaBody } from './base.js';

const GREEN = '#6f9a4e', BELLY = '#a8c97f', SHORT = '#33572f', GOLD = '#e5cc5b', ORANGE = '#d17b24', GEM = '#c8323c', GLOVE = '#41643c', CUFF = '#2b4528';
const hy = h => 200 - h;                  // altura (cm) -> y do quadro
const PW = 2.4;                           // seção levemente quadrada (o sapo é largo e cheio)
const sp = v => Math.sign(v) * Math.pow(Math.abs(v), 2 / PW);

// perfil medido no modelo: altura, meia largura, costas (z), frente (z)
// (barril, como na referência de costas: peito e ombros largos, cintura mais fina no cinturão, topo largo e redondo)
const BODY = [[52, 22, -20, 14], [55, 34, -25, 19], [58, 38, -28, 23], [64, 39.5, -31, 28], [70, 40.5, -33, 31], [76, 41, -34, 34],
  [84, 41.5, -34, 36], [92, 41.5, -34, 36], [100, 41.5, -34, 34], [108, 42, -36, 32], [116, 42.5, -38, 29], [124, 42.5, -40, 26],
  [130, 42.5, -40, 24], [136, 42.5, -40, 26], [141, 42.5, -39, 31], [146, 42, -37, 35], [151, 41, -36, 39], [156, 40, -35, 40],
  [160, 39, -34, 36], [164, 37, -33, 31], [168, 35, -31, 25], [172, 32.5, -28, 21], [175, 29, -23, 18], [177, 25, -17, 14], [178.5, 17, -10, 9]];   // cheio: cintura tão larga quanto o peito, barriga para a frente
// interpolação suave (Catmull-Rom) do perfil
function profile(h) {
  const P = BODY; let i = 0; while (i < P.length - 2 && h > P[i + 1][0]) i++;
  const p0 = P[Math.max(0, i - 1)], p1 = P[i], p2 = P[i + 1], p3 = P[Math.min(P.length - 1, i + 2)];
  const u = Math.max(0, Math.min(1, (h - p1[0]) / (p2[0] - p1[0])));
  const cr = k => { const a = p0[k], b = p1[k], c = p2[k], d = p3[k]; return 0.5 * (2 * b + (-a + c) * u + (2 * a - 5 * b + 4 * c - d) * u * u + (-a + 3 * b - 3 * c + d) * u * u * u); };
  return { a: cr(1), zb: cr(2), zf: cr(3) };
}
// fileira do sólido na altura h, com folga extra (cinturão por cima do corpo)
function row(h, extra = 0, scale = 1, scaleZ = scale) {   // scale: largura; scaleZ: profundidade
  const { a, zb, zf } = profile(h), A = a * scale + extra, cz = (zf + zb) / 2, hd = (zf - zb) / 2 * scaleZ + extra;
  return { y: hy(h), l: -A, r: A, zf: (x, y, z, f) => cz + hd * sp(f) };
}

// --- rosto do King Frog no jogo (os mesmos desenhos do visualizador principal), pintado na frente da cabeça
function gameEye(g, slope, glint = true) {
  g.fillStyle = INKS; g.beginPath(); g.moveTo(14, 34 - slope * 0.2); g.lineTo(114, 34 + slope); g.lineTo(114, 104); g.lineTo(14, 104); g.closePath(); g.fill();
  if (glint) { g.fillStyle = '#ffffff'; g.fillRect(70, 62, 18, 18); }
}
// olho do sapo (como a referência): bloco preto com o topo caindo para dentro (o lado de dentro, do nariz, é x = 128) e
// o fundo redondo, com dois brilhos redondos (o grande embaixo, perto do nariz; o pequeno em cima)
function frogEye(g, slope) {
  g.fillStyle = INKS; g.beginPath(); g.moveTo(10, 36 - slope * 0.3); g.lineTo(120, 36 + slope);
  g.bezierCurveTo(124, 100, 96, 122, 62, 122); g.bezierCurveTo(26, 122, 4, 98, 10, 36 - slope * 0.3); g.closePath(); g.fill();
  g.fillStyle = '#ffffff'; g.beginPath(); g.arc(80, 88, 15, 0, 7); g.fill(); g.beginPath(); g.arc(46, 58 + slope * 0.2, 9, 0, 7); g.fill();
}
const EYES = {
  normal: g => gameEye(g, 22), angry: g => gameEye(g, 46),   // os do modelo 3D antigo (visualizador principal)
  pain: g => { g.lineWidth = 16; g.lineCap = 'round'; g.lineJoin = 'round'; g.strokeStyle = INKS; g.beginPath(); g.moveTo(26, 30); g.lineTo(100, 66); g.lineTo(26, 102); g.stroke(); },
  ko: g => { g.lineWidth = 16; g.lineCap = 'round'; g.strokeStyle = INKS; g.beginPath(); g.moveTo(30, 30); g.lineTo(98, 98); g.moveTo(98, 30); g.lineTo(30, 98); g.stroke(); },
};
const MOUTH = {
  normal: g => { g.lineWidth = 7; g.lineCap = 'round'; g.strokeStyle = INKS; g.beginPath(); g.moveTo(10, 70); g.quadraticCurveTo(64, 56, 118, 70); g.stroke(); },
  angry: g => { g.fillStyle = '#f8f4e6'; g.strokeStyle = INKS; g.lineWidth = 6; g.lineJoin = 'round';
    g.beginPath(); g.moveTo(14, 74); g.quadraticCurveTo(64, 56, 114, 74); g.lineTo(110, 90); g.quadraticCurveTo(64, 76, 18, 90); g.closePath(); g.fill(); g.stroke();
    g.lineWidth = 3; g.beginPath(); g.moveTo(16, 82); g.quadraticCurveTo(64, 66, 112, 82); g.stroke();
    for (let x = 30; x < 104; x += 12) { g.beginPath(); g.moveTo(x, 64 + Math.abs(x - 64) * 0.18); g.lineTo(x, 86 + Math.abs(x - 64) * 0.06); g.stroke(); } },
  pain: g => { g.fillStyle = '#f8f4e6'; g.strokeStyle = INKS; g.lineWidth = 6; g.beginPath(); g.moveTo(12, 70); g.quadraticCurveTo(64, 40, 116, 72); g.lineTo(112, 84); g.quadraticCurveTo(64, 62, 16, 82); g.closePath(); g.fill(); g.stroke(); g.lineWidth = 3; for (let x = 28; x < 110; x += 14) { g.beginPath(); g.moveTo(x, 56); g.lineTo(x, 80); g.stroke(); } },
  ko: g => { g.fillStyle = '#2a0a12'; g.strokeStyle = INKS; g.lineWidth = 6; g.beginPath(); g.ellipse(64, 70, 40, 16, 0, 0, 7); g.fill(); g.stroke(); g.fillStyle = '#ff5c7a'; g.beginPath(); g.ellipse(84, 84, 14, 12, 0, 0, 7); g.fill(); },
};
const nose = g => { g.fillStyle = INKS; g.beginPath(); g.ellipse(34, 64, 13, 9, 0.35, 0, 7); g.fill(); g.beginPath(); g.ellipse(94, 64, 13, 9, -0.35, 0, 7); g.fill(); };
// posições do rosto (as do modelo 3D no visualizador principal): centro x, altura, largura, altura da peça
const FACE = { eyeX: 19.5, eyeH: 169, eyeS: 19.5, noseH: 165, noseS: 16.6, mouthH: 154, mouthW: 62 };
const PR = 8;   // pintura: 8 px por cm (1600 x 1600 para 200 x 200 cm)
function paintFront(c, face, noMouth = false) {
  const g = c.getContext('2d'); g.setTransform(PR, 0, 0, PR, 100 * PR, 0);   // x de -100 a 100, y = 200 - h
  g.fillStyle = GREEN; g.fillRect(-100, 0, 200, 200);
  g.fillStyle = BELLY; g.beginPath(); g.ellipse(0, hy(112), 30, 35, 0, 0, 7); g.fill();   // barriga clara: topo redondo logo abaixo do lábio
  // o papo inteiro no verde claro (o mesmo formato da bolsa: em cima acaba antes, perto da boca)
  { const c = PAPO.c.value, r = PAPO.r.value; g.beginPath(); g.ellipse(0, hy(c.y * 100), r.x * 100, r.y * 100, 0, 0, Math.PI); g.ellipse(0, hy(c.y * 100), r.x * 100, r.y * 100 / 1.5, 0, Math.PI, Math.PI * 2); g.fill(); }
  const at = (draw, cx, h, w, hh, flip = false) => {          // desenho em canvas 128 (rosto do jogo) no lugar dele
    const k = document.createElement('canvas'); k.width = k.height = 256; const kg = k.getContext('2d'); kg.scale(2, 2); draw(kg);
    g.save(); g.translate(cx, hy(h)); g.scale(flip ? -1 : 1, 1); g.drawImage(k, -w / 2, -hh / 2, w, hh); g.restore();
  };
  const F = FACE, E = EYES[face] || EYES.normal, Mo = MOUTH[face] || MOUTH.normal;
  at(nose, 0, F.noseH, F.noseS, F.noseS);
  if (!noMouth) at(Mo, 0, F.mouthH, F.mouthW, F.mouthW * 0.35);   // como no principal: o quadrado da boca achatado na faixa da boca
}
function paintBack(c) { const g = c.getContext('2d'); g.fillStyle = GREEN; g.fillRect(0, 0, c.width, c.height); }

// --- papo: bolsa da garganta, logo abaixo da boca. A própria pele incha (a borda some na pele em volta, sem emenda),
// para a frente e um pouco para baixo, só na frente do pescoço. 4 estágios como no jogo (0 = vazio).
const PAPO = { amt: { value: 0 }, c: { value: new THREE.Vector2(0, 1.39) }, r: { value: new THREE.Vector2(0.31, 0.17) } };   // bolsa quase da largura da boca até ~123 cm
const PAPO_STAGES = [0, 0.05, 0.1, 0.15, 0.21];   // m
function papoShader(mat, skin = false) {   // skin: desenha o traço em "U" da borda (só na pele, não na casca)
  const prev = mat.onBeforeCompile, key = mat.customProgramCacheKey();
  mat.onBeforeCompile = (sh, r) => {
    prev.call(mat, sh, r);
    Object.assign(sh.uniforms, { uPapoAmt: PAPO.amt, uPapoC: PAPO.c, uPapoR: PAPO.r });
    sh.vertexShader = 'uniform float uPapoAmt; uniform vec2 uPapoC, uPapoR; varying float vPapoE, vPapoF;\n' + sh.vertexShader.replace('#include <begin_vertex>', `#include <begin_vertex>
      {
        vec2 pq = (position.xy - uPapoC) / uPapoR;
        if (pq.y > 0.0) pq.y *= 1.5;                       // para cima acaba antes (longe da boca)
        float e = dot(pq, pq);
        float pw = pow(max(0.0, 1.0 - e), 0.4);            // bolsa: cheia até a borda e cai de uma vez nela (o contorno desenha o "U")
        float fr = smoothstep(0.1, 0.45, normal.z);        // só a frente da garganta
        vPapoE = e; vPapoF = fr * (1.0 - smoothstep(0.1, 0.35, pq.y));   // traço só na metade de baixo (o "U")
        transformed += normalize(normal + vec3(0.0, -0.45 * (1.0 - smoothstep(-0.6, 0.2, pq.y)), 0.85)) * uPapoAmt * pw * fr;   // a parte de baixo cai sobre o peito
      }`);
    if (skin) sh.fragmentShader = 'uniform float uPapoAmt; varying float vPapoE, vPapoF;\n' + sh.fragmentShader.replace('#include <map_fragment>', `#include <map_fragment>
      {
        float aa = fwidth(vPapoE) * 1.2;
        float ln = (1.0 - smoothstep(0.07, 0.07 + aa, abs(vPapoE - 0.88))) * vPapoF * smoothstep(0.015, 0.05, uPapoAmt);
        diffuseColor.rgb = mix(diffuseColor.rgb, vec3(0.006, 0.012, 0.006), ln);
      }`);
  };
  mat.customProgramCacheKey = () => key + '|papo';
  mat.needsUpdate = true;
}

// --- peitoral: dois volumes suaves no peito, logo abaixo do papo (onde a bolsa assenta quando enche)
const PEC = { x: 15, h: 126, rx: 15, ry: 9, lift: 3.2 };   // logo abaixo do papo   // cm
function pecs(g) {
  const p = g.attributes.position, nr = g.attributes.normal;
  for (let i = 0; i < p.count; i++) {
    if (nr.getZ(i) < 0.2) continue;
    const x = p.getX(i) * 100, y = p.getY(i) * 100;
    let w = 0; for (const s of [-1, 1]) w = Math.max(w, Math.max(0, 1 - ((x - s * PEC.x) / PEC.rx) ** 2 - ((y - PEC.h) / PEC.ry) ** 2));
    if (!w) continue;
    const k = PEC.lift / 100 * w * w * (3 - 2 * w) * Math.min(1, (nr.getZ(i) - 0.2) / 0.3);
    p.setXYZ(i, p.getX(i) + nr.getX(i) * k, p.getY(i) + nr.getY(i) * k, p.getZ(i) + nr.getZ(i) * k);
  }
  g.computeVertexNormals();
}

// --- sobrancelhas: dois cordões em relevo na própria pele, acima de onde vão os olhos, com a ponta de dentro mais baixa
// (cara brava do sprite). Cada um é uma elipse inclinada que empurra a pele para fora pela normal.
const BROW = { x: 19.5, h: 170, len: 13.5, wid: 5.5, tilt: 0.3, lift: 8 };   // cm (centro, meio comprimento, meia largura, inclinação, altura)
function browRidges(g) {
  const p = g.attributes.position, nr = g.attributes.normal;
  for (let i = 0; i < p.count; i++) {
    if (nr.getZ(i) < 0.15) continue;                          // só na frente
    const x = p.getX(i) * 100, y = p.getY(i) * 100;
    let w = 0;
    for (const s of [-1, 1]) {
      const dx = x - s * BROW.x, dy = y - BROW.h, a = s * BROW.tilt;   // dentro (perto do nariz) mais baixo
      const u = dx * Math.cos(a) + dy * Math.sin(a), v = -dx * Math.sin(a) + dy * Math.cos(a);
      w = Math.max(w, Math.max(0, 1 - (u / BROW.len) ** 2 - (v / BROW.wid) ** 2));
    }
    if (!w) continue;
    const sm = w * w * w * (w * (w * 6 - 15) + 10);           // smootherstep: sobe e desce sem quina (sem degraus na malha)
    const k = BROW.lift / 100 * sm * Math.min(1, (nr.getZ(i) - 0.15) / 0.25);
    p.setXYZ(i, p.getX(i) + nr.getX(i) * k, p.getY(i) + nr.getY(i) * k, p.getZ(i) + nr.getZ(i) * k);
  }
  g.computeVertexNormals();
}

export default {
  title: 'King Frog',
  arena: 'frog',   // abre na arena própria (core.js)
  scale: 1.18,   // grandalhão: maior que os demais lutadores
  units: [100, 200, 0],
  faces: [['normal', 'Normal'], ['angry', 'Raiva'], ['pain', 'Dor'], ['ko', 'K.O.']],
  skills: [['signature', 'Assinatura'], ['tongue', 'Golpe de Língua'], ['poison', 'Pele Venenosa'], ['leap', 'Salto Real Esmagador'], ['mud', 'Cuspidela de Lama'], ['flies', 'Captura de Moscas'], ['geyser', 'Gêiser do Pântano']],
  others: [['valeria', 'Valéria'], ['frogmeshy', 'Frog (Meshy)']],
  hudExtra: 'Papo: ' + [0, 1, 2, 3, 4].map(k => `<button data-papo="${k}">${k}</button>`).join('') + '<button id="papoPulse">Infla e solta</button> Forma: <button id="toxic">Pele Venenosa</button>',
  async build({ root, U }) {
    const { X, Y, D, loft } = U;
    const P = {};
    const mk = () => { const c = document.createElement('canvas'); c.width = c.height = 200 * PR; return c; };
    const cF = mk(), cB = mk(); paintFront(cF, 'normal'); paintBack(cB);
    const tex = c => { const t = new THREE.CanvasTexture(c); t.colorSpace = THREE.SRGBColorSpace; t.anisotropy = 8; return t; };
    const tF = tex(cF), tB = tex(cB);
    const skin = U.paintMaterial(tF, tB, [-100, 0, 100, 200]);

    // --- tronco + cabeça (acima do cinturão): uma peça só, como o sapo do jogo (sem pescoço)
    // corpo: o da Valéria (base.js) largo e baixo, como sapo: cabeça de ovo bem larga (o queixo quase não afina: o
    // pescoço já começa largo), pescoço curto abrindo nos ombros, tronco em barril; tudo com a pele pintada do sapo
    // (barriga, papo, nariz e boca). Do cinturão para baixo ficam o short, as pernas e os pés de três dedos dele.
    const B = valeriaBody(U, bodyOpts(VALERIA, { cx: 0, skin: GREEN, face: GREEN, neck: GREEN,
      head: { top: 54, bot: 70, a: 40, d: 0.9, cz: -2 }, yoke: { y0: 58, y1: 76, a0: 32, a1: 43, d: 0.84, cz: 0, end: 84 },
      torso: { y0: 72, y1: 110, a: 42, d: 0.84, cz: 0 }, legs: null, shoe: null }));
    B.face.children[0].material = skin; B.torso.children[0].material = skin;
    P.upper = B.torso; P.head = B.face;
    // olhos como no modelo antigo: calombo de pele saltado em cada olho e, na frente dele, um plano com o desenho do olho
    // (troca com a expressão)
    const eyeTex = {}; for (const k of Object.keys(EYES)) { const c = document.createElement('canvas'); c.width = c.height = 256; const g = c.getContext('2d'); g.scale(2, 2); EYES[k](g); const t = new THREE.CanvasTexture(c); t.colorSpace = THREE.SRGBColorSpace; eyeTex[k] = t; }
    const E = { x: 20, h: 167, rx: 12.5, ry: 9.5, rz: 10.5, cz: 15, s: 19.5 };   // cm: centro do calombo, raios, tamanho do plano
    P.eyes = [];
    for (const sx of [-1, 1]) {
      const g = new THREE.SphereGeometry(1, 40, 28); g.scale(D(E.rx), D(E.ry), D(E.rz)); g.translate(X(sx * E.x), Y(hy(E.h)), D(E.cz));
      const dome = withOutline(new THREE.Mesh(g, skin), 0.011, [Y(hy(E.h)), Y(hy(E.h + 3))]);   // casca só na metade de cima (sem risco na emenda)
      P.head.add(dome);
      const m = new THREE.MeshBasicMaterial({ map: eyeTex.normal, transparent: true, alphaTest: 0.1, depthWrite: false, polygonOffset: true, polygonOffsetFactor: -4, side: THREE.DoubleSide });   // dois lados: o espelhado fica de costas
      const plane = new THREE.Mesh(new THREE.PlaneGeometry(D(E.s), D(E.s)), m);
      plane.position.set(X(sx * E.x), Y(hy(E.h + 0.5)), D(E.cz + E.rz + 2.4));      // na frente do calombo (sem a ponta entrar nele)
      plane.rotation.y = sx * 0.12;                                                // vira um pouco para o lado, como o calombo
      if (sx > 0) plane.scale.x = -1;                                              // o da direita é o espelho
      P.head.add(plane); P.eyes.push(plane);
    }
    papoShader(skin); papoShader(P.head.children[1].material);
   // o papo incha a pele e a casca juntas
    // calombos dos olhos: sobem acima da linha da cabeça, como no modelo (o olho pintado fica na frente deles)
    // --- coroa: anel com 5 pontas e bolinhas nas pontas, pedra vermelha em losango na frente
    {
      const A = 17, CZ = -3, B0 = 177, NA = 160;   // CZ: centro da coroa um pouco para trás no alto da cabeça
      const spikes = [0, 1, 2, 3, 4].map(k => Math.PI / 2 + k * Math.PI * 2 / 5);
      const tri = th => { let k = 0; for (const s of spikes) { const d = Math.abs(((th - s + Math.PI * 3) % (Math.PI * 2)) - Math.PI); k = Math.max(k, Math.max(0, 1 - d / 0.62)); } return k; };
      const hAt = th => 6 + 13 * Math.pow(tri(th), 1.3);
      const pos = [], idx = [];
      const ring = (r, top) => { for (let j = 0; j <= NA; j++) { const th = j / NA * Math.PI * 2; pos.push(X(r * Math.cos(th)), Y(hy(B0 + (top ? hAt(th) : 0))), D(CZ + r * Math.sin(th))); } };
      ring(A, 0); ring(A, 1); ring(A - 1.6, 1); ring(A - 1.6, 0);
      const R = NA + 1;
      for (let k = 0; k < 4; k++) for (let j = 0; j < NA; j++) { const a = k * R + j, b = a + 1, c = ((k + 1) % 4) * R + j, d = c + 1; idx.push(a, c, b, b, c, d); }
      const g = new THREE.BufferGeometry(); g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3)); g.setIndex(idx); g.computeVertexNormals();
      const crown = new THREE.Group(), cm = flat(GOLD); cm.side = THREE.DoubleSide;
      crown.add(withOutline(new THREE.Mesh(g, cm), 0.006));
      for (const s of spikes) {
        const ball = withOutline(new THREE.Mesh(new THREE.SphereGeometry(D(2), 14, 10), flat(GOLD)), 0.005);
        ball.position.set(X(A * Math.cos(s)), Y(hy(B0 + 19.5)), D(CZ + A * Math.sin(s))); crown.add(ball);
      }
      const gem = withOutline(new THREE.Mesh(new THREE.OctahedronGeometry(D(3.4)), flat(GEM)), 0.005);
      gem.scale.set(0.8, 1.15, 0.45); gem.position.set(0, Y(hy(B0 + 6)), D(CZ + A + 0.8)); crown.add(gem);
      P.crown = crown;
    }
    // --- do cinturão para baixo (short, pernas e pés): o modelo 3D do Meshy, com cor chapada (frog_lower.js)
    // --- luvas: punho arredondado, polegar do lado de dentro, punho da luva (manga) escuro. Y = do pulso aos nós,
    // Z = lado da palma (como as luvas do modelo: a orientação da guarda/soco é a mesma)
    const glove = sx => {
      const u = new THREE.Group();
      const fist = withOutline(new THREE.Mesh(new RoundedBoxGeometry(D(36), D(42), D(38), 5, D(15)), flat(GLOVE)), 0.009);
      fist.position.y = D(4); u.add(fist);
      const thumb = withOutline(new THREE.Mesh(new RoundedBoxGeometry(D(13), D(22), D(16), 4, D(6)), flat(GLOVE)), 0.007);
      thumb.position.set(-sx * D(17), D(-2), D(11)); thumb.rotation.z = sx * 0.35; u.add(thumb);
      const cuff = withOutline(new THREE.Mesh(new THREE.CylinderGeometry(D(15), D(14), D(12), 22), flat(CUFF)), 0.007);
      cuff.position.y = D(-20); u.add(cuff);
      return u;
    };
    P.gloves = { [-1]: glove(-1), [1]: glove(1) };

    // --- montagem: tronco + cabeça giram em cima do cinturão; coroa no alto da cabeça; quadril com cinturão e short
    const WAIST = Y(hy(97)), upper = new THREE.Group(); upper.position.y = WAIST; root.add(upper);
    P.upper.position.y = -WAIST; upper.add(P.upper);
    const NECK = Y(hy(150)), neck = new THREE.Group(); neck.position.y = NECK - WAIST; upper.add(neck);
    P.crown.position.y = -NECK; neck.add(P.crown);
    P.head.position.y = -NECK; neck.add(P.head);   // cabeça gira no pescoço (como a da Valéria)
    const HIPS = Y(hy(80)), hips = new THREE.Group(); hips.position.y = HIPS; root.add(hips);
    const lower = await meshyLower({ root, hips, HIPS }); P.lower = lower;
    root.add(P.gloves[-1], P.gloves[1]);
    const placeFoot = lower.placeFoot;
    let curFace = 'normal', mouthHidden = false; const repaintFace = () => { paintFront(cF, curFace, mouthHidden); tF.needsUpdate = true; };
    const setFace = f => { curFace = f; repaintFace(); for (const e of P.eyes) { e.material.map = eyeTex[f] || eyeTex.normal; e.material.needsUpdate = true; } };
    // luvas: o motor põe a "mão" (alvos fantasma); a luva vai lá e gira: na guarda nós para cima e palma para ele, no
    // soco nós para a frente e palma para baixo, acompanhando o giro do tronco
    const hands = { [-1]: new THREE.Object3D(), [1]: new THREE.Object3D() }; root.add(hands[-1], hands[1]);
    const UPV = new THREE.Vector3(0, 1, 0), FWD = new THREE.Vector3(0, 0, 1);
    // papo: vai para o estágio pedido com uma mola (enche macio e treme um pouco ao parar); "Infla e solta" enche até o
    // fim e esvazia sozinho
    let papoTarget = 0, papoV = 0, pulseT = -1;
    document.querySelectorAll('[data-papo]').forEach(b => b.onclick = () => { pulseT = -1; papoTarget = PAPO_STAGES[+b.dataset.papo]; });
    document.querySelector('#papoPulse').onclick = () => { pulseT = 0; };
    window.PAPO = PAPO;
    // ================= especiais (os do jogo, skills.cpp e skill_poses.cpp, com os efeitos do laboratório, vfx/lab_frog.cpp)
    const easeO = k => 1 - (1 - k) ** 3, win = (a, b, x) => Math.max(0, Math.min(1, (x - a) / (b - a))), TZ = 0.95, K = 0.0072;
    const PINK = '#f43f5e', PINK2 = '#fb7185', PALE = '#fef08a', MUD = '#3b2010', MUD2 = '#5a341b', MUDD = '#221208', DUST = '#c9b48a', AQUA = '#67e8f9', TOX = '#4ade80', INKT = '#0f1c10';
    root.updateMatrixWorld(true);
    const fxRoot = new THREE.Group(); root.add(fxRoot); const off = new THREE.Vector3();
    const glow = (col, o = 1, add = false) => new THREE.MeshBasicMaterial({ color: col, transparent: true, opacity: o, depthWrite: false, side: THREE.DoubleSide, blending: add ? THREE.AdditiveBlending : THREE.NormalBlending });
    const queue = []; let now = 0; const after = (dt, fn) => queue.push({ at: now + dt, fn });
    const FX = { sign: -99, tongue: -99, poison: -99, leap: -99, mud: -99, flies: -99, geyser: -99, crack: -99 };
    // bolinhas: faíscas, saliva, poeira, lama e gotas
    const dotG = new THREE.SphereGeometry(1, 10, 8), dots = Array.from({ length: 260 }, () => { const m = new THREE.Mesh(dotG, glow('#fff')); m.visible = false; fxRoot.add(m); return { m, life: 0, v: new THREE.Vector3() }; });
    let dotI = 0; const dot = (p, v, rr, col, life, g = 3, op = 1, grow = 0) => { const q = dots[dotI++ % dots.length]; q.m.position.copy(p); q.m.scale.setScalar(rr); q.v.copy(v); q.r = rr; q.m.material.color.set(col); q.m.material.opacity = op; q.op = op; q.life = q.life0 = life; q.g = g; q.grow = grow; q.m.visible = true; };
    const burst = (p, n, spd, rr, cols, life, g = 3) => { for (let i = 0; i < n; i++) { const a = Math.random() * 6.283, sp = spd * (0.5 + Math.random()); dot(p, new THREE.Vector3(Math.cos(a) * sp, Math.sin(a) * sp * 0.8 + 0.4, (Math.random() - 0.5) * spd), rr * (0.6 + Math.random() * 0.7), cols[i % cols.length], life * (0.7 + Math.random() * 0.6), g); } };
    const dust = (x, z, n, pw) => { for (let i = 0; i < n; i++) { const a = i / n * 6.283 + (Math.random() - 0.5) * 0.2, sp = pw * (0.5 + Math.random() * 0.7); dot(new THREE.Vector3(x + Math.cos(a) * 0.1, 0.05, z + Math.sin(a) * 0.1), new THREE.Vector3(Math.cos(a) * sp, 0.15 + Math.random() * 0.3, Math.sin(a) * sp * 0.7), 0.045 + Math.random() * 0.035, Math.random() < 0.5 ? DUST : '#e5d8bb', 0.6, 0, 0.97, 0.2); } };
    const ringG = new THREE.RingGeometry(0.84, 1, 44), rings = Array.from({ length: 10 }, () => { const m = new THREE.Mesh(ringG, glow('#fff')); m.visible = false; fxRoot.add(m); return { m, life: 0 }; });
    let ringI = 0; const ring = (p, R, col, life, flatR = false) => { const q = rings[ringI++ % rings.length]; q.m.position.copy(p); q.R = R; q.life = q.life0 = life; q.m.material.color.set(col); q.flat = flatR; q.m.rotation.set(flatR ? -Math.PI / 2 : 0, 0, 0); q.m.scale.setScalar(0.02); q.m.visible = true; };
    const segG = new THREE.BoxGeometry(1, 1, 1), ZA = new THREE.Vector3(0, 0, 1), segs = Array.from({ length: 40 }, () => { const m = new THREE.Mesh(segG, glow('#fff')); m.visible = false; fxRoot.add(m); return { m, life: 0 }; });
    let segI = 0; const seg = (a, b, w, col, life) => { const q = segs[segI++ % segs.length], d = b.clone().sub(a), L = d.length(); q.m.position.copy(a).addScaledVector(d, 0.5); q.m.quaternion.setFromUnitVectors(ZA, d.normalize()); q.m.scale.set(w, w, L); q.m.material.color.set(col); q.m.material.opacity = 1; q.life = q.life0 = life; q.m.visible = true; };
    const word = (txt, col) => { const c = document.createElement('canvas'); c.width = 400; c.height = 170; const g = c.getContext('2d'); g.font = '700 120px Bangers, Impact, sans-serif'; g.textAlign = 'center'; g.textBaseline = 'middle'; g.lineJoin = 'round'; g.lineWidth = 22; g.strokeStyle = '#0f1c10'; g.strokeText(txt, 200, 90); g.fillStyle = col; g.fillText(txt, 200, 90);
      const t = new THREE.CanvasTexture(c); t.colorSpace = THREE.SRGBColorSpace; const m = new THREE.Mesh(new THREE.PlaneGeometry(0.5, 0.21), new THREE.MeshBasicMaterial({ map: t, transparent: true, depthWrite: false })); m.visible = false; fxRoot.add(m); return m; };
    const bum = word('BUM!', '#facc15');
    // pontos na superfície da pele (o raio vem de frente), para a boca, o peito e as gotas assentarem nela
    const rc = new THREE.Raycaster(), surfZ = (x, y) => { rc.set(new THREE.Vector3(x, y, 6), new THREE.Vector3(0, 0, -1)); let z = 0.2; for (const h of rc.intersectObjects([P.upper, P.head], true)) if (h.object.material === skin && h.point.z > z) z = h.point.z; return z; };
    const MOUTHY = Y(hy(FACE.mouthH)), mouthZ = surfZ(0, MOUTHY), chestZ = surfZ(0.05, 1.12);
    const mouthO = new THREE.Object3D(); mouthO.position.set(0, MOUTHY, mouthZ); P.head.add(mouthO);
    const mouthAt = () => fxRoot.worldToLocal(mouthO.getWorldPosition(new THREE.Vector3()));
    // boca aberta (sprites MudSpit/OpenMouth): lábio escuro, fundo marrom; abre por cima da boca pintada
    const mouthOpen = new THREE.Group(); mouthOpen.position.set(0, MOUTHY - 0.04, mouthZ - 0.035); mouthOpen.visible = false; P.head.add(mouthOpen);
    { const a = new THREE.SphereGeometry(1, 24, 16); const lip = new THREE.Mesh(a, flat(INKT)); lip.scale.set(0.18, 0.105, 0.05); mouthOpen.add(lip); const inn = new THREE.Mesh(a, flat('#431407')); inn.scale.set(0.15, 0.08, 0.058); mouthOpen.add(inn); const t = new THREE.Mesh(a, flat(PINK2)); t.scale.set(0.08, 0.04, 0.03); t.position.set(0, -0.035, 0.038); mouthOpen.add(t); mouthOpen.userData.tongue = t; }
    const setOpen = k => { const hide = k > 0.02; if (hide !== mouthHidden) { mouthHidden = hide; repaintFace(); } mouthOpen.visible = hide; mouthOpen.scale.set(0.5 + 0.5 * k, Math.max(0.02, k), 1); };
    // língua: tubo refeito a cada quadro com casca de contorno, faixa clara e a bola da ponta
    const tubeOf = (col, basic) => { const NR = 8, NN = 17, g = new THREE.BufferGeometry(), pos = new Float32Array(NR * NN * 3), idx = []; for (let i = 0; i < NN - 1; i++) for (let j = 0; j < NR; j++) { const a = i * NR + j, b = i * NR + (j + 1) % NR, c = a + NR, d = b + NR; idx.push(a, c, b, b, c, d); } g.setAttribute('position', new THREE.BufferAttribute(pos, 3)); g.setIndex(idx); const m = new THREE.Mesh(g, basic); m.frustumCulled = false; m.visible = false; fxRoot.add(m);
      m.userData.set = (pts, rf, up = 0) => { const T = new THREE.Vector3(), U_ = new THREE.Vector3(), V_ = new THREE.Vector3(), Yv = new THREE.Vector3(0, 1, 0); pts.forEach((p, i) => { T.copy(pts[Math.min(NN - 1, i + 1)]).sub(pts[Math.max(0, i - 1)]).normalize(); U_.crossVectors(T, Yv); if (U_.lengthSq() < 1e-4) U_.set(1, 0, 0); U_.normalize(); V_.crossVectors(U_, T).normalize(); const rr = rf(i / (NN - 1)); for (let j = 0; j < NR; j++) { const a = j / NR * 6.283, o = i * NR + j; pos[o * 3] = p.x + U_.x * Math.cos(a) * rr + V_.x * (Math.sin(a) * rr + up * rr); pos[o * 3 + 1] = p.y + U_.y * Math.cos(a) * rr + V_.y * (Math.sin(a) * rr + up * rr); pos[o * 3 + 2] = p.z + U_.z * Math.cos(a) * rr + V_.z * (Math.sin(a) * rr + up * rr); } }); g.attributes.position.needsUpdate = true; g.computeVertexNormals(); }; return m; };
    const tOut = tubeOf(INKT, new THREE.MeshBasicMaterial({ color: INKT, side: THREE.BackSide })), tBody = tubeOf(PINK, flat(PINK)), tHi = tubeOf(PALE, new THREE.MeshBasicMaterial({ color: PALE }));
    const tTip = new THREE.Group(); tTip.visible = false; fxRoot.add(tTip); tTip.add(withOutline(new THREE.Mesh(new THREE.SphereGeometry(0.085, 18, 12), flat(PINK2)), 0.008)); { const sp2 = new THREE.Mesh(new THREE.SphereGeometry(0.02, 8, 6), new THREE.MeshBasicMaterial({ color: '#fff' })); sp2.position.set(-0.03, 0.04, 0.066); tTip.add(sp2); }
    const drawTongue = (pts, wB = 0.075, wT = 0.05) => { const rf = s => wB + (wT - wB) * s, up = s => rf(s) + 0.009; tOut.userData.set(pts, up); tBody.userData.set(pts, rf); tHi.userData.set(pts, s => rf(s) * 0.22, 3); tOut.visible = tBody.visible = tHi.visible = tTip.visible = true; tTip.position.copy(pts[pts.length - 1]); };
    const hideTongue = () => { tOut.visible = tBody.visible = tHi.visible = tTip.visible = false; };
    // lâmina de vácuo: triângulos da boca até a ponta, somem rápido
    const fans = Array.from({ length: 16 }, () => { const g = new THREE.BufferGeometry(); g.setAttribute('position', new THREE.BufferAttribute(new Float32Array(9), 3)); const m = new THREE.Mesh(g, glow(PINK, 0.5)); m.frustumCulled = false; m.visible = false; fxRoot.add(m); return { m, life: 0 }; });
    let fanI = 0; const fan = (base, a, b) => { const q = fans[fanI++ % fans.length], ar = q.m.geometry.attributes.position; ar.setXYZ(0, base.x, base.y, base.z); ar.setXYZ(1, a.x, a.y, a.z); ar.setXYZ(2, b.x, b.y, b.z); ar.needsUpdate = true; q.life = 1; q.m.visible = true; seg(a, b, 0.018, '#ffffff', 0.2); };
    let lastTip = null, tongueHit = false;
    // bomba de lama e poças (6 s): fundo escuro, miolo marrom, respingos e bolhas
    const bombG = new THREE.Group(); bombG.visible = false; fxRoot.add(bombG); bombG.add(withOutline(new THREE.Mesh(new THREE.SphereGeometry(0.115, 18, 12), flat(MUD)), 0.008)); { const h = new THREE.Mesh(new THREE.SphereGeometry(0.05, 10, 8), flat(MUD2)); h.position.set(-0.04, 0.05, 0.05); bombG.add(h); }
    const bomb = { on: false, t: 0, from: new THREE.Vector3(), to: new THREE.Vector3() };
    const puddle = new THREE.Group(); puddle.position.set(0, 0.012, TZ); puddle.rotation.x = -Math.PI / 2; puddle.visible = false; fxRoot.add(puddle);
    { for (const [rr, c, z] of [[1, MUDD, 0], [0.88, MUD, 0.002], [0.55, MUD2, 0.004]]) { const g = new THREE.CircleGeometry(rr, 36); const pp = g.attributes.position; for (let i = 1; i < pp.count; i++) { const a = Math.atan2(pp.getY(i), pp.getX(i)), w = 1 + 0.07 * Math.sin(a * 5 + rr * 3) + 0.04 * Math.sin(a * 9); pp.setXY(i, pp.getX(i) * w, pp.getY(i) * w); } const m = new THREE.Mesh(g, flat(c)); m.position.z = z; puddle.add(m); } for (let i = 0; i < 8; i++) { const a = i * 2.399, d = 1.1 + (i % 4) * 0.1, m = new THREE.Mesh(new THREE.CircleGeometry(0.05 + (i % 3) * 0.022, 12), flat(MUD)); m.position.set(Math.cos(a) * d, Math.sin(a) * d, 0.002); puddle.add(m); } }
    // gêiser: coluna em degradê turquesa com contorno, listras de água e espuma no alto
    const geyMat = new THREE.ShaderMaterial({ transparent: true, depthWrite: false, side: THREE.DoubleSide, uniforms: { uT: { value: 0 } },
      vertexShader: 'varying vec3 vN; varying vec3 vV; varying vec2 vU; void main() { vU = uv; vec4 mv = modelViewMatrix * vec4(position, 1.0); vN = normalize(normalMatrix * normal); vV = normalize(-mv.xyz); gl_Position = projectionMatrix * mv; }',
      fragmentShader: `uniform float uT; varying vec3 vN; varying vec3 vV; varying vec2 vU;
        void main() { float r = 1.0 - abs(dot(normalize(vN), normalize(vV))), h = vU.y;
          vec3 c = h < 0.3 ? mix(vec3(0.06, 0.46, 0.43), vec3(0.02, 0.71, 0.83), h / 0.3) : h < 0.7 ? mix(vec3(0.02, 0.71, 0.83), vec3(0.40, 0.91, 0.98), (h - 0.3) / 0.4) : mix(vec3(0.40, 0.91, 0.98), vec3(0.94, 0.99, 0.98), (h - 0.7) / 0.3);
          float st = step(0.93, fract(vU.x * 7.0 + sin(h * 22.0 - uT * 9.0) * 0.14 + uT * 0.4)); c = mix(c, vec3(1.0), st * 0.75);
          if (r > 0.86) c = vec3(0.03, 0.2, 0.27);
          gl_FragColor = vec4(c, r > 0.86 ? 1.0 : 0.9); }` });
    const GEY_H = 3.3, geyGroup = new THREE.Group(); geyGroup.position.set(0, 0, TZ); geyGroup.visible = false; fxRoot.add(geyGroup);
    const geyCol = (() => { const g = new THREE.CylinderGeometry(0.16, 0.26, 1, 24, 12, true); g.translate(0, 0.5, 0); const m = new THREE.Mesh(g, geyMat); m.renderOrder = 4; geyGroup.add(m); return m; })();
    const foam = new THREE.Group(); geyGroup.add(foam); for (const [x, y, z, rr] of [[0, 0, 0, 0.19], [0.13, -0.04, 0.05, 0.13], [-0.13, -0.03, 0.04, 0.13], [0.04, 0.09, -0.05, 0.12], [-0.05, -0.05, -0.1, 0.11]]) { const g = new THREE.SphereGeometry(rr, 14, 10); g.translate(x, y, z); foam.add(withOutline(new THREE.Mesh(g, flat('#f0fdfa')), 0.007)); }
    // moscas: corpo escuro, olhos vermelhos, duas asas claras batendo; voam em volta da boca e a língua as pega
    const flies = [[0.5, 0.088, 70, 20, 35, 80, 0.35], [2.3, -0.08, 80, 25, 42, 90, 0.46], [4.1, 0.095, 60, 18, 30, 74, 0.57], [5.5, -0.085, 75, 22, 36, 85, 0.68]].map(([a0, sp, rx, ry, rz, bh, cp]) => { const g = new THREE.Group(); g.visible = false; fxRoot.add(g); const b = new THREE.SphereGeometry(0.038, 12, 8); b.scale(1, 0.85, 1.25); g.add(withOutline(new THREE.Mesh(b, flat('#0f172a')), 0.004)); for (const sd of [-1, 1]) { const e = new THREE.Mesh(new THREE.SphereGeometry(0.014, 8, 6), new THREE.MeshBasicMaterial({ color: '#ef4444' })); e.position.set(sd * 0.018, 0.012, 0.04); g.add(e); } const wings = [-1, 1].map(sd => { const wg = new THREE.CircleGeometry(0.05, 14); wg.scale(1, 0.55, 1); wg.rotateX(-Math.PI / 2); wg.translate(sd * 0.05, 0, 0); const w = new THREE.Mesh(wg, glow('#e0f2fe', 0.8)); g.add(w); return w; }); return { g, wings, a0, sp, rx, ry, rz, bh, cp, caught: false }; });
    // Pele Venenosa (sprite Poison): pele verde-escura com a espiral lenta de manchas, gotas verdes, olhos e pedra verdes
    const PZ = { amt: { value: 0 }, t: { value: 0 } };
    const poisonOf = (mat, swirl) => { const prev = mat.onBeforeCompile, key = mat.customProgramCacheKey(); mat.onBeforeCompile = (sh, rr) => { prev.call(mat, sh, rr); sh.uniforms.uPz = PZ.amt; sh.uniforms.uPzT = PZ.t;
        if (swirl) sh.vertexShader = 'varying vec3 vPzP;\n' + sh.vertexShader.replace('#include <begin_vertex>', '#include <begin_vertex>\n vPzP = position;');
        sh.fragmentShader = 'uniform float uPz, uPzT;' + (swirl ? ' varying vec3 vPzP;' : '') + '\n' + sh.fragmentShader.replace('#include <color_fragment>', `#include <color_fragment>
        { vec3 pzTint = vec3(0.3, 0.42, 0.6);` + (swirl ? `
          vec2 pzQ = vPzP.y > 1.42 ? (vPzP.xy - vec2(0.0, 1.62)) / vec2(0.5, 0.4) : (vPzP.xy - vec2(0.0, 1.08)) / vec2(0.5, 0.4);
          float pzA = atan(pzQ.y, pzQ.x), pzR = length(pzQ) * (1.0 + 0.2 * sin(pzA * 3.0 + uPzT * 0.3)), pzB = floor(pzR * 9.0 + pzA / 6.283 * 2.0 - uPzT * 0.6);
          float pzM = mod(pzB, 3.0); pzTint *= pzM < 1.0 ? vec3(0.45, 0.62, 0.45) : pzM < 2.0 ? vec3(0.72, 0.9, 0.7) : vec3(1.0);` : '') + `
          diffuseColor.rgb *= mix(vec3(1.0), pzTint, uPz); }`); };
      mat.customProgramCacheKey = () => key + '|pz' + (swirl ? 1 : 0); mat.needsUpdate = true; };
    poisonOf(skin, true);
    { const seen = new Set(); P.lower.update?.(); root.traverse(o => { if (o.isMesh && o.material.vertexColors && !seen.has(o.material)) { seen.add(o.material); poisonOf(o.material, false); } }); }
    const glovePz = []; for (const sd of [-1, 1]) P.gloves[sd].traverse(o => { if (o.isMesh && o.material.side !== THREE.BackSide) glovePz.push({ m: o.material, a: o.material.color.clone(), b: new THREE.Color('#1f3a22') }); });
    const crownPz = []; P.crown.traverse(o => { if (o.isMesh && o.material.side !== THREE.BackSide && o.material.color) crownPz.push({ m: o.material, a: o.material.color.clone(), b: o.material.color.getHexString() === new THREE.Color(GEM).getHexString() ? new THREE.Color('#10b981') : new THREE.Color('#9a8535') }); });
    const eyeA = new THREE.Color('#ffffff'), eyeB = new THREE.Color(TOX);
    let toxicOn = false; const toxB = document.querySelector('#toxic'); if (toxB) toxB.onclick = () => { toxicOn = !toxicOn; toxB.style.outline = toxicOn ? '2px solid #fc6' : ''; };
    // gotas: escorrem pela pele, esticam e caem; no peito, na cabeça e nas luvas
    const dripG = new THREE.SphereGeometry(1, 10, 8), dripMat = new THREE.MeshBasicMaterial({ color: TOX }), dripIn = [[-0.26, 1.2, 0], [0.26, 1.18, 0.4], [-0.14, 1.02, 0.75], [0.16, 1.0, 0.2], [-0.32, 1.62, 0.3], [0.32, 1.6, 0.65]].map(([x, y, ph]) => { const m = new THREE.Mesh(dripG, dripMat), z = surfZ(x, y) - 0.005; m.visible = false; (y > 1.42 ? P.head : P.upper).add(m); return { m, x, y, z, ph }; }), dripG2 = [[-1, 0.55], [1, 0.9]].map(([sd, ph]) => { const m = new THREE.Mesh(dripG, dripMat); m.visible = false; root.add(m); return { m, sd, ph }; });
    // rachaduras do salto: sete ramos em zigue-zague e os destroços
    const crack = new THREE.Group(); crack.position.y = 0.014; crack.visible = false; fxRoot.add(crack);
    { const m = new THREE.Mesh(new THREE.CircleGeometry(0.16, 16), flat('#17110b')); m.rotation.x = -Math.PI / 2; crack.add(m); for (let b = 0; b < 7; b++) { let dist = 0, ang = b / 7 * 6.283 + (b * 0.73 % 0.4) - 0.2, px = 0, pz = 0; const n = 5; for (let k = 0; k < n; k++) { dist += 0.2 * (0.8 + ((b + k) * 0.37 % 0.4)); ang += ((b * 3 + k * 5) % 7 / 7 - 0.5) * 0.6; const nx = Math.cos(ang) * dist, nz = Math.sin(ang) * dist, L = Math.hypot(nx - px, nz - pz), g = new THREE.PlaneGeometry(L, 0.05 * (1 - k / n) + 0.012); g.rotateX(-Math.PI / 2); const q = new THREE.Mesh(g, flat('#17110b')); q.position.set((px + nx) / 2, 0, (pz + nz) / 2); q.rotation.y = -Math.atan2(nz - pz, nx - px); crack.add(q); px = nx; pz = nz; } } }
    const debrisG = new THREE.DodecahedronGeometry(1, 0), debris = Array.from({ length: 18 }, () => { const o = new THREE.Mesh(debrisG, flat('#6b5a44')); o.visible = false; fxRoot.add(o); return { o, life: 0, v: new THREE.Vector3(), rs: 0 }; });
    // coroa: posição em coordenadas do root (centro da coroa) com giro em torno do próprio centro
    const C0 = new THREE.Vector3(0, Y(hy(186)), -0.03), crownQ = new THREE.Quaternion(), ZZ = new THREE.Vector3(0, 0, 1);
    const crownAt = (T, rot) => { root.updateMatrixWorld(true); const lp = neck.worldToLocal(root.localToWorld(T.clone())); crownQ.setFromAxisAngle(ZZ, rot); P.crown.quaternion.copy(crownQ); P.crown.position.copy(lp).sub(C0.clone().applyQuaternion(crownQ)); };
    const crownHome = () => { P.crown.position.set(0, -NECK, 0); P.crown.quaternion.identity(); };
    const skills = {
      // assinatura (laboratório): tira a coroa, guarda debaixo do braço, bate no peito (BUM!) e põe a coroa de volta
      signature: { dur: 3.0, start: () => { FX.sign = now; after(0.9, () => setFace('angry')); after(2.16, () => setFace('normal')); },
        pose: (k, pose) => ({}) },
      // língua gigante em grande arco, de um lado ao outro, com a lâmina de vácuo e o impacto na frente
      tongue: { dur: 1.3, start: () => { FX.tongue = now; tongueHit = false; lastTip = null; setFace('angry'); after(1.3, () => setFace('normal')); },
        pose: (k, pose) => { const p = k / 1.3, q = Math.sin(p * Math.PI), v = [-0.07 * q, -0.03 * q, -0.45 * q, 0, 0]; pose[-1] = v; pose[1] = [...v]; return { drop: 0.08 * q, lean: 0.22 * Math.sin(p * 6.283) }; } },
      // toxina: treme com as mãos no peito e a pele vira (dura 6 s)
      poison: { dur: 1.0, start: () => { FX.poison = now; setFace('angry'); after(1, () => setFace('normal')); after(0.3, () => burst(new THREE.Vector3(0, 1.2, chestZ), 20, 1.1, 0.018, [TOX, '#10b981'], 0.7)); },
        pose: (k, pose) => { const q = Math.sin(k * Math.PI), v = [0.09, 0.03 + 0.03 * q, 0.38, 0, 0]; pose[-1] = v; pose[1] = [...v]; return { drop: 0.06 * q, tilt: 0.05 * Math.sin(k * 37.7) * q }; } },
      // salta para fora da tela, cai esmagando (poeira, rachaduras, destroços); a coroa fica no alto e cai depois na cabeça
      leap: { dur: 1.3, start: () => { FX.leap = now; setFace('angry'); after(1.3, () => setFace('normal')); after(0.26, () => dust(0, 0, 16, 1.1));
          after(0.754, () => { FX.crack = now; dust(0, 0, 20, 1.6); ring(new THREE.Vector3(0, 0.02, 0), 1.3, DUST, 0.45, true); for (let i = 0; i < debris.length; i++) { const q = debris[i], a = Math.random() * 6.283, sp = 0.8 + Math.random() * 1.6; q.o.position.set(Math.cos(a) * 0.2, 0.03, Math.sin(a) * 0.2); q.v.set(Math.cos(a) * sp, 1.2 + Math.random() * 1.8, Math.sin(a) * sp * 0.7); q.rs = (Math.random() - 0.5) * 12; q.o.scale.setScalar(0.022 + Math.random() * 0.03); q.life = 1.3; q.o.visible = true; } }); },
        pose: (k, pose) => { const v = [0, 0.2, -0.1, 0, 0]; pose[-1] = v; pose[1] = [...v]; return {}; } },
      // disparo de lama: enche o papo, abre a boca e cospe a bomba em arco; cai e vira poça
      mud: { dur: 1.3, start: () => { FX.mud = now; setFace('angry'); after(1.3, () => setFace('normal')); papoTarget = PAPO_STAGES[4]; after(0.57, () => { papoTarget = 0; bomb.on = true; bomb.t = 0; bomb.from.copy(mouthAt()); bomb.to.set(0, 0.02, TZ); }); },
        pose: (k, pose) => { const q = Math.sin(k / 1.3 * Math.PI), v = [0.16 * q, 0, -0.18, 0, 0]; pose[-1] = v; pose[1] = [...v]; return { lean: 0.1 * q }; } },
      // abre os braços e estica o pescoço; a língua pega as quatro moscas, em 0,70 a cura
      flies: { dur: 1.7, start: () => { FX.flies = now; for (const f of flies) f.caught = false; after(1.19, () => { for (let i = 0; i < 24; i++) { const a = Math.random() * 6.283, d = Math.random() * 0.3; dot(new THREE.Vector3(Math.cos(a) * d, 0.4 + Math.random() * 0.85, chestZ - 0.1), new THREE.Vector3((Math.random() - 0.5) * 0.7, 0.9 + Math.random(), 0), 0.02 + Math.random() * 0.02, [TOX, '#86efac', '#facc15'][i % 3], 0.75, 0); } }); },
        pose: (k, pose) => { const p = k / 1.7, q = Math.sin(p * Math.PI), v = [-0.03 * q, 0.13 * q, -0.55 * q, 0, 0]; pose[-1] = v; pose[1] = [...v]; return { lean: 0.15 * Math.sin(p * Math.PI * 3) }; } },
      // uppercut de direita: bolhas sob o rival, a coluna sobe em 0,35 e desce até 0,85
      geyser: { dur: 1.5, start: () => { FX.geyser = now; setFace('angry'); after(1.5, () => setFace('normal')); after(0.525, () => ring(new THREE.Vector3(0, 0.02, TZ), 0.75, AQUA, 0.6, true)); },
        pose: (k, pose) => { const q = Math.sin(k / 1.5 * Math.PI); pose[1] = [0.3 * q, 0.43 * q, -0.05, 0, 0]; pose[-1] = [0, 0, 0, 0, 0]; return { lean: -0.25 * q }; } },
    };
    const fxFrame = c => {
      now = c.t; const t = now, dt = c.dt, S = root.scale.x || 1, cam = window.VIN?.camera;
      for (let i = queue.length - 1; i >= 0; i--) if (now >= queue[i].at) { queue[i].fn(); queue.splice(i, 1); }
      // deslocamentos do corpo neste quadro: no salto sobe para fora da tela e fica lá até cair em 0,58
      off.set(0, 0, 0);
      const pL = (t - FX.leap) / 1.3, lOn = pL > 0 && pL < 1; if (lOn) { const j = pL <= 0.3 ? Math.sin(pL / 0.3 * Math.PI / 2) : pL <= 0.48 ? 1 : pL <= 0.58 ? 1 - (pL - 0.48) / 0.1 : 0; off.y += 2.7 * j; const tk = Math.min(1, j * 3); if (tk > 0.01) { placeFoot(-1, -0.02 * tk, 0.25 * tk); placeFoot(1, 0.02 * tk, 0.25 * tk); } const sq = pL > 0.57 && pL < 0.78 ? Math.sin(win(0.57, 0.78, pL) * Math.PI) : 0; if (sq > 0) root.scale.set(S * (1 + 0.14 * sq), S * (1 - 0.2 * sq), S * (1 + 0.14 * sq)); }
      const pFl = (t - FX.flies) / 1.7; if (pFl > 0 && pFl < 1) { off.y += 0.07 * Math.sin(pFl * Math.PI); neck.position.y += 0.05 * Math.sin(pFl * Math.PI); neck.rotation.x -= 0.25 * Math.sin(pFl * Math.PI); }
      root.position.add(off); fxRoot.position.copy(off).negate().divideScalar(S); root.updateMatrixWorld(true); lower.update();
      for (const q of dots) { if (q.life <= 0) { q.m.visible = false; continue; } q.life -= dt; q.v.y -= q.g * dt; q.m.position.addScaledVector(q.v, dt); const u = q.life / q.life0; if (q.grow) { q.r += q.grow * dt; q.m.scale.setScalar(q.r); q.m.material.opacity = q.op * Math.min(1, u * 3.5); } else q.m.scale.setScalar(q.r * Math.max(0.05, u)); if (q.m.position.y < 0.01) q.life = 0; }
      for (const q of rings) { if (q.life <= 0) { q.m.visible = false; continue; } q.life -= dt; const u = 1 - q.life / q.life0, e = q.R * easeO(u) + 0.03; q.m.scale.set(e, e, 1); q.m.material.opacity = Math.max(0, 1 - u); if (cam && !q.flat) q.m.quaternion.copy(cam.quaternion); }
      for (const q of segs) { if (q.life <= 0) { q.m.visible = false; continue; } q.life -= dt; q.m.material.opacity = Math.max(0, q.life / q.life0); }
      for (const q of fans) { if (q.life <= 0) { q.m.visible = false; continue; } q.life -= dt * 4.5; q.m.material.opacity = Math.max(0, q.life) * 0.5; }
      for (const q of debris) { if (q.life <= 0) { q.o.visible = false; continue; } q.life -= dt; q.v.y -= 6 * dt; q.o.position.addScaledVector(q.v, dt); q.o.rotation.x += q.rs * dt; q.o.rotation.z += q.rs * dt * 0.7; if (q.o.position.y < 0.02) { q.o.position.y = 0.02; q.v.set(q.v.x * 0.6, 0, q.v.z * 0.6); q.rs *= 0.6; } }
      let open = 0; const mp = mouthAt();
      // ---- língua
      { const p = (t - FX.tongue) / 1.3, on = p >= 0.22 && p <= 0.74; let shown = false; if (p > 0.1 && p < 0.86) open = Math.max(open, Math.sin(win(0.1, 0.86, p) * Math.PI)); if (on) { const a = (p - 0.22) / 0.52, ea = Math.sin((a - 0.5) * Math.PI) * 0.5 + 0.5, ext = a < 0.16 ? Math.sin(a / 0.16 * Math.PI / 2) : a <= 0.84 ? 1 : Math.cos((a - 0.84) / 0.16 * Math.PI / 2), tip = new THREE.Vector3(mp.x + (-170 + ea * 340) * K * ext, mp.y - Math.sin(a * Math.PI) * 22 * K + Math.cos(a * Math.PI * 2) * 10 * K, mp.z + 0.05 + Math.sin(a * Math.PI) * 1.25 * ext);
          if (ext > 0.01) { const pts = []; for (let i = 0; i <= 16; i++) { const s = i / 16, w = Math.sin((a - s * 0.38) * Math.PI * 3) * 38 * K * Math.sin(s * Math.PI) * ext, v = Math.sin(s * Math.PI) * 16 * K * ext; pts.push(new THREE.Vector3(mp.x + (tip.x - mp.x) * s + w, mp.y + (tip.y - mp.y) * s + v, mp.z + (tip.z - mp.z) * s)); } drawTongue(pts); shown = true; if (ext > 0.25 && lastTip) fan(mp, lastTip, tip); lastTip = tip.clone(); if (Math.random() < 0.75) dot(new THREE.Vector3(tip.x + (Math.random() - 0.5) * 0.1, tip.y + (Math.random() - 0.5) * 0.1, tip.z), new THREE.Vector3((Math.random() - 0.45) * 1.6, (Math.random() - 0.5) * 1.2, 0), 0.018 + Math.random() * 0.02, PALE, 0.45, 3); }
          if (a >= 0.48 && !tongueHit) { tongueHit = true; const hit = tip.clone(); ring(hit, 0.34, '#ffffff', 0.3); ring(hit, 0.42, PINK, 0.34); for (let l = 0; l < 8; l++) { const an = l / 8 * 6.283 + (Math.random() - 0.5) * 0.2, A = hit.clone().add(new THREE.Vector3(Math.cos(an) * 0.11, Math.sin(an) * 0.11, 0)), B = hit.clone().add(new THREE.Vector3(Math.cos(an) * (0.26 + Math.random() * 0.14), Math.sin(an) * (0.26 + Math.random() * 0.14), (Math.random() - 0.5) * 0.25)); seg(A, B, 0.02, '#ffffff', 0.22); } burst(hit, 28, 1.7, 0.014, ['#ffffff', PINK, '#facc15'], 0.45, 2); } }
        const fOn = (t - FX.flies) / 1.7; if (!shown && !(fOn > 0 && fOn < 1)) hideTongue(); }
      // ---- bomba e poça
      if (bomb.on) { bomb.t += dt / 0.37; const u = Math.min(1, bomb.t), P0 = bomb.from, P1 = bomb.to; bombG.visible = true; bombG.position.lerpVectors(P0, P1, u); bombG.position.y += Math.sin(u * Math.PI) * 0.55; bombG.rotation.x += dt * 9; bombG.rotation.z += dt * 6; if (Math.random() < 0.6) dot(bombG.position, new THREE.Vector3((Math.random() - 0.5) * 0.5, -0.5, 0), 0.022, Math.random() < 0.5 ? MUD : MUD2, 0.4, 3); if (u >= 1) { bomb.on = false; bombG.visible = false; FX.puddle = now; for (let i = 0; i < 22; i++) { const a = Math.random() * 6.283, sp = 0.9 + Math.random() * 1.7; dot(new THREE.Vector3(0, 0.03, TZ), new THREE.Vector3(Math.cos(a) * sp, 1.2 + Math.random() * 1.6, Math.sin(a) * sp * 0.7), 0.024 + Math.random() * 0.02, i % 2 ? MUD : MUD2, 0.6, 6); } } }
      { const a = t - (FX.puddle ?? -99), on = a >= 0 && a < 6.3; puddle.visible = on; if (on) { const k = 0.52 * easeO(Math.min(1, a / 0.35)) * (1 - ss(5.7, 6.3, a)); puddle.scale.set(k, k * 0.92, 1); if (a < 5.3 && Math.random() < 0.25) { const an = Math.random() * 6.283, d = k * Math.random() * 0.75; dot(new THREE.Vector3(Math.cos(an) * d, 0.02, TZ + Math.sin(an) * d), new THREE.Vector3(0, 0.03, 0), 0.015 + Math.random() * 0.02, MUD2, 0.5, 0, 1, 0.05); } } }
      { const p = (t - FX.mud) / 1.3; if (p > 0.32 && p < 0.72) open = Math.max(open, ss(0.32, 0.42, p) * (1 - ss(0.6, 0.72, p))); }
      // ---- gêiser
      { const p = (t - FX.geyser) / 1.5; if (p > 0 && p < 0.35 && Math.random() < 0.6) { const a = Math.random() * 6.283, d = Math.random() * 0.24; dot(new THREE.Vector3(Math.cos(a) * d, 0.02, TZ + Math.sin(a) * d), new THREE.Vector3(0, 0.6 + Math.random() * 0.5, 0), 0.016 + Math.random() * 0.02, '#a5f3fc', 0.35, 0); }
        const up = p >= 0.35 && p < 0.6 ? Math.sin((p - 0.35) / 0.25 * Math.PI / 2) : 0, dn = p >= 0.6 && p < 0.85 ? 1 - ((p - 0.6) / 0.25) ** 1.6 : 0, h = Math.max(up, dn); geyGroup.visible = h > 0.01; if (h > 0.01) { geyMat.uniforms.uT.value = t; const w = Math.max(0.1, h); geyCol.scale.set(w, GEY_H * h, w); foam.position.y = GEY_H * h; foam.scale.setScalar(w); if (p < 0.72) for (let d = 0; d < 3; d++) { const a = Math.random() * 6.283, rr = Math.random() * 0.17 * w; dot(new THREE.Vector3(Math.cos(a) * rr, GEY_H * h * (0.4 + Math.random() * 0.6), TZ + Math.sin(a) * rr), new THREE.Vector3(Math.cos(a) * (0.8 + Math.random()), 1 + Math.random() * 1.6, Math.sin(a) * 0.8), 0.018 + Math.random() * 0.024, Math.random() < 0.5 ? AQUA : '#ffffff', 0.7, 5); } if (dn > 0 && Math.random() < 0.08) ring(new THREE.Vector3(0, 0.02, TZ), 0.6, AQUA, 0.5, true); } }
      // ---- moscas
      { const p = pFl, on = p > 0 && p < 1; let tng = null; for (const f of flies) { f.g.visible = on && !(f.caught && p > f.cp + 0.05); if (!on) continue; const a = f.a0 + f.sp * 60 * (p * 1.7); if (!f.caught) { f.g.position.set(mp.x + Math.cos(a) * f.rx * K, 0.85 + (f.bh + Math.sin(a * 2.2) * f.ry) * K + 0.4, mp.z - 0.1 + Math.sin(a) * f.rz * K * 1.5 + 0.2); if (p >= f.cp) { f.caught = true; burst(f.g.position, 8, 0.9, 0.012, ['#ffffff', '#facc15'], 0.3, 0); } } else f.g.position.lerp(mp, 1 - Math.pow(0.45, dt * 60)); f.g.rotation.y = -a; const fl = Math.sin(t * 57); f.wings[0].rotation.z = -0.5 * fl; f.wings[1].rotation.z = 0.5 * fl; if (p > f.cp - 0.05 && p < f.cp + 0.04) tng = f; }
        if (on) { const near = flies.some(f => p > f.cp - 0.07 && p < f.cp + 0.05); if (near) open = Math.max(open, 0.85); if (tng) { const u = p < tng.cp ? win(tng.cp - 0.05, tng.cp, p) : 1 - win(tng.cp, tng.cp + 0.04, p), dest = tng.g.position, pts = []; for (let i = 0; i <= 16; i++) { const s = i / 16; pts.push(new THREE.Vector3().lerpVectors(mp, dest, s * u)); pts[i].y += Math.sin(s * Math.PI) * 0.05 * u; } drawTongue(pts, 0.05, 0.035); } else hideTongue(); } }
      // ---- rachaduras
      { const u = (t - FX.crack) / 6, on = u > 0 && u < 1; crack.visible = on; if (on) crack.scale.setScalar(easeO(Math.min(1, u * 14)) * (1 - ss(0.86, 1, u))); }
      // ---- coroa: assinatura e salto
      { let touched = false;
        if (pS(t) > 0 && pS(t) < 1) { const p = pS(t), take = ss(0, 0.14, p), down = ss(0.14, 0.28, p), up = ss(0.74, 0.88, p), rest = ss(0.88, 1, p), hold = take * (1 - rest), mv = down * (1 - up), lift = ss(0.1, 0.14, p) * (1 - down) * (1 - up), T = new THREE.Vector3(-0.56 * mv, C0.y + (1.15 - C0.y) * mv + 0.09 * lift, C0.z + (0.42 - C0.z) * mv); crownAt(T, 0.6 * mv); touched = true;
          const G = P.gloves[-1], tgt = new THREE.Vector3(T.x - 0.04, T.y - 0.17, T.z + 0.02); G.position.lerp(tgt, hold);
          const b = win(0.3, 0.72, p), act = b > 0 && b < 1, kk = act ? Math.abs(Math.sin(b * Math.PI * 4)) : 0, R = P.gloves[1], rt = new THREE.Vector3(0.07 - 0.04 * kk, 1.12, chestZ + 0.03 + 0.2 * (1 - kk)); R.position.lerp(rt, ss(0.26, 0.3, p) * (1 - ss(0.72, 0.78, p))); upper.scale.set(act ? 1 + 0.05 * (1 - kk) : 1, act ? 1 + 0.03 * kk : 1, act ? 1 + 0.05 * (1 - kk) : 1); if (act) open = Math.max(open, 0.35 + 0.2 * kk); bum.visible = act && kk > 0.8; if (bum.visible) { bum.position.set(0.72 + 0.05 * Math.sin(b * 40), 1.2, -0.3); bum.scale.setScalar(1.1); if (cam) bum.quaternion.copy(cam.quaternion); } else if (!act) upper.scale.set(1, 1, 1); } else { bum.visible = false; upper.scale.set(1, 1, 1); }
        if (lOn && pL >= 0.44 && pL < 0.84) { const u = (pL - 0.44) / 0.4; crownAt(new THREE.Vector3(0, C0.y + (3.9 - C0.y) * (1 - Math.pow(u, 2.2)) , C0.z), Math.sin(u * Math.PI * 4) * 0.18 * (1 - u)); touched = true; if (!FX.crownDone && u > 0.97) { FX.crownDone = true; } }
        if (lOn && pL >= 0.84 && !FX.crownSpark) { FX.crownSpark = true; burst(new THREE.Vector3(0, C0.y, 0), 20, 1.4, 0.016, ['#facc15', '#e11d48', '#38bdf8'], 0.5, 1); }
        if (!lOn) FX.crownSpark = false;
        if (!touched) crownHome(); }
      setOpen(open);
      // ---- pele venenosa: 6 s (ou fixa pelo botão)
      { const want = toxicOn || (t - FX.poison >= 0.25 && t - FX.poison < 6) ? 1 : 0, prev = PZ.amt.value, nx = prev + (want - prev) * Math.min(1, dt * (want ? 2.4 : 1.8)); PZ.amt.value = nx; PZ.t.value = t; if (Math.abs(nx - prev) > 1e-3) { for (const q of glovePz) q.m.color.copy(q.a).lerp(q.b, nx); for (const q of crownPz) q.m.color.copy(q.a).lerp(q.b, nx); for (const e of P.eyes) e.material.color.copy(eyeA).lerp(eyeB, nx); }
        const dr = (m, x, y, z, q, sc) => { m.visible = true; if (q < 0.7) { const l = (0.012 + 0.08 * q / 0.7) * sc; m.position.set(x, y - l, z); m.scale.set(0.017 * sc, l, 0.012 * sc); } else { const f = (q - 0.7) / 0.3, l = 0.022 * sc * (1 - f * 0.6); m.position.set(x, y - 0.1 * sc - f * f * 0.6, z); m.scale.set(0.016 * sc * (1 - f * 0.5), l, 0.016 * sc * (1 - f * 0.5)); } };
        const onD = nx > 0.5; for (const d of dripIn) { if (!onD) { d.m.visible = false; continue; } dr(d.m, d.x, d.y, d.z, (t * 0.45 + d.ph) % 1, 1); } for (const d of dripG2) { if (!onD) { d.m.visible = false; continue; } const G = P.gloves[d.sd].position; dr(d.m, G.x, G.y - 0.12, G.z, (t * 0.45 + d.ph) % 1, 1.15); } }
    };
    const pS = t => (t - FX.sign) / 3;
    const onFrame = c => {
      if (pulseT >= 0) { pulseT += c.dt; papoTarget = pulseT < 0.9 ? PAPO_STAGES[4] : 0; if (pulseT > 1.6) pulseT = -1; }
      for (let i = 0; i < 4; i++) { const h = c.dt / 4; papoV += ((papoTarget - PAPO.amt.value) * 140 - papoV * 9) * h; PAPO.amt.value = Math.max(0, PAPO.amt.value + papoV * h); }
      for (const sx of [-1, 1]) {
        const G = P.gloves[sx], k = Math.min(1, Math.max(0, c.pose[sx][0] / 0.3));
        G.position.copy(hands[sx].position);
        const yq = new THREE.Quaternion().setFromAxisAngle(UPV, upper.rotation.y * 0.8);
        const dir = new THREE.Vector3(0, 0.8, 0.6).lerp(new THREE.Vector3(0, -0.1, 1), k).normalize().applyQuaternion(yq);
        const zRef = FWD.clone().negate().lerp(UPV.clone().negate(), k).applyQuaternion(yq);
        const zA = zRef.addScaledVector(dir, -zRef.dot(dir)).normalize(), xA = new THREE.Vector3().crossVectors(dir, zA);
        G.quaternion.setFromRotationMatrix(new THREE.Matrix4().makeBasis(xA, dir, zA));
      }
      fxFrame(c);
    };
    return { P, upper, WAIST, neck, NECK, hips, HIPS, hands, hipSway: 0.4, hand: { x: 0.6, y: 1.1, z: 0.46, yaw: -0.12 }, placeFoot, setFace, skills, onFrame };
  },
};
