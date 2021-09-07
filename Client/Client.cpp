#include "stdafx.h"
#include <winsock2.h>
#include <windows.h>
#include <WS2tcpip.h>
#include <stdio.h>
#include <process.h>
#include <conio.h>
#include <string>
#include <iostream>
#include <queue>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <utility>

#define PORT 6000
#define DATA_BUFSIZE 8192
#define SERVER_ADDR "127.0.0.2"
#define RECEIVE 0
#define SEND 1
#define SCREEN_INIT 0
#define SCREEN_LOGIN 1
#define SCREEN_MAIN 2
#define SCREEN_PLAY_GAME 3
#define SCREEN_REGISTER 4



#pragma comment(lib, "Ws2_32.lib")
using namespace std;

int rows;
int columns;
long long int currMilisec = 0;
long long int startTurnTime = 0;
int timeLeft = 0;
bool isInGame = false;
string usernamePlayer = "";


unsigned int __stdcall timerThread(void *params);
void				workPerHunderedMilisec(long long int milisec);

void(*onTick)(long long int);

HANDLE screen = GetStdHandle(STD_OUTPUT_HANDLE);
HANDLE mutexx = CreateMutex(0, 0, 0);
HANDLE mutexScreen = CreateMutex(0, 0, 0);

bool newScreen = false;
char keyPress = '\0';

int currentScreen = SCREEN_INIT;

#include "Menu.h"
#include "Utilities.h"
#include "Message.h"


// Structure definition
typedef struct {
	WSAOVERLAPPED overlapped;
	WSABUF dataBuff;
	CHAR buffer[DATA_BUFSIZE];
	int bufLen;
	int recvBytes;
	int sentBytes;
	int operation;
} PER_IO_OPERATION_DATA, *LPPER_IO_OPERATION_DATA;

typedef struct {
	SOCKET socket;
} PER_HANDLE_DATA, *LPPER_HANDLE_DATA;


HANDLE completionPort;

struct SK {
	bool waiting = true;
	int prevOperation = RECEIVE;
	int numberInQueue = 0;

	LPPER_HANDLE_DATA perHandleData = NULL;

	LPPER_IO_OPERATION_DATA perIoData = NULL;
	queue<string> needSendMessages;
	string restMessage = "";
	queue<Message> response;
	HANDLE mutex;

	SK(LPPER_HANDLE_DATA _perHandleData, LPPER_IO_OPERATION_DATA _perIoData) {
		this->perHandleData = _perHandleData;
		this->perIoData = _perIoData;
		this->mutex = CreateMutex(0, 0, 0);
	}

	void lock() {
		WaitForSingleObject(this->mutex, INFINITE);
	}

	void unlock() {
		ReleaseMutex(this->mutex);
	}

	bool canSendNewMsg() {
		return ((perIoData->recvBytes <= perIoData->sentBytes) || (perIoData->operation == RECEIVE)) && needSendMessages.size() > 0;
	}

	bool canContinuteSendMsg() {
		return perIoData->recvBytes > perIoData->sentBytes && perIoData->operation == SEND && !waiting;
	}

	void changeWaiting(bool wait) {
		waiting = wait;
	}



	void recvMsg() {
		DWORD transferredBytes = 0;
		DWORD flags = 0;
		ZeroMemory(&(perIoData->overlapped), sizeof(OVERLAPPED));
		perIoData->recvBytes = 0;
		perIoData->sentBytes = 0;
		perIoData->dataBuff.len = DATA_BUFSIZE;
		perIoData->dataBuff.buf = perIoData->buffer;
		numberInQueue++;
		perIoData->operation = RECEIVE;
		if (WSARecv(perHandleData->socket,
			&(perIoData->dataBuff),
			1,
			&transferredBytes,
			&flags,
			&(perIoData->overlapped), NULL) == SOCKET_ERROR) {
			if (WSAGetLastError() != ERROR_IO_PENDING) {
				printf("WSARecv() failed with error %d\n", WSAGetLastError());
			}
		}
	}

	void continuteSendMsg() {
		DWORD flags = 0;
		DWORD transferredBytes = 0;
		ZeroMemory(&(perIoData->overlapped), sizeof(OVERLAPPED));
		perIoData->dataBuff.buf = perIoData->buffer + perIoData->sentBytes;
		perIoData->dataBuff.len = perIoData->recvBytes - perIoData->sentBytes;
		perIoData->operation = SEND;
		if (WSASend(perHandleData->socket,
			&(perIoData->dataBuff),
			1,
			&transferredBytes,
			0,
			&(perIoData->overlapped),
			NULL) == SOCKET_ERROR) {
			if (WSAGetLastError() != ERROR_IO_PENDING) {
				printf("WSASend() failed with error %d\n", WSAGetLastError());
			}
		}
	}

