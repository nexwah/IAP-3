/**************
 * Front Motor - Motor 1
 * Back Motor - Motor 2
 * Side Motor - Motor 3(left), Motor 4(right)
 * Back Fans - FAN1(left), Fan2(right)
 * Side Fans - Fan3(left), Fan4(right)
 */

#include <Wire.h>
#include "esp_system.h"
#include <HardwareSerial.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include "Arduino.h"
#include "PCF8575.h"
#include "SPI.h"
#include <Time.h>
#include <TimeLib.h>
#include <esp_now.h>
#include <WiFi.h>

//define
HardwareSerial MySerial2(2);
PCF8575 pcf8575(0x26);
#define motor_PUL 4
#define motor1_ENA P16
#define motor1_DIR P17
#define motor2_ENA P14
#define motor2_DIR P15
#define motor3_ENA P12
#define motor3_DIR P13
#define motor4_ENA P10
#define motor4_DIR P11
#define relay1 P07
#define relay2 P06
#define relay3 P05
#define relay4 P04
#define relay5 P03
#define INH P02
#define ADDA P01
#define ADDB P00
hw_timer_t * timer0 = NULL;
hw_timer_t * timer1 = NULL;
hw_timer_t * timer2 = NULL;

//variable
bool deviceConnected = false;
bool receivedFlag = false;
bool workingmoderunning = false;
char ble_receivedByte[20] = {};
int testmode = 1; //1 is testmode, 0 is working mode
int regenerationmode = 1; // 1 is regeration mode, 0 is absorption mode
int timer0_Flag = 0; //check timer triggered?
int timer1_Flag = 1; //check timer triggered?
int timer2_Flag = 0; 
int setTimer2 = 0;
int AbsorptionTime = 600; //seconds
int RegenerationTime = 3600; //seconds
int datalogTime = 10; //seconds
int relayTime = 60; //seconds
int cnt = 2;
int relaycase = 0;

uint16_t sensordata1 = 0;
uint16_t sensordata2 = 0;
uint16_t sensordata3 = 0;
uint16_t sensordata4 = 0;
int highest = 400;
unsigned long premillis = 0;

BLEScan* pBLEScan;

typedef struct struct_message {
  int id = 0;
  int sensordata = 0;
} struct_message;

struct_message myData;

//function
void PCF8575_Setup();
void RELAY_Call(int relay, int start);

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&myData, incomingData, sizeof(myData));
  if(myData.id == 1){
    sensordata1 = myData.sensordata;
  }
  if(myData.id == 2){
    sensordata2 = myData.sensordata;
  }
  if(myData.id == 3){
    sensordata3 = myData.sensordata;
  }
  if(myData.id == 4){
    sensordata4 = myData.sensordata;
  }
}

void ESP_NOW_Setup(){
  Serial.println("ESP_NOW_Setup");
  WiFi.mode(WIFI_STA);
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  //register the callback function that will be called when a message is received.
  esp_now_register_recv_cb(OnDataRecv);
}

//setup
void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 NITTO 2 PROGRAM START");
  MySerial2.begin(9600, SERIAL_8N1, 16, 17); //RX, TX, SubG
  //BLE_Setup();
  ESP_NOW_Setup();
  PCF8575_Setup(); //IO_Control
  premillis = millis();
}

//loop
void loop() {
  if(MySerial2.available()){
    char c = MySerial2.read();
    if(c == 'R'){
      char command = MySerial2.read();
      if(command == '7'){
        Serial.println("R7");
        MySerial2.print("R7");
        RELAY_Call(relay1,1);
      }
      if(command == '8'){
        Serial.println("R8");
        MySerial2.print("R8");
        RELAY_Call(relay1,0);
      }
      if(command == '9'){
        Serial.println("R9");
        MySerial2.print("R9");
        RELAY_Call(relay2,1);
      }
      if(command == '0'){
        Serial.println("R0");
        MySerial2.print("R0");
        RELAY_Call(relay2,0);
      }
    }
  }
  if(millis()-premillis>=5000){
    premillis = millis();
    //BLE();
    char buf[20];
    //sprintf(buf,"I%d,%d,%d,%dN",sensordata1,sensordata2,sensordata3,sensordata4);
    sprintf(buf,"I%d,%d,%d,%dN",sensordata1,sensordata2,sensordata3,sensordata4);
    MySerial2.print(buf); Serial.println(buf);
    sensordata1=0; sensordata2=0; sensordata3=0; sensordata4=0;
  }
}

