#pragma once

void ScreenMenu(SOCKET);
void ScreenLogin(SOCKET, Message*);

//bool ScreenMenu(SOCKET, Message*, string, int);
//bool ScreenLogin(SOCKET, Message *, string, int);


//bool ScreenLogin(SOCKET client, Message *msg, string req, int columns)
void ScreenLogin(SOCKET client, Message *msg) {
	string menu1[] = { "BACK", "CONTINUE" };
	enum menu1 {
		BACK, CONTINUE
	};

	printItem(columns / 2 - 2, 2, CLR_NORML, "CARO TAT");
	printItem(columns / 2 - 8, 6, CLR_NORML, "****** LOGIN ******");
	string username, password;
	ShowConsoleCursor(true);
	printItem(columns / 3 + 3, 10, CLR_NORML, "Username: ");
	printItem(columns / 3 + 3, 13, CLR_NORML, "Password: ");
	gotoxy(columns / 3 + 3 + 10, 10);
	getline(cin, username);
	gotoxy(columns / 3 + 3 + 10, 13);
	getline(cin, password);
	ShowConsoleCursor(false);
	int command = getMenu1(menu1);
	system("cls");

	switch (command) {
	case CONTINUE:
		msg = new Message(REQ_LOGIN, username + "$" + password);
	case BACK:
		ScreenMenu(client);
	}
}

void ScreenMenu(SOCKET client) {
	string menu[] = { "LOGIN", "EXIT" };
	enum menu
	{
		LOGIN, EXIT
	};

	system("cls");
	int command = getMenu(menu);
	system("cls");
	switch (command)
	{
	case LOGIN:
		ScreenLogin(client, new Message);
		break;
	case EXIT:
		exit(0);
		break;
	}
}