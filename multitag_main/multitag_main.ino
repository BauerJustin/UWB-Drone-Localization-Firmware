#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>

#include "DW1000Ranging.h"
#include "DW1000.h"

/******** SETTINGS *********/
#define TAG1  // use this to select the tag
//#define TAG2
//#define TAG3
#define TRANSMIT_WINDOW 125  // in ms
#define RANGING_WINDOW  2000 // in ms
#define TCP_CONNECTION_RETRY 100
/******** PIN DEFINITIONS *************/
#define SPI_SCK 18
#define SPI_MISO 19
#define SPI_MOSI 23
#define DW_CS 4     // what is this?
// connection pins
const uint8_t PIN_RST = 27; // reset pin
const uint8_t PIN_IRQ = 34; // irq pin
const uint8_t PIN_SS = 21;   // spi select pin

#define I2C_SDA 4
#define I2C_SCL 5

/******* DEVICE CONFIG ***********/
// leftmost two bytes below will become the "short address"

#ifdef TAG1
    // For TAG1 use addr 7D
    char tag_addr[] = "7D:00:22:EA:82:60:3B:9C";
#endif

#ifdef TAG2
    // For TAG2 use addr 7E
    char tag_addr[] = "7E:00:22:EA:82:60:3B:9C";
#endif

#ifdef TAG3
    // For TAG3 use addr 7F
    char tag_addr[] = "7F:00:22:EA:82:60:3B:9C";
#endif

// variables for position determination
#define N_ANCHORS 4   //THIS VERSION WORKS ONLY WITH 4 ANCHORS. May be generalized to 5 or more.
#define RANGING_RETRIES 10
#define ANCHOR_DISTANCE_EXPIRED 5000   //measurements older than this are ignore (milliseconds)

uint32_t last_anchor_update[N_ANCHORS] = {0}; //millis() value last time anchor was seen
uint16_t last_anchor_addr[N_ANCHORS] = {0}; //keep track of anchor addresses (HEX)
float last_anchor_distance[N_ANCHORS] = {0.0}; //most recent distance reports

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library.
// ESP32 SDA = GPIO21 = Stemma Blue
// ESP32 SCL = GPIO22 = Stemma Yellow
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

/****** WIFI CONFIG ********/
const char* ssid = "DCS";
const char* password = "701BA2887E";
const char* serverIP = "192.168.0.2";
const int serverPort = 12345;
WiFiClient client;
/**** JSON variables ********/
DynamicJsonDocument jsonDoc(192);
JsonObject measurements = jsonDoc.createNestedObject("measurements");

/**** For multitag ****/
uint8_t numRangeTransmitted = 0; // number of times newRange() is called
uint8_t numAnchors = 0;
uint8_t rangingCount = 0;

// UDP read buffer
char packetBuffer[2] = "";

// 1 to indicate that it can run ranging
// 0 to not run ranging
uint8_t hasToken = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);
  delay(1000);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }

  // Clear the buffer
  display.clearDisplay();
  display.setTextSize(2);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner

  display.println("UWB tag");
  display.display();

  //initialize configuration
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  DW1000Ranging.initCommunication(PIN_RST, PIN_SS, PIN_IRQ); //Reset, CS, IRQ pin

  //set callbacks
  DW1000Ranging.attachNewRange(newRange);
  DW1000Ranging.attachNewDevice(newDevice);
  DW1000Ranging.attachInactiveDevice(inactiveDevice);

  // start as tag, do not assign random short address
  //  DW1000Ranging.startAsTag(tag_addr, DW1000.MODE_LONGDATA_RANGE_LOWPOWER, false);
  //  DW1000Ranging.startAsTag(tag_addr, DW1000.MODE_SHORTDATA_FAST_LOWPOWER, false);  // range 7 m  smart power 10 m
  DW1000Ranging.startAsTag(tag_addr, DW1000.MODE_LONGDATA_FAST_LOWPOWER, false);  // 6.5 m
  //  DW1000Ranging.startAsTag(tag_addr, DW1000.MODE_SHORTDATA_FAST_ACCURACY, false);  // 5 m, smart power 5 m
  //  DW1000Ranging.startAsTag(tag_addr, DW1000.MODE_LONGDATA_FAST_ACCURACY, false);  // 5 m
  //  DW1000Ranging.startAsTag(tag_addr, DW1000.MODE_LONGDATA_RANGE_ACCURACY, false);  // 5 m
  /*
    The following modes are pre-configured and one of them needs to be chosen:
    - `MODE_LONGDATA_RANGE_LOWPOWER` (basically this is 110 kb/s data rate, 16 MHz PRF and long preambles)
    - `MODE_SHORTDATA_FAST_LOWPOWER` (basically this is 6.8 Mb/s data rate, 16 MHz PRF and short preambles)
    - `MODE_LONGDATA_FAST_LOWPOWER` (basically this is 6.8 Mb/s data rate, 16 MHz PRF and long preambles)
    - `MODE_SHORTDATA_FAST_ACCURACY` (basically this is 6.8 Mb/s data rate, 64 MHz PRF and short preambles)
    - `MODE_LONGDATA_FAST_ACCURACY` (basically this is 6.8 Mb/s data rate, 64 MHz PRF and long preambles)
    - `MODE_LONGDATA_RANGE_ACCURACY` (basically this is 110 kb/s data rate, 64 MHz PRF and long preambles)
  */
  //   DW1000Class::useSmartPower(true); //enable higher power for short messages
  
  // Start Wifi
  long int wifiStartTime = millis();

  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - wifiStartTime >= 10000) { // 10 second timeout
      Serial.println("Failed to connect to WiFi. Exiting setup.");
      return;
    }
    delay(1000);
    Serial.print(".");
  }

  Serial.println("Connected to WiFi");

  connectToServer(client);
  // connect to tcp server
  if(client.connected()){
    Serial.println("Connected to TCP server");
    client.setNoDelay(true);
    // send initial packet to GCS. (doesn't matter what is sent, as only the IP and port is needed)
    sendMeasurements(&jsonDoc, &measurements, client);
  }else{
    Serial.println("Cannot connect to TCP client");
  }
}

