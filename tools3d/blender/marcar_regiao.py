"""Marca uma região do corpo (atributo de vértice) por "inundação" a partir de uma semente, parando nas quinas vivas.

  blender -b --factory-startup -P tools3d/blender/marcar_regiao.py -- corpo.glb saida.glb --nome _SHIRT
          --semente 0 -0.3 0.80 --caixa -0.2 0.2 0.58 1.02 --angulo 45 [--cores "#4d6e3f,#36522a"]

Uso típico: a camisa do Jack tem a MESMA cor do casaco, então não dá para separar por cor. A camisa fica no vão do
casaco aberto e a beira do casaco é uma quina viva: a inundação começa no peito e para nessa quina, pegando só a
camisa. O visualizador recolore o que tiver o atributo (ex.: camisa mostarda).
"""
import bpy, bmesh, os, sys, math
from mathutils import Vector

argv = sys.argv[sys.argv.index('--') + 1:]
src, out = os.path.abspath(argv[0]), os.path.abspath(argv[1])
name = argv[argv.index('--nome') + 1]
seed = Vector(tuple(float(x) for x in argv[argv.index('--semente') + 1:argv.index('--semente') + 4]))
bx = [float(x) for x in argv[argv.index('--caixa') + 1:argv.index('--caixa') + 5]]   # xmin xmax zmin zmax
ang = math.radians(float(argv[argv.index('--angulo') + 1])) if '--angulo' in argv else math.radians(45)
cols = [tuple(int(h.strip().lstrip('#')[i:i + 2], 16) for i in (0, 2, 4)) for h in argv[argv.index('--cores') + 1].split(',')] if '--cores' in argv else None

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=src)
arm = next(o for o in bpy.context.scene.objects if o.type == 'ARMATURE')
body = max((o for o in arm.children if o.type == 'MESH'), key=lambda o: len(o.data.vertices))
me, mw = body.data, body.matrix_world
img = next((n.image for m in me.materials if m and m.use_nodes for n in m.node_tree.nodes if n.type == 'TEX_IMAGE' and n.image), None)
W, H = img.size
px = img.pixels[:]

bm = bmesh.new()
bm.from_mesh(me)
orig_index = bm.verts.layers.int.new('orig')
for v in bm.verts:
    v[orig_index] = v.index
bmesh.ops.remove_doubles(bm, verts=bm.verts[:], dist=1e-5)   # costuras soldadas: a inundação atravessa as emendas do UV
bm.faces.ensure_lookup_table()
uv = bm.loops.layers.uv.active


def face_ok(f):
    c = mw @ f.calc_center_median()
    if not (bx[0] <= c.x <= bx[1] and bx[2] <= c.z <= bx[3]):
        return False
    if cols:
        u = sum(l[uv].uv.x for l in f.loops) / len(f.loops)
        v = sum(l[uv].uv.y for l in f.loops) / len(f.loops)
        k = (min(H - 1, int(v * H)) * W + min(W - 1, int(u * W))) * 4
        cc = [x * 255 for x in px[k:k + 3]]
        return min(sum((a - b) ** 2 for a, b in zip(cc, q)) for q in cols) < 30 ** 2
    return True


start = min((f for f in bm.faces if face_ok(f)), key=lambda f: ((mw @ f.calc_center_median()) - seed).length)
mark, stack = {start}, [start]
while stack:
    f = stack.pop()
    for e in f.edges:
        for g in e.link_faces:
            if g in mark or not face_ok(g):
                continue
            if f.normal.angle(g.normal) > ang:      # quina viva (beira do casaco): para aqui
                continue
            mark.add(g)
            stack.append(g)
verts_orig = set()
merged = {v.index: [] for v in bm.verts}
print('Região', name, ':', len(mark), 'faces')
# volta para os índices da malha original (vários vértices originais por vértice soldado)
bm2 = bmesh.new()
bm2.from_mesh(me)
key_of = {v.co.to_tuple(5): v.index for v in bm.verts}
marked_pos = {v.co.to_tuple(5) for f in mark for v in f.verts}
# valor = 0.01 na borda da região até 1.0 a uns 5 cm para dentro: o visualizador usa isso como sombra de contato
# (a camisa escurece perto da beira do casaco, como se o casaco fizesse sombra nela)
border = set()
for f in mark:
    for e in f.edges:
        if any(g not in mark for g in e.link_faces) or len(e.link_faces) < 2:
            border |= {v.co.to_tuple(5) for v in e.verts}
from mathutils import kdtree
kd = kdtree.KDTree(len(border))
for i, c in enumerate(border):
    kd.insert(c, i)
kd.balance()
FALL = float(argv[argv.index('--sombra') + 1]) if '--sombra' in argv else 0.05


def val(co):
    k = co.to_tuple(5)
    if k not in marked_pos:
        return 0.0
    d = kd.find(co)[2] if border else FALL
    return 0.01 + 0.99 * min(1.0, d / FALL)


vals = [val(v.co) for v in me.vertices]
bm.free(); bm2.free()
a = me.attributes.new(name, 'FLOAT', 'POINT')
a.data.foreach_set('value', vals)
bpy.ops.export_scene.gltf(filepath=out, export_format='GLB', export_extras=True, export_attributes=True)
print('Exportado:', out, '| vértices marcados:', sum(1 for x in vals if x > 0))
