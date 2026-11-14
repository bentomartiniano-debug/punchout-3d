"""Pós-processamento de um asset do Meshy (roda no Blender sem janela).

  blender -b --factory-startup -P tools3d/blender/postprocess.py -- entrada.glb saida.glb [--textura chapada.png]
          [--tipo corpo|prop] [--altura 2.15] [--espelhar]

Corpo: remove sobras que não pertencem ao esqueleto (o rig do Meshy exporta uma esfera solta), aplica a escala
do esqueleto (vem em centímetros), troca a textura pela versão chapada e cria os soquetes do plano
(head_top, head_face, hand_L_grip, hand_R_grip, spine_back, hips_front) como empties presos nos ossos.
Prop: escala para a altura pedida, base na origem, troca a textura; --espelhar gera a mão oposta.
"""
import bpy, bmesh, os, sys, math
from mathutils import Vector, Matrix

argv = sys.argv[sys.argv.index('--') + 1:]
src, out = os.path.abspath(argv[0]), os.path.abspath(argv[1])
opt = {}
i = 2
while i < len(argv):
    if argv[i] in ('--alinhar-eixo', '--centro-base', '--virar', '--encher', '--normais-fora', '--interior-escuro', '--rosto-nitido', '--fechar-boca', '--espelhar', '--aplainar-olhos', '--arredondar-cabeca', '--decepar-maos'):
        opt[argv[i].lstrip('-')] = True
        i += 1
    else:
        opt[argv[i]] = argv[i + 1]
        i += 2
kind = opt.get('--tipo', 'corpo')


