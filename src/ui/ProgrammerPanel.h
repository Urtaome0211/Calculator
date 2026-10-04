#pragma once
#include "ModePanel.h"
#include "Keypad.h"
#include "BitGrid.h"
// ============================================================
// 程序员模式：HEX/DEC/OCT/BIN 进制切换、8~64 位字长、
// 位翻转网格、逻辑/移位运算
// ============================================================
class ProgrammerPanel : public ModePanel {
public:
  HWND Create(HWND parent, HINSTANCE inst, const AppContext& ctx) override;
  void OnSize(int w, int h) override;
  void OnShow() override;
  bool OnChar(wchar_t c) override;
  bool OnKey(WPARAM vk) override;
  void LoadValue(const std::wstring& v) override;
  int AltLineCount() const override { return 4; }
private:
  LRESULT HandleMessage(UINT msg, WPARAM w, LPARAM l) override;
  bool OnCommand(int id) override;
  void Insert(const std::wstring& token);
  void UpdateDisplay();
  void Eval();
  void SetBase(unsigned base);
  void SetBits(unsigned bits);
  void ApplyBaseEnabled();                  // 按进制禁用数字键
  std::wstring expr_;
  unsigned base_ = 16, bits_ = 64;
  uint64_t value_ = 0;
  bool hasValue_ = false, justEval_ = false;
  Keypad topPad_, pad_;
  BitGrid bitGrid_;
};