	void sendNewMsg() {
		string msg = needSendMessages.front();
		needSendMessages.pop();

		DWORD transferredBytes = 0;
		ZeroMemory(&(perIoData->overlapped), sizeof(OVERLAPPED));
		perIoData->recvBytes = msg.length();
		perIoData->sentBytes = 0;
		strcpy_s(perIoData->buffer, msg.c_str());
		perIoData->dataBuff.len = msg.length();
		perIoData->dataBuff.buf = perIoData->buffer;

		perIoData->operation = SEND;
		if (WSASend(perHandleData->socket,
			&(perIoData->dataBuff),
			1,
			&transferredBytes,
			0,
			&(perIoData->overlapped),
			NULL) == SOCKET_ERROR) {
			if (WSAGetLastError() != ERROR_IO_PENDING) {
				printf("WSASend() %s failed with error %d\n", msg.c_str(), WSAGetLastError());
			}
		}
	}

	void send(string s) {
		lock();
		needSendMessages.push(s);
		if (canSendNewMsg()) {
			sendNewMsg();
		}
		unlock();
	}
};

vector<SK*> socks;
unsigned __stdcall serverWorkerThread(LPVOID CompletionPortID);
SOCKET client;

void ScreenMenu(SK *);
void ScreenLogin(SK *);
void ScreenRegister(SK *);
void game(SK *);

vector<pair<string, string>> split(string, string, string);
vector<pair<string, string>> listFriend;
vector<pair<string, string>> listCanChallenge;
vector<pair<string, string>> listFriendInvitation;
vector<pair<string, string>> listChallengeInvitation;
SK *sock;

#include "main-menu.h"
#include "Game.h"

void(*onReceive) (string) = NULL;
fstream file;
string fileName;

void handleMessageReceived(string a) {
	messageToResponse(a, sock->restMessage, sock->response);

	while (!sock->response.empty()) {
		Message resp = sock->response.front();
		sock->response.pop();
		int responseCode = atoi(resp.content.substr(0, 3).c_str());

		switch (resp.command) {
		case REQ_SEND_CHALLENGE_INVITATION:
			dataSource[3].second.push_back({ resp.content, "" });
			updateListInvitationChallenge(dataSource[3].second);
			break;

		case REQ_START_GAME:
			drawHeader("Press [F2] to join match!", false);
			haveGame = true;
			counter = 0;
			type = resp.content.at(0);
			isInGame = true;
			timeLeft = 20;
			startTurnTime = currMilisec;
			currentScreen = SCREEN_PLAY_GAME;
			newScreen = true;
			break;

		case REQ_END_GAME:
			WaitForSingleObject(mutexGame, INFINITE);
			print(WIDTH * 3 + 3, 7, CLR_NORML, resp.content + " is winner!!!");
			print(WIDTH * 3 + 3, 9, CLR_NORML, "Press [F3] to back the main menu.");
			ReleaseMutex(mutexGame);
			haveGame = false;
			isInGame = false;
			break;

		case TRANSFILE_START: 
			fileName = usernamePlayer + "_" + getCurrentDateTime("%d_%m_%Y_%H_%M_%S") + ".txt";
			file.open(fileName.c_str(), ios::app | ios::binary);
		
			break;

		case TRANSFILE_DATA: {
			int lineNumber = atoi(resp.content.substr(0, 2).c_str());
			string lineContent = resp.content.substr(2);
			if (file.is_open()) {
				file << lineContent << endl;
			}
			else {
				file.open(fileName.c_str(), ios::app | ios::binary); 
				file << lineContent << endl;
			}
		}
			break;

		case TRANSFILE_FINISH: {
			if (file.is_open()) 
				file.close();
			}
			 break;

		case RESPONSE:
			switch (responseCode)
			{
			case RES_REGISTER_SUCCESSFUL:
				printItem(columns / 3 + 3, 13, CLR_NORML, "REGISTER SUCCESSFUL. YOU CAN LOGIN.");
				Sleep(2000);
				currentScreen = SCREEN_INIT;
				newScreen = true;
				break;

			case RES_REGISTER_FAIL:
				printItem(columns / 3 + 3, 13, CLR_NORML, "REGISTER FAIL. PLEASE TRY AGAIN.");
				Sleep(2000);
				currentScreen = SCREEN_REGISTER;
				newScreen = true;
				break;

			case RES_LOGIN_SUCCESSFUL:
				currentScreen = SCREEN_MAIN;
				newScreen = true;
				break;

			case RES_LOGIN_FAIL:
				printItem(columns / 3 + 3, 13, CLR_NORML, "LOGIN FAIL. PLEASE TRY AGAIN.");
				Sleep(2000);
				currentScreen = SCREEN_LOGIN;
				newScreen = true;
				break;

			case RES_GET_LIST_FRIEND_SUCCESSFUL:
				listFriend = split(resp.content.substr(3), "$", "&");
				if (currentScreen == SCREEN_MAIN) updateListFriend(listFriend);
				break;

			case RES_GET_LIST_CAN_CHALLENGE_SUCCESSFUL:
				listCanChallenge = split(resp.content.substr(3), "$", "&");
				if (currentScreen == SCREEN_MAIN) updateListChallenge(listCanChallenge);
				break;

			case RES_DENY_CHALLENGE_INVITATION:
				drawHeader("The player refused the challenge", false);
				break;

			case RES_SEND_CHALLENGE_INVITATION_SUCCESSFUL:
				drawHeader("Waiting for opponent", false);
				break;

			case RES_SEND_CHALLENGE_INVITATION_FAIL:
				drawHeader("Unable to send your invitation", false);
				break;

			case RES_LOGOUT_SUCCESSFUL:
				currentScreen = SCREEN_INIT;
				newScreen = true;
				break;

			case RES_PLAY_SUCCESSFUL: {
				int pos1 = resp.content.find("&");
				int pos2 = resp.content.find("$");
				string _type = resp.content.substr(3, 1);
				string xx = resp.content.substr(pos1 + 1, 2);
				string yy = resp.content.substr(pos2 + 1, 2);
				WaitForSingleObject(mutexGame, INFINITE);
				drawBoard(atoi(xx.c_str()), atoi(yy.c_str()), _type);
				ReleaseMutex(mutexGame);
				counter++;
				timeLeft = 20;
				startTurnTime = currMilisec;
			}
									  break;

			case RES_PLAY_FAIL:
				print(WIDTH * 3 + 3, 7, COLOR_RED, "Invalid move");
				break;
			}
			break;
		}
	}
}



