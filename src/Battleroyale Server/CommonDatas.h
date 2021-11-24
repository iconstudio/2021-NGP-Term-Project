#pragma once
#include "pch.h"
#define COMMON_PORT 15000


// �ڱ��� �Ϸ� �ð�
const double EWALL_CLOSE_PERIOD = 300.0;
const double EWALL_DAMAGE_PER_SECOND = 1.5;

const int PLAYERS_NUMBER_MAX = 10;					// �ִ� �÷��̾� ��
const int PLAYER_HEALTH = 10;						// �÷��̾� �ִ� ü��
const double PLAYER_MOVE_SPEED = km_per_hr(20);		// �÷��̾� �̵� �ӵ�
const double PLAYER_ATTACK_COOLDOWN = 0.2;			// ���� �� Ÿ��
const double PLAYER_INVINCIBLE_DURATION = 2.5;		// ���� �ð�

const double SNOWBALL_DURATION = 0.6;				// ����ü ���� �ð�
const double SNOWBALL_VELOCITY = km_per_hr(50);		// ����ü �̵� �ӵ�

// ������ ��
const int FRAMERATE = 20;
const double FRAME_TIME = (1.0 / FRAMERATE);

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

struct RenderInstance {
	RENDER_TYPES instance_type;

	int image_index;
	double x, y, angle;
};

void SendData(SOCKET, PACKETS, const char* = nullptr, int = 0);
void ErrorAbort(const char*);
void ErrorDisplay(const char*);
