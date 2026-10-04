#pragma once
#include "ModePanel.h"
// ============================================================
// 图形模式：最多 3 条函数曲线，滚轮缩放、拖拽平移、双击复位
// ============================================================
class GraphingPanel : public ModePanel {
public:
  HWND Create(HWND parent, HINSTANCE inst, const AppContext& ctx) override;
  void OnSize(int w, int h) override;
  void OnShow() override;
private:
  LRESULT HandleMessage(UINT msg, WPARAM w, LPARAM l) override;
  bool OnCommand(int id) override;
  void Replot();                            // 重新求值并绘制
  struct Eq { HWND edit = nullptr; bool on = false; COLORREF color = RGB(0,0,0); };
  Eq eqs_[3];
  HWND canvas_ = nullptr;
  double xmin_ = -10, xmax_ = 10, ymin_ = -10, ymax_ = 10;
  bool viewSet_ = false;                    // 是否已手动调整视口
  bool dragging_ = false;
  int lastX_ = 0, lastY_ = 0;
  static const int kSamples = 600;           // 每条曲线采样点数
  double ys_[3][kSamples];                   // 采样缓存（绘图直接使用）
  bool ok_[3] = {false, false, false};
  static LRESULT CALLBACK CanvasProc(HWND, UINT, WPARAM, LPARAM);
  void PaintCanvas(HDC dc);
  void ZoomAt(int cx, double factor);
  void Pan(int dx, int dy);
};
