// ============================================================
// 核心引擎单元测试（控制台）：表达式 / 整数 / 单位 / 日期 / 格式化
// 编译：见 tests\build_tests.bat
// ============================================================
#include "../src/Common.h"
#include <locale.h>
#include "../src/core/ExpressionEngine.h"
#include "../src/core/IntegerEngine.h"
#include "../src/core/UnitConverter.h"
#include "../src/core/DateCalculator.h"
#include "../src/core/NumberFormat.h"

static int g_fail = 0, g_pass = 0;

#define CHECK(cond, name)                                    \
  do {                                                       \
    if (cond) { g_pass++; }                                  \
    else { g_fail++; wprintf(L"[FAIL] %s\n", name); }         \
  } while (0)

static bool Eval(const std::wstring& e, double expect, const wchar_t* name,
                 Expr::AngleMode ang = Expr::AngleMode::Deg) {
  double v;
  std::wstring err;
  if (!Expr::Evaluate(e, v, err, ang)) {
    wprintf(L"[FAIL] %s : 求值失败 (%s)\n", name, err.c_str());
    g_fail++;
    return false;
  }
  if (std::fabs(v - expect) > 1e-9 * std::max(1.0, std::fabs(expect))) {
    wprintf(L"[FAIL] %s : got %.12g expect %.12g\n", name, v, expect);
    g_fail++;
    return false;
  }
  g_pass++;
  return true;
}

static bool EvalErr(const std::wstring& e, const wchar_t* name) {
  double v;
  std::wstring err;
  if (Expr::Evaluate(e, v, err)) {
    wprintf(L"[FAIL] %s : 应当失败却得到 %.12g\n", name, v);
    g_fail++;
    return false;
  }
  g_pass++;
  return true;
}

static bool IntEval(const std::wstring& e, unsigned base, unsigned bits, uint64_t expect, const wchar_t* name) {
  uint64_t v;
  std::wstring err;
  if (!IntEng::Evaluate(e, base, bits, v, err)) {
    wprintf(L"[FAIL] %s : 求值失败 (%s)\n", name, err.c_str());
    g_fail++;
    return false;
  }
  if (v != expect) {
    wprintf(L"[FAIL] %s : got %llu expect %llu\n", name, (unsigned long long)v, (unsigned long long)expect);
    g_fail++;
    return false;
  }
  g_pass++;
  return true;
}

