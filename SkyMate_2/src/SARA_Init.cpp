/*
1, Power on
2. Reset
3. Check if on
4. Turn off echo
5. 
*/

#include "SkyMate.h"
#include <Sanderbuilt_SARA.h>

SkyMate::SkyMate(int8_t sara_toggle, int8_t sara_enable, int8_t sara_reset, int8_t sara_power) {
	_sara_toggle = sara_toggle;
	_sara_enable = sara_enable;
	_sara_reset = sara_reset;
	_sara_power = sara_power;
}

void SkyMate::init(Sanderbuilt_SARA &tracker, Stream &modem, Stream &debug, Navigation &nav, Preferences &_preferences) {
	myTracker = &tracker;
	modemStream = &modem;
	debugStream = &debug;
	myNav = &nav;
	preferences = &_preferences;

	if (_sara_enable >= 0) {
		digitalWrite(_sara_enable, LOW);
		pinMode(_sara_enable, OUTPUT);
	}

	if (_sara_toggle >= 0) {
		digitalWrite(_sara_toggle, LOW);
		pinMode(_sara_toggle, OUTPUT);
	}

	if (_sara_reset >= 0) {
		pinMode(_sara_reset, OUTPUT);
		digitalWrite(_sara_reset, HIGH);
	}

	if (_sara_power >= 0) {
		pinMode(_sara_power, INPUT);
	}

	_pref.altitude = preferences->getUInt("pref_alt", PREFERENCES_ALTITUDE_FEET);
	_pref.distance = preferences->getUInt("pref_dist", PREFERENCES_DISTANCE_KM);
	_pref.horizontal = preferences->getUInt("pref_hor", PREFERENCES_HORIZONTAL_KNOTS);
	_pref.taskSpeed = preferences->getUInt("pref_task_speed", PREFERENCES_TASK_SPEED_KPH);
	_pref.vertical = preferences->getUInt("pref_vert", PREFERENCES_VERTICAL_KNOTS);

	preferences->getString("pilot_first_name", _pilot.firstName, 20);
	preferences->getString("pilot_last_name", _pilot.lastName, 20);
	preferences->getString("pilot_rego", _pilot.rego, 8);
	preferences->getString("pilot_glider", _pilot.gliderType, 15);

	DEBUG_PRINTLN(_pilot.firstName);
	DEBUG_PRINTLN(_pilot.lastName);

	trackerID = preferences->getUInt("trackerID", 0);
}

void SkyMate::loopReset(void) {
	initLoopStatus = INIT_COM_START;
	initLoopID = INIT_PWR_KEY_ON;
}

