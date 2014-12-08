#include <LGPS.h> // LinkIt GPS Library
#include <LBattery.h> // Want to be able to read the battery charge level
#include <LGPRS.h> //include the base GPRS library
#include <LGPRSClient.h> //include the ability to Post and Get information using HTTP
#include <Wire.h>
#include <math.h>
#define COLOR_SENSOR_ADDR  0x39//the I2C address for the color sensor 
#define REG_CTL 0x80
#define REG_TIMING 0x81
#define REG_INT 0x82
#define REG_INT_SOURCE 0x83
#define REG_ID 0x84
#define REG_GAIN 0x87
#define REG_LOW_THRESH_LOW_BYTE 0x88
#define REG_LOW_THRESH_HIGH_BYTE 0x89
#define REG_HIGH_THRESH_LOW_BYTE 0x8A
#define REG_HIGH_THRESH_HIGH_BYTE 0x8B
#define REG_BLOCK_READ 0xCF
#define REG_GREEN_LOW 0xD0
#define REG_GREEN_HIGH 0xD1
#define REG_RED_LOW 0xD2
#define REG_RED_HIGH 0xD3
#define REG_BLUE_LOW 0xD4
#define REG_BLUE_HIGH 0xD5
#define REG_CLEAR_LOW 0xD6
#define REG_CLEAR_HIGH 0xD7
#define CTL_DAT_INIITIATE 0x03
#define CLR_INT 0xE0
//Timing Register
#define SYNC_EDGE 0x40
#define INTEG_MODE_FREE 0x00
#define INTEG_MODE_MANUAL 0x10
#define INTEG_MODE_SYN_SINGLE 0x20
#define INTEG_MODE_SYN_MULTI 0x30
 
#define INTEG_PARAM_PULSE_COUNT1 0x00
#define INTEG_PARAM_PULSE_COUNT2 0x01
#define INTEG_PARAM_PULSE_COUNT4 0x02
#define INTEG_PARAM_PULSE_COUNT8 0x03
//Interrupt Control Register 
#define INTR_STOP 40
#define INTR_DISABLE 0x00
#define INTR_LEVEL 0x10
#define INTR_PERSIST_EVERY 0x00
#define INTR_PERSIST_SINGLE 0x01
//Interrupt Souce Register
#define INT_SOURCE_GREEN 0x00
#define INT_SOURCE_RED 0x01
#define INT_SOURCE_BLUE 0x10
#define INT_SOURCE_CLEAR 0x03
//Gain Register
#define GAIN_1 0x00
#define GAIN_4 0x10
#define GAIN_16 0x20
#define GANI_64 0x30
#define PRESCALER_1 0x00
#define PRESCALER_2 0x01
#define PRESCALER_4 0x02
#define PRESCALER_8 0x03
#define PRESCALER_16 0x04
#define PRESCALER_32 0x05
#define PRESCALER_64 0x06
 
int readingdata[20];
int i,green,red,blue,clr,ctl;
double X,Y,Z,x,y,z;
//gprs settings
//int temp_address = 72; //1001000 written in decimal for the TC74 Sensor
// These are the variables you will want to change based on your IOT data streaming account / provider
char action[] = "POST "; // Edit to build your command - "GET ", "POST ", "HEAD ", "OPTIONS " - note trailing space
char server[] = "things.ubidots.com";
char path[] = "/api/v1.6/variables/"; // Common path
char battkey[] = "5481c7ef7625422406fbbda5"; // Battery API Key
char tempkey[] = "547f611076254207524fc6a6"; // Temp API Key
char token[] = "XXXXXXXXX"; // Edit to insert you API Token
int port = 80; // HTTP
// Here are the program variables
unsigned long ReportingInterval = 100000; // How often do you want to update the IOT site in milliseconds
unsigned long LastReport = 0; // When was the last time you reported
const int ledPin = 13; // Light to blink when program terminates
String Location = ""; // Will build the Location string here
// Create instantiations of the GPRS and GPS functions
LGPRSClient globalClient; // See this support topic from Mediatek - http://labs.mediatek.com/forums/posts/list/75.page
gpsSentenceInfoStruct info; // instantiate

String redstr;
String greenstr;
String bluestr;
String lightstr;

void setup()
{  
	Serial.begin(19200);
	Wire.begin(); // join i2c bus (address optional for master)
        
        LGPS.powerOn(); // Start the GPS first as it takes time to get a fix
        Serial.println("GPS Powered on, and waiting ...");
        Serial.println("Attach to GPRS network"); // Attach to GPRS network - need to add timeout
        while (!LGPRS.attachGPRS("eseye.com","user","pass")) { //attachGPRS(const char *apn, const char *username, const char *password);
          delay(500);
          }
        LGPRSClient client; //Client has to be initiated after GPRS is established with the correct APN settings - see above link
        globalClient = client; // Again this is a temporary solution described in support forums

        Serial.println("ok, setup done");
}