def smooth_region(obj, verts_idx, iterations, factor=0.5):
    """Alisa só os vértices escolhidos; a borda da região fica presa para não abrir degrau."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bm.verts.ensure_lookup_table()
    keep = {v.index for v in bm.verts if v.index in set(verts_idx)}
    coords = {v.co.to_tuple(5) for v in bm.verts if v.index in keep}
    bmesh.ops.remove_doubles(bm, verts=bm.verts[:], dist=1e-5)   # costuras do glTF: solda antes de alisar (UV fica nos cantos das faces)
    bm.verts.ensure_lookup_table()
    sel = {v.index for v in bm.verts if v.co.to_tuple(5) in coords}
    inner = [bm.verts[i] for i in sel if all(e.other_vert(bm.verts[i]).index in sel for e in bm.verts[i].link_edges)]
    for _ in range(iterations):
        bmesh.ops.smooth_vert(bm, verts=inner, factor=factor, use_axis_x=True, use_axis_y=True, use_axis_z=True)
    bm.to_mesh(obj.data)
    bm.free()
    obj.data.update()

def group_weights(obj, name):
    g = obj.vertex_groups.get(name)
    if not g:
        return {}
    return {v.index: ge.weight for v in obj.data.vertices for ge in v.groups if ge.group == g.index}


def smoothstep(a, b, x):
    t = max(0.0, min(1.0, (x - a) / (b - a)))
    return t * t * (3 - 2 * t)


def round_head(obj, expo=2.6):
    """Puxa a metade de trás do crânio para um superelipsoide do tamanho da cabeça (some o degrau da nuca).
    A frente (focinho, boca) fica como o Meshy fez; o pescoço vai soltando aos poucos."""
    w = group_weights(obj, 'Head')
    mw = obj.matrix_world
    core = [mw @ obj.data.vertices[i].co for i, x in w.items() if x >= 0.5]
    if not core:
        return
    lo = Vector([min(p[k] for p in core) for k in range(3)])
    hi = Vector([max(p[k] for p in core) for k in range(3)])
    rx = max(abs(lo.x), abs(hi.x))
    top = hi.z
    rz = (top - lo.z) * 0.62
    c = Vector((0, (lo.y + hi.y) / 2, top - rz))
    r = Vector((rx, (hi.y - lo.y) / 2, rz))
    inv = mw.inverted()
    moved = []
    for i, hw in w.items():
        v = obj.data.vertices[i]
        p = mw @ v.co
        d = p - c
        f = smoothstep(0.15, 0.6, hw) * smoothstep(c.y - r.y * 0.15, c.y + r.y * 0.35, p.y) * smoothstep(c.z - r.z * 0.9, c.z - r.z * 0.2, p.z)
        if f <= 0:
            continue
        s = (abs(d.x / r.x) ** expo + abs(d.y / r.y) ** expo + abs(d.z / r.z) ** expo) ** (1 / expo)
        if s < 1e-6:
            continue
        v.co = inv @ p.lerp(c + d / s, f)
        moved.append(i)
    smooth_region(obj, moved, 8, 0.4)
    print('Cabeça arredondada:', obj.name, len(moved), 'vértices; centro', tuple(round(x, 3) for x in c), 'raios', tuple(round(x, 3) for x in r))


def cut_hand(obj, side, thin=0.0):
    """Corta a mão num plano perpendicular ao antebraço, logo depois do pulso, e tampa o buraco."""
    bones = arm.data.bones
    hand, fore = bones[side + 'Hand'], bones[side + 'ForeArm']
    wrist = arm.matrix_world @ hand.head_local
    elbow = arm.matrix_world @ fore.head_local
    n = (wrist - elbow).normalized()
    inv = obj.matrix_world.inverted()
    p_local = inv @ (wrist - n * float(opt.get('--recuo-pulso', 0.04)))   # o osso da mão começa já na base do polegar: corta um pouco antes, onde o pulso é fino
    n_local = (inv.to_3x3() @ n).normalized()
    arm_w = {}
    for g in (side + 'Hand', side + 'ForeArm'):
        for i, x in group_weights(obj, g).items():
            arm_w[i] = arm_w.get(i, 0) + x
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bm.verts.ensure_lookup_table()
    region = {bm.verts[i] for i, x in arm_w.items() if x >= 0.5}
    bmesh.ops.remove_doubles(bm, verts=bm.verts[:], dist=1e-5)
    region = {v for v in region if v.is_valid}
    faces = [f for f in bm.faces if all(v in region for v in f.verts)]
    geom = list({v for f in faces for v in f.verts}) + list({e for f in faces for e in f.edges}) + faces
    bmesh.ops.bisect_plane(bm, geom=geom, plane_co=p_local, plane_no=n_local, clear_outer=True)
    # some o que ficou solto do lado da mão (pedaços fora da região do braço que o corte não pegou)
    dead = [v for v in bm.verts if (v.co - p_local).dot(n_local) > 0.002 and v in region]
    bmesh.ops.delete(bm, geom=dead, context='VERTS')
    edges = [e for e in bm.edges if e.is_boundary and all(abs((v.co - p_local).dot(n_local)) < 0.03 for v in e.verts)]
    caps = bmesh.ops.holes_fill(bm, edges=edges, sides=0)['faces']
    # tampa com a cor da pele: cada canto copia o UV que o mesmo vértice já tem na face vizinha
    uv = bm.loops.layers.uv.active
    if uv:
        for f in caps:
            for l in f.loops:
                other = next((ol for ol in l.vert.link_loops if ol.face not in caps), None)
                if other:
                    l[uv].uv = other[uv].uv.copy()
    # pulso afinado: o antebraço vai estreitando até o corte, para caber folgado na boca da luva
    if thin > 0:
        axis_p = inv @ elbow
        for v in bm.verts:
            if v not in region:
                continue
            t = (v.co - p_local).dot(n_local)
            if -0.22 < t <= 0.001:
                foot = axis_p + n_local * (v.co - axis_p).dot(n_local)
                k = 1 - thin * smoothstep(-0.22, 0.0, t)
                v.co = foot + (v.co - foot) * k
    bm.to_mesh(obj.data)
    bm.free()
    obj.data.update()
    print('Mão decepada:', side, 'pulso', tuple(round(x, 3) for x in wrist))


def smoothed_normals(obj, iterations):
    """Normal de cada vértice tirada de uma cópia alisada N vezes (a malha em si não muda)."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    key = [v.co.to_tuple(5) for v in bm.verts]
    bmesh.ops.remove_doubles(bm, verts=bm.verts[:], dist=1e-5)
    for _ in range(iterations):
        bmesh.ops.smooth_vert(bm, verts=bm.verts[:], factor=0.5, use_axis_x=True, use_axis_y=True, use_axis_z=True)
    # posição original -> normal da cópia alisada (o remove_doubles guarda a coordenada do vértice que sobrou)
    bm2 = bmesh.new()
    bm2.from_mesh(obj.data)
    bmesh.ops.remove_doubles(bm2, verts=bm2.verts[:], dist=1e-5)
    bm2.verts.ensure_lookup_table()
    bm.verts.ensure_lookup_table()
    bm.normal_update()
    by_pos = {v2.co.to_tuple(5): bm.verts[v2.index].normal.copy() for v2 in bm2.verts}
    bm.free(); bm2.free()
    return [by_pos.get(k) or obj.data.vertices[i].normal for i, k in enumerate(key)]


