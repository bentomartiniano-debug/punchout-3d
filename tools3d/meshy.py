"""Cliente do Meshy para o pipeline 3D do K.O.-RAMA.

A chave vem SEMPRE da variável de ambiente MESHY_API_KEY (nunca de arquivo do projeto).

  python tools3d/meshy.py saldo
  python tools3d/meshy.py plano  assets3d/frog/jobs.json        # mostra o que seria enviado e o custo, não gasta
  python tools3d/meshy.py enviar assets3d/frog/jobs.json --sim  # envia só os trabalhos ainda não feitos
  python tools3d/meshy.py baixar assets3d/frog/jobs.json        # acompanha e baixa o que terminou

Cada trabalho fica registrado em <pasta>/meshy_log.json (id da tarefa, status, créditos gastos, arquivos baixados).
Um trabalho com id registrado nunca é reenviado: para refazer, apague a entrada dele no log (ou use --refazer NOME).
"""
import base64, json, mimetypes, os, sys, time, urllib.request, urllib.error
sys.stdout.reconfigure(encoding='utf-8')
from pathlib import Path

API = 'https://api.meshy.ai/openapi'
KIND = {  # tipo do trabalho -> (rota, custo estimado em créditos)
    'multi': ('/v1/multi-image-to-3d', 30),
    'image': ('/v1/image-to-3d', 30),
    'image_lite': ('/v1/image-to-3d', 15),
    'rig': ('/v1/rigging', 5),
}


def key():
    k = os.environ.get('MESHY_API_KEY', '').strip()
    if not k:
        sys.exit('Defina a variável de ambiente MESHY_API_KEY.')
    return k


def call(method, route, body=None):
    req = urllib.request.Request(API + route, method=method, headers={'Authorization': 'Bearer ' + key(), 'Content-Type': 'application/json'},
                                 data=json.dumps(body).encode() if body is not None else None)
    try:
        with urllib.request.urlopen(req, timeout=120) as r:
            return json.loads(r.read() or b'{}')
    except urllib.error.HTTPError as e:
        msg = e.read().decode(errors='replace')
        sys.exit(f'Meshy respondeu {e.code} em {method} {route}: {msg}')


def data_uri(path):
    mime = mimetypes.guess_type(str(path))[0] or 'image/png'
    return f'data:{mime};base64,' + base64.b64encode(Path(path).read_bytes()).decode()


def load(jobs_file):
    jobs_file = Path(jobs_file)
    spec = json.loads(jobs_file.read_text(encoding='utf-8'))
    log_file = jobs_file.parent / 'meshy_log.json'
    log = json.loads(log_file.read_text(encoding='utf-8')) if log_file.exists() else {}
    return jobs_file.parent, spec, log_file, log


def save(log_file, log):
    log_file.write_text(json.dumps(log, indent=2, ensure_ascii=False), encoding='utf-8')


def body_for(base, job, log):
    """Monta o corpo do pedido a partir do jobs.json (caminhos relativos à pasta do personagem)."""
    p = dict(job.get('params', {}))
    if job['kind'] == 'multi':
        p['image_urls'] = [data_uri(base / f) for f in job['images']]
    elif job['kind'] in ('image', 'image_lite'):
        p['image_url'] = data_uri(base / job['image'])
    elif job['kind'] == 'rig':
        dep = log.get(job['from'], {})
        if dep.get('status') != 'SUCCEEDED':
            return None  # espera o modelo de origem terminar
        p['input_task_id'] = dep['task_id']
    return p


def cmd_saldo():
    print(json.dumps(call('GET', '/v1/balance'), indent=2))


def cmd_plano(jobs_file, refazer=()):
    base, spec, _, log = load(jobs_file)
    total = 0
    for name, job in spec['jobs'].items():
        done = name in log and name not in refazer
        cost = KIND[job['kind']][1]
        total += 0 if done else cost
        print(f"{'feito ' if done else 'ENVIAR'}  {name:28s} {job['kind']:10s} {cost:3d} créditos")
    print(f'Total a gastar: {total} créditos')
    return total


