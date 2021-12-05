#pragma once
#include "stdafx.h"
#include "CommonDatas.h"
#include "GameInstance.h"



/* 소켓 */
SOCKET my_socket; // 서버 소켓
SOCKADDR_IN my_address; // 서버 주소
int my_address_size = sizeof(my_address);

/* 다중 스레드 정보 */
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
vector<GameInstance*> instances; // 인스턴스 목록
normal_distribution<> random_distrubution; // 서버의 무작위 분포 범위
default_random_engine randomizer;

bool game_started;
const int WORLD_W = 1280, WORLD_H = 1280;
int** PLAYER_SPAWN_PLACES; // 플레이어가 맨 처음에 생성될 위치의 배열
const int SPAWN_DISTANCE = 300; // 플레이어 생성 위치를 정할 때 사용하는 거리 값

/* 스레드 선언 */
DWORD WINAPI ConnectProcess(LPVOID arg); // 다중, 수신 스레드
DWORD WINAPI GameProcess(LPVOID arg); // 단일, 송신 스레드

void Initialize();
void Startup();
void Ready();
void Cleanup();

void ClientConnect();
void ClientDisconnect(int player_index);

// 정해둔 스폰 지점에 플레이어 캐릭터들을 생성한다.
void CreatePlayerCharacters();

void ProceedContinuation(); // 게임 진행 확인
bool CheckClientNumber(); // 접속한 클라이언트 수 확인
bool ValidateSocketMessage(int socket_state); // 받은 소켓 메시지 검증
void BakeRenderingInfos(); // 렌더링 정보 만들기
void SendRenderingInfos(SOCKET my_socket); // 렌더링 정보 보내기

inline DWORD WINAPI AwaitClientAcceptEvent() {
	AtomicPrintLn("AwaitClientAcceptEvent()");
	return WaitForSingleObject(event_accept, INFINITE);
}

inline DWORD WINAPI AwaitReceiveEvent() {
	AtomicPrintLn("AwaitReceiveEvent()");
	return WaitForSingleObject(event_game_communicate, INFINITE);
}

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

// 지정한 위치에 인스턴스를 생성한다.
template<class _GameClass = GameInstance>
_GameClass* Instantiate(double x = 0.0, double y = 0.0) {
	auto result = new _GameClass();
	result->x = x;
	result->y = y;

	instances.push_back(result);

	return result;
}

// 지정한 인스턴스를 삭제한다.
template<class _GameClass = GameInstance>
void Kill(_GameClass* target) {
	auto loc = find_if(instances.begin(), instances.end(), [target] (const auto& lhs) {
		return (lhs == target);
	});

	if (loc != instances.end()) {
		target->OnDestroy();
		instances.erase(loc);
	}
}

// 두 게임 인스턴스의 충돌을 검사한다.
template<class _GameClass1, class _GameClass2>
inline _GameClass2* CheckCollision(_GameClass1* self, _GameClass2* other) {
	if (self && other && self != other) {
		if (self->IsCollideWith(other))
			return other;
	}
	return nullptr;
}

// 어떤 게임 인스턴스에게 충돌하는 인스턴스를, 식별자 fid를 기반으로 찾아낸다.
template<class _GameClassTarget, class _GameClassSelf>
_GameClassTarget* SeekCollision(_GameClassSelf* self, const char* fid) {
	if (self && !instances.empty()) {
		auto CopyList = vector<GameInstance*>(instances);

		auto it = std::find_if(CopyList.begin(), CopyList.end(), [&] (GameInstance* inst) {
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

// cout으로 출력하기
template<typename Ty>
void AtomicPrint(Ty caption) {
	cout << caption;
}

// 여러 개의 값을 함수 하나로 cout으로 출력하기
template<typename Ty1, typename... Ty2>
void AtomicPrint(Ty1 caption, Ty2... args) {
	AtomicPrint(caption);
	AtomicPrint(args...);
}

// cout으로 출력하고 한줄 띄우기
template<typename... Ty>
void AtomicPrintLn(Ty... args) {
	AtomicPrint(args..., "\n");
}
