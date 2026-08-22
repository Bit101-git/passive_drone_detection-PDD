#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <RF24.h>
#include <math.h>
#include <string.h>

static const int PIN_RX5808_DATA = 23;   // CH1 (DATA)
static const int PIN_RX5808_CLK  = 18;   // CH2 (CLK)
static const int PIN_RX5808_CS   = 5;    // CH3 (CS)
static const int PIN_RX5808_RSSI = 34;   // RSSI (ADC)

static const int PIN_VIDEO_SDA = 21;
static const int PIN_VIDEO_SCL = 22;
static const int PIN_ELRS_SDA  = 16;
static const int PIN_ELRS_SCL  = 17;
TwoWire I2C_VIDEO = TwoWire(0);
TwoWire I2C_ELRS  = TwoWire(1);

static const int PIN_LED_CAL   = 25; // verde
static const int PIN_LED_VIDEO = 32; // rojo: video analogico
static const int PIN_LED_ELRS  = 33; // rojo: saltos compatibles con ELRS
static const int PIN_BUZZER    = 4;

static const int PIN_NRF_SCK  = 14;
static const int PIN_NRF_MISO = 12;
static const int PIN_NRF_MOSI = 13;
static const int PIN_NRF_CE   = 26;
static const int PIN_NRF_CSN  = 27;
SPIClass NRF_SPI(HSPI);
RF24 radio(PIN_NRF_CE, PIN_NRF_CSN);

static const char* BAND_NAMES[9] = {"A","B","E","F","R","U","O","L","H"};
static const uint16_t FREQS[9][8] = {
  {5865,5845,5825,5805,5785,5765,5745,5725}, // A
  {5733,5752,5771,5790,5809,5828,5847,5866}, // B
  {5705,5685,5665,5645,5885,5905,5925,5945}, // E
  {5740,5760,5780,5800,5820,5840,5860,5880}, // F
  {5658,5695,5732,5769,5806,5843,5880,5917}, // R
  {5325,5348,5366,5384,5402,5420,5438,5456}, // U
  {5474,5492,5510,5528,5546,5564,5582,5600}, // O
  {5333,5373,5413,5453,5493,5533,5573,5613}, // L
  {5653,5693,5733,5773,5813,5853,5893,5933}  // H
};


static const uint32_t SCAN_INTERVAL_MS = 1200;  // reevaluar canal
static const uint16_t TUNE_SETTLE_MS   = 25;

static const int      RSSI_CAL_SAMPLES = 250;   // auto-cal baseline
static const uint16_t RSSI_CAL_DELAY_MS = 4;

static const int      RSSI_WINDOW_SAMPLES = 24; // por frecuencia: mean/stddev
static const uint16_t RSSI_WINDOW_DELAY_MS = 3;

static const int      RSSI_MARGIN       = 220;  // baseline + margen => “señal”
static const int      RSSI_STDDEV_MAX   = 120;  // estabilidad: stddev bajo

static const int      RSSI_HYST         = 80;   // histéresis lock/unlock

static const uint32_t NRF_SCAN_INTERVAL_MS = 40;
static const uint8_t  NRF_FIRST_CHANNEL = 0;    // 2400 MHz
static const uint8_t  NRF_LAST_CHANNEL  = 83;   // 2483 MHz
static const uint8_t  NRF_SAMPLES_PER_CHANNEL = 2;
static const uint8_t  FHSS_MIN_HITS = 5;
static const uint8_t  FHSS_MIN_SPREAD = 18;
static const uint8_t  FHSS_CONFIRM_SCANS = 3;
static const uint32_t FHSS_HOLD_MS = 1000;

static const uint8_t VIDEO_PERSIST_N_DEFAULT = 3; // N configurable por defecto 3

struct VideoLock {
  bool locked = false;
  int  band = -1;
  int  ch   = -1;
  uint16_t freq = 0;
  int mean = 0;
  int stddev = 0;
} g_video;

static int g_rssiBaseline = 0;
static int g_rssiThresholdOn  = 0; // baseline + margin
static int g_rssiThresholdOff = 0; // thresholdOn - hyst

