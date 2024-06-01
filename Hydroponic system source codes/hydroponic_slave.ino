#include <ThingSpeak.h>
#include <ArduinoHttpClient.h>
#include <WiFi.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <DHT.h>

//---------- WiFi SSID and Password Setup ----------
const char *ssid = "your WiFi name";
const char *password = "your WiFi password";

//---------- ThingSpeak ID Setup ----------
const char* THINGSPEAK_READ_API_KEY = "TIOXXXXXXXXXXXXX"; //Replace with your THINGSPEAK_READ_API_KEY master channel

const char* THINGSPEAK_HOST = "api.thingsp3eak.com";
const int THINGSPEAK_PORT = 80;
const int THINGSPEAK_READ_CHANNEL_ID = 237XXXX; // Replace with your THINGSPEAK_READ_CHANNEL_ID master channel 

WiFiClient client;
HttpClient httpClient = HttpClient(client, THINGSPEAK_HOST, THINGSPEAK_PORT);

//---------- THINGSPEAK ID SETUP ----------
const char* THINGSPEAK_API_KEY = "NOVK6SSWK3SG6NGW"; // Replace with your THINGSPEAK_API_KEY slave channel
const int THINGSPEAK_CHANNEL_ID = 232XXXX; // Replace with your THINGSPEAK_CHANNEL_ID slave channel

