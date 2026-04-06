/*
  CIPHER — ESP32 Touch Interface
  ================================
  Copper tape touch pads wired to ESP32 touch-capable pins.
  Sends commands over Serial USB to the Python bridge.

  WIRING (copper tape → ESP32 pin):
  ──────────────────────────────────
  PAD "ROTATE"  → GPIO 15  (T3)
  PAD "ENTER"   → GPIO 13  (T4)
  PAD "1"       → GPIO 12  (T5)
  PAD "2"       → GPIO 27  (T7)
  PAD "3"       → GPIO 33  (T8)
  PAD "4"       → GPIO 32  (T9)

  MODE SWITCHING:
  ──────────────────────────────────
  Hold ENTER alone for 1s       → enter code mode (sends ENT)
  Hold ENTER + 1 together for 1s → exit code mode  (sends ESC)

  Adjust THRESHOLD if pads are too sensitive or not triggering.
  Lower value = less sensitive. Typical range: 20–50.
*/

#include <Arduino.h>

// ── CONFIG ────────────────────────────────────────────────────────
#define THRESHOLD   25
#define DEBOUNCE_MS 500

// ── TOUCH STATE (interrupt-based, for ROT and digit pads) ─────────
volatile bool rotDetected  = false;
volatile bool pad1Detected = false;
volatile bool pad2Detected = false;
volatile bool pad3Detected = false;
volatile bool pad4Detected = false;

uint8_t rotVal  = 0;
uint8_t pad1Val = 0;
uint8_t pad2Val = 0;
uint8_t pad3Val = 0;
uint8_t pad4Val = 0;

// ── INTERRUPT HANDLERS ────────────────────────────────────────────
void gotRot()  { rotDetected  = true; rotVal  = touchRead(T3); }
void gotPad1() { pad1Detected = true; pad1Val = touchRead(T5); }
void gotPad2() { pad2Detected = true; pad2Val = touchRead(T7); }
void gotPad3() { pad3Detected = true; pad3Val = touchRead(T8); }
void gotPad4() { pad4Detected = true; pad4Val = touchRead(T9); }

// ── DEBOUNCE TIMESTAMPS ───────────────────────────────────────────
unsigned long lastRot  = 0;
unsigned long lastPad1 = 0;
unsigned long lastPad2 = 0;
unsigned long lastPad3 = 0;
unsigned long lastPad4 = 0;

// ── HOLD STATE (polled, for ENTER and ENTER+1 combo) ─────────────
unsigned long entPressTime  = 0;
unsigned long pad1PressTime = 0;
bool entHolding  = false;
bool pad1Holding = false;
bool entFired    = false;
bool comboFired  = false;
unsigned long pad2PressTime = 0;
bool pad2Holding = false;
bool bkFired = false;

// ── HELPER: fire command if debounce passed ────────────────────────
void fireIfReady(const char* command, unsigned long &lastTime) {
  unsigned long now = millis();
  if (now - lastTime > DEBOUNCE_MS) {
    Serial.println(command);
    lastTime = now;
  }
}

// ── SETUP ─────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("CIPHER ESP32 READY");

  // interrupt-based pads (ROT and digits)
  // ignore these mappings in the comments, they are wrong.
  touchAttachInterrupt(T8, gotRot,  THRESHOLD); // ROTATE
  touchAttachInterrupt(T4, gotPad1, THRESHOLD); // KEY 1
  touchAttachInterrupt(T5, gotPad2, THRESHOLD); // KEY 2
  touchAttachInterrupt(T2, gotPad3, THRESHOLD); // KEY 3
  touchAttachInterrupt(T3, gotPad4, THRESHOLD); // KEY 4
}

// ── LOOP ──────────────────────────────────────────────────────────
void loop() {

  // ── POLLED: ENTER and ENTER+1 combo ─────────────────────────────
  bool entTouched  = touchRead(T9) < THRESHOLD;  // ENTER pin
  bool pad1Touched = touchRead(T4) < THRESHOLD;  // 1 pin — adjust to match your wiring

  bool pad2Touched = touchRead(T7) < THRESHOLD; // adjust pin to match your wiring

  if (pad2Touched && !pad2Holding) {
      pad2Holding = true;
      pad2PressTime = millis();
      bkFired = false;
  }
  if (!pad2Touched) {
      pad2Holding = false;
      bkFired = false;
  }

  // ENTER + 2 held together for 1s → backspace
  if (entHolding && pad2Holding && !bkFired) {
      unsigned long comboStart = max(entPressTime, pad2PressTime);
      if (millis() - comboStart > 1000) {
          Serial.println("BACK");
          bkFired = true;
          entFired = true;
      }
  }

  // track ENTER hold
  if (entTouched && !entHolding) {
    entHolding = true;
    entPressTime = millis();
    entFired = false;
    comboFired = false;
  }
  if (!entTouched) {
    entHolding = false;
    entFired = false;
  }

  // track pad 1 hold
  if (pad1Touched && !pad1Holding) {
    pad1Holding = true;
    pad1PressTime = millis();
    comboFired = false;
  }
  if (!pad1Touched) {
    pad1Holding = false;
    comboFired = false;
  }

  // ENTER + 1 held together for 1s → exit code mode
  if (entHolding && pad1Holding && !comboFired) {
    unsigned long comboStart = max(entPressTime, pad1PressTime);
    if (millis() - comboStart > 1000) {
      Serial.println("ESC");
      comboFired = true;
      entFired = true; // prevent ENT from also firing
    }
  }
  // ENTER alone held for 1s → enter code mode
  else if (entHolding && !pad1Holding && !entFired) {
    if (millis() - entPressTime > 1000) {
      Serial.println("ENT");
      entFired = true;
    }
  }

  // ── INTERRUPT-BASED: ROT and digit pads ─────────────────────────
  if (rotDetected) {
    rotDetected = false;
    fireIfReady("ROT", lastRot);
  }
  if (pad1Detected) {
    pad1Detected = false;
    if (!entHolding) fireIfReady("1", lastPad1);
  }
  if (pad2Detected) {
    pad2Detected = false;
    if (!entHolding) fireIfReady("2", lastPad2);
  }
  if (pad3Detected) {
    pad3Detected = false;
    if (!entHolding) fireIfReady("3", lastPad3);
  }
  if (pad4Detected) {
    pad4Detected = false;
    if (!entHolding) fireIfReady("4", lastPad4);
  }
}