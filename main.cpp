#include "mbed.h"

BufferedSerial pc(USBTX, USBRX, 921600);

// CAN設定 (F446のピンに合わせて適宜変更してください。通常は PA_11, PA_12 や PB_8, PB_9 など)
CAN can(PA_11, PA_12, 1000000);

DigitalOut led1(PC_0);
DigitalOut led2(PC_1);
DigitalOut led3(PC_2);
DigitalOut led4(PC_3);

FileHandle *mbed::mbed_override_console(int fd)
{
    return &pc;
}

int main()
{
    led1 = 1;
    ThisThread::sleep_for(500ms);
    printf("\r\n--- CAN Receiver F446 ---\r\n");


    CANMessage msg;

    while (true)
    {
        
        // CANメッセージを受信したかチェック
        if (can.read(msg)){
            printf("Rx Id 0x5X\r\n,msg.id");
            // IDが 0x701（または送信側で設定したID）のデータかチェック
            if (msg.id == 0x701)
            {
                led2 = 1;
                // 2バイトのデータから距離（mm）を復元
                uint16_t received_distance1 = 
                    static_cast<uint16_t>(msg.data[0]) | 
                    (static_cast<uint16_t>(msg.data[1]) << 8);

                    printf("Received ID: 0x%X | Distance: %u mm  ,", msg.id, received_distance1);
            }
            if(msg.id == 0x702){
                led3 = 1;

                uint16_t received_distance2 = static_cast<uint16_t>(msg.data[0]) | (static_cast<uint16_t>(msg.data[1]) << 8);
                    printf("Received ID: 0x%X | Distance: %u mm  ,", msg.id, received_distance2);
            }
            if(msg.id == 0x703){
                led4 = 1;
                uint16_t received_distance3 = static_cast<uint16_t>(msg.data[0]) | (static_cast<uint16_t>(msg.data[1]) << 8);
                printf("Received ID: 0x%X | Distance: %u mm\r", msg.id, received_distance3);
            }
            printf("\n");

        }

        // CPUを占有しないためのウェイト
        ThisThread::sleep_for(1ms);
    }
}