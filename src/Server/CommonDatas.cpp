#include "CommonDatas.h"


int WINAPI SendData(SOCKET socket, PACKETS type, const char* buffer, int length) {
	int result = send(socket, reinterpret_cast<char*>(&type), sizeof(PACKETS), 0);
	if (SOCKET_ERROR == result) {
		ErrorDisplay("send 1");
		return SOCKET_ERROR;
	}

	if (buffer) {
		result = send(socket, buffer, length, 0);
		if (SOCKET_ERROR == result) {
			ErrorDisplay("send 2");
			return SOCKET_ERROR;
		}
	}
	return result;
}


void ErrorAbort(const char* msg) {
	LPVOID lpMsgBuf;
	int error{ WSAGetLastError() };

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, error,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPTSTR>(&lpMsgBuf), 0, nullptr);

	MessageBox(nullptr, static_cast<LPCTSTR>(lpMsgBuf), reinterpret_cast<LPCTSTR>(msg), MB_ICONERROR);

	LocalFree(lpMsgBuf);
	exit(error);
}

void ErrorDisplay(const char* msg) {
	LPVOID lpMsgBuf;

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPTSTR>(&lpMsgBuf), 0, nullptr);

	std::cout << "[" << msg << "] " << static_cast<char*>(lpMsgBuf) << std::endl;

	LocalFree(lpMsgBuf);
}
