#include "stdafx.h"
#include "Framework.h"
#include "CommonDatas.h"
#include "Main.h"

ServerFramework framework;

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
	int player_index = client->player_index;

	while (true) {
		framework.AwaitReceiveEvent();

		PACKETS header;
		ZeroMemory(&header, HEADER_SIZE);

		// 1-1. 패킷 헤더 수신
		int result = recv(client_socket, reinterpret_cast<char*>(&header), HEADER_SIZE, MSG_WAITALL);
		if (framework.ValidateSocketMessage(result)) {
			framework.DisconnectClient(client);
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
				if (framework.ValidateSocketMessage(result)) {
					framework.DisconnectClient(client);
				}

				CCharacter* player_ch = client->player_character;
				if (player_ch && !player_ch->dead) {
					auto& player_x = player_ch->x;
					auto& player_y = player_ch->y;

					player_ch->SetSpeed(0);
					for (int i = 0; i < client_data_size; ++i) {
						char input = client_data[i];

						switch (input) {
							case VK_LEFT:
							{
								player_ch->SetVelocity(PLAYER_MOVE_SPEED, 180);
							}
							break;

							case VK_UP:
							{
								player_ch->SetVelocity(PLAYER_MOVE_SPEED, 90);
							}
							break;

							case VK_RIGHT:
							{
								player_ch->SetVelocity(PLAYER_MOVE_SPEED, 0);
							}
							break;

							case VK_DOWN:
							{
								player_ch->SetVelocity(PLAYER_MOVE_SPEED, 270);
							}
							break;

							case 'A': // 총알
							{
								auto bullet = framework.Instantiate<CBullet>(player_x, player_y - 20);
								bullet->SetVelocity(SNOWBALL_SPEED, player_ch->direction);
								bullet->SetOwner(player_index);
								//bullet->SetDirection(player_ch->direction);
								//bullet->SetSpeed(SNOWBALL_SPEED);
							}
							break;

							case VK_SPACE: // 특수 능력
							{

							}
							break;
						}
					}
				} // IF (player_ch)
			} // CASE PACKETS::CLIENT_KEY_INPUT
			break;

			case PACKETS::CLIENT_PING:
			{
				// 아무것도 안함
			}
			break;

			default: break;
		}

		// 2. 게임 진행
		if (client_data)
			AtomicPrintLn("받은 패킷 내용: ", client_data);

		framework.ProceedContinuation();
	}

	return 0;
}

DWORD WINAPI GameUpdateProcess(LPVOID arg) {
	while (true) {
		framework.AwaitUpdateEvent();

		// 3. 게임 처리
		framework.GameUpdate();

		// 4. 렌더링 정보 작성
		framework.CreateRenderingInfos();

		// 5. 렌더링 정보 전송
		framework.SendGameInfosToAll();

		// 6. 대기
		Sleep(FRAME_TIME);

		framework.CastReceiveEvent();
	}

	return 0;
}

CCharacter::CCharacter()
	: GameInstance()
	, attack_cooltime(0.0), inv_time(0.0)
	, health(PLAYER_HEALTH) {
	SetRenderType(RENDER_TYPES::CHARACTER);
	SetBoundBox(RECT{ -8, -8, 8, 8 });

	image_speed = 0.4;
}

void CCharacter::OnUpdate(double frame_advance) {
	auto collide_bullet = framework.SeekCollision<CBullet>(this, "Bullet");

	if (collide_bullet && collide_bullet->owner != owner) {
		framework.Kill(collide_bullet);
		cout << "플레이어 " << owner << "의 총알 충돌" << endl;

		GetHurt(1);
	}

	if (hspeed != 0.0 || vspeed != 0.0)
		direction = point_direction(0, 0, hspeed, vspeed);

	if (0 < inv_time) {
		inv_time -= frame_advance;
	}

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
