#pragma once
#include <stdint.h>
#include <string>

struct HardwareSerial {
  virtual int read() = 0;
  virtual int available() = 0;
  virtual void print(const char *a) = 0;
  virtual void print(uint8_t a) = 0;

  virtual void println(const char *a) = 0;
};

// I exist