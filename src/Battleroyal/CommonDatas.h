#pragma once
#include "pch.h"
#define COMMON_PORT 15000


// 자기장 완료 시간
const double EWALL_CLOSE_PERIOD = 300.0;
const double EWALL_DAMAGE_PER_SECOND = 1.5;

// 최대 플레이어 수
const int PLAYERS_NUMBER_MAX = 10;

// 프레임 수
const int FRAMERATE = 100;
const double FRAME_TIME = (1.0 / FRAMERATE);

enum PACKETS : int {
	CLIENT_PING = 0				// 빈 패킷을 보낼 때 사용하는 메시지
	, CLIENT_KEY_INPUT			// 입력을 보낼 때 사용하는 메시지
	, CLIENT_GAME_START			// 서버에게 게임 시작을 요청하는 메시지
	, CLIENT_PLAY_CONTINUE		// 게임을 다시 시작하기 위해 재접속을 요청하는 메시지
	, CLIENT_PLAY_DENY			// 게임을 다시하지 않는다고 알려주는 메시지

	, SERVER_SET_CAPATIN		// 클라이언트에게 너가 방장이라고 알려주는 메시지
	, SERVER_GAME_START			// 클라이언트에게 게임이 시작되었음을 알려주는 메시지
	, SERVER_PLAYER_COUNT		// 클라이언트에게 플레이어가 몇 명인지 알려주는 메시지
	, SERVER_GAME_STATUS		// 클라이언트에게 게임 상태를 알려주는 메시지
	, SERVER_RENDER_INFO		// 클라이언트에게 렌더링 정보를 보내주는 메시지
	, SERVER_GAME_DONE			// 클라이언트에게 게임이 끝났음을 알려주는 메시지
	, SERVER_REPLAY				// 클라이언트에게 게임을 다시 시작함을 알려주는 메시지
};

struct GameUpdateMessage {
	int players_count;

	int target_player;
	double player_hp;
	double player_x, player_y, player_direction;
};

enum RENDER_TYPES : int {
	CHARACTER = 0
	, BULLET
};

struct RenderInstance {
	RENDER_TYPES instance_type;		//무슨 이미지인지

	int image_index;				//애니메이션 프레임
	double x, y, angle;				//좌표, 각도
};

void SendData(SOCKET, PACKETS, const char* = nullptr, int = 0);
void ErrorAbort(const char*);
void ErrorDisplay(const char*);
