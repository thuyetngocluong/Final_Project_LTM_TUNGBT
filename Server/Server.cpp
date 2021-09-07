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
void				solveGetListCanChallengeReq(Account*, Message&);
void				solveSendFriendInvitationReq(Account*, Message&);
void				solveSendChallengeInvitationReq(Account*, Message&);

void				solveStopMatchReq(Account*, Message&);
void				solvePlayReq(Account*, Message&);
void				startGame(Account *player1, Account *player2);
void				endGame(Match *match);

void				solveRequestFromClient(Account*, Message&);
void				solveResponseFromClient(Account*, Message&);
void				solveMsgRecv(Account*, Message&);

void				processAccount(Account*);
unsigned __stdcall	serverWorkerThread(LPVOID CompletionPortID);

void				receiveMessage(Account*, char*, int);
void				newClientConnect(LPPER_HANDLE_DATA, LPPER_IO_OPERATION_DATA, string, int);
void				clientDisconnect(Account*);
Account*			findAccount(LPPER_HANDLE_DATA);
Message				listFriendToMessage(string);
Message				listCanChallangeToMessage(string);

Match*				getMatch(Account*);
void				removeMatch(Match*);

#include "CompletionPortServer.h"

vector<Match*> matches;
set<Account*> accounts;
Account *account = NULL;


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


/** Do when  new client connect */
void newClientConnect(LPPER_HANDLE_DATA perHandleData, LPPER_IO_OPERATION_DATA perIoData, string ip, int port) {
	account = new Account(perHandleData, perIoData);
	account->IP = ip;
	account->PORT = port;
	accounts.insert(account);
	account->recvMsg();
}


/** Do when  client disconnected*/
void clientDisconnect(Account *account) {

	Match* match = getMatch(account);

	if (match != NULL) {
		if (account == match->xAcc) {
			match->win = O_WIN;
			endGame(match);
		}
		else {
			match->win = X_WIN;
			endGame(match);
		}
	}

	CloseHandle(account->mutex);
	cout << getCurrentDateTime() << " Client [" << account->IP << ":" << account->PORT << "]: Disconnected" << endl;
	accounts.erase(account);
	while (!account->requests.empty()) account->requests.pop();
	while (!account->messagesNeedToSend.empty()) account->messagesNeedToSend.pop();
	closesocket(account->perHandleData->socket);
	database->updateStatus(account->username, 0);
	delete account;

	for (auto i = accounts.begin(); i != accounts.end(); i++) {
		Account *acc = (*i);
		if (acc->signInStatus != LOGGED) continue;
		acc->send(listFriendToMessage(acc->username));
		acc->send(listCanChallangeToMessage(acc->username));
	}
}



