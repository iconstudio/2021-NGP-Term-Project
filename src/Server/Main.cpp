#include "stdafx.h"
#include "Main.h"

void ErrorAbort(const char* msg);
void ErrorDisplay(const char* msg);

int main() {
	WSADATA wsa;

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != NOERROR)
	{
		return 0;
	}

	SOCKET l_socket{ socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) };

	if (l_socket == INVALID_SOCKET)
	{
		ErrorAbort("socket()");
	}

	sockaddr_in ser_addr;
	ZeroMemory(&ser_addr, sizeof(ser_addr));

	ser_addr.sin_family		 = AF_INET;
	ser_addr.sin_addr.s_addr	 = htonl(INADDR_ANY);
	ser_addr.sin_port		 = htons(static_cast<u_short>(SERVERPORT));

	if (bind(l_socket, reinterpret_cast<sockaddr*>(&ser_addr), sizeof(ser_addr)) == SOCKET_ERROR)
	{
		ErrorAbort("bind()");
	}

	if (listen(l_socket, SOMAXCONN) == SOCKET_ERROR)
	{
		ErrorAbort("listen()");
	}

	SOCKET		 cl_socket;
	sockaddr_in	 cl_addr;
	int			 cl_addr_len{ sizeof(cl_addr) };

	while (true)
	{
		cl_socket = accept(l_socket, reinterpret_cast<sockaddr*>(&cl_socket), &cl_addr_len);

		if (cl_socket == INVALID_SOCKET)
		{
			ErrorDisplay("accept()");
			break;
		}

		std::cout << "[TCP 서버] 클라이언트 접속 : IP 주소 = " << inet_ntoa(cl_addr.sin_addr)
			<< ", 포트 번호 = " << ntohs(cl_addr.sin_port) << std::endl << std::endl;

		char key_input;

		while (true)
		{
			int retval{ recv(cl_socket, reinterpret_cast<char*>(&key_input), sizeof(char), 0) };

			if (retval == SOCKET_ERROR)
			{
				ErrorDisplay("recv()");
				break;
			}
			else if (retval == 0)
			{
				break;
			}

			switch (key_input)
			{
			case VK_UP: { std::cout << "상" << std::endl; }
			break;

			case VK_DOWN: { std::cout << "하" << std::endl; }
			break;

			case VK_LEFT: { std::cout << "좌" << std::endl; }
			break;

			case VK_RIGHT: { std::cout << "우" << std::endl; }
			break;

			case VK_SPACE: { std::cout << "스페이스" << std::endl; }
			break;
			}
		}

		closesocket(cl_socket);
		std::cout << "[TCP 서버] 클라이언트 종료 : " << cl_addr.sin_addr.s_addr << std::endl;
	}

	closesocket(l_socket);
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