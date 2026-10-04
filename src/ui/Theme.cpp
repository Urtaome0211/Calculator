#include "Theme.h"
#include <map>

// ============================================================
// 主题实现：Windows 11 浅/深配色、字体缓存、GDI+ 绘图辅助
// ============================================================
namespace Theme {
namespace {

const Palette kLight = {
  RGB(243, 243, 243),   // bg
  RGB(255, 255, 255),   // card
  RGB(255, 255, 255),   // num
  RGB(249, 249, 249),   // fn
  RGB(0, 103, 192),     // accent
  RGB(0, 0, 0),         // text
  RGB(87, 87, 87),      // textDim
  RGB(255, 255, 255),   // accentText
  RGB(229, 229, 229),   // border
  RGB(240, 240, 240),   // hoverNum
  RGB(237, 237, 237),   // hoverFn
  RGB(216, 216, 216),   // grid
  {RGB(0, 103, 192), RGB(196, 43, 28), RGB(15, 123, 15)},  // curve
};

const Palette kDark = {
  RGB(32, 32, 32),      // bg
  RGB(43, 43, 43),      // card
  RGB(46, 46, 46),      // num
  RGB(50, 50, 50),      // fn
  RGB(76, 194, 255),    // accent
  RGB(255, 255, 255),   // text
  RGB(158, 158, 158),   // textDim
  RGB(0, 0, 0),         // accentText（深色下强调键用黑字）
  RGB(58, 58, 58),      // border
  RGB(63, 63, 63),      // hoverNum
  RGB(67, 67, 67),      // hoverFn
  RGB(58, 58, 58),      // grid
  {RGB(76, 194, 255), RGB(255, 153, 164), RGB(108, 203, 95)},  // curve
};

bool g_dark = false;
float g_scale = 1.0f;
std::map<std::pair<int, bool>, Gdiplus::Font*> g_fonts;
std::map<std::pair<int, bool>, HFONT> g_hfonts;
std::map<int, Gdiplus::Font*> g_iconFonts;
HBRUSH g_cardBrush = nullptr;

Gdiplus::Font* MakeFont(float size, bool bold, const wchar_t* family) {
  return new Gdiplus::Font(family, size, bold ? Gdiplus::FontStyleBold : Gdiplus::FontStyleRegular,
                           Gdiplus::UnitPixel);
}

void RebuildBrush() {
  if (g_cardBrush) { DeleteObject(g_cardBrush); g_cardBrush = nullptr; }
  g_cardBrush = CreateSolidBrush(P().card);
}

}  // namespace

void Init() {
  HDC dc = GetDC(nullptr);
  int dpi = dc ? GetDeviceCaps(dc, LOGPIXELSX) : 96;
  if (dc) ReleaseDC(nullptr, dc);
  g_scale = dpi / 96.0f;
  // 跟随系统主题（应用模式）
  DWORD light = 1, size = sizeof(light);
  if (RegGetValueW(HKEY_CURRENT_USER,
                   L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                   L"AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, &light, &size) == ERROR_SUCCESS) {
    g_dark = (light == 0);
  }
  RebuildBrush();
}

bool IsDark() { return g_dark; }
void SetDark(bool dark) {
  if (g_dark == dark) return;
  g_dark = dark;
  RebuildBrush();
}

const Palette& P() { return g_dark ? kDark : kLight; }

void ApplyTitlebar(HWND h) {
  BOOL dark = IsDark() ? TRUE : FALSE;
  DwmSetWindowAttribute(h, 20 /*DWMWA_USE_IMMERSIVE_DARK_MODE*/, &dark, sizeof(dark));
  // Win11 圆角窗口
  int corner = 2;  // DWMWCP_ROUND
  DwmSetWindowAttribute(h, 33 /*DWMWA_WINDOW_CORNER_PREFERENCE*/, &corner, sizeof(corner));
}

float Scale() { return g_scale; }
int S(int px) { return (int)(px * g_scale + 0.5f); }

Gdiplus::Color C(COLORREF c) {
  return Gdiplus::Color(GetRValue(c), GetGValue(c), GetBValue(c));
}

Gdiplus::Font* Font(float size, bool bold) {
  auto key = std::make_pair((int)(size * 10 + 0.5f), bold);
  auto it = g_fonts.find(key);
  if (it != g_fonts.end()) return it->second;
  Gdiplus::Font* f = MakeFont(size, bold, L"Segoe UI Variable Display");
  g_fonts[key] = f;
  return f;
}

Gdiplus::Font* IconFont(float size) {
  int key = (int)(size * 10 + 0.5f);
  auto it = g_iconFonts.find(key);
  if (it != g_iconFonts.end()) return it->second;
  Gdiplus::Font* f = MakeFont(size, false, L"Segoe MDL2 Assets");
  g_iconFonts[key] = f;
  return f;
}

HFONT FontH(int size, bool bold) {
  auto key = std::make_pair(size, bold);
  auto it = g_hfonts.find(key);
  if (it != g_hfonts.end()) return it->second;
  HFONT f = CreateFontW(-size * 96 / 72, 0, 0, 0, bold ? FW_SEMIBOLD : FW_NORMAL, FALSE, FALSE,
                        FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                        L"Segoe UI Variable Display");
  g_hfonts[key] = f;
  return f;
}

HBRUSH CardBrush() { return g_cardBrush; }

void ApplyNativeDark(HWND ctrl) {
  // 让原生 EDIT / COMBOBOX 在深色主题下使用深色风格
  SetWindowTheme(ctrl, IsDark() ? L"DarkMode_Explorer" : nullptr, nullptr);
}

Gdiplus::Color BtnColor(Kind k, bool hover, bool down) {
  const Palette& p = P();
  switch (k) {
    case Kind::Accent:
      if (down) return IsDark() ? C(RGB(64, 170, 226)) : C(RGB(0, 90, 168));
      if (hover) return IsDark() ? C(RGB(89, 203, 255)) : C(RGB(0, 116, 205));
      return C(p.accent);
    case Kind::Function:
      if (down) return C(p.border);
      if (hover) return C(p.hoverFn);
      return C(p.fn);
    case Kind::Memory:
      if (down || hover) return C(p.hoverNum);
      return Gdiplus::Color(0, 0, 0, 0);   // 透明
    case Kind::Toggle:
      return C(p.card);
    default:  // Number
      if (down) return C(p.border);
      if (hover) return C(p.hoverNum);
      return C(p.num);
  }
}

void FillRound(Gdiplus::Graphics& g, const Gdiplus::RectF& r, const Gdiplus::Color& c, float radius) {
  Gdiplus::GraphicsPath path;
  path.AddArc(r.X, r.Y, radius * 2, radius * 2, 180, 90);
  path.AddArc(r.X + r.Width - radius * 2, r.Y, radius * 2, radius * 2, 270, 90);
  path.AddArc(r.X + r.Width - radius * 2, r.Y + r.Height - radius * 2, radius * 2, radius * 2, 0, 90);
  path.AddArc(r.X, r.Y + r.Height - radius * 2, radius * 2, radius * 2, 90, 90);
  path.CloseFigure();
  Gdiplus::SolidBrush b(c);
  g.FillPath(&b, &path);
}

void DrawTextCentered(Gdiplus::Graphics& g, const std::wstring& s, Gdiplus::Font* f,
                      const Gdiplus::Color& c, const Gdiplus::RectF& r) {
  Gdiplus::StringFormat fmt;
  fmt.SetAlignment(Gdiplus::StringAlignmentCenter);
  fmt.SetLineAlignment(Gdiplus::StringAlignmentCenter);
  fmt.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);
  Gdiplus::SolidBrush b(c);
  g.DrawString(s.c_str(), (INT)s.size(), f, r, &fmt, &b);
}

float TextWidth(Gdiplus::Graphics& g, const std::wstring& s, Gdiplus::Font* f) {
  Gdiplus::RectF box;
  g.MeasureString(s.c_str(), (INT)s.size(), f, Gdiplus::PointF(0, 0), &box);
  return box.Width;
}

void PaintBuffer(HWND h, PAINTSTRUCT& ps, const std::function<void(Gdiplus::Graphics&, int, int)>& fn) {
  RECT rc;
  GetClientRect(h, &rc);
  int w = rc.right - rc.left, hgt = rc.bottom - rc.top;
  if (w <= 0 || hgt <= 0) return;
  HDC mem = CreateCompatibleDC(ps.hdc);
  HBITMAP bmp = CreateCompatibleBitmap(ps.hdc, w, hgt);
  HGDIOBJ old = SelectObject(mem, bmp);
  {
    Gdiplus::Graphics g(mem);
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);
    fn(g, w, hgt);
  }
  BitBlt(ps.hdc, 0, 0, w, hgt, mem, 0, 0, SRCCOPY);
  SelectObject(mem, old);
  DeleteObject(bmp);
  DeleteDC(mem);
}

}  // namespace Theme
