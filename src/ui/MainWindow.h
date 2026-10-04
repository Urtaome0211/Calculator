#pragma once
#include "Common.h"
#include "Commands.h"
#include "ModePanel.h"
#include "DisplayPanel.h"
#include "HistoryPanel.h"
#include "CtrlButton.h"
#include "../core/HistoryStore.h"
// ============================================================
// 主窗口：顶栏（汉堡菜单/模式名/历史/主题）+ 显示区 +
// 当前模式面板 + 导航浮出菜单 + 历史侧栏 + 键盘路由
// ============================================================
class MainWindow {
public:
  static MainWindow* Inst();
  bool Create(HINSTANCE inst, int nCmdShow);
  HWND Hwnd() const { return hwnd_; }
  void SwitchMode(int navCmd);
  void RefreshMemory();
  void RefreshHistory();
  void AddHistory(const std::wstring& expr, const std::wstring& result);
  void ToggleThemePub();   // 导航菜单主题入口
  int Run();
private:
  static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
  LRESULT Handle(UINT, WPARAM, LPARAM);
  ModePanel* CreatePanel(int modeCmd);
  void Layout();
  void ShowFlyout(bool show);
  void ToggleHistory();
  void ToggleTheme();
  std::wstring ModeName(int navCmd) const;

  int currentMode_ = ID_NAV_STANDARD;
  HWND hwnd_ = nullptr;
  HWND flyout_ = nullptr;
  std::unique_ptr<CtrlButton> btnNav_, btnHistory_, btnTheme_;
  std::unique_ptr<DisplayPanel> display_;
  std::unique_ptr<HistoryPanel> historyPanel_;
  History::Store store_;
  double memory_ = 0.0;
  std::vector<std::pair<int, std::unique_ptr<ModePanel>>> panels_;
  ModePanel* active_ = nullptr;
  bool historyOpen_ = false;
  HINSTANCE inst_ = nullptr;
  AppContext ctx_;
};
