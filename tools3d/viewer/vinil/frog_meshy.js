// King Frog no modo vinil: o modelo 3D dele (assets3d/frog/clean: corpo do Meshy com esqueleto, coroa e luvas, os
// mesmos do visualizador principal) com o acabamento do vinil (faixas de luz + retícula + contorno grosso de casca)
// e os ossos movidos pelas animações do motor: o quadril, a coluna e a cabeça seguem a pose do tronco; os braços vão
// por IK até as luvas (que o motor posiciona como mãos) e as pernas por IK até os pés (plantados, passada, salto).
// Rosto 2.5D (olhos em planos, boca e narinas decalcadas na curva da cabeça) e barriga clara vêm do principal.
import { THREE, ramp, INK } from './core.js';
import { GLTFLoader } from 'three/addons/loaders/GLTFLoader.js';
import { mergeVertices } from 'three/addons/utils/BufferGeometryUtils.js';

const ASSETS = '../../assets3d/frog/clean/';
const K = 0.85;                       // escala do modelo (2,09 m no arquivo) para o porte do vinil
const OUTLINE = 0.016;                // espessura da casca (no espaço do modelo)
const INK_PUSH = 0.02;                // quanto a casca recua na linha de visão (m)
const GLOVE_SCALE = 1.55, CROWN_SCALE = 1.8, GLOVE_SINK = -0.1;
const FACE = { eyeX: 0.23, eyeY: -0.07, eyeZ: -0.045, eyeS: 0.23, mouthY: -0.25, mouthLift: 0.02, mouthW: 3.2 };
const UP = new THREE.Vector3(0, 1, 0), RIGHT = new THREE.Vector3(1, 0, 0), FWD = new THREE.Vector3(0, 0, 1);

// --- rosto do King Frog no jogo (os mesmos desenhos do visualizador principal)
const INKS = '#0f1c10';
function faceCanvas(draw) { const c = document.createElement('canvas'); c.width = c.height = 256; const g = c.getContext('2d'); g.scale(2, 2); draw(g); const t = new THREE.CanvasTexture(c); t.colorSpace = THREE.SRGBColorSpace; return t; }
function gameEye(g, slope, glint = true) {   // bloco preto com o topo caindo para dentro (cara de mau) e um ponto branco de brilho
  g.fillStyle = INKS; g.beginPath(); g.moveTo(14, 34 - slope * 0.2); g.lineTo(114, 34 + slope); g.lineTo(114, 104); g.lineTo(14, 104); g.closePath(); g.fill();
  if (glint) { g.fillStyle = '#ffffff'; g.fillRect(70, 62, 18, 18); }
}
const EYES = {
  normal: g => gameEye(g, 22),
  angry: g => gameEye(g, 46),
  pain: g => { g.lineWidth = 16; g.lineCap = 'round'; g.lineJoin = 'round'; g.strokeStyle = INKS; g.beginPath(); g.moveTo(26, 30); g.lineTo(100, 66); g.lineTo(26, 102); g.stroke(); },
  ko: g => { g.lineWidth = 16; g.lineCap = 'round'; g.strokeStyle = INKS; g.beginPath(); g.moveTo(30, 30); g.lineTo(98, 98); g.moveTo(98, 30); g.lineTo(30, 98); g.stroke(); },
};
const MOUTH = {
  normal: g => { g.lineWidth = 7; g.lineCap = 'round'; g.strokeStyle = INKS; g.beginPath(); g.moveTo(10, 70); g.quadraticCurveTo(64, 56, 118, 70); g.stroke(); },
  angry: g => { g.fillStyle = '#f8f4e6'; g.strokeStyle = INKS; g.lineWidth = 6; g.lineJoin = 'round';   // rangendo os dentes
    g.beginPath(); g.moveTo(14, 74); g.quadraticCurveTo(64, 56, 114, 74); g.lineTo(110, 90); g.quadraticCurveTo(64, 76, 18, 90); g.closePath(); g.fill(); g.stroke();
    g.lineWidth = 3; g.beginPath(); g.moveTo(16, 82); g.quadraticCurveTo(64, 66, 112, 82); g.stroke();
    for (let x = 30; x < 104; x += 12) { g.beginPath(); g.moveTo(x, 64 + Math.abs(x - 64) * 0.18); g.lineTo(x, 86 + Math.abs(x - 64) * 0.06); g.stroke(); } },
  pain: g => { g.fillStyle = '#f8f4e6'; g.strokeStyle = INKS; g.lineWidth = 6; g.beginPath(); g.moveTo(12, 70); g.quadraticCurveTo(64, 40, 116, 72); g.lineTo(112, 84); g.quadraticCurveTo(64, 62, 16, 82); g.closePath(); g.fill(); g.stroke(); g.lineWidth = 3; for (let x = 28; x < 110; x += 14) { g.beginPath(); g.moveTo(x, 56); g.lineTo(x, 80); g.stroke(); } },
  ko: g => { g.fillStyle = '#2a0a12'; g.strokeStyle = INKS; g.lineWidth = 6; g.beginPath(); g.ellipse(64, 70, 40, 16, 0, 0, 7); g.fill(); g.stroke(); g.fillStyle = '#ff5c7a'; g.beginPath(); g.ellipse(84, 84, 14, 12, 0, 0, 7); g.fill(); },
};
const faceTex = {}; for (const k of Object.keys(EYES)) faceTex[k] = { eye: faceCanvas(EYES[k]), mouth: faceCanvas(MOUTH[k]) };
const noseTex = faceCanvas(g => { g.fillStyle = INKS; g.beginPath(); g.ellipse(34, 64, 13, 9, 0.35, 0, 7); g.fill(); g.beginPath(); g.ellipse(94, 64, 13, 9, -0.35, 0, 7); g.fill(); });
// depthWrite false: o rosto 2.5D não entra na profundidade; polygonOffset: fica sempre por cima da pele
const planeMat = t => new THREE.MeshBasicMaterial({ map: t, transparent: true, alphaTest: 0.1, depthWrite: false, polygonOffset: true, polygonOffsetFactor: -4 });

