#pragma once

#include "common.h"

/**
 *
 *
 */
int getMenu(string items[])
{
	int pointer = 0;
	char key;
	printItem(columns / 2 - 4 - 6, 2, CLR_NORML, "WELLCOME TO CARO TAT");
	printItem(columns / 2 - 2, 6, CLR_NORML, "MENU");
	while (1)
	{

		printItem(columns / 2 - 20, 9, CLR_NORML, "", SEPARATOR_HORIZONTAL, 39);
		printItem(columns / 2 - 20, 19, CLR_NORML, "", SEPARATOR_HORIZONTAL, 39);


		for (int i = 10; i < 19; i++) {
			gotoxy(columns / 2 - 20, i);
			cout << SEPARATOR_VERTICAL;

			gotoxy(columns / 2 - 20 + 38, i);
			cout << SEPARATOR_VERTICAL;

		}

		for (int i = 0; i < 3; i++)
		{
			if (i == pointer) {
				printItem(columns / 2 - 6, 2 * i + 12, COLOR_GREEN, ">   " + items[i], ' ', 9, ALIGN_LEFT);
			}
			else {
				printItem(columns / 2 - 6, 2 * i + 12, CLR_NORML, "    " + items[i], ' ', 9, ALIGN_LEFT);
			}
		}
		color(CLR_NORML);
		key = _getch();

		if (key == UP)
		{
			if (pointer > 0) pointer--;
			else pointer = 0;
		}
		if (key == DOWN)
		{
			if (pointer < 2) pointer++;
			else pointer = 1;
		}
		if (key == ENTER)
		{
			if (pointer < 3 && pointer >= 0)
			{
				break;
			}
		}
	}
	return pointer;
}

int getMenu1(string items[])
{
	int pointer = 0;
	char key;
	while (1)
	{
		for (int i = 0; i < 2; i++)
		{
			if (i == pointer) {
				printItem(columns*(i + 1) / 3 - 2, 17, COLOR_GREEN, items[i]);
			}
			else {
				printItem(columns*(i + 1) / 3 - 2, 17, CLR_NORML, items[i]);

			}
		}
		color(CLR_NORML);
		key = _getch();

		if (key == LEFT)
		{
			if (pointer > 0) pointer--;
			else pointer = 0;
		}
		if (key == RIGHT)
		{
			if (pointer < 1) pointer++;
			else pointer = 1;
		}
		if (key == ENTER)
		{
			if (pointer < 2 && pointer >= 0)
			{
				break;
			}
		}
	}
	return pointer;
}