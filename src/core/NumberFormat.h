#pragma once
#include "Common.h"
// ============================================================
// 数字格式化工具：负责各种进制、科学计数、千分位的显示
// ============================================================
namespace NumFmt {
  // 无千分位的原始数字串（用于拼回表达式，例如 "5000"）
  std::wstring FormatRaw(double v);
  // 带千分位的显示串（例如 "5,000.25"）
  std::wstring FormatDouble(double v);
  // 给一段文本中的连续数字串加千分位（用于表达式行显示）
  std::wstring GroupExpr(const std::wstring& s);
  // 任意进制整数输出：16 进制每 4 位分组、8 进制每 3 位、2 进制每 4 位、10 进制每 3 位
  std::wstring FormatInteger(uint64_t v, unsigned base, bool upper = true);
  // 按位宽把无符号数解释为有符号十进制（程序员模式）
  std::wstring FormatSigned(uint64_t v, unsigned bits);
  // 坐标轴短格式（如 1.5k、2M）
  std::wstring FormatCompact(double v);
}
