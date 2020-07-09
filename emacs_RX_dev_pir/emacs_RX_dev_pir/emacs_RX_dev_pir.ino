
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

enum cmd_type {
    nada     = 0,   
    chan_chg = 1, 
    rad_chg   = 2, 
    pa_chg  = 4
};

enum PA_type {
    PA_MIN = 0,   
    PA_LOW, 
    PA_HIGH, 
    PA_MAX
};

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
  cmd_type my_cmd;
  int      ch_cmd;
  int      r_num;
  PA_type  pa_cmd;    
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
//int   typ = 0; 
int   chan = 0;
int   rad = 0; 
PA_type   pwr_lvl;
int   state; // keep for dev

// Used to store value received by the Master
struct dataStruct ReceivedMessage[1] = {0};

// Used to store ping sent by the Master to TX
struct pingCmd PingMessage[1] = {nada,0};

//=========================
//========= Setup =========
//=========================
void setup(void) {
//    lcd.begin(20, 4);
    Serial.begin(9600);
    SPI.begin();
//    delay(1000);
  
    pinMode(SCK, OUTPUT); // 13
    pinMode(MISO, INPUT); // 12
    pinMode(MOSI, OUTPUT); // 11
    pinMode(SS, OUTPUT); // 10
    pinMode(CE, OUTPUT); // 9 

    setupMasterRadio(81,PA_MIN); // channel number
}

//===========================
//======== Main Loop ========
//===========================
void loop(void) {
  receive_data();
  serialCommInput();
}
//===========================

void receive_data() {  
    if (radio.available(addresses[1])) {
      radio.read(&ReceivedMessage, sizeof(ReceivedMessage)); 
      rad = ReceivedMessage[0].radNum;
      receive_rm(rad);
    }
    if (radio.available(addresses[2])){
      radio.read(&ReceivedMessage, sizeof(ReceivedMessage)); 
      rad = ReceivedMessage[0].radNum;
      receive_rm(rad);
    }
    if (radio.available(addresses[3])){
      radio.read(&ReceivedMessage, sizeof(ReceivedMessage)); 
      rad = ReceivedMessage[0].radNum;
      receive_rm(rad);
    }
    if (radio.available(addresses[4])){
      radio.read(&ReceivedMessage, sizeof(ReceivedMessage));
      rad = ReceivedMessage[0].radNum; 
      receive_rm(rad);
    }
    if (radio.available(addresses[5])){
      radio.read(&ReceivedMessage, sizeof(ReceivedMessage));
      rad = ReceivedMessage[0].radNum; 
      receive_rm(rad);
    }  
}

//===================================
//======= Setup Master Radio ========
//===================================

void setupMasterRadio(int mychannel,PA_type pat) { 
    radio.begin(); // Start the NRF24L01
    radio.setDataRate( RF24_250KBPS ); // RF24_1MBPS, RF24_2MBPS
    setPowerLevel(pat);
    //radio.setPALevel(RF24_PA_MIN); // MIN, LOW, HIGH, and MAX
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
//    Serial.print("Master RF Comms Starting on channel "+String(myData.chNum));
//    Serial.println("\tusing radio "+ String(myData.radNum));
//    delay(1000);
    checkRadioConn();
}

