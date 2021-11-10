#pragma once
#include "pch.h"


typedef LRESULT(CALLBACK* WindowProcedure)(HWND, UINT, WPARAM, LPARAM);

class GameClientInstance {
public:
	double x, y;
};

class ClientFramework {
public:
	ClientFramework(int rw, int rh, int vw, int vh, int pw, int ph);
	~ClientFramework();

	void Initialize();
	void Update();
	void Render();

	void on_mousedown(WPARAM button, LPARAM cursor);
	void on_mouseup(WPARAM button, LPARAM cursor);
	void on_keydown(WPARAM key);
	void on_keyup(WPARAM key);
	void input_register(WPARAM virtual_button);
	bool input_check(WPARAM virtual_button);
	bool input_check_pressed(WPARAM virtual_button);
};

class WindowsClient {
public:
	WindowsClient(LONG width, LONG height);
	~WindowsClient();

	BOOL initialize(HINSTANCE handle, WNDPROC procedure, LPCWSTR title, LPCWSTR id, INT cmd_show);

	HINSTANCE instance;						// 프로세스 인스턴스
	HWND hwindow;							// 창 인스턴스
	WindowProcedure procedure;				// 창 처리기
	WNDCLASSEX properties;					// 창 등록정보
	LPCWSTR title_caption, class_id;		// 창 식별자
	LONG width, height;						// 창 크기
};
