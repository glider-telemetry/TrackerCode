// 
// 
// 
#include "SkyMate_Display.h"
#define DEBUG
#include "Debug.h"
//#include <qrcode.h>

#ifndef _swap_int16_t
#define _swap_int16_t(a, b) { int16_t t = a; a = b; b = t; }
#endif
#ifndef _swap_uint16_t
#define _swap_uint16_t(a, b) { uint16_t t = a; a = b; b = t; }
#endif

/**************************************************************************
Sharp Memory Display Connector
-----------------------------------------------------------------------
Pin   Function        Notes
===   ==============  ===============================
1   VIN             3.3-5.0V (into LDO supply)
2   3V3             3.3V out
3   GND
4   SCLK            Serial Clock
5   MOSI            Serial Data Input
6   CS              Serial Chip Select
9   EXTMODE         COM Inversion Select (Low = SW clock/serial)
7   EXTCOMIN        External COM Inversion Signal
8   DISP            Display On(High)/Off(Low)
**************************************************************************/

#define SHARPMEM_BIT_WRITECMD   (0x80)
#define SHARPMEM_BIT_VCOM       (0x40)
#define SHARPMEM_BIT_CLEAR      (0x20)
#define TOGGLE_VCOM             do { _sharpmem_vcom = _sharpmem_vcom ? 0x00 : SHARPMEM_BIT_VCOM; } while(0);

static const int spiClk = 1000000;

#define SHARPMEM_LCDWIDTH 144
#define SHARPMEM_LCDHEIGHT 168

byte sharpmem_buffer[(SHARPMEM_LCDWIDTH * SHARPMEM_LCDHEIGHT) / 8];

SkyMate_Display::SkyMate_Display(uint8_t ss,uint16_t width, uint16_t height) :
	Adafruit_GFX(width, height) {
	_ss = ss;
	digitalWrite(_ss, LOW);
	pinMode(_ss, OUTPUT);
	}

void SkyMate_Display::begin(SPIClass &port) {
	setRotation(0);
	mySPI = &port;

	//mySPI->setClockDivider(84);    // SPI clock frequency must be < 1 MHz
	//mySPI->setDataMode(SPI_MODE0);  // CPOL = 0, CPHA = 0
	//mySPI->setBitOrder(MSBFIRST);   // LSB comes first
}

void SkyMate_Display::init()
{
	//begin();
	clearDisplay();
	setRotation(2);
	setTextSize(1); // use SystemFont in "default" text area
	setTextColor(BLACK);
	setFont(&FreeSans12pt7b);
}

void SkyMate_Display::show_qr(uint16_t tracker_id) {
	// START SHOW QR
	QRCode qrcode;
	char trackset[60];
	char skymate_id[60];
	int mult_x;
	int mult_y = 1;
	int offset_x = 28;
	int offset_y = 40;
	int16_t x1, y1;
	uint16_t w1, h1;

	uint8_t qrcodeData[108];
	sprintf(trackset, "%s%d", config_url, tracker_id);
	qrcode_initText(&qrcode, qrcodeData, 3, 0, trackset);
	for (uint8_t y = 0; y < qrcode.size; y++) {
		mult_x = 1;
		for (uint8_t x = 0; x < qrcode.size; x++) {
			if (qrcode_getModule(&qrcode, x, y)) {
				fillRect((x + mult_x + offset_x), (y + mult_y + offset_y), 3, 3, BLACK);
			}
			mult_x = mult_x + 2;
		}
		mult_y = mult_y + 2;
	}
	sprintf(skymate_id, "%d", tracker_id);
	setTextColor(BLACK);
	setFont(&FreeSansBold12pt7b);
	setTextSize(1);
	getTextBounds("SkyMate", 0, 0, &x1, &y1, &w1, &h1);
	setCursor((144 / 2) - (w1 / 2), 25);
	print("SkyMate");
	setFont(&FreeSansBold18pt7b);
	getTextBounds(skymate_id, 0, 0, &x1, &y1, &w1, &h1);
	setCursor((144 / 2) - (w1 / 2), 157);
	print(skymate_id);
	// END SHOW QR
}

void SkyMate_Display::setTrackerID(uint16_t tracker_id) {
	trackerID = tracker_id;
}

void SkyMate_Display::deviceState(float voltage, SkyMateNetwork net) {
	deviceState(voltage, net.rsrq);
}

