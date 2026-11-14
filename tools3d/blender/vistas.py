"""Renderiza vistas ortográficas de um .glb para conferir forma (sem abrir o Blender).

  blender -b --factory-startup -P tools3d/blender/vistas.py -- modelo.glb pasta_saida [--zoom-cabeca]
Gera frente/costas/lado/tres_quartos_costas .png com iluminação de estúdio (workbench, sem textura).
"""
import bpy, os, sys, math
from mathutils import Vector

argv = sys.argv[sys.argv.index('--') + 1:]
src, outdir = os.path.abspath(argv[0]), os.path.abspath(argv[1])
zoom_head = '--zoom-cabeca' in argv
os.makedirs(outdir, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=src)
scene = bpy.context.scene
for o in list(scene.objects):
    if '--sem-cinto' in argv and o.type == 'MESH' and o.name.startswith('belt'):
        bpy.data.objects.remove(o, do_unlink=True)   # olha o corpo por baixo do cinturão
meshes = [o for o in scene.objects if o.type == 'MESH']
dg = bpy.context.evaluated_depsgraph_get()
pts = [m.matrix_world @ v.co for m in meshes for v in m.evaluated_get(dg).to_mesh().vertices]
lo = Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts)))
hi = Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts)))
c = (lo + hi) / 2
size = max(hi - lo)
if zoom_head:
    c.z = hi.z - (hi.z - lo.z) * 0.18
    size *= 0.45
scene.render.engine = 'BLENDER_WORKBENCH'
scene.display.shading.light = 'STUDIO'
scene.display.shading.color_type = 'TEXTURE' if '--textura' in argv else 'SINGLE'
scene.display.shading.show_cavity = True
scene.render.resolution_x = scene.render.resolution_y = 700
cam = bpy.data.objects.new('cam', bpy.data.cameras.new('cam'))
cam.data.type = 'ORTHO'
cam.data.ortho_scale = size * 1.1
scene.collection.objects.link(cam)
scene.camera = cam
for name, ang in (('frente', 0), ('costas', 180), ('lado', 90), ('tres_quartos_costas', 140)):
    a = math.radians(ang)
    d = Vector((math.sin(a), -math.cos(a), 0))     # frente do personagem = -Y
    cam.location = c + d * 10
    cam.rotation_euler = (-d).to_track_quat('-Z', 'Y').to_euler()
    scene.render.filepath = os.path.join(outdir, name + '.png')
    bpy.ops.render.render(write_still=True)
print('Vistas em', outdir)
