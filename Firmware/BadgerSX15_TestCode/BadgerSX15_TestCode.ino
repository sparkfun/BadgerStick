#include <EEPROM.h>
#define MEM_KEY  501
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
boolean test_mode = false;
///////////////////////////////////////////////////////////////////////////////////////////////
void setup()
{

}
///////////////////////////////////////////////////////////////////////////////////////////////
void loop()
{
 test_mode_check();
 delay(1000); 
}
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
    EEPROM.write(MEM_KEY, 0xAB);
    delay(5000);
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
void test_mode_check()
{
 test_mode = true;
 if((EEPROM.read(MEM_KEY) == 0xAB))
   {
    test_mode = false;
   }
 if(test_mode)
   {
    test_code();
   }
 if(!(test_mode))
   {
    pinMode(STAT, OUTPUT);
    digitalWrite(STAT, HIGH);
    delay(1000);
    digitalWrite(STAT, LOW);
   } 
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