void SkyMate_Display::deviceState(float voltage, int rssi_level) {
	if (voltage < 3.5) {
		drawBitmap(133, 3, batt0, 9, 15, BLACK);
	}
	else if (voltage < 3.5) {
		drawBitmap(133, 3, batt1, 9, 15, BLACK);
	}
	else if (voltage < 3.7) {
		drawBitmap(133, 3, batt2, 9, 15, BLACK);
	}
	else if (voltage < 3.8) {
		drawBitmap(133, 3, batt3, 9, 15, BLACK);
	}
	else {
		drawBitmap(133, 3, batt4, 9, 15, BLACK);
	}


	if (rssi_level > 20) {
		drawBitmap(2, 3, ant0, 9, 15, BLACK);
	}
	else if (rssi_level > 15) {
		drawBitmap(2, 3, ant1, 9, 15, BLACK);
	}
	else if (rssi_level > 11) {
		drawBitmap(2, 3, ant2, 9, 15, BLACK);
	}
	else if (rssi_level > 7) {
		drawBitmap(2, 3, ant3, 9, 15, BLACK);
	}
	else {
		drawBitmap(2, 3, ant4, 9, 15, BLACK);
	}
}

void SkyMate_Display::navigationDisplay(double current_heading, NavigationTurnPoint turnPoint) {
	navigationDisplay(current_heading, turnPoint.name, turnPoint.name);
}

void SkyMate_Display::navigationDisplay(double current_heading, char* current_waypoint, char* current_waypoint_abbr) { // HEADER FOR EVERY DISPLAY MODE - Arrows, waypoint name 
	int16_t x1, y1;
	uint16_t w1, h1;
	setFont(&FreeSerifBold18pt7b);

	int cent = 9;

	if (current_heading < -180) {
		current_heading += 360;
	}
	else if (current_heading > 180) {
		current_heading -= 360;
	}
	
	
	if (current_heading < -45) {
		// less than -45
		drawBitmap(cent, 4, nav_left5, 126, 15, BLACK);
	}
	else if (current_heading < -35) {
		// less than -45
		drawBitmap(cent, 4, nav_left4, 126, 15, BLACK);
	}
	else if (current_heading < -25) {
		// less than -45
		drawBitmap(cent, 4, nav_left3, 126, 15, BLACK);
	}
	else if (current_heading < -15) {
		// between -45 and -20
		drawBitmap(cent, 4, nav_left2, 126, 15, BLACK);
	}
	else if (current_heading < -5) {
		// between -20 and -5
		drawBitmap(cent, 4, nav_left1, 126, 15, BLACK);
	}
	else if (current_heading < 5) {
		// between -5 and 5
		drawBitmap(cent, 4, nav_centre, 126, 15, BLACK);
	}
	else if (current_heading < 15) {
		// between 5 and 20
		drawBitmap(cent, 4, nav_right1, 126, 15, BLACK);
	}
	else if (current_heading < 25) {
		// between 20 and 45
		drawBitmap(cent, 4, nav_right2, 126, 15, BLACK);
	}
	else if (current_heading < 35) {
		// more than 45
		drawBitmap(cent, 4, nav_right3, 126, 15, BLACK);
	}
	else if (current_heading < 45) {
		// more than 45
		drawBitmap(cent, 4, nav_right4, 126, 15, BLACK);
	}
	else if (current_heading > 45) {
		// more than 45
		drawBitmap(cent, 4, nav_right5, 126, 15, BLACK);
	}

	drawRect(0, 44, 144, 2, BLACK);

	char* waypoint_string;
	char* place_suffix;
	setTextColor(BLACK);
	setFont(&FreeSans12pt7b);
	setTextSize(1);
	getTextBounds(current_waypoint, 0, 0, &x1, &y1, &w1, &h1);
	if (w1 > 140) {
		waypoint_string = current_waypoint_abbr;
	}
	else {
		waypoint_string = current_waypoint;
	}
	getTextBounds(waypoint_string, 0, 0, &x1, &y1, &w1, &h1);
	setCursor((144 / 2) - (w1 / 2), 40);
	println(waypoint_string);
}

