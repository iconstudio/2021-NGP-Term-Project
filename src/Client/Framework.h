#pragma once
#include "pch.h"
#include "Sprite.h"
#include "CommonDatas.h"

#define DEFAULT_SERVER_IP "127.0.0.1"

DWORD WINAPI CommunicateProcess(LPVOID arg);			//스레드 함수


struct SockInfo {
	SOCKET client_socket;

	SockInfo(SOCKET sk, HANDLE hd);
	~SockInfo();
};

class ClientFramework {
public:
	ClientFramework();
	~ClientFramework();

	void Initialize();
	void Update();
	void Render(HWND window);

	PACKETS RecvPacket(SOCKET sock);
	friend DWORD WINAPI CommunicateProcess(LPVOID arg);	// 서버와의 통신용 스레드 함수

	void OnMouseDown(WPARAM button, LPARAM cursor);
	void OnMouseUp(WPARAM button, LPARAM cursor);

	// 마지막에 수신한 렌더링 정보
	RenderInstance last_render_info[RENDER_INST_COUNT];

	COLORREF background_color = COLOR_WHITE;
	
	int mouse_x;
	int mouse_y;

	wchar_t players_count_buffer[80];	//플레이어 수 표기용 배열

	bool connect_status = false;		//접속 상황 확인

	bool QTE = false;				//QTE관련 변수
	double get_buffed = false;
	double QTEtime = 0;

	double flash_cooltime = 0;		//점멸 쿨타임	

	bool dead = false;
	double ghost = 1.0;

	bool SendGamestart = false;
	bool captain = false;

private:
	SOCKET my_socket;
	SOCKADDR_IN	server_address;
	HDC surface_double;

	PAINTSTRUCT painter;
	vector<GameSprite*> sprites;

	int terrain_seed = 0;
	HDC map_surface;				// 맵 HDC
	HBITMAP map_bitmap;				// 맵 HBITMAP
	vector<int> map_data{ 6400 };	// 타일 vector

	char key_checkers[SEND_INPUT_COUNT];	 // 입력중인 키

	int player_num;
	GameUpdateMessage player_info;
	int cool_down = 0;
	int bullet_left = 3;
	double bullet_cooldown = 0;
	double reload_cooldown = 0;
	bool reloading = false;
	double hp = 100;
	int me = 0;
	bool win = false;

	struct { int x, y, w, h, xoff, yoff; } view, port;
};


