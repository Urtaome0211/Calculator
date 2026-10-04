#include "ModePanel.h"
#include "Theme.h"

// ============================================================
// 模式面板基类：公共窗口类与默认消息处理
// ============================================================
namespace {
bool g_registered = false;
}

const wchar_t* ModePanel::ClassName() {
  if (!g_registered) {
    WNDCLASSW wc = {};
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = ModePanel::PanelProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"W11Panel";
    RegisterClassW(&wc);
    g_registered = true;
  }
  return L"W11Panel";
}

LRESULT CALLBACK ModePanel::PanelProc(HWND h, UINT m, WPARAM w, LPARAM l) {
  ModePanel* p = (ModePanel*)GetWindowLongPtrW(h, GWLP_USERDATA);
  if (m == WM_NCCREATE) {
    p = (ModePanel*)((CREATESTRUCTW*)l)->lpCreateParams;
    SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)p);
    if (p) p->hwnd_ = h;
    return DefWindowProcW(h, m, w, l);
  }
  if (p) return p->HandleMessage(m, w, l);
  return DefWindowProcW(h, m, w, l);
}

LRESULT ModePanel::HandleMessage(UINT msg, WPARAM w, LPARAM l) {
  switch (msg) {
    case WM_SIZE:
      OnSize(LOWORD(l), HIWORD(l));
      return 0;
    case WM_ERASEBKGND:
      return 1;
    case WM_PAINT: {
      PAINTSTRUCT ps;
      HDC dc = BeginPaint(hwnd_, &ps);
      RECT rc;
      GetClientRect(hwnd_, &rc);
      HBRUSH br = CreateSolidBrush(Theme::P().bg);
      FillRect(dc, &rc, br);
      DeleteObject(br);
      EndPaint(hwnd_, &ps);
      return 0;
    }
    case WM_COMMAND:
      if (HIWORD(w) == BN_CLICKED && OnCommand(LOWORD(w))) return 0;
      break;
  }
  return DefWindowProcW(hwnd_, msg, w, l);
}
