﻿#pragma once
#include "stdafx.h"
#include "CommonDatas.h"


CRITICAL_SECTION client_permission, print_permission;

class CCharacter {
public:
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

void ClientConnect();
void ClientDisconnect(int player_index);

void ProceedContinuation();
bool CheckClientNumber();
bool ValidateSocketMessage(int socket_state);
void BakeRenderingInfos();
void SendRenderingInfos(SOCKET my_socket);

template<typename Ty>
void AtomicPrint(Ty caption) {
	cout << caption;
}

template<typename Ty1, typename... Ty2>
void AtomicPrint(Ty1 caption, Ty2... args) {
	AtomicPrint(caption);
	AtomicPrint(args...);
}

template<typename... Ty>
void AtomicPrintLn(Ty... args) {
	AtomicPrint(args..., "\n");
}
