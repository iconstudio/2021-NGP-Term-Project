// Client.cpp : 애플리케이션에 대한 진입점을 정의합니다.

#include "pch.h"
#include "framework.h"
#include "Client.h"

#define MAX_LOADSTRING 100
#define RENDER_TIMER_ID 1

HANDLE event_render;

GameSprite tile_sprite(L"../../res/tiles.png", 2, 0, 0);

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

WindowsClient game_client{ CLIENT_W, CLIENT_H };
ClientFramework framework;

WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow) {
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_CLIENT, szWindowClass, MAX_LOADSTRING);

	if (!game_client.initialize(hInstance, WndProc, szTitle, szWindowClass, nCmdShow)) {
		return FALSE;
	}

	framework.Initialize();

	event_render = CreateEvent(NULL, true, false, NULL);

	MSG msg;
	while (true) {
		if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				break;
			}

			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		// 창 생성
	case WM_CREATE:
	{
		SetTimer(hwnd, RENDER_TIMER_ID, FRAME_TIME * 1000, NULL);
	}
	break;

	// 렌더링 타이머
	case WM_TIMER:
	{
		framework.Update();
		InvalidateRect(hwnd, NULL, FALSE);
	}
	break;

	// 마우스 왼쪽 누름
	case WM_LBUTTONDOWN:
	{
		//mouse_x = LOWORD(lParam) * ((float)VIEW_W / (float)CLIENT_W);
		//mouse_y = HIWORD(lParam) * ((float)VIEW_H / (float)CLIENT_H);
		framework.OnMouseDown(MK_LBUTTON, lParam);
	}
	break;

	// 마우스 왼쪽 뗌
	case WM_LBUTTONUP:
	{
		framework.OnMouseUp(MK_LBUTTON, lParam);
	}
	break;

	// 마우스 오른쪽 누름
	case WM_RBUTTONDOWN:
	{
		framework.OnMouseDown(MK_RBUTTON, lParam);
	}
	break;

	// 마우스 오른쪽 뗌
	case WM_RBUTTONUP:
	{
		framework.OnMouseUp(MK_RBUTTON, lParam);
	}
	break;

	// 마우스 휠 누름
	case WM_MBUTTONDOWN:
	{
		framework.OnMouseDown(MK_MBUTTON, lParam);
	}
	break;

	// 마우스 휠 뗌
	case WM_MBUTTONUP:
	{
		framework.OnMouseUp(MK_MBUTTON, lParam);
	}
	break;

	// 렌더링
	case WM_PAINT:
	{
		WaitForSingleObject(event_render, INFINITE);
		framework.Render(hwnd);
	}
	break;

	// 창 종료
	case WM_DESTROY:
	{
		KillTimer(hwnd, RENDER_TIMER_ID);
		PostQuitMessage(0);
	}
	break;

	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}

DWORD WINAPI CommunicateProcess(LPVOID arg) {
	while (true) {

		PACKETS packet = CLIENT_PING;
		int retval = recv((SOCKET)arg, (char*)&packet, sizeof(PACKETS), MSG_WAITALL);
		if (retval == SOCKET_ERROR) {
		}

		if (SERVER_QTE == packet)
		{
			framework.QTE = true;
		}

		if (SERVER_SET_INDEX == packet)
		{
			int result = recv((SOCKET)arg, reinterpret_cast<char*>(&framework.me), sizeof(int), MSG_WAITALL);
		}

		if (SERVER_GAME_STATUS == packet) {
			int result = recv((SOCKET)arg, reinterpret_cast<char*>(&framework.player_info), sizeof(GameUpdateMessage), MSG_WAITALL);
			if (result == SOCKET_ERROR) {
			}
			framework.view.x = max(0, min(WORLD_W - framework.view.w, framework.player_info.player_x - framework.view.xoff));
			framework.view.y = max(0, min(WORLD_H - framework.view.h, framework.player_info.player_y - framework.view.yoff));
		}

		if (SERVER_RENDER_INFO == packet) {
			int result = recv((SOCKET)arg, reinterpret_cast<char*>(framework.last_render_info), sizeof(RenderInstance) * RENDER_INST_COUNT, MSG_WAITALL);
			if (result == SOCKET_ERROR) {
			}

		}

		if (SERVER_PLAYER_COUNT == packet) {
			int result = recv((SOCKET)arg, reinterpret_cast<char*>(&framework.player_num), sizeof(int), MSG_WAITALL);
			if (result == SOCKET_ERROR) {
			}
		}

		if (SERVER_TERRAIN_SEED == packet) {
			int tilex = WORLD_W / tile_sprite.get_width();      //x축 타일 갯수
			int tiley = WORLD_H / tile_sprite.get_height();     //y축 타일 갯수

			int result = recv((SOCKET)arg, reinterpret_cast<char*>(&framework.terrain_seed), sizeof(int), MSG_WAITALL);
			framework.map_data.resize(tilex * tiley);
			fill(framework.map_data.begin(), framework.map_data.end(), 0);
			fill_n(framework.map_data.begin(), 100, 1);

			std::default_random_engine rng(framework.terrain_seed);
			shuffle(framework.map_data.begin(), framework.map_data.end(), rng);

			for (int ty = 0; ty < tilex; ++ty) {
				for (int tx = 0; tx < tiley; ++tx) {
					tile_sprite.draw(framework.map_surface, tx * tile_sprite.get_width(), ty * tile_sprite.get_height(), framework.map_data[(ty * tilex) + tx], 0);
				}
			}

			SetEvent(event_render);
		}
	}
	return 0;
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
	properties.hIcon = LoadIcon(handle, MAKEINTRESOURCE(IDI_CLIENT));
	properties.hCursor = LoadCursor(nullptr, IDC_ARROW);
	properties.hbrBackground = CreateSolidBrush(0);
	properties.lpszMenuName = NULL;
	properties.lpszClassName = reinterpret_cast<LPCWSTR>(id);
	properties.hIconSm = LoadIcon(properties.hInstance, MAKEINTRESOURCE(IDI_SMALL));
	RegisterClassEx(&properties);

	DWORD window_attributes = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
	HWND hWnd = CreateWindow(reinterpret_cast<LPCWSTR>(id), reinterpret_cast<LPCWSTR>(title), window_attributes
		, CW_USEDEFAULT, 0, width, height, nullptr, nullptr, instance, nullptr);
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
