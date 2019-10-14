#include "m365_uart.h"

void M365UartReciever::RecieveScooterData() {
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
    if (*packetCursor > recievedData[2]) {
      // First byte of data is not part of length,
      // therefor we use > not >= (so 0 indexed data, equals 1 indexed size)
      // These chinese...
      commState->hasCompletedBeforeCRC = true;
    }
  }
  if (miSerial->available() > 1 && commState->hasCompletedBeforeCRC) {
    recievedData[3 + *packetCursor] = miSerial->read();
    recievedData[4 + *packetCursor] = miSerial->read();
    commState->hasCompletedPacket = true;
  }
}

void M365UartReciever::ResetCommState() {
  commState->hasPreambleFirst = false;
  commState->hasPreambleSecond = false;
  commState->hasLength = false;
  commState->hasCompletedBeforeCRC = false;
  commState->hasCompletedPacket = false;
}

mijiaPacket *M365UartReciever::CreatePacketFromRecieved() {
  mijiaPacket *p = new mijiaPacket();
  int arraySize = recievedData[2] + 6;

  if (arraySize < 3) {
    p->validPacket = false;
    p->validPacketError = 1;
    return p;
  }
  p->sig1 = recievedData[0];
  p->sig2 = recievedData[1];

  p->length = recievedData[2];
  p->actualChecksum += p->length;

  if (p->length != arraySize - 6) {
    // "- 6" because of: preamble, length itself, CRC and source
    p->validPacket = false;
    p->validPacketError = 2;
    return p;
  }

  p->source = recievedData[3];
  p->actualChecksum += p->source;

  p->command = recievedData[4];
  p->actualChecksum += p->command;

  p->argument = recievedData[5];
  p->actualChecksum += p->argument;

  p->payloadLength = p->length - 2;
  if (p->payloadLength > 0) {
    for (int i = 0; i < p->payloadLength; i++) {
      p->payloadData[i] = recievedData[6 + i];
      p->actualChecksum += p->payloadData[i];
    }
  }
  int offset = 6 + p->payloadLength;

  p->originChecksum = recievedData[offset + 1] * 256 + recievedData[offset];
  p->actualChecksum = 0xFFFF ^ p->actualChecksum;

  p->validPacket = (p->originChecksum == p->actualChecksum);
  p->validPacketError = (p->validPacket == true) ? 0 : 3;

  return p;
}

void M365UartReciever::PrintMijiaPacketToSerial(HardwareSerial *serial,
                                                mijiaPacket *p, char prefix) {
  // Structure is assumed as: --<prefix>: v1,v2,v3...vn

  serial->print("-*-");
  serial->print(prefix);
  serial->print(": ");
  serial->print(p->length);
  serial->print(",");

  serial->print(p->source);
  serial->print(",");

  serial->print(p->command);
  serial->print(",");

  serial->print(p->argument);
  serial->print(",");

  serial->print(p->payloadLength);
  serial->print(",");

  for (int i = 0; i < p->payloadLength; i++) {
    serial->print(p->payloadData[i]);
    serial->print(",");
  }

  serial->print(p->originChecksum);
  serial->print(",");

  serial->print(p->actualChecksum);
  serial->println("");
}