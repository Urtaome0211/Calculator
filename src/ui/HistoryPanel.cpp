#include "HistoryPanel.h"
#include "Theme.h"
#include "Commands.h"
#include "../core/NumberFormat.h"

// ============================================================
// 历史记录侧栏：自绘列表 + 清除/删除，双击载入数值
// ============================================================
namespace {
bool g_registered = false;
}

HWND HistoryPanel::Create(HWND parent, int id, const std::function<void(const std::wstring&)>& onLoad) {
  onLoad_ = onLoad;
  if (!g_registered) {
    WNDCLASSW wc = {};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = HistoryPanel::WndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"W11History";
    RegisterClassW(&wc);
    g_registered = true;
  }
  hwnd_ = CreateWindowExW(0, L"W11History", L"", WS_CHILD | WS_VISIBLE,
                          0, 0, 200, 300, parent, (HMENU)(INT_PTR)id,
                          GetModuleHandleW(nullptr), this);
  return hwnd_;
}

HistoryPanel* HistoryPanel::From(HWND h) {
  return (HistoryPanel*)GetWindowLongPtrW(h, GWLP_USERDATA);
}

void HistoryPanel::EnsureList() {
  if (list_) return;
  // 自绘列表：每条记录两行（表达式 + 结果）
  list_ = CreateWindowExW(0, L"LISTBOX", L"",
                          WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_OWNERDRAWFIXED |
                          LBS_NOINTEGRALHEIGHT | LBS_NOTIFY,
                          0, 0, 10, 10, hwnd_, nullptr, GetModuleHandleW(nullptr), nullptr);
  SendMessageW(list_, WM_SETFONT, (WPARAM)Theme::FontH(14), TRUE);
  SendMessageW(list_, LB_SETITEMHEIGHT, 0, Theme::S(54));
  btnClear_ = CreateWindowExW(0, L"BUTTON", L"清除全部", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                              0, 0, 10, 10, hwnd_, (HMENU)ID_HISTORY_CLEAR,
                              GetModuleHandleW(nullptr), nullptr);
  btnDel_ = CreateWindowExW(0, L"BUTTON", L"删除选中", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                            0, 0, 10, 10, hwnd_, (HMENU)ID_HISTORY_DELETE,
                            GetModuleHandleW(nullptr), nullptr);
  SendMessageW(btnClear_, WM_SETFONT, (WPARAM)Theme::FontH(13), TRUE);
  SendMessageW(btnDel_, WM_SETFONT, (WPARAM)Theme::FontH(13), TRUE);
  Theme::ApplyNativeDark(btnClear_);
  Theme::ApplyNativeDark(btnDel_);
  // 列表深色主题
  SetWindowTheme(list_, Theme::IsDark() ? L"DarkMode_Explorer" : nullptr, nullptr);
}

void HistoryPanel::Refresh(const History::Store& store) {
  EnsureList();
  entries_ = store.Entries();
  SendMessageW(list_, LB_RESETCONTENT, 0, 0);
  for (size_t i = 0; i < entries_.size(); i++) {
    SendMessageW(list_, LB_ADDSTRING, 0, (LPARAM)i);
  }
  InvalidateRect(hwnd_, nullptr, TRUE);
}

void HistoryPanel::OnSize(int w, int h) {
  if (!list_) return;
  int btnH = Theme::S(36);
  MoveWindow(btnDel_, Theme::S(8), h - btnH - Theme::S(8), (w - Theme::S(24)) / 2, btnH, TRUE);
  MoveWindow(btnClear_, Theme::S(12) + (w - Theme::S(24)) / 2, h - btnH - Theme::S(8),
             (w - Theme::S(24)) / 2, btnH, TRUE);
  MoveWindow(list_, Theme::S(8), Theme::S(44), w - Theme::S(16),
             h - Theme::S(44) - btnH - Theme::S(20), TRUE);
}

void HistoryPanel::ClearAll() {
  // 由 MainWindow 通过消息转发（此处直接清空本地并通知宿主）
  // 宿主（MainWindow）持有 Store，这里仅发送通知：通过父窗口命令
  SendMessageW(GetParent(hwnd_), WM_COMMAND, MAKEWPARAM(ID_HISTORY_CLEAR, BN_CLICKED), (LPARAM)hwnd_);
}

void HistoryPanel::DeleteSelected() {
  LRESULT sel = SendMessageW(list_, LB_GETCURSEL, 0, 0);
  if (sel == LB_ERR) return;
  SendMessageW(GetParent(hwnd_), WM_COMMAND, MAKEWPARAM(ID_HISTORY_DELETE, BN_CLICKED), (LPARAM)(INT_PTR)sel);
}

LRESULT CALLBACK HistoryPanel::WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
  HistoryPanel* p = HistoryPanel::From(h);
  if (m == WM_NCCREATE) {
    p = (HistoryPanel*)((CREATESTRUCTW*)l)->lpCreateParams;
    SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)p);
    if (p) p->hwnd_ = h;
    return DefWindowProcW(h, m, w, l);
  }
  if (!p) return DefWindowProcW(h, m, w, l);
  return p->Handle(m, w, l);
}

