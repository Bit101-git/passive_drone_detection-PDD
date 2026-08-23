#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <RF24.h>
#include <math.h>
#include <string.h>

static const int PIN_RX5808_DATA = 23;
static const int PIN_RX5808_CLK  = 18;
static const int PIN_RX5808_CS   = 5;
static const int PIN_RX5808_RSSI = 34;

static const int PIN_VIDEO_SDA = 21;
static const int PIN_VIDEO_SCL = 22;
static const int PIN_NRF_OLED_SDA = 16;
static const int PIN_NRF_OLED_SCL = 17;
TwoWire I2C_VIDEO = TwoWire(0);
TwoWire I2C_NRF_OLED = TwoWire(1);

static const int PIN_LED_CAL   = 12;
static const int PIN_LED_VIDEO = 14;
static const int PIN_LED_NRF   = 13;
static const int PIN_BUZZER    = 15;

static const int PIN_NRF_SCK  = 25;
static const int PIN_NRF_MISO = 19;
static const int PIN_NRF_MOSI = 26;
static const int PIN_NRF_CE   = 27;
static const int PIN_NRF_CSN  = 33;
SPIClass NRF_SPI(HSPI);
RF24 radio(PIN_NRF_CE, PIN_NRF_CSN);

static const char* BAND_NAMES[9] = {"A","B","E","F","R","U","O","L","H"};
static const uint16_t FREQS[9][8] = {
  {5865,5845,5825,5805,5785,5765,5745,5725},
  {5733,5752,5771,5790,5809,5828,5847,5866},
  {5705,5685,5665,5645,5885,5905,5925,5945},
  {5740,5760,5780,5800,5820,5840,5860,5880},
  {5658,5695,5732,5769,5806,5843,5880,5917},
  {5325,5348,5366,5384,5402,5420,5438,5456},
  {5474,5492,5510,5528,5546,5564,5582,5600},
  {5333,5373,5413,5453,5493,5533,5573,5613},
  {5653,5693,5733,5773,5813,5853,5893,5933}
};

static const uint32_t SCAN_INTERVAL_MS = 1200;
static const uint16_t TUNE_SETTLE_MS   = 25;

static const uint32_t AMBIENT_CAL_MS = 3000;

static const int      RSSI_WINDOW_SAMPLES = 24;
static const uint16_t RSSI_WINDOW_DELAY_MS = 3;

static const int      RSSI_MARGIN       = 220;
static const int      RSSI_STDDEV_MAX   = 120;

static const int      RSSI_HYST         = 80;

static const uint32_t NRF_SCAN_INTERVAL_MS = 40;
static const uint8_t  NRF_FIRST_CHANNEL = 0;
static const uint8_t  NRF_LAST_CHANNEL  = 83;
static const uint8_t  NRF_CHANNEL_COUNT = 84;
static const uint8_t  NRF_SAMPLES_PER_CHANNEL = 2;
static const uint8_t  NRF_AMBIENT_SAMPLES_PER_STEP = 3;
static const uint8_t  NRF_AMBIENT_HITS_TO_IGNORE = 1;
static const uint8_t  FHSS_MIN_HITS = 5;
static const uint8_t  FHSS_MIN_SPREAD = 18;
static const uint8_t  FHSS_CONFIRM_SCANS = 3;
static const uint32_t FHSS_HOLD_MS = 1000;

static const uint8_t VIDEO_PERSIST_N_DEFAULT = 3;

struct VideoLock {
  bool locked = false;
  int  band = -1;
  int  ch   = -1;
  uint16_t freq = 0;
  int mean = 0;
  int stddev = 0;
} g_video;

static int g_rssiBaseline = 0;
static int g_rssiThresholdOn  = 0;
static int g_rssiThresholdOff = 0;
static int g_videoNoiseFloor[9][8];
static bool g_videoNoiseReady = false;

static uint8_t  g_videoScore = 0;
static char     g_videoClass[24] = "NOISE/INTERFERENCE";
static uint8_t  g_videoPersistCount = 0;
static uint8_t  g_videoPersistN = VIDEO_PERSIST_N_DEFAULT;

static bool g_videoTarget = false;

static bool g_nrfOk = false;
static bool g_fhssTarget = false;
static uint8_t g_rfHits = 0;
static uint8_t g_rfFirstHit = 0;
static uint8_t g_rfLastHit = 0;
static uint8_t g_fhssConfirm = 0;
static uint32_t g_lastFhssMs = 0;
static uint8_t g_nrfAmbientHits[NRF_CHANNEL_COUNT];
static bool g_nrfAmbientBusy[NRF_CHANNEL_COUNT];
static uint8_t g_nrfAmbientBusyCount = 0;

