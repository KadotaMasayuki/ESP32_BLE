// 2020/07/15,22,08/03
//  BLEのアドバタイズデータを見ていろいろ判断する
//  3軸加速度センサ(ADXL345)の値を判断する
//
// 参考
// http://marchan.e5.valueserver.jp/cabin/comp/jbox/arc212/doc21203.html

//
// Environment...
//   Arduino 1.8.13
//   ESP32 1.0.4
//

#include <BLEDevice.h>


// シリアルモニタによるデバッグをしないときはコメントアウト
#define DEBUG 1

// 入出力ピン情報
// IN1 (GPI36)
// IN2 (GPI39)
// IN3 (GPI34)
// OUT1 (GPIO25)
// OUT2 (GPIO26)
// OUT3 (GPIO27)
//

#define PIN_IN_1  36
#define PIN_IN_2  39
#define PIN_IN_3  34
#define PIN_OUT_1 25
#define PIN_OUT_2 26
#define PIN_OUT_3 27


// スリープ時間
#define MS_TO_SLEEP 350


/* BLE  */
#define BLE_DEVICE_NAME "AECBC"        // デバイス名(サービス名)
#define BLE_DEVICE_NUMBER 101            // デバイス識別番号 0 - 65535
const int scanning_sec = 1;   // スキャン時間[sec]
BLEScan *pScan;

// BLE機能タイプ
#define FUNCTION_TYPE_ACC 1     // 追加の機能タイプ(加速度)
#define FUNCTION_LENGTH_ACC 7   // 追加の機能タイプ(加速度)のデータ長

// 呼出し
#define PIN_CALL 33  // 呼出し機能出力ピン。呼び出すときL
#define MS_TO_NEXT_CALL 300 * 1000  // 呼出し後に再度呼出しできるまでの無視期間
uint32_t call_interval_count = 0;  // 無視期間のカウント
#define RSSI_LIMIT -50  // 間違いなくすぐ近くに居ると思われる値。この値より大きいと近く、小さいと遠い。


void setup() {
  // put your setup code here, to run once:

  //////////////////////////////////////////////////////////////////////////
  // 変数設定
  //////////////////////////////////////////////////////////////////////////

  // 無視期間のカウントを、無視期間終了値に設定
  call_interval_count = MS_TO_NEXT_CALL;

  //////////////////////////////////////////////////////////////////////////
  // GPIO
  //////////////////////////////////////////////////////////////////////////

  // ピン設定
  pinMode(PIN_IN_1, INPUT_PULLUP);
  pinMode(PIN_IN_2, INPUT_PULLUP);
  pinMode(PIN_IN_3, INPUT_PULLUP);
  pinMode(PIN_OUT_1, OUTPUT);
  pinMode(PIN_OUT_2, OUTPUT);
  pinMode(PIN_OUT_3, OUTPUT);
  pinMode(PIN_CALL, OUTPUT); digitalWrite(PIN_CALL, LOW);

  //////////////////////////////////////////////////////////////////////////
  // シリアルモニタ
  //////////////////////////////////////////////////////////////////////////

  // シリアルモニタの初期化
  #ifdef DEBUG
    Serial.begin(115200);
    delay(500);
    Serial.println();
    Serial.print("BLE_DEVICE_NAME=");
    Serial.println(BLE_DEVICE_NAME);
    Serial.print("BLE_DEVICE_NUMBER=");
    Serial.println(BLE_DEVICE_NUMBER);
  #endif

  //////////////////////////////////////////////////////////////////////////
  // BLE
  //////////////////////////////////////////////////////////////////////////

  // BLEデバイスの初期化
  BLEDevice::init("");  // スキャンするだけなので、名乗る必要はない
  
  // スキャンオプジェクト取得
  pScan = BLEDevice::getScan();

  // パッシブスキャンにする(アクティブスキャンは、デバイスを見つけたらデータを取りに行く)
  pScan->setActiveScan(false);

}

