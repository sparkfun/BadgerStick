#include <SoftwareSerial.h>

SoftwareSerial master(5,6);

int row1 = 0;
int row2 = 0;
int row3 = 0;
int row4 = 0;

char current;
char message[17];

int pos = 0;

int charID = 0; 

bool back = 0;
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

  for(int i=0; i<16; i++){
  message[i]= ' ';}
  

}

void loop(){ 

  for(int i=0; i<16; i++){
  Serial.print(message[i]);}
  delay(100); //just here to slow down the output, and show it will work  even during a delay

  readKeys();

  cls();

  //MASTER COMMS
  if(master.available()){

    if(master.read()==200){

      while(master.available()){
        master.read();}

    }

      for(int i = 0; i<17; i++){
      master.print(message[i]);

      if(i==16){sentMsg = 1; delay(10000);}
   
      }

      
      
  }

  if(sentMsg){
      for(int i = 0; i<17; i++){
      message[i] = ' ';
      }
      pos = 0;
      sentMsg = 0;
  }



}


void backspace(){
  back = 1;
  if(pos!=0){pos--;}
  message[pos]=' ';
  delay(50);}


void addChar(){
  //ADD

  message[pos]=current;
  if(pos!=15){pos++;}
  delay(50);}

void readKeys(){

if(analogRead(A3)>40){analogRead(A3); delay(50);charID = map(analogRead(A3), 950, 50, 13, 1);

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

}

void cls(){


  Serial.write(0xFE);
  Serial.write(128);
  Serial.write("                ");
  Serial.write(0xFE);
  Serial.write(128);



}

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
 */

