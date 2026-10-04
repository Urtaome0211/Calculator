#include "MainWindow.h"
#include "Theme.h"
#include "KeypadPanel.h"
#include "StandardPanel.h"
#include "ScientificPanel.h"
#include "ProgrammerPanel.h"
#include "GraphingPanel.h"
#include "DatePanel.h"
#include "ConverterPanel.h"
#include "../core/UnitConverter.h"

// ============================================================
// 主窗口：顶栏 + 显示区 + 模式面板 + 导航浮出菜单 + 历史侧栏
// ============================================================
namespace {

const int kNavBtn = 3001;

struct FlyItem {
  int id;
  std::wstring label;
  bool header;
};

const std::vector<FlyItem>& FlyItems() {
  static const std::vector<FlyItem> items = {
    {0, L"模式", true},
    {ID_NAV_STANDARD, L"标准", false},
    {ID_NAV_SCIENTIFIC, L"科学", false},
    {ID_NAV_GRAPHING, L"图形", false},
    {ID_NAV_PROGRAMMER, L"程序员", false},
    {ID_NAV_DATE, L"日期计算", false},
    {0, L"转换器", true},
    {ID_NAV_CONV_CURRENCY, L"货币", false},
    {ID_NAV_CONV_VOLUME, L"体积", false},
    {ID_NAV_CONV_LENGTH, L"长度", false},
    {ID_NAV_CONV_WEIGHT, L"重量", false},
    {ID_NAV_CONV_TEMP, L"温度", false},
    {ID_NAV_CONV_AREA, L"面积", false},
    {ID_NAV_CONV_SPEED, L"速度", false},
    {ID_NAV_CONV_TIME, L"时间", false},
    {ID_NAV_CONV_ENERGY, L"能量", false},
    {ID_NAV_CONV_PRESSURE, L"压力", false},
    {ID_NAV_CONV_POWER, L"功率", false},
    {ID_NAV_CONV_ANGLE, L"角度", false},
    {ID_NAV_CONV_DATA, L"数据存储", false},
    {ID_NAV_CONV_FREQ, L"频率", false},
    {0, L"设置", true},
    {ID_THEME_TOGGLE, L"深色 / 浅色", false},
  };
  return items;
}

bool IsConvCmd(int cmd) {
  return cmd >= ID_NAV_CONV_CURRENCY && cmd <= ID_NAV_CONV_FREQ;
}

std::wstring ConvCatId(int cmd) {
  switch (cmd) {
    case ID_NAV_CONV_CURRENCY: return L"currency";
    case ID_NAV_CONV_VOLUME: return L"volume";
    case ID_NAV_CONV_LENGTH: return L"length";
    case ID_NAV_CONV_WEIGHT: return L"weight";
    case ID_NAV_CONV_TEMP: return L"temperature";
    case ID_NAV_CONV_AREA: return L"area";
    case ID_NAV_CONV_SPEED: return L"speed";
    case ID_NAV_CONV_TIME: return L"time";
    case ID_NAV_CONV_ENERGY: return L"energy";
    case ID_NAV_CONV_PRESSURE: return L"pressure";
    case ID_NAV_CONV_POWER: return L"power";
    case ID_NAV_CONV_ANGLE: return L"angle";
    case ID_NAV_CONV_DATA: return L"data";
    case ID_NAV_CONV_FREQ: return L"frequency";
  }
  return L"length";
}

bool g_flyoutRegistered = false;

// ---------- 导航浮出菜单 ----------
LRESULT CALLBACK FlyoutProc(HWND h, UINT m, WPARAM w, LPARAM l) {
  MainWindow* owner = (MainWindow*)GetWindowLongPtrW(h, GWLP_USERDATA);
  if (m == WM_NCCREATE) {
    owner = (MainWindow*)((CREATESTRUCTW*)l)->lpCreateParams;
    SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)owner);
    SetWindowLongPtrW(h, 0, -1);   // hover 索引
    return DefWindowProcW(h, m, w, l);
  }
  if (!owner) return DefWindowProcW(h, m, w, l);
  switch (m) {
    case WM_PAINT: {
      PAINTSTRUCT ps;
      HDC dc = BeginPaint(h, &ps);
      Theme::PaintBuffer(h, ps, [&](Gdiplus::Graphics& g, int cw, int ch) {
        const Theme::Palette& p = Theme::P();
        Gdiplus::SolidBrush bg(Theme::C(p.card));
        g.FillRectangle(&bg, 0, 0, cw, ch);
        int hover = (int)GetWindowLongPtrW(h, 0);
        const auto& items = FlyItems();
        int rowH = Theme::S(36);
        int y = Theme::S(8);
        for (int i = 0; i < (int)items.size(); i++) {
          const FlyItem& it = items[i];
          Gdiplus::RectF r(0, (float)y, (float)cw, (float)rowH);
          if (!it.header && i == hover) {
            Theme::FillRound(g, Gdiplus::RectF((float)Theme::S(6), (float)y, (float)cw - 2 * Theme::S(6), (float)rowH),
                             Theme::C(p.hoverNum), (float)Theme::S(4));
          }
          if (it.header) {
            Theme::DrawTextCentered(g, it.label, Theme::Font((float)Theme::S(11), true),
                                    Theme::C(p.textDim),
                                    Gdiplus::RectF((float)Theme::S(16), (float)y, (float)cw, (float)rowH));
          } else {
            Gdiplus::StringFormat fmt;
            fmt.SetAlignment(Gdiplus::StringAlignmentNear);
            fmt.SetLineAlignment(Gdiplus::StringAlignmentCenter);
            Gdiplus::SolidBrush b(Theme::C(p.text));
            Gdiplus::RectF tr((float)Theme::S(16), (float)y, (float)cw - Theme::S(32), (float)rowH);
            g.DrawString(it.label.c_str(), (INT)it.label.size(), Theme::Font((float)Theme::S(14)),
                         tr, &fmt, &b);
          }
          y += rowH;
        }
      });
      EndPaint(h, &ps);
      return 0;
    }
    case WM_ERASEBKGND:
      return 1;
    case WM_MOUSEMOVE: {
      int y = GET_Y_LPARAM(l);
      int idx = (y - Theme::S(8)) / Theme::S(36);
      const auto& items = FlyItems();
      if (idx >= 0 && idx < (int)items.size() && !items[idx].header) {
        if (GetWindowLongPtrW(h, 0) != idx) {
          SetWindowLongPtrW(h, 0, idx);
          InvalidateRect(h, nullptr, FALSE);
        }
      } else if (GetWindowLongPtrW(h, 0) != -1) {
        SetWindowLongPtrW(h, 0, -1);
        InvalidateRect(h, nullptr, FALSE);
      }
      TRACKMOUSEEVENT tme = {sizeof(tme), TME_LEAVE, h, 0};
      TrackMouseEvent(&tme);
      return 0;
    }
    case WM_MOUSELEAVE:
      SetWindowLongPtrW(h, 0, -1);
      InvalidateRect(h, nullptr, FALSE);
      return 0;
    case WM_LBUTTONUP: {
      int y = GET_Y_LPARAM(l);
      int idx = (y - Theme::S(8)) / Theme::S(36);
      const auto& items = FlyItems();
      if (idx >= 0 && idx < (int)items.size() && !items[idx].header) {
        int id = items[idx].id;
        DestroyWindow(h);
        if (id == ID_THEME_TOGGLE) owner->ToggleThemePub();
        else owner->SwitchMode(id);
      }
      return 0;
    }
    case WM_ACTIVATE:
      if (LOWORD(w) == WA_INACTIVE) DestroyWindow(h);
      return 0;
    case WM_KILLFOCUS:
      DestroyWindow(h);
      return 0;
  }
  return DefWindowProcW(h, m, w, l);
}

}  // namespace

