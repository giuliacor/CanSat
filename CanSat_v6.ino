#include <Adafruit_AHTX0.h>
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>
#include <Servo.h>
#include <string>
#include <sd_defines.h>
#include <sd_diskio.h>
#include <LoRa.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <Wire.h>

Adafruit_AHTX0 aht;
SFE_UBLOX_GNSS myGPS;
Servo myservo;
bool parachute_deployed = false;
int posDegrees = 180;
int LED = 17;
float altitude_aux = 0.0;
float altitude = 0.0;
int counter = 1;
int servoPin = 23;

const int RFM_CS = 16;
const int RFM_INT = 26;



void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT); //LED
  Wire.begin();

  //---SERVO SETUP---
  myservo.attach(servoPin);
  //myservo.write(posDegrees);

  //---GPS SETUP---
  if(myGPS.begin() == true){
  myGPS.setI2COutput(COM_TYPE_UBX);
    myGPS.saveConfiguration();
    Serial.println("GPS setup Successful!");
  } else{
    Serial.println("GPS failed to start");
    while(1);    
  }

  //---BME280 SETUP---
  Serial.println("Adafruit AHT10/AHT20 demo!");
  if (! aht.begin()) {
    Serial.println("Could not find AHT? Check wiring");
    while (1) delay(10);
  }
  Serial.println("AHT10 or AHT20 found");

  //---SD SETUP---
  /*if(!SD.begin(5)){
    Serial.println("Card Mount Failed");
    //while(1);
  }else{
    Serial.println("Card Mount Successful!");
  }
  uint8_t cardType = SD.cardType();*/

  //---LoRa SETUP---
   Serial.println("LoRa Sender");
   LoRa.setPins(RFM_CS, -1, RFM_INT);
  if (!LoRa.begin(866E6)) {
    Serial.println("Starting LoRa failed!");
    //overwrite(SD, "/temp_humidity.txt", "NEW MEASUREMENTS:");
    //return;
    while (1);
  }

  LoRa.setTxPower(2); 
  LoRa.setFrequency(868E6); // frequency - frequency in Hz (433E6, 868E6, 915E6)
  LoRa.setSpreadingFactor(7); // ranges from 6-12, default 7
  LoRa.setSignalBandwidth(125E3);     // defaults to 125E3. Supported values are 7.8E3, 10.4E3, 15.6E3, 20.8E3, 31.25E3, 41.7E3, 62.5E3, 125E3, 250E3, and 500E3.
  LoRa.setCodingRate4(5); // Supported values are between 5 and 8, these correspond to coding rates of 4/5 and 4/8. The coding rate numerator is fixed at 4.
  LoRa.setPreambleLength(8); //preambleLength - preamble length in symbols, defaults to 8. Supported values are between 6 and 65535.
  LoRa.setSyncWord(0xF3);   // ranges from 0-0xFF, default 0x34
  LoRa.disableCrc();
  LoRa.disableInvertIQ();
  Serial.println("LoRa Initializing OK!");
  //OVERWRITING SD DATA
  //overwrite(SD, "/temp_humidity.txt", "NEW MEASUREMENTS:");*/
}

void loop() {
  Serial.println("========================================================================================");
  //---blink---
  digitalWrite(LED, HIGH);
  delay(100);
  digitalWrite(LED, LOW);
  delay(100);

  //---GPS---
  altitude = transmit_gps();

  //Parachute deployment system 1

  if (counter++ > 5){ 
  Serial.println("checking parachute...");
    check_parachute(altitude, altitude_aux);
  }

  //---Sensor to SD---
  read_write_sensor();
  
  
  //Parachute deployment system 2
 Serial.print("Altitude_t-1: ");
 Serial.println(altitude_aux);
 Serial.print("Altitude_t: ");
 Serial.println(altitude);
  altitude_aux = altitude;
  counter++;
  delay(500);
}

//========FUNCTIONS========

