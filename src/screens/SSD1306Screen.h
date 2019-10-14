#pragma once

#include "ScooterInfo.h"
#include "comm/m365_uart.h"
#include <Arduino.h>
#include <U8g2lib.h>

class SSD1306Screen {
private:
  U8G2_SSD1306_128X64_NONAME_F_4W_HW_SPI *lcdDisp;
  char textBuffer[100];

public:
  SSD1306Screen(int cs, int dc);

  void DisplayDebug(mijiaPacket *p);
  void DisplayTriggers(ScooterInfo &sInfo);
  void DisplayHello();
};