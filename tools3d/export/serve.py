# tools3d/export/serve.py: serve o repositório e grava em assets3d/game/ (só nesta pasta).
import http.server, pathlib, subprocess, sys
ROOT = pathlib.Path(__file__).resolve().parents[2]
OUT = ROOT / 'assets3d' / 'game'
class H(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *a, **k): super().__init__(*a, directory=str(ROOT), **k)
    def end_headers(self):
        self.send_header('Cache-Control', 'no-store')   # sem isso o navegador segue usando o módulo antigo depois de uma edição
        super().end_headers()
    def do_GET(self):
        # as edições do Nerd no working copy foram reprovadas: para o jogo vale o Batalhator do commit
        if self.path.split('?')[0] == '/tools3d/viewer/vinil/batalhator.js':
            body = subprocess.check_output(['git', 'show', 'HEAD:tools3d/viewer/vinil/batalhator.js'], cwd=ROOT)
            self.send_response(200); self.send_header('Content-Type', 'text/javascript'); self.send_header('Content-Length', str(len(body))); self.end_headers(); self.wfile.write(body); return
        super().do_GET()
    def do_PUT(self):
        name = pathlib.PurePosixPath(self.path).name
        if not (name.endswith('.glb') or name.endswith('.kfx')): self.send_error(400); return   # .kfx: trilhas dos efeitos dos especiais (export_fx.html)
        OUT.mkdir(parents=True, exist_ok=True)
        (OUT / name).write_bytes(self.rfile.read(int(self.headers['Content-Length'])))
        self.send_response(204); self.end_headers()
http.server.ThreadingHTTPServer(('127.0.0.1', int(sys.argv[1]) if len(sys.argv) > 1 else 8770), H).serve_forever()
