// 2020/06/30,07/22,08/03
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


// シリアルモニタによるデバッグをしないときはコメントアウト
#define DEBUG 1


// 入出力ピン情報
// IN1 (GPI36)
// IN2 (GPI39)
// IN3 (GPI34)
// OUT1 (GPIO25)
// OUT2 (GPIO26)
// OUT3 (GPIO27)
#define PIN_IN_1  36
#define PIN_IN_2  39
#define PIN_IN_3  34
#define PIN_OUT_1 25
#define PIN_OUT_2 26
#define PIN_OUT_3 27


// スリープ時間
#define MS_TO_SLEEP 350


// シーケンス番号
//uint8_t seq_number;  // ディープスリープ運用するときは、メモリクリアされるので 'RTC_DATA_ATTR static uint8_t seq_number' と宣言したほうが良いかも?
RTC_DATA_ATTR uint8_t seq_number = 0;


// BLE
#define BLE_DEVICE_NAME "AECBC"        // デバイス名(サービス名)
#define BLE_DEVICE_NUMBER 100            // デバイス識別番号 0 - 65535
const int advertising_msec = 350;  // アドバタイジング時間[msec]
BLEServer *pBLEServer;
BLEAdvertising *pBLEAdvertising;

// BLE機能タイプ
#define FUNCTION_LENGTH_ACC 9   // 追加の機能タイプ(加速度)のデータ長(「データ長」と「機能タイプ識別子」含む)
#define FUNCTION_TYPE_ACC 1     // 追加の機能タイプ(加速度)識別子

// 加速度センサADXL345
#include <Wire.h>
#define ADXL345_ADDRESS (0x1d)  // 0x53(pin SD0 to L) or 0x1d(pin SD0 to H)
#define HISTORY_SIZE 3
uint8_t regTbl[6];
RTC_DATA_ATTR float x_m_s2_array[HISTORY_SIZE]; // [m/s2]  // X軸の履歴
RTC_DATA_ATTR float y_m_s2_array[HISTORY_SIZE]; // [m/s2]  // Y軸の履歴
RTC_DATA_ATTR float z_m_s2_array[HISTORY_SIZE]; // [m/s2]  // Z軸の履歴
const float x_m_s2_max = 0.2; // [m/s2]  // 動いているかどうかの閾値
const float y_m_s2_max = 0.2; // [m/s2]  // 動いているかどうかの閾値
const float z_m_s2_max = 0.2; // [m/s2]  // 動いているかどうかの閾値


void setup() {
  // put your setup code here, to run once:

  //////////////////////////////////////////////////////////////////////////
  // 変数設定
  //////////////////////////////////////////////////////////////////////////

  // シーケンス番号を初期化
  seq_number = 0;

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
  // I2C
  //////////////////////////////////////////////////////////////////////////

  // I2C設定
  Wire.begin();

  // DATA_FORMAT(データ形式の制御)設定開始
  // addr : 0x31
  // data : 0x0b(13bit, +/-16g),  0x03(10bit, +/-16g)
  writeI2c(ADXL345_ADDRESS, 0x31, 0x0b);
  
  // POWER_TCL(節電機能の制御)設定開始
  // addr : 0x2d
  // data : 0x08(measure mode = do not sleep)
  writeI2c(ADXL345_ADDRESS, 0x2d, 0x08);

  //////////////////////////////////////////////////////////////////////////
  // BLE
  //////////////////////////////////////////////////////////////////////////
  
  // BLEデバイスの初期化
  BLEDevice::init(BLE_DEVICE_NAME);

  // BLEサーバ作成
  pBLEServer = BLEDevice::createServer();
  
  // アドバタイジングオプジェクト取得
  pBLEAdvertising = pBLEServer->getAdvertising();

  #ifdef DEBUG
    Serial.println("Initialized.");
  #endif
}


