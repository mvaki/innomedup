//changelog
//v1_5
//removed firebase
//removed mailing
//
//v1_4
//Changed oled printing
//Fixed email problem
//Fixed not reading distance with new us sensor
//v1_3
//Added weightThreshold on html
//Added option for PowerSaving mode on html
//On button click start scale calibration
//On uploadStatus also upload weight
//v1_2
//Added weight sensor
//v1_1
//Fixed wrong time on email
//v1_0
//Disabled battery reading for NO_PCB version

//TO DO
//add logo on oled


#include <WiFi.h>
#include "HTML.h"
#include "EEPROM.h"
#include <WebServer.h>
#include "time.h"
#include "HX711.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

//Needed for the OLED
#include "SSD1306Wire.h"


WebServer server(80);

#include "WiFiManager.h"

#define BIN_VERSION 1 //0 NO_PCB, 1 PCB,2 MINI_PCB
#ifndef LED_BUILTIN
#define LED_BUILTIN 2 // pin number is specific to your esp32 board
#endif 

//to upload to innomed up base
WiFiClientSecure wificlientsecure;
HTTPClient http;
#define apiUrl "https://intel.ps/innomedup/api/1.0/"
int binID;
byte macAddr[6];
String macString;

//Pinout for OLED, scale and ultrasound

#if BIN_VERSION == 0 //NO PCB
  #define usPowerPin 19
  #define usEchoPin 5
  #define usTriggerPin 18
  #define OLED_POWER 23
  #define OLED_SDA 21
  #define OLED_SCL 22
  #define HX711_CLK 32
  #define HX711_DT 33 
#elif BIN_VERSION == 1 //PCB
  #define usPowerPin 25
  #define usEchoPin 32
  #define usTriggerPin 14
  //#define OLED_POWER 23
  #define OLED_SDA 21
  #define OLED_SCL 22
  #define HX711_CLK 18
  #define HX711_DT 19 
#elif BIN_VERSION == 2 //MINI PCB
  #define usPowerPin 17
  #define usEchoPin 18
  #define usTriggerPin 5
#endif


//For weight sensor
#if BIN_VERSION == 0 //NO PCB
  #define scalePresent true
  HX711 scale;
  float loadCellCalibration=21.4; //Upload HX711calibration.ino to find loadCellCalibration and zeroFactor 
  //int zeroFactor=660228; //removes the need to tare
  int loadCellCalibrationInt=214;
  int zeroFactor=491461; //removes the need to tare
  float weight=0;
  int scaleCalibrated;
#elif BIN_VERSION == 1 //PCB
  #define scalePresent true
  HX711 scale;
  float loadCellCalibration=21.4; //Upload HX711calibration.ino to find loadCellCalibration and zeroFactor 
  int loadCellCalibrationInt=214;
  int zeroFactor=491461; //removes the need to tare
  float weight=0;
#elif BIN_VERSION == 2 //MINI PCB
  #define scalePresent false
#endif
//#define weightThreshold 1400 
int weightThreshold = 1400; //in grams. If measured weight exceeds this value the bin is considered full

//For ultrasound
#define usPresent true
#define distanceThresholdUpperLimit 80 //in cm
#define distanceThresholdBottomLimit 50

//For OLED
#if (BIN_VERSION==0 || BIN_VERSION==1) 
  #define oledPresent true
  SSD1306Wire display(0x3c, OLED_SDA, OLED_SCL, GEOMETRY_128_32); 
#else 
  #define oledPresent false
#endif

//Needed to input WiFi credentials
#define WiFiButton 26
//For battery management

#if (BIN_VERSION==1 || BIN_VERSION==2) //Only PCB and MiniPCB have the required circuit 
  #define batteryMeasurement true
#else 
  #define batteryMeasurement false
#endif

#define batteryMeasurementControlPin 13
#define batteryAnalogPin 33 //pin 9 
int sleepOnLowBattery=false; //Set to true if you want the system to sleep on Low Battery
float batteryVoltage,batteryPercentage;


