#pragma once

using namespace std;

string board[HEIGHT][WIDTH];

void createBoard()
{
	for (int i = 0; i < HEIGHT; i++)
	{
		for (int j = 0; j < WIDTH; j++)
		{
			board[i][j] = " - ";
		}
	}
}

void game(SOCKET client, Message *msg, string req)
{
	int pointerX = 0, pointerY = 0, counter = 0;
	char key;

	createBoard();
	while (1)
	{
		ShowConsoleCursor(false);
		for (int y = 0; y < HEIGHT; y++)
		{
			for (int x = 0; x < WIDTH; x++)
			{
				int colour;
				if (board[x][y] == HAS_X) colour = CLR_X;
				else if (board[x][y] == HAS_O) colour = CLR_O;
				else if (x == pointerX && y == pointerY) colour = CLR_FOCUS;
				else colour = CLR_NORML;
				print(x * 3, y, colour, board[x][y]);
			}
			print(WIDTH * 3, y, 255, "*");
		}

		if (counter % 2 == 0) {
			print(WIDTH * 3 + 3, 1, CLR_X, "X turn.");
		}
		else {
			print(WIDTH * 3 + 3, 1, CLR_O, "O turn.");
		}

		print(WIDTH * 3 + 3, 3, CLR_NORML, "Press F1 to exit game.");

		color(CLR_NORML);
		key = _getch();

		if (key == UP)
		{
			if (pointerY > 0) pointerY--;
		}
		if (key == DOWN)
		{
			if (pointerY < HEIGHT - 1) pointerY++;
		}
		if (key == LEFT)
		{
			if (pointerX > 0) pointerX--;
		}
		if (key == RIGHT)
		{
			if (pointerX < WIDTH - 1) pointerX++;
		}
		if (key == ENTER)
		{
			if (board[pointerX][pointerY] == EMPTY)
			{
				if (counter % 2 == 0)
				{
					board[pointerX][pointerY] = HAS_X;
				}
				else
				{
					board[pointerX][pointerY] = HAS_O;
				}
				//play(client, msg, req, pointerX, pointerY); // SEND TO SERVER
				counter++; 
				system("CLS");
			}
		}
		if (key == STOP)
		{
			int choice = MessageBox(NULL, L"Do you want to quit game?", L"Exit game", MB_OKCANCEL);
			if (choice == 1)
			{
				//surrender(client, msg, req); //SEND TO SERVER
				break; // RETURN TO ONLINE LIST
			}
		}
	}
}

int getOnlineList(SOCKET client, Message *msg, string req, vector<string> users)
{
	int lenList = users.size();
	int pointer = 0;
	char key;

	while (1)
	{
		for (int i = 0; i < lenList; i++)
		{
			if (i == pointer) print(0, i, CLR_FOCUS, users[i]);
			else print(0, i, CLR_NORML, users[i]);
		}
		color(7);
		cout << endl;
		key = _getch();

		if (key == UP)
		{
			if (pointer > 0) pointer--;
			else pointer = lenList - 1;
		}
		if (key == DOWN)
		{
			if (pointer < lenList - 1) pointer++;
			else pointer = 0;
		}
		if (key == ENTER)
		{
			if (pointer < lenList && pointer >= 0)
			{
				//challenge(client, msg, req, users[pointer]); // Send challenge to enemy to server
				break;
			}
		}
	}
	return pointer;
}