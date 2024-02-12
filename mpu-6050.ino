/*
Avtor: Grega Rotar
Namen: 
  Komunikacija ESP32 z MPU6050 in izhod v serijsko komunikacijo z možnostjo nastavitve lastnosti kot
  2g, 4g, 8g in 16g ter kot deg/s od 250, 500, 1000 do 2000.
  Prilagaja biase glede na izbrano natančnost.
  Omogoča tudi obsežno kalibracijo z matriko iz Magneto 1.2
*/
#include "mpu6050_const.h"
#include <Wire.h>

// MPU-6050
// pospeskomer spremenljivke
int16_t accelX, accelY, accelZ;
double gForceX, gForceY, gForceZ;
// žiroskop spremenljivke
int16_t gyroX, gyroY, gyroZ;
double rotX, rotY, rotZ;

// I2C za MPU-6050
int mpuAddr = 0x68;
// kalibracijska matrika pridobljena z Magneto 1.2
typedef float Matrix3x3[3][3];
Matrix3x3 mpuCalibMatrix = {
  { 1.000542, 0.000006, 0.002136 },
  { 0.000006, 0.999202, 0.001130 },
  { 0.002136, 0.001130, 0.980568 }
};

void setup() {
  Serial.begin(115200);
  // MPU-6050
  Wire.begin();  // defaultni 21 (SDA) in 22 (SCL) drugace dodaj kot parameter
  setupMpu();
}


String output = "";
unsigned int loopSleep = 1000;
void loop() {

  // bere podatke MPU-6050
  recordAccelRegisters();
  recordGyroRegisters();
  output = String(gForceX) + "," + String(gForceY) + "," + String(gForceZ) + "," + String(rotX) + "," + String(rotY) + "," + String(rotZ);
  Serial.println(output);
  delay(1000);
}

// funkcije za MPU-6050
// mpu6050_const.h vključuje variable za to funkcijo
void setupMpu() {
  // FIXME: dodaj support za to da se nastavi žiroskop in pospeškomer
  // konfiguracija power-ja
  Wire.beginTransmission(mpuAddr);
  Wire.write(0x6B);        // register za power management
  Wire.write(0b00000000);  // nastavi SLEEP register na 0 (glej 4.28)
  Wire.endTransmission();

  // konfiguracija žiroskopa
  Wire.beginTransmission(mpuAddr);
  Wire.write(0x1B);
  // Bit 3 in 4 za konfiguracijo gyroscopa (glej 4.4)
  // 00 (0) +/- 250deg/s
  // 01 (1) +/- 500deg/s
  // 10 (2) +/- 1000deg/s
  // 11 (3) +/- 2000deg/s

  uint8_t gyro_config;
  switch (FS_SEL) {
    case 0:
      gyro_config = 0b00000000;  // nastavljen na +/- 250deg/sec
      break;
    case 1:
      gyro_config = 0b00001000;  // nastavljen na +/- 500deg/sec
      break;
    case 2:
      gyro_config = 0b00010000;  // nastavljen na +/- 1000deg/sec
      break;
    case 3:
      gyro_config = 0b00011000;  // nastavljen na +/- 2000deg/sec
      break;
  }
  Wire.write(gyro_config);
  Wire.endTransmission();

  // konfiguracija pospeškomera
  Wire.beginTransmission(mpuAddr);
  Wire.write(0x1C);
  // 00 (0) +/- 2g
  // 01 (1) +/- 4g
  // 10 (2) +/- 8g
  // 11 (3) +/- 16g
  uint8_t accel_config;
  switch (AFS_SEL) {
    case 0:
      accel_config = 0b00000000;  // nastavljen na +/- 250deg/sec
      break;
    case 1:
      accel_config = 0b00001000;  // nastavljen na +/- 500deg/sec
      break;
    case 2:
      accel_config = 0b00010000;  // nastavljen na +/- 1000deg/sec
      break;
    case 3:
      accel_config = 0b00011000;  // nastavljen na +/- 2000deg/sec
      break;
  }
  Wire.write(accel_config);
  Wire.endTransmission();
}