//======================================
//======= Check Radio Connection =======
//======================================
void checkRadioConn() {    
    printf_begin(); // Call this before calling printDetails()
    if (radio.isChipConnected()) {
      Serial.println("\nMaster Radio is connected to Channel: " + String(radio.getChannel()));
      radio.printDetails();  
      Serial.println("RF Comms Starting...");
    }else{
      Serial.println("\nRadio is not connected; showing Channel: " + String(radio.getChannel()));
      radio.printDetails();
    }  
}
//======================================
//======= Receive Message et al ========
//====================================== 
void receive_rm(int radnum) {
    assignValuesFromRcvMsg();
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
          //send_ping_cmd(nada, chan, rad, PA_MIN);
          break;
      case 2: // K30 + DHT11 alternate pipe
          chan_lcd_routine();
          co2_lcd_routine();
          h_lcd_routine();
          t_lcd_routine();
          f_lcd_routine();
          break;
      case 3: // PIR + Audio detector
          chan_lcd_routine();
          if (pir) {
            gate = 0;
            pir_routine();
          }
          if (gate) {
            pir = 0;
            audio_routine();
          }
          //send_ping_cmd(nada, chan, rad, PA_LOW); 
          break;
      case 4: // PIR + Audio detector
          chan_lcd_routine();
          if (pir) {
//            gate = 0;
            pir_routine();
          }
//          if (gate) {
//            pir = 0;
//            audio_routine();
//          }
          //send_ping_cmd(nada, chan, rad, PA_LOW); 
          break;
      case 5: // PIR + Audio detector
          chan_lcd_routine();
          if (pir) {
            gate = 0;
            pir_routine();
          }
          if (gate) {
            pir = 0;
            audio_routine();
          }
          //send_ping_cmd(nada, chan, rad, PA_LOW); 
          break;
      default:
          break;
    }
}
void assignValuesFromRcvMsg(){  
    chan = ReceivedMessage[0].chNum;
    rad  = ReceivedMessage[0].radNum;       
    co2  = ReceivedMessage[0].co2_val + sensorOffset;
    h    = ReceivedMessage[0].h_val;
    t    = ReceivedMessage[0].c_val;
    f    = ReceivedMessage[0].f_val;
    pir  = ReceivedMessage[0].pir_state;
    gate = ReceivedMessage[0].audio_gate;
}
void setPowerLevel(PA_type pwr) {
  switch (pwr) {
    case 0:
        radio.setPALevel(RF24_PA_MIN);
        break;
    case 1:
        radio.setPALevel(RF24_PA_LOW);
        break;
    case 2:
        radio.setPALevel(RF24_PA_HIGH);
        break;
    case 3:
        radio.setPALevel(RF24_PA_MAX);
        break;
    default:
        break;
  }
}
//============================
//======= LCD Routines =======
//============================
void chan_lcd_routine(void) { 
    Serial.println("\nRX Channel number: " + String(chan));
    Serial.println("Input Radio number: " + String(rad)); 
}
void co2_lcd_routine(void) {
    Serial.println("CO2 level: "+String(co2));
}
void h_lcd_routine(void) {
    Serial.println("Humidity: " + String(h) + "%");
}
void t_lcd_routine(void) {
    Serial.println("Degrees C: " + String(t) + "C");
}
void f_lcd_routine(void) {
    Serial.println("Degrees F: " + String(f) + "F");
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
void send_ping_cmd(cmd_type cmd, int ch, int rn, PA_type pat){
    PingMessage[0].my_cmd = cmd;
    PingMessage[0].ch_cmd = ch;
    PingMessage[0].r_num = rn;
    PingMessage[0].pa_cmd = pat;
    Serial.println("Ping CMD Type: "+String(PingMessage[0].my_cmd));
    Serial.println("Ping Channel: "+String(PingMessage[0].ch_cmd));
    Serial.println("Ping Radio Number: "+String(PingMessage[0].r_num));
    Serial.println("Ping Pwr Level: "+String(PingMessage[0].pa_cmd));
//    Serial.println("Channel: " + String(radio.getChannel())); // DBPRINT 
    radio.stopListening(); // Change to TX role just long enough to ping cmd     
    radio.openWritingPipe(addresses[0]);
    radio.write(&PingMessage, sizeof(PingMessage));
    radio.startListening();
}
//========================================//
//============ Serial Commands ===========//
//========================================//
/* 
 *  CHANGE RADIO NUMBER WITH CAUTION!!
 * NOTE about calling rad_chg: It is a general 
 * rule not to change the radio number 
 * of a TX node when sending serial commands. 
 * This precludes possible pipe collisions, 
 * as TX pipes are assigned by radio number. 
*/

void serialCommInput() {
  if (Serial.available()) {
    char c = toupper(Serial.read());
    if (c == 'A' ) {
      Serial.println("\n*** CHANGING TX RADIO 4 TO CHANNEL 97");
      send_ping_cmd(chan_chg, 97, 4, PA_MIN);
    } 
    else if (c == 'B') {
      Serial.println("\n*** CHANGING RX MASTER TO CHANNEL 97");
      setupMasterRadio(97, PA_MIN); 
    } 
    else if (c == 'C') {
      Serial.println("\n*** CHANGING TX RADIO 5 TO CHANNEL 97");
      send_ping_cmd(chan_chg, 97, 5, PA_MIN);
    } 
    else if (c == 'D') {
      Serial.println("\n*** CHANGING INCOMING TX PA TO LOW");
      send_ping_cmd(pa_chg, chan, rad, PA_LOW);
    } 
    else if (c == 'E') {
      Serial.println("\n*** CHANGING INCOMING TX PA TO MIN");
      send_ping_cmd(pa_chg, chan, rad, PA_MIN);
    } 
    else if (c == 'F') {
      Serial.println("\n*** CHANGING TX Radio 4 channel TO 76");
      //send_ping_cmd(rad_chg, 76, 4, PA_MIN);
    } 
    else if (c == 'G') {
      Serial.println("\n*** CHANGING TX Radio 5 channel TO 76");
      //send_ping_cmd(chan_chg, 76, 5, PA_MIN);
      //setupMasterRadio(76,PA_MIN); 
    } 
    else if (c == 'H') {
      Serial.println("\n*** CHANGING RX TO CH 76");
//      send_ping_cmd(rad_chg, 81, 5, PA_MIN);
      setupMasterRadio(76,PA_MIN); 
    } 
    else if (c == 'I') {
      Serial.println("\n*** CHANGING RX TO CH 81");
//      send_ping_cmd(rad_chg, 81, 3, PA_MIN);
      setupMasterRadio(81,PA_MIN); 
    } 
    else if (c == 'J') {
      Serial.println("\n*** CHANGING TX Radio to 4");
      //send_ping_cmd(rad_chg, 81, 4, PA_MIN);
    } else if (c == 'K') {
      Serial.println("\n*** CHANGING TX Radio to 5");
      //send_ping_cmd(rad_chg, 81, 5, PA_MIN);
    } 
    else if (c == 'L') {
      Serial.println("\n*** Initializing TX Radio to ch 81, rad 5");
      setupMasterRadio(76,PA_MIN); 
      delay(5000);     
      send_ping_cmd(rad_chg, 81, 5, PA_MIN);
      setupMasterRadio(81,PA_MIN); // channel number
    }
  }
}