static uint8_t  g_videoScore = 0;                 
static char     g_videoClass[24] = "RUIDO/INTERFERENCIA";
static uint8_t  g_videoPersistCount = 0;          
static uint8_t  g_videoPersistN = VIDEO_PERSIST_N_DEFAULT;

static bool g_videoTarget = false;

static bool g_nrfOk = false;
static bool g_elrsTarget = false;
static uint8_t g_rfHits = 0;
static uint8_t g_rfFirstHit = 0;
static uint8_t g_rfLastHit = 0;
static uint8_t g_fhssConfirm = 0;
static uint32_t g_lastFhssMs = 0;

static int readRSSI()
{
  return analogRead(PIN_RX5808_RSSI); // 0..4095
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

static void scanBestVideo(int &bestBand, int &bestCh, uint16_t &bestFreq, int &bestMean, int &bestStd)
{
  bestBand = -1; bestCh = -1; bestFreq = 0;
  bestMean = -1; bestStd = 99999;

  for (int b = 0; b < 9; b++) {
    for (int c = 0; c < 8; c++) {
      uint16_t f = FREQS[b][c];
      rx5808SetFrequencyMHz(f);

      int mean = 0, sd = 0;
      computeMeanStddev(mean, sd);

      if (mean > bestMean) {
        bestMean = mean;
        bestStd  = sd;
        bestBand = b;
        bestCh   = c;
        bestFreq = f;
      }
    }
  }
}

static bool isVideoPresent(int mean, int stddev, bool currentlyLocked)
{
  int onTh  = g_rssiThresholdOn;
  int offTh = g_rssiThresholdOff;

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

static uint8_t computeVideoConfidenceScore(int mean, int stddev)
{
  const int baseline = g_rssiBaseline;
  const int onTh = g_rssiThresholdOn;

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

static const char* classifySignal(int mean, int stddev, uint8_t score)
{
  const int onTh = g_rssiThresholdOn;

  if (mean >= (onTh + 250) && stddev > RSSI_STDDEV_MAX) return "FUERTE_INESTABLE";
  if (mean < onTh || score < 40) return "RUIDO/INTERFERENCIA";

  bool stable = stddev <= RSSI_STDDEV_MAX;
  bool present = mean >= onTh;
  if (present && stable && score >= 70) return "VIDEO_VALIDO";

  return "RUIDO/INTERFERENCIA";
}

#if 0

static uint8_t crsf_crc8(const uint8_t *ptr, uint8_t len)
{
  uint8_t crc = 0;
  while (len--) {
    crc ^= *ptr++;
    for (uint8_t i = 0; i < 8; i++)
      crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0xD5) : (uint8_t)(crc << 1);
  }
  return crc;
}

static void decodeCRSFChannels(const uint8_t payload[22], uint16_t ch[16])
{
  uint32_t bitbuf = 0;
  int bits = 0, idx = 0;

  for (int i = 0; i < 22; i++) {
    bitbuf |= ((uint32_t)payload[i]) << bits;
    bits += 8;
    while (bits >= 11 && idx < 16) {
      ch[idx++] = bitbuf & 0x7FF;
      bitbuf >>= 11;
      bits -= 11;
    }
  }
}

static int16_t mapChannelToPercent(int v, int vmin, int vmax)
{
  if (v < vmin) v = vmin;
  if (v > vmax) v = vmax;
  long num = (long)(v - vmin) * 200L;
  long den = (long)(vmax - vmin);
  return (int16_t)(num / den - 100);
}

static uint8_t mapChannelToThrottle(int v, int vmin, int vmax)
{
  if (v < vmin) v = vmin;
  if (v > vmax) v = vmax;
  long num = (long)(v - vmin) * 100L;
  long den = (long)(vmax - vmin);
  return (uint8_t)(num / den);
}

static void printChannelsBrief(const uint16_t ch[16])
{
  const int VMIN = 172;
  const int VMAX = 1811;

  int16_t roll     = mapChannelToPercent(ch[0], VMIN, VMAX);
  int16_t pitch    = mapChannelToPercent(ch[1], VMIN, VMAX);
  uint8_t throttle = mapChannelToThrottle(ch[2], VMIN, VMAX);
  int16_t yaw      = mapChannelToPercent(ch[3], VMIN, VMAX);

  Serial.print("CRSF: ROLL="); Serial.print(roll);
  Serial.print(" PITCH="); Serial.print(pitch);
  Serial.print(" THR="); Serial.print(throttle);
  Serial.print(" YAW="); Serial.print(yaw);

  for (int i = 4; i <= 8; i++) {
    Serial.print(" AUX"); Serial.print(i - 3);
    Serial.print("="); Serial.print(ch[i]);
  }
  Serial.println();
}

static bool detectCommandActive(const uint16_t ch[16])
{
  if (!g_haveLastCh) return false;

  const int idxs[] = {0,1,2,3,4,5,6,7,8};
  for (int k = 0; k < (int)(sizeof(idxs)/sizeof(idxs[0])); k++) {
    int i = idxs[k];
    uint16_t a = ch[i], b = g_lastCh[i];
    uint16_t d = (a > b) ? (a - b) : (b - a);
    if (d >= COMMAND_DELTA) return true;
  }
  return false;
}

static void pollCRSF()
{
  static uint8_t frame[64];
  static int pos = 0;
  static uint32_t lastByteMs = 0;

  while (CRSFSerial.available()) {
    uint8_t b = CRSFSerial.read();
    lastByteMs = millis();

    if (pos == 0) { frame[pos++] = b; continue; }
    frame[pos++] = b;

    if (pos == 2) continue;
    uint8_t len = frame[1];
    int total = 2 + len;

    if (pos >= total) {
      uint8_t type = frame[2];
      uint8_t crc  = frame[2 + (len - 1)];
      uint8_t calc = crsf_crc8(&frame[2], (uint8_t)(len - 1));

      if (calc == crc) {
        g_lastCrsfValidMs = millis();

        if (type == 0x16 && len >= (1 + 22 + 1)) {
          uint16_t channels[16] = {0};
          decodeCRSFChannels(&frame[3], channels);

          bool active = detectCommandActive(channels);
          uint32_t now = millis();

          g_rcActive = active;
          static uint32_t lastActiveMs = 0;
          if (active) lastActiveMs = now;
          g_rcTarget = (!g_linkLost) && (now - lastActiveMs <= 700);

          printChannelsBrief(channels);

          if (active && (now - g_lastCmdAlertMs >= COMMAND_RATE_MS)) {
            g_lastCmdAlertMs = now;
            Serial.println("ALERTA: comandos activos detectados.");
            digitalWrite(PIN_ALERT, HIGH);
            delay(30);
            digitalWrite(PIN_ALERT, LOW);
          }

          memcpy(g_lastCh, channels, sizeof(g_lastCh));
          g_haveLastCh = true;
        }
      }
      pos = 0;
    }

    if (pos >= (int)sizeof(frame)) pos = 0;
  }

  if (pos > 0 && (millis() - lastByteMs) > 20) pos = 0;
}

static void updateCRSFWatchdog()
{
  uint32_t now = millis();
  bool lost = (g_lastCrsfValidMs == 0) || (now - g_lastCrsfValidMs > CRSF_WATCHDOG_MS);

  if (lost && !g_linkLost) {
    g_linkLost = true;
    Serial.println("WATCHDOG: LINK LOST (sin CRSF valido).");
  } else if (!lost && g_linkLost) {
    g_linkLost = false;
    Serial.println("WATCHDOG: LINK OK (CRSF activo).");
  }
}
#endif

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
    g_elrsTarget = false;
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
      g_elrsTarget = true;
    }
  } else if (g_fhssConfirm > 0) {
    g_fhssConfirm--;
  }

  if (millis() - g_lastFhssMs > FHSS_HOLD_MS) g_elrsTarget = false;
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
  digitalWrite(PIN_LED_ELRS, g_elrsTarget ? HIGH : LOW);
}

