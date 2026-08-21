#include "SerialCmd.h"
#include "Data.h"
#include "Radio.h"
#include "Led.h"
#include "Favorites.h"
#include <string.h>

static char cmdBuf[SerialCfg::CMD_BUF_SIZE];
static size_t cmdLen = 0;

void serialPrintHelp() {
  Serial.println();
  Serial.println("=== RDA5807M ===");
  Serial.println("  f101.1  frekans");
  Serial.printf("  v0..%u  ses\n", RadioCfg::VOLUME_MAX);
  Serial.println("  m       mute");
  Serial.println("  b       bass ac/kapa");
  Serial.println("  n       mono/stereo");
  Serial.println("  u / d   seek");
  Serial.printf("  GP%d    sonraki frekans\n", Pins::SEEK_UP_PIN);
  Serial.printf("  GP%d    onceki frekans\n", Pins::SEEK_DOWN_PIN);
  Serial.println("  led on / led off / led");
  Serial.println("  radio reset   RDA yeniden baslat");
  Serial.println("  ?       durum + RSSI");
  Serial.println("  h       yardim");
}

void serialPrintStatus() {
  uint8_t rssi = 0;
  const bool rssiOk = radioReadRssi(rssi);

  if (!radioIsReady()) {
    Serial.printf("Radyo HAZIR DEGIL | initError=%s\n",
                  radioInitErrorStr(radioGetInitError()));
  }

  if (g_radio.frequencyValid) {
    Serial.printf("Freq %.1f MHz", g_radio.freqMHz);
  } else {
    Serial.printf("Freq DOGRULANMADI (son bilinen %.1f MHz)", g_radio.freqMHz);
  }

  Serial.printf(" | Vol %u/%u | Mute %s | Bass %s | %s | LED %s",
                g_radio.volume, RadioCfg::VOLUME_MAX,
                g_radio.muted ? "ON" : "OFF",
                g_radio.bassOn ? "ON" : "OFF",
                g_radio.forceMono ? "MONO" : "STEREO",
                ledIsOn() ? "ON" : "OFF");

  if (rssiOk) {
    Serial.printf(" | RSSI %u\n", rssi);
    if (rssi < RadioCfg::RSSI_WEAK_THRESHOLD) {
      Serial.println("RSSI dusuk: anteni uzat / pencereye yaklastir.");
    }
  } else {
    Serial.println(" | RSSI ? (I2C okuma hatasi)");
  }

  if (!g_radio.frequencyValid) {
    Serial.println("Uyari: donanim frekansi dogrulanamadi; yeni tune deneyin.");
  }

  if (favoritesIsValid(FavCfg::ACTIVE_SLOT)) {
    Serial.printf("Favori %d: %.1f MHz\n",
                  FavCfg::ACTIVE_SLOT + 1,
                  favoritesGetFreq(FavCfg::ACTIVE_SLOT));
  } else {
    Serial.printf("Favori %d: bos\n", FavCfg::ACTIVE_SLOT + 1);
  }
}

void serialCmdInit() {
  Serial.begin(SerialCfg::BAUD);
  cmdLen = 0;
  delay(SerialCfg::BOOT_DELAY_MS);
}

static bool isDigitsOrDot(const char *s, bool allowDot) {
  if (!s || !*s) return false;
  bool seenDot = false;
  for (const char *p = s; *p; ++p) {
    if (*p >= '0' && *p <= '9') continue;
    if (allowDot && *p == '.' && !seenDot) {
      seenDot = true;
      continue;
    }
    return false;
  }
  if (allowDot && s[0] == '.' && s[1] == '\0') return false;
  return true;
}

static const char *busyMsg() {
  return "Radyo mesgul (seek/tune/recover), sonra dene";
}

static bool requireReady() {
  if (radioIsReady()) return true;
  Serial.printf("Radyo baslatilmadi (%s). 'radio reset' dene.\n",
                radioInitErrorStr(radioGetInitError()));
  return false;
}

void serialHandleRadioEvents() {
  switch (radioConsumeSeekEvent()) {
    case RadioSeekEvent::None:
      break;
    case RadioSeekEvent::Ok:
      serialPrintStatus();
      break;
    case RadioSeekEvent::NoStation:
      Serial.println("Seek: istasyon bulunamadi");
      serialPrintStatus();
      break;
    case RadioSeekEvent::Timeout:
      Serial.println("Seek timeout");
      serialPrintStatus();
      break;
  }

  switch (radioConsumeTuneEvent()) {
    case RadioTuneEvent::None:
      break;
    case RadioTuneEvent::Ok:
      serialPrintStatus();
      break;
    case RadioTuneEvent::Timeout:
      Serial.println("Tune timeout (STC gelmedi)");
      serialPrintStatus();
      break;
    case RadioTuneEvent::ChannelMismatch:
      Serial.println("Tune hata: READCHAN hedefle uyusmadi");
      serialPrintStatus();
      break;
    case RadioTuneEvent::I2cError:
      Serial.println("Tune hata: I2C");
      serialPrintStatus();
      break;
  }
}