void loop()
{
 
 //Do the RGB Reading stuff
  setTimingReg(INTEG_MODE_FREE);//Set trigger mode.Including free mode,manually mode,single synchronizition mode or so.
  setInterruptSourceReg(INT_SOURCE_GREEN); //Set interrupt source 
  setInterruptControlReg(INTR_LEVEL|INTR_PERSIST_EVERY);//Set interrupt mode
  setGain(GAIN_1|PRESCALER_4);//Set gain value and prescaler value
  setEnableADC();//Start ADC of the color sensor
     
 //now prepare for GPRS stuff and Ubi
 if (globalClient.available()) {// if there are incoming bytes available from the server
 char c = globalClient.read(); // read them and print them:
 Serial.print(c);
 }
 if (millis() >= LastReport + ReportingInterval) { // Section to report - will convert to a function on next rev
 
 
 readRGB();
 String sendstring="{\"value\":"+ redstr +  "}";
 SendToUbidots(sendstring, "xxx"); //variable goes here
 String sendstring2="{\"value\":"+ greenstr + "}"; 
 SendToUbidots(sendstring2,  "xxx"); //variable goes here
 String sendstring3="{\"value\":"+ bluestr + "}";
 SendToUbidots(sendstring3,  "xxx"); //variable goes here
 String sendstring4="{\"value\":"+ lightstr + "}";
 SendToUbidots(sendstring4, "xxx"); //variable goes here
 
 SendToUbidots(GPSbat(), "xxx"); //variable goes here
 clearInterrupt();
 LastReport = millis();
 }
//delay(5000);

 
}
/************************************/
void setTimingReg(int x)
{
   Wire.beginTransmission(COLOR_SENSOR_ADDR);
   Wire.write(REG_TIMING);
   Wire.write(x);
   Wire.endTransmission();  
   delay(100); 
}
void setInterruptSourceReg(int x)
{
   Wire.beginTransmission(COLOR_SENSOR_ADDR);
   Wire.write(REG_INT_SOURCE);
   Wire.write(x);
   Wire.endTransmission();  
   delay(100);
}
void setInterruptControlReg(int x)
{
   Wire.beginTransmission(COLOR_SENSOR_ADDR);
   Wire.write(REG_INT);
   Wire.write(x);
   Wire.endTransmission();  
   delay(100);
}
void setGain(int x)
{
   Wire.beginTransmission(COLOR_SENSOR_ADDR);
   Wire.write(REG_GAIN);
   Wire.write(x);
   Wire.endTransmission();
}
void setEnableADC()
{
 
   Wire.beginTransmission(COLOR_SENSOR_ADDR);
   Wire.write(REG_CTL);
   Wire.write(CTL_DAT_INIITIATE);
   Wire.endTransmission();  
   delay(100);  
}
void clearInterrupt()
{
   Wire.beginTransmission(COLOR_SENSOR_ADDR);
   Wire.write(CLR_INT);
   Wire.endTransmission(); 
}
void readRGB()
{
  Wire.beginTransmission(COLOR_SENSOR_ADDR);
   Wire.write(REG_BLOCK_READ);
   Wire.endTransmission();
 
   Wire.beginTransmission(COLOR_SENSOR_ADDR);
   Wire.requestFrom(COLOR_SENSOR_ADDR,8);
   delay(500);
   if(8<= Wire.available())    // if two bytes were received 
  { 
    for(i=0;i<8;i++)
    {
      readingdata[i]=Wire.read();
      //Serial.println(readingdata[i],BIN);
     }
  }
  green=readingdata[1]*256+readingdata[0];
  red=readingdata[3]*256+readingdata[2];
  blue=readingdata[5]*256+readingdata[4];
  clr=readingdata[7]*256+readingdata[6];
  //Serial.println("The RGB value and Clear channel value are");
  redstr=String(red);
  greenstr=String(green);
  bluestr=String(blue);
  int light = (green+red+blue)/3;
  lightstr=String(light);
  Serial.println(red,DEC);
  Serial.println(green,DEC);
  Serial.println(blue,DEC);
  Serial.println(clr,DEC);  
}
void calculateCoordinate()
{
  X=(-0.14282)*red+(1.54924)*green+(-0.95641)*blue;
  Y=(-0.32466)*red+(1.57837)*green+(-0.73191)*blue;
  Z=(-0.68202)*red+(0.77073)*green+(0.56332)*blue;
  x=X/(X+Y+Z);
  y=Y/(X+Y+Z);
  if((X>0)&&(Y>0)&&(Z>0))
  {
    Serial.println("The x,y value is");
	Serial.print("(");
    Serial.print(x,2);
	Serial.print(" , ");
    Serial.print(y,2);
	Serial.println(")");
	//Serial.println("Please reference the figure(Chromaticity Diagram) in the wiki ");
	//Serial.println("so as to get the recommended color.");
  }
 else
 Serial.println("Error,the value overflow");
}

