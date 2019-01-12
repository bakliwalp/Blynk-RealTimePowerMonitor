#define NAMEandVERSION "BlynkRTM-V0.1"
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
/* v0 = Bridge */
/* V1 = Hum */ 
/* V2 = Temp */ 
/* V3 = Comfort */ 
/* V4 = WifiSignall */ 
/* V5 = Motion */
/* V6 = DoorLock */ 
/* V7 = Watts */ 
/* V8 = Current */ 
/* V9 = Arm/Disarm */  
/* V10 = Alarm/Peace */  


/* Comment this out to disable prints and save space */
//#define BLYNK_PRINT Serial
#define doorpin D0
#define PIR D5
#define buzzpin D8

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>
Adafruit_ADS1115 ads;  /* Use this for the 16-bit ADC version - https://github.com/adafruit/Adafruit_ADS1X15 */

#include "SSD1306.h" // alias for #include "SSD1306Wire.h" https://github.com/ThingPulse/esp8266-oled-ssd1306
 SSD1306  display(0x3c, D2, D1); // SDA, SCL , GPIO 13 and GPIO 12

// Bridge widget on virtual pin 1
WidgetBridge thermostat(V0);

#include "DHTesp.h"   // https://github.com/beegee-tokyo/DHTesp
#ifdef ESP32
#pragma message(THIS EXAMPLE IS FOR ESP8266 ONLY!)
#error Select ESP8266 board.
#endif
DHTesp dht;
float t;
float h;

ComfortState cf;
#ifdef __cplusplus
extern "C" {
#endif
uint8_t temprature_sens_read();
#ifdef __cplusplus
}
#endif
uint8_t temprature_sens_read();
float temp_celsius;


double offsetI;
double filteredI;
double sqI,sumI;
int16_t sampleI;
double Irms;

int connectionattempts;
bool connection;

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "auth";

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "ssid";
char pass[] = "pass";

BlynkTimer timer;


#define DHTPIN 10    

float current;
float watts;
bool doorlock;
bool armed;
bool movement;
bool result;

bool buzzing;


// Mac address should be different for each device in your LAN
//byte arduino_mac[] = { 0x68, 0xC6, 0x3A, 0x95, 0x78, 0x8D };
IPAddress arduino_ip ( 192,  168,   1,  12);
IPAddress dns_ip     ( 192,  168,   1,   3);
IPAddress gateway_ip ( 192,  168,   1,   1);
IPAddress subnet_mask(255, 255, 255,   0);


void setup()
{
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.hostname(NAMEandVERSION);
  WiFi.config(arduino_ip, dns_ip, gateway_ip, subnet_mask);
  // Debug console
  delay(10);
  dht.setup(DHTPIN, DHTesp::DHT22); // Connect DHT sensor to GPIO 10
  pinMode(DHTPIN, INPUT);
  pinMode(doorpin, INPUT);
  pinMode(PIR, INPUT);
  pinMode(buzzpin, OUTPUT);
  ads.setGain(GAIN_FOUR);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  ads.begin();
  displayInitSeq();
  // Wire.begin(5, 4); // Wire.begin([SDA-Pin],[SCL-Pin]);
  // Setup a function to be called every second
  timer.setInterval(1000L, doorstatus);
  timer.setInterval(2000L, power);
  timer.setInterval(2000L, motion);
  timer.setInterval(5000L, displayData);
  timer.setInterval(2500L, buzzer);
  timer.setInterval(1500L, protection);
  timer.setInterval(10000L, connectionstatus);
  timer.setInterval(10000L, chkWifiSignal);
  timer.setInterval(5000L, checkComfort);

}

void displayInitSeq()
{
  delay(dht.getMinimumSamplingPeriod());
  h = dht.getHumidity();
  t = dht.getTemperature();
  if (isnan(h) || isnan(t)) {
   Serial.println("Failed to read from DHT sensor!");
    return;
  }

  yield();
  display.init();
  display.clear();
  display.flipScreenVertically();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_10);
  display.drawString(64, 0, String(NAMEandVERSION)); 
  display.display();
