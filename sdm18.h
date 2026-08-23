#ifndef SDM18_H_
#define SDM18_H_

#include "mbed.h"

class sdm18
{
public:
    // F303K8
    sdm18(BufferedSerial &sensor, CAN &can);

    // F446RE
    sdm18(CAN &can);

    // SDM18 control
    bool startscan();
    bool stopscan();
    bool setbaudrate(char baudrate);

    // F303K8
    // SDM18から23byteを取得
    bool getdata();

    // F303K8
    // 23byteをCAN 3フレームへ分割して送信
    bool sdm18_send();

    // F446RE
    // CAN 3フレームを受信して23byteへ復元
    bool can_receive();

    // F446RE
    // CRC計算・距離抽出・+30mm
    bool process_data();

    uint16_t calculate_crc16(uint8_t *buf, int len);

    uint16_t get_distance();

    // デバッグ用
    void debug_print_frame();

private:
    CAN &_can;
    BufferedSerial *_sensor;

    // F303で取得したSDM18フレーム
    uint8_t scan_recv_start[23];

    // F446でCANから復元するフレーム
    uint8_t can_recv_buffer[23];

    bool can_received_701;
    bool can_received_702;
    bool can_received_703;

    uint16_t distance;
    uint16_t crc_result;
    uint16_t checksum;
};

#endif