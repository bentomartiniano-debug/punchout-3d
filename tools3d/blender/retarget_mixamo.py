"""Passa uma animação do Mixamo (.fbx) para o esqueleto do Meshy de um personagem e exporta um .glb com a animação.
Transferência pela rotação de cada osso no mundo em relação à pose de descanso (o Mixamo descansa em T, o Meshy em A):
  rot_alvo(t) = [rot_fonte(t) · rot_fonte_descanso⁻¹] · rot_alvo_descanso
O quadril também anda (deslocamento escalado pela altura do quadril). Dedos do Mixamo são ignorados.

  blender -b --factory-startup -P tools3d/blender/retarget_mixamo.py -- corpo.glb anim.fbx saida.glb [--nome samba] [--passo 1] [--loop 0.5] [--suave 1]
  --loop: corta no trecho cujo fim emenda no começo (mín. essa fração do clipe) e repete o 1º quadro no fim
"""
import bpy, os, sys
from mathutils import Matrix

argv = sys.argv[sys.argv.index('--') + 1:]
body_p, fbx_p, out = (os.path.abspath(a) for a in argv[:3])
opt = dict(zip(argv[3::2], argv[4::2]))
NAME, STEP = opt.get('--nome', 'danca'), int(opt.get('--passo', 1))
MAP = {'Hips': 'Hips', 'Spine': 'Spine02', 'Spine1': 'Spine01', 'Spine2': 'Spine', 'Neck': 'neck', 'Head': 'Head'}
for s in ('Left', 'Right'):
    for b in ('Shoulder', 'Arm', 'ForeArm', 'Hand', 'UpLeg', 'Leg', 'Foot', 'ToeBase'):
        MAP[s + b] = s + b

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=body_p)
tgt = next(o for o in bpy.context.scene.objects if o.type == 'ARMATURE')
before = set(bpy.context.scene.objects)
bpy.ops.import_scene.fbx(filepath=fbx_p)
src = next(o for o in bpy.context.scene.objects if o.type == 'ARMATURE' and o not in before)
sact = src.animation_data.action
f0, f1 = (int(x) for x in sact.frame_range)
pre = 'mixamorig:' if any(b.name.startswith('mixamorig:') for b in src.data.bones) else ''
pairs = [(pre + k, v) for k, v in MAP.items() if pre + k in src.pose.bones and v in tgt.pose.bones]
print('Ossos ligados:', len(pairs), '| quadros', f0, '-', f1)

rot = lambda m: m.to_3x3().normalized()
sw, tw = src.matrix_world, tgt.matrix_world
src_rest = {s: rot(sw @ src.data.bones[s].matrix_local) for s, _ in pairs}
tgt_rest = {t: rot(tw @ tgt.data.bones[t].matrix_local) for _, t in pairs}
tw_rot_inv = rot(tw).inverted()
s_hips, t_hips = pre + 'Hips', 'Hips'
s_h0 = (sw @ src.data.bones[s_hips].head_local)
t_h0 = (tw @ tgt.data.bones[t_hips].head_local)
ratio = t_h0.z / max(1e-6, s_h0.z)
print('Escala do quadril:', round(ratio, 3))
s2t = dict(pairs)
order = [b.name for b in tgt.data.bones]               # pais antes dos filhos

tgt.animation_data_create()
act = bpy.data.actions.new(NAME); tgt.animation_data.action = act
for pb in tgt.pose.bones:
    pb.rotation_mode = 'QUATERNION'
t2s = {v: k for k, v in s2t.items()}
n = 0
frames = []
for f in range(f0, f1 + 1, STEP):
    bpy.context.scene.frame_set(f)
    frame_pose = {}
    posed = {}                                          # matriz (espaço da armadura) já decidida de cada osso alvo
    for name in order:
        bone = tgt.data.bones[name]; pb = tgt.pose.bones[name]
        P = bone.parent
        rest_rel = (P.matrix_local.inverted() @ bone.matrix_local) if P else bone.matrix_local.copy()
        parent_pose = posed.get(P.name, P.matrix_local) if P else Matrix.Identity(4)
        base = parent_pose @ rest_rel                    # onde o osso fica sem girar nada
        if name in t2s:
            s = t2s[name]
            D = rot(sw @ src.pose.bones[s].matrix) @ src_rest[s].inverted()
            R = tw_rot_inv @ (D @ tgt_rest[name])
            M = Matrix.Translation(base.to_translation()) @ R.to_4x4()
            if name == t_hips:                           # quadril anda: deslocamento da fonte no mundo, escalado
                d = (sw @ src.pose.bones[s].head) - s_h0
                M = Matrix.Translation(tw.inverted().to_3x3() @ (d * ratio) + base.to_translation()) @ R.to_4x4()
        else:
            M = base
        posed[name] = M
        basis = rest_rel.inverted() @ parent_pose.inverted() @ M
        loc, q, _ = basis.decompose()
        frame_pose[name] = (q.copy(), loc.copy())
    frames.append(frame_pose)
    n += 1
