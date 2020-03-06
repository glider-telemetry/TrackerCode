#ifndef _SKYMATE_h
#define _SKYMATE_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

#include <Sanderbuilt_SARA.h>
#include <Preferences.h>
#include <TinyGPS++.h>
#include <esp_wifi.h>
#include <WiFi.h>

#ifndef OTA_URL
#error "Set OTA_URL"
#define OTA_URL " "
#endif

#ifndef OTA_BIN
#error "Set OTA_BIN"
#define OTA_BIN "  "
#endif

#ifndef SERVER
#error "Set SERVER url"
#define SERVER " "
#endif

#ifndef BACKUP_IP
#error "Set BACKUP_IP"
#define BACKUP_IP " "
#endif

#ifndef PORT
#error "Set Socket Port"
#define PORT 1234
#endif

#define UPDATE_IDLE 0
#define UPDATE_SHUTDOWN 1
#define UPDATE_CONNECT_WIFI 2
#define UPDATE_CONNECT_WIFI_TIMEOUT 10000
#define UPDATE_CLIENT_CONNECT 3
#define UPDATE_CLIENT_CONNECT_TIMEOUT 5000
#define UPDATE_READ_HEADER 4
#define UPDATE_CHECK_BEGIN 5
#define UPDATE_DOWNLOAD_FILE 6

#define UPDATE_COMPLETE 21
#define UPDATE_COMPLETE_DELAY 3000
#define UPDATE_T_ERROR 99
#define UPDATE_ERROR_DELAY 3000

#define UPDATE_COM_START 0
#define UPDATE_COM_STAGE_1 1
#define UPDATE_COM_STAGE_2 2

#define INIT_DELAYED_POWER_ON -1
#define INIT_DELAYED_POWER_ON_WAIT 500
#define INIT_POWER_ON 0
#define INIT_DISPLAY_RESET_WAIT 300
#define INIT_DISPLAY_WAIT 500
#define INIT_POWER_ON_WAIT 500
#define INIT_RESET_ON 1
#define INIT_RESET_ON_WAIT 12000
#define INIT_RESET_OFF 2
#define INIT_RESET_OFF_WAIT 500
#define INIT_PWR_KEY_ON 3
#define INIT_PWR_KEY_ON_WAIT 500
#define INIT_PWR_KEY_OFF 4
#define INIT_PWR_KEY_OFF_WAIT 500
#define INIT_CHECK_POWER 5
#define INIT_CHECK_POWER_WAIT 1000
#define	INIT_CHECK_RESPONSE 6
#define INIT_CHECK_RESPONSE_WAIT 1000
#define INIT_CHECK_RESPONSE_COUNT 10
#define INIT_ECHO 7
#define INIT_CMEE 8
#define INIT_GET_IMEI 9
#define INIT_SET_NETWORK 11
#define INIT_SET_CREG 12
#define INIT_SET_UCGED 13
#define INIT_SET_APN 14
#define INIT_REQUEST_PHONE_NUMBER 15
#define INIT_REQUEST_FIRMWARE_VERSION 16
#define INIT_REQUEST_ATI 17
#define INIT_CHECK_FREQUENCY 18
#define INIT_GPS_SEND 19

#define INIT_CHECK_PROFILE 20
#define INIT_DISABLE_PROFILE 21
#define INIT_SET_MNO_PROFILE 22
#define MNO_PROFILE 4
#define INIT_PROFILE_RESET 23

#define INIT_CHECK_BANDMASK 30
#define INIT_DISABLE_BANDMASK 31
#define INIT_SET_BANDMASK 32
#define INIT_RESET_BANDMASK 33
#define BANDMASK "134217732‬"

#define INIT_SOFT_RESET 40
#define INIT_RESET_DELAY 41

#define INIT_COMPLETE 69
#define INIT_SHUTDOWN 90
#define INIT_CLOSE_SOCKETS 91
#define INIT_REQUEST_SHUTDOWN 92

#define INIT_SHUTDOWN_COMPLETE 99

#define INIT_COM_START 0
#define INIT_COM_STAGE_1 1
#define INIT_COM_STAGE_2 2
	
#define INIT_COMMAND_TIMEOUT 20000
#define INIT_COMMAND_MAX_COUNT 2

#define GPS_NO_POSITION 0
#define GPS_POSITION_AVAILABLE 1
#define GPS_IDLE 99

