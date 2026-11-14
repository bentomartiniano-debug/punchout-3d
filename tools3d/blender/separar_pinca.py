"""Separa a pinça móvel da garra (a de cima, em gancho, do lado +Y) numa peça própria com a origem na dobradiça,
para o visualizador abrir e fechar a garra girando só ela.

  blender -b --factory-startup -P tools3d/blender/separar_pinca.py -- garra.glb saida.glb [--teste render.png]
  Joana: --hy 0.075 --hz 0.244 (no visualizador, abrir = girar -X: claw.dir -1)
  Região da pinça: acima do vão (z > ZV) e do lado +Y (y > YV), ou a ponta curva por cima (z > ZT); base em y > YB, z > ZB.
"""
import bpy, bmesh, os, sys
from mathutils import Vector, Matrix

argv = sys.argv[sys.argv.index('--') + 1:]
src, out = os.path.abspath(argv[0]), os.path.abspath(argv[1])
opt = dict(zip(argv[2::2], argv[3::2]))
ZV, YV, ZT = float(opt.get('--zv', 0.2625)), float(opt.get('--yv', -0.02)), float(opt.get('--zt', 0.37))
YB, ZB = float(opt.get('--yb', 0.03)), float(opt.get('--zb', 0.235))
HINGE = Vector((0.0, float(opt.get('--hy', 0.06)), float(opt.get('--hz', 0.24))))

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=src)
ob = next(o for o in bpy.context.scene.objects if o.type == 'MESH')
bpy.context.view_layer.objects.active = ob
for o in bpy.context.scene.objects:
    o.select_set(o is ob)
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)


def in_pinca(c):
    return (c.z > ZV and (c.y > YV or c.z > ZT)) or (c.y > YB and c.z > ZB)


bpy.ops.object.mode_set(mode='EDIT'); bpy.ops.mesh.select_mode(type='FACE'); bpy.ops.mesh.select_all(action='DESELECT')
bm = bmesh.from_edit_mesh(ob.data)
n = 0
for f in bm.faces:
    if in_pinca(f.calc_center_median()):
        f.select_set(True); n += 1
bmesh.update_edit_mesh(ob.data)
bpy.ops.mesh.separate(type='SELECTED')
bpy.ops.object.mode_set(mode='OBJECT')
pinca = next(o for o in bpy.context.scene.objects if o.type == 'MESH' and o is not ob)
pinca.name = 'pinca'; ob.name = 'garra'
# tampa o corte nas duas peças (senão a garra aberta mostra o oco): solda os vértices e fecha as bordas
def cap(o):
    mats = [m for m in o.data.materials]
    dark = min(range(len(mats)), key=lambda i: sum(mats[i].node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value[:3]))
    bm = bmesh.new(); bm.from_mesh(o.data)
    bmesh.ops.remove_doubles(bm, verts=bm.verts[:], dist=1e-5)
    edges = [e for e in bm.edges if e.is_boundary]
    new = bmesh.ops.holes_fill(bm, edges=edges, sides=0)['faces']
    for f in new:
        f.material_index = dark
    bmesh.ops.triangulate(bm, faces=new)
    bm.to_mesh(o.data); bm.free(); o.data.update()
    return len(new)
print('Tampas:', cap(ob), 'na garra,', cap(pinca), 'na pinça')
# origem da pinça na dobradiça: o visualizador gira o nó 'pinca' no eixo X local
pinca.data.transform(Matrix.Translation(-HINGE)); pinca.location = HINGE
print('Pinça separada:', n, 'faces | dobradiça', tuple(HINGE))
if '--teste' in opt:
    sc = bpy.context.scene
    sc.render.engine = 'BLENDER_WORKBENCH'; sc.display.shading.light = 'FLAT'; sc.display.shading.color_type = 'OBJECT'
    pinca.color = (0.1, 0.4, 1, 1); ob.color = (1, 0.5, 0.2, 1)
    sc.render.resolution_x = sc.render.resolution_y = 400
    cam = bpy.data.objects.new('cam', bpy.data.cameras.new('cam')); sc.collection.objects.link(cam); sc.camera = cam
    cam.data.type = 'ORTHO'; cam.data.ortho_scale = 0.5; cam.location = Vector((1, 0, 0.2))
    cam.rotation_euler = (Vector((0, 0, 0.2)) - cam.location).to_track_quat('-Z', 'Y').to_euler()
    sc.render.filepath = os.path.abspath(opt['--teste']); bpy.ops.render.render(write_still=True)
    bpy.data.objects.remove(cam)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.export_scene.gltf(filepath=out, export_format='GLB', use_selection=True)
print('Exportado:', out)
