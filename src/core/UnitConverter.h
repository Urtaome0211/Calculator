#pragma once
#include "Common.h"
// ============================================================
// 单位转换器：货币 / 体积 / 长度 / 重量 / 温度 / 面积 / 速度 /
// 时间 / 能量 / 压力 / 功率 / 角度 / 数据存储 / 频率 共 14 类
// ============================================================
namespace UnitConv {
  struct Category {
    std::wstring id;                        // 内部标识，如 "length"
    std::wstring name;                      // 中文显示名，如 "长度"
    std::vector<std::wstring> units;        // 该类别全部单位（中文名）
  };
  const std::vector<Category>& Categories();
  const Category* Find(const std::wstring& id);
  // 换算：失败返回 false；温度等非线性类别内部处理偏移
  bool Convert(const std::wstring& catId, const std::wstring& from,
               const std::wstring& to, double v, double& out);
}
