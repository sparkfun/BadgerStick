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
Written in Arduino 1.6.3
Tested with SparkFun BadgerStick (Interactive Badge)

This code is beerware; if you see me (or any other SparkFun 
employee) at the local, and you've found our code helpful, please
buy us a round!

Distributed as-is; no warranty is given.
****************************************************************/

#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <SparkFun_LED_8x7.h>
#include <Chaplex.h>

// Parameters
#define DEBUG             0
#define LED_PIN           13
#define MAX_ACTIONS       30    // Number of actions per loop
#define MAX_TEXT_LENGTH   20    // Number of characters (>7)
#define BITMAP_DISP_TIME  2000  // Time (ms) to pause on bitmap
#define FRAME_DISP_TIME   100   // Time (ms) to pause on frame
#define CONWAY_NUM_GENS   20    // Number of generations
#define CONWAY_DISP_TIME  100   // Time (ms) to pause generation
#define RAIN_DISP_TIME    50    // Time (ms) between frames
#define RX_DELAY          500   // Time to wait for buffer to fill
#define INITIAL_TIMEOUT   10    // Waiting for first data (ms)
#define TRANSFER_TIMEOUT  1000  // Waiting for more data (ms)

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

// Other constants
#define BITMAP_SIZE       7     // Number of bytes in bitmap
#define BITS_PER_BYTE     8
#define PROD_TEST_SUCCESS 0xAB  // Value expected in EEPROM

// EEPROM addresses
#define ADDR_NUM_ACTIONS    0    // Total number of actions
#define ADDR_LIST_START     1    // Starting addr of actions list
#define ADDR_TABLE_START    ADDR_LIST_START + MAX_ACTIONS
#define ADDR_PROD_TEST      1023

// Initial animation, bitmap, and text
const byte flame00[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04};
const byte flame01[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x04};
const byte flame02[] = {0x00, 0x00, 0x00, 0x00, 0x3C, 0x0C, 0x04};
const byte flame03[] = {0x00, 0x00, 0x00, 0x7C, 0x3C, 0x0C, 0x04};
const byte flame04[] = {0x00, 0x00, 0x74, 0x7C, 0x3C, 0x0C, 0x04};
const byte flame05[] = {0x00, 0x50, 0x74, 0x7C, 0x3C, 0x0C, 0x04};
const byte flame06[] = {0x10, 0x50, 0x74, 0x7C, 0x3C, 0x0C, 0x04};
const byte conway_start[] = {0x00, 0x00, 0x00, 0x00, 0x60,
                                                0xA0,0x20};
const char initial_text[] = "#BadgerHack";

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
int A7val;

// Global variables
SoftwareSerial softy(11, 10);  // RX, TX
byte out_msg[MAX_PAYLOAD_SIZE];
byte in_msg[MAX_PAYLOAD_SIZE];
uint8_t bytes_received;
static byte led_pins[] = {2, 3, 4, 5, 6, 7, 8, 9};
uint8_t action_cnt;
uint8_t prev_action;
uint16_t timeout;

/***************************************************************
 * Main
 **************************************************************/

