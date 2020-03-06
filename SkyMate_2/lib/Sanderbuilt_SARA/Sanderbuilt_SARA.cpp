#include "Sanderbuilt_SARA.h"

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

Sanderbuilt_SARA::Sanderbuilt_SARA(int8_t rst)
{
	_rstpin = rst;
	mySerial = 0;
	ok_reply = F("OK");
	error_reply = F("+CME ERROR: Operation not supported");
	download_reply = F(">");
}

boolean Sanderbuilt_SARA::beginSerial(Sanderbuilt_SARA_StreamType &port) {
	mySerial = &port;
	return true;
}

/********* Main Loops ******************************************/

uint8_t Sanderbuilt_SARA::readloop(void) {
	uint16_t status;
	_readStatus = false;
	bool hasRecieved = false;
	unsigned long currentMillis = millis();

	while ((mySerial->available() && (!_readStatus)) || ((_replyCounter > 0) && (!_readStatus))) {
		hasRecieved = true;
		if (_replyCounter >= 500) {
			_replyCounter = 0;
			return SANDERBUILT_SARA_STRING_OVERRUN;
		}
		if (mySerial->available()) {
			char c = mySerial->read();

			if (c == '\r') {
				//DEBUG_PRINT(F("\t*R*"));
			}
			else if (c == 0xA) {
				if (_replyCounter == 0) { // the first 0x0A is ignored
										  //DEBUG_PRINTLN(F("IGNORED"));
				}
				else {
					_readStatus = true;
					DEBUG_PRINTLN(F(" "));
					//replybuffer[_replyCounter] = ',';
					//_replyCounter++;
					replybuffer[_replyCounter] = 0; // Null
					_replyCounter = 0;
				}
			}
			else {
				if (_replyCounter == 0) {
					DEBUG_PRINT(currentMillis);
					DEBUG_PRINT(F("\t<--- "));
					_replyTime = currentMillis;
				}

				if (_replyCounter == 0 && c == '>') {
					DEBUG_PRINTLN(c);
					return SANDERBUILT_SARA_DOWNLOAD;
				} else if (_replyCounter == 0 && c == '@') {
					DEBUG_PRINTLN(c);
					return SANDERBUILT_SARA_DOWNLOAD;
				}
				else {
					replybuffer[_replyCounter] = c;
					DEBUG_PRINT(c);
					_replyCounter++;
				}
			}
		}

		currentMillis = millis();
		if ((_replyCounter > 0) && ((currentMillis - _replyTime) > 1000)) {
			// just for testing
			DEBUG_PRINTLN(F(" "));
			DEBUG_PRINT(currentMillis);
			DEBUG_PRINT(F("\t<--> SARA REPLY COUNTER TIMEOUT "));
			DEBUG_PRINTLN(_replyCounter);
			_replyCounter = 0;
			return SANDERBUILT_SARA_REPLY_COUNTER_TIMEOUT;
		}
	}

	if (_readStatus) {
		if (prog_char_strcmp(replybuffer, (prog_char*)ok_reply) == 0) return SANDERBUILT_SARA_OK;

		uint16_t status;

		if (strcmp(replybuffer, "ERROR") == 0) return SANDERBUILT_SARA_ERROR;

		if (parseReply(F("+CME ERROR:"), &status, ',', 0)) {
			error = status;

			DEBUG_PRINT(currentMillis);
			DEBUG_PRINTLN(F("\t<-e- ERROR"));
			return SANDERBUILT_SARA_ERROR;
		}

		if (prog_char_strcmp(replybuffer, (prog_char*)error_reply) == 0) {
			error = 3;
			return SANDERBUILT_SARA_ERROR;
		}

		//if (prog_char_strcmp(replybuffer, (prog_char*)download_reply) == 0) return SANDERBUILT_SARA_DOWNLOAD;

		strcpy(unknownbuffer, replybuffer);
		return SANDERBUILT_SARA_REPLY_STRING;
	} else if ((_replyCounter > 0) && ((currentMillis - _replyTime) < 50)) {
		return SANDERBUILT_SARA_REPLY_IN_PROGRESS;
	} else if ((_replyCounter > 0) && ((currentMillis - _replyTime) > 1000)) {
		// just for testing
		DEBUG_PRINTLN(F(" "));
		DEBUG_PRINT(currentMillis);
		DEBUG_PRINT(F("\t<--> SARA REPLY COUNTER TIMEOUT "));
		DEBUG_PRINTLN(_replyCounter);
		_replyCounter = 0;
		return SANDERBUILT_SARA_REPLY_COUNTER_TIMEOUT;
	}
	return SANDERBUILT_SARA_NO_STRING;
}

