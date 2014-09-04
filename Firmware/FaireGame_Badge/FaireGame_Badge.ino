/*

FairGame Final Firmware

Created 9/4/14
By Nick Poole

*/

#include <EEPROM.h>

boolean workingInventory[] = {0,0,0,0,0,0,0,0,0,0}; // This array stores the items that have been collected by the player

int validInventory[] = {11,13,17,19,23,29,31,37,41,43}; // This array stores the valid values for each item, to be compared against the
                                                        // item code from each station so that they can't be easily spoofed.

void setup() {
  
   // Obvi
Serial.begin(1200); 
  
  // RGB LED - 10 is Red, 11 is Green, 9 is Blue
pinMode(9, OUTPUT); 
pinMode(10, OUTPUT);
pinMode(11, OUTPUT);

  // Buzzer is on pin 2
pinMode(2, OUTPUT); 

ledTest(1); // This is a test for the user, it allows them to verify that their soldering 
            // was up to snuff and that the LED is functional. 
            // It runs once on startup and fades a rainbow pattern.

pullInventory(); // This function retrieves the working inventory items from EEPROM

delay(1000);

}

void loop() {

   displayInventory();
  
      for(int c = 0; c < 500; c++){ // Check for incoming serial a BUNCH
        checkPoint();
      }
    
  delay(2500); // Rest on it
  
}

void pullInventory() {
 
  for(int i = 0; i < 10; i++){
   
   if(EEPROM.read(i)==validInventory[i]){workingInventory[i]=1;}
    
  }
  
}

void displayInventory() { // Simply diplays each inventory item in sequence
  
  if(workingInventory[0]==1){
    analogWrite(10,60); analogWrite(11, 30); analogWrite(9, 0); //Bright Red
    delay(500);
    digitalWrite(9,0); digitalWrite(10,0); digitalWrite(11,0); 
    delay(250);
  }
  
  if(workingInventory[1]==1){
    analogWrite(10, 8); analogWrite(11, 0); analogWrite(9, 0); //Dark Red
    delay(500);
    digitalWrite(9,0); digitalWrite(10,0); digitalWrite(11,0); 
    delay(250);
  }
  
  if(workingInventory[2]==1){
    analogWrite(10, 35); analogWrite(11, 90); analogWrite(9, 20); //Yellow
    delay(500);
    digitalWrite(9,0); digitalWrite(10,0); digitalWrite(11,0); 
    delay(250);
  }
  
  if(workingInventory[3]==1){
    analogWrite(10, 0); analogWrite(11, 30); analogWrite(9, 0); //Green
    delay(500);
    digitalWrite(9,0); digitalWrite(10,0); digitalWrite(11,0); 
    delay(250);
  }
  
  if(workingInventory[4]==1){
    analogWrite(10, 0); analogWrite(11, 30); analogWrite(9, 30); //Aqua
    delay(500);
    digitalWrite(9,0); digitalWrite(10,0); digitalWrite(11,0); 
    delay(250);
  }
  
  if(workingInventory[5]==1){
    analogWrite(10, 8); analogWrite(11, 30); analogWrite(9, 0); //Orange
    delay(500);
    digitalWrite(9,0); digitalWrite(10,0); digitalWrite(11,0); 
    delay(250);
  }
  
  if(workingInventory[6]==1){
    analogWrite(10, 0); analogWrite(11, 0); analogWrite(9, 30); //Blue
    delay(500);
    digitalWrite(9,0); digitalWrite(10,0); digitalWrite(11,0); 
    delay(250);
  }
  
  if(workingInventory[7]==1){
    analogWrite(10, 8); analogWrite(11, 30); analogWrite(9, 30); //White
    delay(500);
    digitalWrite(9,0); digitalWrite(10,0); digitalWrite(11,0); 
    delay(250);
  }

  if(workingInventory[8]==1){
    analogWrite(10, 8); analogWrite(11, 0); analogWrite(9, 30); //Pink
    delay(500);
    digitalWrite(9,0); digitalWrite(10,0); digitalWrite(11,0); 
    delay(250);
  }
  
  if(workingInventory[9]==1){
    analogWrite(10, 5); analogWrite(11, 0); analogWrite(9, 100); //Violet
    delay(500);
    digitalWrite(9,0); digitalWrite(10,0); digitalWrite(11,0); 
    delay(250);
  } 

}

void checkPoint() { // Find out which station is trying to talk to us, write valid values to first 10 EEPROM addresses 
                    // For testing purposes, if a "$" is returned then the item was successfully collected. A "!" signifies error. 
  int inBit = 0;
  
  if(Serial.available()){
    
    inBit = Serial.read();
        
    switch (inBit) {
    
      case 'A': 
        EEPROM.write(0, 11);
        Serial.print("$");
        pullInventory();
        break;
      
      case 'B': 
        EEPROM.write(1, 13);
        Serial.print("$");
        pullInventory();
        break;
      
      case 'C': 
        EEPROM.write(2, 17);
        Serial.print("$");
        pullInventory();
        break;
      
      case 'D': 
        EEPROM.write(3, 19);
        Serial.print("$");
        pullInventory();
        break;

      case 'E': 
        EEPROM.write(4, 23);
        Serial.print("$");
        pullInventory();
        break;

      case 'F': 
        EEPROM.write(5, 29);
        Serial.print("$");
        pullInventory();
        break;

      case 'G': 
        EEPROM.write(6, 31);
        Serial.print("$");
        pullInventory();
        break;

      case 'H': 
        EEPROM.write(7, 37);
        Serial.print("$");
        pullInventory();
        break;

      case 'I': 
        EEPROM.write(8, 41);
        Serial.print("$");
        pullInventory();
        break;

      case 'J': 
        EEPROM.write(9, 43);
        Serial.print("$");
        pullInventory();
        break;
      
      case 'X': // This command is used for testing purposes and clears all inventory from EEPROM
        killInventory();
        Serial.print("&");
        pullInventory();
        break;
      
      default: 
        Serial.print("!");
        analogWrite(10, 8);  
        delay(500); 
        analogWrite(10, 0);   
    
    }
  }

}

void killInventory() { // This function is used to clear all inventory items from EEPROM
 
  for(int i = 0; i < 10; i++){
    EEPROM.write(i,0);
  }
  
}

void ledTest(int repeats){ // rainbow flashy!
  
int r, g, b;  

  for(int z = 0; z<repeats; z++){
  
  r=15;
  g=0;
  b=0;
  
    while(g<30){
      analogWrite(10, r); analogWrite(11, g); analogWrite(9, b);
      if(r>0){r--;}
      g++;
      delay(5);
    }
    
    while(b<30){
      analogWrite(10, r); analogWrite(11, g); analogWrite(9, b);
      g--;
      b++;
      delay(5);
    }
    
    while(b>0){
      analogWrite(10, r); analogWrite(11, g); analogWrite(9, b);
      b--;
      if(r<15){r++;}
      delay(5);
    }
  
  }

analogWrite(10, 0); analogWrite(11, 0); analogWrite(9, 0);

}
