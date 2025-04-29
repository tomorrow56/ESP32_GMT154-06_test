#include <Arduino.h>
#include <LovyanGFX.hpp>
#include <WiFi.h>
#include <time.h>
#include "html_content.h"  // HTMLコンテンツを含むヘッダファイル
#include "wifi_manager.h"  // WiFi設定管理ライブラリ
#include "ntp_clock.h"    // NTP時計ライブラリ
#include "touch_manager.h" // タッチセンサー管理ライブラリ

// バックライト設定
#define BACKLIGHT_CHANNEL 0
#define BACKLIGHT_FREQ 5000
#define BACKLIGHT_RESOLUTION 8
#define BACKLIGHT_MAX 255
#define TFT_BACKLIGHT_ON LOW  // 元の設定に戻す

class LGFX : public lgfx::LGFX_Device {
private:
  lgfx::Panel_ST7789 _panel_instance;
  lgfx::Bus_SPI _bus_instance;
  // バックライト制御をライブラリから切り離す
public:
  LGFX(void) {
    {
      auto cfg = _bus_instance.config();
      cfg.spi_host = VSPI_HOST;
      cfg.spi_mode = 3;
      cfg.freq_write = 27000000;
      cfg.pin_sclk = 18;
      cfg.pin_mosi = 23;
      cfg.pin_miso = -1;
      cfg.pin_dc = 2;
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }
    {
      auto cfg = _panel_instance.config();
      cfg.pin_cs = -1;
      cfg.pin_rst = 4;
      cfg.pin_busy = -1;
      cfg.panel_width = 240;
      cfg.panel_height = 240;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      cfg.offset_rotation = 0;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits = 1;
      cfg.readable = false;
      cfg.invert = true;
      cfg.rgb_order = false;
      cfg.dlen_16bit = false;
      cfg.bus_shared = true;
      _panel_instance.config(cfg);
    }
    // バックライト制御をライブラリから切り離すため、この部分をコメントアウト
    /*
    {
      auto cfg = _light_instance.config();
      cfg.pin_bl = 25;
      cfg.invert = false;
      cfg.freq = BACKLIGHT_FREQ;
      cfg.pwm_channel = BACKLIGHT_CHANNEL;
      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);
    }
    */
    setPanel(&_panel_instance);
  }
};
LGFX lcd;

// ライブラリのインスタンス
WiFiManager wifiManager("ESP32-Clock-Setup", "12345678", IPAddress(192, 168, 4, 1));
NTPClock ntpClock(&lcd, "ntp.nict.jp", 9 * 3600, 0);
TouchManager touchManager(&lcd, 32, 83, 3000); // IO32をタッチセンサーとして使用

// バックライト設定
#define LCD_BACKLIGHT_PIN 25
#define LCD_BACKLIGHT_CHANNEL 0
#define LCD_BACKLIGHT_FREQ 5000
#define LCD_BACKLIGHT_RESOLUTION 8

// バックライトの明るさ設定値（0-255）
uint8_t backlightBrightness = 128; // 初期値は50%（128/255）

// バックライトをPWM制御する関数
void setBacklightBrightness(uint8_t brightness) {
  // PWM設定
  ledcSetup(LCD_BACKLIGHT_CHANNEL, LCD_BACKLIGHT_FREQ, LCD_BACKLIGHT_RESOLUTION);
  ledcAttachPin(LCD_BACKLIGHT_PIN, LCD_BACKLIGHT_CHANNEL);
  
  // 明るさの反転（LOWで点灯のため）
  ledcWrite(LCD_BACKLIGHT_CHANNEL, 255 - brightness);
  
  // 現在の明るさを更新
  backlightBrightness = brightness;
  
  Serial.printf("[DEBUG] バックライトの明るさを%d%%に設定\n", (brightness * 100) / 255);
}