void SkyMate_Display::startSummary(SkyMateAltitude maxAlt, SkyMateSpeed maxSpeed, SkyMatePosition currentPosition) {
	int16_t x1, y1;
	uint16_t w1, h1;
	char alt_state[20], speed_state[20];

	// draw framework
	drawLine(0, 64, 144, 64, BLACK);
	drawLine(0, 115, 144, 115, BLACK);
	drawLine(105, 64, 105, 168, BLACK);
	setFont(&FreeMonoBold18pt7b);
	// ALTITUDE DIFFERENCE
	SkyMateAltitude altitudeDelta(currentPosition.alt.value - maxAlt.value);
	if (altitudeDelta.value >= 0) {
		// over altitude
		drawBitmap(110, 74, small_cross, 31, 31, BLACK);
		sprintf(alt_state, "+%d", altitudeDelta.feet());
		getTextBounds(alt_state, 0, 0, &x1, &y1, &w1, &h1);
		setCursor((104 / 2) - (w1 / 2), 92);
		print(alt_state);
	}
	else {
		drawBitmap(110, 74, small_tick, 31, 31, BLACK);
		sprintf(alt_state, "%d", altitudeDelta.feet());
		getTextBounds(alt_state, 0, 0, &x1, &y1, &w1, &h1);
		setCursor((104 / 2) - (w1 / 2), 92);
		print(alt_state);
	}
	setCursor(60, 110);
	setFont(&FreeSerif9pt7b);
	print("FEET");
	// SPEED DIFFERENCE
	SkyMateSpeed speedDelta(currentPosition.speed.value - maxSpeed.value);
	if (speedDelta.value >= 0) {
		// over speed
		drawBitmap(110, 125, small_cross, 31, 31, BLACK);
		sprintf(speed_state, "+%d", speedDelta.knots());
		setFont(&FreeMonoBold18pt7b);
		getTextBounds(speed_state, 0, 0, &x1, &y1, &w1, &h1);
		setCursor((104 / 2) - (w1 / 2), 144);
		print(speed_state);
	}
	else {
		drawBitmap(110, 125, small_tick, 31, 31, BLACK);
		sprintf(speed_state, "%d", speedDelta.knots());
		setFont(&FreeMonoBold18pt7b);
		getTextBounds(speed_state, 0, 0, &x1, &y1, &w1, &h1);
		setCursor((104 / 2) - (w1 / 2), 144);
		print(speed_state);
	}
	setCursor(42, 165);
	setFont(&FreeSerif9pt7b);
	print("KNOTS");
	//refresh();
}

void SkyMate_Display::preRace(NavigationStartLine startLine, double distance, long countdown, SkyMatePosition currentPosition) {
	int16_t x1, y1;
	uint16_t w1, h1;
	char speed_state[20];
	char alt_state[20];
	char dist_state[10];
	char gate_state[10];
	char gps_alt_state[10];
	char gsp_state[10];

	// first row
	drawLine(0, 64, 144, 64, BLACK);
	// second row
	drawLine(0, 98, 144, 98, BLACK);
	// third row
	drawLine(0, 135, 144, 135, BLACK);
	// vertical separator
	drawLine(80, 64, 80, 168, BLACK);
	// draw km to start
	formatDistance(dist_state, distance/1000);
	getTextBounds(dist_state, 0, 0, &x1, &y1, &w1, &h1);
	setCursor((78 / 2) - (w1 / 2), 90);
	print(dist_state);
	// draw time until gate opens
	formatCountdownTimer(gate_state, countdown);
	getTextBounds(gate_state, 0, 0, &x1, &y1, &w1, &h1);
	setCursor((64 / 2) - (w1 / 2) + 79, 90);
	print(gate_state);
	// draw current gps altitude
	currentPosition.alt.sprintFeet(gps_alt_state);
	getTextBounds(gps_alt_state, 0, 0, &x1, &y1, &w1, &h1);
	setCursor((78 / 2) - (w1 / 2), 125);
	print(gps_alt_state);
	// draw current groundspeed
	currentPosition.speed.sprintKnots(gsp_state);
	getTextBounds(gsp_state, 0, 0, &x1, &y1, &w1, &h1);
	setCursor((64 / 2) - (w1 / 2) + 79, 125);
	print(gsp_state);
	// draw altitude difference
	SkyMateAltitude altitudeDelta(currentPosition.alt.value - startLine.maxAlt.value);
	if (altitudeDelta.value >= 0) {
		// currently over alt - invert colours
		sprintf(alt_state, "+%0.0f", altitudeDelta.feet());
		fillRect(0, 135, 80, 35, BLACK);
		getTextBounds(alt_state, 0, 0, &x1, &y1, &w1, &h1);
		setCursor((78 / 2) - (w1 / 2), 160);
		setTextColor(WHITE, BLACK);
		print(alt_state);
		setTextColor(BLACK);
	}
	else {
		sprintf(alt_state, "%0.0f", altitudeDelta.feet());
		getTextBounds(alt_state, 0, 0, &x1, &y1, &w1, &h1);
		setCursor((78 / 2) - (w1 / 2), 160);
		print(alt_state);
	}
	// draw speed difference
	SkyMateSpeed speedDelta(currentPosition.speed.value - startLine.maxSpeed.value);
	if (speedDelta.value >= 0) {
		// currently overspeed - invert colours
		sprintf(speed_state, "+%0.0f", speedDelta.knots());
		fillRect(80, 135, 144, 35, BLACK);
		getTextBounds(speed_state, 0, 0, &x1, &y1, &w1, &h1);
		setCursor((64 / 2) - (w1 / 2) + 79, 160);
		setTextColor(WHITE, BLACK);
		print(speed_state);
		setTextColor(BLACK);
	}
	else {
		sprintf(speed_state, "%0.0f", speedDelta.knots());
		getTextBounds(speed_state, 0, 0, &x1, &y1, &w1, &h1);
		setCursor((64 / 2) - (w1 / 2) + 79, 160);
		print(speed_state);
	}
	refresh();
}

