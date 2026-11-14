"""Dreads da Joana feitos por código (cores tiradas da peruca do Meshy): cada mecha é um tubo afinando que nasce num
anel logo abaixo da aba do chapéu e cai até o ombro, sem entrar no corpo. Controle total de onde nasce cada mecha:
nada na frente do rosto, e do lado do siri só atrás do olho. O lado do gancho é mais volumoso.
Grava o atributo _SWAY_HAIR (0 na raiz, 1 na ponta): no visualizador só as pontas balançam, a raiz fica presa no chapéu.

  blender -b --factory-startup -P tools3d/blender/make_joana_dreads.py -- corpo.glb peruca_meshy.glb saida.glb
          [--n 60] [--grossura 0.036] [--volume 0.09] [--raiz 1.56] [--fim 0.0] [--rosto 58] [--siri 160] [--seed 7]
  Joana: --raiz 1.6 --volume 0.03 --rosto 60   (touca na cor dos dreads; as mechas sobem pela cabeça até --subida graus)
  --fim: altura das pontas relativa ao ombro (0 = na altura do ombro)
  --rosto: meia-abertura (graus) do vão do rosto; --siri: até que ângulo (graus, 180 = lado do siri) as mechas vão
"""
import bpy, bmesh, os, sys, math, random
from mathutils import Vector
from mathutils.bvhtree import BVHTree

argv = sys.argv[sys.argv.index('--') + 1:]
body_p, ref_p, out = (os.path.abspath(a) for a in argv[:3])
opt = dict(zip(argv[3::2], argv[4::2]))
N = int(opt.get('--n', 60))
SINK = float(opt.get('--afundar', -0.15))      # altura do centro da mecha sobre o couro cabeludo (x grossura)
KSCALP = 5                                   # pontos da mecha sobre o couro cabeludo (da touca até a raiz)
CAP = float(opt.get('--touca', 0.012))
THICK = float(opt.get('--grossura', 0.036))
PUFF = float(opt.get('--volume', 0.09))
ZROOT = float(opt.get('--raiz', 1.56))
ETOP = float(opt.get('--subida', 72))   # até que elevação (graus, 90 = topo) a mecha sobe pelo couro cabeludo
FACE = math.radians(float(opt.get('--rosto', 58)))
CRAB = math.radians(float(opt.get('--siri', 160)))
random.seed(int(opt.get('--seed', 7)))

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=body_p)
arm = next(o for o in bpy.context.scene.objects if o.type == 'ARMATURE')
body = max((o for o in arm.children if o.type == 'MESH'), key=lambda o: len(o.data.vertices))
dg = bpy.context.evaluated_depsgraph_get()
bm = bmesh.new(); bm.from_object(body, dg); bm.transform(body.matrix_world)
tree = BVHTree.FromBMesh(bm)
mw = body.matrix_world
g = body.vertex_groups['Head']
# cabeça sem a haste do olho de siri (ela sai pela lateral, x < -0.215, acima da bochecha)
head = [mw @ v.co for v in body.data.vertices for ge in v.groups if ge.group == g.index and ge.weight > 0.6]
head = [q for q in head if not (q.x < -0.215 and q.z > 1.42)]
band = [q for q in head if abs(q.z - ZROOT) < 0.025]
cx = (min(q.x for q in band) + max(q.x for q in band)) / 2
cy = (min(q.y for q in band) + max(q.y for q in band)) / 2
def head_r(phi):
    d = Vector((math.cos(phi), math.sin(phi), 0))
    return max(((q.x - cx) * d.x + (q.y - cy) * d.y) for q in band)
sh_z = (arm.matrix_world @ arm.data.bones['LeftArm'].head_local).z
z_end = sh_z + float(opt.get('--fim', 0.0))
print('Cabeça na raiz: centro', round(cx, 3), round(cy, 3), '| ombro', round(sh_z, 3))

# cores da peruca do Meshy: os dois marrons mais comuns da textura
names = {o.name for o in bpy.context.scene.objects}
bpy.ops.import_scene.gltf(filepath=ref_p)
ref = next(o for o in bpy.context.scene.objects if o.name not in names and o.type == 'MESH')
# a peruca limpa (postprocess --cores-por-face) guarda cada cor num material: usa esses tons (lineares), do escuro ao claro
tones = sorted({tuple(m.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value[:3]) for m in ref.data.materials if m and m.use_nodes}, key=sum)
print('Tons da peruca do Meshy:', [tuple(int(255 * x ** (1 / 2.2)) for x in c) for c in tones])
tones = [tuple(x * 0.55 for x in tones[0])] + tones          # + um tom mais escuro: mechas vizinhas se separam
GOLD = (0.62, 0.40, 0.06)
for o in [o for o in bpy.context.scene.objects if o.name not in names]:
    bpy.data.objects.remove(o, do_unlink=True)

