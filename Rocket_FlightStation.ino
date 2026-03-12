/*
 * ============================================================
 *  MaxIQ MK10 Rocket Flight Station Firmware v1.0
 *  Board:    CWA V2+ (ESP32-S3)
 *  Arduino:  Tools > Board > esp32 > ESP32S3 Dev Module
 *            Tools > Flash Size > 8MB
 *            Tools > USB CDC on Boot > Enabled (when uploading via USB-C)
 *                                    > Disabled (for standalone flight)
 * ============================================================
 *
 * HARDWARE:
 *   Core:     CWA V2+ (ESP32-S3FN8)
 *   IIA:      Accelerometer - LIS2DH12 (0x19) or KXTJ3-1057 (0x0F)
 *             Firmware auto-detects which chip is present
 *   IWB:      SPL06-001 Barometer/Thermometer (0x77)
 *   PLA:      AXP2101 Power Management (battery charging)
 *   EBS:      SD Card interface (SPI)
 *
 * PINS (CWA V2+):
 *   I2C SDA:  GPIO 17
 *   I2C SCL:  GPIO 18
 *   Red LED:  GPIO 40 (HIGH = on)
 *   Neopixel: GPIO 39
 *   SPI for EBS SD card - uses expansion port:
 *     MISO:   GPIO 38
 *     MOSI:   GPIO 37
 *     SCK:    GPIO 36
 *     CS:     GPIO 35 (X1)
 *
 * ============================================================
 * LIBRARIES TO INSTALL (Sketch > Manage Libraries)
 * ============================================================
 *   1. SparkFun LIS2DH12        (SparkFun)
 *   2. KXTJ3-1057               (ldab) - search "KXTJ3"
 *   3. XPowersLib               (lewisxhe) - search "XPowers"
 *   4. Freenove WS2812          (Freenove)
 *
 * MANUAL ZIP INSTALL:
 *   None required - SPL06 is now handled via direct I2C (ESP32-S3 compatible)
 *
 * ============================================================
 * LED STATUS CODES
 * ============================================================
 *   Neopixel BLUE   = Initializing
 *   Neopixel GREEN  = All OK, on pad, ready
 *   Neopixel YELLOW = Sensor warning (still logging)
 *   Neopixel RED    = SD card failed (critical - no logging)
 *   Neopixel WHITE  = LAUNCH detected
 *   Neopixel CYAN   = APOGEE detected
 *   Neopixel PURPLE = DESCENT
 *   Neopixel ORANGE = LANDED
 *   Red LED blink   = Writing to SD card
 *
 * ============================================================
 * FLIGHT STATE MACHINE
 * ============================================================
 *   PAD      -> accel < 2.5g, logging at 500ms
 *   LAUNCH   -> accel >= 2.5g for 3 consecutive reads
 *   COAST    -> accel drops back < 2.5g, altitude still rising
 *   APOGEE   -> pressure stops decreasing (altitude peaks)
 *   DESCENT  -> pressure increasing after apogee
 *   LANDED   -> accel stable near 1g for 10+ seconds
 *
 * ============================================================
 */

// ── Libraries ──────────────────────────────────────────────
#include <Wire.h>
#include <SPI.h>
#include "SD.h"  // Use ESP32 built-in SD library
#include <SparkFun_LIS2DH12.h>
#include <kxtj3-1057.h>

// ── Flight State enum - must be declared before any function that uses it ──
enum FlightState { PAD, LAUNCH, COAST, APOGEE, DESCENT, LANDED };

// ── SPL06-001 Barometer via direct I2C (ESP32-S3 compatible) ─
// SPL06-007.h uses direct register access that crashes on ESP32-S3
#define SPL06_ADDR        0x77
#define SPL06_PSR_B2      0x00
#define SPL06_TMP_B2      0x03
#define SPL06_PRS_CFG     0x06
#define SPL06_TMP_CFG     0x07
#define SPL06_MEAS_CFG    0x08
#define SPL06_CFG_REG     0x09
#define SPL06_COEF        0x10

// Calibration coefficients
int32_t  spl_c0, spl_c1;
int32_t  spl_c00, spl_c10;
int16_t  spl_c01, spl_c11, spl_c20, spl_c21, spl_c30;
float    spl_kP = 1040384.0;  // for 16x oversampling
float    spl_kT = 1040384.0;

