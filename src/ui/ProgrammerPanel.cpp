#include "ProgrammerPanel.h"
#include "Theme.h"
#include "DisplayPanel.h"
#include "../core/IntegerEngine.h"
#include "../core/NumberFormat.h"

// ============================================================
// 程序员模式：HEX/DEC/OCT/BIN + 字长 + 位网格 + 位运算键盘
// ============================================================
HWND ProgrammerPanel::Create(HWND parent, HINSTANCE inst, const AppContext& ctx) {
  ctx_ = ctx;
  hwnd_ = CreateWindowExW(0, ModePanel::ClassName(), L"programmer", WS_CHILD | WS_VISIBLE,
                          0, 0, 10, 10, parent, nullptr, inst, this);

  using K = Theme::Kind;
  // 顶行：进制 + 字长
  std::vector<KeyRow> topRows = {
    {{{L"HEX", IDK_PR_BASE_HEX, K::Toggle, 1, 1, true},
      {L"DEC", IDK_PR_BASE_DEC, K::Toggle, 1, 1, true},
      {L"OCT", IDK_PR_BASE_OCT, K::Toggle, 1, 1, true},
      {L"BIN", IDK_PR_BASE_BIN, K::Toggle, 1, 1, true},
      {L"QWORD", IDK_PR_WS_QWORD, K::Toggle, 1, 1, true},
      {L"DWORD", IDK_PR_WS_DWORD, K::Toggle, 1, 1, true},
      {L"WORD", IDK_PR_WS_WORD, K::Toggle, 1, 1, true},
      {L"BYTE", IDK_PR_WS_BYTE, K::Toggle, 1, 1, true}}},
  };
  topPad_.Build(hwnd_, topRows, 8, Theme::S(4), Theme::S(4));

  // 位网格
  bitGrid_.Create(hwnd_, 0x3200, [this]() {
    // 位翻转后同步表达式与显示
    value_ = bitGrid_.Value();
    hasValue_ = true;
    expr_ = NumFmt::FormatInteger(value_, base_);
    // 去掉分组空格，作为表达式数字
    std::wstring clean;
    for (wchar_t c : expr_) if (c != L' ' && c != L',') clean += c;
    expr_ = clean;
    justEval_ = false;
    UpdateDisplay();
  });

  // 主键盘 6×7
  std::vector<KeyRow> rows = {
    {{{L"(", IDK_PR_LPAR, K::Function}, {L")", IDK_PR_RPAR, K::Function},
      {L"%", IDK_PR_MOD, K::Function}, {L"\uE94F", IDK_BACK, K::Function},
      {L"CE", IDK_CE, K::Function}, {L"C", IDK_CLEAR, K::Function}}},
    {{{L"A", IDK_PR_A, K::Number}, {L"<<", IDK_PR_LSH, K::Function},
      {L">>", IDK_PR_RSH, K::Function}, {L"mod", IDK_PR_MOD, K::Function},
      {L"÷", IDK_DIV, K::Function}}},
    {{{L"B", IDK_PR_B, K::Number}, {L"7", IDK_7, K::Number},
      {L"8", IDK_8, K::Number}, {L"9", IDK_9, K::Number},
      {L"×", IDK_MUL, K::Function}}},
    {{{L"C", IDK_PR_C, K::Number}, {L"4", IDK_4, K::Number},
      {L"5", IDK_5, K::Number}, {L"6", IDK_6, K::Number},
      {L"−", IDK_SUB, K::Function}}},
    {{{L"D", IDK_PR_D, K::Number}, {L"1", IDK_1, K::Number},
      {L"2", IDK_2, K::Number}, {L"3", IDK_3, K::Number},
      {L"+", IDK_ADD, K::Function}}},
    {{{L"E", IDK_PR_E, K::Number}, {L"~", IDK_PR_NOT, K::Function},
      {L"±", IDK_SIGN, K::Function}, {L"0", IDK_0, K::Number},
      {L"=", IDK_EQU, K::Accent}}},
    {{{L"F", IDK_PR_F, K::Number}, {L"AND", IDK_PR_AND, K::Function},
      {L"OR", IDK_PR_OR, K::Function}, {L"XOR", IDK_PR_XOR, K::Function},
      {L"NOT", IDK_PR_NOT, K::Function}, {L"NAND", IDK_PR_NAND, K::Function}}},
  };
  pad_.Build(hwnd_, rows, 6, Theme::S(4), Theme::S(4));

  SetBase(16);
  SetBits(64);
  return hwnd_;
}

