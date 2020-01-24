#include "SkyMate.h"

unsigned long SkyMate::calculateMessageLength(bool firstMessage) {
	return calculateMessageLength(firstMessage, MAXIMUM_SEND);
}

unsigned long SkyMate::calculateMessageLength(bool firstMessage, int maximum_send) {
	char battBuff[16];
	char postLine[150];
	long messageLength = 0;
	int pointCount = 0;


	dtostrf(_voltage, 1, 2, battBuff);
	/*
	{
	ID:IMEI,
	v:VOLTAGE,
	d:DATA,
	r:RSSI,
	t:TASK STATUS, CURRENT TURNPOINT
	p:{
	*/
	if (firstMessage) {
		sprintf(preLine, "{\"ID\":%s,\"v\":%s,\"b\":\"%s,%s\",\"ub\":\"%s\",\"p\":{", imei, battBuff, __DATE__, __TIME__, firmware);
	}
	else {
		sprintf(preLine, "{\"ID\":%s,\"v\":%s,\"r\":\"%s,%s,%i\",\"t\":\"%i,%i\",\"p\":{", imei, battBuff, rsrpCH, rsrqCH, dieTemperature, myNav->task.getTaskStatus(), myNav->task.getTurnPointNumber());
	}

	messageLength = strlen(preLine);

	for (int i = 0; i < maximum_send; i++) {
		SkyMatePosition tempPosition;
		char tempString[300];
		if (getUnwrittenPos(&tempPosition, SEND_STATUS_NEW)) {
			createPostLine(tempPosition, tempString);
			messageLength += strlen(tempString);
			if (i > 0) messageLength++; // Add the "," for between points.
		}
	}
	messageLength += 2; // "}}\n"

	return messageLength;
}

void SkyMate::sendMessage(void) {
	sendMessage(MAXIMUM_SEND);
}

void SkyMate::sendMessage(int maximum_send) {
	// send preLine here
	modemStream->print(preLine);
	DEBUG_PRINT(preLine);

	for (int i = 0; i < maximum_send; i++) {
		SkyMatePosition tempPosition;
		char tempString[300];
		if (getUnwrittenPos(&tempPosition, SEND_STATUS_PENDING)) {
			if (i > 0) {
				modemStream->print(","); // Add the "," for between points.
				DEBUG_PRINT(",");
			}
			createPostLine(tempPosition, tempString);
			modemStream->print(tempString);
			DEBUG_PRINT(tempString);
		}
	}
	
	modemStream->print("}}\n");
	DEBUG_PRINT("}}\n");
}

void SkyMate::createPostLine(SkyMatePosition position, char *postLine) {
	/*
	TIMESTAMP: {
	lat: LATITUDE,
	lon: LONGITUDE,
	s: SPEED,
	h: HEADING,
	a: ALTITUDE,
	t: TEMPERATURE,
	p: PRESSURE
	}
	*/
	sprintf(postLine, "\"%i\":{\"lat\":%0.6f,\"lon\":%0.6f,\"s\":%0.1f,\"h\":%0.0f,\"a\":%0.1f}", position.time, position.lat, position.lon, position.speed.mps(), position.heading, position.alt);
}

bool SkyMate::getUnwrittenPos(SkyMatePosition *position, uint8_t status) {
	uint16_t i;

	for (i = 0; i < LF_SAMPLES; i++) {
		uint16_t counter = i + lfSampleCounter;
		if (counter >= LF_SAMPLES) counter -= LF_SAMPLES;

		if (lfSamples[counter].sendStatus == status) {
			lfSamples[counter].sendStatus++;

			position->alt = lfSamples[counter].alt;
			position->heading = lfSamples[counter].heading;
			position->lat = lfSamples[counter].lat;
			position->lon = lfSamples[counter].lon;
			//position->pressure = lfSamples[counter].pressure;
			position->sendStatus = lfSamples[counter].sendStatus;
			position->speed = lfSamples[counter].speed;
			//position->temperature = lfSamples[counter].temperature;
			position->time = lfSamples[counter].time;
			return true;
		}
	}

	for (i = 0; i < HF_SAMPLES; i++) {
		uint16_t counter = i + hfSampleCounter;
		if (counter >= HF_SAMPLES) counter -= HF_SAMPLES;

		if (hfSamples[counter].sendStatus == status) {
			hfSamples[counter].sendStatus++;

			position->alt = hfSamples[counter].alt;
			position->heading = hfSamples[counter].heading;
			position->lat = hfSamples[counter].lat;
			position->lon = hfSamples[counter].lon;
			//position->pressure = hfSamples[counter].pressure;
			position->sendStatus = hfSamples[counter].sendStatus;
			position->speed = hfSamples[counter].speed;
			//position->temperature = hfSamples[counter].temperature;
			position->time = hfSamples[counter].time;
			return true;
		}
	}

	return false;
}