def soft_normals(obj, iterations, keep=(), keep_iter=4):
    """Normais suaves no corpo; nos vértices de `keep` (rosto, fivela) um alisamento leve: o nariz continua marcado,
    mas somem as covinhas que o Meshy deixa (olhos, relevo miúdo) e que viravam manchas de sombra."""
    heavy = smoothed_normals(obj, iterations)
    light = smoothed_normals(obj, keep_iter) if keep else heavy
    normals = [light[i] if i in keep else heavy[i] for i in range(len(heavy))]
    obj.data.normals_split_custom_set_from_vertices([n.normalized() for n in normals])
    print('Normais suaves:', obj.name, iterations, 'passadas;', len(keep), 'vértices com alisamento leve')


bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=src)
scene = bpy.context.scene
arm = next((o for o in scene.objects if o.type == 'ARMATURE'), None)

# 1. sobras soltas
for o in list(scene.objects):
    if o.type == 'MESH' and arm and o.parent != arm:
        print('Removido objeto solto:', o.name)
        bpy.data.objects.remove(o, do_unlink=True)

# 2. textura chapada
if '--textura' in opt:
    img = bpy.data.images.load(os.path.abspath(opt['--textura']))
    for m in bpy.data.materials:
        if m.use_nodes:
            for n in m.node_tree.nodes:
                if n.type == 'TEX_IMAGE':
                    n.image = img
            b = next((n for n in m.node_tree.nodes if n.type == 'BSDF_PRINCIPLED'), None)
            if b:
                b.inputs['Roughness'].default_value = 1.0
                b.inputs['Metallic'].default_value = 0.0

