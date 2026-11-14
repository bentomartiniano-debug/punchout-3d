// Jack Mãos de Colmeia no modo vinil: o modelo 3D dele (assets3d/jack/clean: corpo do Meshy com esqueleto, chapéu e
// colmeias, os mesmos do visualizador principal) com o acabamento do vinil (faixas de luz + retícula + contorno grosso
// de casca). Como no jogo, sem braços: o braço vira um toco liso no ombro e as colmeias flutuam onde o motor põe as
// mãos. O quadril, a coluna e a cabeça seguem a pose do tronco; as pernas vão por IK até os pés.
// Do principal: cabelo recolhido sob o chapéu, roupa (camisa mostarda com favos, emblema nas costas, favos na barra do
// casaco), abas do casaco atrasando no movimento e o rosto do sprite (olhos e boca decalcados na curva da cabeça).
import { THREE, ramp, INK, SPR } from './core.js';
import { GLTFLoader } from 'three/addons/loaders/GLTFLoader.js';
import { mergeVertices } from 'three/addons/utils/BufferGeometryUtils.js';

const ASSETS = '../../assets3d/jack/clean/';
const K = 1.15;                       // escala do modelo (1,45 m no arquivo) para o porte do vinil
const OUTLINE = 0.012;                // espessura da casca (no espaço do modelo)
const INK_PUSH = 0.02;                // quanto a casca recua na linha de visão (m)
const HIVE_SCALE = 1.0, HAT_SCALE = 1.15, HAT_Y = 1.165, HAT_TILT = 12;   // chapéu: aba na altura 1,165 m do modelo, sobe 12° na frente
const FACE = { eyeX: 0.075, eyeY: -0.03, eyeS: 0.18, mouthY: -0.163, mouthLift: 0.008, mouthW: 1.48 };
const UP = new THREE.Vector3(0, 1, 0), RIGHT = new THREE.Vector3(1, 0, 0), FWD = new THREE.Vector3(0, 0, 1);

// --- rosto do sprite do jogo (jack.png): recortes [x, y, w, h, ox, oy] no quadro, montados nas caixas dos olhos e da boca
const FS = { spr: { E_normal: [1894, 1086, 56, 17, 152, 113], E_angry: [1600, 1086, 60, 21, 150, 103], E_pain: [1248, 1086, 84, 38, 153, 90], E_x: [1778, 1086, 56, 19, 152, 109], E_tired: [1662, 1086, 56, 19, 152, 111],
                    M_normal: [2140, 1086, 20, 8, 170, 146], M_grit: [1952, 1086, 34, 16, 163, 140], M_pain: [2088, 1086, 32, 14, 165, 141], M_oh: [1580, 1086, 18, 22, 171, 136] },
             eBox: [130, 85, 100, 50], mBox: [141, 134, 74, 26],
             faces: { normal: ['E_normal', 'M_normal'], angry: ['E_angry', 'M_grit'], pain: ['E_pain', 'M_pain'], ko: ['E_x', 'M_oh'] } };
function spriteCanvas(img, name, box) {
  const k = 4, c = document.createElement('canvas'); c.width = box[2] * k; c.height = box[3] * k;
  const [x, y, w, h, ox, oy] = FS.spr[name];
  c.getContext('2d').drawImage(img, x, y, w, h, (ox - box[0]) * k, (oy - box[1]) * k, w * k, h * k);
  const t = new THREE.CanvasTexture(c); t.colorSpace = THREE.SRGBColorSpace; return t;
}
// depthWrite false: o rosto 2.5D não entra na profundidade; polygonOffset: fica sempre por cima da pele
const planeMat = t => new THREE.MeshBasicMaterial({ map: t, transparent: true, alphaTest: 0.1, depthWrite: false, polygonOffset: true, polygonOffsetFactor: -4 });