void SkyMate::clearPositionStatus(int8_t status) {
	writePositionStatus(status);
}

void SkyMate::writePositionStatus(int8_t status, int8_t newStatus) {
	uint16_t i;
	
	for (i = 0; i < LF_SAMPLES; i++) {
		if (lfSamples[i].sendStatus == status || (status == -1 && lfSamples[i].sendStatus != SEND_STATUS_SENT)) {
			lfSamples[i].sendStatus = newStatus;
		}
	}

	for (i = 0; i < HF_SAMPLES; i++) {
		if (hfSamples[i].sendStatus == status || (status == -1 && hfSamples[i].sendStatus != SEND_STATUS_SENT)) {
			hfSamples[i].sendStatus = newStatus;
		}
	}
}

void SkyMate::readServerSocketResponse(void) {
	uint16_t status;
	long lstatus;
	
	if (checkString("trk")) {
		firstSend = false;
		if (parseString("trk-uuid:", &status, ',', 0)) {
			if (trackerID != status) {
				trackerID = status;
				preferences->putUInt("trackerID", trackerID);
			}
		}

		if (parseString("trk-pos:", &status, ',', 0)) {
			writePositionStatus(SEND_STATUS_DOWNLOADED, SEND_STATUS_SENT);
		}

		if (parseString("trk-tsk:", &status, ',', 0)) {
			if (status > 0) {
				if (myNav->task.getTaskStatus() != TASK_ONTASK) {
					myNav->task.wipeTask();
					uint16_t turnPointLength = 0;
					if (status > 0 && parseString("trk-tsk:", &turnPointLength, ',', 1)) {
						myNav->task.setTask(turnPointLength);
					}
				}
			}
			else {
				myNav->task.wipeTask();
			}
		}

		if (parseString("trk-pref:", &status, ',', 0)) {
			_pref.vertical = status;
			preferences->putUInt("pref_vert", status);
			if (parseString("trk-pref:", &status, ',', 1)) {
				_pref.horizontal = status;
				preferences->putUInt("pref_hor", status);
			}
			if (parseString("trk-pref:", &status, ',', 2)) {
				_pref.altitude = status;
				preferences->putUInt("pref_alt", status);
			}
			if (parseString("trk-pref:", &status, ',', 3)) {
				_pref.distance = status;
				preferences->putUInt("pref_dist", status);
			}
			if (parseString("trk-pref:", &status, ',', 4)) {
				_pref.taskSpeed = status;
				preferences->putUInt("pref_task_speed", status);
			}
		}

		if (checkString("trk-pilot:")) {
			if (parseReplyQuoted("trk-pilot:", _pilot.firstName, 20, ',', 0)) {
				preferences->putString("pilot_first_name", _pilot.firstName);
			}
			if (parseReplyQuoted("trk-pilot:", _pilot.lastName, 20, ',', 1)) {
				preferences->putString("pilot_last_name", _pilot.lastName);
			}
			if (parseReplyQuoted("trk-pilot:", _pilot.gliderType, 15, ',', 2)) {
				preferences->putString("pilot_glider", _pilot.gliderType);
			}
			if (parseReplyQuoted("trk-pilot:", _pilot.rego, 8, ',', 3)) {
				preferences->putString("pilot_rego", _pilot.rego);
			}
		}

		if (checkString("trk-update")) {
			if (parseReplyQuoted("trk-update:", SSID, 30, ',', 0)) {
			} 

			if (parseReplyQuoted("trk-update:", PSWD, 30, ',', 1)) {
				mainLoopID = MAIN_LOOP_UPDATE_READY;
			}
		}

		if (checkString("trk-saraupdate")) {
			mainLoopID = MAIN_LOOP_SARA_UPDATE_READY;
		}

		if (parseString("trk-comp:", &status, ',', 0)) {
			// Comp Status
			if (status > 0) {
				myNav->task.compTask = true;
			}
			
			if (parseString("trk-comp:", &lstatus, ',', 1)) {
				myNav->task.startLine.gateTime = lstatus;
			}
		}

		if (myNav->task.getTaskStatus() != TASK_ONTASK) {
			// trk-tp:tpNumber,"name",lat,lon,radius
			if (parseString("trk-tp:", &status, ',', 0)) {
				char name[20];
				if (!parseReplyQuoted("trk-tp:", name, 20, ',', 1)) {
					sprintf(name, "tp %i", (status + 1));
				}
				double radius;
				if (!parseString("trk-tp:", &radius, ',', 4)) {
					radius = 500;
				}
				double lat;
				double lon;
				if (parseString("trk-tp:", &lat, ',', 2) && parseString("trk-tp:", &lon, ',', 3)) {
					myNav->task.setTurnPoint(status, name, lat, lon, radius);
					//DEBUG_PRINT(status); DEBUG_PRINT(","); DEBUG_PRINT(name); DEBUG_PRINT(","); DEBUG_PRINT(lat); DEBUG_PRINT(","); DEBUG_PRINT(lon); DEBUG_PRINT(","); DEBUG_PRINTLN(radius);
				}
			}
			// trk-tp:"name",lat,lon,maxAlt,maxSpeed
			else if (checkString("trk-sl:")) {
				char name[20];
				if (!parseReplyQuoted("trk-sl:", name, 20, ',', 0)) {
					strcpy(name, "Start");
				}
				double maxAlt;
				if (!parseString("trk-sl:", &maxAlt, ',', 3)) {
					maxAlt = 0;
				}
				double maxSpeed;
				if (!parseString("trk-sl:", &maxSpeed, ',', 4)) {
					maxSpeed = 0;
				}
				double lat;
				double lon;
				if (parseString("trk-sl:", &lat, ',', 1) && parseString("trk-sl:", &lon, ',', 2)) {
					myNav->task.setStartLine(name, lat, lon, maxAlt, maxSpeed);
					//DEBUG_PRINT(status); DEBUG_PRINT(","); DEBUG_PRINT(name); DEBUG_PRINT(","); DEBUG_PRINT(lat); DEBUG_PRINT(","); DEBUG_PRINTLN(lon);
				}
			}
			// trk-tp:"name",lat,lon,radius
			else if (checkString("trk-fl:")) {
				char name[20];
				if (!parseReplyQuoted("trk-fl:", name, 20, ',', 0)) {
					strcpy(name, "Start");
				}
				double minAlt;
				if (!parseString("trk-fl:", &minAlt, ',', 3)) {
					minAlt = 0;
				}
				double lat;
				double lon;
				if (parseString("trk-fl:", &lat, ',', 1) && parseString("trk-fl:", &lon, ',', 2)) {
					myNav->task.setFinishLine(name, lat, lon, minAlt);
					//DEBUG_PRINT(name); DEBUG_PRINT(","); DEBUG_PRINT(lat); DEBUG_PRINT(","); DEBUG_PRINTLN(lon);
				}
			}
		}
	}
}

