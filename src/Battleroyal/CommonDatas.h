#pragma once
#include "pch.h"

#define COMMON_PORT 15000
#define GAME_SCENE_W 1280
#define GAME_SCENE_H 1280
#define CLIENT_W 960
#define CLIENT_H 540
#define VIEW_W 320
#define VIEW_H 240
#define PORT_W 640
#define PORT_H 480


constexpr double METER_TO_PIXELS = 16.;
constexpr double HOUR_TO_SECONDS = 3600.;
constexpr double KPH_TO_PPS = (1000.0 * METER_TO_PIXELS / HOUR_TO_SECONDS);

constexpr double km_per_hr(const double velocity) {
	return velocity * KPH_TO_PPS;
}

const double EWALL_BEGIN_TIME = 30.0;				// 자기장 시작 시간
const double EWALL_CLOSE_PERIOD = 300.0;			// 자기장 완료 시간
const double EWALL_DAMAGE_PER_SECOND = 0.7;

const int PLAYERS_NUMBER_MAX = 10;					// 최대 플레이어 수
const double PLAYER_HEALTH = 100.0;					// 플레이어 최대 체력
const double PLAYER_MOVE_SPEED = km_per_hr(20);		// 플레이어 이동 속도
const double PLAYER_ATTACK_COOLDOWN = 0.2;			// 공격 쿨 타임
const double PLAYER_INVINCIBLE_DURATION = 2.5;		// 무적 시간

const double SNOWBALL_DURATION = 0.6;				// 투사체 지속 시간
const double SNOWBALL_VELOCITY = km_per_hr(50);		// 투사체 이동 속도

// 프레임 수
const int FRAMERATE = 60;
const double FRAME_TIME = (1.0 / FRAMERATE);

// 송수신 설정
const int SEND_INPUT_COUNT = 6;


enum PACKETS : int {
	// 클라이언트 -> 서버
	CLIENT_PING = 0				// 빈 패킷을 보낼 때 사용하는 메시지
	, CLIENT_KEY_INPUT			// 입력을 보낼 때 사용하는 메시지
	, CLIENT_GAME_START			// 서버에게 게임 시작을 요청하는 메시지
	, CLIENT_PLAY_CONTINUE		// 게임을 다시 시작하기 위해 재접속을 요청하는 메시지
	, CLIENT_PLAY_DENY			// 게임을 다시하지 않는다고 알려주는 메시지

	// 서버 -> 클라이언트
	, SERVER_SET_CAPATIN		// 방장임을 알려주는 메시지
	, SERVER_GAME_START			// 게임이 시작되었음을 알려주는 메시지
	, SERVER_PLAYER_COUNT		// 플레이어가 몇 명인지 알려주는 메시지
	, SERVER_GAME_STATUS		// 게임 상태를 알려주는 메시지
	, SERVER_RENDER_INFO		// 렌더링 정보를 보내주는 메시지
	, SERVER_GAME_DONE			// 게임이 끝났음을 알려주는 메시지
	, SERVER_REPLAY				// 게임을 다시 시작함을 알려주는 메시지
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

enum INPUT_TYPES : int {
	NONE = 0
	, PRESS = 1
	, RELEASE = 2
};

const WPARAM keybinding[6] = { VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT, 'a', 's' };

struct InputStream {
	WPARAM code = 0;
	INPUT_TYPES type;
};

struct RenderInstance {
	RENDER_TYPES instance_type;

	int image_index;
	double x, y, angle;
};

void SendData(SOCKET, PACKETS, const char* = nullptr, int = 0);
void ErrorAbort(const char*);
void ErrorDisplay(const char*);