static int readRSSI()
{
  return analogRead(PIN_RX5808_RSSI);
}

static void computeMeanStddev(int &meanOut, int &stddevOut)
{
  long sum = 0;
  long sumsq = 0;

  for (int i = 0; i < RSSI_WINDOW_SAMPLES; i++) {
    int v = readRSSI();
    sum += v;
    sumsq += (long)v * (long)v;
    delay(RSSI_WINDOW_DELAY_MS);
  }

  long n = RSSI_WINDOW_SAMPLES;
  long mean = sum / n;
  long ex2 = sumsq / n;
  long var = ex2 - mean * mean;
  if (var < 0) var = 0;

  int stddev = (int)sqrt((double)var);

  meanOut = (int)mean;
  stddevOut = stddev;
}

static void rx5808WriteRegister(uint8_t addr, uint32_t data20)
{
  digitalWrite(PIN_RX5808_CS, LOW);
  delayMicroseconds(2);

  uint32_t packet = 0;
  packet |= ((uint32_t)(addr & 0x0F) << 21);
  packet |= ((uint32_t)0 << 20);
  packet |= (data20 & 0xFFFFF);

  for (int i = 24; i >= 0; --i) {
    digitalWrite(PIN_RX5808_CLK, LOW);
    digitalWrite(PIN_RX5808_DATA, (packet >> i) & 0x01);
    delayMicroseconds(1);
    digitalWrite(PIN_RX5808_CLK, HIGH);
    delayMicroseconds(1);
  }

  digitalWrite(PIN_RX5808_CLK, LOW);
  digitalWrite(PIN_RX5808_CS, HIGH);
  delayMicroseconds(2);
}

static void rx5808SetFrequencyMHz(uint16_t freqMHz)
{
  uint16_t divider = (uint16_t)((freqMHz - 479) / 2);
  rx5808WriteRegister(0x01, divider);
  delay(TUNE_SETTLE_MS);
}

static int getVideoNoiseFloor(int band, int ch)
{
  if (g_videoNoiseReady && band >= 0 && band < 9 && ch >= 0 && ch < 8) {
    return g_videoNoiseFloor[band][ch];
  }
  return g_rssiBaseline;
}

static int getVideoThresholdOn(int band, int ch)
{
  return getVideoNoiseFloor(band, ch) + RSSI_MARGIN;
}

static int getVideoThresholdOff(int band, int ch)
{
  return getVideoThresholdOn(band, ch) - RSSI_HYST;
}

static void scanBestVideo(int &bestBand, int &bestCh, uint16_t &bestFreq, int &bestMean, int &bestStd)
{
  bestBand = -1; bestCh = -1; bestFreq = 0;
  bestMean = -1; bestStd = 99999;
  int bestSignal = -99999;

  for (int b = 0; b < 9; b++) {
    for (int c = 0; c < 8; c++) {
      uint16_t f = FREQS[b][c];
      rx5808SetFrequencyMHz(f);

      int mean = 0, sd = 0;
      computeMeanStddev(mean, sd);

      int signalOverNoise = mean - getVideoNoiseFloor(b, c);
      if (signalOverNoise > bestSignal) {
        bestSignal = signalOverNoise;
        bestMean = mean;
        bestStd  = sd;
        bestBand = b;
        bestCh   = c;
        bestFreq = f;
      }
    }
  }
}

static bool isVideoPresent(int band, int ch, int mean, int stddev, bool currentlyLocked)
{
  int onTh  = getVideoThresholdOn(band, ch);
  int offTh = getVideoThresholdOff(band, ch);

  bool strong = mean >= (currentlyLocked ? offTh : onTh);
  bool stable = stddev <= RSSI_STDDEV_MAX;

  bool veryStrong = mean >= (onTh + 250);

  return (strong && stable) || (veryStrong && strong);
}

static inline int clampi(int v, int lo, int hi) { return (v < lo) ? lo : (v > hi) ? hi : v; }

static void updateVideoPersistence(bool present)
{
  if (present) {
    if (g_videoPersistCount < 255) g_videoPersistCount++;
  } else {
    g_videoPersistCount = 0;
  }
}

