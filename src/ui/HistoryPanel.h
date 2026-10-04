#pragma once
#include "Common.h"
#include "../core/HistoryStore.h"
// ============================================================
// 历史记录侧栏：自绘列表 + 清除/删除按钮，双击载入数值
// ============================================================
class HistoryPanel {
public:
  HWND Create(HWND parent, int id, const std::function<void(const std::wstring&)>& onLoad);
  static HistoryPanel* From(HWND h);
  void Refresh(const History::Store& store);
  void OnSize(int w, int h);
  HWND Hwnd() const { return hwnd_; }
private:
  static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
  LRESULT Handle(UINT, WPARAM, LPARAM);
  void EnsureList();
  void ClearAll();
  void DeleteSelected();
  HWND hwnd_ = nullptr, list_ = nullptr, btnClear_ = nullptr, btnDel_ = nullptr;
  std::function<void(const std::wstring&)> onLoad_;
  std::vector<History::Entry> entries_;
};
