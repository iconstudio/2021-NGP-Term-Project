#include "stdafx.h"
#include "CommonDatas.h"
#include "ServerFramework.h"
#include "Main.h"

ServerFramework::ServerFramework() : s_port(15000), players(CLIENT_NUMBER_MAX), instances(40),
player_process_index(0), player_number_last(0), player_captain(0), player_winner(0),
rendering_infos_last(RENDER_INST_COUNT), game_started(false)
{
	PLAYER_SPAWN_PLACES = new int* [CLIENT_NUMBER_MAX];

	double dir_increment = (360.0 / CLIENT_NUMBER_MAX);
	for (int i = 0; i < CLIENT_NUMBER_MAX; ++i) {
		double dir = dir_increment * i;
		int cx = static_cast<int>(WORLD_W * 0.5 + lengthdir_x(SPAWN_DISTANCE, dir));
		int cy = static_cast<int>(WORLD_W * 0.5 + lengthdir_y(SPAWN_DISTANCE, dir));

		PLAYER_SPAWN_PLACES[i] = new int[2]{ cx, cy };
	}
}

ServerFramework::~ServerFramework()
{
	CloseHandle(event_accept);
	CloseHandle(event_game_process);
	closesocket(s_socket);

	for (auto& p : players)
	{
		delete p;
	}

	for (auto& i : instances)
	{
		delete i;
	}

	for (int i = 0; i < CLIENT_NUMBER_MAX; ++i)
	{
		delete PLAYER_SPAWN_PLACES[i];
	}

	delete[] PLAYER_SPAWN_PLACES;
	players.shrink_to_fit();
	instances.shrink_to_fit();

	WSACleanup();
}

bool ServerFramework::Initialize()
{
	WSADATA wsa;

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != NOERROR)
	{
		ErrorAbort("WSAStartup()");
		return false;
	}

	s_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (s_socket == INVALID_SOCKET)
	{
		ErrorAbort("socket()");
		return false;
	}

	bool option = true;

	if (setsockopt(s_socket, SOL_SOCKET, SO_REUSEADDR,
		reinterpret_cast<char*>(&option), sizeof(option)) == SOCKET_ERROR)
	{
		ErrorAbort("setsockopt()");
		return false;
	}

	ZeroMemory(&s_address, sizeof(s_address));
	s_address.sin_family		 = AF_INET;
	s_address.sin_addr.s_addr	 = htonl(INADDR_ANY);
	s_address.sin_port			 = htons(s_port);

	if (bind(s_socket, reinterpret_cast<sockaddr*>(&s_address), sizeof(s_address)) == SOCKET_ERROR)
	{
		ErrorAbort("bind()");
		return false;
	}

	if (listen(s_socket, SOMAXCONN) == SOCKET_ERROR)
	{
		ErrorAbort("listen()");
		return false;
	}

	AtomicPrintLn("서버 시작");

	event_accept		 = CreateEvent(NULL, FALSE, TRUE, NULL);
	event_game_process	 = CreateEvent(NULL, FALSE, FALSE, NULL);
}

void ServerFramework::CreatePlayer()
{
	auto sz = players.size();
	for (int i = 0; i < sz; ++i) {
		int places[2] = { 80, 80 };//PLAYER_SPAWN_PLACES[i];
		auto character = Instantiate<CCharacter>(places[0], places[1]);

		players[i]->player_character = character;
		players[i]->player_character->owner = players[i]->player_index;
		//SendData(player->my_socket, PACKETS::SERVER_GAME_START);
	}
}

void ServerFramework::CreateRenderingInfos()
{
	if (!instances.empty()) {
		AtomicPrintLn("렌더링 정보 생성\n크기: ", instances.size());
		if (!rendering_infos_last.empty()) {
			ZeroMemory(rendering_infos_last.data(), rendering_infos_last.size());
		}

		auto CopyList = instances;

		// 플레이어 개체를 맨 위로
		std::partition(CopyList.begin(), CopyList.end(), [&](GameInstance* inst) {
			return (strcmp(inst->GetIdentifier(), "Player") == 0);
			});

		int index = 0;
		for (auto it = CopyList.begin(); it != CopyList.end(); ++it) {
			auto render_infos = (*it)->my_renders;

			// 인스턴스가 살아있는 경우에만 렌더링 메세지 전송
			if (!(*it)->dead) {
				auto dest = (rendering_infos_last.data() + index);
				auto src = &render_infos;

				memcpy(dest, src, sizeof(RenderInstance));
				index++;
			}
		}
	}
	else if (!rendering_infos_last.empty()) {
			ZeroMemory(rendering_infos_last.data(), rendering_infos_last.size());
	}
}

void ServerFramework::SendRenderingInfos(SOCKET client_socket)
{
	auto renderings = reinterpret_cast<char*>(rendering_infos_last.data());
	auto render_size = sizeof(rendering_infos_last);

	SendData(client_socket, SERVER_RENDER_INFO, renderings, render_size);
}

template<class _GameClass>
_GameClass* ServerFramework::Instantiate(double x, double y)
{
	auto result = new _GameClass();
	result->x = x;
	result->y = y;

	instances.push_back(result);

	return result;
}

template<class _GameClass = GameInstance>
void Kill(_GameClass* target) {
	auto loc = find_if(instances.begin(), instances.end(), [target](const auto& lhs) {
		return (lhs == target);
		});

	if (loc != instances.end()) {
		target->OnDestroy();
		instances.erase(loc);
	}
}

template<class _GameClassTarget, class _GameClassSelf>
_GameClassTarget* SeekCollision(_GameClassSelf* self, const char* fid) {
	if (self && !instances.empty()) {
		auto CopyList = vector<GameInstance*>(instances);

		auto it = std::find_if(CopyList.begin(), CopyList.end(), [&](GameInstance* inst) {
			auto iid = inst->GetIdentifier();
			auto id_check = strcmp(iid, fid);

			return (0 == id_check);
			});

		if (it != CopyList.end()) {
			return dynamic_cast<_GameClassTarget*>(*it);
		}
	}
	return nullptr;
}