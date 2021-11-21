#pragma once
#include "pch.h"
#define COMMON_PORT 15000


// �ڱ��� �Ϸ� �ð�
const double EWALL_CLOSE_PERIOD = 300.0;
const double EWALL_DAMAGE_PER_SECOND = 1.5;

// �ִ� �÷��̾� ��
const int PLAYERS_NUMBER_MAX = 10;

// ������ ��
const int FRAMERATE = 100;
const double FRAME_TIME = (1.0 / FRAMERATE);

enum PACKETS : int {
	CLIENT_PING = 0				// �� ��Ŷ�� ���� �� ����ϴ� �޽���
	, CLIENT_KEY_INPUT			// �Է��� ���� �� ����ϴ� �޽���
	, CLIENT_GAME_START			// �������� ���� ������ ��û�ϴ� �޽���
	, CLIENT_PLAY_CONTINUE		// ������ �ٽ� �����ϱ� ���� �������� ��û�ϴ� �޽���
	, CLIENT_PLAY_DENY			// ������ �ٽ����� �ʴ´ٰ� �˷��ִ� �޽���

	, SERVER_SET_CAPATIN		// Ŭ���̾�Ʈ���� �ʰ� �����̶�� �˷��ִ� �޽���
	, SERVER_GAME_START			// Ŭ���̾�Ʈ���� ������ ���۵Ǿ����� �˷��ִ� �޽���
	, SERVER_PLAYER_COUNT		// Ŭ���̾�Ʈ���� �÷��̾ �� ������ �˷��ִ� �޽���
	, SERVER_GAME_STATUS		// Ŭ���̾�Ʈ���� ���� ���¸� �˷��ִ� �޽���
	, SERVER_RENDER_INFO		// Ŭ���̾�Ʈ���� ������ ������ �����ִ� �޽���
	, SERVER_GAME_DONE			// Ŭ���̾�Ʈ���� ������ �������� �˷��ִ� �޽���
	, SERVER_REPLAY				// Ŭ���̾�Ʈ���� ������ �ٽ� �������� �˷��ִ� �޽���
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
	RENDER_TYPES instance_type;		//���� �̹�������

	int image_index;				//�ִϸ��̼� ������
	double x, y, angle;				//��ǥ, ����
};

void SendData(SOCKET, PACKETS, const char* = nullptr, int = 0);
void ErrorAbort(const char*);
void ErrorDisplay(const char*);
