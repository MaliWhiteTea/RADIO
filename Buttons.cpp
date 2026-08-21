#include "Buttons.h"
#include "Data.h"
#include "Radio.h"

/*
  Kisa bas: seek
  Basili tut: ses (GECICI — ileride kaldirilacak)
  SEEK_UP   = ses +
  SEEK_DOWN = ses -
*/

enum class BtnPhase : uint8_t {
  Idle,
  DebouncePress,
  Pressed,          // kisa mi uzun mu bekleniyor
  VolumeHold,       // basili tutarak ses
  DebounceRelease
};

struct SeekBtn {
  int pin;
  bool seekUp;      // true=UP/sag, false=DOWN/sol
  BtnPhase phase;
  unsigned long t0;
  unsigned long lastVolMs;
  bool didVolume;   // hold ile ses yapildiysa birakinca seek yok
};

static SeekBtn btnUp = { Pins::SEEK_UP_PIN, true, BtnPhase::Idle, 0, 0, false };
static SeekBtn btnDown = { Pins::SEEK_DOWN_PIN, false, BtnPhase::Idle, 0, 0, false };

void buttonsInit() {
  if (RadioCfg::DEBUG_RADIO) {
    Serial.printf("[BTN] INPUT_PULLUP UP=GP%d DOWN=GP%d (hold=volume TEMP)\n",
                  Pins::SEEK_UP_PIN, Pins::SEEK_DOWN_PIN);
  }
  pinMode(Pins::SEEK_UP_PIN, INPUT_PULLUP);
  pinMode(Pins::SEEK_DOWN_PIN, INPUT_PULLUP);
  btnUp.phase = BtnPhase::Idle;
  btnDown.phase = BtnPhase::Idle;
}

static void adjustVolume(bool up) {
  if (!radioIsReady() || radioIsBusy()) return;

  int v = (int)g_radio.volume + (up ? 1 : -1);
  if (v < 0) v = 0;
  if (v > (int)RadioCfg::VOLUME_MAX) v = (int)RadioCfg::VOLUME_MAX;
  if ((uint8_t)v == g_radio.volume) return;

  if (radioSetVolume((uint8_t)v)) {
    Serial.printf("Vol %u/%u\n", g_radio.volume, RadioCfg::VOLUME_MAX);
  }
}

static void doSeek(const SeekBtn &b) {
  if (!radioIsReady()) return;
  if (radioIsBusy()) return;
  if (radioSeekStart(b.seekUp)) {
    Serial.printf("Seek %s (GP%d)...\n", b.seekUp ? "UP" : "DOWN", b.pin);
  } else {
    Serial.println("Seek baslatilamadi");
  }
}

static void updateSeekBtn(SeekBtn &b) {
  const bool rawPressed = (digitalRead(b.pin) == LOW);
  const unsigned long now = millis();

  switch (b.phase) {
    case BtnPhase::Idle:
      if (rawPressed) {
        b.phase = BtnPhase::DebouncePress;
        b.t0 = now;
        b.didVolume = false;
      }
      break;

    case BtnPhase::DebouncePress:
      if (!rawPressed) {
        b.phase = BtnPhase::Idle;
      } else if ((now - b.t0) >= ButtonCfg::DEBOUNCE_MS) {
        b.phase = BtnPhase::Pressed;
        b.t0 = now;
      }
      break;

    case BtnPhase::Pressed:
      if (!rawPressed) {
        // Kisa bas: seek
        if (!b.didVolume) doSeek(b);
        b.phase = BtnPhase::DebounceRelease;
        b.t0 = now;
      } else if ((now - b.t0) >= ButtonCfg::VOL_HOLD_MS) {
        // Gecici: basili tut = ses
        adjustVolume(b.seekUp);
        b.didVolume = true;
        b.lastVolMs = now;
        b.phase = BtnPhase::VolumeHold;
      }
      break;

    case BtnPhase::VolumeHold:
      if (!rawPressed) {
        b.phase = BtnPhase::DebounceRelease;
        b.t0 = now;
      } else if ((now - b.lastVolMs) >= ButtonCfg::VOL_REPEAT_MS) {
        adjustVolume(b.seekUp);
        b.lastVolMs = now;
      }
      break;

    case BtnPhase::DebounceRelease:
      if (rawPressed) {
        b.phase = BtnPhase::Pressed;
        b.t0 = now;
        b.didVolume = false;
      } else if ((now - b.t0) >= ButtonCfg::DEBOUNCE_MS) {
        b.phase = BtnPhase::Idle;
      }
      break;
  }
}

void buttonsUpdate() {
  updateSeekBtn(btnDown);
  updateSeekBtn(btnUp);
}