uint8_t spl_readReg(uint8_t reg) {
  Wire.beginTransmission(SPL06_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)SPL06_ADDR, (uint8_t)1);
  return Wire.available() ? Wire.read() : 0;
}

void spl_writeReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(SPL06_ADDR);
  Wire.write(reg); Wire.write(val);
  Wire.endTransmission();
}

bool spl_init() {
  // Check device is present
  uint8_t id = spl_readReg(0x0D);
  if ((id & 0xF0) != 0x10) return false;  // SPL06 product ID = 0x1x

  // Read calibration coefficients (18 bytes starting at 0x10)
  Wire.beginTransmission(SPL06_ADDR);
  Wire.write(SPL06_COEF);
  Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)SPL06_ADDR, (uint8_t)18);
  uint8_t coef[18];
  for (int i = 0; i < 18; i++) coef[i] = Wire.available() ? Wire.read() : 0;

  spl_c0  = (int32_t)((coef[0] << 4) | (coef[1] >> 4));
  if (spl_c0 & 0x800) spl_c0 |= 0xFFFFF000;
  spl_c1  = (int32_t)(((coef[1] & 0x0F) << 8) | coef[2]);
  if (spl_c1 & 0x800) spl_c1 |= 0xFFFFF000;
  spl_c00 = (int32_t)((coef[3] << 12) | (coef[4] << 4) | (coef[5] >> 4));
  if (spl_c00 & 0x80000) spl_c00 |= 0xFFF00000;
  spl_c10 = (int32_t)(((coef[5] & 0x0F) << 16) | (coef[6] << 8) | coef[7]);
  if (spl_c10 & 0x80000) spl_c10 |= 0xFFF00000;
  spl_c01 = (int16_t)((coef[8]  << 8) | coef[9]);
  spl_c11 = (int16_t)((coef[10] << 8) | coef[11]);
  spl_c20 = (int16_t)((coef[12] << 8) | coef[13]);
  spl_c21 = (int16_t)((coef[14] << 8) | coef[15]);
  spl_c30 = (int16_t)((coef[16] << 8) | coef[17]);

  // Configure: 16x oversampling pressure and temp, background mode
  spl_writeReg(SPL06_PRS_CFG, 0x04);   // 1 meas/s, 16x oversampling
  spl_writeReg(SPL06_TMP_CFG, 0x84);   // 1 meas/s, 16x, use external sensor
  spl_writeReg(SPL06_MEAS_CFG, 0x07);  // continuous pressure and temp
  spl_writeReg(SPL06_CFG_REG,  0x04);  // pressure result shift (for 16x)
  delay(100);
  return true;
}

float spl_getPressure() {
  Wire.beginTransmission(SPL06_ADDR);
  Wire.write(SPL06_PSR_B2);
  Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)SPL06_ADDR, (uint8_t)3);
  int32_t raw = 0;
  if (Wire.available() >= 3) {
    raw = ((int32_t)Wire.read() << 16) | ((int32_t)Wire.read() << 8) | Wire.read();
    if (raw & 0x800000) raw |= 0xFF000000;
  }
  float Praw_sc = (float)raw / spl_kP;
  float Traw_sc;
  // Quick temp read for compensation
  Wire.beginTransmission(SPL06_ADDR);
  Wire.write(SPL06_TMP_B2);
  Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)SPL06_ADDR, (uint8_t)3);
  int32_t traw = 0;
  if (Wire.available() >= 3) {
    traw = ((int32_t)Wire.read() << 16) | ((int32_t)Wire.read() << 8) | Wire.read();
    if (traw & 0x800000) traw |= 0xFF000000;
  }
  Traw_sc = (float)traw / spl_kT;
  float pcomp = spl_c00 + Praw_sc * (spl_c10 + Praw_sc * (spl_c20 + Praw_sc * spl_c30))
              + Traw_sc * spl_c01
              + Traw_sc * Praw_sc * (spl_c11 + Praw_sc * spl_c21);
  return pcomp / 100.0;  // Pa to hPa
}

