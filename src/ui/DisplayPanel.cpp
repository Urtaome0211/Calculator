#include "DisplayPanel.h"
#include "Theme.h"
#include "../core/NumberFormat.h"

// ============================================================
// 显示区：表达式行 + 结果行 + 次要信息行 + M 记忆徽标
// ============================================================
namespace {
bool g_registered = false;
}

int DisplayPanel::Height() const {
  return Theme::S(8) + Theme::S(24) + Theme::S(56) + (int)alt_.size() * Theme::S(22) + Theme::S(6);
}

HWND DisplayPanel::Create(HWND parent, int id) {
  if (!g_registered) {
    WNDCLASSW wc = {};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = DisplayPanel::WndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"W11Display";
    RegisterClassW(&wc);
    g_registered = true;
  }
  hwnd_ = CreateWindowExW(0, L"W11Display", L"", WS_CHILD | WS_VISIBLE,
                          0, 0, 100, 100, parent, (HMENU)(INT_PTR)id,
                          GetModuleHandleW(nullptr), this);
  return hwnd_;
}

DisplayPanel* DisplayPanel::From(HWND h) {
  return (DisplayPanel*)GetWindowLongPtrW(h, GWLP_USERDATA);
}

void DisplayPanel::SetExpr(const std::wstring& s) {
  expr_ = s;
  InvalidateRect(hwnd_, nullptr, FALSE);
}
void DisplayPanel::SetResult(const std::wstring& s, bool error) {
  result_ = s;
  error_ = error;
  InvalidateRect(hwnd_, nullptr, FALSE);
}
void DisplayPanel::SetAltLines(const std::vector<std::wstring>& lines) {
  alt_ = lines;
  InvalidateRect(hwnd_, nullptr, FALSE);
}
void DisplayPanel::SetMemory(bool on) {
  mem_ = on;
  InvalidateRect(hwnd_, nullptr, FALSE);
}
void DisplayPanel::ClearAll() {
  expr_.clear();
  result_.clear();
  error_ = false;
  InvalidateRect(hwnd_, nullptr, FALSE);
}

void DisplayPanel::Paint(HDC) {
  PAINTSTRUCT ps;
  HDC hdc = BeginPaint(hwnd_, &ps);
  Theme::PaintBuffer(hwnd_, ps, [&](Gdiplus::Graphics& g, int w, int h) {
    const Theme::Palette& p = Theme::P();
    Gdiplus::SolidBrush bg(Theme::C(p.bg));
    g.FillRectangle(&bg, 0, 0, w, h);

    // M 记忆徽标
    if (mem_) {
      Gdiplus::RectF chip(0, (float)Theme::S(12), (float)Theme::S(38), (float)Theme::S(22));
      Theme::FillRound(g, chip, Theme::C(p.accent), (float)Theme::S(4));
      Theme::DrawTextCentered(g, L"M", Theme::Font((float)Theme::S(11), true),
                              Theme::C(p.accentText), chip);
    }

    int y = Theme::S(8);
    // 表达式行（右对齐、字号随宽度自适应）
    int exprH = Theme::S(24);
    if (!expr_.empty()) {
      Gdiplus::Font* f = Theme::Font((float)Theme::S(14));
      float tw = Theme::TextWidth(g, expr_, f);
      int size = Theme::S(14);
      while (tw > w - Theme::S(16) && size > Theme::S(9)) {
        size -= Theme::S(1);
        f = Theme::Font((float)size);
        tw = Theme::TextWidth(g, expr_, f);
      }
      Gdiplus::RectF r(0, (float)y, (float)w - Theme::S(12), (float)exprH);
      Gdiplus::StringFormat fmt;
      fmt.SetAlignment(Gdiplus::StringAlignmentFar);
      fmt.SetLineAlignment(Gdiplus::StringAlignmentCenter);
      fmt.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);
      Gdiplus::SolidBrush b(Theme::C(p.textDim));
      g.DrawString(expr_.c_str(), (INT)expr_.size(), f, r, &fmt, &b);
    }
    y += exprH;

    // 结果行
    int resH = Theme::S(56);
    if (!result_.empty()) {
      Gdiplus::Font* f = Theme::Font((float)Theme::S(40), true);
      float tw = Theme::TextWidth(g, result_, f);
      int size = Theme::S(40);
      while (tw > w - Theme::S(16) && size > Theme::S(14)) {
        size -= Theme::S(2);
        f = Theme::Font((float)size, true);
        tw = Theme::TextWidth(g, result_, f);
      }
      COLORREF c = error_ ? (Theme::IsDark() ? RGB(255, 153, 164) : RGB(196, 43, 28)) : p.text;
      Gdiplus::RectF r(0, (float)y, (float)w - Theme::S(12), (float)resH);
      Gdiplus::StringFormat fmt;
      fmt.SetAlignment(Gdiplus::StringAlignmentFar);
      fmt.SetLineAlignment(Gdiplus::StringAlignmentCenter);
      fmt.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);
      Gdiplus::SolidBrush b(Theme::C(c));
      g.DrawString(result_.c_str(), (INT)result_.size(), f, r, &fmt, &b);
    }
    y += resH;

    // 次要信息行
    for (const auto& line : alt_) {
      Gdiplus::Font* f = Theme::Font((float)Theme::S(12));
      Gdiplus::RectF r(0, (float)y, (float)w - Theme::S(12), (float)Theme::S(22));
      Gdiplus::StringFormat fmt;
      fmt.SetAlignment(Gdiplus::StringAlignmentFar);
      fmt.SetLineAlignment(Gdiplus::StringAlignmentCenter);
      fmt.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);
      Gdiplus::SolidBrush b(Theme::C(p.textDim));
      g.DrawString(line.c_str(), (INT)line.size(), f, r, &fmt, &b);
      y += Theme::S(22);
    }
  });
  EndPaint(hwnd_, &ps);
}

LRESULT CALLBACK DisplayPanel::WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
  DisplayPanel* d = DisplayPanel::From(h);
  if (m == WM_NCCREATE) {
    d = (DisplayPanel*)((CREATESTRUCTW*)l)->lpCreateParams;
    SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)d);
    if (d) d->hwnd_ = h;
    return DefWindowProcW(h, m, w, l);
  }
  if (!d) return DefWindowProcW(h, m, w, l);
  switch (m) {
    case WM_PAINT:
      d->Paint(nullptr);
      return 0;
    case WM_ERASEBKGND:
      return 1;
  }
  return DefWindowProcW(h, m, w, l);
}
