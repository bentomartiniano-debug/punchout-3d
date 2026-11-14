"""Punho fechado da Valéria em código (na cor da pele): palma, quatro dedos enrolados em volta do cabo do guarda-chuva e
o polegar por cima. Origem no pulso, +Y para os dedos (eixo do antebraço), cabo no eixo X passando em (0, GY, 0);
nós dos dedos para +Z. A mão direita é o espelho da esquerda em Z.

  blender -b --factory-startup -P tools3d/blender/make_valeria_fist.py -- saida_L.glb saida_R.glb [--pele "#e49d6f"]
"""
import bpy, bmesh, os, sys, math
from mathutils import Vector, Matrix

argv = sys.argv[sys.argv.index('--') + 1:]
outL, outR = os.path.abspath(argv[0]), os.path.abspath(argv[1])
opt = dict(zip(argv[2::2], argv[3::2]))
hexc = opt.get('--pele', '#e49d6f').lstrip('#')
SKIN = tuple((int(hexc[i:i + 2], 16) / 255) ** 2.2 for i in (0, 2, 4))
GY = 0.075            # altura do cabo (a partir do pulso)
RC = 0.026            # raio do cabo + folga: os dedos passam em volta dele


def build():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    parts = []
    # palma: bloco arredondado do pulso até a base dos dedos (atrás do cabo, lado -Z = palma encostada no cabo)
    bpy.ops.mesh.primitive_cube_add(size=1, location=(0, 0.042, 0.012))
    palm = bpy.context.object; palm.scale = (0.078, 0.07, 0.042)
    bpy.ops.object.transform_apply(scale=True)
    m = palm.modifiers.new('b', 'BEVEL'); m.width = 0.014; m.segments = 4
    parts.append(palm)
    # dedos: arcos (toro parcial) em volta do cabo, do dorso (+Z) pela frente até a palma (-Z)
    for i, x in enumerate((-0.027, -0.009, 0.009, 0.027)):
        r_tube = 0.0115 if i else 0.0105            # mindinho um pouco mais fino
        bm = bmesh.new()
        NA, NT = 14, 8
        a0, a1 = math.radians(-70), math.radians(200)          # ângulo em volta do cabo (0 = +Y, 90 = -Z)
        R = RC + r_tube
        rings = []
        for j in range(NA + 1):
            a = a0 + (a1 - a0) * j / NA
            c = Vector((x, GY + R * math.cos(a), -R * math.sin(a)))
            tng = Vector((0, -math.sin(a), -math.cos(a)))
            n1 = Vector((1, 0, 0)); n2 = tng.cross(n1).normalized()
            rt = r_tube * (1.0 if j < NA - 2 else 0.8)
            rings.append([bm.verts.new(c + (n1 * math.cos(2 * math.pi * k / NT) + n2 * math.sin(2 * math.pi * k / NT)) * rt) for k in range(NT)])
        for r0, r1 in zip(rings, rings[1:]):
            for k in range(NT):
                bm.faces.new((r0[k], r0[(k + 1) % NT], r1[(k + 1) % NT], r1[k]))
        bm.faces.new(list(reversed(rings[0]))); bm.faces.new(rings[-1])
        me = bpy.data.meshes.new('dedo%d' % i); bm.to_mesh(me); bm.free()
        ob = bpy.data.objects.new('dedo%d' % i, me); bpy.context.scene.collection.objects.link(ob); parts.append(ob)
    # polegar: cápsula do lado +X (ponta do guarda-chuva), deitada por cima dos dedos na frente
    bpy.ops.mesh.primitive_uv_sphere_add(segments=16, ring_count=10, radius=1, location=(0.03, GY + 0.004, -0.034))
    th = bpy.context.object; th.scale = (0.016, 0.03, 0.013); th.rotation_euler = (0, 0, math.radians(-25))
    bpy.ops.object.transform_apply(scale=True, rotation=True)
    parts.append(th)
    # junta tudo, suaviza
    for o in bpy.context.scene.objects:
        o.select_set(o in parts)
    bpy.context.view_layer.objects.active = palm
    for o in parts:
        for md in list(o.modifiers):
            bpy.context.view_layer.objects.active = o; bpy.ops.object.modifier_apply(modifier=md.name)
    bpy.context.view_layer.objects.active = palm
    bpy.ops.object.join()
    fist = bpy.context.object; fist.name = 'punho'
    for p in fist.data.polygons:
        p.use_smooth = True
    mat = bpy.data.materials.new('pele'); mat.use_nodes = True
    mat.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value = (*SKIN, 1)
    fist.data.materials.clear(); fist.data.materials.append(mat)
    return fist


for out, mirror in ((outL, False), (outR, True)):
    fist = build()
    if mirror:
        fist.data.transform(Matrix.Scale(-1, 4, Vector((0, 0, 1))))
        fist.data.flip_normals()
    bpy.ops.object.select_all(action='DESELECT'); fist.select_set(True)
    # glTF: Y do Blender vira -Z do three; queremos +Y (dedos) e cabo em X no espaço do three -> gira para exportar Y-up certo
    bpy.ops.export_scene.gltf(filepath=out, export_format='GLB', use_selection=True, export_yup=False)
    print('Punho:', out, len(fist.data.vertices), 'vértices')
