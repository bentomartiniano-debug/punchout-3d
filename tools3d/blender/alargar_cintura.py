"""Alarga a cintura (só move vértices): em volta da altura --z, o tronco cresce para os lados (x) e um pouco na
profundidade, com queda suave (gaussiana) para cima e para baixo; braços e mãos não mexem.

  blender -b --factory-startup -P tools3d/blender/alargar_cintura.py -- corpo.glb saida.glb [--z 0.99] [--fator 1.15] [--altura 0.09]
"""
import bpy, os, sys, math

argv = sys.argv[sys.argv.index('--') + 1:]
src, out = os.path.abspath(argv[0]), os.path.abspath(argv[1])
opt = dict(zip(argv[2::2], argv[3::2]))
ZC, F, HW = float(opt.get('--z', 0.99)), float(opt.get('--fator', 1.15)), float(opt.get('--altura', 0.09))
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=src)
arm = next(o for o in bpy.context.scene.objects if o.type == 'ARMATURE')
body = max((o for o in arm.children if o.type == 'MESH'), key=lambda o: len(o.data.vertices))
mw = body.matrix_world; inv = mw.inverted(); gs = body.vertex_groups
ARMG = {gs[n].index for n in gs.keys() if 'Arm' in n or 'Hand' in n}
P = [mw @ v.co for v in body.data.vertices]
band = [q for q in P if abs(q.z - ZC) < 0.015 and abs(q.x) < 0.25]
cy = (min(q.y for q in band) + max(q.y for q in band)) / 2
n = 0
for v, q in zip(body.data.vertices, P):
    w = math.exp(-((q.z - ZC) / HW) ** 2)
    if w < 0.01 or sum(ge.weight for ge in v.groups if ge.group in ARMG) > 0.3 or abs(q.x) > 0.3:
        continue
    q.x *= 1 + (F - 1) * w
    q.y = cy + (q.y - cy) * (1 + (F - 1) * 0.4 * w)
    v.co = inv @ q; n += 1
body.data.update()
print('Cintura alargada:', n, 'vértices | fator', F, 'em z', ZC)
bpy.ops.export_scene.gltf(filepath=out, export_format='GLB', export_extras=True, export_attributes=True)
print('Exportado:', out)