static void calibrateRSSI()
{
  Serial.println("Calibrando RSSI (baseline)... No transmitas cerca durante esto.");
  long sum = 0;

  g_calibrating = true;

  rx5808SetFrequencyMHz(FREQS[0][0]);
  delay(120);

  for (int i = 0; i < RSSI_CAL_SAMPLES; i++) {
    sum += readRSSI();
    delay(RSSI_CAL_DELAY_MS);
    updateLedStateMachine();
    buzzerUpdate();
  }

  g_rssiBaseline = (int)(sum / RSSI_CAL_SAMPLES);
  g_rssiThresholdOn  = g_rssiBaseline + RSSI_MARGIN;
  g_rssiThresholdOff = g_rssiThresholdOn - RSSI_HYST;

  g_calibrating = false;

  Serial.print("RSSI baseline="); Serial.print(g_rssiBaseline);
  Serial.print(" thresholdOn="); Serial.print(g_rssiThresholdOn);
  Serial.print(" thresholdOff="); Serial.println(g_rssiThresholdOff);
}

static const uint8_t OLED_ADDR_VIDEO = 0x3C;
static const uint8_t OLED_ADDR_ELRS  = 0x3C; // permitido: esta en otro bus

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
  {0x38,0x54,0x54,0x54,0x18},{0x08,0x7E,0x09,0x01,0x02},{0x0C,0x52,0x52,0x52,0x3E},{0x7F,0x08,0x04,0x04,0x78},
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
  oledCmd(bus, addr, 0x20); oledCmd(bus, addr, 0x00);
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

