#pragma once
/*
  Tum degistirilebilir ayarlar ve paylasilan durum burada.
  Pin isimleri *_PIN — Arduino LED/SDA/SCL makro cakismasini onlemek icin.
*/

#include <Arduino.h>

// ===================== PROJE =====================
namespace ProjectCfg {
  constexpr const char *VERSION = "0.2.1";
}

// ===================== PINLER =====================
namespace Pins {
  constexpr int LED_PIN = 27;
  constexpr int SEEK_UP_PIN = 13;
  constexpr int SEEK_DOWN_PIN = 14;
  constexpr int I2C_SDA_PIN = 21;
  constexpr int I2C_SCL_PIN = 22;
  constexpr int DAC_OUT_PIN = 25;   // ESP32 DAC1 — jingle
  constexpr int DAC2_OUT_PIN = 26;  // ESP32 DAC2 — opsiyonel L+R
  // constexpr int FAV_PIN = ??;
}

// ===================== SERI =====================
namespace SerialCfg {
  constexpr unsigned long BAUD = 115200;
  constexpr unsigned long BOOT_DELAY_MS = 300;
  constexpr size_t CMD_BUF_SIZE = 64;
}

// ===================== I2C / RDA5807M =====================
namespace RadioCfg {
  constexpr bool DEBUG_RADIO = true;

  constexpr uint8_t I2C_ADDR_SEQ = 0x10;
  constexpr uint8_t I2C_ADDR_RAND = 0x11;
  constexpr uint8_t I2C_ADDR = I2C_ADDR_SEQ;
  constexpr uint32_t I2C_HZ = 100000;

  constexpr float FREQ_MIN_MHZ = 87.0f;
  constexpr float FREQ_MAX_MHZ = 108.0f;
  constexpr float FREQ_STEP_MHZ = 0.1f;

  constexpr float DEFAULT_FREQ_MHZ = 93.1f;
  constexpr uint8_t DEFAULT_VOLUME = 6;
  constexpr uint8_t VOLUME_MAX = 15;
  constexpr bool DEFAULT_BASS = false;
  constexpr bool DEFAULT_MONO = false;
  constexpr bool DEFAULT_MUTED = false;
  constexpr bool DEFAULT_FREQUENCY_VALID = false;

  constexpr uint16_t REG02_BASE = 0xC00D;
  constexpr uint16_t REG04_VALUE = 0x0C00;
  constexpr uint16_t REG05_BASE = 0x9080;

  constexpr unsigned long STATUS_POLL_MS = 50;
  constexpr unsigned long OPERATION_MIN_SETTLE_MS = 50;  // komut yazimindan sonra min bekleme
  constexpr unsigned long SEEK_TIMEOUT_MS = 2000;
  constexpr unsigned long TUNE_TIMEOUT_MS = 1000;
  constexpr unsigned long INIT_DELAY_MS = 60;
  constexpr unsigned long INIT_TUNE_WAIT_EXTRA_MS = 200;
  constexpr uint8_t RSSI_WEAK_THRESHOLD = 20;
}

// ===================== SES / JINGLE (DAC) =====================
namespace AudioCfg {
  constexpr bool PLAY_ON_BOOT = true;
  constexpr bool MUTE_RADIO_WHILE_PLAYING = true;
  constexpr uint8_t DAC_IDLE_LEVEL = 128;
  // Jingle, radyo ses seviyesinden bagimsiz olarak bu sabit maksimum ayarla calar.
  constexpr float DAC_GAIN = 3.0f;
  // Zayif duyulursa LOUD_MODE raylara iter (bozuk ama yuksek)
  constexpr bool LOUD_MODE = true;
  constexpr int LOUD_THRESHOLD = 8;
  constexpr int LOUD_PUSH = 4;
  // Sadece LIN: GPIO25. GPIO26 kullanilmiyor.
  constexpr bool USE_SECOND_DAC = false;
}

// ===================== LED =====================
namespace LedCfg {
  constexpr bool ACTIVE_HIGH = true;
  constexpr bool START_ON = true;  // gecici: baslangicta acik (ileride degisecek)
  constexpr int BLINK_ON_MS = 80;
  constexpr int BLINK_OFF_MS = 80;
  constexpr int FAV_SAVE_BLINKS = 3;
  constexpr int FAV_SAVE_ON_MS = 100;
  constexpr int FAV_SAVE_OFF_MS = 80;
  constexpr int FAV_EMPTY_BLINKS = 2;
  constexpr int FAV_EMPTY_ON_MS = 40;
  constexpr int FAV_EMPTY_OFF_MS = 40;
}

// ===================== DUGMELER =====================
namespace ButtonCfg {
  constexpr unsigned long DEBOUNCE_MS = 30;
  constexpr unsigned long FAV_LONG_PRESS_MS = 1200;
  // Gecici: basili tutunca ses (ileride kaldirilacak)
  constexpr unsigned long VOL_HOLD_MS = 400;     // bundan uzun basili = ses
  constexpr unsigned long VOL_REPEAT_MS = 180;   // basili iken ses adimi araligi
}

// ===================== FAVORILER =====================
namespace FavCfg {
  constexpr int COUNT = 6;
  constexpr int ACTIVE_SLOT = 0;
  constexpr const char *NVS_NAMESPACE = "radio";
}

// ===================== RUNTIME STATE =====================
struct RadioState {
  float freqMHz;
  uint8_t volume;
  bool muted;
  bool bassOn;
  bool forceMono;
  bool frequencyValid;
};

struct LedState {
  bool on;
};

struct FavState {
  float freq[FavCfg::COUNT];
  bool valid[FavCfg::COUNT];
};

extern RadioState g_radio;
extern LedState g_led;
extern FavState g_fav;
