/*
 *  Adele Parsons
 *  IoT Final project 
 *  
 *  This code connects to a MQTT server and catches messages sent from the dashboard.
 *  A LED Matrix scrolls text in response to the messages.
 *  
*/

//Libraries to include
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <ESP8266WiFi.h>    //For WiFi connectivity
#include "Wire.h"           //For enabling i2c
#include <LiquidTWI.h>      //For writing to the LCD (via i2c)
#include <PubSubClient.h>   //For traversing the MQTT server
#include <ArduinoJson.h>    //For parsing/reading our JSON strings
#include <Adafruit_NeoPixel.h>

//WiFi and MQTT variables
#define wifi_ssid "Perth"
#define wifi_password "LeoTheDog"
#define topic_name "dogWearableMessage"
char message[201];
char bufferChar[50];
WiFiClient espClient;            
PubSubClient mqtt(espClient);  

//Variables to define
#define	MAX_DEVICES	4
#define	CS_PIN		15  // or SS
#define mqtt_server "mediatedspaces.net"  //this is its address, unique to the server
#define mqtt_user "hcdeiot"               //this is its server login, unique to the server
#define mqtt_password "esp8266"           //this is it server password, unique to the server


// SPI hardware interface
MD_MAX72XX mx = MD_MAX72XX(CS_PIN, MAX_DEVICES);

// We always wait a bit between updates of the display
#define  DELAYTIME  100 

// Function to scroll text across the LED matrix
void scrollText(char *p)
{
  uint8_t	charWidth;
  uint8_t	cBuf[8];	// this should be ok for all built-in fonts

  mx.clear();

  while (*p != '\0')
  {
    charWidth = mx.getChar(*p++, sizeof(cBuf) / sizeof(cBuf[0]), cBuf);

    for (uint8_t i = 0; i < charWidth + 1; i++)	// allow space between characters
    {
      mx.transform(MD_MAX72XX::TSL);
      if (i < charWidth)
        mx.setColumn(0, cBuf[i]);
      delay(DELAYTIME);
    }
  }
  delay(1000);
  mx.clear();
}

///setup wifi
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);         
  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) { //if not connected, try again . . . forever
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
}                                     

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

void setup()
{
  Serial.begin(115200);
  setup_wifi();         //setup the wifi

  mqtt.setServer(mqtt_server, 1883);  //set the server for the PubSub client
  mqtt.setCallback(callback);         //register the callback function
  mx.begin();
}

void loop() {
  if (!mqtt.connected()) {
    reconnect();
  }
  mqtt.loop();
}

//Callback for messages recieved from MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println();
  Serial.print("Message arrived [");
  Serial.print(topic); //'topic' refers to the incoming topic name, the 1st argument of the callback function
  Serial.println("] ");

  DynamicJsonBuffer  jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(payload);

  if (!root.success()) { //well?
    Serial.println("parseObject() failed, are you sure this message is JSON formatted.");
    return;
  }

  if (strcmp(topic, "dogWearableMessage") == 0) {
    Serial.println("A message is being delivered . . .");

    String line1 = root["line1"].as<String>();  //read line1 values and put them in a String
    if (line1.indexOf("feeling") != -1) {
      scrollText("I'm feeling great!");
    } else if (line1.indexOf("Hi") != -1 || line1.indexOf("hi") != -1 || line1.indexOf("Hey") != -1 || line1.indexOf("hey") != -1 || line1.indexOf("hello") != -1 || line1.indexOf("Hello") != -1) {
      scrollText("Hello friend!");
    } else if (line1.indexOf("up") != -1 || line1.indexOf("doing") != -1) {
      scrollText("I've been playing with all my toys!");
    } else if (line1.indexOf("walk") != -1) {
      scrollText("DID YOU SAY WALK!? lET'S GO!");
    } else if (line1.indexOf("squirrel") != -1) {
      scrollText("SQUIRREL! WHERE?");
    } else if (line1.indexOf("cat") != -1 || line1.indexOf("kitty") != -1) {
      scrollText("I LOVE CATS!");
    } else if (line1.indexOf("love") != -1) {
      scrollText("I loveeeeee you!");
    } else if (line1.indexOf("Park") != -1) {
      scrollText("I love the park!! I tweeted a place for us to play.");
    } else if (line1.indexOf("PetStore") != -1) {
      scrollText("Yes! I tweeted a place for me to get a toy.");
    } else if (line1.indexOf("Shopping") != -1) {
      scrollText("Let's go shopping! I tweeted a dog-friendly store.");
    } else if (line1.indexOf("Food") != -1) {
      scrollText("I'm hungry too! I tweeted a dog friendly restaurant.");
    }  else if (line1.indexOf("Vet") != -1) {
      scrollText("I don't feel good. Let's find a vet nearby.");
    } else {
      scrollText("Woof woof! Bark bark!!");
    }
  }
}

