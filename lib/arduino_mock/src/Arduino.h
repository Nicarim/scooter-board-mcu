#pragma once

struct HardwareSerial {
  virtual int read() = 0;
  virtual int available() = 0;
};

// I exist