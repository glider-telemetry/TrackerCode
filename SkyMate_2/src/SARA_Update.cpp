#include "SkyMate.h";
#include <Sanderbuilt_SARA.h>

long SkyMate::saraUpdateLoop(unsigned long currentMillis) {
	uint8_t readLoopStatus = myTracker->readloop();
	uint8_t intReadLoop = 0;
	uint16_t status;
	long lstatus;

	// Check for unscolicited messages.
	if (readLoopStatus == SANDERBUILT_SARA_REPLY_STRING) {
		myTracker->parseReplyBuffer(replybuffer);
		// Error
		if (parseString("+UUFTPCD:", &status, ',', 0)) {
			// Op Code
			if (parseString("+UUFTPCD:", &status, ',', 1)) {
				// Length of data
			}
		}

		if (parseString("+UUFTPCR:", &status, ',', 0)) {
			// Op Code
			if (parseString("+UUFTPCR:", &status, ',', 1)) {
				// FTP Result
				// 0 Fail, 1 Success
			}
		}
	}

	if (saraUpdateLoopID != NETWORK_IDLE) {
		// Check for "L0.0.00.00.05.06,A.02.01"
		switch (saraUpdateLoopID) {
		case NETWORK_CHECK_NETWORK_STATUS:
			if (readLoopStatus == SANDERBUILT_SARA_REPLY_STRING) {
				if (parseString("+CREG: ", &status, ',', 1)) {
					netStatus = status;
					writeNetStatus(status);
				}
			}
			if (sendSaraUpdateCommand(readLoopStatus, currentMillis, SANDERBUILT_SARA_OK, (netStatus != 99) ? NETWORK_CHECK_REQUEST_QUALITY : NETWORK_IDLE)) {
				myTracker->requestNetworkStatus();
			}
			break;
		case NETWORK_CHECK_REQUEST_QUALITY:
			if (readLoopStatus == SANDERBUILT_SARA_REPLY_STRING) {
				if (parseString("+CSQ:", &status, ',', 0)) {
					signalPower = status;
					writeRSSI(status);
				}
			}
			if (sendSaraUpdateCommand(readLoopStatus, currentMillis, SANDERBUILT_SARA_OK, NETWORK_DISPLAY_SIGNAL)) {
				myTracker->requestNetworkQuality();
			}
			break;
		case NETWORK_DISPLAY_SIGNAL:
			if (netStatus == 1 || netStatus == 5) {
				if (signalPower != 99) {
					saraUpdateLoopID = FOTA_FIRMWARE_CHECK;
				}
				else {
					saraUpdateLoopID = NETWORK_IDLE;
				}
			}
			else {
				saraUpdateLoopID = NETWORK_IDLE;
			}
			break;
		case FOTA_FIRMWARE_CHECK:
			if (readLoopStatus == SANDERBUILT_SARA_REPLY_STRING) {
				myTracker->parseReplyBuffer(replybuffer);
				if (checkString("L0.0.00.00.05.06,A.02.01")) {
					saraUpdateStatus = SARA_UPDATE_STAGE_1;
				}
			}
			else if (sendSaraUpdateCommand(readLoopStatus, currentMillis, SANDERBUILT_SARA_OK, FTP_SET_URL, 500, FTP_ERROR)) {
				myTracker->requestFirmwareVersion();
			}
			break;
		// SETUP FTP OPTIONS
		case FTP_SET_URL:
			if (sendSaraUpdateCommand(readLoopStatus, currentMillis, SANDERBUILT_SARA_OK, FTP_SET_PORT)) {
				myTracker->setFTPURL("ftptracker.f1gp.com.au");
			}
			break;
		case FTP_SET_PORT:
			if (sendSaraUpdateCommand(readLoopStatus, currentMillis, SANDERBUILT_SARA_OK, FTP_SET_USERNAME)) {
				myTracker->setFTPPort(21);
			}
			break;
		case FTP_SET_USERNAME:
			if (sendSaraUpdateCommand(readLoopStatus, currentMillis, SANDERBUILT_SARA_OK, FTP_SET_PASSWORD)) {
				myTracker->setFTPUser("todd@ftptracker.f1gp.com.au");
			}
			break;
		case FTP_SET_PASSWORD:
			if (sendSaraUpdateCommand(readLoopStatus, currentMillis, SANDERBUILT_SARA_OK, FTP_LOGIN)) {
				myTracker->setFTPPassword("B1gkn0cker$");
			}
			break;
		case FTP_LOGIN:
			if (sendSaraUpdateCommand(readLoopStatus, currentMillis, SANDERBUILT_SARA_OK, FTP_LOGIN_WAIT)) {
				myTracker->ftpLogin();
			}
			break;
		case FTP_LOGIN_WAIT:
			if (readLoopStatus == SANDERBUILT_SARA_REPLY_STRING) {
				myTracker->parseReplyBuffer(replybuffer);
				if (parseString("+UUFTPCR: ", &status, ',', 0)) {
					// Op Code
					if (parseString("+UUFTPCR: ", &status, ',', 1)) {
						if (status == 1) {
							saraUpdateLoopID = FTP_DOWNLOAD_UPDATE_1;
						}
						else {
							saraUpdateLoopID = FTP_ERROR;
						}
						// FTP Result
						// 0 Fail, 1 Success
					}
				}
			} else if ((currentMillis - saraUpdatePrevMillis) > 10000) {
				saraUpdateLoopID = FTP_ERROR;
			}
			break;
		case FTP_DOWNLOAD_UPDATE_1:
			if (sendSaraUpdateCommand(readLoopStatus, currentMillis, SANDERBUILT_SARA_OK, FTP_DOWNLOAD_UPDATE_1_WAIT, 60000)) {
				saraUpdateTopStatus = saraUpdateStatus;
				switch (saraUpdateStatus) {
				case SARA_UPDATE_INIT:
					saraUpdateLoopID = FOTA_NOT_REQD;
					saraUpdateLoopStatus = MAIN_COM_START;
					break;
				case SARA_UPDATE_STAGE_1:
					myTracker->ftpRetrieveFOTA("0bb_stg1_pkg1_0m_L56A0201_to_L58A0204.bin");
					break;
				case SARA_UPDATE_STAGE_2:
					myTracker->ftpRetrieveFOTA("0bb_stg1_pkg2_4m_L56A0201_to_L58A0204.bin");
					break;
				case SARA_UPDATE_STAGE_3:
					myTracker->ftpRetrieveFOTA("0bb_stg1_pkg3_8m_L56A0201_to_L58A0204.bin");
					break;
				case SARA_UPDATE_STAGE_MAIN:
					myTracker->ftpRetrieveFOTA("0bb_stg2_L56A0201_to_L58A0204.bin");
					break;
				case FTP_ERROR:
					saraUpdateLoopID = FTP_ERROR;
					saraUpdateLoopStatus = MAIN_COM_START;
					break;
				default:
					DEBUG_PRINTLN("DEFAULT ERROR");
					saraUpdateLoopID = FTP_ERROR;
					saraUpdateLoopStatus = MAIN_COM_START;
					break;
				}
			}
			break;
		case FTP_DOWNLOAD_UPDATE_1_WAIT:
			if (readLoopStatus == SANDERBUILT_SARA_REPLY_STRING) {
				myTracker->parseReplyBuffer(replybuffer);
				if (parseString("+UUFTPCR: ", &status, ',', 0)) {
					// Op Code
					saraUpdatePrevMillis = currentMillis;
					if (parseString("+UUFTPCR: ", &status, ',', 1)) {
						if (status == 0) {
							saraUpdateLoopID = FTP_ERROR;
						}
						else if (status == 1) {
							saraUpdateLoopID = FTP_DOWNLOAD_UPDATE_1_FOTA;
						}
						// FTP Result
						// 0 Fail, 1 Success
					}
				}
			}
			else if ((currentMillis - saraUpdatePrevMillis) > 120000) {
				saraUpdateLoopID = FTP_ERROR;
			}
			break;
		case FTP_DOWNLOAD_UPDATE_1_FOTA:
			if (sendSaraUpdateCommand(readLoopStatus, currentMillis, SANDERBUILT_SARA_OK, FTP_DOWNLOAD_UPDATE_1_FOTA_WAIT, 1260000)) {
				myTracker->FOTAInstall();
			}
			break;
		case FTP_DOWNLOAD_UPDATE_1_FOTA_WAIT:
			if (readLoopStatus == SANDERBUILT_SARA_REPLY_COUNTER_TIMEOUT) {
				saraUpdateLoopID = FTP_DOWNLOAD_UPDATE_1_FOTA_WAIT_2;
				saraUpdatePrevMillis = currentMillis;
			}
			else if ((currentMillis - saraUpdatePrevMillis) > 1260000) {
				saraUpdateLoopID = FTP_DOWNLOAD_UPDATE_1_AT;
			}
			break;
		case FTP_DOWNLOAD_UPDATE_1_FOTA_WAIT_2:
			if ((currentMillis - saraUpdatePrevMillis) > 10000) {
				saraUpdateLoopID = FTP_DOWNLOAD_UPDATE_1_AT;
			}
			break;
		case FTP_DOWNLOAD_UPDATE_1_AT:
			if (sendSaraUpdateCommand(readLoopStatus, currentMillis, SANDERBUILT_SARA_OK, FTP_DOWNLOAD_UPDATE_1_FOTA_CHECK, 60000, FTP_DOWNLOAD_UPDATE_1_AT)) {
				myTracker->requestECHO();
			}
			break;
		case FTP_DOWNLOAD_UPDATE_1_FOTA_CHECK:
			if (saraUpdateTopStatus == SARA_UPDATE_STAGE_MAIN) {
				saraUpdateLoopID = FTP_DOWNLOAD_UPDATE_LOOP_COMPLETE;
				saraUpdateLoopStatus = MAIN_COM_START;
			} 
			else if (readLoopStatus == SANDERBUILT_SARA_REPLY_STRING) {
				myTracker->parseReplyBuffer(replybuffer);
				if (checkString("+UFWSTATUS: 55436f6d,c8,0") || checkString("+UFWSTATUS: 55436f6d, c8, 0")) {
					saraUpdateStatus = SARA_UPDATE_STAGE_MAIN;
					DEBUG_PRINTLN("SUCCESSSSSSS !!!!!!!");
				}
				else if (checkString("+UFWSTATUS: 55457272,19a,ffe3") || checkString("+UFWSTATUS: 55457272, 19a, ffe3")) {
					saraUpdateStatus++;
				}
				else if (checkString("+UFWSTATUS: 55457272,19a,ffed") || checkString("+UFWSTATUS: 55457272, 19a, ffed")) {
					saraUpdateStatus = SARA_UPDATE_STAGE_1;
				}
				else if (checkString("+UFWSTATUS:")) {
					saraUpdateStatus = FTP_ERROR;
				}
			} else if (sendSaraUpdateCommand(readLoopStatus, currentMillis, SANDERBUILT_SARA_OK, FTP_SET_URL, 600000)) {
				myTracker->FOTAInstallCheck();
			}
			break;
		case FTP_DOWNLOAD_UPDATE_LOOP_COMPLETE:
			/*if ((currentMillis - saraUpdatePrevMillis) > 5000) {
				ESP.restart();
				saraUpdateLoopID = NETWORK_IDLE;
			}*/
			break;
		case FTP_ERROR:
			/*if ((currentMillis - saraUpdatePrevMillis) > 5000) {
				saraUpdateLoopID = NETWORK_IDLE;
				ESP.restart();
			}*/
			break;
		default:
			break;
		}
	}
	return saraUpdateLoopID;
}