uint16_t Sanderbuilt_SARA::parseError(void) {
	return error;
}

/********* SEND COMMANDS ***************************************/

bool Sanderbuilt_SARA::requestOK(void) {
	mySerial->println(F("AT"));
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN("AT");
	return true;
}

bool Sanderbuilt_SARA::requestECHO(void) {
	mySerial->println(F("ATE0"));
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN("ATE0");
	return true;
}

bool Sanderbuilt_SARA::setRadio(uint8_t selectedAct) {
	mySerial->print(F("AT+URAT=")); mySerial->println(selectedAct);
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT("AT+URAT="); DEBUG_PRINTLN(selectedAct);
	return true;
}

void Sanderbuilt_SARA::setCREG(uint8_t mode) {
	mySerial->print(F("AT+CREG=")); mySerial->println(mode);
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT("AT+CREG="); DEBUG_PRINTLN(mode);
}

void Sanderbuilt_SARA::setUCGED(uint8_t mode) {
	mySerial->print(F("AT+UCGED=")); mySerial->println(mode);
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT("AT+UCGED="); DEBUG_PRINTLN(mode);
}

void Sanderbuilt_SARA::requestDeviceVersion(void) {
	mySerial->println(F("ATI"));
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN("ATI");
}

void Sanderbuilt_SARA::requestFirmwareVersion(void) {
	mySerial->println(F("ATI9"));
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN("ATI9");
}

bool Sanderbuilt_SARA::requestSimStatus(void) {
	mySerial->println(F("AT+CPIN?"));
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN("AT+CPIN?");
	return true;
}

int8_t Sanderbuilt_SARA::parseSimStatus(char *buffer) {
	char status[16];
	if (sscanf(buffer, "+CPIN: %" STR(sizeof(status) - 1) "s", status) == 1) {
		if (startsWith("READY", status)) {
			return SANDERBUILT_SARA_SIM_READY;
		}
		else {
			return SANDERBUILT_SARA_SIM_NEEDS_PIN;
		}
	}
	return -1;
}

void Sanderbuilt_SARA::requestNetworkStatus(void) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN(F("AT+CREG?"));
	mySerial->println(F("AT+CREG?"));
}

void Sanderbuilt_SARA::requestNetworkQuality(void) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN(F("AT+CSQ"));
	mySerial->println(F("AT+CSQ"));
}

void Sanderbuilt_SARA::requestExtendedNetworkQuality(void) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN(F("AT+CESQ"));
	mySerial->println(F("AT+CESQ"));
}

void Sanderbuilt_SARA::requestRSRPvalues(void) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN(F("AT+VZWRSRP?"));
	mySerial->println(F("AT+VZWRSRP?"));
}

void Sanderbuilt_SARA::requestRSRQvalues(void) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN(F("AT+VZWRSRQ?"));
	mySerial->println(F("AT+VZWRSRQ?"));
}

void Sanderbuilt_SARA::requestUCGEDvalues(void) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN(F("AT+UCGED?"));
	mySerial->println(F("AT+UCGED?"));
}

void Sanderbuilt_SARA::requestUPSD(uint8_t profile_id) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(F("AT+UPSD=")); DEBUG_PRINTLN(profile_id);
	mySerial->print(F("AT+UPSD=")); mySerial->println(profile_id);
}

void Sanderbuilt_SARA::requestUPSD(uint8_t profile_id, uint8_t param_tag) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(F("AT+UPSD=")); DEBUG_PRINT(profile_id); DEBUG_PRINT(F(",")); DEBUG_PRINTLN(param_tag);
	mySerial->print(F("AT+UPSD=")); mySerial->print(profile_id);  mySerial->print(F(",")); mySerial->print(param_tag);
}

void Sanderbuilt_SARA::requestUPSD(uint8_t profile_id, uint8_t param_tag, char *param_value) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(F("AT+UPSD=")); DEBUG_PRINT(profile_id); DEBUG_PRINT(F(",")); DEBUG_PRINT(param_tag); DEBUG_PRINT(F(",")); DEBUG_PRINTLN(param_value);
	mySerial->print(F("AT+UPSD=")); mySerial->println(profile_id); mySerial->print(F(",")); mySerial->print(param_tag); mySerial->print(F(",")); mySerial->println(param_value);
}


