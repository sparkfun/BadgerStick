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
  int v1 = analogRead(3);
  int v2 = analogRead(2);
  int v3 = analogRead(1);
  int v4 = analogRead(0);

#if 0
  Serial.print("Line 3: ");
  Serial.print(v3);
#endif

  // Line 1 (on key down)
  if ( (v1 > 40) && (!v1_above_thresh) ) {
#if DEBUG
    Serial.print("Line 1: ");
    Serial.print(v1);
    Serial.print(" ");
#endif
    v1_above_thresh = true;
    if ( (v1 > 40) && (v1 <= 110) ) {
      current = 'A';
    } else if ( (v1 > 110) && (v1 <= 160) ) {
      current = 'B';
    } else if ( (v1 > 160) && (v1 <= 220) ) {
      current = 'C';
    } else if ( (v1 > 220) && (v1 <= 275) ) {
      current = 'D';
    } else if ( (v1 > 275) && (v1 <= 330) ) {
      current = 'E';
    } else if ( (v1 > 330) && (v1 <= 385) ) {
      current = 'F';
    } else if ( (v1 > 385) && (v1 <= 445) ) {
      current = 'G';
    } else if ( (v1 > 445) && (v1 <= 505) ) {
      current = 'H';
    } else if ( (v1 > 505) && (v1 <= 570) ) {
      current = 'I';
    } else if ( (v1 > 570) && (v1 <= 645) ) {
      current = 'J';
    } else if ( (v1 > 645) && (v1 <= 735) ) {
      current = 'K';
    } else if ( (v1 > 735) && (v1 <= 825) ) {
      current = 'L';
    } else if ( (v1 > 825) && (v1 <= 930) ) {
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

  } else if (v1 < 20) {
    v1_above_thresh = false;
  }

  // Line 2 (on key down)
  if ( (v2 > 40) && (!v2_above_thresh) ) {
#if DEBUG
    Serial.print("Line 2: ");
    Serial.print(v2);
    Serial.print(" ");
#endif
    v2_above_thresh = true;
    if ( (v2 > 40) && (v2 <= 110) ) {
      current = 'N';
    } else if ( (v2 > 110) && (v2 <= 160) ) {
      current = 'O';
    } else if ( (v2 > 160) && (v2 <= 220) ) {
      current = 'P';
    } else if ( (v2 > 220) && (v2 <= 275) ) {
      current = 'Q';
    } else if ( (v2 > 275) && (v2 <= 330) ) {
      current = 'R';
    } else if ( (v2 > 330) && (v2 <= 385) ) {
      current = 'S';
    } else if ( (v2 > 385) && (v2 <= 445) ) {
      current = 'T';
    } else if ( (v2 > 445) && (v2 <= 505) ) {
      current = 'U';
    } else if ( (v2 > 505) && (v2 <= 570) ) {
      current = 'V';
    } else if ( (v2 > 570) && (v2 <= 645) ) {
      current = 'W';
    } else if ( (v2 > 645) && (v2 <= 735) ) {
      current = 'X';
    } else if ( (v2 > 735) && (v2 <= 825) ) {
      current = 'Y';
    } else if ( (v2 > 825) && (v2 <= 930) ) {
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

  } else if (v2 < 20) {
    v2_above_thresh = false;
  }

  // Line 3 (on key down)
  if ( (v3 > 400) && (v3 <= 890) && (!v3_above_thresh) ) {
#if DEBUG
    Serial.print("Line 3: ");
    Serial.print(v3);
    Serial.print(" ");
#endif
    v3_above_thresh = true;
    if ( (v3 > 400) && (v3 <= 615) ) {
      current = '1';
    } else if ( (v3 > 615) && (v3 <= 700) ) {
      current = '2';
    } else if ( (v3 > 700) && (v3 <= 760) ) {
      current = '3';
    } else if ( (v3 > 760) && (v3 <= 795) ) {
      current = '4';
    } else if ( (v3 > 795) && (v3 <= 815) ) {
      current = '5';
    } else if ( (v3 > 815) && (v3 <= 832) ) {
      current = '6';
    } else if ( (v3 > 832) && (v3 <= 845) ) {
      current = '7';
    } else if ( (v3 > 845) && (v3 <= 855) ) {
      current = '8';
    } else if ( (v3 > 855) && (v3 <= 865) ) {
      current = '9';
    } else if ( (v3 > 865) && (v3 <= 875) ) {
      current = '0';
    } else if ( (v3 > 875) && (v3 <= 890) ) {
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

  } else if ( (v3 <= 400) || (v3 > 890) ) {
    v3_above_thresh = false;
  }

  // Line 4 (on key down)
  if ( (v4 > 40) && (!v4_above_thresh) ) {
#if DEBUG
    Serial.print("Line 4: ");
    Serial.print(v4);
    Serial.print(" ");
#endif
    v4_above_thresh = true;
    if ( (v4 > 110) && (v4 <= 160) ) {
      current = '.';
    } else if ( (v4 > 160) && (v4 <= 220) ) {
      current = '!';
    } else if ( (v4 > 220) && (v4 <= 275) ) {
      current = '@';
    } else if ( (v4 > 275) && (v4 <= 330) ) {
      current = '#';
    } else if ( (v4 > 330) && (v4 <= 385) ) {
      current = ':';
    } else if ( (v4 > 385) && (v4 <= 445) ) {
      current = '/';
    } else if ( (v4 > 445) && (v4 <= 505) ) {
      current = '(';
    } else if ( (v4 > 505) && (v4 <= 570) ) {
      current = ')';
    } else if ( (v4 > 570) && (v4 <= 645) ) {
      current = ' ';
    } else if ( (v4 > 645) && (v4 <= 735) ) {
      current = ' ';
    } else if ( (v4 > 735) && (v4 <= 825) ) {
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

#if DEBUG
  Serial.println();
#endif
}

/*
if(analogRead(A3)>40){
  analogRead(A3); 
  delay(50);
  charID = map(analogRead(A3), 950, 50, 13, 1);

switch(charID){

  case 1: 
    current = 'A';
    break;
  case 2: 
    current = 'B';
    break;
  case 3: 
    current = 'C';
    break;
  case 4: 
    current = 'D';
    break;
  case 5: 
    current = 'E';
    break;
  case 6: 
    current = 'F';
    break;
  case 7: 
    current = 'G';
    break;
  case 8: 
    current = 'H';
    break;
  case 9: 
    current = 'I';
    break;
  case 10: 
    current = 'J';
    break;
  case 11: 
    current = 'K';
    break;
  case 12: 
    current = 'L';
    break;
  case 13: 
    current = 'M';
    break;

  
}

charID = 0;
delay(50);
cls();

if(!back){
addChar();}
back=0;


}


if(analogRead(A2)>40){analogRead(A2); delay(50); charID = (map(analogRead(A2), 950, 50, 13, 1));

switch(charID){

  case 1: 
    current = 'N';
    break;
  case 2: 
    current = 'O';
    break;
  case 3: 
    current = 'P';
    break;
  case 4: 
    current = 'Q';
    break;
  case 5: 
    current = 'R';
    break;
  case 6: 
    current = 'S';
    break;
  case 7: 
    current = 'T';
    break;
  case 8: 
    current = 'U';
    break;
  case 9: 
    current = 'V';
    break;
  case 10: 
    current = 'W';
    break;
  case 11: 
    current = 'X';
    break;
  case 12: 
    current = 'Y';
    break;
  case 13: 
    current = 'Z';
    break;

  
}

charID = 0;
delay(50);
cls();
if(!back){
addChar();}
back=0;

}

if(analogRead(A1)<800){analogRead(A1); delay(50); charID = (map(analogRead(A1), 800, 40, 13, 1));

switch(charID){

  case 1: 
    current = '1';
    break;
  case 2: 
    current = '2';
    break;
  case 3: 
    current = '3';
    break;
  case 4: 
    current = '4';
    break;
  case 5: 
    current = '5';
    break;
  case 6: 
    current = '6';
    break;
  case 7: 
    current = '7';
    break;
  case 8: 
    current = '8';
    break;
  case 9: 
    current = '9';
    break;
  case 10: 
    current = '0';
    break;
  case 11: 
    backspace();
    break;
  case 12: 
    backspace();
    break;
  case 13: 
    backspace();
    break;

  
}

charID = 0;
delay(50);
cls();
if(!back){
addChar();}
back=0;

}


if(analogRead(A0)>40){analogRead(A0); delay(50); charID = (map(analogRead(A0), 950, 50, 13, 1));

switch(charID){

  case 1: 
    current = '.';
    break;
  case 2: 
    current = '.';
    break;
  case 3: 
    current = '!';
    break;
  case 4: 
    current = '@';
    break;
  case 5: 
    current = '#';
    break;
  case 6: 
    current = ':';
    break;
  case 7: 
    current = '/';
    break;
  case 8: 
    current = '(';
    break;
  case 9: 
    current = ')';
    break;
  case 10: 
    current = ' ';
    break;
  case 11: 
    current = ' ';
    break;
  case 12: 
    current = ' ';
    break;
  case 13: 
    current = ' ';
    break;

  
}

charID = 0;
delay(50);
cls();
if(!back){
addChar();}
back=0;

}

//message[pos+1] = '_';

}*/

/* UNIT B READKEYS
 *  
 *  void readKeys(){

if(analogRead(A3)>40){analogRead(A3); delay(50);charID = map(analogRead(A3), 850, 80, 13, 1);

switch(charID){

  case 1: 
    current = 'A';
    break;
  case 2: 
    current = 'B';
    break;
  case 3: 
    current = 'C';
    break;
  case 4: 
    current = 'D';
    break;
  case 5: 
    current = 'E';
    break;
  case 6: 
    current = 'F';
    break;
  case 7: 
    current = 'G';
    break;
  case 8: 
    current = 'H';
    break;
  case 9: 
    current = 'I';
    break;
  case 10: 
    current = 'J';
    break;
  case 11: 
    current = 'K';
    break;
  case 12: 
    current = 'L';
    break;
  case 13: 
    current = 'M';
    break;

  
}

charID = 0;
delay(50);
cls();

if(!back){
addChar();}
back=0;

}

if(analogRead(A2)>40){analogRead(A2); delay(50); charID = (map(analogRead(A2), 850, 80, 13, 1));

switch(charID){

  case 1: 
    current = 'N';
    break;
  case 2: 
    current = 'O';
    break;
  case 3: 
    current = 'P';
    break;
  case 4: 
    current = 'Q';
    break;
  case 5: 
    current = 'R';
    break;
  case 6: 
    current = 'S';
    break;
  case 7: 
    current = 'T';
    break;
  case 8: 
    current = 'U';
    break;
  case 9: 
    current = 'V';
    break;
  case 10: 
    current = 'W';
    break;
  case 11: 
    current = 'X';
    break;
  case 12: 
    current = 'Y';
    break;
  case 13: 
    current = 'Z';
    break;

  
}

charID = 0;
delay(50);
cls();
if(!back){
addChar();}
back=0;

}

if(analogRead(A1)<800){analogRead(A1); delay(50); charID = (map(analogRead(A1), 850, 80, 13, 1));

switch(charID){

  case 1: 
    current = '1';
    break;
  case 2: 
    current = '2';
    break;
  case 3: 
    current = '3';
    break;
  case 4: 
    current = '4';
    break;
  case 5: 
    current = '5';
    break;
  case 6: 
    current = '6';
    break;
  case 7: 
    current = '7';
    break;
  case 8: 
    current = '8';
    break;
  case 9: 
    current = '9';
    break;
  case 10: 
    current = '0';
    break;
  case 11: 
    backspace();
    break;
  case 12: 
    backspace();
    break;
  case 13: 
    backspace();
    break;

  
}

charID = 0;
delay(50);
cls();
if(!back){
addChar();}
back=0;

}


if(analogRead(A0)>40){analogRead(A0); delay(50); charID = (map(analogRead(A0), 850, 80, 13, 1));

switch(charID){

  case 1: 
    current = '.';
    break;
  case 2: 
    current = '.';
    break;
  case 3: 
    current = '!';
    break;
  case 4: 
    current = '@';
    break;
  case 5: 
    current = '#';
    break;
  case 6: 
    current = ':';
    break;
  case 7: 
    current = '/';
    break;
  case 8: 
    current = '(';
    break;
  case 9: 
    current = ')';
    break;
  case 10: 
    current = ' ';
    break;
  case 11: 
    current = ' ';
    break;
  case 12: 
    current = ' ';
    break;
  case 13: 
    current = ' ';
    break;

  
}

charID = 0;
delay(50);
cls();
if(!back){
addChar();}
back=0;

}

//message[pos+1] = '_';

}
*/

