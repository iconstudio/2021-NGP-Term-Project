#pragma once
#include "Framework.h"
#include "CommonDatas.h"


DWORD WINAPI CommunicateProcess(LPVOID arg);
DWORD WINAPI GameProcess(LPVOID arg);

class CCharacter : public GameInstance {
public:
	CCharacter();

	virtual void OnUpdate(double frame_advance);

	int health;
	double attack_cooltime;
};

class CBullet : public GameInstance {
public:
	CBullet();

	virtual void OnUpdate(double frame_advance);

	double lifetime;
};
