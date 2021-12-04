#include "stdafx.h"
#include "Main.h"
#include "CommonDatas.h"


DWORD WINAPI GameProcess(LPVOID arg);
DWORD WINAPI ConnectProcess(LPVOID arg);

SOCKET my_socket;
RenderInstance* rendering_infos_last;

int main() {
	cout << "서버 시작" << endl;

	CreateThread(NULL, 0, ConnectProcess, nullptr, 0, NULL);
	return 0;
}

DWORD WINAPI GameProcess(LPVOID arg) {
	SOCKET client_socket = reinterpret_cast<SOCKET>(arg);

	while (true) {
		auto renderings = reinterpret_cast<char*>(rendering_infos_last);
		auto render_size = sizeof(rendering_infos_last) * RENDER_INST_COUNT;

		SendData(client_socket, SERVER_RENDER_INFO, renderings, render_size);

		Sleep(FRAME_TIME);
	}

	return 0;
}

DWORD WINAPI ConnectProcess(LPVOID arg) {
	SOCKET new_socket;
	SOCKADDR_IN my_address;
	auto my_addr_size = sizeof(my_address);

	while (true) {
		new_socket = connect(my_socket, (SOCKADDR*)(&my_address), my_addr_size);
		if (INVALID_SOCKET == new_socket) {
			ErrorAbort("connect()");
			continue;
		}

		auto th = CreateThread(NULL, 0, GameProcess, (&new_socket), 0, NULL);
		CloseHandle(th);
	}

	return 0;
}