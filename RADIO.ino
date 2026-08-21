/*
  ESP32-WROOM-32E + RDA5807M FM radyo
*/

#include "Data.h"
#include "Led.h"
#include "Radio.h"
#include "Favorites.h"
#include "Buttons.h"
#include "SerialCmd.h"

void setup() {
  serialCmdInit();

  // Pin tesisi — GPIO output hatasindan ONCE gorunsun
  Serial.printf("PINS LED=%d UP=%d DOWN=%d SDA=%d SCL=%d\n",
                Pins::LED_PIN,
                Pins::SEEK_UP_PIN,
                Pins::SEEK_DOWN_PIN,
                Pins::I2C_SDA_PIN,
                Pins::I2C_SCL_PIN);

  favoritesInit();
  ledInit();
  buttonsInit();

  Serial.println();
  Serial.println("RDA5807M baslatiliyor...");

  if (!radioInit()) {
    Serial.printf("HATA: radio init (%s) SDA=%d SCL=%d\n",
                  radioInitErrorStr(radioGetInitError()),
                  Pins::I2C_SDA_PIN, Pins::I2C_SCL_PIN);
  } else {
    Serial.printf("I2C: 0x%02X OK | RDA hazir.\n", RadioCfg::I2C_ADDR_SEQ);
    serialPrintStatus();
  }

  serialPrintHelp();
}

void loop() {
  radioUpdate();
  ledUpdate();
  serialHandleRadioEvents();
  buttonsUpdate();
  serialCmdUpdate();
}
