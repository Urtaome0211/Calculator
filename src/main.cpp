// ============================================================
// 计算器入口：DPI 感知 + GDI+ 启动 + 主窗口 + 消息循环
// ============================================================
#include "Common.h"
#include "ui/Theme.h"
#include "ui/MainWindow.h"
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
  SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

  ULONG_PTR gdiToken = 0;
  Gdiplus::GdiplusStartupInput gsi;
  Gdiplus::GdiplusStartup(&gdiToken, &gsi, nullptr);

  INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES};
  InitCommonControlsEx(&icc);

  Theme::Init();

  if (!MainWindow::Inst()->Create(hInstance, nCmdShow)) {
    Gdiplus::GdiplusShutdown(gdiToken);
    return 1;
  }
  int rc = MainWindow::Inst()->Run();
  Gdiplus::GdiplusShutdown(gdiToken);
  return rc;
}