int _tmain(int argc, _TCHAR* argv[])
{
	ShowConsoleCursor(false);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

	SOCKADDR_IN serverAddr;

	SYSTEM_INFO systemInfo;
	LPPER_HANDLE_DATA perHandleData;
	LPPER_IO_OPERATION_DATA perIoData;
	DWORD transferredBytes;
	DWORD flags;
	WSADATA wsaData;

	onReceive = handleMessageReceived;


	onTick = workPerHunderedMilisec;
	_beginthreadex(0, 0, timerThread, 0, 0, 0);


	if (WSAStartup((2, 2), &wsaData) != 0) {
		printf("WSAStartup() failed with error %d\n", GetLastError());
		return 1;
	}

	// Step 1: Setup an I/O completion port
	if ((completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0)) == NULL) {
		printf("CreateIoCompletionPort() failed with error %d\n", GetLastError());
		return 1;
	}

	// Step 2: Determine how many processors are on the system
	GetSystemInfo(&systemInfo);

	// Step 3: Create worker threads based on the number of processors available on the
	// system. Create two worker threads for each processor	
	for (int i = 0; i < (int)systemInfo.dwNumberOfProcessors * 2; i++) {
		// Create a server worker thread and pass the completion port to the thread
		if (_beginthreadex(0, 0, serverWorkerThread, (void*)completionPort, 0, 0) == 0) {
			printf("Create thread failed with error %d\n", GetLastError());
			return 1;
		}
	}

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(PORT);
	inet_pton(AF_INET, SERVER_ADDR, &serverAddr.sin_addr);
	client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (WSAConnect(client, (sockaddr*)&serverAddr, sizeof(serverAddr), 0, 0, 0, 0)) {
		printf("Cannot connect\n");
		return 1;
	};

	// Step 6: Create a socket information structure to associate with the socket
	if ((perHandleData = (LPPER_HANDLE_DATA)GlobalAlloc(GPTR, sizeof(PER_HANDLE_DATA))) == NULL) {
		printf("GlobalAlloc() failed with error %d\n", GetLastError());
		return 1;
	}

	// Step 7: Associate the accepted socket with the original completion port
	printf("Socket number %d got connected...\n", client);
	perHandleData->socket = client;

	if (CreateIoCompletionPort((HANDLE)client, completionPort, (DWORD)perHandleData, 0) == NULL) {
		printf("CreateIoCompletionPort() failed with error %d\n", GetLastError());
		return 1;
	}

	// Step 8: Create per I/O socket information structure to associate with the WSARecv call
	if ((perIoData = (LPPER_IO_OPERATION_DATA)GlobalAlloc(GPTR, sizeof(PER_IO_OPERATION_DATA))) == NULL) {
		printf("GlobalAlloc() failed with error %d\n", GetLastError());
		return 1;
	}

	sock = new SK(perHandleData, perIoData);
	socks.push_back(sock);
	sock->recvMsg();
	string s;

	dataSource.push_back({ "List Player Online", vector<pair<string, string>>() });
	dataSource.push_back({ "List Challenger", vector<pair<string, string>>() });
	dataSource.push_back({ "List Friend Invitation", vector<pair<string, string>>() });
	dataSource.push_back({ "List Challenge Invitation", vector<pair<string, string>>() });
	dataSource.push_back({ "Logout", vector<pair<string, string>>() });


	newScreen = true;
	while (1) {
		while (!newScreen) {};
		newScreen = false;
		system("cls");
		switch (currentScreen) {
		case SCREEN_INIT:
			ScreenMenu(sock);
			break;
		case SCREEN_LOGIN:
			ScreenLogin(sock);
			break;
		case SCREEN_MAIN:
			ScreenMainMenu(sock);
			break;
		case SCREEN_PLAY_GAME:
			game(sock);
			break;
		}
	}

	_getch();
	return 0;
}

