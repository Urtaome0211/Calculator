#pragma once
#include "ModePanel.h"
#include "Keypad.h"
#include "../core/ExpressionEngine.h"
// ============================================================
// 键盘计算面板基类（标准 / 科学模式共用）：
// 记忆条 + 主键盘 + 表达式状态机 + 实时求值显示
// ============================================================
class KeypadPanel : public ModePanel {
public:
  HWND Create(HWND parent, HINSTANCE inst, const AppContext& ctx) override;
  void OnSize(int w, int h) override;
  void OnShow() override;
  bool OnChar(wchar_t c) override;
  bool OnKey(WPARAM vk) override;
  void LoadValue(const std::wstring& v) override;
protected:
  virtual void BuildLayout(Keypad& pad) = 0;                   // 子类提供主键盘规格
  virtual std::vector<KeyCell> MemoryRow() const;              // 记忆条（可扩展 DEG/RAD/GRAD）
  virtual bool OnCommand(int id) override;                     // 子类扩展后回调本实现
  virtual Expr::AngleMode Angle() const { return Expr::AngleMode::Deg; }
  // ---- 工具 ----
  void Insert(const std::wstring& token);                      // 插入 token 并刷新
  void UpdateDisplay();                                        // 表达式 + 实时结果
  bool EvalNow(double& v);                                     // 当前表达式求值
  void PushHistory();                                          // expr_ 与当前结果入历史
  void SetResultText(double v);
  void PressDigit(wchar_t c);
  void PressOp(const std::wstring& sym);                       // "+" "−" "×" "÷"
  void PressEquals();
  void ApplyUnary(const std::wstring& prefix, const std::wstring& suffix);
  std::wstring expr_;                                          // 原始表达式（无千分位）
  bool justEval_ = false;
  double lastResult_ = 0.0;
  Keypad pad_, memPad_;
  bool second_ = false;
private:
  LRESULT HandleMessage(UINT msg, WPARAM w, LPARAM l) override;
  void Layout();
  int padTop_ = 0;
};
