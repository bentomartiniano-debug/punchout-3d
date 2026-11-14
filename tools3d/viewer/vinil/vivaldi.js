// Maestra Vivaldi no modo vinil, modelada pela folha de modelo do usuário (assets3d/vivaldi/ref/vivaldi_turnaround.png:
// frente, perfil esquerdo, costas e perfil direito). Medidas em px DA FOLHA: x e y da vista de frente, z = x do perfil
// − 595 (a frente para +z). Escala 350 px = 1 m; chão em y = 806, eixo em x = 222.
//  - cabelão de nuvem menta: uma malha só, a união de gomos redondos (vincos entre eles), com o degradê e o balanço do
//    cabelo da Valéria (escuro embaixo, quase branco no alto, placas escorrendo; mola e ondulação)
//  - rosto claro de cantos redondos com a viseira verde facetada em cima, os óculos dourados de lentes ciano (olhos de
//    tédio, pálpebra pesada) e a boca roxa; pescoço escuro
//  - peitilho branco-lilás em V, casaco roxo com as dragonas e os suspensórios dourados, cinto com a fivela, saia rodada
//    com a faixa branca
//  - no lugar das pernas a cauda de fantasma em espiral, do verde ao azul-noite, com a ponta dobrada para trás, balançando
//  - sem braços (como no jogo): as luvas redondas de fantasma flutuam (fiapo no lugar do pulso); a esquerda segura a
//    batuta de maestro
import { THREE, ss, INKS, flat, withOutline } from './core.js';
import { TessellateModifier } from 'three/addons/modifiers/TessellateModifier.js';
import { mergeVertices } from 'three/addons/utils/BufferGeometryUtils.js';
import { RoundedBoxGeometry } from 'three/addons/geometries/RoundedBoxGeometry.js';

const S_PX = 350, FOOT = 806, CX = 222;
const C = { hair: '#a9dcc0', vein: '#3aa877', veinHi: '#5cc596', skin: '#e9f1ec', visor: '#7cc4a2', lens: '#62d3dc', lid: '#3fb3c2', iris: '#123a40',
            gold: '#f2b417', lip: '#5b2a80', neck: '#2b2838', bib: '#ece6f6', purple: '#4b2382', purpleLight: '#6b3bb0', buckle: '#8d5fd8',
            white: '#f2f0f4', tail: '#5f9e80', tailTip: '#2a2c48', hand: '#bfe8d2', ring: '#cdeee0' };
const col = h => ({ value: new THREE.Color(h) });