float spl_getTemp() {
  Wire.beginTransmission(SPL06_ADDR);
  Wire.write(SPL06_TMP_B2);
  Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)SPL06_ADDR, (uint8_t)3);
  int32_t raw = 0;
  if (Wire.available() >= 3) {
    raw = ((int32_t)Wire.read() << 16) | ((int32_t)Wire.read() << 8) | Wire.read();
    if (raw & 0x800000) raw |= 0xFF000000;
  }
  float Traw_sc = (float)raw / spl_kT;
  return spl_c0 * 0.5f + spl_c1 * Traw_sc;
}
#include "Freenove_WS2812_Lib_for_ESP32.h"
#define XPOWERS_CHIP_AXP2101
#include "XPowersLib.h"

// ── Pin Definitions (CWA V2+) ──────────────────────────────
#define I2C_SDA       17
#define I2C_SCL       18
#define RED_LED       40    // HIGH = on
#define NEOPIXEL_PIN  39
#define SD_MISO       38
#define SD_MOSI       37
#define SD_SCK        36
#define SD_CS         35

// ── I2C Addresses ──────────────────────────────────────────
#define LIS2DH12_ADDR  0x19
#define KXTJ3_ADDR     0x0F
#define AXP2101_ADDR   0x34

// ── Logging / Timing ───────────────────────────────────────
#define LOG_INTERVAL_MS    500    // Normal logging interval
#define LAUNCH_THRESHOLD_G 2.5   // g-force to detect launch
#define LAND_STABLE_SEC    10    // seconds of 1g stability = landed
#define APOGEE_WINDOW      5     // consecutive rising pressure = apogee

// ── Neopixel ───────────────────────────────────────────────
Freenove_ESP32_WS2812 *strip = nullptr;

// ── Sensor Objects ─────────────────────────────────────────
SPARKFUN_LIS2DH12 *accelLIS = nullptr;
KXTJ3 *accelKX = nullptr;
XPowersAXP2101 *pmu = nullptr;
SPIClass *sdSPI = nullptr;

// ── State ──────────────────────────────────────────────────
bool sdOK       = false;
bool lisOK      = false;   // LIS2DH12 found
bool kxOK       = false;   // KXTJ3 found
bool accelOK    = false;   // either accel found
bool baroOK     = false;
bool pmuOK      = false;
bool sensorWarn = false;

// ── Flight State Machine ────────────────────────────────────
FlightState flightState = PAD;

unsigned long lastLogTime    = 0;
unsigned long packetCount    = 0;
unsigned long landedTimer    = 0;
int           launchCount    = 0;   // consecutive high-g reads
int           apogeeCount    = 0;   // consecutive pressure-rising reads
float         lastPressure   = 0;
float         maxAltitude    = 0;
float         launchPressure = 0;   // pressure at launch for relative altitude
File          logFile;
String        logFileName;

// ── Neopixel Helper ────────────────────────────────────────
void setColor(int r, int g, int b) {
  if (strip == nullptr) return;
  strip->setLedColorData(0, r, g, b);
  strip->show();
}

// ── Relative Altitude from Pressure ────────────────────────
// Uses barometric formula: ~8.43m per hPa near sea level
float pressureToAltitude(float pressureHPa, float refPressureHPa) {
  return (refPressureHPa - pressureHPa) * 8.43;
}

// ── Get Total Acceleration Magnitude ───────────────────────
float getAccelMagnitude(float x, float y, float z) {
  return sqrt(x*x + y*y + z*z);
}

// ── Flight State Name ───────────────────────────────────────
const char* getStateName(FlightState s) {
  switch(s) {
    case PAD:     return "PAD";
    case LAUNCH:  return "LAUNCH";
    case COAST:   return "COAST";
    case APOGEE:  return "APOGEE";
    case DESCENT: return "DESCENT";
    case LANDED:  return "LANDED";
    default:      return "UNKNOWN";
  }
}

