#include "pch.h"
#include "CommonDatas.h"
#include "Framework.h"


ServerFramework::ServerFramework(int rw, int rh)
	: WORLD_W(rw), WORLD_H(rh), SPAWN_DISTANCE(rh * 0.4)
	, status(SERVER_STATES::LISTEN)
	, my_socket(0), my_address(), client_number(0)
	, player_number_last(0), player_captain(-1) {

	players.reserve(PLAYERS_NUMBER_MAX);

	PLAYER_SPAWN_PLACES = new int*[PLAYERS_NUMBER_MAX];

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

	for (auto& player : players) {
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
		ErrorQuit("bind()");
		return false;
	}

	if (SOCKET_ERROR == listen(my_socket, PLAYERS_NUMBER_MAX + 1)) {
		ErrorQuit("listen()");
		return false;
	}

	thread_game_process = CreateThread(NULL, 0, GameProcess, nullptr, 0, NULL);

	return true;
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
				SOCKET new_client = PlayerConnect(player_number_last);
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
				ForeachInstances([&](GameInstance*& inst) {
					inst->OnUpdate(FRAME_TIME);
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

		case EXIT: {}
				 break;

		default:
			break;
	}
}

SOCKET ServerFramework::PlayerConnect(int player) {
	SOCKADDR_IN address;
	int address_length = sizeof(address);

	SOCKET new_client = accept(my_socket, reinterpret_cast<SOCKADDR*>(&address), &address_length);
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

void ServerFramework::SetStatus(SERVER_STATES state) {
	if (status != state) {
		cout << "서버 상태 변경: " << status << " -> " << state << endl;

		status = state;
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

void ErrorQuit(std::string msg) {
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

	std::cout << "[" << msg << "] " << static_cast<char*>(lpMsgBuf) << std::endl;

	LocalFree(lpMsgBuf);
}