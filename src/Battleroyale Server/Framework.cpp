#include "pch.h"
#include "CommonDatas.h"
#include "Framework.h"

ServerFramework::ServerFramework(int rw, int rh)
	: WORLD_W(rw), WORLD_H(rh), SPAWN_DISTANCE(rh * 0.4), randomizer{ 0 }
	, status(SERVER_STATES::LISTEN), status_begin(false)
	, my_socket(0), my_address(), client_number(0), my_process_index(0)
	, thread_game_starter(NULL), thread_game_process(NULL), rendering_infos_last(nullptr)
	, player_number_last(0), player_captain(-1), player_winner(-1) {

	players.reserve(PLAYERS_NUMBER_MAX);

	PLAYER_SPAWN_PLACES = new int* [PLAYERS_NUMBER_MAX];

	double dir_increment = (360.0 / PLAYERS_NUMBER_MAX);
	for (int i = 0; i < PLAYERS_NUMBER_MAX; ++i) {
		double dir = dir_increment * i;
		int cx = static_cast<int>(WORLD_W * 0.5 + lengthdir_x(SPAWN_DISTANCE, dir));
		int cy = static_cast<int>(WORLD_W * 0.5 + lengthdir_y(SPAWN_DISTANCE, dir));

		PLAYER_SPAWN_PLACES[i] = new int[2]{ cx, cy };
	}
}

ServerFramework::~ServerFramework() {
	closesocket(my_socket);

	for (auto player : players) {
		CloseHandle(player->client_handle);
	}
	Clean();

	CloseHandle(thread_game_starter);
	CloseHandle(thread_game_process);

	CloseHandle(event_player_accept);
	CloseHandle(event_game_start);
	CloseHandle(event_receives);
	CloseHandle(event_game_process);
	CloseHandle(event_send_renders);
}

bool ServerFramework::Initialize() {
	WSADATA wsadata;
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsadata)) {
		// 오류
		return false;
	}

	my_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == my_socket) {
		// 오류
		return false;
	}

	BOOL option = TRUE;
	if (SOCKET_ERROR == setsockopt(my_socket, SOL_SOCKET, SO_REUSEADDR
		, reinterpret_cast<char*>(&option), sizeof(option))) {
		// 오류
		return false;
	}

	ZeroMemory(&my_address, sizeof(my_address));
	my_address.sin_family = AF_INET;
	my_address.sin_addr.s_addr = htonl(INADDR_ANY);
	my_address.sin_port = htons(COMMON_PORT);

	if (SOCKET_ERROR == bind(my_socket, reinterpret_cast<sockaddr*>(&my_address), sizeof(my_address))) {
		ErrorAbort("bind()");
		return false;
	}

	if (SOCKET_ERROR == listen(my_socket, PLAYERS_NUMBER_MAX + 1)) {
		ErrorAbort("listen()");
		return false;
	}
	cout << "서버 시작" << endl;

	event_player_accept = CreateEvent(NULL, FALSE, FALSE, NULL);
	event_game_start = CreateEvent(NULL, FALSE, FALSE, NULL);
	event_receives = CreateEvent(NULL, FALSE, FALSE, NULL);
	event_game_process = CreateEvent(NULL, FALSE, FALSE, NULL);
	event_send_renders = CreateEvent(NULL, TRUE, FALSE, NULL); // 수동 리셋 이벤트 객체

	// event_player_accept
	CreateThread(NULL, 0, ConnectProcess, nullptr, 0, NULL);
	// event_game_start
	thread_game_starter = CreateThread(NULL, 0, GameInitializeProcess, nullptr, 0, NULL);
	// event_game_process
	thread_game_process = CreateThread(NULL, 0, GameProcess, nullptr, 0, NULL);

	return true;
}