// --- cabelo sob o chapéu: o que passa da aba (cabelo espetado) é recolhido para dentro da copa (no corpo e na casca)
const hatHair = { brimY: { value: 1.165 }, r: { value: 0.145 }, h: { value: 0.16 }, slope: { value: Math.tan(THREE.MathUtils.degToRad(HAT_TILT)) } };
// abas do casaco: atrasam no movimento (deslocamento no espaço do modelo, peso _sway_coat pintado no Blender)
const swayU = { coat: { value: new THREE.Vector3() } };
// roupa (por cima da textura, no espaço do corpo em repouso)
const O = { honey: '#e8b13a', backR: 0.05, backY: 0.84, shirt: '#d9a441', skin: '#f0c3ac', neckY: 0.935, coreHalfW: 0.09, shirtHex: 0.036, shirtHalfW: 0.13, shirtY0: 0.598, hex: '#26401d', hexCell: 0.032, hexFrom: 0.55 };
function bodyShader(mat, paint) {   // paint false: só o que mexe nos vértices (para a casca)
  const prev = mat.onBeforeCompile, key = mat.customProgramCacheKey();
  mat.onBeforeCompile = (sh, r) => {
    prev.call(mat, sh, r);
    Object.assign(sh.uniforms, { uBrimY: hatHair.brimY, uCrownR: hatHair.r, uCrownH: hatHair.h, uSlope: hatHair.slope, uCoatOff: swayU.coat });
    sh.vertexShader = 'uniform float uBrimY, uCrownR, uCrownH, uSlope; uniform vec3 uCoatOff; attribute float _sway_coat; attribute float _shirt; varying float vCoatW; varying float vShirt; varying vec3 vBP;\n' + sh.vertexShader
      .replace('#include <begin_vertex>', `#include <begin_vertex>
        float by = uBrimY + uSlope * position.z;                       // aba inclinada: mais alta na frente
        float hk = smoothstep(by - 0.015, by + 0.025, position.y);    // embaixo da aba o cabelo fica como é
        float hr = length(position.xz);
        if (hr > uCrownR) transformed.xz = mix(transformed.xz, position.xz * (uCrownR / hr), hk);
        transformed.y = mix(transformed.y, by + min(position.y - by, uCrownH) * 0.75, hk * step(by, position.y));
        transformed += uCoatOff * _sway_coat;
        vCoatW = _sway_coat; vShirt = _shirt; vBP = position;`);
    if (!paint) return;
    Object.assign(sh.uniforms, { uSkin: { value: new THREE.Color(O.skin) }, uShirt: { value: new THREE.Color(O.shirt) }, uHexCol: { value: new THREE.Color(O.hex) }, uHoney: { value: new THREE.Color(O.honey) } });
    sh.fragmentShader = 'uniform vec3 uSkin, uShirt, uHexCol, uHoney; varying float vCoatW; varying float vShirt; varying vec3 vBP;\n' + sh.fragmentShader
      .replace('#include <map_fragment>', `#include <map_fragment>
        vec3 tc = diffuseColor.rgb;
        bool greenish = tc.g > tc.r * 1.08 && tc.g > tc.b * 1.05;
        // camisa: região marcada no Blender + faixa central (pega as manchas verdes perto do cinto); acima do decote em U é pescoço
        float neck = ${O.neckY.toFixed(3)} + 4.0 * vBP.x * vBP.x;
        bool front = vBP.z > 0.0 && vBP.y > ${O.shirtY0.toFixed(3)};
        bool core = front && abs(vBP.x) < ${O.coreHalfW.toFixed(3)} && vBP.y < neck;
        if ((vShirt > 0.005 || core) && front) {
          if (vBP.y > neck) diffuseColor.rgb = uSkin;
          else {
            diffuseColor.rgb = uShirt;
            vec2 hp = vBP.xy / ${O.shirtHex.toFixed(4)};                  // favos miúdos no tecido
            const vec2 hs2 = vec2(1.0, 1.7320508);
            vec4 c2 = floor(vec4(hp, hp - vec2(0.5, 1.0)) / hs2.xyxy) + 0.5;
            vec4 h2 = vec4(hp - c2.xy * hs2, hp - (c2.zw + 0.5) * hs2);
            vec2 g2 = dot(h2.xy, h2.xy) < dot(h2.zw, h2.zw) ? h2.xy : h2.zw;
            vec2 a2 = abs(g2); float d2 = max(dot(a2, hs2 * 0.5), a2.x);
            diffuseColor.rgb *= 1.0 - 0.14 * smoothstep(0.40, 0.45, d2);
            float side = smoothstep(${(O.coreHalfW * 0.6).toFixed(3)}, ${O.shirtHalfW.toFixed(3)}, abs(vBP.x));   // sombra embaixo das abas do casaco
            diffuseColor.rgb *= 1.0 - 0.25 * side;
            gTone = max(gTone, side * 0.6);
          }
        }
        // emblema nas costas: 3 favos em triângulo entre as omoplatas
        if (vBP.z < -0.04 && abs(vBP.x) < 0.2 && greenish && vShirt < 0.005) {
          float a = ${O.backR.toFixed(3)}, R = a / 0.866;
          vec2 base = vec2(0.0, ${O.backY.toFixed(3)});
          vec2 cs[3]; cs[0] = base + vec2(0.0, 0.75 * R); cs[1] = base + vec2(-a, -0.75 * R); cs[2] = base + vec2(a, -0.75 * R);
          vec2 bp = vec2(atan(vBP.x, -vBP.z) * length(vBP.xz), vBP.y);
          float best = 9.0;
          for (int i = 0; i < 3; i++) { vec2 q = abs(bp - cs[i]); best = min(best, max(q.x, 0.5 * q.x + 0.866 * q.y) / a); }
          if (best < 1.0) diffuseColor.rgb = mix(uHoney, uHexCol, smoothstep(0.80, 0.86, best));
        }
        // favos na barra do casaco
        if (vCoatW > ${O.hexFrom.toFixed(3)} && greenish) {
          vec2 p = vec2(atan(vBP.x, vBP.z) * 0.2, vBP.y) / ${O.hexCell.toFixed(4)};
          const vec2 hs = vec2(1.0, 1.7320508);
          vec4 hc = floor(vec4(p, p - vec2(0.5, 1.0)) / hs.xyxy) + 0.5;
          vec4 hh = vec4(p - hc.xy * hs, p - (hc.zw + 0.5) * hs);
          vec2 g = dot(hh.xy, hh.xy) < dot(hh.zw, hh.zw) ? hh.xy : hh.zw;
          vec2 ag = abs(g); float hd = max(dot(ag, hs * 0.5), ag.x);
          float edge = smoothstep(0.40, 0.44, hd) * smoothstep(${O.hexFrom.toFixed(3)}, ${(O.hexFrom + 0.08).toFixed(3)}, vCoatW);
          diffuseColor.rgb = mix(diffuseColor.rgb, uHexCol, edge);
        }`);
  };
  mat.customProgramCacheKey = () => key + '|jackbody' + (paint ? 'P' : '');
}

