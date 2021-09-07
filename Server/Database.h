#pragma once

#define URL "tcp://127.0.0.1"
#define USER "root"
#define PASS "Poypoy2k"
#define DB "ltm"

bool operator == (string &a, string &b) {
	return a.compare(b.c_str()) == 0;
}

class Data {

private:
	SAConnection conn;
	Data();

public:
	Data(Data &other) = delete;

	void operator=(const Data &) = delete;

	static Data* getInstance();

	Player getPlayerByName(string username);

	Player getPlayerByID(long id);

	long getElo(string ofUsername);

	void updateElo(string ofUsername, long elo);

	void updateStatus(string ofUsername, int status);

	vector<Player> getListFriend(Player player);

	bool addFriendRelation(string username1, string username2);

	bool login(string username, string password);

	bool checkValidChallenge(string fromPlayer, string toPlayer);

	vector<Player> getListPlayerCanChallenge(string player);

	bool registerAccount(string username, string password);
};


Data::Data() {
	try {
		conn.Connect(_TSA(DB), _TSA(USER), _TSA(PASS), SA_MySQL_Client);
	}
	catch (SAException &x) {
		conn.Rollback();
		printf("%s\n", x.ErrText().GetMultiByteChars());
	}
}

Data* Data::getInstance() {
	static Data *instance;
	if (instance == nullptr) {
		instance = new Data();
	}
	return instance;
}

Player Data::getPlayerByName(string username) {
	long elo = 0;
	long id = 0;
	SACommand query(&conn);
	try {
		string sql = "SELECT * FROM account WHERE username like '" + username + "'";
		query.setCommandText(sql.c_str());
		query.Execute();
		query.FetchFirst();
		id = query["id"].asLong();
		elo = query["elo"].asLong();
		query.Close();
	}
	catch (SAException &x)
	{
		// print error message
		printf("%s\n", x.ErrText().GetMultiByteChars());
		query.Close();
	}
	return Player(id, username, elo);
}


Player Data::getPlayerByID(long id) {
	string username = "";
	long elo = 0;
	SACommand query(&conn);
	try {
		string sql = "SELECT * FROM account WHERE id = '" + to_string(id) + "'";
		query.setCommandText(sql.c_str());
		query.Execute();
		query.FetchFirst();
		username = query["username"].asString().GetMultiByteChars();
		elo = query["elo"].asLong();
		query.Close();
	}
	catch (SAException &x)
	{
		// print error message
		printf("%s\n", x.ErrText().GetMultiByteChars());
		query.Close();
	}
	return Player(id, username, elo);
}


long Data::getElo(string ofUsername) {
	long elo = 0;
	SACommand query(&conn);
	try {
		string sql = "SELECT * FROM account WHERE username like '" + ofUsername + "'";
		query.setCommandText(sql.c_str());
		query.Execute();
		query.FetchFirst();
		elo = query["elo"].asLong();
		query.Close();
	}
	catch (SAException &x)
	{
		// print error message
		printf("%s\n", x.ErrText().GetMultiByteChars());
		query.Close();
	}
	return elo;
}

void Data::updateElo(string ofUsername, long elo) {
	SACommand update(&conn);
	try {
		string sql = "UPDATE account SET elo = " + to_string(elo) + " WHERE username like '" + ofUsername + "'";
		update.setCommandText(sql.c_str());
		update.Execute();
		conn.Commit();
		update.Close();
	}
	catch (SAException &x)
	{
		// print error message
		printf("%s\n", x.ErrText().GetMultiByteChars());
		update.Close();
	}
}


void Data::updateStatus(string ofUsername, int status) {
	SACommand update(&conn);
	try {
		string sql = "UPDATE account SET status = " + to_string(status) + " WHERE username like '" + ofUsername + "'";
		update.setCommandText(sql.c_str());
		update.Execute();
		conn.Commit();
		update.Close();
	}
	catch (SAException &x)
	{
		// print error message
		printf("%s\n", x.ErrText().GetMultiByteChars());
		update.Close();
	}
}


