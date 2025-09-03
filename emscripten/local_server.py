#!/usr/bin/env python3
from http.server import HTTPServer, SimpleHTTPRequestHandler, test
import os

class CORSRequestHandler(SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
        self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
        self.send_header('Cache-Control', 'no-store, no-cache, must-revalidate')
        SimpleHTTPRequestHandler.end_headers(self)

os.chdir("build")
test(CORSRequestHandler, HTTPServer, port=8000)
