#include "ConverterPanel.h"
#include "Theme.h"
#include "CtrlButton.h"
#include "DisplayPanel.h"
#include "../core/UnitConverter.h"
#include "../core/NumberFormat.h"
#include <cstdlib>

// ============================================================
// 单位转换器面板：类别 + 双单位卡片，双向即时换算
// ============================================================
namespace {
std::wstring WinText(HWND h) {
  int len = GetWindowTextLengthW(h);
  std::wstring s((size_t)len, L'\0');
  if (len > 0) GetWindowTextW(h, &s[0], len + 1);
  return s;
}

std::wstring ComboText(HWND h) {
  int sel = (int)SendMessageW(h, CB_GETCURSEL, 0, 0);
  if (sel < 0) return L"";
  int len = (int)SendMessageW(h, CB_GETLBTEXTLEN, sel, 0);
  std::wstring s((size_t)len, L'\0');
  if (len > 0) SendMessageW(h, CB_GETLBTEXT, sel, (LPARAM)&s[0]);
  return s;
}
}  // namespace

HWND ConverterPanel::Create(HWND parent, HINSTANCE inst, const AppContext& ctx) {
  ctx_ = ctx;
  hwnd_ = CreateWindowExW(0, ModePanel::ClassName(), L"converter", WS_CHILD | WS_VISIBLE,
                          0, 0, 10, 10, parent, nullptr, inst, this);

  // 类别下拉
  comboCat_ = CreateWindowExW(0, L"COMBOBOX", L"",
                              WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
                              0, 0, 10, 10, hwnd_, (HMENU)(INT_PTR)IDK_CV_COMBO_CAT, inst, nullptr);
  const auto& cats = UnitConv::Categories();
  int catSel = 0;
  for (size_t i = 0; i < cats.size(); i++) {
    SendMessageW(comboCat_, CB_ADDSTRING, 0, (LPARAM)cats[i].name.c_str());
    if (cats[i].id == catId_) catSel = (int)i;
  }
  SendMessageW(comboCat_, CB_SETITEMHEIGHT, (WPARAM)-1, Theme::S(26));
  SendMessageW(comboCat_, WM_SETFONT, (WPARAM)Theme::FontH(14), TRUE);
  SendMessageW(comboCat_, CB_SETCURSEL, catSel, 0);

  // 单位下拉与数值输入框
  auto MakeCombo = [&](int id) {
    HWND c = CreateWindowExW(0, L"COMBOBOX", L"",
                             WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
                             0, 0, 10, 10, hwnd_, (HMENU)(INT_PTR)id, inst, nullptr);
    SendMessageW(c, CB_SETITEMHEIGHT, (WPARAM)-1, Theme::S(26));
    SendMessageW(c, WM_SETFONT, (WPARAM)Theme::FontH(14), TRUE);
    Theme::ApplyNativeDark(c);
    return c;
  };
  auto MakeEdit = [&](int id, const std::wstring& text) {
    HWND e = CreateWindowExW(0, L"EDIT", text.c_str(),
                             WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
                             0, 0, 10, 10, hwnd_, (HMENU)(INT_PTR)id, inst, nullptr);
    SendMessageW(e, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN,
                 MAKELPARAM(Theme::S(10), Theme::S(10)));
    SendMessageW(e, WM_SETFONT, (WPARAM)Theme::FontH(22), TRUE);
    Theme::ApplyNativeDark(e);
    return e;
  };
  comboFrom_ = MakeCombo(IDK_CV_COMBO_FROM);
  comboTo_ = MakeCombo(IDK_CV_COMBO_TO);
  editFrom_ = MakeEdit(IDK_CV_EDIT_FROM, L"1");
  editTo_ = MakeEdit(IDK_CV_EDIT_TO, L"");

  btnSwap_ = CtrlButton::Create(hwnd_, L"\uE8AB", IDK_CV_SWAP, Theme::Kind::Function)->Hwnd();

  ReloadUnits();
  Convert(true);
  return hwnd_;
}

