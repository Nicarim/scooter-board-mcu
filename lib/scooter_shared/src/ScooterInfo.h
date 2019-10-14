#pragma once

#include <stdint.h>

struct ScooterInfo {
  uint8_t thorttle = 0;
  uint8_t brake = 0;
  uint16_t speed = 0;
  char scooterName[255] = "";
};