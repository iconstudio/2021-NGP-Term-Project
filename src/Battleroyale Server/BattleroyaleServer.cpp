﻿#include "BattleroyaleServer.h"
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

	WSADATA wsadata;
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsadata)) {
		return 0;
	}

	framework.Initialize();

	while (true) {
		framework.Update();
		cout << "Sleep: " << FRAME_TIME << endl;

		Sleep(FRAME_TIME);
	}

	return 0;
}

DWORD WINAPI CommunicateProcess(LPVOID arg) {
	CCharacter* player_character;

	while (true) {

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
