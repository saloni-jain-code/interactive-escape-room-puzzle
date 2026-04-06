"""
CIPHER — Combined Server
========================
Runs everything in one command:
  - HTTP server (port 8080) — serves the puzzle HTML with no caching
  - WebSocket bridge (port 8766) — forwards ESP32 serial commands to browser
  - Auto-opens the browser to the puzzle page

Install deps:  pip install pyserial websockets
Run:           python3 run.py
"""

import asyncio
import http.server
import socketserver
import threading
import webbrowser
import time
import serial
import serial.tools.list_ports
import websockets

# ── CONFIG ──────────────────────────────────────────────────────────
SERIAL_BAUD  = 115200
WS_PORT      = 8766
HTTP_PORT    = 8080
SERIAL_PORT  = '/dev/cu.usbserial-59270096991'  # or None to auto-detect
HTML_FILE    = 'escape_puzzle.html'
# ────────────────────────────────────────────────────────────────────

connected_browsers = set()

# ── HTTP SERVER (no-cache) ───────────────────────────────────────────
class NoCacheHandler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header('Cache-Control', 'no-store, no-cache, must-revalidate')
        self.send_header('Pragma', 'no-cache')
        self.send_header('Expires', '0')
        super().end_headers()

    def log_message(self, format, *args):
        pass  # suppress HTTP request logs for clean output

def start_http_server():
    with socketserver.TCPServer(('', HTTP_PORT), NoCacheHandler) as httpd:
        print(f'[HTTP] Serving on http://localhost:{HTTP_PORT}')
        httpd.serve_forever()

# ── WEBSOCKET BRIDGE ─────────────────────────────────────────────────
async def browser_handler(websocket):
    print(f'[WS] Browser connected')
    connected_browsers.add(websocket)
    try:
        async for msg in websocket:
            print(f'[WS←Browser] {msg}')
    except websockets.exceptions.ConnectionClosed:
        pass
    finally:
        connected_browsers.discard(websocket)
        print('[WS] Browser disconnected')

async def broadcast(message):
    if connected_browsers:
        await asyncio.gather(
            *[b.send(message) for b in connected_browsers],
            return_exceptions=True
        )

def find_esp32_port():
    if SERIAL_PORT:
        return SERIAL_PORT
    ports = list(serial.tools.list_ports.comports())
    for p in ports:
        desc = (p.description or '').lower()
        if any(x in desc for x in ['cp210', 'ch340', 'ftdi', 'esp', 'uart']):
            print(f'[Serial] Auto-detected ESP32 on: {p.device} ({p.description})')
            return p.device
    if ports:
        print(f'[Serial] No ESP32 detected, using first port: {ports[0].device}')
        return ports[0].device
    return None

async def serial_reader():
    while True:
        port = find_esp32_port()
        if not port:
            print('[Serial] No port found — retrying in 3s...')
            await asyncio.sleep(3)
            continue
        print(f'[Serial] Opening {port} at {SERIAL_BAUD} baud...')
        try:
            ser = serial.Serial(port, SERIAL_BAUD, timeout=1)
            print(f'[Serial] Connected to {port}')
            while True:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    print(f'[Serial→WS] {line}')
                    await broadcast(line)
        except serial.SerialException as e:
            print(f'[Serial] Error: {e} — retrying in 3s...')
            await asyncio.sleep(3)
        except Exception as e:
            print(f'[Serial] Unexpected error: {e} — retrying in 3s...')
            await asyncio.sleep(3)

async def main():
    # Keep WS server alive even if serial_reader exits
    async with websockets.serve(browser_handler, '127.0.0.1', WS_PORT):
        print(f'[WS] Bridge listening on ws://127.0.0.1:{WS_PORT}')
        await serial_reader()

# ── ENTRY POINT ──────────────────────────────────────────────────────
if __name__ == '__main__':
    print('=' * 50)
    print('  CIPHER — Combined Server')
    print('=' * 50)

    # Start HTTP server in background thread
    http_thread = threading.Thread(target=start_http_server, daemon=True)
    http_thread.start()

    # Small delay then open browser
    def open_browser():
        time.sleep(1.5)
        url = f'http://localhost:{HTTP_PORT}/{HTML_FILE}'
        print(f'[Browser] Opening {url}')
        webbrowser.open(url)

    browser_thread = threading.Thread(target=open_browser, daemon=True)
    browser_thread.start()

    print('  Run this in the same folder as your HTML file.')
    print(f'  Browser will open automatically to the puzzle page.')
    print('=' * 50)

    # Run async WS bridge + serial reader on main thread
    asyncio.run(main())