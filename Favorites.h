#pragma once

#include <Arduino.h>

void favoritesInit();
void favoritesLoad();
bool favoritesSaveSlot(int slot, float mhz);
bool favoritesGotoSlot(int slot);
bool favoritesSaveCurrent();
bool favoritesIsValid(int slot);
float favoritesGetFreq(int slot);
