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

#define PORT 6000
#define DATA_BUFSIZE 8192
#define SERVER_ADDR "127.0.0.2"
#define RECEIVE 0
#define SEND 1

#pragma comment(lib, "Ws2_32.lib")
using namespace std;

int rows;
int columns;
HANDLE screen = GetStdHandle(STD_OUTPUT_HANDLE);
HANDLE mutexx = CreateMutex(0, 0, 0);

#include "common.h"
#include "Menu.h"
#include "Utilities.h"
#include "Message.h"
#include "Game.h"
#include "screen.h"
#include "main-menu.h"


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
		//cout << "Receive msg" << endl;

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

		//cout << "Continute send" << endl;

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

		//cout << "New msg" << endl;

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

void ScreenMenu(SK *, int);
void ScreenLogin(SK *, int);

void(*onReceive) (string) = NULL;

void something(string a) {
	if (stoi(a.substr(2, 3)) == RES_LOGIN_SUCCESSFUL) {
		system("cls");
	}
	else if (stoi(a.substr(2, 3)) == RES_LOGIN_FAIL){
		
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

	onReceive = something;

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

	SK *sock = new SK(perHandleData, perIoData);
	socks.push_back(sock);
	sock->recvMsg();
	string s;

	while (1) {
		//ScreenMenu(sock, columns);
		COORD coord = { 100, 20 };
		SetConsoleScreenBufferSize(screen, coord);
		//elo
		dataSource.push_back({ "List Player Online", vector<string>() });
		dataSource.push_back({ "List Challenger", vector<string>() });
		dataSource.push_back({ "List Friend Invitation", vector<string>() });
		dataSource.push_back({ "List Challenge Invitation", vector<string>() });
		dataSource.push_back({ "Logout", vector<string>() });

		int selectTitle = 0;
		int selectContent = -1;
		int focusMode = TITLE;

		ShowConsoleCursor(false);

		while (1) {
			system("cls");
			pair<int, int> select = getMenu(selectTitle, -1);
			selectTitle = select.first;
			selectContent = select.second;
			system("cls");
			switch (selectTitle)
			{
			case 0:

				break;
			case 1:

				break;
			case 4:
				exit(0);
				break;
			}
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

//void ScreenLogin(SK * client, int columns) {
//	string menu1[] = { "BACK", "CONTINUE" };
//	enum menu1 {
//		BACK, CONTINUE
//	};
//
//	printItem(columns / 2 - 2, 2, CLR_NORML, "CARO TAT");
//	printItem(columns / 2 - 8, 6, CLR_NORML, "****** LOGIN ******");
//	string username, password;
//	ShowConsoleCursor(true);
//	printItem(columns / 3 + 3, 10, CLR_NORML, "Username: ");
//	printItem(columns / 3 + 3, 13, CLR_NORML, "Password: ");
//	gotoxy(columns / 3 + 3 + 10, 10);
//	getline(cin, username);
//	gotoxy(columns / 3 + 3 + 10, 13);
//	getline(cin, password);
//	ShowConsoleCursor(false);
//	int command = getMenu1(menu1);
//	system("cls");
//
//	Message *msg;
//
//	switch (command) {
//	case CONTINUE:
//		msg = new Message(REQ_LOGIN, username + "$" + password);
//		client->send(msg->toMessageSend());
//		break;
//	case BACK:
//		ScreenMenu(client, columns);
//		break;
//	}
//}
//
//void ScreenMenu(SK * client) {
//	string menu[] = { "LOGIN", "EXIT" };
//	enum menu
//	{
//		LOGIN, EXIT
//	};
//
//	system("cls");
//	int command = getMenu(menu);
//	system("cls");
//	switch (command)
//	{
//	case LOGIN:
//		ScreenLogin(client);
//		break;
//	case EXIT:
//		exit(0);
//		break;
//	}
//}