void SkyMate_Display::onTask(uint8_t placing, double distance, SkyMatePosition currentPosition, SkyMatePreferences pref) {
	int16_t x1, y1;
	uint16_t w1, h1;
	char distance_display[20], place_display[20], xc_speed_display[20];
	
	char* place_suffix;
	if (placing == 1) {
		place_suffix = "st";
	}
	else if (placing == 2) {
		place_suffix = "nd";
	}
	else if (placing == 3) {
		place_suffix = "rd";
	}
	else {
		place_suffix = "th";
	}
	drawLine(0, 64, 144, 64, BLACK);
	drawLine(0, 100, 144, 100, BLACK);
	drawLine(0, 134, 144, 134, BLACK);
	// draw km to start
	formatDistance(distance_display, distance/1000);
	setFont(&FreeSansBold18pt7b);
	getTextBounds(distance_display, 0, 0, &x1, &y1, &w1, &h1);
	setCursor((144 / 2) - (w1 / 2), 92);
	print(distance_display);
	// draw race position
	sprintf(place_display, "%i%s", placing, place_suffix);
	setFont(&FreeSansBold18pt7b);
	getTextBounds(place_display, 0, 0, &x1, &y1, &w1, &h1);
	setCursor((144 / 2) - (w1 / 2), 127);
	print(place_display);
	// draw current speed
	setFont(&FreeSansBold12pt7b);
	currentPosition.speed.sprintKnots(xc_speed_display);
	getTextBounds(xc_speed_display, 0, 0, &x1, &y1, &w1, &h1);
	setCursor((144 / 2) - (w1 / 2), 158);
	print(xc_speed_display);
}

void SkyMate_Display::taskComplete(uint8_t placing) {
	int16_t x1, y1;
	uint16_t w1, h1;
	char placed[20];
	
	char* place_suffix;

	if (placing == 1) {
		place_suffix = "st";
	}
	else if (placing == 2) {
		place_suffix = "nd";
	}
	else if (placing == 3) {
		place_suffix = "rd";
	}
	else {
		place_suffix = "th";
	}

	drawLine(0, 64, 144, 64, BLACK);
	// draw current race position
	setFont(&FreeMonoBold12pt7b);
	getTextBounds("race", 0, 0, &x1, &y1, &w1, &h1);
	setCursor((144 / 2) - (w1 / 2), 80);
	print("race");
	getTextBounds("completed", 0, 0, &x1, &y1, &w1, &h1);
	setCursor((144 / 2) - (w1 / 2), 102);
	print("completed");
	sprintf(placed, "%i%s", placing, place_suffix);
	setFont(&FreeMonoBold24pt7b);
	getTextBounds(placed, 0, 0, &x1, &y1, &w1, &h1);
	setCursor((144 / 2) - (w1 / 2), 145);
	print(placed);
}

