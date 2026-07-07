#include <Arduino_GFX_Library.h>

// --- Needle pins ---
const int PIN_SPEEDO = 11;
const int PIN_TACHO  = 10;

// --- Screen 1 (physical, CS pin 4) — now shows CPU info ---
#define TFT1_CS   4
#define TFT1_DC   7
#define TFT1_RST  8

// --- Screen 2 (physical, CS pin 9) — now shows GPU info ---
#define TFT2_CS   9
#define TFT2_DC   6
#define TFT2_RST  5

Arduino_DataBus *bus1 = new Arduino_HWSPI(TFT1_DC, TFT1_CS, SCK, MOSI);
Arduino_DataBus *bus2 = new Arduino_HWSPI(TFT2_DC, TFT2_CS, SCK, MOSI);

Arduino_GFX *screen1 = new Arduino_GC9A01(bus1, TFT1_RST, 0, true); // CPU screen
Arduino_GFX *screen2 = new Arduino_GC9A01(bus2, TFT2_RST, 0, true); // GPU screen

// --- Speedometer calibration (CPU temperature) ---
const int SPEEDO_POINTS   = 18;
const float SPEEDO_C[]    = {  10,  15,  20,  25,  30,  35,  40,  45,  50,  55,  60,  65,  70,  75,  80,  85,  90,  95 };
const int   SPEEDO_FREQ[] = {  32,  45,  57,  70,  84,  96, 109, 122, 132, 141, 151, 162, 173, 184, 196, 210, 220, 229 };

// --- Tachometer calibration (GPU temperature) ---
const int TACHO_POINTS   = 23;
const float TACHO_C[]    = {  10,  15,  20,  25,  30,  35,  40,  45,  50,  55,  60,  65,  70,  75,  80,  85,  90,  95, 100, 105, 110, 115, 120 };
const int   TACHO_FREQ[] = {  30,  35,  44,  55,  64,  73,  81,  90,  98, 106, 114, 123, 132, 142, 150, 158, 167, 174, 183, 194, 203, 211, 220 };

// --- Needle smoothing ---
// speedoTarget/tachoTarget = the frequency we WANT to reach (from calibration lookup)
// speedoCurrent/tachoCurrent = the frequency actually being sent right now,
// which glides gradually toward the target instead of jumping instantly
float speedoTarget  = 32;
float tachoTarget   = 30;
float speedoCurrent = 32;
float tachoCurrent  = 30;

// Smoothing strength — smaller number = smoother/slower glide, larger = snappier
// Try values between 0.05 (very smooth) and 0.3 (fast) to taste
const float NEEDLE_SMOOTH = 0.12;

// How often we nudge the needle toward its target, in milliseconds
// Smaller number = smoother motion, larger number = choppier
const int NEEDLE_UPDATE_MS = 20;
unsigned long lastNeedleUpdate = 0;

// --- Parsed sensor values ---
float cpu=0, gpu=0, cpuu=0, cpuc=0;
float ramu=0, ramt=0, gpuc=0;
float vram=0, vramm=0, ghot=0;

unsigned long lastScreenUpdate = 0;
const int SCREEN_UPDATE_MS = 500;

// --- Color theme ---
#define TEXT_COLOR 0xDA64   // #D94E22 — all numbers/labels/ring
#define BG_COLOR   0x60C1   // #61180D — background

// --- Needle control ---

void setSpeedoFreq(int freq) {
  if (freq <= 0) {
    TCCR1A = 0; TCCR1B = 0;
    digitalWrite(PIN_SPEEDO, LOW);
    return;
  }
  pinMode(PIN_SPEEDO, OUTPUT);
  TCCR1A = _BV(COM1A0);
  TCCR1B = _BV(WGM12) | _BV(CS12) | _BV(CS10);
  OCR1A  = (16000000UL / (2UL * 1024UL * (unsigned long)freq)) - 1;
}

void setTachoFreq(int freq) {
  if (freq <= 0) {
    TCCR2A = 0; TCCR2B = 0;
    digitalWrite(PIN_TACHO, LOW);
    return;
  }
  pinMode(PIN_TACHO, OUTPUT);
  TCCR2A = _BV(COM2A0) | _BV(WGM21);
  TCCR2B = _BV(CS22) | _BV(CS21) | _BV(CS20);
  int ocrVal = (16000000UL / (2UL * 1024UL * (unsigned long)freq)) - 1;
  if (ocrVal > 255) ocrVal = 255;
  if (ocrVal < 0)   ocrVal = 0;
  OCR2A = ocrVal;
}

