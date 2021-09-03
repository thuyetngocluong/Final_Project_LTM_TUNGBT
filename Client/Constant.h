#pragma once

#define CLR_FOCUS 207
#define COLOR_RED 12
#define COLOR_GREEN 10
#define CLR_NORML 7
#define CLR_X     27
#define CLR_O     32

#define WIDTH   20
#define HEIGHT  20

#define KEY_ENTER   13
#define STOP    59

#define EMPTY   " - "
#define HAS_X   " X "
#define HAS_O   " O "

#define LEFT 0
#define RIGHT 1

void color(int color)
{
	SetConsoleTextAttribute(screen, color);
}

void gotoxy(int x, int y)
{
	COORD c;
	c.X = x;
	c.Y = y;
	SetConsoleCursorPosition(screen, c);
}

void printItem(int x, int y, int colour, string item, char fillChar = ' ', int lengthItem = -1, int fillMode = LEFT)
{
	gotoxy(x, y);
	color(colour);

	if (lengthItem == -1) {
		cout << item;
	} else
	if (fillMode == LEFT) {
		cout << left << setw(lengthItem) << setfill(fillChar) << item;
	}
	else if (fillMode == RIGHT) {
		cout << right << setw(lengthItem) << setfill(fillChar) << item ;
	}
}

void print(int x, int y, int colour, string item)
{
	gotoxy(x, y);
	color(colour);
	cout << item;
}