MainWindow* MainWindow::Inst() {
  static MainWindow w;
  return &w;
}

bool MainWindow::Create(HINSTANCE inst, int nCmdShow) {
  inst_ = inst;
  WNDCLASSW wc = {};
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = MainWindow::WndProc;
  wc.hInstance = inst;
  wc.lpszClassName = L"W11Main";
  wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
  RegisterClassW(&wc);

  if (!g_flyoutRegistered) {
    WNDCLASSW fwc = {};
    fwc.style = CS_HREDRAW | CS_VREDRAW;
    fwc.lpfnWndProc = FlyoutProc;
    fwc.hInstance = inst;
    fwc.lpszClassName = L"W11Flyout";
    fwc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    fwc.cbWndExtra = sizeof(LONG_PTR);
    RegisterClassW(&fwc);
    g_flyoutRegistered = true;
  }

  hwnd_ = CreateWindowExW(0, L"W11Main", L"计算器",
                          WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                          CW_USEDEFAULT, CW_USEDEFAULT, Theme::S(380), Theme::S(700),
                          nullptr, nullptr, inst, this);
  if (!hwnd_) return false;
  Theme::ApplyTitlebar(hwnd_);

  // 顶栏按钮
  btnNav_ = std::unique_ptr<CtrlButton>(CtrlButton::Create(hwnd_, L"\uE700", kNavBtn, Theme::Kind::Memory));
  btnTheme_ = std::unique_ptr<CtrlButton>(CtrlButton::Create(hwnd_, L"\uE793", ID_THEME_TOGGLE, Theme::Kind::Memory));
  btnHistory_ = std::unique_ptr<CtrlButton>(CtrlButton::Create(hwnd_, L"\uE81C", ID_HISTORY_TOGGLE, Theme::Kind::Memory));

  // 显示区
  display_ = std::make_unique<DisplayPanel>();
  display_->Create(hwnd_, 3002);

  // 历史侧栏
  historyPanel_ = std::make_unique<HistoryPanel>();
  historyPanel_->Create(hwnd_, 3003, [this](const std::wstring& v) {
    if (active_) active_->LoadValue(v);
  });

  // 应用上下文
  ctx_.display = display_.get();
  ctx_.history = &store_;
  ctx_.memory = &memory_;
  ctx_.mainWindow = hwnd_;
  ctx_.addHistory = [this](const std::wstring& e, const std::wstring& r) { AddHistory(e, r); };
  ctx_.refreshMemory = [this]() { RefreshMemory(); };
  ctx_.refreshHistory = [this]() { RefreshHistory(); };
  ctx_.loadValue = [this](const std::wstring& v) { if (active_) active_->LoadValue(v); };

  SwitchMode(ID_NAV_STANDARD);
  ShowWindow(hwnd_, nCmdShow);
  UpdateWindow(hwnd_);
  return true;
}

