#include "Led.h"
#include "Data.h"

static int blinkLeft = 0;
static int blinkOnMs = LedCfg::BLINK_ON_MS;
static int blinkOffMs = LedCfg::BLINK_OFF_MS;
static bool blinkLedOn = false;
static bool blinkRestore = false;
static unsigned long blinkNextMs = 0;

void ledInit() {
  blinkLeft = 0;
  if (RadioCfg::DEBUG_RADIO) {
    Serial.printf("[LED] pinMode OUTPUT gpio=%d\n", Pins::LED_PIN);
  }
  pinMode(Pins::LED_PIN, OUTPUT);
  ledSet(LedCfg::START_ON);
}

void ledSet(bool on) {
  blinkLeft = 0;
  g_led.on = on;
  const bool level = LedCfg::ACTIVE_HIGH ? on : !on;
  digitalWrite(Pins::LED_PIN, level ? HIGH : LOW);
}

void ledToggle() {
  ledSet(!g_led.on);
}

bool ledIsOn() {
  return g_led.on;
}

void ledBlink(int times, int onMs, int offMs) {
  if (times <= 0) return;
  if (onMs < 0) onMs = LedCfg::BLINK_ON_MS;
  if (offMs < 0) offMs = LedCfg::BLINK_OFF_MS;

  blinkRestore = g_led.on;
  blinkOnMs = onMs;
  blinkOffMs = offMs;
  blinkLeft = times;
  blinkLedOn = true;
  blinkNextMs = millis() + (unsigned long)blinkOnMs;

  digitalWrite(Pins::LED_PIN, LedCfg::ACTIVE_HIGH ? HIGH : LOW);
}

void ledUpdate() {
  if (blinkLeft <= 0) return;

  const unsigned long now = millis();
  if ((long)(now - blinkNextMs) < 0) return;

  if (blinkLedOn) {
    digitalWrite(Pins::LED_PIN, LedCfg::ACTIVE_HIGH ? LOW : HIGH);
    blinkLedOn = false;
    blinkNextMs = now + (unsigned long)blinkOffMs;
  } else {
    blinkLeft--;
    if (blinkLeft <= 0) {
      ledSet(blinkRestore);
      return;
    }
    digitalWrite(Pins::LED_PIN, LedCfg::ACTIVE_HIGH ? HIGH : LOW);
    blinkLedOn = true;
    blinkNextMs = now + (unsigned long)blinkOnMs;
  }
}