static void handleCommand(char *cmd) {
  while (*cmd == ' ' || *cmd == '\t') ++cmd;
  char *end = cmd + strlen(cmd);
  while (end > cmd && (end[-1] == ' ' || end[-1] == '\t')) {
    *--end = '\0';
  }
  for (char *p = cmd; *p; ++p) {
    if (*p >= 'A' && *p <= 'Z') *p = (char)(*p - 'A' + 'a');
  }
  if (*cmd == '\0') return;

  if (strcmp(cmd, "h") == 0 || strcmp(cmd, "help") == 0) {
    serialPrintHelp();
  } else if (strcmp(cmd, "?") == 0 || strcmp(cmd, "status") == 0) {
    serialPrintStatus();
  } else if (strcmp(cmd, "radio reset") == 0) {
    Serial.println("RDA yeniden baslatiliyor...");
    if (radioInit()) {
      Serial.println("RDA hazir");
      serialPrintStatus();
    } else {
      Serial.printf("HATA: radio init (%s)\n",
                    radioInitErrorStr(radioGetInitError()));
    }
  } else if (strcmp(cmd, "led on") == 0) {
    ledSet(true);
    Serial.println("LED ON");
  } else if (strcmp(cmd, "led off") == 0) {
    ledSet(false);
    Serial.println("LED OFF");
  } else if (strcmp(cmd, "led") == 0) {
    ledToggle();
    Serial.printf("LED %s\n", ledIsOn() ? "ON" : "OFF");
  } else if (strcmp(cmd, "m") == 0 || strcmp(cmd, "mute") == 0) {
    if (!requireReady()) return;
    if (radioIsBusy()) Serial.println(busyMsg());
    else if (!radioSetMute(!g_radio.muted)) {
      if (radioIsBusy()) Serial.println(busyMsg());
      else Serial.println("I2C yazma hatasi");
    } else serialPrintStatus();
  } else if (strcmp(cmd, "b") == 0) {
    if (!requireReady()) return;
    if (radioIsBusy()) Serial.println(busyMsg());
    else if (!radioSetBass(!g_radio.bassOn)) {
      if (radioIsBusy()) Serial.println(busyMsg());
      else Serial.println("I2C yazma hatasi");
    } else serialPrintStatus();
  } else if (strcmp(cmd, "n") == 0) {
    if (!requireReady()) return;
    if (radioIsBusy()) Serial.println(busyMsg());
    else if (!radioSetMono(!g_radio.forceMono)) {
      if (radioIsBusy()) Serial.println(busyMsg());
      else Serial.println("I2C yazma hatasi");
    } else serialPrintStatus();
  } else if (strcmp(cmd, "u") == 0) {
    if (!requireReady()) return;
    if (radioIsBusy()) Serial.println(busyMsg());
    else if (radioSeekStart(true)) Serial.println("Seek UP...");
    else if (radioIsBusy()) Serial.println(busyMsg());
    else Serial.println("Seek baslatilamadi");
  } else if (strcmp(cmd, "d") == 0) {
    if (!requireReady()) return;
    if (radioIsBusy()) Serial.println(busyMsg());
    else if (radioSeekStart(false)) Serial.println("Seek DOWN...");
    else if (radioIsBusy()) Serial.println(busyMsg());
    else Serial.println("Seek baslatilamadi");
  } else if (cmd[0] == 'f' && cmd[1] != '\0') {
    if (!requireReady()) return;
    const char *num = cmd + 1;
    if (!isDigitsOrDot(num, true)) {
      Serial.println("Frekans ornek: f101.1");
      return;
    }
    const float f = atof(num);
    if (f < RadioCfg::FREQ_MIN_MHZ || f > RadioCfg::FREQ_MAX_MHZ) {
      Serial.printf("%.1f - %.1f\n", RadioCfg::FREQ_MIN_MHZ, RadioCfg::FREQ_MAX_MHZ);
      return;
    }
    if (radioIsBusy()) Serial.println(busyMsg());
    else if (!radioSetFrequency(f)) {
      if (radioIsBusy()) Serial.println(busyMsg());
      else Serial.println("I2C yazma hatasi");
    } else Serial.println("Tune...");
  } else if (cmd[0] == 'v' && cmd[1] != '\0') {
    if (!requireReady()) return;
    const char *num = cmd + 1;
    if (!isDigitsOrDot(num, false)) {
      Serial.println("Ses ornek: v8");
      return;
    }
    const int v = atoi(num);
    if (v < 0 || v > (int)RadioCfg::VOLUME_MAX) {
      Serial.printf("0 - %u\n", RadioCfg::VOLUME_MAX);
      return;
    }
    if (radioIsBusy()) Serial.println(busyMsg());
    else if (!radioSetVolume((uint8_t)v)) {
      if (radioIsBusy()) Serial.println(busyMsg());
      else Serial.println("I2C yazma hatasi");
    } else serialPrintStatus();
  } else {
    Serial.println("h yaz");
  }
}

void serialCmdUpdate() {
  static bool cmdOverflow = false;

  while (Serial.available() > 0) {
    const int raw = Serial.read();
    if (raw < 0) break;
    const char c = (char)raw;

    if (c == '\n' || c == '\r') {
      if (cmdOverflow) {
        Serial.println("Komut cok uzun");
        cmdOverflow = false;
        cmdLen = 0;
        continue;
      }
      if (cmdLen == 0) continue;
      cmdBuf[cmdLen] = '\0';
      handleCommand(cmdBuf);
      cmdLen = 0;
      continue;
    }

    if (cmdOverflow) continue;

    if (cmdLen + 1 < SerialCfg::CMD_BUF_SIZE) {
      cmdBuf[cmdLen++] = c;
    } else {
      cmdOverflow = true;
      cmdLen = 0;
    }
  }
}
