#pragma once

#define BOARD_WIDTH 32
#define BOARD_HEIGHT 32
#define STREAK_TO_WIN 5

#define WIN true
#define NOT_WIN false


#define X_WIN 1
#define O_WIN 0
#define GAME_DRAW -1

#include "Account.h"

struct Match {
	Account *xAcc;
	Account *oAcc;
	int win = 0;
	int numberXPlay = 0;
	int numberOPlay = 0;
	string nameLogFile;

	char board[BOARD_WIDTH][BOARD_HEIGHT];

	Match(Account *_xAcc, Account *_oAcc) {
		memset(board, NULL, sizeof(board));
		this->xAcc = _xAcc;
		this->oAcc = _oAcc;
		nameLogFile = this->xAcc->username + "vs" + this->oAcc->username + "_" + getCurrentDateTime("%d_%m_%Y_%H_%M_%S") + ".txt";
	}

	bool xCanPlay(int _x, int _y) {
		return numberXPlay == numberOPlay && board[_x][_y] == NULL;
	}

	bool oCanPlay(int _x, int _y) {
		return numberXPlay == numberOPlay + 1 && board[_x][_y] == NULL;
	}
	
	int xPlay(int _x, int _y) {
		numberXPlay++;
		board[_x][_y] = 'x';
		if (numberXPlay + numberOPlay == BOARD_WIDTH * BOARD_HEIGHT) {
			return GAME_DRAW;
		}
		return winCheck('x', _x, _y);
	}

	int oPlay(int _x, int _y) {
		numberOPlay++;
		board[_x][_y] = 'o';
		if (numberXPlay + numberOPlay == BOARD_WIDTH * BOARD_HEIGHT) {
			return GAME_DRAW;
		}
		return winCheck('o', _x, _y);
	}

	private: bool winCheck(char c, int _x, int _y) {
		return checkRow(c, _x, _y) || checkCol(c, _x, _y) || checkPrimaryDiagon(c, _x, _y) || checkSubDiagon(c, _x, _y);
	}

	private: bool checkRow(char c, int _x, int _y) {
		int count = 1;
		int right = _y + 1;
		int left = _y - 1;
		while (right < BOARD_HEIGHT) {
			if (this->board[_x][right] == c) {
				count++;
				if (count >= STREAK_TO_WIN) return true;
			}
			else break;
			right++;
		}

		while (left >= 0) {
			if (this->board[_x][left] == c) {
				count++;
				if (count >= STREAK_TO_WIN) return true;
			}
			else break;
			left--;
		}
		return false;
	}
	
	private: bool checkCol(char c, int _x, int _y) {
		int count = 1;
		int up = _x - 1;
		int down = _x + 1;
		while (up >= 0) {
			if (this->board[up][_y] == c) {
				count++;
				if (count >= STREAK_TO_WIN) return true;
			}
			else break;
			up++;
		}

		while (down < BOARD_HEIGHT) {
			if (this->board[down][_y] == c) {
				count++;
				if (count >= STREAK_TO_WIN) return true;
			}
			else break;
			down--;
		}

		return false;
	}

	private: bool checkPrimaryDiagon(char c, int _x, int _y) {
		int count = 1;
		int delta_diagon = 1;
		while (_x - delta_diagon >= 0 && _y - delta_diagon >= 0) {
			if (this->board[_x - delta_diagon][_y - delta_diagon] == c) {
				count++;
				if (count >= STREAK_TO_WIN) return true;
			}
			else break;
			delta_diagon++;
		}

		delta_diagon = 1;
		while (_x + delta_diagon < BOARD_WIDTH && _y + delta_diagon < BOARD_HEIGHT) {
			if (this->board[_x + delta_diagon][_y + delta_diagon] == c) {
				count++;
				if (count >= STREAK_TO_WIN) return true;
			}
			else break;
			delta_diagon++;
		}
		return false;
	}

	private: bool checkSubDiagon(char c, int _x, int _y) {
		int count = 1;
		int delta_diagon = 1;
		while (_x + delta_diagon < BOARD_WIDTH && _y - delta_diagon >= 0) {
			if (this->board[_x + delta_diagon][_y - delta_diagon] == c) {
				count++;
				if (count >= STREAK_TO_WIN) return true;
			}
			else break;
			delta_diagon++;
		}

		delta_diagon = 1;
		while (_x + delta_diagon >= 0 && _y + delta_diagon < BOARD_HEIGHT) {
			if (this->board[_x - delta_diagon][_y + delta_diagon] == c) {
				count++;
				if (count >= STREAK_TO_WIN) return true;
			}
			else break;
			delta_diagon++;
		}
		return false;
	}

};