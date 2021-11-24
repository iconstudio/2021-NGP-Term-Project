#pragma once
#include "Framework.h"
#include "CommonDatas.h"


class CCharacter : public GameInstance {
public:
	CCharacter();

	virtual void OnUpdate(double frame_advance);

	virtual const char* GetIdentifier() const;

	void UpdateMessage(int index, int count, double x, double y, double hp, double direction);
	void GetHurt(int dmg);
	void Die();

	double health;
	double attack_cooltime;
	double inv_time;

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
