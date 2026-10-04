#include "NumberFormat.h"

namespace NumFmt {
namespace {
void TrimZeros(std::wstring& s) {
  // 仅在小数部分末尾去 0（整数部分的 0 有意义）
  if (s.find(L'.') == std::wstring::npos) return;
  size_t e = s.find_last_not_of(L'0');
  if (e == std::wstring::npos) { s = L"0"; return; }
  if (s[e] == L'.') e--;
  s.erase(e + 1);
}
}

// 无千分位、去尾零的数字串，用于拼回表达式
std::wstring FormatRaw(double v) {
  if (std::isnan(v)) return L"0";
  if (std::isinf(v)) return v > 0 ? L"1e999" : L"-1e999";
  if (v == 0.0) return L"0";
  double a = std::fabs(v);
  wchar_t buf[64];
  if (a >= 1e15 || a < 1e-9) {
    swprintf(buf, 64, L"%.10e", v);          // 科学计数
    std::wstring s(buf);
    size_t ep = s.find(L'e');
    if (ep != std::wstring::npos) {
      std::wstring man = s.substr(0, ep);
      TrimZeros(man);
      s = man + s.substr(ep);                // 指数部分保留
    }
    return s;
  }
  swprintf(buf, 64, L"%.15g", v);
  std::wstring s(buf);
  TrimZeros(s);
  if (s == L"-0") s = L"0";
  return s;
}

std::wstring GroupDigits(const std::wstring& s) {
  // 给纯数字串的整数部分加千分位
  std::wstring out;
  size_t i = 0, n = s.size();
  if (i < n && (s[i] == L'-' || s[i] == L'+')) { out += s[i]; i++; }
  size_t start = i;
  while (i < n && s[i] != L'.' && s[i] != L'e' && s[i] != L'E') i++;
  std::wstring intPart = s.substr(start, i - start);
  for (size_t k = 0; k < intPart.size(); k++) {
    if (k > 0 && (intPart.size() - k) % 3 == 0) out += L',';
    out += intPart[k];
  }
  out += s.substr(i);
  return out;
}

std::wstring GroupExpr(const std::wstring& s) {
  // 表达式中连续数字段加千分位（不影响运算符/函数/字母）
  std::wstring out;
  size_t i = 0, n = s.size();
  while (i < n) {
    wchar_t c = s[i];
    bool numStart = (c >= L'0' && c <= L'9') ||
                    (c == L'.' && i + 1 < n && s[i + 1] >= L'0' && s[i + 1] <= L'9');
    if (numStart) {
      size_t st = i;
      while (i < n && ((s[i] >= L'0' && s[i] <= L'9') || s[i] == L'.')) i++;
      size_t ep = i;
      if (i < n && (s[i] == L'e' || s[i] == L'E')) {
        i++;
        if (i < n && (s[i] == L'+' || s[i] == L'-')) i++;
        while (i < n && s[i] >= L'0' && s[i] <= L'9') i++;
      }
      out += GroupDigits(s.substr(st, i - st - (ep < i ? i - ep : 0)));
      if (ep < i) out += s.substr(ep, i - ep);
    } else { out += c; i++; }
  }
  return out;
}

std::wstring FormatDouble(double v) { return GroupDigits(FormatRaw(v)); }

std::wstring FormatInteger(uint64_t v, unsigned base, bool upper) {
  if (v == 0) return L"0";
  static const wchar_t* digitsU = L"0123456789ABCDEF";
  static const wchar_t* digitsL = L"0123456789abcdef";
  const wchar_t* digits = upper ? digitsU : digitsL;
  wchar_t tmp[80];
  int len = 0;
  while (v > 0) { tmp[len++] = digits[v % base]; v /= base; }
  // 分组：16 进制/2 进制每 4 位，8 进制每 3 位，10 进制每 3 位
  unsigned group = base == 8 ? 3 : (base == 10 ? 3 : 4);
  wchar_t sep = base == 10 ? L',' : L' ';
  std::wstring out;
  for (int i = len - 1; i >= 0; i--) {
    out += tmp[i];
    if (i > 0 && (len - i) % group == 0) out += sep;
  }
  return out;
}

std::wstring FormatSigned(uint64_t v, unsigned bits) {
  if (bits >= 64) return FormatDouble((double)(int64_t)v);
  // 按位宽符号扩展
  uint64_t sign = 1ull << (bits - 1);
  int64_t sv = (int64_t)((v ^ sign) - sign);
  return FormatDouble((double)sv);
}

std::wstring FormatCompact(double v) {
  double a = std::fabs(v);
  wchar_t buf[48];
  const wchar_t* suffix = L"";
  double scale = 1.0;
  if (a >= 1e12) { suffix = L"T"; scale = 1e12; }
  else if (a >= 1e9) { suffix = L"G"; scale = 1e9; }
  else if (a >= 1e6) { suffix = L"M"; scale = 1e6; }
  else if (a >= 1e3) { suffix = L"k"; scale = 1e3; }
  else { swprintf(buf, 48, L"%.4g", v); return buf; }
  swprintf(buf, 48, L"%.3g%s", v / scale, suffix);
  return buf;
}
}  // namespace NumFmt
