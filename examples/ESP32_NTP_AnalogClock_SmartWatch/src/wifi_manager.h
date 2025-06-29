#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Arduino_JSON.h>
#include "html_content.h"

// AsyncElegantOTAの前方宣言
// 実際のインクルードはwifi_manager.cppで行う
class AsyncElegantOtaClass;
extern AsyncElegantOtaClass AsyncElegantOTA;

class WiFiManager {
public:
    // 動作モード
    enum OperationMode {
        MODE_NORMAL,  // 通常モード（WiFi接続、時計表示）
        MODE_AP_SETUP // 設定モード（APモードでWeb設定ページ提供）
    };
    
private:
    // WiFi設定
    String ssid;
    String password;
    String otaUsername;
    String otaPassword;
    bool isConfigured;
    
    // 設定保存用
    Preferences preferences;
    
    // APモード設定
    const char* apSSID;
    const char* apPassword;
    IPAddress apIP;
    
    OperationMode currentMode;
    
    // Webサーバー
    AsyncWebServer* server;
    
    // HTMLテンプレート処理関数
    String processTemplate(const String& var);
    
public:
    // コンストラクタ
    WiFiManager(const char* apSSID = "ESP32-Clock-Setup", 
                const char* apPassword = "12345678",
                const IPAddress& apIP = IPAddress(192, 168, 4, 1));
    
    // 初期化
    void begin();
    
    // 設定を読み込む
    void loadSettings();
    
    // APモードを開始
    void startAPMode();
    
    // WiFiに接続
    bool connectToWiFi();
    
    // OTAサーバーを初期化
    void setupOTA();
    
    // Webサーバーを設定
    void setupWebServer();
    
    // 現在のモードを取得
    OperationMode getCurrentMode() { return currentMode; }
    
    // WiFi接続状態を取得
    bool isConnected() { return WiFi.status() == WL_CONNECTED; }
    
    // 設定済みかどうかを取得
    bool isWiFiConfigured() { return isConfigured; }
    
    // 設定を保存
    void saveWiFiSettings(const String& newSSID, const String& newPassword);
    void saveOTASettings(const String& newUsername, const String& newPassword);
    
    // 設定値を取得
    String getSSID() { return ssid; }
    String getOTAUsername() { return otaUsername; }
    String getOTAPassword() { return otaPassword; }
    
    // APモードのIPアドレスを取得
    IPAddress getAPIP() { return apIP; }
};

#endif // WIFI_MANAGER_H
