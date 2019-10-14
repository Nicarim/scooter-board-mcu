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

enum SourceAddress {
  BLE = 0x20,
  SCOOTER = 0x21,
  SOURCE_ESC = 0x23,
  BMS_REQUEST = 0x22,
  BMS_RESPONSE = 0x25,
};

class M365UartReciever {
private:
  mijiaCommState *commState;
  HardwareSerial *miSerial;
  uint8_t *packetCursor;
  uint8_t *recievedData;

public:
  M365UartReciever() {
    this->commState = nullptr;
    this->miSerial = nullptr;
    this->packetCursor = nullptr;
    this->recievedData = new uint8_t[0xFF]{0};
  };

  M365UartReciever(uint8_t *recievedData) {
    this->commState = nullptr;
    this->miSerial = nullptr;
    this->packetCursor = nullptr;
    this->recievedData = recievedData;
  };

  M365UartReciever(mijiaCommState *commState, HardwareSerial *miSerial,
                   uint8_t *packetCursor) {
    this->commState = commState;
    this->miSerial = miSerial;
    this->packetCursor = packetCursor;
    this->recievedData = new uint8_t[0xFF]{0};
  };

  void InitializeWithDefault();
  void RecieveScooterData();
  void ResetCommState();
  mijiaPacket *CreatePacketFromRecieved();
  static void PrintMijiaPacketToSerial(HardwareSerial *serial, mijiaPacket *p,
                                       char prefix);

  uint8_t *GetPacketCursor() { return this->packetCursor; };
  void SetPacketCursor(uint8_t *packetCursor) {
    this->packetCursor = packetCursor;
  };

  void SetMiSerial(HardwareSerial *miSerial) { this->miSerial = miSerial; };

  void SetCommState(mijiaCommState *commState) { this->commState = commState; };

  uint8_t *GetRecievedData() { return this->recievedData; }

  bool HasCompletedData() { return this->commState->hasCompletedPacket; }
};