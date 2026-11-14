"""Repinta a bermuda do King Frog limpa, por face (a textura do Meshy ali é uma colcha de manchas).

  blender -b --factory-startup -P tools3d/blender/pintar_bermuda.py -- corpo.glb saida.glb textura_saida.png

1. Separa bermuda x pele das pernas pela cor de cada face, e passa um filtro de maioria pelas vizinhas
   (mancha verde dentro da bermuda vira bermuda, mancha escura na coxa vira pele).
2. Pinta como na folha: verde-escuro, faixa preta na lateral de fora (inclui as franjas rasgadas),
   barra marrom embaixo (só onde encosta na perna, fora da faixa lateral).
3. Barriga: restos amarelados acima do cinturão viram verde de barriga.
"""
import bpy, os, sys, math, json, subprocess
from mathutils import Vector

argv = sys.argv[sys.argv.index('--') + 1:]
src, out, tex_out = (os.path.abspath(a) for a in argv[:3])
GREEN, BLACK, BROWN, BELLY = (60, 99, 68), (32, 30, 31), (61, 47, 38), (131, 164, 93)
SKIN = [(131, 164, 93), (157, 188, 121), (107, 134, 79)]
YELLOW = [(217, 175, 77), (199, 177, 101), (216, 134, 60), (217, 134, 77)]

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=src)
scene = bpy.context.scene
arm = next(o for o in scene.objects if o.type == 'ARMATURE')
body = next(o for o in arm.children if o.type == 'MESH' and not o.name.startswith('belt'))
me, mw = body.data, body.matrix_world
img = next(n.image for m in me.materials if m and m.use_nodes for n in m.node_tree.nodes if n.type == 'TEX_IMAGE' and n.image)
W, H = img.size
px = img.pixels[:]
uvl = me.uv_layers.active.data
gidx = {g.index: g.name for g in body.vertex_groups}
LOWER = {'Hips', 'LeftUpLeg', 'RightUpLeg', 'Spine'}


def sample(u, v):
    k = (min(H - 1, max(0, int(v * H))) * W + min(W - 1, max(0, int(u * W)))) * 4
    return [x * 255 for x in px[k:k + 3]]


def d2(a, b):
    return sum((x - y) ** 2 for x, y in zip(a, b))


def face_color(poly):
    cs = [sample(*uvl[l].uv) for l in poly.loop_indices]
    u = sum(uvl[l].uv.x for l in poly.loop_indices) / poly.loop_total
    v = sum(uvl[l].uv.y for l in poly.loop_indices) / poly.loop_total
    return cs + [sample(u, v)]


lower_v = {v.index for v in me.vertices if sum(ge.weight for ge in v.groups if gidx.get(ge.group) in LOWER) >= 0.5}
# 1. bermuda x pele
cand, is_shorts = [], {}
for poly in me.polygons:
    p = mw @ poly.center
    if 0.42 < p.z < 1.02 and all(vi in lower_v for vi in poly.vertices):
        cs = face_color(poly)
        skin_votes = sum(min(d2(c, s) for s in SKIN) < min(d2(c, s) for s in [GREEN, BLACK, BROWN] + YELLOW) for c in cs)
        is_shorts[poly.index] = skin_votes * 2 < len(cs)
        cand.append(poly.index)
edge_faces = {}
for i in cand:
    for ek in me.polygons[i].edge_keys:
        edge_faces.setdefault(ek, []).append(i)
nbrs = {i: [f for ek in me.polygons[i].edge_keys for f in edge_faces[ek] if f != i] for i in cand}
for _ in range(3):   # maioria das vizinhas
    new = {}
    for i in cand:
        nb = [is_shorts[f] for f in nbrs[i]]
        if len(nb) >= 2:
            s = sum(nb)
            new[i] = True if s * 2 > len(nb) else False if s * 2 < len(nb) else is_shorts[i]
        else:
            new[i] = is_shorts[i]
    is_shorts = new
shorts = [i for i in cand if is_shorts[i]]
print('Faces da bermuda:', len(shorts), 'de', len(cand))

# 2. cores: lateral de fora preta, barra marrom onde a bermuda encosta na perna, resto verde
label = {}
for i in shorts:
    poly = me.polygons[i]
    p = mw @ poly.center
    n = (mw.to_3x3() @ poly.normal).normalized()
    label[i] = 'side' if (abs(p.x) > 0.25 and n.x * p.x > 0 and abs(n.x) > 0.82) else 'green'
for _ in range(4):   # maioria das vizinhas: a borda preto/verde fica uma linha limpa, sem ilhas soltas
    new = {}
    for i in shorts:
        nb = [label[f] for f in nbrs[i] if f in label]
        new[i] = max(set(nb), key=nb.count) if len(nb) >= 2 and nb.count(label[i]) * 2 < len(nb) else label[i]
    label = new
for i in shorts:
    p = mw @ me.polygons[i].center
    if label[i] == 'green' and p.z < 0.8 and any(not is_shorts.get(f, False) and (mw @ me.polygons[f].center).z < p.z for f in nbrs[i]):
        label[i] = 'hem'
COLS = {'green': GREEN, 'side': GREEN, 'hem': GREEN, 'skin': SKIN[0], 'belly': BELLY}   # toda verde: a faixa lateral entortava nas franjas rasgadas da malha
paint = {i: label[i] for i in shorts}
for i in cand:
    if not is_shorts[i]:
        paint[i] = 'skin'
# 3. barriga: resto amarelado no tronco
yel = 0
for poly in me.polygons:
    p = mw @ poly.center
    if 0.95 < p.z < 1.5 and math.hypot(p.x, p.y) < 0.6:
        cs = face_color(poly)
        torso = all(vi in lower_v or (mw @ me.vertices[vi].co).z > 1.0 for vi in poly.vertices)
        band = p.z < 1.26 and torso and poly.index not in paint   # faixa da barriga atrás/embaixo do cinturão: tudo chapado
        if band or sum(min(d2(c, y) for y in YELLOW) < 55 ** 2 for c in cs) >= 1:
            paint[poly.index] = 'belly'; yel += 1
print('Pintura:', {k: list(paint.values()).count(k) for k in COLS}, '| amarelo na barriga:', yel)

# materiais de cor chapada em vez de pintar a textura: a textura do Meshy é picotada em ilhas minúsculas e o
# mipmap mistura a cor das ilhas vizinhas (era daí o amarelo e as manchas que voltavam na bermuda)
slot = {}
for k, c in COLS.items():
    m = bpy.data.materials.new('bermuda_' + k)
    m.use_nodes = True
    bsdf = m.node_tree.nodes['Principled BSDF']
    bsdf.inputs['Base Color'].default_value = (*[(x / 255) ** 2.2 for x in c], 1)
    bsdf.inputs['Roughness'].default_value = 1.0
    me.materials.append(m)
    slot[k] = len(me.materials) - 1
for i, k in paint.items():
    me.polygons[i].material_index = slot[k]
bpy.ops.export_scene.gltf(filepath=out, export_format='GLB', export_extras=True)
print('Exportado:', out)
