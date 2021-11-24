#include "BattleroyaleServer.h"
#include "CommonDatas.h"
#include "Framework.h"
#include "pch.h"

ServerFramework framework{ GAME_SCENE_W, GAME_SCENE_H };

normal_distribution<> server_distrubution;
default_random_engine server_randomizer{ 0 };

int main() {
	cout << "Hello World!\n";

	if (!framework.Initialize()) {
		WSACleanup();
		return 0;
	}

	framework.Instantiate<CCharacter>(40, 40);
	auto a = framework.Instantiate<CBullet>(40, 40);
	auto b = framework.Instantiate<CBullet>(40, 40);
	auto c = framework.Instantiate<CBullet>(140, 40);
	framework.GameUpdate();
	framework.Startup();

	WSACleanup();
	return 0;
}

DWORD WINAPI CommunicateProcess(LPVOID arg) {
	PlayerInfo* client_info = reinterpret_cast<PlayerInfo*>(arg);
	SOCKET client_socket = client_info->client_socket;
	int player_index = client_info->index;

	while (true) {
		PACKETS packet;
		int data_size = 0;
		char* data = nullptr;

		/*
		LPDWORD my_size = nullptr;
		WSABUF my_data;
		ZeroMemory(&my_data, sizeof(my_data));
		my_data.len = sizeof(PACKETS);
		my_data.buf = new CHAR[sizeof(PACKETS)];
		DWORD my_flags = MSG_WAITALL;

		int rr = WSARecv(client_socket, &my_data, 1, my_size, &my_flags, NULL, NULL);
		*/

		int result = recv(client_socket, reinterpret_cast<char*>(&packet),
						  sizeof(PACKETS), MSG_WAITALL);

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
				if (player_index == framework.player_captain &&
					packet == PACKETS::CLIENT_GAME_START) {
					if (1 < framework.client_number) {
						framework.CastStartGame(true);
						break;
					}
				} // 다른 메시지는 버린다.
			} break;

			case GAME:
			{
				// 꾸준한 통신
				while (true) {
					framework.AwaitReceiveEvent(); // event_recieves

					// 만약 핑 메시지가 오면 데이터를 받지 않는다.
					if (0 < data_size) {
						result = recv(client_socket, data, 1, MSG_WAITALL);
						// result = recv(client_socket, data, data_size, MSG_WAITALL);
					}

					if (data && packet == PACKETS::CLIENT_KEY_INPUT) {
						char button = (*data);

						if (data) {
							switch (*data) {
								case 'W':
								case 'w':
								{
									framework.QueingPlayerAction(client_info
																 , ACTION_TYPES::SET_HSPEED
																 , -PLAYER_MOVE_SPEED);
									std::cout << player_index << " - w" << std::endl;
								} break;

								case 'A':
								case 'a':
								{
									framework.QueingPlayerAction(client_info
																 , ACTION_TYPES::SET_HSPEED
																 , PLAYER_MOVE_SPEED);
									std::cout << player_index << " - a" << std::endl;
								} break;

								case 'S':
								case 's':
								{
									framework.QueingPlayerAction(client_info
																 , ACTION_TYPES::SET_VSPEED
																 , -PLAYER_MOVE_SPEED);
									std::cout << player_index << " - s" << std::endl;
								} break;

								case 'D':
								case 'd':
								{
									framework.QueingPlayerAction(client_info
																 , ACTION_TYPES::SET_VSPEED
																 , PLAYER_MOVE_SPEED);
									std::cout << player_index << " - d" << std::endl;
								} break;

								case VK_SPACE:
								{
									std::cout << player_index << " - space" << std::endl;
								} break;
							}
						}
					} // 다른 메시지는 버린다.

					/*
									TODO: I/O Overlapeed 모델로 바꾸기 위해서는 APC 함수들이
					   필수적이라고 한다. 운영체제의 메시지 큐를 사용하는 함수가 있다.
					*/
					framework.CastProcessingGame();

					framework.AwaitSendRendersEvent(); // event_send_renders
					framework.SendRenderings();

					framework.CastStartReceive(true);
				}
			} break;

			case GAME_OVER:
			{
				if (packet == PACKETS::CLIENT_PLAY_CONTINUE) {

				} else if (packet == PACKETS::CLIENT_PLAY_DENY) {
				}
			} break;

			case GAME_RESTART:
			{

			} break;

			case EXIT:
			{
			} break;

			default:
				break;
		}
	}

	closesocket(client_socket);
	return 0;
}

DWORD WINAPI GameInitializeProcess(LPVOID arg) {
	while (true) {
		framework.AwaitStartGameEvent();

		shuffle(framework.players.begin(), framework.players.end(),
				server_randomizer);

		auto sz = framework.players.size();
		for (int i = 0; i < sz; ++i) {
			auto player = framework.players.at(i);
			auto places = framework.PLAYER_SPAWN_PLACES[i];
			player->player_character =
				framework.Instantiate<CCharacter>(places[0], places[1]);
			static_cast<CCharacter*>(player->player_character)->index =
				player->index;
			// 캐릭터 클래스에 캐릭터의 번호 부여
		}

		framework.CastStartReceive(true);
		framework.SetStatus(GAME);
	}

	return 0;
}

/*
				TODO: I/O Overlapped 모델로 변경하기

				왜냐하면 게임의 지연없이 한번에 여러 클라이언트를 처리하기
   위해서는 동시 실행이 필수적이다. IOCP 말고 이 부분에만 Overlapped 모델을
   사용하면 좋을 것 같다.
*/
DWORD WINAPI GameProcess(LPVOID arg) {
	while (true) {
		framework.AwaitProcessingGameEvent(); // 이 함수를 WaitForSingleObjectEx로,
											  // evemt_game_process
		framework.CastStartReceive(false);
		Sleep(LERP_MIN); // 이 함수를 SleepEx로

		if (1 < framework.GetClientCount()) {
		  // 게임 처리
			framework.InterpretPlayerAction();

			framework.ProceedReceiveIndex();
		} else { // 게임 판정승 혹은 게임 강제 종료
		}
	}

	return 0;
}

DWORD WINAPI ConnectProcess(LPVOID arg) {
	while (true) {
		framework.AwaitClientAcceptEvent(); // event_game_process

		SOCKET new_client = framework.PlayerConnect();
		if (INVALID_SOCKET == new_client) {
			cerr << "accept 오류!";
			return 0;
		}
	}

	return 0;
}

CCharacter::CCharacter()
	: GameInstance(), attack_cooltime(0.0),
	health(PLAYER_HEALTH), update_info{} {
	SetSprite(0);
	SetBoundBox(RECT{ -6, -6, 6, 6 });
}

void CCharacter::OnUpdate(double frame_advance) {
	CBullet* collide_bullet =
		framework.SeekCollision<CBullet>(this, "Bullet");
	if (collide_bullet) {
		health--;
		framework.Kill(collide_bullet);
		cout << "플레이어 " << owner << "의 총알 충돌" << endl;
	}

	UpdateMessage(index, framework.GetClientCount(), x, y, health, direction);

	GameInstance::OnUpdate(frame_advance);
}

const char* CCharacter::GetIdentifier() const { return "Player"; }

void CCharacter::UpdateMessage(int index, int count, double x, double y, int hp,
							   double direction) {
	update_info.player_x = x;
	update_info.player_y = y;
	update_info.player_direction = direction;
	update_info.player_hp = hp;
	update_info.target_player = index;
	update_info.players_count = count;
}

CBullet::CBullet() : GameInstance(), lifetime(SNOWBALL_DURATION) {
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

const char* CBullet::GetIdentifier() const { return "Bullet"; }