void ProgrammerPanel::OnSize(int w, int h) {
  int topH = Theme::S(40);
  int gridH = Theme::S(88);
  RECT rt = {0, 0, w, topH};
  topPad_.Layout(rt);
  RECT rg = {0, topH + Theme::S(4), w, topH + Theme::S(4) + gridH};
  // 位网格直接移动窗口
  HWND g = GetDlgItem(hwnd_, 0x3200);
  if (g) MoveWindow(g, rg.left, rg.top, w, gridH, TRUE);
  RECT rp = {0, rg.bottom + Theme::S(4), w, h};
  pad_.Layout(rp);
}

void ProgrammerPanel::OnShow() { UpdateDisplay(); }

void ProgrammerPanel::Insert(const std::wstring& token) {
  std::wstring out;
  if (IntEng::InsertToken(expr_, token, base_, out)) {
    expr_ = out;
    justEval_ = false;
    UpdateDisplay();
  }
}

void ProgrammerPanel::UpdateDisplay() {
  ctx_.display->SetExpr(expr_);
  uint64_t v;
  std::wstring err;
  if (IntEng::Evaluate(expr_, base_, bits_, v, err)) {
    value_ = v;
    hasValue_ = true;
    bitGrid_.SetValue(v);
    ctx_.display->SetResult(NumFmt::FormatInteger(v, base_), false);
  } else {
    if (hasValue_) bitGrid_.SetValue(value_);
    ctx_.display->SetResult(L"", false);
  }
  // 四种进制明细
  std::vector<std::wstring> lines;
  lines.push_back(L"DEC  " + NumFmt::FormatSigned(value_, bits_));
  lines.push_back(L"HEX  " + NumFmt::FormatInteger(value_, 16));
  lines.push_back(L"OCT  " + NumFmt::FormatInteger(value_, 8));
  lines.push_back(L"BIN  " + NumFmt::FormatInteger(value_, 2));
  ctx_.display->SetAltLines(lines);
}

void ProgrammerPanel::Eval() {
  uint64_t v;
  std::wstring err;
  if (IntEng::Evaluate(expr_, base_, bits_, v, err)) {
    value_ = v;
    hasValue_ = true;
    bitGrid_.SetValue(v);
    justEval_ = true;
    ctx_.display->SetResult(NumFmt::FormatInteger(v, base_), false);
    if (ctx_.addHistory) {
      ctx_.addHistory(expr_, NumFmt::FormatInteger(v, base_));
    }
  } else if (!err.empty()) {
    ctx_.display->SetResult(err, true);
  }
  UpdateDisplay();
}

void ProgrammerPanel::SetBase(unsigned base) {
  base_ = base;
  ApplyBaseEnabled();
  topPad_.SetChecked(IDK_PR_BASE_HEX, base == 16);
  topPad_.SetChecked(IDK_PR_BASE_DEC, base == 10);
  topPad_.SetChecked(IDK_PR_BASE_OCT, base == 8);
  topPad_.SetChecked(IDK_PR_BASE_BIN, base == 2);
  UpdateDisplay();
}

void ProgrammerPanel::SetBits(unsigned bits) {
  bits_ = bits;
  bitGrid_.SetBits(bits);
  topPad_.SetChecked(IDK_PR_WS_QWORD, bits == 64);
  topPad_.SetChecked(IDK_PR_WS_DWORD, bits == 32);
  topPad_.SetChecked(IDK_PR_WS_WORD, bits == 16);
  topPad_.SetChecked(IDK_PR_WS_BYTE, bits == 8);
  UpdateDisplay();
}

