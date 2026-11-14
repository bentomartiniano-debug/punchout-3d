// O Cremoso no modo vinil, modelado pela folha de modelo do usuário (assets3d/cremoso/ref/cremoso_turnaround.png:
// frente, perfil esquerdo, costas e perfil direito). Medidas em px DA FOLHA: x e y da vista de frente, z = x do perfil
// − 592 (a frente para +z). Escala 287 px = 1 m (a altura do boneco do jogo); chão em y = 729, eixo em x = 241.
//  - chapéu de chef: dois gomos de pano por cima de uma faixa branca contornada
//  - cabeça de creme (bloco redondo) com as marcas mais escuras do creme; sobrancelhas grossas marrons em relevo,
//    olhos escuros de canto redondo, bigode de guidão em relevo e o tufo de creme embaixo
//  - corpo em sino com as marcas do creme; avental branco contornado com o bolso e as duas bolinhas vermelhas
//  - base de calda dourada no lugar das pernas
//  - mãos de creme segurando a bandeja verde de três copinhos com as gotas vermelhas e as chamas de creme
import { THREE, ss, INKS, flat, withOutline } from './core.js';
import { TessellateModifier } from 'three/addons/modifiers/TessellateModifier.js';
import { mergeVertices } from 'three/addons/utils/BufferGeometryUtils.js';

const S_PX = 287, FOOT = 729, CX = 241;
const C = { cream: '#fde25e', streak: '#efb51c', brow: '#a86a12', eye: '#2e2218', stache: '#9a5f14', white: '#f7f5f3', hat: '#f4f0ee',
            line: '#2d251e', red: '#f2141f', base: '#f3bd1c', hand: '#fbd85a', cup: '#1f5a2b', drop: '#e8101c' };
const col = h => ({ value: new THREE.Color(h) });