ModePanel* MainWindow::CreatePanel(int modeCmd) {
  for (auto& [cmd, p] : panels_) {
    if (cmd == modeCmd) return p.get();
  }
  std::unique_ptr<ModePanel> panel;
  switch (modeCmd) {
    case ID_NAV_STANDARD: panel = std::make_unique<StandardPanel>(); break;
    case ID_NAV_SCIENTIFIC: panel = std::make_unique<ScientificPanel>(); break;
    case ID_NAV_GRAPHING: panel = std::make_unique<GraphingPanel>(); break;
    case ID_NAV_PROGRAMMER: panel = std::make_unique<ProgrammerPanel>(); break;
    case ID_NAV_DATE: panel = std::make_unique<DatePanel>(); break;
    case ID_NAV_CONVERTER: panel = std::make_unique<ConverterPanel>(); break;
    default: return nullptr;
  }
  panel->Create(hwnd_, inst_, ctx_);
  ModePanel* raw = panel.get();
  panels_.emplace_back(modeCmd, std::move(panel));
  return raw;
}

std::wstring MainWindow::ModeName(int navCmd) const {
  switch (navCmd) {
    case ID_NAV_STANDARD: return L"标准";
    case ID_NAV_SCIENTIFIC: return L"科学";
    case ID_NAV_GRAPHING: return L"图形";
    case ID_NAV_PROGRAMMER: return L"程序员";
    case ID_NAV_DATE: return L"日期计算";
  }
  if (IsConvCmd(navCmd)) {
    const UnitConv::Category* cat = UnitConv::Find(ConvCatId(navCmd));
    return cat ? (L"转换器 · " + cat->name) : L"转换器";
  }
  return L"转换器";
}