//---------- WiFi Connection ----------
void connectToWiFi() {
  Serial.print("PROGRESS: CONNECTING TO WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 20000) {
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

//---------- PH & EC READ VARIABLE DECLARATION ----------
double ecValue = 0;               //get ec value read from ThingSpeak channel
double phValue = 0;               //get pH value read from ThingSpeak channel
long nowId = 0;                   //get current data ID from ThingSpeak channel
long prevId = 0;                  //get previous data ID from ThingSpeak channel
float dataec = 0.0;               //check ec value has been updated or not due to WiFi connection
float dataph = 0.0;               //check pH value has been updated or not due to WiFi connection

int eccount = 0;                  //to access current ecarray element
float ecarray[1000] = {0.0};      //array to hold current and previous ec value
int preveccount = 0;              //to access previous ecarray element 

int phcount = 0;                  //to access current pharray element
float pharray[1000];              //array to hold current and previous pH value
int prevphcount = 0;              //to access previous pharray element

float finalec = 0.0;              //finalize ec value for pump regulation
float finalph = 0.0;              //finalize pH value for pump regulation

int readflag = 0;                 //ec value read flag
float pec = 0.0;                  //store previous ec value for comparison purpose
float cec = 0.0;                  //store current ec value for comparision purpose

int readphflag = 0;               //ph value read flag
int ecphcontrol = 0;              //check whether ecph sensors are immersed into nutrient or not
float pph = 0.0;                  //store previous pH value for comparison purpose
float cph = 0.0;                  //store current pH value for comparison purpose
int ecdone = 0;                   //flag for ec regulation completion

//---------- SENSORY VARIABLE DECLARATION ----------
#define FILTER_SIZE 10                                //array size of moving average filter                          
double filtered_LDR, LDR, room_temperature, humidity; //store filtered data of LDR, room temperature, room humidity
double nutrient_temperature;                          //store filtered data of nutrient temperature
int waterLevel;                                       //store water tank level status (0 or 1)
int nutrientLevel;                                    //store nutrient tank level status (0 or 1)
int npumpState = HIGH;                                //toggling variable for nutrient pump
int lightState = HIGH;                                //toggling variable for light switch
int coolingState = HIGH;                              //toggling variable for fan switch
int pumpABState = HIGH;                               //toggling variable for nutrient AB pump
int pumpPHUPState = HIGH;                             //toggling variable for pH solution up pump
int pumpPHDOWNState = HIGH;                           //toggling variable for pH solution down pump
int pumpwaterState = HIGH;                            //toggling variable for water pump          

int lightflag = 0;                                    //light status flag
int pumpflag = 0;                                     //pump status flag 
int ABflag = 0;                                       //pump AB status flag
int PHflag = 0;                                       //pump PH status flag   

//---------- INPUT TEST PIN DEFINITION ----------
#define LDR_INPUT_PIN 34                  //LDR sensor input pin
#define DHT11_INPUT_PIN 13                //DHT11 sensor input pin
#define DHTTYPE DHT11                     //DHT11 definition
#define FLOAT_SENSOR_WATER 18             //Water tank level sensor input pin
#define FLOAT_SENSOR_RESERVOIR 19         //Reservoir tank level sensor input pin
DHT dht(DHT11_INPUT_PIN, DHTTYPE);        //DHT11 sensor library activation
OneWire oneWire(33);                      //Liquid temperature input sensor
DallasTemperature DS18B20(&oneWire); 

//---------- OUTPUT RELAY PIN DEFINITION ----------
#define LIGHT_RELAY_PIN 25                //light activation pin
#define COOLING_RELAY_PIN 27              //fan activation pin
#define RESERVOIR_RELAY_PIN 4             //nutrient pump activation pin 
#define WATER_TO_RESERVOIR_RELAY_PIN 23   //water pump activation pin
#define A_RELAY_PIN 26                    //Nutrient A pump
#define B_RELAY_PIN 15                    //Nutrient B pump
#define UP_RELAY_PIN 14                   //pH up buffer pump
#define DOWN_RELAY_PIN 5                  //pH down buffer pump

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
SensorData ldrData, roomTempData, humidityData, nutrientTempData;

//---------- OLED DISPLAY SETUP ----------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//---------- MILLIS VARIABLE DECLARATION ----------
const unsigned long updateInterval = 15000;      //15 seconds = 15000ms
unsigned long lastUpdate = 0;
const unsigned long lightinterval1 = 43200000;  //12 hours = 43200000ms
const unsigned long pumpinterval = 900000;      //15 minutes = 900000ms
const unsigned long coolinginterval = 900000;
const unsigned long nutpumpinterval = 2000;
unsigned long currentMillis = 0;              //record current time
unsigned long currentMillis2 = 0;
unsigned long previousMillis1 = 0;            //record previous time of nutrient pump
unsigned long previousMillis2 = 0;            //record previous time of light activation
unsigned long previousMillis3 = 0;            //record previous time of fan activation
unsigned long previousMillis4 = 0;            //record previous time of ec and pH measurement 
unsigned long previousMillis5 = 0;            //record previous time of pump AB activation
unsigned long previousMillis6 = 0;            //record previous time of pump PHUP activation
unsigned long previousMillis7 = 0;            //record previous time of pump PHDOWN activation
unsigned long previousMillis8 = 0;            //record previous time of water pump activation

void setup(){
  Serial.begin(115200);

  //---------- OLED STARTUP ATTEMPT ----------
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)){
    Serial.println(F("WARNING: OLED INITIALIZATION FAILED."));
    for (;;);
  }
  //---------- WiFi CONNECTION ATTEMPT ----------
  connectToWiFi();

  //---------- THINGSPEAK CONNECITON ATTEMPT ----------
  ThingSpeak.begin(client);

  //---------- OLED DISPLAY ATTEMPT ----------
  display.display();
  display.clearDisplay();

  //---------- DHT11 STARTUP ----------
  dht.begin();

  //---------- LIQUID TEMPERATURE SENSOR STARTUP ----------
  DS18B20.begin();

  //---------- FLOAT SENSOR INITIALIZATION ----------
  pinMode(FLOAT_SENSOR_WATER, INPUT_PULLUP);
  pinMode(FLOAT_SENSOR_RESERVOIR,INPUT_PULLUP);

  //---------- RELAY OUTPUT DECLARATION ----------
  pinMode(LIGHT_RELAY_PIN, OUTPUT);
  pinMode(RESERVOIR_RELAY_PIN, OUTPUT);
  pinMode(WATER_TO_RESERVOIR_RELAY_PIN, OUTPUT);
  pinMode(COOLING_RELAY_PIN, OUTPUT);
  pinMode(A_RELAY_PIN, OUTPUT);
  pinMode(B_RELAY_PIN, OUTPUT);
  pinMode(UP_RELAY_PIN, OUTPUT);
  pinMode(DOWN_RELAY_PIN, OUTPUT);

  //---------- RELAY DEACTIVATION ----------
  digitalWrite(LIGHT_RELAY_PIN, HIGH);
  digitalWrite(RESERVOIR_RELAY_PIN, HIGH);
  digitalWrite(COOLING_RELAY_PIN,HIGH);
  digitalWrite(WATER_TO_RESERVOIR_RELAY_PIN,HIGH);
  digitalWrite(COOLING_RELAY_PIN, HIGH);
  digitalWrite(A_RELAY_PIN, HIGH);
  digitalWrite(B_RELAY_PIN,HIGH);
  digitalWrite(UP_RELAY_PIN, HIGH);
  digitalWrite(DOWN_RELAY_PIN,HIGH);
}

