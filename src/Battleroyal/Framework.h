#pragma once
#include "pch.h"
#include "Sprite.h"
#include "CommonDatas.h"

//#define SERVER_IP "192.168.120.35"
#define SERVER_IP "127.0.0.1"

enum CLIENT_STATES : int {
	TITLE = 0		// 타이틀 화면
	, LOBBY			// 로비
	, GAME			// 게임
	, SPECTATOR		// 게임 관전
	, GAME_OVER		// 게임 완료
	, GAME_RESTART	// 게임 다시 참가
	, EXIT			// 클라이언트 종료

};

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

	void SetSprite(GameSprite* sprite);

	PACKETS RecvPacket(SOCKET sock);
	int RecvTitleMessage(SOCKET sock);
	int RecvLobbyMessage(SOCKET sock);
	int SendGameMessage(SOCKET sock, PACKETS type, InputStream keys[]);
	int RecvGameMessage(SOCKET sock);

	CLIENT_STATES status;

	COLORREF background_color = COLOR_WHITE;
	const int WORLD_W, WORLD_H;

private:
	SOCKET my_socket;
	SOCKADDR_IN	server_address;

	int	player_index = 0;
	int player_num = 1;
	bool player_captain = false;
	int title_duration = 0;

	InputStream keys[6];

	bool buttonsets[SEND_INPUT_COUNT];		//0 = w, 1 = s, 2 = a, 3 = d

	// 마지막에 수신한 렌더링 정보
	RenderInstance* last_render_info;

	class CInputChecker {
	public:
		int state = 0;			//안누르면 0 

		void on_none() { state = NONE; }
		void on_press() { state = PRESS; }
		void on_release() { state = RELEASE; }
		bool is_pressing() const { return (0 <= time); }
		bool is_pressed() const { return (0 == time); }
	};

	map<WPARAM, CInputChecker> key_checkers;			//키 누른것 확인

	struct { int x, y, w, h, xoff, yoff; } view, port;
	bool view_track_enabled;
	int view_target_player;

	PAINTSTRUCT painter;
	vector<GameSprite*> sprites;

	void DrawRenderInstances();
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

//template<class Predicate>
//inline void ClientFramework::for_each_instances(Predicate predicate) {
//	if (!instances.empty()) {
//		std::for_each(instances.begin(), instances.end(), predicate);
//	}
//}
