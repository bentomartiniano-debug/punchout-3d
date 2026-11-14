// Lady Espinha no modo vinil, modelada pela folha de modelo do usuário (assets3d/espinha/ref/espinha_turnaround.png:
// frente, perfil esquerdo, costas e perfil direito alinhados). Todas as medidas são em px DA FOLHA, lidas da vista de
// frente (x, y) e do perfil (z = x do perfil − 569, a frente para +z). Escala: 351 px = 1 m (a altura da coroa à sola
// bate com a do boneco do jogo); chão em y = 806, eixo do corpo em x = 213.
//  - cabeça: bloco verde bem arredondado (frente estufada, lados em barril, queixo afinando) com as costelas (frente, lados e costas), espinhos dourados dos lados;
//    rosto pintado: risco da testa em V, máscaras pretas, olhos amarelos, nariz de losango dourado em relevo, lábios
//  - coroa: dália de pétalas rosa em quatro voltas com o cone dourado e os cones pequenos no meio
//  - pescoço verde curto, colar de contas douradas com a conta rosa na frente
//  - tronco azul com busto (os riscos do peito e o umbigo eram do desenho 2D: no vinil o volume já mostra), cinto dourado
//  - quadril: sunga azul (cava contornada por argola dourada), coxas verdes com costelas, a placa rosa de moldura
//    dourada na frente com o botão e as três pontas embaixo; ligas douradas nos joelhos
//  - botas azuis com as três marcas douradas do lado de fora e a sola dourada (o bico de dentro azul)
// Sem braços, como no jogo: as luvas de cacto flutuam.
import { THREE, ss, rrect, INKS, flat, vcol, withOutline, legPlacer } from './core.js';
import { RoundedBoxGeometry } from 'three/addons/geometries/RoundedBoxGeometry.js';
import { mergeGeometries, mergeVertices } from 'three/addons/utils/BufferGeometryUtils.js';

const S_PX = 351, FOOT = 806, CX = 213;
const C = {
  cactus: '#08cf8f', rib: '#046b42', neck: '#06a86c', thigh: '#05b874',
  petal: ['#ff3a84', '#f7327b', '#ff4b91'], petalLt: '#fc7ab4', petalIn: '#c4205e', gold: '#ffdc2e',
  bead: '#ffe23f', beadPink: '#ff6fa3', blue: '#3fd2f0', brief: '#12c0ea', boot: '#05aad3',
  band: '#fbb21e', flap: '#f92561', frame: '#ffc223', mark: '#f2d024',
  mask: '#151c1a', eye: '#fbef50', pupil: '#2e291d', lip: '#04563a', lipDk: '#03402b', brow: '#03552f',
};
const col = h => ({ value: new THREE.Color(h) });
// medidas da folha (px)
const HEAD = { x0: 112, x1: 314, top: 252, bot: 437, z0: -69, z1: 94, r: 46 };
const HCX = (HEAD.x0 + HEAD.x1) / 2, HCZ = (HEAD.z0 + HEAD.z1) / 2;
const HBULGE = 14;   // quanto a frente do rosto estufa no meio (px)
// largura da cabeça na altura y: barril (mais larga no meio) e queixo afinando
const headW = y => { const v = (y - (HEAD.top + HEAD.bot) / 2) / ((HEAD.bot - HEAD.top) / 2); return 1 + 0.03 * (1 - v * v) - 0.1 * Math.max(0, v) ** 2; };
const CROWN = { x: 213, z: 10 };

