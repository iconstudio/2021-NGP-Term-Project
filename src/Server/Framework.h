#pragma once
#include "stdafx.h"
#include "CommonDatas.h"
#include "GameInstance.h"


/* 스레드 선언 */
DWORD WINAPI ConnectProcess(LPVOID arg); // 다중, 수신 스레드
DWORD WINAPI GameProcess(LPVOID arg); // 단일, 송신 스레드

class CCharacter : public GameInstance {
public:
	CCharacter();

	virtual void OnUpdate(double frame_advance);

	virtual const char* GetIdentifier() const;

	void GetHurt(int dmg);
	void Die();

	double health;
	double attack_cooltime;
	double inv_time;
};

class CBullet : public GameInstance {
public:
	CBullet();

	virtual void OnUpdate(double frame_advance);

	virtual const char* GetIdentifier() const;

	double lifetime;
};

class ClientSession {
public:
	SOCKET my_socket;
	HANDLE my_thread;

	int player_index; // 플레이어 번호
	CCharacter* player_character;

	ClientSession(SOCKET sk, HANDLE th, int id);
	~ClientSession();
};

class ServerFramework {
public:
	ServerFramework();
	~ServerFramework();

	void Initialize(); // 서버 초기화
	void Startup(); // 서버 구동
	void GameReady(); // 게임 준비

	SOCKET AcceptClient();
	void ConnectClient(SOCKET client_socket); // 플레이어 접속
	void DisconnectClient(ClientSession* client); // 플레이어 종료

	void CreatePlayerCharacters(); // 플레이어 생성

	void ProceedContinuation();							// 게임 진행 확인
	void ValidateSocketMessage(int socket_state);		// 받은 소켓 메시지 검증
	void CreateRenderingInfos();											// 렌더링 정보 생성
	void SendRenderingInfos(SOCKET client_socket);							// 렌더링 정보 전송

	void SetConnectProcess();			// accept 이벤트 활성화
	void SetGameProcess();			    // 게임 프로세스 이벤트 활성화
	const int GetPlayerNumber() const;
	
	inline DWORD WINAPI AwaitClientAcceptEvent();
	inline DWORD WINAPI AwaitReceiveEvent();
	inline DWORD WINAPI AwaitQuitEvent();

	friend DWORD WINAPI ConnectProcess(LPVOID arg);
	friend DWORD WINAPI GameProcess(LPVOID arg);

	// 지정한 위치에 인스턴스를 생성한다.
	template<class _GameClass = GameInstance>
	_GameClass* Instantiate(double x = 0.0, double y = 0.0);

	// 지정한 인스턴스를 삭제한다.
	template<class _GameClass = GameInstance>
	void Kill(_GameClass* target);

	// 두 게임 인스턴스의 충돌을 검사한다.
	template<class _GameClass1, class _GameClass2>
	inline _GameClass2* CheckCollision(_GameClass1* self, _GameClass2* other);

	// 어떤 게임 인스턴스에게 충돌하는 인스턴스를, 식별자 fid를 기반으로 찾아낸다.
	template<class _GameClassTarget, class _GameClassSelf>
	_GameClassTarget* SeekCollision(_GameClassSelf* self, const char* fid);

private:
	/* 소켓 속성 */
	SOCKET my_socket; // 서버 소켓
	SOCKADDR_IN my_address; // 서버 주소
	int my_address_size = sizeof(my_address);

	/* 다중 스레드 속성 */
	HANDLE event_accept; // 클라이언트 수용 신호
	HANDLE event_game_communicate; // 게임 처리 신호
	HANDLE event_quit; // 종료 신호
	CRITICAL_SECTION permission_client, permission_;

	/* 플레이어 관련 속성 */
	vector<ClientSession*> players; // 플레이어 목록
	int player_process_index; // 현재 처리 중인 플레이어의 순번 [0~client_number)
	int	players_number; // 지금 접속한 플레이어의 수
	int player_number_last; // 마지막에 추가된 플레이어의 번호
	int	player_captain; // 방장 플레이어
	int player_winner; // 승리한 플레이어

	/* 게임 관련 속성 */
	bool game_started;
	int** PLAYER_SPAWN_PLACES; // 플레이어가 맨 처음에 생성될 위치의 배열
	const int SPAWN_DISTANCE = 300; // 플레이어 생성 위치를 정할 때 사용하는 거리 값

private:
	vector<GameInstance*> instances;	 // 인스턴스 목록
	vector<RenderInstance> rendering_infos_last;		// 렌더링 정보
};

inline const int ServerFramework::GetPlayerNumber() const {
	return players.size();
}

inline DWORD WINAPI ServerFramework::AwaitClientAcceptEvent() {
	AtomicPrintLn("AwaitClientAcceptEvent()");
	return WaitForSingleObject(event_accept, INFINITE);
}

inline DWORD WINAPI ServerFramework::AwaitReceiveEvent() {
	AtomicPrintLn("AwaitReceiveEvent()");
	return WaitForSingleObject(event_game_communicate, INFINITE);
}

inline DWORD __stdcall ServerFramework::AwaitQuitEvent() {
	AtomicPrintLn("AwaitQuitEvent()");
	return WaitForSingleObject(event_quit, INFINITE);
}

template<class _GameClass>
_GameClass* ServerFramework::Instantiate(double x, double y) {
	auto result = new _GameClass();
	result->x = x;
	result->y = y;

	instances.push_back(result);

	return result;
}

template<class _GameClass>
void ServerFramework::Kill(_GameClass* target) {
	auto loc = find_if(instances.begin(), instances.end(), [target](const auto& lhs) {
		return (lhs == target);
		});

	if (loc != instances.end()) {
		target->OnDestroy();
		instances.erase(loc);
	}
}

template<class _GameClass1, class _GameClass2>
inline _GameClass2* ServerFramework::CheckCollision(_GameClass1* self, _GameClass2* other) {
	if (self && other && self != other) {
		if (self->IsCollideWith(other))
			return other;
	}
	return nullptr;
}

template<class _GameClassTarget, class _GameClassSelf>
_GameClassTarget* ServerFramework::SeekCollision(_GameClassSelf* self, const char* fid) {
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
