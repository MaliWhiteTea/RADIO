#include "Data.h"

RadioState g_radio = {
  RadioCfg::DEFAULT_FREQ_MHZ,
  RadioCfg::DEFAULT_VOLUME,
  RadioCfg::DEFAULT_MUTED,
  RadioCfg::DEFAULT_BASS,
  RadioCfg::DEFAULT_MONO,
  RadioCfg::DEFAULT_FREQUENCY_VALID
};

LedState g_led = {
  LedCfg::START_ON
};

FavState g_fav = {};
