#include "stdafx.h"
#include "CommonDatas.h"
#include "Framework.h"

ServerFramework::ServerFramework()
	: status(SERVER_STATES::LOBBY)
	, players_survived(0)
	, QTE_time(QTE_PERIOD_MAX)
	, randomizer(std::random_device{}()), random_distrubution() {
	InitializeCriticalSection(&permission_print);

	PLAYER_SPAWN_PLACES = new int* [CLIENT_NUMBER_MAX];

	double dir_increment = (360.0 / CLIENT_NUMBER_MAX);
	for (int i = 0; i < CLIENT_NUMBER_MAX; ++i) {
		double dir = dir_increment * i;
		int cx = static_cast<int>(WORLD_W / 2 + lengthdir_x(SPAWN_DISTANCE, dir));
		int cy = static_cast<int>(WORLD_H / 2 + lengthdir_y(SPAWN_DISTANCE, dir));

		PLAYER_SPAWN_PLACES[i] = new int[2]{ cx, cy };
	}

	players.reserve(CLIENT_NUMBER_MAX);
	if (NULL == (event_accept = CreateEvent(NULL, FALSE, TRUE, NULL))) {
		ErrorAbort("CreateEvent[event_accept]");
		return;
	}

	if (NULL == (event_game_communicate = CreateEvent(NULL, FALSE, FALSE, NULL))) {
		ErrorAbort("CreateEvent[event_game_communicate]");
		return;
	}

	if (NULL == (event_game_update = CreateEvent(NULL, FALSE, FALSE, NULL))) {
		ErrorAbort("CreateEvent[event_game_update]");
		return;
	}

	if (NULL == (event_quit = CreateEvent(NULL, FALSE, FALSE, NULL))) {
		ErrorAbort("CreateEvent[event_quit]");
		return;
	}
}

ServerFramework::~ServerFramework() {
	for (int i = 0; i < CLIENT_NUMBER_MAX; ++i) {
		delete PLAYER_SPAWN_PLACES[i];
	}

	delete[] PLAYER_SPAWN_PLACES;

	players.clear();
	instances.clear();
	rendering_infos_last.clear();

	players.shrink_to_fit();
	instances.shrink_to_fit();
	rendering_infos_last.shrink_to_fit();

	DeleteCriticalSection(&permission_print);

	CloseHandle(event_accept);
	CloseHandle(event_game_communicate);
	CloseHandle(event_game_update);
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

	// 게임 처리
	if (!CreateThread(NULL, 0, GameUpdateProcess, nullptr, 0, NULL)) {
		ErrorAbort("CreateThread[ConnectProcess]");
	}

	CastReceiveEvent(true);
}

void ServerFramework::GameReady() {
	CreatePlayerCharacters();
	SendGameBeginMsgToAll();

	auto sz = players.size();
	for (int i = 0; i < sz; ++i) {
		auto player = players.at(i);
		int player_socket = player->my_socket;

		SendPlayersCount(player_socket);
	}

	SetStatus(SERVER_STATES::GAME);
}

bool ServerFramework::GameUpdate() {
	for (auto inst : instances) {
		inst->OnUpdate(FRAME_TIME);
	}
	QTE_time -= FRAME_TIME;

	return true;
}

void ServerFramework::SetStatus(SERVER_STATES state) {
	if (status != state) {
		AtomicPrintLn("서버의 상태 변경: ", (int)status, " → ", (int)state);
		status = state;
	}
}

SERVER_STATES ServerFramework::GetStatus() const {
	return status;
}

SOCKET ServerFramework::AcceptClient() {
	SOCKADDR_IN client_address;
	int client_addr_size = sizeof(client_address);
	ZeroMemory(&client_address, client_addr_size);

	SOCKET client_socket = accept(my_socket, reinterpret_cast<SOCKADDR*>(&client_address), &client_addr_size);

	return client_socket;
}

