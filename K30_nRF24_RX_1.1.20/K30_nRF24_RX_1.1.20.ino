/* 
*   File: K30_nRF24_LCD_RX_X.X.20.ino
*   K30 CO2 sensor
*   nRF24L01 transceiver 
*   20x4 LCD 
*   Arduino MEGA, UNO or NANO
* 
*     Written by:   Jacob Romero  
*     email:        admin@jrom.io 
*     Creative Engineering Solutions LLC 
*   
*   This module is the stable version of the receiver (RX) in a 
*   TX/RX pair. The data is transmitted continuously by the 
*   TX module's nRF24L01 (2.4 GHz transceiver), which
*   transmits four 32-bit packets of data from 2 sensors: 
*   the DHT-11 (temp and humid), and the K30 (CO2). 
*   This is a development version only which may change w/o 
*   notice, and which may contain commented and uncommented 
*   development code.    * 
*/
 
// NRF24L01 library created by TMRh20 https://github.com/TMRh20/RF24
#include <RF24.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "SPI.h"

/////////////////////////
//// LCD Statements  ////
#include <LiquidCrystal.h>
#include <Wire.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define D2          2 // LCD DB7
#define D3          3 // LCD DB6
#define D4          4 //LCD DB5
#define D5          5 // LCD DB4
#define EN_LCD      6 // LCD E
#define RS_LCD      7 // LCD RS
#define TEST        8 // Test output

// Enable for MEGA
//#define CE 49
//#define MISO 50
//#define MOSI 51
//#define SCK 52
//#define SS 53

// Enable for UNO or NANO
#define SCK 13
#define MISO 12
#define MOSI 11
#define SS 10
#define CE 9

// Used to store value received by the NRF24L01
uint32_t ReceivedMessage[1]; 

// Receiver address
const uint32_t pipe1 = 0xC6C6C6C0;

//=============================
//==== Instantiate modules ====
//=============================
RF24 radio(CE,SS); 
// NRF24L01 uses SPI pins + Pin 49(CE) and 53(SS)
// on the MEGA
LiquidCrystal lcd(7,6,5,4,3,2);
//=============================

int rm = 0; // Holds ReceivedMessage value
int co2 = 0;
int rc  = 1;
float h = 0;
float t = 0;
float f = 0;
float hif = 0;
float hic = 0;
int sensorOffset = 0;
int waitCount = 0; 
bool sync = false;

//=========================
//========= Setup =========
//=========================
void setup(void){
    lcd.begin(20,4);
    Serial.begin(9600);
    lcd.setCursor(0,0);
    lcd.print("Environmental Sensor");
    lcd.setCursor(0,1);
    lcd.print("Standby for data");
    lcd.setCursor(0,2);
    lcd.print("TEST DATA");
    lcd.setCursor(0,3);
    lcd.print("CES, LLC.");
    delay(1000);
    Serial.println("RF Comms Starting...");
    
    pinMode(SCK, OUTPUT); // 13
    pinMode(MISO, INPUT); // 12
    pinMode(MOSI, OUTPUT); // 11
    pinMode(SS, OUTPUT); // 10
    pinMode(CE, OUTPUT); // 9
    
    pinMode(TEST, OUTPUT); // 8
    pinMode(RS_LCD, OUTPUT); // 7
    pinMode(EN_LCD, OUTPUT); // 6
    
    radio.begin(); // Start the NRF24L01
    radio.setDataRate( RF24_250KBPS );    
    radio.openReadingPipe(1,pipe1); // Get NRF24L01 ready to receive
    radio.setPALevel(RF24_PA_MIN);
    radio.startListening(); // Listen to see if information received
}

//===========================
//======== Main Loop ========
//===========================
void loop(void){ 
    int count;  
    while (radio.available()){
        // Read information from the NRF24L01
        radio.read(&ReceivedMessage, sizeof(ReceivedMessage)); 
        rm = *ReceivedMessage;          
        //Serial.print("Validation: ");
        //Serial.println(ReceivedMessage[0]);
        if(ReceivedMessage[0] < 10){
          receive_rm(1);
        }
        else if(11<=ReceivedMessage[0] && ReceivedMessage[0]< 40){
          receive_rm(2);
        }
        else if(41<=ReceivedMessage[0] && ReceivedMessage[0]< 101){
          receive_rm(3);
        }
        else if(ReceivedMessage[0] >= 101){
          receive_rm(0);
        }

//        lcd.setCursor(0,2);
//        lcd.print("                    ");
//        lcd.setCursor(0,2);
//        lcd.print(ReceivedMessage[0]);
        
        //co2 = rm;
        //co2_lcd_routine();
          
//          sync_data(); 
//          while(sync){
//            for(count=0; count<4; count++){
//              receive_rm(count);
//              delay(1500);
//            }      
//          }   
    //delay(10);
    
    }
}

