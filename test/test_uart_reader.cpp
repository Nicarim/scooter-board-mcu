#include <Arduino.h>
#include <fakeit.hpp>
#include <unity.h>

using namespace fakeit;

#include "comm/m365_uart.h"

// TODO: convert all of it to classes.

void test_recieve_scooter_data_first_byte(void) {
  Mock<HardwareSerial> hw_mock;

  uint8_t packetCursor = 0;
  mijiaCommState _commState;

  Fake(Method(hw_mock, read));
  Fake(Method(hw_mock, available));

  When(Method(hw_mock, read)).Return(0x55);
  When(Method(hw_mock, available)).Return(true).AlwaysReturn(false);

  M365UartReciever *c =
      new M365UartReciever(&_commState, &hw_mock.get(), &packetCursor);

  TEST_ASSERT_FALSE(_commState.hasPreambleFirst);
  TEST_ASSERT_FALSE(_commState.hasPreambleSecond);

  c->RecieveScooterData();

  uint8_t *recievedData = c->GetRecievedData();

  TEST_ASSERT_EQUAL(recievedData[0], 0x55);
  TEST_ASSERT_TRUE(_commState.hasPreambleFirst);
  TEST_ASSERT_FALSE(_commState.hasPreambleSecond);
}

void test_recieve_scooter_data_whole_preamble_seperate(void) {
  Mock<HardwareSerial> hw_mock;

  uint8_t packetCursor = 0;
  mijiaCommState _commState;

  Fake(Method(hw_mock, read));
  Fake(Method(hw_mock, available));

  When(Method(hw_mock, read)).Return(0x55).Return(0xAA);
  When(Method(hw_mock, available))
      .Return(1, 4_Times(0), 5_Times(0), 2_Times(1), 3_Times(0));

  M365UartReciever *c =
      new M365UartReciever(&_commState, &hw_mock.get(), &packetCursor);

  TEST_ASSERT_FALSE(_commState.hasPreambleFirst);
  TEST_ASSERT_FALSE(_commState.hasPreambleSecond);

  c->RecieveScooterData();

  uint8_t *recievedData = c->GetRecievedData();

  TEST_ASSERT_EQUAL(recievedData[0], 0x55);
  TEST_ASSERT_NOT_EQUAL(recievedData[1], 0xAA);
  TEST_ASSERT_TRUE(_commState.hasPreambleFirst);
  TEST_ASSERT_FALSE(_commState.hasPreambleSecond);

  c->RecieveScooterData();

  TEST_ASSERT_EQUAL(recievedData[0], 0x55);
  TEST_ASSERT_NOT_EQUAL(recievedData[1], 0xAA);
  TEST_ASSERT_TRUE(_commState.hasPreambleFirst);
  TEST_ASSERT_FALSE(_commState.hasPreambleSecond);

  c->RecieveScooterData();

  TEST_ASSERT_EQUAL(recievedData[0], 0x55);
  TEST_ASSERT_EQUAL(recievedData[1], 0xAA);
  TEST_ASSERT_TRUE(_commState.hasPreambleFirst);
  TEST_ASSERT_TRUE(_commState.hasPreambleSecond);
}

void test_recieve_data_with_length_no_data(void) {
  Mock<HardwareSerial> hw_mock;

  uint8_t packetCursor = 0;
  mijiaCommState _commState;

  Fake(Method(hw_mock, read));
  Fake(Method(hw_mock, available));

  When(Method(hw_mock, read)).Return(0x55, 0xAA, 0x04);
  When(Method(hw_mock, available)).Return(3_Times(1)).AlwaysReturn(0);

  M365UartReciever *c =
      new M365UartReciever(&_commState, &hw_mock.get(), &packetCursor);

  TEST_ASSERT_FALSE(_commState.hasPreambleFirst);
  TEST_ASSERT_FALSE(_commState.hasPreambleSecond);
  TEST_ASSERT_FALSE(_commState.hasLength);

  c->RecieveScooterData();
  uint8_t *recievedData = c->GetRecievedData();

  TEST_ASSERT_TRUE(_commState.hasPreambleFirst);
  TEST_ASSERT_TRUE(_commState.hasPreambleSecond);
  TEST_ASSERT_TRUE(_commState.hasLength);

  TEST_ASSERT_EQUAL(recievedData[0], 0x55);
  TEST_ASSERT_EQUAL(recievedData[1], 0xAA);
  TEST_ASSERT_EQUAL(recievedData[2], 0x04);
}

