#pragma once
#include "pch.h"
#include "CommonDatas.h"


DWORD WINAPI ConnectProcess(LPVOID arg);
DWORD WINAPI CommunicateProcess(LPVOID arg);
DWORD WINAPI GameInitializeProcess(LPVOID arg);
DWORD WINAPI GameProcess(LPVOID arg);

struct PlayerInfo {
	SOCKET client_socket;
	HANDLE client_handle;
	int index; // 플레이어 번호

	void* player_character = nullptr;

	PlayerInfo(SOCKET sk, HANDLE hd, int id);
};

enum SERVER_STATES : int {
	LISTEN = 0			// 클라이언트 접속 대기
	, LOBBY				// 로비
	, GAME				// 게임
	, GAME_OVER			// 게임 완료
	, GAME_RESTART		// 게임 다시 시작
	, EXIT				// 서버 종료
};

enum class MSG_TYPES : int {
	NONE = 0
	, SET_HSPEED
	, SET_VSPEED
	, SHOOT_LT // 좌측으로 사격
	, SHOOT_RT // 우측으로 사격
	, SHOOT_UP // 상단으로 사격
	, SHOOT_DW // 하단으로 사격
};

const int LERP_MIN = 50;
const int LERP_MAX = 200;

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

	bool Initialize();
	void Startup();
	void GameUpdate();
	void Clean();

	SOCKET PlayerConnect();
	void PlayerDisconnect(PlayerInfo* player);

	void SetCaptain(PlayerInfo* player);
	void SetStatus(SERVER_STATES state);

	SERVER_STATES GetStatus() const;
	int GetClientCount() const;

	void CastClientAccept(bool flag);
	void CastStartReceive(bool flag);
	void CastProcessingGame();
	void CastSendRenders();

	inline DWORD AwaitClientAcceptEvent();
	inline DWORD AwaitReceiveEvent();
	inline DWORD AwaitProcessingGameEvent();
	inline DWORD AwaitSendRendersEvent();

	template<class _GameClass = GameInstance>
	_GameClass* Instantiate(int x = 0, int y = 0);

	template<class _GameClass = GameInstance>
	void Kill(_GameClass* target);

	SERVER_STATES status;

	friend DWORD WINAPI ConnectProcess(LPVOID arg);
	friend DWORD WINAPI CommunicateProcess(LPVOID arg);
	friend DWORD WINAPI GameInitializeProcess(LPVOID arg);
	friend DWORD WINAPI GameProcess(LPVOID arg);

private:
	bool status_begin = true;
	SOCKET my_socket;
	SOCKADDR_IN	my_address;

	vector<HANDLE> thread_list; // 스레드 목록
	vector<PlayerInfo*> players; // 플레이어 목록

	HANDLE thread_game_starter;
	HANDLE thread_game_process;

	HANDLE event_player_accept; // 플레이어 접속을 받는 이벤트 객체
	HANDLE event_game_start; // 게임 시작을 하는 이벤트 객체
	HANDLE event_receives; // 플레이어의 입력을 받는 이벤트 객체
	HANDLE event_game_process; // 충돌 처리를 하는 이벤트 객체
	HANDLE event_send_renders; // 렌더링 정보를 보내는 이벤트 객체

	WSAOVERLAPPED io_behavior;

	int **PLAYER_SPAWN_PLACES; // 플레이어가 맨 처음에 생성될 위치의 배열
	const int WORLD_W, WORLD_H;
	const int SPAWN_DISTANCE;

	int	client_number; // 지금 접속한 플레이어의 수
	int player_number_last; // 마지막에 추가된 플레이어의 번호
	int	player_captain; // 방장 플레이어

	RenderInstance render_last[40];
	vector<GameInstance*> instances;

	struct IO_MSG {
		MSG_TYPES type;
		int player_index = 0;
		int* data = nullptr;
	};

	vector<IO_MSG*> io_queue;
	map<WPARAM, bool> key_checkers;

	template<class Predicate>
	void ForeachInstances(Predicate predicate);
};

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

inline DWORD __stdcall ServerFramework::AwaitReceiveEvent() {
	return WaitForSingleObject(event_receives, INFINITE);
}

inline DWORD __stdcall ServerFramework::AwaitProcessingGameEvent() {
	return WaitForSingleObject(event_game_process, INFINITE);
}

inline DWORD __stdcall ServerFramework::AwaitSendRendersEvent() {
	return WaitForSingleObject(event_send_renders, INFINITE);
}
