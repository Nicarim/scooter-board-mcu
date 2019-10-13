#include "comm/m365_uart.h"
#include <Arduino.h>
#include <MemoryFree.h>
#include <U8g2lib.h>

U8G2_SSD1306_128X64_NONAME_F_4W_HW_SPI lcdDisp(U8G2_R0, 49, 48);

#define ENABLE_WRITE_PIN 22

char textBuffer[100];
bool lcdHelloPrinted = false;
unsigned long lastSuccess;
unsigned long startSend;
bool messageSent = false;
bool sentGateEnabled = false;

struct scooterInfo {
  uint8_t thorttle = 0;
  uint8_t brake = 0;
  uint16_t speed = 0;
  char scooterName[255] = "";
} sInfo;

void setup() {
  // put your setup code here, to run once:
  Serial1.begin(115200);
  pinMode(ENABLE_WRITE_PIN, OUTPUT);
  lcdDisp.begin();
  lastSuccess = millis();
  Serial.begin(192000);
  Serial.println("Starting sniffing");
}

mijiaCommState g_commState;

uint8_t packetCursor = 0;

uint8_t recievedData[0xFF]; // maximum length of a packet

void save_incoming_ble_data(mijiaPacket *p) {
  sInfo.brake = p->payloadData[2];
  sInfo.thorttle = p->payloadData[1];
}

void print_packet_to_serial(mijiaPacket *p, char prefix) {
  // Structure is assumed as: --<prefix>: v1,v2,v3...vn

  Serial.print("-*-");
  Serial.print(prefix);
  Serial.print(": ");
  Serial.print(p->length);
  Serial.print(",");

  Serial.print(p->source);
  Serial.print(",");

  Serial.print(p->command);
  Serial.print(",");

  Serial.print(p->argument);
  Serial.print(",");

  Serial.print(p->payloadLength);
  Serial.print(",");

  for (int i = 0; i < p->payloadLength; i++) {
    Serial.print(p->payloadData[i]);
    Serial.print(",");
  }

  Serial.print(p->originChecksum);
  Serial.print(",");

  Serial.print(p->actualChecksum);
  Serial.println("");
}

void display_debug_screen(mijiaPacket *p) {
  Serial.print("Free memory is: ");
  Serial.println(freeMemory());

  int8_t heightJump = lcdDisp.getMaxCharHeight();
  lcdDisp.clearBuffer();
  sprintf(textBuffer, "C: %x", p->command);
  lcdDisp.drawStr(0, heightJump * 1, textBuffer);
  sprintf(textBuffer, "L: %x", p->length);
  lcdDisp.drawStr(0, heightJump * 2 + 5, textBuffer);
  sprintf(textBuffer, "S: %x", p->source);
  lcdDisp.drawStr(0, heightJump * 3 + 5, textBuffer);
  for (int i = 0; (i < p->payloadLength && i < 4); i++) {
    sprintf(textBuffer, "A%d:%x", i, p->payloadData[i]);
    lcdDisp.drawStr(0, heightJump * (4 + i) + 5, textBuffer);
  }

  for (int i = 4; (i < p->payloadLength && i < 10); i++) {
    sprintf(textBuffer, "A%d:%x", i, p->payloadData[i]);
    lcdDisp.drawStr(60, heightJump * (i - 4), textBuffer);
  }
}

void display_triggers() {
  lcdDisp.clearBuffer();
  // sprintf(textBuffer, "Bra: %d", sInfo.brake);
  float brakePercentage = (sInfo.brake - 40.0) / 154.0;
  if (brakePercentage >= 0.01) {
    lcdDisp.drawFrame(1, 1, 12, 62 * brakePercentage);
  }

  if (sInfo.scooterName[0] != 0) {
    sprintf(textBuffer, "%.12s", sInfo.scooterName);
    lcdDisp.drawStr(16, 8, textBuffer);
  }

  float throttlePercentage = (sInfo.thorttle - 40.0) / 155.0;
  if (throttlePercentage >= 0.01) {
    lcdDisp.drawBox(118, 70 - (90.0 * throttlePercentage), 12,
                    90.0 * throttlePercentage);
  }
  double speedReal = ((double)sInfo.speed) / 1000.0f;
  if ((int)sInfo.speed < 0) {
    sprintf(textBuffer, "reverse");
  } else {
    sprintf(textBuffer, "%.1f km/h", speedReal);
  }
  lcdDisp.drawStr(20, 45, textBuffer);

  lcdDisp.sendBuffer();
}

void loop() {
  // put your main code here, to run repeatedly:
  recieveScooterData(&Serial1, &g_commState, recievedData, &packetCursor);
  if (g_commState.hasCompletedPacket) {
    mijiaPacket *p =
        create_packet_from_array(recievedData, recievedData[2] + 6);

    // int8_t heightJump = lcdDisp.getMaxCharHeight();
    // int8_t widthJump = lcdDisp.getMaxCharWidth();
    if (!p->validPacket) {
      print_packet_to_serial(p, 'E');
    } else if (p->command == 0x01 && p->source == 0x24) {
      for (int i = 0; i < p->payloadLength; i++) {
        sInfo.scooterName[i] = (char)p->payloadData[i];
      }
    } else if (p->command == 0x01 && p->source == 0x23 && p->argument == 0xB0) {
      sInfo.speed = (p->payloadData[11] << 8) | p->payloadData[10];
    } else if (p->command == 0x64 && p->source == 0x20) {
      // print_packet_to_serial(p, 'D');
      save_incoming_ble_data(p);
      lastSuccess = millis();
      messageSent = false;
      display_triggers();
    } else {
      print_packet_to_serial(p, 'D');
    }

    reset_comm_state(&g_commState);
    packetCursor = 0;
    delete p;
  }
  if (startSend - millis() > 1 && sentGateEnabled) {
    digitalWrite(ENABLE_WRITE_PIN, LOW);
    sentGateEnabled = false;
  }
  if (lastSuccess - millis() > 3 && !messageSent) {
    if (Serial1.availableForWrite() > 0) {
      // TODO: This need a refactor, but is a copy for now to test it out
      // uint8_t dataToSend[10] = {0x55, 0xAA, 0x04, 0x22,
      //                           0x01, 0x31, 0x32, 0x00, // data packets
      //                           0x00, 0x00};            // CRC to be fileld

      uint8_t dataToSend[12] = {
          0x55,           0xAA,        0x06, 0x20,
          0x61,           0xB0,        0x20, // 5(offset), 6(len)
          0x02,                              // 7
          sInfo.thorttle, sInfo.brake,       // 8(throttle), 9(brake)
          0xB7,           0xFF               // CRC
      };
      uint16_t crccalc = 0x00;
      for (int i = 2; i < 10; i++) {
        crccalc = crccalc + dataToSend[i];
      }
      crccalc = crccalc ^ 0xffff;
      dataToSend[10] = (uint8_t)(crccalc & 0xff);
      dataToSend[11] = (uint8_t)((crccalc & 0xff00) >> 8);
      digitalWrite(ENABLE_WRITE_PIN, HIGH);
      startSend = millis();
      sentGateEnabled = true;
      Serial1.write(dataToSend, 12);
      lastSuccess = millis();
      messageSent = true;
    }
  }

  if (!lcdHelloPrinted) {
    lcdDisp.clearBuffer();
    lcdDisp.setFont(u8g2_font_artossans8_8r);
    lcdDisp.drawStr(0, 20, "Your Scooter");
    lcdDisp.drawStr(0, 40, "is ready");
    lcdDisp.sendBuffer();
    lcdHelloPrinted = true;
  }
}
