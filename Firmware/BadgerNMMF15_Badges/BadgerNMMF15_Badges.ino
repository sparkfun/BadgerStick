/****************************************************************
BadgerNMMF15_Badges.ino
NoCo Mini Maker Faire 2015 Badger
Shawn Hymel @ SparkFun Electronics
Nick Poole @ SparkFun Electronics
Kade Drobeck @ SparkFun Electronics
August 19, 2015
https://github.com/sparkfun/BadgerStick

Receives commands from the #BadgerHack stations to display
custom text and graphics.

Hardware Connections:

IMPORTANT:  The Charlieplex LED board is designed for 2.0 - 3.3V!
            Higher voltages can damage the LEDs.

 Arduino Pin | Charlieplex Board
 ------------|------------------
      2      |         2
      3      |         3
      4      |         4
      5      |         5
      6      |         6
      7      |         7
      8      |         8
      9      |         9

Resources:
Include SoftwareSerial.h, Chaplex.h, SparkFun_LED_8x7.h, EEPROM.h
The Chaplex library can be found at: 
http://playground.arduino.cc/Code/Chaplex

Development environment specifics:
Written in Arduino 1.0.6
Tested with SparkFun BadgerStick (Interactive Badge)

This code is beerware; if you see me (or any other SparkFun 
employee) at the local, and you've found our code helpful, please
buy us a round!

Distributed as-is; no warranty is given.
****************************************************************/

#include <SoftwareSerial.h>
#include <SparkFun_LED_8x7.h>
#include <Chaplex.h>

// Parameters
#define DEBUG             1
#define LED_PIN           13

// Communications constants
#define MAX_PAYLOAD_SIZE  21
#define IN_BUF_MAX        MAX_PAYLOAD_SIZE + 2
#define SERIAL_SOF        0xD5
#define SERIAL_EOF        0xAA
#define HEADER_CLEAR      0x01
#define HEADER_TEXT       0x02
#define HEADER_BITMAP     0x03
#define HEADER_FRAME      0x04
#define HEADER_CONWAY     0x05
#define ACK               0xAC

// EEPROM addresses
#define ADDR_PROD_TEST    501

// Global variables
SoftwareSerial softy(11, 10);  // RX, TX
static byte led_pins[] = {2, 3, 4, 5, 6, 7, 8, 9};
byte in_msg[MAX_PAYLOAD_SIZE];
uint8_t bytes_received;
const byte sparkfun_logo[] = { 0,0,0,0,1,0,0,0,
                               0,0,0,0,1,0,1,0,
                               0,0,1,0,1,1,1,0,
                               0,0,1,1,1,1,1,0,
                               0,0,1,1,1,1,0,0,
                               0,0,1,1,0,0,0,0,
                               0,0,1,0,0,0,0,0 };

// Global variables for production test
int A0PinSet[] = {2, 4, 6, 8, 10, 16, 18};//7 pins
int A1PinSet[] = {3, 5, 7, 9, 17, 19};//6 pins
int ALLPINS[] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 16 ,17, 18, 19, 14, 15};//15 pins
int STAT = 13;
int A0TEST = 0;
int A1TEST = 0;
int A0_1error = 0;
int A0_2error = 0;
int A1_1error = 0;
int A1_2error = 0;
int pin_read1error = 0;
int pin_read2error = 0;
int A6val;

void setup() {
  
  uint8_t i;
  
  //***PRODUCTION TEST GOES HERE***

  // Blink a few times to show that we are alive
  pinMode(LED_PIN, OUTPUT);
  for ( i = 0; i < 3; i++ ) {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
  }
  
    // Initialize serial comms
#if DEBUG
  Serial.begin(9600);
  Serial.print(F("Production test ID: 0x"));
  Serial.println(EEPROM.read(ADDR_PROD_TEST), HEX);
#endif

  // Initialize and clear LED array
  Plex.init(led_pins);
  Plex.clear();
  Plex.display();
  
  // Seed the RNGesus
  randomSeed(analogRead(0));

}

void loop() {
  Plex.scrollText("Sup", 2);
  delay(1000);

}
