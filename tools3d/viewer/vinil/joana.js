// Joana Cara-de-Craca no modo vinil, no corpo da Valéria (base.js) e pela arte nova dela: chapéu de três pontas
// vermelho com friso dourado, dreads castanhos com anéis dourados (cada um com mola), tapa-olho; a metade de siri pintada
// na pele com a divisão em zigue-zague (nariz de pele por cima, boca de siri com dentes, olho de siri na haste com
// pálpebras), descendo pelo pescoço até o top; top faixa listrado com busto, barriga de fora, colete curto escuro,
// short listrado com retalhos e cinto, bota de cano dobrado de um lado e perna de pau do outro. Sem braços (como no
// jogo): garra de siri com pontas de metal e gancho com corrente (com física). Medidas em px do quadro; S px = 1 m.
import { THREE, ss, rrect, INKS, flat, withOutline, legPlacer } from './core.js';
import { RoundedBoxGeometry } from 'three/addons/geometries/RoundedBoxGeometry.js';
import { TessellateModifier } from 'three/addons/modifiers/TessellateModifier.js';
import { VALERIA, bodyOpts, valeriaBody } from './base.js';

const S_PX = 185, FOOT = 335, CX = 180;
const C = { skin: '#c47c5a', band: '#8a1f2b', gold: '#f0b636', shell: '#e35f0a', shellDark: '#a83c08', patch: '#1b1e22', cream: '#ede6cc',
            coat: '#262b36', coatLight: '#3a4150', skirt: '#d8cfa4', stripe: '#8c2a2e', waist: '#3b2418', wood: '#6e3d22', woodDark: '#4a2614',
            shoe: '#3a2a1e', sole: '#141812', dread: '#2b1b15', dread2: '#3f281b', silver: '#cdd4d2', cuff: '#3e4446', mouthIn: '#4a1414' };
const sp = (v, pw) => Math.sign(v) * Math.pow(Math.abs(v), 2 / pw);
const col = h => ({ value: new THREE.Color(h) });

