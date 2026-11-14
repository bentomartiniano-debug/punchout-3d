"""Cabelão de magma da Valéria em código: metaballs (cúpula grande, massa de trás, laterais que emolduram o rosto e
gomos arredondados embaixo, como no sprite) com uma bola negativa abrindo o rosto. Vira malha, sai de dentro da cabeça
e ganha _SWAY_HAIR (gomos de baixo balançam). A cor de lava (degradê + brilho) é do shader do visualizador.

  blender -b --factory-startup -P tools3d/blender/make_valeria_hair.py -- corpo.glb saida.glb [--rosto 1.0] [--franja 0.6] [--teste render]
  Valéria: --rosto 1.0 --franja 0.6
"""
import bpy, bmesh, os, sys, math
from mathutils import Vector
from mathutils.bvhtree import BVHTree

argv = sys.argv[sys.argv.index('--') + 1:]
body_p, out = os.path.abspath(argv[0]), os.path.abspath(argv[1])
opt = dict(zip(argv[2::2], argv[3::2]))

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=body_p)
arm = next(o for o in bpy.context.scene.objects if o.type == 'ARMATURE')
body = max((o for o in arm.children if o.type == 'MESH'), key=lambda o: len(o.data.vertices))
mw = body.matrix_world
g = body.vertex_groups['Head']
H = [mw @ v.co for v in body.data.vertices for ge in v.groups if ge.group == g.index and ge.weight > 0.6]
x0, x1 = min(p.x for p in H), max(p.x for p in H)
y0, y1 = min(p.y for p in H), max(p.y for p in H)
ztop = max(p.z for p in H)
hw = (x1 - x0) / 2                       # meia largura da cabeça
cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
zc = ztop - hw * 1.15                     # centro da caixa craniana
print('Cabeça: meia largura', round(hw, 3), 'topo', round(ztop, 3), 'frente', round(y0, 3))

# ---- metaballs (medidas em múltiplos da meia largura da cabeça: serve para qualquer cabeça)
mb = bpy.data.metaballs.new('lava')
mb.resolution = 0.012; mb.render_resolution = 0.012; mb.threshold = 0.15   # limiar baixo: a bola aparece com ~0,78 do raio
V = 0.78
def ball(p, vis, neg=False, size=None):
    """vis = raio visível (ou semi-eixos visíveis, se size)"""
    if size:
        R = 2 * k; e = mb.elements.new(type='ELLIPSOID')
        e.size_x, e.size_y, e.size_z = (sv / (V * R) for sv in size)
    else:
        R = vis / V; e = mb.elements.new(type='BALL')
    e.co = p; e.radius = R; e.use_negative = neg
    return e
k = hw
ball(Vector((cx, cy + 0.6 * k, zc + 0.45 * k)), 0, size=(1.9 * k, 1.5 * k, 1.2 * k))          # cúpula
ball(Vector((cx, cy + 1.0 * k, zc - 0.5 * k)), 0, size=(1.7 * k, 1.1 * k, 1.2 * k))           # massa de trás
for s_ in (-1, 1):
    ball(Vector((cx + s_ * 1.7 * k, cy + 0.4 * k, zc - 0.6 * k)), 0.85 * k)                    # laterais (emolduram o rosto)
NL = 9                                                                                     # gomos de baixo
for i in range(NL):
    a_ = math.radians(-20 + 220 * i / (NL - 1))
    d = Vector((math.cos(a_), math.sin(a_), 0))
    ball(Vector((cx, cy + 0.3 * k, zc - 1.25 * k - 0.1 * k * math.sin(i * 2.1))) + d * 1.45 * k, 0.55 * k)
# moldura do rosto (capuz do sprite): franja sobre a testa e massas na frente das laterais do rosto
FB = float(opt.get('--franja', 0.3))     # base da franja acima do centro do crânio (x meia largura)
for s_ in (-1, 1):
    ball(Vector((cx + s_ * 1.18 * k, y0 + 0.4 * k, zc - 0.1 * k)), 0.45 * k)
    ball(Vector((cx + s_ * 1.1 * k, y0 + 0.5 * k, zc - 0.75 * k)), 0.4 * k)