int max4(int a, int b, int c, int d)
{
   int maxguess;
   maxguess = max(a,b);  // biggest of A and B
   maxguess = max(maxguess, c);  // but maybe C is bigger?
   maxguess = max(maxguess, d);  // but maybe C is bigger?
   return(maxguess);
}

void BLE(){
    BLEScanResults foundDevices = pBLEScan->start(1, false);
    pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory
    //highest = max4(sensordata1,sensordata2,sensordata3,sensordata4);
    /*Serial.print("C02-01:");Serial.print(sensordata1);
    Serial.print(", C02-02:");Serial.print(sensordata2);
    Serial.print(", C02-03:");Serial.print(sensordata3);
    Serial.print(", C02-04:");Serial.println(sensordata4); 
    Serial.print("Highest:");Serial.println(highest);*/  
}

void PCF8575_Setup(){
  Serial.println("PCF8575(MOTOR,RLY,TACH) Setup Function");
  pinMode(motor_PUL,OUTPUT);
  pcf8575.pinMode(motor1_ENA,OUTPUT);
  pcf8575.pinMode(motor1_DIR,OUTPUT);
  pcf8575.pinMode(motor2_ENA,OUTPUT);
  pcf8575.pinMode(motor2_DIR,OUTPUT);
  pcf8575.pinMode(motor3_ENA,OUTPUT);
  pcf8575.pinMode(motor3_DIR,OUTPUT);
  pcf8575.pinMode(motor4_ENA,OUTPUT);
  pcf8575.pinMode(motor4_DIR,OUTPUT);
  pcf8575.pinMode(relay1,OUTPUT);
  pcf8575.pinMode(relay2,OUTPUT);
  pcf8575.pinMode(relay3,OUTPUT);
  pcf8575.pinMode(relay4,OUTPUT);
  pcf8575.pinMode(relay5,OUTPUT);
  pcf8575.pinMode(INH,OUTPUT);
  pcf8575.pinMode(ADDA,OUTPUT);
  pcf8575.pinMode(ADDB,OUTPUT);
  pcf8575.begin();

  digitalWrite(motor_PUL,LOW);
  pcf8575.digitalWrite(motor1_ENA,LOW);
  pcf8575.digitalWrite(motor1_DIR,LOW);
  pcf8575.digitalWrite(motor2_ENA,LOW);
  pcf8575.digitalWrite(motor2_DIR,LOW);
  pcf8575.digitalWrite(motor3_ENA,LOW);
  pcf8575.digitalWrite(motor3_DIR,LOW);
  pcf8575.digitalWrite(motor4_ENA,LOW);
  pcf8575.digitalWrite(motor4_DIR,LOW);
  pcf8575.digitalWrite(relay1,LOW);
  pcf8575.digitalWrite(relay2,LOW);
  pcf8575.digitalWrite(relay3,LOW);
  pcf8575.digitalWrite(relay4,LOW);
  pcf8575.digitalWrite(relay5,LOW);
  pcf8575.digitalWrite(INH,LOW);
  pcf8575.digitalWrite(ADDA,LOW);
  pcf8575.digitalWrite(ADDB,LOW);

  delay(1000);
  Serial.println("PCF8575 Setup Done");
}

void RELAY_Call(int relay, int start){ //start:0, relay ON ; start:1, relay OFF
  if(start){
    Serial.println("RELAY ON");
    pcf8575.digitalWrite(relay,HIGH);
  } else{
    Serial.println("RELAY OFF");
    pcf8575.digitalWrite(relay,LOW);
  }
}
