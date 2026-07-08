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
#include <Adafruit_SH110X.h>
#include <SD.h>
#include <pico/util/queue.h>

// ---------- Écran SH1106 128x64 ----------
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_ADDR     0x3C

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ---------- Broches ----------
const int buzzer = 8;
const int CS_pin = 7;

// ---------- Capteurs ----------
Adafruit_BMP280 bmp;
Adafruit_MPU6050 mpu;

// ---------- Timing ----------
const unsigned long INTERVAL_MS     = 10;    // 100 Hz = 1 lecture toutes les 10ms
const unsigned long DISPLAY_MS      = 200;   // écran rafraîchi à 5 Hz (lisible)
const unsigned long PAGE_CHANGE_MS  = 2000;  // changement de page toutes les 2s

unsigned long lastSensor  = 0;
unsigned long lastDisplay = 0;
unsigned long lastPage    = 0;

int page = 0;

// Dernières valeurs lues (partagées entre lecture et affichage)
float temperature, pressure, altitude;
float ax, ay, az;
float gx, gy, gz;

queue_t data_transit;

const unsigned long INTERVAL_MS     = 10;    // 100 Hz = 1 lecture toutes les 10ms
const unsigned long BUFFER_LENGTH  = 300;
const unsigned long BUFFER_SAVE_LENGTH  = 40;

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

  // --- Écran ---
  if (!display.begin(OLED_ADDR, true)) {
    bip(1500);
    delay(1500);
  } else {
    Serial.println("[OK] Ecran SH1106 detecte");
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(15, 20); display.println("Pico Sensor Box");
    display.setCursor(25, 36); display.println("100 Hz - GO !");
    display.display();
    bip(400); delay(400); bip(400);
    delay(1500);
  }

  // --- BMP280 ---
  if (!bmp.begin(0x76)) {
    display.clearDisplay();
    display.setTextColor(SH110X_WHITE);
    display.setTextSize(1);
    display.setCursor(15, 20); display.println("Pico Sensor Box");
    display.setCursor(25, 36); display.println("[Erreur] BMP280 non detecte");
    display.display();
    bip(1500);;
    delay(1500);
  } else {
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                    Adafruit_BMP280::SAMPLING_X1,   // temp x1
                    Adafruit_BMP280::SAMPLING_X2,   // pression x2
                    Adafruit_BMP280::FILTER_X4,
                    Adafruit_BMP280::STANDBY_MS_1); // standby minimal
    display.clearDisplay();
    display.setTextColor(SH110X_WHITE);
    display.setTextSize(1);
    display.setCursor(15, 20); display.println("Pico Sensor Box");
    display.setCursor(25, 36); display.println("[OK] BMP280 detecte");
    display.display();
    bip(400); delay(400); bip(400);
    delay(1500);
  }

  // --- MPU6050 ---
  if (!mpu.begin(0x69)) {
    display.clearDisplay();
    display.setTextColor(SH110X_WHITE);
    display.setTextSize(1);
    display.setCursor(15, 20); display.println("Pico Sensor Box");
    display.setCursor(25, 36); display.println("[ERREUR] MPU6050 non detecte !");
    display.display();
    bip(1500);
    delay(1500);
  } else {
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_44_HZ); // filtre adapté à 100Hz
    display.clearDisplay();
    display.setTextColor(SH110X_WHITE);
    display.setTextSize(1);
    display.setCursor(15, 20); display.println("Pico Sensor Box");
    display.setCursor(25, 36); display.println("[OK] MPU6050 detecte");
    display.display();
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

    // -------- Écran rafraîchi à 5 Hz (lisible) --------
    if (now - lastDisplay >= DISPLAY_MS) {
      lastDisplay = now;

      display.clearDisplay();
      display.setTextColor(SH110X_WHITE);
      display.setTextSize(1);

      if (page == 0) {
        display.setCursor(0, 0);  display.println("=== BMP280 ===");
        display.setCursor(0, 16); display.print("Temp : "); display.print(temperature, 1); display.println(" C");
        display.setCursor(0, 30); display.print("Pres : "); display.print(pressure, 1);    display.println(" hPa");
        display.setCursor(0, 44); display.print("Alt  : "); display.print(altitude, 1);    display.println(" m");

      } else if (page == 1) {
        display.setCursor(0, 0);  display.println("= Accelero m/s2 =");
        display.setCursor(0, 16); display.print("X : "); display.println(ax, 2);
        display.setCursor(0, 30); display.print("Y : "); display.println(ay, 2);
        display.setCursor(0, 44); display.print("Z : "); display.println(az, 2);

      } else if (page == 2) {
        display.setCursor(0, 0);  display.println("=  Gyro rad/s  =");
        display.setCursor(0, 16); display.print("X : "); display.println(gx, 3);
        display.setCursor(0, 30); display.print("Y : "); display.println(gy, 3);
        display.setCursor(0, 44); display.print("Z : "); display.println(gz, 3);
      }

      display.setCursor(100, 56); display.print(page + 1); display.print("/3");
      display.display();
    }

    // -------- Changement de page toutes les 2s --------
    if (now - lastPage >= PAGE_CHANGE_MS) {
      lastPage = now;
      page = (page + 1) % 3;
    }
  }


void setup1() {    // Ecriture (coeur 2)
  delay(12000);

  SPI.setRX(16);   // MISO
  SPI.setSCK(18);  // SCK
  SPI.setTX(19);   // MOSI

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
  if (buffer >= BUFFER_SAVE_LENGTH){
    buffer = 0;
    datasave.flush();
  }
}