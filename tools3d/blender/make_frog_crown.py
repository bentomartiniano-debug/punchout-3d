"""Coroa do King Frog modelada por script (referência: assets3d/frog/input/crown_ref.png). Sem créditos do Meshy.

  blender -b --factory-startup -P tools3d/blender/make_frog_crown.py -- saida.glb

Anel dourado com 5 pontas (a do meio maior), bolinhas nas pontas e gema vermelha em losango na frente.
Origem na base do anel (é onde encaixa no soquete head_top). Frente para +Y no Blender (= +Z no glTF).
"""
import bpy, bmesh, math, sys
from mathutils import Vector

out = sys.argv[sys.argv.index('--') + 1]
bpy.ops.wm.read_factory_settings(use_empty=True)


def mat(name, rgb):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    b = m.node_tree.nodes['Principled BSDF']
    b.inputs['Base Color'].default_value = (*rgb, 1)
    b.inputs['Roughness'].default_value = 1.0
    return m


GOLD, GOLD_DARK, RUBY = mat('ouro', (0.86, 0.66, 0.16)), mat('ouro_escuro', (0.62, 0.44, 0.08)), mat('rubi', (0.75, 0.05, 0.08))
R, BAND, SEG = 0.11, 0.035, 40          # raio do anel, altura da faixa, segmentos
PEAKS = [(-90, 0.075), (-90 + 36, 0.055), (-90 - 36, 0.055), (-90 + 72, 0.045), (-90 - 72, 0.045)]  # (ângulo da ponta, altura); -90° = frente

bm = bmesh.new()
# perfil superior do anel: a borda sobe em cada ponta (dente triangular) e desce entre elas
def top_height(deg):
    h = 0.0
    for a, ph in PEAKS:
        d = abs((deg - a + 180) % 360 - 180)
        h = max(h, ph * max(0.0, 1 - d / 17))
    return BAND + h

thick = 0.012
rings = []
for side_r in (R, R - thick):
    bot, top = [], []
    for i in range(SEG * 3):
        deg = i * 360 / (SEG * 3)
        a = math.radians(deg)
        x, y = side_r * math.cos(a), side_r * math.sin(a)
        bot.append(bm.verts.new((x, y, 0)))
        top.append(bm.verts.new((x, y, top_height(deg))))
    rings.append((bot, top))
(ob, ot), (ib, it) = rings
n = len(ob)
for i in range(n):
    j = (i + 1) % n
    bm.faces.new((ob[i], ob[j], ot[j], ot[i]))      # fora
    bm.faces.new((ib[j], ib[i], it[i], it[j]))      # dentro
    bm.faces.new((ot[i], ot[j], it[j], it[i]))      # topo
    bm.faces.new((ib[i], ib[j], ob[j], ob[i]))      # base
me = bpy.data.meshes.new('coroa')
bm.to_mesh(me)
bm.free()
crown = bpy.data.objects.new('prop_frog_crown', me)
bpy.context.collection.objects.link(crown)
crown.data.materials.append(GOLD)
# faixa mais escura na base (segunda cor da referência)
bpy.ops.mesh.primitive_cylinder_add(vertices=48, radius=R + 0.004, depth=0.012, location=(0, 0, 0.008))
base = bpy.context.object
base.name = 'faixa'
base.data.materials.append(GOLD_DARK)
# bolinhas nas pontas
parts = [crown, base]
for a, ph in PEAKS:
    t = math.radians(a)
    bpy.ops.mesh.primitive_uv_sphere_add(segments=10, ring_count=6, radius=0.009, location=(R * math.cos(t), R * math.sin(t), BAND + ph + 0.004))
    s = bpy.context.object
    s.data.materials.append(GOLD)
    parts.append(s)
# gema em losango na frente (-Y no Blender = frente do personagem depois da exportação com +Y para cima)
bpy.ops.mesh.primitive_cone_add(vertices=4, radius1=0.018, depth=0.012, location=(0, -R - 0.006, BAND * 0.85), rotation=(math.radians(90), 0, 0))
g = bpy.context.object
g.scale = (1, 1.4, 1)
g.data.materials.append(RUBY)
parts.append(g)
bpy.ops.object.select_all(action='DESELECT')
for p in parts:
    p.select_set(True)
bpy.context.view_layer.objects.active = crown
bpy.ops.object.join()
bpy.ops.object.shade_flat()
bpy.ops.export_scene.gltf(filepath=out, export_format='GLB', use_selection=True)
print('Coroa exportada:', out)
