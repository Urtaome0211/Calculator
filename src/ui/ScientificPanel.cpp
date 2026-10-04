#include "ScientificPanel.h"
// ============================================================
// 科学模式面板：8 列 × 6 行主键盘 + 三角侧栏 + 2nd 切换 + 角度制切换
// 说明：ScientificPanel.h 未声明 RefreshAngleChecks 且不允许改动头文件，
// 故以匿名命名空间自由函数实现（通过 Keypad::Get 判空，等价于成员函数）。
// ============================================================

namespace {
// 按当前角度制勾选记忆条上的 DEG/RAD/GRAD 按钮
void RefreshAngleChecks(Keypad& memPad, Expr::AngleMode angle) {
  if (CtrlButton* b = memPad.Get(IDK_SC_DEG))  b->SetChecked(angle == Expr::AngleMode::Deg);
  if (CtrlButton* b = memPad.Get(IDK_SC_RAD))  b->SetChecked(angle == Expr::AngleMode::Rad);
  if (CtrlButton* b = memPad.Get(IDK_SC_GRAD)) b->SetChecked(angle == Expr::AngleMode::Grad);
}
}

// ---------- 记忆条：MC/MR/M+/M-/MS + 角度制切换（共 8 格） ----------
std::vector<KeyCell> ScientificPanel::MemoryRow() const {
  using K = Theme::Kind;
  return {
    {L"MC",   IDK_MC,       K::Memory, 1, 1, true},
    {L"MR",   IDK_MR,       K::Memory, 1, 1, true},
    {L"M+",   IDK_MP,       K::Memory, 1, 1, true},
    {L"M-",   IDK_MM,       K::Memory, 1, 1, true},
    {L"MS",   IDK_MS,       K::Memory, 1, 1, true},
    {L"DEG",  IDK_SC_DEG,   K::Toggle, 1, 1, true},
    {L"RAD",  IDK_SC_RAD,   K::Toggle, 1, 1, true},
    {L"GRAD", IDK_SC_GRAD,  K::Toggle, 1, 1, true},
  };
}

// ---------- 主键盘：8 列 × 6 行 ----------
// 行 0：2nd π e C ⌫ ( ) + sin(跨 2 行)
// 行 1：x² 1/x |x| exp mod % ÷（第 8 列被 sin 占用）
// 行 2：√x xʸ x√y 7 8 9 × + cos(跨 2 行)
// 行 3：10ˣ log logy 4 5 6 −（第 8 列被 cos 占用）
// 行 4：ln 2ˣ n! 1 2 3 + + tan(跨 2 行)
// 行 5：rand eˣ ± 0 . =（第 8 列被 tan 占用）
// KeyCell 聚合顺序：{label, id, kind, colSpan, rowSpan, small}
void ScientificPanel::BuildLayout(Keypad& pad) {
  using K = Theme::Kind;
  const K Fn = K::Function;

  std::vector<KeyRow> rows;
  rows.push_back(KeyRow{{
    {L"2nd",  IDK_SC_2ND,  K::Toggle},
    {L"π",    IDK_SC_PI,   Fn},
    {L"e",    IDK_SC_E,    Fn},
    {L"C",    IDK_CLEAR,   Fn},
    {L"\uE94F", IDK_BACK,  Fn},          // 退格图标
    {L"(",    IDK_SC_LPAR, Fn},
    {L")",    IDK_SC_RPAR, Fn},
    {L"sin",  IDK_SC_SIN,  Fn, 1, 2},
  }});
  rows.push_back(KeyRow{{
    {L"x²",   IDK_SQ,      Fn},
    {L"1/x",  IDK_RECIP,   Fn},
    {L"|x|",  IDK_SC_ABS,  Fn},
    {L"exp",  IDK_SC_EPOW, Fn},
    {L"mod",  IDK_SC_MOD,  Fn},
    {L"%",    IDK_PERCENT, Fn},
    {L"÷",    IDK_DIV,     Fn},
  }});
  rows.push_back(KeyRow{{
    {L"√x",   IDK_SQRT,    Fn},
    {L"xʸ",   IDK_SC_POW,  Fn},
    {L"x√y",  IDK_SC_ROOTY, Fn},
    {L"7",    IDK_7,       K::Number},
    {L"8",    IDK_8,       K::Number},
    {L"9",    IDK_9,       K::Number},
    {L"×",    IDK_MUL,     Fn},
    {L"cos",  IDK_SC_COS,  Fn, 1, 2},
  }});
  rows.push_back(KeyRow{{
    {L"10ˣ",  IDK_SC_POW10, Fn},
    {L"log",  IDK_SC_LOG,  Fn},
    {L"logy", IDK_SC_LOGB, Fn},
    {L"4",    IDK_4,       K::Number},
    {L"5",    IDK_5,       K::Number},
    {L"6",    IDK_6,       K::Number},
    {L"−",    IDK_SUB,     Fn},
  }});
  rows.push_back(KeyRow{{
    {L"ln",   IDK_SC_LN,   Fn},
    {L"2ˣ",   IDK_SC_2POW, Fn},
    {L"n!",   IDK_SC_FACT, Fn},
    {L"1",    IDK_1,       K::Number},
    {L"2",    IDK_2,       K::Number},
    {L"3",    IDK_3,       K::Number},
    {L"+",    IDK_ADD,     Fn},
    {L"tan",  IDK_SC_TAN,  Fn, 1, 2},
  }});
  rows.push_back(KeyRow{{
    {L"rand", IDK_SC_RAND, Fn},
    {L"eˣ",   IDK_CE,      Fn},         // 复用 IDK_CE 作为 eˣ 按钮 ID
    {L"±",    IDK_SIGN,    Fn},
    {L"0",    IDK_0,       K::Number},
    {L".",    IDK_DOT,     K::Number},
    {L"=",    IDK_EQU,     K::Accent},
  }});

  // 8 列、间距 4px（随 DPI 缩放）；基类 Create 建好面板窗口后调用本函数
  pad.Build(hwnd_, rows, 8, Theme::S(4), Theme::S(4));
}

