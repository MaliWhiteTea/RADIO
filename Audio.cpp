#include "Audio.h"
#include "Data.h"
#include "Radio.h"
#include "JingleData.h"

/*
  ESP32 GPIO25 = DAC1 (gercek analog).
  LOUD_MODE: zayif sinyali raylara yaklastirir (bozulma artar, ses yukselir).
*/

static size_t playIndex = 0;
static bool playing = false;
static unsigned long nextSampleUs = 0;
static const unsigned long SAMPLE_PERIOD_US = 1000000UL / JINGLE_SAMPLE_RATE;
static bool mutedRadioForPlay = false;

static uint8_t shapeSample(uint8_t s) {
  int c = (int)s - 128;
  // once lineer kazanc
  c = (int)(c * AudioCfg::DAC_GAIN);

  if (AudioCfg::LOUD_MODE) {
    // Esik ustunu raylara it (RMS yukselir)
    const int thr = AudioCfg::LOUD_THRESHOLD;
    if (c > thr) {
      c = thr + (c - thr) * (int)AudioCfg::LOUD_PUSH;
      if (c > 127) c = 127;
    } else if (c < -thr) {
      c = -thr + (c + thr) * (int)AudioCfg::LOUD_PUSH;
      if (c < -127) c = -127;
    }
  }

  int out = 128 + c;
  if (out < 0) out = 0;
  if (out > 255) out = 255;
  return (uint8_t)out;
}

void audioInit() {
  playing = false;
  playIndex = 0;
  dacWrite(Pins::DAC_OUT_PIN, AudioCfg::DAC_IDLE_LEVEL);
  // Stereo amp ise L+R baglanabilir
  if (AudioCfg::USE_SECOND_DAC) {
    dacWrite(Pins::DAC2_OUT_PIN, AudioCfg::DAC_IDLE_LEVEL);
  }
  if (RadioCfg::DEBUG_RADIO) {
    Serial.printf("[AUDIO] DAC gpio=%d%s rate=%u len=%u gain=%.1f loud=%d\n",
                  Pins::DAC_OUT_PIN,
                  AudioCfg::USE_SECOND_DAC ? "+26" : "",
                  (unsigned)JINGLE_SAMPLE_RATE, (unsigned)JINGLE_LEN,
                  AudioCfg::DAC_GAIN, AudioCfg::LOUD_MODE ? 1 : 0);
  }
}

bool audioIsPlaying() {
  return playing;
}

void audioStop() {
  playing = false;
  playIndex = 0;
  dacWrite(Pins::DAC_OUT_PIN, AudioCfg::DAC_IDLE_LEVEL);
  if (AudioCfg::USE_SECOND_DAC) {
    dacWrite(Pins::DAC2_OUT_PIN, AudioCfg::DAC_IDLE_LEVEL);
  }
  if (mutedRadioForPlay) {
    mutedRadioForPlay = false;
    if (radioIsReady() && !radioIsBusy()) {
      radioSetMute(false);
    }
  }
}

void audioPlayJingle() {
  if (playing) return;

  if (AudioCfg::MUTE_RADIO_WHILE_PLAYING && radioIsReady() && !radioIsBusy()) {
    if (radioSetMute(true)) {
      mutedRadioForPlay = true;
    }
  }

  playIndex = 0;
  playing = true;
  nextSampleUs = micros();
  Serial.println("[AUDIO] Jingle basladi (DAC loud)");
}

void audioUpdate() {
  if (!playing) return;

  const unsigned long now = micros();
  while (playing && (long)(now - nextSampleUs) >= 0) {
    if (playIndex >= JINGLE_LEN) {
      audioStop();
      Serial.println("[AUDIO] Jingle bitti");
      return;
    }
    const uint8_t s = shapeSample(JINGLE_PCM[playIndex++]);
    dacWrite(Pins::DAC_OUT_PIN, s);
    if (AudioCfg::USE_SECOND_DAC) {
      dacWrite(Pins::DAC2_OUT_PIN, s);
    }
    nextSampleUs += SAMPLE_PERIOD_US;
  }
}
