
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
#define IRQ_GATE_IN  0    // 
#define PIR_PIN 3 //PIR sensor input
//=============================

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

//// Used to store value sent by the TX
struct dataStruct SentMessage[1] = {0};

//==========================================
//======= Interrupt Service Routines =======
//==========================================
void soundISR() {
    audioFlag = true;
//    transmit_audio_gate(audioFlag);
//    audioFlag = false;  
//    delay(4000);  
}

void pirISR() {  
    //pir = digitalRead(PIR_PIN);
    intruderFlag = true;
//    transmit_pir_state(intruderFlag);
//    intruderFlag = false;
    delay(4000);
}

//// See: https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/
//// attachInterrupt(digitalPinToInterrupt(pin), ISR, mode)
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
  attachInterrupt(digitalPinToInterrupt(IRQ_GATE_IN), soundISR, RISING);
  attachInterrupt(digitalPinToInterrupt(PIR_PIN), pirISR, RISING);
  pinMode(PIR_PIN, INPUT); // Pin 2
  
  setupTXRadio(5,76); // (radio number, channel number)  
 
}

//==========================
//======= Main Loop ========
//==========================

void loop() {
  delay(10); 
  pir = digitalRead(PIR_PIN);
  if(pir == 1) intruderFlag - true;
  transmit_pir_state(intruderFlag);
  
  gate = digitalRead(AUDIO_GATE_IN);
  if (gate == 1) audioFlag = true;
  transmit_audio_gate(audioFlag);
//  if(audioFlag) soundISR();
  //soundISR();
  //delay(1000);    
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
    radio.openWritingPipe(addresses[1]);
    radio.openReadingPipe(1,addresses[0]);
    //radio.startListening(); // Initialize radio to RX mode
    radio.setChannel(mychannel);
    Serial.println("\nChannel: " + String(radio.getChannel()));
  }
  else if (rn == 2){    
    radio.openWritingPipe(addresses[2]);
    radio.openReadingPipe(1,addresses[0]);
    //radio.startListening(); // Initialize radio to RX mode
    radio.setChannel(mychannel);
    Serial.println("\nChannel: " + String(radio.getChannel()));
  }
  else if (rn == 3){    
    radio.openWritingPipe(addresses[3]);
    radio.openReadingPipe(1,addresses[0]);
    //radio.startListening(); // Initialize radio to RX mode
    radio.setChannel(mychannel);
    Serial.println("\nChannel: " + String(radio.getChannel()));
  }
  else if (rn == 4){    
    radio.openWritingPipe(addresses[4]);
    radio.openReadingPipe(1,addresses[0]);
    //radio.startListening(); // Initialize radio to RX mode
    radio.setChannel(mychannel);
    Serial.println("\nChannel: " + String(radio.getChannel()));
  }
  else if (rn == 5){    
    radio.openWritingPipe(addresses[5]);
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
//    SentMessage[0].co2_val = co2;
//    SentMessage[0].h_val = h;
//    SentMessage[0].c_val = t;
//    SentMessage[0].f_val = f;
  if (sendFlag){ // If PIR is activated
    SentMessage[0].pir_state = 1;
    SentMessage[0].audio_gate = 0;
    Serial.println("\nPIR Set State: "+String(SentMessage[0].pir_state));
    Serial.println("Radio: " + String(SentMessage[0].radNum));
    Serial.println("Channel: " + String(radio.getChannel()));
    radio.stopListening(); // Sets radio to TX mode
    radio.write(&SentMessage, sizeof(SentMessage));
    //radio.openReadingPipe(1,addresses[0]);
    //radio.startListening(); // Sets radio back to RX mode 
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
    radio.openReadingPipe(1,addresses[0]);
    radio.startListening(); // Sets radio back to RX mode
    delay(3000);
    audioFlag = false; 
    SentMessage[0].audio_gate = 0;    
    Serial.println("\nAudio Gate: "+String(SentMessage[0].audio_gate));
  }
} 