const char * APSSID = "INNOMED-UP";
const char * APPassword = "upcycle!";

#define scaleButton 27

//Data stored in EEPROM
String ssidString;
String passwordString;
int checkInterval=60; //time between sensor checks
int statusInterval=600; //when to post the devices Status
String location; //Devices location
int powerSaving=true; //If enabled will turn off Display
int displayClock=true;

#define SerialBaud 115200
int debug=true;


//For time 
//const char* ntpServer = "pool.ntp.org";
const char* ntpServer = "pool.ntp.org";
long gmtOffset_sec = 0;
int gmtOffset_hr=0;
String gmtOffset_str;
const int daylightOffset_sec = 3600;
struct tm timestruct;

//RTC_DATA_ATTR keeps the variable after sleep or restart
RTC_DATA_ATTR int sleepCounter = 0;
RTC_DATA_ATTR bool previouslyFull=false;
RTC_DATA_ATTR bool hardReset=true; 
RTC_NOINIT_ATTR bool newCredentials=false; //carefull RTC DATA ATTR keeps values only after wake up and not restart


void setup()
{
  if(hardReset && esp_reset_reason()!=ESP_RST_SW) newCredentials=false; //when the system is hardReset check if we have newCredentials
  /********************ESP initialize********************/
  pinMode(BUILTIN_LED,OUTPUT);
  digitalWrite(BUILTIN_LED,HIGH);
  delay(1000); //wait for everything to settle 
  Serial.begin(SerialBaud);
  digitalWrite(BUILTIN_LED,LOW);
  /********************BatteryMeasurement********************/
  #if (BIN_VERSION==1 || BIN_VERSION==2)
    pinMode(batteryMeasurementControlPin,OUTPUT);
  #endif
  
  #if(oledPresent&&BIN_VERSION==2) //only minipcb has oledpower pin
    pinMode(OLED_POWER,OUTPUT);
  #endif
  if(batteryMeasurement) {
    batteryVoltage=readBatteryPercentage(batteryAnalogPin,batteryMeasurementControlPin);
    if(debug) Serial.println("batteryVoltage:"+String(batteryVoltage));
    batteryPercentage=map(constrain(int(batteryVoltage*100),155,200),155,200,0,100);
    if(debug) Serial.println("batteryPercentage:"+String(batteryPercentage));
    if(batteryVoltage<1.55){
      if(debug) Serial.println("Low Battery!");
      if(sleepOnLowBattery){
        esp_sleep_enable_ext0_wakeup(to_gpio(WiFiButton), 0); //wake up if WiFi button is pressed
        // #if scalePresent
        // esp_sleep_enable_ext0_wakeup(to_gpio(scaleButton), 0); //wake up if WiFi button is pressed
        // #endif
        uint64_t unsignedCheckInterval=(uint64_t)checkInterval*60000000;
        esp_sleep_enable_timer_wakeup(unsignedCheckInterval); //set timer to wake up the esp after the defined time
        delay(1);
        esp_deep_sleep_start();
      }
    }
  }
  /********************WiFi credentials********************/
  EEPROM.begin(400);
  if(!CheckWIFICreds()){
    if(debug) Serial.println("No WIFI credentials stored in memory. Loading form...");
    newCredentials=true;
    while(loadWIFICredsForm(APSSID,APPassword)); 
  }
  else{
    ssidString=EEPROM.readString(100);
    passwordString=EEPROM.readString(200);
    EEPROM.get(300,checkInterval);
    EEPROM.get(300+sizeof(int),statusInterval);
    EEPROM.get(300+2*sizeof(int),gmtOffset_hr);
    EEPROM.get(300+3*sizeof(int),debug);
    EEPROM.get(300+4*sizeof(int),sleepOnLowBattery);
    EEPROM.get(300+5*sizeof(int),powerSaving);
    EEPROM.get(300+6*sizeof(int),weightThreshold);
    EEPROM.get(300+7*sizeof(int),binID);
    EEPROM.get(300+8*sizeof(int),displayClock);
    #if scalePresent
      EEPROM.get(300+9*sizeof(int),zeroFactor);
      EEPROM.get(300+10*sizeof(int),loadCellCalibrationInt);
      loadCellCalibration=float(loadCellCalibrationInt/10.0);
      if(!loadCellCalibrationInt) loadCellCalibrationInt=-1; //sometimes is nan
    #endif
    if(checkInterval<=0||checkInterval>=1000000) checkInterval=60; //60mins is set as the maximum value.
    if (statusInterval==0) statusInterval=600;
    if(gmtOffset_hr<-12||gmtOffset_hr>14) gmtOffset_hr=0;
    gmtOffset_sec=gmtOffset_hr*3600;
    if(gmtOffset_hr>=0) gmtOffset_str="GMT:+"+String(gmtOffset_hr);
    else gmtOffset_str="GMT:"+String(gmtOffset_hr);

    if(debug){
      Serial.print("SSID:");
      Serial.println(ssidString.c_str());
      Serial.print("checkInterval:");
      Serial.println(checkInterval);
      Serial.print("statusInterval:");
      Serial.println(statusInterval);
      //Serial.print("gmtOffset:");
      Serial.println(gmtOffset_str);
      Serial.println("debug:"+String(debug));
      Serial.println("sleepOnLowBattery:"+String(sleepOnLowBattery));
      Serial.println("newCredentials:"+String(newCredentials));
      Serial.println("powerSaving:"+String(powerSaving));
      Serial.println("weightThreshold:"+String(weightThreshold));
      Serial.println("zeroFactor:"+String(zeroFactor));
      Serial.println("loadCellCalibration:"+String(loadCellCalibration));
      Serial.println("binID:"+String(binID));
    }   
  }
  
  /********************Scale initialize**************/
  #if scalePresent 
    calibrateScale(HX711_DT,HX711_CLK,loadCellCalibration,zeroFactor);
  #endif

  /********************HC-SR04P initialize**************/
  if(usPresent==true){
    pinMode(usEchoPin,INPUT);
    pinMode(usTriggerPin,OUTPUT);
    pinMode(usPowerPin,OUTPUT);
    digitalWrite(usTriggerPin, LOW);
  }
  
  /**************************Buttons********************/
  #if BIN_VERSION == 0 //NOPCB
    pinMode(WiFiButton,INPUT_PULLUP); //no external pullup needed 
    pinMode(scaleButton,INPUT_PULLUP);
//    Serial.println("INPUT PULLUP");
  #else //PCB and MINIPCB version have external pullups
    pinMode(WiFiButton,INPUT);
  #endif
  pinMode(scaleButton,INPUT);

  /**************************Time*************************/
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  if(newCredentials){
    if(debug) Serial.println("New Credentials Found.");
    newCredentials=false;
    //isBinNew();
    isBinRegistered(binID);
    #if(oledPresent)
    {
      if(debug) Serial.println("Printing on OLED");
      macString=WiFi.macAddress();
      displayDebugMessage("Your MAC is:",0,0,false,0,false,false);
      displayDebugMessage(macString,0,10,true,15000,true,false);
    }
    #endif
    uploadStatus(); //make sure setting changes are uploaded
  }
   
}

