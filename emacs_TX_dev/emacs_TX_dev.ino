
/*     
 *     File: emacs_TX_dev.ino
 *     K30 CO2 sensor
 *     nRF24L01 transceiver
 *     DHT11 Temp/Humid sensor
 *     Arduino MEGA, UNO or NANO
 *     
 *     Written by:   Jacob Romero  
 *     email:        admin@jrom.io 
 *     Creative Engineering Solutions LLC 
 *     
 *   This module is the dev version of the transmitter (TX) in a 
 *   TX/RX pair. The data is transmitted continuously by the 
 *   TX module's nRF24L01 (2.4 GHz transceiver), which
 *   transmits four 32-bit packets of data from 2 sensors: 
 *   the DHT-11 (temp and humid), and the K30 (CO2). 
 *   This is a development version only which may change w/o 
 *   notice, and which may contain commented and uncommented 
 *   development code.      
 *   
 *   https://github.com/FirstCypress/K30_CO2_I2C_Arduino
 */
 
//========= Libraries =========//
// NRF24L01 library created by TMRh20 https://github.com/TMRh20/RF24
#include <K30_I2C.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <RF24_config.h>
#include <DHT.h>
#include <DHT_U.h>
#include <printf.h>
#include <SPI.h>
#include "SPI.h"
#include <stdlib.h>
#include <stdio.h> 
#include <Adafruit_Sensor.h>

struct dataStruct{
  int   type;
  int   value;
}myData;

// Define Arduino UNO pin I/Os
#define SCK 13
#define MISO 12
#define MOSI 11
#define SS 10
#define CE 9
#define speakerOut    4 // Test output
#define DHTPIN        8 // DHT Pin

// Enable for MEGA
//#define CE 49
//#define MISO 50
//#define MOSI 51
//#define SCK 52
//#define SS 53

// Uncomment whatever type of DHT you're using!
#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

// Receiver address
const uint32_t pipe1 = 0xC6C6C6C0;

byte addresses[][6] = {"1Node","2Node","3Node","4Node"};

// Used to store value received by the NRF24L01
//uint32_t SentMessage[1] = {0};
  
//=============================
//==== Instantiate modules ====
//=============================
RF24 radio(CE,SS); 
K30_I2C k30_i2c = K30_I2C(0x68);
DHT dht(DHTPIN, DHTTYPE);
//=============================

int co2 = 0;
int rc  = 1;
int sensorOffset = 0;
float h = 0;
float t = 0;
float f = 0;
float hif = 0;
float hic = 0;
bool validCO2Flag = false;

struct dataStruct SentMessage[1] = {0};
//uint32_t SentMessage[1] = {0};

//=========================
//========= Setup =========
//=========================                         
void setup() { 
  Serial.begin(9600);
  dht.begin();
  SPI.begin(); 
  pinMode(speakerOut, OUTPUT);
  
  pinMode(SCK, OUTPUT);
  pinMode(MISO, INPUT);
  pinMode(MOSI, OUTPUT);
  pinMode(SS, OUTPUT); 
  pinMode(CE, OUTPUT);

  setupTXRadio(1,76); // (radio number, channel number)  
}

//==========================
//======= Main Loop ========
//==========================
void loop() {
  readDHT();
  delay(2000);
  rc = k30_i2c.readCO2(co2);
  //int co2Value = co2;
  SentMessage[0].type = 1;
  SentMessage[0].value = co2;
  if (rc == 0) { 
    validCO2Flag = true;       
//    Serial.print("CO2: ");
//    Serial.println(String(SentMessage[0].value) +" ppm");      
//    Serial.print("Type: ");
//    Serial.println(String(SentMessage[0].type));
    sendMessage(validCO2Flag);    
    Serial.println("Channel inside main: " + String(radio.getChannel()));
    radio.write(&SentMessage, sizeof(SentMessage));
    Serial.println("Message sent");
  }
  else{
    validCO2Flag = false;
    digitalWrite(speakerOut, LOW); 
  }  
}

//==========================
//=== Setup Radio Method ===
//==========================
void setupTXRadio(int rn, int mychannel){
  radio.begin(); // Start the NRF24L01
  radio.setDataRate( RF24_250KBPS );  
  //radio.setDataRate( RF24_1MBPS );  
  //radio.openWritingPipe(pipe1);
  //radio.openWritingPipe(addresses[1]); // Get NRF24L01 ready to transmit
  radio.setPALevel(RF24_PA_MIN); // MIN, LOW, HIGH, and MAX
  //radio.setChannel(0);
  //Serial.println("Channel: " + String(radio.getChannel()));
  
// Open a writing and reading pipe on each radio, with 1Node as the receive pipe
  if(rn == 1){
    radio.openWritingPipe(addresses[1]);
    radio.openReadingPipe(1,addresses[0]);
    radio.setChannel(mychannel);
    Serial.println("\nChannel: " + String(radio.getChannel()));
  }
  else if (rn == 2){    
    radio.openWritingPipe(addresses[2]);
    radio.openReadingPipe(1,addresses[0]);
    radio.setChannel(12);
    Serial.println("\nChannel: " + String(radio.getChannel()));
  }else if (rn == 3){    
    radio.openWritingPipe(addresses[3]);
    radio.openReadingPipe(1,addresses[0]);
    radio.setChannel(23);
    Serial.println("\nChannel: " + String(radio.getChannel()));
  }else if (rn == 4){    
    radio.openWritingPipe(addresses[4]);
    radio.openReadingPipe(1,addresses[0]);
    radio.setChannel(34);
    Serial.println("\nChannel: " + String(radio.getChannel()));
  }
  else{
    radio.openWritingPipe(addresses[0]);
    radio.openReadingPipe(1,addresses[1]);
  }
  //radio.startListening();
  radio.stopListening(); // Sets radio to Transmitter mode
  Serial.println("RF Comms Starting...");
}

