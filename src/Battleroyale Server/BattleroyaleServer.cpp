#include "BattleroyaleServer.h"
#include "pch.h"
#include "CommonDatas.h"


ServerFramework framework{ GAME_SCENE_W, GAME_SCENE_H };

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
	CCharacter* player_character;
	SOCKET client_socket = reinterpret_cast<SOCKET>(&arg);

	while (true) {
		PacketMessage* packet = nullptr;
		int result = recv(client_socket, reinterpret_cast<char*>(&packet)
						  , sizeof(PacketMessage), MSG_WAITALL);
		if (SOCKET_ERROR == result) {
			break;
		} else if (0 == result) {
			break;
		}
		
		int data_size = packet->size;

		void* data = nullptr;
		if (0 < data_size) {
			result = recv(client_socket, reinterpret_cast<char*>(&data), data_size, MSG_WAITALL);
		}

		switch (framework.status) {
			case LOBBY:
			{
				cout << "대기실 입장" << endl;


			}
			break;

			case GAME:
			{

			}
			break;

			case GAME_OVER:
			{

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

	return 0;
}

DWORD WINAPI GameProcess(LPVOID arg) {
	CCharacter* player_character;

	while (true) {

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
