#include "pch.h"
#include "CommonDatas.h"
#include "Framework.h"


ServerFramework::ServerFramework(int rw, int rh)
	: WORLD_W(rw), WORLD_H(rh), SPAWN_DISTANCE(rh * 0.4), randomizer{ 0 }
	, status(SERVER_STATES::LISTEN), game_started(false)
	, my_socket(0), my_address(), client_number(0), my_process_index(0)
	, rendering_infos_last(nullptr)
	, player_number_last(0), player_captain(-1), player_winner(-1) {
	InitializeCriticalSection(&player_infos_permission);
	InitializeCriticalSection(&print_permission);

	players.reserve(CLIENT_NUMBER_MAX);

	PLAYER_SPAWN_PLACES = new int* [CLIENT_NUMBER_MAX];

	double dir_increment = (360.0 / CLIENT_NUMBER_MAX);
	for (int i = 0; i < CLIENT_NUMBER_MAX; ++i) {
		double dir = dir_increment * i;
		int cx = static_cast<int>(WORLD_W * 0.5 + lengthdir_x(SPAWN_DISTANCE, dir));
		int cy = static_cast<int>(WORLD_W * 0.5 + lengthdir_y(SPAWN_DISTANCE, dir));

		PLAYER_SPAWN_PLACES[i] = new int[2]{ cx, cy };
	}

	event_status = CreateEvent(NULL, FALSE, TRUE, NULL);
	event_player_accept = CreateEvent(NULL, FALSE, FALSE, NULL);
	event_game_start = CreateEvent(NULL, FALSE, FALSE, NULL);
	event_receives = CreateEvent(NULL, FALSE, FALSE, NULL);
	event_game_process = CreateEvent(NULL, FALSE, FALSE, NULL);
	event_send_renders = CreateEvent(NULL, FALSE, FALSE, NULL);

	// event_player_accept
	CreateThread(NULL, 0, ::ConnectProcess, nullptr, 0, NULL);
	// event_send_renders
	//CreateThread(NULL, 0, ::SendRenderingsProcess, nullptr, 0, NULL);
	// event_game_start
	//CreateThread(NULL, 0, ::GameReadyProcess, nullptr, 0, NULL);
}

ServerFramework::~ServerFramework() {
	DeleteCriticalSection(&player_infos_permission);
	DeleteCriticalSection(&print_permission);

	closesocket(my_socket);

	for (auto player : players) {
		CloseHandle(player->client_thread);
	}
	Clean();

	CloseHandle(event_status);
	CloseHandle(event_player_accept);
	CloseHandle(event_game_start);
	CloseHandle(event_receives);
	CloseHandle(event_game_process);
	CloseHandle(event_send_renders);
}

bool ServerFramework::Initialize() {
	WSADATA wsadata;
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsadata)) {
		ErrorDisplay("WSAStartup()");
		return false;
	}

	my_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == my_socket) {
		ErrorDisplay("socket()");
		return false;
	}

	BOOL option = TRUE;
	if (SOCKET_ERROR == setsockopt(my_socket, SOL_SOCKET, SO_REUSEADDR
		, reinterpret_cast<char*>(&option), sizeof(option))) {
		ErrorDisplay("setsockopt()");
		return false;
	}

	ZeroMemory(&my_address, sizeof(my_address));
	my_address.sin_family = AF_INET;
	my_address.sin_addr.s_addr = htonl(INADDR_ANY);
	my_address.sin_port = htons(COMMON_PORT);

	if (SOCKET_ERROR == bind(my_socket, reinterpret_cast<sockaddr*>(&my_address), sizeof(my_address))) {
		ErrorDisplay("bind()");
		return false;
	}

	if (SOCKET_ERROR == listen(my_socket, CLIENT_NUMBER_MAX + 1)) {
		ErrorDisplay("listen()");
		return false;
	}
	AtomicPrintLn("���� ����");

	return true;
}

