#pragma once
#include "Common.h"
#include "Theme.h"
// ============================================================
// 自定义按钮控件：Win11 圆角按键、悬停/按下动画、切换高亮
// ============================================================
class CtrlButton {
public:
  static CtrlButton* Create(HWND parent, const std::wstring& label, int id,
                            Theme::Kind kind, bool small = false);
  static CtrlButton* From(HWND h);
  HWND Hwnd() const { return hwnd_; }
  void SetLabel(const std::wstring& label);
  void SetChecked(bool checked);
  bool Checked() const { return checked_; }
  void SetEnabled(bool enabled);
  void SetKind(Theme::Kind k);
private:
  static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
  void Paint(HDC dc);
  HWND hwnd_ = nullptr;
  std::wstring label_;
  int id_ = 0;
  Theme::Kind kind_ = Theme::Kind::Number;
  bool checked_ = false, hover_ = false, down_ = false, small_ = false;
};