#define NETWORK_IDLE 0
#define NETWORK_CHECK_NETWORK_STATUS 1
#define CHECK_DIE_TEMPERATURE 2
#define NETWORK_CHECK_REQUEST_QUALITY 3
#define NETWORK_CHECK_REQUEST_EXTENDED_QUALITY 4
#define NETWORK_CHECK_UCGED 5
#define NETWORK_CHECK_RSRQS 6

#define NETWORK_SYSTEM_MODE 20
#define NETWORK_DISPLAY_SIGNAL 21
#define NETWORK_MNO_PROFILE 22
#define NETWORK_UPSD 23
#define NETWORK_COPS 24

#define NETWORK_READY 27

#define HTTP_MODE_REQUEST_TRACKER_ID 38
#define HTTP_MODE_SET_SSL 39
#define HTTP_MODE_REQUEST_GET 40
#define HTTP_MODE_SERVER_STATUS_COMPLETE 41
#define HTTP_MODE_POSITION_SET_URL 42
#define HTTP_MODE_POSITION_DELETE_POST_FILE 43
#define HTTP_MODE_SEND_POSITION 44
#define HTTP_MODE_POSITION_POST 45
#define HTTP_MODE_SEND_POSITION_COMPLETE 46
#define HTTP_MODE_TASK 47
#define HTTP_MODE_TASK_COMPLETE 48

#define HTTP_MODE_POST_SET_CID 60
#define HTTP_MODE_POST_SET_URL 61
#define HTTP_MODE_POST_SET_CONTENT 62
#define HTTP_MODE_POST_POSITION_SET_DATA 63
#define HTTP_MODE_POST_SEND_POSITION 64
#define HTTP_MODE_POST_SEND_POSITION_COMPLETE 65

#define ATTACH_GPRS 67
#define SOCKET_GET_IP 68
#define SOCKET_DEFAULT_IP 69
#define SOCKET_CREATE_SOCKET 70
#define SOCKET_CHECK_SOCKET 71
#define SOCKET_CONNECT_SOCKET 72
#define SOCKET_WRITE_DATA 73
#define SOCKET_CLEAR_SOCKET 74
#define SOCKET_GET_ERROR 75
#define SOCKET_LOOP_SUCCESS 76
#define SOCKET_LOOP_COMPLETE 77

#define SOCKET_WRITE_DATA_BYTES1 80
#define SOCKET_WRITE_DATA_BYTES2 81

#define NETWORK_ENGINEERING_MODE 90
#define NETWORK_NOT_READY 99

#define MAIN_LOOP_UPDATE_READY 109
#define MAIN_LOOP_SARA_UPDATE_READY 110

#define SARA_UPDATE_INIT 0
#define SARA_UPDATE_STAGE_1 1
#define SARA_UPDATE_STAGE_2 2
#define SARA_UPDATE_STAGE_3 3
#define SARA_UPDATE_STAGE_MAIN 4

#define FOTA_FIRMWARE_CHECK 200
#define FTP_SET_URL 201
#define FTP_SET_PORT 202
#define FTP_SET_USERNAME 203
#define FTP_SET_PASSWORD 204
#define FTP_LOGIN 205
#define FTP_LOGIN_WAIT 206

#define FTP_DOWNLOAD_UPDATE_1 210
#define FTP_DOWNLOAD_UPDATE_1_FOTA 211
#define FTP_DOWNLOAD_UPDATE_1_WAIT 212
#define FTP_DOWNLOAD_UPDATE_1_FOTA_WAIT 213
#define FTP_DOWNLOAD_UPDATE_1_FOTA_WAIT_2 214
#define FTP_DOWNLOAD_UPDATE_1_AT 215
#define FTP_DOWNLOAD_UPDATE_1_FOTA_CHECK 216

#define FOTA_NOT_REQD 296
#define FTP_DOWNLOAD_UPDATE_LOOP_COMPLETE 297

#define FTP_AT_CHECK 298
#define FTP_AT_CHECK_TIME 2000
#define FTP_ERROR 299

#define MAIN_COM_START 0
#define MAIN_COM_STAGE_1 1
#define MAIN_COM_STAGE_2 2

#define MAIN_COMMAND_TIMEOUT 200
#define MAIN_COMMAND_MAX_COUNT 2
#define MAIN_COMMAND_DOWNLOAD_TIMEOUT 3000

#define HTTP_READ_IDLE 0
#define HTTP_READ_WAIT 1
#define HTTP_READ_REQUEST 2
#define HTTP_DELETE_FILE 3
#define HTTP_POST_ERROR 99

