#pragma once
#include "stdafx.h"
#include "CommonDatas.h"
<<<<<<< Updated upstream
#include "GameInstance.h"

CRITICAL_SECTION client_permission, print_permission;

class CCharacter : public GameInstance {
public:
	CCharacter();

	virtual void OnUpdate(double frame_advance);

	virtual const char* GetIdentifier() const;

	void GetHurt(int dmg);
	void Die();

	double health;
	double attack_cooltime;
	double inv_time;
};

class CBullet : public GameInstance {
public:
	CBullet();

	virtual void OnUpdate(double frame_advance);

	virtual const char* GetIdentifier() const;

	double lifetime;
};

class ClientSession {
public:
	SOCKET my_socket;
	HANDLE my_thread;

	int player_index; // 플레이어 번호
	CCharacter* player_character;

	ClientSession(SOCKET sk, HANDLE th, int id);
	~ClientSession();
};

/* 게임 관련 속성 */
normal_distribution<> random_distrubution; // 서버의 무작위 분포 범위
default_random_engine randomizer;
=======
>>>>>>> Stashed changes
