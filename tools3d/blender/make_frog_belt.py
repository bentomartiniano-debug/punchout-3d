"""Cinturão de campeão do King Frog, modelado e pintado limpo (a textura do Meshy no cinturão vem picotada
em dezenas de ilhas com sombra pintada, e não dá para desenhar contorno reto nela).

  blender -b --factory-startup -P tools3d/blender/make_frog_belt.py -- corpo.glb saida.glb textura_saida.png

1. Acha a altura do cinturão pelas faces douradas do corpo.
2. Mede a cintura com raios saindo do eixo do corpo (o cinturão contorna a barriga certinho, por fora).
3. Monta uma faixa com placa frontal mais alta (afunila nas laterais, como na folha), fivela e plaquinhas
   em relevo leve.
4. UV limpo (ângulo x altura) e textura desenhada à mão em código: dourado chapado, contorno preto em todas
   as bordas, fivela com o "C" vazado e plaquinhas.
5. Pesos copiados do corpo (vértice mais próximo) -> o cinturão dobra junto com quadril e coluna.
"""
import bpy, bmesh, os, sys, math, json, subprocess
from mathutils import Vector
from mathutils.bvhtree import BVHTree

argv = sys.argv[sys.argv.index('--') + 1:]
src, out, tex_out = (os.path.abspath(a) for a in argv[:3])

GOLD = (217, 175, 77)
INK = (32, 30, 31)
N_ANG = 144
OFFSET = 0.012          # folga por fora da barriga
THICK = 0.025
TW, TH = 2048, 256      # textura: u = volta inteira, v = altura absoluta


def smoothstep(a, b, x):
    t = max(0.0, min(1.0, (x - a) / (b - a)))
    return t * t * (3 - 2 * t)


bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=src)
scene = bpy.context.scene
arm = next(o for o in scene.objects if o.type == 'ARMATURE')
body = next(o for o in arm.children if o.type == 'MESH')
for o in list(scene.objects):
    if o.type == 'MESH' and o.name.startswith('belt'):
        bpy.data.objects.remove(o, do_unlink=True)   # roda de novo sem duplicar

dg = bpy.context.evaluated_depsgraph_get()
mw = body.matrix_world
me = body.data

# 1. faixa de altura do dourado
img = next(n.image for m in me.materials if m and m.use_nodes for n in m.node_tree.nodes if n.type == 'TEX_IMAGE' and n.image)
W, H = img.size
px = img.pixels[:]
uvl = me.uv_layers.active.data
gold_back = []
for poly in me.polygons:
    u = sum(uvl[l].uv.x for l in poly.loop_indices) / poly.loop_total
    v = sum(uvl[l].uv.y for l in poly.loop_indices) / poly.loop_total
    k = (min(H - 1, max(0, int(v * H))) * W + min(W - 1, max(0, int(u * W)))) * 4
    c = [x * 255 for x in px[k:k + 3]]
    p = mw @ poly.center
    if sum((a - b) ** 2 for a, b in zip(c, GOLD)) < 45 ** 2 and 0.6 < p.z < 1.4 and p.y > 0:
        gold_back.append(p.z)
gold_back.sort()
z_lo = gold_back[int(len(gold_back) * 0.03)]
z_hi = gold_back[int(len(gold_back) * 0.97)]
pad = (z_hi - z_lo) * 0.08
z_lo, z_hi = z_lo - pad, z_hi + pad
print('Cinturão: altura', round(z_lo, 3), '->', round(z_hi, 3))

# perfil da altura ao redor: placa da frente sobe/desce (frente = -Y, ângulo 0)
band_h = z_hi - z_lo
PLATE_UP, PLATE_DOWN = band_h * 0.42, band_h * 0.30


def plate(theta):
    a = abs(math.atan2(math.sin(theta), math.cos(theta)))
    return 1 - smoothstep(math.radians(30), math.radians(46), a)


