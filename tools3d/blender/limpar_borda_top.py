"""Limpa a borda de uma peça de roupa com a pele na textura chapada (só textura): cada face da região que mistura texels
da roupa e da pele é repintada inteira com a cor que predomina nela (a divisa passa a seguir a malha, sem borrão).
Opcional: troca o tom da pele na textura inteira.

  blender -b --factory-startup -P tools3d/blender/limpar_borda_top.py -- corpo.glb textura.png saida.png
          --pele "#e49d6f" --roupa "#eb2f14,#8f1216,#e2b23a" --mistura "#ac4f4a,#b97d72,#b97d6f,#e22f14"
          [--z 1.08 1.42] [--nova-pele "#c98a5e"] [--barra "#8f1216" 0.028 "#eb2f14"]
  --barra: faixa escura de altura fixa acompanhando a borda de baixo da roupa; o resto da roupa fica com a cor principal
"""
import bpy, os, sys, json, subprocess

argv = sys.argv[sys.argv.index('--') + 1:]
body_p, tex_in, tex_out = (os.path.abspath(a) for a in argv[:3])
rest = argv[3:]
def arg(name, n=1, default=None):
    if name not in rest:
        return default
    i = rest.index(name)
    return rest[i + 1] if n == 1 else rest[i + 1:i + 1 + n]
hx = lambda h: tuple(int(h.lstrip('#')[i:i + 2], 16) for i in (0, 2, 4))
SKIN = hx(arg('--pele', default='#e49d6f'))
CLOTH = [hx(h) for h in arg('--roupa', default='#eb2f14').split(',')]
MIX = [hx(h) for h in arg('--mistura', default='').split(',') if h]
Z0, Z1 = (float(v) for v in arg('--z', 2, ['1.08', '1.42']))
NEW_SKIN = arg('--nova-pele')

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=body_p)
arm = next(o for o in bpy.context.scene.objects if o.type == 'ARMATURE')
body = max((o for o in arm.children if o.type == 'MESH'), key=lambda o: len(o.data.vertices))
me, mw = body.data, body.matrix_world
img = bpy.data.images.load(tex_in)
W, H = img.size
px = img.pixels[:]
uvl = me.uv_layers.active.data
near = lambda c, t: sum((a - b) ** 2 for a, b in zip(c, t)) < 18 ** 2


def sample(u, v):
    k = (min(H - 1, max(0, int(v * H))) * W + min(W - 1, max(0, int(u * W)))) * 4
    return tuple(int(x * 255 + 0.5) for x in px[k:k + 3])


tris, n_skin, n_cloth = [], 0, 0
for p in me.polygons:
    q = mw @ p.center
    if not (Z0 < q.z < Z1):
        continue
    uvs = [tuple(uvl[l].uv) for l in p.loop_indices]
    cu = sum(u for u, _ in uvs) / len(uvs); cv = sum(v for _, v in uvs) / len(uvs)
    # amostra nos cantos (puxados um pouco para dentro), nos meios das arestas e no centro
    pts = [(cu + (u - cu) * 0.8, cv + (v - cv) * 0.8) for u, v in uvs]
    pts += [((uvs[i][0] + uvs[(i + 1) % len(uvs)][0]) / 2 * 0.8 + cu * 0.2, (uvs[i][1] + uvs[(i + 1) % len(uvs)][1]) / 2 * 0.8 + cv * 0.2) for i in range(len(uvs))]
    pts.append((cu, cv))
    cols = [sample(u, v) for u, v in pts]
    s = sum(near(c, SKIN) for c in cols)
    cl = [c for c in cols if any(near(c, t) for t in CLOTH)]
    m = sum(any(near(c, t) for t in MIX) for c in cols)
    if not ((s and cl) or m):
        continue                                  # face limpa: não mexe
    if len(cl) > s:
        best = max(CLOTH, key=lambda t: sum(near(c, t) for c in cl)); n_cloth += 1
    else:
        best = SKIN; n_skin += 1
    tris.append([[list(uv) for uv in uvs], list(best)])