void ServerFramework::Startup() {
	while (true) {
		switch (status) {
			case LISTEN:
			{
				if (!status_begin) {
					cout << "S: Listening" << endl;

					CastClientAccept(true);
					status_begin = true;
				}
			}
			break;

			case LOBBY:
			{
				if (!status_begin) {
					cout << "S: Lobby" << endl;


					if (client_number < PLAYERS_NUMBER_MAX) {
						CastClientAccept(true);
					} else {
						CastClientAccept(false);
					}
					status_begin = true;
				}
			}
			break;

			case GAME:
			{
				if (!status_begin) {
					cout << "S: Starting the game" << endl;

					CastClientAccept(false);
					status_begin = true;
				}
			}
			break;

			case GAME_OVER:
			{
				if (!status_begin) {
					cout << "S: The game is over." << endl;

					status_begin = true;
				}
			}
			break;

			case GAME_RESTART:
			{
				if (!status_begin) {
					cout << "S: Restarting the game" << endl;

					status_begin = true;
				}
			}
			break;

			case EXIT:
			{
				// 종료
			}
			return;

			default:
				break;
		}
	}
}

void ServerFramework::GameReady() {
	shuffle(players.begin(), players.end(), randomizer);

	my_process_index = 0;
	player_winner = -1;
}

void ServerFramework::GameUpdate() {
	ForeachInstances([&](GameInstance*& inst) {
		inst->OnUpdate(FRAME_TIME);
	});
}

void ServerFramework::Clean() {
	players.clear();
	instances.clear();

	players.shrink_to_fit();
	instances.shrink_to_fit();

	players.reserve(PLAYERS_NUMBER_MAX);
	SetCaptain(nullptr);
}

SOCKET ServerFramework::PlayerConnect() {
	SOCKADDR_IN address;
	int address_length = sizeof(address);

	SOCKET new_socket = accept(my_socket, reinterpret_cast<SOCKADDR*>(&address), &address_length);
	if (INVALID_SOCKET == new_socket) {
		// 오류
		return new_socket;
	}

	switch (GetStatus()) {
		case LISTEN:
		{
			// 첫번째 플레이어 접속
			SetStatus(LOBBY);
		}
		break;

		case LOBBY:
		{
			if (PLAYERS_NUMBER_MAX <= client_number) {
				closesocket(new_socket);
				return 0;
			}
		}
		break;

		default:
		{
			return 0;
		}
	}

	auto client_info = new PlayerInfo(new_socket, 0, player_number_last++);
	HANDLE new_thread = CreateThread(NULL, 0, CommunicateProcess, (client_info), 0, NULL);
	client_info->client_handle = new_thread;

	// 첫번째 플레이어
	if (client_number == 0) {
		SetCaptain(client_info);

		SendData(new_socket, PACKETS::SERVER_SET_CAPATIN);
	}

	client_number++;
	cout << "새 플레이어 접속: " << new_socket << endl;
	cout << "현재 플레이어 수: " << client_number << " / " << PLAYERS_NUMBER_MAX << endl;

	players.emplace_back(client_info);

	SendData(new_socket, PACKETS::SERVER_PLAYER_COUNT
			 , reinterpret_cast<char*>(&client_number), sizeof(client_number));

	return new_socket;
}

void ServerFramework::PlayerDisconnect(PlayerInfo* player) {
	auto dit = find(players.begin(), players.end(), player);

	if (dit != players.end()) {
		auto player = (*dit);

		CloseHandle(player->client_handle);

		auto id = player->index;
		auto character = player->player_character;
		if (character)
			Kill(static_cast<GameInstance*>(character));
		client_number--;

		cout << "플레이어 종료: " << player->client_socket << endl;
		cout << "현재 플레이어 수: " << client_number << " / " << PLAYERS_NUMBER_MAX << endl;

		players.erase(dit);

		// 플레이어 0명 혹은 1명
		if (client_number < 2) {
			switch (status) {
				case LISTEN:
				{
					if (0 == client_number) {
						Clean();
					}
				}
				break;

				case LOBBY:
				{
					if (0 == client_number) {
						SetStatus(LISTEN);
						Clean();
					}
				}
				break;

				case GAME: { /* 여기서 처리 안함 */ } break;
				case GAME_OVER: { /* 여기서 처리 안함 */ } break;
				case GAME_RESTART: { /* 여기서 처리 안함 */ } break;
				case EXIT: { /* 여기서 처리 안함 */ } break;
				default: break;
			}
		}

		// 방장이 나감
		if (player_captain == id) {
			if (0 < client_number) {
				auto new_captain = players.at(0);
				SetCaptain(new_captain);
				SendData(new_captain->client_socket, PACKETS::SERVER_SET_CAPATIN);
			}
		}

	}
}

