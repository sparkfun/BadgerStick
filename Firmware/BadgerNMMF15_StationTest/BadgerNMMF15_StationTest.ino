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
#define DEBUG             1
#define RX_TIMEOUT        200  // ms
#define LED_PIN           13
#define TRIG_PIN          5

// Communications constants
#define SOFT_BAUD_RATE    1200
#define MAX_PAYLOAD_SIZE  21
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

// Example actions
const char test_str[] = "Oh my.";
const byte test_img[] = {0x00,0x24,0x24,0x00,0x42,0x3C,0x00};

// Global variables
SoftwareSerial softy(11, 10);  // RX, TX
byte out_msg[MAX_PAYLOAD_SIZE];
byte in_msg[MAX_PAYLOAD_SIZE];
uint8_t bytes_received;
uint8_t led;
bool trig;
bool prev_trig;
bool transferring;

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
  
  // Initialize globals
  trig = false;
  prev_trig = false;
  led = 0;
  transferring = false;
}

void loop() {
  
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
    
    // Wait for message from Badger
    transferring = true;
    while ( transferring ) {
      led ^= 0x01;
      digitalWrite(LED_PIN, led);
      doTransfer();
      delay(500);
    }
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
    
      for ( uint8_t i = 0; i < 2; i++ ) {
        digitalWrite(LED_PIN, HIGH);
        memset(in_msg, 0, MAX_PAYLOAD_SIZE);
        bytes_received = receiveMessage(in_msg, RX_TIMEOUT);

        // See if anything was received. If not, exit loop.
        if ( bytes_received > 0 ) {
      
        // Print the message received
#if DEBUG
        Serial.print(F("Received: "));
        for ( uint8_t i = 0; i < bytes_received; i++ ) {
          Serial.print(F("0x"));
          Serial.print(in_msg[i], HEX);
          Serial.print(F(" "));
        }
        Serial.println();
#endif
        } else {
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
      memset(out_msg, 0, MAX_PAYLOAD_SIZE);
      out_msg[0] = HEADER_TEXT;
      out_msg[1] = strlen(test_str);
      memcpy(out_msg + 2, test_str, strlen(test_str));
      len = strlen(test_str) + 2;
      break;
      
    // Send bitmap
    case 1:
      memset(out_msg, 0, MAX_PAYLOAD_SIZE);
      out_msg[0] = HEADER_BITMAP;
      memcpy(out_msg + 1, test_img, 7);
      len = 7;
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