void MainWindow::SwitchMode(int navCmd) {
  int key = IsConvCmd(navCmd) ? ID_NAV_CONVERTER : navCmd;
  ModePanel* panel = CreatePanel(key);
  if (!panel) return;
  currentMode_ = navCmd;
  if (active_ && active_ != panel) ShowWindow(active_->Hwnd(), SW_HIDE);
  active_ = panel;
  ShowWindow(panel->Hwnd(), SW_SHOW);

  if (key == ID_NAV_CONVERTER) {
    auto* conv = dynamic_cast<ConverterPanel*>(panel);
    if (conv) conv->SetCategory(ConvCatId(navCmd));
  }

  // 清空显示区并按模式设置次要行
  display_->SetExpr(L"");
  display_->SetResult(L"", false);
  display_->SetAltLines(std::vector<std::wstring>((size_t)active_->AltLineCount(), L""));
  display_->SetMemory(memory_ != 0.0);
  active_->OnShow();

  // 按模式的最小窗口尺寸调整
  int minW = Theme::S(320), minH = Theme::S(560);
  switch (key) {
    case ID_NAV_SCIENTIFIC: minW = Theme::S(760); minH = Theme::S(560); break;
    case ID_NAV_GRAPHING: minW = Theme::S(660); minH = Theme::S(560); break;
    case ID_NAV_PROGRAMMER: minW = Theme::S(760); minH = Theme::S(640); break;
    case ID_NAV_DATE: minW = Theme::S(460); minH = Theme::S(520); break;
    case ID_NAV_CONVERTER: minW = Theme::S(440); minH = Theme::S(520); break;
  }
  RECT rc;
  GetWindowRect(hwnd_, &rc);
  int w = rc.right - rc.left, h = rc.bottom - rc.top;
  if (w < minW || h < minH) {
    SetWindowPos(hwnd_, nullptr, 0, 0, std::max(w, minW), std::max(h, minH),
                 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
  }
  ShowFlyout(false);
  Layout();
  InvalidateRect(hwnd_, nullptr, TRUE);
}

void MainWindow::Layout() {
  RECT rc;
  GetClientRect(hwnd_, &rc);
  int w = rc.right - rc.left, h = rc.bottom - rc.top;
  int topH = Theme::S(48);
  int histW = historyOpen_ ? Theme::S(280) : 0;
  int contentW = w - histW;

  // 顶栏按钮
  if (btnNav_->Hwnd()) MoveWindow(btnNav_->Hwnd(), Theme::S(4), Theme::S(4), Theme::S(40), Theme::S(40), TRUE);
  if (btnTheme_->Hwnd()) MoveWindow(btnTheme_->Hwnd(), contentW - Theme::S(46), Theme::S(4), Theme::S(40), Theme::S(40), TRUE);
  if (btnHistory_->Hwnd()) MoveWindow(btnHistory_->Hwnd(), contentW - Theme::S(92), Theme::S(4), Theme::S(40), Theme::S(40), TRUE);

  // 显示区
  int dispH = display_ ? display_->Height() : Theme::S(120);
  HWND dh = GetDlgItem(hwnd_, 3002);
  if (dh) MoveWindow(dh, Theme::S(12), topH + Theme::S(6), contentW - 2 * Theme::S(12), dispH, TRUE);

  // 模式面板
  if (active_) {
    int py = topH + Theme::S(6) + dispH + Theme::S(2);
    MoveWindow(active_->Hwnd(), 0, py, contentW, h - py, TRUE);
  }

  // 历史侧栏
  if (historyPanel_) {
    if (historyOpen_) {
      MoveWindow(historyPanel_->Hwnd(), contentW, topH, histW, h - topH, TRUE);
      ShowWindow(historyPanel_->Hwnd(), SW_SHOW);
    } else {
      ShowWindow(historyPanel_->Hwnd(), SW_HIDE);
    }
  }
}

void MainWindow::ShowFlyout(bool show) {
  if (!show) {
    if (flyout_ && IsWindow(flyout_)) DestroyWindow(flyout_);
    flyout_ = nullptr;
    return;
  }
  if (flyout_ && IsWindow(flyout_)) { DestroyWindow(flyout_); flyout_ = nullptr; return; }
  const auto& items = FlyItems();
  int rowH = Theme::S(36);
  int fw = Theme::S(264);
  int fh = Theme::S(16) + (int)items.size() * rowH;
  POINT pt = {Theme::S(4), Theme::S(48)};
  ClientToScreen(hwnd_, &pt);
  flyout_ = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, L"W11Flyout", L"",
                            WS_POPUP, pt.x, pt.y, fw, fh, hwnd_, nullptr, inst_, this);
  ShowWindow(flyout_, SW_SHOWNOACTIVATE);
  SetFocus(flyout_);
}

void MainWindow::ToggleThemePub() {
  ToggleTheme();
}

void MainWindow::ToggleTheme() {
  Theme::SetDark(!Theme::IsDark());
  Theme::ApplyTitlebar(hwnd_);
  InvalidateRect(hwnd_, nullptr, TRUE);
  // 刷新所有子控件并给原生控件套用深色样式
  EnumChildWindows(hwnd_, [](HWND h, LPARAM) -> BOOL {
    wchar_t cls[32] = {};
    GetClassNameW(h, cls, 31);
    if (lstrcmpiW(cls, L"Edit") == 0 || lstrcmpiW(cls, L"ComboBox") == 0)
      Theme::ApplyNativeDark(h);
    InvalidateRect(h, nullptr, TRUE);
    return TRUE;
  }, 0);
  if (active_) InvalidateRect(active_->Hwnd(), nullptr, TRUE);
}

