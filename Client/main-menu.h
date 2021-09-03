#pragma once
#include "common.h"

#define SEPARATOR_VERTICAL_MAIN_MENU '\xB3'
#define SEPARATOR_HORIZONTAL_MAIN_MENU '\xC4'
#define WIDTH_SCREEN_TITLE 40
#define WIDTH_SCREEN_CONTENT 40

#define HEADER 0
#define CONTENT 1
#define TITLE 2
#define PAGE 3

#define KEY_UP      72
#define KEY_DOWN    80
#define KEY_LEFT    75
#define KEY_RIGHT   77


#define MAX_CONTENT 10

int selectingTitle = 0;
int selectingContent = -1;
int selectingPage = 0;

vector<pair<string, vector<string>>> dataSource;

void drawTemplate();
void drawContent(int, int);
void drawTitle();
void drawTitle(int);

void drawSelectTitle(int, int);
void drawSelectContent(int, int);

void drawPage();

void drawHeader(string, bool);


void updateListFriend(vector<string> listFriend) {
	dataSource[0].second = listFriend;
	drawTitle(0);
	if (selectingTitle == 0) { drawContent(0, selectingPage); }
}

void updateListChallenge(vector<string> listChallenge) {
	dataSource[1].second = listChallenge;
	drawTitle(1);
	if (selectingTitle == 1) { drawContent(1, selectingPage); }
}

void updateListInvitationFriend(vector<string> listInvitationFriend) {
	dataSource[2].second = listInvitationFriend;
	drawTitle(2);
	if (selectingTitle == 2) { drawContent(2, selectingPage); }
}

void updateListInvitationChallenge(vector<string> listInvitationChallenge) {
	dataSource[3].second = listInvitationChallenge;
	drawTitle(3);
	if (selectingTitle == 3) { drawContent(3, selectingPage); }
}

pair<int, int> getMenu(int _selectTitle, int _selectContent)
{
	selectingTitle = _selectTitle;
	selectingContent = _selectContent;
	int sizeTitles = dataSource.size();
	int sizeContents = 0;
	int pointer = 0;
	char key;
	int focusMode = TITLE;

	WaitForSingleObject(mutexx, INFINITE);
	drawTemplate();
	drawTitle();
	drawContent(selectingTitle, selectingPage);
	drawPage();
	ReleaseMutex(mutexx);

	while (1)
	{
		color(CLR_NORML);
		key = _getch();

		WaitForSingleObject(mutexx, INFINITE);

		sizeContents = dataSource[selectingTitle].second.size();

		switch (key) {
		case KEY_UP:
			switch (focusMode) {
			case TITLE:
				pointer = (pointer + sizeTitles - 1) % sizeTitles;
				drawSelectTitle(selectingTitle, pointer);
				selectingTitle = pointer;
				drawContent(selectingTitle, selectingPage);
				break;
			case CONTENT:
				pointer = (pointer + sizeContents - 1) % sizeContents;
				if (selectingPage != pointer / MAX_CONTENT) {
					selectingPage = pointer / MAX_CONTENT;
					drawContent(selectingTitle, selectingPage);
					drawPage();
				};
				drawSelectContent(selectingContent, pointer);
				selectingContent = pointer;
				break;
			}
			break;

		case KEY_DOWN:
			switch (focusMode) {
			case TITLE:
				pointer = (pointer + sizeTitles + 1) % sizeTitles;
				drawSelectTitle(selectingTitle, pointer);
				selectingTitle = pointer;
				drawContent(selectingTitle, selectingPage);
				break;
			case CONTENT:
				pointer = (pointer + sizeContents + 1) % sizeContents;
				if (selectingPage != pointer / MAX_CONTENT) {
					selectingPage = pointer / MAX_CONTENT;
					drawContent(selectingTitle, selectingPage);
					drawPage();
				};
				drawSelectContent(selectingContent, pointer);
				selectingContent = pointer;
				break;
			}
			break;

		case KEY_LEFT:
			switch (focusMode) {
			case CONTENT:
				focusMode = TITLE;
				selectingContent = -1;
				selectingPage = 0;
				drawContent(selectingTitle, selectingPage);
				drawHeader("", false);
				drawPage();
				pointer = selectingTitle;
				break;
			case PAGE:
				break;
			}
			break;

		case KEY_RIGHT:
			switch (focusMode) {
			case TITLE:
				if (sizeContents <= 0) break;
				focusMode = CONTENT;
				selectingContent = 0;
				pointer = selectingContent;
				drawSelectContent(selectingContent, pointer);
				break;
			case PAGE:

				break;
			}
			break;
		}
		ReleaseMutex(mutexx);



		if (key == ENTER)
		{
			break;
		}

	}

	return{ selectingTitle, selectingContent };
}


/// Implement Function