// バックライトを点灯する関数
void turnOnBacklight() {
  // 現在の明るさでバックライトを点灯
  setBacklightBrightness(backlightBrightness);
}

// ネットワーク情報を表示する関数
void displayNetworkInfo() {
  // IPアドレス表示中フラグを設定
  TouchManager::isShowingIPAddress = true;

  lcd.fillScreen(TFT_BLACK);
  lcd.setTextColor(TFT_WHITE);
  lcd.setTextSize(1);
  lcd.setTextFont(2);

  lcd.setCursor(10, 10);
  lcd.println("Network Information");

  lcd.setCursor(10, 40);
  if (wifiManager.isConnected()) {
    lcd.println("WiFi: Connected");
    lcd.setCursor(10, 60);
    lcd.print("SSID: ");
    lcd.println(WiFi.SSID());
    lcd.setCursor(10, 80);
    lcd.print("IP: ");
    lcd.println(WiFi.localIP().toString());
    lcd.setCursor(10, 100);
    lcd.print("Signal: ");
    lcd.print(WiFi.RSSI());
    lcd.println(" dBm");
  } else if (wifiManager.getCurrentMode() == WiFiManager::MODE_AP_SETUP) {
    lcd.println("WiFi: AP Mode");
    lcd.setCursor(10, 60);
    lcd.println("AP Mode: ESP32-Clock-Setup");
    lcd.setCursor(10, 80);
    lcd.print("AP IP: ");
    lcd.println("192.168.4.1");
  } else {
    lcd.println("WiFi: Disconnected");
  }

  delay(3000);
  TouchManager::isShowingIPAddress = false;
}