// Run once
void setup() {
  
  uint8_t i;
  const byte *ptr;
  uint8_t animation;
  
  // Setup onboard LED for debugging
  pinMode(LED_PIN, OUTPUT);
  
  // Initialize serial comms
#if DEBUG
  Serial.begin(9600);
  Serial.println(F("NMMF '15 Badgers"));
  Serial.println();
  Serial.println(F("EEPROM ADDRESSES"));
  Serial.print(F("num_actions: "));
  Serial.println(ADDR_NUM_ACTIONS, DEC);
  Serial.print(F("Actions list: "));
  Serial.print(ADDR_LIST_START, DEC);
  Serial.print(F(" - "));
  Serial.println(ADDR_LIST_START + MAX_ACTIONS - 1, DEC);
  Serial.print(F("Actions table: "));
  Serial.print(ADDR_TABLE_START, DEC);
  Serial.print(F(" - "));
  Serial.println(ADDR_TABLE_START + (MAX_ACTIONS * 
                          MAX_TEXT_LENGTH) - 1, DEC);
  Serial.print(F("Production Test: "));
  Serial.println(ADDR_PROD_TEST, DEC);
  Serial.println();
#endif

  // If we have not passed production test, fill default actions
  if ( EEPROM.read(ADDR_PROD_TEST) != PROD_TEST_SUCCESS ) {
    clearActions();
    ptr = conway_start;
    addAction(HEADER_CONWAY, ptr, 0);
    ptr = flame00;
    addAction(HEADER_FRAME, ptr, 0);
    ptr = flame01;
    addAction(HEADER_FRAME, ptr, 0);
    ptr = flame02;
    addAction(HEADER_FRAME, ptr, 0);
    ptr = flame03;
    addAction(HEADER_FRAME, ptr, 0);
    ptr = flame04;
    addAction(HEADER_FRAME, ptr, 0);
    ptr = flame05;
    addAction(HEADER_FRAME, ptr, 0);
    for ( i = 0; i < 15; i++ ) {
      ptr = flame06;
      addAction(HEADER_FRAME, ptr, 0);
    }
    animation = ANIM_RAIN;
    addAction(HEADER_ANIMATION, &animation, 0);
    ptr = (const byte*)initial_text;
    addAction(HEADER_TEXT, ptr, strlen(initial_text));
  }

  // Production test code
  while ( EEPROM.read(ADDR_PROD_TEST) != PROD_TEST_SUCCESS ) {
    test_code();
  }
  
  // Initialize the software serial port
  softy.begin(SOFT_BAUD_RATE);
  
  // Seed the RNGesus
  randomSeed(analogRead(0));
  
  // Blink a few times to show that we are alive
  for ( i = 0; i < 3; i++ ) {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
  }
  
  // Initialize LED array
  Plex.init(led_pins);
  
  // Initialize action counter
  action_cnt = 0;
  
  // Initialize checking
  prev_action = 0;
}

// Superloop
void loop() {
  
  Serial.print("Prev: ");
  Serial.print(prev_action, DEC);
  Serial.print(" Current: ");
  Serial.println(EEPROM.read(ADDR_LIST_START + action_cnt), DEC);
  
  // Skip transfers if middle of animation
  if ( ((prev_action != HEADER_FRAME) ||
      (EEPROM.read(ADDR_LIST_START + action_cnt) != 
      HEADER_FRAME)) || action_cnt == 0 ) {
        
    // Clear LEDs
    Plex.clear();
    Plex.display();
  
    // Try sending the BREAK message
    out_msg[0] = HEADER_BREAK;
#if DEBUG
    Serial.print(F("Sending: 0x"));
    Serial.println(out_msg[0], HEX);
#endif
    transmitMessage(out_msg, 1);
    
    // Magic delay to let input buffer fill
    delay(RX_DELAY);
    
    // Get message, add action, send ACK until timeout
    timeout = INITIAL_TIMEOUT;
    while ( true ) {
      memset(in_msg, 0, MAX_PAYLOAD_SIZE);
      bytes_received = receiveMessage(in_msg, timeout);
    
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
        // We got a response! Make timeout longer
        timeout = TRANSFER_TIMEOUT;

      } else {
        break;
      }
      
      // Send ACK
      out_msg[0] = HEADER_ACK;
#if DEBUG
    Serial.print(F("Sending: 0x"));
    Serial.println(out_msg[0], HEX);
#endif
      transmitMessage(out_msg, 1);
      
      // Clear or add action as per the message
      interpretMessage();
    }
  }
  
  // Perform desired action and update counter
  if ( EEPROM.read(ADDR_NUM_ACTIONS) > 0 ) {
    performAction(action_cnt);
    prev_action = EEPROM.read(ADDR_LIST_START + action_cnt);
    action_cnt = (action_cnt + 1) % EEPROM.read(ADDR_NUM_ACTIONS);
  }
}

/***************************************************************
 * High Level Main Functions
 **************************************************************/
 
