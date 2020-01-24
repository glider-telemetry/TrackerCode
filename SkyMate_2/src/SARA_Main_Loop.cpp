// 
// 
// 

#include "SkyMate.h"
#include <Sanderbuilt_SARA.h>

bool SkyMate::setMainLoop(uint8_t loopID) {
	if (mainLoopID == NETWORK_IDLE && httpReadID < HTTP_READ_REQUEST && socketReadID < SOCKET_READ_REQUEST) {
		mainLoopID = loopID;
		return true;
	} else {
		return false;
	}
}

void SkyMate::battVoltage(float voltage) {
	_voltage = voltage;
}

uint8_t SkyMate::mainLoop(unsigned long currentMillis) {
	uint8_t readLoopStatus = myTracker->readloop();
	uint8_t intReadLoop = 0;
	uint16_t status;
	long lstatus;

	// Check for unscolicited messages.
	if (readLoopStatus == SANDERBUILT_SARA_REPLY_STRING) {
		myTracker->parseReplyBuffer(replybuffer);
		// Error
		if (parseString("+UUHTTPCR:", &status, ',', 2)) {
			if (status == 1) {
				httpReadID = HTTP_READ_REQUEST;
			} else {
				httpReadID = HTTP_POST_ERROR;
			}
		}

		if (parseString("+UUSORD:", &status, ',', 1)) {
			socketReadChars = status;
			socketReadID = SOCKET_READ_REQUEST;
			socketReadStatus = SOCKET_COM_START;
		}

		if (parseString("+UUSOCL:", &status, ',', 0)) {
			socket = 99;
		}
	}

	if (httpReadID == HTTP_READ_WAIT) {
		if ((currentMillis - transmissionTime) > HTTP_RECEIVE_TIMEOUT) {
			//httpReadID = HTTP_READ_IDLE;
			transmissionTime = currentMillis;
			httpReadID = HTTP_READ_REQUEST;
			httpReadStatus = HTTP_COM_START;
		}
	}

	if (socketReadID == SOCKET_READ_WAIT) {
		if ((currentMillis - transmissionTime) > SOCKET_RECEIVE_TIMEOUT) {
			socketReadID = SOCKET_READ_ERROR;
			socketReadStatus = SOCKET_COM_START;
		}
	}

	if ((currentMillis - socketReadPrevMillis) > SOCKET_HARD_TIMEOUT) {
		initLoopID = INIT_RESET_ON;
		socketReadPrevMillis = currentMillis;
	}

	if (httpReadID > 1 && mainLoopID == NETWORK_IDLE) {
		switch (httpReadID) {
		case HTTP_READ_REQUEST:
			if (readLoopStatus == SANDERBUILT_SARA_OK) {
				// OK is the end of the http read string.
				// Can use this space to handle the data/
				httpReadID = HTTP_DELETE_FILE;
				httpReadStatus = HTTP_COM_START;
				// 
				clearPositionStatus(SEND_STATUS_DOWNLOADED);
			} else {
				if (httpReadStatus == HTTP_COM_START) {
					if (readLoopStatus != SANDERBUILT_SARA_REPLY_IN_PROGRESS) {
						myTracker->requestReadPartialFile("HTTP",0,300);
						httpReadPrevMillis = currentMillis;
						httpReadStatus++;
					}
				} else {
					if ((currentMillis - httpReadPrevMillis) > HTTP_READ_COMMAND_TIMEOUT) {
						// handle a timeout- for now we will just try again
						httpReadID = HTTP_POST_ERROR;
						httpReadStatus = HTTP_COM_START;
					} else {
						if (readLoopStatus == SANDERBUILT_SARA_REPLY_STRING) { // if a read string is available
							//previousTransmissionTime = currentMillis - transmissionTime;
							readServerHTTPResponse();
							httpReadPrevMillis = currentMillis;
							httpReadStatus++;
						}
					}
				}
			}
			break;
		case HTTP_DELETE_FILE:
			if (httpReadStatus == HTTP_COM_START) {
				if (readLoopStatus != SANDERBUILT_SARA_REPLY_IN_PROGRESS) {
					myTracker->requestDeleteFile("HTTP");
					httpReadPrevMillis = currentMillis;
					httpReadStatus++;
				}
			} else {
				if ((currentMillis - httpReadPrevMillis) > HTTP_READ_COMMAND_TIMEOUT) {
					// handle a timeout- for now we will just try again
					httpReadStatus = HTTP_COM_START;
				} else {
					if (readLoopStatus == SANDERBUILT_SARA_OK) {
						httpReadID = HTTP_READ_IDLE;
						httpReadStatus = HTTP_COM_START;
					}
					else if (readLoopStatus == SANDERBUILT_SARA_ERROR) {
						// No file. May as well leave it alone.
						httpReadID = HTTP_READ_IDLE;
						httpReadStatus = HTTP_COM_START;
					}
				}
			}
			break;
		case HTTP_POST_ERROR:
			DEBUG_PRINTLN(F("POST ERROR"));
			clearPositionStatus(SEND_STATUS_DOWNLOADED);
			httpReadID = HTTP_READ_IDLE;
			httpReadStatus = HTTP_COM_START;
			break;
		default:
			break;
		}
	}
	else if (socketReadID > 1 && mainLoopID == NETWORK_IDLE) {
		switch (socketReadID) {
		case SOCKET_READ_REQUEST:
			if (readLoopStatus == SANDERBUILT_SARA_OK) {
				// OK is the end of the Socket read string.
				// Can use this space to handle the data/
				socketReadID = SOCKET_READ_IDLE;
				socketReadStatus = SOCKET_COM_START;
				// 
				clearPositionStatus(SEND_STATUS_DOWNLOADED);
			}
			else {
				if (socketReadStatus == SOCKET_COM_START) {
					if (readLoopStatus != SANDERBUILT_SARA_REPLY_IN_PROGRESS) {
						myTracker->readSocketData(socket, socketReadChars);
						socketReadPrevMillis = currentMillis;
						socketReadStatus++;
					}
				}
				else {
					if ((currentMillis - socketReadPrevMillis) > SOCKET_READ_COMMAND_TIMEOUT) {
						// handle a timeout- for now we will just try again
						socketReadID = SOCKET_READ_ERROR;
						socketReadStatus = SOCKET_COM_START;
					}
					else {
						if (readLoopStatus == SANDERBUILT_SARA_REPLY_STRING) { // if a read string is available
							readServerSocketResponse();
							socketReadPrevMillis = currentMillis;
							socketReadStatus++;
						}
					}
				}
			}
			break;
		case SOCKET_READ_ERROR:
			DEBUG_PRINTLN(F("SOCKET READ ERROR"));
			clearPositionStatus(SEND_STATUS_DOWNLOADED);
			socketReadID = SOCKET_READ_IDLE;
			socketReadStatus = SOCKET_COM_START;
			break;
		default:
			break;
		}
	}
	else if (mainLoopID != NETWORK_IDLE) {
		switch (mainLoopID) {
		case NETWORK_CHECK_NETWORK_STATUS:
			if (readLoopStatus == SANDERBUILT_SARA_REPLY_STRING) {
				if (parseString("+CREG: ", &status, ',', 1)) {
					intReadLoop = MAIN_LOOP_NETWORK_STATUS_AVAILABLE;
					netStatus = status;
					writeNetStatus(status);
				}
			}
			if (sendMainCommand(readLoopStatus, currentMillis, SANDERBUILT_SARA_OK, (netStatus != 99) ? CHECK_DIE_TEMPERATURE : NETWORK_IDLE)) {
				myTracker->requestNetworkStatus();
			}
			break;
		case CHECK_DIE_TEMPERATURE:
			if (readLoopStatus == SANDERBUILT_SARA_REPLY_STRING) {
				if (parseString("+UTEMP: ", &status, ',', 0)) {
					dieTemperature = status;
				}
			}
			if (sendMainCommand(readLoopStatus, currentMillis, SANDERBUILT_SARA_OK, NETWORK_CHECK_REQUEST_QUALITY,500,NETWORK_CHECK_REQUEST_QUALITY)) {
				myTracker->requestTemperature();
			}
			break;
		case NETWORK_CHECK_REQUEST_QUALITY:
			if (readLoopStatus == SANDERBUILT_SARA_REPLY_STRING) {
				if (parseString("+CSQ:", &status, ',', 0)) {
					signalPower = status;
					writeRSSI(status);
					intReadLoop = MAIN_LOOP_SIGNAL_QUALITY_AVAILABLE;
				}
			}
			if (sendMainCommand(intReadLoop, currentMillis, MAIN_LOOP_SIGNAL_QUALITY_AVAILABLE, NETWORK_CHECK_REQUEST_EXTENDED_QUALITY)) {
				myTracker->requestNetworkQuality();
			}
			break; 
		case NETWORK_CHECK_REQUEST_EXTENDED_QUALITY:
			if (readLoopStatus == SANDERBUILT_SARA_REPLY_STRING) {
				if (parseString("+CESQ:", &status, ',', 4)) {
					writeRSRQ(status);
					if (parseString("+CESQ:", &status, ',', 5)) {
						writeRSRP(status);
					}
				}
			}
			if ((netStatus == 1 || netStatus == 5) && signalPower != 99) {
				if (sendMainCommand(readLoopStatus, currentMillis, SANDERBUILT_SARA_OK, NETWORK_CHECK_UCGED)) {
					myTracker->requestExtendedNetworkQuality();
				}
			}
			else {
				if (sendMainCommand(readLoopStatus, currentMillis, SANDERBUILT_SARA_OK, NETWORK_IDLE)) {
					myTracker->requestExtendedNetworkQuality();
				}
			}
			
			break;
		case NETWORK_CHECK_UCGED:
			if (readLoopStatus == SANDERBUILT_SARA_REPLY_STRING) {
				parseReplyQuoted("+RSRP:", rsrpCH, 10, ',', 2);
				parseReplyQuoted("+RSRQ:", rsrqCH, 10, ',', 2);
			}
			//+RSRP:
			if (sendMainCommand(readLoopStatus, currentMillis, SANDERBUILT_SARA_OK, NETWORK_DISPLAY_SIGNAL)) {
				myTracker->requestUCGEDvalues();
			}
			break;
		case NETWORK_DISPLAY_SIGNAL:
			if (netStatus == 1 || netStatus == 5) {
				if (signalPower != 99) {
					mainLoopID = NETWORK_READY;
				}
				else {
					mainLoopID = NETWORK_IDLE;
				}
			}
			else {
				mainLoopID = NETWORK_IDLE;
			}
			break;
		case NETWORK_READY:
			if (socketReadID == SOCKET_READ_IDLE && httpReadID == HTTP_READ_IDLE) {
				if (firstSend) {
					mainLoopID = ATTACH_GPRS;
				}
				else if (getPostionStorePending() > 60) {
					// if the backlog is greater than 60 positions;
					mainLoopID = HTTP_MODE_POSITION_SET_URL;
				}
				else {
					mainLoopID = ATTACH_GPRS;
				}
			}
			else {
				mainLoopID = NETWORK_IDLE;
			}
			break;
		case HTTP_MODE_POSITION_SET_URL:
			if (sendMainCommand(readLoopStatus, currentMillis, SANDERBUILT_SARA_OK, HTTP_MODE_POSITION_DELETE_POST_FILE, 10000, NETWORK_IDLE)) {
				myTracker->setURL(0, "t.sanderbuilt.com");
			}
			break;
		case HTTP_MODE_POSITION_DELETE_POST_FILE:
			if (readLoopStatus == SANDERBUILT_SARA_ERROR) {
				mainLoopID = HTTP_MODE_SEND_POSITION;
				mainLoopStatus = MAIN_COM_START;
			} else if (sendMainCommand(readLoopStatus, currentMillis, SANDERBUILT_SARA_OK, HTTP_MODE_SEND_POSITION, 500, NETWORK_IDLE)) {
				myTracker->requestDeleteFile("MESS");
			}
			break;
		case HTTP_MODE_SEND_POSITION:
			if (mainLoopStatus == MAIN_COM_START) {
				if (readLoopStatus != SANDERBUILT_SARA_REPLY_IN_PROGRESS) { // Ensure nothing is being recieved
					mainPrevMillis = currentMillis;
					mainLoopStatus++;
					unsigned long messageLength = calculateMessageLength(false,HTTP_MAXIMUM_SEND);
					if (messageLength > 0) {
						myTracker->requestPushFile("MESS", messageLength);
					}
					else {
						mainLoopStatus = MAIN_COM_START;
						mainLoopID = NETWORK_IDLE;
					}
				}
			}
			else if (mainLoopStatus == MAIN_COM_STAGE_1) {
				if (readLoopStatus == SANDERBUILT_SARA_DOWNLOAD) {
					mainLoopStatus++;
					sendMessage(HTTP_MAXIMUM_SEND);
				}
				else if (readLoopStatus == SANDERBUILT_SARA_ERROR || readLoopStatus == SANDERBUILT_SARA_REPLY_COUNTER_TIMEOUT) {
					clearPositionStatus(SEND_STATUS_PENDING);
					mainLoopStatus = MAIN_COM_START;
					mainLoopID = NETWORK_IDLE;
				}
				else if ((currentMillis - mainPrevMillis) > MAIN_COMMAND_DOWNLOAD_TIMEOUT) { // Handle Timeout
					clearPositionStatus(SEND_STATUS_PENDING);
					mainLoopStatus = MAIN_COM_START;
					mainLoopID = NETWORK_IDLE;
				}
			}
			else {
				if (readLoopStatus == SANDERBUILT_SARA_OK) {
					mainLoopStatus = MAIN_COM_START;
					mainLoopID = HTTP_MODE_POSITION_POST;
				}
				else if (readLoopStatus == SANDERBUILT_SARA_ERROR || readLoopStatus == SANDERBUILT_SARA_REPLY_COUNTER_TIMEOUT) {
					clearPositionStatus(SEND_STATUS_DOWNLOADED);
					mainLoopStatus = MAIN_COM_START;
					mainLoopID = NETWORK_IDLE;
				}
				else if ((currentMillis - mainPrevMillis) > MAIN_COMMAND_DOWNLOAD_TIMEOUT) { // Handle Timeout
					clearPositionStatus(SEND_STATUS_DOWNLOADED);
					mainLoopStatus = MAIN_COM_START;
					mainLoopID = NETWORK_IDLE;
				}
				else {
					modemStream->print(" ");// Write spaces to the modem in an attempt to fill the download buffer if for whatever reason the correct number of characters were not written (Prevents a long timeout).
				}
			}
			break;
		case HTTP_MODE_SET_SSL:
			if (sendMainCommand(readLoopStatus, currentMillis, SANDERBUILT_SARA_OK, HTTP_MODE_REQUEST_GET)) {
				myTracker->setSSL(1, 1);
			}
			break;
		case HTTP_MODE_REQUEST_GET:
			if (sendMainCommand(readLoopStatus, currentMillis, SANDERBUILT_SARA_OK, HTTP_MODE_SERVER_STATUS_COMPLETE)) {
				createTrackerIDURL();
				myTracker->requestHTTPGET(0, path, "HTTP");
			}
			break;
		case HTTP_MODE_POSITION_POST:
			if (sendMainCommand(readLoopStatus, currentMillis, SANDERBUILT_SARA_OK, HTTP_MODE_SERVER_STATUS_COMPLETE)) {
				myTracker->requestHTTPPOST(0, "/post", "HTTP", "MESS", 4);
			}
			break;
		case HTTP_MODE_SERVER_STATUS_COMPLETE:
			transmissionTime = currentMillis;
			httpReadID = HTTP_READ_WAIT;
			mainLoopID = NETWORK_IDLE; 
		case ATTACH_GPRS:
			if (socket == 99) {
				if (sendMainCommand(readLoopStatus, currentMillis, SANDERBUILT_SARA_OK, (ipSet == true) ? SOCKET_CREATE_SOCKET : SOCKET_GET_IP)) {
					myTracker->requestSetGPRS();
				}
			}
			else {
				mainLoopID = SOCKET_CHECK_SOCKET;
			}
			break;
		case SOCKET_GET_IP:
			if (readLoopStatus == SANDERBUILT_SARA_REPLY_STRING) {
				char tempIP[20];
				if (parseReplyQuoted("+UDNSRN: ", tempIP, 20, ',', 0)) {
					strcpy(ipAddress, tempIP);
					ipSet = true;
				}
			}
			if (sendMainCommand(readLoopStatus, currentMillis, SANDERBUILT_SARA_OK, SOCKET_CREATE_SOCKET, 70000, SOCKET_DEFAULT_IP)) {
				myTracker->requestIP("t.sanderbuilt.com");
			}
			break;
		case SOCKET_DEFAULT_IP:
			strcpy(ipAddress, "13.211.90.105");
			ipSet = true;
			mainLoopID = SOCKET_CREATE_SOCKET;
			break;
		case SOCKET_CREATE_SOCKET:
			if (socket == 99) {
				if (readLoopStatus == SANDERBUILT_SARA_REPLY_STRING) {
					if (parseString("+USOCR: ", &status, ',', 0)) {
						socket = status;
					}
				}
				if (sendMainCommand(readLoopStatus, currentMillis, SANDERBUILT_SARA_OK, SOCKET_CHECK_SOCKET, 10000)) {
					myTracker->createSocket(6);
				}
			}
			else {
				mainLoopID = SOCKET_CHECK_SOCKET;
				mainLoopStatus = MAIN_COM_START;
			}
			break;
		case SOCKET_CHECK_SOCKET:
			if (readLoopStatus == SANDERBUILT_SARA_REPLY_STRING) {
				if (parseString("+USOCTL:", &status, ',', 2)) {
					if (status > 0 && status < 5) {
						mainLoopID = SOCKET_WRITE_DATA_BYTES1; //SOCKET_WRITE_DATA;
					}
					else {
						mainLoopID = SOCKET_CONNECT_SOCKET;
					}
					mainLoopStatus = MAIN_COM_START;
					break;
				}
			}
			if (sendMainCommand(readLoopStatus, currentMillis, SANDERBUILT_SARA_OK, SOCKET_CONNECT_SOCKET, 500, SOCKET_CLEAR_SOCKET)) {
				myTracker->socketControl(socket, 10);
			}
			break;
		case SOCKET_CONNECT_SOCKET:
			if (sendMainCommand(readLoopStatus, currentMillis, SANDERBUILT_SARA_OK, SOCKET_WRITE_DATA_BYTES1, 120000, SOCKET_GET_ERROR)) {
				myTracker->connectSocket(socket, ipAddress, 1234);
				//myTracker->connectSocket(socket, "13.211.90.105", 1234);
			}
			break;
		case SOCKET_WRITE_DATA_BYTES1:
			if (sendMainCommand(readLoopStatus, currentMillis, SANDERBUILT_SARA_DOWNLOAD, SOCKET_WRITE_DATA_BYTES2, 120000, SOCKET_GET_ERROR)) {
				myTracker->writeSocketDataBytes(socket, calculateMessageLength(firstSend));
			}
			break;
		case SOCKET_WRITE_DATA_BYTES2:
			if (sendMainCommand(readLoopStatus, currentMillis, SANDERBUILT_SARA_OK, SOCKET_LOOP_SUCCESS, 120000, SOCKET_GET_ERROR)) {
				sendMessage();
			}
			break;
		case SOCKET_CLEAR_SOCKET:
			socket = 99;
			ipSet = false;
		case SOCKET_GET_ERROR:
			if (sendMainCommand(readLoopStatus, currentMillis, SANDERBUILT_SARA_OK, SOCKET_LOOP_COMPLETE)) {
				clearPositionStatus(SEND_STATUS_PENDING);
				clearPositionStatus(SEND_STATUS_DOWNLOADED);
				if (socketErrorCounter >= 5) {
					initLoopID = INIT_RESET_ON;
					socketReadPrevMillis = currentMillis;
				}
				else {
					myTracker->getSocketError();
				}
				socketErrorCounter++;
			}
			break;
		case SOCKET_LOOP_SUCCESS:
			socketErrorCounter = 0;
			socketReadID = SOCKET_READ_WAIT;
		case SOCKET_LOOP_COMPLETE:
			transmissionTime = currentMillis;
			mainLoopID = NETWORK_IDLE;
			break;
		default:
			break;
		}
	}
	return mainLoopID;
}