int interpolate(float value, const float* cTable,
                const int* freqTable, int count) {
  if (value <= cTable[0])       return freqTable[0];
  if (value >= cTable[count-1]) return freqTable[count-1];
  for (int i = 0; i < count - 1; i++) {
    if (value >= cTable[i] && value <= cTable[i+1]) {
      float t = (value - cTable[i]) / (cTable[i+1] - cTable[i]);
      return (int)(freqTable[i] + t * (freqTable[i+1] - freqTable[i]));
    }
  }
  return freqTable[0];
}

// --- Screen drawing helpers ---

void drawRing(Arduino_GFX *scr) {
  scr->drawCircle(120, 120, 115, TEXT_COLOR);
  scr->drawCircle(120, 120, 114, TEXT_COLOR);
  scr->drawCircle(120, 120, 113, TEXT_COLOR);
}

void drawRow(Arduino_GFX *scr, int y, const char* label,
             const char* value, uint16_t color) {
  scr->fillRect(10, y - 2, 220, 28, BG_COLOR);

  scr->setTextSize(1);
  scr->setTextColor(TEXT_COLOR);
  int labelLen = strlen(label);
  int labelX = 120 - (labelLen * 6 / 2);
  scr->setCursor(labelX, y);
  scr->print(label);

  scr->setTextSize(2);
  scr->setTextColor(TEXT_COLOR);
  int valueLen = strlen(value);
  int valueX = 120 - (valueLen * 12 / 2);
  scr->setCursor(valueX, y + 12);
  scr->print(value);
}

// --- Screen updates ---

// Screen 1 (physical CS pin 4) — CPU info
void updateScreen1() {
  static float lastCpuc = -1, lastRamu = -1, lastRamt = -1, lastCpuu = -1;

  if (cpuc == lastCpuc && ramu == lastRamu &&
      ramt == lastRamt && cpuu == lastCpuu) return;

  char buf[20];

  sprintf(buf, "%dMHZ", (int)cpuc);
  drawRow(screen1, 55, "CPU CLOCK", buf, TEXT_COLOR);

  sprintf(buf, "%d/%dGB", (int)ramu, (int)ramt);
  drawRow(screen1, 110, "RAM USAGE", buf, TEXT_COLOR);

  sprintf(buf, "%d%%", (int)cpuu);
  drawRow(screen1, 165, "CPU LOAD", buf, TEXT_COLOR);

  lastCpuc = cpuc;
  lastRamu = ramu;
  lastRamt = ramt;
  lastCpuu = cpuu;
}

// Screen 2 (physical CS pin 9) — GPU info
void updateScreen2() {
  static float lastGpuc = -1, lastGhot = -1, lastVram = -1, lastVramm = -1;

  if (gpuc == lastGpuc && ghot == lastGhot &&
      vram == lastVram && vramm == lastVramm) return;

  char buf[20];

  sprintf(buf, "%dMHz", (int)gpuc);
  drawRow(screen2, 55, "GPU CLOCK", buf, TEXT_COLOR);

  sprintf(buf, "%d/%dMB", (int)vram, (int)vramm);
  drawRow(screen2, 110, "V-RAM USAGE", buf, TEXT_COLOR);

  sprintf(buf, "%dC", (int)ghot);
  drawRow(screen2, 165, "GPU HOTSPOT", buf, TEXT_COLOR);

  lastGpuc  = gpuc;
  lastGhot  = ghot;
  lastVram  = vram;
  lastVramm = vramm;
}

// --- Serial parsing ---

void parseSerial(String line) {
  auto getValue = [&](const char* key) -> float {
    String k = String(key) + "=";
    int idx = line.indexOf(k);
    if (idx == -1) return 0;
    idx += k.length();
    int end = line.indexOf(',', idx);
    if (end == -1) end = line.length();
    return line.substring(idx, end).toFloat();
  };

  cpu   = getValue("CPU");
  gpu   = getValue("GPU");
  cpuu  = getValue("CPUU");
  cpuc  = getValue("CPUC");
  ramu  = getValue("RAMU");
  ramt  = getValue("RAMT");
  gpuc  = getValue("GPUC");
  vram  = getValue("VRAM");
  vramm = getValue("VRAMM");
  ghot  = getValue("GHOT");
}