export default {
  title: 'Maestra Vivaldi',
  arena: 'vivaldi',   // abre na arena própria (core.js)
  units: [S_PX, FOOT, CX],
  faces: [['normal', 'Normal'], ['angry', 'Raiva'], ['pain', 'Dor'], ['ko', 'K.O.']],
  skills: [['signature', 'Assinatura'], ['staccato', 'Cadência Staccato'], ['winter', 'Inverno Allegro'], ['seasons', 'Modo Quatro Estações'], ['jab', 'Estocada em Crescendo'], ['requiem', 'Réquiem de Outono'], ['spring', 'Sonata de Primavera']],
  hudExtra: 'Estação: <button data-season="-1">Concerto</button><button data-season="0">Primavera</button><button data-season="1">Verão</button><button data-season="2">Outono</button><button data-season="3">Inverno</button>',
  others: [['valeria', 'Valéria'], ['frog', 'King Frog'], ['joana', 'Joana'], ['marcov', 'Marcov'], ['espinha', 'Espinha'], ['cremoso', 'Cremoso']],
  async build({ root, U }) {
    const { X, Y, D } = U, P = {};
    const V = (x, y, z) => new THREE.Vector3(X(x), Y(y), D(z));
    const ol = (m, w = 0.009) => {
      const g = withOutline(m, w); g.position.copy(m.position); g.quaternion.copy(m.quaternion); g.scale.copy(m.scale);
      m.position.set(0, 0, 0); m.quaternion.identity(); m.scale.set(1, 1, 1); return g; };
    function pattern(mat, glsl, key, uniforms = {}) {   // padrão em px da folha (vPx)
      const prev = mat.onBeforeCompile, k0 = mat.customProgramCacheKey();
      mat.onBeforeCompile = (sh, r) => {
        prev.call(mat, sh, r); Object.assign(sh.uniforms, uniforms);
        sh.vertexShader = 'varying vec3 vPx;\n' + sh.vertexShader.replace('#include <begin_vertex>', `#include <begin_vertex>
          vPx = vec3(position.x * ${S_PX.toFixed(1)} + ${CX.toFixed(1)}, ${FOOT.toFixed(1)} - position.y * ${S_PX.toFixed(1)}, position.z * ${S_PX.toFixed(1)});`);
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
    const rowsOf = (tab, pw, y0, y1, n) => Array.from({ length: n + 1 }, (_, i) => { const y = y0 + (y1 - y0) * i / n, [a, zf, zb] = lerpT(tab, y); return { y, pt: oval(CX, a, zf, zb, pw) }; });
    const frontZ = (tab, pw, x, y) => { const [a, zf] = lerpT(tab, y), c = Math.max(-1, Math.min(1, (x - CX) / a)), cc = Math.sign(c) * Math.pow(Math.abs(c), pw / 2); return zf * sp(Math.sqrt(Math.max(0, 1 - cc * cc)), pw); };
    // placa fina assentada numa superfície: contorno em (x, y) px (furos opcionais), z = surf(x, y) + lift
    const plate = (pts, surf, lift, depth, mat, holes = []) => {
      const sh = new THREE.Shape(pts.map(([x, y]) => new THREE.Vector2(x, y)));
      for (const h of holes) sh.holes.push(new THREE.Path(h.map(([x, y]) => new THREE.Vector2(x, y))));
      let g = new THREE.ExtrudeGeometry(sh, { depth, bevelEnabled: false, curveSegments: 8 });
      g = new TessellateModifier(3, 8).modify(g);
      const p = g.attributes.position;
      for (let i = 0; i < p.count; i++) { const x = p.getX(i), y = p.getY(i), z = p.getZ(i); p.setXYZ(i, X(x), Y(y), D(surf(x, y) + lift + z)); }
      g.deleteAttribute('normal'); g.deleteAttribute('uv'); g = mergeVertices(g, 1e-6);
      g.index.array.reverse();   // o y da forma vira −y no mundo: desvira as faces
      g.computeVertexNormals(); mat.side = THREE.DoubleSide;
      return new THREE.Mesh(g, mat);
    };
    const tubeVar = (pts, rFn, NA = 12) => {   // tubo de raio variável (u de 0 a 1 ao longo)
      const c = new THREE.CatmullRomCurve3(pts.map(p => V(...p))), NS = 64, pos = [], idx = [], fr = c.computeFrenetFrames(NS, false);
      for (let i = 0; i <= NS; i++) { const u = i / NS, p = c.getPointAt(u), n = fr.normals[i], b = fr.binormals[i], r = D(rFn(u));
        for (let j = 0; j <= NA; j++) { const a = j / NA * Math.PI * 2; pos.push(p.x + (n.x * Math.cos(a) + b.x * Math.sin(a)) * r, p.y + (n.y * Math.cos(a) + b.y * Math.sin(a)) * r, p.z + (n.z * Math.cos(a) + b.z * Math.sin(a)) * r); } }
      for (let i = 0; i < NS; i++) for (let j = 0; j < NA; j++) { const a = i * (NA + 1) + j, b = a + 1, cc = a + NA + 1, d = cc + 1; idx.push(a, b, cc, b, d, cc); }
      const g = new THREE.BufferGeometry(); g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3)); g.setIndex(idx); g.computeVertexNormals(); return g;
    };

    // ================= grupos do motor
    const seasonU = { uSeason: { value: 0 }, uS0: { value: new THREE.Color('#fff0f7') }, uS1: { value: new THREE.Color('#f9a8d4') }, uS2: { value: new THREE.Color('#db2777') } };   // peruca das estações: alto, meio, embaixo
    const WAIST = Y(590), NECK = Y(505), HIPS = Y(606);
    const upper = new THREE.Group(); upper.position.y = WAIST; root.add(upper);
    const body = new THREE.Group(); body.position.y = -WAIST; upper.add(body);
    const neck = new THREE.Group(); neck.position.y = NECK - WAIST; upper.add(neck);
    const head = new THREE.Group(); head.position.y = -NECK; neck.add(head);

    const hairT = { value: 0 }, hairLag = { value: new THREE.Vector3() };
    // ================= cabelão de nuvem: união de gomos (esferas), puxada de um centro: cada direção vai até o gomo mais
    // longe — uma malha só, com o vinco onde um gomo encontra o outro
    { const O = new THREE.Vector3(222, 390, -40);
      const SOFT = 20;   // largura (px) da curva onde um gomo encontra o outro
      const PUFFS = [[222, 300, -12, 75], [163, 312, -22, 60], [281, 312, -22, 60], [222, 366, 40, 50], [178, 378, 30, 40], [266, 378, 30, 40],
        [124, 378, -25, 58], [320, 378, -25, 58], [122, 442, -16, 52], [322, 442, -16, 52], [140, 474, -12, 30], [304, 474, -12, 30],
        [142, 452, 14, 40], [302, 452, 14, 40], [222, 312, -86, 68], [222, 392, -88, 64], [222, 452, -80, 48], [164, 418, -76, 54], [280, 418, -76, 54]]
        .map(([x, y, z, r]) => ({ c: new THREE.Vector3(x, y, z), r }));
      const g = new THREE.SphereGeometry(1, 160, 120), p = g.attributes.position, d = new THREE.Vector3(), oc = new THREE.Vector3();
      for (let i = 0; i < p.count; i++) { d.set(p.getX(i), -p.getY(i), p.getZ(i)).normalize();   // y da folha para baixo
        // união suave (máximo amaciado): o gomo encontra o vizinho numa curva, não num vinco (o vinco puxava risco do contorno)
        let t = 0; for (const P2 of PUFFS) { oc.subVectors(P2.c, O); const b = d.dot(oc), disc = b * b - (oc.lengthSq() - P2.r * P2.r);
          if (disc >= 0) { const ti = b + Math.sqrt(disc), h = Math.max(SOFT - Math.abs(t - ti), 0) / SOFT; t = Math.max(t, ti) + h * h * SOFT / 4; } }
        p.setXYZ(i, X(O.x + d.x * t), Y(O.y + d.y * t), D(O.z + d.z * t)); }
      g.deleteAttribute('normal'); g.deleteAttribute('uv'); const mg = mergeVertices(g, 1e-6); mg.computeVertexNormals();

      // degradê e movimento como o cabelo da Valéria: menta-escuro embaixo, menta, quase branco no alto, com placas macias
      // que escorrem devagar (veios um tico mais claros entre elas); o cabelo balança — fica para trás quando a cabeça mexe
      // (mola) e ondula de leve, mais nos gomos de baixo e de trás, nada no alto preso na cabeça
      mg.computeBoundingBox(); const bb = mg.boundingBox;
      const hairU = { uT: hairT, uLag: hairLag, uMin: { value: bb.min.clone() }, uMax: { value: bb.max.clone() } };
      const SWAY = `
        vec3 hH(vec3 p) { p = vec3(dot(p, vec3(127.1, 311.7, 74.7)), dot(p, vec3(269.5, 183.3, 246.1)), dot(p, vec3(113.5, 271.9, 124.6))); return fract(sin(p) * 43758.5453); }
        float hNoise(vec3 p) { vec3 i = floor(p), f = fract(p); f = f * f * (3.0 - 2.0 * f);
          float n = mix(mix(mix(hH(i).x, hH(i + vec3(1,0,0)).x, f.x), mix(hH(i + vec3(0,1,0)).x, hH(i + vec3(1,1,0)).x, f.x), f.y),
                        mix(mix(hH(i + vec3(0,0,1)).x, hH(i + vec3(1,0,1)).x, f.x), mix(hH(i + vec3(0,1,1)).x, hH(i + vec3(1,1,1)).x, f.x), f.y), f.z);
          return n * 2.0 - 1.0; }
        vec3 hairSway(vec3 p) { vec3 q = (p - uMin) / (uMax - uMin);
          float w = pow(1.0 - q.y, 1.6) * 0.9 + 0.5 * (1.0 - q.z) * (1.0 - q.y) + 0.08;
          w *= 1.0 - 0.9 * smoothstep(0.5, 0.8, q.z) * (1.0 - smoothstep(0.35, 0.6, q.y));   // em volta do rosto quase não mexe (não entra nele)
          vec3 wob = vec3(sin(uT * 1.9 + q.y * 5.0 + q.z * 3.0), 0.25 * sin(uT * 1.3 + q.x * 6.0), cos(uT * 1.6 + q.x * 4.0)) * 0.007;
          return p + (uLag + wob) * w; }
`;
      const m = flat(C.hair), prevHC = m.onBeforeCompile, k0H = m.customProgramCacheKey();   // por cima da sombra de quadrinho
      m.onBeforeCompile = (sh, r) => { prevHC.call(m, sh, r); Object.assign(sh.uniforms, hairU, seasonU);
        sh.vertexShader = 'uniform float uT; uniform vec3 uLag, uMin, uMax; varying vec3 vHP;' + SWAY + sh.vertexShader.replace('#include <begin_vertex>', '#include <begin_vertex>\nvHP = position; transformed = hairSway(position);');
        sh.fragmentShader = 'uniform float uT, uSeason; uniform vec3 uLag, uMin, uMax, uS0, uS1, uS2; varying vec3 vHP;' + SWAY + sh.fragmentShader.replace('#include <map_fragment>', `#include <map_fragment>
          vec3 q = (vHP - uMin) / (uMax - uMin);
          float flow = 0.045 * sin(q.x * 13.0 + uT * 1.3) * sin(q.z * 11.0 - uT * 0.9) + 0.03 * sin((q.y * 9.0 - uT * 0.7) * 3.0);
          float t = clamp(q.y + flow, 0.0, 1.0);
          vec3 c = mix(vec3(0.24, 0.55, 0.43), vec3(0.47, 0.78, 0.63), smoothstep(0.05, 0.4, t));
          c = mix(c, vec3(0.68, 0.88, 0.77), smoothstep(0.4, 0.7, t));
          c = mix(c, vec3(0.90, 0.98, 0.94), smoothstep(0.72, 0.97, t));
          // caracóis escorrendo (no lugar dos veios de magma da Valéria): espirais claras espalhadas numa grade solta em
          // volta do cabelo, cada uma girando devagar, e a grade toda descendo; manchinha suave por baixo
          vec3 pp = vec3(vHP.x * ${S_PX.toFixed(1)} + ${CX.toFixed(1)}, ${FOOT.toFixed(1)} - vHP.y * ${S_PX.toFixed(1)}, vHP.z * ${S_PX.toFixed(1)}) - vec3(222.0, 390.0, -40.0);
          vec2 cyl = vec2(atan(pp.z, pp.x) * 130.0, pp.y - uT * 5.0);   // em volta (px de arco) e altura, descendo
          vec2 cid = floor(cyl / 54.0); float sw = 0.0;
          for (int i = -1; i <= 1; i++) for (int j = -1; j <= 1; j++) {
            vec2 cc = cid + vec2(float(i), float(j)); vec3 rh = hH(vec3(cc, 7.0));
            vec2 ctr = (cc + 0.25 + 0.5 * rh.xy) * 54.0, dv = cyl - ctr; float R = 15.0 + 7.0 * rh.z, r = length(dv);
            if (r > R) continue;
            float hand = rh.x > 0.5 ? 1.0 : -1.0, th = hand * atan(dv.y, dv.x) / 6.2831853 + uT * 0.12 * hand + rh.y, b = R / 2.2;
            float turn = r / b - fract(th), f = fract(r / b - th), e = abs(f - 0.5) * b;
            float k = 1.0 - smoothstep(R * 0.8, R, r), aa = fwidth(e) + 0.2;
            sw = max(sw, (1.0 - smoothstep(1.3 - aa, 1.3 + aa, e)) * k); }
          vec3 fp = q * vec3(5.0, 6.0, 5.0) + vec3(0.0, uT * 0.18, 0.0);
          c = mix(c * (0.94 + 0.06 * hNoise(fp * 2.3)), min(c * 1.14 + 0.06, vec3(1.0)), sw * 0.75);
          vec3 sc = mix(mix(uS2, uS1, smoothstep(0.08, 0.5, t)), uS0, smoothstep(0.55, 0.95, t));
          c = mix(c, min(sc * (1.0 + 0.14 * sw), vec3(1.0)), uSeason);
          diffuseColor.rgb = c;`); };
      m.customProgramCacheKey = () => k0H + '|vivHair';
      P.hair = new THREE.Mesh(mg, m); head.add(P.hair);
      const hm = new THREE.MeshBasicMaterial({ color: 0x14141c, side: THREE.BackSide });   // contorno que balança junto
      hm.onBeforeCompile = sh => { Object.assign(sh.uniforms, hairU);
        sh.vertexShader = 'uniform float uT; uniform vec3 uLag, uMin, uMax;' + SWAY + sh.vertexShader.replace('#include <begin_vertex>', '#include <begin_vertex>\ntransformed = hairSway(position) + normal * 0.010;')
          // casca recuada na linha de visão (como a da Valéria): some nas dobras entre os gomos, fica só na silhueta
          .replace('#include <project_vertex>', 'vec4 mvPosition = modelViewMatrix * vec4(transformed, 1.0); mvPosition.xyz += normalize(mvPosition.xyz) * 0.024; gl_Position = projectionMatrix * mvPosition;'); };
      hm.customProgramCacheKey = () => 'vivHairHull';
      P.hair.add(new THREE.Mesh(mg, hm)); }

    // ================= rosto: bloco claro de cantos redondos, frente quase reta, queixo arredondado
    const FACE = [[392, 2, 60, 6], [396, 62, 72, 10], [430, 63, 76, 12], [465, 63, 76, 12], [490, 52, 70, 12], [504, 34, 60, 10], [510, 2, 52, 6]], FPW = 3;
    const faceZ = (x, y) => frontZ(FACE, FPW, x, y);
    const FX0 = 150, FX1 = 294, FY0 = 390, FY1 = 512, FW = 1024, FH = Math.round(FW * (FY1 - FY0) / (FX1 - FX0));
    // óculos: lente de cada lado (dx para fora, dy para baixo), em volta do centro do olho
    const LENS = [[-22, -10], [-13, -18], [20, -19], [27, -9], [27, 11], [19, 20], [-17, 20], [-25, 10]];
    const lensPts = (s, k = 1) => LENS.map(([dx, dy]) => [222 + s * 34 + s * dx * k, 442 + dy * k]);
    const VISOR = [[166, 393], [278, 393], [288, 413], [250, 414], [222, 419], [194, 414], [156, 413]], BRIDGE = [[214, 437], [230, 437], [230, 446], [214, 446]];
    const ink = (g, w) => { g.strokeStyle = INKS; g.lineWidth = w; g.lineCap = 'round'; g.lineJoin = 'round'; };
    function vivaldiFace(g, f) {
      g.fillStyle = C.skin; g.fillRect(FX0, FY0, FX1 - FX0, FY1 - FY0);
      // bochechas um tico mais escuras para os lados
      for (const s of [-1, 1]) { const gr = g.createRadialGradient(222 + s * 60, 470, 4, 222 + s * 60, 470, 40); gr.addColorStop(0, 'rgba(150,190,170,0.35)'); gr.addColorStop(1, 'rgba(150,190,170,0)'); g.fillStyle = gr; g.fillRect(FX0, 420, FX1 - FX0, 90); }
      for (const s of [-1, 1]) {
        const cx = 222 + s * 34, cy = 443, L = lensPts(s, 1.0);
        g.save(); g.beginPath(); L.forEach(([x, y], i) => i ? g.lineTo(x, y) : g.moveTo(x, y)); g.closePath(); g.fillStyle = C.lens; g.fill(); g.clip();
        if (f === 'pain') { ink(g, 3.2); g.beginPath(); g.moveTo(cx + s * 9, cy - 7); g.lineTo(cx - s * 7, cy); g.lineTo(cx + s * 9, cy + 7); g.stroke(); }
        else if (f === 'ko') { ink(g, 3.2); g.beginPath(); g.moveTo(cx - 8, cy - 8); g.lineTo(cx + 8, cy + 8); g.moveTo(cx + 8, cy - 8); g.lineTo(cx - 8, cy + 8); g.stroke(); }
        else {   // olho de tédio: todo branco (sem pupila), pálpebra de cima pesada e reta (na raiva desce para o nariz)
          g.fillStyle = '#ffffff'; g.beginPath(); g.ellipse(cx, cy + 2, 15, 10, 0, 0, 7); g.fill();
          const yi = f === 'angry' ? cy + 3 : cy - 1, yo = f === 'angry' ? cy - 6 : cy - 3;
          g.fillStyle = C.lid; g.beginPath(); g.moveTo(cx - s * 20, yi); g.lineTo(cx + s * 20, yo); g.lineTo(cx + s * 30, cy - 30); g.lineTo(cx - s * 30, cy - 30); g.closePath(); g.fill();
          ink(g, 2.6); g.beginPath(); g.moveTo(cx - s * 16, yi + 0.4); g.lineTo(cx + s * 17, yo + 0.4); g.stroke();
          ink(g, 1.4); g.beginPath(); g.ellipse(cx, cy + 2, 15, 10, 0, 0.15, Math.PI - 0.15); g.stroke(); }
        g.restore();
      }
      // os riscos da viseira e dos óculos pintados no rosto (a borda das peças em relevo cai em cima deles)
      const poly = (pts, w) => { ink(g, w); g.beginPath(); pts.forEach(([x, y], i) => i ? g.lineTo(x, y) : g.moveTo(x, y)); g.closePath(); g.stroke(); };
      poly(VISOR, 2.4); for (const s of [-1, 1]) { poly(lensPts(s, 1.28), 2.2); poly(lensPts(s, 1.0), 1.6); } poly(BRIDGE, 1.6);
      // boca roxa pequena; raiva = dentes cerrados; dor = ondulada; K.O. = "oh"
      if (f === 'angry') { g.fillStyle = C.lip; g.beginPath(); g.ellipse(222, 482, 14, 6.5, 0, 0, 7); g.fill(); g.fillStyle = '#f6f2fa'; g.fillRect(211, 479, 22, 6); ink(g, 1); g.beginPath(); for (const x of [217, 222, 227]) { g.moveTo(x, 479); g.lineTo(x, 485); } g.stroke(); }
      else if (f === 'pain') { ink(g, 2.2); g.strokeStyle = C.lip; g.beginPath(); g.moveTo(210, 482); for (let k = 0; k < 3; k++) g.quadraticCurveTo(214 + k * 8, k % 2 ? 486 : 478, 218 + k * 8, 482); g.stroke(); }
      else if (f === 'ko') { g.fillStyle = C.lip; g.beginPath(); g.ellipse(222, 483, 6, 7, 0, 0, 7); g.fill(); g.fillStyle = '#2a1030'; g.beginPath(); g.ellipse(222, 484, 3.5, 4.5, 0, 0, 7); g.fill(); }
      else { g.fillStyle = C.lip; g.beginPath(); g.moveTo(209, 482); g.quadraticCurveTo(215, 475, 222, 478); g.quadraticCurveTo(229, 475, 235, 482); g.quadraticCurveTo(222, 490, 209, 482); g.fill(); }
    }
    const faceTex = f => { const c = document.createElement('canvas'); c.width = FW; c.height = FH; const g = c.getContext('2d'), k = FW / (FX1 - FX0); g.scale(k, k); g.translate(-FX0, -FY0); vivaldiFace(g, f);
      const t = new THREE.CanvasTexture(c); t.colorSpace = THREE.SRGBColorSpace; t.anisotropy = 8; return t; };
    const headU = { tFace: { value: faceTex('normal') } };
    { const m = pattern(flat(C.skin), `{ vec2 fuv = vec2((vPx.x - ${FX0.toFixed(1)}) / ${(FX1 - FX0).toFixed(1)}, 1.0 - (vPx.y - ${FY0.toFixed(1)}) / ${(FY1 - FY0).toFixed(1)});
        if (vPx.z > 20.0) diffuseColor.rgb = texture2D(tFace, fuv).rgb;
        else diffuseColor.rgb = mix(uNape, diffuseColor.rgb, smoothstep(4.0, 20.0, vPx.z)); }`, 'vivFace', { ...headU, uNape: col('#3b4a50') });   // atrás do rosto (sob o cabelo) escurece
      head.add(withOutline(new THREE.Mesh(sections(rowsOf(FACE, FPW, 392, 510, 40)), m), 0.009)); }
    // viseira verde facetada no alto do rosto (com a ponta descendo no meio, sobre a ponte dos óculos)
    { const m = flat(C.visor); m.flatShading = true;
      head.add(plate(VISOR, (x, y) => faceZ(x, Math.max(y, 400)), 2.5, 10, m)); }   // sem casca: a casca de placa fina serrilhava; o risco vai pintado
    // óculos: aros dourados (placa com furo) e a ponte
    { const gm = flat(C.gold);
      for (const s of [-1, 1]) head.add(plate(lensPts(s, 1.28), faceZ, 3, 6, gm.clone(), [lensPts(s, 1.0)]));
      head.add(plate(BRIDGE, faceZ, 3, 6, gm.clone())); }
    // pescoço escuro
    { const m = new THREE.Mesh(new THREE.CylinderGeometry(D(24), D(28), D(30), 24), flat(C.neck)); m.position.copy(V(222, 512, 16)); head.add(m); }

    // ================= tronco: casaco roxo (dragonas e suspensórios dourados), peitilho, cinto com a fivela
    const TORSO = [[500, 46, 42, 40], [512, 74, 62, 58], [530, 82, 68, 64], [560, 80, 70, 64], [590, 84, 72, 66], [602, 82, 70, 64]], TPW = 2.6;
    const torsoZ = (x, y) => frontZ(TORSO, TPW, x, y);
    body.add(withOutline(new THREE.Mesh(sections(rowsOf(TORSO, TPW, 500, 602, 34)), flat(C.purple)), 0.009));   // desce por dentro do cinto: inclinando não abre vão
    // peitilho plissado: a própria peça branca em pregas que abrem em leque do pescoço para baixo (crista e vale)
    { const pleat = (x, y) => { const a = Math.atan2(x - CX, y - 488), r = Math.hypot(x - CX, y - 488); return 4.5 * Math.min(1, r / 34) * Math.sin(a * 11); };
      body.add(withOutline(plate([[186, 500], [258, 500], [264, 522], [248, 548], [222, 568], [196, 548], [180, 522]], (x, y) => torsoZ(x, y) + pleat(x, y), 3, 8, pattern(flat(C.bib), `{ float a = atan(vPx.x - 222.0, vPx.y - 488.0), r = length(vPx.xy - vec2(222.0, 488.0)), ph = sin(a * 11.0);
        float k = min(1.0, r / 34.0); diffuseColor.rgb = mix(diffuseColor.rgb, diffuseColor.rgb * 0.82, smoothstep(0.2, -0.9, ph) * k);   // vale da prega mais escuro
        float cs = cos(a * 11.0), w = fwidth(cs) * 1.5;   // risco no fundo de cada vale
        diffuseColor.rgb = mix(diffuseColor.rgb, vec3(0.42, 0.38, 0.52), (1.0 - smoothstep(0.0, w, abs(cs))) * step(ph, 0.0) * k * 0.75); }`, 'vivBib')), 0.006)); }
    // dragonas: blocos dourados grossos de quina chanfrada caindo pelos ombros; suspensórios em dois gomos (com a
    // juntinha no meio) descendo da dragona até o cinto, na frente e nas costas
    { const gm = flat(C.gold);
      const torsoBackZ = (x, y) => { const [a, , zb] = lerpT(TORSO, y), c = Math.max(-1, Math.min(1, (x - CX) / a)), cc = Math.sign(c) * Math.pow(Math.abs(c), TPW / 2); return zb * sp(Math.sqrt(Math.max(0, 1 - cc * cc)), TPW); };
      const strap = (s, surf) => [[524, 548], [552, 574]].map(([y0, y1]) => plate([[222 + s * 50 - 7, y0], [222 + s * 50 + 7, y0], [222 + s * 52 + 7, y1], [222 + s * 52 - 7, y1]], surf, 1, 5, gm.clone()));
      for (const s of [-1, 1]) {
        const ep = new THREE.Mesh(new RoundedBoxGeometry(D(44), D(18), D(104), 3, D(5)), gm); ep.position.copy(V(222 + s * 72, 516, 0)); ep.rotation.z = -s * 0.26; body.add(ol(ep, 0.006));
        for (const m of strap(s, torsoZ)) body.add(withOutline(m, 0.005));
        const bg = new THREE.Group(); bg.rotation.y = Math.PI; body.add(bg);   // costas: feitas de frente e viradas
        for (const m of strap(-s, (x, y) => torsoBackZ(2 * CX - x, y))) bg.add(withOutline(m, 0.005));
      } }
    const BELT = [[572, 88, 76, 70], [608, 90, 78, 72]];
    // cinto fofo: a faixa estufa no meio da altura (borda redonda em cima e embaixo, como uma boia)
    { const rows = []; for (let i = 0; i <= 14; i++) { const y = 572 + 36 * i / 14, v = i / 14, [a, zf, zb] = lerpT(BELT, y), b = 6 * Math.sin(Math.PI * v) ** 0.6;
        rows.push({ y, pt: oval(CX, a + b, zf + b, zb + b, TPW) }); }
      body.add(withOutline(new THREE.Mesh(sections(rows), flat(C.purpleLight)), 0.008)); }
    body.add(withOutline(plate([[205, 576], [239, 576], [239, 604], [205, 604]], (x, y) => frontZ(BELT, TPW, x, y) + 6, 1, 6, flat(C.buckle)), 0.005));

    // ================= quadril: saia rodada com a faixa branca e a cauda de fantasma em espiral
    const hips = new THREE.Group(); hips.position.y = HIPS; root.add(hips);
    const low = new THREE.Group(); low.position.y = -HIPS; hips.add(low);
    { const SK = [[582, 78, 64, 60], [604, 86, 72, 68], [630, 100, 84, 82], [656, 112, 92, 94], [668, 114, 93, 96], [674, 104, 84, 88], [677, 54, 40, 40]];
      // faixa branca ondulada (sobe e desce em volta) e a saia com pregas largas que abrem para a barra
      const m = pattern(flat(C.purple), `{ float th = atan(vPx.z, vPx.x - 222.0), yc = 634.0 + 4.0 * sin(th * 3.0 + 0.6);
          if (abs(vPx.y - yc) < 6.5) diffuseColor.rgb = uWhite; }`, 'vivSkirt', { uWhite: col(C.white) });
      const rows = rowsOf(SK, 2.4, 582, 677, 38).map(r => { const k = ss(604, 668, r.y), base = r.pt;
        return { y: r.y, pt: th => { const [x, z] = base(th), f = 1 + 0.035 * k * Math.cos(th * 9); return [CX + (x - CX) * f, z * f]; } }; });
      const sg = sections(rows, 144); low.add(withOutline(new THREE.Mesh(sg, m), 0.010));   // a boca sobe por dentro do cinto e do casaco
      P.skirt = { g: sg, base: Float32Array.from(sg.attributes.position.array) }; }
    // rabicho do blazer nas costas: duas abas roxas caindo do cinto por cima da saia, com o corte em V no meio (moldadas
    // nas costas da saia; feitas como placa de frente e viradas para trás)
    const flaps = [];   // abas do rabicho (malha + posições de descanso) para o pano
    { const SKB = [[604, 86, 68], [630, 100, 82], [656, 112, 94], [668, 114, 96]];   // [y, a, z das costas]
      const backZ = (x, y) => { const yy = Math.min(y, 666), [a, zb] = lerpT(SKB, yy), c = Math.max(-1, Math.min(1, (x - CX) / a)), cc = Math.sign(c) * Math.pow(Math.abs(c), 1.2);
        return zb * sp(Math.sqrt(Math.max(0, 1 - cc * cc)), 2.4) + Math.max(0, y - 666) * 0.6; };
      const tm = flat(C.purple);
      for (const s of [-1, 1]) { const q = pts => pts.map(([dx, y]) => [CX + s * dx, y]);
        const flap = plate(s > 0 ? q([[2, 606], [74, 606], [80, 660], [72, 690], [40, 700], [12, 706], [4, 650]]) : q([[2, 606], [74, 606], [80, 660], [72, 690], [40, 700], [12, 706], [4, 650]]).reverse(), backZ, 2, 5, tm.clone());
        const fg = new THREE.Group(); fg.rotation.y = Math.PI; fg.add(withOutline(flap, 0.007)); low.add(fg);
        const fp = flap.geometry.attributes.position; flaps.push({ p: fp, base: Float32Array.from(fp.array), s, g: flap.geometry }); } }
    P.tail = new THREE.Group(); P.tail.position.copy(V(222, 672, -4)); low.add(P.tail);
    { // gomos em espiral: o raio cai da barriga até a ponta e um sulco gira em volta (sorvete de casquinha)
      const R = [[672, 50], [690, 50], [712, 45], [735, 36], [758, 26], [778, 15], [792, 5], [797, 0.5]];
      const rows = []; for (let i = 0; i <= 60; i++) { const y = 672 + 125 * i / 60, [r] = lerpT(R, y), groove = ss(690, 720, y) * (1 - ss(785, 797, y));
        // a ponta dobra para trás (e sobe um tico), como rabo de fantasma em vez de espetada no chão
        const u = Math.max(0, Math.min(1, (y - 700) / 97)), back = 62 * u * u, lift = 26 * u * u * u;
        rows.push({ y: y - lift, pt: th => { const k = 1 - 0.14 * groove * Math.pow(0.5 + 0.5 * Math.cos(th - (y - 690) * 0.16), 3), rr = r * k; return [222 + rr * Math.cos(th), -4 - back + rr * Math.sin(th)]; } }); }
      const g = sections(rows, 64); g.translate(-X(222), -Y(672), D(4));
      // (a malha é local à cauda: o y da folha é vPx.y − FOOT + 672)
      const m = pattern(flat(C.tail), `diffuseColor.rgb = mix(diffuseColor.rgb, uTip, smoothstep(700.0, 792.0, vPx.y - ${(FOOT - 672).toFixed(1)}));`, 'vivTail', { uTip: col(C.tailTip) });
      P.tail.add(withOutline(new THREE.Mesh(g, m), 0.009)); }

    // ================= mãos: luvas redondas como as da Lady Espinha, mas de fantasma (menta, punho roxo e branco) e no
    // lugar do pulso um fiapo de fantasma enrolando para baixo; a mão esquerda dela segura a batuta de maestro (cabo de
    // bulbo dourado e a vareta branca fina)
    const L = (x, y, z) => new THREE.Vector3(D(x), D(y), D(z)), LP = (x, y, z) => [CX + x, FOOT - y, z];   // px locais da luva
    const wisps = [];
    function ghostHand(sx, holdBaton) {
      const g = new THREE.Group(), hm = flat(C.hand);
      // punho liso, sem dedos (a mão sem a batuta tem só o polegar para fora: aberta)
      let fg = holdBaton ? new RoundedBoxGeometry(D(70), D(78), D(66), 8, D(27)) : new RoundedBoxGeometry(D(66), D(72), D(30), 8, D(13));   // a aberta é uma palma fina fg.deleteAttribute('normal'); fg.deleteAttribute('uv');
      if (!holdBaton) { fg = new TessellateModifier(D(2.2), 10).modify(fg); fg.deleteAttribute('normal'); }   // malha fina para puxar os dedos
      fg = mergeVertices(fg, 1e-6);
      if (!holdBaton) {   // polegar puxado da própria malha do punho (sem cilindro colado): um calombo redondo subindo pelo lado de dentro,
        // de pé macio (sem vinco) e ponta redonda
        // e os quatro dedos puxados do alto do punho, abertos em leque (a mão espalmada)
        const P3 = fg.attributes.position, v = new THREE.Vector3(), o = new THREE.Vector3();
        const BUMPS = [[L(-sx * 31, 10, 3), new THREE.Vector3(-sx * 0.6, 0.75, 0.2), 15, 24],
          ...[[-22, 28, 0.2], [-7.5, 34, 0.07], [7.5, 34, -0.07], [22, 28, -0.2]].map(([x, h, a]) => [L(x, 33, 0), new THREE.Vector3(-a, 1, 0.05), 6.8, h, true])];
        const ow = [];   // espessura do contorno por vértice: some no pé dos dedos e do polegar (o vão não risca)
        for (let i = 0; i < P3.count; i++) { v.fromBufferAttribute(P3, i); o.copy(v); let cut = 0;
          for (const [A, , R] of BUMPS) { const d = o.distanceTo(A) / D(R); cut = Math.max(cut, ss(0.7, 0.9, d) * (1 - ss(1.15, 1.5, d))); }
          ow.push(1 - 0.9 * cut);
          for (const [A, dir, R, H, col] of BUMPS) { const d = o.distanceTo(A) / D(R); if (d < 1) { const dome = Math.sqrt(1 - d * d), m = col ? 0.7 * ss(0, 1, (1 - d) * 3.2) + 0.3 * dome : dome * ss(0, 1, (1 - d) * 2.4);
            v.addScaledVector(dir.clone().normalize(), D(H) * m); } }   // dedo: coluna de ponta redonda; polegar: calombo
          P3.setXYZ(i, v.x, v.y, v.z); }
        fg.setAttribute('ow', new THREE.Float32BufferAttribute(ow, 1)); }
      fg.computeVertexNormals();
      const fist = new THREE.Mesh(fg, hm); fist.position.copy(L(0, 8, 0));
      if (holdBaton) g.add(ol(fist, 0.009));
      else {   // mão aberta: contorno próprio, que some no pé dos dedos e do polegar (ow): o vão não risca
        const im = new THREE.MeshBasicMaterial({ color: 0x14141c, side: THREE.BackSide });
        im.onBeforeCompile = sh => { sh.vertexShader = 'attribute float ow;' + String.fromCharCode(10) + sh.vertexShader.replace('#include <begin_vertex>', '#include <begin_vertex>' + String.fromCharCode(10) + 'transformed += normal * 0.008 * ow;'); };
        im.customProgramCacheKey = () => 'vivHandInk'; fist.add(new THREE.Mesh(fg, im)); g.add(fist); }
      const c1 = new THREE.Mesh(new THREE.CylinderGeometry(D(30), D(30), D(10), 28), flat(C.purple)); c1.position.copy(L(0, -34, 0)); g.add(ol(c1, 0.006));
      const c2 = new THREE.Mesh(new THREE.CylinderGeometry(D(28), D(28), D(9), 28), flat(C.white)); c2.position.copy(L(0, -43, 0)); g.add(ol(c2, 0.005));
      const c3 = new THREE.Mesh(new THREE.CylinderGeometry(D(26), D(24), D(8), 28), flat(C.purple)); c3.position.copy(L(0, -51, 0)); g.add(ol(c3, 0.005));
      // fiapo de fantasma saindo do punho, afinando e enrolando para trás
      // (anima: ondula e chicoteia de leve, cada mão no seu tempo — a malha é refeita a cada quadro)
      const wispPts = t => [[0, -50, 0], [4, -70, -4], [12, -88, -14], [6, -102, -28], [-4, -106, -38]].map(([x, y, z], k) => {
        const w = k / 4, a = 7 * w * Math.sin(t * 2.6 - k * 1.1 + sx), b = 5 * w * Math.cos(t * 2.1 - k * 0.9 + sx * 2);
        return LP(sx * x + a, y + 2 * w * Math.sin(t * 1.7 + k), z + b); });
      const wispR = u => 21 * (1 - u) ** 0.9 + 1.2;
      const wisp = new THREE.Mesh(tubeVar(wispPts(0), wispR, 16),
        pattern(flat(C.hand), `diffuseColor.rgb = mix(diffuseColor.rgb, uDeep, smoothstep(${(FOOT + 50).toFixed(1)}, ${(FOOT + 106).toFixed(1)}, vPx.y));`, 'vivWisp', { uDeep: col(C.tail) }));
      g.add(ol(wisp, 0.006));
      wisps.push(t => { const ng = tubeVar(wispPts(t), wispR, 16); wisp.geometry.attributes.position.array.set(ng.attributes.position.array); wisp.geometry.attributes.position.needsUpdate = true; wisp.geometry.computeVertexNormals(); ng.dispose(); });
      if (holdBaton) {   // batuta de maestro presa no punho: o cabo de madeira fica dentro da mão e só um pedacinho dele sai
        // em cima, com o anel dourado; dali a vareta branca comprida e fina sobe até a ponta
        const bat = new THREE.Group(), ax = new THREE.Vector3(sx * 0.5, 0.8, 0.32).normalize(); bat.position.copy(L(0, 6, 16)); bat.quaternion.setFromUnitVectors(new THREE.Vector3(0, 1, 0), ax); g.add(bat);
        const prof = [[0, -28], [8, -26], [12, -18], [12, -2], [9, 14], [7, 30], [6.5, 47]].map(([r, y]) => new THREE.Vector2(D(r), D(y)));
        const handle = new THREE.Mesh(new THREE.LatheGeometry(prof, 24), flat('#d8a868')); bat.add(ol(handle, 0.005));
        const ferrule = new THREE.Mesh(new THREE.CylinderGeometry(D(5.2), D(6.4), D(8), 18), flat(C.gold)); ferrule.position.y = D(50); bat.add(ol(ferrule, 0.004));
        const stick = new THREE.Mesh(new THREE.CylinderGeometry(D(1.4), D(4.2), D(140), 12), flat(C.white)); stick.position.y = D(123); bat.add(ol(stick, 0.004));
      }
      return g;
    }
    P.gloves = { [-1]: ghostHand(-1, false), [1]: ghostHand(1, true) }; root.add(P.gloves[-1], P.gloves[1]);
    const hands = { [-1]: new THREE.Object3D(), [1]: new THREE.Object3D() }; root.add(hands[-1], hands[1]);
    const UPV = new THREE.Vector3(0, 1, 0), FWD = new THREE.Vector3(0, 0, 1);

    const setFace = f => { headU.tFace.value.dispose(); headU.tFace.value = faceTex(f); };
    const hp = new THREE.Vector3(), hPrev = new THREE.Vector3(), hVel = new THREE.Vector3(), hv = new THREE.Vector3(), ha = new THREE.Vector3(), lag = new THREE.Vector3(), lagV = new THREE.Vector3(), hq = new THREE.Quaternion();
    let hInit = false;
    const kp = new THREE.Vector3(), kPrev = new THREE.Vector3(), kVel = new THREE.Vector3(), kv = new THREE.Vector3(), ka = new THREE.Vector3(), kLag = new THREE.Vector3(), kLagV = new THREE.Vector3(), kq = new THREE.Quaternion();
    let kInit = false;
    // ================= especiais (os do jogo, skills.cpp e skill_poses.cpp, com os efeitos do laboratório, vfx/lab_vivaldi.cpp).
    // A batuta fica na mão P.gloves[1]
    const easeO = k => 1 - (1 - k) ** 3, win = (a, b, x) => Math.max(0, Math.min(1, (x - a) / (b - a))), DIST = 1.75, CHEST = Y(545);
    const GHOST = '#5eead4', GDEEP = '#0f766e', PURP = '#a855f7', PDEEP = '#581c87', ICE = '#bae6fd', IDEEP = '#0284c7', PINK = '#f9a8d4', GOLD = '#facc15', LEAF = '#ea580c', GREEN = '#4ade80';
    const fxRoot = new THREE.Group(); root.add(fxRoot);
    const glow = (col, o = 1, add = false) => new THREE.MeshBasicMaterial({ color: col, transparent: true, opacity: o, depthWrite: false, side: THREE.DoubleSide, blending: add ? THREE.AdditiveBlending : THREE.NormalBlending });
    const queue = []; let now = 0; const after = (dt, fn) => queue.push({ at: now + dt, fn });
    const FX = { sign: -99, stacc: -99, winter: -99, ice: -99, seasons: -99, jab: -99, req: -99, leaves: -99, spring: -99, season: -1 };
    // figurinhas de frente para a câmera: notas, pétalas, folhas, flocos e brilhos
    const texCache = {}; const tex = (key, draw) => texCache[key] || (texCache[key] = (() => { const c = document.createElement('canvas'); c.width = c.height = 128; draw(c.getContext('2d')); const t = new THREE.CanvasTexture(c); t.colorSpace = THREE.SRGBColorSpace; return t; })());
    const noteTex = (n, col) => tex('n' + n + col, g => { g.font = '700 104px serif'; g.textAlign = 'center'; g.textBaseline = 'middle'; g.lineJoin = 'round'; g.lineWidth = 12; g.strokeStyle = '#0b0217'; g.strokeText(n, 64, 66); g.fillStyle = col; g.fillText(n, 64, 66); });
    const leafTex = (col, edge) => tex('l' + col, g => { g.fillStyle = col; g.strokeStyle = edge; g.lineWidth = 6; g.beginPath(); g.moveTo(14, 64); g.quadraticCurveTo(64, 10, 114, 64); g.quadraticCurveTo(64, 118, 14, 64); g.fill(); g.stroke(); g.beginPath(); g.moveTo(14, 64); g.lineTo(114, 64); g.stroke(); });
    const flakeTex = () => tex('flake', g => { g.strokeStyle = '#ffffff'; g.lineWidth = 9; g.lineCap = 'round'; for (let i = 0; i < 3; i++) { g.save(); g.translate(64, 64); g.rotate(i * Math.PI / 3); g.beginPath(); g.moveTo(0, -50); g.lineTo(0, 50); g.moveTo(-14, -34); g.lineTo(0, -20); g.lineTo(14, -34); g.moveTo(-14, 34); g.lineTo(0, 20); g.lineTo(14, 34); g.stroke(); g.restore(); } });
    const dotTex = col => tex('d' + col, g => { const gr = g.createRadialGradient(64, 64, 0, 64, 64, 60); gr.addColorStop(0, '#ffffff'); gr.addColorStop(0.3, col); gr.addColorStop(1, 'rgba(0,0,0,0)'); g.fillStyle = gr; g.fillRect(0, 0, 128, 128); });
    const quadG = new THREE.PlaneGeometry(1, 1), quads = Array.from({ length: 170 }, () => { const m = new THREE.Mesh(quadG, new THREE.MeshBasicMaterial({ transparent: true, depthWrite: false, side: THREE.DoubleSide })); m.visible = false; fxRoot.add(m); return { m, life: 0, v: new THREE.Vector3() }; });
    let qI = 0; const quad = (map, p, v, size, life, g = 0, spin = 0) => { const q = quads[qI++ % quads.length]; q.m.material.map = map; q.m.material.needsUpdate = true; q.m.position.copy(p); q.m.scale.setScalar(size); q.v.copy(v); q.size = size; q.life = q.life0 = life; q.g = g; q.spin = spin; q.a = Math.random() * 6.28; q.m.visible = true; };
    const NOTES = ['♪', '♫', '♩', '♬'], note = (p, v, size, col, life = 1) => quad(noteTex(NOTES[Math.floor(Math.random() * 4)], col), p, v, size, life);
    const sparkle = (p, n, spd, size, cols, life = 0.7) => { for (let i = 0; i < n; i++) { const a = Math.random() * 6.283, sp = spd * (0.4 + Math.random()); quad(dotTex(cols[i % cols.length]), p, new THREE.Vector3(Math.cos(a) * sp, Math.sin(a) * sp, (Math.random() - 0.5) * sp), size * (0.7 + Math.random() * 0.6), life, 0.6); } };
    const ringG = new THREE.RingGeometry(0.84, 1, 40), rings = Array.from({ length: 22 }, () => { const m = new THREE.Mesh(ringG, glow('#fff')); m.visible = false; fxRoot.add(m); return { m, life: 0 }; });
    let ringI = 0; const ring = (p, R, col, life, flatR = false, vz = 0) => { const q = rings[ringI++ % rings.length]; q.m.position.copy(p); q.R = R; q.vz = vz; q.life = q.life0 = life; q.m.material.color.set(col); q.flat = flatR; q.m.rotation.set(flatR ? -Math.PI / 2 : 0, 0, 0); q.m.scale.setScalar(0.02); q.m.visible = true; };
    // pauta da Cadência: cinco linhas de luz do peito até o rival
    const staff = Array.from({ length: 5 }, (_, i) => { const g = new THREE.CylinderGeometry(0.006, 0.006, 1, 6); g.rotateX(Math.PI / 2); g.translate(0, 0, 0.5); const m = new THREE.Mesh(g, glow(i % 2 ? GHOST : '#e9d5ff', 0.9)); m.position.set(0, CHEST + 0.09 + (i - 2) * 0.05, 0.3); m.visible = false; fxRoot.add(m); return m; });
    // cristais do Inverno Allegro em volta do rival
    const crystals = Array.from({ length: 9 }, (_, i) => { const a = i * 6.283 / 9 + (i % 2 ? 0.15 : -0.12), rr = 0.18 + (i * 7 % 5) * 0.03, H = 0.65 + (i * 5 % 7) * 0.13, g = new THREE.ConeGeometry(0.11 + (i % 3) * 0.02, H, 6); g.translate(0, H / 2, 0); const mt = flat(i % 2 ? '#7dd3fc' : '#e0f2fe'); mt.flatShading = true; const o = withOutline(new THREE.Mesh(g, mt), 0.006); o.position.set(Math.cos(a) * rr, 0, DIST + Math.sin(a) * rr); o.rotation.set(Math.sin(a) * 0.35, 0, -Math.cos(a) * 0.35); o.visible = false; fxRoot.add(o); return o; });
    // estocada: feixe de ectoplasma em três camadas
    const beam = new THREE.Group(); beam.position.set(0, CHEST + 0.1, 0.3); beam.visible = false; fxRoot.add(beam); [[0.055, GDEEP, 0.5], [0.032, GHOST, 0.8], [0.012, '#ffffff', 1]].forEach(([rr, col, o]) => { const g = new THREE.CylinderGeometry(rr * 0.55, rr, 1, 12); g.rotateX(Math.PI / 2); g.translate(0, 0, 0.5); beam.add(new THREE.Mesh(g, glow(col, o))); });
    // sonata: canteiro de flores que brota em roda no chão
    const flowerOf = (pet, core, R) => { const g = new THREE.Group(), pm = flat(pet), cm = flat(core); for (let i = 0; i < 5; i++) { const a = i / 5 * 6.283, m = new THREE.Mesh(new THREE.SphereGeometry(1, 10, 8), pm); m.scale.set(R * 0.52, R * 0.72, R * 0.2); m.position.set(Math.sin(a) * R * 0.62, Math.cos(a) * R * 0.62, 0); m.rotation.z = -a; g.add(m); } const c = new THREE.Mesh(new THREE.SphereGeometry(R * 0.36, 10, 8), cm); c.position.z = R * 0.1; g.add(c); return g; };
    const bed = Array.from({ length: 12 }, (_, i) => { const a = i / 12 * 6.283, f = flowerOf(i % 2 ? '#fbcfe8' : '#f9a8d4', GOLD, 0.075); f.position.set(Math.cos(a) * 0.72, 0.03, Math.sin(a) * 0.62); f.rotation.x = -Math.PI / 2; f.visible = false; fxRoot.add(f); return f; });
    // enfeites de cada estação, assentados na peruca por raio: flores, sóis, folhas, flocos
    const hairDecor = [0, 1, 2, 3].map(() => { const g = new THREE.Group(); g.visible = false; head.add(g); return g; });
    { root.updateMatrixWorld(true); P.hair.geometry.computeBoundingBox(); const ctr = new THREE.Vector3(); P.hair.geometry.boundingBox.getCenter(ctr); const ray = new THREE.Raycaster();
      const dirs = [[-0.5, 0.5, 0.7], [0.5, 0.55, 0.65], [0, 0.85, 0.5], [-0.85, 0.25, 0.45], [0.85, 0.3, 0.42], [-0.72, -0.25, 0.6], [0.72, -0.2, 0.62], [-0.45, 0.55, -0.7], [0.45, 0.5, -0.72], [0, 0.35, -0.93]];
      const sun = () => { const g = new THREE.Group(), R = 0.05; g.add(new THREE.Mesh(new THREE.CylinderGeometry(R, R, 0.012, 16).rotateX(Math.PI / 2), flat('#fde047'))); for (let i = 0; i < 12; i++) { const a = i / 12 * 6.283, c = new THREE.Mesh(new THREE.ConeGeometry(0.014, 0.036, 4), flat('#f59e0b')); c.position.set(Math.sin(a) * 0.066, Math.cos(a) * 0.066, 0); c.rotation.z = -a; g.add(c); } return g; };
      const LEAVES = ['#ea580c', '#b45309', '#dc2626', '#f59e0b', '#c2410c'];
      const leaf = i => { const m = new THREE.Mesh(new THREE.PlaneGeometry(0.14, 0.14), new THREE.MeshBasicMaterial({ map: leafTex(LEAVES[i % 5], '#3b1a08'), transparent: true, alphaTest: 0.3, side: THREE.DoubleSide })); m.rotation.z = i * 1.3; return m; };
      const flake = i => { const m = new THREE.Mesh(new THREE.PlaneGeometry(0.13, 0.13), new THREE.MeshBasicMaterial({ map: flakeTex(), transparent: true, alphaTest: 0.3, side: THREE.DoubleSide })); m.rotation.z = i * 0.7; return m; };
      dirs.forEach((dv, i) => { const d = new THREE.Vector3(...dv).normalize(), org = ctr.clone().addScaledVector(d, 3); ray.set(org, d.clone().negate()); const h = ray.intersectObject(P.hair)[0]; if (!h) return; const n = h.point.clone(), out = n.clone().add(d);
        [flowerOf('#fbcfe8', GOLD, 0.055), sun(), leaf(i), flake(i)].forEach((o, k) => { const w = new THREE.Group(); w.add(o); w.position.copy(head.worldToLocal(n.clone().addScaledVector(d, 0.012))); w.lookAt(out); hairDecor[k].add(w); }); }); }
    const SEASONS = [['#fff0f7', '#f9a8d4', '#db2777'], ['#fffbe6', '#fde047', '#d97706'], ['#fff1e6', '#fb923c', '#9a3412'], ['#ffffff', '#dbeafe', '#3b82f6']];
    let seasonNow = -1; const setSeason = k => { if (k === seasonNow) return; seasonNow = k; seasonU.uSeason.value = k < 0 ? 0 : 1; hairDecor.forEach((g, i) => { g.visible = i === k; }); if (k >= 0) { seasonU.uS0.value.set(SEASONS[k][0]); seasonU.uS1.value.set(SEASONS[k][1]); seasonU.uS2.value.set(SEASONS[k][2]); } };
    document.querySelectorAll('[data-season]').forEach(b => b.onclick = () => { FX.season = +b.dataset.season; FX.seasons = -99; document.querySelectorAll('[data-season]').forEach(q => { q.style.outline = q === b ? '2px solid #fc6' : ''; }); });
    const skills = {
      // assinatura (laboratório): rege em quatro tempos desenhando o compasso com a batuta, corpo e peruca balançam e as notas voam a cada tempo
      signature: { dur: 2.6, start: () => { FX.sign = now; for (let i = 0; i < 4; i++) after(0.05 + i * 0.65, () => { const cols = ['#74c69d', '#ffffff', '#9d4edd', '#ffb703']; for (let k = 0; k < 2; k++) { const tip = P.gloves[1].position.clone(); tip.y += 0.3; quad(noteTex(NOTES[(i + k) % 4], cols[(i + k) % 4]), tip, new THREE.Vector3(-0.55 + k * 0.4, 0.28 - Math.random() * 0.9, 0.9), 0.17 + i * 0.03, 1.5, 0.05); } }); },
        pose: k => { const p = k / 2.6; return { twist: 0.1 * Math.sin(p * Math.PI * 4), tilt: 0.08 * Math.sin(p * Math.PI * 8), lean: -0.04 }; } },
      // cadência: cinco estocadas alternadas, cada uma mais longa (crescendo), na pauta de luz
      staccato: { dur: 1.5, start: () => { FX.stacc = now; for (let i = 0; i < 5; i++) after((0.2 + i * 0.15) * 1.5, () => { const g = 1 + i * 0.25; for (let n = 0; n < 3 + i; n++) note(new THREE.Vector3((Math.random() - 0.5) * 0.25, CHEST + 0.1 + (Math.random() - 0.3) * 0.2, 0.3), new THREE.Vector3((Math.random() - 0.5) * 0.4, (Math.random() - 0.4) * 0.5, 2.6 + Math.random() * 0.9), 0.13 * g, i % 2 ? PURP : GHOST, 0.6); sparkle(new THREE.Vector3(0, CHEST + 0.1, DIST - 0.15), 6 + i * 2, 1 + i * 0.2, 0.05, [GHOST, PURP]); }); },
        pose: (k, pose) => { const p = k / 1.5, beat = Math.min(4, Math.floor(p * 5)), local = (p * 5) % 1, sd = beat % 2 ? -1 : 1, q = Math.sin(local * Math.PI), reach = 0.42 + beat * 0.09; pose[sd] = [reach * q, 0.08 * q, 0.8 * q, 0, 0]; pose[-sd] = [0, 0, 0.25, 0, 0]; return { twist: -sd * 0.16 * q, lean: 0.1 * q }; } },
      // inverno: uppercut de gelo; nasce o bloco de cristais no rival, que estilhaça depois
      winter: { dur: 1.1, start: () => { FX.winter = now; after(0.5, () => { FX.ice = now; const b = new THREE.Vector3(0, 0.9, DIST); sparkle(b, 18, 1.3, 0.07, [ICE, IDEEP, '#ffffff']); ring(new THREE.Vector3(0, 0.015, DIST), 1.05, ICE, 0.7, true); }); },
        pose: (k, pose) => { const p = k / 1.1, prep = ss(0, 0.3, p), up = Math.sin(win(0.3, 1, p) * Math.PI); pose[1] = [0.42 * up, -0.2 * prep * (1 - up) + 0.5 * up, 0.75 * up, 0, 0]; pose[-1] = [0, 0, 0.1, 0, 0]; return { lean: -0.24 * up + 0.06 * prep * (1 - up), drop: 0.05 * prep * (1 - up), twist: -0.2 * up }; } },
      // quatro estações: rege em círculos e o tempo muda a cada 1,5 s por 6 s (peruca e o que gira em volta)
      seasons: { dur: 1.2, start: () => { FX.seasons = now; FX.season = -1; document.querySelectorAll('[data-season]').forEach(q => { q.style.outline = ''; }); ring(new THREE.Vector3(0, 0.015, 0), 1.2, GHOST, 0.7, true); },
        pose: (k, pose) => { const a = (k / 1.2) * Math.PI * 4, e = ss(0, 0.2, k) * (1 - ss(0.95, 1.2, k)); pose[1] = [0.08 * e, (0.2 + 0.2 * Math.sin(a)) * e, (-0.1 - 0.38 * Math.cos(a)) * e, 0, 0]; pose[-1] = [0, 0.14 * Math.abs(Math.sin(a * 0.5)) * e, 0, 0, 0]; return { twist: 0.08 * Math.sin(a) * e }; } },
      jab: { dur: 0.85, start: () => { FX.jab = now; after(0.36, () => { const b = new THREE.Vector3(0, CHEST + 0.1, DIST - 0.15); sparkle(b, 22, 1.5, 0.07, [GHOST, GDEEP, '#ffffff']); ring(b, 0.5, GHOST, 0.35); }); },
        pose: (k, pose) => { const q = Math.sin(win(0.2, 1, k / 0.85) * Math.PI); pose[1] = [0.7 * q, 0.1 * q, 0.95 * q, 0, 0]; pose[-1] = [-0.04 * q, 0, 0, 0, 0]; return { lean: 0.2 * q, twist: -0.2 * q }; } },
      // réquiem: rege de lado a lado soltando anéis roxos que vão até o rival; folhas caem nele por 4 s
      requiem: { dur: 1.2, start: () => { FX.req = now; for (let i = 0; i < 7; i++) after(i * 0.1, () => { ring(new THREE.Vector3(0, CHEST + 0.1, 0.3), 0.3, PURP, 0.55, false, 2.7); note(new THREE.Vector3((Math.random() - 0.5) * 0.4, CHEST + 0.2, 0.2), new THREE.Vector3((Math.random() - 0.5) * 0.4, 0.6, 0.9), 0.13, PURP, 0.9); }); after(0.48, () => { sparkle(new THREE.Vector3(0, CHEST + 0.1, DIST), 14, 1.2, 0.07, ['#e9d5ff', PURP]); FX.leaves = now; }); },
        pose: (k, pose) => { const p = k / 1.2, a = Math.sin(p * Math.PI * 2), e = ss(0, 0.15, p) * (1 - ss(0.82, 1, p)), v = [0.12 * e, 0.24 * e, 0.3 * a * e, 0, 0]; pose[1] = [v[0], v[1], (0.15 - 0.38 * a) * e, 0, 0]; pose[-1] = [v[0], v[1], (0.15 + 0.38 * a) * e, 0, 0]; return { twist: 0.12 * a * e, lean: 0.04 * e }; } },
      // sonata: abre os braços e flutua; acorde curativo com notas subindo do chão, pétalas e o canteiro florindo
      spring: { dur: 1.4, start: () => { FX.spring = now; after(0.7, () => { sparkle(new THREE.Vector3(0, CHEST + 0.15, 0), 14, 1.2, 0.07, [GREEN, PINK]); ring(new THREE.Vector3(0, 0.015, 0), 1.25, GREEN, 0.8, true); }); },
        pose: (k, pose) => { const e = ss(0, 0.3, k) * (1 - ss(1.05, 1.4, k)), v = [-0.02 * e, 0.32 * e, -0.3 * e, 0, 0]; pose[1] = v; pose[-1] = [...v]; return { lean: -0.08 * e }; } },
    };
    const fxFrame = c => {
      now = c.t; const t = now, dt = c.dt, cam = window.VIN?.camera;
      for (let i = queue.length - 1; i >= 0; i--) if (now >= queue[i].at) { queue[i].fn(); queue.splice(i, 1); }
      for (const q of quads) { if (q.life <= 0) { q.m.visible = false; continue; } q.life -= dt; q.v.y -= q.g * dt; q.m.position.addScaledVector(q.v, dt); const u = q.life / q.life0; q.m.scale.setScalar(q.size * Math.min(1, u * 3)); q.m.material.opacity = Math.min(1, u * 2.5); if (cam) { q.m.quaternion.copy(cam.quaternion); if (q.spin) q.m.rotateZ(q.a + t * q.spin); } if (q.m.position.y < 0.01) q.life = 0; }
      for (const q of rings) { if (q.life <= 0) { q.m.visible = false; continue; } q.life -= dt; const u = 1 - q.life / q.life0, e = q.R * easeO(u) + 0.03; q.m.position.z += q.vz * dt; q.m.scale.set(e, e, 1); q.m.material.opacity = Math.max(0, 1 - u); if (cam && !q.flat) q.m.quaternion.copy(cam.quaternion); }
      // assinatura: a batuta percorre o compasso de quatro tempos e deixa um rastro
      { const p = (t - FX.sign) / 2.6; if (c.skill === 'signature' && p >= 0 && p < 1) { const PATH = [[0.58, 1.5], [0.2, 1.0], [0.62, 1.08], [0.38, 1.58]], beat = Math.floor(p * 4) % 4, q = (p * 4) % 1, a = PATH[beat], b = PATH[(beat + 1) % 4], u = q * q * (3 - 2 * q), w = ss(0, 0.08, p) * (1 - ss(0.92, 1, p)), G = P.gloves[1]; G.position.lerp(new THREE.Vector3(a[0] + (b[0] - a[0]) * u, a[1] + (b[1] - a[1]) * u, 0.48), w); G.rotateZ(w * (0.3 * Math.sin(p * 12) - 0.2)); G.rotateX(w * 0.5); const tip = G.position.clone().add(new THREE.Vector3(-0.08, 0.34, 0.14)); quad(dotTex(GHOST), tip, new THREE.Vector3(), 0.07, 0.3); const L = P.gloves[-1]; L.position.y += 0.1 * Math.abs(Math.sin(p * Math.PI * 4)) * w; } }
      // pauta
      { const u = (t - FX.stacc) / 1.5, a = u > 0 && u < 0.92 ? ss(0, 0.1, u) * (1 - ss(0.82, 0.92, u)) : 0; for (const m of staff) { m.visible = a > 0.01; if (m.visible) { m.scale.z = (DIST - 0.3) * a; m.material.opacity = 0.9 * a; } } }
      // nevasca subindo pela mão e cristais
      { const u = (t - FX.winter) / 1.1; if (u > 0 && u < 0.5) { const g = P.gloves[1].position, a = u * 40; quad(flakeTex(), g.clone().add(new THREE.Vector3(Math.cos(a) * 0.14, (Math.random() - 0.5) * 0.15, Math.sin(a) * 0.14)), new THREE.Vector3(0, 0.8, 0), 0.08, 0.5, 0, 3); }
        const f = (t - FX.ice) * 60, on = f >= 0 && f < 70, gr = on ? Math.min(1, f / 8) : 0; for (const o of crystals) { o.visible = on; if (on) o.scale.set(0.7 + 0.3 * gr, Math.max(0.01, gr), 0.7 + 0.3 * gr); }
        if (f >= 70 && !FX.iceDone) { FX.iceDone = true; const b = new THREE.Vector3(0, 0.7, DIST); for (let i = 0; i < 26; i++) { const a = Math.random() * 6.283, sp = 0.8 + Math.random() * 1.6; quad(flakeTex(), b.clone().add(new THREE.Vector3((Math.random() - 0.5) * 0.5, (Math.random() - 0.5) * 0.7, 0)), new THREE.Vector3(Math.cos(a) * sp, 1 + Math.random() * 2, Math.sin(a) * sp), 0.09, 0.9, 6, 5); } } if (f < 60) FX.iceDone = false; }
      // estações: a peruca muda a cada 1,5 s; as partículas da estação giram em volta
      { const se = t - FX.seasons, onS = se > 0 && se < 6.6, k = onS ? Math.floor((se + 0.5) / 1.5) % 4 : FX.season; setSeason(k);
        if (onS && Math.random() < 0.6) { const a = Math.random() * 6.283, rad = 0.55 + Math.random() * 0.25, p = new THREE.Vector3(Math.cos(a) * rad, 0.15 + Math.random() * 1.45, Math.sin(a) * rad), v = new THREE.Vector3(-Math.sin(a) * 1.3, 0, Math.cos(a) * 1.3); if (k === 0) { v.y = (Math.random() - 0.5) * 0.3; quad(leafTex('#f9a8d4', '#be185d'), p, v, 0.075, 1.2, 0, 3); } else if (k === 1) { v.y = 0.5 + Math.random() * 0.5; quad(dotTex(GOLD), p, v, 0.07, 1.0); } else if (k === 2) { v.y = -0.5; quad(leafTex(LEAF, '#7c2d12'), p, v, 0.1, 1.3, 0, 2); } else { v.y = -0.4; quad(flakeTex(), p, v, 0.075, 1.2, 0, 3); } } }
      // feixe
      { const u = (t - FX.jab) / 0.85, a = u > 0.2 && u < 0.7 ? ss(0.2, 0.3, u) * (1 - ss(0.6, 0.7, u)) : 0; beam.visible = a > 0.01; if (beam.visible) { beam.scale.set(0.7 + 0.5 * a + 0.1 * Math.sin(t * 40), 0.7 + 0.5 * a + 0.1 * Math.sin(t * 40), Math.max(0.01, (DIST - 0.45) * a)); } }
      // folhas caindo no rival
      { const u = t - FX.leaves; if (u > 0 && u < 4 && Math.random() < 0.3) quad(leafTex(Math.random() < 0.5 ? LEAF : '#b45309', '#7c2d12'), new THREE.Vector3((Math.random() - 0.5) * 1, 1.75, DIST + (Math.random() - 0.5) * 0.5), new THREE.Vector3((Math.random() - 0.5) * 0.4, -0.75, 0), 0.11, 2.2, 0, 2); }
      // sonata
      { const u = (t - FX.spring) / 1.4, a = u > 0 && u < 1.6 ? ss(0, 0.3, u) * (1 - ss(1.25, 1.6, u)) : 0; bed.forEach((f, i) => { const g = Math.max(0, Math.min(1, a * 1.5 - i * 0.03)); f.visible = g > 0.01; if (f.visible) f.scale.setScalar(g); });
        if (u > 0 && u < 0.95 && Math.random() < 0.55) { note(new THREE.Vector3((Math.random() - 0.5) * 1.2, 0.08, (Math.random() - 0.5) * 0.8), new THREE.Vector3(0, 1 + Math.random() * 0.8, 0), 0.13, Math.random() < 0.5 ? GREEN : PINK, 0.9); quad(leafTex('#f9a8d4', '#be185d'), new THREE.Vector3((Math.random() - 0.5) * 1.4, 1.1 + Math.random() * 0.8, (Math.random() - 0.5) * 0.8), new THREE.Vector3((Math.random() - 0.5) * 0.3, -0.5, 0), 0.07, 1.3, 0, 3); } }
    };
    const onFrame = c => {
      // fantasma: a cauda balança e gira devagar (a espiral anda)
      const t = c.t, dt = Math.min(0.05, Math.max(c.dt || 1 / 60, 1e-3));
      for (const f of wisps) f(t);
      P.tail.rotation.z = 0.09 * Math.sin(t * 1.7); P.tail.rotation.x = 0.08 * Math.sin(t * 1.3 + 1) - 0.04; P.tail.rotation.y = 0.18 * Math.sin(t * 0.9);
      // rabicho de pano: fica para trás quando o quadril mexe (mola) e tremula de leve; preso no cinto, solto na ponta
      hips.updateWorldMatrix(true, false); hips.getWorldPosition(kp); if (!kInit) { kPrev.copy(kp); kInit = true; }
      kv.copy(kp).sub(kPrev).divideScalar(dt); ka.copy(kv).sub(kVel).divideScalar(dt); kVel.copy(kv); kPrev.copy(kp);
      kLagV.addScaledVector(kLag, -70 * dt).addScaledVector(kLagV, -6 * dt).addScaledVector(ka, -1.2 * dt); kLag.addScaledVector(kLagV, dt); if (kLag.length() > 0.09) kLag.setLength(0.09);
      hips.getWorldQuaternion(kq).invert(); const Lk = kLag.clone().applyQuaternion(kq);
      for (const f of flaps) { const a = f.p.array, B = f.base;
        for (let i = 0; i < a.length; i += 3) { const yy = FOOT - B[i + 1] * S_PX, w = Math.max(0, (yy - 606) / 100), w2 = w * w;
          const fl = 0.004 * w * Math.sin(t * 4.1 + yy * 0.08 + f.s), out = Math.max(0, -Lk.z) * 0.6 * w2;   // a aba abre para fora quando o corpo vai para a frente
          a[i] = B[i] - Lk.x * w2; a[i + 1] = B[i + 1] + Lk.y * w2 + Math.abs(Lk.z) * 0.3 * w2; a[i + 2] = B[i + 2] - Lk.z * w2 + out + fl; }   // (aba virada: x e z trocam de sinal)
        f.p.needsUpdate = true; }
      // saia: a barra fica para trás quando o quadril mexe (o mesmo atraso do rabicho), preso no cinto, solto embaixo;
      // ondas de pano correndo em volta da barra (só para dentro: não atravessa as abas do rabicho)
      { const g = P.skirt.g, a = g.attributes.position.array, B = P.skirt.base;
        for (let i = 0; i < a.length; i += 3) { const yy = FOOT - B[i + 1] * S_PX, w = Math.min(1, Math.max(0, (yy - 600) / 74)), w2 = w * w; if (!w) continue;
          const x = B[i], z = B[i + 2], th = Math.atan2(z, x), wave = 0.008 * w * (0.5 * Math.sin(t * 2.2 + th * 5 + yy * 0.02) - 0.5);
          a[i] = x * (1 + wave) + Lk.x * w2; a[i + 1] = B[i + 1] + Lk.y * w2 + Math.abs(Lk.z) * 0.3 * w2; a[i + 2] = z * (1 + wave) + Lk.z * w2; }
        g.attributes.position.needsUpdate = true; g.computeVertexNormals(); }
      // cabelo: mola presa na cabeça (fica para trás quando ela mexe e volta balançando), como o da Valéria
      head.updateWorldMatrix(true, false); head.getWorldPosition(hp); if (!hInit) { hPrev.copy(hp); hInit = true; }
      hv.copy(hp).sub(hPrev).divideScalar(dt); ha.copy(hv).sub(hVel).divideScalar(dt); hVel.copy(hv); hPrev.copy(hp);
      lagV.addScaledVector(lag, -110 * dt).addScaledVector(lagV, -8 * dt).addScaledVector(ha, -0.9 * dt); lag.addScaledVector(lagV, dt); if (lag.length() > 0.07) lag.setLength(0.07);
      head.getWorldQuaternion(hq).invert(); hairLag.value.copy(lag).applyQuaternion(hq); hairT.value = t;
      for (const sx of [-1, 1]) {
        const G = P.gloves[sx], k = Math.min(1, Math.max(0, c.pose[sx][0] / 0.3));
        G.position.copy(hands[sx].position);
        const yq = new THREE.Quaternion().setFromAxisAngle(UPV, upper.rotation.y * 0.8);
        const dir = new THREE.Vector3(0, 1, 0.06).lerp(new THREE.Vector3(0, 0.25, 1), k).normalize().applyQuaternion(yq);
        const zRef = FWD.clone().lerp(UPV.clone().negate(), k).applyQuaternion(yq), zA = zRef.addScaledVector(dir, -zRef.dot(dir)).normalize(), xA = new THREE.Vector3().crossVectors(dir, zA);
        G.quaternion.setFromRotationMatrix(new THREE.Matrix4().makeBasis(xA, dir, zA));
      }
      fxFrame(c);
    };
    P.cloth = [{ geos: [P.skirt.g], top: Y(600), bot: Y(674), pow: 2, gain: 0.9 }, { geos: flaps.map(f => f.g), top: Y(606), bot: Y(706), pow: 2, gain: 0.9 }];   // tools3d/export
    return { P, upper, WAIST, neck, NECK, hips, HIPS, hands, hipSway: 0.3, hand: { x: 0.6, y: Y(470), z: 0.1, yaw: -0.05 }, placeFoot: () => {}, setFace, skills, onFrame };
  },
};
