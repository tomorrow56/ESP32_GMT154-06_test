#include <Arduino.h>
#include <LovyanGFX.hpp>
#include <Avatar.h>
#include <faces/FaceTemplates.hpp>

// バックライト設定
#define BACKLIGHT_CHANNEL 0
#define BACKLIGHT_FREQ 5000
#define BACKLIGHT_RESOLUTION 8
#define BACKLIGHT_MAX 255
#define TFT_BACKLIGHT_ON LOW  // バックライトの極性（LOWがON）

// ディスプレイ設定用クラス
class LGFX : public lgfx::LGFX_Device {
private:
  lgfx::Panel_ST7789 _panel_instance;
  lgfx::Bus_SPI _bus_instance;
  lgfx::Light_PWM _light_instance;

public:
  LGFX(void) {
    // SPIバス設定
    {
      auto cfg = _bus_instance.config();
      cfg.spi_host = VSPI_HOST;  // VSPI_HOST or HSPI_HOST
      cfg.spi_mode = 3;          // SPI mode 3 for ST7789
      cfg.freq_write = 27000000; // SPI clock frequency
      cfg.pin_sclk = 18;         // CLK pin (IO18)
      cfg.pin_mosi = 23;         // DATA pin (IO23)
      cfg.pin_miso = -1;         // Not used
      cfg.pin_dc = 2;            // D/C pin (IO2)
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }

    // パネル設定
    {
      auto cfg = _panel_instance.config();
      cfg.pin_cs = -1;           // CS pin (not used)
      cfg.pin_rst = 4;           // RESET pin (IO4)
      cfg.pin_busy = -1;         // BUSY pin (not used)
      cfg.panel_width = 240;     // 実際のディスプレイ幅
      cfg.panel_height = 240;    // 実際のディスプレイ高さ
      cfg.offset_x = 0;          // パネルのX方向オフセット
      cfg.offset_y = 0;          // パネルのY方向オフセット
      cfg.offset_rotation = 0;   // 回転方向のオフセット
      cfg.dummy_read_pixel = 8;  // ダミーリードのピクセル数
      cfg.dummy_read_bits = 1;   // ダミーリードのビット数
      cfg.readable = false;      // 読み取り可能フラグ
      cfg.invert = true;         // パネル色反転
      cfg.rgb_order = false;     // RGB/BGR順序 (false = BGR)
      cfg.dlen_16bit = false;    // 16ビットデータ長モード
      cfg.bus_shared = true;     // SPIバス共有設定

      _panel_instance.config(cfg);
    }

    // バックライト設定
    {
      auto cfg = _light_instance.config();
      cfg.pin_bl = 25;           // バックライトピン (IO25)
      cfg.invert = false;        // バックライト極性反転
      cfg.freq = BACKLIGHT_FREQ; // PWM周波数
      cfg.pwm_channel = BACKLIGHT_CHANNEL; // PWMチャンネル

      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);
    }

    setPanel(&_panel_instance);
  }
};

// ディスプレイインスタンス
LGFX lcd;

// M5Stackクラスの代わりにLovyanGFXを使用するためのクラス
class M5LovyanGFX {
public:
  LGFX& Display;

  M5LovyanGFX(LGFX& lcd) : Display(lcd) {}

  void begin() {
    Display.init();
    Display.setRotation(0);
    Display.setBrightness(BACKLIGHT_MAX / 2); // 50%の輝度
    Display.fillScreen(TFT_BLACK);
  }

  void update() {
    // 必要に応じて更新処理を実装
  }
};

// M5LovyanGFXインスタンス
M5LovyanGFX M5(lcd);

// Avatarインスタンス
using namespace m5avatar;
Avatar avatar;

// 表情変更用変数
int expressionIndex = 0;  // デフォルトはNeutral
const Expression expressions[] = {
  Expression::Neutral,
  Expression::Happy,
  Expression::Sleepy,
  Expression::Sad,
  Expression::Doubt
};
const char* expressionNames[] = {
  "Neutral",
  "Happy",
  "Sleepy",
  "Sad",
  "Doubt"
};
const int expressionCount = sizeof(expressions) / sizeof(expressions[0]);

// タッチ入力用変数
const int TOUCH_PIN = 32;  // IO32
const int TOUCH_THRESHOLD = 83;  // タッチ検出のしきい値（小さいほど感度が高い）
const unsigned long TOUCH_DURATION = 300;  // 0.3秒長押し
unsigned long touchStartTime = 0;
bool touchDetected = false;
bool expressionChanged = false;

// 顔の種類変更用
Face* faces[5];
int faceIndex = 0;
const char* faceNames[] = {
  "Default",
  "Doggy",
  "Omega",
  "Girly",
  "PinkDemon"
};
const int faceCount = sizeof(faces) / sizeof(Face*);

// 色パレット変更用
ColorPalette* colorPalettes[3];
int paletteIndex = 0;
const char* paletteNames[] = {
  "Default",
  "Pastel",
  "Vivid"
};
const int paletteCount = sizeof(colorPalettes) / sizeof(ColorPalette*);

// 情報表示用タイマー
unsigned long infoTimer = 0;
const unsigned long infoInterval = 1000; // 1秒ごとに情報更新

// 顔切り替え用タイマー
unsigned long faceTimer = 0;
const unsigned long faceInterval = 15000; // 15秒ごとに顔切り替え

// 色切り替え用タイマー
unsigned long paletteTimer = 0;
const unsigned long paletteInterval = 30000; // 30秒ごとに色切り替え

