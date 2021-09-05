#pragma once
#pragma once

void(*onNewClientConnect)(LPPER_HANDLE_DATA, LPPER_IO_OPERATION_DATA, string, int) = NULL;
void(*onReceive) (Account*, char*, int) = NULL;
void(*onClientDisconnect)(Account*) = NULL;
Account*	(*getAccountHasEvent)(LPPER_HANDLE_DATA) = NULL;


bool createServer(string addressServer, int portServer) {

	if (onNewClientConnect == NULL || onReceive == NULL
		|| onClientDisconnect == NULL || getAccountHasEvent == NULL) {
		cout << "Need configure function needed:" << endl;
		cout << "onNewClientConnect" << endl;
		cout << "onReceive" << endl;
		cout << "onClientDisconnect" << endl;
		cout << "getAccountHasEvent" << endl;
		return false;
	}

	SOCKADDR_IN serverAddr, clientAddr;
	SOCKET listenSock, acceptSock;
	HANDLE completionPort;
	SYSTEM_INFO systemInfo;
	LPPER_HANDLE_DATA perHandleData;
	LPPER_IO_OPERATION_DATA perIoData;
	DWORD transferredBytes;
	DWORD flags;
	WSADATA wsaData;
	char IP[INET_ADDRSTRLEN];
	int PORT;


	if (WSAStartup((2, 2), &wsaData) != 0) {
		printf("WSAStartup() failed with error %d\n", GetLastError());
		return false;
	}

	// Step 1: Setup an I/O completion port
	if ((completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0)) == NULL) {
		printf("CreateIoCompletionPort() failed with error %d\n", GetLastError());
		return false;
	}

	// Step 2: Determine how many processors are on the system
	GetSystemInfo(&systemInfo);
	// Step 3: Create worker threads based on the number of processors available on the
	// system. Create two worker threads for each processor	
	for (int i = 0; i < (int)systemInfo.dwNumberOfProcessors * 2; i++) {
		// Create a server worker thread and pass the completion port to the thread
		if (_beginthreadex(0, 0, serverWorkerThread, (void*)completionPort, 0, 0) == 0) {
			printf("Create thread failed with error %d\n", GetLastError());
			return false;
		}
	}

	// Step 4: Create a listening socket
	if ((listenSock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET) {
		printf("WSASocket() failed with error %d\n", WSAGetLastError());
		return false;
	}

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(portServer);
	inet_pton(AF_INET, addressServer.c_str(), &serverAddr.sin_addr);

	if (bind(listenSock, (PSOCKADDR)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		printf("bind() failed with error %d\n", WSAGetLastError());
		return false;
	}

	// Prepare socket for listening
	if (listen(listenSock, 20) == SOCKET_ERROR) {
		printf("listen() failed with error %d\n", WSAGetLastError());
		return false;
	}

	printf("Server startd\nIP: %s\nPORT: %d\n", addressServer.c_str(), portServer);

	while (1) {
		// Step 5: Accept connections
		if ((acceptSock = WSAAccept(listenSock, (sockaddr*)&clientAddr, NULL, NULL, 0)) == SOCKET_ERROR) {
			printf("WSAAccept() failed with error %d\n", WSAGetLastError());
			continue;
		}

		// Step 6: Create a socket information structure to associate with the socket
		if ((perHandleData = (LPPER_HANDLE_DATA)GlobalAlloc(GPTR, sizeof(PER_HANDLE_DATA))) == NULL) {
			printf("GlobalAlloc() failed with error %d\n", GetLastError());
			continue;
		}

		// Step 7: Associate the accepted socket with the original completion port
		//printf("Socket number %d got connected...\n", acceptSock);
		perHandleData->socket = acceptSock;
		if (CreateIoCompletionPort((HANDLE)acceptSock, completionPort, (DWORD)perHandleData, 0) == NULL) {
			printf("CreateIoCompletionPort() failed with error %d\n", GetLastError());
			continue;
		}


		// Step 8: Create per I/O socket information structure to associate with the WSARecv call
		if ((perIoData = (LPPER_IO_OPERATION_DATA)GlobalAlloc(GPTR, sizeof(PER_IO_OPERATION_DATA))) == NULL) {
			printf("GlobalAlloc() failed with error %d\n", GetLastError());
			continue;
		}

		inet_ntop(AF_INET, &clientAddr, IP, INET_ADDRSTRLEN);
		PORT = ntohs(clientAddr.sin_port);

		if (onNewClientConnect != NULL) {
			onNewClientConnect(perHandleData, perIoData, IP, PORT);
		}

	}
	_getch();
}


unsigned __stdcall serverWorkerThread(LPVOID completionPortID)
{
	printf("Thread %d start!\n", GetCurrentThreadId());

	HANDLE completionPort = (HANDLE)completionPortID;

	DWORD transferredBytes;
	LPPER_HANDLE_DATA perHandleData;
	LPPER_IO_OPERATION_DATA perIoData;
	Account *account = NULL;

	while (TRUE) {

		if (GetQueuedCompletionStatus(completionPort, &transferredBytes,
			(LPDWORD)&perHandleData, (LPOVERLAPPED *)&perIoData, INFINITE) == 0) {
			printf("GetQueuedCompletionStatus() failed with error %d\n", GetLastError());
			account = getAccountHasEvent(perHandleData);
			if (account == NULL) continue;
			if (onClientDisconnect != NULL) {
				onClientDisconnect(account);
			}
			continue;
		}

		if (getAccountHasEvent == NULL) continue;

		account = getAccountHasEvent(perHandleData);

		if (account == NULL) continue;

		account->lock();
		account->waiting = false;

		// Check to see if an error has occurred on the socket and if so
		// then close the socket and cleanup the SOCKET_INFORMATION structure
		// associated with the socket
		if (transferredBytes == 0) {
			printf("Closing socket %d\n", perHandleData->socket);
			GlobalFree(perHandleData);
			GlobalFree(perIoData);
			if (onClientDisconnect != NULL) {
				onClientDisconnect(account);
			}
			continue;
		}


		if (perIoData->operation == RECEIVE) {
			perIoData->dataBuff.buf[transferredBytes] = 0;
			account->numberReceiveInQueue--;
			if (onReceive != NULL) {
				onReceive(account, perIoData->dataBuff.buf, transferredBytes);
			}

		}
		else if (perIoData->operation == SEND) {
			perIoData->sentBytes += transferredBytes;

			if (account->canContinuteSendMsg()) {
				account->continuteSendMsg();
			}
			else if (account->canSendNewMsg()) {
				account->sendNewMsg();
			}
			else if (account->numberReceiveInQueue <= 0) {
				account->recvMsg();
			}
			else {
				ZeroMemory(&(perIoData->overlapped), sizeof(OVERLAPPED));
				perIoData->recvBytes = 0;
				perIoData->sentBytes = 0;
				perIoData->dataBuff.len = DATA_BUFSIZE;
				perIoData->dataBuff.buf = perIoData->buffer;
				perIoData->operation = RECEIVE;
			}
		}


		account->waiting = true;
		account->unlock();
	}

	printf("Thread %d stop!\n", GetCurrentThreadId());
}