export default {
  title: 'Lady Espinha',
  arena: 'espinha',   // abre na arena própria (core.js)
  units: [S_PX, FOOT, CX],
  faces: [['normal', 'Normal'], ['angry', 'Raiva'], ['pain', 'Dor'], ['ko', 'K.O.']],
  skills: [['signature', 'Assinatura'], ['slap', 'La Garra del Cacto'], ['flyer', 'Gran Salto Luchador'], ['veil', 'Velo de las Espinas'], ['needle', 'Estocada de Mandacaru'], ['wind', 'Viento del Desierto'], ['tears', 'Lágrimas de Frida']],
  hudExtra: 'Forma: <button id="bloom">Viva La Caatinga</button>',
  others: [['valeria', 'Valéria'], ['frog', 'King Frog'], ['joana', 'Joana'], ['marcov', 'Marcov']],
  async build({ root, U }) {
    const { X, Y, D } = U, P = {};
    const V = (x, y, z) => new THREE.Vector3(X(x), Y(y), D(z));
    const ol = (m, w = 0.009) => {   // contorno em volta de uma peça já posicionada
      const g = withOutline(m, w); g.position.copy(m.position); g.quaternion.copy(m.quaternion); g.scale.copy(m.scale);
      m.position.set(0, 0, 0); m.quaternion.identity(); m.scale.set(1, 1, 1); return g; };
    // padrão no shader em px da folha (vPx = x, y, z da folha; off = px do ponto (0,0,0) da malha)
    function pattern(mat, glsl, key, uniforms = {}, off = [CX, FOOT]) {
      const prev = mat.onBeforeCompile, k0 = mat.customProgramCacheKey();
      mat.onBeforeCompile = (sh, r) => {
        prev.call(mat, sh, r); Object.assign(sh.uniforms, uniforms);
        sh.vertexShader = 'varying vec3 vPx; varying vec3 vNo;\n' + sh.vertexShader.replace('#include <begin_vertex>', `#include <begin_vertex>
          vPx = vec3(position.x * ${S_PX.toFixed(1)} + ${off[0].toFixed(1)}, ${off[1].toFixed(1)} - position.y * ${S_PX.toFixed(1)}, position.z * ${S_PX.toFixed(1)}); vNo = normal;`);
        sh.fragmentShader = 'varying vec3 vPx; varying vec3 vNo;\n' + Object.entries(uniforms).map(([u, v]) => `uniform ${v.value && v.value.isTexture ? 'sampler2D' : v.value && v.value.isColor ? 'vec3' : 'float'} ${u};\n`).join('')
          + sh.fragmentShader.replace('#include <map_fragment>', '#include <map_fragment>\n' + glsl);
      };
      mat.customProgramCacheKey = () => k0 + '|' + key; return mat;
    }
    // sólido por seções: cada linha { y, pt(th) -> [x, z] } (th = 0 → +x, π/2 → frente); linhas de cima para baixo
    function sections(rows, NA = 64) {
      const pos = [], idx = [];
      for (const r of rows) for (let j = 0; j <= NA; j++) { const [x, z] = r.pt(j / NA * Math.PI * 2); pos.push(X(x), Y(r.y), D(z)); }
      for (let i = 0; i < rows.length - 1; i++) for (let j = 0; j < NA; j++) { const a = i * (NA + 1) + j, b = a + 1, c = a + NA + 1, d = c + 1; idx.push(a, b, c, b, d, c); }
      const g = new THREE.BufferGeometry(); g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3)); g.setIndex(idx); g.computeVertexNormals(); return g;
    }
    const sp = (v, pw) => Math.sign(v) * Math.pow(Math.abs(v), 2 / pw);
    // seção oval: meia largura a, frente zf, costas zb (positivos), centro cx, superelipse pw
    const oval = (cx, a, zf, zb, pw = 2, fz = null) => th => { const c = Math.cos(th), s = Math.sin(th); const x = cx + a * sp(c, pw); let z = (s >= 0 ? zf : zb) * sp(s, pw); if (fz) z = fz(x, z, s); return [x, z]; };
    const lerpT = (tab, y) => {   // tabela [[y, v...]] → valores interpolados
      if (y <= tab[0][0]) return tab[0].slice(1); const L = tab[tab.length - 1]; if (y >= L[0]) return L.slice(1);
      for (let i = 1; i < tab.length; i++) if (y <= tab[i][0]) { const [a, b] = [tab[i - 1], tab[i]], k = (y - a[0]) / (b[0] - a[0]); return a.slice(1).map((v, j) => v + (b[j + 1] - v) * k); } };
    const tube = (pts, r, NA = 10, closed = false) => new THREE.TubeGeometry(new THREE.CatmullRomCurve3(pts.map(p => V(...p)), closed, 'catmullrom', 0.5), Math.max(24, pts.length * 3), D(r), NA, closed);
    const cone = (r, h, color, seg = 16) => { const g = new THREE.ConeGeometry(D(r), D(h), seg); g.translate(0, D(h / 2), 0); return new THREE.Mesh(g, flat(color)); };
    const aim = (m, x, y, z, dir) => { m.position.copy(V(x, y, z)); m.quaternion.setFromUnitVectors(new THREE.Vector3(0, 1, 0), new THREE.Vector3(dir[0], -dir[1], dir[2]).normalize()); return m; };   // dir em px (y para baixo)

    // ================= grupos do motor: tronco (cintura), pescoço/cabeça, quadril
    const WAIST = Y(545), NECK = Y(440), HIPS = Y(569);
    const upper = new THREE.Group(); upper.position.y = WAIST; root.add(upper);
    const body = new THREE.Group(); body.position.y = -WAIST; upper.add(body);
    const neck = new THREE.Group(); neck.position.y = NECK - WAIST; upper.add(neck);
    const head = new THREE.Group(); head.position.y = -NECK; neck.add(head);
    const hips = new THREE.Group(); hips.position.y = HIPS; root.add(hips);
    const low = new THREE.Group(); low.position.y = -HIPS; hips.add(low);

    // ================= cabeça: bloco de cantos redondos; costelas no shader, rosto pintado de frente
    const FX0 = HEAD.x0 - 4, FX1 = HEAD.x1 + 4, FY0 = HEAD.top, FY1 = HEAD.bot + 4;   // retângulo da pintura (px da folha)
    const FW = 1024, FH = Math.round(FW * (FY1 - FY0) / (FX1 - FX0));
    const faceCanvas = f => {
      const c = document.createElement('canvas'); c.width = FW; c.height = FH; const g = c.getContext('2d');
      const k = FW / (FX1 - FX0); g.scale(k, k); g.translate(-FX0, -FY0); espinhaFace(g, f); return c; };
    const faceTex = f => { const t = new THREE.CanvasTexture(faceCanvas(f)); t.colorSpace = THREE.SRGBColorSpace; t.anisotropy = 8; return t; };
    const headU = { tFace: { value: faceTex('normal') }, uRib: col(C.rib) };
    {
      const g = new RoundedBoxGeometry(D(HEAD.x1 - HEAD.x0), D(HEAD.bot - HEAD.top), D(HEAD.z1 - HEAD.z0), 8, D(HEAD.r));
      g.translate(X(HCX), Y((HEAD.top + HEAD.bot) / 2), D(HCZ));
      // menos caixa: a frente estufa (rosto arredondado), os lados abrem em barril e o queixo afina um pouco
      { const p = g.attributes.position, hw = (HEAD.x1 - HEAD.x0) / 2, hh = (HEAD.bot - HEAD.top) / 2, my = (HEAD.top + HEAD.bot) / 2;
        for (let i = 0; i < p.count; i++) { let x = p.getX(i) * S_PX + CX, y = FOOT - p.getY(i) * S_PX, z = p.getZ(i) * S_PX;
          const u = (x - HCX) / hw, v = (y - my) / hh, fz = Math.max(0, (z - HCZ) / ((HEAD.z1 - HEAD.z0) / 2));
          x = HCX + (x - HCX) * headW(y); z += HBULGE * (1 - u * u) * (1 - 0.5 * v * v) * fz ** 2;
          p.setXYZ(i, X(x), Y(y), D(z)); }
        g.computeVertexNormals(); }
      // costelas: frente em x = ±28,5 e ±69; lados em z = −38, −4, 31, 66; costas em x = 0 e ±74 (como na folha)
      const m = pattern(flat(C.cactus), `{
        vec3 n = normalize(vNo); float dx = vPx.x - ${CX.toFixed(1)}, rib = 1e3;
        if (n.z > 0.7) { float a = abs(dx); rib = min(abs(a - 28.5), abs(a - 69.0)); }
        else if (n.z < -0.7) { float a = abs(dx); rib = min(a, abs(a - 74.0)); }
        else if (abs(n.x) > 0.7) { float z = vPx.z; rib = min(min(abs(z + 38.0), abs(z + 4.0)), min(abs(z - 31.0), abs(z - 66.0))); }
        float w = fwidth(rib) * 0.8; diffuseColor.rgb = mix(diffuseColor.rgb, uRib, 1.0 - smoothstep(1.3 - w, 1.3 + w, rib));
        if (n.z > 0.35) {   // pintura do rosto (projetada de frente)
          vec2 fuv = vec2((vPx.x - ${FX0.toFixed(1)}) / ${(FX1 - FX0).toFixed(1)}, 1.0 - (vPx.y - ${FY0.toFixed(1)}) / ${(FY1 - FY0).toFixed(1)});
          vec4 fc = texture2D(tFace, fuv); diffuseColor.rgb = mix(diffuseColor.rgb, fc.rgb, fc.a * smoothstep(0.35, 0.6, n.z)); }
        else if (abs(n.x) > 0.5 && vPx.z > -6.0) {   // o risco da testa continua pelos lados até o meio
          float d = abs(vPx.y - 313.0), w2 = fwidth(vPx.y) * 0.8; diffuseColor.rgb = mix(diffuseColor.rgb, uRib, 1.0 - smoothstep(1.4 - w2, 1.4 + w2, d)); } }`, 'espHead', headU);
      P.headMesh = new THREE.Mesh(g, m); head.add(withOutline(P.headMesh, 0.009));
    }
    // nariz: losango dourado em relevo (pirâmide de 4 faces), com os riscos da cruz
    { const g = new THREE.OctahedronGeometry(1, 0); g.scale(D(13), D(18), D(7)); const n = new THREE.Mesh(g, flat(C.gold)); n.material.flatShading = true;
      n.position.copy(V(213, 363, HEAD.z1 + HBULGE * 0.97 + 1)); head.add(ol(n, 0.005)); }
    // espinhos: três de cada lado (o de baixo menor) e dois nos cantos de trás, apontando para trás e para fora
    for (const s of [-1, 1]) {
      for (const [y, L, r] of [[316, 22, 10], [367, 22, 10], [415, 16, 8]]) head.add(ol(aim(cone(r, L, C.gold), CX + s * ((HEAD.x1 - CX) * headW(y) - 2), y, -10, [s, -0.08, 0.12]), 0.005));
      for (const y of [320, 372]) head.add(ol(aim(cone(9, 18, C.gold), CX + s * ((HEAD.x1 - CX) * headW(y) - 16), y, HEAD.z0 + 10, [s * 0.5, -0.05, -1]), 0.005));
    }

    // ================= rosto desenhado (px da folha): risco da testa em V, máscaras, olhos, lábios
    function espinhaFace(g, f) {
      const ink = (w, c = INKS) => { g.strokeStyle = c; g.lineWidth = w; g.lineCap = 'round'; g.lineJoin = 'round'; };
      const ang = f === 'angry', pain = f === 'pain', ko = f === 'ko';
      // testa: risco de lado a lado que desce em V no meio (na raiva o V afunda)
      const vy = ang ? 326 : pain ? 316 : 321;
      ink(3, C.brow); g.beginPath(); g.moveTo(FX0, 310); g.quadraticCurveTo(170, 311, 206, vy - 3); g.lineTo(213, vy); g.lineTo(220, vy - 3); g.quadraticCurveTo(256, 311, FX1, 310); g.stroke();
      // costelas: a pintura é por cima delas (o shader desenha as costelas; aqui só as peças do rosto)
      for (const s of [-1, 1]) {
        // máscara preta: borda de cima reta (raiva: desce para dentro), de baixo em arco
        const xo = 213 + s * 89, xi = 213 + s * 19, to = ang ? 318 : pain ? 326 : 323, ti = ang ? 333 : pain ? 322 : 328;
        g.fillStyle = C.mask; g.beginPath(); g.moveTo(xo, to); g.lineTo(xi, ti);
        g.bezierCurveTo(xi - s * 1, 348, 213 + s * 36, 363, 213 + s * 54, 363);
        g.bezierCurveTo(213 + s * 76, 363, xo + s * 4, 338, xo, to); g.closePath(); g.fill();
        // olho: anel amarelo com a pupila; dor = ">" "<" amarelo; K.O. = X amarelo
        const ex = 213 + s * 55, ey = 347;
        if (pain) { ink(4.5, C.eye); g.beginPath(); g.moveTo(ex + s * 10, ey - 8); g.lineTo(ex - s * 7, ey); g.lineTo(ex + s * 10, ey + 8); g.stroke(); continue; }
        if (ko) { ink(4.5, C.eye); g.beginPath(); g.moveTo(ex - 8, ey - 8); g.lineTo(ex + 8, ey + 8); g.moveTo(ex + 8, ey - 8); g.lineTo(ex - 8, ey + 8); g.stroke(); continue; }
        g.fillStyle = '#0d1210'; g.beginPath(); g.arc(ex, ey, 13.6, 0, 7); g.fill();
        g.fillStyle = C.eye; g.beginPath(); g.arc(ex, ey, 12.4, 0, 7); g.fill();
        g.fillStyle = C.pupil; g.beginPath(); g.arc(ex, ey, ang ? 4.2 : 5.6, 0, 7); g.fill();
      }
      // lábios verde-escuros: o de cima em M, o de baixo cheio, o risco no meio (raiva: dentes; dor: ondulada; K.O.: "oh")
      if (ang) { g.fillStyle = C.lipDk; rrect(g, 193, 396, 40, 16, 6); g.fill(); g.fillStyle = '#f2efe4'; rrect(g, 197, 400, 32, 8, 2); g.fill();
        ink(1.2); g.beginPath(); for (const x of [205, 213, 221]) { g.moveTo(x, 400); g.lineTo(x, 408); } g.stroke(); return; }
      if (pain) { ink(3.2, C.lipDk); g.beginPath(); g.moveTo(194, 405); for (let k = 0; k < 5; k++) g.quadraticCurveTo(198 + k * 8, k % 2 ? 410 : 400, 202 + k * 8, 405); g.stroke(); return; }
      if (ko) { g.fillStyle = C.lipDk; g.beginPath(); g.ellipse(213, 405, 9, 11, 0, 0, 7); g.fill(); g.fillStyle = '#1e0a10'; g.beginPath(); g.ellipse(213, 406, 5, 7, 0, 0, 7); g.fill(); return; }
      g.fillStyle = C.lip;
      g.beginPath(); g.moveTo(191, 405); g.bezierCurveTo(197, 398, 203, 391, 208, 393); g.quadraticCurveTo(213, 396, 218, 393); g.bezierCurveTo(223, 391, 229, 398, 236, 405); g.closePath(); g.fill();   // de cima
      g.beginPath(); g.moveTo(193, 406); g.bezierCurveTo(198, 418, 228, 418, 234, 406); g.closePath(); g.fill();   // de baixo
      g.fillStyle = 'rgba(255,255,255,0.16)'; g.beginPath(); g.ellipse(213, 411, 9, 2.6, 0, 0, 7); g.fill();   // brilho do lábio
      ink(1.6, C.lipDk); g.beginPath(); g.moveTo(190, 405); g.quadraticCurveTo(213, 407, 236, 405); g.stroke();
    }
    P.setFaceTex = f => { headU.tFace.value.dispose(); headU.tFace.value = faceTex(f); };

    // ================= coroa: dália em quatro voltas em volta do eixo (213, z 10), cone dourado e cones pequenos
    {
      const AX = CROWN.x, AZ = CROWN.z, KZ = 0.92;
      // miolo: domo rosa-escuro entre o alto da cabeça e o cone (pelas frestas aparece flor, nunca o vazio)
      { const g = new THREE.SphereGeometry(1, 40, 20, 0, Math.PI * 2, 0, Math.PI * 0.62); g.scale(D(112), D(56), D(103)); g.translate(X(AX), Y(244), D(AZ));
        head.add(new THREE.Mesh(g, flat(C.petalIn))); }
      // pétala-modelo: oval de ponta fina (mais larga a 42% da base), em concha
      let leaf = new THREE.SphereGeometry(1, 16, 12); leaf.deleteAttribute('uv'); leaf.deleteAttribute('normal'); leaf = mergeVertices(leaf, 1e-5); { const p = leaf.attributes.position, fu = u => Math.pow(u, 0.5) * Math.pow(1 - u, 1.15), fmax = fu(0.5 / 1.65);
        for (let i = 0; i < p.count; i++) { const y = p.getY(i), u = (y + 1) / 2, s0 = Math.sqrt(Math.max(1e-6, 1 - y * y)), f = fu(u) / fmax;
          p.setXYZ(i, p.getX(i) / s0 * f, y, p.getZ(i) / s0 * f * (1 - 0.5 * u)); } }   // ponta fina, base redonda, afina para a ponta
      const RINGS = [
        // base (r0, y), inclinação e comprimento tirados do contorno da folha (frente e perfil): as orelhas de cima
        // abrem subindo (ponta em ~(108, 165)); as outras voltas caem quase retas como gotas penduradas
        { y: 188, r0: 36, el: 18, n: 9, len: 76, w: 46, off: 0.5, curl: 0.05 },
        { y: 190, r0: 30, el: -60, n: 8, len: 62, w: 40, off: 0.5, curl: 0.05, light: 2 },   // em volta do cone; as duas da frente mais claras
        { y: 192, r0: 70, el: -40, n: 12, len: 80, w: 50, off: 0, curl: 0.06 },
        { y: 212, r0: 98, el: -62, n: 14, len: 72, w: 48, off: 0.5, curl: 0.05 },
        { y: 236, r0: 108, el: -78, n: 15, len: 58, w: 44, off: 0, curl: 0.04 },   // franja de baixo
      ];
      const geos = [], petals = [];
      RINGS.forEach((R, ri) => {
        const front = [];
        for (let k = 0; k < R.n; k++) front.push([k, Math.cos((k + R.off) / R.n * Math.PI * 2)]);
        const lights = new Set(front.sort((a, b) => b[1] - a[1]).slice(0, R.light || 0).map(e => e[0]));
        for (let k = 0; k < R.n; k++) {
          const az = Math.PI / 2 + (k + R.off) / R.n * Math.PI * 2;   // k = 0 na frente
          const rad = new THREE.Vector3(Math.cos(az), 0, Math.sin(az) * KZ).normalize(), el = R.el * Math.PI / 180;
          const base = new THREE.Vector3(AX + R.r0 * Math.cos(az), R.y, AZ + R.r0 * KZ * Math.sin(az));
          const dir = rad.clone().multiplyScalar(Math.cos(el)).add(new THREE.Vector3(0, -Math.sin(el), 0)).normalize();   // px: y para baixo
          const tang = new THREE.Vector3(-rad.z, 0, rad.x).normalize(), out = new THREE.Vector3().crossVectors(tang, dir).normalize();
          if (out.dot(rad) - out.y * 0.4 < 0) out.negate();   // face de fora: para fora e para cima
          const g = leaf.clone(); g.scale(R.w / 2, R.len / 2, R.w * 0.17);   // pétala gordinha (de qualquer lado parece larga, como no desenho)
          { const p = g.attributes.position, su = [], sid = [], pi = petals.length;
            for (let i = 0; i < p.count; i++) { const u = (p.getY(i) / (R.len / 2) + 1) / 2, xn = p.getX(i) / (R.w / 2);
              p.setZ(i, p.getZ(i) - 0.1 * R.w * xn * xn - R.curl * R.len * u * u);   // concha: beiradas e ponta viram para dentro
              su.push(u * u); sid.push(pi); }
            g.setAttribute('su', new THREE.Float32BufferAttribute(su, 1)); g.setAttribute('sid', new THREE.Float32BufferAttribute(sid, 1)); petals.push({ ring: ri, len: R.len }); }
          const toW = v => new THREE.Vector3(v.x, -v.y, v.z), c = base.clone().addScaledVector(dir, R.len / 2);
          const bx = toW(tang), by = toW(dir), bz = toW(out); if (new THREE.Vector3().crossVectors(bx, by).dot(bz) < 0) bx.negate();
          g.applyMatrix4(new THREE.Matrix4().makeBasis(bx, by, bz));
          g.scale(D(1), D(1), D(1)); g.translate(X(c.x), Y(c.y), D(c.z));
          const cl = new THREE.Color(lights.has(k) ? C.petalLt : C.petal[(k + ri) % 3]), ca = []; for (let i = 0; i < g.attributes.position.count; i++) ca.push(cl.r, cl.g, cl.b);
          g.setAttribute('color', new THREE.Float32BufferAttribute(ca, 3)); geos.push(g);
        }
      });
      const crown = mergeGeometries(geos); crown.computeVertexNormals();   // indexada: normais suaves por pétala (o contorno não racha)
      const cm = vcol();
      // contorno desenhado em cada pétala: onde a superfície vira de lado para quem olha, tinta (mesmo encostada em outra)
      cm.onBeforeCompile = sh => { sh.fragmentShader = sh.fragmentShader.replace('#include <opaque_fragment>', `
        { float rimK = 1.0 - abs(dot(normalize(geometryNormal), geometryViewDir)); outgoingLight = mix(outgoingLight, vec3(0.05, 0.03, 0.04), smoothstep(0.78, 0.86, rimK)); }
        #include <opaque_fragment>`); };
      cm.customProgramCacheKey = () => 'petalRim';
      P.crownGeo = crown; P.petals = petals; head.add(withOutline(new THREE.Mesh(crown, cm), 0.007));
      // cone dourado (ponta em y 135) e seis cones pequenos em volta da base
      head.add(ol(aim(cone(24, 51, C.gold, 28), AX, 186, AZ, [0, -1, 0]), 0.007));
      for (let k = 0; k < 6; k++) { const a = Math.PI / 2 + (k + 0.5) / 6 * Math.PI * 2; head.add(ol(aim(cone(7, 26, C.gold), AX + 27 * Math.cos(a), 190, AZ + 25 * Math.sin(a), [Math.cos(a) * 0.3, -1, Math.sin(a) * 0.3]), 0.005)); }
    }

    // ================= pescoço e colar
    { const rows = []; for (let y = 428; y <= 472; y += 4) rows.push({ y, pt: oval(213, 43, 43, 45) });
      body.add(withOutline(new THREE.Mesh(sections(rows, 48), flat(C.neck)), 0.008)); }
    { const N = 24;
      for (let k = 0; k < N; k++) { const a = Math.PI / 2 + k / N * Math.PI * 2, f = (1 + Math.sin(a)) / 2;
        const x = 213 + 52 * Math.cos(a), z = 8 + 52 * Math.sin(a), y = 449 + 17 * f * f;
        const b = new THREE.Mesh(new THREE.SphereGeometry(D(7.2), 14, 10), flat(C.bead)); b.position.copy(V(x, y, z)); body.add(ol(b, 0.004)); }
      const p = new THREE.Mesh(new THREE.SphereGeometry(D(9), 16, 12), flat(C.beadPink)); p.position.copy(V(213, 476, 64)); body.add(ol(p, 0.005)); }

    // ================= tronco: medidas da frente (meia largura) e do perfil (frente / costas), busto
    const TORSO = [[452, 18, 8, 16], [456, 40, 18, 26], [461, 57, 27, 33], [467, 65, 34, 37], [473, 77, 44, 40], [479, 83, 55, 43], [485, 86, 64, 44], [491, 87, 71, 45],
      [497, 87, 76, 45], [503, 87, 79, 45], [509, 86, 80, 44], [515, 84, 79, 42], [521, 82, 74, 40], [527, 79, 71, 37], [533, 76, 67, 34], [539, 73, 63, 31], [545, 72, 61, 30], [564, 71, 60, 30]];
    const TPW = 2.3;
    const bust = (x, y) => { const w = Math.exp(-(((y - 498) / 24) ** 2)), a = Math.abs(x - 213); return 1 - w * (0.1 - 0.12 * Math.exp(-(((a - 40) / 22) ** 2))); };
    const torsoPt = y => { const [a, zf, zb] = lerpT(TORSO, y); return oval(213, a, zf, zb, TPW, (x, z, s) => s > 0 ? z * bust(x, y) : z); };
    const torsoZ = (x, y) => { const [a] = lerpT(TORSO, y), c = Math.max(-1, Math.min(1, (x - 213) / a)), cc = Math.sign(c) * Math.pow(Math.abs(c), TPW / 2), s = Math.sqrt(Math.max(0, 1 - cc * cc)); return torsoPt(y)(Math.atan2(s, cc))[1]; };
    { const rows = []; for (let y = 452; y <= 564; y += 2) rows.push({ y, pt: torsoPt(y) });
      // alças amarelas pintadas no pano: duas faixas do cinto, por cima dos ombros, até o cinto nas costas (contínuas)
      const tm = pattern(flat(C.blue), `{ float d = abs(abs(vPx.x - 213.0) - 48.0), w = fwidth(vPx.x) * 0.8; diffuseColor.rgb = mix(diffuseColor.rgb, uBand, 1.0 - smoothstep(6.0 - w, 6.0 + w, d)); }`, 'espTop', { uBand: col(C.band) });
      body.add(withOutline(new THREE.Mesh(sections(rows, 72), tm), 0.009));
    }

    // ================= quadril: o maiô (recortado da união sunga ∪ coxas, só a parte acima da cava), cinto, argolas, placa;
    // as coxas são peças separadas presas nas pernas (andam no passo e saem pela cava sem deformar o maiô)
    const PELV = [[556, 74, 60, 34], [569, 80, 66, 40], [578, 84, 68, 46], [590, 84, 70, 54], [605, 82, 71, 59], [625, 76, 71, 61], [645, 66, 69, 59], [660, 54, 63, 52], [670, 40, 55, 42], [680, 24, 38, 26], [690, 8, 14, 8]];
    const SIL = [[556, 70], [569, 79], [578, 85], [590, 90], [602, 94], [614, 97], [626, 99], [638, 101], [650, 102], [690, 102]];   // borda de fora da coxa (frente)
    const legC = y => 54 + 7 * ss(575, 690, y), legZ = -2;
    const thighR = y => { const o = lerpT(SIL, y)[0]; return Math.max(8, o - legC(y)); };
    // dentro da sunga/coxas na altura y? (x, z em px)
    const inPelvis = (x, z, y) => { const [a, zf, zb] = lerpT(PELV, y), zz = z >= 0 ? zf : zb; if (((x - 213) / a) ** 2 + (z / zz) ** 2 <= 1) return true;
      const r = thighR(y), rz = Math.min(r, 45); for (const s of [-1, 1]) if (((x - 213 - s * legC(y)) / r) ** 2 + ((z - legZ) / rz) ** 2 <= 1) return true; return false; };
    // contorno da seção pela união: a partir do meio, o raio mais longe em cada direção (busca em passos)
    const unionPt = y => th => { const c = Math.cos(th), s = Math.sin(th); let r = 0; for (let t = 160; t > 0; t -= 0.5) if (inPelvis(213 + c * t, s * t, y)) { r = t; break; } return [213 + c * Math.max(r, 3), s * Math.max(r, 3)]; };
    // cava da sunga em volta de cada coxa: ângulo φ em volta do eixo da coxa (0 = para fora, +90° = frente, −90° = costas)
    const loopY = phi => { const d = phi * 180 / Math.PI;
      if (d >= 0) { const t = Math.max(0, Math.min(1, (d - 37) / 88)); return 570 + 108 * ss(0, 1, Math.pow(t, 0.7)); }
      const t = Math.max(0, Math.min(1, (-d - 52) / 68)); return 570 + 108 * ss(0, 1, Math.sqrt(t)); };
    {
      const rows = []; for (let y = 556; y <= 690; y += 2) rows.push({ y, pt: unionPt(y) });
      rows.push({ y: 691, pt: () => [213, 0] });
      const m = pattern(flat(C.thigh), `{
        float side = vPx.x >= ${CX.toFixed(1)} ? 1.0 : -1.0, lc = 54.0 + 7.0 * smoothstep(575.0, 690.0, vPx.y);
        float ox = (vPx.x - ${CX.toFixed(1)}) * side - lc, oz = vPx.z + 2.0, d = degrees(atan(oz, ox)), yl;
        if (d >= 0.0) { float t = clamp((d - 37.0) / 88.0, 0.0, 1.0); yl = 570.0 + 108.0 * smoothstep(0.0, 1.0, pow(t, 0.7)); }
        else { float t = clamp((-d - 52.0) / 68.0, 0.0, 1.0); yl = 570.0 + 108.0 * smoothstep(0.0, 1.0, sqrt(t)); }
        if (vPx.y < yl) diffuseColor.rgb = uBrief;
        else if (cos(8.0 * atan(oz, ox)) > 0.965) diffuseColor.rgb = uRib; }`, 'espHip', { uBrief: col(C.brief), uRib: col('#048a55') });
      const hg = sections(rows, 128), hp = hg.attributes.position, idx = hg.index.array, keep = [];
      const brief = i => { const x = hp.getX(i) * S_PX + CX, y = FOOT - hp.getY(i) * S_PX, z = hp.getZ(i) * S_PX, sd = x >= CX ? 1 : -1;
        return y < loopY(Math.atan2(z - legZ, (x - CX) * sd - legC(y))) + 3; };
      for (let t = 0; t < idx.length; t += 3) if (brief(idx[t]) || brief(idx[t + 1]) || brief(idx[t + 2])) keep.push(idx[t], idx[t + 1], idx[t + 2]);
      hg.setIndex(keep);   // a cava fica aberta: a coxa (presa na perna) sai por ela
      P.hipMesh = new THREE.Mesh(hg, m); low.add(withOutline(P.hipMesh, 0.009));
      // argola dourada na cava (por cima da costura azul/verde): pontos na superfície, afastados 2 px
      for (const s of [-1, 1]) {
        const pts = [], push = deg => { const phi = deg * Math.PI / 180, y = loopY(phi), lc = legC(y), dx = Math.cos(phi) * s, dz = Math.sin(phi);
          let t = 0; for (; t < 70; t += 0.5) if (!inPelvis(213 + s * lc + dx * t, legZ + dz * t, y)) break;
          pts.push([213 + s * lc + dx * (t + 2), y, legZ + dz * (t + 2)]); };
        for (let d = 30; d <= 130; d += 4) push(d);
        for (let d = -142; d <= -46; d += 4) push(d);
        const front = pts.slice(0, 26), back = pts.slice(26).reverse();
        low.add(new THREE.Mesh(tube(front, 3.6, 10), flat(C.band)), new THREE.Mesh(tube(back, 3.6, 10), flat(C.band)));
      }
    }
    // cinto dourado (com a borda de cima fechando até o tronco)
    { const rows = []; for (let y = 545; y <= 569; y += 2) rows.push({ y, pt: oval(213, 82, 71, 39, 2.3) });
      low.add(withOutline(new THREE.Mesh(sections(rows, 96), flat(C.band)), 0.008));
      const lid = [{ y: 545, pt: oval(213, 70, 59, 29, 2.3) }, { y: 545, pt: oval(213, 82, 71, 39, 2.3) }];
      const lm = flat(C.band); lm.side = THREE.DoubleSide; low.add(new THREE.Mesh(sections(lid, 96), lm)); }
    // placa da frente e a mesma replicada nas costas: moldura dourada, rosa no meio, botão, três pontas embaixo; cada uma
    // presa no cinto por cima e solta embaixo (balança com o quadril, como uma faixa de pano duro). A fivela fica no cinto.
    const box = (x0, x1, y0, y1, z0, z1, c, r = 2) => { const g = new RoundedBoxGeometry(D(x1 - x0), D(y1 - y0), D(z1 - z0), 2, D(r)); const m = new THREE.Mesh(g, flat(c)); m.position.copy(V((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2)); return m; };
    low.add(ol(box(184, 243, 552, 572, 75, 83, C.frame, 3), 0.006));   // fivela no cinto (fixa)
    const plates = [];
    for (const side of [1, -1]) {
      const fz = side > 0 ? 74 : 42, pv = V(213, 558, fz + 3), sw = new THREE.Group(); sw.position.copy(pv);
      const parts = [ol(box(186, 241, 553, 685, fz, fz + 6, C.frame, 3), 0.006), box(193, 234, 572, 674, fz + 5, fz + 7.5, C.flap, 1.5)];
      const bt = new THREE.Mesh(new THREE.SphereGeometry(D(10), 18, 14), flat(C.gold)); bt.scale.z = 0.7; bt.position.copy(V(214, 618, fz + 8)); parts.push(ol(bt, 0.005));
      for (const [x, L, r] of [[198, 20, 6], [213.5, 31, 8], [229, 20, 6]]) parts.push(ol(aim(cone(r, L, C.gold), x, 685, fz + 3, [0, 1, 0]), 0.005));
      for (const o of parts) { o.position.sub(pv); sw.add(o); }
      const holder = new THREE.Group(); if (side < 0) holder.rotation.y = Math.PI; holder.add(sw); low.add(holder);   // a de trás: a mesma placa virada
      if (side < 0) sw.rotation.x = -0.3;   // já nasce pendendo para fora (o onFrame faz o mesmo): o .glb exportado guarda isto como descanso
      plates.push({ sw, side, rest: side > 0 ? 0 : -0.3, th: 0, thv: 0, ph: 0, phv: 0 });   // a de trás pende para fora por cima do bumbum
    }

    // ================= pernas (do quadril ao tornozelo, com a liga dourada) e botas
    const LEG_TOP = 592, SHIN_TOP = 640, ANK = 705, LX = { [-1]: 213 - 61, [1]: 213 + 61 }, legs = {}, feet = {};
    for (const s of [-1, 1]) {
      const x = LX[s], m = pattern(flat(C.thigh), `if (cos(8.0 * atan(vPx.z, vPx.x - ${x.toFixed(1)})) > 0.965) diffuseColor.rgb = uRib;`, 'leg' + s, { uRib: col('#048a55') }, [x, LEG_TOP]);
      const rows = []; for (let y = SHIN_TOP; y <= ANK + 18; y += 3) rows.push({ y, pt: oval(x, 43, 42, 44) });   // entra fundo na bota: inclinada no passo não descola
      const g = sections(rows, 48); g.translate(-X(x), -Y(LEG_TOP), 0);
      const leg = withOutline(new THREE.Mesh(g, m), 0.008); leg.position.set(X(x), Y(LEG_TOP), 0); root.add(leg); legs[s] = leg;
      // coxa: mesma superfície que a união tinha (eixo legC, raio até a silhueta), um tico por dentro do maiô
      { const tr = []; for (let y = 572; y <= 696; y += 2) { const yy = Math.min(y, 690), rr = thighR(yy) - 1.2, rz = Math.min(rr, 45) - 1; tr.push({ y, pt: oval(213 + s * legC(yy), rr, rz + legZ, rz - legZ) }); }
        const tg = sections(tr, 72); tg.translate(-X(x), -Y(LEG_TOP), 0);
        const tm = pattern(flat(C.thigh), `if (cos(8.0 * atan(vPx.z + 2.0, (vPx.x - ${x.toFixed(1)}) * ${s.toFixed(1)} + ${(61).toFixed(1)} - (54.0 + 7.0 * smoothstep(575.0, 690.0, vPx.y)))) > 0.965) diffuseColor.rgb = uRib;`, 'thigh' + s, { uRib: col('#048a55') }, [x, LEG_TOP]);
        leg.add(withOutline(new THREE.Mesh(tg, tm), 0.009)); }
      // liga dourada na boca da bota (vai com a bota: não descola quando a perna inclina no passo)
      const br = []; for (const y of [687, 689, 699, 702]) br.push({ y, pt: oval(x, y === 687 || y === 702 ? 46.5 : 48, y === 687 || y === 702 ? 46.5 : 48, y === 687 || y === 702 ? 49 : 50.5) });
      const bg = sections(br, 64); bg.translate(-X(x), -Y(ANK), 0); P.garters = P.garters || {}; P.garters[s] = (withOutline(new THREE.Mesh(bg, flat(C.band)), 0.006));
      // bota: cano reto, abre um pouco para fora embaixo, bico para a frente embaixo; sola dourada (o bico de dentro azul)
      const f = new THREE.Group(); f.position.set(X(x), Y(ANK), 0); root.add(f); feet[s] = f; f.add(P.garters[s]);
      const BOOT = [[699, 0, 42, 49, 49], [702, 0, 44, 51, 51], [740, 0.6, 44.5, 52, 52], [765, 1.4, 45.5, 54, 53], [775, 2, 46, 61, 54], [783, 2.4, 46.5, 66, 55]];
      const brows = []; for (let y = 699; y <= 783; y += 2) { const [dx, a, zf, zb] = lerpT(BOOT, y); brows.push({ y, pt: oval(x + s * dx, a, zf, zb, 2.3) }); }
      const bootG = sections(brows, 72); bootG.translate(-X(x), -Y(ANK), 0);
      const bm = pattern(flat(C.boot), `{   // marcas douradas: três "I" deitados no lado de fora, um pouco para a frente
        float th = atan(vPx.z, (vPx.x - ${x.toFixed(1)}) * ${s.toFixed(1)}), sArc = (th - 1.13) * 50.0;
        for (int i = 0; i < 3; i++) { float dy = vPx.y - (i == 0 ? 722.0 : i == 1 ? 743.0 : 764.0), sa = abs(sArc);
          bool bar = sa < 9.0 && abs(dy) < 2.1, cap = sa > 6.5 && sa < 11.0 && abs(dy) < 5.0;
          if ((bar || cap) && vPx.z > 0.0) diffuseColor.rgb = uMark; } }`, 'boot' + s, { uMark: col(C.mark) }, [x, ANK]);
      f.add(withOutline(new THREE.Mesh(bootG, bm), 0.008));
      { const lid = new THREE.Mesh(new THREE.CircleGeometry(1, 40), flat(C.thigh)); lid.rotation.x = -Math.PI / 2; lid.scale.set(D(45), D(50), 1); lid.position.copy(V(x, 703, 0)).sub(V(x, ANK, 0)); f.add(lid); }   // boca da bota fechada na cor da perna (sem buraco escuro)
      const SOLE = [[783, 46.5, 66, 55], [785, 49, 69, 57], [801, 49, 70, 57], [804, 46, 66, 54]];
      const srows = []; for (const [y, a, zf, zb] of SOLE) srows.push({ y, pt: oval(x + s * 2.4, a, zf, zb, 2.3) });
      srows.push({ y: 804.5, pt: () => [x + s * 2.4, 6] });
      const sg = sections(srows, 72); sg.translate(-X(x), -Y(ANK), 0);
      const sm = pattern(flat(C.band), `if (vPx.z > 4.0 && (vPx.x - ${(x + s * 2.4).toFixed(1)}) * ${s.toFixed(1)} < 0.0) diffuseColor.rgb = uBoot;`, 'sole' + s, { uBoot: col(C.boot) }, [x, ANK]);
      f.add(withOutline(new THREE.Mesh(sg, sm), 0.008));
    }
    const placeFoot = legPlacer({ legs, feet, hipX: { [-1]: X(LX[-1]), [1]: X(LX[1]) }, top: Y(LEG_TOP), bot: Y(ANK), hips, hipsY: HIPS });

    // ================= luvas de cacto (como no jogo): punho verde liso, espinhos dourados, punho dourado
    const L = (x, y, z) => new THREE.Vector3(x, y, z);
    function glove(sx) {
      const g = new THREE.Group();
      const fist = new THREE.Mesh(new RoundedBoxGeometry(0.25, 0.28, 0.24, 5, 0.095), flat(C.cactus)); fist.position.copy(L(0, 0.025, 0)); g.add(ol(fist, 0.010));
      const cuff = new THREE.Mesh(new THREE.CylinderGeometry(0.105, 0.105, 0.055, 28), flat(C.band)); cuff.position.copy(L(0, -0.14, 0)); g.add(ol(cuff, 0.007));
      for (const [x, y] of [[0.135, 0.08], [-0.135, 0.08], [0.135, -0.035], [-0.135, -0.035], [0.07, 0.175], [-0.07, 0.175]]) {
        const c2 = new THREE.ConeGeometry(0.014, 0.055, 10); c2.translate(0, 0.0275, 0); const m = new THREE.Mesh(c2, flat(C.gold));
        m.position.set(x, y, 0); m.quaternion.setFromUnitVectors(new THREE.Vector3(0, 1, 0), new THREE.Vector3(Math.sign(x), y > 0.15 ? 1 : 0.3, 0).normalize()); g.add(ol(m, 0.004));
      }
      return g;
    }
    P.gloves = { [-1]: glove(-1), [1]: glove(1) }; root.add(P.gloves[-1], P.gloves[1]);
    const hands = { [-1]: new THREE.Object3D(), [1]: new THREE.Object3D() }; root.add(hands[-1], hands[1]);
    const UPV = new THREE.Vector3(0, 1, 0), FWD = new THREE.Vector3(0, 0, 1);

    const setFace = f => P.setFaceTex(f);
    // coroa com mola: cada pétala puxada pela aceleração da cabeça (as de cima mais moles) e uma brisa leve
    const hp = new THREE.Vector3(), hPrev = new THREE.Vector3(), hVel = new THREE.Vector3(), hAcc = new THREE.Vector3(), hq = new THREE.Quaternion();
    let hInit = false, crownBase = null;
    const springs = P.petals.map((pt, i) => ({ o: new THREE.Vector3(), v: new THREE.Vector3(), k: 40 + pt.ring * 6 + (i % 3) * 7, max: D(pt.len * 0.18) }));
    function crownStep(c) {
      const dt = Math.min(0.05, Math.max(c.dt || 1 / 60, 1e-3)), g = P.crownGeo, p = g.attributes.position, su = g.attributes.su.array, sid = g.attributes.sid.array;
      if (!crownBase) crownBase = Float32Array.from(p.array);
      head.updateWorldMatrix(true, false); head.getWorldPosition(hp); if (!hInit) { hPrev.copy(hp); hInit = true; }
      const vel = hp.clone().sub(hPrev).divideScalar(dt); hAcc.copy(vel).sub(hVel).divideScalar(dt); hVel.copy(vel); hPrev.copy(hp);
      head.getWorldQuaternion(hq).invert(); const A = hAcc.clone().applyQuaternion(hq).clampLength(0, 40);
      springs.forEach((st, i) => { st.v.addScaledVector(st.o, -st.k * dt).addScaledVector(st.v, -4.5 * dt).addScaledVector(A, -1.1 * dt);
        st.v.x += Math.sin(c.t * 1.7 + i * 0.9) * 0.01 * dt * st.k; st.v.z += Math.cos(c.t * 1.3 + i * 1.3) * 0.01 * dt * st.k;
        st.o.addScaledVector(st.v, dt); if (st.o.length() > st.max) st.o.setLength(st.max); });
      const a = p.array;
      for (let i = 0, n = su.length; i < n; i++) { const w = su[i], o = springs[sid[i]].o; a[i * 3] = crownBase[i * 3] + o.x * w; a[i * 3 + 1] = crownBase[i * 3 + 1] + o.y * w; a[i * 3 + 2] = crownBase[i * 3 + 2] + o.z * w; }
      p.needsUpdate = true;
    }
    const kp = new THREE.Vector3(), kPrev = new THREE.Vector3(), kVel = new THREE.Vector3(), kq = new THREE.Quaternion(); let kInit = false;
    function platesStep(c) {
      const dt = Math.min(0.05, Math.max(c.dt || 1 / 60, 1e-3));
      hips.updateWorldMatrix(true, false); hips.getWorldPosition(kp); if (!kInit) { kPrev.copy(kp); kInit = true; }
      const v = kp.clone().sub(kPrev).divideScalar(dt), A = v.clone().sub(kVel).divideScalar(dt); kVel.copy(v); kPrev.copy(kp);
      hips.getWorldQuaternion(kq).invert(); A.applyQuaternion(kq).clampLength(0, 30);
      for (const P2 of plates) {   // mola: inclina para trás/frente (x) e para o lado (z) com a aceleração do quadril, brisa leve
        P2.thv += (-60 * P2.th - 6 * P2.thv + A.z * P2.side * 1.4 + Math.sin(c.t * 2.3 + P2.side) * 0.6) * dt; P2.th += P2.thv * dt;
        P2.phv += (-60 * P2.ph - 6 * P2.phv - A.x * P2.side * 1.4 + Math.sin(c.t * 1.7 + P2.side * 2) * 0.4) * dt; P2.ph += P2.phv * dt;
        P2.th = Math.max(-0.5, Math.min(0.04, P2.th)); P2.ph = Math.max(-0.35, Math.min(0.35, P2.ph));   // nunca entra no corpo
        P2.sw.rotation.set(P2.rest + P2.th, 0, P2.ph); }
    }
    // ================= especiais (os do jogo, skills.cpp e skill_poses.cpp, com os efeitos do laboratório, vfx/lab_espinha.cpp)
    const easeO = k => 1 - (1 - k) ** 3, win = (a, b, x) => Math.max(0, Math.min(1, (x - a) / (b - a))), DIST = 1.75, CHEST = Y(490);
    const CACT = '#16a34a', SAND = '#d6b36a', SANDD = '#92400e', DAHL = '#e11d48', MAG = '#db2777', GOLD = '#facc15', SPINE = '#fef3c7';
    const fxRoot = new THREE.Group(); root.add(fxRoot);
    const glow = (col, o = 1) => new THREE.MeshBasicMaterial({ color: col, transparent: true, opacity: o, depthWrite: false, side: THREE.DoubleSide });
    const queue = []; let now = 0; const after = (dt, fn) => queue.push({ at: now + dt, fn });
    const FX = { sign: -99, slap: -99, flyer: -99, veil: -99, needle: -99, wind: -99, tears: -99, crack: -99, gust: -99 };
    // bolinhas: poeira, areia, confete, lágrimas e pétalas redondas
    const dotG = new THREE.SphereGeometry(1, 10, 8), dots = Array.from({ length: 240 }, () => { const m = new THREE.Mesh(dotG, glow('#fff')); m.visible = false; fxRoot.add(m); return { m, life: 0, v: new THREE.Vector3() }; });
    let dotI = 0; const dot = (p, v, rr, col, life, g = 3, op = 1, grow = 0) => { const q = dots[dotI++ % dots.length]; q.m.position.copy(p); q.m.scale.setScalar(rr); q.v.copy(v); q.r = rr; q.m.material.color.set(col); q.m.material.opacity = op; q.op = op; q.life = q.life0 = life; q.g = g; q.grow = grow; q.m.visible = true; };
    const dust = (x, z, n, pw) => { for (let i = 0; i < n; i++) { const a = Math.random() * 6.283, sp = pw * (0.4 + Math.random() * 0.7); dot(new THREE.Vector3(x + Math.cos(a) * 0.1, 0.05, z + Math.sin(a) * 0.1), new THREE.Vector3(Math.cos(a) * sp, 0.3 + Math.random() * 0.4, Math.sin(a) * sp), 0.04, i % 2 ? SAND : '#fef3c7', 0.75, 0, 0.75, 0.2); } };
    const burst = (p, n, spd, rr, cols, life, g = 3) => { for (let i = 0; i < n; i++) { const a = Math.random() * 6.283, sp = spd * (0.5 + Math.random()); dot(p, new THREE.Vector3(Math.cos(a) * sp, Math.sin(a) * sp * 0.8 + 0.4, (Math.random() - 0.5) * spd), rr * (0.6 + Math.random() * 0.7), cols[i % cols.length], life * (0.7 + Math.random() * 0.6), g); } };
    const ringG = new THREE.RingGeometry(0.84, 1, 40), rings = Array.from({ length: 12 }, () => { const m = new THREE.Mesh(ringG, glow('#fff')); m.visible = false; fxRoot.add(m); return { m, life: 0 }; });
    let ringI = 0; const ring = (p, R, col, life, flatR = false) => { const q = rings[ringI++ % rings.length]; q.m.position.copy(p); q.R = R; q.life = q.life0 = life; q.m.material.color.set(col); q.flat = flatR; q.m.rotation.set(flatR ? -Math.PI / 2 : 0, 0, 0); q.m.scale.setScalar(0.02); q.m.visible = true; };
    // espinhos voando: cones dourados que apontam para onde vão
    const spineG = new THREE.ConeGeometry(0.012, 0.11, 6), spines = Array.from({ length: 40 }, () => { const m = new THREE.Mesh(spineG, flat(SPINE)); m.visible = false; fxRoot.add(m); return { m, life: 0, v: new THREE.Vector3() }; });
    let spI = 0; const spine = (p, v, life = 0.5, g = 2) => { const q = spines[spI++ % spines.length]; q.m.position.copy(p); q.v.copy(v); q.life = q.life0 = life; q.g = g; q.m.visible = true; };
    const spineBurst = (p, n) => { for (let i = 0; i < n; i++) { const a = Math.random() * 6.283, sp = 1.4 + Math.random() * 1.6; spine(p, new THREE.Vector3(Math.cos(a) * sp, Math.sin(a) * sp * 0.7 + 0.4, (Math.random() - 0.5) * 1.5), 0.45); } };
    const UPY = new THREE.Vector3(0, 1, 0);
    // estrelas de lucha
    const starG = (() => { const sh = new THREE.Shape(); for (let i = 0; i < 10; i++) { const a = i / 10 * 6.283 + Math.PI / 2, q = i % 2 ? 0.022 : 0.055; i ? sh.lineTo(Math.cos(a) * q, Math.sin(a) * q) : sh.moveTo(Math.cos(a) * q, Math.sin(a) * q); } sh.closePath(); return new THREE.ExtrudeGeometry(sh, { depth: 0.012, bevelEnabled: false }); })();
    const stars = Array.from({ length: 22 }, () => { const m = new THREE.Mesh(starG, flat(GOLD)); const o = withOutline(m, 0.003); o.visible = false; fxRoot.add(o); return { o, m, life: 0, v: new THREE.Vector3() }; });
    let stI = 0; const star = (p, v, sz, col, life, g = 0) => { const q = stars[stI++ % stars.length]; q.o.position.copy(p); q.v.copy(v); q.sz = sz; q.life = q.life0 = life; q.g = g; q.m.material.color.set(col); q.o.visible = true; };
    // arco do tapa
    const slashes = [-1, 1].map(() => { const m = new THREE.Mesh(new THREE.TorusGeometry(0.34, 0.03, 6, 26, 2.3), glow(SPINE, 0.9)); m.visible = false; fxRoot.add(m); return m; }); const slashT = { [-1]: -99, [1]: -99 };
    // rachaduras do salto
    const crack = new THREE.Group(); crack.position.set(0, 0.014, DIST); crack.rotation.x = -Math.PI / 2; crack.visible = false; fxRoot.add(crack);
    { crack.add(new THREE.Mesh(new THREE.CircleGeometry(0.2, 18), glow('#3b1a08', 0.8))); for (let i = 0; i < 9; i++) { const a = i / 9 * 6.283 + (i % 2) * 0.2, L = 0.42 + (i * 7 % 5) * 0.07, g = new THREE.PlaneGeometry(L, 0.03); g.translate(L / 2, 0, 0); const m = new THREE.Mesh(g, glow('#3b1a08', 0.85)); m.rotation.z = a; crack.add(m); } }
    // véu: casca de espinhos dourados em volta dela (dois sentidos), mais anel verde pulsando
    const veil = new THREE.Group(); veil.visible = false; fxRoot.add(veil); const veilOut = new THREE.Group(), veilIn = new THREE.Group(); veil.add(veilOut, veilIn);
    { const g = new THREE.ConeGeometry(0.022, 0.2, 6), mt = flat(SPINE); g.translate(0, 0.1, 0); for (let i = 0; i < 64; i++) { const y = 1 - (i + 0.5) / 64 * 1.84, rr = Math.sqrt(Math.max(0, 1 - y * y)), a = i * 2.399, d = new THREE.Vector3(Math.cos(a) * rr, y, Math.sin(a) * rr), o = withOutline(new THREE.Mesh(g, mt), 0.003); o.position.set(d.x * 0.6, 0.9 + d.y * 0.86, d.z * 0.5); o.quaternion.setFromUnitVectors(UPY, d); (i % 2 ? veilOut : veilIn).add(o); } }
    // estocada: espinho gigante (cone com brilho verde) que voa até o rival
    const needle = new THREE.Group(); needle.visible = false; fxRoot.add(needle); { const g = new THREE.ConeGeometry(0.07, 0.8, 10); g.rotateX(Math.PI / 2); needle.add(withOutline(new THREE.Mesh(g, flat(SPINE)), 0.006)); const gl = new THREE.Mesh(new THREE.SphereGeometry(0.14, 12, 8), glow(CACT, 0.4)); gl.scale.z = 2.6; gl.position.z = -0.1; needle.add(gl); }
    // flores: dália em dois anéis de pétalas com miolo; usada na forma Viva La Caatinga e nas Lágrimas
    const flowerOf = (col, deep, core, R) => { const g = new THREE.Group(), pm = flat(col), dm = flat(deep), cm = flat(core), pg = new THREE.SphereGeometry(1, 10, 8); const add = (n, rr, L, W, m, z) => { for (let i = 0; i < n; i++) { const a = i / n * 6.283 + z * 2, q = new THREE.Mesh(pg, m); q.scale.set(W, L, 0.28 * W + 0.01); q.position.set(Math.sin(a) * rr, Math.cos(a) * rr, z); q.rotation.z = -a; g.add(withOutline(q, 0)); q.parent.position.copy(q.position); q.parent.quaternion.copy(q.quaternion); q.parent.scale.copy(q.scale); q.position.set(0, 0, 0); q.quaternion.identity(); q.scale.set(1, 1, 1); } }; add(10, R * 0.68, R * 0.42, R * 0.26, pm, 0); add(7, R * 0.38, R * 0.3, R * 0.2, dm, R * 0.14); const c = new THREE.Mesh(new THREE.SphereGeometry(R * 0.2, 10, 8), cm); c.position.z = R * 0.28; g.add(c); return g; };
    root.updateMatrixWorld(true);
    const crownMesh = head.children.find(o => o.children?.[0]?.geometry === P.crownGeo); P.crownGeo.computeBoundingBox(); const cb = P.crownGeo.boundingBox, cTop = cb.max.y, cW = (cb.max.x - cb.min.x) / 2, cZ = (cb.max.z + cb.min.z) / 2;
    // Viva La Caatinga (sprite Bloom): a coroa vira um arranjo de flores grandes coloridas; margaridas nos ombros e nas coxas
    const crownBits = head.children.filter(o => o === crownMesh || new THREE.Box3().setFromObject(o).min.y > Y(250));   // tudo da coroa de pétalas (miolo, pétalas e cones)
    const bloomParts = []; const bloomHead = new THREE.Group(); bloomHead.visible = false; head.add(bloomHead); bloomParts.push(bloomHead);
    [[-0.88, -0.06, '#a855f7', '#7c3aed', 0.095, 0.1], [-0.5, 0.26, '#ff007f', '#be0060', 0.115, 0.22], [-0.1, 0.42, '#e11d48', '#9f1239', 0.115, 0.15], [0.26, 0.44, '#ffffff', '#e5e7eb', 0.105, 0.22], [0.62, 0.26, '#ffbe0b', '#fb923c', 0.115, 0.16], [0.92, -0.04, '#fb923c', '#ea580c', 0.095, 0.06], [-0.22, 0.14, '#ffbe0b', '#fb923c', 0.09, 0.34], [0.2, 0.12, '#ff007f', '#be0060', 0.09, 0.36], [-0.34, 0.36, '#fb923c', '#ea580c', 0.08, -0.16], [0.36, 0.36, '#a855f7', '#7c3aed', 0.08, -0.16], [0, 0.3, '#ff007f', '#be0060', 0.1, -0.24], [-0.7, 0.12, '#ffbe0b', '#fb923c', 0.095, -0.1], [0.7, 0.12, '#e11d48', '#9f1239', 0.095, -0.1], [-0.4, 0.1, '#ffffff', '#e5e7eb', 0.09, -0.3], [0.42, 0.1, '#fb923c', '#ea580c', 0.09, -0.3], [0, 0.55, '#ffbe0b', '#fb923c', 0.1, 0.0]].forEach(([x, y, c1, c2, R, z]) => { const f = flowerOf(c1, c2, '#ffbe0b', R); f.position.set(cW * x * 1.15, cTop - 0.2 + y * 0.12, cZ + z); if (z < 0) f.rotation.y = Math.PI; f.rotation.x = z < 0 ? 0.5 : -0.35; bloomHead.add(f); });
    const daisy = () => flowerOf('#ffffff', '#f3f4f6', '#ffbe0b', 0.045);
    for (const sd of [-1, 1]) { const a = daisy(); a.position.copy(V(213 + sd * 78, 450, 34)); a.visible = false; body.add(a); bloomParts.push(a); const b = daisy(); b.position.set(0, -0.08, D(32)); b.visible = false; legs[sd].add(b); bloomParts.push(b); }
    const skinOf = new THREE.Color('#ffffff');
    let bloomOn = false; const setBloom = on => { bloomOn = on; for (const b of bloomParts) b.visible = on; for (const o of crownBits) o.visible = !on; burst(new THREE.Vector3(0, 1.55, 0), 22, 1.6, 0.018, ['#ff007f', '#ffbe0b', '#ffffff', '#a855f7'], 0.8, 2); };
    const bloomB = document.querySelector('#bloom'); if (bloomB) bloomB.onclick = () => { setBloom(!bloomOn); bloomB.style.outline = bloomOn ? '2px solid #fc6' : ''; };
    // lágrimas: dálias que abrem em volta da cabeça
    const tearFlowers = [0, 1, 2, 3, 4, 5].map(i => { const f = flowerOf(i % 2 ? DAHL : MAG, '#831843', GOLD, 0.08); f.visible = false; fxRoot.add(f); return { f, a: i / 6 * 6.283 }; });
    const skills = {
      // assinatura (laboratório): punhos sobem no duplo bíceps; ela curva a cabeça e sacode a coroa, as flores balançando atrasadas
      signature: { dur: 3.0, start: () => { FX.sign = now; setFace('angry'); after(3.0, () => setFace('normal')); },
        pose: (k, pose) => { const p = k / 3, rise = ss(0.02, 0.2, p) * (1 - ss(0.8, 0.96, p)), fl = 0.03 * Math.sin(k * 16) * rise; pose[-1] = [-0.14 * rise, 0.4 * rise + fl, -0.5 * rise, 0, 0]; pose[1] = [-0.14 * rise, 0.4 * rise - fl, -0.5 * rise, 0, 0]; return { drop: 0.03 * rise, lean: 0.08 * rise }; } },
      // dois tapas de cacto em gancho: um em 0,35 e o outro em 0,58, com o arco e os espinhos
      slap: { dur: 1.1, start: () => { FX.slap = now; setFace('angry'); after(1.1, () => setFace('normal')); [[-1, 0.385], [1, 0.638]].forEach(([sd, d]) => after(d, () => { slashT[sd] = now; const p = new THREE.Vector3(sd * 0.1, CHEST + 0.05, 0.98); burst(p, 10, 1.3, 0.016, [SPINE, CACT], 0.5); spineBurst(p, 12); })); },
        pose: (k, pose) => { const p = k / 1.1, s1 = Math.sin(win(0.18, 0.48, p) * Math.PI), s2 = Math.sin(win(0.42, 0.74, p) * Math.PI); pose[-1] = [0.6 * s1, 0.1 * s1, (-0.4 + 1.6 * win(0.18, 0.48, p)) * (s1 > 0 ? 1 : 0), 0, 0]; pose[1] = [0.6 * s2, 0.1 * s2, (-0.4 + 1.6 * win(0.42, 0.74, p)) * (s2 > 0 ? 1 : 0), 0, 0]; return { twist: 0.4 * (s2 - s1), lean: 0.14 * Math.max(s1, s2) }; } },
      // gran salto: agacha, decola com poeira, voa dando mortal com estrelas e confete e cai no rival em 0,82; volta ao lugar
      flyer: { dur: 1.7, start: () => { FX.flyer = now; setFace('angry'); after(1.7, () => setFace('normal')); after(0.2, () => { dust(0, 0, 12, 1); ring(new THREE.Vector3(0, 0.015, 0), 0.95, SAND, 0.45, true); }); after(1.394, () => { FX.crack = now; dust(0, DIST, 18, 1.6); burst(new THREE.Vector3(0, 0.4, DIST), 20, 2.4, 0.02, [GOLD, MAG], 0.8); ring(new THREE.Vector3(0, 0.015, DIST), 1.6, GOLD, 0.6, true); for (let i = 0; i < 5; i++) star(new THREE.Vector3((Math.random() - 0.5) * 0.7, 1.3 + Math.random() * 0.4, DIST), new THREE.Vector3((Math.random() - 0.5) * 2, 1 + Math.random() * 2, 0), 1.4, GOLD, 1, 2); }); },
        pose: (k, pose) => { const p = k / 1.7, cr = ss(0, 0.12, p) * (1 - ss(0.12, 0.2, p)), air = ss(0.12, 0.25, p) * (1 - ss(0.75, 0.82, p)), hit = ss(0.78, 0.82, p) * (1 - ss(0.9, 1, p)), v = [0.1 * air, 0.32 * air - 0.1 * cr - 0.2 * hit, 0.3 * air, 0, 0]; pose[-1] = v; pose[1] = [...v]; return { drop: 0.1 * cr + 0.1 * hit, lean: 0.22 * hit }; } },
      veil: { dur: 1.0, start: () => { FX.veil = now; after(0.5, () => { const p = new THREE.Vector3(0, 0.9, 0); ring(p, 1.1, CACT, 0.5); spineBurst(p, 26); }); },
        pose: (k, pose) => { const b = ss(0.4, 0.6, k), cross = 1 - b, e = ss(0, 0.25, k) * (1 - ss(0.75, 1, k)), up = 0.1 * e; pose[-1] = [(0.1 * cross) * e, up, (1.1 * cross - 0.4 * b) * e, 0, 0]; pose[1] = [(0.1 * cross) * e, up, (1.1 * cross - 0.4 * b) * e, 0, 0]; return { drop: 0.06 * cross * e, lean: 0.1 * cross * e - 0.08 * b * e }; } },
      needle: { dur: 0.9, start: () => { FX.needle = now; setFace('angry'); after(0.9, () => setFace('normal')); after(0.27, () => {}); after(0.47, () => { const p = new THREE.Vector3(0, CHEST + 0.05, DIST - 0.2); burst(p, 12, 1.6, 0.016, [SPINE, CACT], 0.5); spineBurst(p, 12); ring(p, 0.45, SPINE, 0.3); }); },
        pose: (k, pose) => { const q = Math.sin(win(0.15, 1, k / 0.9) * Math.PI); pose[1] = [0.72 * q, 0.06 * q, 0.82 * q, 0, 0]; pose[-1] = [0, -0.03 * q, 0, 0, 0]; return { lean: 0.2 * q, twist: -0.24 * q, drop: 0.03 * q }; } },
      // viento: rodopio rasteiro, as luvas orbitam o corpo agachado e o redemoinho de areia parte para o rival
      wind: { dur: 1.3, start: () => { FX.wind = now; after(0.52, () => { FX.gust = now; dust(0, DIST, 14, 1.3); }); },
        pose: (k, pose) => { const p = k / 1.3, a = p * Math.PI * 6, e = ss(0, 0.12, p) * (1 - ss(0.82, 1, p)); pose[-1] = [0.24 * Math.sin(a) * e, -0.18 * e, (1 - Math.cos(a)) * 0.4 * e, 0, 0]; pose[1] = [-0.24 * Math.sin(a) * e, -0.18 * e, (1 + Math.cos(a)) * 0.4 * e, 0, 0]; return { drop: 0.11 * e, twist: a * e, lean: 0.14 * e }; } },
      // lágrimas de Frida: mãos no rosto chorando ouro, depois abertas para o céu e as dálias abrem
      tears: { dur: 1.5, start: () => { FX.tears = now; setFace('pain'); after(0.75, () => { setFace('normal'); burst(new THREE.Vector3(0, 1.2, 0.1), 14, 1.3, 0.016, [GOLD, DAHL], 0.8); }); },
        pose: (k, pose) => { const p = k / 1.5, op = ss(0.5, 0.75, p), e = ss(0, 0.2, p) * (1 - ss(0.85, 1, p)), v = [(-0.02 - 0.05 * op) * e, (0.36 + 0.2 * op) * e, (1.0 - 1.45 * op) * e, 0, 0]; pose[-1] = v; pose[1] = [...v]; return { lean: 0.1 * (1 - op) * e - 0.1 * op * e, tilt: 0.04 * Math.sin(k * 18) * (1 - op) * e }; } },
    };
    const fxFrame = c => {
      now = c.t; const t = now, dt = c.dt, cam = window.VIN?.camera;
      for (let i = queue.length - 1; i >= 0; i--) if (now >= queue[i].at) { queue[i].fn(); queue.splice(i, 1); }
      for (const q of dots) { if (q.life <= 0) { q.m.visible = false; continue; } q.life -= dt; q.v.y -= q.g * dt; q.m.position.addScaledVector(q.v, dt); const u = q.life / q.life0; if (q.grow) { q.r += q.grow * dt; q.m.scale.setScalar(q.r); q.m.material.opacity = q.op * u; } else q.m.scale.setScalar(q.r * Math.max(0.05, u)); if (q.m.position.y < 0.01) q.life = 0; }
      for (const q of rings) { if (q.life <= 0) { q.m.visible = false; continue; } q.life -= dt; const u = 1 - q.life / q.life0, e = q.R * easeO(u) + 0.03; q.m.scale.set(e, e, 1); q.m.material.opacity = Math.max(0, 1 - u); if (cam && !q.flat) q.m.quaternion.copy(cam.quaternion); }
      for (const q of spines) { if (q.life <= 0) { q.m.visible = false; continue; } q.life -= dt; q.v.y -= q.g * dt; q.m.position.addScaledVector(q.v, dt); q.m.quaternion.setFromUnitVectors(UPY, q.v.clone().normalize()); if (q.m.position.y < 0.01) q.life = 0; }
      for (const q of stars) { if (q.life <= 0) { q.o.visible = false; continue; } q.life -= dt; q.v.y -= q.g * dt; q.o.position.addScaledVector(q.v, dt); q.o.rotation.z += dt * 6; q.o.scale.setScalar(q.sz * Math.min(1, q.life / q.life0 * 3)); if (cam) { const z = q.o.rotation.z; q.o.quaternion.copy(cam.quaternion); q.o.rotateZ(z); q.o.rotation.z = z; } if (q.o.position.y < 0.01) q.life = 0; }
      // assinatura: cabeça curva e sacode; a coroa solta com atraso
      { const p = (t - FX.sign) / 3; if (p > 0 && p < 1) { const bow = ss(0.04, 0.22, p) * (1 - ss(0.8, 0.96, p)), sh = win(0.06, 0.78, p), ph = sh * Math.PI * 12, inten = Math.sin(sh * Math.PI), sw = inten * (Math.sin(ph) + 0.35 * Math.sin(2 * ph)) * 0.9; neck.rotation.x += 0.72 * bow + 0.1 * inten * Math.sin(ph * 2); neck.rotation.z += 0.24 * sw; neck.rotation.y += 0.16 * sw; neck.position.y -= 0.03 * bow; for (const sd of [-1, 1]) { const G = P.gloves[sd]; G.position.x = hands[sd].position.x + sd * 0.1 * bow; G.scale.setScalar(1 + 0.12 * bow); } if (inten > 0.5 && Math.random() < 0.3) { const a = Math.random() * 6.283; dot(new THREE.Vector3(Math.cos(a) * 0.25 + 0.2 * sw, 1.35, 0.2 + Math.sin(a) * 0.2), new THREE.Vector3(Math.cos(a) * 0.8, 0.5, 0.4), 0.016, Math.random() < 0.5 ? MAG : DAHL, 0.8, 3); } } else { P.gloves[-1].scale.setScalar(1); P.gloves[1].scale.setScalar(1); } }
      // arcos dos tapas
      for (const [i, sd] of [[0, -1], [1, 1]]) { const u = (t - slashT[sd]) / 0.26, on = u > 0 && u < 1, m = slashes[i]; m.visible = on; if (on) { m.position.set(0, CHEST + 0.05, 0.95); m.rotation.set(0, 0, sd < 0 ? -0.2 : Math.PI + 0.2 - 0.9); m.scale.setScalar(0.7 + 0.5 * u); m.material.opacity = 0.9 * (1 - u); } }
      // salto: arco até o rival com mortal, depois volta correndo; pés encolhidos no ar
      { const p = (t - FX.flyer) / 1.7; if (p > 0 && p < 1) { const fl = win(0.12, 0.82, p), back = ss(0.88, 1, p), arc = Math.sin(fl * Math.PI), Z = p < 0.88 ? (DIST - 0.3) * fl * fl * (3 - 2 * fl) : (DIST - 0.3) * (1 - back); root.position.z += Z; root.position.y += 0.75 * arc * (p < 0.82 ? 1 : 0); const fa = p > 0.14 && p < 0.8 ? win(0.14, 0.8, p) * Math.PI * 2 : 0, PIV = 0.75; root.rotation.x = fa; root.position.y += PIV * (1 - Math.cos(fa)); root.position.z -= PIV * Math.sin(fa); const tk = arc > 0 ? Math.min(1, arc * 1.8) : 0; if (tk > 0.01) { placeFoot(-1, -0.03 * tk, 0.25 * tk); placeFoot(1, 0.03 * tk, 0.25 * tk); } if (p > 0.15 && p < 0.8) { if (Math.random() < 0.7) star(new THREE.Vector3((Math.random() - 0.5) * 0.3, root.position.y + 0.3 + Math.random() * 0.7, root.position.z), new THREE.Vector3(0, (Math.random() - 0.5) * 0.4, 0), 1, Math.random() < 0.5 ? GOLD : MAG, 0.5); if (Math.random() < 0.6) dot(new THREE.Vector3((Math.random() - 0.5) * 0.5, root.position.y + Math.random() * 1, root.position.z), new THREE.Vector3((Math.random() - 0.5) * 0.5, 0.2, 0), 0.016, ['#22c55e', '#ffffff', '#ef4444', '#3b82f6', GOLD][Math.floor(Math.random() * 5)], 0.8, 1); } } else root.rotation.x = 0; }
      { const u = (t - FX.crack) / 2.6, on = u > 0 && u < 1; crack.visible = on; if (on) { crack.scale.setScalar(easeO(Math.min(1, u * 8))); for (const m of crack.children) m.material.opacity = 0.85 * (1 - ss(0.65, 1, u)); } }
      // véu (6 s)
      { const v = t - FX.veil, a = ss(0.42, 0.75, v) * (1 - ss(5.6, 6, v)); veil.visible = a > 0.01; if (veil.visible) { veilOut.rotation.y = t * 0.8; veilIn.rotation.y = -t * 1.1; veil.scale.set(0.7 + 0.3 * a, a, 0.7 + 0.3 * a); veil.position.y = 0.9 * (1 - a); } }
      // espinho gigante
      { const u = (t - FX.needle - 0.27) / 0.2, on = u > 0 && u < 1; needle.visible = on; if (on) needle.position.set(0, CHEST + 0.05, 0.3 + (DIST - 0.5) * u); }
      // redemoinho de areia
      { const p = (t - FX.wind) / 1.3; if (p > 0 && p < 0.9) { const z = p < 0.4 ? 0 : DIST * Math.min(1, (p - 0.4) / 0.3); for (let i = 0; i < 4; i++) { const a = Math.random() * 6.283, h = Math.random() * 1.5, rr = 0.16 + h * 0.34; dot(new THREE.Vector3(Math.cos(a) * rr, h, z + Math.sin(a) * rr), new THREE.Vector3(-Math.sin(a) * 2.6, 0.3 + Math.random() * 0.6, Math.cos(a) * 2.6), 0.014 + Math.random() * 0.012, Math.random() < 0.7 ? SAND : (Math.random() < 0.5 ? '#fef3c7' : SANDD), 0.32, 0); } } }
      // lágrimas e dálias
      { const u = (t - FX.tears) / 1.5; if (u > 0 && u < 0.9 && Math.random() < 0.55) { for (const sd of [-1, 1]) if (u < 0.5 && Math.random() < 0.6) dot(new THREE.Vector3(sd * 0.11, 1.32, 0.34), new THREE.Vector3(sd * 0.05, 0.15, 0.1), 0.014, GOLD, 0.65, 3); const pp = new THREE.Vector3((Math.random() - 0.5) * 1.2, 0.8 + Math.random() * 1, (Math.random() - 0.5) * 0.7); dot(pp, new THREE.Vector3((Math.random() - 0.5) * 0.3, -0.5, 0), 0.022, Math.random() < 0.5 ? DAHL : MAG, 1, 0); }
        const fa = u > 0.3 && u < 1.2 ? ss(0.3, 0.55, u) * (1 - ss(0.95, 1.2, u)) : 0; for (const q of tearFlowers) { q.f.visible = fa > 0.01; if (q.f.visible) { const an = q.a + t * 0.8; q.f.position.set(Math.cos(an) * 0.5, 1.45 + Math.sin(an * 2) * 0.05, Math.sin(an) * 0.42); q.f.lookAt(q.f.position.x * 2, 1.5, q.f.position.z * 2); q.f.scale.setScalar(fa); } } }
    };
    P.plates = plates;   // tools3d/export
    const onFrame = c => {
      crownStep(c); platesStep(c);
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
    return { P, upper, WAIST, neck, NECK, hips, HIPS, hands, hipSway: 0.6, hand: { x: 0.5, y: Y(505), z: 0.42, yaw: -0.12 }, placeFoot, setFace, skills, onFrame };
  },
};
