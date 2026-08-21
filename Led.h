#pragma once

#include <Arduino.h>

void ledInit();
void ledUpdate();              // loop: non-blocking blink
void ledSet(bool on);
void ledToggle();
bool ledIsOn();
// Blocking degil: kuyruga alir, ledUpdate ile yanip soner
void ledBlink(int times, int onMs = -1, int offMs = -1);
