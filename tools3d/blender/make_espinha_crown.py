"""Coroa de pétalas da Lady Espinha em código: anéis de pétalas (magenta, rosa e vinho, como no sprite) sobre o topo
plano da cabeça de cacto, curvadas para fora e passando da borda, com o cone dourado no meio. Exporta no espaço do corpo
(o visualizador prende na cabeça como a peruca). _SWAY_HAIR: 0 na base da pétala, 1 na ponta (as pontas balançam).

  blender -b --factory-startup -P tools3d/blender/make_espinha_crown.py -- corpo.glb saida.glb [--aneis 4] [--seed 3]
"""
import bpy, bmesh, os, sys, math, random
from mathutils import Vector

argv = sys.argv[sys.argv.index('--') + 1:]
body_p, out = os.path.abspath(argv[0]), os.path.abspath(argv[1])
opt = dict(zip(argv[2::2], argv[3::2]))
NR = int(opt.get('--aneis', 3)); random.seed(int(opt.get('--seed', 3)))

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=body_p)
arm = next(o for o in bpy.context.scene.objects if o.type == 'ARMATURE')
body = max((o for o in arm.children if o.type == 'MESH'), key=lambda o: len(o.data.vertices))
mw = body.matrix_world; g = body.vertex_groups['Head']
H = [mw @ v.co for v in body.data.vertices for ge in v.groups if ge.group == g.index and ge.weight > 0.6]
ztop = max(p.z for p in H)
top = [p for p in H if p.z > ztop - 0.03]
cx, cy = sum(p.x for p in top) / len(top), sum(p.y for p in top) / len(top)
hw = max(max(abs(p.x - cx) for p in H), max(abs(p.y - cy) for p in H))
print('Topo da cabeça z', round(ztop, 3), '| meia largura', round(hw, 3))

COLS = [(0.88, 0.08, 0.48), (1.0, 0.44, 0.71), (0.69, 0.06, 0.37)]
bm = bmesh.new(); sway, mats = [], []


def petal(base, d, elev, length, width, mi):
    """pétala larga em concha: sai da base apontando para fora/cima (elev) e a ponta curva um pouco para fora/baixo"""
    up = Vector((0, 0, 1)); side = Vector((-d.y, d.x, 0))
    NS, NW = 7, 5
    rows = []
    c = base.copy()
    for i in range(NS + 1):
        t = i / NS
        e = elev - 0.6 * t * t                          # a ponta abre (cai) um pouco
        ax = d * math.cos(e) + up * math.sin(e)
        if i:
            c = c + ax * (length / NS)                     # anda ao longo da pétala, curvando
        w = width * math.sin(math.pi * (0.08 + 0.92 * t)) ** 0.7 * (1 - 0.35 * t * t)
        nrm = ax.cross(side).normalized()               # normal da pétala (para a concha)
        rows.append([bm.verts.new(c + side * (w * (j / NW * 2 - 1)) - nrm * (0.35 * w * (j / NW * 2 - 1) ** 2)) for j in range(NW + 1)])
        sway.extend([t] * (NW + 1))
    for r0, r1 in zip(rows, rows[1:]):
        for j in range(NW):
            f = bm.faces.new((r0[j], r0[j + 1], r1[j + 1], r1[j])); f.material_index = mi; f.smooth = True


NR = int(opt.get('--aneis', 4))
for ring in range(NR):                                 # de fora (larga, abrindo) para dentro (em pé): um monte cheio
    k = ring / max(1, NR - 1)
    n = int(18 - 3 * ring)
    rr = hw * (0.85 - 0.6 * k)
    for i in range(n):
        a = 2 * math.pi * (i + 0.5 * ring) / n + random.uniform(-0.1, 0.1)
        d = Vector((math.cos(a), math.sin(a), 0))
        base = Vector((cx, cy, ztop + 0.005 + 0.03 * ring)) + d * rr
        petal(base, d, math.radians(52 + 30 * k) + random.uniform(-0.1, 0.1), hw * (1.05 - 0.2 * k) * random.uniform(0.9, 1.1),
              hw * (0.46 - 0.08 * k), (ring + i) % 3)
# cone dourado no meio (pirâmide de 4 lados, como no sprite)
gold = len(COLS)
c0 = Vector((cx, cy, ztop + 0.03 * NR + 0.05)); hgt = hw * 0.55; rb = hw * 0.22
bv = [bm.verts.new(c0 + Vector((math.cos(a) * rb, math.sin(a) * rb, 0))) for a in (math.pi / 4 + i * math.pi / 2 for i in range(4))]
tip = bm.verts.new(c0 + Vector((0, 0, hgt))); sway.extend([0.0] * 5)
for i in range(4):
    f = bm.faces.new((bv[i], bv[(i + 1) % 4], tip)); f.material_index = gold
f = bm.faces.new(list(reversed(bv))); f.material_index = gold
bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
me = bpy.data.meshes.new('coroa'); bm.to_mesh(me); bm.free()
for c in COLS + [(0.95, 0.72, 0.12)]:
    m = bpy.data.materials.new('coroa_%02x%02x%02x' % tuple(int(x * 255) for x in c)); m.use_nodes = True
    m.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value = (*(x ** 2.2 for x in c), 1)
    me.materials.append(m)
att = me.attributes.new('_SWAY_HAIR', 'FLOAT', 'POINT')
for i, x in enumerate(sway):
    att.data[i].value = x ** 1.5
ob = bpy.data.objects.new('prop_espinha_coroa', me); bpy.context.scene.collection.objects.link(ob)
print('Coroa:', len(me.polygons), 'faces')
bpy.ops.object.select_all(action='DESELECT'); ob.select_set(True)
bpy.ops.export_scene.gltf(filepath=out, export_format='GLB', use_selection=True, export_attributes=True)
print('Exportado:', out)
