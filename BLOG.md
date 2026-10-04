# 用 5000 行纯 Win32 + GDI+ 代码，从零复刻一个 Windows 11 风格计算器

> 没有 Qt，没有 Electron，甚至没有 MFC —— 只有 `CreateWindowEx`、`GDI+` 和一颗把细节做到位的心。
> 本文完整记录一款仿 Windows 11 计算器的设计与实现：五种计算模式、14 类单位转换器、计算历史、深浅主题，外加我在开发中踩过的五个真实的坑。

---

## 1. 为什么要写这个项目

Windows 自带的计算器一直是我心中的"小而美"典范：圆角按键、实时求值、模式切换干脆利落。某天我突然想：如果不用任何 UI 框架，就用最原始的 Win32 API + GDI+，能把它复刻到什么程度？

这个项目的目标很明确：

| 需求 | 说明 |
| --- | --- |
| 五种计算模式 | 标准、科学、图形、程序员、日期计算 |
| 单位转换器 | 货币、体积、长度、重量、温度等 14 类 113 个单位 |
| 计算历史 | 记录每次计算过程，可回看、可载入 |
| 纯 C++ | 模块化源码，Visual Studio 直接编译 |
| 可执行文件 | 编译出独立的 .exe，双击即用 |

最终产物：**253 KB 的 `Calculator.exe`**，零运行时依赖，61 项单元测试全部通过。

## 2. 技术选型：为什么是纯 Win32 + GDI+

| 方案 | 体积 | 启动速度 | 依赖 | 学习价值 |
| --- | --- | --- | --- | --- |
| Qt Widgets | ~40 MB | 慢 | Qt DLL 若干 | 高，但框架代劳太多 |
| Electron | ~150 MB | 很慢 | 整个 Chromium | 低，本质是前端 |
| WinUI 3 | ~10 MB | 中 | Windows App SDK | 中，XAML 心智负担 |
| **纯 Win32 + GDI+** | **253 KB** | **极快** | **系统自带** | **最高，全手工** |

Win32 是 Windows 的"汇编层"，一切都要自己来：窗口过程、消息循环、命中测试、双缓冲、DPI 缩放。但正因如此，你对每个像素都有绝对控制权——这也是"仿"得足够像的前提。GDI+ 提供抗锯齿的矢量绘制（圆角矩形、曲线、文字），恰好够用。

编译环境是 MSVC（本项目用 Visual Studio 2026 的 v145 工具集，VS2022 用户把 vcxproj 里的 `PlatformToolset` 改成 `v143` 即可），语言标准 C++17。

## 3. 整体架构：核心引擎与界面彻底分离

```
src/
├─ core/                        核心引擎层 —— 纯计算，不依赖任何窗口代码
│  ├─ ExpressionEngine           表达式解析（分词→调度场→逆波兰求值）
│  ├─ IntegerEngine              程序员整数引擎（任意进制 + 位运算）
│  ├─ UnitConverter              单位换算表（14 类 113 单位）
│  ├─ DateCalculator             日期算法（差值/加减/星期）
│  ├─ HistoryStore               历史记录持久化
│  └─ NumberFormat               数字格式化（千分位/进制分组）
├─ ui/                          界面层 —— 窗口、控件与绘制
│  ├─ Theme                      浅/深配色、DPI、GDI+ 双缓冲
│  ├─ CtrlButton / Keypad        自绘圆角按钮 / 声明式键盘布局
│  ├─ DisplayPanel / BitGrid     显示区 / 位翻转网格
│  ├─ ModePanel / KeypadPanel    模式面板基类 / 键盘计算状态机
│  └─ MainWindow + 6 个模式面板  主窗口、导航菜单、历史侧栏
├─ main.cpp                      程序入口
└─ Commands.h                    全局命令 ID 表
```

三条设计原则：

1. **引擎零 UI 依赖**：`src/core` 不引用任何窗口代码，用控制台测试程序直接验证（`tests/core_test.cpp` 的 61 项断言全部通过后才接入界面）。
2. **声明式布局**：键盘用 `KeyCell`（文字、命令 ID、配色、跨行跨列）描述，`Keypad` 按窗口尺寸自动铺满——改布局只改数据，不写布局代码。
3. **多态面板**：六种模式统一继承 `ModePanel`，主窗口通过虚接口调度，互不感知。