// material do vinil + casca invertida (mesma geometria e esqueleto, vértices empurrados na normal depois do skinning)
function toonify(root) {
  const hulls = [];
  root.traverse(o => {
    if (!o.isMesh || o.userData.hull) return;
    const old = o.material;
    o.material = new THREE.MeshToonMaterial({ map: old.map || null, color: old.map ? 0xffffff : (old.color || 0xffffff), gradientMap: ramp, name: old.name });
    // sem mipmap: a textura do Meshy é picotada em ilhas e o mipmap puxa a cor da ilha vizinha (fios brancos nas costuras)
    if (o.material.map) { const m = o.material.map; m.colorSpace = THREE.SRGBColorSpace; m.generateMipmaps = false; m.minFilter = THREE.LinearFilter; m.needsUpdate = true; }
    const mat = new THREE.MeshBasicMaterial({ color: INK, side: THREE.BackSide });
    mat.onBeforeCompile = sh => {
      sh.vertexShader = sh.vertexShader
        // casca recuada na linha de visão: some nas dobras e fica só na silhueta
        .replace('#include <project_vertex>', `vec4 mvPosition = modelViewMatrix * vec4(transformed, 1.0); mvPosition.xyz += normalize(mvPosition.xyz) * ${INK_PUSH.toFixed(4)}; gl_Position = projectionMatrix * mvPosition;`)
        .replace('#include <skinning_vertex>', `#include <skinning_vertex>
        #ifdef USE_SKINNING
        vec3 nrm = normalize(objectNormal);   // já passou pelo skinning (e pela esfera do ombro)
        #else
        vec3 nrm = normalize(normal);
        #endif
        #ifdef ARM_COLLAPSE
        transformed += nrm * ${OUTLINE.toFixed(4)} * (1.0 - armK);
        #else
        transformed += nrm * ${OUTLINE.toFixed(4)};
        #endif`);
    };
    // o Meshy separa vértices nas costuras da textura; a casca precisa da malha soldada, senão abre rasgos
    const keep = ['position', 'skinIndex', 'skinWeight', '_sway_coat', '_shirt'];
    const g = o.geometry.clone(); for (const k of Object.keys(g.attributes)) if (!keep.includes(k)) g.deleteAttribute(k);
    const welded = mergeVertices(g, 1e-4); welded.computeVertexNormals();
    if (o.isSkinnedMesh && o.geometry.attributes._sway_coat) bodyShader(mat, false);
    if (o.isSkinnedMesh) { armWeights(welded, o.skeleton); armCollapse(mat, true); }   // casca acompanha os braços recolhidos
    const hull = o.isSkinnedMesh ? new THREE.SkinnedMesh(welded, mat) : new THREE.Mesh(welded, mat);
    if (o.isSkinnedMesh) hull.bind(o.skeleton, o.bindMatrix);
    hull.userData.hull = true; o.userData.hullMesh = hull; hulls.push([o, hull]);
  });
  // com esqueleto: irmã da malha (mesmo bind); sem esqueleto: filha dela (acompanha a peça para onde for)
  for (const [o, hull] of hulls) { if (o.isSkinnedMesh) { o.parent.add(hull); hull.position.copy(o.position); hull.quaternion.copy(o.quaternion); hull.scale.copy(o.scale); } else o.add(hull); }
}

