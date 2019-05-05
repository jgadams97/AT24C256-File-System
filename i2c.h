char I2C_PIN_SCL;
char I2C_PIN_SDA;

//Read a digital pin (open collector).
char i2cDigitalRead(char pin) {
	pinMode(pin, INPUT_PULLUP);
	return digitalRead(pin);
}

//Write a digital pin (open collector).
void i2cDigitalWrite(char pin, char val) {
	if (val == LOW) {
		digitalWrite(pin, LOW);
		pinMode(pin, OUTPUT);
	} else {
		pinMode(pin, INPUT_PULLUP);
	}
}

//Setup I2C.
void i2cSetup(char pinSDA, char pinSCL) {
	I2C_PIN_SCL = pinSCL;
	I2C_PIN_SDA = pinSDA;
	i2cDigitalWrite(I2C_PIN_SCL, LOW);
	i2cDigitalWrite(I2C_PIN_SDA, LOW);
}

//Begin a transmission.
void i2cBeginTransmission() {
	i2cDigitalWrite(I2C_PIN_SCL, HIGH);
	i2cDigitalWrite(I2C_PIN_SDA, HIGH);
	i2cDigitalWrite(I2C_PIN_SDA, LOW);
	i2cDigitalWrite(I2C_PIN_SCL, LOW);
}

//End a transmission.
void i2cEndTransmission() {
	i2cDigitalWrite(I2C_PIN_SCL, LOW);
	i2cDigitalWrite(I2C_PIN_SDA, LOW);
	i2cDigitalWrite(I2C_PIN_SCL, HIGH);
	i2cDigitalWrite(I2C_PIN_SDA, HIGH);
}

//Send a byte.
void i2cSend(char b) {
	for (char i = 0; i < 8; i++) {
		if (b & 0b10000000)
			i2cDigitalWrite(I2C_PIN_SDA, HIGH);
		else
			i2cDigitalWrite(I2C_PIN_SDA, LOW);
		i2cDigitalWrite(I2C_PIN_SCL, HIGH);
		i2cDigitalWrite(I2C_PIN_SCL, LOW);
		b <<= 1;
	}
}

//Read a byte.
char i2cReceive() {
	char b = 0;
	for (char i = 0; i < 8; i++) {
		b <<= 1;
		if (i2cDigitalRead(I2C_PIN_SDA))
			b++;
		i2cDigitalWrite(I2C_PIN_SCL, HIGH);
		i2cDigitalWrite(I2C_PIN_SCL, LOW);
	}
	return b;
}

//Check for acknowledge.
char i2cAcknowledge() {
	char r = i2cDigitalRead(I2C_PIN_SDA);
	i2cDigitalWrite(I2C_PIN_SCL, HIGH);
	i2cDigitalWrite(I2C_PIN_SCL, LOW);
	return r;
}
