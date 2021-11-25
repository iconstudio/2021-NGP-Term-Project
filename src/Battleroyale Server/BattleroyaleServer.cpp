#include "pch.h"
#include "CommonDatas.h"
#include "Framework.h"
#include "BattleroyaleServer.h"


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
	auto& player_key_storage = client_info->key_storage;

	while (true) {
		PACKETS packet;
		int data_size = 0;

		/*
		LPDWORD my_size = nullptr;
		WSABUF my_data;
		ZeroMemory(&my_data, sizeof(my_data));
		my_data.len = sizeof(PACKETS);
		my_data.buf = new CHAR[sizeof(PACKETS)];
		DWORD my_flags = MSG_WAITALL;

		int rr = WSARecv(client_socket, &my_data, 1, my_size, &my_flags, NULL, NULL);
		*/

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
				if (player_index == framework.player_captain &&
					packet == PACKETS::CLIENT_GAME_START) {
					if (1 < framework.client_number) {
						framework.CastStartGame(true);
						break;
					}
				} // 다른 메시지는 버린다.

				Sleep(2000);
				framework.CastStartGame(true);
				break;
			} break;

			case GAME:
			{
				// 꾸준한 통신
				while (true) {
					framework.AwaitReceiveEvent(); // event_recieves

					// 만약 핑 메시지가 오면 데이터를 받지 않는다.
					if (packet == PACKETS::CLIENT_KEY_INPUT) {
						data_size = SEND_INPUT_COUNT * sizeof(InputStream);

						result = recv(client_socket, reinterpret_cast<char*>(key_storage)
									  , data_size, MSG_WAITALL);
						if (SOCKET_ERROR == result) {
							framework.PlayerDisconnect(client_info);
							break;
						} else if (0 == result) {
							framework.PlayerDisconnect(client_info);
							break;
						}

						for (int i = 0; i < SEND_INPUT_COUNT; ++i) {
							auto button = key_storage[i];
							auto keycode = button.code;
							auto keystat = button.type; // NONE, PRESS, RELEASE

							switch (keystat) {
								case NONE:
								{

								}
								break;

								case PRESS:
								{

								}
								break;

								case RELEASE:
								{

								}
								break;

							}
							switch (button) {
								case 'W': case 'w':
								{
									framework.QueingPlayerAction(client_info
																 , ACTION_TYPES::SET_HSPEED
																 , -PLAYER_MOVE_SPEED);
									std::cout << player_index << " - w" << std::endl;
								}
								break;

								case 'A': case 'a':
								{
									framework.QueingPlayerAction(client_info
																 , ACTION_TYPES::SET_HSPEED
																 , PLAYER_MOVE_SPEED);
									std::cout << player_index << " - a" << std::endl;
								}
								break;

								case 'S': case 's':
								{
									framework.QueingPlayerAction(client_info
																 , ACTION_TYPES::SET_VSPEED
																 , -PLAYER_MOVE_SPEED);
									std::cout << player_index << " - s" << std::endl;
								}
								break;

								case 'D': case 'd':
								{
									framework.QueingPlayerAction(client_info
																 , ACTION_TYPES::SET_VSPEED
																 , PLAYER_MOVE_SPEED);
									std::cout << player_index << " - d" << std::endl;
								}
								break;

								case VK_SPACE:
								{
									auto cc = reinterpret_cast<CCharacter*>(client_info->player_character);
									if (cc) {
										framework.QueingPlayerAction(client_info
																	 , ACTION_TYPES::SHOOT
																	 , static_cast<int>(cc->image_angle));
									}
									std::cout << player_index << " - space" << std::endl;
								}
								break;
							}
						}
					} // 다른 메시지는 버린다.

					framework.CastProcessingGame();

					framework.AwaitSendRendersEvent(); // event_send_renders
					framework.SendRenderings();

					framework.CastSendRenders(false);

					framework.CastStartReceive(true);
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

			case EXIT:
			{

			}
			break;

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

		shuffle(framework.players.begin(), framework.players.end(), server_randomizer);

		auto sz = framework.players.size();
		for (int i = 0; i < sz; ++i) {
			auto player = framework.players.at(i);
			auto places = framework.PLAYER_SPAWN_PLACES[i];
			auto character = framework.Instantiate<CCharacter>(places[0], places[1]);

			player->player_character = character;
			character->owner = player->index;
		}

		framework.CastStartReceive(true);
		framework.SetStatus(GAME);
	}

	return 0;
}

/*
	TODO: I/O Overlapped 모델로 변경하기

	왜냐하면 게임의 지연없이 한번에 여러 클라이언트를 처리하기 위해서는 동시 실행이 필수적이다.
	IOCP 말고 이 부분에만 Overlapped 모델을 사용하면 좋을 것 같다.
*/
DWORD WINAPI GameProcess(LPVOID arg) {
	while (true) {
		framework.AwaitProcessingGameEvent(); // event_game_process

		framework.CastStartReceive(false);
		Sleep(LERP_MIN); // 이 함수를 SleepEx로

		if (1 < framework.GetClientCount()) {
			// 게임 처리
			framework.ProceedContinuation();
		} else {
			// 게임 판정승 혹은 게임 강제 종료
		}
	}

	return 0;
}

DWORD WINAPI ConnectProcess(LPVOID arg) {
	while (true) {
		framework.AwaitClientAcceptEvent();

		SOCKET new_client = framework.PlayerConnect();
		if (INVALID_SOCKET == new_client) {
			cerr << "accept 오류!";
			return 0;
		}
	}

	return 0;
}

CCharacter::CCharacter()
	: GameInstance(), update_info{}
	, attack_cooltime(0.0), inv_time(0.0)
	, health(PLAYER_HEALTH) {
	SetRenderType(RENDER_TYPES::CHARACTER);
	SetBoundBox(RECT{ -6, -6, 6, 6 });
}

void CCharacter::OnUpdate(double frame_advance) {
	auto collide_bullet = framework.SeekCollision<CBullet>(this, "Bullet");

	if (collide_bullet) {
		GetHurt(1);
		framework.Kill(collide_bullet);
		cout << "플레이어 " << owner << "의 총알 충돌" << endl;
	}

	direction = point_direction(0, 0, hspeed, vspeed);

	//UpdateMessage(owner, framework.GetClientCount(), x, y, health, image_angle);

	GameInstance::OnUpdate(frame_advance);
}

const char* CCharacter::GetIdentifier() const { return "Player"; }

void CCharacter::UpdateMessage(int count) {
	update_info.target_player = owner;
	update_info.player_x = x;
	update_info.player_y = y;
	update_info.player_direction = direction;
	update_info.player_hp = health;
	update_info.players_count = count;
}

void CCharacter::GetHurt(int dmg) {
	if (inv_time <= 0) {
		health -= dmg;
		if (health <= 0) {
			cout << "플레이어 " << owner << " 사망." << endl;
			Die();
		} else {
			inv_time = PLAYER_INVINCIBLE_DURATION;
		}
	}
}

void CCharacter::Die() {
	framework.Kill(this);
}

CBullet::CBullet()
	: GameInstance(), lifetime(SNOWBALL_DURATION) {
	SetRenderType(RENDER_TYPES::BULLET);
	SetBoundBox(RECT{ -2, -2, 2, 2 });
}

void CBullet::OnUpdate(double frame_advance) {
	lifetime -= frame_advance;
	if (lifetime <= 0) {
		framework.Kill(this);
		return;
	}

	image_angle = point_direction(0, 0, hspeed, vspeed);
	GameInstance::OnUpdate(frame_advance);
}

const char* CBullet::GetIdentifier() const { return "Bullet"; }
