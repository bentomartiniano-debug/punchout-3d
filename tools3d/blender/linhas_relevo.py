"""Pinta de preto as quinas do relevo (contorno de quadrinho) direto na textura, só nas áreas de uma cor.

  blender -b --factory-startup -P tools3d/blender/linhas_relevo.py -- corpo.glb textura.png saida.png
          [--cor "#d9af4d"] [--raio 0.012] [--limiar 0.08]

Assa no Cycles uma máscara de quina (normal do Bevel x normal da superfície) no próprio UV da malha;
depois, onde a máscara passa do limiar e a textura é da cor pedida (ex.: o dourado do cinturão), vira preto.
"""
import bpy, os, sys
import numpy as np

argv = sys.argv[sys.argv.index('--') + 1:]
src, tex_path, out = (os.path.abspath(a) for a in argv[:3])
opt = dict(zip(argv[3::2], argv[4::2]))
hexc = opt.get('--cor', '#d9af4d').lstrip('#')
cor = np.array([int(hexc[i:i + 2], 16) for i in (0, 2, 4)], float)
raio = float(opt.get('--raio', 0.012))
limiar = float(opt.get('--limiar', 0.08))

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=src)
scene = bpy.context.scene
obj = next(o for o in scene.objects if o.type == 'MESH')
tex = bpy.data.images.load(tex_path)
W, H = tex.size

mask = bpy.data.images.new('quinas', W, H, float_buffer=True)
mat = bpy.data.materials.new('assar')
mat.use_nodes = True
nt = mat.node_tree
nt.nodes.clear()
bev = nt.nodes.new('ShaderNodeBevel'); bev.samples = 16; bev.inputs['Radius'].default_value = raio
geo = nt.nodes.new('ShaderNodeNewGeometry')
dot = nt.nodes.new('ShaderNodeVectorMath'); dot.operation = 'DOT_PRODUCT'
inv = nt.nodes.new('ShaderNodeMath'); inv.operation = 'SUBTRACT'; inv.inputs[0].default_value = 1.0
emi = nt.nodes.new('ShaderNodeEmission')
outn = nt.nodes.new('ShaderNodeOutputMaterial')
img = nt.nodes.new('ShaderNodeTexImage'); img.image = mask
nt.links.new(bev.outputs['Normal'], dot.inputs[0])
nt.links.new(geo.outputs[opt.get('--normal', 'Normal')], dot.inputs[1])   # 'True Normal' pega quinas mesmo com sombreado suave
nt.links.new(dot.outputs['Value'], inv.inputs[1])
nt.links.new(inv.outputs['Value'], emi.inputs['Color'])
nt.links.new(emi.outputs['Emission'], outn.inputs['Surface'])
obj.data.materials.clear()
obj.data.materials.append(mat)
nt.nodes.active = img

cache = opt.get('--mascara')   # .npy: reaproveita a máscara assada (o bake leva uns 3 minutos)
if cache and os.path.exists(cache):
    m = np.load(cache)
else:
    scene.render.engine = 'CYCLES'
    scene.cycles.samples = 16
    scene.cycles.device = 'CPU'
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.ops.object.bake(type='EMIT', margin=4)
    m = np.array(mask.pixels[:], np.float32).reshape(H, W, 4)[:, :, 0]
    if cache:
        np.save(cache, m)
t = np.array(tex.pixels[:], np.float32).reshape(H, W, 4)
rgb = t[:, :, :3] * 255
area = np.linalg.norm(rgb - cor, axis=2) < 40
print('Máscara na cor: percentis 50/90/97/99 =', np.percentile(m[area], [50, 90, 97, 99]).round(4))
linha = (m > limiar) & area
# sombra pintada pelo Meshy encostada na cor (ex.: laranja na beira do cinturão) vira traço preto fino
if '--vizinha' in opt:
    hv = opt['--vizinha'].lstrip('#')
    viz = np.linalg.norm(rgb - np.array([int(hv[i:i + 2], 16) for i in (0, 2, 4)], float), axis=2) < 30
    perto = area.copy()
    for _ in range(int(opt.get('--largura', 4))):
        perto = perto | np.roll(perto, 1, 0) | np.roll(perto, -1, 0) | np.roll(perto, 1, 1) | np.roll(perto, -1, 1)
    linha |= viz & perto
t[linha, 0], t[linha, 1], t[linha, 2] = 32 / 255, 30 / 255, 31 / 255
res = bpy.data.images.new('saida', W, H, alpha=True)
res.pixels = t.ravel()
res.filepath_raw = out
res.file_format = 'PNG'
res.save()
print('Quinas pintadas:', int(linha.sum()), 'texels de', int(area.sum()), 'da cor', '#' + hexc, '->', out)
