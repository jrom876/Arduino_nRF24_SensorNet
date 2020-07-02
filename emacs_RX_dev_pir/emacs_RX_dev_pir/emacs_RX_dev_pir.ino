
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
    libraries, but significantly modified from the github version.
    
    This is a development version only which may change w/o
    notice, and which may contain commented and uncommented
    development code.

    Credits:
    RF24SensorNet library
    https://github.com/szaffarano/RF24SensorNet
    NRF24L01 library
    TMRh20 https://github.com/TMRh20/RF24
*/
 
//========= Libraries =========//
#include <RF24.h>
#include <nRF24L01.h>
#include <RF24_config.h>
#include <SPI.h>
#include <stdlib.h>
#include <stdio.h> 
#include <printf.h>

//// LCD Statements  ////
//#include <LiquidCrystal.h>
#include <Wire.h>
#include <stdio.h>
#include <time.h>

//#define D2          2 // LCD DB7
//#define D3          3 // LCD DB6
//#define D4          4 //LCD DB5
//#define D5          5 // LCD DB4
//#define EN_LCD      6 // LCD E
//#define RS_LCD      7 // LCD RS

#define SCK 13
#define MISO 12
#define MOSI 11
#define SS 10
#define CE 9
//#define PIR_PIN 8

struct dataStruct{
  int   chNum;
  int   radNum;
  int   co2_val;
  int   h_val;
  int   c_val;
  int   f_val;
  int   pir_state;
  int   audio_gate;
}myData;

struct pingCmd{
  int   cmd_type;
  int   ch_cmd;  
}myPing;

byte addresses[][6] = {"1Node","2Node","3Node","4Node","5Node","6Node"};

//=============================
//==== Instantiate modules ====
//=============================
RF24 radio(CE, SS); // Arduino Uno pins 9 and 10
//LiquidCrystal lcd(7, 6, 5, 4, 3, 2);
//=============================

int   rm =  0; // Holds ReceivedMessage value
int   co2 = 0;
int   pir = 0;
int   gate = 0;
int   h = 0;
int   t = 0;
int   f = 0;
int   hif = 0;
int   hic = 0;
int   sensorOffset = 0;
int   typ = 0; 
int   chan = 0;
int   rad = 0; 
int   state;

// Used to store value received by the Master
struct dataStruct ReceivedMessage[1] = {0};

// Used to store ping sent by the Master to TX
struct pingCmd PingMessage[1] = {0};

//=========================
//========= Setup =========
//=========================
void setup(void) {
//    lcd.begin(20, 4);
    Serial.begin(9600);
    SPI.begin();
//    lcd.clear();
//    lcd.setCursor(0, 0);
//    lcd.print("Environmental Sensor");
//    lcd.setCursor(0, 1);
//    lcd.print("Standby for data");
//    lcd.setCursor(0, 2);
//    lcd.print("TEST DATA THIS LINE");
//    lcd.setCursor(0, 3);
//    lcd.print("CES, LLC.");
    delay(1000);
  
    pinMode(SCK, OUTPUT); // 13
    pinMode(MISO, INPUT); // 12
    pinMode(MOSI, OUTPUT); // 11
    pinMode(SS, OUTPUT); // 10
    pinMode(CE, OUTPUT); // 9  
//    pinMode(PIR_PIN,INPUT); // 8 test pin
//    pinMode(RS_LCD, OUTPUT); // 7
//    pinMode(EN_LCD, OUTPUT); // 6

    setupMasterRadio(76); // channel number
}

//===========================
//======== Main Loop ========
//===========================
void loop(void) {
  receive_data();
}

void receive_data() {  
    if (radio.available(addresses[1])) {
      radio.read(&ReceivedMessage, sizeof(ReceivedMessage)); 
      rad = ReceivedMessage[0].radNum;
      receive_rm(rad);
      //send_master_cmd(12);
    }
    if (radio.available(addresses[2])){
      radio.read(&ReceivedMessage, sizeof(ReceivedMessage)); 
      rad = ReceivedMessage[0].radNum;
      receive_rm(rad);
      //radio.stopListening(); // Listen to see if information received
      //send_master_cmd(22);
    }
    if (radio.available(addresses[3])){
      radio.read(&ReceivedMessage, sizeof(ReceivedMessage)); 
      rad = ReceivedMessage[0].radNum;
      receive_rm(rad);
      //radio.stopListening(); // Listen to see if information received
      //send_master_cmd(32);
    }
    if (radio.available(addresses[4])){
      radio.read(&ReceivedMessage, sizeof(ReceivedMessage));
      rad = ReceivedMessage[0].radNum; 
      receive_rm(rad);
      //radio.stopListening(); // Listen to see if information received
      //send_master_cmd(42);
    }
    if (radio.available(addresses[5])){
      radio.read(&ReceivedMessage, sizeof(ReceivedMessage));
      rad = ReceivedMessage[0].radNum; 
      receive_rm(rad);
      //radio.stopListening(); // Listen to see if information received
      //send_master_cmd(52);
    }  
}

//===================================
//======= Setup Master Radio ========
//===================================

