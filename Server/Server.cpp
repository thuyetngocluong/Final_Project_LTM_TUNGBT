// SingleIOCPServer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdexcept>
#include <memory>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <process.h>
#include <conio.h>
#include <map>
#include <set>
#include <SQLAPI.h>

#include <ctype.h>
#include <time.h>
#include <queue>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <utility>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

#define SERVER_ADDR "127.0.0.2"
#define SERVER_PORT 6000
#define DATA_BUFSIZE 8192
#define RECEIVE 0
#define SEND 1


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


#include "Protocol.h"
#include "Utilities.h"
#include "Message.h"
#include "Player.h"
#include "Database.h"
#include "File.h"
#include "Match.h"


void				deleteAccount(Account*);
Account*			findAccount(string);
Account*			findAccount(LPPER_HANDLE_DATA);

void				solveReqLogin(Account*, Message&);
void				solveReqLogout(Account*, Message&);
void				solveReqGetListFriend(Account*, Message&);
void				solveReqSendFriendInvitation(Account*, Message&);
void				solveReqSendChallengeInvitation(Account*, Message&);
void				getLog(Account*, Message&);
void				solveReqChat(Account*, Message&);

void				solveReqStopMatch(Account*, Message&);
void				solveReqPlay(Account*, Message&);
void				startGame(Match *match);
void				endGame(Match *match);

void				solveResponseFromClient(Account*, Message&);
void				solveRequest(Account*, Message&);

void				processAccount(Account*);
unsigned __stdcall	serverWorkerThread(LPVOID CompletionPortID);

#include "Account.h"

set<Account*> accounts;
Account *account = NULL;
static map<pair<Account*, Account*>, Match*> mapMatch;

bool sendingRequest = false;
HANDLE mutexSendingRequest;

Data *database;


int _tmain(int argc, _TCHAR* argv[])
{
	database = Data::getInstance();

	SOCKADDR_IN serverAddr, clientAddr;
	SOCKET listenSock, acceptSock;
	HANDLE completionPort;
	SYSTEM_INFO systemInfo;
	LPPER_HANDLE_DATA perHandleData;
	LPPER_IO_OPERATION_DATA perIoData;
	DWORD transferredBytes;
	DWORD flags;
	WSADATA wsaData;
	

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

		account = new Account(perHandleData, perIoData);
		inet_ntop(AF_INET, &clientAddr, account->IP, INET_ADDRSTRLEN);
		account->PORT = ntohs(clientAddr.sin_port);
		accounts.insert(account);

		account->recvMsg();

		Sleep(5000);
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
	Account *account = NULL;

	while (TRUE) {
		if (account != NULL) {
			account->lock();
			account->waiting = true;
			account->unlock();
		}

		if (GetQueuedCompletionStatus(completionPort, &transferredBytes,
			(LPDWORD)&perHandleData, (LPOVERLAPPED *)&perIoData, INFINITE) == 0) {
			printf("GetQueuedCompletionStatus() failed with error %d\n", GetLastError());
			continue;
		}

		account = findAccount(perHandleData);
		if (account == NULL) continue;

		account->lock();
		account->waiting = false;

		// Check to see if an error has occurred on the socket and if so
		// then close the socket and cleanup the SOCKET_INFORMATION structure
		// associated with the socket
		if (transferredBytes == 0 && perIoData->operation != SEND) {
			printf("Closing socket %d\n", perHandleData->socket);
			GlobalFree(perHandleData);
			GlobalFree(perIoData);
			deleteAccount(account);
			continue;
		}

		

		if (perIoData->operation == RECEIVE) {
			account->numberInQueue -= 1;
			// process received message here
			perIoData->dataBuff.buf[transferredBytes] = 0;
			saveMessage(perIoData->dataBuff.buf, account->restMessage, account->requests);

			if (account->requests.empty() && account->numberInQueue <= 0) {
				account->recvMsg();
			} 

			while (!account->requests.empty()) {
				Message request = account->requests.front();
				account->requests.pop();
				cout << "Recv: " << request.toMessageSend() << endl;
				if (request.command == RESPONSE_TO_SERVER) {
					solveResponseFromClient(account, request);
				}
				else {
					solveRequest(account, request);
				} 
			}
		}
		else if (perIoData->operation == SEND) {
			perIoData->sentBytes += transferredBytes;

			if (account->canContinuteSendMsg()) {
				account->continuteSendMsg();
			}
			else if (account->canSendNewMsg()) {
				account->sendNewMsg();
			}
			else if (account->numberInQueue <= 0) {
				cout << "Sent" << endl;
				account->recvMsg();
			}
		}

		account->unlock();
	}

	printf("Thread %d stop!\n", GetCurrentThreadId());
}



