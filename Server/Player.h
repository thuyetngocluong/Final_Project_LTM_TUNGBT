#pragma once

class Player {
	private:
		long id = 0;
		string username = "";
		long elo = 0;
public:
	Player(long _id, string _username, long _elo) {
		this->id = _id;
		this->username = _username;
		this->elo = _elo;
	}

	long getID() {
		return this->id;
	}

	string getUsername() {
		return this->username;
	}

	long getElo() {
		return this->elo;
	}

	long setElo(long _elo) {
		this->elo = _elo;
	}
};