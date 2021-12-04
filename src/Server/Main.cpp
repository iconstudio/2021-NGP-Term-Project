#include "stdafx.h"
#include "Main.h"
#include "Framework.h"
#include "CommonDatas.h"
#include "ServerFramework.h"

ServerFramework framework{};

DWORD WINAPI ConnectProcess(LPVOID arg);
DWORD WINAPI GameProcess(LPVOID arg);

SOCKET my_socket;
SOCKADDR_IN my_address;
auto my_address_size = sizeof(my_address);

HANDLE event_accept;

RenderInstance* rendering_infos_last;

int main() {
	WSADATA wsadata;
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsadata)) {
		ErrorAbort("WSAStartup()");
		return false;
	}

	my_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == my_socket) {
		ErrorAbort("socket()");
		return false;
	}

	BOOL option = TRUE;
	if (SOCKET_ERROR == setsockopt(my_socket, SOL_SOCKET, SO_REUSEADDR
		, reinterpret_cast<char*>(&option), sizeof(option))) {
		ErrorAbort("setsockopt()");
		return false;
	}

	ZeroMemory(&my_address, sizeof(my_address));
	my_address.sin_family = AF_INET;
	my_address.sin_addr.s_addr = htonl(INADDR_ANY);
	my_address.sin_port = htons(COMMON_PORT);

	if (SOCKET_ERROR == bind(my_socket, reinterpret_cast<SOCKADDR*>(&my_address), my_address_size)) {
		ErrorAbort("bind()");
		return false;
	}

	if (SOCKET_ERROR == listen(my_socket, CLIENT_NUMBER_MAX + 1)) {
		ErrorAbort("listen()");
		return false;
	}
	AtomicPrintLn("서버 시작");

	event_accept = CreateEvent(NULL, FALSE, TRUE, NULL);

	CreateThread(NULL, 0, ConnectProcess, nullptr, 0, NULL);

	while (true) {
		// 서버 대기
	}

	return 0;
}

DWORD WINAPI GameProcess(LPVOID arg) {
	SOCKET client_socket = reinterpret_cast<SOCKET>(arg);

	while (true) {
		PACKETS header;
		ZeroMemory(&header, HEADER_SIZE);

		// 1-1. 패킷 헤더 수신
		int result = recv(client_socket, reinterpret_cast<char*>(&header), HEADER_SIZE, 0);
		if (SOCKET_ERROR == result) {
			break;
		} else if (0 == result) {
			break;
		}

		char* client_data = nullptr;
		int client_data_size = 0;

		// 1-2. 패킷 내용 수신
		switch (header) {
			case PACKETS::CLIENT_KEY_INPUT:
			{
				client_data = new char[SEND_INPUT_COUNT];
				client_data_size = SEND_INPUT_COUNT;
				
				int result = recv(client_socket, client_data, client_data_size, MSG_WAITALL);
				if (SOCKET_ERROR == result) {
					break;
				} else if (0 == result) {
					break;
				}
			}

			break;

			default: break;
		}
		if (client_data)
			cout << client_data;

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
	SOCKET client_socket;
	SOCKADDR_IN client_address;
	int my_addr_size = sizeof(my_address);

	while (true) {
		client_socket = accept(my_socket, (SOCKADDR*)(&client_address), &my_addr_size);
		if (INVALID_SOCKET == client_socket) {
			ErrorDisplay("connect()");
			continue;
		}

		auto th = CreateThread(NULL, 0, GameProcess, (&client_socket), 0, NULL);
		if (!th) {
			ErrorDisplay("CreateThread()");
			continue;
		} else {
			CloseHandle(th);
		}

		WaitForSingleObject(event_accept, INFINITE);
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
/*
int main() {
        if (framework.Initialize() == -1)
        {
                return 0;
        }

        while (true)
        {
                if (!framework.Connect())
                {
                        break;
                }


        }

        framework.Close();
}
*/