void ProgrammerPanel::ApplyBaseEnabled() {
  // 按进制禁用不合法的数字键
  auto en = [&](int id, bool ok) {
    CtrlButton* b = pad_.Get(id);
    if (b) b->SetEnabled(ok);
  };
  en(IDK_PR_A, base_ == 16);
  en(IDK_PR_B, base_ == 16);
  en(IDK_PR_C, base_ == 16);
  en(IDK_PR_D, base_ == 16);
  en(IDK_PR_E, base_ == 16);
  en(IDK_PR_F, base_ == 16);
  en(IDK_8, base_ > 8);
  en(IDK_9, base_ > 8);
  en(IDK_2, base_ > 2);
  en(IDK_3, base_ > 2);
  en(IDK_4, base_ > 2);
  en(IDK_5, base_ > 2);
  en(IDK_6, base_ > 2);
  en(IDK_7, base_ > 2);
}

bool ProgrammerPanel::OnCommand(int id) {
  switch (id) {
    case IDK_PR_BASE_HEX: SetBase(16); return true;
    case IDK_PR_BASE_DEC: SetBase(10); return true;
    case IDK_PR_BASE_OCT: SetBase(8); return true;
    case IDK_PR_BASE_BIN: SetBase(2); return true;
    case IDK_PR_WS_QWORD: SetBits(64); return true;
    case IDK_PR_WS_DWORD: SetBits(32); return true;
    case IDK_PR_WS_WORD: SetBits(16); return true;
    case IDK_PR_WS_BYTE: SetBits(8); return true;
    case IDK_0: if (justEval_) { expr_.clear(); justEval_ = false; } Insert(L"0"); return true;
    case IDK_1: if (justEval_) { expr_.clear(); justEval_ = false; } Insert(L"1"); return true;
    case IDK_2: if (justEval_) { expr_.clear(); justEval_ = false; } Insert(L"2"); return true;
    case IDK_3: if (justEval_) { expr_.clear(); justEval_ = false; } Insert(L"3"); return true;
    case IDK_4: if (justEval_) { expr_.clear(); justEval_ = false; } Insert(L"4"); return true;
    case IDK_5: if (justEval_) { expr_.clear(); justEval_ = false; } Insert(L"5"); return true;
    case IDK_6: if (justEval_) { expr_.clear(); justEval_ = false; } Insert(L"6"); return true;
    case IDK_7: if (justEval_) { expr_.clear(); justEval_ = false; } Insert(L"7"); return true;
    case IDK_8: if (justEval_) { expr_.clear(); justEval_ = false; } Insert(L"8"); return true;
    case IDK_9: if (justEval_) { expr_.clear(); justEval_ = false; } Insert(L"9"); return true;
    case IDK_PR_A: if (justEval_) { expr_.clear(); justEval_ = false; } Insert(L"A"); return true;
    case IDK_PR_B: if (justEval_) { expr_.clear(); justEval_ = false; } Insert(L"B"); return true;
    case IDK_PR_C: if (justEval_) { expr_.clear(); justEval_ = false; } Insert(L"C"); return true;
    case IDK_PR_D: if (justEval_) { expr_.clear(); justEval_ = false; } Insert(L"D"); return true;
    case IDK_PR_E: if (justEval_) { expr_.clear(); justEval_ = false; } Insert(L"E"); return true;
    case IDK_PR_F: if (justEval_) { expr_.clear(); justEval_ = false; } Insert(L"F"); return true;
    case IDK_ADD: if (justEval_) { expr_ = NumFmt::FormatInteger(value_, base_); justEval_ = false; } Insert(L"+"); return true;
    case IDK_SUB: if (justEval_) { expr_ = NumFmt::FormatInteger(value_, base_); justEval_ = false; } Insert(L"−"); return true;
    case IDK_MUL: if (justEval_) { expr_ = NumFmt::FormatInteger(value_, base_); justEval_ = false; } Insert(L"×"); return true;
    case IDK_DIV: if (justEval_) { expr_ = NumFmt::FormatInteger(value_, base_); justEval_ = false; } Insert(L"÷"); return true;
    case IDK_PR_MOD:
      if (justEval_) { expr_ = NumFmt::FormatInteger(value_, base_); justEval_ = false; }
      Insert(L"%"); return true;
    case IDK_PR_LSH:
      if (justEval_) { expr_ = NumFmt::FormatInteger(value_, base_); justEval_ = false; }
      Insert(L"<<"); return true;
    case IDK_PR_RSH:
      if (justEval_) { expr_ = NumFmt::FormatInteger(value_, base_); justEval_ = false; }
      Insert(L">>"); return true;
    case IDK_PR_AND: Insert(L"&"); return true;
    case IDK_PR_OR: Insert(L"|"); return true;
    case IDK_PR_XOR: Insert(L"^"); return true;
    case IDK_PR_NOT: Insert(L"~"); return true;
    case IDK_PR_NAND: Insert(L"~&"); return true;
    case IDK_PR_LPAR: Insert(L"("); return true;
    case IDK_PR_RPAR: Insert(L")"); return true;
    case IDK_SIGN: {
      // ± ：二进制补码取反（-x = ~x + 1，按字长掩码）
      uint64_t v;
      std::wstring err;
      if (!IntEng::Evaluate(expr_.empty() ? L"0" : expr_, base_, bits_, v, err)) return true;
      uint64_t neg = (0ull - v) & IntEng::Mask(bits_);
      std::wstring raw = NumFmt::FormatInteger(neg, base_);
      std::wstring clean;
      for (wchar_t c : raw) if (c != L' ' && c != L',') clean += c;
      expr_ = clean;
      justEval_ = false;
      UpdateDisplay();
      return true;
    }
    case IDK_EQU: Eval(); return true;
    case IDK_CLEAR:
      expr_.clear();
      justEval_ = false;
      ctx_.display->SetExpr(L"");
      ctx_.display->SetResult(L"", false);
      return true;
    case IDK_CE:
      expr_.clear();
      UpdateDisplay();
      return true;
    case IDK_BACK: {
      std::wstring out;
      if (IntEng::BackspaceExpr(expr_, out)) {
        expr_ = out;
        justEval_ = false;
        UpdateDisplay();
      }
      return true;
    }
  }
  return false;
}

