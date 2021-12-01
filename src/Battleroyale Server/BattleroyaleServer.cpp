#include "pch.h"
#include "CommonDatas.h"
#include "Framework.h"
#include "BattleroyaleServer.h"


/*
	TODO: I/O Overlapped 모델로 변경하기

	왜냐하면 게임의 지연없이 한번에 여러 클라이언트를 처리하기 위해서는 동시 실행이 필수적이다.
	IOCP 말고 이 부분에만 Overlapped 모델을 사용하면 좋을 것 같다.
*/
ServerFramework framework{ GAME_SCENE_W, GAME_SCENE_H };

int main() {
	cout << "Hello World!\n";

	if (!framework.Initialize()) {
		WSACleanup();
		return 0;
	}

	framework.Startup();

	WSACleanup();
	return 0;
}

DWORD WINAPI CommunicateProcess(LPVOID arg) {
	PlayerInfo* client_info = reinterpret_cast<PlayerInfo*>(arg);
	SOCKET client_socket = client_info->client_socket;
	int player_index = client_info->player_index;

	bool thread_done = false;
	while (!thread_done) {
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
		if (!framework.ValidateSocketMessage(result)) {
			framework.PlayerDisconnect(client_info);
			break;
		}

		switch (framework.GetStatus()) {
			case LOBBY:
			{
				// 방장의 게임 시작 메시지
				if (packet == PACKETS::CLIENT_GAME_START) {
					if (framework.CheckClientNumber()) {
						//Sleep(100);
						framework.CastStartGame(true);
						break;
					}
				} // 다른 메시지는 버린다.

				Sleep(3000);
				//framework.CastStartGame(true);
				break;
			} break;

			case GAME:
			{
				// 꾸준한 통신
				while (true) {
					auto input_state = framework.AwaitReceiveEvent(); // event_recieves
					if (WAIT_TIMEOUT == input_state) {
						break;
					}

					// 만약 핑 메시지가 오면 데이터를 받지 않는다.
					if (packet == PACKETS::CLIENT_KEY_INPUT) {
						auto key_storage = new InputStream[SEND_INPUT_COUNT];
						data_size = SEND_INPUT_COUNT * sizeof(InputStream);

						result = recv(client_socket, reinterpret_cast<char*>(key_storage)
									  , data_size, MSG_WAITALL);
						if (!framework.ValidateSocketMessage(result)) {
							framework.PlayerDisconnect(client_info);
							break;
						}

						bool check_lt = false;
						bool check_rt = false;
						bool check_up = false;
						bool check_dw = false;
						bool check_shoot = false;
						bool check_blink = false;
						bool check_reload = false;
						for (int i = 0; i < SEND_INPUT_COUNT; ++i) {
							auto button = key_storage[i];
							auto keycode = button.code;
							auto keystat = button.type;

							switch (keystat) {
								case NONE:
								{
									switch (keycode) {
										case VK_LEFT: { check_lt = false; } break;
										case VK_RIGHT: { check_rt = false; } break;
										case VK_UP: { check_up = false; } break;
										case VK_DOWN: { check_dw = false; } break;
										case VK_SPACE: { check_blink = false; }	 break; // 특능
										case 'A': case 'a': { check_shoot = false; } break; // 공격
										case 'R': case 'r': { check_reload = false; } break; // 재장전
										default: break;
									}
								}
								break;

								case PRESS:
								{
									switch (keycode) {
										case VK_LEFT: { check_lt = true; } break;
										case VK_RIGHT: { check_rt = true; } break;
										case VK_UP: { check_up = true; } break;
										case VK_DOWN: { check_dw = true; } break;
										case VK_SPACE: { check_blink = true; }	 break; // 특능
										case 'A': case 'a': { check_shoot = true; } break; // 공격
										case 'R': case 'r': { check_reload = true; } break; // 재장전
										default: break;
									}
								}
								break;

								case RELEASE:
								{
									switch (keycode) {
										case VK_LEFT: { check_lt = false; } break;
										case VK_RIGHT: { check_rt = false; } break;
										case VK_UP: { check_up = false; } break;
										case VK_DOWN: { check_dw = false; } break;
										case VK_SPACE: { check_blink = false; }	 break; // 특능
										case 'A': case 'a': { check_shoot = false; } break; // 공격
										case 'R': case 'r': { check_reload = false; } break; // 재장전
										default: break;
									}
								}
								break;
							}
						}

						auto pchar = reinterpret_cast<CCharacter*>(client_info->player_character);
						if (pchar && !pchar->dead) { // 게임 상태
							int check_horz = check_rt - check_lt; // 좌우 이동
							int check_vert = check_dw - check_up; // 상하 이동

							if (0 != check_horz) {
								pchar->x += FRAME_TIME * PLAYER_MOVE_SPEED * check_horz;
							}

							if (0 != check_vert) {
								pchar->y += FRAME_TIME * PLAYER_MOVE_SPEED * check_vert;
							}

							if (check_blink) {
								//TODO
							}

							if (check_shoot) {
								auto bullet = framework.Instantiate<CBullet>(pchar->x, pchar->y);
								bullet->SetVelocity(SNOWBALL_SPEED, pchar->direction);
								bullet->SetOwner(player_index);
							}

							if (check_reload) {

							}
						} else if (pchar && pchar->dead) { // 관전 상태

						}
					}
				} // 다른 메시지는 버린다.

				framework.CastProcessingGame();

				framework.AwaitSendRendersEvent(); // event_send_renders
				framework.SendRenderingInfos(client_socket);

				framework.CastSendingRenderingInfos(false);

				SleepEx(FRAME_TIME, TRUE);
				framework.CastStartReceive(true);
			}
			break;

			case GAME_OVER:
			{
				if (packet == PACKETS::CLIENT_PLAY_CONTINUE) { //TODO

				} else if (packet == PACKETS::CLIENT_PLAY_DENY) {
					framework.PlayerDisconnect(client_info);
					thread_done = true;
				}
			}
			break;

			case GAME_RESTART:
			{
				//TODO
			}
			break;

			case EXIT:
			{
				thread_done = true;
			}
			break;

			default:
				break;
		}
	}

	closesocket(client_socket);
	return 0;
}

DWORD WINAPI ConnectProcess(LPVOID arg) {
	while (true) {
		framework.ProcessConnect();
	}

	return 0;
}

DWORD WINAPI GameReadyProcess(LPVOID arg) {
	while (true) {
		framework.ProcessReady();
		framework.CreatePlayerCharacters<CCharacter>();
	}

	return 0;
}

DWORD WINAPI GameProcess(LPVOID arg) {
	while (true) {
		framework.ProcessGame();
	}

	return 0;
}

CCharacter::CCharacter()
	: GameInstance()
	, attack_cooltime(0.0), inv_time(0.0)
	, health(PLAYER_HEALTH) {
	SetRenderType(RENDER_TYPES::CHARACTER);
	SetBoundBox(RECT{ -6, -6, 6, 6 });
}

void CCharacter::OnUpdate(double frame_advance) {
	auto collide_bullet = framework.SeekCollision<CBullet>(this, "Bullet");

	if (collide_bullet) {
		framework.Kill(collide_bullet);
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
		} else {
			inv_time = PLAYER_INVINCIBLE_DURATION;
		}
	} else {
		inv_time -= FRAME_TIME;
	}
}

void CCharacter::Die() {
	dead = true;
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
	AssignRenderingInfo(image_angle);

	GameInstance::OnUpdate(frame_advance);
}

const char* CBullet::GetIdentifier() const { return "Bullet"; }
