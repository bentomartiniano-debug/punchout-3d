// Valéria Vulcânica no modo vinil: modelada com volume de verdade, na proporção exata do sprite de frente (quadro de
// 480 px): cabelo = massa redonda com um rebaixo onde o rosto encaixa (capuz), rosto = volume próprio com o desenho do
// jogo, top = camada por cima do tronco, saia = 3 camadas de babado, pernas saindo do meio da saia, sapatos,
// guarda-chuvas flutuando (no jogo ela não tem braços). Medidas em px do sprite; S px = 1 m.
import { THREE, ss, lin, rrect, INK, INKS, ramp, flat, vcol, withOutline, legPlacer, loadSprite } from './core.js';
import { GLTFLoader } from 'three/addons/loaders/GLTFLoader.js';
import { RoundedBoxGeometry } from 'three/addons/geometries/RoundedBoxGeometry.js';
import { VALERIA, valeriaBody } from './base.js';

const tierShade = { w: { value: 5.5 }, k: { value: 0.25 } };   // sombra embaixo de cada babado: largura (px do sprite) e força
const inkPush = { value: 0.015 };   // quanto a casca do cabelo recua na linha de visão (m)

export default {
  title: 'Valéria',
  arena: 'pernambuco',   // abre na arena própria (core.js)
  units: [245, 440, 240],
  faces: [['normal', 'Normal'], ['angry', 'Raiva'], ['pain', 'Dor'], ['ko', 'K.O.']],
  skills: [['signature', 'Assinatura'], ['magma', 'Soco Magmático'], ['erupt', 'Erupção Solavanco'], ['guard', 'Escudo de Calor'], ['kick', 'Estocada de Lava'], ['ash', 'Erupção de Cinzas'], ['phoenix', 'Reagrupamento das Brasas']],
  hudExtra: '<button data-atk="umbL">Golpe E</button><button data-atk="umbR">Golpe D</button> Forma: <button id="cold">Magma Frio</button>',
  sliders: [['tierW', 'Babado: sombra largura', 2, 28, 0.5, tierShade.w], ['tierK', 'Babado: sombra força', 0, 1, 0.01, tierShade.k]],
  others: [['frog', 'King Frog']],
  async build({ root, clock, U }) {
  const { X, Y, D, S, FOOT, loft } = U;
  const { frameCanvas, cleanFill, rowsOf } = await loadSprite('valeria', 480);
  // --- shader de lava e rosto em código (os mesmos do visualizador da Valéria 3D)
  const lavaU = { time: { value: 0 }, heat: { value: 0 }, lag: { value: new THREE.Vector3() } };
  // ruído e células 3D para a textura de magma (placas com veios)
  const LAVA_GLSL = `
  vec3 lavaH(vec3 p) { p = vec3(dot(p, vec3(127.1, 311.7, 74.7)), dot(p, vec3(269.5, 183.3, 246.1)), dot(p, vec3(113.5, 271.9, 124.6))); return fract(sin(p) * 43758.5453); }
  float lavaNoise(vec3 p) { vec3 i = floor(p), f = fract(p); f = f * f * (3.0 - 2.0 * f);
    float n = mix(mix(mix(lavaH(i).x, lavaH(i + vec3(1,0,0)).x, f.x), mix(lavaH(i + vec3(0,1,0)).x, lavaH(i + vec3(1,1,0)).x, f.x), f.y),
                  mix(mix(lavaH(i + vec3(0,0,1)).x, lavaH(i + vec3(1,0,1)).x, f.x), mix(lavaH(i + vec3(0,1,1)).x, lavaH(i + vec3(1,1,1)).x, f.x), f.y), f.z);
    return n * 2.0 - 1.0; }
  // balanço do cabelo: o tufo do rabo e as pontas de baixo ficam para trás quando a cabeça mexe, e ondulam de leve
  vec3 hairSway(vec3 p, float lock, vec3 lmin, vec3 lmax, vec3 lag, float t) {
    vec3 q = (p - lmin) / (lmax - lmin);
    float w = lock > 1.5 && lock < 3.0 ? pow(clamp((lock - 2.0) / 0.9, 0.0, 1.0), 1.3) * 1.6 + 0.2
            : lock > 3.0 ? 0.15 : lock > 0.5 ? 0.25 * (1.0 - q.y) : 0.6 * pow(1.0 - q.y, 2.0);
    vec3 wob = vec3(sin(t * 2.1 + q.y * 5.0), 0.0, cos(t * 1.7 + q.x * 4.0)) * 0.006 * w;
    return p + (lag + wob) * w;
  }
  // placas de magma: x = distância ao centro da placa, y = distância de verdade até a borda (linha nítida em qualquer zoom);
  // os centros borbulham devagar (t), então as placas mudam de forma enquanto escorrem
  vec3 lavaPt(vec3 c, float t) { return 0.5 + 0.38 * sin(t + 6.2831 * lavaH(c)); }
  vec2 lavaCells(vec3 p, float t) { vec3 i = floor(p), f = fract(p), mr = vec3(0.0), mg = vec3(0.0); float md = 8.0;
    for (int x = -1; x <= 1; x++) for (int y = -1; y <= 1; y++) for (int z = -1; z <= 1; z++) {
      vec3 g = vec3(x, y, z), r = g + lavaPt(i + g, t) - f; float d = dot(r, r);
      if (d < md) { md = d; mr = r; mg = g; } }
    float ed = 8.0;
    for (int x = -1; x <= 1; x++) for (int y = -1; y <= 1; y++) for (int z = -1; z <= 1; z++) {
      vec3 g = mg + vec3(x, y, z), r = g + lavaPt(i + g, t) - f;
      if (dot(mr - r, mr - r) > 1e-5) ed = min(ed, dot(0.5 * (mr + r), normalize(r - mr))); }
    return vec2(length(mr), ed); }
  `;
  function lavaShader(mat, box) {
    const prev = mat.onBeforeCompile;
    mat.onBeforeCompile = (sh, r) => {
      prev(sh, r);
      Object.assign(sh.uniforms, { uLavaT: lavaU.time, uHeat: lavaU.heat, uLag: lavaU.lag, uLMin: { value: box.min.clone() }, uLMax: { value: box.max.clone() } });
      sh.vertexShader = 'varying vec3 vLavaP; varying float vLock; attribute float _lock; uniform float uLavaT; uniform vec3 uLMin, uLMax, uLag;' + LAVA_GLSL + sh.vertexShader.replace('#include <begin_vertex>', '#include <begin_vertex>' + String.fromCharCode(10) + 'vLavaP = position; vLock = _lock; transformed = hairSway(position, _lock, uLMin, uLMax, uLag, uLavaT);');
      sh.fragmentShader = 'uniform float uLavaT, uHeat; uniform vec3 uLMin, uLMax; varying vec3 vLavaP; varying float vLock;' + LAVA_GLSL + sh.fragmentShader
        .replace('#include <map_fragment>', `#include <map_fragment>
          vec3 q = (vLavaP - uLMin) / (uLMax - uLMin);                 // 0..1 dentro da caixa do cabelo
          float flow = 0.045 * sin(q.x * 13.0 + uLavaT * 1.3) * sin(q.z * 11.0 - uLavaT * 0.9) + 0.03 * sin((q.y * 9.0 - uLavaT * 0.7) * 3.0);
          float t = clamp(q.y + flow, 0.0, 1.0);
          float core = smoothstep(0.62, 0.95, q.y) * (1.0 - smoothstep(0.12, 0.42, length(q.xz - vec2(0.5, 0.45))));
          vec3 c = mix(vec3(0.55, 0.04, 0.10), vec3(0.88, 0.16, 0.08), smoothstep(0.05, 0.35, t));
          c = mix(c, vec3(1.0, 0.45, 0.04), smoothstep(0.35, 0.62, t));
          c = mix(c, vec3(1.0, 0.76, 0.12), smoothstep(0.62, 0.85, t));
          c = mix(c, vec3(1.0, 0.95, 0.80), core);
          if (vLock > 0.5 && vLock < 1.5) c = mix(vec3(1.0, 0.42, 0.05), vec3(1.0, 0.84, 0.16), smoothstep(0.42, 0.72, q.y + flow));   // franja: amarelo com ponta laranja
          else if (vLock > 1.5 && vLock < 3.0) {   // tufo do rabo: laranja embaixo, amarelo, creme no alto (altura ao longo do rabo)
            float h = clamp((vLock - 2.0) / 0.9 + flow, 0.0, 1.0);
            c = mix(vec3(1.0, 0.45, 0.05), vec3(1.0, 0.82, 0.14), smoothstep(0.0, 0.45, h)); c = mix(c, vec3(1.0, 0.95, 0.78), smoothstep(0.6, 0.95, h));
          } else if (vLock > 3.0) c = vec3(0.23, 0.13, 0.08);   // prendedor marrom-escuro
          if (vLock < 3.0) {   // textura de magma suave: placas um pouco mais escuras, veios quentes entre elas (mesma cor, só o brilho muda)
            // escorre: as placas descem do alto da cabeça para as pontas, com um redemoinho lento por cima
            vec3 flowP = q * vec3(5.5, 6.5, 5.5) + vec3(0.0, uLavaT * 0.22, 0.0);
            vec3 warp = vec3(lavaNoise(q * 2.5 + vec3(0.0, uLavaT * 0.12, 0.0)), lavaNoise(q * 2.5 + vec3(5.2, uLavaT * 0.12, 1.3)), lavaNoise(q * 2.5 + vec3(1.7, uLavaT * 0.12, 9.2)));
            vec2 vr = lavaCells(flowP + warp * 0.45, uLavaT * 0.5);
            float aa = fwidth(vr.y);
            float vein = 1.0 - smoothstep(0.045 - aa, 0.045 + aa, vr.y);          // veio: linha nítida na borda das placas
            vein = max(vein, 0.18 * (1.0 - smoothstep(0.045, 0.16, vr.y)));      // brilho curto em volta do veio
            float mott = lavaNoise(flowP * 2.4) * 0.5 + 0.5;                     // manchinhas dentro da placa, descendo junto
            vec3 hot = min(c * (1.25 + 0.15 * uHeat) + vec3(0.10, 0.08, 0.02) * (1.0 + 0.8 * uHeat), vec3(1.0));
            c = mix(c * (0.90 + 0.08 * mott), hot, vein * (0.55 + 0.45 * uHeat));
            c += vec3(0.30, 0.12, 0.0) * uHeat * vein * (0.6 + 0.4 * sin(uLavaT * 18.0));   // veios pulsam enquanto carrega
          }
          diffuseColor.rgb = c;`)
        .replace('#include <opaque_fragment>', 'outgoingLight = mix(outgoingLight, diffuseColor.rgb, 0.35);   // brilha por conta própria (o volume ainda aparece)' + String.fromCharCode(10) + '#include <opaque_fragment>');
    };
    mat.customProgramCacheKey = () => 'lava';
    mat.needsUpdate = true;
  }
  const V_GOLD = '#f4c430', V_YEL = '#ffd21f', V_RED = '#e0502a';
  function rrect(g, x, y, w, h, r) { g.beginPath(); g.moveTo(x + r, y); g.arcTo(x + w, y, x + w, y + h, r); g.arcTo(x + w, y + h, x, y + h, r); g.arcTo(x, y + h, x, y, r); g.arcTo(x, y, x + w, y, r); g.closePath(); }
  // side: -1 olho da esquerda da tela, +1 da direita (o lado de dentro fica perto do nariz)
  function valEye(g, side, ex) {
    const inner = side < 0 ? 1 : -1;            // direção do nariz no canvas
    g.lineCap = 'round'; g.lineJoin = 'round';
    // risco dourado (sobrancelha): reto no normal, desce para dentro na raiva
    const bIn = ex === 'angry' || ex === 'grit' ? 30 : ex === 'pain' ? 40 : 36, bOut = ex === 'angry' || ex === 'grit' ? 18 : ex === 'pain' ? 24 : 34;
    const xi = 64 + inner * 46, xo = 64 - inner * 46;
    g.strokeStyle = INKS; g.lineWidth = 16; g.beginPath(); g.moveTo(xo, bOut); g.lineTo(xi, bIn); g.stroke();
    g.strokeStyle = V_GOLD; g.lineWidth = 8; g.beginPath(); g.moveTo(xo, bOut); g.lineTo(xi, bIn); g.stroke();
    if (ex === 'ko') {                          // X
      g.strokeStyle = INKS; g.lineWidth = 13; g.beginPath(); g.moveTo(36, 54); g.lineTo(92, 102); g.moveTo(92, 54); g.lineTo(36, 102); g.stroke();
    } else if (ex === 'pain') {                 // olho espremido: > <
      g.strokeStyle = INKS; g.lineWidth = 12; g.beginPath(); g.moveTo(64 - inner * 30, 58); g.lineTo(64 + inner * 26, 78); g.lineTo(64 - inner * 30, 98); g.stroke();
    } else {
      const lid = ex === 'squint' ? 20 : ex === 'angry' || ex === 'grit' ? 8 : 0;
      const y0 = 52 + lid, h = 50 - lid;
      g.fillStyle = INKS; rrect(g, 22, y0 - 7, 84, h + 14, 16); g.fill();
      g.fillStyle = V_YEL; rrect(g, 30, y0, 68, h, 10); g.fill();
      if (lid) { g.fillStyle = INKS; g.beginPath(); g.moveTo(30, y0); g.lineTo(98, y0); g.lineTo(64 + inner * 34, y0 + (ex === 'squint' ? 6 : 12)); g.lineTo(64 - inner * 34, y0); g.fill(); }
      g.fillStyle = '#ffffff'; g.fillRect(64 - inner * 10 - 9, y0 + 9, 16, 16);   // brilho
    }
    if (side < 0) {                             // cicatriz na bochecha da esquerda
      g.strokeStyle = INKS; g.lineWidth = 7; g.beginPath(); g.moveTo(8, 104); g.lineTo(30, 124); g.moveTo(20, 98); g.lineTo(42, 118); g.stroke();
    }
  }
  function valMouth(g, ex) {                    // canvas 256x90 (a peça da boca tem altura 0,35 da largura)
    g.lineCap = 'round'; g.lineJoin = 'round'; g.strokeStyle = INKS; g.lineWidth = 9;
    if (ex === 'normal') {                      // sorriso aberto com dentes
      g.fillStyle = '#ffffff'; g.beginPath(); g.moveTo(52, 22); g.quadraticCurveTo(128, 34, 204, 22); g.quadraticCurveTo(190, 82, 128, 82); g.quadraticCurveTo(66, 82, 52, 22); g.closePath(); g.fill(); g.stroke();
    } else if (ex === 'angry' || ex === 'grit' || ex === 'squint') {   // dentes cerrados
      g.fillStyle = '#ffffff'; rrect(g, 70, 26, 116, 40, 12); g.fill(); g.stroke();
      g.lineWidth = 5; g.beginPath(); g.moveTo(74, 46); g.lineTo(182, 46); for (const x of [98, 128, 158]) { g.moveTo(x, 28); g.lineTo(x, 64); } g.stroke();
    } else if (ex === 'pain') {                 // boca torta ondulada
      g.beginPath(); g.moveTo(74, 52); g.quadraticCurveTo(92, 30, 110, 52); g.quadraticCurveTo(128, 72, 146, 52); g.quadraticCurveTo(164, 30, 182, 52); g.stroke();
    } else {                                    // K.O.: "oh"
      g.fillStyle = '#5a1a14'; g.beginPath(); g.ellipse(128, 48, 22, 28, 0, 0, 7); g.fill(); g.stroke();
    }
  }
  const valNose = g => {                        // nariz: ponta arredondada com sombra embaixo (canvas 128)
    g.lineCap = 'round'; g.strokeStyle = INKS; g.lineWidth = 8;
    g.fillStyle = 'rgba(120,55,30,0.45)'; g.beginPath(); g.ellipse(64, 84, 26, 10, 0, 0, 7); g.fill();
    g.beginPath(); g.moveTo(48, 80); g.quadraticCurveTo(64, 96, 80, 80); g.stroke();
    g.lineWidth = 6; g.beginPath(); g.moveTo(62, 30); g.quadraticCurveTo(54, 58, 56, 72); g.stroke();
  };

  // ================================================================ montagem
  const FACES = { normal: [['valeriaRigF0_eyesN'], 'valeriaRigM_grin'], angry: [['valeriaRigF0_eyesN', 'valeriaRigE_brows'], 'valeriaRigM_grit'],
                  pain: [['valeriaRigE_pain'], 'valeriaRigM_pain'], ko: [['valeriaRigE_x'], 'valeriaRigM_oh'] };
  const isSkin = (r, g, b, a) => a > 120 && r > 140 && r < 205 && g > 50 && g < 110 && b < 70;
  const isHair = (r, g, b, a) => a > 120 && r > 150 && (g < 45 || r > 205) && !(r > 200 && g > 180 && b < 40 && false);
  const headF = frameCanvas('valeriaRigF0_head'), headB = frameCanvas('valeriaRigB0_head');
  const hairRows = rowsOf(headF, (r, g, b, a) => a > 120);
  const faceRows = rowsOf(headF, isSkin, 140, 250);
  const faceHalf = y => { let best = null; for (const r of faceRows) if (!best || Math.abs(r.y - y) < Math.abs(best.y - y)) best = r; return best && Math.abs(best.y - y) < 8 ? (best.r - best.l) / 2 : 0; };
  const FACE_BOT = faceRows.length ? faceRows[faceRows.length - 1].y : 200, FACE_LAST = faceRows.length ? (faceRows[faceRows.length - 1].r - faceRows[faceRows.length - 1].l) / 2 + 6 : 50;
  const hairF = cleanFill(headF, isHair), hairB = cleanFill(headB, isHair);
  const P = {};
  const HEAD_UP = D(18);
  const HAIR_FIT = 1.0, HAIR_UP = 0.11, HAIR_FWD = 0.0, FACE_W = 107, EYE_Y = 160;

  // cor da lava como no sprite: anéis em volta de um miolo creme no alto (frente), indo a amarelo, laranja, vermelho e vinho
  const LAVA = [[0, [1.0, 0.95, 0.82]], [0.3, [1.0, 0.85, 0.2]], [0.5, [1.0, 0.55, 0.05]], [0.7, [0.94, 0.2, 0.06]], [0.86, [0.82, 0.04, 0.16]], [1.1, [0.55, 0.0, 0.12]]];
  function lava(x, y, z) {
    const r = Math.hypot((x - 240) / 118, (y - 72) / 175, Math.min(0, z - 40) / 420);
    for (let i = 0; i < LAVA.length - 1; i++) { const [a, ca] = LAVA[i], [b, cb] = LAVA[i + 1]; if (r <= b) { const t = ss(a, b, r); return ca.map((v, k) => v + (cb[k] - v) * t); } }
    return LAVA[LAVA.length - 1][1];
  }
  // cabelo: o da Valéria 3D (make_valeria_hair.py: capuz, franja repartida, rabo de cavalo, lava), ajustado à cabeça nova
  const HAIR_D = 0.82, FACE_CZ = 34, FACE_D = 0.62;
  const YOKE_D = 0.62, YOKE_CZ = -2;
  const B = valeriaBody(U, VALERIA), { faceA, yokeA, neckZ } = B;   // o corpo dela é a base de todos (base.js)
  {
    const gl = await new GLTFLoader().loadAsync('../../assets3d/valeria/clean/prop_valeria_hair.glb');
    const hairObj = gl.scene; let mesh = null; hairObj.traverse(o => { if (o.isMesh) mesh = o; });
    mesh.geometry.computeBoundingBox(); const bb = mesh.geometry.boundingBox;
    const m = new THREE.MeshToonMaterial({ color: 0xffffff, gradientMap: ramp }); lavaShader(m, bb); mesh.material = m;
    const hm = new THREE.MeshBasicMaterial({ color: INK, side: THREE.BackSide });
    hm.onBeforeCompile = sh => {   // contorno acompanha o balanço do cabelo
      Object.assign(sh.uniforms, { uLavaT: lavaU.time, uLag: lavaU.lag, uLMin: { value: bb.min.clone() }, uLMax: { value: bb.max.clone() } });
      sh.uniforms.uInkPush = inkPush;
      sh.vertexShader = 'attribute float _lock; uniform float uLavaT, uInkPush; uniform vec3 uLMin, uLMax, uLag;' + LAVA_GLSL + sh.vertexShader.replace('#include <begin_vertex>', '#include <begin_vertex>\ntransformed = hairSway(position, _lock, uLMin, uLMax, uLag, uLavaT) + normal * 0.006;')
        // casca recuada 1,5 cm na linha de visão: some nas dobras entre os gomos (sujeirinhas pretas), fica na silhueta
        .replace('#include <project_vertex>', 'vec4 mvPosition = modelViewMatrix * vec4(transformed, 1.0); mvPosition.xyz += normalize(mvPosition.xyz) * uInkPush; gl_Position = projectionMatrix * mvPosition;');
    };
    hm.customProgramCacheKey = () => 'lavaInk';
    const hairHull = new THREE.Mesh(mesh.geometry, hm); hairHull.userData.hairHull = true; mesh.add(hairHull);
    // o cabelo foi feito em volta do rosto da Valéria 3D: escala pela largura do rosto e põe a abertura dele sobre o rosto novo
    // (rosto antigo: 0,32 m de largura, olhos em y 1,63, frente em z 0,15)
    const sc = D(FACE_W) / 0.32 * HAIR_FIT;
    hairObj.scale.setScalar(sc);
    hairObj.position.set(X(240) - 0 * sc, Y(EYE_Y) - 1.63 * sc + HAIR_UP, D(FACE_CZ + FACE_D * FACE_W / 2) - 0.15 * sc + HAIR_FWD);
    P.hair = hairObj; P.hairScale = sc;
  }
  // rosto: o ovo da base (rosto, pescoço e ombros), pintado com o desenho do jogo (olhos/nariz/boca)
  const EYE_L = [218, 160], EYE_R = [262, 160], EYE_S = 44, NOSE_Y = 176, MOUTH_Y = 190, MOUTH_W = 52;
  const drawFace = f => (g, at) => {
    at(EYE_L[0], EYE_L[1], EYE_S, EYE_S, gg => valEye(gg, -1, f), 128, 128);
    at(EYE_R[0], EYE_R[1], EYE_S, EYE_S, gg => valEye(gg, 1, f), 128, 128);
    at(240, NOSE_Y, 22, 22, valNose, 128, 128);
    at(240, MOUTH_Y, MOUTH_W, MOUTH_W * 90 / 256, gg => valMouth(gg, f), 256, 90);
  };
  const faceMat = B.faceMat; faceMat.map.dispose(); faceMat.map = B.faceTexture(drawFace('normal'));
  P.face = B.face; P.faceMat = faceMat;
  // tronco (barriga marrom) e top laranja por cima
  {
    // pescoço e ombros (pele): sai de baixo do queixo e abre em curva até a largura dos ombros, descendo para dentro do top
    const top = [];
    P.torso = B.torso;
    // top: faixa (tomara que caia) que começa nos ombros, com volume de busto; faixa amarela pintada em cima, borda com
    // contorno preto e uma tampa por dentro até a pele (de cima não aparece vão nenhum)
    const topA = y => 66 + 2 * Math.exp(-(((y - 232) / 8) ** 2)) - 6 * ss(236, 252, y);   // colado no corpo: ombros -> peito -> barra justa no tronco
    const bust = (x, y, z, f) => z + (f > 0 ? 5 * Math.exp(-(((y - 233) / 10) ** 2)) * f * f * (0.6 + 0.4 * Math.cos((x - 240) / 34 * Math.PI)) : 0);
    const T0 = 216, TY = 224;                              // borda de cima / fim da faixa amarela
    // barra justa: nas últimas fileiras o top encosta na barriga (mesma elipse do tronco, 1,5 px por fora) — sem fenda embaixo
    const HEM = y => ss(242, 252, y);
    const tA = y => { const a = Math.max(y <= 232 ? yokeA(y) + 2.5 : 0, topA(y)); return a + (58.5 - a) * HEM(y); };
    const tZ = (x, y, z, f) => { const k = HEM(y), a = tA(y); return bust(x, y, z, f) + k * (-2 - 0.02 * a * f); };
    const ain = yokeA(T0) - 1.5;                            // tampa: da borda até a pele do ombro
    // a tampa fecha até o centro (pele do pescoço): a cabeça fica um pouco acima e, de cima, via-se o oco do top
    for (const [a, yel] of [[2, 2], [ain, 2], [ain, 1]]) top.push({ y: T0, l: 240 - a, r: 240 + a, zf: (x, y, z, f) => neckZ(T0) + YOKE_D * a * f, yel });
    for (const [y, yel] of [[T0, 1], [218, 1], [220, 1], [222, 1], [TY, 1], [TY, 0]]) top.push({ y, l: 240 - tA(y), r: 240 + tA(y), yel });
    for (let y = 226; y <= 252; y += 2) top.push({ y, l: 240 - tA(y), r: 240 + tA(y), yel: 0 });
    const YEL = lin([0.95, 0.76, 0.1]), ORG = lin([0.94, 0.29, 0.05]), NECK = lin([0x8f / 255, 0x4a / 255, 0x2c / 255]);
    const tg = loft(top, { depth: 0.74, cz: -2, capTop: false, capBot: false, zFn: tZ });
    const tc = []; for (const r of top) for (let j = 0; j <= 40; j++) tc.push(...(r.yel === 2 ? NECK : r.yel ? YEL : ORG));
    tg.setAttribute('color', new THREE.Float32BufferAttribute(tc, 3));
    P.top = withOutline(new THREE.Mesh(tg, vcol()), 0.009);
    const edge = []; for (let j = 0; j <= 64; j++) { const th = j / 64 * Math.PI * 2, a = tA(T0), x = 240 + a * Math.cos(th), f = Math.sin(th); edge.push(new THREE.Vector3(X(x), Y(T0), D(tZ(x, T0, -2 + a * 0.74 * f, f)))); }
    P.collar = new THREE.Mesh(new THREE.TubeGeometry(new THREE.CatmullRomCurve3(edge, true), 96, D(1.8), 6, true), new THREE.MeshBasicMaterial({ color: INK }));   // contorno da borda de cima
    // "^" preto no meio do top (como no sprite)
    const vpts = [[226, 240], [240, 229], [254, 240]].map(([x, y]) => { const a = tA(y), dx = x - 240, f = Math.sqrt(Math.max(0, 1 - (dx / a) ** 2)); return new THREE.Vector3(X(x), Y(y), D(bust(x, y, -2 + a * 0.74 * f, f) + 1.5)); });
    P.chev = new THREE.Mesh(new THREE.TubeGeometry(new THREE.CatmullRomCurve3(vpts, false, 'catmullrom', 0), 12, D(2), 6), new THREE.MeshBasicMaterial({ color: INK }));
    const belt = []; for (let y = 282; y <= 292; y += 2) belt.push({ y, l: 240 - 62, r: 240 + 62 });
    P.belt = withOutline(new THREE.Mesh(loft(belt, { depth: 0.72, cz: -4, capTop: false, capBot: false }), flat(0x3a1a12)), 0.006);
  }
  // saia: 3 camadas de babado (cone aberto, barra ondulada), cada uma por cima da de baixo
  // forma de um babado: altura e raio no ângulo th e no parâmetro t (0 = cós, 1 = barra ondulada)
  const tierY = (T, t, th) => T.y0 + (T.y1 - T.y0) * t + T.wav * t * t * (0.5 + 0.5 * Math.cos(T.nW * th));
  const tierR = (T, t, th) => (T.a0 + (T.a1 - T.a0) * t) * (1 + 0.035 * t * Math.sin(T.nW * th));
  // raio máximo que o babado de baixo pode ter na altura yy sem furar o de cima (por dentro dele; depois da barra
  // vai abrindo aos poucos, sem degrau)
  function tierLimit(U, th, yy) {
    const hy = tierY(U, 1, th);
    if (yy >= hy) return tierR(U, 1, th) - 3 + 1.6 * (yy - hy);
    let lo = 0, hi = 1; for (let k = 0; k < 24; k++) { const m = (lo + hi) / 2; if (tierY(U, m, th) < yy) lo = m; else hi = m; }
    return tierR(U, lo, th) - 3;
  }
  const softMin = (r, lim, k = 2) => lim - k * Math.log(1 + Math.exp((lim - r) / k));
  function tier(y0, y1, a0, a1, color, wav = 9, nW = 10, over = null, hemAbove = 0, hemWav = wav) {   // over: babado de cima; hemAbove: linha (y) que faz sombra aqui
    const T = { y0, y1, a0, a1, wav, nW };
    const NA = 160, NV = 16, pos = [], idx = [], ang = [];
    for (let i = 0; i <= NV; i++) {
      const t = i / NV;
      for (let j = 0; j <= NA; j++) {
        const th = j / NA * Math.PI * 2, yy = tierY(T, t, th);
        let r = tierR(T, t, th);
        if (over) r = softMin(r, tierLimit(over, th, yy));
        pos.push(X(240 + r * Math.cos(th)), Y(yy), D(-4 + r * 0.74 * Math.sin(th)));
        ang.push(th, t);
      }
    }
    for (let i = 0; i < NV; i++) for (let j = 0; j < NA; j++) { const a = i * (NA + 1) + j, b = a + 1, c = a + NA + 1, d = c + 1; idx.push(a, b, c, b, d, c); }
    const g = new THREE.BufferGeometry(); g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3)); g.setIndex(idx); g.computeVertexNormals();
    g.setAttribute('fold', new THREE.Float32BufferAttribute(ang, 2));
    const m = flat(color); m.side = THREE.DoubleSide;
    m.onBeforeCompile = sh => {   // riscos pretos nas dobras (vales das pregas), mais fortes perto da barra
      sh.vertexShader = 'attribute vec2 fold; varying vec2 vFold;\n' + sh.vertexShader.replace('#include <begin_vertex>', '#include <begin_vertex>\nvFold = fold;');
      Object.assign(sh.uniforms, { uTierW: tierShade.w, uTierK: tierShade.k });
      sh.fragmentShader = 'varying vec2 vFold; uniform float uTierW, uTierK;\n' + sh.fragmentShader.replace('#include <map_fragment>', '#include <map_fragment>\n' +
        'float fs = sin(' + nW.toFixed(1) + ' * vFold.x); float w = mix(0.0, 0.06, smoothstep(0.15, 0.9, vFold.y));\n' +
        'if (fs < -1.0 + w && vFold.y > 0.15) diffuseColor.rgb = vec3(0.06, 0.1, 0.06);\n' +
        (hemAbove ? 'float yy = ' + y0.toFixed(1) + ' + ' + (y1 - y0).toFixed(1) + ' * vFold.y + ' + wav.toFixed(1) + ' * vFold.y * vFold.y * (0.5 + 0.5 * cos(' + nW.toFixed(1) + ' * vFold.x));' +
          'float hemU = ' + hemAbove.toFixed(1) + ' + ' + hemWav.toFixed(1) + ' * (0.5 + 0.5 * cos(' + nW.toFixed(1) + ' * vFold.x));' +
          'extraTone = uTierK * (1.0 - smoothstep(hemU + uTierW * 0.35, hemU + uTierW, yy));' : ''));   // faixa de sombra logo abaixo da barra de cima, seguindo a onda dela
    };
    m.customProgramCacheKey = () => 'tier' + y0 + '|' + hemAbove;
    const mesh = withOutline(new THREE.Mesh(g, m), 0.008); mesh.userData.T = T; return mesh;
  }
  P.skirt1 = tier(286, 324, 66, 88, 0xf04a0c, 9, 10, null, 286, 0);                     // sombra no alto, junto da cintura
  P.skirt2 = tier(318, 346, 82, 106, 0xf2c21a, 9, 10, P.skirt1.userData.T, 324);
  P.skirt3 = tier(340, 378, 98, 122, 0x2fd0c0, 9, 10, P.skirt2.userData.T, 346);
  const under = flat(0x2a1210); under.side = THREE.DoubleSide;   // fundo escuro dentro da saia
  P.under = new THREE.Mesh(new THREE.CircleGeometry(D(118), 40), under); P.under.rotation.x = Math.PI / 2; P.under.position.set(0, Y(372), D(-4)); P.under.scale.y = 0.74;
  // pernas saindo do meio da saia e sapatos: os da base
  P.legL = B.legs[-1]; P.legR = B.legs[1]; P.bootL = B.feet[-1]; P.bootR = B.feet[1];
  // guarda-chuvas: copa de 8 gomos (laranja, laranja claro, amarelo, turquesa) com varetas brancas, botão dourado na ponta,
  // cabo atrás. A cor sai de um shader (gomo e vareta pela posição na copa): borda nítida em qualquer distância.
  // Em repouso fica fechado (varetas deitadas no cabo, pano em pregas); no golpe abre, avança e gira, depois fecha.
  const UNG = 8, UNS = 16, UNC = UNG * 12, U_L = D(78), U_TIP = D(95);
  const lerp = (a, b, k) => a + (b - a) * k;
  function umbCanopy() {
    const n = (UNS + 1) * (UNC + 1), gp = new Float32Array(n), idx = [];
    for (let i = 0; i <= UNS; i++) for (let j = 0; j <= UNC; j++) gp[i * (UNC + 1) + j] = j / UNC * UNG;
    for (let i = 0; i < UNS; i++) for (let j = 0; j < UNC; j++) {
      const a = i * (UNC + 1) + j, b = a + 1, c = a + UNC + 1, d = c + 1; idx.push(a, b, c, b, d, c);
    }
    const g = new THREE.BufferGeometry();
    g.setAttribute('position', new THREE.BufferAttribute(new Float32Array(n * 3), 3)); g.setAttribute('gp', new THREE.BufferAttribute(gp, 1)); g.setIndex(idx);
    const m = new THREE.MeshToonMaterial({ gradientMap: ramp, side: THREE.DoubleSide });
    const C = [[1, 0.55, 0.05], [1, 0.29, 0], [0.18, 0.82, 0.75], [0.95, 0.77, 0.1]].map(c => new THREE.Vector3(...lin(c)));   // cima-dir, cima-esq, baixo-esq, baixo-dir
    m.onBeforeCompile = sh => {
      sh.uniforms.uC = { value: C };
      sh.vertexShader = 'attribute float gp;\nvarying float vGp;\n' + sh.vertexShader.replace('#include <begin_vertex>', '#include <begin_vertex>\nvGp = gp;');
      sh.fragmentShader = 'varying float vGp;\nuniform vec3 uC[4];\n' + sh.fragmentShader.replace('#include <color_fragment>', `#include <color_fragment>
        int q = int(clamp(floor(vGp / 2.0), 0.0, 3.0));
        vec3 col = q == 0 ? uC[0] : q == 1 ? uC[1] : q == 2 ? uC[2] : uC[3];
        float t = fract(vGp), e = min(t, 1.0 - t), aa = fwidth(vGp) * 0.75;
        diffuseColor.rgb = mix(col, vec3(1.0), 1.0 - smoothstep(0.07 - aa, 0.07 + aa, e));`);
    };
    m.customProgramCacheKey = () => 'umbCanopy';
    const mesh = new THREE.Mesh(g, m);
    mesh.userData.setOpen = o => {                        // o: 0 fechado → 1 aberto
      const a0 = lerp(140, 95, o) * Math.PI / 180, kp = lerp(44, 45, o) * Math.PI / 180, sag = lerp(0.25, 0.07, o);
      const L = U_L * lerp(1.25, 1, o);                   // fechado: mais comprido e gordo, cobre a mão
      const p = g.attributes.position.array;
      for (let i = 0; i <= UNS; i++) {
        const s = i / UNS, a1 = a0 + kp * s;
        const r = L * (Math.cos(a0) - Math.cos(a1)) / kp, z = U_TIP + L * (Math.sin(a1) - Math.sin(a0)) / kp;   // vareta curva a partir da ponta
        for (let j = 0; j <= UNC; j++) {
          const gpv = j / UNC * UNG, f = Math.PI * 2 * gpv / UNG, rr = r * (1 - sag * Math.sin(Math.PI * (gpv % 1)));   // pano cede entre as varetas
          const k = (i * (UNC + 1) + j) * 3; p[k] = rr * Math.cos(f); p[k + 1] = rr * Math.sin(f); p[k + 2] = z;
        }
      }
      g.attributes.position.needsUpdate = true; g.computeVertexNormals(); g.computeBoundingSphere();
    };
    mesh.userData.setOpen(0);
    return mesh;
  }
  function umbrella() {
    const u = new THREE.Group(), spin = new THREE.Group();
    const canopy = umbCanopy(); spin.add(withOutline(canopy, 0.008));
    const knob = withOutline(new THREE.Mesh(new THREE.SphereGeometry(D(9), 16, 12), flat(0xf2c21a)), 0.006); knob.position.z = U_TIP + D(3); spin.add(knob);
    u.add(spin);
    const shaft = withOutline(new THREE.Mesh(new THREE.CylinderGeometry(D(5), D(5), D(115), 10), flat(0x6a3c1c)), 0.005);   // cabo no eixo, da ponta para trás
    shaft.rotation.x = Math.PI / 2; shaft.position.z = D(95 - 57.5); u.add(shaft);
    const hook = withOutline(new THREE.Mesh(new THREE.TorusGeometry(D(16), D(6), 8, 18, Math.PI), flat(0x7a4a24)), 0.005);   // gancho na ponta de trás
    hook.rotation.y = Math.PI / 2; hook.position.set(0, -D(16), D(-20)); u.add(hook);
    const fist = withOutline(new THREE.Mesh(new THREE.SphereGeometry(D(20), 16, 12), flat(0xa85c38)), 0.007); u.add(fist);   // punho segura o cabo atrás da copa fechada
    u.userData = { canopy, spin, atk: -9, open: 0 };
    return u;
  }
  P.umbL = umbrella(); P.umbR = umbrella();
  const head = new THREE.Group(); head.add(P.hair, P.face);
  P.head = head;
  const WAIST = Y(290), upper = new THREE.Group(); upper.position.y = WAIST; root.add(upper);   // tronco + cabeça: pivô na cintura
  for (const k of ['torso', 'top', 'collar', 'chev', 'belt']) { P[k].position.y = -WAIST; upper.add(P[k]); }
  const NECK = Y(215), neck = new THREE.Group(); neck.position.y = NECK - WAIST; upper.add(neck);   // cabeça: pivô no pescoço
  head.position.y = HEAD_UP - NECK; neck.add(head);
  const CINCH = Y(286), skirt = new THREE.Group(); skirt.position.y = CINCH; root.add(skirt);      // saia: pivô no cós
  for (const k of ['skirt1', 'skirt2', 'skirt3']) { P[k].position.y = -CINCH; skirt.add(P[k]); }
  { // boca da saia fechada: anel laranja do cós até o pano (de cima não aparece o vão entre a cintura e a saia)
    const cap = new THREE.Mesh(new THREE.RingGeometry(D(44), D(67), 48), flat(0xf04a0c)); cap.material.side = THREE.DoubleSide;
    cap.rotation.x = -Math.PI / 2; cap.scale.y = 0.74; cap.position.set(0, Y(287) - CINCH, D(-4)); skirt.add(cap);
    P.under.position.y = Y(372) - CINCH; skirt.add(P.under);   // fundo escuro dentro da saia (por baixo não se vê através)
  }
  for (const k of ['legL', 'legR', 'bootL', 'bootR', 'umbL', 'umbR']) root.add(P[k]);
  document.querySelectorAll('[data-atk]').forEach(b => b.onclick = () => { P[b.dataset.atk].userData.atk = clock.elapsedTime; });
  const LEG0 = Y(426), LEG1 = Y(330);
  const placeFoot = legPlacer({ legs: { [-1]: P.legL, [1]: P.legR }, feet: { [-1]: P.bootL, [1]: P.bootR }, hipX: { [-1]: X(165), [1]: X(315) }, top: LEG1, bot: LEG0, hips: skirt, hipsY: CINCH });
  // rosto do Magma Frio (sprite ColdMagma): pele carvão, olhos retangulares em brasa, faixa de fogo na testa, boca reta
  let coldOn = false, faceNow = 'normal';
  const drawCold = f => (g, at) => { g.fillStyle = '#4b3832'; g.fillRect(0, 0, 1024, 1024);
    for (const [x, sd] of [[EYE_L[0], -1], [EYE_R[0], 1]]) at(x, EYE_L[1], 30, 22, gg => { if (f === 'ko') { gg.strokeStyle = '#f97316'; gg.lineWidth = 16; gg.lineCap = 'round'; gg.beginPath(); gg.moveTo(30, 20); gg.lineTo(98, 70); gg.moveTo(98, 20); gg.lineTo(30, 70); gg.stroke(); return; } gg.fillStyle = '#f97316'; gg.beginPath(); gg.roundRect(8, f === 'pain' ? 34 : 8, 112, f === 'pain' ? 24 : 74, 12); gg.fill(); gg.fillStyle = '#ffedd5'; gg.beginPath(); gg.roundRect(34, f === 'pain' ? 40 : 26, 60, f === 'pain' ? 10 : 34, 8); gg.fill(); if (f === 'angry') { gg.fillStyle = '#4b3832'; gg.beginPath(); gg.moveTo(0, -4); gg.lineTo(128, -4); sd < 0 ? (gg.lineTo(128, 42), gg.lineTo(0, 0)) : (gg.lineTo(128, 0), gg.lineTo(0, 42)); gg.fill(); } }, 128, 90);
    at(240, 136, 84, 24, gg => { const gr = gg.createLinearGradient(0, 80, 0, 0); gr.addColorStop(0, '#7c2d12'); gr.addColorStop(0.45, '#ea580c'); gr.addColorStop(0.8, '#f97316'); gr.addColorStop(1, '#ffedd5'); gg.fillStyle = gr; gg.strokeStyle = '#09090b'; gg.lineWidth = 7; gg.beginPath(); gg.moveTo(6, 22); gg.bezierCurveTo(60, -6, 196, -6, 250, 22); gg.bezierCurveTo(230, 72, 160, 34, 128, 74); gg.bezierCurveTo(96, 34, 26, 72, 6, 22); gg.fill(); gg.stroke(); }, 256, 80);
    at(240, NOSE_Y, 22, 22, valNose, 128, 128);
    at(240, MOUTH_Y, 26, 8, gg => { gg.fillStyle = '#09090b'; gg.fillRect(0, 0, 128, 40); }, 128, 40); };
  const setFace = f => { faceNow = f; faceMat.map.dispose(); faceMat.map = B.faceTexture(coldOn ? drawCold(f) : drawFace(f)); faceMat.needsUpdate = true; };
  // guarda-chuva: "Golpe" abre (0,12 s), avança e gira, fecha (até 0,8 s); no soco carregado abre e gira na batida
  const updateHand = (u, sx, hp, c) => {
    const d = u.userData, ka = c.t - d.atk;
    const [fw, , , op, spn] = hp;
    const oA = ka < 0 || ka > 0.8 ? 0 : ka < 0.12 ? ss(0, 0.12, ka) : ka < 0.55 ? 1 : 1 - ss(0.55, 0.8, ka);
    const o = Math.max(oA, op);
    if (Math.abs(o - d.open) > 1e-4) { d.open = o; d.canopy.userData.setOpen(o); }
    const thr = 0.28 * (ka < 0 || ka > 0.8 ? 0 : ka < 0.15 ? ss(0, 0.15, ka) : 1 - ss(0.4, 0.8, ka)) + fw;
    d.spin.rotation.z += (0.25 * Math.max(oA, op) + spn) * sx;
    c.placeHand(u, sx, hp, c, thr);
  };
  // cabelo: mola presa na cabeça (fica para trás quando ela mexe e volta balançando); lava acende ao carregar
  const hairPrev = new THREE.Vector3(), hairVel = new THREE.Vector3(), lag = new THREE.Vector3(), lagV = new THREE.Vector3();
  const hp = new THREE.Vector3(), hv = new THREE.Vector3(), ha = new THREE.Vector3(), hq = new THREE.Quaternion();
  let hairInit = false;
  // ================= especiais (os do jogo, skills.cpp e skill_poses.cpp, com os efeitos do laboratório, vfx/lab_valeria.cpp)
  const easeO = k => 1 - (1 - k) ** 3, win = (a, b, x) => Math.max(0, Math.min(1, (x - a) / (b - a))), DIST = 1.75;
  const fxRoot = new THREE.Group(); root.add(fxRoot);
  const glow = (col, o = 1, add = false) => new THREE.MeshBasicMaterial({ color: col, transparent: true, opacity: o, depthWrite: false, side: THREE.DoubleSide, blending: add ? THREE.AdditiveBlending : THREE.NormalBlending });
  const queue = []; let now = 0; const after = (dt, fn) => queue.push({ at: now + dt, fn });
  const FX = { sign: -99, magma: -99, erupt: -99, guard: -99, kick: -99, ash: -99, phoenix: -99, pillar: -99, spike: -99, heat: 0, kickLeg: 0 };
  const YEL = '#facc15', ORA = '#ea580c', RED = '#dc2626', HOT = '#ff4500', ASH = '#3f3f46', ASHL = '#71717a', WHITE = '#fff7ed';
  // bolinhas: brasas, gotas de lava, fumaça e cinza (fumaça cresce e some), com gravidade e vida
  const dotG = new THREE.SphereGeometry(1, 10, 8), dots = Array.from({ length: 240 }, () => { const m = new THREE.Mesh(dotG, glow('#fff')); m.visible = false; fxRoot.add(m); return { m, life: 0, v: new THREE.Vector3() }; });
  let dotI = 0; const dot = (p, v, rr, col, life, g = 3, op = 1, grow = 0) => { const q = dots[dotI++ % dots.length]; q.m.position.copy(p); q.m.scale.setScalar(rr); q.v.copy(v); q.r = rr; q.m.material.color.set(col); q.m.material.opacity = op; q.op = op; q.life = q.life0 = life; q.g = g; q.grow = grow; q.m.visible = true; };
  const burst = (p, n, spd, rr, cols, life, g = 3) => { for (let i = 0; i < n; i++) { const a = Math.random() * 6.283, sp = spd * (0.5 + Math.random()); dot(p, new THREE.Vector3(Math.cos(a) * sp, Math.sin(a) * sp * 0.8 + 0.4, (Math.random() - 0.5) * spd), rr * (0.6 + Math.random() * 0.7), cols[i % cols.length], life * (0.7 + Math.random() * 0.6), g); } };
  const ember = (p, fast) => dot(p, new THREE.Vector3((Math.random() - 0.5) * (fast ? 2.4 : 0.7), 0.5 + Math.random() * (fast ? 2 : 0.9), (Math.random() - 0.5) * (fast ? 2.4 : 0.7)), 0.009 + Math.random() * 0.01, Math.random() < 0.5 ? YEL : HOT, 0.7, 0);
  const smoke = (p, big, ash) => dot(p, new THREE.Vector3((Math.random() - 0.5) * 0.3, 0.3 + Math.random() * 0.4, (Math.random() - 0.5) * 0.3), big ? 0.09 : 0.045, ash ? (Math.random() < 0.5 ? ASH : ASHL) : (Math.random() < 0.5 ? '#52525b' : '#27272a'), big ? 1.1 : 0.7, 0, 0.75, big ? 0.3 : 0.14);
  const ringG = new THREE.RingGeometry(0.84, 1, 40), rings = Array.from({ length: 16 }, () => { const m = new THREE.Mesh(ringG, glow('#fff')); m.visible = false; fxRoot.add(m); return { m, life: 0 }; });
  let ringI = 0; const ring = (p, R, col, life, flatR = false, delay = 0) => after(delay, () => { const q = rings[ringI++ % rings.length]; q.m.position.copy(p); q.R = R; q.life = q.life0 = life; q.m.material.color.set(col); q.flat = flatR; q.m.rotation.set(flatR ? -Math.PI / 2 : 0, 0, 0); q.m.visible = true; });
  // marcas de queimado no chão
  const scorches = Array.from({ length: 14 }, () => { const m = new THREE.Mesh(new THREE.CircleGeometry(1, 24), glow('#1c1917', 0.8)); m.rotation.x = -Math.PI / 2; m.visible = false; fxRoot.add(m); return { m, life: 0 }; });
  let scI = 0; const scorch = (x, z, rr, life = 3) => { const q = scorches[scI++ % scorches.length]; q.m.position.set(x, 0.012 + scI * 0.0003, z); q.m.scale.set(rr, rr, 1); q.life = q.life0 = life; q.m.visible = true; };
  // "FREVO!" da assinatura (o texto do laboratório)
  const frevo = (() => { const c = document.createElement('canvas'); c.width = 512; c.height = 180; const g = c.getContext('2d'); g.font = '700 130px Bangers, Impact, sans-serif'; g.textAlign = 'center'; g.textBaseline = 'middle'; g.lineJoin = 'round'; g.lineWidth = 24; g.strokeStyle = '#0f1c10'; g.strokeText('FREVO!', 256, 95); g.fillStyle = '#2dd4bf'; g.fillText('FREVO!', 256, 95);
    const t = new THREE.CanvasTexture(c); t.colorSpace = THREE.SRGBColorSpace; const m = new THREE.Mesh(new THREE.PlaneGeometry(0.9, 0.32), new THREE.MeshBasicMaterial({ map: t, transparent: true, depthWrite: false })); m.visible = false; root.add(m); return m; })();
  // pilar vulcânico: coluna de lava (casca laranja, miolo amarelo) que sobe embaixo do rival, com rochas na base
  const pillar = new THREE.Group(); pillar.position.set(0, 0, DIST); pillar.visible = false; fxRoot.add(pillar);
  { const prof = (R, H) => { const pts = []; for (let i = 0; i <= 14; i++) { const u = i / 14; pts.push(new THREE.Vector2(R * (1 - 0.35 * u) * (1 + 0.1 * Math.sin(u * 14)) + 0.001, u * H)); } pts.push(new THREE.Vector2(0.001, H)); return new THREE.LatheGeometry(pts, 20); };
    const lt = (() => { const c = document.createElement('canvas'); c.width = c.height = 256; const g = c.getContext('2d'); g.fillStyle = ORA; g.fillRect(0, 0, 256, 256); for (let i = 0; i < 70; i++) { const x = (i * 97) % 256, y = (i * 53) % 256, rr = 6 + (i * 7) % 26; g.fillStyle = i % 3 === 0 ? YEL : i % 3 === 1 ? RED : '#fb923c'; for (const dx of [-256, 0, 256]) for (const dy of [-256, 0, 256]) { g.beginPath(); g.ellipse(x + dx, y + dy, rr, rr * 1.9, 0, 0, 7); g.fill(); } } const t = new THREE.CanvasTexture(c); t.colorSpace = THREE.SRGBColorSpace; t.wrapS = t.wrapT = THREE.RepeatWrapping; t.repeat.set(2, 2); return t; })(); pillar.userData.tex = lt;
    pillar.add(withOutline(new THREE.Mesh(prof(0.34, 1), new THREE.MeshBasicMaterial({ map: lt })), 0.008)); const core = new THREE.Mesh(prof(0.2, 1.03), new THREE.MeshBasicMaterial({ color: YEL })); pillar.add(core); pillar.userData.core = core;
    const cap = new THREE.Mesh(new THREE.SphereGeometry(0.2, 14, 10), new THREE.MeshBasicMaterial({ color: '#fff3b0' })); cap.scale.y = 0.05; cap.position.y = 1.03; pillar.add(cap); }
  const pillarRocks = Array.from({ length: 7 }, (_, i) => { const a = i / 7 * 6.283, m = new THREE.Mesh(new THREE.ConeGeometry(0.1, 0.34 + (i % 3) * 0.08, 5), flat('#27272a')); m.material.flatShading = true; const g = withOutline(m, 0.005); g.position.set(Math.cos(a) * 0.4, 0.14, DIST + Math.sin(a) * 0.4); g.rotation.set(Math.sin(a) * 0.5, 0, -Math.cos(a) * 0.5); g.visible = false; fxRoot.add(g); return g; });
  // pico de obsidiana da estocada: cristais escuros com veio de lava
  const spike = new THREE.Group(); spike.position.set(0, 0, DIST - 0.1); spike.visible = false; fxRoot.add(spike);
  [[0, 0, 0.17, 1.15, 0], [-0.17, 0.03, 0.11, 0.72, 0.3], [0.16, -0.02, 0.1, 0.62, -0.3], [0.04, 0.17, 0.09, 0.55, -0.25], [-0.05, -0.17, 0.08, 0.5, 0.3]].forEach(([x, z, R, H, tilt], i) => { const g = new THREE.ConeGeometry(R, H, 5); g.translate(0, H / 2, 0); const m = flat(i % 2 ? '#3f3f46' : '#27272a'); m.flatShading = true; const o = withOutline(new THREE.Mesh(g, m), 0.006); o.position.set(x, 0, z); o.rotation.z = tilt; spike.add(o);
    const v = new THREE.Mesh(new THREE.ConeGeometry(R * 0.35, H * 0.72, 4), new THREE.MeshBasicMaterial({ color: HOT })); v.position.set(x + R * 0.28, H * 0.34, z + R * 0.62); v.rotation.z = tilt; spike.add(v); });
  // asas de fênix: seis penas de fogo em leque de cada lado, atrás dela
  const featherTex = (() => { const c = document.createElement('canvas'); c.width = 64; c.height = 256; const g = c.getContext('2d'), gr = g.createLinearGradient(0, 256, 0, 0); gr.addColorStop(0, '#fff3b0'); gr.addColorStop(0.35, YEL); gr.addColorStop(0.7, HOT); gr.addColorStop(1, 'rgba(220,38,38,0.1)'); g.fillStyle = gr; g.beginPath(); g.moveTo(32, 256); g.bezierCurveTo(0, 170, 6, 60, 32, 0); g.bezierCurveTo(58, 60, 64, 170, 32, 256); g.fill(); const t = new THREE.CanvasTexture(c); t.colorSpace = THREE.SRGBColorSpace; return t; })();
  const featherG = new THREE.PlaneGeometry(0.2, 0.72); featherG.translate(0, 0.36, 0);
  const wings = [-1, 1].map(sd => { const g = new THREE.Group(); g.position.set(sd * 0.1, Y(225) - WAIST, -0.3); g.visible = false; upper.add(g); const fs = []; for (let i = 0; i < 6; i++) { const h = new THREE.Group(); h.add(new THREE.Mesh(featherG, new THREE.MeshBasicMaterial({ map: featherTex, transparent: true, depthWrite: false, side: THREE.DoubleSide, blending: THREE.AdditiveBlending }))); h.rotation.z = -sd * (0.25 + i * 0.3); h.scale.setScalar(1.25 - i * 0.09); g.add(h); fs.push(h); } return { g, fs, sd }; });
  // aura do escudo de calor
  const aura = new THREE.Mesh(new THREE.SphereGeometry(0.78, 28, 20), glow('#fb923c', 0.14, true)); aura.position.y = 0.9; aura.scale.y = 1.25; aura.visible = false; fxRoot.add(aura);
  // Magma Frio (sprite ColdMagma), visual próprio: pele carvão, cabelo de obsidiana lisa com o domo de lava no meio e
  // dois veios, olhos em brasa, top e babados em marrom, laranja e azul-petróleo, sapatos azul-petróleo
  const coldU = { value: 0 };
  const COLD = { a4502c: '#4b3832', a85c38: '#4b3832', '8f4a2c': '#2c1e19', f04a0c: '#7c2d12', f2c21a: '#ea580c', '2fd0c0': '#1e293b', '1c8a80': '#0f172a', '3a1a12': '#18181b' };
  const coldMats = [];
  for (const grp of [P.torso, P.top, P.belt, P.skirt1, P.skirt2, P.skirt3, P.under, P.legL, P.legR, P.bootL, P.bootR]) grp.traverse(o => { if (!o.isMesh || !o.material.color || o.userData.hull || coldMats.some(q => q.m === o.material)) return; const hx = o.material.color.getHexString(), to = o.material.vertexColors ? null : COLD[hx]; if (o.material.vertexColors || to) coldMats.push({ m: o.material, c: o.material.color.clone(), to: to ? new THREE.Color(to) : new THREE.Color(0.42, 0.27, 0.22) }); });
  const wrapCold = (m, glsl, tag) => { const prev = m.onBeforeCompile, key = m.customProgramCacheKey(); m.onBeforeCompile = (sh, rd) => { prev.call(m, sh, rd); sh.uniforms.uCold = coldU; sh.fragmentShader = 'uniform float uCold;' + sh.fragmentShader.replace('#include <dithering_fragment>', '#include <dithering_fragment>\n' + glsl); }; m.customProgramCacheKey = () => key + '|' + tag; m.needsUpdate = true; };
  P.hair.traverse(o => { if (o.isMesh && o.material.isMeshToonMaterial) wrapCold(o.material, `{ vec3 q = (vLavaP - uLMin) / (uLMax - uLMin);
      // como o sprite: massa preta lisa (cinza só no alto), domo laranja em degradê no meio e dois veios finos (cores em sRGB)
      vec3 ob = mix(vec3(0.035, 0.035, 0.043), vec3(0.20, 0.20, 0.22), smoothstep(0.35, 1.0, q.y));
      float dr = length(vec2((q.x - 0.5) / 0.34, (q.y - 0.56) / 0.2));
      float dome = (1.0 - smoothstep(0.96, 1.0, dr)) * smoothstep(0.42, 0.5, q.z);
      float h = clamp((q.y - 0.38) / 0.36, 0.0, 1.0);
      vec3 lv = h < 0.45 ? mix(vec3(0.486, 0.176, 0.071), vec3(0.918, 0.345, 0.047), h / 0.45) : h < 0.8 ? mix(vec3(0.918, 0.345, 0.047), vec3(0.976, 0.451, 0.086), (h - 0.45) / 0.35) : mix(vec3(0.976, 0.451, 0.086), vec3(1.0, 0.929, 0.835), (h - 0.8) / 0.2);
      float v1 = 1.0 - smoothstep(0.004, 0.009, abs(q.x - 0.26 - 0.03 * sin(q.y * 26.0))), v2 = 1.0 - smoothstep(0.004, 0.009, abs(q.x - 0.76 + 0.03 * sin(q.y * 22.0)));
      vec3 cc = mix(ob, vec3(0.976, 0.451, 0.086), max(v1, v2) * step(0.3, q.y) * step(q.y, 0.72) * step(0.3, q.z));
      gl_FragColor.rgb = mix(gl_FragColor.rgb, mix(cc, lv, dome), uCold); }`, 'coldHair'); });
  for (const u of [P.umbL, P.umbR]) u.traverse(o => { if (o.isMesh && o.material.isMeshToonMaterial && !o.material.userData.coldW) { o.material.userData.coldW = true; wrapCold(o.material, 'gl_FragColor.rgb = mix(gl_FragColor.rgb, gl_FragColor.rgb * vec3(0.42, 0.26, 0.22), uCold);', 'coldUmb'); } });
  let faceSh = null; { const prev = faceMat.onBeforeCompile; faceMat.onBeforeCompile = (sh, rd) => { prev.call(faceMat, sh, rd); faceSh = sh; }; faceMat.needsUpdate = true; }
  const faceBase = { c: new THREE.Color(VALERIA.face), n: new THREE.Color(VALERIA.neck) };
  // um risquinho de magma em cada canela (como no sprite) e o brilho dos olhos
  const veinM = new THREE.MeshBasicMaterial({ color: '#f97316' }), coldBits = [];
  const vein = (pts, parent, rr = 1.6) => { const m = new THREE.Mesh(new THREE.TubeGeometry(new THREE.CatmullRomCurve3(pts), 16, D(rr), 5), veinM); m.visible = false; parent.add(m); coldBits.push(m); };
  for (const sd of [-1, 1]) for (const a of [0.45]) { const L = LEG1 - LEG0; vein([0.42, 0.55, 0.68].map((u, i) => new THREE.Vector3(D(11 * Math.sin(a)) + D(i % 2 ? 2.5 : -2.5), -u * L, D(12.3 * Math.cos(a)))), (sd < 0 ? P.legL : P.legR)); }
  for (const x of [EYE_L[0], EYE_R[0]]) { const m = new THREE.Mesh(new THREE.SphereGeometry(D(11), 12, 8), glow('#f97316', 0.22, true)); m.scale.set(1.3, 0.8, 0.3); m.position.set(X(x), Y(EYE_L[1]), D(66)); m.visible = false; P.face.add(m); coldBits.push(m); }
  let coldTarget = 0; const setCold = on => { coldOn = on; coldTarget = on ? 1 : 0; coldU.value = coldTarget; for (const q of coldMats) q.m.color.copy(on ? q.to : q.c); for (const b of coldBits) b.visible = on; if (faceSh) { faceSh.uniforms.uFaceC.value.copy(on ? new THREE.Color('#4b3832') : faceBase.c); faceSh.uniforms.uNeckC.value.copy(on ? new THREE.Color('#2c1e19') : faceBase.n); } setFace(faceNow); const p = new THREE.Vector3(0, 0.95, 0); ring(p, 1.05, on ? '#f97316' : '#fde047', 0.5); for (let i = 0; i < 10; i++) smoke(new THREE.Vector3((Math.random() - 0.5) * 0.7, 0.3 + Math.random() * 1.1, 0.1), true); burst(p, 18, 1.8, 0.016, ['#f97316', '#fdba74'], 0.7, 1); };
  const coldB = document.querySelector('#cold'); if (coldB) coldB.onclick = () => { setCold(!coldOn); coldB.style.outline = coldOn ? '2px solid #fc6' : ''; };
  const bothP = v => ({ [-1]: v, [1]: v });
  const setPose = (pose, v) => { pose[-1] = v[-1]; pose[1] = v[1]; };
  const bumpU = u => Math.sin(Math.min(1, Math.max(0, u)) * Math.PI), fractU = u => u - Math.floor(u);
  // o estado da dança no ponto p (0..1): pés { [lado]: [dx, sobe, dz] }, corpo e mãos
  const dance = p => { const out = { feet: { [-1]: [0, 0, 0], [1]: [0, 0, 0] }, rootY: 0, spin: 0, flare: 0, word: false, up: 0, midR: 0, midL: 0, drop: 0, lean: 0, tilt: 0, twist: 0 };
    const crouch = bumpU(win(0, 0.08, p)), hop = bumpU(win(0.08, 0.2, p)), land = bumpU(win(0.2, 0.25, p)), tes = win(0.27, 0.55, p), sp = win(0.57, 0.86, p), end = ss(0.87, 0.93, p) * (1 - ss(0.96, 1, p));
    out.up = Math.max(crouch, hop, ss(0.24, 0.3, p), end); out.up *= p > 0.96 ? 1 - ss(0.96, 1, p) : 1; out.drop = 0.06 * (crouch + land);
    out.rootY = 0.19 * hop; out.word = hop > 0.6; out.flare = 0.16 * hop;
    for (const sd of [-1, 1]) { out.feet[sd][0] += sd * 0.15 * hop; out.feet[sd][1] += 0.16 * hop; }
    out.lean -= 0.06 * hop;
    if (tes > 0 && tes < 1) { const b = tes * 4, sd = Math.floor(b) % 2 ? 1 : -1, kick = bumpU(fractU(b)), f = out.feet[sd], o = out.feet[-sd], env = ss(0, 0.06, tes) * (1 - ss(0.94, 1, tes));
      f[0] -= sd * 0.09 * kick; f[1] += 0.2 * kick; f[2] += 0.3 * kick; o[1] += 0.03 * kick; o[0] += sd * 0.02;
      out.rootY += 0.05 * kick; out.drop += 0.04 * (1 - kick) * env; out.tilt = sd * 0.1 * kick; out.twist = -sd * 0.14 * kick; out.lean -= 0.1 * kick; out.flare += 0.14 * kick;
      out.midR = (0.25 + sd * 0.4) * env; out.midL = (0.25 - sd * 0.4) * env; }
    if (sp > 0 && sp < 1) { const inOut = sp * sp * (3 - 2 * sp), env = ss(0, 0.1, sp) * (1 - ss(0.9, 1, sp)), st = sp * 8, sd = Math.floor(st) % 2 ? 1 : -1, l = bumpU(fractU(st)); out.spin = inOut * Math.PI * 4; out.feet[sd][1] += 0.11 * l * env; out.feet[sd][2] += 0.07 * l * env; out.feet[sd][0] -= sd * 0.05 * l * env; out.flare += 0.42 * bumpU(sp); out.midR = 0.2 * env; out.midL = 0.3 * env; out.rootY += 0.025 * l * env; }
    if (end > 0) { const f = out.feet[-1]; f[1] += 0.17 * end; f[2] -= 0.12 * end; f[0] += 0.05 * end; out.tilt += 0.09 * end; out.lean -= 0.08 * end; out.midR -= 0.1 * end; out.flare += 0.08 * end; }
    return out; };
  // põe o pé também para a frente/trás (o legPlacer só faz x e altura): refaz a perna até o pé deslocado
  const DOWN = new THREE.Vector3(0, -1, 0), legOf = { [-1]: P.legL, [1]: P.legR }, bootOf = { [-1]: P.bootL, [1]: P.bootR }, BOOTZ = { [-1]: P.bootL.position.z, [1]: P.bootR.position.z }, HIPX = { [-1]: X(165), [1]: X(315) };
  const place3 = (sd, dx, lift, dz) => { placeFoot(sd, dx, lift); if (!dz) return; const leg = legOf[sd], tgt = new THREE.Vector3(HIPX[sd] + dx, LEG0 + lift, dz), dir = tgt.sub(leg.position), len = dir.length(); leg.quaternion.setFromUnitVectors(DOWN, dir.divideScalar(len)); leg.scale.y = len / (LEG1 - LEG0); bootOf[sd].position.z = BOOTZ[sd] + dz; };
  const skills = {
    // assinatura: coreografia de frevo, sorrindo. Agacha, salta abrindo as pernas ("FREVO!"), tesoura de quatro chutes
    // alternados com a sombrinha indo de um lado ao outro, duas voltas em passinhos e a pose final de perna erguida
    signature: { dur: 4.0, start: () => { FX.sign = now; },
      pose: (k, pose) => { const p = k / 4.0, d = dance(p); pose[1] = [0.04 * d.up, 0.42 * d.up, d.midR, 1, 0.45]; pose[-1] = [0.02 * d.up, 0.1 * d.up, d.midL, 0, 0]; return { drop: d.drop, lean: d.lean, tilt: d.tilt, twist: d.twist }; } },
    // soco magmático: arma girando a sombrinha, solta o direto em 0,42 com os anéis, fumaça e lava
    magma: { dur: 1.1, start: () => { FX.magma = now; setFace('angry'); after(1.1, () => setFace('normal')); after(0.46, () => { const p = P.umbR.position.clone(); p.z += 0.2; ring(p, 1.1, ORA, 0.5); ring(p, 0.75, YEL, 0.42); for (let i = 0; i < 9; i++) smoke(p, true); burst(p, 22, 2.8, 0.018, [YEL, ORA, WHITE], 0.6); }); },
      pose: (k, pose) => { const p = k / 1.1, w = ss(0, 0.42, p), hit = win(0.42, 0.62, p), rec = win(0.62, 1, p), th = Math.sin(hit * Math.PI / 2) * (1 - rec), sp = p * Math.PI * 6 * (1 - hit); FX.heat = p < 0.42 ? p / 0.42 : p < 0.68 ? 1 : 1 - (p - 0.68) / 0.32;
        pose[1] = [(-0.24 * w) * (1 - th) + 0.78 * th, (-0.04 * w + Math.sin(sp) * 0.05 * w) * (1 - th) + 0.06 * th, 0.42 * th + Math.cos(sp) * 0.07 * w * (1 - th), Math.max(w * 0.6, th), 0.6 * w + 0.3 * th]; return { twist: 0.34 * w * (1 - th) - 0.42 * th, lean: 0.18 * th - 0.06 * w * (1 - th) }; } },
    // erupção: ergue as duas sombrinhas e crava no chão; o pilar de lava sobe embaixo do rival
    erupt: { dur: 1.3, start: () => { FX.erupt = now; setFace('angry'); after(1.3, () => setFace('normal')); [0.15, 0.32, 0.5].forEach(d => ring(new THREE.Vector3(0, 0.015, DIST), 0.8, ORA, 0.45, true, d)); after(0.15, () => { scorch(0, DIST, 0.5, 4); scorch(0.3, DIST + 0.2, 0.2, 4); scorch(-0.28, DIST - 0.15, 0.22, 4); });
      after(0.715, () => { FX.pillar = now; const b = new THREE.Vector3(0, 0.25, DIST); burst(b, 26, 3, 0.022, [YEL, ORA, WHITE], 0.8, 4); for (let i = 0; i < 16; i++) { const a = Math.random() * 6.283, sp = 0.8 + Math.random() * 1.2; dot(new THREE.Vector3(Math.cos(a) * 0.1, 0.5 + Math.random() * 1.2, DIST + Math.sin(a) * 0.1), new THREE.Vector3(Math.cos(a) * sp, 1.5 + Math.random() * 2, Math.sin(a) * sp), 0.04 + Math.random() * 0.03, Math.random() < 0.5 ? YEL : ORA, 1.5, 6); } for (let i = 0; i < 10; i++) smoke(new THREE.Vector3((Math.random() - 0.5) * 0.5, 0.2 + Math.random() * 0.6, DIST), true); }); },
      pose: (k, pose) => { const p = k / 1.3, ra = ss(0, 0.45, p), sl = ss(0.45, 0.55, p), rec = win(0.8, 1, p), y = (0.42 * ra - 0.92 * sl) * (1 - rec), v = [0.06 * sl * (1 - rec), y, 0.6 * (1 - rec) * ra, 0, 0]; setPose(pose, bothP(v)); return { lean: (-0.08 * ra + 0.3 * sl) * (1 - rec), drop: 0.13 * sl * (1 - rec) }; } },
    guard: { dur: 0.9, start: () => { FX.guard = now; }, pose: (k, pose) => { const e = ss(0, 0.3, k) * (1 - ss(0.6, 0.9, k)), v = [0.02 * e, 0.12 * e, -0.25 * e, e, 0.25 * e]; setPose(pose, bothP(v)); return { drop: 0.03 * e, tilt: 0.04 * Math.sin(k * 30) * e }; } },
    // estocada: inclina para trás e estoca com a perna quente; o pico de obsidiana rompe embaixo do rival
    kick: { dur: 0.9, start: () => { FX.kick = now; setFace('angry'); after(0.9, () => setFace('normal')); after(0.34, () => { FX.spike = now; const b = new THREE.Vector3(0, 0.25, DIST - 0.1); burst(b, 18, 2.4, 0.02, [YEL, RED], 0.6, 4); for (let i = 0; i < 14; i++) dot(new THREE.Vector3((Math.random() - 0.5) * 0.25, 0.1 + Math.random() * 0.5, DIST - 0.1), new THREE.Vector3((Math.random() - 0.5) * 2, 1 + Math.random() * 2, (Math.random() - 0.5) * 2), 0.03, Math.random() < 0.5 ? '#27272a' : '#52525b', 1, 6); ring(new THREE.Vector3(0, 0.015, DIST - 0.1), 0.6, ORA, 0.4, true); }); },
      pose: (k, pose) => { const q = Math.sin(win(0.15, 1, k / 0.9) * Math.PI); FX.kickLeg = q; pose[1] = [0.3 * q, -0.22 * q, 0.3 * q, 0, 0]; pose[-1] = [-0.02 * q, 0.08 * q, 0, 0, 0]; return { lean: -0.28 * q, drop: 0.04 * q }; } },
    // cinzas: varre o braço de um lado ao outro e a rajada vai cobrir os olhos do rival
    ash: { dur: 1.0, start: () => { FX.ash = now; setFace('angry'); after(1.0, () => setFace('normal')); after(0.42, () => { for (let i = 0; i < 14; i++) smoke(new THREE.Vector3((Math.random() - 0.5) * 0.7, 0.9 + Math.random() * 0.6, DIST + (Math.random() - 0.5) * 0.4), true, true); }); },
      pose: (k, pose) => { const u = win(0.1, 0.5, k), e = 1 - ss(0.6, 1, k); pose[1] = [0.28 * Math.sin(u * Math.PI) * e, 0.08 * e, (1.5 * u - 0.15) * e, 0.7 * e, 0.3 * e]; return { twist: (-0.3 + 0.6 * u) * e }; } },
    // reagrupamento: braços abertos e erguidos, as brasas do ringue espiralam para dentro, abrem as asas de fênix
    phoenix: { dur: 1.5, start: () => { FX.phoenix = now; after(0.9, () => { const c0 = new THREE.Vector3(0, 0.95, 0); burst(c0, 22, 2.4, 0.018, [YEL, HOT, WHITE], 0.7, 1); ring(new THREE.Vector3(0, 0.015, 0), 1.45, HOT, 0.8, true); }); },
      pose: (k, pose) => { const e = ss(0, 0.35, k) * (1 - ss(1.1, 1.5, k)), v = [-0.04 * e, 0.42 * e, -0.3 * e, 0.8 * e, 0.2 * e]; FX.heat = 0.8 * e; setPose(pose, bothP(v)); return { lean: -0.08 * e }; } },
  };
  const fxFrame = c => {
    now = c.t; const t = now, dt = c.dt, cam = window.VIN?.camera;
    for (let i = queue.length - 1; i >= 0; i--) if (now >= queue[i].at) { queue[i].fn(); queue.splice(i, 1); }
    for (const q of dots) { if (q.life <= 0) { q.m.visible = false; continue; } q.life -= dt; q.v.y -= q.g * dt; q.m.position.addScaledVector(q.v, dt); const u = q.life / q.life0; if (q.grow) { q.r += q.grow * dt; q.m.scale.setScalar(q.r); q.m.material.opacity = q.op * u; } else q.m.scale.setScalar(q.r * Math.max(0.05, u)); if (q.m.position.y < 0.01) { if (q.g > 4 && q.r > 0.03) scorch(q.m.position.x, q.m.position.z, 0.1 + Math.random() * 0.07, 1.6); q.life = 0; } }
    for (const q of rings) { if (q.life <= 0) { q.m.visible = false; continue; } q.life -= dt; const u = 1 - q.life / q.life0, e = q.R * easeO(u) + 0.03; q.m.scale.set(e, e, 1); q.m.material.opacity = Math.max(0, 1 - u); if (cam && !q.flat) q.m.quaternion.copy(cam.quaternion); }
    for (const q of scorches) { if (q.life <= 0) { q.m.visible = false; continue; } q.life -= dt; q.m.material.opacity = 0.8 * Math.min(1, q.life / (q.life0 * 0.5)); }
    if (c.skill !== 'magma' && c.skill !== 'phoenix') FX.heat = Math.max(0, FX.heat - dt * 2.4);
    // assinatura: dança (pés, salto, giro e saia)
    { const p = (t - FX.sign) / 4.0, on = p > 0 && p < 1, d = on ? dance(p) : null;
      root.rotation.y = on ? d.spin : 0; const fl = on ? d.flare : 0; skirt.scale.set(1 + fl, 1 - 0.1 * fl, 1 + fl);
      if (on) { root.position.y += d.rootY; for (const sd of [-1, 1]) place3(sd, ...d.feet[sd]); }
      frevo.visible = on && d.word; if (frevo.visible) { if (cam) frevo.quaternion.copy(cam.quaternion); frevo.position.set(0, 1.92, 0.2); frevo.rotation.y = cam ? frevo.rotation.y : 0; } }
    // soco magmático: faíscas, brasas e gotas enquanto arma
    { const p = (t - FX.magma) / 1.1; if (p > 0.08 && p < 1) { const g = P.umbR.position.clone(); g.z += 0.1; if (Math.random() < 0.85 * FX.heat) ember(g, true); if (Math.random() < 0.5 * FX.heat) dot(g, new THREE.Vector3((Math.random() - 0.5), 0.6, (Math.random() - 0.5)), 0.016, Math.random() < 0.5 ? YEL : ORA, 0.6, 4); if (Math.random() < 0.35) smoke(g, false); } }
    // erupção: o chão fumega antes; o pilar sobe, segura e baixa
    { const p = (t - FX.erupt) / 1.3; if (p > 0 && p < 0.55) { if (Math.random() < 0.5) smoke(new THREE.Vector3((Math.random() - 0.5) * 0.6, 0.04, DIST + (Math.random() - 0.5) * 0.5), false); if (Math.random() < 0.6) ember(new THREE.Vector3((Math.random() - 0.5) * 0.3, 0.05, DIST), false); }
      const f = (t - FX.pillar) * 60, on = f >= 0 && f < 60, H = on ? 2.3 * (f < 12 ? Math.sin(f / 12 * Math.PI / 2) : f > 34 ? Math.pow(0.86, f - 34) : 1) : 0; pillar.visible = on && H > 0.03; for (const q of pillarRocks) q.visible = on; if (pillar.visible) { pillar.userData.tex.offset.y = -t * 1.3; pillar.scale.set(1 + 0.06 * Math.sin(t * 30), Math.max(0.01, H), 1 + 0.06 * Math.cos(t * 27)); if (Math.random() < 0.7) { const a = Math.random() * 6.283; dot(new THREE.Vector3(Math.cos(a) * 0.18, H * (0.6 + Math.random() * 0.4), DIST + Math.sin(a) * 0.18), new THREE.Vector3(Math.cos(a) * 1.4, 1 + Math.random() * 1.2, Math.sin(a) * 1.4), 0.025 + Math.random() * 0.02, Math.random() < 0.5 ? YEL : ORA, 1, 5); } } }
    // estocada
    { const p = (t - FX.kick) / 0.9; if (c.skill === 'kick') { placeFoot(1, 0, 0.32 * FX.kickLeg); if (p < 0.5 && Math.random() < 0.7) ember(new THREE.Vector3(X(315), 0.25 + Math.random() * 0.3, 0.1), false); }
      const f = (t - FX.spike) * 60, on = f >= 0 && f < 54; spike.visible = on; if (on) { const gr = f < 6 ? f / 6 : f > 40 ? Math.max(0.01, 1 - (f - 40) / 14) : 1; spike.scale.set(0.6 + 0.4 * gr, Math.max(0.01, gr), 0.6 + 0.4 * gr); } }
    // escudo de calor (5 s)
    { const g = t - FX.guard, a = ss(0, 0.4, g) * (1 - ss(4.6, 5, g)); aura.visible = a > 0.01; if (aura.visible) { aura.material.opacity = 0.13 * a * (0.8 + 0.2 * Math.sin(t * 9)); aura.scale.set(1 + 0.03 * Math.sin(t * 7), 1.25, 1 + 0.03 * Math.sin(t * 7)); if (Math.random() < 0.5 * a) ember(new THREE.Vector3((Math.random() - 0.5) * 0.9, Math.random() * 1.3, (Math.random() - 0.5) * 0.6), false); if (Math.random() < 0.05 * a) ring(new THREE.Vector3(0, 0.015, 0), 1.15, HOT, 0.6, true); if (Math.random() < 0.04 * a) ring(new THREE.Vector3(0, 1, 0), 0.95, '#fb923c', 0.4); } }
    // cinzas: rajada da mão até o rival
    { const p = (t - FX.ash) / 1.0; if (p > 0 && p < 0.45 && Math.random() < 0.85) { const g = P.umbR.position.clone(); dot(g, new THREE.Vector3((Math.random() - 0.5) * 0.3, (Math.random() - 0.3) * 0.3, 2.6 + Math.random() * 0.9), 0.06, Math.random() < 0.5 ? ASH : ASHL, 0.75, 0, 0.8, 0.26); if (Math.random() < 0.5) ember(g.clone().add(new THREE.Vector3(0, 0, Math.random() * 1)), false); } }
    // asas de fênix e brasas subindo em espiral para dentro
    { const p = (t - FX.phoenix) / 1.5, target = p > 0 && p < 0.85 ? 1 : 0; for (const w of wings) { w.a = (w.a || 0) + (target - (w.a || 0)) * Math.min(1, dt * 7); w.g.visible = w.a > 0.02; if (w.g.visible) { w.g.scale.setScalar(w.a); const fl = Math.sin(t * 7) * 0.12; w.fs.forEach((h, i) => { h.rotation.z = -w.sd * (0.25 + i * 0.3 + fl * (1 + i * 0.2)); }); } }
      if (p > 0 && p < 0.75) for (let i = 0; i < 2; i++) { const a = Math.random() * 6.283, rad = 0.8 + Math.random() * 0.6, y0 = 0.05 + Math.random() * 0.4; dot(new THREE.Vector3(Math.cos(a) * rad, y0, Math.sin(a) * rad), new THREE.Vector3(-Math.cos(a) * rad / 0.47, (0.95 - y0) / 0.47, -Math.sin(a) * rad / 0.47), 0.012 + Math.random() * 0.012, Math.random() < 0.5 ? YEL : HOT, 0.47, 0); } }
    // magma frio: fumaça fina saindo do cabelo
    if (coldOn && Math.random() < 0.06) smoke(new THREE.Vector3((Math.random() - 0.5) * 0.4, 1.5, 0), false);
  };
  const onFrame = c => { const { t, dt } = c, heat = Math.max(c.heat, FX.heat);
    lavaU.heat.value += (heat - lavaU.heat.value) * Math.min(1, dt * 10);
    head.updateWorldMatrix(true, false); head.getWorldPosition(hp);
    if (!hairInit) { hairPrev.copy(hp); hairInit = true; }
    hv.copy(hp).sub(hairPrev).divideScalar(Math.max(dt, 1e-3)); ha.copy(hv).sub(hairVel).divideScalar(Math.max(dt, 1e-3));
    hairVel.copy(hv); hairPrev.copy(hp);
    lagV.addScaledVector(lag, -140 * dt).addScaledVector(lagV, -9 * dt).addScaledVector(ha, -0.9 * dt);
    lag.addScaledVector(lagV, dt); if (lag.length() > 0.07) lag.setLength(0.07);
    head.getWorldQuaternion(hq).invert();
    lavaU.lag.value.copy(lag).applyQuaternion(hq).divideScalar(P.hairScale);
    lavaU.time.value = t;
    fxFrame(c);
  };
  window.inkPush = inkPush;
  P.cloth = [{ roots: [P.skirt1, P.skirt2, P.skirt3, P.under], top: Y(290), bot: Y(378), pow: 1.3, gain: 0.75 }];   // tools3d/export
  return { P, upper, WAIST, neck, NECK, hips: skirt, HIPS: CINCH, head, hands: { [-1]: P.umbL, [1]: P.umbR }, hand: { x: 0.738, y: 0.92, z: 0.465, yaw: -0.35 }, placeFoot, setFace, updateHand, skills, onFrame };
  },
};
