
/*     Arduino PIR Motion Sensor with
 *     nRF24L01 transceiver driver
 *     Arduino UNO
 *     
 *  by Jacob Romero, Creative Engineering Solutions LLC
 *  
 */

//========= Libraries =========//
//#include <K30_I2C.h>
#include <RF24.h>
#include <RF24_config.h>
#include <nRF24L01.h>
//#include <DHT.h>
//#include <DHT_U.h>
#include <printf.h>
#include <SPI.h>
#include "SPI.h"
#include <stdlib.h>
#include <stdio.h> 
#include <time.h>
//#include <Adafruit_Sensor.h>

//=============================
//======= nRF24L01 PINS =======
//=============================
#define MISO 12
#define MOSI 11
#define SCK 13
#define SS 10
#define CE 9

//==============================
//======= SEN-12642 PINS =======
//==============================
#define PIN_ANALOG_IN A0  // SEN-12642 Audio
#define AUDIO_GATE_IN 2   // SEN-12642 Gate
#define PIR_PIN 3 //PIR sensor input
//=============================

//=================================
//======= Custom Data types =======
//=================================

enum cmd_type {
    nada      = 0,   
    chan_chg  = 1, 
    rad_chg   = 2, 
    pa_chg    = 4
};

enum PA_type {
    PA_MIN  = 0,   
    PA_LOW  = 1, 
    PA_HIGH = 2, 
    PA_MAX  = 3
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
  cmd_type  my_cmd;
  int       ch_cmd;
  int       r_num;
  PA_type   pa_cmd;  
}myPing;

byte addresses[][6] = {"1Node","2Node","3Node","4Node","5Node","6Node"};

//=============================
//==== Instantiate modules ====
//=============================
RF24 radio(CE,SS); 
//K30_I2C k30_i2c = K30_I2C(0x68);
//DHT dht(DHTPIN, DHTTYPE);
//=============================

int   co2 = 0;
int   pir = 0;
int   gate = 0;
float h = 0;
float t = 0;
float f = 0;

int suradNum = 5;
int suchNum = 81;
PA_type supwrNum = PA_MIN;

int radNum;
int chNum;
PA_type pwrNum;

bool  intruderFlag = false;
bool  audioFlag = false; 
bool  suflag = false;
unsigned long clocktime;

cmd_type myCmd;
int chCmd;
int radCmd;
PA_type paCmd;

//// Used to store value sent by the TX
struct dataStruct SentMessage[1] = {0};

// Used to store ping sent by the Master to TX
struct pingCmd PingMessage[1] = {nada,1,0,PA_MIN};

//=========================
//========= Setup =========
//=========================                         
void setup() { 
  Serial.begin(9600);
  
  pinMode(SCK, OUTPUT);
  pinMode(MISO, INPUT);
  pinMode(MOSI, OUTPUT);
  pinMode(SS, OUTPUT); 
  pinMode(CE, OUTPUT);

  pinMode(AUDIO_GATE_IN, INPUT); // Pin 3
  pinMode(PIR_PIN, INPUT); // Pin 2 
  pinMode(PIN_ANALOG_IN,INPUT); // Pin A0

  initRadioValues(5, 81, PA_MIN);
  setupTXRadio(radNum,chNum,pwrNum); // (radio number, channel number) 

// 
//  if(!suflag) {
//    initRadioValues(5, 81, PA_MIN);
//    setupTXRadio(radNum,chNum,pwrNum); // (radio number, channel number) 
//    suflag = true; 
//  }   
}

//==========================
//======= Main Loop ========
//==========================

void loop() {   
  delay(10); 
  pir = digitalRead(PIR_PIN);
  if(pir == 1) intruderFlag = true;
  transmit_pir_state(intruderFlag);  
  
  gate = digitalRead(AUDIO_GATE_IN);
  if (gate == 1) audioFlag = true;
  transmit_audio_gate(audioFlag);  
  delay(10); 
  audioFlag = false;
    
  if (radio.available(addresses[0])) {
      radio.read(&PingMessage, sizeof(PingMessage));
      get_ping_cmd_RX();
      setupTXRadio(radNum,chNum,pwrNum);
  } 
}

//=======================================
//=== Get Ping Command from RX Master ===
//=======================================
void get_ping_cmd_RX() {      
    myCmd = PingMessage[0].my_cmd;
    switch (myCmd) {
      case 0: // change nada
          break;
      case 1: // chan_chg only
          chNum = PingMessage[0].ch_cmd;
          break;
      case 2: // rad_chg only
          //radNum = PingMessage[0].r_num;
          break;
      case 3: // chan_chg and rad_chg
          //chNum = PingMessage[0].ch_cmd;
          //radNum = PingMessage[0].r_num;
          break;
      case 4: // pa_chg only
          pwrNum = PingMessage[0].pa_cmd;
          break;
      case 5: // chan_chg and pa_chg
          chNum = PingMessage[0].ch_cmd;
          pwrNum = PingMessage[0].pa_cmd;
          break;
      case 6: // rad_chg and pa_chg
          //radNum = PingMessage[0].r_num;
          //pwrNum = PingMessage[0].pa_cmd;
          break;
      case 7: // change them all. CAUTION!!
          //chNum = PingMessage[0].ch_cmd;
          //radNum = PingMessage[0].r_num;
          //pwrNum = PingMessage[0].pa_cmd;
          break;
      default:
          break;
    }
    Serial.println("Ping Data: "+String(chNum)+" "+String(radNum)+" "+String(pwrNum));
}

