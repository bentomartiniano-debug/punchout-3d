// Jack Mãos de Colmeia no modo vinil, modelado pela folha de modelo do usuário (assets3d/jack/ref/jack_turnaround.png:
// frente, perfil, costas e 3/4). Medidas em px DA FOLHA, lidas da vista de frente (x, y) e do perfil (z, a frente para
// +z). Escala 310 px = 1 m; chão em y = 640, eixo em x = 262.
//  - capuz verde pontudo (a ponta tomba para trás), aberto em volta do rosto, borda enrolada e o forro escuro por dentro
//  - rosto de bloco cor de osso: olhos de traço preto, sobrancelha, o nariz comprido em relevo e a placa da boca de
//    moldura escura; os protetores amarelos dos lados (aparecem na borda do capuz)
//  - gola de cachecol grossa em volta do pescoço, caindo um pouco na frente
//  - capa verde aberta na frente, forro cáqui aparecendo nas bordas e por dentro, barra com pregas
//  - túnica e calção verde-escuros, cinto marrom com a fivela dourada; pernas finas cor de osso; botas verdes de cano
//    dobrado e sola escura
//  - sem braços (como no jogo): as colmeias de favos flutuam, de pé, com a boca sextavada virada para o golpe
import { THREE, INKS, ss, flat, withOutline, legPlacer } from './core.js';
import { RoundedBoxGeometry } from 'three/addons/geometries/RoundedBoxGeometry.js';

const S_PX = 310, FOOT = 640, CX = 262;
const C = { shirt: '#e8ad2e', shorts: '#2f4a33', cloak: '#2e4b34', cloakD: '#1f3424', inner: '#18271b', lining: '#bdb58a', tunic: '#29432f', belt: '#5a3a22', gold: '#e2b23a', goldD: '#9a7016',
            skin: '#e6ddbf', skinD: '#c9bf9c', plate: '#efe8d0', frame: '#23362a', ear: '#f2b52a', hive: '#f2b52a', hiveD: '#c88a12', hole: '#2a1808',
            boot: '#2b472f', sole: '#1b2b1f', ink: '#141a16' };
const col = h => ({ value: new THREE.Color(h) });