long SkyMate::initLoop(unsigned long currentMillis) {
	if (initLoopID == INIT_COMPLETE | initLoopID == INIT_SHUTDOWN_COMPLETE) return initLoopID;
	uint16_t status;
	uint8_t readLoopStatus = myTracker->readloop();

	switch (initLoopID) {
	case INIT_DELAYED_POWER_ON:
		if (initLoopStatus == INIT_COM_START) {
			initPrevMillis = currentMillis;
			if (_sara_enable >= 0) {
				digitalWrite(_sara_enable, LOW); // Power off
			} else {
				initLoopStatus = INIT_COM_START;
				initLoopID = INIT_POWER_ON;
			}
			initLoopStatus++;
		}
		else {
			// Wait a for INIT_DELAYED_POWER_ON_WAIT milliseconds
			if ((currentMillis - initPrevMillis) > INIT_DELAYED_POWER_ON_WAIT) {
				initLoopStatus = INIT_COM_START;
				initLoopID = INIT_POWER_ON;
			}
		}
		break;

	case INIT_POWER_ON:  // Switch Power on to MODEM
		if (initLoopStatus == INIT_COM_START) {
			initPrevMillis = currentMillis;
			if (_sara_power >= 0 && resetCounter == 0) {
				if (digitalRead(_sara_power)) {
					DEBUG_PRINTLN("POWER ALREADY ON");

					initLoopStatus = INIT_COM_START;
					initLoopID = INIT_CHECK_RESPONSE;
					break;
				}
			}
			if (_sara_enable >= 0) {
				digitalWrite(_sara_enable, HIGH); // Power on
			} else {
				initLoopStatus = INIT_COM_START;
				initLoopID = INIT_PWR_KEY_ON;
			}
			initLoopStatus++;
		}
		else {
			// Wait a for INIT_POWER_ON_WAIT milliseconds
			if ((currentMillis - initPrevMillis) > INIT_POWER_ON_WAIT) {
				initLoopStatus = INIT_COM_START;
				initLoopID = INIT_RESET_ON;
			}
		}
		break;

	case INIT_RESET_ON:
		if (initLoopStatus == INIT_COM_START) { // Reset
			DEBUG_PRINT(currentMillis);
			DEBUG_PRINT("\t");
			DEBUG_PRINTLN("INIT_RESET_ON");
			resetLoop = true;
			socketErrorCounter = 0;
			initPrevMillis = currentMillis;
			if (_sara_reset >= 0) {
				digitalWrite(_sara_reset, LOW);
			}
			else {
				initLoopStatus = INIT_COM_START;
				initLoopID = INIT_PWR_KEY_ON;
			}
			initLoopStatus++;
		}
		else {
			// Wait a for INIT_RESET_ON_WAIT milliseconds
			if ((currentMillis - initPrevMillis) > INIT_RESET_ON_WAIT) {
				initLoopStatus = INIT_COM_START;
				initLoopID = INIT_RESET_OFF;
			}
		}
		break;

	case INIT_RESET_OFF:
		if (initLoopStatus == INIT_COM_START) {
			DEBUG_PRINT(currentMillis);
			DEBUG_PRINT("\t");
			DEBUG_PRINTLN("INIT_RESET_OFF");
			initPrevMillis = currentMillis;
			if (_sara_reset >= 0) {
				digitalWrite(_sara_reset, HIGH);
			}
			else {
				initLoopStatus = INIT_COM_START;
				initLoopID = INIT_PWR_KEY_ON;
			}
			initLoopStatus++;
		}
		else {
			// Wait a for INIT_RESET_OFF_WAIT milliseconds
			if ((currentMillis - initPrevMillis) > INIT_PWR_KEY_OFF_WAIT) {
				initLoopStatus = INIT_COM_START;
				initLoopID = INIT_PWR_KEY_ON;
			}
		}
		break;

	case INIT_PWR_KEY_ON:
		if (initLoopStatus == INIT_COM_START) { // Reset SARA
			DEBUG_PRINT(currentMillis);
			DEBUG_PRINT("\t");
			DEBUG_PRINTLN("INIT_PWR_KEY_ON");
			initPrevMillis = currentMillis;
			if (_sara_power >= 0) {
				if (digitalRead(_sara_power)) {
					DEBUG_PRINTLN("POWER ALREADY ON");

					initLoopStatus = INIT_COM_START;
					initLoopID = INIT_CHECK_RESPONSE;
					break;
				}
			}
			if (_sara_toggle >= 0) {
				digitalWrite(_sara_toggle, LOW);
				initLoopStatus++;
			} else {
				initLoopStatus = INIT_COM_START;
				initLoopID = INIT_PWR_KEY_OFF;
			}
		}
		else {
			// Wait a for INIT_RESET_ON_WAIT milliseconds
			if ((currentMillis - initPrevMillis) > INIT_PWR_KEY_ON_WAIT) {
				initLoopStatus = INIT_COM_START;
				initLoopID = INIT_PWR_KEY_OFF;
			}
		}
		break;

	case INIT_PWR_KEY_OFF:
		if (initLoopStatus == INIT_COM_START) {
			DEBUG_PRINT(currentMillis);
			DEBUG_PRINT("\t");
			DEBUG_PRINTLN("INIT_PWR_KEY_OFF");
			initPrevMillis = currentMillis;
			if (_sara_toggle >= 0) {
				digitalWrite(_sara_toggle, HIGH);
			} else {
				initLoopStatus = INIT_COM_START;
				initLoopID = INIT_CHECK_RESPONSE;
			}
			initLoopStatus++;
		}
		else {
			// Wait a for INIT_RESET_OFF_WAIT milliseconds
			if ((currentMillis - initPrevMillis) > INIT_PWR_KEY_OFF_WAIT) {
				initLoopStatus = INIT_COM_START;
				initLoopID = INIT_CHECK_RESPONSE;
			}
		}
		break;

	case INIT_CHECK_POWER:
		if (initLoopStatus == INIT_COM_START) {
			initPrevMillis = currentMillis;
			initLoopStatus++;
		}
		else {
			if (digitalRead(_sara_power)) {
				DEBUG_PRINTLN("POWER ALREADY ON");

				initLoopStatus = INIT_COM_START;
				initLoopID = INIT_CHECK_RESPONSE;
				break;
			}
			// Wait a for INIT_RESET_OFF_WAIT milliseconds
			if ((currentMillis - initPrevMillis) > INIT_CHECK_POWER_WAIT) {
				initLoopStatus = INIT_COM_START;
				initLoopID = INIT_RESET_ON;
			}
		}
		break;
	case INIT_CHECK_RESPONSE: // Send 'AT' to SARA till we recieve a response
		if (initLoopStatus == INIT_COM_START) {
			if (readLoopStatus != SANDERBUILT_SARA_REPLY_IN_PROGRESS) { // Ensure nothing is being recieved
				initPrevMillis = currentMillis;
				initLoopStatus++;
				myTracker->requestOK();
			}
		}
		else {
			if (readLoopStatus == SANDERBUILT_SARA_OK) {
				resetCounter = 0;
				initLoopStatus = INIT_COM_START;
				initLoopID = INIT_ECHO;
			}
			else if ((currentMillis - initPrevMillis) > INIT_CHECK_RESPONSE_WAIT) {
				// If we have waited longer than INIT_CHECK_RESPONSE_WAIT
				if (initLoopStatus < INIT_CHECK_RESPONSE_COUNT) {
					if (readLoopStatus != SANDERBUILT_SARA_REPLY_IN_PROGRESS) { // Ensure nothing is being recieved
						initPrevMillis = currentMillis;
						initLoopStatus++;
						myTracker->requestOK();
					}
				}
				else {
					resetCounter++;
					// Trouble contacting SIM7000
					if (resetCounter < 2) { // If this is the first try, start with simple reset
						initLoopStatus = INIT_COM_START;
						initLoopID = INIT_RESET_ON;
					}
					else { // Last resort, do a full power down and start up
						initLoopStatus = INIT_COM_START;
						initLoopID = INIT_DELAYED_POWER_ON;
					}
				}
			}
		}
		break;

/************************** BEGINS HERE ************************************************/
	case INIT_ECHO:
		if (sendInitCommand(readLoopStatus, currentMillis, INIT_CMEE)) {
			myTracker->requestECHO();
		}
		break;
	case INIT_CMEE:
		if (sendInitCommand(readLoopStatus, currentMillis, INIT_GET_IMEI)) {
			myTracker->requestSetCMEE(1);
		}
		break;
	case INIT_GET_IMEI:
		if (initLoopStatus == INIT_COM_START) {
			// Get tracker IMEI
			myTracker->getIMEI(imei);
		}
		bandSet = true;
		if (bandSet && profileSet) {
			initLoopID = INIT_SET_CREG;
		}
		else if (profileSet) {
			initLoopID = INIT_CHECK_BANDMASK;
		}
		else {
			initLoopID = INIT_CHECK_PROFILE;
		}
		break;

/************************** MNO PROFILE ************************************************/
	case INIT_CHECK_PROFILE:
		if (readLoopStatus == SANDERBUILT_SARA_REPLY_STRING) {
			myTracker->parseReplyBuffer(replybuffer);
			parseString("+UMNOPROF: ", &MNOProfile, ',', 0);
			if (MNOProfile == MNO_PROFILE) {
				profileSet = true;
			}
		}

		if (sendInitCommand(readLoopStatus, currentMillis, profileSet ? INIT_SET_CREG : INIT_DISABLE_PROFILE, INIT_SET_CREG)) {
			myTracker->requestMNOProfile();
		}
		break;
	case INIT_DISABLE_PROFILE:
		if (sendInitCommand(readLoopStatus, currentMillis, INIT_SET_MNO_PROFILE, INIT_SET_CREG)) {
			myTracker->disableModem();
		}
		break;
	case INIT_SET_MNO_PROFILE:
		if (profileSet) {
			initLoopID = INIT_SET_CREG;
		}
		else {
			if (sendInitCommand(readLoopStatus, currentMillis, INIT_PROFILE_RESET)) {
				myTracker->setMNOProfile(MNO_PROFILE);
			}
		}
		break;
	case INIT_PROFILE_RESET:
		profileSet = true;
		initLoopID = INIT_SOFT_RESET;
		break;

/************************** BAND PROFILE ************************************************/
	case INIT_CHECK_BANDMASK:
		if (readLoopStatus == SANDERBUILT_SARA_REPLY_STRING) {
			myTracker->parseReplyBuffer(replybuffer);
			parseString("+UBANDMASK:", bandBitmask, ',', 1);
			if (strcmp(bandBitmask, BANDMASK) == 0) {
				bandSet = true;
			}
		}

		if (sendInitCommand(readLoopStatus, currentMillis, bandSet ? INIT_SET_CREG : INIT_DISABLE_BANDMASK, INIT_SET_CREG)) {
			myTracker->requestFrequencyMap();
		}
		break;
	case INIT_DISABLE_BANDMASK:
		if (sendInitCommand(readLoopStatus, currentMillis, INIT_SET_BANDMASK, INIT_SET_CREG)) {
			myTracker->disableModem();
		}
		break;
	case INIT_SET_BANDMASK:
		if (sendInitCommand(readLoopStatus, currentMillis, INIT_RESET_BANDMASK, INIT_RESET_BANDMASK)) {
			myTracker->setFrequencyMap(0, BANDMASK);
		}
		break;
	case INIT_RESET_BANDMASK:
		bandSet = true;
		initLoopID = INIT_SOFT_RESET;
		break;

/***************************** Soft Reset ***************************************************/
	case INIT_SOFT_RESET:
		if (sendInitCommand(readLoopStatus, currentMillis, INIT_RESET_DELAY, INIT_RESET_ON)) {
			myTracker->silentReset();
		}
		break;
	case INIT_RESET_DELAY:
		if ((currentMillis - initPrevMillis) > 5000) {
			initLoopID = INIT_CHECK_POWER;
			initPrevMillis = currentMillis;
		}
		break;

/************************** General Settings ************************************************/
	case INIT_SET_CREG:
		profileSet = true;
		bandSet = true;
		if (sendInitCommand(readLoopStatus, currentMillis, INIT_SET_UCGED)) {
			myTracker->setCREG(2);
		}
		break; 
	case INIT_SET_UCGED:
		if (sendInitCommand(readLoopStatus, currentMillis, INIT_SET_APN)) {
			myTracker->setUCGED(5);
		}
		break;
	case INIT_SET_APN:
		#ifdef APN
		if (sendInitCommand(readLoopStatus, currentMillis, INIT_REQUEST_PHONE_NUMBER)) {
			myTracker->setAPN(APN);
		}
		#else
		initLoopID = INIT_REQUEST_PHONE_NUMBER;
		#endif
		break;
	case INIT_REQUEST_PHONE_NUMBER:
		if (readLoopStatus == SANDERBUILT_SARA_REPLY_STRING) {
			myTracker->parseReplyBuffer(replybuffer);
			parseReplyQuoted("+CNUM", phoneNumber, 15, ',', 1);
		}
		if (sendInitCommand(readLoopStatus, currentMillis, INIT_REQUEST_FIRMWARE_VERSION, INIT_REQUEST_FIRMWARE_VERSION)) {
			myTracker->requestPhoneNumber();
		}
		break;
	case INIT_REQUEST_FIRMWARE_VERSION:
		if (initLoopStatus != INIT_COM_START) {
			if (readLoopStatus == SANDERBUILT_SARA_REPLY_STRING) {
				myTracker->parseReplyBuffer(replybuffer); // Ensure nothing is being recieved
				if (strlen(replybuffer) < 30) {
					strcpy(firmware, replybuffer);
				}
			}
		}
		if (sendInitCommand(readLoopStatus, currentMillis, INIT_REQUEST_ATI)) {
			myTracker->requestFirmwareVersion();
		}
		break;
	case INIT_REQUEST_ATI:
		/*
		if (sendInitCommand(readLoopStatus, currentMillis, INIT_CHECK_FREQUENCY)) {
			myTracker->requestDeviceVersion();
			if (resetLoop) socketReadPrevMillis = currentMillis;
		}
		break;*/
	case INIT_CHECK_FREQUENCY:
		if (sendInitCommand(readLoopStatus, currentMillis, INIT_COMPLETE, INIT_COMPLETE)) {
			myTracker->requestFrequencyMap();
		}
		break;
	case INIT_COMPLETE:
		break;

/************************** SHUTDOWN ************************************************/
	case INIT_SHUTDOWN:
		initLoopID = INIT_CLOSE_SOCKETS;
		break;
	case INIT_CLOSE_SOCKETS:
		if (socket == 99) {
			initLoopID = INIT_REQUEST_SHUTDOWN;
		} 
		else if (initLoopStatus == INIT_COM_START) {
			if (readLoopStatus != SANDERBUILT_SARA_REPLY_IN_PROGRESS) { // Ensure nothing is being recieved
				initPrevMillis = currentMillis;
				initLoopStatus++;
				myTracker->closeSocket(socket);
			}
		}
		else {
			if (readLoopStatus == SANDERBUILT_SARA_OK) {
				resetCounter = 0;
				initLoopStatus = INIT_COM_START;
				initLoopID = INIT_REQUEST_SHUTDOWN;
				socket = 99;
			}
			else if ((currentMillis - initPrevMillis) > 4500) {
				resetCounter = 0;
				initLoopStatus = INIT_COM_START;
				initLoopID = INIT_REQUEST_SHUTDOWN;
			}
		}
		break;
	case INIT_REQUEST_SHUTDOWN:
		if (initLoopStatus == INIT_COM_START) {
			if (readLoopStatus != SANDERBUILT_SARA_REPLY_IN_PROGRESS) { // Ensure nothing is being recieved
				initPrevMillis = currentMillis;
				initLoopStatus++;
				myTracker->requestShutdown();
			}
		}
		else {
			if (readLoopStatus == SANDERBUILT_SARA_OK) {
				resetCounter = 0;
				initLoopStatus = INIT_COM_START;
				initLoopID = INIT_SHUTDOWN_COMPLETE;
			}
			else if ((currentMillis - initPrevMillis) > INIT_COMMAND_TIMEOUT) {
				// If we have waited longer than INIT_COMMAND_TIMEOUT
				// Do nothing for now.
				resetCounter = 0;
				initLoopStatus = INIT_COM_START;
				initLoopID = INIT_SHUTDOWN_COMPLETE;
			}
		}
		break;
	default:
		break;
	}

	return initLoopID;
}

