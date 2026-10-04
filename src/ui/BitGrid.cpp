// ============================================================
// BitGrid.cpp —— 程序员模式 64 位位图网格控件实现
// 布局：每行 16 个方格，最高位在最左、最低位在最右，行间位号递减；
// 点击方格翻转对应位；悬停高亮；底部每 4 位一组标注组内最高位序号。
// 仅依赖 BitGrid.h / Theme.h / Common.h，不引用其他项目文件。
// ============================================================
#include "BitGrid.h"
#include "Theme.h"

namespace {
// 网格布局参数（全部像素尺寸经 Theme::S 按 DPI 缩放）
struct Layout {
  int left = 0;     // 左/右边距
  int top = 0;      // 上边距
  int gap = 0;      // 方格间距
  int labelH = 0;   // 底部标签区高度
  int rows = 0;     // 行数
  float cellW = 0;  // 方格宽度
  float cellH = 0;  // 方格高度
};

Layout CalcLayout(int w, int h, unsigned bits) {
  Layout L;
  L.left = Theme::S(6);
  L.top = Theme::S(4);
  L.gap = Theme::S(4);
  L.labelH = Theme::S(18);
  L.rows = (int)(bits / 16);
  if (L.rows < 1) L.rows = 1;
  L.cellW = (float)(w - 2 * L.left - 15 * L.gap) / 16.0f;
  L.cellH = (float)(h - L.top - L.labelH - (L.rows - 1) * L.gap) / (float)L.rows;
  return L;
}

// 位号 i（0 为最低位）所在的行列：最高位在 0 行 0 列，行内从左到右位号递减
void CellPos(unsigned bits, int i, int& row, int& col) {
  int pos = (int)(bits - 1) - i;  // 从最高位起算的序号
  row = pos / 16;
  col = pos % 16;
}
}  // namespace

// ------------------------------------------------------------
// 创建子窗口：注册专属窗口类（仅一次），GWLP_USERDATA 存 this
// ------------------------------------------------------------
HWND BitGrid::Create(HWND parent, int id, const std::function<void()>& onToggle) {
  static bool registered = false;
  if (!registered) {
    WNDCLASSW wc = {};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = BitGrid::WndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"W11BitGrid";
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    RegisterClassW(&wc);
    registered = true;
  }
  onToggle_ = onToggle;
  hwnd_ = CreateWindowExW(0, L"W11BitGrid", L"", WS_CHILD | WS_VISIBLE,
                          0, 0, 0, 0, parent, (HMENU)(INT_PTR)id,
                          GetModuleHandleW(nullptr), this);
  return hwnd_;
}

// ------------------------------------------------------------
// 从窗口句柄取回控件对象
// ------------------------------------------------------------
BitGrid* BitGrid::From(HWND h) {
  return (BitGrid*)GetWindowLongPtrW(h, GWLP_USERDATA);
}

// ------------------------------------------------------------
// 窗口过程
// ------------------------------------------------------------
LRESULT CALLBACK BitGrid::WndProc(HWND h, UINT msg, WPARAM w, LPARAM l) {
  if (msg == WM_NCCREATE) {
    // 创建时从 lpCreateParams 取出 this 并存入 GWLP_USERDATA
    BitGrid* self = (BitGrid*)((CREATESTRUCTW*)l)->lpCreateParams;
    self->hwnd_ = h;
    SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)self);
    return TRUE;
  }
  BitGrid* self = (BitGrid*)GetWindowLongPtrW(h, GWLP_USERDATA);
  if (!self) return DefWindowProcW(h, msg, w, l);

  switch (msg) {
    case WM_PAINT:
      self->Paint((HDC)w);
      return 0;

    case WM_ERASEBKGND:
      return 1;  // 背景在 PaintBuffer 回调里自行填充，禁止系统擦除闪烁

    case WM_MOUSEMOVE: {
      // 注册离开通知，鼠标移出窗口时清除悬停
      TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, h, 0 };
      TrackMouseEvent(&tme);
      int idx = self->HitTest(GET_X_LPARAM(l), GET_Y_LPARAM(l));
      if (idx != self->hoverIdx_) {
        self->hoverIdx_ = idx;
        InvalidateRect(h, nullptr, FALSE);
      }
      return 0;
    }

    case WM_MOUSELEAVE:
      if (self->hoverIdx_ != -1) {
        self->hoverIdx_ = -1;
        InvalidateRect(h, nullptr, FALSE);
      }
      return 0;

    case WM_LBUTTONDOWN: {
      int idx = self->HitTest(GET_X_LPARAM(l), GET_Y_LPARAM(l));
      if (idx >= 0) self->Toggle(idx);
      return 0;
    }
  }
  return DefWindowProcW(h, msg, w, l);
}

