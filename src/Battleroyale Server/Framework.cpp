#include "pch.h"
#include "Framework.h"


GameInstance::GameInstance()
	: owner(-1)
	, sprite_index(0), box{}
	, dead(false)
	, x(0), y(0), hspeed(0.0), vspeed(0.0) {}

GameInstance::~GameInstance() {}

void GameInstance::OnCreate() {}

void GameInstance::OnDestroy() {}

void GameInstance::OnUpdate(double frame_advance) {
	x += hspeed * frame_advance;
	y += vspeed * frame_advance;
}

void GameInstance::SetSprite(int sprite) {
	sprite_index = sprite;
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

bool GameInstance::IsCollideWith(RECT& other) {
	return !(other.right <= GetBoundLT()
			 || other.bottom <= GetBoundTP()
			 || GetBoundRT() < other.left
			 || GetBoundBT() < other.top);
}

bool GameInstance::IsCollideWith(GameInstance*& other) {
	return !(other->GetBoundRT() <= GetBoundLT()
			 || other->GetBoundBT() <= GetBoundTP()
			 || GetBoundRT() < other->GetBoundLT()
			 || GetBoundBT() < other->GetBoundTP());
}

ServerFramework::ServerFramework(int rw, int rh)
	: delta_time(0.0), elapsed(0)
	, WORLD_W(rw), WORLD_H(rh), SPAWN_DISTANCE(rh * 0.4)
	, status(SERVER_STATES::LISTEN)
	, client_number(0), player_captain(-1) {

	PLAYER_SPAWN_PLACES = new int*[PLAYERS_NUMBER_MAX];

	double dir_increment = (360.0 / PLAYERS_NUMBER_MAX);
	for (int i = 0; i < PLAYERS_NUMBER_MAX; ++i) {
		double dir = dir_increment * i;
		int cx = (int)(WORLD_W * 0.5 + lengthdir_x(SPAWN_DISTANCE, dir));
		int cy = (int)(WORLD_W * 0.5 + lengthdir_y(SPAWN_DISTANCE, dir));

		PLAYER_SPAWN_PLACES[i] = new int[2]{ cx, cy };
	}
}

ServerFramework::~ServerFramework() {}

void ServerFramework::Initialize() {
	StartDelta();
}

void ServerFramework::Update() {
	InspectDelta();

	// get a elapsed second
	delta_time =  ((double)elapsed / (double)tick_type::period::den);

	ForeachInstances([&](GameInstance*& inst) {
		inst->OnUpdate(delta_time);
	});

	StartDelta();
}

void ServerFramework::StartDelta() {
	clock_previos = std::chrono::system_clock::now();
}

void ServerFramework::InspectDelta() {
	clock_now = std::chrono::system_clock::now();

	elapsed = std::chrono::duration_cast<tick_type>(clock_now - clock_previos).count();
}