bool SkyMate::sendInitCommand(uint8_t readLoopStatus, unsigned long currentMillis, long nextCommand) {
	return sendInitCommand(readLoopStatus, currentMillis, nextCommand, -1);
}

bool SkyMate::sendInitCommand(uint8_t readLoopStatus, unsigned long currentMillis, long nextCommand, long errorCommand) {
	if (initLoopStatus == INIT_COM_START) { // First run
		if (readLoopStatus != SANDERBUILT_SARA_REPLY_IN_PROGRESS) { // Ensure nothing is being recieved
			initPrevMillis = currentMillis;
			initLoopStatus++;
			return true; // Request to send command to SIM7000
		}
	}
	else {
		if (readLoopStatus == SANDERBUILT_SARA_OK) {
			resetCounter = 0;
			initLoopStatus = INIT_COM_START;
			initLoopID = nextCommand;
		}
		else if (readLoopStatus == SANDERBUILT_SARA_ERROR || readLoopStatus == SANDERBUILT_SARA_REPLY_COUNTER_TIMEOUT) {
			if (errorCommand != -1) {
				initLoopStatus = INIT_COM_START;
				initLoopID = errorCommand;
			}
		}
		else if ((currentMillis - initPrevMillis) > INIT_COMMAND_TIMEOUT) {
			if (initLoopStatus < INIT_COMMAND_MAX_COUNT) { // Maximum retries
				if (readLoopStatus != SANDERBUILT_SARA_REPLY_IN_PROGRESS) { // Ensure nothing is being recieved
					initPrevMillis = currentMillis;
					initLoopStatus++;
					return true; // Request to send command to SIM7000
				}
			}
			else {
				// Reset SIM7000 using reset pin.
				initLoopStatus = INIT_COM_START;
				initLoopID = INIT_RESET_ON;
			}
		}
	}
	return false;
}

void SkyMate::shutdown(void) {
	initLoopStatus = INIT_COM_START;
	initLoopID = INIT_SHUTDOWN;
	firstSend = true;
	ipSet = false;
}

void SkyMate::getImei(char *_imei) {
	strcpy(_imei, imei);
}
