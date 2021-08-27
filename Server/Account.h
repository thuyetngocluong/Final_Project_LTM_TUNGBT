#pragma once

#define LOGGED 1
#define NOT_LOGGED 0

#define IN_GAME 1
#define NOT_IN_GAME 0

#define SENDING 3 
#define SENT 2
#define BEGIN_SEND 1
#define NOT_SEND 0

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

	int sendStatus;

	string restMessage = "";
	queue<Message> requests;
	queue<Message> messagesNeesToSend;
	queue<pair<char*, int>> fileNeedToSend;

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
		return ((perIoData->recvBytes <= perIoData->sentBytes) || (perIoData->operation == RECEIVE)) && (!messagesNeesToSend.empty() || !fileNeedToSend.empty());
	}

	bool canContinuteSendMsg() {
		return perIoData->recvBytes > perIoData->sentBytes && perIoData->operation == SEND && !waiting;
	}

	void recvMsg() {
		sendStatus = NOT_SEND;
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

	void continuteSendMsg() {
		sendStatus = SENDING;
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
		sendStatus = BEGIN_SEND;
		if (!fileNeedToSend.empty()) {
			pair<char*, int> file = fileNeedToSend.front();
			perIoData->recvBytes = file.second;
			perIoData->sentBytes = 0;
			perIoData->dataBuff.len = file.second;
			perIoData->dataBuff.buf = perIoData->buffer;
			strcpy_s(perIoData->buffer, file.first);
		} else
		if (!messagesNeesToSend.empty()) {
			Message response = messagesNeesToSend.front();
			messagesNeesToSend.pop();
			string responseStr = response.toMessageSend();
			cout << "Sending: " << responseStr << endl;
			perIoData->recvBytes = responseStr.length();
			perIoData->sentBytes = 0;
			perIoData->dataBuff.len = responseStr.length();
			perIoData->dataBuff.buf = perIoData->buffer;
			strcpy_s(perIoData->buffer, responseStr.c_str());
		} 

		DWORD transferredBytes = 0;
		ZeroMemory(&(perIoData->overlapped), sizeof(OVERLAPPED));
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

	void send(Message msgNeedToSend) {
		while (perIoData->operation == SEND) {}
		messagesNeesToSend.push(msgNeedToSend);
		if (WaitForSingleObject(this->mutex, 0) != WAIT_TIMEOUT) {
			if (canSendNewMsg()) {
				sendNewMsg();
			}
		}
		unlock();
	}
	
	void sendFile(char *payload, int length) {
		while (perIoData->operation == SEND) {}
		//strcpy_s(tmp, payload);
		fileNeedToSend.push({ File(900, length, payload).toCharArray(), length });
		if (WaitForSingleObject(this->mutex, INFINITE) != WAIT_TIMEOUT) {
			if (canSendNewMsg()) {
				cout << "Send File" << endl;
				sendNewMsg();
			}
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


void sendFile(string nameFile, Account *account) {
	char *payload = NULL;
	fstream ifs;
	ifs.open(nameFile, ios::in | ios::binary);

	if (ifs.is_open()) {
		streambuf *sb = ifs.rdbuf();
		streamsize size = sb->pubseekoff(ifs.beg, ifs.end);
		cout << size << endl;

		if (size < DATA_BUFSIZE) {
			payload = new char[size];
			sb->pubseekpos(ifs.beg);
			streamsize bSuccess = sb->sgetn(payload, size);
			// send file
			account->sendFile(payload, bSuccess);

		}
		else {
			payload = new char[DATA_BUFSIZE + 1];
			sb->pubseekpos(ifs.beg);
			streamsize sizeRest = 0;
			while (sizeRest < size) {
				// read data from file
				sb->pubseekpos(sizeRest);
				size_t bSuccess = sb->sgetn(payload, DATA_BUFSIZE);

				//printf("Upload %.2lf%%\r", sizeRest * 100.0 / size);
				fflush(stdin);
				// send fill
				account->sendFile(payload, bSuccess);
				sizeRest += bSuccess;
			} // end while send file
		}

		printf("\nUpload 100%%\n");
		printf("Upload Finished!\n");
			

		ifs.close();
	}

	//remove(nameFile.c_str());
}


