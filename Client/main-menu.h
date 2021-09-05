#pragma once
#include "common.h"

#define SEPARATOR_VERTICAL_MAIN_MENU '\xB3'
#define SEPARATOR_HORIZONTAL_MAIN_MENU '\xC4'
#define WIDTH_SCREEN_TITLE 40
#define WIDTH_SCREEN_CONTENT 20

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
bool draw = false;
bool haveGame = false;

vector<pair<string, vector<pair<string, string>>>> dataSource;

void drawTemplate();
void drawContent(int, int);
void drawTitle();
void drawTitle(int);

void drawSelectTitle(int, int);
void drawSelectContent(int, int);

void drawPage();

void drawHeader(string, bool);


void updateListFriend(vector<pair<string, string>> _listFriend) {
	WaitForSingleObject(mutexx, INFINITE);
	dataSource[0].second = _listFriend;
	drawTitle(0);
	if (selectingTitle == 0) { drawContent(0, selectingPage); }
	ReleaseMutex(mutexx);
}

void updateListChallenge(vector<pair<string, string>> _listChallenge) {
	WaitForSingleObject(mutexx, INFINITE);
	dataSource[1].second = _listChallenge;
	drawTitle(1);
	if (selectingTitle == 1) { drawContent(1, selectingPage); }
	ReleaseMutex(mutexx);
}

void updateListInvitationFriend(vector<pair<string, string>> _listInvitationFriend) {
	WaitForSingleObject(mutexx, INFINITE);
	dataSource[2].second = _listInvitationFriend;
	drawTitle(2);
	if (selectingTitle == 2) { drawContent(2, selectingPage); }
	ReleaseMutex(mutexx);
}

void updateListInvitationChallenge(vector<pair<string, string>> _listInvitationChallenge) {
	WaitForSingleObject(mutexx, INFINITE);
	dataSource[3].second = _listInvitationChallenge;
	drawTitle(3);
	if (selectingTitle == 3) { drawContent(3, selectingPage); }
	ReleaseMutex(mutexx);
}

void ScreenMainMenu(SK* aSock)
{

	int sizeTitles = dataSource.size();
	int sizeContents = 0;
	int pointer = 0;
	char key;
	int focusMode = TITLE;

	sock->send(Message(REQ_GET_LIST_CHALLENGE, "").toMessageSend());
	WaitForSingleObject(mutexx, INFINITE);
	system("cls");
	drawTemplate();
	drawTitle();
	drawContent(selectingTitle, selectingPage);
	drawPage();
	draw = true;
	ReleaseMutex(mutexx);

	while (1)
	{

		color(CLR_NORML);
		char key;
		key = _getch();

		WaitForSingleObject(mutexx, INFINITE);

		int sizeContents = dataSource[selectingTitle].second.size();

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

		if (key == PLAY && haveGame) {
			break;
		}

		if (key == 'y' || key == 'Y') {
			if (selectingTitle == 0 && selectingContent != -1) {
				string name = dataSource[selectingTitle].second[selectingContent].first;
				selectingContent = -1;
				focusMode = TITLE;
				aSock->send(Message(REQ_SEND_CHALLENGE_INVITATION, name).toMessageSend());
			}

			else if (selectingTitle == 1 && selectingContent != -1) {
				string name = dataSource[selectingTitle].second[selectingContent].first;
				selectingContent = -1;
				focusMode = TITLE;
				aSock->send(Message(REQ_SEND_CHALLENGE_INVITATION, name).toMessageSend());
			}

			if (selectingTitle == 3 && selectingContent != -1) {
				string name = dataSource[selectingTitle].second[selectingContent].first;
				dataSource[selectingTitle].second.erase(dataSource[selectingTitle].second.begin() + selectingContent);
				selectingContent = -1;
				focusMode = TITLE;
				updateListInvitationChallenge(dataSource[selectingTitle].second);
				aSock->send(Message(RESPONSE, reform(RES_ACCEPT_CHALLENGE_INVITATION, SIZE_RESPONSE_CODE) + name).toMessageSend());
			}
		}

		if (key == 'n' || key == 'N') {
			if (selectingTitle == 3 && selectingContent != -1) {
				string name = dataSource[selectingTitle].second[selectingContent].first;
				dataSource[selectingTitle].second.erase(dataSource[selectingTitle].second.begin() + selectingContent);
				selectingContent = -1;
				focusMode = TITLE;
				updateListInvitationChallenge(dataSource[selectingTitle].second);
				aSock->send(Message(RESPONSE, reform(RES_DENY_CHALLENGE_INVITATION, SIZE_RESPONSE_CODE) + name).toMessageSend());
			}
		}

		if (key == ENTER && selectingTitle == 4)
		{
			aSock->send(Message(REQ_LOGOUT, "").toMessageSend());
			listFriend.clear();
			listCanChallenge.clear();
			listFriendInvitation.clear();
			listChallengeInvitation.clear();
			break;
		}

	}
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

	vector<pair<string, string>> itemContent = dataSource[selectingTitle].second;

	if (page * 10 > itemContent.size()) return;
	int i = 0;
	for (i = 0; i < itemContent.size() - page * MAX_CONTENT && i < MAX_CONTENT; i++) {
		printItem(WIDTH_SCREEN_TITLE + 5, i * 2 + 4, CLR_NORML, "  " + itemContent[i + MAX_CONTENT * page].first + " " + itemContent[i + MAX_CONTENT * page].second, ' ', WIDTH_SCREEN_CONTENT);
	}

	if (selectingContent >= 0 && selectingContent < itemContent.size()) {
		printItem(WIDTH_SCREEN_TITLE + 5, (selectingContent % MAX_CONTENT) * 2 + 4, COLOR_RED, "> " + itemContent[selectingContent].first + " " + itemContent[selectingContent].second, ' ', WIDTH_SCREEN_CONTENT);
	}

	for (int j = i; j < MAX_CONTENT; j++) {
		printItem(WIDTH_SCREEN_TITLE + 5, j * 2 + 4, CLR_NORML, " ", ' ', WIDTH_SCREEN_CONTENT);
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

	printItem(WIDTH_SCREEN_TITLE + 5, (prevSelect % MAX_CONTENT) * 2 + 4, CLR_NORML, "  " + dataSource[selectingTitle].second[prevSelect].first + " " + dataSource[selectingTitle].second[prevSelect].second, ' ', WIDTH_SCREEN_CONTENT);
	printItem(WIDTH_SCREEN_TITLE + 5, (currentSelect  % MAX_CONTENT) * 2 + 4, COLOR_RED, "> " + dataSource[selectingTitle].second[currentSelect].first + " " + dataSource[selectingTitle].second[currentSelect].second, ' ', WIDTH_SCREEN_CONTENT);

	if (selectingTitle == 0 || selectingTitle == 1) {
		drawHeader("Challenge " + dataSource[selectingTitle].second[currentSelect].first + " ?", true);
	}
	else if (selectingTitle == 3) {
		drawHeader("Play with " + dataSource[selectingTitle].second[currentSelect].first + " ?", true);
	}
}


void drawHeader(string content, bool showChooseBox) {
	printItem(0, 0, COLOR_RED, content, ' ', 60);
	if (showChooseBox) {
		printItem(0, 1, CLR_NORML, "[Y] Yes");
		printItem(0, 2, CLR_NORML, "[N] No");
	}
	else {
		printItem(0, 1, CLR_NORML, "", ' ', 20);
		printItem(0, 2, CLR_NORML, "", ' ', 20);
	}
}