void Sanderbuilt_SARA::requestReadCOPS(void) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN(F("AT+COPS?"));
	mySerial->println(F("AT+COPS?"));
}

void Sanderbuilt_SARA::requestCGDCONT(void) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN(F("AT+CGDCONT?"));
	mySerial->println(F("AT+CGDCONT?"));
}

void Sanderbuilt_SARA::requestTemperature(void) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN(F("AT+UTEMP?"));
	mySerial->println(F("AT+UTEMP?"));
}

void Sanderbuilt_SARA::requestFrequencyMap(void) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN(F("AT+UBANDMASK?"));
	mySerial->println(F("AT+UBANDMASK?"));
}

void Sanderbuilt_SARA::setFrequencyMap(uint8_t RAT, char* bitmask1) {
	mySerial->print(F("AT+UBANDMASK=")); mySerial->print(RAT); mySerial->print(F(",")); mySerial->println(bitmask1);
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT("AT+UBANDMASK="); DEBUG_PRINT(RAT); DEBUG_PRINT(","); DEBUG_PRINTLN(bitmask1);
}

void Sanderbuilt_SARA::setMNOProfile(uint8_t mode) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(F("AT+UMNOPROF=")); DEBUG_PRINTLN(mode);
	mySerial->print(F("AT+UMNOPROF=")); mySerial->println(mode);
}

void Sanderbuilt_SARA::requestMNOProfile(void) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN(F("AT+UMNOPROF?"));
	mySerial->println(F("AT+UMNOPROF?"));
}

void Sanderbuilt_SARA::setAPN(char *url) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(F("AT+CGDCONT=1,\"IP\",\"")); DEBUG_PRINT(url); DEBUG_PRINTLN(F("\""));
	mySerial->print(F("AT+CGDCONT=1,\"IP\",\"")); mySerial->print(url); mySerial->println(F("\""));
}

void Sanderbuilt_SARA::disableModem(void) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN(F("AT+CFUN=0"));
	mySerial->println(F("AT+CFUN=0"));
}

void Sanderbuilt_SARA::silentReset(void) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN(F("AT+CFUN=15"));
	mySerial->println(F("AT+CFUN=15"));
}

void Sanderbuilt_SARA::requestGPRSStatus(void) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN(F("AT+CGATT?"));
	mySerial->println(F("AT+CGATT?"));
}

void Sanderbuilt_SARA::requestSetGPRS(void) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN(F("AT+CGATT=1"));
	mySerial->println(F("AT+CGATT=1"));
}

void Sanderbuilt_SARA::requestShutdown(void) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN(F("AT+CPWROFF"));
	mySerial->println(F("AT+CPWROFF"));
}

void Sanderbuilt_SARA::parseReplyBuffer(char *serverResponse) {
	strcpy(serverResponse, unknownbuffer);
}

uint8_t Sanderbuilt_SARA::getIMEI(char *imei) {
	getReply(F("AT+GSN"));

	// up to 15 chars
	strncpy(imei, replybuffer, 15);
	imei[15] = 0;

	readline(); // eat 'OK'

	return strlen(imei);
}

void Sanderbuilt_SARA::requestSetCMEE(uint8_t parameter) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(F("AT+CMEE=")); DEBUG_PRINTLN(parameter);
	mySerial->print(F("AT+CMEE=")); mySerial->println(parameter);
}

void Sanderbuilt_SARA::requestCONTRDP(uint8_t profile_id) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(F("AT+CGCONTRDP=")); DEBUG_PRINTLN(profile_id);
	mySerial->print(F("AT+CGCONTRDP=")); mySerial->println(profile_id);
}

void Sanderbuilt_SARA::requestPhoneNumber(void) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN(F("AT+CNUM"));
	mySerial->println(F("AT+CNUM"));
}

/********* SOCKETS *********************************************/

void Sanderbuilt_SARA::createSocket(uint8_t protocol) {
	mySerial->print(F("AT+USOCR=")); mySerial->println(protocol);
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT("AT+USOCR="); DEBUG_PRINTLN(protocol);
}

void Sanderbuilt_SARA::setSocketOption(uint8_t socket, char *options) {
	mySerial->print(F("AT+USOGO=")); mySerial->print(socket); mySerial->print(F(",")); mySerial->println(options);
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT("AT+USOGO="); DEBUG_PRINT(socket); DEBUG_PRINT(","); DEBUG_PRINTLN(options);
}

