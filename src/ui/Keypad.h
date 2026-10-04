#pragma once
#include "Common.h"
#include "CtrlButton.h"
// ============================================================
// 键盘面板：按行列规格批量创建按钮，随窗口尺寸自动铺满布局
// ============================================================
struct KeyCell {
  std::wstring label;
  int id = 0;                              // 0 表示空白占位
  Theme::Kind kind = Theme::Kind::Number;
  int colSpan = 1;
  int rowSpan = 1;
  bool small = false;
};
struct KeyRow { std::vector<KeyCell> cells; };

class Keypad {
public:
  void Build(HWND parent, const std::vector<KeyRow>& rows, int columns, int hGap, int vGap);
  void Layout(const RECT& rc);             // 各行等高铺满
  CtrlButton* Get(int id);
  void SetChecked(int id, bool on);
private:
  struct Item { int id; HWND h; int row, col, colSpan, rowSpan; };
  std::vector<Item> items_;
  int columns_ = 1, hGap_ = 4, vGap_ = 4, rows_ = 1;
  HWND parent_ = nullptr;
};
