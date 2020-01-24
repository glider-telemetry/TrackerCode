#include <Arduino.h>

#include <qrcode.h>
#include <Update.h>
#include <WiFi.h>
#define VERSION "2.018"
#define SETTINGS_VERSION "2.006"
#define DAY __DATE__
#define TIME __TIME__

// Generic Board definition for code readability
#include "Boards/generic_board.h"

#define DEBUG
#define DEBUG_STREAM Serial

//#include <Update.h>

// Local includes
#include "Debug.h"
#include "SkyMate_Display.h"
#include "SkyMate.h"
#include "BQ24259.h"

// ESP32 included libraries
#include <Wire.h>
#include <SPI.h>
#include <Preferences.h>
#include <time.h>

// Externally Required Libraries
#include <TinyGPS++.h>
#include <Adafruit_Sensor.h>
#include <arduinoFFT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_BMP3XX.h>
#include <Sanderbuilt_SARA.h>

time_t convertToUnixTime(TinyGPSTime time, TinyGPSDate date);

SPIClass MYSPI;
HardwareSerial GPS(2);

#define GPS_STREAM GPS
#define MODEM_STREAM Serial1

SkyMatePosition currentPos;

// Pressure
Adafruit_BMP3XX bmp;
bool pressure_available = false;

// Tracker Initialisation
Sanderbuilt_SARA tracker = Sanderbuilt_SARA(99);

TinyGPSPlus gps;
Navigation nav(gps);
SkyMate mate = SkyMate(N_PWR_ON, -1, N_RESET, V_INT);
SkyMate_Display sky_display = SkyMate_Display(SHARP_SS, SHARP_WIDTH, SHARP_HEIGHT);

unsigned long shutdownTime = 0;

BQ24259 charger = BQ24259();

unsigned long prevMainLoopTime = 0;
bool firstRun = true;
char imei[16];
bool gpsReading = false;
bool toggle = true;
bool debounce = false;
bool updating = false;

bool updateDisplay = true;
unsigned long displayRefreshTime = 0;
unsigned long startUpTime = 0;

float battLevelCalibration;
bool battCal;
Preferences preferences;

uint8_t chargeStatus = 0;
bool shutWait = false;

uint8_t onState = ON_STATE_STARTUP;
uint8_t onStateStatus = ON_STATE_START;

void setup()
{
	// Setup On Switch
	pinMode(ON_SW, INPUT);

	// Setup Serial Streams
	DEBUG_STREAM.begin(DEBUG_STREAM_BAUD);
	MODEM_STREAM.begin(MODEM_STREAM_BAUD, SERIAL_8N1, MODEM_STREAM_RX, MODEM_STREAM_TX,false);
	GPS_STREAM.begin(GPS_STREAM_BAUD, SERIAL_8N1, GPS_STREAM_RX, GPS_STREAM_TX, false);

	preferences.begin(SETTINGS_VERSION, false);

	// Dirty Fix for poor board design (Prototype Versions Only)
#ifdef SCL_PULLUP
	digitalWrite(SCL_PULLUP, HIGH);
	pinMode(SCL_PULLUP, OUTPUT);
#endif // SCL_LOOKUP

	// Setup Charger with default settings.
	charger.begin();
	charger.readFaults(false);
	charger.readFaults(true);

	// Initiate Display
	MYSPI.begin(SHARP_SCK, SHARP_MISO, SHARP_MOSI, SHARP_SS);
 
	sky_display.begin(MYSPI);
	sky_display.init();

	// Initialise Main Modem Code
	tracker.beginSerial(MODEM_STREAM);
  
	mate.init(tracker, MODEM_STREAM, DEBUG_STREAM, nav, preferences);

#ifdef BATT_VOLTAGE
	pinMode(BATT_VOLTAGE, INPUT);
	battLevelCalibration = preferences.getFloat("battCalibration", 537.5);
	battCal = preferences.getBool("battCal", false);
#endif // BATT_VOLTAGE

	// Setup i2C
	Wire.begin(SKYMATE_SDA, SKYMATE_SCL);

	DEBUG_PRINT(DAY); DEBUG_PRINT(","); DEBUG_PRINTLN(TIME);

	if (bmp.begin(0x76)) {
		pressure_available = true;

		// Set up oversampling and filter initialization
		bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
		bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
		bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
	} else {
		Serial.println("Could not BMP388");
	}
}

