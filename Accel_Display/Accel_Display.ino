#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LSM303_U.h>

#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"

#define startup 10000
#define tauntTime 5000
#define resetTime 30000

/* Assign a unique ID to this sensor at the same time */
Adafruit_LSM303_Accel_Unified accel = Adafruit_LSM303_Accel_Unified(54321);

Adafruit_7segment matrix = Adafruit_7segment();

unsigned long time;
unsigned long lastTaunt;
unsigned long lastMovement;
unsigned long startUni;
boolean pickedUp;

int timeOnUni;
int numAttempts;

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
  
  // Wait 10s to startup (to place unicycle down, etc.)
  delay(startup);
  pickedUp = false;
  lastTaunt = millis();
  lastMovement = 0;
  
  timeOnUni = 0;
  numAttempts = 0;
}

void loop() {
  /* Get a new sensor event */
  sensors_event_t accel_event;
  accel.getEvent(&accel_event);
  
  time = millis();
  
  if (!pickedUp && true) { // Accelerometer detected change in orientation
    pickedUp = true;
    startUni = time;
  } else if (pickedUp && true && lastMovement < time - resetTime) { // Reset if accelerometer down & last movement was 30s ago
    pickedUp = false;
    
    // Reset counters
    matrix.writeDigitNum(0, 0);
    matrix.writeDigitNum(1, 0);
    matrix.writeDigitNum(3, 0);
    matrix.writeDigitNum(4, 0);
    matrix.writeDisplay();
    // Reset records
    timeOnUni = 0;
    numAttempts = 0;
  }
  
  if (pickedUp) { // Unicycle is currently up
    if (true) { // Past threshold accelerometer, i.e. person fell
      // Set time & record on matrix.
      // Post on Twitter
      // Play negative comment
      pickedUp = false;
      timeOnUni = (time - startUni) / 1000; // In seconds
      if (timeOnUni >= 100) { // Just put 99
        matrix.writeDigitNum(3, 9);
        matrix.writeDigitNum(4, 9);
      } else {
        matrix.writeDigitNum(3, (timeOnUni / 10) % 10);
        matrix.writeDigitNum(4, timeOnUni % 10);
      }
      
      numAttempts++;
      if(numAttempts >= 100) {
        matrix.writeDigitNum(1, 9);
        matrix.writeDigitNum(2, 9);
      } else {
        matrix.writeDigitNum(1, (numAttempts / 10) % 10);
        matrix.writeDigitNum(2, numAttempts % 10);
      }
      
      startUni = 0;
    } else { // Waiting for them to fall
      delay (10);
    }
    lastMovement = time;
  }
  else {
    if (lastTaunt < time - tauntTime){ // Taunt every tauntTime ms
      // Play taunt
      lastTaunt = time; // Reset taunt timer
    }
    delay(500);
  }
  matrix.writeDisplay();
}
