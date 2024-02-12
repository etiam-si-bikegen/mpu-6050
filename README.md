# MPU-6050
## Funkcionalnosti
- Možnost izbire merilnega obsega
- Enostaven vnos kalibracijskih biasov
- Podpora za kalibracijsko matriko programa [Magneto 1.2](https://sailboatinstruments.blogspot.com/2011/09/improved-magnetometer-calibration-part.html)

## Kje nastaviti BIASE?
- v ```mpu6050_const.h``` datoteki

```c
#define BIAS_GYRO_X -911.125
#define BIAS_GYRO_Y 72.875
#define BIAS_GYRO_Z 14

#define BIAS_ACCEL_X 160.156425
#define BIAS_ACCEL_Y -294.984133
#define BIAS_ACCEL_Z -1871.282168
```

## Kje nastaviti kalibracijsko matriko
- v glavni kodi
```c
Matrix3x3 mpuCalibMatrix = {
  { 1.000542, 0.000006, 0.002136 },
  { 0.000006, 0.999202, 0.001130 },
  { 0.002136, 0.001130, 0.980568 }
};
```