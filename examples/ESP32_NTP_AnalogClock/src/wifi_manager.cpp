#include "wifi_manager.h"
#include <AsyncElegantOTA.h>

// コンストラクタ
WiFiManager::WiFiManager(const char* apSSID, const char* apPassword, const IPAddress& apIP)
    : apSSID(apSSID), apPassword(apPassword), apIP(apIP), currentMode(MODE_NORMAL), isConfigured(false) {
    // デフォルト値の設定
    ssid = "";
    password = "";
    otaUsername = "admin";
    otaPassword = "admin";
    
    // Webサーバーの初期化
    server = new AsyncWebServer(80);
}

// 初期化
void WiFiManager::begin() {
    // 設定を読み込む
    loadSettings();
    
    // WiFiモードを初期化
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
}

// 設定を読み込む
void WiFiManager::loadSettings() {
    preferences.begin("clock", false);
    ssid = preferences.getString("ssid", "");
    password = preferences.getString("password", "");
    otaUsername = preferences.getString("otaUser", "admin");
    otaPassword = preferences.getString("otaPwd", "admin");
    isConfigured = preferences.getBool("configured", false);
    preferences.end();
    
    Serial.println("設定読み込み完了");
    Serial.printf("SSID: %s\n", ssid.c_str());
    Serial.printf("設定済み: %s\n", isConfigured ? "はい" : "いいえ");
}

// APモードを開始
void WiFiManager::startAPMode() {
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(apSSID, apPassword);
    
    Serial.println("APモードを開始しました");
    Serial.printf("SSID: %s\n", apSSID);
    Serial.printf("Password: %s\n", apPassword);
    Serial.printf("IPアドレス: %s\n", apIP.toString().c_str());
    
    // Webサーバーの設定
    setupWebServer();
    currentMode = MODE_AP_SETUP;
}

// WiFiに接続
bool WiFiManager::connectToWiFi() {
    if (ssid.length() == 0) {
        Serial.println("WiFi設定がありません。APモードを開始します");
        startAPMode();
        return false;
    }
    
    Serial.println("WiFiに接続中...");
    
    // WiFiモードをステーションモードに設定
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    
    // 設定されたSSIDに接続
    Serial.printf("WiFiネットワーク %s に接続を試みます...\n", ssid.c_str());
    
    WiFi.begin(ssid.c_str(), password.c_str());
    
    int attempt = 0;
    while (WiFi.status() != WL_CONNECTED && attempt < 20) {
        delay(500);
        Serial.print(".");
        attempt++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi接続成功!");
        Serial.printf("接続先SSID: %s\n", WiFi.SSID().c_str());
        Serial.printf("IPアドレス: %s\n", WiFi.localIP().toString().c_str());
        
        currentMode = MODE_NORMAL;
        return true;
    } else {
        Serial.println("\nWiFi接続失敗");
        
        delay(2000);
        startAPMode();
        return false;
    }
}

// OTAサーバーを初期化
void WiFiManager::setupOTA() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi未接続のためOTAサーバーを開始できません");
        return;
    }
    
    // OTA更新ページの設定
    AsyncElegantOTA.begin(server, otaUsername.c_str(), otaPassword.c_str());
    
    // 通常モードではルートページにステータス情報を表示
    server->on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        String html = MAIN_HTML;
        html.replace("%SSID%", WiFi.SSID());
        html.replace("%IP%", WiFi.localIP().toString());
        request->send(200, "text/html", html);
    });
    
    // 設定ページを追加
    server->on("/setup", HTTP_GET, [this](AsyncWebServerRequest *request) {
        Serial.println("[DEBUG] /setup エンドポイントにアクセスされました");
        
        // 直接置換処理を使用してHTMLを生成
        String html = SETUP_HTML;
        html.replace("%SSID%", ssid);
        html.replace("%PASSWORD%", password);
        html.replace("%OTA_USERNAME%", otaUsername);
        html.replace("%OTA_PASSWORD%", otaPassword);
        
        // 生成したHTMLを送信
        request->send(200, "text/html", html);
    });
    
    // 再起動ページ
    server->on("/restart", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", RESTART_HTML);
        delay(1000);
        ESP.restart();
    });
    
    // サーバーを開始
    server->begin();
    Serial.println("OTAサーバー開始");
}

// HTMLテンプレート処理関数
String WiFiManager::processTemplate(const String& var) {
    Serial.printf("[DEBUG] テンプレート変数を処理: %s\n", var.c_str());
    
    String result;
    
    if (var == "SSID") {
        result = ssid;
    } else if (var == "PASSWORD") {
        result = password;
    } else if (var == "OTA_USERNAME") {
        result = otaUsername;
    } else if (var == "OTA_PASSWORD") {
        result = otaPassword;
    } else {
        result = String();
    }
    
    Serial.printf("[DEBUG] テンプレート結果: %s = %s\n", var.c_str(), result.c_str());
    return result;
}

