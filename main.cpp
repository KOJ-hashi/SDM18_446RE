#include "mbed.h"

// F446のCANピン設定（ボードによって異なる場合は変更してください 例: PA_11, PA_12 または PB_8, PB_9）
CAN can(PA_11, PA_12, 1000000); // ボーレート 1Mbps

DigitalOut led1(PA_5); // 起動確認用LED（Nucleoの緑LEDなど）
DigitalOut led2(PC_13); // CAN受信のたびに光るデバッグ用LED

int main()
{
    led1 = 1;
    led2 = 0;

    ThisThread::sleep_for(500ms);

    // 必要であればPC確認用のシリアルを用意してもOKですが、本番用としてスッキリさせています
    // BufferedSerial pc(USBTX, USBRX, 115200);

    CANMessage msg;

    while (true)
    {
        // CANメッセージを受信したかチェック
        if (can.read(msg))
        {
            // F303から ID: 0x701 で送られてきたデータかチェック
            if (msg.id == 0x701 && msg.len == 2)
            {
                // 下位バイトと上位バイトを結合して距離を復元
                uint16_t distance = static_cast<uint16_t>(msg.data[0]) | 
                                    (static_cast<uint16_t>(msg.data[1]) << 8);

                // 受信成功の証としてLED2を反転（ぬるぬる受信できれば激しく点滅します）
                led2 = !led2;

                // ==========================================
                // ここでロボットの制御処理や変数への格納を行う
                // 例: target_distance = distance;
                // ==========================================
            }
        }

        // CPUを占有しないための最小限のウェイト（必要に応じて調整）
        ThisThread::sleep_for(1ms);
    }
}