// Read the receive buffer and add the listed action
void interpretMessage() {
  
  const byte *ptr;
  char text[MAX_TEXT_LENGTH];
  uint8_t animation = 0;

  // Read the header
  switch ( in_msg[0] ) {
  
    // Clear the actions
    case HEADER_CLEAR:
#if DEBUG
      Serial.println(F("Clearing actions"));
#endif
      clearActions();
      break;
          
    // Add text
    case HEADER_TEXT:
#if DEBUG
      Serial.println(F("Adding text"));
#endif
      ptr = (const byte*)(in_msg + 2);
      addAction(HEADER_TEXT, ptr, in_msg[1]);
      break;
      
    // Add bitmap
    case HEADER_BITMAP:
#if DEBUG
      Serial.println(F("Adding bitmap"));
#endif
      ptr = (const byte *)(in_msg + 1);
      addAction(HEADER_BITMAP, ptr, 0);
      break;
      
    // Add frame
    case HEADER_FRAME:
#if DEBUG
      Serial.println(F("Adding frame"));
#endif
      ptr = (const byte *)(in_msg + 1);
      addAction(HEADER_FRAME, ptr, 0);
      break;
      
    // Add Conway's Game of Life
    case HEADER_CONWAY:
#if DEBUG
      Serial.println(F("Adding Conway"));
#endif
      ptr = (const byte *)(in_msg + 1);
      addAction(HEADER_CONWAY, ptr, 0);
      break;
      
      // Add math animation
    case HEADER_ANIMATION:
#if DEBUG
      Serial.print(F("Adding animation #"));
      Serial.println(in_msg[1]);
#endif
      animation = in_msg[1];
      addAction(HEADER_ANIMATION, &animation, 0);
      break;

    // Unknown header
    default:
      break;
  }
}

/***************************************************************
 * Communication Functions
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
      Serial.print("0x");
      Serial.print(in_buf[buf_idx], HEX);
      Serial.print(" ");
      buf_idx++;
      delay(2);  // Magical delay to let the buffer fill up
    }
    Serial.println();
    
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

/***************************************************************
 * High Level Draw Functions
 **************************************************************/

// Clear all actions from the table
void clearActions() {
  
  uint16_t i;
  
  // Clear actions list
  for(i = ADDR_LIST_START; i < ADDR_LIST_START + MAX_ACTIONS; i++){
    EEPROM.update(i, 0);
  }
  
  // Clear actions length and counter
  EEPROM.update(ADDR_NUM_ACTIONS, 0);
  action_cnt = 0;
}

// Add an action to the table
bool addAction(uint8_t action, const byte *buf, uint8_t len) {
  
  uint8_t i;
  
  // Get the current number of actions
  uint8_t num_actions = EEPROM.read(ADDR_NUM_ACTIONS);
  
  // If we are at max actions, don't add anything
  if ( num_actions + 1 > MAX_ACTIONS ) {
    return false;
  }
  
  // Predefine the offset into the actions table
  uint16_t offset = num_actions * MAX_TEXT_LENGTH;
  
  // Say which action we are adding
#if DEBUG
  Serial.print(F("Adding action: "));
  Serial.println(action);
#endif
                    
  // What action we add is based on the action header
  switch ( action ) {
    
    // Add text to the buffer (clear it first)
    case HEADER_TEXT:
      for ( i = 0; i < MAX_TEXT_LENGTH; i++ ) {
        EEPROM.update(ADDR_TABLE_START + offset + i, 0);
      }
      EEPROM.update(ADDR_LIST_START + num_actions, HEADER_TEXT);
      if ( len >= MAX_TEXT_LENGTH ) {
        len = MAX_TEXT_LENGTH;
      }
      for ( i = 0; i < len; i++ ) {
        EEPROM.update(ADDR_TABLE_START + offset + i, buf[i]);
      }
      break;
      
    // Add bitmap to the buffer
    case HEADER_BITMAP:
      EEPROM.update(ADDR_LIST_START + num_actions, HEADER_BITMAP);
      for ( i = 0; i < BITMAP_SIZE; i++ ) {
        EEPROM.update(ADDR_TABLE_START + offset + i, buf[i]);
      }
      break;
      
    // Add frame to the buffer
    case HEADER_FRAME:
      EEPROM.update(ADDR_LIST_START + num_actions, HEADER_FRAME);
      for ( i = 0; i < BITMAP_SIZE; i++ ) {
        EEPROM.update(ADDR_TABLE_START + offset + i, buf[i]);
      }
      break;
      
    // Add Conway's game to buffer
    case HEADER_CONWAY:
      EEPROM.update(ADDR_LIST_START + num_actions, HEADER_CONWAY);
      for ( i = 0; i < BITMAP_SIZE; i++ ) {
        EEPROM.update(ADDR_TABLE_START + offset + i, buf[i]);
      }
      break;
      
    // Add math animation to buffer
    case HEADER_ANIMATION:
      EEPROM.update(ADDR_LIST_START + num_actions, 
                                              HEADER_ANIMATION);
      EEPROM.update(ADDR_TABLE_START + offset + i, *buf);
      break;
      
    // Unknown case. Exit.
    default:
      return false;
  }
  
  // Increment number of actions
  EEPROM.update(ADDR_NUM_ACTIONS, num_actions + 1);
  
  return true;
}

