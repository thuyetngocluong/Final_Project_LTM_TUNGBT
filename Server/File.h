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

struct File {
	string fileName = "";
	streamsize pos = 0;

	File() {}

	
	File(string _fileName) {
		this->fileName = fileName;
	}


	char *toCharArraySend(streamsize pos, int opCode) {
		ifstream f;

		f.open(fileName.c_str(), ios::in | ios::binary);
		
		if (!f.is_open()) return NULL;

		streambuf *sb = f.rdbuf();
		streamsize size = sb->pubseekoff(pos, f.end);
		streamsize sizePayload = 0;
		if (size < BLOCK_SIZE) {
			sizePayload = size;
		}
		else {
			sizePayload = BLOCK_SIZE;
		}

		char *payload = new char[sizePayload];

		streamsize sizeRest = 0;

		while (sizeRest < sizePayload) {
			sb->pubseekpos(pos + sizeRest);

			streamsize bSuccess = sb->sgetn(payload + sizeRest, sizePayload - sizeRest);

			sizeRest += bSuccess;
		}

		char *msg = new char[SIZE_OPCODE + SIZE_LENGTH + sizePayload];
		int index = 0;
		copy(msg, index, to_string(opCode), SIZE_OPCODE);
		index += SIZE_OPCODE;

		copy(msg, index, reform(sizePayload, SIZE_LENGTH), SIZE_LENGTH);
		index += SIZE_LENGTH;
		copy(msg, index, payload, sizePayload);
	
		f.close();

		return msg;
	}
};


/*
* @function getStatus: get status of username
* @param request: message brings username
*
* @return LOCK, ACTIVE or NOT_EXIST
**/
int getStatus(Message request, char* name) {
	string line;
	ifstream myfile(name);

	size_t found = request.content.find("$");
	string username = request.content.substr(0, found);
	string password = request.content.substr(found + 1);

	if (myfile.is_open()) {
		while (getline(myfile, line)) {
			string user;
			int status = NOT_EXIST;

			stringstream ss(line);

			ss >> user;
			ss >> status;

			if (strcmp(user.c_str(), request.content.c_str()) == 0) {
				return status;
			}
		}
		myfile.close();
	}

	return NOT_EXIST;
}


void writeNewLogFile(char* name) {
	ofstream out(name, ios::app);
	ifstream in(name);
	if (in.is_open()) {
		bool empty = (in.get(), in.eof());
		if (!empty) {
			in.close();
			return;
		}
		in.close();
		out << "****************************************" << std::endl;
		out << "     Meaning               Result Code" << std::endl;
		out << "UNDENTIFY_RESULT               000" << std::endl << std::endl;
		out << "LOGIN_SUCCESSFUL               100" << std::endl;
		out << "LOGIN_FAIL_LOCKED              111" << std::endl;
		out << "LOGIN_FAIL_NOT_EXIST           112" << std::endl;
		out << "LOGIN_FAIL_ANOTHER_DEVICE      113" << std::endl;
		out << "LOGIN_FAIL_ALREADY_LOGIN       114" << std::endl << std::endl;
		out << "LOGOUT_SUCCESSFUL              300" << std::endl;
		out << "LOGOUT_FAIL                    310" << std::endl;
		out << "LOGOUT_FAIL_NOT_LOGIN          311" << std::endl << std::endl;
		out << "POST_SUCCESSFUL                200" << std::endl;
		out << "POST_FAIL                      210" << std::endl;
		out << "POST_FAIL_NOT_LOGIN            211" << std::endl;
		out << "****************************************" << std::endl << std::endl;
	}
	out.close();
}

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
