#pragma once

#define LOGGED 1
#define NOT_LOGGED 0

#define IN_GAME 1
#define NOT_IN_GAME 0

/*Struct contains information of the socket communicating with client*/

struct Account {
	char IP[INET_ADDRSTRLEN];//# ip of account
	int PORT;//#port of account
	string username = "";//#username which the account logged in
	int signInStatus = NOT_LOGGED;//# login status of the account
	int matchStatus = NOT_IN_GAME;

	string restMessage = "";
	std::queue<Message> requests;

	HANDLE mutex;
	Account() {
		mutex = CreateMutex(0, 0, 0);
	}
};


/*
* @function getInforAccount: get information of a account
* @param account: account need to get information
*
* @return: a string brings information of account
**/
string getInforAccount(Account *account) {
	string rs = account->IP;
	rs += ":";
	rs += std::to_string(account->PORT);

	return rs;
}


/*
* @funtion getCurrentDateTime: get current date and time
*
* @return string brings information of current date and time
**/
string getCurrentDateTime() {
	char MY_TIME[22];
	struct tm newtime;
	__time64_t long_time;
	errno_t err;

	// Get time as 64-bit integer.
	_time64(&long_time);
	// Convert to local time.
	err = _localtime64_s(&newtime, &long_time);
	if (err) {
		return "[00/00/00 00:00:00]";
	}
	strftime(MY_TIME, sizeof(MY_TIME), "[%d/%m/%Y %H:%M:%S]", &newtime);
	string result = MY_TIME;

	return result;
}

/*
* @function saveLog: save request of account to file
* @param account: the information of account
* @param message: the information of message
* @param statusCode: status Code after account perform request
*
* @return true if save successfully, false if not
**/
bool saveLog(Account *account, Message &message, int resultCode) {
	
	string rs = getInforAccount(account) + " " + getCurrentDateTime();

	return save(rs.c_str());
}




