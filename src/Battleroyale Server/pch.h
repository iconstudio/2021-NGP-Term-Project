#pragma once
#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma warning(disable: 4244)

/// Windows ��� ����:
#define WIN32_LEAN_AND_MEAN // ���� ������ �ʴ� ������ Windows ������� �����մϴ�.
#include <windows.h>
#include <winperf.h>

/// C ��Ÿ�� ��� �����Դϴ�.
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <time.h>

/// ATL / MFC ��� ����:
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS // �Ϻ� CString �����ڴ� ��������� ����˴ϴ�.
#include <atlbase.h>
#include <atlimage.h>
#include <atlstr.h>

// ���� ��� ����
#define _USE_MATH_DEFINES
#include <math.h>

// ǥ�� ���̺귯��
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

enum WindowInfo : int
{
	GAME_SCENE_W = 1280,
	GAME_SCENE_H = 720,
	CLIENT_W = 960,
	CLIENT_H = 540,
	VIEW_W = 320,
	VIEW_H = 240,
	PORT_W = 640,
	PORT_H = 480
};

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
