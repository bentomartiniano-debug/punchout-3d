// Corpo-base do vinil: o corpo da Valéria (valeria.js) vira a forma de todos os lutadores. Cada um passa as próprias
// medidas (em px do seu quadro) e veste por cima o que é só dele (cabelo, roupa, chapéu...). As formas são as dela:
//  - rosto em ovo (testa larga, afina até um queixo redondo) que vira pescoço e ombros numa superfície só, com o
//    maxilar pintado (pescoço um tom mais escuro, risco preto no queixo) e o desenho do rosto por cima;
//  - tronco em elipse; pernas saindo do quadril (pivô em cima, para o legPlacer); sapato com forma de pé (calcanhar
//    estreito, bico largo e redondo), sola, boca onde entra a perna e tira com fivela.
// Os números entre parênteses nos comentários são as medidas da Valéria (quadro de 480 px, S = 245).
import { THREE, ss, INK, INKS, flat, withOutline } from './core.js';
import { RoundedBoxGeometry } from 'three/addons/geometries/RoundedBoxGeometry.js';

// medidas da Valéria; cada personagem muda o que precisar (bodyOpts(V, {...}) mistura um nível de profundidade)
export const VALERIA = {
  cx: 240, skin: 0xa4502c, face: '#a85c38', neck: '#8f4a2c', pw: 2,
  head: { top: 136, bot: 210, a: 56, d: 0.62, cz: 34 },                 // ovo do rosto
  yoke: { y0: 196, y1: 222, a0: 27, a1: 68, d: 0.62, cz: -2, end: 232 }, // pescoço abrindo até os ombros
  torso: { y0: 222, y1: 292, a: 57, d: 0.72, cz: -4 },
  legs: { dx: 75, top: 330, bot: 426, r: 12, d: 1 },
  shoe: { k: 1, toe: 40, up: 0x2fd0c0, sole: 0x1c8a80, buckle: 0xf2c21a, foot: 440 },
};
export const bodyOpts = (base, o) => { const r = { ...base }; for (const k in o) r[k] = o[k] && typeof o[k] === 'object' && !Array.isArray(o[k]) && base[k] && typeof base[k] === 'object' ? { ...base[k], ...o[k] } : o[k]; return r; };

