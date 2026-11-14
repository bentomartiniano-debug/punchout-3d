"""Barra do casaco/jaleco presa só no quadril: as coxas não puxam (nem esticam) o pano na esquiva e no salto.
Usa o atributo _SWAY_COAT (gravado por pesos_balanco.py) para achar a barra. Não mexe na geometria.

  blender -b --factory-startup -P tools3d/blender/saia_no_quadril.py -- corpo.glb saida.glb [--cintura 0.9]
"""
import bpy, os, sys

argv = sys.argv[sys.argv.index('--') + 1:]
src, out = os.path.abspath(argv[0]), os.path.abspath(argv[1])
waist = float(argv[argv.index('--cintura') + 1]) if '--cintura' in argv else 0.9

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=src)
arm = next(o for o in bpy.context.scene.objects if o.type == 'ARMATURE')
body = max((o for o in arm.children if o.type == 'MESH'), key=lambda o: len(o.data.vertices))
me, mw = body.data, body.matrix_world
att = me.attributes.get('_SWAY_COAT')
legs = [g for g in body.vertex_groups if 'Leg' in g.name or 'Foot' in g.name or 'Toe' in g.name]
hips = body.vertex_groups.get('Hips')
n = 0
# o pano é o que está longe do osso da coxa (a calça fica colada nele); vale também para o forro de dentro
from mathutils.geometry import intersect_point_line
segs = []
for side in ('Left', 'Right'):
    for b in ('UpLeg', 'Leg'):
        bb = arm.data.bones[side + b]
        segs.append((arm.matrix_world @ bb.head_local, arm.matrix_world @ bb.tail_local))
def leg_dist(q):
    best = 9.0
    for a0, a1 in segs:
        pt, t = intersect_point_line(q, a0, a1)
        t = max(0.0, min(1.0, t)); pt = a0.lerp(a1, t)
        best = min(best, (q - pt).length)
    return best
HEM = float(argv[argv.index('--barra') + 1]) if '--barra' in argv else 0.62
KEEP = float(argv[argv.index('--perna') + 1]) if '--perna' in argv else 0.5   # quanto do peso fica nas coxas
GAP = float(argv[argv.index('--folga') + 1]) if '--folga' in argv else 0.125
for v in me.vertices:
    q = mw @ v.co
    if q.z > waist or q.z < 0.4:
        continue
    if att.data[v.index].value <= 0 and leg_dist(q) < GAP:
        continue
    leg_w = sum(ge.weight for ge in v.groups if body.vertex_groups[ge.group] in legs)
    if leg_w <= 0:
        continue
    # parte do peso fica nas coxas (o pano acompanha a perna e ela não fura o jaleco), o resto vai para o quadril
    # (não estica nem rasga). Mesma regra para o pano de fora e o forro -> sem frestas entre eles.
    for ge in list(v.groups):
        g = body.vertex_groups[ge.group]
        if g in legs:
            g.add([v.index], ge.weight * KEEP, 'REPLACE')
    hips.add([v.index], leg_w * (1 - KEEP), 'ADD')
    # forro e pano de fora balançam juntos (antes só o branco tinha peso: as camadas se separavam e o
    # contorno de quadrinho desenhava as frestas em preto)
    ramp = max(0.0, min(1.0, (waist - q.z) / (waist - HEM))) ** 1.3
    att.data[v.index].value = max(att.data[v.index].value, ramp)
    n += 1
print('Vértices da barra presos no quadril:', n)
bpy.ops.export_scene.gltf(filepath=out, export_format='GLB', export_extras=True, export_attributes=True)
print('Exportado:', out)