void loop() {
  // シーケンス番号インクリメント
  seq_number ++;

  //////////////////////////////////////////////////////////////////////////
  // ピン
  //////////////////////////////////////////////////////////////////////////

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
    Serial.print(",");
    Serial.print(pin_in_2);
    Serial.print(",");
    Serial.print(pin_in_3);
    Serial.println(")");
  #endif
  
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

  //////////////////////////////////////////////////////////////////////////
  // I2C
  //////////////////////////////////////////////////////////////////////////
  
  // XYZ値の先頭アドレスから6byteのレジスタデータを要求、取得
  readI2c(ADXL345_ADDRESS, 0x32, 6, regTbl);
  
  // データを各XYZの値に変換する(LSB単位)
  // 13bit値 で +/-16[g] を表現するため、値1あたり、32/(2^13)=0.00390625[g]
  // 1[g] = 9.80665[m/s2]より、値1あたり、0.00390625*9.80665=0.03830723[m/s2]
  int16_t x;  // ADXL345から読んだ値
  int16_t y;
  int16_t z;
  float x_g;  // [G] に換算
  float y_g;
  float z_g;
  float x_m_s2;  // [m/s^2]に換算
  float y_m_s2;
  float z_m_s2;
  x = (((int16_t)regTbl[1]) << 8) | regTbl[0];
  y = (((int16_t)regTbl[3]) << 8) | regTbl[2];
  z = (((int16_t)regTbl[5]) << 8) | regTbl[4];
  x_g = x * 0.00390625;
  y_g = y * 0.00390625;
  z_g = z * 0.00390625;
  x_m_s2 = x_g * 9.80665;
  y_m_s2 = y_g * 9.80665;
  z_m_s2 = z_g * 9.80665;

  // 加速度バッファを更新
  for (int i = 1; i < HISTORY_SIZE; i ++) {
    x_m_s2_array[i - 1] = x_m_s2_array[i];
    y_m_s2_array[i - 1] = y_m_s2_array[i];
    z_m_s2_array[i - 1] = z_m_s2_array[i];
  }
  x_m_s2_array[HISTORY_SIZE - 1] = x_m_s2;
  y_m_s2_array[HISTORY_SIZE - 1] = y_m_s2;
  z_m_s2_array[HISTORY_SIZE - 1] = z_m_s2;

  // 各XYZ軸の加速度([raw][G][m/s^2])を出力する
  #ifdef DEBUG
    Serial.print("   X:");
    Serial.print(x);
    Serial.print(",");
    Serial.print(x_g);
    Serial.print("[G],");
    Serial.print(x_m_s2);
    Serial.print("[m/s2]");
    Serial.print(" / Y:");
    Serial.print(y);
    Serial.print(",");
    Serial.print(y_g);
    Serial.print("[G],");
    Serial.print(y_m_s2);
    Serial.print("[m/s2]");
    Serial.print(" / Z:");
    Serial.print(z);
    Serial.print(",");
    Serial.print(z_g);
    Serial.print("[G],");
    Serial.print(z_m_s2);
    Serial.print("[m/s2]");
    Serial.println();
  #endif

  // 過去の加速度値と比較し、動いているか判断
  bool is_x_moving = false;
  bool is_y_moving = false;
  bool is_z_moving = false;
  for (int i = 1; i < HISTORY_SIZE; i ++) {
    if (abs(x_m_s2_array[i-1] - x_m_s2_array[i]) > x_m_s2_max) {
      is_x_moving = true;
    }
    if (abs(y_m_s2_array[i-1] - y_m_s2_array[i]) > y_m_s2_max) {
      is_y_moving = true;
    }
    if (abs(z_m_s2_array[i-1] - z_m_s2_array[i]) > z_m_s2_max) {
      is_z_moving = true;
    }
  }

  // 各軸が動いているかを出力する
  #ifdef DEBUG
    Serial.print("   moving (x, y, z) = (");
    Serial.print(is_x_moving);
    Serial.print(",");
    Serial.print(is_y_moving);
    Serial.print(",");
    Serial.print(is_z_moving);
    Serial.print(")");
    Serial.println();
  #endif

  //////////////////////////////////////////////////////////////////////////
  // BLE
  //////////////////////////////////////////////////////////////////////////

  // アドバタイズデータを作成
  setAdvertisingData(pBLEAdvertising, pin_in_1, pin_in_2, pin_in_3, x, is_x_moving, y, is_y_moving, z, is_z_moving);
  // アドバタイズ開始
  pBLEAdvertising->start();
  #ifdef DEBUG
    Serial.print("Advertising ");
    Serial.print(advertising_msec);
    Serial.print("[ms]  count=");
    Serial.print((int)seq_number);
  #endif
  // アドバタイズ時間だけ待つ
  delay(advertising_msec);
  // アドバタイズ停止
  pBLEAdvertising->stop();
  #ifdef DEBUG
    Serial.println(" ... stoped! ");
  #endif

  //////////////////////////////////////////////////////////////////////////
  // Wait
  //////////////////////////////////////////////////////////////////////////

  #ifdef DEBUG
    Serial.print("Loop Waiting ");
    Serial.print(MS_TO_SLEEP);
    Serial.println("[ms]");
    Serial.println();
  #endif
  delay(MS_TO_SLEEP);
}


