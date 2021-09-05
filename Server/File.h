#pragma once


#define FILE_ACCOUNT "account.txt"
#define FILE_LOG "log_20183994.txt"

#define LOCKED 1
#define ACTIVE 0
#define NOT_EXIST -1

#define SAVE_SUCCESSFUL true
#define SAVE_FAIL false


#define SIZE_OPCODE 2
#define SIZE_LENGTH 5
#define BLOCK_SIZE 65535


/*
* @function save: save a const char to file
* @param log: log need to save
*
* @return true if save successfully, false if not
**/
bool save(const char* log, char* name) {
	ofstream myfile(name, ios::app);
	if (myfile.is_open()) {
		myfile << log;
		myfile << std::endl;
		myfile.close();
		return true;
	}

	return false;
}

bool save(string fileName, string dateTime, string username, string xx, string yy) {
	ofstream myfile(fileName.c_str(), ios::app);
	if (myfile.is_open()) {
		myfile << dateTime << "\t";
		myfile << username;
		
		if (username.length() / 8 == 0) myfile << "\t\t\t";
		if (username.length() / 8 == 1) myfile << "\t\t";
		if (username.length() / 8 == 2) myfile << "\t";
		
		myfile << "play\t";
		myfile << xx << " " << yy;
		myfile << std::endl;
		myfile.close();
		return true;
	}

	return false;
}

bool save(string fileName, string username, string ip, string port) {
	ofstream myfile(fileName.c_str(), ios::app);
	if (myfile.is_open()) {
		myfile << username;

		if (username.length() / 8 == 0) myfile << "\t\t\t";
		if (username.length() / 8 == 1) myfile << "\t\t";
		if (username.length() / 8 == 2) myfile << "\t";

		myfile << "IP:\t";
		myfile << ip << ":" << port;
		myfile << std::endl;
		myfile.close();
		return true;
	}

	return false;
}

string getListFriendOnline(string username) {
	string rs;

	return rs;
}
