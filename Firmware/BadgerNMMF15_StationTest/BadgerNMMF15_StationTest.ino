/****************************************************************
BadgerNMMF15_StationTest.ino
NoCo Mini Maker Faire 2015 Badger
Shawn Hymel @ SparkFun Electronics
Nick Poole @ SparkFun Electronics
August 26, 2015
https://github.com/sparkfun/BadgerStick

Tests "reprogramming" the NMMF'15 Badgers.

Resources:
Include SoftwareSerial.h
The Chaplex library can be found at: 
http://playground.arduino.cc/Code/Chaplex

Development environment specifics:
Written in Arduino 1.6.3
Tested with Arduino Pro Mini 3.3V/8MHz

This code is beerware; if you see me (or any other SparkFun 
employee) at the local, and you've found our code helpful, please
buy us a round!

Distributed as-is; no warranty is given.
****************************************************************/

#include <SoftwareSerial.h>

// Parameters
#define DEBUG             0
#define RX_TIMEOUT        1000  // ms
#define MAX_TEXT_LENGTH   20    // Number of characters (>7)
#define TEXT_BTN          9
#define IMAGE_BTN         8
#define CONWAY_BTN        7
#define ANIM_BTN          6
#define TRIG_PIN          5
#define LED_PIN           13
#define TEXT_LED          A3
#define IMAGE_LED         A2
#define CONWAY_LED        A1
#define ANIM_LED          A0

// Other constants
#define BITMAP_SIZE       7     // Number of bytes in bitmap
#define BITS_PER_BYTE     8

// Communications constants
#define SOFT_BAUD_RATE    1200
#define MAX_PAYLOAD_SIZE  MAX_TEXT_LENGTH + 2
#define IN_BUF_MAX        MAX_PAYLOAD_SIZE + 2
#define SERIAL_BEGIN      0x55
#define SERIAL_SOF        0xD5
#define SERIAL_EOF        0xAA
#define HEADER_CLEAR      0x01
#define HEADER_TEXT       0x02
#define HEADER_BITMAP     0x03
#define HEADER_FRAME      0x04
#define HEADER_CONWAY     0x05
#define HEADER_ANIMATION  0x06
#define HEADER_ACK        0xAC
#define HEADER_BREAK      0xBE

// Animation constants
#define ANIM_RAIN         1
#define ANIM_EXPLOSIONS   2

// Example actions
const char test_str[] = "ABCDEFGHIJKLMNOPQRSTUV";
const byte test_img[] = {0x00,0x24,0x24,0x00,0x42,0x3C,0x00};
const byte test_conway[] = {0x00,0x00,0x00,0x00,0x60,0xA0,0x20};
const byte test_anim = ANIM_EXPLOSIONS;

// Global variables
SoftwareSerial softy(11, 10);  // RX, TX
byte out_msg[MAX_PAYLOAD_SIZE];
byte in_msg[MAX_PAYLOAD_SIZE];
uint8_t bytes_received;
uint8_t led;
bool trig;
bool prev_trig;
bool transferring;
bool add_text;
bool add_image;
bool add_conway;
bool add_anim;
bool prev_text;
bool prev_image;
bool prev_conway;
bool prev_anim;

/***************************************************************
 * Main
 **************************************************************/

