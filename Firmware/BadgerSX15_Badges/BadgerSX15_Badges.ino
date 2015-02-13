#include <SoftwareSerial.h>
#include <SparkFun_LED_8x7.h>
#include <Chaplex.h>
#include <EEPROM.h>

// Constants
#define DEBUG             1
#define RX_TIMEOUT        200  // ms

// Communications constants
#define MAX_PAYLOAD_SIZE  8
#define IN_BUF_MAX        MAX_PAYLOAD_SIZE + 2
#define SERIAL_BEGIN      0x55
#define SERIAL_SOF        0xD5
#define SERIAL_EOF        0xAA
#define BADGE_HEADER      0xBA
#define WAR_HEADER        0xFC
#define ID_HEADER         0x1D
#define STATION_HEADER    0x57
#define ACK               0xAC

// EEPROM addresses
#define ADDR_ID_L        0x00
#define ADDR_ID_H        0x01
#define ADDR_STATIONS    0x02

// Global variables
byte badge_id[] = {BADGE_HEADER, 0x00, 0x00};
SoftwareSerial softy(11, 10);  // RX, TX
static byte led_pins[] = {2, 3, 4, 5, 6, 7, 8, 9}; // Pins for LEDs
byte stations;
byte r;

/***************************************************************
 * Main
 **************************************************************/

void setup() {
  
  // Initialize serial comms
#if DEBUG
  Serial.begin(9600);
#endif
  
  // Initialize and clear LED array
  Plex.init(led_pins);
  Plex.clear();
  Plex.display();
  
  // Seed the RNGesus
  randomSeed(analogRead(0));
  
  // Get our badge ID
  badge_id[1] = EEPROM.read(ADDR_ID_H);
  badge_id[2] = EEPROM.read(ADDR_ID_L);
  
  // Read which stations we've visited
  stations = EEPROM.read(ADDR_STATIONS);
}

void loop() {
  
  // Create an input buffer for our incoming message
  byte in_msg[MAX_PAYLOAD_SIZE];
  uint8_t bytes_received;
  memset(in_msg, 0, MAX_PAYLOAD_SIZE);
  
  // Send message
  softy.begin(600);
#if DEBUG
  Serial.print("Sending ID:");
  for ( int i = 0; i < 3; i++ ) {
    Serial.print(" 0x");
    Serial.print(badge_id[i], HEX);
  }
  Serial.println();
#endif
  transmitMessage(badge_id, 3);
  
  // Get a message
  bytes_received = receiveMessage(in_msg, RX_TIMEOUT);
  
  // If we received a message, print it
#if DEBUG
  if ( bytes_received > 0 ) {
    Serial.print("WAIT: Bytes: ");
    Serial.print(bytes_received);
    Serial.print(" Received:");
    for ( int i = 0; i < bytes_received; i++ ) {
      Serial.print(" 0x");
      Serial.print(in_msg[i], HEX);
    }
    Serial.println();
  }
#endif

  // If we get a new badge ID, store it to EEPROM
  if ( (bytes_received == 3) && (in_msg[0] == ID_HEADER) ) {
    badge_id[1] = in_msg[1];
    badge_id[2] = in_msg[2];
    EEPROM.write(ADDR_ID_L, badge_id[2]);
    EEPROM.write(ADDR_ID_H, badge_id[1]);
#if DEBUG
    Serial.print("New ID: 0x");
    Serial.print(EEPROM.read(ADDR_ID_H), HEX);
    Serial.print(" 0x");
    Serial.println(EEPROM.read(ADDR_ID_L), HEX);
#endif
  }
  
  // If we get a badge station, save the ID as a flag
  if ( (bytes_received == 2) && (in_msg[0] == STATION_HEADER) ) {
    r = 1 << in_msg[1];
    if ( !(stations & r) ) {
      stations = r | stations;
      EEPROM.write(ADDR_STATIONS, stations);
#if DEBUG
      Serial.println("Writing stations to EEPROM");
#endif
    }
#if DEBUG
    Serial.print("Stations: ");
    Serial.println(stations, BIN);
#endif
  }
}

/***************************************************************
 * Functions
 **************************************************************/

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
