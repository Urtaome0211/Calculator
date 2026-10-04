#include "StandardPanel.h"

// ============================================================
// 标准模式：4×6 数字键盘（记忆条由基类提供）
// ============================================================
void StandardPanel::BuildLayout(Keypad& pad) {
  using K = Theme::Kind;
  std::vector<KeyRow> rows = {
    {{{L"%", IDK_PERCENT, K::Function}, {L"CE", IDK_CE, K::Function},
      {L"C", IDK_CLEAR, K::Function}, {L"\uE94F", IDK_BACK, K::Function}}},
    {{{L"1/x", IDK_RECIP, K::Function}, {L"x²", IDK_SQ, K::Function},
      {L"√x", IDK_SQRT, K::Function}, {L"÷", IDK_DIV, K::Function}}},
    {{{L"7", IDK_7, K::Number}, {L"8", IDK_8, K::Number},
      {L"9", IDK_9, K::Number}, {L"×", IDK_MUL, K::Function}}},
    {{{L"4", IDK_4, K::Number}, {L"5", IDK_5, K::Number},
      {L"6", IDK_6, K::Number}, {L"−", IDK_SUB, K::Function}}},
    {{{L"1", IDK_1, K::Number}, {L"2", IDK_2, K::Number},
      {L"3", IDK_3, K::Number}, {L"+", IDK_ADD, K::Function}}},
    {{{L"±", IDK_SIGN, K::Number}, {L"0", IDK_0, K::Number},
      {L".", IDK_DOT, K::Number}, {L"=", IDK_EQU, K::Accent}}},
  };
  pad.Build(hwnd_, rows, 4, Theme::S(4), Theme::S(4));
}