static uint8_t computeVideoConfidenceScore(int band, int ch, int mean, int stddev)
{
  const int baseline = getVideoNoiseFloor(band, ch);
  const int onTh = getVideoThresholdOn(band, ch);

  int denom = (onTh - baseline);
  if (denom <= 0) denom = 1;
  int num = (mean - baseline);
  num = clampi(num, 0, denom);
  int intensity = (num * 100) / denom;

  int stability = 0;
  if (stddev < 0) stddev = 0;
  if (stddev >= RSSI_STDDEV_MAX) stability = 0;
  else stability = ((RSSI_STDDEV_MAX - stddev) * 100) / RSSI_STDDEV_MAX;

  uint8_t pc = g_videoPersistCount;
  uint8_t N = g_videoPersistN;
  if (N == 0) N = 1;
  if (pc > N) pc = N;
  int persistence = (pc * 100) / N;

  int score = (intensity * 50 + stability * 30 + persistence * 20) / 100;
  score = clampi(score, 0, 100);
  return (uint8_t)score;
}

static const char* classifySignal(int band, int ch, int mean, int stddev, uint8_t score)
{
  const int onTh = getVideoThresholdOn(band, ch);

  if (mean >= (onTh + 250) && stddev > RSSI_STDDEV_MAX) return "STRONG_UNSTABLE";
  if (mean < onTh || score < 40) return "NOISE/INTERFERENCE";

  bool stable = stddev <= RSSI_STDDEV_MAX;
  bool present = mean >= onTh;
  if (present && stable && score >= 70) return "VALID_VIDEO";

  return "NOISE/INTERFERENCE";
}

static bool initNRF24()
{
  NRF_SPI.begin(PIN_NRF_SCK, PIN_NRF_MISO, PIN_NRF_MOSI, PIN_NRF_CSN);
  if (!radio.begin(&NRF_SPI)) return false;
  radio.setAutoAck(false);
  radio.setDataRate(RF24_1MBPS);
  radio.setPALevel(RF24_PA_MIN);
  radio.disableCRC();
  radio.stopListening();
  return true;
}

static bool channelHasEnergy(uint8_t channel)
{
  radio.setChannel(channel);
  radio.startListening();
  delayMicroseconds(180);
  bool detected = radio.testRPD();
  radio.stopListening();
  return detected;
}

static void scanNRF24FHSS()
{
  if (!g_nrfOk) {
    g_fhssTarget = false;
    return;
  }

  uint8_t hits = 0;
  uint8_t first = 255;
  uint8_t last = 0;

  for (uint8_t ch = NRF_FIRST_CHANNEL; ch <= NRF_LAST_CHANNEL; ch++) {
    bool active = false;
    for (uint8_t n = 0; n < NRF_SAMPLES_PER_CHANNEL; n++) {
      if (channelHasEnergy(ch)) active = true;
    }
    if (active) {
      if (g_nrfAmbientBusy[ch - NRF_FIRST_CHANNEL]) continue;
      hits++;
      if (first == 255) first = ch;
      last = ch;
    }
  }

  g_rfHits = hits;
  g_rfFirstHit = (first == 255) ? 0 : first;
  g_rfLastHit = last;
  uint8_t spread = (first == 255) ? 0 : (last - first);
  bool hoppingPattern = hits >= FHSS_MIN_HITS && spread >= FHSS_MIN_SPREAD;

  if (hoppingPattern) {
    if (g_fhssConfirm < FHSS_CONFIRM_SCANS) g_fhssConfirm++;
    if (g_fhssConfirm >= FHSS_CONFIRM_SCANS) {
      g_lastFhssMs = millis();
      g_fhssTarget = true;
    }
  } else if (g_fhssConfirm > 0) {
    g_fhssConfirm--;
  }

  if (millis() - g_lastFhssMs > FHSS_HOLD_MS) g_fhssTarget = false;
}

static bool g_buzzerOn = false;
static uint32_t g_buzzerUntilMs = 0;
static const int BUZZER_FREQ = 2200;

static void buzzerInit()
{
  ledcAttach(PIN_BUZZER, BUZZER_FREQ, 8);
  ledcWriteTone(PIN_BUZZER, 0);
  ledcWrite(PIN_BUZZER, 0);
}

static void buzzerStart(uint32_t durationMs)
{
  uint32_t now = millis();
  g_buzzerUntilMs = now + durationMs;

  if (!g_buzzerOn) {
    g_buzzerOn = true;
    ledcWriteTone(PIN_BUZZER, BUZZER_FREQ);
    ledcWrite(PIN_BUZZER, 128);
  }
}

static void buzzerUpdate()
{
  if (!g_buzzerOn) return;
  uint32_t now = millis();
  if ((int32_t)(now - g_buzzerUntilMs) >= 0) {
    ledcWriteTone(PIN_BUZZER, 0);
    ledcWrite(PIN_BUZZER, 0);
    g_buzzerOn = false;
  }
}

static bool g_calibrating = false;

