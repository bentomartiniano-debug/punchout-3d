"""Veste a peruca na cabeça do corpo: escala pelo tamanho da cabeça, assenta no topo e empurra para fora tudo
que entrar no corpo (os dreads caem por cima dos ombros/costas). Exporta a peruca já no espaço do corpo em repouso.

  blender -b --factory-startup -P tools3d/blender/vestir_peruca.py -- corpo.glb peruca.glb saida.glb
          [--largura 1.35] [--topo 0.04] [--recuo 0.02] [--folga 0.025] [--corte dz] [--espelhar]
  --espelhar troca os lados da peruca (o lado mais volumoso vai para o outro ombro)
"""
import bpy, bmesh, os, sys
from mathutils import Vector, Matrix
from mathutils.bvhtree import BVHTree

argv = sys.argv[sys.argv.index('--') + 1:]
body_p, wig_p, out = (os.path.abspath(a) for a in argv[:3])
FLIP = '--espelhar' in argv
argv = [a for a in argv if a != '--espelhar']
opt = dict(zip(argv[3::2], argv[4::2]))
WID, TOP, BACK, GAP = (float(opt.get(k, d)) for k, d in (('--largura', 1.35), ('--topo', 0.04), ('--recuo', 0.02), ('--folga', 0.025)))

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=body_p)
arm = next(o for o in bpy.context.scene.objects if o.type == 'ARMATURE')
body = max((o for o in arm.children if o.type == 'MESH'), key=lambda o: len(o.data.vertices))
dg = bpy.context.evaluated_depsgraph_get()
bm = bmesh.new(); bm.from_object(body, dg); bm.transform(body.matrix_world)
tree = BVHTree.FromBMesh(bm)
g = body.vertex_groups['Head']
head = [body.matrix_world @ v.co for v in body.data.vertices for ge in v.groups if ge.group == g.index and ge.weight > 0.6]
hx0, hx1 = min(p.x for p in head), max(p.x for p in head)
hy0, hy1 = min(p.y for p in head), max(p.y for p in head)
htop = max(p.z for p in head)
print('Cabeça: largura', round(hx1 - hx0, 3), 'topo', round(htop, 3))
names_before = {o.name for o in bpy.context.scene.objects}

bpy.ops.import_scene.gltf(filepath=wig_p)
wigs = [o for o in bpy.context.scene.objects if o.name not in names_before and o.type == 'MESH']
wig = wigs[0]
bpy.context.view_layer.objects.active = wig
for o in bpy.context.scene.objects:
    o.select_set(o in wigs)
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
P = [v.co for v in wig.data.vertices]
wx0, wx1 = min(p.x for p in P), max(p.x for p in P)
wy0, wy1 = min(p.y for p in P), max(p.y for p in P)
wz1 = max(p.z for p in P)
s = (hx1 - hx0) * WID / (wx1 - wx0)
c_w = Vector(((wx0 + wx1) / 2, (wy0 + wy1) / 2, wz1))
target = Vector(((hx0 + hx1) / 2, (hy0 + hy1) / 2 + BACK, htop + TOP))
wig.data.transform(Matrix.Translation(target) @ Matrix.Scale(s, 4) @ Matrix.Translation(-c_w))
if FLIP:   # depois de assentar: espelha em torno do eixo da cabeça (o vão do rosto continua no meio) e desvira as faces
    ax = (arm.matrix_world @ arm.data.bones['Head'].head_local).x
    wig.data.transform(Matrix.Translation((ax, 0, 0)) @ Matrix.Scale(-1, 4, Vector((1, 0, 0))) @ Matrix.Translation((-ax, 0, 0)))
    wig.data.flip_normals()
    print('Peruca espelhada no eixo da cabeça x =', round(ax, 3))
# empurra para fora do corpo o que entrou nele (abaixo do topo da cabeça)
if '--corte' in opt:   # encurta sem cortar reto: comprime a parte de baixo até a altura pedida (as pontas continuam pontas)
    sh = arm.matrix_world @ arm.data.bones['LeftArm'].head_local
    z_end = sh.z + float(opt['--corte']); z_start = z_end + 0.2
    zmin = min(v.co.z for v in wig.data.vertices)
    k = (z_start - z_end) / max(1e-6, z_start - zmin)
    for v in wig.data.vertices:
        if v.co.z < z_start:
            v.co.z = z_start - (z_start - v.co.z) * k
    print('Peruca encurtada: pontas em z', round(z_end, 3))
moved = 0
# só o que está DENTRO do corpo, e abaixo do topo da cabeça, sai para fora (uma passada, sem projetar o resto)
for v in wig.data.vertices:
    if v.co.z > htop - 0.08:
        continue
    loc, nrm, idx, d = tree.find_nearest(v.co)
    if loc is None:
        continue
    if (v.co - loc).dot(nrm) < 0:
        v.co = loc + nrm * GAP
        moved += 1
print('Peruca: escala', round(s, 3), '| vértices empurrados para fora do corpo:', moved)
for p in wig.data.polygons:
    p.use_smooth = True
wig.name = 'prop_dreads_vestida'
bpy.ops.object.select_all(action='DESELECT'); wig.select_set(True)
bpy.ops.export_scene.gltf(filepath=out, export_format='GLB', use_selection=True)
print('Exportado:', out)
