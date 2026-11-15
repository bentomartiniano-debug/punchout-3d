// Dr. Marcov no modo vinil, no corpo da Valéria (base.js) e pela arte atual dele (retrato HD skin0 e o boneco do
// jogo): o macaco cientista, com cara de chimpanzé. Cabeça de pelo marrom com a máscara clara dos olhos às bochechas,
// sobrancelhas grossas em relevo, olhos grandes de íris castanha, nariz em bolota, focinho claro saltado com a boca,
// cabelo espetado, orelhas viradas para a frente com o miolo mais escuro e as
// costeletas pontudas; jaleco branco aberto que fecha por cima dos ombros (gola, lapelas, bolso com canetas, bolsos de
// baixo, dobras com risco e forro na sombra), camisa azul-petróleo com a tela do monitor cardíaco, pernas de pelo, botas
// azul-marinho, o rabo. Sem braços (como no jogo): as luvas marrons com as pontas dos dedos claras e o punho com a
// luzinha flutuam. Medidas em px do quadro; S px = 1 m.
import { THREE, ss, rrect, INK, INKS, flat, withOutline, legPlacer } from './core.js';
import { RoundedBoxGeometry } from 'three/addons/geometries/RoundedBoxGeometry.js';
import { mergeVertices } from 'three/addons/utils/BufferGeometryUtils.js';
import { TessellateModifier } from 'three/addons/modifiers/TessellateModifier.js';
import { ConvexGeometry } from 'three/addons/geometries/ConvexGeometry.js';
import { VALERIA, bodyOpts, valeriaBody } from './base.js';

const S_PX = 245, FOOT = 457, CX = 240;
const C = { fur: '#7b4b2f', neck: '#74462a', skin: '#cf9f6f', skinDark: '#a8764c', muzzle: '#b98a5e', nose: '#a86e4a', line: '#8b5a3c', earIn: '#8b5a3c', hair: '#5e3824', hairHi: '#8b5a3c', brow: '#6a3a20',
            coat: '#eef0e6', coatShade: '#c4ccc4', lining: '#d4dad2', shirt: '#1e3a4e', shirtDark: '#142a3a', screen: '#3ec6c8', pants: '#3a4250', pantsDark: '#2a303b', belt: '#2a1c14', shoe: '#2a4a5e', shoeHi: '#3d6680',
            sole: '#14262c', led: '#7ee8e0', red: '#c8443a', glove: '#7a4628', tip: '#c08a58', cuff: '#24384a', tail: '#7a4628' };
const col = h => ({ value: new THREE.Color(h) });

