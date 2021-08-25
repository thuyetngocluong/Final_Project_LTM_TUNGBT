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
void messageToRequest(string tmp, string &restMessage, std::queue<Message> &requests) {
	Message request;
	restMessage += tmp;
	tmp = restMessage;
	size_t found = restMessage.find(DELIMITER);

	while (found != std::string::npos) {
		restMessage = tmp.substr(found + SIZE_COMMAND);
		string s = tmp.substr(0, found);
		
		if (check(s) == true) {
			request.command = atoi(s.substr(0, SIZE_COMMAND).c_str());
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