void Sanderbuilt_SARA::closeSocket(uint8_t socket) {
	mySerial->print(F("AT+USOCL=")); mySerial->println(socket);
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT("AT+USOCL="); DEBUG_PRINTLN(socket);
}

void Sanderbuilt_SARA::connectSocket(uint8_t socket, char *remoteAddress, uint16_t port) {
	mySerial->print(F("AT+USOCO=")); mySerial->print(socket); mySerial->print(F(",\"")); mySerial->print(remoteAddress); mySerial->print(F("\",")); mySerial->println(port);
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT("AT+USOCO="); DEBUG_PRINT(socket); DEBUG_PRINT(",\""); DEBUG_PRINT(remoteAddress); DEBUG_PRINT("\","); DEBUG_PRINTLN(port);
}

void Sanderbuilt_SARA::writeSocketData(uint8_t socket, uint8_t length, char *data) {
	mySerial->print(F("AT+USOWR=")); mySerial->print(socket); mySerial->print(F(",")); mySerial->print(length); mySerial->print(F(",\"")); mySerial->print(data); mySerial->print(F("\""));
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT("AT+USOWR="); DEBUG_PRINT(socket); DEBUG_PRINT(","); DEBUG_PRINT(length); DEBUG_PRINT(",\""); DEBUG_PRINT(data); DEBUG_PRINTLN("\"");
}

void Sanderbuilt_SARA::writeSocketDataBytes(uint8_t socket, unsigned long length) {
	mySerial->print(F("AT+USOWR=")); mySerial->print(socket); mySerial->print(F(",")); mySerial->println(length);\
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT("AT+USOWR="); DEBUG_PRINT(socket); DEBUG_PRINT(","); DEBUG_PRINTLN(length);
}

void Sanderbuilt_SARA::readSocketData(uint8_t socket, unsigned long length) {
	mySerial->print(F("AT+USORD=")); mySerial->print(socket); mySerial->print(F(",")); mySerial->println(length);
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT("AT+USORD="); DEBUG_PRINT(socket); DEBUG_PRINT(","); DEBUG_PRINTLN(length);
}

void Sanderbuilt_SARA::socketControl(uint8_t socket, uint8_t paramID) {
	mySerial->print(F("AT+USOCTL=")); mySerial->print(socket); mySerial->print(F(",")); mySerial->println(paramID);
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT("AT+USOCTL="); DEBUG_PRINT(socket); DEBUG_PRINT(","); DEBUG_PRINTLN(paramID);
}

void Sanderbuilt_SARA::getSocketError(void) {
	mySerial->println(F("AT+USOER"));
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN("AT+USOER");
}

void Sanderbuilt_SARA::sendCustomCommand(char *text) {
	mySerial->println(F(text));
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN(text);
}

void Sanderbuilt_SARA::requestIP(char* url) {
	mySerial->print(F("AT+UDNSRN=0,\"")); mySerial->print(url); mySerial->println(F("\""));
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT("AT+UDNSRN=0,\""); DEBUG_PRINT(url); DEBUG_PRINTLN("\"");
}

/********* HTTP ************************************************/ 

void Sanderbuilt_SARA::setHTTPTimeout(uint8_t time) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(F("AT+UHTTP=0,7,")); DEBUG_PRINTLN(time);
	mySerial->print(F("AT+UHTTP=0,7,")); mySerial->println(time);
}

bool Sanderbuilt_SARA::setIP(uint8_t profile, char* url) {
	mySerial->print(F("AT+UHTTP=")); mySerial->print(profile); mySerial->print(F(",0,")); mySerial->print(F("\"")); mySerial->print(url); mySerial->println(F("\""));
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT("AT+UHTTP="); DEBUG_PRINT(profile); DEBUG_PRINT(",0,"); DEBUG_PRINT('"'); DEBUG_PRINT(url); DEBUG_PRINTLN('"');
	return true;
}

bool Sanderbuilt_SARA::setURL(uint8_t profile, char *url) {
	mySerial->print(F("AT+UHTTP=")); mySerial->print(profile); mySerial->print(F(",1,")); mySerial->print(F("\"")); mySerial->print(url); mySerial->println(F("\""));
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT("AT+UHTTP="); DEBUG_PRINT(profile); DEBUG_PRINT(",1,"); DEBUG_PRINT('"'); DEBUG_PRINT(url); DEBUG_PRINTLN('"');
	return true;
}

