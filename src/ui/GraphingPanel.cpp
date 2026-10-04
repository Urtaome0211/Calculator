#include "GraphingPanel.h"
#include "Theme.h"
#include "CtrlButton.h"
#include "DisplayPanel.h"
#include "../core/ExpressionEngine.h"
#include "../core/NumberFormat.h"
#include <limits>
// ============================================================
// 图形模式实现：最多 3 条函数曲线，滚轮缩放、拖拽平移、双击复位
// 画布为自定义子窗口类 W11GraphCanvas，绘制用 Theme::PaintBuffer 双缓冲
// ============================================================

// ------------------------------------------------------------
// 创建：三条方程输入行 + 开关 + 绘图/重置按钮 + 画布
// ------------------------------------------------------------
HWND GraphingPanel::Create(HWND parent, HINSTANCE inst, const AppContext& ctx) {
  ctx_ = ctx;
  // 面板窗口：由 ModePanel::PanelProc 分发消息（ClassName 已由 ModePanel.cpp 注册）
  hwnd_ = CreateWindowExW(0, ModePanel::ClassName(), L"graph", WS_CHILD | WS_VISIBLE,
                          0, 0, 10, 10, parent, nullptr, inst, this);
  if (!hwnd_) return nullptr;

  // 三条方程输入行：开关按钮 + 扁平编辑框
  const wchar_t* defs[3] = {L"x^2", L"sin(x)", L""};
  for (int i = 0; i < 3; i++) {
    eqs_[i].on = (i == 0);                       // 默认只启用第一条
    eqs_[i].color = Theme::P().curve[i];
    eqs_[i].edit = CreateWindowExW(0, L"EDIT", defs[i],
                                   WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
                                   0, 0, 10, 10, hwnd_,
                                   (HMENU)(INT_PTR)(IDK_GR_EDIT1 + i), inst, nullptr);
    if (eqs_[i].edit) {
      SendMessageW(eqs_[i].edit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN,
                   MAKELPARAM(Theme::S(10), Theme::S(10)));
      SendMessageW(eqs_[i].edit, WM_SETFONT, (WPARAM)Theme::FontH(14), TRUE);
    }
    // 开关按钮：子控件 ID 即命令 ID，OnSize/OnCommand 里用 GetDlgItem 获取
    CtrlButton* tog = CtrlButton::Create(hwnd_, std::wstring(1, (wchar_t)(L'1' + i)),
                                         IDK_GR_TOG1 + i, Theme::Kind::Toggle, true);
    if (tog) tog->SetChecked(eqs_[i].on);
  }

  // 绘图 / 重置视图按钮（HWND 由 GetDlgItem(hwnd_, IDK_GR_PLOT/IDK_GR_RESET) 获取）
  CtrlButton::Create(hwnd_, L"绘图", IDK_GR_PLOT, Theme::Kind::Accent);
  CtrlButton::Create(hwnd_, L"重置视图", IDK_GR_RESET, Theme::Kind::Function);

  // 注册画布专属类（进程内仅一次；CS_DBLCLKS 以接收 WM_LBUTTONDBLCLK）
  static bool canvasRegistered = false;
  if (!canvasRegistered) {
    WNDCLASSW wc = {};
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = GraphingPanel::CanvasProc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = L"W11GraphCanvas";
    RegisterClassW(&wc);
    canvasRegistered = true;
  }
  canvas_ = CreateWindowExW(0, L"W11GraphCanvas", L"", WS_CHILD | WS_VISIBLE,
                            0, 0, 10, 10, hwnd_, nullptr, inst, this);
  return hwnd_;
}

