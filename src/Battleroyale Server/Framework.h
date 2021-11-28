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

	int index; // �÷��̾� ��ȣ
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
	RECT box; // �浹ü

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

	/* ��� ���� �Ӽ� */
	SOCKET my_socket;
	SOCKADDR_IN	my_address;

	/* ���� ������ ���� �Ӽ� */
	WSAOVERLAPPED io_behavior;
	HANDLE thread_game_starter;
	HANDLE thread_game_process;
	HANDLE event_player_accept; // �÷��̾� ������ �޴� �̺�Ʈ ��ü
	HANDLE event_game_start; // ���� ������ �ϴ� �̺�Ʈ ��ü
	HANDLE event_receives; // �÷��̾��� �Է��� �޴� �̺�Ʈ ��ü
	HANDLE event_game_process; // �浹 ó���� �ϴ� �̺�Ʈ ��ü
	HANDLE event_send_renders; // ������ ������ ������ �̺�Ʈ ��ü
	int my_process_index; // ���� ó�� ���� �÷��̾��� ���� [0~client_number)

	/* �÷��̾� ���� �Ӽ� */
	vector<PlayerInfo*> players; // �÷��̾� ���
	int	client_number; // ���� ������ �÷��̾��� ��
	int player_number_last; // �������� �߰��� �÷��̾��� ��ȣ
	int	player_captain; // ���� �÷��̾�
	int player_winner; // �¸��� �÷��̾�

	/* ���� ���� �Ӽ� */
	vector<GameInstance*> instances; // �ν��Ͻ� ���
	normal_distribution<> random_distrubution; // ������ ������ ���� ����
	default_random_engine randomizer;

	const int WORLD_W, WORLD_H;
	int** PLAYER_SPAWN_PLACES; // �÷��̾ �� ó���� ������ ��ġ�� �迭
	const int SPAWN_DISTANCE;

	RenderInstance* rendering_infos_last; // ������ ������ ����
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