#define HTTP_COM_START 0
#define HTTP_COM_STAGE_1 1
#define HTTP_COM_STAGE_2 2

#define HTTP_READ_COMMAND_TIMEOUT 1000
#define HTTP_RECEIVE_TIMEOUT 180000

#define HTTP_IDLE 0
#define HTTP_IN_PROGRESS 1
#define HTTP_REQUEST 2
#define HTTP_RECEIVE 3

#define HTTP_READ_AVAILABLE 1

#define SOCKET_READ_IDLE 0
#define SOCKET_READ_WAIT 1
#define SOCKET_READ_REQUEST 2
#define SOCKET_READ_ERROR 99

#define SOCKET_COM_START 0	
#define SOCKET_COM_STAGE_1 1
#define SOCKET_COM_STAGE_2 2

#define SOCKET_READ_COMMAND_TIMEOUT 1000
#define SOCKET_RECEIVE_TIMEOUT 10000
#define SOCKET_HARD_TIMEOUT 300000

#define MAIN_LOOP_NETWORK_STATUS_AVAILABLE 1
#define MAIN_LOOP_SIGNAL_QUALITY_AVAILABLE 2
#define MAIN_LOOP_EXTENDED_SIGNAL_QUALITY_AVAILABLE 3

#define SEND_STATUS_NEW 0
#define SEND_STATUS_PENDING 1
#define SEND_STATUS_DOWNLOADED 2
#define SEND_STATUS_SENT 3

#define TASK_FREE_FLIGHT -1
#define TASK_PRE_PRESTART 0
#define TASK_PRESTART 1
#define TASK_ONTASK 2
#define TASK_FINISHED 3

#define PREFERENCES_VERTICAL_KNOTS 0
#define PREFERENCES_VERTICAL_MS 1

#define PREFERENCES_HORIZONTAL_KNOTS 0
#define PREFERENCES_HORIZONTAL_MPH 1
#define PREFERENCES_HORIZONTAL_KPH 2

#define PREFERENCES_ALTITUDE_FEET 0
#define PREFERENCES_ALTITUDE_METRES 1

#define PREFERENCES_DISTANCE_KM 0
#define PREFERENCES_DISTANCE_NM 1
#define PREFERENCES_DISTANCE_MI 2

#define PREFERENCES_TASK_SPEED_KNOTS 0
#define PREFERENCES_TASK_SPEED_MPH 1
#define PREFERENCES_TASK_SPEED_KPH 2

#define TURNPOINT_NEAR_THRESHOLD 500 // 500m
#define TURNPOINT_MISSED_THRESHOLD 5000 // 5000m

#define HF_SAMPLES 600		// 10 minutes
#define LF_SAMPLES 720		// 600*5 = 50 min
#define LF_SAMPLE_RATE 5
#define MAXIMUM_SEND 10
#define HTTP_MAXIMUM_SEND 240
#define MAXIMUM_BACKLOG 5

struct SkyMatePreferences {
	uint8_t vertical = PREFERENCES_VERTICAL_KNOTS;
	uint8_t horizontal = PREFERENCES_HORIZONTAL_KNOTS;
	uint8_t altitude = PREFERENCES_ALTITUDE_FEET;
	uint8_t distance = PREFERENCES_DISTANCE_KM;
	uint8_t taskSpeed = PREFERENCES_TASK_SPEED_KPH;
};

struct SkyMatePilot {
	char firstName[20];
	char lastName[20];
	char rego[8];
	char gliderType[15];
};

struct SkyMateSpeed {
	double value;
	double knots() { return value; }
	double mph() { return _GPS_MPH_PER_KNOT * value; }
	double mps() { return _GPS_MPS_PER_KNOT * value; }
	double kmph() { return _GPS_KMPH_PER_KNOT * value; }
	void sprintKnots(char* text) { sprintf(text, "%0.0fkt", knots()); }
	void sprintMph(char* text) { sprintf(text, "%0.0fmph", mph()); }
	void sprintMps(char* text) { sprintf(text, "%0.0fmps", mps()); }
	void sprintkmph(char* text) { sprintf(text, "%0.0fkmph", kmph()); }
	void setKnots(double _value) { value = _value; }
	void setMph(double _value) { value = _value / _GPS_MPH_PER_KNOT; }
	void setMps(double _value) { value = _value / _GPS_MPS_PER_KNOT; }
	void setKmph(double _value) { value = _value / _GPS_KMPH_PER_KNOT; }

	SkyMateSpeed(double _value) { value = _value; }
	SkyMateSpeed() { value = 0; }
};