// ------------------------------------------------------------
// 绘制：BeginPaint + Theme::PaintBuffer 双缓冲，回调内用 GDI+ 绘制
// ------------------------------------------------------------
void BitGrid::Paint(HDC /*dc*/) {
  PAINTSTRUCT ps;
  BeginPaint(hwnd_, &ps);
  Theme::PaintBuffer(hwnd_, ps, [this](Gdiplus::Graphics& g, int w, int h) {
    // 1) 窗口背景
    Gdiplus::SolidBrush bg(Theme::C(Theme::P().card));
    g.FillRectangle(&bg, 0, 0, w, h);

    Layout L = CalcLayout(w, h, bits_);
    if (L.cellW <= 0 || L.cellH <= 0) return;

    // 2) 方格：只绘制 bits_ 个有效位，其余留白
    for (int i = 0; i < (int)bits_; ++i) {
      int row, col;
      CellPos(bits_, i, row, col);
      float x = (float)L.left + col * (L.cellW + L.gap);
      float y = (float)L.top + row * (L.cellH + L.gap);
      Gdiplus::RectF r(x, y, L.cellW, L.cellH);
      Gdiplus::Color c;
      if (i == hoverIdx_) {
        c = Theme::C(Theme::P().hoverNum);          // 悬停叠加：浅/深色主题都用 hoverNum
      } else if (value_ & (1ull << i)) {
        c = Theme::C(Theme::P().accent);            // 置位
      } else {
        c = Theme::C(Theme::P().fn);                // 未置位
      }
      Theme::FillRound(g, r, c, (float)Theme::S(3));
    }

    // 3) 底部标签：每 4 个方格一组，居中标注该组最高位序号
    Gdiplus::Font* f = Theme::Font((float)Theme::S(9));
    Gdiplus::Color labelColor = Theme::C(Theme::P().textDim);
    int groups = ((int)bits_ + 3) / 4;
    for (int gi = 0; gi < groups; ++gi) {
      float x0 = (float)L.left + gi * 4 * (L.cellW + L.gap);
      float gw = 4.0f * L.cellW + 3.0f * L.gap;
      Gdiplus::RectF lr(x0 - (float)L.gap * 0.5f,
                        (float)(h - L.labelH),
                        gw + (float)L.gap,
                        (float)L.labelH);
      std::wstring s = std::to_wstring((int)bits_ - 1 - gi * 4);
      Theme::DrawTextCentered(g, s, f, labelColor, lr);
    }
  });
  EndPaint(hwnd_, &ps);
}

// ------------------------------------------------------------
// 命中检测：把客户区坐标换算成位号；最高位在最左、最低位在最右，
// 行内从左到右位号递减；落在间隙或无效位返回 -1
// ------------------------------------------------------------
int BitGrid::HitTest(int x, int y) const {
  RECT rc;
  GetClientRect(hwnd_, &rc);
  Layout L = CalcLayout(rc.right, rc.bottom, bits_);
  if (L.cellW <= 0 || L.cellH <= 0) return -1;

  int offX = x - L.left;
  int offY = y - L.top;
  if (offX < 0 || offY < 0) return -1;

  int col = (int)(offX / (L.cellW + L.gap));
  int row = (int)(offY / (L.cellH + L.gap));
  if (col < 0 || col > 15 || row < 0 || row >= L.rows) return -1;
  if (offX - col * (L.cellW + L.gap) >= L.cellW) return -1;  // 落在横向间隙
  if (offY - row * (L.cellH + L.gap) >= L.cellH) return -1;  // 落在纵向间隙

  int i = (int)bits_ - 1 - (row * 16 + col);  // 行列 → 位号（最高位在左上角）
  return (i >= 0 && i < (int)bits_) ? i : -1;
}

// ------------------------------------------------------------
// 翻转第 i 位并通知外部
// ------------------------------------------------------------
void BitGrid::Toggle(int i) {
  value_ ^= (1ull << i);
  if (onToggle_) onToggle_();
  InvalidateRect(hwnd_, nullptr, FALSE);
}

// ------------------------------------------------------------
// 设置数值 / 字长（仅 8/16/32/64，其余按 64 处理）
// ------------------------------------------------------------
void BitGrid::SetValue(uint64_t v) {
  value_ = v;
  InvalidateRect(hwnd_, nullptr, FALSE);
}

void BitGrid::SetBits(unsigned bits) {
  bits_ = (bits == 8 || bits == 16 || bits == 32 || bits == 64) ? bits : 64;
  InvalidateRect(hwnd_, nullptr, FALSE);
}
