#include <fakeit.hpp>
#include <Arduino.h>
#include <unity.h>

using namespace fakeit;

#include <uart_reader.h>


void test_recieve_scooter_data_first_byte(void) {
    Mock<HardwareSerial> hw_mock;

    uint8_t recievedData[0xFF];
    uint8_t packetCursor = 0;
    mijiaCommState _commState;

    Fake(Method(hw_mock, read));
    Fake(Method(hw_mock, available));

    When(Method(hw_mock, read)).Return(0x55);
    When(Method(hw_mock, available)).Return(true).AlwaysReturn(false);

    TEST_ASSERT_FALSE(_commState.hasPreambleFirst);
    TEST_ASSERT_FALSE(_commState.hasPreambleSecond);

    HardwareSerial &hw = hw_mock.get();

    recieveScooterData(&hw, &_commState, recievedData, &packetCursor);

    TEST_ASSERT_EQUAL(recievedData[0], 0x55);
    TEST_ASSERT_TRUE(_commState.hasPreambleFirst);
    TEST_ASSERT_FALSE(_commState.hasPreambleSecond);
}

void test_recieve_scooter_data_whole_preamble_seperate(void) {
    Mock<HardwareSerial> hw_mock;

    uint8_t recievedData[0xFF];
    uint8_t packetCursor = 0;
    mijiaCommState _commState;

    Fake(Method(hw_mock, read));
    Fake(Method(hw_mock, available));

    When(Method(hw_mock, read)).Return(0x55).Return(0xAA);
    When(Method(hw_mock, available)).Return(true, 4_Times(false), 5_Times(false), 2_Times(true), 3_Times(false));

    TEST_ASSERT_FALSE(_commState.hasPreambleFirst);
    TEST_ASSERT_FALSE(_commState.hasPreambleSecond);

    HardwareSerial &hw = hw_mock.get();

    recieveScooterData(&hw, &_commState, recievedData, &packetCursor);

    TEST_ASSERT_EQUAL(recievedData[0], 0x55);
    TEST_ASSERT_NOT_EQUAL(recievedData[1], 0xAA);
    TEST_ASSERT_TRUE(_commState.hasPreambleFirst);
    TEST_ASSERT_FALSE(_commState.hasPreambleSecond);


    recieveScooterData(&hw, &_commState, recievedData, &packetCursor);

    TEST_ASSERT_EQUAL(recievedData[0], 0x55);
    TEST_ASSERT_NOT_EQUAL(recievedData[1], 0xAA);
    TEST_ASSERT_TRUE(_commState.hasPreambleFirst);
    TEST_ASSERT_FALSE(_commState.hasPreambleSecond);
    

    recieveScooterData(&hw, &_commState, recievedData, &packetCursor);

    TEST_ASSERT_EQUAL(recievedData[0], 0x55);
    TEST_ASSERT_EQUAL(recievedData[1], 0xAA);
    TEST_ASSERT_TRUE(_commState.hasPreambleFirst);
    TEST_ASSERT_TRUE(_commState.hasPreambleSecond);
}

void test_packet_create_ble_correct_seven_length(void) {
    uint8_t* mp = new uint8_t[13] {
        0x55, 0xAA, // scooter identifier 0, 1
        0x07, // length 2
        0x20, // source - which device 3
        0x65, // command issued or answered 4
        0x00, // command argument 5
        0x04, 0x1D, 0x13, 0x00, 0x00, // payload 6, 7, 8, 9, 10,
        0x3F, 0xFF //CRC 11, 12
    };

    mijiaPacket* pack = create_packet_from_array(mp, 13);

    TEST_ASSERT_TRUE(pack->validPacket);

    TEST_ASSERT_EQUAL(pack->sig1, mp[0]);
    TEST_ASSERT_EQUAL(pack->sig2, mp[1]);
    TEST_ASSERT_EQUAL(pack->length, mp[2]);
    TEST_ASSERT_EQUAL(pack->payloadLength, mp[2] - 2);
    for (int i=0; i < mp[2] - 2; i++) {
        TEST_ASSERT_EQUAL_MESSAGE(pack->payloadData[i], mp[6 + i], "Array is not properly filled for mijiaPacket");
    }

    TEST_ASSERT_EQUAL(pack->originChecksum, pack->actualChecksum);
    

    delete[] mp;
    delete pack;
    //Asses everything is read correctly from this packet.
    // Next is to fill some sample files into test .csv's and test loading of them.
}

