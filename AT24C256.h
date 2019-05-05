#include "i2c.h"
char EEPROM_ADDRESS;
char EEPROM_STATUS;

void eepromWrite(unsigned int addr, char val) {
	EEPROM_STATUS = 1;
	i2cBeginTransmission();
	i2cSend(0b10100000 | EEPROM_ADDRESS);
	if (i2cAcknowledge() != 0) {
		EEPROM_STATUS = 0;
		return;
	}
	i2cSend(addr >> 8);
	if (i2cAcknowledge() != 0) {
		EEPROM_STATUS = 0;
		return;
	}
	i2cSend(addr & 0xFF);
	if (i2cAcknowledge() != 0) {
		EEPROM_STATUS = 0;
		return;
	}
	i2cSend(val);
	if (i2cAcknowledge() != 0) {
		EEPROM_STATUS = 0;
		return;
	}
	i2cEndTransmission();
	delay(5);
}

char eepromRead(unsigned int addr) {
	EEPROM_STATUS = 1;
	char val = 0;
	i2cBeginTransmission();
	i2cSend(0b10100000 | EEPROM_ADDRESS);
	if (i2cAcknowledge() != 0) {
		EEPROM_STATUS = 0;
		return 0;
	}
	i2cSend(addr >> 8);
	if (i2cAcknowledge() != 0) {
		EEPROM_STATUS = 0;
		return 0;
	}
	i2cSend(addr & 0xFF);
	if (i2cAcknowledge() != 0) {
		EEPROM_STATUS = 0;
		return 0;
	}

	i2cBeginTransmission();
	i2cSend(0b10100001 | EEPROM_ADDRESS);
	if (i2cAcknowledge() != 0) {
		EEPROM_STATUS = 0;
		return 0;
	}
	val = i2cReceive();
	if (i2cAcknowledge() != 1) {
		EEPROM_STATUS = 0;
		return 0;
	}
	i2cEndTransmission();

	return val;	
}

char eepromSetup(char addr, int pinSDA, int pinSCL) {
	i2cSetup(pinSDA, pinSCL);
	EEPROM_ADDRESS = (addr << 1) & 0b110;
	eepromRead(0);
	return EEPROM_STATUS;
}