void ServerFramework::Startup() {
	while (true) {
		AwaitStatusBeginEvent();

		switch (status) {
			case LISTEN:
			{
				AtomicPrintLn("S: Listening");

				CastClientAccept(true);
			}
			break;

			case LOBBY:
			{
				AtomicPrintLn("S: Lobby");

				if (client_number < CLIENT_NUMBER_MAX) {
					CastClientAccept(true);
				} else {
					CastClientAccept(false);
				}
			}
			break;

			case GAME:
			{
				AtomicPrintLn("S: Game");

				while (true) {
					ProcessGame();
					BakeRenderingInfos();
					for (auto player : players) {
						auto client_socket = player->client_socket;
						SendRenderingInfos(client_socket);
					}

					Sleep(600);
				}

				CastClientAccept(false);
			}
			break;

			case GAME_OVER:
			{
				AtomicPrintLn("S: Game Over");
			}
			break;

			case GAME_RESTART:
			{
				AtomicPrintLn("S: Restart Game");
				game_started = false;
			}
			break;

			case EXIT:
			{
				return;
			}
			return;

			default:
				break;
		}
	}
}

void ServerFramework::ProcessConnect() {
	SOCKET new_client = PlayerConnect();
	if (INVALID_SOCKET == new_client) {
		ErrorDisplay("PlayerConnect()");
	}

	if (status == LISTEN || status == LOBBY) {
		CastClientAccept(true);
	}
}

void ServerFramework::ProcessReady() {
	AtomicPrintLn("ProcessReady()");

	shuffle(players.begin(), players.end(), randomizer);

	my_process_index = 0;
	player_winner = -1;

	CastStartReceive(true);
	SetStatus(GAME);
}

void ServerFramework::ProcessGame() {
	if (CheckClientNumber()) { // ���� ó��
		ProceedContinuation();
	} else { // ���� ������ Ȥ�� ���� ���� ����
		auto numb = GetClientNumber();

		if (0 == numb) {
			Clean();
			SetStatus(LISTEN);
		} else if (1 == numb) {
			Clean();
			SetStatus(LOBBY);
		}
	}
}

void ServerFramework::ProcessSync() {
	for (auto player : players) {
		auto client_socket = player->client_socket;
		SendRenderingInfos(client_socket);
	}

	Sleep(FRAME_TIME);
}

void ServerFramework::Clean() {
	players.clear();
	instances.clear();

	players.shrink_to_fit();
	instances.shrink_to_fit();

	players.reserve(CLIENT_NUMBER_MAX);
	SetCaptain(nullptr);
	CastStartGame(false);
	CastStartReceive(false);
}

