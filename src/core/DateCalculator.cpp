#include "DateCalculator.h"
#include <cwchar>
#include <cwctype>

// ============================================================
// 日期计算实现：基于 Howard Hinnant 民用历算法（proleptic Gregorian），
// ToDays/FromDays 为距 1970-01-01（Unix 纪元）的天数。
// ============================================================
namespace DateCalc {
namespace {

// Hinnant days_from_civil：返回距 1970-01-01 的天数
long long days_from_civil(int y, unsigned m, unsigned d) {
  y -= m <= 2;
  const int era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = (unsigned)(y - era * 400);
  const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097LL + (long long)doe;
}

// Hinnant civil_from_days：天数（距 1970-01-01）转回年月日
Date civil_from_days(long long z) {
  z += 719468;
  const long long era = (z >= 0 ? z : z - 146096) / 146097;
  const unsigned doe = (unsigned)(z - era * 146097);
  const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  int y = (int)yoe + (int)era * 400;
  const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  const unsigned mp = (5 * doy + 2) / 153;
  const unsigned d = doy - (153 * mp + 2) / 5 + 1;
  const unsigned m = mp + (mp < 10 ? 3 : -9);
  y += (m <= 2);
  return Date{ y, (int)m, (int)d };
}

bool IsLeap(int y) { return y % 4 == 0 && (y % 100 != 0 || y % 400 == 0); }

int DaysInMonth(int y, int m) {
  switch (m) {
    case 1: case 3: case 5: case 7: case 8: case 10: case 12: return 31;
    case 4: case 6: case 9: case 11: return 30;
    case 2: return IsLeap(y) ? 29 : 28;
    default: return 0;
  }
}

} // namespace

bool Parse(const std::wstring& s, Date& out) {
  // 去掉前后空格
  size_t b = 0, e = s.size();
  while (b < e && iswspace(s[b])) ++b;
  while (e > b && iswspace(s[e - 1])) --e;
  if (b == e) return false;
  const std::wstring t = s.substr(b, e - b);

  int y = 0, m = 0, d = 0, n = 0;
  bool ok = false;
  if (swscanf(t.c_str(), L"%d-%d-%d%n", &y, &m, &d, &n) == 3 && n == (int)t.size()) {
    // "YYYY-MM-DD"：严格要求定长（4-2-2）
    ok = (t.size() == 10 && t[4] == L'-' && t[7] == L'-');
  } else if (swscanf(t.c_str(), L"%d/%d/%d%n", &y, &m, &d, &n) == 3 && n == (int)t.size()) {
    ok = true; // "YYYY/M/D"：月/日允许不补零
  }
  if (!ok) return false;

  Date r{ y, m, d };
  if (!IsValid(r)) return false;
  out = r;
  return true;
}

std::wstring Format(const Date& d) {
  wchar_t buf[16];
  swprintf(buf, 16, L"%04d-%02d-%02d", d.y, d.m, d.d);
  return buf;
}

bool IsValid(const Date& d) {
  if (d.y < 1 || d.y > 9999) return false;
  if (d.m < 1 || d.m > 12) return false;
  if (d.d < 1) return false;
  return d.d <= DaysInMonth(d.y, d.m);
}

Date Today() {
  SYSTEMTIME st;
  GetLocalTime(&st);
  return Date{ (int)st.wYear, (int)st.wMonth, (int)st.wDay };
}

long long ToDays(const Date& d) { return days_from_civil(d.y, (unsigned)d.m, (unsigned)d.d); }

Date FromDays(long long n) { return civil_from_days(n); }

Date AddDays(const Date& d, long long n) { return FromDays(ToDays(d) + n); }

Date AddMonths(const Date& d, long long n) {
  // 总月数 = (d.m - 1 + n)；注意 C++ 负数除法向零取整，年份用 std::floor
  const long long total = (long long)(d.m - 1) + n;
  const long long y = d.y + (long long)std::floor(total / 12.0);
  long long m = total % 12;
  if (m < 0) m += 12; // 余数为负时加 12 保证月份为正
  m += 1;
  Date r{ (int)y, (int)m, d.d };
  const int maxd = DaysInMonth(r.y, r.m);
  if (r.d > maxd) r.d = maxd; // 月末自动裁剪（如 1-31 加 1 月 -> 2-29）
  return r;
}

Date AddYears(const Date& d, long long n) {
  Date r{ d.y + (int)n, d.m, d.d };
  const int maxd = DaysInMonth(r.y, r.m);
  if (r.d > maxd) r.d = maxd; // 闰日加平年时裁剪为 2-28
  return r;
}

void Diff(const Date& a, const Date& b, long long& totalDays, int& years, int& months, int& days) {
  totalDays = ToDays(b) - ToDays(a);
  years = b.y - a.y;
  months = b.m - a.m;
  days = b.d - a.d;
  if (days < 0) {
    --months;
    Date t = AddMonths(a, (long long)years * 12 + months);
    days = (int)(ToDays(b) - ToDays(t));
  }
  if (months < 0) {
    --years;
    months += 12;
  }
}

std::wstring WeekdayName(const Date& d) {
  // 以 1970-01-01（周四）为锚点计算星期，不依赖 ToDays 的具体纪元偏移
  static const long long kAnchor = ToDays(Date{1970, 1, 1});
  long long wd = (ToDays(d) - kAnchor) % 7;
  if (wd < 0) wd += 7;   // 0 = 周四
  static const wchar_t* kNames[] = { L"周四", L"周五", L"周六", L"周日", L"周一", L"周二", L"周三" };
  return kNames[(size_t)wd];
}

} // namespace DateCalc
