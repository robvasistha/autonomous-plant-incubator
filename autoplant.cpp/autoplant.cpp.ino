

/*
  Sketch generated by the Arduino IoT Cloud Thing "Untitled"
  https://create.arduino.cc/cloud/things/d75d354e-a8e2-404e-a2aa-c1e0c66b1d17

  Arduino IoT Cloud Variables description

  The following variables are automatically generated and updated when changes are made to the Thing

  int waterLimit;
  int battery_value;
  int moistValue;
  float temperature;
  int dryLimit;

  Variables which are marked as READ/WRITE in the Cloud Thing will also have functions
  which are called when their values are changed from the Dashboard.
  These functions are generated with the Thing and added at the end of this sketch.
*/
#define COLORED 0
#define UNCOLORED 1
#define DISPLAY_UPDATE_INTERVAL 1000

#include "thingProperties.h"
#include <WiFiNINA.h>
#include <Wire.h>
#include <SPI.h>
#include <RTCZero.h>
#include "epd2in9.h"
#include "epdpaint.h"
#include "imagedata.h"
//#include "DS1307.h" //oorrect DC1307 grove lobrary
#include "DHT.h"
#include "HX711.h"
#include "constants.h"
#include <iostream>

//defining sensor objects
DHT dht(TEMP_SENS, DHT11);              //fixed notation
HX711 scale = HX711();           //weight object name changed.
RTCZero rtc_wifi;

//describes the battery object.

unsigned long currentTime(0);
unsigned long lastPrintToDisplay(0);

unsigned char image[1024];
Paint paint(image, 0, 0);    // width should be the multiple of 8
Epd epd;

int wifi = 0;
byte set_hour = 0;
byte set_minute = 0;
byte set_second = 0;


char ssid[] = SECRET_SSID;                // your network SSID (name)
char pass[] = SECRET_PASS;               // your network password (use for WPA, or use as key for WEP)*/

int status = WL_IDLE_STATUS;             // needed for RTC as well as connecting to the internet.
int ledState = LOW;                       //ledState used to set the LED
unsigned long previousMillisInfo = 0;     //will store last time Wi-Fi information was updated
unsigned long previousMillisLED = 0;      // will store the last time LED was updated
const int intervalInfo = 5000;

//placeholder figure to calibrate sensor (has potential to cause bugs)
float calibration_factor = -83886.00;
float waterLevel = 0;
float humidity = 0;
const int GMT = -5;
int EnclosureTemp = 40;
float weight = 0;
float maxWeight = 1000; //we need to find out what the weight is when full for a comparison in the loop()
float WeightValue = 1;
int wateringTick = 0;
int tempTick = 0;
int LEDTick = 0;
int weightTick = 0;
int tickDelayPump = 0;
int tickDelayLED = 0;
int tickDelayTemp = 0;
int tickDelayWeight = 0;

int waterHour;//sync with IoT to store the variables.
int waterMinute;

int LEDHour;//sync with IoT to store the variables.
int LEDMinute;

float battery_analog;

// Pushingbox API
/*
  char *api_server = “api.pushingbox.com”;
  char *deviceId1 = “v7D2202342900F3C”;
  char *deviceId2 = “vC4F10125FFD871F”;
  char *deviceId3 = “v85B903287356B3A”;

  int tag = -1; //Relating the sensor to notifications
*/

void RealTimeSet()
{
  set_hour = rtc_wifi.getHours();
  set_minute = rtc_wifi.getMinutes();
  set_second = rtc_wifi.getSeconds();
}


