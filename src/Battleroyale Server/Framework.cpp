#include "pch.h"
#include "Framework.h"

// 소켓 함수 오류 출력 후 종료
void ErrorQuit(std::string msg)
{
	LPVOID lpMsgBuf;

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPTSTR>(&lpMsgBuf), 0, nullptr);

	// 프로젝트 설정의 문자 집합 멀티바이트로 변경하여 사용
	MessageBox(nullptr, static_cast<LPCTSTR>(lpMsgBuf), msg.c_str(), MB_ICONERROR);

	LocalFree(lpMsgBuf);
	exit(true);
}

// 소켓 함수 오류 출력
void DisplayError(std::string msg)
{
	LPVOID lpMsgBuf;

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPTSTR>(&lpMsgBuf), 0, nullptr);

	std::cout << "[" << msg << "] " << static_cast<char*>(lpMsgBuf) << std::endl;

	LocalFree(lpMsgBuf);
}

ServerFramework::ServerFramework(int rw, int rh)
	: WORLD_W(rw), WORLD_H(rh), SPAWN_DISTANCE(rh * 0.4)
	, status(SERVER_STATES::LISTEN)
	, client_number(0), player_captain(-1) {

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

	for (HANDLE player : player_handles) {
		CloseHandle(player);
	}

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
	my_address.sin_port = htons(SERVERPORT);		// 서버 포트 추가 필요

	if (SOCKET_ERROR == bind(my_socket, reinterpret_cast<sockaddr*>(&my_address), sizeof(my_address))) {
		// 오류
		ErrorQuit("bind()");
		return false;
	}

	if (SOCKET_ERROR == listen(my_socket, PLAYERS_NUMBER_MAX + 1)) {
		// 오류
		ErrorQuit("listen()");
		return false;
	}

	return true;
}

void ServerFramework::Update() {
	switch (status) {
		case LISTEN:
		{
			SOCKADDR_IN address;
			int address_length = sizeof(address);

			int result = accept(my_socket, reinterpret_cast<SOCKADDR*>(&address), &address_length);
			if (SOCKET_ERROR == result) {
				// 오류
				cerr << "accept 오류!";
				return;
			}


		}
		break;

		case LOBBY:
		{

		}
		break;

		case GAME:
		{

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
