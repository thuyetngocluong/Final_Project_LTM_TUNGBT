#pragma once

#define CLR_FOCUS 207
#define CLR_NORML 7
#define COLOR_GREEN 10
#define COLOR_RED 12
#define CLR_X     27
#define CLR_O     32


#define SEPARATOR_VERTICAL '\xDB'
#define SEPARATOR_HORIZONTAL '\xDB'

#define WIDTH   20
#define HEIGHT  20

#define UP      72
#define DOWN    80
#define LEFT    75
#define RIGHT   77
#define ENTER   13
#define STOP    59
#define PLAY	60

#define EMPTY   " - "
#define HAS_X   " X "
#define HAS_O   " O "
#define ALIGN_LEFT 0
#define ALIGN_RIGHT 1

/*
 * @function	color: set console text color
 * @param		color: color coding in decimal
 *
 */
void color(int color)
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

/*
 * @function	gotoxy: set console cursor to position
 * @param		x: horizontal coordinates of console
 * @param		y: vertical coordinates of console
 */
void gotoxy(int x, int y)
{
	COORD c;
	c.X = x;
	c.Y = y;
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), c);
}

/*
 * @function	ShowConsoleCursor: set the console cursor visibility or invisibility
 * @param		showFlag: true - the console cursor visibility
 						  false - the console cursor invisibility
 */
void ShowConsoleCursor(bool showFlag)
{
	CONSOLE_CURSOR_INFO cursorInfo;

	GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
	cursorInfo.bVisible = showFlag; // set the cursor visibility
	SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
}

/*
 * @function	print: print content in console
 * @param		x: horizontal coordinates of console
 * @param		y: vertical coordinates of console
 * @param		colour: text color of the content
 * @param		content: text to print
 */
void print(int x, int y, int colour, string content)
{
	gotoxy(x, y);
	color(colour);
	cout << content;
}

void printItem(int x, int y, int colour, string item, char fillChar = ' ', int lengthItem = -1, int fillMode = ALIGN_LEFT)
{
	gotoxy(x, y);
	color(colour);

	if (lengthItem == -1) {
		cout << item;
	}
	else
		if (fillMode == ALIGN_LEFT) {
			cout << left << setw(lengthItem) << setfill(fillChar) << item;
		}
		else if (fillMode == ALIGN_RIGHT) {
			cout << right << setw(lengthItem) << setfill(fillChar) << item;
		}
}