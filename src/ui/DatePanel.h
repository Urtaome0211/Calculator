#pragma once
#include "ModePanel.h"
#include "Keypad.h"
#include "../core/DateCalculator.h"
// ============================================================
// 日期计算：两个日期之差 / 基准日期加减天数（周/月/年）
// ============================================================
class DatePanel : public ModePanel {
public:
  HWND Create(HWND parent, HINSTANCE inst, const AppContext& ctx) override;
  void OnSize(int w, int h) override;
  void OnShow() override;
  int AltLineCount() const override { return 2; }
private:
  LRESULT HandleMessage(UINT msg, WPARAM w, LPARAM l) override;
  bool OnCommand(int id) override;
  void DoCalc();
  void ResetInputs();
  void ApplyMode();
  HWND editFrom_ = nullptr, editTo_ = nullptr;      // 日期差模式
  HWND editBase_ = nullptr, editAmount_ = nullptr;  // 加减模式
  HWND comboUnit_ = nullptr, comboSign_ = nullptr;
  bool modeDiff_ = true;
  Keypad topPad_;
};
