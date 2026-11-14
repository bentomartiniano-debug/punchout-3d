"""Dá corpo ao tronco: a metade de trás (do quadril ao ombro) é esticada para trás em relação ao centro de cada fatia
(--costas) e o tronco todo alarga para os lados (--largura: peito e costas, mais no peito, nada no pescoço).
Suave no pescoço e na barra, sem rasgar nada (só move vértices).

  blender -b --factory-startup -P tools3d/blender/engrossar_tronco.py -- corpo.glb saida.glb [--costas 1.3] [--largura 1.0]
"""
import bpy, os, sys

argv = sys.argv[sys.argv.index('--') + 1:]
src, out = os.path.abspath(argv[0]), os.path.abspath(argv[1])
opt = dict(zip(argv[2::2], argv[3::2]))
KB = float(opt.get('--costas', 1.3))
KX = float(opt.get('--largura', 1.0))


def smoothstep(a, b, x):
    t = max(0.0, min(1.0, (x - a) / (b - a)))
    return t * t * (3 - 2 * t)


bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=src)
arm = next(o for o in bpy.context.scene.objects if o.type == 'ARMATURE')
body = max((o for o in arm.children if o.type == 'MESH'), key=lambda o: len(o.data.vertices))
me, mw = body.data, body.matrix_world
inv = mw.inverted()
gs = body.vertex_groups
ARMG = {gs[s + b].index for s in ('Left', 'Right') for b in ('Arm', 'ForeArm', 'Hand') if s + b in gs}
z_sh = (arm.matrix_world @ arm.data.bones['LeftArm'].head_local).z
torso = [(v, mw @ v.co) for v in me.vertices if sum(ge.weight for ge in v.groups if ge.group in ARMG) < 0.5]
# centro (frente/trás) de cada fatia, a partir do tronco no meio (|x| pequeno: sem os braços)
cys = {}
for k in range(0, 90):
    z = 0.3 + k * 0.012
    band = [q.y for _, q in torso if abs(q.z - z) < 0.02 and abs(q.x) < 0.15]
    if band:
        cys[k] = (min(band) + max(band)) / 2
def cy_at(z):
    k = min(cys, key=lambda kk: abs(0.3 + kk * 0.012 - z))
    return cys[k]
n = 0
for v, q in torso:
    w = smoothstep(0.45, 0.85, q.z) * (1 - smoothstep(z_sh - 0.02, z_sh + 0.08, q.z))
    wx = smoothstep(0.62, 0.98, q.z) * (1 - smoothstep(z_sh + 0.01, z_sh + 0.1, q.z))   # peito/costas largos, pescoço não
    c = cy_at(q.z)
    moved = False
    if w > 0 and q.y > c:
        q.y = c + (q.y - c) * (1 + (KB - 1) * w); moved = True
    if wx > 0 and KX != 1:
        q.x = q.x * (1 + (KX - 1) * wx); moved = True
    if moved:
        v.co = inv @ q
        n += 1
me.update()
print('Tronco:', n, 'vértices | costas', KB, '| largura', KX)
bpy.ops.export_scene.gltf(filepath=out, export_format='GLB', export_extras=True, export_attributes=True)
print('Exportado:', out)
