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
#define DATA_BUFSIZE 65535
#define RECEIVE 0
#define SEND 1


// Structure definition
typedef struct {
	WSAOVERLAPPED overlapped;
	WSABUF dataBuff;
	char buffer[DATA_BUFSIZE];
	streamsize bufLen;
	streamsize recvBytes;
	streamsize sentBytes;
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

void				solveLoginReq(Account*, Message&);
void				solveLogoutReq(Account*, Message&);
void				solveGetListFriendReq(Account*, Message&);
void				solveSendFriendInvitationReq(Account*, Message&);
void				solveSendChallengeInvitationReq(Account*, Message&);
void				getLog(Account*, Message&);
void				solveChatReq(Account*, Message&);

void				solveStopMatchReq(Account*, Message&);
void				solvePlayReq(Account*, Message&);
void				startGame(Match *match);
void				endGame(Match *match);

void				solveResponseFromClient(Account*, Message&);
void				solveRequest(Account*, Message&);

void				processAccount(Account*);
unsigned __stdcall	serverWorkerThread(LPVOID CompletionPortID);
void				receiveMessage(Account*, char*, int);
void				newClientConnect(LPPER_HANDLE_DATA, LPPER_IO_OPERATION_DATA, string, int);
void				clientDisconnect(Account*);
Account*			findAccount(LPPER_HANDLE_DATA);

#include "Account.h"
#include "CompletionPortServer.h"

set<Account*> accounts;
Account *account = NULL;
static map<pair<Account*, Account*>, Match*> mapMatch;

bool sendingRequest = false;
HANDLE mutexSendingRequest;

Data *database;


int _tmain(int argc, _TCHAR* argv[])
{
	database = Data::getInstance();

	getAccountHasEvent = findAccount;

	onClientDisconnect = clientDisconnect;

	onReceive = receiveMessage;

	onNewClientConnect = newClientConnect;

	createServer(SERVER_ADDR, SERVER_PORT);


	_getch();
	return 0;
}

void newClientConnect(LPPER_HANDLE_DATA perHandleData, LPPER_IO_OPERATION_DATA perIoData, string ip, int port) {
	account = new Account(perHandleData, perIoData);
	account->IP = ip;
	account->PORT = port;
	accounts.insert(account);
	//account->recvMsg();

	Sleep(2000);
	sendFile("test1.rar", account);
}

void clientDisconnect(Account *account) {
	CloseHandle(account->mutex);
	accounts.erase(account);
	while (!account->requests.empty()) account->requests.pop();
	while (!account->messagesNeesToSend.empty()) account->messagesNeesToSend.pop();
	closesocket(account->perHandleData->socket);
	delete account;
}

void receiveMessage(Account* account, char* msg, int length) {

	messageToRequest(msg, account->restMessage, account->requests);

	if (account->requests.empty() && account->numberReceiveInQueue <= 0) {
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
void solveLoginReq(Account *account, Message &request) {
	Message response(RESPONSE_TO_CLIENT, reform(UNDENTIFIED, SIZE_RESPONSE_CODE));
	string content = request.content;
	size_t found = content.find("$");
	string username = content.substr(0, found);
	string password = content.substr(found + 1);

	//if the account already logged 
	if (database->login(username, password)) {
		account->username = username;
		response.content = reform(S2C_LOGIN_SUCCESSFUL, SIZE_RESPONSE_CODE);
		vector<Player> listFr = database->getListFriend(database->getPlayerByName(account->username));
		int i = 0;
		string listFrReform = "";
		for (auto player = listFr.begin(); player != listFr.end(); player++) {
			i++;
			listFrReform += player->getUsername() + "&" + to_string(player->getElo());
			if (i != listFr.size()) listFrReform += "$";
		}

		Message ms1(RESPONSE_TO_CLIENT, listFrReform);

		account->send(response); // send response
		account->send(ms1); // send list friend
	}
	else {
		response.content = reform(S2C_LOGIN_FAIL, SIZE_RESPONSE_CODE);
		account->send(response);
	}

}


/*
* @function logout: solve logout command of the account with the request
* @param account: the account need to solve request
* @param request: the request of the account
*
* @param: a message brings result code
**/
void solveLogoutReq(Account *account, Message &request) {
	Message response(RESPONSE_TO_CLIENT, reform(UNDENTIFIED, SIZE_RESPONSE_CODE));

	if (account->signInStatus == NOT_LOGGED) {
		response.content = reform(UNDENTIFIED, S2C_LOGOUT_FAIL);
		saveLog(account, request, S2C_LOGOUT_FAIL);
	}
	else {
		account->signInStatus = NOT_LOGGED;
		response.content = reform(UNDENTIFIED, S2C_LOGOUT_SUCCESSFUL);
		saveLog(account, request, S2C_LOGOUT_SUCCESSFUL);
	}

	account->send(response);

}


/**Solve Register Account request**/
void solveReqRegisterAccount(Account *account, Message &request) {
	Message response(RESPONSE_TO_CLIENT, reform(UNDENTIFIED, SIZE_RESPONSE_CODE));
	size_t found = request.content.find("$");
	string username = request.content.substr(0, found);
	string password = request.content.substr(found + 1);
	
	if (database->registerAccount(username, password)) {
		response.content = reform(S2C_REGISTER_SUCCESSFUL, SIZE_RESPONSE_CODE);
	}
	else {
		response.content = reform(S2C_REGISTER_FAIL, SIZE_RESPONSE_CODE);
	}

	account->send(response);

}


/**Solve Get List request**/
void solveGetListFriendReq(Account *account, Message &request) {
	Message response(RESPONSE_TO_CLIENT, reform(UNDENTIFIED, SIZE_RESPONSE_CODE));

	vector<Player> listFr = database->getListFriend(database->getPlayerByName(account->username));
	string listFrReform = "";
	int i = 0;

	for (auto player = listFr.begin(); player != listFr.end(); player++) {
		i++;
		listFrReform += player->getUsername() + "&" + to_string(player->getElo());
		if (i != listFr.size()) listFrReform += "$";
	}

	response.content = reform(S2C_GET_LIST_FRIEND_SUCCESSFUL, SIZE_RESPONSE_CODE) + listFrReform;

	account->send(response);

}


/**Solve Add Friend request**/
void solveSendFriendInvitationReq(Account *account, Message &request) {
	Message response(RESPONSE_TO_CLIENT, reform(UNDENTIFIED, SIZE_RESPONSE_CODE));

	Account *userInvited = findAccount(request.content);

	if (userInvited == NULL) {
		response.content = reform(S2C_SEND_FRIEND_INVITATION_FAIL, SIZE_RESPONSE_CODE);
		account->send(response);
	}
	else {
		Message requestToFr(S2C_SEND_FRIEND_INVITATION, account->username);

		response.content = reform(S2C_SEND_FRIEND_INVITATION_SUCCESSFUL, SIZE_RESPONSE_CODE);

		account->send(response);
		userInvited->send(requestToFr);
	}


}


/**Solve Challenge request**/
void solveSendChallengeInvitationReq(Account *account, Message &request) {
	Message response(RESPONSE_TO_CLIENT, reform(UNDENTIFIED, SIZE_RESPONSE_CODE));

	Account *userChallenged = findAccount(request.content);


	if (userChallenged == NULL) {
		response.content = reform(S2C_SEND_CHALLENGE_INVITATION_FAIL, SIZE_RESPONSE_CODE);
		account->send(response);
	}
	else {
		Message requestToFr(S2C_SEND_CHALLENGE_INVITATION, account->username);

		response.content = reform(S2C_SEND_CHALLENGE_INVITATION_SUCCESSFUL, SIZE_RESPONSE_CODE);

		account->send(response);
		userChallenged->send(requestToFr);
	}


}


/**Solve Get Log request**/
void getLog(Account *account, Message &request) {
	Message response(RESPONSE_TO_CLIENT, reform(UNDENTIFIED, SIZE_RESPONSE_CODE));


}


/**Solve Chat request**/
void solveChatReq(Account *account, Message &request) {
	Message response(RESPONSE_TO_CLIENT, reform(UNDENTIFIED, SIZE_RESPONSE_CODE));


	
}


/**Solve Get request**/
void solveResAcceptFriendInvitation(Account *account, Message &response) {
	Message resp(RESPONSE_TO_CLIENT, reform(UNDENTIFIED, SIZE_RESPONSE_CODE));

	Account *userAccepted = findAccount(response.content);

	if (userAccepted == NULL) {
		resp.content = reform(S2C_SEND_FRIEND_INVITATION_FAIL, SIZE_RESPONSE_CODE);

	}
	else {
		Message requestToFr(C2S_ACCEPT_FRIEND_INVITATION, account->username);

		resp.content = reform(S2C_SEND_FRIEND_INVITATION_SUCCESSFUL, SIZE_RESPONSE_CODE);
	}

}

/**Solve Get request**/
void solveResDenyFriendInvitation(Account *account, Message &response) {
	SOCKET sock = account->perHandleData->socket;
	Message resp(RESPONSE_TO_CLIENT, reform(UNDENTIFIED, SIZE_RESPONSE_CODE));

	if (sock == NULL) {
		resp.content = reform(S2C_SEND_FRIEND_INVITATION_FAIL, SIZE_RESPONSE_CODE);
	}
	else {
		Message requestToFr(C2S_DENY_FRIEND_INVITATION, account->username);

		resp.content = reform(S2C_SEND_FRIEND_INVITATION_SUCCESSFUL, SIZE_RESPONSE_CODE);
	}


}

/**Solve Get request**/
void solveResAcceptChallengeInvitation(Account *account, Message &response) {
	Message resp(RESPONSE_TO_CLIENT, reform(UNDENTIFIED, SIZE_RESPONSE_CODE));

	Account *userAccepted = findAccount(response.content);

	if (userAccepted == NULL) {
		resp.content = reform(S2C_SEND_CHALLENGE_INVITATION_FAIL, SIZE_RESPONSE_CODE);
	}
	else {
		Message requestToFr(C2S_ACCEPT_CHALLENGE_INVITATION, account->username);

		resp.content = reform(S2C_SEND_CHALLENGE_INVITATION_SUCCESSFUL, SIZE_RESPONSE_CODE);
	}

}

/**Solve Get request**/
void solveResDenyChallengInvitation(Account *account, Message &response) {
	SOCKET sock = account->perHandleData->socket;
	Message resp(RESPONSE_TO_CLIENT, reform(UNDENTIFIED, SIZE_RESPONSE_CODE));

	if (sock == NULL) {
		resp.content = reform(S2C_SEND_CHALLENGE_INVITATION_FAIL, SIZE_RESPONSE_CODE);
	}
	else {
		Message requestToFr(C2S_DENY_CHALLENGE_INVITATION, account->username);
		resp.content = reform(S2C_SEND_CHALLENGE_INVITATION_SUCCESSFUL, SIZE_RESPONSE_CODE);
	}
}



///////////////////////////// In Gamme /////////////////////////////

/**Solve Stop Match request**/
void solveStopMatchReq(Account *account, Message &request) {
	Message response(RESPONSE_TO_CLIENT, reform(UNDENTIFIED, SIZE_RESPONSE_CODE));
	//
	// get database
	// solve request
	//

	

}


/**Solve Play request**/
void solvePlayReq(Account *account, Message &request) {
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
	case C2S_LOGIN:
		solveLoginReq(account, request);
		break;
	case C2S_LOGOUT:
		solveLogoutReq(account, request);
		break;
	case C2S_REGISTER:
		solveReqRegisterAccount(account, request);
		break;
	case C2S_GET_LIST_FRIEND:
		solveGetListFriendReq(account, request);
		break;
	case C2S_SEND_FRIEND_INVITATION:
		solveSendFriendInvitationReq(account, request);
		break;
	case C2S_SEND_CHALLENGE_INVITATION:
		solveSendChallengeInvitationReq(account, request);
		break;
	case C2S_STOP_GAME:
		solveStopMatchReq(account, request);
		break;
	case C2S_PLAY:
		solvePlayReq(account, request);
		break;
	case C2S_CHAT:
		solveChatReq(account, request);
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
		account->send(resp);
		return;
	};

	switch (statusCode) {
	case C2S_ACCEPT_FRIEND_INVITATION:
		solveResAcceptFriendInvitation(account, response);
		break;
	case C2S_DENY_FRIEND_INVITATION:
		solveResDenyFriendInvitation(account, response);
		break;
	case C2S_ACCEPT_CHALLENGE_INVITATION:
		solveResAcceptChallengeInvitation(account, response);
		break;
	case C2S_DENY_CHALLENGE_INVITATION:
		solveResDenyChallengInvitation(account, response);
		break;
	case C2S_RECEIVED_REQUEST_START_GAME:
		sloveResRecivedRequestStartGame(account, response);
		break;
	case C2S_RECEIVED_REQUEST_END_GAME:
		break;
	}


}