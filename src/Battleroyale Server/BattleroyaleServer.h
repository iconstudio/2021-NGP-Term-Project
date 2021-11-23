#pragma once
#include "Framework.h"
#include "CommonDatas.h"


class CCharacter : public GameInstance {
public:
	CCharacter();

	virtual void OnUpdate(double frame_advance);

	virtual const char* GetIdentifier() const;

	void UpdateMessage(int index, int count, double x, double y, int hp, double direction);

	int health;
	int index;
	double attack_cooltime;

	GameUpdateMessage update_info;		// 클라이언트에 전달할 플레이어 정보
};

class CBullet : public GameInstance {
public:
	CBullet();

	virtual void OnUpdate(double frame_advance);

	virtual const char* GetIdentifier() const;

	double lifetime;
};
