"""Limpa uma vista da folha do personagem para o Meshy: fundo liso, sem textos/setas/amostras de cor/sombra do chão;
fica só a figura (maior pedaço que não é cinza neutro claro). Opcional: alisa o rosto (tira pontilhados de guia).

  python tools3d/limpar_folha.py folha.png saida.png x0 y0 x1 y1 [--rosto fx0 fy0 fx1 fy1] [--lado 768]
"""
import sys
import numpy as np
from PIL import Image
from collections import deque

a = sys.argv
src, out = a[1], a[2]
box = tuple(int(v) for v in a[3:7])
im = np.asarray(Image.open(src).convert('RGB').crop(box)).astype(np.int16)
mx, mn = im.max(2), im.min(2)
bg = ((mx - mn) < 20) & (mx > 95)                    # cinza neutro claro: fundo, grade, sombra do chão
H, W = bg.shape


def flood(mask, seeds):
    """pixels de `mask` ligados (4-viz.) às sementes"""
    seen = np.zeros_like(mask, bool); q = deque()
    for y, x in seeds:
        if mask[y, x] and not seen[y, x]:
            seen[y, x] = True; q.append((y, x))
    while q:
        y, x = q.popleft()
        for yy, xx in ((y + 1, x), (y - 1, x), (y, x + 1), (y, x - 1)):
            if 0 <= yy < H and 0 <= xx < W and mask[yy, xx] and not seen[yy, xx]:
                seen[yy, xx] = True; q.append((yy, xx))
    return seen


# fundo = cinza claro ligado à borda (o branco das fivelas, dentro da figura, fica)
border = [(0, x) for x in range(W)] + [(H - 1, x) for x in range(W)] + [(y, 0) for y in range(H)] + [(y, W - 1) for y in range(H)]
outside = flood(bg, border)
# figura = maior pedaço do resto (textos, setas e amostras de cor são pedaços menores)
rest = ~outside
labels = np.zeros((H, W), np.int32); n = 0; best, best_n = 0, 0
for y in range(H):
    for x in range(W):
        if rest[y, x] and not labels[y, x]:
            n += 1; comp = flood(rest & (labels == 0), [(y, x)]); labels[comp] = n
            c = int(comp.sum())
            if c > best_n: best, best_n = n, c
fig = labels == best
BG = np.array([210, 210, 212], np.int16)
res = np.where(fig[..., None], im, BG)
if '--rosto' in a:
    i = a.index('--rosto'); fx0, fy0, fx1, fy1 = (int(v) for v in a[i + 1:i + 5])
    face = res[fy0:fy1, fx0:fx1]
    lum = face.sum(2)
    skin = np.median(face[(lum > 380) & (lum < 650)].reshape(-1, 3), axis=0)
    dots = (lum > 200) & (np.abs(lum - skin.sum()) > 6)   # tudo que não é o contorno preto vira pele lisa
    dots &= fig[fy0:fy1, fx0:fx1] & (face.max(2) - face.min(2) > 25)   # só pele da figura (o fundo cinza fica)
    face[dots] = skin
    print('rosto: pele', skin, '| pontos apagados', int(dots.sum()))
img = Image.fromarray(res.astype(np.uint8))
if '--lado' in a:
    s = int(a[a.index('--lado') + 1]); img = img.resize((s, s), Image.LANCZOS)
img.save(out)
print('limpo:', out, img.size, '| pedaços descartados', n - 1)
