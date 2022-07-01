const char INDEX_HTML[] =
"<!DOCTYPE HTML>"
"<html>"
"<head>"
"<meta content=\"text/html; charset=ISO-8859-1\""
" http-equiv=\"content-type\">"
"<meta name = \"viewport\" content = \"width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0\">"
"<title>ESP32 Web Form Demo</title>"
"<style>"
"\"body { background-color: #808080; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; text-align:center;}\""
"</style>"
"</head>"
"<body>"
"<h3>Enter your WiFi credentials</h3>"
"<form action=\"/\" method=\"post\">"
"<p>"
"<label>SSID:&nbsp;</label>"
"<input maxlength=\"30\" name=\"ssid\"><br>"
"<label>Key:&nbsp;&nbsp;&nbsp;&nbsp;</label><input maxlength=\"30\" name=\"password\"><br>"
"<label>Check Interval (mins):</label><input maxlength=\"30\" name=\"checkInterval\" value=\"60\"><br>"
"<label>Status Interval (mins):</label><input maxlength=\"30\" name=\"statusInterval\" value=\"600\"><br>"
"<label>Country GMT:</label><input maxlength=\"30\" name=\"gmtOffset_hr\" value=\"2\"><br>"
"<label>Sleep on Low Bat(0 or 1):</label><input maxlength=\"30\" name=\"sleepOnLowBattery\" value=\"0\"><br>"
"<label>PowerSaving(0 or 1):</label><input maxlength=\"30\" name=\"powerSaving\" value=\"1\"><br>"
"<label>debug(0 or 1):</label><input maxlength=\"30\" name=\"debug\" value=\"0\"><br>"
"<label>max Weight (g):</label><input maxlength=\"30\" name=\"weightThreshold\" value=\"1500\"><br>"
"<input type=\"submit\" value=\"Save\">"
"</p>"
"</form>"
"</body>"
"</html>";
