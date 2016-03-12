#include <SoftwareSerial.h>

#define DEBUG   0

// Constants
const uint16_t BUF_LEN = 16;

SoftwareSerial master(5,6);

// Globals
int row1 = 0;
int row2 = 0;
int row3 = 0;
int row4 = 0;

bool v1_above_thresh = false;
bool v2_above_thresh = false;
bool v3_above_thresh = false;
bool v4_above_thresh = false;

char message[BUF_LEN + 1];

int pos = 0;

int charID = 0; 

bool sentMsg = 0;

void setup() {
  
  Serial.begin (9600);
  master.begin (57600);

  Serial.write(0xFE);
  Serial.print("r");

  //Serial.write(0xFE);
  //Serial.write(124);
  //Serial.write(157);
    

  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH);

  pinMode(9, OUTPUT);
  digitalWrite(9, 1);

  pinMode(8, OUTPUT);
  digitalWrite(8, 1);

  message[0] = '_';
  for (int i=1; i<16; i++ ){
    message[i]= ' ';
  }
  message[BUF_LEN] = 0;

  delay(100);
  displayBuffer();

}

void loop(){ 

  delay(150); //just here to slow down the output, and show it will work  even during a delay

  readKeys();

  //MASTER COMMS
  if(master.available()){

    if(master.read()==200){

      while(master.available()){
        master.read();}

    }

      for(int i = 0; i<17; i++){
        if ( message[i] == '_' ) {
          master.print(' ');
        } else {
          master.print(message[i]);
        }

        if ( i >= 16 ) {
          sentMsg = 1; 
          delay(2000);
        }
   
      }

      
      
  }

  if(sentMsg){
      message[0] = '_';
      for(int i = 1; i<17; i++){
        message[i] = ' ';
      }
      pos = 0;
      sentMsg = 0;
      displayBuffer();
#if DEBUG
      Serial.println("Buffer sent and display cleared");
#endif
  }



}

// Add a character to the message buffer
void addChar(char c){

  // Append character and increment pointer
  message[pos] = c;
  pos++;
  if ( pos >= BUF_LEN ) {
    pos = BUF_LEN - 1;
  }

  // Add cursor
  if ( (pos < BUF_LEN) && (message[pos] == ' ') ) {
    message[pos] = '_';
  }

  // Update display
  displayBuffer();
}

// Remove last character from message buffer
void backspace() {

  if ( pos <= 0 ) {
    pos = 0;
  } else if ( pos >= BUF_LEN - 1 ) {
    pos = BUF_LEN - 1;
    if ( message[pos] == '_' ) {
      message[pos] = ' ';
      pos--;
      message[pos] = '_';
    } else {
      message[pos] = '_';
    }
  } else {
    message[pos] = ' ';
    pos--;
    message[pos] = '_';
  }

  // Update display
  displayBuffer();
}

// Clear display
void cls() {
  Serial.write(0xFE);
  Serial.write(128);
  Serial.write("                ");
  Serial.write(0xFE);
  Serial.write(128);
}

// Send buffer to LCD
void displayBuffer() {
  cls();
  for (int i=0; i<16; i++) {
    Serial.print(message[i]);
  }
}

