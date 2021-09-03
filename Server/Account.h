#pragma once

#define LOGGED 1
#define NOT_LOGGED 0

#define IN_GAME 1
#define NOT_IN_GAME 0

#define SIZE_BLOCK 65536

/*Struct contains information of the socket communicating with client*/

struct Account {
	bool waiting = true;
	int numberReceiveInQueue = 0;

	LPPER_HANDLE_DATA perHandleData = NULL;
	LPPER_IO_OPERATION_DATA perIoData = NULL;

	string IP;//# ip of account
	int PORT;//#port of account
	string username = "";//#username which the account logged in

	int signInStatus = NOT_LOGGED;//# login status of the account
	int matchStatus = NOT_IN_GAME;

	string restMessage = "";
	queue<Message> requests;
	queue<Message> messagesNeesToSend;

	File* fileNeedSend = NULL;

	HANDLE mutex;

	Account(LPPER_HANDLE_DATA _perHandleData, LPPER_IO_OPERATION_DATA _perIoData);

	void lock(DWORD milisec);

	void unlock();

	bool canSendNewMsg();

	bool canContinuteSendMsg();

	void recvMsg();

	void continuteSendMsg();

	void sendNewMsg();

	void send(Message msgNeedToSend);

	void send(char *payload, int length);

	void sendFile(string nameFile, streamsize pos);
};

Account::Account(LPPER_HANDLE_DATA _perHandleData, LPPER_IO_OPERATION_DATA _perIoData) {
	this->perHandleData = _perHandleData;
	this->perIoData = _perIoData;
	mutex = CreateMutex(0, 0, 0);
}

void Account::lock(DWORD milisec = INFINITE) {
	WaitForSingleObject(this->mutex, milisec);
}

void Account::unlock() {
	ReleaseMutex(this->mutex);
}

bool Account::canSendNewMsg() {
	return ((perIoData->recvBytes <= perIoData->sentBytes) || (perIoData->operation == RECEIVE)) && !messagesNeesToSend.empty();
}

bool Account::canContinuteSendMsg() {
	return perIoData->recvBytes > perIoData->sentBytes && perIoData->operation == SEND && !waiting;
}


void Account::recvMsg() {
	numberReceiveInQueue++;
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

void Account::continuteSendMsg() {
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

void Account::sendNewMsg() {

	if (fileNeedSend != NULL) {
	}

	Message response = messagesNeesToSend.front();
	messagesNeesToSend.pop();
	string responseStr = response.toMessageSend();
	DWORD transferredBytes = 0;

	cout << getCurrentDateTime() << " Client [" << IP << ":" << PORT << "]: Send\t" << responseStr << endl;

	ZeroMemory(&(perIoData->overlapped), sizeof(OVERLAPPED));
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


void Account::send(Message msgNeedToSend) {
	lock();
	messagesNeesToSend.push(msgNeedToSend);
	if (canSendNewMsg()) {
		sendNewMsg();
	}
	unlock();
}


void Account::send(char *payload, int length) {
	lock();
	
	if (canSendNewMsg())  {
		sendNewMsg();
	}
	unlock();
}


void Account::sendFile(string nameFile, streamsize pos) {
	ifstream f;
	f.open(nameFile.c_str(), ios::in | ios::binary);
	
	if (f.is_open()) {
		streambuf *sb = f.rdbuf();
		sb->pubseekpos(pos);
		streamsize size = sb->pubseekoff(pos, f.end);

		streamsize sizePayload = 0;

		if (size < SIZE_BLOCK) {
			sizePayload = size;
		}
		else {
			sizePayload = SIZE_BLOCK;
		}

		char *payload = new char[sizePayload];

		streamsize sizeRest = 0;

		while (sizeRest < size && sizeRest < SIZE_BLOCK) {
			sb->pubseekpos(pos + sizeRest);

			streamsize bSuccess = sb->sgetn(payload + sizeRest, sizePayload - sizeRest);

			sizeRest += bSuccess;
		}
		
		send(payload, sizePayload);

		f.close();
	}
}



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
* @function saveLog: save request of account to file
* @param account: the information of account
* @param message: the information of message
* @param statusCode: status Code after account perform request
*
* @return true if save successfully, false if not
**/
bool saveLog(Account *account, Message &message, int resultCode) {
	
	string rs = getInforAccount(account) + " " + getCurrentDateTime();

	return save(rs.c_str(), (char*) account->username.c_str());
}



