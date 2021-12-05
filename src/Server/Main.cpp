#include "stdafx.h"
#include "Framework.h"
#include "CommonDatas.h"
#include "Main.h"
#include "ServerFramework.h"

// 스레드 프로세스
DWORD WINAPI ConnectProcess(LPVOID arg);
DWORD WINAPI GameProcess(LPVOID arg);

ServerFramework f{};

int main() {
	f.Initialize();

	CreateThread(NULL, 0, ConnectProcess, nullptr, 0, NULL);

	Sleep(8000);
	f.CreatePlayer();
	f.SetGameProcess();

	// 클라이언트 연결
	while (true)
	{
	}
}

CCharacter::CCharacter() : GameInstance() {
	SetRenderType(RENDER_TYPES::CHARACTER);
	SetBoundBox(RECT{ -6, -6, 6, 6 });
}

DWORD WINAPI ConnectProcess(LPVOID arg) {
	while (true) {
		SOCKET listen_socket = f.GetListenSocket();
		SOCKET client_socket;
		SOCKADDR_IN client_address;
		int my_addr_size = sizeof(client_address);

		f.SetConnectProcess();

		client_socket = accept(listen_socket, reinterpret_cast<SOCKADDR*>(&client_address), &my_addr_size);
		if (INVALID_SOCKET == client_socket) {
			ErrorDisplay("connect()");
			continue;
		}

		BOOL option = FALSE;						//네이글 알고리즘 on/off
		setsockopt(listen_socket,					//해당 소켓
			IPPROTO_TCP,							//소켓의 레벨
			TCP_NODELAY,							//설정 옵션
			reinterpret_cast<const char*>(&option),	// 옵션 포인터
			sizeof(option));						//옵션 크기

		auto client = new ClientSession(client_socket, NULL, f.GetPlayerNumber());

		auto th = CreateThread(NULL, 0, GameProcess, reinterpret_cast<LPVOID>(client), 0, NULL);
		if (!th) {
			ErrorDisplay("CreateThread()");
			continue;
		}
		CloseHandle(th);
		f.AddPlayer(client);

		AtomicPrintLn("클라이언트 접속: ", client_socket, ", 수: ", f.GetPlayerNumber());

		WaitForSingleObject(f.GetAcceptEvent(), INFINITE);
	}

	return 0;
}

DWORD WINAPI GameProcess(LPVOID arg) {
	ClientSession* client = reinterpret_cast<ClientSession*>(arg);
	SOCKET client_socket = client->my_socket;

	while (true) {
		WaitForSingleObject(f.GetGameProcessEvent(), INFINITE);

		PACKETS header;
		ZeroMemory(&header, HEADER_SIZE);

		// 1-1. 패킷 헤더 수신
		int result = recv(client_socket, reinterpret_cast<char*>(&header), HEADER_SIZE, MSG_WAITALL);
		if (SOCKET_ERROR == result) {
			break;
		} else if (0 == result) {
			break;
		}
		AtomicPrintLn("받은 패킷 헤더: ", header);

		char* client_data = nullptr;
		int client_data_size = 0;

		// 1-2. 패킷 내용 수신
		switch (header) {
			case PACKETS::CLIENT_KEY_INPUT:
			{
				client_data = new char[SEND_INPUT_COUNT];
				client_data_size = SEND_INPUT_COUNT;
				ZeroMemory(client_data, client_data_size);

				int result = recv(client_socket, client_data, client_data_size, MSG_WAITALL);
				if (SOCKET_ERROR == result) {
					break;
				} else if (0 == result) {
					break;
				}
				AtomicPrintLn("받은 패킷 내용: ", client_data);
			}
			break;

			case PACKETS::CLIENT_PING:
			{
				// 아무것도 안함
			}
			break;

			default: break;
		}

		// 2. 게임 진행
		CCharacter* client_char = client->player_character;
		if (!client_char) {
			//client_char = Instantiate<CCharacter>(50, 50);
		}

		if (client_char) {
			for (int i = 0; i < client_data_size; ++i) {
				auto input = client_data[i];

				switch (input) {
					case VK_UP:
					{
						client_char->y -= 2;
					}
					break;

					case VK_LEFT:
					{
						client_char->x -= 2;
					}
					break;

					case VK_RIGHT:
					{
						client_char->x += 2;
					}
					break;

					case VK_DOWN:
					{
						client_char->y += 2;
					}
					break;
				}
			}
			client_char->AssignRenderingInfo(0);
		}

		// 3. 게임 처리

		// 4. 렌더링 정보 작성
		f.CreateRenderingInfos();

		// 5. 렌더링 정보 전송
		f.SendRenderingInfos(client_socket);

		// 6. 대기
		Sleep(FRAME_TIME);
		f.SetGameProcess();
	}

	return 0;
}

const char* CCharacter::GetIdentifier() const { return "Player"; }

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
