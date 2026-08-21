#include "Favorites.h"
#include "Data.h"
#include "Radio.h"
#include "Led.h"
#include <Preferences.h>

static Preferences prefs;

void favoritesInit() {
  for (int i = 0; i < FavCfg::COUNT; i++) {
    g_fav.freq[i] = 0;
    g_fav.valid[i] = false;
  }
  favoritesLoad();
}

void favoritesLoad() {
  prefs.begin(FavCfg::NVS_NAMESPACE, true);
  for (int i = 0; i < FavCfg::COUNT; i++) {
    char key[8];
    char keyOk[10];
    snprintf(key, sizeof(key), "f%d", i);
    snprintf(keyOk, sizeof(keyOk), "f%dok", i);
    g_fav.valid[i] = prefs.getBool(keyOk, false);
    g_fav.freq[i] = prefs.getFloat(key, 0.0f);
    if (g_fav.freq[i] < RadioCfg::FREQ_MIN_MHZ ||
        g_fav.freq[i] > RadioCfg::FREQ_MAX_MHZ) {
      g_fav.valid[i] = false;
    }
  }
  prefs.end();
}

bool favoritesSaveSlot(int slot, float mhz) {
  if (slot < 0 || slot >= FavCfg::COUNT) return false;

  mhz = radioChannelToFreq(radioFreqToChannel(mhz));

  prefs.begin(FavCfg::NVS_NAMESPACE, false);
  char key[8];
  char keyOk[10];
  snprintf(key, sizeof(key), "f%d", slot);
  snprintf(keyOk, sizeof(keyOk), "f%dok", slot);

  const size_t n1 = prefs.putFloat(key, mhz);
  const size_t n2 = prefs.putBool(keyOk, true);
  prefs.end();

  if (n1 == 0 || n2 == 0) {
    Serial.println("Favori NVS yazma hatasi");
    return false;
  }

  g_fav.freq[slot] = mhz;
  g_fav.valid[slot] = true;
  return true;
}

bool favoritesIsValid(int slot) {
  if (slot < 0 || slot >= FavCfg::COUNT) return false;
  return g_fav.valid[slot];
}

float favoritesGetFreq(int slot) {
  if (!favoritesIsValid(slot)) return 0;
  return g_fav.freq[slot];
}

bool favoritesGotoSlot(int slot) {
  if (!favoritesIsValid(slot)) {
    Serial.println("Favori bos. Kaydetmek icin yildiz tusunu basili tut.");
    ledBlink(LedCfg::FAV_EMPTY_BLINKS, LedCfg::FAV_EMPTY_ON_MS, LedCfg::FAV_EMPTY_OFF_MS);
    return false;
  }
  if (radioIsBusy()) {
    Serial.println("Seek/tune sirasinda favori degistirilemez");
    return false;
  }
  Serial.printf("Favori %d -> %.1f MHz\n", slot + 1, g_fav.freq[slot]);
  return radioSetFrequency(g_fav.freq[slot]);
}

bool favoritesSaveCurrent() {
  const int slot = FavCfg::ACTIVE_SLOT;
  if (!favoritesSaveSlot(slot, g_radio.freqMHz)) return false;
  Serial.printf("Favori %d kaydedildi: %.1f MHz\n", slot + 1, g_fav.freq[slot]);
  ledBlink(LedCfg::FAV_SAVE_BLINKS, LedCfg::FAV_SAVE_ON_MS, LedCfg::FAV_SAVE_OFF_MS);
  return true;
}