LRESULT HistoryPanel::Handle(UINT m, WPARAM w, LPARAM l) {
  switch (m) {
    case WM_SIZE:
      OnSize(LOWORD(l), HIWORD(l));
      return 0;
    case WM_ERASEBKGND:
      return 1;
    case WM_PAINT: {
      PAINTSTRUCT ps;
      HDC dc = BeginPaint(hwnd_, &ps);
      Theme::PaintBuffer(hwnd_, ps, [&](Gdiplus::Graphics& g, int cw, int ch) {
        const Theme::Palette& p = Theme::P();
        Gdiplus::SolidBrush bg(Theme::C(p.bg));
        g.FillRectangle(&bg, 0, 0, cw, ch);
        // 标题
        Gdiplus::StringFormat fmt;
        fmt.SetAlignment(Gdiplus::StringAlignmentNear);
        fmt.SetLineAlignment(Gdiplus::StringAlignmentCenter);
        Gdiplus::SolidBrush b(Theme::C(p.text));
        Gdiplus::RectF r((float)Theme::S(12), 0, (float)cw, (float)Theme::S(44));
        g.DrawString(L"历史记录", 4, Theme::Font((float)Theme::S(15), true), r, &fmt, &b);
        // 空提示
        if (entries_.empty()) {
          Gdiplus::RectF er(0, (float)Theme::S(80), (float)cw, (float)Theme::S(40));
          Theme::DrawTextCentered(g, L"暂无历史记录", Theme::Font((float)Theme::S(13)),
                                  Theme::C(p.textDim), er);
        }
      });
      EndPaint(hwnd_, &ps);
      return 0;
    }
    case WM_COMMAND:
      if (HIWORD(w) == BN_CLICKED) {
        int id = LOWORD(w);
        if (id == ID_HISTORY_CLEAR) ClearAll();
        else if (id == ID_HISTORY_DELETE) DeleteSelected();
        return 0;
      }
      break;
    case WM_DRAWITEM: {
      // 自绘列表项
      DRAWITEMSTRUCT* di = (DRAWITEMSTRUCT*)l;
      if (di->CtlID == 0 && di->itemID >= 0 && di->itemID < entries_.size()) {
        const Theme::Palette& p = Theme::P();
        const History::Entry& e = entries_[di->itemID];
        Gdiplus::Graphics g(di->hDC);
        g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);
        RECT rc = di->rcItem;
        int w = rc.right - rc.left, h = rc.bottom - rc.top;
        // 背景
        Gdiplus::SolidBrush bg(Theme::C((di->itemState & ODS_SELECTED) ? p.hoverNum : p.card));
        g.FillRectangle(&bg, rc.left, rc.top, w, h);
        // 圆角卡片
        Theme::FillRound(g, Gdiplus::RectF((float)rc.left + 2, (float)rc.top + 2,
                                           (float)w - 4, (float)h - 4),
                         Theme::C((di->itemState & ODS_SELECTED) ? p.hoverNum : p.card),
                         (float)Theme::S(6));
        Gdiplus::StringFormat fmt;
        fmt.SetAlignment(Gdiplus::StringAlignmentNear);
        fmt.SetLineAlignment(Gdiplus::StringAlignmentCenter);
        fmt.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);
        Gdiplus::SolidBrush dim(Theme::C(p.textDim));
        Gdiplus::RectF r1((float)rc.left + Theme::S(12), (float)rc.top + Theme::S(4),
                          (float)w - Theme::S(24), (float)Theme::S(24));
        g.DrawString(e.expr.c_str(), (INT)e.expr.size(), Theme::Font((float)Theme::S(12)),
                     r1, &fmt, &dim);
        Gdiplus::SolidBrush txt(Theme::C(p.text));
        Gdiplus::RectF r2((float)rc.left + Theme::S(12), (float)rc.top + Theme::S(26),
                          (float)w - Theme::S(24), (float)Theme::S(26));
        std::wstring res = L"= " + e.result;
        g.DrawString(res.c_str(), (INT)res.size(), Theme::Font((float)Theme::S(15), true),
                     r2, &fmt, &txt);
        return TRUE;
      }
      return FALSE;
    }
    case WM_MEASUREITEM: {
      MEASUREITEMSTRUCT* mi = (MEASUREITEMSTRUCT*)l;
      mi->itemHeight = Theme::S(54);
      return TRUE;
    }
    case WM_CTLCOLORLISTBOX:
      SetBkColor((HDC)w, Theme::P().card);
      SetTextColor((HDC)w, Theme::P().text);
      return (LRESULT)Theme::CardBrush();
    case WM_LBUTTONDBLCLK: {
      // 列表双击由 LBN_DBLCLK 通知转发（见下方统一处理）
      break;
    }
    default:
      break;
  }
  // 列表双击通知
  if (m == WM_COMMAND && HIWORD(w) == LBN_DBLCLK && list_ && (HWND)l == list_) {
    LRESULT sel = SendMessageW(list_, LB_GETCURSEL, 0, 0);
    if (sel != LB_ERR && sel >= 0 && sel < (LRESULT)entries_.size() && onLoad_) {
      onLoad_(entries_[sel].result);
    }
    return 0;
  }
  return DefWindowProcW(hwnd_, m, w, l);
}