bpy.ops.object.select_all(action='DESELECT')
if kind == 'corpo' and arm:
    # 3. aplica a escala do esqueleto e da malha (fica 1 unidade = 1 metro, sem escala nos objetos)
    for o in [arm] + [c for c in arm.children]:
        o.select_set(True)
    bpy.context.view_layer.objects.active = arm
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    # 3b. as protuberâncias dos olhos viram rosto liso (o olho é plano 2.5D por cima)
    if opt.get('aplainar-olhos'):
        for m in [c for c in arm.children if c.type == 'MESH']:
            g = m.vertex_groups.get('Head')
            if not g:
                continue
            head = [(v.index, v.co.z) for v in m.data.vertices if any(ge.group == g.index and ge.weight >= 0.4 for ge in v.groups)]
            zs = sorted(z for _, z in head)
            cut = zs[int(len(zs) * 0.45)]
            smooth_region(m, [i for i, z in head if z >= cut], 40)
            print('Olhos aplainados:', m.name, len(head), 'vértices da cabeça')
    # 4b. crânio arredondado: a parte de cima da cabeça é puxada para um elipsoide liso (some o degrau da nuca e o relevo dos olhos)
    if opt.get('arredondar-cabeca'):
        for m in [c for c in arm.children if c.type == 'MESH']:
            round_head(m)
    # 4. soquetes presos nos ossos, posicionados pela malha real (vértices de cada osso)
    meshes = [c for c in arm.children if c.type == 'MESH']
    bpy.context.view_layer.update()

    def verts_of(group, min_w=0.5):
        pts = []
        for m in meshes:
            g = m.vertex_groups.get(group)
            if not g:
                continue
            for v in m.data.vertices:
                for ge in v.groups:
                    if ge.group == g.index and ge.weight >= min_w:
                        pts.append(m.matrix_world @ v.co)
        return pts

    def bone_world(name, tail=False):
        b = arm.data.bones[name]
        return arm.matrix_world @ (b.tail_local if tail else b.head_local)

    def socket(name, bone, world_pos):
        if bone not in arm.data.bones:
            print('Osso não encontrado para', name, bone)
            return
        e = bpy.data.objects.new(name, None)
        e.empty_display_type = 'ARROWS'
        e.empty_display_size = 0.05
        scene.collection.objects.link(e)
        e.parent = arm
        e.parent_type = 'BONE'
        e.parent_bone = bone
        bpy.context.view_layer.update()
        e.matrix_world = Matrix.Translation(world_pos)   # posição de mundo exata, presa no osso
        print('Soquete', name, '->', bone, tuple(round(x, 3) for x in world_pos))

    head = verts_of('Head')
    if head:
        top = max(head, key=lambda v: v.z)
        front = min(head, key=lambda v: v.y)              # frente do personagem = -Y no Blender
        hz = sorted(v.z for v in head)
        eye_z = hz[int(len(hz) * 0.7)]                    # altura dos olhos: terço superior da cabeça
        socket('head_top', 'Head', Vector((0, (top.y + sum(v.y for v in head) / len(head)) / 2, top.z)))
        socket('head_face', 'Head', Vector((0, front.y, eye_z)))
    for side, bone in (('L', 'LeftHand'), ('R', 'RightHand')):
        hv = verts_of(bone, 0.3)
        if hv:  # centro da mão pela malha (os ossos do Meshy vêm em outra escala de unidade)
            socket(f'hand_{side}_grip', bone, sum(hv, Vector()) / len(hv))
    torso = verts_of('Spine02', 0.3) or verts_of('Spine01', 0.3)
    if torso:
        socket('spine_back', 'Spine02', Vector((0, max(v.y for v in torso), sum(v.z for v in torso) / len(torso))))
    hips = verts_of('Hips', 0.3)
    if hips:
        socket('hips_front', 'Hips', Vector((0, min(v.y for v in hips), sum(v.z for v in hips) / len(hips))))
    # 4c. mãos decepadas no pulso: a luva entra no lugar (o soquete hand_*_grip já foi criado com a mão inteira)
    if opt.get('decepar-maos'):
        for m in meshes:
            for side in ('Left', 'Right'):
                cut_hand(m, side, float(opt.get('--afinar-pulso', 0)))
    # 4d. normais de sombra suaves: a luz dura do toon passa a seguir a forma grande, não cada triângulo (calção sem facetas)
    if '--normais-suaves' in opt:
        for m in meshes:
            keep = set()
            if opt.get('rosto-nitido'):
                # rosto humano: nariz e queixo precisam da sombra real; só a frente da cabeça (o cabelo continua suave)
                hw = group_weights(m, 'Head')
                ys = [(m.matrix_world @ m.data.vertices[i].co).y for i, w in hw.items() if w >= 0.5]
                front = sorted(ys)[int(len(ys) * 0.35)] if ys else 0
                keep = {i for i, w in hw.items() if w >= 0.5 and (m.matrix_world @ m.data.vertices[i].co).y <= front}
            if '--normais-cor' in opt:   # peças duras (fivela): sombra do próprio relevo, sem misturar com a barriga
                img = next((n.image for mt in m.data.materials if mt and mt.use_nodes for n in mt.node_tree.nodes if n.type == 'TEX_IMAGE' and n.image), None)
                if img:
                    W, H = img.size; px = img.pixels[:]; uvl = m.data.uv_layers.active.data
                    cols = [tuple(int(h.strip().lstrip('#')[i:i + 2], 16) for i in (0, 2, 4)) for h in opt['--normais-cor'].split(',')]
                    for poly in m.data.polygons:
                        u = sum(uvl[l].uv.x for l in poly.loop_indices) / poly.loop_total
                        v = sum(uvl[l].uv.y for l in poly.loop_indices) / poly.loop_total
                        k = (min(H - 1, int(v * H)) * W + min(W - 1, int(u * W))) * 4
                        c = [x * 255 for x in px[k:k + 3]]
                        if min(sum((a - b) ** 2 for a, b in zip(c, cc)) for cc in cols) < 40 ** 2:
                            keep |= set(poly.vertices)
            soft_normals(m, int(opt['--normais-suaves']), keep, 1 if '--normais-cor' in opt and not opt.get('rosto-nitido') else 4)
    # 5. pés no chão
    dg = bpy.context.evaluated_depsgraph_get()
    minz = min((m.matrix_world @ v.co).z for m in meshes for v in m.evaluated_get(dg).to_mesh().vertices)
    arm.location.z -= minz