export function valeriaBody(U, o) {
  const { X, Y, D, S, FOOT, loft } = U, H = o.head, K = o.yoke, cx = o.cx;
  const span = K.y1 - K.y0;
  // ovo do rosto: largura a em cima, afina da metade para baixo até o queixo (fb)
  const faceA = y => { const t = (y - H.top) / (H.bot - H.top), lo = Math.max(0, (t - 0.45) / 0.55); return H.a * Math.sqrt(Math.max(0.02, 1 - Math.min(1, lo) ** 2 * 0.92)); };
  const yokeA = y => K.a0 + (K.a1 - K.a0) * Math.pow(ss(K.y0, K.y1, y), 1.6);
  const neckZ = y => K.cz + (H.cz - 8 * H.a / 56) * (1 - ss(K.y0 - 2 * span / 26, K.y1 + 2 * span / 26, y));
  // perfil inteiro da cabeça (a cúpula de cima que o loft fecha + o ovo) e a frente dela, para assentar peças no rosto
  const domeH = 0.9 * Math.min(H.a * H.d, H.a);
  const headA = y => { if (y < H.top) { const t = (H.top - y) / domeH; return t >= 1 ? 0 : H.a * Math.sqrt(1 - t * t); } return y <= K.y0 ? faceA(y) : Math.max(faceA(y), yokeA(y)); };
  const headZ = (x, y) => { const a = headA(y); if (a < 1) return H.cz; const c = Math.min(1, Math.abs(x - cx) / a); return H.cz + a * H.d * Math.sqrt(1 - c * c); };
  const out = { faceA, yokeA, neckZ, headA, headZ, headTop: H.top - domeH };
  // ponto exato da superfície do ovo (rosto, pescoço e ombros) na altura y e no ângulo th (0 = +x, π/2 = frente), em
  // px [x, z]; e o mesmo ponto afastado off px pela normal (para peças que vestem a cabeça sem atravessar)
  {
    const sp = v => Math.sign(v) * Math.pow(Math.abs(v), 2 / o.pw), b0 = K.y0 - 4 * span / 26, b1 = K.y0 + 16 * span / 26;
    out.surf = (y, th) => {
      const a = Math.max(0.01, headA(y)), x = cx + a * sp(Math.cos(th)), z0 = H.cz + a * H.d * sp(Math.sin(th));
      const k = y < H.top ? 0 : ss(b0, b1, y); if (!k) return [x, z0];
      const want = neckZ(y) + (z0 - H.cz) / (H.d * a) * K.d * yokeA(y); return [x, z0 + (want - z0) * k];
    };
    out.surfOff = (y, th, off) => {
      const e = 0.4, [x, z] = out.surf(y, th), [xa, za] = out.surf(y + e, th), [xb, zb] = out.surf(y, th + 0.01);
      const ty = [xa - x, e, za - z], tt = [xb - x, 0, zb - z];   // derivadas (y para baixo)
      let n = [ty[1] * tt[2] - ty[2] * tt[1], ty[2] * tt[0] - ty[0] * tt[2], ty[0] * tt[1] - ty[1] * tt[0]];
      const l = Math.hypot(...n) || 1; n = n.map(v => v / l); if (n[0] * (x - cx) + n[2] * (z - H.cz) < 0) n = n.map(v => -v);
      return [x + n[0] * off, y + n[1] * off, z + n[2] * off];
    };
  }

  // --- rosto + pescoço + ombros (uma superfície); uv no quadrado do rosto para a pintura
  const s = H.a / 56, box = [cx - 70 * s, H.top - 31 * s, cx + 70 * s, H.top + 109 * s];
  {
    const rows = [];
    for (let y = H.top; y <= K.end; y += 2 * s) { const a = Math.max(faceA(y), yokeA(y)); rows.push({ y, l: cx - a, r: cx + a }); }
    const b0 = K.y0 - 4 * span / 26, b1 = K.y0 + 16 * span / 26;   // onde o rosto vira pescoço (sem "bola" de queixo)
    const g = loft(rows, { depth: H.d, cz: H.cz, NA: 48, capTop: true, capBot: false, pw: o.pw,
      zFn: (x, y, z) => { const k = ss(b0, b1, y); if (!k) return z; const a = Math.max(faceA(y), yokeA(y)), want = neckZ(y) + (z - H.cz) / (H.d * a) * K.d * yokeA(y); return z + (want - z) * k; } });
    const uv = [], p = g.attributes.position;
    for (let i = 0; i < p.count; i++) uv.push((p.getX(i) * S + cx - box[0]) / (box[2] - box[0]), 1 - (FOOT - p.getY(i) * S - box[1]) / (box[3] - box[1]));
    g.setAttribute('uv', new THREE.Float32BufferAttribute(uv, 2));
    out.faceGeo = g; out.faceBox = box;
  }
  // pintura do rosto: pele, pescoço mais escuro abaixo do maxilar, risco do queixo; draw(g, at) desenha olhos, boca...
  // at(x, y, w, h, fn, cw, ch): fn desenha num canvas cw x ch que vai para o retângulo (x, y, w, h) em px do quadro
  const FR = 1024;
  out.faceCanvas = (draw = null) => {
    const c = document.createElement('canvas'); c.width = c.height = FR; const g = c.getContext('2d');
    const k = FR / (box[2] - box[0]), at = (x, y, w, h, fn, cw, ch) => { g.save(); g.translate((x - w / 2 - box[0]) * k, (y - h / 2 - box[1]) * k); g.scale(w * k / cw, h * k / ch); fn(g); g.restore(); };
    g.fillStyle = o.face; g.fillRect(0, 0, FR, FR);
    if (o.jaw !== false) {   // maxilar desenhado (quem tem o rosto largo/sem queixo, como o Marcov, desliga)
      const jaw = []; for (let y = H.top + (H.bot - H.top) * 0.486; y <= H.bot; y += 1) jaw.push([cx - faceA(y), y]);
      const jawR = jaw.slice().reverse().map(([x, y]) => [2 * cx - x, y]), P2 = ([x, y]) => [(x - box[0]) * k, (y - box[1]) * k];
      g.beginPath(); g.moveTo(...P2(jaw[0])); for (const p of [...jaw, ...jawR]) g.lineTo(...P2(p));
      g.lineTo(FR, FR); g.lineTo(0, FR); g.closePath(); g.fillStyle = o.neck; g.fill();
      g.beginPath(); g.moveTo(...P2(jaw[0])); for (const p of [...jaw, ...jawR]) g.lineTo(...P2(p));
      g.strokeStyle = INKS; g.lineWidth = 2.2 * s * k; g.lineCap = 'round'; g.lineJoin = 'round'; g.stroke();
    }
    if (draw) draw(g, at);
    return c;
  };
  out.faceTexture = draw => { const t = new THREE.CanvasTexture(out.faceCanvas(draw)); t.colorSpace = THREE.SRGBColorSpace; t.anisotropy = 8; return t; };
  out.faceMat = new THREE.MeshBasicMaterial({ map: out.faceTexture(o.drawFace || null) });   // pele chapada como no sprite
  // a pintura é projetada de frente: atrás (normal para trás) fica só a pele, e o tom do pescoço abaixo do começo dele
  out.faceMat.onBeforeCompile = sh => {
    Object.assign(sh.uniforms, { uFaceC: { value: new THREE.Color(o.face) }, uNeckC: { value: new THREE.Color(o.neck) }, uNeckY: { value: Y(K.y0 + 6 * span / 26) } });
    sh.vertexShader = 'varying float vNz, vOy;\n' + sh.vertexShader.replace('#include <begin_vertex>', '#include <begin_vertex>\nvNz = normal.z; vOy = position.y;');
    sh.fragmentShader = 'uniform vec3 uFaceC, uNeckC; uniform float uNeckY; varying float vNz, vOy;\n' + sh.fragmentShader.replace('#include <map_fragment>', `#include <map_fragment>
      diffuseColor.rgb = mix(diffuseColor.rgb, vOy > uNeckY ? uFaceC : uNeckC, smoothstep(0.02, -0.12, vNz));`);
  };
  out.faceMat.customProgramCacheKey = () => 'baseFace';
  out.faceMat.userData.flatSkin = true;   // pele sem luz: numa arena o core escurece/tinge pela luz dela (core.js)
  out.face = withOutline(new THREE.Mesh(out.faceGeo, out.faceMat), 0.013 * s * 245 / S, [Y(K.y1), Y(K.y0 + 10 * span / 26)]);   // contorno some no pescoço

  // --- tronco
  { const T = o.torso, rows = []; for (let y = T.y0; y <= T.y1; y += 4) rows.push({ y, l: cx - T.a, r: cx + T.a });
    out.torso = withOutline(new THREE.Mesh(loft(rows, { depth: T.d, cz: T.cz, capTop: false, capBot: false, pw: o.pw }), flat(o.skin)), 0.009); }

  // --- pernas (pivô no quadril) e sapatos (no tornozelo)
  const L = o.legs;
  out.legs = {}; out.feet = {}; out.hipX = {};
  if (L) for (const sd of [-1, 1]) {
    const x = cx + sd * L.dx, rows = []; for (let y = L.top; y <= L.bot; y += 4) rows.push({ y, l: x - L.r, r: x + L.r });
    const g = loft(rows, { depth: L.d, cz: 0, capTop: false, capBot: false, pw: o.pw }); g.translate(-X(x), -Y(L.top), 0);
    const leg = withOutline(new THREE.Mesh(g, flat(o.legCol ?? o.skin)), 0.008); leg.position.set(X(x), Y(L.top), 0);
    out.legs[sd] = leg; out.hipX[sd] = X(x);
    if (o.shoe) { const f = shoe(D, o.shoe, sd); f.position.x += X(x); f.position.y = Y(o.shoe.foot ?? FOOT); out.feet[sd] = f; }
  }
  out.legTop = L ? Y(L.top) : 0; out.legBot = L ? Y(L.bot) : 0;
  return out;
}