def top(theta):
    return z_hi + PLATE_UP * plate(theta)


def bot(theta):
    return z_lo - PLATE_DOWN * plate(theta)


# 2. raio da cintura por ângulo (o maior entre as alturas da faixa), medido no corpo em repouso
bm_eval = bmesh.new()
bm_eval.from_object(body, dg)
bm_eval.transform(mw)
tree = BVHTree.FromBMesh(bm_eval)
zs_all = [p.z for p in [v.co for v in bm_eval.verts] if z_lo - 0.1 < p.z < z_hi + 0.1]
cy = 0.0
ring = [v.co for v in bm_eval.verts if abs(v.co.z - (z_lo + z_hi) / 2) < 0.03]
if ring:
    cy = (min(p.y for p in ring) + max(p.y for p in ring)) / 2
radius = []
for i in range(N_ANG):
    th = 2 * math.pi * i / N_ANG
    d = Vector((math.sin(th), -math.cos(th), 0))
    best = 0.0
    for j in range(9):
        z = bot(th) + (top(th) - bot(th)) * j / 8
        o = Vector((0, cy, z))
        hit = tree.ray_cast(o, d, 1.0)   # de dentro para fora: o braço pendurado do lado não atrapalha
        if hit[0]:
            best = max(best, (hit[0] - o).length)
    radius.append(best)
# alisa a volta e garante que não afunda
radius = [max(radius[i], sum(radius[(i + k) % N_ANG] for k in range(-4, 5)) / 9) for i in range(N_ANG)]
for _ in range(3):
    radius = [sum(radius[(i + k) % N_ANG] for k in range(-2, 3)) / 5 for i in range(N_ANG)]
bm_eval.free()

# relevo: fivela e plaquinhas (ângulo em graus, meia-largura em graus, alto/baixo relativo, empurrão)
z_mid = (z_lo + z_hi) / 2
BUCKLE = (0.0, 12.5, z_lo - PLATE_DOWN * 0.45, z_hi + PLATE_UP * 0.6, 0.018)
SMALL = [(s * a, 3.4, z_lo + band_h * 0.12, z_hi - band_h * 0.05, 0.008) for s in (-1, 1) for a in (19.0, 26.5)]
RAISED = [BUCKLE] + SMALL


def bump(th, z):
    deg = math.degrees(math.atan2(math.sin(th), math.cos(th)))
    out = 0.0
    for a, hw, zl, zh, push in RAISED:
        e = 0.9
        fx = 1 - smoothstep(hw - e, hw + e * 0.3, abs(deg - a))
        fz = smoothstep(zl - 0.004, zl + 0.004, z) * (1 - smoothstep(zh - 0.004, zh + 0.004, z))
        out = max(out, push * fx * fz)
    return out


# 3. malha: anel externo (grade densa por causa do relevo), anel interno, tampas em cima e embaixo
ROWS = 16
bm = bmesh.new()
uv = bm.loops.layers.uv.new('UVMap')
outer, inner = [], []
for i in range(N_ANG):
    th = 2 * math.pi * i / N_ANG
    d = Vector((math.sin(th), -math.cos(th), 0))
    r = radius[i] + OFFSET
    col_o, col_i = [], []
    for j in range(ROWS + 1):
        z = bot(th) + (top(th) - bot(th)) * j / ROWS
        col_o.append(bm.verts.new(Vector((0, cy, z)) + d * (r + bump(th, z))))
        if j in (0, ROWS):   # lado de dentro não aparece: só a linha de cima e a de baixo
            col_i.append(bm.verts.new(Vector((0, cy, z)) + d * (r - THICK)))
    outer.append(col_o)
    inner.append(col_i)

z_min = min(bot(2 * math.pi * i / N_ANG) for i in range(N_ANG)) - 0.01
z_max = max(top(2 * math.pi * i / N_ANG) for i in range(N_ANG)) + 0.01


