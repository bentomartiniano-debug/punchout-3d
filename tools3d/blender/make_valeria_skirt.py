"""Saia de babados da Valéria em código: camadas (laranja, amarelo, turquesa — cores do sprite e da folha) da cintura
até a barra do short, cada uma um tronco de cone com pregas e barra ondulada, um pouco mais larga que a de cima (fica em
camadas). Sai de dentro do corpo e grava _SWAY_HAIR (0 na cintura, 1 na barra) para o balanço.

  blender -b --factory-startup -P tools3d/blender/make_valeria_skirt.py -- corpo.glb saida.glb [--cintura 0.97] [--barra 0.71]
"""
import bpy, bmesh, os, sys, math
from mathutils import Vector
from mathutils.bvhtree import BVHTree

argv = sys.argv[sys.argv.index('--') + 1:]
body_p, out = os.path.abspath(argv[0]), os.path.abspath(argv[1])
opt = dict(zip(argv[2::2], argv[3::2]))
ZW, ZH = float(opt.get('--cintura', 0.97)), float(opt.get('--barra', 0.71))

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=body_p)
arm = next(o for o in bpy.context.scene.objects if o.type == 'ARMATURE')
body = max((o for o in arm.children if o.type == 'MESH'), key=lambda o: len(o.data.vertices))
mw = body.matrix_world
dg = bpy.context.evaluated_depsgraph_get()
bmb = bmesh.new(); bmb.from_object(body, dg); bmb.transform(mw); tree = BVHTree.FromBMesh(bmb)
P = [mw @ v.co for v in body.data.vertices]


def section(z):
    """meia largura / meia profundidade do corpo na altura z (só tronco e coxas: sem as mãos)"""
    b = [q for q in P if abs(q.z - z) < 0.012 and abs(q.x) < 0.27]
    return (max(abs(q.x) for q in b), (max(q.y for q in b) - min(q.y for q in b)) / 2, (max(q.y for q in b) + min(q.y for q in b)) / 2)


rxw, ryw, cyw = section(ZW)
rxh = max(section(z)[0] for z in (ZH + 0.02, ZH + 0.06, ZH + 0.1))
ryh = max(section(z)[1] for z in (ZH + 0.02, ZH + 0.06, ZH + 0.1))
print('Cintura', round(rxw, 3), round(ryw, 3), '| quadril/coxas', round(rxh, 3), round(ryh, 3))

# raio do corpo por ângulo e altura (em volta do centro da cintura), suavizado: cada anel passa por fora dele (saia lisa)
NA = 72
ZBIN = 0.02
polar = {}
for q in P:
    if abs(q.x) > 0.27 or not (ZH - 0.05 < q.z < ZW + 0.03):
        continue
    a = math.atan2((q.y - cyw), q.x); ia = int(round(a / (2 * math.pi) * NA)) % NA
    key = (ia, int(q.z / ZBIN))
    rr = math.hypot(q.x, q.y - cyw)
    if rr > polar.get(key, 0):
        polar[key] = rr
def body_r(ia, z):
    zb = int(z / ZBIN); best = 0
    for d in range(-3, 4):                     # vizinhos em ângulo e altura: contorno suave
        for dz in (-1, 0, 1):
            best = max(best, polar.get(((ia + d) % NA, zb + dz), 0) * (1 - 0.01 * abs(d)))
    return best
TIERS = [((1.0, 0.42, 0.06), 0.0, 0.42), ((1.0, 0.80, 0.12), 0.3, 0.72), ((0.10, 0.80, 0.82), 0.6, 1.0)]   # cor, início, fim (0=cintura, 1=barra)
NV = 7
PLEATS = 14
bm = bmesh.new()
sway, mats = [], []
for ti, (col, s0, s1) in enumerate(TIERS):
    rings = []
    for j in range(NV + 1):
        s = s0 + (s1 - s0) * j / NV
        z = ZW + (ZH - ZW) * s
        flare = s ** 1.3                                   # justa em cima, abre embaixo
        rx = rxw * 1.0 + (rxh * 1.28 - rxw * 1.0) * flare + 0.012 * ti * s
        ry = ryw * 1.0 + (ryh * 1.45 - ryw * 1.0) * flare + 0.012 * ti * s
        ring = []
        for i in range(NA):
            a = 2 * math.pi * i / NA
            u = (j / NV)                                   # 0 no começo da camada, 1 na barra dela
            pleat = 1 + 0.045 * u * math.sin(PLEATS * a + ti)         # pregas abrem para a barra
            zz = z - 0.008 * u * (1 + math.sin(PLEATS * a + ti))       # barra em ondas largas (babado), sem pontas
            rr = math.hypot(math.cos(a) * rx, math.sin(a) * ry)
            rr = max(rr, body_r(i, zz) + 0.008 + 0.022 * s + 0.006 * ti) * pleat   # nunca dentro do corpo; colada na cintura
            p = Vector((math.cos(a) * rr, cyw + math.sin(a) * rr, zz))
            ring.append(bm.verts.new(p)); sway.append(s)
        rings.append(ring)
    for r0, r1 in zip(rings, rings[1:]):
        for i in range(NA):
            f = bm.faces.new((r0[i], r0[(i + 1) % NA], r1[(i + 1) % NA], r1[i])); f.material_index = ti; f.smooth = True
    m = bpy.data.materials.new('saia_%d' % ti); m.use_nodes = True
    m.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value = (*(c ** 2.2 for c in col), 1)
    mats.append(m)
bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
moved = 0
me = bpy.data.meshes.new('saia'); bm.to_mesh(me); bm.free()
for m in mats:
    me.materials.append(m)
att = me.attributes.new('_SWAY_HAIR', 'FLOAT', 'POINT')
for i, x in enumerate(sway):
    att.data[i].value = x ** 1.5
ob = bpy.data.objects.new('prop_valeria_skirt', me); bpy.context.scene.collection.objects.link(ob)
print('Saia:', len(TIERS), 'camadas,', len(me.vertices), 'vértices |', moved, 'empurrados para fora do corpo')
bpy.ops.object.select_all(action='DESELECT'); ob.select_set(True)
bpy.ops.export_scene.gltf(filepath=out, export_format='GLB', use_selection=True, export_attributes=True)
print('Exportado:', out)
