#pragma once
#include "Common.h"
#include "Commands.h"
// ============================================================
// 模式面板基类：主窗口与各模式之间的统一接口
// ============================================================
namespace History { class Store; }

struct AppContext {
  class DisplayPanel* display = nullptr;
  History::Store* history = nullptr;
  double* memory = nullptr;                                  // 计算器共享记忆值
  std::function<void(const std::wstring&, const std::wstring&)> addHistory;
  std::function<void()> refreshMemory;                       // 刷新 M 徽标
  std::function<void()> refreshHistory;                      // 刷新历史列表
  std::function<void(const std::wstring&)> loadValue;        // 从历史载入数值
  HWND mainWindow = nullptr;
};

class ModePanel {
public:
  virtual ~ModePanel() = default;
  virtual HWND Create(HWND parent, HINSTANCE inst, const AppContext& ctx) = 0;
  virtual void OnSize(int w, int h) = 0;
  virtual void OnShow() {}                                    // 切换到本模式时调用
  virtual bool OnChar(wchar_t c) { return false; }            // 字符键
  virtual bool OnKey(WPARAM vk) { return false; }             // 功能键（回车/退格等）
  virtual void LoadValue(const std::wstring& v) {}            // 点击历史载入数值
  virtual int AltLineCount() const { return 0; }              // 显示区次要行数
  HWND Hwnd() const { return hwnd_; }
  static const wchar_t* ClassName();
protected:
  static LRESULT CALLBACK PanelProc(HWND, UINT, WPARAM, LPARAM);
  virtual LRESULT HandleMessage(UINT msg, WPARAM w, LPARAM l);  // 默认处理 WM_SIZE/WM_COMMAND/绘制
  virtual bool OnCommand(int id) { return false; }
  AppContext ctx_;
  HWND hwnd_ = nullptr;
};
