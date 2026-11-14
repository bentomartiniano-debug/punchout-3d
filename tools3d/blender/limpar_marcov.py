"""Limpeza do corpo do Dr. Marcov: separa as roupas por material chapado e prepara o balanço do jaleco.

  blender -b --factory-startup -P tools3d/blender/limpar_marcov.py -- corpo.glb saida.glb

1. Classifica cada face pela cor da textura + posição + osso:
   - jaleco (brancos/cinzas claros, fora da cabeça)  -> material branco chapado
   - camisa (escuros no tronco e nos antebraços)     -> material azul-petróleo chapado (sombra só do shader)
   - calça (escuros entre a cintura e as botas)      -> material grafite chapado
   - pelo da cabeça/pescoço (marrom e o cinza/azul que vazou da camisa no pescoço) -> marrom chapado
   - pele do rosto, mãos e as botas continuam com a textura
2. Some com o reator do peito: as faces ciano/teal do peito viram camisa e o relevo é achatado até a camisa em volta.
3. Grava _SWAY_COAT só no jaleco (preso nos ombros, solto na barra) e _SWAY_HAIR no topete.
"""
import bpy, bmesh, os, sys, math
from mathutils import Vector

argv = sys.argv[sys.argv.index('--') + 1:]
src, out = os.path.abspath(argv[0]), os.path.abspath(argv[1])
NECK_Z, WAIST_Z, BOOT_Z, HEM_Z, SHOULDER_Z = 1.38, 0.86, 0.36, 0.62, 1.30

WHITE = [(226, 238, 238), (254, 255, 255), (190, 198, 201), (167, 177, 178), (156, 148, 146)]
SKIN = [(219, 158, 137), (187, 152, 132)]
FUR = [(90, 65, 58)]
DARK = [(61, 56, 53), (85, 83, 84), (96, 101, 105), (13, 51, 52)]
CYAN = [(46, 230, 230), (58, 123, 213)]

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


def near(c, pal, tol=40):
    return min(sum((a - b) ** 2 for a, b in zip(c, p)) for p in pal) < tol ** 2


def classify(cs):
    votes = {'white': 0, 'skin': 0, 'fur': 0, 'dark': 0, 'cyan': 0}
    for c in cs:
        best = min(((k, min(sum((a - b) ** 2 for a, b in zip(c, p)) for p in pal)) for k, pal in
                    (('white', WHITE), ('skin', SKIN), ('fur', FUR), ('dark', DARK), ('cyan', CYAN))), key=lambda t: t[1])
        votes[best[0]] += 1
    return max(votes, key=votes.get)


def head_w(poly):
    return min(sum(ge.weight for ge in me.vertices[vi].groups if gidx.get(ge.group) in ('Head', 'neck', 'head_end', 'headfront')) for vi in poly.vertices)


def arm_w(poly):
    return min(sum(ge.weight for ge in me.vertices[vi].groups if 'Arm' in gidx.get(ge.group, '') or 'Hand' in gidx.get(ge.group, '')) for vi in poly.vertices)


def flat(name, rgb):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    b = m.node_tree.nodes['Principled BSDF']
    b.inputs['Base Color'].default_value = (*[(c / 255) ** 2.2 for c in rgb], 1)
    b.inputs['Roughness'].default_value = 1.0
    me.materials.append(m)
    return len(me.materials) - 1


