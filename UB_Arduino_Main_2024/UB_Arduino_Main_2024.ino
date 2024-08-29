
/* Benötigte Libraries:
Schadstoffsensor:
  Adafruit PM25 AQI Sensor
GPS:
  TinyGPSPlus
DHT:
  DHT sensor library
*/

// Hier sollen die #include's, #define's und (nicht Methoden) Variablen stehen.
#include "UB_Arduino_Extra_2024.h"

#include <Wire.h>

// Schadstoffsensor Start
#include "Adafruit_PM25AQI.h"
Adafruit_PM25AQI aqi_front = Adafruit_PM25AQI();
Adafruit_PM25AQI aqi_back = Adafruit_PM25AQI();
Adafruit_PM25AQI aqi_bottom = Adafruit_PM25AQI();
const uint8_t front_select = 0;
const uint8_t back_select = 1;
const uint8_t bottom_select = 2;
// Schadstoffsensor Fertig

// GPS Start
#include "TinyGPS++.h"
TinyGPSPlus gps;
#define gpsSerial Serial1
// GPS Fertig

// DHT/Luftfeuchtigkeit und Temperatur Start
#include "DHT.h"
#define DHTTYPE DHT22
#define DHT22_Pin 7
DHT dht(DHT22_Pin, DHTTYPE);
// DHT/Luftfeuchtigkeit und Temperatur Fertig


// Erstellung dder verschiedenen Status und Error Nachrichten
const Nachricht<Nachricht_Art::STATUS, Nachricht_Unterart::SETUP_FERTIG> statusSetupFertig;
const Nachricht<Nachricht_Art::STATUS, Nachricht_Unterart::PRINT> statusPrint;
const Nachricht<Nachricht_Art::ERROR, Nachricht_Unterart::PRINT> errorPrint;
/* Benutzung:
statusSetupFertig::send()           // 1-mal am Ende vom Setup benutzen
statusPrint::send(nachricht)        // benutzen um eine Status Nachricht zu senden
errorPrint::send(nachricht)         // benutzen um eine Error Nachricht zu senden
*/


// Erstelle ein Sensor_UB Struct für jeden Sensor
const Sensor_UB GPS("GPS", 6);
const Sensor_UB Schadstoffsensor_front("particleFront", 6);
const Sensor_UB Schadstoffsensor_back("particleBack", 6);
const Sensor_UB Schadstoffsensor_bottom("particleBottom", 6);
const Sensor_UB Luftfeuchte_DHT("humid_DHT", 1);
const Sensor_UB Temperatur_DHT("Temp_DHT", 1);

//Function to switch i2c multiplexer
void TCA9548A(uint8_t bus) {
  Wire.beginTransmission(0x70);  // TCA9548A address is 0x70
  Wire.write(1 << bus);          // send byte to select bus
  Wire.endTransmission();
  //Serial.print(bus);
}

void setup() {
  // Generelles Setup
  Serial.begin(57600);
  statusPrint.send("Verbindung Erfolgreich");

  // Sensoren Setup
  gpsSerial.begin(9600);
  statusPrint.send("GPS begonnen");

  dht.begin();
  statusPrint.send("DHT begonnen");

  TCA9548A(front_select);
  while (!aqi_front.begin_I2C()) {
    TCA9548A(front_select);
    errorPrint.send("Could not find front particle sensor");
    delay(10);
  }
  statusPrint.send("front particle sensor PMSA003I found");

  TCA9548A(back_select);
  while (!aqi_back.begin_I2C()) {
    TCA9548A(back_select);
    errorPrint.send("Could not find back particle sensor!");
    delay(10);
  }
  statusPrint.send("back particle sensor PMSA003I found");

  TCA9548A(bottom_select);
  while (!aqi_bottom.begin_I2C()) {
    TCA9548A(bottom_select);
    errorPrint.send("Could not find bottom particle sensor!");
    delay(10);
  }
  statusPrint.send("bottom particle sensor PMSA003I found");

  statusSetupFertig.send();
  delay(1000);
}


void loop() {
  GPSstuff();
  temp_luftStuff();
  SchadstoffStuff(front_select);
  SchadstoffStuff(back_select);
  SchadstoffStuff(bottom_select);
  delay(500);  // Verögerung zwischen Lesungen
}
void GPSstuff() {
  unsigned long start = millis();
  bool newdata = false;

  while (millis() - start < 3000) {
    if (gpsSerial.available()) {
      char c = gpsSerial.read();
      if (gps.encode(c)) {
        newdata = true;
        break;
      }
    }
  }

  if (newdata && gps.location.isValid()) {
    String gps_Values[6];

    gps_Values[0] = String(gps.location.lat(), 6);   // Latitude
    gps_Values[1] = String(gps.location.lng(), 6);   // Longitude
    gps_Values[2] = String(gps.altitude.meters());   // Altitude (Höhe)
    gps_Values[3] = String(gps.speed.kmph());        // Geschwindigkeit
    gps_Values[4] = String(gps.hdop.value());        // Horizontal Diminution of Precision (Horizontale Abnahme der Genauigkeit)
    gps_Values[5] = String(gps.satellites.value());  // Number of Satellites

    const String gps_Values_Array[] = { gps_Values[0], gps_Values[1], gps_Values[2], gps_Values[3], gps_Values[4], gps_Values[5] };

    GPS.sendValues(gps_Values_Array);
  } else {
    if (newdata) {
      statusPrint.send("No new GPS Data");
    } else {
      statusPrint.send("Invalid GPS data");
    }
  }
}


void SchadstoffStuff(uint8_t sensorSelect) {
  TCA9548A(sensorSelect);
  PM25_AQI_Data data;

  if (sensorSelect == front_select) {
    if (!aqi_front.read(&data)) {
      errorPrint.send("Could not read from AQI");
      return;
    }
  } else if (sensorSelect == back_select) {
    if (!aqi_back.read(&data)) {
      errorPrint.send("Could not read from AQI");
      return;
    }
  } else {
    if (!aqi_bottom.read(&data)) {
      errorPrint.send("Could not read from AQI");
      return;
    }
  }

  String pm03, pm05, pm10, pm25, pm50, pm100;

  // "Particles > 0.3um / 0.1L air:"
  pm03 = String(data.particles_03um);
  // "Particles > 0.5um / 0.1L air:"
  pm05 = String(data.particles_05um);
  // "Particles > 1.0um / 0.1L air:"
  pm10 = String(data.particles_10um);
  // "Particles > 2.5um / 0.1L air:"
  pm25 = String(data.particles_25um);
  // "Particles > 5.0um / 0.1L air:"
  pm50 = String(data.particles_50um);
  // "Particles > 10 um / 0.1L air:"
  pm100 = String(data.particles_100um);

  const String pm_Values_Array[] = { pm03, pm05, pm10, pm25, pm50, pm100 };

  if (sensorSelect == front_select) {
    Schadstoffsensor_front.sendValues(pm_Values_Array);
  } else if (sensorSelect == back_select) {
    Schadstoffsensor_back.sendValues(pm_Values_Array);
  } else {
    Schadstoffsensor_bottom.sendValues(pm_Values_Array);
  }
}


void temp_luftStuff() {
  const String humidity = String(dht.readHumidity());
  const String temperature = String(dht.readTemperature());

  Luftfeuchte_DHT.sendValues(&humidity);
  Temperatur_DHT.sendValues(&temperature);
}
