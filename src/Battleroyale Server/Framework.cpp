#include "pch.h"
#include "Framework.h"


ServerFramework::ServerFramework(int rw, int rh)
	: WORLD_W(rw), WORLD_H(rh), SPAWN_DISTANCE(rh * 0.4)
	, status(SERVER_STATES::LISTEN)
	, my_socket(0), my_address(), client_number(0)
	, player_number_last(0), player_captain(-1) {

	players.reserve(PLAYERS_NUMBER_MAX);

	PLAYER_SPAWN_PLACES = new int* [PLAYERS_NUMBER_MAX];

	double dir_increment = (360.0 / PLAYERS_NUMBER_MAX);
	for (int i = 0; i < PLAYERS_NUMBER_MAX; ++i) {
		double dir = dir_increment * i;
		int cx = (int)(WORLD_W * 0.5 + lengthdir_x(SPAWN_DISTANCE, dir));
		int cy = (int)(WORLD_W * 0.5 + lengthdir_y(SPAWN_DISTANCE, dir));

		PLAYER_SPAWN_PLACES[i] = new int[2]{ cx, cy };
	}
}

ServerFramework::~ServerFramework() {
	closesocket(my_socket);

	for (auto& player : players) {
		CloseHandle(player->client_handle);
	}
	players.clear();

	CloseHandle(thread_game_process);
	CloseHandle(event_receives);
	CloseHandle(event_game_process);
	CloseHandle(event_send_renders);
}

void ServerFramework::SetStatus(SERVER_STATES state) {
	if (status != state) {
		cout << "서버 상태 변경: " << status << " -> " << state << endl;

		status = state;
	}
}

void ServerFramework::Initialize() {
	WSADATA wsadata;
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsadata)) {
		// 오류
		return;
	}

	my_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == my_socket) {
		// 오류
		return;
	}

	int result = listen(my_socket, PLAYERS_NUMBER_MAX + 1);
	if (SOCKET_ERROR == result) {
		// 오류
		return;
	}

	thread_game_process = CreateThread(NULL, 0, GameProcess, nullptr, 0, NULL);
}

void ServerFramework::Startup() {
	switch (status) {
		case LISTEN:
		{
			cout << "첫번째 클라이언트 대기 중" << endl;

			while (true) {
				SOCKET new_client = PlayerConnect(0);
				if (INVALID_SOCKET == new_client) {
					cerr << "accept 오류!";
					return;
				}

				// 첫번째 플레이어 접속
				SetStatus(LOBBY);
				break;
			}
		}
		break;

		case LOBBY:
		{
			cout << "대기실 입장" << endl;

			while (true) {
				SOCKET new_client = PlayerConnect(0);
				if (INVALID_SOCKET == new_client) {
					cerr << "로비: accept 오류!";
					return;
				}

				if (status != LOBBY) {
					break;
				}
			}
		}
		break;

		case GAME:
		{
			while (true) {

				//cout << "Sleep: " << FRAME_TIME << endl;

				Sleep(FRAME_TIME);
			}
		}
		break;

		case GAME_OVER:
		{

		}
		break;

		case GAME_RESTART:
		{

		}
		break;

		case EXIT: {}
			break;

		default:
			break;
	}

	ForeachInstances([&](GameInstance*& inst) {
		inst->OnUpdate(FRAME_TIME);
	});
}

SOCKET ServerFramework::PlayerConnect(int player) {
	SOCKADDR_IN address;
	int address_length = sizeof(address);

	SOCKET new_client = accept(my_socket, (SOCKADDR*)&address, &address_length);
	if (INVALID_SOCKET == new_client) {
		// 오류
		return new_client;
	}

	HANDLE new_thread = CreateThread(NULL, 0, CommunicateProcess, nullptr, 0, NULL);

	players.emplace_back(new PlayerInfo(new_client, new_thread, player_number_last++));
	client_number++;

	return new_client;
}

void ServerFramework::PlayerDisconnect(int player) {
	auto dit = find_if(players.begin(), players.end(), [player](PlayerInfo* pi) {
		return (pi->index == player);
	});

	if (dit != players.end()) {
		closesocket((*dit)->client_socket);
		CloseHandle((*dit)->client_handle);
		players.erase(dit);

		client_number--;
	}
}

GameInstance::GameInstance()
	: owner(-1)
	, sprite_index(0), box{}
	, dead(false)
	, x(0), y(0), hspeed(0.0), vspeed(0.0) {}

GameInstance::~GameInstance() {}

void GameInstance::OnCreate() {}

void GameInstance::OnDestroy() {}

void GameInstance::OnUpdate(double frame_advance) {
	x += hspeed * frame_advance;
	y += vspeed * frame_advance;
}

void GameInstance::SetSprite(int sprite) {
	sprite_index = sprite;
}

void GameInstance::SetBoundBox(const RECT& mask) {
	CopyRect(&box, &mask);
}

int GameInstance::GetBoundLT() const {
	return x + box.left;
}

int GameInstance::GetBoundTP() const {
	return y + box.top;
}

int GameInstance::GetBoundRT() const {
	return x + box.right;
}

int GameInstance::GetBoundBT() const {
	return y + box.bottom;
}

bool GameInstance::IsCollideWith(RECT& other) {
	return !(other.right <= GetBoundLT()
			 || other.bottom <= GetBoundTP()
			 || GetBoundRT() < other.left
			 || GetBoundBT() < other.top);
}

bool GameInstance::IsCollideWith(GameInstance*& other) {
	return !(other->GetBoundRT() <= GetBoundLT()
			 || other->GetBoundBT() <= GetBoundTP()
			 || GetBoundRT() < other->GetBoundLT()
			 || GetBoundBT() < other->GetBoundTP());
}

PlayerInfo::PlayerInfo(SOCKET sk, HANDLE hd, int id) {
	client_socket = sk;
	client_handle = hd;
	index = id;
}