// east-1.compute.amazonaws.com - 3.0.183.162 - 1234

uint16_t SkyMate::getTrackerID() {
	return trackerID;
}

bool SkyMate::sendMainCommand(uint8_t readLoopStatus, unsigned long currentMillis, uint8_t messageCompare, long nextCommand) {
	return sendMainCommand(readLoopStatus, currentMillis, messageCompare, nextCommand, MAIN_COMMAND_TIMEOUT, -1);
}

bool SkyMate::sendMainCommand(uint8_t readLoopStatus, unsigned long currentMillis, uint8_t messageCompare, long nextCommand, unsigned long timeOut) {
	return sendMainCommand(readLoopStatus, currentMillis, messageCompare, nextCommand, timeOut, -1);
}

bool SkyMate::sendMainCommand(uint8_t readLoopStatus, unsigned long currentMillis, uint8_t messageCompare, long nextCommand, unsigned long timeOut, long errorCommand) {
	//return sendCommand(&mainLoopStatus, &mainLoopID, &mainPrevMillis, readLoopStatus, currentMillis, messageCompare, nextCommand, timeOut, errorCommand);
	if (mainLoopStatus == MAIN_COM_START) { // First run
		if (readLoopStatus != SANDERBUILT_SARA_REPLY_IN_PROGRESS) { // Ensure nothing is being recieved
			mainPrevMillis = currentMillis;
			mainLoopStatus++;
			return true; // Request to send command to SIM7000
		}
	} else {
		if (readLoopStatus == messageCompare) {
			mainLoopStatus = MAIN_COM_START;
			mainLoopID = nextCommand;
		} else if (readLoopStatus == SANDERBUILT_SARA_ERROR || readLoopStatus == SANDERBUILT_SARA_REPLY_COUNTER_TIMEOUT) {
			if (errorCommand != -1) {
				mainLoopStatus = MAIN_COM_START;
				mainLoopID = errorCommand;
			}
		} else if ((currentMillis - mainPrevMillis) > timeOut) {
			if (mainLoopStatus < MAIN_COMMAND_MAX_COUNT) { // Maximum retries
				if (readLoopStatus != SANDERBUILT_SARA_REPLY_IN_PROGRESS) { // Ensure nothing is being recieved
					mainPrevMillis = currentMillis;
					mainLoopStatus++;
					return true; // Request to send command to SIM7000
				}
			} else {
				// Reset SIM7000 using reset pin.
				mainLoopStatus = MAIN_COM_START;
				mainLoopID = NETWORK_IDLE;

				initLoopID = INIT_CHECK_RESPONSE;
				initLoopStatus = INIT_COM_START;
			}
		}
	}
	return false;
}