// Perform the desired action based on the number provided
void performAction(uint8_t index) {
  
  uint8_t i;
  
  // Make sure index is not out of range
  if ( index >= EEPROM.read(ADDR_NUM_ACTIONS) ) {
    return;
  }
  
  // Predefine the offset into the actions table
  uint16_t offset = index * MAX_TEXT_LENGTH;
  
  // Get the desired action
  uint8_t action = EEPROM.read(ADDR_LIST_START + index);
  
   // Say which action we are about to do
#if DEBUG
  Serial.print(F("Performing action: "));
  Serial.println(action);
#endif

  // Do the thing we said we would do
  switch ( action ) {
    
    // Scroll text once
    case HEADER_TEXT:
      Plex.clear();
      Plex.display();
      char text_buf[MAX_TEXT_LENGTH + 1];
      memset(text_buf, 0, MAX_TEXT_LENGTH + 1);
      for ( i = 0; i < MAX_TEXT_LENGTH; i++ ) {
        text_buf[i] = EEPROM.read(ADDR_TABLE_START + offset + i);
      }
      Plex.scrollText(text_buf, 1, true);
      break;
      
    // Show bitmap for a set amount of time
    case HEADER_BITMAP:
      showBitmap(offset, BITMAP_DISP_TIME);
      break;
      
    // Show bitmap (frame) for a short amount of time
    case HEADER_FRAME:
      showBitmap(offset, FRAME_DISP_TIME);
      break;
      
    // Play Conway's game of live
    case HEADER_CONWAY:
      playConway(offset, CONWAY_NUM_GENS);
      break;
      
    // Show a math-based animation
    case HEADER_ANIMATION:
      switch ( EEPROM.read(ADDR_TABLE_START + offset) ) {
        case ANIM_RAIN:
          makeItRain();
          delay(500);
          break;
          
         case ANIM_EXPLOSIONS:
           showExplosions();
           break;
           
          default:
            return;
      }
      break;
    
    // Unknown index. End.
    default:
      return;
 }
}

/***************************************************************
 * Low Level Animation/Display Functions
 **************************************************************/

// Display a bitmap on the LEDs for a given amount of time
void showBitmap(uint16_t offset, unsigned long time) {
  
  byte bitmap_buf[NUM_LEDS];
  
  // Translate bytes to bit array
  for ( uint8_t y = 0; y < BITMAP_SIZE; y++ ) {
    for ( uint8_t i = 0; i < BITS_PER_BYTE; i++ ) {
      bitmap_buf[(y * BITS_PER_BYTE) + i] = 
          (EEPROM.read(ADDR_TABLE_START + offset + y) >> i) & 0x01;
    }
  }
  
  // Draw on LEDs
  Plex.drawBitmap(bitmap_buf);
  Plex.display();
  
  // Wait for the specified time (ms)
  delay(time);
}