void SkyMate_Display::basicNavigationDisplay(SkyMatePosition currentPosition, double distance, SkyMatePreferences pref) {
	int16_t x1, y1;
	uint16_t w1, h1;
	char buff[20];
	char unitbuff[4];

	fillRect(0, 105, 144, 2, BLACK);
	fillRect(71, 44, 2, 61, BLACK);

	setTextColor(BLACK);

	switch (pref.horizontal)
	{
	case PREFERENCES_HORIZONTAL_KNOTS:
		strcpy(unitbuff, "kt");
		dtostrf(currentPosition.speed.knots(), 1, 0, buff);
		break;
	case PREFERENCES_HORIZONTAL_KPH:
		strcpy(unitbuff, "kph");
		dtostrf(currentPosition.speed.kmph(), 1, 0, buff);
		break;
	case PREFERENCES_HORIZONTAL_MPH:
		strcpy(unitbuff, "mph");
		dtostrf(currentPosition.speed.mph(), 1, 0, buff);
		break;
	}

	setFont(&FreeSans9pt7b);
	getTextBounds(unitbuff, 0, 0, &x1, &y1, &w1, &h1);
	setCursor(68-w1, 102);
	println(unitbuff);

	setFont(&FreeSansBold18pt7b);
	getTextBounds(buff, 0, 0, &x1, &y1, &w1, &h1);
	setCursor(35 - (w1 / 2), 87);
	println(buff);

	double temp_dist;
	switch (pref.distance)
	{
	case PREFERENCES_DISTANCE_KM:
		strcpy(unitbuff, "km");
		temp_dist = distance / 1000;
		break;
	case PREFERENCES_DISTANCE_MI:
		strcpy(unitbuff, "mi");
		temp_dist = distance / 1609.344;
		break;
	case PREFERENCES_DISTANCE_NM:
		strcpy(unitbuff, "nm");
		temp_dist = distance / 1852;
		break;
	}

	if (temp_dist < 1) {
		dtostrf(temp_dist, 1, 2, buff);
	}
	else if (temp_dist < 10) {
		dtostrf(temp_dist, 1, 1, buff);
	}
	else if (temp_dist < 999) {
		dtostrf(temp_dist, 1, 0, buff);
	}
	else {
		strcpy(buff, "---");
	}

	setFont(&FreeSans9pt7b);
	getTextBounds(unitbuff, 0, 0, &x1, &y1, &w1, &h1);
	setCursor(140-w1, 102);
	println(unitbuff);

	setFont(&FreeSansBold18pt7b);
	getTextBounds(buff, 0, 0, &x1, &y1, &w1, &h1);
	setCursor(107 - (w1 / 2), 87);
	println(buff);

	switch (pref.altitude) {
	case PREFERENCES_ALTITUDE_FEET:
		strcpy(unitbuff, "ft");
		dtostrf(currentPosition.alt.feet(), 1, 0, buff);
		break;
	case PREFERENCES_ALTITUDE_METRES:
		strcpy(unitbuff, "m");
		dtostrf(currentPosition.alt.meters(), 1, 0, buff);
		break;
	}

	setFont(&FreeSans9pt7b);
	getTextBounds(unitbuff, 0, 0, &x1, &y1, &w1, &h1);
	setCursor(140-w1, 164);
	print(unitbuff);

	setFont(&FreeSansBold18pt7b);
	getTextBounds(buff, 0, 0, &x1, &y1, &w1, &h1);
	setCursor(71 - (w1 / 2), 149);
	println(buff);
}