vector<Player> Data::getListFriend(Player player) {

	vector<Player> rs;
	SACommand query(&conn);

	try {
		string sql = "SELECT * FROM account where status = 1 AND id IN (SELECT id_person2 as id FROM friend WHERE id_person1 = " + to_string(player.getID()) +
			" UNION SELECT id_person1 as id FROM friend WHERE id_person2 = " + to_string(player.getID()) + ")";
		query.setCommandText(sql.c_str());
		query.Execute();
		while (query.FetchNext()) {
			long id = query["id"].asLong();
			string username = query["username"].asString().GetMultiByteChars();
			long elo = query["elo"].asLong();

			rs.push_back(Player(id, username, elo));
		}

		query.Close();
	}
	catch (SAException &x)
	{
		// print error message
		printf("%s\n", x.ErrText().GetMultiByteChars());
		query.Close();
	}
	return rs;
}


bool Data::addFriendRelation(string username1, string username2) {
	Player player1 = getPlayerByName(username1);
	Player player2 = getPlayerByName(username2);

	bool successful = false;
	SACommand insert(&conn);

	try {
		string sql = "INSERT INTO friend (`id_person1`, `id_person2`) VALUES ( '" + to_string(player1.getID()) + "', '" + to_string(player2.getID()) + "')";
		insert.setCommandText(sql.c_str());

		insert.Execute();
		successful = true;
		conn.Commit();
		insert.Close();
	}
	catch (SAException &x)
	{
		// print error message
		printf("%s\n", x.ErrText().GetMultiByteChars());
		insert.Close();
	}
	return successful;
}


bool Data::login(string username, string password) {
	SACommand query(&conn);
	try {
		string sql = "SELECT * FROM account WHERE username like '" + username + "' AND password like '" + password + "' " + "AND status = 0";

		query.setCommandText(sql.c_str());
		query.Execute();
		if (query.FetchNext()) {
			query.Close();
			return true;
		}
		query.Close();
	}
	catch (SAException &x)
	{
		// print error message
		printf("%s\n", x.ErrText().GetMultiByteChars());
		query.Close();
	}
	return false;
}


bool Data::checkValidChallenge(string fromPlayer, string toPlayer) {

	vector<Player> players = getListPlayerCanChallenge(fromPlayer);
	for (int i = 0; i < players.size(); i++) {
		if (players[i].getUsername() == toPlayer) {
			return true;
		}
	}
	return false;
}


vector<Player> Data::getListPlayerCanChallenge(string player) {
	SACommand query(&conn);
	vector<Player> rs;
	try {
		string sql = "SELECT * FROM account WHERE elo  < (select elo from account where username like '" + player + "') GROUP BY elo ORDER BY elo DESC LIMIT 10";
		query.setCommandText(sql.c_str());
		query.Execute();
		while (query.FetchNext()) {
			long id = query["id"].asLong();
			string username = query["username"].asString().GetMultiByteChars();
			long elo = query["elo"].asLong();
			long status = query["status"].asLong();
			if (status == 1) {
				rs.push_back(Player(id, username, elo));
			}
		}

		sql = "SELECT * FROM account WHERE elo  > (select elo from account where username like '" + player + "') GROUP BY elo ORDER BY elo ASC LIMIT 10";
		query.setCommandText(sql.c_str());
		query.Execute();
		while (query.FetchNext()) {
			long id = query["id"].asLong();
			string username = query["username"].asString().GetMultiByteChars();
			long elo = query["elo"].asLong();
			long status = query["status"].asLong();
			if (status == 1) {
				rs.push_back(Player(id, username, elo));
			}
		}
		sql = "SELECT * FROM account WHERE username not like '" + player + "'  AND elo in (select elo from account where username like '" + player + "')"; 
		query.setCommandText(sql.c_str());
		query.Execute();
		while (query.FetchNext()) {
			long id = query["id"].asLong();
			string username = query["username"].asString().GetMultiByteChars();
			long elo = query["elo"].asLong();
			long status = query["status"].asLong();
			if (status == 1) {
				rs.push_back(Player(id, username, elo));
			}
		}
		query.Close();
	}
	catch (SAException &x)
	{
		// print error message
		printf("%s\n", x.ErrText().GetMultiByteChars());
		query.Close();
	}
	return rs;
}



bool Data::registerAccount(string username, string password) {
	bool successful = false;
	SACommand insert(&conn);

	try {
		string sql = "INSERT INTO account (`username`, `password`) VALUES ( '" + username + "', '" + password + "')";
		insert.setCommandText(sql.c_str());

		insert.Execute();
		successful = true;
		conn.Commit();
		insert.Close();
	}
	catch (SAException &x)
	{
		// print error message
		printf("%s\n", x.ErrText().GetMultiByteChars());
		insert.Close();
	}
	return successful;
}