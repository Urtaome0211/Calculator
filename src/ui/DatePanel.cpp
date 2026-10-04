#include "DatePanel.h"
#include "Theme.h"
#include "CtrlButton.h"
#include "DisplayPanel.h"
#include "../core/NumberFormat.h"
#include <cstdlib>

// ============================================================
// 日期计算面板：日期差值 / 基准日期加减（天/周/月/年）
// ============================================================
namespace {
// 编辑框与加减号下拉在 Commands.h 中没有全局 ID，面板内自定义
enum : int {
  EDT_FROM = 0x3101, EDT_TO, EDT_BASE, EDT_AMOUNT,
  EDT_SIGN = 0x3105
};

std::wstring WinText(HWND h) {
  int len = GetWindowTextLengthW(h);
  std::wstring s((size_t)len, L'\0');
  if (len > 0) GetWindowTextW(h, &s[0], len + 1);
  return s;
}

struct CardLayout { RECT card, label, ctrl; };

// 日期差模式 2 张卡、加减模式 4 张卡
std::vector<CardLayout> DateCards(int w, bool diff) {
  const int y0 = Theme::S(44) + Theme::S(12);
  const int ch = Theme::S(84);
  int n = diff ? 2 : 4;
  int cw = diff ? (w - 3 * Theme::S(12)) / 2 : (w - 5 * Theme::S(12)) / 4;
  std::vector<CardLayout> cards;
  for (int i = 0; i < n; i++) {
    int x = Theme::S(12) + i * (cw + Theme::S(12));
    CardLayout c;
    c.card = {x, y0, x + cw, y0 + ch};
    c.label = {x + Theme::S(10), y0 + Theme::S(8), x + cw - Theme::S(10), y0 + Theme::S(22)};
    c.ctrl = {x + Theme::S(10), y0 + Theme::S(30), x + cw - Theme::S(10), y0 + Theme::S(70)};
    cards.push_back(c);
  }
  return cards;
}
}  // namespace

HWND DatePanel::Create(HWND parent, HINSTANCE inst, const AppContext& ctx) {
  ctx_ = ctx;
  hwnd_ = CreateWindowExW(0, ModePanel::ClassName(), L"date", WS_CHILD | WS_VISIBLE,
                          0, 0, 10, 10, parent, nullptr, inst, this);

  using K = Theme::Kind;
  // 顶部模式切换
  std::vector<KeyRow> modeRows = {
    {{{L"日期差值", IDK_DT_MODE_DIFF, K::Toggle}, {L"加减天数", IDK_DT_MODE_ADD, K::Toggle}}},
  };
  topPad_.Build(hwnd_, modeRows, 2, Theme::S(4), Theme::S(4));

  // 日期编辑框（扁平无边框，卡片背景由 WM_PAINT 绘制）
  auto MakeEdit = [&](int id, const std::wstring& text) {
    HWND e = CreateWindowExW(0, L"EDIT", text.c_str(),
                             WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
                             0, 0, 10, 10, hwnd_, (HMENU)(INT_PTR)id, inst, nullptr);
    SendMessageW(e, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN,
                 MAKELPARAM(Theme::S(10), Theme::S(10)));
    SendMessageW(e, WM_SETFONT, (WPARAM)Theme::FontH(14), TRUE);
    Theme::ApplyNativeDark(e);
    return e;
  };

  DateCalc::Date today = DateCalc::Today();
  editFrom_ = MakeEdit(EDT_FROM, DateCalc::Format(DateCalc::AddDays(today, -30)));
  editTo_ = MakeEdit(EDT_TO, DateCalc::Format(today));
  editBase_ = MakeEdit(EDT_BASE, DateCalc::Format(today));
  editAmount_ = MakeEdit(EDT_AMOUNT, L"1");

  // 单位下拉（天/周/月/年）与加减号下拉
  auto MakeCombo = [&](int id) {
    HWND c = CreateWindowExW(0, L"COMBOBOX", L"",
                             WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
                             0, 0, 10, 10, hwnd_, (HMENU)(INT_PTR)id, inst, nullptr);
    SendMessageW(c, CB_SETITEMHEIGHT, (WPARAM)-1, Theme::S(26));
    SendMessageW(c, WM_SETFONT, (WPARAM)Theme::FontH(14), TRUE);
    Theme::ApplyNativeDark(c);
    return c;
  };
  comboUnit_ = MakeCombo(IDK_DT_COMBO_UNIT);
  for (auto* s : {L"天", L"周", L"月", L"年"}) SendMessageW(comboUnit_, CB_ADDSTRING, 0, (LPARAM)s);
  SendMessageW(comboUnit_, CB_SETCURSEL, 0, 0);
  comboSign_ = MakeCombo(EDT_SIGN);
  SendMessageW(comboSign_, CB_ADDSTRING, 0, (LPARAM)L"+");
  SendMessageW(comboSign_, CB_ADDSTRING, 0, (LPARAM)L"−");
  SendMessageW(comboSign_, CB_SETCURSEL, 0, 0);

  // 计算 / 重置按钮
  CtrlButton::Create(hwnd_, L"计算", IDK_DT_CALC, Theme::Kind::Accent);
  CtrlButton::Create(hwnd_, L"重置", IDK_DT_RESET, Theme::Kind::Function);

  ApplyMode();
  return hwnd_;
}

