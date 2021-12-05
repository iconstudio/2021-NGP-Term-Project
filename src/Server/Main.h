#pragma once
#include "stdafx.h"
#include "CommonDatas.h"
#include "GameInstance.h"

CRITICAL_SECTION client_permission, print_permission;

class CCharacter : public GameInstance {
public:
	CCharacter();

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
normal_distribution<> random_distrubution; // 서버의 무작위 분포 범위
default_random_engine randomizer;

const int WORLD_W = 1280, WORLD_H = 1280;
int** PLAYER_SPAWN_PLACES; // 플레이어가 맨 처음에 생성될 위치의 배열
const int SPAWN_DISTANCE = 300; // 플레이어 생성 위치를 정할 때 사용하는 거리 값

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
