#pragma once

// define the size of the information fields
#define SIZE_COMMAND						2
#define SIZE_RESPONSE_CODE					3
#define DELIMITER							"\r\n"


// define the error
#define CLIENT_DISCONNECTED					0

// define the command
#define REQ_UNKNOWN 00

#define REQ_LOGIN 10
#define REQ_REGISTER 11
#define REQ_LOGOUT 12
#define REQ_GET_LIST_FRIEND 20
#define REQ_SEND_FRIEND_INVITATION 30
#define REQ_SEND_CHALLENGE_INVITATION 40
#define REQ_START_GAME 50
#define REQ_STOP_GAME 51
#define REQ_PLAY_CHESS 52
#define REQ_END_GAME 58
#define REQ_PENDING_START_GAME 59
#define REQ_CHAT 80

#define RESPONSE 90

#define RESPONSE_TO_SERVER 99


#define RES_UNDENTIFIED 000

#define	RES_LOGIN_SUCCESSFUL  100
#define	RES_LOGIN_FAIL  101

#define	RES_REGISTER_SUCCESSFUL  110
#define	RES_REGISTER_FAIL  111

#define	RES_LOGOUT_SUCCESSFUL  120
#define	RES_LOGOUT_FAIL  121

#define	RES_GET_LIST_FRIEND_SUCCESSFUL 200
#define	RES_GET_LIST_FRIEND_FAIL 201

#define	RES_SEND_FRIEND_INVITATION_SUCCESSFUL 300
#define	RES_SEND_FRIEND_INVITATION_FAIL 301
#define	RES_ACCEPT_FRIEND_INVITATION 302
#define	RES_DENY_FRIEND_INVITATION 303

#define	RES_SEND_CHALLENGE_INVITATION_SUCCESSFUL 400
#define	RES_SEND_CHALLENGE_INVITATION_FAIL 401
#define	RES_ACCEPT_CHALLENGE_INVITATION 402
#define	RES_DENY_CHALLENGE_INVITATION 403

#define	RES_STOP_GAME_SUCCESSFUL 510

#define	RES_PLAY_SUCCESSFUL 520
#define	RES_PLAY_FAIL 521


#define	S2C_CHAT_SUCCESSFUL 800
#define	S2C_CHAT_FAIL 801

#define RECV_FULL							true
#define NOT_RECV_FULL						false
#define RECEIVE								0
#define SEND								1
#define BUFF_SIZE							2048

string restMessage = "";

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
* @function 
* @param 
* @param requests: the queue of request
**/
void messageToResponse(string tmp, string &restMessage, std::queue<Message> &requests) {
	Message request;
	restMessage += tmp;
	tmp = restMessage;
	size_t found = restMessage.find(DELIMITER);

	while (found != std::string::npos) {
		restMessage = tmp.substr(found + SIZE_COMMAND);
		string s = tmp.substr(0, found);

		if (check(s) == true) {
			request.command = atoi(s.substr(0, SIZE_COMMAND).c_str());
			request.content = s.substr(SIZE_COMMAND, s.length() - 2);
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
