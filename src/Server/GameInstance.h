#pragma once
#include "stdafx.h"
#include "CommonDatas.h"


class GameInstance {
public:
	GameInstance();
	virtual ~GameInstance();

	virtual void OnUpdate(double frame_advance);

	void SetOwner(int player_index);
	void SetRenderType(RENDER_TYPES sprite);
	void SetImageNumber(int number);
	void SetDirection(double dir);
	void SetSpeed(double speed);
	void SetVelocity(double speed, double dir);
	virtual const char* GetIdentifier() const;

	RenderInstance& AssignRenderingInfo(double angle);
	RenderInstance& GetRenderInstance();

	void SetBoundBox(const RECT& mask);
	int GetBoundLT() const;
	int GetBoundTP() const;
	int GetBoundRT() const;
	int GetBoundBT() const;
	bool IsCollideWith(GameInstance* other);

	bool dead;
	int owner;
	double x, y, hspeed, vspeed, direction;
	double image_angle, image_index, image_speed, image_number;

	RECT box; // 충돌체
	RenderInstance my_renders;
};