bool Sanderbuilt_SARA::setSSL(uint8_t profile, uint8_t secure) {
	mySerial->print(F("AT+UHTTP=")); mySerial->print(profile); mySerial->print(F(",6,")); mySerial->println(secure);
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT("AT+UHTTP="); DEBUG_PRINT(profile); DEBUG_PRINT(",6,");  DEBUG_PRINTLN(secure);
	return true;
}

bool Sanderbuilt_SARA::requestHTTPGET(uint8_t profile, char *path, char *filename) {
	mySerial->print(F("AT+UHTTPC=")); mySerial->print(profile); mySerial->print(F(",1,\"")); mySerial->print(path); mySerial->print(F("\",\"")); mySerial->print(filename); mySerial->println(F("\""));
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(F("AT+UHTTPC=")); DEBUG_PRINT(profile); DEBUG_PRINT(F(",1,\"")); DEBUG_PRINT(path); DEBUG_PRINT(F("\",\"")); DEBUG_PRINT(filename); DEBUG_PRINTLN(F("\""));
	return true;
}

/*
Content Type:
- 0: application/x-www-form-urlencoded
- 1: text/plain
- 2: application/octet-stream
- 3: multipart/form-data
- 4: application/json
- 5: application/xml
*/
bool Sanderbuilt_SARA::requestHTTPPOST(uint8_t profile, char *path, char *filename, char *postFilename, uint8_t contentType) {
	mySerial->print(F("AT+UHTTPC=")); mySerial->print(profile); mySerial->print(F(",4,\"")); mySerial->print(path); mySerial->print(F("\",\"")); mySerial->print(filename); mySerial->print(F("\",\""));
	mySerial->print(postFilename); mySerial->print(F("\",")); mySerial->println(contentType);
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(F("AT+UHTTPC=")); DEBUG_PRINT(profile); DEBUG_PRINT(F(",4,\"")); DEBUG_PRINT(path); DEBUG_PRINT(F("\",\"")); DEBUG_PRINT(filename); DEBUG_PRINT(F("\",\""));
	DEBUG_PRINT(postFilename); DEBUG_PRINT(F("\",")); DEBUG_PRINTLN(contentType);
	return true;
}

void Sanderbuilt_SARA::requestHTTPError(uint8_t profile_id) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(F("AT+UHTTPER=")); DEBUG_PRINTLN(profile_id);
	mySerial->print(F("AT+UHTTPER=")); mySerial->println(profile_id);
}

/********* HTTP ************************************************/

void Sanderbuilt_SARA::setFTPIP(char* ip) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(F("AT+UFTP=0,\"")); DEBUG_PRINT(ip); DEBUG_PRINTLN(F("\""));
	mySerial->print(F("AT+UFTP=0,\"")); mySerial->print(ip); mySerial->println(F("\""));
}

void Sanderbuilt_SARA::setFTPURL(char* url) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(F("AT+UFTP=1,\"")); DEBUG_PRINT(url); DEBUG_PRINTLN(F("\""));
	mySerial->print(F("AT+UFTP=1,\"")); mySerial->print(url); mySerial->println(F("\""));
}

void Sanderbuilt_SARA::setFTPUser(char* user) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(F("AT+UFTP=2,\"")); DEBUG_PRINT(user); DEBUG_PRINTLN(F("\""));
	mySerial->print(F("AT+UFTP=2,\"")); mySerial->print(user); mySerial->println(F("\""));
}

void Sanderbuilt_SARA::setFTPPassword(char* password) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(F("AT+UFTP=3,\"")); DEBUG_PRINT(password); DEBUG_PRINTLN(F("\""));
	mySerial->print(F("AT+UFTP=3,\"")); mySerial->print(password); mySerial->println(F("\""));
}

void Sanderbuilt_SARA::setFTPAccount(char* account) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(F("AT+UFTP=4,\"")); DEBUG_PRINT(account); DEBUG_PRINTLN(F("\""));
	mySerial->print(F("AT+UFTP=4,\"")); mySerial->print(account); mySerial->println(F("\""));
}

void Sanderbuilt_SARA::setFTPMode(uint8_t mode) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(F("AT+UFTP=6,")); DEBUG_PRINTLN(mode);
	mySerial->print(F("AT+UFTP=6,")); mySerial->println(mode);
}

void Sanderbuilt_SARA::setFTPPort(uint16_t port) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(F("AT+UFTP=7,")); DEBUG_PRINTLN(port);
	mySerial->print(F("AT+UFTP=7,")); mySerial->println(port);
}

