// King Frog vinil — do cinturão para baixo vem do modelo 3D do Meshy (body_frog.glb): o short, as pernas e os pés com
// o esqueleto dele. Só os triângulos abaixo do cinturão ficam; a pintura em retalhos do Meshy sai e cada parte ganha cor
// chapada (short verde-escuro, barra mais escura, pele verde, pés laranja — os pés pelos pesos dos ossos Foot/ToeBase).
// Casca de contorno com o mesmo esqueleto, recuada na linha de visão (sem riscos nas dobras). O quadril segue o grupo
// "hips" do motor e as pernas vão por IK até os pés (plantados, passada, salto).
import { THREE, ramp, INK, lin } from './core.js';
import { GLTFLoader } from 'three/addons/loaders/GLTFLoader.js';
import { mergeVertices } from 'three/addons/utils/BufferGeometryUtils.js';

const K = 0.85;                 // escala do modelo (2,09 m no arquivo) para o porte do vinil
const WIDE = 1.18;              // mais largo e fundo (não mais alto): combina com o tronco cheio de código
const CUT = 0.9;                // corta acima de 90 cm (fica por baixo do cinturão de código, 86 a 101 cm)
const OUTLINE = 0.016, INK_PUSH = 0.04;   // casca recua mais: some nas dobras que sobraram
const COL = { short: lin([0x33 / 255, 0x57 / 255, 0x2f / 255]), hem: lin([0x22 / 255, 0x3b / 255, 0x1f / 255]),
  skin: lin([0x6f / 255, 0x9a / 255, 0x4e / 255]), foot: lin([0xd1 / 255, 0x7b / 255, 0x24 / 255]) };
const UP = new THREE.Vector3(0, 1, 0), RIGHT = new THREE.Vector3(1, 0, 0), FWD = new THREE.Vector3(0, 0, 1);