Account* findAccount(string username) {
	for (auto i = accounts.begin(); i != accounts.end(); i++) {
		if ((*i)->username.compare(username) == 0) {
			return (*i);
		}
	}

	return NULL;
}

Account* findAccount(LPPER_HANDLE_DATA perHandleData) {
	for (auto i = accounts.begin(); i != accounts.end(); i++) {
		if ((*i)->perHandleData == perHandleData) {
			return (*i);
		}
	}

	return NULL;
}

/**
* @function deleteAccount: delete account and socket
* @param socket: socket need to delete
*
(**/
void deleteAccount(Account *account) {
	CloseHandle(account->mutex);
	accounts.erase(account);
	while (!account->requests.empty()) account->requests.pop();
	while (!account->messagesNeesToSend.empty()) account->messagesNeesToSend.pop();
	closesocket(account->perHandleData->socket);
	delete account;
}

/*
* @function login: solve login command of the account with the request
* @param account: the account need to solve request
* @param request: the request of the account
*
* @param: a message brings result code
**/
void solveReqLogin(Account *account, Message &request) {
	Message response(RESPONSE_TO_CLIENT, reform(UNDENTIFIED, SIZE_RESPONSE_CODE));
	string content = request.content;
	size_t found = content.find("$");
	string username = content.substr(0, found);
	string password = content.substr(found + 1);

	//if the account already logged 
	if (database->login(username, password)) {
		account->username = username;
		response.content = reform(LOGIN_SUCCESSFUL, SIZE_RESPONSE_CODE);
		vector<Player> listFr = database->getListFriend(database->getPlayerByName(account->username));
		int i = 0;
		string listFrReform = "";
		for (auto player = listFr.begin(); player != listFr.end(); player++) {
			i++;
			listFrReform += player->getUsername() + "&" + to_string(player->getElo());
			if (i != listFr.size()) listFrReform += "$";
		}

		Message ms1(RESPONSE_TO_CLIENT, listFrReform);

		cout << "Pending send: " << response.toMessageSend() << ms1.toMessageSend() << endl;

		account->addMessageNeedToSend(response); // send response
		account->addMessageNeedToSend(ms1); // send list friend
	}
	else {
		account->addMessageNeedToSend(response);
		response.content = reform(LOGIN_FAIL, SIZE_RESPONSE_CODE);
	}

}


/*
* @function logout: solve logout command of the account with the request
* @param account: the account need to solve request
* @param request: the request of the account
*
* @param: a message brings result code
**/
void solveReqLogout(Account *account, Message &request) {
	Message response(RESPONSE_TO_CLIENT, reform(UNDENTIFIED, SIZE_RESPONSE_CODE));

	if (account->signInStatus == NOT_LOGGED) {
		response.content = reform(UNDENTIFIED, LOGOUT_FAIL);
		saveLog(account, request, LOGOUT_FAIL);
	}
	else {
		account->signInStatus = NOT_LOGGED;
		response.content = reform(UNDENTIFIED, LOGOUT_SUCCESSFUL);
		saveLog(account, request, LOGOUT_SUCCESSFUL);
	}

	account->addMessageNeedToSend(response);

}


/**Solve Register Account request**/
void solveReqRegisterAccount(Account *account, Message &request) {
	Message response(RESPONSE_TO_CLIENT, reform(UNDENTIFIED, SIZE_RESPONSE_CODE));

	if (solveReqRegisterAccount(request)) {
		response.content = reform(REGISTER_SUCCESSFUL, SIZE_RESPONSE_CODE);
	}
	else {
		response.content = reform(REGISTER_FAIL, SIZE_RESPONSE_CODE);
	}

}


