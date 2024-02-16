#include <Wire.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <BleMouse.h>
#include <Keypad.h>

#define ROWS 2
#define COLS 4

uint8_t data[6];
int16_t gyroX, gyroZ;

int Sensitivity = 400;

BleMouse bleMouse;

uint8_t i2cData[14];

const uint8_t IMUAddress = 0x68;
const uint16_t I2C_TIMEOUT = 1000;

char keyMap[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' }
};

uint8_t rowPins[ROWS] = {14, 27};
uint8_t colPins[COLS] = {33, 32, 18, 19};

Keypad keypad = Keypad(makeKeymap(keyMap), rowPins, colPins, ROWS, COLS);

// Function to write data to I2C device
uint8_t i2cWrite(uint8_t registerAddress, uint8_t* data, uint8_t length, bool sendStop) {
  Wire.beginTransmission(IMUAddress);
  Wire.write(registerAddress);
  Wire.write(data, length);
  return Wire.endTransmission(sendStop);  // Returns 0 on success
}

// Function to write a single byte to I2C device
uint8_t i2cWrite2(uint8_t registerAddress, uint8_t data, bool sendStop) {
  return i2cWrite(registerAddress, &data, 1, sendStop);  // Returns 0 on success
}

// Function to read data from I2C device
uint8_t i2cRead(uint8_t registerAddress, uint8_t* data, uint8_t nbytes) {
  uint32_t timeOutTimer;
  Wire.beginTransmission(IMUAddress);
  Wire.write(registerAddress);
  if (Wire.endTransmission(false))
    return 1;
  Wire.requestFrom(IMUAddress, nbytes, (uint8_t) true);
  for (uint8_t i = 0; i < nbytes; i++) {
    if (Wire.available())
      data[i] = Wire.read();
    else {
      timeOutTimer = micros();
      while (((micros() - timeOutTimer) < I2C_TIMEOUT) && !Wire.available())
        ;
      if (Wire.available())
        data[i] = Wire.read();
      else
        return 2;
    }
  }
  return 0;
}

void setup() {
  Wire.begin();

  // Initializing IMU
  i2cData[0] = 7;
  i2cData[1] = 0x00;
  i2cData[3] = 0x00;

  while (i2cWrite(0x19, i2cData, 4, false))
    ;
  while (i2cWrite2(0x6B, 0x01, true))
    ;
  while (i2cRead(0x75, i2cData, 1))
    ;
  delay(100);
  while (i2cRead(0x3B, i2cData, 6))
    ;

  // Initializing Serial and BleMouse
  Serial.begin(115200);
  bleMouse.begin();
  delay(100);
}

void loop() {
  // Reading sensor data from IMU
  while (i2cRead(0x3B, i2cData, 14))
    ;

  gyroX = ((i2cData[8] << 8) | i2cData[9]);
  gyroZ = ((i2cData[12] << 8) | i2cData[13]);

  // Adjusting gyro data based on sensitivity
  gyroX = gyroX / Sensitivity / 1.1 * -1;
  gyroZ = gyroZ / Sensitivity * -1;

  if (bleMouse.isConnected()) {
    // Sending mouse movement data
    bleMouse.move(gyroZ, gyroX);

    // Handling keypad inputs for mouse clicks and movements
    char key = keypad.getKey();
    switch (key) {
      case '1':
        bleMouse.click(MOUSE_LEFT);
        break;
      case '2':
        bleMouse.click(MOUSE_RIGHT);
        break;
      case '3':
        bleMouse.click(MOUSE_BACK);
        break;
      case 'A':
        bleMouse.click(MOUSE_FORWARD);
        break;
      case '4':
        bleMouse.click(MOUSE_MIDDLE);
        break;
      case '5':
        bleMouse.move(0, 0, 1);
        break;
      case '6':
        bleMouse.move(0, 0, -1);
        break;
      case 'B':
        bleMouse.click(MOUSE_LEFT);
        bleMouse.click(MOUSE_LEFT);
        break;
    }

    delay(20);  // Delay for stability
  }
}