void ConverterPanel::OnSize(int w, int h) {
  // 类别下拉
  MoveWindow(comboCat_, Theme::S(12), Theme::S(10), w - 2 * Theme::S(12), Theme::S(38), TRUE);

  // 双卡片
  int y0 = Theme::S(58), ch = Theme::S(110);
  int cw = (w - 3 * Theme::S(12) - Theme::S(48)) / 2;
  int leftX = Theme::S(12), rightX = w - Theme::S(12) - cw;
  int innerW = cw - 2 * Theme::S(10);
  MoveWindow(comboFrom_, leftX + Theme::S(10), y0 + Theme::S(10), innerW, Theme::S(36), TRUE);
  MoveWindow(editFrom_, leftX + Theme::S(10), y0 + Theme::S(52), innerW, Theme::S(52), TRUE);
  MoveWindow(comboTo_, rightX + Theme::S(10), y0 + Theme::S(10), innerW, Theme::S(36), TRUE);
  MoveWindow(editTo_, rightX + Theme::S(10), y0 + Theme::S(52), innerW, Theme::S(52), TRUE);

  // 交换按钮居中
  if (btnSwap_) {
    int midW = rightX - (leftX + cw);
    int sx = leftX + cw + (midW - Theme::S(44)) / 2;
    int sy = y0 + (ch - Theme::S(44)) / 2;
    MoveWindow(btnSwap_, sx, sy, Theme::S(44), Theme::S(44), TRUE);
  }
  InvalidateRect(hwnd_, nullptr, TRUE);
}

void ConverterPanel::OnShow() { Convert(true); }

void ConverterPanel::SetCategory(const std::wstring& id) {
  catId_ = id;
  const auto& cats = UnitConv::Categories();
  for (size_t i = 0; i < cats.size(); i++) {
    if (cats[i].id == id) {
      SendMessageW(comboCat_, CB_SETCURSEL, (WPARAM)i, 0);
      break;
    }
  }
  ReloadUnits();
}

void ConverterPanel::ReloadUnits() {
  const UnitConv::Category* cat = UnitConv::Find(catId_);
  if (!cat) return;
  updating_ = true;                    // 屏蔽重入：期间的 CBN_SELCHANGE/EN_CHANGE 一律忽略
  SendMessageW(comboFrom_, CB_RESETCONTENT, 0, 0);
  SendMessageW(comboTo_, CB_RESETCONTENT, 0, 0);
  for (const auto& u : cat->units) {
    SendMessageW(comboFrom_, CB_ADDSTRING, 0, (LPARAM)u.c_str());
    SendMessageW(comboTo_, CB_ADDSTRING, 0, (LPARAM)u.c_str());
  }
  SendMessageW(comboFrom_, CB_SETCURSEL, 0, 0);
  SendMessageW(comboTo_, CB_SETCURSEL, std::min<WPARAM>(1, cat->units.size() - 1), 0);
  SetWindowTextW(editFrom_, L"1");
  SetWindowTextW(editTo_, L"");
  updating_ = false;
}

void ConverterPanel::Swap() {
  // 交换单位选择与数值
  std::wstring fromUnit = ComboText(comboFrom_);
  std::wstring toUnit = ComboText(comboTo_);
  for (HWND c : {comboFrom_, comboTo_}) {
    // 找到单位在列表中的位置
  }
  // 直接在两个下拉中查找并交换选择
  const UnitConv::Category* cat = UnitConv::Find(catId_);
  if (!cat) return;
  auto findIdx = [&](HWND cb, const std::wstring& name) {
    for (size_t i = 0; i < cat->units.size(); i++) {
      std::wstring t;
      int len = (int)SendMessageW(cb, CB_GETLBTEXTLEN, (WPARAM)i, 0);
      t.resize((size_t)len);
      if (len > 0) SendMessageW(cb, CB_GETLBTEXT, (WPARAM)i, (LPARAM)&t[0]);
      if (t == name) return (int)i;
    }
    return -1;
  };
  int i1 = findIdx(comboFrom_, fromUnit);
  int i2 = findIdx(comboTo_, toUnit);
  std::wstring v1 = WinText(editFrom_);
  std::wstring v2 = WinText(editTo_);
  updating_ = true;
  if (i1 >= 0) SendMessageW(comboTo_, CB_SETCURSEL, i1, 0);
  if (i2 >= 0) SendMessageW(comboFrom_, CB_SETCURSEL, i2, 0);
  SetWindowTextW(editFrom_, v2.c_str());
  SetWindowTextW(editTo_, v1.c_str());
  updating_ = false;
  Convert(true);
}

