"""Fivela do Jack modelada em código (a do Meshy saiu borrada e torta, girada uns 20°).

  blender -b --factory-startup -P tools3d/blender/make_jack_buckle.py -- corpo.glb saida.glb

1. Acha a fivela do Meshy pela cor dourada na frente do cinto e afunda ela até a superfície do cinto.
2. Põe no lugar uma fivela reta: moldura dourada retangular com cantos chanfrados, janela escura (o cinto aparece
   por trás) e a barra/língua no meio, como na folha. Quinas vivas -> o contorno de vinco do visualizador desenha
   as bordas em preto sozinho.
3. Presa 100% no osso do quadril (não deforma, não balança).
"""
import bpy, bmesh, os, sys, math
from mathutils import Vector

argv = sys.argv[sys.argv.index('--') + 1:]
src, out = os.path.abspath(argv[0]), os.path.abspath(argv[1])
GOLD = (201, 165, 74)

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=src)
arm = next(o for o in bpy.context.scene.objects if o.type == 'ARMATURE')
body = max((o for o in arm.children if o.type == 'MESH'), key=lambda o: len(o.data.vertices))
me, mw = body.data, body.matrix_world
img = next(n.image for m in me.materials if m and m.use_nodes for n in m.node_tree.nodes if n.type == 'TEX_IMAGE' and n.image)
W, H = img.size
px = img.pixels[:]
uvl = me.uv_layers.active.data

gold_v, pts = set(), []
for p in me.polygons:
    u = sum(uvl[l].uv.x for l in p.loop_indices) / p.loop_total
    v = sum(uvl[l].uv.y for l in p.loop_indices) / p.loop_total
    k = (min(H - 1, int(v * H)) * W + min(W - 1, int(u * W))) * 4
    c = [x * 255 for x in px[k:k + 3]]
    pc = mw @ p.center
    if abs(pc.x) < 0.09 and 0.48 < pc.z < 0.68 and pc.y < 0 and sum((a - b) ** 2 for a, b in zip(c, GOLD)) < 40 ** 2:
        gold_v |= set(p.vertices)
        pts.append(pc)
