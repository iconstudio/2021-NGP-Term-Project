﻿#pragma once
#include "stdafx.h"
#include "CommonDatas.h"
#include "GameInstance.h"


CRITICAL_SECTION client_permission, print_permission;

class CCharacter : public GameInstance {
public:
	virtual const char* GetIdentifier() const;
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

/* 게임 관련 속성 */
vector<GameInstance*> instances; // 인스턴스 목록
normal_distribution<> random_distrubution; // 서버의 무작위 분포 범위
default_random_engine randomizer;

// 지정한 위치에 인스턴스를 생성한다.
template<class _GameClass = GameInstance>
_GameClass* Instantiate(double x = 0.0, double y = 0.0) {
	auto result = new _GameClass();
	result->x = x;
	result->y = y;

	instances.push_back(result);

	return result;
}

// 지정한 포인터의 개체를 삭제한다.
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

// 정해둔 스폰 지점에 플레이어 캐릭터들을 생성한다.
template<class _GamePlayerClass>
void CreatePlayerCharacters();

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

void ClientConnect();
void ClientDisconnect(int player_index);

void ProceedContinuation(); // 게임 진행 확인
bool CheckClientNumber(); // 접속한 클라이언트 수 확인
bool ValidateSocketMessage(int socket_state); // 받은 소켓 메시지 검증
void BakeRenderingInfos(); // 렌더링 정보 만들기
void SendRenderingInfos(SOCKET my_socket); // 렌더링 정보 보내기

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

// 한줄 띄우고 cout으로 출력하기
template<typename... Ty>
void AtomicPrintLn(Ty... args) {
	AtomicPrint(args..., "\n");
}