//===========================
//=== Send Message Method ===
//===========================
void sendMessage(bool validCO2Flag){
  if (validCO2Flag){ // If we get a valid CO2 reading
      transmit_co2();
      delay(2000);
      transmit_h();
      delay(2000);
      transmit_t();
      delay(2000);
      transmit_f();
      delay(2000);
  }
  else {
//    SentMessage[0] = 0;
    radio.write(SentMessage, 1);
  }
}

//================================
//======= Transmit Methods =======
//================================
  void transmit_co2(void){
    SentMessage[0].type = 1;
    SentMessage[0].value = co2;
    Serial.println("CO2 Value: "+String(SentMessage[0].value)+" ppm");
    Serial.println("CO2 Type: "+String(SentMessage[0].type));
    radio.write(&SentMessage, sizeof(SentMessage));
    //delay(2000);
  }
//================================
  void transmit_h(void){
    SentMessage[0].type = 2;
    SentMessage[0].value = h;
    Serial.print(F("Humidity: "));
    Serial.println(String(SentMessage[0].value)+"%  ");
    Serial.print("Humidity Type: ");
    Serial.println(String(SentMessage[0].type));
    radio.write(&SentMessage, sizeof(SentMessage));
    //delay(2000);
  }  
//===============================
  void transmit_t(void){
    SentMessage[0].type = 3;
    SentMessage[0].value = t;
    Serial.print(F("Temp: "));
    Serial.println(String(SentMessage[0].value)+" degrees C ");
    Serial.print("Temp Type: ");
    Serial.println(String(SentMessage[0].type));
    radio.write(&SentMessage, sizeof(SentMessage));
    //delay(2000);
  }
//===============================
  void transmit_f(void){
    SentMessage[0].type = 4;
    SentMessage[0].value = f;
    Serial.print(F("Temp: "));
    Serial.println(String(SentMessage[0].value)+" degrees F ");
    Serial.print("Temp Type: ");
    Serial.println(String(SentMessage[0].type));
    //Serial.println(myData+"F");
    radio.write(&SentMessage, sizeof(SentMessage));
    //delay(2000);
  }
//===============================
  void transmit_hif(void){
//    myData = hif;
    Serial.print(F("Heat Index: "));
    Serial.println(String(hif)+"F");
    SentMessage[0].type = 5;
    SentMessage[0].value = hif;
    //Serial.println(myData+"F");
    radio.write(&SentMessage, sizeof(SentMessage));
    //delay(2000);    
  }
//===============================
  void transmit_hic(void){
//    myData = hic;
    Serial.print(F("Heat Index: "));
    Serial.println(String(f)+"C");
    SentMessage[0].type = 6;
    SentMessage[0].value = hic;
    //Serial.println(myData+"C");
    radio.write(&SentMessage, sizeof(SentMessage));
    //delay(2000);    
  }
//=========================================
//===========  Read DHT Method  ===========
//=========================================
 void readDHT(void) {
  // Wait a few seconds between measurements.
  delay(2000);

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));   
  }

  // Compute heat index in Fahrenheit (the default)
  hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  hic = dht.computeHeatIndex(t, h, false);

  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.print(F("°C "));
  Serial.print(f);
  Serial.print(F("°F  Heat index: "));
  Serial.print(hic);
  Serial.print(F("°C "));
  Serial.print(hif);
  Serial.println(F("°F"));  
  }
//===============================
  
//  radio.begin(); // Start the NRF24L01
//  radio.setDataRate( RF24_250KBPS );  
//  //radio.setDataRate( RF24_1MBPS );  
//  //radio.openWritingPipe(pipe1);
//  //radio.openWritingPipe(addresses[1]); // Get NRF24L01 ready to transmit
//  radio.setPALevel(RF24_PA_MIN); // MIN, LOW, HIGH, and MAX
//  //radio.setChannel(0);
//  //Serial.println("Channel: " + String(radio.getChannel()));
//  
//// Open a writing and reading pipe on each radio, with 1Node as the receive pipe
//  if(radioNumber == 1){
//    radio.openWritingPipe(addresses[1]);
//    radio.openReadingPipe(1,addresses[0]);
//    radio.setChannel(16);
//    Serial.println("Channel: " + String(radio.getChannel()));
//  }
////  else if (radioNumber == 2){    
////    radio.openWritingPipe(addresses[2]);
////    radio.openReadingPipe(1,addresses[0]);
////    //radio.setChannel(2);
////  }else if (radioNumber == 3){    
////    radio.openWritingPipe(addresses[3]);
////    radio.openReadingPipe(1,addresses[0]);
////    //radio.setChannel(3);
////  }else if (radioNumber == 4){    
////    radio.openWritingPipe(addresses[4]);
////    radio.openReadingPipe(1,addresses[0]);
////    //radio.setChannel(4);
////  }
//  else{
//    radio.openWritingPipe(addresses[0]);
//    radio.openReadingPipe(1,addresses[1]);
//  }
//  //radio.startListening();
//  radio.stopListening(); // Sets radio to Transmitter mode
//  Serial.println("RF Comms Starting...");