void test_packet_create_ble_correct_six_length(void) {
    uint8_t *mp = new uint8_t[12] {
        0x55, 0xAA,
        0x06,
        0x21,
        0x64,
        0x00, 0x00, 0x00, 0x00, 0x00, 
        0x74, 0xFF
    };

    mijiaPacket* pack = create_packet_from_array(mp, 12);
    TEST_ASSERT_TRUE(pack->validPacket);

    TEST_ASSERT_EQUAL(pack->sig1, mp[0]);
    TEST_ASSERT_EQUAL(pack->sig2, mp[1]);
    TEST_ASSERT_EQUAL(pack->length, mp[2]);
    TEST_ASSERT_EQUAL(pack->payloadLength, mp[2] - 2);
    for (int i=0; i < mp[2] - 2; i++) {
        TEST_ASSERT_EQUAL_MESSAGE(pack->payloadData[i], mp[6 + i], "Array is not properly filled for mijiaPacket");
    }

    TEST_ASSERT_EQUAL(pack->originChecksum, pack->actualChecksum);
    delete[] mp;
    delete pack;
}

void test_packet_create_ble_correct_nine_length(void) {
    uint8_t *mp = new uint8_t[15] {
        0x55, 0xAA,
        0x09,
        0x20,
        0x64,
        0x00,
        0x06, 0x27, 0x26, 0x00, 0x00, 0x72, 0x00,
        0xAD, 0xFE
    };

    mijiaPacket* pack = create_packet_from_array(mp, 15);
    TEST_ASSERT_TRUE(pack->validPacket);

    TEST_ASSERT_EQUAL(pack->sig1, mp[0]);
    TEST_ASSERT_EQUAL(pack->sig2, mp[1]);
    TEST_ASSERT_EQUAL(pack->length, mp[2]);
    TEST_ASSERT_EQUAL(pack->payloadLength, mp[2] - 2);
    for (int i=0; i < mp[2] - 2; i++) {
        TEST_ASSERT_EQUAL_MESSAGE(pack->payloadData[i], mp[6 + i], "Array is not properly filled for mijiaPacket");
    }

    TEST_ASSERT_EQUAL(pack->originChecksum, pack->actualChecksum);
    delete[] mp;
    delete pack;
}

void test_packet_create_ble_invalid(void) {
    uint8_t* mp = new uint8_t[13] {
        0x55, 0xAA, // scooter identifier 0, 1
        0x07, // length 2
        0x20, // source - which device 3
        0x65, // command issued or answered 4
    };
    mijiaPacket* pack = create_packet_from_array(mp, 5);

    TEST_ASSERT_FALSE(pack->validPacket);

    TEST_ASSERT_EQUAL(pack->sig1, mp[0]);
    TEST_ASSERT_EQUAL(pack->sig2, mp[1]);
    TEST_ASSERT_EQUAL(pack->length, mp[2]);

    delete[] mp;
    delete pack;
}

void test_packet_create_ble_invalid_crc(void) {
    uint8_t* mp = new uint8_t[13] {
        0x55, 0xAA, // scooter identifier 0, 1
        0x07, // length 2
        0x20, // source - which device 3
        0x65, // command issued or answered 4
        0x00, // command argument 5
        0x04, 0x1D, 0x13, 0x00, 0x00, // payload 6, 7, 8, 9, 10,
        0x4F, 0xFF //CRC 11, 12
    };

    mijiaPacket* pack = create_packet_from_array(mp, 13);

    TEST_ASSERT_FALSE(pack->validPacket);
    TEST_ASSERT_NOT_EQUAL(pack->originChecksum, pack->actualChecksum);

    delete[] mp;
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_recieve_scooter_data_first_byte);
    RUN_TEST(test_recieve_scooter_data_whole_preamble_seperate);
    RUN_TEST(test_packet_create_ble_correct_six_length);
    RUN_TEST(test_packet_create_ble_correct_seven_length);
    RUN_TEST(test_packet_create_ble_correct_nine_length);
    RUN_TEST(test_packet_create_ble_invalid);
    RUN_TEST(test_packet_create_ble_invalid_crc);
    UNITY_END();

    return 0;
}