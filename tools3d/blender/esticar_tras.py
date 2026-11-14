"""Estica para trás só a metade de trás de um prop (ex.: aba de trás do chapéu cobrindo a raiz dos dreads).
A frente (-Y) não muda; atrás do centro, y vai para c + (y - c) * fator.

  blender -b --factory-startup -P tools3d/blender/esticar_tras.py -- prop.glb saida.glb [--fator 1.3]
"""
import bpy, os, sys

argv = sys.argv[sys.argv.index('--') + 1:]
src, out = os.path.abspath(argv[0]), os.path.abspath(argv[1])
F = float(argv[argv.index('--fator') + 1]) if '--fator' in argv else 1.3
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=src)
obs = [o for o in bpy.context.scene.objects if o.type == 'MESH']
ys = [(o.matrix_world @ v.co).y for o in obs for v in o.data.vertices]
c = (min(ys) + max(ys)) / 2
for o in obs:
    inv = o.matrix_world.inverted()
    for v in o.data.vertices:
        q = o.matrix_world @ v.co
        if q.y > c:
            q.y = c + (q.y - c) * F
            v.co = inv @ q
    o.data.update()
print('Esticado para trás: fator', F, '| y de', round(min(ys), 3), 'a', round(max(ys), 3), '-> centro', round(c, 3))
bpy.ops.export_scene.gltf(filepath=out, export_format='GLB', export_extras=True)
print('Exportado:', out)