void setup() {
  // Initialize serial and wait for port to open:
  Serial.begin(9600);
  /***E-Paper Display Initialisation***/
  if (epd.Init(lut_full_update) != 0) {
    Serial.print("e-Paper init failed");
    return;
  }

  epd.ClearFrameMemory(0xFF);   // bit set = white, bit reset = black
  epd.DisplayFrame();
  epd.ClearFrameMemory(0xFF);   // bit set = white, bit reset = black
  epd.DisplayFrame();

  if (epd.Init(lut_partial_update) != 0) {
    Serial.print("e-Paper init failed");
    return;
  }
  /***********************************/



  Serial.print("1");
  while (!Serial);
  // This delay gives the chance to wait for a Serial Monitor without blocking if none is found
  delay(1500);
  Serial.print("2");
  // Defined in thingProperties.h
  initProperties();
  Serial.print("3");
  // Connect to Arduino IoT Cloud
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  Serial.print("4");
  /*
     The following function allows you to obtain more information
     related to the state of network and IoT Cloud connection and errors
     the higher number the more granular information you’ll get.
     The default is 0 (only errors).
     Maximum is 4
  */
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();
  Serial.print("5");
  //------------------------------------

  Serial.print("startup");

  rtc_wifi.begin();            //TODO: beginning the clock every time it starts can mess with it. Need to have oscillating if statement
  RealTimeSet();
  dht.begin();  //running default constructor

  pinMode(PUMP, OUTPUT);   // set relay pin to output
  pinMode(LED_STRIP, OUTPUT);     // set led pin to output
  pinMode(WEIGHT_CLK, INPUT);
  pinMode(WEIGHT_DOUT, INPUT);
  digitalWrite(PUMP, HIGH);


  //Weight Sensor Setup.
  //scale.begin(WEIGHT_DOUT,WEIGHT_CLK); //uncommented the constants to get this working.
  //scale.set_scale(calibration_factor);
  //scale.tare();   //reset scale to 0 assuming no weight.


  //Serial.print("Before wifi setup");

  // set the LED as output
  //pinMode(LED_BUILTIN, OUTPUT);


  // attempt to connect to Wi-Fi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to network: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);

    unsigned long epoch;

    // Variable for number of tries to NTP service
    int numberOfTries = 0, maxTries = 6;

    // Get epoch
    do {
      epoch = WiFi.getTime();
      numberOfTries++;
    }

    while ((epoch == 0) && (numberOfTries < maxTries));

    if (numberOfTries == maxTries) {
      Serial.print("NTP unreachable!!");
      while (1);
    }

    else {
      Serial.print("Epoch received: ");
      Serial.println(epoch);
      rtc_wifi.setEpoch(epoch + GMT * 60 * 60);
      Serial.println();
    }

    RealTimeSet();
    Serial.println("Initializing real time done.");

    // you're connected now, so print out the data:
    Serial.println("You're connected to the network");
    Serial.println("---------------------------------------");
  }
  Serial.println("outside connection loop");
}

void loop() {
  ArduinoCloud.update();

  currentTime = millis();

  if (currentTime - lastPrintToDisplay >= DISPLAY_UPDATE_INTERVAL) {
    Display();
    lastPrintToDisplay = currentTime;
  }
  wifiConnection();

  TempCheck();
  WeightCheck();
  BatteryLevel();
  LEDCheck();
  WaterPump();
  waterLevel =  trunc((weight / maxWeight) * 100);
  temperature = trunc(dht.readTemperature());


}


/*
  Since DryLimit is READ_WRITE variable, onDryLimitChange() is
  executed every time a new value is received from IoT Cloud.
*/
void onDryLimitChange()  {
  // Add your code here to act upon DryLimit change
}
/*
  Since MoistValue is READ_WRITE variable, onMoistValueChange() is
  executed every time a new value is received from IoT Cloud.
*/
void onMoistValueChange()  {
  // Add your code here to act upon MoistValue change
}
/*
  Since WaterLimit is READ_WRITE variable, onWaterLimitChange() is
  executed every time a new value is received from IoT Cloud.
*/
void onWaterLimitChange()  {
  // Add your code here to act upon WaterLimit change
}

void onTemperatureChange(){
  
}
void onBatteryValueChange(){
  
}

