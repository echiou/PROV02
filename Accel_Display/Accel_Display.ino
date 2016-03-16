#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LSM303_U.h>
#include <math.h>

#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"

#include <Wtv020sd16p.h>

int resetPin = 2;  // The pin number of the reset pin.
int clockPin = 6;  // The pin number of the clock pin.
int dataPin = 4;  // The pin number of the data pin.
int busyPin = 5;  // The pin number of the busy pin.

#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <string.h>
#include "utility/debug.h"

// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  9
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIVIDER); // you can change this clock speed
//#define WLAN_SSID       "http://localhost:8080"            // cannot be longer than 32 characters!
//#define WLAN_PASS       "#Yolo$WAGG420"


#define WLAN_SSID       "prashan"            // cannot be longer than 32 characters!
#define WLAN_PASS       "prashan7"
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

#define IDLE_TIMEOUT_MS  3000      // Amount of time to wait (in milliseconds) with no data 
                                   // received before closing the connection.  If you know the server
                                   // you're accessing is quick to respond, you can reduce this value.

// What page to grab!
#define WEBSITE      "cyclepath-posts.herokuapp.com"
#define WEBPAGE      "/tweet"


#define startup 5000
#define tauntTime 5000
#define resetTime 10000
#define numFsounds 6
#define numTsounds 3

/* Assign a unique ID to this sensor at the same time */
Adafruit_LSM303_Accel_Unified accel = Adafruit_LSM303_Accel_Unified(54321);

Adafruit_7segment matrix = Adafruit_7segment();

Wtv020sd16p wtv020sd16p(resetPin,clockPin,dataPin,busyPin);

unsigned long time;
unsigned long recordTime;
unsigned long lastTaunt;
unsigned long lastMovement;
unsigned long startUni;
boolean pickedUp;
boolean inUse;

const uint8_t off = 0;

int timeOnUni;
int numAttempts;
int timeProne;

uint32_t ip;
  
void playSound(bool fall){
  // open file by name
  int s = fall ? random(numFsounds) : random(numTsounds)+6;
  wtv020sd16p.asyncPlayVoice(s);
}

void setup() {
  Serial.begin(9600);
  Serial.println("hello");
  wtv020sd16p.reset();

  /* Initialise the module */
  Serial.println(F("\nInitializing..."));
  if (!cc3000.begin())
  {
    Serial.println(F("Couldn't begin()! Check your wiring?"));
    while(1);
  }
  Serial.print(F("\nAttempting to connect to ")); Serial.println(WLAN_SSID);
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(F("Failed!"));
    while(1);
  }
  Serial.println(F("Connected!"));
  
  /* Wait for DHCP to complete */
  Serial.println(F("Request DHCP"));
  while (!cc3000.checkDHCP())
  {
    delay(100); // ToDo: Insert a DHCP timeout!
  }  

  ip = 0;
  // Try looking up the website's IP address
  Serial.print(WEBSITE); Serial.print(F(" -> "));
  while (ip == 0) {
    if (! cc3000.getHostByName(WEBSITE, &ip)) {
      Serial.println(F("Couldn't resolve!"));
    }
    delay(500);
  }
  cc3000.printIPdotsRev(ip);
  
  /* Initialise the sensor */
  if (!accel.begin())
  {
    /* There was a problem detecting the ADXL345 ... check your connections */
    //Serial.println("Ooops, no LSM303 detected ... Check your wiring!");
    while (1);
  }

  matrix.begin(0x70);
  
  // Reset digits
  matrix.writeDigitNum(0, 0);
  matrix.writeDigitNum(1, 0);
  matrix.writeDigitNum(3, 0);
  matrix.writeDigitNum(4, 0);
  matrix.writeDisplay();
  
  // Wait 10s to startup (to place unicycle down, etc.)
  //delay(startup);
  Serial.println("Let's begin");
  pickedUp = false;
  inUse = false;
  lastTaunt = millis();
  lastMovement = 0;
  timeProne = 0;
  
  timeOnUni = 0;
  numAttempts = 0;
}

boolean accelerometer_up(sensors_event_t accel_event) {
  if (accel_event.acceleration.y <= 0){
    return false;
  }
  return true;
}  

void makePostRequest(String data) 
{
  
  Adafruit_CC3000_Client client = cc3000.connectTCP(ip, 80);

  if (client.connected()) {
       // Post request from tutorial online
    Serial.print("Sending to Server: ");                    
    client.println("POST /tweet HTTP/1.1");           
    Serial.print("POST /tweet HTTP/1.1");           
    client.println("Host: cyclepath-posts.herokuapp.com");
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.println("Connection: close");
    client.println("User-Agent: Arduino/1.0");
    client.print("Content-Length: ");
    client.println(data.length());
    client.println();
    client.print(data);
    client.println();                           
  } else {
    Serial.println(F("Connection failed"));    
    return;
  }
  /* Read data until either the connection is closed, or the idle timeout is reached. */ 
  unsigned long lastRead = millis();
  while (client.connected() && (millis() - lastRead < IDLE_TIMEOUT_MS)) {
    while (client.available()) {
      char c = client.read();
      Serial.print(c);
      lastRead = millis();
    }
  }
  
  client.close();
  Serial.println(F("-------------------------------------"));
  
  /* You need to make sure to clean up after yourself or the CC3000 can freak out */
  /* the next time your try to connect ... */
  //Serial.println(F("\n\nDisconnecting"));
  //cc3000.disconnect();
  

}

