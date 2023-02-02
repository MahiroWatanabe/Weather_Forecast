#include <M5Core2.h>
#include "M5_ENV.h"
#include "time.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

SHT3X sht30;
QMP6988 qmp6988;

float tmp      = 0.0;
float hum      = 0.0;
float pressure = 0.0;

//時刻の表示
const char* ntpServer = "ntp.nict.jp";
const long  gmtOffset_sec = 3600 * 9;
const int   daylightOffset_sec = 0;

//WiFi初期設定-----------------
#include "WiFiClient.h"
WiFiClient client;
const char* ssid = "F660A-h2b6-A"; //wifiのssid
const char* password = "k4dxx2aq"; //wifiのパスワード
//2.4GHzで接続すること

//ambient初期設定-------------------------------------
#include "Ambient.h" // Ambientのヘッダーをインクルード
Ambient ambient; // Ambientオブジェクトを定義
unsigned int channelId = 57339; // AmbientのチャネルID
const char* writeKey = "458df91f87173b03"; // ライトキー

//天気API-------------------------------
const String endpoint = "http://api.openweathermap.org/data/2.5/weather?q=";
const String endpoint2 = "http://api.openweathermap.org/data/2.5/forecast?q=";
const String Local[] = {"tokyo", "hokkaido", "fukuoka", "sendai", "osaka", "niigata", "naha"};
const String key = ",jp&APPID=f4789af7a66473596deff651bf9622fb";

void setup() {
  M5.begin();
  Serial.begin(115200);

  M5.lcd.setTextSize(2);
  Wire.begin();
  qmp6988.init();
  //wifi------------
  WiFi.begin(ssid, password);  //  Wi-Fi APに接続
  while (WiFi.status() != WL_CONNECTED) {  //  Wi-Fi AP接続待ち
    delay(100);
  }
  //ambient---------------------------------------------------------------------------
  ambient.begin(channelId, writeKey, &client); // チャネルIDとライトキーを指定してAmbientの初期化
  //時刻の表示
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  //時刻の表示
  printLocalTime();
  //
}

void loop() {
  delay(1);
  if (M5.BtnA.wasPressed()) {
    drawWeather();
  }
  if (M5.BtnB.wasPressed()) {
    drawCurrent();
  }
  if (M5.BtnC.wasPressed()) {
    drawAllWeather();
  }
  M5.update();
}

