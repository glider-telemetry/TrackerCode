// SkyMate_Display.h

#ifndef _SKYMATE_DISPLAY_h
#define _SKYMATE_DISPLAY_h

#include <Adafruit_GFX.h>
//#include <Adafruit_SharpMem.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans24pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSerifBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeSerif9pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <qrcode.h>
#include "bitmaps.h"
#include <SPI.h>

#include "SkyMate.h"

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#define AMPLITUDE 300           // Depending on your audio source level, you may need to increase this value

#define BLACK 0
#define WHITE 1

typedef struct
{
	int heading_disp;
	char* waypoint_disp;
	char* abbr_waypoint_disp;
	char* distance_disp;
	char* gate_disp;
	char* alt_disp;
	char* speed_disp;
	int alt_diff;
	int speed_diff;
	char* place_disp;
	char* xc_speed;
} display_data;

class SkyMate_Display : public Adafruit_GFX
{
protected:
	uint8_t _ss, _clk, _mosi, _miso;
	uint32_t _sharpmem_vcom;

	void sendbyte(uint8_t data);
	void sendbyteLSB(uint8_t data);

	void beginTransaction(void);
	void endTransaction(void);

	char* config_url = "https://app.skymate.com.au/trackers/track_set/";
	byte peak[8] = {0,0,0,0,0,0,0,0};
	int trackerID = 0;
	void displayBand(int band, int dsize);
	void drawLargeBattery(uint16_t width);
	void formatCountdownTimer(char* text, long countdown);
	void formatDistance(char* text, double distance);

	SPIClass *mySPI;
public:
	//SkyMate_Display(uint8_t clk, uint8_t mosi, uint8_t miso, uint8_t ss, uint16_t w = 96, uint16_t h = 96);
	SkyMate_Display(uint8_t ss, uint16_t w = 96, uint16_t h = 96);
	void begin(SPIClass &port);
	void drawPixel(int16_t x, int16_t y, uint16_t color);
	uint8_t getPixel(uint16_t x, uint16_t y);
	void clearDisplay();
	//void clearBuffer();
	void refresh(void);

	void init(void);
	void show_qr(uint16_t tracker_id);
	void setTrackerID(uint16_t tracker_id);
	void deviceState(float voltage, SkyMateNetwork net);
	void deviceState(float voltage, int rssi_level);
	void navigationDisplay(double current_heading, NavigationTurnPoint turnPoint);
	void navigationDisplay(double current_heading, char* current_waypoint, char* current_waypoint_abbr);
	void startSummary(SkyMateAltitude maxAlt, SkyMateSpeed maxSpeed, SkyMatePosition currentPosition);
	void preRace(NavigationStartLine startLine, double distance, long countdown, SkyMatePosition currentPosition);
	void onTask(uint8_t placing, double distance, SkyMatePosition currentPosition, SkyMatePreferences pref);
	void taskComplete(uint8_t placing);
	void basicDisplay(SkyMatePosition currentPosition, SkyMatePreferences pref);
	void basicNavigationDisplay(SkyMatePosition currentPosition, double distance, SkyMatePreferences pref);
	void displaySignal(uint8_t signalPower, uint8_t rsrq, uint8_t rsrp);
	void displayAudio(double *ptr, uint16_t length);
	void charging(uint8_t chargeStatus, SkyMatePilot pilot, uint16_t trackerID);
	void shutdownCounter(unsigned long shutdownTimer);
};

#endif

