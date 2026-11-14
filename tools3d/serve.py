"""Servidor do visualizador sem cache (o navegador sempre pega a versão nova da página e dos modelos).

  python tools3d/serve.py 8766
"""
import sys
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer


class NoCache(SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header('Cache-Control', 'no-store, must-revalidate')
        self.send_header('Expires', '0')
        super().end_headers()


port = int(sys.argv[1]) if len(sys.argv) > 1 else 8766
ThreadingHTTPServer(('', port), NoCache).serve_forever()