//GPRS functions below


void SendToUbidots(String payload, String apikey)
{
int num; // part of the length calculation
String le; // length of the payload in characters
num=payload.length(); // How long is the payload
le=String(num); //this is to calcule the length of var
Serial.print("length of the string is: " + le);
Serial.print("what we are actually sending is: " + payload);
Serial.print("Connect to "); // For the console - show you are connecting
Serial.println(server);
if (globalClient.connect(server, port)){ // if you get a connection, report back via serial:
Serial.println("connected"); // Console monitoring
globalClient.print(action); // These commands build a JSON request for Ubidots but fairly standard
globalClient.print(path); // specs for this command here: http://ubidots.com/docs/api/index.html
globalClient.print(apikey); // Prints the battery key
globalClient.println("/values HTTP/1.1");
globalClient.println(F("Content-Type: application/json"));
globalClient.print(F("Content-Length: "));
globalClient.println(le);
globalClient.print(F("X-Auth-Token: "));
globalClient.println(token);
globalClient.print(F("Host: "));
globalClient.println(server);
globalClient.println();
globalClient.println(payload); // The payload defined above
globalClient.println();
globalClient.println((char)26); //This terminates the JSON SEND with a carriage return
}
}
String TempPayload()
{
String payload;
String value = String(22); // Read the Temperature
payload="{\"value\":"+ value + "}"; // Build the JSON packet without GPS info
return payload;
}
String GPSbat()
{
String payload;
String value = String(LBattery.level()); // Read the battery level
LGPS.getData(&info); // Get a GPS fix
if (ParseLocation((const char*)info.GPGGA)) { // This is where we break out needed location information
Serial.print("Location is: ");
Serial.println(Location); // This is the format needed by Ubidots
}
if (Location.length()==0) {
payload="{\"value\":"+ value + "}"; //Build the JSON packet without GPS info
}
else {
payload="{\"value\":"+ value + ", \"context\":"+ Location + "}"; // with GPS info
}
return payload;
}
boolean ParseLocation(const char* GPGGAstr)
// Refer to http://www.gpsinformation.org/dale/nmea.htm#GGA
// Sample data: $GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
{
char latarray[6];
char longarray[6];
int index = 0;
Serial.println(GPGGAstr);
Serial.print("Fix Quality: ");
Serial.println(GPGGAstr[43]);
if (GPGGAstr[43]=='0') { // This is the place in the sentence that shows Fix Quality 0 means no fix
Serial.println("No GPS Fix");
Location = ""; // No fix then no Location string
return 0;
}
String GPSstring = String(GPGGAstr);
for (int i=20; i<=26; i++) { // We have to jump through some hoops here
latarray[index] = GPGGAstr[i]; // we need to pick out the minutes from the char array
index++;
}
float latdms = atof(latarray); // and convert them to a float
float lattitude = latdms/60; // and convert that to decimal degrees
String lattstring = String(lattitude);// Then put back into a string
Location = "{\"lat\":";
if(GPGGAstr[28] == 'S') Location = Location + "-";
Location += GPSstring.substring(18,20) + "." + lattstring.substring(2,4);
index = 0;
for (int i=33; i<=38; i++) { // And do the same thing for longitude
longarray[index] = GPGGAstr[i]; // the good news is that the GPS data is fixed column
index++;
}
float longdms = atof(longarray); // and convert them to a float
float longitude = longdms/60; // and convert that to decimal degrees
String longstring = String(longitude);// Then put back into a string
Location += ", \"lng\":";
String newloc;
Serial.println("substring: " + GPSstring.substring(31,32));
 if(GPSstring.substring(31,32)=="0") //fix because ubidots don't accept a leading '0'
  {
  newloc = GPSstring.substring(32,33);
  }
  else
  {
  newloc = GPSstring.substring(31,33);
  }

if(GPGGAstr[41] == 'W') Location = Location + "-";
if(GPGGAstr[30] == '0') {
Location = Location + newloc + "." + longstring.substring(2,4) + "}";
}
else {
Location = Location + GPSstring.substring(30,33) + "." + longstring.substring(2,4) + "}";
}
return 1;
}


