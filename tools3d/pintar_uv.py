"""Pinta triângulos/polígonos de UV numa textura (chamado pelos scripts do Blender, que não têm PIL).

  python tools3d/pintar_uv.py dados.json saida.png      dados = {"src": png, "tris": [[[[u,v],...], [r,g,b]], ...]}
Cada polígono é preenchido com folga de 3 px (cobre a borda da ilha sem deixar fio da cor antiga).
"""
import json, sys
from PIL import Image, ImageDraw

D = json.load(open(sys.argv[1]))
im = Image.open(D['src']).convert('RGBA')
W, H = im.size
dr = ImageDraw.Draw(im)
for pts, col in D['tris']:
    poly = [(u * W, (1 - v) * H) for u, v in pts]
    dr.polygon(poly, fill=tuple(col) + (255,), outline=tuple(col) + (255,), width=3)
im.save(sys.argv[2])
print('UV pintado:', len(D['tris']), 'polígonos ->', sys.argv[2])