void ServerFramework::ConnectClient(SOCKET client_socket) {
	if (GetStatus() == SERVER_STATES::LOBBY) {
		CastAcceptEvent(true);
	}

	BOOL option = TRUE; // Nagle 알고리즘
	setsockopt(my_socket, IPPROTO_TCP, TCP_NODELAY
		, reinterpret_cast<const char*>(&option), sizeof(option));

	auto client = new ClientSession(client_socket, player_index_last);

	auto th = CreateThread(NULL, 0, GameProcess, (LPVOID)(client), 0, NULL);
	if (NULL == th) {
		ErrorDisplay("CreateThread[GameProcess]");
		return;
	}
	CloseHandle(th);

	SendData(client_socket, PACKETS::SERVER_SET_INDEX
			 , reinterpret_cast<char*>(&player_index_last), sizeof(player_index_last));

	if (0 == GetPlayerNumber()) {
		SendData(client_socket, PACKETS::SERVER_SET_CAPTAIN);
	}

	SendTerrainSeed(client_socket);

	players.push_back(client);

	AtomicPrintLn("클라이언트 접속: ", client_socket, ", 수: ", ++players_number);
	player_index_last++;
}

vector<ClientSession*>::iterator ServerFramework::DisconnectClient(ClientSession* client) {
	auto iter = std::find(players.begin(), players.end(), client);
	if (iter != players.end()) {
		players_number--;
		AtomicPrintLn("클라이언트 종료: ", client->my_socket, ", 수: ", players_number);

		iter = players.erase(iter);
		closesocket(client->my_socket);

		if (client->player_character) {
			Kill(client->player_character);
		}

		if (1 < players_number) {
			CastUpdateEvent(true);
		}
	}

	return iter;
}

void ServerFramework::ProceedContinuation() {
	if (players_number <= player_process_index++) {
		// 플레이어 사망 확인
		int player_alives = 0;
		ClientSession* survivor = nullptr;

		for (auto it = players.begin(); it != players.end(); ++it) { // ++ 오류
			auto player = *it;

			if (player) {
				// 플레이어 사망
				if (player->player_character && player->player_character->dead) {
					//it = DisconnectClient(player);
				} else {
					survivor = player;
					player_alives++;
				}/* else {
					it = DisconnectClient(player);
				}*/
			}

			if (players_number <= 1) {
				if (0 == players_number) {
					CastQuitEvent();
					break;
				} else if (1 == players_number) {
					// 부전승!
					break;
				} else if (1 == player_alives) {
					// 승리!
					break;
				}
			}
		}

		if (players.empty()) {
			// 종료
			CastQuitEvent();
		} else if (1 == players_number) {
			// 부전승
			auto winner = players.at(0);
			SendNotificationToTheWinner(winner->my_socket);
		} else if (survivor && 1 == player_alives) {
			//승리
			SendNotificationToTheWinner(survivor->my_socket);
		} else {
			// 모든 플레이어의 수신이 종료되면 렌더링으로 이벤트 전환
			CastUpdateEvent(true);
		}

		player_process_index = 0;
	} else {
		// 수신
		CastReceiveEvent(true);
	}
}

bool ServerFramework::ValidateSocketMessage(int socket_state) {
	if (SOCKET_ERROR == socket_state) {
		return false;
	} else if (0 == socket_state) {
		return false;
	}

	return true;
}

void ServerFramework::CreatePlayerCharacters() {
	auto sz = players.size();
	for (int i = 0; i < sz; ++i) {
		auto player = players.at(i);
		auto places = PLAYER_SPAWN_PLACES[i];
		auto character = Instantiate<CCharacter>(places[0], places[1]);
		character->owner = player->player_index;

		player->player_character = character;
		//SendData(player->my_socket, PACKETS::SERVER_GAME_START);
	}
}

