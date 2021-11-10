#include "pch.h"
#include "Framework.h"
#include "resource.h"


ClientFramework::ClientFramework(int rw, int rh, int vw, int vh, int pw, int ph)
	: painter{}
	, WORLD_W(rw), WORLD_H(rh)
	, view{ 0, 0, vw, vh }, port{ 0, 0, pw, ph }
	, view_track_enabled(false), view_target_player(-1) {
	view.xoff = vw * 0.5;
	view.yoff = vh * 0.5;
	port.x = (CLIENT_W - pw) * 0.5;
}

ClientFramework::~ClientFramework() {}

void ClientFramework::Initialize() {
	InputRegister(MK_LBUTTON);
	InputRegister(MK_RBUTTON);
	InputRegister(MK_MBUTTON);
	InputRegister(VK_ESCAPE);
}

void ClientFramework::Update() {
	for (auto& key_pair : key_checkers) {
		short check = GetAsyncKeyState(key_pair.first);

		auto state = key_pair.second;

		if (HIBYTE(check) == 0) { // released
			state.on_release();
		} else if (check & 0x8000) {
			state.on_press();
		}
	}

	// �ڵ�

	if (view_track_enabled) {
		if (view_target_player != -1) {
			//ViewSetPosition(view_target->x, view_target->y);
		}
	}
}

void ClientFramework::Render(HWND window) {
	HDC surface_app = BeginPaint(window, &painter);

	HDC surface_double = CreateCompatibleDC(surface_app);
	HBITMAP m_hBit = CreateCompatibleBitmap(surface_app, WORLD_W, WORLD_H);
	HBITMAP m_oldhBit = (HBITMAP)SelectObject(surface_double, m_hBit);

	// �ʱ�ȭ
	//Render::draw_clear(surface_double, world_w, world_h, background_color);

	HDC surface_back = CreateCompatibleDC(surface_app);
	HBITMAP m_newBit = CreateCompatibleBitmap(surface_app, view.w, view.h);
	HBITMAP m_newoldBit = (HBITMAP)SelectObject(surface_back, m_newBit);

	// ����������
	/*
	for_each_instances([&](GameInstance*& inst) {
		if (inst->sprite_index) {
			if (!(view.x + view.w <= inst->bbox_left() || inst->bbox_right() < view.x
				|| view.y + view.h <= inst->bbox_top() || inst->bbox_bottom() < view.y))
				inst->on_render(surface_double);
		} else {
			inst->on_render(surface_double);
		}
	});*/

	// ���� ���� -> �� ����
	BitBlt(surface_back, 0, 0, view.w, view.h, surface_double, view.x, view.y, SRCCOPY);
	//Render::draw_end(surface_double, m_oldhBit, m_hBit);

	// �� ���� -> ȭ�� ����
	StretchBlt(surface_app, port.x, port.y, port.w, port.h
			   , surface_back, 0, 0, view.w, view.h, SRCCOPY);
	//Render::draw_end(surface_back, m_newoldBit, m_newBit);

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
	properties.lpszClassName = id;
	properties.hIconSm = LoadIcon(properties.hInstance, MAKEINTRESOURCE(IDI_SMALL));
	RegisterClassEx(&properties);

	DWORD window_attributes = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
	HWND hWnd = CreateWindow(id, title, window_attributes
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