void Sanderbuilt_SARA::ftpLogout(void) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN(F("AT+UFTPC=0"));
	mySerial->println(F("AT+UFTPC=0")); 
}

void Sanderbuilt_SARA::ftpLogin(void) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN(F("AT+UFTPC=1"));
	mySerial->println(F("AT+UFTPC=1"));
}

void Sanderbuilt_SARA::ftpDeleteFile(char* fileName) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(F("AT+UFTPC=2,\"")); DEBUG_PRINT(fileName); DEBUG_PRINTLN(F("\""));
	mySerial->print(F("AT+UFTPC=2,\"")); mySerial->print(fileName); mySerial->println(F("\""));
}

void Sanderbuilt_SARA::ftpRetrieveFile(char* fileName, char* localFileName) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(F("AT+UFTPC=5,\"")); DEBUG_PRINT(fileName); DEBUG_PRINT(F("\",\"")); DEBUG_PRINT(localFileName); DEBUG_PRINTLN(F("\""));
	mySerial->print(F("AT+UFTPC=5,\"")); mySerial->print(fileName); mySerial->print(F("\",\"")); mySerial->print(localFileName); mySerial->println(F("\""));
}

void Sanderbuilt_SARA::ftpRetrieveFOTA(char* fileName) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(F("AT+UFTPC=100,\"")); DEBUG_PRINT(fileName); DEBUG_PRINTLN(F("\""));
	mySerial->print(F("AT+UFTPC=100,\"")); mySerial->print(fileName); mySerial->println(F("\""));
}

void Sanderbuilt_SARA::ftpRetrieveFileList(void) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN(F("AT+UFTPC=14"));
	mySerial->println(F("AT+UFTPC=14"));
}

void Sanderbuilt_SARA::ftpRetrieveFileList(char* fileName) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(F("AT+UFTPC=14,\"")); DEBUG_PRINT(fileName); DEBUG_PRINTLN(F("\""));
	mySerial->print(F("AT+UFTPC=14,\"")); mySerial->print(fileName); mySerial->println(F("\""));
}

/********* FOTA ************************************************/

void Sanderbuilt_SARA::FOTAInstall(void) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN(F("AT+UFWINSTALL"));
	mySerial->println(F("AT+UFWINSTALL"));
}

void Sanderbuilt_SARA::FOTAInstallCheck(void) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN(F("AT+UFWSTATUS?"));
	mySerial->println(F("AT+UFWSTATUS?"));
}

/********* FILE SYSTEM *****************************************/

/*
Push file of 'size' to the module. Wait to recieve '>' from module to start
*/
void Sanderbuilt_SARA::requestPushFile(char *filename, uint16_t size) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(F("AT+UDWNFILE=\"")); DEBUG_PRINT(filename); DEBUG_PRINT(F("\","));  DEBUG_PRINTLN(size);
	mySerial->print(F("AT+UDWNFILE=\"")); mySerial->print(filename); mySerial->print(F("\",")); mySerial->println(size);
}

void Sanderbuilt_SARA::requestPushFile(char *filename, uint16_t size, char *tag) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(F("AT+UDWNFILE=\"")); DEBUG_PRINT(filename); DEBUG_PRINT(F("\","));  DEBUG_PRINT(size); DEBUG_PRINT(F(",\""));  DEBUG_PRINT(tag); DEBUG_PRINTLN(F("\""));
	mySerial->print(F("AT+UDWNFILE=\"")); mySerial->print(filename); mySerial->print(F("\",")); mySerial->print(size); mySerial->print(F(",\"")); mySerial->print(tag);  mySerial->println(F("\""));
}

void Sanderbuilt_SARA::requestPushPartialFile(char *filename, uint16_t offset, uint16_t size) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(F("AT+UDWNFILE=\"")); DEBUG_PRINT(filename); DEBUG_PRINT(F("\",")); DEBUG_PRINT(offset); DEBUG_PRINT(F(",")); DEBUG_PRINTLN(size);
	mySerial->print(F("AT+UDWNFILE=\"")); mySerial->print(filename); mySerial->print(F("\",")); mySerial->println(size);
}

