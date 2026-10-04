#include "IntegerEngine.h"

// ============================================================
// 程序员模式整数引擎：任意进制 + 位运算 + 字长掩码
// ============================================================
namespace IntEng {
namespace {

enum class TT { Num, Op, LP, RP, Bad };

struct Tok {
  TT t = TT::Bad;
  std::wstring s;
  size_t pos = 0, len = 0;
  uint64_t val = 0;
  int prec = 0;
  bool right = false;
  bool unary = false;
};

bool IsHexDigit(wchar_t c) {
  return (c >= L'0' && c <= L'9') || (c >= L'a' && c <= L'f') || (c >= L'A' && c <= L'F');
}
int DigitValue(wchar_t c) {
  if (c >= L'0' && c <= L'9') return c - L'0';
  if (c >= L'a' && c <= L'f') return c - L'a' + 10;
  if (c >= L'A' && c <= L'F') return c - L'A' + 10;
  return -1;
}

std::vector<Tok> Tokenize(const std::wstring& s, unsigned base) {
  std::vector<Tok> toks;
  size_t i = 0, n = s.size();
  while (i < n) {
    wchar_t c = s[i];
    if (c == L' ') { i++; continue; }
    if (IsHexDigit(c) || c == L'.') {
      size_t st = i;
      bool any = false;
      uint64_t v = 0;
      while (i < n && IsHexDigit(s[i])) {
        int d = DigitValue(s[i]);
        if (d >= (int)base) break;          // 数字超出进制范围 → 视为非法
        v = v * base + d;
        i++; any = true;
      }
      if (!any) { toks.push_back({TT::Bad, s.substr(i, 1), i, 1}); i++; continue; }
      toks.push_back({TT::Num, s.substr(st, i - st), st, i - st, v});
      continue;
    }
    // 双字符运算符
    if (c == L'<' && i + 1 < n && s[i + 1] == L'<') {
      toks.push_back({TT::Op, L"<<", i, 2, 0, 4}); i += 2; continue;
    }
    if (c == L'>' && i + 1 < n && s[i + 1] == L'>') {
      toks.push_back({TT::Op, L">>", i, 2, 0, 4}); i += 2; continue;
    }
    if (c == L'~' && i + 1 < n && s[i + 1] == L'&') {
      toks.push_back({TT::Op, L"~&", i, 2, 0, 2}); i += 2; continue;   // NAND
    }
    if (c == L'~' && i + 1 < n && s[i + 1] == L'|') {
      toks.push_back({TT::Op, L"~|", i, 2, 0, 2}); i += 2; continue;   // NOR
    }
    switch (c) {
      case L'(':
        toks.push_back({TT::LP, L"(", i, 1}); i++; continue;
      case L')':
        toks.push_back({TT::RP, L")", i, 1}); i++; continue;
      case L'+':
        toks.push_back({TT::Op, L"+", i, 1, 0, 6}); i++; continue;
      case L'-': case L'−': {
        bool unary = toks.empty();
        if (!unary) {
          const Tok& p = toks.back();
          unary = p.t == TT::Op || p.t == TT::LP;
        }
        toks.push_back({TT::Op, unary ? L"u-" : L"-", i, 1, 0, unary ? 9 : 6, unary, unary});
        i++; continue;
      }
      case L'*': case L'×':
        toks.push_back({TT::Op, L"*", i, 1, 0, 8}); i++; continue;
      case L'/': case L'÷':
        toks.push_back({TT::Op, L"/", i, 1, 0, 8}); i++; continue;
      case L'%':
        toks.push_back({TT::Op, L"%", i, 1, 0, 8}); i++; continue;
      case L'&':
        toks.push_back({TT::Op, L"&", i, 1, 0, 3}); i++; continue;
      case L'^':
        toks.push_back({TT::Op, L"^", i, 1, 0, 2}); i++; continue;
      case L'|':
        toks.push_back({TT::Op, L"|", i, 1, 0, 1}); i++; continue;
      case L'~': {
        bool unary = toks.empty();
        if (!unary) {
          const Tok& p = toks.back();
          unary = p.t == TT::Op || p.t == TT::LP;
        }
        toks.push_back({TT::Op, L"~", i, 1, 0, 9, true, unary});
        i++; continue;
      }
      default:
        toks.push_back({TT::Bad, s.substr(i, 1), i, 1}); i++; continue;
    }
  }
  return toks;
}

bool ToRPN(const std::vector<Tok>& toks, std::vector<Tok>& out, std::wstring& err) {
  std::vector<Tok> st;
  for (const Tok& t : toks) {
    if (t.t == TT::Bad) { err = L"输入无效"; return false; }
    switch (t.t) {
      case TT::Num:
        out.push_back(t); break;
      case TT::LP:
        st.push_back(t); break;
      case TT::RP: {
        bool found = false;
        while (!st.empty()) {
          if (st.back().t == TT::LP) { found = true; st.pop_back(); break; }
          out.push_back(st.back());
          st.pop_back();
        }
        if (!found) { err = L"括号不匹配"; return false; }
        break;
      }
      case TT::Op: {
        while (!st.empty() && st.back().t == TT::Op) {
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

}  // namespace

uint64_t Mask(unsigned bits) {
  return bits >= 64 ? ~0ull : ((1ull << bits) - 1);
}

bool Evaluate(const std::wstring& expr, unsigned base, unsigned bits,
              uint64_t& out, std::wstring& err) {
  if (expr.empty()) { err = L"输入无效"; return false; }
  if (base < 2 || base > 16) { err = L"输入无效"; return false; }
  auto toks = Tokenize(expr, base);
  std::vector<Tok> rpn;
  if (!ToRPN(toks, rpn, err)) return false;
  const uint64_t mask = Mask(bits);
  std::vector<uint64_t> st;
  for (const Tok& t : rpn) {
    switch (t.t) {
      case TT::Num:
        st.push_back(t.val);
        break;
      case TT::Op: {
        if (t.s == L"~" || (t.unary && t.s == L"u-")) {
          if (st.empty()) { err = L"输入无效"; return false; }
          uint64_t a = st.back() & mask;
          st.back() = t.s == L"~" ? (~a & mask) : ((0ull - a) & mask);
          break;
        }
        if (st.size() < 2) { err = L"输入无效"; return false; }
        uint64_t b = st.back() & mask; st.pop_back();
        uint64_t a = st.back() & mask; st.pop_back();
        uint64_t r;
        if (t.s == L"+") r = (a + b) & mask;
        else if (t.s == L"-") r = (a - b) & mask;
        else if (t.s == L"*") r = (a * b) & mask;
        else if (t.s == L"/") {
          if (b == 0) { err = L"无法除以零"; return false; }
          r = (a / b) & mask;
        } else if (t.s == L"%") {
          if (b == 0) { err = L"无法除以零"; return false; }
          r = (a % b) & mask;
        } else if (t.s == L"<<") r = (b >= bits ? 0 : (a << b)) & mask;
        else if (t.s == L">>") r = (b >= bits ? 0 : (a >> b)) & mask;
        else if (t.s == L"&") r = (a & b) & mask;
        else if (t.s == L"^") r = (a ^ b) & mask;
        else if (t.s == L"|") r = (a | b) & mask;
        else if (t.s == L"~&") r = (~(a & b)) & mask;
        else if (t.s == L"~|") r = (~(a | b)) & mask;
        else { err = L"输入无效"; return false; }
        st.push_back(r);
        break;
      }
      default: break;
    }
  }
  if (st.size() != 1) { err = L"输入无效"; return false; }
  out = st.back() & mask;
  return true;
}

bool InsertToken(const std::wstring& expr, const std::wstring& token,
                 unsigned base, std::wstring& out) {
  out = expr;
  auto toks = Tokenize(expr, base);
  const Tok* last = toks.empty() ? nullptr : &toks.back();

  auto append = [&](const std::wstring& t) {
    if (!out.empty() && out.back() != L' ') out += L' ';
    out += t;
  };

  // 数字 token（含 A-F）
  if (!token.empty() && IsHexDigit(token[0])) {
    int d = DigitValue(token[0]);
    if (d >= (int)base) return false;                       // 当前进制不允许该数字
    if (out.empty()) { out = token; return true; }
    if (!last) { out = token; return true; }
    if (last->t == TT::Num) { out += token; return true; }
    if (last->t == TT::Op || last->t == TT::LP) { append(token); return true; }
    if (last->t == TT::RP) { append(L"×"); out += L" "; out += token; return true; }
    return false;
  }
  // 运算符
  if (token == L"+" || token == L"-" || token == L"−" || token == L"*" || token == L"×" ||
      token == L"/" || token == L"÷" || token == L"%" || token == L"&" || token == L"|" ||
      token == L"^") {
    if (out.empty()) { out = L"0"; append(token); return true; }
    if (last && last->t == TT::Op) {
      // 合并移位运算符：< + < => << ；> + > => >>
      if ((token == L"<" && last->s == L"<") || (token == L">" && last->s == L">")) {
        out = out.substr(0, last->pos) + (token == L"<" ? L"<<" : L">>") + out.substr(last->pos + last->len);
        return true;
      }
      out = out.substr(0, last->pos);
      while (!out.empty() && out.back() == L' ') out.pop_back();
      append(token);
      return true;
    }
    if (last && last->t == TT::LP) {
      if (token == L"-" || token == L"−" || token == L"~") { out += token; return true; }
      return false;
    }
    append(token);
    return true;
  }
  if (token == L"<" || token == L">") {
    if (!last) return false;
    if (last->t == TT::Op) {
      if ((token == L"<" && last->s == L"<") || (token == L">" && last->s == L">")) {
        out = out.substr(0, last->pos) + (token == L"<" ? L"<<" : L">>") + out.substr(last->pos + last->len);
        return true;
      }
      return false;
    }
    if (last->t == TT::LP) return false;
    append(token);
    return true;
  }
  if (token == L"~") {
    if (out.empty()) { out = token; return true; }
    if (last && last->t == TT::Op) return false;            // 已是 ~/其他运算符
    if (last && last->t == TT::LP) { out += token; return true; }
    append(token);
    return true;
  }
  if (token == L"(") {
    if (out.empty()) { out = token; return true; }
    if (last && (last->t == TT::Num || last->t == TT::RP)) {
      append(L"×"); out += L" "; out += token;
      return true;
    }
    append(token);
    return true;
  }
  if (token == L")") {
    int depth = 0;
    for (auto& t : toks) { if (t.t == TT::LP) depth++; else if (t.t == TT::RP) depth--; }
    if (depth <= 0) return false;
    if (!last || (last->t != TT::Num && last->t != TT::RP)) return false;
    out += L" )";
    return true;
  }
  return false;
}

bool BackspaceExpr(const std::wstring& expr, std::wstring& out) {
  auto toks = Tokenize(expr, 16);
  if (toks.empty()) return false;
  const Tok& last = toks.back();
  size_t cut = last.pos;
  if (last.t == TT::Num && last.len > 1) cut = last.pos + last.len - 1;
  out = expr.substr(0, cut);
  while (!out.empty() && out.back() == L' ') out.pop_back();
  return true;
}

}  // namespace IntEng