//  WiFi.begin(ssid, pass);
//  yield();
//  delay(3000);  
//  if (WiFi.status() != WL_CONNECTED) {
//    Serial.print(".");
//    display.drawString(64, 10, "Connection to Wifi...");
//    display.drawString(64, 20, "FAILED!");
//    delay(2000);
//    display.display();
//    ESP.restart(); 
//  }
  yield();
  Serial.println("");
  Serial.println("WiFi connected");
  display.drawString(64, 20, "WiFi connected!");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP()); 
  display.drawString(64, 30, "Local IP: " + String(WiFi.localIP().toString()) );
  display.display();  
  while (Blynk.connected() == false) {
    display.drawString(64, 40, "Connecting to Server...");
    delay(1000);
    display.display();
    Blynk.begin(auth, ssid, pass, IPAddress(192,168,1,3), 8441);
  }
  display.drawString(64, 50, "Connected to Server!");
  Serial.println("Connected to Blynk server");
  display.display();
  yield();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  
}

void loop()
{
  Blynk.run();
  timer.run(); // Initiates BlynkTimer
  result = Blynk.connected();
}

void checkComfort(){
  yield();
  getTemperature();
  if (getTemperature) {
    Blynk.virtualWrite(V1, h);
    Blynk.virtualWrite(V2, t);
    thermostat.virtualWrite(V44, t); // Sends the value to the thermostat.
    displayData();  
  }
}

bool getTemperature() {
  yield();
  // Reading temperature for humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
  TempAndHumidity newValues = dht.getTempAndHumidity();
  // Check if any reads failed and exit early (to try again).
  if (dht.getStatus() != 0) {
    Serial.println("DHT22 error status: " + String(dht.getStatusString()));
    return false;
  }

  float heatIndex = dht.computeHeatIndex(newValues.temperature, newValues.humidity);
  float dewPoint = dht.computeDewPoint(newValues.temperature, newValues.humidity);
  float cr = dht.getComfortRatio(cf, newValues.temperature, newValues.humidity);
  //  h = dht.getHumidity();
  //  t = dht.getTemperature(); // or dht.readTemperature(true) for Fahrenheit
  h = newValues.humidity;
  t = newValues.temperature;
  Serial.println("T= " + String(t));
  Serial.println("H= " + String(h));
  yield();
  String comfortStatus;
  switch(cf) {
    case Comfort_OK:
      comfortStatus = "Comfort_OK";
      Blynk.setProperty(V3, "offLabel", "Comfort OK");
      break;
    case Comfort_TooHot:
      comfortStatus = "Comfort_TooHot";
      Blynk.setProperty(V3, "offLabel", "Comfort TooHot");
      break;
    case Comfort_TooCold:
      comfortStatus = "Comfort_TooCold";
      Blynk.setProperty(V3, "offLabel", "Comfort TooCold");
      break;
    case Comfort_TooDry:
      comfortStatus = "Comfort_TooDry";
      Blynk.setProperty(V3, "offLabel", "Comfort TooDry");
      break;
    case Comfort_TooHumid:
      comfortStatus = "Comfort_TooHumid";
      Blynk.setProperty(V3, "offLabel", "Comfort TooHumid");
      break;
    case Comfort_HotAndHumid:
      comfortStatus = "Comfort_HotAndHumid";
      Blynk.setProperty(V3, "offLabel", "Comfort HotAndHumid");
      break;
    case Comfort_HotAndDry:
      comfortStatus = "Comfort_HotAndDry";
      Blynk.setProperty(V3, "offLabel", "Comfort HotAndDry");
      break;
    case Comfort_ColdAndHumid:
      comfortStatus = "Comfort_ColdAndHumid";
      Blynk.setProperty(V3, "offLabel", "Comfort ColdAndHumid");
      break;
    case Comfort_ColdAndDry:
      comfortStatus = "Comfort_ColdAndDry";
      Blynk.setProperty(V3, "offLabel", "Comfort ColdAndDry");
      break;
    default:
      comfortStatus = "Unknown:";
      Blynk.setProperty(V3, "offLabel", "Unknown");
      break;
  };

  Serial.println(" T:" + String(newValues.temperature) + " H:" + String(newValues.humidity) + " I:" + String(heatIndex) + " D:" + String(dewPoint) + " " + comfortStatus);
  return true;
  yield();
}


