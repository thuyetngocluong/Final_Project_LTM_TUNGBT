#pragma once

// define the size of the information fields
#define SIZE_COMMAND						2
#define SIZE_RESPONSE_CODE					3
#define DELIMITER							"\r\n"


// define the error
#define CLIENT_DISCONNECTED					0

// define the command
#define REQ_UNKNOWN							00
#define REQ_LOGIN							10
#define REQ_LOGOUT							11

#define REQ_GET_LIST						20
#define REQ_ADD_FRIEND						21
#define REQ_CHALLENGE						22
#define REQ_ACCEPT_CHALLENGE				23
#define REQ_DENY_CHALLENGE					24

#define REQ_START_MATCH						30
#define REQ_STOP_MATCH						31
#define REQ_PLAY							32

#define REQ_GET_LOG							40

#define REQ_CHAT							80

#define RESPONSE							90


// define the response code
#define RES_UNDENTIFY_RESULT				000
	
#define RES_LOGIN_SUCCESSFUL				100
#define RES_LOGIN_FAIL						101
#define RES_LOGIN_FAIL_LOCKED				102
#define RES_LOGIN_FAIL_WRONG_PASS			103
#define RES_LOGIN_FAIL_NOT_EXIST			104
#define RES_LOGIN_FAIL_ANOTHER_DEVICE		105
#define RES_LOGIN_FAIL_ALREADY_LOGIN		106


#define RES_LOGOUT_SUCCESSFUL				110
#define RES_LOGOUT_FAIL						111

#define RES_GET_LIST_SUCCESSFUL				200
#define RES_GET_LIST_FAIL					201
#define RES_SEND_BROADCAST_LIST				209

#define RES_ADD_FRIEND_SUCCESSFUL			210
#define RES_ADD_FRIEND_FAIL					211

#define RES_CHALLENGE_SUCCESSFUL			220
#define RES_CHALLENGE_FAIL					221

#define RES_ACCEPT_CHALLENGE_SUCCESSFUL		230
#define RES_ACCEPT_CHALLENGE_FAIL_DENY		231
#define RES_ACCEPT_CHALLENGE_FAIL_IN_GAME	232

#define REQ_DENY_CHALLENGE_SUCCESSFUL		240
#define REQ_DENY_CHALLENGE_FAIL				241

#define RES_START_MATCH_SUCCESSFUL			300
#define RES_START_MATCH_FAIL				301
#define RES_STOP_MATCH_SUCCESSFUL			310
#define RES_STOP_MATCH_FAIL					311

#define RES_PLAY_SUCCESSFUL 320
#define RES_PLAY_FAIL 321


#define RES_GET_LOG_SUCCESSFUL 400
#define RES_GET_LOG_FAIL 401

#define RES_CHAT_SUCCESSFUL 800
#define RES_CHAT_FAIL 801
#define RES_SEND_BROADCAST_CHAT 809

#define RECV_FULL true
#define NOT_RECV_FULL false
#define RECEIVE 0
#define SEND 1
#define BUFF_SIZE 2048

struct Message {
	int command;
	string content;//# content of Message

	//constructor
	Message(int _command, string _content) {
		this->command = _command;
		this->content = _content;
	}

	Message() {
		this->command = 0;
		this->content = "";
	}

	string toMessageSend() {
		string rs = reform(command, SIZE_COMMAND) + content + DELIMITER;
		return rs;
	}
};

bool check(string &msg_rcv) {
	if (msg_rcv.length() < SIZE_COMMAND) return false;
	for (int i = 0; i < SIZE_COMMAND; i++) {
		if (isdigit(msg_rcv.at(i)) == false) return false;
	}
	return true;
}


/*
* @function saveMessage: save request in to queue
* @param tmp: message just received
* @param restMessage: the rest of message
* @param requests: the queue of request
**/
void saveMessage(string tmp, string &restMessage, std::queue<Message> &requests) {
	Message request;
	restMessage += tmp;
	tmp = restMessage;
	size_t found = restMessage.find(DELIMITER);

	while (found != std::string::npos) {
		restMessage = tmp.substr(found + 2);
		string s = tmp.substr(0, found);

		if (check(tmp) == true) {
			request.command = atoi(s.substr(SIZE_COMMAND).c_str());
			request.content = s.substr(SIZE_COMMAND);
		}
		else {
			request.command = REQ_UNKNOWN;
			request.content = s;
		}

		requests.push(request);
		tmp = restMessage;
		found = restMessage.find(DELIMITER);
	}
}