M_COAT, M_SHIRT, M_PANTS, M_FUR = flat('marcov_jaleco', (244, 247, 247)), flat('marcov_camisa', (36, 56, 79)), flat('marcov_calca', (75, 74, 80)), flat('marcov_pelo', (90, 65, 58))
counts = {}
reactor = []
for p in me.polygons:
    cs = [sample(*uvl[l].uv) for l in p.loop_indices]
    u = sum(uvl[l].uv.x for l in p.loop_indices) / p.loop_total
    v = sum(uvl[l].uv.y for l in p.loop_indices) / p.loop_total
    cs.append(sample(u, v))
    k = classify(cs)
    c = mw @ p.center
    hw = head_w(p)
    new = None
    if c.z < BOOT_Z:
        new = None                                            # botas: textura (luzes ciano)
    elif hw > 0.5 or c.z > NECK_Z:
        if k in ('dark', 'cyan', 'white', 'fur') or (k == 'skin' and c.z < 1.40 and c.y > -0.15):   # pescoço: pelo (sem o azul da camisa)
            new = M_FUR if k != 'white' or c.z > NECK_Z + 0.05 else M_COAT   # gola do jaleco fica branca
        # pele do rosto/orelhas fica com a textura
    elif k == 'white':
        new = M_COAT
    else:   # tronco/pernas: nada fica com textura (some a sujeira); o que não é jaleco é camisa ou calça
        chest = c.y < 0 and abs(c.x) < 0.13 and 1.0 < c.z < 1.32
        if k == 'cyan' or (chest and k == 'dark' and near(cs[-1], [(13, 51, 52)], 45)):
            reactor.append(p.index)
        new = M_PANTS if (c.z < WAIST_Z and arm_w(p) < 0.5) else M_SHIRT
    if new is not None:
        p.material_index = new
        counts[new] = counts.get(new, 0) + 1
print('Faces por material novo:', {me.materials[k].name: v for k, v in counts.items()}, '| reator:', len(reactor))