export default {
  title: 'Joana Cara-de-Craca',
  arena: 'joana',   // abre na arena própria (core.js)
  units: [S_PX, FOOT, CX],
  faces: [['normal', 'Normal'], ['angry', 'Raiva'], ['pain', 'Dor'], ['ko', 'K.O.']],
  skills: [['signature', 'Assinatura'], ['crush', 'Tesoura Esmagadora'], ['hook', 'Gancho'], ['shell', 'Carapaça de Cracas'], ['coral', 'Coral'], ['anchor', 'Âncora'], ['bounty', 'Anêmona']],
  hudExtra: 'Forma: <button id="calcified">Carapaça Calcária</button><button id="deep">Maré das Profundezas</button>',
  others: [['valeria', 'Valéria'], ['frog', 'King Frog']],
  async build({ root, U }) {
    const { X, Y, D, loft } = U, P = {};
    // corpo: o da Valéria (base.js) nas medidas dela: cabeça de ovo cheia (o chapéu e as dreads cobrem o alto e a nuca),
    // pescoço que abre nos ombros e no peito até debaixo do top, barriga, perna de carne com o sapato da Valéria em marrom
    const B = valeriaBody(U, bodyOpts(VALERIA, { cx: CX, skin: C.skin, face: C.skin, neck: '#a8644a',
      head: { top: 78, bot: 134, a: 56, d: 0.86, cz: 0 }, yoke: { y0: 124, y1: 152, a0: 22, a1: 42.5, d: 0.74, cz: -2, end: 158 },
      torso: { y0: 140, y1: 232, a: 41, d: 0.74, cz: -2 }, legs: { dx: 37, top: 268, bot: 306, r: 13, d: 1 },
      shoe: { k: 0.87, toe: 30, up: C.shoe, sole: C.sole, buckle: C.gold, foot: FOOT } }));
    const V = (x, y, z) => new THREE.Vector3(X(x), Y(y), D(z));
    const ol = (m, w = 0.009) => {   // contorno; a posição/giro já dados à peça passam para o grupo (a casca vai junto)
      const g = withOutline(m, w); g.position.copy(m.position); g.quaternion.copy(m.quaternion); g.scale.copy(m.scale);
      m.position.set(0, 0, 0); m.quaternion.identity(); m.scale.set(1, 1, 1); return g; };

    // --- padrões no shader (px do quadro: x, y para baixo, z), com os atributos da casca (arc: px ao longo do pano a
    // partir do meio das costas; edge: px até a borda aberta mais perto)
    function pattern(mat, glsl, key, uniforms = {}) {
      const prev = mat.onBeforeCompile, k0 = mat.customProgramCacheKey();
      mat.onBeforeCompile = (sh, r) => {
        prev.call(mat, sh, r);
        Object.assign(sh.uniforms, uniforms);
        const A = /\bvArc\b|\bvEdge\b|\bvAng\b|\bvInner\b/.test(glsl);
        sh.vertexShader = 'varying vec3 vPx;\n' + (A ? 'attribute float arc, edge, ang, inner; varying float vArc, vEdge, vAng, vInner;\n' : '') + sh.vertexShader.replace('#include <begin_vertex>',
          `#include <begin_vertex>\nvPx = vec3(position.x * ${S_PX.toFixed(1)} + ${CX.toFixed(1)}, ${FOOT.toFixed(1)} - position.y * ${S_PX.toFixed(1)}, position.z * ${S_PX.toFixed(1)});` + (A ? ' vArc = arc; vEdge = edge; vAng = ang; vInner = inner;' : ''));
        sh.fragmentShader = 'varying vec3 vPx;\n' + (A ? 'varying float vArc, vEdge, vAng, vInner;\n' : '') + Object.keys(uniforms).map(u => `uniform ${uniforms[u].type || 'vec3'} ${u};\n`).join('') + sh.fragmentShader
          .replace('#include <map_fragment>', '#include <map_fragment>\n' + glsl);
      };
      mat.customProgramCacheKey = () => k0 + '|' + key;
      return mat;
    }
    // casca com espessura entre dois ângulos de cada fileira (th: 0 = +x, π/2 = frente); fileiras de cima para baixo.
    // row(y) = { a, d, cz, t0, t1 }; hem(th) desce a última fileira; zAdj mexe na profundidade; rmod(th, y) põe px a
    // mais no raio (pregas do saiote, dobras do casaco).
    function shell({ y0, y1, step, row, NC = 140, thick = 3, pw = 2.4, hem = null, zAdj = null, rmod = null }) {
      const ys = []; for (let y = y0; y <= y1 + 1e-6; y += step) ys.push(y);
      const O = [], I = [], arc = [], edge = [], ang = [];
      ys.forEach((y, i) => {
        const { a, d, cz, t0, t1 } = row(y), pts = [];
        for (let j = 0; j <= NC; j++) {
          const th = t0 + (t1 - t0) * j / NC, c = Math.cos(th), s = Math.sin(th), dy = hem && i === ys.length - 1 ? hem(th) : 0;
          const fm = rmod ? rmod(th, y + dy) : 0, A = a + fm, Dd = d + fm * d / a; ang.push(th);
          let xo = A * sp(c, pw), zo = cz + Dd * sp(s, pw), xi = (A - thick) * sp(c, pw), zi = cz + (Dd - thick) * sp(s, pw);
          if (zAdj) { zo = zAdj(xo, y, zo, s, true); zi = zAdj(xi, y, zi, s, false); }
          O.push(V(CX + xo, y + dy, zo)); I.push(V(CX + xi, y + dy, zi)); pts.push([xo, zo]);
        }
        const L = [0]; for (let j = 1; j <= NC; j++) L.push(L[j - 1] + Math.hypot(pts[j][0] - pts[j - 1][0], pts[j][1] - pts[j - 1][1]));
        let mid = 0; { let best = 1e9; for (let j = 0; j <= NC; j++) { const th = t0 + (t1 - t0) * j / NC, dd = Math.abs(Math.atan2(Math.sin(th - 1.5 * Math.PI), Math.cos(th - 1.5 * Math.PI))); if (dd < best) { best = dd; mid = L[j]; } } }
        for (let j = 0; j <= NC; j++) { arc.push(L[j] - mid); edge.push(Math.min(L[j], L[NC] - L[j])); }
      });
      const pos = [], N = NC + 1, off = O.length, idx = [];
      for (const v of [...O, ...I]) pos.push(v.x, v.y, v.z);
      for (let i = 0; i < ys.length - 1; i++) for (let j = 0; j < NC; j++) {
        const a = i * N + j, b = a + 1, c = a + N, d = c + 1;
        idx.push(a, b, c, b, d, c, off + a, off + c, off + b, off + b, off + c, off + d);
      }
      for (let i = 0; i < ys.length - 1; i++) for (const j of [0, NC]) { const a = i * N + j, c = a + N; if (j === 0) idx.push(a, c, off + a, off + a, c, off + c); else idx.push(a, off + a, c, off + a, off + c, c); }
      for (const i of [0, ys.length - 1]) for (let j = 0; j < NC; j++) { const a = i * N + j, b = a + 1; if (i === 0) idx.push(a, off + a, b, b, off + a, off + b); else idx.push(a, b, off + a, b, off + b, off + a); }
      const g = new THREE.BufferGeometry(); g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3));
      g.setAttribute('arc', new THREE.Float32BufferAttribute([...arc, ...arc], 1)); g.setAttribute('edge', new THREE.Float32BufferAttribute([...edge, ...edge], 1));
      g.setAttribute('ang', new THREE.Float32BufferAttribute([...ang, ...ang], 1)); g.setAttribute('inner', new THREE.Float32BufferAttribute([...O.map(() => 0), ...I.map(() => 1)], 1));
      g.setIndex(idx); g.computeVertexNormals(); return g;
    }
    // peça afunilada ao longo de uma curva (chifre da garra, gancho, dreads): pts e raios em px
    function horn(pts, radii, side = new THREE.Vector3(1, 0, 0), NA = 16) {
      const curve = new THREE.CatmullRomCurve3(pts.map(p => V(...p))), NS = 40, pos = [], idx = [];
      for (let i = 0; i <= NS; i++) {
        const u = i / NS, c = curve.getPointAt(u), t = curve.getTangentAt(u), n = new THREE.Vector3().crossVectors(t, side).normalize(), b = new THREE.Vector3().crossVectors(n, t).normalize();
        const k = u * (radii.length - 1), i0 = Math.min(radii.length - 2, Math.floor(k)), r = D(radii[i0] + (radii[i0 + 1] - radii[i0]) * (k - i0));
        for (let j = 0; j <= NA; j++) { const a = j / NA * Math.PI * 2; pos.push(c.x + (n.x * Math.cos(a) + b.x * Math.sin(a)) * r, c.y + (n.y * Math.cos(a) + b.y * Math.sin(a)) * r, c.z + (n.z * Math.cos(a) + b.z * Math.sin(a)) * r); }
      }
      for (let i = 0; i < NS; i++) for (let j = 0; j < NA; j++) { const a = i * (NA + 1) + j, b = a + 1, c = a + NA + 1, d = c + 1; idx.push(a, c, b, b, c, d); }
      const lu = [], la = []; for (let i = 0; i <= NS; i++) for (let j = 0; j <= NA; j++) { lu.push(i / NS); la.push(j / NA * Math.PI * 2); }
      const g = new THREE.BufferGeometry(); g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3)); g.setIndex(idx);
      g.setAttribute('lu', new THREE.Float32BufferAttribute(lu, 1)); g.setAttribute('la', new THREE.Float32BufferAttribute(la, 1)); g.computeVertexNormals(); return g;
    }
    const planeMat = t => new THREE.MeshBasicMaterial({ map: t, transparent: true, alphaTest: 0.1, depthWrite: false, polygonOffset: true, polygonOffsetFactor: -4 });

    // ================= cabeça (pivô no pescoço)
    const WAIST = Y(226), NECK = Y(128), HIPS = Y(230);
    const upper = new THREE.Group(); upper.position.y = WAIST; root.add(upper);
    const body = new THREE.Group(); body.position.y = -WAIST; upper.add(body);
    const neck = new THREE.Group(); neck.position.y = NECK - WAIST; upper.add(neck);
    const head = new THREE.Group(); head.position.y = D(14) - NECK; neck.add(head);   // cabeça erguida como a da Valéria (o pescoço aparece acima da gola)
    const HA = 56, HPW = 2, HD = 0.86, { headA, headZ } = B;   // cabeça = ovo da base
    P.head = B.face; head.add(P.head);
    // rosto desenhado em código na textura do ovo (como o da Valéria), pelo retrato do jogo: tapa-olho de fundo redondo
    // com a tira subindo para baixo da bandana, nariz em triângulo, meio sorriso que entra na carapaça com o dente de
    // fora. Coordenadas em px do quadro; a carapaça cobre tudo de x = 177 para a direita.
    const ink = (g, w) => { g.strokeStyle = INKS; g.lineWidth = w; g.lineCap = 'round'; g.lineJoin = 'round'; };
    const SEAM = 176.6;   // onde a carapaça começa na frente do rosto
    const nose = (g, fill) => { g.beginPath(); g.moveTo(SEAM, 100); g.lineTo(167.5, 114.5); g.lineTo(185.5, 114.5); g.closePath(); if (fill) { g.fillStyle = fill; g.fill(); } ink(g, 2.4); g.stroke(); };
    // metade de siri, pintada na carapaça: o vinco (olho fechado da casca, que muda com a expressão), o nariz de casca e
    // a boca de siri, uma fenda escura em zigue-zague com dentes brancos pontudos, continuando a boca dela
    const koMouth = (g, crab) => {
      g.fillStyle = '#4a1414'; g.beginPath(); g.ellipse(SEAM, 122, 11, 8.5, 0, 0, 7); g.fill();
      g.fillStyle = '#e8607a'; g.beginPath(); g.ellipse(SEAM - 2, 126.5, 6, 3, 0, 0, 7); g.fill();
      if (crab) { g.fillStyle = '#f6f2e4'; for (const [x, y, h] of [[SEAM + 3, 114.2, 4], [SEAM + 7.5, 115.6, 3.4], [SEAM + 4, 129.6, -3.6], [SEAM + 8, 128.2, -3]]) { g.beginPath(); g.moveTo(x - 2, y); g.lineTo(x + 2, y); g.lineTo(x, y + h); g.closePath(); g.fill(); ink(g, 1.1); g.stroke(); g.fillStyle = '#f6f2e4'; } }
      g.beginPath(); g.ellipse(SEAM, 122, 11, 8.5, 0, 0, 7); ink(g, 2.2); g.stroke(); };
    function crabFace(g, f) {
      const strap = () => { g.save(); g.translate(0, 3); ink(g, 3.2); g.beginPath(); g.moveTo(136, 83); g.lineTo(121, 61); g.moveTo(163, 82); g.lineTo(180, 58); g.stroke(); g.restore(); };
      g.fillStyle = 'rgba(255, 196, 140, 0.55)'; for (const [x, y, r] of [[196, 74, 3.2], [205, 80, 2.4], [212, 106, 2.8], [199, 132, 2.6], [190, 70, 2]]) { g.beginPath(); g.ellipse(x, y, r * 1.3, r, 0.4, 0, 7); g.fill(); }
      ink(g, 1.6); g.beginPath(); g.moveTo(181, 84); g.quadraticCurveTo(200, 80, 224, 86); g.moveTo(190, 104); g.lineTo(196, 126); g.lineTo(193, 142); g.moveTo(212, 112); g.lineTo(222, 128); g.stroke();   // emendas das placas
      strap();   // a tira do tapa-olho passa por cima da casca
      nose(g, C.skin);   // nariz inteiro na cor da pele, mesmo do lado de siri
      if (f === 'ko') { koMouth(g, true); nose(g, C.skin); return; }
      const open = f === 'angry' ? 8 : f === 'ko' ? 10 : f === 'pain' ? 1.5 : 4.5, y0 = f === 'angry' ? 116 : 119, y1 = 113;   // da emenda até o canto
      const top = x => y0 + (y1 - y0) * (x - SEAM + 2) / 32, xs = [SEAM - 9, 183, 190, 197, 204, 207];
      g.beginPath(); g.moveTo(xs[0], top(xs[0])); for (const x of xs.slice(1)) g.lineTo(x, top(x) + ((x * 7) % 2 ? -1.2 : 1.2));
      g.lineTo(207, top(207) + open * 0.45); for (const x of xs.slice(0, -1).reverse()) g.lineTo(x, top(x) + open * (x < 185 ? 0.95 : 0.8));
      g.closePath(); g.fillStyle = '#3a0f06'; g.fill(); ink(g, 1.8); g.stroke();
      g.fillStyle = '#f6f2e4';
      const tooth = (x, y, w, h) => { g.beginPath(); g.moveTo(x - w / 2, y); g.lineTo(x + w / 2, y); g.lineTo(x, y + h); g.closePath(); g.fill(); ink(g, 1.2); g.stroke(); g.fillStyle = '#f6f2e4'; };
      if (f !== 'pain') { for (const [x, w, h] of [[180.5, 5, 5.5], [189.5, 4.5, 4.6], [198, 4, 3.8]]) tooth(x, top(x) - 0.3, w, h);
        for (const [x, w, h] of (f === 'angry' || f === 'ko' ? [[185, 4.2, -4], [194, 4, -3.6]] : [[186, 3.6, -2.6]])) tooth(x, top(x) + open * 0.9, w, h); }
    }
    function joanaFace(g, f) {
      g.save(); g.translate(0, 3);   // tapa-olho 3 px abaixo da barra da bandana (sobra lugar para a sobrancelha)
      ink(g, 3.2); g.beginPath(); g.moveTo(136, 83); g.lineTo(121, 61); g.moveTo(163, 82); g.lineTo(180, 58); g.stroke();   // tira (some embaixo da bandana)
      g.fillStyle = INKS; g.beginPath(); g.moveTo(132.5, 81.5); g.lineTo(166.5, 80.5); g.bezierCurveTo(168.5, 96, 160, 104.5, 149, 104.5); g.bezierCurveTo(138, 104.5, 130.5, 96, 132.5, 81.5); g.closePath(); g.fill();
      g.fillStyle = '#2b3037'; g.beginPath(); g.moveTo(136, 84.5); g.lineTo(163.2, 83.8); g.bezierCurveTo(164.4, 95, 158, 101, 149, 101); g.bezierCurveTo(140, 101, 134.6, 95, 136, 84.5); g.closePath(); g.fill();
      g.strokeStyle = 'rgba(255,255,255,0.32)'; g.lineWidth = 1.5; g.lineCap = 'round'; g.beginPath(); g.moveTo(139, 88); g.quadraticCurveTo(139.5, 95, 145, 98); g.stroke(); g.restore();   // brilho do couro
      if (f === 'angry') { ink(g, 3.8); g.beginPath(); g.moveTo(130, 76.5); g.lineTo(167, 83); g.stroke(); }        // sobrancelha desce para o nariz
      if (f === 'pain') { ink(g, 3.8); g.beginPath(); g.moveTo(131, 82.5); g.lineTo(165, 76.5); g.stroke(); }       // sobe no meio
      g.fillStyle = 'rgba(110, 48, 28, 0.35)'; g.beginPath(); g.ellipse(173, 117, 6, 2.4, 0, 0, 7); g.fill();     // sombra do nariz
      nose(g, null);
      if (f === 'angry') {                                  // dentes cerrados
        g.fillStyle = '#f6f2e4'; rrect(g, 146, 116, 36, 10.5, 3.5); g.fill(); ink(g, 2.2); g.stroke();
        ink(g, 1.2); g.beginPath(); g.moveTo(147, 121.2); g.lineTo(176, 121.2); for (const x of [153.5, 160.5, 167.5]) { g.moveTo(x, 116.5); g.lineTo(x, 126); } g.stroke();
      } else if (f === 'pain') {                            // boca torta ondulada
        ink(g, 2.4); g.beginPath(); g.moveTo(143, 122); g.quadraticCurveTo(148.5, 116.5, 154, 122); g.quadraticCurveTo(159.5, 127.5, 165, 122); g.quadraticCurveTo(170.5, 116.5, 177, 121); g.stroke();
      } else if (f === 'ko') {                              // "oh"
        koMouth(g, false); nose(g, C.skin);   // o nariz por cima da boca aberta
      } else {                                              // sorriso aberto, dentes de cima à mostra
        g.fillStyle = '#f6f2e4'; g.beginPath(); g.moveTo(141, 118.5); g.quadraticCurveTo(162, 122.5, 186, 116.5); g.quadraticCurveTo(176, 129.5, 158, 129); g.quadraticCurveTo(146, 128, 141, 118.5); g.closePath(); g.fill(); ink(g, 2.4); g.stroke();
        ink(g, 1.2); g.beginPath(); g.moveTo(144, 122); g.quadraticCurveTo(160, 125.5, 176, 121); g.stroke(); }
    }
    const drawFace = f => (g, at) => at(CX, 117, 140, 140, gg => { gg.translate(-110, -47); gg.translate(2 * CX, 0); gg.scale(-1, 1); joanaFace(gg, f); }, 140, 140);   // desenhado espelhado: siri à esquerda da tela   // a caixa do rosto da base
    const faceMat = B.faceMat; faceMat.map.dispose(); faceMat.map = B.faceTexture(drawFace('normal'));
    const crabC = document.createElement('canvas'); crabC.width = crabC.height = 1024;
    const crabTex = new THREE.CanvasTexture(crabC); crabTex.anisotropy = 8;
    const paintCrab = f => { const g = crabC.getContext('2d'), k = 1024 / 140; g.setTransform(1, 0, 0, 1, 0, 0); g.clearRect(0, 0, 1024, 1024); g.setTransform(-k, 0, 0, k, (2 * CX - 110) * k, -47 * k); crabFace(g, f); crabTex.needsUpdate = true; };
    paintCrab('normal');
    // chapéu de três pontas (tricórnio) marrom, como na imagem: copa arredondada com faixa escura e a aba larga virada
    // para cima entre as três pontas (uma na frente, duas atrás dos lados); a aba é uma placa com espessura
    { const HAT = '#a3262a', HATD = '#5e1416', HY = 64, hat = new THREE.Group(); head.add(hat);
      const crown = []; for (let y = 14; y <= HY + 2; y += 2) { const t = (y - 14) / (HY + 2 - 14), a = 60 * Math.sqrt(Math.max(0.04, 1 - (1 - t) ** 2 * 0.9)); crown.push({ y, l: CX - a, r: CX + a }); }
      hat.add(ol(new THREE.Mesh(loft(crown, { depth: 0.92, cz: -2, NA: 72, capTop: true, capBot: false }), flat(HAT)), 0.010));
      const band = []; for (let y = HY - 8; y <= HY + 1; y += 1.5) band.push({ y, l: CX - 61.5, r: CX + 61.5 });
      hat.add(ol(new THREE.Mesh(loft(band, { depth: 0.92, cz: -2, NA: 72, capTop: false, capBot: false }), flat(HATD)), 0.005));
      const NB = 144, NR = 8, pos = [], idx = [], k3 = th => Math.cos(3 * (th - Math.PI / 2));   // 1 nas pontas
      // tricórnio de verdade: a aba sai reta um pouco e depois sobe quase em pé entre as pontas (as três paredes); nas
      // pontas ela não sobe e se estica para fora, formando o bico; friso dourado na borda de cima das paredes
      const brim = (th, u, up) => { const sf = 1 - Math.pow(0.5 + 0.5 * k3(th), 3), w = Math.max(0, (u - 0.3) / 0.7);
        const r = 60 + 14 * Math.min(1, u / 0.3) + (4 * sf + 30 * (1 - sf)) * w, lift = 40 * sf * Math.pow(w, 0.85);
        return V(CX + r * Math.cos(th), HY + 2 - lift - (up ? 2.5 : 0), -2 + r * 0.92 * Math.sin(th)); };
      for (const up of [true, false]) for (let i = 0; i <= NR; i++) for (let j = 0; j <= NB; j++) { const v = brim(j / NB * Math.PI * 2, i / NR, up); pos.push(v.x, v.y, v.z); }
      const W = NB + 1, off = (NR + 1) * W;
      for (let i = 0; i < NR; i++) for (let j = 0; j < NB; j++) { const a = i * W + j, b = a + 1, c = a + W, d = c + 1; idx.push(a, c, b, b, c, d, off + a, off + b, off + c, off + b, off + d, off + c); }
      for (let j = 0; j < NB; j++) { const a = NR * W + j, b = a + 1; idx.push(a, off + a, b, b, off + a, off + b); }   // borda de fora
      const bg = new THREE.BufferGeometry(); bg.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3)); bg.setIndex(idx); bg.computeVertexNormals();
      const bm = flat(HAT); bm.side = THREE.DoubleSide; hat.add(ol(new THREE.Mesh(bg, bm), 0.009));
      { const pts = []; for (let j = 0; j < 180; j++) pts.push(brim(j / 180 * Math.PI * 2, 1, true));
        hat.add(ol(new THREE.Mesh(new THREE.TubeGeometry(new THREE.CatmullRomCurve3(pts, true), 240, D(2.4), 8, true), flat(C.gold)), 0.004)); }   // friso dourado
      P.hat = hat; }
    // a metade de siri é pintada na própria pele (sem relevo), com a divisão irregular em zigue-zague: laranja da
    // carapaça com a borda mais escura junto da divisão e o risco preto nela; na frente, os desenhos de siri por cima
    const koU = { value: 0 };   // 1 no K.O. (a boca aberta atravessa a divisão)
    const CRAB_SEAM = Y => `(183.4 + 2.7 * (abs(fract(${Y} / 9.0) * 2.0 - 1.0) - 0.5) * 2.0 + 3.0 * sin(${Y} * 0.19 + 1.3) + 1.6 * sin(${Y} * 0.53))`;
    { const prevFB = faceMat.onBeforeCompile;
      faceMat.onBeforeCompile = (sh, r) => { prevFB(sh, r);
        Object.assign(sh.uniforms, { uKO: koU, uCrab: { value: crabTex }, uShellC: col(C.shell), uShellD: col(C.shellDark), uInkC: col('#1a0a04') });
        sh.vertexShader = 'varying vec3 vFp;\n' + sh.vertexShader.replace('#include <begin_vertex>', `#include <begin_vertex>
vFp = vec3(position.x * ${S_PX.toFixed(1)} + ${CX.toFixed(1)}, ${FOOT.toFixed(1)} - position.y * ${S_PX.toFixed(1)}, position.z * ${S_PX.toFixed(1)});`);
        sh.fragmentShader = 'uniform sampler2D uCrab; uniform float uKO; uniform vec3 uShellC, uShellD, uInkC; varying vec3 vFp;\n' + sh.fragmentShader.replace('#include <color_fragment>', `#include <color_fragment>
          { float dd = ${CRAB_SEAM('vFp.y')} - vFp.x, aa = fwidth(dd) * 0.75, crab = smoothstep(-aa, aa, dd);
            vec2 np = vFp.xy; float nose = step(101.0, np.y) * step(np.y, 113.6) * step(abs(np.x - 183.4), (np.y - 100.0) * 0.621 - 0.9);   // triângulo do nariz (lado espelhado)
            if (vFp.z > 8.0) { crab *= 1.0 - nose; dd += nose * 100.0; }
            float mo = uKO * step(length((vFp.xy - vec2(183.4, 122.0)) / vec2(12.2, 9.6)), 1.0) * step(8.0, vFp.z);   // dentro da boca do K.O. não tem o risco da divisão
            dd += mo * 100.0 * sign(dd + 1e-4);
            vec3 cc = mix(uShellD, uShellC, smoothstep(2.6 - aa, 2.6 + aa, dd));
            vec2 cuv = vec2((vFp.x - 110.0) / 140.0, 1.0 - (vFp.y - 47.0) / 140.0);
            if (vFp.z > 8.0 && cuv.x > 0.0 && cuv.x < 1.0 && cuv.y > 0.0 && cuv.y < 1.0) { vec4 ct = texture2D(uCrab, cuv); cc = mix(cc, pow(ct.rgb, vec3(2.2)), ct.a); }
            diffuseColor.rgb = mix(diffuseColor.rgb, cc, crab);
            diffuseColor.rgb = mix(diffuseColor.rgb, uInkC, 1.0 - smoothstep(1.1 - aa, 1.1 + aa, abs(dd))); }`);
      };
      faceMat.customProgramCacheKey = () => 'joanaFace'; faceMat.needsUpdate = true; }
    // olho de siri na haste, saindo do alto da carapaça: globo creme (esclera) com a pupila preta em pé e o brilho, aro
    // laranja em volta da frente, concha laranja atrás e a pálpebra de casca que desce com a expressão
    const eyeTex = {};
    { const draw = ko => { const c = document.createElement('canvas'); c.width = c.height = 128; const g = c.getContext('2d');
        if (ko) { g.strokeStyle = INKS; g.lineWidth = 15; g.lineCap = 'round'; g.beginPath(); g.moveTo(36, 36); g.lineTo(92, 92); g.moveTo(92, 36); g.lineTo(36, 92); g.stroke(); }
        else { g.fillStyle = INKS; g.beginPath(); g.ellipse(64, 66, 22, 40, 0, 0, 7); g.fill(); g.fillStyle = '#ffffff'; g.beginPath(); g.ellipse(74, 44, 8, 10, 0, 0, 7); g.fill(); }
        const t = new THREE.CanvasTexture(c); t.colorSpace = THREE.SRGBColorSpace; return t; };
      eyeTex.on = draw(false); eyeTex.ko = draw(true);
      { const c = document.createElement('canvas'); c.width = c.height = 128; const g = c.getContext('2d'); g.strokeStyle = INKS; g.lineWidth = 14; g.lineCap = 'round'; g.lineJoin = 'round';
        g.beginPath(); g.moveTo(40, 34); g.lineTo(88, 64); g.lineTo(40, 94); g.stroke(); eyeTex.pain = new THREE.CanvasTexture(c); eyeTex.pain.colorSpace = THREE.SRGBColorSpace; }
      const E = [144, 24, 87], R = 14;
      head.add(ol(new THREE.Mesh(horn([[154, 92, 44], [151, 76, 66], [147, 52, 86], [145, 36, 88]], [6.5, 6, 5.5, 5]), flat(C.shell)), 0.006));   // haste saindo do meio da metade de siri
      const eg = new THREE.Group(); eg.position.copy(V(...E)); head.add(eg);
      eg.add(withOutline(new THREE.Mesh(new THREE.SphereGeometry(D(R), 32, 22), flat('#f4ecd2')), 0.008));
      const cup = new THREE.Mesh(new THREE.SphereGeometry(D(R + 1.2), 28, 18, 0, Math.PI * 2, Math.PI / 3, Math.PI * 2 / 3), flat(C.shell)); cup.rotation.x = Math.PI / 2; cup.material.side = THREE.DoubleSide; eg.add(withOutline(cup, 0.006));
      { const lip = new THREE.Mesh(new THREE.TorusGeometry(D((R + 1.2) * Math.sin(Math.PI / 3)), D(1.6), 8, 36), flat(C.shell)); lip.position.z = D((R + 1.2) * 0.5); eg.add(withOutline(lip, 0.004)); }   // borda da concha: uma abertura só
      const pupil = new THREE.Mesh(new THREE.PlaneGeometry(D(R * 1.1), D(R * 1.1)), planeMat(eyeTex.on)); pupil.position.z = D(R + 0.4); eg.add(pupil); P.pupil = pupil;
      const lidOf = top => { const lg = new THREE.Group(); eg.add(lg);
        const m = new THREE.Mesh(new THREE.SphereGeometry(D(R + 0.7), 28, 12, 0, Math.PI * 2, top ? 0 : Math.PI - 1.1, 1.1), flat(C.shell)); m.material.side = THREE.DoubleSide; lg.add(withOutline(m, 0.006)); return lg; };
      P.lid = lidOf(true); P.lidLow = lidOf(false); P.lid.rotation.x = -0.75; P.lidLow.rotation.x = 0.75; }
    // dreads: mechas grossas que saem de baixo da bandana (lado direito dela e nuca) e caem pelo ombro e pelas costas
    // (cada uma presa na raiz e com mola: ficam para trás quando a cabeça mexe e voltam balançando, como o cabelo da
    // Valéria); o trançado é uma espiral mais escura no shader, anéis dourados em algumas
    const dreadMat = (c, k) => { const m = flat(c);
      m.onBeforeCompile = sh => {
        sh.vertexShader = 'attribute float lu, la; varying float vLU, vLA;\n' + sh.vertexShader.replace('#include <begin_vertex>', '#include <begin_vertex>\nvLU = lu; vLA = la;');
        sh.fragmentShader = 'varying float vLU, vLA;\n' + sh.fragmentShader.replace('#include <map_fragment>', `#include <map_fragment>
          { float k = fract(vLU * 11.0 + vLA / 6.2831 * 1.0); float w = fwidth(k) * 1.5;
            diffuseColor.rgb *= mix(1.0, 0.6, smoothstep(0.30 - w, 0.30 + w, k) * (1.0 - smoothstep(0.62 - w, 0.62 + w, k))); }`);
      };
      m.customProgramCacheKey = () => 'dread' + k; return m; };
    const dreadM = [dreadMat('#7a4630', 0), dreadMat('#5e3422', 1)];   // castanho em dois tons alternados, com o trançado
    P.dreads = [];
    { const locks = [[160, 200], [170, 206], [180, 210], [200, 212], [222, 212], [244, 212], [266, 212], [288, 212], [310, 212], [330, 210], [348, 206], [362, 200], [376, 196], [20, 194]];   // em volta de tudo menos o rosto
      locks.forEach(([phD, yEnd], i) => {
        const ph = phD * Math.PI / 180, at = r => [CX + r * Math.cos(ph), r * Math.sin(ph)];
        const pts = [[...at(HA - 4), 0], [...at(HA + 8), 0], [...at(HA + 14), 0], [...at(HA + 12), 0]].map((p, k) => [p[0], [66, 110, 160, yEnd][k], p[1]]);
        const root = V(...pts[0]), dg = new THREE.Group(); dg.position.copy(root); head.add(dg);
        const g = horn(pts, [9, 10.5, 10, 5.5]); g.translate(-root.x, -root.y, -root.z);
        dg.add(withOutline(new THREE.Mesh(g, dreadM[i % 2]), 0.008));
        if (i % 3 === 1) { const at = new THREE.CatmullRomCurve3(pts.map(q => V(...q))).getPointAt(0.55 + (i % 2) * 0.12), rg = new THREE.Mesh(new THREE.CylinderGeometry(D(11.5), D(11.5), D(5), 18), flat(C.gold)); rg.position.copy(at).sub(root); dg.add(ol(rg, 0.004)); }   // anel dourado
                P.dreads.push(dg);
      }); }

    // ================= tronco: barriga de fora, top faixa listrado com busto, colete curto
    const TA = y => 41, TD = 0.74;
    { const m = flat(C.skin);   // barriga de fora (o peito e os ombros são o pescoço da base, com a pintura de siri)
      B.torso.children[0].material = m; body.add(B.torso);
      // top faixa: por cima do tronco, com volume de busto na frente; listras creme e vermelho; borda de cima em curva
      { const rows = []; for (let y = 138; y <= 188; y += 1.5) rows.push({ y, l: CX - 44, r: CX + 44 });
        const bust = (x, y, z, f) => z + (f > 0 ? 12 * Math.exp(-(((y - 156) / 13) ** 2)) * f * f * Math.exp(-(((Math.abs(x - CX) - 19) / 12) ** 2)) : 0);
        const tm = pattern(flat(C.cream), `
          { if (fract((vPx.y - 138.0) / 12.5) > 0.5) diffuseColor.rgb = uRed;
            if (vPx.y > 185.5) diffuseColor.rgb = uInk; }`, 'bandeau', { uRed: col(C.stripe), uInk: col('#16120c') });
        body.add(ol(new THREE.Mesh(loft(rows, { depth: TD, cz: -2, NA: 72, capTop: false, capBot: false, zFn: bust }), tm), 0.009)); }
    }
    // casaco: barril escuro aberto na frente; perto da abertura assenta no top; friso dourado na borda e na barra,
    // lapela mais clara no alto, costura no meio das costas
    { const CA = [[122, 44], [130, 51], [146, 54], [164, 52], [184, 47], [200, 45], [212, 47], [222, 50]];   // colete: ombro, busto, cintura fina e abre um pouco no quadril
      const cA = y => { let i = 0; while (i < CA.length - 2 && y > CA[i + 1][0]) i++; const [y0, a0] = CA[i], [y1, a1] = CA[i + 1], t = Math.max(0, Math.min(1, (y - y0) / (y1 - y0))); return a0 + (a1 - a0) * t * t * (3 - 2 * t); };
      const GAP = 30, CD = 0.68;
      const zAdj = (x, y, z, s, outer) => { if (s <= 0) return z; const w = 1 - ss(GAP, GAP + 26, Math.abs(x)); const ta = TA(y) + 3, want = -2 + ta * TD * Math.sqrt(Math.max(0, 1 - Math.min(1, (Math.abs(x) / ta) ** 2.4))) + (outer ? 6 : 3); return z + (Math.min(z, want) - z) * w; };
      const NF = 14, foldK = y => 0;
      const g = shell({ y0: 122, y1: 222, step: 1.5, thick: 3, row: y => { const a = cA(y), sn = Math.pow(GAP / a, 2.4 / 2); const t0 = Math.asin(Math.min(0.999, sn)); return { a, d: a * CD, cz: -4, t0: Math.PI / 2 + t0, t1: Math.PI * 2.5 - t0 }; }, zAdj,
        rmod: (th, y) => { const a = cA(y), t0 = Math.asin(Math.min(0.999, Math.pow(GAP / a, 1.2))), near = (Math.PI - t0 - Math.abs(th - Math.PI * 1.5)) * a;
          return foldK(y) * Math.cos(NF * th) * ss(6, 24, near); } });   // as dobras somem perto da abertura (a borda fica lisa para o friso)
      const m = pattern(flat(C.coat), `
        { float fs = cos(${NF.toFixed(1)} * vAng), fw = 0.075 * smoothstep(160.0, 228.0, vPx.y);
          if (vInner < 0.5 && fs < -1.0 + fw && vEdge > 12.0) diffuseColor.rgb = uInk;     // risco no fundo da dobra
          
          if (abs(vArc) < 1.3 && vPx.z < 0.0) diffuseColor.rgb = uLight;                   // costura das costas
          float trim = (1.0 - smoothstep(4.6, 5.2, vEdge)) * smoothstep(1.2, 1.8, vEdge);
          float hem = smoothstep(${(222 - 6).toFixed(1)}, ${(222 - 5.4).toFixed(1)}, vPx.y) * (1.0 - smoothstep(${(222 - 2).toFixed(1)}, ${(222 - 1.4).toFixed(1)}, vPx.y));
          diffuseColor.rgb = mix(diffuseColor.rgb, uGold, max(trim, hem));
          if (vInner > 0.5 && vEdge > 1.8) { diffuseColor.rgb = uLining; extraTone = 0.65; } }`, 'coat', { uLight: col(C.coatLight), uGold: col('#7a5234'), uInk: col('#0b0d12'), uLining: col('#4a2a20') });   // friso marrom, forro escuro   // forro vinho por dentro
      m.side = THREE.DoubleSide; P.clothGeos = [g];
      body.add(ol(new THREE.Mesh(g, m), 0.010)); }

    // ================= quadril: short creme de listras vermelhas (como no retrato e no modelo), cinto com fivela dourada.
    // O assento fica no quadril; cada perna do short vai presa na perna (acompanha a passada) e termina na barra dobrada.
    const hips = new THREE.Group(); hips.position.y = HIPS; root.add(hips);
    const low = new THREE.Group(); low.position.y = -HIPS; hips.add(low);
    // xo, yo: onde o (0, 0) da peça cai no quadro (as pernas do short são desenhadas no espaço da perna)
    const stripes = (n, extra = '', xo = 0, yo = 0) => pattern(flat(C.skirt), `
        { vec2 w = vec2(vPx.x + ${xo.toFixed(1)}, vPx.y - ${yo.toFixed(1)});                 // px do quadro
          float u = (w.x - 180.0) / 19.0 + 0.5;                                             // faixas verticais pela largura: iguais na frente e nas costas, contínuas da cintura às pernas
          if (abs(fract(u) - 0.5) < 0.25) diffuseColor.rgb = uStripe;
          // retalhos costurados: um no assento, um na perna esquerda da tela (pano marrom e azul gasto, ponto tracejado)
          for (int k = 0; k < 2; k++) {
            vec4 r = k == 0 ? vec4(196.0, 238.0, 216.0, 256.0) : vec4(127.0, 266.0, 147.0, 283.0);
            vec2 q = w - r.xy, sz = r.zw - r.xy;
            if (vPx.z > 0.0 && q.x > 0.0 && q.y > 0.0 && q.x < sz.x && q.y < sz.y) {
              diffuseColor.rgb = k == 0 ? uPatchA : uPatchB;
              float e = min(min(q.x, sz.x - q.x), min(q.y, sz.y - q.y));
              float along = q.x + q.y;
              if (e < 1.6 && e > 0.6 && fract(along / 3.0) < 0.55) diffuseColor.rgb = uInk;   // ponto
              if (e < 0.5) diffuseColor.rgb = uInk; } }
          ${extra}
        }`, 'short' + n + extra.length + '|' + xo, { uStripe: col(C.stripe), uInk: col('#16120c'), uPatchA: col('#8a5e3c'), uPatchB: col('#5d7088') });
    { const rows = []; for (let y = 222; y <= 266; y += 1.5) { const a = (49 + 12 * ss(222, 248, y)) * Math.sqrt(Math.max(0.36, 1 - Math.max(0, (y - 244) / 23) ** 2)); rows.push({ y, l: CX - a, r: CX + a }); }   // embaixo arredonda para dentro das coxas
      const m = stripes(12, `extraTone = max(extraTone, 0.6 * (1.0 - smoothstep(234.0, 242.0, vPx.y)));   // sombra do cinto`);
      low.add(ol(new THREE.Mesh(loft(rows, { depth: 0.72, cz: -4, NA: 72, pw: 2.2, capTop: false, capBot: false }), m), 0.009));
      const sb = new THREE.Mesh(new THREE.CircleGeometry(D(37), 48), flat(C.skirt)); sb.material.side = THREE.DoubleSide; sb.rotation.x = Math.PI / 2; sb.scale.y = 0.72; sb.position.copy(V(CX, 265.5, -4)); low.add(sb);   // fundo chato do assento (entre as coxas)
      // cinto marrom com fivela dourada, e a tampa de cima (de cima não aparece o vão entre o tronco e o short)
      const BA = 51.5, bR = []; for (let y = 221; y <= 233; y += 2) bR.push({ y, l: CX - BA, r: CX + BA });
      low.add(ol(new THREE.Mesh(loft(bR, { depth: 0.72, cz: -4, NA: 72, capTop: false, capBot: false }), flat(C.waist)), 0.007));
      const cap = new THREE.Mesh(new THREE.RingGeometry(D(36), D(BA), 48), flat(C.waist)); cap.material.side = THREE.DoubleSide; cap.rotation.x = -Math.PI / 2; cap.scale.y = 0.72; cap.position.copy(V(CX, 221, -4)); low.add(cap);
      const bz = -4 + BA * 0.72;
      const bk = ol(new THREE.Mesh(new RoundedBoxGeometry(D(18), D(14), D(3.5), 2, D(1.5)), flat(C.gold)), 0.005); bk.position.copy(V(CX, 227, bz + 1.4)); low.add(bk);
      const bi = new THREE.Mesh(new RoundedBoxGeometry(D(9), D(6.5), D(2), 2, D(1)), flat(C.waist)); bi.position.copy(V(CX, 227, bz + 2.8)); low.add(bi); }
    // perna do short em volta da coxa (no espaço da perna: pivô no quadril), listras em volta, barra dobrada mais larga
    const shortLeg = (x, top) => {
      const g = new THREE.Group(), rows = [], cuff = [];
      for (let y = 248; y <= 286; y += 2) { const r = 25 + 2 * ss(248, 286, y); rows.push({ y, l: x - r, r: x + r }); }
      for (let y = 284; y <= 293; y += 1.5) cuff.push({ y, l: x - 28.5, r: x + 28.5 });
      for (const [rr, cb] of [[rows, false], [cuff, true]]) {
        const lg = loft(rr, { depth: 0.9, cz: 0, NA: 48, capTop: false, capBot: false }); lg.translate(-X(x), -Y(top), 0);
        const m = stripes(8, cb ? `if (w.y < 285.5) diffuseColor.rgb = uInk;` : '', x - CX, FOOT - top); m.side = THREE.DoubleSide;
        g.add(ol(new THREE.Mesh(lg, m), cb ? 0.007 : 0.009));
      }
      return g;
    };

    // ================= pernas: de pau (direita dela, esquerda da tela) e de carne com sapato
    const LEG_TOP = 268, ANK = 312, legs = {}, feet = {}, LXp = { [-1]: 143, [1]: 217 };
    for (const s of [-1, 1]) {
      const x = LXp[s], peg = s > 0, rows = [];
      for (let y = LEG_TOP; y <= ANK + 4; y += 3) { const r = peg ? 10 : 15; rows.push({ y, l: x - r, r: x + r }); }
      if (!peg) { legs[s] = B.legs[s]; feet[s] = B.feet[s]; root.add(legs[s], feet[s]); continue; }   // perna e sapato da base
      const g = loft(rows, { depth: 1, cz: 0, NA: 28, capTop: false, capBot: true }); g.translate(-X(x), -Y(LEG_TOP), 0);
      const leg = new THREE.Group(); leg.position.copy(V(x, LEG_TOP, 0)); root.add(leg);
      leg.add(ol(new THREE.Mesh(g, flat(C.wood)), 0.008));
      legs[s] = leg;
      const f = new THREE.Group(); f.position.copy(V(x, ANK, 0)); root.add(f);
      if (peg) {   // ponta da perna de pau, com anel dourado
        const tip = new THREE.CylinderGeometry(D(9.5), D(10), D(FOOT - ANK + 2), 20); tip.translate(0, D(-(FOOT - ANK) / 2), 0); f.add(ol(new THREE.Mesh(tip, flat(C.wood)), 0.008));   // reta até o chão
        const ring = new THREE.CylinderGeometry(D(10.6), D(10.6), D(3), 20); ring.translate(0, D(-(FOOT - ANK) + 2), 0); f.add(new THREE.Mesh(ring, flat(C.woodDark)));
      } else {     // sapato: cano baixo marrom, sola escura, bico para a frente
        const up = new RoundedBoxGeometry(D(42), D(16), D(50), 4, D(7)); up.translate(0, D(-10), D(8)); f.add(ol(new THREE.Mesh(up, flat(C.shoe)), 0.008));
        const so = new RoundedBoxGeometry(D(46), D(7), D(54), 3, D(3)); so.translate(0, D(-20), D(8)); f.add(ol(new THREE.Mesh(so, flat(C.sole)), 0.007));
      }
      feet[s] = f;
    }
    for (const s of [-1, 1]) legs[s].add(shortLeg(LXp[s], LEG_TOP));   // o short vai junto com a perna
    { const cf = new THREE.CylinderGeometry(D(18), D(16), D(18), 24); cf.translate(0, Y(305) - Y(LEG_TOP), 0); legs[-1].add(ol(new THREE.Mesh(cf, flat(C.shoe)), 0.007)); }   // cano dobrado da bota
    const placeFoot = legPlacer({ legs, feet, hipX: { [-1]: X(LXp[-1]), [1]: X(LXp[1]) }, top: Y(LEG_TOP), bot: Y(ANK), hips, hipsY: HIPS });

    // ================= mãos: garra de siri (direita dela) e gancho (esquerda). Y local = para onde o golpe vai, Z = cima
    const L = (x, y, z) => new THREE.Vector3(D(x), D(y), D(z));   // px no espaço da mão
    function claw() {
      const g = new THREE.Group(), R = 34;
      g.add(ol(new THREE.Mesh(new THREE.SphereGeometry(D(R), 36, 26), flat(C.shell)), 0.009));
      const hornG = (pts, radii) => { const c = new THREE.CatmullRomCurve3(pts.map(p => L(...p))), NS = 30, NA = 14, pos = [], idx = [], side = new THREE.Vector3(1, 0, 0);
        for (let i = 0; i <= NS; i++) { const u = i / NS, p = c.getPointAt(u), t = c.getTangentAt(u), n = new THREE.Vector3().crossVectors(t, side).normalize(), b = new THREE.Vector3().crossVectors(n, t).normalize(), k = u * (radii.length - 1), i0 = Math.min(radii.length - 2, Math.floor(k)), r = D(radii[i0] + (radii[i0 + 1] - radii[i0]) * (k - i0));
          for (let j = 0; j <= NA; j++) { const a = j / NA * Math.PI * 2; pos.push(p.x + (n.x * Math.cos(a) + b.x * Math.sin(a)) * r, p.y + (n.y * Math.cos(a) + b.y * Math.sin(a)) * r, p.z + (n.z * Math.cos(a) + b.z * Math.sin(a)) * r); } }
        for (let i = 0; i < NS; i++) for (let j = 0; j < NA; j++) { const a = i * (NA + 1) + j, b = a + 1, cc = a + NA + 1, d = cc + 1; idx.push(a, cc, b, b, cc, d); }
        const gg = new THREE.BufferGeometry(); gg.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3)); gg.setIndex(idx); gg.computeVertexNormals(); return gg; };
      // pinças em placa angulosa (como na imagem): perfil chanfrado com a ponta virada, espessura de lado; a de cima é
      // maior e gira para abrir. Pontas de metal em pirâmide nas faces de fora, losangos de metal no punho.
      const METAL = '#b9b4a2';
      const plate = (prof, th) => { const sh = new THREE.Shape(prof.map(([y, z]) => new THREE.Vector2(D(y), D(z))));
        const eg = new THREE.ExtrudeGeometry(sh, { depth: D(th), bevelEnabled: true, bevelThickness: D(2), bevelSize: D(2), bevelSegments: 1 });
        const p = eg.attributes.position; for (let i = 0; i < p.count; i++) { const x = p.getX(i), y = p.getY(i), z = p.getZ(i); p.setXYZ(i, z - D(th / 2), x, y); }
        eg.computeVertexNormals(); return eg; };
      const stud = (grp, y, z, side, sz = 7) => { const c = new THREE.Mesh(new THREE.ConeGeometry(D(sz * 0.62), D(sz), 3), flat(METAL)); c.position.copy(L(side * 13.5, y, z));
        c.quaternion.setFromUnitVectors(new THREE.Vector3(0, 1, 0), new THREE.Vector3(side, 0, 0)); c.rotateY(Math.PI / 6); grp.add(ol(c, 0.004)); };
      const top = new THREE.Group(); top.position.copy(L(0, 18, 8)); g.add(top);
      top.add(ol(new THREE.Mesh(plate([[0, 0], [0, 16], [22, 27], [48, 26], [68, 15], [79, 0], [70, 2], [52, 9], [28, 9], [8, 3]], 22), flat(C.shell)), 0.009));
      const bot = new THREE.Group(); bot.position.copy(L(0, 18, -10)); g.add(bot);
      bot.add(ol(new THREE.Mesh(plate([[0, 0], [0, -15], [22, -24], [44, -22], [60, -11], [66, 3], [58, -2], [40, -7], [20, -7], [6, -3]], 20), flat(C.shell)), 0.009));
      top.scale.setScalar(1.12); bot.scale.setScalar(1.08); top.position.copy(L(0, 12, 8)); bot.position.copy(L(0, 12, -8));   // pinças grandes como na imagem
      for (const sd of [-1, 1]) { stud(top, 22, 16, sd); stud(top, 44, 17, sd); stud(top, 64, 9, sd, 6); stud(bot, 22, -14, sd); stud(bot, 42, -13, sd, 6); }
      for (const sd of [-1, 1]) { const dm = new THREE.Mesh(new THREE.OctahedronGeometry(D(8)), flat(METAL)); dm.scale.set(0.35, 1, 0.8); dm.position.copy(L(sd * (R - 1), -6, -4)); g.add(ol(dm, 0.004)); }
      g.userData = { top, bot };
      return g;
    }
    function hook() {
      const g = new THREE.Group();
      g.add(ol(new THREE.Mesh(new THREE.CylinderGeometry(D(26), D(26), D(7), 36), flat(C.silver)), 0.008));
      const cuff = new THREE.Mesh(new THREE.CylinderGeometry(D(20), D(22), D(14), 30), flat(C.cuff)); cuff.position.y = D(-10); g.add(ol(cuff, 0.008));
      const pts = [[0, 0, 0], [0, 26, 0]]; for (let k = 0; k <= 8; k++) { const a = -Math.PI / 2 + k / 8 * Math.PI * 1.25; pts.push([0, 26 + 18 * Math.cos(a), 18 + 18 * Math.sin(a)]); }
      const c = new THREE.CatmullRomCurve3(pts.map(p => L(...p))), NS = 60, NA = 14, pos = [], idx = [];
      for (let i = 0; i <= NS; i++) { const u = i / NS, p = c.getPointAt(u), t = c.getTangentAt(u), n = new THREE.Vector3().crossVectors(t, new THREE.Vector3(1, 0, 0)).normalize(), b = new THREE.Vector3().crossVectors(n, t).normalize(), r = D(u < 0.8 ? 5.5 : 5.5 * (1 - (u - 0.8) / 0.2) + 0.6);
        for (let j = 0; j <= NA; j++) { const a = j / NA * Math.PI * 2; pos.push(p.x + (n.x * Math.cos(a) + b.x * Math.sin(a)) * r, p.y + (n.y * Math.cos(a) + b.y * Math.sin(a)) * r, p.z + (n.z * Math.cos(a) + b.z * Math.sin(a)) * r); } }
      for (let i = 0; i < NS; i++) for (let j = 0; j < NA; j++) { const a = i * (NA + 1) + j, b = a + 1, cc = a + NA + 1, d = cc + 1; idx.push(a, cc, b, b, cc, d); }
      const hg = new THREE.BufferGeometry(); hg.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3)); hg.setIndex(idx); hg.computeVertexNormals();
      g.add(ol(new THREE.Mesh(hg, flat(C.silver)), 0.007));
      { const tip = c.getPointAt(1), dir = c.getTangentAt(1); const fl = new THREE.Mesh(new THREE.ConeGeometry(D(7), D(14), 4), flat(C.silver)); fl.scale.z = 0.45;   // farpa da ponta
        fl.position.copy(tip).addScaledVector(dir, D(4)); fl.quaternion.setFromUnitVectors(new THREE.Vector3(0, 1, 0), dir); g.add(ol(fl, 0.005)); }
      { const rg = new THREE.Mesh(new THREE.TorusGeometry(D(7.5), D(2.6), 8, 18), flat(C.silver)); rg.rotation.x = Math.PI / 2; rg.position.copy(L(0, 6, 0)); g.add(ol(rg, 0.004)); }
      // corrente: elos alternados (um deitado, outro em pé) numa curva que cai do punho e volta ao pé do gancho
      { const cc = new THREE.CatmullRomCurve3([L(-18, -10, -4), L(-26, -4, -34), L(-14, 6, -60), L(6, 10, -54), L(10, 8, -24), L(2, 6, -6)]);
        const N = 13, lm = flat('#9aa1a4'), links = [];
        for (let k = 0; k < N; k++) { const u = (k + 0.5) / N, p = cc.getPointAt(u), t = cc.getTangentAt(u);
          const lk = new THREE.Mesh(new THREE.TorusGeometry(D(4.6), D(1.5), 6, 14), lm); lk.scale.set(1, 1.55, 1);
          const q = new THREE.Quaternion().setFromUnitVectors(new THREE.Vector3(0, 1, 0), t); lk.quaternion.copy(q).multiply(new THREE.Quaternion().setFromAxisAngle(new THREE.Vector3(0, 1, 0), k % 2 ? Math.PI / 2 : 0));
          lk.position.copy(p); const lg = ol(lk, 0.003); g.add(lg); links.push(lg); }
        // a corrente de verdade: pontos com peso (verlet) presos no punho e no pé do gancho; os elos seguem os pontos
        g.userData.chain = { links, A: cc.getPointAt(0), B: cc.getPointAt(1), local: Array.from({ length: N + 1 }, (_, i) => cc.getPointAt(i / N)), seg: cc.getLength() / N }; }
      return g;
    }
    P.hands = { [-1]: claw(), [1]: hook() }; root.add(P.hands[-1], P.hands[1]);
    const hands = { [-1]: new THREE.Object3D(), [1]: new THREE.Object3D() }; root.add(hands[-1], hands[1]);
    const UPV = new THREE.Vector3(0, 1, 0);

    const setFace = f => {
      faceMat.map.dispose(); faceMat.map = B.faceTexture(drawFace(f)); faceMat.needsUpdate = true; paintCrab(f); koU.value = f === 'ko' ? 1 : 0;
      const LID = { normal: [-0.75, 0, 0.75], angry: [0.12, -0.35, 0.75], pain: [0.18, 0, -0.18], ko: [-0.3, 0, 0.5] }[f] || [-0.75, 0, 0.75];
      P.lid.rotation.set(LID[0], 0, LID[1]); P.lidLow.rotation.x = LID[2];
      P.pupil.material.map = f === 'ko' ? eyeTex.ko : f === 'pain' ? eyeTex.pain : eyeTex.on; P.pupil.material.needsUpdate = true;
    };
    // corrente do gancho: verlet em coordenadas do mundo, pontas presas no gancho, gravidade, comprimento fixo por elo
    const cq = new THREE.Quaternion(), UPL = new THREE.Vector3(0, 1, 0), TW = new THREE.Quaternion().setFromAxisAngle(new THREE.Vector3(0, 1, 0), Math.PI / 2);
    function chainStep(c) {
      const G = P.hands[1], ch = G.userData.chain; if (!ch) return;
      const dt = Math.min(0.05, Math.max(c.dt || 1 / 60, 1e-3)); G.updateWorldMatrix(true, false);
      if (!ch.w) { ch.w = ch.local.map(q => G.localToWorld(q.clone())); ch.o = ch.w.map(q => q.clone()); }
      const w = ch.w, o = ch.o, n = w.length - 1, segW = ch.seg * G.getWorldScale(new THREE.Vector3()).x;
      for (let i = 1; i < n; i++) { const t = w[i].clone(); w[i].addScaledVector(t.clone().sub(o[i]), 0.97); w[i].y -= 9.8 * dt * dt; o[i].copy(t); }
      w[0].copy(G.localToWorld(ch.A.clone())); w[n].copy(G.localToWorld(ch.B.clone()));
      for (let it = 0; it < 12; it++) for (let i = 0; i < n; i++) { const dv = w[i + 1].clone().sub(w[i]), l = dv.length() || 1e-6, k = (l - segW) / l;
        if (i === 0) w[1].addScaledVector(dv, -k); else if (i + 1 === n) w[i].addScaledVector(dv, k); else { w[i].addScaledVector(dv, k / 2); w[i + 1].addScaledVector(dv, -k / 2); } }
      G.getWorldQuaternion(cq).invert();
      ch.links.forEach((lg, k) => { const a = w[k], b = w[k + 1], mid = a.clone().add(b).multiplyScalar(0.5), t = b.clone().sub(a).normalize().applyQuaternion(cq);
        lg.position.copy(G.worldToLocal(mid)); lg.quaternion.setFromUnitVectors(UPL, t); if (k % 2) lg.quaternion.multiply(TW); });
    }
    const hp = new THREE.Vector3(), hPrev = new THREE.Vector3(), hVel = new THREE.Vector3(), hAcc = new THREE.Vector3(), lag = new THREE.Vector3(), lagV = new THREE.Vector3(), hq = new THREE.Quaternion();
    let hInit = false;
    // ================= especiais (os do jogo, engine.cpp, com os efeitos do laboratório, vfx/lab_joana.cpp). A garra é a mão
    // de P.hands[-1] e o gancho a de P.hands[1]. O relógio dos efeitos é o do quadro; "after" agenda os momentos de impacto
    const CLAW = -1, HOOK = 1, easeO = k => 1 - (1 - k) ** 3;
    const fxRoot = new THREE.Group(); root.add(fxRoot);
    const glow = (col, o = 1) => new THREE.MeshBasicMaterial({ color: col, transparent: true, opacity: o, depthWrite: false, side: THREE.DoubleSide });
    const queue = []; let now = 0; const after = (dt, fn) => queue.push({ at: now + dt, fn });
    const FX = { rum: -99, hook: -99, coral: -99, shell: -99, bounty: -99, sign: -99, wave: -99, crater: -99, open: null, clawS: 1, tchac: -99 };
    const pos = sx => P.hands[sx].position.clone();
    // faíscas e gotas: bolinhas com velocidade, gravidade e vida
    const sparkG = new THREE.SphereGeometry(1, 8, 6), sparks = Array.from({ length: 170 }, () => { const m = new THREE.Mesh(sparkG, glow('#fff')); m.visible = false; fxRoot.add(m); return { m, life: 0, v: new THREE.Vector3() }; });
    let sparkI = 0; const spark = (p, v, r, col, life, g = 3) => { const q = sparks[sparkI++ % sparks.length]; q.m.position.copy(p); q.v.copy(v); q.r = r; q.m.scale.setScalar(r); q.m.material.color.set(col); q.life = q.life0 = life; q.g = g; q.m.visible = true; };
    const burst = (p, n, spd, r, cols, life, g = 3) => { for (let i = 0; i < n; i++) { const a = Math.random() * 6.283, e = Math.random() * 6.283, sp = spd * (0.5 + Math.random()); spark(p, new THREE.Vector3(Math.cos(a) * sp, Math.sin(a) * sp * 0.8 + 0.4, Math.sin(e) * sp * 0.6 + 0.4), r * (0.6 + Math.random() * 0.7), cols[i % cols.length], life * (0.7 + Math.random() * 0.6), g); } };
    // anéis de choque: de frente (flat = false) ou deitados no chão (flat = true)
    const ringG = new THREE.RingGeometry(0.84, 1, 40), rings = Array.from({ length: 16 }, () => { const m = new THREE.Mesh(ringG, glow('#fff')); m.visible = false; fxRoot.add(m); return { m, life: 0 }; });
    let ringI = 0; const ring = (p, R, col, life, flatR = false, delay = 0) => after(delay, () => { const q = rings[ringI++ % rings.length]; q.m.position.copy(p); q.R = R; q.life = q.life0 = life; q.m.material.color.set(col); q.m.rotation.set(flatR ? -Math.PI / 2 : 0, 0, 0); q.m.visible = true; });
    // o X do corte da tesoura
    const crossBars = Array.from({ length: 4 }, () => { const m = new THREE.Mesh(new THREE.PlaneGeometry(1, 0.1), glow('#ffe600')); m.visible = false; fxRoot.add(m); return m; }); let crossT = -99, crossP = new THREE.Vector3();
    // palavras da assinatura (as do laboratório): GLUB enquanto bebe, HIC! depois
    const word = (txt, col) => { const c = document.createElement('canvas'); c.width = 400; c.height = 170; const g = c.getContext('2d'); g.font = '700 120px Bangers, Impact, sans-serif'; g.textAlign = 'center'; g.textBaseline = 'middle'; g.lineJoin = 'round'; g.lineWidth = 22; g.strokeStyle = '#0f1c10'; g.strokeText(txt, 200, 90); g.fillStyle = col; g.fillText(txt, 200, 90);
      const t = new THREE.CanvasTexture(c); t.colorSpace = THREE.SRGBColorSpace; const m = new THREE.Mesh(new THREE.PlaneGeometry(0.5, 0.21), new THREE.MeshBasicMaterial({ map: t, transparent: true, depthWrite: false })); m.visible = false; fxRoot.add(m); return m; };
    const glub = word('GLUB', '#a3e635'), hic = word('HIC!', '#fde047');
    // garrafa de rum (sprite: vidro verde, rolha, rótulo RUM, líquido âmbar que desce conforme ela bebe); origem no meio
    const bottle = new THREE.Group(); bottle.visible = false; fxRoot.add(bottle);
    { const prof = [[0, -0.15], [0.058, -0.15], [0.062, -0.13], [0.062, 0.04], [0.024, 0.09], [0.022, 0.15], [0, 0.15]].map(([x, y]) => new THREE.Vector2(x, y));
      const glass = flat('#166534'); glass.transparent = true; glass.opacity = 0.62; bottle.add(withOutline(new THREE.Mesh(new THREE.LatheGeometry(prof, 20), glass), 0.005));
      const cork = new THREE.Mesh(new THREE.CylinderGeometry(0.026, 0.024, 0.035, 12), flat('#92400e')); cork.position.y = 0.165; bottle.add(cork);
      const c = document.createElement('canvas'); c.width = 256; c.height = 128; const g = c.getContext('2d'); g.fillStyle = '#f3efe0'; g.fillRect(0, 0, 256, 128); g.font = '700 86px Bangers, Impact, sans-serif'; g.textAlign = 'center'; g.textBaseline = 'middle'; g.fillStyle = '#7f1d1d'; g.fillText('RUM', 128, 68);
      const t = new THREE.CanvasTexture(c); t.colorSpace = THREE.SRGBColorSpace; const lab = new THREE.Mesh(new THREE.CylinderGeometry(0.0635, 0.0635, 0.075, 20, 1, true), new THREE.MeshBasicMaterial({ map: t })); lab.position.y = -0.045; lab.rotation.y = -Math.PI / 2; bottle.add(lab);
      const liq = new THREE.CylinderGeometry(0.052, 0.052, 0.18, 16); liq.translate(0, 0.09, 0); P.rum = new THREE.Mesh(liq, flat('#b45309')); P.rum.position.y = -0.14; bottle.add(P.rum); }
    // Gancho: corrente em linha do punho até o gancho que voa (elos alternados)
    const flyLinks = Array.from({ length: 26 }, (_, i) => { const m = new THREE.Mesh(new THREE.TorusGeometry(D(4.6), D(1.5), 6, 12), flat('#9aa1a4')); m.scale.set(1, 1.55, 1); m.visible = false; fxRoot.add(m); return { m, i }; });
    // Armadura de cracas (sprite joanaFrontBarnacleArmor): ombreiras vulcânicas, placa peitoral em três camadas com as
    // fissuras douradas e a craca gigante de dentes, cracas menores, algas, espinhos na nuca e a placa das costas. As peças
    // se montam uma a uma (Carapaça: 5 s; Carapaça Calcária e Maré: fixa). SU = px por unidade do sprite
    const SU = 2.5, tessA = new TessellateModifier(D(4), 6), armor = []; root.updateMatrixWorld(true);
    const ray = new THREE.Raycaster(), hitZ = (x, y, sign) => { ray.set(new THREE.Vector3(X(x), Y(y), sign * 3), new THREE.Vector3(0, 0, -sign)); const h = ray.intersectObject(body, true).find(q => !q.object.userData.hull); return h ? Math.abs(h.point.z) : 0; };
    let ZF = 0, ZB = 0; for (const dx of [-30, 0, 30]) for (const y of [160, 185, 210]) { ZF = Math.max(ZF, hitZ(CX + dx, y, 1)); ZB = Math.max(ZB, hitZ(CX + dx, y, -1)); }
    const shellZ = (x, Z) => Z * Math.sqrt(Math.max(0.1, 1 - (x / 0.46) ** 2));   // a placa é um casco curvo por cima da roupa
    const plateG = (pts, Z, off, thick) => { const sh = new THREE.Shape(pts.map(([x, y]) => new THREE.Vector2(D(x * SU), D(-y * SU)))); const g = tessA.modify(new THREE.ExtrudeGeometry(sh, { depth: D(thick), bevelEnabled: true, bevelThickness: D(1), bevelSize: D(1), bevelSegments: 1 }));
      const p = g.attributes.position; for (let i = 0; i < p.count; i++) p.setZ(i, shellZ(p.getX(i), Z) + D(off) + p.getZ(i)); g.computeVertexNormals(); return g; };
    const P_OUT = [[-18, -14], [0, -18], [18, -14], [20, 8], [12, 16], [-12, 16], [-20, 8]], P_MID = [[-14, -11], [0, -14], [14, -11], [16, 6], [10, 12], [-10, 12], [-16, 6]], P_IN = [[-10, -8], [0, -10], [10, -8], [12, 4], [8, 9], [-8, 9], [-12, 4]];
    // craca: anel de rocha, miolo laranja e amarelo; teeth = os dentes brancos da gigante. Eixo +Z
    const barn = (rr, teeth) => { const g = new THREE.Group(), rim = new THREE.LatheGeometry([[0.62, 0.62], [1, 0], [1.02, -0.1], [0, -0.1]].map(([x, y]) => new THREE.Vector2(x * rr, y * rr)), 16); rim.rotateX(Math.PI / 2); g.add(withOutline(new THREE.Mesh(rim, flat('#5c4d3c')), 0.004));
      const disc = (k, col, z) => { const m = new THREE.Mesh(new THREE.CircleGeometry(rr * k, 16), flat(col)); m.position.z = z * rr; g.add(m); }; disc(0.68, '#d97706', 0.42); disc(0.42, '#fde047', 0.46);
      if (teeth) for (const s of [-1, 1]) { const t = new THREE.Mesh(new THREE.ConeGeometry(rr * 0.2, rr * 0.34, 3), flat('#ffffff')); t.position.set(0, s * rr * 0.34, 0.5 * rr); t.rotation.z = s > 0 ? Math.PI : 0; g.add(t); } return g; };
    const addArmor = (g, d) => { g.visible = false; body.add(g); armor.push({ g, d }); return g; };
    const chest = (Z, turn, d) => { const g = new THREE.Group(); g.position.set(X(CX), Y(186), 0); g.rotation.y = turn;
      [[P_OUT, 0, 3.5, '#4a3728'], [P_MID, 4, 2, '#8c7355'], [P_IN, 6.5, 2, '#ba9e7b']].forEach(([pts, off, th, col], i) => { const m = new THREE.Mesh(plateG(pts, Z, off, th), flat(col)); g.add(i ? m : withOutline(m, 0.007)); });
      for (const [[x0, y0], [x1, y1]] of [[[0, -14], [0, 10]], [[-12, -8], [8, 10]], [[12, -8], [-8, 10]]]) { const pts = Array.from({ length: 6 }, (_, k) => { const u = k / 5, x = D((x0 + (x1 - x0) * u) * SU); return new THREE.Vector3(x, D(-(y0 + (y1 - y0) * u) * SU), shellZ(x, Z) + D(10)); }); g.add(new THREE.Mesh(new THREE.TubeGeometry(new THREE.CatmullRomCurve3(pts), 10, D(1.3), 5), new THREE.MeshBasicMaterial({ color: '#fde047' }))); }
      const big = barn(D(9 * SU), true); big.position.set(0, 0, shellZ(0, Z) + D(11)); g.add(big);
      for (const s of [-1, 1]) { const b = barn(D(4 * SU), false); const x = D(s * 10 * SU); b.position.set(x, D(-8 * SU), shellZ(x, Z) + D(9)); g.add(b); }
      return addArmor(g, d); };
    chest(ZF, 0, 0.05); chest(ZB, Math.PI, 0.05);
    for (const s of [-1, 1]) { const g = new THREE.Group(); g.position.copy(V(CX + s * 62, 150, 10)); g.scale.setScalar(1.25);   // ombreira: rocha facetada, camada clara e a craca no alto, mais uma menor do lado
      const rock = new THREE.IcosahedronGeometry(1, 1), o = new THREE.Mesh(rock, flat('#4a3728')); o.material.flatShading = true; o.scale.set(D(32), D(24), D(30)); g.add(ol(o, 0.007));
      const inn = new THREE.Mesh(rock, flat('#8c7355')); inn.material.flatShading = true; inn.scale.set(D(25), D(18), D(23)); inn.position.set(0, D(9), D(5)); g.add(inn);
      const b1 = barn(D(12), false); b1.position.set(0, D(23), D(8)); b1.rotation.x = -1.0; g.add(b1); const b2 = barn(D(7), false); b2.position.set(s * D(23), D(4), D(18)); b2.rotation.y = s * 0.7; g.add(b2); addArmor(g, 0.3).userData.k = 1.25; armor[armor.length - 1].k = 1.25; }
    { const g = new THREE.Group(); for (let k = -2; k <= 2; k++) { const c = new THREE.Mesh(new THREE.ConeGeometry(D(7), D(20), 5), flat('#ba9e7b')); c.position.copy(V(CX + k * 15, 142 + Math.abs(k) * 2, shellZ(0, ZF) * 100 / 100 * S_PX * 0.8)); c.rotation.z = -k * 0.2; g.add(ol(c, 0.004)); } addArmor(g, 0.45); }   // espinhos da gola
    for (const s of [-1, 1]) { const g = new THREE.Group(); const x = D(s * 15 * SU), z = shellZ(x, ZF) + D(4), top = Y(186 + 6 * SU), pts = [[x, top], [x + D(s * 8), top - D(14)], [x + D(s * 3), top - D(26)], [x, top - D(38)]].map(([a, b]) => new THREE.Vector3(X(CX) + a, b, z)); g.add(withOutline(new THREE.Mesh(new THREE.TubeGeometry(new THREE.CatmullRomCurve3(pts), 12, D(2.6), 6), flat('#2d6a4f')), 0.003)); addArmor(g, 0.55); }   // algas
    let barnGrow = 0, calcified = false, deep = false, mut = 0;
    // Coral: galhos rosa em tubos que brotam do chão na frente dela, com pontas claras
    const coralG = new THREE.Group(); coralG.position.set(0.05, 0, 1.15); coralG.visible = false; fxRoot.add(coralG);
    { const branch = (o, d, len, r, depth) => { const e = o.clone().addScaledVector(d, len), mid = o.clone().lerp(e, 0.5).add(new THREE.Vector3(d.z * 0.05, 0, d.x * 0.05)); coralG.add(withOutline(new THREE.Mesh(new THREE.TubeGeometry(new THREE.CatmullRomCurve3([o, mid, e]), 8, r, 8), flat(depth > 1 ? '#f72585' : '#ff758f')), 0.006));
        if (!depth) { const t = new THREE.Mesh(new THREE.SphereGeometry(r * 1.5, 10, 8), flat('#ffd6e0')); t.position.copy(e); coralG.add(t); return; }
        [-1, 1, depth === 3 ? 0 : null].forEach(k => { if (k === null) return; const a = k * 0.55, nd = d.clone().applyAxisAngle(new THREE.Vector3(0, 0, 1), a).applyAxisAngle(new THREE.Vector3(0, 1, 0), k * 0.9 + depth).normalize(); branch(e, nd, len * 0.72, r * 0.68, depth - 1); }); };
      branch(new THREE.Vector3(0, -0.02, 0), new THREE.Vector3(0, 1, 0), 0.3, D(8.5), 3); }
    // Âncora: cratera no chão e três ondas (perfil do sprite joanaTsunamiWaveFront) correndo para a frente
    const waveShape = pts => { const sh = new THREE.Shape(), v = pts.map(([x, y]) => new THREE.Vector2((x - 70) / 100, (98 - y) / 100)); sh.moveTo(v[0].x, v[0].y); sh.splineThru(v.slice(1)); sh.closePath(); return sh; };
    const wave = (pts, col, w) => { const g = new THREE.ExtrudeGeometry(waveShape(pts), { depth: w, bevelEnabled: false, curveSegments: 32 }); g.rotateY(-Math.PI / 2); g.translate(w / 2, 0, 0); return withOutline(new THREE.Mesh(g, flat(col)), 0.008); };
    const P1 = [[8, 98], [14, 66], [28, 34], [64, 16], [90, 8], [118, 18], [132, 38], [120, 40], [106, 32], [92, 34], [76, 36], [60, 48], [48, 68], [40, 80], [32, 98]];
    const P2 = [[28, 92], [34, 68], [50, 44], [80, 28], [98, 20], [115, 28], [122, 42], [112, 43], [100, 38], [88, 42], [76, 48], [64, 62], [54, 82]];
    const waves = [0, 1, 2].map(i => { const g = new THREE.Group(); g.add(wave(P1, '#2a7f9e', 1.5)); const c = wave(P2, '#a8dadc', 1.56); c.position.z = 0.03; g.add(c); g.visible = false; fxRoot.add(g); return { g, d: i * 0.16, k: 1 + i * 0.15 }; });
    const crater = new THREE.Group(); { const a = new THREE.Mesh(new THREE.CircleGeometry(0.34, 24), glow('#1b1310', 0.75)), b = new THREE.Mesh(new THREE.RingGeometry(0.3, 0.42, 24), glow('#7a5a3a', 0.9)); crater.add(a, b); crater.rotation.x = -Math.PI / 2; crater.position.set(0, 0.012, 0.42); crater.visible = false; fxRoot.add(crater); }
    // Anêmona: duas colônias nos ombros (pé roxo, disco rosa, tentáculos ondulando com ponta azul brilhante)
    const anemones = [-1, 1].map(s => { const g = new THREE.Group(); g.position.copy(V(CX + s * 50, 138, 4)); g.visible = false; upper.add(g); g.position.y -= WAIST; const tents = [];
      const base = new THREE.Mesh(new THREE.CylinderGeometry(D(15), D(19), D(16), 16), flat('#7209b7')); base.position.y = D(8); g.add(withOutline(base, 0)); base.position.y = D(8);
      const disc = new THREE.Mesh(new THREE.CylinderGeometry(D(19), D(15), D(5), 18), flat('#f72585')); disc.position.y = D(18); g.add(disc);
      for (let i = 0; i < 9; i++) { const a = i / 9 * 6.283, h = new THREE.Group(); h.position.set(Math.cos(a) * D(12), D(20), Math.sin(a) * D(12)); h.rotation.set(Math.sin(a) * 0.4, 0, -Math.cos(a) * 0.4); const curve = new THREE.CatmullRomCurve3([new THREE.Vector3(), new THREE.Vector3(0, D(16), 0), new THREE.Vector3(Math.cos(a) * D(5), D(32), Math.sin(a) * D(5))]);
        h.add(withOutline(new THREE.Mesh(new THREE.TubeGeometry(curve, 8, D(3.2), 6), flat(i % 2 ? '#560bad' : '#7209b7')), 0.003)); const tip = new THREE.Mesh(new THREE.SphereGeometry(D(5), 10, 8), new THREE.MeshBasicMaterial({ color: '#4cc9f0' })); tip.position.copy(curve.getPoint(1)); h.add(tip); g.add(h); tents.push({ h, a, x: h.rotation.x, z: h.rotation.z }); }
      return { g, tents }; });
    // Formas: tinta quente da Maré (como o filtro do laboratório)
    const tintList = []; for (const grp of [body, low, head]) grp.traverse(o => { if (o.isMesh && o.material.color && o.material.isMeshToonMaterial) tintList.push({ m: o.material, c: o.material.color.clone() }); });
    const deepTint = new THREE.Color('#ffb98a');
    const paintDeep = k => { for (const it of tintList) it.m.color.copy(it.c).lerp(it.c.clone().multiply(deepTint), k); };
    const calcB = document.querySelector('#calcified'), deepB = document.querySelector('#deep');
    if (calcB) calcB.onclick = () => { calcified = !calcified; calcB.style.outline = calcified ? '2px solid #fc6' : ''; };
    if (deepB) deepB.onclick = () => { deep = !deep; deepB.style.outline = deep ? '2px solid #fc6' : ''; if (deep) { const p = V(CX, 190, 0); ring(p, 1.3, '#ff8533', 0.7); burst(p, 36, 2.6, 0.022, ['#ff8533', '#ea580c', '#fde047'], 0.9, 4); } };
    const both = (a, b) => ({ [CLAW]: a, [HOOK]: b });
    const setPose = (pose, v) => { pose[-1] = v[-1]; pose[1] = v[1]; };
    const skills = {
      // assinatura (laboratório): a garra entra no colete, tira a garrafa, vira até o fim, HIC!, e joga por cima do ombro
      signature: { dur: 3.2, start: () => { FX.sign = now; FX.tossed = false; },
        pose: k => { const p = k / 3.2, raise = ss(0.26, 0.36, p) * (1 - ss(0.66, 0.74, p)), wind = ss(0.74, 0.8, p) * (1 - ss(0.8, 0.88, p)); FX.open = p < 0.18 ? 0.7 * ss(0, 0.12, p) : p < 0.8 ? 0.12 : 0.6 * (1 - ss(0.82, 1, p)); FX.clawS = 1; return { lean: -0.2 * raise - 0.08 * wind, tilt: -0.12 * raise, twist: CLAW * (0.14 * raise - 0.3 * wind) }; } },
      crush: { dur: 0.85, start: () => after(0.2, () => { const p = pos(CLAW); p.z += 0.22; crossT = now; crossP.copy(p); ring(p, 0.72, '#ff8533', 0.42); burst(p, 32, 2.8, 0.018, ['#ff8533', '#fde047', '#ffffff'], 0.55); }),
        pose: (k, pose) => { const b = ss(0, 0.15, k) * (1 - ss(0.15, 0.21, k)), hit = ss(0.15, 0.21, k) * (1 - ss(0.5, 0.85, k)); FX.open = k < 0.17 ? 0.8 * ss(0, 0.14, k) : 0; FX.clawS = 1 + 0.25 * hit; pose[CLAW] = [-0.2 * b + 0.62 * easeO(hit), 0.04 * hit, 0.45 * hit, 0, 0]; return { twist: CLAW * (0.3 * hit - 0.14 * b), lean: 0.16 * hit }; } },
      // gancho: voa na corrente até o alvo, gira, volta e encaixa no punho com faíscas
      hook: { dur: 1.25, start: () => { FX.hook = now; after(1.25 * 0.88, () => { const p = pos(HOOK); ring(p, 0.45, '#f4be41', 0.3); burst(p, 20, 2.2, 0.012, ['#f4be41', '#ffffff'], 0.45); }); },
        pose: (k, pose) => { const p = k / 1.25, e = ss(0, 0.25, p) * (1 - ss(0.88, 1, p)); pose[HOOK] = [0.22 * e, 0.06 * e, 0.3 * e, 0, 0]; return { twist: HOOK * 0.24 * e, lean: 0.08 * e }; } },
      shell: { dur: 0.8, start: () => { FX.shell = now; after(0.28, () => { const p = V(CX, 190, 0); ring(p, 0.95, '#a8dadc', 0.6); burst(p, 22, 2, 0.014, ['#f4be41', '#ba9e7b', '#a8dadc'], 0.7); burst(p, 24, 1.1, 0.03, ['#cfefff', '#ffffff'], 0.8, -1); }); },
        pose: (k, pose) => { const e = ss(0, 0.28, k) * (1 - ss(0.55, 0.8, k)); setPose(pose, both([0.02 * e, 0.06 * e, 1.15 * e, 0, 0], [0.02 * e, -0.02 * e, 1.15 * e, 0, 0])); return { drop: 0.04 * e, lean: 0.12 * e }; } },
      // coral: gancho de baixo com a garra e o coral rompe o chão embaixo do rival
      coral: { dur: 1.3, start: () => { FX.coral = now; const o = new THREE.Vector3(0.05, 0.01, 1.15); after(0.4, () => { [0, 0.08, 0.16].forEach((d, i) => ring(o, 0.34 + i * 0.13, '#a8dadc', 0.6, true, d)); burst(o.clone().setY(0.1), 18, 1.5, 0.016, ['#a8dadc', '#ffffff', '#457b9d', '#f72585', '#ff758f', '#2a9d8f'], 0.8, 5); }); },
        pose: (k, pose) => { const p = k / 1.3, low = ss(0, 0.25, p) * (1 - ss(0.25, 0.42, p)), up = ss(0.25, 0.42, p) * (1 - ss(0.75, 1, p)); pose[CLAW] = [0.28 * up, -0.28 * low + 0.34 * up, 0.25 * up, 0, 0]; return { twist: CLAW * (0.22 * up - 0.22 * low), lean: -0.12 * up + 0.08 * low, drop: 0.07 * low }; } },
      // âncora: ergue o gancho e crava no chão; dali sai a onda
      anchor: { dur: 1.2, start: () => { const T = 0.56; after(T, () => { FX.wave = now; FX.crater = now; const o = new THREE.Vector3(0, 0.03, 0.42); ring(o, 0.8, '#ffffff', 0.45, true); burst(o, 16, 1.6, 0.016, ['#ffffff', '#a8dadc'], 0.7, 5); }); },
        pose: (k, pose) => { const up = ss(0, 0.42, k), sl = ss(0.42, 0.56, k), rel = ss(0.75, 1.2, k), e = 1 - rel; pose[HOOK] = [0.12 * e, (0.52 * up - 1.0 * sl) * e, 0.7 * e, 0, 0]; return { lean: (-0.1 * up + 0.34 * sl) * e, drop: 0.09 * sl * e, twist: HOOK * 0.14 * sl * e }; } },
      // anêmona: cura; as colônias brotam nos ombros, a garra rumina e sobem anéis, gotas e esporos
      bounty: { dur: 1.6, start: () => { FX.bounty = now; for (let i = 0; i < 4; i++) ring(new THREE.Vector3(0, 0.015, 0), 1.0, '#4cc9f0', 0.8, true, 0.2 + i * 0.28); for (let i = 0; i < 4; i++) ring(V(CX, 200 - i * 0, 0), 0.55, '#f72585', 0.6, true, 0.32 + i * 0.25); },
        pose: (k, pose) => { const p = k / 1.6, e = ss(0, 0.2, p) * (1 - ss(0.8, 1, p)); FX.open = p > 0.18 && p < 0.78 ? 0.55 * Math.max(0, Math.sin((p - 0.18) / 0.6 * Math.PI * 6)) : null; setPose(pose, both([-0.05 * e, -0.06 * e, -0.22 * e, 0, 0], [-0.05 * e, -0.06 * e, -0.22 * e, 0, 0])); return { lean: -0.1 * e, tilt: 0.03 * Math.sin(k * 3) * e }; } },
    };
    const fxFrame = c => {
      now = c.t; const dt = c.dt, cam = window.VIN?.camera;
      for (let i = queue.length - 1; i >= 0; i--) if (now >= queue[i].at) { queue[i].fn(); queue.splice(i, 1); }
      for (const q of sparks) { if (q.life <= 0) { q.m.visible = false; continue; } q.life -= dt; q.v.y -= q.g * dt; q.m.position.addScaledVector(q.v, dt); q.m.scale.setScalar(q.r * Math.max(0.05, q.life / q.life0)); if (q.m.position.y < 0.01) q.life = 0; }
      for (const q of rings) { if (q.life <= 0) { q.m.visible = false; continue; } q.life -= dt; const u = 1 - q.life / q.life0, e = q.R * easeO(u) + 0.03; q.m.scale.set(e, e, 1); q.m.material.opacity = Math.max(0, 1 - u); if (cam && q.m.rotation.x === 0) q.m.quaternion.copy(cam.quaternion); }
      // X do corte
      { const u = (now - crossT) / 0.4, on = u > 0 && u < 1; crossBars.forEach((b, i) => { b.visible = on; if (!on) return; if (cam) b.quaternion.copy(cam.quaternion); b.rotateZ((i < 2 ? Math.PI / 4 : 0) + (i % 2) * Math.PI / 2); b.position.copy(crossP); const L0 = (i < 2 ? 1.1 : 0.65) * easeO(u); b.scale.set(L0, 1 - u, 1); b.material.opacity = 1 - u; b.material.color.set(i < 2 ? '#ffe600' : '#ffffff'); }); }
      // assinatura: trilha da garra (x, y, z da garrafa; ângulo da garrafa), garrafa e palavras
      { const p = (now - FX.sign) / 3.2, G = P.hands[CLAW], on = c.skill === 'signature' && p >= 0 && p < 1;
        const KEYS = [[0.12, -0.12, 0.98, 0.3, 0, 0.3], [0.26, -0.3, 0.98, 0.62, 0, 0.3], [0.36, -0.21, 1.27, 0.4, 112, 1], [0.66, -0.21, 1.27, 0.4, 112, 1], [0.74, -0.3, 1.0, 0.62, 0, 0.3], [0.8, -0.5, 1.36, 0.2, -50, 0.3]];   // p, x, y, z da garrafa, ângulo, quanto a garra vem de lado
        let i = 0; while (i < KEYS.length - 2 && p > KEYS[i + 1][0]) i++; const a = KEYS[i], b = KEYS[i + 1], u = ss(0, 1, (p - a[0]) / (b[0] - a[0])), gx = a[1] + (b[1] - a[1]) * u, gy = a[2] + (b[2] - a[2]) * u, gz = a[3] + (b[3] - a[3]) * u, ang = (a[4] + (b[4] - a[4]) * u) * Math.PI / 180, side = a[5] + (b[5] - a[5]) * u, aim = new THREE.Vector3(side, 0, 1 - side * 0.85).normalize();
        if (on) { const w = ss(0, 0.12, p) * (1 - ss(0.84, 1, p)); G.position.lerp(new THREE.Vector3(gx, gy, gz).addScaledVector(aim, -0.32), w);   // a garrafa fica entre as pinças: a mão vem atrás dela
          if (w > 0.5) { const zA = new THREE.Vector3(0, 1, 0).addScaledVector(aim, -aim.y).normalize(), xA = new THREE.Vector3().crossVectors(aim, zA); G.quaternion.setFromRotationMatrix(new THREE.Matrix4().makeBasis(xA, aim, zA)); } }
        const held = on && p >= 0.12 && p < 0.8, drink = ss(0.36, 0.66, p), toss = (p - 0.8) / 0.2, flying = p >= 0.8 && toss < 1;
        bottle.visible = held || flying;
        if (held) { bottle.position.set(gx, gy, gz); bottle.rotation.set(0, 0, -ang); bottle.scale.setScalar(p < 0.2 ? 0.9 * ss(0.12, 0.2, p) + 0.1 : 1); P.rum.scale.y = Math.max(0.01, 1 - drink); P.rum.visible = drink < 0.99; }
        if (flying) { const t = toss * 3.2 * 0.2 / 0.64; bottle.position.set(-0.56 - toss * 1.5, 1.36 + 2.1 * t - 3.3 * t * t, 0.08 - toss * 1.3); bottle.rotation.set(toss * 5, 0, 0.87 - toss * 14); bottle.scale.setScalar(Math.max(0.01, 1 - ss(0.75, 1, toss))); }
        if (on && p > 0.38 && p < 0.65 && Math.random() < 0.25) spark(new THREE.Vector3(-0.05, 1.2, 0.42), new THREE.Vector3(0.1 + Math.random() * 0.3, -0.3, 0.3), 0.012, '#d97706', 0.4, 4);
        glub.visible = on && p > 0.38 && p < 0.66 && Math.sin(p * 90) > 0; hic.visible = on && p > 0.68 && p < 0.8;
        for (const m of [glub, hic]) if (m.visible) { if (cam) m.quaternion.copy(cam.quaternion); m.position.set(0.42, 1.62, 0.3); } }
      // gancho voando
      { const u = (now - FX.hook) / 1.25, on = u > 0.3 && u < 0.88; for (const l of flyLinks) l.m.visible = on;
        if (on) { const G = P.hands[HOOK], w = hands[HOOK].position.clone(), tgt = new THREE.Vector3(0.02, 1.0, 1.55), s = (u - 0.44) / 0.22, a = s * Math.PI * 4;
          const pHook = u < 0.44 ? w.clone().lerp(tgt, easeO((u - 0.3) / 0.14)) : u < 0.66 ? tgt.clone().add(new THREE.Vector3(Math.cos(a) * 0.2, Math.sin(a) * 0.13, 0)) : tgt.clone().lerp(w, ((u - 0.66) / 0.22) ** 2);
          G.position.copy(pHook); if (u >= 0.44 && u < 0.66) { G.rotateY(a); if (Math.random() < 0.5) spark(pHook, new THREE.Vector3(-Math.sin(a) * 1.6, Math.cos(a) * 1.3, 0.2), 0.012, Math.random() < 0.65 ? '#cbd2d0' : '#f4be41', 0.4); }
          const d = pHook.clone().sub(w), up = new THREE.Vector3(0, 1, 0), q = new THREE.Quaternion().setFromUnitVectors(up, d.clone().normalize()); flyLinks.forEach(l => { l.m.position.copy(w).addScaledVector(d, (l.i + 0.5) / flyLinks.length); l.m.quaternion.copy(q); if (l.i % 2) l.m.rotateY(Math.PI / 2); l.m.scale.y = Math.min(2.4, d.length() / flyLinks.length / D(6)); }); } }
      // armadura: as peças se montam e somem em ordem
      { const sk = now - FX.shell, on = calcified || deep || (sk > 0 && sk < 5), tgt = on ? 1 : 0; barnGrow += (tgt - barnGrow) * Math.min(1, dt * (on ? 3.6 : 7));
        for (const b of armor) { const a = Math.max(0, Math.min(1, (barnGrow - b.d) / 0.45)); b.g.visible = a > 0.01; if (a > 0.01) { const e = (0.2 + 0.8 * easeO(a)) * (b.k || 1); b.g.scale.set(e, e, b.k || 1); } }
        if (sk > 0.3 && sk < 5 && Math.random() < 0.3) { const a = Math.random() * 6.283; spark(new THREE.Vector3(Math.cos(a) * 0.3, 0.7 + Math.random() * 0.3, Math.sin(a) * 0.3), new THREE.Vector3(0, 0.5 + Math.random() * 0.4, 0), 0.016 + Math.random() * 0.018, '#cfefff', 0.9, -0.1); } }
      // coral
      { const u = (now - FX.coral) / 1.3, on = u > 0.3 && u < 1; coralG.visible = on; if (on) { const gr = u < 0.58 ? Math.min(1, Math.sin((u - 0.3) / 0.28 * Math.PI / 2) * 1.05) : u < 0.8 ? 1 : Math.max(0.01, Math.cos((u - 0.8) / 0.2 * Math.PI / 2)); coralG.scale.set(0.4 + 0.6 * gr, Math.max(0.01, gr), 0.4 + 0.6 * gr); } else if (u > 0.1 && u <= 0.3 && Math.random() < 0.5) spark(new THREE.Vector3(0.05 + (Math.random() - 0.5) * 0.3, 0.02, 1.15 + (Math.random() - 0.5) * 0.25), new THREE.Vector3(0, 0.7 + Math.random() * 0.5, 0), 0.012, Math.random() < 0.5 ? '#a8dadc' : '#ffffff', 0.4, 1); }
      // ondas e cratera
      { const wt = now - FX.wave; for (const w of waves) { const q = (wt - w.d) / 1.0, on = q > 0 && q < 1; w.g.visible = on; if (on) { w.g.position.set(0, 0, 0.3 + q * 2.0); const e = (0.3 + 0.45 * ss(0, 0.45, q)) * w.k; w.g.scale.set(0.5 + 0.7 * q, e * (1 - ss(0.8, 1, q) * 0.9), e); if (Math.random() < 0.5) spark(new THREE.Vector3((Math.random() - 0.5) * 1.2, 0.3 + Math.random() * 0.5 * e, w.g.position.z + 0.2), new THREE.Vector3(0, 0.9, 1.2), 0.014, Math.random() < 0.5 ? '#ffffff' : '#a8dadc', 0.45, 4); } } }
      { const u = (now - FX.crater) / 1.2, on = u > 0 && u < 1; crater.visible = on; if (on) { const a = 1 - ss(0.6, 1, u); crater.children[0].material.opacity = 0.75 * a; crater.children[1].material.opacity = 0.9 * a; } }
      // anêmona
      { const u = (now - FX.bounty) / 1.6, gr = u <= 0 || u >= 0.94 ? 0 : u < 0.2 ? Math.sin(u / 0.2 * Math.PI / 2) : u < 0.72 ? 1 : Math.cos((u - 0.72) / 0.22 * Math.PI / 2);
        for (const a of anemones) { a.g.visible = gr > 0.01; if (gr <= 0.01) continue; a.g.scale.setScalar(gr * 1.5); for (const q of a.tents) { q.h.rotation.x = q.x + Math.sin(now * 4 + q.a) * 0.22; q.h.rotation.z = q.z + Math.cos(now * 3.4 + q.a) * 0.22; } }
        if (u > 0.12 && u < 0.85 && Math.random() < 0.55) { const a = Math.random() * 6.283; spark(new THREE.Vector3(Math.cos(a) * (0.25 + Math.random() * 0.25), 0.25 + Math.random() * 0.6, Math.sin(a) * 0.25), new THREE.Vector3(Math.cos(a) * 0.3, 0.8 + Math.random() * 0.8, 0), 0.016 + Math.random() * 0.012, Math.random() < 0.5 ? '#4cc9f0' : '#f72585', 0.8, 0); } }
      // maré: entra aos poucos (tinta quente, 14% maior, mãos maiores)
      { const m0 = mut; mut += ((deep ? 1 : 0) - mut) * Math.min(1, dt * 5); if (Math.abs(mut - m0) > 1e-4) paintDeep(mut); }
    };
    const onFrame = c => {
      { const dt = Math.min(0.05, Math.max(c.dt || 1 / 60, 1e-3));
        head.updateWorldMatrix(true, false); head.getWorldPosition(hp); if (!hInit) { hPrev.copy(hp); hInit = true; }
        const v = hp.clone().sub(hPrev).divideScalar(dt); hAcc.copy(v).sub(hVel).divideScalar(dt); hVel.copy(v); hPrev.copy(hp);
        lagV.addScaledVector(lag, -110 * dt).addScaledVector(lagV, -8 * dt).addScaledVector(hAcc, -0.9 * dt); lag.addScaledVector(lagV, dt); if (lag.length() > 0.08) lag.setLength(0.08);
        head.getWorldQuaternion(hq).invert(); const Lg = lag.clone().applyQuaternion(hq), Ah = hAcc.clone().applyQuaternion(hq);
        // cada dread com a sua mola (rigidez um pouco diferente: não balançam em bloco): ficam para trás com a aceleração
        // da cabeça, abrem para fora quando ela cai no salto e batem de volta na aterrissagem; cada uma passa um pouco
        // do movimento para a vizinha
        const A = Ah.clampLength(0, 40);
        P.dreads.forEach((d, i) => { const st = d.userData.st || (d.userData.st = { ax: 0, az: 0, f: 0, vx: 0, vz: 0, vf: 0, k: 62 + (i % 4) * 13, r: new THREE.Vector2(d.position.x, d.position.z).normalize() });
          const nb = P.dreads[(i + 1) % P.dreads.length].userData.st, kk = st.k;
          st.vx += (-kk * st.ax - 5 * st.vx + 2.2 * A.z + (nb ? 8 * (nb.ax - st.ax) : 0)) * dt;
          st.vz += (-kk * st.az - 5 * st.vz - 2.2 * A.x + (nb ? 8 * (nb.az - st.az) : 0)) * dt;
          st.vf += (-kk * st.f - 6 * st.vf - 1.6 * A.y) * dt;
          st.ax = Math.max(-0.6, Math.min(0.6, st.ax + st.vx * dt)); st.az = Math.max(-0.6, Math.min(0.6, st.az + st.vz * dt)); st.f = Math.max(-0.25, Math.min(0.55, st.f + st.vf * dt));
          const w = 0.02 * Math.sin(c.t * 1.7 + i * 0.9);
          // a ponta anda (rotation.z, -rotation.x) no chão; o que iria para dentro do corpo (contra o raio) é cortado: a mecha
          // balança de lado e para fora, nunca entra no corpo; um afastamento mínimo deixa sempre uma folga
          let dx = st.az + st.f * st.r.x + w * 0.6, dz = -(st.ax - st.f * st.r.y + w);
          const rin = dx * st.r.x + dz * st.r.y, lim = 0.03;
          if (rin < lim) { dx += (lim - rin) * st.r.x; dz += (lim - rin) * st.r.y; }
          d.rotation.set(-dz, 0, dx); });
      }
      queueMicrotask(() => chainStep(c));
      for (const sx of [-1, 1]) {
        const G = P.hands[sx], k = Math.min(1, Math.max(0, c.pose[sx][0] / 0.3));
        G.position.copy(hands[sx].position);
        const yq = new THREE.Quaternion().setFromAxisAngle(UPV, upper.rotation.y * 0.8);
        const dir = new THREE.Vector3(sx * 0.15, 0.2, 1).lerp(new THREE.Vector3(0, -0.05, 1), k).normalize().applyQuaternion(yq);
        const zRef = UPV.clone().applyQuaternion(yq), zA = zRef.addScaledVector(dir, -zRef.dot(dir)).normalize(), xA = new THREE.Vector3().crossVectors(dir, zA);
        G.quaternion.setFromRotationMatrix(new THREE.Matrix4().makeBasis(xA, dir, zA));
        if (G.userData.top) {   // garra: abre na guarda, fecha com força no fim do golpe
          const open = 0.35 * (1 - ss(0.55, 0.9, k)) + 0.15 * Math.sin(c.t * 2.4) ** 2;
          const op = c.skill && FX.open !== null ? FX.open : open;
          G.userData.top.rotation.x = 0.05 + op * 0.8; G.userData.bot.rotation.x = -0.05 - op * 0.5;
        }
        G.scale.setScalar((sx === CLAW && c.skill ? FX.clawS : 1) * (1 + 0.15 * mut));
      }
      if (!c.skill) { FX.open = null; FX.clawS = 1; }
      fxFrame(c);
    };
    P.cloth = [{ geos: P.clothGeos, top: Y(165), bot: Y(222), pow: 1.5, gain: 0.7 }];   // tools3d/export
    return { P, upper, WAIST, neck, NECK, hips, HIPS, hands, hipSway: 0.6, hand: { x: 0.5, y: Y(165), z: 0.42, yaw: -0.12 }, placeFoot, setFace, skills, onFrame, get scale() { return 1 + 0.14 * mut; } };
  },
};