SK* findSK(LPPER_HANDLE_DATA perHandleData) {
	for (int i = 0; i < socks.size(); i++) {

		if (socks[i] != NULL && socks[i]->perHandleData == perHandleData) {
			return socks[i];
		}
	}
	return NULL;
}


void deleteSK(SK *sk) {
	ReleaseMutex(sk->mutex);
	CloseHandle(sk->mutex);
	for (auto sock = socks.begin(); sock != socks.end(); sock++) {
		if (*sock == sk) {
			socks.erase(sock);
			break;
		}
	}
	while (!sk->needSendMessages.empty()) sk->needSendMessages.pop();
	closesocket(sk->perHandleData->socket);
	GlobalFree(sk->perHandleData);
	GlobalFree(sk->perIoData);
	delete sk;
}

unsigned __stdcall serverWorkerThread(LPVOID completionPortID)
{
	HANDLE completionPort = (HANDLE)completionPortID;
	DWORD transferredBytes;
	LPPER_HANDLE_DATA perHandleData;
	LPPER_IO_OPERATION_DATA perIoData;
	SK *sk = NULL;

	while (TRUE) {
		if (GetQueuedCompletionStatus(completionPort, &transferredBytes,
			(LPDWORD)&perHandleData, (LPOVERLAPPED *)&perIoData, INFINITE) == 0) {
			printf("GetQueuedCompletionStatus() failed with error %d\n", GetLastError());
			return 0;
		}

		sk = findSK(perHandleData);

		if (sk == NULL) {
			cout << " Not Found! \n " << endl;
			continue;
		}

		sk->lock();
		sk->waiting = false;


		if (transferredBytes == 0 && perIoData->operation != SEND) {
			printf("Closing socket %d\n", perHandleData->socket);
			if (closesocket(sk->perHandleData->socket) == SOCKET_ERROR) {
				printf("closesocket() failed with error %d\n", WSAGetLastError());
				return 0;
			}
			deleteSK(sk);
			continue;
		}


		if (perIoData->operation == RECEIVE) {
			// process received message here
			perIoData->dataBuff.buf[transferredBytes] = 0;

			if (onReceive != NULL) {
				onReceive(perIoData->dataBuff.buf);
			}
			sk->numberInQueue--;
			if (sk->numberInQueue <= 0) {
				sk->recvMsg();
			}
		}
		else if (perIoData->operation == SEND) {
			perIoData->sentBytes += transferredBytes;
			if (sk->canContinuteSendMsg()) {
				sk->continuteSendMsg();
			}
			else if (sk->canSendNewMsg()) {
				sk->sendNewMsg();
			}
			else if (sk->numberInQueue <= 0) {
				sk->recvMsg();
			}
			else {
				ZeroMemory(&(perIoData->overlapped), sizeof(OVERLAPPED));
				perIoData->recvBytes = 0;
				perIoData->sentBytes = 0;
				perIoData->dataBuff.len = DATA_BUFSIZE;
				perIoData->dataBuff.buf = perIoData->buffer;
				perIoData->operation = RECEIVE;
			}
		}

		sk->waiting = true;
		sk->unlock();
	}
}

