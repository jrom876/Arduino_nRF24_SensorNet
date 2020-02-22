
/*     
 *     File: K30_nRF24_DHT_TX_X.X.20.ino
 *     K30 CO2 sensor
 *     nRF24L01 transceiver
 *     DHT11 Temp/Humid sensor
 *     Arduino MEGA, UNO or NANO
 *     
 *     Written by:   Jacob Romero  
 *     email:        admin@jrom.io 
 *     Creative Engineering Solutions LLC 
 *     
 *   This module is the stable version of the transmitter (TX) in a 
 *   TX/RX pair. The data is transmitted continuously by the 
 *   TX module's nRF24L01 (2.4 GHz transceiver), which
 *   transmits four 32-bit packets of data from 2 sensors: 
 *   the DHT-11 (temp and humid), and the K30 (CO2). 
 *   This is a development version only which may change w/o 
 *   notice, and which may contain commented and uncommented 
 *   development code.      
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
#include "SPI.h"
#include <stdio.h> 

struct dataStruct{
  char  type;
  int   value;
}myData;

// Define Arduino UNO pin I/Os
#define SCK 13
#define MISO 12
#define MOSI 11
#define SS 10
#define CE 9
#define speakerOut    4 // Test output
#define DHTPIN        2 // DHT Pin

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

//Validation byte (future use)
#define val(n) (n==0)?0xCC:0xAA 

//Shock Burst Packet (future use)
#define txPreamble(v) (v==0)? 0xAA:0x55

// Receiver address
const uint32_t pipe1 = 0xC6C6C6C0;

//=============================
//==== Instantiate modules ====
//=============================
RF24 radio(CE,SS);  // NRF24L01 uses SPI pins + Pin 9 and 10 on the UNO
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

// Used to store value received by the NRF24L01
uint32_t SentMessage[1] = {0};

//=========================
//========= Setup =========
//=========================                         
void setup() { 
  Serial.begin(9600);
  dht.begin(); 
  pinMode(speakerOut, OUTPUT);
  
  pinMode(CE, OUTPUT);
  pinMode(MISO, INPUT);
  pinMode(MOSI, OUTPUT);
  pinMode(SCK, OUTPUT);
  pinMode(SS, OUTPUT);

  radio.begin(); // Start the NRF24L01
  radio.setDataRate( RF24_250KBPS );  
  //radio.setDataRate( RF24_1MBPS );  
  radio.openWritingPipe(pipe1); // Get NRF24L01 ready to transmit
  radio.setPALevel(RF24_PA_MIN); // MIN, LOW, HIGH, and MAX
  radio.stopListening(); // Sets radio to Transmitter mode
  Serial.println("RF Comms Starting...");
}

//==========================
//======= Main Loop ========
//==========================
void loop() {
  readDHT();
  delay(2000);
  digitalWrite(speakerOut, LOW); // The Piezo buzzer, LOW == Off

  rc = k30_i2c.readCO2(co2);
  int co2Value = co2;
  if (rc == 0) { 
    validCO2Flag = true;       
//    Serial.print("CO2: ");
//    Serial.println(co2+" ppm");
    sendMessage(validCO2Flag);
    Serial.println("Message sent");
    digitalWrite(speakerOut, HIGH); // Piezo HIGH == On
    delay(500);
    digitalWrite(speakerOut, LOW);
  }
  else{
    validCO2Flag = false;
    digitalWrite(speakerOut, LOW); 
  } 
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
    SentMessage[0] = 0;
    radio.write(SentMessage, 1);
  }
}

//================================
//======= Transmit Methods =======
//================================
  void transmit_co2(void){
    SentMessage[0] = co2 - sensorOffset;
    Serial.print("CO2: ");
    Serial.println(String(co2)+" ppm");
    myData.type = 'o';
    myData.value = co2;
    //Serial.println(SentMessage[0]+"ppm");
    radio.write(&SentMessage, sizeof(SentMessage));
    //delay(2000);
  }
//================================
  void transmit_h(void){
    SentMessage[0] = h;
    Serial.print(F("Humidity: "));
    Serial.println(String(h)+"%");
    myData.type = 'u';
    myData.value = h;
    //Serial.println(SentMessage[0]+"%");
    radio.write(&SentMessage, sizeof(SentMessage));
    //delay(2000);
  }  
//===============================
  void transmit_t(void){
    SentMessage[0] = t;
    Serial.print(F("Temp: "));
    Serial.println(String(t)+"C");
    myData.type = 'l';
    myData.value = t;
    //Serial.println(SentMessage[0]+"C");
    radio.write(&SentMessage, sizeof(SentMessage));
    //delay(2000);
  }
//===============================
  void transmit_f(void){
    SentMessage[0] = f;
    Serial.print(F("Temp: "));
    Serial.println(String(f)+"F");
    myData.type = 'r';
    myData.value = f;
    //Serial.println(SentMessage[0]+"F");
    radio.write(&SentMessage, sizeof(SentMessage));
    //delay(2000);
  }
//===============================
  void transmit_hif(void){
    SentMessage[0] = hif;
    Serial.print(F("Heat Index: "));
    Serial.println(String(hif)+"F");
    myData.type = 'i';
    myData.value = hif;
    //Serial.println(SentMessage[0]+"F");
    radio.write(&SentMessage, sizeof(SentMessage));
    //delay(2000);    
  }
//===============================
  void transmit_hic(void){
    SentMessage[0] = hic;
    Serial.print(F("Heat Index: "));
    Serial.println(String(f)+"C");
    myData.type = 'j';
    myData.value = hic;
    //Serial.println(SentMessage[0]+"C");
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
  