bool ServerFramework::CheckClientNumber() const {
	return (CLIENT_NUMBER_MIN <= client_number);
}

bool ServerFramework::ValidateSocketMessage(int socket_state) {
	if (SOCKET_ERROR == socket_state) {
		return false;
	} else if (0 == socket_state) {
		return false;
	}

	return true;
}

void ServerFramework::SetCaptain(PlayerInfo* player) {
	if (player) {
		player_captain = player->index;
	} else {
		player_captain = -1;
	}
}

void ServerFramework::SetStatus(SERVER_STATES state) {
	if (status != state) {
		cout << "서버 상태 변경: " << status << " -> " << state << endl;

		status = state;
		status_begin = false;
	}
}

SERVER_STATES ServerFramework::GetStatus() const {
	return status;
}

int ServerFramework::GetClientNumber() const {
	return client_number;
}

void ServerFramework::ProceedContinuation() {
	if (my_process_index < client_number) {
		my_process_index++;
	} else {
		my_process_index = 0;

		// 게임 상태 갱신
		GameUpdate();
		BakeRenderingInfos();

		// 게임 승패 판정


		CastSendingRenderingInfos(true);
	}
}

void ServerFramework::BakeRenderingInfos() {
	if (!instances.empty()) {
		if (rendering_infos_last) {
			delete[] rendering_infos_last;
		}
		rendering_infos_last = new RenderInstance[RENDER_INST_COUNT];

		auto CopyList = vector<GameInstance*>(instances);

		// 플레이어 개체를 맨 위로
		std::partition(CopyList.begin(), CopyList.end(), [&](GameInstance* inst) {
			return (strcmp(inst->GetIdentifier(), "Player") == 0);
		});

		int index = 0;
		for (auto it = CopyList.begin(); it != CopyList.end(); ++it) {
			auto& render_infos = (*it)->GetRenderInstance();

			rendering_infos_last[index++] = render_infos;
		}
	} else if (rendering_infos_last) {
		delete[] rendering_infos_last;
		rendering_infos_last = nullptr;
	}
}

void ServerFramework::SendRenderingInfos(SOCKET my_socket) {
	if (rendering_infos_last) {
		const char* my_render_info = reinterpret_cast<char*>(&rendering_infos_last);
		const size_t my_render_size = RENDER_INST_COUNT * sizeof(RenderInstance);

		SendData(my_socket, SERVER_RENDER_INFO, my_render_info, my_render_size);
	}
}

void ServerFramework::CastClientAccept(bool flag) {
	if (flag && client_number < PLAYERS_NUMBER_MAX) {
		SetEvent(event_player_accept);
	} else {
		ResetEvent(event_player_accept);
	}
}

void ServerFramework::CastStartGame(bool flag) {
	if (flag) {
		SetEvent(event_game_start);
	} else {
		ResetEvent(event_game_start);
	}
}

void ServerFramework::CastStartReceive(bool flag) {
	if (flag) {
		SetEvent(event_receives);
	} else {
		ResetEvent(event_receives);
	}
}

void ServerFramework::CastProcessingGame() {
	SetEvent(event_game_process);
}

void ServerFramework::CastSendingRenderingInfos(bool flag) {
	if (flag) {
		SetEvent(event_send_renders);
	} else {
		ResetEvent(event_send_renders);
	}
}

