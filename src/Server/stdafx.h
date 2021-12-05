#pragma once
#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma warning(disable: 4244)

// Windows 헤더 파일:
#define WIN32_LEAN_AND_MEAN  거의 사용되지 않는 내용은 Windows 헤더에서 제외합니다.
#include <windows.h>
#include <winperf.h>
#include <WinSock2.h>

// C 런타임 헤더 파일입니다.
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <time.h>

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

// cout으로 출력하기
template<typename Ty>
void AtomicPrint(Ty caption) {
	cout << caption;
}

// 여러 개의 값을 함수 하나로 cout으로 출력하기
template<typename Ty1, typename... Ty2>
void AtomicPrint(Ty1 caption, Ty2... args) {
	AtomicPrint(caption);
	AtomicPrint(args...);
}

// cout으로 출력하고 한줄 띄우기
template<typename... Ty>
void AtomicPrintLn(Ty... args) {
	AtomicPrint(args..., "\n");
}
