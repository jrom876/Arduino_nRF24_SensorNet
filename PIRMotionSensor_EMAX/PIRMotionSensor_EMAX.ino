
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
    nada     = 0,   
    chan_chg = 1, 
    pa_chg   = 2, 
    rad_chg  = 4
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
bool  intruderFlag = false;
bool  audioFlag = false;
unsigned long clocktime;
cmd_type myCmd = nada;
int chCmd = 0;
int radCmd;
PA_type paCmd;

//// Used to store value sent by the TX
struct dataStruct SentMessage[1] = {0};

// Used to store ping sent by the Master to TX
struct pingCmd PingMessage[1] = {nada,0,0,PA_MIN};

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
  
  setupTXRadio(5,81); // (radio number, channel number)  
 
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
  receive_data();
}

void receive_data() {  
    if (radio.available(addresses[0])) {
      radio.read(&PingMessage, sizeof(PingMessage)); 
      myCmd = PingMessage[0].my_cmd;
      chCmd = PingMessage[0].ch_cmd;
      radCmd = PingMessage[0].r_num;
      paCmd = PingMessage[0].pa_cmd;
      //setupTXRadio(r_num,81); // (radio number, channel number)
      Serial.println("Ping: "+ String(myCmd)+" "+String(chCmd)+" "+String(radCmd)+" "+String(paCmd));
      changeRadio(chCmd,radCmd,paCmd);
    }
}

void changeRadio(int chan, int rad, PA_type pwr) {
  chan = ((chan <= 255) && (chan >= 0))?chan:255;
  rad = ((rad <= 5) && (rad >= 0))?rad:0;
  setupTXRadio(rad,chan); // (radio number, channel number)
  setPowerLevel(pwr);
  Serial.println("Changes: "+ String(myCmd)+" "+String(chCmd)+" "+String(radCmd)+" "+String(paCmd));
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
        //
        break;
  }
}

//==========================
//=== Setup Radio Method ===
//==========================
void setupTXRadio(int rn, int mychannel){
  radio.begin(); // Start the NRF24L01
  radio.setDataRate( RF24_250KBPS ); // RF24_1MBPS, RF24_2MBPS   
  radio.setPALevel(RF24_PA_MIN); // MIN, LOW, HIGH, and MAX
  SentMessage[0].chNum = mychannel;
  SentMessage[0].radNum = rn;
  
// Open a writing and reading pipe on each radio, with 1Node as the reading pipe
  if(rn == 1){
    radio.openWritingPipe(addresses[rn]);
    radio.openReadingPipe(1,addresses[0]);
    //radio.startListening(); // Initialize radio to RX mode
    radio.setChannel(mychannel);
    Serial.println("\nChannel: " + String(radio.getChannel()));
  }
  else if (rn == 2){    
    radio.openWritingPipe(addresses[rn]);
    radio.openReadingPipe(1,addresses[0]);
    //radio.startListening(); // Initialize radio to RX mode
    radio.setChannel(mychannel);
    Serial.println("\nChannel: " + String(radio.getChannel()));
  }
  else if (rn == 3){    
    radio.openWritingPipe(addresses[rn]);
    radio.openReadingPipe(1,addresses[0]);
    //radio.startListening(); // Initialize radio to RX mode
    radio.setChannel(mychannel);
    Serial.println("\nChannel: " + String(radio.getChannel()));
  }
  else if (rn == 4){    
    radio.openWritingPipe(addresses[rn]);
    radio.openReadingPipe(1,addresses[0]);
    //radio.startListening(); // Initialize radio to RX mode
    radio.setChannel(mychannel);
    Serial.println("\nChannel: " + String(radio.getChannel()));
  }
  else if (rn == 5){    
    radio.openWritingPipe(addresses[rn]);
    radio.openReadingPipe(1,addresses[0]);
    //radio.startListening(); // Initialize radio to RX mode
    radio.setChannel(mychannel);
    Serial.println("\nChannel: " + String(radio.getChannel()));
  }
  else{
    radio.openWritingPipe(addresses[0]);
    radio.openReadingPipe(1,addresses[1]);
    radio.startListening(); // Initialize radio to RX mode
  }  
  Serial.println("RF Comms Starting...");
}

//==================================
//======= Transmit PIR State =======
//==================================
void transmit_pir_state(bool sendFlag){
  if (sendFlag){ // If PIR is activated
    SentMessage[0].pir_state = 1;
    SentMessage[0].audio_gate = 0;
    Serial.println("\nPIR Set State: "+String(SentMessage[0].pir_state));
    Serial.println("Radio: " + String(SentMessage[0].radNum));
    Serial.println("Channel: " + String(radio.getChannel()));
    radio.stopListening(); // Sets radio to TX mode
    radio.write(&SentMessage, sizeof(SentMessage));
    radio.startListening(); // Sets radio back to RX mode
    delay(3000); 
    intruderFlag = false; 
    SentMessage[0].pir_state = 0;  
    Serial.println("\nPIR State: "+String(SentMessage[0].pir_state));
  }
}

//===================================
//======= Transmit Audio Flag =======
//===================================
void transmit_audio_gate(bool agate){
  if (agate){ // If Audio Alarm is Activated
    SentMessage[0].audio_gate = 1;
    SentMessage[0].pir_state = 0;
    Serial.println("\nAudio Detected ");
    radio.stopListening(); // Sets radio to TX mode
    radio.write(&SentMessage, sizeof(SentMessage));
    radio.startListening(); // Sets radio back to RX mode
    delay(3000);
    audioFlag = false; 
    SentMessage[0].audio_gate = 0;    
    Serial.println("\nAudio Gate: "+String(SentMessage[0].audio_gate));
  }
} 
