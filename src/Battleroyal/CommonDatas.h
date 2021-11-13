#pragma once
#include "pch.h"


// �ִ� �÷��̾� ��
const int PLAYERS_NUMBER_MAX = 10;

// ������ ��
const int FRAMERATE = 60;
const double FRAME_TIME = (1.0 / FRAMERATE);

enum PACKETS : int {
	CLIENT_PING = 0				// �� ��Ŷ�� ���� �� ����ϴ� �޽���
	, CLIENT_KEY_INPUT			// �Է��� ���� �� ����ϴ� �޽���
	, CLIENT_GAME_START			// �������� ���� ������ ��û�ϴ� �޽���
	, CLIENT_PLAY_CONTINUE		// ������ �ٽ� �����ϱ� ���� �������� ��û�ϴ� �޽���
	, CLIENT_PLAY_DENY			// ������ �ٽ����� �ʴ´ٰ� �˷��ִ� �޽���

	, SERVER_GAME_START			// Ŭ���̾�Ʈ���� ������ ���۵Ǿ����� �˷��ִ� �޽���
	, SERVER_PLAYER_COUNT		// Ŭ���̾�Ʈ���� �÷��̾ �� ������ �˷��ִ� �޽���
	, SERVER_GAME_STATUS		// Ŭ���̾�Ʈ���� ���� ���¸� �˷��ִ� �޽���
	, SERVER_RENDER_INFO		// Ŭ���̾�Ʈ���� ������ ������ �����ִ� �޽���
	, SERVER_GAME_DONE			// Ŭ���̾�Ʈ���� ������ �������� �˷��ִ� �޽���
	, SERVER_REPLAY				// Ŭ���̾�Ʈ���� ������ �ٽ� �������� �˷��ִ� �޽���
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