void loop(){
  #if scalePresent
    if(!digitalRead(scaleButton)){ //if scale button is pressed for 1 seconds or more enter calibration
      unsigned long timenow=millis();
      delay(1000);
      if(!digitalRead(scaleButton)){ //if button is pressed for 1 seconds 
        if(debug)Serial.println("Calibrating Scale");
        scaleCalibrationStart();
      }
    }
  #endif
  if(!digitalRead(WiFiButton)){ //if WiFi button is pressed for 3 seconds or more enter WiFi input credentials mode
    unsigned long timenow=millis();
    delay(3000);
    if(!digitalRead(WiFiButton)){ //if button is pressed for 3 seconds 
      newCredentials=true;
      if(debug) Serial.println("Wiping WiFi credentials from memory...");
      //wipe eeprom but keep scale calibration. 
      wipeEEPROM();
      #if scalePresent
        loadCellCalibrationInt=int(loadCellCalibration*10);
        EEPROM.put(300+9*sizeof(int),zeroFactor);
        EEPROM.put(300+10*sizeof(int),loadCellCalibrationInt);
        EEPROM.commit();
      #endif

      while(loadWIFICredsForm(APSSID,APPassword));
    }
  }
  

  if(hardReset){ //When turned on check if Bin has been registered before
    if(debug) Serial.println("System was hard Reset");
    macString=WiFi.macAddress();
    #if oledPresent
      displayDebugMessage("Your MAC is:",0,0,false,0,false,false);
      displayDebugMessage(macString,0,10,true,15000,true,false);
    #endif
    isBinRegistered(binID);
    WiFi.mode(WIFI_OFF); //if we put this here it fixes the connectWiFi fail after the light sleep of checkIfFull.
    delay(10);
    #if scalePresent
      weight=readScaleData2(debug);
      weight=constrain(weight,0,200000);
    #else 
      weight=0;
    #endif
    previouslyFull=checkIfFull(previouslyFull,hardReset);
    binIsFull(debug,previouslyFull);
    hardReset=false;
  }else{
    bool isFull;
    bool overWeight=false;
    #if(scalePresent)
    {
      weight=readScaleData2(debug);
      weight=constrain(weight,0,200000);
      if(debug) Serial.println("weight:"+String(weight));
      if(weight>=weightThreshold) {
        isFull=true;
        overWeight=true;
        if (previouslyFull!=isFull){
          previouslyFull=isFull;
          binIsFull(debug,isFull);
        }
      }else{
        isFull=false;
        overWeight=false;
        //Only if us is not present we will upload that the bin is empty just from the weight.
        if(!usPresent){
          if (previouslyFull!=isFull){
          previouslyFull=isFull;
          binIsFull(debug,isFull);
          }
        }
      }
    }
    #endif
    if(usPresent){
      if(!overWeight){ //if it is already overweight no need to check volume
        bool isFull=checkIfFull(previouslyFull,hardReset);
        if (previouslyFull!=isFull){
          previouslyFull=isFull;
          binIsFull(debug,isFull);
        }
      }
    } 
  }
  if(displayClock)//display Clock
    {
      char timeNow[30];
      getTime(timeNow,&timestruct);
      strftime(timeNow,40, "%H:%M", &timestruct);
      if(!powerSaving) displayDebugMessage(timeNow,32,0,false,10000,false,true);
      else displayDebugMessage(timeNow,2,0,false,10000,false,true);
    }
  if(sleepCounter*checkInterval>=statusInterval){
    sleepCounter=0;
    uploadStatus();
  }

  if(debug) Serial.println("SleepCounter:"+String(sleepCounter));
  sleepCounter++;
  esp_sleep_enable_ext0_wakeup(to_gpio(WiFiButton), 0); //wake up if WiFi button is pressed
  uint64_t unsignedCheckInterval=(uint64_t)checkInterval*60000000;
  esp_sleep_enable_timer_wakeup(unsignedCheckInterval); //set timer to wake up the esp after the defined time
  delay(1);
  esp_deep_sleep_start();
}

