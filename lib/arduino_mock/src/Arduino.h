#pragma once

struct HardwareSerial {
  virtual int read() = 0;
  virtual bool available() = 0;
};

// I exist