﻿#include "GameBehavior.h"


GameInstance::GameInstance()
	: owner(-1)
	, image_angle(0.0), image_index(0.0), image_speed(0.0), image_number(0.0)
	, my_renders{}
	, box{}, dead(false)
	, x(0), y(0), hspeed(0.0), vspeed(0.0), direction(0.0) {
	ZeroMemory(&my_renders, sizeof(my_renders));
}

GameInstance::~GameInstance() {
	delete& my_renders;
}

void GameInstance::OnUpdate(double frame_advance) {
	if (hspeed != 0.0 || vspeed != 0.0) {
		x += hspeed * frame_advance;
		y += vspeed * frame_advance;
	}

	if (image_speed != 0.0 && 1.0 < image_number) {
		image_index += image_speed;

		if (image_index < 0) {
			image_index += image_number;
		} else if (image_number <= image_index) {
			image_index -= image_number;
		}
	}
}

void GameInstance::SetOwner(int player_index) {
	owner = player_index;
}

void GameInstance::SetRenderType(RENDER_TYPES sprite) {
	my_renders.instance_type = sprite;
}

void GameInstance::SetImageNumber(int number) {
	image_number = static_cast<double>(number);
}

void GameInstance::SetDirection(double dir) {
	if (hspeed != 0.0 || vspeed != 0.0) {
		auto speed = point_distance(0, 0, hspeed, vspeed);

		hspeed = lengthdir_x(speed, dir);
		vspeed = lengthdir_y(speed, dir);
	}

	direction = dir;
}

void GameInstance::SetSpeed(double speed) {
	if (hspeed != 0.0 || vspeed != 0.0) {
		auto old_dir = point_direction(0, 0, hspeed, vspeed);

		hspeed = lengthdir_x(speed, old_dir);
		vspeed = lengthdir_y(speed, old_dir);
	}
}

void GameInstance::SetVelocity(double speed, double dir) {
	hspeed = lengthdir_x(speed, dir);
	vspeed = lengthdir_y(speed, dir);
}

RenderInstance& GameInstance::GetRenderInstance() {
	return my_renders;
}

void GameInstance::SetBoundBox(const RECT& mask) {
	CopyRect(&box, &mask);
}

int GameInstance::GetBoundLT() const {
	return x + box.left;
}

int GameInstance::GetBoundTP() const {
	return y + box.top;
}

int GameInstance::GetBoundRT() const {
	return x + box.right;
}

int GameInstance::GetBoundBT() const {
	return y + box.bottom;
}

const char* GameInstance::GetIdentifier() const {
	return "Instance";
}

bool GameInstance::IsCollideWith(GameInstance* other) {
	return !(other->GetBoundRT() <= GetBoundLT()
		|| other->GetBoundBT() <= GetBoundTP()
		|| GetBoundRT() < other->GetBoundLT()
		|| GetBoundBT() < other->GetBoundTP());
}

RenderInstance& GameInstance::AssignRenderingInfo(double angle) {
	my_renders.x = x;
	my_renders.y = y;

	my_renders.image_index = static_cast<int>(image_index);
	my_renders.angle = angle;

	return my_renders;
}