void drawWeather() {
  M5.Lcd.clear();
  //時刻の表示
  printLocalTime();
  //
  ///東京の天気表示
  HTTPClient http;
  http.begin(endpoint + "tokyo" + key);
  int httpCode = http.GET();

  if (httpCode > 0) {
    String payload = http.getString();

    DynamicJsonBuffer jsonBuffer;
    String json = payload;
    JsonObject& weatherdata = jsonBuffer.parseObject(json);
    if (!weatherdata.success()) {
      Serial.println("parseObject() failed");
    }

    const char* weather = weatherdata["weather"][0]["main"].as<char*>();
    const double matemp = weatherdata["main"]["temp_max"].as<double>();
    const double mitemp = weatherdata["main"]["temp_min"].as<double>();

    M5.Lcd.setTextColor(RED);
    M5.Lcd.setTextSize(3);
    M5.Lcd.setCursor(27, 80);
    M5.Lcd.print(matemp - 273.15);

    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setCursor(97, 80);
    M5.Lcd.print("|");

    M5.Lcd.setTextColor(BLUE);
    M5.Lcd.setCursor(117, 80);
    M5.Lcd.print(mitemp - 273.15);
  }

  else {
    Serial.println("Error on HTTP request");
  }

  http.end();
  ///最高気温、最低気温表示
  ///3時間ごとの天気予報表示
  http.begin(endpoint2 + "tokyo" + key);
  int httpCode2 = http.GET();

  if (httpCode2 > 0) {
    String payload2 = http.getString();

    DynamicJsonBuffer jsonBuffer;
    String json2 = payload2;
    JsonObject& weatherdata2 = jsonBuffer.parseObject(json2);

    if (!weatherdata2.success()) {
      Serial.println("parseObject() failed");
    }
    const double temp1 = weatherdata2["list"][0]["main"]["temp"].as<double>();
    const double temp2 = weatherdata2["list"][2]["main"]["temp"].as<double>();
    const double temp3 = weatherdata2["list"][4]["main"]["temp"].as<double>();
    const double temp4 = weatherdata2["list"][6]["main"]["temp"].as<double>();
    const double sumtmp = (temp1 + temp2 + temp3 + temp4) / 4;
    M5.Lcd.setTextSize(3);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setCursor(27, 150);
    M5.Lcd.print("0-");
    weatherjudge(temp1, 27, 190);
    M5.Lcd.setCursor(92, 150);
    M5.Lcd.print("6-");
    weatherjudge(temp2, 92, 190);
    M5.Lcd.setCursor(165, 150);
    M5.Lcd.print("12-");
    weatherjudge(temp3, 165, 190);
    M5.Lcd.setCursor(245, 150);
    M5.Lcd.print("18-");
    weatherjudge(temp4, 245, 190);
    double d = sumtmp - 273.15;
    int x = 200;
    int y = 30;
    if (d <= 0 ) {
      M5.Lcd.drawJpgFile(SD, "/sensornetwork/11.jpg", x, y);
    } else if (0 < d && d <= 15) {
      M5.Lcd.drawJpgFile(SD, "/sensornetwork/5.jpg", x, y);
    } else if (15 < d && d <= 25) {
      M5.Lcd.drawJpgFile(SD, "/sensornetwork/2.jpg", x, y);
    } else if (25 < d) {
      M5.Lcd.drawJpgFile(SD, "/sensornetwork/1.jpg", x, y);
    }
  } else {
    Serial.println("Error on HTTP request");
  }
  http.end();
  ///
  delay(30000);
  ///
}

void weatherjudge(double tmp, int x, int y) {
  double v = tmp - 273.15;
  jpeg_div_t scale = JPEG_DIV_4;
  if (v <= 0 ) {
    M5.Lcd.drawJpgFile(SD, "/sensornetwork/11.jpg", x, y, 0, 0, 0, 0, scale);
  } else if (0 < v && v <= 15) {
    M5.Lcd.drawJpgFile(SD, "/sensornetwork/5.jpg", x, y, 0, 0, 0, 0, scale);
  } else if (15 < v && v <= 25) {
    M5.Lcd.drawJpgFile(SD, "/sensornetwork/2.jpg", x, y, 0, 0, 0, 0, scale);
  } else if (25 < v) {
    M5.Lcd.drawJpgFile(SD, "/sensornetwork/1.jpg", x, y, 0, 0, 0, 0, scale);
  }
}

void drawCurrent() {
  M5.Lcd.clear();
  //時刻の表示
  printLocalTime();
  //
  pressure = qmp6988.calcPressure();
  if (sht30.get() == 0) {
    tmp = sht30.cTemp;
    hum = sht30.humidity;
  } else {
    tmp = 0, hum = 0;
  }
  ///一番下の気温、湿度、圧力を表示
  M5.Lcd.setTextColor(RED);
  M5.lcd.setTextSize(3);
  M5.lcd.setCursor(27, 80);
  M5.Lcd.printf("%2.1f  \r\n", tmp);
  M5.Lcd.setTextColor(BLUE);
  M5.lcd.setCursor(27, 130);
  M5.Lcd.printf("%2.0f%%  \r\n", hum);
  M5.Lcd.setTextColor(WHITE);
  M5.lcd.setCursor(27, 180);
  M5.Lcd.printf("%2.0fPa\r\n", pressure);
  //////////////////////////////////////////////////////////
  ///画像の表示
  ///天気のアイコン表示
  int tx = 200;
  int ty = 30;
  int ttx = 220;
  int tty = 150;
  jpeg_div_t scale = JPEG_DIV_4;
  if (tmp <= 0 ) {
    M5.Lcd.drawJpgFile(SD, "/sensornetwork/11.jpg", tx, ty);
    M5.Lcd.drawJpgFile(SD, "/sensornetwork/19.jpg", ttx, tty, 0, 0, 0, 0, scale);
  } else if (0 < tmp && tmp <= 15) {
    M5.Lcd.drawJpgFile(SD, "/sensornetwork/5.jpg", tx, ty);
    M5.Lcd.drawJpgFile(SD, "/sensornetwork/18.jpg", ttx, tty, 0, 0, 0, 0, scale);
  } else if (15 < tmp && tmp <= 25) {
    M5.Lcd.drawJpgFile(SD, "/sensornetwork/2.jpg", tx, ty);
    M5.Lcd.drawJpgFile(SD, "/sensornetwork/20.jpg", ttx, tty, 0, 0, 0, 0, scale);
  } else if (25 < tmp) {
    M5.Lcd.drawJpgFile(SD, "/sensornetwork/1.jpg", tx, ty);
    M5.Lcd.drawJpgFile(SD, "/sensornetwork/16.jpg", ttx, tty, 0, 0, 0, 0, scale);
    if (70 <= hum) {
      M5.Lcd.drawJpgFile(SD, "/sensornetwork/17.jpg", ttx, tty, 0, 0, 0, 0, scale);
    }
  }
  ///ambient送信
  ambient.set(1, String(tmp).c_str());
  ambient.set(2, String(hum).c_str());
  ambient.set(3, String(pressure).c_str());

  ambient.send();

  delay(5000);  // Delay (ms)
}

