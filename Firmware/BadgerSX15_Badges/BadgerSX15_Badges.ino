/****************************************************************
BadgerSX15_Badges.ino
SX Create 2015 Badger Game
Shawn Hymel @ SparkFun Electronics
Nick Poole @ SparkFun Electronics
Kade Drobeck @ SparkFun Electronics
February 16, 2015
https://github.com/sparkfun/Interactive_Badges

Plays the #BadgerHack game. Take the badge to the various
stations located around SX Create to accumulate points. When you
reach the required number of stations, the badge unlocks and
displays a coupon code to be used at sparkfun.com.

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
#include <EEPROM.h>

// Constants
#define DEBUG             0
#define RX_TIMEOUT        200  // ms
#define LED_PIN           13
#define START_ANIMATIONS  4
#define NUM_ANIMATIONS    3
#define STATIONS_TO_WIN   4

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
#define ADDR_CODE_L      0x02
#define ADDR_CODE_H      0x03
#define ADDR_STATIONS_L  0x04
#define ADDR_STATIONS_H  0x05
#define ADDR_PROD_TEST   501

// Global variables
byte badge_id[] = {BADGE_HEADER, 0x00, 0x00};
SoftwareSerial softy(11, 10);  // RX, TX
static byte led_pins[] = {2, 3, 4, 5, 6, 7, 8, 9}; // Pins for LEDs
byte in_msg[MAX_PAYLOAD_SIZE];
uint8_t bytes_received;
uint16_t stations;
uint16_t r;
uint8_t animation;
uint8_t g_i;
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
int A7val;

/***************************************************************
 * Main
 **************************************************************/

void setup() {
  
  uint8_t i;
  
  // Perform production test
  while ( EEPROM.read(ADDR_PROD_TEST) != 0xAB ) {
    test_code();
  }
  
  // Blink a few times to show that we are alive
  pinMode(LED_PIN, OUTPUT);
  for ( i = 0; i < 3; i++ ) {
    digitalWrite(LED_PIN, HIGH);
    delay(250);
    digitalWrite(LED_PIN, LOW);
    delay(250);
  }
  pinMode(LED_PIN, INPUT);
  
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
  
  // Get our badge ID
  badge_id[1] = EEPROM.read(ADDR_ID_H);
  badge_id[2] = EEPROM.read(ADDR_ID_L);
  
  // Read which stations we've visited
  stations = readStations();
#if DEBUG
  Serial.print(F("Stations: "));
  Serial.println(stations, BIN);
#endif

  // If we have a win condition, scroll just the coupon code
  if ( (badge_id[1] != 0) || (badge_id[2] != 0) ) {
    if ( numStations() >= STATIONS_TO_WIN ) {
      for ( i = 0; i < 3; i++ ) {
        displayAnimation(3);
      }
    }
  }
  
  animation = START_ANIMATIONS;
}

void loop() {
  
  // Create an input buffer for our incoming message
  memset(in_msg, 0, MAX_PAYLOAD_SIZE);
  
  // Try sending and receiving a message a few times
  for ( g_i = 0; g_i < 5; g_i++ ) {
    softy.begin(600);
#if DEBUG
    Serial.print(F("Sending ID:"));
    for ( int i = 0; i < 3; i++ ) {
      Serial.print(" 0x");
      Serial.print(badge_id[i], HEX);
    }
    Serial.println();
#endif
    transmitMessage(badge_id, 3);
  
    // Get a message
    bytes_received = receiveMessage(in_msg, RX_TIMEOUT);
  
#if DEBUG
    // If we received a message, print it
    if ( bytes_received > 0 ) {
      Serial.print(F("WAIT: Bytes: "));
      Serial.print(bytes_received);
      Serial.print(F(" Received:"));
      for ( int i = 0; i < bytes_received; i++ ) {
        Serial.print(F(" 0x"));
        Serial.print(in_msg[i], HEX);
      }
      Serial.println();
    }
#endif

    // If we get a new badge ID, store the ID and code to EEPROM
    if ( (bytes_received == 5) && (in_msg[0] == ID_HEADER) ) {
      badge_id[1] = in_msg[1];
      badge_id[2] = in_msg[2];
      EEPROM.write(ADDR_ID_L, badge_id[2]);
      EEPROM.write(ADDR_ID_H, badge_id[1]);
      EEPROM.write(ADDR_CODE_L, in_msg[4]);
      EEPROM.write(ADDR_CODE_H, in_msg[3]);
#if DEBUG
      Serial.print(F("New ID: 0x"));
      Serial.print(EEPROM.read(ADDR_ID_H), HEX);
      Serial.print(F(" 0x"));
      Serial.println(EEPROM.read(ADDR_ID_L), HEX);
      Serial.print(F("New coupon code: "));
      Serial.print(EEPROM.read(ADDR_CODE_H), HEX);
      Serial.println(EEPROM.read(ADDR_CODE_L), HEX);
#endif
    }
  
    // If we get a badge station, save the ID as a flag
    if ( (bytes_received == 2) && (in_msg[0] == STATION_HEADER) ) {
      
      // If we have not seen the station before, record it
      r = 1 << in_msg[1];
      if ( !(stations & r) ) {
        stations = r | stations;
        writeStations();
      }
#if DEBUG
      Serial.print(F("Stations: "));
      Serial.println(stations, BIN);
#endif

      // If win, show coupon. Then show number of stations.
      if ( (badge_id[1] != 0) || (badge_id[2] != 0) ) {
        if ( numStations() >= STATIONS_TO_WIN ) {
          displayAnimation(2);
          for ( uint8_t j = 0; j < 3; j++ ) {
            displayAnimation(3);
          }
        }
      }
      displayAnimation(1);
    }
  }
  
  // Choose and perform an animation
  displayAnimation(animation);
  animation++;
  if ( animation >= START_ANIMATIONS + NUM_ANIMATIONS ) {
    animation = START_ANIMATIONS;
  }
}

