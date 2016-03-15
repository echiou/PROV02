#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LSM303_U.h>

#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"

#define startup 5000
#define tauntTime 5000
#define resetTime 10000

/* Assign a unique ID to this sensor at the same time */
Adafruit_LSM303_Accel_Unified accel = Adafruit_LSM303_Accel_Unified(54321);

Adafruit_7segment matrix = Adafruit_7segment();

unsigned long time;
unsigned long recordTime;
unsigned long lastTaunt;
unsigned long lastMovement;
unsigned long startUni;
boolean pickedUp;
boolean inUse;

int timeOnUni;
int numAttempts;
int timeProne;

void setup() {
  Serial.begin(9600);

  /* Initialise the sensor */
  if (!accel.begin())
  {
    /* There was a problem detecting the ADXL345 ... check your connections */
    Serial.println("Ooops, no LSM303 detected ... Check your wiring!");
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
  delay(startup);
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
  if (accel_event.acceleration.x <= 0){
    return false;
  }
  return true;
}  

void loop() {
  /* Get a new sensor event */
  sensors_event_t accel_event;
  accel.getEvent(&accel_event);
  
  boolean unicycleUp = accelerometer_up(accel_event);
  
  time = millis();
  timeProne = time - lastMovement;
  
  if (!pickedUp && unicycleUp) { // Accelerometer detected change in orientation
    if(!inUse) {
      Serial.println("Welcome to hell");
      
      inUse = true;
      recordTime = 0;
      numAttempts = 0;
    }
    pickedUp = true;
    startUni = time;
    Serial.println("Unicycle picked up!");
  } else if (pickedUp) { // Unicycle is recording data
    if (unicycleUp) { // Wait for them to fall
      delay (100);
      lastMovement = time;
      Serial.println("Waiting for you to fall");
    } else { // They fell!
      // Set time & record on matrix.
      timeOnUni = (time - startUni) / 1000; // In seconds
      Serial.println("You got to " + (String)timeOnUni + " seconds.");
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
      
      boolean drawDots = false;
      matrix.writeDigitNum(0, (recordTime / 10) % 10, drawDots);
      matrix.writeDigitNum(1, recordTime % 10, drawDots);
      matrix.drawColon(drawDots);
      matrix.writeDigitNum(3, (numAttempts / 10) % 10, drawDots);
      matrix.writeDigitNum(4, numAttempts % 10, drawDots);
    
    
      matrix.writeDisplay();     
      // Post on Twitter
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
        Serial.println("Haha you wimp");
        lastTaunt = time; // Reset taunt timer
      }
      delay(100);
    }
  }
}
