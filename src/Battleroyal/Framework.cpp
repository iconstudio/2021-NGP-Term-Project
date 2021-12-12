#include "pch.h"
#include "Framework.h"
#include "resource.h"

GameSprite player_sprite("../../res/PlayerWalkDown_strip6.png", 6, 0, 0);
GameSprite bullet_sprite("../../res/PlayerWalkRight_strip4.png", 4, 0, 0);
GameSprite player2sprite("../../res/PlayerWalkRight_strip4.png", 4, 0, 0);
GameSprite buttonsprite("../../res/Start_button.png", 1, 0, 0);


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


	keys[0].code = VK_UP;
	keys[1].code = VK_DOWN;
	keys[2].code = VK_LEFT;
	keys[3].code = VK_RIGHT;
	keys[4].code = 'a';
	keys[5].code = 's';
	

	InputRegister(VK_ESCAPE);

	SetSprite(&player_sprite);
	SetSprite(&player2sprite);
	SetSprite(&buttonsprite);
	SetSprite(&bullet_sprite);
}

void ClientFramework::Update() {
	int retval;


	switch (status) {
	case TITLE:
	{
		background_color = COLOR_YELLOW;
		auto address_size = sizeof(server_address);
		if (title_duration < 200)	//로비까지 시간 100 = 1초 
		{
			title_duration++;
			break;
		}
		int result = connect(my_socket, reinterpret_cast<sockaddr*>(&server_address), address_size);
		if (SOCKET_ERROR == result) {
			// 오류
			return;
		}

		status = LOBBY;

		mouse_x = 0;
		mouse_y = 0;
	}
	break;

	case LOBBY:
	{
		CreateThread(NULL, 0, ::CommunicateProcess, (void*)my_socket, 0, NULL);
		/*background_color = COLOR_RED;
		PACKETS packet = RecvPacket(my_socket);
		if (packet == SERVER_SET_CAPATIN)
			player_captain = true;

		recv(my_socket, (char*)(&packet), sizeof(PACKETS), MSG_WAITALL);

		if (player_captain == true &&
			mouse_x > VIEW_W / 2 - sprites[2]->get_width() / 2 &&
			mouse_x < VIEW_W / 2 + sprites[2]->get_width() / 2 &&
			mouse_y > VIEW_H / 2 - sprites[2]->get_height() / 2 &&
			mouse_y < VIEW_H / 2 + sprites[2]->get_height() / 2)
		{
		}

		if (RecvPacket(my_socket) == SERVER_PLAYER_COUNT)
		{
			recv(my_socket, (char*)player_count, sizeof(int), 0);
		}
		if (RecvPacket(my_socket) == SERVER_GAME_START)

		if (player_captain == true &&
			mouse_x > VIEW_W / 2 - sprites[2]->get_width() / 2 &&
			mouse_x < VIEW_W / 2 + sprites[2]->get_width() / 2 &&
			mouse_y > VIEW_H / 3 * 2 - sprites[2]->get_height() / 2 &&
			mouse_y < VIEW_H / 3 * 2 + sprites[2]->get_height() / 2)
		{
			SendData(my_socket, CLIENT_GAME_START, nullptr, 0);
		}
		if (packet == SERVER_PLAYER_COUNT)
		{
			retval = recv(my_socket, (char*)player_count, sizeof(int), 0);
			if (retval == SOCKET_ERROR)
			{
			}
		}
		if (packet == SERVER_GAME_START)
		{
			status = GAME;
		}*/
	}
	break;

	case GAME:
	{
	}
	break;


	case SPECTATOR:
	{
	}
	break;

	default:
		break;
	}

}

void ClientFramework::Render(HWND window) {
	HDC surface_app = BeginPaint(window, &painter);

	surface_double = CreateCompatibleDC(surface_app);
	HBITMAP m_hBit = CreateCompatibleBitmap(surface_app, WORLD_W, WORLD_H);
	HBITMAP m_oldhBit = static_cast<HBITMAP>(SelectObject(surface_double, m_hBit));

	// 초기화
	Render::draw_clear(surface_double, WORLD_W, WORLD_H, background_color);

	HDC surface_back = CreateCompatibleDC(surface_app);
	HBITMAP m_newBit = CreateCompatibleBitmap(surface_app, view.w, view.h);
	HBITMAP m_newoldBit = reinterpret_cast<HBITMAP>(SelectObject(surface_back, m_newBit));


	//&& player_captain == true
	if (status == LOBBY)
		sprites[2]->draw(surface_double, (VIEW_W - sprites[2]->get_width()) / 2, (VIEW_H - sprites[2]->get_height()) / 3 * 2, 0.0, 0.0, 1.0, 1.0, 1.0);

	// 파이프라인
	if (status == GAME) {
		for (auto inst = last_render_info; inst != NULL; inst++)
		{
			//if (!(view.x + view.w <= inst->bbox.left || inst->bbox.right < view.x
			//	|| view.y + view.h <= inst->bbox.top || inst->bbox.bottom < view.y))
			//	inst->draw(surface_double);
			//else {
			//}
			if (inst)
			{
				switch (inst->instance_type) {
					case CHARACTER:
					{
						player_sprite.draw(surface_double, inst->x, inst->y, inst->image_index, 0);
					}
					break;

					case BULLET:
					{
						bullet_sprite.draw(surface_double, inst->x, inst->y, inst->image_index, 0);
					}
					break;

					default:
						break;
				}
			}
		}
	}

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

	mouse_x = LOWORD(cursor) * ((float)VIEW_W / (float)CLIENT_W);
	mouse_y = HIWORD(cursor) * ((float)VIEW_H / (float)CLIENT_H);
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

void ClientFramework::SetSprite(GameSprite* sprite) {
	sprites.push_back(sprite);
	sprites[0]->get_height();
}


int ClientFramework::RecvGameMessage(SOCKET sock) {
	int retval;

	retval = recv(sock, reinterpret_cast<char*>(last_render_info), sizeof(RenderInstance) * 40, MSG_WAITALL);

	return retval;
}
PACKETS ClientFramework::RecvPacket(SOCKET sock) {

	PACKETS packet = CLIENT_PING;
	int retval = recv(sock, (char*)&packet, sizeof(int), MSG_WAITALL);
	if (retval == SOCKET_ERROR) {
	}

	return packet;
}

void SendData(SOCKET socket, PACKETS type, const char* buffer, int length) {
	int result = send(socket, reinterpret_cast<char*>(&type), sizeof(PACKETS), 0);
	if (SOCKET_ERROR == result) {
		ErrorAbort("send 1");
	}

	if (buffer) {
		result = send(socket, buffer, length, 0);
		if (SOCKET_ERROR == result) {
			ErrorAbort("send 2");
		}
	}
}

SockInfo::SockInfo(SOCKET sk, HANDLE hd) {
	client_socket = sk;
	client_handle = hd;
}

SockInfo::~SockInfo() {
}
