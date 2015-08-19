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
#define MAX_ACTIONS       15    // Number of actions per loop
#define MAX_TEXT_LENGTH   20    // Number of characters (>7)
#define BITMAP_DISP_TIME  2000  // Time (ms) to pause on bitmap
#define FRAME_DISP_TIME   100   // Time (ms) to pause on frame

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

// Other constants
#define BITMAP_SIZE       7     // Number of bytes in bitmap
#define BITS_PER_BYTE     8

// EEPROM addresses
#define ADDR_PROD_TEST    501

// Global variables
SoftwareSerial softy(11, 10);  // RX, TX
static byte led_pins[] = {2, 3, 4, 5, 6, 7, 8, 9};
byte in_msg[MAX_PAYLOAD_SIZE];
uint8_t bytes_received;
uint8_t actions[MAX_ACTIONS];
byte actions_table[MAX_TEXT_LENGTH * MAX_ACTIONS];
uint8_t action_cnt;
uint8_t action_len;
const byte flame00[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20};
const byte flame01[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x20};
const byte flame02[] = {0x00, 0x00, 0x00, 0x00, 0x3C, 0x30, 0x20};
const byte flame03[] = {0x00, 0x00, 0x00, 0x3E, 0x3C, 0x30, 0x20};
const byte flame04[] = {0x00, 0x00, 0x2E, 0x3E, 0x3C, 0x30, 0x20};
const byte flame05[] = {0x00, 0x0A, 0x2E, 0x3E, 0x3C, 0x30, 0x20};
const byte sparkfun_logo[] = {0x08, 0x0A, 0x2E, 0x3E, 0x3C, 
                                                0x30, 0x20};
const char initial_text[] = "#BadgerHack";


// Global variables for production test
int A0PinSet[] = {2, 4, 6, 8, 10, 16, 18};//7 pins
int A1PinSet[] = {3, 5, 7, 9, 17, 19};//6 pins
int ALLPINS[] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 16 ,17, 18, 19, 14, 
                                                    15};//15 pins
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
  const char *ptr;
  
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
  Serial.println(F("NMMF '15 Badgers"));
#endif

  // Initialize LED array
  Plex.init(led_pins);
  
  // Initialize actions tables
  clearActions();
  
  // Seed the RNGesus
  randomSeed(analogRead(0));
  
  // Set initial actions
  ptr = (const char *)flame00;
  addAction(HEADER_FRAME, ptr, 0);
  ptr = (const char *)flame01;
  addAction(HEADER_FRAME, ptr, 0);
  ptr = (const char *)flame02;
  addAction(HEADER_FRAME, ptr, 0);
  ptr = (const char *)flame03;
  addAction(HEADER_FRAME, ptr, 0);
  ptr = (const char *)flame04;
  addAction(HEADER_FRAME, ptr, 0);
  ptr = (const char *)flame05;
  addAction(HEADER_FRAME, ptr, 0);
  ptr = (const char *)sparkfun_logo;
  addAction(HEADER_BITMAP, ptr, 0);
  ptr = initial_text;
  addAction(HEADER_TEXT, ptr, strlen(ptr));

}

void loop() {
  
  // Loop through actions
  for ( action_cnt = 0; action_cnt < action_len; action_cnt++ ) {
    performAction(action_cnt);
  }
}

/***************************************************************
 * High Level Functions
 **************************************************************/

// Clear all actions from the table
void clearActions() {
  
  // Clear display
  Plex.clear();
  Plex.display();
  
  // Clear action tables
  memset(actions, 0, MAX_ACTIONS);
  memset(actions_table, 0, MAX_TEXT_LENGTH * MAX_ACTIONS);
  
  // Clear actions length and counter
  action_cnt = 0;
  action_len = 0;
}

// Add an action to the table
bool addAction(uint8_t action, const void *buf, uint8_t len) {
   
  // If we are at max actions, don't add anything
  if ( action_len + 1 >= MAX_ACTIONS ) {
    return false;
  }
  
  // Predefine the offset into the actions table
  uint16_t offset = action_len * MAX_TEXT_LENGTH * sizeof(byte);
  
  // What action we add is based on the action header
  switch ( action ) {
    
    // Add text to the buffer
    case HEADER_TEXT:
      actions[action_len] = HEADER_TEXT;
      if ( len >= MAX_TEXT_LENGTH ) {
        len = MAX_TEXT_LENGTH;
      }
      memcpy(actions_table + offset, buf, len);
      break;
      
    // Add bitmap to the buffer
    case HEADER_BITMAP:
      actions[action_len] = HEADER_BITMAP;
      memcpy(actions_table + offset, buf, BITMAP_SIZE);
      break;
      
    // Add frame to buffer
    case HEADER_FRAME:
      actions[action_len] = HEADER_FRAME;
      memcpy(actions_table + offset, buf, BITMAP_SIZE);
      break;
      
    // Unknown case. Exit.
    default:
      return false;
  }
  
  // Increment number of actions
  action_len++;
  
  return true;
}

// Perform the desired action based on the number provided
void performAction(uint8_t index) {
  
  // Make sure index is not out of range
  if ( index >= action_len ) {
    return;
  }
  
  // Predefine the offset into the actions table
  uint16_t offset = index * MAX_TEXT_LENGTH * sizeof(byte);
  
  // Clear the display in preparation
  Plex.clear();
  Plex.display();
  
  // Do the thing we said we would do
  switch ( actions[index] ) {
    
    // Scroll text once
    case HEADER_TEXT:
      char text_buf[MAX_TEXT_LENGTH];
      memset(text_buf, 0, MAX_TEXT_LENGTH);
      memcpy(text_buf, actions_table + offset, MAX_TEXT_LENGTH);
      Plex.scrollText(text_buf, 1, true);
      break;
      
    // Show bitmap for a set amount of time
    case HEADER_BITMAP:
      showBitmap(offset, BITMAP_DISP_TIME);
      break;
      
    // Show bitmap (frame) for short amount of time
    case HEADER_FRAME:
      showBitmap(offset, FRAME_DISP_TIME);
      break;
      
    // Unknown index. End.
    default:
      return;
  }
}

/***************************************************************
 * Animation/Display Functions
 **************************************************************/

void showBitmap(uint16_t offset, unsigned long time) {
  
  byte bitmap_buf[NUM_LEDS];
  
  // Translate bytes to bit array
  for ( uint8_t y = 0; y < BITMAP_SIZE; y++ ) {
    for ( uint8_t i = 0; i < BITS_PER_BYTE; i++ ) {
      bitmap_buf[(y * BITS_PER_BYTE) + i] = 
          (actions_table[offset + y] >> i) & 0x01;
    }
  }
  
  // Draw on LEDs
  Plex.drawBitmap(bitmap_buf);
  Plex.display();
  
  // Wait for the specified time (ms)
  delay(time);
}