void test_recieve_data_full_valid_packet(void) {
  Mock<HardwareSerial> hw_mock;

  uint8_t packetCursor = 0;
  mijiaCommState _commState;

  Fake(Method(hw_mock, read));
  Fake(Method(hw_mock, available));

  When(Method(hw_mock, read))
      .Return(0x55, 0xAA, 0x04, 0x01, 0x02, 0x03, 0x04, 0x05, 0xFF, 0xF0);
  When(Method(hw_mock, available))
      .Return(25_Times(1), 5_Times(2))
      .AlwaysReturn(0);

  M365UartReciever *c =
      new M365UartReciever(&_commState, &hw_mock.get(), &packetCursor);

  TEST_ASSERT_FALSE(_commState.hasPreambleFirst);
  TEST_ASSERT_FALSE(_commState.hasPreambleSecond);
  TEST_ASSERT_FALSE(_commState.hasLength);
  TEST_ASSERT_FALSE(_commState.hasCompletedBeforeCRC);
  TEST_ASSERT_FALSE(_commState.hasCompletedPacket);

  c->RecieveScooterData();
  uint8_t *recievedData = c->GetRecievedData();

  TEST_ASSERT_TRUE(_commState.hasPreambleFirst);
  TEST_ASSERT_TRUE(_commState.hasPreambleSecond);
  TEST_ASSERT_TRUE(_commState.hasLength);
  TEST_ASSERT_FALSE(_commState.hasCompletedBeforeCRC);
  TEST_ASSERT_FALSE(_commState.hasCompletedPacket);

  TEST_ASSERT_EQUAL(recievedData[0], 0x55);
  TEST_ASSERT_EQUAL(recievedData[1], 0xAA);
  TEST_ASSERT_EQUAL(recievedData[2], 0x04);
  TEST_ASSERT_EQUAL(recievedData[3], 0x01);
  TEST_ASSERT_NOT_EQUAL(recievedData[4], 0x02);

  c->RecieveScooterData();

  TEST_ASSERT_FALSE(_commState.hasCompletedBeforeCRC);
  TEST_ASSERT_TRUE(_commState.hasLength);
  TEST_ASSERT_EQUAL(recievedData[4], 0x02);
  TEST_ASSERT_NOT_EQUAL(recievedData[5], 0x03);

  c->RecieveScooterData();

  TEST_ASSERT_FALSE(_commState.hasCompletedBeforeCRC);
  TEST_ASSERT_TRUE(_commState.hasLength);
  TEST_ASSERT_EQUAL(recievedData[5], 0x03);
  TEST_ASSERT_NOT_EQUAL(recievedData[6], 0x04);

  c->RecieveScooterData();

  TEST_ASSERT_FALSE(_commState.hasCompletedBeforeCRC);
  TEST_ASSERT_TRUE(_commState.hasLength);
  TEST_ASSERT_EQUAL(recievedData[6], 0x04);
  TEST_ASSERT_NOT_EQUAL(recievedData[7], 0x05);
  TEST_ASSERT_NOT_EQUAL(recievedData[8], 0xFF);
  TEST_ASSERT_NOT_EQUAL(recievedData[9], 0xF0);

  c->RecieveScooterData();

  TEST_ASSERT_TRUE(_commState.hasCompletedBeforeCRC);
  TEST_ASSERT_FALSE(_commState.hasCompletedPacket);
  TEST_ASSERT_EQUAL(recievedData[7], 0x05);
  TEST_ASSERT_NOT_EQUAL(recievedData[8], 0xFF);
  TEST_ASSERT_NOT_EQUAL(recievedData[9], 0xF0);

  c->RecieveScooterData();

  TEST_ASSERT_TRUE(_commState.hasPreambleFirst);
  TEST_ASSERT_TRUE(_commState.hasPreambleSecond);
  TEST_ASSERT_TRUE(_commState.hasLength);
  TEST_ASSERT_TRUE(_commState.hasCompletedBeforeCRC);
  TEST_ASSERT_TRUE(_commState.hasCompletedPacket);
  TEST_ASSERT_EQUAL(recievedData[8], 0xFF);
  TEST_ASSERT_EQUAL(recievedData[9], 0xF0);

  c->ResetCommState();

  TEST_ASSERT_FALSE(_commState.hasPreambleFirst);
  TEST_ASSERT_FALSE(_commState.hasPreambleSecond);
  TEST_ASSERT_FALSE(_commState.hasLength);
  TEST_ASSERT_FALSE(_commState.hasCompletedBeforeCRC);
  TEST_ASSERT_FALSE(_commState.hasCompletedPacket);
}

