#include "KeypadPanel.h"
#include "Theme.h"
#include "DisplayPanel.h"
#include "../core/NumberFormat.h"

// ============================================================
// 键盘面板基类：记忆条 + 主键盘 + 表达式状态机 + 实时显示
// ============================================================
HWND KeypadPanel::Create(HWND parent, HINSTANCE inst, const AppContext& ctx) {
  ctx_ = ctx;
  hwnd_ = CreateWindowExW(0, ModePanel::ClassName(), L"keypad", WS_CHILD | WS_VISIBLE,
                          0, 0, 10, 10, parent, nullptr, inst, this);
  // 记忆条
  auto memCells = MemoryRow();
  if (!memCells.empty()) {
    std::vector<KeyRow> memRows;
    memRows.push_back({memCells});
    memPad_.Build(hwnd_, memRows, (int)memCells.size(), Theme::S(4), Theme::S(4));
  }
  // 主键盘（由子类 BuildLayout 装配）
  BuildLayout(pad_);
  return hwnd_;
}

void KeypadPanel::OnSize(int w, int h) {
  Layout();
  (void)w; (void)h;
}

void KeypadPanel::Layout() {
  RECT rc;
  GetClientRect(hwnd_, &rc);
  int w = rc.right - rc.left, h = rc.bottom - rc.top;
  int memH = Theme::S(44);
  bool hasMem = false;
  // 判断是否有记忆条（通过 Get 判断）
  hasMem = memPad_.Get(IDK_MC) != nullptr || memPad_.Get(IDK_SC_DEG) != nullptr;
  if (hasMem) {
    RECT mr = {0, 0, w, memH};
    memPad_.Layout(mr);
    RECT pr = {0, memH + Theme::S(4), w, h - memH - Theme::S(4)};
    pad_.Layout(pr);
  } else {
    RECT pr = {0, 0, w, h};
    pad_.Layout(pr);
  }
}

void KeypadPanel::OnShow() { UpdateDisplay(); }

void KeypadPanel::Insert(const std::wstring& token) {
  std::wstring out;
  if (Expr::InsertToken(expr_, token, out)) {
    expr_ = out;
    UpdateDisplay();
  }
}

void KeypadPanel::UpdateDisplay() {
  ctx_.display->SetExpr(NumFmt::GroupExpr(expr_));
  double v;
  if (EvalNow(v)) {
    ctx_.display->SetResult(NumFmt::FormatDouble(v), false);
  } else {
    ctx_.display->SetResult(L"", false);
  }
}

bool KeypadPanel::EvalNow(double& v) {
  std::wstring err;
  return Expr::Evaluate(expr_, v, err, Angle());
}

void KeypadPanel::SetResultText(double v) {
  lastResult_ = v;
  ctx_.display->SetResult(NumFmt::FormatDouble(v), false);
}

void KeypadPanel::PushHistory() {
  std::wstring exprText = NumFmt::GroupExpr(expr_);
  std::wstring resText = NumFmt::FormatDouble(lastResult_);
  if (ctx_.addHistory) ctx_.addHistory(exprText, resText);
}

void KeypadPanel::PressDigit(wchar_t c) {
  if (justEval_) {
    expr_.clear();
    justEval_ = false;
  }
  Insert(std::wstring(1, c));
}

void KeypadPanel::PressOp(const std::wstring& sym) {
  if (justEval_) {
    expr_ = NumFmt::FormatRaw(lastResult_);
    justEval_ = false;
  }
  if (expr_.empty()) expr_ = L"0";
  Insert(sym);
}

void KeypadPanel::PressEquals() {
  double v;
  std::wstring err;
  if (Expr::Evaluate(expr_, v, err, Angle())) {
    SetResultText(v);
    justEval_ = true;
    PushHistory();
  } else if (!err.empty()) {
    ctx_.display->SetResult(err, true);
  }
}

void KeypadPanel::ApplyUnary(const std::wstring& prefix, const std::wstring& suffix) {
  double v;
  if (!EvalNow(v)) return;
  std::wstring raw = NumFmt::FormatRaw(v);
  expr_ = prefix + raw + suffix;
  justEval_ = false;
  UpdateDisplay();
}

void KeypadPanel::LoadValue(const std::wstring& v) {
  std::wstring s = v;
  // 去掉千分位与分组空格
  std::wstring clean;
  for (wchar_t c : s) {
    if (c == L',' || c == L' ') continue;
    clean += c;
  }
  if (clean.empty()) return;
  expr_ = clean;
  justEval_ = false;
  UpdateDisplay();
}

std::vector<KeyCell> KeypadPanel::MemoryRow() const {
  return {
    {L"MC", IDK_MC, Theme::Kind::Memory, 1, 1, true},
    {L"MR", IDK_MR, Theme::Kind::Memory, 1, 1, true},
    {L"M+", IDK_MP, Theme::Kind::Memory, 1, 1, true},
    {L"M−", IDK_MM, Theme::Kind::Memory, 1, 1, true},
    {L"MS", IDK_MS, Theme::Kind::Memory, 1, 1, true},
  };
}

