#pragma once

#pragma once
#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma warning(disable: 4244)

/// Windows 헤더 파일:
#define WIN32_LEAN_AND_MEAN // 거의 사용되지 않는 내용은 Windows 헤더에서 제외합니다.
#include <windows.h>
#include <winperf.h>

/// C 런타임 헤더 파일입니다.
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <time.h>

/// ATL / MFC 헤더 파일:
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS // 일부 CString 생성자는 명시적으로 선언됩니다.
#include <atlbase.h>
#include <atlimage.h>
#include <atlstr.h>

// 수학 상수 선언
#define _USE_MATH_DEFINES
#include <math.h>

// 표준 라이브러리
#include <memory>
#include <vector>
#include <map>
#include <algorithm>
#include <string>
#include <iostream>
#include <fstream>
#include <cmath>
#include <chrono>
#include <random>

using namespace std;

inline double radtodeg(double value) {
	return value / M_PI * 180.0;
}

inline double degtorad(double value) {
	return value * M_PI / 180.0;
}

inline double lengthdir_x(double length, double direction) {
	return cos(degtorad(direction)) * length;
}

inline double lengthdir_y(double length, double direction) {
	return -sin(degtorad(direction)) * length;
}

inline double point_distance(double x1, double y1, double x2, double y2) {
	return sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
}

inline double point_direction(double x1, double y1, double x2, double y2) {
	return radtodeg(atan2(y1 - y2, x2 - x1));
}

namespace Render {
void transform_set(HDC, XFORM&);
void transform_set_identity(HDC);
void transform_set_rotation(HDC);

void draw_clear(HDC, int width, int height, COLORREF color = 0);
BOOL draw_rectangle(HDC, int, int, int, int);
void draw_end(HDC, HGDIOBJ, HGDIOBJ);

static XFORM transform_identity{ 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f };
}
