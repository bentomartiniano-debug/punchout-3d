// Ringue de Pernambuco (arena da Valéria) em código, no acabamento do vinil, seguindo o concept
// arena-concepts/pernambuco-kit/01-ringue-plataforma-A30-v2.png: tablado alto de piso creme com a estrela de frevo no
// meio e leques nos cantos, beirada laranja, quatro postes oitavados em faixas, proteções nas cordas, saia de festa
// em panos (vermelho, azul, amarelo, com triângulo de outra cor) sobre a estrutura de madeira com travas em X, e duas
// escadas retas nas quinas da frente. As medidas são as do jogo (engine-cpp/app/ring_renderer.cpp): meia largura
// 270, cordas a 38/68/98, poste até 125, tablado 35 acima do chão; 1 unidade do jogo = UNIT m (ringue de boxe real).
// Cores das cordas e das proteções também vêm do jogo. Piso em y = 0 (onde os lutadores pisam).
import { THREE, flat, withOutline } from './core.js';
import { RoundedBoxGeometry } from 'three/addons/geometries/RoundedBoxGeometry.js';

export const UNIT = 0.0122;
const u = v => v * UNIT;
const HALF = 270, EDGE = 292, APRON = 35, POST_TOP = 125, ROPES = [38, 68, 98];
const C = { cream: '#f3e2bf', creamLine: '#e6cfa3', orange: '#f26a1b', red: '#e2342a', yellow: '#f7c22c', blue: '#1f9fd0', wood: '#7a4a2a', woodDark: '#5a3418',
            ground: '#d7b48c', groundDark: '#c69f77', ropes: ['#f26d56', '#ffe4ab', '#3bbbb2'], pads: ['#f1c650', '#24a9a1', '#ef775f', '#24a9a1'] };

// piso pintado num canvas (cores chapadas: a luz e a retícula vêm do shader)
function floorTexture() {
  const N = 2048, c = document.createElement('canvas'); c.width = c.height = N;
  const g = c.getContext('2d'), k = N / (2 * EDGE), P = v => (v + EDGE) * k;   // unidades do jogo -> px
  g.fillStyle = C.orange; g.fillRect(0, 0, N, N);
  g.fillStyle = C.cream; g.fillRect(P(-HALF + 6), P(-HALF + 6), (2 * HALF - 12) * k, (2 * HALF - 12) * k);
  g.strokeStyle = C.creamLine; g.lineWidth = 2.5 * k; g.strokeRect(P(-HALF + 16), P(-HALF + 16), (2 * HALF - 32) * k, (2 * HALF - 32) * k);
  const wedges = (cx, cy, r, a0, a1, n, cols) => { for (let i = 0; i < n; i++) { const a = a0 + (a1 - a0) * i / n, b = a0 + (a1 - a0) * (i + 1) / n;
    g.fillStyle = cols[i % cols.length]; g.beginPath(); g.moveTo(P(cx), P(cy)); g.arc(P(cx), P(cy), r * k, a, b); g.closePath(); g.fill(); } };
  // estrela de frevo: 16 gomos em pétala (ponta reta), miolo creme e centro vermelho
  { const R = 112, n = 16, cols = [C.red, C.yellow, C.blue, C.yellow];
    for (let i = 0; i < n; i++) { const a = i / n * Math.PI * 2, b = (i + 1) / n * Math.PI * 2, m = (a + b) / 2;
      g.fillStyle = cols[i % 4]; g.beginPath(); g.moveTo(P(0), P(0));
      g.lineTo(P(Math.cos(a) * R * 0.93), P(Math.sin(a) * R * 0.93)); g.lineTo(P(Math.cos(m) * R), P(Math.sin(m) * R)); g.lineTo(P(Math.cos(b) * R * 0.93), P(Math.sin(b) * R * 0.93)); g.closePath(); g.fill(); }
    g.fillStyle = C.cream; g.beginPath(); g.arc(P(0), P(0), 22 * k, 0, 7); g.fill();
    g.strokeStyle = C.blue; g.lineWidth = 4 * k; g.beginPath(); g.arc(P(0), P(0), 17 * k, 0, 7); g.stroke();
    g.fillStyle = C.red; g.beginPath(); g.arc(P(0), P(0), 11 * k, 0, 7); g.fill(); }
  // leques nos cantos (quarto de sol virado para o meio)
  for (const [sx, sy] of [[-1, -1], [1, -1], [1, 1], [-1, 1]]) {
    const cx = sx * (HALF - 22), cy = sy * (HALF - 22), base = Math.atan2(-sy, -sx);
    wedges(cx, cy, 58, base - Math.PI / 4, base + Math.PI / 4, 8, [C.red, C.yellow, C.blue, C.yellow]);
    g.fillStyle = C.cream; g.beginPath(); g.moveTo(P(cx), P(cy)); g.arc(P(cx), P(cy), 20 * k, base - Math.PI / 4, base + Math.PI / 4); g.closePath(); g.fill();
  }
  const t = new THREE.CanvasTexture(c); t.colorSpace = THREE.SRGBColorSpace; t.anisotropy = 8; return t;
}