# sinal contínuo (q e -q são a mesma rotação; misturar os dois faz a interpolação dar a volta)
for name in frames[0]:
    for i in range(1, len(frames)):
        q0, q1 = frames[i - 1][name][0], frames[i][name][0]
        if q0.dot(q1) < 0:
            frames[i][name] = (-q1, frames[i][name][1])
# alisamento leve (tira os picos da captura sem perder o ritmo): média ponderada 1-2-1, circular (o clipe é um loop)
SM = int(opt.get('--suave', 1))
for _ in range(SM):
    new = []
    N0 = len(frames)
    for i in range(N0):
        fp = {}
        for name, (q, loc) in frames[i].items():
            qa, qb = frames[(i - 1) % N0][name][0], frames[(i + 1) % N0][name][0]
            if qa.dot(q) < 0: qa = -qa
            if qb.dot(q) < 0: qb = -qb
            v = [qa[k] * 0.25 + q[k] * 0.5 + qb[k] * 0.25 for k in range(4)]
            from mathutils import Quaternion
            qn = Quaternion(v).normalized()
            la, lb = frames[(i - 1) % N0][name][1], frames[(i + 1) % N0][name][1]
            fp[name] = (qn, la * 0.25 + loc * 0.5 + lb * 0.25)
        new.append(fp)
    frames = new
# loop sem emenda: o trecho [a, b) cuja pose do fim (b) é a mais parecida com a do começo (a), com pelo menos LOOPMIN do clipe
def diff(p0, p1):
    e = sum(p0[k][0].rotation_difference(p1[k][0]).angle for k in t2s)
    return e + 4 * (p0[t_hips][1] - p1[t_hips][1]).length
N = len(frames)
a, b = 0, N - 1
if '--loop' in opt:
    LOOPMIN = float(opt['--loop'])
    best = None
    for i in range(0, N // 3):
        for j in range(i + int(N * LOOPMIN), N):
            e = diff(frames[i], frames[j])
            if best is None or e < best[0]:
                best = (e, i, j)
    _, a, b = best
    print('Loop: quadros', a, '->', b, 'de', N, '| diferença', round(best[0], 3), '| início-fim antes', round(diff(frames[0], frames[-1]), 3))
for fr, fp in enumerate(frames[a:b + 1], start=1):
    for name, (q, loc) in fp.items():
        pb = tgt.pose.bones[name]
        pb.rotation_quaternion = q
        pb.keyframe_insert('rotation_quaternion', frame=fr)
        if name == t_hips:
            pb.location = loc
            pb.keyframe_insert('location', frame=fr)
fp = frames[a]                                         # último quadro = primeiro: emenda exata
for name, (q, loc) in fp.items():
    pb = tgt.pose.bones[name]; pb.rotation_quaternion = q; pb.keyframe_insert('rotation_quaternion', frame=b - a + 2)
    if name == t_hips:
        pb.location = loc; pb.keyframe_insert('location', frame=b - a + 2)
print('Quadros gravados:', b - a + 2)
bpy.data.objects.remove(src, do_unlink=True)
for o in list(bpy.context.scene.objects):
    if o.type == 'MESH' and o.parent is None and o not in before:
        bpy.data.objects.remove(o, do_unlink=True)
bpy.context.scene.render.fps = 30 // STEP
for other in list(bpy.data.actions):   # só a animação nova vai no arquivo (o fbx e o rig trazem outras)
    if other is not act:
        bpy.data.actions.remove(other)
bpy.ops.object.select_all(action='DESELECT'); tgt.select_set(True)
for c in tgt.children:
    c.select_set(True)
bpy.ops.export_scene.gltf(filepath=out, export_format='GLB', use_selection=True, export_animations=True,
                          export_animation_mode='ACTIONS', export_force_sampling=True)
print('Exportado:', out)