/**Solve Get List request**/
void solveReqGetListFriend(Account *account, Message &request) {
	Message response(RESPONSE_TO_CLIENT, reform(UNDENTIFIED, SIZE_RESPONSE_CODE));

	vector<Player> listFr = database->getListFriend(database->getPlayerByName(account->username));
	string listFrReform = "";
	int i = 0;

	for (auto player = listFr.begin(); player != listFr.end(); player++) {
		i++;
		listFrReform += player->getUsername() + "&" + to_string(player->getElo());
		if (i != listFr.size()) listFrReform += "$";
	}

	response.content = reform(GET_LIST_FRIEND_SUCCESSFUL, SIZE_RESPONSE_CODE) + listFrReform;

	account->addMessageNeedToSend(response);

}


/**Solve Add Friend request**/
void solveReqSendFriendInvitation(Account *account, Message &request) {
	Message response(RESPONSE_TO_CLIENT, reform(UNDENTIFIED, SIZE_RESPONSE_CODE));

	SOCKET sock = account->perHandleData->socket;

	if (sock == NULL) {
		response.content = reform(SEND_FRIEND_INVITATION_FAIL, SIZE_RESPONSE_CODE);
		account->addMessageNeedToSend(response);
	}
	else {
		Message requestToFr(SREQ_SEND_FRIEND_INVITATION, account->username);

		response.content = reform(SEND_FRIEND_INVITATION_SUCCESSFUL, SIZE_RESPONSE_CODE);

		account->addMessageNeedToSend(response);
		account->addMessageNeedToSend(requestToFr);
	}


}


/**Solve Challenge request**/
void solveReqSendChallengeInvitation(Account *account, Message &request) {
	Message response(RESPONSE_TO_CLIENT, reform(UNDENTIFIED, SIZE_RESPONSE_CODE));

	SOCKET sock = account->perHandleData->socket;

	if (sock == NULL) {
		response.content = reform(SEND_CHALLENGE_INVITATION_FAIL, SIZE_RESPONSE_CODE);
		account->addMessageNeedToSend(response);
	}
	else {
		Message requestToFr(SREQ_SEND_CHALLENGE_INVITATION, account->username);


		response.content = reform(SEND_CHALLENGE_INVITATION_SUCCESSFUL, SIZE_RESPONSE_CODE);
		account->addMessageNeedToSend(response);
		account->addMessageNeedToSend(requestToFr);
	}


}


/**Solve Get Log request**/
void getLog(Account *account, Message &request) {
	Message response(RESPONSE_TO_CLIENT, reform(UNDENTIFIED, SIZE_RESPONSE_CODE));


}


/**Solve Chat request**/
void solveReqChat(Account *account, Message &request) {
	Message response(RESPONSE_TO_CLIENT, reform(UNDENTIFIED, SIZE_RESPONSE_CODE));


	
}


/**Solve Get request**/
void solveResAcceptFriendInvitation(Account *account, Message &response) {
	Message resp(RESPONSE_TO_CLIENT, reform(UNDENTIFIED, SIZE_RESPONSE_CODE));
	SOCKET sock = account->perHandleData->socket;

	if (sock == NULL) {
		resp.content = reform(SEND_FRIEND_INVITATION_FAIL, SIZE_RESPONSE_CODE);

	}
	else {
		Message requestToFr(ACCEPT_FRIEND_INVITATION, account->username);

		resp.content = reform(SEND_FRIEND_INVITATION_SUCCESSFUL, SIZE_RESPONSE_CODE);
	}

}

/**Solve Get request**/
void solveResDenyFriendInvitation(Account *account, Message &response) {
	SOCKET sock = account->perHandleData->socket;
	Message resp(RESPONSE_TO_CLIENT, reform(UNDENTIFIED, SIZE_RESPONSE_CODE));

	if (sock == NULL) {
		resp.content = reform(SEND_FRIEND_INVITATION_FAIL, SIZE_RESPONSE_CODE);
	}
	else {
		Message requestToFr(DENY_FRIEND_INVITATION, account->username);

		resp.content = reform(SEND_FRIEND_INVITATION_SUCCESSFUL, SIZE_RESPONSE_CODE);
	}


}

