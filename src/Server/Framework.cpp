#include "stdafx.h"
#include "CommonDatas.h"
#include "Framework.h"

ServerFramework::ServerFramework() {
	PLAYER_SPAWN_PLACES = new int* [CLIENT_NUMBER_MAX];

	double dir_increment = (360.0 / CLIENT_NUMBER_MAX);
	for (int i = 0; i < CLIENT_NUMBER_MAX; ++i) {
		double dir = dir_increment * i;
		int cx = static_cast<int>(WORLD_W * 0.5 + lengthdir_x(SPAWN_DISTANCE, dir));
		int cy = static_cast<int>(WORLD_H * 0.5 + lengthdir_y(SPAWN_DISTANCE, dir));

		PLAYER_SPAWN_PLACES[i] = new int[2]{ cx, cy };
	}

	players.reserve(CLIENT_NUMBER_MAX);
	if (NULL == (event_accept = CreateEvent(NULL, FALSE, TRUE, NULL))) {
		ErrorAbort("CreateEvent[event_accept]");
		return;
	}

	if (NULL == (event_game_communicate = CreateEvent(NULL, FALSE, TRUE, NULL))) {
		ErrorAbort("CreateEvent[event_game_communicate]");
		return;
	}

	if (NULL == (event_quit = CreateEvent(NULL, FALSE, FALSE, NULL))) {
		ErrorAbort("CreateEvent[event_quit]");
		return;
	}
}

ServerFramework::~ServerFramework() {
	CloseHandle(event_accept);
	CloseHandle(event_game_communicate);
	CloseHandle(event_quit);

	closesocket(my_socket);

	WSACleanup();
}

void ServerFramework::Initialize() {
	WSADATA wsadata;
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsadata)) {
		ErrorAbort("WSAStartup()");
		return;
	}

	my_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == my_socket) {
		ErrorAbort("socket()");
		return;
	}

	BOOL option = TRUE;
	if (SOCKET_ERROR == setsockopt(my_socket, SOL_SOCKET, SO_REUSEADDR
		, reinterpret_cast<char*>(&option), sizeof(option))) {
		ErrorAbort("setsockopt()");
		return;
	}

	ZeroMemory(&my_address, sizeof(my_address));
	my_address.sin_family = AF_INET;
	my_address.sin_addr.s_addr = htonl(INADDR_ANY);
	my_address.sin_port = htons(COMMON_PORT);

	if (SOCKET_ERROR == bind(my_socket, reinterpret_cast<SOCKADDR*>(&my_address), my_address_size)) {
		ErrorAbort("bind()");
		return;
	}

	if (SOCKET_ERROR == listen(my_socket, CLIENT_NUMBER_MAX + 1)) {
		ErrorAbort("listen()");
		return;
	}

	AtomicPrintLn("서버 시작");
}

void ServerFramework::Startup() {
	// 클라이언트 연결
	if (!CreateThread(NULL, 0, ConnectProcess, nullptr, 0, NULL)) {
		ErrorAbort("CreateThread[ConnectProcess]");
	}

	Sleep(8000);
	CreatePlayerCharacters();
}

void ServerFramework::GameReady() {
}

SOCKET ServerFramework::AcceptClient() {
	SOCKADDR_IN client_address;
	int client_addr_size = sizeof(client_address);
	ZeroMemory(&client_address, client_addr_size);

	SOCKET client_socket = accept(my_socket, reinterpret_cast<SOCKADDR*>(&client_address), &client_addr_size);

	return client_socket;
}

void ServerFramework::ConnectClient(SOCKET client_socket) {
	SetEvent(event_accept);

	BOOL option = FALSE;
	setsockopt(my_socket, IPPROTO_TCP, TCP_NODELAY
		, reinterpret_cast<const char*>(&option), sizeof(option));

	auto client = new ClientSession(client_socket, NULL, players_number++);

	auto th = CreateThread(NULL, 0, GameProcess, (LPVOID)(client), 0, NULL);
	if (NULL == th) {
		ErrorDisplay("CreateThread[GameProcess]");
		return;
	}
	CloseHandle(th);

	players.push_back(client);

	AtomicPrintLn("클라이언트 접속: ", client_socket, ", 수: ", players_number);
}

void ServerFramework::DisconnectClient(ClientSession* client) {
}

void ServerFramework::CreatePlayerCharacters() {
	auto sz = players.size();
	for (int i = 0; i < sz; ++i) {
		auto player = players.at(i);
		int places[2] = { 80, 80 };//PLAYER_SPAWN_PLACES[i];
		auto character = Instantiate<CCharacter>(places[0], places[1]);

		player->player_character = character;
		character->owner = player->player_index;
		//SendData(player->my_socket, PACKETS::SERVER_GAME_START);
	}
}

void ServerFramework::ProceedContinuation() {

}

void ServerFramework::ValidateSocketMessage(int socket_state) {
}

void ServerFramework::CreateRenderingInfos() {
	if (!instances.empty()) {
		AtomicPrintLn("렌더링 정보 생성\n크기: ", instances.size());
		if (!rendering_infos_last.empty()) {
			rendering_infos_last.clear();
			rendering_infos_last.shrink_to_fit();
			rendering_infos_last.reserve(RENDER_INST_COUNT);
		}

		auto CopyList = vector<GameInstance*>(instances);

		// 플레이어 개체를 맨 위로
		std::partition(CopyList.begin(), CopyList.end(), [&](GameInstance* inst) {
			return (strcmp(inst->GetIdentifier(), "Player") == 0);
			});

		int index = 0;
		for (auto it = CopyList.begin(); it != CopyList.end(); ++it) {
			auto render_infos = (*it)->my_renders;

			// 인스턴스가 살아있는 경우에만 렌더링 메세지 전송
			if (!(*it)->dead) {
				auto dest = (rendering_infos_last.data() + index);
				auto src = &render_infos;

				memcpy(dest, src, sizeof(RenderInstance));
				index++;
			}
		}
	}
	else if (!rendering_infos_last.empty()) {
		rendering_infos_last.clear();
		rendering_infos_last.shrink_to_fit();
		rendering_infos_last.reserve(RENDER_INST_COUNT);
	}
}

void ServerFramework::SendRenderingInfos(SOCKET client_socket) {
	auto renderings = reinterpret_cast<char*>(rendering_infos_last.data());
	auto render_size = sizeof(rendering_infos_last);

	SendData(client_socket, SERVER_RENDER_INFO, renderings, render_size);
}

void ServerFramework::SetConnectProcess() {
	SetEvent(event_accept);
}

void ServerFramework::SetGameProcess() {
	SetEvent(event_game_communicate);
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