// barriga clara: elipse lisa no espaço do corpo em repouso, pintada no shader (borda limpa, sem os retalhos da textura)
const belly = { color: { value: new THREE.Color('#a8c97f') }, c: { value: new THREE.Vector2(0, 1.42) }, r: { value: new THREE.Vector2(0.34, 0.5) }, floor: { value: 0.97 }, ceil: { value: 1.79 } };
function bellyShader(mat) {
  const prev = mat.onBeforeCompile;
  mat.onBeforeCompile = (sh, r) => {
    prev.call(mat, sh, r);
    Object.assign(sh.uniforms, { uBellyCol: belly.color, uBellyC: belly.c, uBellyR: belly.r, uBellyFloor: belly.floor, uBellyCeil: belly.ceil });
    sh.vertexShader = 'varying vec3 vBindPos; varying vec3 vBindNrm;\n' + sh.vertexShader.replace('#include <begin_vertex>', '#include <begin_vertex>\nvBindPos = position; vBindNrm = normal;');
    sh.fragmentShader = 'uniform vec3 uBellyCol; uniform vec2 uBellyC, uBellyR; uniform float uBellyFloor, uBellyCeil; varying vec3 vBindPos; varying vec3 vBindNrm;\n' + sh.fragmentShader
      .replace('#include <map_fragment>', `#include <map_fragment>
        vec2 q = (vBindPos.xy - uBellyC) / uBellyR;
        float e = dot(q, q), aa = fwidth(e) * 1.5;
        float m = (1.0 - smoothstep(1.0 - aa, 1.0 + aa, e)) * smoothstep(0.05, 0.2, normalize(vBindNrm).z)
                * smoothstep(uBellyFloor - 0.005, uBellyFloor + 0.005, vBindPos.y) * (1.0 - smoothstep(uBellyCeil - 0.005, uBellyCeil + 0.005, vBindPos.y));
        diffuseColor.rgb = mix(diffuseColor.rgb, uBellyCol, m);`);
  };
  const key = mat.customProgramCacheKey(); mat.customProgramCacheKey = () => key + '|belly';
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
        // casca recuada na linha de visão: some nas dobras (short, luvas, corpo) e fica só na silhueta
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
    const g = o.geometry.clone(); for (const k of Object.keys(g.attributes)) if (!['position', 'skinIndex', 'skinWeight'].includes(k)) g.deleteAttribute(k);
    const welded = mergeVertices(g, 1e-4); welded.computeVertexNormals();
    if (o.isSkinnedMesh && o.geometry.attributes.armW) { armWeights(welded, o.skeleton); armCollapse(mat, true); mat.userData.armHull = true; }   // casca acompanha os braços recolhidos
    const hull = o.isSkinnedMesh ? new THREE.SkinnedMesh(welded, mat) : new THREE.Mesh(welded, mat);
    if (o.isSkinnedMesh) hull.bind(o.skeleton, o.bindMatrix);
    hull.userData.hull = true; o.userData.hullMesh = hull; hulls.push([o, hull]);
  });
  // com esqueleto: irmã da malha (mesmo bind); sem esqueleto: filha dela (acompanha a peça para onde for)
  for (const [o, hull] of hulls) { if (o.isSkinnedMesh) { o.parent.add(hull); hull.position.copy(o.position); hull.quaternion.copy(o.quaternion); hull.scale.copy(o.scale); } else o.add(hull); }
}

