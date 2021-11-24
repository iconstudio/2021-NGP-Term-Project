#pragma once
#include "pch.h"
#define COMMON_PORT 15000


// ?�기???�료 ?�간
const double EWALL_CLOSE_PERIOD = 300.0;
const double EWALL_DAMAGE_PER_SECOND = 1.5;

// 최�? ?�레?�어 ??
const int PLAYERS_NUMBER_MAX = 10;

// ?�레????
const int FRAMERATE = 100;
const double FRAME_TIME = (1.0 / FRAMERATE);

enum PACKETS : int {
	CLIENT_PING = 0				// �??�킷??보낼 ???�용?�는 메시지
	, CLIENT_KEY_INPUT			// ?�력??보낼 ???�용?�는 메시지
	, CLIENT_GAME_START			// ?�버?�게 게임 ?�작???�청?�는 메시지
	, CLIENT_PLAY_CONTINUE		// 게임???�시 ?�작?�기 ?�해 ?�접?�을 ?�청?�는 메시지
	, CLIENT_PLAY_DENY			// 게임???�시?��? ?�는?�고 ?�려주는 메시지

	, SERVER_SET_CAPATIN		// ?�라?�언?�에�??��? 방장?�라�??�려주는 메시지
	, SERVER_GAME_START			// ?�라?�언?�에�?게임???�작?�었?�을 ?�려주는 메시지
	, SERVER_PLAYER_COUNT		// ?�라?�언?�에�??�레?�어가 �?명인지 ?�려주는 메시지
	, SERVER_GAME_STATUS		// ?�라?�언?�에�?게임 ?�태�??�려주는 메시지
	, SERVER_RENDER_INFO		// ?�라?�언?�에�??�더�??�보�?보내주는 메시지
	, SERVER_GAME_DONE			// ?�라?�언?�에�?게임???�났?�을 ?�려주는 메시지
	, SERVER_REPLAY				// ?�라?�언?�에�?게임???�시 ?�작?�을 ?�려주는 메시지
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