export function buildPernambucoRing() {
  const ring = new THREE.Group(), ol = (m, w = 0.008) => { const g = withOutline(m, w); g.position.copy(m.position); g.quaternion.copy(m.quaternion); m.position.set(0, 0, 0); m.quaternion.identity(); return g; };
  const box = (w, h, d, x, y, z, col, r = 1.5) => { const m = new THREE.Mesh(new RoundedBoxGeometry(u(w), u(h), u(d), 2, u(r)), flat(col)); m.position.set(u(x), u(y), u(z)); return m; };

  // chão em volta (calçamento) no nível de baixo do tablado
  { const g = new THREE.PlaneGeometry(u(2800), u(2260)); g.rotateX(-Math.PI / 2); g.translate(0, 0, u(370)); const m = flat(C.ground);   // praça até a ponte; atrás dela, o mar
    m.onBeforeCompile = sh => {
      sh.vertexShader = 'varying vec2 vG;\n' + sh.vertexShader.replace('#include <begin_vertex>', '#include <begin_vertex>\nvG = position.xz;');
      sh.fragmentShader = 'uniform vec3 uDark;\nvarying vec2 vG;\n' + sh.fragmentShader.replace('#include <map_fragment>', `#include <map_fragment>
        { vec2 q = vG / ${u(26).toFixed(4)}; q.x += 0.5 * mod(floor(q.y), 2.0); vec2 f = abs(fract(q) - 0.5);
          if (max(f.x, f.y) > 0.46) diffuseColor.rgb = uDark; }`);
      sh.uniforms.uDark = { value: new THREE.Color(C.groundDark) };
    };
    m.customProgramCacheKey = () => 'calcamento';
    const gr = new THREE.Mesh(g, m); gr.position.y = u(-APRON); ring.add(gr); }

  // tampo: piso pintado, beirada laranja grossa
  { const top = new THREE.Mesh(new THREE.BoxGeometry(u(2 * EDGE), u(6), u(2 * EDGE)), [flat(C.orange), flat(C.orange), new THREE.MeshToonMaterial({ map: floorTexture(), gradientMap: flat(C.cream).gradientMap }), flat(C.orange), flat(C.orange), flat(C.orange)]);
    top.position.y = u(-3); ring.add(ol(top, 0.007)); }

  // estrutura de madeira por baixo: vigas na base, pilares e travas em X entre eles
  const SIDES = [[0, 1], [1, 0], [0, -1], [-1, 0]];   // frente, direita, trás, esquerda (normal da face)
  for (const [nx, nz] of SIDES) {
    const along = new THREE.Vector3(nz, 0, -nx), face = EDGE - 10;
    const beam = box(2 * EDGE - 24, 6, 8, nx * face, -APRON + 3, nz * face, C.woodDark, 1); beam.rotation.y = Math.atan2(along.x, along.z) - Math.PI / 2; ring.add(ol(beam, 0.005));
    const N = 5, span = (2 * EDGE - 24) / N;
    for (let i = 0; i <= N; i++) { const s = -EDGE + 12 + i * span; const p = box(7, APRON - 6, 7, nx * face + along.x * s, -APRON / 2 - 3, nz * face + along.z * s, C.wood, 1); ring.add(ol(p, 0.004)); }
    for (let i = 0; i < N; i++) for (const d of [-1, 1]) {
      const s = -EDGE + 12 + (i + 0.5) * span, len = Math.hypot(span, APRON - 8);
      const x = box(len, 4, 4, nx * face + along.x * s, -APRON / 2 - 3, nz * face + along.z * s, C.wood, 1);
      x.quaternion.setFromRotationMatrix(new THREE.Matrix4().makeBasis(along.clone().multiplyScalar(1).applyAxisAngle(new THREE.Vector3(nx, 0, nz), d * Math.atan2(APRON - 8, span)), new THREE.Vector3(0, 1, 0).applyAxisAngle(new THREE.Vector3(nx, 0, nz), d * Math.atan2(APRON - 8, span)), new THREE.Vector3(nx, 0, nz)));
      ring.add(ol(x, 0.004));
    }
    // saia de festa: panos em meia-lua pendurados da beirada, cores em rodízio, com um triângulo de outra cor
    const cols = [C.red, C.blue, C.yellow], tri = [C.yellow, C.yellow, C.blue];
    const NS = 6, w = (2 * EDGE) / NS;
    for (let i = 0; i < NS; i++) {
      const s = -EDGE + (i + 0.5) * w, k = (i + (nx + 2 * nz + 4)) % 3, pos = new THREE.Vector3(nx * (EDGE + 0.6) + along.x * s, u(-6) / UNIT, nz * (EDGE + 0.6) + along.z * s);
      const sh = new THREE.Shape(); sh.moveTo(u(-w / 2), 0); sh.lineTo(u(w / 2), 0); sh.absarc(0, 0, u(w / 2), 0, -Math.PI, true);
      const pg = new THREE.ExtrudeGeometry(sh, { depth: u(1.2), bevelEnabled: false, curveSegments: 20 }); pg.scale(1, (APRON - 12) / (w / 2), 1);
      const panel = new THREE.Mesh(pg, flat(cols[k])); panel.position.set(u(pos.x), u(pos.y), u(pos.z));
      panel.quaternion.setFromRotationMatrix(new THREE.Matrix4().makeBasis(along, new THREE.Vector3(0, 1, 0), new THREE.Vector3(nx, 0, nz)));
      const pgrp = ol(panel, 0.005); ring.add(pgrp);
      const ts = new THREE.Shape(); ts.moveTo(u(-w * 0.17), u(-1)); ts.lineTo(u(w * 0.17), u(-1)); ts.lineTo(0, u(-(APRON - 14))); ts.closePath();
      const tg = new THREE.ExtrudeGeometry(ts, { depth: u(0.6), bevelEnabled: false }); const tm = new THREE.Mesh(tg, flat(tri[k]));
      tm.position.copy(pgrp.position).addScaledVector(new THREE.Vector3(nx, 0, nz), u(1.4)); tm.quaternion.copy(pgrp.quaternion); ring.add(tm);   // o contorno levou a posição do pano para o grupo
    }
  }

  // postes oitavados em faixas (do chão até o topo), base de madeira, proteções nas alturas das cordas
  const BANDS = [[C.blue, 0.16], [C.yellow, 0.18], [C.red, 0.18], [C.yellow, 0.16], [C.red, 0.16], [C.blue, 0.16]];
  const corners = [[-HALF, HALF], [HALF, HALF], [HALF, -HALF], [-HALF, -HALF]];
  corners.forEach(([cx, cz], ci) => {
    ring.add(ol(box(34, 14, 34, cx, -APRON + 7, cz, C.woodDark, 2), 0.006));
    let y = -APRON + 14; const H = POST_TOP - y;
    for (const [col, f] of BANDS) { const h = H * f; const g = new THREE.CylinderGeometry(u(11), u(11), u(h), 8); const m = new THREE.Mesh(g, flat(col)); m.position.set(u(cx), u(y + h / 2), u(cz)); m.rotation.y = Math.PI / 8; ring.add(ol(m, 0.006)); y += h; }
    const toC = new THREE.Vector3(-cx, 0, -cz).normalize();
    ROPES.forEach(h => { const p = box(16, 16, 14, cx + toC.x * 13, h, cz + toC.z * 13, C.pads[ci], 4); p.rotation.y = Math.atan2(toC.x, toC.z); ring.add(ol(p, 0.005)); });
  });
  // cordas: três por lado, entre as proteções, cores do jogo
  for (let i = 0; i < 4; i++) {
    const [ax, az] = corners[i], [bx, bz] = corners[(i + 1) % 4];
    ROPES.forEach((h, r) => {
      const a = new THREE.Vector3(u(ax), u(h), u(az)), b = new THREE.Vector3(u(bx), u(h), u(bz)), len = a.distanceTo(b);
      const g = new THREE.CylinderGeometry(u(2.4), u(2.4), len, 10); g.rotateZ(Math.PI / 2);
      const m = new THREE.Mesh(g, flat(C.ropes[r])); m.name = 'rope'; m.position.copy(a).add(b).multiplyScalar(0.5); m.rotation.y = -Math.atan2(b.z - a.z, b.x - a.x); ring.add(ol(m, 0.004));
    });
  }
  // escadas retas nas duas quinas da frente, apontando para a frente (mesmo eixo)
  for (const sx of [-1, 1]) for (let k = 0; k < 3; k++) {
    const h = APRON * (3 - k) / 3, d = 22;
    ring.add(ol(box(64, h, d, sx * (HALF - 50), -APRON + h / 2, EDGE + d / 2 + k * d - 2, k % 2 ? C.wood : C.woodDark, 1.5), 0.006));
  }
  return ring;
}
