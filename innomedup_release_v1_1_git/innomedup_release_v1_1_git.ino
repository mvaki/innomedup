//changelog
//v1_1
//Fixed wrong time on email
//v1_0
//Disabled battery reading for NO_PCB version

//TO DO



#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "HTML.h"
#include "EEPROM.h"
#include <WebServer.h>
#include "time.h"


//Provide the token generation process info.
#include <addons/TokenHelper.h>

//Needed for the OLED
#include "SSD1306Wire.h"


WebServer server(80);

#include "WiFiManager.h"

#define BIN_VERSION 0 //0 NO_PCB, 1 PCB,2 MINI_PCB
#ifndef LED_BUILTIN
#define LED_BUILTIN 2 // pin number is specific to your esp32 board
#endif 

//For Notification Emails

//#include <ESP32TimeHelper.h>
#include <ESP_Mail_Client.h>

bool enableEmail = true;
#define NOTIFICATION_EMAIL "yourlocal@gmail.com"
#define NOTIFICATION_PASSWORD "yourpassword"
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 587
#define SENDER_NAME "InnomedUP"


//void smtpCallback(SMTP_Status status);
void sendEmail(const String recipient, String message);

//For Firebase Firestore

#define USER_EMAIL "yourmail@gmail.com" //replace with your email
#define USER_PASSWORD "yourpassword"
#define API_KEY  "AskForAPI"
#define FIREBASE_PROJECT_ID "AskForProjectID"


//Define FirebaseESP32 data object for data sending and receiving
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

byte macAddr[6];
String macString;

//Pinout for OLED and ultrasound

#if BIN_VERSION == 0 //NO PCB
  #define usPowerPin 19
  #define usEchoPin 5
  #define usTriggerPin 18
  #define OLED_POWER 23
  #define OLED_SDA 21
  #define OLED_SCL 22 
#elif BIN_VERSION == 1 //PCB
  #define usPowerPin 17
#elif BIN_VERSION == 2 //MINI PCB
  #define usPowerPin 17
  #define usEchoPin 18
  #define usTriggerPin 5
#endif

//For ultrasound
#define usPresent true
#define distanceThresholdUpperLimit 100 //in cm
#define distanceThresholdBottomLimit 5

//For OLED
#if (BIN_VERSION==0 || BIN_VERSION==1) 
  #define oledPresent true
  SSD1306Wire display(0x3c, OLED_SDA, OLED_SCL); 
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

#define fullButton 27

//Data stored in EEPROM
String ssidString;
String passwordString;
int checkInterval=60; //time between sensor checks
int statusInterval=600; //when to post the devices Status
String location; //Devices location

#define SerialBaud 115200
int debug=true;


//For time 
//const char* ntpServer = "pool.ntp.org";
const char* ntpServer = "0.europe.pool.ntp.org";
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
  delay(1000); //wait for everything to settle 
  Serial.begin(SerialBaud);
  /********************BatteryMeasurement********************/
  #if (BIN_VERSION==1 || BIN_VERSION==2)
    pinMode(batteryMeasurementControlPin,OUTPUT);
  #endif
  
  if(oledPresent) pinMode(OLED_POWER,OUTPUT);
  if(batteryMeasurement) {
    batteryVoltage=readBatteryPercentage(batteryAnalogPin,batteryMeasurementControlPin);
    batteryPercentage=map(batteryVoltage,1.55,2.2,0,100);
    if(batteryVoltage<1.55){
      if(debug) Serial.println("Low Battery!");
      if(sleepOnLowBattery){
        esp_sleep_enable_ext0_wakeup(to_gpio(WiFiButton), 0); //wake up if WiFi button is pressed
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
    }   
  }
  
  /********************HC-SR04P initialize**************/
  if(usPresent==true){
    pinMode(usEchoPin,INPUT);
    pinMode(usTriggerPin,OUTPUT);
    pinMode(usPowerPin,OUTPUT);
  }
  
  /**************************Buttons********************/
  //rtc_gpio_deinit(WiFiButton);
  #if BIN_VERSION == 0 //NOPCB
    pinMode(WiFiButton,INPUT_PULLUP); //no external pullup needed 
//    Serial.println("INPUT PULLUP");
  #elif BIN_VERSION == 0 //NO PCB
    pinMode(WiFiButton,INPUT);
  #endif
  pinMode(fullButton,INPUT);
  
  /********************Firebase initialize****************/
  firebaseInit(); // dont call this function or firebase.begin another time because it creates an error
  

  /**************************Time*************************/
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  if(newCredentials){
    if(debug) Serial.println("New Credentials Found.");
    newCredentials=false;
    isBinNew();
    if(oledPresent){
      if(debug) Serial.println("Printing on OLED");
      macString=WiFi.macAddress();
      displayDebugMessage("Your MAC is:",false,0,false);
      displayDebugMessage(macString,true,15000,true);
    }
    uploadStatus(); //make sure setting changes are uploaded
  }
   
}