unsigned long lastDisplayTime = 0;
unsigned long tokenTime = 0;

void loop() {
  // put your main code here, to run repeatedly:

  client.setNoDelay(true);
  // if connection was dropped, reconnect
  if(!client.connected()){
    connectToServer(client);
    // send initial packet to GCS. (doesn't matter what is sent, as only the IP and port is needed)
    sendMeasurements(&jsonDoc, &measurements, client);
  }
    
  // if there are data to read from
  if(client.available())
  {
    // clear RX buffer
    client.flush();
    // received the token
    hasToken = 1;
    tokenTime = millis();
  }

  // run ranging if hasToken = 1  
  if (hasToken)
  {
    // get measurements
    DW1000Ranging.loop();
  }
  
  if (rangingCount < RANGING_RETRIES)
  {
    if ((millis() - tokenTime > RANGING_WINDOW) && hasToken)
    {
      // Call createJsonPackage to populate the JSON document
      createJsonPackage(&jsonDoc, &measurements);
      
      // Call transmitJsonPackage to send the UDP message
      transmitJsonPackage(&jsonDoc, client);
      
      hasToken = 0; // release token after sending measurements
      rangingCount++;
    }
  }
  else
  {
    if (((millis() - tokenTime > TRANSMIT_WINDOW)
    ||(numRangeTransmitted == N_ANCHORS))
    && hasToken)// can add numRangeTransmitted == numAnchors here later
    {
      // Call createJsonPackage to populate the JSON document
      createJsonPackage(&jsonDoc, &measurements);
      
      // Call transmitJsonPackage to send the UDP message
      transmitJsonPackage(&jsonDoc, client);
      
      hasToken = 0; // release token after sending measurements
      numRangeTransmitted = 0;
    }
  }

  if ((millis() - lastDisplayTime) > 1000) // every 1 second
  {
      display_uwb();
      lastDisplayTime = millis();
  }
}

void display_uwb()
{
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2); //Normal 1:1 pixel scale
  display.setCursor(0, 0); //Start at top-left corner

  //loop through all current anchor distances and print
  for (int i = 0; i < N_ANCHORS; i++)
  {
    display.print(last_anchor_addr[i], HEX);
    display.print(":");
    display.println(last_anchor_distance[i], 2); //print 2 decimal places
  }
  display.display();
}

//newRange callback
void newRange()
{
  int i;

  //index of this anchor, expecting values 1 to 4
  uint16_t addr = DW1000Ranging.getDistantDevice()->getShortAddress();
  int index = addr & 0x07; //expect devices 1 to 7
  if (index > 0 && index < 5) {
    // note down how many anchors has transmitted their range
    numRangeTransmitted++;
    last_anchor_update[index - 1] = millis();  //(-1) => array index
    float range = DW1000Ranging.getDistantDevice()->getRange();
    last_anchor_distance[index - 1] = range;
    last_anchor_addr[index - 1] = addr;
  }
}  //end newRange

void newDevice(DW1000Device *device)
{
//  Serial.print("Device added: ");
//  Serial.println(device->getShortAddress(), HEX);
  numAnchors++;
}

void inactiveDevice(DW1000Device *device)
{
//  Serial.print("delete inactive device: ");
//  Serial.println(device->getShortAddress(), HEX);
  numAnchors--;
}

void createJsonPackage(DynamicJsonDocument *jsonDoc, JsonObject *measurements) {
  char macHexString[3];
  
  for (int i = 0; i < N_ANCHORS; i++) {
    if (last_anchor_distance[i] == 0.0) continue; // Skip empty anchors
    sprintf(macHexString, "%02x", last_anchor_addr[i]);
    (*measurements)[macHexString] = last_anchor_distance[i]; //send raw distances
  }
  
  char id[3];
  strncpy(id, tag_addr, 2);
  id[2] = '\0'; // Null-terminate the string
  (*jsonDoc)["id"] = id;

  (*jsonDoc)["timestamp"] = float(millis() / 1000.0); // Using tag's timestamp
}

void transmitJsonPackage(DynamicJsonDocument *jsonDoc, WiFiClient &client) {
  String jsonStr;
  serializeJson(*jsonDoc, jsonStr);
  int i = 0;
  
  // send packet if connected
  if (client.connected())
    client.print(jsonStr);
  
}

void sendMeasurements(DynamicJsonDocument *jsonDoc, JsonObject *measurements, WiFiClient &client){
  // Call createJsonPackage to populate the JSON document
  createJsonPackage(jsonDoc, measurements);
  // Call transmitJsonPackage to send the message
  transmitJsonPackage(jsonDoc, client);
}
inline void connectToServer(WiFiClient &client){
  int i = 0;
  while(!client.connected() && i < TCP_CONNECTION_RETRY){
    i++;
    Serial.println("TCP Server not connected, retrying...");
    client.connect(serverIP, serverPort);
    delay(100*i); // small delay to buffer the request
  }
}