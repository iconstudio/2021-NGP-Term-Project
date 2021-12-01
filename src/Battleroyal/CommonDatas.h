#pragma once
#include "pch.h"


//#define SERVER_IP "192.168.120.35"
#define SERVER_IP "127.0.0.1"
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

const double PLAYER_HEALTH = 100.0;					// �÷��̾� �ִ� ü��
const double PLAYER_MOVE_SPEED = km_per_hr(20);		// �÷��̾� �̵� �ӵ�
const double PLAYER_ATTACK_COOLDOWN = 0.2;			// ���� �� Ÿ��
const double PLAYER_INVINCIBLE_DURATION = 2.5;		// ���� �ð�
const double PLAYER_BLINK_DISTANCE = 64.0;			// �÷��̾� ���� �Ÿ�

const double SNOWBALL_DURATION = 0.6;				// ����ü ���� �ð�
const double SNOWBALL_SPEED = km_per_hr(50);		// ����ü �̵� �ӵ�

// ������ ��
constexpr int FRAMERATE = 20;
constexpr double FRAME_TIME = (1.0 / FRAMERATE);

/* �ۼ��� ���� */
const int SEND_INPUT_COUNT = 6;
const int CLIENT_NUMBER_MAX = 10; // �ִ� �÷��̾� ��
const int CLIENT_NUMBER_MIN = 2;
const int RENDER_INST_COUNT = 40;

constexpr int LERP_MIN = 50;
constexpr int LERP_MAX = 200;

/* ���� ������ ���� */
constexpr int WAIT_FOR_INPUTS_PERIOD = LERP_MIN + FRAME_TIME * 1000;


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
	RELEASED = 0		// 0000(2)
	, RELEASED_NOW = 2	// 0010(2)
	, PRESSED = 4		// 0100(2)
	, PRESSED_NOW = 12	// 1100(2)
};

struct InputStream {
	int code = 0;
	INPUT_TYPES type;
};

struct RenderInstance {
	RENDER_TYPES instance_type;

	int image_index;		// �� ��° �̹����� ��������Ʈ���� �������
	double x, y, angle;		// �̹��� ȸ�� ����/����
};

void SendData(SOCKET, PACKETS, const char* = nullptr, int = 0);
void ErrorAbort(const char*);
void ErrorDisplay(const char*);