bool KeypadPanel::OnCommand(int id) {
  switch (id) {
    case IDK_0: PressDigit(L'0'); return true;
    case IDK_1: PressDigit(L'1'); return true;
    case IDK_2: PressDigit(L'2'); return true;
    case IDK_3: PressDigit(L'3'); return true;
    case IDK_4: PressDigit(L'4'); return true;
    case IDK_5: PressDigit(L'5'); return true;
    case IDK_6: PressDigit(L'6'); return true;
    case IDK_7: PressDigit(L'7'); return true;
    case IDK_8: PressDigit(L'8'); return true;
    case IDK_9: PressDigit(L'9'); return true;
    case IDK_DOT: PressDigit(L'.'); return true;
    case IDK_ADD: PressOp(L"+"); return true;
    case IDK_SUB: PressOp(L"−"); return true;
    case IDK_MUL: PressOp(L"×"); return true;
    case IDK_DIV: PressOp(L"÷"); return true;
    case IDK_EQU: PressEquals(); return true;
    case IDK_CLEAR:
      expr_.clear();
      justEval_ = false;
      ctx_.display->SetExpr(L"");
      ctx_.display->SetResult(L"", false);
      return true;
    case IDK_CE: {
      std::wstring out;
      if (Expr::ClearEntry(expr_, out)) {
        expr_ = out;
        UpdateDisplay();
      }
      return true;
    }
    case IDK_BACK: {
      std::wstring out;
      if (Expr::BackspaceExpr(expr_, out)) {
        expr_ = out;
        justEval_ = false;
        UpdateDisplay();
      }
      return true;
    }
    case IDK_PERCENT: {
      std::wstring out;
      if (Expr::ApplyPercent(expr_, out, Angle())) {
        expr_ = out;
        UpdateDisplay();
      }
      return true;
    }
    case IDK_SIGN: {
      std::wstring out;
      if (Expr::NegateEntry(expr_, out)) {
        expr_ = out;
        justEval_ = false;
        UpdateDisplay();
      }
      return true;
    }
    case IDK_RECIP: ApplyUnary(L"1/(", L")"); return true;
    case IDK_SQ: ApplyUnary(L"sqr(", L")"); return true;
    case IDK_SQRT: ApplyUnary(L"sqrt(", L")"); return true;
    case IDK_MC:
      if (ctx_.memory) {
        *ctx_.memory = 0;
        if (ctx_.refreshMemory) ctx_.refreshMemory();
      }
      return true;
    case IDK_MR: {
      if (ctx_.memory) {
        if (justEval_) { expr_.clear(); justEval_ = false; }
        Insert(NumFmt::FormatRaw(*ctx_.memory));
      }
      return true;
    }
    case IDK_MP: case IDK_MM: case IDK_MS: {
      if (!ctx_.memory) return true;
      double v;
      if (!EvalNow(v)) return true;
      if (id == IDK_MP) *ctx_.memory += v;
      else if (id == IDK_MM) *ctx_.memory -= v;
      else *ctx_.memory = v;
      if (ctx_.refreshMemory) ctx_.refreshMemory();
      return true;
    }
  }
  return false;
}

bool KeypadPanel::OnChar(wchar_t c) {
  if (c >= L'0' && c <= L'9') { PressDigit(c); return true; }
  switch (c) {
    case L'.': PressDigit(L'.'); return true;
    case L'+': PressOp(L"+"); return true;
    case L'-': PressOp(L"−"); return true;
    case L'*': PressOp(L"×"); return true;
    case L'/': PressOp(L"÷"); return true;
    case L'%': {
      std::wstring out;
      if (Expr::ApplyPercent(expr_, out, Angle())) { expr_ = out; UpdateDisplay(); }
      return true;
    }
    case L'(': Insert(L"("); return true;
    case L')': Insert(L")"); return true;
    case L',': Insert(L","); return true;
  }
  return false;
}

bool KeypadPanel::OnKey(WPARAM vk) {
  switch (vk) {
    case VK_RETURN: PressEquals(); return true;
    case VK_BACK: {
      std::wstring out;
      if (Expr::BackspaceExpr(expr_, out)) { expr_ = out; justEval_ = false; UpdateDisplay(); }
      return true;
    }
    case VK_ESCAPE:
      expr_.clear();
      justEval_ = false;
      ctx_.display->SetExpr(L"");
      ctx_.display->SetResult(L"", false);
      return true;
    case VK_DELETE: {
      std::wstring out;
      if (Expr::ClearEntry(expr_, out)) { expr_ = out; UpdateDisplay(); }
      return true;
    }
    case VK_F9: {
      std::wstring out;
      if (Expr::NegateEntry(expr_, out)) { expr_ = out; justEval_ = false; UpdateDisplay(); }
      return true;
    }
  }
  return false;
}

LRESULT KeypadPanel::HandleMessage(UINT msg, WPARAM w, LPARAM l) {
  return ModePanel::HandleMessage(msg, w, l);
}
