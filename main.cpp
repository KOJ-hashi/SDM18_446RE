#include "mbed.h"

BufferedSerial pc(USBTX, USBRX, 115200);

// F446側のCANピン定義 (通常 PA_11 = RD, PA_12 = TD)
CAN can(PA_11, PA_12, 500000); // 500 kbps

FileHandle *mbed::mbed_override_console(int fd)
{
    return &pc;
}

int main()
{
    ThisThread::sleep_for(500ms);

    printf("\r\n--- SDM18 F446 Receiver ---\r\n");
    printf("Waiting for CAN message (ID: 0x100)...\r\n");

    CANMessage msg;

    while (true)
    {
        // CANメッセージを受信したかチェック
        if (can.read(msg))
        {
            // IDが 0x100 かつデータ長が 2バイトか確認
            if (msg.id == 0x701 && msg.len == 2)
            {
                uint16_t distance = static_cast<uint16_t>(msg.data[0]) | 
                                    (static_cast<uint16_t>(msg.data[1]) << 8);

                printf("Received Distance from F303: %u mm\r\n", distance);
            }
        }

        ThisThread::sleep_for(5ms);
    }
}