def cmd_enviar(jobs_file, sim=False, refazer=()):
    base, spec, log_file, log = load(jobs_file)
    total = cmd_plano(jobs_file, refazer)
    if not sim:
        print('Nada foi enviado. Repita com --sim para gastar os créditos.')
        return
    pending = True
    while pending:
        pending = False
        for name, job in spec['jobs'].items():
            if name in log and name not in refazer:
                continue
            body = body_for(base, job, log)
            if body is None:
                pending = True
                continue
            route = KIND[job['kind']][0]
            task = call('POST', route, body)['result']
            log[name] = {'task_id': task, 'kind': job['kind'], 'route': route, 'status': 'PENDING', 'sent_at': time.strftime('%Y-%m-%d %H:%M:%S')}
            save(log_file, log)
            print(f'Enviado {name}: {task}')
        if pending:  # há rig esperando o modelo: acompanha até liberar
            cmd_baixar(jobs_file, wait=True, only_pending=True)
    print(f'Envio concluído (estimado {total} créditos).')


def download(url, dest):
    dest.parent.mkdir(parents=True, exist_ok=True)
    with urllib.request.urlopen(url, timeout=300) as r:
        dest.write_bytes(r.read())
    return str(dest)


def cmd_baixar(jobs_file, wait=True, only_pending=False):
    base, spec, log_file, log = load(jobs_file)
    while True:
        busy = False
        for name, entry in log.items():
            if entry.get('status') in ('SUCCEEDED', 'FAILED', 'CANCELED'):
                continue
            task = call('GET', f"{entry['route']}/{entry['task_id']}")
            entry['status'] = task.get('status')
            entry['progress'] = task.get('progress')
            entry['consumed_credits'] = task.get('consumed_credits')
            if task.get('status') == 'SUCCEEDED':
                files = {}
                src = base / 'source'
                if entry['kind'] == 'rig':
                    res = task.get('result') or {}
                    files['glb'] = download(res['rigged_character_glb_url'], src / f'{name}.glb')
                    for anim in ('walking', 'running'):
                        u = (res.get('basic_animations') or {}).get(f'{anim}_glb_url')
                        if u:
                            files[anim] = download(u, src / f'{name}_{anim}.glb')
                else:
                    files['glb'] = download(task['model_urls']['glb'], src / f'{name}.glb')
                    for i, t in enumerate(task.get('texture_urls') or []):
                        if t.get('base_color'):
                            files[f'base_color_{i}'] = download(t['base_color'], src / f'{name}_base_color_{i}.png')
                    if task.get('thumbnail_url'):
                        files['thumb'] = download(task['thumbnail_url'], src / f'{name}_thumb.png')
                entry['files'] = files
                entry['task'] = {k: task.get(k) for k in ('id', 'status', 'consumed_credits', 'finished_at', 'texture_prompt')}
                print(f'Pronto {name}: {files["glb"]}')
            elif task.get('status') == 'FAILED':
                entry['error'] = task.get('task_error')
                print(f'FALHOU {name}: {task.get("task_error")} (créditos devolvidos)')
            else:
                busy = True
                print(f"{name}: {task.get('status')} {task.get('progress')}%")
            save(log_file, log)
        if not busy or not wait:
            break
        if only_pending and all(spec['jobs'][n].get('kind') != 'rig' or n in log for n in spec['jobs']):
            break
        time.sleep(15)
    spent = sum(e.get('consumed_credits') or 0 for e in log.values())
    print(f'Créditos gastos nesta pasta: {spent}')


if __name__ == '__main__':
    a = sys.argv[1:]
    if not a:
        sys.exit(__doc__)
    refazer = tuple(a[a.index('--refazer') + 1].split(',')) if '--refazer' in a else ()
    if a[0] == 'saldo':
        cmd_saldo()
    elif a[0] == 'plano':
        cmd_plano(a[1], refazer)
    elif a[0] == 'enviar':
        cmd_enviar(a[1], sim='--sim' in a, refazer=refazer)
    elif a[0] == 'baixar':
        cmd_baixar(a[1])
    else:
        sys.exit(__doc__)
