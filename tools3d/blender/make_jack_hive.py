"""Colmeia-luva do Jack modelada em código (a do Meshy veio oca, com parede dupla, e não tem conserto limpo).

  blender -b --factory-startup -P tools3d/blender/make_jack_hive.py -- saida_L.glb saida_R.glb

Barril fechado de 6 anéis estufados (sulcos mais escuros entre eles), topo em cúpula, fundo fechado (o pulso entra
por ele) e dois furos redondos de verdade na frente, com o interior preto. Mesmo eixo das luvas: altura em Z,
base na origem, furos virados para -Y (a frente da folha). A direita é o espelho da esquerda.
"""
import bpy, bmesh, os, sys, math
from mathutils import Vector, Matrix

argv = sys.argv[sys.argv.index('--') + 1:]
out_l, out_r = (os.path.abspath(a) for a in argv[:2])
H, R = 0.34, 0.15          # altura e raio máximo
BANDS, SEG, ROWS = 6, 48, 72


def mat(name, rgb):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    b = m.node_tree.nodes['Principled BSDF']
    b.inputs['Base Color'].default_value = (*[(c / 255) ** 2.2 for c in rgb], 1)
    b.inputs['Roughness'].default_value = 1.0
    return m


YEL, GROOVE, HOLE = mat('colmeia_amarelo', (247, 207, 92)), mat('colmeia_sulco', (214, 163, 58)), mat('colmeia_furo', (20, 12, 6))
WALL = mat('colmeia_parede_furo', (150, 96, 28))   # parede do furo: marrom de cera, dá a profundidade


def radius(z):
    t = z / H
    barrel = R * (0.80 + 0.20 * math.sin(math.pi * min(1, t * 1.05)))         # estufado no meio
    dome = math.sqrt(max(0.0, 1 - ((t - 0.86) / 0.14) ** 2)) if t > 0.86 else 1.0   # topo arredondado
    band = (t * BANDS) % 1.0
    bulge = 0.012 * math.sin(math.pi * band)                                      # cada anel estufa
    return (barrel + bulge) * dome


bm = bmesh.new()
rings = []
for j in range(ROWS + 1):
    z = H * j / ROWS
    r = max(radius(z), 0.0005 if j == ROWS else 0.0)
    rings.append([bm.verts.new((r * math.cos(2 * math.pi * i / SEG), r * math.sin(2 * math.pi * i / SEG), z)) for i in range(SEG)])
for j in range(ROWS):
    band = ((j + 0.5) / ROWS * BANDS) % 1.0
    groove = band < 0.12 or band > 0.88
    for i in range(SEG):
        f = bm.faces.new((rings[j][i], rings[j][(i + 1) % SEG], rings[j + 1][(i + 1) % SEG], rings[j + 1][i]))
        f.material_index = 1 if groove else 0
bot = bm.faces.new(list(reversed(rings[0])))
bot.material_index = 0
top_c = bm.verts.new((0, 0, H))
for i in range(SEG):
    bm.faces.new((rings[ROWS][i], rings[ROWS][(i + 1) % SEG], top_c))
bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
me = bpy.data.meshes.new('hive')
bm.to_mesh(me)
bm.free()
hive = bpy.data.objects.new('prop_jack_hive_L', me)
bpy.context.scene.collection.objects.link(hive)
for m in (YEL, GROOVE, HOLE, WALL):
    me.materials.append(m)

# furos: cilindros subtraídos na frente (-Y); as paredes novas do furo herdam o material preto do cortador
for zc in (0.6 * H, 0.38 * H):
    # furo em funil (largo fora, estreito dentro): mesmo de frente aparece a parede, e o furo lê como fundo
    bpy.ops.mesh.primitive_cone_add(vertices=28, radius1=0.016, radius2=0.04, depth=0.1, location=(0, -radius(zc) - 0.02, zc), rotation=(math.radians(90), 0, 0))
    cut = bpy.context.object
    cut.data.materials.append(HOLE)
    mod = hive.modifiers.new('furo', 'BOOLEAN')
    mod.operation = 'DIFFERENCE'
    mod.object = cut
    mod.solver = 'EXACT'
    mod.material_mode = 'TRANSFER'
    bpy.context.view_layer.objects.active = hive
    bpy.ops.object.modifier_apply(modifier='furo')
    bpy.data.objects.remove(cut, do_unlink=True)
# paredes do furo (de lado para o eixo) ficam marrom de cera; só o fundo fica preto -> lê como buraco fundo
wall_i = list(me.materials).index(WALL)
for p in me.polygons:
    is_hole = me.materials[p.material_index].name.startswith('colmeia_furo')
    if is_hole and abs(p.normal.y) < 0.6:
        p.material_index = wall_i
    p.use_smooth = not is_hole   # dentro do furo: faces retas (com sombreado liso parecia uma bola preta)
print('Colmeia:', len(me.polygons), 'faces')

bpy.ops.object.select_all(action='DESELECT')
hive.select_set(True)
bpy.ops.export_scene.gltf(filepath=out_l, export_format='GLB', use_selection=True)
hive.data.transform(Matrix.Scale(-1, 4, (1, 0, 0)))
hive.data.flip_normals()
hive.name = 'prop_jack_hive_R'
bpy.ops.export_scene.gltf(filepath=out_r, export_format='GLB', use_selection=True)
print('Exportado:', out_l, out_r)