// Play Conway's Game of Life
void playConway(uint16_t offset, uint16_t num_gens) {
  
  byte gol_1[NUM_LEDS];
  byte gol_2[NUM_LEDS];
  byte *gol_prev;
  byte *gol_new;
  bool gol_swap = true;
  uint8_t cell_state;
  uint8_t neighbors;
  uint16_t g;
  int8_t x;
  int8_t y;
  int8_t rx;
  int8_t ry;
  
  // Translate bytes to bit array
  for ( y = 0; y < BITMAP_SIZE; y++ ) {
    for ( x = 0; x < BITS_PER_BYTE; x++ ) {
      gol_1[(y * BITS_PER_BYTE) + x] = 
          (EEPROM.read(ADDR_TABLE_START + offset + y) >> x) & 0x01;
    }
  }
  
    // Show initial state
  Plex.drawBitmap(gol_1);
  Plex.display();
  delay(CONWAY_DISP_TIME);
  
  // Play Conway's game for a number of generations
  for ( g = 0; g < num_gens; g++ ) {
    
    // Swap array pointers
    if ( gol_swap ) {
      gol_prev = gol_1;
      gol_new = gol_2;
      gol_swap = false;
    } else {
      gol_prev = gol_2;
      gol_new = gol_1;
      gol_swap = true;
    }
    
    // Go through each cell
    for ( y = 0; y < COL_SIZE; y++ ) {
      for ( x = 0; x < ROW_SIZE; x++ ) {
        
        // Determine if cell is alive or dead
        cell_state = gol_prev[(y * ROW_SIZE) + x];
        
        // Count number of alive neighbors (8 directions)
        neighbors = 0;
        for ( ry = -1; ry <= 1; ry++ ) {
          
          // Don't want to go past array bounds
          if ( ((y + ry) < 0) || ((y + ry) >= COL_SIZE) ) {
            continue;
          }
          
          for ( rx = -1; rx <= 1; rx++ ) {
            
            // Don't want to go past array bounds
            if ( ((x + rx) < 0) || ((x + rx) >= ROW_SIZE) ) {
              continue;
            }
            
            // Don't want to count self
            if ( (rx == 0) && (ry == 0) ) {
              continue;
            }
            
            // Only count alive neighbors
            if (gol_prev[((y + ry) * ROW_SIZE) + (x + rx)] == 1) {
              neighbors++;
            }
          }
        }
        
        // See if the cell lives or dies in next generation
        if ( ((cell_state == 1) && (neighbors == 2)) ||
                                         (neighbors == 3) ) {
          gol_new[(y * ROW_SIZE) + x] = 1;
        } else {
          gol_new[(y * ROW_SIZE) + x] = 0;
        }
      }
    }
    
    // Print next generation to Serial
#if 0
    for ( uint8_t j = 0; j < COL_SIZE; j++ ) {
      for ( uint8_t i = 0; i < ROW_SIZE; i++ ) {
        Serial.print(gol_new[(j * ROW_SIZE) + i]);
      }
      Serial.println();
    }
    Serial.println();
#endif
    
    // Show generation
    Plex.drawBitmap(gol_new);
    Plex.display();
    delay(CONWAY_DISP_TIME);
  }
}

// "Rain" pixels. Param wait = ms between frames
void makeItRain() {
  
  int r;
  int pix;
  uint8_t fill_count = 0;
  byte screen_filled = 0;
  byte frame_buf[] = { 0,0,0,0,0,0,0,0,
                      0,0,0,0,0,0,0,0,
                      0,0,0,0,0,0,0,0,
                      0,0,0,0,0,0,0,0,
                      0,0,0,0,0,0,0,0,
                      0,0,0,0,0,0,0,0,
                      0,0,0,0,0,0,0,0 };
                      
  // Continue to animate until the screen is filled with pixels
  while ( !screen_filled ) {
    
    // Find an open column
    do {
      r = random(0, 8);
    } while ( frame_buf[r] != 0 );
    
    // Start animating the pixel down that column
    frame_buf[r] = 1;
    Plex.drawBitmap(frame_buf);
    Plex.display();
    
    // Scan all pixels, find the last open pixel in that column
    for ( pix = 55; pix >= 0; pix-- ) {
      if ( frame_buf[pix] == 1 ) {
        if ( pix > 47 || frame_buf[pix + 8] == 1 ) {
          // Do nothing
        } else {
          frame_buf[pix] = 0;
          frame_buf[pix + 8] = 1;
        }
      }
    }
    
    // Check to see fi the screen is filled
    fill_count = 0;
    for ( pix = 55; pix >= 0; pix-- ) {
      fill_count += frame_buf[pix];
    }
    if ( fill_count >= 56 ) {
      screen_filled = 1;
    }
    
    // Draw the buffer
    Plex.clear();
    Plex.drawBitmap(frame_buf);
    Plex.display();
    delay(RAIN_DISP_TIME);
  }
}