bool SkyMate::sendCommand(long *loopStatus, long *loopID, unsigned long *prevMillis, uint8_t readLoopStatus, unsigned long currentMillis, uint8_t messageCompare, long nextCommand, unsigned long timeOut, long errorCommand) {
	if (*loopStatus == MAIN_COM_START) { // First run
		if (readLoopStatus != SANDERBUILT_SARA_REPLY_IN_PROGRESS) { // Ensure nothing is being recieved
			*prevMillis = currentMillis;
			++(*loopStatus);
			return true; // Request to send command to SIM7000
		}
	}
	else {
		if (readLoopStatus == messageCompare) {
			*loopStatus = MAIN_COM_START;
			*loopID = nextCommand;
		}
		else if (readLoopStatus == SANDERBUILT_SARA_ERROR || readLoopStatus == SANDERBUILT_SARA_REPLY_COUNTER_TIMEOUT) {
			if (errorCommand != -1) {
				*loopStatus = MAIN_COM_START;
				*loopID = errorCommand;
			}
		}
		else if ((currentMillis - mainPrevMillis) > timeOut) {
			if (*loopStatus < MAIN_COMMAND_MAX_COUNT) { // Maximum retries
				if (readLoopStatus != SANDERBUILT_SARA_REPLY_IN_PROGRESS) { // Ensure nothing is being recieved
					*prevMillis = currentMillis;
					++(*loopStatus);
					return true; // Request to send command to SIM7000
				}
			}
			else {
				// Reset SIM7000 using reset pin.
				*loopStatus = MAIN_COM_START;
				*loopID = NETWORK_IDLE;
			}
		}
	}
	return false;
}

