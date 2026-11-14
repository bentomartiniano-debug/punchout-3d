"""Joana: leva a haste do olho de siri (sai da bochecha e sobe pela lateral da cabeça) para a frente e para fora, para
o olho ficar fora da aba do chapéu e visível de frente. A base na bochecha fica parada; quanto mais alto, mais anda.

  blender -b --factory-startup -P tools3d/blender/mover_olho_siri.py -- corpo.glb saida.glb
          [--x -0.215] [--base 1.42] [--frente 0.2] [--fora 0.04] [--baixar 0.0]
  Joana: --frente 0.2 --fora 0.07 --baixar 0.05
"""
import bpy, os, sys

argv = sys.argv[sys.argv.index('--') + 1:]
src, out = os.path.abspath(argv[0]), os.path.abspath(argv[1])
opt = dict(zip(argv[2::2], argv[3::2]))
XS, ZR = float(opt.get('--x', -0.215)), float(opt.get('--base', 1.42))
FWD, OUT, DOWN = float(opt.get('--frente', 0.2)), float(opt.get('--fora', 0.04)), float(opt.get('--baixar', 0.0))

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=src)
arm = next(o for o in bpy.context.scene.objects if o.type == 'ARMATURE')
body = max((o for o in arm.children if o.type == 'MESH'), key=lambda o: len(o.data.vertices))
me, mw = body.data, body.matrix_world
inv = mw.inverted()
sel = [(v, mw @ v.co) for v in me.vertices if (mw @ v.co).x < XS and (mw @ v.co).z > ZR]
zt = max(q.z for _, q in sel)
for v, q in sel:
    t = min(1.0, (q.z - ZR) / (zt - ZR))
    s = t * t * (3 - 2 * t)
    q.y -= FWD * s          # frente do corpo = -Y
    q.x -= OUT * s
    q.z -= DOWN * s
    v.co = inv @ q
me.update()
print('Olho de siri:', len(sel), 'vértices movidos | base', ZR, 'topo', round(zt, 3))
bpy.ops.export_scene.gltf(filepath=out, export_format='GLB', export_extras=True, export_attributes=True)
print('Exportado:', out)