void test_recieve_data_not_recieving_when_completed(void) {
  Mock<HardwareSerial> hw_mock;

  uint8_t packetCursor = 0;
  mijiaCommState _commState;
  _commState.hasCompletedPacket = true;

  Fake(Method(hw_mock, read));
  Fake(Method(hw_mock, available));

  When(Method(hw_mock, read)).AlwaysReturn(0x55);
  When(Method(hw_mock, available)).AlwaysReturn(1);

  M365UartReciever *c =
      new M365UartReciever(&_commState, &hw_mock.get(), &packetCursor);

  c->RecieveScooterData();
  TEST_ASSERT_TRUE(_commState.hasCompletedPacket);
  TEST_ASSERT_FALSE(_commState.hasPreambleFirst);
  TEST_ASSERT_FALSE(_commState.hasPreambleSecond);
  TEST_ASSERT_FALSE(_commState.hasLength);
  TEST_ASSERT_FALSE(_commState.hasCompletedBeforeCRC);
  VerifyNoOtherInvocations(Method(hw_mock, read));
}

void test_packet_create_from_recieved_data(void) {
  Mock<HardwareSerial> hw_mock;

  uint8_t packetCursor = 0;
  mijiaCommState _commState;

  Fake(Method(hw_mock, read));
  Fake(Method(hw_mock, available));

  When(Method(hw_mock, read))
      .Return(0x55, 0xAA,                   // scooter identifier 0, 1
              0x07,                         // length 2
              0x20,                         // source - which device 3
              0x65,                         // command issued or answered 4
              0x00,                         // command argument 5
              0x04, 0x1D, 0x13, 0x00, 0x00, // payload 6, 7, 8, 9, 10,
              0x3F, 0xFF);                  // CRC 11, 12
  When(Method(hw_mock, available))
      .Return(40_Times(1), 5_Times(2))
      .AlwaysReturn(0);
  M365UartReciever *c =
      new M365UartReciever(&_commState, &hw_mock.get(), &packetCursor);

  TEST_ASSERT_FALSE(_commState.hasPreambleFirst);
  TEST_ASSERT_FALSE(_commState.hasPreambleSecond);
  TEST_ASSERT_FALSE(_commState.hasLength);
  TEST_ASSERT_FALSE(_commState.hasCompletedBeforeCRC);
  TEST_ASSERT_FALSE(_commState.hasCompletedPacket);

  c->RecieveScooterData();
  uint8_t *recievedData = c->GetRecievedData();

  TEST_ASSERT_TRUE(_commState.hasPreambleFirst);
  TEST_ASSERT_TRUE(_commState.hasPreambleSecond);
  TEST_ASSERT_TRUE(_commState.hasLength);
  TEST_ASSERT_EQUAL(recievedData[3],
                    0x20); // This is because in first cycle we can already
                           // fetch one byte since it was in order
  TEST_ASSERT_NOT_EQUAL(recievedData[4], 0x65);
  for (int i = 0; i < 6; i++) {
    c->RecieveScooterData();
    TEST_ASSERT_FALSE(_commState.hasCompletedBeforeCRC);
  }
  c->RecieveScooterData();
  TEST_ASSERT_TRUE(_commState.hasCompletedBeforeCRC);
  TEST_ASSERT_FALSE(_commState.hasCompletedPacket);

  c->RecieveScooterData();
  TEST_ASSERT_TRUE(_commState.hasCompletedBeforeCRC);
  TEST_ASSERT_TRUE(_commState.hasCompletedPacket);

  TEST_ASSERT_EQUAL(recievedData[11], 0x3F);
  TEST_ASSERT_EQUAL(recievedData[12], 0xFF);

  // We got everything, now let's create packet

  mijiaPacket *pack = c->CreatePacketFromRecieved();

  TEST_ASSERT_EQUAL(pack->validPacketError, 0);
  TEST_ASSERT_TRUE(pack->validPacket);
}

void test_packet_create_ble_correct_seven_length(void) {
  uint8_t *mp = new uint8_t[13]{
      0x55, 0xAA,                   // scooter identifier 0, 1
      0x07,                         // length 2
      0x20,                         // source - which device 3
      0x65,                         // command issued or answered 4
      0x00,                         // command argument 5
      0x04, 0x1D, 0x13, 0x00, 0x00, // payload 6, 7, 8, 9, 10,
      0x3F, 0xFF                    // CRC 11, 12
  };
  M365UartReciever *c = new M365UartReciever(mp);
  mijiaPacket *pack = c->CreatePacketFromRecieved();

  TEST_ASSERT_TRUE(pack->validPacket);

  TEST_ASSERT_EQUAL(pack->sig1, mp[0]);
  TEST_ASSERT_EQUAL(pack->sig2, mp[1]);
  TEST_ASSERT_EQUAL(pack->length, mp[2]);
  TEST_ASSERT_EQUAL(pack->payloadLength, mp[2] - 2);
  for (int i = 0; i < mp[2] - 2; i++) {
    TEST_ASSERT_EQUAL_MESSAGE(pack->payloadData[i], mp[6 + i],
                              "Array is not properly filled for mijiaPacket");
  }

  TEST_ASSERT_EQUAL(pack->originChecksum, pack->actualChecksum);

  delete[] mp;
  delete pack;
  // Asses everything is read correctly from this packet.
  // Next is to fill some sample files into test .csv's and test loading of
  // them.
}

