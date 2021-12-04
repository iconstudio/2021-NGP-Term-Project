#ifndef _SERVER_FRAMEWORK
#define _SERVER_FRAMEWORK

class ServerFramework
{
public:
	ServerFramework() : s_port(15000), cl_addr_len(sizeof(cl_addr)) {}
public:
	int Initialize();			 // 서버 초기화
	bool Connect();				 // 클라이언트 연결
	void Disconnect();			 // 클라이언트 연결 해제
	void Close();				 // 서버 종료
public:
	void RecieveKeyInput();
public:
	SOCKET GetServerSocket() { return s_socket; }
	sockaddr_in GetClientAddr() { return cl_addr; }

private:
	SOCKET		 s_socket;		  // 서버 소켓
	sockaddr_in	 s_address;		  // 서버 주소
	u_short		 s_port;		  // 서버 포트번호
private:
	SOCKET		 cl_socket;		  // 클라이언트 소켓
	sockaddr_in	 cl_addr;		  // 클라이언트 주소
	int			 cl_addr_len;	  // 클라이언트 주소 길이
private:
	std::vector<char> key_input;
};

#endif