void SkyMate::readServerHTTPResponse(void) {
	uint16_t status;
	long lstatus;

	if (parseString("trk-uuid:", &status, ',', 0)) {
		trackerID = status;
		writePositionStatus(SEND_STATUS_DOWNLOADED, SEND_STATUS_SENT);
	}
	if (checkString("trk")) {
		if (parseString("trk-uuid:", &status, ',', 0)) {
			if (trackerID != status) {
				trackerID = status;
				preferences->putUInt("trackerID", trackerID);
			}
		}

		if (parseString("trk-pos:", &status, ',', 0)) {
			writePositionStatus(SEND_STATUS_DOWNLOADED, SEND_STATUS_SENT);
		}

		if (parseString("trk-tsk:", &status, ',', 0)) {
			if (status > 0) {
				if (myNav->task.getTaskStatus() != TASK_ONTASK) {
					myNav->task.wipeTask();
					uint16_t turnPointLength = 0;
					if (status > 0 && parseString("trk-tsk:", &turnPointLength, ',', 1)) {
						myNav->task.setTask(turnPointLength);
					}
				}
			}
			else {
				myNav->task.wipeTask();
			}
		}

		if (parseString("trk-comp:", &status, ',', 0)) {
			// Comp Status
			if (status > 0) {
				myNav->task.compTask = true;
			}

			if (parseString("trk-comp:", &lstatus, ',', 1)) {
				myNav->task.startLine.gateTime = lstatus;
			}
		}
	}
}