#include "HistoryStore.h"
#include <shlobj.h>
#include <cwchar>
#include <utility>

// ============================================================
// 计算历史：内存列表 + 持久化到 %APPDATA%\W11Calc\history.txt
// 文件格式：UTF-8（带 BOM），每行一条记录，字段以 '\t' 分隔：expr、result、time
// ============================================================
namespace History {
namespace {

// 字段中的 '\t'、'\r'、'\n' 在保存时替换为空格，避免破坏行/字段结构
std::wstring SanitizeField(const std::wstring& s) {
  std::wstring r = s;
  for (wchar_t& c : r) {
    if (c == L'\t' || c == L'\r' || c == L'\n') c = L' ';
  }
  return r;
}

// 当前时间 "HH:MM:SS"
std::wstring NowTime() {
  SYSTEMTIME st;
  GetLocalTime(&st);
  wchar_t buf[16];
  swprintf(buf, 16, L"%02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond);
  return buf;
}

} // namespace

Store::Store() {
  // %APPDATA%\W11Calc\history.txt
  wchar_t appdata[MAX_PATH] = { 0 };
  if (GetEnvironmentVariableW(L"APPDATA", appdata, MAX_PATH) > 0 && appdata[0] != L'\0') {
    const std::wstring dir = std::wstring(appdata) + L"\\W11Calc";
    path_ = dir + L"\\history.txt";
    // 目录不存在时创建（已存在返回 ERROR_ALREADY_EXISTS，忽略返回值）
    SHCreateDirectoryExW(NULL, dir.c_str(), NULL);
  }
  Load();
}

void Store::Load() {
  entries_.clear();
  if (path_.empty()) return;

  HANDLE h = CreateFileW(path_.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
                         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (h == INVALID_HANDLE_VALUE) return; // 文件不存在或打开失败：静默忽略

  std::string bytes;
  char buf[4096];
  DWORD got = 0;
  while (ReadFile(h, buf, sizeof(buf), &got, NULL) && got > 0) bytes.append(buf, got);
  CloseHandle(h);
  if (bytes.empty()) return;

  // 检测并跳过 UTF-8 BOM (EF BB BF)
  if (bytes.size() >= 3 &&
      (unsigned char)bytes[0] == 0xEF && (unsigned char)bytes[1] == 0xBB && (unsigned char)bytes[2] == 0xBF) {
    bytes.erase(0, 3);
  }
  if (bytes.empty()) return;

  // UTF-8 转宽字符
  const int wlen = MultiByteToWideChar(CP_UTF8, 0, bytes.data(), (int)bytes.size(), NULL, 0);
  if (wlen <= 0) return;
  std::wstring text((size_t)wlen, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, bytes.data(), (int)bytes.size(), &text[0], wlen);

  // 按行、按 '\t' 切分；字段不足 3 个的行跳过
  size_t pos = 0;
  while (pos <= text.size()) {
    const size_t nl = text.find(L'\n', pos);
    const size_t end = (nl == std::wstring::npos) ? text.size() : nl;
    std::wstring line = text.substr(pos, end - pos);
    pos = (nl == std::wstring::npos) ? text.size() + 1 : nl + 1;
    if (!line.empty() && line.back() == L'\r') line.pop_back();
    if (line.empty()) continue;

    const size_t t1 = line.find(L'\t');
    if (t1 == std::wstring::npos) continue;
    const size_t t2 = line.find(L'\t', t1 + 1);
    if (t2 == std::wstring::npos) continue;

    Entry e;
    e.expr = line.substr(0, t1);
    e.result = line.substr(t1 + 1, t2 - t1 - 1);
    e.time = line.substr(t2 + 1);
    entries_.push_back(std::move(e));
  }
  if (entries_.size() > kMax) entries_.resize(kMax);
}

void Store::Save() {
  if (path_.empty()) return;

  // 组装 UTF-8 字节流：先写 BOM，再逐行转码
  std::string bytes;
  bytes.push_back((char)0xEF);
  bytes.push_back((char)0xBB);
  bytes.push_back((char)0xBF);
  for (const Entry& e : entries_) {
    const std::wstring line = SanitizeField(e.expr) + L"\t" +
                              SanitizeField(e.result) + L"\t" +
                              SanitizeField(e.time) + L"\r\n";
    const int need = WideCharToMultiByte(CP_UTF8, 0, line.c_str(), (int)line.size(), NULL, 0, NULL, NULL);
    if (need <= 0) continue;
    const size_t off = bytes.size();
    bytes.resize(off + (size_t)need);
    WideCharToMultiByte(CP_UTF8, 0, line.c_str(), (int)line.size(), &bytes[off], need, NULL, NULL);
  }

  HANDLE h = CreateFileW(path_.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL,
                         CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (h == INVALID_HANDLE_VALUE) return; // 写入失败静默忽略
  DWORD written = 0;
  WriteFile(h, bytes.data(), (DWORD)bytes.size(), &written, NULL);
  CloseHandle(h);
}

void Store::Add(const std::wstring& expr, const std::wstring& result) {
  Entry e;
  e.expr = expr;
  e.result = result;
  e.time = NowTime();
  entries_.insert(entries_.begin(), std::move(e)); // 最新在上
  if (entries_.size() > kMax) entries_.resize(kMax); // 丢弃最旧（末尾）
  Save();
}

void Store::Clear() {
  entries_.clear();
  Save();
}

void Store::Remove(size_t index) {
  if (index >= entries_.size()) return; // 越界忽略
  entries_.erase(entries_.begin() + index);
  Save();
}

} // namespace History
