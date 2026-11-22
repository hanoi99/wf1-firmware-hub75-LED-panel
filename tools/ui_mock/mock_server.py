#!/usr/bin/env python3
"""Mock des Geraete-HTTP-API, damit die Weboberflaeche (data/) ohne Hardware testbar ist.
Die Szenenliste stammt aus dem echten C++-Code:  bash tools/sim/build.sh && python3 tools/ui_mock/mock_server.py
Aufruf: mock_server.py [port]   ->  http://localhost:8080/"""
import json, os, subprocess, sys, time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import parse_qs

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
out = subprocess.run([os.path.join(ROOT, ".sim", "status_json")], capture_output=True, text=True).stdout
scenes = json.loads([l for l in out.splitlines() if l.startswith("[{")][0])
state = {"scene": "fire", "brightness": 96, "text": "Hallo Welt", "demo": {"on": False, "sec": 20, "ids": "fire,rain,stars"},
         "ssid": "HeimNetz", "t0": time.time(), "log": []}

def status():
    return {"version": "mock", "scene": state["scene"], "title": next(s["title"] for s in scenes if s["id"] == state["scene"]),
            "brightness": state["brightness"], "maxBrightness": 180, "text": state["text"], "maxText": 240, "demo": state["demo"],
            "sys": {"heap": 91234, "minHeap": 80000, "up": int(time.time() - state["t0"]), "rssi": -61, "ip": "192.168.1.50",
                    "ap": False, "time": True, "dbl": True}, "scenes": scenes}

class H(BaseHTTPRequestHandler):
    def log_message(self, *a): pass
    def send(self, code, body, ctype="application/json"):
        b = body if isinstance(body, bytes) else json.dumps(body).encode()
        self.send_response(code); self.send_header("Content-Type", ctype); self.send_header("Content-Length", str(len(b))); self.end_headers(); self.wfile.write(b)
    def do_GET(self):
        p = self.path.split("?")[0]
        if p == "/api/status": return self.send(200, status())
        if p == "/api/state": return self.send(200, {"scene": state["scene"], "demo": state["demo"]["on"], "brightness": state["brightness"], "note": "48.2, 11.5" if state["scene"] == "iss" else "", "heap": 90000, "rssi": -61, "up": int(time.time() - state["t0"])})
        if p == "/api/wifi": return self.send(200, {"ssid": state["ssid"], "connected": True, "ip": "192.168.1.50", "ap": False, "scanning": False, "networks": [{"ssid": "HeimNetz", "rssi": -55, "open": False}, {"ssid": "Gast", "rssi": -70, "open": True}]})
        if p == "/api/log": return self.send(200, state["log"])
        f = {"/": "index.html"}.get(p, p.lstrip("/"))
        path = os.path.join(ROOT, "data", f)
        if f in ("index.html", "app.js", "style.css") and os.path.exists(path):
            ct = {"html": "text/html; charset=utf-8", "js": "application/javascript", "css": "text/css"}[f.rsplit(".", 1)[1]]
            return self.send(200, open(path, "rb").read(), ct)
        self.send(404, b"not found", "text/plain")
    def do_POST(self):
        n = int(self.headers.get("Content-Length", 0)); a = {k: v[0] for k, v in parse_qs(self.rfile.read(n).decode(), keep_blank_values=True).items()}
        p = self.path; state["log"].append([p, a])
        if p == "/api/scene":
            if a.get("id") not in [s["id"] for s in scenes]: return self.send(400, {"ok": False, "error": "unknown scene"})
            state["scene"] = a["id"]; state["demo"]["on"] = False
        elif p == "/api/option":
            sc = next((s for s in scenes if s["id"] == a.get("scene")), None); o = sc and next((o for o in sc["opts"] if o["k"] == a.get("key")), None)
            if not o: return self.send(400, {"ok": False, "error": "invalid option or value"})
            o["v"] = int(a["value"])
        elif p == "/api/text": state["text"] = a.get("text", "")
        elif p == "/api/brightness": state["brightness"] = int(a["value"])
        elif p == "/api/demo":
            d = state["demo"]
            if "on" in a: d["on"] = a["on"] == "1"
            if "sec" in a: d["sec"] = int(a["sec"])
            if "ids" in a: d["ids"] = a["ids"]
        elif p == "/api/wifi": state["ssid"] = a.get("ssid", "")
        self.send(200, {"ok": True})

if __name__ == "__main__":
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8080
    print(f"UI-Mock auf http://localhost:{port}/"); ThreadingHTTPServer(("127.0.0.1", port), H).serve_forever()