export default {
  title: 'Dr. Marcov',
  arena: 'marcov',   // abre na arena própria (core.js)
  units: [S_PX, FOOT, CX],
  faces: [['normal', 'Normal'], ['angry', 'Raiva'], ['pain', 'Dor'], ['ko', 'K.O.']],
  skills: [['signature', 'Assinatura'], ['rocket', 'Propulsão Foguete'], ['laser', 'Raio Orbital Sputnik'], ['plasma', 'Escudo Sputnik'], ['kinetic', 'Impulso de Reação'], ['exhaust', 'Fumaça de Exaustão'], ['o2', 'Célula de Oxigênio']],
  hudExtra: 'Forma: <button id="zero">Gravidade Zero</button>',
  others: [['valeria', 'Valéria'], ['frog', 'King Frog'], ['joana', 'Joana']],
  async build({ root, U }) {
    const { X, Y, D, loft } = U, P = {};
    // corpo: o da Valéria (base.js) maior e mais largo: cabeção de ovo largo, pescoço que abre nos ombros, tronco da
    // camisa (a calça embaixo), pernas da calça e o sapato da Valéria escuro
    // rosto anguloso: da linha dos olhos desce reto até a maçã do rosto, quebra no canto do maxilar e fecha num queixo largo e reto
    const JAW = [[149, 86], [170, 86], [193, 73], [207, 50], [214, 44]];
    const jawA = y => { if (y <= JAW[0][0]) return JAW[0][1]; for (let i = 1; i < JAW.length; i++) if (y <= JAW[i][0]) { const [y0, a0] = JAW[i - 1], [y1, a1] = JAW[i]; return a0 + (a1 - a0) * (y - y0) / (y1 - y0); } return JAW[JAW.length - 1][1]; };
    const B = valeriaBody(U, bodyOpts(VALERIA, { cx: CX, skin: C.shirt, legCol: C.pants, face: C.fur, neck: C.neck, jaw: false, faceA: jawA, headPw: 3.2,
      head: { top: 149, bot: 214, a: 86, d: 0.82, cz: 0 }, yoke: { y0: 204, y1: 230, a0: 40, a1: 64, d: 0.7, cz: -2, end: 236 },
      torso: { y0: 222, y1: 344, a: 66, d: 0.66, cz: -4 }, legs: { dx: 48, top: 340, bot: 422, r: 21, d: 0.95 },
      shoe: { k: 1.2, toe: 20, up: C.shoe, sole: C.sole, rim: C.shoe, strap: C.led, buckle: null, foot: FOOT } }));
    const V = (x, y, z) => new THREE.Vector3(X(x), Y(y), D(z));
    const ol = (m, w = 0.009) => {   // contorno; a posição/giro da peça passam para o grupo (a casca vai junto)
      const g = withOutline(m, w); g.position.copy(m.position); g.quaternion.copy(m.quaternion); g.scale.copy(m.scale);
      m.position.set(0, 0, 0); m.quaternion.identity(); m.scale.set(1, 1, 1); return g; };
    // padrão no shader em px do quadro (x, y para baixo, z), com os atributos das cascas (ang, edge, inner)
    function pattern(mat, glsl, key, uniforms = {}) {
      const prev = mat.onBeforeCompile, k0 = mat.customProgramCacheKey();
      mat.onBeforeCompile = (sh, r) => {
        prev.call(mat, sh, r); Object.assign(sh.uniforms, uniforms);
        const A = /\bvAng\b|\bvEdge\b|\bvInner\b/.test(glsl);
        sh.vertexShader = 'varying vec3 vPx;\n' + (A ? 'attribute float ang, edge, inner; varying float vAng, vEdge, vInner;\n' : '') + sh.vertexShader.replace('#include <begin_vertex>',
          `#include <begin_vertex>\nvPx = vec3(position.x * ${S_PX.toFixed(1)} + ${CX.toFixed(1)}, ${FOOT.toFixed(1)} - position.y * ${S_PX.toFixed(1)}, position.z * ${S_PX.toFixed(1)});` + (A ? ' vAng = ang; vEdge = edge; vInner = inner;' : ''));
        sh.fragmentShader = 'varying vec3 vPx;\n' + (A ? 'varying float vAng, vEdge, vInner;\n' : '') + Object.keys(uniforms).map(u => `uniform ${uniforms[u].type || 'vec3'} ${u};\n`).join('') + sh.fragmentShader
          .replace('#include <map_fragment>', '#include <map_fragment>\n' + glsl);
      };
      mat.customProgramCacheKey = () => k0 + '|' + key; return mat;
    }
    // casca com espessura a partir de pontos de cada fileira: pt(y, th, off) dá [x, y, z] em px; atributos ang (th),
    // edge (px até a borda aberta mais perto, ao longo da fileira) e inner (lado de dentro)
    function shellFrom(ys, ths, pt, thick) {
      const O = [], I = [], edge = [], ang = [], NC = ths.length - 1;
      for (const y of ys) { const row = ths.map(th => pt(y, th, thick)), rin = ths.map(th => pt(y, th, 0)), L = [0];
        for (let j = 1; j <= NC; j++) L.push(L[j - 1] + Math.hypot(row[j][0] - row[j - 1][0], row[j][2] - row[j - 1][2]));
        row.forEach((p, j) => { O.push(V(...p)); I.push(V(...rin[j])); edge.push(Math.min(L[j], L[NC] - L[j])); ang.push(ths[j]); }); }
      const pos = [], N = NC + 1, off = O.length, idx = [];
      for (const v of [...O, ...I]) pos.push(v.x, v.y, v.z);
      for (let i = 0; i < ys.length - 1; i++) for (let j = 0; j < NC; j++) { const a = i * N + j, b = a + 1, c = a + N, d = c + 1; idx.push(a, b, c, b, d, c, off + a, off + c, off + b, off + b, off + c, off + d); }
      for (let i = 0; i < ys.length - 1; i++) for (const j of [0, NC]) { const a = i * N + j, c = a + N; if (j === 0) idx.push(a, c, off + a, off + a, c, off + c); else idx.push(a, off + a, c, off + a, off + c, c); }
      for (const i of [0, ys.length - 1]) for (let j = 0; j < NC; j++) { const a = i * N + j, b = a + 1; if (i === 0) idx.push(a, off + a, b, b, off + a, off + b); else idx.push(a, b, off + a, b, off + b, off + a); }
      const g = new THREE.BufferGeometry(); g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3));
      g.setAttribute('edge', new THREE.Float32BufferAttribute([...edge, ...edge], 1)); g.setAttribute('ang', new THREE.Float32BufferAttribute([...ang, ...ang], 1));
      g.setAttribute('inner', new THREE.Float32BufferAttribute([...O.map(() => 0), ...I.map(() => 1)], 1)); g.setIndex(idx); g.computeVertexNormals(); return g;
    }
    const range = (a, b, n) => Array.from({ length: n + 1 }, (_, i) => a + (b - a) * i / n);
    const tube = (pts, r0, r1 = r0, NA = 14, side = new THREE.Vector3(1, 0, 0)) => {   // tubo afunilado por uma curva (px)
      const c = new THREE.CatmullRomCurve3(pts.map(p => V(...p))), NS = 96, pos = [], idx = [];
      let n = null;
      for (let i = 0; i <= NS; i++) { const u = i / NS, p = c.getPointAt(u), t = c.getTangentAt(u);
        if (!n) { n = new THREE.Vector3().crossVectors(t, side); if (n.lengthSq() < 1e-6) n.set(0, 0, 1); } else n.addScaledVector(t, -n.dot(t));   // transporte paralelo
        n.normalize(); const b = new THREE.Vector3().crossVectors(n, t).normalize(), r = D(typeof r0 === 'function' ? r0(u) : r0 + (r1 - r0) * u);
        for (let j = 0; j <= NA; j++) { const a = j / NA * Math.PI * 2; pos.push(p.x + (n.x * Math.cos(a) + b.x * Math.sin(a)) * r, p.y + (n.y * Math.cos(a) + b.y * Math.sin(a)) * r, p.z + (n.z * Math.cos(a) + b.z * Math.sin(a)) * r); } }
      for (let i = 0; i < NS; i++) for (let j = 0; j < NA; j++) { const a = i * (NA + 1) + j, b = a + 1, cc = a + NA + 1, d = cc + 1; idx.push(a, cc, b, b, cc, d); }
      const g = new THREE.BufferGeometry(); g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3)); g.setIndex(idx); g.computeVertexNormals(); return g;
    };
    const box = (w, h, d, x, y, z, c, r = 2) => { const m = new THREE.Mesh(new RoundedBoxGeometry(D(w), D(h), D(d), 3, D(r)), typeof c === 'string' ? flat(c) : c); m.position.copy(V(x, y, z)); return m; };

    // ================= grupos do motor
    const WAIST = Y(312), NECK = Y(212), HIPS = Y(326);
    const upper = new THREE.Group(); upper.position.y = WAIST; root.add(upper);
    const body = new THREE.Group(); body.position.y = -WAIST; upper.add(body);
    const neck = new THREE.Group(); neck.position.y = NECK - WAIST; upper.add(neck);
    const head = new THREE.Group(); head.position.y = D(11) - NECK; neck.add(head);   // cabeça erguida como a da Valéria
    // ================= a cabeça em volumes (pelas referências de macaco e pelo retrato): cada peça é o casco convexo de poucos
    // pontos (px do quadro, x a partir do centro, espelhado) com as quinas arredondadas (o casco encolhe e ganha uma bola de
    // raio r em cada ponto), então os planos ficam retos e as bordas macias. Crânio de pelo atrás; placa clara do rosto com a
    // testa recuada; arco da testa saltado sobre os olhos; focinho largo projetado com o nariz pequeno no alto; maxilar;
    // orelhas grandes em concha; cabelo em touca com espetos
    const spikes = [], furGeos = [];   // (o balanço antigo dos pelos da malha única não se aplica mais)
    const ink = (g, w) => { g.strokeStyle = INKS; g.lineWidth = w; g.lineCap = 'round'; g.lineJoin = 'round'; };
    const BALL = (() => { const ig = new THREE.IcosahedronGeometry(1, 2), ip = ig.attributes.position, seen = new Set(), out = [];
      for (let i = 0; i < ip.count; i++) { const k = [ip.getX(i), ip.getY(i), ip.getZ(i)].map(v => v.toFixed(3)).join(); if (!seen.has(k)) { seen.add(k); out.push(new THREE.Vector3(ip.getX(i), ip.getY(i), ip.getZ(i))); } } return out; })();
    const roundHull = (q, r) => {   // q: pontos no mundo; r em px
      let pts = q; if (r > 0) { const c = new THREE.Vector3(); q.forEach(v => c.add(v)); c.multiplyScalar(1 / q.length); const rr = D(r); pts = [];
        for (const v of q) { const d = v.clone().sub(c), L = d.length(), in0 = c.clone().addScaledVector(d, Math.max(0.2, 1 - rr / Math.max(L, 1e-6))); for (const n of BALL) pts.push(in0.clone().addScaledVector(n, rr)); } }
      let g = new ConvexGeometry(pts); g.deleteAttribute('normal'); if (g.attributes.uv) g.deleteAttribute('uv'); g = mergeVertices(g, 1e-5); g.computeVertexNormals(); return g; };
    const hullGeo = (pts, r, mirror = true) => { const q = []; for (const [x, y, z] of pts) { q.push(V(CX + x, y, z)); if (mirror && x !== 0) q.push(V(CX - x, y, z)); } return roundHull(q, r); };
    const piece = (pts, mat, r = 6, w = 0.010, mirror = true) => { const m = new THREE.Mesh(hullGeo(pts, r, mirror), typeof mat === 'string' ? flat(mat) : mat); head.add(w ? ol(m, w) : m); return m; };
    const swayTip = (m, pts) => { const tip = pts[pts.length - 1], base = pts.slice(0, -1), n = base.length, c = base.reduce((a, q) => [a[0] + q[0] / n, a[1] + q[1] / n, a[2] + q[2] / n], [0, 0, 0]);
      const A = V(CX + c[0], c[1], c[2]), ax = V(CX + tip[0], tip[1], tip[2]).sub(A), L2 = ax.lengthSq(), pp = m.geometry.attributes.position, sm = new Float32Array(pp.count), v = new THREE.Vector3();
      for (let i = 0; i < pp.count; i++) { const t = Math.min(1, Math.max(0, v.fromBufferAttribute(pp, i).sub(A).dot(ax) / L2)); sm[i] = t * t; }
      m.geometry.setAttribute('sm', new THREE.BufferAttribute(sm, 1)); return m; };
    // --- crânio (pelo)
    piece([[0, 97, 5], [38, 99, 30], [42, 98, -30], [0, 100, -48], [72, 112, 10], [66, 114, -40],
           [86, 140, 0], [76, 138, 34], [70, 140, -52], [0, 128, -76], [86, 172, -4], [74, 172, 30], [62, 175, -56], [0, 170, -80],
           [70, 200, -6], [58, 200, 26], [46, 204, -48], [0, 204, -62], [40, 218, 14], [34, 218, -30], [0, 218, -40]], C.fur, 12, 0.011);
    // --- peças do rosto com pintura (projeção de frente, px do quadro)
    const FB = { x0: 150, x1: 330, y0: 92, y1: 224 }, FK = 6;
    const paints = [];   // { mat, bg, part }
    const faceTex = (bg, part, f) => { const c = document.createElement('canvas'); c.width = (FB.x1 - FB.x0) * FK; c.height = (FB.y1 - FB.y0) * FK; const g = c.getContext('2d');
      g.scale(FK, FK); g.translate(-FB.x0, -FB.y0); g.fillStyle = bg; g.fillRect(FB.x0, FB.y0, FB.x1 - FB.x0, FB.y1 - FB.y0); marcovFace(g, f, part);
      const t = new THREE.CanvasTexture(c); t.colorSpace = THREE.SRGBColorSpace; t.anisotropy = 8; return t; };
    const painted = (pts, bg, part, r = 6, w = 0.010) => { const mat = new THREE.MeshToonMaterial({ color: '#ffffff', gradientMap: flat('#fff').gradientMap, map: faceTex(bg, part, 'normal') });
      mat.onBeforeCompile = () => {};   // marca a peça para o exportador assar (é o que leva as expressões para o jogo)
      const m = piece(pts, mat, r, w), g = m.geometry, pp = g.attributes.position, uv = new Float32Array(pp.count * 2);
      for (let i = 0; i < pp.count; i++) uv.set([(pp.getX(i) * S_PX + CX - FB.x0) / (FB.x1 - FB.x0), 1 - (FOOT - pp.getY(i) * S_PX - FB.y0) / (FB.y1 - FB.y0)], i * 2);
      g.setAttribute('uv', new THREE.BufferAttribute(uv, 2)); paints.push({ mat, bg, part }); return m; };
    // placa do rosto: escudo de seis lados (lados retos, quebra no canto do maxilar, queixo largo); a testa recua para trás
    painted([[44, 101, 34], [68, 111, 38], [79, 127, 44], [81, 166, 47], [65, 198, 45], [40, 216, 43], [0, 217, 46], [0, 102, 38], [0, 165, 52],
             [44, 101, 14], [68, 111, 15], [79, 127, 16], [81, 166, 18], [65, 198, 18], [40, 216, 18]], C.skin, 'plate', 7, 0.011);
    // maxilar: bloco largo que avança embaixo do focinho (a boca é a junta dos dois)
    painted([[49, 187, 44], [42, 188, 77], [0, 190, 81], [47, 204, 86], [0, 205, 92], [40, 218, 44], [37, 217, 78], [0, 218, 84]], C.skin, 'jaw', 6, 0.009);
    // focinho: almofada larga e baixa, mais larga embaixo, bem projetada
    painted([[38, 169, 44], [47, 191, 44], [34, 171, 70], [42, 187, 77], [0, 169, 74], [0, 190, 81], [41, 179, 76]], C.muzzle, 'muzzle', 10, 0.010);
    // nariz de macaco: pequeno, em coração, no alto do focinho, com as duas narinas juntas viradas para a frente
    painted([[7, 150, 60], [13, 167, 76], [0, 172, 82], [0, 148, 64], [8, 160, 73]], C.muzzle, 'nose', 5, 0);   // sem contorno: é um relevo do focinho, não um disco
    // arco da testa: a barra saltada sobre os olhos (a testa proeminente), com as sobrancelhas pintadas nela
    painted([[84, 124, 16], [84, 140, 16], [80, 124, 49], [80, 140, 50], [58, 115, 52], [34, 110, 54], [34, 142, 56], [0, 108, 55], [0, 144, 58], [0, 106, 38], [40, 108, 36]], C.skin, 'ridge', 6, 0.008);   // topo em arco, não reto

    function marcovFace(g, f, part) {
      if (part === 'plate') {
        g.strokeStyle = C.line; g.lineWidth = 2.2; g.lineCap = 'round'; g.beginPath();   // rugas da testa
        for (const [y, x0, x1] of [[105.5, 212, 268], [110, 200, 280]]) { g.moveTo(x0, y); g.lineTo(x1, y); } g.stroke();
        g.fillStyle = C.skinDark; for (const sd of [-1, 1]) { g.beginPath(); g.moveTo(240 + sd * 81, 170); g.lineTo(240 + sd * 66, 198); g.lineTo(240 + sd * 42, 216); g.lineTo(240 + sd * 56, 190); g.closePath(); g.fill(); }   // plano do maxilar na sombra
        if (f === 'pain') { ink(g, 4.5); g.beginPath(); for (const sd of [-1, 1]) { g.moveTo(240 + sd * 52, 146); g.lineTo(240 + sd * 26, 154); g.lineTo(240 + sd * 52, 162); } g.stroke(); }
        else if (f === 'ko') { ink(g, 4.5); g.beginPath(); for (const sd of [-1, 1]) { const x = 240 + sd * 38; g.moveTo(x - 9, 145); g.lineTo(x + 9, 163); g.moveTo(x + 9, 145); g.lineTo(x - 9, 163); } g.stroke(); }
        else for (const sd of [-1, 1]) {   // olho de desenho: branco arredondado embaixo do arco, íris castanha grande, pupila e brilho
          const X0 = x => 240 + sd * x, lid = f === 'angry' ? 5 : 0;
          const eye = () => { g.beginPath(); g.moveTo(X0(60), 142 + lid * 0.4); g.lineTo(X0(17), 147 + lid); g.quadraticCurveTo(X0(16), 164, X0(38), 165); g.quadraticCurveTo(X0(60), 164, X0(60), 142 + lid * 0.4); g.closePath(); };
          eye(); g.fillStyle = '#fbf8f0'; g.fill();
          g.save(); eye(); g.clip();
          g.fillStyle = '#6b3d1e'; g.beginPath(); g.arc(X0(37), 153 + lid * 0.5, 10, 0, 7); g.fill();
          g.fillStyle = '#17100c'; g.beginPath(); g.arc(X0(37), 153 + lid * 0.5, 5.4, 0, 7); g.fill();
          g.fillStyle = '#ffffff'; g.beginPath(); g.arc(240 + sd * 37 - 3.4, 149.4 + lid * 0.5, 2.2, 0, 7); g.fill();
          g.restore(); ink(g, 2.6); eye(); g.stroke();
          g.strokeStyle = C.line; g.lineWidth = 2; g.beginPath(); g.moveTo(X0(46), 171); g.quadraticCurveTo(X0(56), 173, X0(64), 169); g.stroke(); }   // olheira
      }
      if (part === 'ridge') {   // sobrancelhas grossas pintadas no arco: altas do lado de fora, caindo para o nariz
        const tin = f === 'angry' ? 4 : f === 'pain' ? -8 : f === 'ko' ? -6 : 0, tout = f === 'angry' ? -3 : f === 'pain' ? 5 : f === 'ko' ? 4 : 0;
        ink(g, 9); g.strokeStyle = '#1c1210'; for (const sd of [-1, 1]) { g.beginPath(); g.moveTo(240 + sd * 64, 126 + tout); g.lineTo(240 + sd * 8, 135 + tin); g.stroke(); } }
      if (part === 'nose') { g.fillStyle = '#1c1210'; for (const sd of [-1, 1]) { g.beginPath(); g.ellipse(240 + sd * 5.8, 165, 4.8, 2.4, sd * 0.95, 0, 7); g.fill(); } }
      if (part === 'muzzle') { g.strokeStyle = C.line; g.lineWidth = 2; g.lineCap = 'round'; g.beginPath(); g.moveTo(240, 175); g.lineTo(240, f === 'ko' ? 179 : 190); g.stroke();   // o sulco do lábio
        if (f === 'ko') { g.fillStyle = '#3a0f12'; g.beginPath(); g.ellipse(240, 190, 16, 9, 0, 0, 7); g.fill(); g.fillStyle = '#c2475a'; g.beginPath(); g.ellipse(240, 194.5, 10, 3.6, 0, 0, 7); g.fill(); ink(g, 3.2); g.beginPath(); g.ellipse(240, 190, 16, 9, 0, 0, 7); g.stroke(); } }   // boca aberta: elipse centrada na junta do focinho com o maxilar (metade em cada peça)
      if (part === 'jaw') {
        if (f === 'angry') { g.fillStyle = '#f4f1e6'; g.fillRect(200, 190, 80, 13); ink(g, 1.6); g.beginPath(); for (const x of [213, 226.5, 240, 253.5, 267]) { g.moveTo(x, 190); g.lineTo(x, 203); } g.stroke(); ink(g, 3.6); g.strokeRect(200, 190, 80, 13); }
        else if (f === 'pain') { ink(g, 4.5); g.beginPath(); g.moveTo(196, 195); for (let k = 0; k < 6; k++) g.quadraticCurveTo(203 + k * 15, k % 2 ? 201 : 189, 211 + k * 15, 195); g.stroke(); }
        else if (f === 'ko') { g.fillStyle = '#3a0f12'; g.beginPath(); g.ellipse(240, 190, 16, 9, 0, 0, 7); g.fill(); g.fillStyle = '#c2475a'; g.beginPath(); g.ellipse(240, 194.5, 10, 3.6, 0, 0, 7); g.fill(); ink(g, 3.2); g.beginPath(); g.ellipse(240, 190, 16, 9, 0, 0, 7); g.stroke(); }
        else { ink(g, 4.6); g.beginPath(); g.moveTo(190, 196.5); g.quadraticCurveTo(240, 192, 290, 196.5); g.stroke(); }
        g.strokeStyle = C.line; g.lineWidth = 2.2; g.lineCap = 'round'; g.beginPath(); g.moveTo(224, 207); g.lineTo(256, 207); g.stroke(); }
    }
    const muzzleMat = paints.find(q => q.part === 'jaw').mat, muzzleTex = f => faceTex(C.skin, 'jaw', f);   // a mastigação troca a boca (no maxilar)

    // ================= cabelo: touca escura do alto da testa até a nuca e os espetos no topo; tufos de pelo nas bochechas
    piece([[0, 92, 8], [40, 94, 34], [46, 98, 46], [66, 106, 44], [0, 97, 50], [44, 93, -34], [0, 95, -54], [78, 108, 12], [70, 109, -44], [82, 120, 34],
           [90, 131, 2], [74, 150, -56], [0, 126, -81], [0, 162, -84]], C.hair, 9, 0.010);
    for (const [n, z0, h0, rr, lean] of [[7, 30, 40, 14, 3], [6, 6, 38, 14, -3], [5, -20, 30, 13, -9]])   // três fileiras de espetos, da testa à nuca
      for (let i = 0; i < n; i++) { const k = i - (n - 1) / 2, x = k * 21, z = z0 - Math.abs(k) * 3, h = h0 + (i % 2 ? 9 : 0) - Math.abs(k) * 3, yb = 99 + Math.abs(k) * 2.2;
        const pts = [[x - rr, yb, z - rr], [x + rr, yb, z - rr], [x - rr, yb + 1, z + rr], [x + rr, yb + 1, z + rr], [x + k * 6 + 3, yb - h, z + lean]]; swayTip(piece(pts, C.hair, 3, 0.008, false), pts); }
    for (const sd of [-1, 1]) for (const [y, o, tx, ty, hh] of [[160, 86, 116, 170, 12], [180, 84, 118, 198, 15], [200, 76, 104, 224, 14], [214, 60, 84, 236, 11]])   // costeletas volumosas
    { const pts = [[o, y - hh, -24], [o, y - hh, 20], [o - 8, y + hh, -20], [o - 8, y + hh, 16], [tx, ty, -2]].map(([x, yy, z]) => [sd * x, yy, z]); swayTip(piece(pts, C.fur, 3, 0.008, false), pts); }

    // ================= orelhas: discos grandes em concha, virados para a frente, com o miolo escuro
    for (const sd of [-1, 1]) {
      const eg = new THREE.Group(); eg.position.copy(V(CX + sd * 100, 150, -6)); eg.rotation.y = sd * 0.5; head.add(eg);
      const ring = (rx, ry, z, ox = 0) => Array.from({ length: 10 }, (_, k) => { const a = (k + 0.5) * Math.PI / 5; return new THREE.Vector3(D(ox + Math.cos(a) * rx), D(Math.sin(a) * ry), D(z)); });
      eg.add(ol(new THREE.Mesh(roundHull([...ring(27, 35, 0), ...ring(21, 28, -10), ...ring(23, 30, 6)], 5), flat(C.skin)), 0.011));
      eg.add(new THREE.Mesh(roundHull([...ring(15, 22, 6.5, sd * 2), ...ring(11, 17, 10, sd * 3)], 2.5), flat(C.earIn)));
    }

    // ================= camisa azul-petróleo: faixas escuras no alto, tela do monitor cardíaco com a linha, barrinha de
    // luzes embaixo, um botão; da cintura para baixo o cinto e a calça cinza-chumbo (mesma peça)
    const TZ = (x, y) => -4 + 66 * 0.66 * Math.sqrt(Math.max(0, 1 - ((x - CX) / 66) ** 2));
    { const m = pattern(flat(C.shirt), `
        { if (vPx.y < 246.0 && fract((vPx.y - 228.0) / 9.0) < 0.42) diffuseColor.rgb = uDark;
          if (vPx.y > 312.0) diffuseColor.rgb = vPx.y < 320.0 ? uBelt : uPants;
        }`, 'mshirt', { uDark: col(C.shirtDark), uBelt: col(C.belt), uPants: col(C.pants) });
      B.torso.children[0].material = m; body.add(B.torso);
      const sc = box(44, 28, 5, CX, 266, TZ(CX, 266) + 1, C.screen, 3); body.add(ol(sc, 0.005));
      body.add(new THREE.Mesh(tube([[222, 268, TZ(CX, 266) + 4.2], [229, 268, TZ(CX, 266) + 4.2], [233, 258, TZ(CX, 266) + 4.2], [239, 276, TZ(CX, 266) + 4.2], [244, 264, TZ(CX, 266) + 4.2], [258, 268, TZ(CX, 266) + 4.2]], 1.4), flat('#f4fbfb')));
      body.add(ol(box(40, 8, 3, CX, 289, TZ(CX, 289), C.shirtDark, 1.5), 0.004));
      for (const [x, c] of [[228, '#e8862c'], [246, '#9fb4b8']]) body.add(box(7, 4, 2, x, 289, TZ(x, 289) + 1.6, c, 1));
      body.add(ol(box(14, 10, 3, CX, 316, TZ(CX, 316) + 1, '#b8bcc0', 2), 0.004));   // fivela do cinto
      for (const y of [298]) { const b = new THREE.Mesh(new THREE.CylinderGeometry(D(3.5), D(3.5), D(2), 14), flat('#3d6672')); b.rotation.x = Math.PI / 2; b.position.copy(V(CX, y, TZ(CX, y) + 1)); body.add(b); } }

    // ================= jaleco branco aberto: casca do ombro até o joelho, abertura na frente, dobras que crescem para a
    // barra com o risco no fundo, forro mais escuro na sombra, gola com lapelas, bolso com canetas e os bolsos de baixo
    { // o alto fecha por cima dos ombros até a gola, colado no pescoço
      const CA = y => 56 + 19 * ss(204, 236, y) + 24 * ss(300, 396, y), GAP = y => 18 + 12 * ss(290, 396, y), CD = y => 0.68 + 0.16 * (1 - ss(204, 250, y));
      const ys = range(201, 396, 130), NF = 12;
      const pt = (y, th, o) => { const a = CA(y) + o, fold = 2.6 * ss(290, 396, y) * Math.cos(NF * th); return [CX + (a + fold) * Math.cos(th), y, -4 + (a + fold) * CD(y) * Math.sin(th)]; };
      // a abertura: ângulos de um lado da gola ao outro passando pelas costas; atrás a fenda em V (abre da cintura para a
      // barra) divide o jaleco em duas abas, e é por ela que o rabo sai. Cada aba é uma casca com as bordas abertas
      const t0 = y => Math.asin(Math.min(0.99, GAP(y) / CA(y))), VG = y => 24 * ss(316, 396, y), tv = y => Math.asin(Math.min(0.99, VG(y) / CA(y)));
      const piece = (thA, thB) => { const NC = 60, O = [], I = [], edge = [], ang = [], pos = [], idx = [];
        ys.forEach(y => { const ths = range(thA(y), thB(y), NC), pr = ths.map(th => pt(y, th, 3)), L = [0];
          for (let j = 1; j <= NC; j++) L.push(L[j - 1] + Math.hypot(pr[j][0] - pr[j - 1][0], pr[j][2] - pr[j - 1][2]));
          ths.forEach((th, j) => { O.push(V(...pr[j])); I.push(V(...pt(y, th, 0))); edge.push(Math.min(L[j], L[NC] - L[j])); ang.push(th); }); });
        const N = NC + 1, off = O.length;
        for (const v of [...O, ...I]) pos.push(v.x, v.y, v.z);
        for (let i = 0; i < ys.length - 1; i++) for (let j = 0; j < NC; j++) { const a = i * N + j, b = a + 1, c = a + N, d = c + 1; idx.push(a, b, c, b, d, c, off + a, off + c, off + b, off + b, off + c, off + d); }
        for (let i = 0; i < ys.length - 1; i++) for (const j of [0, NC]) { const a = i * N + j, c = a + N; if (j === 0) idx.push(a, c, off + a, off + a, c, off + c); else idx.push(a, off + a, c, off + a, off + c, c); }
        for (const i of [0, ys.length - 1]) for (let j = 0; j < NC; j++) { const a = i * N + j, b = a + 1; if (i === 0) idx.push(a, off + a, b, b, off + a, off + b); else idx.push(a, b, off + a, b, off + b, off + a); }
        const gg = new THREE.BufferGeometry(); gg.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3));
        gg.setAttribute('edge', new THREE.Float32BufferAttribute([...edge, ...edge], 1)); gg.setAttribute('ang', new THREE.Float32BufferAttribute([...ang, ...ang], 1));
        gg.setAttribute('inner', new THREE.Float32BufferAttribute([...O.map(() => 0), ...I.map(() => 1)], 1)); gg.setIndex(idx); gg.computeVertexNormals(); return gg; };
      const coatGeos = [piece(y => Math.PI / 2 + t0(y), y => Math.PI * 1.5 - tv(y)), piece(y => Math.PI * 1.5 + tv(y), y => Math.PI * 2.5 - t0(y))];
      const m = pattern(flat(C.coat), `
        { float fs = cos(${NF.toFixed(1)} * vAng), fw = 0.08 * smoothstep(300.0, 394.0, vPx.y);
          if (vInner < 0.5 && fs < -1.0 + fw && vEdge > 10.0) diffuseColor.rgb = uInk;
          if (vInner < 0.5 && abs(vPx.x - 240.0) > 52.0 && vPx.z > -10.0) diffuseColor.rgb = mix(diffuseColor.rgb, uCoatSh, 0.55);
          if (vInner > 0.5) { diffuseColor.rgb = uLining; extraTone = 0.6; } }`, 'mcoat', { uInk: col('#1a2224'), uCoatSh: col(C.coatShade), uLining: col(C.lining) });
      m.side = THREE.DoubleSide; for (const g of coatGeos) body.add(ol(new THREE.Mesh(g, m), 0.010)); P.clothGeos = [...coatGeos];
      const cz = (x, y) => -4 + CA(y) * CD(y) * Math.sqrt(Math.max(0, 1 - ((x - CX) / CA(y)) ** 2)) + 3;
      // peças coladas no jaleco: cascas finas tiradas da própria superfície dele (seguem a curva e as dobras, não são
      // cartões retos). patch(ys, us, th(y, u), off): u vai de um lado ao outro da peça
      const patch = (ys, us, th, off, col, w = 0.005) => { const g = shellFrom(ys, us, (y, u, o) => pt(y, th(y, u), 3 + off + o), 1.2); P.clothGeos.push(g); return ol(new THREE.Mesh(g, flat(col)), w); };
      const thX = (x, y) => Math.acos(Math.max(-1, Math.min(1, (x - CX) / CA(y))));   // ângulo da frente que cai no x
      // lapelas: faixa dobrada ao longo da abertura, do alto do jaleco até onde ela termina, larga em cima e fechando
      // em ponta embaixo, sempre colada na beira da abertura
      for (const s of [-1, 1]) { const t0 = y => Math.asin(Math.min(0.99, GAP(y) / CA(y))), wd = y => 17 * (1 - ss(232, 264, y)) + 0.6;
        body.add(patch(range(202, 264, 40), range(0, 1, 10), (y, u) => Math.PI / 2 - s * (t0(y) + u * wd(y) / CA(y)), 0.4, '#f9faf4')); }
      // bolso do peito (lado esquerdo dele, direita da tela) com as canetas; bolsos de baixo dos dois lados
      const pocket = (x0, x1, y0, y1) => patch(range(y0, y1, 12), range(0, 1, 12), (y, u) => thX(x0 + (x1 - x0) * u, y), 0.3, '#f4f6ee');
      { const x = 280, y = 268; body.add(pocket(x - 14, x + 14, y - 15, y + 15)); const z = pt(y - 15, thX(x, y - 15), 4.5)[2];
        for (const [dx, c] of [[-6, C.red], [5, '#2e7d8e']]) { const pn = new THREE.Mesh(new THREE.CylinderGeometry(D(2.4), D(2.4), D(26), 10), flat(c)); pn.position.copy(V(x + dx, y - 16, z + 1)); body.add(ol(pn, 0.003)); } }
      for (const s of [-1, 1]) { const x = CX + s * 50; body.add(pocket(x - 16, x + 16, 331, 361)); } }

    // ================= quadril e rabo
    const hips = new THREE.Group(); hips.position.y = HIPS; root.add(hips);
    const low = new THREE.Group(); low.position.y = -HIPS; hips.add(low);
    // o rabo nasce no meio de trás do quadril, desce por dentro do jaleco, sai pela fenda e sobe atrás dele em ponto de
    // interrogação (a ponta enrolada na altura da cintura)
    const TP = [[CX, 344, -38], [CX + 1, 364, -60], [CX + 4, 386, -82], [CX + 16, 404, -104], [CX + 40, 398, -122], [CX + 62, 370, -128],
                [CX + 70, 334, -122], [CX + 62, 304, -114], [CX + 44, 296, -112], [CX + 36, 312, -114]];
    const tailG = new THREE.Group(); tailG.position.copy(V(...TP[0])); low.add(tailG);
    // animação: a parte de fora da fenda gira para os lados em volta da saída (devagar), sobe e desce um pouco, uma onda
    // corre da base para a ponta e a ponta enrola e desenrola
    const tailPts = t => { const [px, py, pz] = TP[2], sw = 0.42 * Math.sin(t * 1.3), lf = 0.16 * Math.sin(t * 1.9 + 0.7), cu = 0.5 + 0.5 * Math.sin(t * 2.3);
      const [cxc, cyc, czc] = TP[7];
      return TP.map(([x, y, z], k) => { if (k < 3) return [x, y, z]; const w = ss(2, TP.length - 1, k), tip = ss(6, TP.length - 1, k);
        // ponta: enrola mais (puxa para o centro da volta) ou abre
        x += (cxc - x) * 0.2 * tip * cu; y += (cyc - y) * 0.2 * tip * cu; z += (czc - z) * 0.12 * tip * cu;
        // sobe/desce girando em volta do eixo x na saída
        let dy = y - py, dz = z - pz; [dy, dz] = [dy * Math.cos(lf * w) - dz * Math.sin(lf * w), dy * Math.sin(lf * w) + dz * Math.cos(lf * w)]; y = py + dy; z = pz + dz;
        // para os lados girando em volta do eixo y na saída
        let dx = x - px; dz = z - pz; const a = sw * w; [dx, dz] = [dx * Math.cos(a) + dz * Math.sin(a), -dx * Math.sin(a) + dz * Math.cos(a)]; x = px + dx; z = pz + dz;
        // a onda que corre para a ponta
        const o = 5 * w * Math.sin(t * 3.4 - k * 0.9); return [x + o, y + o * 0.4, z]; }); };
    // ponta arredondada: o raio afina e fecha numa calota (antes acabava num corte reto)
    const tailR = u => (11 - 4.2 * u) * (u > 0.965 ? Math.sqrt(Math.max(0, 1 - ((u - 0.965) / 0.035) ** 2)) : 1);
    const tailMesh = new THREE.Mesh(tube(tailPts(0), tailR), flat(C.tail)); tailMesh.geometry.translate(-tailG.position.x, -tailG.position.y, -tailG.position.z); tailG.add(ol(tailMesh, 0.009));
    const waveTail = t => { const g = tube(tailPts(t), tailR); g.translate(-tailG.position.x, -tailG.position.y, -tailG.position.z);
      tailMesh.geometry.attributes.position.array.set(g.attributes.position.array); tailMesh.geometry.attributes.position.needsUpdate = true; tailMesh.geometry.computeVertexNormals(); g.dispose(); };

    // ================= calça: o gancho (fecha o vão entre as pernas, anda com o quadril) e as pernas da base em cinza-chumbo
    // com o vinco na frente e a barra larga caindo por cima da bota
    { const pel = new THREE.Mesh(new THREE.SphereGeometry(1, 32, 20), flat(C.pants)); pel.scale.set(D(68), D(30), D(42)); pel.position.copy(V(CX, 338, -4)); low.add(ol(pel, 0.009)); }
    const LXp = { [-1]: 192, [1]: 288 }, legs = {}, feet = {};
    for (const s of [-1, 1]) { legs[s] = B.legs[s]; root.add(legs[s]);
      // bota de cientista: cano curto azul-marinho, pé comprido de bico arredondado, biqueira mais clara, a tira no
      // peito do pé, a luzinha ciano do lado de fora e a sola grossa escura
      const f = new THREE.Group(); f.position.copy(V(LXp[s], 418, 0)); root.add(f);
      const cuff = new THREE.Mesh(new THREE.CylinderGeometry(D(25), D(27), D(12), 24), flat(C.pants)); cuff.position.y = D(-1); f.add(ol(cuff, 0.008));   // barra da calça
      const sh = new THREE.Mesh(new RoundedBoxGeometry(D(46), D(26), D(48), 4, D(10)), flat(C.shoe)); sh.position.y = D(-16); f.add(ol(sh, 0.009));
      const ft = new THREE.Mesh(new RoundedBoxGeometry(D(50), D(20), D(76), 4, D(9)), flat(C.shoe)); ft.position.set(0, D(-28), D(12)); f.add(ol(ft, 0.009));
      const cap = new THREE.Mesh(new RoundedBoxGeometry(D(51), D(16), D(24), 4, D(7)), flat(C.shoeHi)); cap.position.set(0, D(-28), D(39)); f.add(ol(cap, 0.006));
      const st = new THREE.Mesh(new RoundedBoxGeometry(D(52), D(4), D(12), 2, D(2)), flat(C.shoeHi)); st.position.set(0, D(-19), D(16)); st.rotation.x = -0.45; f.add(ol(st, 0.004));
      const led = new THREE.Mesh(new RoundedBoxGeometry(D(2), D(4), D(36), 1, D(1)), new THREE.MeshBasicMaterial({ color: C.led })); led.position.set(D(s * 25.5), D(-27), D(10)); f.add(led);
      const so = new THREE.Mesh(new RoundedBoxGeometry(D(54), D(8), D(80), 2, D(4)), flat(C.sole)); so.position.set(0, D(-37), D(12)); f.add(ol(so, 0.006));
      feet[s] = f; }
    const placeFoot = legPlacer({ legs, feet, hipX: { [-1]: X(LXp[-1]), [1]: X(LXp[1]) }, top: B.legTop, bot: Y(418), hips, hipsY: HIPS });

    // ================= luvas marrons quadradas: três pontas de dedo claras em cima, polegar claro do lado de dentro,
    // punho azul-petróleo com a luzinha. Y = dos punhos aos nós dos dedos, Z = lado da palma (como as luvas do Frog)
    const L = (x, y, z) => new THREE.Vector3(D(x), D(y), D(z));
    function glove(sx) {
      const g = new THREE.Group();
      const fist = new THREE.Mesh(new RoundedBoxGeometry(D(70), D(62), D(58), 5, D(16)), flat(C.glove)); fist.position.copy(L(0, 6, 0)); g.add(ol(fist, 0.010));
      for (const x of [-20, 0, 20]) { const t = new THREE.Mesh(new RoundedBoxGeometry(D(13), D(20), D(10), 3, D(3)), flat(C.tip)); t.position.copy(L(x, 32, 22)); g.add(ol(t, 0.005)); }
      const th = new THREE.Mesh(new RoundedBoxGeometry(D(18), D(26), D(18), 3, D(6)), flat(C.tip)); th.position.copy(L(-sx * 36, 4, 10)); g.add(ol(th, 0.006));
      const cuff = new THREE.Mesh(new RoundedBoxGeometry(D(72), D(20), D(60), 3, D(5)), flat(C.cuff)); cuff.position.copy(L(0, -32, 0)); g.add(ol(cuff, 0.008));
      const led = new THREE.Mesh(new RoundedBoxGeometry(D(22), D(8), D(3), 2, D(1.5)), new THREE.MeshBasicMaterial({ color: C.led })); led.position.copy(L(0, -32, 30.5)); g.add(ol(led, 0.003));
      return g;
    }
    P.gloves = { [-1]: glove(-1), [1]: glove(1) }; root.add(P.gloves[-1], P.gloves[1]);
    const hands = { [-1]: new THREE.Object3D(), [1]: new THREE.Object3D() }; root.add(hands[-1], hands[1]);
    const UPV = new THREE.Vector3(0, 1, 0), FWD = new THREE.Vector3(0, 0, 1);

    const setFace = f => { for (const q of paints) { q.mat.map = faceTex(q.bg, q.part, f); q.mat.needsUpdate = true; } };
    // balanço dos pelos (topete e costeletas): cada ponta com a sua mola, puxada para trás pela aceleração da cabeça
    // (rigidez um pouco diferente em cada uma, para não balançarem em bloco); os vértices andam conforme o sm deles
    const hp = new THREE.Vector3(), hPrev = new THREE.Vector3(), hVel = new THREE.Vector3(), hAcc = new THREE.Vector3(), hq = new THREE.Quaternion();
    let hInit = false;
    const springs = spikes.map((sp, i) => ({ o: new THREE.Vector3(), v: new THREE.Vector3(), k: 70 + (i % 4) * 14, max: D(sp.h * 0.35) }));
    const furSets = () => furGeos.map(g => { const p = g.attributes.position, sid = g.attributes.sid, sm = g.attributes.sm, idx = [];
      for (let i = 0; i < p.count; i++) if (sid.getX(i) >= 0 && sm.getX(i) > 0) idx.push(i);
      return { p, idx, base: Float32Array.from(idx.flatMap(i => [p.getX(i), p.getY(i), p.getZ(i)])), sid: idx.map(i => sid.getX(i)), sm: idx.map(i => sm.getX(i)) }; });
    let furs = null;
    function furStep(c) {
      const dt = Math.min(0.05, Math.max(c.dt || 1 / 60, 1e-3)); furs = furs || furSets();
      head.updateWorldMatrix(true, false); head.getWorldPosition(hp); if (!hInit) { hPrev.copy(hp); hInit = true; }
      const vel = hp.clone().sub(hPrev).divideScalar(dt); hAcc.copy(vel).sub(hVel).divideScalar(dt); hVel.copy(vel); hPrev.copy(hp);
      head.getWorldQuaternion(hq).invert(); const A = hAcc.clone().applyQuaternion(hq).clampLength(0, 40);
      springs.forEach((st, i) => { const idle = new THREE.Vector3(Math.sin(c.t * 2.1 + i * 1.7), 0, Math.cos(c.t * 1.7 + i * 2.3)).multiplyScalar(0.012 * st.k);
        st.v.addScaledVector(st.o, -st.k * dt).addScaledVector(st.v, -5 * dt).addScaledVector(A, -1.6 * dt).addScaledVector(idle, dt); st.o.addScaledVector(st.v, dt);
        if (st.o.length() > st.max) st.o.setLength(st.max); });
      for (const f of furs) { const a = f.p.array; f.idx.forEach((i, j) => { const o = springs[f.sid[j]].o, m = f.sm[j];
        a[i * 3] = f.base[j * 3] + o.x * m; a[i * 3 + 1] = f.base[j * 3 + 1] + o.y * m; a[i * 3 + 2] = f.base[j * 3 + 2] + o.z * m; }); f.p.needsUpdate = true; }
    }
    // ================= especiais (os do jogo, skills.cpp e skill_poses.cpp, com os efeitos do laboratório, vfx/lab_marcov.cpp)
    const easeO = k => 1 - (1 - k) ** 3, win = (a, b, x) => Math.max(0, Math.min(1, (x - a) / (b - a))), DIST = 1.75, CHEST = Y(262);
    const PLASMA = '#38bdf8', DEEP = '#0369a1', ICE = '#bae6fd', ORANGE = '#f97316', SMOKE = '#6b7280', SMOKE2 = '#9ca3af', RED = '#ef4444', O2 = '#7dd3fc', YEL = '#fde047';
    root.updateMatrixWorld(true);
    // os efeitos ficam no mundo: fxRoot desconta o deslocamento que os golpes dão no corpo
    const fxRoot = new THREE.Group(); root.add(fxRoot); const off = new THREE.Vector3();
    const glow = (col, o = 1, add = false) => new THREE.MeshBasicMaterial({ color: col, transparent: true, opacity: o, depthWrite: false, side: THREE.DoubleSide, blending: add ? THREE.AdditiveBlending : THREE.NormalBlending });
    const queue = []; let now = 0; const after = (dt, fn) => queue.push({ at: now + dt, fn });
    const FX = { sign: -99, rocket: -99, laser: -99, plasma: -99, kinetic: -99, exhaust: -99, o2: -99 };
    // bolinhas: fagulhas, fumaça e fogo
    const dotG = new THREE.SphereGeometry(1, 10, 8), dots = Array.from({ length: 260 }, () => { const m = new THREE.Mesh(dotG, glow('#fff')); m.visible = false; fxRoot.add(m); return { m, life: 0, v: new THREE.Vector3() }; });
    let dotI = 0; const dot = (p, v, rr, col, life, g = 3, op = 1, grow = 0) => { const q = dots[dotI++ % dots.length]; q.m.position.copy(p); q.m.scale.setScalar(rr); q.v.copy(v); q.r = rr; q.m.material.color.set(col); q.m.material.opacity = op; q.op = op; q.life = q.life0 = life; q.g = g; q.grow = grow; q.m.visible = true; };
    const burst = (p, n, spd, rr, cols, life, g = 3) => { for (let i = 0; i < n; i++) { const a = Math.random() * 6.283, sp = spd * (0.5 + Math.random()); dot(p, new THREE.Vector3(Math.cos(a) * sp, Math.sin(a) * sp * 0.8 + 0.4, (Math.random() - 0.5) * spd), rr * (0.6 + Math.random() * 0.7), cols[i % cols.length], life * (0.7 + Math.random() * 0.6), g); } };
    const puff = (p, v, rr, life, op = 0.97, grow = 0.3) => dot(p, v, rr, Math.random() < 0.5 ? SMOKE : SMOKE2, life, 0, op, grow);
    const ringG = new THREE.RingGeometry(0.86, 1, 44), rings = Array.from({ length: 12 }, () => { const m = new THREE.Mesh(ringG, glow('#fff')); m.visible = false; fxRoot.add(m); return { m, life: 0 }; });
    let ringI = 0; const ring = (p, R, col, life, flatR = false) => { const q = rings[ringI++ % rings.length]; q.m.position.copy(p); q.R = R; q.life = q.life0 = life; q.m.material.color.set(col); q.flat = flatR; q.m.rotation.set(flatR ? -Math.PI / 2 : 0, 0, 0); q.m.scale.setScalar(0.02); q.m.visible = true; };
    // riscos de velocidade e raios: barrinhas esticadas entre dois pontos
    const segG = new THREE.BoxGeometry(1, 1, 1), ZA = new THREE.Vector3(0, 0, 1), segs = Array.from({ length: 70 }, () => { const m = new THREE.Mesh(segG, glow('#fff')); m.visible = false; fxRoot.add(m); return { m, life: 0, v: new THREE.Vector3() }; });
    let segI = 0; const seg = (a, b, w, col, life, v = null) => { const q = segs[segI++ % segs.length], d = b.clone().sub(a), L = d.length(); q.m.position.copy(a).addScaledVector(d, 0.5); q.m.quaternion.setFromUnitVectors(ZA, d.normalize()); q.m.scale.set(w, w, L); q.m.material.color.set(col); q.m.material.opacity = 1; q.life = q.life0 = life; q.v.set(0, 0, 0); if (v) q.v.copy(v); q.m.visible = true; };
    const bolt = (a, b, w, col, life, n = 5) => { let p = a.clone(); for (let i = 1; i <= n; i++) { const q = a.clone().lerp(b, i / n); if (i < n) q.add(new THREE.Vector3((Math.random() - 0.5) * 0.09, (Math.random() - 0.5) * 0.09, (Math.random() - 0.5) * 0.09)); seg(p, q, w, col, life); p = q; } };
    // chama em dois tons: o miolo claro virado para a câmera e a borda colorida, no traço chapado do vinil
    const flameMat = (out, core) => new THREE.ShaderMaterial({ uniforms: { uO: { value: new THREE.Color(out).convertLinearToSRGB() }, uC: { value: new THREE.Color(core).convertLinearToSRGB() } },
      vertexShader: 'varying vec3 vN; varying vec3 vV; void main() { vec4 mv = modelViewMatrix * vec4(position, 1.0); vN = normalize(normalMatrix * normal); vV = normalize(-mv.xyz); gl_Position = projectionMatrix * mv; }',
      fragmentShader: 'uniform vec3 uO; uniform vec3 uC; varying vec3 vN; varying vec3 vV; void main() { gl_FragColor = vec4(mix(uO, uC, step(0.72, abs(dot(normalize(vN), normalize(vV))))), 1.0); }' });
    // as chamas das botas, sob a sola (a assinatura sobe nelas)
    const bootFlames = [-1, 1].map(sd => { const h = D(70), g = new THREE.ConeGeometry(D(22), h, 14); g.rotateX(Math.PI); g.translate(0, -h / 2, 0); const o = withOutline(new THREE.Mesh(g, flameMat(ORANGE, YEL)), 0.006); o.position.set(0, D(-41), D(12)); o.visible = false; feet[sd].add(o); return o; });
    // as turbinas nas costas, azuis, viradas para trás
    const bodyBox = new THREE.Box3().setFromObject(body), BACK = bodyBox.min.z;
    const jetFl = [-1, 1].map(sd => { const h = 0.5, g = new THREE.ConeGeometry(0.075, h, 14); g.rotateX(-Math.PI / 2); g.translate(0, 0, -h / 2); const o = withOutline(new THREE.Mesh(g, flameMat(PLASMA, '#ffffff')), 0.006); o.position.set(sd * 0.2, Y(300), BACK + 0.04); o.visible = false; body.add(o); return o; });
    const jetPos = sd => new THREE.Vector3(sd * 0.2, Y(300), BACK - 0.02);
    const thruster = pw => { for (const sd of [-1, 1]) { const p = jetPos(sd).add(off); dot(p, new THREE.Vector3(sd * Math.random() * 0.3, -(0.4 + Math.random() * 0.6) * pw, -(1.2 + Math.random() * 1.6) * pw), 0.024 + Math.random() * 0.02, Math.random() < 0.5 ? PLASMA : ORANGE, 0.3, 0, 0.9); if (Math.random() < 0.4) puff(p.clone().setY(p.y - 0.1), new THREE.Vector3(0, -0.5 * Math.random(), -1.2 * Math.random()), 0.05, 0.7, 0.9); } };
    // assinatura: a banana (fruta que encolhe a cada mordida, casca em três tiras que sobra na mão e voa)
    const bananaG = new THREE.Group(); bananaG.visible = false; root.add(bananaG);
    const fruitP = new THREE.Group(); bananaG.add(fruitP);
    { const cv = new THREE.CatmullRomCurve3([new THREE.Vector3(0, 0, 0), new THREE.Vector3(-0.035, 0.1, 0), new THREE.Vector3(-0.025, 0.2, 0), new THREE.Vector3(0.03, 0.28, 0)]), g = new THREE.TubeGeometry(cv, 18, 0.036, 10, false), m = flat('#fde047'); fruitP.add(withOutline(new THREE.Mesh(g, m), 0.006)); const cap = new THREE.SphereGeometry(0.036, 10, 8); cap.translate(0.03, 0.28, 0); fruitP.add(withOutline(new THREE.Mesh(cap, m), 0.006)); }
    const peel = new THREE.Group(); root.add(peel); peel.visible = false; bananaG.add(peel);
    for (const [dx, dz] of [[-1, 0.2], [1, 0.25], [0.1, -1]]) { const cv = new THREE.CatmullRomCurve3([new THREE.Vector3(0, 0.02, 0), new THREE.Vector3(dx * 0.05, 0.03, dz * 0.05), new THREE.Vector3(dx * 0.11, -0.04, dz * 0.11), new THREE.Vector3(dx * 0.14, -0.14, dz * 0.14)]); peel.add(withOutline(new THREE.Mesh(new THREE.TubeGeometry(cv, 12, 0.024, 8, false), flat('#facc15')), 0.006)); }
    { const g = new THREE.CylinderGeometry(0.03, 0.038, 0.05, 10); g.translate(0, -0.015, 0); peel.add(withOutline(new THREE.Mesh(g, flat('#78350f')), 0.005)); }
    const word = (txt, col) => { const c = document.createElement('canvas'); c.width = 400; c.height = 170; const g = c.getContext('2d'); g.font = '700 120px Bangers, Impact, sans-serif'; g.textAlign = 'center'; g.textBaseline = 'middle'; g.lineJoin = 'round'; g.lineWidth = 22; g.strokeStyle = '#0f1c10'; g.strokeText(txt, 200, 90); g.fillStyle = col; g.fillText(txt, 200, 90);
      const t = new THREE.CanvasTexture(c); t.colorSpace = THREE.SRGBColorSpace; const m = new THREE.Mesh(new THREE.PlaneGeometry(0.5, 0.21), new THREE.MeshBasicMaterial({ map: t, transparent: true, depthWrite: false })); m.visible = false; fxRoot.add(m); return m; };
    const nhac = word('NHAC!', YEL), hmm = word('HMM!', YEL);
    // a boca: o ponto na frente do focinho aonde a banana chega; e as bocas da mastigação (oh e dentes), sem mexer nos olhos
    const headBox = new THREE.Box3().setFromObject(head), mouth = new THREE.Object3D(); mouth.position.copy(head.worldToLocal(new THREE.Vector3(0, Y(194), headBox.max.z))); head.add(mouth);
    const chewOh = muzzleTex('ko'), chewGrit = muzzleTex('angry'), chewBase = muzzleMat.map;
    // escudo: bolha de plasma com a malha hexagonal pulsando
    const bubbleMat = new THREE.ShaderMaterial({ transparent: true, depthWrite: false, uniforms: { uT: { value: 0 }, uA: { value: 1 } },
      vertexShader: 'varying vec3 vN; varying vec3 vV; void main() { vec4 mv = modelViewMatrix * vec4(position, 1.0); vN = normalize(normalMatrix * normal); vV = normalize(-mv.xyz); gl_Position = projectionMatrix * mv; }',
      fragmentShader: `uniform float uT; uniform float uA; varying vec3 vN; varying vec3 vV;
        void main() { vec3 n = normalize(vN); float r = 1.0 - abs(dot(n, normalize(vV)));
          vec2 h = n.xy * 6.0, q = vec2(1.0, 1.732), a = mod(h, q) - q * 0.5, b = mod(h - q * 0.5, q) - q * 0.5, g = dot(a, a) < dot(b, b) ? a : b;
          float d = max(dot(abs(g), normalize(q)), abs(g.x)), hex = step(0.45, d) * (0.55 + 0.25 * sin(uT * 5.0));
          vec3 cA = vec3(0.22, 0.74, 0.97), cB = vec3(0.73, 0.90, 0.99);
          float al = mix(0.05, 0.28, smoothstep(0.1, 0.8, r)) + 0.42 * smoothstep(0.82, 0.95, r);
          gl_FragColor = vec4(mix(cA, cB, max(hex, smoothstep(0.8, 0.95, r))), max(al, hex * 0.7) * uA); }` });
    const bubble = new THREE.Mesh(new THREE.SphereGeometry(1.05, 40, 28), bubbleMat); bubble.position.y = 0.95; bubble.scale.z = 0.8; bubble.visible = false; root.add(bubble);
    // vidro: bolha de oxigênio e capacete, só o contorno de tinta e o brilho, o miolo transparente
    const glassMat = (ink, tint, tintA, rimAt) => new THREE.ShaderMaterial({ transparent: true, depthWrite: false, uniforms: { uI: { value: new THREE.Color(ink).convertLinearToSRGB() }, uC: { value: new THREE.Color(tint).convertLinearToSRGB() }, uA: { value: tintA }, uR: { value: rimAt } },
      vertexShader: 'varying vec3 vN; varying vec3 vV; void main() { vec4 mv = modelViewMatrix * vec4(position, 1.0); vN = normalize(normalMatrix * normal); vV = normalize(-mv.xyz); gl_Position = projectionMatrix * mv; }',
      fragmentShader: `uniform vec3 uI; uniform vec3 uC; uniform float uA; uniform float uR; varying vec3 vN; varying vec3 vV;
        void main() { vec3 n = normalize(vN); float r = 1.0 - abs(dot(n, normalize(vV)));
          float hi = step(0.62, r) * step(r, 0.74) * step(n.x, -0.3) * step(0.25, n.y);
          float cr = step(0.5, r) * step(0.45, n.x) * step(n.y, 0.5);
          vec4 col = vec4(uC, uA); if (cr > 0.5) col = vec4(1.0, 0.95, 0.72, max(uA, 0.32)); if (hi > 0.5) col = vec4(1.0, 1.0, 1.0, 0.85); if (r > uR) col = vec4(uI, 1.0);
          gl_FragColor = col; }` });
    const o2Mat = glassMat(O2, '#ffffff', 0.12, 0.72), bubG = new THREE.SphereGeometry(1, 14, 10), bubbles = Array.from({ length: 40 }, () => { const m = new THREE.Mesh(bubG, o2Mat); m.visible = false; fxRoot.add(m); return { m, life: 0, v: new THREE.Vector3() }; });
    let bubI = 0; const bubbleUp = (p, v, rr, life) => { const q = bubbles[bubI++ % bubbles.length]; q.m.position.copy(p); q.v.copy(v); q.r = rr; q.life = q.life0 = life; q.m.scale.setScalar(rr); q.m.visible = true; };
    // o Sputnik: esfera com a luz vermelha, dois painéis azuis e as quatro antenas; o feixe azul e a mira no chão
    const SKY = 2.55, LZ = 1.05, sat = new THREE.Group(); sat.position.set(0, SKY, LZ); sat.visible = false; fxRoot.add(sat);
    { sat.add(withOutline(new THREE.Mesh(new THREE.SphereGeometry(0.16, 20, 14), flat('#cbd5e1')), 0.008));
      const lamp = new THREE.Mesh(new THREE.SphereGeometry(0.05, 10, 8), new THREE.MeshBasicMaterial({ color: RED })); lamp.position.set(0, -0.05, 0.14); sat.add(lamp);
      for (const sd of [-1, 1]) { const pg = new THREE.BoxGeometry(0.34, 0.18, 0.02); pg.translate(sd * 0.34, 0, 0); sat.add(withOutline(new THREE.Mesh(pg, flat('#1e3a8a')), 0.006)); for (let i = 1; i < 3; i++) { const l = new THREE.Mesh(new THREE.BoxGeometry(0.008, 0.18, 0.024), new THREE.MeshBasicMaterial({ color: '#60a5fa' })); l.position.set(sd * (0.17 + i * 0.34 / 3), 0, 0); sat.add(l); } }
      for (let i = 0; i < 4; i++) { const a = -0.55 - i * 0.68, g = new THREE.CylinderGeometry(0.006, 0.006, 0.34, 6); g.translate(0, 0.17, 0); const m = new THREE.Mesh(g, flat('#0f1c10')); m.position.set(Math.cos(a) * 0.14, Math.sin(a) * 0.14, -0.04); m.rotation.z = a - Math.PI / 2; sat.add(m); } }
    const charge = new THREE.Mesh(new THREE.SphereGeometry(1, 16, 12), glow(PLASMA, 0.8, true)); charge.position.set(0, SKY - 0.1, LZ); charge.visible = false; fxRoot.add(charge);
    const beamParts = [[DEEP, 0.55, 1], [PLASMA, 0.8, 0.65], ['#ffffff', 0.7, 0.3]].map(([col, op, k]) => { const g = new THREE.CylinderGeometry(0.1 * k, 0.4 * k, SKY, 24, 1, true); g.translate(0, SKY / 2, 0); const m = new THREE.Mesh(g, glow(col, op, true)); m.position.set(0, 0, LZ); m.visible = false; fxRoot.add(m); return { m, op }; });
    const beamFloor = new THREE.Mesh(new THREE.CircleGeometry(0.62, 32), glow(ICE, 0.8, true)); beamFloor.rotation.x = -Math.PI / 2; beamFloor.position.set(0, 0.02, LZ); beamFloor.visible = false; fxRoot.add(beamFloor);
    const reticle = new THREE.Group(); reticle.position.set(0, 0.014, LZ); reticle.visible = false; fxRoot.add(reticle); const retMats = [glow('#0c4a6e', 1), glow(RED, 1)];
    retMats.forEach((mt, pass) => { const w = pass ? 0.018 : 0.036, y = pass ? 0.003 : 0; for (const R of [0.68, 0.37]) { const m = new THREE.Mesh(new THREE.RingGeometry(R - w, R + w, 48), mt); m.rotation.x = -Math.PI / 2; m.position.y = y; reticle.add(m); } for (let i = 0; i < 4; i++) { const g = new THREE.PlaneGeometry(0.34, w * 2); g.translate(0.68, 0, 0); const m = new THREE.Mesh(g, mt); m.rotation.set(-Math.PI / 2, 0, i * Math.PI / 2); m.position.y = y; reticle.add(m); } });
    // Gravidade Zero (sprite Zero): capacete de vidro com a antena da bolinha vermelha, a gola de metal e a mochila
    // de tanques nas costas; ele flutua
    const domeR = (headBox.max.y - headBox.min.y) * 0.56, domeY = (headBox.max.y + headBox.min.y) / 2 - 0.02, domeZ = (headBox.max.z + headBox.min.z) / 2;
    const zeroG = new THREE.Group(); zeroG.visible = false; neck.add(zeroG); zeroG.position.y = -NECK;
    { const dome = new THREE.Mesh(new THREE.SphereGeometry(domeR, 48, 32), glassMat('#0f1c10', '#e0f2fe', 0.1, 0.93)); dome.position.set(0, domeY, domeZ); dome.renderOrder = 5; zeroG.add(dome);
      const cy = domeY - domeR * 0.84, cg = new THREE.TorusGeometry(domeR * 0.68, D(14), 12, 44); cg.rotateX(Math.PI / 2); cg.scale(1, 0.7, 1); cg.translate(0, cy, domeZ); zeroG.add(withOutline(new THREE.Mesh(cg, flat('#cbd5e1')), 0.008));
      const a = 0.42, tip = new THREE.Vector3(Math.sin(a) * (domeR + 0.26), domeY + Math.cos(a) * (domeR + 0.26), domeZ), base = new THREE.Vector3(Math.sin(a) * domeR * 0.97, domeY + Math.cos(a) * domeR * 0.97, domeZ), ag = new THREE.CylinderGeometry(0.009, 0.009, tip.distanceTo(base), 8), an = new THREE.Mesh(ag, flat('#0f1c10')); an.position.copy(tip).add(base).multiplyScalar(0.5); an.quaternion.setFromUnitVectors(new THREE.Vector3(0, 1, 0), tip.clone().sub(base).normalize()); zeroG.add(an);
      const ball = new THREE.SphereGeometry(0.036, 12, 10); ball.translate(tip.x, tip.y, tip.z); zeroG.add(withOutline(new THREE.Mesh(ball, flat('#d90429')), 0.006)); }
    const packG = new THREE.Group(); packG.visible = false; body.add(packG);
    { const by = Y(262), bz = BACK - 0.05, fr = new THREE.Mesh(new RoundedBoxGeometry(0.5, 0.52, 0.09, 3, 0.03), flat('#94a3b8')); fr.position.set(0, by, bz); packG.add(ol(fr, 0.009));
      for (const sd of [-1, 1]) { const tg = new THREE.CapsuleGeometry(0.075, 0.26, 6, 16); const tk = new THREE.Mesh(tg, flat('#f1f5f9')); tk.position.set(sd * 0.115, by, bz - 0.09); packG.add(ol(tk, 0.008)); const st = new THREE.Mesh(new THREE.CylinderGeometry(0.077, 0.077, 0.03, 16), flat('#d90429')); st.position.set(sd * 0.115, by + 0.07, bz - 0.09); packG.add(st); } }
    let zeroOn = false; const setZero = on => { zeroOn = on; zeroG.visible = on; packG.visible = on; burst(new THREE.Vector3(0, domeY, 0.1), 14, 1, 0.016, [ICE, '#ffffff'], 0.6, 0); };
    const zeroB = document.querySelector('#zero'); if (zeroB) zeroB.onclick = () => { setZero(!zeroOn); zeroB.style.outline = zeroOn ? '2px solid #fc6' : ''; };
    const skills = {
      // assinatura (laboratório): agacha, sobe nas chamas das botas, pega a banana, três mordidas (NHAC!), HMM!, joga a casca no chão
      signature: { dur: 4.4, start: () => { FX.sign = now; },
        pose: (k, pose) => { const p = k / 4.4, rise = ss(0.06, 0.2, p) * (1 - ss(0.88, 0.98, p)), cr = Math.sin(win(0, 0.08, p) * Math.PI); pose[-1] = [0.03 * rise, 0.1 * rise, -0.12 * rise, 0, 0]; return { drop: 0.07 * cr, lean: 0.04 * rise }; } },
      // arrancada a jato: agacha, dispara com as turbinas e o soco direito estica; o corpo avança
      rocket: { dur: 1.1, start: () => { FX.rocket = now; setFace('angry'); after(1.1, () => setFace('normal')); after(0.55, () => { const p = new THREE.Vector3(0, CHEST, DIST - 0.15); burst(p, 18, 2, 0.022, [ORANGE, PLASMA, '#ffffff'], 0.5); ring(p, 0.34, ORANGE, 0.3); }); },
        pose: (k, pose) => { const p = k / 1.1, cr = ss(0, 0.3, p), d = Math.sin(win(0.3, 1, p) * Math.PI); pose[1] = [-0.09 * cr * (1 - d) + 0.72 * d, -0.04 * cr * (1 - d) + 0.06 * d, 0.3 * d, 0, 0]; pose[-1] = [0, 0, -0.1, 0, 0]; return { drop: 0.13 * cr * (1 - d), lean: 0.3 * d, twist: -0.25 * d }; } },
      // raio orbital: ergue o braço chamando o satélite e baixa no disparo; a mira gira no chão e o feixe desce
      laser: { dur: 1.7, start: () => { FX.laser = now; setFace('angry'); after(1.7, () => setFace('normal')); after(0.935, () => { const p = new THREE.Vector3(0, 0.05, LZ); burst(p, 22, 2.2, 0.02, [ICE, PLASMA, '#ffffff'], 0.6); ring(p, 1, PLASMA, 0.45, true); }); },
        pose: (k, pose) => { const p = k / 1.7, up = ss(0, 0.3, p), fi = ss(0.55, 0.65, p), e = 1 - ss(0.9, 1, p); pose[1] = [0.18 * fi * e, (0.72 * up * (1 - fi) + 0.2 * fi) * e, -0.38 * up * (1 - fi) * e, 0, 0]; pose[-1] = [0.06 * e, 0.07 * e, -0.05 * e, 0, 0]; return { lean: (-0.1 * up + 0.15 * fi) * e }; } },
      // escudo Sputnik: mãos à frente e a bolha fica 6 s
      plasma: { dur: 1.0, start: () => { FX.plasma = now; },
        pose: (k, pose) => { const q = Math.sin(k * Math.PI), v = [0.06 + 0.23 * q, 0.07, -0.08 - 0.08 * q, 0.6 * q, 0]; pose[-1] = v; pose[1] = [...v]; return {}; } },
      // impulso de reação: puxa o punho para trás e solta com o rastro de fogo na luva
      kinetic: { dur: 0.9, start: () => { FX.kinetic = now; setFace('angry'); after(0.9, () => setFace('normal')); after(0.36, () => { const p = new THREE.Vector3(0, CHEST, DIST - 0.2); burst(p, 16, 1.8, 0.022, [ORANGE, PLASMA], 0.5); ring(p, 0.28, ORANGE, 0.28); }); },
        pose: (k, pose) => { const p = k / 0.9, pu = ss(0, 0.3, p), q = Math.sin(win(0.3, 1, p) * Math.PI); pose[1] = [-0.14 * pu * (1 - q) + 0.77 * q, 0.05 * q, 0.1 * q, 0, 0]; pose[-1] = [0, 0, -0.1, 0, 0]; return { lean: -0.2 * pu * (1 - q) + 0.35 * q, twist: 0.3 * pu * (1 - q) - 0.3 * q }; } },
      // fumaça de exaustão: mãos para baixo e para trás, as turbinas sopram a fumaça até o rival e a nuvem fecha nele
      exhaust: { dur: 1.3, start: () => { FX.exhaust = now; setFace('angry'); after(1.3, () => setFace('normal')); after(0.52, () => { for (let i = 0; i < 16; i++) puff(new THREE.Vector3((Math.random() - 0.5) * 0.75, 0.3 + Math.random() * 1, DIST + (Math.random() - 0.5) * 0.45), new THREE.Vector3(0, Math.random() * 0.25, 0), 0.16 + Math.random() * 0.1, 1.5, 0.97, 0.14); }); },
        pose: (k, pose) => { const p = k / 1.3, q = Math.sin(p * Math.PI), v = [-0.05 * q, -0.14 * q, -0.32 * q, 0, 0]; pose[-1] = v; pose[1] = [...v]; return { lean: 0.2 * Math.sin(p * 6.283), drop: 0.08 * q }; } },
      // célula de oxigênio: mãos no peito, bolhas subindo e três anéis
      o2: { dur: 1.5, start: () => { FX.o2 = now; after(0.75, () => { for (let i = 0; i < 3; i++) ring(new THREE.Vector3(0, 0.58 + i * 0.15, 0), 0.78 + i * 0.15, O2, 0.6 + i * 0.05, true); }); },
        pose: (k, pose) => { const p = k / 1.5, e = ss(0, 0.15, p) * (1 - ss(0.85, 1, p)), v = [0.08 * e, (0.16 + 0.03 * Math.sin(p * 12.566)) * e, 0.62 * e, 0, 0]; pose[-1] = v; pose[1] = [...v]; return {}; } },
    };
    const fxFrame = c => {
      now = c.t; const t = now, dt = c.dt, cam = window.VIN?.camera;
      for (let i = queue.length - 1; i >= 0; i--) if (now >= queue[i].at) { queue[i].fn(); queue.splice(i, 1); }
      // deslocamentos do corpo neste quadro
      off.set(0, 0, 0);
      if (zeroOn) off.y += 0.1 + 0.035 * Math.sin(t * 2);
      const pS = (t - FX.sign) / 4.4, sOn = pS > 0 && pS < 1, riseS = sOn ? ss(0.06, 0.2, pS) * (1 - ss(0.88, 0.98, pS)) : 0;
      off.y += 0.27 * riseS + 0.03 * Math.sin(t * 3) * riseS;
      const pR = (t - FX.rocket) / 1.1, dR = pR > 0 && pR < 1 ? Math.sin(win(0.3, 1, pR) * Math.PI) : 0; off.z += 0.58 * dR;
      const pK = (t - FX.kinetic) / 0.9, dK = pK > 0 && pK < 1 ? Math.sin(win(0.3, 1, pK) * Math.PI) : 0; off.z += 0.25 * dK;
      const pL = (t - FX.laser) / 1.7, lOn = pL > 0 && pL < 1; if (lOn) off.y += 0.08 * ss(0, 0.3, pL) * (1 - ss(0.55, 0.65, pL));
      const pO = (t - FX.o2) / 1.5; if (pO > 0 && pO < 1) off.y += 0.05 * Math.sin(pO * 6.283);
      root.position.add(off); fxRoot.position.copy(off).negate();
      for (const q of dots) { if (q.life <= 0) { q.m.visible = false; continue; } q.life -= dt; q.v.y -= q.g * dt; q.m.position.addScaledVector(q.v, dt); const u = q.life / q.life0; if (q.grow) { q.r += q.grow * dt; q.m.scale.setScalar(q.r); q.m.material.opacity = q.op * Math.min(1, u * 3.5); } else q.m.scale.setScalar(q.r * Math.max(0.05, u)); if (q.m.position.y < 0.01) q.life = 0; }
      for (const q of rings) { if (q.life <= 0) { q.m.visible = false; continue; } q.life -= dt; const u = 1 - q.life / q.life0, e = q.R * easeO(u) + 0.03; q.m.scale.set(e, e, 1); q.m.material.opacity = Math.max(0, 1 - u); if (cam && !q.flat) q.m.quaternion.copy(cam.quaternion); }
      for (const q of segs) { if (q.life <= 0) { q.m.visible = false; continue; } q.life -= dt; q.m.position.addScaledVector(q.v, dt); q.m.material.opacity = Math.max(0, q.life / q.life0); }
      for (const q of bubbles) { if (q.life <= 0) { q.m.visible = false; continue; } q.life -= dt; q.m.position.addScaledVector(q.v, dt); q.m.position.x += Math.sin(t * 5 + q.r * 90) * 0.003; q.m.scale.setScalar(q.r * Math.min(1, q.life / q.life0 * 5)); }
      // turbinas nas costas (foguete até 0,75, exaustão até a metade)
      const pE = (t - FX.exhaust) / 1.3, eOn = pE > 0 && pE < 0.5, rOn = pR > 0 && pR < 0.75 && pR > 0.28, jp = rOn ? 1 : eOn ? 0.7 : 0;
      jetFl.forEach((o, i) => { o.visible = jp > 0; if (jp > 0) { const f = jp * (0.8 + 0.25 * Math.sin(t * 40 + i * 2)); o.scale.set(0.8 + 0.3 * jp, 0.8 + 0.3 * jp, f); } });
      if (jp > 0) thruster(jp);
      if (rOn && pR > 0.3) { for (let i = 0; i < 2; i++) { const a = new THREE.Vector3((Math.random() - 0.5) * 1.1, 0.3 + Math.random() * 1.3, off.z - 0.3 - Math.random() * 0.6); seg(a, a.clone().add(new THREE.Vector3(0, 0, -0.5 - Math.random() * 0.5)), 0.012, '#ffffff', 0.18, new THREE.Vector3(0, 0, -2)); } }
      // fumaça da exaustão a caminho do rival
      if (eOn) for (let i = 0; i < 2; i++) puff(new THREE.Vector3((Math.random() - 0.5) * 0.4, 0.45 + Math.random() * 0.5, 0.25), new THREE.Vector3((Math.random() - 0.5) * 0.3, (Math.random() - 0.4) * 0.3, 2.6 + Math.random()), 0.09 + Math.random() * 0.05, 0.85, 0.97, 0.22);
      // rastro de fogo na luva do impulso
      if (pK > 0 && pK < 0.6) { const g = P.gloves[1].position; for (let i = 0; i < 3; i++) dot(new THREE.Vector3(g.x + (Math.random() - 0.5) * 0.1, g.y + (Math.random() - 0.5) * 0.1, g.z - 0.08).add(off), new THREE.Vector3(0, (Math.random() - 0.5) * 0.5, -(1.5 + Math.random() * 2)), 0.024 + Math.random() * 0.024, Math.random() < 0.5 ? ORANGE : PLASMA, 0.28, 0, 0.95); }
      // satélite, carga, feixe e mira
      { const ch = lOn && pL < 0.55 ? ss(0, 0.2, pL) : 0, be = lOn ? ss(0.55, 0.58, pL) * (1 - ss(0.82, 0.85, pL)) : 0, sh = lOn ? ss(0, 0.12, pL) * (1 - ss(0.9, 1, pL)) : 0;
        let topY = SKY; if (sh > 0.01 && cam) { fxRoot.updateWorldMatrix(true, false); let lo = 1.5, hi = SKY; const pv = new THREE.Vector3(); for (let i = 0; i < 10; i++) { const y = (lo + hi) / 2; pv.set(0, y, LZ); fxRoot.localToWorld(pv).project(cam); if (pv.y > 0.68) hi = y; else lo = y; } topY = lo; }
        sat.visible = sh > 0.01; if (sat.visible) { sat.position.y = topY + (1 - sh) * 1.2; sat.rotation.z = Math.sin(t * 2) * 0.2; }
        charge.position.y = topY - 0.1;
        charge.visible = ch > 0.05; if (charge.visible) { charge.scale.setScalar(0.05 + 0.1 * ch + Math.sin(t * 30) * 0.015); charge.material.opacity = 0.8 * ch; }
        for (const q of beamParts) { q.m.visible = be > 0.03; if (q.m.visible) { const w = be * (1 + Math.sin(t * 50) * 0.08); q.m.scale.set(w, topY / SKY, w); q.m.material.opacity = q.op * be; } }
        beamFloor.visible = be > 0.03; if (beamFloor.visible) { beamFloor.scale.setScalar(be); beamFloor.material.opacity = 0.8 * be; if (Math.random() < 0.8) { const a = Math.random() * 6.283, sp = 1 + Math.random() * 1.2; dot(new THREE.Vector3(Math.cos(a) * 0.3, 0.05, LZ + Math.sin(a) * 0.3), new THREE.Vector3(Math.cos(a) * sp, 1 + Math.random() * 1.5, Math.sin(a) * sp), 0.016, Math.random() < 0.5 ? ICE : '#ffffff', 0.45, 4); } }
        const ra = Math.max(ch, be); reticle.visible = ra > 0.03; if (reticle.visible) { reticle.rotation.y = t * 3; reticle.scale.setScalar(1 - 0.36 * ch); retMats[0].opacity = ra; retMats[1].opacity = ra; } }
      // escudo: 6 s, com os raios correndo na bolha
      { const v = t - FX.plasma, a = v > 0 ? ss(0, 0.35, v) * (1 - ss(5.6, 6, v)) : 0; bubble.visible = a > 0.01; if (bubble.visible) { bubbleMat.uniforms.uT.value = t; bubbleMat.uniforms.uA.value = a; bubble.scale.setScalar(0.85 + 0.15 * a); bubble.scale.z *= 0.8; if (Math.random() < 0.12) { const an = (Math.random() - 0.5) * 2.2, y = 0.5 + Math.random() * 1, pos = aa => new THREE.Vector3(Math.sin(aa) * 0.93, y + (Math.random() - 0.5) * 0.3, Math.cos(aa) * 0.74).add(off); bolt(pos(an), pos(an + 0.5), 0.012, ICE, 0.16); } } }
      // oxigênio
      if (pO > 0 && pO < 1 && Math.random() < 0.6) bubbleUp(new THREE.Vector3((Math.random() - 0.5) * 0.7, 0.3 + Math.random() * 0.6, 0.2 + (Math.random() - 0.5) * 0.4), new THREE.Vector3((Math.random() - 0.5) * 0.15, 0.6 + Math.random() * 0.7, 0), 0.04 + Math.random() * 0.045, 1.1);
      // assinatura: as chamas, a banana na mão, as mordidas e a casca
      const fla = sOn && pS > 0.04 && pS < 0.98 ? riseS + 0.3 : 0;
      bootFlames.forEach((o, i) => { o.visible = fla > 0; if (fla > 0) { const f = 0.8 + 0.25 * Math.sin(t * 40 + i * 2.2); o.scale.set(1, fla * f, 1); } });
      nhac.visible = false; hmm.visible = false; let chew = 0;
      if (sOn) { const G = P.gloves[1], rest = hands[1].position, grab = ss(0.2, 0.3, pS), toM = ss(0.32, 0.4, pS), toss = win(0.74, 0.86, pS), back = ss(0.86, 0.96, pS), bites = [[0.4, 0.48], [0.5, 0.58], [0.6, 0.68]];
        let eaten = 0; for (const [a, b] of bites) { if (pS >= b) eaten++; else if (pS > a) chew = win(a, b, pS); }
        const left = Math.max(0, 1 - eaten / 3 - (chew > 0.4 ? 0.1 : 0));
        // a mastigação puxa a cabeça e reveza a boca
        if (chew > 0) { neck.rotation.x += 0.09 * Math.sin(chew * Math.PI * 4); muzzleMat.map = Math.sin(chew * Math.PI * 4) > 0 ? chewOh : chewGrit; } else if (muzzleMat.map === chewOh || muzzleMat.map === chewGrit) muzzleMat.map = chewBase;
        root.updateMatrixWorld(true); const mp = root.worldToLocal(mouth.getWorldPosition(new THREE.Vector3()));
        // a mão acompanha: a ponta da banana fica na boca, e sobe conforme a banana encurta
        const hold = new THREE.Vector3(mp.x + 0.02, mp.y - 0.1 - 0.4 * left, mp.z + 0.13), fetch = new THREE.Vector3(0.6, 0.75, 0.42), thr = new THREE.Vector3(-0.28, 1.0, 0.5), tgt = new THREE.Vector3();
        if (pS < 0.74) tgt.lerpVectors(fetch, hold, toM); else tgt.lerpVectors(hold, thr, easeO(toss));
        G.position.lerpVectors(rest, tgt, grab * (1 - back));
        const tossed = pS > 0.74, showB = pS > 0.2 && !tossed; bananaG.visible = showB;
        if (showB) { bananaG.position.set(G.position.x, G.position.y + 0.13, G.position.z); bananaG.rotation.set(-0.42 * toM, 0, -0.2 * (1 - toM)); bananaG.scale.setScalar(1.4 * Math.min(1, win(0.2, 0.25, pS) * 1.5 + 0.2)); fruitP.visible = left > 0.02; fruitP.scale.y = Math.max(0.02, left); if (peel.parent !== bananaG) bananaG.add(peel); peel.position.set(0, 0, 0); peel.rotation.set(0, 0, 0); peel.scale.setScalar(1); peel.visible = true; }
        if (tossed) { if (peel.parent !== fxRoot) { fxRoot.add(peel); } const x0 = hold.x, y0 = hold.y + off.y + 0.05, z0 = hold.z, u = toss; peel.visible = pS < 0.985; peel.position.set(x0 + (-0.82 - x0) * u, y0 + (0.145 - y0) * u + Math.sin(u * Math.PI) * 0.34, z0 + (0.62 - z0) * u); peel.rotation.set(0, 0, (1 - u) * -9); peel.scale.setScalar(pS > 0.95 ? Math.max(0.02, 1 - win(0.95, 0.985, pS)) : 1); }
        const fw = (m, x, y, z, k) => { m.visible = true; m.position.set(x, y + off.y, z); m.scale.setScalar(k); if (cam) m.quaternion.copy(cam.quaternion); };
        if (chew > 0.15 && chew < 0.85) fw(nhac, 0.55, 1.15, -0.45, 1.1 + 0.3 * Math.sin(chew * Math.PI));
        if (pS > 0.68 && pS < 0.74) fw(hmm, -0.62, 1.2, 0.5, 1.15);
      } else { if (peel.visible && peel.parent === fxRoot) peel.visible = false; if (bananaG.visible) bananaG.visible = false; if (muzzleMat.map === chewOh || muzzleMat.map === chewGrit) muzzleMat.map = chewBase; }
    };
    P.tailMesh = tailMesh;   // tools3d/export
    const onFrame = c => {
      furStep(c); waveTail(c.t);
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
    P.cloth = [{ geos: P.clothGeos, top: Y(262), bot: Y(394), pow: 1.4, gain: 1 }];   // tools3d/export
    return { P, upper, WAIST, neck, NECK, hips, HIPS, hands, hipSway: 0.6, hand: { x: 0.5, y: Y(262), z: 0.44, yaw: -0.12 }, placeFoot, setFace, skills, onFrame };
  },
};
