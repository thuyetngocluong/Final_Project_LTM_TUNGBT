#pragma once

// define the size of the information fields
#define SIZE_COMMAND						2
#define SIZE_RESPONSE_CODE					3
#define DELIMITER							"\r\n"


// define the error
#define CLIENT_DISCONNECTED					0

// define the command

#define RECV_FULL true
#define NOT_RECV_FULL false
#define RECEIVE 0
#define SEND 1
#define BUFF_SIZE 2048

#define REQ_UNKNOWN 00

#define CREQ_LOGIN 10
#define CREQ_REGISTER 11
#define CREQ_LOGOUT 12
#define CREQ_GET_LIST_FRIEND 20
#define CREQ_SEND_FRIEND_INVITATION 30
#define CREQ_SEND_CHALLENGE_INVITATION 40
#define CREQ_STOP_GAME 51
#define CREQ_PLAY 52
#define CREQ_CHAT 80

#define RESPONSE_TO_CLIENT 90

#define SREQ_SEND_LIST_FRIEND 29
#define SREQ_SEND_FRIEND_INVITATION 39
#define SREQ_SEND_CHALLENGE_INVITATION 49
#define SREQ_START_GAME 59
#define SREQ_END_GAME 58
#define SREQ_SEND_LOG 69
#define SREQ_SEND_CHAT 89

#define RESPONSE_TO_SERVER 99


#define UNDENTIFIED 000

#define	LOGIN_SUCCESSFUL  100
#define	LOGIN_FAIL  101
#define	LOGIN_FAIL_LOCKED  102
#define	LOGIN_FAIL_WRONG  103
#define	LOGIN_FAIL_ANOTHER_DEVICE  104
#define	LOGIN_FAIL_LOGGED  105
#define	REGISTER_SUCCESSFUL  110
#define	REGISTER_FAIL  111
#define	LOGOUT_SUCCESSFUL  120
#define	LOGOUT_FAIL  121
#define	GET_LIST_FRIEND_SUCCESSFUL 200
#define	GET_LIST_FRIEND_FAIL 201
#define	SEND_FRIEND_INVITATION_SUCCESSFUL 300
#define	SEND_FRIEND_INVITATION_FAIL 301
#define	SEND_CHALLENGE_INVITATION_SUCCESSFUL 400
#define	SEND_CHALLENGE_INVITATION_FAIL 401
#define	STOP_GAME_SUCCESS 510
#define	PLAY_SUCCESSFUL 520
#define	PLAY_FAIL 521
#define	CHAT_SUCCESSFUL 800
#define	CHAT_FAIL 801



#define	ACCEPT_FRIEND_INVITATION 390
#define	DENY_FRIEND_INVITATION 391
#define	ACCEPT_CHALLENGE_INVITATION 490
#define	DENY_CHALLENGE_INVITATION 491
#define	RECEIVED_REQUEST_START_GAME 590
#define	RECEIVED_REQUEST_END_GAME 580

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