void drawAllWeather() {
  M5.Lcd.clear();
  ///日本地図描画
  jpeg_div_t scale2 = JPEG_DIV_2;
  M5.Lcd.drawJpgFile(SD, "/sensornetwork/nihon2.jpg", 40, 0);
  //////
  //時刻の表示
  printLocalTime();
  //
  ///各地のAPI取得
  HTTPClient http;
  String v = "";
  for (int i = 0; i < 7; i++) {
    v = Local[i];
    http.begin(endpoint + v + key);
    int httpCode = http.GET();

    if (httpCode > 0) {
      String payload = http.getString();
      DynamicJsonBuffer jsonBuffer;
      String json = payload;
      JsonObject& weatherdata = jsonBuffer.parseObject(json);

      const char* weather = weatherdata["weather"][0]["main"].as<char*>();
      const double temp = weatherdata["main"]["temp"].as<double>();
      double v = temp - 273.15;
      int ttx = 0;
      int tty = 0;
      jpeg_div_t scale = JPEG_DIV_8;
      if (i == 0) {
        ttx = 220;
        tty = 170;
      } else if (i == 1) {
        ttx = 280;
        tty = 50;
      } else if (i == 2) {
        ttx = 10;
        tty = 140;
      } else if (i == 3) {
        ttx = 250;
        tty = 100;
      } else if (i == 4) {
        ttx = 140;
        tty = 190;
      } else if (i == 5) {
        ttx = 160;
        tty = 90;
      } else if (i == 6) {
        ttx = 100;
        tty = 40;
      }
      if (v <= 0 ) {
        M5.Lcd.drawJpgFile(SD, "/sensornetwork/19.jpg", ttx, tty, 0, 0, 0, 0, scale);
      } else if (0 < v && v <= 15) {
        M5.Lcd.drawJpgFile(SD, "/sensornetwork/18.jpg", ttx, tty, 0, 0, 0, 0, scale);
      } else if (15 < v && v <= 25) {
        M5.Lcd.drawJpgFile(SD, "/sensornetwork/20.jpg", ttx, tty, 0, 0, 0, 0, scale);
      } else if (25 < v) {
        M5.Lcd.drawJpgFile(SD, "/sensornetwork/16.jpg", ttx, tty, 0, 0, 0, 0, scale);
      } else if (30 <= v) {
        M5.Lcd.drawJpgFile(SD, "/sensornetwork/17.jpg", ttx, tty, 0, 0, 0, 0, scale);
      }
    } else {
      Serial.println("Error on HTTP request");
    }
    http.end();
  }
  delay(30000);
}

///時刻の表示
void printLocalTime() {
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {
    M5.Lcd.println(" ");
    return;
  }
  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.printf("%04d/%02d/%02d", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
}