## 4. 核心实现剖析

### 4.1 表达式引擎：一个"迷你编译器"

计算器的灵魂是把 `sin(30) + 2π × 5!` 变成数字。我实现了一条经典流水线：**分词 → 隐式乘法补全 → 调度场算法 → 逆波兰求值**。

分词器把输入切成 `Num / Op / LP / RP / Func / Const / Fact / Comma` 八类 token：

```cpp
// 数字支持小数点与科学计数法
if (IsDigit(c) || (c == L'.' && i + 1 < n && IsDigit(s[i + 1]))) {
  size_t st = i;
  while (i < n && IsDigit(s[i])) i++;
  if (i < n && s[i] == L'.') { i++; while (i < n && IsDigit(s[i])) i++; }
  if (i < n && (s[i] == L'e' || s[i] == L'E')) { /* 指数部分 */ }
  toks.push_back({TT::Num, s.substr(st, i - st), st, i - st});
}
```

三个最难的细节：

**① 一元负号与幂的优先级**。`-2^2` 必须等于 `-(2^2) = -4`，而 `2^-3` 是 `2^(-3)`。解法是给一元负号一个独立的优先级 3 且右结合——与 `^` 同级，但因其右结合，压栈时不会把 `^` 弹出去：

```cpp
// 一元负号优先级 3 且右结合，使 -2^2 = -(2^2)，2^-3 = 2^(-3)
toks.push_back({TT::Op, unary ? L"u-" : L"-", i, 1, unary ? 3 : 1, unary, unary});
```

**② 隐式乘法**。`2π`、`2(3+1)`、`sin(30)cos(30)` 都是合法输入。我在分词后加一趟扫描：前一个 token 是"运算数结尾"（数字/常量/变量/右括号/阶乘）、当前 token 是"运算数开头"（数字/常量/变量/左括号/函数）时，自动插入一个 `×`：

```cpp
bool prevOperand = p.t == TT::Num || p.t == TT::Const || p.t == TT::Var ||
                   p.t == TT::RP || p.t == TT::Fact;
bool curOperand = t.t == TT::Num || t.t == TT::Const || t.t == TT::Var ||
                  t.t == TT::LP || t.t == TT::Func;
if (prevOperand && curOperand)
  out.push_back({TT::Op, L"*", t.pos, 0, 2, false, false});
```

**③ 多参数函数的参数边界**。科学模式有 `xroot(3,8)`、`logy(2,8)`、`mod(a,b)` 这类双参数函数。逆波兰求值遇到函数时，怎么知道该弹几个操作数？我的方案是**帧标记**：函数出现时先向输出流压一个帧标记，右括号闭合时立即把函数弹出到输出流；求值时函数弹出操作数直到碰到帧标记为止：

```cpp
case TT::Func:
  out.push_back({TT::Mark, L"FRAME", t.pos, 0});  // 参数区域帧标记
  st.push_back(t); break;
case TT::RP: {
  /* ...弹栈直到左括号... */
  if (!st.empty() && st.back().t == TT::Func) {   // 函数参数到此结束
    out.push_back(st.back());
    st.pop_back();
  }
  break;
}
```

这样 `xroot(3,8)` 的 RPN 是 `FRAME 3 8 xroot`，求值时 `xroot` 弹 8、弹 3、弹帧，得到两个参数——逗号不再需要任何特殊处理，完美支持嵌套的 `f(g(a,b),c)`。

**百分号语义**也照搬了 Windows 计算器：`50+10%` 中 `10%` 相对左侧结果 `50` 计算，得 `55`；`200×10%` 则 `10% = 0.1`，得 `20`。实现是"找到最后一个数字、向左找最近的顶层加减号、替换数字段并吃掉 % 号"：

```cpp
if (opIdx >= 0 && (toks[opIdx].s == L"+" || toks[opIdx].s == L"-")) {
  std::wstring left = expr.substr(0, toks[opIdx].pos);
  double base = 0; std::wstring err;
  if (!EvalAll(left, 0.0, ang, base, err)) base = 0;
  newV = base * v / 100.0;          // A ± B%  =>  A ± A×B/100
} else {
  newV = v / 100.0;                 // 其余情况 => B/100
}
```

### 4.2 程序员模式：把 uint64_t 玩出花