void loop(){
  if(!digitalRead(WiFiButton)){ //if WiFi button is pressed for 3 seconds or more enter WiFi input credentials mode
    unsigned long timenow=millis();
    delay(3000);
    if(!digitalRead(WiFiButton)){ //if button is pressed for 3 seconds 
      newCredentials=true;
      if(debug) Serial.println("Wiping WiFi credentials from memory...");
      wipeEEPROM();
      while(loadWIFICredsForm(APSSID,APPassword));
    }
  }
  

  if(hardReset){ //When turned on check if Bin has been registered before
    if(debug) Serial.println("System was hard Reset");
    macString=WiFi.macAddress();
    displayDebugMessage("Your MAC is:",false,0,false);
    displayDebugMessage(macString,true,15000,true);
    isBinNew();
    WiFi.mode(WIFI_OFF); //if we put this here it fixes the connectWiFi fail after the light sleep of checkIfFull.
    delay(10);
    previouslyFull=checkIfFull(previouslyFull,hardReset);
    binIsFull(debug,previouslyFull);
    hardReset=false;
  }
  else{
    if(usPresent){
      bool isFull=checkIfFull(previouslyFull,hardReset);
      if (previouslyFull!=isFull){
        previouslyFull=isFull;
        binIsFull(debug,isFull);
      }
    } 
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

void isBinNew(){ //if the bin is new initialize all the neccessary values to null
//connect to WiFi  
  connectWiFi(debug,ssidString.c_str(),passwordString.c_str());
  String documentPath = "SmartBins/"+macString;
  if (!Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str())){
    if(debug) Serial.println("Firebase couldnt get document:"+String(fbdo.errorReason()));
  };
  if (fbdo.errorReason().endsWith("not found.")){
    if(debug) Serial.println("Unregistered Bin. Uploading null values");
    std::vector<struct fb_esp_firestore_document_write_t> writes;
    //A write object that will be written to the document.
    //struct fb_esp_firestore_document_write_t transform_write;
    struct fb_esp_firestore_document_write_t update_write;
    update_write.type = fb_esp_firestore_document_write_type_update;
    FirebaseJson content;
    #if batteryMeasurement 
      content.set("fields/batteryLevel/stringValue", "null");
    #endif
    content.set("fields/timeEmptied/stringValue", "null");
    content.set("fields/timeFilled/stringValue", "null");
    content.set("fields/weight/stringValue", "null");
    content.set("fields/isFull/stringValue", "null");
    update_write.update_document_content = content.raw();
    update_write.update_document_path = documentPath.c_str();
    writes.push_back(update_write);
    if (!Firebase.Firestore.commitDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, writes /* dynamic array of fb_esp_firestore_document_write_t */, "" /* transaction */))
            Serial.println(fbdo.errorReason());
  }
  else{
    if(debug) Serial.println("Bin Already Registered");
  }
}

bool setFirebaseFirestoreValue(String varname[], String value[],int numberOfValues){
    //The dyamic array of write object fb_esp_firestore_document_write_t.
  std::vector<struct fb_esp_firestore_document_write_t> writes;
  //A write object that will be written to the document.
  //struct fb_esp_firestore_document_write_t transform_write;
  struct fb_esp_firestore_document_write_t update_write;
  update_write.type = fb_esp_firestore_document_write_type_update;
  FirebaseJson content;
  String documentPath = "SmartBins/"+macString;
  String mask="";
  for(int i=0; i<numberOfValues;++i){
    content.set("fields/"+varname[i]+"/stringValue", String(value[i]).c_str());
    if (i==numberOfValues-1) mask+=varname[i];
    else mask+=varname[i]+',';
  }
  update_write.update_document_content = content.raw();
  update_write.update_document_path = documentPath.c_str();
  //Set the document mask field paths that will be updated
  //Use comma to separate between the field paths
  update_write.update_masks = mask.c_str();

  //Add a write object to a write array.
  writes.push_back(update_write);

  if (!Firebase.Firestore.commitDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, writes /* dynamic array of fb_esp_firestore_document_write_t */, "" /* transaction */))
            //Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
        //else
            Serial.println(fbdo.errorReason());
}

String getFirebaseFirestoreValue(String varname){
  FirebaseJson content;
  String documentPath = "SmartBins/"+macString;
  String mask = varname;
  if (Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), mask.c_str())){
    String recipientEmail=fbdo.payload().c_str();
    //Serial.println("Email String is "+ fbdo.jsonString());
    //decode json string
    int index1 = recipientEmail.indexOf("\"recipientEmail\":");
    if (index1!=-1) {
      index1 = recipientEmail.indexOf("\"stringValue\":",index1);
      index1 = recipientEmail.indexOf("\"",index1+14);
      int index2 = recipientEmail.indexOf("\"",index1+1);
      recipientEmail=recipientEmail.substring(index1+1,index2);
      return recipientEmail;
    }
    else{
      if(debug) Serial.println("recipientEmail was not found");
      return "null";
    }
  }
  else{
    Serial.println("Could not get email recipient:"+String(fbdo.errorReason()));
    return "null";
  }      
}

