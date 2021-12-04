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
		char* client_data = nullptr;
		int client_data_size = 0;

		// 1. Recv
		client_data = new char[sizeof(PACKETS)];
		int result = recv(client_socket, client_data, sizeof(PACKETS), 0);
		if (SOCKET_ERROR == result) {
			break;
		} else if if (SOCKET_ERROR == result) {
			break;
		}

		// 2. 게임 진행

		// 3. 게임 처리

		// 4. 렌더링 정보 작성
		BakeRenderingInfos();

		// 5. 렌더링 정보 전송
		SendRenderingInfos(client_socket);
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
			ErrorDisplay("connect()");
			continue;
		}

		auto th = CreateThread(NULL, 0, GameProcess, (&new_socket), 0, NULL);
		if (0 == th) {
			ErrorDisplay("CreateThread()");
			continue;
		}

		CloseHandle(th);
	}

	return 0;
}

void ClientConnect() {
}

void ClientDisconnect(int player_index) {
}

void ProceedContinuation() {

}

bool CheckClientNumber() {
	return false;
}

bool ValidateSocketMessage(int socket_state) {
	return false;
}

void BakeRenderingInfos() {

}

void SendRenderingInfos(SOCKET client_socket) {
	auto renderings = reinterpret_cast<char*>(rendering_infos_last);
	auto render_size = sizeof(rendering_infos_last) * RENDER_INST_COUNT;

	SendData(client_socket, SERVER_RENDER_INFO, renderings, render_size);

	Sleep(FRAME_TIME);
}

ClientSession::ClientSession(SOCKET sk, HANDLE th, int id)
	: my_socket(sk), my_thread(th)
	, player_index(id), player_character(nullptr) {
}

ClientSession::~ClientSession() {
	closesocket(my_socket);
	CloseHandle(my_thread);

	player_index = -1;

	if (player_character) {
		delete player_character;
	}
}
