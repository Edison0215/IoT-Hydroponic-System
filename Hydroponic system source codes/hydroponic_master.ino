#include <ThingSpeak.h>
#include <ArduinoHttpClient.h>
#include <WiFi.h>
#include <Adafruit_ADS1X15.h>
#include <DFRobot_ESP_EC_PH.h>
#include <OneWire.h>
#include <EEPROM.h>
#include <Wire.h>

int ecflag = 0;
//---------- WiFi SSID and Password Setup ----------
const char *ssid = "your WiFi name";
const char *password = "your WiFi password";

//---------- ThingSpeak ID Setup ----------
const char* THINGSPEAK_WRITE_API_KEY = "ZDKXXXXXXXXXX"; // Replace with your THINGSPEAK_WRITE_API_KEY master channel

const char* THINGSPEAK_HOST = "api.thingspeak.com";
const int THINGSPEAK_PORT = 80;
const int THINGSPEAK_WRITE_CHANNEL_ID = 237XXXX; // Replace with your THINGSPEAK_WRITE_CHANNEL_ID master channel 
WiFiClient client;
HttpClient httpClient = HttpClient(client, THINGSPEAK_HOST, THINGSPEAK_PORT);

//---------- WiFi Connection ----------
void connectToWiFi() {
  Serial.print("PROGRESS: CONNECTING TO WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 20000){
    Serial.print(".");
    delay(100);
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Failed");
  } else {
    Serial.println("Connected!");
    Serial.println(WiFi.localIP());
  }
}

//---------- Variable declaration ----------
#define FILTER_SIZE 10
double ECvoltage, PHvoltage, phValue, ecValue;
int status = 3; //calibration flag
long count = 0;
//---------- INPUT TEST PIN DEFINITION ----------
DFRobot_ESP_EC_PH ecph;  

//---------- ADS 1115 Setup ----------
Adafruit_ADS1115 ads;

//---------- Time variable ----------
const unsigned long updateInterval = 1000; // Update every 10 seconds
unsigned long lastUpdate = 0;

//---------- MOVING AVERAGE FILTER ALGORITHM ----------
struct SensorData {
  double buffer[FILTER_SIZE];
  int currentIndex = 0;

  double movingAverage(double newValue) {
    buffer[currentIndex] = newValue;
    currentIndex = (currentIndex + 1) % FILTER_SIZE;
    double sum = 0;
    for (int i = 0; i < FILTER_SIZE; i++) {
      sum += buffer[i];
    }
    return sum / FILTER_SIZE;
  }
};
SensorData ecData, phData;

void setup() {
  Serial.begin(115200);
  //---------- EEPROM DATA CLEANING ----------
  EEPROM.begin(4096); 
  for (int i = 0; i < 4096; i++) {
    EEPROM.write(i, 0);
    if (i == 0){
    Serial.println("PROGRESS: EEPROM CLEANING IN PROGRESS...");
    }
  }
  EEPROM.end();
  Serial.println("MESSAGE: EEPROM CLEANED.");
  //---------- ADS1115 INITIALIZATION VERIFICATION ----------
  ads.setGain(GAIN_ONE);
  if (!ads.begin()) {
    Serial.println("WARINING: ADS1115 INITIALIZATION FAILED.");
    while (1);
  }
  //---------- LIBRARY ACTIVATION ----------
  connectToWiFi();
  ThingSpeak.begin(client);
  ecph.begin(1000,3000);
  delay(1000);
}

void loop() {
  //---------- pH & EC DATA COLLECTION  ----------
    ECvoltage = ads.readADC_SingleEnded(1) / 10; // Read EC voltage
    PHvoltage = ads.readADC_SingleEnded(0) / 10; // Read pH voltage
    phValue = ecph.readPH(PHvoltage, 25); // Convert voltage to pH with temperature compensation
    phValue = phData.movingAverage(phValue); //apply moving average
    ecValue = ecph.readEC(ECvoltage, 25); // Convert voltage to EC with temperature compensation
    ecValue = ecData.movingAverage(ecValue); //apply moving average
  
  //---------- EC and PH CALIBRATION FEATURE ----------
    ecph.ECcalibration(ECvoltage, 25);
    ecph.PHcalibration(PHvoltage, 25);
    status = ecph.isCalibrated();

    Serial.print("\nEC: ");
    Serial.print(ecValue, 3);
    Serial.print("\tPH: ");
    Serial.print(phValue,3);

  //---------- UNCALIBRATED PH, CALIBRATED EC ----------
  if (status == 1){
    Serial.print("\tPH Calibrate: Pending");
    Serial.print("   EC Calibrate: Done");
  }
//---------- CALIBRATED PH, UNCALIBRATED EC ----------
  else if (status == 2){
    Serial.print("\tPH Calibrate: Done");
    Serial.print("   EC Calibrate: Pending");
  }
//---------- UNCALIBRATED PH & EC ----------
  else if (status == 3){
    Serial.print("\tPH Calibrate: Pending");
    Serial.print("   EC Calibrate: Pending");
  }
  else if (status == 0){//0
    if(ecph.ecphcontrol()){
      ecflag = 1;
    }
    else{
      ecflag = 0;
    }
    Serial.print("\tCalibratiion done; Transmitting data"); 
    Serial.print("\tECPHCONTROL: ");
    Serial.print(ecflag);
    ThingSpeak.setField(1, static_cast<float>(ecValue));
    ThingSpeak.setField(2, static_cast<float>(phValue));
    ThingSpeak.setField(3, static_cast<long>(count));
    ThingSpeak.setField(4, static_cast<int>(ecflag));
    ThingSpeak.writeFields(THINGSPEAK_WRITE_CHANNEL_ID, THINGSPEAK_WRITE_API_KEY);  
    count = count + 1;
  }
  delay(100);
}