// I2C書き込み 
void writeI2c(byte device_addr, byte register_addr, byte value) {
  Wire.beginTransmission(device_addr);
  Wire.write(register_addr);
  Wire.write(value);
  int result = Wire.endTransmission();
  #ifdef DEBUG
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
  #endif
}

// I2c読み出し
void readI2c(byte device_addr, byte register_addr, int len, byte buffer[]) {
  Wire.beginTransmission(device_addr);
  Wire.write(register_addr);
  int result = Wire.endTransmission(false);
  #ifdef DEBUG
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
  #endif
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


void setAdvertisingData(BLEAdvertising* pBLEAdvertising, int pin_in_1, int pin_in_2, int pin_in_3, int16_t x, bool is_x_moving, int16_t y, bool is_y_moving, int16_t z, bool is_z_moving) {
  // Manufacturer Data のみで運用する。
  // Advertiseデータの長さは規格上31バイト。本プログラムでこの全てを使うが、このうち、下記strDataに使えるのは29バイト。
  std::string strData = "";
  strData += (char)0xff;                            // manufacturer ID low byte (屋内用途であれば、未認証でも0xffffを使える)
  strData += (char)0xff;                            // manufacturer ID high byte (屋内用途であれば、未認証でも0xffffを使える)
  strData += (char)(((uint16_t)BLE_DEVICE_NUMBER) & 0xff);         // デバイス識別番号 最下位バイト
  strData += (char)((((uint16_t)BLE_DEVICE_NUMBER) >> 8) & 0xff);  // デバイス識別番号 最上位バイト
  strData += (char)seq_number;                      // シーケンス番号
  strData += (char)(pin_in_1 << 2) | (pin_in_2 << 1) | pin_in_3;  // ピン入力情報
  // ここまでが基本情報。
  // ここから追加情報
  strData += (char)FUNCTION_LENGTH_ACC;     // 加速度情報の長さ
  strData += (char)FUNCTION_TYPE_ACC;       // 加速度情報識別子
  strData += (char)(x & 0xff);            // 加速度Xの下位ビット
  strData += (char)((x >> 8) & 0xff);
  strData += (char)(y & 0xff);            // 加速度Yの下位ビット
  strData += (char)((y >> 8) & 0xff);
  strData += (char)(z & 0xff);            // 加速度Zの下位ビット
  strData += (char)((z >> 8) & 0xff);
  strData += (char)(is_x_moving << 2) | (is_y_moving << 1) | is_z_moving;    // 加速度X,Y,Zによる、「動作中」判定

  // デバイス名とフラグをセットし、送信情報を組み込んでアドバタイズオブジェクトに設定する
  BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
  oAdvertisementData.setName(BLE_DEVICE_NAME);
  oAdvertisementData.setFlags(0x06);      // LE General Discoverable Mode | BR_EDR_NOT_SUPPORTED
  oAdvertisementData.setManufacturerData(strData);
  #ifdef DEBUG
    std::string payload = oAdvertisementData.getPayload();
    for (int i = 0; i < payload.length(); i ++) {
      Serial.print((char)payload[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
  #endif
  pBLEAdvertising->setAdvertisementData(oAdvertisementData);
}
