
/*
    File: emacs_RX_dev.ino
    nRF24L01 transceiver
    20x4 LCD
    Arduino MEGA, UNO or NANO

      Written by:   Jacob Romero
      email:        admin@jrom.io
      Creative Engineering Solutions LLC

    This module is the dev version of the receiver (RX) in a
    TX/RX pair. The data is transmitted continuously by the
    TX module's nRF24L01 (2.4 GHz transceiver), which
    transmits four 32-bit packets of data from 2 sensors:
    the DHT-11 (temp and humid), and the K30 (CO2).
    This is a development version only which may change w/o
    notice, and which may contain commented and uncommented
    development code.
*/

//#include <RF24.h>
//#include <nRF24L01.h>
//#include <RF24_config.h>
//#include <printf.h>
//#include <SPI.h>
//#include "SPI.h"
//#include <stdlib.h>
//#include <stdio.h> 
// NRF24L01 library created by TMRh20 https://github.com/TMRh20/RF24
#include <RF24.h>
#include <nRF24L01.h>
#include <RF24_config.h>
#include <SPI.h>
#include <stdlib.h>
#include <stdio.h> 
#include <printf.h>

/////////////////////////
//// LCD Statements  ////
#include <LiquidCrystal.h>
#include <Wire.h>
#include <stdio.h>
#include <time.h>

#define D2          2 // LCD DB7
#define D3          3 // LCD DB6
#define D4          4 //LCD DB5
#define D5          5 // LCD DB4
#define EN_LCD      6 // LCD E
#define RS_LCD      7 // LCD RS

// Enable for UNO or NANO
#define SCK 13
#define MISO 12
#define MOSI 11
#define SS 10
#define CE 9

// Enable for MEGA
//#define CE 49
//#define MISO 50
//#define MOSI 51
//#define SCK 52
//#define SS 53

struct dataStruct {
  int   type;
  int   value;
} myData;

byte addresses[][6] = {"1Node", "2Node","3Node","4Node"};

// Receiver address
const uint32_t pipe1 = 0xC6C6C6C0;

//=============================
//==== Instantiate modules ====
//=============================
RF24 radio(CE, SS); // Arduino Uno pins 9 and 10
LiquidCrystal lcd(7, 6, 5, 4, 3, 2);
//=============================

int rm =  0; // Holds ReceivedMessage value
int co2 = 0;
int rc  = 1;
float h = 0;
float t = 0;
float f = 0;
float hif = 0;
float hic = 0;
int sensorOffset = 0;
bool sync = false;
int val = 0;
int typ = 0; 

// Used to store value received by the NRF24L01
//uint32_t ReceivedMessage[1]; 

// Used to store value received by the NRF24L01
struct dataStruct ReceivedMessage[1] = {0};

//=========================
//========= Setup =========
//=========================
void setup(void) {
    lcd.begin(20, 4);
    Serial.begin(9600);
    SPI.begin();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Environmental Sensor");
    lcd.setCursor(0, 1);
    lcd.print("Standby for data");
    lcd.setCursor(0, 2);
    lcd.print("TEST DATA THIS LINE");
    lcd.setCursor(0, 3);
    lcd.print("CES, LLC.");
    delay(1000);
  
    pinMode(SCK, OUTPUT); // 13
    pinMode(MISO, INPUT); // 12
    pinMode(MOSI, OUTPUT); // 11
    pinMode(SS, OUTPUT); // 10
    pinMode(CE, OUTPUT); // 9  
    pinMode(RS_LCD, OUTPUT); // 7
    pinMode(EN_LCD, OUTPUT); // 6

    setupMasterRadio(76); // channel number
}

 void setupMasterRadio(int mychannel) { 
    radio.begin(); // Start the NRF24L01
    radio.setDataRate( RF24_250KBPS ); // or use RF24_1MBPS
    radio.setPALevel(RF24_PA_MIN); // MIN, LOW, HIGH, and MAX
    radio.setChannel(mychannel);
//    radio.openReadingPipe(1,pipe1);
    radio.openReadingPipe(1, addresses[1]); // Get NRF24L01 ready to receive
    radio.startListening(); // Listen to see if information received
    Serial.println("Channel in setup: " + String(radio.getChannel()));
 
    Serial.println("RF Comms Starting...");
 }
//===========================
//======== Main Loop ========
//===========================
void loop(void) {
  while (radio.available()) {
    radio.read(&ReceivedMessage, sizeof(ReceivedMessage)); 
    val = ReceivedMessage[0].value;
    typ = ReceivedMessage[0].type; 
    Serial.println("\nChannel inside main: " + String(radio.getChannel()));
    Serial.println("Value: "+String(val)+"\tType: "+String(typ));
    receive_rm(typ);
    delay(2000);
  }
}

void receive_rm(int c) {
  switch (c) {
    case 1: // Start with CO2 reading
      lcd.clear();
      co2_lcd_routine();
      break;

    case 2: // Humidity
      //h = ReceivedMessage[0].value;
      h_lcd_routine();
      break;

    case 3: // Temp Celcius
      //t = ReceivedMessage[0].value;
      t_lcd_routine();
      break;

    case 4: // Temp Fahrenheit
      //f = ReceivedMessage[0].value;
      f_lcd_routine();
      break;

    default: // Error Handling
      break;
  }
}

void co2_lcd_routine(void) {
//  co2 = ReceivedMessage[0].value;
  //    Serial.print("Validation of CO2: ");
  //    Serial.print(co2);
  Serial.print("CO2 level: ");
  Serial.println(val - sensorOffset);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("                     ");
  lcd.setCursor(0, 0);
  lcd.print("CO2: " + String(val) + " ppm");
}

void h_lcd_routine(void) {
  h = float(ReceivedMessage[0].value);
  //Serial.print("Validation for humidity: ");
  //Serial.println(myData[0]);
  Serial.println("Humidity: " + String(h) + "%");
//  Serial.println(h);
  lcd.setCursor(0, 1);
  lcd.print("                    ");
  lcd.setCursor(0, 1);
  lcd.print("Humidity: " + String(h) + "%");
  //    lcd.setCursor(0,3);
  //    lcd.print("                    ");
}

void t_lcd_routine(void) {
  t = float(ReceivedMessage[0].value);
  //Serial.print("Validation for temp c: ");
  //Serial.println(myData[0]);
  Serial.println("Degrees: " + String(t) + "C");
//  Serial.println(t);
  lcd.setCursor(0, 2);
  lcd.print("                    ");
  lcd.setCursor(0, 2);
  lcd.print(String(t) + " degrees C");
}
void f_lcd_routine(void) {
  f = float(ReceivedMessage[0].value);
  //Serial.print("Validation for temp f: ");
  //Serial.println(myData[0]);
  Serial.println("Degrees: " + String(f) + "F");
//  Serial.println(f);
  lcd.setCursor(0, 3);
  lcd.print("                    ");
  lcd.setCursor(0, 3);
  lcd.print(String(f) + " degrees F");
}
