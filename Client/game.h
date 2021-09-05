#pragma once

HANDLE mutexGame = CreateMutex(0, 0, 0);
string board[HEIGHT][WIDTH];
int counter = 0;
string type = "";

/**
 * Initialize the board 
 */
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

/**
 * @function	drawBoard: draw the player's move
 * @param		x: horizontal coordinates of console
 * @param		y: vertical coordinates of console
 * @param		_type: the player' type
 */
void drawBoard(int x, int y, string _type) {

	if (_type == "X") {
		board[x][y] = HAS_X;
		printItem(x * 3, y, CLR_X, board[x][y]);
		color(CLR_NORML);
	}
	else if (_type == "O") {
		board[x][y] = HAS_O;
		printItem(x * 3, y, CLR_O, board[x][y]);
		color(CLR_NORML);
	}

}

/**
 * @function	game: play game
 * @param		sock: struct SK connect to server
 */
void game(SK *sock)
{
	int turn = type == "X" ? 0 : 1;

	int pointerX = 0, pointerY = 0;
	char key;

	createBoard();
	while (1)
	{
		WaitForSingleObject(mutexGame, INFINITE);
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

		print(WIDTH * 3 + 3, 1, CLR_NORML, "You are " + type);

		if (counter % 2 == 0) {
			print(WIDTH * 3 + 3, 3, CLR_X, "X turn.");
		}
		else {
			print(WIDTH * 3 + 3, 3, CLR_O, "O turn.");
		}

		print(WIDTH * 3 + 3, 5, CLR_NORML, "Press [F1] to exit game.");

		color(CLR_NORML);
		ReleaseMutex(mutexGame);
		key = _getch();
		WaitForSingleObject(mutexGame, INFINITE);
		if (haveGame) {

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
			if (key == ENTER && counter % 2 == turn)
			{
				if (board[pointerX][pointerY] == EMPTY)
				{
					sock->send(Message(REQ_PLAY_CHESS, reform(pointerX, 2) + "$" + reform(pointerY, 2)).toMessageSend());
					system("CLS");
				}
			}
			if (key == STOP)
			{
				int choice = MessageBox(NULL, L"Do you want to quit game?", L"Exit game", MB_OKCANCEL);
				if (choice == 1)
				{
					sock->send(Message(REQ_STOP_GAME, "").toMessageSend());
				}
			}
		}
		else {
			if (key == 61) {
				currentScreen = SCREEN_MAIN;
				newScreen = true;
				ReleaseMutex(mutexGame);
				break;
			}
		}
		ReleaseMutex(mutexGame);
	}
}