struct SkyMateAltitude {
	double value;
	double meters() { return value; }
	double feet() { return _GPS_FEET_PER_METER * value; }
	void setMeters(double _value) { value = _value; }
	void setFeet(double _value) { value = _value / _GPS_FEET_PER_METER; }
	void sprintMeters(char* text) { sprintf(text, "%0.0fm", meters()); }
	void sprintFeet(char* text) { sprintf(text, "%0.0fft", feet()); }

	SkyMateAltitude(double _value) { value = _value; }
	SkyMateAltitude() { value = 0; }
};

struct SkyMatePosition {
	bool valid = false;
	double lat;
	double lon;
	SkyMateAltitude alt;
	SkyMateSpeed speed;
	double heading;
	//double temperature = 0;
	//double pressure = 0;
	time_t time;
	uint8_t sendStatus = SEND_STATUS_SENT;
};

struct SkyMateNetwork {
	uint8_t netStatus = 0;
	uint8_t rssi = 255;
	float rsrq = 255;
	uint8_t rsrp = 255;
};

struct NavigationPoint {
	double lat;
	double lon;
};

struct NavigationTaskInfo {
	long timeIn = 0;
	long timeOut = 0;
	double nearest = -1;
	double length = -1;
	bool rounded = false;
	void resetTaskInfo() { timeIn = 0; timeOut = 0; nearest = -1; length = -1; rounded = false; }
};

struct NavigationTurnPoint {
	NavigationPoint turnPoint;
	NavigationTaskInfo taskInfo;
	long outerDiv;
	long innerDiv;
	char name[20];
	bool valid = false;
};

struct NavigationStartLine : NavigationTurnPoint {
	SkyMateAltitude maxAlt;
	SkyMateSpeed maxSpeed;
	long gateTime = 0;
	long course = 0;
	bool behindLine = false;
	SkyMatePosition startPos;
};

struct NavigationFinishLine : NavigationTurnPoint {
	SkyMateAltitude minAlt;
	bool finished = false;
	long course = 0;
	SkyMatePosition finishPos;
};

struct NavigationTask {
	friend class Navigation;
	friend class SkyMate;
public:
	NavigationTurnPoint getCurrentTurnPoint() const { return (taskStatus < TASK_ONTASK) ? startLine : ((currentTurnPoint < turnPointCount) ? turnPoint[currentTurnPoint] : finishLine); }
	bool isStarted() const { return (taskStatus > TASK_PRESTART); }
	bool isFinished() const { return (taskStatus == TASK_FINISHED); }
	double turnPointDistance() const { return distanceToTurnPoint; }
	int getTaskStatus() const { return taskStatus; }
	int8_t getTurnPointNumber() const { return currentTurnPoint; }
	bool isBehindStartLine() const { return startLine.behindLine; }
	bool isFinalLeg() const { return (currentTurnPoint == turnPointCount); }
	NavigationStartLine getStartLine() const { return startLine; }
	bool checkTaskValid(void);
	void wipeTask(void);
	void setTask(uint8_t _turnPointCount);
	void setTurnPoint(uint8_t point, char* name, double lat, double lon, double radius);
	void setStartLine(char* name, double lat, double lon, double maxAlt, double maxSpeed);
	void setFinishLine(char* name, double lat, double lon, double minAlt);
private:
	NavigationStartLine startLine;
	NavigationTurnPoint turnPoint[10];
	NavigationFinishLine finishLine;
	int taskStatus = TASK_PRESTART;
	int8_t currentTurnPoint = 0;
	int8_t turnPointCount = 0;
	double distanceToTurnPoint = -1;
	double distanceToPreviousPoint = -1;
	bool compTask = false;
	bool valid = false;
	bool emptyTask = true;
	double calculateOuterDiv(NavigationPoint point1, NavigationPoint point2, NavigationPoint point3);
	double calculateInnerDiv(double outerDiv);
	double insideWedge(NavigationTurnPoint turnPoint, double lat, double lon);
	double courseTo(double lat1, double long1, double lat2, double long2)
	{
		// returns course in degrees (North=0, West=270) from position 1 to position 2,
		// both specified as signed decimal-degrees latitude and longitude.
		// Because Earth is no exact sphere, calculated course may be off by a tiny fraction.
		// Courtesy of Maarten Lamers
		double dlon = radians(long2 - long1);
		lat1 = radians(lat1);
		lat2 = radians(lat2);
		double a1 = sin(dlon) * cos(lat2);
		double a2 = sin(lat1) * cos(lat2) * cos(dlon);
		a2 = cos(lat1) * sin(lat2) - a2;
		a2 = atan2(a1, a2);
		if (a2 < 0.0)
		{
			a2 += TWO_PI;
		}
		return degrees(a2);
	}
	double courseTo(NavigationPoint point1, NavigationPoint point2) {
		return courseTo(point1.lat, point1.lon, point2.lat, point2.lon);
	}
};

