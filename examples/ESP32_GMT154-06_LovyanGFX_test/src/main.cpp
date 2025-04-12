#include <Arduino.h>
#include <LovyanGFX.hpp>

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

void setup() {
  // シリアル通信初期化
  Serial.begin(115200);
  Serial.println("ESP32 LCD Hello World with LovyanGFX");

  // LCDの初期化
  lcd.init();
  
  // 画面の向きを設定（0, 1, 2, 3）
  lcd.setRotation(0);
  
  // 画面を青色で塗りつぶす
  lcd.fillScreen(TFT_BLUE);
  
  // テキスト設定
  lcd.setTextColor(TFT_WHITE, TFT_BLUE);
  lcd.setTextSize(1);
  
  // Hello Worldメッセージを表示
  lcd.setCursor(10, 8);
  lcd.setTextFont(4);
  lcd.println("Hello World!");

  // バージョン情報表示
  lcd.setCursor(10, 35);
  lcd.setTextFont(2);
  lcd.println("LovyanGFX v" + String(LGFX_VERSION_MAJOR) + "." + 
             String(LGFX_VERSION_MINOR) + "." + String(LGFX_VERSION_PATCH));
  
  // 接続情報表示
  lcd.setCursor(10, 55);
  lcd.println("GMT154-06 LCD Panel");
  lcd.setCursor(10, 70);
  lcd.println("ST7789 Driver");

  // バックライト情報表示
  lcd.fillRect(10, 90, 220, 30, TFT_BLUE);
  lcd.setCursor(10, 95);
  lcd.println("Backlight: Fixed ON");
  lcd.setCursor(10, 110);
  lcd.println("Brightness: 50%");
  
  // ピン情報表示
  lcd.fillRect(10, 130, 220, 50, TFT_BLUE);
  lcd.setCursor(10, 135);
  lcd.println("Pin Configuration:");
  lcd.setCursor(10, 150);
  lcd.println("RESET:IO4 D/C:IO2");
  lcd.setCursor(10, 165);
  lcd.println("DATA:IO23 CLK:IO18 BL:IO25");
  
  // バックライトを50%の輝度に設定
  lcd.setBrightness(BACKLIGHT_MAX / 2);
  
  Serial.println("Setup complete");
}

void loop() {
  // メインループでは何もしない
  delay(1000);
}