//=========================================
//=========== Construction Zone ===========

bool wait_for_co2(void){
  bool flag = false;
  int wait = 0;
  waitCount = 16;
  while(wait<waitCount){
    if(rm>250){
      bool flag = true;
      break;
    }
    else {
      wait++;
      delay(250);
    }    
  }
  return flag;
}

// This method waits until there is a data reading
// above 250, which means it is probably a valid 
// CO2 reading. It also sets the sync flag, and only 
// then does it allow the display loop to proceed.

void sync_data(void){
  while(!sync){
    if(rm>250){
      sync = true;
      break;
    }
    else {
      delay(10);
    }    
  }
}
//========================================

void receive_rm(int c){      
    switch(c){
      case 0: // Start with CO2 reading
        //radio.read(&ReceivedMessage, sizeof(ReceivedMessage)); 
        //rm = *ReceivedMessage;
        co2 = rm;
        co2_lcd_routine();        
        break;  
              
      case 1: // Humidity
        //radio.read(&ReceivedMessage, sizeof(ReceivedMessage)); 
        //rm = *ReceivedMessage;
        h = rm;
        h_lcd_routine();
        break;    
            
      case 2: // Temp Celcius
        //radio.read(&ReceivedMessage, sizeof(ReceivedMessage)); 
        //rm = *ReceivedMessage;
        t = rm;
        t_lcd_routine();
        break;      
          
      case 3: // Temp Fahrenheit
        //radio.read(&ReceivedMessage, sizeof(ReceivedMessage)); 
        //rm = *ReceivedMessage;
        f = rm;
        f_lcd_routine();
        break;     
           
      default: // Error Handling
        break;
    }
}

void co2_lcd_routine(void){
    //Serial.print("Validation: ");
    //Serial.println(ReceivedMessage[0]);
    Serial.print("CO2 level: ");
    Serial.println(co2 - sensorOffset);
      
     lcd.clear();
     if (co2<200){
        //lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("We have a problem");
        lcd.setCursor(0,1);
        lcd.println("CO2: "+String(co2));       
      }
        else if (co2>=200 && co2<500){
        //lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("We must be outside");
        lcd.setCursor(0,1);
        lcd.print("CO2: "+ String(co2)+" ppm");        
      }
      else if(co2>=500 && co2<1000) {
        //lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Environmental Sensor");
        lcd.setCursor(0,1);
        lcd.print("CO2 in ppm: "+ String(co2));
        lcd.setCursor(0,2);
        lcd.print("CO2 Level is LOW");
        lcd.setCursor(0,3);
        lcd.print("                    ");
        lcd.setCursor(0,3);
        lcd.print("Opening CO2 Valve");
      } 
      else if(co2>=1000 && co2<1500){
        //lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Environmental Sensor");
        lcd.setCursor(0,1);
        lcd.print("CO2 in ppm: "+ String(co2));
        lcd.setCursor(0,2);
        lcd.print("CO2 Level is GOOD");
        lcd.setCursor(0,3);
        lcd.print("                    ");
        lcd.setCursor(0,3);
        lcd.print("Cycling CO2 Valve");
      }        
      else if (co2>=1500 && co2<2500) { 
        //lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("CO2 Sensor Output");
        lcd.setCursor(0,1);
        lcd.print("CO2 in ppm: "+ String(co2));      
        lcd.setCursor(0,2);
        lcd.print("CO2 Level is HIGH");
        lcd.setCursor(0,3);
        lcd.print("                    ");
        lcd.setCursor(0,3);
        lcd.print("Closing CO2 Valve");
      }
      else if(rm>2500){
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("CO2 LEVEL IS HIGH!!");
        lcd.setCursor(0,1);
        lcd.print("CO2 in ppm: "+ String(co2));      
        lcd.setCursor(0,2);
        lcd.print("DANGER! CLEAR ROOM!");
        lcd.setCursor(0,3);
        lcd.print("                    ");
        lcd.setCursor(0,3);
        lcd.print("Closing CO2 Valve");
      }      
    }