static void updateLedStateMachine()
{
  static uint32_t lastBlinkMs = 0;
  static bool blinkPhase = false;

  uint32_t now = millis();
  if (now - lastBlinkMs >= 250) {
    lastBlinkMs = now;
    blinkPhase = !blinkPhase;
  }

  if (g_calibrating) digitalWrite(PIN_LED_CAL, blinkPhase ? HIGH : LOW);
  else digitalWrite(PIN_LED_CAL, LOW);

  digitalWrite(PIN_LED_VIDEO, g_videoTarget ? HIGH : LOW);
  digitalWrite(PIN_LED_NRF, g_fhssTarget ? HIGH : LOW);
}

static void testRedLedsOnStartup()
{
  digitalWrite(PIN_LED_VIDEO, HIGH);
  digitalWrite(PIN_LED_NRF, HIGH);
  delay(350);
  digitalWrite(PIN_LED_VIDEO, LOW);
  digitalWrite(PIN_LED_NRF, LOW);
}

static void calibrateRSSI()
{
  Serial.println("Detecting ambient noise at 5.8 GHz and 2.4 GHz... Please wait.");

  static long videoSums[9][8];
  static uint8_t videoCounts[9][8];
  memset(videoSums, 0, sizeof(videoSums));
  memset(videoCounts, 0, sizeof(videoCounts));
  memset(g_nrfAmbientHits, 0, sizeof(g_nrfAmbientHits));
  memset(g_nrfAmbientBusy, 0, sizeof(g_nrfAmbientBusy));

  g_calibrating = true;
  g_videoTarget = false;
  g_fhssTarget = false;
  g_fhssConfirm = 0;
  g_lastFhssMs = 0;

  long videoTotal = 0;
  int videoTotalCount = 0;
  uint8_t band = 0;
  uint8_t ch = 0;
  uint8_t rfCh = NRF_FIRST_CHANNEL;
  uint32_t startMs = millis();

  while (millis() - startMs < AMBIENT_CAL_MS) {
    rx5808SetFrequencyMHz(FREQS[band][ch]);

    for (uint8_t n = 0; n < 3; n++) {
      int v = readRSSI();
      videoSums[band][ch] += v;
      if (videoCounts[band][ch] < 250) videoCounts[band][ch]++;
      videoTotal += v;
      videoTotalCount++;
    }

    if (g_nrfOk) {
      for (uint8_t n = 0; n < NRF_AMBIENT_SAMPLES_PER_STEP; n++) {
        if (channelHasEnergy(rfCh)) {
          uint8_t idx = rfCh - NRF_FIRST_CHANNEL;
          if (g_nrfAmbientHits[idx] < 255) g_nrfAmbientHits[idx]++;
        }
        rfCh++;
        if (rfCh > NRF_LAST_CHANNEL) rfCh = NRF_FIRST_CHANNEL;
      }
    }

    ch++;
    if (ch >= 8) {
      ch = 0;
      band++;
      if (band >= 9) band = 0;
    }

    updateLedStateMachine();
    buzzerUpdate();
  }

  g_rssiBaseline = (videoTotalCount > 0) ? (int)(videoTotal / videoTotalCount) : 0;
  g_rssiThresholdOn  = g_rssiBaseline + RSSI_MARGIN;
  g_rssiThresholdOff = g_rssiThresholdOn - RSSI_HYST;
  g_videoNoiseReady = true;

  for (int b = 0; b < 9; b++) {
    for (int c = 0; c < 8; c++) {
      if (videoCounts[b][c] > 0) {
        g_videoNoiseFloor[b][c] = (int)(videoSums[b][c] / videoCounts[b][c]);
      } else {
        g_videoNoiseFloor[b][c] = g_rssiBaseline;
      }
    }
  }

  g_nrfAmbientBusyCount = 0;
  for (uint8_t i = 0; i < NRF_CHANNEL_COUNT; i++) {
    g_nrfAmbientBusy[i] = g_nrfAmbientHits[i] >= NRF_AMBIENT_HITS_TO_IGNORE;
    if (g_nrfAmbientBusy[i]) g_nrfAmbientBusyCount++;
  }

  g_calibrating = false;
  updateLedStateMachine();

  Serial.print("RSSI baseline="); Serial.print(g_rssiBaseline);
  Serial.print(" thresholdOn="); Serial.print(g_rssiThresholdOn);
  Serial.print(" thresholdOff="); Serial.println(g_rssiThresholdOff);
  Serial.print("NRF24 ambient-noise channels ignored=");
  Serial.println(g_nrfAmbientBusyCount);
}

