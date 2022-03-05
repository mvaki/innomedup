/*
 Setup your scale and start the sketch WITHOUT a weight on the scale
 Once readings are displayed place a known weight on the scale
 Press +/- or a/z or / and * to adjust the calibration_factor until the output readings match the known weight
 When you are done save 
 1)the zeroFactor which is displayed before you put any weights on the scale
 2)The calibration factor

 Connections:
 ESP32 GPIO32 -> HX711 CLK
 ESP32 GPIO33 -> HX711 DOUT
 ESP32 3.3V -> HX711 VCC
 ESP32 GND -> HX711 GND 
*/

#include "HX711.h"

HX711 scale;
bool readLine=false;
char inputChars[32];
int charcount=0;
float calibration_factor = 21.4; // this calibration factor is adjusted according to my load cell
float units;
float ounces;
long zero_factor;

void setup() {
  scale.begin(33,32);
  //scale.begin(2,3);
  Serial.begin(115200);
  Serial.println("HX711 calibration sketch");
  Serial.println("Remove all weight from scale");
  Serial.println("After readings begin, place known weight on scale");
  Serial.println("Press + or a to increase calibration factor by 1.0");
  Serial.println("Press - or z to decrease calibration factor by 1.0");
  Serial.println("Press * to increase calibration factor by 0.1");
  Serial.println("Press / to decrease calibration factor by 0.1");

  scale.set_scale();
  scale.tare();  //Reset the scale to 0

  zero_factor = scale.read_average(); //Get a baseline reading
  Serial.print("Zero factor: "); //This can be used to remove the need to tare the scale. Useful in permanent scale projects.
  Serial.println(zero_factor);
}

void loop() {

  scale.set_scale(calibration_factor); //Adjust to this calibration factor

  Serial.print("Reading: ");
  units = scale.get_units();
//  if (units < 0)
//  {
//    units = 0.00;
//  }
  ounces = units * 0.035274;
  Serial.print(units);
  Serial.print(" grams"); 
  Serial.print(" calibration_factor: ");
  Serial.print(calibration_factor);
  Serial.println();

  while(Serial.available()>0&&!readLine){
    inputChars[charcount]=Serial.read();
    //Serial.print(inputChars[charcount]);
    if(inputChars[charcount]=='\n'){
      readLine=true; 
    }
    charcount++;
  }
  //decode Pc commands and reinitialize
  if(readLine){
    if(isDigit(inputChars[0])||isDigit(inputChars[1])){
      calibration_factor=atoi(inputChars);
    }
    else{
      for(int i=0;i<charcount;++i){
        if(inputChars[i] == '+' || inputChars[i] == 'a')
          calibration_factor += 1;
        else if(inputChars[i] == '-' || inputChars[i] == 'z')
          calibration_factor -= 1;
        else if(inputChars[i] == '/')
          calibration_factor -= 0.1;
        else if(inputChars[i] == '*')
          calibration_factor += 0.1;
        else if(inputChars[i] == '='){
          Serial.print("Zero factor: ");
          Serial.print(zero_factor);
          Serial.print("   calibration_factor: ");
          Serial.println(calibration_factor);
          delay(100000);
        }
      }
    }
    readLine=false;
    charcount=0;   
  }
  
  if(Serial.available())
  {
    char temp = Serial.read();
    if(temp == '+' || temp == 'a')
      calibration_factor += 1;
    else if(temp == '-' || temp == 'z')
      calibration_factor -= 1;  
  }
}
