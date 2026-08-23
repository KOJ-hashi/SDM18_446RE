#include "mbed.h"

CAN can(PA_11, PA_12, 1000000);          // CANボーレート 1Mbps
BufferedSerial pc(USBTX, USBRX, 115200);  // ログ確認用のPCシリアル

DigitalOut led1(PA_5);  // 起動確認用LED
DigitalOut led2(PC_13); // 受信のたびに光るLED

// 10秒分のデータを保存するバッファ（仮に1000個分用意）
#define LOG_SIZE 50
uint16_t distance_log[LOG_SIZE];
int log_index = 0;
bool log_completed = false;

int main()
{
    led1 = 1;
    led2 = 0;

    ThisThread::sleep_for(500ms);
    printf("\r\n--- F446 CAN Log Collector Started ---\r\n");
    printf("Collecting %d samples...\r\n", LOG_SIZE);

    CANMessage msg;

    while (true)
    {
        // CANメッセージを受信
        if (can.read(msg))
        {
            if (msg.id == 0x701 && msg.len == 2)
            {
                uint16_t distance = static_cast<uint16_t>(msg.data[0]) | 
                                    (static_cast<uint16_t>(msg.data[1]) << 8);

                led2 = !led2;

                // まだバッファがいっぱいになっていなければメモリに記録
                if (!log_completed)
                {
                    distance_log[log_index] = distance;
                    log_index++;

                    // 指定数たまったら一度だけ一気にダンプ
                    if (log_index >= LOG_SIZE)
                    {
                        log_completed = true;
                        printf("\r\n--- [LOG DUMP START] ---\r\n");
                        for (int i = 0; i < LOG_SIZE; i++)
                        {
                            printf("[%d]: %u mm\r\n", i, distance_log[i]);
                        }
                        printf("--- [LOG DUMP END] ---\r\n\r\n");
                    }
                }
            }
        }

        // ログ収集完了後も、ロボットの処理を止めないためにyieldで最速を維持
        ThisThread::yield();
    }
}