PlayerInfo* ServerFramework::GetPlayer(int player_index) {
	auto loc = find_if(players.begin(), players.end(), [player_index](PlayerInfo*& lhs) {
		return (lhs->index == player_index);
	});

	if (loc != players.end()) {
		return *loc;
	}
	return nullptr;
}

PlayerInfo::PlayerInfo(SOCKET sk, HANDLE hd, int id) {
	client_socket = sk;
	client_handle = hd;
	index = id;
}

PlayerInfo::~PlayerInfo() {
	delete player_character;
}

GameInstance::GameInstance()
	: owner(-1)
	, image_angle(0.0), image_index(0.0), image_speed(0.0), image_number(0.0)
	, my_renders{}
	, box{}, dead(false)
	, x(0), y(0), hspeed(0.0), vspeed(0.0), direction(0.0) {
	ZeroMemory(&my_renders, sizeof(my_renders));
}

GameInstance::~GameInstance() {
	delete& my_renders;
}

void GameInstance::OnCreate() {}

void GameInstance::OnDestroy() {}

void GameInstance::OnUpdate(double frame_advance) {
	if (hspeed != 0.0 || vspeed != 0.0) {
		x += hspeed * frame_advance;
		y += vspeed * frame_advance;
	}

	if (image_speed != 0.0 && 1.0 < image_number) {
		image_index += image_speed;

		if (image_index < 0) {
			image_index += image_number;
		} else if (image_number <= image_index) {
			image_index -= image_number;
		}
	}
}

void GameInstance::SetRenderType(RENDER_TYPES sprite) {
	my_renders.instance_type = sprite;
}

void GameInstance::SetImageNumber(int number) {
	image_number = static_cast<double>(number);
}

RenderInstance& GameInstance::GetRenderInstance() {
	return my_renders;
}

void GameInstance::SetBoundBox(const RECT& mask) {
	CopyRect(&box, &mask);
}

int GameInstance::GetBoundLT() const {
	return x + box.left;
}

int GameInstance::GetBoundTP() const {
	return y + box.top;
}

int GameInstance::GetBoundRT() const {
	return x + box.right;
}

int GameInstance::GetBoundBT() const {
	return y + box.bottom;
}

const char* GameInstance::GetIdentifier() const {
	return "Instance";
}

bool GameInstance::IsCollideWith(GameInstance* other) {
	return !(other->GetBoundRT() <= GetBoundLT()
		|| other->GetBoundBT() <= GetBoundTP()
		|| GetBoundRT() < other->GetBoundLT()
		|| GetBoundBT() < other->GetBoundTP());
}

RenderInstance& GameInstance::AssignRenderingInfo(double angle) {
	my_renders.x = x;
	my_renders.y = y;

	my_renders.image_index = image_index;
	my_renders.angle = angle;

	return my_renders;
}

void SendData(SOCKET socket, PACKETS type, const char* buffer, int length) {
	int result = send(socket, reinterpret_cast<char*>(&type), sizeof(PACKETS), 0);
	if (SOCKET_ERROR == result) {
		ErrorAbort("send 1");
	}

	if (buffer) {
		result = send(socket, buffer, length, 0);
		if (SOCKET_ERROR == result) {
			ErrorAbort("send 2");
		}
	}
}

void ErrorAbort(const char* msg) {
	LPVOID lpMsgBuf;

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPTSTR>(&lpMsgBuf), 0, nullptr);

	MessageBox(nullptr, static_cast<LPCTSTR>(lpMsgBuf), msg, MB_ICONERROR);

	LocalFree(lpMsgBuf);
	exit(true);
}

void ErrorDisplay(const char* msg) {
	LPVOID lpMsgBuf;

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPTSTR>(&lpMsgBuf), 0, nullptr);

	std::cout << "[" << msg << "] " << static_cast<char*>(lpMsgBuf) << std::endl;

	LocalFree(lpMsgBuf);
}
