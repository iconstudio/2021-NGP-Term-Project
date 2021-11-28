#include "pch.h"
#include "CommonDatas.h"
#include "Framework.h"
#include "BattleroyaleServer.h"


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
	int player_index = client_info->index;

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
		if (!framework.ValidateSocketMessage(result)) {
			framework.PlayerDisconnect(client_info);
			break;
		}

		switch (framework.GetStatus()) {
			case LOBBY:
			{
				// ������ ���� ���� �޽���
				if (packet == PACKETS::CLIENT_GAME_START) {
					if (framework.CheckClientNumber()) {
						framework.CastStartGame(true);
						break;
					}
				} // �ٸ� �޽����� ������.

				Sleep(5000);
				framework.CastStartGame(true);
				break;
			} break;

			case GAME:
			{
				// ������ ���
				while (true) {
					framework.AwaitReceiveEvent(); // event_recieves

					// ���� �� �޽����� ���� �����͸� ���� �ʴ´�.
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
						for (int i = 0; i < SEND_INPUT_COUNT; ++i) {
							auto button = key_storage[i];
							auto keycode = button.code;
							auto keystat = button.type;

							switch (keystat) {
								case NONE:
								{
									if (keycode == VK_LEFT) {
										check_lt = false;
									} else if (keycode == VK_RIGHT) {
										check_rt = false;
									} else if (keycode == VK_UP) {
										check_up = false;
									} else if (keycode == VK_DOWN) {
										check_dw = false;
									} else if (keycode == VK_SPACE) { // Ư��
										check_blink = false;
									} else if (keycode == 'A') { // ����
										check_shoot = false;
									}
								}
								break;

								case PRESS:
								{
									if (keycode == VK_LEFT) {
										check_lt = true;
									} else if (keycode == VK_RIGHT) {
										check_rt = true;
									} else if (keycode == VK_UP) {
										check_up = true;
									} else if (keycode == VK_DOWN) {
										check_dw = true;
									} else if (keycode == VK_SPACE) { // Ư��
										check_blink = true;
									} else if (keycode == 'A') { // ����
										check_shoot = true;
									}
								}
								break;

								case RELEASE:
								{
									if (keycode == VK_LEFT) {
										check_lt = false;
									} else if (keycode == VK_RIGHT) {
										check_rt = false;
									} else if (keycode == VK_UP) {
										check_up = false;
									} else if (keycode == VK_DOWN) {
										check_dw = false;
									} else if (keycode == VK_SPACE) { // Ư��
										check_blink = false;
									} else if (keycode == 'A') { // ����
										check_shoot = false;
									}
								}
								break;
							}
						}

						auto pchar = reinterpret_cast<CCharacter*>(client_info->player_character);
						if (pchar && !pchar->dead) { // ���� ����
							int check_horz = check_rt - check_lt; // �¿� �̵�
							int check_vert = check_dw - check_up; // ���� �̵�

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
								//TODO
							}
						} else if (pchar && pchar->dead) { // ���� ����
						}
					} // �ٸ� �޽����� ������.

					framework.CastProcessingGame();

					framework.AwaitSendRendersEvent(); // event_send_renders
					framework.SendRenderingInfos(client_socket);

					framework.CastSendingRenderingInfos(false);

					SleepEx(FRAME_TIME, TRUE);
					framework.CastStartReceive(true);
				}
			}
			break;

			case GAME_OVER:
			{
				if (packet == PACKETS::CLIENT_PLAY_CONTINUE) { //TODO

				} else if (packet == PACKETS::CLIENT_PLAY_DENY) {

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
				//TODO
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

		framework.GameReady();
		framework.CreatePlayerCharacters<CCharacter>();

		framework.CastStartReceive(true);
		framework.SetStatus(GAME);
	}

	return 0;
}

/*
	TODO: I/O Overlapped �𵨷� �����ϱ�

	�ֳ��ϸ� ������ �������� �ѹ��� ���� Ŭ���̾�Ʈ�� ó���ϱ� ���ؼ��� ���� ������ �ʼ����̴�.
	IOCP ���� �� �κп��� Overlapped ���� ����ϸ� ���� �� ����.
*/
DWORD WINAPI GameProcess(LPVOID arg) {
	while (true) {
		framework.AwaitProcessingGameEvent(); // event_game_process

		framework.CastStartReceive(false);
		Sleep(LERP_MIN); // �� �Լ��� SleepEx��

		if (framework.CheckClientNumber()) {
			// ���� ó��
			framework.ProceedContinuation();
		} else {
			// ���� ������ Ȥ�� ���� ���� ����
		}
	}

	return 0;
}

DWORD WINAPI ConnectProcess(LPVOID arg) {
	while (true) {
		framework.AwaitClientAcceptEvent();

		SOCKET new_client = framework.PlayerConnect();
		if (INVALID_SOCKET == new_client) {
			cerr << "accept ����!";
			return 0;
		}
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
		cout << "�÷��̾� " << owner << "�� �Ѿ� �浹" << endl;

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
			cout << "�÷��̾� " << owner << " ���." << endl;
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