void SkyMate_Display::basicDisplay(SkyMatePosition currentPosition, SkyMatePreferences pref) {
	int16_t x1, y1;
	uint16_t w1, h1;
	char buff[20];
	char unitbuff[4];

	fillRect(0, 74, 144, 16, BLACK);
	fillRect(0, 151, 144, 16, BLACK);

	setTextColor(WHITE);
	setFont();
	switch (pref.horizontal)
	{
	case PREFERENCES_HORIZONTAL_KNOTS:
		dtostrf(currentPosition.speed.knots(), 1, 0, buff);
		strcpy(unitbuff, "kt");
		break;
	case PREFERENCES_HORIZONTAL_KPH:
		dtostrf(currentPosition.speed.kmph(), 1, 0, buff);
		strcpy(unitbuff, "kph");
		break;
	case PREFERENCES_HORIZONTAL_MPH:
		dtostrf(currentPosition.speed.mph(), 1, 0, buff);
		strcpy(unitbuff, "mph");
		break;
	}
	getTextBounds(unitbuff, 0, 0, &x1, &y1, &w1, &h1);
	setCursor(139 - w1, 87 - 8);
	print(unitbuff);

	setCursor(4, 87 - 8);
	print("Speed");

	setTextColor(BLACK);
	setFont(&FreeSansBold24pt7b);

	getTextBounds(buff, 0, 0, &x1, &y1, &w1, &h1);
	setCursor(72 - (w1 / 2), 60);
	println(buff);


	setTextColor(WHITE);
	setFont();
	switch (pref.altitude)
	{
	case PREFERENCES_ALTITUDE_FEET:
		dtostrf(currentPosition.alt.feet(), 1, 0, buff);
		strcpy(unitbuff, "ft");
		break;
	case PREFERENCES_ALTITUDE_METRES:
		dtostrf(currentPosition.alt.meters(), 1, 0, buff);
		strcpy(unitbuff, "m");
		break;
	}
	getTextBounds(unitbuff, 0, 0, &x1, &y1, &w1, &h1);
	setCursor(139 - w1, 164 - 8);
	print(unitbuff);

	setCursor(4, 164-8);
	print("Alt");

	setTextColor(BLACK);
	setFont(&FreeSansBold24pt7b);
	
	getTextBounds(buff, 0, 0, &x1, &y1, &w1, &h1);
	setCursor(72 - (w1 / 2), 137);
	println(buff);
}

void SkyMate_Display::displaySignal(uint8_t signalPower, uint8_t rsrq, uint8_t rsrp) {
	fillRect(0, 0, 144, 168, WHITE);
	setFont(&FreeSans9pt7b);
	setCursor(0, 25);
	if (signalPower < 32) {
		print("RSSI: -"); print(113 - (signalPower * 2)); println("db");
	}
	else {
		println("RSSI: NA");
	}

	if (rsrq < 35) {
		float rsrqdb = 20 - (rsrq*0.5);
		print("RSRQ: -"); printf("%0.1f",rsrqdb); println("db");
	}
	else {
		println("RSRQ: NA");
	}
	if (rsrp < 95) {
		print("RSRP: -"); print(141 - rsrp); println("db");
	}
	else {
		println("RSRP: NA");
	}
	refresh();
}

void SkyMate_Display::displayAudio(double *ptr, uint16_t length) {

	fillRect(0, 0, 144, 168, WHITE);
	for (int i = 2; i < (length / 2); i++) { // Don't use sample 0 and only the first SAMPLES/2 are usable.
											  // Each array element represents a frequency and its value, is the amplitude. Note the frequencies are not discrete.
		if (ptr[i] > 1500) { // Add a crude noise filter, 10 x amplitude or more
			if (i <= 2)             displayBand(0, (int)ptr[i]); // 125Hz
			if (i >2 && i <= 4)   displayBand(1, (int)ptr[i]); // 250Hz
			if (i >4 && i <= 7)   displayBand(2, (int)ptr[i]); // 500Hz
			if (i >7 && i <= 15)  displayBand(3, (int)ptr[i]); // 1000Hz
			if (i >15 && i <= 40)  displayBand(4, (int)ptr[i]); // 2000Hz
			if (i >40 && i <= 70)  displayBand(5, (int)ptr[i]); // 4000Hz
			if (i >70 && i <= 288) displayBand(6, (int)ptr[i]); // 8000Hz
			if (i >288) displayBand(7, (int)ptr[i]); // 16000Hz
													   //Serial.println(i);
		}

		for (byte band = 0; band <= 7; band++) drawLine(1 + 18 * band, 148 - peak[band], 17 + 18 * band, 148 - peak[band], BLACK);
	}
	if (millis() % 2 == 0) { for (byte band = 0; band <= 7; band++) { if (peak[band] > 0) peak[band] -= 1; } } // Decay the peak
	refresh();
}

