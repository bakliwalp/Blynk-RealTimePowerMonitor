/*************************************************************
  Download latest Blynk library here:
    https://github.com/blynkkk/blynk-library/releases/latest

  Blynk is a platform with iOS and Android apps to control
  Arduino, Raspberry Pi and the likes over the Internet.
  You can easily build graphic interfaces for all your
  projects by simply dragging and dropping widgets.

    Downloads, docs, tutorials: http://www.blynk.cc
    Sketch generator:           http://examples.blynk.cc
    Blynk community:            http://community.blynk.cc
    Follow us:                  http://www.fb.com/blynkapp
                                http://twitter.com/blynk_app

  Blynk library is licensed under MIT license
  This example code is in public domain.

 *************************************************************

  This example shows how value can be pushed from Arduino to
  the Blynk App.

  NOTE:
  BlynkTimer provides SimpleTimer functionality:
    http://playground.arduino.cc/Code/SimpleTimer

  App project setup:
    Value Display widget attached to Virtual Pin V5
 *************************************************************/


/* ******* Virtual pins ********* */
/* V1 = Temp */ 
/* V2 = Hum */ 
/* V3 = DoorLock */ 
/* V4 = Watts */ 
/* V5 = Current */
/* V6 = Motion */  

/* V7 = Arm/Disarm */  
/* V8 = Alarm/Peace */  

/* Comment this out to disable prints and save space */
//#define BLYNK_PRINT Serial
#define doorpin D0
#define PIR D5
#define buzzpin D8

#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>
Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */

#include "SSD1306.h" // alias for #include "SSD1306Wire.h" https://github.com/ThingPulse/esp8266-oled-ssd1306
 SSD1306  display(0x3c, D2, D1); // SDA, SCL , GPIO 13 and GPIO 12
String display_temp;
String display_humid;
String dispwatts;

double offsetI;
double filteredI;
double sqI,sumI;
int16_t sampleI;
double Irms;

int connectionattempts;
bool connection;

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "xxxxxxxxxxxxxxxxxxxxxxxxxxx";

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "SSID";
char pass[] = "PASSWORD";

BlynkTimer timer;


#define DHTPIN 10    
#define DHTTYPE DHT22   // DHT 22, AM2302, AM2321   
DHT dht(10, DHTTYPE);

float current;
float watts;
bool doorlock;
bool armed;
bool movement;
bool result;

bool buzzing;


// Mac address should be different for each device in your LAN
byte arduino_mac[] = { 0x68, 0xC6, 0x3A, 0x95, 0x78, 0x8D };
IPAddress arduino_ip ( 192,  168,   1,  12);
IPAddress dns_ip     (  8,   8,   8,   8);
IPAddress gateway_ip ( 192,  168,   1,   1);
IPAddress subnet_mask(255, 255, 255,   0);


void setup()
{
  Serial.begin(9600);
  WiFi.hostname("Blynk-RealTimePowerMonitor");
  WiFi.mode(WIFI_STA);
  WiFi.hostname("Blynk-RealTimePowerMonitor");
  WiFi.config(arduino_ip, dns_ip, gateway_ip, subnet_mask);
  // Debug console



  yield();
  delay(10);
  dht.begin();
  pinMode(DHTPIN, INPUT);
  pinMode(doorpin, INPUT);
  pinMode(PIR, INPUT);
  pinMode(buzzpin, OUTPUT);

  display.init();
  display.clear();
  display.flipScreenVertically();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.display();

//  Blynk.begin(auth, ssid, pass);
  // You can also specify server:
  //Blynk.begin(auth, ssid, pass, "blynk-cloud.com", 8442);
 Blynk.begin(auth, ssid, pass, IPAddress(192,168,1,3), 8080);
      yield();
  ads.setGain(GAIN_FOUR);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  ads.begin();
    yield();
// Wire.begin(5, 4); // Wire.begin([SDA-Pin],[SCL-Pin]);
  // Setup a function to be called every second
  timer.setInterval(10000L, sendTempHum);
  yield();
  timer.setInterval(1000L, doorstatus);
  timer.setInterval(2000L, power);
  timer.setInterval(2000L, motion);
  timer.setInterval(5000L,displayData);
  timer.setInterval(2500L,buzzer);
  timer.setInterval(10000L, connectionstatus);
  timer.setInterval(1500L, protection);
  yield();  



  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
      yield();
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
    yield();
  });
  ArduinoOTA.setHostname("Blynk-RealTimePowerMonitor"); // OPTIONAL NAME FOR OTA
  yield();
  ArduinoOTA.begin();
  yield();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  

}

void loop()
{
  yield();
  ArduinoOTA.handle();
  yield();
  Blynk.run();
  yield();
  timer.run(); // Initiates BlynkTimer
  yield();
  result = Blynk.connected();
  yield();
}


