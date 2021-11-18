#include "pch.h"
#include "CommonDatas.h"


int SendMyMessage(SOCKET sk, PACKETS type, int size, void* data) {
	PACKETS packet = type;

	if (0 < size) {
		int result = send(sk, reinterpret_cast<char*>(&packet), sizeof(packet), 0);
		send(sk, reinterpret_cast<char*>(data), sizeof(data), 0);

		return result;
	} else {
		return send(sk, reinterpret_cast<char*>(&packet), sizeof(packet), 0);
	}
}
