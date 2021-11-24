#pragma once
#include "Framework.h"
#include "CommonDatas.h"


class CCharacter : public GameInstance {
public:
	CCharacter();

	virtual void OnUpdate(double frame_advance);

	virtual const char* GetIdentifier() const;

	void UpdateMessage(int index, int count, double x, double y, int hp, double direction);
	void GetHurt(int dmg);
	void Die();

	int health;
	double attack_cooltime;

	// Ŭ���̾�Ʈ�� ������ �÷��̾� ����
	GameUpdateMessage update_info;
};

class CBullet : public GameInstance {
public:
	CBullet();

	virtual void OnUpdate(double frame_advance);

	virtual const char* GetIdentifier() const;

	double lifetime;
};