# ângulos: phi = atan2(y, x); frente do corpo = -Y (phi -90). Lado do gancho = +X (phi 0), siri = -X (phi 180)
lo, hi = -math.pi / 2 + FACE, CRAB            # do canto do rosto (lado do gancho), pelas costas, até atrás do siri
def density(phi):                             # lado do gancho mais cheio
    return 1.6 if phi < math.radians(40) else 1.0
cands = [lo + (hi - lo) * i / 2000 for i in range(2001)]
acc, roots = 0.0, []
tot = sum(density(p) for p in cands)
step = tot / N
nxt = step / 2
for p in cands:
    acc += density(p)
    if acc >= nxt:
        roots.append(p); nxt += step

out_bm = bmesh.new()
sway = []
mats = []
SIDES, SEG = 7, 14
for k, phi0 in enumerate(roots):
    layer = k % 2                              # duas camadas desencontradas: sem frestas entre as mechas
    phi = phi0 + random.uniform(-0.04, 0.04)
    d = Vector((math.cos(phi), math.sin(phi), 0))
    back = max(0.0, math.sin(phi))             # costas = 1
    zr = ZROOT + 0.02 * back - 0.01 * layer
    r0 = head_r(phi) + 0.004 + 0.016 * layer      # rente ao couro cabeludo
    root = Vector((cx, cy, zr)) + d * r0
    hook = phi < math.radians(40)
    length = (zr - z_end) * (1.0 + (0.12 if hook else 0.0)) + random.uniform(-0.04, 0.04)
    thick = THICK * (1.1 if hook else 1.0) * random.uniform(0.85, 1.15)
    wave = random.uniform(0, 6.28)
    pts = []
    for i in range(SEG + 1):
        s = i / SEG
        bulge = PUFF * (1.2 if hook else 1.0) * s + 0.012 * s * layer          # cai rente (o corpo empurra para fora só onde precisa)
        side = Vector((-d.y, d.x, 0)) * (0.012 * math.sin(s * 4.5 + wave))
        p = root + d * bulge + side - Vector((0, 0, length * s))
        # fora do corpo: se entrou (ou encostou), sai pela normal da superfície
        loc, nrm, _, dist = tree.find_nearest(p)
        if loc is not None:
            inside = (p - loc).dot(nrm) < 0
            if inside or dist < thick + 0.012:
                p = loc + nrm * (thick + 0.012)
        pts.append(p)
    # a mecha nasce lá em cima, dentro da touca: desce colada no couro cabeludo até a raiz (emenda sem degrau)
    top = []
    hc = Vector((cx, cy, ZROOT - 0.06))              # centro aproximado da cabeça
    e0 = math.atan2(zr - hc.z, r0)
    e1 = math.radians(ETOP) - 0.12 * random.random()
    for i in range(KSCALP):
        e = e1 + (e0 - e1) * i / KSCALP              # do cocuruto descendo até a raiz
        dirv = (d * math.cos(e) + Vector((0, 0, math.sin(e)))).normalized()
        hit, nrm, _, _ = tree.ray_cast(hc + dirv * 0.5, -dirv)
        if hit is None:
            continue
        top.append(hit + nrm * (thick * SINK))        # afundada na touca: só o lombo da mecha aparece
    pts = top + pts
    NP = len(pts) - 1
    # anéis do tubo
    rings = []
    for i, p in enumerate(pts):
        s = max(0, i - KSCALP) / SEG
        t = (pts[min(i + 1, NP)] - pts[max(i - 1, 0)]).normalized()
        a = t.cross(Vector((0, 0, 1)))
        if a.length < 1e-4:
            a = Vector((1, 0, 0))
        a.normalize(); b = t.cross(a).normalized()
        rad = thick * (1.0 - 0.3 * s) * (0.75 + 0.25 * math.sin(min(1.0, s * 6) * math.pi / 2)) * (1 + 0.14 * math.sin(s * math.pi * 9 + wave))   # nós da corda
        ring = []
        for j in range(SIDES):
            ang = 2 * math.pi * j / SIDES
            ring.append(out_bm.verts.new(p + (a * math.cos(ang) + b * math.sin(ang)) * rad))
            sway.append(s ** 1.4)
        rings.append(ring)
    tip = out_bm.verts.new(pts[-1] - Vector((0, 0, thick * 0.6))); sway.append(1.0)
    mi = random.randrange(1, len(tones))       # mechas nos tons claros: aparecem sobre a touca escura
    for r0_, r1_ in zip(rings, rings[1:]):
        for j in range(SIDES):
            f = out_bm.faces.new((r0_[j], r0_[(j + 1) % SIDES], r1_[(j + 1) % SIDES], r1_[j])); f.material_index = mi; f.smooth = True
    for j in range(SIDES):
        f = out_bm.faces.new((rings[-1][j], rings[-1][(j + 1) % SIDES], tip)); f.material_index = mi; f.smooth = True
    f = out_bm.faces.new(list(reversed(rings[0]))); f.material_index = mi
    if random.random() < 0.3:                  # argola dourada perto da ponta
        i0 = int(SEG * random.uniform(0.6, 0.8))
        p0, p1 = pts[i0], pts[i0 + 1]
        t = (p1 - p0).normalized(); a = t.cross(Vector((0, 0, 1))).normalized(); b = t.cross(a).normalized()
        rr = thick * 1.05
        bands = [[out_bm.verts.new(c + (a * math.cos(2 * math.pi * j / SIDES) + b * math.sin(2 * math.pi * j / SIDES)) * rr) for j in range(SIDES)] for c in (p0, p0.lerp(p1, 0.6))]
        sway.extend([(i0 / SEG) ** 1.4] * SIDES + [((i0 + 0.6) / SEG) ** 1.4] * SIDES)
        for j in range(SIDES):
            out_bm.faces.new((bands[0][j], bands[0][(j + 1) % SIDES], bands[1][(j + 1) % SIDES], bands[1][j])).material_index = len(tones)
