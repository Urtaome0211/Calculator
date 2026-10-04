#include "Keypad.h"

// ============================================================
// 键盘面板：按规格批量创建按钮，随尺寸自动布局
// ============================================================
void Keypad::Build(HWND parent, const std::vector<KeyRow>& rows, int columns,
                   int hGap, int vGap) {
  parent_ = parent;
  columns_ = columns;
  hGap_ = hGap;
  vGap_ = vGap;
  rows_ = (int)rows.size();
  items_.clear();
  int r = 0;
  for (const auto& row : rows) {
    int c = 0;
    for (const auto& cell : row.cells) {
      int span = std::max(1, cell.colSpan);
      if (cell.id != 0) {
        CtrlButton* btn = CtrlButton::Create(parent, cell.label, cell.id, cell.kind, cell.small);
        if (btn) items_.push_back({cell.id, btn->Hwnd(), r, c, span, std::max(1, cell.rowSpan)});
      }
      c += span;
    }
    r++;
  }
}

void Keypad::Layout(const RECT& rc) {
  if (items_.empty()) return;
  int w = rc.right - rc.left, h = rc.bottom - rc.top;
  if (w <= 0 || h <= 0 || rows_ <= 0) return;
  float cellW = (float)(w - (columns_ - 1) * hGap_) / columns_;
  float cellH = (float)(h - (rows_ - 1) * vGap_) / rows_;
  for (const auto& it : items_) {
    // 从该行第一个格子开始累计 x（跳过跨行占位）
    float x = (float)rc.left;
    // 该行第 col 列之前的格子宽度未知，直接按列宽计算（跨列由 col 累加保证）
    x += it.col * (cellW + hGap_);
    float y = (float)rc.top + it.row * (cellH + vGap_);
    float iw = it.colSpan * cellW + (it.colSpan - 1) * hGap_;
    float ih = it.rowSpan * cellH + (it.rowSpan - 1) * vGap_;
    MoveWindow(it.h, (int)(x + 0.5f), (int)(y + 0.5f), (int)(iw + 0.5f), (int)(ih + 0.5f), TRUE);
  }
}

CtrlButton* Keypad::Get(int id) {
  for (auto& it : items_) {
    if (it.id == id) return CtrlButton::From(it.h);
  }
  return nullptr;
}

void Keypad::SetChecked(int id, bool on) {
  CtrlButton* b = Get(id);
  if (b) b->SetChecked(on);
}
