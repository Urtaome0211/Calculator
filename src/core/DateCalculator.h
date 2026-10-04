#pragma once
#include "Common.h"
// ============================================================
// 日期计算：日期差 / 加减天数、月数、年数（含闰年与月末裁剪）
// ============================================================
namespace DateCalc {
  struct Date { int y = 0, m = 0, d = 0; };
  bool Parse(const std::wstring& s, Date& out);        // 支持 YYYY-MM-DD / YYYY/M/D
  std::wstring Format(const Date& d);                  // YYYY-MM-DD
  bool IsValid(const Date& d);
  Date Today();
  long long ToDays(const Date& d);                     // 距公元 1-1-1 的天数
  Date FromDays(long long n);
  Date AddDays(const Date& d, long long n);
  Date AddMonths(const Date& d, long long n);          // 月末自动裁剪
  Date AddYears(const Date& d, long long n);
  // b - a 的差：总天数 + 分解为 年/月/日
  void Diff(const Date& a, const Date& b, long long& totalDays, int& years, int& months, int& days);
  std::wstring WeekdayName(const Date& d);             // 周一 … 周日
}