class Navigation {
	friend class SkyMate;
public:
	Navigation(TinyGPSPlus& gps);
	bool behindLine(SkyMatePosition currentPosition, NavigationPoint turnPoint, double course);
	void loop(SkyMatePosition currentPosition);
	double headingToTP(SkyMatePosition currentPosition);
	double relativeBearing(SkyMatePosition currentPosition);
	double distanceBetween(NavigationTurnPoint point1, NavigationTurnPoint point2);
	double distanceBetween(NavigationTurnPoint point1, SkyMatePosition point2);
	double distanceBetween(SkyMatePosition point1, NavigationTurnPoint point2);
	NavigationTask task;
private:
	TinyGPSPlus* myGPS;
};

class SkyMate {
public:
	SkyMate(int8_t, int8_t, int8_t, int8_t);

	void init(Sanderbuilt_SARA &tracker, Stream &modem, Stream &debug, Navigation &nav, Preferences &_preferences );
	void loopReset(void);
	//void setInitLoop(long initLoop);
	long initLoop(unsigned long currentMillis);
	void shutdown(void);
	bool sendInitCommand(uint8_t readLoopStatus, unsigned long currentMillis, long nextCommand);
	bool sendInitCommand(uint8_t readLoopStatus, unsigned long currentMillis, long nextCommand, long errorCommand);
	void getImei(char *_imei);


	bool setMainLoop(uint8_t);
	void setUpdateLoop(long loop) { updateLoopID = loop; }
	void setSaraUpdateLoop(long loop) { saraUpdateLoopID = loop; }
	void battVoltage(float);
	uint8_t mainLoop(unsigned long currentMillis);
	uint16_t getTrackerID(void);

	long updateLoop(unsigned long currentMillis);
	long saraUpdateLoop(unsigned long currentMillis);

	uint8_t returnNetStatus()   const { return netStatus; }
	uint8_t returnSignalPower()   const { return signalPower; }
	uint8_t returnSignalQuality()   const { return signalQuality; }
	uint8_t returnRSRQ()   const { return rsrq; }
	uint8_t returnRSRP()   const { return rsrp; }

	unsigned long mainPrevMillis = 0;
	unsigned long httpReadPrevMillis = 0;
	unsigned long socketReadPrevMillis = 0;
	unsigned long transmissionTime = 0;

	void writePositionStore(SkyMatePosition position);
	uint16_t getPostionStorePending(void);
	bool getUnwrittenPos(SkyMatePosition*position, uint8_t);

	SkyMatePreferences getPref() { return _pref; }
	SkyMatePilot getPilot() { return _pilot; }

	SkyMateNetwork getNetworkStatus() { return _net; }

protected:
	int8_t _sara_toggle = -1;
	int8_t _sara_enable = -1;
	int8_t _sara_reset = -1;
	int8_t _sara_power = -1;

	long initLoopID = INIT_POWER_ON;
	long initLoopStatus = INIT_COM_START;

	long updateLoopID = UPDATE_CONNECT_WIFI;
	long updateLoopStatus = UPDATE_COM_START;
	unsigned long updatePrevMillis = 0;

	void updateLoopNextStep(long loopID, unsigned long currentMillis);
	int readUpdateLine(void);

	bool firstSend = true;
	bool resetLoop = true;

	unsigned long initPrevMillis = 0;
	long resetCounter = 0;
	char imei[16];
	char firmware[30];
	char phoneNumber[15];
	int MNOProfile = -1;
	bool profileSet = false;

	bool bandSet = false;
	char bandBitmask[15];

	long mainLoopID = NETWORK_IDLE;
	long mainLoopStatus = MAIN_COM_START;
	long httpReadID = HTTP_READ_IDLE;
	long httpReadStatus = HTTP_COM_START;
	long socketReadID = SOCKET_READ_IDLE;
	long socketReadStatus = SOCKET_COM_START;

	long socketErrorCounter = 0;

