/**
*
*	Create by Thuyetln
*	Date: 31/07/2021
*
**/

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
		cout << "Receive msg" << endl;

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
	
		cout << "Continute send" << endl;

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

		cout << "New msg" << endl;

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

void(*onReceive) (string) = NULL;

void something(string a) {
	cout << a << endl;
}

int _tmain(int argc, _TCHAR* argv[])
{
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
		cout << "send msg: ";
		getline(cin, s);
		sock->send(s + "\r\n");
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
		if (sk != NULL) {
			cout << sk->perIoData->operation << " ..." << endl;
		}
		if (GetQueuedCompletionStatus(completionPort, &transferredBytes,
			(LPDWORD)&perHandleData, (LPOVERLAPPED *)&perIoData, INFINITE) == 0) {
			printf("GetQueuedCompletionStatus() failed with error %d\n", GetLastError());
			return 0;
		}
		
		cout << "Operator: " << perIoData->operation << " Transfer:" << transferredBytes << endl;
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
			cout << perIoData->sentBytes << " " << perIoData->recvBytes << " " << sk->needSendMessages.size() <<endl;
			if (sk->canContinuteSendMsg()) {
				cout << "Continute" << endl;
				sk->continuteSendMsg();
			}
			else if (sk->canSendNewMsg()) {
				cout << "New Send" << endl;
				sk->sendNewMsg();
			}
			else if (sk->numberInQueue <= 0) {
				cout << "Recv" << endl;
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