void Sanderbuilt_SARA::requestPushPartialFile(char *filename, uint16_t offset, uint16_t size, char *tag) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(F("AT+UDWNFILE=\"")); DEBUG_PRINT(filename); DEBUG_PRINT(F("\",")); DEBUG_PRINT(offset); DEBUG_PRINT(F(",")); DEBUG_PRINT(size); DEBUG_PRINT(F(",\""));  DEBUG_PRINT(tag); DEBUG_PRINTLN(F("\""));
	mySerial->print(F("AT+UDWNFILE=\"")); mySerial->print(filename); mySerial->print(F("\",")); mySerial->print(size); mySerial->print(F(",\"")); mySerial->print(tag);  mySerial->println(F("\""));
}

void Sanderbuilt_SARA::requestReadFile(char *filename) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(F("AT+URDFILE=\"")); DEBUG_PRINT(filename); DEBUG_PRINTLN(F("\""));
	mySerial->print(F("AT+URDFILE=\"")); mySerial->print(filename); mySerial->println(F("\""));
}

void Sanderbuilt_SARA::requestReadPartialFile(char *filename, uint16_t offset, uint16_t size) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(F("AT+URDFILE=\"")); DEBUG_PRINT(filename); DEBUG_PRINT(F("\",")); DEBUG_PRINT(offset); DEBUG_PRINT(F(",")); DEBUG_PRINTLN(size);
	mySerial->print(F("AT+URDBLOCK=\"")); mySerial->print(filename); mySerial->print(F("\",")); mySerial->print(offset); mySerial->print(F(",")); mySerial->println(size);
}

void Sanderbuilt_SARA::requestDeleteFile(char *filename) {
	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(F("AT+UDELFILE=\"")); DEBUG_PRINT(filename); DEBUG_PRINTLN(F("\""));
	mySerial->print(F("AT+UDELFILE=\"")); mySerial->print(filename); mySerial->println(F("\""));
}

/********* LOW LEVEL *******************************************/

inline int Sanderbuilt_SARA::available(void) {
	return mySerial->available();
}

inline size_t Sanderbuilt_SARA::write(uint8_t x) {
	return mySerial->write(x);
}

inline int Sanderbuilt_SARA::read(void) {
	return mySerial->read();
}

inline int Sanderbuilt_SARA::peek(void) {
	return mySerial->peek();
}

inline void Sanderbuilt_SARA::flush() {
	mySerial->flush();
}

void Sanderbuilt_SARA::flushInput() {
	// Read all available serial input to flush pending data.
	uint16_t timeoutloop = 0;
	while (timeoutloop++ < 40) {
		while (available()) {
			read();
			timeoutloop = 0;  // If char was received reset the timer
		}
		delay(1);
	}
	_readStatus = false;
	_replyCounter = 0;
}

bool Sanderbuilt_SARA::startsWith(const char* pre, const char* str)
{
	return (strncmp(pre, str, strlen(pre)) == 0);
}

uint8_t Sanderbuilt_SARA::readline(uint16_t timeout, boolean multiline) {
	uint16_t replyidx = 0;

	while (timeout--) {
		if (replyidx >= 254) {
			//DEBUG_PRINTLN(F("SPACE"));
			break;
		}

		while (mySerial->available()) {
			char c = mySerial->read();
			if (c == '\r') continue;
			if (c == 0xA) {
				if (replyidx == 0)   // the first 0x0A is ignored
					continue;

				if (!multiline) {
					timeout = 0;         // the second 0x0A is the end of the line
					break;
				}
			}
			replybuffer[replyidx] = c;
			//DEBUG_PRINT(c, HEX); DEBUG_PRINT("#"); DEBUG_PRINTLN(c);
			replyidx++;
		}

		if (timeout == 0) {
			//DEBUG_PRINTLN(F("TIMEOUT"));
			break;
		}
		delay(1);
	}
	replybuffer[replyidx] = 0;  // null term
	return replyidx;
}

uint8_t Sanderbuilt_SARA::getReply(char *send, uint16_t timeout) {
	flushInput();


	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN(send);


	mySerial->println(send);

	uint8_t l = readline(timeout);

	DEBUG_PRINT(F("\t<--- ")); DEBUG_PRINTLN(replybuffer);

	return l;
}

uint8_t Sanderbuilt_SARA::getReply(Sanderbuilt_SARA_FlashStringPtr send, uint16_t timeout) {
	flushInput();


	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN(send);


	mySerial->println(send);

	uint8_t l = readline(timeout);

	DEBUG_PRINT(F("\t<--- ")); DEBUG_PRINTLN(replybuffer);

	return l;
}

