// SingleIOCPServer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <winsock2.h>
#include <WS2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <process.h>
#include <conio.h>
#include <map>
#include <mutex>
#include <ctype.h>
#include <time.h>
#include <queue>
#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>

using std::map;
using std::string;
using std::ifstream;
using std::ofstream;
using std::stringstream;
using std::ios;

#include "Utilities.h"
#include "Message.h"
#include "File.h"
#include "Account.h"
#include "Match.h"


#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 6000
#define DATA_BUFSIZE 8192
#define RECEIVE 0
#define SEND 1

#pragma comment(lib, "Ws2_32.lib")


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


void				deleteAccount(SOCKET&);
void				lock(HANDLE&);
void				unlock(HANDLE&);

Message				login(SOCKET&, Message&);
Message				logout(SOCKET&, Message&);
Message				getList(SOCKET&, Message&);
Message				addFriend(SOCKET&, Message&);
Message				challenge(SOCKET&, Message&);
Message				acceptChallenge(SOCKET&, Message&);
Message				denyChallenge(SOCKET&, Message&);
Message				startMatch(SOCKET&, Message&);
Message				stopMatch(SOCKET&, Message&);
Message				play(SOCKET&, Message&);
Message				getLog(SOCKET&, Message&);
Message				chat(SOCKET&, Message&);

Message				solveRequest(Account*, Message&);
void				processAccount(Account*);
unsigned __stdcall	serverWorkerThread(LPVOID CompletionPortID);


map<SOCKET, Account*> mapAccounts;