// Draw some explosions on the LED array
void showExplosions() {
  
  uint8_t i;
  uint8_t x;
  uint8_t y;
  uint8_t radius;
  
  // Draw 2 circles expanding outward starting from a random spot
  for ( i = 0; i < 10; i++ ) {
    x = random(0, 8);
    y = random(0, 7);
    for ( radius = 0; radius < 12; radius++ ) {
      Plex.clear();
      Plex.circle(x, y, radius);
      Plex.circle(x, y, radius + 1);
      Plex.display();
      delay(30);
    }
  }
  Plex.clear();
  Plex.display();
}


/***************************************************************
 * Production Tests
 **************************************************************/
 
 ///////////////////////////////////////////////////////////////////////////////////////////////
void A0_control()
{
 digitalWrite(14, LOW);//A4
 digitalWrite(15, HIGH);//A5
}
///////////////////////////////////////////////////////////////////////////////////////////////
void A1_control()
{
 digitalWrite(14, HIGH);//A4
 digitalWrite(15, LOW); //A5
}
///////////////////////////////////////////////////////////////////////////////////////////////
void A0_test()
{
  A6val = analogRead(A6);
  A7val = analogRead(A7);
  for(int a=0; a<7; a++)
     {
      if(!(digitalRead(A0PinSet[a])))
        {
        }
      else A0_1error++;
     } 
  for(int b=0; b<6; b++)
     {  
      if(digitalRead(A1PinSet[b]))
        {
        }
      else A1_1error++;
     }
  if((A6val < 100) && (A7val > 1000))
    {
    }
  else pin_read1error++;
  if((A0_1error == 0) && (A1_1error == 0) && (pin_read1error == 0))
    {
     A0TEST = 1;
    }
  else A0TEST = 0;
}
///////////////////////////////////////////////////////////////////////////////////////////////
void A1_test()
{
  A6val = analogRead(A6);
  A7val = analogRead(A7);
  for(int c=0; c<6; c++)
     {
      if(!(digitalRead(A1PinSet[c])))
        {
        }
      else A1_2error++;
     } 
  for(int d=0; d<7; d++)
     {  
      if(digitalRead(A0PinSet[d]))
        {
        }
      else A0_2error++;
     }
  if((A7val < 100) && (A6val > 1000))
    {
    }
  else pin_read2error++;
  if((A0_2error == 0) && (A1_2error == 0) && (pin_read2error == 0))
    {
     A1TEST = 1;
    }
  else A1TEST = 0;  
}
///////////////////////////////////////////////////////////////////////////////////////////////
void check_results()
{
 if((A0TEST == 1) && (A1TEST == 1))
   {
    pin_default();
    pinMode(STAT, OUTPUT);
    digitalWrite(STAT, HIGH);
    EEPROM.write(ADDR_PROD_TEST, PROD_TEST_SUCCESS);
    
    delay(2000);
   }      
}
///////////////////////////////////////////////////////////////////////////////////////////////
void pin_setup()
{
  pinMode(14, OUTPUT);
  pinMode(15, OUTPUT);
  for(int i=0; i<7;i++)
    {
     pinMode(A0PinSet[i], INPUT);
    }
  for(int h=0; h<6; h++)
    {
     pinMode(A1PinSet[h], INPUT);
    }
  pinMode(A6, INPUT);
  pinMode(A7, INPUT); 
}
///////////////////////////////////////////////////////////////////////////////////////////////
void test_code()
{
    A0_1error = 0;
    A0_2error = 0;
    A1_1error = 0;
    A1_2error = 0;
    pin_read1error = 0;
    pin_read2error = 0; 
    pin_setup(); 
    A0_control();
    delay(100);
    A0_test();
    delay(100);
    A1_control();
    delay(100);
    A1_test();
    delay(100);
    check_results();
    delay(100);
}
///////////////////////////////////////////////////////////////////////////////////////////////
void pin_default()
{
 for(int k=0; k<15; k++)
    {
     pinMode(ALLPINS[k], INPUT);//set everything back to input
     digitalWrite(ALLPINS[k], HIGH);//activate internal pullups
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////END

