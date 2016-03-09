#include <FrequencyTimer2.h>

// Parameters
#define DEBUG             1
#define CLEAR_FIRST       1
#define RX_TIMEOUT        1000  // ms
#define MAX_TEXT_LENGTH   20    // Number of characters (>7)

// Other constants
#define BITMAP_SIZE       7     // Number of bytes in bitmap
#define BITS_PER_BYTE     8

// Communications constants
#define UART2_BAUD_RATE    1200
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

// Global variables
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
bool add_rain;
bool add_expl;
bool prev_text;
bool prev_image;
bool prev_conway;
bool prev_anim;
char send_anim;

int swtgnd[8] = {28, 29, 30, 31, 32, 33, 34, 35}; 
int swtdec[7] = {46, 45, 44, A4, A1, A5, A2};
int ledgnd[8] = {10, 11, 9, 12, 8, 13, 37, 36}; 
int ledvcc[7] = {53, 52, 51, 50, 49, 48, 47}; 

/*UNIT B 
int swtgnd[8] = {35, 34, 33, 32, 31, 30, 29, 28};
int swtdec[7] = {46, 45, 44, A4, A1, A5, A2};
int ledgnd[8] = {36, 37, 13, 8, 12, 9, 11, 10};
int ledvcc[7] = {53, 52, 51, 50, 49, 48, 47};
*/

bool frame[7][8] =
{
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0}
};

bool blankFrame[7][8] =
{
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0}
};

bool smile[7][8] =
{
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 1, 1, 1, 1, 0, 0},
  {0, 0, 1, 0, 0, 1, 0, 0},
  {0, 0, 1, 1, 1, 1, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0}
};

bool picSwt = 0;
bool lastPicSwt = 0;
bool conSwt = 0;
bool txtSwt = 0;
bool xplSwt = 0;
bool ranSwt = 0;

char message[17] = "                ";
//char prevMessage[17] = "                ";

byte send_img[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00};

void setup() {

  for (int i = 0; i < 8; i++) {
    pinMode(ledgnd[i], OUTPUT);
    digitalWrite(ledgnd[i], HIGH);
  }

  for (int i = 0; i < 7; i++) {
    pinMode(ledvcc[i], OUTPUT);
    digitalWrite(ledvcc[i], LOW);
  }

  for (int i = 0; i < 8; i++) {
    pinMode(swtgnd[i], OUTPUT);
    digitalWrite(swtgnd[i], LOW);
  }

  for (int i = 0; i < 7; i++) {
    pinMode(swtdec[i], INPUT);
    digitalWrite(swtdec[i], HIGH);
  }

  FrequencyTimer2::disable();
  FrequencyTimer2::setPeriod(50000);
  FrequencyTimer2::setOnOverflow(scanOut);

  pinMode(A13, OUTPUT);
  digitalWrite(A13, LOW);

  pinMode(A14, INPUT);
  digitalWrite(A14, HIGH);

  Serial.begin(9600);
  Serial1.begin(57600);
  Serial2.begin(UART2_BAUD_RATE);

  #if DEBUG 
  Serial.println("I AM STATION");
  #endif 

  // Initialize globals
  trig = false;
  prev_trig = false;
  led = 0;
  transferring = false;
  add_text = false;
  add_image = false;
  add_conway = false;
  add_rain = false;
  add_expl = false;
  prev_text = false;
  prev_image = false;
  prev_conway = false;
  prev_anim = false;

  pinMode(20, OUTPUT);
  digitalWrite(20, LOW);

  pinMode(21, OUTPUT);
  digitalWrite(21, LOW);

}

void loop() {

  picSwt = digitalRead(A8);
  if(picSwt!=lastPicSwt){

    if(picSwt==0){clearFrame();}
    if(picSwt==1){newFrame();}

    lastPicSwt = picSwt;
  }
  
  if(picSwt==1){scanIn();}

  if(digitalRead(A14)==LOW){
  checkSwitches();
  checkText();
  formatImage();
  FrequencyTimer2::setOnOverflow(0);
  digitalWrite(21, HIGH);

while ( Serial2.available() > 0 ) {
      Serial2.read();
    }
  
  transferring = true;
    while ( transferring ) {
      doTransfer();
      delay(500);
    }
  
  
  FrequencyTimer2::setOnOverflow(scanOut);
  digitalWrite(21, LOW);
  }
  

}

void scanOut() {

  for (int x = 0; x < 8; x++) {

    for (int y = 0; y < 7; y++) {

      digitalWrite(ledvcc[y], frame[y][x]);

    }

    digitalWrite(ledgnd[x], 0);

    delay(1);

    digitalWrite(ledgnd[x], 1);

  }

}

void scanIn() {

  for (int x = 8; x >= 0; x--) {

      pinMode(swtgnd[x], OUTPUT); // Ground
      digitalWrite(swtgnd[x], LOW);

  for (int y = 0; y < 7; y++) {

      if (digitalRead(swtdec[y]) == 0) {

        frame[y][x] = !frame[y][x];
        delay(200);

      }

    }

    pinMode(swtgnd[x], INPUT); // Hi-Z
    digitalWrite(swtgnd[x], LOW);

  }
  
}

