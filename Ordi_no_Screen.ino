/*
  Raspberry Pi Pico — BMP280 + MPU6050 + Buzzer + LCD SH1106
  Bus I2C : SDA = GP0, SCL = GP1
  Buzzer  : GP6
  Vitesse de lecture : 100 Hz (toutes les 10ms)
  -------------------------------------------------------
  Bibliothèques nécessaires :
    - Adafruit BMP280 Library  (+ Adafruit Unified Sensor)
    - Adafruit MPU6050         (+ Adafruit Unified Sensor)
    - Adafruit SH110X
    - Adafruit GFX Library
*/

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <SD.h>
#include <pico/util/queue.h>


// ---------- Broches ----------
const int buzzer = 8;
const int CS_pin = 7;

// ---------- Capteurs ----------
Adafruit_BMP280 bmp;
Adafruit_MPU6050 mpu;

// ---------- Timing ----------
const unsigned long INTERVAL_MS     = 10;    // 100 Hz = 1 lecture toutes les 10ms
const unsigned long BUFFER_LENGTH  = 40;     // Enregistrement de 40 mesures a la

unsigned long lastSensor  = 0;
unsigned long lastDisplay = 0;
unsigned long lastPage    = 0;

// Dernières valeurs lues (partagées entre lecture et affichage)
float temperature, pressure, altitude;
float ax, ay, az;
float gx, gy, gz;

queue_t data_transit;

unsigned buffer = 0;

File datasave;

void bip(int duree_ms = 80) {
  tone(buzzer, 1000);
  delay(duree_ms);                   // Attend 1 seconde
  noTone(buzzer);  // Éteint le buzzer
}

typedef struct {
    unsigned Time;
    float Temperature;
    float Pressure;
    float Altitude;
    float ax;
    float ay;
    float az;
    float gx;
    float gy;
    float gz;
} data_unit;


void setup() {    // Lecture (coeur 1)
  Serial.begin(115200);

  pinMode(buzzer, OUTPUT);

  Wire.setSDA(0);
  Wire.setSCL(1);
  Wire.begin();
  Wire.setClock(400000);   // I2C rapide (400kHz) pour tenir 100Hz

  // --- BMP280 ---
  if (!bmp.begin(0x76)) {
    bip(1500);;
    delay(1500);
  } else {
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                    Adafruit_BMP280::SAMPLING_X1,   // temp x1
                    Adafruit_BMP280::SAMPLING_X2,   // pression x2
                    Adafruit_BMP280::FILTER_X4,
                    Adafruit_BMP280::STANDBY_MS_1); // standby minimal
    bip(400); delay(400); bip(400);
    delay(1500);
  }

  // --- MPU6050 ---
  if (!mpu.begin(0x69)) {
    bip(1500);
    delay(1500);
  } else {
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_44_HZ); // filtre adapté à 100Hz
    bip(400); delay(400); bip(400);
    delay(1500);
  }
  queue_init(&data_transit, sizeof(data_unit), BUFFER_LENGTH);
}

void loop() {
  unsigned long now = millis();

  // -------- Lecture capteurs à 100 Hz --------
    if (now - lastSensor >= INTERVAL_MS) {
      lastSensor = now;

      temperature = bmp.readTemperature();
      pressure    = bmp.readPressure() / 100.0;
      altitude    = bmp.readAltitude(1013.25);

      sensors_event_t accel, gyro, temp_mpu;
      mpu.getEvent(&accel, &gyro, &temp_mpu);
      ax = accel.acceleration.x; ay = accel.acceleration.y; az = accel.acceleration.z;
      gx = gyro.gyro.x;          gy = gyro.gyro.y;          gz = gyro.gyro.z;

      data_unit data_t = {now, temperature, pressure, altitude, ax, ay, az, gx, gy, gz};
      queue_try_add(&data_transit, &data_t);
    }
  }


void setup1() {    // Ecriture (coeur 2)
  delay(9000);
  if (!SD.begin(CS_pin)){
    bip(1500);
    delay(1500);
  }
  else {
    datasave = SD.open("data.csv", FILE_WRITE);
    datasave.println("Timestamp,Temp,Pres,Alt,Ax,Ay,Az,Gx,Gy,Gz");
    bip(400); delay(400); bip(400);
  }
}


void loop1() {
  data_unit data_recue;
  queue_remove_blocking(&data_transit, &data_recue);

  datasave.print(data_recue.Time); datasave.print(",");
  datasave.print(data_recue.Temperature); datasave.print(",");
  datasave.print(data_recue.Pressure); datasave.print(",");
  datasave.print(data_recue.Altitude); datasave.print(",");
  datasave.print(data_recue.ax); datasave.print(",");
  datasave.print(data_recue.ay); datasave.print(",");
  datasave.print(data_recue.az); datasave.print(",");
  datasave.print(data_recue.gx); datasave.print(",");
  datasave.print(data_recue.gy); datasave.print(",");
  datasave.println(data_recue.gz);

  buffer++;
  if (buffer >= BUFFER_LENGTH){
    buffer = 0;
    datasave.flush();
  }
}