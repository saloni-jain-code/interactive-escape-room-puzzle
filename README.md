# CIPHER
 
An escape room puzzle installation built with an ESP32, copper tape capacitive touch pads, and a browser-based interface. Players rotate four overlaid card segments to reveal a hidden 4-digit code, then enter it using the touch pads.
 
---
 
## How It Works
 
The system has three layers:
 
- **ESP32** — reads copper tape touch pads and sends short string commands over USB serial
- **`run.py`** — Python bridge that forwards serial commands to the browser via WebSocket, and serves the HTML with no caching
- **`escape_puzzle.html`** — browser puzzle interface, handles all game logic and rendering
 
---
 
## Hardware
 
- ESP32 development board
- Copper tape (for touch pads)
- USB cable (data-capable, not charge-only)
 
**Wiring:**
 
| Pad | ESP32 Pin |
|-----|-----------|
| ROTATE | GPIO 15 (T3) |
| ENTER | GPIO 13 (T4) |
| 1 | GPIO 12 (T5) |
| 2 | GPIO 27 (T7) |
| 3 | GPIO 33 (T8) |
| 4 | GPIO 32 (T9) |
 
Each copper tape pad connects directly to the GPIO pin. No resistors needed — the ESP32 has built-in capacitive touch sensing.
 
---
 
## Installation
 
### 1. Flash the ESP32
 
Open `main.cpp` in PlatformIO or Arduino IDE and upload to your ESP32. Make sure your serial monitor / bridge script is **not** running when flashing — it will block the port.
 
### 2. Install Python dependencies
 
```bash
pip install pyserial websockets
```
 
### 3. Configure the serial port
 
In `run.py`, set `SERIAL_PORT` to your ESP32's port:
 
```python
SERIAL_PORT = '/dev/cu.usbserial-XXXXXXXX'  # macOS/Linux
# SERIAL_PORT = 'COM3'                       # Windows
```
 
Set to `None` to auto-detect.
 
---
 
## Usage
 
1. Flash and keep the ESP32 plugged in
2. In the project folder, run:
 
```bash
python3 run.py
```
 
The browser will open automatically to the puzzle. The HTTP server runs on port 8080 and the WebSocket bridge on port 8766.
 
---
 
## Controls
 
| Action | Input |
|--------|-------|
| Rotate current card | Touch ROTATE pad |
| Enter code mode | Hold ENTER for 1 second |
| Exit code mode | Hold ENTER + 1 for 1 second |
| Enter digit | Touch digit pad (1–4) |
| Backspace | Hold ENTER + 2 for 1 second |
| Submit code | Hold ENTER (in code mode) |
 
Cards advance automatically once aligned. All four cards must be at 0° rotation for the code to be revealed.
 
---
 
## Tuning
 
If pads are too sensitive or triggering each other, adjust these values in `main.cpp`:
 
```cpp
#define THRESHOLD   25   // lower = less sensitive
#define DEBOUNCE_MS 500  // higher = more time between triggers
```
 
Typical threshold range is 20–50. If you have long wire runs between pads, route them away from each other to reduce crosstalk.
 
