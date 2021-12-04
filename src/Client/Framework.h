﻿#pragma once
#include "pch.h"
#include "Sprite.h"
#include "CommonDatas.h"

#define SERVER_IP "192.168.137.151"
//#define SERVER_IP "127.0.0.1"

enum CLIENT_STATES : int {
	TITLE = 0		// 타이틀 화면
	, LOBBY			// 로비
	, GAME			// 게임
	, SPECTATOR		// 게임 관전
	, GAME_OVER		// 게임 완료
	, GAME_RESTART	// 게임 다시 참가
	, EXIT			// 클라이언트 종료

};

class ClientFramework {
public:
	COLORREF background_color = COLOR_WHITE;
	const int WORLD_W, WORLD_H;

	ClientFramework(int rw, int rh, int vw, int vh, int pw, int ph);
	~ClientFramework();

	void OnMouseDown(WPARAM button, LPARAM cursor);
	void OnMouseUp(WPARAM button, LPARAM cursor);

	void Initialize();
	void Update();
	void Render(HWND window);

	PACKETS RecvPacket(SOCKET sock);
private:
	SOCKET my_socket;
	SOCKADDR_IN	server_address;
	HDC surface_double;

	struct { int x, y, w, h, xoff, yoff; } view, port;
	bool view_track_enabled;
	int view_target_player;

	CLIENT_STATES status;

	PAINTSTRUCT painter;
	vector<GameSprite*> sprites;

	// 마지막에 수신한 렌더링 정보
	RenderInstance* last_render_info;
	bool key_checkers[7];			//입력중인 키
	int mouse_x;
	int mouse_y;
};

typedef LRESULT(CALLBACK* WindowProcedure)(HWND, UINT, WPARAM, LPARAM);

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
