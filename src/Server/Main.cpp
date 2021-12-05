#include "stdafx.h"
#include "Framework.h"
#include "CommonDatas.h"
#include "ServerFramework.h"
#include "Main.h"


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

	players_number = 0;
	players.reserve(CLIENT_NUMBER_MAX);

	PLAYER_SPAWN_PLACES = new int* [CLIENT_NUMBER_MAX];

	double dir_increment = (360.0 / CLIENT_NUMBER_MAX);
	for (int i = 0; i < CLIENT_NUMBER_MAX; ++i) {
		double dir = dir_increment * i;
		int cx = static_cast<int>(WORLD_W * 0.5 + lengthdir_x(SPAWN_DISTANCE, dir));
		int cy = static_cast<int>(WORLD_W * 0.5 + lengthdir_y(SPAWN_DISTANCE, dir));

		PLAYER_SPAWN_PLACES[i] = new int[2]{ cx, cy };
	}

	if (NULL == (event_accept = CreateEvent(NULL, FALSE, TRUE, NULL))) {
		ErrorAbort("CreateEvent[event_accept]");
		return 1;
	}

	if (NULL == (event_game_communicate = CreateEvent(NULL, FALSE, TRUE, NULL))) {
		ErrorAbort("CreateEvent[event_game_communicate]");
		return 1;
	}

	if (NULL == (event_quit = CreateEvent(NULL, FALSE, FALSE, NULL))) {
		ErrorAbort("CreateEvent[event_quit]");
		return 1;
	}

	// 클라이언트 연결
	if (!CreateThread(NULL, 0, ConnectProcess, nullptr, 0, NULL)) {
		ErrorAbort("CreateThread[ConnectProcess]");
	}

	Sleep(8000);
	CreatePlayerCharacters();
	SetEvent(event_game_communicate);
	
	// 서버 대기
	WaitForSingleObject(event_quit, INFINITE);

	CloseHandle(event_accept);
	CloseHandle(event_game_communicate);
	CloseHandle(event_quit);

	closesocket(my_socket);

	return 0;
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

		BOOL option = FALSE;
		setsockopt(my_socket, IPPROTO_TCP, TCP_NODELAY
			, reinterpret_cast<const char*>(&option), sizeof(option));

		auto client = new ClientSession(client_socket, NULL, players_number++);

		auto th = CreateThread(NULL, 0, GameProcess, (LPVOID)(client), 0, NULL);
		if (NULL == th) {
			ErrorDisplay("CreateThread[GameProcess]");
			continue;
		}
		CloseHandle(th);

		players.push_back(client);

		AtomicPrintLn("클라이언트 접속: ", client_socket, ", 수: ", players_number);

		WaitForSingleObject(event_accept, INFINITE);
	}

	return 0;
}

DWORD WINAPI GameProcess(LPVOID arg) {
	ClientSession* client = reinterpret_cast<ClientSession*>(arg);
	SOCKET client_socket = client->my_socket;

	while (true) {
		WaitForSingleObject(event_game_communicate, INFINITE);

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
		SetEvent(event_game_communicate);
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

CCharacter::CCharacter()
	: GameInstance()
	, attack_cooltime(0.0), inv_time(0.0)
	, health(PLAYER_HEALTH) {
	SetRenderType(RENDER_TYPES::CHARACTER);
	SetBoundBox(RECT{ -6, -6, 6, 6 });
}

void CCharacter::OnUpdate(double frame_advance) {
	auto collide_bullet = SeekCollision<CBullet>(this, "Bullet");

	if (collide_bullet) {
		Kill(collide_bullet);
		cout << "플레이어 " << owner << "의 총알 충돌" << endl;

		GetHurt(1);
	}

	if (hspeed != 0.0 || vspeed != 0.0)
		direction = point_direction(0, 0, hspeed, vspeed);

	AssignRenderingInfo(direction);

	GameInstance::OnUpdate(frame_advance);
}

const char* CCharacter::GetIdentifier() const { return "Player"; }

void CCharacter::GetHurt(int dmg) {
	if (inv_time <= 0) {
		health -= dmg;
		if (health <= 0) {
			cout << "플레이어 " << owner << " 사망." << endl;
			Die();
		}
		else {
			inv_time = PLAYER_INVINCIBLE_DURATION;
		}
	}
	else {
		inv_time -= FRAME_TIME;
	}
}

void CCharacter::Die() {
	dead = true;
	Kill(this);
}

CBullet::CBullet()
	: GameInstance(), lifetime(SNOWBALL_DURATION) {
	SetRenderType(RENDER_TYPES::BULLET);
	SetBoundBox(RECT{ -2, -2, 2, 2 });
}

void CBullet::OnUpdate(double frame_advance) {
	lifetime -= frame_advance;
	if (lifetime <= 0) {
		Kill(this);
		return;
	}

	image_angle = point_direction(0, 0, hspeed, vspeed);
	AssignRenderingInfo(image_angle);

	GameInstance::OnUpdate(frame_advance);
}

const char* CBullet::GetIdentifier() const { return "Bullet"; }

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