export default {
  title: 'Jack Mãos de Colmeia',
  arena: 'jack',   // abre na arena própria (core.js)
  units: [S_PX, FOOT, CX],
  faces: [['normal', 'Normal'], ['angry', 'Raiva'], ['pain', 'Dor'], ['ko', 'K.O.']],
  skills: [['signature', 'Assinatura'], ['sting', 'Ferroada'], ['swarm', 'Infestação'], ['comb', 'Explosão de Favos'], ['molasses', 'Melaço'], ['wall', 'Muralha de Mel'], ['jelly', 'Geleia Real']],
  hudExtra: 'Capuz: <button data-hood="1">Com capuz</button><button data-hood="0">Chapéu de pena</button> <button id="honeySkin">Pele de Mel</button>',
  others: [['valeria', 'Valéria'], ['frog', 'King Frog'], ['espinha', 'Espinha'], ['vivaldi', 'Vivaldi']],
  async build({ root, U }) {
    const { X, Y, D } = U, P = {};
    const V = (x, y, z) => new THREE.Vector3(X(x), Y(y), D(z));
    const ol = (m, w = 0.009) => { const g = withOutline(m, w); g.position.copy(m.position); g.quaternion.copy(m.quaternion); g.scale.copy(m.scale); m.position.set(0, 0, 0); m.quaternion.identity(); m.scale.set(1, 1, 1); return g; };
    function pattern(mat, glsl, key, uniforms = {}, off = [CX, FOOT], rest = false) {   // padrão no shader em px da folha (vPx)
      const prev = mat.onBeforeCompile, k0 = mat.customProgramCacheKey();
      mat.onBeforeCompile = (sh, r) => {
        prev.call(mat, sh, r); Object.assign(sh.uniforms, uniforms);
        sh.vertexShader = (rest ? 'attribute vec3 aRest;\n' : '') + 'varying vec3 vPx; varying vec3 vNo;\n' + sh.vertexShader.replace('#include <begin_vertex>', `#include <begin_vertex>
          vec3 pp = ${rest ? 'aRest' : 'position'}; vPx = vec3(pp.x * ${S_PX.toFixed(1)} + ${off[0].toFixed(1)}, ${off[1].toFixed(1)} - pp.y * ${S_PX.toFixed(1)}, pp.z * ${S_PX.toFixed(1)}); vNo = normal;`);
        sh.fragmentShader = 'varying vec3 vPx; varying vec3 vNo;\n' + Object.entries(uniforms).map(([u, v]) => `uniform ${v.value && v.value.isTexture ? 'sampler2D' : v.value && v.value.isColor ? 'vec3' : 'float'} ${u};\n`).join('')
          + sh.fragmentShader.replace('#include <map_fragment>', '#include <map_fragment>\n' + glsl);
      };
      mat.customProgramCacheKey = () => k0 + '|' + key + (rest ? 'R' : ''); return mat;
    }
    const sp = (v, pw) => Math.sign(v) * Math.pow(Math.abs(v), 2 / pw);
    const lerpT = (tab, y) => {
      if (y <= tab[0][0]) return tab[0].slice(1); const L = tab[tab.length - 1]; if (y >= L[0]) return L.slice(1);
      for (let i = 1; i < tab.length; i++) if (y <= tab[i][0]) { const [a, b] = [tab[i - 1], tab[i]], k = (y - a[0]) / (b[0] - a[0]); return a.slice(1).map((v, j) => v + (b[j + 1] - v) * k); } };
    // casca/sólido por seções de uma tabela [y, meia largura a, frente zf, costas zb, deslocamento z dz]; gap(y) = meia
    // largura da abertura na frente (0 = anel fechado); ripple(th, y) = fator no raio (pregas)
    const ringPt = (a, zf, zb, dz, pw, th, cx = CX) => { const c = Math.cos(th), s = Math.sin(th); return [cx + a * sp(c, pw), dz + (s >= 0 ? zf : zb) * sp(s, pw)]; };
    function shellGeo(tab, y0, y1, n, { pw = 2.2, NA = 96, gap = () => 0, ripple = null, dip = null, cx = CX }) {
      const pos = [], idx = [], edges = { r: [], l: [] };
      for (let i = 0; i <= n; i++) {
        const y = y0 + (y1 - y0) * i / n, [a, zf, zb, dz = 0] = lerpT(tab, y), ow = Math.min(gap(y), a * 0.98);
        const thR = ow > 0 ? Math.acos(Math.pow(ow / a, pw / 2)) : Math.PI / 2, t0 = Math.PI - thR, t1 = 2 * Math.PI + thR;   // da borda esquerda da abertura, pelas costas, até a direita
        for (let j = 0; j <= NA; j++) { const th = t0 + (t1 - t0) * j / NA, k = ripple ? ripple(th, y) : 1; const [x, z] = ringPt(a * k, zf * k, zb * k, dz, pw, th, cx), yy = dip ? y + dip(th, y) : y; pos.push(X(x), Y(yy), D(z));
          if (j === 0) edges.l.push([x, y, z]); if (j === NA) edges.r.push([x, y, z]); }
      }
      for (let i = 0; i < n; i++) for (let j = 0; j < NA; j++) { const a = i * (NA + 1) + j, b = a + 1, c = a + NA + 1, d = c + 1; idx.push(a, b, c, b, d, c); }
      const g = new THREE.BufferGeometry(); g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3)); g.setIndex(idx); g.computeVertexNormals(); g.userData.edges = edges; return g;
    }
    const tube = (pts, r, NA = 10) => new THREE.TubeGeometry(new THREE.CatmullRomCurve3(pts.map(p => V(...p)), false, 'catmullrom', 0.5), Math.max(24, pts.length * 3), D(r), NA, false);
    // pano de duas faces: por fora a cor, por dentro (face de trás) o forro
    const cloth = (outer, inner, glsl = '', key = '', uni = {}, rest = false) => { const m = pattern(flat(outer), `if (!gl_FrontFacing) diffuseColor.rgb = uIn; ${glsl}`, 'cloth' + key, { uIn: col(inner), ...uni }, [CX, FOOT], rest); m.side = THREE.DoubleSide; return m; };

    // ================= grupos do motor
    const WAIST = Y(430), NECK = Y(320), HIPS = Y(445);
    const upper = new THREE.Group(); upper.position.y = WAIST; root.add(upper);
    const body = new THREE.Group(); body.position.y = -WAIST; upper.add(body);
    const neck = new THREE.Group(); neck.position.y = NECK - WAIST; upper.add(neck);
    const head = new THREE.Group(); head.position.y = -NECK; neck.add(head);
    const hips = new THREE.Group(); hips.position.y = HIPS; root.add(hips);
    const low = new THREE.Group(); low.position.y = -HIPS; hips.add(low);

    // ================= rosto: bloco cor de osso (cantos redondos, queixo um pouco mais estreito); pintura de frente
    const F = { x0: CX - 47, x1: CX + 47, top: 222, bot: 340, zf: 54, zb: -14 };   // o alto do bloco fica dentro do capuz
    const FW = 1024, FH = Math.round(FW * (F.bot - F.top) / (F.x1 - F.x0));
    const faceTex = f => { const c = document.createElement('canvas'); c.width = FW; c.height = FH; const g = c.getContext('2d'), k = FW / (F.x1 - F.x0); g.scale(k, k); g.translate(-F.x0, -F.top); drawFace(g, f);
      const t = new THREE.CanvasTexture(c); t.colorSpace = THREE.SRGBColorSpace; t.anisotropy = 8; return t; };
    function drawFace(g, f) {
      const ang = f === 'angry', pain = f === 'pain', ko = f === 'ko', ink = (w, c = C.ink) => { g.strokeStyle = c; g.lineWidth = w; g.lineCap = 'round'; g.lineJoin = 'round'; };
      for (const s of [-1, 1]) {
        const ex = CX + s * 23, ey = 268;
        // sobrancelha: traço curto acima do olho (raiva: desce para o nariz; dor: sobe no meio)
        ink(3.2); g.beginPath();
        if (ang) { g.moveTo(ex + s * 11, ey - 20); g.lineTo(ex - s * 9, ey - 12); } else if (pain) { g.moveTo(ex + s * 11, ey - 13); g.lineTo(ex - s * 9, ey - 20); } else { g.moveTo(ex + s * 11, ey - 17); g.lineTo(ex - s * 9, ey - 17); }
        g.stroke();
        if (pain) { ink(3.4); g.beginPath(); g.moveTo(ex + s * 6, ey - 7); g.lineTo(ex - s * 4, ey); g.lineTo(ex + s * 6, ey + 7); g.stroke(); continue; }
        if (ko) { ink(3.4); g.beginPath(); g.moveTo(ex - 6, ey - 7); g.lineTo(ex + 6, ey + 7); g.moveTo(ex + 6, ey - 7); g.lineTo(ex - 6, ey + 7); g.stroke(); continue; }
        g.fillStyle = C.ink; const h = ang ? 10 : 15; g.beginPath(); g.roundRect(ex - 4.5, ey - h / 2, 9, h, 2.5); g.fill();   // olho: traço preto em pé
      }
      // boca: retângulo claro de moldura escura no queixo (raiva: dentes cerrados; dor: ondulada; K.O.: aberta)
      const my = 318; g.fillStyle = C.frame; g.beginPath(); g.roundRect(CX - 21, my - 10, 42, 20, 4); g.fill();
      g.fillStyle = ko ? '#3a1a10' : C.plate; g.beginPath(); g.roundRect(CX - 17, my - 6.5, 34, 13, 2.5); g.fill();
      if (ang) { ink(1.4, C.frame); g.beginPath(); for (const x of [-9, 0, 9]) { g.moveTo(CX + x, my - 6); g.lineTo(CX + x, my + 6); } g.moveTo(CX - 17, my); g.lineTo(CX + 17, my); g.stroke(); }
      if (pain) { ink(1.8, C.frame); g.beginPath(); g.moveTo(CX - 14, my); for (let k = 0; k < 4; k++) g.quadraticCurveTo(CX - 10.5 + k * 7, k % 2 ? my + 4 : my - 4, CX - 7 + k * 7, my); g.stroke(); }
      // melaço (sprite): sombra dura sobre os olhos e o olhar brilhando em ciano
      if (f === 'molasses') { g.fillStyle = 'rgba(15,23,42,0.9)'; g.beginPath(); g.moveTo(F.x0, 246); g.quadraticCurveTo(CX, 258, F.x1, 246); g.lineTo(F.x1, 286); g.quadraticCurveTo(CX, 294, F.x0, 286); g.fill();
        g.fillStyle = '#00f5d4'; for (const s of [-1, 1]) { g.beginPath(); g.roundRect(CX + s * 23 - 8, 266, 16, 7, 2); g.fill(); } }
      // sombra do capuz na testa
      const sh = g.createLinearGradient(0, F.top, 0, F.top + 34); sh.addColorStop(0, 'rgba(40,50,40,0.55)'); sh.addColorStop(1, 'rgba(40,50,40,0)'); g.fillStyle = sh; g.fillRect(F.x0, F.top, F.x1 - F.x0, 34);
    }
    const headU = { tFace: { value: faceTex('normal') }, uSide: col(C.skinD) };
    {
      const g = new RoundedBoxGeometry(D(F.x1 - F.x0), D(F.bot - F.top), D(F.zf - F.zb), 6, D(16));
      g.translate(X(CX), Y((F.top + F.bot) / 2), D((F.zf + F.zb) / 2));
      { const p = g.attributes.position; for (let i = 0; i < p.count; i++) { const y = FOOT - p.getY(i) * S_PX, x = p.getX(i) * S_PX, t = Math.max(0, (y - 290) / 50);
          p.setX(i, (x * (1 - 0.12 * t * t)) / S_PX); } g.computeVertexNormals(); }   // queixo um pouco mais estreito
      const m = pattern(flat(C.skin), `{ vec3 n = normalize(vNo);
        if (n.z > 0.45) { vec2 fuv = vec2((vPx.x - ${F.x0.toFixed(1)}) / ${(F.x1 - F.x0).toFixed(1)}, 1.0 - (vPx.y - ${F.top.toFixed(1)}) / ${(F.bot - F.top).toFixed(1)});
          vec4 fc = texture2D(tFace, fuv); diffuseColor.rgb = mix(diffuseColor.rgb, fc.rgb, fc.a * smoothstep(0.45, 0.7, n.z)); }
        else if (abs(n.x) > 0.6) diffuseColor.rgb = uSide; }`, 'jackFace', headU);   // lados do bloco um tom abaixo (rosto de bloco, como na folha)
      head.add(withOutline(new THREE.Mesh(g, m), 0.008));
    }
    // nariz comprido em relevo (bloco que afina para cima, ponta redonda embaixo)
    { const g = new RoundedBoxGeometry(D(13), D(56), D(14), 3, D(4)); const p = g.attributes.position;
      for (let i = 0; i < p.count; i++) { const t = (p.getY(i) / D(28) + 1) / 2; p.setX(i, p.getX(i) * (1 - 0.3 * t)); p.setZ(i, p.getZ(i) * (1 - 0.45 * t) + D(3) * (1 - t)); }
      g.computeVertexNormals(); const n = new THREE.Mesh(g, flat(C.skin)); n.position.copy(V(CX, 270, F.zf + 4)); head.add(ol(n, 0.006)); }

    // ================= capuz: pontudo, a ponta tomba para trás; aberto em volta do rosto (arco em cima), borda enrolada
    const HOOD = [[118, 6, 6, 8, -46], [128, 20, 14, 26, -52], [145, 40, 26, 58, -42], [165, 58, 40, 86, -30], [190, 80, 54, 100, -18], [215, 100, 62, 104, -10],
                  [240, 112, 66, 104, -7], [262, 116, 68, 104, -5], [285, 106, 66, 100, -4], [305, 98, 60, 94, -4]];
    const hoodGap = y => y < 228 ? 0 : 58 * Math.sqrt(Math.min(1, (y - 228) / 30));   // arco baixo: o capuz cobre a testa
    {
      const g = shellGeo(HOOD, 118, 305, 60, { pw: 2.1, NA: 120, gap: hoodGap });
      const hood = new THREE.Group(); head.add(hood); P.hood = hood;   // tudo do capuz (some na variação de chapéu)
      // protetores amarelos dos lados do rosto (aparecem na borda do capuz)
      for (const s of [-1, 1]) { const e = new THREE.Mesh(new RoundedBoxGeometry(D(26), D(54), D(40), 3, D(8)), flat(C.ear)); e.position.copy(V(CX + s * 58, 252, 18)); hood.add(ol(e, 0.007)); }
      hood.add(withOutline(new THREE.Mesh(g, cloth(C.cloak, C.inner, '', 'hood')), 0.009));
      // borda enrolada da abertura: do pé direito, pelo arco de cima, ao pé esquerdo
      const E = g.userData.edges, path = [...E.r.slice().reverse().filter(p => p[1] >= 228), ...E.l.filter(p => p[1] >= 228)];
      if (path.length > 3) hood.add(withOutline(new THREE.Mesh(tube(path.map(([x, y, z]) => [x, y, z + 1]), 5, 10), flat(C.cloak)), 0.006));
      // fundo escuro atrás do rosto (o capuz por dentro, sem buraco para o vazio)
      const bk = new THREE.Mesh(new THREE.SphereGeometry(1, 24, 16), flat(C.inner)); bk.scale.set(D(92), D(90), D(70)); bk.position.copy(V(CX, 252, -26)); hood.add(bk);
    }
    // ================= variação sem capuz: o cabelo loiro e o chapéu de pena (o chapéu antigo dele)
    {
      const hat = new THREE.Group(); hat.visible = false; head.add(hat); P.hat = hat;
      // cabelo: casca em volta da cabeça, aberta no rosto, franja em mechas nas costas e nos lados
      { const g = new THREE.SphereGeometry(1, 48, 24, Math.PI / 2 + 0.8, Math.PI * 2 - 1.6, 0, Math.PI * 0.72), p = g.attributes.position;
        for (let i = 0; i < p.count; i++) { const y = p.getY(i); if (y < -0.2) { const a = Math.atan2(p.getZ(i), p.getX(i)); p.setY(i, y - 0.16 * Math.abs(Math.sin(a * 7))); } }   // pontas das mechas
        g.computeVertexNormals(); const m = flat('#f3d77a'); m.side = THREE.DoubleSide; g.scale(D(68), D(76), D(64)); g.translate(X(CX), Y(252), D(6));
        { const q = g.attributes.position; for (let i = 0; i < q.count; i++) { const y = FOOT - q.getY(i) * S_PX; if (y > 238) continue;   // acima da aba: cabe dentro da copa (não atravessa nas costas)
            const dx = q.getX(i) * S_PX, dz = q.getZ(i) * S_PX - 14, e = Math.hypot(dx / 60, dz / 52); if (e > 1) { q.setX(i, q.getX(i) / e); q.setZ(i, (dz / e + 14) / S_PX); } } g.computeVertexNormals(); } hat.add(withOutline(new THREE.Mesh(g, m), 0.008)); P.hairGeo = g; }
      const HX = CX, HY = 230, HZ = 14, put = m => { m.position.add(V(HX, HY, HZ).sub(V(CX, FOOT, 0))); return m; };
      const crown = shellGeo([[164, 0, 0, 0, HZ], [170, 30, 28, 28, HZ], [180, 44, 40, 40, HZ], [200, 50, 45, 45, HZ], [230, 54, 48, 48, HZ]], 164, 230, 24, { pw: 2.3, NA: 64,
        dip: (th, y) => 10 * Math.exp(-Math.pow(Math.cos(th) * 3, 2)) * Math.max(0, (182 - y) / 18) });   // vinco do alto, de frente para trás
      hat.add(withOutline(new THREE.Mesh(crown, flat('#f2cf55')), 0.009));
      { const b = shellGeo([[214, 55.5, 49.5, 49.5, HZ], [229, 55.5, 49.5, 49.5, HZ]], 214, 229, 3, { pw: 2.3, NA: 64 }); const m = flat('#7a4a24'); m.side = THREE.DoubleSide; hat.add(withOutline(new THREE.Mesh(b, m), 0.006)); }
      // aba: anel elíptico com espessura, sobe nos lados e cai um pouco na frente
      { const RX = 104, RZ = 90, NU = 8, NB = 96, pos = [], idx = [];
        const ring = (u, dy) => { for (let j = 0; j <= NB; j++) { const th = j / NB * Math.PI * 2, c = Math.cos(th), sn = Math.sin(th), ix = 52 * c, iz = 46 * sn, ox = RX * c, oz = RZ * sn;
          const lift = 16 * Math.pow(Math.abs(c), 2.5) * u * u - 6 * Math.pow(Math.max(0, sn), 2) * u * u;
          pos.push(X(CX + ix + (ox - ix) * u), Y(HY - lift + dy), D(HZ + iz + (oz - iz) * u)); } };
        for (let i = 0; i <= NU; i++) ring(i / NU, -2); for (let i = NU; i >= 0; i--) ring(i / NU, 2);
        const W = NB + 1; for (let i = 0; i < 2 * (NU + 1) - 1; i++) for (let j = 0; j < NB; j++) { const a = i * W + j, b = a + 1, c = a + W, d = c + 1; idx.push(a, b, c, b, d, c); }
        const g = new THREE.BufferGeometry(); g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3)); g.setIndex(idx); g.computeVertexNormals();
        const m = flat('#f2cf55'); m.side = THREE.DoubleSide; hat.add(withOutline(new THREE.Mesh(g, m), 0.008)); }
      // pena branca do lado direito dele, saindo da fita para cima e para trás (o chapéu todo cresce depois)
      { const sh = new THREE.Shape(); sh.moveTo(0, 0); sh.quadraticCurveTo(D(26), D(36), D(6), D(92)); sh.quadraticCurveTo(D(-17), D(42), 0, 0);
        const g = new THREE.ExtrudeGeometry(sh, { depth: D(1.5), bevelEnabled: true, bevelThickness: D(0.8), bevelSize: D(0.8), bevelSegments: 2, curveSegments: 16 });
        const f = new THREE.Mesh(g, flat('#f6f4ec')); f.position.copy(V(CX - 48, 216, HZ + 18)); f.rotation.set(-0.15, -0.55, 0.4, 'YXZ'); hat.add(ol(f, 0.006)); }
      // chapéu maior que o cabelo (o cabelo não atravessa): copa, fita, aba e pena escalados em volta da base da copa
      const cap = new THREE.Group(), piv = V(HX, HY + 4, HZ); cap.position.copy(piv); cap.scale.setScalar(1.25); hat.add(cap);
      for (const o of hat.children.slice(1, -1)) { o.position.sub(piv); cap.add(o); }
    }
    const setHood = on => { P.hood.visible = on; P.hat.visible = !on;
      document.querySelectorAll('[data-hood]').forEach(b => b.style.outline = (b.dataset.hood === '1') === on ? '2px solid #fc6' : ''); };
    document.querySelectorAll('[data-hood]').forEach(b => b.onclick = () => setHood(b.dataset.hood === '1'));
    setHood(true);

    // ================= gola de cachecol: rolo grosso em volta do pescoço, cai um pouco na frente (a aba com a costura)
    const COWL = [[288, 58, 70, 58, -8], [296, 96, 76, 88, -8], [310, 108, 80, 98, -6], [326, 112, 80, 100, -4], [344, 106, 72, 98, -4], [360, 94, 62, 92, -4], [366, 70, 46, 72, -4]];   // na frente fica baixo: a placa da boca aparece em cima
    { const g = shellGeo(COWL, 288, 366, 30, { pw: 2.2, NA: 96, ripple: (th, y) => 1 + 0.025 * Math.cos(th * 6) * Math.sin(Math.PI * (y - 290) / 76),
        dip: (th, y) => 8 * Math.pow(Math.max(0, Math.sin(th)), 4) * (1 - 0.6 * (y - 290) / 76) });   // o cachecol sobe na frente e cobre a boca
      body.add(withOutline(new THREE.Mesh(g, flat(C.cloak)), 0.009));
    }

    // ================= capa: aberta na frente (o forro cáqui dobra para fora nas bordas), barra com pregas
    const CAPE = [[330, 104, 68, 96, -4], [360, 116, 74, 106, -6], [400, 128, 80, 118, -8], [450, 138, 86, 130, -10], [495, 145, 90, 140, -12], [526, 150, 92, 146, -12]];
    const capeGap = y => 34 + 40 * Math.min(1, Math.max(0, (y - 340) / 186)) ** 1.2;
    { const ripple = (th, y) => 1 + 0.05 * Math.cos(th * 7 + 0.4) * Math.min(1, Math.max(0, (y - 380) / 140));
      const g = shellGeo(CAPE, 330, 526, 50, { pw: 2.3, NA: 120, gap: capeGap, ripple });
      const m = cloth(C.cloak, C.lining, `if (gl_FrontFacing && vPx.z > 20.0) { float gx = 34.0 + 40.0 * pow(clamp((vPx.y - 340.0) / 186.0, 0.0, 1.0), 1.2);
          float d = abs(vPx.x - ${CX.toFixed(1)}) - gx; if (d < 9.0) diffuseColor.rgb = uIn; }
        if (gl_FrontFacing && vPx.y > 520.0) diffuseColor.rgb = mix(diffuseColor.rgb, uD, 0.5);
        float arc = atan(vPx.x - ${CX.toFixed(1)}, vPx.z) * 120.0;   // px ao longo da volta (0 = frente, ±377 = costas)
        if (gl_FrontFacing) {   // retalhos: pano de outro verde e um marrom, com o ponto da costura tracejado
          for (int k = 0; k < 4; k++) {
            vec4 rc = k == 0 ? vec4(250.0, 392.0, 300.0, 436.0) : k == 1 ? vec4(-330.0, 452.0, -270.0, 494.0) : k == 2 ? vec4(-150.0, 410.0, -112.0, 446.0) : vec4(120.0, 470.0, 158.0, 504.0);
            vec2 q = vec2(arc, vPx.y) - rc.xy, sz = rc.zw - rc.xy;
            if (q.x > 0.0 && q.y > 0.0 && q.x < sz.x && q.y < sz.y) {
              diffuseColor.rgb = k == 1 ? uPatchB : k == 3 ? uPatchC : uPatchA;
              float e = min(min(q.x, sz.x - q.x), min(q.y, sz.y - q.y));
              if (e < 2.4 && e > 1.0 && fract((q.x + q.y) / 4.0) < 0.55) diffuseColor.rgb = uStitch;
              if (e < 0.8) diffuseColor.rgb = uStitch; } } }
        else {   // forro cáqui com favos (sextavados) em linha mais escura
          vec2 hp = vec2(arc, vPx.y) / 13.0, rr = vec2(1.0, 1.7320508), hh = rr * 0.5;
          vec2 a = mod(hp, rr) - hh, b = mod(hp - hh, rr) - hh, gv = dot(a, a) < dot(b, b) ? a : b, pa = abs(gv);
          float d = max(dot(pa, vec2(0.5, 0.8660254)), pa.x), fw = fwidth(d);
          diffuseColor.rgb = mix(diffuseColor.rgb, uHexL, smoothstep(0.45 - fw, 0.45 + fw, d)); }`, 'cape', { uD: col(C.cloakD), uPatchA: col('#3f6243'), uPatchB: col('#5b4a30'), uPatchC: col('#24402c'), uStitch: col('#bdb58a'), uHexL: col('#958c62') }, true);
      g.setAttribute('aRest', g.attributes.position.clone()); body.add(withOutline(new THREE.Mesh(g, m), 0.009)); P.capeGeo = g; }

    // ================= camisa amarela (sob a capa), cinto e fivela; bermuda: o assento no quadril e cada perna da
    // bermuda presa na perna (acompanha a passada, como o short da Joana), barra dobrada acima do joelho
    { const g = shellGeo([[330, 60, 52, 64, 0], [370, 64, 56, 66, 0], [404, 62, 56, 64, 0], [432, 64, 57, 64, 0]], 330, 432, 26, { pw: 2.2, NA: 72 });
      const m = flat(C.shirt); m.side = THREE.DoubleSide; body.add(withOutline(new THREE.Mesh(g, m), 0.008)); }
    { const SEAT = [[428, 65, 58, 64, 0], [440, 68, 60, 64, 0], [452, 66, 56, 60, 0], [462, 52, 46, 50, 0], [468, 30, 28, 30, 0]];   // embaixo arredonda para dentro das coxas
      const m = flat(C.shorts); m.side = THREE.DoubleSide; low.add(withOutline(new THREE.Mesh(shellGeo(SEAT, 428, 468, 18, { pw: 2.2, NA: 72 }), m), 0.008)); }
    const bermudaLeg = (x, top) => {
      const g = new THREE.Group(), m = flat(C.shorts); m.side = THREE.DoubleSide;
      const leg = shellGeo([[436, 28, 28, 28, 0], [470, 29, 29, 29, 0], [496, 31, 30, 30, 0]], 436, 496, 16, { pw: 2, NA: 40, cx: x }); leg.translate(-X(x), -Y(top), 0); g.add(withOutline(new THREE.Mesh(leg, m), 0.008));
      const cuff = shellGeo([[492, 34, 33, 33, 0], [504, 34, 33, 33, 0]], 492, 504, 3, { pw: 2, NA: 40, cx: x }); cuff.translate(-X(x), -Y(top), 0);
      const mc = flat(C.cloakD); mc.side = THREE.DoubleSide; g.add(withOutline(new THREE.Mesh(cuff, mc), 0.007));
      return g;
    };
    { const g = shellGeo([[424, 66, 59, 66, 0], [442, 68, 60, 66, 0]], 424, 442, 4, { pw: 2.2, NA: 72 }); const m = flat(C.belt); m.side = THREE.DoubleSide; low.add(withOutline(new THREE.Mesh(g, m), 0.007));
      const bk = new THREE.Mesh(new RoundedBoxGeometry(D(36), D(26), D(6), 2, D(2)), flat(C.gold)); bk.position.copy(V(CX, 433, 61)); low.add(ol(bk, 0.006));
      const ins = new THREE.Mesh(new RoundedBoxGeometry(D(22), D(13), D(4), 2, D(1.5)), flat(C.belt)); ins.position.copy(V(CX, 433, 63)); low.add(ins); }

    // ================= pernas finas cor de osso e botas verdes de cano dobrado
    const LEG_TOP = 462, ANK = 560, LX = { [-1]: CX - 46, [1]: CX + 46 }, legs = {}, feet = {};
    for (const s of [-1, 1]) {
      const x = LX[s], g = new THREE.CylinderGeometry(D(13), D(12), D(ANK - LEG_TOP + 10), 20); g.translate(0, -D((ANK - LEG_TOP + 10) / 2), 0);
      const leg = withOutline(new THREE.Mesh(g, flat(C.skin)), 0.008); leg.position.set(X(x), Y(LEG_TOP), 0); root.add(leg); legs[s] = leg;
      leg.add(bermudaLeg(x, LEG_TOP));   // a perna da bermuda vai junto com a perna
      const f = new THREE.Group(); f.position.set(X(x), Y(ANK), 0); root.add(f); feet[s] = f;
      const at = (x0, y, z) => V(x0 - x + CX, y - ANK + FOOT, z);   // px da folha relativos ao tornozelo
      // cano: dobra larga em cima, cano reto, pé arredondado com o bico para a frente, sola escura
      const cuff = new THREE.Mesh(new THREE.CylinderGeometry(D(36), D(33), D(26), 28), flat(C.boot)); cuff.position.copy(at(x, 552, 2)); f.add(ol(cuff, 0.008));
      const shaft = new THREE.Mesh(new THREE.CylinderGeometry(D(30), D(31), D(40), 28), flat(C.boot)); shaft.position.copy(at(x, 584, 2)); f.add(ol(shaft, 0.008));
      const foot = new THREE.Mesh(new THREE.SphereGeometry(1, 28, 16, 0, Math.PI * 2, 0, Math.PI / 2), flat(C.boot)); foot.scale.set(D(46), D(40), D(66)); foot.position.copy(at(x, 628, 34)); f.add(ol(foot, 0.008));
      const sole = new THREE.Mesh(new THREE.CylinderGeometry(1, 1, 1, 32), flat(C.sole)); sole.scale.set(D(48), D(12), D(68)); sole.position.copy(at(x, 634, 34)); f.add(ol(sole, 0.007));
      f.rotation.y = -s * 0.12;
    }
    const placeFoot = legPlacer({ legs, feet, hipX: { [-1]: X(LX[-1]), [1]: X(LX[1]) }, top: Y(LEG_TOP), bot: Y(ANK), hips, hipsY: HIPS });

    // ================= colmeias: cesto de anéis empilhados (de pé), na frente o favo de seis blocos sextavados em volta
    // da boca sextavada funda; Z local = para onde o golpe vai, Y = para cima
    const hexPrism = (r, h, color) => { const g = new THREE.CylinderGeometry(D(r), D(r), D(h), 6); g.rotateX(Math.PI / 2); g.rotateZ(Math.PI / 6); return new THREE.Mesh(g, flat(color)); };
    function hive() {
      const g = new THREE.Group(), pts = [];
      // perfil do cesto (px): cinco anéis com sulcos, mais largo no meio, tampa redonda em cima
      const tiers = [[0, 38], [24, 52], [50, 60], [78, 62], [106, 58], [134, 46]];   // [altura da base do anel, raio]
      pts.push(new THREE.Vector2(0, D(-78)));
      tiers.forEach(([y0, r], i) => { const yb = -78 + y0, yt = yb + (i === tiers.length - 1 ? 22 : 28);
        pts.push(new THREE.Vector2(D(r - 6), D(yb)), new THREE.Vector2(D(r), D(yb + 5)), new THREE.Vector2(D(r + 1.5), D((yb + yt) / 2)), new THREE.Vector2(D(r), D(yt - 4)), new THREE.Vector2(D(r - 7), D(yt))); });
      pts.push(new THREE.Vector2(D(22), D(84)), new THREE.Vector2(D(10), D(90)), new THREE.Vector2(0, D(91)));
      const body = new THREE.Mesh(new THREE.LatheGeometry(pts, 48), flat(C.hive)); g.add(ol(body, 0.009));
      // favo da frente: seis blocos sextavados em volta da boca (saltados do cesto), a boca escura funda
      const FZ = 52, RC = 19, hexG = (r, d) => { const sh = new THREE.Shape(); for (let k = 0; k < 6; k++) { const a = k / 6 * Math.PI * 2; k ? sh.lineTo(D(r * Math.cos(a)), D(r * Math.sin(a))) : sh.moveTo(D(r * Math.cos(a)), D(r * Math.sin(a))); }
        return new THREE.ExtrudeGeometry(sh, { depth: D(d), bevelEnabled: true, bevelThickness: D(1.5), bevelSize: D(1.5), bevelSegments: 1 }); };
      for (let k = 0; k < 6; k++) { const a = (k + 0.5) / 6 * Math.PI * 2, b = new THREE.Mesh(hexG(RC - 2.2, 16), flat(k % 2 ? C.hive : '#f6c14a'));   // vizinhos de uma colmeia de verdade: a √3·r, nas arestas
        b.material.flatShading = true; b.position.set(D(Math.cos(a) * RC * Math.sqrt(3)), D(Math.sin(a) * RC * Math.sqrt(3)), D(FZ)); g.add(ol(b, 0.006)); }
      const hole = new THREE.Mesh(hexG(RC - 0.2, 14), new THREE.MeshBasicMaterial({ color: '#000000' })); hole.position.set(0, 0, D(FZ)); g.add(hole);   // célula do meio vazia: preta
      return g;
    }
    P.hives = { [-1]: hive(), [1]: hive() }; root.add(P.hives[-1], P.hives[1]);
    const hands = { [-1]: new THREE.Object3D(), [1]: new THREE.Object3D() }; root.add(hands[-1], hands[1]);
    const UPV = new THREE.Vector3(0, 1, 0), FWD = new THREE.Vector3(0, 0, 1);

    const setFace = f => { headU.tFace.value.dispose(); headU.tFace.value = faceTex(f); };
    // pano/cabelo com física: molas por setor (em volta do eixo) puxadas pela aceleração e pelo giro de quem segura,
    // brisa leve; peso w por vértice (0 = preso), nunca entra para dentro
    const clothSim = (g, wOf, owner, spinOf, { max = 0.12, k = 38, wave = 0.006, NS = 10 } = {}) => {
      const p = g.attributes.position, n = p.count, base = Float32Array.from(p.array), w = new Float32Array(n), ph = new Float32Array(n);
      for (let i = 0; i < n; i++) { const y = FOOT - p.getY(i) * S_PX, x = p.getX(i) * S_PX, z = p.getZ(i) * S_PX; w[i] = wOf(y); ph[i] = Math.atan2(z, x); }
      const sp = Array.from({ length: NS }, () => ({ o: new THREE.Vector3(), v: new THREE.Vector3() }));
      const pos = new THREE.Vector3(), prev = new THREE.Vector3(), vel = new THREE.Vector3(), acc = new THREE.Vector3(), q = new THREE.Quaternion(); let init = false, rotPrev = 0;
      return c => {
        const dt = Math.min(0.05, Math.max(c.dt || 1 / 60, 1e-3)), rot = spinOf();
        owner.updateWorldMatrix(true, false); owner.getWorldPosition(pos); if (!init) { prev.copy(pos); rotPrev = rot; init = true; }
        const v = pos.clone().sub(prev).divideScalar(dt); acc.copy(v).sub(vel).divideScalar(dt); vel.copy(v); prev.copy(pos);
        owner.getWorldQuaternion(q).invert(); const A = acc.clone().applyQuaternion(q).clampLength(0, 30), spin = (rot - rotPrev) / dt; rotPrev = rot;
        sp.forEach((s, j) => { const a = (j + 0.5) / NS * Math.PI * 2 - Math.PI, tan = new THREE.Vector3(-Math.sin(a), 0, Math.cos(a));
          s.v.addScaledVector(s.o, -k * dt).addScaledVector(s.v, -5 * dt).addScaledVector(A, -0.9 * dt).addScaledVector(tan, -spin * 0.6 * dt);
          s.v.x += Math.sin(c.t * 1.3 + j) * 0.012 * dt * k; s.v.z += Math.cos(c.t * 1.1 + j * 1.7) * 0.012 * dt * k;
          s.o.addScaledVector(s.v, dt); if (s.o.length() > max) s.o.setLength(max); });
        const a = p.array;
        for (let i = 0; i < n; i++) { if (!w[i]) continue; const f = ((ph[i] + Math.PI) / (Math.PI * 2)) * NS - 0.5, k0 = ((Math.floor(f) % NS) + NS) % NS, k1 = (k0 + 1) % NS, t = f - Math.floor(f);
          const o0 = sp[k0].o, o1 = sp[k1].o, rx = Math.cos(ph[i]), rz = Math.sin(ph[i]);
          let ox = (o0.x + (o1.x - o0.x) * t) * w[i], oy = (o0.y + (o1.y - o0.y) * t) * w[i] * 0.4, oz = (o0.z + (o1.z - o0.z) * t) * w[i];
          const wv = Math.sin(c.t * 2.2 + ph[i] * 4 + w[i] * 3) * wave * w[i];   // ondulação
          ox += rx * wv; oz += rz * wv;
          const inward = ox * rx + oz * rz; if (inward < 0) { ox -= rx * inward * 0.75; oz -= rz * inward * 0.75; }   // não entra no corpo
          a[i * 3] = base[i * 3] + ox; a[i * 3 + 1] = base[i * 3 + 1] + oy + Math.abs(inward) * 0.15 * w[i]; a[i * 3 + 2] = base[i * 3 + 2] + oz; }
        p.needsUpdate = true; g.computeVertexNormals();
      };
    };
    const capeSim = clothSim(P.capeGeo, y => Math.pow(Math.min(1, Math.max(0, (y - 335) / 191)), 1.5), body, () => upper.rotation.y);
    // cabelo: preso embaixo do chapéu, as mechas de baixo balançam com a cabeça (mais mole e mais curto que a capa)
    const hairSim = clothSim(P.hairGeo, y => Math.pow(Math.min(1, Math.max(0, (y - 238) / 70)), 1.3), head, () => upper.rotation.y + neck.rotation.y, { max: 0.05, k: 30, wave: 0.004, NS: 8 });
    // ================= especiais (os do jogo, engine.cpp): cada um tem pose e efeito próprios; a boca da colmeia fica
    // virada para a frente enquanto duram (é de lá que sai tudo)
    const easeO = k => 1 - (1 - k) ** 3, HONEY = '#f59e0b', JELLY = '#fef08a';
    const fxRoot = new THREE.Group(); root.add(fxRoot);
    const mouth = sx => fxRoot.worldToLocal(P.hives[sx].localToWorld(new THREE.Vector3(0, 0, D(70))));
    const FX = { sting: -99, sig: -99, swarm: -99, comb: -99, molasses: -99, wall: -99, jelly: -99, side: 1 };
    const both = (e, up = 0.1, mid = 0.25, fwd = 0.45) => ({ [-1]: [fwd * e, up * e, mid * e, 0, 0], [1]: [fwd * e, up * e, mid * e, 0, 0] });
    const setPose = (pose, v) => { pose[-1] = v[-1]; pose[1] = v[1]; };
    // gota alongada (a origem fica no topo, cresce para baixo; as peças dela passam a escala pelo grupo do contorno)
    const dropGeo = new THREE.SphereGeometry(1, 16, 12); dropGeo.translate(0, -1, 0);
    // Ferroada: as duas colmeias se juntam e projetam um ferrão de luz (farpado, núcleo branco) com anéis de choque
    const glow = (col, o) => new THREE.MeshBasicMaterial({ color: col, transparent: true, opacity: o, blending: THREE.AdditiveBlending, depthWrite: false, side: THREE.DoubleSide });
    const barbed = k => { const pts = [[0, 0], [0.05, 0.04], [0.36, 0.05], [0.42, 0.11], [0.5, 0.06], [0.64, 0.15], [1, 0]].map(([z, r]) => new THREE.Vector2(r * k, z)); const g = new THREE.LatheGeometry(pts, 16); g.rotateX(Math.PI / 2); return g; };
    const stinger = new THREE.Group(); stinger.visible = false; fxRoot.add(stinger);
    stinger.add(new THREE.Mesh(barbed(1), glow('#f59e0b', 0.55)), new THREE.Mesh(barbed(0.45), glow('#fff7c2', 0.95)));
    const stingRings = [0.3, 0.55, 0.8].map(z => { const m = new THREE.Mesh(new THREE.RingGeometry(0.86, 1, 32), glow('#ffeb64', 0.7)); m.position.z = z; stinger.add(m); return m; });
    // Infestação: enxame de abelhas (listradas, asas batendo) sai das bocas e fica rondando o alvo por 6 s
    const beeBody = new THREE.SphereGeometry(1, 12, 8);
    const makeBee = () => { const g = new THREE.Group(), b = new THREE.Mesh(beeBody, flat(C.hive)); b.scale.set(D(11), D(11), D(18)); g.add(b);
      const st = new THREE.Mesh(beeBody, flat(C.ink)); st.scale.set(D(11.6), D(11.6), D(5)); g.add(st);
      const wings = [-1, 1].map(s => { const w = new THREE.Mesh(beeBody, flat('#ffffff')); w.scale.set(D(14), D(2), D(8)); w.position.set(s * D(13), D(10), 0); g.add(w); return w; });
      g.visible = false; fxRoot.add(g); return { g, wings }; };
    const flap = (b, t, ph) => b.wings.forEach((w, i) => { w.rotation.z = (i ? 1 : -1) * Math.sin(t * 70 + ph) * 0.7; });
    const bees = Array.from({ length: 26 }, (_, i) => ({ ...makeBee(), side: i % 2 ? 1 : -1, delay: i * 0.018, a: i * 2.4, r: 0.12 + (i % 5) * 0.06, y: ((i * 7) % 9 - 4) * 0.05, sp: 2.2 + (i % 4) * 0.6 }));
    // Assinatura: a colmeia ergue o chapéu e 16 abelhas saem de baixo dele, rodeiam o Jack e voltam para dentro
    const sigBees = Array.from({ length: 16 }, (_, i) => ({ ...makeBee(), i }));
    const SWARM = new THREE.Vector3(0, 1.0, 1.45);
    // Explosão de Favos: cacos sextavados voam das bocas, com um anel de favo abrindo
    const hexShape = r => { const sh = new THREE.Shape(); for (let k = 0; k < 6; k++) { const a = k / 6 * Math.PI * 2; k ? sh.lineTo(D(r * Math.cos(a)), D(r * Math.sin(a))) : sh.moveTo(D(r), 0); } sh.closePath(); return sh; };
    const hexG = (r, d) => new THREE.ExtrudeGeometry(hexShape(r), { depth: D(d), bevelEnabled: true, bevelThickness: D(1.5), bevelSize: D(1.5), bevelSegments: 1 });
    const shardG = hexG(14, 7), shards = Array.from({ length: 28 }, (_, i) => { const g = withOutline(new THREE.Mesh(shardG, flat(i % 3 ? C.hive : '#f6c14a')), 0.004); g.visible = false; fxRoot.add(g);
      const a = i * 2.399, r = 0.5 + (i % 5) * 0.35; return { g, side: i % 2 ? 1 : -1, v: new THREE.Vector3(Math.cos(a) * r, 0.5 + Math.sin(a) * r, 2.2 + (i % 3) * 0.5), spin: 4 + i % 6, o: new THREE.Vector3(), set: false }; });
    const ringG = new THREE.RingGeometry(0.82, 1, 6), rings = [-1, 1].map(() => { const m = new THREE.Mesh(ringG, new THREE.MeshBasicMaterial({ color: '#ffe27a', transparent: true, side: THREE.DoubleSide, depthWrite: false })); m.visible = false; fxRoot.add(m); return m; });
    // Melaço (6 s): mel escorre da roupa e das colmeias, olhar bravo, pingos caindo (sprite "Molasses")
    root.updateMatrixWorld(true);
    const ray = new THREE.Raycaster(), frontZ = (x, y) => { ray.set(new THREE.Vector3(X(x), Y(y), 3), new THREE.Vector3(0, 0, -1)); const h = ray.intersectObject(body, true).find(i => !i.object.userData.hull); return h ? h.point.z : D(60); };
    const coat = (col, list) => { const g = new THREE.Group(); g.visible = false; g.position.y = -WAIST; upper.add(g);
      const items = list.map(([x, y, w, len]) => { const b = new THREE.Mesh(dropGeo, flat(col)), o = withOutline(b, 0.004); o.position.set(X(x), Y(y), frontZ(x, y)); o.scale.set(D(w * 0.7), 0.001, D(w * 0.25)); g.add(o); return { b: o, len }; }); return { g, items }; };
    const COAT = [[CX - 42, 352, 13, 44], [CX + 34, 342, 11, 38], [CX - 6, 382, 14, 46], [CX + 50, 392, 9, 30], [CX - 58, 420, 9, 28]];
    const honeyCoat = coat(HONEY, COAT), jellyCoat = coat(JELLY, COAT.map(([x, y, w, l]) => [x + 8, y - 6, w + 1, l + 6]));
    // rachaduras fundas nas colmeias (zigue-zague pela superfície do cesto, dos dois lados do favo); o mel sai delas
    const hiveCracks = [-1, 1].map(s => { const g = new THREE.Group(); g.visible = false; P.hives[s].add(g);
      for (const [a0, y0, y1] of [[-0.95, 72, -66], [0.9, 66, -58], [-1.45, 40, -40], [1.4, 50, -30]]) { const n = 8, pts = []; for (let k = 0; k <= n; k++) { const a = a0 + (k % 2 ? 0.13 : -0.13), y = y0 + (y1 - y0) * k / n; pts.push(new THREE.Vector3(D(65 * Math.sin(a)), D(y), D(65 * Math.cos(a)))); }
        g.add(new THREE.Mesh(new THREE.TubeGeometry(new THREE.CatmullRomCurve3(pts, false, 'catmullrom', 0.05), 40, D(3.4), 6), flat(C.ink))); }
      return g; });
    // brilho dos olhos: duas luzinhas ciano na frente do rosto, pulsando enquanto dura
    const eyeGlow = [-1, 1].map(s => { const m = new THREE.Mesh(new THREE.SphereGeometry(D(13), 14, 10), glow('#00f5d4', 0.55)); m.scale.set(1.3, 0.6, 0.4); m.position.copy(V(CX + s * 23, 269, F.zf + 4)); m.visible = false; head.add(m); return m; });
    const hiveDrips = col => [-1, 1].map(s => { const g = new THREE.Group(); g.visible = false; P.hives[s].add(g);
      const items = [[-34, 62, 44, 15, 58], [30, 66, 40, 13, 52], [2, 78, 26, 14, 64], [-52, 20, 28, 10, 34], [50, 28, 26, 10, 30]].map(([x, y, z, w, len]) => { const o = withOutline(new THREE.Mesh(dropGeo, flat(col)), 0.004); o.position.set(D(x), D(y), D(z)); o.scale.set(D(w), 0.001, D(w * 0.7)); g.add(o); return { b: o, len }; }); return { g, items }; });
    const honeyHive = hiveDrips(HONEY), jellyHive = hiveDrips(JELLY);
    const growDrips = (set, a) => { set.g.visible = a > 0.01; for (const it of set.items) it.b.scale.y = Math.max(0.001, D(it.len) * 0.5 * a); };
    const falling = Array.from({ length: 10 }, (_, i) => { const m = new THREE.Mesh(new THREE.SphereGeometry(D(7), 10, 8), flat(HONEY)); m.scale.y = 1.5; m.visible = false; fxRoot.add(m); return { m, side: i % 2 ? 1 : -1, ph: i * 0.137, x: ((i * 5) % 7 - 3) * D(14) }; });
    // Muralha de Mel (laboratório): 16 favos de LUZ em dois anéis orbitando o Jack (preenchimento a 36%, contorno claro e
    // um hexágono interno); sempre de frente para a câmera, como no original. Some em etapas (3 golpes no jogo)
    const hexFill = new THREE.CircleGeometry(1, 6), hexEdge = new THREE.RingGeometry(0.9, 1, 6), hexIn = new THREE.RingGeometry(0.53, 0.58, 6);
    const wallCells = Array.from({ length: 16 }, (_, i) => { const up = i < 8, g = new THREE.Group(), mats = [glow('#ffd21e', 0.36), glow('#fff88c', 0.88), glow('#ffffdc', 0.5)];
      [hexFill, hexEdge, hexIn].forEach((h, k) => { const m = new THREE.Mesh(h, mats[k]); m.rotation.z = Math.PI / 6; g.add(m); }); g.visible = false; fxRoot.add(g);
      return { g, mats, a: ((up ? i : i - 8 + 0.5) / 8) * Math.PI * 2, y: up ? 1.12 : 0.74, ph: i * 0.4, stage: i % 3 }; });
    const WALL = { t: 6.5, r: 0.78, hex: 0.17 };
    // Geleia Real: brilhos perolados subindo em volta dele enquanto cura
    const sparks = Array.from({ length: 22 }, (_, i) => { const m = new THREE.Mesh(new THREE.OctahedronGeometry(D(9)), new THREE.MeshBasicMaterial({ color: i % 3 ? '#fff6b0' : '#ffffff' })); m.visible = false; fxRoot.add(m); return { m, a: i * 2.399, ph: i * 0.09, r: 0.42 + (i % 4) * 0.09 }; });
    // Pele de Mel (passiva): a pele toda fica âmbar
    const skinCol = new THREE.Color(C.skin), skinMats = new Set(); root.traverse(o => { if (o.isMesh && o.material.color?.equals(skinCol)) skinMats.add(o.material); });
    let honeySkin = false; const hsb = document.querySelector('#honeySkin');
    if (hsb) hsb.onclick = () => { honeySkin = !honeySkin; hsb.style.outline = honeySkin ? '2px solid #fc6' : ''; for (const m of skinMats) m.color.set(honeySkin ? '#d97706' : C.skin); headU.uSide.value.set(honeySkin ? '#b45309' : C.skinD); };
    const skills = {
      // a colmeia sobe até encostar no chapéu/capuz (25%), segura enquanto as abelhas dão a volta e só desce depois de ele assentar
      signature: { dur: 2.6, start: t => { FX.sig = t; setFace('angry'); }, pose: (k, pose) => { const l = ss(0, 0.65, k) * (1 - ss(2.2, 2.6, k)); pose[1] = [0.02 * l, 0.5 * l, 0.8 * l, 0, 0]; return { twist: -0.1 * l, tilt: 0.05 * l, lean: -0.05 * l }; } },
      sting: { dur: 0.75, start: t => { FX.sting = t; }, pose: (k, pose) => { const j = ss(0, 0.2, k), hit = ss(0.2, 0.3, k) * (1 - ss(0.5, 0.75, k)), back = j * (1 - ss(0.2, 0.28, k)), rel = 1 - ss(0.55, 0.75, k), v = {};
        for (const s of [-1, 1]) v[s] = [-0.1 * back + 0.5 * easeO(hit), 0.04 * j * rel, 1.5 * j * rel, 0, 0]; setPose(pose, v); return { lean: 0.16 * hit - 0.06 * back, drop: 0.02 * back }; } },
      swarm: { dur: 0.9, start: t => { FX.swarm = t + 0.25; }, pose: (k, pose) => { const e = ss(0.05, 0.3, k) * (1 - ss(0.65, 0.9, k)); setPose(pose, both(e)); return { twist: 0.1 * Math.sin(k / 0.9 * Math.PI * 4) * e, lean: 0.08 * e }; } },
      comb: { dur: 0.9, start: t => { FX.comb = t + 0.32; for (const h of shards) h.set = false; }, pose: (k, pose) => { const w = ss(0, 0.28, k) * (1 - ss(0.28, 0.36, k)), e = ss(0.28, 0.36, k) * (1 - ss(0.6, 0.9, k)), v = both(e, 0.1, 0.25, 0.5); for (const s of [-1, 1]) { v[s][0] -= 0.14 * w; v[s][2] += 0.3 * w; } setPose(pose, v); return { lean: 0.14 * e - 0.08 * w, drop: 0.03 * w }; } },
      molasses: { dur: 0.8, start: t => { FX.molasses = t; setFace('molasses'); }, pose: (k, pose) => { const e = ss(0, 0.2, k) * (1 - ss(0.55, 0.8, k)), sh = Math.sin(k * 50) * 0.006 * e; setPose(pose, both(e, 0.16 + sh, 0.45, 0.12)); return { drop: 0.04 * e, lean: 0.1 * e, tilt: sh * 2 }; } },
      wall: { dur: 0.7, start: t => { FX.wall = t + 0.2; }, pose: (k, pose) => { const e = ss(0, 0.2, k) * (1 - ss(0.5, 0.7, k)); setPose(pose, both(e, 0.08, 0.75, 0.3)); return { lean: -0.05 * e }; } },
      jelly: { dur: 1.1, start: t => { FX.jelly = t; }, pose: (k, pose) => { const e = ss(0, 0.25, k) * (1 - ss(0.8, 1.1, k)); setPose(pose, both(e, 0.3, 0.55, 0.02)); return { lean: -0.1 * e, tilt: 0.04 * Math.sin(k * 6) * e }; } },
    };
    const fxFrame = c => {
      const t = c.t;
      // ferrão de luz
      { const k = t - FX.sting, on = k > 0.21 && k < 0.55; stinger.visible = on;
        if (on) { const p = k / 0.75, reach = p <= 0.4 ? Math.sin((p - 0.28) / 0.12 * Math.PI / 2) : Math.cos((p - 0.4) / 0.34 * Math.PI / 2), R = Math.max(0.02, reach);
          stinger.position.copy(P.hives[-1].position).add(P.hives[1].position).multiplyScalar(0.5); stinger.position.z += D(70); stinger.scale.set(0.5 + 0.5 * R, 0.5 + 0.5 * R, 1.7 * R);
          stingRings.forEach((m, i) => { const q = (i + 1) / 3, e = (0.1 + 0.2 * q) * (1 + q); m.scale.set(e, e, 1); m.material.opacity = (1 - q * 0.8) * R * 0.9; }); } }
      // assinatura: o chapéu/capuz só mexe depois que a colmeia encosta, girando em torno do alto da cabeça (sem sair dela);
      // as abelhas saem de baixo dele, dão uma volta completa em torno do Jack e voltam para dentro
      { const k = t - FX.sig, p = k / 2.6, l = ss(0.25, 0.34, p) * (1 - ss(0.74, 0.84, p)), A = P.hat.visible ? P.hat : P.hood, a = -0.16 * l, ca = Math.cos(a), sa = Math.sin(a), px = X(CX), py = Y(232);
        A.rotation.z = a; A.position.set(px - (px * ca - py * sa) + D(4) * l, py - (px * sa + py * ca) + D(12) * l, 0);
        for (const b of sigBees) { const q = (p - 0.34 - b.i * 0.009) / 0.26, on = q > 0 && q < 1; b.g.visible = on; if (!on) continue;
          const out = Math.sin(q * Math.PI), an = Math.PI / 2 + q * Math.PI * 2, rr = 0.03 + out * 0.78;
          b.g.position.set(Math.cos(an) * rr, 1.5 - out * 0.55 + Math.sin(q * 12 + b.i) * 0.03 * out, Math.sin(an) * rr); b.g.rotation.y = -an; b.g.scale.setScalar(0.25 + 0.75 * Math.min(1, out * 2.5)); flap(b, t, b.i); }
        if (k > 2.6 && k < 90) { setFace('normal'); FX.sig = -99; } }
      // enxame
      const sw = t - FX.swarm;
      for (const b of bees) { const q = sw - b.delay, on = q > 0 && q < 6; b.g.visible = on; if (!on) continue;
        const ang = b.a + t * b.sp, tgt = new THREE.Vector3(SWARM.x + Math.cos(ang) * b.r * 1.4, SWARM.y + b.y + Math.sin(t * 5 + b.a) * 0.05, SWARM.z + Math.sin(ang) * b.r), f = ss(0, 0.55, q);
        const prev = b.g.position.clone(); b.g.position.copy(q < 0.55 ? mouth(b.side).lerp(tgt, f) : tgt); b.g.scale.setScalar(Math.max(0.01, 1 - ss(5.6, 6, q)));
        const dir = b.g.position.clone().sub(prev); if (dir.lengthSq() > 1e-7) b.g.rotation.y = Math.atan2(dir.x, dir.z);
        flap(b, t, b.a); }
      // favos
      const cb = t - FX.comb;
      for (const h of shards) { const on = cb > 0 && cb < 1.1; h.g.visible = on; if (!on) continue; if (!h.set) { h.o.copy(mouth(h.side)); h.set = true; }
        h.g.position.copy(h.o).addScaledVector(h.v, cb); h.g.position.y -= 3.2 * cb * cb; h.g.rotation.set(cb * h.spin, cb * h.spin * 0.7, 0); h.g.scale.setScalar(Math.max(0.01, 1 - ss(0.8, 1.1, cb))); }
      rings.forEach((m, i) => { const on = cb > 0 && cb < 0.4; m.visible = on; if (on) { m.position.copy(mouth(i ? 1 : -1)); m.scale.setScalar(0.05 + cb * 1.3); m.material.opacity = 1 - cb / 0.4; } });
      // melaço
      const ml = t - FX.molasses, ma = ss(0.1, 0.8, ml) * (1 - ss(5.6, 6, ml));
      growDrips(honeyCoat, ma); for (const d of honeyHive) growDrips(d, ma);
      for (const g of hiveCracks) g.visible = ma > 0.02;
      for (const m of eyeGlow) { m.visible = ma > 0.02; m.material.opacity = ma * (0.4 + 0.25 * Math.sin(t * 6)); }
      for (const f of falling) { const on = ma > 0.6; f.m.visible = on; if (on) { const q = (t * 0.9 + f.ph) % 1, p = P.hives[f.side].position; f.m.position.set(p.x + f.x, (p.y - D(70)) * (1 - q), p.z + D(10)); } }
      if (ml > 6 && ml < 90) { setFace('normal'); FX.molasses = -99; }
      // muralha: gira, pulsa e abre do centro para o raio; cai em etapas
      { const wl = t - FX.wall, cam = window.VIN?.camera, sp = ss(0, 0.4, wl);
        for (const w of wallCells) { const out = WALL.t - (2 - w.stage) * 1.6, a = sp * (1 - ss(out, out + 0.35, wl)), on = wl > 0 && a > 0.01; w.g.visible = on; if (!on) continue;
          const an = w.a + wl * 1.44, r = WALL.r * (0.35 + 0.65 * sp), pu = 0.75 + 0.25 * Math.sin(t * 4.2 + w.ph);
          w.g.position.set(Math.cos(an) * r, w.y + Math.sin(t * 3.5 + w.ph) * 0.02, Math.sin(an) * r); w.g.scale.setScalar(WALL.hex * (0.4 + 0.6 * sp)); if (cam) w.g.quaternion.copy(cam.quaternion);
          w.mats[0].opacity = 0.36 * a * pu; w.mats[1].opacity = 0.88 * a * pu; w.mats[2].opacity = 0.5 * a * pu; } }
      // geleia
      const jl = t - FX.jelly, ja = ss(0.1, 0.5, jl) * (1 - ss(2.1, 2.8, jl));
      growDrips(jellyCoat, ja); for (const d of jellyHive) growDrips(d, ja);
      for (const m of sparks) { const q = jl - m.ph, on = q > 0 && q < 1.5; m.m.visible = on; if (on) { const u = q / 1.5; m.m.position.set(Math.cos(m.a + u * 2) * m.r, 0.25 + u * 1.5, Math.sin(m.a + u * 2) * m.r * 0.8); m.m.rotation.y = t * 4; m.m.scale.setScalar(Math.max(0.01, Math.sin(u * Math.PI))); } }
    };

    const onFrame = c => {
      capeSim(c); if (P.hat.visible) hairSim(c);
      for (const sx of [-1, 1]) {
        const G = P.hives[sx], k = c.skill ? 0 : Math.min(1, Math.max(0, c.pose[sx][0] / 0.3));   // nos especiais a boca fica para a frente
        G.position.copy(hands[sx].position);
        const yq = new THREE.Quaternion().setFromAxisAngle(UPV, upper.rotation.y * 0.8);
        // guarda: boca para a frente e um pouco para fora; no golpe ele vira a mão: a boca aponta para baixo e o alto do
        // cesto vai para a frente (é com ele que bate)
        const dir = new THREE.Vector3(sx * 0.25, 0.05, 1).lerp(new THREE.Vector3(0, -1, 0.08), k).normalize().applyQuaternion(yq);
        const ref = UPV.clone().lerp(FWD, k).normalize().applyQuaternion(yq);
        const yA = ref.addScaledVector(dir, -ref.dot(dir)).normalize(), xA = new THREE.Vector3().crossVectors(yA, dir);
        G.quaternion.setFromRotationMatrix(new THREE.Matrix4().makeBasis(xA, yA, dir));
      }
      fxFrame(c);
    };
    P.cloth = [{ geos: [P.capeGeo], top: Y(335), bot: Y(526), pow: 1.5, gain: 1 }, { geos: [P.hairGeo], top: Y(238), bot: Y(308), pow: 1.3, gain: 0.45 }];   // tools3d/export
    return { P, upper, WAIST, neck, NECK, hips, HIPS, hands, hipSway: 0.5, hand: { x: 0.62, y: Y(385), z: 0.34, yaw: -0.1 }, placeFoot, setFace, skills, onFrame };
  },
};
