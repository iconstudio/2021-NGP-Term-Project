#pragma once
#include "pch.h"


// 최대 플레이어 수
const int PLAYERS_NUMBER_MAX = 10;

// 프레임 수
const int FRAMERATE = 60;
const double FRAME_TIME = (1.0 / FRAMERATE);

enum PACKETS : int {
	CLIENT_PING = 0				// 빈 패킷을 보낼 때 사용하는 메시지
	, CLIENT_KEY_INPUT			// 입력을 보낼 때 사용하는 메시지
	, CLIENT_GAME_START			// 서버에게 게임 시작을 요청하는 메시지
	, CLIENT_PLAY_CONTINUE		// 게임을 다시 시작하기 위해 재접속을 요청하는 메시지
	, CLIENT_PLAY_DENY			// 게임을 다시하지 않는다고 알려주는 메시지

	, SERVER_GAME_START			// 클라이언트에게 게임이 시작되었음을 알려주는 메시지
	, SERVER_PLAYER_COUNT		// 클라이언트에게 플레이어가 몇 명인지 알려주는 메시지
	, SERVER_GAME_STATUS		// 클라이언트에게 게임 상태를 알려주는 메시지
	, SERVER_RENDER_INFO		// 클라이언트에게 렌더링 정보를 보내주는 메시지
	, SERVER_GAME_DONE			// 클라이언트에게 게임이 끝났음을 알려주는 메시지
	, SERVER_REPLAY				// 클라이언트에게 게임을 다시 시작함을 알려주는 메시지
};

struct PacketMessage {
	const PACKETS type;
};

struct GameUpdateMessage {
	int players_count;

	int target_player;
	int player_hp;
	double player_x, player_y, player_direction;
};

enum RENDER_TYPES : int {
	CHARACTER = 0
	, BULLET
};

struct RenderInstance {
	const RENDER_TYPES instance_type;

	int sprite_index;
	double x, y, angle;
};

struct GameInput {
	WPARAM button;
};
