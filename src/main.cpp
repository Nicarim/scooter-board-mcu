#include <Arduino.h>
#include <MemoryFree.h>
#include <U8g2lib.h>
#include <uart_reader.h>

U8G2_SSD1306_128X64_NONAME_F_4W_HW_SPI lcdDisp(U8G2_R0, 49, 48);

char textBuffer[100];
bool lcdHelloPrinted = false;

struct scooterInfo {
  int thorttle = 0;
  int brake = 0;
} sInfo;

void setup() {
  // put your setup code here, to run once:
  Serial1.begin(115200);
  lcdDisp.begin();
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
    lcdDisp.drawFrame(3, 1, 15, 62 * brakePercentage);
  }

  float throttlePercentage = (sInfo.thorttle - 40.0) / 155.0;
  if (throttlePercentage >= 0.01) {
    lcdDisp.drawBox(112, 70 - (90.0 * throttlePercentage), 15,
                    90.0 * throttlePercentage);
  }

  lcdDisp.sendBuffer();
}

void loop() {
  // put your main code here, to run repeatedly:
  recieveScooterData(&Serial1, &g_commState, recievedData, &packetCursor);
  if (g_commState.hasCompletedPacket) {
    mijiaPacket *p =
        create_packet_from_array(recievedData, recievedData[2] + 6);

    int8_t heightJump = lcdDisp.getMaxCharHeight();
    // int8_t widthJump = lcdDisp.getMaxCharWidth();
    if (!p->validPacket) {
      print_packet_to_serial(p, 'E');
    } else if (p->command == 0x64 && p->source == 0x20) {
      // print_packet_to_serial(p, 'D');
      display_triggers();
      save_incoming_ble_data(p);
    }
    reset_comm_state(&g_commState);
    packetCursor = 0;
    delete p;
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
