"""Gancho de pirata da Joana modelado em código (o do Meshy veio com um boneco inventado embaixo).

  blender -b --factory-startup -P tools3d/blender/make_joana_hook.py -- saida.glb
Copo (punho) cinza na base, em cima a haste e o gancho prateado em arco (curva em "?"), ponta afiada.
Mesmo eixo das luvas: altura em Z, base (boca do pulso) na origem.
"""
import bpy, bmesh, os, sys, math
from mathutils import Vector, Matrix

out = os.path.abspath(sys.argv[sys.argv.index('--') + 1])
bpy.ops.wm.read_factory_settings(use_empty=True)


def mat(name, rgb):
    m = bpy.data.materials.new(name); m.use_nodes = True
    b = m.node_tree.nodes['Principled BSDF']
    b.inputs['Base Color'].default_value = (*[(c / 255) ** 2.2 for c in rgb], 1); b.inputs['Roughness'].default_value = 1.0
    return m


CUP, BAND, STEEL = mat('gancho_copo', (96, 104, 116)), mat('gancho_faixa', (40, 44, 52)), mat('gancho_aco', (196, 204, 214))
parts = []
# copo do punho (mais largo embaixo)
bpy.ops.mesh.primitive_cone_add(vertices=32, radius1=0.085, radius2=0.068, depth=0.14, location=(0, 0, 0.07))
o = bpy.context.object; o.data.materials.append(CUP); parts.append(o)
bpy.ops.mesh.primitive_cylinder_add(vertices=32, radius=0.088, depth=0.025, location=(0, 0, 0.02))
o = bpy.context.object; o.data.materials.append(BAND); parts.append(o)
bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=0.045, depth=0.03, location=(0, 0, 0.155))
o = bpy.context.object; o.data.materials.append(BAND); parts.append(o)
# haste + arco do gancho: tubo seguindo uma curva
R, cz = 0.085, 0.27
pts = [Vector((0, 0, 0.16)), Vector((0, 0, cz))]
for i in range(1, 25):   # arco a partir do topo da haste: sobe, passa por cima e desce do outro lado (formato de "?")
    a = math.radians(225 * i / 24)
    pts.append(Vector((-R + R * math.cos(a), 0, cz + R * math.sin(a))))
cu = bpy.data.curves.new('gancho', 'CURVE'); cu.dimensions = '3D'; cu.bevel_depth = 0.02; cu.bevel_resolution = 4
sp = cu.splines.new('POLY'); sp.points.add(len(pts) - 1)
for p, v in zip(sp.points, pts):
    p.co = (v.x, v.y, v.z, 1)
for i, p in enumerate(sp.points):
    p.radius = 1.0 if i < len(pts) - 6 else max(0.15, 1.0 - (i - (len(pts) - 6)) / 6)   # afina na ponta
ob = bpy.data.objects.new('gancho', cu); bpy.context.scene.collection.objects.link(ob)
bpy.context.view_layer.objects.active = ob; ob.select_set(True)
bpy.ops.object.convert(target='MESH'); ob = bpy.context.object; ob.data.materials.append(STEEL); parts.append(ob)
bpy.ops.object.select_all(action='DESELECT')
for o in parts:
    o.select_set(True)
bpy.context.view_layer.objects.active = parts[0]
bpy.ops.object.join()
hook = bpy.context.object; hook.name = 'prop_joana_hook_L'
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
for p in hook.data.polygons:
    p.use_smooth = True
print('Gancho:', len(hook.data.polygons), 'faces')
bpy.ops.export_scene.gltf(filepath=out, export_format='GLB')
print('Exportado:', out)