xs, zs = [p.x for p in pts], [p.z for p in pts]
ys = sorted(p.y for p in pts)
front_y = ys[len(ys) // 2]   # frente típica da fivela velha (mediana)
cx, cz = (min(xs) + max(xs)) / 2, (min(zs) + max(zs)) / 2
# superfície do cinto logo ao lado da fivela (onde ela vai encostar)
side = [mw @ v.co for v in me.vertices if abs((mw @ v.co).z - cz) < 0.02 and 0.07 < abs((mw @ v.co).x - cx) < 0.11 and (mw @ v.co).y < 0]
belt_y = sum(p.y for p in side) / len(side) if side else min(p.y for p in pts) + 0.01
print('Fivela do Meshy:', len(pts), 'faces; centro x', round(cx, 3), 'z', round(cz, 3), '| cinto em y', round(belt_y, 3))

# 1. afunda a fivela velha até o cinto (e os gêmeos de costura juntos)
inv = mw.inverted()
orig_key = [v.co.to_tuple(5) for v in me.vertices]   # chave antes de mexer (mexer muda a coordenada)
twins = {}
for i, k in enumerate(orig_key):
    twins.setdefault(k, []).append(i)
moved = set()
for vi in list(gold_v):
    for t in twins[orig_key[vi]]:
        if t in moved:
            continue
        p = mw @ me.vertices[t].co
        if p.y < front_y + 0.008:
            p.y = front_y + 0.008
            me.vertices[t].co = inv @ p
        moved.add(t)

# a fivela velha vira cinto: material chapado verde-escuro nas faces douradas
BELT = bpy.data.materials.new('cinto_jack')
BELT.use_nodes = True
BELT.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value = (*[(c / 255) ** 2.2 for c in (38, 64, 29)], 1)
BELT.node_tree.nodes['Principled BSDF'].inputs['Roughness'].default_value = 1.0
me.materials.append(BELT)
bi = len(me.materials) - 1
for p in me.polygons:
    if set(p.vertices) & gold_v and all(v in gold_v or True for v in p.vertices):
        pc = mw @ p.center
        if abs(pc.x) < 0.09 and 0.48 < pc.z < 0.68:
            p.material_index = bi

# 2. fivela nova (frente = -Y)
BW, BH, T = 0.115, 0.088, 0.016          # largura, altura, espessura da moldura
WW, WH = 0.062, 0.042                     # janela
y0 = front_y + 0.004                      # costas logo atrás da frente da fivela velha: nada aparece entre as duas


def mat(name, rgb):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    b = m.node_tree.nodes['Principled BSDF']
    b.inputs['Base Color'].default_value = (*[(c / 255) ** 2.2 for c in rgb], 1)
    b.inputs['Roughness'].default_value = 1.0
    return m


GOLDM, DARK = mat('fivela_ouro', (214, 172, 66)), mat('fivela_fundo', (38, 64, 29))
bm = bmesh.new()


def box(x0, x1, z0, z1, ya, yb, mi, chamfer=0.0):
    """Caixa (com chanfro nos cantos da frente, opcional)."""
    c = chamfer
    front = [(x0 + c, z0), (x1 - c, z0), (x1, z0 + c), (x1, z1 - c), (x1 - c, z1), (x0 + c, z1), (x0, z1 - c), (x0, z0 + c)] if c else [(x0, z0), (x1, z0), (x1, z1), (x0, z1)]
    vb = [bm.verts.new((x, ya, z)) for x, z in front]
    vf = [bm.verts.new((x, yb, z)) for x, z in front]
    bm.faces.new(vb).material_index = mi
    bm.faces.new(list(reversed(vf))).material_index = mi
    n = len(front)
    for i in range(n):
        bm.faces.new((vb[i], vb[(i + 1) % n], vf[(i + 1) % n], vf[i])).material_index = mi


# fundo escuro (o cinto visto pela janela) e a moldura em 4 barras
box(cx - BW / 2 + 0.004, cx + BW / 2 - 0.004, cz - BH / 2 + 0.004, cz + BH / 2 - 0.004, y0, y0 - 0.004, 1)
fx0, fx1, fz0, fz1 = cx - BW / 2, cx + BW / 2, cz - BH / 2, cz + BH / 2
wx0, wx1, wz0, wz1 = cx - WW / 2, cx + WW / 2, cz - WH / 2, cz + WH / 2
yb = y0 - T
box(fx0, fx1, wz1, fz1, y0, yb, 0)      # cima
box(fx0, fx1, fz0, wz0, y0, yb, 0)      # baixo
box(fx0, wx0, wz0, wz1, y0, yb, 0)      # esquerda
box(wx1, fx1, wz0, wz1, y0, yb, 0)      # direita
# língua: barra que entra pela janela vinda da direita (o "C" da folha)
box(cx - 0.004, wx1 + 0.002, cz - 0.009, cz + 0.009, y0 - 0.003, y0 - T * 0.75, 0)
bmesh.ops.remove_doubles(bm, verts=bm.verts[:], dist=1e-6)
bm.normal_update()
bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
mesh = bpy.data.meshes.new('buckle_jack')
bm.to_mesh(mesh)
bm.free()
for m in (GOLDM, DARK):
    mesh.materials.append(m)
for p in mesh.polygons:
    p.use_smooth = False
buckle = bpy.data.objects.new('buckle_jack', mesh)
bpy.context.scene.collection.objects.link(buckle)
buckle.parent = arm
buckle.matrix_parent_inverse = arm.matrix_world.inverted()
g = buckle.vertex_groups.new(name='Hips')
g.add(list(range(len(mesh.vertices))), 1.0, 'REPLACE')
md = buckle.modifiers.new('esqueleto', 'ARMATURE')
md.object = arm
print('Fivela nova:', len(mesh.polygons), 'faces, presa no quadril')
bpy.ops.export_scene.gltf(filepath=out, export_format='GLB', export_extras=True, export_attributes=True)
print('Exportado:', out)
