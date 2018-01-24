/*
 * Adele Parsons
 * IoT Final project
 * 
 */

//Libraries to include
#include <ESP8266WiFi.h>    //For WiFi connectivity
#include "Wire.h"           //For enabling i2c
#include <LiquidTWI.h>      //For writing to the LCD (via i2c)
#include <PubSubClient.h>   //For traversing the MQTT server
#include <ArduinoJson.h>    //For parsing/reading our JSON strings
#include <Adafruit_NeoPixel.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>


#define wifi_ssid "Perth"
#define wifi_password "LeoTheDog"

#define PIN 13


#define mqtt_server "mediatedspaces.net"  //this is its address, unique to the server
#define mqtt_user "hcdeiot"               //this is its server login, unique to the server
#define mqtt_password "esp8266"           //this is it server password, unique to the server

WiFiClient espClient;         
PubSubClient mqtt(espClient);

#define topic_name "rgbChange" // Unique topic name to subscribe to

char message[201]; //201, as last character in the array is the NULL character, denoting the end of the array

//Initialize pixels and lux sensor
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(60, PIN, NEO_GRB + NEO_KHZ800);
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);

//And here are three variables (unsigned ints) to hold our 3 color values
//we use unsigned ints to prevent the possibility of negative (-) values
unsigned int r; //red
unsigned int g; //green
unsigned int b; //blue
String light = "true";
String night;
String clicked;
int NUM_LEDS = 60;
bool nightMode = false;

void configureSensor(void)
{
  tsl.enableAutoRange(true);            /* Auto-gain ... switches automatically between 1x and 16x */

  /* Changing the integration time gives you better sensor resolution (402ms = 16-bit data) */
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);      /* fast but low resolution */
}

///SETUP
void setup() {
  Serial.begin(115200); //we keep the Serial lines fast as there is a good deal of activity to catch
  setup_wifi();         //setup the wifi

  mqtt.setServer(mqtt_server, 1883);  //set the server for the PubSub client
  mqtt.setCallback(callback);         //register the callback function

  pixels.begin(); // This initializes the NeoPixel library.
  for (int i = 0; i < 60; i++) { // Start with white pixels
    pixels.setPixelColor(i, pixels.Color(10, 10, 10));
    pixels.show();
    delay(50);
  }
  if (!tsl.begin())
  {
    /* There was a problem detecting the TSL2561 ... check your connections */
    Serial.print("Ooops, no TSL2561 detected ... Check your wiring or I2C ADDR!");
    while (1);
  }
  configureSensor();

}


///setup WiFi
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);              // We start by connecting to a WiFi network
  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) { //if not connected, try again . . . forever
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");

//Monitor the connection to MQTT server, if down, reconnect
void reconnect() {
  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqtt.connect(topic_name, mqtt_user, mqtt_password)) {
      Serial.println("connected");
      mqtt.subscribe(topic_name);
    } else {                      
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
  if (!mqtt.connected()) {
    reconnect();
  }
  mqtt.loop();

  // If nighttime mode is on, then turn the pixels on if it is dark out
  if (nightMode == true) {
    sensors_event_t event;
    tsl.getEvent(&event);
    if (event.light <= 10) {
      for (int i = 0; i < 60; i++) { // Red pixels
        pixels.setPixelColor(i, pixels.Color(10, 10, 10));
        pixels.show();
        delay(50);
      }
    } else if (event.light > 10) {
      for (int i = 0; i < 60; i++) { // Red pixels
        pixels.setPixelColor(i, pixels.Color(0, 0, 0));
        pixels.show();
        delay(50);
      }
    }
  }
}

// Function for the LED Pixels
uint32_t Wheel(byte WheelPos) {
  if (WheelPos < 85) {
    return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if (WheelPos < 170) {
    WheelPos -= 85;
    return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
    WheelPos -= 170;
    return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

// Callback from the MQTT server
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println();
  Serial.print("Message arrived [");
  Serial.print(topic); //'topic' refers to the incoming topic name, the 1st argument of the callback function
  Serial.println("] ");

  DynamicJsonBuffer  jsonBuffer; //blah blah blah a DJB
  JsonObject& root = jsonBuffer.parseObject(payload); //parse it!

  if (!root.success()) { //well?
    Serial.println("parseObject() failed, are you sure this message is JSON formatted.");
    return;
  }
  
  if (strcmp(topic, "rgbChange") == 0) {
    Serial.println("A message for the RGB led . . ."); //debug message
    r = root["r"].as<int>();  //read the r, g, b values from the parsed JSON string
    g = root["g"].as<int>();
    b = root["b"].as<int>();
    for (int i = 0; i < 60; i++) {
      pixels.setPixelColor(i, pixels.Color(r, g, b));
      pixels.show();
      delay(50);
    }
    light = root["light"].as<String>();
    night = root["night"].as<String>();
    clicked = root["clicked"].as<String>();
  }
  if (light == "false") {    //if the toggle is off (false) set the colors to (0, 0, 0), or off
    for (int i = 0; i < 60; i++) {
      pixels.setPixelColor(i, pixels.Color(0, 0, 0));
      pixels.show();
      delay(50);
    }
  }

  else if (light == "true") { //if true, set to current r, g, b values
    Serial.print("light on");
    for (int i = 0; i < 60; i++) {
      pixels.setPixelColor(i, pixels.Color(10, 10, 10));
      pixels.show();
      delay(50);
    }
  } else if (clicked == "true") {
    for (int i = 0; i < 60; i++) {
      pixels.setPixelColor(i, pixels.Color(0, 0, 0));
      pixels.show();
      delay(50);
    }
    uint16_t i, j;

    for (j = 0; j < 256 * 3; j++) {
      for (i = 0; i < pixels.numPixels(); i++) {
        pixels.setPixelColor(i, Wheel(((i * 256 / pixels.numPixels()) + j) & 255));
      }
      pixels.show();
    }
  } else if (night == "true") {
    nightMode = true;
  } else if (night == "false") {
    nightMode = false;
  }
}
