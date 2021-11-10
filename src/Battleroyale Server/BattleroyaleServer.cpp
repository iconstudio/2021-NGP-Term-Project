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

	framework.Initialize();
	while (true) {
		framework.Update();

		Sleep(FRAME_TIME);
	}
}

DWORD WINAPI CommunicateProcess(LPVOID arg) {
	while (true) {

	}

	return 0;
}

DWORD WINAPI GameProcess(LPVOID arg) {
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
