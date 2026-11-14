"""Desenha a textura do cinturão (chamado pelo blender/make_frog_belt.py): dourado chapado com contorno preto.

  python tools3d/belt_texture.py params.json saida.png
u = volta inteira (frente no meio), v = altura absoluta entre z_min e z_max.
"""
import json, math, sys
from PIL import Image, ImageDraw

P = json.load(open(sys.argv[1]))
TW, TH, N = P['tw'], P['th'], P['n_ang']
INK, GOLD = tuple(P['ink']), tuple(P['gold'])
LW = max(3, int(TH * 0.026))
im = Image.new('RGB', (TW, TH), GOLD)
dr = ImageDraw.Draw(im)


def to_px(i, z):
    u = (i / N + 0.5) % 1.0
    v = (z - P['z_min']) / (P['z_max'] - P['z_min'])
    return u * TW, (1 - v) * TH


# fora da faixa = tinta (a borda da malha amostra preto, nunca dourado sobrando) e bordas de cima/baixo
for key, sign in (('top', -1), ('bot', 1)):
    pts = [to_px(k / 4, z) for k, z in enumerate(P[key])]
    pts.sort()
    pts = [(x - TW, y) for x, y in pts[-8:]] + pts + [(x + TW, y) for x, y in pts[:8]]   # costura atrás sem pontinha
    poly = pts + ([(TW, 0), (0, 0)] if key == 'top' else [(TW, TH), (0, TH)])
    dr.polygon(poly, fill=INK)
    # a linha de borda entra um pouco no dourado
    dr.line([(x, y - sign * LW * 0.5) for x, y in pts], fill=INK, width=LW, joint='curve')


def rect_px(a, hw, zl, zh):
    x0, y1 = to_px(math.radians(a - hw) / (2 * math.pi) * N, zl)
    x1, y0 = to_px(math.radians(a + hw) / (2 * math.pi) * N, zh)
    return x0, y0, x1, y1


for a, hw, zl, zh, _ in P['raised']:
    dr.rounded_rectangle(rect_px(a, hw, zl, zh), radius=LW * 2, outline=INK, width=LW)
# "C" da fivela: retângulo interno com entalhe à direita, como na folha
a, hw, zl, zh, _ = P['buckle']
x0, y0, x1, y1 = rect_px(a, hw, zl, zh)
w, h = x1 - x0, y1 - y0
ix0, iy0, ix1, iy1 = x0 + w * 0.2, y0 + h * 0.2, x1 - w * 0.2, y1 - h * 0.2
nx = ix1 - (ix1 - ix0) * 0.22
ny0, ny1 = iy0 + (iy1 - iy0) * 0.36, iy1 - (iy1 - iy0) * 0.36
dr.line([(ix0, iy0), (ix1, iy0), (ix1, ny0), (nx, ny0), (nx, ny1), (ix1, ny1), (ix1, iy1), (ix0, iy1), (ix0, iy0)],
        fill=INK, width=LW, joint='curve')
dr.rectangle((0, 0, 10, 10), fill=INK)   # canto de tinta: tampas e lado de dentro
im.save(sys.argv[2])
print('Textura do cinturão:', sys.argv[2])
