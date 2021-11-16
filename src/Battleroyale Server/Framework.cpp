#include "pch.h"
#include "CommonDatas.h"
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
		int cx = static_cast<int>(WORLD_W * 0.5 + lengthdir_x(SPAWN_DISTANCE, dir));
		int cy = static_cast<int>(WORLD_W * 0.5 + lengthdir_y(SPAWN_DISTANCE, dir));

		PLAYER_SPAWN_PLACES[i] = new int[2]{ cx, cy };
	}
}

ServerFramework::~ServerFramework() {
	closesocket(my_socket);

	for (auto player : players) {
		CloseHandle(player->client_handle);
	}
	players.clear();

	CloseHandle(thread_game_process);
	CloseHandle(event_receives);
	CloseHandle(event_game_process);
	CloseHandle(event_send_renders);
}

bool ServerFramework::Initialize() {
	WSADATA wsadata;
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsadata)) {
		// 오류
		return false;
	}

	my_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == my_socket) {
		// 오류
		return false;
	}

	ZeroMemory(&my_address, sizeof(my_address));
	my_address.sin_family = AF_INET;
	my_address.sin_addr.s_addr = htonl(INADDR_ANY);
	my_address.sin_port = htons(COMMON_PORT);

	if (SOCKET_ERROR == bind(my_socket, reinterpret_cast<sockaddr*>(&my_address), sizeof(my_address))) {
		ErrorAbort("bind()");
		return false;
	}

	if (SOCKET_ERROR == listen(my_socket, PLAYERS_NUMBER_MAX + 1)) {
		ErrorAbort("listen()");
		return false;
	}

	//thread_list.push_back(CreateThread(NULL, 0, GameProcess, nullptr, 0, NULL));

	event_game_start = CreateEvent(NULL, FALSE, FALSE, NULL);
	event_receives = CreateEvent(NULL, TRUE, FALSE, NULL);
	event_game_process = CreateEvent(NULL, FALSE, FALSE, NULL);
	event_send_renders = CreateEvent(NULL, FALSE, FALSE, NULL);

	thread_game_starter = CreateThread(NULL, 0, GameInitializeProcess, nullptr, 0, NULL);
	thread_game_process = CreateThread(NULL, 0, GameProcess, nullptr, 0, NULL);

	return true;
}

void ServerFramework::Startup() {
	while (true) {
		switch (status) {
			case LISTEN:
			{
				cout << "S: Listening" << endl;

				while (true) {
					SOCKET new_client = PlayerConnect();
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
				cout << "S: Entering Lobby" << endl;

				while (true) {
					if (status != LOBBY) {
						break;
					}

					SOCKET new_client = PlayerConnect();
					if (INVALID_SOCKET == new_client) {
						cerr << "로비: accept 오류!";
						return;
					}
				}
			}
			break;

			case GAME:
			{
				while (true) {
					ForeachInstances([&](GameInstance*& inst) {
						//inst->OnUpdate(FRAME_TIME);
					});

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

			case EXIT:
			{
				// 종료
			}
				return;

			default:
				break;
		}
	}
}

SOCKET ServerFramework::PlayerConnect() {
	SOCKADDR_IN address;
	int address_length = sizeof(address);

	SOCKET new_socket = accept(my_socket, reinterpret_cast<SOCKADDR*>(&address), &address_length);
	if (INVALID_SOCKET == new_socket) {
		// 오류
		return new_socket;
	}

	// 첫번째 플레이어
	if (client_number == 0) {
		player_captain = player_number_last;
	}

	auto client_info = new PlayerInfo(new_socket, 0, player_number_last++);
	HANDLE new_thread = CreateThread(NULL, 0, CommunicateProcess, (client_info), 0, NULL);
	client_info->client_handle = new_thread;

	cout << "새 플레이어 접속: " << new_socket << endl;
	cout << "현재 플레이어 수: " << client_number << " / " << PLAYERS_NUMBER_MAX << endl;

	players.emplace_back(client_info);

	client_number++;
	SendData(new_socket, PACKETS::SERVER_PLAYER_COUNT
			 , reinterpret_cast<char*>(client_number), sizeof(client_number));

	return new_socket;
}

void ServerFramework::PlayerDisconnect(PlayerInfo*& player) {
	auto dit = find(players.begin(), players.end(), player);

	if (dit != players.end()) {
		auto player = (*dit);

		CloseHandle(player->client_handle);

		auto id = player->index;
		auto character = player->player_character;
		if (character)
			Kill(static_cast<GameInstance*>(character));

		players.erase(dit);
		client_number--;

		// 플레이어 0명 혹은 1명
		if (client_number < 2) {
			switch (status) {
				case LISTEN:
				{
					if (0 == client_number) {
						players.clear();
						instances.clear();
					}
				}
				break;

				case LOBBY:
				{
					SetStatus(LISTEN);
				}
				break;

				case GAME: { /* 여기서 처리 안함 */ } break;
				case GAME_OVER: { /* 여기서 처리 안함 */ } break;
				case GAME_RESTART: { /* 여기서 처리 안함 */ } break;
				case EXIT: { /* 여기서 처리 안함 */ } break;
				default: break;
			}
		}

		// 방장이 나감
		if (player_captain == id) {
			switch (status) {
				case LISTEN:
				{
					if (0 < client_number)
						player_captain = players.at(0)->index;
				}
				break;

				case LOBBY:
				{
					player_captain = players.at(0)->index;
				}
				break;

				case GAME: { /* 여기서 처리 안함 */ } break;
				case GAME_OVER: { /* 여기서 처리 안함 */ } break;
				case GAME_RESTART: { /* 여기서 처리 안함 */ } break;
				case EXIT: { /* 여기서 처리 안함 */ } break;
				default: break;
			}
		}

	}
}

void ServerFramework::SetStatus(SERVER_STATES state) {
	if (status != state) {
		cout << "서버 상태 변경: " << status << " -> " << state << endl;

		status = state;
	}
}

GameInstance::GameInstance()
	: owner(-1), sprite_index(0), box{}, dead(false)
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

void SendData(SOCKET socket, PACKETS type, const char* buffer, int length) {
	int result = send(socket, reinterpret_cast<char*>(&type), sizeof(PACKETS), 0);
	if (SOCKET_ERROR == result) {
		ErrorAbort("send 1");
	}

	if (buffer) {
		result = send(socket, buffer, length, 0);
		if (SOCKET_ERROR == result) {
			ErrorAbort("send 2");
		}
	}
}

void ErrorAbort(std::string msg) {
	LPVOID lpMsgBuf;

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPTSTR>(&lpMsgBuf), 0, nullptr);

	// 프로젝트 설정의 문자 집합 멀티바이트로 변경하여 사용
	MessageBox(nullptr, static_cast<LPCTSTR>(lpMsgBuf), msg.c_str(), MB_ICONERROR);

	LocalFree(lpMsgBuf);
	exit(true);
}

void ErrorDisplay(std::string msg) {
	LPVOID lpMsgBuf;

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPTSTR>(&lpMsgBuf), 0, nullptr);

	std::cout << "[" << msg << "] " << static_cast<LPTSTR>(lpMsgBuf) << std::endl;

	LocalFree(lpMsgBuf);
}