// メインプログラムの初期化関数
void setup() {
  // シリアル通信の初期化
  Serial.begin(115200);
  delay(1000); // シリアルポートが安定するまで待機
  Serial.println("\nESP32 NTPアナログ時計起動");
  Serial.println("[DEBUG] シリアル初期化完了");
  
  // バックライトを最初に設定 - 50%の明るさで点灯
  setBacklightBrightness(backlightBrightness);
  Serial.printf("[DEBUG] バックライトの初期明るさ: %d%%\n", (backlightBrightness * 100) / 255);
  
  // ヒープメモリの確認
  Serial.printf("[DEBUG] 空きメモリ: %d bytes\n", ESP.getFreeHeap());
  
  // LCDの初期化
  Serial.println("[DEBUG] LCD初期化開始");
  
  // SPIピンを一度リセット
  pinMode(18, OUTPUT); // SCK
  pinMode(23, OUTPUT); // MOSI
  pinMode(2, OUTPUT);  // DC
  pinMode(4, OUTPUT);  // RST
  
  // バックライトを確実に点灯
  turnOnBacklight();
  
  // LCD初期化
  lcd.init();
  lcd.setRotation(0);
  // バックライトは手動制御するので、この設定は無効
  // lcd.setBrightness(BACKLIGHT_MAX);
  lcd.fillScreen(TFT_BLACK);
  
  // バックライトを再度確認
  turnOnBacklight();
  
  Serial.println("[DEBUG] LCD初期化完了");
  delay(500);
  
  // テストパターンを表示
  Serial.println("[DEBUG] テストパターン表示");
  lcd.drawRect(10, 10, 220, 220, TFT_RED);
  lcd.fillCircle(120, 120, 50, TFT_BLUE);
  lcd.drawLine(0, 0, 240, 240, TFT_GREEN);
  lcd.drawLine(0, 240, 240, 0, TFT_GREEN);
  delay(1000); // テストパターンを確認する時間
  
  // スプラッシュ画面表示
  Serial.println("[DEBUG] スプラッシュ画面表示");
  lcd.fillScreen(TFT_BLACK); // テストパターンを消去
  lcd.setTextColor(TFT_WHITE);
  lcd.setTextSize(1);
  lcd.setTextFont(2);
  lcd.setCursor(40, 100);
  lcd.println("ESP32 NTP Clock with OTA");
  lcd.setCursor(60, 120);
  lcd.println("Version 1.0");
  delay(1000); // スプラッシュ画面を確認する時間

  // ---（ネットワーク情報表示ブロックを削除）---
  
  // WiFi管理ライブラリの初期化
  Serial.println("[DEBUG] WiFiManager初期化開始");
  wifiManager.begin();
  Serial.println("[DEBUG] WiFiManager初期化完了");
  delay(500);
  
  // タッチセンサーの初期化
  Serial.println("[DEBUG] タッチセンサー初期化");
  touchManager.begin();
  touchManager.setNTPClock(&ntpClock); // NTPClockの参照を設定
  Serial.println("[DEBUG] タッチセンサー初期化完了");
  delay(500);
  
  // 起動時にタッチされているか確認
  Serial.println("[DEBUG] タッチ状態確認");
  uint16_t touchValue = touchManager.getTouchValue();
  uint16_t threshold = touchManager.getTouchThreshold();
  Serial.printf("[DEBUG] タッチ値: %d, 閾値: %d\n", touchValue, threshold);
  
  if (touchValue < threshold) {
    Serial.println("[DEBUG] 起動時にタッチ検出。設定モードで起動します");
    wifiManager.startAPMode();
  } else {
    // WiFi接続を試みる
    Serial.println("[DEBUG] WiFi接続開始");
    if (wifiManager.connectToWiFi()) {
      Serial.println("[DEBUG] WiFi接続成功");
      
      // ネットワーク情報を表示
      Serial.println("[DEBUG] ネットワーク情報表示");
      displayNetworkInfo();
      // displayNetworkInfo後にクロックフェイスを再描画
      // ntpClock.drawClockFace(); // 不要なためコメントアウト
      
      // NTP時計の初期化
      Serial.println("[DEBUG] NTPClock初期化開始");
      ntpClock.begin();
      Serial.println("[DEBUG] NTPClock初期化完了");
      delay(500);
      
      // NTPから時刻を取得
      Serial.println("[DEBUG] NTP同期開始");
      bool syncResult = ntpClock.syncTimeWithNTP();
      Serial.printf("[DEBUG] NTP同期%s\n", syncResult ? "成功" : "失敗");
      delay(500);
      

      // OTAサーバーを設定
      Serial.println("[DEBUG] OTAサーバー設定開始");
      wifiManager.setupOTA();
      Serial.println("[DEBUG] OTAサーバー設定完了");
    } else {
      Serial.println("[DEBUG] WiFi接続失敗");
    }
  }
  
  Serial.println("[DEBUG] setup()完了");
  Serial.printf("[DEBUG] 空きメモリ: %d bytes\n", ESP.getFreeHeap());
}

