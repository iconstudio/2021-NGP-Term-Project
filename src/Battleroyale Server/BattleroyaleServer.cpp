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
				// ������ ���� ���� �޽���

				// ���� �ʱ�ȭ
				if (player_index == framework.player_captain &&
					packet == PACKETS::CLIENT_GAME_START) {
					if (1 < framework.client_number) {
						framework.CastStartGame(true);
						break;
					}
				} // �ٸ� �޽����� ������.
			} break;

			case GAME:
			{
				// ������ ���
				while (true) {
					framework.AwaitReceiveEvent(); // event_recieves

					// ���� �� �޽����� ���� �����͸� ���� �ʴ´�.
					if (packet == PACKETS::CLIENT_KEY_INPUT) {
						data = new char;
						result = recv(client_socket, data, 1, MSG_WAITALL);
						char button = (*data);

						if (data) {
							switch (*data) {
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
					} // �ٸ� �޽����� ������.

					/*
						TODO: I/O Overlapeed �𵨷� �ٲٱ� ���ؼ��� APC �Լ�����
						�ʼ����̶�� �Ѵ�. �ü���� �޽��� ť�� ����ϴ� �Լ��� �ִ�.
					*/
					framework.CastProcessingGame();

					framework.AwaitSendRendersEvent(); // event_send_renders
					framework.SendRenderings();

					framework.CastSendRenders(false);

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
	TODO: I/O Overlapped �𵨷� �����ϱ�

	�ֳ��ϸ� ������ �������� �ѹ��� ���� Ŭ���̾�Ʈ�� ó���ϱ� ���ؼ��� ���� ������ �ʼ����̴�.
	IOCP ���� �� �κп��� Overlapped ���� ����ϸ� ���� �� ����.
*/
DWORD WINAPI GameProcess(LPVOID arg) {
	while (true) {
		framework.AwaitProcessingGameEvent(); // �� �Լ��� WaitForSingleObjectEx��,
											  // evemt_game_process
		framework.CastStartReceive(false);
		Sleep(LERP_MIN); // �� �Լ��� SleepEx��

		if (1 < framework.GetClientCount()) {
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
	: GameInstance(), update_info{}
	, attack_cooltime(0.0)
	, health(PLAYER_HEALTH) {
	SetRenderType(RENDER_TYPES::CHARACTER);
	SetBoundBox(RECT{ -6, -6, 6, 6 });
}

void CCharacter::OnUpdate(double frame_advance) {
	CBullet* collide_bullet = framework.SeekCollision<CBullet>(this, "Bullet");

	if (collide_bullet) {
		health--;
		framework.Kill(collide_bullet);
		cout << "�÷��̾� " << owner << "�� �Ѿ� �浹" << endl;
	}

	image_angle = point_direction(0, 0, hspeed, vspeed);
	UpdateMessage(owner, framework.GetClientCount(), x, y, health, image_angle);

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
