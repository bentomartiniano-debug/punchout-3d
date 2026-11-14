"""Testa saliente da Lady Espinha (como no sprite): a parte de cima da frente da cabeça avança num degrau, de lado a
lado; os olhos ficam embaixo dele. Só move vértices da cabeça.

  blender -b --factory-startup -P tools3d/blender/testa_espinha.py -- corpo.glb saida.glb [--altura 0.33] [--avanco 0.03]
  --altura: onde fica o degrau, em fração da altura da cabeça a partir do topo
"""
import bpy, os, sys

argv = sys.argv[sys.argv.index('--') + 1:]
src, out = os.path.abspath(argv[0]), os.path.abspath(argv[1])
opt = dict(zip(argv[2::2], argv[3::2]))
FR, AV = float(opt.get('--altura', 0.33)), float(opt.get('--avanco', 0.03))
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=src)
arm = next(o for o in bpy.context.scene.objects if o.type == 'ARMATURE')
body = max((o for o in arm.children if o.type == 'MESH'), key=lambda o: len(o.data.vertices))
mw = body.matrix_world; inv = mw.inverted(); g = body.vertex_groups['Head']
hv = [(v, mw @ v.co) for v in body.data.vertices if any(ge.group == g.index and ge.weight > 0.5 for ge in v.groups)]
z0 = (arm.matrix_world @ arm.data.bones['Head'].head_local).z
ztop = max(q.z for _, q in hv); zb = ztop - FR * (ztop - z0)
ys = [q.y for _, q in hv]; cy = (min(ys) + max(ys)) / 2; yf = min(ys)


def ss(a, b, x):
    t = max(0.0, min(1.0, (x - a) / (b - a))); return t * t * (3 - 2 * t)


n = 0
for v, q in hv:
    if q.y > cy:
        continue
    w = ss(zb - 0.012, zb + 0.006, q.z) * ss(cy, yf + 0.02, q.y) * (1 - 0.5 * ss(ztop - 0.04, ztop, q.z))
    if w <= 0:
        continue
    q.y -= AV * w
    v.co = inv @ q; n += 1
body.data.update()
print('Testa:', n, 'vértices | degrau em z', round(zb, 3), '| avanço', AV)
bpy.ops.export_scene.gltf(filepath=out, export_format='GLB', export_extras=True, export_attributes=True)
print('Exportado:', out)
