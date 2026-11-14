"""Espinhos dourados e colar da Lady Espinha, colados no corpo (seguem o esqueleto): cones curtos sobre a pele verde
(cabeça, braços e pernas; nunca na frente do rosto), cada um com o peso do osso da face onde nasce; colar = anel dourado
no pescoço com pingente rosa na frente.

  blender -b --factory-startup -P tools3d/blender/add_espinha_spines.py -- corpo.glb saida.glb [--n 70] [--seed 5]
"""
import bpy, bmesh, os, sys, math, random
from mathutils import Vector, Matrix

argv = sys.argv[sys.argv.index('--') + 1:]
src, out = os.path.abspath(argv[0]), os.path.abspath(argv[1])
opt = dict(zip(argv[2::2], argv[3::2]))
NSP = int(opt.get('--n', 70)); random.seed(int(opt.get('--seed', 5)))

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=src)
arm = next(o for o in bpy.context.scene.objects if o.type == 'ARMATURE')
body = max((o for o in arm.children if o.type == 'MESH'), key=lambda o: len(o.data.vertices))
me, mw = body.data, body.matrix_world
gs = body.vertex_groups
img = next(n.image for m in me.materials if m and m.use_nodes for n in m.node_tree.nodes if n.type == 'TEX_IMAGE' and n.image)
W, H = img.size; px = img.pixels[:]; uvl = me.uv_layers.active.data
gn = {g.index: g.name for g in gs}
head_c = arm.matrix_world @ arm.data.bones['Head'].head_local


def green(p):
    u = sum(uvl[l].uv.x for l in p.loop_indices) / p.loop_total; v = sum(uvl[l].uv.y for l in p.loop_indices) / p.loop_total
    k = (min(H - 1, max(0, int(v * H))) * W + min(W - 1, max(0, int(u * W)))) * 4
    r, g, b = px[k:k + 3]
    return g > r * 1.25 and g > b * 1.25


cands, areas = [], []
for p in me.polygons:
    q = mw @ p.center; nrm = (mw.to_3x3() @ p.normal).normalized()
    names = {gn[ge.group] for i in p.vertices for ge in me.vertices[i].groups if ge.weight > 0.4}
    if not names or not green(p):
        continue
    if 'Head' in names and nrm.y < -0.55:          # frente do rosto: sem espinho
        continue
    if not any(k in nm for nm in names for k in ('Head', 'Arm', 'Leg')):
        continue
    cands.append((p, q, nrm)); areas.append(p.area)
picks = []
tot = sum(areas)
while len(picks) < NSP and cands:
    r = random.uniform(0, tot); acc = 0
    for (c, a) in zip(cands, areas):
        acc += a
        if acc >= r:
            if all((c[1] - o[1]).length > 0.07 for o in picks):
                picks.append(c)
            break
    tot = tot
print('Espinhos:', len(picks), 'de', len(cands), 'faces verdes possíveis')

bm = bmesh.new(); wts = []
SEG = 6
for p, q, nrm in picks:
    L = random.uniform(0.05, 0.07); R = 0.016
    t1 = nrm.orthogonal().normalized(); t2 = nrm.cross(t1)
    base = [bm.verts.new(q - nrm * 0.004 + (t1 * math.cos(2 * math.pi * i / SEG) + t2 * math.sin(2 * math.pi * i / SEG)) * R) for i in range(SEG)]
    tip = bm.verts.new(q + nrm * L)
    for i in range(SEG):
        bm.faces.new((base[i], base[(i + 1) % SEG], tip)).material_index = 0
    w = {}
    for i in p.vertices:
        for ge in me.vertices[i].groups:
            w[gn[ge.group]] = w.get(gn[ge.group], 0) + ge.weight / p.loop_total
    wts.extend([w] * (SEG + 1))
# colar: anel no pescoço + pingente
neck = arm.matrix_world @ arm.data.bones['neck'].head_local
band = [mw @ v.co for v in me.vertices if abs((mw @ v.co).z - (neck.z + 0.03)) < 0.012 and abs((mw @ v.co).x - neck.x) < 0.1]
rx = max(abs(q.x - neck.x) for q in band) + 0.01; cyn = (max(q.y for q in band) + min(q.y for q in band)) / 2
ry = (max(q.y for q in band) - min(q.y for q in band)) / 2 + 0.01
NT, NS2, rt = 28, 6, 0.011
ring = []
for i in range(NT):
    a = 2 * math.pi * i / NT
    c = Vector((neck.x + rx * math.cos(a), cyn + ry * math.sin(a), neck.z + 0.03 - 0.02 * max(0, -math.sin(a))))
    dr = Vector((math.cos(a), math.sin(a), 0))
    ring.append([bm.verts.new(c + (dr * math.cos(2 * math.pi * j / NS2) + Vector((0, 0, 1)) * math.sin(2 * math.pi * j / NS2)) * rt) for j in range(NS2)])
    wts.extend([{'neck': 0.5, 'Spine': 0.5}] * NS2)
for i in range(NT):
    for j in range(NS2):
        f = bm.faces.new((ring[i][j], ring[(i + 1) % NT][j], ring[(i + 1) % NT][(j + 1) % NS2], ring[i][(j + 1) % NS2])); f.material_index = 0; f.smooth = True
pc = Vector((neck.x, cyn - ry - 0.012, neck.z + 0.0))
r0 = bmesh.ops.create_icosphere(bm, subdivisions=2, radius=0.02, matrix=Matrix.Translation(pc))
for v in r0['verts']:
    wts.append({'neck': 0.5, 'Spine': 0.5})
for f in {f for v in r0['verts'] for f in v.link_faces}:
    f.material_index = 1; f.smooth = True
bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
sm = bpy.data.meshes.new('espinhos'); bm.to_mesh(sm); bm.free()
for nm, c in (('espinho_ouro', (0.95, 0.72, 0.12)), ('pingente', (0.95, 0.25, 0.55))):
    m = bpy.data.materials.new(nm); m.use_nodes = True
    m.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value = (*(x ** 2.2 for x in c), 1)
    sm.materials.append(m)
so = bpy.data.objects.new('espinhos', sm); bpy.context.scene.collection.objects.link(so)
for vi, w in enumerate(wts):
    for gname, x in w.items():
        if x > 0:
            (so.vertex_groups.get(gname) or so.vertex_groups.new(name=gname)).add([vi], x, 'REPLACE')
so.parent = arm; so.modifiers.new('Armature', 'ARMATURE').object = arm
print('Colar: raio', round(rx, 3), round(ry, 3), 'em z', round(neck.z, 3))
bpy.ops.export_scene.gltf(filepath=out, export_format='GLB', export_extras=True, export_attributes=True)
print('Exportado:', out)