// Read keypad
void readKeys(){

  char current = ' ';

  // Read touch pot voltages
  int v1 = analogRead(0);
  int v2 = analogRead(1);
  int v3 = analogRead(2);
  int v4 = analogRead(3);

#if DEBUG
  Serial.print("v: ");
  Serial.print(v1);
  Serial.print(" ");
  Serial.print(v2);
  Serial.print(" ");
  Serial.print(v3);
  Serial.print(" ");
  Serial.print(v4);
  Serial.print(" ");
#endif

  // Line 4 (on key down)
  if ( (v4 > 40) && (v4 <= 860) && (!v4_above_thresh) ) {
#if DEBUG
    Serial.print("Line 4: ");
    Serial.print(v4);
    Serial.print(" ");
#endif
    v4_above_thresh = true;
    if ( (v4 <= 860) && (v4 > 795) ) {
      current = '.';
    } else if ( (v4 <= 795) && (v4 > 715) ) {
      current = '!';
    } else if ( (v4 <= 715) && (v4 > 645) ) {
      current = '@';
    } else if ( (v4 <= 645) && (v4 > 575) ) {
      current = '#';
    } else if ( (v4 <= 575) && (v4 > 510) ) {
      current = ':';
    } else if ( (v4 <= 510) && (v4 > 440) ) {
      current = '/';
    } else if ( (v4 <= 440) && (v4 > 375) ) {
      current = '(';
    } else if ( (v4 <= 375) && (v4 > 305) ) {
      current = ')';
    } else if ( (v4 <= 305) && (v4 > 40) ) {
      current = ' ';
    }

    // Add the character to the buffer
    addChar(current);
    
#if 0
    Serial.print(" ");
    Serial.print(pos);
    Serial.print(" ");
    Serial.print(message);
#endif

    // End early
    return;

  } else if (v4 < 20) {
    v4_above_thresh = false;
  }

  // Line 3 (on key down)
  if ( (v3 > 40) && (!v3_above_thresh) ) {
#if DEBUG
    Serial.print("Line 3: ");
    Serial.print(v3);
    Serial.print(" ");
#endif
    v3_above_thresh = true;
    if ( (v3 <= 930) && (v3 > 860) ) {
      current = '1';
    } else if ( (v3 <= 860) && (v3 > 795) ) {
      current = '2';
    } else if ( (v3 <= 795) && (v3 > 715) ) {
      current = '3';
    } else if ( (v3 <= 715) && (v3 > 645) ) {
      current = '4';
    } else if ( (v3 <= 645) && (v3 > 575) ) {
      current = '5';
    } else if ( (v3 <= 575) && (v3 > 510) ) {
      current = '6';
    } else if ( (v3 <= 510) && (v3 > 440) ) {
      current = '7';
    } else if ( (v3 <= 440) && (v3 > 375) ) {
      current = '8';
    } else if ( (v3 <= 375) && (v3 > 305) ) {
      current = '9';
    } else if ( (v3 <= 305) && (v3 > 235) ) {
      current = '0';
    } else if ( (v3 <= 235) && (v3 > 40) ) {
      backspace();
      return;
    }

    // Add the character to the buffer
    addChar(current);
    
#if 0
    Serial.print(" ");
    Serial.print(pos);
    Serial.print(" ");
    Serial.print(message);
#endif

    // End early
    return;

  } else if (v3 < 10) {
    v3_above_thresh = false;
  }

  // Line 2 (on key down)
  if ( (v2 > 40) && (!v2_above_thresh) ) {
#if DEBUG
    Serial.print("Line 2: ");
    Serial.print(v2);
    Serial.print(" ");
#endif
    v2_above_thresh = true;
    if ( (v2 <= 930) && (v2 > 860) ) {
      current = 'N';
    } else if ( (v2 <= 860) && (v2 > 795) ) {
      current = 'O';
    } else if ( (v2 <= 795) && (v2 > 715) ) {
      current = 'P';
    } else if ( (v2 <= 715) && (v2 > 645) ) {
      current = 'Q';
    } else if ( (v2 <= 645) && (v2 > 575) ) {
      current = 'R';
    } else if ( (v2 <= 575) && (v2 > 510) ) {
      current = 'S';
    } else if ( (v2 <= 510) && (v2 > 440) ) {
      current = 'T';
    } else if ( (v2 <= 440) && (v2 > 375) ) {
      current = 'U';
    } else if ( (v2 <= 375) && (v2 > 305) ) {
      current = 'V';
    } else if ( (v2 <= 305) && (v2 > 235) ) {
      current = 'W';
    } else if ( (v2 <= 235) && (v2 > 170) ) {
      current = 'X';
    } else if ( (v2 <= 170) && (v2 > 100) ) {
      current = 'Y';
    } else if ( (v2 <= 100) && (v2 > 40) ) {
      current = 'Z';
    }

    // Add the character to the buffer
    addChar(current);
    
#if 0
    Serial.print(" ");
    Serial.print(pos);
    Serial.print(" ");
    Serial.print(message);
#endif

    // End early
    return;

  } else if (v2 < 10) {
    v2_above_thresh = false;
  }

  // Line 1 (on key down)
  if ( (v1 > 40) && (!v1_above_thresh) ) {
#if DEBUG
    Serial.print("Line 1: ");
    Serial.print(v1);
    Serial.print(" ");
#endif
    v1_above_thresh = true;
    if ( (v1 <= 930) && (v1 > 860) ) {
      current = 'A';
    } else if ( (v1 <= 860) && (v1 > 795) ) {
      current = 'B';
    } else if ( (v1 <= 795) && (v1 > 715) ) {
      current = 'C';
    } else if ( (v1 <= 715) && (v1 > 645) ) {
      current = 'D';
    } else if ( (v1 <= 645) && (v1 > 575) ) {
      current = 'E';
    } else if ( (v1 <= 575) && (v1 > 510) ) {
      current = 'F';
    } else if ( (v1 <= 510) && (v1 > 440) ) {
      current = 'G';
    } else if ( (v1 <= 440) && (v1 > 375) ) {
      current = 'H';
    } else if ( (v1 <= 375) && (v1 > 305) ) {
      current = 'I';
    } else if ( (v1 <= 305) && (v1 > 235) ) {
      current = 'J';
    } else if ( (v1 <= 235) && (v1 > 170) ) {
      current = 'K';
    } else if ( (v1 <= 170) && (v1 > 100) ) {
      current = 'L';
    } else if ( (v1 <= 100) && (v1 > 40) ) {
      current = 'M';
    }

    // Add the character to the buffer
    addChar(current);
    
#if 0
    Serial.print(" ");
    Serial.print(pos);
    Serial.print(" ");
    Serial.print(message);
#endif

    // End early
    return;

  } else if (v1 < 10) {
    v1_above_thresh = false;
  }

#if DEBUG
  Serial.println();
#endif
}