void loop() {
  /* Get a new sensor event */
  sensors_event_t accel_event;
  accel.getEvent(&accel_event);
  
  boolean unicycleUp = accelerometer_up(accel_event);
  
  time = millis();
  timeProne = round(time - lastMovement);
  
  if (!pickedUp && unicycleUp) { // Accelerometer detected change in orientation
    if(!inUse) {
      //Serial.println("Welcome to hell");
      
      inUse = true;
      recordTime = 0;
      numAttempts = 0;
    }
    pickedUp = true;
    startUni = time;
    Serial.println("Unicycle picked up!");
  } else if (pickedUp) { // Unicycle is recording data
    boolean drawDots = false;
    if (unicycleUp) { // Wait for them to fall
      delay (100);
      lastMovement = time;
      Serial.println("Waiting for you to fall");
    } else { // They fell!
      // Set time & record on matrix.
      timeOnUni = (time - startUni) / 1000; // In seconds
      if (timeOnUni > 100) {
        timeOnUni = 99;
      }
      Serial.println("You got to " + (String)timeOnUni + " seconds.");
      
      playSound(true);
      
      // Flash time (Maybe TODO: Modularize this?)         
      matrix.writeDigitRaw(0, off);
      matrix.writeDigitRaw(1, off);
      matrix.drawColon(drawDots);
      matrix.writeDigitNum(3, (numAttempts / 10) % 10, drawDots);
      matrix.writeDigitNum(4, numAttempts % 10, drawDots);
    
      matrix.writeDisplay();
      delay(200);
      
      matrix.writeDigitNum(0, (timeOnUni / 10) % 10, drawDots);
      matrix.writeDigitNum(1, timeOnUni % 10, drawDots);
      matrix.drawColon(drawDots);
      matrix.writeDigitNum(3, (numAttempts / 10) % 10, drawDots);
      matrix.writeDigitNum(4, numAttempts % 10, drawDots);
    
      matrix.writeDisplay(); 
      delay(750);
      
      matrix.writeDigitRaw(0, off);
      matrix.writeDigitRaw(1, off);
      matrix.drawColon(drawDots);
      matrix.writeDigitNum(3, (numAttempts / 10) % 10, drawDots);
      matrix.writeDigitNum(4, numAttempts % 10, drawDots);
    
      matrix.writeDisplay();
      delay(200);
      
      matrix.writeDigitNum(0, (timeOnUni / 10) % 10, drawDots);
      matrix.writeDigitNum(1, timeOnUni % 10, drawDots);
      matrix.drawColon(drawDots);
      matrix.writeDigitNum(3, (numAttempts / 10) % 10, drawDots);
      matrix.writeDigitNum(4, numAttempts % 10, drawDots);
    
      matrix.writeDisplay(); 
      delay(750);
      
      matrix.writeDigitRaw(0, off);
      matrix.writeDigitRaw(1, off);
      matrix.drawColon(drawDots);
      matrix.writeDigitNum(3, (numAttempts / 10) % 10, drawDots);
      matrix.writeDigitNum(4, numAttempts % 10, drawDots);
    
      matrix.writeDisplay();
      delay(200);
      
      matrix.writeDigitNum(0, (timeOnUni / 10) % 10, drawDots);
      matrix.writeDigitNum(1, timeOnUni % 10, drawDots);
      matrix.drawColon(drawDots);
      matrix.writeDigitNum(3, (numAttempts / 10) % 10, drawDots);
      matrix.writeDigitNum(4, numAttempts % 10, drawDots);
    
      matrix.writeDisplay(); 
      delay(750);
      
      if (timeOnUni > recordTime) {
        recordTime = timeOnUni;
      }
      if (recordTime >= 100) { // Just put 99
        recordTime = 99;
      }
      numAttempts++;
      if(numAttempts >= 100) {
        numAttempts = 99;
      }
      
      matrix.writeDigitNum(0, (recordTime / 10) % 10, drawDots);
      matrix.writeDigitNum(1, recordTime % 10, drawDots);
      matrix.drawColon(drawDots);
      matrix.writeDigitNum(3, (numAttempts / 10) % 10, drawDots);
      matrix.writeDigitNum(4, numAttempts % 10, drawDots);
    
      matrix.writeDisplay();     
      // Post on Twitter
      if(numAttempts % 3 == 0){
        makePostRequest(String(numAttempts));
      }
      // Play negative comment
      

      Serial.println("Haha you fell");
      startUni = 0;
      lastMovement = time;
      pickedUp = false;
    }
  } else if (!pickedUp && !unicycleUp) {
    if (inUse && timeProne > resetTime) { // Prone for 10s
      // Reset digits
      matrix.writeDigitNum(0, 0);
      matrix.writeDigitNum(1, 0);
      matrix.writeDigitNum(3, 0);
      matrix.writeDigitNum(4, 0);
      matrix.writeDisplay();
      // Reset records
      timeOnUni = 0;
      numAttempts = 0;
      inUse = false;
      Serial.println("You gave up!");
      lastTaunt = time;
    } else if (inUse) {
      Serial.println("Waiting for you to try again");
      delay(100);
    } else {
      if (lastTaunt < time - tauntTime){ // Taunt every tauntTime ms
        // Play taunt
        playSound(false);
        Serial.println("Haha you wimp");
        lastTaunt = time; // Reset taunt timer
      }
      delay(100);
    }
  }
}
