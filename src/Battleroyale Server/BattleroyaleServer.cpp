#include "BattleroyaleServer.h"
#include "pch.h"
#include "CommonDatas.h"
#include "Framework.h"


ServerFramework framework{ GAME_SCENE_W, GAME_SCENE_H };

normal_distribution<> server_distrubution;
default_random_engine server_randomizer{0};

const int PLAYER_HEALTH = 10;
const double PLAYER_MOVE_SPEED = km_per_hr(20);
const double PLAYER_ATTACK_COOLDOWN = 0.2;
const double SNOWBALL_DURATION = 0.6;
const double SNOWBALL_VELOCITY = km_per_hr(50);

int main() {
	cout << "Hello World!\n";

	if (!framework.Initialize())
	{
		WSACleanup();
		return 0;
	}

	framework.Startup();

	WSACleanup();
}

DWORD WINAPI CommunicateProcess(LPVOID arg) {
	PlayerInfo* client_info = reinterpret_cast<PlayerInfo*>(arg);
	SOCKET client_socket = client_info->client_socket;
	int player_index = client_info->index;

	while (true) {
		PACKETS packet;
		int data_size = 0;
		char* data = nullptr;

		int result = recv(client_socket, reinterpret_cast<char*>(&packet), sizeof(PACKETS), MSG_WAITALL);

		if (SOCKET_ERROR == result) {
			framework.PlayerDisconnect(client_info);
			break;
		} else if (0 == result) {
			framework.PlayerDisconnect(client_info);
			break;
		}

		switch (framework.GetStatus()) {
			case LOBBY:
			{
				// 방장의 게임 시작 메시지

				// 게임 초기화
				if (player_index == framework.player_captain
					&& packet == PACKETS::CLIENT_GAME_START) {
					if (1 < framework.client_number) {
						SetEvent(framework.event_game_start);
						break;
					}
				} // 다른 메시지는 버린다.
			}
			break;

			case GAME:
			{
				// 꾸준한 통신
				while (true) {
					WaitForSingleObject(framework.event_receives, INFINITE);

					// 만약 핑 메시지가 오면 데이터를 받지 않는다.
					if (0 < data_size) {
						result = recv(client_socket, data, 1, MSG_WAITALL);
						//result = recv(client_socket, data, data_size, MSG_WAITALL);
					}

					// 게임 초기화
					if (packet == PACKETS::CLIENT_KEY_INPUT) {
						auto character = reinterpret_cast<CCharacter*>(client_info->player_character);

						if (data) {
							switch (*data) {
								case 'W':
								{
									character->x -= PLAYER_MOVE_SPEED * FRAME_TIME;
								}
								break;

								case 'A':
								{
									character->y -= PLAYER_MOVE_SPEED * FRAME_TIME;
								}
								break;

								case 'S':
								{
									character->x += PLAYER_MOVE_SPEED * FRAME_TIME;
								}
								break;

								case 'D':
								{
									character->y += PLAYER_MOVE_SPEED * FRAME_TIME;
								}
								break;

								case VK_SPACE:
								{

								}
								break;

							}
						}
					} // 다른 메시지는 버린다.

					SetEvent(framework.event_game_process);

					WaitForSingleObject(framework.event_send_renders, FRAME_TIME);


				}
			}
			break;

			case GAME_OVER:
			{
				if (packet == PACKETS::CLIENT_PLAY_CONTINUE) {

				} else if (packet == PACKETS::CLIENT_PLAY_DENY) {

				}
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

	closesocket(client_socket);
	return 0;
}

DWORD __stdcall GameInitializeProcess(LPVOID arg) {
	while (true) {
		WaitForSingleObject(framework.event_game_start, INFINITE);

		shuffle(framework.players.begin(), framework.players.end(), server_randomizer);

		auto sz = framework.players.size();
		for (int i = 0; i < sz; ++i) {
			auto player = framework.players.at(i);
			auto places = framework.PLAYER_SPAWN_PLACES[i];
			player->player_character = framework.Instantiate<CCharacter>(places[0], places[1]);
		}

		SetEvent(framework.event_receives);
		framework.SetStatus(GAME);
	}

	return 0;
}

DWORD WINAPI GameProcess(LPVOID arg) {
	CCharacter* player_character;

	while (true) {
		WaitForSingleObject(framework.event_game_process, INFINITE);

		ResetEvent(framework.event_receives);

		if (1 < framework.client_number) {

			SetEvent(framework.event_send_renders);
			break;
		} else { // 게임 판정승 혹은 게임 강제 종료

		}
	}

	return 0;
}

DWORD WINAPI ConnectProcess(LPVOID arg) {
	while (true) {
		WaitForSingleObject(framework.event_player_accept, INFINITE);

		SOCKET new_client = framework.PlayerConnect();
		if (INVALID_SOCKET == new_client) {
			cerr << "accept 오류!";
			return 0;
		}
	}

	return 0;
}

CCharacter::CCharacter()
	: attack_cooltime(0.0), health(PLAYER_HEALTH) {
	SetSprite(0);
	SetBoundBox(RECT{ -6, -6, 6, 6 });
}

void CCharacter::OnUpdate(double frame_advance) {
	GameInstance::OnUpdate(frame_advance);
}

CBullet::CBullet()
	: lifetime(SNOWBALL_DURATION) {
	SetSprite(1);
	SetBoundBox(RECT{ -2, -2, 2, 2 });
}

void CBullet::OnUpdate(double frame_advance) {
	lifetime -= frame_advance;
	if (lifetime <= 0) {
		framework.Kill(this);
	}

	GameInstance::OnUpdate(frame_advance);
}
