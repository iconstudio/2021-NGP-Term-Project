#pragma once
#include "pch.h"
#include "CommonDatas.h"


enum SERVER_STATES : int {
	LISTEN = 0			// 클라이언트 접속 대기
	, LOBBY				// 로비
	, GAME				// 게임
	, GAME_OVER			// 게임 완료
	, GAME_RESTART		// 게임 다시 시작
	, EXIT				// 서버 종료
};

struct GameUpdateMessage {
	int players_count;

	int target_player;
	int player_hp;
	double player_x, player_y, player_direction;
};

class GameInstance {
public:
	GameInstance();
	virtual ~GameInstance();

	virtual void OnCreate();
	virtual void OnDestroy();
	virtual void OnUpdate(double frame_advance);

	void SetSprite(int sprite);
	void SetBoundBox(const RECT& mask);
	int GetBoundLT() const;
	int GetBoundTP() const;
	int GetBoundRT() const;
	int GetBoundBT() const;

	bool IsCollideWith(RECT& other);
	bool IsCollideWith(GameInstance*& other);

	int owner;
	double x, y, hspeed, vspeed;

private:
	int sprite_index;
	RECT box; // 충돌체
	bool dead;
};


class ServerFramework {
public:
	ServerFramework(int room_width, int room_height);
	~ServerFramework();

	void Initialize();
	void msg_update();
	void Update();

	void StartDelta();
	void InspectDelta();

	template<class Predicate>
	void for_each_instances(Predicate predicate);

	template<class _GameClass = GameInstance>
	_GameClass* Instantiate(int x = 0, int y = 0);

	template<class _GameClass = GameInstance>
	void Kill(_GameClass* target);

	SERVER_STATES status;
	HANDLE players[PLAYERS_NUMBER_MAX];

	HANDLE event_receives; // 플레이어의 입력을 받는 이벤트 객체
	HANDLE event_game_process; // 충돌 처리를 하는 이벤트 객체
	HANDLE event_send_renders; // 렌더링 정보를 보내는 이벤트 객체

	int **PLAYER_SPAWN_PLACES; // 플레이어가 맨 처음에 생성될 위치의 배열
	const int WORLD_W, WORLD_H;
	const int SPAWN_DISTANCE;

	int	client_number;
	int	player_captain;

	vector<GameInstance*> instances;
	vector<int> player_queue;

	using tick_type = std::chrono::microseconds;
	using clock_type = std::chrono::system_clock::time_point;
	clock_type clock_previos, clock_now;
	LONGLONG elapsed;
	double delta_time;
};

template<class Predicate>
inline void ServerFramework::for_each_instances(Predicate predicate) {
	if (!instances.empty()) {
		auto CopyList = vector<GameInstance*>(instances);

		std::for_each(CopyList.begin(), CopyList.end(), predicate);
	}
}

template<class _GameClass>
inline _GameClass* ServerFramework::Instantiate(int x, int y) {
	auto result = new _GameClass();
	result->x = x;
	result->y = y;
	result->OnCreate();

	instances.push_back(result);

	return result;
}

template<class _GameClass>
inline void ServerFramework::Kill(_GameClass* target) {
	auto loc = find_if(instances.begin(), instances.end(), [target](const auto& lhs) {
		return (lhs == target);
	});

	if (loc != instances.end()) {
		target->OnDestroy();
		instances.erase(loc);
	}
}