void SkyMate_Display::displayBand(int band, int dsize) {
	int dmax = 140;
	dsize /= AMPLITUDE;
	if (dsize > dmax) dsize = dmax;
	for (int s = 0; s <= dsize; s = s + 2) { drawLine(1 + 18 * band , 148 - s, 17 + 18 * band, 148 - s, BLACK); }
	if (dsize > peak[band]) { peak[band] = dsize; }
}

void SkyMate_Display::charging(uint8_t chargeStatus, SkyMatePilot pilot, uint16_t trackerID) {
	int16_t x1, y1;
	uint16_t w1, h1;
	char buff[20];

	fillRect(0, 0, 144, 168, WHITE);
	uint16_t time = millis();
	uint16_t slider = time / 50;
	slider %= 40;

	switch (chargeStatus) {
	case 0:
		// Plugged in not charging
		drawLargeBattery(30);
		break;
	case 1:
	case 2:
		// Fast charge
		drawLargeBattery(slider + 30);
		break;
	case 3:
		// Charge Complete
		drawLargeBattery(80);
		break;
	default:
		break;
	}

	if (trackerID != 0) {
		setTextColor(BLACK);
		setFont(&FreeSansBold24pt7b);

		sprintf(buff, "%d", trackerID);

		getTextBounds(buff, 0, 0, &x1, &y1, &w1, &h1);
		setCursor(72 - (w1 / 2), 7 + h1);
		println(buff);

		getTextBounds(pilot.rego, 0, 0, &x1, &y1, &w1, &h1);
		setCursor(72 - (w1 / 2), 160);
		println(pilot.rego);
	}
	//refresh();
}

void SkyMate_Display::drawLargeBattery(uint16_t width) {
	fillRect(0, 0, 144, 168, WHITE);
	fillRect(22, 50, 100, 68, BLACK);
	fillRect(27, 55, 90, 58, WHITE);
	fillRect(122, 73, 10, 22, BLACK);
	if (width > 0) {
		fillRect(32, 60, width, 48, BLACK);
	}
	//refresh();
}

void SkyMate_Display::shutdownCounter(unsigned long shutdownTimer) {
	int16_t x1, y1;
	uint16_t w1, h1;
	float timer = (shutdownTimer / 1000) + 1;
	char timerState[10];

	sprintf(timerState, "%0.0f", timer);
	fillRect(0, 0, 144, 168, WHITE);

	setFont(&FreeSansBold24pt7b);
	getTextBounds(timerState, 0, 0, &x1, &y1, &w1, &h1);
	setCursor((144 / 2) - (w1 / 2), 84 + (h1/2));
	print(timerState);
	refresh();
}

void SkyMate_Display::formatCountdownTimer(char* text, long countdown) {
	if (countdown >= 600) {// Greater than 10 minutes
		uint16_t minutes = countdown / 60;
		sprintf(text, "%d min", countdown);
	}
	else if (countdown >= 0) {
		uint8_t minutes = countdown / 60;
		uint8_t seconds = countdown % 60;
		sprintf(text, "%d:%02d", minutes, seconds);
	}
	else {
		sprintf(text, "OPEN");
	}
}

void SkyMate_Display::formatDistance(char* text, double distance) {
	if (distance >= 100) {
		sprintf(text, "%0.0fkm", distance);
	}
	else if (distance >= 10) {
		sprintf(text, "%0.1fkm", distance);
	}
	else if (distance > 0) {
		sprintf(text, "%0.2fkm", distance);
	}
	else {
		sprintf(text, "-.--km");
	}
}

// 1<<n is a costly operation on AVR -- table usu. smaller & faster
static const uint8_t PROGMEM
set[] = { 1,  2,  4,  8,  16,  32,  64,  128 },
clr[] = { (uint8_t)~1 , (uint8_t)~2 , (uint8_t)~4 , (uint8_t)~8,
(uint8_t)~16, (uint8_t)~32, (uint8_t)~64, (uint8_t)~128 };