// メインループ
void loop() {
  static unsigned long lastDebugTime = 0;
  static unsigned long loopCount = 0;
  unsigned long currentTime = millis();
  loopCount++;
  
  // 10秒ごとにデバッグ情報を出力
  if (currentTime - lastDebugTime > 10000) {
    Serial.printf("[DEBUG] loop実行回数: %lu, 空きメモリ: %d bytes\n", 
                  loopCount, ESP.getFreeHeap());
    lastDebugTime = currentTime;
    loopCount = 0;
  }
  
  // ワッチドッグタイマーをリセット
  delay(1); // 他のタスクに実行時間を譲る
  yield();
  
  try {
    // 現在の動作モードに応じた処理
    WiFiManager::OperationMode currentMode = wifiManager.getCurrentMode();
    
    if (currentMode == WiFiManager::MODE_NORMAL) {
      // 通常モード
      
      // タッチセンサーをチェック
      if (touchManager.checkLongTouch()) {
        // 長押し検出、設定モードに切り替え
        Serial.println("[DEBUG] 長押し検出。設定モードに切り替えます");
        wifiManager.startAPMode();
        return;
      }
      
      // 短いタッチでIPアドレスを表示
      static unsigned long lastIPDisplayTime = 0;
      unsigned long currentIPTime = millis();
      
      // 前回のIP表示から最低3秒経過しているか確認（連打防止）
      if (touchManager.checkShortTouch() && (currentIPTime - lastIPDisplayTime > 3000)) {
        Serial.println("[DEBUG] 短いタッチ検出。IPアドレスを表示します");
        lastIPDisplayTime = currentIPTime;
        
        // IPアドレス表示処理を実行
        displayNetworkInfo();
        // IPアドレス表示後にクロックフェイスを再描画
        ntpClock.drawClockFace();
      }

      
      // 初期化済みフラグの設定
      static bool clockInitialized = true; // setup内で初期化済みにする
      
      // 5秒ごとにバックライトを確認
      static unsigned long lastBacklightCheckTime = 0;
      if (currentTime - lastBacklightCheckTime > 5000) { // 5秒ごと
        // バックライトの明るさを再設定
        setBacklightBrightness(backlightBrightness);
        lastBacklightCheckTime = currentTime;
      }
      
      // 時計を更新
      try {
        // WiFi接続状態を確認
        if (wifiManager.isConnected()) {
          // NTP同期済みの時計を更新
          ntpClock.updateClock();
        } else {
          // WiFi未接続時も内部時計で更新
          struct tm timeinfo;
          if (getLocalTime(&timeinfo)) {
            ntpClock.drawClockHands(timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
          }
        }
      } catch (const std::exception& e) {
        Serial.printf("[ERROR] 時計更新エラー: %s\n", e.what());
      }
        
      // 1時間ごとにNTP同期
      static unsigned long lastSyncTime = 0;
      unsigned long currentSyncTime = millis();
      if (wifiManager.isConnected() && (currentSyncTime - lastSyncTime > 3600000)) { // 1時間 = 3600000ms
        Serial.println("[DEBUG] NTP再同期開始");
        bool syncResult = ntpClock.syncTimeWithNTP();
        lastSyncTime = currentSyncTime;
        if (syncResult) {
          Serial.println("[DEBUG] NTP同期成功");
        } else {
          Serial.println("[DEBUG] NTP同期失敗");
        }
      }
      
      // WiFi接続が切れた場合は再接続を試みる
      if (!wifiManager.isConnected()) {
        static unsigned long lastReconnectTime = 0;
        
        if (currentTime < lastReconnectTime) {
          lastReconnectTime = 0;
        }
        
        // 30秒ごとに再接続を試みる
        if (currentTime - lastReconnectTime > 30000) { // 30秒 = 30000ms
          Serial.println("[DEBUG] WiFi再接続を試みます");
          bool connectResult = false;
          try {
            connectResult = wifiManager.connectToWiFi();
          } catch (const std::exception& e) {
            Serial.printf("[ERROR] WiFi接続エラー: %s\n", e.what());
          }
          
          if (connectResult) {
            lastReconnectTime = currentTime;
            Serial.println("[DEBUG] WiFi再接続成功");
          } else {
            Serial.println("[DEBUG] WiFi再接続失敗");
          }
        }
      }
    } else if (currentMode == WiFiManager::MODE_AP_SETUP) {
      // APモード（設定モード）
      // タッチセンサーをチェックして長押しで通常モードに戻る
      if (touchManager.checkLongTouch()) {
        Serial.println("[DEBUG] 長押し検出。通常モードに切り替えます");
        Serial.println("[DEBUG] ESP32を再起動します");
        delay(1000); // シリアル出力が完了するまで待機
        ESP.restart(); // ESP32を再起動して通常モードで起動
      }
    }
  } catch (const std::exception& e) {
    Serial.printf("[ERROR] loop内で例外が発生しました: %s\n", e.what());
  } catch (...) {
    Serial.println("[ERROR] loop内で不明な例外が発生しました");
  }
}