# reator: achata o relevo até a camisa em volta (gêmeos de costura juntos)
if reactor:
    rv = {vi for i in reactor for vi in me.polygons[i].vertices}
    rc = sum((mw @ me.vertices[vi].co for vi in rv), Vector()) / len(rv)
    # a moldura do reator (mesma cor da camisa) também sai: tudo na frente do peito perto do centro
    R_IN = 0.085
    BOX = (0.11, 1.03, 1.32)   # caixa do peito onde fica o reator (meia-largura, z de baixo, z de cima)
    inbox = lambda q: abs(q.x) < BOX[0] and BOX[1] < q.z < BOX[2] and q.y < 0
    rv |= {v.index for v in me.vertices if inbox(mw @ v.co) and not any(gidx.get(g.group, '').endswith(('Arm', 'Shoulder')) and g.weight > 0.3 for g in v.groups)}
    rv = {vi for vi in rv if me.polygons and True}
    for p in me.polygons:
        if all(vi in rv for vi in p.vertices):
            p.material_index = M_SHIRT
    def ring_ok(q): return q.y < 0 and not inbox(q) and abs(q.x) < BOX[0] + 0.04 and BOX[1] - 0.05 < q.z < BOX[2] + 0.05
    ring = [mw @ v.co for v in me.vertices if v.index not in rv and ring_ok(mw @ v.co)]
    # só a camisa (não o jaleco, que fica mais à frente): os mais de trás do anel
    ys = sorted(q.y for q in ring); ring = [q for q in ring if q.y > ys[len(ys) // 3]] if ys else ring
    # plano da camisa em volta (y = a + b·x + c·z) e TODO o reator vai para esse plano: vira peito liso
    import numpy as np
    A = np.array([[1, q.x, q.z] for q in ring]); Y = np.array([q.y for q in ring])
    coef = np.linalg.lstsq(A, Y, rcond=None)[0]
    inv = mw.inverted()
    for vi in rv:
        q = mw @ me.vertices[vi].co
        q.y = float(coef[0] + coef[1] * q.x + coef[2] * q.z)
        me.vertices[vi].co = inv @ q
    print('Reator achatado no plano do peito:', len(rv), 'vértices')

# beira do jaleco: faces do jaleco encostadas na camisa/calça viram traço preto (a gola/lapela fica desenhada)
M_INK = flat('marcov_traco', (20, 18, 22))
edge_faces = {}
for p in me.polygons:
    for ek in p.edge_keys:
        edge_faces.setdefault(ek, []).append(p.index)
ink = set()
for p in me.polygons:
    if p.material_index == M_COAT and (mw @ p.center).y < 0.02:
        nb = [me.polygons[f].material_index for ek in p.edge_keys for f in edge_faces[ek] if f != p.index]
        if any(m in (M_SHIRT, M_PANTS) for m in nb):
            ink.add(p.index)
for _ in range(2):   # a borda fina fica escondida na quina: alarga 2 fileiras para a frente do jaleco
    grow = set()
    for i in ink:
        for ek in me.polygons[i].edge_keys:
            for f in edge_faces[ek]:
                if f not in ink and me.polygons[f].material_index == M_COAT and (mw @ me.polygons[f].center).y < 0.02:
                    grow.add(f)
    ink |= grow
ink = set()   # (desligado: o traço por faces saía serrilhado; a beira fica com o contorno de profundidade)
for i in ink:
    me.polygons[i].material_index = M_INK
print('Traço na beira do jaleco:', len(ink), 'faces')

# saia do jaleco: presa só no quadril (as coxas passam por dentro sem arrastar o pano) e um pouco mais larga
coat_v = {vi for p in me.polygons if p.material_index in (M_COAT, M_INK) for vi in p.vertices}
hips_g = body.vertex_groups.get('Hips')
legs = [g for g in body.vertex_groups if 'UpLeg' in g.name or g.name.endswith('Leg')]
inv2 = mw.inverted()
for vi in coat_v:
    v = me.vertices[vi]; q = mw @ v.co
    if q.z < WAIST_Z + 0.05:
        t = max(0.0, min(1.0, (WAIST_Z + 0.05 - q.z) / 0.2))
        for g in legs:
            g.remove([vi])
        hips_g.add([vi], 1.0, 'REPLACE')
        r = Vector((q.x, q.y + 0.0, 0)); 
        if r.length > 1e-6:
            q += r.normalized() * 0.025 * t   # afasta das pernas
            v.co = inv2 @ q

# peito/ombros: nada de pelo abaixo do pescoço na frente, e o que sobrou com textura (caneta do bolso) vira jaleco
fixed = 0
for p in me.polygons:
    c = mw @ p.center
    if BOOT_Z < c.z < 1.36:
        if p.material_index == M_FUR:
            p.material_index = (M_FUR if c.z > 1.28 else M_SHIRT) if abs(c.x) < 0.16 else M_COAT; fixed += 1
        elif p.material_index == 0:   # embaixo do queixo é pelo; mais abaixo, a caneta do bolso vira jaleco
            p.material_index = M_FUR if c.z > 1.24 and abs(c.x) < 0.16 else M_COAT; fixed += 1
print('Pelo/textura fora do lugar no tronco:', fixed)

# limpa ilhas soltas (pontinhos de pelo na camisa, de camisa no pelo): maioria das vizinhas, 3 passadas
ef = {}
for p in me.polygons:
    for ek in p.edge_keys:
        ef.setdefault(ek, []).append(p.index)
for _ in range(3):
    ch = {}
    for p in me.polygons:
        if p.material_index not in (M_SHIRT, M_FUR, M_PANTS, M_COAT, 0):
            continue
        nb = [me.polygons[f].material_index for ek in p.edge_keys for f in ef[ek] if f != p.index]
        if len(nb) >= 2:
            top = max(set(nb), key=nb.count)
            if top != p.material_index and nb.count(top) * 2 > len(nb) and top in (M_SHIRT, M_FUR, M_PANTS, M_COAT):
                ch[p.index] = top
    for i, m in ch.items():
        me.polygons[i].material_index = m
    print('Ilhas limpas:', len(ch))

# camisa: alisa as rugas do Meshy e recua 2 cm atrás do jaleco -> o contorno de profundidade desenha a lapela
shirt_pos = {me.vertices[vi].co.to_tuple(5) for p in me.polygons if p.material_index == M_SHIRT and (mw @ p.center).y < 0 for vi in p.vertices}
coat_pos = {me.vertices[vi].co.to_tuple(5) for p in me.polygons if p.material_index in (M_COAT, M_INK) for vi in p.vertices}
bm = bmesh.new(); bm.from_mesh(me)
okeys = [v.co.to_tuple(5) for v in bm.verts]
bmesh.ops.remove_doubles(bm, verts=bm.verts[:], dist=1e-5)
bm.verts.ensure_lookup_table()
sel = [v for v in bm.verts if v.co.to_tuple(5) in shirt_pos and v.co.to_tuple(5) not in coat_pos]
selset = set(sel)
inner = [v for v in sel if all(e.other_vert(v) in selset for e in v.link_edges)]
for _ in range(45):
    bmesh.ops.smooth_vert(bm, verts=inner, factor=0.6, use_axis_x=True, use_axis_y=True, use_axis_z=True)
inv3 = mw.inverted(); back = (inv3.to_3x3() @ Vector((0, 0.02, 0)))
# (sem recuar: abria furos na emenda com o pelo e a calça; a lapela agora sai pelo contorno de IDs)
moved = {}
bm2 = bmesh.new(); bm2.from_mesh(me); bmesh.ops.remove_doubles(bm2, verts=bm2.verts[:], dist=1e-5); bm2.verts.ensure_lookup_table()
for v2, v in zip(bm2.verts, bm.verts):
    moved[v2.co.to_tuple(5)] = v.co.copy()
bm.free(); bm2.free()
selkeys = {k for k in shirt_pos if k not in coat_pos}
for v in me.vertices:
    k = v.co.to_tuple(5)
    if k in selkeys and k in moved:
        v.co = moved[k]
print('Camisa alisada e recuada:', len(sel), 'vértices')

# balanço: só o jaleco (preso nos ombros, solto na barra) e o topete
coat_w = [0.0] * len(me.vertices)
hair_w = [0.0] * len(me.vertices)
for p in me.polygons:
    for vi in p.vertices:
        c = mw @ me.vertices[vi].co
        if p.material_index in (M_COAT, M_INK) and arm_w(p) < 0.5 and c.z < SHOULDER_Z:
            coat_w[vi] = max(coat_w[vi], max(0.0, min(1.0, (SHOULDER_Z - c.z) / (SHOULDER_Z - HEM_Z))) ** 1.3)
        if p.material_index == M_FUR and c.z > 1.62:
            hair_w[vi] = max(hair_w[vi], min(1.0, (c.z - 1.62) / 0.16))
twins = {}
for v in me.vertices:
    twins.setdefault(v.co.to_tuple(5), []).append(v.index)
for arr in (coat_w, hair_w):
    for g in twins.values():
        w = max(arr[i] for i in g)
        for i in g:
            arr[i] = w
for name, arr in (('_SWAY_COAT', coat_w), ('_SWAY_HAIR', hair_w)):
    old = me.attributes.get(name)
    if old:
        me.attributes.remove(old)
    a = me.attributes.new(name, 'FLOAT', 'POINT')
    a.data.foreach_set('value', arr)
    print('Balanço', name, ':', sum(1 for x in arr if x > 0), 'vértices')
# geometria mudou: refaz as normais suaves (as antigas ficavam tortas no peito)
bmn = bmesh.new(); bmn.from_mesh(me)
key = [v.co.to_tuple(5) for v in bmn.verts]
bmesh.ops.remove_doubles(bmn, verts=bmn.verts[:], dist=1e-5)
for _ in range(25):
    bmesh.ops.smooth_vert(bmn, verts=bmn.verts[:], factor=0.5, use_axis_x=True, use_axis_y=True, use_axis_z=True)
bmo = bmesh.new(); bmo.from_mesh(me); bmesh.ops.remove_doubles(bmo, verts=bmo.verts[:], dist=1e-5)
bmo.verts.ensure_lookup_table(); bmn.verts.ensure_lookup_table(); bmn.normal_update()
by = {vo.co.to_tuple(5): bmn.verts[vo.index].normal.copy() for vo in bmo.verts}
nrm = [(by.get(k) or me.vertices[i].normal).normalized() for i, k in enumerate(key)]
if reactor:   # peito achatado: camadas sobrepostas do reator viram uma superfície só, com a normal do plano
    pn = (mw.inverted().to_3x3() @ Vector((float(coef[1]), -1.0, float(coef[2])))).normalized()
    for vi in rv:
        nrm[vi] = pn
me.normals_split_custom_set_from_vertices(nrm)
bmn.free(); bmo.free()
bpy.ops.export_scene.gltf(filepath=out, export_format='GLB', export_extras=True, export_attributes=True)
print('Exportado:', out)