void loop()
{
	unsigned long loopMillis = millis();
	long updateLoopStatus = -1;
	long saraUpdateLoopStatus = NETWORK_IDLE;

	int16_t x1, y1;
	uint16_t w1, h1;
	char buff[20];
	char buff2[20];
	
	while (GPS_STREAM.available()) {
		gps.encode(GPS_STREAM.read());
	}
	
	// General state of opperation:
	switch (onState)
	{
	case ON_STATE_STARTUP:
		// This is the first time running
		if (digitalRead(ON_SW)) {
			// We are meant to be on
			onState = ON_STATE_RUNNING;
			startUpTime = millis();
		}
		else {
			// Just charging
			onState = ON_STATE_CHARGING;
		}
		break;
	case ON_STATE_CHARGING:
		if (!digitalRead(ON_SW)) {
			if ((loopMillis - prevMainLoopTime) > CHARGING_LOOP_INTERVAL) {
				prevMainLoopTime = loopMillis;
				chargeStatus = charger.read8(BQ24259_SYSTEM_STATUS_ADDR);
				if ((chargeStatus & BQ24259_PG_STATE) == BQ24259_PG_STATE) {
					// Charger display
					shutWait = false;

				}
				else {
					// Charger display
					DEBUG_PRINTLN("SHUTDOWN");
					charger.readFaults(true);
					//charger.readFaults(true);
					if (shutWait == true) {
						onState = ON_STATE_SHUTDOWN;
					}
					else {
						shutWait = true;
					}
				}

				// Calibrate Voltage.
#ifdef BATT_VOLTAGE
				DEBUG_PRINT("BATT VOLTAGE = "); DEBUG_PRINTLN(analogRead(BATT_VOLTAGE) / battLevelCalibration);
				if (((chargeStatus & BQ24259_CHARGE_STATE_MASK) >> BQ24259_CHARGE_STATE_POSN) == 3) { // If charged
					float temp = analogRead(BATT_VOLTAGE) / 4.2;
					if (temp > 400 && temp < 1000) {
						if (battCal != true) {
							battLevelCalibration = temp;

							battCal = true;
							preferences.putBool("battCal", battCal);
							preferences.putFloat("battCalibration", battLevelCalibration);

							DEBUG_PRINT("Batt Cal = "); DEBUG_PRINTLN(battLevelCalibration);

						}
						else if ((analogRead(BATT_VOLTAGE) / battLevelCalibration) > 4.25 || (analogRead(BATT_VOLTAGE) / battLevelCalibration) < 4.05) {
							battLevelCalibration = temp;
							battCal = true;
							preferences.putBool("battCal", battCal);
							preferences.putFloat("battCalibration", battLevelCalibration);

							DEBUG_PRINT("Batt Cal = "); DEBUG_PRINTLN(battLevelCalibration);
						}
					}
				}

				mate.battVoltage(analogRead(BATT_VOLTAGE) / battLevelCalibration);
#endif // BATT_VOLTAGE
			}
			sky_display.charging((chargeStatus & BQ24259_CHARGE_STATE_MASK) >> BQ24259_CHARGE_STATE_POSN, mate.getPilot(), mate.getTrackerID());
			sky_display.setFont();
			sky_display.setCursor(4, 158);
			sky_display.println(VERSION);
			sky_display.refresh();
		}
		else {
			onState = ON_STATE_RUNNING;
			DEBUG_PRINTLN("RUNNING");
			startUpTime = millis();
			mate.loopReset();
		}
		break;
	case ON_STATE_RUNNING:
		if (!digitalRead(ON_SW)) {
			onState = ON_STATE_RUNDOWN;
			shutdownTime = 0;
		}
		else {
			shutdownTime = 0;
			long initLoop = mate.initLoop(loopMillis);
			long mainLoop = 0;
			uint16_t trackerID = 0;

			if (gps.altitude.isValid() & gps.speed.isValid()) {
				if (gps.altitude.isUpdated() & (gps.altitude.age() < 500) & gps.speed.isUpdated() & (gps.speed.age() < 500)) {
					// Altitude and Speed are on separate NMEA strings so this ensures all data is new.

					currentPos.valid = true;
					currentPos.alt.value = gps.altitude.meters();
					currentPos.heading = gps.course.deg();
					currentPos.lat = gps.location.lat();
					currentPos.lon = gps.location.lng();
					currentPos.speed.value = gps.speed.knots();
					currentPos.time = convertToUnixTime(gps.time, gps.date);
					currentPos.sendStatus = SEND_STATUS_NEW;

					/*if (pressure_available) {
						if (bmp.performReading()) {
							currentPos.temperature = bmp.temperature;
							currentPos.pressure = bmp.pressure;
						}
					}
					else {
						currentPos.temperature = 99;
						currentPos.pressure = 0;
					}*/

					updateDisplay = true;

					mate.writePositionStore(currentPos);
					nav.loop(currentPos);
					DEBUG_PRINT(millis()); DEBUG_PRINT("\tGPS> Valid Position "); DEBUG_PRINTLN(currentPos.time);

					if (mate.getPostionStorePending() >= MAXIMUM_BACKLOG) {
						if (mate.setMainLoop(NETWORK_CHECK_NETWORK_STATUS)) prevMainLoopTime = loopMillis;
					}
				}
			}

			if (initLoop == INIT_COMPLETE) {
				if ((loopMillis - prevMainLoopTime) > MAIN_LOOP_INTERVAL) {
					if (mate.setMainLoop(NETWORK_CHECK_NETWORK_STATUS)) prevMainLoopTime = loopMillis;
				}
				mainLoop = mate.mainLoop(loopMillis);

				if (mainLoop == MAIN_LOOP_UPDATE_READY) {
					onState = ON_STATE_UPDATE;
					shutdownTime = 0;
				}
				else if (mainLoop == MAIN_LOOP_SARA_UPDATE_READY) {
					onState = ON_STATE_SARA_UPDATE;
					shutdownTime = 0;
				}
			}

			if (updateDisplay || ((loopMillis - displayRefreshTime) > 500)) {
				float battVoltage = 99;
				updateDisplay = false;
				displayRefreshTime = loopMillis;
#ifdef BATT_VOLTAGE
				battVoltage = analogRead(BATT_VOLTAGE) / battLevelCalibration;
				mate.battVoltage(battVoltage);
#endif // BATT_VOLTAGE
				if ((loopMillis - startUpTime) < 2000) {
					sky_display.fillRect(0, 0, 144, 168, WHITE);
					sky_display.setFont();
					sky_display.setCursor(3, 158);
					sky_display.print("BUILD: ");
					sky_display.print(__DATE__);
					sky_display.drawBitmap(7, 51, cloudLogo, 128, 64, BLACK);
					sky_display.refresh();
				}
				else if (((loopMillis - startUpTime) < 10000) || !gps.altitude.isValid()) {
					sky_display.fillRect(0, 0, 144, 168, WHITE);
					if (mate.getTrackerID() > 0) {
						sky_display.show_qr(mate.getTrackerID());
					}
					else {
						sky_display.drawBitmap(7, 51, cloudLogo, 128, 64, BLACK);
					}
					sky_display.refresh();
				}
				else {
					sky_display.fillRect(0, 0, 144, 168, WHITE);
					sky_display.deviceState(battVoltage, mate.getNetworkStatus());
					switch (nav.task.getTaskStatus())
					{
					case TASK_FREE_FLIGHT:
						sky_display.basicDisplay(currentPos, mate.getPref());
						break;
					case TASK_PRE_PRESTART:
						sky_display.basicDisplay(currentPos, mate.getPref());
						break;
					case TASK_PRESTART:
						sky_display.navigationDisplay(nav.relativeBearing(currentPos), nav.task.getCurrentTurnPoint());
						sky_display.basicNavigationDisplay(currentPos, nav.task.turnPointDistance(), mate.getPref());
						break;
					case TASK_ONTASK:
						sky_display.navigationDisplay(nav.relativeBearing(currentPos), nav.task.getCurrentTurnPoint());
						sky_display.basicNavigationDisplay(currentPos, nav.task.turnPointDistance(), mate.getPref());
						break;
					case TASK_FINISHED:
						sky_display.basicDisplay(currentPos, mate.getPref());
						break;
					default:
						break;
					}
					sky_display.refresh();
				}
			}
		}
		break;
	case ON_STATE_RUNDOWN:
		// shutdown counter
		if (shutdownTime == 0) {
			shutdownTime = loopMillis;
			debounce = false;
		}
		else if ((loopMillis - shutdownTime) > SHUTDOWN_TIME) {
			if (!digitalRead(ON_SW)) {
				if (!charger.readPowerGood()) {
					onState = ON_STATE_SHUTDOWN;
				}
				else {
					onState = ON_STATE_CHARGING;
				}
			}
			else {
				mate.loopReset();
				onState = ON_STATE_RUNNING;
			}
		}
		else if ((loopMillis - shutdownTime) > 500 & debounce == false) {
			debounce = true;
			mate.shutdown();
			mate.initLoop(loopMillis);
		}
		else if (debounce == false) {
			if (digitalRead(ON_SW)) {
				shutdownTime = 0;
				onState = ON_STATE_RUNNING;
				startUpTime = millis();
			}
		}
		else {
			sky_display.shutdownCounter(SHUTDOWN_TIME - (loopMillis - shutdownTime));
			mate.initLoop(loopMillis);
		}
		break;
	case ON_STATE_UPDATE:
		updateLoopStatus = mate.updateLoop(loopMillis);
		if (shutdownTime == 0) {
			shutdownTime = loopMillis;
		}

		if ((loopMillis - shutdownTime) > SHUTDOWN_TIME) {
			if (updateLoopStatus == UPDATE_SHUTDOWN) {
				mate.setUpdateLoop(UPDATE_CONNECT_WIFI);
			}
		}
		else if (updateLoopStatus == UPDATE_IDLE) {
			mate.setUpdateLoop(UPDATE_SHUTDOWN);
			mate.shutdown();
			mate.initLoop(loopMillis);
		}
		else {
			mate.initLoop(loopMillis);
		}

		sky_display.fillRect(0, 0, 144, 168, WHITE);
		sky_display.setFont(&FreeSans9pt7b);

		switch (updateLoopStatus)
		{
		case UPDATE_SHUTDOWN:
			sky_display.shutdownCounter(SHUTDOWN_TIME - (loopMillis - shutdownTime));
			break;
		case UPDATE_CONNECT_WIFI:
			strcpy(buff, "CONNECTING");
			strcpy(buff2, "TO WIFI");
			break;
		case UPDATE_CLIENT_CONNECT:
			strcpy(buff, "CONNECTING");
			strcpy(buff2, "TO SERVER");
			break;
		case UPDATE_READ_HEADER:
			strcpy(buff, "DOWNLOAD");
			strcpy(buff2, "FROM SERVER");
			break;
		case UPDATE_CHECK_BEGIN:
			strcpy(buff, "CHECKING");
			strcpy(buff2, "FILE");
			break;
		case UPDATE_DOWNLOAD_FILE:
			strcpy(buff, "UPDATING");
			strcpy(buff2, " ");
			break;
		case UPDATE_COMPLETE:
			strcpy(buff, "COMPLETE");
			strcpy(buff2, "RESTART");
			if (!digitalRead(ON_SW)) {
				ESP.restart();
			}
			break;
		case UPDATE_T_ERROR:
			strcpy(buff, "UPDATE ERROR");
			strcpy(buff2, "RESTART");
			if (!digitalRead(ON_SW)) {
				ESP.restart();
			}
			break;
		default:
			break;
		}

		sky_display.getTextBounds(buff, 0, 0, &x1, &y1, &w1, &h1);
		sky_display.setCursor(71 - (w1/2), 80);
		sky_display.println(buff);
		sky_display.getTextBounds(buff2, 0, 0, &x1, &y1, &w1, &h1);
		sky_display.setCursor(71 - (w1 / 2), 84 + h1);
		sky_display.println(buff2);
		sky_display.refresh();
		break;
	case ON_STATE_SARA_UPDATE:
		saraUpdateLoopStatus = mate.saraUpdateLoop(loopMillis);
		if (shutdownTime == 0) {
			shutdownTime = loopMillis;
		}

		if (saraUpdateLoopStatus == NETWORK_IDLE) {
			mate.setSaraUpdateLoop(NETWORK_CHECK_NETWORK_STATUS);
		}

		sky_display.fillRect(0, 0, 144, 168, WHITE);
		sky_display.setFont(&FreeSans9pt7b);

		switch (saraUpdateLoopStatus)
		{
		case NETWORK_CHECK_NETWORK_STATUS:
		case NETWORK_CHECK_REQUEST_QUALITY:
		case NETWORK_DISPLAY_SIGNAL:
		case FOTA_FIRMWARE_CHECK:
			strcpy(buff, "CHECKING");
			strcpy(buff2, "NETWORK");
			break;
		case FTP_SET_URL:
		case FTP_SET_PORT:
		case FTP_SET_USERNAME:
		case FTP_SET_PASSWORD:
		case FTP_LOGIN:
		case FTP_LOGIN_WAIT:
			strcpy(buff, "SETTING");
			strcpy(buff2, "FTP");
			break;
		case FTP_DOWNLOAD_UPDATE_1:
		case FTP_DOWNLOAD_UPDATE_1_FOTA:
		case FTP_DOWNLOAD_UPDATE_1_WAIT:
			strcpy(buff, "DOWNLOAD");
			strcpy(buff2, "FOTA UPDATE");
			break;
		case FTP_DOWNLOAD_UPDATE_1_FOTA_WAIT:
		case FTP_DOWNLOAD_UPDATE_1_FOTA_WAIT_2:
		case FTP_DOWNLOAD_UPDATE_1_AT:
		case FTP_DOWNLOAD_UPDATE_1_FOTA_CHECK:
			strcpy(buff, "INSTALLING");
			strcpy(buff2, "FOTA UPDATE");
			break;
		case FOTA_NOT_REQD:
			strcpy(buff, "NOT REQUIRED");
			strcpy(buff2, "RESTART");
			if (!digitalRead(ON_SW)) {
				ESP.restart();
			}
			break;
		case FTP_ERROR:
			strcpy(buff, "UPDATE ERROR");
			strcpy(buff2, "RESTART");
			if (!digitalRead(ON_SW)) {
				ESP.restart();
			}
			break;
		case FTP_DOWNLOAD_UPDATE_LOOP_COMPLETE:
			strcpy(buff, "FOTA COMPLETE");
			strcpy(buff2, "RESTART");
			if (!digitalRead(ON_SW)) {
				ESP.restart();
			}
			break;
		default:
			break;
		}

		sky_display.getTextBounds(buff, 0, 0, &x1, &y1, &w1, &h1);
		sky_display.setCursor(71 - (w1 / 2), 80);
		sky_display.println(buff);
		sky_display.getTextBounds(buff2, 0, 0, &x1, &y1, &w1, &h1);
		sky_display.setCursor(71 - (w1 / 2), 84 + h1);
		sky_display.println(buff2);
		sky_display.refresh();
		break;
	case ON_STATE_SHUTDOWN:
		sky_display.clearDisplay();
		charger.shutdown();
		delay(1000);
		onState = ON_STATE_STARTUP;
		break;
	default:
		break;
	}
}


time_t convertToUnixTime(TinyGPSTime time, TinyGPSDate date) {
	tm myTime;
	myTime.tm_sec = (byte)time.second();
	myTime.tm_min = (byte)time.minute();
	myTime.tm_hour = (byte)time.hour();
	myTime.tm_mday = (byte)date.day();
	myTime.tm_mon = (byte)date.month() - 1;
	myTime.tm_year = (byte)(date.year() - 1900);

	return mktime(&myTime);
}