void setupMasterRadio(int mychannel) { 
    radio.begin(); // Start the NRF24L01
    radio.setDataRate( RF24_250KBPS ); // RF24_1MBPS, RF24_2MBPS
    radio.setPALevel(RF24_PA_MIN); // MIN, LOW, HIGH, and MAX
    radio.setChannel(mychannel);
    radio.openWritingPipe(addresses[0]);
    radio.openReadingPipe(1, addresses[1]);
    radio.openReadingPipe(2, addresses[2]); 
    radio.openReadingPipe(3, addresses[3]); 
    radio.openReadingPipe(4, addresses[4]); 
    radio.openReadingPipe(5, addresses[5]); 
    radio.startListening(); // Listen to see if information received
  //    Serial.println("Channel in setup: " + String(radio.getChannel())); // DBPRINT
    myData.chNum = mychannel;
    myData.radNum = 0; // 0 is the master controller's number
    Serial.print("Master RF Comms Starting on channel "+String(myData.chNum));
    Serial.println("\tusing radio "+ String(myData.radNum));
}
 
void receive_rm(int radnum) {
    chan = ReceivedMessage[0].chNum;
    rad  = ReceivedMessage[0].radNum;       
    co2  = ReceivedMessage[0].co2_val + sensorOffset;
    h    = ReceivedMessage[0].h_val;
    t    = ReceivedMessage[0].c_val;
    f    = ReceivedMessage[0].f_val;
    pir  = ReceivedMessage[0].pir_state;
    gate = ReceivedMessage[0].audio_gate;
   
    switch (radnum) {
      case 0: // Master only mode, not used
        //do something when var equals 0
        break;
      case 1: // K30 + DHT11 main pipe
          chan_lcd_routine();
          co2_lcd_routine();
          h_lcd_routine();
          t_lcd_routine();
          f_lcd_routine();
        break;
      case 2: // K30 + DHT11 alternate pipe
          chan_lcd_routine();
          co2_lcd_routine();
          h_lcd_routine();
          t_lcd_routine();
          f_lcd_routine();
        break;
      case 3: // DHT11 only
          chan_lcd_routine();
          h_lcd_routine();
          t_lcd_routine();
          f_lcd_routine();
        break;
      case 4: // Audio detector
          if (gate) {
            pir = 0;
            audio_routine();
          } 
        break;
      case 5: // PIR + Audio detector
          chan_lcd_routine();
          if (pir && gate) {
            pir_routine();
            audio_routine();
          }
          else if (pir) {
            gate = 0;
            pir_routine();
          }
          else if (gate) {
            pir = 0;
            audio_routine();
          } 
        break;
      default:
        // if nothing else matches, do the default
        // default is optional
        break;
    }
}

void chan_lcd_routine(void) { 
    Serial.println("\nRX Channel number: " + String(chan));
    Serial.println("Input Radio number: " + String(rad));
//    lcd.setCursor(0, 0);
//    lcd.print("                    ");
//    lcd.setCursor(0, 0);
//    lcd.print("Chan: " + String(chan) + "  Radio: "+String(rad)); 
}

void co2_lcd_routine(void) {
    Serial.println("CO2 level: "+String(co2));
  //  lcd.clear();
//    lcd.setCursor(0, 1);
//    lcd.print("                    ");
//    lcd.setCursor(0, 1);
//    lcd.print("CO2: " + String(co2) + " ppm");
}

void h_lcd_routine(void) {
    Serial.println("Humidity: " + String(h) + "%");
//    lcd.setCursor(0, 2);
//    lcd.print("                   ");
//    lcd.setCursor(0, 2);
//    lcd.print("Humid: " + String(h) + "%");
}

void t_lcd_routine(void) {
    Serial.println("Degrees C: " + String(t) + "C");
//    lcd.setCursor(0, 3);
//    lcd.print("          ");
//    lcd.setCursor(0, 3);
//    lcd.print("Deg "+String(t) + "C");
}

void f_lcd_routine(void) {
    Serial.println("Degrees F: " + String(f) + "F");
//    lcd.setCursor(10, 3);
//    lcd.print("          ");
//    lcd.setCursor(10, 3);
//    lcd.print(String(f) + "F");
//    delay(2000);
}

void pir_routine(void) {  
    if (pir) {
      Serial.println("Intruder Detected" );
      Serial.println("Time Stamp: " + String(100));   
    }else {
      Serial.println("No Intruders" );  
    }
    delay(2000);
}

void audio_routine(void) {  
    if (gate) {
      Serial.println("Audio Detected" );
      Serial.println("Time Stamp: " + String(200));   
    }else {
      Serial.println("All Quiet" );  
    }
    delay(2000);
}

//================================
//======= Send Ping Method =======
//================================

void send_master_cmd(int msg){
    PingMessage[0].cmd_type = 1;
    PingMessage[0].ch_cmd = msg;
    Serial.println("Sending CMD Type: "+String(PingMessage[0].cmd_type));
    Serial.println("Sending Channel CMD: "+String(PingMessage[0].ch_cmd));
//    Serial.println("Channel: " + String(radio.getChannel())); // DBPRINT
    radio.stopListening(); // Change to TX role just long enough to ping cmd
    radio.write(&PingMessage, sizeof(PingMessage));
    radio.startListening(); // Change back to receiver role
  }


//========================================//
//============ Serial Commands ===========//
//========================================//

void serialCommInput() {
  if (Serial.available() > 0 ) {
    if(Serial.peek() == 'c') {
      Serial.read();
      state = Serial.parseInt();
//      digitalWrite(pir,state);??
    }
    while (Serial.available() > 0) {
      Serial.read();
    }
  }
}
