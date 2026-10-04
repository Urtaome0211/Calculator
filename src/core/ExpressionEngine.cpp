#include "ExpressionEngine.h"
#include "NumberFormat.h"
#include <random>

// ============================================================
// 表达式引擎实现：分词 -> 隐式乘法补全 -> 调度场算法 -> 逆波兰求值
// ============================================================
namespace Expr {
namespace {

enum class TT { Num, Op, LP, RP, Func, Const, Var, Comma, Fact, Mark, Bad };

struct Tok {
  TT t = TT::Bad;
  std::wstring s;
  size_t pos = 0, len = 0;
  int prec = 0;
  bool right = false;    // 右结合
  bool unary = false;
};

const double PI = 3.14159265358979323846;
const double E = 2.71828182845904523536;
const double PHI = 1.61803398874989484820;

std::mt19937_64& Rng() {
  static std::mt19937_64 r(std::random_device{}());
  return r;
}

bool IsDigit(wchar_t c) { return c >= L'0' && c <= L'9'; }
bool IsLetter(wchar_t c) { return (c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z'); }

bool IsFuncName(const std::wstring& w) {
  return w == L"sin" || w == L"cos" || w == L"tan" || w == L"asin" || w == L"acos" ||
         w == L"atan" || w == L"log" || w == L"ln" || w == L"sqrt" || w == L"abs" ||
         w == L"exp" || w == L"mod" || w == L"pow10" || w == L"pow2" || w == L"xroot" ||
         w == L"logy" || w == L"rand" || w == L"sqr";
}

std::vector<Tok> Tokenize(const std::wstring& s) {
  std::vector<Tok> toks;
  size_t i = 0, n = s.size();
  while (i < n) {
    wchar_t c = s[i];
    if (c == L' ') { i++; continue; }
    // 数字（含小数点与科学计数）
    if (IsDigit(c) || (c == L'.' && i + 1 < n && IsDigit(s[i + 1]))) {
      size_t st = i;
      bool any = false;
      while (i < n && IsDigit(s[i])) { i++; any = true; }
      if (i < n && s[i] == L'.') {
        i++;
        while (i < n && IsDigit(s[i])) i++;
        any = true;
      }
      if (any && i < n && (s[i] == L'e' || s[i] == L'E')) {
        size_t save = i;
        i++;
        if (i < n && (s[i] == L'+' || s[i] == L'-')) i++;
        size_t d0 = i;
        while (i < n && IsDigit(s[i])) i++;
        if (d0 == i) i = save;   // 无指数数字，回退
      }
      toks.push_back({TT::Num, s.substr(st, i - st), st, i - st});
      continue;
    }
    // 标识符：函数 / 常量 / 变量
    if (IsLetter(c)) {
      size_t st = i;
      while (i < n && IsLetter(s[i])) i++;
      std::wstring w = s.substr(st, i - st);
      for (auto& ch : w) ch = towlower(ch);
      if (w == L"pi" || w == L"tau" || w == L"phi" || w == L"e")
        toks.push_back({TT::Const, w, st, i - st});
      else if (w == L"x" || w == L"y")
        toks.push_back({TT::Var, w, st, i - st});
      else if (IsFuncName(w))
        toks.push_back({TT::Func, w, st, i - st});
      else
        toks.push_back({TT::Bad, w, st, i - st});
      continue;
    }
    switch (c) {
      case L'π':
        toks.push_back({TT::Const, L"pi", i, 1}); i++; continue;
      case L'(':
        toks.push_back({TT::LP, L"(", i, 1}); i++; continue;
      case L')':
        toks.push_back({TT::RP, L")", i, 1}); i++; continue;
      case L',':
        toks.push_back({TT::Comma, L",", i, 1}); i++; continue;
      case L'!':
        toks.push_back({TT::Fact, L"!", i, 1}); i++; continue;
      case L'+':
        toks.push_back({TT::Op, L"+", i, 1, 1}); i++; continue;
      case L'-': case L'−': {
        bool unary = toks.empty();
        if (!unary) {
          const Tok& p = toks.back();
          unary = p.t == TT::Op || p.t == TT::LP || p.t == TT::Comma || p.t == TT::Func;
        }
        // 一元负号优先级 3 且右结合，使 -2^2 = -(2^2)，2^-3 = 2^(-3)
        toks.push_back({TT::Op, unary ? L"u-" : L"-", i, 1, unary ? 3 : 1, unary, unary});
        i++; continue;
      }
      case L'*': case L'×':
        toks.push_back({TT::Op, L"*", i, 1, 2}); i++; continue;
      case L'/': case L'÷':
        toks.push_back({TT::Op, L"/", i, 1, 2}); i++; continue;
      case L'%':
        toks.push_back({TT::Op, L"%", i, 1, 2}); i++; continue;
      case L'^':
        toks.push_back({TT::Op, L"^", i, 1, 3, true}); i++; continue;
      default:
        toks.push_back({TT::Bad, s.substr(i, 1), i, 1}); i++; continue;
    }
  }
  return toks;
}

// 隐式乘法：数字/常量/变量/右括号/阶乘 后接 数字/常量/变量/左括号/函数
std::vector<Tok> AddImplicitMul(const std::vector<Tok>& toks) {
  std::vector<Tok> out;
  out.reserve(toks.size() * 2);
  for (size_t k = 0; k < toks.size(); k++) {
    const Tok& t = toks[k];
    if (!out.empty()) {
      const Tok& p = out.back();
      bool prevOperand = p.t == TT::Num || p.t == TT::Const || p.t == TT::Var ||
                         p.t == TT::RP || p.t == TT::Fact;
      bool curOperand = t.t == TT::Num || t.t == TT::Const || t.t == TT::Var ||
                        t.t == TT::LP || t.t == TT::Func;
      if (prevOperand && curOperand)
        out.push_back({TT::Op, L"*", t.pos, 0, 2, false, false});
    }
    out.push_back(t);
  }
  return out;
}

bool ToRPN(const std::vector<Tok>& toks, std::vector<Tok>& out, std::wstring& err) {
  std::vector<Tok> st;
  for (const Tok& t : toks) {
    if (t.t == TT::Bad) { err = L"输入无效"; return false; }
    switch (t.t) {
      case TT::Num: case TT::Const: case TT::Var:
        out.push_back(t); break;
      case TT::Fact:
        out.push_back(t); break;                     // 后缀运算符直接输出
      case TT::Func:
        out.push_back({TT::Mark, L"FRAME", t.pos, 0});  // 参数区域帧标记（求值时界定参数范围）
        st.push_back(t); break;
      case TT::LP:
        st.push_back(t); break;
      case TT::Comma: {
        // 弹出运算符直到左括号（左括号保留在栈上）
        while (!st.empty() && st.back().t != TT::LP) {
          if (st.back().t == TT::Func) { err = L"输入无效"; return false; }
          out.push_back(st.back());
          st.pop_back();
        }
        if (st.empty()) { err = L"输入无效"; return false; }
        break;
      }
      case TT::RP: {
        bool found = false;
        while (!st.empty()) {
          if (st.back().t == TT::LP) { found = true; st.pop_back(); break; }
          out.push_back(st.back());
          st.pop_back();
        }
        if (!found) { err = L"括号不匹配"; return false; }
        // 若括号属于函数参数表，立即把函数弹出（函数参数到此结束）
        if (!st.empty() && st.back().t == TT::Func) {
          out.push_back(st.back());
          st.pop_back();
        }
        break;
      }
      case TT::Op: {
        // 未闭合函数视作最高优先级左结合伪运算符：遇到任何运算符都先弹出
        while (!st.empty() && (st.back().t == TT::Op || st.back().t == TT::Func)) {
          if (st.back().t == TT::Func) {
            out.push_back(st.back());
            st.pop_back();
            continue;
          }
          const Tok& top = st.back();
          if (top.prec > t.prec || (top.prec == t.prec && !t.right)) {
            out.push_back(top);
            st.pop_back();
          } else break;
        }
        st.push_back(t);
        break;
      }
      default: break;
    }
  }
  for (auto it = st.rbegin(); it != st.rend(); ++it) {
    if (it->t == TT::LP) { err = L"括号不匹配"; return false; }
    out.push_back(*it);
  }
  return true;
}

struct Val { double d; bool mark; bool frame; };

double ToRad(double v, AngleMode ang) {
  if (ang == AngleMode::Deg) return v * PI / 180.0;
  if (ang == AngleMode::Grad) return v * PI / 200.0;
  return v;
}
double FromRad(double v, AngleMode ang) {
  if (ang == AngleMode::Deg) return v * 180.0 / PI;
  if (ang == AngleMode::Grad) return v * 200.0 / PI;
  return v;
}

bool EvalRPN(const std::vector<Tok>& rpn, double x, AngleMode ang, double& out, std::wstring& err) {
  std::vector<Val> st;
  for (const Tok& t : rpn) {
    switch (t.t) {
      case TT::Num: {
        wchar_t* e = nullptr;
        double v = wcstod(t.s.c_str(), &e);
        if (e && *e) { err = L"输入无效"; return false; }
        st.push_back({v, false, false});
        break;
      }
      case TT::Const:
        st.push_back({t.s == L"pi" ? PI : t.s == L"tau" ? 2 * PI : t.s == L"phi" ? PHI : E, false, false});
        break;
      case TT::Var:
        st.push_back({x, false, false});
        break;
      case TT::Mark:
        st.push_back({0, false, true});        // 函数参数区域帧标记
        break;
      case TT::Op: {
        if (t.unary) {
          if (st.empty() || st.back().mark) { err = L"输入无效"; return false; }
          st.back().d = -st.back().d;
        } else {
          if (st.size() < 2) { err = L"输入无效"; return false; }
          double b = st.back().d; st.pop_back();
          if (st.empty() || st.back().mark) { err = L"输入无效"; return false; }
          double a = st.back().d; st.pop_back();
          double r;
          if (t.s == L"+") r = a + b;
          else if (t.s == L"-") r = a - b;
          else if (t.s == L"*") r = a * b;
          else if (t.s == L"/") { if (b == 0.0) { err = L"无法除以零"; return false; } r = a / b; }
          else if (t.s == L"%") { if (b == 0.0) { err = L"无法除以零"; return false; } r = fmod(a, b); }
          else if (t.s == L"^") {
            if (a < 0 && b != floor(b)) { err = L"结果未定义"; return false; }
            r = pow(a, b);
            if (std::isnan(r) || std::isinf(r)) { err = a == 0 ? L"结果未定义" : L"溢出"; return false; }
          }
          else { err = L"输入无效"; return false; }
          st.push_back({r, false});
        }
        break;
      }
      case TT::Fact: {
        if (st.empty() || st.back().mark) { err = L"输入无效"; return false; }
        double v = st.back().d;
        if (v < 0 || v != floor(v)) { err = L"结果未定义"; return false; }
        if (v > 170) { err = L"溢出"; return false; }
        double r = 1;
        for (int i = 2; i <= (int)v; i++) r *= i;
        st.back().d = r;
        break;
      }
      case TT::Func: {
        // 弹出参数直到帧标记（含帧标记本身）；帧标记界定本函数的参数区域
        std::vector<double> args;
        while (!st.empty()) {
          if (st.back().frame) { st.pop_back(); break; }
          if (st.back().mark) { st.pop_back(); continue; }
          args.push_back(st.back().d);
          st.pop_back();
        }
        if (args.empty()) { err = L"输入无效"; return false; }
        std::reverse(args.begin(), args.end());
        const std::wstring& f = t.s;
        double r = 0;
        bool ok = false;
        if (args.size() == 1) {
          double v = args[0];
          if (f == L"sin") { r = sin(ToRad(v, ang)); ok = true; }
          else if (f == L"cos") { r = cos(ToRad(v, ang)); ok = true; }
          else if (f == L"tan") { r = tan(ToRad(v, ang)); ok = true; }
          else if (f == L"asin") { if (v < -1 || v > 1) break; r = FromRad(asin(v), ang); ok = true; }
          else if (f == L"acos") { if (v < -1 || v > 1) break; r = FromRad(acos(v), ang); ok = true; }
          else if (f == L"atan") { r = FromRad(atan(v), ang); ok = true; }
          else if (f == L"log") { if (v <= 0) break; r = log10(v); ok = true; }
          else if (f == L"ln") { if (v <= 0) break; r = log(v); ok = true; }
          else if (f == L"sqrt") { if (v < 0) break; r = sqrt(v); ok = true; }
          else if (f == L"abs") { r = fabs(v); ok = true; }
          else if (f == L"exp") { r = exp(v); ok = true; }
          else if (f == L"pow10") { r = pow(10.0, v); ok = true; }
          else if (f == L"pow2") { r = pow(2.0, v); ok = true; }
          else if (f == L"sqr") { r = v * v; ok = true; }
          else if (f == L"rand") { r = (double)Rng()() / (double)Rng().max(); ok = true; }
        } else if (args.size() == 2) {
          if (f == L"mod") { r = fmod(args[0], args[1]); ok = true; }
          else if (f == L"xroot") {
            if (args[0] == 0 || (args[1] < 0 && fmod(fabs(args[0]), 2.0) == 0)) { err = L"结果未定义"; return false; }
            r = pow(args[1], 1.0 / args[0]);
            ok = true;
          } else if (f == L"logy") {
            if (args[0] <= 0 || args[0] == 1 || args[1] <= 0) { err = L"结果未定义"; return false; }
            r = log(args[1]) / log(args[0]);
            ok = true;
          }
        }
        if (!ok) { err = L"结果未定义"; return false; }
        if (std::isnan(r) || std::isinf(r)) { err = L"溢出"; return false; }
        st.push_back({r, false});
        break;
      }
      default: break;
    }
  }
  if (st.size() != 1 || st.back().mark) { err = L"输入无效"; return false; }
  out = st.back().d;
  return true;
}

bool EvalAll(const std::wstring& expr, double x, AngleMode ang, double& out, std::wstring& err) {
  if (expr.empty()) { err = L"输入无效"; return false; }
  auto toks = Tokenize(expr);
  toks = AddImplicitMul(toks);
  std::vector<Tok> rpn;
  if (!ToRPN(toks, rpn, err)) return false;
  return EvalRPN(rpn, x, ang, out, err);
}

// 单个 token 分类（供 InsertToken 使用）
TT ClassifyToken(const std::wstring& token) {
  if (token.empty()) return TT::Bad;
  if (token.back() == L'(' && token.size() > 1) {   // 形如 "sin("
    std::wstring name = token.substr(0, token.size() - 1);
    for (auto& c : name) c = towlower(c);
    return IsFuncName(name) ? TT::Func : TT::Bad;
  }
  if (token == L"(") return TT::LP;
  if (token == L")") return TT::RP;
  if (token == L",") return TT::Comma;
  if (token == L"!") return TT::Fact;
  if (token == L"π" || token == L"e") return TT::Const;
  if (token == L"+" || token == L"-" || token == L"−" || token == L"*" || token == L"×" ||
      token == L"/" || token == L"÷" || token == L"^") return TT::Op;
  if (IsDigit(token[0]) || token[0] == L'.') return TT::Num;
  return TT::Bad;
}

bool EndsWithOp(const std::wstring& expr) {
  if (expr.empty()) return false;
  auto toks = Tokenize(expr);
  return !toks.empty() && toks.back().t == TT::Op && !toks.back().unary;
}

}  // namespace

// ------------------------------------------------------------
// 对外接口
// ------------------------------------------------------------
bool Evaluate(const std::wstring& expr, double& out, std::wstring& err, AngleMode ang) {
  return EvalAll(expr, 0.0, ang, out, err);
}

bool EvaluateX(const std::wstring& expr, double x, double& out, std::wstring& err, AngleMode ang) {
  return EvalAll(expr, x, ang, out, err);
}

bool InsertToken(const std::wstring& expr, const std::wstring& token, std::wstring& out) {
  TT kind = ClassifyToken(token);
  if (kind == TT::Bad) return false;
  out = expr;
  auto toks = Tokenize(expr);
  const Tok* last = toks.empty() ? nullptr : &toks.back();

  // 帮助函数：追加（前导空格）
  auto append = [&](const std::wstring& t) {
    if (!out.empty() && out.back() != L' ') out += L' ';
    out += t;
  };

  switch (kind) {
    case TT::Num: {
      bool isDot = (token == L".");
      if (out.empty()) { out = isDot ? L"0." : token; return true; }
      if (!last) { out = token; return true; }
      if (last->t == TT::Num) {
        if (isDot) {
          if (last->s.find(L'.') != std::wstring::npos) return false;   // 已有小数点
        }
        out += token;
        return true;
      }
      if (last->t == TT::Op || last->t == TT::Func || last->t == TT::LP || last->t == TT::Comma ||
          last->t == TT::Mark) {
        append(isDot ? L"0." : token);
        return true;
      }
      // 右括号 / 常量 / 变量 / 阶乘 之后 → 隐式乘
      append(L"×");
      out += L" ";
      out += isDot ? L"0." : token;
      return true;
    }
    case TT::Op: {
      if (out.empty()) { out = L"0"; append(token); return true; }
      if (EndsWithOp(out)) {
        // 替换末尾运算符（保留一元负号场景：连续运算符时替换最后一个）
        out = out.substr(0, last->pos);
        while (!out.empty() && out.back() == L' ') out.pop_back();
        append(token);
        return true;
      }
      if (last && (last->t == TT::LP || last->t == TT::Func || last->t == TT::Comma)) {
        // 括号后只允许负号
        if (token == L"-" || token == L"−") { out += token; return true; }
        return false;
      }
      append(token);
      return true;
    }
    case TT::LP: {
      if (out.empty()) { out = token; return true; }
      if (last && (last->t == TT::Num || last->t == TT::RP || last->t == TT::Const ||
                   last->t == TT::Var || last->t == TT::Fact)) {
        append(L"×");
        out += L" ";
        out += token;
        return true;
      }
      append(token);
      return true;
    }
    case TT::RP: {
      int depth = 0;
      for (auto& t : toks) { if (t.t == TT::LP) depth++; else if (t.t == TT::RP) depth--; }
      if (depth <= 0) return false;
      if (last && (last->t == TT::Op || last->t == TT::LP || last->t == TT::Func ||
                   last->t == TT::Comma)) return false;
      out += L" )";
      return true;
    }
    case TT::Func: {
      if (out.empty()) { out = token + L" "; return true; }
      if (last && (last->t == TT::Num || last->t == TT::RP || last->t == TT::Const ||
                   last->t == TT::Var || last->t == TT::Fact)) {
        append(L"×");
        out += L" ";
        out += token;
        out += L" ";
        return true;
      }
      append(token);
      out += L" ";
      return true;
    }
    case TT::Const: {
      if (out.empty()) { out = token; return true; }
      if (last && (last->t == TT::Num || last->t == TT::RP || last->t == TT::Const ||
                   last->t == TT::Var || last->t == TT::Fact)) {
        append(L"×");
        out += L" ";
        out += token;
        return true;
      }
      append(token);
      return true;
    }
    case TT::Fact: {
      if (!last) return false;
      if (last->t == TT::Num || last->t == TT::RP) { out += L"!"; return true; }
      return false;
    }
    case TT::Comma: {
      int depth = 0;
      for (auto& t : toks) { if (t.t == TT::LP) depth++; else if (t.t == TT::RP) depth--; }
      if (depth <= 0) return false;
      if (!last || (last->t != TT::Num && last->t != TT::RP && last->t != TT::Const &&
                    last->t != TT::Var)) return false;
      out += L", ";
      return true;
    }
    default:
      return false;
  }
}

bool BackspaceExpr(const std::wstring& expr, std::wstring& out) {
  auto toks = Tokenize(expr);
  if (toks.empty()) return false;
  const Tok& last = toks.back();
  size_t cut = last.pos;
  if (last.t == TT::Num && last.len > 1) cut = last.pos + last.len - 1;  // 数字只删一位
  out = expr.substr(0, cut);
  while (!out.empty() && out.back() == L' ') out.pop_back();
  return true;
}

bool ClearEntry(const std::wstring& expr, std::wstring& out) {
  auto toks = Tokenize(expr);
  if (toks.empty()) return false;
  // 找到最后一个数字 token
  int idx = -1;
  for (int i = (int)toks.size() - 1; i >= 0; i--) {
    if (toks[i].t == TT::Num) { idx = i; break; }
  }
  if (idx < 0) { out = L""; return true; }
  // 若其后（含自己）只有数字与一元符号等，删到该数字开头
  out = expr.substr(0, toks[idx].pos);
  while (!out.empty() && out.back() == L' ') out.pop_back();
  return true;
}

bool ApplyPercent(const std::wstring& expr, std::wstring& out, AngleMode ang) {
  auto toks = Tokenize(expr);
  if (toks.empty()) return false;
  // 最后一个数字 token
  int numIdx = -1;
  for (int i = (int)toks.size() - 1; i >= 0; i--) {
    if (toks[i].t == TT::Num) { numIdx = i; break; }
  }
  if (numIdx < 0) return false;
  const Tok& num = toks[numIdx];
  wchar_t* e = nullptr;
  double v = wcstod(num.s.c_str(), &e);
  // 向左找最近的顶层二元运算符
  int depth = 0, opIdx = -1;
  for (int i = numIdx - 1; i >= 0; i--) {
    if (toks[i].t == TT::RP) depth++;
    else if (toks[i].t == TT::LP) depth--;
    else if (toks[i].t == TT::Op && depth == 0 && !toks[i].unary) { opIdx = i; break; }
  }
  double newV;
  if (opIdx >= 0 && (toks[opIdx].s == L"+" || toks[opIdx].s == L"-")) {
    std::wstring left = expr.substr(0, toks[opIdx].pos);
    while (!left.empty() && left.back() == L' ') left.pop_back();
    double base = 0;
    std::wstring err;
    if (!EvalAll(left, 0.0, ang, base, err)) base = 0;
    newV = base * v / 100.0;
  } else {
    newV = v / 100.0;
  }
  std::wstring replacement = NumFmt::FormatRaw(newV);
  if (opIdx >= 0 && (toks[opIdx].s == L"+" || toks[opIdx].s == L"-") && newV < 0)
    replacement = L"(" + replacement + L")";
  // 替换数字段，并连带消费紧随其后的百分号
  size_t end = num.pos + num.len;
  if (numIdx + 1 < (int)toks.size() && toks[numIdx + 1].t == TT::Op && toks[numIdx + 1].s == L"%")
    end = toks[numIdx + 1].pos + toks[numIdx + 1].len;
  out = expr.substr(0, num.pos) + replacement + expr.substr(end);
  return true;
}

bool NegateEntry(const std::wstring& expr, std::wstring& out) {
  auto toks = Tokenize(expr);
  if (toks.empty()) { out = L"-"; return true; }
  const Tok& last = toks.back();
  if (last.t == TT::Num) {
    double v = wcstod(last.s.c_str(), nullptr);
    double nv = -v;
    const Tok* prev = toks.size() >= 2 ? &toks[toks.size() - 2] : nullptr;
    bool wrap = prev && (prev->t == TT::Op || prev->t == TT::LP || prev->t == TT::Comma);
    std::wstring rep = NumFmt::FormatRaw(nv);
    if (wrap) rep = L"(" + rep + L")";
    out = expr.substr(0, last.pos) + rep + expr.substr(last.pos + last.len);
    return true;
  }
  // 无数字输入项：对整个表达式取反
  double v = 0;
  std::wstring err;
  if (!EvalAll(expr, 0.0, AngleMode::Deg, v, err)) return false;
  out = NumFmt::FormatRaw(-v);
  return true;
}

}  // namespace Expr