// ------------------------------------------------------------
// 布局：三行输入 + 按钮行 + 画布（全部用 Theme::S 缩放）
// ------------------------------------------------------------
void GraphingPanel::OnSize(int w, int h) {
  const int margin = Theme::S(10);
  const int togW = Theme::S(36), togH = Theme::S(32);
  const int rowH = Theme::S(40), gap = Theme::S(4);
  const int editX = margin + togW + Theme::S(8);

  for (int i = 0; i < 3; i++) {
    const int y = margin + i * (rowH + gap);
    HWND tog = GetDlgItem(hwnd_, IDK_GR_TOG1 + i);
    if (tog) MoveWindow(tog, margin, y + (rowH - togH) / 2, togW, togH, TRUE);
    if (eqs_[i].edit)
      MoveWindow(eqs_[i].edit, editX, y + (rowH - togH) / 2,
                 w - editX - margin, togH, TRUE);
  }

  // 按钮行：绘图 / 重置视图
  const int btnY = margin + 3 * (rowH + gap) + gap;
  const int btnW = Theme::S(110), btnH = Theme::S(40);
  HWND plot = GetDlgItem(hwnd_, IDK_GR_PLOT);
  HWND reset = GetDlgItem(hwnd_, IDK_GR_RESET);
  if (plot) MoveWindow(plot, margin, btnY, btnW, btnH, TRUE);
  if (reset) MoveWindow(reset, margin + btnW + Theme::S(8), btnY, btnW, btnH, TRUE);

  // 画布：占剩余全部空间，底部边距 S(8)
  if (canvas_) {
    const int canvasY = btnY + btnH + Theme::S(8);
    int cw = w, ch = h - canvasY - Theme::S(8);
    if (cw < 10) cw = 10;
    if (ch < 10) ch = 10;
    MoveWindow(canvas_, 0, canvasY, cw, ch, TRUE);
    InvalidateRect(canvas_, nullptr, TRUE);      // 尺寸变化后按新尺寸重绘
  }
}

void GraphingPanel::OnShow() { Replot(); }

// ------------------------------------------------------------
// 面板级消息：仅补 EDIT/STATIC 的背景/文字颜色，其余交基类
// ------------------------------------------------------------
LRESULT GraphingPanel::HandleMessage(UINT msg, WPARAM w, LPARAM l) {
  switch (msg) {
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORSTATIC: {
      SetBkColor((HDC)w, Theme::P().card);
      SetTextColor((HDC)w, Theme::P().text);
      return (LRESULT)Theme::CardBrush();
    }
    default:
      break;
  }
  return ModePanel::HandleMessage(msg, w, l);
}

bool GraphingPanel::OnCommand(int id) {
  switch (id) {
    case IDK_GR_PLOT:
      Replot();
      return true;
    case IDK_GR_RESET:
      viewSet_ = false;
      Replot();
      return true;
    case IDK_GR_TOG1:
    case IDK_GR_TOG2:
    case IDK_GR_TOG3: {
      const int i = id - IDK_GR_TOG1;
      eqs_[i].on = !eqs_[i].on;
      HWND togHwnd = GetDlgItem(hwnd_, id);
      CtrlButton* tog = togHwnd ? CtrlButton::From(togHwnd) : nullptr;
      if (tog) tog->SetChecked(eqs_[i].on);
      Replot();
      return true;
    }
    default:
      break;
  }
  return ModePanel::OnCommand(id);
}

// ------------------------------------------------------------
// 重新求值并重绘：对每条启用方程采样 kSamples 个点，自动适配 y 范围
// ------------------------------------------------------------
void GraphingPanel::Replot() {
  // 采样：x 在 [xmin_,xmax_] 上均匀取 kSamples 个点，逐点 EvaluateX（弧度制）
  for (int i = 0; i < 3; i++) {
    ok_[i] = false;
    if (!eqs_[i].on || !eqs_[i].edit) continue;
    wchar_t buf[1024];
    GetWindowTextW(eqs_[i].edit, buf, 1024);
    const std::wstring expr(buf);
    if (expr.empty()) continue;
    bool any = false;
    std::wstring err;
    for (int j = 0; j < kSamples; j++) {
      const double x = xmin_ + (xmax_ - xmin_) * (double)j / (double)(kSamples - 1);
      double v = 0.0;
      if (Expr::EvaluateX(expr, x, v, err, Expr::AngleMode::Rad)) {
        ys_[i][j] = v;
        any = true;
      } else {
        ys_[i][j] = std::numeric_limits<double>::quiet_NaN();  // 定义域外 → 断线
      }
    }
    ok_[i] = any;   // 至少一个采样点成功即有效；全部失败（含空表达式）判无效
  }

  // 未手动调整视口时自动适配 y 范围（忽略 NaN/Inf，上下各留 10% 余量）
  if (!viewSet_) {
    double lo = 0.0, hi = 0.0;
    bool any = false;
    for (int i = 0; i < 3; i++) {
      if (!ok_[i]) continue;
      for (int j = 0; j < kSamples; j++) {
        const double v = ys_[i][j];
        if (!std::isfinite(v)) continue;
        if (!any) { lo = hi = v; any = true; }
        else { lo = std::min(lo, v); hi = std::max(hi, v); }
      }
    }
    if (!any) { ymin_ = -10; ymax_ = 10; }        // 无有效数据
    else {
      const double pad = (hi - lo) * 0.10;
      ymin_ = lo - pad;
      ymax_ = hi + pad;
      if (ymin_ == ymax_) { ymin_ -= 1; ymax_ += 1; }  // 常数函数
    }
  }

  // 更新显示区：表达式行 + 无效提示
  if (ctx_.display) {
    std::wstring joined, bad;
    for (int i = 0; i < 3; i++) {
      if (!eqs_[i].on || !eqs_[i].edit) continue;
      wchar_t buf[1024];
      GetWindowTextW(eqs_[i].edit, buf, 1024);
      if (!joined.empty()) joined += L" ; ";
      joined += buf;
      if (!ok_[i] && bad.empty()) bad = buf;
    }
    ctx_.display->SetExpr(L"y = " + joined);
    if (!bad.empty()) ctx_.display->SetResult(L"输入无效: " + bad, true);
    else ctx_.display->SetResult(L"", false);
  }

  if (canvas_) InvalidateRect(canvas_, nullptr, FALSE);
}

