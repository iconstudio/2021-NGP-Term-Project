#ifndef _SERVER_FRAMEWORK
#define _SERVER_FRAMEWORK

class ServerFramework
{
public:
	ServerFramework();
	~ServerFramework();
public:
	bool Initialize();			 // 서버 초기화
public:
	void CreatePlayer();													// 플레이어 생성
	void AddPlayer(ClientSession* player) { players.push_back(player); }	// 플레이어 추가
	void CreateRenderingInfos();											// 렌더링 정보 생성
	void SendRenderingInfos(SOCKET client_socket);							// 렌더링 정보 전송
public:
	void ProceedContinuation();							// 게임 진행 확인
	void ValidateSocketMessage(int socket_state);		// 받은 ㅅ켓 메시지 검증
public:
	// 지정한 위치에 인스턴스를 생성한다.
	template<class _GameClass = GameInstance>
	_GameClass* Instantiate(double x = 0.0, double y = 0.0);

	// 지정한 인스턴스를 삭제한다.
	template<class _GameClass = GameInstance>
	void Kill(_GameClass* target);

	// 두 게임 인스턴스의 충돌을 검사한다.
	template<class _GameClass1, class _GameClass2>
	inline _GameClass2* CheckCollision(_GameClass1* self, _GameClass2* other);

	// 어떤 게임 인스턴스에게 충돌하는 인스턴스를, 식별자 fid를 기반으로 찾아낸다.
	template<class _GameClassTarget, class _GameClassSelf>
	_GameClassTarget* SeekCollision(_GameClassSelf* self, const char* fid);
public:
	void SetConnectProcess() { SetEvent(event_accept); }			// accept 이벤트 활성화
	void SetGameProcess() { SetEvent(event_game_process); }			// 게임 프로세스 이벤트 활성화
public:
	SOCKET GetListenSocket() { return s_socket; }
	HANDLE GetAcceptEvent() { return event_accept; }
	HANDLE GetGameProcessEvent() { return event_game_process; }
	int GetPlayerNumber() { return players.size(); }
public:
	friend DWORD WINAPI ConnectProcess(LPVOID arg);
	friend DWORD WINAPI GameProcess(LPVOID arg);

private:
	SOCKET s_socket;					 // 서버 소켓
	sockaddr_in s_address;				 // 서버 주소
	u_short s_port;						 // 서버 포트번호
private:
	HANDLE event_accept;				 // 클라이언트 수용 이벤트
	HANDLE event_game_process;			 // 게임 처리 이벤트
private:
	vector<ClientSession*> players;		 // 플레이어 목록
	int player_process_index;			 // 현재 처리 중인 플레이어의 순번 [0~client_number)
	int player_number_last;				 // 마지막에 추가된 플레이어의 번호
	int player_captain;					 // 방장 플레이어
	int player_winner;					 // 승리 플레이어
private:
	vector<GameInstance*> instances;	 // 인스턴스 목록
	vector<RenderInstance> rendering_infos_last;		// 렌더링 정보
private:
	bool game_started;
};

#endif

template<class _GameClass1, class _GameClass2>
inline _GameClass2* CheckCollision(_GameClass1* self, _GameClass2* other) {
	if (self && other && self != other) {
		if (self->IsCollideWith(other))
			return other;
	}
	return nullptr;
}