void MainWindow::ToggleHistory() {
  historyOpen_ = !historyOpen_;
  RefreshHistory();
  Layout();
}

void MainWindow::RefreshMemory() {
  if (display_) display_->SetMemory(memory_ != 0.0);
}

void MainWindow::RefreshHistory() {
  if (historyPanel_) historyPanel_->Refresh(store_);
}

void MainWindow::AddHistory(const std::wstring& expr, const std::wstring& result) {
  store_.Add(expr, result);
  RefreshHistory();
}

LRESULT CALLBACK MainWindow::WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
  MainWindow* wnd = (MainWindow*)GetWindowLongPtrW(h, GWLP_USERDATA);
  if (m == WM_NCCREATE) {
    wnd = (MainWindow*)((CREATESTRUCTW*)l)->lpCreateParams;
    SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)wnd);
    if (wnd) wnd->hwnd_ = h;
    return DefWindowProcW(h, m, w, l);
  }
  if (wnd) return wnd->Handle(m, w, l);
  return DefWindowProcW(h, m, w, l);
}

LRESULT MainWindow::Handle(UINT m, WPARAM w, LPARAM l) {
  switch (m) {
    case WM_PAINT: {
      PAINTSTRUCT ps;
      HDC dc = BeginPaint(hwnd_, &ps);
      Theme::PaintBuffer(hwnd_, ps, [&](Gdiplus::Graphics& g, int cw, int ch) {
        const Theme::Palette& p = Theme::P();
        Gdiplus::SolidBrush bg(Theme::C(p.bg));
        g.FillRectangle(&bg, 0, 0, cw, ch);
        // 顶栏分隔线
        Gdiplus::Pen pen(Theme::C(p.border), 1.0f);
        g.DrawLine(&pen, 0.0f, (float)Theme::S(48), (float)cw, (float)Theme::S(48));
        // 模式名
        Gdiplus::StringFormat fmt;
        fmt.SetAlignment(Gdiplus::StringAlignmentNear);
        fmt.SetLineAlignment(Gdiplus::StringAlignmentCenter);
        Gdiplus::SolidBrush b(Theme::C(p.text));
        Gdiplus::RectF r((float)Theme::S(52), 0, (float)cw - Theme::S(160), (float)Theme::S(48));
        std::wstring name = ModeName(currentMode_);
        g.DrawString(name.c_str(), (INT)name.size(), Theme::Font((float)Theme::S(14), true), r, &fmt, &b);
      });
      EndPaint(hwnd_, &ps);
      return 0;
    }
    case WM_ERASEBKGND:
      return 1;
    case WM_COMMAND:
      switch (LOWORD(w)) {
        case kNavBtn: ShowFlyout(!flyout_); return 0;
        case ID_HISTORY_TOGGLE: ToggleHistory(); return 0;
        case ID_THEME_TOGGLE: ToggleTheme(); return 0;
        case ID_HISTORY_CLEAR:
          store_.Clear();
          RefreshHistory();
          return 0;
        case ID_HISTORY_DELETE:
          store_.Remove((size_t)l);
          RefreshHistory();
          return 0;
      }
      break;
    case WM_SIZE:
      Layout();
      return 0;
    case WM_GETMINMAXINFO: {
      MINMAXINFO* mmi = (MINMAXINFO*)l;
      mmi->ptMinTrackSize.x = Theme::S(320);
      mmi->ptMinTrackSize.y = Theme::S(480);
      return 0;
    }
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
  }
  return DefWindowProcW(hwnd_, m, w, l);
}

int MainWindow::Run() {
  MSG msg;
  while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
    bool routed = false;
    if (active_ && (msg.message == WM_CHAR || msg.message == WM_KEYDOWN)) {
      HWND f = GetFocus();
      wchar_t cls[32] = {};
      if (f) GetClassNameW(f, cls, 31);
      bool editable = lstrcmpiW(cls, L"Edit") == 0 || lstrcmpiW(cls, L"ComboBox") == 0 ||
                      lstrcmpiW(cls, L"ListBox") == 0 || lstrcmpiW(cls, L"SysListView32") == 0;
      if (!editable) {
        if (msg.message == WM_CHAR) routed = active_->OnChar((wchar_t)msg.wParam);
        else routed = active_->OnKey(msg.wParam);
      }
    }
    if (!routed) {
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
    }
  }
  return (int)msg.wParam;
}
