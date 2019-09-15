#include "uart_reader.h"

void recieveScooterData(HardwareSerial *miSerial, mijiaCommState *commState,
                        uint8_t *recievedData, uint8_t *packetCursor) {
  if (commState->hasCompletedPacket) {
    // We need to process current packet before we go further.
    return;
  }
  if (miSerial->available() && !commState->hasPreambleFirst &&
      !commState->hasPreambleSecond) {
    recievedData[0] = miSerial->read();
    if (recievedData[0] == miSignatureFirst) {
      commState->hasPreambleFirst = true;
    }
  }

  if (miSerial->available() && commState->hasPreambleFirst &&
      !commState->hasPreambleSecond) {
    recievedData[1] = miSerial->read();
    if (recievedData[1] == miSignatureSecond) {
      commState->hasPreambleSecond = true;
    } else {
      commState->hasPreambleFirst = false;
    }
  }

  if (miSerial->available() > 0 && commState->hasPreambleFirst &&
      commState->hasPreambleSecond && !commState->hasLength) {
    recievedData[2] = miSerial->read();
    commState->hasLength = true;
  }
  if (miSerial->available() > 0 && commState->hasLength &&
      !commState->hasCompletedBeforeCRC) {
    recievedData[3 + *packetCursor] = miSerial->read();
    ++*packetCursor;
    if (*packetCursor >= recievedData[2]) {
      commState->hasCompletedBeforeCRC = true;
    }
  }
  if (miSerial->available() > 1 && commState->hasCompletedBeforeCRC) {
    recievedData[3 + *packetCursor] = miSerial->read();
    recievedData[3 + *packetCursor + 1] = miSerial->read();
    commState->hasPreambleFirst = false;
    commState->hasPreambleSecond = false;
    commState->hasLength = false;
    commState->hasCompletedBeforeCRC = false;
    *packetCursor = 0;
    commState->hasCompletedPacket = true;
  }
}

mijiaPacket *create_packet_from_array(uint8_t *data, uint8_t arraySize) {

  mijiaPacket *p = new mijiaPacket();

  if (arraySize < 3) {
    p->validPacket = false;
    p->validPacketError = 1;
    return p;
  }
  p->sig1 = data[0];
  p->sig2 = data[1];

  p->length = data[2];
  p->actualChecksum += p->length;

  if (p->length != arraySize - 6) {
    // "- 6" because of: preamble, length itself, CRC and source
    p->validPacket = false;
    p->validPacketError = 2;
    return p;
  }

  p->source = data[3];
  p->actualChecksum += p->source;

  p->command = data[4];
  p->actualChecksum += p->command;

  p->argument = data[5];
  p->actualChecksum += p->argument;

  p->payloadLength = p->length - 2;
  if (p->payloadLength > 0) {
    for (int i = 0; i < p->payloadLength; i++) {
      p->payloadData[i] = data[6 + i];
      p->actualChecksum += p->payloadData[i];
    }
  }
  int offset = 6 + p->payloadLength;

  p->originChecksum = data[offset + 1] * 256 + data[offset];
  p->actualChecksum = 0xFFFF ^ p->actualChecksum;

  p->validPacket = (p->originChecksum == p->actualChecksum);
  p->validPacketError = (p->validPacket == true) ? 0 : 3;

  return p;
}