// --- sem braços: cada vértice ganha o peso do braço esquerdo/direito (ossos Arm/ForeArm/Hand); no shader, antes do
// skinning, ele é puxado para uma esfera no ombro, na proporção desse peso. O braço vira um toco liso.
const ARM_R = 0.045, ARM_IN = 0.07;   // raio do toco e quanto o centro dele entra no tronco (espaço do modelo)
const armU = { shL: { value: new THREE.Vector3() }, shR: { value: new THREE.Vector3() } };
function armWeights(geo, skeleton) {
  const si = geo.attributes.skinIndex, sw = geo.attributes.skinWeight, out = new Float32Array(si.count * 2);
  const side = skeleton.bones.map(b => /^Left(Arm|ForeArm|Hand)/.test(b.name) ? 1 : /^Right(Arm|ForeArm|Hand)/.test(b.name) ? 2 : 0);
  for (let i = 0; i < si.count; i++) for (let k = 0; k < 4; k++) { const s = side[si.getComponent(i, k)]; if (s) out[i * 2 + s - 1] += sw.getComponent(i, k); }
  geo.setAttribute('armW', new THREE.BufferAttribute(out, 2));
}
function armCollapse(mat, hull = false) {   // hull: a casca do toco vai mais para dentro e perde a espessura (sem riscos no ombro)
  const prev = mat.onBeforeCompile, key = mat.customProgramCacheKey();
  mat.onBeforeCompile = (sh, r) => {
    prev.call(mat, sh, r);
    Object.assign(sh.uniforms, { uShL: armU.shL, uShR: armU.shR });
    sh.vertexShader = '#define ARM_COLLAPSE\nattribute vec2 armW; uniform vec3 uShL, uShR;\n' + sh.vertexShader
      .replace('#include <beginnormal_vertex>', `#include <beginnormal_vertex>
        vec3 armP = position; float armK = 0.0;
        for (int s = 0; s < 2; s++) {
          float w = s == 0 ? armW.x : armW.y; if (w < 0.001) continue;
          vec3 c = s == 0 ? uShL : uShR, d = position - c; float L = length(d);
          vec3 n = L > 1e-5 ? d / L : vec3(0.0, 1.0, 0.0);
          float k = clamp(sqrt(w) * 1.25, 0.0, 1.0);   // puxa já com pouco peso: a lateral desce redonda até o toco (sem quina)
          armP = mix(armP, c + n * ${(hull ? ARM_R * 0.6 : ARM_R).toFixed(3)}, k); armK = max(armK, k);
          objectNormal = normalize(mix(objectNormal, n, k));
        }`)
      .replace('#include <begin_vertex>', '#include <begin_vertex>\ntransformed = armP;');
  };
  mat.customProgramCacheKey = () => key + '|arms';
}