static void oledDrawChar(int x, int y, char ch)
{
  if (ch < 32 || ch > 126) ch = '?';
  const uint8_t* glyph = Font5x7[ch - 32];
  for (int col = 0; col < 5; col++) {
    uint8_t bits = pgm_read_byte(&glyph[col]);
    for (int row = 0; row < 7; row++) {
      oledSetPixel(x + col, y + row, (bits & (1 << row)) != 0);
    }
  }
}

static void oledDrawText(int x, int y, const char* s)
{
  int cx = x;
  while (*s) {
    oledDrawChar(cx, y, *s++);
    cx += 6;
    if (cx > 122) break;
  }
}

static void oledFlush(TwoWire &bus, uint8_t addr)
{
  oledCmd(bus, addr, 0x21); oledCmd(bus, addr, 0x00); oledCmd(bus, addr, 0x7F);
  oledCmd(bus, addr, 0x22); oledCmd(bus, addr, 0x00); oledCmd(bus, addr, 0x07);

  uint16_t i = 0;
  while (i < 1024) {
    oledDataStart(bus, addr);
    for (int k = 0; k < 24 && i < 1024; k++, i++) bus.write(g_oledBuf[i]);
    oledDataEnd(bus);
  }
}

static void drawVideoScreen()
{
  oledClearBuf();
  oledDrawText(0, 0,  "VIDEO MONITOR");
  oledDrawText(0, 10, "Status:");
  oledDrawText(48, 10, g_videoTarget ? "TARGET" : "BUSCANDO");

  char line[32];
  if (g_video.band >= 0 && g_video.ch >= 0) {
    snprintf(line, sizeof(line), "B:%s CH:%d %4uMHz", BAND_NAMES[g_video.band], g_video.ch + 1, g_video.freq);
  } else {
    snprintf(line, sizeof(line), "B:-- CH:-- ----MHz");
  }
  oledDrawText(0, 22, line);

  snprintf(line, sizeof(line), "mean:%4d sd:%3d", g_video.mean, g_video.stddev);
  oledDrawText(0, 34, line);

  snprintf(line, sizeof(line), "score:%3u", (unsigned)g_videoScore);
  oledDrawText(0, 46, line);

  oledDrawText(0, 56, g_videoClass);
}