// zajem podatkov pospeškomera
void recordAccelRegisters() {
  Wire.beginTransmission(mpuAddr);
  Wire.write(0x3B);  // izbran začetni register
  Wire.endTransmission();
  Wire.requestFrom(mpuAddr, 6);  // zahteva accel registre (3B - 40)
  while (Wire.available() < 6)
    ;  // program caka da je na voljo vseh 6 bytov
  accelX = (Wire.read() << 8) | Wire.read();
  accelY = (Wire.read() << 8) | Wire.read();
  accelZ = (Wire.read() << 8) | Wire.read();

  processAccelData();
}

// pretvorba podatkov pospeškomera v berljive vrednosti
void processAccelData() {
  // LSB/g glej poglavje 4.17
  // FIXME: vrednost je določena le ko je accel nastavljen na 2G
  // kalibracija z magneto 1.2 100 podatkov
  // 16384
  // what this is doing is basicly just dividing bias so that it matches current setting of deg/sec
  // FIXME: not the best method. Calibrate it to current settings
  gForceX = (accelX - ((BIAS_ACCEL_X) / (LSB_G_CALIBRATED / LSB_G)));
  gForceY = (accelY - ((BIAS_ACCEL_Y) / (LSB_G_CALIBRATED / LSB_G)));
  gForceZ = (accelZ - ((BIAS_ACCEL_Z) / (LSB_G_CALIBRATED / LSB_G)));

  gForceX = mpuCalibMatrix[0][0] * gForceX + mpuCalibMatrix[0][1] * gForceY + mpuCalibMatrix[0][2] * gForceZ;
  gForceY = mpuCalibMatrix[1][0] * gForceX + mpuCalibMatrix[1][1] * gForceY + mpuCalibMatrix[1][2] * gForceZ;
  gForceZ = mpuCalibMatrix[2][0] * gForceX + mpuCalibMatrix[2][1] * gForceY + mpuCalibMatrix[2][2] * gForceZ;

  gForceX = gForceX / LSB_G;
  gForceY = gForceY / LSB_G;
  gForceZ = gForceZ / LSB_G;
}

// zajem podatkov žiroskopa
void recordGyroRegisters() {
  Wire.beginTransmission(0b1101000);  //I2C address of the MPU
  Wire.write(0x43);                   //izbran začeten register
  Wire.endTransmission();
  Wire.requestFrom(0b1101000, 6);  //zahteva gyro registere (43 - 48)
  while (Wire.available() < 6)
    ;
  gyroX = Wire.read() << 8 | Wire.read();
  gyroY = Wire.read() << 8 | Wire.read();
  gyroZ = Wire.read() << 8 | Wire.read();
  processGyroData();
}

// pretvorba podatkov žiroskopa v berljive vrednosti
void processGyroData() {
  // LSB/deg/s glej poglavje 4.19
  // what this is doing is basicly just dividing bias so that it matches current setting of deg/sec
  // FIXME: not the best method. Calibrate it to current settings
  // FIXME: i dont know if this matrix is the best for gyro data to
  rotX = (gyroX - ((BIAS_GYRO_X) / (LSB_DEG_CALIBRATED / LSB_DEG)));
  rotY = (gyroY - ((BIAS_GYRO_Y) / (LSB_DEG_CALIBRATED / LSB_DEG)));
  rotZ = (gyroZ - ((BIAS_GYRO_Z) / (LSB_DEG_CALIBRATED / LSB_DEG)));

  rotX = mpuCalibMatrix[0][0] * rotX + mpuCalibMatrix[0][1] * rotY + mpuCalibMatrix[0][2] * rotZ;
  rotY = mpuCalibMatrix[1][0] * rotX + mpuCalibMatrix[1][1] * rotY + mpuCalibMatrix[1][2] * rotZ;
  rotZ = mpuCalibMatrix[2][0] * rotX + mpuCalibMatrix[2][1] * rotY + mpuCalibMatrix[2][2] * rotZ;

  rotX = rotX / LSB_DEG;
  rotY = rotY / LSB_DEG;
  rotZ = rotZ / LSB_DEG;
}