// --- IK de dois ossos (independe do eixo local dos ossos do Meshy)
const _a = new THREE.Vector3(), _b = new THREE.Vector3(), _c = new THREE.Vector3(), _q = new THREE.Quaternion(), _pq = new THREE.Quaternion();
function rotateBoneWorld(bone, from, to) {
  if (from.lengthSq() < 1e-12 || to.lengthSq() < 1e-12) return;
  _q.setFromUnitVectors(from.clone().normalize(), to.clone().normalize());
  bone.getWorldQuaternion(_pq);
  const wanted = _q.multiply(_pq);
  bone.parent.getWorldQuaternion(_pq);
  bone.quaternion.copy(_pq.invert().multiply(wanted));
  bone.updateMatrixWorld(true);
}
function solveIK(upper, lower, hand, target, pole) {
  upper.getWorldPosition(_a); lower.getWorldPosition(_b); hand.getWorldPosition(_c);
  const l1 = _a.distanceTo(_b), l2 = _b.distanceTo(_c);
  const toT = target.clone().sub(_a); const d = Math.min(toT.length(), (l1 + l2) * 0.999); toT.setLength(d);
  const cosA = (l1 * l1 + d * d - l2 * l2) / (2 * l1 * d), sinA = Math.sqrt(Math.max(0, 1 - cosA * cosA));
  const dir = toT.clone().normalize(), side = pole.clone().sub(_a).projectOnPlane(dir);
  if (side.lengthSq() < 1e-8) side.copy(new THREE.Vector3(0, -1, 0).projectOnPlane(dir));
  if (side.lengthSq() < 1e-8) side.set(1, 0, 0).projectOnPlane(dir);
  side.normalize();
  const elbow = _a.clone().addScaledVector(dir, l1 * cosA).addScaledVector(side, l1 * sinA);
  rotateBoneWorld(upper, _b.clone().sub(_a), elbow.clone().sub(_a));
  lower.getWorldPosition(_b); hand.getWorldPosition(_c);
  rotateBoneWorld(lower, _c.clone().sub(_b), _a.clone().add(toT).sub(_b));
}
// gira um osso em eixo de MUNDO (radianos)
function rotW(bone, axis, rad) {
  if (!rad) return;
  const pq = bone.parent.getWorldQuaternion(new THREE.Quaternion());
  bone.quaternion.premultiply(pq.clone().invert().multiply(new THREE.Quaternion().setFromAxisAngle(axis, rad)).multiply(pq));
  bone.updateMatrixWorld(true);
}