void DatePanel::OnSize(int w, int h) {
  // 顶部模式切换
  RECT rt = {Theme::S(12), 0, Theme::S(12) + std::min(w - 2 * Theme::S(12), Theme::S(360)), Theme::S(44)};
  topPad_.Layout(rt);

  // 卡片与控件
  auto cards = DateCards(w, modeDiff_);
  HWND ctrls[4] = {nullptr, nullptr, nullptr, nullptr};
  if (modeDiff_) { ctrls[0] = editFrom_; ctrls[1] = editTo_; }
  else { ctrls[0] = editBase_; ctrls[1] = comboSign_; ctrls[2] = editAmount_; ctrls[3] = comboUnit_; }
  for (int i = 0; i < (modeDiff_ ? 2 : 4); i++) {
    if (!ctrls[i]) continue;
    const RECT& c = cards[i].ctrl;
    MoveWindow(ctrls[i], c.left, c.top, c.right - c.left, c.bottom - c.top, TRUE);
  }

  // 按钮行
  int by = Theme::S(44) + Theme::S(12) + Theme::S(84) + Theme::S(12);
  HWND b1 = GetDlgItem(hwnd_, IDK_DT_CALC);
  HWND b2 = GetDlgItem(hwnd_, IDK_DT_RESET);
  if (b1) MoveWindow(b1, Theme::S(12), by, Theme::S(140), Theme::S(44), TRUE);
  if (b2) MoveWindow(b2, Theme::S(12) + Theme::S(140) + Theme::S(8), by, Theme::S(140), Theme::S(44), TRUE);
  InvalidateRect(hwnd_, nullptr, TRUE);
}

void DatePanel::OnShow() { ApplyMode(); }

void DatePanel::ApplyMode() {
  bool d = modeDiff_;
  ShowWindow(editFrom_, d ? SW_SHOW : SW_HIDE);
  ShowWindow(editTo_, d ? SW_SHOW : SW_HIDE);
  ShowWindow(editBase_, d ? SW_HIDE : SW_SHOW);
  ShowWindow(editAmount_, d ? SW_HIDE : SW_SHOW);
  ShowWindow(comboUnit_, d ? SW_HIDE : SW_SHOW);
  ShowWindow(comboSign_, d ? SW_HIDE : SW_SHOW);
  topPad_.SetChecked(IDK_DT_MODE_DIFF, d);
  topPad_.SetChecked(IDK_DT_MODE_ADD, !d);
  RECT rc;
  GetClientRect(hwnd_, &rc);
  OnSize(rc.right - rc.left, rc.bottom - rc.top);
  ctx_.display->SetExpr(L"");
  ctx_.display->SetResult(L"", false);
  ctx_.display->SetAltLines({L"", L""});
}

bool DatePanel::OnCommand(int id) {
  switch (id) {
    case IDK_DT_MODE_DIFF: modeDiff_ = true; ApplyMode(); return true;
    case IDK_DT_MODE_ADD: modeDiff_ = false; ApplyMode(); return true;
    case IDK_DT_CALC: DoCalc(); return true;
    case IDK_DT_RESET: ResetInputs(); return true;
  }
  return false;
}

void DatePanel::ResetInputs() {
  DateCalc::Date today = DateCalc::Today();
  SetWindowTextW(editFrom_, DateCalc::Format(DateCalc::AddDays(today, -30)).c_str());
  SetWindowTextW(editTo_, DateCalc::Format(today).c_str());
  SetWindowTextW(editBase_, DateCalc::Format(today).c_str());
  SetWindowTextW(editAmount_, L"1");
  SendMessageW(comboUnit_, CB_SETCURSEL, 0, 0);
  SendMessageW(comboSign_, CB_SETCURSEL, 0, 0);
  ApplyMode();
}

