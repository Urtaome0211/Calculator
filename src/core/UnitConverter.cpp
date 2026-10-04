// ============================================================
// UnitConverter.cpp —— namespace UnitConv 的实现
// 货币 / 体积 / 长度 / 重量 / 温度 / 面积 / 速度 / 时间 / 能量 /
// 压力 / 功率 / 角度 / 数据存储 / 频率 共 14 个类别。
// 除温度（含偏移）外均为线性换算：out = v * factor(from) / factor(to)。
// 文件为 UTF-8 编码（/utf-8 编译），中文一律使用宽字符 L"..."
// ============================================================
#include "UnitConverter.h"

#include <limits>

namespace UnitConv {

namespace {

constexpr double kPi = 3.14159265358979323846;

// 单位 → 基准单位 的换算系数（温度类为非线性，factor 不使用，填 0）
struct UnitF {
  const wchar_t* name;
  double         factor;
};

// ---------------- 货币：基准 人民币 (CNY)，近似参考汇率 ----------------
constexpr UnitF kCurrencyUnits[] = {
  {L"人民币 (CNY)",        1.0},
  {L"美元 (USD)",          7.19},
  {L"欧元 (EUR)",          7.85},
  {L"日元 (JPY)",          0.0482},
  {L"英镑 (GBP)",          9.15},
  {L"港币 (HKD)",          0.919},
  {L"韩元 (KRW)",          0.0053},
  {L"新台币 (TWD)",        0.226},
  {L"澳元 (AUD)",          4.68},
  {L"加元 (CAD)",          5.25},
  {L"新加坡元 (SGD)",      5.35},
  {L"瑞士法郎 (CHF)",      8.30},
  {L"卢布 (RUB)",          0.079},
  {L"印度卢比 (INR)",      0.086},
  {L"泰铢 (THB)",          0.20},
  {L"马来西亚林吉特 (MYR)", 1.55},
};

// ---------------- 体积：基准 升 ----------------
constexpr UnitF kVolumeUnits[] = {
  {L"毫升",     0.001},
  {L"升",       1.0},
  {L"立方米",   1000.0},
  {L"立方厘米", 0.001},
  {L"立方英寸", 0.016387064},
  {L"立方英尺", 28.316846592},
  {L"美制加仑", 3.785411784},
  {L"英制加仑", 4.54609},
  {L"美制品脱", 0.473176473},
  {L"美制夸脱", 0.946352946},
};

// ---------------- 长度：基准 米 ----------------
constexpr UnitF kLengthUnits[] = {
  {L"毫米", 0.001},
  {L"厘米", 0.01},
  {L"米",   1.0},
  {L"千米", 1000.0},
  {L"英寸", 0.0254},
  {L"英尺", 0.3048},
  {L"码",   0.9144},
  {L"英里", 1609.344},
  {L"海里", 1852.0},
  {L"光年", 9.4607304725808e15},
};

// ---------------- 重量：基准 千克 ----------------
constexpr UnitF kWeightUnits[] = {
  {L"毫克", 1e-6},
  {L"克",   0.001},
  {L"千克", 1.0},
  {L"吨",   1000.0},
  {L"盎司", 0.028349523125},
  {L"磅",   0.45359237},
  {L"斤",   0.5},
  {L"两",   0.05},
  {L"克拉", 0.0002},
};

// ---------------- 温度：基准 开尔文（非线性，含偏移；factor 不用） ----------------
constexpr UnitF kTemperatureUnits[] = {
  {L"摄氏度 (°C)", 0.0},
  {L"华氏度 (°F)", 0.0},
  {L"开尔文 (K)",  0.0},
  {L"兰氏度 (°R)", 0.0},
};

// ---------------- 面积：基准 平方米 ----------------
constexpr UnitF kAreaUnits[] = {
  {L"平方毫米", 1e-6},
  {L"平方厘米", 1e-4},
  {L"平方米",   1.0},
  {L"平方千米", 1e6},
  {L"公顷",     1e4},
  {L"亩",       666.6666666667},
  {L"平方英尺", 0.09290304},
  {L"平方码",   0.83612736},
  {L"英亩",     4046.8564224},
  {L"平方英里", 2589988.110336},
};

// ---------------- 速度：基准 米/秒 ----------------
constexpr UnitF kSpeedUnits[] = {
  {L"米/秒",   1.0},
  {L"千米/时", 1.0 / 3.6},
  {L"英里/时", 0.44704},
  {L"节",      0.5144444444444},
  {L"英尺/秒", 0.3048},
  {L"马赫",    340.29},
};

// ---------------- 时间：基准 秒 ----------------
constexpr UnitF kTimeUnits[] = {
  {L"毫秒", 0.001},
  {L"秒",   1.0},
  {L"分钟", 60.0},
  {L"小时", 3600.0},
  {L"天",   86400.0},
  {L"周",   604800.0},
  {L"月",   2629746.0},
  {L"年",   31556952.0},
};

// ---------------- 能量：基准 焦耳 ----------------
constexpr UnitF kEnergyUnits[] = {
  {L"焦耳",     1.0},
  {L"千焦",     1000.0},
  {L"卡路里",   4.184},
  {L"千卡",     4184.0},
  {L"瓦时",     3600.0},
  {L"千瓦时",   3.6e6},
  {L"电子伏特", 1.602176634e-19},
  {L"英热单位", 1055.05585262},
};

// ---------------- 压力：基准 帕斯卡 ----------------
constexpr UnitF kPressureUnits[] = {
  {L"帕斯卡",         1.0},
  {L"千帕",           1000.0},
  {L"兆帕",           1e6},
  {L"巴",             1e5},
  {L"标准大气压",     101325.0},
  {L"毫米汞柱",       133.322387415},
  {L"磅力/平方英寸",  6894.757293168},
  {L"托",             133.322368421},
};

// ---------------- 功率：基准 瓦特 ----------------
constexpr UnitF kPowerUnits[] = {
  {L"瓦特",           1.0},
  {L"千瓦",           1000.0},
  {L"兆瓦",           1e6},
  {L"马力",           735.49875},
  {L"英热单位/小时",  0.2930710702},
};

// ---------------- 角度：基准 弧度 ----------------
constexpr UnitF kAngleUnits[] = {
  {L"度",    kPi / 180.0},
  {L"弧度",  1.0},
  {L"分",    kPi / 10800.0},
  {L"秒",    kPi / 648000.0},
  {L"梯度",  kPi / 200.0},
  {L"圈",    2.0 * kPi},
};

// ---------------- 数据存储：基准 字节（1024 进制） ----------------
constexpr UnitF kDataUnits[] = {
  {L"比特", 0.125},
  {L"字节", 1.0},
  {L"KB",   1024.0},
  {L"MB",   1024.0 * 1024.0},
  {L"GB",   1024.0 * 1024.0 * 1024.0},
  {L"TB",   1024.0 * 1024.0 * 1024.0 * 1024.0},
  {L"PB",   1024.0 * 1024.0 * 1024.0 * 1024.0 * 1024.0},
};

// ---------------- 频率：基准 赫兹 ----------------
constexpr UnitF kFrequencyUnits[] = {
  {L"赫兹", 1.0},
  {L"千赫", 1e3},
  {L"兆赫", 1e6},
  {L"吉赫", 1e9},
  {L"太赫", 1e12},
  {L"转/分", 1.0 / 60.0},
};

constexpr size_t kCurrencyCount    = sizeof(kCurrencyUnits) / sizeof(kCurrencyUnits[0]);
constexpr size_t kVolumeCount      = sizeof(kVolumeUnits) / sizeof(kVolumeUnits[0]);
constexpr size_t kLengthCount      = sizeof(kLengthUnits) / sizeof(kLengthUnits[0]);
constexpr size_t kWeightCount      = sizeof(kWeightUnits) / sizeof(kWeightUnits[0]);
constexpr size_t kTemperatureCount = sizeof(kTemperatureUnits) / sizeof(kTemperatureUnits[0]);
constexpr size_t kAreaCount        = sizeof(kAreaUnits) / sizeof(kAreaUnits[0]);
constexpr size_t kSpeedCount       = sizeof(kSpeedUnits) / sizeof(kSpeedUnits[0]);
constexpr size_t kTimeCount        = sizeof(kTimeUnits) / sizeof(kTimeUnits[0]);
constexpr size_t kEnergyCount      = sizeof(kEnergyUnits) / sizeof(kEnergyUnits[0]);
constexpr size_t kPressureCount    = sizeof(kPressureUnits) / sizeof(kPressureUnits[0]);
constexpr size_t kPowerCount       = sizeof(kPowerUnits) / sizeof(kPowerUnits[0]);
constexpr size_t kAngleCount       = sizeof(kAngleUnits) / sizeof(kAngleUnits[0]);
constexpr size_t kDataCount        = sizeof(kDataUnits) / sizeof(kDataUnits[0]);
constexpr size_t kFrequencyCount   = sizeof(kFrequencyUnits) / sizeof(kFrequencyUnits[0]);

// 内部类别描述：id / 中文名 / 单位表（顺序固定）
struct CategoryDef {
  const wchar_t* id;
  const wchar_t* name;
  const UnitF*   units;
  size_t         count;
};

constexpr CategoryDef kCategories[] = {
  {L"currency",    L"货币",     kCurrencyUnits,    kCurrencyCount},
  {L"volume",      L"体积",     kVolumeUnits,      kVolumeCount},
  {L"length",      L"长度",     kLengthUnits,      kLengthCount},
  {L"weight",      L"重量",     kWeightUnits,      kWeightCount},
  {L"temperature", L"温度",     kTemperatureUnits, kTemperatureCount},
  {L"area",        L"面积",     kAreaUnits,        kAreaCount},
  {L"speed",       L"速度",     kSpeedUnits,       kSpeedCount},
  {L"time",        L"时间",     kTimeUnits,        kTimeCount},
  {L"energy",      L"能量",     kEnergyUnits,      kEnergyCount},
  {L"pressure",    L"压力",     kPressureUnits,    kPressureCount},
  {L"power",       L"功率",     kPowerUnits,       kPowerCount},
  {L"angle",       L"角度",     kAngleUnits,       kAngleCount},
  {L"data",        L"数据存储", kDataUnits,        kDataCount},
  {L"frequency",   L"频率",     kFrequencyUnits,   kFrequencyCount},
};

// 温度 → 开尔文（调用前已保证 unit 属于温度类别）
double toKelvin(const std::wstring& unit, double v) {
  if (unit == L"摄氏度 (°C)") return v + 273.15;
  if (unit == L"华氏度 (°F)") return (v + 459.67) * 5.0 / 9.0;
  if (unit == L"开尔文 (K)")  return v;
  return v * 5.0 / 9.0;  // 兰氏度 (°R)
}

// 开尔文 → 温度单位（反向变换）
double fromKelvin(const std::wstring& unit, double k) {
  if (unit == L"摄氏度 (°C)") return k - 273.15;
  if (unit == L"华氏度 (°F)") return k * 9.0 / 5.0 - 459.67;
  if (unit == L"开尔文 (K)")  return k;
  return k * 9.0 / 5.0;  // 兰氏度 (°R)
}

}  // namespace（匿名）

const std::vector<Category>& Categories() {
  static const std::vector<Category> cats = []() {
    std::vector<Category> v;
    v.reserve(sizeof(kCategories) / sizeof(kCategories[0]));
    for (const CategoryDef& def : kCategories) {
      Category c;
      c.id   = def.id;
      c.name = def.name;
      c.units.reserve(def.count);
      for (size_t i = 0; i < def.count; ++i) {
        c.units.push_back(def.units[i].name);
      }
      v.push_back(c);
    }
    return v;
  }();
  return cats;
}

const Category* Find(const std::wstring& id) {
  const std::vector<Category>& cats = Categories();
  for (const Category& c : cats) {
    if (c.id == id) return &c;
  }
  return nullptr;
}

bool Convert(const std::wstring& catId, const std::wstring& from,
             const std::wstring& to, double v, double& out) {
  // 1) 查找类别
  const CategoryDef* def = nullptr;
  for (const CategoryDef& d : kCategories) {
    if (catId == d.id) {
      def = &d;
      break;
    }
  }
  if (!def) return false;

  // 2) 查找源 / 目标单位
  const UnitF* uFrom = nullptr;
  const UnitF* uTo   = nullptr;
  for (size_t i = 0; i < def->count; ++i) {
    if (from == def->units[i].name) uFrom = &def->units[i];
    if (to   == def->units[i].name) uTo   = &def->units[i];
  }
  if (!uFrom || !uTo) return false;

  // 3) NaN 直接透传
  if (std::isnan(v)) {
    out = std::numeric_limits<double>::quiet_NaN();
    return true;
  }

  // 4) 温度走开尔文（含偏移），其余线性换算
  if (catId == L"temperature") {
    out = fromKelvin(to, toKelvin(from, v));
  } else {
    out = v * uFrom->factor / uTo->factor;
  }
  return true;
}

}  // namespace UnitConv