// sapato da Valéria (k = escala das medidas dela, em px do quadro de quem usa)
function shoe(D, o, s) {
  const k = o.k, SH_L = D(84 * k), SH_W = D(44 * k), SH_TOE = o.toe * Math.PI / 180;
  const shoePt = (L, W, hFn, y0, u, t, kk = 1) => {
    const e = Math.pow(Math.max(0, 1 - Math.pow(Math.abs(2 * u - 1), 2.6)), 1 / 2.6);
    const w = W / 2 * (0.8 + 0.2 * u) * e * kk, h = hFn(u) * Math.pow(e, 0.5) * kk, c = Math.cos(t), sn = Math.sin(t);
    const y = sn >= 0 ? h * Math.pow(sn, 0.55) : -h * 0.12 * Math.pow(-sn, 0.4);
    return [(u - 0.5) * L, y0 + h * 0.12 + y, w * Math.sign(c) * Math.pow(Math.abs(c), 0.7)];
  };
  const shoeGeo = (L, W, hFn, y0 = 0, u0 = 0, u1 = 1, kk = 1) => {
    const NU = u1 - u0 < 1 ? 6 : 40, NT = 32, pos = [], idx = [];
    for (let i = 0; i <= NU; i++) for (let j = 0; j < NT; j++) pos.push(...shoePt(L, W, hFn, y0, u0 + (u1 - u0) * i / NU, j / NT * Math.PI * 2, kk));
    for (let i = 0; i < NU; i++) for (let j = 0; j < NT; j++) { const a = i * NT + j, b = i * NT + (j + 1) % NT, c = a + NT, d = b + NT; idx.push(a, c, b, b, c, d); }
    const g = new THREE.BufferGeometry(); g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3)); g.setIndex(idx); g.computeVertexNormals(); return g;
  };
  const upperH = u => D(28 * k) - D(14 * k) * Math.pow(Math.max(0, (u - 0.3) / 0.7), 0.9);
  const b = new THREE.Group();
  const upper = withOutline(new THREE.Mesh(shoeGeo(SH_L, SH_W, upperH, D(5 * k)), flat(o.up)), 0.009);
  const sole = withOutline(new THREE.Mesh(shoeGeo(SH_L * 1.05, SH_W * 1.1, () => D(5 * k)), flat(o.sole)), 0.007);
  const AX = -0.2 * SH_L;
  const rim = withOutline(new THREE.Mesh(new THREE.TorusGeometry(D(15 * k), D(4 * k), 10, 28), flat(o.rim ?? o.up)), 0.006);
  rim.rotation.x = Math.PI / 2; rim.scale.y = 0.85; rim.position.set(AX, D(32 * k), 0);
  const hole = new THREE.Mesh(new THREE.CircleGeometry(D(15 * k), 28), flat(0x0f1c10)); hole.rotation.x = -Math.PI / 2; hole.scale.y = 0.85; hole.position.set(AX, D(31 * k), 0);
  b.add(sole, upper, rim, hole);
  if (o.buckle !== null) {
    const SU0 = 0.5, SU1 = 0.6, SK = 1.06;
    b.add(new THREE.Mesh(shoeGeo(SH_L, SH_W, upperH, D(5 * k), SU0, SU1, SK), flat(o.strap ?? o.sole)));
    const ta = s > 0 ? 0.75 : Math.PI - 0.75, bp = shoePt(SH_L, SH_W, upperH, D(5 * k), (SU0 + SU1) / 2, ta, SK + 0.04);
    const buckle = withOutline(new THREE.Mesh(new RoundedBoxGeometry(D(14 * k), D(14 * k), D(4 * k), 2, D(1.5 * k)), flat(o.buckle)), 0.005);
    buckle.position.set(...bp); buckle.rotation.x = -s * 0.75; b.add(buckle);
  }
  const phi = s > 0 ? -SH_TOE : Math.PI + SH_TOE;
  b.rotation.y = phi;
  const dir = new THREE.Vector3(Math.cos(phi), 0, -Math.sin(phi));
  b.position.set(-AX * dir.x, 0, -AX * dir.z);
  return b;
}
