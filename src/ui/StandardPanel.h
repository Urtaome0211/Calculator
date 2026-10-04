#pragma once
#include "KeypadPanel.h"
// ============================================================
// 标准模式：4×6 数字键盘 + 百分号/倒数/平方/开方 + 记忆条
// ============================================================
class StandardPanel : public KeypadPanel {
protected:
  void BuildLayout(Keypad& pad) override;
};