// ===========================================================
//  SETUP
// ===========================================================
void setup() {
  // ── LED Init ──
  pinMode(RED_LED, OUTPUT);
  digitalWrite(RED_LED, LOW);
  strip = new Freenove_ESP32_WS2812(1, NEOPIXEL_PIN, 0, TYPE_GRB);
  strip->begin();
  strip->setBrightness(40);
  setColor(0, 0, 255);  // BLUE = starting

  Serial.begin(115200);
  delay(1000);
  Serial.println("=== Rocket Flight Station v1.0 ===");

  Wire.begin(I2C_SDA, I2C_SCL);

  // ── AXP2101 Power Management (PLA) ──
  Serial.print("PLA Power... ");
  pmu = new XPowersAXP2101(Wire, I2C_SDA, I2C_SCL, AXP2101_ADDR);
  if (pmu->init()) {
    pmu->setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V2);
    pmu->setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_500MA);
    pmu->setDC1Voltage(3300);
    pmu->enableDC1();
    Serial.println("OK");
    pmuOK = true;
  } else {
    Serial.println("NOT FOUND"); sensorWarn = true;
  }

  // ── Accelerometer (IIA) - auto-detect LIS2DH12 or KXTJ3 ──
  Serial.print("IIA Accel LIS2DH12... ");
  accelLIS = new SPARKFUN_LIS2DH12();
  if (accelLIS->begin()) {
    Serial.println("OK (LIS2DH12)");
    lisOK = true; accelOK = true;
  } else {
    Serial.println("not found, trying KXTJ3...");
    accelKX = new KXTJ3(KXTJ3_ADDR);
    if (accelKX->begin(100, 2, true) == IMU_SUCCESS) {  // 100Hz, ±2g, high-res
      Serial.println("IIA Accel KXTJ3: OK");
      kxOK = true; accelOK = true;
    } else {
      Serial.println("IIA Accel: NOT FOUND"); sensorWarn = true;
    }
  }

  // ── SPL06 Barometer (IWB) ──
  Serial.print("IWB Baro... ");
  if (spl_init()) {
    lastPressure = spl_getPressure();
    Serial.println("OK - " + String(lastPressure, 1) + " hPa");
    baroOK = true;
  } else {
    Serial.println("NOT FOUND"); sensorWarn = true;
  }

  // ── SD Card (EBS) ──
  Serial.print("EBS SD Card... ");
  sdSPI = new SPIClass(HSPI);
  sdSPI->begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if (SD.begin(SD_CS, *sdSPI)) {
    // Create uniquely named log file
    int fileNum = 0;
    do {
      logFileName = "/FLIGHT" + String(fileNum) + ".CSV";
      fileNum++;
    } while (SD.exists(logFileName));

    logFile = SD.open(logFileName, FILE_WRITE);
    if (logFile) {
      // Write CSV header
      logFile.println(
        "Packet,Millis,State,"
        "AccelX_g,AccelY_g,AccelZ_g,AccelMag_g,"
        "Pressure_hPa,Temp_C,RelAlt_m,"
        "BattVolt_mV,BattPct,"
        "Event"
      );
      logFile.flush();
      Serial.println("OK - " + logFileName);
      sdOK = true;
    } else {
      Serial.println("FILE OPEN FAILED");
    }
  } else {
    Serial.println("INIT FAILED - CHECK SD CARD");
  }

  if (!sdOK) {
    setColor(255, 0, 0);  // RED = SD failed
    Serial.println("*** CRITICAL: No SD card - no data will be logged! ***");
    // Don't halt - still useful to see LED states and serial output
  }

  // ── Record launch pad pressure for relative altitude ──
  launchPressure = lastPressure;

  // ── Status LED ──
  if (!sdOK) {
    setColor(255, 0, 0);        // RED = SD failed
  } else if (sensorWarn) {
    setColor(255, 150, 0);      // YELLOW = sensor warning
  } else {
    setColor(0, 255, 0);        // GREEN = all good, on pad
  }

  Serial.println("=== Setup Complete - On Pad ===\n");
}

