// 2020/06/03,12
//
// 参考
// http://marchan.e5.valueserver.jp/cabin/comp/jbox/arc212/doc21203.html


#include <BLEDevice.h>



// 入出力ピン情報
// IN1 (PIN8, IO32)
// IN2 (PIN9, IO33)
// IN3 (PIN10, IO25)
// OUT1 (PIN28, IO17)
// OUT2 (PIN30, IO18)
// OUT3 (PIN31, IO19)
//

#define PIN_IN_1  32
#define PIN_IN_2  32
#define PIN_IN_3  25
#define PIN_OUT_1 17
#define PIN_OUT_2 18
#define PIN_OUT_3 19


/* 基本属性定義  */
#define BLE_DEVICE_NAME "AECBC"        // デバイス名(サービス名)
#define BLE_DEVICE_NUMBER 1            // デバイス識別番号 0 - 4294967295

const int scanning_sec = 2;  // スキャン時間[sec]
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
  unsigned int device_number = 0;
  int seq_number = 0;
  int pin_info_1 = 0;
  int pin_info_2 = 0;
  int pin_info_3 = 0;
  int rssi = 0;
  
  delay(100);

  // スキャン開始
  Serial.println("Scanning!");
  BLEScanResults foundDevices = pScan->start(scanning_sec);
  int foundCount = foundDevices.getCount();

  // 見つかったデバイス郡から、指定のデバイス(BLE_DEVICE_NAMEのもの)を処理
  for (int i = 0; i < foundCount; i ++) {
    BLEAdvertisedDevice dev = foundDevices.getDevice(i);
    std::string device_name = dev.getName();
    if (device_name == BLE_DEVICE_NAME) {
      if (dev.haveManufacturerData()) {
        std::string data = dev.getManufacturerData();
        manu_code = data[1] << 8 | data[0];
        device_number = data[5] << 24 | data[4] << 16 | data[3] << 8 | data[2];
        seq_number = data[6];
        pin_info_1 = data[7];
        pin_info_2 = data[8];
        pin_info_3 = data[9];
        rssi = dev.getRSSI();

        Serial.print("Device Number:");
        Serial.print(device_number);

        Serial.print("   INFO1:");
        Serial.print(pin_info_1);
        
        Serial.print("   INFO2:");
        Serial.print(pin_info_2);
        
        Serial.print("   INFO3:");
        Serial.print(pin_info_3);
        
        Serial.print("   RSSI:");
        Serial.print(rssi);
        Serial.println("dBm");

        Serial.print("RAW:");
        Serial.println(data.c_str());

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
  if (pin_info_1 == 0)
  {
    digitalWrite(PIN_OUT_1, LOW);
  }
  else
  {
    digitalWrite(PIN_OUT_1, HIGH);
  }
  if (pin_info_2 == 0)
  {
    digitalWrite(PIN_OUT_2, LOW);
  }
  else
  {
    digitalWrite(PIN_OUT_2, HIGH);
  }
  if (pin_info_3 == 0)
  {
    digitalWrite(PIN_OUT_3, LOW);
  }
  else
  {
    digitalWrite(PIN_OUT_3, HIGH);
  }

  // wait
  delay(delay_msec);
}
