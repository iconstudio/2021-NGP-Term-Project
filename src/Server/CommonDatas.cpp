#include "CommonDatas.h"


int WINAPI RecvData(SOCKET client_socket, PACKETS*&, char*& data_output, int* data_output_size) {
	PACKETS header;
	ZeroMemory(&header, HEADER_SIZE);

	int result = recv(client_socket, reinterpret_cast<char*>(&header), HEADER_SIZE, 0);
	if (SOCKET_ERROR == result) {
		return result;
	} else if (0 == result) {
		return result;
	}

	char* client_data = nullptr;
	int client_data_size = 0;

	// 1-2. 패킷 내용 수신
	switch (header) {
		case PACKETS::CLIENT_KEY_INPUT:
		{
			client_data = new char[SEND_INPUT_COUNT];
			client_data_size = SEND_INPUT_COUNT;
			ZeroMemory(&client_data, client_data_size);
			
			result = recv(client_socket, client_data, client_data_size, 0);
			if (SOCKET_ERROR == result) {
				return result;
			} else if (0 == result) {
				return result;
			}
		}
		break;

		default: break;
	}

	return result;
}

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
	int error = WSAGetLastError();

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, error,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPTSTR>(&lpMsgBuf), 0, nullptr);


	std::cout << "[" << msg << "] " << static_cast<char*>(lpMsgBuf) << std::endl;

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
