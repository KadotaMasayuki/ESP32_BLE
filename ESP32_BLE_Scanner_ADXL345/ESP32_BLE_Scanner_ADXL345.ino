// 2020/07/15,2020/07/22
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


void setup() {
  // put your setup code here, to run once:

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
  // GPIO
  //////////////////////////////////////////////////////////////////////////

  // ピン設定
  pinMode(PIN_IN_1, INPUT_PULLUP);
  pinMode(PIN_IN_2, INPUT_PULLUP);
  pinMode(PIN_IN_3, INPUT_PULLUP);
  pinMode(PIN_OUT_1, OUTPUT);
  pinMode(PIN_OUT_2, OUTPUT);
  pinMode(PIN_OUT_3, OUTPUT);

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
        manu_code = data[1] << 8 | data[0];  // manufacturer ID
        device_number = (((uint16_t)data[3]) << 8) | (uint16_t)data[2];  // 識別番号
        seq_number = data[4];  // シーケンス番号
        pin_info_1 = data[5];
        pin_info_2 = data[6];
        pin_info_3 = data[7];
        x = (((int16_t)data[9]) << 8) | data[8];
        is_x_moving = data[10];
        y = (((int16_t)data[12]) << 8) | data[11];
        is_y_moving = data[13];
        z = (((int16_t)data[15]) << 8) | data[14];
        is_z_moving = data[16];
        rssi = dev.getRSSI();

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
        #endif
        
        // ピン出力  スキャン結果のうち、ピン情報を出力ピンに出力する
        if (pin_info_1 == 0) {
          digitalWrite(PIN_OUT_1, LOW);
        } else {
          digitalWrite(PIN_OUT_1, HIGH);
        }
        if (pin_info_2 == 0) {
          digitalWrite(PIN_OUT_2, LOW);
        } else {
          digitalWrite(PIN_OUT_2, HIGH);
        }
        if (pin_info_3 == 0) {
          digitalWrite(PIN_OUT_3, LOW);
        } else {
          digitalWrite(PIN_OUT_3, HIGH);
        }

        break;
      }
    }
  }

  // ピン入力
  int pin_in_1;
  int pin_in_2;
  int pin_in_3;
  pin_in_1 = digitalRead(PIN_IN_1);
  pin_in_2 = digitalRead(PIN_IN_2);
  pin_in_3 = digitalRead(PIN_IN_3);
  #ifdef DEBUG
    Serial.print("   PIN_IN(1,2,3):(");
    Serial.print(pin_in_1);
    Serial.print(pin_in_2);
    Serial.print(pin_in_3);
    Serial.println(")");
  #endif
  
  // ピン出力は、BLEのスキャン結果により出力する
  ;

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
