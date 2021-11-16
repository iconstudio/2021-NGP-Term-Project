#pragma once
#include "pch.h"
#include "Sprite.h"
#include "CommonDatas.h"

#define SERVER_IP "127.0.0.1"
#define SERVER_PT 9000

enum CLIENT_STATES : int {
	TITLE = 0		// Ÿ��Ʋ ȭ��
	, LOBBY			// �κ�
	, GAME			// ����
	, SPECTATOR		// ���� ����
	, GAME_OVER		// ���� �Ϸ�
	, GAME_RESTART	// ���� �ٽ� ����
	, EXIT			// Ŭ���̾�Ʈ ����

};

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

	int RecvLobbyMessage(SOCKET sock);
	int SendGameMessage(SOCKET sock, PACKETS type, char data[]);
	int RecvGameMessage(SOCKET sock);

	CLIENT_STATES status;

	COLORREF background_color = COLOR_WHITE;
	const int WORLD_W, WORLD_H;

private:
	SOCKET my_socket;
	SOCKADDR_IN	server_address;
	int	player_index = 0;
	bool buttonsets[6];						//0 = w, 1 = s, 2 = a, 3 = d
	bool player_captain;

	// �������� ������ ������ ����
	RenderInstance* last_render_info;

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
	vector<GameSprite*> sprites;
};

class WindowsClient {
public:
	WindowsClient(LONG width, LONG height);
	~WindowsClient();

	BOOL initialize(HINSTANCE handle, WNDPROC procedure, LPCWSTR title, LPCWSTR id, INT cmd_show);

	HINSTANCE instance;						// ���μ��� �ν��Ͻ�
	HWND hwindow;							// â �ν��Ͻ�
	WindowProcedure procedure;				// â ó����
	WNDCLASSEX properties;					// â �������
	LPCWSTR title_caption, class_id;		// â �ĺ���
	LONG width, height;						// â ũ��
};
