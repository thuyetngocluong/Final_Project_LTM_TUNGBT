#pragma once

#define BOARD_WIDTH 32
#define BOARD_HEIGHT 32

#include "Account.h"


struct Match {
	SOCKET xSock;
	SOCKET ySock;
	char board[BOARD_WIDTH][BOARD_HEIGHT];

	Match(SOCKET &_xSock, SOCKET &_ySock) {
		memset(board, NULL, sizeof(board));
		this->xSock = _xSock;
		this->ySock = _ySock;
	}

	void xPlay(int _x, int _y) {
		board[_x][_y] = 'x';
	}

	void oPlay(int _x, int _y) {
		board[_x][_y] = 'o';
	}

	bool winCheck(char c, int _x, int _y) {
		return fiveInARow(c, _x, _y) || fiveInACol(c, _x, _y) || fiveInAPrimaryDiagon(c, _x, _y) || fiveInASubDiagon(c, _x, _y);
	}

	private: bool fiveInARow(char c, int _x, int _y) {
		int count = 1;
		int right = _y + 1;
		int left = _y - 1;
		while (right < BOARD_HEIGHT) {
			if (this->board[_x][right] == c) {
				count++;
				if (count >= 5) return true;
			}
			else break;
			right++;
		}

		while (left >= 0) {
			if (this->board[_x][left] == c) {
				count++;
				if (count >= 5) return true;
			}
			else break;
			left--;
		}
		return false;
	}
	
	private: bool fiveInACol(char c, int _x, int _y) {
		int count = 1;
		int up = _x - 1;
		int down = _x + 1;
		while (up >= 0) {
			if (this->board[up][_y] == c) {
				count++;
				if (count >= 5) return true;
			}
			else break;
			up++;
		}

		while (down < BOARD_HEIGHT) {
			if (this->board[down][_y] == c) {
				count++;
				if (count >= 5) return true;
			}
			else break;
			down--;
		}

		return false;
	}

	private: bool fiveInAPrimaryDiagon(char c, int _x, int _y) {
		int count = 1;
		int delta_diagon = 1;
		while (_x - delta_diagon >= 0 && _y - delta_diagon >= 0) {
			if (this->board[_x - delta_diagon][_y - delta_diagon] == c) {
				count++;
				if (count >= 5) return true;
			}
			else break;
			delta_diagon++;
		}

		delta_diagon = 1;
		while (_x + delta_diagon < BOARD_WIDTH && _y + delta_diagon < BOARD_HEIGHT) {
			if (this->board[_x + delta_diagon][_y + delta_diagon] == c) {
				count++;
				if (count >= 5) return true;
			}
			else break;
			delta_diagon++;
		}
		return false;
	}

	private: bool fiveInASubDiagon(char c, int _x, int _y) {
		int count = 1;
		int delta_diagon = 1;
		while (_x + delta_diagon < BOARD_WIDTH && _y - delta_diagon >= 0) {
			if (this->board[_x + delta_diagon][_y - delta_diagon] == c) {
				count++;
				if (count >= 5) return true;
			}
			else break;
			delta_diagon++;
		}

		delta_diagon = 1;
		while (_x + delta_diagon >= 0 && _y + delta_diagon < BOARD_HEIGHT) {
			if (this->board[_x - delta_diagon][_y + delta_diagon] == c) {
				count++;
				if (count >= 5) return true;
			}
			else break;
			delta_diagon++;
		}
		return false;
	}

};