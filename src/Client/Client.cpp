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
ClientFramework framework{ WORLD_W, WORLD_H, VIEW_W,
                          VIEW_H,       PORT_W,       PORT_H };

// 전역 변수:
HINSTANCE hInst;                                // 현재 인스턴스입니다.
WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.

// 이 코드 모듈에 포함된 함수의 선언을 전달합니다:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 여기에 코드를 입력합니다

    // 전역 문자열을 초기화합니다.
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_CLIENT, szWindowClass, MAX_LOADSTRING);

    // 애플리케이션 초기화를 수행합니다:
    if (!game_client.initialize(hInstance, WndProc, szTitle, szWindowClass, nCmdShow))
    {
        return FALSE;
    }

    framework.Initialize();

    event_render = CreateEvent(NULL, true, false, NULL);

    MyRegisterClass(hInstance);

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CLIENT));

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

    return (int) msg.wParam;
}



//
//  함수: MyRegisterClass()
//
//  용도: 창 클래스를 등록합니다.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CLIENT));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_CLIENT);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   함수: InitInstance(HINSTANCE, int)
//
//   용도: 인스턴스 핸들을 저장하고 주 창을 만듭니다.
//
//   주석:
//
//        이 함수를 통해 인스턴스 핸들을 전역 변수에 저장하고
//        주 프로그램 창을 만든 다음 표시합니다.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  함수: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  용도: 주 창의 메시지를 처리합니다.
//
//  WM_COMMAND  - 애플리케이션 메뉴를 처리합니다.
//  WM_PAINT    - 주 창을 그립니다.
//  WM_DESTROY  - 종료 메시지를 게시하고 반환합니다.
//

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

// 정보 대화 상자의 메시지 처리기입니다.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}


DWORD WINAPI CommunicateProcess(LPVOID arg) {
	while (true) {

        PACKETS packet = CLIENT_PING;
        int retval = recv((SOCKET)arg, (char*)&packet, sizeof(PACKETS), MSG_WAITALL);
        if (retval == SOCKET_ERROR) {
        }

        if (SERVER_GAME_STATUS == packet)
        {
            int result = recv((SOCKET)arg, reinterpret_cast<char*>(&framework.playerinfo), sizeof(GameUpdateMessage), MSG_WAITALL);
            if (result == SOCKET_ERROR) {
            }
            framework.view.x = max(0, min(framework.world_w - framework.view.w, framework.playerinfo.player_x - framework.view.xoff));
            framework.view.y = max(0, min(framework.world_h - framework.view.h, framework.playerinfo.player_y - framework.view.yoff));
        }

        if (SERVER_RENDER_INFO == packet)
        {
            int result = recv((SOCKET)arg, reinterpret_cast<char*>(framework.last_render_info), sizeof(RenderInstance) * RENDER_INST_COUNT, MSG_WAITALL);
            if (result == SOCKET_ERROR) {
            }

        }

        if (SERVER_PLAYER_COUNT == packet)
        {
            int result = recv((SOCKET)arg, reinterpret_cast<char*>(&framework.player_num), sizeof(int), MSG_WAITALL);
            if (result == SOCKET_ERROR) {
            }
        }

        if (SERVER_TERRAIN_SEED == packet)
        {
            int tilex = WORLD_W / tile_sprite.get_width();      //x축 타일 갯수
            int tiley = WORLD_H / tile_sprite.get_height();     //y축 타일 갯수

            int result = recv((SOCKET)arg, reinterpret_cast<char*>(&framework.terrain_seed), sizeof(int), MSG_WAITALL);
            framework.mapdata.resize(tilex * tiley);
            fill(framework.mapdata.begin(), framework.mapdata.end(), 0);
            fill_n(framework.mapdata.begin(), 100, 1);

            std::default_random_engine rng(framework.terrain_seed);
            shuffle(framework.mapdata.begin(), framework.mapdata.end(), rng);

            for (int ty = 0; ty < tilex; ++ty)
            {
                for (int tx = 0; tx < tiley; ++tx)
                {
                    tile_sprite.draw(framework.map_surface, tx * tile_sprite.get_width(), ty * tile_sprite.get_height(), framework.mapdata[(ty * tilex) + tx], 0);
                }
            }

            SetEvent(event_render);
        }
    }
	return 0;
}