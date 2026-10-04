#pragma once
#include "KeypadPanel.h"
// ============================================================
// 科学模式：8 列键盘 + 三角函数侧栏 + 2nd 功能切换 + 角度制切换
// ============================================================
class ScientificPanel : public KeypadPanel {
public:
  void OnShow() override;
  bool OnChar(wchar_t c) override;   // 支持 p/e/^/! 快捷输入
protected:
  void BuildLayout(Keypad& pad) override;
  std::vector<KeyCell> MemoryRow() const override;
  Expr::AngleMode Angle() const override { return angle_; }
  bool OnCommand(int id) override;
private:
  void ApplySecond();                       // 刷新 2nd 相关按钮文字与行为
  void PressFunc(const std::wstring& name); // 插入 "name("
  Expr::AngleMode angle_ = Expr::AngleMode::Deg;
};
