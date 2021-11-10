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
	void Render(HWND window);

	void InputRegister(WPARAM virtual_button);
	bool InputCheck(WPARAM virtual_button);
	bool InputCheckPressed(WPARAM virtual_button);

	void OnMouseDown(WPARAM button, LPARAM cursor);
	void OnMouseUp(WPARAM button, LPARAM cursor);

	void ViewTrackEnable(bool flag);
	void ViewSetTarget(int target_player);
	void ViewSetPosition(int vx, int vy);

	COLORREF background_color = COLOR_WHITE;
	const int WORLD_W, WORLD_H;

private:
	class CInputChecker {
	public:
		int time = -1;

		void on_press() { time++; }
		void on_release() { time = -1; }
		bool is_pressing() const { return (0 <= time); }
		bool is_pressed() const { return (0 == time); }
	};

	map<WPARAM, CInputChecker> key_checkers;

	struct { int x, y, w, h, xoff, yoff; } view, port;
	bool view_track_enabled;
	int view_target_player;

	PAINTSTRUCT painter;
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
