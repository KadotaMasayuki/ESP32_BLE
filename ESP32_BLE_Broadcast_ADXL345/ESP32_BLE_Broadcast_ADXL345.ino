// 2020/06/30
//  BLEのアドバタイズで3軸加速度センサ(ADXL345)の値を飛ばす
//
// 参考
// http://marchan.e5.valueserver.jp/cabin/comp/jbox/arc212/doc21202.html

//
// Environment...
//   Arduino 1.8.13
//   ESP32 1.0.4
//   ADXL345 : 3-Axis, ±2 g/±4 g/±8 g/±16 g Digital Accelerometer

#include <BLEDevice.h>
#include <BLEServer.h>
//#include <BLEUtils.h>
//#include <BLEScan.h>
//#include <BLEAdvertisedDevice.h>


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
const int delay_msec = 300;  // ループごとの休止時間[msec]

BLEServer *pBLEServer;
BLEAdvertising *pBLEAdvertising;


// 加速度センサADXL345
#include <Wire.h>
#define ADXL345_ADDRESS (0x53)  // 0x53(pin SD0 to L) or 0x1d(pin SD0 to H)
#define HISTORY_SIZE 4
uint8_t regTbl[6];
float xx[HISTORY_SIZE]; // [m/s2]
float yy[HISTORY_SIZE]; // [m/s2]
float zz[HISTORY_SIZE]; // [m/s2]
const float x_max = 0.2; // [m/s2]
const float y_max = 0.2; // [m/s2]
const float z_max = 0.2; // [m/s2]

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(500);

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

  // I2C設定
  Wire.begin();

  //DATA_FORMAT(データ形式の制御)設定開始
  // addr : 0x31
  // data : 0x0b(13bit, +/-16g),  0x03(10bit, +/-16g)
  writeI2c(ADXL345_ADDRESS, 0x31, 0x0b);
  
  //POWER_TCL(節電機能の制御)設定開始
  // addr : 0x2d
  // data : 0x08(measure mode = do not sleep)
  writeI2c(ADXL345_ADDRESS, 0x2d, 0x08);
  
  Serial.println("Initialized.");
}

// I2C書き込み 
void writeI2c(byte device_addr, byte register_addr, byte value) {
  Wire.beginTransmission(device_addr);
  Wire.write(register_addr);
  Wire.write(value);
  int result = Wire.endTransmission();
  Serial.print("WRITE_I2C:");
  Serial.print(device_addr);
  Serial.print("/");
  Serial.print(register_addr);
  Serial.print("/");
  Serial.print(value);
  Serial.print("/");
  if (result == 0) {
    Serial.print("Success");
  } else if (result == 1) {
    Serial.print("Fail(1):data too long to fit in transmit buffer");
  } else if (result == 2) {
    Serial.print("Fail(2):received NACK on transmit of address");
  } else if (result == 3) {
    Serial.print("Fail(3):received NACK on transmit of data");
  } else if (result == 4) {
    Serial.print("Fail(4):other error");
  } else {
    Serial.print("Fail(");
    Serial.print(result);
    Serial.print("):other error");
  }
  Serial.println();
}