else:
    meshes = [o for o in scene.objects if o.type == 'MESH']
    for o in meshes:
        o.select_set(True)
    bpy.context.view_layer.objects.active = meshes[0]
    if len(meshes) > 1:
        bpy.ops.object.join()
    obj = bpy.context.view_layer.objects.active
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    if opt.get('alinhar-eixo'):
        # peça inclinada (punho na diagonal): o eixo mais comprido (PCA) vira o eixo Z, boca do pulso embaixo
        import numpy as np
        P = np.array([v.co[:] for v in obj.data.vertices]); c0 = P.mean(0)
        w, V = np.linalg.eigh(np.cov((P - c0).T)); ax = Vector(V[:, -1].tolist())
        if ax.z < 0: ax = -ax
        obj.data.transform(Matrix.Translation(Vector(c0.tolist())) @ ax.rotation_difference(Vector((0, 0, 1))).to_matrix().to_4x4() @ Matrix.Translation(-Vector(c0.tolist())))
        print('Eixo alinhado: era', tuple(round(x, 2) for x in ax))
    if opt.get('virar'):   # peça veio de cabeça para baixo (punho embaixo): gira 180° em X -> boca do pulso na base
        obj.data.transform(Matrix.Rotation(math.pi, 4, 'X'))
    # escala pela maior dimensão pedida e base/centro na origem
    target = float(opt.get('--altura', '0') or 0)
    dims = obj.dimensions
    if target:
        s = target / max(dims)
        obj.scale = (s, s, s)
        bpy.ops.object.transform_apply(scale=True)
    vs = [obj.matrix_world @ v.co for v in obj.data.vertices]
    c = Vector(((min(v.x for v in vs) + max(v.x for v in vs)) / 2, (min(v.y for v in vs) + max(v.y for v in vs)) / 2, min(v.z for v in vs)))
    if opt.get('centro-base'):   # a boca do pulso fica no eixo (o punho pode ser torto): gira sem desencaixar
        zmin = min(v.z for v in vs); hz = max(v.z for v in vs) - zmin
        base = [v for v in vs if v.z < zmin + 0.12 * hz]
        c = Vector(((min(v.x for v in base) + max(v.x for v in base)) / 2, (min(v.y for v in base) + max(v.y for v in base)) / 2, zmin))
    obj.data.transform(Matrix.Translation(-c))
    if '--suavizar' in opt:
        smooth_region(obj, range(len(obj.data.vertices)), int(opt['--suavizar']), 0.5)
        print('Suavizado:', opt['--suavizar'], 'passadas')
    if '--alargar-boca' in opt:
        # boca da luva abre em sino: o pulso entra folgado sem aparecer a borda do toco
        k = float(opt['--alargar-boca'])
        vs = obj.data.vertices
        h = max(v.co.z for v in vs)
        cx = sum(v.co.x for v in vs) / len(vs); cy = sum(v.co.y for v in vs) / len(vs)
        r_min = float(opt.get('--boca-raio', 0))   # boca redonda: o lado achatado da luva também cobre o antebraço
        for v in vs:
            w = 1 - smoothstep(0.0, 0.35 * h, v.co.z)
            f = 1 + (k - 1) * w
            dx, dy = (v.co.x - cx) * f, (v.co.y - cy) * f
            r = math.hypot(dx, dy)
            if r_min and r > 1e-6:
                s_r = max(r, r_min * w + r * (1 - w)) / r
                dx, dy = dx * s_r, dy * s_r
            v.co.x, v.co.y = cx + dx, cy + dy
        print('Boca da luva alargada:', k)
    if opt.get('fechar-boca'):
        # tampa a abertura da luva: olhando pela boca não dá para ver o ringue através dela
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        bmesh.ops.remove_doubles(bm, verts=bm.verts[:], dist=1e-5)
        n_caps = len(bmesh.ops.holes_fill(bm, edges=[e for e in bm.edges if e.is_boundary], sides=0)['faces'])
        bm.to_mesh(obj.data)
        bm.free()
        print('Boca da luva fechada:', n_caps, 'tampas')
    # o Meshy às vezes entrega faces viradas para dentro (a peça parece oca com dupla face): todas para fora
    if opt.get('normais-fora'):   # só em peça fechada (num chapéu de aba fina isso vira faces pretas)
        bm = bmesh.new(); bm.from_mesh(obj.data)
        bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
        bm.to_mesh(obj.data); bm.free(); obj.data.update()
    for poly in obj.data.polygons:
        poly.use_smooth = True
    if '--cores-por-face' in opt:
        # objeto simples: troca a textura picada do Meshy por cores chapadas por face (sem manchas nas bordas das ilhas)
        hexes = opt['--cores-por-face'].split(',')
        pal = [tuple(int(h.strip('#')[i:i + 2], 16) / 255 for i in (0, 2, 4)) for h in hexes]
        img = next((n.image for m in obj.data.materials if m and m.use_nodes for n in m.node_tree.nodes if n.type == 'TEX_IMAGE' and n.image), None)
        px, (W, H) = list(img.pixels), img.size
        uv = obj.data.uv_layers.active.data
        mats = []
        for i, c in enumerate(pal):
            m = bpy.data.materials.new(f'cor_{i}')
            m.use_nodes = True
            b = m.node_tree.nodes['Principled BSDF']
            b.inputs['Base Color'].default_value = (*[x ** 2.2 for x in c], 1)
            b.inputs['Roughness'].default_value = 1.0
            mats.append(m)
        obj.data.materials.clear()
        for m in mats:
            obj.data.materials.append(m)
        for poly in obj.data.polygons:
            u = sum(uv[l].uv.x for l in poly.loop_indices) / poly.loop_total
            v = sum(uv[l].uv.y for l in poly.loop_indices) / poly.loop_total
            x, y = min(W - 1, max(0, int(u * W))), min(H - 1, max(0, int(v * H)))
            k = (y * W + x) * 4
            c = px[k:k + 3]
            poly.material_index = min(range(len(pal)), key=lambda i: sum((a - b) ** 2 for a, b in zip(c, pal[i])))
        # tira ilhas de uma face só (ruído) olhando a cor das vizinhas
        for _ in range(2):
            me = obj.data
            edge_faces = {}
            for poly in me.polygons:
                for ek in poly.edge_keys:
                    edge_faces.setdefault(ek, []).append(poly.index)
            for poly in me.polygons:
                nb = [me.polygons[f].material_index for ek in poly.edge_keys for f in edge_faces[ek] if f != poly.index]
                if nb and nb.count(poly.material_index) == 0:
                    poly.material_index = max(set(nb), key=nb.count)
        print('Cores por face:', hexes)
        if '--apagar-indice' in opt:   # tira as faces de uma das cores (ex.: o rosto branco que o Meshy pôs dentro da peruca)
            di = int(opt['--apagar-indice'])
            bm = bmesh.new(); bm.from_mesh(obj.data)
            dead = [f for f in bm.faces if f.material_index == di]
            bmesh.ops.delete(bm, geom=dead, context='FACES')
            bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context='VERTS')
            bm.to_mesh(obj.data); bm.free(); obj.data.update()
            print('Faces apagadas da cor', di, ':', len(dead))
        if '--faixa' in opt:
            # faixa (ex.: fita do chapéu): pega a altura onde a cor da faixa domina e pinta a volta inteira nessa altura,
            # sem os riscos da cor vizinha que o Meshy deixou na textura
            fi = int(opt['--faixa'])
            band = [p for p in obj.data.polygons if p.material_index == fi]
            if band:
                zs = sorted(p.center.z for p in band)
                lo, hi = zs[int(len(zs) * 0.1)], zs[int(len(zs) * 0.9)]
                rs = sorted(Vector((p.center.x, p.center.y)).length for p in band)
                rmin = rs[len(rs) // 2] * 0.85
                n = 0
                for p in obj.data.polygons:
                    if lo <= p.center.z <= hi and abs(p.normal.z) < 0.6 and Vector((p.center.x, p.center.y)).length >= rmin and p.material_index == 0:
                        p.material_index = fi; n += 1
                print('Faixa limpa:', n, 'faces repintadas entre z', round(lo, 3), round(hi, 3))
        if opt.get('interior-escuro'):
            # peça oca (o Meshy fez parede dupla): o que está virado para dentro aparece pelos furos -> última cor (escura)
            cxy = Vector((sum(v.co.x for v in obj.data.vertices), sum(v.co.y for v in obj.data.vertices))) / len(obj.data.vertices)
            n_in = 0
            for poly in obj.data.polygons:
                rad = Vector((poly.center.x - cxy.x, poly.center.y - cxy.y))
                if rad.length > 1e-6 and Vector((poly.normal.x, poly.normal.y)).dot(rad.normalized()) < -0.2:
                    poly.material_index = len(pal) - 1; n_in += 1
            print('Interior escuro:', n_in, 'faces')
            if opt.get('encher'):
                # enche a peça: some a parede de dentro e as bordas que sobram (furos e a boca de baixo) viram tampas
                # chapadas escuras -> não dá mais para ver o fundo do ringue através dela
                bm = bmesh.new(); bm.from_mesh(obj.data)
                bmesh.ops.remove_doubles(bm, verts=bm.verts[:], dist=1e-4)
                dark = len(pal) - 1
                inner = [f for f in bm.faces if f.material_index == dark and Vector((f.normal.x, f.normal.y)).length > 0.3
                         and Vector((f.normal.x, f.normal.y)).dot(Vector((f.calc_center_median().x - cxy.x, f.calc_center_median().y - cxy.y)).normalized()) < -0.2]
                bmesh.ops.delete(bm, geom=inner, context='FACES')
                loose = [v for v in bm.verts if not v.link_faces]
                bmesh.ops.delete(bm, geom=loose, context='VERTS')
                caps = bmesh.ops.holes_fill(bm, edges=[e for e in bm.edges if e.is_boundary], sides=0)['faces']
                bmesh.ops.triangulate(bm, faces=caps)
                for f in bm.faces:
                    if f.material_index >= len(pal) or f in caps:
                        f.material_index = dark
                for f in bm.faces:
                    f.smooth = True
                bm.normal_update()
                bm.to_mesh(obj.data); bm.free(); obj.data.update()
                print('Peça enchida:', len(inner), 'faces de dentro removidas, tampas novas')
    if '--normais-suaves' in opt:   # peça com amassado do Meshy (copa do chapéu): sombra segue a forma geral
        soft_normals(obj, int(opt['--normais-suaves']))
    obj.name = opt.get('--nome', obj.name)
    if opt.get('espelhar'):
        obj.data.transform(Matrix.Scale(-1, 4, (1, 0, 0)))
        obj.data.flip_normals()

bpy.ops.export_scene.gltf(filepath=out, export_format='GLB', export_extras=True)
print('Exportado:', out)