程序员模式用同一套分词/调度场骨架，但求值器是纯整数：字面量按当前进制解析（DEC 下 `A` 就是非法 token），所有运算按字长掩码截断，天然支持二进制补码回绕：

```cpp
const uint64_t mask = Mask(bits);
// ...
else if (t.s == L"-") r = (a - b) & mask;
else if (t.s == L"<<") r = (b >= bits ? 0 : (a << b)) & mask;
else if (t.s == L"~&") r = (~(a & b)) & mask;      // NAND
```

`0 - 1` 在 BYTE 字长下得到 `FF`——补码语义自动成立。显示时按位宽符号扩展：`FormatSigned` 用算术右移技巧把 `0xFFFFFFFF`（DWORD）还原为 `-1`。64 个可点击的位翻转小方块（BitGrid）让"眼见为实"的二进制操作变得直观。

### 4.3 图形模式：600 个采样点连成曲线

方程按弧度制逐点求值，采样 600 个点，遇到 `NaN/Inf`（如 `sqrt(x)` 在负数域）就断线，保证曲线正确：

```cpp
double x = xmin_ + (xmax_ - xmin_) * j / (kSamples - 1);
if (Expr::EvaluateX(text, x, y, err, Expr::AngleMode::Rad)) ys_[i][j] = y;
else ys_[i][j] = std::numeric_limits<double>::quiet_NaN();
```

缩放以光标为锚点，平移按像素比例换算到坐标范围，双击复位并重新自动适配 y 轴——这些都是 30 行内就能讲清的小数学。

### 4.4 日期算法：从罗马教皇历法借来的公式

日期差用 Howard Hinnant 的民用历算法（`days_from_civil` / `civil_from_days`），把年月日映射为连续天数，减法一减了事；`AddMonths` 则做了月末裁剪——`2024-01-31` 加一个月是 `2024-02-29`（闰年）而不是 `2024-03-02`。星期几以 `1970-01-01`（周四）为锚点计算，不依赖天数的纪元偏移。

### 4.5 UI 框架：声明式键盘与双缓冲

六种模式共用一个自绘按钮控件（圆角矩形、悬停/按下/选中三态、MDL2 图标字体），键盘布局是纯数据声明：

```cpp
std::vector<KeyRow> rows = {
  {{{L"%", IDK_PERCENT, K::Function}, {L"CE", IDK_CE, K::Function},
    {L"C", IDK_CLEAR, K::Function}, {L"\uE94F", IDK_BACK, K::Function}}},
  {{{L"1/x", IDK_RECIP, K::Function}, {L"x²", IDK_SQ, K::Function},
    {L"√x", IDK_SQRT, K::Function}, {L"÷", IDK_DIV, K::Function}}},
  // ... 4 行省略
};
pad.Build(hwnd_, rows, 4, Theme::S(4), Theme::S(4));
```

所有绘制走 `Theme::PaintBuffer` 内存位图双缓冲，一次 `BitBlt` 上屏；浅/深主题只是两套调色板切换 + 全局重绘；所有尺寸经 `Theme::S()` 做 DPI 缩放。

## 5. 实战踩坑：这六个错误我保证你会再遇到

**坑一：`rpcndr.h` 的 `#define small char`。**
某天编译器突然报"bool 后面的 char 非法"，指向一行完全无辜的 `bool small = false`。真相：`windows.h` 链式包含的 `rpcndr.h` 里有一条 1980 年代遗留的 `#define small char`，把我所有叫 `small` 的参数/成员全部宏替换成了 `char`。解法是在公共头里 `#undef small`。

**坑二：`WIN32_LEAN_AND_MEAN` 与 GDI+ 打架。**
为了加快编译我定义了 `WIN32_LEAN_AND_MEAN`，结果 `gdiplus.h` 依赖的 `IUnknown`/`STDMETHOD` 没人声明了，爆出几十个莫名其妙的 GDI+ 头错误。解法：先 `#include <objidl.h>` 再 `#include <gdiplus.h>`。

**坑三：聚合初始化的花括号玄学。**
`std::vector<KeyRow>` 的每行要写成三层括号 `{{{cell}, {cell}}}`——因为 `KeyRow` 的成员是个 `vector<KeyCell>`，两层括号会被解释成"把多个元素喂给 KeyRow 的唯一成员"。这类错误 MSVC 的报错信息长得能绕地球一圈，但真相只有一层括号之差。

