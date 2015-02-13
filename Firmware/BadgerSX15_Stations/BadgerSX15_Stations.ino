#include <SoftwareSerial.h>
#include <EEPROM.h>

// Communications constants
#define MAX_PAYLOAD_SIZE  8
#define IN_BUF_MAX        MAX_PAYLOAD_SIZE + 2
#define SERIAL_BEGIN      0x55
#define SERIAL_SOF        0xD5
#define SERIAL_EOF        0xAA
#define ID_ADDRESS_L      0
#define ID_ADDRESS_H      1

// Constants
#define RX_TIMEOUT        200  // ms

// Global variables
SoftwareSerial softy(11, 10);

/***************************************************************
 * Main
 **************************************************************/

void setup() {
  
  // Initialize serial comms
  Serial.begin(9600);
  
}

void loop() {
  
  // Create a message and length (not more than 8!)
  byte msg[] = {0xBA, 0x00, 0x01};
  uint8_t len = 3;
  
  // Create an input buffer for our incoming message
  byte in_msg[MAX_PAYLOAD_SIZE];
  uint8_t bytes_received;
  memset(in_msg, 0, MAX_PAYLOAD_SIZE);
  word badge_id;
  
  // Get a message
  softy.begin(600);
  bytes_received = receiveMessage(in_msg, RX_TIMEOUT);
  Serial.print("Bytes: ");
  Serial.println(bytes_received);
  softy.end();
  
  // If we received a message, test it
  
  if ( bytes_received > 0 ) {
    
    Serial.println( "Something's Talking..." );
    badge_id = extract_ID(in_msg, bytes_received);
       
     Serial.print( "It's badge #" );
     Serial.print( badge_id );
     Serial.println( "!" );
     
    Serial.println( "Sending Station Identifier" );
    softy.begin(600);
    byte msg[] = { 0x57, 0x01 }; 
    transmitMessage( msg, 2 );
    softy.end();
      
    }
  
  Serial.println();
 
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
    
    // Time out if we have been waiting too long for a message
    if ( (millis() - start_time > timeout) && (timeout > 0) ) {
      return 0;
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

word extract_ID (byte msg[], uint8_t len) {
  
  uint8_t id_ptr; // pointer to badge id low byte
  
  for ( uint8_t i = 0; i < len; i++ ) {
    
    if ( msg[i] == 0xBA ) { id_ptr = i + 1; } 
    
  }
  
  word id = word ( msg[id_ptr], msg[id_ptr+1] );
  
  return id;
  
}


