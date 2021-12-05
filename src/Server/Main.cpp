#include "stdafx.h"
#include "Framework.h"
#include "CommonDatas.h"
#include "Main.h"
#include "ServerFramework.h"


ServerFramework framework;

normal_distribution<> random_distrubution; // 서버의 무작위 분포 범위
default_random_engine randomizer;

int main() {
	framework.Initialize();
	framework.Startup();

	// 서버 대기
	framework.AwaitQuitEvent();

	return 0;
}

DWORD WINAPI ConnectProcess(LPVOID arg) {
	while (true) {
		framework.SetConnectProcess();
		SOCKET client_socket = framework.AcceptClient();
		if (INVALID_SOCKET == client_socket) {
			ErrorDisplay("connect()");
			continue;
		}

		framework.ConnectClient(client_socket);

		framework.AwaitClientAcceptEvent();
	}

	return 0;
}

DWORD WINAPI GameProcess(LPVOID arg) {
	ClientSession* client = reinterpret_cast<ClientSession*>(arg);
	SOCKET client_socket = client->my_socket;

	while (true) {
		framework.AwaitReceiveEvent();

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
		framework.CreateRenderingInfos();

		// 5. 렌더링 정보 전송
		framework.SendRenderingInfos(client_socket);

		// 6. 대기
		Sleep(FRAME_TIME);
		framework.SetGameProcess();
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
