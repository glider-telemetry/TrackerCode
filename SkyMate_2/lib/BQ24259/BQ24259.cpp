/*!
 *  @file BQ24259.cpp
 */

#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#ifdef __AVR_ATtiny85__
#include "TinyWireM.h"
#define Wire TinyWireM
#else
#include <Wire.h>
#endif

#include "BQ24259.h"

 /*!
  *    @brief  Instantiates a new BQ24259 class
  */
BQ24259::BQ24259() {}

/*!
 *    @brief  Setups the HW
 *    @param  *theWire
 *    @return True if initialization was successful, otherwise false.
 */
bool BQ24259::begin(TwoWire *theWire) {
	_wire = theWire;
	_i2caddr = BQ24259_I2C_ADDR_PRIM;
	return init();
}

/*!
 *    @brief  Setups the HW
 *    @param  addr
 *    @return True if initialization was successful, otherwise false.
 */
bool BQ24259::begin(uint8_t addr) {
	_i2caddr = addr;
	_wire = &Wire;
	return init();
}

/*!
 *    @brief  Setups the HW
 *    @param  addr
 *    @param  *theWire
 *    @return True if initialization was successful, otherwise false.
 */
bool BQ24259::begin(uint8_t addr, TwoWire *theWire) {
	_i2caddr = addr;
	_wire = theWire;
	return init();
}

/*!
 *    @brief  Setups the HW with default address
 *    @return True if initialization was successful, otherwise false.
 */
bool BQ24259::begin() {
	_i2caddr = BQ24259_I2C_ADDR_PRIM;
	_wire = &Wire;
	return init();
}

/*!
 *    @brief  init function
 *    @return True if initialization was successful, otherwise false.
 */
bool BQ24259::init() {
	write8(BQ24259_INPUT_SOURCE_CONTROL_ADDR, 0x34);
	write8(BQ24259_POWER_ON_CONFIGURATION_ADDR, 0x1B);
	// Can do standard config stuff here.
	// Standard watchdog timer
	uint8_t watchdog_reg = read8(BQ24259_TERMINATION_TIMER_CONTROL_ADDR);
	uint8_t watchdog_disable = (watchdog_reg & ~BQ24259_WATCHDOG_TIMER_MASK) | BQ24259_WATCHDOG_TIMER_DISABLED;
	watchdog_disable = watchdog_disable & ~BQ24259_CHARGE_TERMINATION_ENABLE; // disable the charge termination;

	write8(BQ24259_TERMINATION_TIMER_CONTROL_ADDR, watchdog_disable);

	uint8_t temperature_reg = read8(BQ24259_BOOST_THERMAL_REG_CONTROL_ADDR);
	uint8_t temperature_set = (temperature_reg & ~BQ24259_BOOST_MODE_TEMP_MASK) | BQ24259_BOOST_MODE_TEMP_65;
	write8(BQ24259_BOOST_THERMAL_REG_CONTROL_ADDR, temperature_set);

	// Start Charging

	//uint8_t powerOn_reg = read8(BQ24259_POWER_ON_CONFIGURATION_ADDR);
	//uint8_t powerOn_set = (powerOn_reg | BQ24259_OTG_CONFIG);
	//write8(BQ24259_POWER_ON_CONFIGURATION_ADDR, powerOn_set);

	return true;
}

/*!
*    @brief  Read Power Good
*    @return True if power good, otherwise false.
*/
bool BQ24259::readPowerGood() {
	uint8_t status_reg = read8(BQ24259_SYSTEM_STATUS_ADDR);
	if ((status_reg & BQ24259_PG_STATE) == BQ24259_PG_STATE) {
		return true;
	}
	else {
		return false;
	}
}

/*!
*    @brief  Read Charge Status
*    @return 0 = Not Charging, 1 = Pre Charge, 2 - Fast Charging, 3 = Charge Complete
*/
uint8_t BQ24259::readChargeStatus() {
	uint8_t status_reg = read8(BQ24259_SYSTEM_STATUS_ADDR);
	return (status_reg & BQ24259_CHARGE_STATE_MASK) >> BQ24259_CHARGE_STATE_POSN;
}