/**Solve Get request**/
void solveResAcceptChallengeInvitation(Account *account, Message &response) {
	SOCKET sock = account->perHandleData->socket;
	Message resp(RESPONSE_TO_CLIENT, reform(UNDENTIFIED, SIZE_RESPONSE_CODE));

	if (sock == NULL) {
		resp.content = reform(SEND_CHALLENGE_INVITATION_FAIL, SIZE_RESPONSE_CODE);
	}
	else {
		Message requestToFr(ACCEPT_CHALLENGE_INVITATION, account->username);

		resp.content = reform(SEND_CHALLENGE_INVITATION_SUCCESSFUL, SIZE_RESPONSE_CODE);
	}

}

/**Solve Get request**/
void solveResDenyChallengInvitation(Account *account, Message &response) {
	SOCKET sock = account->perHandleData->socket;
	Message resp(RESPONSE_TO_CLIENT, reform(UNDENTIFIED, SIZE_RESPONSE_CODE));

	if (sock == NULL) {
		resp.content = reform(SEND_CHALLENGE_INVITATION_FAIL, SIZE_RESPONSE_CODE);
	}
	else {
		Message requestToFr(DENY_CHALLENGE_INVITATION, account->username);
		resp.content = reform(SEND_CHALLENGE_INVITATION_SUCCESSFUL, SIZE_RESPONSE_CODE);
	}
}



///////////////////////////// In Gamme /////////////////////////////

/**Solve Stop Match request**/
void solveReqStopMatch(Account *account, Message &request) {
	Message response(RESPONSE_TO_CLIENT, reform(UNDENTIFIED, SIZE_RESPONSE_CODE));
	//
	// get database
	// solve request
	//

	

}


/**Solve Play request**/
void solveReqPlay(Account *account, Message &request) {
	Message response(RESPONSE_TO_CLIENT, reform(UNDENTIFIED, SIZE_RESPONSE_CODE));

}


/** Send command StartGame to client**/
void startGame(Match *match) {
 
}

/** Send command EndGame to client**/
void endGame(Match *match) {

}

void sloveResRecivedRequestStartGame(Account *account, Message &request) {

}


/////////////////////////////////////////////////////////////

/*
* @function solve: solve request of the account
* @param account: the account need to solve request
* @param request: the request of account
*
* @return: the Message struct brings result code
**/
void solveRequest(Account *account, Message &request) {
	switch (request.command) {
	case CREQ_LOGIN:
		solveReqLogin(account, request);
		break;
	case CREQ_LOGOUT:
		solveReqLogout(account, request);
		break;
	case CREQ_REGISTER:
		solveReqRegisterAccount(account, request);
		break;
	case CREQ_GET_LIST_FRIEND:
		solveReqGetListFriend(account, request);
		break;
	case CREQ_SEND_FRIEND_INVITATION:
		solveReqSendFriendInvitation(account, request);
		break;
	case CREQ_SEND_CHALLENGE_INVITATION:
		solveReqSendChallengeInvitation(account, request);
		break;
	case CREQ_STOP_GAME:
		solveReqStopMatch(account, request);
		break;
	case CREQ_PLAY:
		solveReqPlay(account, request);
		break;
	case CREQ_CHAT:
		solveReqChat(account, request);
		break;
	default:
		saveLog(account, request, UNDENTIFIED);
		break;
	}
}

void solveResponseFromClient(Account *account, Message &response) {
	int statusCode = atoi(response.content.substr(0, SIZE_RESPONSE_CODE).c_str());

	if (statusCode == 0) {
		Message resp(RESPONSE_TO_CLIENT, reform(UNDENTIFIED, SIZE_RESPONSE_CODE));
		account->addMessageNeedToSend(resp);
		return;
	};

	switch (statusCode) {
	case ACCEPT_FRIEND_INVITATION:
		solveResAcceptFriendInvitation(account, response);
		break;
	case DENY_FRIEND_INVITATION:
		solveResDenyFriendInvitation(account, response);
		break;
	case ACCEPT_CHALLENGE_INVITATION:
		solveResAcceptChallengeInvitation(account, response);
		break;
	case DENY_CHALLENGE_INVITATION:
		solveResDenyChallengInvitation(account, response);
		break;
	case RECEIVED_REQUEST_START_GAME:
		sloveResRecivedRequestStartGame(account, response);
		break;
	case RECEIVED_REQUEST_END_GAME:
		break;
	}


}