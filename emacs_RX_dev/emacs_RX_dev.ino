
/*
    File: emacs_RX_dev.ino
    nRF24L01 transceiver
    20x4 LCD
    Arduino MEGA, UNO or NANO

      Written by:   Jacob Romero
      email:        admin@jrom.io
      Creative Engineering Solutions LLC

    This module is the dev version of the receiver (RX) in a
    TX/RX pair. 
    Sensor data is transmitted continuously by the
    TX module's nRF24L01 (2.4 GHz transceiver), which
    currently transmits packets of data from 2 sensors:
    the DHT-11 (temp and humid), and the K30 (CO2).
    This will be expanded to include my RF24SensorNet_master
    libraries, significantly modified from the github version.
    
    This is a development version only which may change w/o
    notice, and which may contain commented and uncommented
    development code.

    Credits:
    RF24SensorNet library
    https://github.com/szaffarano/RF24SensorNet
    NRF24L01 library
    TMRh20 https://github.com/TMRh20/RF24
*/

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

struct dataStruct{
  int   chNum;
  int   radNum;
  int   co2_val;
  int   h_val;
  int   c_val;
  int   f_val;
}myData;

byte addresses[][6] = {"1Node", "2Node","3Node","4Node"};

//=============================
//==== Instantiate modules ====
//=============================
RF24 radio(CE, SS); // Arduino Uno pins 9 and 10
LiquidCrystal lcd(7, 6, 5, 4, 3, 2);
//=============================

int   rm =  0; // Holds ReceivedMessage value
int   co2 = 0;
int   rc  = 1;
int   h = 0;
int   t = 0;
int   f = 0;
int   hif = 0;
int   hic = 0;
int   sensorOffset = 0;
bool  sync = false;
int   val = 0;
int   typ = 0; 
int   chan = 0;
int   rad = 0; 

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
    radio.openReadingPipe(1, addresses[1]); // Get NRF24L01 ready to receive
    radio.startListening(); // Listen to see if information received
//    Serial.println("Channel in setup: " + String(radio.getChannel()));
    myData.chNum = mychannel;
    myData.radNum = 0; // 0 is the master controller's number
    Serial.print("Master RF Comms Starting on channel "+String(myData.chNum));
    Serial.println("\tusing radio "+ String(myData.radNum));
 }
//===========================
//======== Main Loop ========
//===========================
void loop(void) {
  while (radio.available()) {
    radio.read(&ReceivedMessage, sizeof(ReceivedMessage)); 
    receive_rm();
//    delay(2000);
  }
}

void receive_rm() {
  chan = ReceivedMessage[0].chNum;
  rad  = ReceivedMessage[0].radNum;       
  co2  = ReceivedMessage[0].co2_val;
  h    = ReceivedMessage[0].h_val;
  t    = ReceivedMessage[0].c_val;
  f    = ReceivedMessage[0].f_val;
  chan_lcd_routine();
  co2_lcd_routine();
  h_lcd_routine();
  t_lcd_routine();
  f_lcd_routine(); 
}

void chan_lcd_routine(void) { 
    Serial.print("\nChannel number: " + String(chan));
    Serial.println("\tTX Radio number: " + String(rad));
    lcd.setCursor(0, 0);
    lcd.print("                    ");
    lcd.setCursor(0, 0);
    lcd.print("Chan:" + String(chan) + " Rad:"+String(rad)); 
}

void co2_lcd_routine(void) {
  Serial.println("CO2 level: "+String(co2));
//  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("                    ");
  lcd.setCursor(0, 1);
  lcd.print("CO2:" + String(co2) + "ppm");
}

void h_lcd_routine(void) {
//  h = float(val);
  Serial.println("Humidity: " + String(h) + "%");
  lcd.setCursor(0, 2);
  lcd.print("                   ");
  lcd.setCursor(0, 2);
  lcd.print("Humid:" + String(h) + "%");
}

void t_lcd_routine(void) {
//  t = float(val);
  Serial.println("Degrees C: " + String(t) + "C");
  lcd.setCursor(0, 3);
  lcd.print("          ");
  lcd.setCursor(0, 3);
  lcd.print("Deg "+String(t) + "C");
}
void f_lcd_routine(void) {
//  f = float(val);
  Serial.println("Degrees F: " + String(f) + "F");
  lcd.setCursor(10, 3);
  lcd.print("          ");
  lcd.setCursor(10, 3);
  lcd.print(String(f) + "F");
  delay(2000);
}
