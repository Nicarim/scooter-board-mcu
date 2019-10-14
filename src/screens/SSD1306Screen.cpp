#include "SSD1306Screen.h"

SSD1306Screen::SSD1306Screen(int cs, int dc) {
  lcdDisp = new U8G2_SSD1306_128X64_NONAME_F_4W_HW_SPI(U8G2_R0, cs, dc);
  lcdDisp->begin();
  lcdDisp->setFont(u8g2_font_artossans8_8r);
}

void SSD1306Screen::DisplayDebug(mijiaPacket *p) {
  int8_t heightJump = lcdDisp->getMaxCharHeight();
  lcdDisp->clearBuffer();
  sprintf(textBuffer, "C: %x", p->command);
  lcdDisp->drawStr(0, heightJump * 1, textBuffer);
  sprintf(textBuffer, "L: %x", p->length);
  lcdDisp->drawStr(0, heightJump * 2 + 5, textBuffer);
  sprintf(textBuffer, "S: %x", p->source);
  lcdDisp->drawStr(0, heightJump * 3 + 5, textBuffer);
  for (int i = 0; (i < p->payloadLength && i < 4); i++) {
    sprintf(textBuffer, "A%d:%x", i, p->payloadData[i]);
    lcdDisp->drawStr(0, heightJump * (4 + i) + 5, textBuffer);
  }

  for (int i = 4; (i < p->payloadLength && i < 10); i++) {
    sprintf(textBuffer, "A%d:%x", i, p->payloadData[i]);
    lcdDisp->drawStr(60, heightJump * (i - 4), textBuffer);
  }
}

void SSD1306Screen::DisplayTriggers(ScooterInfo &sInfo) {
  lcdDisp->clearBuffer();
  // sprintf(textBuffer, "Bra: %d", sInfo.brake);
  float brakePercentage = (sInfo.brake - 40.0) / 154.0;
  if (brakePercentage >= 0.01) {
    lcdDisp->drawFrame(1, 1, 12, 62 * brakePercentage);
  }

  if (sInfo.scooterName[0] != 0) {
    sprintf(textBuffer, "%.12s", sInfo.scooterName);
    lcdDisp->drawStr(16, 8, textBuffer);
  }

  float throttlePercentage = (sInfo.thorttle - 40.0) / 155.0;
  if (throttlePercentage >= 0.01) {
    lcdDisp->drawBox(118, 70 - (90.0 * throttlePercentage), 12,
                     90.0 * throttlePercentage);
  }
  double speedReal = (double)sInfo.speed / 1000.0f;
  if ((int)sInfo.speed < 0) {
    sprintf(textBuffer, "reverse");
  } else {
    sprintf(textBuffer, "%.1f km/h", speedReal);
  }
  lcdDisp->drawStr(20, 45, textBuffer);

  lcdDisp->sendBuffer();
}

void SSD1306Screen::DisplayHello() {
  lcdDisp->clearBuffer();

  lcdDisp->drawStr(0, 20, "Your Scooter");
  lcdDisp->drawStr(0, 40, "is ready");
  lcdDisp->sendBuffer();
}