BAR = arg('--barra', 3)
if BAR:
    import math
    bar_c, bar_h, main_c = hx(BAR[0]), float(BAR[1]), hx(BAR[2])
    cloth_faces = []
    painted = {i for i, t in enumerate(tris)}
    for p in me.polygons:
        q = mw @ p.center
        if not (Z0 < q.z < Z1):
            continue
        uvs = [tuple(uvl[l].uv) for l in p.loop_indices]
        cu = sum(u for u, _ in uvs) / len(uvs); cv = sum(v for _, v in uvs) / len(uvs)
        c = sample(cu, cv)
        if any(near(c, t) for t in CLOTH):
            cloth_faces.append((q, uvs))
    # borda de baixo por setor angular (em volta do eixo do tronco)
    NS = 36
    low = {}
    for q, _ in cloth_faces:
        sct = int((math.atan2(q.y, q.x) + math.pi) / (2 * math.pi) * NS) % NS
        low[sct] = min(low.get(sct, 9), q.z)
    for q, uvs in cloth_faces:
        sct = int((math.atan2(q.y, q.x) + math.pi) / (2 * math.pi) * NS) % NS
        zl = min(low.get((sct + d) % NS, 9) for d in (-1, 0, 1))
        tris.append([[list(uv) for uv in uvs], list(bar_c if q.z < zl + bar_h else main_c)])
    print('Barra:', len(cloth_faces), 'faces de roupa recoloridas (faixa', bar_h, ')')
    # barra reta: altura da borda por setor (percentil baixo, suavizada); perto dela, acima = roupa, abaixo = pele
    zs = {}
    for q, _ in cloth_faces:
        if math.hypot(q.x, q.y) < 0.2 and q.z < 1.25:
            zs.setdefault(int((math.atan2(q.y, q.x) + math.pi) / (2 * math.pi) * NS) % NS, []).append(q.z)
    pct = {k: sorted(v)[len(v) // 25] for k, v in zs.items()}   # quase o mínimo: a borda de verdade (sem pingos soltos)
    hem = {k: sum(pct.get((k + d) % NS, pct[k]) for d in (-2, -1, 0, 1, 2)) / 5 for k in pct}
    nfix = 0
    gs = body.vertex_groups
    armg = {gs[n].index for n in gs.keys() if 'Arm' in n or 'Hand' in n or 'Shoulder' in n}
    for p in me.polygons:
        q = mw @ p.center
        if math.hypot(q.x, q.y) > 0.2:
            continue
        if max(sum(ge.weight for ge in me.vertices[i].groups if ge.group in armg) for i in p.vertices) > 0.2:
            continue                              # braço: fica como está
        sct = int((math.atan2(q.y, q.x) + math.pi) / (2 * math.pi) * NS) % NS
        if sct not in hem or not (hem[sct] - 0.015 < q.z < hem[sct] + 0.06):
            continue
        tris.append([[list(uvl[l].uv) for l in p.loop_indices], list(main_c if q.z >= hem[sct] else SKIN)]); nfix += 1
    print('Barra reta:', nfix, 'faces')
print('Borda limpa:', len(tris), 'faces (', n_cloth, 'roupa,', n_skin, 'pele)')
pj = tex_out[:-4] + '_tris.json'
json.dump({'src': tex_in, 'tris': tris}, open(pj, 'w'))
here = os.path.dirname(os.path.abspath(__file__))
subprocess.run(['python', os.path.join(here, '..', 'pintar_uv.py'), pj, tex_out], check=True)
os.remove(pj)
if NEW_SKIN:
    code = ("from PIL import Image\nimport numpy as np\nim=np.asarray(Image.open(r'%s').convert('RGBA')).astype(int)\n"
            "d=((im[...,:3]-np.array(%s))**2).sum(2)<18**2\nim[d,:3]=np.array(%s)\nImage.fromarray(im.astype('uint8')).save(r'%s')\nprint('pele trocada:',int(d.sum()),'texels')"
            % (tex_out, list(SKIN), list(hx(NEW_SKIN)), tex_out))
    subprocess.run(['python', '-c', code], check=True)