void doorstatus()
{
  if (digitalRead(doorpin) == LOW )
  {
    doorlock = 1;
    Serial.println("locked");
    yield();
  }
  else 
  {
    doorlock = 0;
    Serial.println("UNlocked");
    yield();
  }
  Blynk.virtualWrite(V6, doorlock);
  yield();
}

void motion()
{
  if (digitalRead(PIR) == HIGH )
  {
    movement = 1;
    Serial.println("motion");
  }
  else 
  {
    movement = 0;
    Serial.println("clear");
  }
  Blynk.virtualWrite(V5, movement);
  yield();
}

void power()
{
  yield();
  double current = calcIrms(200)*0.045;
  Serial.print(current);
  Serial.println("A");
  watts = current*243;
  Serial.print(watts);
  Serial.println("W");
  Blynk.virtualWrite(V8, current);
  Blynk.virtualWrite(V7, watts);  
}


void protection(){
  if (armed == 1) {
      //Blynk.virtualWrite(V9, armed);  
      if (doorlock == 0){
        buzzing = 1;
        buzzer();
        Blynk.virtualWrite(V10, buzzing); 
        yield();
      }
    }
    else if (armed == 0){
      //Blynk.virtualWrite(V9, armed);  
      buzzing = 0;
      buzzer();
      Blynk.virtualWrite(V10, buzzing);  
      yield();
    }
    //buzzing = param.asInt(); // assigning incoming value from pin V1 to a variable
  
    // process received value
  
}

  BLYNK_WRITE(V9)
  {
    armed = param.asInt(); // assigning incoming value from pin V7 to a variable
    yield();
  }

void buzzer()
{

  if (buzzing == 1)
  {  
    digitalWrite(buzzpin, HIGH);
    Serial.println("buzzing");
    yield();
  }
  else 
  {
    digitalWrite(buzzpin, LOW);      
    Serial.println("not buzzing");
    yield();
  }
  yield();
}


double squareRoot(double fg)  
{
  double n = fg / 2.0;
  double lstX = 0.0;
  while (n != lstX)
  {
    lstX = n;
    n = (n + fg / n) / 2.0;
  }
  return n;
}

double calcIrms(unsigned int Number_of_Samples)
{
  /* Be sure to update this value based on the IC and the gain settings! */
  float multiplier = 0.03125F;    /* ADS1115 @ +/- 4.096V gain (16-bit results) */
  for (unsigned int n = 0; n < Number_of_Samples; n++)
  {
    sampleI = ads.readADC_Differential_0_1();
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
      display.drawString(64, 0 , "RealtimePwr: " + String(watts) + " W");
    }
    display.setFont(ArialMT_Plain_24);
    display.setTextAlignment(TEXT_ALIGN_CENTER);    
    display.drawString(66, 13, String(t) + "°C");
    display.drawString(66, 40, String(h) + "%");  
    display.display();
    yield();
}

void connectionstatus()
{
  connection = Blynk.connected();
  if (connection == 0)
  {
      connectionattempts ++;
      Serial.println();
      Serial.print("connectionattempts");
      Serial.print(connectionattempts);
      Serial.println();
      display.init();
      display.clear();
      display.flipScreenVertically();
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.setFont(ArialMT_Plain_10);
      display.drawString(64, 0, " CONNECTING ...");   
      display.display();
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

void chkWifiSignal()
{
  yield();
  int WifiSignal = -(WiFi.RSSI()) ;
  Blynk.virtualWrite(V4, WifiSignal);
}

BLYNK_CONNECTED() {
  thermostat.setAuthToken("6e64d0b17cff468eb77708b5a5d5e006"); // Place the AuthToken of the Thermostat here
  yield();
}

void heartbeat(){
  
}