// ------------------------------------------------------------
// 画布绘制：Theme::PaintBuffer 双缓冲（BeginPaint 在 CanvasProc 的 WM_PAINT 中）
// ------------------------------------------------------------
void GraphingPanel::PaintCanvas(HDC dc) {
  RECT rc;
  GetClientRect(canvas_, &rc);
  const int cw = rc.right - rc.left, ch = rc.bottom - rc.top;
  if (cw <= 0 || ch <= 0) return;
  PAINTSTRUCT ps = {};
  ps.hdc = dc;
  ps.rcPaint = rc;
  Theme::PaintBuffer(canvas_, ps, [this](Gdiplus::Graphics& g, int w, int h) {
    const auto& pal = Theme::P();
    // 绘图区：左 S(56)、右 S(16)、上 S(28)、下 S(44)
    const float plotLeft = (float)Theme::S(56);
    const float plotTop = (float)Theme::S(28);
    const float plotRight = (float)(w - Theme::S(16));
    const float plotBottom = (float)(h - Theme::S(44));
    const float plotW = plotRight - plotLeft;
    const float plotH = plotBottom - plotTop;
    if (plotW <= 0 || plotH <= 0 || !(xmax_ > xmin_) || !(ymax_ > ymin_)) return;

    // 背景
    g.Clear(Theme::C(pal.card));

    // 网格：横向 5 等分、纵向 8 等分，线宽 1
    Gdiplus::Pen gridPen(Theme::C(pal.grid), 1.0f);
    for (int i = 0; i <= 8; i++) {
      const float x = plotLeft + plotW * i / 8.0f;
      g.DrawLine(&gridPen, x, plotTop, x, plotBottom);
    }
    for (int j = 0; j <= 5; j++) {
      const float y = plotTop + plotH * j / 5.0f;
      g.DrawLine(&gridPen, plotLeft, y, plotRight, y);
    }

    // 绘图区边框 1px
    Gdiplus::Pen borderPen(Theme::C(pal.border), 1.0f);
    g.DrawRectangle(&borderPen, plotLeft, plotTop, plotW, plotH);

    // 刻度标签：x 轴 8 个分点（绘图区下方居中）、y 轴 5 个分点（左侧右对齐）
    Gdiplus::Font* labFont = Theme::Font((float)Theme::S(11));
    Gdiplus::SolidBrush dimBrush(Theme::C(pal.textDim));
    Gdiplus::StringFormat rightSF;
    rightSF.SetAlignment(Gdiplus::StringAlignmentFar);
    rightSF.SetLineAlignment(Gdiplus::StringAlignmentCenter);
    for (int i = 0; i <= 8; i++) {
      const double xv = xmin_ + (xmax_ - xmin_) * i / 8.0;
      const Gdiplus::RectF r(plotLeft + plotW * i / 8.0f - (float)Theme::S(20),
                             plotBottom + (float)Theme::S(4),
                             (float)Theme::S(40), (float)Theme::S(16));
      Theme::DrawTextCentered(g, NumFmt::FormatCompact(xv), labFont, Theme::C(pal.textDim), r);
    }
    for (int j = 0; j <= 5; j++) {
      const double yv = ymax_ - (ymax_ - ymin_) * j / 5.0;
      const Gdiplus::RectF r(0.0f, plotTop + plotH * j / 5.0f - (float)Theme::S(8),
                             plotLeft - (float)Theme::S(8), (float)Theme::S(16));
      g.DrawString(NumFmt::FormatCompact(yv).c_str(), -1, labFont, r, &rightSF, &dimBrush);
    }

    // 坐标轴线：0 在可视范围内才画（y=0 横线、x=0 竖线，颜色 border）
    if (ymin_ < 0.0 && ymax_ > 0.0) {
      const float y0 = plotTop + (float)((ymax_ - 0.0) / (ymax_ - ymin_)) * plotH;
      g.DrawLine(&borderPen, plotLeft, y0, plotRight, y0);
    }
    if (xmin_ < 0.0 && xmax_ > 0.0) {
      const float x0 = plotLeft + (float)((0.0 - xmin_) / (xmax_ - xmin_)) * plotW;
      g.DrawLine(&borderPen, x0, plotTop, x0, plotBottom);
    }

    // 曲线：折线连接相邻有限点（前后都有限才连线），裁剪到绘图区
    g.SetClip(Gdiplus::RectF(plotLeft, plotTop, plotW, plotH));
    for (int i = 0; i < 3; i++) {
      if (!ok_[i]) continue;
      const COLORREF c = eqs_[i].color;
      Gdiplus::Pen pen(Gdiplus::Color(255, GetRValue(c), GetGValue(c), GetBValue(c)), 2.5f);
      bool hasPrev = false;
      float prevX = 0.0f, prevY = 0.0f;
      for (int j = 0; j < kSamples; j++) {
        const double v = ys_[i][j];
        if (!std::isfinite(v)) { hasPrev = false; continue; }
        const double x = xmin_ + (xmax_ - xmin_) * (double)j / (double)(kSamples - 1);
        const float px = plotLeft + (float)((x - xmin_) / (xmax_ - xmin_)) * plotW;
        const float py = plotTop + (float)((ymax_ - v) / (ymax_ - ymin_)) * plotH;
        if (hasPrev) g.DrawLine(&pen, prevX, prevY, px, py);
        prevX = px; prevY = py; hasPrev = true;
      }
    }
    g.ResetClip();

    // 图例：左上角（边距 S(12)），色线 + 方程文本
    Gdiplus::Font* legFont = Theme::Font((float)Theme::S(11));
    Gdiplus::StringFormat legendSF;
    legendSF.SetFormatFlags(Gdiplus::StringFormatFlagsNoClip);
    legendSF.SetLineAlignment(Gdiplus::StringAlignmentCenter);
    float ly = (float)Theme::S(12);
    for (int i = 0; i < 3; i++) {
      if (!eqs_[i].on) continue;
      const COLORREF c = eqs_[i].color;
      Gdiplus::Pen pen(Gdiplus::Color(255, GetRValue(c), GetGValue(c), GetBValue(c)), 2.5f);
      const float lx = (float)Theme::S(12);
      const float lineY = ly + (float)Theme::S(8);
      g.DrawLine(&pen, lx, lineY, lx + (float)Theme::S(22), lineY);
      wchar_t buf[1024] = {};
      if (eqs_[i].edit) GetWindowTextW(eqs_[i].edit, buf, 1024);
      const Gdiplus::RectF r(lx + (float)Theme::S(28), ly,
                             (float)Theme::S(60), (float)Theme::S(16));
      g.DrawString(buf, -1, legFont, r, &legendSF, &dimBrush);
      ly += (float)Theme::S(18);
    }
  });
}