void setup() {
  
  // Setup debug serial
#if DEBUG
  Serial.begin(9600);
  Serial.println(F("NMMF '15 Station Test"));
  Serial.println();
#endif

  // Set up soft serial
  softy.begin(SOFT_BAUD_RATE);
  
  // Set up pins
  pinMode(TRIG_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  pinMode(TEXT_BTN, INPUT_PULLUP);
  pinMode(TEXT_LED, OUTPUT);
  digitalWrite(TEXT_LED, LOW);
  pinMode(IMAGE_BTN, INPUT_PULLUP);
  pinMode(IMAGE_LED, OUTPUT);
  digitalWrite(IMAGE_LED, LOW);
  pinMode(CONWAY_BTN, INPUT_PULLUP);
  pinMode(CONWAY_LED, OUTPUT);
  digitalWrite(CONWAY_LED, LOW);
  pinMode(ANIM_BTN, INPUT_PULLUP);
  pinMode(ANIM_LED, OUTPUT);
  digitalWrite(ANIM_LED, LOW);
  
  // Initialize globals
  trig = false;
  prev_trig = false;
  led = 0;
  transferring = false;
  add_text = false;
  add_image = false;
  add_conway = false;
  add_anim = false;
  prev_text = false;
  prev_image = false;
  prev_conway = false;
  prev_anim = false;
}

void loop() {
  
  bool btn_state;
  
  // See if settings buttons were pushed
  btn_state = digitalRead(TEXT_BTN);
  if ( !btn_state && prev_text ) {
    add_text = !add_text;
  }
  prev_text = btn_state;
  btn_state = digitalRead(IMAGE_BTN);
  if ( !btn_state && prev_image ) {
    add_image = !add_image;
  }
  prev_image = btn_state;
  btn_state = digitalRead(CONWAY_BTN);
  if ( !btn_state && prev_conway ) {
    add_conway = !add_conway;
  }
  prev_conway = btn_state;
  btn_state = digitalRead(ANIM_BTN);
  if ( !btn_state && prev_anim ) {
    add_anim = !add_anim;
  }
  prev_anim = btn_state;
  
  // Switch LEDs based on add states
  digitalWrite(TEXT_LED, add_text);
  digitalWrite(IMAGE_LED, add_image);
  digitalWrite(CONWAY_LED, add_conway);
  digitalWrite(ANIM_LED, add_anim);
  
  // See if button was pushed
  if ( digitalRead(TRIG_PIN) == 0 ) {
    trig = true;
  } else {
    trig = false;
  }
  if ( (trig == true) && (prev_trig == false) ) {
    
#if DEBUG
    Serial.println("Triggered! Waiting for Badger...");
#endif

    // Flush receiver
    while ( softy.available() > 0 ) {
      softy.read();
    }
    
    // Wait for message from Badger
    transferring = true;
    while ( transferring ) {
      led ^= 0x01;
      digitalWrite(LED_PIN, led);
      doTransfer();
      delay(500);
    }
    
    // Reset settings
    add_text = false;
    add_image = false;
    add_conway = false;
    add_anim = false;
  }
  prev_trig = trig;
}

/***************************************************************
 * Communication Functions
 **************************************************************/

// Look for a BREAK then transmit actions
void doTransfer() {
  
  uint8_t i;
  
  // Get a message
  memset(in_msg, 0, MAX_PAYLOAD_SIZE);
  bytes_received = receiveMessage(in_msg, RX_TIMEOUT);

  if ( bytes_received > 0 ) {
  
    // Print the message received
#if DEBUG
    Serial.print(F("Received: "));
    for ( i = 0; i < bytes_received; i++ ) {
      Serial.print(F("0x"));
      Serial.print(in_msg[i], HEX);
      Serial.print(F(" "));
    }
    Serial.println();
#endif

    // If it's a BREAK, we can send our messages
    if ( in_msg[0] == HEADER_BREAK ) {
    
      // Send clear all message
      out_msg[0] = HEADER_CLEAR;
#if DEBUG
      Serial.print(F("Sending: 0x"));
      Serial.println(out_msg[0], HEX);
#endif
      transmitMessage(out_msg, 1);
      
      // Construct array of what we want to add
      bool to_add[4] = {add_text, add_image, 
                        add_conway, add_anim};
    
      // Loop through things to add
      for ( uint8_t i = 0; i < 4; i++ ) {
        
        // Only add if we want to!
        if ( to_add[i] ) {
        
          // Flash LED, wait for ACK
          digitalWrite(LED_PIN, HIGH);
          memset(in_msg, 0, MAX_PAYLOAD_SIZE);
          bytes_received = receiveMessage(in_msg, RX_TIMEOUT);
  
          // See if anything was received. If not, exit loop.
          if ( bytes_received <= 0 ) {
#if DEBUG
            Serial.println("Timeout");
#endif
            break;
          }
        
          // If ACK, continue
          if ( in_msg[0] == HEADER_ACK ) {
#if DEBUG
            Serial.println("ACK");
#endif
            sendAction(i);
          } else {
            break;
          }
          digitalWrite(LED_PIN, LOW);
        }
      }
      digitalWrite(LED_PIN, LOW);
      transferring = false;
    }
  }
}

// Tansmits a user-generated action to the Badger
void sendAction(uint8_t idx) {
  
  uint8_t len = 0;
  
  switch ( idx ) {
    
    // Send text
    case 0: 
      len = strlen(test_str);
      if ( len > MAX_TEXT_LENGTH ) {
        len = MAX_TEXT_LENGTH;
      }
      memset(out_msg, 0, MAX_PAYLOAD_SIZE);
      out_msg[0] = HEADER_TEXT;
      out_msg[1] = len;
      memcpy(out_msg + 2, test_str, len);
      len = len + 2;
      break;
      
    // Send bitmap
    case 1:
      len = BITMAP_SIZE + 1;
      memset(out_msg, 0, MAX_PAYLOAD_SIZE);
      out_msg[0] = HEADER_BITMAP;
      memcpy(out_msg + 1, test_img, len);
      break;
      
    // Send Conway
    case 2:
      len = BITMAP_SIZE + 1;
      memset(out_msg, 0, MAX_PAYLOAD_SIZE);
      out_msg[0] = HEADER_CONWAY;
      memcpy(out_msg + 1, test_conway, len);
      break;
      
    // Send animation
    case 3:
      len = 2;
      memset(out_msg, 0, MAX_PAYLOAD_SIZE);
      out_msg[0] = HEADER_ANIMATION;
      out_msg[1] = test_anim;
      break;
      
    // Unknown case
    default:
      return;
  }
  
#if DEBUG
      Serial.print(F("Sending: "));
      for ( uint8_t i = 0; i < len; i++ ) {
        Serial.print(F("0x"));
        Serial.print(out_msg[i], HEX);
        Serial.print(F(" "));
      }
      Serial.println();
#endif
      transmitMessage(out_msg, len);
        
}

// Transmit message with BEGIN, SOF, Checksum, and EOF bytes
void transmitMessage(byte msg[], uint8_t len) {
  
  int i;
  byte cs;
  byte *out_buf;
  uint8_t buf_size;
  
  // If message is greater than max size, only xmit max bytes
  if ( len > MAX_PAYLOAD_SIZE ) {
    len = MAX_PAYLOAD_SIZE;
  }
  
  // Full buffer is message + BEGIN, SOF, CS, EOF bytes
  buf_size = len + 4;
  
  // Calculate checksum
  cs = createChecksum(msg, len);
  
  // Create the output buffer with BEGIN, SOF, CS, and EOF
  out_buf = (byte*)malloc(buf_size * sizeof(byte));
  out_buf[0] = SERIAL_BEGIN;
  out_buf[1] = SERIAL_SOF;
  memcpy(out_buf + 2, msg, len);
  out_buf[buf_size - 2] = cs;
  out_buf[buf_size - 1] = SERIAL_EOF;
  
  // Transmit buffer
  for ( i = 0; i < buf_size; i++ ) {
    softy.write(out_buf[i]);
  }
  
  // Free some memory
  free(out_buf);
}

// Receive a message (number of bytes received is returned)
// Timeout is in ms. 0 timeout means wait forever (blocking).
uint8_t receiveMessage(byte msg[], unsigned long timeout) {
  
  boolean valid;
  uint8_t buf_idx;
  unsigned long start_time;
  byte in_buf[IN_BUF_MAX];
  byte r;
  
  // Our receiver. Wait until we get a valid message or time out
  start_time = millis();
  valid = false;
  while ( !valid ) {
    
    // Wait until we see a Start of Frame (SOF) or time out
    memset(in_buf, 0, IN_BUF_MAX);
    while ( !softy.available() ) {
      if ( (millis() - start_time > timeout) && (timeout > 0) ) {
        return 0;
      }
    }
    r = softy.read();
    if ( r != SERIAL_SOF ) {
      continue;
    }
    
    // Read buffer
    delay(2);  // Magical delay to let the buffer fill up
    buf_idx = 0;
    while ( softy.available() > 0 ) {
      if ( buf_idx >= IN_BUF_MAX ) {
        buf_idx = IN_BUF_MAX - 1;
      }
      in_buf[buf_idx] = softy.read();
      buf_idx++;
      delay(2);  // Magical delay to let the buffer fill up
    }
    
    // End of Frame (EOF) check
    if ( in_buf[buf_idx - 1] != SERIAL_EOF ) {
      continue;
    }
    
    // Verify the checksum
    if ( verifyChecksum(in_buf, buf_idx - 1) ) {
      valid = 1;
    }
  }
  
  // Copy our message (don't copy checksum or EOF bytes)
  memcpy(msg, in_buf, buf_idx - 2);
  
  return buf_idx - 2;
}

// Calculate checksum (ignore Begin and SOF bytes)
byte createChecksum(byte buf[], uint8_t len) {
  
  uint8_t i;
  byte sum;
  
  // XOR all bytes in the message
  sum = 0;
  for ( i = 0; i < len; i++ ) {
    sum ^= buf[i];
  }
  
  return sum;
}

// Check the checksum
boolean verifyChecksum(byte buf[], uint8_t len) {
  
  uint8_t i;
  byte sum;
  
  // XOR all bytes in the message (including checksum byte)
  sum = 0;
  for ( i = 0; i < len; i++ ) {
    sum ^= buf[i];
  }
  
  // Sum should be 0 if the checksum is correct
  return (sum ? false : true);
}