void ConverterPanel::Convert(bool fromTo) {
  if (updating_) return;
  HWND srcEdit = fromTo ? editFrom_ : editTo_;
  HWND dstEdit = fromTo ? editTo_ : editFrom_;
  HWND srcCombo = fromTo ? comboFrom_ : comboTo_;
  HWND dstCombo = fromTo ? comboTo_ : comboFrom_;
  std::wstring srcText = WinText(srcEdit);
  std::wstring clean;
  for (wchar_t c : srcText) if (c != L',' && c != L' ') clean += c;
  std::wstring fromUnit = ComboText(srcCombo);
  std::wstring toUnit = ComboText(dstCombo);

  if (clean.empty()) {
    updating_ = true;                  // 防止写文本触发的 EN_CHANGE 反向重入形成死循环
    SetWindowTextW(dstEdit, L"");
    updating_ = false;
    double rate = 0;
    if (UnitConv::Convert(catId_, fromUnit, toUnit, 1.0, rate))
      ctx_.display->SetAltLines({L"1 " + fromUnit + L" = " + NumFmt::FormatDouble(rate) + L" " + toUnit});
    return;
  }
  wchar_t* end = nullptr;
  double v = wcstod(clean.c_str(), &end);
  if (end == clean.c_str()) {
    updating_ = true;
    SetWindowTextW(dstEdit, L"");
    updating_ = false;
    ctx_.display->SetResult(L"输入无效", true);
    return;
  }
  double out = 0;
  if (!UnitConv::Convert(catId_, fromUnit, toUnit, v, out)) {
    updating_ = true;
    SetWindowTextW(dstEdit, L"");
    updating_ = false;
    ctx_.display->SetResult(L"输入无效", true);
    return;
  }
  updating_ = true;
  SetWindowTextW(dstEdit, NumFmt::FormatDouble(out).c_str());
  updating_ = false;

  ctx_.display->SetExpr(clean + L" " + fromUnit);
  ctx_.display->SetResult(NumFmt::FormatDouble(out) + L" " + toUnit, false);
  double rate = 0;
  if (UnitConv::Convert(catId_, fromUnit, toUnit, 1.0, rate))
    ctx_.display->SetAltLines({L"1 " + fromUnit + L" = " + NumFmt::FormatDouble(rate) + L" " + toUnit});
}

bool ConverterPanel::OnCommand(int id) {
  if (id == IDK_CV_SWAP) { Swap(); return true; }
  return false;
}

LRESULT ConverterPanel::HandleMessage(UINT msg, WPARAM w, LPARAM l) {
  switch (msg) {
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORLISTBOX: {
      HDC dc = (HDC)w;
      SetBkColor(dc, Theme::P().card);
      SetTextColor(dc, Theme::P().text);
      return (LRESULT)Theme::CardBrush();
    }
    case WM_COMMAND:
      switch (HIWORD(w)) {
        case CBN_SELCHANGE:
          if (LOWORD(w) == IDK_CV_COMBO_CAT) {
            int sel = (int)SendMessageW(comboCat_, CB_GETCURSEL, 0, 0);
            const auto& cats = UnitConv::Categories();
            if (sel >= 0 && sel < (int)cats.size()) {
              catId_ = cats[sel].id;
              ReloadUnits();          // 内部已把两个输入框重置为 "1" 与 ""
              Convert(true);
            }
          } else if (LOWORD(w) == IDK_CV_COMBO_FROM || LOWORD(w) == IDK_CV_COMBO_TO) {
            Convert(true);
          }
          return 0;
        case EN_CHANGE:
          if (LOWORD(w) == IDK_CV_EDIT_FROM) Convert(true);
          else if (LOWORD(w) == IDK_CV_EDIT_TO) Convert(false);
          return 0;
      }
      break;
    case WM_PAINT: {
      PAINTSTRUCT ps;
      HDC dc = BeginPaint(hwnd_, &ps);
      Theme::PaintBuffer(hwnd_, ps, [&](Gdiplus::Graphics& g, int cw, int ch) {
        Gdiplus::SolidBrush bg(Theme::C(Theme::P().bg));
        g.FillRectangle(&bg, 0, 0, cw, ch);
        // 两张圆角卡片
        int y0 = Theme::S(58), cardH = Theme::S(110);
        int cardW = (cw - 3 * Theme::S(12) - Theme::S(48)) / 2;
        Theme::FillRound(g,
                         Gdiplus::RectF((float)Theme::S(12), (float)y0, (float)cardW, (float)cardH),
                         Theme::C(Theme::P().card), (float)Theme::S(8));
        Theme::FillRound(g,
                         Gdiplus::RectF((float)(cw - Theme::S(12) - cardW), (float)y0, (float)cardW, (float)cardH),
                         Theme::C(Theme::P().card), (float)Theme::S(8));
      });
      EndPaint(hwnd_, &ps);
      return 0;
    }
  }
  return ModePanel::HandleMessage(msg, w, l);
}