// ------------------------------------------------------------
// 滚轮缩放：像素 cx 对应的 x 值不动，y 用画布垂直中心做锚点
// ------------------------------------------------------------
void GraphingPanel::ZoomAt(int cx, double factor) {
  RECT rc;
  GetClientRect(canvas_, &rc);
  const int w = rc.right - rc.left, h = rc.bottom - rc.top;
  const int plotLeft = Theme::S(56), plotTop = Theme::S(28);
  const int plotRight = w - Theme::S(16), plotBottom = h - Theme::S(44);
  const int plotW = plotRight - plotLeft, plotH = plotBottom - plotTop;
  if (plotW <= 0 || plotH <= 0) return;
  if (cx < plotLeft) cx = plotLeft;              // 光标限制在绘图区内
  if (cx > plotRight) cx = plotRight;
  const double xc = xmin_ + (xmax_ - xmin_) * (double)(cx - plotLeft) / (double)plotW;
  xmin_ = xc - (xc - xmin_) * factor;
  xmax_ = xc + (xmax_ - xc) * factor;
  // y 方向翻转：y 值从下往上增大、ymax_ 对应 top；锚点取画布垂直中心
  const double yc = ymax_ - (ymax_ - ymin_) * 0.5;
  ymin_ = yc - (yc - ymin_) * factor;
  ymax_ = yc + (ymax_ - yc) * factor;
  viewSet_ = true;
  Replot();
}