// ---------- 命令处理：科学按键 + 2nd 分支，其余交基类 ----------
bool ScientificPanel::OnCommand(int id) {
  switch (id) {
    case IDK_SC_2ND:                       // 切换第二功能
      second_ = !second_;
      ApplySecond();
      pad_.SetChecked(IDK_SC_2ND, second_);
      return true;
    case IDK_SC_PI:   Insert(L"π");  return true;
    case IDK_SC_E:    Insert(L"e");  return true;
    case IDK_SC_FACT: Insert(L"!");  return true;
    case IDK_SC_POW:  Insert(L"^");  return true;
    case IDK_SC_LPAR: Insert(L"(");  return true;
    case IDK_SC_RPAR: Insert(L")");  return true;
    case IDK_SC_SIN:   PressFunc(second_ ? L"asin" : L"sin");   return true;
    case IDK_SC_COS:   PressFunc(second_ ? L"acos" : L"cos");   return true;
    case IDK_SC_TAN:   PressFunc(second_ ? L"atan" : L"tan");   return true;
    case IDK_SC_ABS:   PressFunc(L"abs");                       return true;
    case IDK_SC_MOD:   PressFunc(L"mod");                       return true;
    case IDK_SC_EPOW:  PressFunc(L"exp");                       return true;
    case IDK_SC_POW10: PressFunc(second_ ? L"pow2" : L"pow10"); return true;
    case IDK_SC_2POW:  PressFunc(L"pow2");                      return true;
    case IDK_SC_LOG:   PressFunc(second_ ? L"logy" : L"log");   return true;
    case IDK_SC_LN:    PressFunc(second_ ? L"exp" : L"ln");     return true;
    case IDK_SC_ROOTY: PressFunc(L"xroot");                     return true;
    case IDK_SC_LOGB:  PressFunc(L"logy");                      return true;
    case IDK_SC_RAND:  PressFunc(L"rand");                      return true;
    case IDK_CE:       PressFunc(L"exp");                       return true;  // eˣ 按钮
    case IDK_SC_DEG:
      angle_ = Expr::AngleMode::Deg;
      UpdateDisplay();
      RefreshAngleChecks(memPad_, angle_);
      return true;
    case IDK_SC_RAD:
      angle_ = Expr::AngleMode::Rad;
      UpdateDisplay();
      RefreshAngleChecks(memPad_, angle_);
      return true;
    case IDK_SC_GRAD:
      angle_ = Expr::AngleMode::Grad;
      UpdateDisplay();
      RefreshAngleChecks(memPad_, angle_);
      return true;
    default:
      return KeypadPanel::OnCommand(id);  // 数字/四则/记忆等共享命令
  }
}

// 插入函数 token（引擎自动补隐式乘法与右括号）
void ScientificPanel::PressFunc(const std::wstring& name) {
  Insert(name + L"(");
}

// ---------- 2nd 模式按钮文字切换（7 组） ----------
void ScientificPanel::ApplySecond() {
  auto label = [this](int id, const wchar_t* on, const wchar_t* off) {
    if (CtrlButton* b = pad_.Get(id)) b->SetLabel(second_ ? on : off);
  };
  label(IDK_SC_SIN,   L"sin⁻¹", L"sin");
  label(IDK_SC_COS,   L"cos⁻¹", L"cos");
  label(IDK_SC_TAN,   L"tan⁻¹", L"tan");
  label(IDK_SC_POW,   L"x√y",   L"xʸ");
  label(IDK_SC_POW10, L"2ˣ",    L"10ˣ");
  label(IDK_SC_LOG,   L"logy",  L"log");
  label(IDK_SC_LN,    L"eˣ",    L"ln");
}

// ---------- 切到科学模式：刷新显示 + 恢复 2nd/角度制按钮状态 ----------
void ScientificPanel::OnShow() {
  KeypadPanel::OnShow();          // 基类刷新表达式与结果
  ApplySecond();
  RefreshAngleChecks(memPad_, angle_);
  pad_.SetChecked(IDK_SC_2ND, second_);
}

// ---------- 快捷字符输入：p/P → π，e → e，^ → ^，! → ! ----------
bool ScientificPanel::OnChar(wchar_t c) {
  switch (c) {
    case L'p': case L'P': Insert(L"π");  return true;
    case L'e':            Insert(L"e");  return true;  // 科学模式无程序员 A-F 输入
    case L'^':            Insert(L"^");  return true;
    case L'!':            Insert(L"!");  return true;
    default:
      return KeypadPanel::OnChar(c);     // 数字/+-*/./(/)/,/%
  }
}