void loop() {
  bool to_call = false;  // 呼出するならtrue
  
  // スキャン開始
  #ifdef DEBUG
    Serial.print("Scanning ");
    Serial.print(scanning_sec);
    Serial.println("[sec]");
  #endif
  BLEScanResults foundDevices = pScan->start(scanning_sec);
  int foundCount = foundDevices.getCount();

  // 見つかったデバイス群から、指定のデバイス(BLE_DEVICE_NAMEのもの)を処理
  int manu_code = 0;
  uint16_t device_number = 0;
  int seq_number = 0;
  int pin_info_1 = 0;
  int pin_info_2 = 0;
  int pin_info_3 = 0;
  int16_t x = 0;
  bool is_x_moving = 0;
  int16_t y = 0;
  bool is_y_moving = 0;
  int16_t z = 0;
  bool is_z_moving = 0;
  int rssi = 0;
  for (int i = 0; i < foundCount; i ++) {
    BLEAdvertisedDevice dev = foundDevices.getDevice(i);
    std::string device_name = dev.getName();
    if (device_name == BLE_DEVICE_NAME) {
      if (dev.haveManufacturerData()) {
        std::string data = dev.getManufacturerData();
        if (data.length() >= 7) {
          int func_len = (int)data[6];
          if (data.length() >= 6 + func_len) {
            if (data[7] == FUNCTION_TYPE_ACC) {
              manu_code = data[1] << 8 | data[0];  // manufacturer ID
              device_number = (((uint16_t)data[3]) << 8) | (uint16_t)data[2];  // 識別番号
              seq_number = data[4];  // シーケンス番号
              pin_info_1 = (data[5] & 0x04) >> 2;
              pin_info_2 = (data[5] & 0x02) >> 1;
              pin_info_3 = data[5] & 0x01;
              x = (((int16_t)data[9]) << 8) | data[8];
              y = (((int16_t)data[11]) << 8) | data[10];
              z = (((int16_t)data[13]) << 8) | data[12];
              is_x_moving = (data[14] & 0x04) >> 2;
              is_y_moving = (data[14] & 0x02) >> 1;
              is_z_moving = data[14] & 0x01;
              rssi = dev.getRSSI();
              
              // 呼出するか
              if (is_x_moving || is_y_moving || is_z_moving) {
                to_call = false;
              } else if (rssi > RSSI_LIMIT) {
                to_call = true;  // 動いていなくて十分近い
              } else {
                to_call = false;
              }

              #ifdef DEBUG
                Serial.print("Device Number:");
                Serial.print(device_number);
      
                Serial.print("   SN:");
                Serial.print(seq_number);
      
                Serial.print("   INFO(1,2,3):(");
                Serial.print(pin_info_1);
                Serial.print(",");
                Serial.print(pin_info_2);
                Serial.print(",");
                Serial.print(pin_info_3);
                Serial.println(")");
      
                Serial.print("   X:");
                if (is_x_moving) {
                  Serial.print(" moving     : ");
                } else {
                  Serial.print(" not moving : ");
                }
                Serial.print(x);
                Serial.print(",");
                Serial.print(x * 0.00390625);
                Serial.print("[G],");
                Serial.print(x * 0.03830723);
                Serial.print("[m/s2]");
                Serial.println();
                
                Serial.print("   Y:");
                if (is_y_moving) {
                  Serial.print(" moving     : ");
                } else {
                  Serial.print(" not moving : ");
                }
                Serial.print(y);
                Serial.print(",");
                Serial.print(y * 0.00390625);
                Serial.print("[G],");
                Serial.print(y * 0.03830723);
                Serial.print("[m/s2]");
                Serial.println();
            
                Serial.print("   Z:");
                if (is_z_moving) {
                  Serial.print(" moving     : ");
                } else {
                  Serial.print(" not moving : ");
                }
                Serial.print(z);
                Serial.print(",");
                Serial.print(z * 0.00390625);
                Serial.print("[G],");
                Serial.print(z * 0.03830723);
                Serial.print("[m/s2]");
                Serial.println();
              
                Serial.print("   RSSI:");
                Serial.print(rssi);
                Serial.println("[dBm]");

                Serial.print("   RAW:");
                for (int i = 0; i < data.length(); i ++) {
                  Serial.print((char)data[i], HEX);
                  Serial.print(" ");
                }
                Serial.println();

                Serial.print("CALL INTERVAL : ");
                Serial.println(call_interval_count);

                Serial.print("TO CALL : ");
                if (to_call) {
                  Serial.print("ON");
                  if (call_interval_count < MS_TO_NEXT_CALL) {
                    Serial.print(" but now interval");
                  }
                } else {
                  Serial.print("OFF");
                }
                Serial.println();
              #endif
            }
          }
        }
      }
    }
  }

  // 自身のピン入力
  int pin_in_1;
  int pin_in_2;
  int pin_in_3;
  pin_in_1 = digitalRead(PIN_IN_1);
  pin_in_2 = digitalRead(PIN_IN_2);
  pin_in_3 = digitalRead(PIN_IN_3);
  // ピン出力
  //    ピン入力の結果を出力ピンにそのまま出力する
  if (pin_in_1 == 0) {
    digitalWrite(PIN_OUT_1, LOW);
  } else {
    digitalWrite(PIN_OUT_1, HIGH);
  }
  if (pin_in_2 == 0) {
    digitalWrite(PIN_OUT_2, LOW);
  } else {
    digitalWrite(PIN_OUT_2, HIGH);
  }
  if (pin_in_3 == 0) {
    digitalWrite(PIN_OUT_3, LOW);
  } else {
    digitalWrite(PIN_OUT_3, HIGH);
  }
  #ifdef DEBUG
    Serial.print("   PIN_IN(1,2,3):(");
    Serial.print(pin_in_1);
    Serial.print(pin_in_2);
    Serial.print(pin_in_3);
    Serial.println(")");
  #endif

  //////////////////////////////////////////////////////////////////////////
  // 呼び出し
  //////////////////////////////////////////////////////////////////////////
  
  if (call_interval_count < MS_TO_NEXT_CALL) {
    call_interval_count += MS_TO_SLEEP + (scanning_sec * 1000);
  } else if (to_call) {
    digitalWrite(PIN_CALL, HIGH);   // ONして
    delay(400);                     // ちょっと待って
    digitalWrite(PIN_CALL, LOW);    // OFFする
    call_interval_count = 0;        // カウントをゼロにして、インターバル期間を開始する
  }

  //////////////////////////////////////////////////////////////////////////
  // Wait
  //////////////////////////////////////////////////////////////////////////

  #ifdef DEBUG
    Serial.print("Waiting ");
    Serial.print(MS_TO_SLEEP);
    Serial.println("[ms]");
    Serial.println();
  #endif
  delay(MS_TO_SLEEP);
}
