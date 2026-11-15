// Batalhator 2000 no modo vinil (rascunho da fase 1), modelado em código na proporção do sprite de frente do jogo
// (quadro de 480 px): o robô. Cabeça em casinha com visor vermelho e duas antenas; barra dos ombros com as brocas
// listradas apontando para cima e para fora; peitoral cinza em escudo com placas de ferrugem; anel listrado na cintura;
// quadril em trapézio com painel e etiqueta dourada; pernas em bloco com faixa dourada. Nas costas: painel com tela e
// painel com o disco dourado. Sem braços (como no jogo): as serras flutuam e giram. Medidas em px; S px = 1 m.
import { THREE, ss, flat, withOutline, legPlacer, loadSprite } from './core.js';
import { RoundedBoxGeometry } from 'three/addons/geometries/RoundedBoxGeometry.js';
import { VALERIA, bodyOpts, valeriaBody } from './base.js';

const S_PX = 245, FOOT = 455, CX = 240;
const C = { steel: '#5d6b80', steelLight: '#7f8ea3', navy: '#2a3446', navyDark: '#1c2433', ink: '#0d0f16', red: '#e8173f', white: '#f2f4f8',
            yellow: '#f5b301', rust: '#7a3f12', saw: '#5f6d82', sawDark: '#2f3a4c', sawLight: '#dfe5ee' };
const col = h => ({ value: new THREE.Color(h) });

