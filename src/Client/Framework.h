#pragma once
#include "pch.h"
#include "Sprite.h"
#include "CommonDatas.h"

#define SERVER_IP "192.168.120.47"
//#define SERVER_IP "127.0.0.1"

DWORD WINAPI CommunicateProcess(LPVOID arg);			//스레드 함수

enum CLIENT_STATES : int {
	TITLE = 0		// 타이틀 화면
	, LOBBY			// 로비
	, GAME			// 게임 
	, SPECTATOR		// 게임 관전
	, GAME_OVER		// 게임 완료
	, GAME_RESTART	// 게임 다시 참가
	, EXIT			// 클라이언트 종료

};

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

	char buffer[80];	//플레이어 수 표기용 배열
	wchar_t strforplayernum[80];

private:
	CLIENT_STATES status;

	SOCKET my_socket;
	SOCKADDR_IN	server_address;
	HDC surface_double;

	PAINTSTRUCT painter;
	vector<GameSprite*> sprites;

	int terrain_seed = 0;
	HDC map_surface;				// 맵 HDC
	HBITMAP map_bitmap;				// 맵 HBITMAP
	vector<int> mapdata{ 6400 };	// 타일 vectoraa

	char key_checkers[SEND_INPUT_COUNT];	 // 입력중인 키

	int player_num;
	GameUpdateMessage playerinfo;
	int cooldown = 0;
	int bulletleft = 3;
	int bulletcooldown = 0;
	double hp = 100;

	struct { int x, y, w, h, xoff, yoff; } view, port;
	bool view_track_enabled;
	int view_target_player;

};