// Send prefix, suffix, and newline. Return response (and also set replybuffer with response).
uint8_t Sanderbuilt_SARA::getReply(Sanderbuilt_SARA_FlashStringPtr prefix, char *suffix, uint16_t timeout) {
	flushInput();


	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(prefix); DEBUG_PRINTLN(suffix);


	mySerial->print(prefix);
	mySerial->println(suffix);

	uint8_t l = readline(timeout);

	DEBUG_PRINT(F("\t<--- ")); DEBUG_PRINTLN(replybuffer);

	return l;
}

// Send prefix, suffix, and newline. Return response (and also set replybuffer with response).
uint8_t Sanderbuilt_SARA::getReply(Sanderbuilt_SARA_FlashStringPtr prefix, int32_t suffix, uint16_t timeout) {
	flushInput();


	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(prefix); DEBUG_PRINTLN(suffix, DEC);


	mySerial->print(prefix);
	mySerial->println(suffix, DEC);

	uint8_t l = readline(timeout);

	DEBUG_PRINT(F("\t<--- ")); DEBUG_PRINTLN(replybuffer);

	return l;
}

// Send prefix, suffix, suffix2, and newline. Return response (and also set replybuffer with response).
uint8_t Sanderbuilt_SARA::getReply(Sanderbuilt_SARA_FlashStringPtr prefix, int32_t suffix1, int32_t suffix2, uint16_t timeout) {
	flushInput();


	DEBUG_PRINT(millis()); DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(prefix);
	DEBUG_PRINT(suffix1, DEC); DEBUG_PRINT(','); DEBUG_PRINTLN(suffix2, DEC);


	mySerial->print(prefix);
	mySerial->print(suffix1);
	mySerial->print(',');
	mySerial->println(suffix2, DEC);

	uint8_t l = readline(timeout);

	DEBUG_PRINT(F("\t<--- ")); DEBUG_PRINTLN(replybuffer);

	return l;
}

boolean Sanderbuilt_SARA::parseReply(Sanderbuilt_SARA_FlashStringPtr toreply, uint16_t *v, char divider, uint8_t index) {
	char *p = prog_char_strstr(replybuffer, (prog_char*)toreply);  // get the pointer to the voltage
	if (p == 0) return false;
	p += prog_char_strlen((prog_char*)toreply);
	//DEBUG_PRINTLN(p);
	for (uint8_t i = 0; i<index; i++) {
		// increment dividers
		p = strchr(p, divider);
		if (!p) return false;
		p++;
		//DEBUG_PRINTLN(p);

	}
	*v = atoi(p);

	return true;
}

boolean Sanderbuilt_SARA::parseReply(Sanderbuilt_SARA_FlashStringPtr toreply, char *v, char divider, uint8_t index) {
	uint8_t i = 0;
	char *p = prog_char_strstr(replybuffer, (prog_char*)toreply);
	if (p == 0) return false;
	p += prog_char_strlen((prog_char*)toreply);

	for (i = 0; i<index; i++) {
		// increment dividers
		p = strchr(p, divider);
		if (!p) return false;
		p++;
	}

	for (i = 0; i<strlen(p); i++) {
		if (p[i] == divider)
			break;
		v[i] = p[i];
	}

	v[i] = '\0';

	return true;
}

// Parse a quoted string in the response fields and copy its value (without quotes)
// to the specified character array (v).  Only up to maxlen characters are copied
// into the result buffer, so make sure to pass a large enough buffer to handle the
// response.
boolean Sanderbuilt_SARA::parseReplyQuoted(Sanderbuilt_SARA_FlashStringPtr toreply, char *v, int maxlen, char divider, uint8_t index) {
	uint8_t i = 0, j;
	// Verify response starts with toreply.
	char *p = prog_char_strstr(replybuffer, (prog_char*)toreply);
	if (p == 0) return false;
	p += prog_char_strlen((prog_char*)toreply);

	// Find location of desired response field.
	for (i = 0; i<index; i++) {
		// increment dividers
		p = strchr(p, divider);
		if (!p) return false;
		p++;
	}

	// Copy characters from response field into result string.
	for (i = 0, j = 0; j<maxlen && i<strlen(p); ++i) {
		// Stop if a divier is found.
		if (p[i] == divider)
			break;
		// Skip any quotation marks.
		else if (p[i] == '"')
			continue;
		v[j++] = p[i];
	}

	// Add a null terminator if result string buffer was not filled.
	if (j < maxlen)
		v[j] = '\0';

	return true;
}