SOCKET ServerFramework::PlayerConnect() {
	SOCKADDR_IN address;
	int address_length = sizeof(address);

	SOCKET new_socket = accept(my_socket, reinterpret_cast<SOCKADDR*>(&address), &address_length);
	if (INVALID_SOCKET == new_socket) {
		// ����
		return new_socket;
	}

	EnterCriticalSection(&player_infos_permission);
	switch (GetStatus()) {
		case LISTEN:
		{
			// ù��° �÷��̾� ����
			SetStatus(LOBBY);
		}
		break;

		case LOBBY:
		{
			if (CLIENT_NUMBER_MAX <= client_number) {
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
	client_info->client_thread = new_thread;

	SendData(new_socket, PACKETS::SERVER_PLAYER_COUNT
			 , reinterpret_cast<char*>(&client_number), sizeof(client_number));

	// ù��° �÷��̾�
	if (client_number == 0) {
		SetCaptain(client_info);

		//SendData(new_socket, PACKETS::SERVER_SET_CAPATIN);
	}

	client_number++;
	cout << "�� �÷��̾� ����: " << new_socket << endl;
	cout << "���� �÷��̾� ��: " << client_number << " / " << CLIENT_NUMBER_MAX << endl;

	players.emplace_back(client_info);

	LeaveCriticalSection(&player_infos_permission);

	return new_socket;
}

void ServerFramework::PlayerDisconnect(PlayerInfo* player) {
	EnterCriticalSection(&player_infos_permission);
	auto dit = find(players.begin(), players.end(), player);

	if (dit != players.end()) {
		auto player = (*dit);

		CloseHandle(player->client_thread);

		auto id = player->player_index;
		auto character = player->player_character;
		if (character)
			Kill(static_cast<GameInstance*>(character));
		client_number--;

		cout << "�÷��̾� ����: " << player->client_socket << endl;
		cout << "���� �÷��̾� ��: " << client_number << " / " << CLIENT_NUMBER_MAX << endl;

		players.erase(dit);

		// �÷��̾� 0�� Ȥ�� 1��
		if (!CheckClientNumber()) {
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

				case GAME:
				{
					Clean();
					SetStatus(LISTEN);
				}
				break;

				case GAME_OVER:
				{
					Clean();
					SetStatus(LISTEN);
				}
				break;

				case GAME_RESTART: { /* ���⼭ ó�� ���� */ } break;
				case EXIT: { /* ���⼭ ó�� ���� */ } break;
				default: break;
			}
		}

		// ������ ����
		if (player_captain == id) {
			if (0 < client_number) {
				auto new_captain = players.at(0);
				SetCaptain(new_captain);
				SendData(new_captain->client_socket, PACKETS::SERVER_SET_CAPATIN);
			}
		}
	}
	LeaveCriticalSection(&player_infos_permission);
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
		player_captain = player->player_index;
	} else {
		player_captain = -1;
	}
}

void ServerFramework::SetStatus(SERVER_STATES state) {
	if (status != state) {
		AtomicPrintLn("���� ���� ����: ", status, " -> ", state);

		status = state;
		CastStatusChanged();
	}
}

SERVER_STATES ServerFramework::GetStatus() const {
	return status;
}

int ServerFramework::GetClientNumber() const {
	return client_number;
}

void ServerFramework::ProceedContinuation() {
	cout << "ProceedContinuation()" << endl;
	if (my_process_index < client_number) {
		cout << "�ϳ��� Ŭ���̾�Ʈ ������ ó��: " << my_process_index << endl;
		my_process_index++;

		CastStartReceive(true);
	} else {
		my_process_index = 0;
		cout << "���� �ν��Ͻ� ó��" << endl;

		// ���� ���� ����
		ForeachInstances([&](GameInstance*& inst) {
			inst->OnUpdate(FRAME_TIME);
		});

		BakeRenderingInfos();

		// ���� ���� ����


		CastSendingRenderingInfos(true);
	}
}

void ServerFramework::BakeRenderingInfos() {
	if (!instances.empty()) {
		AtomicPrintLn("������ ���� ����\nũ��: ", instances.size());
		if (rendering_infos_last) {
			delete[] rendering_infos_last;
		}
		rendering_infos_last = new RenderInstance[RENDER_INST_COUNT];

		auto CopyList = vector<GameInstance*>(instances);

		// �÷��̾� ��ü�� �� ����
		std::partition(CopyList.begin(), CopyList.end(), [&](GameInstance* inst) {
			return (strcmp(inst->GetIdentifier(), "Player") == 0);
		});

		int index = 0;
		for (auto it = CopyList.begin(); it != CopyList.end(); ++it) {
			auto& render_infos = (*it)->GetRenderInstance();

			// �ν��Ͻ��� ����ִ� ��쿡�� ������ �޼��� ����
			if (!(*it)->dead)
				rendering_infos_last[index++] = render_infos;
		}
	} else if (rendering_infos_last) {
		delete[] rendering_infos_last;
		rendering_infos_last = nullptr;
	}
}

void ServerFramework::SendRenderingInfos(SOCKET my_socket) {
	if (rendering_infos_last) {
		AtomicPrintLn("SendRenderingInfos()");
		const char* my_render_info = reinterpret_cast<char*>(&rendering_infos_last);
		const size_t my_render_size = RENDER_INST_COUNT * sizeof(RenderInstance);

		SendData(my_socket, SERVER_RENDER_INFO, my_render_info, my_render_size);
	}
}

void ServerFramework::CastStatusChanged() {
	SetEvent(event_status);
}

void ServerFramework::CastClientAccept(bool flag) {
	AtomicPrintLn("CastClientAccept: ", boolalpha, flag);
	if (flag && client_number < CLIENT_NUMBER_MAX) {
		SetEvent(event_player_accept);
	} else {
		ResetEvent(event_player_accept);
	}
}

void ServerFramework::CastStartGame(bool flag) {
	AtomicPrintLn("CastStartGame: ", boolalpha, flag);
	if (flag) {
		SetEvent(event_game_start);
	} else {
		ResetEvent(event_game_start);
	}
}

void ServerFramework::CastStartReceive(bool flag) {
	AtomicPrintLn("CastStartReceive: ", boolalpha, flag);
	if (flag) {
		SetEvent(event_receives);
	} else {
		ResetEvent(event_receives);
	}
}

void ServerFramework::CastProcessingGame() {
	AtomicPrintLn("CastProcessingGame");
	SetEvent(event_game_process);
}

void ServerFramework::CastSendingRenderingInfos(bool flag) {
	AtomicPrintLn("CastSendingRenderingInfos: ", boolalpha, flag);
	if (flag) {
		SetEvent(event_send_renders);
	} else {
		ResetEvent(event_send_renders);
	}
}

PlayerInfo* ServerFramework::GetPlayer(int player_index) {
	auto loc = find_if(players.begin(), players.end(), [player_index](PlayerInfo*& lhs) {
		return (lhs->player_index == player_index);
	});

	if (loc != players.end()) {
		return *loc;
	}
	return nullptr;
}

PlayerInfo::PlayerInfo(SOCKET sk, HANDLE hd, int id) {
	client_socket = sk;
	client_thread = hd;
	player_index = id;
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

void GameInstance::SetOwner(int player_index) {
	owner = player_index;
}

void GameInstance::SetRenderType(RENDER_TYPES sprite) {
	my_renders.instance_type = sprite;
}

void GameInstance::SetImageNumber(int number) {
	image_number = static_cast<double>(number);
}

void GameInstance::SetDirection(double dir) {
	if (hspeed != 0.0 || vspeed != 0.0) {
		auto speed = point_distance(0, 0, hspeed, vspeed);

		hspeed = lengthdir_x(speed, dir);
		vspeed = lengthdir_y(speed, dir);
	}

	direction = dir;
}

void GameInstance::SetSpeed(double speed) {
	if (hspeed != 0.0 || vspeed != 0.0) {
		auto old_dir = point_direction(0, 0, hspeed, vspeed);

		hspeed = lengthdir_x(speed, old_dir);
		vspeed = lengthdir_y(speed, old_dir);
	}
}

void GameInstance::SetVelocity(double speed, double dir) {
	hspeed = lengthdir_x(speed, dir);
	vspeed = lengthdir_y(speed, dir);
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

	my_renders.image_index = static_cast<int>(image_index);
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
	int error = WSAGetLastError();

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, error,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPTSTR>(&lpMsgBuf), 0, nullptr);

	MessageBox(nullptr, static_cast<LPCTSTR>(lpMsgBuf), msg, MB_ICONERROR);

	LocalFree(lpMsgBuf);
	exit(error);
}

void ErrorDisplay(const char* msg) {
	LPVOID lpMsgBuf;

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPTSTR>(&lpMsgBuf), 0, nullptr);

	std::cout << "[" << msg << "] " << static_cast<char*>(lpMsgBuf) << std::endl;

	LocalFree(lpMsgBuf);
}
