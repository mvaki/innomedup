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

Step 1. Connect the hardware for the NO_PCB version as in the picture below. 
![innomedup_nopcb_variant_v0 2](https://user-images.githubusercontent.com/37118897/153802363-54a3113d-c0b0-47d4-bd40-260771c730d4.jpg)

Step 2. Install Arduino IDE
Install the latest version of Arduino IDE from
https://www.arduino.cc/en/software

Step 3. Install the ESP library by following the instructions on 
https://randomnerdtutorials.com/installing-the-esp32-board-in-arduino-ide-windows-instructions/

Step 4. Install the required libraries
On Arduino go to Sketch->Include Library->Manage Libraries 
Download HX711 Arduino Library by Bogdan Necula, Andreas Motl (0.7.4)
Download Firebase Arduino Client Library for Espressif ESP8266 and ESP32 by Mobizt (2.8.3)
Download ESP Mail Client by Mobizt (1.6.4)
Download ESP8266 and ESP32 OLED driver for SSD1306 displays by ThingPulse,Weinberg (4.2.1)

The programm has been tested with the versions mentioned above. Please install these versions. 

Step 5. Create a gmail account and change security settings so that esp can send emails.
For Greece the email is innomedupgrc@gmail.com
When you create the email go to https://myaccount.google.com/lesssecureapps?pli=1 and allow less secure apps access 

Step 6. Open innomedup_release_v1_1. Before uploading the code to your ESP change the following lines:
Also before uploading this sketch upload a simple sketch like Blink located in File->examples->01.Basics to make sure you have selected the right version of ESP32. 

#define NOTIFICATION_EMAIL "YourEmail@gmail.com"
#define NOTIFICATION_PASSWORD "yourPassword"

#define USER_EMAIL "YourEmail@gmail.com"
#define USER_PASSWORD "yourPassword"
#define SENDER_NAME "InnomedUP"

Step 7. Upload to your ESP32

Step 8. Everytime the ESP32 is powered on it should display on the OLED its MAC address. Please note it

Step 9. If you havent set up the ESP32 it should automatically create a wifi network named INNOMED-UP. The password is upcycle!
Use a smartphone or a computer to connect to the network, open a browser and type the address 192.168.4.1. The following page should be displayed.
![innomed_ap](https://user-images.githubusercontent.com/37118897/153802616-93ade98d-1aa2-4c42-b077-749abd6f4c40.jpg)

After settiing it up the device will restart. If you want to repeat the setup hold the button connected on pin 26 for 5 seconds.
This will delete all saved settings including the network SSID and pass. 

Step 10. If everything is setup correctly your ESP should upload its data on Firebase.
It is possible to send a notification email to the manager of the bin.
To do so please send an email to innomedupgrc@gmail.com with the MAC of the ESP and the recipient email.
ie 84:CC:A8:5C:C7:04 recipientemail@gmail.com.

