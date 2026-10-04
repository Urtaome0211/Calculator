#pragma once
#include "Common.h"
// ============================================================
// 表达式引擎（标准 / 科学 / 图形模式共用）
// 基于“分词 → 调度场算法 → 逆波兰求值”的数学表达式解析器，
// 支持 + - * / % ^ ! () 、函数、常量、隐式乘法、科学计数法。
// ============================================================
namespace Expr {
  enum class AngleMode { Deg, Rad, Grad };

  // 求值表达式；失败时 err 为中文错误提示
  bool Evaluate(const std::wstring& expr, double& out, std::wstring& err,
                AngleMode ang = AngleMode::Deg);
  // 带自变量 x 的求值（图形模式）
  bool EvaluateX(const std::wstring& expr, double x, double& out, std::wstring& err,
                 AngleMode ang = AngleMode::Rad);
  // 智能插入一个 token：数字拼接、运算符替换、自动补乘号等
  bool InsertToken(const std::wstring& expr, const std::wstring& token, std::wstring& out);
  // 退格：删除最后一个 token（数字只删一位）
  bool BackspaceExpr(const std::wstring& expr, std::wstring& out);
  // CE：清除当前正在输入的数字
  bool ClearEntry(const std::wstring& expr, std::wstring& out);
  // 百分号：按 Windows 计算器规则（50+10% => 50+5）
  bool ApplyPercent(const std::wstring& expr, std::wstring& out, AngleMode ang = AngleMode::Deg);
  // ±：取反当前输入项
  bool NegateEntry(const std::wstring& expr, std::wstring& out);
}
