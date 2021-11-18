#include "pch.h"
#include "Framework.h"
#include "resource.h"

void ErrorAbort(const char* msg) {
	LPVOID lpMsgBuf;

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPTSTR>(&lpMsgBuf), 0, nullptr);

	MessageBox(nullptr, static_cast<LPCTSTR>(lpMsgBuf), msg, MB_ICONERROR);

	LocalFree(lpMsgBuf);
	exit(true);
}

void ErrorDisplay(const char* msg) {
	LPVOID lpMsgBuf;

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPTSTR>(&lpMsgBuf), 0, nullptr);

	std::cout << "[" << msg << "] " << static_cast<char*>(lpMsgBuf) << std::endl;

	LocalFree(lpMsgBuf);
}

ClientFramework::ClientFramework(int rw, int rh, int vw, int vh, int pw, int ph)
	: painter{}
	, WORLD_W(rw), WORLD_H(rh)
	, view{ 0, 0, vw, vh }, port{ 0, 0, pw, ph }
	, view_track_enabled(false), view_target_player(-1) {
	view.xoff = vw * 0.5;
	view.yoff = vh * 0.5;
	port.x = (CLIENT_W - pw) * 0.5;
}

ClientFramework::~ClientFramework() {
	closesocket(my_socket);
}

void ClientFramework::Initialize() {
	WSADATA wsadata;

	if (0 != WSAStartup(MAKEWORD(2, 2), &wsadata)) {
		// 오류
		return;
	}

	my_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == my_socket) {
		// 오류
		return;
	}

	auto address_size = sizeof(server_address);
	ZeroMemory(&server_address, address_size);
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = inet_addr(SERVER_IP);
	server_address.sin_port = htons(COMMON_PORT);


	InputRegister('W');
	InputRegister('S');
	InputRegister('A');
	InputRegister('D');
	InputRegister(VK_SPACE);
	InputRegister(VK_ESCAPE);
}

void ClientFramework::Update() {
	int retval;

	for (auto& key_pair : key_checkers) {
		short check = GetAsyncKeyState(key_pair.first);

		auto state = key_pair.second;

		if (HIBYTE(check) == 0) { // released
			state.on_release();
		}
		else if (check & 0x8000) {
			state.on_press();
		}
	}



	switch (status) {
	case TITLE:
	{
		background_color = COLOR_YELLOW;
		
		if (title_duration < 20)
		{
			title_duration++;
			break;
		}

		auto address_size = sizeof(server_address);
		int result = connect(my_socket, reinterpret_cast<sockaddr*>(&server_address), address_size);
		if (SOCKET_ERROR == result) {
			// 오류
			return;
		}
		status = LOBBY;
		RecvTitleMessage(my_socket);

	}
	break;

	case LOBBY:
	{
		background_color = COLOR_RED;

		if (player_captain == true)
		{
			PACKETS packet = CLIENT_GAME_START;
			int result = send(my_socket, reinterpret_cast<char*>(&packet), sizeof(packet), 0);;
			if (result == SOCKET_ERROR)
			{
				ErrorDisplay("send()");
			}
			status = GAME;
		}
	}
	break;

	case GAME:
	{
		int itercount = 0;
		PACKETS gamemessage = CLIENT_KEY_INPUT;

		for (auto it = key_checkers.begin(); it != key_checkers.end(); it++) {		// key_checkers에서 값을 읽어 배열 제작
			buttonsets[itercount] = (it->second.time == -1);
			itercount++;
		}

		SendGameMessage(my_socket, CLIENT_KEY_INPUT, reinterpret_cast<char*>(buttonsets));
		RecvGameMessage(my_socket);

		if (view_track_enabled) {
			if (view_target_player != -1) {
				//ViewSetPosition(view_target->x, view_target->y);
			}
		}
	}
	break;


	case SPECTATOR:
	{
		if (view_track_enabled) {
			if (view_target_player != -1) {
				//ViewSetPosition(view_target->x, view_target->y);
			}
		}
	}
	break;

	default:
		break;
	}
}

void ClientFramework::Render(HWND window) {
	HDC surface_app = BeginPaint(window, &painter);

	HDC surface_double = CreateCompatibleDC(surface_app);
	HBITMAP m_hBit = CreateCompatibleBitmap(surface_app, WORLD_W, WORLD_H);
	HBITMAP m_oldhBit = static_cast<HBITMAP>(SelectObject(surface_double, m_hBit));

	// 초기화
	Render::draw_clear(surface_double, WORLD_W, WORLD_H, background_color);

	HDC surface_back = CreateCompatibleDC(surface_app);
	HBITMAP m_newBit = CreateCompatibleBitmap(surface_app, view.w, view.h);
	HBITMAP m_newoldBit = reinterpret_cast<HBITMAP>(SelectObject(surface_back, m_newBit));

	// 파이프라인

	//for_each_instances([&](GameSprite*& inst) {
	//	if (inst->sprite_index) {
	//		if (!(view.x + view.w <= inst->bbox_left() || inst->bbox_right() < view.x
	//			|| view.y + view.h <= inst->bbox_top() || inst->bbox_bottom() < view.y))
	//			inst->on_render(surface_double);
	//	} else {
	//		inst->on_render(surface_double);
	//	}
	//});

	// 이중 버퍼 -> 백 버퍼
	BitBlt(surface_back, 0, 0, view.w, view.h, surface_double, view.x, view.y, SRCCOPY);
	Render::draw_end(surface_double, m_oldhBit, m_hBit);

	// 백 버퍼 -> 화면 버퍼
	StretchBlt(surface_app, port.x, port.y, port.w, port.h
		, surface_back, 0, 0, view.w, view.h, SRCCOPY);
	Render::draw_end(surface_back, m_newoldBit, m_newBit);

	DeleteDC(surface_back);
	DeleteDC(surface_double);
	ReleaseDC(window, surface_app);
	EndPaint(window, &painter);
}

