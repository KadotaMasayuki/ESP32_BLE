// 2020/06/03,12,22
//
// 参考
// http://marchan.e5.valueserver.jp/cabin/comp/jbox/arc212/doc21203.html

//
// Environment...
//   Arduino 1.8.13
//   ESP32 1.0.4
//

#include <BLEDevice.h>


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


/* 基本属性定義  */
#define BLE_DEVICE_NAME "AECBC"        // デバイス名(サービス名)
#define BLE_DEVICE_NUMBER 1            // デバイス識別番号 0 - 65535

const int scanning_sec = 2;   // スキャン時間[sec]
const int delay_msec = 1000;  // ループごとの休止時間[msec]

BLEScan *pScan;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  // BLEデバイスの初期化
  BLEDevice::init("");  // スキャンするだけなので、名乗る必要はない
  
  // スキャンオプジェクト取得
  pScan = BLEDevice::getScan();

  // パッシブスキャンにする(アクティブスキャンは、デバイスを見つけたらデータを取りに行く)
  pScan->setActiveScan(false);

  // ピン設定
  pinMode(PIN_IN_1, INPUT_PULLUP);
  pinMode(PIN_IN_2, INPUT_PULLUP);
  pinMode(PIN_IN_3, INPUT_PULLUP);
  pinMode(PIN_OUT_1, OUTPUT);
  pinMode(PIN_OUT_2, OUTPUT);
  pinMode(PIN_OUT_3, OUTPUT);

  Serial.println("Initialized.");
}

void loop() {
  int manu_code = 0;
  uint16_t device_number = 0;
  int seq_number = 0;
  int pin_info_1 = 0;
  int pin_info_2 = 0;
  int pin_info_3 = 0;
  int rssi = 0;
  
  delay(100);
  Serial.println();

  // スキャン開始
  Serial.println("Scanning!");
  BLEScanResults foundDevices = pScan->start(scanning_sec);
  int foundCount = foundDevices.getCount();

  // 見つかったデバイス群から、指定のデバイス(BLE_DEVICE_NAMEのもの)を処理
  for (int i = 0; i < foundCount; i ++) {
    BLEAdvertisedDevice dev = foundDevices.getDevice(i);
    std::string device_name = dev.getName();
    if (device_name == BLE_DEVICE_NAME) {
      if (dev.haveManufacturerData()) {
        std::string data = dev.getManufacturerData();
        manu_code = data[1] << 8 | data[0];
        device_number = (((uint16_t)data[3]) << 8) | (uint16_t)data[2];
        seq_number = data[4];
        pin_info_1 = data[5];
        pin_info_2 = data[6];
        pin_info_3 = data[7];
        rssi = dev.getRSSI();

        Serial.print("Device Number:");
        Serial.print(device_number);

        Serial.print("   SN:");
        Serial.print(seq_number);

        Serial.print("   INFO(1,2,3):(");
        Serial.print(pin_info_1);
        Serial.print(pin_info_2);
        Serial.print(pin_info_3);
        Serial.print(")");
        
        Serial.print("   RSSI:");
        Serial.print(rssi);
        Serial.println("[dBm]");

        Serial.print("RAW:");
        for (int i = 0; i < data.length(); i ++) {
          Serial.print((char)data[i], HEX);
          Serial.print(" ");
        }
        Serial.println();
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
  Serial.print("PIN_IN(1,2,3):(");
  Serial.print(pin_in_1);
  Serial.print(pin_in_2);
  Serial.print(pin_in_3);
  Serial.println(")");
  
  // ピン出力  スキャン結果を出力する
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

  // wait
  Serial.print("Waiting ");
  Serial.print(delay_msec);
  Serial.println("[ms]");
  delay(delay_msec);
}