static const uint8_t OLED_ADDR_VIDEO = 0x3C;
static const uint8_t OLED_ADDR_NRF   = 0x3C;
static const uint32_t OLED_I2C_HZ = 100000;

static const uint8_t OLED_COL_OFFSET_VIDEO = 0;
static const uint8_t OLED_COL_OFFSET_NRF   = 0;

static uint8_t g_oledBuf[1024];

static const uint8_t Font5x7[95][5] PROGMEM = {
  {0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x5F,0x00,0x00},{0x00,0x07,0x00,0x07,0x00},{0x14,0x7F,0x14,0x7F,0x14},
  {0x24,0x2A,0x7F,0x2A,0x12},{0x23,0x13,0x08,0x64,0x62},{0x36,0x49,0x55,0x22,0x50},{0x00,0x05,0x03,0x00,0x00},
  {0x00,0x1C,0x22,0x41,0x00},{0x00,0x41,0x22,0x1C,0x00},{0x14,0x08,0x3E,0x08,0x14},{0x08,0x08,0x3E,0x08,0x08},
  {0x00,0x50,0x30,0x00,0x00},{0x08,0x08,0x08,0x08,0x08},{0x00,0x60,0x60,0x00,0x00},{0x20,0x10,0x08,0x04,0x02},
  {0x3E,0x51,0x49,0x45,0x3E},{0x00,0x42,0x7F,0x40,0x00},{0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4B,0x31},
  {0x18,0x14,0x12,0x7F,0x10},{0x27,0x45,0x45,0x45,0x39},{0x3C,0x4A,0x49,0x49,0x30},{0x01,0x71,0x09,0x05,0x03},
  {0x36,0x49,0x49,0x49,0x36},{0x06,0x49,0x49,0x29,0x1E},{0x00,0x36,0x36,0x00,0x00},{0x00,0x56,0x36,0x00,0x00},
  {0x08,0x14,0x22,0x41,0x00},{0x14,0x14,0x14,0x14,0x14},{0x00,0x41,0x22,0x14,0x08},{0x02,0x01,0x51,0x09,0x06},
  {0x32,0x49,0x79,0x41,0x3E},{0x7E,0x11,0x11,0x11,0x7E},{0x7F,0x49,0x49,0x49,0x36},{0x3E,0x41,0x41,0x41,0x22},
  {0x7F,0x41,0x41,0x22,0x1C},{0x7F,0x49,0x49,0x49,0x41},{0x7F,0x09,0x09,0x09,0x01},{0x3E,0x41,0x49,0x49,0x7A},
  {0x7F,0x08,0x08,0x08,0x7F},{0x00,0x41,0x7F,0x41,0x00},{0x20,0x40,0x41,0x3F,0x01},{0x7F,0x08,0x14,0x22,0x41},
  {0x7F,0x40,0x40,0x40,0x40},{0x7F,0x02,0x0C,0x02,0x7F},{0x7F,0x04,0x08,0x10,0x7F},{0x3E,0x41,0x41,0x41,0x3E},
  {0x7F,0x09,0x09,0x09,0x06},{0x3E,0x41,0x51,0x21,0x5E},{0x7F,0x09,0x19,0x29,0x46},{0x46,0x49,0x49,0x49,0x31},
  {0x01,0x01,0x7F,0x01,0x01},{0x3F,0x40,0x40,0x40,0x3F},{0x1F,0x20,0x40,0x20,0x1F},{0x3F,0x40,0x38,0x40,0x3F},
  {0x63,0x14,0x08,0x14,0x63},{0x07,0x08,0x70,0x08,0x07},{0x61,0x51,0x49,0x45,0x43},{0x00,0x7F,0x41,0x41,0x00},
  {0x02,0x04,0x08,0x10,0x20},{0x00,0x41,0x41,0x7F,0x00},{0x04,0x02,0x01,0x02,0x04},{0x40,0x40,0x40,0x40,0x40},
  {0x00,0x01,0x02,0x04,0x00},{0x20,0x54,0x54,0x54,0x78},{0x7F,0x48,0x44,0x44,0x38},{0x38,0x44,0x44,0x44,0x20},
  {0x38,0x44,0x44,0x48,0x7F},{0x38,0x54,0x54,0x54,0x18},{0x08,0x7E,0x09,0x01,0x02},{0x0C,0x52,0x52,0x52,0x3E},{0x7F,0x08,0x04,0x04,0x78},
  {0x00,0x44,0x7D,0x40,0x00},{0x20,0x40,0x44,0x3D,0x00},{0x7F,0x10,0x28,0x44,0x00},{0x00,0x41,0x7F,0x40,0x00},
  {0x7C,0x04,0x18,0x04,0x78},{0x7C,0x08,0x04,0x04,0x78},{0x38,0x44,0x44,0x44,0x38},{0x7C,0x14,0x14,0x14,0x08},
  {0x08,0x14,0x14,0x18,0x7C},{0x7C,0x08,0x04,0x04,0x08},{0x48,0x54,0x54,0x54,0x20},{0x04,0x3F,0x44,0x40,0x20},
  {0x3C,0x40,0x40,0x20,0x7C},{0x1C,0x20,0x40,0x20,0x1C},{0x3C,0x40,0x30,0x40,0x3C},{0x44,0x28,0x10,0x28,0x44},
  {0x0C,0x50,0x50,0x50,0x3C},{0x44,0x64,0x54,0x4C,0x44},{0x00,0x08,0x36,0x41,0x00},{0x00,0x00,0x7F,0x00,0x00},
  {0x00,0x41,0x36,0x08,0x00},{0x10,0x08,0x08,0x10,0x08}
};

