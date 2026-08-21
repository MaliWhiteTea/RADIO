#pragma once

#include <Arduino.h>

enum class RadioSeekEvent : uint8_t {
  None,
  Ok,
  NoStation,
  Timeout
};

enum class RadioTuneEvent : uint8_t {
  None,
  Ok,
  Timeout,
  ChannelMismatch,
  I2cError
};

enum class RadioInitError : uint8_t {
  None,
  SoftResetI2c,
  EnableI2c,
  TuneStart,
  TuneStcTimeout,
  TuneChannelMismatch,
  TuneI2cError,
  MissingTuneEvent,
  NotInitialized
};

bool radioInit();
void radioUpdate();

bool radioIsReady();
bool radioIsBusy();
bool radioIsSeeking();
bool radioIsTuning();

RadioInitError radioGetInitError();
const char *radioInitErrorStr(RadioInitError err);

bool radioSetFrequency(float mhz);
bool radioSetVolume(uint8_t vol);
bool radioSetMute(bool mute);
bool radioSetBass(bool on);
bool radioSetMono(bool on);
bool radioApply(bool tunePulse);

bool radioSeekStart(bool up);
RadioSeekEvent radioConsumeSeekEvent();
RadioTuneEvent radioConsumeTuneEvent();

bool radioReadRssi(uint8_t &rssi);

uint16_t radioFreqToChannel(float mhz);
float radioChannelToFreq(uint16_t channel);