String readBinData(String binID){
  connectWiFi(debug,ssidString.c_str(),passwordString.c_str());
  wificlientsecure.setInsecure(); 
  http.begin(wificlientsecure, apiUrl);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  String httpRequestData = "api=bin_info&id="+String(binID); 
  Serial.println(httpRequestData);
  int httpResponseCode = http.POST(httpRequestData);
  if(debug) Serial.print("HTTP Response code: ");
  if(debug) Serial.println(httpResponseCode);
  if(httpResponseCode>0){
    String response = http.getString();  //Get the response to the request
    Serial.println(httpResponseCode);   //Print return code
    Serial.println(response);           //Print request answer
    http.end();
    return response;  
  }else{
    Serial.print("Error on sending POST: ");
    Serial.println(httpResponseCode);
    http.end();
    return String(httpResponseCode);
  }
}

void isBinRegistered(int binID){
  connectWiFi(debug,ssidString.c_str(),passwordString.c_str());
  wificlientsecure.setInsecure(); 
  http.begin(wificlientsecure, apiUrl);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  String httpRequestData = "api=bin&id="+String(binID); 
  Serial.println(httpRequestData);
  int httpResponseCode = http.POST(httpRequestData);
  if(debug) Serial.print("HTTP Response code: ");
  if(debug) Serial.println(httpResponseCode);
  if(httpResponseCode>0){
    String response = http.getString();  //Get the response to the request
    StaticJsonDocument<200> doc;
      // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, response);
    // Test if parsing succeeds.
    if (error) {
      if(debug) Serial.print(F("deserializeJson() failed: "));
      if(debug) Serial.println(error.f_str());
      return;
    }else{
      int code=doc["code"];
      if (code==3){
        if(debug) Serial.println("Bin is not Registered!");
      }else if(code==4){
        if(debug) Serial.println("Bin Already Registered!");
      }
    }
    if(debug) Serial.println();
    if(debug) Serial.println(httpResponseCode);   //Print return code
    if(debug) Serial.println(response);           //Print request answer
    http.end();
    //return response;  
  }else{
    if(debug) Serial.println("Bin not Registered!");
    if(debug) Serial.print("Error on sending POST: ");
    if(debug) Serial.println(httpResponseCode);
    http.end();
    //return String(httpResponseCode);
  }
}

