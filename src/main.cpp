#include "ScooterInfo.h"
#include "comm/m365_uart.h"
#include "screens/SSD1306Screen.h"
#include <Arduino.h>
#include <MemoryFree.h>

SSD1306Screen *scr;

#define ENABLE_WRITE_PIN 22

bool lcdHelloPrinted = false;
unsigned long lastSuccess;
unsigned long startSend;
bool messageSent = true;
M365UartReciever *uartReciever;
mijiaCommState g_commState;
uint8_t packetCursor = 0;
unsigned long timeoutForHello = 3000000;
String logBuffer;
ScooterInfo sInfo;

void setup() {
  // put your setup code here, to run once:
  Serial1.begin(115200);

  Serial.begin(192000);
  Serial.println("Starting sniffing");
  uartReciever = new M365UartReciever();
  uartReciever->SetMiSerial(&Serial1);
  uartReciever->SetCommState(&g_commState);
  uartReciever->SetPacketCursor(&packetCursor);
  scr = new SSD1306Screen(49, 48);
  pinMode(ENABLE_WRITE_PIN, OUTPUT);
  lastSuccess = micros();
}

void print_packet_to_serial(mijiaPacket *p, char prefix) {
  M365UartReciever::PrintMijiaPacketToSerial(&Serial, p, prefix);
}

void display_debug_screen(mijiaPacket *p) { scr->DisplayDebug(p); }

void loop() {
  unsigned long startedAt = micros();
  if ((micros() - lastSuccess) > 2500 && !messageSent) {
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
      startSend = micros();
      digitalWrite(ENABLE_WRITE_PIN, HIGH);
      Serial1.write(dataToSend, 12);
      // this waits until whole buffer is send out. see
      // https://forum.arduino.cc/index.php?topic=372226.msg2566999#msg2566999
      // clang-format off
      while (!(UCSR1A & (1 << TXC1)));
      // clang-format on
      digitalWrite(ENABLE_WRITE_PIN, LOW);
      lastSuccess = micros();
      messageSent = true;
      scr->DisplayTriggers(sInfo);
    }
  }

  uartReciever->RecieveScooterData();
  if (uartReciever->HasCompletedData()) {
    mijiaPacket *p = uartReciever->CreatePacketFromRecieved();

    if (!p->validPacket) {
      print_packet_to_serial(p, 'E');
    } else {
      if (p->command == 0x01 && p->source == 0x24) {
        for (int i = 0; i < p->payloadLength; i++) {
          sInfo.scooterName[i] = (char)p->payloadData[i];
        }
      } else if (p->command == 0x01 && p->source == 0x23 &&
                 p->argument == 0xB0) {
        sInfo.speed = (p->payloadData[11] << 8) | p->payloadData[10];
      } else if (p->command == 0x64 && p->source == 0x20) {
        // print_packet_to_serial(p, 'D');
        sInfo.brake = p->payloadData[2];
        sInfo.thorttle = p->payloadData[1];
        lastSuccess = micros();
        messageSent = false;
      }
      // print_packet_to_serial(p, 'D');
    }
    uartReciever->ResetCommState();
    packetCursor = 0;
    delete p;
  }
  unsigned long endedAt = micros();
  if (endedAt - startedAt > 60) {
    // Serial.print("Main Recieving and sending takes: ");
    // Serial.println(endedAt - startedAt);
    if (logBuffer.length() > 0) {
      Serial.print(logBuffer);
      logBuffer = "";
    }
  }

  if (!lcdHelloPrinted) {
    scr->DisplayHello();
    lcdHelloPrinted = true;
  }
}
