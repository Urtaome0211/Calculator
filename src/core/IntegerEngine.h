#pragma once
#include "Common.h"
// ============================================================
// 程序员模式整数引擎：任意进制 + 位运算 + 字长掩码
// 支持 + - * / % << >> & | ^ ~ ( ) 以及 A-F 十六进制数字
// ============================================================
namespace IntEng {
  // 字长掩码：bits 取 8/16/32/64
  uint64_t Mask(unsigned bits);
  // 按 base 进制解释 expr 中的数字字面量并求值
  bool Evaluate(const std::wstring& expr, unsigned base, unsigned bits,
                uint64_t& out, std::wstring& err);
  // 智能插入 token（数字/运算符），数字合法性按 base 判断
  bool InsertToken(const std::wstring& expr, const std::wstring& token,
                   unsigned base, std::wstring& out);
  bool BackspaceExpr(const std::wstring& expr, std::wstring& out);
}
