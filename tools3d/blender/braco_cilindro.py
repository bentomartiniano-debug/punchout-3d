"""Troca a manga do Meshy (fundida no casaco no A-pose, solta rebarbas de pano embaixo do braço na guarda) por um
cilindro sólido do ombro ao pulso, preso nos ossos do braço e pintado com a cor média da manga.

Só some a manga colada no braço (--perto do eixo); o pano do casaco que tinha peso de braço fica e passa a seguir o
tronco (sem buraco e sem esticar).

  blender -b --factory-startup -P tools3d/blender/braco_cilindro.py -- corpo.glb saida.glb [--min 0.2] [--perto 0.14] [--lado 0.8] [--raio R]
  Joana (depois de engrossar_tronco.py --costas 1.3 --largura 1.25): --perto 0.14 --lado 0.8 --casca 1.0 --frente 50  (laterais sólidas só dos lados e costas; a frente fica livre)
"""
import bpy, bmesh, os, sys, math
from mathutils import Vector
from mathutils.geometry import intersect_point_line

argv = sys.argv[sys.argv.index('--') + 1:]
src, out = os.path.abspath(argv[0]), os.path.abspath(argv[1])
opt = dict(zip(argv[2::2], argv[3::2]))
MINW = float(opt.get('--min', 0.2))
NEAR = float(opt.get('--perto', 0.14))

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=src)
arm = next(o for o in bpy.context.scene.objects if o.type == 'ARMATURE')
body = max((o for o in arm.children if o.type == 'MESH'), key=lambda o: len(o.data.vertices))
me, mw = body.data, body.matrix_world
inv = mw.inverted()
gs = body.vertex_groups
img = next(n.image for m in me.materials if m and m.use_nodes for n in m.node_tree.nodes if n.type == 'TEX_IMAGE' and n.image)
W, H = img.size
px = img.pixels[:]
uvl = me.uv_layers.active.data

