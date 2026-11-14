"""Marcov: repinta de marrom (pelo) o azul/cinza da camisa que vazou embaixo da bochecha. Só mexe na textura.

  blender -b --factory-startup -P tools3d/blender/pintar_pelo_pescoco.py -- corpo.glb saida.glb textura_saida.png
"""
import bpy, os, sys, json, subprocess

argv = sys.argv[sys.argv.index('--') + 1:]
src, out, tex_out = (os.path.abspath(a) for a in argv[:3])
FUR = (90, 65, 58)
SHIRT = [(61, 56, 53), (85, 83, 84), (96, 101, 105), (13, 51, 52), (36, 56, 79), (58, 123, 213)]

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=src)
arm = next(o for o in bpy.context.scene.objects if o.type == 'ARMATURE')
body = max((o for o in arm.children if o.type == 'MESH'), key=lambda o: len(o.data.vertices))
me, mw = body.data, body.matrix_world
img = next(n.image for m in me.materials if m and m.use_nodes for n in m.node_tree.nodes if n.type == 'TEX_IMAGE' and n.image)
W, H = img.size
px = img.pixels[:]
uvl = me.uv_layers.active.data
gidx = {g.index: g.name for g in body.vertex_groups}


def sample(u, v):
    k = (min(H - 1, max(0, int(v * H))) * W + min(W - 1, max(0, int(u * W)))) * 4
    return [x * 255 for x in px[k:k + 3]]


tris = []
for p in me.polygons:
    hw = min(sum(ge.weight for ge in me.vertices[vi].groups if gidx.get(ge.group) in ('Head', 'neck')) for vi in p.vertices)
    if hw < 0.5:
        continue                      # só a cabeça/pescoço: a camisa de verdade fica como está
    u = sum(uvl[l].uv.x for l in p.loop_indices) / p.loop_total
    v = sum(uvl[l].uv.y for l in p.loop_indices) / p.loop_total
    cs = [sample(*uvl[l].uv) for l in p.loop_indices] + [sample(u, v)]
    shirtish = sum(min(sum((a - b) ** 2 for a, b in zip(c, s)) for s in SHIRT) < 40 ** 2 for c in cs)
    if shirtish * 2 >= len(cs):
        tris.append([[list(uvl[l].uv) for l in p.loop_indices], FUR])
print('Faces do pescoço repintadas de pelo:', len(tris))
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
