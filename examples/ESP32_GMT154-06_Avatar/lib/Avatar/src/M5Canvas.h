#ifndef M5CANVAS_H_
#define M5CANVAS_H_

#include <LovyanGFX.hpp>

// main.cppで定義されているカスタムLGFXクラスの前方宣言
// 重複定義を避けるために前方宣言のみを行う
class LGFX;

// 外部で定義されたディスプレイインスタンスを参照
extern LGFX lcd;

// M5GFXのM5CanvasをLovyanGFXのLGFXSpriteに置き換えるための定義
typedef lgfx::LGFX_Sprite M5Canvas;

#endif // M5CANVAS_H_