export default {
  title: 'Jack (Meshy)',
  units: [245, 440, 240],
  faces: [['normal', 'Normal'], ['angry', 'Raiva'], ['pain', 'Dor'], ['ko', 'K.O.']],
  others: [['valeria', 'Valéria'], ['frog', 'King Frog']],
  async build({ scene, root }) {
    const loader = new GLTFLoader();
    const load = f => loader.loadAsync(ASSETS + f);
    const img = new Image();
    const [body, gL, gR, hat] = await Promise.all([load('body_jack.glb'), load('prop_jack_hive_L.glb'), load('prop_jack_hive_R.glb'), load('prop_jack_hat.glb'),
      new Promise((ok, bad) => { img.onload = ok; img.onerror = bad; img.src = SPR + 'jack.png'; })]);
    const jack = body.scene;
    const holder = new THREE.Group(); holder.scale.setScalar(K); holder.add(jack); root.add(holder);
    const bones = {}, sockets = {};
    jack.traverse(o => { if (o.isBone) bones[o.name] = o; if (/^(head_top|head_face|hand_[LR]_grip|spine_back)$/.test(o.name)) sockets[o.name] = o; });
    root.updateMatrixWorld(true);
    jack.traverse(o => { if (o.isSkinnedMesh) armWeights(o.geometry, o.skeleton); });
    { const m = jack.getObjectByProperty('isSkinnedMesh', true);
      for (const [u, b] of [[armU.shL, bones.LeftArm], [armU.shR, bones.RightArm]]) { u.value.copy(m.worldToLocal(b.getWorldPosition(new THREE.Vector3()))); u.value.x -= Math.sign(u.value.x) * ARM_IN; }
      const L = armU.shL.value, R = armU.shR.value, ax = (L.x - R.x) / 2, ay = (L.y + R.y) / 2, az = (L.z + R.z) / 2;   // tocos no espelho da média
      L.set(ax, ay, az); R.set(-ax, ay, az); }
    toonify(jack);
    jack.traverse(o => { if (o.isSkinnedMesh && !o.userData.hull) { if (o.geometry.attributes._sway_coat) bodyShader(o.material, true); armCollapse(o.material); } });

    // chapéu no topo da cabeça (aba na altura do modelo, inclinada para cima na frente); colmeias soltas na cena
    root.updateMatrixWorld(true);
    const crown = hat.scene; toonify(crown); sockets.head_top.add(crown); crown.scale.setScalar(HAT_SCALE);
    { const wp = sockets.head_top.getWorldPosition(new THREE.Vector3()); wp.y = holder.localToWorld(new THREE.Vector3(0, HAT_Y, 0)).y;
      crown.position.copy(sockets.head_top.worldToLocal(wp));
      const pq = sockets.head_top.getWorldQuaternion(new THREE.Quaternion());
      const want = new THREE.Quaternion().setFromAxisAngle(RIGHT, THREE.MathUtils.degToRad(HAT_TILT)).multiply(pq.clone().multiply(crown.quaternion));
      crown.quaternion.copy(pq.invert().multiply(want)); }
    const gloves = { [-1]: gR.scene, [1]: gL.scene };   // -1 = esquerda da tela = mão direita dele (de frente para a câmera)
    for (const g of Object.values(gloves)) { toonify(g); g.traverse(o => { if (o.isMesh && !o.userData.hull) o.material.side = THREE.DoubleSide; }); g.scale.setScalar(HIVE_SCALE * K); scene.add(g); }

    // --- pose de descanso: tudo volta a ela a cada quadro e a pose do motor entra por cima
    root.updateMatrixWorld(true);
    const rest = new Map(); for (const b of Object.values(bones)) rest.set(b, { q: b.quaternion.clone(), p: b.position.clone() });
    const side = sx => sx < 0 ? 'Right' : 'Left';
    const feetHome = {}, feetQ = {};
    for (const sx of [-1, 1]) { const F = bones[side(sx) + 'Foot']; feetHome[sx] = root.worldToLocal(F.getWorldPosition(new THREE.Vector3())); feetQ[sx] = F.getWorldQuaternion(new THREE.Quaternion()); }
    const HIPS = bones.Hips.getWorldPosition(new THREE.Vector3()).y, NECK = bones.Head.getWorldPosition(new THREE.Vector3()).y;
    const chest = bones.Spine02.getWorldPosition(new THREE.Vector3());

    // --- rosto do sprite preso no osso da cabeça: cada olho é uma metade do recorte, boca embaixo, decalcados na pele
    const half = (b, s) => [b[0] + (s ? b[2] / 2 : 0), b[1], b[2] / 2, b[3]];
    const faceTex = {};
    for (const [f, [e, m]] of Object.entries(FS.faces)) faceTex[f] = { eyeA: spriteCanvas(img, e, half(FS.eBox, 0)), eyeB: spriteCanvas(img, e, half(FS.eBox, 1)), mouth: spriteCanvas(img, m, FS.mBox) };
    const fp = new THREE.Group(); bones.Head.add(fp);
    const eyes = [0, 1].map(i => new THREE.Mesh(new THREE.BufferGeometry(), planeMat(faceTex.normal[i ? 'eyeB' : 'eyeA'])));
    const mouth = new THREE.Mesh(new THREE.BufferGeometry(), planeMat(faceTex.normal.mouth));
    fp.add(...eyes, mouth);
    {
      const headRest = bones.Head.matrixWorld.clone(), headRestInv = headRest.clone().invert();
      const headCut = sockets.head_face.getWorldPosition(new THREE.Vector3()).y - 0.3 * K, rayHead = [];
      jack.traverse(m => {
        if (!m.isSkinnedMesh || m.userData.hull || !m.geometry.index || /buckle/.test(m.name)) return;
        const pos = m.geometry.attributes.position, idx = m.geometry.index, out = [], tri = [0, 0, 0].map(() => new THREE.Vector3());
        for (let t = 0; t < idx.count; t += 3) {
          for (let k = 0; k < 3; k++) tri[k].fromBufferAttribute(pos, idx.getX(t + k)).applyMatrix4(m.matrixWorld);
          if ((tri[0].y + tri[1].y + tri[2].y) / 3 < headCut) continue;
          for (const p of tri) out.push(p.x, p.y, p.z);
        }
        const g = new THREE.BufferGeometry(); g.setAttribute('position', new THREE.Float32BufferAttribute(out, 3));
        rayHead.push(new THREE.Mesh(g, new THREE.MeshBasicMaterial({ side: THREE.DoubleSide })));
      });
      const q0 = bones.Head.getWorldQuaternion(new THREE.Quaternion()).invert();
      const anchor = bones.Head.worldToLocal(sockets.head_face.getWorldPosition(new THREE.Vector3()));
      const local = (dx, dy, dz) => new THREE.Vector3(dx, dy, dz).applyQuaternion(q0).add(anchor);
      const ray = new THREE.Raycaster();
      const decal = (center, w, h, nu, nv, lift) => {
        const fwd = new THREE.Vector3(0, 0, 1).applyQuaternion(q0), right = new THREE.Vector3(1, 0, 0).applyQuaternion(q0), up = new THREE.Vector3(0, 1, 0).applyQuaternion(q0);
        const pos = [], uv = [], idx = [];
        for (let j = 0; j <= nv; j++) for (let i = 0; i <= nu; i++) {
          const u = i / nu, v = j / nv;
          const p = center.clone().addScaledVector(right, (u - 0.5) * w).addScaledVector(up, (v - 0.5) * h);
          const o = p.clone().addScaledVector(fwd, 0.6).applyMatrix4(headRest), d = p.clone().applyMatrix4(headRest).sub(o).normalize();
          ray.set(o, d); ray.far = 1.5;
          const hit = ray.intersectObjects(rayHead, false)[0];
          const sp = hit ? hit.point.clone().applyMatrix4(headRestInv) : p;
          sp.addScaledVector(fwd, lift); pos.push(sp.x, sp.y, sp.z); uv.push(u, v);
        }
        for (let j = 0; j < nv; j++) for (let i = 0; i < nu; i++) { const a = j * (nu + 1) + i, b = a + 1, c = a + nu + 1, d = c + 1; idx.push(a, b, d, a, d, c); }
        const g = new THREE.BufferGeometry(); g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3)); g.setAttribute('uv', new THREE.Float32BufferAttribute(uv, 2)); g.setIndex(idx);
        return g;
      };
      const { eyeY: y, eyeS: s, mouthY: my, mouthLift: lift, mouthW } = FACE, off = s / 2;
      eyes.forEach((m, i) => { m.geometry = decal(local(i ? off : -off, y, 0), s, s, 12, 10, lift); });
      mouth.geometry = decal(local(0, my, 0), s * mouthW, s * mouthW * FS.mBox[3] / FS.mBox[2], 28, 6, lift);
    }
    const setFace = f => { const T = faceTex[f] || faceTex.normal; eyes[0].material.map = T.eyeA; eyes[1].material.map = T.eyeB; mouth.material.map = T.mouth; for (const m of [...eyes, mouth]) m.material.needsUpdate = true; };

    // --- o que o motor mexe: grupos "fantasma" (não aparecem) cuja pose vira rotação nos ossos
    const upper = new THREE.Group(), neck = new THREE.Group(), hips = new THREE.Group();
    upper.position.y = HIPS; hips.position.y = HIPS; neck.position.y = NECK - HIPS; upper.add(neck); root.add(upper, hips);
    const hands = { [-1]: new THREE.Object3D(), [1]: new THREE.Object3D() }; root.add(hands[-1], hands[1]);
    const footOff = { [-1]: [0, 0], [1]: [0, 0] };
    const placeFoot = (sx, dx, lift) => { footOff[sx] = [dx, lift]; };
    const tmp = new THREE.Vector3(), pole = new THREE.Vector3();
    const sway = { off: new THREE.Vector3(), vel: new THREE.Vector3(), last: null };
    const onFrame = c => {
      for (const [b, r] of rest) { b.quaternion.copy(r.q); b.position.copy(r.p); }
      root.updateMatrixWorld(true);
      // quadril: desce/desloca com o tronco e gira (o resto do giro vai para a coluna, a cabeça por último)
      const H = bones.Hips;
      const hw = H.getWorldPosition(new THREE.Vector3()).add(tmp.set(hips.position.x, hips.position.y - HIPS, hips.position.z));
      H.position.copy(H.parent.worldToLocal(hw)); H.updateMatrixWorld(true);
      rotW(H, UP, hips.rotation.y); rotW(H, RIGHT, hips.rotation.x); rotW(H, FWD, hips.rotation.z);
      for (const n of ['Spine', 'Spine01', 'Spine02']) { rotW(bones[n], UP, (upper.rotation.y - hips.rotation.y) / 3); rotW(bones[n], RIGHT, (upper.rotation.x - hips.rotation.x) / 3); rotW(bones[n], FWD, (upper.rotation.z - hips.rotation.z) / 3); }
      rotW(bones.Head, UP, neck.rotation.y); rotW(bones.Head, RIGHT, neck.rotation.x); rotW(bones.Head, FWD, neck.rotation.z);
      // pernas: IK até o pé (plantado na casa dele + o que a passada/o salto pedem), pé reto no chão
      for (const sx of [-1, 1]) {
        const k = side(sx), [dx, lift] = footOff[sx];
        const target = root.localToWorld(feetHome[sx].clone().add(tmp.set(dx, lift, 0)));
        const hip = bones[k + 'UpLeg'].getWorldPosition(new THREE.Vector3());
        pole.copy(hip).add(tmp.set(k === 'Left' ? 0.15 : -0.15, -0.5, 1.0));   // joelho para a frente e um pouco para fora
        solveIK(bones[k + 'UpLeg'], bones[k + 'Leg'], bones[k + 'Foot'], target, pole);
        const F = bones[k + 'Foot'], pq = F.parent.getWorldQuaternion(new THREE.Quaternion()).invert();
        F.quaternion.copy(pq.multiply(feetQ[sx])); F.updateMatrixWorld(true);
      }
      // abas do casaco: mola puxada de volta a zero, empurrada pela velocidade do quadril (só para trás e para os lados)
      const hp = bones.Hips.getWorldPosition(new THREE.Vector3());
      if (!sway.last) sway.last = hp.clone();
      const dt = Math.max(1e-3, c.dt), hv = hp.clone().sub(sway.last).divideScalar(dt * K); sway.last.copy(hp);
      for (let i = 0; i < 6; i++) { const h = dt / 6; sway.vel.addScaledVector(sway.off, -220 * h).addScaledVector(sway.vel, -9 * h).addScaledVector(hv, -22 * h); sway.off.addScaledVector(sway.vel, h); }
      sway.off.clampLength(0, 0.06);
      swayU.coat.value.copy(sway.off).multiplyScalar(1.4); swayU.coat.value.z = Math.min(0, swayU.coat.value.z);
      // colmeias flutuam onde o motor pôs as mãos: furos para cima (levemente para dentro), boca da colmeia para a
      // frente; no soco ela vai reta para onde o golpe vai; giram junto com o tronco
      for (const sx of [-1, 1]) {
        const G = gloves[sx], k = Math.min(1, Math.max(0, c.pose[sx][0] / 0.3));
        G.position.copy(hands[sx].getWorldPosition(new THREE.Vector3()));
        const yq = new THREE.Quaternion().setFromAxisAngle(UP, upper.rotation.y * 0.8);
        const dir = new THREE.Vector3(0, 0.2, 1).lerp(new THREE.Vector3(0, -0.05, 1), k).normalize().applyQuaternion(yq);
        const zRef = new THREE.Vector3(sx * 0.35, 1, 0).normalize().applyQuaternion(yq);
        const zA = zRef.addScaledVector(dir, -zRef.dot(dir)).normalize(), xA = new THREE.Vector3().crossVectors(dir, zA);
        G.quaternion.setFromRotationMatrix(new THREE.Matrix4().makeBasis(xA, dir, zA));
      }
    };
    return { P: { jack, bones, sockets, gloves, crown, armU }, upper, WAIST: HIPS, neck, NECK, hips, HIPS, hands, hipSway: 0.6,
             hand: { x: 0.3 * K, y: chest.y + 0.02 * K, z: chest.z + 0.4 * K, yaw: -0.12 }, placeFoot, setFace, onFrame };
  },
};