void h_lcd_routine(void){
    h = float(*ReceivedMessage);    
    //Serial.print("Validation: ");
    //Serial.println(ReceivedMessage[0]);
    Serial.println("Humidity: "+String(h)+"%");
    //Serial.println(h);      
     //lcd.clear();
     if (h<=100){      
        lcd.setCursor(0,0);
        lcd.print("Environmental Sensor");
        //lcd.setCursor(0,1);
        //lcd.print("Humidity: ");
        lcd.setCursor(0,2);
        lcd.print("                    ");
        lcd.setCursor(0,2);
        lcd.print("Humidity: " +String(h)+"%");
        lcd.setCursor(0,3);
        lcd.print("                    ");
        lcd.setCursor(0,3);
        lcd.print("CES, LLC.");       
      }
     else if (h>100){
        //lcd.clear();        
        lcd.setCursor(0,0);
        lcd.print("Environmental Sensor");
        lcd.setCursor(0,1);
        lcd.print("We have a problem!");
        lcd.setCursor(0,2);
        lcd.print("                    ");
        lcd.setCursor(0,2);
        lcd.print("Humidity: "+String(h)+" %"); 
        lcd.setCursor(0,3);
        lcd.print("                    ");
        lcd.setCursor(0,3);
        lcd.print("CES, LLC.");        
      } 
    }

void t_lcd_routine(void){
    t = float(*ReceivedMessage);    
    //Serial.print("Validation: ");
    //Serial.println(ReceivedMessage[0]);
    Serial.println("Degrees: "+String(t)+"C");
    //Serial.println(t);      
     //lcd.clear();
     if (t<=100){
        //lcd.clear();        
        lcd.setCursor(0,0);
        lcd.print("Environmental Sensor");
        //lcd.setCursor(0,1);
        //lcd.print("Temp Celcius");
        lcd.setCursor(0,2);
        lcd.print("                    ");
        lcd.setCursor(0,2);
        lcd.print(String(t)+" degrees C");
        lcd.setCursor(0,3);
        lcd.print("                    ");
        lcd.setCursor(0,3);
        lcd.print("CES, LLC.");         
      }
     else if (t>100){
        //lcd.clear();        
        lcd.setCursor(0,0);
        lcd.print("Environmental Sensor");
        lcd.setCursor(0,1);
        lcd.print("We have a problem!");
        lcd.setCursor(0,2);
        lcd.print("                    ");
        lcd.setCursor(0,2);
        lcd.print("Temp: "+String(t)+"C"); 
        lcd.setCursor(0,3);
        lcd.print("                    ");
        lcd.setCursor(0,3);
        lcd.print("It Burns!!");            
      } 
    }

void f_lcd_routine(void){
    f = float(*ReceivedMessage);    
    //Serial.print("Validation: ");
    //Serial.println(ReceivedMessage[0]);
    Serial.println("Degrees: "+String(f)+"F");
    //Serial.println(f);      
     //lcd.clear();
     if (f<=200){
        //lcd.clear();        
        lcd.setCursor(0,0);
        lcd.print("Environmental Sensor");
        //lcd.setCursor(0,1);
        //lcd.print("Temp Fahrenheit");
        lcd.setCursor(0,2);
        lcd.print("                    ");
        lcd.setCursor(0,2);
        lcd.print(String(f)+" degrees F");
        lcd.setCursor(0,3);
        lcd.print("                    ");
        lcd.setCursor(0,3);
        lcd.print("CES, LLC.");         
      }
     else if (f>200){
        //lcd.clear();        
        lcd.setCursor(0,0);
        lcd.print("Environmental Sensor");
        //lcd.setCursor(0,1);
        //lcd.print("We have a problem!");
        lcd.setCursor(0,2);
        lcd.print("                    ");
        lcd.setCursor(0,2);
        lcd.print("Degrees: "+String(f)+"F"); 
        lcd.setCursor(0,3);
        lcd.print("                    ");
        lcd.setCursor(0,3);
        lcd.print("It Burns!!");      
      } 
    }
//======= Construction Zone =======
void change_waitCount(int n){
  
}
    

        //lcd.clear();        
        //lcd.setCursor(0,0);
        //lcd.print("                    ");   