/*!
*    @brief  Read faults - Output to serial
*    @return Raw faults register
*/
uint8_t BQ24259::readFaults(bool debug = false) {
	uint8_t fault_reg = read8(BQ24259_NEW_FAULT_ADDR);

	if (debug) {
		if ((fault_reg & BQ24259_WATCHDOG_FAULT) == BQ24259_WATCHDOG_FAULT) Serial.println("BQ24259 - Watchdog Timer Expiration");
		if ((fault_reg & BQ24259_OTG_FAULT) == BQ24259_OTG_FAULT) Serial.println("BQ24259 - Cannot Start Boost Function");
		if ((fault_reg & BQ24259_CHARGE_FAULT_MASK) == BQ24259_CHARGE_FAULT_INPUT_FAULT) Serial.println("BQ24259 - Input Fault");
		if ((fault_reg & BQ24259_CHARGE_FAULT_MASK) == BQ24259_CHARGE_FAULT_THERMAL_SHUTDOWN) Serial.println("BQ24259 - Thermal Shutdown");
		if ((fault_reg & BQ24259_CHARGE_FAULT_MASK) == BQ24259_CHARGE_FAULT_TIMER_EXPIRATION) Serial.println("BQ24259 - Charge Timer Expiration");
		if ((fault_reg & BQ24259_BATT_FAULT) == BQ24259_BATT_FAULT) Serial.println("Battery OVP");
		if ((fault_reg & BQ24259_NTC_FAULT_COLD) == BQ24259_NTC_FAULT_COLD) Serial.println("NTC Cold");
		if ((fault_reg & BQ24259_NTC_FAULT_HOT) == BQ24259_NTC_FAULT_HOT) Serial.println("NTC Hot");
	}

	return fault_reg;
}

/*!
 *   @brief  Set Charger to switch off batfet (Disable Watchdog Timer First)
 */
void BQ24259::shutdown() {
	uint8_t watchdog_reg = read8(BQ24259_TERMINATION_TIMER_CONTROL_ADDR);
	uint8_t watchdog_disable = (watchdog_reg & ~BQ24259_WATCHDOG_TIMER_MASK) | BQ24259_WATCHDOG_TIMER_DISABLED;
	write8(BQ24259_TERMINATION_TIMER_CONTROL_ADDR, watchdog_disable);
	
	uint8_t conf_reg = read8(BQ24259_MISC_OPERATION_CONTROL_ADDR);
	uint8_t conf_shutdown = conf_reg | BQ24259_BATFET_DISABLE;
	write8(BQ24259_MISC_OPERATION_CONTROL_ADDR, conf_shutdown);
}

/*!
 *    @brief  Low level 16 bit write procedures
 *    @param  reg
 *    @param  value
 */
void BQ24259::write16(uint8_t reg, uint16_t value) {
	_wire->beginTransmission(_i2caddr);
	_wire->write((uint8_t)reg);
	_wire->write(value >> 8);
	_wire->write(value & 0xFF);
	_wire->endTransmission();
}

/*!
 *    @brief  Low level 16 bit read procedure
 *    @param  reg
 *    @return value
 */
uint16_t BQ24259::read16(uint8_t reg) {
	uint16_t val = 0xFFFF;
	uint8_t state;

	_wire->beginTransmission(_i2caddr);
	_wire->write((uint8_t)reg);
	state = _wire->endTransmission();

	if (state == 0) {
		_wire->requestFrom((uint8_t)_i2caddr, (uint8_t)2);
		val = _wire->read();
		val <<= 8;
		val |= _wire->read();
	}

	return val;
}

/*!
 *    @brief  Low level 8 bit write procedure
 *    @param  reg
 *    @param  value
 */
void BQ24259::write8(uint8_t reg, uint8_t value)
{
	_wire->beginTransmission(_i2caddr);
	_wire->write((uint8_t)reg);
	_wire->write(value);
	_wire->endTransmission();
}

/*!
 *    @brief  Low level 8 bit read procedure
 *    @param  reg
 *    @return value
 */
uint8_t BQ24259::read8(uint8_t reg)
{
	uint8_t val = 0xFF;
	uint8_t state;

	_wire->beginTransmission(_i2caddr);
	_wire->write((uint8_t)reg);
	state = _wire->endTransmission();

	if (state == 0)
	{
		_wire->requestFrom((uint8_t)_i2caddr, (uint8_t)1);
		val = _wire->read();
	}

	return val;
}
