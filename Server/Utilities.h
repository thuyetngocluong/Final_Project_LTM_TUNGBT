#pragma once

/*
* @function reform: reform a integer to string having numberOfChar characters
* @function a: the integer need to reform
* @function numberOfChars: the number of characters result
*
* @return: a string having numberOfChars characters
**/
string reform(int a, int numberOfChars) {
	string rs;
	string numStr = std::to_string(a);
	for (int i = 0; i < numberOfChars - numStr.length(); i++) {
		rs += "0";
	}
	rs += numStr;

	return rs;
}

void reform(string &a, int numberOfChars) {
	for (int i = 0; i < numberOfChars - a.length(); i++) {
		a += " ";
	}
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
* @function isNumber: Check if a string is a number or not
* @param s: string needs to be check
*
* @return true if string is a number, false if string not a number
**/
bool isNumber(char *s) {
	int i = 0;

	while (s[i] != '\0') {
		if (isdigit(s[i]) == false) return false;
		i++;
	}

	return true;
}
