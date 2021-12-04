#include "stdafx.h"
#include "ServerFramework.h"

int ServerFramework::Initialize()
{
	WSADATA wsa;

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != NOERROR)
	{
		return -1;
	}

	s_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (s_socket == INVALID_SOCKET)
	{
		ErrorAbort("socket()");
	}

	ZeroMemory(&s_address, sizeof(s_address));

	s_address.sin_family		 = AF_INET;
	s_address.sin_addr.s_addr	 = htonl(INADDR_ANY);
	s_address.sin_port			 = htons(s_port);

	if (bind(s_socket, reinterpret_cast<sockaddr*>(&s_address), sizeof(s_address)) == SOCKET_ERROR)
	{
		ErrorAbort("bind()");
	}

	if (listen(s_socket, SOMAXCONN) == SOCKET_ERROR)
	{
		ErrorAbort("listen()");
	}
}

bool ServerFramework::Connect()
{
	cl_socket = accept(s_socket, reinterpret_cast<sockaddr*>(&cl_addr), &cl_addr_len);

	if (cl_socket == INVALID_SOCKET)
	{
		ErrorDisplay("accept()");
		return false;
	}

	return true;
}

void ServerFramework::Disconnect()
{
	closesocket(cl_socket);
}

void ServerFramework::Close()
{
	closesocket(s_socket);
	WSACleanup();
}