void setup() {
  // シリアル通信初期化
  Serial.begin(115200);
  Serial.println("ESP32 GMT154-06 Avatar");

  // タッチセンサーの初期化 (IO32)
  Serial.println("Touch sensor initialized: IO32");
  Serial.print("Touch threshold: ");
  Serial.println(TOUCH_THRESHOLD);
  
  // タッチセンサーの設定
  touchSetCycles(0x1000, 0x1000);  // タッチセンサーの感度を調整

  // LCDの初期化
  M5.begin();
  
  // 顔の種類を初期化
  faces[0] = avatar.getFace();  // デフォルトの顔
  faces[1] = new DoggyFace();
  faces[2] = new OmegaFace();
  faces[3] = new GirlyFace();
  faces[4] = new PinkDemonFace();
  
  // 色パレットを初期化
  colorPalettes[0] = new ColorPalette();  // デフォルトの色
  colorPalettes[1] = new ColorPalette();
  colorPalettes[2] = new ColorPalette();
  
  // パステルカラーパレット
  colorPalettes[1]->set(COLOR_PRIMARY, lcd.color24to16(0x383838));  // 目
  colorPalettes[1]->set(COLOR_BACKGROUND, lcd.color24to16(0xfac2a8));  // 肩
  colorPalettes[1]->set(COLOR_SECONDARY, TFT_PINK);  // 頬
  
  // ビビッドカラーパレット
  colorPalettes[2]->set(COLOR_PRIMARY, TFT_YELLOW);
  colorPalettes[2]->set(COLOR_BACKGROUND, TFT_DARKCYAN);
  
  // Avatarの初期化
  avatar.init(8);  // 8bitカラーモードで描画開始
  
  // ディスプレイサイズに合わせてアバターの位置を調整
  // M5Stackは320x240、GMT154-06は240x240なので中央に配置する
  avatar.setPosition(0, -40);  // 左に40ピクセル移動して中央に表示
  
  // アバターのサイズを80%に縮小
  avatar.setScale(0.8);
  
  // 表情を設定
  avatar.setExpression(expressions[expressionIndex]);  // Happyがデフォルト
  
  // 初期顔を設定
  avatar.setFace(faces[0]);
  
  // 初期色パレットを設定
  avatar.setColorPalette(*colorPalettes[0]);
  
  // 口の動きをランダムに設定
  avatar.setMouthOpenRatio(0.0);
  
  // 情報表示
  Serial.println("Avatar initialized");
  Serial.println("Pin Configuration:");
  Serial.println("RESET:IO4 D/C:IO2 DATA:IO23 CLK:IO18 BL:IO25");
}

void loop() {
  // 現在の時間を取得
  unsigned long currentMillis = millis();
  
  // タッチ入力処理 (IO32)
  int touchValue = touchRead(TOUCH_PIN);
  
  // デバッグ用にタッチ値を定期的に表示
  static unsigned long lastTouchDebugTime = 0;
  if (currentMillis - lastTouchDebugTime >= 500) { // 0.5秒ごとに表示
    lastTouchDebugTime = currentMillis;
    Serial.print("Touch value (IO32): ");
    Serial.println(touchValue);
  }
  
  // タッチが検出されたとき
  if (touchValue < TOUCH_THRESHOLD && !touchDetected) {
    touchDetected = true;
    touchStartTime = currentMillis;
    expressionChanged = false;
    Serial.println("Touch detected!");
  }
  
  // タッチが1秒以上続いているかチェック
  if (touchDetected && !expressionChanged && (currentMillis - touchStartTime >= TOUCH_DURATION)) {
    // 次の表情に変更
    expressionIndex = (expressionIndex + 1) % expressionCount;
    avatar.setExpression(expressions[expressionIndex]);
    
    // 情報表示
    Serial.print("Expression changed to: ");
    Serial.println(expressionNames[expressionIndex]);
    expressionChanged = true;  // 表情変更済みフラグ
  }
  
  // タッチが離れたとき
  if (touchValue >= TOUCH_THRESHOLD && touchDetected) {
    touchDetected = false;
    Serial.println("Touch released");
  }
  
  // 顔の種類をDefaultに固定
  if (currentMillis - faceTimer >= faceInterval) {
    faceTimer = currentMillis;
    
    // Default顔に固定
    faceIndex = 0; // Default顔のインデックス
    avatar.setFace(faces[faceIndex]);
    
    // 情報表示
    Serial.print("Face: ");
    Serial.println(faceNames[faceIndex]);
  }
  
  // 色パレットをデフォルトに固定
  if (currentMillis - paletteTimer >= paletteInterval) {
    paletteTimer = currentMillis;
    
    // デフォルトの色パレットに固定
    paletteIndex = 0; // デフォルトパレットのインデックス
    avatar.setColorPalette(*colorPalettes[paletteIndex]);
    
    // 情報表示
    Serial.print("Color Palette: ");
    Serial.println(paletteNames[paletteIndex]);
  }
  
  // リップシンクシミュレーション
  if (currentMillis - infoTimer >= infoInterval) {
    infoTimer = currentMillis;
    
    // ランダムな口の開き具合を設定（1/3に減らす）
    float mouthOpenRatio = (float)random(0, 33) / 100.0; // 0～0.33の範囲に制限
    avatar.setMouthOpenRatio(mouthOpenRatio);
    
    // 情報表示
    Serial.print("Mouth: ");
    Serial.println(mouthOpenRatio);
  }
  
  // 適度な遅延
  delay(50);
}