// ===========================================================
//  MAIN LOOP
// ===========================================================
void loop() {
  if (millis() - lastLogTime < LOG_INTERVAL_MS) return;
  lastLogTime = millis();
  packetCount++;

  // ── Read Accelerometer ──
  float accelX = 0, accelY = 0, accelZ = 0, accelMag = 0;
  if (lisOK && accelLIS->available()) {
    accelX = accelLIS->getX();
    accelY = accelLIS->getY();
    accelZ = accelLIS->getZ();
    accelMag = getAccelMagnitude(accelX, accelY, accelZ);
  } else if (kxOK) {
    accelX = accelKX->axisAccel(X);
    accelY = accelKX->axisAccel(Y);
    accelZ = accelKX->axisAccel(Z);
    accelMag = getAccelMagnitude(accelX, accelY, accelZ);
  }

  // ── Read Barometer ──
  float pressure = baroOK ? spl_getPressure() : 0;
  float temp_C   = baroOK ? spl_getTemp()     : 0;
  float relAlt   = pressureToAltitude(pressure, launchPressure);
  if (relAlt > maxAltitude) maxAltitude = relAlt;

  // ── Read Battery ──
  uint16_t battVolt = 0;
  int battPct = 0;
  if (pmuOK && pmu != nullptr) {
    battVolt = pmu->getBattVoltage();
    battPct  = constrain((battVolt - 3200) / 10, 0, 100);
  }

  // ── Flight State Machine ──
  String event = "";
  FlightState prevState = flightState;

  switch (flightState) {
    case PAD:
      if (accelOK && accelMag >= LAUNCH_THRESHOLD_G) {
        launchCount++;
        if (launchCount >= 3) {
          flightState = LAUNCH;
          event = "LAUNCH_DETECTED";
          launchPressure = pressure;  // Reset reference to actual launch pressure
          setColor(255, 255, 255);    // WHITE = launch!
          Serial.println("*** LAUNCH DETECTED ***");
        }
      } else {
        launchCount = 0;
      }
      break;

    case LAUNCH:
      if (accelMag < LAUNCH_THRESHOLD_G) {
        flightState = COAST;
        event = "MOTOR_BURNOUT";
        setColor(255, 165, 0);  // ORANGE-ish = coasting
        Serial.println("*** MOTOR BURNOUT - COASTING ***");
      }
      break;

    case COAST:
      // Apogee = pressure starts rising (altitude falling)
      if (pressure > lastPressure) {
        apogeeCount++;
        if (apogeeCount >= 3) {
          flightState = APOGEE;
          event = "APOGEE maxAlt=" + String(maxAltitude, 0) + "m";
          setColor(0, 255, 255);  // CYAN = apogee
          Serial.println("*** APOGEE - Max Alt: " + String(maxAltitude, 0) + "m ***");
        }
      } else {
        apogeeCount = 0;
      }
      break;

    case APOGEE:
      flightState = DESCENT;
      event = "DESCENT_START";
      setColor(128, 0, 128);  // PURPLE = descending
      Serial.println("*** DESCENDING ***");
      break;

    case DESCENT:
      // Landed = accel stable near 1g for LAND_STABLE_SEC seconds
      if (accelOK && accelMag > 0.8 && accelMag < 1.2) {
        if (landedTimer == 0) landedTimer = millis();
        if (millis() - landedTimer >= LAND_STABLE_SEC * 1000) {
          flightState = LANDED;
          event = "LANDED";
          setColor(255, 100, 0);  // ORANGE = landed
          Serial.println("*** LANDED ***");
        }
      } else {
        landedTimer = 0;
      }
      break;

    case LANDED:
      // Stay here - blink red LED slowly to signal recovery
      if ((millis() / 1000) % 2 == 0) {
        digitalWrite(RED_LED, HIGH);
      } else {
        digitalWrite(RED_LED, LOW);
      }
      break;
  }

  lastPressure = pressure;

  // ── Serial Output ──
  Serial.printf("[%s] #%lu | Accel: X%.2f Y%.2f Z%.2f (%.2fg) | "
                "P:%.1fhPa T:%.1fC Alt:%.1fm | Batt:%dmV %d%%\n",
    getStateName(flightState), packetCount,
    accelX, accelY, accelZ, accelMag,
    pressure, temp_C, relAlt,
    battVolt, battPct);
  if (event.length() > 0) {
    Serial.println("  EVENT: " + event);
  }

  // ── Log to SD Card ──
  if (sdOK) {
    digitalWrite(RED_LED, HIGH);  // Blink red LED during write

    String csv =
      String(packetCount) + "," +
      String(millis()) + "," +
      String(getStateName(flightState)) + "," +
      String(accelX, 3) + "," + String(accelY, 3) + "," +
      String(accelZ, 3) + "," + String(accelMag, 3) + "," +
      String(pressure, 2) + "," + String(temp_C, 1) + "," +
      String(relAlt, 1) + "," +
      String(battVolt) + "," + String(battPct) + "," +
      event;

    logFile.println(csv);
    logFile.flush();  // Ensure data written - important for crash recovery

    digitalWrite(RED_LED, LOW);
  }
}
