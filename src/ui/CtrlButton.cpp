#include "CtrlButton.h"

// ============================================================
// 自定义按钮：Win11 圆角按键 + 悬停/按下反馈 + 切换高亮
// ============================================================
namespace {
bool g_registered = false;
}

CtrlButton* CtrlButton::Create(HWND parent, const std::wstring& label, int id,
                               Theme::Kind kind, bool small) {
  if (!g_registered) {
    WNDCLASSW wc = {};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = CtrlButton::WndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"W11Btn";
    wc.hCursor = LoadCursorW(nullptr, IDC_HAND);
    RegisterClassW(&wc);
    g_registered = true;
  }
  auto* btn = new CtrlButton();
  btn->label_ = label;
  btn->id_ = id;
  btn->kind_ = kind;
  btn->small_ = small;
  btn->hwnd_ = CreateWindowExW(0, L"W11Btn", L"", WS_CHILD | WS_VISIBLE,
                               0, 0, 40, 40, parent, (HMENU)(INT_PTR)id,
                               GetModuleHandleW(nullptr), btn);
  if (!btn->hwnd_) { delete btn; return nullptr; }
  return btn;
}

CtrlButton* CtrlButton::From(HWND h) {
  return (CtrlButton*)GetWindowLongPtrW(h, GWLP_USERDATA);
}

void CtrlButton::SetLabel(const std::wstring& label) {
  label_ = label;
  InvalidateRect(hwnd_, nullptr, FALSE);
}

void CtrlButton::SetChecked(bool checked) {
  checked_ = checked;
  InvalidateRect(hwnd_, nullptr, FALSE);
}

void CtrlButton::SetEnabled(bool enabled) {
  EnableWindow(hwnd_, enabled ? TRUE : FALSE);
  InvalidateRect(hwnd_, nullptr, FALSE);
}

void CtrlButton::SetKind(Theme::Kind k) {
  kind_ = k;
  InvalidateRect(hwnd_, nullptr, FALSE);
}

bool IsGlyph(wchar_t c) { return c >= 0xE700 && c <= 0xF8FF; }

void CtrlButton::Paint(HDC) {
  PAINTSTRUCT ps;
  HDC hdc = BeginPaint(hwnd_, &ps);
  Theme::PaintBuffer(hwnd_, ps, [&](Gdiplus::Graphics& g, int w, int h) {
    const Theme::Palette& p = Theme::P();
    bool enabled = IsWindowEnabled(hwnd_) ? true : false;
    bool checked = checked_;
    // 填充
    Gdiplus::Color fill = Theme::BtnColor(kind_, hover_ && enabled, down_);
    if (kind_ == Theme::Kind::Toggle && checked) fill = Theme::C(p.accent);
    if (kind_ == Theme::Kind::Memory && !hover_ && !down_) fill = Gdiplus::Color(0, 0, 0, 0);
    if (fill.GetA() > 0)
      Theme::FillRound(g, Gdiplus::RectF(0.5f, 0.5f, (float)w - 1, (float)h - 1), fill, (float)Theme::S(4));
    else {
      // 透明背景：填充父背景色防止残影（由 WM_ERASEBKGND 已保证，这里补一次）
      Gdiplus::SolidBrush bg(Theme::C(p.bg));
      g.FillRectangle(&bg, 0, 0, w, h);
    }
    // 按钮边框（浅色主题数字/功能键的细边框）
    if (!Theme::IsDark() && kind_ != Theme::Kind::Memory && kind_ != Theme::Kind::Accent && !checked) {
      Gdiplus::Pen pen(Theme::C(p.border), 1.0f);
      Gdiplus::RectF r(0.5f, 0.5f, (float)w - 1.5f, (float)h - 1.5f);
      // 画圆角边框
      Gdiplus::GraphicsPath path;
      float rad = (float)Theme::S(4);
      path.AddArc(r.X, r.Y, rad * 2, rad * 2, 180, 90);
      path.AddArc(r.X + r.Width - rad * 2, r.Y, rad * 2, rad * 2, 270, 90);
      path.AddArc(r.X + r.Width - rad * 2, r.Y + r.Height - rad * 2, rad * 2, rad * 2, 0, 90);
      path.AddArc(r.X, r.Y + r.Height - rad * 2, rad * 2, rad * 2, 90, 90);
      path.CloseFigure();
      g.DrawPath(&pen, &path);
    }
    // 文字
    Gdiplus::Color textC;
    if (!enabled) textC = Theme::C(p.textDim);
    else if (checked) textC = Theme::C(p.accentText);
    else if (kind_ == Theme::Kind::Accent) textC = Theme::C(p.accentText);
    else if (kind_ == Theme::Kind::Memory) textC = Theme::C(p.textDim);
    else textC = Theme::C(p.text);
    bool glyph = !label_.empty() && IsGlyph(label_[0]);
    Gdiplus::Font* f = glyph ? Theme::IconFont((float)Theme::S(small_ ? 12 : 17))
                             : Theme::Font((float)Theme::S(small_ ? 12 : 17), false);
    Gdiplus::RectF r(0, 0, (float)w, (float)h);
    if (down_) r.Y += 1;
    Theme::DrawTextCentered(g, label_, f, textC, r);
  });
  EndPaint(hwnd_, &ps);
}

LRESULT CALLBACK CtrlButton::WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
  CtrlButton* b = CtrlButton::From(h);
  if (m == WM_NCCREATE) {
    b = (CtrlButton*)((CREATESTRUCTW*)l)->lpCreateParams;
    SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)b);
    if (b) b->hwnd_ = h;
    return DefWindowProcW(h, m, w, l);
  }
  if (!b) return DefWindowProcW(h, m, w, l);
  switch (m) {
    case WM_PAINT:
      b->Paint(nullptr);
      return 0;
    case WM_ERASEBKGND:
      return 1;
    case WM_LBUTTONDOWN:
      b->down_ = true;
      SetCapture(h);
      InvalidateRect(h, nullptr, FALSE);
      return 0;
    case WM_LBUTTONUP: {
      BOOL wasDown = b->down_;
      b->down_ = false;
      ReleaseCapture();
      InvalidateRect(h, nullptr, FALSE);
      if (wasDown) {
        POINT pt;
        GetCursorPos(&pt);
        RECT rc;
        GetWindowRect(h, &rc);
        if (PtInRect(&rc, pt)) {
          if (b->kind_ == Theme::Kind::Toggle) b->checked_ = !b->checked_;
          SendMessageW(GetParent(h), WM_COMMAND, MAKEWPARAM(b->id_, BN_CLICKED), (LPARAM)h);
        }
      }
      return 0;
    }
    case WM_MOUSEMOVE: {
      if (!b->hover_) {
        b->hover_ = true;
        TRACKMOUSEEVENT tme = {sizeof(tme), TME_LEAVE, h, 0};
        TrackMouseEvent(&tme);
        InvalidateRect(h, nullptr, FALSE);
      }
      return 0;
    }
    case WM_MOUSELEAVE:
      b->hover_ = false;
      InvalidateRect(h, nullptr, FALSE);
      return 0;
    case WM_SETFOCUS:
      return 0;   // 按钮不抢占焦点，键盘路由由主窗口处理
    case WM_ENABLE:
      InvalidateRect(h, nullptr, FALSE);
      return 0;
    case WM_SETCURSOR:
      SetCursor(LoadCursorW(nullptr, IsWindowEnabled(h) ? IDC_HAND : IDC_ARROW));
      return TRUE;
  }
  return DefWindowProcW(h, m, w, l);
}
