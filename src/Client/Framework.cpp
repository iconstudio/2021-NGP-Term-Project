#include "pch.h"
#include "Framework.h"
#include "Resource.h"

GameSprite player_sprite(L"../../res/PlayerWalkDown_strip6.png", 6, 16, 50);
GameSprite player_rightsprite(L"../../res/PlayerWalkRight_strip4.png", 4, 16, 50);
GameSprite player_backsprite(L"../../res/PlayerWalkUp_strip4.png", 4, 16, 50);
GameSprite bullet_sprite(L"../../res/Snowball.png", 1, 17, 17);
GameSprite health_sprite(L"../../res/health.png", 3, 0, 0);


ClientFramework::ClientFramework(int vw, int vh, int pw, int ph)
	: painter{}
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

	int option = FALSE;
	setsockopt(my_socket, IPPROTO_TCP, TCP_NODELAY
			   , reinterpret_cast<const char*>(&option), sizeof(option));

	auto address_size = sizeof(server_address);
	ZeroMemory(&server_address, address_size);
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = inet_addr(SERVER_IP);
	server_address.sin_port = htons(COMMON_PORT);

	for (int t = 0; t < 40; ++t) {
		last_render_info[t].instance_type = BLANK;
	}

	HDC WorldDC = GetDC(NULL);
	map_surface = CreateCompatibleDC(WorldDC);
	map_bitmap = CreateCompatibleBitmap(WorldDC, WORLD_W, WORLD_H);
	SelectObject(map_surface, map_bitmap);

	Render::draw_clear(map_surface, WORLD_W, WORLD_H, COLOR_BLACK);

	int result = connect(my_socket, reinterpret_cast<sockaddr*>(&server_address), address_size);

	if (SOCKET_ERROR == result) {
		// 오류
		ErrorAbort("connect error");
	}

	CreateThread(NULL, 0, ::CommunicateProcess, (void*)my_socket, 0, NULL);
}

void ClientFramework::Update() {
	background_color = COLOR_YELLOW;

	ZeroMemory(key_checkers, sizeof(key_checkers));

	if (GetAsyncKeyState(VK_UP) & 0x8000 || GetAsyncKeyState(VK_UP) & 0x8001)
		key_checkers[0] = VK_UP;
	if (GetAsyncKeyState(VK_DOWN) & 0x8000 || GetAsyncKeyState(VK_DOWN) & 0x8001)
		key_checkers[1] = VK_DOWN;
	if (GetAsyncKeyState(VK_LEFT) & 0x8000 || GetAsyncKeyState(VK_LEFT) & 0x8001)
		key_checkers[2] = VK_LEFT;
	if (GetAsyncKeyState(VK_RIGHT) & 0x8000 || GetAsyncKeyState(VK_RIGHT) & 0x8001)
		key_checkers[3] = VK_RIGHT;
	if ((GetAsyncKeyState('A') & 0x8000 || GetAsyncKeyState('A') & 0x8001) && bulletcooldown <= 0) {
		key_checkers[4] = 'A';
		bulletcooldown = FRAMERATE;
		--bulletleft;
	}

	if (GetAsyncKeyState('S') & 0x8000 || GetAsyncKeyState('S') & 0x8001)
		key_checkers[5] = 'S';
	if (GetAsyncKeyState('D') & 0x8000 || GetAsyncKeyState('D') & 0x8001)
		key_checkers[6] = 'D';
	if (GetAsyncKeyState('F') & 0x8000 || GetAsyncKeyState('F') & 0x8001) {
		key_checkers[7] = 'F';
		bulletleft = 3;
	}

	if (key_checkers) {
		SendData(my_socket, CLIENT_KEY_INPUT, key_checkers, sizeof(key_checkers));
	}

	if (0 < bulletcooldown) {
		bulletcooldown -= FRAME_TIME;
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

	Render::draw_clear(surface_back, WORLD_W, WORLD_H, background_color);

	//&& player_captain == true
	//if (status == LOBBY)
	//	sprites[2]->draw(surface_double, (VIEW_W - sprites[2]->get_width()) / 2, (VIEW_H - sprites[2]->get_height()) / 3 * 2, 0.0, 0.0, 1.0, 1.0, 1.0);

	BitBlt(surface_double, 0, 0, WORLD_W, WORLD_H, map_surface, 0, 0, SRCCOPY);

	for (auto it = begin(last_render_info); it != end(last_render_info); it++) {
		if (it) {
			switch (it->instance_type) {
				case CHARACTER:
				{
					player_sprite.draw(surface_double, it->x, it->y, it->image_index, 0);
				}
				break;

				case BULLET:
				{
					bullet_sprite.draw(surface_double, it->x, it->y, it->image_index, 0);
				}
				break;

				case BLANK:
				break;

				default:
				break;
			}
		}
	}

	TextOut(surface_double, VIEW_W / 2, 0, TEXT(""), 12);

	// 이중 버퍼 -> 백 버퍼
	BitBlt(surface_back, 0, 0, view.w, view.h, surface_double, view.x, view.y, SRCCOPY);
	Render::draw_end(surface_double, m_oldhBit, m_hBit);

	// UI
	health_sprite.draw(surface_back, 0, 0, 3 - hp / 33, 0, 0.5, 0.5);

	for (int curbullet = 0; curbullet < bulletleft; ++curbullet) {
		bullet_sprite.draw(surface_back, VIEW_W - (bullet_sprite.get_width() / 2 + 10) * curbullet - 40, VIEW_H - (bullet_sprite.get_height() / 2 * 3), 0, 0, 0.8, 0.8, 0.5);
	}

	// 백 버퍼 -> 화면 버퍼
	StretchBlt(surface_app, port.x, port.y, port.w, port.h
		, surface_back, 0, 0, view.w, view.h, SRCCOPY);
	Render::draw_end(surface_back, m_newoldBit, m_newBit);

	DeleteDC(surface_back);
	DeleteDC(surface_double);
	ReleaseDC(window, surface_app);
	EndPaint(window, &painter);
}

PACKETS ClientFramework::RecvPacket(SOCKET sock) {
	PACKETS packet = CLIENT_PING;
	int retval = recv(sock, reinterpret_cast<char*>(&packet), sizeof(PACKETS), MSG_WAITALL);
	if (retval == SOCKET_ERROR) {
		ErrorAbort("recv packet");
	}
	return packet;
}

void ClientFramework::OnMouseDown(const WPARAM button, const LPARAM cursor) {
	auto vk_status = key_checkers[button];
	//vk_status.on_press();

	mouse_x = LOWORD(cursor) * ((float)VIEW_W / (float)CLIENT_W);
	mouse_y = HIWORD(cursor) * ((float)VIEW_H / (float)CLIENT_H);
}

void ClientFramework::OnMouseUp(const WPARAM button, const LPARAM cursor) {
	auto vk_status = key_checkers[button];
	//vk_status.on_release();

	//mouse_x = LOWORD(cursor);
	//mouse_y = HIWORD(cursor);
}