// --- Setup ---

void setup() {
  Serial.begin(9600);
  pinMode(PIN_SPEEDO, OUTPUT);
  pinMode(PIN_TACHO,  OUTPUT);

  screen1->begin();
  screen2->begin();

  // ══════════════════════════════════════════════════════════
  // SCREEN ROTATION — how to change it
  // setRotation() only accepts 4 values: 0, 1, 2, or 3
  //   0 = normal (0°)
  //   1 = rotated 90° clockwise
  //   2 = rotated 180° (upside down)
  //   3 = rotated 270° clockwise (= 90° counter-clockwise)
  //
  // A true arbitrary angle like 30° is NOT supported by this library —
  // the screen hardware itself only refreshes in these 4 fixed orientations.
  // If you truly need 30°, that requires manually rotating every
  // coordinate with trigonometry (sin/cos) before drawing, which is a
  // much bigger separate task — ask if you want to explore that later.
  //
  // To use 90° rotation right now, uncomment these two lines:
  // screen1->setRotation(1);
  // screen2->setRotation(1);
  // ══════════════════════════════════════════════════════════

  screen1->fillScreen(BG_COLOR);
  drawRing(screen1);
  screen1->setTextColor(TEXT_COLOR);
  screen1->setTextSize(2);
  screen1->setCursor(50, 110);
  screen1->print("CPU SCREEN");

  screen2->fillScreen(BG_COLOR);
  drawRing(screen2);
  screen2->setTextColor(TEXT_COLOR);
  screen2->setTextSize(2);
  screen2->setCursor(50, 110);
  screen2->print("GPU SCREEN");

  sweepTest();

  Serial.println("BMW Cluster Monitor ready.");
}

// --- Main loop ---

void loop() {
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();

    if (line.startsWith("CPU=")) {
      parseSerial(line);

      // Only update the TARGET here — actual needle motion happens
      // gradually below, independent of how often serial data arrives
      speedoTarget = interpolate(cpu,
                       SPEEDO_C, SPEEDO_FREQ, SPEEDO_POINTS);
      tachoTarget = interpolate(gpu,
                      TACHO_C, TACHO_FREQ, TACHO_POINTS);
    }
  }

  // --- Smoothly glide needles toward their targets ---
  unsigned long now = millis();
  if (now - lastNeedleUpdate >= NEEDLE_UPDATE_MS) {
    lastNeedleUpdate = now;

    speedoCurrent += (speedoTarget - speedoCurrent) * NEEDLE_SMOOTH;
    tachoCurrent  += (tachoTarget  - tachoCurrent)  * NEEDLE_SMOOTH;

    setSpeedoFreq((int)speedoCurrent);
    setTachoFreq((int)tachoCurrent);
  }

  // --- Update screens ---
  if (now - lastScreenUpdate >= SCREEN_UPDATE_MS) {
    lastScreenUpdate = now;
    updateScreen1(); // CPU
    updateScreen2(); // GPU
  }
}

// --- Startup needle sweep ---

void sweepTest() {
  // Reduced delay from 300ms to 120ms for a smoother, quicker sweep
  for (int i = 0; i < SPEEDO_POINTS; i++) {
    setSpeedoFreq(SPEEDO_FREQ[i]);
    if (i < TACHO_POINTS) setTachoFreq(TACHO_FREQ[i]);
    delay(120);
  }
  delay(400);
  for (int i = SPEEDO_POINTS - 1; i >= 0; i--) {
    setSpeedoFreq(SPEEDO_FREQ[i]);
    if (i < TACHO_POINTS) setTachoFreq(TACHO_FREQ[i]);
    delay(120);
  }
  setSpeedoFreq(SPEEDO_FREQ[0]);
  setTachoFreq(TACHO_FREQ[0]);
  speedoCurrent = SPEEDO_FREQ[0];
  tachoCurrent  = TACHO_FREQ[0];
}