void wifiConnection() {
  unsigned long currentMillisInfo = millis();

  // check if the time after the last update is bigger the interval
  if (currentMillisInfo - previousMillisInfo >= intervalInfo) {
    previousMillisInfo = currentMillisInfo;

    Serial.println("Board Information:");
    // print your board's IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    // print your network's SSID:
    Serial.println();
    Serial.println("Network Information:");
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.println(rssi);
    Serial.println("---------------------------------------");
  }

  unsigned long currentMillisLED = millis();

  // measure the signal strength and convert it into a time interval
  int intervalLED = WiFi.RSSI() * -10;

  // check if the time after the last blink is bigger the interval
  if (currentMillisLED - previousMillisLED >= intervalLED) {
    previousMillisLED = currentMillisLED;

    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }

    // set the LED with the ledState of the variable:
    digitalWrite(LED_BUILTIN, ledState);
  }
}
//added semicolons where it needed to be done.
void WaterPump() {
  // Water Pump + Relay Function
  moistValue = analogRead(MOISTURE_SENSOR);
  Serial.print("moisture = ");
  Serial.print(moistValue);
  Serial.print("\n");
  dryLimit = 720;

  if (moistValue <= dryLimit || (rtc_wifi.getHours() == waterHour && rtc_wifi.getMinutes() == waterMinute)) {
    // enter if moisture

    if (tickDelayPump == 0) {//set to 0 at start
      // enter if pump not turned on in last 100 ticks
      wateringTick = 20;//no. of ticks to water with
      digitalWrite(PUMP, LOW);//pump turns on
      digitalWrite(BUZZER, HIGH); //buzzer turns on
      Serial.print("Pump on");
      tickDelayPump = 1;
    }

  }

  if (wateringTick == 1) {
    // if pump on ticker reaches 1, send pump off signal
    digitalWrite(PUMP, HIGH);    //pump high = off
    digitalWrite(BUZZER, LOW); //turn buzzer off
    Serial.print("Pump off");
    tickDelayPump = 100;
  }

  tickDelayPump = constrain(tickDelayPump - 1, 0, 1000); //decrement each loop stop it at 0
  wateringTick = constrain(wateringTick - 1, 0, 1000); //decrements tick by 1 and prevents it from going into minus
}

//humidity check

void LEDCheck() {
  //write a function to check the current time and minute then turn on led if the user set time is
  if (rtc_wifi.getHours() == LEDHour && rtc_wifi.getMinutes() == LEDMinute) {
    //TODO: internet time here.
    Serial.print(tickDelayLED);
    Serial.print("\n");
    if (tickDelayLED == 0) {
      LEDTick = 100;
      digitalWrite(LED_STRIP, HIGH);
      tickDelayLED = 100;
    }//TODO: remove the ticker from this, implement internet time.
  }
  if (LEDTick == 1) {
    digitalWrite(LED_STRIP, LOW);
    Serial.print("strip off");
    delay(1000);
  }
  //}
  tickDelayLED = constrain(tickDelayLED - 1, 0, 1000);
  LEDTick = constrain(LEDTick - 1, 0, 1000);

}

void TempCheck() {
  //checks the temperature
  temperature = trunc(dht.readTemperature());
  humidity = dht.readHumidity();
  if (temperature > EnclosureTemp)
    //tag = 3;
  {
    if (tickDelayTemp == 0) {
      tempTick = 30;
      digitalWrite(BUZZER, HIGH); //buzzer goes off to warn that the temperature is too high!
      Serial.print("Buzzer active");
      Serial.println("Temperature is too high!");
      tickDelayTemp = 1;
    }
  }
  if (tempTick == 1) {
    digitalWrite(BUZZER, LOW);
    Serial.print("Buzzer off");
    tickDelayTemp = 100;
  }
  tickDelayTemp = constrain(tickDelayTemp - 1, 0, 1000);
  tempTick = constrain(tempTick - 1, 0, 1000);
}



void WeightCheck() {
  //checks the weight of the water canister.
  weight = scale.get_units();
  if (weight < WeightValue)
  {
    if (tickDelayWeight == 0) {

      weightTick = 30;
      digitalWrite(BUZZER, HIGH); //buzzer goes off to warn that the temperature is too high!
      Serial.print("Refill water");
      tickDelayWeight = 1;
    }
  }
  if (weightTick == 1) {
    digitalWrite(BUZZER, LOW);
    tickDelayWeight = 100;
  }
  tickDelayWeight = constrain(tickDelayWeight - 1, 0, 1000);
  weightTick = constrain(weightTick - 1, 0, 1000);
}

void BatteryLevel() {
  //tag = 2;
  //reads the battery level and displays it.
  //likely will have to allocate a number of "bars" to a discrete battery level and display this by fault on E-INK display.

  battery_analog = analogRead(A7);
  delay(100)  //a little delay to see if we can obtain a smoother battery reading.   

  float input_volt = (battery_analog * 3.3) / 1024; //converts the battery level into an analog voltage

  float battery_percentage = ((((battery_analog / 1023) * 3.3) - 2.66) / 0.62) * 100;
  Serial.println("Battery Percantage is: ");
  battery_value = battery_percentage;

}

