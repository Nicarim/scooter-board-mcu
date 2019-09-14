#include <Arduino.h>
#include <MemoryFree.h>
#include <U8g2lib.h>
#include <uart_reader.h>

U8G2_SSD1306_128X64_NONAME_F_4W_HW_SPI lcdDisp(U8G2_R0, 49, 48);

char textBuffer[100];
bool lcdHelloPrinted = false;

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

void loop() {
  // put your main code here, to run repeatedly:
  recieveScooterData(&Serial1, &g_commState, recievedData, &packetCursor);
  if (g_commState.hasCompletedPacket) {
    mijiaPacket *p =
        create_packet_from_array(recievedData, recievedData[2] + 6);
    Serial.print("Free memory is: ");
    Serial.println(freeMemory());
    int8_t heightJump = lcdDisp.getMaxCharHeight();
    if (!p->validPacket) {
      Serial.print("Data validity is: ");
      Serial.println("BAD!!!");
      Serial.println(p->validPacketError);
      Serial.println("Invalid Packet is: ");
      Serial.print(p->length, HEX);
      Serial.print(p->command, HEX);
      Serial.print(p->actualChecksum, HEX);
      Serial.print(p->originChecksum, HEX);
      Serial.println("----- END");

    } else {
      lcdDisp.clearBuffer();
      sprintf(textBuffer, "Got pack [%x]", p->command);
      lcdDisp.drawStr(0, heightJump * 1, textBuffer);
      sprintf(textBuffer, "L[%x]", p->length);
      lcdDisp.drawStr(0, heightJump * 2 + 5, textBuffer);
      sprintf(textBuffer, "S[%x]", p->source);
      lcdDisp.drawStr(0, heightJump * 3 + 5, textBuffer);
      lcdDisp.sendBuffer();
    }
    delete p;
    g_commState.hasCompletedPacket = false;
  }
  if (!lcdHelloPrinted) {
    lcdDisp.clearBuffer();
    lcdDisp.setFont(u8g2_font_artossans8_8r);
    lcdDisp.drawStr(0, 20, "Your Scooter is ready");
    lcdDisp.sendBuffer();
    lcdHelloPrinted = true;
  }
}
