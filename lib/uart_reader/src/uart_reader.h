#pragma once

#include <Arduino.h>
#include <stdint.h>

const uint8_t miSignatureFirst = 0x55;
const uint8_t miSignatureSecond = 0xAA;

const uint8_t escSignatureFirst = 0x5A;
const uint8_t escSignatureSecond = 0xA5;

struct mijiaPacket {
  uint8_t sig1;
  uint8_t sig2;

  uint8_t length;
  uint8_t source;

  uint8_t command;
  uint8_t argument;

  uint8_t payloadLength;
  uint8_t payloadData[0xff];

  uint16_t originChecksum;
  uint16_t actualChecksum;

  bool validPacket;
  uint8_t validPacketError;
};

struct mijiaCommState {
  bool hasPreambleFirst = false;
  bool hasPreambleSecond = false;
  bool hasLength = false;
  bool hasCompletedBeforeCRC = false;
  bool hasCompletedPacket = false;
};

enum ControllerSource { MIJIA, ESC, UNKNOWN };

/*
 SCHEMATIC OF M365 CONTROLLERS
   +---------------------------------------------------------------------------------+
   |   +----------------+ | |   |      BLE       |   ESC                    BMS
 Response                      | |   |  Controller    +-------<-+ through ESC |
   |   +----------------+      |  |      +------------------------------+ | | |
 ^                |  |      | +-------------------------+  |            | | |  |
 |  |      | |        BMS Request      |  |            | |       |  | |  | v |
 Through ESC      v  |            | |       |  | SCOOTER(P) +---v--------------+
 +--------------------+  | |       |  +------------+      Engine      | |
 Battery Controller |  | |       |               | Controller (ESC) | | BMS |  |
   |       +-------------> +------------------+ +--------------------+  | | BLE
 (Passive)                                                           | | |
   +---------------------------------------------------------------------------------+

*/

enum SourceAddress {
  BLE = 0x20,
  SCOOTER = 0x21,
  SOURCE_ESC = 0x23,
  BMS_REQUEST = 0x22,
  BMS_RESPONSE = 0x25,
};

mijiaPacket *create_packet_from_array(uint8_t *data, uint8_t arraySize);
void recieveScooterData(HardwareSerial *miSerial, mijiaCommState *commState,
                        uint8_t *recievedData, uint8_t *packetCursor);