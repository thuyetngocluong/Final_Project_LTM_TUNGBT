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

#include "Account.h"
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
void				startGame(Account *player1, Account *player2);
void				endGame(Match *match);

void				solveResponseFromClient(Account*, Message&);
void				solveRequest(Account*, Message&);

void				processAccount(Account*);
unsigned __stdcall	serverWorkerThread(LPVOID CompletionPortID);
void				receiveMessage(Account*, char*, int);
void				newClientConnect(LPPER_HANDLE_DATA, LPPER_IO_OPERATION_DATA, string, int);
void				clientDisconnect(Account*);
Account*			findAccount(LPPER_HANDLE_DATA);

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

int numberClient = 0;
Account *acc1, *acc2;

void newClientConnect(LPPER_HANDLE_DATA perHandleData, LPPER_IO_OPERATION_DATA perIoData, string ip, int port) {
	account = new Account(perHandleData, perIoData);
	account->IP = ip;
	account->PORT = port;
	accounts.insert(account);
	account->recvMsg();

	numberClient++;
	if (numberClient == 1) {
		acc1 = account;
		acc1->username = "thuyetln";
	}
	if (numberClient == 2) {
		acc2 = account;
		acc2->username = "anhnh";
		startGame(acc1, acc2);
	}
		 

}

void clientDisconnect(Account *account) {
	CloseHandle(account->mutex);
	cout << getCurrentDateTime() << " Client [" << account->IP << ":" << account->PORT << "]: Disconnected" << endl;
	accounts.erase(account);
	while (!account->requests.empty()) account->requests.pop();
	while (!account->messagesNeesToSend.empty()) account->messagesNeesToSend.pop();
	closesocket(account->perHandleData->socket);
	delete account;
}