void test_packet_create_ble_correct_six_length(void) {
  uint8_t *mp = new uint8_t[12]{0x55, 0xAA, 0x06, 0x21, 0x64, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x74, 0xFF};

  M365UartReciever *c = new M365UartReciever(mp);
  mijiaPacket *pack = c->CreatePacketFromRecieved();
  TEST_ASSERT_TRUE(pack->validPacket);

  TEST_ASSERT_EQUAL(pack->sig1, mp[0]);
  TEST_ASSERT_EQUAL(pack->sig2, mp[1]);
  TEST_ASSERT_EQUAL(pack->length, mp[2]);
  TEST_ASSERT_EQUAL(pack->payloadLength, mp[2] - 2);
  for (int i = 0; i < mp[2] - 2; i++) {
    TEST_ASSERT_EQUAL_MESSAGE(pack->payloadData[i], mp[6 + i],
                              "Array is not properly filled for mijiaPacket");
  }

  TEST_ASSERT_EQUAL(pack->originChecksum, pack->actualChecksum);
  delete[] mp;
  delete pack;
}

void test_packet_create_ble_correct_nine_length(void) {
  uint8_t *mp = new uint8_t[15]{0x55, 0xAA, 0x09, 0x20, 0x64, 0x00, 0x06, 0x27,
                                0x26, 0x00, 0x00, 0x72, 0x00, 0xAD, 0xFE};

  M365UartReciever *c = new M365UartReciever(mp);
  mijiaPacket *pack = c->CreatePacketFromRecieved();
  TEST_ASSERT_TRUE(pack->validPacket);

  TEST_ASSERT_EQUAL(pack->sig1, mp[0]);
  TEST_ASSERT_EQUAL(pack->sig2, mp[1]);
  TEST_ASSERT_EQUAL(pack->length, mp[2]);
  TEST_ASSERT_EQUAL(pack->payloadLength, mp[2] - 2);
  for (int i = 0; i < mp[2] - 2; i++) {
    TEST_ASSERT_EQUAL_MESSAGE(pack->payloadData[i], mp[6 + i],
                              "Array is not properly filled for mijiaPacket");
  }

  TEST_ASSERT_EQUAL(pack->originChecksum, pack->actualChecksum);
  delete[] mp;
  delete pack;
}

void test_packet_create_ble_invalid(void) {
  uint8_t *mp = new uint8_t[13]{
      0x55, 0xAA, // scooter identifier 0, 1
      0x07,       // length 2
      0x20,       // source - which device 3
      0x65,       // command issued or answered 4
  };
  M365UartReciever *c = new M365UartReciever(mp);
  mijiaPacket *pack = c->CreatePacketFromRecieved();

  TEST_ASSERT_FALSE(pack->validPacket);

  TEST_ASSERT_EQUAL(pack->sig1, mp[0]);
  TEST_ASSERT_EQUAL(pack->sig2, mp[1]);
  TEST_ASSERT_EQUAL(pack->length, mp[2]);

  delete[] mp;
  delete pack;
}

void test_packet_create_ble_invalid_crc(void) {
  uint8_t *mp = new uint8_t[13]{
      0x55, 0xAA,                   // scooter identifier 0, 1
      0x07,                         // length 2
      0x20,                         // source - which device 3
      0x65,                         // command issued or answered 4
      0x00,                         // command argument 5
      0x04, 0x1D, 0x13, 0x00, 0x00, // payload 6, 7, 8, 9, 10,
      0x4F, 0xFF                    // CRC 11, 12
  };

  M365UartReciever *c = new M365UartReciever(mp);
  mijiaPacket *pack = c->CreatePacketFromRecieved();

  TEST_ASSERT_FALSE(pack->validPacket);
  TEST_ASSERT_NOT_EQUAL(pack->originChecksum, pack->actualChecksum);

  delete[] mp;
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_recieve_scooter_data_first_byte);
  RUN_TEST(test_recieve_scooter_data_whole_preamble_seperate);
  RUN_TEST(test_recieve_data_not_recieving_when_completed);
  RUN_TEST(test_packet_create_from_recieved_data);
  RUN_TEST(test_recieve_data_with_length_no_data);
  RUN_TEST(test_recieve_data_full_valid_packet);
  RUN_TEST(test_packet_create_ble_correct_six_length);
  RUN_TEST(test_packet_create_ble_correct_seven_length);
  RUN_TEST(test_packet_create_ble_correct_nine_length);
  RUN_TEST(test_packet_create_ble_invalid);
  RUN_TEST(test_packet_create_ble_invalid_crc);
  UNITY_END();

  return 0;
}