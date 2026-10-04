#pragma once
#include "Common.h"
// ============================================================
// 显示区：表达式行 + 大字号结果行 + 若干次要信息行（如程序员模式
// 的四种进制、日期模式的结果明细），带 M 记忆徽标
// ============================================================
class DisplayPanel {
public:
  HWND Create(HWND parent, int id);
  static DisplayPanel* From(HWND h);
  void SetExpr(const std::wstring& s);
  void SetResult(const std::wstring& s, bool error = false);
  void SetAltLines(const std::vector<std::wstring>& lines);
  void SetMemory(bool on);
  int Height() const;                      // 按次要行数与 DPI 计算所需高度
  void ClearAll();
private:
  static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
  void Paint(HDC dc);
  std::wstring expr_, result_;
  std::vector<std::wstring> alt_;
  bool error_ = false, mem_ = false;
  HWND hwnd_ = nullptr;
};