//GPS
float transmit_gps(){
    float latitude = myGPS.getLatitude();
    latitude = latitude / 10000000;
    float longitude = myGPS.getLongitude();
    longitude = longitude / 10000000;
    float time = myGPS.getHour();
    float altitude = myGPS.getAltitude();
    altitude = altitude / 1000;
    Serial.print("time:"); Serial.print(time); Serial.println(" ");
    Serial.print("latitude:"); Serial.print(latitude, 5); Serial.println(" ");
    Serial.print("longitude: "); Serial.print(longitude, 5); Serial.println(" ");
    Serial.print("altitude: "); Serial.print(altitude); Serial.println(" ");
    LoRa.beginPacket();
    if(!LoRa.beginPacket()){
      Serial.println("Packet error");
      return altitude;
    }
    Serial.println("Sending GPS Data:");
    LoRa.print("time:"); LoRa.print(time); LoRa.println(" ");
    LoRa.print("latitude:"); LoRa.print(latitude); LoRa.println(" ");
    LoRa.print("longitude: "); LoRa.print(longitude); LoRa.println(" ");
    LoRa.print("altitude: "); LoRa.print(altitude); LoRa.println(" ");
    LoRa.endPacket();
    return altitude;
}
//SD
void overwrite(fs::FS &fs, const char * path, const char * message) {
    File file = fs.open(path, FILE_WRITE);
    Serial.printf("Writing in file: %s\n", path);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}
void appendFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if(!file){
    Serial.println("Failed to open file for appending");
    return;
  }
  if(file.print(message)){
      Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}
void read(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\n", path);
  File file = fs.open(path);
  if(!file){
      Serial.println("Failed to open file for reading");
      return;
  }
  Serial.print("Read from file: ");
  while(file.available()){
      Serial.write(file.read());
  }
  file.close();
}
//BME280
void read_write_sensor() {
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);// populate temp and humidity objects with fresh data
  Serial.print("Temperature: "); Serial.print(temp.temperature); Serial.println(" degrees C");
  Serial.print("Humidity: "); Serial.print(humidity.relative_humidity); Serial.println("% rH");
  char temp_str_aux[8];
  dtostrf(temp.temperature, 6, 2, temp_str_aux);
  char hum_str_aux[8];
  dtostrf(humidity.relative_humidity, 6, 2, hum_str_aux);
  char temp_str[128];
  strcpy(temp_str,"Temperature: ");
  strcat(temp_str,temp_str_aux);
  strcat(temp_str," degrees C");
  char hum_str[128];
  strcpy(hum_str, "Humidity: ");
  strcat(hum_str, hum_str_aux);
  strcat(hum_str, "% rH");
  LoRa.beginPacket();
  LoRa.println(temp_str);
  LoRa.println(hum_str);
  LoRa.endPacket();
  /*appendFile(SD, "/temp_humidity.txt",temp_str);
  appendFile(SD, "/temp_humidity.txt", "\n");
  appendFile(SD, "/temp_humidity.txt", hum_str);
  appendFile(SD, "/temp_humidity.txt", "\n");*/
}
//Parachute functions
void check_parachute(float altitude, float altitude_aux){
  float precision = 1.0;
 if ( (altitude_aux - altitude > precision) && (parachute_deployed == false) ){
   deploy_parachute();
   Serial.println("Deploying parachute!");
   LoRa.beginPacket();
   if(!LoRa.beginPacket()){
      LoRa.print("Packet error: deploy");
   }
   LoRa.print("Deploying parachute!");
   Serial.println("Sending parachute info");
   LoRa.endPacket();
 }
 else if (parachute_deployed == false) {   
   Serial.println("Parachute not deploying yet");
   LoRa.beginPacket();
   if(!LoRa.beginPacket()){
      LoRa.print("Packet error: not_deploy");
   }
   LoRa.print("Parachute not deploying yet");
   Serial.println("Sending parachute info");
   LoRa.endPacket();
 }
 else if (parachute_deployed == true){
   Serial.println("Parachute has deployed!");
   LoRa.beginPacket();
   if(!LoRa.beginPacket()){
      LoRa.print("Packet error: deployed");
   }
   LoRa.print("Parachute has deployed!");
   Serial.println("Sending parachute info");
   LoRa.endPacket();
 }
}
void deploy_parachute(){
  myservo.write(180);
  delay(220);
  myservo.write(90);
  parachute_deployed = true;
}
