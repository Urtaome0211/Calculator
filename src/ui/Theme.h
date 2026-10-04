#pragma once
#include "Common.h"
#include <gdiplus.h>
// ============================================================
// 主题：Windows 11 风格配色 / 字体 / GDI+ 绘图辅助 / DPI 缩放
// ============================================================
namespace Theme {
  enum class Kind { Number, Function, Accent, Memory, Toggle };

  struct Palette {
    COLORREF bg, card, num, fn, accent, text, textDim, accentText, border;
    COLORREF hoverNum, hoverFn, grid;
    COLORREF curve[3];                       // 图形模式三条曲线颜色
  };

  void Init();
  bool IsDark();
  void SetDark(bool dark);
  const Palette& P();
  void ApplyTitlebar(HWND h);                // 沉浸式深色标题栏

  float Scale();                             // DPI 缩放系数
  int S(int px);                             // 像素 × 缩放

  Gdiplus::Color C(COLORREF c);              // COLORREF -> GDI+ Color
  Gdiplus::Font* Font(float size, bool bold = false);
  Gdiplus::Font* IconFont(float size);       // Segoe MDL2 Assets 图标字体
  HFONT FontH(int size, bool bold = false);  // 原生控件用 HFONT
  HBRUSH CardBrush();                        // 原生控件背景刷（跟随主题）
  void ApplyNativeDark(HWND ctrl);           // 给 EDIT/COMBOBOX 套深色主题
  Gdiplus::Color BtnColor(Kind k, bool hover, bool down);

  void FillRound(Gdiplus::Graphics& g, const Gdiplus::RectF& r, const Gdiplus::Color& c, float radius);
  void DrawTextCentered(Gdiplus::Graphics& g, const std::wstring& s, Gdiplus::Font* f,
                        const Gdiplus::Color& c, const Gdiplus::RectF& r);
  float TextWidth(Gdiplus::Graphics& g, const std::wstring& s, Gdiplus::Font* f);
  // 双缓冲绘制：回调内用 Graphics 绘制，结束后一次性贴到屏幕
  void PaintBuffer(HWND h, PAINTSTRUCT& ps, const std::function<void(Gdiplus::Graphics&, int, int)>& fn);
}