/**************************************************************************/
/*!
@brief Draws a single pixel in image buffer
@param[in]  x
The x position (0 based)
@param[in]  y
The y position (0 based)
*/
/**************************************************************************/
void SkyMate_Display::drawPixel(int16_t x, int16_t y, uint16_t color)
{
	if ((x < 0) || (x >= _width) || (y < 0) || (y >= _height)) return;

	switch (rotation) {
	case 1:
		_swap_int16_t(x, y);
		x = WIDTH - 1 - x;
		break;
	case 2:
		x = WIDTH - 1 - x;
		y = HEIGHT - 1 - y;
		break;
	case 3:
		_swap_int16_t(x, y);
		y = HEIGHT - 1 - y;
		break;
	}

	if (color) {
		sharpmem_buffer[(y * WIDTH + x) / 8] |=
			pgm_read_byte(&set[x & 7]);
	}
	else {
		sharpmem_buffer[(y * WIDTH + x) / 8] &=
			pgm_read_byte(&clr[x & 7]);
	}
}

/**************************************************************************/
/*!
@brief Gets the value (1 or 0) of the specified pixel from the buffer
@param[in]  x
The x position (0 based)
@param[in]  y
The y position (0 based)
@return     1 if the pixel is enabled, 0 if disabled
*/
/**************************************************************************/
uint8_t SkyMate_Display::getPixel(uint16_t x, uint16_t y)
{
	if ((x >= _width) || (y >= _height)) return 0; // <0 test not needed, unsigned

	switch (rotation) {
	case 1:
		_swap_uint16_t(x, y);
		x = WIDTH - 1 - x;
		break;
	case 2:
		x = WIDTH - 1 - x;
		y = HEIGHT - 1 - y;
		break;
	case 3:
		_swap_uint16_t(x, y);
		y = HEIGHT - 1 - y;
		break;
	}

	return sharpmem_buffer[(y * WIDTH + x) / 8] &
		pgm_read_byte(&set[x & 7]) ? 1 : 0;
}

/**************************************************************************/
/*!
@brief Clears the screen
*/
/**************************************************************************/
void SkyMate_Display::clearDisplay()
{
	memset(sharpmem_buffer, 0xff, (SHARPMEM_LCDWIDTH * SHARPMEM_LCDHEIGHT) / 8);
	// Send the clear screen command rather than doing a HW refresh (quicker)
	beginTransaction();
	sendbyte(_sharpmem_vcom | SHARPMEM_BIT_CLEAR);
	sendbyteLSB(0x00);
	TOGGLE_VCOM;
	endTransaction();
}

/**************************************************************************/
/*!
@brief Renders the contents of the pixel buffer on the LCD
*/
/**************************************************************************/
void SkyMate_Display::refresh(void)
{
	uint16_t i, totalbytes, currentline, oldline;
	totalbytes = (SHARPMEM_LCDWIDTH * SHARPMEM_LCDHEIGHT) / 8;

	// Send the write command
	beginTransaction();
	sendbyte(SHARPMEM_BIT_WRITECMD | _sharpmem_vcom);

	TOGGLE_VCOM;

	// Send the address for line 1
	oldline = currentline = 1;
	sendbyteLSB(currentline);

	// Send image buffer
	for (i = 0; i<totalbytes; i++)
	{
		sendbyteLSB(sharpmem_buffer[i]);
		currentline = ((i + 1) / (SHARPMEM_LCDWIDTH / 8)) + 1;
		if (currentline != oldline)
		{
			// Send end of line and address bytes
			sendbyteLSB(0x00);
			if (currentline <= SHARPMEM_LCDHEIGHT)
			{
				sendbyteLSB(currentline);
			}
			oldline = currentline;
		}
	}

	// Send another trailing 8 bits for the last line
	sendbyteLSB(0x00);
	endTransaction();
}

/**************************************************************************/
/*!
@brief  Sends a single byte in SPI.
*/
/**************************************************************************/
void SkyMate_Display::sendbyte(uint8_t data)
{
	// invert byte to make LSB first
	// code from http://forum.arduino.cc/index.php?topic=134482.0

	mySPI->transfer(data);
}

void SkyMate_Display::sendbyteLSB(uint8_t data)
{
	data = ((data >> 1) & 0x55) | ((data << 1) & 0xaa);
	data = ((data >> 2) & 0x33) | ((data << 2) & 0xcc);
	data = ((data >> 4) | (data << 4));

	mySPI->transfer(data);
}

void SkyMate_Display::beginTransaction(void) {
	mySPI->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
	digitalWrite(_ss, HIGH);
}

void SkyMate_Display::endTransaction(void) {
	digitalWrite(_ss, LOW);
	mySPI->endTransaction();
}
