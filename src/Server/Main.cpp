#include "stdafx.h"
#include "Main.h"
#include "Framework.h"
#include "CommonDatas.h"
#include "ServerFramework.h"
#include "CommonDatas.h"

//ServerFramework framework{};

// 스레드 프로세스
DWORD WINAPI ConnectProcess(LPVOID arg);
DWORD WINAPI GameProcess(LPVOID arg);

// 소켓 정보
SOCKET my_socket;
SOCKADDR_IN my_address;
int my_address_size = sizeof(my_address);

HANDLE event_accept; // 클라이언트 수용 신호
HANDLE event_game_process; // 게임 처리 신호

RenderInstance rendering_infos_last[RENDER_INST_COUNT];

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

	client_number = 0;
	players.reserve(CLIENT_NUMBER_MAX);

	PLAYER_SPAWN_PLACES = new int* [CLIENT_NUMBER_MAX];

	double dir_increment = (360.0 / CLIENT_NUMBER_MAX);
	for (int i = 0; i < CLIENT_NUMBER_MAX; ++i) {
		double dir = dir_increment * i;
		int cx = static_cast<int>(WORLD_W * 0.5 + lengthdir_x(SPAWN_DISTANCE, dir));
		int cy = static_cast<int>(WORLD_W * 0.5 + lengthdir_y(SPAWN_DISTANCE, dir));

		PLAYER_SPAWN_PLACES[i] = new int[2]{ cx, cy };
	}

	event_accept = CreateEvent(NULL, FALSE, TRUE, NULL);
	event_game_process = CreateEvent(NULL, FALSE, FALSE, NULL);

	// 클라이언트 연결
	CreateThread(NULL, 0, ConnectProcess, nullptr, 0, NULL);

	Sleep(8000);
	CreatePlayerCharacters();
	SetEvent(event_game_process);

	while (true) {
		// 서버 대기
	}

	CloseHandle(event_accept);
	CloseHandle(event_game_process);
	closesocket(my_socket);

	return 0;
}

CCharacter::CCharacter()
	: GameInstance() {
	SetRenderType(RENDER_TYPES::CHARACTER);
	SetBoundBox(RECT{ -6, -6, 6, 6 });
}
DWORD WINAPI ConnectProcess(LPVOID arg) {
	while (true) {
		SOCKET client_socket;
		SOCKADDR_IN client_address;
		int my_addr_size = sizeof(client_address);

		SetEvent(event_accept);

		client_socket = accept(my_socket, reinterpret_cast<SOCKADDR*>(&client_address), &my_addr_size);
		if (INVALID_SOCKET == client_socket) {
			ErrorDisplay("connect()");
			continue;
		}

		BOOL option = FALSE;							//네이글 알고리즘 on/off
		setsockopt(my_socket,						//해당 소켓
			IPPROTO_TCP,							//소켓의 레벨
			TCP_NODELAY,							//설정 옵션
			reinterpret_cast<const char*>(&option),	// 옵션 포인터
			sizeof(option));						//옵션 크기

		auto client = new ClientSession(client_socket, NULL, client_number++);

		auto th = CreateThread(NULL, 0, GameProcess, (LPVOID)(client), 0, NULL);
		if (!th) {
			ErrorDisplay("CreateThread()");
			continue;
		}
		CloseHandle(th);
		players.push_back(client);

		AtomicPrintLn("클라이언트 접속: ", client_socket, ", 수: ", client_number);

		WaitForSingleObject(event_accept, INFINITE);
	}

	return 0;
}

DWORD WINAPI GameProcess(LPVOID arg) {
	ClientSession* client = reinterpret_cast<ClientSession*>(arg);
	SOCKET client_socket = client->my_socket;

	while (true) {
		WaitForSingleObject(event_game_process, INFINITE);

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
		BakeRenderingInfos();

		// 5. 렌더링 정보 전송
		SendRenderingInfos(client_socket);

		// 6. 대기
		Sleep(FRAME_TIME);
		SetEvent(event_game_process);
	}

	return 0;
}

void ClientConnect() {
}

void ClientDisconnect(int player_index) {
}

void CreatePlayerCharacters() {
	auto sz = players.size();
	for (int i = 0; i < sz; ++i) {
		auto player = players.at(i);
		int places[2] = {80, 80};//PLAYER_SPAWN_PLACES[i];
		auto character = Instantiate<CCharacter>(places[0], places[1]);

		player->player_character = character;
		character->owner = player->player_index;
		//SendData(player->my_socket, PACKETS::SERVER_GAME_START);
	}
}

void ProceedContinuation() {

}

bool CheckClientNumber() {
	return false;
}

bool ValidateSocketMessage(int socket_state) {
	return false;
}

// 렌더링 정보 생성 함수
void BakeRenderingInfos() {
	if (!instances.empty()) {
		AtomicPrintLn("렌더링 정보 생성\n크기: ", instances.size());
		if (rendering_infos_last) {
			ZeroMemory(rendering_infos_last, sizeof(rendering_infos_last));
		}

		auto CopyList = vector<GameInstance*>(instances);

		// 플레이어 개체를 맨 위로
		std::partition(CopyList.begin(), CopyList.end(), [&] (GameInstance* inst) {
			return (strcmp(inst->GetIdentifier(), "Player") == 0);
		});

		int index = 0;
		for (auto it = CopyList.begin(); it != CopyList.end(); ++it) {
			auto render_infos = (*it)->GetRenderInstance();

			// 인스턴스가 살아있는 경우에만 렌더링 메세지 전송
			if (!(*it)->dead) {
				auto dest = (rendering_infos_last + index);
				auto src = &render_infos;

				memcpy(dest, src, sizeof(RenderInstance));
				index++;
			}
		}
	} else if (rendering_infos_last) {
		if (rendering_infos_last) {
			ZeroMemory(rendering_infos_last, sizeof(rendering_infos_last));
		}
	}
}

// 렌더링 정보 전송
void SendRenderingInfos(SOCKET client_socket) {
	auto renderings = reinterpret_cast<char*>(rendering_infos_last);
	auto render_size = sizeof(rendering_infos_last);

	SendData(client_socket, SERVER_RENDER_INFO, renderings, render_size);
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