void receiveMessage(Account* account, char* msg, int length) {

	messageToRequest(msg, account->restMessage, account->requests);

	if (account->numberReceiveInQueue <= 0) {
		account->recvMsg();
	}


	while (!account->requests.empty()) {
		Message request = account->requests.front();
		account->requests.pop();
		cout << getCurrentDateTime() << " Client [" << account->IP << ":" << account->PORT << "]: Receive\t" << request.toMessageSend() << endl;
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


/*
* @function login: solve login command of the account with the request
* @param account: the account need to solve request
* @param request: the request of the account
*
* @param: a message brings result code
**/
void solveLoginReq(Account *account, Message &request) {
	Message response(RESPONSE_TO_CLIENT, reform(RES_UNDENTIFIED, SIZE_RESPONSE_CODE));
	string content = request.content;
	size_t found = content.find("$");
	string username = content.substr(0, found);
	string password = content.substr(found + 1);

	//if the account already logged 
	if (database->login(username, password) && account->signInStatus != LOGGED) {
		account->username = username;
		account->signInStatus = LOGGED;
		response.content = reform(RES_LOGIN_SUCCESSFUL, SIZE_RESPONSE_CODE);
		vector<Player> listFr = database->getListFriend(database->getPlayerByName(account->username));
		int i = 0;
		string listFrReform = "";
		for (auto player = listFr.begin(); player != listFr.end(); player++) {
			i++;
			listFrReform += player->getUsername() + "&" + to_string(player->getElo());
			if (i != listFr.size()) listFrReform += "$";
		}

		Message msgListFr(RESPONSE_TO_CLIENT, listFrReform);

		account->send(response); // send response
		account->send(msgListFr); // send list friend
	}
	else {
		response.content = reform(RES_LOGIN_FAIL, SIZE_RESPONSE_CODE);
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
	Message response(RESPONSE_TO_CLIENT, reform(RES_UNDENTIFIED, SIZE_RESPONSE_CODE));

	if (account->signInStatus == NOT_LOGGED) {
		response.content = reform(RES_UNDENTIFIED, RES_LOGOUT_FAIL);
		saveLog(account, request, RES_LOGOUT_FAIL);
	}
	else {
		account->signInStatus = NOT_LOGGED;
		response.content = reform(RES_UNDENTIFIED, RES_LOGOUT_SUCCESSFUL);
		saveLog(account, request, RES_LOGOUT_SUCCESSFUL);
	}

	account->send(response);

}


/**Solve Register Account request**/
void solveReqRegisterAccount(Account *account, Message &request) {
	Message response(RESPONSE_TO_CLIENT, reform(RES_UNDENTIFIED, SIZE_RESPONSE_CODE));
	size_t found = request.content.find("$");
	string username = request.content.substr(0, found);
	string password = request.content.substr(found + 1);
	
	if (database->registerAccount(username, password)) {
		response.content = reform(RES_REGISTER_SUCCESSFUL, SIZE_RESPONSE_CODE);
	}
	else {
		response.content = reform(RES_REGISTER_FAIL, SIZE_RESPONSE_CODE);
	}
	
	account->send(response);
}



/**Solve Get List request**/
void solveGetListFriendReq(Account *account, Message &request) {

	vector<Player> listFr = database->getListFriend(database->getPlayerByName(account->username));
	string listFrReform = "";
	int i = 0;

	for (auto player = listFr.begin(); player != listFr.end(); player++) {
		i++;
		listFrReform += player->getUsername() + "&" + to_string(player->getElo());
		if (i != listFr.size()) listFrReform += "$";
	}

	string content = reform(RES_GET_LIST_FRIEND_SUCCESSFUL, SIZE_RESPONSE_CODE) + listFrReform;

	account->send(Message(RESPONSE_TO_CLIENT, content));

}


/**Solve Add Friend request**/
void solveSendFriendInvitationReq(Account *account, Message &request) {
	Message response(RESPONSE_TO_CLIENT, reform(RES_UNDENTIFIED, SIZE_RESPONSE_CODE));

	SOCKET sock = account->perHandleData->socket;

	if (sock == NULL) {
		response.content = reform(RES_SEND_FRIEND_INVITATION_FAIL, SIZE_RESPONSE_CODE);
		account->send(response);
	}
	else {
		Message requestToFr(REQ_SEND_FRIEND_INVITATION, account->username);

		response.content = reform(RES_SEND_FRIEND_INVITATION_SUCCESSFUL, SIZE_RESPONSE_CODE);

		account->send(response);
		account->send(requestToFr);
	}


}


/**Solve Challenge request**/
void solveSendChallengeInvitationReq(Account *account, Message &request) {
	Message response(RESPONSE_TO_CLIENT, reform(RES_UNDENTIFIED, SIZE_RESPONSE_CODE));

	SOCKET sock = account->perHandleData->socket;

	if (sock == NULL) {
		response.content = reform(RES_SEND_CHALLENGE_INVITATION_FAIL, SIZE_RESPONSE_CODE);
		account->send(response);
	}
	else {
		Message requestToFr(REQ_SEND_CHALLENGE_INVITATION, account->username);

		response.content = reform(RES_SEND_CHALLENGE_INVITATION_SUCCESSFUL, SIZE_RESPONSE_CODE);

		account->send(response);
		account->send(requestToFr);
	}


}


/**Solve Get Log request**/
void getLog(Account *account, Message &request) {
	Message response(RESPONSE_TO_CLIENT, reform(RES_UNDENTIFIED, SIZE_RESPONSE_CODE));


}


/**Solve Chat request**/
void solveChatReq(Account *account, Message &request) {
	Message response(RESPONSE_TO_CLIENT, reform(RES_UNDENTIFIED, SIZE_RESPONSE_CODE));


	
}


/**Solve Get request**/
void solveResAcceptFriendInvitation(Account *account, Message &response) {
	Message resp(RESPONSE_TO_CLIENT, reform(RES_UNDENTIFIED, SIZE_RESPONSE_CODE));
	SOCKET sock = account->perHandleData->socket;

	if (sock == NULL) {
		resp.content = reform(RES_SEND_FRIEND_INVITATION_FAIL, SIZE_RESPONSE_CODE);

	}
	else {
		Message requestToFr(RES_ACCEPT_FRIEND_INVITATION, account->username);

		resp.content = reform(RES_SEND_FRIEND_INVITATION_SUCCESSFUL, SIZE_RESPONSE_CODE);
	}

}

/**Solve Get request**/
void solveResDenyFriendInvitation(Account *account, Message &response) {
	SOCKET sock = account->perHandleData->socket;
	Message resp(RESPONSE_TO_CLIENT, reform(RES_UNDENTIFIED, SIZE_RESPONSE_CODE));

	if (sock == NULL) {
		resp.content = reform(RES_SEND_FRIEND_INVITATION_FAIL, SIZE_RESPONSE_CODE);
	}
	else {
		Message requestToFr(RES_DENY_FRIEND_INVITATION, account->username);

		resp.content = reform(RES_SEND_FRIEND_INVITATION_SUCCESSFUL, SIZE_RESPONSE_CODE);
	}


}

/**Solve Get request**/
void solveResAcceptChallengeInvitation(Account *account, Message &response) {
	SOCKET sock = account->perHandleData->socket;
	Message resp(RESPONSE_TO_CLIENT, reform(RES_UNDENTIFIED, SIZE_RESPONSE_CODE));

	if (sock == NULL) {
		resp.content = reform(RES_SEND_CHALLENGE_INVITATION_FAIL, SIZE_RESPONSE_CODE);
	}
	else {
		Message requestToFr(RES_ACCEPT_CHALLENGE_INVITATION, account->username);

		resp.content = reform(RES_SEND_CHALLENGE_INVITATION_SUCCESSFUL, SIZE_RESPONSE_CODE);
	}

}

/**Solve Get request**/
void solveResDenyChallengInvitation(Account *account, Message &response) {
	SOCKET sock = account->perHandleData->socket;
	Message resp(RESPONSE_TO_CLIENT, reform(RES_UNDENTIFIED, SIZE_RESPONSE_CODE));

	if (sock == NULL) {
		resp.content = reform(RES_SEND_CHALLENGE_INVITATION_FAIL, SIZE_RESPONSE_CODE);
	}
	else {
		Message requestToFr(RES_DENY_CHALLENGE_INVITATION, account->username);
		resp.content = reform(RES_SEND_CHALLENGE_INVITATION_SUCCESSFUL, SIZE_RESPONSE_CODE);
	}
}



///////////////////////////// In Gamme /////////////////////////////

vector<Match*> matches;

Match* getMatch(Account* account) {

	for (auto i = matches.begin(); i < matches.end(); i++) {
		if ((*i)->xAcc == account || (*i)->oAcc == account) {
			return (*i);
		}
	}
	return NULL;
}

void removeMatch(Match *match) {
	for (auto i = matches.begin(); i != matches.end(); i++) 
		if ((*i) == match) {
			matches.erase(i);
			delete match;
			return;
		}
}


/**Solve Stop Match request**/
void solveStopMatchReq(Account *account, Message &request) {
	Message response(RESPONSE_TO_CLIENT, reform(RES_UNDENTIFIED, SIZE_RESPONSE_CODE));
	//
	// get database
	// solve request
	//
	// xử lý khi client đột ngột muốn kết thúc
	Match* match = getMatch(account);
	if (account == match->xAcc) {
		match->win = O_WIN;
		endGame(match);
	}
	else {
		match->win = X_WIN;
		endGame(match);
	}


}



/**Solve Play request**/
// nhận tọa độ cập nhật vào bàn cờ, xử lý thắng thua,  thông báo thắng thua, cập nhật elo
void solvePlayReq(Account *account, Message &request) {
	Message response(RESPONSE_TO_CLIENT, reform(RES_UNDENTIFIED, SIZE_RESPONSE_CODE));
	string content = request.content;
	size_t found = content.find("$");
	string xx = content.substr(0, found);
	string yy = content.substr(found + 1);
	int x = atoi(xx.c_str());
	int y = atoi(yy.c_str());

	Match* match = getMatch(account);


	if (match != NULL) {
		if (account == match->xAcc) {
			if (!match->xCanPlay(x, y)) {
				account->send(Message(RESPONSE_TO_CLIENT, reform(RES_PLAY_FAIL, SIZE_RESPONSE_CODE)));
			}
			else {
				save((char*)(match->nameLogFile.c_str()), getCurrentDateTime(), match->xAcc->username, xx, yy);

				string rsp = reform(RES_PLAY_SUCCESSFUL, SIZE_RESPONSE_CODE) + match->xAcc->username + "&" + xx + "$" + yy;
				
				switch (match->xPlay(x, y)) {
				case WIN:
					match->win = X_WIN;
					endGame(match);
					break;
				case NOT_WIN:
					match->xAcc->send(Message(RESPONSE_TO_CLIENT, rsp));
					match->oAcc->send(Message(RESPONSE_TO_CLIENT, rsp));
					break;
				case GAME_DRAW:
					match->win = GAME_DRAW;
					endGame(match);
					break;
				default:
					break;
				}
			}
		}
		else
		{
			if (!match->oCanPlay(x, y)) {
				account->send(Message(RESPONSE_TO_CLIENT, reform(RES_PLAY_FAIL, SIZE_RESPONSE_CODE)));
			}
			else {
				save((char*)(match->nameLogFile.c_str()), getCurrentDateTime(), match->oAcc->username, xx, yy);

				string rsp = reform(RES_PLAY_SUCCESSFUL, SIZE_RESPONSE_CODE) + match->oAcc->username + "&" + xx + "$" + yy;
				match->xAcc->send(Message(RESPONSE_TO_CLIENT, rsp));
				match->oAcc->send(Message(RESPONSE_TO_CLIENT, rsp));

				switch (match->oPlay(x, y)) {
				case WIN:
					match->win = O_WIN;
					endGame(match);
					break;
				case NOT_WIN:
					break;
				case GAME_DRAW:
					match->win = GAME_DRAW;
					endGame(match);
					break;
				default:
					break;
				}
			}
		}
	}
	else {
		account->send(Message(RESPONSE_TO_CLIENT, reform(RES_PLAY_FAIL, SIZE_RESPONSE_CODE)));
	}
	
}


/** Send command StartGame to client**/
// viết log
void startGame(Account *playerX, Account *playerO) {
	Match* match = new Match(playerX, playerO); // tạo match
	matches.push_back(match); // gán match vào mảng

	Message response_1(REQ_START_GAME, "X$" + playerX->username); 	// gửi request đến hai client
	Message response_2(REQ_START_GAME, "O$" + playerO->username);

	playerX->send(response_1);
	playerO->send(response_2);

	playerX->matchStatus = IN_GAME;
	playerO->matchStatus = IN_GAME;

	string log = "********************** START GAME ***************************";
	string log1 = "Time Start:\t\t\t" + getCurrentDateTime();
	string log2 = "*************************************************************\n";

	save(log.c_str(), (char*)(match->nameLogFile.c_str()));
	save(log1.c_str(), (char*)(match->nameLogFile.c_str()));
	save((char*)(match->nameLogFile.c_str()), "X Player: " + playerX->username, playerX->IP, to_string(playerX->PORT));
	save((char*)(match->nameLogFile.c_str()), "O Player: " + playerO->username, playerO->IP, to_string(playerO->PORT));
	save(log2.c_str(), (char*)(match->nameLogFile.c_str()));

	
}

/** Send command EndGame to client**/
// xóa match khỏi hàng đợi, gửi file log đến 2 client, cập nhật database
void endGame(Match *match) {

	switch (match->win)
	{
	case X_WIN: {
		match->xAcc->send(Message(REQ_END_GAME, match->xAcc->username));
		match->oAcc->send(Message(REQ_END_GAME, match->xAcc->username));
		database->updateElo(match->xAcc->username, database->getElo(match->xAcc->username) + 3);
		database->updateElo(match->oAcc->username, database->getElo(match->oAcc->username) - 3);
		string log = "\n********************** END GAME: X WIN **********************";
		save(log.c_str(), (char*)(match->nameLogFile.c_str()));
		// chỉnh save log, gửi file
		break;
	}
	case O_WIN: {
		match->xAcc->send(Message(REQ_END_GAME, match->oAcc->username));
		match->oAcc->send(Message(REQ_END_GAME, match->oAcc->username));
		database->updateElo(match->xAcc->username, database->getElo(match->xAcc->username) - 3);
		database->updateElo(match->oAcc->username, database->getElo(match->oAcc->username) + 3);
		string log = "\n********************** END GAME: O WIN **********************";
		save(log.c_str(), (char*)(match->nameLogFile.c_str()));
		// chỉnh savelog, gửi file
		break;
	}
	case GAME_DRAW: {
		match->xAcc->send(Message(REQ_END_GAME, ""));
		match->oAcc->send(Message(REQ_END_GAME, ""));
		string log = "\n******************** END GAME: GAME DRAW ********************";
		save(log.c_str(), (char*)(match->nameLogFile.c_str()));
		break;
	}
	default:
		// chỉnh log, gửi file
		break;
	
	} 
	
	save(("Time End : \t\t\t" + getCurrentDateTime()).c_str(), (char*)(match->nameLogFile.c_str()));
	save("*************************************************************", (char*)(match->nameLogFile.c_str()));
	match->xAcc = NOT_IN_GAME;
	match->oAcc = NOT_IN_GAME;
	removeMatch(match); 

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
	case REQ_LOGIN:
		solveLoginReq(account, request);
		break;
	case REQ_LOGOUT:
		solveLogoutReq(account, request);
		break;
	case REQ_REGISTER:
		solveReqRegisterAccount(account, request);
		break;
	case REQ_GET_LIST_FRIEND:
		solveGetListFriendReq(account, request);
		break;
	case REQ_SEND_FRIEND_INVITATION:
		solveSendFriendInvitationReq(account, request);
		break;
	case REQ_SEND_CHALLENGE_INVITATION:
		solveSendChallengeInvitationReq(account, request);
		break;
	case REQ_STOP_GAME:
		solveStopMatchReq(account, request);
		break;
	case REQ_PLAY_CHESS:
		solvePlayReq(account, request);
		break;
	case REQ_CHAT:
		solveChatReq(account, request);
		break;
	default:
		saveLog(account, request, RES_UNDENTIFIED);
		break;
	}
}

void solveResponseFromClient(Account *account, Message &response) {
	int statusCode = atoi(response.content.substr(0, SIZE_RESPONSE_CODE).c_str());

	if (statusCode == 0) {
		Message resp(RESPONSE_TO_CLIENT, reform(RES_UNDENTIFIED, SIZE_RESPONSE_CODE));
		account->send(resp);
		return;
	};

	switch (statusCode) {
	case RES_ACCEPT_FRIEND_INVITATION:
		solveResAcceptFriendInvitation(account, response);
		break;
	case RES_DENY_FRIEND_INVITATION:
		solveResDenyFriendInvitation(account, response);
		break;
	case RES_ACCEPT_CHALLENGE_INVITATION:
		solveResAcceptChallengeInvitation(account, response);
		break;
	case RES_DENY_CHALLENGE_INVITATION:
		solveResDenyChallengInvitation(account, response);
		break;
	}
}