void DatePanel::DoCalc() {
  if (modeDiff_) {
    DateCalc::Date a, b;
    if (!DateCalc::Parse(WinText(editFrom_), a) || !DateCalc::Parse(WinText(editTo_), b) ||
        !DateCalc::IsValid(a) || !DateCalc::IsValid(b)) {
      ctx_.display->SetResult(L"无效日期", true);
      return;
    }
    long long total;
    int years, months, days;
    DateCalc::Diff(a, b, total, years, months, days);
    ctx_.display->SetExpr(DateCalc::Format(a) + L" → " + DateCalc::Format(b));
    ctx_.display->SetResult(NumFmt::FormatDouble((double)total) + L" 天", false);
    std::wstring detail;
    if (years > 0) detail += std::to_wstring(years) + L" 年 ";
    if (months > 0 || years > 0) detail += std::to_wstring(months) + L" 月 ";
    detail += std::to_wstring(days) + L" 天";
    ctx_.display->SetAltLines({
      L"相差 " + detail,
      DateCalc::Format(a) + L" (" + DateCalc::WeekdayName(a) + L") → " +
      DateCalc::Format(b) + L" (" + DateCalc::WeekdayName(b) + L")"});
  } else {
    DateCalc::Date base;
    if (!DateCalc::Parse(WinText(editBase_), base) || !DateCalc::IsValid(base)) {
      ctx_.display->SetResult(L"无效日期", true);
      return;
    }
    std::wstring amtText = WinText(editAmount_);
    std::wstring clean;
    for (wchar_t c : amtText) if (c != L',' && c != L' ') clean += c;
    wchar_t* end = nullptr;
    double amount = wcstod(clean.c_str(), &end);
    if (clean.empty() || end == clean.c_str()) {
      ctx_.display->SetResult(L"无效数字", true);
      return;
    }
    int sign = SendMessageW(comboSign_, CB_GETCURSEL, 0, 0) == 0 ? 1 : -1;
    long long n = (long long)amount * sign;
    int unit = (int)SendMessageW(comboUnit_, CB_GETCURSEL, 0, 0);
    DateCalc::Date result;
    std::wstring unitName = L"天";
    switch (unit) {
      case 0: result = DateCalc::AddDays(base, n); unitName = L"天"; break;
      case 1: result = DateCalc::AddDays(base, n * 7); unitName = L"周"; break;
      case 2: result = DateCalc::AddMonths(base, n); unitName = L"月"; break;
      default: result = DateCalc::AddYears(base, n); unitName = L"年"; break;
    }
    long long totalDays = DateCalc::ToDays(result) - DateCalc::ToDays(base);
    ctx_.display->SetExpr(DateCalc::Format(base) + L" " + (sign > 0 ? L"+" : L"−") + L" " +
                          clean + L" " + unitName);
    ctx_.display->SetResult(DateCalc::Format(result) + L" (" + DateCalc::WeekdayName(result) + L")", false);
    ctx_.display->SetAltLines({
      L"共加 " + std::to_wstring(totalDays) + L" 天",
      L"基准日期 " + DateCalc::Format(base)});
  }
}

LRESULT DatePanel::HandleMessage(UINT msg, WPARAM w, LPARAM l) {
  switch (msg) {
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORLISTBOX: {
      HDC dc = (HDC)w;
      SetBkColor(dc, Theme::P().card);
      SetTextColor(dc, Theme::P().text);
      return (LRESULT)Theme::CardBrush();
    }
    case WM_PAINT: {
      PAINTSTRUCT ps;
      HDC dc = BeginPaint(hwnd_, &ps);
      Theme::PaintBuffer(hwnd_, ps, [&](Gdiplus::Graphics& g, int cw, int ch) {
        Gdiplus::SolidBrush bg(Theme::C(Theme::P().bg));
        g.FillRectangle(&bg, 0, 0, cw, ch);
        auto cards = DateCards(cw, modeDiff_);
        const std::vector<std::wstring> labels =
            modeDiff_ ? std::vector<std::wstring>{L"开始日期", L"结束日期"}
                      : std::vector<std::wstring>{L"基准日期", L"加减", L"数量", L"单位"};
        Gdiplus::Font* f = Theme::Font((float)Theme::S(12));
        for (size_t i = 0; i < cards.size() && i < labels.size(); i++) {
          const CardLayout& c = cards[i];
          if (c.card.right <= c.card.left || c.card.bottom <= c.card.top) continue;
          Theme::FillRound(g,
                           Gdiplus::RectF((float)c.card.left, (float)c.card.top,
                                          (float)(c.card.right - c.card.left),
                                          (float)(c.card.bottom - c.card.top)),
                           Theme::C(Theme::P().card), (float)Theme::S(8));
          Theme::DrawTextCentered(g, labels[i], f, Theme::C(Theme::P().textDim),
                                  Gdiplus::RectF((float)c.label.left, (float)c.label.top,
                                                 (float)(c.label.right - c.label.left),
                                                 (float)(c.label.bottom - c.label.top)));
        }
      });
      EndPaint(hwnd_, &ps);
      return 0;
    }
  }
  return ModePanel::HandleMessage(msg, w, l);
}