// Webサーバーを設定
void WiFiManager::setupWebServer() {
    // CORSヘッダーを設定
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");
    
    // NOT FOUNDハンドラを設定
    server->onNotFound([](AsyncWebServerRequest *request) {
        Serial.printf("[DEBUG] NOT FOUND: %s\n", request->url().c_str());
        request->send(404, "text/plain", "Not found");
    });
    
    // ルートページ
    server->on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        Serial.println("[DEBUG] ルートページにアクセスされました");
        
        // 最適化されたテンプレート処理でSETUP_HTMLを表示
        String html = SETUP_HTML;
        html.replace("%SSID%", ssid);
        html.replace("%PASSWORD%", password);
        html.replace("%OTA_USERNAME%", otaUsername);
        html.replace("%OTA_PASSWORD%", otaPassword);
        
        request->send(200, "text/html", html);
    });
    
    // テンプレート処理ありのルートページ
    server->on("/setup-template", HTTP_GET, [this](AsyncWebServerRequest *request) {
        Serial.println("[DEBUG] テンプレート処理ありのルートページにアクセスされました");
        
        // 事前にテンプレート変数を処理してHTMLを生成
        String html = SETUP_HTML;
        html.replace("%SSID%", ssid);
        html.replace("%PASSWORD%", password);
        html.replace("%OTA_USERNAME%", otaUsername);
        html.replace("%OTA_PASSWORD%", otaPassword);
        
        // 生成したHTMLを送信
        request->send(200, "text/html", html);
    });
    
    // シンプルなテンプレートページ
    server->on("/simple-template", HTTP_GET, [this](AsyncWebServerRequest *request) {
        Serial.println("[DEBUG] シンプルなテンプレートページにアクセスされました");
        
        // シンプルなHTMLを生成
        String html = "<html><body><h1>ESP32 Settings</h1>";
        html += "<p>SSID: " + ssid + "</p>";
        html += "<p>OTA Username: " + otaUsername + "</p>";
        html += "<p><a href='/'>Back to main page</a></p>";
        html += "</body></html>";
        
        request->send(200, "text/html", html);
    });
    
    // テストページ
    server->on("/test", HTTP_GET, [](AsyncWebServerRequest *request) {
        Serial.println("[DEBUG] テストページにアクセスされました");
        request->send_P(200, "text/html", TEST_HTML);
    });
    
    // プレーンテキストテストページ
    server->on("/plaintest", HTTP_GET, [](AsyncWebServerRequest *request) {
        Serial.println("[DEBUG] プレーンテキストテストページにアクセスされました");
        request->send(200, "text/plain", "ESP32 Web Server is working!");
    });
    
    // WiFi設定の保存
    server->on("/save-wifi", HTTP_POST, [](AsyncWebServerRequest *request) {
        Serial.println("[DEBUG] WiFi設定の保存リクエストを受信しました");
        request->send(200, "text/plain", "リクエストを受信しました");
    }, NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        String json = String((char*)data, len);
        Serial.printf("[DEBUG] 受信したJSON: %s\n", json.c_str());
        JSONVar jsonObj = JSON.parse(json);
        
        if (JSON.typeof(jsonObj) == "object") {
            if (jsonObj.hasOwnProperty("ssid") && jsonObj.hasOwnProperty("password")) {
                String newSSID = (const char*)jsonObj["ssid"];
                String newPassword = (const char*)jsonObj["password"];
                
                saveWiFiSettings(newSSID, newPassword);
            }
        }
    });
    
    // OTA設定の保存
    server->on("/save-ota", HTTP_POST, [](AsyncWebServerRequest *request) {
        Serial.println("[DEBUG] OTA設定の保存リクエストを受信しました");
        request->send(200, "text/plain", "リクエストを受信しました");
    }, NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        String json = String((char*)data, len);
        Serial.printf("[DEBUG] 受信したJSON: %s\n", json.c_str());
        JSONVar jsonObj = JSON.parse(json);
        
        if (JSON.typeof(jsonObj) == "object") {
            if (jsonObj.hasOwnProperty("otaUsername") && jsonObj.hasOwnProperty("otaPassword")) {
                String newOtaUser = (const char*)jsonObj["otaUsername"];
                String newOtaPwd = (const char*)jsonObj["otaPassword"];
                
                saveOTASettings(newOtaUser, newOtaPwd);
            }
        }
    });
    
    // 再起動
    server->on("/restart", HTTP_GET, [](AsyncWebServerRequest *request) {
        Serial.println("[DEBUG] 再起動リクエストを受信しました");
        request->send_P(200, "text/html", RESTART_HTML);
        delay(1000);
        ESP.restart();
    });
    
    // サーバーを開始
    server->begin();
    Serial.println("Webサーバーを開始しました");
    Serial.println("  - ルートページ: http://192.168.4.1/");
    Serial.println("  - テストページ: http://192.168.4.1/test");
}

// WiFi設定を保存
void WiFiManager::saveWiFiSettings(const String& newSSID, const String& newPassword) {
    preferences.begin("clock", false);
    preferences.putString("ssid", newSSID);
    preferences.putString("password", newPassword);
    preferences.putBool("configured", true);
    preferences.end();
    
    ssid = newSSID;
    password = newPassword;
    isConfigured = true;
    
    Serial.println("WiFi設定を保存しました");
    Serial.printf("SSID: %s\n", ssid.c_str());
}

// OTA設定を保存
void WiFiManager::saveOTASettings(const String& newUsername, const String& newPassword) {
    preferences.begin("clock", false);
    preferences.putString("otaUser", newUsername);
    preferences.putString("otaPwd", newPassword);
    preferences.end();
    
    otaUsername = newUsername;
    otaPassword = newPassword;
    
    Serial.println("OTA設定を保存しました");
}