void ServerFramework::CreateRenderingInfos() {
	if (!instances.empty()) {
		players_survived = 0;
		auto sz = players.size();
		for (int i = 0; i < sz; ++i) {
			auto player = players.at(i);
			auto places = PLAYER_SPAWN_PLACES[i];
			auto character = player->player_character;
			if (character && !character->dead) {
				players_survived++;
			}
		}

		AtomicPrintLn("렌더링 정보 생성\n크기: ", instances.size());
		if (!rendering_infos_last.empty()) {
			rendering_infos_last.clear();
			rendering_infos_last.shrink_to_fit();
			rendering_infos_last.reserve(RENDER_INST_COUNT);
		}

		auto CopyList = vector<GameInstance*>(instances);

		// 플레이어 개체를 맨 위로
		std::partition(CopyList.begin(), CopyList.end(), [&](GameInstance* inst) {
			return (strcmp(inst->GetIdentifier(), "Player") != 0);
		});

		int index = 0;
		for (auto it = CopyList.begin(); it != CopyList.end(); ++it) {
			auto render_infos = (*it)->my_renders;

			// 인스턴스가 살아있는 경우에만 렌더링 메세지 전송
			if (!(*it)->dead) {
				auto src = render_infos;

				rendering_infos_last.push_back(src);
				index++;
			}
		}
	} else if (!rendering_infos_last.empty()) {
		rendering_infos_last.clear();
		rendering_infos_last.shrink_to_fit();
		rendering_infos_last.reserve(RENDER_INST_COUNT);
	}
}

void ServerFramework::SendGameBeginMsgToAll() {
	for (int i = 0; i < players.size(); ++i) {
		auto player = players.at(i);
		int player_socket = player->my_socket;

		SendData(player_socket, PACKETS::SERVER_GAME_START);
	}
}

void ServerFramework::SendTerrainSeed(SOCKET client_socket) {
	int seed = random_distrubution(randomizer);
	SendData(client_socket, PACKETS::SERVER_TERRAIN_SEED
		, reinterpret_cast<char*>(&seed), sizeof(seed));
}

void ServerFramework::SendPlayersCount(SOCKET client_socket) {
	SendData(client_socket, SERVER_PLAYER_COUNT
			 , reinterpret_cast<char*>(&players_number), sizeof(players_number));
}

void ServerFramework::SendGameStatus(ClientSession* client) {
	auto client_socket = client->my_socket;
	auto player_index = client->player_index;
	auto player_character = client->player_character;

	if (player_character) {
		GameUpdateMessage state;
		state.players_count = players_survived;
		state.player_hp = player_character->health;
		state.player_inv = player_character->invincible;
		state.player_x = player_character->x;
		state.player_y = player_character->y;
		state.player_direction = player_character->direction;

		SendData(client_socket, SERVER_GAME_STATUS
				 , reinterpret_cast<char*>(&state), sizeof(GameUpdateMessage));
	}
}

void ServerFramework::SendRenderingInfos(SOCKET client_socket) {
	auto renderings = reinterpret_cast<char*>(rendering_infos_last.data());
	auto render_size = sizeof(RenderInstance) * RENDER_INST_COUNT;

	AtomicPrintLn("렌더링 정보 전송 (크기: ", render_size, ")");
	SendData(client_socket, SERVER_RENDER_INFO, renderings, render_size);
}

void ServerFramework::SendGameInfosToAll() {
	if (QTE_time <= 0) {
		uniform_int_distribution<> qte_distrubution(0, players.size() - 1);

		auto player_pos = qte_distrubution(randomizer);
		auto player = players.at(player_pos);
		auto client_socket = player->my_socket;
		SendData(client_socket, PACKETS::SERVER_QTE);

		QTE_time = QTE_PERIOD_MAX;
	}

	for (int i = 0; i < players.size(); ++i) {
		auto player = players.at(i);
		int player_socket = player->my_socket;

		SendRenderingInfos(player_socket);
		SendGameStatus(player);
	}
}

void ServerFramework::SendNotificationToTheWinner(SOCKET client_socket) {
	SendData(client_socket, PACKETS::SERVER_GAME_DONE);
}

void ServerFramework::CastAcceptEvent(bool flag) {
	if (flag)
		SetEvent(event_accept);
	else
		ResetEvent(event_accept);
}

void ServerFramework::CastReceiveEvent(bool flag) {
	if (flag)
		SetEvent(event_game_communicate);
	else
		ResetEvent(event_game_communicate);
}

void ServerFramework::CastUpdateEvent(bool flag) {
	if (flag)
		SetEvent(event_game_update);
}

void ServerFramework::CastQuitEvent() {
	SetEvent(event_quit);
}

ClientSession::ClientSession(SOCKET sk, int id)
	: my_socket(sk), player_index(id), player_character(nullptr) {}

ClientSession::~ClientSession() {
	player_index = -1;

	if (player_character) {
		delete player_character;
	}
}