def uv_of(i, z):
    return ((i / N_ANG + 0.5) % 1.0, (z - z_min) / (z_max - z_min))


INK_UV = (0.0005, 0.999)   # canto pintado de preto: tampas e lado de dentro
for i in range(N_ANG):
    a, b = i, (i + 1) % N_ANG
    for j in range(ROWS):
        f = bm.faces.new((outer[a][j], outer[b][j], outer[b][j + 1], outer[a][j + 1]))
        u0 = uv_of(i, 0)[0]
        for l, ii in zip(f.loops, (i, i + 1, i + 1, i)):
            l[uv].uv = (u0 + (ii - i) / N_ANG, uv_of(ii, l.vert.co.z)[1])   # costura atrás: u passa de 1 e repete

    f = bm.faces.new((inner[a][1], inner[b][1], inner[b][0], inner[a][0]))
    for l in f.loops:
        l[uv].uv = INK_UV
    for row in (0, ROWS):
        if row == 0:
            f = bm.faces.new((inner[a][0], inner[b][0], outer[b][0], outer[a][0]))
        else:
            f = bm.faces.new((outer[a][ROWS], outer[b][ROWS], inner[b][1], inner[a][1]))
        for l in f.loops:
            l[uv].uv = INK_UV
bm.normal_update()
mesh = bpy.data.meshes.new('belt_frog')
bm.to_mesh(mesh)
bm.free()
for p in mesh.polygons:
    p.use_smooth = True
belt = bpy.data.objects.new('belt_frog', mesh)
scene.collection.objects.link(belt)

# 4. textura desenhada fora do Blender (o Python do Blender não tem PIL): manda o perfil e as placas
params = {
    'tw': TW, 'th': TH, 'n_ang': N_ANG, 'z_min': z_min, 'z_max': z_max, 'gold': GOLD, 'ink': INK,
    'top': [top(2 * math.pi * (k / 4) / N_ANG) for k in range(N_ANG * 4 + 1)],
    'bot': [bot(2 * math.pi * (k / 4) / N_ANG) for k in range(N_ANG * 4 + 1)],
    'raised': RAISED, 'buckle': BUCKLE,
}
pj = tex_out[:-4] + '_params.json'
with open(pj, 'w') as fh:
    json.dump(params, fh)
here = os.path.dirname(os.path.abspath(__file__))
subprocess.run(['python', os.path.join(here, '..', 'belt_texture.py'), pj, tex_out], check=True)
os.remove(pj)

mat = bpy.data.materials.new('belt_frog')
mat.use_nodes = True
bsdf = mat.node_tree.nodes['Principled BSDF']
bsdf.inputs['Roughness'].default_value = 1.0
tn = mat.node_tree.nodes.new('ShaderNodeTexImage')
tn.image = bpy.data.images.load(tex_out)
mat.node_tree.links.new(tn.outputs['Color'], bsdf.inputs['Base Color'])
mesh.materials.append(mat)

# 5. pesos do corpo (vértice mais próximo) e preso no esqueleto
belt.parent = arm
mod = belt.modifiers.new('pesos', 'DATA_TRANSFER')
mod.object = body
mod.use_vert_data = True
mod.data_types_verts = {'VGROUP_WEIGHTS'}
mod.vert_mapping = 'POLYINTERP_NEAREST'
mod.layers_vgroup_select_src = 'ALL'
mod.layers_vgroup_select_dst = 'NAME'
bpy.context.view_layer.objects.active = belt
bpy.ops.object.datalayout_transfer(modifier='pesos')
bpy.ops.object.modifier_apply(modifier='pesos')
am = belt.modifiers.new('esqueleto', 'ARMATURE')
am.object = arm
print('Cinturão:', len(mesh.vertices), 'vértices; grupos', len(belt.vertex_groups))

bpy.ops.export_scene.gltf(filepath=out, export_format='GLB', export_extras=True)
print('Exportado:', out)
