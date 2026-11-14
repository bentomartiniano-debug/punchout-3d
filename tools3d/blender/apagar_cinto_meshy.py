"""Some com o cinturão que o Meshy esculpiu no corpo (o cinturão de verdade agora é peça própria).

  blender -b --factory-startup -P tools3d/blender/apagar_cinto_meshy.py -- corpo.glb saida.glb textura_saida.png

1. Faces douradas da cintura: os vértices afundam até o raio da pele logo acima/abaixo (some o relevo).
2. Na textura, cada face dourada é repintada: metade de cima vira barriga, metade de baixo vira bermuda
   (o desenho dos triângulos no UV é feito pelo python do sistema, que tem PIL).
"""
import bpy, bmesh, os, sys, math, json, subprocess
from mathutils import Vector

# funções iguais às do postprocess.py (ele não pode ser importado: roda ao carregar)
def smooth_region(obj, verts_idx, iterations, factor=0.5):
    """Alisa só os vértices escolhidos; a borda da região fica presa para não abrir degrau."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bm.verts.ensure_lookup_table()
    keep = {v.index for v in bm.verts if v.index in set(verts_idx)}
    coords = {v.co.to_tuple(5) for v in bm.verts if v.index in keep}
    bmesh.ops.remove_doubles(bm, verts=bm.verts[:], dist=1e-5)   # costuras do glTF: solda antes de alisar (UV fica nos cantos das faces)
    bm.verts.ensure_lookup_table()
    sel = {v.index for v in bm.verts if v.co.to_tuple(5) in coords}
    inner = [bm.verts[i] for i in sel if all(e.other_vert(bm.verts[i]).index in sel for e in bm.verts[i].link_edges)]
    for _ in range(iterations):
        bmesh.ops.smooth_vert(bm, verts=inner, factor=factor, use_axis_x=True, use_axis_y=True, use_axis_z=True)
    bm.to_mesh(obj.data)
    bm.free()
    obj.data.update()

def group_weights(obj, name):
    g = obj.vertex_groups.get(name)
    if not g:
        return {}
    return {v.index: ge.weight for v in obj.data.vertices for ge in v.groups if ge.group == g.index}


def soft_normals(obj, iterations):
    """Normais tiradas de uma cópia bem alisada da malha; a malha em si não muda (silhueta e relevo ficam)."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bm.verts.ensure_lookup_table()
    key = [v.co.to_tuple(5) for v in bm.verts]
    bmesh.ops.remove_doubles(bm, verts=bm.verts[:], dist=1e-5)
    for _ in range(iterations):
        bmesh.ops.smooth_vert(bm, verts=bm.verts[:], factor=0.5, use_axis_x=True, use_axis_y=True, use_axis_z=True)
    # posição original -> normal da cópia alisada (o remove_doubles guarda a coordenada do vértice que sobrou)
    bm2 = bmesh.new()
    bm2.from_mesh(obj.data)
    bmesh.ops.remove_doubles(bm2, verts=bm2.verts[:], dist=1e-5)
    bm2.verts.ensure_lookup_table()
    bm.verts.ensure_lookup_table()
    bm.normal_update()
    by_pos = {v2.co.to_tuple(5): bm.verts[v2.index].normal.copy() for v2 in bm2.verts}
    normals = [by_pos.get(k) or obj.data.vertices[i].normal for i, k in enumerate(key)]
    bm.free(); bm2.free()
    obj.data.normals_split_custom_set_from_vertices([n.normalized() for n in normals])
    print('Normais suaves:', obj.name, iterations, 'passadas')


argv = sys.argv[sys.argv.index('--') + 1:]
src, out, tex_out = (os.path.abspath(a) for a in argv[:3])
GOLD = (217, 175, 77)
BELLY, SHORTS = (131, 164, 93), (60, 99, 68)

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


def sample(u, v):
    k = (min(H - 1, max(0, int(v * H))) * W + min(W - 1, max(0, int(u * W)))) * 4
    return [x * 255 for x in px[k:k + 3]]


# a faixa inteira que o cinturão novo cobre (por trás ele vai de 0.925 a 1.131): tudo nela vira pele lisa,
# sem depender da cor (o laranja e o marrom de sombra do Meshy misturavam com o dourado)
Z_LO, Z_HI = float(os.environ.get('CINTO_Z_LO', 0.9)), float(os.environ.get('CINTO_Z_HI', 1.125))
TORSO = {'Hips', 'Spine', 'Spine01', 'Spine02', 'LeftUpLeg', 'RightUpLeg'}
gidx = {g.index: g.name for g in body.vertex_groups}
torso_v = {v.index for v in me.vertices if sum(ge.weight for ge in v.groups if gidx.get(ge.group) in TORSO) >= 0.55}
gold_faces = []
for poly in me.polygons:
    p = mw @ poly.center
    if not all(vi in torso_v for vi in poly.vertices):
        continue                                   # braço, axila e mão ficam de fora (eram eles que rasgavam)
    u = sum(uvl[l].uv.x for l in poly.loop_indices) / poly.loop_total
    v = sum(uvl[l].uv.y for l in poly.loop_indices) / poly.loop_total
    c = sample(u, v)
    # dourado do cinto e as sombras laranja/marrom que o Meshy pintou em volta dele
    gold = any(sum((a - b) ** 2 for a, b in zip(c, ref)) < 45 ** 2 for ref in (GOLD, (216, 134, 60), (199, 177, 101), (61, 47, 38)))
    if Z_LO < p.z < Z_HI or (gold and 0.8 < p.z < 1.3):
        gold_faces.append(poly.index)