void drawTemplate() {
	color(CLR_NORML);
	for (int i = 3; i < rows - 4; i++) {
		gotoxy(WIDTH_SCREEN_TITLE + 1, i);
		cout << SEPARATOR_VERTICAL_MAIN_MENU;
	}

	for (int i = 0; i < columns; i++) {
		gotoxy(i, 3);
		cout << SEPARATOR_HORIZONTAL_MAIN_MENU;
	}
}

void drawPage() {
	printItem(WIDTH_SCREEN_TITLE + 17, MAX_CONTENT * 2 + 5, CLR_NORML, "Page " + to_string(selectingPage), ' ', 20);
}

void drawContent(int titleIndex, int page) {

	if (selectingTitle != titleIndex || selectingTitle < 0 || selectingTitle >= dataSource.size()) return;

	vector<string> itemContent = dataSource[selectingTitle].second;

	if (page * 10 > itemContent.size()) return;
	int i = 0;
	for (i = 0; i < itemContent.size() - page * MAX_CONTENT && i < MAX_CONTENT; i++) {
		printItem(WIDTH_SCREEN_TITLE + 5, i * 2 + 4, CLR_NORML, "  " + itemContent[i + MAX_CONTENT * page], ' ', WIDTH_SCREEN_CONTENT);
	}

	if (selectingContent >= 0 && selectingContent < itemContent.size()) {
		printItem(WIDTH_SCREEN_TITLE + 5, (selectingContent % MAX_CONTENT) * 2 + 4, COLOR_RED, "> " + itemContent[selectingContent], ' ', WIDTH_SCREEN_TITLE);
	}

	for (int j = i; j < MAX_CONTENT; j++) {
		printItem(WIDTH_SCREEN_TITLE + 5, j * 2 + 4, CLR_NORML, "", ' ', WIDTH_SCREEN_CONTENT);
	}
}


void drawTitle(int titleIndex) {
	if (titleIndex >= 0 && titleIndex < dataSource.size()) {
		if (selectingTitle == titleIndex) {
			printItem(0, titleIndex * 3 + 4, COLOR_GREEN, "> " + dataSource[titleIndex].first + " (" + to_string(dataSource[titleIndex].second.size()) + ")", ' ', WIDTH_SCREEN_TITLE);
		}
		else {
			printItem(0, titleIndex * 3 + 4, CLR_NORML, "  " + dataSource[titleIndex].first + " (" + to_string(dataSource[titleIndex].second.size()) + ")", ' ', WIDTH_SCREEN_TITLE);
		}
	}
}

void drawTitle() {
	for (int i = 0; i < dataSource.size(); i++) {
		printItem(0, i * 3 + 4, CLR_NORML, "  " + dataSource[i].first + " (" + to_string(dataSource[i].second.size()) + ")", ' ', WIDTH_SCREEN_TITLE);
	}
	if (selectingTitle >= 0 && selectingTitle < dataSource.size()) {
		printItem(0, selectingTitle * 3 + 4, COLOR_GREEN, "> " + dataSource[selectingTitle].first + " (" + to_string(dataSource[selectingTitle].second.size()) + ")", ' ', WIDTH_SCREEN_TITLE);
	}
}


void drawSelectTitle(int prevSelect, int currentSelect) {
	printItem(0, prevSelect * 3 + 4, CLR_NORML, "  " + dataSource[prevSelect].first + " (" + to_string(dataSource[prevSelect].second.size()) + ")", ' ', WIDTH_SCREEN_TITLE);
	printItem(0, currentSelect * 3 + 4, COLOR_GREEN, "> " + dataSource[currentSelect].first + " (" + to_string(dataSource[currentSelect].second.size()) + ")", ' ', WIDTH_SCREEN_TITLE);
}


void drawSelectContent(int prevSelect, int currentSelect) {

	int sizeContent = dataSource[selectingTitle].second.size();

	printItem(WIDTH_SCREEN_TITLE + 5, (prevSelect % MAX_CONTENT) * 2 + 4, CLR_NORML, "  " + dataSource[selectingTitle].second[prevSelect], ' ', WIDTH_SCREEN_CONTENT);
	printItem(WIDTH_SCREEN_TITLE + 5, (currentSelect  % MAX_CONTENT) * 2 + 4, COLOR_RED, "> " + dataSource[selectingTitle].second[currentSelect], ' ', WIDTH_SCREEN_CONTENT);

	drawHeader(dataSource[selectingTitle].second[currentSelect], true);
}


void drawHeader(string content, bool showChooseBox) {
	printItem(0, 0, COLOR_RED, content, ' ', 30);
	if (showChooseBox) {
		printItem(0, 1, CLR_NORML, "[Y] Yes");
		printItem(0, 2, CLR_NORML, "[N] No");
	}
	else {
		printItem(0, 1, CLR_NORML, "", ' ', 20);
		printItem(0, 2, CLR_NORML, "", ' ', 20);
	}
}