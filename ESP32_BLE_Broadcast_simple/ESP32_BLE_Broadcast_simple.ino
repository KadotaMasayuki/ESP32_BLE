// 2020/06/03,12,22
//
// 参考
// http://marchan.e5.valueserver.jp/cabin/comp/jbox/arc212/doc21202.html

//
// Environment...
//   Arduino 1.8.13
//   ESP32 1.0.4
//

#include <BLEDevice.h>
#include <BLEServer.h>


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

uint8_t seq_number;  // シーケンス番号。ディープスリープ運用するときは、メモリクリアされちゃうかも。  'RTC_DATA_ATTR static uint8_t seq_number' と宣言したほうが良いかも?

const int advertising_msec = 1000;  // アドバタイジング時間[msec]
const int delay_msec = 1000;  // ループごとの休止時間[msec]

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
  delay(100);
  Serial.println();

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
  
  // ピン出力  ピン入力をそのまま出力する
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

  // アドバタイズデータを作成
  setAdvertisingData(pBLEAdvertising, pin_in_1, pin_in_2, pin_in_3);
  // アドバタイズ開始
  pBLEAdvertising->start();
  Serial.print("Advertising ");
  Serial.print(advertising_msec);
  Serial.print("[ms] ");
  delay(advertising_msec);
  // アドバタイズ停止
  pBLEAdvertising->stop();
  Serial.print(" ... stoped! ");
  Serial.println((int)seq_number);

  seq_number ++;
  // wait
  Serial.print("Waiting ");
  Serial.print(delay_msec);
  Serial.println("[ms]");
  delay(delay_msec);
}

void setAdvertisingData(BLEAdvertising* pBLEAdvertising, int pin_in_1, int pin_in_2, int pin_in_3) {
  unsigned long num = (unsigned long)BLE_DEVICE_NUMBER;
  
  std::string strData = "";
  strData += (char)0xff;                            // manufacturer ID low byte
  strData += (char)0xff;                            // manufacturer ID high byte
  strData += (char)((uint16_t)num & 0xff);           // サーバー識別番号 最下位バイト
  strData += (char)((((uint16_t)num) >> 8) & 0xff);  // サーバー識別番号 最上位バイト
  strData += (char)seq_number;                      // シーケンス番号
  strData += (char)pin_in_1;
  strData += (char)pin_in_2;
  strData += (char)pin_in_3;

  // デバイス名とフラグをセットし、送信情報を組み込んでアドバタイズオブジェクトに設定する
  BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
  oAdvertisementData.setName(BLE_DEVICE_NAME);
  oAdvertisementData.setFlags(0x06);      // LE General Discoverable Mode | BR_EDR_NOT_SUPPORTED
  oAdvertisementData.setManufacturerData(strData);
  std::string payload = oAdvertisementData.getPayload();
  for (int i = 0; i < payload.length(); i ++) {
    Serial.print((char)payload[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
  pBLEAdvertising->setAdvertisementData(oAdvertisementData);
}