z_mid = (Z_LO + Z_HI) / 2
print('Faces do cinto do Meshy:', len(gold_faces), 'meio em z', round(z_mid, 3))

# 2. pintura no UV, calculada antes de mexer na malha (o alisamento solda vértices e muda os índices)
tris = []
for i in gold_faces:
    poly = me.polygons[i]
    col = BELLY if (mw @ poly.center).z > (Z_LO + Z_HI) / 2 else SHORTS
    tris.append([[list(uvl[l].uv) for l in poly.loop_indices], col])

# 1. afunda: raio por fatia de ângulo, medido na pele sem dourado logo acima e abaixo
gold_v = {vi for i in gold_faces for vi in me.polygons[i].vertices}
# gêmeos de costura: o glTF duplica o vértice nas emendas do UV; os dois precisam andar juntos, senão a
# emenda abre e vira furo (dá para ver o outro lado)
_pos = {}
for v in me.vertices:
    _pos.setdefault(v.co.to_tuple(5), []).append(v.index)
gold_v = {t for vi in gold_v for t in _pos[me.vertices[vi].co.to_tuple(5)]}
ring = [mw @ v.co for v in me.vertices if abs((mw @ v.co).z - z_mid) < 0.05]
cy = (min(p.y for p in ring) + max(p.y for p in ring)) / 2 if ring else 0.0
BINS = 72
above, below = [[] for _ in range(BINS)], [[] for _ in range(BINS)]
binof = lambda p: int((math.atan2(p.x, -(p.y - cy)) + math.pi) / (2 * math.pi) * BINS) % BINS
for v in me.vertices:
    if v.index in gold_v:
        continue
    p = mw @ v.co
    if math.hypot(p.x, p.y) > 0.62:
        continue
    if Z_HI + 0.005 < p.z < Z_HI + 0.06:
        above[binof(p)].append(math.hypot(p.x, p.y - cy))
    elif Z_LO - 0.06 < p.z < Z_LO - 0.005:
        below[binof(p)].append(math.hypot(p.x, p.y - cy))
med = lambda r: sorted(r)[len(r) // 2] if r else None
up, dn = [med(r) for r in above], [med(r) for r in below]
inv = mw.inverted()
moved = 0
for vi in gold_v:
    v = me.vertices[vi]
    p = mw @ v.co
    i = binof(p)
    ru, rd = up[i] or dn[i], dn[i] or up[i]
    if not ru:
        continue
    t = min(1, max(0, (p.z - Z_LO) / (Z_HI - Z_LO)))
    r_new = (rd + (ru - rd) * t) * 0.97        # pele lisa ligando a barriga à bermuda
    r = math.hypot(p.x, p.y - cy)
    if r > r_new:                               # só afunda (empurrar para fora fazia aletas)
        k = max(r_new, r - 0.04) / r              # no máximo 4 cm: afundar demais dobrava a pele e abria furo
        v.co = inv @ Vector((p.x * k, cy + (p.y - cy) * k, p.z))
        moved += 1
print('Vértices afundados:', moved)
# alisa a faixa com a malha soldada (sem soldar, as costuras do glTF rasgavam a pele) e a borda presa
def flipped():
    n = 0
    for poly in me.polygons:
        p = mw @ poly.center
        if 0.75 < p.z < 1.4 and all(vi in torso_v for vi in poly.vertices):
            rad = Vector((p.x, p.y - cy, 0)).normalized()
            n += (mw.to_3x3() @ poly.normal).dot(rad) < -0.6
    return n
print('Faces viradas para dentro antes de alisar:', flipped())
smooth_region(body, list(gold_v), 10, 0.4)
# o alisamento mexeu nos vértices: refaz as normais suaves do sombreado de quadrinho
soft_normals(body, 25)

pj = tex_out[:-4] + '_tris.json'
src_png = tex_out[:-4] + '_src.png'
img.filepath_raw = src_png
img.file_format = 'PNG'
img.save()
with open(pj, 'w') as fh:
    json.dump({'src': src_png, 'tris': tris}, fh)
here = os.path.dirname(os.path.abspath(__file__))
subprocess.run(['python', os.path.join(here, '..', 'pintar_uv.py'), pj, tex_out], check=True)
os.remove(pj); os.remove(src_png)
img2 = bpy.data.images.load(tex_out)
for m in me.materials:
    if m and m.use_nodes:
        for n in m.node_tree.nodes:
            if n.type == 'TEX_IMAGE':
                n.image = img2
bpy.ops.export_scene.gltf(filepath=out, export_format='GLB', export_extras=True)
print('Exportado:', out)
