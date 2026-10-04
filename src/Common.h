#pragma once
// 公共头文件：Windows 基础头 + 常用标准库，所有源文件最先包含
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <objidl.h>   // GDI+ 头文件需要 IUnknown/STDMETHOD 等 OLE 基础类型
#include <windowsx.h>
#include <commctrl.h>
#include <dwmapi.h>
// rpcndr.h 遗留宏会把 "small" 展开成 "char"，污染标识符，这里取消定义
#ifdef small
#undef small
#endif
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
