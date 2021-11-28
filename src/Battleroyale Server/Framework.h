#pragma once
#include "pch.h"
#include "CommonDatas.h"


const int LERP_MIN = 50;
const int LERP_MAX = 200;

DWORD WINAPI ConnectProcess(LPVOID arg);
DWORD WINAPI CommunicateProcess(LPVOID arg);
DWORD WINAPI GameReadyProcess(LPVOID arg);
DWORD WINAPI GameProcess(LPVOID arg);

struct PlayerInfo {
	SOCKET client_socket;
	HANDLE client_handle;

	int index; // 플레이어 번호
	void* player_character = nullptr;

	PlayerInfo(SOCKET sk, HANDLE hd, int id);
	~PlayerInfo();
};

enum SERVER_STATES : int {
	LISTEN = 0			// 클라이언트 접속 대기
	, LOBBY				// 로비
	, GAME				// 게임
	, GAME_OVER			// 게임 완료
	, GAME_RESTART		// 게임 다시 시작
	, EXIT				// 서버 종료
};

enum class ACTION_TYPES : int {
	NONE = 0
	, SET_HSPEED
	, SET_VSPEED
	, SHOOT // 투사체 발사
};

class GameInstance {
public:
	GameInstance();
	virtual ~GameInstance();

	virtual void OnCreate();
	virtual void OnDestroy();
	virtual void OnUpdate(double frame_advance);
	virtual const char* GetIdentifier() const;

	void SetRenderType(RENDER_TYPES sprite);
	void SetImageNumber(int number);
	void SetDirection(double dir);
	void SetSpeed(double speed);
	void SetVelocity(double speed, double dir);

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

private:
	RECT box; // 충돌체

	RenderInstance my_renders;
};

class ServerFramework {
public:
	ServerFramework(int room_width, int room_height);
	~ServerFramework();

	bool Initialize();
	void Startup();
	void ProcessConnect();
	void ProcessReady();
	void ProcessGame();
	void Clean();

	void SetStatus(SERVER_STATES state);
	void SetCaptain(PlayerInfo* player);
	SERVER_STATES GetStatus() const;
	int GetClientNumber() const;

	SOCKET PlayerConnect();
	void PlayerDisconnect(PlayerInfo* player);

	bool CheckClientNumber() const;
	bool ValidateSocketMessage(int socket_state);
	void ProceedContinuation();
	void BakeRenderingInfos();
	void SendRenderingInfos(SOCKET my_socket);

	template<class _GameClass = GameInstance>
	_GameClass* Instantiate(double x = 0.0, double y = 0.0);

	template<class _GameClass = GameInstance>
	void Kill(_GameClass* target);

	template<class _GamePlayerClass>
	void CreatePlayerCharacters();

	template<class _GameClassTarget, class _GameClassSelf>
	_GameClassTarget* SeekCollision(_GameClassSelf* self, const char* fid);

	void CastClientAccept(bool flag);
	void CastStartReceive(bool flag);
	void CastStartGame(bool flag);
	void CastProcessingGame();
	void CastSendingRenderingInfos(bool flag);

	inline DWORD WINAPI AwaitClientAcceptEvent();
	inline DWORD WINAPI AwaitReceiveEvent();
	inline DWORD WINAPI AwaitStartGameEvent();
	inline DWORD WINAPI AwaitProcessingGameEvent();
	inline DWORD WINAPI AwaitSendRendersEvent();

	friend DWORD WINAPI CommunicateProcess(LPVOID arg);
	friend DWORD WINAPI GameProcess(LPVOID arg);

private:
	PlayerInfo* GetPlayer(int player_index);

	template<class _GameClass1, class _GameClass2>
	_GameClass2* CheckCollision(_GameClass1* self, _GameClass2* other);

	template<class Predicate>
	void ForeachInstances(Predicate predicate);

	SERVER_STATES status;
	bool status_begin;

	/* 통신 관련 속성 */
	SOCKET my_socket;
	SOCKADDR_IN	my_address;

