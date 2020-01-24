#include "SkyMate.h";
#include <Update.h>;

long SkyMate::updateLoop(unsigned long currentMillis) {
	int status = 0;
	size_t written = 0;
	switch (updateLoopID) {
	case UPDATE_CONNECT_WIFI:
		if (updateLoopStatus == UPDATE_COM_START) {
			updatePrevMillis = currentMillis;
			WiFi.begin(SSID, PSWD);
			updateLoopStatus++;
			DEBUG_PRINTLN("CONNECT_WIFI");
		}
		else {
			if ((currentMillis - updatePrevMillis) > UPDATE_CONNECT_WIFI_TIMEOUT) {
				updateLoopNextStep(UPDATE_T_ERROR, currentMillis);
			}
			else if (WiFi.status() == WL_CONNECTED) {
				// Wifi is connected
				updateLoopNextStep(UPDATE_CLIENT_CONNECT, currentMillis);
			}
		}
		break;
	case UPDATE_CLIENT_CONNECT:
		if (updateLoopStatus == UPDATE_COM_START) {
			if (client.connect(host, port)) {
				client.printf("GET %s HTTP/1.1\r\nHost: %s\r\nCache-Control: no-cache\r\nConnection: close\r\n\r\n", bin, host);
			}
			else {
				DEBUG_PRINTLN(F("Connection to failed. Please check your setup"));
				updateLoopNextStep(UPDATE_T_ERROR, currentMillis);
			}

			DEBUG_PRINTLN("CLIENT CONNECT");
			updateLoopStatus++;
		}
		else {
			if ((currentMillis - updatePrevMillis) > UPDATE_CLIENT_CONNECT_TIMEOUT) {
				updateLoopNextStep(UPDATE_T_ERROR, currentMillis);
			}
			else if (client.available() != 0) {
				// Wifi is connected
				updateLoopNextStep(UPDATE_READ_HEADER, currentMillis);
			}
		}
		break;
	case UPDATE_READ_HEADER:

		DEBUG_PRINTLN("UPDATE_READ_HEADER");
		status = readUpdateLine();
		if (status == 1) {
			updateLoopNextStep(UPDATE_CHECK_BEGIN, currentMillis);
		}
		else if (status == -1) {
			updateLoopNextStep(UPDATE_T_ERROR, currentMillis);
		}
		break;
	case UPDATE_CHECK_BEGIN:

		DEBUG_PRINTLN("UPDATE CHECK BEGIN");
		if (contentLength && isValidContentType) {
			if (Update.begin(contentLength)) {
				updateLoopNextStep(UPDATE_DOWNLOAD_FILE, currentMillis);
			}
			else {
				updateLoopNextStep(UPDATE_T_ERROR, currentMillis);
				DEBUG_PRINTLN("Not enough space to begin OTA");
				client.flush();
			}
		}
		else {
			updateLoopNextStep(UPDATE_T_ERROR, currentMillis);
			DEBUG_PRINTLN("There was no content in the response");
			client.flush();
		}
		break;
	case UPDATE_DOWNLOAD_FILE:
		DEBUG_PRINTLN("Begin OTA.");
		// No activity would appear on the Serial monitor
		// So be patient. This may take 2 - 5mins to complete
		written = Update.writeStream(client);

		if (written == contentLength) {
			DEBUG_PRINT("Written : "); DEBUG_PRINT(written); DEBUG_PRINTLN(" successfully");
		}

		if (Update.end()) {
			if (Update.isFinished()) {
				DEBUG_PRINTLN("Update successfully completed. Rebooting.");
				updateLoopNextStep(UPDATE_COMPLETE, currentMillis);
			}
			else {
				DEBUG_PRINTLN(F("Update not finished? Something went wrong!"));
				updateLoopNextStep(UPDATE_T_ERROR, currentMillis);
				client.flush();
			}
		}
		else {
			DEBUG_PRINT("Error Occurred. Error #: "); DEBUG_PRINTLN(Update.getError());
			updateLoopNextStep(UPDATE_T_ERROR, currentMillis);
			client.flush();
		}
		break;
	case UPDATE_COMPLETE:
		// Should add a delay here;
		/*if ((currentMillis - updatePrevMillis) > UPDATE_COMPLETE_DELAY) {
			ESP.restart();
		}*/
		break;
	case UPDATE_T_ERROR:
		/*if ((currentMillis - updatePrevMillis) > UPDATE_ERROR_DELAY) {
			ESP.restart();
		}*/
		break;
	default:
		break;
	}

	return updateLoopID;
}

void SkyMate::updateLoopNextStep(long loopID, unsigned long currentMillis) {
	updateLoopID = loopID;
	updateLoopStatus = UPDATE_COM_START;
	updatePrevMillis = currentMillis;
}

int SkyMate::readUpdateLine() {
	String line = client.readStringUntil('\n');
	// remove space, to check if the line is end of headers
	line.trim();

	// if the the line is empty,
	// this is end of headers
	// break the while and feed the
	// remaining `client` to the
	// Update.writeStream();
	if (!line.length()) {
		//headers ended
		return 1; // and get the OTA started
	}

	// Check if the HTTP Response is 200
	// else break and Exit Update
	if (line.startsWith("HTTP/1.1")) {
		if (line.indexOf("200") < 0) {
			DEBUG_PRINTLN(F("Got a non 200 status code from server. Exiting OTA Update."));
			return -1;
		}
	}

	// extract headers here
	// Start with content length
	if (line.startsWith("Content-Length: ")) {
		contentLength = atol((getHeaderValue(line, "Content-Length: ")).c_str());
		DEBUG_PRINT(F("Got ")); DEBUG_PRINT(contentLength); DEBUG_PRINTLN(F(" bytes from server"));
	}

	// Next, the content type
	if (line.startsWith("Content-Type: ")) {
		String contentType = getHeaderValue(line, "Content-Type: ");
		Serial.println("Got " + contentType + " payload.");
		if (contentType == "application/octet-stream") {
			isValidContentType = true;
		}
	}

	return 0;
}