void SkyMate::createTrackerIDURL() {
	sprintf(path, "/s?ID=%s&v=%s", imei, "1.000");
}

boolean SkyMate::checkString(char* toreply) {
	char* p = strstr(replybuffer, toreply);
	if (p == 0) return false;

	return true;
}

boolean SkyMate::parseString(char* toreply, double* v, char divider, uint8_t index) {
	char* p = strstr(replybuffer, toreply);
	if (p == 0) return false;
	p += strlen(toreply);

	for (uint8_t i = 0; i < index; i++) {
		// increment dividers
		p = strchr(p, divider);
		if (!p) return false;
		p++;
		//DEBUG_PRINTLN(p);

	}
	*v = atof(p);

	return true;
}

boolean SkyMate::parseString(char *toreply, uint16_t *v, char divider, uint8_t index) {
	char *p = strstr(replybuffer, toreply);
	if (p == 0) return false;
	p += strlen(toreply);

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

boolean SkyMate::parseString(char *toreply, int *v, char divider, uint8_t index) {
	char *p = strstr(replybuffer, (prog_char*)toreply);
	if (p == 0) return false;
	p += strlen((prog_char*)toreply);

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

boolean SkyMate::parseString(char* toreply, long* v, char divider, uint8_t index) {
	char* p = strstr(replybuffer, (prog_char*)toreply);
	if (p == 0) return false;
	p += strlen((prog_char*)toreply);

	for (uint8_t i = 0; i < index; i++) {
		// increment dividers
		p = strchr(p, divider);
		if (!p) return false;
		p++;
		//DEBUG_PRINTLN(p);

	}
	*v = atol(p);

	return true;
}

boolean SkyMate::parseString(char* toreply, char *v, char divider, uint8_t index) {
	uint8_t i = 0;

	char *p = strstr(replybuffer, (prog_char*)toreply);
	if (p == 0) return false;
	p += strlen((prog_char*)toreply);

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

boolean SkyMate::parseReplyQuoted(char* toreply, char* v, int maxlen, char divider, uint8_t index) {
	uint8_t i = 0, j;
	// Verify response starts with toreply.
	char* p = strstr(replybuffer, (prog_char*)toreply);
	if (p == 0) return false;
	p += strlen((prog_char*)toreply);

	// Find location of desired response field.
	for (i = 0; i < index; i++) {
		// increment dividers
		p = strchr(p, divider);
		if (!p) return false;
		p++;
	}

	// Copy characters from response field into result string.
	for (i = 0, j = 0; j < maxlen && i < strlen(p); ++i) {
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

void SkyMate::writePositionStore(SkyMatePosition position) {
	hfSamples[hfSampleCounter] = position;

	hfSampleCounter++;
	if (hfSampleCounter >= HF_SAMPLES) hfSampleCounter = 0;

	if ((hfSampleCounter % LF_SAMPLE_RATE) == 0) {
		if (hfSamples[hfSampleCounter].valid == true) {
			//lfSamples[lfSampleCounter] = hfSamples[hfSampleCounter];
			lfSamples[lfSampleCounter].alt = hfSamples[hfSampleCounter].alt;
			lfSamples[lfSampleCounter].heading = hfSamples[hfSampleCounter].heading;
			lfSamples[lfSampleCounter].lat = hfSamples[hfSampleCounter].lat;
			lfSamples[lfSampleCounter].lon = hfSamples[hfSampleCounter].lon;
			//lfSamples[lfSampleCounter].pressure = hfSamples[hfSampleCounter].pressure;
			lfSamples[lfSampleCounter].sendStatus = hfSamples[hfSampleCounter].sendStatus;
			lfSamples[lfSampleCounter].speed = hfSamples[hfSampleCounter].speed;
			//lfSamples[lfSampleCounter].temperature = hfSamples[hfSampleCounter].temperature;
			lfSamples[lfSampleCounter].time = hfSamples[hfSampleCounter].time;
			lfSamples[lfSampleCounter].valid = hfSamples[hfSampleCounter].valid;
			
			lfSampleCounter++;
			if (lfSampleCounter >= LF_SAMPLES) lfSampleCounter = 0;
		}
	}
}

uint16_t SkyMate::getPostionStorePending(void) {
	uint16_t counter = 0;
	for (uint16_t i = 0; i < LF_SAMPLES; i++) {
		if (lfSamples[i].sendStatus == SEND_STATUS_NEW) {
			counter++;
		}
	}

	for (uint16_t i = 0; i < HF_SAMPLES; i++) {
		if (hfSamples[i].sendStatus == SEND_STATUS_NEW) {
			counter++;
		}
	}
	return counter;
}

void SkyMate::writeNetStatus(uint8_t status) {
	_net.netStatus = status;
}

void SkyMate::writeRSSI(uint8_t _rssi) {
	if (_rssi < 32) {
		_net.rssi = 113 - (_rssi * 2);
	}
	else {
		_net.rssi = 255;
	}
}

void SkyMate::writeRSRP(uint8_t _rsrp) {
	if (rsrp < 95) {
		_net.rsrp = 141 - rsrp;
	}
	else {
		_net.rsrp = 255;
	}
}

void SkyMate::writeRSRQ(uint8_t _rsrq) {
	if (_rsrq < 35) {
		_net.rsrq = 20 - (_rsrq * 0.5);
	}
	else {
		_net.rsrq = 255;
	}
}

double SkyMate::distanceBetween(double lat1, double long1, double lat2, double long2)
{
	// Borrowed from tinygps++
	// returns distance in meters between two positions, both specified
	// as signed decimal-degrees latitude and longitude. Uses great-circle
	// distance computation for hypothetical sphere of radius 6372795 meters.
	// Because Earth is no exact sphere, rounding errors may be up to 0.5%.
	// Courtesy of Maarten Lamers
	double delta = radians(long1 - long2);
	double sdlong = sin(delta);
	double cdlong = cos(delta);
	lat1 = radians(lat1);
	lat2 = radians(lat2);
	double slat1 = sin(lat1);
	double clat1 = cos(lat1);
	double slat2 = sin(lat2);
	double clat2 = cos(lat2);
	delta = (clat1 * slat2) - (slat1 * clat2 * cdlong);
	delta = sq(delta);
	delta += sq(clat2 * sdlong);
	delta = sqrt(delta);
	double denom = (slat1 * slat2) + (clat1 * clat2 * cdlong);
	delta = atan2(delta, denom);
	return delta * 6372795;
}
