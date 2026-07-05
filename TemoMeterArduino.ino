#include <Arduino_GFX_Library.h>

// --- Needle pins ---
const int PIN_SPEEDO = 11;
const int PIN_TACHO  = 10;

// --- Screen 1 (GPU info) ---
#define TFT1_CS   4
#define TFT1_DC   7
#define TFT1_RST  8

// --- Screen 2 (CPU info) ---
#define TFT2_CS   9
#define TFT2_DC   6
#define TFT2_RST  5

Arduino_DataBus *bus1 = new Arduino_HWSPI(TFT1_DC, TFT1_CS, SCK, MOSI);
Arduino_DataBus *bus2 = new Arduino_HWSPI(TFT2_DC, TFT2_CS, SCK, MOSI);

Arduino_GFX *gpuScreen = new Arduino_GC9A01(bus1, TFT1_RST, 0, true);
Arduino_GFX *cpuScreen = new Arduino_GC9A01(bus2, TFT2_RST, 0, true);

// --- Calibration tables ---
const int SPEEDO_POINTS   = 12;
const float SPEEDO_C[]    = {  10,  20,  30,  40,  50,  60,  70,  80,  90, 100, 110, 120 };
const int   SPEEDO_FREQ[] = {  16,  30,  43,  56,  67,  78,  89, 102, 113, 125, 138, 152 };

const int TACHO_POINTS   = 14;
const float TACHO_C[]    = {  11,  20,  30,  35,  40,  50,  60,  70,  75,  80,  90, 100, 110, 120 };
const int   TACHO_FREQ[] = {  15,  16,  25,  30,  35,  43,  52,  65,  70,  73,  78,  87,  96, 106 };

const float TACHO_DEAD_ZONE = 2.0;

int speedoTarget = 16;  
int tachoTarget  = 15; 
float lastGpu    = 0;
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

// Draws the ring border once
void drawRing(Arduino_GFX *scr) {
  scr->drawCircle(120, 120, 115, TEXT_COLOR);
  scr->drawCircle(120, 120, 114, TEXT_COLOR);
  scr->drawCircle(120, 120, 113, TEXT_COLOR);
}

// Draws one row: small label above, big value below, vertically centered at y
// Clears only this row's rectangle first — no full-screen flash
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

// --- Screen updates (only redraw a row when its value actually changes) ---

void updateGpuScreen() {
  static float lastGpuc = -1, lastGhot = -1, lastVram = -1, lastVramm = -1;

  if (gpuc == lastGpuc && ghot == lastGhot &&
      vram == lastVram && vramm == lastVramm) return;

  char buf[20];

  sprintf(buf, "%dMHz", (int)gpuc);
  drawRow(gpuScreen, 55, "GPU CLOCK", buf, TEXT_COLOR);

  sprintf(buf, "%d/%dMB", (int)vram, (int)vramm);
  drawRow(gpuScreen, 110, "V-RAM USAGE", buf, TEXT_COLOR);

  sprintf(buf, "%dC", (int)ghot);
  drawRow(gpuScreen, 165, "GPU HOTSPOT", buf, TEXT_COLOR);

  lastGpuc  = gpuc;
  lastGhot  = ghot;
  lastVram  = vram;
  lastVramm = vramm;
}

void updateCpuScreen() {
  static float lastRamu = -1, lastRamt = -1, lastCpuu = -1, lastCpuc = -1;

  if (ramu == lastRamu && ramt == lastRamt &&
      cpuu == lastCpuu && cpuc == lastCpuc) return;

  char buf[20];

  sprintf(buf, "%dMHZ", (int)cpuc);
  drawRow(cpuScreen, 55, "CPU CLOCK", buf, TEXT_COLOR);

  sprintf(buf, "%d/%dGB", (int)ramu, (int)ramt);
  drawRow(cpuScreen, 110, "RAM USAGE", buf, TEXT_COLOR);

  sprintf(buf, "%d%%", (int)cpuu);
  drawRow(cpuScreen, 165, "CPU LOAD", buf, TEXT_COLOR);

  lastRamu = ramu;
  lastRamt = ramt;
  lastCpuu = cpuu;
  lastCpuc = cpuc;
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

  gpuScreen->begin();
  cpuScreen->begin();

  // --- For future 90-degree rotation, uncomment these two lines ---
  // gpuScreen->setRotation(1);
  // cpuScreen->setRotation(1);

  gpuScreen->fillScreen(BG_COLOR);
  drawRing(gpuScreen);
  gpuScreen->setTextColor(TEXT_COLOR);
  gpuScreen->setTextSize(2);
  gpuScreen->setCursor(50, 110);
  gpuScreen->print("GPU SCREEN");

  cpuScreen->fillScreen(BG_COLOR);
  drawRing(cpuScreen);
  cpuScreen->setTextColor(TEXT_COLOR);
  cpuScreen->setTextSize(2);
  cpuScreen->setCursor(50, 110);
  cpuScreen->print("CPU SCREEN");

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

      speedoTarget = interpolate(cpu,
                       SPEEDO_C, SPEEDO_FREQ, SPEEDO_POINTS);
      setSpeedoFreq(speedoTarget);

      if (abs(gpu - lastGpu) >= TACHO_DEAD_ZONE) {
        tachoTarget = interpolate(gpu,
                        TACHO_C, TACHO_FREQ, TACHO_POINTS);
        setTachoFreq(tachoTarget);
        lastGpu = gpu;
      }
    }
  }

  unsigned long now = millis();
  if (now - lastScreenUpdate >= SCREEN_UPDATE_MS) {
    lastScreenUpdate = now;
    updateGpuScreen();
    updateCpuScreen();
  }
}

// --- Startup needle sweep ---

void sweepTest() {
  for (int i = 0; i < SPEEDO_POINTS; i++) {
    setSpeedoFreq(SPEEDO_FREQ[i]);
    if (i < TACHO_POINTS) setTachoFreq(TACHO_FREQ[i]);
    delay(300);
  }
  delay(500);
  for (int i = SPEEDO_POINTS - 1; i >= 0; i--) {
    setSpeedoFreq(SPEEDO_FREQ[i]);
    if (i < TACHO_POINTS) setTachoFreq(TACHO_FREQ[i]);
    delay(300);
  }
  setSpeedoFreq(SPEEDO_FREQ[0]);
  setTachoFreq(TACHO_FREQ[0]);
}
