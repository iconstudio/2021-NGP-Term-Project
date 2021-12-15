#include "pch.h"
#include "Battleroyale.h"
#include "Framework.h"

#define MAX_LOADSTRING 100
#define RENDER_TIMER_ID 1

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

WCHAR szTitle[MAX_LOADSTRING];       // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING]; // 기본 창 클래스 이름입니다.

WindowsClient game_client{ CLIENT_W, CLIENT_H };
ClientFramework framework{ WORLD_W, WORLD_H, VIEW_W,
						  VIEW_H,       PORT_W,       PORT_H };

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow) {
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_BATTLEROYAL, szWindowClass, MAX_LOADSTRING);

	if (!game_client.initialize(hInstance, WndProc, szTitle, szWindowClass, nCmdShow)) {
		return FALSE;
	}

	framework.Initialize();

	// 코드
	framework.background_color = COLOR_NAVY;
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
			SetTimer(hwnd, RENDER_TIMER_ID, 1, NULL);
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
			mouse_x = LOWORD(lParam) * ((float)VIEW_W / (float)CLIENT_W);
			mouse_y = HIWORD(lParam) * ((float)VIEW_H / (float)CLIENT_H);
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
	SOCKET my_socket = (SOCKET)arg;
	PACKETS packet;

	bool thread_done = false;

	while (true) {
		int data_size = 0;

		int result = recv(my_socket, reinterpret_cast<char*>(&packet), sizeof(PACKETS), MSG_WAITALL);
		if (packet == SERVER_PLAYER_COUNT) {
			recv(my_socket, (char*)framework.player_index, sizeof(int), MSG_WAITALL);
		}

		if (framework.player_index == 0) {
			framework.player_captain = true;
		}
		if (result == 0 || result == SOCKET_ERROR) {
			break;
		}

		for (int i = 0; i < 6; i++) {
			short check = GetAsyncKeyState(framework.keys[i].code);

			if (check & 0x0000) { // released
				framework.keys[i].type = NONE;
			} else if (check & 0x8000) {
				framework.keys[i].type = PRESS;
			} else if (check & 0x0001) {
				framework.keys[i].type = RELEASE;
			}
		}

		if (result == SOCKET_ERROR) {
		}

		if (packet == SERVER_SET_CAPATIN)
			framework.player_captain = true;

		switch (framework.GetStatus()) {

			case TITLE:
			{
				//framework.background_color = COLOR_YELLOW;
				//auto address_size = sizeof(SOCKADDR_IN);

				//if (framework.title_duration < 200)	//로비까지 시간 100 = 1초 
				//{
				//	framework.title_duration++;
				//	break;
				//}
				//int result = connect(my_socket, reinterpret_cast<sockaddr*>(&framework.server_address), address_size);
				//if (SOCKET_ERROR == result) {
				//	// 오류
				//	return 0;
				//}
				//framework.status = LOBBY;

			}
			break;

			case LOBBY:
			{
				PACKETS packet;
				recv(my_socket, reinterpret_cast<char*>(&packet), sizeof(PACKETS), MSG_WAITALL);

				if (packet == SERVER_SET_CAPATIN)
					framework.player_captain = true;

				framework.background_color = COLOR_RED;

				if (framework.player_captain == true &&
					mouse_x > VIEW_W / 2 - framework.sprites[2]->get_width() / 2 &&
					mouse_x < VIEW_W / 2 + framework.sprites[2]->get_width() / 2 &&
					mouse_y > VIEW_H / 2 - framework.sprites[2]->get_height() / 2 &&
					mouse_y < VIEW_H / 2 + framework.sprites[2]->get_height() / 2) {
				}

				if (packet == SERVER_PLAYER_COUNT) {
					result = recv(my_socket, (char*)(&framework.player_count), sizeof(int), 0);
					if (result == SOCKET_ERROR) {
					}
				}
				packet = CLIENT_GAME_START;
				SendData(my_socket, CLIENT_GAME_START, nullptr, 0);
				if (packet == SERVER_GAME_START) {
				}
				framework.status = GAME;
			}
			break;

			case GAME:
			{
				while (true) {

					framework.background_color = COLOR_GREEN;

					SendData(my_socket, CLIENT_KEY_INPUT, (char*)framework.keys, sizeof(InputStream) * 6);

					int retval;
					if (packet == SERVER_RENDER_INFO) {
						retval = recv(my_socket, reinterpret_cast<char*>(framework.last_render_info), sizeof(RenderInstance) * 40, MSG_WAITALL);
						if (retval == SOCKET_ERROR)
							int itercount = 0;
						PACKETS gamemessage = CLIENT_KEY_INPUT;

						int retval;
						if (packet == SERVER_RENDER_INFO) {
							retval = recv(my_socket, reinterpret_cast<char*>(framework.last_render_info), sizeof(RenderInstance) * 40, MSG_WAITALL);
							if (retval == SOCKET_ERROR) {
							}
						}



						if (framework.view_track_enabled) {
							SendData(my_socket, CLIENT_GAME_START, (char*)framework.keys, sizeof(InputStream) * 6);


							if (framework.view_track_enabled) {
								if (framework.view_target_player != -1) {
									framework.ViewSetPosition(framework.last_render_info[framework.view_target_player].x, framework.last_render_info[framework.view_target_player].y);
								}
							}
						}
					}
					break;

			case GAME_OVER:
			{
			}
			break;

			case SPECTATOR:
			{
				//if (view_track_enabled) {
				//	if (view_target_player != -1) {
				//		//ViewSetPosition(view_target->x, view_target->y);
				//	}
				//}
			}
			break;

			case GAME_RESTART:
			{
				//TODO
			}
			break;

			case EXIT:
			{
				thread_done = true;
			}
			break;

			default:
			break;
				}
			}

			return 0;
		}
	}
}