/** Do when receive message from client */
void receiveMessage(Account* account, char* msg, int length) {
	messageToRequest(msg, account->restMessage, account->requests);

	if (account->numberReceiveInQueue <= 0) {
		account->recvMsg();
	}


	while (!account->requests.empty()) {
		Message msg = account->requests.front();
		account->requests.pop();
		cout << getCurrentDateTime() << " Client [" << account->IP << ":" << account->PORT << "]: Receive\t" << msg.toMessageSend() << endl;
		solveMsgRecv(account, msg);
		
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
* @function listFriendToMessage: get list ffriend
* @param username: name of player get list ffriend
*
* @return the message to send
***/
Message	listFriendToMessage(string username) {
	vector<Player> listFr = database->getListFriend(database->getPlayerByName(username));
	int i = 0;
	string listFrReform = "";
	for (auto player = listFr.begin(); player != listFr.end(); player++) {
		i++;
		listFrReform += player->getUsername() + "&" + to_string(player->getElo());
		if (i != listFr.size()) listFrReform += "$";
	}

	return Message(RESPONSE, reform(RES_GET_LIST_FRIEND_SUCCESSFUL, SIZE_RESPONSE_CODE) + listFrReform);
}



/**
* @function listCanChallangeToMessage: get list player can challenge
* @param username: name of player get list challenge
* 
* @return the message to send 
***/
Message	listCanChallangeToMessage(string username) {
	vector<Player> listFr = database->getListPlayerCanChallenge(username);
	string listFrReform = "";
	int i = 0;

	for (auto player = listFr.begin(); player != listFr.end(); player++) {
		i++;
		listFrReform += player->getUsername() + "&" + to_string(player->getElo());
		if (i != listFr.size()) listFrReform += "$";
	}

	string content = reform(RES_GET_LIST_CAN_CHALLENGE_SUCCESSFUL, SIZE_RESPONSE_CODE) + listFrReform;
	return Message(RESPONSE, content);
}


/**
* @function solveLoginReq: solve login request
* @param account: account send request
* @param request: the request of account
***/
void solveLoginReq(Account *account, Message &request) {
	Message response(RESPONSE, reform(RES_UNDENTIFIED, SIZE_RESPONSE_CODE));
	string content = request.content;
	size_t found = content.find("$");
	string username = content.substr(0, found);
	string password = content.substr(found + 1);

	//if the account already logged 
	if (username != "" && database->login(username, password) && account->signInStatus != LOGGED) {
		account->username = username;
		database->updateStatus(account->username, 1);
		account->signInStatus = LOGGED;
		response.content = reform(RES_LOGIN_SUCCESSFUL, SIZE_RESPONSE_CODE);

		account->send(response); // send response

		for (auto i = accounts.begin(); i != accounts.end(); i++) {
			Account *acc = (*i);
			if (acc->signInStatus != LOGGED) continue;
			acc->send(listFriendToMessage(acc->username));
			acc->send(listCanChallangeToMessage(acc->username));
		}
	}
	else {
		response.content = reform(RES_LOGIN_FAIL, SIZE_RESPONSE_CODE);
		account->send(response);
	}

}


/**
* @function solveLogoutReq: solve logout request
* @param account: account send request
* @param request: the request of account
***/
void solveLogoutReq(Account *account, Message &request) {
	Message response(RESPONSE, reform(RES_UNDENTIFIED, SIZE_RESPONSE_CODE));

	if (account->signInStatus == NOT_LOGGED) {
		response.content = reform(RES_LOGOUT_FAIL, SIZE_RESPONSE_CODE);
	}
	else {
		account->signInStatus = NOT_LOGGED;
		database->updateStatus(account->username, 0);
		response.content = reform(RES_LOGOUT_SUCCESSFUL, SIZE_RESPONSE_CODE);
		for (auto i = accounts.begin(); i != accounts.end(); i++) {
			Account *acc = (*i);
			if (acc->signInStatus != LOGGED) continue;
			acc->send(listFriendToMessage(acc->username));
			acc->send(listCanChallangeToMessage(acc->username));
		}
	}

	account->send(response);

}


/**
* @function solveReqRegisterAccount: solve register request
* @param account: account send request
* @param request: the request of account
***/
void solveReqRegisterAccount(Account *account, Message &request) {
	Message response(RESPONSE, reform(RES_UNDENTIFIED, SIZE_RESPONSE_CODE));
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


/**
* @function solveGetListFriendReq: solve get list friend request
* @param account: account send request
* @param request: the request of account
***/
void solveGetListFriendReq(Account *account, Message &request) {
	if (account->signInStatus != LOGGED) {
		account->send(Message(RESPONSE, reform(RES_LOGIN_FAIL, SIZE_RESPONSE_CODE)));
		account->send(Message(RESPONSE, reform(RES_GET_LIST_FRIEND_FAIL, SIZE_RESPONSE_CODE)));
		return;
	}
	vector<Player> listFr = database->getListFriend(database->getPlayerByName(account->username));
	string listFrReform = "";
	int i = 0;

	for (auto player = listFr.begin(); player != listFr.end(); player++) {
		i++;
		listFrReform += player->getUsername() + "&" + to_string(player->getElo());
		if (i != listFr.size()) listFrReform += "$";
	}

	string content = reform(RES_GET_LIST_FRIEND_SUCCESSFUL, SIZE_RESPONSE_CODE) + listFrReform;

	account->send(Message(RESPONSE, content));

}


/**
* @function solveGetListCanChallengeReq: solve get list player can challenge request
* @param account: account send request
* @param request: the request of account
***/
void solveGetListCanChallengeReq(Account*, Message&) {
	if (account->signInStatus != LOGGED) {
		account->send(Message(RESPONSE, reform(RES_LOGIN_FAIL, SIZE_RESPONSE_CODE)));
		account->send(Message(RESPONSE, reform(RES_GET_LIST_CAN_CHALLENGE_FAIL, SIZE_RESPONSE_CODE)));
		return;
	}
	

	account->send(listCanChallangeToMessage(account->username));
}


/**
* @function solveSendFriendInvitationReq: solve send add friend invitation request
* @param account: account send request
* @param request: the request of account
***/
void solveSendFriendInvitationReq(Account *account, Message &request) {
	if (account->signInStatus != LOGGED) {
		account->send(Message(RESPONSE, reform(RES_LOGIN_FAIL, SIZE_RESPONSE_CODE)));
		account->send(Message(RESPONSE, reform(RES_SEND_FRIEND_INVITATION_FAIL, SIZE_RESPONSE_CODE)));
		return;
	}
	Account *toAcc = findAccount(request.content);
	
	if (toAcc == NULL) {
		account->send(Message(RESPONSE, reform(RES_SEND_FRIEND_INVITATION_FAIL, SIZE_RESPONSE_CODE)));
	}
	else {
		account->send(Message(RESPONSE, reform(RES_SEND_FRIEND_INVITATION_SUCCESSFUL, SIZE_RESPONSE_CODE)));

		toAcc->send(Message(REQ_SEND_FRIEND_INVITATION, account->username));
	}
}


/**
* @function solveSendChallengeInvitationReq: solve send challenge invitation request
* @param account: account send request
* @param request: the request of account
***/
void solveSendChallengeInvitationReq(Account *account, Message &request) {

	if (account->signInStatus != LOGGED) {
		account->send(Message(RESPONSE, reform(RES_LOGIN_FAIL, SIZE_RESPONSE_CODE)));
		account->send(Message(RESPONSE, reform(RES_SEND_CHALLENGE_INVITATION_FAIL, SIZE_RESPONSE_CODE)));
		return;
	}

	Account *toAcc = findAccount(request.content);
	
	if (toAcc == NULL || !database->checkValidChallenge(account->username, toAcc->username)) {
		account->send(Message(RESPONSE, reform(RES_SEND_CHALLENGE_INVITATION_FAIL, SIZE_RESPONSE_CODE)));
	}
	else {
		account->send(Message(RESPONSE, reform(RES_SEND_CHALLENGE_INVITATION_SUCCESSFUL, SIZE_RESPONSE_CODE)));
		
		toAcc->send(Message(REQ_SEND_CHALLENGE_INVITATION, account->username));
		
	}


}



/**
* @function solveResAcceptFriendInvitation: solve accept add friend invitation from client
* @param account: account send request
* @param response: the response of account
***/
void solveResAcceptFriendInvitation(Account *account, Message &response) {

	if (account->signInStatus != LOGGED) {
		account->send(Message(RESPONSE, reform(RES_LOGIN_FAIL, SIZE_RESPONSE_CODE)));
		return;
	}

	string username = response.content.substr(SIZE_RESPONSE_CODE);
	Account *toAcc = findAccount(username);
	
	// send list friend to account
	database->addFriendRelation(account->username, username);

	account->send(listFriendToMessage(account->username));
	
	// if toAcc not NULL
	if (toAcc != NULL) {
		Message requestToFr(RESPONSE, reform(RES_ACCEPT_FRIEND_INVITATION, SIZE_RESPONSE_CODE) + account->username);
		toAcc->send(requestToFr);

		// send list friend to toAcc
		toAcc->send(listFriendToMessage(toAcc->username));
	}
}

/**
* @function solveResDenyFriendInvitation: solve deny add friend invitation from client
* @param account: account send request
* @param response: the response of account
***/
void solveResDenyFriendInvitation(Account *account, Message &response) {
	
	if (account->signInStatus != LOGGED) {
		account->send(Message(RESPONSE, reform(RES_LOGIN_FAIL, SIZE_RESPONSE_CODE)));
		return;
	}

	string username = response.content.substr(SIZE_RESPONSE_CODE);
	Account *toAcc = findAccount(username);
	if (toAcc != NULL) {
		Message requestToFr(RESPONSE, reform(RES_DENY_FRIEND_INVITATION, SIZE_RESPONSE_CODE) + account->username);
		toAcc->send(requestToFr);
	}
}


/**
* @function solveResAcceptChallengeInvitation: solve accept challenge invitation from client
* @param account: account send request
* @param response: the response of account
***/
void solveResAcceptChallengeInvitation(Account *account, Message &response) {

	if (account->signInStatus != LOGGED) {
		account->send(Message(RESPONSE, reform(RES_LOGIN_FAIL, SIZE_RESPONSE_CODE)));
		return;
	}

	string username = response.content.substr(SIZE_RESPONSE_CODE);
	Account *toAcc = findAccount(username);

	if (toAcc != NULL && toAcc->matchStatus == NOT_IN_GAME) {
		toAcc->send(Message(RESPONSE, reform(RES_ACCEPT_CHALLENGE_INVITATION, SIZE_RESPONSE_CODE) + account->username));
		account->send(Message(RESPONSE, reform(RES_ACCEPT_CHALLENGE_INVITATION, SIZE_RESPONSE_CODE) + toAcc->username));
		int isX = rand() % 2;
		if (isX == 0) {
			startGame(toAcc, account);
		}
		else {
			startGame(account, toAcc);
		}
	}
	else {
		account->send(Message(RESPONSE, reform(RES_DENY_CHALLENGE_INVITATION, SIZE_RESPONSE_CODE) + toAcc->username));
	}
}


/**
* @function solveResDenyChallengInvitation: solve deny challenge invitation from client
* @param account: account send request
* @param response: the response of account
***/
void solveResDenyChallengInvitation(Account *account, Message &response) {

	if (account->signInStatus != LOGGED) {
		account->send(Message(RESPONSE, reform(RES_LOGIN_FAIL, SIZE_RESPONSE_CODE)));
		return;
	}

	string username = response.content.substr(SIZE_RESPONSE_CODE);
	Account *toAcc = findAccount(username);

	if (toAcc != NULL) {
		toAcc->send(Message(RESPONSE, reform(RES_DENY_CHALLENGE_INVITATION, SIZE_RESPONSE_CODE) + account->username));	
	}

}



///////////////////////////// In Gamme /////////////////////////////

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


/**
* @function solveStopMatchReq: solve stop request
* @param account: account send request
* @param request: the request of account
***/
void solveStopMatchReq(Account *account, Message &request) {

	if (account->signInStatus != LOGGED) {
		account->send(Message(RESPONSE, reform(RES_LOGIN_FAIL, SIZE_RESPONSE_CODE)));
		return;
	}

	Match* match = getMatch(account);
	if (match == NULL) {
		// TO-DO
		return;
	}

	save((getCurrentDateTime() + "\t" + account->username + "\tSURRENDER").c_str() , (char*)(match->nameLogFile.c_str()));

	if (account == match->xAcc) {
		match->win = O_WIN;
		endGame(match);
	}
	else {
		match->win = X_WIN;
		endGame(match);
	}


}



/**
* @function solvePlayReq: solve play request
* @param account: account send request
* @param request: the request of account
***/
void solvePlayReq(Account *account, Message &request) {

	if (account->signInStatus != LOGGED) {
		account->send(Message(RESPONSE, reform(RES_LOGIN_FAIL, SIZE_RESPONSE_CODE)));
		return;
	}

	Message response(RESPONSE, reform(RES_UNDENTIFIED, SIZE_RESPONSE_CODE));
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
				account->send(Message(RESPONSE, reform(RES_PLAY_FAIL, SIZE_RESPONSE_CODE)));
			}
			else {
				save((char*)(match->nameLogFile.c_str()), getCurrentDateTime(), match->xAcc->username, xx, yy);

				string rsp = reform(RES_PLAY_SUCCESSFUL, SIZE_RESPONSE_CODE) + "X&" + xx + "$" + yy;
				match->xAcc->send(Message(RESPONSE, rsp));
				match->oAcc->send(Message(RESPONSE, rsp));
				//match->startTurnTime = currMilisec;
				switch (match->xPlay(x, y)) {
				case WIN:
					match->win = X_WIN;
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
		else
		{
			if (!match->oCanPlay(x, y)) {
				account->send(Message(RESPONSE, reform(RES_PLAY_FAIL, SIZE_RESPONSE_CODE)));
			}
			else {
				save((char*)(match->nameLogFile.c_str()), getCurrentDateTime(), match->oAcc->username, xx, yy);

				string rsp = reform(RES_PLAY_SUCCESSFUL, SIZE_RESPONSE_CODE) + "O&" + xx + "$" + yy;
				match->xAcc->send(Message(RESPONSE, rsp));
				match->oAcc->send(Message(RESPONSE, rsp));
				//match->startTurnTime = currMilisec;
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
		account->send(Message(RESPONSE, reform(RES_PLAY_FAIL, SIZE_RESPONSE_CODE)));
	}
	
}



/**
* @function startGame: solve event start game
* @param playerX: player play X in match
* @param playerO: player play O in match
***/
void startGame(Account *playerX, Account *playerO) {
	Match* match = new Match(playerX, playerO); // tạo match
	//match->startTurnTime = currMilisec;
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


/**
* @function endgame: solve event end game
* @param match: match need end or stop
***/
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

	match->xAcc->matchStatus = NOT_IN_GAME;
	match->oAcc->matchStatus = NOT_IN_GAME;

	// send log
	match->xAcc->sendFile(match->nameLogFile);
	match->oAcc->sendFile(match->nameLogFile);

	for (auto i = accounts.begin(); i != accounts.end(); i++) {
		Account *acc = (*i);
		acc->send(listFriendToMessage(acc->username));
		acc->send(listCanChallangeToMessage(acc->username));
	}

	removeMatch(match); 

}


/////////////////////////////////////////////////////////////

void solveMsgRecv(Account *account, Message &msg) {

	switch (msg.command) {
	case RESPONSE:
		solveResponseFromClient(account, msg);
	default:
		solveRequestFromClient(account, msg);
		break;
	}
}

void solveRequestFromClient(Account *account, Message &request) {
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
	case REQ_GET_LIST_CAN_CHALLENGE:
		solveGetListCanChallengeReq(account, request);
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
	default:
		break;
	}
}

void solveResponseFromClient(Account *account, Message &response) {
	int statusCode = atoi(response.content.substr(0, SIZE_RESPONSE_CODE).c_str());
	
	if (statusCode == 0) {
		Message resp(RESPONSE, reform(RES_UNDENTIFIED, SIZE_RESPONSE_CODE));
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