// --- IK de dois ossos (independe do eixo local dos ossos do Meshy)
const _a = new THREE.Vector3(), _b = new THREE.Vector3(), _c = new THREE.Vector3(), _q = new THREE.Quaternion(), _pq = new THREE.Quaternion();
function rotateBoneWorld(bone, from, to) {
  if (from.lengthSq() < 1e-12 || to.lengthSq() < 1e-12) return;
  _q.setFromUnitVectors(from.clone().normalize(), to.clone().normalize());
  bone.getWorldQuaternion(_pq); const wanted = _q.multiply(_pq);
  bone.parent.getWorldQuaternion(_pq); bone.quaternion.copy(_pq.invert().multiply(wanted)); bone.updateMatrixWorld(true);
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
function rotW(bone, axis, rad) {
  if (!rad) return;
  const pq = bone.parent.getWorldQuaternion(new THREE.Quaternion());
  bone.quaternion.premultiply(pq.clone().invert().multiply(new THREE.Quaternion().setFromAxisAngle(axis, rad)).multiply(pq));
  bone.updateMatrixWorld(true);
}

// alisa a malha do Meshy (dobrinhas e calombos que viravam riscos de contorno e sombra em retalho): filtro de Taubin
// (alisa sem encolher) sobre a malha soldada — o Meshy separa vértices nas costuras, então os vértices são agrupados
// pela posição. A borda aberta (onde esta peça encosta em outra) fica parada, para não abrir fresta. No fim as normais
// saem da malha soldada (lisas também nas costuras).
const SMOOTH_IT = 22;
function smoothMesh(g, tri) {
  const pos = g.attributes.position, n = pos.count, keyOf = new Int32Array(n), keys = new Map(), P = [];
  for (let i = 0; i < n; i++) {
    const k = Math.round(pos.getX(i) * 1e4) + ',' + Math.round(pos.getY(i) * 1e4) + ',' + Math.round(pos.getZ(i) * 1e4);
    let id = keys.get(k); if (id === undefined) { id = P.length; keys.set(k, id); P.push(new THREE.Vector3(pos.getX(i), pos.getY(i), pos.getZ(i))); }
    keyOf[i] = id;
  }
  const N = P.length, nb = Array.from({ length: N }, () => new Set()), edge = new Map();
  for (let t = 0; t < tri.length; t += 3) for (let e = 0; e < 3; e++) {
    const a = keyOf[tri[t + e]], b = keyOf[tri[t + (e + 1) % 3]]; if (a === b) continue;
    nb[a].add(b); nb[b].add(a); const ek = a < b ? a + '_' + b : b + '_' + a; edge.set(ek, (edge.get(ek) || 0) + 1);
  }
  const fixed = new Uint8Array(N); for (const [ek, c] of edge) if (c === 1) { const [a, b] = ek.split('_').map(Number); fixed[a] = fixed[b] = 1; }
  const used = new Uint8Array(N); for (let t = 0; t < tri.length; t++) used[keyOf[tri[t]]] = 1;
  const step = lam => {
    const D = P.map((p, i) => { if (!used[i] || fixed[i] || !nb[i].size) return null; const c = new THREE.Vector3(); for (const j of nb[i]) c.add(P[j]); return c.divideScalar(nb[i].size).sub(p).multiplyScalar(lam); });
    D.forEach((d, i) => { if (d) P[i].add(d); });
  };
  for (let k = 0; k < SMOOTH_IT; k++) { step(0.55); step(-0.58); }
  for (let i = 0; i < n; i++) pos.setXYZ(i, P[keyOf[i]].x, P[keyOf[i]].y, P[keyOf[i]].z);
  pos.needsUpdate = true;
  // normais da malha soldada
  const NR = Array.from({ length: N }, () => new THREE.Vector3()), e1 = new THREE.Vector3(), e2 = new THREE.Vector3();
  for (let t = 0; t < tri.length; t += 3) {
    const a = keyOf[tri[t]], b = keyOf[tri[t + 1]], c = keyOf[tri[t + 2]];
    e1.subVectors(P[b], P[a]); e2.subVectors(P[c], P[a]); const fn = e1.cross(e2);
    NR[a].add(fn); NR[b].add(fn); NR[c].add(fn);
  }
  const nrm = g.attributes.normal; for (let i = 0; i < n; i++) { const v = NR[keyOf[i]].clone().normalize(); nrm.setXYZ(i, v.x, v.y, v.z); }
  nrm.needsUpdate = true;
}

export async function meshyLower({ root, hips, HIPS }) {
  const gl = await new GLTFLoader().loadAsync('../../assets3d/frog/clean/body_frog.glb');
  const frog = gl.scene, holder = new THREE.Group(); holder.scale.set(K * WIDE, K, K * WIDE); holder.add(frog); root.add(holder);
  root.updateMatrixWorld(true);
  const bones = {}; frog.traverse(o => { if (o.isBone) bones[o.name] = o; });
  const meshes = []; frog.traverse(o => { if (o.isSkinnedMesh) meshes.push(o); });
  const v = new THREE.Vector3(); let beltSrc = null;
  for (const o of meshes) {
    if (/belt/.test(o.name)) { beltSrc = o; continue; }                // o cinturão vira peça rígida (abaixo)
    const g = o.geometry, pos = g.attributes.position, si = g.attributes.skinIndex, sw = g.attributes.skinWeight;
    // só os triângulos abaixo do corte (na pose de descanso)
    const idx = g.index.array, keep = [];
    // (sem braços: as mãos do Meshy pendem abaixo do corte e ficavam flutuando)
    const armB = o.skeleton.bones.map(b => /(Arm|ForeArm|Hand)$/.test(b.name));
    const ok = i => { if (v.fromBufferAttribute(pos, i).applyMatrix4(o.matrixWorld).y >= CUT) return false; let w = 0; for (let k = 0; k < 4; k++) if (armB[si.getComponent(i, k)]) w += sw.getComponent(i, k); return w < 0.2; };
    for (let t = 0; t < idx.length; t += 3) if (ok(idx[t]) && ok(idx[t + 1]) && ok(idx[t + 2])) keep.push(idx[t], idx[t + 1], idx[t + 2]);
    g.setIndex(keep);
    smoothMesh(g, keep);
    // cor chapada por parte, decidida por TRIÂNGULO (divisa nítida, sem borrão entre o laranja e o verde): pés pelos
    // pesos dos ossos do pé; a pele que sobra entre o short e o cinturão fica da cor do short
    const mat = o.material.name || '', footB = o.skeleton.bones.map(b => /Foot|ToeBase/.test(b.name));
    const footW = i => { let w = 0; for (let k = 0; k < 4; k++) if (footB[si.getComponent(i, k)]) w += sw.getComponent(i, k); return w; };
    const ys = i => v.fromBufferAttribute(pos, i).applyMatrix4(o.matrixWorld).y;
    const triCol = [];
    for (let t = 0; t < keep.length; t += 3) {
      const [a, b, c] = [keep[t], keep[t + 1], keep[t + 2]];
      let col = COL.skin;
      if (/bermuda_hem/.test(mat)) col = COL.hem;
      else if (/bermuda_(green|side)/.test(mat)) col = COL.short;
      else if ((ys(a) + ys(b) + ys(c)) / 3 < 0.12 || (footW(a) + footW(b) + footW(c)) / 3 > 0.97) col = COL.foot;   // pé: tudo abaixo de 12 cm (divisa reta) e os dedos
      else if ((ys(a) + ys(b) + ys(c)) / 3 > 0.72) col = COL.short;
      triCol.push(col);
    }
    const flatG = g.toNonIndexed(), cols = new Float32Array(flatG.attributes.position.count * 3);
    triCol.forEach((c, t) => { for (let k = 0; k < 3; k++) cols.set(c, (t * 3 + k) * 3); });
    flatG.setAttribute('color', new THREE.BufferAttribute(cols, 3));
    o.geometry = flatG;
    o.material = new THREE.MeshToonMaterial({ vertexColors: true, gradientMap: ramp });
    // casca invertida com o mesmo esqueleto, soldada (o Meshy separa vértices nas costuras) e recuada na linha de visão
    const hm = new THREE.MeshBasicMaterial({ color: INK, side: THREE.BackSide });
    hm.onBeforeCompile = sh => {
      sh.vertexShader = sh.vertexShader
        .replace('#include <project_vertex>', `vec4 mvPosition = modelViewMatrix * vec4(transformed, 1.0); mvPosition.xyz += normalize(mvPosition.xyz) * ${INK_PUSH.toFixed(4)}; gl_Position = projectionMatrix * mvPosition;`)
        .replace('#include <skinning_vertex>', `#include <skinning_vertex>
          transformed += normalize(objectNormal) * ${OUTLINE.toFixed(4)};`);
    };
    const hg = g.clone(); for (const k of Object.keys(hg.attributes)) if (!['position', 'skinIndex', 'skinWeight'].includes(k)) hg.deleteAttribute(k);
    const welded = mergeVertices(hg, 1e-4); welded.computeVertexNormals();
    const hull = new THREE.SkinnedMesh(welded, hm); hull.bind(o.skeleton, o.bindMatrix);
    o.parent.add(hull); hull.position.copy(o.position); hull.quaternion.copy(o.quaternion); hull.scale.copy(o.scale);
  }
  // cinturão do modelo (com a textura e a fivela dele): sai do skinning (puxava junto com as coxas) e vira peça rígida
  // que segue o osso do quadril, folgado e mais alto que o original
  let belt = null;
  if (beltSrc) {
    const g = beltSrc.geometry.clone(); g.deleteAttribute('skinIndex'); g.deleteAttribute('skinWeight');
    g.computeBoundingBox(); const c = g.boundingBox.getCenter(new THREE.Vector3());
    g.translate(-c.x, -c.y, -c.z); g.scale(1.08, 1.2, 1.08); g.translate(c.x, c.y, c.z);   // folgado e mais alto: o tronco inclina por dentro dele sem aparecer
    const map = beltSrc.material.map; if (map) { map.colorSpace = THREE.SRGBColorSpace; map.generateMipmaps = false; map.minFilter = THREE.LinearFilter; map.needsUpdate = true; }
    belt = new THREE.Mesh(g, new THREE.MeshToonMaterial({ map, gradientMap: ramp }));
    const hm = new THREE.MeshBasicMaterial({ color: INK, side: THREE.BackSide });
    hm.onBeforeCompile = sh => { sh.vertexShader = sh.vertexShader.replace('#include <begin_vertex>', `#include <begin_vertex>
      transformed += normal * ${(OUTLINE * 0.7).toFixed(4)};`).replace('#include <project_vertex>', `vec4 mvPosition = modelViewMatrix * vec4(transformed, 1.0); mvPosition.xyz += normalize(mvPosition.xyz) * ${INK_PUSH.toFixed(4)}; gl_Position = projectionMatrix * mvPosition;`); };
    const wg = mergeVertices(g.clone().deleteAttribute('uv').deleteAttribute('normal'), 1e-4); wg.computeVertexNormals();
    belt.add(new THREE.Mesh(wg, hm));
    belt.userData.rest = new THREE.Matrix4().copy(bones.Hips.matrixWorld).invert().multiply(beltSrc.matrixWorld);
    belt.matrixAutoUpdate = false; root.parent.add(belt); beltSrc.visible = false;
  }
  // pose de descanso: tudo volta a ela a cada quadro e a pose do motor entra por cima
  const rest = new Map(); for (const b of Object.values(bones)) rest.set(b, { q: b.quaternion.clone(), p: b.position.clone() });
  const side = sx => sx < 0 ? 'Right' : 'Left';                   // -1 = esquerda da tela = perna direita dele
  const home = {}, footQ = {};
  for (const sx of [-1, 1]) { const F = bones[side(sx) + 'Foot']; home[sx] = root.worldToLocal(F.getWorldPosition(new THREE.Vector3())); footQ[sx] = F.getWorldQuaternion(new THREE.Quaternion()); }
  const off = { [-1]: [0, 0], [1]: [0, 0] };
  const placeFoot = (sx, dx, lift) => { off[sx] = [dx, lift]; };
  const tmp = new THREE.Vector3(), pole = new THREE.Vector3();
  const update = () => {
    for (const [b, r] of rest) { b.quaternion.copy(r.q); b.position.copy(r.p); }
    root.updateMatrixWorld(true);
    // quadril: desce/desloca e gira com o grupo do quadril do motor (o cinturão de código vai junto)
    const H = bones.Hips;
    const hw = H.getWorldPosition(new THREE.Vector3()).add(tmp.set(hips.position.x, hips.position.y - HIPS, hips.position.z));
    H.position.copy(H.parent.worldToLocal(hw)); H.updateMatrixWorld(true);
    rotW(H, UP, hips.rotation.y); rotW(H, RIGHT, hips.rotation.x); rotW(H, FWD, hips.rotation.z);
    // pernas: IK até o pé (na casa dele + o que a passada/o salto pedem), pé reto no chão
    for (const sx of [-1, 1]) {
      const k = side(sx), [dx, lift] = off[sx];
      const target = root.localToWorld(home[sx].clone().add(tmp.set(dx, lift, 0)));
      const hip = bones[k + 'UpLeg'].getWorldPosition(new THREE.Vector3());
      pole.copy(hip).add(tmp.set(k === 'Left' ? 0.25 : -0.25, -0.5, 1.0));   // joelho para a frente e para fora
      solveIK(bones[k + 'UpLeg'], bones[k + 'Leg'], bones[k + 'Foot'], target, pole);
      const F = bones[k + 'Foot'], pq = F.parent.getWorldQuaternion(new THREE.Quaternion()).invert();
      F.quaternion.copy(pq.multiply(footQ[sx])); F.updateMatrixWorld(true);
    }
    if (belt) { belt.matrix.multiplyMatrices(bones.Hips.matrixWorld, belt.userData.rest); belt.matrixWorldNeedsUpdate = true; }
  };
  return { frog, bones, placeFoot, update };
}