void loop(){
  //---------- UPDATE CURRENT TIME ----------
  currentMillis = millis();

  // //---------- NUTRIENT TEMPERATURE DATA COLLECTION ----------  
  DS18B20.requestTemperatures();                          //activate temperature reading from DALLAS
  nutrient_temperature = DS18B20.getTempCByIndex(0);      //apply moving average
  nutrient_temperature = nutrientTempData.movingAverage(nutrient_temperature);

  //---------- ROOM CONDITION DATA COLLECTION ----------
  filtered_LDR = ldrData.movingAverage(analogRead(LDR_INPUT_PIN));      //get room intensity analog data
  LDR = map(filtered_LDR, 0, 4095, 100, 0);                             //map intensity analog data from 0% to 100%
  room_temperature = roomTempData.movingAverage(dht.readTemperature()); //get room temperature analog data from DHT11
  humidity = humidityData.movingAverage(dht.readHumidity());            //get room humidity analog data from DHT11
  waterLevel = digitalRead(FLOAT_SENSOR_WATER);                         //get water tank level
  nutrientLevel = digitalRead(FLOAT_SENSOR_RESERVOIR);                  //get nutrient tank level

  //---------- CHECK WHETHER NUTRIENT LEVEL IS ENOUGH ----------
  if(!nutrientLevel){                                                   //if nutrient level is low
    //digitalWrite(RESERVOIR_RELAY_PIN, LOW); //                            //turn on the water pump (pump water into reservoir tank)
    digitalWrite(WATER_TO_RESERVOIR_RELAY_PIN, LOW);
    pumpflag = 0;
  }

  else{                                                                 //if nutrient tank is enough
    digitalWrite(RESERVOIR_RELAY_PIN, LOW);
    //---------- DATA-WRITTEN THINGSPEAK FIELDS DECLARATION ----------
    ThingSpeak.setField(1, static_cast<float>(LDR));                    //send light intensity data to ThingSpeak
    ThingSpeak.setField(2, static_cast<float>(humidity));               //send humidity data to ThingSpeak
    ThingSpeak.setField(3, static_cast<float>(room_temperature));       //send room temperature data to ThingSpeak
    ThingSpeak.setField(4, static_cast<float>(nutrient_temperature));   //send nutrient temperature data to ThingSpeak
    ThingSpeak.setField(5, static_cast<int>(waterLevel));               //send nutrient temperature data to ThingSpeak
    ThingSpeak.setField(6, static_cast<int>(nutrientLevel));           //send nutrient temperature data to ThingSpeak
    ThingSpeak.writeFields(THINGSPEAK_CHANNEL_ID, THINGSPEAK_API_KEY);  //declare all the data-written ThingSpeak fields

    //---------- SENDING DATA TO THINGSPEAK ----------
    if (currentMillis - lastUpdate >= updateInterval){                                                //transmit data at 15 seconds interval
      ecValue = ThingSpeak.readFloatField(THINGSPEAK_READ_CHANNEL_ID, 1, THINGSPEAK_READ_API_KEY);    //update ec data
      phValue = ThingSpeak.readFloatField(THINGSPEAK_READ_CHANNEL_ID, 2, THINGSPEAK_READ_API_KEY);    //update pH data
      nowId = ThingSpeak.readLongField(THINGSPEAK_READ_CHANNEL_ID, 3, THINGSPEAK_READ_API_KEY);       //update data ID
      ecphcontrol = ThingSpeak.readLongField(THINGSPEAK_READ_CHANNEL_ID, 4, THINGSPEAK_READ_API_KEY); //get the ECPH control flag (0 or 1)

      //---------- THINGSPEAK DATA ID COMPARISON ----------
      if (nowId != prevId){ //if new ec and pH data are updated in ThingSpeak
        dataec = ecValue;   //store the new ec value
        dataph = phValue;   //store the new pH value
      }
      else{                 //if new ec and pH data are not updated in ThingSpeak
        dataec = 88;        //indicate as 88
        dataph = 88;        //indicate as 88
      } 
      prevId = nowId;       //store the current data ID as previous ID 
      lastUpdate = currentMillis; //update time for next interval
    }

    //---------- LIGHT MANAGEMENT ----------
    if (LDR <40.0){                                                 //if light intensity is lower than 30%
      digitalWrite(LIGHT_RELAY_PIN,LOW);
    }
    else{                                                            //if light intensity is higher than 30%
      digitalWrite(LIGHT_RELAY_PIN, HIGH);                           //deactivate the light
    }

    //---------- COOLING MANAGEMENT ----------
    if (room_temperature >10.0){                                      //if room temperature is greater than 40C
      if (currentMillis - previousMillis3 >= coolinginterval){        //toggle the fan at 
        previousMillis3 = currentMillis;                              //update time for next interval
        coolingState = (coolingState == HIGH) ? LOW : HIGH;           //toggle if-else function
        digitalWrite(COOLING_RELAY_PIN, coolingState);                //send activation signal to cooling system
      }
    }

    else{                                                             //if room temperature is below 40C
      digitalWrite(COOLING_RELAY_PIN, HIGH);                          //deactivate the cooling system
    }

    //---------- ECPH DATA MANAGEMENT ----------
    if(dataec == 88 and dataph == 88){                                //if no new ec and pH data from ThingSpeak
      ecphcontrol = 0;                                                // set ecph control flag as LOW
    }

    //---------- FINALIZING EC VALUE ----------                     
    if(dataec != 88 and ecphcontrol == 1){                            //if new ec data obtained and ecph sensors are ready
      preveccount = eccount;                                          //store the current index count as previous index count
      eccount = (eccount+1)%1000;                                     //increment current index count (rollover at 1000)
      ecarray[eccount] = dataec;                                      //store the ec value at current index of ecarray

      if ((ecarray[eccount] != ecarray[preveccount])){               //compare the current and previous ec value
        readflag = 1;                                                 
        cec = ecarray[eccount];                                       //store the current ec value into variable
        pec = ecarray[preveccount];                                   //store the previous ec value into variable

        if (abs(cec - pec) < 0.06){                                   //if absolute difference between them is lower than 0.06 (valid data)
          finalec = (cec + pec) /2;                                   //determine the average of both as the final ec value
        }
        else{                                                         //if abosolute difference between them is larger than 0.06 (invalid data)          
          finalec = 0.0;                                              //denote final ec value as 0 (reading in progress)
        }
      }
      else{                                                           //if the current and previous ec value are the same (data is absolutely stable)
        finalec = ecarray[eccount];                                   //denote final ec value as the current data                          
      }
  }
  else{                                                              //if new data and ecph sensors are not ready
    eccount = 0;                                                      //reset eccount as 0 (start over)
    preveccount = 0;                                                  //reset preveccount as 0 (start over)
    finalec = 0.0;                                                    //reset finalec as 0 (start over)
    pec = 0.0;                                                        //reset pec as 0 (start over)
    cec = 0.0;                                                        //reset pec as 0 (start over)
    ecarray[1000] = {0.0};                                            //reset ec array as 0 (start over)
  }

  //---------- FINALIZING PH VALUE ----------   
  if(dataph != 88 and ecphcontrol == 1){                              //if new pH data obtained and ecph sensors are ready
      prevphcount = phcount;                                          //store the current index count as previous index count                                    
      phcount = (phcount+1)%1000;                                     //increment current index count (rollover at 1000)
      pharray[phcount] = dataph;                                      //store the pH value at current index of ecarray

      if ((pharray[phcount] != pharray[prevphcount])){                //compare the current and previous pH value (different = new data)
        readphflag = 1;
        cph = pharray[phcount];                                       //store the current pH value into variable
        pph = pharray[prevphcount];                                   //store the previous pH value into variable

        if (abs(cph - pph) < 0.06){                                   //if absolute difference between them is lower than 0.06 (valid data)
          finalph = (cph + pph) /2;                                   //determine the average of both as the final ec value
        }
        else {                                                        //if abosolute difference between them is larger than 0.06 (invalid data)  
          finalph = 0.0;                                              //denote final pH value as 0 (reading in progress)
        }
      }
      else {
        finalph = pharray[phcount];                                   //denote final pH value as the current data 
      }
  }
  else{                                                              //if new data and ecph sensors are not ready
    phcount = 0;                                                     //reset phcount as 0 (start over)
    prevphcount = 0;                                                 //reset prevphcount as 0 (start over)
    finalph = 0.0;                                                   //reset finalph as 0 (start over)
    pph = 0.0;                                                       //reset pph as 0 (start over)
    cph = 0.0;                                                       //reset cph as 0 (start over)
    pharray[1000] = {0.0};                                           //reset ph array as 0 (start over)               
  }

  currentMillis2 = millis();                                         //declare new time for pump management

  //---------- EC PUMP MANAGEMENT ----------
  if (currentMillis - previousMillis4 >= 15000){                    //finalec and finalph are taken within 2 minutes for regulation
    previousMillis4 = currentMillis;                                 //update time for next interval
    
    if (finalec != 0.0 and finalec <0.8){                           //if finalec is ready and ec value is low
      digitalWrite(A_RELAY_PIN,LOW);                                //activate pump A
      digitalWrite(B_RELAY_PIN,LOW);                                //activate pump B
      digitalWrite(WATER_TO_RESERVOIR_RELAY_PIN,HIGH);              //deactivate water pump 
      pumpABState = LOW;                                            //set ABState as LOW
      pumpwaterState = HIGH;                                         //set waterState as HIGH
      ecdone = 0;                                                   //ec regulation not done
    }
    else if (finalec != 0.0 and finalec >= 0.8 and finalec <= 2.0){ //if finalec is ready and ec value falls in optimal condition 
      digitalWrite(A_RELAY_PIN,HIGH);                               //deactivate pump A
      digitalWrite(B_RELAY_PIN,HIGH);                               //deactivate pump B
      digitalWrite(WATER_TO_RESERVOIR_RELAY_PIN,HIGH);            //deactivate water pump 
      pumpABState = HIGH;                                           //set ABState as HIGH
      pumpwaterState = HIGH;                                         //set waterState as HIGH
      ecdone = 1;                                                   //ec regulation done (proceed to pH regulation)
    }
    else if (finalec != 0.0 and finalec >2.0){                      //if finalec is ready and finalec is high
      digitalWrite(A_RELAY_PIN,HIGH);                               //deactivate pump A
      digitalWrite(B_RELAY_PIN,HIGH);                               //deactivate pump B
      digitalWrite(WATER_TO_RESERVOIR_RELAY_PIN, LOW);              //activate water pump 
      pumpABState = HIGH;                                           //set ABState as HIGH
      pumpwaterState = LOW;                                         //set waterState as LOW 
      ecdone = 0;                                                   //ec regulation not done
    }
    else if (finalec == 0.0){                                       //if finalec is not ready
      digitalWrite(A_RELAY_PIN, HIGH);                              //deactivate pump A                          
      digitalWrite(B_RELAY_PIN,HIGH);                               //deactivate pump B
      digitalWrite(WATER_TO_RESERVOIR_RELAY_PIN,HIGH);              //deactivate water pump 
      pumpABState = HIGH;                                           //set ABState as HIGH
      pumpwaterState = HIGH;                                        //set waterState as HIGH
      ecdone = 0;                                                   //ec regulation not done
    }

    //---------- PH PUMP MANAGEMENT ----------    
    if (ecdone == 1 and finalph != 0.0 and finalph <6.0){                //if finalph is ready and ph value is low
      digitalWrite(UP_RELAY_PIN,LOW);                                   //activate pump PH UP
      digitalWrite(DOWN_RELAY_PIN,HIGH);                                //deactivate pump PH DOWN
      pumpPHUPState = LOW;                                              //set PHState as LOW
      pumpPHDOWNState = HIGH;                                           //set PHState as HIGH
    }
    else if (ecdone == 1 and finalph != 0.0 and finalph >= 6.0 and finalph <= 8.0){  //if finalph is ready and ph value falls in optimal condition 
      digitalWrite(UP_RELAY_PIN,HIGH);                                  //deactivate pump PHUP
      digitalWrite(DOWN_RELAY_PIN,HIGH);                                //deactivate pump PHDOWN
      pumpPHUPState = HIGH;                                             //set PHState as HIGH
      pumpPHDOWNState = HIGH;                                           //set PHState as HIGH
    }
    else if (ecdone == 1 and finalph != 0.0 and finalph >8.0){          //if finalph is ready and finalph is high
      digitalWrite(UP_RELAY_PIN,HIGH);                                  //deactivate pump PHUP
      digitalWrite(DOWN_RELAY_PIN,LOW);                                 //activate pump PHDOWN
      pumpPHUPState = HIGH;                                             //set PHState as HIGH
      pumpPHDOWNState = LOW;                                            //set PHState as LOW
    }
    else if (ecdone == 1 and finalph == 0.0){                           //if finalph is not ready
      digitalWrite(UP_RELAY_PIN, HIGH);                                 //deactivate pump PHUP                        
      digitalWrite(DOWN_RELAY_PIN,HIGH);                                //deactivate pump PHDOWN
      pumpPHUPState = HIGH;                                             //set PHState as HIGH
      pumpPHDOWNState = HIGH;                                           //set PHState as HIGH
    }
  }

    //---------- AB PUMP ON/OFF TIME CONTROL ----------
    if (pumpABState == LOW){                                          //if pump AB are activated
      if (currentMillis2 - previousMillis5 >= 2000){                  //activate pump AB for 1 seconds
        digitalWrite(A_RELAY_PIN,HIGH);                               //deactivate pump A once 5 seconds has reached 
        digitalWrite(B_RELAY_PIN,HIGH);                               //deactivate pump B once 5 seconds has reached
        previousMillis5 = currentMillis2;
      }
    }

    //---------- WATER PUMP ON/OFF TIME CONTROL ----------
    if (pumpwaterState == LOW){                                        //ionly triggered when EC is overly concentrated
      if (currentMillis2 - previousMillis8 >= 10000){                  //activate water pump for 10 seconds
        digitalWrite(WATER_TO_RESERVOIR_RELAY_PIN,HIGH);               //deactivate pump PHUP once 2 seconds has reached 
        previousMillis8 = currentMillis2; 
      }
    }  

    //---------- PHUP PUMP ON/OFF TIME CONTROL ----------
    if (pumpPHUPState == LOW){                                        //if PHUP is activated
      if (currentMillis2 - previousMillis6 >= 2000){                  //activate pump PHUP for 2 seconds
        previousMillis6 = currentMillis2; 
        digitalWrite(UP_RELAY_PIN,HIGH);                              //deactivate pump PHUP once 2 seconds has reached 
      }
    }

    //---------- PHDOWN PUMP ON/OFF TIME CONTROL ----------
    if (pumpPHDOWNState == LOW){                                      //if pump AB are activated
      if (currentMillis2 - previousMillis7 >= 2000){                   //activate pump AB for 2 seconds
        previousMillis7 = currentMillis2; 
        digitalWrite(DOWN_RELAY_PIN,HIGH);                            //deactivate pump PHDOWN once 2 seconds has reached 
      }
    }
  //}
}
  //---------- OLED DISPLAY ----------
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.print("ECPH: ");
  display.print(ecphcontrol);
  display.setCursor(0, 20);
  display.print("P: ");
  display.print(finalph, 2);
  display.setCursor(60, 20);
  display.print("E: ");
  display.print(finalec, 2);
  display.setCursor(0, 30);
  display.print("L: ");
  display.print(LDR, 2);
  display.setCursor(60, 30);
  display.print("H: ");
  display.print(humidity, 2);
  display.setCursor(0, 40);
  display.print("T: ");
  display.print(room_temperature, 2);
  display.setCursor(60, 40);
  display.print("nT: ");
  display.print(nutrient_temperature, 2);
  display.setCursor(0, 50);
  display.print("wL: ");
  display.print(waterLevel);
  display.setCursor(60, 50);
  display.print("nL: ");
  display.print(nutrientLevel);
  display.display();
  delay(10);
}