void sendBinData(int binID, String varname[], String value[],int numberOfValues){
  connectWiFi(debug,ssidString.c_str(),passwordString.c_str());
  wificlientsecure.setInsecure(); 
  http.begin(wificlientsecure, apiUrl);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  String httpRequestData = "api=bin&id="+String(binID);
  for(int i=0;i<numberOfValues;++i){
    httpRequestData+='&'+varname[i]+'='+value[i];
  } 
  Serial.println(httpRequestData);
  int httpResponseCode = http.POST(httpRequestData);
  if(debug) Serial.print("HTTP Response code: ");
  if(debug) Serial.println(httpResponseCode);

}

void uploadStatus(){
  if(debug) Serial.println("Uploading Status");
  #if batteryMeasurement 
    #if scalePresent
      String fireVariables[3] = {"full","battery_level","weight"};
      String fireValues[3] = {previouslyFull?"1":"0",String(batteryPercentage),String(weight)};
      sendBinData(binID,fireVariables,fireValues,3);
    #else 
      String fireVariables[2] = {"full","battery_level"};
      String fireValues[2] = {previouslyFull?"1":"0",String(batteryPercentage)};
      sendBinData(binID,fireVariables,fireValues,2);
    #endif
  #else
    #if scalePresent
      String fireVariables[2] = {"full","weight"};
      String fireValues[2] = {previouslyFull?"1":"0",String(weight)};
      sendBinData(binID,fireVariables,fireValues,2);
    #else 
      String fireVariables[1] = {"full"};
      String fireValues[1] = {previouslyFull?1:0};
      sendBinData(binID,fireVariables,fireValues,1);
    #endif
  #endif

  if(debug) Serial.println("Uploaded");
}