export default {
  title: 'Batalhator 2000',
  arena: 'batalhator',   // abre na arena própria (core.js)
  units: [S_PX, FOOT, CX],
  faces: [['normal', 'Normal'], ['angry', 'Raiva'], ['pain', 'Dor'], ['ko', 'K.O.']],
  skills: [['signature', 'Assinatura'], ['saw', 'Vórtice de Serras'], ['overclock', 'Sobrecarga de Reator'], ['emp', 'Descarga PEM'], ['missiles', 'Barragem de Mísseis'], ['holo', 'Combo Holográfico'], ['apocalypse', 'Raio Apocalíptico']],
  others: [['valeria', 'Valéria'], ['frog', 'King Frog'], ['cremoso', 'Cremoso']],
  hudExtra: 'Forma: <button data-form="1">Fase 1</button><button data-form="2">Fase 2</button><button data-form="3">Fase 3</button>',
  async build({ root, U }) {
    const P = {}, spr = await loadSprite('boss', 480);
    const X = x => (x - CX) / S_PX, Y = y => (FOOT - y) / S_PX, D = v => v / S_PX;
    const V = (x, y, z) => new THREE.Vector3(X(x), Y(y), D(z));
    const ol = (m, w = 0.009) => {
      const g = withOutline(m, w); g.position.copy(m.position); g.quaternion.copy(m.quaternion); g.scale.copy(m.scale);
      m.position.set(0, 0, 0); m.quaternion.identity(); m.scale.set(1, 1, 1); return g; };
    // prisma de um contorno (px do quadro, y para baixo) com profundidade d (px), centrado em z = cz, cantos chanfrados
    const prism = (pts, d, cz, color, bevel = 3) => {
      const s = new THREE.Shape(pts.map(([x, y]) => new THREE.Vector2(X(x), Y(y))));
      const g = new THREE.ExtrudeGeometry(s, { depth: D(d - 2 * bevel), bevelEnabled: true, bevelThickness: D(bevel), bevelSize: D(bevel), bevelSegments: 2, curveSegments: 4 });
      g.translate(0, 0, D(cz - d / 2 + bevel));
      return ol(new THREE.Mesh(g, typeof color === 'string' ? flat(color) : color), 0.010);
    };
    const box = (w, h, d, x, y, z, color, r = 4) => { const m = new THREE.Mesh(new RoundedBoxGeometry(D(w), D(h), D(d), 3, D(r)), flat(color)); m.position.copy(V(x, y, z)); return m; };

    // ================= grupos do motor
    const WAIST = Y(305), NECK = Y(186), HIPS = Y(312);
    const upper = new THREE.Group(); upper.position.y = WAIST; root.add(upper);
    const body = new THREE.Group(); body.position.y = -WAIST; upper.add(body);
    const neck = new THREE.Group(); neck.position.y = NECK - WAIST; upper.add(neck);
    const head = new THREE.Group(); head.position.y = -NECK; neck.add(head);
    // formas: 1 = robô inteiro, 2 = robô avariado com o piloto, 3 = o nerd. R12 = o que as duas primeiras dividem
    const F1 = { head: new THREE.Group(), chest: new THREE.Group(), pelvis: new THREE.Group() }, R12 = { body: new THREE.Group(), low: new THREE.Group() };
    head.add(F1.head); body.add(R12.body); R12.body.add(F1.chest);

    // ================= cabeça: casinha de aço, visor vermelho com riscos, antenas
    F1.head.add(prism([[179, 182], [179, 127], [240, 87], [301, 127], [301, 182]], 96, -4, C.steel));
    F1.head.add(prism([[190, 178], [190, 134], [240, 102], [290, 134], [290, 178]], 6, 45, C.steelLight, 1));   // placa clara do rosto (como no sprite)
    const visor = box(100, 28, 8, 240, 158, 50, C.red, 5); F1.head.add(ol(visor, 0.007));
    for (const x of [212, 236, 260]) { const s = new THREE.Mesh(new THREE.BoxGeometry(D(4), D(20), D(2)), flat(C.white)); s.position.copy(V(x, 158, 54.5)); s.rotation.z = -0.6; F1.head.add(s); }
    F1.head.add(ol(box(116, 11, 20, 240, 140, 50, C.navyDark, 3), 0.006));   // capô escuro que avança sobre o visor
    F1.head.add(ol(box(50, 9, 8, 240, 177, 47, C.navyDark, 2), 0.004));     // grade do queixo
    for (const s of [-1, 1]) {   // cubos laterais (as orelhas do robô), com o miolo amarelo
      const e = new THREE.Mesh(new THREE.CylinderGeometry(D(16), D(16), D(16), 22), flat(C.navyDark)); e.rotation.z = Math.PI / 2; e.position.copy(V(240 + s * 66, 152, -4)); F1.head.add(ol(e, 0.006));
      const c = new THREE.Mesh(new THREE.CylinderGeometry(D(7), D(7), D(19), 16), flat(C.yellow)); c.rotation.z = Math.PI / 2; c.position.copy(V(240 + s * 66, 152, -4)); F1.head.add(c);
      const b = new THREE.Mesh(new THREE.CylinderGeometry(D(7), D(9), D(10), 12), flat(C.navyDark)); b.position.copy(V(240 + s * 34, 104, -4)); b.rotation.z = -s * 0.32; F1.head.add(ol(b, 0.004)); }   // pé da antena
    for (const s of [-1, 1]) {
      const st = new THREE.CylinderGeometry(D(3), D(3), D(52), 10); const m = new THREE.Mesh(st, flat(C.ink)); m.position.copy(V(240 + s * 40, 84, -4)); m.rotation.z = -s * 0.32; F1.head.add(m);
      const b = new THREE.Mesh(new THREE.SphereGeometry(D(8), 16, 12), flat(C.red)); b.position.copy(V(240 + s * 53, 58, -4)); F1.head.add(ol(b, 0.005));
    }
    // emotes do visor: raiva = visor mais baixo e inclinado; dor/K.O. = visor apagado
    const setFace = f => { if (P.nerdFace) P.nerdFace(f); visor.parent.rotation.z = f === 'angry' ? 0.05 : 0; visor.material.color.set(f === 'ko' ? '#4a1020' : f === 'pain' ? '#ff6a3d' : C.red); };

    // ================= tronco: barra dos ombros, peitoral em escudo, ferrugem, brocas, anel da cintura
    R12.body.add(ol(box(156, 16, 92, 237, 194, -4, C.ink, 4), 0.006));
    R12.body.add(prism([[173, 200], [307, 200], [300, 262], [240, 296], [180, 262]], 80, -2, C.steel));
    F1.chest.add(prism([[192, 206], [288, 206], [284, 254], [240, 280], [196, 254]], 6, 39, C.steelLight, 1));   // placa clara da frente
    for (const [x, y, rz] of [[186, 216, 0.4], [294, 238, -0.3]]) { const r = box(22, 18, 6, x, y, 39, C.rust, 2); r.rotation.z = rz; R12.body.add(ol(r, 0.005)); }
    // costas: painel com tela de linhas
    R12.body.add(ol(box(104, 58, 6, 240, 228, -42, C.navyDark, 3), 0.006));
    for (const y of [220, 234]) { const l = new THREE.Mesh(new THREE.BoxGeometry(D(80), D(2.5), D(2)), flat(C.white)); l.position.copy(V(240, y, -45.5)); R12.body.add(l); }
    // brocas: cones de faixas pretas e amarelas saindo dos ombros, para cima e para fora
    for (const s of [-1, 1]) {
      const L = 132, g = new THREE.ConeGeometry(D(31), D(L), 36, 24, false); g.translate(0, D(L / 2), 0);
      const m = flat(C.yellow);
      m.onBeforeCompile = sh => {
        sh.vertexShader = 'varying float vH;\n' + sh.vertexShader.replace('#include <begin_vertex>', `#include <begin_vertex>\nvH = position.y * ${S_PX.toFixed(1)} + atan(position.z, position.x) * 6.366;   // espiral fecha a volta sem emenda (40 px por volta: faixas largas)`);
        sh.fragmentShader = 'varying float vH;\n' + sh.fragmentShader.replace('#include <map_fragment>', '#include <map_fragment>\nif (fract(vH / 40.0) < 0.5) diffuseColor.rgb = vec3(0.05, 0.06, 0.09);');
      };
      m.customProgramCacheKey = () => 'drill';
      const dr = new THREE.Mesh(g, m); dr.position.copy(V(240 + s * 80, 214, -4));
      dr.quaternion.setFromUnitVectors(new THREE.Vector3(0, 1, 0), new THREE.Vector3(s * 0.55, 0.8, 0).normalize());
      R12.body.add(ol(dr, 0.008));
      // ombreira: bloco arredondado de onde a broca sai, colar de aço e anel amarelo
      const ax = new THREE.Vector3(s * 0.55, 0.8, 0).normalize(), UP0 = new THREE.Vector3(0, 1, 0);
      const pd = new THREE.Mesh(new THREE.SphereGeometry(1, 28, 18), flat(C.navy)); pd.scale.set(D(42), D(30), D(46)); pd.position.copy(V(240 + s * 86, 210, -4)); R12.body.add(ol(pd, 0.009));
      const col = new THREE.Mesh(new THREE.CylinderGeometry(D(33), D(37), D(18), 30), flat(C.steel)); col.position.copy(V(240 + s * 80, 214, -4)).addScaledVector(ax, D(9)); col.quaternion.setFromUnitVectors(UP0, ax); R12.body.add(ol(col, 0.007));
      const bd = new THREE.Mesh(new THREE.CylinderGeometry(D(33.5), D(34.5), D(5), 30), flat(C.yellow)); bd.position.copy(V(240 + s * 80, 214, -4)).addScaledVector(ax, D(20)); bd.quaternion.setFromUnitVectors(UP0, ax); R12.body.add(bd);
      // costelas: três placas escuras em degrau no flanco, embaixo da ombreira
      for (let i = 0; i < 3; i++) R12.body.add(ol(box(24 - i * 3, 11, 64 - i * 6, 240 + s * (72 - i * 5), 236 + i * 15, -2, C.navyDark, 3), 0.005));
    }
    R12.body.add(ol(box(62, 12, 58, 240, 187, -4, C.steel, 4), 0.006));   // pescoço: anel de aço entre a cabeça e os ombros
    { const g = new THREE.CylinderGeometry(D(52), D(52), D(22), 40); const m = flat(C.navyDark);
      m.onBeforeCompile = sh => {
        sh.vertexShader = 'varying vec3 vP;\n' + sh.vertexShader.replace('#include <begin_vertex>', '#include <begin_vertex>\nvP = position;');
        sh.fragmentShader = 'varying vec3 vP;\n' + sh.fragmentShader.replace('#include <map_fragment>', `#include <map_fragment>
          { float a = atan(vP.z, vP.x) * 6.0; float k = fract(a / 3.14159); if (k < 0.18) diffuseColor.rgb = vec3(0.95); else if (k > 0.5 && k < 0.62) diffuseColor.rgb = vec3(0.96, 0.7, 0.0); }`);
      };
      m.customProgramCacheKey = () => 'ring';
      const r = new THREE.Mesh(g, m); r.scale.z = 0.8; r.position.copy(V(240, 300, -4)); R12.body.add(ol(r, 0.007)); }

    // ================= quadril: trapézio com painel em escudo e etiqueta dourada; nas costas o disco dourado
    const hips = new THREE.Group(); hips.position.y = HIPS; root.add(hips);
    const low = new THREE.Group(); low.position.y = -HIPS; hips.add(low); low.add(R12.low); R12.low.add(F1.pelvis);
    R12.low.add(prism([[140, 308], [340, 308], [318, 404], [163, 404]], 100, -4, C.navy));
    F1.pelvis.add(prism([[172, 318], [308, 318], [302, 378], [240, 400], [178, 378]], 6, 47, C.navyDark, 1));
    F1.pelvis.add(ol(box(34, 14, 4, 266, 356, 51, C.yellow, 3), 0.005));
    { const dk = new THREE.Mesh(new THREE.BoxGeometry(D(6), D(4), D(2)), flat(C.ink)); dk.position.copy(V(256, 356, 53.5)); F1.pelvis.add(dk); }
    R12.low.add(ol(box(120, 46, 6, 240, 350, -55, C.navyDark, 3), 0.006));
    for (const s of [-1, 1]) {   // juntas do quadril: tambor de aço com o miolo amarelo, de onde a perna sai
      const j = new THREE.Mesh(new THREE.CylinderGeometry(D(25), D(25), D(22), 26), flat(C.steel)); j.rotation.z = Math.PI / 2; j.position.copy(V(240 + s * 100, 372, -4)); R12.low.add(ol(j, 0.007));
      const h = new THREE.Mesh(new THREE.CylinderGeometry(D(10), D(10), D(25), 16), flat(C.yellow)); h.rotation.z = Math.PI / 2; h.position.copy(V(240 + s * 100, 372, -4)); R12.low.add(h); }
    { const d = new THREE.Mesh(new THREE.CylinderGeometry(D(12), D(12), D(3), 24), flat(C.yellow)); d.rotation.x = Math.PI / 2; d.position.copy(V(240, 350, -59)); R12.low.add(ol(d, 0.004)); }

    // ================= pernas em bloco com faixa dourada (o pé é a parte de baixo do bloco)
    const LEG_TOP = 380, ANK = 413, LXp = { [-1]: 170, [1]: 310 }, legs = {}, feet = {};
    const legPlace = {};
    for (const s of [-1, 1]) {
      const x = LXp[s];
      const leg = new THREE.Group(); leg.position.copy(V(x, LEG_TOP, 0)); root.add(leg);
      const lb = new THREE.Mesh(new RoundedBoxGeometry(D(64), D(40), D(78), 3, D(5)), flat(C.navy)); lb.position.set(0, D(-20), 0); leg.add(ol(lb, 0.009));
      const kp = new THREE.Mesh(new RoundedBoxGeometry(D(48), D(30), D(12), 3, D(4)), flat(C.steel)); kp.position.set(0, D(-20), D(41)); leg.add(ol(kp, 0.006));   // joelheira
      legs[s] = leg;
      const f = new THREE.Group(); f.position.copy(V(x, ANK, 0)); root.add(f);
      const fb = new THREE.Mesh(new RoundedBoxGeometry(D(70), D(42), D(88), 3, D(5)), flat(C.navy)); fb.position.set(0, D(-21), D(4)); f.add(ol(fb, 0.009));
      const st = new THREE.Mesh(new THREE.BoxGeometry(D(71), D(4), D(89)), flat(C.yellow)); st.position.set(0, D(-2), D(4)); f.add(st);
      const tc = new THREE.Mesh(new RoundedBoxGeometry(D(74), D(24), D(26), 3, D(5)), flat(C.steel)); tc.position.set(0, D(-14), D(50)); f.add(ol(tc, 0.007));   // biqueira
      feet[s] = f; legPlace[s] = { leg: leg.position.clone(), foot: f.position.clone() };
    }
    // pernas de robô: o bloco de cima segue o quadril, o pé vai para onde a passada pede (sem esticar)
    const placeFoot = (sx, dx, lift) => {
      const p = legPlace[sx]; feet[sx].position.set(p.foot.x + dx, p.foot.y + lift, p.foot.z);
      const a = new THREE.Vector3(p.leg.x, p.leg.y - HIPS, 0).applyMatrix4(hips.matrix);
      legs[sx].position.set(a.x + dx * 0.5, Math.max(a.y, feet[sx].position.y + D(36)), a.z);
    };

    // ================= serras: disco de dentes, anéis escuros, cubo amarelo, faixa de brilho; giram. Y = para onde o golpe vai
    function saw() {
      const g = new THREE.Group(), spin = new THREE.Group(); g.add(spin);
      const R = 46, N = 14, s = new THREE.Shape();
      for (let k = 0; k < N * 2; k++) { const r = D(k % 2 ? R * 0.8 : R), a = k / (N * 2) * Math.PI * 2; if (k) s.lineTo(r * Math.cos(a), r * Math.sin(a)); else s.moveTo(r * Math.cos(a), r * Math.sin(a)); }
      const tg = new THREE.ExtrudeGeometry(s, { depth: D(5), bevelEnabled: false }); tg.translate(0, 0, D(-2.5)); tg.rotateX(-Math.PI / 2);
      spin.add(ol(new THREE.Mesh(tg, flat(C.sawLight)), 0.007));
      for (const [r, h, c] of [[33, 8, C.saw], [22, 10, C.sawDark], [12, 12, C.navyDark], [6, 14, C.yellow]]) { const d = new THREE.Mesh(new THREE.CylinderGeometry(D(r), D(r), D(h), 40), flat(c)); spin.add(r > 20 ? ol(d, 0.005) : d); }
      const hl = new THREE.Mesh(new THREE.BoxGeometry(D(60), D(9), D(9)), new THREE.MeshBasicMaterial({ color: C.white, transparent: true, opacity: 0.45 })); hl.rotation.y = 0.7; hl.position.y = D(4.5); spin.add(hl);
      const sock = new THREE.Mesh(new THREE.CylinderGeometry(D(18), D(20), D(16), 24), flat(C.steel)); sock.position.y = D(-14); g.add(ol(sock, 0.006));
      g.userData.spin = spin; return g;
    }
    P.saws = { [-1]: saw(), [1]: saw() }; root.add(P.saws[-1], P.saws[1]);
    const hands = { [-1]: new THREE.Object3D(), [1]: new THREE.Object3D() }; root.add(hands[-1], hands[1]);
    const UPV = new THREE.Vector3(0, 1, 0);

    // ================= fase 2: robô avariado. Sem a cabeça: uma serra no lugar; peito aberto com os fios; fita cinza
    // remendando as brocas, o peito, o quadril e as pernas; curativo na broca; o piloto nerd na cabine do quadril
    const F2 = { head: new THREE.Group(), body: new THREE.Group(), low: new THREE.Group() }; head.add(F2.head); R12.body.add(F2.body); R12.low.add(F2.low);
    { const ts = saw(); ts.rotation.x = Math.PI / 2; ts.position.copy(V(240, 140, -2)); ts.scale.setScalar(1.15); F2.head.add(ts); F2.topSaw = ts;
      F2.head.add(ol(box(40, 26, 40, 240, 180, -4, C.navyDark, 4), 0.007));   // toco do pescoço
      F2.body.add(prism([[196, 206], [284, 206], [280, 250], [240, 272], [200, 250]], 6, 39, '#141a26', 1));   // peito aberto
      const wire = (pts, c) => { const curve = new THREE.CatmullRomCurve3(pts.map(p => V(...p))); F2.body.add(new THREE.Mesh(new THREE.TubeGeometry(curve, 48, D(2.6), 8), flat(c))); };
      wire([[226, 176, 30], [212, 205, 40], [236, 228, 44], [214, 252, 42]], '#e8173f');
      wire([[250, 176, 30], [262, 210, 44], [232, 232, 46], [248, 256, 42]], '#19d3c5');
      wire([[244, 180, 28], [270, 220, 42], [272, 250, 40]], '#2a8cff');
      wire([[234, 182, 28], [248, 214, 46], [226, 246, 44]], '#e8173f');
      const tape = (grp, x, y, z, w, rz) => { const t = box(w, 14, 5, x, y, z, '#9aa6b8', 2); t.rotation.z = rz; grp.add(ol(t, 0.005)); const l = new THREE.Mesh(new THREE.BoxGeometry(D(w - 10), D(2), D(1)), flat(C.white)); l.position.copy(V(x, y - 2, z + 3)); l.rotation.z = rz; grp.add(l); };
      tape(F2.body, 128, 168, 26, 56, 0.3); tape(F2.body, 196, 258, 42, 60, 0.18);
      for (const rz of [0.7, -0.7]) { const b = box(34, 7, 4, 348, 176, 24, C.rust, 2); b.rotation.z = rz; F2.body.add(ol(b, 0.004)); }   // curativo na broca
      tape(F2.low, 222, 352, 52, 56, 0.12); tape(F2.low, 268, 360, 52, 58, -0.15);
      // cabine: moldura ciano em escudo e o piloto (desenho do sprite) atrás do vidro
      F2.low.add(prism([[170, 330], [310, 330], [304, 392], [240, 412], [176, 392]], 6, 30, '#19d3c5', 1));   // moldura do buraco, atrás do nerd
      F2.cockpit = true; }
    const F2legs = []; for (const s of [-1, 1]) { const t = new THREE.Group(); const b = box(56, 14, 5, 0, 0, 0, '#9aa6b8', 2); b.position.set(0, 0, 0); t.add(ol(b, 0.005)); t.position.set(0, D(-14), D(46)); t.rotation.z = s * 0.3; feet[s].add(t); F2legs.push(t); }

    // ================= fase 3: o nerd (cabeção, fone vermelho, óculos, camisa verde com o crachá, jeans, tênis vermelho)
    const N3 = { head: new THREE.Group(), body: new THREE.Group(), low: new THREE.Group() }; head.add(N3.head); body.add(N3.body); low.add(N3.low);
    const NC = { skin: '#ffd9c0', hair: '#4a2a14', red: '#e8203a', redDark: '#8c1020', blue: '#3ab4ff', shirt: '#2a9d8a', shirtDark: '#1e5a46', jeans: '#1f3557', white: '#f4f4f2', grey: '#cfd8e2', badge: '#1f4a3a' };
    const NV = (x, y, z) => V(x, y - 8, z);   // o nerd pisa um pouco mais baixo no quadro
    const HA3 = y => 86 - 22 * ss(206, 232, y);
    // o nerd no corpo da Valéria (base.js), cabeçudo e largo: cabeça de ovo, pescoço abrindo nos ombros, tronco da
    // camisa, pernas do jeans e o sapato da Valéria como tênis vermelho de sola branca (medidas já 8 px mais baixas)
    const NB = valeriaBody(U, bodyOpts(VALERIA, { cx: CX, skin: NC.shirt, legCol: NC.jeans, face: NC.skin, neck: '#efbfa2',
      head: { top: 145, bot: 222, a: 84, d: 0.72, cz: 0 }, yoke: { y0: 212, y1: 236, a0: 30, a1: 76, d: 0.6, cz: -4, end: 244 },
      torso: { y0: 222, y1: 336, a: 80, d: 0.6, cz: -4 }, legs: { dx: 42, top: 352, bot: 446, r: 21, d: 0.85 },
      shoe: { k: 1.15, toe: 20, up: NC.red, sole: NC.white, rim: NC.red, buckle: null, foot: FOOT } }));
    { N3.head.add(NB.face);   // cabeça = ovo da base
      // cabelo: touca arredondada no alto (acompanha a cúpula da cabeça), não um bloco de topo reto
      const hr = [{ y: 84, l: CX - 3, r: CX + 3 }]; for (let y = 94; y <= 200; y += 2) { const t = Math.max(0, (158 - y) / 66), a = (HA3(y) + 2.5) * Math.sqrt(Math.max(0.02, 1 - t * t)); hr.push({ y: y - 8, l: CX - a, r: CX + a }); }
      const hm = flat(NC.hair);
      hm.onBeforeCompile = sh => {
        sh.vertexShader = 'varying vec3 vP;\n' + sh.vertexShader.replace('#include <begin_vertex>', '#include <begin_vertex>\nvP = position;');
        sh.fragmentShader = 'varying vec3 vP;\n' + sh.fragmentShader.replace('#include <clipping_planes_fragment>', `#include <clipping_planes_fragment>
          { float yy = ${FOOT.toFixed(1)} - vP.y * ${S_PX.toFixed(1)} + 8.0; float sn = vP.z / max(1e-4, length(vP.xz));
            if (yy > 138.0 + 64.0 * smoothstep(-0.3, -0.9, sn)) discard; }`);
      };
      hm.customProgramCacheKey = () => 'nerdhair';
      N3.head.add(new THREE.Mesh(U.loft(hr, { depth: 0.74, cz: 0, NA: 64, pw: 2.3, capTop: false, capBot: false }), hm));
      for (const [x, h, rz] of [[234, 30, 0.15], [252, 24, -0.25]]) { const cn = new THREE.Mesh(new THREE.ConeGeometry(D(9), D(h), 12), flat(NC.hair)); cn.position.copy(NV(x, 92 - h / 2, -4)); cn.rotation.z = rz; N3.head.add(ol(cn, 0.006)); }
      // fone: arco vermelho por cima e as conchas nas orelhas, luzinhas azuis
      const band = new THREE.Mesh(new THREE.TorusGeometry(D(94), D(5.5), 10, 48, Math.PI), flat(NC.red)); band.position.copy(NV(240, 182, -6)); band.scale.y = 1.12; N3.head.add(ol(band, 0.006));
      for (const s of [-1, 1]) { const cup = new THREE.Mesh(new THREE.CylinderGeometry(D(24), D(24), D(20), 28), flat(NC.red)); cup.rotation.z = Math.PI / 2; cup.position.copy(NV(240 + s * 92, 182, -2)); N3.head.add(ol(cup, 0.008));
        const inn = new THREE.Mesh(new THREE.CylinderGeometry(D(14), D(14), D(22), 20), flat(NC.redDark)); inn.rotation.z = Math.PI / 2; inn.position.copy(NV(240 + s * 92, 182, -2)); N3.head.add(inn);
        const lt = new THREE.Mesh(new THREE.SphereGeometry(D(6), 12, 10), flat(NC.blue)); lt.position.copy(NV(240 + s * 58, 112, 40)); N3.head.add(ol(lt, 0.004)); }
      // rosto desenhado em código na pele da cabeça (px do sprite): olhos atrás das lentes, bochechas, o sorrisão aberto
      const fb = NB.faceBox;
      const nerdFace = (g, f) => {
        const ang = f === 'angry', pain = f === 'pain', ko = f === 'ko', ink = (w, c = '#16121a') => { g.strokeStyle = c; g.lineWidth = w; g.lineCap = 'round'; g.lineJoin = 'round'; };
        for (const sx of [-1, 1]) { const ex = 240 + sx * 35, ey = 147;
          g.fillStyle = 'rgba(255,120,140,0.55)'; g.beginPath(); g.ellipse(240 + sx * 52, 180, 12, 7, 0, 0, 7); g.fill();   // bochecha
          if (pain) { ink(4.5); g.beginPath(); g.moveTo(ex + sx * 9, ey - 9); g.lineTo(ex - sx * 6, ey); g.lineTo(ex + sx * 9, ey + 9); g.stroke(); continue; }
          if (ko) { ink(4.5); g.beginPath(); g.moveTo(ex - 8, ey - 8); g.lineTo(ex + 8, ey + 8); g.moveTo(ex + 8, ey - 8); g.lineTo(ex - 8, ey + 8); g.stroke(); continue; }
          g.fillStyle = '#ffffff'; g.beginPath(); g.ellipse(ex, ey, 13, ang ? 9 : 14, 0, 0, 7); g.fill();
          g.fillStyle = '#2a7fd0'; g.beginPath(); g.arc(ex + sx * -1, ey + 1, 9, 0, 7); g.fill();
          g.fillStyle = '#10213a'; g.beginPath(); g.arc(ex + sx * -1, ey + 1, 5, 0, 7); g.fill();
          g.fillStyle = '#ffffff'; g.beginPath(); g.arc(ex - 3, ey - 3, 2.6, 0, 7); g.fill();
          if (ang) { g.fillStyle = '#ffd9c0'; g.beginPath(); g.moveTo(ex - 15, ey - 16); g.lineTo(ex + 15, ey - 16); g.lineTo(ex + sx * -15, ey - 3); g.lineTo(ex + sx * 15, ey - 9); g.closePath(); g.fill();   // pálpebra brava
            ink(3.4); g.beginPath(); g.moveTo(ex + sx * 15, ey - 10); g.lineTo(ex - sx * 15, ey - 3); g.stroke(); } }
        // boca: o sorrisão do sprite (topo reto, fundo redondo), dentes em cima, língua; raiva = dentes cerrados, dor = careta, K.O. = "oh"
        if (ko) { g.fillStyle = '#16121a'; g.beginPath(); g.ellipse(240, 196, 11, 13, 0, 0, 7); g.fill(); g.fillStyle = '#7a1428'; g.beginPath(); g.ellipse(240, 197, 7, 9, 0, 0, 7); g.fill(); return; }
        if (pain) { ink(4); g.beginPath(); g.moveTo(214, 194); for (let k = 0; k < 6; k++) g.quadraticCurveTo(218 + k * 9, k % 2 ? 200 : 188, 222 + k * 9, 194); g.stroke(); return; }
        if (ang) { g.fillStyle = '#16121a'; g.beginPath(); g.roundRect(208, 184, 64, 24, 8); g.fill(); g.fillStyle = '#f4f4f2'; g.beginPath(); g.roundRect(213, 189, 54, 14, 4); g.fill();
          ink(1.8); g.beginPath(); g.moveTo(213, 196); g.lineTo(267, 196); for (const x of [224, 235, 246, 257]) { g.moveTo(x, 189); g.lineTo(x, 203); } g.stroke(); return; }
        g.fillStyle = '#16121a'; g.beginPath(); g.moveTo(204, 180); g.lineTo(276, 180); g.bezierCurveTo(276, 206, 260, 220, 240, 220); g.bezierCurveTo(220, 220, 204, 206, 204, 180); g.fill();
        g.save(); g.beginPath(); g.moveTo(209, 184); g.lineTo(271, 184); g.bezierCurveTo(271, 204, 257, 215, 240, 215); g.bezierCurveTo(223, 215, 209, 204, 209, 184); g.clip();
        g.fillStyle = '#d0283e'; g.fillRect(200, 180, 80, 40); g.fillStyle = '#f4f4f2'; g.fillRect(200, 180, 80, 9);   // boca e os dentes de cima
        g.fillStyle = '#ff8fa0'; g.beginPath(); g.ellipse(240, 214, 18, 9, 0, 0, 7); g.fill(); g.restore();   // língua
      };
      const drawFace = f => (g, at) => at((fb[0] + fb[2]) / 2, (fb[1] + fb[3]) / 2, fb[2] - fb[0], fb[3] - fb[1], gg => { gg.translate(-fb[0], -fb[1]); nerdFace(gg, f); }, fb[2] - fb[0], fb[3] - fb[1]);
      P.nerdFaceMat = NB.faceMat; NB.faceMat.map.dispose(); NB.faceMat.map = NB.faceTexture(drawFace('normal'));
      P.nerdFace = f => { P.nerdFaceMat.map.dispose(); P.nerdFaceMat.map = NB.faceTexture(drawFace(f)); P.nerdFaceMat.needsUpdate = true; };
      // óculos de verdade: aro preto grosso de cantos redondos em cada lente, assentado na curva do rosto, ponte entre
      // as lentes e a lente ciano transparente com o reflexo branco em diagonal
      const fzOf = (x, y) => NB.headZ(x, y);
      const lensTex = (() => { const c = document.createElement('canvas'); c.width = c.height = 128; const g = c.getContext('2d'); g.fillStyle = 'rgba(150,225,255,0.28)'; g.fillRect(0, 0, 128, 128);
        g.strokeStyle = 'rgba(255,255,255,0.85)'; g.lineCap = 'round'; for (const [o, w] of [[0, 12], [26, 6]]) { g.lineWidth = w; g.beginPath(); g.moveTo(30 + o, 20); g.lineTo(78 + o, 100); g.stroke(); }
        const t = new THREE.CanvasTexture(c); t.colorSpace = THREE.SRGBColorSpace; return t; })();
      const rrect = (cx, cy, w, h, rr, n = 64) => { const pts = []; for (let k = 0; k < n; k++) { const a = k / n * Math.PI * 2, c = Math.cos(a), sn = Math.sin(a);
          const px = Math.sign(c) * Math.pow(Math.abs(c), 0.35) * w / 2, py = Math.sign(sn) * Math.pow(Math.abs(sn), 0.35) * h / 2; pts.push([cx + px, cy + py]); } return pts; };   // retângulo de cantos redondos (superelipse)
      for (const sx of [-1, 1]) { const cx = 240 + sx * 35, cy = 147, W = 60, H = 46;
        const ring = rrect(cx, cy, W, H).map(([x, y]) => V(x, y, fzOf(x, y) + 5));
        N3.head.add(ol(new THREE.Mesh(new THREE.TubeGeometry(new THREE.CatmullRomCurve3(ring, true), 96, D(3.6), 8, true), flat('#16121a')), 0.004));
        const lg = new THREE.PlaneGeometry(D(W - 4), D(H - 4), 10, 8), lp = lg.attributes.position;
        for (let i = 0; i < lp.count; i++) { const x = cx + lp.getX(i) / D(1), y = cy - lp.getY(i) / D(1); lp.setXYZ(i, X(x), Y(y), D(fzOf(x, y) + 3.5)); }
        lg.computeVertexNormals(); N3.head.add(new THREE.Mesh(lg, new THREE.MeshBasicMaterial({ map: lensTex, transparent: true, depthWrite: false }))); }
      { const br = [[234 - 30 + 64, 140], [246 - 30 + 34, 138]].map(([x, y]) => [x, y]);   // ponte entre os aros
        const pts = [[240 - 6, 141], [240, 138], [240 + 6, 141]].map(([x, y]) => V(x, y, fzOf(x, y) + 6));
        N3.head.add(new THREE.Mesh(new THREE.TubeGeometry(new THREE.CatmullRomCurve3(pts), 12, D(3.2), 8), flat('#16121a'))); }
      // hastes: dos aros até as conchas do fone
      for (const sx of [-1, 1]) { const pts = [[240 + sx * 65, 143], [240 + sx * 80, 146], [240 + sx * 88, 150]].map(([x, y], k) => V(x, y, fzOf(240 + sx * 64, y) + 5 - k * 22));
        N3.head.add(new THREE.Mesh(new THREE.TubeGeometry(new THREE.CatmullRomCurve3(pts), 12, D(2.6), 6), flat('#16121a'))); }
    }
    { N3.body.add(NB.torso);   // tronco da base com a camisa
      const sz = 80 * 0.6 - 4;
      for (const s of [-1, 1]) {   // mangas curtas: ombro redondo, tubo cônico caindo para fora e a barra mais escura
        const sh = NV(240 + s * 78, 250, -4), TH = s * 0.62, dirv = new THREE.Vector3(Math.sin(TH), -Math.cos(TH), 0), UPV3 = new THREE.Vector3(0, 1, 0);
        const cap = new THREE.Mesh(new THREE.SphereGeometry(D(22), 22, 16), flat(NC.shirt)); cap.position.copy(sh); N3.body.add(ol(cap, 0.007));
        const sl = new THREE.Mesh(new THREE.CylinderGeometry(D(20), D(25), D(46), 22), flat(NC.shirt)); sl.position.copy(sh).addScaledVector(dirv, D(22)); sl.rotation.z = TH; N3.body.add(ol(sl, 0.007));
        const cf = new THREE.Mesh(new THREE.CylinderGeometry(D(25.5), D(26), D(9), 22), flat(NC.shirtDark)); cf.position.copy(sh).addScaledVector(dirv, D(42)); cf.rotation.z = TH; N3.body.add(ol(cf, 0.006));
        const arm = new THREE.Mesh(new THREE.CylinderGeometry(D(13), D(12), D(18), 16), flat(NC.skin)); arm.position.copy(sh).addScaledVector(dirv, D(54)); arm.rotation.z = TH; N3.body.add(ol(arm, 0.006)); }
      const pk = box(30, 40, 4, 188, 267, sz + 2, NC.grey, 2); pk.position.y -= D(8); N3.body.add(ol(pk, 0.005));
      // gola polo: duas abas verde-escuras dobradas sobre o peito e o botão da carcela
      for (const sx of [-1, 1]) { const sh = new THREE.Shape([[0, 0], [sx * 30, 4], [sx * 22, 24], [sx * 4, 16]].map(([x, y]) => new THREE.Vector2(D(x), D(-y))));
        const g = new THREE.ExtrudeGeometry(sh, { depth: D(3), bevelEnabled: true, bevelThickness: D(1), bevelSize: D(1), bevelSegments: 1 });
        const fl = new THREE.Mesh(g, flat(NC.shirtDark)); fl.position.copy(V(240 + sx * 4, 230, sz - 4)); fl.rotation.x = -0.35; N3.body.add(ol(fl, 0.005)); }
      { const bt = new THREE.Mesh(new THREE.SphereGeometry(D(3.4), 12, 8), flat(NC.white)); bt.scale.z = 0.6; bt.position.copy(V(240, 250, sz + 1)); N3.body.add(ol(bt, 0.003)); }
      for (const [x, c] of [[180, NC.blue], [192, NC.red]]) { const pen = new THREE.Mesh(new THREE.CylinderGeometry(D(2.5), D(2.5), D(16), 8), flat(c)); pen.position.copy(NV(x, 246, sz + 3)); N3.body.add(pen); }
      const cv = document.createElement('canvas'); cv.width = 256; cv.height = 128; const cg = cv.getContext('2d'); cg.fillStyle = NC.badge; cg.fillRect(0, 0, 256, 128); cg.fillStyle = '#f5d76e'; cg.font = 'bold 54px monospace'; cg.textAlign = 'center'; cg.textBaseline = 'middle'; cg.fillText('BOSS 01', 128, 66);
      const bt = new THREE.CanvasTexture(cv); bt.colorSpace = THREE.SRGBColorSpace;
      const bm = flat(NC.badge);
      const bd = new THREE.Mesh(new THREE.BoxGeometry(D(62), D(34), D(4)), [bm, bm, bm, bm, new THREE.MeshBasicMaterial({ map: bt }), bm]);
      bd.position.copy(NV(240, 270, sz + 2)); N3.body.add(ol(bd, 0.005)); }
    { const rows = []; for (let y = 342; y <= 368; y += 2) rows.push({ y: y - 8, l: CX - 80 + (y - 342) * 0.25, r: CX + 80 - (y - 342) * 0.25 });
      N3.low.add(ol(new THREE.Mesh(U.loft(rows, { depth: 0.6, cz: -4, NA: 48, pw: 2.8, capTop: false, capBot: true }), flat(NC.jeans)), 0.008));
      // cinto marrom com a fivela prateada
      const br = []; for (let y = 326; y <= 336; y += 2) br.push({ y, l: CX - 82, r: CX + 82 });
      N3.low.add(ol(new THREE.Mesh(U.loft(br, { depth: 0.6, cz: -4, NA: 48, pw: 2.8, capTop: false, capBot: false }), flat('#4a2e1c')), 0.006));
      N3.low.add(ol(box(22, 14, 4, 240, 331, 82 * 0.6 - 2, '#cfd8e2', 2), 0.004)); }
    const nLegs = {}, nFeet = {}, NLX = { [-1]: 198, [1]: 282 }, NTOP = 360, NANK = 438;
    for (const s of [-1, 1]) { nLegs[s] = NB.legs[s]; nFeet[s] = NB.feet[s]; root.add(nLegs[s], nFeet[s]); }   // pernas e tênis da base
    const nPlace = legPlacer({ legs: nLegs, feet: nFeet, hipX: { [-1]: X(NLX[-1]), [1]: X(NLX[1]) }, top: NB.legTop, bot: NB.legBot, hips, hipsY: HIPS });
    // mãos do nerd: punho com esparadrapo segurando o manche de fliperama (bola vermelha)
    const Lh = (x, y, z) => new THREE.Vector3(D(x), D(y), D(z));
    function stick() {
      const g = new THREE.Group();
      g.add(ol(new THREE.Mesh(new RoundedBoxGeometry(D(44), D(46), D(42), 4, D(14)), flat(NC.skin)), 0.008));
      for (const y of [-8, 6]) { const tp = new THREE.Mesh(new THREE.BoxGeometry(D(45), D(5), D(43)), flat('#e2a88a')); tp.position.y = D(y); g.add(tp); }
      const rod = new THREE.Mesh(new THREE.CylinderGeometry(D(4), D(4), D(30), 10), flat('#9aa6b8')); rod.position.copy(Lh(0, 30, 0)); g.add(ol(rod, 0.004));
      const ball = new THREE.Mesh(new THREE.SphereGeometry(D(24), 24, 18), flat(NC.red)); ball.position.copy(Lh(0, 54, 0)); g.add(ol(ball, 0.008));
      const hl = new THREE.Mesh(new THREE.SphereGeometry(D(6), 12, 10), new THREE.MeshBasicMaterial({ color: '#ffffff' })); hl.position.copy(Lh(-9, 64, 14)); g.add(hl);
      return g;
    }
    P.sticks = { [-1]: stick(), [1]: stick() }; root.add(P.sticks[-1], P.sticks[1]);
    // ================= o piloto da fase 2: o nerd INTEIRO atravessa o robô pelo buraco do quadril. A metade de cima (cabeça, camisa com
    // mangas e cinto) fica na frente, os manches nas mãos, e as perninhas de jeans saem pelas costas do robozão, balançando.
    // Tudo no grupo "pilot" (nó "sway" no jogo). Sem cúpula de vidro: só o aro de aço do buraco, na frente e atrás
    { const K = 0.62, pilot = new THREE.Group(), up = new THREE.Group();
      up.add(N3.head.clone(true), N3.body.clone(true), N3.low.clone(true));
      up.scale.setScalar(K); up.position.set(0, Y(376) - K * Y(350), D(30)); pilot.add(up);
      for (const sx of [-1, 1]) { const st = stick(); st.scale.setScalar(K * 0.95); st.position.copy(V(240 + sx * 66, 388, 62)); st.rotation.set(0.35, 0, -sx * 0.25); pilot.add(st); }
      // pernas: saem do quadril para trás (eixo -z), jeans, e o tênis vermelho de sola branca na ponta
      const legs2 = new THREE.Group(); legs2.position.copy(V(240, 376, 20)); pilot.add(legs2); F2.nerdLegs = legs2;
      for (const sx of [-1, 1]) { const lg = new THREE.Group(); lg.position.x = D(sx * 21);
        const jn = new THREE.Mesh(new THREE.CylinderGeometry(D(14), D(13), D(120), 20), flat(NC.jeans)); jn.rotation.x = Math.PI / 2; jn.position.z = -D(60); lg.add(ol(jn, 0.006));
        const sh = new THREE.Mesh(new RoundedBoxGeometry(D(30), D(26), D(44), 4, D(8)), flat(NC.red)); sh.position.set(0, -D(3), -D(128)); lg.add(ol(sh, 0.007));
        const sl = new THREE.Mesh(new THREE.BoxGeometry(D(32), D(7), D(46)), flat(NC.white)); sl.position.set(0, -D(15), -D(128)); lg.add(sl);
        legs2.add(lg); }
      F2.low.add(pilot); P.dreads = [pilot]; F2.pilot = pilot;
      // aro do buraco: argola de aço com o filete ciano na frente e outra atrás, para as pernas saírem de dentro
      for (const [z, col] of [[52, '#19d3c5'], [-58, '#9aa6b8']]) { const rim = new THREE.Mesh(new THREE.TorusGeometry(1, 0.09, 12, 48), flat(col)); rim.scale.set(D(56), D(40), D(14)); rim.position.copy(V(240, 378, z)); F2.low.add(ol(rim, 0.006)); } }

    // ================= troca de forma (botões no painel; ?fase=2 abre direto)
    let form = 1;
    const setForm = n => {
      form = n;
      R12.body.visible = R12.low.visible = n < 3; F1.head.visible = F1.chest.visible = F1.pelvis.visible = n === 1;
      F2.head.visible = F2.body.visible = F2.low.visible = n === 2; for (const t of F2legs) t.visible = n === 2;
      N3.head.visible = N3.body.visible = N3.low.visible = n === 3;
      for (const s of [-1, 1]) { legs[s].visible = feet[s].visible = P.saws[s].visible = n < 3; nLegs[s].visible = nFeet[s].visible = P.sticks[s].visible = n === 3; }
      document.querySelectorAll('[data-form]').forEach(b => b.style.outline = +b.dataset.form === n ? '2px solid #fc6' : '');
    };
    document.querySelectorAll('[data-form]').forEach(b => b.onclick = () => setForm(+b.dataset.form));
    setForm(+(new URLSearchParams(location.search).get('fase') || 1));
    const placeAll = (sx, dx, lift) => { placeFoot(sx, dx, lift); nPlace(sx, dx, lift); };

    // ================= especiais (os do jogo, skills.cpp e renderBoss em ring_renderer.cpp): cada fase com os seus e o golpe troca
    // para a fase dele. O alvo fica à frente, em coordenadas do mundo; o root escala por fase, então tudo divide por rs()
    const easeO = k => 1 - (1 - k) ** 3, win = (a, b, x) => Math.max(0, Math.min(1, (x - a) / (b - a))), TZ = 1.05;
    const rs = () => root.scale.x || 1;
    const GOLD = '#ffd700', RED = '#ef4444', AMBER = '#f59e0b', CYAN = '#00f5d4', ICE = '#a5f3fc', INKC = '#0d0f16';
    const fxRoot = new THREE.Group(); root.add(fxRoot);
    const glow = (col, o = 1, add = false) => new THREE.MeshBasicMaterial({ color: col, transparent: true, opacity: o, depthWrite: false, side: THREE.DoubleSide, blending: add ? THREE.AdditiveBlending : THREE.NormalBlending });
    const queue = []; let now = 0; const after = (dt, fn) => queue.push({ at: now + dt, fn });
    const FX = { sign: -99, saw: -99, overclock: -99, emp: -99, missiles: -99, holo: -99, apocalypse: -99 };
    const dotG = new THREE.SphereGeometry(1, 10, 8), dots = Array.from({ length: 240 }, () => { const m = new THREE.Mesh(dotG, glow('#fff')); m.visible = false; fxRoot.add(m); return { m, life: 0, v: new THREE.Vector3() }; });
    let dotI = 0; const dot = (p, v, rr, col, life, g = 3, op = 1, grow = 0) => { const q = dots[dotI++ % dots.length]; q.m.position.copy(p); q.m.scale.setScalar(rr); q.v.copy(v); q.r = rr; q.m.material.color.set(col); q.m.material.opacity = op; q.op = op; q.life = q.life0 = life; q.g = g; q.grow = grow; q.m.visible = true; };
    const burst = (p, n, spd, rr, cols, life, g = 3) => { for (let i = 0; i < n; i++) { const a = Math.random() * 6.283, sp = spd * (0.5 + Math.random()); dot(p, new THREE.Vector3(Math.cos(a) * sp, Math.sin(a) * sp * 0.8 + 0.4, (Math.random() - 0.5) * spd), rr * (0.6 + Math.random() * 0.7), cols[i % cols.length], life * (0.7 + Math.random() * 0.6), g); } };
    const segG = new THREE.BoxGeometry(1, 1, 1), ZA = new THREE.Vector3(0, 0, 1), segs = Array.from({ length: 60 }, () => { const m = new THREE.Mesh(segG, glow('#fff')); m.visible = false; fxRoot.add(m); return { m, life: 0 }; });
    let segI = 0; const seg = (a, b, w, col, life) => { const q = segs[segI++ % segs.length], d = b.clone().sub(a), L = d.length(); q.m.position.copy(a).addScaledVector(d, 0.5); q.m.quaternion.setFromUnitVectors(ZA, d.normalize()); q.m.scale.set(w, w, L); q.m.material.color.set(col); q.m.material.opacity = 1; q.life = q.life0 = life; q.m.visible = true; };
    const bolt = (a, b, w, col, life, n = 5) => { let p = a.clone(); for (let i = 1; i <= n; i++) { const q = a.clone().lerp(b, i / n); if (i < n) q.add(new THREE.Vector3((Math.random() - 0.5) * 0.12, Math.random() * 0.12, (Math.random() - 0.5) * 0.12)); seg(p, q, w, col, life); p = q; } };
    // fagulhas douradas com borda vermelha da serra, como no jogo
    const spark = (p, spread) => dot(new THREE.Vector3(p.x + (Math.random() - 0.5) * spread, p.y + (Math.random() - 0.5) * spread, p.z + (Math.random() - 0.5) * spread), new THREE.Vector3((Math.random() - 0.5) * 1.6, 0.4 + Math.random() * 1.2, (Math.random() - 0.5) * 1.6), 0.014 + Math.random() * 0.016, Math.random() < 0.7 ? GOLD : RED, 0.32, 4);
    // onda no chão em volta do alvo: dois anéis e o miolo translúcido (térmica vermelha ou PEM ciano)
    const wave = new THREE.Group(); wave.rotation.x = -Math.PI / 2; wave.visible = false; fxRoot.add(wave);
    const waveFill = new THREE.Mesh(new THREE.CircleGeometry(1, 48), glow(AMBER, 0.25)), waveEdge = new THREE.Mesh(new THREE.RingGeometry(0.965, 1, 56), glow(RED)), waveIn = new THREE.Mesh(new THREE.RingGeometry(0.7, 0.72, 56), glow(AMBER)); wave.add(waveFill, waveEdge, waveIn);
    let waveT = -99, waveEmp = false; const startWave = emp => { waveT = now; waveEmp = emp; waveEdge.material.color.set(emp ? CYAN : RED); waveFill.material.color.set(emp ? ICE : AMBER); waveIn.material.color.set(emp ? ICE : AMBER); };
    // soquete do pulso (sprite WristSocket): aparece quando a serra sai voando
    const sockets = { [-1]: null, [1]: null };
    for (const sx of [-1, 1]) { const g = new THREE.Group(); const add = (rr, h, c, z) => { const m = new THREE.Mesh(new THREE.CylinderGeometry(D(rr), D(rr), D(h), 28), flat(c)); m.rotation.x = Math.PI / 2; m.position.z = D(z); g.add(rr > 20 ? ol(m, 0.006) : m); }; add(30, 8, INKC, 0); add(21, 9, C.red, 1); add(13, 10, INKC, 2); add(7, 11, C.yellow, 3); g.visible = false; root.add(g); sockets[sx] = g; }
    // fase 2: a cabeça do robô virou a luva esquerda (sprite HeadLeft), a direita continua a serra
    const headGlove = new THREE.Group(); headGlove.visible = false; root.add(headGlove);
    { root.updateMatrixWorld(true); const hb = new THREE.Box3().setFromObject(F1.head), hc = F1.head.worldToLocal(hb.getCenter(new THREE.Vector3())), cl = F1.head.clone(); cl.visible = true; cl.position.copy(hc).negate(); headGlove.add(cl); headGlove.scale.setScalar(0.62); }
    // micromísseis: corpo claro, ponta vermelha, aleta escura; apontam para onde vão
    const UPY = new THREE.Vector3(0, 1, 0), mis = [0, 1, 2, 3].map(() => { const g = new THREE.Group(); g.add(withOutline(new THREE.Mesh(new THREE.CapsuleGeometry(0.038, 0.13, 4, 12), flat('#e2e8f0')), 0.005)); const nose = new THREE.ConeGeometry(0.039, 0.07, 12); nose.translate(0, 0.125, 0); g.add(withOutline(new THREE.Mesh(nose, flat(RED)), 0.005)); const fin = new THREE.BoxGeometry(0.13, 0.05, 0.012); fin.translate(0, -0.08, 0); g.add(new THREE.Mesh(fin, flat(C.navyDark))); g.visible = false; fxRoot.add(g); return g; });
    // o raio apocalíptico: coluna vermelha com borda escura, miolo claro e faixas descendo
    const colMat = new THREE.ShaderMaterial({ transparent: true, depthWrite: false, uniforms: { uA: { value: 1 } },
      vertexShader: 'varying vec3 vN; varying vec3 vV; void main() { vec4 mv = modelViewMatrix * vec4(position, 1.0); vN = normalize(normalMatrix * normal); vV = normalize(-mv.xyz); gl_Position = projectionMatrix * mv; }',
      fragmentShader: 'uniform float uA; varying vec3 vN; varying vec3 vV; void main() { float r = 1.0 - abs(dot(normalize(vN), normalize(vV))); gl_FragColor = vec4(r > 0.8 ? vec3(0.5, 0.11, 0.11) : vec3(0.937, 0.267, 0.267), (r > 0.8 ? 1.0 : 0.93) * uA); }' });
    const BEAM_H = 6, beamG = new THREE.Group(); beamG.visible = false; fxRoot.add(beamG);
    { const g = new THREE.CylinderGeometry(0.23, 0.33, BEAM_H, 28, 1, true); g.translate(0, BEAM_H / 2, 0); const col = new THREE.Mesh(g, colMat); col.renderOrder = 5; beamG.add(col); const cg = new THREE.CylinderGeometry(0.06, 0.1, BEAM_H, 16, 1, true); cg.translate(0, BEAM_H / 2, 0); const core = new THREE.Mesh(cg, glow('#fff1f2', 0.95)); core.renderOrder = 6; beamG.add(core); }
    const beamBands = Array.from({ length: 7 }, () => { const m = new THREE.Mesh(new THREE.CylinderGeometry(0.32, 0.32, 0.05, 28, 1, true), glow('#fecaca', 0.42, true)); m.renderOrder = 7; beamG.add(m); return m; });
    const beamFloor = new THREE.Mesh(new THREE.CircleGeometry(1, 36), glow('#fecaca', 0.85, true)); beamFloor.rotation.x = -Math.PI / 2; beamFloor.position.y = 0.02; beamG.add(beamFloor);
    // clones do Nerd: o boneco copiado com as cores tingidas de azul e transparente (a tinta de contorno sai), piscando
    let clones = null; const holoMats = new Map(), holoDepth = new THREE.MeshBasicMaterial({ colorWrite: false });
    const buildClones = () => { clones = [0, 1, 2].map(() => { const g = new THREE.Group(); for (const ch of root.children) { if (ch === fxRoot || ch === headGlove || ch === P.sticks[-1] || ch === P.sticks[1] || !ch.visible || ch === P.saws[-1] || ch === P.saws[1]) continue; if (ch === sockets[-1] || ch === sockets[1] || ch === hands[-1] || ch === hands[1]) continue; g.add(ch.clone()); }
        const sticks = { [-1]: P.sticks[-1].clone(), [1]: P.sticks[1].clone() }; g.add(sticks[-1], sticks[1]); g.position.y = 0; const pre = [];
        g.traverse(o => { if (!o.isMesh) return; if (o.material.side === THREE.BackSide) { o.visible = false; return; } let mt = holoMats.get(o.material); if (!mt) { const base = new THREE.Color(o.material.color?.getHex?.() ?? 0xffffff); mt = new THREE.MeshBasicMaterial({ color: base.multiply(new THREE.Color(120 / 255, 200 / 255, 1)).lerp(new THREE.Color(0.25, 0.65, 1), 0.5), map: o.material.map?.isTexture && o.material.transparent ? o.material.map : null, alphaTest: o.material.transparent ? 0.3 : 0, transparent: true, opacity: 0.6 }); holoMats.set(o.material, mt); } o.material = mt; pre.push(o); }); for (const o of pre) o.add(new THREE.Mesh(o.geometry, holoDepth));
        g.visible = false; fxRoot.add(g); return { g, sticks }; }); };
    const needForm = n => { if (form !== n) { setForm(n); } };
    const skills = {
      // assinatura (laboratório): as serras giram e são arremessadas em espiral, crescendo, e voltam aos pulsos
      signature: { dur: 3.0, start: () => { FX.sign = now; setFace('angry'); after(3.0, () => setFace('normal')); },
        pose: (k, pose) => { const p = k / 3, th = ss(0.3, 0.45, p) * (1 - ss(0.82, 1, p)); pose[-1] = [0.1 * th, 0.14 * th, -0.2 * th, 0, 0]; pose[1] = [0.1 * th, 0.14 * th, -0.2 * th, 0, 0]; return { lean: -0.08 * th, tilt: 0.03 * Math.sin(k * 24) * th }; } },
      // fase 1. Serras desacopladas voam até o alvo e voltam aos pulsos, com os soquetes à mostra
      saw: { dur: 1.1, start: () => { needForm(1); FX.saw = now; setFace('angry'); after(1.1, () => setFace('normal')); after(0.55, () => { const p = new THREE.Vector3(0, 0.75 / rs(), 1.3 / rs()); burst(p, 18, 1.8, 0.018, [GOLD, RED], 0.5); }); },
        pose: (k, pose) => { const p = k / 1.1, q = Math.sin(Math.min(1, p / 0.28) * Math.PI / 2) * (1 - ss(0.8, 1, p)), v = [0.2 * q, 0.03 * q, 0.06 * q, 0, 0]; pose[-1] = v; pose[1] = [...v]; return { lean: 0.1 * q }; } },
      // fase 1. Onda térmica do reator: treme acumulando e solta a onda vermelha em 0,4
      overclock: { dur: 1.4, start: () => { needForm(1); FX.overclock = now; setFace('angry'); after(1.4, () => setFace('normal')); after(0.56, () => startWave(false)); },
        pose: (k, pose) => { const p = k / 1.4, ch = ss(0, 0.38, p) * (1 - ss(0.4, 0.5, p)), re = ss(0.38, 0.44, p) * (1 - ss(0.8, 1, p)), sh = 0.018 * ch * Math.sin(k * 70), v = [-0.05 * ch + 0.08 * re, 0.05 * ch + sh + 0.16 * re, -0.3 * ch - 0.45 * re, 0, 0]; pose[-1] = v; pose[1] = [...v]; return { drop: 0.07 * ch, lean: -0.1 * re, tilt: 0.02 * Math.sin(k * 60) * ch }; } },
      // fase 2. Ergue a cabeça e a serra e crava na lona: o pulso sai em 0,45
      emp: { dur: 1.4, start: () => { needForm(2); FX.emp = now; setFace('angry'); after(1.4, () => setFace('normal')); after(0.63, () => { startWave(true); for (const sx of [-1, 1]) burst(new THREE.Vector3(sx * 0.45, 0.06, 0.45), 10, 1.5, 0.016, [CYAN, ICE], 0.45); }); },
        pose: (k, pose) => { const p = k / 1.4, up = ss(0, 0.36, p) * (1 - ss(0.36, 0.45, p)), dn = ss(0.38, 0.45, p) * (1 - ss(0.72, 1, p)), v = [0.08 * dn - 0.02 * up, 0.5 * up - 0.42 * dn, -0.15 * up, 0, 0]; pose[-1] = v; pose[1] = [...v]; return { drop: 0.16 * dn, lean: 0.24 * dn - 0.12 * up }; } },
      // fase 2. Quatro micromísseis em arco, saindo das brocas, um de cada lado
      missiles: { dur: 1.7, start: () => { needForm(2); FX.missiles = now; setFace('angry'); after(1.7, () => setFace('normal')); },
        pose: (k, pose) => { const p = k / 1.7, e = ss(0, 0.1, p) * (1 - ss(0.86, 1, p)), rec = [0.05, 0.233, 0.417, 0.6].reduce((a, h) => a + Math.max(0, 1 - Math.abs(p - h - 0.02) / 0.05), 0), v = [-0.02 * e, -0.08 * e, -0.28 * e, 0, 0]; pose[-1] = v; pose[1] = [...v]; return { lean: -0.05 * e - 0.08 * rec, drop: 0.03 * e }; } },
      // fase 3. Os clones do Nerd piscando em volta do alvo, o piloto mexendo os manches a toda
      holo: { dur: 1.6, start: () => { needForm(3); if (!clones) buildClones(); FX.holo = now; setFace('angry'); after(1.6, () => setFace('normal')); },
        pose: (k, pose) => { const p = k / 1.6, e = ss(0, 0.12, p) * (1 - ss(0.88, 1, p)), a = Math.sin(k * 26); pose[-1] = [(0.1 + 0.1 * a) * e, 0.04 * e, 0, 0, 0]; pose[1] = [(0.1 - 0.1 * a) * e, 0.04 * e, 0, 0, 0]; return { tilt: 0.03 * a * e }; } },
      // fase 3. O feixe orbital desce em 0,4 e fica até 0,85, com os dois manches empurrados
      apocalypse: { dur: 1.9, start: () => { needForm(3); FX.apocalypse = now; setFace('angry'); after(1.9, () => setFace('normal')); },
        pose: (k, pose) => { const p = k / 1.9, up = ss(0, 0.32, p) * (1 - ss(0.36, 0.42, p)), go = ss(0.36, 0.42, p) * (1 - ss(0.85, 1, p)), sh = 0.012 * Math.sin(k * 60) * go, v = [0.2 * go, 0.34 * up + 0.03 * go + sh, 0.05 * go, 0, 0]; pose[-1] = v; pose[1] = [...v]; return { lean: 0.14 * go - 0.08 * up }; } },
    };
    const fxFrame = c => {
      now = c.t; const t = now, dt = c.dt, S = rs(), cam = window.VIN?.camera, tz = TZ / S;
      for (let i = queue.length - 1; i >= 0; i--) if (now >= queue[i].at) { queue[i].fn(); queue.splice(i, 1); }
      for (const q of dots) { if (q.life <= 0) { q.m.visible = false; continue; } q.life -= dt; q.v.y -= q.g * dt; q.m.position.addScaledVector(q.v, dt); const u = q.life / q.life0; if (q.grow) { q.r += q.grow * dt; q.m.scale.setScalar(q.r); q.m.material.opacity = q.op * Math.min(1, u * 3.5); } else q.m.scale.setScalar(q.r * Math.max(0.05, u)); if (q.m.position.y < 0.01) q.life = 0; }
      for (const q of segs) { if (q.life <= 0) { q.m.visible = false; continue; } q.life -= dt; q.m.material.opacity = Math.max(0, q.life / q.life0); }
      // luva esquerda da fase 2 (a cabeça) e soquetes
      const hg = form === 2; headGlove.visible = hg; if (form < 3) P.saws[-1].visible = !hg; if (hg) { headGlove.position.copy(hands[-1].position); headGlove.rotation.y = upper.rotation.y * 0.8; }
      // assinatura: espiral das serras (na fase 3 os manches fazem a mesma espiral, mais fechada)
      const pS = (t - FX.sign) / 3; if (pS > 0 && pS < 1) { const th = easeO(win(0.35, 0.6, pS)) * (1 - easeO(win(0.75, 1, pS))), ang = pS * Math.PI * 4, big = form < 3, ax = big ? 0.95 : 0.62, ay = big ? 0.54 : 0.42, cy = big ? 1.03 : 0.95; for (const sx of [-1, 1]) { const G = form === 3 ? P.sticks[sx] : (hg && sx < 0 ? headGlove : P.saws[sx]), dir = sx < 0 ? 1 : -1; G.position.x += (dir * Math.cos(ang) * ax - G.position.x) * th; G.position.y += (cy + dir * Math.sin(ang) * ay - G.position.y) * th; G.position.z += (0.62 - G.position.z) * th; G.scale.setScalar((hg && sx < 0 ? 0.62 : 1) * (1 + 0.6 * th)); if (G.userData.spin) G.userData.spin.rotation.y += dt * 25 * sx; else if (form === 3) G.rotateY(t * 25 * sx); if (th > 0.2 && Math.random() < 0.7) spark(G.position, 0.3); } }
      else { for (const sx of [-1, 1]) { P.saws[sx].scale.setScalar(1); P.sticks[sx].scale.setScalar(1); } headGlove.scale.setScalar(0.62); }
      // serras voando até o alvo e de volta
      { const p = (t - FX.saw) / 1.1, on = p > 0 && p < 1; for (const sx of [-1, 1]) { const so = sockets[sx]; so.visible = on; if (!on) continue; const G = P.saws[sx]; so.position.copy(hands[sx].position); const u = p <= 0.5 ? Math.sin(p / 0.5 * Math.PI / 2) : 1 - Math.sin((p - 0.5) / 0.5 * Math.PI / 2); G.position.lerpVectors(hands[sx].position, new THREE.Vector3(-sx * 0.25 / S, 0.75 / S, 1.3 / S), u); G.position.y += Math.sin(u * Math.PI) * 0.16; G.userData.spin.rotation.y += dt * 25 * sx; for (let i = 0; i < 2; i++) if (u > 0.05) spark(G.position, 0.5); } }
      // onda no chão
      { const a = t - waveT, dur = waveEmp ? 0.6 : 0.55, on = a >= 0 && a < dur; wave.visible = on; if (on) { const R = (0.1 + a * (waveEmp ? 3.8 : 3.2)) / S, life = 1 - a / dur; wave.position.set(0, 0.018, tz); wave.scale.setScalar(R); waveFill.material.opacity = 0.25 * life; waveEdge.material.opacity = life; waveIn.material.opacity = life; if (waveEmp && Math.random() < 0.8) { const an = Math.random() * 6.283, A = new THREE.Vector3(Math.cos(an) * R, 0.03, tz + Math.sin(an) * R), B = new THREE.Vector3(Math.cos(an + 0.4) * R * 0.75, 0.05, tz + Math.sin(an + 0.4) * R * 0.75); bolt(A, B, 0.012, CYAN, 0.14, 4); } } }
      // sobrecarga: vapor vermelho e fagulhas enquanto acumula
      { const p = (t - FX.overclock) / 1.4; if (p > 0 && p < 0.42 && Math.random() < 0.7) { const a = Math.random() * 6.283; dot(new THREE.Vector3(Math.cos(a) * 0.4, 0.7 + Math.random() * 0.7, Math.sin(a) * 0.3), new THREE.Vector3(Math.cos(a) * 0.3, 0.9, Math.sin(a) * 0.3), 0.028, Math.random() < 0.5 ? RED : AMBER, 0.45, 0, 0.9); } }
      // PEM: faíscas ciano subindo pelos braços enquanto arma
      { const p = (t - FX.emp) / 1.4; if (p > 0 && p < 0.45 && Math.random() < 0.45) { const sx = Math.random() < 0.5 ? -1 : 1, g = hands[sx].position; bolt(g.clone().add(new THREE.Vector3((Math.random() - 0.5) * 0.3, -0.15, 0)), g.clone().add(new THREE.Vector3((Math.random() - 0.5) * 0.3, 0.2, 0)), 0.012, CYAN, 0.12, 4); } }
      // mísseis: lançamento 0,3 antes de cada golpe, com o arco e o rastro de fumaça
      { const p = (t - FX.missiles) / 1.7, at = 0.35, span = (0.9 - at) / 3; mis.forEach((g, m) => { const hit = at + span * m, la = Math.max(0.02, hit - 0.3), on = p >= la && p <= hit; g.visible = on; if (!on) { if (p > hit && g.userData.alive) { g.userData.alive = false; const q = g.position.clone(); burst(q, 16, 1.5, 0.022, [AMBER, RED, '#e2e8f0'], 0.45); } return; } g.userData.alive = true; const u = (p - la) / (hit - la), sd = m % 2 === 0 ? -1 : 1, from = new THREE.Vector3(sd * 0.42, Y(160) + m * 0.05, 0.2), to = new THREE.Vector3(-sd * 0.17 / S, (0.55 + (m - 1.5) * 0.1) / S, tz), pos = from.clone().lerp(to, u); pos.y += Math.sin(u * Math.PI) * 0.45; const nx = from.clone().lerp(to, Math.min(1, u + 0.03)); nx.y += Math.sin(Math.min(1, u + 0.03) * Math.PI) * 0.45; g.position.copy(pos); g.quaternion.setFromUnitVectors(UPY, nx.sub(pos).normalize()); dot(pos, new THREE.Vector3(0, 0.05, 0), 0.035, '#cbd5e1', 0.3, 0, 0.85, 0.08); if (u < 0.1) dot(from, new THREE.Vector3((Math.random() - 0.5), 0.6, 0), 0.03, AMBER, 0.25, 0); }); }
      // clones: três em volta do alvo, piscando; fade no fim
      if (clones) { const p = (t - FX.holo) / 1.6, on = p >= 0.25 && p < 1, fade = 1 - Math.max(0, p - 0.85) / 0.15; for (const mt of holoMats.values()) mt.opacity = 0.6 * fade; clones.forEach((q, h) => { const vis = on && (Math.floor(t * 30) + h) % 3 !== 0; q.g.visible = vis; if (!vis) return; q.g.position.set((h - 1) * 0.62 - 0.25, 0, 0.9); q.g.scale.setScalar(0.85); q.g.rotation.z = (1 - h) * 0.08; const ph = Math.floor(t * 20 + h * 3) % 4; for (const sx of [-1, 1]) { const st = q.sticks[sx], jab = (ph === 0 && sx < 0) || (ph === 2 && sx > 0) ? 0.45 : 0; st.position.set(sx * 0.36, 0.86 + (jab ? 0.05 : 0), 0.3 + jab); } }); }
      // o raio: cresce em 0,12 e pulsa, faixas descendo, chão aceso e fagulhas
      { const p = (t - FX.apocalypse) / 1.9, on = p > 0.4 && p < 0.85 ; beamG.visible = on; if (on) { const gr = Math.min(1, (p - 0.4) / 0.12) * (0.93 + 0.07 * Math.sin(t * 48)), out = 1 - win(0.8, 0.85, p); beamG.position.set(0, 0, tz); beamG.children[0].scale.set(gr, 1, gr); beamG.children[1].scale.set(gr, 1, gr); colMat.uniforms.uA.value = out; const beatP = (p * 18) % 1; beamBands.forEach((m, j) => { const u = (beatP + j / 7) % 1; m.position.y = 3 * (1 - u); m.scale.set(gr * (0.72 + 0.3 * u), 1, gr * (0.72 + 0.3 * u)); }); beamFloor.scale.setScalar(0.42 * gr); beamFloor.material.opacity = 0.85 * out; for (let i = 0; i < 2; i++) { const a = Math.random() * 6.283, sp = 1 + Math.random() * 1.4; dot(new THREE.Vector3(Math.cos(a) * 0.3, 0.05, tz + Math.sin(a) * 0.3), new THREE.Vector3(Math.cos(a) * sp, 1 + Math.random() * 1.6, Math.sin(a) * sp), 0.016, Math.random() < 0.5 ? '#fecaca' : RED, 0.45, 4); } } }
    };

    const onFrame = c => {
      if (F2.topSaw) F2.topSaw.userData.spin.rotation.y += c.dt * 9;
      if (F2.nerdLegs) { const tt = performance.now() / 1000; F2.nerdLegs.children.forEach((lg, k) => { lg.rotation.x = Math.sin(tt * 3.1 + k * 2.2) * 0.16; lg.rotation.y = Math.sin(tt * 2.3 + k) * 0.1; }); }
      for (const sx of [-1, 1]) {
        const G = form < 3 ? P.saws[sx] : P.sticks[sx], k = Math.min(1, Math.max(0, c.pose[sx][0] / 0.3));
        G.position.copy(hands[sx].position);
        const yq = new THREE.Quaternion().setFromAxisAngle(UPV, upper.rotation.y * 0.8);
        const dir = new THREE.Vector3(sx * 0.1, 0.15, 1).lerp(new THREE.Vector3(0, 0, 1), k).normalize().applyQuaternion(yq);   // disco de frente, como no sprite
        G.quaternion.setFromUnitVectors(UPV, dir);
        if (G.userData.spin) G.userData.spin.rotation.y += c.dt * (6 + 30 * k) * sx;
        if (form === 3) G.quaternion.setFromUnitVectors(UPV, new THREE.Vector3(sx * 0.1, 1, 0.25 + 0.6 * k).normalize().applyQuaternion(yq));   // manche de pé
      }
      fxFrame(c);
    };
    return { P, upper, WAIST, neck, NECK, hips, HIPS, hands, hipSway: 0.25, hand: { x: 0.72, y: Y(245), z: 0.5, yaw: -0.12 }, placeFoot: placeAll, setFace, skills, onFrame, get scale() { return form === 1 ? 1.32 : form === 2 ? 1.18 : 1; } };   // fase 1 é o maior de todos, a 2 grandalhão, a 3 volta ao tamanho dos demais
  },
};
