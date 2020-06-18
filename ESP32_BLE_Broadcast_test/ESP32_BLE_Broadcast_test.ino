// 2020/06/03
//
// 参考
// http://marchan.e5.valueserver.jp/cabin/comp/jbox/arc212/doc21202.html

#include <BLEDevice.h>
#include <BLEServer.h>
//#include <BLEUtils.h>
//#include <BLEScan.h>
//#include <BLEAdvertisedDevice.h>


/* 基本属性定義  */
#define BLE_DEVICE_NAME "KADOTA"        // デバイス名
#define BLE_DEVICE_NUMBER 1             // デバイス識別番号（1～8）

uint8_t seq_number;  // シーケンス番号。ディープスリープ運用するときは、メモリクリアされちゃうかも。  'RTC_DATA_ATTR static uint8_t seq_number' と宣言したほうが良いかも?

const int advertising_sec = 1;  // アドバタイジング時間[sec]

BLEServer *pBLEServer;
BLEAdvertising *pBLEAdvertising;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  // BLEデバイスの初期化
  BLEDevice::init(BLE_DEVICE_NAME);

  // BLEサーバ作成
  pBLEServer = BLEDevice::createServer();
  
  // アドバタイジングオプジェクト取得
  pBLEAdvertising = pBLEServer->getAdvertising();

  // シーケンス番号を初期化
  seq_number = 0;
  Serial.println("Initialized.");
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(100);

  setAdvertisingData(pBLEAdvertising);  // アドバタイズデータを作成
  pBLEAdvertising->start();
  Serial.println("Advertising started! " + (int)seq_number);
  delay(advertising_sec * 1000);
  pBLEAdvertising->stop();
  Serial.println("Advertising stoped! " + (int)seq_number);

  seq_number ++;
  // wait
  delay(5000);
}

void setAdvertisingData(BLEAdvertising* pBLEAdvertising) {
  std::string strData = "";
  strData += (char)0xff;                  // manufacturer ID low byte
  strData += (char)0xff;                  // manufacturer ID high byte
  strData += (char)BLE_DEVICE_NUMBER;     // サーバー識別番号
  strData += (char)seq_number;            // シーケンス番号
  strData += (char)0x01;
  strData += (char)0x02;
  strData += (char)0x03;
  strData += (char)0x04;
  strData += (char)0x05;

  // デバイス名とフラグをセットし、送信情報を組み込んでアドバタイズオブジェクトに設定する
  BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
  oAdvertisementData.setName(BLE_DEVICE_NAME);
  oAdvertisementData.setFlags(0x06);      // LE General Discoverable Mode | BR_EDR_NOT_SUPPORTED
  oAdvertisementData.setManufacturerData(strData);
  Serial.println(oAdvertisementData.getPayload().c_str());
  pBLEAdvertising->setAdvertisementData(oAdvertisementData);
}