bool SkyMate::sendSaraUpdateCommand(uint8_t readLoopStatus, unsigned long currentMillis, uint8_t messageCompare, long nextCommand) {
	return sendSaraUpdateCommand(readLoopStatus, currentMillis, messageCompare, nextCommand, 500, FTP_ERROR);
}

bool SkyMate::sendSaraUpdateCommand(uint8_t readLoopStatus, unsigned long currentMillis, uint8_t messageCompare, long nextCommand, unsigned long timeOut) {
	return sendSaraUpdateCommand(readLoopStatus, currentMillis, messageCompare, nextCommand, timeOut, FTP_ERROR);
}

bool SkyMate::sendSaraUpdateCommand(uint8_t readLoopStatus, unsigned long currentMillis, uint8_t messageCompare, long nextCommand, unsigned long timeOut, long errorCommand) {
	if (saraUpdateLoopStatus == MAIN_COM_START) { // First run
		if (readLoopStatus != SANDERBUILT_SARA_REPLY_IN_PROGRESS) { // Ensure nothing is being recieved
			saraUpdatePrevMillis = currentMillis;
			saraUpdateLoopStatus++;
			return true; // Request to send command to SIM7000
		}
	}
	else {
		if (readLoopStatus == messageCompare) {
			saraUpdateLoopStatus = MAIN_COM_START;
			saraUpdateLoopID = nextCommand;
		}
		else if (readLoopStatus == SANDERBUILT_SARA_ERROR || readLoopStatus == SANDERBUILT_SARA_REPLY_COUNTER_TIMEOUT) {
			if (errorCommand != -1) {
				saraUpdateLoopStatus = MAIN_COM_START;
				saraUpdateLoopID = errorCommand;
			}
		}
		else if ((currentMillis - saraUpdatePrevMillis) > timeOut) {
			if (saraUpdateLoopStatus < 30) { // Maximum retries
				if (readLoopStatus != SANDERBUILT_SARA_REPLY_IN_PROGRESS) { // Ensure nothing is being recieved
					saraUpdatePrevMillis = currentMillis;
					saraUpdateLoopStatus++;
					return true; // Request to send command to SIM7000
				}
			}
			else {
				// Reset SIM7000 using reset pin.
				saraUpdateLoopStatus = MAIN_COM_START;
				saraUpdateLoopID = FTP_ERROR;
			}
		}
	}
	return false;
}