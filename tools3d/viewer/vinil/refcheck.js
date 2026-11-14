// Conferência com a folha de modelo: renderiza o personagem em projeção ortográfica na escala da folha (frente,
// perfil e costas), mostra cada vista ao lado do recorte da folha e mede a diferença de contorno linha a linha.
//   const { refCheck } = await import('/tools3d/viewer/vinil/refcheck.js'); await refCheck(VIN, SHEET);
// SHEET: { url, S (px por metro), FOOT, CX, ZC (x do eixo no perfil), BG [r,g,b], views: { front: [x0, x1], left: [...], back: [...] },
//          y0, y1, backCX (eixo na vista de costas) }
export async function refCheck(VIN, SH, { show = 'all', step = 4, tol = 3, hide = [], overlay = false, crop = null, xcrop = null, k = 1 } = {}) {
  const { THREE, scene, renderer } = VIN;
  const img = await new Promise((ok, bad) => { const i = new Image(); i.onload = () => ok(i); i.onerror = bad; i.src = SH.url; });
  const ref = document.createElement('canvas'); ref.width = img.width; ref.height = img.height;
  const rg = ref.getContext('2d', { willReadFrequently: true }); rg.drawImage(img, 0, 0);
  const H = SH.y1 - SH.y0;
  // pose de descanso (t = 0) e sem chão / luvas
  VIN.frame(-VIN.clock.elapsedTime);
  const hidden = [];
  scene.traverse(o => { if (o.isMesh && o.geometry?.type === 'CircleGeometry' && o.parent === scene) { hidden.push([o, o.visible]); o.visible = false; } });
  for (const o of hide) { hidden.push([o, o.visible]); o.visible = false; }
  const bg = scene.background; scene.background = new THREE.Color(1, 0, 1);
  const views = {
    front: { pos: [0, 0, 10], ref: SH.views.front, map: rx => (rx - SH.CX) / SH.S },
    left: { pos: [-10, 0, 0], ref: SH.views.left, map: rx => (rx - SH.ZC) / SH.S },
    back: { pos: [0, 0, -10], ref: SH.views.back, map: rx => -(SH.backCX - rx) / SH.S },
  };
  const out = {}, panels = [];
  for (const [name, v] of Object.entries(views)) {
    if (show !== 'all' && show !== name) continue;
    const [x0, x1] = (xcrop && xcrop[name]) || v.ref, W = x1 - x0, KW = W * k, KH = H * k;
    const cam = new THREE.OrthographicCamera(v.map(x0), v.map(x1), (SH.FOOT - SH.y0) / SH.S, (SH.FOOT - SH.y1) / SH.S, 0.1, 50);
    cam.position.set(...v.pos); cam.up.set(0, 1, 0); cam.lookAt(0, 0, 0);
    cam.updateProjectionMatrix();
    const rt = new THREE.WebGLRenderTarget(KW, KH, { samples: 4 }); rt.texture.colorSpace = THREE.SRGBColorSpace;
    renderer.setRenderTarget(rt); renderer.render(scene, cam); renderer.setRenderTarget(null);
    const px = new Uint8Array(KW * KH * 4); renderer.readRenderTargetPixels(rt, 0, 0, KW, KH, px); rt.dispose();
    const c = document.createElement('canvas'); c.width = KW; c.height = KH; const g = c.getContext('2d'); const id = g.createImageData(KW, KH);
    for (let y = 0; y < KH; y++) for (let x = 0; x < KW; x++) { const s = ((KH - 1 - y) * KW + x) * 4, d = (y * KW + x) * 4;
      for (let k = 0; k < 3; k++) id.data[d + k] = px[s + k]; id.data[d + 3] = 255; }   // o alvo já sai em sRGB
    g.putImageData(id, 0, 0);
    const rd = rg.getImageData(x0, SH.y0, W, H).data, md = id.data;
    const isRef = SH.obj ? (x, y) => { const i = (y * W + x) * 4, mx = Math.max(rd[i], rd[i + 1], rd[i + 2]), mn = Math.min(rd[i], rd[i + 1], rd[i + 2]); return mx - mn > 38 || mx >= 248 || mx < 150; }
      : (x, y) => { const i = (y * W + x) * 4; return Math.abs(rd[i] - SH.BG[0]) + Math.abs(rd[i + 1] - SH.BG[1]) + Math.abs(rd[i + 2] - SH.BG[2]) > 40; };
    const isMod = (x, y) => { const i = (y * k * KW + x * k) * 4; return !(md[i] > 230 && md[i + 1] < 40 && md[i + 2] > 230); };
    const rows = [];
    for (let y = 0; y < H; y += step) {
      if (SH.y0 + y > SH.FOOT - 2) break;
      let rl = -1, rr = -1, ml = -1, mr = -1;
      for (let x = 0; x < W; x++) { if (isRef(x, y)) { if (rl < 0) rl = x; rr = x; } if (isMod(x, y)) { if (ml < 0) ml = x; mr = x; } }
      if (rl < 0 && ml < 0) continue;
      const dl = ml < 0 || rl < 0 ? 999 : ml - rl, dr = mr < 0 || rr < 0 ? 999 : mr - rr;
      if (Math.abs(dl) > tol || Math.abs(dr) > tol) rows.push(`${SH.y0 + y}: ref ${rl < 0 ? '-' : x0 + rl}..${rr < 0 ? '-' : x0 + rr}  mod ${ml < 0 ? '-' : x0 + ml}..${mr < 0 ? '-' : x0 + mr}  (${dl >= 0 ? '+' : ''}${dl}, ${dr >= 0 ? '+' : ''}${dr})`);
    }
    out[name] = rows;
    panels.push([name, x0, W, c]);
  }
  scene.background = bg; for (const [o, v] of hidden) o.visible = v;
  // painel: folha | modelo | contorno da folha por cima do modelo
  let ov = document.getElementById('refcheck'); if (!ov) { ov = document.createElement('canvas'); ov.id = 'refcheck'; ov.style.cssText = 'position:fixed;inset:0;z-index:20;background:#ddd;cursor:pointer'; ov.onclick = () => ov.remove(); document.body.appendChild(ov); }
  const NP = overlay ? 3 : 2, [cy0, cy1] = crop || [0, H], CH = cy1 - cy0;
  const TW = panels.reduce((s, p) => s + p[2] * k * NP + 12, 0); ov.width = TW; ov.height = CH * k; { const f = Math.min(innerWidth / TW, innerHeight / (CH * k)); ov.style.width = TW * f + 'px'; ov.style.height = CH * k * f + 'px'; ov.style.right = ov.style.bottom = 'auto'; }
  const og = ov.getContext('2d'); og.fillStyle = '#ddd'; og.fillRect(0, 0, TW, CH * k); og.imageSmoothingQuality = 'high';
  let ox = 0;
  for (const [, x0, W, c] of panels) {
    const KW = W * k, KC = CH * k;
    og.drawImage(ref, x0, SH.y0 + cy0, W, CH, ox, 0, KW, KC); og.drawImage(c, 0, cy0 * k, KW, KC, ox + KW, 0, KW, KC);
    if (overlay) { og.drawImage(c, 0, cy0 * k, KW, KC, ox + 2 * KW, 0, KW, KC); og.globalAlpha = 0.45; og.drawImage(ref, x0, SH.y0 + cy0, W, CH, ox + 2 * KW, 0, KW, KC); og.globalAlpha = 1; }
    ox += KW * NP + 12;
  }
  return out;
}

export const ESPINHA_SHEET = { url: '/assets3d/espinha/ref/espinha_turnaround.png', S: 351, FOOT: 806, CX: 213, ZC: 569, backCX: 939.5, BG: [237, 234, 228],
  y0: 120, y1: 812, views: { front: [40, 388], left: [445, 712], back: [766, 1110] } };

export const CREMOSO_SHEET = { url: '/assets3d/cremoso/ref/cremoso_turnaround.png', S: 287, FOOT: 729, CX: 241, ZC: 592, backCX: 937.5, BG: [241, 240, 236], obj: true,
  y0: 220, y1: 740, views: { front: [20, 465], left: [466, 730], back: [742, 1140] } };