static void drawElrsScreen()
{
  oledClearBuf();
  oledDrawText(0, 0, "2.4 GHz / FHSS");
  oledDrawText(0, 10, "NRF24:");
  oledDrawText(42, 10, g_nrfOk ? "OK" : "ERROR");
  oledDrawText(0, 22, "Patron:");
  oledDrawText(42, 22, g_elrsTarget ? "ELRS?" : "NO");

  char line[32];
  snprintf(line, sizeof(line), "hits:%u conf:%u/%u", g_rfHits,
           g_fhssConfirm, FHSS_CONFIRM_SCANS);
  oledDrawText(0, 34, line);
  snprintf(line, sizeof(line), "%u-%u MHz", 2400 + g_rfFirstHit,
           2400 + g_rfLastHit);
  oledDrawText(0, 46, line);
  oledDrawText(0, 56, "FHSS no concluyente");
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
  pinMode(PIN_LED_ELRS, OUTPUT);
  digitalWrite(PIN_LED_CAL, LOW);
  digitalWrite(PIN_LED_VIDEO, LOW);
  digitalWrite(PIN_LED_ELRS, LOW);

  // Buzzer init
  buzzerInit();
  buzzerStart(180); // aviso breve al encender

  analogReadResolution(12);

  I2C_VIDEO.begin(PIN_VIDEO_SDA, PIN_VIDEO_SCL, 400000);
  I2C_ELRS.begin(PIN_ELRS_SDA, PIN_ELRS_SCL, 400000);
  oledInit(I2C_VIDEO, OLED_ADDR_VIDEO);
  oledInit(I2C_ELRS, OLED_ADDR_ELRS);

  g_nrfOk = initNRF24();
  Serial.println("ESP32 RX5808 + NRF24 FHSS monitor");
  Serial.printf("OLED VIDEO SDA=%d SCL=%d addr=0x%02X\n", PIN_VIDEO_SDA, PIN_VIDEO_SCL, OLED_ADDR_VIDEO);
  Serial.printf("OLED ELRS  SDA=%d SCL=%d addr=0x%02X\n", PIN_ELRS_SDA, PIN_ELRS_SCL, OLED_ADDR_ELRS);
  Serial.println(g_nrfOk ? "NRF24: OK" : "NRF24: ERROR");

  calibrateRSSI();

  drawVideoScreen(); oledFlush(I2C_VIDEO, OLED_ADDR_VIDEO);
  drawElrsScreen();  oledFlush(I2C_ELRS, OLED_ADDR_ELRS);
}

void loop()
{
  buzzerUpdate();
  updateLedStateMachine();

  static uint32_t lastNrfScanMs = 0;
  if (millis() - lastNrfScanMs >= NRF_SCAN_INTERVAL_MS) {
    lastNrfScanMs = millis();
    scanNRF24FHSS();
    drawElrsScreen();
    oledFlush(I2C_ELRS, OLED_ADDR_ELRS);
  }

  static uint32_t lastScanMs = 0;
  if (millis() - lastScanMs >= SCAN_INTERVAL_MS) {
    lastScanMs = millis();

    int b, c, mean, sd;
    uint16_t f;
    scanBestVideo(b, c, f, mean, sd);

    bool present = isVideoPresent(mean, sd, g_video.locked);

    updateVideoPersistence(present);
    g_videoScore = computeVideoConfidenceScore(mean, sd);
    const char* cls = classifySignal(mean, sd, g_videoScore);
    strncpy(g_videoClass, cls, sizeof(g_videoClass) - 1);
    g_videoClass[sizeof(g_videoClass) - 1] = '\0';

    bool videoValido = (strcmp(g_videoClass, "VIDEO_VALIDO") == 0);
    g_videoTarget = (g_video.locked && videoValido);

    if (!g_video.locked) {
      if (present) {
        g_video.locked = true;
        g_video.band = b; g_video.ch = c; g_video.freq = f;
        g_video.mean = mean; g_video.stddev = sd;

        Serial.print("VIDEO DETECTADO: Band-"); Serial.print(BAND_NAMES[b]);
        Serial.print(" CH"); Serial.print(c + 1);
        Serial.print(" "); Serial.print(f); Serial.print("MHz");
        Serial.print(" mean="); Serial.print(mean);
        Serial.print(" sd="); Serial.print(sd);
        Serial.print(" score="); Serial.print((int)g_videoScore);
        Serial.print(" class="); Serial.println(g_videoClass);

        if (videoValido) {
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
          Serial.println("ALERTA: cambio de canal/banda detectado.");
          if (videoValido) {
            buzzerStart(2000);
          }
        } else {
          static bool lastWasValid = false;
          if (videoValido && !lastWasValid) buzzerStart(2000);
          lastWasValid = videoValido;
        }
      } else {
        g_video.locked = false;
        g_videoTarget = false;
        Serial.println("VIDEO PERDIDO.");
      }
    }

    // El LED rojo refleja el resultado final de este barrido, sin parpadeos
    // cuando no hay deteccion.
    g_videoTarget = g_video.locked && videoValido;

    drawVideoScreen();
    oledFlush(I2C_VIDEO, OLED_ADDR_VIDEO);
  }
}