/***************************************************************
 * Functions
 **************************************************************/

// Determine if we have visited enough stations
uint8_t numStations() {
  
  uint8_t num_stations = 0;
  uint8_t i;
  
  for ( i = 0; i < 16; i++ ) {
    num_stations += (stations >> i) & 0x0001;
  }

  return num_stations;
}

// Perform an animation based on the number given
void displayAnimation(uint8_t n) {
  switch( n ) {
    case 0:
      // Communicating
      break;
    case 1:
      // Scroll number of stations
      showNumStations();
      break;
    case 2:
      // Win animation (explosions + text)
      showExplosions();
      Plex.scrollText("You win!", 1);
      delay(5000);
      Plex.stopScrolling();
      break;
    case 3:
      // Scroll coupon code
      showCoupon();
      break;
    case 4:
      // SparkFun Logo
      Plex.drawBitmap(sparkfun_logo);
      Plex.display();
      delay(3000);
      Plex.clear();
      Plex.display();
      break;
    case 5:
      // SX Create scroll
      Plex.scrollText("SX Create", 1);
      delay(6000);
      Plex.stopScrolling();
      break;
    case 6:
      // Make it rain!!!
      makeItRain(50);
      delay(500);
      Plex.clear();
      Plex.display();
      break;
    default:
      break;
  }
}

// "Rain" pixels. Param wait = ms between frames
void makeItRain(uint8_t wait) {
  
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
    delay(wait);
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

// Calculate the number of stations visited and display it
void showNumStations() {
  
  char c_tens = 0;
  char c_ones = 0;
  uint8_t n_stations = numStations();
  char msg[] = {'V', 'i', 's', 'i', 't', 'e', 'd', ':', ' ',
                0, '\0', '\0'};
  
  // Create ASCII characters out of the digits
  c_tens = 0x30 + (n_stations / 10);
  c_ones = 0x30 + (n_stations % 10);
  
  // Don't display leading 0
  if ( c_tens == 0x30 ) {
    msg[9] = c_ones;
  } else {
    msg[9] = c_tens;
    msg[10] = c_ones;
  }
  
#if DEBUG
  Serial.print(F("Showing 'Visited: "));
  Serial.print(msg);
  Serial.println("'");
#endif

  // Scroll message
  Plex.scrollText(msg, 1);
  delay(6500);
  Plex.stopScrolling();
}

// Scroll the coupon code across the LED display
void showCoupon() {
  
  uint8_t val;
  char code[] = { 'S', 'X', '1', '5', 0, 0, 0, 0, '\0' };
  uint8_t i;
  
  // Read in coupon code
  val = EEPROM.read(ADDR_CODE_H);
  code[4] = (val >> 4) & 0x0F;
  code[5] = val & 0x0F;
  
  val = EEPROM.read(ADDR_CODE_L);
  code[6] = (val >> 4) & 0x0F;
  code[7] = val & 0x0F;
  
#if DEBUG
  Serial.print("Code read as: ");
  for ( i = 0; i < 8; i++ ) {
    Serial.print(code[i], HEX);
  }
  Serial.println();
#endif

  // Calculate ASCII version of each of the characters
  for ( i = 4; i < 8; i++ ) {
    if ( (0x0A <= code[i]) && (code[i] <= 0x0F) ) {
      code[i] = 0x41 + (code[i] - 0xA);
    } else if ( (0 <= code[i]) && (code[i] <= 9) ) {
      code[i] = 0x30 + code[i];
    } else {
      code[i] = 0x30;
    }
  }
  
#if DEBUG
  Serial.print("Printing code: ");
  for ( i = 0; i < 8; i++ ) {
    Serial.print(code[i]);
  }
  Serial.println();
#endif

  // Scroll message
  Plex.scrollText(code, 1);
  delay(6000);
  Plex.stopScrolling();
}

// Read the coupon code from EEPROM
uint16_t readCoupon() {
  
  uint16_t val;
  
  val = EEPROM.read(ADDR_CODE_L);
  val ^= (EEPROM.read(ADDR_CODE_H) << 8);
  
  return val;
}

// Read the stations the badge has visited from EEPROM
uint16_t readStations() {
  
  uint16_t val;
  
  val = EEPROM.read(ADDR_STATIONS_L);
  val ^= (EEPROM.read(ADDR_STATIONS_H) << 8);
  
  return val;
}

// Write the stations visited to EEPROM
void writeStations() {
  
  byte b;
  
#if DEBUG
  Serial.println(F("Writing stations to EEPROM"));
#endif

  b = stations & 0x00FF;
  EEPROM.write(ADDR_STATIONS_L, b);
  b = stations >> 8;
  EEPROM.write(ADDR_STATIONS_H, b);
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
    EEPROM.write(ADDR_PROD_TEST, 0xAB);
    
    // Erase other EEPROM
    for ( int i = 0; i < 10; i++ ) {
      EEPROM.write(i, 0);
    }
    
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