void ScreenLogin(SK * client) {
	string menu1[] = { "BACK", "LOGIN" };
	enum menu1 {
		BACK, LOGIN
	};

	printItem(columns / 2 - 2, 2, CLR_NORML, "CARO TAT");
	printItem(columns / 2 - 8, 6, CLR_NORML, "****** LOGIN ******");
	string username, password;
	ShowConsoleCursor(true);
	printItem(columns / 3 + 3, 10, CLR_NORML, "Username: ");
	printItem(columns / 3 + 3, 13, CLR_NORML, "Password: ");
	gotoxy(columns / 3 + 3 + 10, 10);
	getline(cin, username);
	usernamePlayer = username;
	gotoxy(columns / 3 + 3 + 10, 13);
	getline(cin, password);
	ShowConsoleCursor(false);
	int command = getMenu1(menu1);
	system("cls");

	Message *msg;

	switch (command) {
	case LOGIN:
		msg = new Message(REQ_LOGIN, username + "$" + password);
		client->send(msg->toMessageSend());

		break;
	case BACK:
		ScreenMenu(client);
		break;
	}
}

void ScreenMenu(SK * client) {
	string menu[] = { "LOGIN", "REGISTER", "EXIT" };
	enum menu
	{
		LOGIN, REGISTER, EXIT
	};

	system("cls");
	int command = getMenu(menu);
	system("cls");
	switch (command)
	{
	case LOGIN:
		ScreenLogin(client);
		break;
	case REGISTER:
		ScreenRegister(client);
		break;
	case EXIT:
		exit(0);
		break;
	}
}

void ScreenRegister(SK * client) {
	string menu1[] = { "BACK", "REGISTER" };
	enum menu1 {
		BACK, REGISTER
	};

	printItem(columns / 2 - 2, 2, CLR_NORML, "CARO TAT");
	printItem(columns / 2 - 8, 6, CLR_NORML, "****** REGISTER ******");
	string username, password;
	ShowConsoleCursor(true);
	printItem(columns / 3 + 3, 10, CLR_NORML, "Username: ");
	printItem(columns / 3 + 3, 13, CLR_NORML, "Password: ");
	gotoxy(columns / 3 + 3 + 10, 10);
	getline(cin, username);
	gotoxy(columns / 3 + 3 + 10, 13);
	getline(cin, password);
	ShowConsoleCursor(false);
	int command = getMenu1(menu1);
	system("cls");

	Message *msg;

	switch (command) {
	case REGISTER:
		msg = new Message(REQ_REGISTER, username + "$" + password);
		client->send(msg->toMessageSend());

		break;
	case BACK:
		ScreenMenu(client);
		break;
	}
}

vector<pair<string, string>> split(string s, string delimiter1, string delimiter2) {
	vector<pair<string, string>> list;
	if (s == "") {
		return vector<pair<string, string>>();
	}
	size_t pos = 0;
	string token, name = "", elo = "";

	while ((pos = s.find(delimiter1)) != string::npos) {
		token = s.substr(0, pos);
		name = token.substr(0, token.find(delimiter2));
		elo = token.substr(token.find(delimiter2) + 1);
		list.push_back({ name, elo });
		s.erase(0, pos + delimiter1.length());
	}

	name = s.substr(0, s.find(delimiter2));
	elo = s.substr(s.find(delimiter2) + 1);
	list.push_back({ name, elo });

	return list;
}


unsigned int __stdcall timerThread(void *params) {
	while (1) {
		Sleep(1000);
		currMilisec += 1;
		onTick(currMilisec);
	}

	return 1;
}

void workPerHunderedMilisec(long long int milisec) {

	if (!isInGame) return;

	long long int delta = milisec - startTurnTime;

	timeLeft = 20 - delta;

	if (delta < 20) {
		WaitForSingleObject(mutexGame, INFINITE);
		print(WIDTH * 3 + 3, 4, CLR_NORML, "Time left: " + reform(timeLeft, 2));
		ReleaseMutex(mutexGame);
	}
	else {
		for (int i = 0; i < HEIGHT; i++)
		{
			for (int j = 0; j < WIDTH; j++)
			{
				if (board[i][j] == " - ") {
					sock->send(Message(REQ_PLAY_CHESS, reform(i, 2) + "$" + reform(j, 2)).toMessageSend());
					startTurnTime = currMilisec;
					break;
				}
			}
		}
	}
}