HL = float(opt.get('--linha', 0.55))   # linha do cabelo: quanto acima do centro do crânio (x meia largura)
ball(Vector((cx, y0 - 0.9 * k, zc - 0.7 * k)), 0, neg=True, size=(float(opt.get('--rosto', 1.25)) * k, 1.5 * k, (HL + 0.7 + 0.6) * k))   # rosto aberto (arredondado, fechado)
ball(Vector((cx, y0 - 1.2 * k, zc - 1.6 * k)), 0, neg=True, size=(1.2 * k, 1.4 * k, 1.0 * k))                   # queixo e pescoço livres
mo = bpy.data.objects.new('lava_mb', mb); bpy.context.scene.collection.objects.link(mo)
bpy.context.view_layer.update()
dg = bpy.context.evaluated_depsgraph_get()
me = bpy.data.meshes.new_from_object(mo.evaluated_get(dg))
bpy.data.objects.remove(mo)
bmh = bmesh.new(); bmh.from_object(body, dg); bmh.transform(mw); htree = BVHTree.FromBMesh(bmh)
fr = bpy.data.metaballs.new('franja'); fr.resolution = 0.008; fr.render_resolution = 0.008; fr.threshold = 0.15
PATH = ((0.2, 0.9, 0.22), (0.38, 0.74, 0.29), (0.58, 0.56, 0.3), (0.72, 0.32, 0.27), (0.8, 0.08, 0.22), (0.84, -0.14, 0.17), (0.86, -0.3, 0.11), (0.86, -0.42, 0.06))
LOCK = []                                  # caminho interpolado (mecha lisa, termina em ponta)
for (a0, b0, r0), (a1, b1, r1) in zip(PATH, PATH[1:]):
    for j in range(4):
        u = j / 4; LOCK.append((a0 + (a1 - a0) * u, b0 + (b1 - b0) * u, r0 + (r1 - r0) * u))
LOCK.append(PATH[-1])
for s_ in (-1, 1):                       # repartido no meio: cada mecha sai do centro da testa e desce por fora do olho até a bochecha
    for (ax, az, r) in LOCK:
        x, z = cx + s_ * ax * k, zc + az * k
        hit, nrm, _, _ = htree.ray_cast(Vector((x, y0 - 1.0, z)), Vector((0, 1, 0)))
        if hit is None:
            continue
        e = fr.elements.new(type='BALL'); e.co = hit + Vector((0, -1, 0)) * (0.35 * r * k); e.radius = r * k / V
fo = bpy.data.objects.new('franja_mb', fr); bpy.context.scene.collection.objects.link(fo)
bpy.context.view_layer.update(); dg = bpy.context.evaluated_depsgraph_get()
mf = bpy.data.meshes.new_from_object(fo.evaluated_get(dg)); bpy.data.objects.remove(fo)
N0 = len(me.vertices)                     # vértices daqui em diante são da franja (_LOCK = 1: cor própria no shader)
bmj = bmesh.new(); bmj.from_mesh(me); bmj.from_mesh(mf); bmj.to_mesh(me); bmj.free(); bpy.data.meshes.remove(mf)
print('Franja repartida:', len(LOCK) * 2, 'bolas')
# rabo de cavalo: prendedor escuro no alto de trás da cúpula e um tufo fofo (nuvem de gomos) subindo para trás
N1 = len(me.vertices)                     # daqui: tufo (_LOCK = 2)
AX = Vector((0, 0.7, 0.72)).normalized()  # direção do rabo: para cima e para trás
# base: o ponto da cúpula nessa direção (raio a partir do centro da cúpula), assim o prendedor encosta nela
TS = float(opt.get('--rabo', 1.35))        # tamanho do tufo
base = Vector((cx, cy + 0.6 * k, zc + 0.45 * k)) + Vector((0, 0.42, 0.91)).normalized() * 1.25 * k
tie = base + AX * 0.02 * k                 # prendedor um pouco acima da superfície da cúpula
rb = bpy.data.metaballs.new('rabo'); rb.resolution = 0.008; rb.render_resolution = 0.008; rb.threshold = 0.15
c0 = tie + AX * 0.5 * k * TS
side = Vector((1, 0, 0)); back = AX.cross(side).normalized()
for off, r in ((Vector(), 0.42), (side * 0.3 * k + AX * 0.1 * k, 0.32), (-side * 0.28 * k + AX * 0.12 * k, 0.33),
               (AX * 0.42 * k, 0.36), (AX * 0.62 * k + side * 0.18 * k, 0.27), (AX * 0.6 * k - side * 0.2 * k, 0.26),
               (back * 0.28 * k + AX * 0.2 * k, 0.3), (-back * 0.22 * k + AX * 0.3 * k, 0.28)):
    e = rb.elements.new(type='BALL'); e.co = c0 + off * TS; e.radius = r * k * TS / V
ro = bpy.data.objects.new('rabo_mb', rb); bpy.context.scene.collection.objects.link(ro)
bpy.context.view_layer.update(); dg = bpy.context.evaluated_depsgraph_get()
mr = bpy.data.meshes.new_from_object(ro.evaluated_get(dg)); bpy.data.objects.remove(ro)
bmj = bmesh.new(); bmj.from_mesh(me); bmj.from_mesh(mr)
N2 = len(bmj.verts)                       # daqui: prendedor (_LOCK = 3)
# prendedor: anel (toro) em volta da base do tufo
NT, NS, RT, RS = 22, 8, 0.24 * k, 0.095 * k
u1 = side; u2 = back
ring = []
for i in range(NT):
    a = 2 * math.pi * i / NT
    cc = tie + (u1 * math.cos(a) + u2 * math.sin(a)) * RT
    rad = (u1 * math.cos(a) + u2 * math.sin(a))
    ring.append([bmj.verts.new(cc + (rad * math.cos(2 * math.pi * j / NS) + AX * math.sin(2 * math.pi * j / NS)) * RS) for j in range(NS)])