	// Sara Update Loop Variables;
	long saraUpdateLoopID = NETWORK_IDLE;
	long saraUpdateLoopStatus = MAIN_COM_START;
	long saraUpdateTopStatus = SARA_UPDATE_STAGE_1;
	long saraUpdateStatus = SARA_UPDATE_INIT;
	unsigned long saraUpdatePrevMillis = 0;

	uint8_t socket = 99;
	uint16_t socketReadChars = 0;
	bool ipSet = false;
	char ipAddress[20];

	char replybuffer[500];
	uint8_t netStatus = 0;
	uint8_t signalPower = 99;
	uint8_t signalQuality = 99;
	uint8_t rsrq = 99;
	uint8_t rsrp = 99;

	int dieTemperature = 0;

	char rsrpCH[10];
	char rsrqCH[10];


	float _voltage = 99;

	char preLine[250];

	bool url_set = false;

	char path[100];

	uint16_t trackerID = 0;

	SkyMatePosition hfSamples[HF_SAMPLES];
	SkyMatePosition lfSamples[LF_SAMPLES];

	uint16_t hfSampleCounter = 0;
	uint16_t lfSampleCounter = 0;
	unsigned long calculateMessageLength(bool);
	unsigned long calculateMessageLength(bool, int);
	void sendMessage();
	void sendMessage(int);
	void clearPositionStatus(int8_t status); 
	void writePositionStatus(int8_t status, int8_t newStatus = SEND_STATUS_NEW);
	void readServerSocketResponse();
	void readServerHTTPResponse();

	double distanceBetween(double lat1, double long1, double lat2, double long2);

	// Update Variables
	// Variables to validate
	// response from S3
	long contentLength = 0;
	bool isValidContentType = false;

	// Your SSID and PSWD that the chip needs
	// to connect to
	char SSID[30];
	char PSWD[30];

	// S3 Bucket Config
	const char host[25] = OTA_URL; // Host => bucket-name.s3.region.amazonaws.com
	int port = 80; // Non https. For HTTPS 443. As of today, HTTPS doesn't work.
	const char bin[31] = OTA_BIN; // bin file name with a slash in front.

	SkyMatePreferences _pref;
	SkyMatePilot _pilot;

	Preferences* preferences;
	Stream *modemStream;
	Stream *debugStream;
	Navigation* myNav;

	Sanderbuilt_SARA *myTracker;

	WiFiClient client;
private:
	bool sendMainCommand(uint8_t readLoopStatus, unsigned long currentMillis, uint8_t messageCompare, long nextCommand);
	bool sendMainCommand(uint8_t readLoopStatus, unsigned long currentMillis, uint8_t messageCompare, long nextCommand, unsigned long timeOut);
	bool sendMainCommand(uint8_t readLoopStatus, unsigned long currentMillis, uint8_t messageCompare, long nextCommand, unsigned long timeOut, long errorCommand);

	bool sendSaraUpdateCommand(uint8_t readLoopStatus, unsigned long currentMillis, uint8_t messageCompare, long nextCommand);
	bool sendSaraUpdateCommand(uint8_t readLoopStatus, unsigned long currentMillis, uint8_t messageCompare, long nextCommand, unsigned long timeOut);
	bool sendSaraUpdateCommand(uint8_t readLoopStatus, unsigned long currentMillis, uint8_t messageCompare, long nextCommand, unsigned long timeOut, long errorCommand);

	bool sendCommand(long* loopStatus, long* loopID, unsigned long* prevMillis, uint8_t readLoopStatus, unsigned long currentMillis, uint8_t messageCompare, long nextCommand, unsigned long timeOut, long errorCommand);

	void writeNetStatus(uint8_t);
	void writeRSSI(uint8_t);
	void writeRSRP(uint8_t);
	void writeRSRQ(uint8_t);

	String getHeaderValue(String header, String headerName) {
		return header.substring(strlen(headerName.c_str()));
	}

	SkyMateNetwork _net;

	void createTrackerIDURL();
	void createPostLine(SkyMatePosition position, char *postLine);
	void createJPosition(SkyMatePosition position, char *postLine);

	boolean checkString(char* toreply);
	boolean parseString(char* toreply, double* v, char divider, uint8_t index);
	boolean parseString(char *toreply, uint16_t *v, char divider, uint8_t index);
	boolean parseString(char *toreply, int *v, char divider, uint8_t index);
	boolean parseString(char* toreply, long *v, char divider, uint8_t index);
	boolean parseString(char* toreply, char *v, char divider, uint8_t index);
	boolean parseReplyQuoted(char* toreply, char* v, int maxlen, char divider, uint8_t index);
	
};
#endif