//=============================
//=== Setup TX Radio Method ===
//=============================
void setupTXRadio(int rn,int mychannel,PA_type pat){
  radio.begin(); // Start the NRF24L01
  radio.setDataRate( RF24_250KBPS ); // RF24_1MBPS, RF24_2MBPS
  setPowerLevel(pat);  
//  radNum = rn;
//  chNum = mychannel;
//  pwrNum = pat; 
  //radio.setPALevel(RF24_PA_MIN); // MIN, LOW, HIGH, and MAX
  SentMessage[0].chNum = mychannel;
  SentMessage[0].radNum = rn;
  
// Open a writing and reading pipe on each radio, with 1Node as the reading pipe
  if(rn == 1){
    radio.openWritingPipe(addresses[rn]);
    radio.openReadingPipe(1,addresses[0]);
    radio.startListening(); // Initialize radio to RX mode
    radio.setChannel(mychannel);
    Serial.println("\nChannel: " + String(radio.getChannel()));
  }
  else if (rn == 2){    
    radio.openWritingPipe(addresses[rn]);
    radio.openReadingPipe(1,addresses[0]);
    radio.startListening(); // Initialize radio to RX mode
    radio.setChannel(mychannel);
    Serial.println("\nChannel: " + String(radio.getChannel()));
  }
  else if (rn == 3){    
    radio.openWritingPipe(addresses[rn]);
    radio.openReadingPipe(1,addresses[0]);
    radio.startListening(); // Initialize radio to RX mode
    radio.setChannel(mychannel);
    Serial.println("\nChannel: " + String(radio.getChannel()));
  }
  else if (rn == 4){    
    radio.openWritingPipe(addresses[rn]);
    radio.openReadingPipe(1,addresses[0]);
    radio.startListening(); // Initialize radio to RX mode
    radio.setChannel(mychannel);
    Serial.println("\nChannel: " + String(radio.getChannel()));
  }
  else if (rn == 5){    
    radio.openWritingPipe(addresses[rn]);
    radio.openReadingPipe(1,addresses[0]);
    radio.startListening(); // Initialize radio to RX mode
    radio.setChannel(mychannel);
    Serial.println("\nChannel: " + String(radio.getChannel()));
  }
  else{
    radio.openWritingPipe(addresses[0]);
    radio.openReadingPipe(1,addresses[1]);
    radio.startListening(); // Initialize radio to RX mode
  }
  checkRadioConn();
}

//======================================
//======= Check Radio Connection =======
//======================================
void checkRadioConn() {    
    printf_begin(); // Call this before calling printDetails()
    if (radio.isChipConnected()) {
      Serial.println("\nRadio Number "+ String(radNum) + " is connected to Channel: " + String(radio.getChannel()));
      radio.printDetails();  
      Serial.println("RF Comms Starting...");
    }else{
      Serial.println("\nRadio is not connected; showing Channel: " + String(radio.getChannel()));
      radio.printDetails();
    }  
}

//==================================
//======= Transmit PIR State =======
//==================================
void transmit_pir_state(bool sendFlag){
  if (sendFlag){ // If PIR is activated
    SentMessage[0].pir_state = digitalRead(PIR_PIN);
    SentMessage[0].audio_gate = 0;
    Serial.println("\nPIR Set State: "+String(digitalRead(PIR_PIN)));
    Serial.println("Sent Msg Radio: " + String(SentMessage[0].radNum));
    Serial.println("Channel: " + String(radio.getChannel()));
    radio.stopListening(); // Sets radio to TX mode
    radio.write(&SentMessage, sizeof(SentMessage));
    radio.openReadingPipe(1,addresses[0]);
    radio.startListening(); // Sets radio back to RX mode
    delay(7000); 
    intruderFlag = false; 
    SentMessage[0].pir_state = digitalRead(PIR_PIN);  
    Serial.println("\nSent Msg PIR State: "+String(SentMessage[0].pir_state));
  }
}

//===================================
//======= Transmit Audio Gate =======
//===================================
void transmit_audio_gate(bool agate){
  if (agate){ // If Audio Alarm is Activated
    SentMessage[0].audio_gate = 1;
    SentMessage[0].pir_state = 0;
    Serial.println("\nAudio Detected ");
    radio.stopListening(); // Sets radio to TX mode
    radio.write(&SentMessage, sizeof(SentMessage));
    radio.openReadingPipe(1,addresses[0]);
    radio.startListening(); // Sets radio back to RX mode
    delay(5000);
    audioFlag = false; 
    SentMessage[0].audio_gate = 0;    
    Serial.println("\nAudio Gate: "+String(SentMessage[0].audio_gate));
    delay(5000);   
    Serial.println("\n5 seconds, coast is clear...  "+String(audioFlag));
  }
}

//==========================
//======= Miscellany =======
//==========================
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

void initRadioValues(int rn,int mychannel,PA_type pat) {  
    radNum = rn;
    chNum = mychannel;
    pwrNum = pat;
}

void get_ping_cmd() {  
    myCmd = PingMessage[0].my_cmd;
    chNum = PingMessage[0].ch_cmd;
    radNum = PingMessage[0].r_num;
    pwrNum = PingMessage[0].pa_cmd;
    //setupTXRadio(radNum,chNum,pwrNum); // (radio number, channel number)
    //changeRadio(chCmd,radCmd,paCmd);
    Serial.println("Ping Data: "+String(chNum)+" "+String(radNum)+" "+String(pwrNum));
}