for i in range(NT):
    r0, r1 = ring[i], ring[(i + 1) % NT]
    for j in range(NS):
        bmj.faces.new((r0[j], r1[j], r1[(j + 1) % NS], r0[(j + 1) % NS]))
bmj.to_mesh(me); bmj.free(); bpy.data.meshes.remove(mr)
print('Rabo de cavalo: tufo', N2 - N1, 'vértices + prendedor')
ob = bpy.data.objects.new('prop_valeria_hair', me); bpy.context.scene.collection.objects.link(ob)

# ---- sai de dentro da cabeça e do pescoço; apaga o que ainda cobrir o rosto
bm = bmesh.new(); bm.from_object(body, dg); bm.transform(mw); tree = BVHTree.FromBMesh(bm)
moved = 0   # não empurra: onde a cúpula entra na testa fica a linha do cabelo (a cabeça esconde o resto)
bm2 = bmesh.new(); bm2.from_mesh(me)
face_box = lambda c: abs(c.x - cx) < 0.62 * k and c.y < cy - 0.2 * k and ztop - 2.6 * k < c.z < ztop - 0.55 * k
dead = []   # o rosto é aberto pelas bolas negativas (borda fechada, sem janela)
bmesh.ops.delete(bm2, geom=dead, context='FACES')
for f in bm2.faces:
    f.smooth = True
bm2.to_mesh(me); bm2.free()
# balanço: gomos de baixo e as costas mexem, a cúpula fica presa
att = me.attributes.new('_SWAY_HAIR', 'FLOAT', 'POINT')
for i, v in enumerate(me.vertices):
    t = max(0.0, min(1.0, (zc - 0.2 * k - v.co.z) / (1.2 * k)))
    tail = 0.8 if N1 <= i < N2 else 0.0   # tufo do rabo balança; o prendedor fica
    att.data[i].value = max((t * t * (3 - 2 * t)) * (0.6 + 0.4 * max(0.0, (v.co.y - cy) / (1.5 * k))), tail)
lk = me.attributes.new('_LOCK', 'FLOAT', 'POINT')
for i in range(N0, len(me.vertices)):
    if i < N1:
        lk.data[i].value = 1.0                                         # franja
    elif i < N2:                                                       # tufo: 2 + altura ao longo do rabo (0..0.9) -> degradê próprio
        h = (me.vertices[i].co - tie).dot(AX) / (1.25 * k * TS)
        lk.data[i].value = 2.0 + max(0.0, min(0.9, h * 0.9))
    else:
        lk.data[i].value = 3.5                                         # prendedor
mat = bpy.data.materials.new('lava'); mat.use_nodes = True
mat.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value = (1.0, 0.35, 0.05, 1)
me.materials.append(mat)
print('Cabelo de magma:', len(me.vertices), 'vértices |', moved, 'empurrados para fora da cabeça |', len(dead), 'faces do rosto apagadas')

if '--teste' in opt:
    sc = bpy.context.scene
    sc.render.engine = 'BLENDER_WORKBENCH'; sc.display.shading.light = 'STUDIO'; sc.display.shading.color_type = 'OBJECT'
    ob.color = (1, 0.4, 0.05, 1); body.color = (0.85, 0.6, 0.45, 1)
    sc.render.resolution_x = sc.render.resolution_y = 360
    cam = bpy.data.objects.new('cam', bpy.data.cameras.new('cam')); sc.collection.objects.link(cam); sc.camera = cam
    cam.data.type = 'ORTHO'; cam.data.ortho_scale = 0.95
    tgt = Vector((cx, cy, zc - 0.3 * k))
    for name, d in (('frente', (0, -1, 0)), ('lado', (1, 0, 0)), ('costas', (0, 1, 0)), ('diag', (0.7, -0.7, 0.2))):
        cam.location = tgt + Vector(d) * 2
        cam.rotation_euler = (tgt - cam.location).to_track_quat('-Z', 'Y').to_euler()
        sc.render.filepath = opt['--teste'] + '_' + name + '.png'; bpy.ops.render.render(write_still=True)
    bpy.data.objects.remove(cam)
bpy.ops.object.select_all(action='DESELECT'); ob.select_set(True)
bpy.ops.export_scene.gltf(filepath=out, export_format='GLB', use_selection=True, export_attributes=True)
print('Exportado:', out)