void Display() {
  //296 x 128

  paint.SetRotate(ROTATE_90);


  paint.SetWidth(10);
  paint.SetHeight(296);

  paint.Clear(UNCOLORED);
  char line1String[30];

  if (wifi == 0)
    sprintf(line1String, "WiFi: Not Connected");
  else
    sprintf(line1String, "WiFi: Connected");

  paint.DrawStringAt(2, 2, line1String, &Font12, COLORED);
  char line2String[30];
  sprintf(line2String, "Battery Level: %d%%", battery_value);
  paint.DrawStringAt(162, 2, line2String, &Font12, COLORED);
  epd.SetFrameMemory(paint.GetImage(), 118, 0, paint.GetWidth(), paint.GetHeight());

  paint.Clear(UNCOLORED);
  char line3String[30];
  sprintf(line3String, "Moisture Level:  %d%%", moistValue);
  paint.DrawStringAt(2, 5, line3String, &Font12, COLORED);
  char line4String[30];
  sprintf(line4String, "Temperature: %dC", temperature);
  paint.DrawStringAt(182, 5, line4String, &Font12, COLORED);
  epd.SetFrameMemory(paint.GetImage(), 20, 0, paint.GetWidth(), paint.GetHeight());

  paint.Clear(UNCOLORED);
  char line5String[30];
  sprintf(line5String, "Reservoir Level: %d%%", waterLevel);
  paint.DrawStringAt(2, 5, line5String, &Font12, COLORED);
  char line6String[30];
  sprintf(line6String, "Humidity:    %d%%", humidity);
  paint.DrawStringAt(182, 5, line6String, &Font12, COLORED);
  epd.SetFrameMemory(paint.GetImage(), 0, 0, paint.GetWidth(), paint.GetHeight());


  paint.SetWidth(16);
  paint.SetHeight(94);

  paint.Clear(UNCOLORED);
  char line7String[30];
  sprintf(line7String, "%d:%d:%d", rtc_wifi.getHours(), rtc_wifi.getMinutes(), rtc_wifi.getSeconds());
  //sprintf(line7String, "15:58:34");
  paint.DrawStringAt(2, 2, line7String, &Font16, COLORED);
  epd.SetFrameMemory(paint.GetImage(), 100, 101, paint.GetWidth(), paint.GetHeight());


  paint.SetWidth(20);
  paint.SetHeight(296);

  paint.Clear(UNCOLORED);
  char line8String[30];
  sprintf(line8String, "Autonomous Incubator");
  paint.DrawStringAt(8, 10, line8String, &Font20, COLORED);
  epd.SetFrameMemory(paint.GetImage(), 56, 0, paint.GetWidth(), paint.GetHeight());


  if (battery_value <= 20) {
    paint.SetWidth(20);
    paint.SetHeight(242);

    paint.Clear(UNCOLORED);
    char line9String[30];
    sprintf(line9String, "Battery Level Low");
    paint.DrawStringAt(2, 4, line9String, &Font20, COLORED);
    epd.SetFrameMemory(paint.GetImage(), 76, 27, paint.GetWidth(), paint.GetHeight());
  }


  if (waterLevel <= 20) {
    paint.SetWidth(20);
    paint.SetHeight(270);

    paint.Clear(UNCOLORED);
    char line10String[30];
    sprintf(line10String, "Water Reservoir Low");
    paint.DrawStringAt(2, 8, line10String, &Font20, COLORED);
    epd.SetFrameMemory(paint.GetImage(), 36, 13, paint.GetWidth(), paint.GetHeight());
  }


  epd.DisplayFrame();
  epd.ClearFrameMemory(0xFF);
}

//Code programmed by Anthony A
/*void sendNotification(float sensorValue, int tag) {
  Serial.println("Sending notification to " + String(api_server));
  if (client.connect(api_server, 80)) {
  Serial.println("Connected to the server");
  String message;
  switch(tag) {
  case 1:
    // code block
    message = "devid=" + String(deviceId1) + "\r\n\r\n";
    break;
  case 2:
    // code block
    message = "devid=" + String(deviceId2) + "\r\n\r\n";
    break;
    case 3:
    // code block
    message = "devid=" + String(deviceId3) + "\r\n\r\n";
    break;
  }

  client.print("POST /pushingbox HTTP/1.1\n");
  client.print("Host: api.pushingbox.com\n");
  client.print("Connection: close\n");
  client.print("Content-Type: application/x-www-form-urlencoded\n");
  client.print("Content-Length: ");
  client.print(message.length());
  client.print("\n\n");
  client.print(message);
  }
  client.stop();
  Serial.println("Notification sent!");
  }*/




