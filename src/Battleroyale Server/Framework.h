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

	int index; // �÷��̾� ��ȣ
	map<WPARAM, INPUT_TYPES> key_storage;
	void* player_character = nullptr;

	PlayerInfo(SOCKET sk, HANDLE hd, int id);
	~PlayerInfo();
};

enum SERVER_STATES : int {
	LISTEN = 0			// Ŭ���̾�Ʈ ���� ���
	, LOBBY				// �κ�
	, GAME				// ����
	, GAME_OVER			// ���� �Ϸ�
	, GAME_RESTART		// ���� �ٽ� ����
	, EXIT				// ���� ����
};

enum class ACTION_TYPES : int {
	NONE = 0
	, SET_HSPEED
	, SET_VSPEED
	, SHOOT // ����ü �߻�
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

	void SetRenderType(RENDER_TYPES sprite);
	void SetImageNumber(int number);
	void SetRenderInstance();
	RenderInstance GetRenderInstance() const;

	void SetBoundBox(const RECT& mask);
	int GetBoundLT() const;
	int GetBoundTP() const;
	int GetBoundRT() const;
	int GetBoundBT() const;

	virtual const char* GetIdentifier() const;

	bool IsCollideWith(GameInstance* other);

	RenderInstance* MakeRenderInfos();

	bool dead;
	int owner;
	double x, y, hspeed, vspeed, direction;
	double image_angle, image_index, image_speed, image_number;

private:
	RECT box; // �浹ü

	RenderInstance my_renders;
};

class ServerFramework {
private:
	struct IO_MSG {
		ACTION_TYPES type;
		int player_index = 0;
		int data = 0;
	};

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
	void CastStartGame(bool flag);
	void CastProcessingGame();
	void CastSendRenders(bool flag);

	template<class _GameClassTarget, class _GameClassSelf>
	_GameClassTarget* SeekCollision(_GameClassSelf* self, const char* fid);

	template<class _GameClass1, class _GameClass2>
	_GameClass2* CheckCollision(_GameClass1* self, _GameClass2* other);

	inline DWORD WINAPI AwaitClientAcceptEvent();
	inline DWORD WINAPI AwaitReceiveEvent();
	inline DWORD WINAPI AwaitStartGameEvent();
	inline DWORD WINAPI AwaitProcessingGameEvent();
	inline DWORD WINAPI AwaitSendRendersEvent();

	IO_MSG* QueingPlayerAction(PlayerInfo* player, ACTION_TYPES type, int data = 0);

	void ProceedContinuation();
	void BuildRenderings();
	void SendRenderings();

	template<class _GameClass = GameInstance>
	_GameClass* Instantiate(int x = 0, int y = 0);

	template<class _GameClass = GameInstance>
	void Kill(_GameClass* target);

	friend DWORD WINAPI ConnectProcess(LPVOID arg);
	friend DWORD WINAPI GameInitializeProcess(LPVOID arg);
	friend DWORD WINAPI CommunicateProcess(LPVOID arg);
	friend DWORD WINAPI GameProcess(LPVOID arg);

private:
	SERVER_STATES status;
	bool status_begin;

	SOCKET my_socket;
	SOCKADDR_IN	my_address;
	WSAOVERLAPPED io_behavior;
	int my_process_index; // ���� ó�� ���� �÷��̾��� ���� (0~client_number)

	vector<HANDLE> thread_list; // ������ ���
	vector<PlayerInfo*> players; // �÷��̾� ���

	int	client_number; // ���� ������ �÷��̾��� ��
	int player_number_last; // �������� �߰��� �÷��̾��� ��ȣ
	int	player_captain; // ���� �÷��̾�

	HANDLE thread_game_starter;
	HANDLE thread_game_process;

	HANDLE event_player_accept; // �÷��̾� ������ �޴� �̺�Ʈ ��ü
	HANDLE event_game_start; // ���� ������ �ϴ� �̺�Ʈ ��ü
	HANDLE event_receives; // �÷��̾��� �Է��� �޴� �̺�Ʈ ��ü
	HANDLE event_game_process; // �浹 ó���� �ϴ� �̺�Ʈ ��ü
	HANDLE event_send_renders; // ������ ������ ������ �̺�Ʈ ��ü

	int** PLAYER_SPAWN_PLACES; // �÷��̾ �� ó���� ������ ��ġ�� �迭
	const int WORLD_W, WORLD_H;
	const int SPAWN_DISTANCE;

	RenderInstance render_last[40];
	vector<GameInstance*> instances;

	vector<IO_MSG*> io_queue;

	PlayerInfo* GetPlayer(int player_index);

	void InterpretPlayerAction();
	void ClearPlayerActions();
	void ContinueToReceive();
	void ContinueToGameProcess();
	void ContinueToSendingRenders();

	template<class Predicate>
	void ForeachInstances(Predicate predicate);
};


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