// --- sem braços (como no jogo: só as luvas flutuam). Cada vértice ganha o peso do braço esquerdo/direito (soma dos
// pesos nos ossos Arm/ForeArm/Hand); no shader, antes do skinning, ele é puxado para uma esfera no ombro, na
// proporção desse peso. O que é só ombro fica; o braço vira um toco.
const ARM_R = 0.06, ARM_IN = 0.17;     // raio do toco e quanto o centro dele entra no tronco (espaço do modelo): ombro rente ao corpo
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
    // na esfera do ombro: posição E normal da esfera (luz e contorno lisos, sem a rugosidade do braço amassado)
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
  title: 'King Frog (Meshy)',
  units: [245, 440, 240],
  faces: [['normal', 'Normal'], ['angry', 'Raiva'], ['pain', 'Dor'], ['ko', 'K.O.']],
  others: [['valeria', 'Valéria']],
  async build({ scene, root, U }) {
    const loader = new GLTFLoader();
    const load = f => loader.loadAsync(ASSETS + f);
    const [body, gL, gR, cr] = await Promise.all([load('body_frog.glb'), load('prop_frog_glove_L.glb'), load('prop_frog_glove_R.glb'), load('prop_frog_crown.glb')]);
    const frog = body.scene;
    const holder = new THREE.Group(); holder.scale.setScalar(K); holder.add(frog); root.add(holder);
    const bones = {}, sockets = {};
    frog.traverse(o => { if (o.isBone) bones[o.name] = o; if (/^(head_top|head_face|hand_[LR]_grip|spine_back|hips_front)$/.test(o.name)) sockets[o.name] = o; });
    root.updateMatrixWorld(true);
    frog.traverse(o => { if (o.isSkinnedMesh) armWeights(o.geometry, o.skeleton); });
    { const m = frog.getObjectByProperty('isSkinnedMesh', true);
      for (const [u, b] of [[armU.shL, bones.LeftArm], [armU.shR, bones.RightArm]]) { u.value.copy(m.worldToLocal(b.getWorldPosition(new THREE.Vector3()))); u.value.x -= Math.sign(u.value.x) * ARM_IN; }
      // o Meshy deixou os ombros tortos (um mais baixo e mais para fora): os dois tocos ficam no espelho da média
      const L = armU.shL.value, R = armU.shR.value, ax = (L.x - R.x) / 2, ay = (L.y + R.y) / 2, az = (L.z + R.z) / 2;
      L.set(ax, ay, az); R.set(-ax, ay, az); }
    toonify(frog);
    frog.traverse(o => { if (o.isSkinnedMesh && !o.userData.hull) armCollapse(o.material); });
    frog.traverse(o => { if (o.isSkinnedMesh && !o.userData.hull && !/belt/.test(o.name) && !/bermuda_(green|side|hem)/.test(o.material.name || '')) bellyShader(o.material); });
    root.updateMatrixWorld(true);
    // cinturão: sai do skinning (que puxava ele junto com as coxas) e vira peça rígida que segue o quadril
    let belt = null;
    frog.traverse(o => { if (o.isSkinnedMesh && !o.userData.hull && /belt/.test(o.name)) belt = belt || o; });
    if (belt) {
      const g = belt.geometry.clone(); g.deleteAttribute('skinIndex'); g.deleteAttribute('skinWeight');
      g.computeBoundingBox(); const c = g.boundingBox.getCenter(new THREE.Vector3());
      g.translate(-c.x, 0, -c.z); g.scale(1.06, 1, 1.06); g.translate(c.x, 0, c.z);   // folga: o corpo dobra por baixo sem furar
      const rigid = new THREE.Mesh(g, belt.material); scene.add(rigid);
      belt.visible = false; if (belt.userData.hullMesh) belt.userData.hullMesh.visible = false;
      rigid.userData.rest = new THREE.Matrix4().copy(bones.Hips.matrixWorld).invert().multiply(belt.matrixWorld);
      rigid.matrixAutoUpdate = false;
      belt = rigid;
      toonify(belt);
    }
    // coroa no topo da cabeça; luvas soltas na cena (seguem a mão do IK)
    const crown = cr.scene; toonify(crown); sockets.head_top.add(crown); crown.position.set(0, -0.02, 0); crown.scale.setScalar(CROWN_SCALE);
    const gloves = { [-1]: gR.scene, [1]: gL.scene };   // -1 = esquerda da tela = mão direita dele (de frente para a câmera)
    for (const g of Object.values(gloves)) { toonify(g); g.traverse(o => { if (o.isMesh && !o.userData.hull) o.material.side = THREE.DoubleSide; }); g.scale.setScalar(GLOVE_SCALE * K); scene.add(g); }

    // --- pose de descanso: tudo volta a ela a cada quadro e a pose do motor entra por cima
    root.updateMatrixWorld(true);
    const rest = new Map(); for (const b of Object.values(bones)) rest.set(b, { q: b.quaternion.clone(), p: b.position.clone() });
    const side = sx => sx < 0 ? 'Right' : 'Left';
    const feetHome = {}, feetQ = {};
    for (const sx of [-1, 1]) { const F = bones[side(sx) + 'Foot']; feetHome[sx] = root.worldToLocal(F.getWorldPosition(new THREE.Vector3())); feetQ[sx] = F.getWorldQuaternion(new THREE.Quaternion()); }
    const HIPS = bones.Hips.getWorldPosition(new THREE.Vector3()).y, NECK = bones.Head.getWorldPosition(new THREE.Vector3()).y;
    const chest = bones.Spine02.getWorldPosition(new THREE.Vector3());

    // --- rosto 2.5D preso no osso da cabeça: olhos em planos, boca e narinas decalcadas na curva da pele
    const fp = new THREE.Group(); bones.Head.add(fp);
    const eyeL = new THREE.Mesh(new THREE.PlaneGeometry(1, 1), planeMat(faceTex.normal.eye)), eyeR = new THREE.Mesh(new THREE.PlaneGeometry(1, 1), planeMat(faceTex.normal.eye));
    const mouth = new THREE.Mesh(new THREE.BufferGeometry(), planeMat(faceTex.normal.mouth)), nose = new THREE.Mesh(new THREE.BufferGeometry(), planeMat(noseTex));
    fp.add(eyeL, eyeR, mouth, nose);
    {
      // cópia estática da cabeça (pose de descanso) para mirar os decalques na pele
      const headRest = bones.Head.matrixWorld.clone(), headRestInv = headRest.clone().invert();
      const headCut = sockets.head_face.getWorldPosition(new THREE.Vector3()).y - 0.5 * K, rayHead = [];
      frog.traverse(m => {
        if (!m.isSkinnedMesh || m.userData.hull || /belt/.test(m.name)) return;
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
      const { eyeX: x, eyeY: y, eyeZ: z, eyeS: s, mouthY: my, mouthLift: lift, mouthW } = FACE;
      for (const [m, dx] of [[eyeL, x], [eyeR, -x]]) { m.position.copy(local(dx, y, z)); m.quaternion.copy(q0); m.scale.set(dx > 0 ? -s : s, s, 1); }   // o da direita da tela é o espelho
      mouth.geometry = decal(local(0, my, 0), s * mouthW, s * mouthW * 0.35, 28, 6, lift);
      nose.geometry = decal(local(0, my + 0.13, 0), s * 0.75, s * 0.75, 8, 8, lift);
    }
    const setFace = f => { const T = faceTex[f] || faceTex.normal; eyeL.material.map = eyeR.material.map = T.eye; mouth.material.map = T.mouth; for (const m of [eyeL, eyeR, mouth]) m.material.needsUpdate = true; };

    // --- o que o motor mexe: grupos "fantasma" (não aparecem) cuja pose vira rotação nos ossos
    const upper = new THREE.Group(), neck = new THREE.Group(), hips = new THREE.Group();
    upper.position.y = HIPS; hips.position.y = HIPS; neck.position.y = NECK - HIPS; upper.add(neck); root.add(upper, hips);
    const hands = { [-1]: new THREE.Object3D(), [1]: new THREE.Object3D() }; root.add(hands[-1], hands[1]);
    const footOff = { [-1]: [0, 0], [1]: [0, 0] };
    const placeFoot = (sx, dx, lift) => { footOff[sx] = [dx, lift]; };
    const tmp = new THREE.Vector3(), pole = new THREE.Vector3();
    const onFrame = c => {
      for (const [b, r] of rest) { b.quaternion.copy(r.q); b.position.copy(r.p); }
      root.updateMatrixWorld(true);
      // quadril: desce/desloca com o tronco e gira (o resto do giro vai para a coluna, a cabeça por último)
      const H = bones.Hips;
      const hw = H.getWorldPosition(new THREE.Vector3()).add(tmp.set(hips.position.x, hips.position.y - HIPS, hips.position.z));
      H.position.copy(H.parent.worldToLocal(hw)); H.updateMatrixWorld(true);
      rotW(H, UP, hips.rotation.y); rotW(H, RIGHT, hips.rotation.x); rotW(H, FWD, hips.rotation.z);
      const sp = ['Spine', 'Spine01', 'Spine02'];
      for (const n of sp) { rotW(bones[n], UP, (upper.rotation.y - hips.rotation.y) / 3); rotW(bones[n], RIGHT, (upper.rotation.x - hips.rotation.x) / 3); rotW(bones[n], FWD, (upper.rotation.z - hips.rotation.z) / 3); }
      rotW(bones.Head, UP, neck.rotation.y); rotW(bones.Head, RIGHT, neck.rotation.x); rotW(bones.Head, FWD, neck.rotation.z);
      // pernas: IK até o pé (plantado na casa dele + o que a passada/o salto pedem), pé reto no chão
      for (const sx of [-1, 1]) {
        const k = side(sx), [dx, lift] = footOff[sx];
        const target = root.localToWorld(feetHome[sx].clone().add(tmp.set(dx, lift, 0)));
        const hip = bones[k + 'UpLeg'].getWorldPosition(new THREE.Vector3());
        pole.copy(hip).add(tmp.set(k === 'Left' ? 0.25 : -0.25, -0.5, 1.0));   // joelho para a frente e para fora
        solveIK(bones[k + 'UpLeg'], bones[k + 'Leg'], bones[k + 'Foot'], target, pole);
        const F = bones[k + 'Foot'], pq = F.parent.getWorldQuaternion(new THREE.Quaternion()).invert();
        F.quaternion.copy(pq.multiply(feetQ[sx])); F.updateMatrixWorld(true);
      }
      // luvas flutuam onde o motor pôs as mãos (como no jogo, sem braço): na guarda, nós dos dedos para cima e palma
      // para ele; no soco, nós para a frente e palma para baixo; giram junto com o tronco
      for (const sx of [-1, 1]) {
        const G = gloves[sx], k = Math.min(1, Math.max(0, c.pose[sx][0] / 0.3));
        G.position.copy(hands[sx].getWorldPosition(new THREE.Vector3()));
        const yq = new THREE.Quaternion().setFromAxisAngle(UP, upper.rotation.y * 0.8);
        const dir = new THREE.Vector3(0, 0.8, 0.6).lerp(new THREE.Vector3(0, -0.1, 1), k).normalize().applyQuaternion(yq);
        const zRef = FWD.clone().negate().lerp(UP.clone().negate(), k).applyQuaternion(yq);
        const zA = zRef.addScaledVector(dir, -zRef.dot(dir)).normalize(), xA = new THREE.Vector3().crossVectors(dir, zA);
        G.quaternion.setFromRotationMatrix(new THREE.Matrix4().makeBasis(xA, dir, zA));
      }
      // cinturão rígido segue o quadril
      if (belt) { belt.matrix.multiplyMatrices(bones.Hips.matrixWorld, belt.userData.rest); belt.matrixWorldNeedsUpdate = true; }
    };
    return { P: { frog, bones, sockets, gloves, crown, belt }, upper, WAIST: HIPS, neck, NECK, hips, HIPS, hands, hipSway: 0.4,
             hand: { x: 0.62 * K, y: chest.y + 0.05 * K, z: chest.z + 0.55 * K, yaw: -0.12 }, placeFoot, setFace, onFrame };
  },
};
