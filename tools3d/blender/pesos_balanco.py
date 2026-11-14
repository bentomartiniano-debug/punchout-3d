"""Grava no corpo quanto cada vértice balança (atributos _SWAY_HAIR e _SWAY_COAT), pela cor da textura e pela altura.

  blender -b --factory-startup -P tools3d/blender/pesos_balanco.py -- corpo.glb saida.glb
          --cabelo "#e4d383,#b1a27b" --cabelo-z 1.12 1.40
          --roupa "#4d6e3f,#5b7a58" --roupa-z 0.78 0.30

Cabelo: 0 na raiz (cabelo-z inicial) até 1 nas pontas (cabelo-z final), só nos vértices com cor de cabelo.
Roupa solta (abas do casaco, capa): 0 na cintura (roupa-z inicial) até 1 na barra (roupa-z final).
O visualizador lê os atributos e aplica as molas (o cabelo atrasa em relação à cabeça; a aba em relação ao quadril).
Os vértices gêmeos de costura recebem o mesmo peso (senão a emenda abre quando balança).
"""
import bpy, os, sys

argv = sys.argv[sys.argv.index('--') + 1:]
src, out = os.path.abspath(argv[0]), os.path.abspath(argv[1])


def colors(flag):
    return [tuple(int(h.strip().lstrip('#')[i:i + 2], 16) for i in (0, 2, 4)) for h in argv[argv.index(flag) + 1].split(',')]


def pair(flag):
    i = argv.index(flag)
    return float(argv[i + 1]), float(argv[i + 2])


HAIR, COAT = colors('--cabelo'), colors('--roupa')
HZ, CZ = pair('--cabelo-z'), pair('--roupa-z')
FRENTE = float(argv[argv.index('--abertura') + 1]) if '--abertura' in argv else 0.0   # meia-largura da abertura da frente

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=src)
arm = next(o for o in bpy.context.scene.objects if o.type == 'ARMATURE')
body = max((o for o in arm.children if o.type == 'MESH'), key=lambda o: len(o.data.vertices))   # o corpo (não a fivela/cinto)
me, mw = body.data, body.matrix_world
img = next(n.image for m in me.materials if m and m.use_nodes for n in m.node_tree.nodes if n.type == 'TEX_IMAGE' and n.image)
W, H = img.size
px = img.pixels[:]
uvl = me.uv_layers.active.data


def sample(u, v):
    k = (min(H - 1, max(0, int(v * H))) * W + min(W - 1, max(0, int(u * W)))) * 4
    return [x * 255 for x in px[k:k + 3]]


def near(c, pal, tol=48):
    return min(sum((a - b) ** 2 for a, b in zip(c, p)) for p in pal) < tol ** 2


def ramp(z, a, b):
    t = (z - a) / (b - a)
    return max(0.0, min(1.0, t))


hair_v, coat_v = {}, {}
for poly in me.polygons:
    cs = [sample(*uvl[l].uv) for l in poly.loop_indices]
    is_hair = sum(near(c, HAIR) for c in cs) * 2 > len(cs)
    is_coat = sum(near(c, COAT) for c in cs) * 2 > len(cs)
    for vi in poly.vertices:
        z = (mw @ me.vertices[vi].co).z
        if is_hair:
            hair_v[vi] = max(hair_v.get(vi, 0), ramp(z, *HZ))
        pw = mw @ me.vertices[vi].co
        front_mid = pw.y < 0 and abs(pw.x) < FRENTE   # casaco aberto: na frente, no meio, é camisa/cinto/fivela (não balança)
        if is_coat and z > CZ[1] - 0.05 and abs(pw.x) < 0.3 and not front_mid:   # sem botas (mesma cor) e sem mangas
            coat_v[vi] = max(coat_v.get(vi, 0), ramp(z, *CZ))
# gêmeos de costura: mesmo peso
twins = {}
for v in me.vertices:
    twins.setdefault(v.co.to_tuple(5), []).append(v.index)
for d in (hair_v, coat_v):
    for group in twins.values():
        w = max(d.get(i, 0) for i in group)
        if w > 0:
            for i in group:
                d[i] = w
for name, d in (('_SWAY_HAIR', hair_v), ('_SWAY_COAT', coat_v)):
    a = me.attributes.new(name, 'FLOAT', 'POINT')
    a.data.foreach_set('value', [d.get(i, 0.0) for i in range(len(me.vertices))])
    print('Balanço', name, 'vértices com peso:', sum(1 for x in d.values() if x > 0))
bpy.ops.export_scene.gltf(filepath=out, export_format='GLB', export_extras=True, export_attributes=True)
print('Exportado:', out)