void uploadStatus(){
  if(debug) Serial.println("Uploading Status");
  //firebaseInit(); //make sure firebase is initialized
  //connect to WiFi  
  connectWiFi(debug,ssidString.c_str(),passwordString.c_str());
  char timeNow[40];
  getTime(timeNow,&timestruct);
  #if batteryMeasurement 
    String fireVariables[5] = {"isFull","batteryLevel","checkInterval","statusInterval","TimeUpdated"};
    String fireValues[5] = {previouslyFull?"yes":"no",String(batteryPercentage),String(checkInterval),String(statusInterval),String(timeNow)+" "+gmtOffset_str};
    setFirebaseFirestoreValue(fireVariables,fireValues,5);
  #else
    String fireVariables[4] = {"isFull","checkInterval","statusInterval","TimeUpdated"};
    String fireValues[4] = {previouslyFull?"yes":"no",String(checkInterval),String(statusInterval),String(timeNow)+" "+gmtOffset_str};
    setFirebaseFirestoreValue(fireVariables,fireValues,4);
  #endif

  if(debug) Serial.println("Uploaded");
}

void binIsFull(bool debug,bool emptyOrFull){
  //connect to WiFi  
  displayDebugMessage(emptyOrFull?"Your Bin is Full":"Your Bin is Empty",false,10000,true);
  connectWiFi(debug,ssidString.c_str(),passwordString.c_str());
  char timeNow[40];
  getTime(timeNow,&timestruct);
  //firebaseInit();
  String recipientEmail=getFirebaseFirestoreValue("recipientEmail");
  bool recipientFound=true;
  if (recipientEmail=="null") recipientFound=false;
  //send command to server
  if(debug)Serial.println("Sending Value to Firebase");
  //setFirebaseFirestoreValue(macString,2);
  #if batteryMeasurement
    String fireVariables[3] = {"isFull","batteryLevel",emptyOrFull?"timeFilled":"timeEmptied"};
    String fireValues[3] = {emptyOrFull?"yes":"no",String(batteryPercentage),String(timeNow)+" "+gmtOffset_str};
    setFirebaseFirestoreValue(fireVariables,fireValues,3);
  #else
    String fireVariables[2] = {"isFull",emptyOrFull?"timeFilled":"timeEmptied"};
    String fireValues[2] = {emptyOrFull?"yes":"no",String(timeNow)+" "+gmtOffset_str};
    setFirebaseFirestoreValue(fireVariables,fireValues,2);
  #endif
  //read recipient emails from firebase (1 for bin sender and 1 for receiver)
  if(enableEmail&&recipientFound){
    if(debug) Serial.println("Sending mail to "+ recipientEmail);
    if(emptyOrFull) sendEmail(recipientEmail.c_str(),"Your Bin is Full. Please arrange pickup");
    else sendEmail(recipientEmail.c_str(),"Your Bin is Empty now");
  }
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
  delay(100); //wait for power on
  unsigned long t1 = micros(),t2;
  digitalWrite(TriggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(TriggerPin, LOW);
  // Wait for pulse on echo pin
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
  delay(1);
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
   if(!getLocalTime(&timeinfo,12000)){
    if(debug) Serial.println("Failed to obtain time.Trying again");
    if(!getLocalTime(&timeinfo,12000)){
      if(debug) Serial.println("Failed again");
      return;
    }
    if(debug) Serial.println("Time obtained");
  }
  strftime(timeNow,40, "%A, %d/%m/%Y %H:%M:%S", &timeinfo);
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


void sendEmail(const char *recipient, String text){ 
  SMTPSession smtp;
  ESP_Mail_Session session;
  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = NOTIFICATION_EMAIL;
  session.login.password = NOTIFICATION_PASSWORD;
  //session.login.user_domain = "client domain or ip e.g. mydomain.com";
  session.time.gmt_offset = gmtOffset_hr;
  session.time.day_light_offset = daylightOffset_sec/3600;
  SMTP_Message message;
  message.sender.name = SENDER_NAME;
  message.sender.email = NOTIFICATION_EMAIL;
  message.subject = "Bin Status Changed";
  message.addRecipient("", recipient);
  //message.addRecipient("name2", "email2");
  message.text.content = text;
  // Connect to server with the session config
  smtp.connect(&session);

  // Start sending Email and close the session
  if (!MailClient.sendMail(&smtp, &message))
    Serial.println("Error sending Email, " + smtp.errorReason());
    
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

void displayDebugMessage(String message, bool addMessage, int offTime, bool turnOffAfterwards){
  if (addMessage){
    display.println(message);
    display.drawLogBuffer(0, 0);
    display.display();
  }
  else{
    digitalWrite(OLED_POWER,HIGH);
    delay(500);
    display.init();
    display.setContrast(255);
    display.setLogBuffer(5, 30);
    //display.println("Your MAC is:");
    display.println(message);
    display.drawLogBuffer(0, 0);
    display.display();
  }
  delay(offTime);
  if(turnOffAfterwards){
    display.displayOff();
    digitalWrite(OLED_POWER,LOW);
  }
}

inline void firebaseInit(){
    /********************Firebase initialize****************/
  config.api_key = API_KEY;
  /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
    /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  //Firebase.setMaxRetry(&fbdo, 3);
  fbdo.setResponseSize(1024);
}