void ClientFramework::OnMouseDown(const WPARAM button, const LPARAM cursor) {
	auto vk_status = key_checkers[button];
	vk_status.on_press();

	//mouse_x = LOWORD(cursor);
	//mouse_y = HIWORD(cursor);
}

void ClientFramework::OnMouseUp(const WPARAM button, const LPARAM cursor) {
	auto vk_status = key_checkers[button];
	vk_status.on_release();

	//mouse_x = LOWORD(cursor);
	//mouse_y = HIWORD(cursor);
}

void ClientFramework::InputRegister(const WPARAM virtual_button) {
	key_checkers.emplace(virtual_button, CInputChecker());
}

bool ClientFramework::InputCheck(const WPARAM virtual_button) {
	auto checker = key_checkers.find(virtual_button);

	if (checker != key_checkers.end()) {
		return checker->second.is_pressing();
	}

	return false;
}

bool ClientFramework::InputCheckPressed(const WPARAM virtual_button) {
	auto checker = key_checkers.find(virtual_button);
	if (checker != key_checkers.end()) {
		return checker->second.is_pressed();
	}

	return false;
}

void ClientFramework::ViewTrackEnable(bool flag) {
	view_track_enabled = flag;
}

void ClientFramework::ViewSetTarget(int target_player) {
	view_target_player = target_player;
}

void ClientFramework::ViewSetPosition(int vx, int vy) {
	view.x = max(0, min(WORLD_W - view.w, vx - view.xoff));
	view.y = max(0, min(WORLD_H - view.h, vy - view.yoff));
}

int ClientFramework::RecvTitleMessage(SOCKET sock) {
	int temp = 0;
	int retval;

	retval = recv(sock, reinterpret_cast<char*>(temp), sizeof(int), MSG_WAITALL);

	if (0 < temp)
    player_captain = false;					//0이면 방장 아니면 쩌리
	else
    player_captain = true;

	return retval;
}

int ClientFramework::RecvLobbyMessage(SOCKET sock) {
	int retval;

	retval = recv(sock, reinterpret_cast<char*>(player_num), sizeof(int), MSG_WAITALL);

	return retval;
}

int ClientFramework::SendGameMessage(SOCKET sock, PACKETS type, char data[]) {
	int result = send(sock, reinterpret_cast<char*>(&type), sizeof(PACKETS), 0);
	if (result == SOCKET_ERROR)
	{
		ErrorDisplay("send()");
		return result;
	}
	send(sock, data, sizeof(data), 0);

	return result;

}

int ClientFramework::RecvGameMessage(SOCKET sock) {
	int retval;

	retval = recv(sock, reinterpret_cast<char*>(last_render_info), sizeof(RenderInstance), MSG_WAITALL);

	return retval;
}

WindowsClient::WindowsClient(LONG cw, LONG ch)
	: width(cw), height(ch), procedure(NULL) {}

WindowsClient::~WindowsClient() {
	UnregisterClassW(class_id, instance);
}

BOOL WindowsClient::initialize(HINSTANCE handle, WNDPROC procedure, LPCWSTR title, LPCWSTR id, INT cmd_show) {
	properties.cbSize = sizeof(WNDCLASSEX);
	properties.style = CS_HREDRAW | CS_VREDRAW;
	properties.lpfnWndProc = procedure;
	properties.cbClsExtra = 0;
	properties.cbWndExtra = 0;
	properties.hInstance = handle;
	properties.hIcon = LoadIcon(handle, MAKEINTRESOURCE(IDI_BATTLEROYAL));
	properties.hCursor = LoadCursor(nullptr, IDC_ARROW);
	properties.hbrBackground = CreateSolidBrush(0);
	properties.lpszMenuName = NULL;
	properties.lpszClassName = reinterpret_cast<LPCSTR>(id);
	properties.hIconSm = LoadIcon(properties.hInstance, MAKEINTRESOURCE(IDI_SMALL));
	RegisterClassEx(&properties);

	DWORD window_attributes = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
	HWND hWnd = CreateWindow(reinterpret_cast<LPCSTR>(id), reinterpret_cast<LPCSTR>(title), window_attributes
		, CW_USEDEFAULT, 0, width, height
		, nullptr, nullptr, instance, nullptr);
	instance = handle;
	title_caption = title;
	class_id = id;

	if (!hWnd) {
		return FALSE;
	}

	hwindow = hWnd;
	ShowWindow(hWnd, cmd_show);
	UpdateWindow(hWnd);

	return TRUE;
}