// I2c読み出し
void readI2c(byte device_addr, byte register_addr, int len, byte buffer[]) {
  Wire.beginTransmission(device_addr);
  Wire.write(register_addr);
  int result = Wire.endTransmission(false);
  Serial.print("READ_I2C:");
  Serial.print(device_addr);
  Serial.print("/");
  Serial.print(register_addr);
  Serial.print("/");
  Serial.print(len);
  Serial.print("/");
  if (result == 0) {
    Serial.print("Success");
  } else if (result == 1) {
    Serial.print("Fail(1):data too long to fit in transmit buffer");
  } else if (result == 2) {
    Serial.print("Fail(2):received NACK on transmit of address");
  } else if (result == 3) {
    Serial.print("Fail(3):received NACK on transmit of data");
  } else if (result == 4) {
    Serial.print("Fail(4):other error");
  } else  {
    Serial.print("Fail(");
    Serial.print(result);
    Serial.print("):other error");
  }
  Serial.println();
  if (result != 0) {
    return;
  }
  Wire.requestFrom(device_addr, len, true);
  int i = 0;
  while(Wire.available())
  {
    buffer[i] = Wire.read();
    i++;
  }
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

  // I2C
  
  // XYZの先頭アドレスに移動、6byteのレジスタデータを要求、取得
  readI2c(ADXL345_ADDRESS, 0x32, 6, regTbl);
  
  // データを各XYZの値に変換する(LSB単位)
  // 13bit値 で +/-16[g] を表現するため、値1あたり、32/(2^13)=0.00390625[g]
  // 1[g] = 9.80665[m/s2]より、値1あたり、0.00390625*9.80665=0.03830723[m/s2]
  int16_t x = (((int16_t)regTbl[1]) << 8) | regTbl[0];
  int16_t y = (((int16_t)regTbl[3]) << 8) | regTbl[2];
  int16_t z = (((int16_t)regTbl[5]) << 8) | regTbl[4];  
  float x_ = x * 0.03830723;
  float y_ = y * 0.03830723;
  float z_ = z * 0.03830723;

  // 加速度バッファを更新
  for (int i = 1; i < HISTORY_SIZE; i ++) {
    xx[i-1] = xx[i];
    yy[i-1] = yy[i];
    zz[i-1] = zz[i];
  }
  xx[HISTORY_SIZE - 1] = x_;
  yy[HISTORY_SIZE - 1] = y_;
  zz[HISTORY_SIZE - 1] = z_;
  
  // 過去の加速度値と比較し、動いているか判断
  bool is_x_moving = false;
  bool is_y_moving = false;
  bool is_z_moving = false;
  for (int i = 1; i < HISTORY_SIZE; i ++) {
    if (abs(xx[i-1] - xx[i]) > x_max) {
      is_x_moving = true;
    }
    if (abs(yy[i-1] - yy[i]) > y_max) {
      is_y_moving = true;
    }
    if (abs(zz[i-1] - zz[i]) > z_max) {
      is_z_moving = true;
    }
  }
  
  // 各XYZ軸の加速度(m/s^2)を出力する
  Serial.print("X : ");
  Serial.print(x);
  Serial.print(" ");
  Serial.print(x * 0.00390625);
  Serial.print("[G] ");
  Serial.print(x_);
  Serial.print("[m/s2]");
  if (is_x_moving) {
    Serial.print("  moving");
  }
  Serial.println();
  
  Serial.print("Y : ");
  Serial.print(y);
  Serial.print(" ");
  Serial.print(y * 0.00390625);
  Serial.print("[G] ");
  Serial.print(y_);
  Serial.print("[m/s2]");
  if (is_z_moving) {
    Serial.print("  moving");
  }
  Serial.println();

  Serial.print("Z : ");
  Serial.print(z);
  Serial.print(" ");
  Serial.print(z * 0.00390625);
  Serial.print("[G] ");
  Serial.print(z_);
  Serial.print("[m/s2]");
  if (is_z_moving) {
    Serial.print("  moving");
  }
  Serial.println();

  // アドバタイズデータを作成
  setAdvertisingData(pBLEAdvertising, pin_in_1, pin_in_2, pin_in_3, x, is_x_moving, y, is_y_moving, z, is_z_moving);
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

void setAdvertisingData(BLEAdvertising* pBLEAdvertising, int pin_in_1, int pin_in_2, int pin_in_3, int16_t x, bool is_x_moving, int16_t y, bool is_y_moving, int16_t z, bool is_z_moving) {
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
  strData += (char)(x & 0xff);
  strData += (char)((x >> 8) & 0xff);
  strData += (char)is_x_moving;
  strData += (char)(y & 0xff);
  strData += (char)((y >> 8) & 0xff);
  strData += (char)is_y_moving;
  strData += (char)(z & 0xff);
  strData += (char)((z >> 8) & 0xff);
  strData += (char)is_z_moving;

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
