#pragma once
#include "Common.h"
// ============================================================
// 计算历史：内存列表 + 持久化到 %APPDATA%\W11Calc\history.txt
// ============================================================
namespace History {
  struct Entry {
    std::wstring expr;    // 计算式
    std::wstring result;  // 结果
    std::wstring time;    // 记录时间 "HH:MM:SS"
  };
  class Store {
  public:
    Store();
    void Add(const std::wstring& expr, const std::wstring& result);
    void Clear();
    void Remove(size_t index);
    const std::vector<Entry>& Entries() const { return entries_; }
  private:
    void Load();
    void Save();
    std::vector<Entry> entries_;
    std::wstring path_;
    static const size_t kMax = 500;   // 最多保存条数
  };
}