// ------------------------------------------------------------
// 拖拽平移
// ------------------------------------------------------------
void GraphingPanel::Pan(int dx, int dy) {
  RECT rc;
  GetClientRect(canvas_, &rc);
  const int w = rc.right - rc.left, h = rc.bottom - rc.top;
  const int plotW = w - Theme::S(56) - Theme::S(16);
  const int plotH = h - Theme::S(28) - Theme::S(44);
  if (plotW <= 0 || plotH <= 0) return;
  const double dxv = (double)dx / (double)plotW * (xmax_ - xmin_);
  xmin_ -= dxv;
  xmax_ -= dxv;
  const double dyv = (double)dy / (double)plotH * (ymax_ - ymin_);
  ymin_ += dyv;
  ymax_ += dyv;
  viewSet_ = true;
  Replot();
}

// ------------------------------------------------------------
// 画布窗口过程
// ------------------------------------------------------------
LRESULT CALLBACK GraphingPanel::CanvasProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
  if (msg == WM_NCCREATE) {
    // 创建时把 GraphingPanel 指针存入 GWLP_USERDATA
    SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                      (LONG_PTR)((CREATESTRUCTW*)l)->lpCreateParams);
    return TRUE;
  }
  GraphingPanel* self = (GraphingPanel*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
  if (!self) return DefWindowProcW(hwnd, msg, w, l);
  switch (msg) {
    case WM_PAINT: {
      PAINTSTRUCT ps;
      HDC dc = BeginPaint(hwnd, &ps);
      self->PaintCanvas(dc);
      EndPaint(hwnd, &ps);
      return 0;
    }
    case WM_MOUSEWHEEL: {
      // lParam 为屏幕坐标，先转客户区坐标
      POINT pt = {GET_X_LPARAM(l), GET_Y_LPARAM(l)};
      ScreenToClient(hwnd, &pt);
      const double factor = pow(1.1, -(double)GET_WHEEL_DELTA_WPARAM(w) / 120.0);
      self->ZoomAt(pt.x, factor);
      return 0;
    }
    case WM_LBUTTONDOWN:
      self->dragging_ = true;
      self->lastX_ = GET_X_LPARAM(l);
      self->lastY_ = GET_Y_LPARAM(l);
      SetCapture(hwnd);
      return 0;
    case WM_MOUSEMOVE:
      if (self->dragging_) {
        const int x = GET_X_LPARAM(l), y = GET_Y_LPARAM(l);
        self->Pan(x - self->lastX_, y - self->lastY_);
        self->lastX_ = x;
        self->lastY_ = y;
      }
      return 0;
    case WM_LBUTTONUP:
      self->dragging_ = false;
      ReleaseCapture();
      return 0;
    case WM_LBUTTONDBLCLK:
      self->viewSet_ = false;                    // 双击复位视口（y 自动适配）
      self->Replot();
      return 0;
    case WM_ERASEBKGND:
      return 1;                                  // 双缓冲绘制，跳过背景擦除防闪烁
    default:
      break;
  }
  return DefWindowProcW(hwnd, msg, w, l);
}
