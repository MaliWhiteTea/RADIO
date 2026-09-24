#pragma once

#include <Arduino.h>

void audioInit();
void audioUpdate();          // loop: non-blocking PCM -> DAC
void audioPlayJingle();      // baslat
bool audioIsPlaying();
void audioStop();