void sendTempHum()
{
  yield();
  Blynk.syncVirtual(V3,V6,V7,V8);
  yield();
  float h = dht.readHumidity();
  yield();
  float t = dht.readTemperature(); // or dht.readTemperature(true) for Fahrenheit
  delay(10);
  yield();
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  
  display_temp = t;
  display_humid = h;
  
  Serial.println();
  Serial.print("t=");
  Serial.println(t);

  Serial.println();
  Serial.print("h=");
  Serial.println(h);


  
  // You can send any value at any time.
  // Please don't send more that 10 values per second.
  Blynk.virtualWrite(V1, h);
  Blynk.virtualWrite(V2, t);

  yield();
}



void doorstatus()
{
  if (digitalRead(doorpin) == LOW )
  {
    doorlock = 1;
    Serial.println("locked");
  }
  else 
  {
    doorlock = 0;
    Serial.println("UNlocked");
  
  }
  Blynk.virtualWrite(V3, doorlock);
    yield();
}

void motion()
{
  if (digitalRead(PIR) == HIGH )
  {
    movement = 1;
      yield();
    Serial.println("motion");
  }
  else 
  {
    movement = 0;
      yield();
    Serial.println("clear");
  }
    yield();
  Blynk.virtualWrite(V6, movement);
  
}

void power()
{
  double current = calcIrms(200)*0.045;
  yield();
  Serial.print(current);
  Serial.println("A");

  int watts = current*243;
  dispwatts = watts;
  
  Serial.print(watts);
  Serial.println("W");
  Blynk.virtualWrite(V4, current);
  yield();

  Blynk.virtualWrite(V5, watts);  
  yield();
}


void protection(){
  if (armed == 1) {
      //Blynk.virtualWrite(V7, armed);  
      if (doorlock == 0){
        buzzing = 1;
        buzzer();
        Blynk.virtualWrite(V8, buzzing); 
      }
    }
    else if (armed == 0){
      //Blynk.virtualWrite(V7, armed);  
      buzzing = 0;
      buzzer();
      Blynk.virtualWrite(V8, buzzing);  
    }
    //buzzing = param.asInt(); // assigning incoming value from pin V1 to a variable
  
    // process received value
      yield();
  
}

  BLYNK_WRITE(V7)
  {
    armed = param.asInt(); // assigning incoming value from pin V7 to a variable
  }

void buzzer()
{

  if (buzzing == 1)
  {  
    digitalWrite(buzzpin, HIGH);
    Serial.println("buzzing");
  }
  else 
  {
    digitalWrite(buzzpin, LOW);      
    Serial.println("not buzzing");
  }
  yield();
}


double squareRoot(double fg)  
{
  yield();  
  double n = fg / 2.0;
  double lstX = 0.0;
  yield();  
  while (n != lstX)
  {
    lstX = n;
    n = (n + fg / n) / 2.0;
  }
  
  yield();
  return n;
  yield();
}

double calcIrms(unsigned int Number_of_Samples)
{
  /* Be sure to update this value based on the IC and the gain settings! */
  float multiplier = 0.03125F;    /* ADS1115 @ +/- 4.096V gain (16-bit results) */
  for (unsigned int n = 0; n < Number_of_Samples; n++)
  {
    sampleI = ads.readADC_Differential_0_1();
  yield();
    // Digital low pass filter extracts the 2.5 V or 1.65 V dc offset, 
  //  then subtract this - signal is now centered on 0 counts.
    offsetI = (offsetI + (sampleI-offsetI)/1024);
    filteredI = sampleI - offsetI;
    //filteredI = sampleI * multiplier;

    // Root-mean-square method current
    // 1) square current values
    sqI = filteredI * filteredI;
    // 2) sum 
    sumI += sqI;
    yield();
  }
  
  Irms = squareRoot(sumI / Number_of_Samples)*multiplier; 

  //Reset accumulators
  sumI = 0;
//--------------------------------------------------------------------------------------       
  yield(); 
  return Irms;
}


void displayData() { 
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);  
    display.setFont(ArialMT_Plain_10);
    if (result == 0){
      display.drawString(64, 0, "*** Disconected!! ***");   
    }
    else {
      display.drawString(64, 0 , "RealtimePwr: " + dispwatts + " W");
      yield();
    }
    display.setFont(ArialMT_Plain_24);
    display.setTextAlignment(TEXT_ALIGN_CENTER);    
    display.drawString(66, 13, display_temp + "°C");
    display.drawString(66, 40, display_humid + "%");
    yield();    
    display.display();
    yield();
}

void connectionstatus()
{
//  result = Blynk.connected();
//  result = Blynk.connect(60);
  connection = Blynk.connected();
  if (connection == 0)
  {
      connectionattempts ++;
      Serial.println();
      Serial.print("connectionattempts");
      Serial.print(connectionattempts);
      Serial.println();
      yield();
      display.init();
      display.clear();
      display.flipScreenVertically();
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.setFont(ArialMT_Plain_10);
      display.drawString(64, 0, " CONNECTING ...");   
      display.display();
      yield();
  }
  else 
  {
    connectionattempts = 0;
  }
  
  if (connectionattempts == 5)
  {
      ESP.restart();  
  }
}
