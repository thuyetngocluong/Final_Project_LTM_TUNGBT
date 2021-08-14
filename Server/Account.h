#pragma once

#define LOGGED 1
#define NOT_LOGGED 0

#define IN_GAME 1
#define NOT_IN_GAME 0

/*Struct contains information of the socket communicating with client*/

struct Account {
	bool waiting = true;
	int numberInQueue = 0;

	LPPER_HANDLE_DATA perHandleData = NULL;
	LPPER_IO_OPERATION_DATA perIoData = NULL;

	char IP[INET_ADDRSTRLEN];//# ip of account
	int PORT;//#port of account
	string username = "";//#username which the account logged in
	int signInStatus = NOT_LOGGED;//# login status of the account
	int matchStatus = NOT_IN_GAME;

	string restMessage = "";
	queue<Message> requests;
	queue<Message> messagesNeesToSend;
	HANDLE mutex;

	Account(LPPER_HANDLE_DATA _perHandleData, LPPER_IO_OPERATION_DATA _perIoData) {
		this->perHandleData = _perHandleData;
		this->perIoData = _perIoData;
		mutex = CreateMutex(0, 0, 0);
	}

	void lock() {
		WaitForSingleObject(this->mutex, INFINITE);
	}

	void unlock() {
		ReleaseMutex(this->mutex);
	}

	bool canSendNewMsg() {
		return ((perIoData->recvBytes <= perIoData->sentBytes) || (perIoData->operation == RECEIVE)) && !messagesNeesToSend.empty();
	}

	bool canContinuteSendMsg() {
		return perIoData->recvBytes > perIoData->sentBytes && perIoData->operation == SEND && !waiting;
	}

	bool canRecvMsg() {
		return !canSendNewMsg() && !canContinuteSendMsg();
	}


	void recvMsg() {
		numberInQueue++;
		DWORD transferredBytes = 0;
		DWORD flags = 0;
		ZeroMemory(&(perIoData->overlapped), sizeof(OVERLAPPED));
		perIoData->recvBytes = 0;
		perIoData->sentBytes = 0;
		perIoData->dataBuff.len = DATA_BUFSIZE;
		perIoData->dataBuff.buf = perIoData->buffer;
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
		cout << "Continue send" << endl;
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
				printf("WSASend() continute message failed with error %d\n", WSAGetLastError());
			}
		}
	}

	void sendNewMsg() {		
		Message response = messagesNeesToSend.front();
		messagesNeesToSend.pop();
		string responseStr = response.toMessageSend();
		DWORD transferredBytes = 0;

		cout << "Sending: " << responseStr << endl;

		perIoData->recvBytes = responseStr.length();
		perIoData->sentBytes = 0;
		strcpy_s(perIoData->buffer, responseStr.c_str());
		perIoData->dataBuff.len = responseStr.length();
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
				printf("WSASend() new message failed with error %d\n", WSAGetLastError());
			}
		}
	}

	void addMessageNeedToSend(Message msgNeedToSend) {
		lock();
		messagesNeesToSend.push(msgNeedToSend);
		if (canSendNewMsg()) {
			waiting = false;
			sendNewMsg();
			waiting = true;
		}
		unlock();
	}
};


/*
* @function getInforAccount: get information of a account
* @param account: account need to get information
*
* @return: a string brings information of account
**/
string getInforAccount(Account *account) {
	string rs = account->IP;
	rs += ":";
	rs += std::to_string(account->PORT);

	return rs;
}


/*
* @funtion getCurrentDateTime: get current date and time
*
* @return string brings information of current date and time
**/
string getCurrentDateTime() {
	char MY_TIME[22];
	struct tm newtime;
	__time64_t long_time;
	errno_t err;

	// Get time as 64-bit integer.
	_time64(&long_time);
	// Convert to local time.
	err = _localtime64_s(&newtime, &long_time);
	if (err) {
		return "[00/00/00 00:00:00]";
	}
	strftime(MY_TIME, sizeof(MY_TIME), "[%d/%m/%Y %H:%M:%S]", &newtime);
	string result = MY_TIME;

	return result;
}

/*
* @function saveLog: save request of account to file
* @param account: the information of account
* @param message: the information of message
* @param statusCode: status Code after account perform request
*
* @return true if save successfully, false if not
**/
bool saveLog(Account *account, Message &message, int resultCode) {
	
	string rs = getInforAccount(account) + " " + getCurrentDateTime();

	return save(rs.c_str());
}


bool solveReqRegisterAccount(Message request) {
	size_t found = request.content.find("$");
	string username = request.content.substr(0, found);
	string password = request.content.substr(found + 1);
	Data *data = Data::getInstance();
	return data->registerAccount(username, password);
}


