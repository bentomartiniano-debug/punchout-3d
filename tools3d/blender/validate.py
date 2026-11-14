"""Validador de assets 3D do K.O.-RAMA (roda dentro do Blender, sem janela).

  blender -b --factory-startup -P tools3d/blender/validate.py -- arquivo.glb relatorio.json [--tipo corpo|prop] [--altura 2.15] [--max-tris 15000]

Confere o que o plano exige (docs/PLANO_3D_MESHY.md, seção 2) e grava um relatório com aprovado/reprovado por item.
"""
import bpy, bmesh, json, math, os, sys
from mathutils import Vector

argv = sys.argv[sys.argv.index('--') + 1:]
src, out = os.path.abspath(argv[0]), os.path.abspath(argv[1])
rest = [a for a in argv[2:] if a not in ('--cor-chapada', '--limpo')]
opt = {k: v for k, v in zip(rest[0::2], rest[1::2])}
kind = opt.get('--tipo', 'corpo')
height_target = float(opt.get('--altura', '0') or 0)
max_tris = int(opt.get('--max-tris', '16000' if kind == 'corpo' else '3500'))

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=src)

meshes = [o for o in bpy.context.scene.objects if o.type == 'MESH']
arms = [o for o in bpy.context.scene.objects if o.type == 'ARMATURE']
report = {'arquivo': src, 'tipo': kind, 'itens': [], 'dados': {}}
shapes = {pb.custom_shape for a in arms for pb in a.pose.bones if pb.custom_shape}  # esfera que o importador do Blender cria para desenhar os ossos
meshes = [o for o in meshes if o not in shapes]
stray = [o.name for o in meshes if arms and o.parent not in arms]
if arms:
    meshes = [o for o in meshes if o.parent in arms]


def item(nome, ok, valor, regra):
    report['itens'].append({'item': nome, 'ok': bool(ok), 'valor': valor, 'regra': regra})


# geometria (avaliada depois do esqueleto, na pose de repouso)
dg = bpy.context.evaluated_depsgraph_get()
tris = 0
islands = 0
mins = Vector((1e9, 1e9, 1e9))
maxs = Vector((-1e9, -1e9, -1e9))
for o in meshes:
    ev = o.evaluated_get(dg)
    me = ev.to_mesh()
    bm = bmesh.new()
    bm.from_mesh(me)
    bmesh.ops.triangulate(bm, faces=bm.faces[:])
    bmesh.ops.remove_doubles(bm, verts=bm.verts[:], dist=1e-5)  # glTF separa vértices nas costuras
    bm.faces.index_update()
    tris += len(bm.faces)
    seen = set()
    for f in bm.faces:  # ilhas soltas
        if f.index in seen:
            continue
        islands += 1
        stack = [f]
        while stack:
            g = stack.pop()
            if g.index in seen:
                continue
            seen.add(g.index)
            for e in g.edges:
                for h in e.link_faces:
                    if h.index not in seen:
                        stack.append(h)
    for v in bm.verts:
        w = o.matrix_world @ v.co
        mins = Vector((min(mins[i], w[i]) for i in range(3)))
        maxs = Vector((max(maxs[i], w[i]) for i in range(3)))
    bm.free()
    ev.to_mesh_clear()

size = maxs - mins
item('Sem objetos soltos fora do esqueleto', not stray, stray, 'nenhum')
report['dados'].update({'triangulos': tris, 'ilhas': islands, 'min': list(mins), 'max': list(maxs),
                        'largura_x': size.x, 'profundidade_y': size.y, 'altura_z': size.z})
item('Triângulos', tris <= max_tris, tris, f'≤ {max_tris}')
if height_target:
    item('Altura', abs(size.z - height_target) / height_target < 0.1, round(size.z, 3), f'{height_target} m ± 10%')
if kind == 'corpo' or '--limpo' in argv:
    item('Pés/base no chão (min Z)', abs(mins.z) < 0.03 * max(size.z, 0.01), round(mins.z, 3), '≈ 0')
item('Centrado em X', abs((mins.x + maxs.x) / 2) < 0.05 * max(size.x, 0.01), round((mins.x + maxs.x) / 2, 3), '≈ 0')
item('Peças soltas', islands <= (12 if kind == 'corpo' else 10), islands, 'poucas ilhas (o Meshy às vezes solta pedaços)')

# materiais e texturas
imgs, pbr = [], []
for m in bpy.data.materials:
    if not m.use_nodes:
        continue
    for n in m.node_tree.nodes:
        if n.type == 'TEX_IMAGE' and n.image:
            imgs.append({'imagem': n.image.name, 'tamanho': list(n.image.size)})
        if n.type == 'NORMAL_MAP':
            pbr.append('normal')
    bsdf = next((n for n in m.node_tree.nodes if n.type == 'BSDF_PRINCIPLED'), None)
    if bsdf:
        for sock in ('Metallic', 'Roughness'):
            if bsdf.inputs[sock].is_linked:
                pbr.append(sock.lower())
report['dados'].update({'materiais': len(bpy.data.materials), 'texturas': imgs, 'mapas_pbr': sorted(set(pbr))})
if '--cor-chapada' not in argv:
    item('Tem textura de cor', len(imgs) >= 1, len(imgs), '≥ 1')
item('Sem mapas PBR', not pbr, sorted(set(pbr)), 'nenhum (o toon ignora)')
item('Textura ≤ 2048', all(max(i['tamanho']) <= 2048 for i in imgs), [i['tamanho'] for i in imgs], '≤ 2048 px')

# esqueleto
if kind == 'corpo':
    if arms:
        arm = arms[0]
        bones = [b.name for b in arm.data.bones]
        report['dados']['ossos'] = bones
        item('Esqueleto', len(bones) >= 15, len(bones), '≥ 15 ossos (bípede)')

        def find(*names):
            for b in arm.data.bones:
                low = b.name.lower()
                if all(n in low for n in names):
                    return b
            return None
        angles = {}
        for side, tags in (('esquerdo', ('left',)), ('direito', ('right',))):
            b = find('arm', *tags)
            b = b or find('upperarm', *tags) or find('arm', side[0])
            if b:
                d = (arm.matrix_world.to_3x3() @ (b.tail_local - b.head_local)).normalized()
                angles[side] = round(math.degrees(math.asin(max(-1, min(1, -d.z)))), 1)  # graus abaixo da horizontal
        report['dados']['angulo_bracos'] = angles
        if angles:
            item('A-pose (braços 30–60° abaixo da horizontal)', all(25 <= a <= 65 for a in angles.values()), angles, '≈ 45°')
        else:
            item('A-pose', False, 'ossos do braço não encontrados', 'nomes dos ossos a mapear')
    else:
        item('Esqueleto', False, 0, 'corpo sem rig (normal antes do auto-rig)')

report['aprovado'] = all(i['ok'] for i in report['itens'])
with open(out, 'w', encoding='utf-8') as f:
    json.dump(report, f, indent=2, ensure_ascii=False)
print('APROVADO' if report['aprovado'] else 'REPROVADO', out)
for i in report['itens']:
    print(('  ok  ' if i['ok'] else '  XX  ') + i['item'], '=', i['valor'], '|', i['regra'])
