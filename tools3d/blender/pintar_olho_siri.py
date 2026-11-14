"""Joana: apaga só a pupila do olho de siri (os texels escuros do globo viram o branco do próprio globo). O visualizador
desenha a pupila por cima (e troca por um X no K.O.). Imprime o centro e o raio da pupila (espaço do corpo, Y para cima
como no visualizador) para o CHARS.joana.crabEye.

  blender -b --factory-startup -P tools3d/blender/pintar_olho_siri.py -- corpo.glb saida.glb textura_saida.png
"""
import bpy, os, sys, json, subprocess
from mathutils import Vector

argv = sys.argv[sys.argv.index('--') + 1:]
src, out, tex_out = (os.path.abspath(a) for a in argv[:3])
XS, ZR = -0.215, 1.42

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=src)
arm = next(o for o in bpy.context.scene.objects if o.type == 'ARMATURE')
body = max((o for o in arm.children if o.type == 'MESH'), key=lambda o: len(o.data.vertices))
me, mw = body.data, body.matrix_world
img = next(n.image for m in me.materials if m and m.use_nodes for n in m.node_tree.nodes if n.type == 'TEX_IMAGE' and n.image)
W, H = img.size
px = img.pixels[:]
uvl = me.uv_layers.active.data
P = [mw @ v.co for v in me.vertices]
stalk = {i for i, q in enumerate(P) if q.x < XS and q.z > ZR}
zt = max(P[i].z for i in stalk)


def sample(u, v):
    k = (min(H - 1, max(0, int(v * H))) * W + min(W - 1, max(0, int(u * W)))) * 4
    return px[k:k + 3]


def orange(c):
    r, g, b = c
    return r > 0.55 and 0.15 < g < 0.65 and b < 0.3


tris, pts, light = [], [], []
for p in me.polygons:
    if not all(i in stalk for i in p.vertices):
        continue
    if min(P[i].z for i in p.vertices) < ZR + 0.55 * (zt - ZR):
        continue                                  # só o topo da haste (o globo)
    u = sum(uvl[l].uv.x for l in p.loop_indices) / p.loop_total
    v = sum(uvl[l].uv.y for l in p.loop_indices) / p.loop_total
    col = sample(u, v)
    if orange(col):
        continue
    if sum(col) > 1.8:                            # branco do globo: guarda a cor para pintar a pupila igual
        light.append(col); continue
    tris.append([[list(uvl[l].uv) for l in p.loop_indices], None])
    pts += [P[i] for i in p.vertices]
white = [int(255 * sorted(c[k] for c in light)[len(light) // 2]) for k in range(3)]
for t in tris:
    t[1] = white
c = sum(pts, Vector()) / len(pts)
r = max((q - c).length for q in pts)
front = min(q.y for q in pts)
print('Pupila do olho de siri: faces', len(tris), '| centro (Y para cima)', [round(c.x, 4), round(c.z, 4), round(-c.y, 4)],
      '| raio', round(r, 4), '| branco do globo', white)
pj, src_png = tex_out[:-4] + '_tris.json', tex_out[:-4] + '_src.png'
img.filepath_raw = src_png; img.file_format = 'PNG'; img.save()
json.dump({'src': src_png, 'tris': tris}, open(pj, 'w'))
here = os.path.dirname(os.path.abspath(__file__))
subprocess.run(['python', os.path.join(here, '..', 'pintar_uv.py'), pj, tex_out], check=True)
os.remove(pj); os.remove(src_png)
img2 = bpy.data.images.load(tex_out)
for m in me.materials:
    if m and m.use_nodes:
        for n in m.node_tree.nodes:
            if n.type == 'TEX_IMAGE':
                n.image = img2
bpy.ops.export_scene.gltf(filepath=out, export_format='GLB', export_extras=True, export_attributes=True)
print('Exportado:', out)