void binIsFull(bool debug,bool emptyOrFull){
  //connect to WiFi
  #if oledPresent  
    displayDebugMessage(emptyOrFull?"Your Bin is Full":"Your Bin is Empty",0,0,false,10000,true,false);
  #endif
  connectWiFi(debug,ssidString.c_str(),passwordString.c_str());
  char timeNow[40];
  getTime(timeNow,&timestruct);
  bool recipientFound=true;
  //send command to server
  if(debug)Serial.println("Sending Value to Firebase");
  //setFirebaseFirestoreValue(macString,2);
  #if scalePresent
    #if batteryMeasurement
      String fireVariables[3] = {"full","battery_level","weight"};
      String fireValues[3] = {emptyOrFull?"1":"0",String(batteryPercentage),String(weight)};
      sendBinData(binID,fireVariables,fireValues,3);
//      setFirebaseFirestoreValue(fireVariables,fireValues,4);
    #else
      String fireVariables[2] = {"full","weight"};
      String fireValues[2] = {emptyOrFull?"1":"0",String(weight)};
      //setFirebaseFirestoreValue(fireVariables,fireValues,3);
      sendBinData(binID,fireVariables,fireValues,3);
    #endif
  #else
    #if batteryMeasurement
      String fireVariables[2] = {"full","battery_level"};
      String fireValues[2] = {emptyOrFull?"1":"0",String(batteryPercentage)};
      setFirebaseFirestoreValue(fireVariables,fireValues,2);
    #else
      String fireVariables[1] = {"full"};
      String fireValues[1] = {emptyOrFull?"1":"0"};
      setFirebaseFirestoreValue(fireVariables,fireValues,1);
    #endif
  #endif
  return;
}


inline int connectWiFi(bool debug,const char* ssid,const char* password){
  if(WiFi.status() == WL_CONNECTED) {
    if(debug) Serial.print("ESP32: Connected\n");
    return 1;
  }
  int timecounter=0;
  WiFi.begin(ssid,password);
  if (debug) Serial.print("ESP32: Connecting");
  while (WiFi.status() != WL_CONNECTED) { //Check for the connection
    delay(2000);
    timecounter++;
    if (debug) Serial.print(".");
    if(timecounter>20){
      timecounter=0;
      if (debug) Serial.print("ESP32: Could not connect to network. Please check your credentials\n"); 
      return 0;
    }
  }
  //Serial.println();
  if(debug) Serial.print("Connected\n");
  //WiFi.macAddress(macAddr);
  //macString=(char*)macAddr;
  macString=WiFi.macAddress();
  return 1;
}

