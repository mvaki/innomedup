/*
 * Function to handle unknown URLs
 */
void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

/*
 * Function for writing WiFi creds to EEPROM
 * Returns: true if save successful, false if unsuccessful
 */
bool writeToMemory(String ssid, String pass,int checkInterval,int statusInterval,int gmt,int debug, int sleepOnLowBattery){
  char buff1[30];
  char buff2[30];
  //int interval;
  ssid.toCharArray(buff1,30);
  pass.toCharArray(buff2,30); 
  EEPROM.writeString(100,buff1);
  EEPROM.writeString(200,buff2);
  EEPROM.put(300,checkInterval);
  EEPROM.put(300+sizeof(int),statusInterval);
  EEPROM.put(300+2*sizeof(int),gmt);
  EEPROM.put(300+3*sizeof(int),debug);
  EEPROM.put(300+4*sizeof(int),sleepOnLowBattery);
  delay(100);
  String s = EEPROM.readString(100);
  String p = EEPROM.readString(200);
  int checkInter,statusInter;
  EEPROM.get(300,checkInter);
  EEPROM.get(300+sizeof(int),statusInter);
  #if debug
  Serial.print("Stored SSID, password, interval are: ");
  Serial.print(s);
  Serial.print(" / ");
  Serial.print(p);
  Serial.print(" / ");
  Serial.print(checkInter);
  Serial.print(" / ");
  Serial.println(statusInter);
  #endif
  if(ssid == s && pass == p){
    return true;  
  }else{
    return false;
  }
}


/*
 * Function for handling form
 */
void handleSubmit(){
  String response_success="<h1>Success</h1>";
  response_success +="<h2>Device will restart in 3 seconds</h2>";

  String response_error="<h1>Error</h1>";
  response_error +="<h2><a href='/'>Go back</a>to try again";
  
  if(writeToMemory(String(server.arg("ssid")),String(server.arg("password")),int(server.arg("checkInterval").toInt()),int(server.arg("statusInterval").toInt()),int(server.arg("gmtOffset_hr").toInt()),int(server.arg("debug").toInt()),int(server.arg("sleepOnLowBattery").toInt()))){
     server.send(200, "text/html", response_success);
     EEPROM.commit();
     delay(3000);
     ESP.restart();
  }else{
     server.send(200, "text/html", response_error);
  }
}

/*
 * Function for home page
 */
void handleRoot() {
  if (server.hasArg("ssid")&& server.hasArg("password")&& server.hasArg("checkInterval")&& server.hasArg("statusInterval")&& server.hasArg("gmtOffset_hr")&& server.hasArg("debug")&& server.hasArg("sleepOnLowBattery")) {
    handleSubmit();
  }
  else {
    server.send(200, "text/html", INDEX_HTML);
  }
}

/*
 * Function for loading form
 * Returns: false if no WiFi creds in EEPROM
 */
bool loadWIFICredsForm(const char* ssid,const char* password  ){
  String s = EEPROM.readString(100);
  String p = EEPROM.readString(200);
  
  //const char* ssid     = "INNOMED-UP";
  //const char* password = "upcycle!";
  #if debug
  Serial.println("Setting Access Point...");
  #endif
  WiFi.softAP(ssid, password);
  
  IPAddress IP = WiFi.softAPIP();
  #if debug
  Serial.print("AP IP address: ");
  Serial.println(IP);
  #endif
  server.on("/", handleRoot);

  server.onNotFound(handleNotFound);

  server.begin();
  
  Serial.println("HTTP server started");
 
  while(s.length() <= 0 && p.length() <= 0){
    server.handleClient();
    delay(100);
  }
  
  return false;
}

/*
 * Function checking WiFi creds in memory 
 * Returns: true if not empty, false if empty
 */
bool CheckWIFICreds(){
  #if debug
  Serial.println("Checking WIFI credentials");
  #endif
  String s = EEPROM.readString(100);
  String p = EEPROM.readString(200);
  #if debug
  Serial.print("Found credentials: ");
  Serial.print(s);
  Serial.print("/");
  Serial.print(p);
  delay(5000);
  #endif
  if(s.length() > 0 && p.length() > 0){
    return true;
  }else{
    return false;
  }
}
