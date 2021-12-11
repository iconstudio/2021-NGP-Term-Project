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
		framework.CastAcceptEvent(true);
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
	int& player_index = client->player_index;

	while (true) {
		framework.AwaitReceiveEvent();

		PACKETS header;
		ZeroMemory(&header, HEADER_SIZE);

		// 1-1. 패킷 헤더 수신
		int result = recv(client_socket, reinterpret_cast<char*>(&header), HEADER_SIZE, MSG_WAITALL);
		if (!framework.ValidateSocketMessage(result)) {
			framework.DisconnectClient(client);
			break;
		}
		framework.AtomicPrintLn("받은 패킷 헤더: ", header);

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
				if (!framework.ValidateSocketMessage(result)) {
					framework.DisconnectClient(client);
					break;
				}

				CCharacter* player_ch = client->player_character;
				if (player_ch && !player_ch->dead) {
					auto& player_x = player_ch->x;
					auto& player_y = player_ch->y;

					player_ch->SetSpeed(0);
					player_ch->SetRenderType(RENDER_TYPES::CHARACTER);
					for (int i = 0; i < client_data_size; ++i) {
						char input = client_data[i];

						switch (input) {
							case VK_LEFT:
							{
								player_ch->SetVelocity(PLAYER_MOVE_SPEED, 180);
								if (!player_ch->invincible)
									player_ch->SetRenderType(RENDER_TYPES::CHARACTER_WALK);
							}
							break;

							case VK_UP:
							{
								player_ch->SetVelocity(PLAYER_MOVE_SPEED, 90);
								if (!player_ch->invincible)
									player_ch->SetRenderType(RENDER_TYPES::CHARACTER_WALK);
							}
							break;

							case VK_RIGHT:
							{
								player_ch->SetVelocity(PLAYER_MOVE_SPEED, 0);
								if (!player_ch->invincible)
									player_ch->SetRenderType(RENDER_TYPES::CHARACTER_WALK);
							}
							break;

							case VK_DOWN:
							{
								player_ch->SetVelocity(PLAYER_MOVE_SPEED, 270);
								if (!player_ch->invincible)
									player_ch->SetRenderType(RENDER_TYPES::CHARACTER_WALK);
							}
							break;

							case 'A': // 총알
							{
								auto bullet = framework.Instantiate<CBullet>(player_x, player_y - 20);
								bullet->SetVelocity(SNOWBALL_SPEED, player_ch->direction);
								bullet->SetOwner(player_index);
								bullet->hspeed += player_ch->hspeed;
								bullet->vspeed += player_ch->vspeed;
							}
							break;

							case 'S': // 특수 능력
							{
								auto pd = player_ch->direction;
								auto ax = lengthdir_x(PLAYER_BLINK_DISTANCE, pd);
								auto ay = lengthdir_y(PLAYER_BLINK_DISTANCE, pd);
								player_ch->AddPosition(ax, ay);
							}
							break;
						}
					}
					
					if (player_ch->hspeed == 0.0 && player_ch->vspeed == 0.0) {
						player_ch->image_speed = 0.0;
						player_ch->image_index = 0.0;
					} else {
						player_ch->image_speed = PLAYER_ANIMATION_SPEED;
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
			framework.AtomicPrintLn("받은 패킷 내용: ", client_data);

		framework.ProceedContinuation();
	}

	closesocket(client_socket);

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

		framework.CastReceiveEvent(true);
	}

	return 0;
}

CCharacter::CCharacter()
	: GameInstance()
	, attack_cooltime(0.0)
	, health(PLAYER_HEALTH), inv_time(0.0), invincible(false) {
	SetRenderType(RENDER_TYPES::CHARACTER);
	SetBoundBox(RECT{ -8, -20, 8, 9 });

	image_speed = 0.0;
}

void CCharacter::OnUpdate(double frame_advance) {
	auto collide_bullet = framework.SeekCollision<CBullet>(this, "Bullet");

	if (collide_bullet && collide_bullet->owner != owner) {
		framework.Kill(collide_bullet);
		framework.AtomicPrintLn("플레이어 ", owner, "의 총알 충돌");

		GetHurt(SNOWBALL_DAMAGE);
	}

	if (hspeed != 0.0 || vspeed != 0.0)
		direction = point_direction(0, 0, hspeed, vspeed);

	if (0 < inv_time) {
		inv_time -= frame_advance;
	} else if (invincible) {
		SetRenderType(RENDER_TYPES::CHARACTER);
		invincible = false;
	}

	GameInstance::OnUpdate(frame_advance);

	AssignRenderingInfo(direction);
}

const char* CCharacter::GetIdentifier() const { return "Player"; }

void CCharacter::GetHurt(double dmg) {
	if (inv_time <= 0) {
		health -= dmg;
		if (health <= 0) {
			framework.AtomicPrintLn("플레이어 ", owner, " 사망");
			Die();
		} else {
			SetRenderType(RENDER_TYPES::CHARACTER_HURT);
			invincible = true;
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
	SetBoundBox(RECT{ -8, -8, 8, 8 });
}

void CBullet::OnUpdate(double frame_advance) {
	lifetime -= frame_advance;
	if (lifetime <= 0) {
		framework.Kill(this);
		return;
	}

	image_angle = point_direction(0, 0, hspeed, vspeed);

	GameInstance::OnUpdate(frame_advance);

	AssignRenderingInfo(image_angle);
}

const char* CBullet::GetIdentifier() const { return "Bullet"; }