static void oledCmd(TwoWire &bus, uint8_t addr, uint8_t cmd)
{
  bus.beginTransmission(addr);
  bus.write(0x00);
  bus.write(cmd);
  bus.endTransmission();
}

static void oledDataStart(TwoWire &bus, uint8_t addr)
{
  bus.beginTransmission(addr);
  bus.write(0x40);
}

static void oledDataEnd(TwoWire &bus)
{
  bus.endTransmission();
}

static void oledInit(TwoWire &bus, uint8_t addr)
{
  oledCmd(bus, addr, 0xAE);
  oledCmd(bus, addr, 0xD5); oledCmd(bus, addr, 0x80);
  oledCmd(bus, addr, 0xA8); oledCmd(bus, addr, 0x3F);
  oledCmd(bus, addr, 0xD3); oledCmd(bus, addr, 0x00);
  oledCmd(bus, addr, 0x40);
  oledCmd(bus, addr, 0x8D); oledCmd(bus, addr, 0x14);

  oledCmd(bus, addr, 0x20); oledCmd(bus, addr, 0x02);
  oledCmd(bus, addr, 0xA1);
  oledCmd(bus, addr, 0xC8);
  oledCmd(bus, addr, 0xDA); oledCmd(bus, addr, 0x12);
  oledCmd(bus, addr, 0x81); oledCmd(bus, addr, 0x7F);
  oledCmd(bus, addr, 0xD9); oledCmd(bus, addr, 0xF1);
  oledCmd(bus, addr, 0xDB); oledCmd(bus, addr, 0x40);
  oledCmd(bus, addr, 0xA4);
  oledCmd(bus, addr, 0xA6);
  oledCmd(bus, addr, 0xAF);
}

static void oledClearBuf() { memset(g_oledBuf, 0, sizeof(g_oledBuf)); }

static void oledSetPixel(int x, int y, bool on)
{
  if (x < 0 || x >= 128 || y < 0 || y >= 64) return;
  uint16_t idx = (y >> 3) * 128 + x;
  uint8_t bit = 1 << (y & 7);
  if (on) g_oledBuf[idx] |= bit;
  else    g_oledBuf[idx] &= ~bit;
}

static void oledDrawChar(int x, int y, uint8_t ch)
{
  if (ch < 32 || ch > 126) ch = '?';
  const uint8_t* glyph = &Font5x7[ch - 32][0];
  for (int col = 0; col < 5; col++) {
    uint8_t bits = pgm_read_byte(&glyph[col]);
    for (int row = 0; row < 7; row++) {
      oledSetPixel(x + col, y + row, (bits & (1 << row)) != 0);
    }
  }
}

static char oledReadAscii(const char* &s)
{
  uint8_t c = (uint8_t)*s++;
  if (c >= 32 && c <= 126) return (char)c;

  if (c == 0xC3 && *s) {
    uint8_t d = (uint8_t)*s++;
    switch (d) {
      case 0x81: return 'A';
      case 0x89: return 'E';
      case 0x8D: return 'I';
      case 0x93: return 'O';
      case 0x9A: return 'U';
      case 0x91: return 'N';
      case 0xA1: return 'a';
      case 0xA9: return 'e';
      case 0xAD: return 'i';
      case 0xB3: return 'o';
      case 0xBA: return 'u';
      case 0xB1: return 'n';
      case 0xBC: return 'u';
      default: return '?';
    }
  }

  if (c == 0 || c == '\r' || c == '\n' || c == '\t') return ' ';
  return '?';
}