	/* 다중 스레드 관련 속성 */
	WSAOVERLAPPED io_behavior;
	HANDLE thread_game_starter;
	HANDLE thread_game_process;
	HANDLE event_player_accept; // 플레이어 접속을 받는 이벤트 객체
	HANDLE event_game_start; // 게임 시작을 하는 이벤트 객체
	HANDLE event_receives; // 플레이어의 입력을 받는 이벤트 객체
	HANDLE event_game_process; // 충돌 처리를 하는 이벤트 객체
	HANDLE event_send_renders; // 렌더링 정보를 보내는 이벤트 객체
	int my_process_index; // 현재 처리 중인 플레이어의 순번 [0~client_number)

	/* 플레이어 관련 속성 */
	vector<PlayerInfo*> players; // 플레이어 목록
	int	client_number; // 지금 접속한 플레이어의 수
	int player_number_last; // 마지막에 추가된 플레이어의 번호
	int	player_captain; // 방장 플레이어
	int player_winner; // 승리한 플레이어

	/* 게임 관련 속성 */
	vector<GameInstance*> instances; // 인스턴스 목록
	normal_distribution<> random_distrubution; // 서버의 무작위 분포 범위
	default_random_engine randomizer;

	const int WORLD_W, WORLD_H;
	int** PLAYER_SPAWN_PLACES; // 플레이어가 맨 처음에 생성될 위치의 배열
	const int SPAWN_DISTANCE;

	RenderInstance* rendering_infos_last; // 전송할 렌더링 정보
};

template<class _GamePlayerClass>
void ServerFramework::CreatePlayerCharacters() {
	auto sz = players.size();
	for (int i = 0; i < sz; ++i) {
		auto player = players.at(i);
		auto places = PLAYER_SPAWN_PLACES[i];
		auto character = Instantiate<_GamePlayerClass>(places[0], places[1]);

		player->player_character = character;
		character->owner = player->index;
		SendData(player->client_socket, PACKETS::SERVER_GAME_START);
	}
}

template<class _GameClassTarget, class _GameClassSelf>
inline _GameClassTarget* ServerFramework::SeekCollision(_GameClassSelf* self, const char* fid) {
	if (self && !instances.empty()) {
		auto CopyList = vector<GameInstance*>(instances);

		auto it = std::find_if(CopyList.begin(), CopyList.end(), [&](GameInstance* inst) {
			auto iid = inst->GetIdentifier();
			auto id_check = strcmp(iid, fid);

			return (0 == id_check);
		});

		if (it != CopyList.end()) {
			return dynamic_cast<_GameClassTarget*>(*it);
		}
	}
	return nullptr;
}

template<class _GameClass1, class _GameClass2>
inline _GameClass2* ServerFramework::CheckCollision(_GameClass1* self, _GameClass2* other) {
	if (self && other && self != other) {
		if (self->IsCollideWith(other))
			return other;
	}
	return nullptr;
}

template<class _GameClass>
inline _GameClass* ServerFramework::Instantiate(double x, double y) {
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

template<class Predicate>
inline void ServerFramework::ForeachInstances(Predicate predicate) {
	if (!instances.empty()) {
		auto CopyList = vector<GameInstance*>(instances);

		std::for_each(CopyList.begin(), CopyList.end(), predicate);
	}
}

inline DWORD WINAPI ServerFramework::AwaitClientAcceptEvent() {
	return WaitForSingleObject(event_player_accept, INFINITE);
}

inline DWORD WINAPI ServerFramework::AwaitReceiveEvent() {
	return WaitForSingleObject(event_receives, INFINITE);
}

inline DWORD WINAPI ServerFramework::AwaitStartGameEvent() {
	return WaitForSingleObject(event_game_start, INFINITE);
}

inline DWORD WINAPI ServerFramework::AwaitProcessingGameEvent() {
	return WaitForSingleObject(event_game_process, INFINITE);
}

inline DWORD WINAPI ServerFramework::AwaitSendRendersEvent() {
	return WaitForSingleObject(event_send_renders, INFINITE);
}