# touca: o topo da cabeça (da raiz das mechas para cima) copiado um pouco para fora, em gomos dos tons da peruca —
# sem o chapéu (K.O.) o cabelo sai de um couro cabeludo de dreads, não da pintura da cabeça
hv = {}
gi = g.index
for p in body.data.polygons:
    vs = p.vertices
    qs = [mw @ body.data.vertices[i].co for i in vs]
    if any(q.z < ZROOT - 0.012 or (q.x < -0.215 and q.z > 1.42) for q in qs):
        continue
    if any(max((ge.weight for ge in body.data.vertices[i].groups if ge.group == gi), default=0) < 0.5 for i in vs):
        continue
    nv = []
    for i, q in zip(vs, qs):
        if i not in hv:
            n = (mw.to_3x3() @ body.data.vertices[i].normal).normalized()
            hv[i] = out_bm.verts.new(q + n * CAP); sway.append(0.0)
        nv.append(hv[i])
    try:
        f = out_bm.faces.new(nv)
    except ValueError:
        continue
    f.material_index = 1                       # touca na cor dos dreads (tom da peruca do Meshy)
    f.smooth = True
print('Touca:', len(hv), 'vértices')
bmesh.ops.recalc_face_normals(out_bm, faces=out_bm.faces[:])
me = bpy.data.meshes.new('dreads_joana')
out_bm.to_mesh(me); out_bm.free()
att = me.attributes.new('_SWAY_HAIR', 'FLOAT', 'POINT')
for i, x in enumerate(sway):
    att.data[i].value = x
for c in tones + [GOLD]:
    m = bpy.data.materials.new('dreads_%02x%02x%02x' % tuple(int(255 * x ** (1 / 2.2)) for x in c)); m.use_nodes = True
    m.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value = (*c, 1)
    me.materials.append(m)
ob = bpy.data.objects.new('prop_dreads_codigo', me); bpy.context.scene.collection.objects.link(ob)
bpy.ops.object.select_all(action='DESELECT'); ob.select_set(True)
print('Dreads:', len(roots), 'mechas,', len(me.vertices), 'vértices | pontas perto de z', round(z_end, 3))
bpy.ops.export_scene.gltf(filepath=out, export_format='GLB', use_selection=True, export_attributes=True)
print('Exportado:', out)