void checkSwitches(){

picSwt = digitalRead(A8);
conSwt = digitalRead(A9);
txtSwt = digitalRead(A10);
xplSwt = digitalRead(A11);
ranSwt = digitalRead(A12);

if(xplSwt==1){add_expl=true;}
else{add_expl=false;}
if(ranSwt==1){add_rain=true;}
else{add_rain=false;}
if(picSwt==1){add_image=true;}
else{add_image=false;}
if(txtSwt==1){add_text=true;}
else{add_text=false;}
if(conSwt==1){add_conway=true;}
else{add_conway=false;}

delay(500);

}

void checkText(){

    FrequencyTimer2::setOnOverflow(0);

int i = 0;

while(Serial1.available()){Serial1.read();}


for(int f = 0; f<10; f++){
i=0;
Serial1.write(200);
delay(10);
while(Serial1.available() && i<17){
message[i] = Serial1.read();
i++;}
delay(10);}

while(Serial1.available()){Serial1.read();}

Serial.println("Text Message:");
Serial.println("-------------");
Serial.println(message);
Serial.println();

//printFrame();

for ( i = 16; i >= 0; i-- ) {
  if ( message[i] == 0x20 || message[i] == 0x00 ) {
    message[i] = 0;
  } else {
    break;
  }
}

#if 1
  for ( i = 0; i < 17; i++ ) {
    Serial.print("0x");
    Serial.print(message[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
#endif

    FrequencyTimer2::setOnOverflow(scanOut);
  
}

void printFrame() {

  Serial.println("Matrix Image");
  Serial.println("------------");

  for(int x = 0; x < 7; x++){

    for(int y = 0; y < 8; y++){

      if(frame[x][y]==0){Serial.print("  ");}
      else{Serial.print("XX");}

}

  Serial.println();

}

Serial.println();

}

void clearFrame() {

  for (int x = 0; x < 8; x++) {

  for (int y = 0; y < 7; y++) {

        frame[y][x] = blankFrame[y][x];

      }
    }
  }
  

void newFrame() {

  for (int x = 0; x < 8; x++) {

  for (int y = 0; y < 7; y++) {

        frame[y][x] = smile[y][x];

      }
    }
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
      if ( CLEAR_FIRST == 1 ) {
        out_msg[0] = HEADER_CLEAR;
      } else {
        out_msg[0] = HEADER_BREAK;
      }
#if DEBUG
      Serial.print(F("Sending: 0x"));
      Serial.println(out_msg[0], HEX);
#endif
      transmitMessage(out_msg, 1);
      
      // Construct array of what we want to add
      bool to_add[5] = {add_text, add_image, 
                        add_conway, add_expl, add_rain};

#if DEBUG

      for(int meh=0; meh<5; meh++){
      Serial.println(to_add[meh], BIN);
      }

#endif
    
      // Loop through things to add
      for ( uint8_t i = 0; i < 5; i++ ) {
        
        // Only add if we want to!
        if ( to_add[i] ) {
        
          // Flash LED, wait for ACK
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
        }
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
      len = strlen(message);
      if ( len > MAX_TEXT_LENGTH ) {
        len = MAX_TEXT_LENGTH;
      }
      memset(out_msg, 0, MAX_PAYLOAD_SIZE);
      out_msg[0] = HEADER_TEXT;
      out_msg[1] = len;
      memcpy(out_msg + 2, message, len);
      len = len + 2;
      break;
      
    // Send bitmap
    case 1:
      len = BITMAP_SIZE + 1;
      memset(out_msg, 0, MAX_PAYLOAD_SIZE);
      out_msg[0] = HEADER_BITMAP;
      memcpy(out_msg + 1, send_img, len);
      break;
      
    // Send Conway
    case 2:
      len = BITMAP_SIZE + 1;
      memset(out_msg, 0, MAX_PAYLOAD_SIZE);
      out_msg[0] = HEADER_CONWAY;
      memcpy(out_msg + 1, send_img, len);
      break;
      
    // Send rain
    case 3:
      len = 2;
      memset(out_msg, 0, MAX_PAYLOAD_SIZE);
      out_msg[0] = HEADER_ANIMATION;
      out_msg[1] = ANIM_RAIN;
      break;

    // Send rain
    case 4:
      len = 2;
      memset(out_msg, 0, MAX_PAYLOAD_SIZE);
      out_msg[0] = HEADER_ANIMATION;
      out_msg[1] = ANIM_EXPLOSIONS;
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
    Serial2.write(out_buf[i]);
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
    while ( !Serial2.available() ) {
      if ( (millis() - start_time > timeout) && (timeout > 0) ) {
        return 0;
      }
    }
    r = Serial2.read();
    Serial.println(r, HEX);
    if ( r != SERIAL_SOF ) {
      Serial.println("skipping");
      continue;
    }
    
    // Read buffer
    delay(100);  // Magical delay to let the buffer fill up
    buf_idx = 0;
    while ( Serial2.available() > 0 ) {
      if ( buf_idx >= IN_BUF_MAX ) {
        buf_idx = IN_BUF_MAX - 1;
      }
      in_buf[buf_idx] = Serial2.read();
#if 0
      Serial.print("Msg read:");
      Serial.println(in_buf[buf_idx], HEX);
#endif
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

void formatImage(){

memset(send_img, 0, 7);

  for (int y = 0; y < 7; y++) {

    for (int x = 0; x < 8; x++) {
      send_img[y] |= (frame[y][x] & 0x1) << x;}

    }

}