static void oledDrawText(int x, int y, const char* s)
{
  int cx = x;
  while (*s && cx <= 122) {
    oledDrawChar(cx, y, (uint8_t)oledReadAscii(s));
    cx += 6;
  }
}

static void oledFlush(TwoWire &bus, uint8_t addr, uint8_t colOffset)
{
  for (uint8_t page = 0; page < 8; page++) {
    oledCmd(bus, addr, (uint8_t)(0xB0 | page));
    oledCmd(bus, addr, (uint8_t)(0x00 | (colOffset & 0x0F)));
    oledCmd(bus, addr, (uint8_t)(0x10 | ((colOffset >> 4) & 0x0F)));

    uint16_t base = (uint16_t)page * 128;
    uint8_t col = 0;
    while (col < 128) {
      oledDataStart(bus, addr);
      for (uint8_t k = 0; k < 16 && col < 128; k++, col++) {
        bus.write(g_oledBuf[base + col]);
      }
      oledDataEnd(bus);
    }
  }
}

static void drawAmbientCalibrationScreen()
{
  oledClearBuf();
  oledDrawText(0, 18, "DETECTING NOISE...");
  oledDrawText(0, 34, "PLEASE WAIT.");
}

static void drawVideoScreen()
{
  oledClearBuf();
  oledDrawText(0, 0,  "VIDEO MONITOR");
  oledDrawText(0, 10, "STATUS:");
  oledDrawText(48, 10, g_videoTarget ? "TARGET" : "SEARCHING");

  char line[32];
  if (g_video.band >= 0 && g_video.ch >= 0) {
    snprintf(line, sizeof(line), "B:%s CH:%d %4uMHz", BAND_NAMES[g_video.band], g_video.ch + 1, g_video.freq);
  } else {
    snprintf(line, sizeof(line), "B:-- CH:-- ----MHz");
  }
  oledDrawText(0, 22, line);

  snprintf(line, sizeof(line), "MEAN:%4d SD:%3d", g_video.mean, g_video.stddev);
  oledDrawText(0, 34, line);

  snprintf(line, sizeof(line), "SCORE:%3u", (unsigned)g_videoScore);
  oledDrawText(0, 46, line);

  oledDrawText(0, 56, g_videoClass);
}

static void drawNrfScreen()
{
  oledClearBuf();
  oledDrawText(0, 0, "2.4 GHz / FHSS");
  oledDrawText(0, 10, "NRF24:");
  oledDrawText(42, 10, g_nrfOk ? "OK" : "ERROR");
  oledDrawText(0, 22, "PATTERN:");
  oledDrawText(42, 22, g_fhssTarget ? ":ELRS?" : ":NO");

  char line[32];
  snprintf(line, sizeof(line), "HITS:%u CONF:%u/%u", g_rfHits,
           g_fhssConfirm, FHSS_CONFIRM_SCANS);
  oledDrawText(0, 34, line);
  snprintf(line, sizeof(line), "%u-%u MHz", 2400 + g_rfFirstHit,
           2400 + g_rfLastHit);
  oledDrawText(0, 46, line);
  oledDrawText(0, 56, "FHSS INCONCLUSIVE.");
}

void setup()
{
  Serial.begin(115200);
  delay(200);

  pinMode(PIN_RX5808_DATA, OUTPUT);
  pinMode(PIN_RX5808_CLK, OUTPUT);
  pinMode(PIN_RX5808_CS, OUTPUT);
  digitalWrite(PIN_RX5808_CS, HIGH);
  digitalWrite(PIN_RX5808_CLK, LOW);

  pinMode(PIN_LED_CAL, OUTPUT);
  pinMode(PIN_LED_VIDEO, OUTPUT);
  pinMode(PIN_LED_NRF, OUTPUT);
  digitalWrite(PIN_LED_CAL, LOW);
  digitalWrite(PIN_LED_VIDEO, LOW);
  digitalWrite(PIN_LED_NRF, LOW);
  testRedLedsOnStartup();

  buzzerInit();
  buzzerStart(180);

  analogReadResolution(12);

  I2C_VIDEO.begin(PIN_VIDEO_SDA, PIN_VIDEO_SCL, OLED_I2C_HZ);
  I2C_NRF_OLED.begin(PIN_NRF_OLED_SDA, PIN_NRF_OLED_SCL, OLED_I2C_HZ);
  oledInit(I2C_VIDEO, OLED_ADDR_VIDEO);
  oledInit(I2C_NRF_OLED, OLED_ADDR_NRF);

  g_nrfOk = initNRF24();
  Serial.println("ESP32 RX5808 + NRF24 FHSS monitor");
  Serial.printf("OLED VIDEO SDA=%d SCL=%d addr=0x%02X\n", PIN_VIDEO_SDA, PIN_VIDEO_SCL, OLED_ADDR_VIDEO);
  Serial.printf("OLED NRF   SDA=%d SCL=%d addr=0x%02X\n", PIN_NRF_OLED_SDA, PIN_NRF_OLED_SCL, OLED_ADDR_NRF);
  Serial.println(g_nrfOk ? "NRF24: OK" : "NRF24: ERROR");

  drawAmbientCalibrationScreen();
  oledFlush(I2C_VIDEO, OLED_ADDR_VIDEO, OLED_COL_OFFSET_VIDEO);
  drawAmbientCalibrationScreen();
  oledFlush(I2C_NRF_OLED, OLED_ADDR_NRF, OLED_COL_OFFSET_NRF);
  calibrateRSSI();

  drawVideoScreen(); oledFlush(I2C_VIDEO, OLED_ADDR_VIDEO, OLED_COL_OFFSET_VIDEO);
  drawNrfScreen();   oledFlush(I2C_NRF_OLED, OLED_ADDR_NRF, OLED_COL_OFFSET_NRF);
}

