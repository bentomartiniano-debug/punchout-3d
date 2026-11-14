"""Reduz a textura do Meshy às cores chapadas da folha de referência (o shader toon faz a luz).

  python tools3d/palette.py textura.png referencia.png saida.png [--cores 10] [--extra "#c9a23a,#6b4a2e"]

1. Extrai a paleta da referência (k-means, ignorando o fundo liso e o contorno preto).
2. Troca cada texel pela cor da paleta mais próxima em percepção (Lab simplificado).
3. Filtro de moda 3×3 para tirar pontinhos isolados nas bordas entre cores.
"""
import sys
from PIL import Image, ImageFilter


def lab(c):
    r, g, b = [x / 255 for x in c[:3]]
    return (0.3 * r + 0.59 * g + 0.11 * b, (r - g) * 0.8, (r + g) * 0.5 - b)  # aproximação barata, suficiente para cores chapadas


def palette_from(ref, k):
    """Paleta da referência sem perder cores pequenas (pés, fivela): muitas cores primeiro, depois junta as parecidas."""
    im = ref.convert('RGB')
    bg = im.getpixel((2, 2))
    px = [p for p in im.get_flattened_data() if sum(abs(a - b) for a, b in zip(p, bg)) > 30 and sum(p) > 90]
    sample = Image.new('RGB', (len(px), 1))
    sample.putdata(px)
    q = sample.quantize(colors=48, method=Image.Quantize.MAXCOVERAGE)
    pal = q.getpalette()
    counts = sorted(q.getcolors(), reverse=True)
    total = sum(c for c, _ in counts)
    out = []
    for c, i in counts:
        col = tuple(pal[i * 3:i * 3 + 3])
        if c / total < 0.002:
            continue  # ruído de anti-aliasing
        if all(sum((a - b) ** 2 for a, b in zip(col, o)) > 22 ** 2 for o in out):
            out.append(col)
        if len(out) >= k:
            break
    return out


def main():
    a = sys.argv[1:]
    src, ref, out = a[0], a[1], a[2]
    k = int(a[a.index('--cores') + 1]) if '--cores' in a else 10
    pal = palette_from(Image.open(ref), k)
    if '--extra' in a:   # cores pequenas que o filtro de ruído descarta (fivela dourada etc.)
        pal += [tuple(int(h.strip().lstrip('#')[i:i + 2], 16) for i in (0, 2, 4)) for h in a[a.index('--extra') + 1].split(',')]
    labs = [lab(c) for c in pal]
    tex = Image.open(src).convert('RGBA')
    cache = {}

    def near(c):
        if c not in cache:
            l = lab(c)
            cache[c] = min(range(len(pal)), key=lambda i: sum((x - y) ** 2 for x, y in zip(l, labs[i])))
        return cache[c]
    rgb = tex.convert('RGB').quantize(colors=256, method=Image.Quantize.MEDIANCUT)  # acelera: 256 cores antes do mapeamento
    lut = rgb.getpalette()
    mapping = [pal[near(tuple(lut[i * 3:i * 3 + 3]))] for i in range(256)]
    flat = Image.new('RGB', tex.size)
    flat.putdata([mapping[i] for i in rgb.getdata()])
    flat = flat.filter(ImageFilter.ModeFilter(3))
    flat.putalpha(tex.getchannel('A'))
    flat.save(out)
    print('Paleta:', ['#%02x%02x%02x' % c for c in pal])
    print('Textura chapada:', out)


if __name__ == '__main__':
    main()
