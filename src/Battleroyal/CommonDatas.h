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

const double EWALL_BEGIN_TIME = 30.0;				// �ڱ��� ���� �ð�
const double EWALL_CLOSE_PERIOD = 300.0;			// �ڱ��� �Ϸ� �ð�
const double EWALL_DAMAGE_PER_SECOND = 0.7;

const int PLAYERS_NUMBER_MAX = 10;					// �ִ� �÷��̾� ��
const double PLAYER_HEALTH = 100.0;					// �÷��̾� �ִ� ü��
const double PLAYER_MOVE_SPEED = km_per_hr(20);		// �÷��̾� �̵� �ӵ�
const double PLAYER_ATTACK_COOLDOWN = 0.2;			// ���� �� Ÿ��
const double PLAYER_INVINCIBLE_DURATION = 2.5;		// ���� �ð�

const double SNOWBALL_DURATION = 0.6;				// ����ü ���� �ð�
const double SNOWBALL_VELOCITY = km_per_hr(50);		// ����ü �̵� �ӵ�

// ������ ��
const int FRAMERATE = 60;
const double FRAME_TIME = (1.0 / FRAMERATE);

// �ۼ��� ����
const int SEND_INPUT_COUNT = 6;


enum PACKETS : int {
	// Ŭ���̾�Ʈ -> ����
	CLIENT_PING = 0				// �� ��Ŷ�� ���� �� ����ϴ� �޽���
	, CLIENT_KEY_INPUT			// �Է��� ���� �� ����ϴ� �޽���
	, CLIENT_GAME_START			// �������� ���� ������ ��û�ϴ� �޽���
	, CLIENT_PLAY_CONTINUE		// ������ �ٽ� �����ϱ� ���� �������� ��û�ϴ� �޽���
	, CLIENT_PLAY_DENY			// ������ �ٽ����� �ʴ´ٰ� �˷��ִ� �޽���

	// ���� -> Ŭ���̾�Ʈ
	, SERVER_SET_CAPATIN		// �������� �˷��ִ� �޽���
	, SERVER_GAME_START			// ������ ���۵Ǿ����� �˷��ִ� �޽���
	, SERVER_PLAYER_COUNT		// �÷��̾ �� ������ �˷��ִ� �޽���
	, SERVER_GAME_STATUS		// ���� ���¸� �˷��ִ� �޽���
	, SERVER_RENDER_INFO		// ������ ������ �����ִ� �޽���
	, SERVER_GAME_DONE			// ������ �������� �˷��ִ� �޽���
	, SERVER_REPLAY				// ������ �ٽ� �������� �˷��ִ� �޽���
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