**坑四：VS2026 的工具集叫 v145，MSBuild 目录却叫 v180。**
`MSBuild\Microsoft\VC\` 下有 `v150/v160/v170/v180` 目录（对应 MSBuild 自身版本），但 `PlatformToolset` 必须写 `v145`（对应 MSVC 14.51）。写 `v180` 会得到"找不到 v180 的生成工具"。

**坑五：先写测试救了我一命。**
日期模块交付后，61 项测试里只有"1970-01-01 是周四"失败。排查发现 `days_from_civil` 少减了 719468 的纪元偏移——差值类计算（AddDays/Diff）因偏移互相抵消而全部正常，只有星期计算露馅。如果没有这条断言，这个 bug 会潜伏到用户某天发现"今天怎么是周五"。

**坑六：一个"清空输入框"引发的栈溢出崩溃。**
交付之后用户反馈：一切到单位转换器就卡住，然后闪退。复现时先用 `SendMessageTimeout` 探活确认 UI 线程假死，再看进程退出码是 `0xC00000FD`——栈溢出，典型的无限递归。根因藏得很深：转换器的 `Convert()` 在"输入为空 / 格式无效"分支里会用 `SetWindowTextW` 清空对面输入框，而程序化修改编辑框文本会**同步**回发 `EN_CHANGE` 通知 → 反向调用 `Convert()` → 又去清空另一侧 → 再触发 `EN_CHANGE`……两个编辑框互相清空、无限递归，直到栈爆。更阴险的是，切换模式时面板会重置两个输入框，恰好踩中这条循环链，所以"一进转换器必崩"，而其他模式安然无恙。修复只需一条纪律：**凡是会同步回发消息的控件写操作，一律套重入锁**——给 `Convert()` 加 `updating_` 标志，所有程序化 `SetWindowTextW` 在锁内执行，通知处理器进门先查锁。

排查过程还有个插曲：我用外部脚本跨进程读编辑框文本做验证，`GetWindowText` 对无标题的编辑框永远返回空串，一度让我以为换算根本没写入，追了半天"幽灵清空"；最后靠程序内日志打印读回值才洗清冤屈。**跨进程读控件文本不可信**——这条也一并记下。

## 6. 效果展示

![标准模式](docs/screenshot-standard.png)
![科学模式](docs/screenshot-scientific.png)
![图形模式](docs/screenshot-graphing.png)
![程序员模式](docs/screenshot-programmer.png)
![日期计算](docs/screenshot-date.png)
![单位转换器](docs/screenshot-converter.png)

## 7. 使用与构建

- **运行**：双击 `Calculator.exe` 即可（或 `bin\x64\Release\Calculator.exe`）。
- **编译**：Visual Studio 打开 `Calculator.sln`，选 Release|x64 生成；或双击 `build.bat`。VS2022 用户先把 vcxproj 中 4 处 `v145` 改为 `v143`。
- **单元测试**：运行 `tests\build_tests.bat`，61 项核心测试全部通过。
- **快捷键**：数字/四则运算直接输入，`Enter` 等号、`Esc` 清除、`Backspace` 退格、`Del` 清当前项、`F9` 取反；科学模式 `p`→π、`e`→e、`^`→幂、`!`→阶乘；程序员模式 `a`~`f` 输入十六进制。
- **历史记录**：按 `=` 自动记录，右侧历史图标打开侧栏，双击条目载入结果，保存在 `%APPDATA%\W11Calc\history.txt`（最多 500 条）。

## 8. 不足与展望

- 货币汇率为内置参考汇率（非实时），后续可接在线汇率接口；
- 深色主题下原生下拉框的弹出列表仍是浅色系统样式；
- 图形模式没有函数自动补全与括号配对提示；
- 数值精度为 double，超大数/高精度场景可换任意精度库。

## 9. 结语

这个项目最让我满足的瞬间，不是界面能跑，而是看着 253 KB 的 exe 在资源管理器里安静地躺着——它不依赖任何运行时，双击即开。所有按钮、每一条曲线、每一个千分位逗号，都是亲手"画"出来的。如果你也想理解 Windows GUI 的底层世界，我强烈建议亲手写一个：你会重新认识 `WM_PAINT`、认识消息循环，也会被 `#define small char` 这样的上古化石逗笑。

完整的构建说明与使用文档见 `README.md`，项目源码见代码仓库。
