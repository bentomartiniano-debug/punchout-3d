"""Abelha low-poly do enxame do Jack (feita em código: pequena demais para gastar crédito do Meshy).

  blender -b --factory-startup -P tools3d/blender/make_jack_bee.py -- saida.glb

Corpo em 3 gomos (cabeça, tórax, abdômen listrado amarelo/preto), ferrão, 2 asas brancas e olhos pretos.
Cores chapadas por material (o visualizador põe o sombreado de quadrinho). Tamanho real: ~6 cm de comprimento,
o enxame instancia e escala. Frente = +Y do Blender (vira -Z no glTF... o visualizador orienta pela velocidade).
"""
import bpy, bmesh, os, sys, math
from mathutils import Vector, Matrix

out = os.path.abspath(sys.argv[sys.argv.index('--') + 1])
bpy.ops.wm.read_factory_settings(use_empty=True)


def mat(name, rgb, alpha=1.0):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    b = m.node_tree.nodes['Principled BSDF']
    b.inputs['Base Color'].default_value = (*[(c / 255) ** 2.2 for c in rgb], 1)
    b.inputs['Roughness'].default_value = 1.0
    if alpha < 1:
        b.inputs['Alpha'].default_value = alpha
        m.surface_render_method = 'BLENDED'
    return m


YEL, BLK, WING, EYE = mat('abelha_amarelo', (245, 196, 40)), mat('abelha_preto', (30, 26, 24)), mat('abelha_asa', (235, 245, 250), 0.85), mat('abelha_olho', (12, 10, 10))
parts = []


def blob(name, loc, scale, material, seg=10, rings=7):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=seg, ring_count=rings, location=loc)
    o = bpy.context.object
    o.scale = scale
    o.name = name
    o.data.materials.append(material)
    parts.append(o)
    return o


# abdômen listrado: faces pintadas por faixa de Y (amarelo e preto alternando)
ab = blob('abdomen', (0, -0.022, 0), (0.016, 0.024, 0.016), YEL, 16, 13)
ab.data.materials.append(BLK)
for p in ab.data.polygons:
    y = p.center.y
    p.material_index = 1 if (int((y + 1.0) * 3.2) % 2 == 0) else 0
blob('torax', (0, 0.006, 0.001), (0.012, 0.012, 0.012), BLK)
head = blob('cabeca', (0, 0.022, 0.002), (0.009, 0.009, 0.009), YEL)
blob('olho_L', (0.006, 0.027, 0.004), (0.003, 0.003, 0.003), EYE, 8, 5)
blob('olho_R', (-0.006, 0.027, 0.004), (0.003, 0.003, 0.003), EYE, 8, 5)
# ferrão: cone preto atrás
bpy.ops.mesh.primitive_cone_add(vertices=8, radius1=0.004, depth=0.01, location=(0, -0.05, 0), rotation=(math.radians(90), 0, 0))
st = bpy.context.object; st.data.materials.append(BLK); parts.append(st)
# asas: elipses achatadas inclinadas para cima e para trás
for sx in (1, -1):
    w = blob('asa', (sx * 0.014, 0.0, 0.016), (0.014, 0.008, 0.0012), WING, 10, 5)
    w.rotation_euler = (math.radians(-10), math.radians(sx * 25), math.radians(sx * -20))

for o in parts:
    o.select_set(True)
bpy.context.view_layer.objects.active = parts[0]
bpy.ops.object.join()
bee = bpy.context.object
bee.name = 'prop_jack_bee'
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
for p in bee.data.polygons:
    p.use_smooth = True
print('Abelha:', len(bee.data.polygons), 'faces')
bpy.ops.export_scene.gltf(filepath=out, export_format='GLB')
print('Exportado:', out)
