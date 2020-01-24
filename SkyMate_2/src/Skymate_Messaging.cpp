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