void loop()
{
  buzzerUpdate();
  updateLedStateMachine();

  static uint32_t lastNrfScanMs = 0;
  if (millis() - lastNrfScanMs >= NRF_SCAN_INTERVAL_MS) {
    lastNrfScanMs = millis();
    scanNRF24FHSS();
    drawNrfScreen();
    oledFlush(I2C_NRF_OLED, OLED_ADDR_NRF, OLED_COL_OFFSET_NRF);
  }

  static uint32_t lastScanMs = 0;
  if (millis() - lastScanMs >= SCAN_INTERVAL_MS) {
    lastScanMs = millis();

    int b, c, mean, sd;
    uint16_t f;
    scanBestVideo(b, c, f, mean, sd);

    bool present = isVideoPresent(b, c, mean, sd, g_video.locked);

    updateVideoPersistence(present);
    g_videoScore = computeVideoConfidenceScore(b, c, mean, sd);
    const char* cls = classifySignal(b, c, mean, sd, g_videoScore);
    strncpy(g_videoClass, cls, sizeof(g_videoClass) - 1);
    g_videoClass[sizeof(g_videoClass) - 1] = '\0';

    bool validVideo = (strcmp(g_videoClass, "VALID_VIDEO") == 0);
    g_videoTarget = (g_video.locked && validVideo);

    if (!g_video.locked) {
      if (present) {
        g_video.locked = true;
        g_video.band = b; g_video.ch = c; g_video.freq = f;
        g_video.mean = mean; g_video.stddev = sd;

        Serial.print("VIDEO DETECTED: Band-"); Serial.print(BAND_NAMES[b]);
        Serial.print(" CH"); Serial.print(c + 1);
        Serial.print(" "); Serial.print(f); Serial.print("MHz");
        Serial.print(" mean="); Serial.print(mean);
        Serial.print(" sd="); Serial.print(sd);
        Serial.print(" score="); Serial.print((int)g_videoScore);
        Serial.print(" class="); Serial.println(g_videoClass);

        if (validVideo) {
          buzzerStart(2000);
        }
      }
    } else {
      if (present) {
        bool changed = (b != g_video.band) || (c != g_video.ch);

        g_video.band = b; g_video.ch = c; g_video.freq = f;
        g_video.mean = mean; g_video.stddev = sd;

        Serial.print("VIDEO: Band-"); Serial.print(BAND_NAMES[b]);
        Serial.print(" CH"); Serial.print(c + 1);
        Serial.print(" "); Serial.print(f); Serial.print("MHz");
        Serial.print(" mean="); Serial.print(mean);
        Serial.print(" sd="); Serial.print(sd);
        Serial.print(" score="); Serial.print((int)g_videoScore);
        Serial.print(" class="); Serial.println(g_videoClass);

        if (changed) {
          Serial.println("ALERT: channel/band change detected.");
          if (validVideo) {
            buzzerStart(2000);
          }
        } else {
          static bool lastWasValid = false;
          if (validVideo && !lastWasValid) buzzerStart(2000);
          lastWasValid = validVideo;
        }
      } else {
        g_video.locked = false;
        g_videoTarget = false;
        Serial.println("VIDEO LOST.");
      }
    }

    g_videoTarget = g_video.locked && validVideo;

    drawVideoScreen();
    oledFlush(I2C_VIDEO, OLED_ADDR_VIDEO, OLED_COL_OFFSET_VIDEO);
  }
}