dead = set()
new_objs = []
gidx = {g.name: g.index for g in gs}
for side in ('Left', 'Right'):
    chain = {gidx[side + b] for b in ('Arm', 'ForeArm', 'Hand') if side + b in gidx}
    B = arm.data.bones
    sh = arm.matrix_world @ B[side + 'Arm'].head_local
    el = arm.matrix_world @ B[side + 'ForeArm'].head_local
    wr = arm.matrix_world @ B[side + 'Hand'].head_local
    aw = [sum(ge.weight for ge in v.groups if ge.group in chain) for v in me.vertices]
    def dist(q):
        best = 9.0
        for a0, a1 in ((sh, el), (el, wr)):
            pt, t = intersect_point_line(q, a0, a1)
            best = min(best, (q - a0.lerp(a1, max(0.0, min(1.0, t)))).length)
        return best
    vd = {}
    def vdist(i):
        if i not in vd:
            vd[i] = dist(mw @ me.vertices[i].co)
        return vd[i]
    faces = [p for p in me.polygons if sum(aw[i] for i in p.vertices) / p.loop_total >= MINW and all(vdist(i) < NEAR for i in p.vertices)]
    ds = sorted(dist(mw @ me.vertices[i].co) for p in faces for i in p.vertices)
    R = float(opt.get('--raio', ds[len(ds) // 2] if ds else 0.06))
    from collections import Counter
    cnt = Counter()
    for p in faces:
        u = sum(uvl[l].uv.x for l in p.loop_indices) / p.loop_total; v_ = sum(uvl[l].uv.y for l in p.loop_indices) / p.loop_total
        k = (min(H - 1, max(0, int(v_ * H))) * W + min(W - 1, max(0, int(u * W)))) * 4
        cnt[tuple(int(c * 255) // 12 * 12 + 6 for c in px[k:k + 3])] += 1
    col = cnt.most_common(1)[0][0] if cnt else (30, 60, 60)
    dead |= {p.index for p in faces}
    # cilindro: começa um pouco dentro do ombro (tampa o buraco) e termina no pulso (a luva cobre a ponta)
    start = sh + (sh - el).normalized() * R * 0.6
    NS, NR = 14, 16
    path = [(start.lerp(el, i / NS), 0.5 * i / NS) for i in range(NS + 1)] + [(el.lerp(wr, i / NS), 0.5 + 0.5 * i / NS) for i in range(1, NS + 1)]
    bm = bmesh.new()
    rings, wts = [], []
    for k, (p, s) in enumerate(path):
        a = (path[min(k + 1, len(path) - 1)][0] - path[max(k - 1, 0)][0]).normalized()
        x = a.cross(Vector((0, 1, 0))).normalized(); y = a.cross(x)
        r = R * (1.0 + 0.15 * max(0.0, (s - 0.85) / 0.15))     # punho abre um pouco
        fo = min(1.0, max(0.0, (s - 0.44) / 0.12))             # cotovelo: braço -> antebraço
        w = {side + 'Shoulder': 0.3, side + 'Arm': 0.7} if s < 0.06 else {side + 'Arm': 1 - fo, side + 'ForeArm': fo}
        ring = []
        for j in range(16):
            ang = 2 * math.pi * j / 16
            ring.append(bm.verts.new(p + (x * math.cos(ang) + y * math.sin(ang)) * r)); wts.append(w)
        rings.append(ring)
    for r0, r1 in zip(rings, rings[1:]):
        for j in range(16):
            bm.faces.new((r0[j], r0[(j + 1) % 16], r1[(j + 1) % 16], r1[j])).smooth = True
    bm.faces.new(rings[0]); bm.faces.new(rings[-1])
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    cm = bpy.data.meshes.new('manga_' + side.lower()); bm.to_mesh(cm); bm.free()
    ob = bpy.data.objects.new('manga_' + side.lower(), cm); bpy.context.scene.collection.objects.link(ob)
    for vi, w in enumerate(wts):
        for gname, x in w.items():
            if x > 0:
                (ob.vertex_groups.get(gname) or ob.vertex_groups.new(name=gname)).add([vi], x, 'REPLACE')
    m = bpy.data.materials.new('manga_' + side.lower()); m.use_nodes = True
    m.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value = (*((c / 255) ** 2.2 for c in col), 1)
    cm.materials.append(m)
    ob.parent = arm
    ob.modifiers.new('Armature', 'ARMATURE').object = arm
    new_objs.append(ob)
    print('Braço cilindro:', side, len(faces), 'faces da manga trocadas | raio', round(R, 3), '| cor', col)
# miolo: um tronco sólido da cor do casaco por dentro, um pouco menor que ele — fresta que sobrar mostra pano, não o vazio
ARMG = {gidx[s_ + b] for s_ in ('Left', 'Right') for b in ('Arm', 'ForeArm', 'Hand') if s_ + b in gidx}
B = arm.data.bones
z0 = (arm.matrix_world @ B['Hips'].head_local).z
z1 = (arm.matrix_world @ B['LeftArm'].head_local).z + float(opt.get('--topo', 0.0))
tv = []
for v in me.vertices:
    if sum(ge.weight for ge in v.groups if ge.group in ARMG) < 0.3:
        tv.append((mw @ v.co, v))
# só nas laterais e costas (a frente fica livre: barriga, camisa e calça aparecem como são), da barra ao ombro
zh = (arm.matrix_world @ B['LeftLeg'].head_local).z + float(opt.get('--barra', 0.08))
CASCA = float(opt.get('--casca', 0.9))   # 0.9 = por dentro do casaco; ~1.0 = na superfície (esconde a lateral comida)
A0, A1 = math.radians(-float(opt.get('--frente', 35))), math.radians(85)   # ângulo: -90 = frente (-Y), +90 = costas
secs = []
bm2 = bmesh.new(); src2 = []
NZ2, NA2 = 14, 12
for sg in (1, -1):
    grid = []
    for k in range(NZ2 + 1):
        h = zh + (z1 - zh) * k / NZ2
        band = [(q, v) for q, v in tv if abs(q.z - h) < 0.03]
        side_b = [(q, v) for q, v in band if q.x * sg > 0]
        if len(side_b) < 6:
            continue
        cy = (max(q.y for q, _ in band) + min(q.y for q, _ in band)) / 2
        rx = max(abs(q.x) for q, _ in side_b) * CASCA
        ry = (max(q.y for q, _ in band) - min(q.y for q, _ in band)) / 2 * CASCA
        if sg == 1:
            secs.append((h, 0.0, rx))
        row = []
        for j in range(NA2):
            ang = A0 + (A1 - A0) * j / (NA2 - 1)
            p = Vector((sg * rx * math.cos(ang), cy + ry * math.sin(ang), h))
            row.append(bm2.verts.new(p)); src2.append(min(side_b, key=lambda qv: (qv[0] - p).length)[1])
        grid.append((row, cy))
    for (r0, cy), (r1, _) in zip(grid, grid[1:]):
        for j in range(NA2 - 1):
            f = bm2.faces.new((r0[j], r1[j], r1[j + 1], r0[j + 1])); f.smooth = True
            c = f.calc_center_median(); f.normal_update()
            if f.normal.dot(Vector((c.x, c.y - cy, 0))) < 0:
                f.normal_flip()
cm = bpy.data.meshes.new('miolo_tronco'); bm2.to_mesh(cm); bm2.free()
ob = bpy.data.objects.new('miolo_tronco', cm); bpy.context.scene.collection.objects.link(ob)
for vi, sv in enumerate(src2):
    ws_ = [(gs[ge.group].name, ge.weight) for ge in sv.groups if ge.group not in ARMG and ge.weight > 0]
    tot_ = sum(w for _, w in ws_) or 1
    for gn_, w in ws_:
        (ob.vertex_groups.get(gn_) or ob.vertex_groups.new(name=gn_)).add([vi], w / tot_, 'REPLACE')
m = bpy.data.materials.new('miolo_tronco'); m.use_nodes = True
m.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value = (*((c / 255) ** 2.2 for c in col), 1)
cm.materials.append(m)
ob.parent = arm; ob.modifiers.new('Armature', 'ARMATURE').object = arm
print('Laterais sólidas: de', round(zh, 2), 'a', round(z1, 2))
# cacos de manga que sobraram na lateral do casaco (tudo com peso de braço, abaixo do ombro, nos lados): o miolo cobre
SIDE = float(opt.get('--lado', 0))
if SIDE > 0:
    armw0 = [sum(ge.weight for ge in v.groups if ge.group in ARMG) for v in me.vertices]
    def lateral(q):
        h, cx, rx = min(secs, key=lambda t: abs(t[0] - q.z))
        return q.z < z1 and abs(q.x - cx) > SIDE * rx
    extra = {p.index for p in me.polygons if p.index not in dead and all(armw0[i] >= 0.1 and lateral(mw @ me.vertices[i].co) for i in p.vertices)}
    dead |= extra
    print('Cacos laterais apagados:', len(extra), 'faces')
# o pano que ficou (casaco) larga o braço: peso do braço vai para o resto, ou para o ombro se não houver resto
keep_v = {i for p in me.polygons if p.index not in dead for i in p.vertices}
armg = {gidx[s_ + b] for s_ in ('Left', 'Right') for b in ('Arm', 'ForeArm', 'Hand') if s_ + b in gidx}
freed = 0
for i in keep_v:
    v = me.vertices[i]
    a = [ge for ge in v.groups if ge.group in armg and ge.weight > 0]
    if not a:
        continue
    rest = [(ge.group, ge.weight) for ge in v.groups if ge.group not in armg and ge.weight > 0]
    tot = sum(w for _, w in rest)
    side_ = 'Left' if 'Left' in gs[a[0].group].name else 'Right'
    for ge in a:
        gs[ge.group].remove([i])
    if tot > 0.05:
        for g, w in rest:
            gs[g].add([i], w / tot, 'REPLACE')
    else:
        gs[side_ + 'Shoulder'].add([i], 1.0, 'REPLACE')
    freed += 1
print('Pano do casaco solto do braço:', freed, 'vértices')
# apaga a manga velha pelo modo de edição (preserva normais e UVs do resto do corpo)
bpy.context.view_layer.objects.active = body
for o in bpy.context.scene.objects: o.select_set(o is body)
bpy.ops.object.mode_set(mode='EDIT'); bpy.ops.mesh.select_mode(type='FACE'); bpy.ops.mesh.select_all(action='DESELECT')
ebm = bmesh.from_edit_mesh(me); ebm.faces.ensure_lookup_table()
for i in dead:
    ebm.faces[i].select_set(True)
bmesh.update_edit_mesh(me)
brd = ebm.verts.layers.int.new('borda')   # marca a borda (os índices mudam depois do delete)
for i in dead:
    for v in ebm.faces[i].verts:
        if any(not f.select for f in v.link_faces):
            v[brd] = 1
bmesh.update_edit_mesh(me)
bpy.ops.mesh.delete(type='FACE')
# tampa os buracos que a manga deixou no casaco (axila, lateral): cada canto da tampa copia o UV do vizinho
ebm = bmesh.from_edit_mesh(me)
brd = ebm.verts.layers.int['borda']
old = ebm.faces.layers.int.new('velha')      # faces que já existiam
for f in ebm.faces:
    f[old] = 1
bpy.ops.mesh.select_all(action='DESELECT')
for v in ebm.verts:
    if v[brd]:
        v.select_set(True)
ebm.select_flush(True)
bmesh.update_edit_mesh(me)
bpy.ops.mesh.fill_holes(sides=0)
ebm = bmesh.from_edit_mesh(me)
uvL = ebm.loops.layers.uv.active
old = ebm.faces.layers.int['velha']
caps = [f for f in ebm.faces if not f[old]]
ebm.faces.layers.int.remove(old)
ebm.verts.layers.int.remove(ebm.verts.layers.int['borda'])
for f in caps:
    for l in f.loops:
        o = next((ol for ol in l.vert.link_loops if ol.face not in caps), None)
        if o:
            l[uvL].uv = o[uvL].uv.copy()
bmesh.update_edit_mesh(me)
print('Buracos tampados:', len(caps), 'faces')
bpy.ops.object.mode_set(mode='OBJECT')
bpy.ops.export_scene.gltf(filepath=out, export_format='GLB', export_extras=True, export_attributes=True)
print('Exportado:', out)
