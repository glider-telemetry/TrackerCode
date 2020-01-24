/*!
 * 	I2C Driver for BQ24259
 */

#ifndef _BQ24259_H
#define _BQ24259_H

#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <Wire.h>
#include "BQ24259_defs.h"

#define BQ24259_I2CADDR_DEFAULT 0x18 ///< I2C address

 /*!
  *    @brief  Class that stores state and functions for interacting with
  *            BQ24259 Battery Charger Chip
  */
class BQ24259 {
public:
	BQ24259();
	bool begin();
	bool begin(TwoWire *theWire);
	bool begin(uint8_t addr);
	bool begin(uint8_t addr, TwoWire *theWire);

	bool init();
	bool readPowerGood();
	uint8_t readChargeStatus();

	uint8_t readFaults(bool debug);

	void shutdown();

	void write16(uint8_t reg, uint16_t val);
	uint16_t read16(uint8_t reg);

	void write8(uint8_t reg, uint8_t val);
	uint8_t read8(uint8_t reg);

private:
	TwoWire *_wire;
	uint8_t _i2caddr;
};

#endif