int _tmain(int argc, _TCHAR* argv[])
{
	SOCKADDR_IN serverAddr, clientAddr;
	SOCKET listenSock, acceptSock;
	HANDLE completionPort;
	SYSTEM_INFO systemInfo;
	LPPER_HANDLE_DATA perHandleData;
	LPPER_IO_OPERATION_DATA perIoData;
	DWORD transferredBytes;
	DWORD flags;
	WSADATA wsaData;
	Account *account;

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
	printf("Server startd\nIP: %s\nPORT: %d\n", SERVER_ADDR, SERVER_PORT);
	// Step 4: Create a listening socket
	if ((listenSock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET) {
		printf("WSASocket() failed with error %d\n", WSAGetLastError());
		return 1;
	}

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	inet_pton(AF_INET, SERVER_ADDR, &serverAddr.sin_addr);
	if (bind(listenSock, (PSOCKADDR)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		printf("bind() failed with error %d\n", WSAGetLastError());
		return 1;
	}

	// Prepare socket for listening
	if (listen(listenSock, 20) == SOCKET_ERROR) {
		printf("listen() failed with error %d\n", WSAGetLastError());
		return 1;
	}

	while (1) {
		// Step 5: Accept connections
		if ((acceptSock = WSAAccept(listenSock, (sockaddr*)&clientAddr, NULL, NULL, 0)) == SOCKET_ERROR) {
			printf("WSAAccept() failed with error %d\n", WSAGetLastError());
			continue;
		}

		// Step 6: Create a socket information structure to associate with the socket
		if ((perHandleData = (LPPER_HANDLE_DATA)GlobalAlloc(GPTR, sizeof(PER_HANDLE_DATA))) == NULL) {
			printf("GlobalAlloc() failed with error %d\n", GetLastError());
			continue;
		}

		// Step 7: Associate the accepted socket with the original completion port
		//printf("Socket number %d got connected...\n", acceptSock);
		perHandleData->socket = acceptSock;
		if (CreateIoCompletionPort((HANDLE)acceptSock, completionPort, (DWORD)perHandleData, 0) == NULL) {
			printf("CreateIoCompletionPort() failed with error %d\n", GetLastError());
			continue;
		}


		// Step 8: Create per I/O socket information structure to associate with the WSARecv call
		if ((perIoData = (LPPER_IO_OPERATION_DATA)GlobalAlloc(GPTR, sizeof(PER_IO_OPERATION_DATA))) == NULL) {
			printf("GlobalAlloc() failed with error %d\n", GetLastError());
			continue;
		}

		account = new Account;
		inet_ntop(AF_INET, &clientAddr, account->IP, INET_ADDRSTRLEN);
		account->PORT = ntohs(clientAddr.sin_port);
		mapAccounts.insert({ perHandleData->socket, account });

		ZeroMemory(&(perIoData->overlapped), sizeof(OVERLAPPED));
		perIoData->sentBytes = 0;
		perIoData->recvBytes = 0;
		perIoData->dataBuff.len = DATA_BUFSIZE;
		perIoData->dataBuff.buf = perIoData->buffer;
		perIoData->operation = RECEIVE;
		flags = 0;

		if (WSARecv(acceptSock, &(perIoData->dataBuff), 1, &transferredBytes, &flags, &(perIoData->overlapped), NULL) == SOCKET_ERROR) {
			if (WSAGetLastError() != ERROR_IO_PENDING) {
				printf("WSARecv() failed with error %d\n", WSAGetLastError());
				deleteAccount(acceptSock);
				continue;
			}
		}
	}
	_getch();
	return 0;
}

unsigned __stdcall serverWorkerThread(LPVOID completionPortID)
{
	printf("Thread %d start!\n", GetCurrentThreadId());

	HANDLE completionPort = (HANDLE)completionPortID;

	DWORD transferredBytes;
	LPPER_HANDLE_DATA perHandleData;
	LPPER_IO_OPERATION_DATA perIoData;
	DWORD flags;
	int lenResponse = 0;
	Message response, request;
	char *cResponse = NULL;
	Account *account;

	while (TRUE) {
		if (GetQueuedCompletionStatus(completionPort, &transferredBytes,
			(LPDWORD)&perHandleData, (LPOVERLAPPED *)&perIoData, INFINITE) == 0) {
			printf("GetQueuedCompletionStatus() failed with error %d\n", GetLastError());
			continue;
		}
		if (mapAccounts.find(perHandleData->socket) == mapAccounts.end()) continue;
		account = mapAccounts.at(perHandleData->socket);
		lock(account->mutex);
		// Check to see if an error has occurred on the socket and if so
		// then close the socket and cleanup the SOCKET_INFORMATION structure
		// associated with the socket
		if (transferredBytes == 0) {
			printf("Closing socket %d\n", perHandleData->socket);
			GlobalFree(perHandleData);
			GlobalFree(perIoData);
			unlock(account->mutex);
			deleteAccount(perHandleData->socket);
			continue;
		}

		// Check to see if the operation field equals RECEIVE. If this is so, then
		// this means a WSARecv call just completed so update the recvBytes field
		// with the transferredBytes value from the completed WSARecv() call
		if (perIoData->operation == RECEIVE) {
			saveMessage(perIoData->dataBuff.buf, account->restMessage, account->requests);
		}
		else if (perIoData->operation == SEND) {
			perIoData->sentBytes += transferredBytes;
		}

		// if queue of request not empty and processed request before completely
		// process new request
		if (!account->requests.empty() && perIoData->recvBytes <= perIoData->sentBytes) {
			request = account->requests.front();
			account->requests.pop();
			response = solveRequest(account, request);

			string tmp = response.toMessageSend();

			lenResponse = tmp.length();

			if (cResponse != NULL) {
				delete[] cResponse;
				cResponse = NULL;
			}

			cResponse = new char[lenResponse + 1];
			strcpy_s(cResponse, lenResponse + 1, tmp.c_str());
			cResponse[lenResponse] = NULL;

			perIoData->recvBytes = lenResponse;
			perIoData->sentBytes = 0;
			perIoData->operation = SEND;
		}

		if (perIoData->recvBytes > perIoData->sentBytes) {
			// Post another WSASend() request.
			// Since WSASend() is not guaranteed to send all of the bytes requested,
			// continue posting WSASend() calls until all received bytes are sent.
			ZeroMemory(&(perIoData->overlapped), sizeof(OVERLAPPED));
			perIoData->dataBuff.buf = cResponse + perIoData->sentBytes;
			perIoData->dataBuff.len = perIoData->recvBytes - perIoData->sentBytes;
			perIoData->operation = SEND;


			if (WSASend(perHandleData->socket, &(perIoData->dataBuff), 1, &transferredBytes, 0, &(perIoData->overlapped), NULL) == SOCKET_ERROR) {
				if (WSAGetLastError() != ERROR_IO_PENDING) {
					printf("WSASend() failed with error %d\n", WSAGetLastError());
					unlock(account->mutex);
					deleteAccount(perHandleData->socket);
					continue;
				}
			}
		}
		else if (account->requests.empty()) {
			// No more bytes to send post another WSARecv() request
			ZeroMemory(&(perIoData->overlapped), sizeof(OVERLAPPED));
			ZeroMemory(perIoData->buffer, sizeof(perIoData->buffer));
			if (cResponse != NULL) {
				delete[] cResponse;
				cResponse = NULL;
			}
			perIoData->recvBytes = 0;
			perIoData->sentBytes = 0;
			perIoData->operation = RECEIVE;
			perIoData->dataBuff.buf = perIoData->buffer;
			perIoData->dataBuff.len = DATA_BUFSIZE;
			flags = 0;

			if (WSARecv(perHandleData->socket,
				&(perIoData->dataBuff),
				1,
				&transferredBytes,
				&flags,
				&(perIoData->overlapped), NULL) == SOCKET_ERROR) {
				if (WSAGetLastError() != ERROR_IO_PENDING) {
					printf("WSARecv() failed with error %d\n", WSAGetLastError());
					unlock(account->mutex);
					deleteAccount(perHandleData->socket);
					continue;
				}
			}
		}
		unlock(account->mutex);
	}

	printf("Thread %d stop!\n", GetCurrentThreadId());
}


void lock(HANDLE &mutex) {
	WaitForSingleObject(mutex, INFINITE);
}

void unlock(HANDLE &mutex) {
	ReleaseMutex(mutex);
}

/**
* @function deleteAccount: delete account and socket
* @param socket: socket need to delete
*
(**/
void deleteAccount(SOCKET &socket) {
	if (mapAccounts.find(socket) == mapAccounts.end()) return;
	Account *acc = mapAccounts[socket];
	CloseHandle(acc->mutex);
	mapAccounts.erase(socket);
	while (!acc->requests.empty()) acc->requests.pop();
	delete acc;
	closesocket(socket);
}

/*
* @function login: solve login command of the account with the request
* @param account: the account need to solve request
* @param request: the request of the account
*
* @param: a message brings result code
**/
Message login(SOCKET &socket, Message &request) {
	Account *account = mapAccounts[socket];
	Message response(RESPONSE, reform(RES_UNDENTIFY_RESULT, SIZE_RESPONSE_CODE));

	int statusAccount = getStatus(request);

	//if the account already logged 
	if (account->signInStatus == LOGGED) {
		saveLog(account, request, RES_LOGIN_FAIL_ALREADY_LOGIN);
		response.content = std::to_string(RES_LOGIN_FAIL_ALREADY_LOGIN);
		return response;
	}

	//check status of account
	switch (getStatus(request)) {
	case ACTIVE:
		account->username = request.content;
		account->signInStatus = LOGGED;
		saveLog(account, request, RES_LOGIN_SUCCESSFUL);
		response.content = std::to_string(RES_LOGIN_SUCCESSFUL);
		break;

	case LOCKED:
		saveLog(account, request, RES_LOGIN_FAIL_LOCKED);
		response.content = std::to_string(RES_LOGIN_FAIL_LOCKED);
		break;

	case NOT_EXIST:
		saveLog(account, request, RES_LOGIN_FAIL_NOT_EXIST);
		response.content = std::to_string(RES_LOGIN_FAIL_NOT_EXIST);
		break;
	}

	return response;
}




/*
* @function logout: solve logout command of the account with the request
* @param account: the account need to solve request
* @param request: the request of the account
*
* @param: a message brings result code
**/
Message logout(SOCKET &socket, Message &request) {
	Account *account = mapAccounts[socket];
	Message response(RESPONSE, reform(RES_UNDENTIFY_RESULT, SIZE_RESPONSE_CODE));

	if (account->signInStatus == NOT_LOGGED) {
		response.content = std::to_string(RES_LOGOUT_FAIL);
		saveLog(account, request, RES_LOGOUT_FAIL);
	}
	else {
		account->signInStatus = NOT_LOGGED;
		response.content = std::to_string(RES_LOGOUT_SUCCESSFUL);
		saveLog(account, request, RES_LOGOUT_SUCCESSFUL);
	}

	return response;
}

/**Solve Get List request**/
Message getList(SOCKET &socket, Message &request) {
	Account *account = mapAccounts[socket];
	Message response(RESPONSE, reform(RES_UNDENTIFY_RESULT, SIZE_RESPONSE_CODE));


	return response;
}


/**Solve Add Friend request**/
Message addFriend(SOCKET &socket, Message &request) {
	Account *account = mapAccounts[socket];
	Message response(RESPONSE, reform(RES_UNDENTIFY_RESULT, SIZE_RESPONSE_CODE));


	return response;
}


/**Solve Challenge request**/
Message challenge(SOCKET &socket, Message &request) {
	Account *account = mapAccounts[socket];
	Message response(RESPONSE, reform(RES_UNDENTIFY_RESULT, SIZE_RESPONSE_CODE));


	return response;
}


/**Solve Accept Challenge request**/
Message acceptChallenge(SOCKET &socket, Message &request) {
	Account *account = mapAccounts[socket];
	Message response(RESPONSE, reform(RES_UNDENTIFY_RESULT, SIZE_RESPONSE_CODE));


	return response;
}

/**Solve Deny Challenge request**/
Message denyChallenge(SOCKET &socket, Message &request) {
	Account *account = mapAccounts[socket];
	Message response(RESPONSE, reform(RES_UNDENTIFY_RESULT, SIZE_RESPONSE_CODE));


	return response;
}


/**Solve Start Match request**/
Message startMatch(SOCKET &socket, Message &request) {
	Account *account = mapAccounts[socket];
	Message response(RESPONSE, reform(RES_UNDENTIFY_RESULT, SIZE_RESPONSE_CODE));


	return response;
}


/**Solve Stop Match request**/
Message stopMatch(SOCKET &socket, Message &request) {
	Account *account = mapAccounts[socket];
	Message response(RESPONSE, reform(RES_UNDENTIFY_RESULT, SIZE_RESPONSE_CODE));


	return response;
}


/**Solve Play request**/
Message play(SOCKET &socket, Message &request) {
	Account *account = mapAccounts[socket];
	Message response(RESPONSE, reform(RES_UNDENTIFY_RESULT, SIZE_RESPONSE_CODE));


	return response;
}


/**Solve Get Log request**/
Message getLog(SOCKET &socket, Message &request) {
	Account *account = mapAccounts[socket];
	Message response(RESPONSE, reform(RES_UNDENTIFY_RESULT, SIZE_RESPONSE_CODE));


	return response;
}
 

/**Solve Chat request**/
Message chat(SOCKET &socket, Message &request) {
	Account *account = mapAccounts[socket];
	Message response(RESPONSE, reform(RES_UNDENTIFY_RESULT, SIZE_RESPONSE_CODE));


	return response;
}




/*
* @function solve: solve request of the account
* @param account: the account need to solve request
* @param request: the request of account
*
* @return: the Message struct brings result code
**/
Message solveRequest(SOCKET &socket, Message &request) {
	Account *account = mapAccounts[socket];
	Message response(RESPONSE, reform(RES_UNDENTIFY_RESULT, SIZE_RESPONSE_CODE));

	switch (request.command) {
	case REQ_LOGIN:
		response = login(socket, request);
		break;
	case REQ_LOGOUT:
		response = logout(socket, request);
		break;
	case REQ_GET_LIST:
		response = getList(socket, request);
		break;
	case REQ_ADD_FRIEND:
		response = addFriend(socket, request);
		break;
	case REQ_CHALLENGE:
		response = challenge(socket, request);
		break;
	case REQ_ACCEPT_CHALLENGE:
		response = acceptChallenge(socket, request);
		break;
	case REQ_DENY_CHALLENGE:
		response = denyChallenge(socket, request);
	case REQ_START_MATCH:
		response = startMatch(socket, request);
		break;
	case REQ_STOP_MATCH:
		response = stopMatch(socket, request);
		break;
	case REQ_PLAY:
		response = play(socket, request);
		break;
	case REQ_GET_LOG:
		response = getLog(socket, request);
		break;
	case REQ_CHAT:
		response = chat(socket, request);
		break;
	case RESPONSE:
		break;
	default:
		saveLog(account, request, RES_UNDENTIFY_RESULT);
		break;
	}

	return response;
}