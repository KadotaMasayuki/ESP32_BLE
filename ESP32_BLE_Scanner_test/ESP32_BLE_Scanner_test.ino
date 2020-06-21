// 2020/06/03
//
// 参考
// http://marchan.e5.valueserver.jp/cabin/comp/jbox/arc212/doc21203.html

#include <BLEDevice.h>


/* 基本属性定義  */
#define BLE_DEVICE_NAME "NAME"         // デバイス名
#define BLE_DEVICE_NUMBER 1             // デバイス識別番号（1～8）

const int scanning_sec = 5;  // スキャン時間[sec]

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

  Serial.println("Initialized.");
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(100);

  Serial.println("Scanning!");
  BLEScanResults foundDevices = pScan->start(scanning_sec);
  int foundCount = foundDevices.getCount();

  for (int i = 0; i < foundCount; i ++) {
    BLEAdvertisedDevice dev = foundDevices.getDevice(i);
    std::string device_name = dev.getName();
    if (device_name == BLE_DEVICE_NAME) {
      if (dev.haveManufacturerData()) {
        std::string data = dev.getManufacturerData();
        int manu_code = data[1] << 8 | data[0];
        int device_number = data[2];
        int seq_number = data[4];
        int8_t signal_power = dev.getTXPower();
        Serial.print("POWER:");
        Serial.print(signal_power);
        Serial.print("dBm  ");
        Serial.println(data.c_str());
      }
    }
  }
  // wait
  delay(5000);
}