export default {
  title: 'O Cremoso',
  arena: 'cremoso',   // abre na arena própria (core.js)
  scale: 1.18,   // grandalhão: maior que os demais lutadores
  units: [S_PX, FOOT, CX],
  faces: [['normal', 'Normal'], ['angry', 'Raiva'], ['pain', 'Dor'], ['ko', 'K.O.']],
  skills: [['signature', 'Assinatura'], ['syrup', 'Calda Quente'], ['whip', 'Tornado de Baunilha'], ['melt', 'Modo Derretido'], ['cone', 'Casquinha'], ['puddle', 'Poça de Calda'], ['cherry', 'Cereja do Bolo']],
  hudExtra: 'Sabor: <button data-flavor="vanilla">Baunilha</button><button data-flavor="frozen">Congelado</button><button data-flavor="revenge">Morango</button>',
  others: [['valeria', 'Valéria'], ['frog', 'King Frog'], ['joana', 'Joana'], ['marcov', 'Marcov'], ['espinha', 'Espinha'], ['vivaldi', 'Vivaldi']],
  async build({ root, U }) {
    const { X, Y, D } = U, P = {}, streakU = col(C.streak);
    const V = (x, y, z) => new THREE.Vector3(X(x), Y(y), D(z));
    const ol = (m, w = 0.009) => {
      const g = withOutline(m, w); g.position.copy(m.position); g.quaternion.copy(m.quaternion); g.scale.copy(m.scale);
      m.position.set(0, 0, 0); m.quaternion.identity(); m.scale.set(1, 1, 1); return g; };
    function pattern(mat, glsl, key, uniforms = {}, off = [CX, FOOT]) {   // padrão em px da folha (vPx)
      const prev = mat.onBeforeCompile, k0 = mat.customProgramCacheKey();
      mat.onBeforeCompile = (sh, r) => {
        prev.call(mat, sh, r); Object.assign(sh.uniforms, uniforms);
        sh.vertexShader = 'varying vec3 vPx;\n' + sh.vertexShader.replace('#include <begin_vertex>', `#include <begin_vertex>
          vPx = vec3(position.x * ${S_PX.toFixed(1)} + ${off[0].toFixed(1)}, ${off[1].toFixed(1)} - position.y * ${S_PX.toFixed(1)}, position.z * ${S_PX.toFixed(1)});`);
        sh.fragmentShader = 'varying vec3 vPx;\n' + Object.entries(uniforms).map(([u, v]) => `uniform ${v.value && v.value.isTexture ? 'sampler2D' : 'vec3'} ${u};\n`).join('')
          + sh.fragmentShader.replace('#include <map_fragment>', '#include <map_fragment>\n' + glsl);
      };
      mat.customProgramCacheKey = () => k0 + '|' + key; return mat;
    }
    // sólido por seções { y, pt(th) -> [x, z] } (th = 0 → +x, π/2 → frente), de cima para baixo
    function sections(rows, NA = 72) {
      const pos = [], idx = [];
      for (const r of rows) for (let j = 0; j <= NA; j++) { const [x, z] = r.pt(j / NA * Math.PI * 2); pos.push(X(x), Y(r.y), D(z)); }
      for (let i = 0; i < rows.length - 1; i++) for (let j = 0; j < NA; j++) { const a = i * (NA + 1) + j, b = a + 1, c = a + NA + 1, d = c + 1; idx.push(a, b, c, b, d, c); }
      const g = new THREE.BufferGeometry(); g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3)); g.setIndex(idx); g.computeVertexNormals(); return g;
    }
    const sp = (v, pw) => Math.sign(v) * Math.pow(Math.abs(v), 2 / pw);
    const oval = (cx, a, zf, zb, pw = 2) => th => { const c = Math.cos(th), s = Math.sin(th); return [cx + a * sp(c, pw), (s >= 0 ? zf : zb) * sp(s, pw)]; };
    const lerpT = (tab, y) => {
      if (y <= tab[0][0]) return tab[0].slice(1); const L = tab[tab.length - 1]; if (y >= L[0]) return L.slice(1);
      for (let i = 1; i < tab.length; i++) if (y <= tab[i][0]) { const [a, b] = [tab[i - 1], tab[i]], k = (y - a[0]) / (b[0] - a[0]); return a.slice(1).map((v, j) => v + (b[j + 1] - v) * k); } };
    // z da frente de uma seção oval no x pedido
    const frontZ = (tab, pw, x, y) => { const [a, zf] = lerpT(tab, y), c = Math.max(-1, Math.min(1, (x - CX) / a)), cc = Math.sign(c) * Math.pow(Math.abs(c), pw / 2); return zf * sp(Math.sqrt(Math.max(0, 1 - cc * cc)), pw); };
    const tubeVar = (pts, rFn, NA = 14) => {   // tubo de raio variável (u de 0 a 1 ao longo)
      const c = new THREE.CatmullRomCurve3(pts.map(p => V(...p))), NS = 64, pos = [], idx = [], fr = c.computeFrenetFrames(NS, false);
      for (let i = 0; i <= NS; i++) { const u = i / NS, p = c.getPointAt(u), n = fr.normals[i], b = fr.binormals[i], r = D(rFn(u));
        for (let j = 0; j <= NA; j++) { const a = j / NA * Math.PI * 2; pos.push(p.x + (n.x * Math.cos(a) + b.x * Math.sin(a)) * r, p.y + (n.y * Math.cos(a) + b.y * Math.sin(a)) * r, p.z + (n.z * Math.cos(a) + b.z * Math.sin(a)) * r); } }
      for (let i = 0; i < NS; i++) for (let j = 0; j < NA; j++) { const a = i * (NA + 1) + j, b = a + 1, cc = a + NA + 1, d = cc + 1; idx.push(a, b, cc, b, d, cc); }
      const g = new THREE.BufferGeometry(); g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3)); g.setIndex(idx); g.computeVertexNormals(); return g;
    };
    // placa fina assentada numa superfície: contorno em (x, y) px, z = surf(x, y) + lift
    const plate = (pts, surf, lift, depth, mat) => {
      const sh = new THREE.Shape(pts.map(([x, y]) => new THREE.Vector2(x, y)));
      let g = new THREE.ExtrudeGeometry(sh, { depth, bevelEnabled: false, curveSegments: 8 });
      g = new TessellateModifier(3, 8).modify(g);
      const p = g.attributes.position;
      for (let i = 0; i < p.count; i++) { const x = p.getX(i), y = p.getY(i), z = p.getZ(i); p.setXYZ(i, X(x), Y(y), D(surf(x, y) + lift + z)); }
      g.deleteAttribute('normal'); g.deleteAttribute('uv'); g = mergeVertices(g, 1e-6);
      g.index.array.reverse();   // o y da forma vira −y no mundo: desvira as faces
      g.computeVertexNormals(); mat.side = THREE.DoubleSide;
      return new THREE.Mesh(g, mat);
    };
    const rrPts = (x0, y0, x1, y1, r, n = 6) => { const out = [], arc = (cx, cy, a0) => { for (let k = 0; k <= n; k++) { const a = a0 + k / n * Math.PI / 2; out.push([cx + r * Math.cos(a), cy + r * Math.sin(a)]); } };
      arc(x1 - r, y1 - r, 0); arc(x0 + r, y1 - r, Math.PI / 2); arc(x0 + r, y0 + r, Math.PI); arc(x1 - r, y0 + r, Math.PI * 1.5); return out; };
    // marcas do creme: riscos curtos mais escuros em volta (ângulo em graus, 90 = frente), fora do avental
    // marcas do creme (como na folha: poucas, grossas, pontas finas, alturas variadas), fora do avental:
    // [ângulo em graus (90 = frente, 270 = costas), y, meio comprimento em graus, meia espessura px]
    const STREAKS = [[150, 404, 16, 6], [30, 404, 16, 6], [160, 438, 14, 6], [20, 438, 14, 6], [205, 420, 16, 6.5], [330, 442, 18, 6.5], [258, 398, 12, 5],
      [148, 556, 18, 7], [32, 556, 18, 7], [160, 602, 22, 7.5], [20, 600, 22, 7.5], [140, 650, 18, 7], [40, 650, 18, 7],
      [222, 548, 16, 7], [300, 556, 18, 7], [195, 600, 14, 7], [262, 606, 22, 7.5], [338, 596, 14, 7], [236, 652, 20, 7.5], [310, 648, 16, 7]];
    const streakGlsl = (zc) => `{ float th = degrees(atan(vPx.z - ${zc.toFixed(1)}, vPx.x - ${CX.toFixed(1)})); if (th < 0.0) th += 360.0; float m = 1e3;
      ${STREAKS.map(([a, y, h, t]) => `{ float da = (mod(th - ${a.toFixed(1)} + 540.0, 360.0) - 180.0) * 2.4, u = clamp(da / ${(h * 2.4).toFixed(1)}, -1.0, 1.0), dy = vPx.y - ${y.toFixed(1)} - 3.0 * u * u;
        m = min(m, length(vec2(max(abs(da) - ${(h * 2.4).toFixed(1)}, 0.0), dy)) - ${t.toFixed(1)} * (1.0 - 0.55 * u * u)); }`).join(' ')}
      float w = fwidth(m); diffuseColor.rgb = mix(diffuseColor.rgb, uStreak, 1.0 - smoothstep(-w, w, m)); }`;

    // ================= grupos do motor: o corpo inteiro gira na base (cintura embaixo do sino); cabeça no pescoço
    const WAIST = Y(690), NECK = Y(497), HIPS = Y(704);
    const upper = new THREE.Group(); upper.position.y = WAIST; root.add(upper);
    const body = new THREE.Group(); body.position.y = -WAIST; upper.add(body);
    const neck = new THREE.Group(); neck.position.y = NECK - WAIST; upper.add(neck);
    const head = new THREE.Group(); head.position.y = -NECK; neck.add(head);

    // ================= corpo em sino (frente da folha: meia largura; perfil: frente e costas)
    const BODY = [[494, 100, 76, 74], [500, 116, 80, 86], [510, 128, 82, 96], [524, 140, 86, 103], [545, 150, 95, 113], [570, 152, 104, 119], [600, 156, 106, 123], [630, 157, 104, 123], [660, 156, 100, 119], [680, 153, 96, 115], [694, 150, 90, 110]];
    const BPW = 2.05;
    { const rows = []; for (let y = 494; y <= 694; y += 2) rows.push({ y, pt: th => { const [a, zf, zb] = lerpT(BODY, y); return oval(CX, a, zf, zb, BPW)(th); } });
      const m = pattern(flat(C.cream), streakGlsl(-14), 'crBody', { uStreak: streakU });
      P.bodyOL = withOutline(new THREE.Mesh(sections(rows, 288), m), 0.009); body.add(P.bodyOL); }   // malha fina: as gotas são a própria pele
    // avental: placa branca colada na frente do sino (contorno escuro), bolso com as duas bolinhas vermelhas
    const bodyZ = (x, y) => frontZ(BODY, BPW, x, y);
    { const half = y => 74 + 11 * Math.min(1, (y - 498) / 189), pts = [];
      for (let y = 498; y <= 672; y += 6) pts.push([CX - half(y), y]);
      for (let k = 0; k <= 6; k++) { const a = Math.PI - k / 6 * Math.PI / 2; pts.push([CX - half(687) + 15 + 15 * Math.cos(a), 672 + 15 * Math.sin(a)]); }
      for (let k = 0; k <= 6; k++) { const a = Math.PI / 2 - k / 6 * Math.PI / 2; pts.push([CX + half(687) - 15 + 15 * Math.cos(a), 672 + 15 * Math.sin(a)]); }
      for (let y = 672; y >= 498; y -= 6) pts.push([CX + half(y), y]);
      const m = pattern(flat(C.white), `{ float e = min(min(vPx.x - (${CX.toFixed(1)} - (74.0 + 11.0 * clamp((vPx.y - 498.0) / 189.0, 0.0, 1.0))), (${CX.toFixed(1)} + (74.0 + 11.0 * clamp((vPx.y - 498.0) / 189.0, 0.0, 1.0))) - vPx.x), 687.0 - vPx.y);
        if (e < 4.5) diffuseColor.rgb = uLine; }`, 'apron', { uLine: col(C.line) });
      body.add(withOutline(plate(pts.reverse(), bodyZ, 1.5, 3, m), 0.004));
      const pk = rrPts(187, 560, 300, 650, 12), pm = pattern(flat(C.white), `{ float e = min(min(vPx.x - 187.0, 300.0 - vPx.x), min(vPx.y - 560.0, 650.0 - vPx.y)); if (e < 4.5) diffuseColor.rgb = uLine; }`, 'pocket', { uLine: col(C.line) });
      body.add(withOutline(plate(pk.reverse(), bodyZ, 5, 8, pm), 0.004));
      for (const [x, y, r] of [[226, 607, 15], [257, 617, 9]]) { const d = new THREE.Mesh(new THREE.SphereGeometry(D(r), 20, 14), flat(C.red)); d.scale.z = 0.45; d.position.copy(V(x, y, bodyZ(x, y) + 14)); body.add(ol(d, 0.004)); } }
    // costura entre a cabeça e o corpo
    { const pts = []; for (let k = 0; k <= 48; k++) { const th = k / 48 * Math.PI * 2, [x, z] = oval(CX, 112, 90, 84, BPW)(th); pts.push(V(x, 497, z)); }
      body.add(new THREE.Mesh(new THREE.TubeGeometry(new THREE.CatmullRomCurve3(pts, true), 96, D(2.2), 6, true), flat(INKS))); }

    // ================= cabeça (bloco redondo de creme)
    const HEAD = [[354, 70, 66, 56], [360, 90, 80, 68], [368, 102, 89, 75], [380, 109, 94, 80], [392, 113, 97, 83], [420, 114, 98, 84], [470, 114, 98, 84], [484, 111, 96, 82], [494, 104, 90, 78], [502, 92, 80, 68]];
    const HPW = 2.25, headZ = (x, y) => frontZ(HEAD, HPW, x, y);
    // olhos (e as expressões) pintados de frente na cabeça
    const FX0 = 120, FX1 = 362, FY0 = 356, FY1 = 502, FW = 1024, FH = Math.round(FW * (FY1 - FY0) / (FX1 - FX0));
    const faceTex = f => { const c = document.createElement('canvas'); c.width = FW; c.height = FH; const g = c.getContext('2d'); const k = FW / (FX1 - FX0); g.scale(k, k); g.translate(-FX0, -FY0); drawFace(g, f);
      const t = new THREE.CanvasTexture(c); t.colorSpace = THREE.SRGBColorSpace; t.anisotropy = 8; return t; };
    function drawFace(g, f) {
      g.lineCap = 'round'; g.lineJoin = 'round';
      for (const s of [-1, 1]) {
        const xi = CX + s * 26, xo = CX + s * 64;   // canto de dentro / de fora
        if (f === 'pain') { g.strokeStyle = C.eye; g.lineWidth = 7; g.beginPath(); g.moveTo(xo, 408); g.lineTo(xi, 424); g.lineTo(xo, 440); g.stroke(); continue; }
        if (f === 'ko') { g.strokeStyle = C.eye; g.lineWidth = 7; const cx = (xi + xo) / 2; g.beginPath(); g.moveTo(cx - 14, 410); g.lineTo(cx + 14, 438); g.moveTo(cx + 14, 410); g.lineTo(cx - 14, 438); g.stroke(); continue; }
        const ti = f === 'angry' ? 414 : 409, to = 402, b = 445, r = 11;   // olho escuro de canto redondo, de cima inclinado para dentro
        g.fillStyle = C.eye; g.beginPath();
        g.moveTo(xo, to + r); g.quadraticCurveTo(xo, to, xo - s * r, to + 0.6); g.lineTo(xi + s * r, ti - 0.6); g.quadraticCurveTo(xi, ti, xi, ti + r);
        g.lineTo(xi, b - r); g.quadraticCurveTo(xi, b, xi + s * r, b); g.lineTo(xo - s * r, b); g.quadraticCurveTo(xo, b, xo, b - r); g.closePath(); g.fill();
      }
    }
    const headU = { tFace: { value: faceTex('normal') }, uStreak: streakU };
    { const rows = []; for (let y = 356; y <= 502; y += 2) rows.push({ y, pt: th => { const [a, zf, zb] = lerpT(HEAD, y); return oval(CX, a, zf, zb, HPW)(th); } });
      const m = pattern(flat(C.cream), streakGlsl(7) + `
        if (vPx.z > 20.0) { vec2 fuv = vec2((vPx.x - ${FX0.toFixed(1)}) / ${(FX1 - FX0).toFixed(1)}, 1.0 - (vPx.y - ${FY0.toFixed(1)}) / ${(FY1 - FY0).toFixed(1)});
          vec4 fc = texture2D(tFace, fuv); diffuseColor.rgb = mix(diffuseColor.rgb, fc.rgb, fc.a); }`, 'crHead', headU);
      P.headOL = withOutline(new THREE.Mesh(sections(rows, 288), m), 0.009); head.add(P.headOL); }
    // sobrancelhas grossas em relevo (marrom), bigode de guidão e o tufo de creme embaixo dele
    const browMat = flat(C.brow), brows = {};
    const setBrows = f => { for (const s of [-1, 1]) {
      const ang = f === 'angry' ? 1 : f === 'pain' ? -0.6 : 0, x0 = CX + s * 76, x1 = CX + s * 23, y0 = 377 - 3 * ang, y1 = 404 + 5 * ang;
      if (brows[s]) { head.remove(brows[s]); brows[s].children[0].geometry.dispose(); }
      const pts = [[x0, y0, headZ(x0, y0) + 2], [(x0 + x1) / 2, (y0 + y1) / 2 - 2, headZ((x0 + x1) / 2, (y0 + y1) / 2) + 4], [x1, y1, headZ(x1, y1) + 4]];
      brows[s] = withOutline(new THREE.Mesh(tubeVar(pts, u => 8.2 + 1.2 * Math.sin(Math.PI * u)), browMat), 0.006); head.add(brows[s]); } };
    setBrows('normal');
    { const pts = [[186, 482], [171, 477], [165, 465], [176, 455], [204, 451], [229, 455], [241, 464], [253, 455], [278, 451], [306, 455], [317, 465], [311, 477], [296, 482]];
      const g = tubeVar(pts.map(([x, y]) => [x, y, headZ(x, y) + 7 - 3 * Math.min(1, Math.abs(x - CX) / 75)]), u => { const d = Math.abs(u - 0.5) * 2; return 5 + 9 * Math.sin(Math.PI * Math.min(1, d * 1.6)) ** 0.6; });
      head.add(withOutline(new THREE.Mesh(g, flat(C.stache)), 0.006)); }
    { // cavanhaque de creme: ponta larga e achatada descendo do bigode
      const g = new THREE.ConeGeometry(1, 1, 24); g.rotateX(Math.PI); g.translate(0, -0.5, 0); g.scale(D(24), D(30), D(9));
      const m = new THREE.Mesh(g, flat(C.cream)); m.position.copy(V(241, 472, headZ(241, 472) + 1)); m.rotation.x = 0.3; head.add(ol(m, 0.006)); }

    // ================= chapéu: faixa branca contornada e dois gomos de pano
    { const rows = []; for (let y = 318; y <= 364; y += 2) rows.push({ y, pt: oval(CX + 1, 92, 93, 72, 2.2) });
      head.add(withOutline(new THREE.Mesh(sections(rows, 72), flat(C.white)), 0.008));
      for (const yy of [319, 362]) { const pts = []; for (let k = 0; k <= 48; k++) { const [x, z] = oval(CX + 1, 93.5, 94.5, 73.5, 2.2)(k / 48 * Math.PI * 2); pts.push(V(x, yy, z)); }
        head.add(new THREE.Mesh(new THREE.TubeGeometry(new THREE.CatmullRomCurve3(pts, true), 96, D(2.4), 6, true), flat(INKS))); }
      const hm = flat(C.hat);
      for (const [x, y, rx, ry, rz] of [[184, 286, 63, 53, 104], [300, 286, 63, 53, 104], [242, 300, 70, 40, 100]]) {
        const p = new THREE.Mesh(new THREE.SphereGeometry(1, 40, 28), hm); p.scale.set(D(rx), D(ry), D(rz)); p.position.copy(V(x, y, 10)); p.rotation.x = 0.12; head.add(ol(p, 0.008)); } }

    // ================= base de calda dourada
    const hips = new THREE.Group(); hips.position.y = HIPS; root.add(hips);
    const low = new THREE.Group(); low.position.y = -HIPS; hips.add(low);
    { const BASE = [[679, 0, 0, 0], [679, 120, 88, 88], [681, 150, 108, 108], [685, 160, 117, 117], [693, 164, 121, 121], [705, 165, 122, 122], [714, 161, 119, 119], [720, 152, 112, 112], [725, 136, 100, 100], [728, 110, 80, 80], [729, 0, 0, 0]];
      const rows = BASE.map(([y, a, zf, zb]) => ({ y, pt: a ? oval(239, a, zf, zb) : () => [239, 0] }));
      low.add(withOutline(new THREE.Mesh(sections(rows, 96), flat(C.base)), 0.009)); }

    // ================= mãos de creme com a bandeja verde: três copinhos, gotas vermelhas e as chamas de creme
    const L = (x, y, z) => new THREE.Vector3(D(x), D(y), D(z));   // px a partir do centro da mão (y para cima)
    // morango: corpo vermelho de ponta redonda para cima (base em y = 0), sementinhas amarelas em fileiras desencontradas
    const berryMat = (r, h) => { const m = flat(C.drop); m.onBeforeCompile = sh => {
      sh.uniforms.uSeed = col('#ffd84a');
      sh.vertexShader = 'varying vec3 vBP;\n' + sh.vertexShader.replace('#include <begin_vertex>', '#include <begin_vertex>\nvBP = position;');
      sh.fragmentShader = 'varying vec3 vBP; uniform vec3 uSeed;\n' + sh.fragmentShader.replace('#include <map_fragment>', `#include <map_fragment>
        { float a = atan(vBP.z, vBP.x) / 6.2831853 * 9.0, v = vBP.y / ${(D(h) / 6.5).toFixed(5)}; float row = floor(v);
          vec2 g = vec2(fract(a + 0.5 * mod(row, 2.0)) - 0.5, (fract(v) - 0.5) * 1.15);
          if (vBP.y > ${(D(h) * 0.12).toFixed(5)} && vBP.y < ${(D(h) * 0.9).toFixed(5)}) { float d = length(g); if (d < 0.13) diffuseColor.rgb = uSeed; else if (d < 0.19) diffuseColor.rgb *= 0.7; } }`); };
      m.customProgramCacheKey = () => 'berry' + r + '_' + h; return m; };
    const berryShape = (r, h) => {   // como a gota, mas de ombro largo e ponta arredondada
      const g = new THREE.SphereGeometry(1, 28, 20), p = g.attributes.position, B0 = 0.3;
      const rr = u => u < B0 ? Math.sqrt(Math.max(0, 1 - ((B0 - u) / B0) ** 2)) : Math.pow(1 - (u - B0) / (1 - B0), 0.7) * (1 - 0.15 * (u - B0)) + 0.0;
      for (let i = 0; i < p.count; i++) { const y = p.getY(i), u = (y + 1) / 2, s0 = Math.sqrt(Math.max(1e-6, 1 - y * y)), k = r * rr(u) / s0; p.setXYZ(i, p.getX(i) * k, u * h, p.getZ(i) * k); }
      g.scale(D(1), D(1), D(1)); g.computeVertexNormals(); return g; };
    const leafM = flat('#2f9e3c');
    const sepals = r => {   // coroa de folhinhas verdes em estrela, abrindo sobre a borda do copinho
      const st = new THREE.Shape(); for (let k = 0; k < 12; k++) { const rr = D(k % 2 ? r * 0.38 : r * 1.15), a = k * Math.PI / 6; if (k) st.lineTo(rr * Math.cos(a), rr * Math.sin(a)); else st.moveTo(rr * Math.cos(a), rr * Math.sin(a)); }
      const g = new THREE.ExtrudeGeometry(st, { depth: D(2.5), bevelEnabled: false }); g.rotateX(-Math.PI / 2);
      const p = g.attributes.position; for (let i = 0; i < p.count; i++) { const d = Math.hypot(p.getX(i), p.getZ(i)); p.setY(i, p.getY(i) - d * d / D(r) * 0.35); }   // pontas caídas
      g.computeVertexNormals(); return new THREE.Mesh(g, leafM); };
    // mão de creme derretendo: gotas da própria pele (esfera unitária na malha, escala na peça) brotam no alto da mão,
    // descem pela lateral, ficam penduradas embaixo e são absorvidas
    const HD = 4;
    const HAND_GLSL = `
      uniform vec4 uHD[${HD}];
      float dripH(vec3 p) {
        float th = atan(p.z, p.x), v = p.y, cr = sqrt(max(1.0 - v * v, 0.04)), h = 0.0;
        for (int i = 0; i < ${HD}; i++) { vec4 d = uHD[i]; if (d.z < 0.02) continue;
          float dth = mod(th - d.x + 3.14159265, 6.2831853) - 3.14159265, ds = dth * cr, dv = v - d.y, R = d.z;
          float b = 1.0 - (ds * ds + dv * dv) / (R * R), hb = b > 0.0 ? sqrt(b) * R * 0.7 : 0.0, ht = 0.0;
          if (dv > 0.0 && dv < d.w) { float k = 1.0 - dv / d.w, w = R * 0.6 * k + 0.02, c = 1.0 - (ds / w) * (ds / w); if (c > 0.0) ht = sqrt(c) * R * 0.4 * k; }
          h = max(h, max(hb, ht)); }
        return h; }`;
    const handDrips = (OL) => {
      const U = { value: Array.from({ length: HD }, () => new THREE.Vector4()) }, list = [];
      const patch = (mat, key, normals) => { const prev = mat.onBeforeCompile, k0 = mat.customProgramCacheKey();
        mat.onBeforeCompile = (sh, r) => { prev.call(mat, sh, r); sh.uniforms.uHD = U;
          let v = sh.vertexShader.replace('void main() {', HAND_GLSL + '\nvoid main() {');
          if (normals) v = v.replace('#include <beginnormal_vertex>', `#include <beginnormal_vertex>
            { vec3 pN = normalize(position), T1 = normalize(vec3(-pN.z, 0.0, pN.x) + 1e-5), Tv = normalize(vec3(-pN.x * pN.y, 1.0 - pN.y * pN.y, -pN.z * pN.y) + 1e-5);
              float g1 = (dripH(normalize(pN + T1 * 0.04)) - dripH(normalize(pN - T1 * 0.04))) / 0.08, gv = (dripH(normalize(pN + Tv * 0.04)) - dripH(normalize(pN - Tv * 0.04))) / 0.08;
              objectNormal = normalize(objectNormal - g1 * T1 - gv * Tv); }`);
          sh.vertexShader = v.replace('#include <begin_vertex>', '#include <begin_vertex>\ntransformed += normalize(normal) * dripH(normalize(position));'); };
        mat.customProgramCacheKey = () => k0 + '|' + key; mat.needsUpdate = true; };
      patch(OL.children[0].material, 'handDrip', true); patch(OL.children[1].material, 'handDripHull', false);
      const spawn = (d, t, first = false) => { d.th = Math.random() * Math.PI * 2 - Math.PI; d.R = 0.22 + Math.random() * 0.1; d.v0 = 0.35 + Math.random() * 0.3;
        d.dur = 2.6 + Math.random() * 2; d.t0 = t - (first ? Math.random() * d.dur : 0); };
      for (let i = 0; i < HD; i++) { const d = {}; spawn(d, 0, true); list.push(d); }
      return t => list.forEach((d, i) => {
        let p = (t - d.t0) / d.dur; if (p >= 1 || p < 0) { spawn(d, t); p = 0; }
        const size = ss(0, 0.16, p) * (1 - ss(0.82, 1, p)), q = Math.min(1, Math.max(0, (p - 0.08) / 0.8)), vc = d.v0 - (d.v0 + 0.92) * Math.pow(q, 1.6);
        U.value[i].set(d.th, vc, d.R * size, Math.min(0.9, (0.15 + 0.7 * q)) * size);
      }); };
    const handSteps = []; P.handOLs = [];
    function glove(sx) {
      const g = new THREE.Group(), cupM = flat(C.cup);
      const hm = flat(C.hand), hand = new THREE.Mesh(new THREE.SphereGeometry(1, 64, 48), hm); hand.scale.set(D(37), D(26), D(31));
      const hOL = ol(hand, 0.008); g.add(hOL); handSteps.push(handDrips(hOL)); P.handOLs.push(hOL);
      const tx = -sx * 6;   // a bandeja puxa um pouco para o lado do corpo
      const tray = new THREE.Group(); tray.rotation.y = sx * 0.6; g.add(tray);   // meio de lado: a ponta de fora vai para trás (como na folha)
      const bar = new THREE.Mesh(new THREE.BoxGeometry(D(96), D(9), D(26)), cupM); bar.position.copy(L(tx, 30, 0)); tray.add(ol(bar, 0.006));
      for (const [dx, up, big] of [[-36, 0, 0], [0, 6, 1], [36, 0, 0]]) {
        const cup = new THREE.Mesh(new THREE.CylinderGeometry(D(14), D(12), D(18), 24), cupM); cup.position.copy(L(tx + dx, 40 + up, 0)); tray.add(ol(cup, 0.006));
        const r = big ? 19 : 16, h = big ? 50 : 42, berry = new THREE.Mesh(berryShape(r, h), berryMat(r, h));
        berry.position.copy(L(tx + dx, 49 + up, 0)); berry.rotation.y = dx * 0.05; tray.add(ol(berry, 0.006));
        const sp = sepals(r); sp.position.copy(L(tx + dx, 51 + up, 0)); sp.rotation.y = dx * 0.07; tray.add(ol(sp, 0.004));
      }
      // alça verde: arco do fim da bandeja em volta da mão, do lado do corpo
      const ring = new THREE.Mesh(new THREE.TorusGeometry(D(32), D(4), 8, 32, Math.PI * 0.95), cupM); ring.rotation.z = -sx * Math.PI * 0.55 - Math.PI * 0.5; ring.position.copy(L(-sx * 4, -2, 0)); g.add(ol(ring, 0.005));
      return g;
    }
    P.gloves = { [-1]: glove(-1), [1]: glove(1) }; root.add(P.gloves[-1], P.gloves[1]);
    const hands = { [-1]: new THREE.Object3D(), [1]: new THREE.Object3D() }; root.add(hands[-1], hands[1]);
    const UPV = new THREE.Vector3(0, 1, 0), FWD = new THREE.Vector3(0, 0, 1);

    // ponto na superfície do sino (ângulo th, altura y), afastado off px para fora
    const bodyPt = (th, y, off = 0) => { const [a, zf, zb] = lerpT(BODY, y); return oval(CX, a + off, zf + off, zb + off, BPW)(th); };
    // cordinhas do avental: saem das beiradas na cintura, dão a volta pelas costas e amarram num laço
    { const Y0 = 562, half = 74 + 11 * (Y0 - 498) / 189, strM = flat(C.white);
      for (const s of [1, -1]) {
        let th0 = 0; for (let k = 0; k <= 200; k++) { const th = Math.PI / 2 - k / 200 * Math.PI / 2; if ((bodyPt(th, Y0)[0] - CX) >= half - 2) { th0 = th; break; } }
        const pts = []; for (let k = 0; k <= 24; k++) { const t = th0 - k / 24 * (th0 + Math.PI / 2), th = s > 0 ? t : Math.PI - t, [x, z] = bodyPt(th, Y0, 2.5); pts.push(V(x, Y0 + 2 * Math.sin(k / 24 * Math.PI), z)); }
        body.add(withOutline(new THREE.Mesh(new THREE.TubeGeometry(new THREE.CatmullRomCurve3(pts), 48, D(3.6), 8), strM), 0.004));
      }
      const [, zb] = bodyPt(-Math.PI / 2, Y0, 5), knot = V(CX, Y0, zb), bow = new THREE.Group(); bow.position.copy(knot); body.add(bow);
      const kn = new THREE.Mesh(new THREE.SphereGeometry(D(8), 16, 12), strM); kn.scale.z = 0.7; bow.add(ol(kn, 0.004));
      for (const s of [-1, 1]) {
        const loop = new THREE.Mesh(new THREE.TorusGeometry(D(17), D(4.2), 10, 28), strM); loop.scale.set(1, 0.62, 1); loop.position.set(D(s * 21), D(4), 0); loop.rotation.z = -s * 0.35; bow.add(ol(loop, 0.004));
        const tail = [V(CX + s * 4, Y0 + 5, zb), V(CX + s * 13, Y0 + 28, zb - 2), V(CX + s * 19, Y0 + 54, zb - 1)].map(p => p.sub(knot));
        bow.add(withOutline(new THREE.Mesh(new THREE.TubeGeometry(new THREE.CatmullRomCurve3(tail), 16, D(3.6), 8), strM), 0.004));
      } }
    // creme derretendo: as gotas são a própria pele do sino estufando (mesma cor, sombra e contorno do corpo): um bulbo com
    // a cauda fina para cima, que brota, desce acelerando pela curva do sino (a cauda espicha) e afunda de volta; depois
    // nasce em outro lugar. Nunca na frente do avental. Deslocamento no shader do corpo e do contorno dele (mesma conta).
    const ND = '${ND}';   // trocado pelo tamanho de cada conjunto
    const DRIP_GLSL = `
      uniform vec4 uDrip[${ND}];
      float dripD(vec3 pp) {   // px da folha: x, y (para baixo), z
        float th = atan(pp.z, pp.x - ${CX.toFixed(1)}), rr = length(vec2(pp.x - ${CX.toFixed(1)}, pp.z)), h = 0.0;
        for (int i = 0; i < ${ND}; i++) { vec4 d = uDrip[i]; if (d.z < 0.3) continue;
          float dth = mod(th - d.x + 3.14159265, 6.2831853) - 3.14159265, ds = dth * rr, dy = pp.y - d.y, R = d.z;
          float b = 1.0 - (ds * ds + dy * dy) / (R * R), hb = b > 0.0 ? sqrt(b) * R * 0.65 : 0.0, ht = 0.0;
          if (dy < 0.0 && dy > -d.w) { float k = 1.0 + dy / d.w, w = R * 0.62 * k + 0.6, c = 1.0 - (ds / w) * (ds / w); if (c > 0.0) ht = sqrt(c) * R * 0.38 * k; }
          h = max(h, max(hb, ht)); }
        return h; }
      vec3 pxOf(vec3 p) { return vec3(p.x * ${S_PX.toFixed(1)} + ${CX.toFixed(1)}, ${FOOT.toFixed(1)} - p.y * ${S_PX.toFixed(1)}, p.z * ${S_PX.toFixed(1)}); }
    `;
    const DISP = `{ vec3 ppD = pxOf(position); transformed += normalize(normal) * dripD(ppD) / ${S_PX.toFixed(1)}; }`;
    const NORM = `{ vec3 ppN = pxOf(position), T1 = normalize(vec3(-ppN.z, 0.0, ppN.x - ${CX.toFixed(1)}) + 1e-5), DY = vec3(0.0, 1.5, 0.0);
      float g1 = (dripD(ppN + T1 * 1.5) - dripD(ppN - T1 * 1.5)) / 3.0, gy = (dripD(ppN + DY) - dripD(ppN - DY)) / 3.0;
      objectNormal = normalize(objectNormal - g1 * T1 + gy * vec3(0.0, 1.0, 0.0)); }`;   // y da folha para baixo = −y do objeto
    const dripPatch = (mat, key, normals, U, n) => {
      const prev = mat.onBeforeCompile, k0 = mat.customProgramCacheKey();
      mat.onBeforeCompile = (sh, r) => {
        prev.call(mat, sh, r); sh.uniforms.uDrip = U;
        let v = sh.vertexShader.replace('void main() {', DRIP_GLSL.replaceAll('${ND}', String(n)) + '\nvoid main() {');
        if (normals) v = v.replace('#include <beginnormal_vertex>', '#include <beginnormal_vertex>\n' + NORM);
        sh.vertexShader = v.replace('#include <begin_vertex>', '#include <begin_vertex>\n' + DISP);
      };
      mat.customProgramCacheKey = () => k0 + '|' + key + n; mat.needsUpdate = true;
    };
    // cada malha com as suas gotas e as suas faixas: a gota (bulbo e cauda) nunca sai da faixa, então não passa por cima da
    // corda do avental; e os ângulos deixam de fora a frente (avental no corpo; olhos, sobrancelhas e bigode na cabeça)
    const dripSets = [];
    const makeDrips = (OL, n, zones) => {
      const U = { value: Array.from({ length: n }, () => new THREE.Vector4()) }, list = [];
      dripPatch(OL.children[0].material, 'drip', true, U, n); dripPatch(OL.children[1].material, 'dripHull', false, U, n);
      const W = zones.reduce((s, z) => s + z.w, 0);
      const spawn = (d, t, first = false) => {
        let r = Math.random() * W, z = zones[0]; for (const zz of zones) { if (r < zz.w) { z = zz; break; } r -= zz.w; }
        d.z = z; d.R = z.r[0] + Math.random() * (z.r[1] - z.r[0]);
        d.th = (z.th[0] + Math.random() * (z.th[1] - z.th[0])) * Math.PI / 180; if (d.th > Math.PI) d.th -= 2 * Math.PI;
        d.ys = z.y0 + d.R + 4 + Math.random() * 10; d.travel = Math.max(0, Math.min(z.y1 - d.R - d.ys, 40 + Math.random() * 90));
        d.dur = 2.6 + d.travel / 40 + Math.random() * 1.5; d.t0 = t - (first ? Math.random() * d.dur : 0);
      };
      for (let i = 0; i < n; i++) { const d = {}; spawn(d, 0, true); list.push(d); }
      dripSets.push(t => list.forEach((d, i) => {
        let p = (t - d.t0) / d.dur; if (p >= 1 || p < 0) { spawn(d, t); p = 0; }
        const size = ss(0, 0.16, p) * (1 - ss(0.78, 1, p)), q = Math.min(1, Math.max(0, (p - 0.08) / 0.78)), yc = d.ys + d.travel * Math.pow(q, 1.7);
        U.value[i].set(d.th, yc, d.R * size, Math.min((8 + 34 * q) * size, yc - d.z.y0));   // a cauda para na borda de cima da faixa
      }));
    };
    // corpo: acima da corda (y 562 ± 7) e abaixo dela até o pé; fora da frente do avental (θ 35°–145°)
    makeDrips(P.bodyOL, 12, [{ y0: 503, y1: 553, th: [147, 393], r: [8, 10], w: 1 }, { y0: 571, y1: 674, th: [147, 393], r: [9.5, 13.5], w: 2 }]);
    // cabeça: da faixa do chapéu até a base; nas laterais do rosto e atrás (sem olhos, sobrancelhas e bigode)
    makeDrips(P.headOL, 7, [{ y0: 366, y1: 492, th: [142, 398], r: [8, 11], w: 1 }]);
    P.dripStep = t => dripSets.forEach(f => f(t));
    body.matrixAutoUpdate = false;
    const bodyM = new THREE.Matrix4(), shM = new THREE.Matrix4(), scM = new THREE.Matrix4(), trM = new THREE.Matrix4().makeTranslation(0, -WAIST, 0);

    const setFace = f => { headU.tFace.value.dispose(); headU.tFace.value = faceTex(f); setBrows(f); };
    // ================= especiais (os do jogo, skills.cpp e skill_poses.cpp, com os efeitos do laboratório, vfx/lab_cremoso.cpp)
    const easeO = k => 1 - (1 - k) ** 3, CHEST = Y(525), MOUTH = Y(462);
    const fxRoot = new THREE.Group(); root.add(fxRoot);
    const glow = (col, o = 1) => new THREE.MeshBasicMaterial({ color: col, transparent: true, opacity: o, depthWrite: false, side: THREE.DoubleSide });
    const queue = []; let now = 0; const after = (dt, fn) => queue.push({ at: now + dt, fn });
    const FX = { sign: -99, whip: -99, syrup: -99, melt: -99, puddle: -99, cone: -99, cherry: -99, cherryHit: -99, flavor: 'vanilla' };
    const FLAVORS = {   // cores do sprite de cada sabor: creme, listra, mão, base; tornado: ponta, gomo, claro, arco
      vanilla: { cream: C.cream, streak: C.streak, hand: C.hand, base: C.base, t: ['#ca8a04', '#eab308', '#fef08a', '#eab308'], hot: '#d97706', foam: '#fffde7' },
      frozen: { cream: '#bae6fd', streak: '#38bdf8', hand: '#dbeafe', base: '#38bdf8', t: ['#0284c7', '#38bdf8', '#bae6fd', '#0284c7'], hot: '#0284c7', foam: '#f8fafc' },
      revenge: { cream: '#e11d48', streak: '#be123c', hand: '#fb7185', base: '#7f1d1d', t: ['#7f1d1d', '#be123c', '#e11d48', '#be123c'], hot: '#991b1b', foam: '#fb7185' },
    };
    const fl = () => FLAVORS[FX.flavor];
    // bolinhas: gotas, migalhas, vapor (op < 1), com gravidade e vida; o raio entra já no nascimento
    const dotG = new THREE.SphereGeometry(1, 10, 8), dots = Array.from({ length: 220 }, () => { const m = new THREE.Mesh(dotG, glow('#fff')); m.visible = false; fxRoot.add(m); return { m, life: 0, v: new THREE.Vector3() }; });
    let dotI = 0; const dot = (p, v, rr, col, life, g = 3, op = 1, grow = 0) => { const q = dots[dotI++ % dots.length]; q.m.position.copy(p); q.m.scale.setScalar(rr); q.v.copy(v); q.r = rr; q.m.material.color.set(col); q.m.material.opacity = op; q.op = op; q.life = q.life0 = life; q.g = g; q.grow = grow; q.m.visible = true; };
    const burst = (p, n, spd, rr, cols, life, g = 3) => { for (let i = 0; i < n; i++) { const a = Math.random() * 6.283, sp = spd * (0.5 + Math.random()); dot(p, new THREE.Vector3(Math.cos(a) * sp, Math.sin(a) * sp * 0.8 + 0.4, (Math.random() - 0.3) * spd), rr * (0.6 + Math.random() * 0.7), cols[i % cols.length], life * (0.7 + Math.random() * 0.6), g); } };
    const ringG = new THREE.RingGeometry(0.84, 1, 40), rings = Array.from({ length: 12 }, () => { const m = new THREE.Mesh(ringG, glow('#fff')); m.visible = false; fxRoot.add(m); return { m, life: 0 }; });
    let ringI = 0; const ring = (p, R, col, life, flatR = false, delay = 0) => after(delay, () => { const q = rings[ringI++ % rings.length]; q.m.position.copy(p); q.R = R; q.life = q.life0 = life; q.m.material.color.set(col); q.flat = flatR; q.m.rotation.set(flatR ? -Math.PI / 2 : 0, 0, 0); q.m.visible = true; });
    // Tornado (cremosoVanillaTornadoBody): o corpo vira um funil de creme em gomos com a ponta no chão, o chapéu em cima, os
    // morangos na borda e os arcos de calda girando em volta
    const tMats = [flat('#ca8a04'), flat('#eab308'), flat('#fef08a')], tornado = new THREE.Group(); tornado.visible = false; root.add(tornado);
    const spinner = new THREE.Group(); tornado.add(spinner);
    { const tier = (y0, y1, r0, r1, m) => { const n = 10, pts = [new THREE.Vector2(0.001, y0)]; for (let i = 0; i <= n; i++) { const u = i / n; pts.push(new THREE.Vector2(r0 + (r1 - r0) * u + Math.sin(u * Math.PI) * 0.035, y0 + (y1 - y0) * u)); } pts.push(new THREE.Vector2(0.001, y1)); spinner.add(withOutline(new THREE.Mesh(new THREE.LatheGeometry(pts, 32), m), 0.009)); };
      tier(0, 0.5, 0.02, 0.26, tMats[0]); tier(0.4, 0.85, 0.2, 0.42, tMats[1]); tier(0.75, 1.2, 0.36, 0.58, tMats[1]); tier(1.14, 1.3, 0.55, 0.44, tMats[2]);
      for (const ph of [0, 2.094, 4.189]) { const pts = []; for (let i = 0; i <= 40; i++) { const u = i / 40, y = 0.05 + u * 1.2, rad = 0.05 + (y / 1.25) * 0.56 + 0.012, a = ph + u * 8; pts.push(new THREE.Vector3(Math.cos(a) * rad, y, Math.sin(a) * rad)); } spinner.add(new THREE.Mesh(new THREE.TubeGeometry(new THREE.CatmullRomCurve3(pts), 90, D(4), 6), tMats[2])); }
      for (const s of [-1, 1]) { const b = new THREE.Mesh(new THREE.SphereGeometry(D(22), 16, 12), flat('#dc2626')); b.position.set(s * 0.5, 1.26, 0); spinner.add(ol(b, 0.005)); }
      const hb = new THREE.Mesh(new THREE.CylinderGeometry(D(50), D(52), D(26), 24), flat(C.white)); hb.position.y = 1.36; tornado.add(ol(hb, 0.008));
      for (const s of [-1, 1]) { const p = new THREE.Mesh(new THREE.SphereGeometry(D(36), 20, 14), flat(C.hat)); p.scale.y = 0.9; p.position.set(s * D(24), 1.46, 0); tornado.add(ol(p, 0.008)); } }
    const arcs = [0.45, 0.8, 1.1].map((y, i) => { const m = new THREE.Mesh(new THREE.TorusGeometry(0.34 + i * 0.13 + 0.12, D(4.5), 8, 30, 1.5), flat('#eab308')); m.rotation.x = Math.PI / 2; m.position.y = y; tornado.add(m); return m; });
    const starG = (() => { const sh = new THREE.Shape(); for (let i = 0; i < 10; i++) { const a = i / 10 * 6.283 + Math.PI / 2, q = D(i % 2 ? 7 : 17); i ? sh.lineTo(Math.cos(a) * q, Math.sin(a) * q) : sh.moveTo(Math.cos(a) * q, Math.sin(a) * q); } sh.closePath(); return new THREE.ExtrudeGeometry(sh, { depth: D(4), bevelEnabled: false }); })();
    const stars = [0, 1, 2, 3].map(() => { const g = withOutline(new THREE.Mesh(starG, flat('#fde047')), 0.004); g.visible = false; fxRoot.add(g); return g; });
    let spinOn = false;
    const setSpin = on => { if (on === spinOn) return; spinOn = on; tornado.visible = on; body.visible = head.visible = low.visible = !on; if (on) { const q = fl().t; [0, 1, 2].forEach(i => tMats[i].color.set(q[i])); arcs.forEach(a => a.material.color.set(q[3])); } };
    // Calda Quente: dois jatos em arco das luvas até o alvo
    const JET = 16, jets = [-1, 1].map(s => Array.from({ length: JET }, (_, i) => { const m = new THREE.Mesh(new THREE.SphereGeometry(1, 12, 10), flat(i % 3 ? '#f59e0b' : '#dc5b0b')); m.visible = false; fxRoot.add(m); return { m, s, u: i / (JET - 1) }; })).flat();
    const TARGET = new THREE.Vector3(0, 0.95, 1.9);
    // Casquinha: a casquinha crocante sai do peito na direção do rival e duas saem das costas (as "aprovadas")
    const wafTex = (() => { const c = document.createElement('canvas'); c.width = c.height = 256; const g = c.getContext('2d'); g.fillStyle = '#d5a45e'; g.fillRect(0, 0, 256, 256); g.strokeStyle = '#86532f'; g.lineWidth = 6; for (let i = -256; i < 512; i += 42) { g.beginPath(); g.moveTo(i, 0); g.lineTo(i + 256, 256); g.moveTo(i + 256, 0); g.lineTo(i, 256); g.stroke(); } const t = new THREE.CanvasTexture(c); t.colorSpace = THREE.SRGBColorSpace; t.wrapS = t.wrapT = THREE.RepeatWrapping; t.repeat.set(3, 2); return t; })();
    const wafM = new THREE.MeshToonMaterial({ map: wafTex, gradientMap: flat('#fff').gradientMap });
    const coneG = (R, Ln) => { const g = new THREE.ConeGeometry(R, Ln, 28, 1, true); g.translate(0, Ln / 2, 0); return g; };
    const chestCone = withOutline(new THREE.Mesh(coneG(0.27, 1.1), wafM), 0.007); chestCone.visible = false; upper.add(chestCone);
    const backCones = [-1, 1].map(s => { const g = withOutline(new THREE.Mesh(coneG(0.11, 0.62), wafM), 0.006); g.visible = false; upper.add(g); g.userData.dir = new THREE.Vector3(s * 0.4, 0.75, -0.5).normalize(); g.position.set(s * 0.24, CHEST - WAIST + 0.08, -0.25); g.quaternion.setFromUnitVectors(new THREE.Vector3(0, 1, 0), g.userData.dir); return g; });
    chestCone.position.set(0, CHEST - WAIST, D(100)); chestCone.quaternion.setFromUnitVectors(new THREE.Vector3(0, 1, 0), new THREE.Vector3(0, 0.02, 1).normalize());
    // Poças: a da calda fervente e a do corpo derretido, no chão; a da cereja no rival
    const flatDisc = col => { const m = new THREE.Mesh(new THREE.CircleGeometry(1, 40), flat(col)); m.rotation.x = -Math.PI / 2; m.visible = false; const e = new THREE.Mesh(new THREE.RingGeometry(0.96, 1.02, 40), new THREE.MeshBasicMaterial({ color: '#2d251e', side: THREE.DoubleSide })); e.position.z = 0.002; m.add(e); fxRoot.add(m); return m; };
    const puddleD = flatDisc(C.base), meltD = flatDisc(C.base), cherryD = flatDisc('#991b1b'); puddleD.position.set(0, 0.012, 0.1); meltD.position.set(0, 0.016, 0); cherryD.position.set(0, 0.012, TARGET.z);
    // Cereja: a cereja gigante com cabinho, cuspida depois de inflar em três estágios
    const cherry = new THREE.Group(); cherry.visible = false; fxRoot.add(cherry);
    { cherry.add(ol(new THREE.Mesh(new THREE.SphereGeometry(0.14, 24, 18), flat('#dc2626')), 0.008)); const hl = new THREE.Mesh(new THREE.SphereGeometry(0.035, 10, 8), new THREE.MeshBasicMaterial({ color: '#ffffff' })); hl.position.set(-0.05, 0.07, 0.11); cherry.add(hl);
      const st = new THREE.Mesh(new THREE.TubeGeometry(new THREE.CatmullRomCurve3([new THREE.Vector3(0, 0.13, 0), new THREE.Vector3(0.03, 0.22, -0.02), new THREE.Vector3(0.09, 0.3, -0.03)]), 12, 0.012, 6), flat('#16a34a')); cherry.add(st); }
    let meltK = 0, inflate = 0;
    // sabores: troca as cores do corpo (creme, listra, mão, base)
    const flavorSets = { cream: new THREE.Color(C.cream), hand: new THREE.Color(C.hand), base: new THREE.Color(C.base) }, flavorMats = { cream: [], hand: [], base: [] };
    for (const grp of [body, head, low, P.gloves[-1], P.gloves[1]]) grp.traverse(o => { if (!o.isMesh || !o.material.color) return; for (const k of Object.keys(flavorSets)) if (o.material.color.equals(flavorSets[k]) && !flavorMats[k].includes(o.material)) flavorMats[k].push(o.material); });
    const setFlavor = k => { FX.flavor = k; const q = FLAVORS[k]; for (const n of Object.keys(flavorMats)) for (const m of flavorMats[n]) m.color.set(q[n]); streakU.value.set(q.streak); puddleD.material.color.set(q.hot); meltD.material.color.set(q.base); document.querySelectorAll('[data-flavor]').forEach(b => { b.style.outline = b.dataset.flavor === k ? '2px solid #fc6' : ''; }); };
    document.querySelectorAll('[data-flavor]').forEach(b => b.onclick = () => setFlavor(b.dataset.flavor));
    const mirror = (v, s) => [v[0], v[1], v[2], v[3] || 0, v[4] || 0];
    const skills = {
      // assinatura (laboratório): o corpo vira tornado de creme com o sabor da forma, e volta bravo
      signature: { dur: 2.6, start: () => { FX.sign = now; setFace('angry'); after(2.6, () => setFace('normal')); },
        pose: (k, pose) => { const e = ss(0.12, 0.3, k / 2.6) * (1 - ss(0.75, 0.9, k / 2.6)); pose[-1] = [0.02 * e, -0.12 * e, -0.14 * e, 0, 0]; pose[1] = [0.02 * e, -0.12 * e, -0.14 * e, 0, 0]; return {}; } },
      syrup: { dur: 1.1, start: () => { FX.syrup = now; setFace('angry'); after(1.1, () => setFace('normal')); },
        pose: (k, pose) => { const p = k / 1.1, w = ss(0, 0.3, p) * (1 - ss(0.35, 0.42, p)), hit = ss(0.35, 0.42, p) * (1 - ss(0.68, 1, p)), v = [-0.12 * w + 0.5 * hit, 0.14 * w + 0.06 * hit, 0.5 * hit + 0.1 * w]; pose[-1] = mirror(v); pose[1] = mirror(v); return { lean: 0.14 * hit - 0.08 * w }; } },
      // tornado: as mãos giram alternadas e sai creme em volta; acaba tonto, com as estrelas
      whip: { dur: 1.3, start: () => { FX.whip = now; setFace('angry'); after(1.3, () => setFace('normal')); },
        pose: (k, pose) => { const p = k / 1.3, c = Math.cos(p * Math.PI * 4), sn = Math.sin(p * Math.PI * 4), e = ss(0, 0.1, p) * (1 - ss(0.85, 1, p)); pose[-1] = [0.28 * sn * e, 0, -0.2 * c * e, 0, 0]; pose[1] = [-0.28 * sn * e, 0, -0.2 * c * e, 0, 0]; return { roll: 0 }; } },
      melt: { dur: 0.9, start: () => { FX.melt = now; }, pose: (k, pose) => { const e = ss(0, 0.5, k); pose[-1] = [0, -0.14 * e, 0, 0, 0]; pose[1] = [0, -0.14 * e, 0, 0, 0]; return { tilt: 0.05 * Math.sin(k * 14) * (1 - ss(0.6, 0.9, k)) }; } },
      // casquinha: arma o soco e a casquinha crocante sai do peito (ignora a guarda); migalhas e anéis
      cone: { dur: 1.0, start: () => { FX.cone = now; setFace('angry'); after(1.0, () => setFace('normal')); after(0.3, () => { const p = new THREE.Vector3(0, CHEST, 0.42); burst(p, 24, 2.2, 0.018, ['#ca8a04', '#eab308', '#d5a45e'], 0.7, 5); [0, 0.06, 0.12].forEach((d, i) => ring(new THREE.Vector3(0, CHEST, 0.45 + i * 0.3), 0.3 + i * 0.12, '#fde047', 0.35, false, d)); }); },
        pose: (k, pose) => { const p = k / 1.0, h = ss(0.15, 0.3, p) * (1 - ss(0.7, 1, p)); pose[1] = [0.46 * h, 0.06 * h, 0.1 * h, 0, 0]; return { twist: 0.2 * h, lean: 0.12 * h }; } },
      puddle: { dur: 1.2, start: () => { FX.puddle = now; after(0.42, () => { const p = new THREE.Vector3(0, 0.03, 0.1); ring(p, 1.1, fl().foam, 0.5, true); burst(p, 18, 1.5, 0.02, [fl().hot, fl().foam], 0.7, 5); }); },
        pose: (k, pose) => { const p = k / 1.2, up = ss(0, 0.35, p) * (1 - ss(0.75, 1, p)), sl = ss(0.3, 0.42, p) * (1 - ss(0.7, 1, p)); const v = [0.02 * up, (0.34 * up - 0.5 * sl), 0.1 * up]; pose[-1] = mirror(v); pose[1] = mirror(v); return { lean: 0.1 * sl, tilt: 0.03 * Math.sin(k * 20) * up }; } },
      // cereja: incha em três estágios, cospe a cereja, ela voa e estoura no rival soltando suco
      cherry: { dur: 1.5, start: () => { FX.cherry = now; setFace('angry'); after(1.5, () => setFace('normal')); after(1.5 * 0.48, () => { const p = new THREE.Vector3(0, MOUTH, D(115)); ring(p, 0.4, '#ff4d6d', 0.3); burst(p, 24, 2.2, 0.018, ['#dc2626', '#f87171'], 0.55); }); },
        pose: (k, pose) => { const p = k / 1.5, prep = ss(0, 0.48, p), rec = p > 0.48 ? Math.sin((p - 0.48) / 0.52 * Math.PI) : 0, v = [-0.08 * prep * (1 - ss(0.48, 0.55, p)) + 0.1 * rec, 0.04 * prep, -0.25 * prep * (1 - ss(0.48, 0.6, p)) + 0.12 * rec]; pose[-1] = mirror(v); pose[1] = mirror(v); return { lean: -0.08 * prep * (1 - ss(0.48, 0.52, p)) + 0.16 * rec }; } },
    };
    const fxFrame = c => {
      now = c.t; const t = now, dt = c.dt, cam = window.VIN?.camera;
      for (let i = queue.length - 1; i >= 0; i--) if (now >= queue[i].at) { queue[i].fn(); queue.splice(i, 1); }
      for (const q of dots) { if (q.life <= 0) { q.m.visible = false; continue; } q.life -= dt; q.v.y -= q.g * dt; q.m.position.addScaledVector(q.v, dt); const u = q.life / q.life0; if (q.grow) { q.r += q.grow * dt; q.m.scale.setScalar(q.r); q.m.material.opacity = q.op * u; } else q.m.scale.setScalar(q.r * Math.max(0.05, u)); if (q.m.position.y < 0.01) q.life = 0; }
      for (const q of rings) { if (q.life <= 0) { q.m.visible = false; continue; } q.life -= dt; const u = 1 - q.life / q.life0, e = q.R * easeO(u) + 0.03; q.m.scale.set(e, e, 1); q.m.material.opacity = Math.max(0, 1 - u); if (cam && !q.flat) q.m.quaternion.copy(cam.quaternion); }
      // tornado
      const ps = (t - FX.sign) / 2.6, pw = (t - FX.whip) / 1.3, sig = ss(0.12, 0.85, ps), wOn = pw > 0.06 && pw < 0.9, on = (sig > 0.05 && sig < 0.98 && ps < 1) || wOn;
      setSpin(on);
      if (on) { spinner.rotation.y = t * 18; tornado.scale.setScalar(1 + 0.03 * Math.sin(t * 22)); arcs.forEach((a, i) => { a.rotation.z = t * 9 + i * 2.1; }); if (Math.random() < 0.6) { const a = Math.random() * 6.283, sp = 1.2 + Math.random() * 1.6; dot(new THREE.Vector3(Math.cos(a) * 0.3, 0.7 + Math.random() * 0.5, Math.sin(a) * 0.3), new THREE.Vector3(Math.cos(a) * sp, 0.8, Math.sin(a) * sp), 0.022 + Math.random() * 0.02, Math.random() < 0.6 ? fl().t[2] : fl().foam, 0.8, 5); } }
      { const a = pw > 0.8 && pw < 1.5; stars.forEach((g, i) => { g.visible = a; if (a) { const an = t * 5 + i * 1.571; g.position.set(Math.cos(an) * 0.36, Y(300), Math.sin(an) * 0.3); g.rotation.y = t * 6; } }); }
      // calda: ferve nas luvas, depois os jatos
      { const u = (t - FX.syrup) / 1.1; if (u > 0 && u < 0.35) for (const s of [-1, 1]) { const g = hands[s].position; if (Math.random() < 0.6) dot(g.clone().add(new THREE.Vector3((Math.random() - 0.5) * 0.15, 0.1, 0.05)), new THREE.Vector3(s * 0.2, 0.6 + Math.random() * 0.5, 0), 0.03, '#fff7e0', 0.5, 0, 0.5, 0.12); if (Math.random() < 0.5) dot(g.clone().add(new THREE.Vector3((Math.random() - 0.5) * 0.2, 0.05, 0.1)), new THREE.Vector3(s * 0.4, 0.6, 0.2), 0.012, Math.random() < 0.5 ? '#f43f5e' : '#fde047', 0.35, 1); }
        const jt = (u - 0.36) / 0.34, jOn = jt > 0 && jt < 1; for (const j of jets) { j.m.visible = jOn; if (!jOn) continue; const head0 = Math.min(1, jt * 2.4), tail = Math.max(0, jt * 2.4 - 1.2), q = tail + (head0 - tail) * j.u, A = hands[j.s].position, B = TARGET.clone().add(new THREE.Vector3(j.s * 0.09, 0, 0)); j.m.position.copy(A).lerp(B, q); j.m.position.y += Math.sin(q * Math.PI) * 0.22; j.m.scale.setScalar(0.04 + 0.025 * Math.sin(j.u * 9 + t * 20)); }
        if (jOn && Math.random() < 0.8) { const s = Math.random() < 0.5 ? -1 : 1; dot(hands[s].position.clone().add(new THREE.Vector3(0, 0, 0.1)), new THREE.Vector3((Math.random() - 0.5) * 1.1, 0.6 + Math.random() * 0.7, 0.8 + Math.random()), 0.022, '#f59e0b', 0.55, 4); dot(new THREE.Vector3((Math.random() - 0.5) * 0.4, 0.9, 0.5 + Math.random() * 1), new THREE.Vector3(0, 0.7, 0), 0.03, '#fff7e0', 0.6, 0, 0.45, 0.14); }
        if (jt >= 1 && jt < 1.1 && !FX.syrupHit) { FX.syrupHit = true; burst(TARGET, 22, 1.8, 0.02, ['#f59e0b', '#dc5b0b', '#fde047'], 0.6, 4); ring(TARGET, 0.5, '#f59e0b', 0.35); } if (u < 0.3) FX.syrupHit = false; }
      // casquinha (extensão igual à do laboratório)
      { const p = (t - FX.cone) / 1.0, e = p < 0.2 || p >= 1 ? 0 : p < 0.58 ? Math.sin((p - 0.2) / 0.38 * Math.PI / 2) : p < 0.76 ? 1 : 1 - (p - 0.76) / 0.24; chestCone.visible = e > 0.01; for (const b of backCones) b.visible = e > 0.01; if (e > 0.01) { chestCone.scale.set(0.35 + 0.65 * e, e, 0.35 + 0.65 * e); for (const b of backCones) b.scale.setScalar(e); } }
      // poça fervente (5,2 s) e derretido (6 s)
      const pd = t - FX.puddle, pdA = ss(0.2, 0.6, pd) * (1 - ss(4.6, 5.2, pd)), ml = t - FX.melt, mlA = ss(0, 0.5, ml) * (1 - ss(5.5, 6, ml));
      puddleD.visible = pdA > 0.01; if (puddleD.visible) { const R = 0.2 + 0.85 * pdA; puddleD.scale.set(R, R * 0.8, 1); if (Math.random() < 0.8) { const a = Math.random() * 6.283, d = Math.random() * R * 0.85; dot(new THREE.Vector3(Math.cos(a) * d, 0.03, 0.1 + Math.sin(a) * d * 0.8), new THREE.Vector3(0, 0.04, 0), 0.02, fl().foam, 0.35 + Math.random() * 0.3, 0, 0.9, 0.12); } if (Math.random() < 0.4) { const a = Math.random() * 6.283, d = Math.random() * R * 0.7; dot(new THREE.Vector3(Math.cos(a) * d, 0.1, 0.1 + Math.sin(a) * d * 0.8), new THREE.Vector3(0, 0.45, 0), 0.05, fl().foam, 0.9, 0, 0.3, 0.09); } }
      meltD.visible = mlA > 0.01; if (meltD.visible) { const R = 0.45 + 0.55 * mlA; meltD.scale.set(R, R * 0.85, 1); if (Math.random() < 0.65 * mlA) { const a = Math.random() * 6.283; dot(new THREE.Vector3(Math.cos(a) * 0.38, 0.35 + Math.random() * 0.35, Math.sin(a) * 0.32), new THREE.Vector3(0, -0.2, 0), 0.02 + Math.random() * 0.012, fl().cream, 1, 3); } if (Math.random() < 0.3 * mlA) dot(new THREE.Vector3((Math.random() - 0.5) * 0.7, 0.4 + Math.random() * 0.4, 0.15), new THREE.Vector3(0, 0.3, 0), 0.05, fl().foam, 0.8, 0, 0.3, 0.08); }
      meltK = Math.max(mlA, pdA * 0.45);
      // cereja
      { const p = (t - FX.cherry) / 1.5; inflate = p <= 0 || p >= 1 ? 0 : p < 0.16 ? 0.28 * ss(0, 0.16, p) : p < 0.32 ? 0.28 + 0.34 * ss(0.16, 0.32, p) : p < 0.48 ? 0.62 + 0.38 * ss(0.32, 0.48, p) : p < 0.56 ? 1 - 0.8 * ss(0.48, 0.56, p) : 0.2 * (1 - ss(0.8, 1, p));
        const f = (p - 0.48) / 0.3, cOn = f > 0 && f < 1; if (cOn) { cherry.position.set(0, MOUTH + (TARGET.y - MOUTH) * f, D(115) + (TARGET.z - D(115)) * f); cherry.rotation.x = f * 9; dot(cherry.position.clone().add(new THREE.Vector3((Math.random() - 0.5) * 0.1, (Math.random() - 0.5) * 0.1, -0.1)), new THREE.Vector3(), 0.03, Math.random() < 0.6 ? '#dc2626' : '#fecdd3', 0.35, 0); } cherry.visible = cOn;
        if (f >= 1 && !FX.cherryDone) { FX.cherryDone = true; FX.cherryHit = t; ring(TARGET, 0.95, '#ff4d6d', 0.4); burst(TARGET, 42, 2.8, 0.02, ['#991b1b', '#dc2626', '#ef4444'], 0.8, 4); } if (p < 0.4) FX.cherryDone = false;
        const h = (t - FX.cherryHit) / 2.4, hOn = h > 0 && h < 1; cherryD.visible = hOn; if (hOn) { const R = 0.32 * easeO(Math.min(1, h * 5)); cherryD.scale.set(Math.max(0.01, R * (1 - ss(0.8, 1, h))), Math.max(0.01, R * 0.8 * (1 - ss(0.8, 1, h))), 1); } }
      // sabores: vapor frio e bolhas de morango
      if (!on && FX.flavor === 'frozen' && Math.random() < 0.3) dot(new THREE.Vector3((Math.random() - 0.5) * 0.8, 0.2 + Math.random() * 0.7, 0.2), new THREE.Vector3(0, 0.2, 0), 0.05, '#bae6fd', 0.9, 0, 0.35, 0.07);
      if (!on && FX.flavor === 'revenge' && Math.random() < 0.45) { const a = Math.random() * 6.283; dot(new THREE.Vector3(Math.cos(a) * 0.4, 0.15 + Math.random() * 0.55, Math.sin(a) * 0.35), new THREE.Vector3(0, 0.45, 0), 0.016 + Math.random() * 0.02, Math.random() < 0.5 ? '#e11d48' : '#fb7185', 0.6, 0); }
    };
    const onFrame = c => {
      // creme molenga: o sino balança como gelatina (cisalhado a partir da base, a cabeça vai junto) e respira
      const t = c.t, sx = 0.035 * Math.sin(t * 2.2) + 0.012 * Math.sin(t * 5.3), sz = 0.025 * Math.sin(t * 1.6 + 1.3), e = 0.012 * Math.sin(t * 4.4);
      const mt = 1 + 0.2 * meltK + 0.14 * inflate, mh = 1 - 0.62 * meltK; shM.makeShear(0, 0, sx, sz, 0, 0); scM.makeScale((1 + e) * mt, (1 - 0.8 * e) * mh, (1 + e) * mt);
      body.matrix.copy(bodyM.multiplyMatrices(shM, scM).multiply(trM)); body.matrixWorldNeedsUpdate = true;
      const hN = (NECK - WAIST) * (1 - 0.8 * e);
      neck.position.y -= (NECK - WAIST) * (1 - mh); neck.scale.set(1 + 0.08 * meltK + 0.28 * inflate, 1 - 0.22 * meltK + 0.22 * inflate, 1 + 0.08 * meltK + 0.28 * inflate);
      neck.position.x = sx * hN; neck.position.z = sz * hN; neck.rotation.z -= sx; neck.rotation.x += sz;
      P.dripStep(t); for (const f of handSteps) f(t);
      for (const sx of [-1, 1]) {
        const G = P.gloves[sx], k = Math.min(1, Math.max(0, c.pose[sx][0] / 0.3));
        G.position.copy(hands[sx].position); G.position.y -= 0.34 * meltK; G.position.x *= 1 + 0.12 * meltK;
        const yq = new THREE.Quaternion().setFromAxisAngle(UPV, upper.rotation.y * 0.8);
        const dir = new THREE.Vector3(0, 1, 0.06).lerp(new THREE.Vector3(0, 0.25, 1), k).normalize().applyQuaternion(yq);   // em pé; no soco a bandeja tomba para a frente
        const zRef = FWD.clone().lerp(UPV.clone().negate(), k).applyQuaternion(yq), zA = zRef.addScaledVector(dir, -zRef.dot(dir)).normalize(), xA = new THREE.Vector3().crossVectors(dir, zA);
        G.quaternion.setFromRotationMatrix(new THREE.Matrix4().makeBasis(xA, dir, zA));
      }
      fxFrame(c);
    };
    P.drip = { body: P.bodyOL, head: P.headOL, hands: P.handOLs, cx: CX, s: S_PX, foot: FOOT };   // tools3d/export
    return { P, upper, WAIST, neck, NECK, hips, HIPS, hands, hipSway: 0.3, hand: { x: 0.7, y: Y(548), z: 0.04, yaw: -0.05 }, placeFoot: () => {}, setFace, skills, onFrame };
  },
};
