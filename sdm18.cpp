#include "sdm18.h"


// ============================================================
// F303K8 constructor
// ============================================================

sdm18::sdm18(BufferedSerial &sensor, CAN &can)
    : _can(can),
      _sensor(&sensor)
{
    _can.frequency(1000000);

    for (int i = 0; i < 23; i++)
    {
        scan_recv_start[i] = 0;
        can_recv_buffer[i] = 0;
    }

    can_received_701 = false;
    can_received_702 = false;
    can_received_703 = false;

    distance = 0;
    crc_result = 0;
    checksum = 0;
}


// ============================================================
// F446RE constructor
// ============================================================

sdm18::sdm18(CAN &can)
    : _can(can),
      _sensor(nullptr)
{
    _can.frequency(1000000);

    for (int i = 0; i < 23; i++)
    {
        scan_recv_start[i] = 0;
        can_recv_buffer[i] = 0;
    }

    can_received_701 = false;
    can_received_702 = false;
    can_received_703 = false;

    distance = 0;
    crc_result = 0;
    checksum = 0;
}


// ============================================================
// CRC16
// ============================================================

uint16_t sdm18::calculate_crc16(uint8_t *buf, int len)
{
    uint16_t crc = 0xFFFF;

    for (int i = 0; i < len; i++)
    {
        crc ^= buf[i];

        for (int bit = 0; bit < 8; bit++)
        {
            if (crc & 0x0001)
            {
                crc >>= 1;
                crc ^= 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return crc;
}


// ============================================================
// Start scan
// ============================================================

bool sdm18::startscan()
{
    if (_sensor == nullptr)
    {
        return false;
    }

    char cmd_start[9] =
    {
        0xA5,
        0x03,
        0x20,
        0x01,
        0x00,
        0x00,
        0x00,
        0x02,
        0x6E
    };

    _sensor->write(cmd_start, sizeof(cmd_start));

    return true;
}


// ============================================================
// Stop scan
// ============================================================

bool sdm18::stopscan()
{
    if (_sensor == nullptr)
    {
        return false;
    }

    char cmd_stop[9] =
    {
        0xA5,
        0x03,
        0x20,
        0x02,
        0x00,
        0x00,
        0x00,
        0x46,
        0x6E
    };

    _sensor->write(cmd_stop, sizeof(cmd_stop));

    return true;
}


// ============================================================
// Set baudrate
// ============================================================

bool sdm18::setbaudrate(char baudrate)
{
    if (_sensor == nullptr)
    {
        return false;
    }

    char cmd[10] =
    {
        0xA5,
        0x03,
        0x20,
        0x10,
        0x00,
        0x00,
        0x01,
        baudrate,
        0xBD,
        0x3F
    };

    _sensor->write(cmd, sizeof(cmd));

    return true;
}


// ============================================================
// SDM18 receive
//
// F303ではここではCRCや距離計算をしない。
// 23byteをそのまま取得するだけ。
// ============================================================

bool sdm18::getdata()
{
    if (_sensor == nullptr)
    {
        return false;
    }

    if (!_sensor->readable())
    {
        return false;
    }

    uint8_t header = 0;

    // --------------------------------------------------------
    // まずA5を探す
    // --------------------------------------------------------

    if (_sensor->read(&header, 1) != 1)
    {
        return false;
    }

    if (header != 0xA5)
    {
        return false;
    }

    scan_recv_start[0] = 0xA5;


    // --------------------------------------------------------
    // A5以降の22byteを取得
    //
    // 一度A5を検出したら、途中のA5には反応しない。
    // --------------------------------------------------------

    size_t read_bytes = 0;

    int timeout = 0;

    while (read_bytes < 22 && timeout < 15)
    {
        if (_sensor->readable())
        {
            ssize_t n = _sensor->read(
                &scan_recv_start[1 + read_bytes],
                22 - read_bytes
            );

            if (n > 0)
            {
                read_bytes += n;
                timeout = 0;
            }
        }
        else
        {
            ThisThread::sleep_for(1ms);
            timeout++;
        }
    }

    if (read_bytes != 22)
    {
        return false;
    }

    return true;
}


// ============================================================
// CAN transmit
//
// 23byte
//
// 0x701 : byte  0～ 7
// 0x702 : byte  8～15
// 0x703 : byte 16～22
// ============================================================

bool sdm18::sdm18_send()
{
    CANMessage msg;


    // --------------------------------------------------------
    // 0x701
    // --------------------------------------------------------

    msg.id = 0x701;
    msg.len = 8;

    for (int i = 0; i < 8; i++)
    {
        msg.data[i] = scan_recv_start[i];
    }

    if (!_can.write(msg))
    {
        return false;
    }


    // --------------------------------------------------------
    // 0x702
    // --------------------------------------------------------

    msg.id = 0x702;
    msg.len = 8;

    for (int i = 0; i < 8; i++)
    {
        msg.data[i] = scan_recv_start[8 + i];
    }

    if (!_can.write(msg))
    {
        return false;
    }


    // --------------------------------------------------------
    // 0x703
    // --------------------------------------------------------

    msg.id = 0x703;
    msg.len = 7;

    for (int i = 0; i < 7; i++)
    {
        msg.data[i] = scan_recv_start[16 + i];
    }

    if (!_can.write(msg))
    {
        return false;
    }

    return true;
}


// ============================================================
// CAN receive
//
// F446で使用
// ============================================================

bool sdm18::can_receive()
{
    CANMessage msg;

    if (!_can.read(msg))
    {
        return false;
    }


    // --------------------------------------------------------
    // 0x701
    // --------------------------------------------------------

    if (msg.id == 0x701 && msg.len == 8)
    {
        for (int i = 0; i < 8; i++)
        {
            can_recv_buffer[i] = msg.data[i];
        }

        can_received_701 = true;
    }


    // --------------------------------------------------------
    // 0x702
    // --------------------------------------------------------

    else if (msg.id == 0x702 && msg.len == 8)
    {
        for (int i = 0; i < 8; i++)
        {
            can_recv_buffer[8 + i] = msg.data[i];
        }

        can_received_702 = true;
    }


    // --------------------------------------------------------
    // 0x703
    // --------------------------------------------------------

    else if (msg.id == 0x703 && msg.len == 7)
    {
        for (int i = 0; i < 7; i++)
        {
            can_recv_buffer[16 + i] = msg.data[i];
        }

        can_received_703 = true;
    }


    // --------------------------------------------------------
    // 3つ揃った
    // --------------------------------------------------------

    if (can_received_701 &&
        can_received_702 &&
        can_received_703)
    {
        for (int i = 0; i < 23; i++)
        {
            scan_recv_start[i] = can_recv_buffer[i];
        }

        can_received_701 = false;
        can_received_702 = false;
        can_received_703 = false;

        return true;
    }

    return false;
}


// ============================================================
// Process data
//
// F446でのみ実行
// ============================================================

bool sdm18::process_data()
{
    // CRCは一旦無効化
    // 実際のSDM18フレーム仕様確認後に戻す

    distance =
        static_cast<uint16_t>(scan_recv_start[12]) |
        (
            static_cast<uint16_t>(scan_recv_start[13]) << 8
        );

    distance += 30;

    return true;
}


// ============================================================
// Get distance
// ============================================================

uint16_t sdm18::get_distance()
{
    return distance;
}


// ============================================================
// Debug
// ============================================================

void sdm18::debug_print_frame()
{
    printf("SDM18 RAW: ");

    for (int i = 0; i < 23; i++)
    {
        printf(
            "%02X ",
            static_cast<uint8_t>(scan_recv_start[i])
        );
    }

    printf("\r\n");
}