float getDistance(int PowerPin, int TriggerPin,int EchoPin, unsigned long timeout){
    // Hold the trigger pin high for at least 10 us
  digitalWrite(PowerPin,HIGH); //enable power
  delay(600); //wait for power on
  unsigned long t1,t2;
  digitalWrite(TriggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(TriggerPin, LOW);
  // Wait for pulse on echo pin
  t1=micros();
  while (digitalRead(EchoPin) == 0 && (micros()-t1)<timeout);
  // Measure how long the echo pin was held high (pulse width)
  // Note: the micros() counter will overflow after ~70 min
  t1 = micros();
  while (digitalRead(EchoPin) == 1 && (micros()-t1)<timeout);
  t2 = micros();
  unsigned long pulse_width = t2 - t1;

  // Calculate distance in centimeters and inches. The constants
  // are found in the datasheet, and calculated from the assumed speed 
  //of sound in air at sea level (~340 m/s).
  digitalWrite(PowerPin,LOW); //disable power to preserve energy
  float  cm = pulse_width / 58.0;
  return cm;
}

float readBatteryPercentage(int analogPinNumber,int controlPin){
  digitalWrite(controlPin,HIGH);
  delay(3);
  digitalWrite(controlPin,LOW);
  delayMicroseconds(50);
  float batteryVoltage=analogRead(analogPinNumber)*3.3/4096; //divide by 4096
  //map(batteryVoltage,
  return batteryVoltage;
}

void wipeEEPROM(){
  for(int i=0;i<400;i++){
    EEPROM.writeByte(i,0);
  }
  EEPROM.commit();
}

void getTime(char * timeNow, struct tm *timestruct){
   struct tm timeinfo;
   configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
   if(!getLocalTime(&timeinfo,15000)){
    if(debug) Serial.println("Failed to obtain time.Trying again");
    if(!getLocalTime(&timeinfo,15000)){
      if(debug) Serial.println("Failed again");
      return;
    }
    if(debug) Serial.println("Time obtained");
  }
  strftime(timeNow,40, "%A, %d/%m/%Y\n%H:%M", &timeinfo);
  *timestruct=timeinfo;
}

gpio_num_t to_gpio(int pinNum){
  switch(pinNum){
  case(0):return GPIO_NUM_0;
  case(1):return GPIO_NUM_1;
  case(2):return GPIO_NUM_2;
  case(3):return GPIO_NUM_3;
  case(4):return GPIO_NUM_4;
  case(5):return GPIO_NUM_5;
  case(6):return GPIO_NUM_6;
  case(7):return GPIO_NUM_7;
  case(8):return GPIO_NUM_8;
  case(9):return GPIO_NUM_9;
  case(10):return GPIO_NUM_10;
  case(11):return GPIO_NUM_11;
  case(12):return GPIO_NUM_12;
  case(13):return GPIO_NUM_13;
  case(14):return GPIO_NUM_14;
  case(15):return GPIO_NUM_15;
  case(16):return GPIO_NUM_16;
  case(17):return GPIO_NUM_17;
  case(18):return GPIO_NUM_18;
  case(19):return GPIO_NUM_19;
  case(21):return GPIO_NUM_21;
  case(22):return GPIO_NUM_22;
  case(23):return GPIO_NUM_23;
  case(25):return GPIO_NUM_25;
  case(26):return GPIO_NUM_26;
  case(27):return GPIO_NUM_27;
  case(32):return GPIO_NUM_32;
  case(33):return GPIO_NUM_33;
  default: return GPIO_NUM_0;
  }
}

bool checkIfFull(bool previouslyFull, bool hardReset){
  float distance=getDistance(usPowerPin,usTriggerPin,usEchoPin,10000);
  int numberOfDetections=0;
  if(debug) Serial.println(previouslyFull?("Checking if Empty\nDistance:" + String(distance)):("Checking if Full\nDistance:" + String(distance)));
  if (previouslyFull==false||hardReset){
    if (distance<distanceThresholdBottomLimit||distance>distanceThresholdUpperLimit){
      //binIsFull(true);
      int numberOfDetections=0;
      for(int i=0; i<12;++i){
        float distance=getDistance(usPowerPin,usTriggerPin,usEchoPin,10000);
        
        if (distance<distanceThresholdBottomLimit||distance>distanceThresholdUpperLimit)
          numberOfDetections++;
        else break;
        if(debug) Serial.println("Distance:"+String(distance));
        esp_sleep_enable_timer_wakeup(5000000);
        //delay(100);
        esp_light_sleep_start();
      }
      if(debug) Serial.println("numberOfDetections:"+String(numberOfDetections));
      
      if(numberOfDetections==12) {
        hardReset=false; //setting hard reset here to make sure it has been decided if bin was initially full or empty
        return true;
      }
      else return false;
    }
  }
  if(previouslyFull==true||hardReset){
    if (distance>distanceThresholdBottomLimit&&distance<distanceThresholdUpperLimit){
      //binIsFull(true);
      int numberOfDetections=0;
      for(int i=0; i<12;++i){
        float distance=getDistance(usPowerPin,usTriggerPin,usEchoPin,10000);
        
        if (distance>distanceThresholdBottomLimit&&distance<distanceThresholdUpperLimit)
          numberOfDetections++;
        else break;
        if(debug) Serial.println("Distance:"+String(distance));
        esp_sleep_enable_timer_wakeup(5000000);
        //delay(100);
        esp_light_sleep_start();
      }
      if(debug) Serial.println("numberOfDetections:"+String(numberOfDetections));
      if(numberOfDetections==12) {
        hardReset=false; //setting hard reset here to make sure it has been decided if bin was initially full or empty
        return false;
      }
      else return true;
    }
  }
}

#if oledPresent
void displayDebugMessage(String message, int posx, int posy, bool addMessage, int offTime, bool turnOffAfterwards, bool printingTime){
  //posx and posy are 128x32
  if(printingTime) display.setFont(ArialMT_Plain_24);
  else display.setFont(ArialMT_Plain_10);
  if (addMessage){
    // display.println(message);
    // display.drawLogBuffer(0, 0);
    // display.display();
    display.drawString(posx,posy,message);
  }
  else{
    #if BIN_VERSION==2
      digitalWrite(OLED_POWER,HIGH);
    #endif
    delay(100);
    display.init();
    display.clear();
    display.drawString(posx,posy,message);
  }
  display.display();
  delay(offTime);
  if(turnOffAfterwards){
    display.displayOff();
    #if BIN_VERSION==2
    digitalWrite(OLED_POWER,LOW);
    #endif
  }
}
#endif


#if scalePresent
void scaleCalibrationStart(){
  displayDebugMessage("Scale Calibration",0,0,false,0,false,false);
  displayDebugMessage("Remove weight, press BTN",0,10,true,0,false,false);
  while(!digitalRead(scaleButton)); //wait to release button because we got here with button press
  delay(20);
  while(digitalRead(scaleButton)); //wait to press button
  scale.tare(); //Reset the scale to 0
  zeroFactor = scale.read_average(); //Get a baseline reading
  displayDebugMessage("Place a known weight",0,0,false,0,false,false);
  displayDebugMessage("Press buttons until accurate.",0,10,true,0,false,false);
  displayDebugMessage("When done press both buttons",0,20,true,5000,false,false);
  while((digitalRead(scaleButton))&&(digitalRead(scaleButton))); //wait to press button
  delay(100);
  loadCellCalibration=20.0;
  while(true){
    int units = scale.get_units();
    displayDebugMessage(String(units)+"gr",0,0,false,0,false,false);
    displayDebugMessage(String(loadCellCalibration),0,10,true,0,false,false);
    scale.set_scale(loadCellCalibration); //Adjust to this calibration factor
    if((!digitalRead(scaleButton))&&(!digitalRead(WiFiButton))) break;
    if(!digitalRead(scaleButton)) loadCellCalibration+=0.5;
    if(!digitalRead(WiFiButton)) loadCellCalibration-=0.5;
    delay(50);
  }
  EEPROM.put(300+9*sizeof(int),zeroFactor);
  loadCellCalibrationInt=int(loadCellCalibration*10);
  EEPROM.put(300+10*sizeof(int),loadCellCalibrationInt);
  EEPROM.commit();
  scale.set_scale(loadCellCalibration);                      // this value is obtained by calibrating the scale with known weights; see the README for details
  //scale.tare();                // reset the scale to 0
  scale.set_offset(zeroFactor);
  }
void calibrateScale(int DOUT, int SCK, float loadCellCalibration, int zeroFactor){
  scale.begin(DOUT, SCK);
  scale.set_scale(loadCellCalibration);                      // this value is obtained by calibrating the scale with known weights; see the README for details
  //scale.tare();                // reset the scale to 0
  scale.set_offset(zeroFactor);
}

float readScaleData2(bool debug){
  //scale.set_scale(loadCellCalibration);
  scale.power_up();
  // delay(10);
  // calibrateScale(HX711_DT,HX711_CLK,loadCellCalibration,zeroFactor);
  float measurment=scale.get_units(10); //average 10 readings
  if(debug) {
    Serial.print("weight2:");
    Serial.println(measurment);
  }
  scale.power_down();
  return measurment;
}
#endif
