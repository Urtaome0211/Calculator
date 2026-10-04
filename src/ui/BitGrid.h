#pragma once
#include "Common.h"
// ============================================================
// 位显示网格（程序员模式）：64 个可点击翻转的小方块，
// 按 16 位一行排布，下方标注位序号
// ============================================================
class BitGrid {
public:
  HWND Create(HWND parent, int id, const std::function<void()>& onToggle);
  static BitGrid* From(HWND h);
  void SetValue(uint64_t v);
  void SetBits(unsigned bits);             // 64/32/16/8，决定显示列数
  uint64_t Value() const { return value_; }
private:
  static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
  void Paint(HDC dc);
  int HitTest(int x, int y) const;
  void Toggle(int i);
  HWND hwnd_ = nullptr;
  uint64_t value_ = 0;
  unsigned bits_ = 64;
  std::function<void()> onToggle_;
  int hoverIdx_ = -1;
};
