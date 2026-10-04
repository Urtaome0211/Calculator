#pragma once
#include "ModePanel.h"
// ============================================================
// 单位转换器：类别 + 双单位卡片，输入任意一侧即时换算另一侧
// ============================================================
class ConverterPanel : public ModePanel {
public:
  HWND Create(HWND parent, HINSTANCE inst, const AppContext& ctx) override;
  void OnSize(int w, int h) override;
  void OnShow() override;
  void SetCategory(const std::wstring& id);   // 由导航指定初始类别
  int AltLineCount() const override { return 1; }
private:
  LRESULT HandleMessage(UINT msg, WPARAM w, LPARAM l) override;
  bool OnCommand(int id) override;
  void Convert(bool fromTo);                  // true=从左到右
  void ReloadUnits();
  void Swap();
  HWND comboCat_ = nullptr, comboFrom_ = nullptr, comboTo_ = nullptr;
  HWND editFrom_ = nullptr, editTo_ = nullptr;
  HWND btnSwap_ = nullptr;
  std::wstring catId_ = L"length";
  bool updating_ = false;
};