bool ProgrammerPanel::OnChar(wchar_t c) {
  if (c >= L'0' && c <= L'9') {
    int d = c - L'0';
    if (d >= (int)base_) return false;
    if (justEval_) { expr_.clear(); justEval_ = false; }
    Insert(std::wstring(1, c));
    return true;
  }
  if ((c >= L'a' && c <= L'f') || (c >= L'A' && c <= L'F')) {
    if (base_ != 16) return false;
    if (justEval_) { expr_.clear(); justEval_ = false; }
    Insert(std::wstring(1, towupper(c)));
    return true;
  }
  switch (c) {
    case L'+': Insert(L"+"); return true;
    case L'-': Insert(L"−"); return true;
    case L'*': Insert(L"×"); return true;
    case L'/': Insert(L"÷"); return true;
    case L'%': Insert(L"%"); return true;
    case L'(': Insert(L"("); return true;
    case L')': Insert(L")"); return true;
    case L'&': Insert(L"&"); return true;
    case L'|': Insert(L"|"); return true;
    case L'^': Insert(L"^"); return true;
    case L'~': Insert(L"~"); return true;
    case L'<': Insert(L"<"); return true;
    case L'>': Insert(L">"); return true;
  }
  return false;
}

bool ProgrammerPanel::OnKey(WPARAM vk) {
  switch (vk) {
    case VK_RETURN: Eval(); return true;
    case VK_ESCAPE:
      expr_.clear();
      justEval_ = false;
      ctx_.display->SetExpr(L"");
      ctx_.display->SetResult(L"", false);
      return true;
    case VK_BACK: {
      std::wstring out;
      if (IntEng::BackspaceExpr(expr_, out)) { expr_ = out; justEval_ = false; UpdateDisplay(); }
      return true;
    }
  }
  return false;
}

void ProgrammerPanel::LoadValue(const std::wstring& v) {
  std::wstring clean;
  for (wchar_t c : v) {
    if (c == L',' || c == L' ') continue;
    clean += c;
  }
  // 校验数字是否合法于当前进制
  for (wchar_t c : clean) {
    int d = -1;
    if (c >= L'0' && c <= L'9') d = c - L'0';
    else if ((c >= L'a' && c <= L'f') || (c >= L'A' && c <= L'F')) d = towlower(c) - L'a' + 10;
    if (d < 0 || d >= (int)base_) return;
  }
  if (clean.empty()) return;
  expr_ = clean;
  justEval_ = false;
  UpdateDisplay();
}

LRESULT ProgrammerPanel::HandleMessage(UINT msg, WPARAM w, LPARAM l) {
  return ModePanel::HandleMessage(msg, w, l);
}