int wmain() {
  _wsetlocale(LC_ALL, L"");
  wprintf(L"=== 计算器核心引擎测试 ===\n");

  // ---- 表达式引擎 ----
  Eval(L"2+3*4", 14, L"2+3*4");
  Eval(L"(2+3)*4", 20, L"(2+3)*4");
  Eval(L"-2^2", -4, L"-2^2 = -(2^2)");
  Eval(L"2^-3", 0.125, L"2^-3");
  Eval(L"(-2)^2", 4, L"(-2)^2");
  Eval(L"10/4", 2.5, L"10/4");
  Eval(L"7%3", 1, L"7%3");
  Eval(L"5!", 120, L"5!");
  Eval(L"sqrt(9)", 3, L"sqrt(9)");
  Eval(L"2π", 2 * 3.14159265358979323846, L"2π 隐式乘");
  Eval(L"2(3+1)", 8, L"2(3+1)");
  Eval(L"sin(30)", 0.5, L"sin(30°)");
  Eval(L"sin(pi/2)", 1.0, L"sin(pi/2) 弧度", Expr::AngleMode::Rad);
  Eval(L"xroot(3,8)", 2, L"xroot(3,8)");
  Eval(L"logy(2,8)", 3, L"logy(2,8)");
  Eval(L"mod(10,3)", 1, L"mod(10,3)");
  Eval(L"sqr(4)", 16, L"sqr(4)");
  Eval(L"1e3+5", 1005, L"科学计数 1e3+5");
  Eval(L"2(3)(4)", 24, L"连续隐式乘");
  Eval(L"sin(30)cos(30)", 0.4330127018922193, L"函数间隐式乘");
  {
    // 隐式应用：sin30
    double v; std::wstring err;
    if (Expr::Evaluate(L"sin30", v, err)) { CHECK(std::fabs(v - 0.5) < 1e-9, L"sin30 隐式应用"); }
    else { g_fail++; wprintf(L"[FAIL] sin30 : %s\n", err.c_str()); }
  }
  {
    // 百分比
    std::wstring out;
    if (Expr::ApplyPercent(L"50+10%", out)) Eval(out, 55, L"50+10%");
    else { g_fail++; wprintf(L"[FAIL] 50+10%%\n"); }
    if (Expr::ApplyPercent(L"200×10%", out)) Eval(out, 20, L"200×10%");
    else { g_fail++; wprintf(L"[FAIL] 200×10%%\n"); }
    if (Expr::ApplyPercent(L"50%", out)) Eval(out, 0.5, L"50%");
    else { g_fail++; wprintf(L"[FAIL] 50%%\n"); }
  }
  {
    // 插入/退格/CE
    std::wstring e = L"2+3", out;
    CHECK(Expr::InsertToken(e, L"5", out) && out == L"2+35", L"数字拼接 2+35");
    CHECK(Expr::BackspaceExpr(out, e) && e == L"2+3", L"退格一位");
    CHECK(Expr::InsertToken(L"2+", L"×", out) && out == L"2 ×", L"运算符替换");
    CHECK(Expr::InsertToken(L"2", L"(", out) && out == L"2 × (", L"隐式乘括号");
    CHECK(Expr::InsertToken(L"", L".", out) && out == L"0.", L"小数点开头");
    CHECK(!Expr::InsertToken(L"2.", L".", out), L"拒绝重复小数点");
    CHECK(Expr::ClearEntry(L"2+3", out) && out == L"2+", L"CE 清除当前数");
  }
  EvalErr(L"1/0", L"除零报错");
  EvalErr(L"2+", L"残缺表达式");
  EvalErr(L"sqrt(-1)", L"负数开方");

  // ---- 程序员整数引擎 ----
  IntEval(L"FF+1", 16, 64, 0x100, L"FF+1");
  IntEval(L"7&3", 16, 64, 3, L"7&3");
  IntEval(L"1<<4", 10, 64, 16, L"1<<4");
  IntEval(L"256>>4", 10, 64, 16, L"256>>4");
  IntEval(L"F ^ A", 16, 64, 5, L"F XOR A");
  IntEval(L"~0", 16, 8, 0xFF, L"~0 (BYTE)");
  IntEval(L"FFFFFFFF", 16, 32, 0xFFFFFFFFull, L"DWORD 掩码");
  IntEval(L"0-1", 16, 8, 0xFF, L"0-1 补码回绕");
  IntEval(L"A ~& 3", 16, 64, ~(0xA & 3), L"NAND");
  CHECK(NumFmt::FormatSigned(0xFFFFFFFFull, 32) == L"-1", L"有符号显示 -1");
  CHECK(NumFmt::FormatInteger(0x7FFFFFFFull, 16) == L"7FFF FFFF", L"十六进制分组");

  // ---- 单位转换 ----
  {
    double v;
    CHECK(UnitConv::Convert(L"length", L"千米", L"米", 1.0, v) && std::fabs(v - 1000) < 1e-9, L"1km=1000m");
    CHECK(UnitConv::Convert(L"length", L"英里", L"千米", 1.0, v) && std::fabs(v - 1.609344) < 1e-9, L"1mi=1.609344km");
    CHECK(UnitConv::Convert(L"temperature", L"摄氏度 (°C)", L"华氏度 (°F)", 0.0, v) && std::fabs(v - 32) < 1e-9, L"0°C=32°F");
    CHECK(UnitConv::Convert(L"temperature", L"华氏度 (°F)", L"摄氏度 (°C)", 212.0, v) && std::fabs(v - 100) < 1e-9, L"212°F=100°C");
    CHECK(UnitConv::Convert(L"currency", L"美元 (USD)", L"人民币 (CNY)", 1.0, v) && std::fabs(v - 7.19) < 1e-9, L"1USD=7.19CNY");
    CHECK(UnitConv::Convert(L"weight", L"千克", L"斤", 1.0, v) && std::fabs(v - 2) < 1e-9, L"1kg=2斤");
    CHECK(!UnitConv::Convert(L"length", L"不存在的单位", L"米", 1.0, v), L"未知单位失败");
  }

  // ---- 日期 ----
  {
    DateCalc::Date a{2024, 1, 31}, b{2024, 3, 1};
    long long total; int y, m, d;
    DateCalc::Diff(a, b, total, y, m, d);
    CHECK(total == 30 && y == 0 && m == 1 && d == 1, L"Diff(2024-01-31→03-01)=30天/0年1月1天");
    DateCalc::Date c = DateCalc::AddMonths(a, 1);
    CHECK(c.y == 2024 && c.m == 2 && c.d == 29, L"月末裁剪 2024-02-29");
    CHECK(DateCalc::WeekdayName(DateCalc::Date{1970, 1, 1}) == L"周四", L"1970-01-01 周四");
    DateCalc::Date p;
    CHECK(DateCalc::Parse(L"2024-03-01", p) && p.y == 2024 && p.m == 3 && p.d == 1, L"标准格式解析");
    CHECK(!DateCalc::Parse(L"2024-13-01", p) || !DateCalc::IsValid(p), L"拒绝无效日期");
  }

  // ---- 数字格式化 ----
  CHECK(NumFmt::FormatDouble(1234567.5) == L"1,234,567.5", L"千分位");
  CHECK(NumFmt::FormatRaw(5000.0) == L"5000", L"原始格式");
  CHECK(NumFmt::GroupExpr(L"5000+300000") == L"5,000+300,000", L"表达式分组");
  CHECK(NumFmt::FormatInteger(0xABCDu, 16) == L"ABCD", L"十六进制输出");

  wprintf(L"\n==== 通过 %d / 失败 %d ====\n", g_pass, g_fail);
  return g_fail == 0 ? 0 : 1;
}
