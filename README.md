# innomedup

Materials used:
 
 1x ESP32 Dev Kit 
 
 1x HC-SR04P (Be careful to use to version with the P as it is 3.3V compatible) 
 
 1x HX711 
 
 4x Load Cells 
 
 1x OLED 128x32 I2C 
 
 1x Battery Holder with battery charger 
 
 1x Slide Switch  
 
 1x Push Button 
 
 Wire 

***Step 1.*** Connect the hardware for the NO_PCB version as in the picture below. 
![innomedup_nopcb_variant_v0 2](https://user-images.githubusercontent.com/37118897/153802363-54a3113d-c0b0-47d4-bd40-260771c730d4.jpg)

***Step 2.*** Install Arduino IDE
Install the latest version of Arduino IDE from
https://www.arduino.cc/en/software

***Step 3.*** Install the ESP library by following the instructions on 
https://randomnerdtutorials.com/installing-the-esp32-board-in-arduino-ide-windows-instructions/

***Step 4.*** Install the required libraries
On Arduino go to Sketch->Include Library->Manage Libraries

Download HX711 Arduino Library by Bogdan Necula, Andreas Motl (0.7.4)

(For version V1.4 only!) Download Firebase Arduino Client Library for Espressif ESP8266 and ESP32 by Mobizt (2.8.3)

(For version V1.4 only!) Download ESP Mail Client by Mobizt (1.6.4)

Download ESP8266 and ESP32 OLED driver for SSD1306 displays by ThingPulse,Weinberg (4.2.1)


The program has been tested with the versions mentioned above. Please install these versions. 

***Step 5.*** (For version V1.4 only!)  Create a gmail account and change security settings so that esp can send emails.
For Greece the email is innomedupgrc@gmail.com
When you create the email go to follow the instructions in this link (https://randomnerdtutorials.com/esp32-send-email-smtp-server-arduino-ide/) to get an app password.
Use this password for NOTIFICATION_PASSWORD and if you want for USER_PASSWORD too (this can be different)

***Step 6.*** Before uploading any sketch upload a simple sketch like Blink located in File->examples->01.Basics to make sure you have selected the right version of ESP32.
When you upload Blink you should see an LED light flashing on your ESP32 Board.

***Step 7.*** This requires that you have set up your bin's scale if there is one. If there is no scale please set #define scalePresent false in the code for your bin version. You need to calibrate the scale and find the loadCellCalibration and zeroFactor values.
To do so, use one of the following options:
1) Make sure the scale is connected to the ESP32. Upload HX711calibration.ino and open the serial monitor on ArduinoIDE (press ctrl+shift+M).
On the monitor set Serial Speed to 115200. When you start the program make sure there is no weight on the scale to measure the the zeroFactor (tare).\
Now put a known weight on the scale that is above 1Kg and use the serial monitor to send + or - until the measured weight is close to the real value.
When you get really close send / or * to get as close as possible. When you are done send =. Save the values for loadCellCalibration and zeroFactor as you will need them in the next step.
2) (Only for PCB version) Finish all the steps and when you turn on the power to the system have the scale button pressed for some time. You should see on your display instructions to calibrate the scale.
First remove the weight from the scale (not the wooden plates or whatever will be placed there permenantly) and press the button again.
Then place a known weight and when you are ready to start press a button. 
A weight measurement should appear on the display as well as the calibration factor. Use the buttons to change the factor until the weight that appears matches the actual weight of the object you placed. 
When done press both buttons at the same time to exit. 
 
***Step 8.*** Open innomedup_release_v1_4_git.ino. Before uploading the code to your ESP32 change the following lines:
For NOTIFICATION_EMAIL and USER_EMAIL you can use the gmail you created on step 5. Please send an email to innomedupgrc@gmail.com to request an API_KEY and a FIREBASE_PROJECT_ID and mention the email/password you would like to use to access firebase (For version V1.4 only!) . This does not have to be the same as the NOTIFICATION_EMAIL/NOTIFICATION_PASSWORD.

For loadCellCalibration and zeroFactor follow the instructions on ***Step 7***

(For version V1.4 only!) #define NOTIFICATION_EMAIL "YourEmail@gmail.com"

(For version V1.4 only!) #define NOTIFICATION_PASSWORD "yourPassword"

(For version V1.4 only!) #define USER_EMAIL "YourEmail@gmail.com"

(For version V1.4 only!) #define USER_PASSWORD "yourPassword"

(For version V1.4 only!) #define SENDER_NAME "InnomedUP"

(For version V1.4 only!) #define API_KEY  "askForAPI"

(For version V1.4 only!) #define FIREBASE_PROJECT_ID "askForProjectID"

float loadCellCalibration=21.4;

int zeroFactor=491461;



***Step 9.*** Upload to your ESP32

***Step 10.*** Everytime the ESP32 is powered on it should display on the OLED its MAC address. Please note it

***Step 11.*** If you haven't set up the ESP32 it should automatically create a WiFi network named INNOMED-UP. The password is upcycle!
Use a smartphone or a computer to connect to the network, open a browser and type the address 192.168.4.1. The following page should be displayed.

<img src="https://user-images.githubusercontent.com/37118897/153802616-93ade98d-1aa2-4c42-b077-749abd6f4c40.jpg" width="200">

SSID and password are the specs of the WiFi you want the ESP32 to connect to.
Check interval: The time (in minutes) between checking if the bin is Full. Longer time intervals consume less energy.

Status interval: The device will upload some data and declare its presence in the time (in minutes) specified here. Longer time intervals consume less energy. 

Country GMT: Your local Greenwich Mean Time

Sleep on Low Battery: Use only for PCB and MINI PCB versions.
Set to 1 if you want your device to enter low power mode if the battery is at a critical level. This helps extend the life of your battery.

debug: use only to troubleshoot a device. Requires connection with a computer. 

After settiing it up the device will restart. If you want to repeat the setup hold the button connected on pin 26 for 5 seconds.
This will delete all saved settings including the network SSID and pass. 

***Step 12.*** If everything is setup correctly your ESP32 should upload its data on Firebase (v1.4) or on the inomed up website (v1.5).
It is possible to send a notification email to the manager of the bin.
(For version V1.4 only!) To do so please send an email to innomedupgrc@gmail.com with the MAC of the ESP32 and the recipient email.

ie 84:CC:A8:5C:C7:04 recipientemail@gmail.com.

