#include <nRF24L01.h>
#include <RF24.h>
#include <RF24_config.h>

#include <SPI.h>

#include <EEPROM.h>

// Example testing sketch for various DHT humidity/temperature sensors
// Written by ladyada, public domain

//#include "DHT.h"
#include <PCF8814.h> 
#include <math.h>


//#define DHTPIN 3     // what digital pin we're connected to

#define motorustart A1
#define button A2
#define testled A3
#define motoru 9
#define boiler 4
#define light 3
#define termistor 2

//PCF8814 Lcd(13,11,10,6); // LCD sets SPI SCLK: 7 pin, SDA: 8 pin, CS: 9 pin. RESET: 6 pin
PCF8814 Lcd(7,8,5,6);
//PCF8814 Lcd(7,8,9,6);origin soft

bool work = false;

float sumTemperature = 0.0;
float temperature = 0.0;
float Temp;// = 20.8;
float gisteresis = 0.05;
float mintemp = 50;
float maxtemp = 0;
int mtime = 0;
int savetime = 0;
int counter = 0;
String ltime;
long previousMillis = 0;
long buttonMillis = 0; 
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
const uint64_t pipe = 0xF0F1F2F3F4LL; // индитификатор передачи, "труба"
RF24 radio(8, 9); // CE, CSN

//DHT dht(DHTPIN, DHTTYPE);

void setup() {
  
  radio.begin();
  delay(2);
  radio.setChannel(9); // канал (0-127)
  radio.setDataRate(RF24_1MBPS); 
  radio.setPALevel(RF24_PA_HIGH);   

  radio.openWritingPipe(pipe); // открываем трубу на передачу.
  delay(1000);
  
  Lcd.Init();
  delay(100);
  Lcd.Mirror(1,1);
  //Lcd.GotoXY(4,4);
  //Lcd.Print("Loading...");
  
  Serial.begin(115200);
  //Serial.println("Hello.. i termostat.");

  
  //EEPROM_float_write(0, 20.8);
  //EEPROM.write(0, 20.80);
  Temp = EEPROM_float_read(0);
  
  delay(100);
  pinMode(boiler, OUTPUT);
  pinMode(testled, OUTPUT);
  pinMode(button, INPUT);
  //pinMode(motoru, OUTPUT);
  //pinMode(light, OUTPUT);
  pinMode(termistor, OUTPUT);
  digitalWrite(light, HIGH);
  digitalWrite(termistor, HIGH);
  //digitalWrite(testled, HIGH);
  //dht.begin();
  Lcd.Clear();
  delay(100);
  draw();
  //motor(256);
  //delay(1000);
  //motor(190);
  //delay(1000);
  motor(255);
}

 // создаем метод для перевода показаний сенсора в градусы Цельсия 
double Getterm(int RawADC) {
  double temp;
  temp = log(((10230000/RawADC) - 10000));
  temp = 1 / (0.001129148 + (0.000234125 * temp) + (0.0000000876741 * temp * temp * temp));
  temp = temp - 273.15;
  return temp;
}

void EEPROM_float_write(int addr, float val) // запись в ЕЕПРОМ
{  
  byte *x = (byte *)&val;
  for(byte i = 0; i < 4; i++) EEPROM.write(i+addr, x[i]);
}

float EEPROM_float_read(int addr) // чтение из ЕЕПРОМ
{    
  byte x[4];
  for(byte i = 0; i < 4; i++) x[i] = EEPROM.read(i+addr);
  float *y = (float *)&x;
  return y[0];
}


void motor(int val){
  pinMode(motorustart, OUTPUT);
  analogWrite(motorustart, 255);
  delay(1000);
  pinMode(motorustart, INPUT);
  //analogWrite(motorustart, 0);
  analogWrite(motoru, val);  
}

String timer(int val,int time){
  String timer;
  //int time=millis()/1000;

  if(time/60/60<10){timer = "0";}    
  timer = timer + time/60/60;
  timer = timer + ':';
  if(time/60%60<10){timer = timer + '0';}    
  timer = timer + (time/60)%60;
  timer = timer + ':';
  if(time%60<10){timer = timer + '0';}    
  timer = timer + time%60; 
 
  if (val == 1){
        mtime = 0; 
  }  
  
  return timer;  
}

int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

void draw(){
  Lcd.Clear();
  Lcd.GotoXY(0,0);
  Lcd.Print("Tstat:");
  
  Lcd.GotoXY(0,1);
  Lcd.Print("Boiler:");

  Lcd.GotoXY(0,2);
  Lcd.Print("Timer:");
    
  Lcd.GotoXY(0,7);
  Lcd.Print("Temp:");
  
  Lcd.GotoXY(0,4);
  Lcd.Print("TempDes:");

  Lcd.GotoXY(0,5);
  Lcd.Print("TempMax:");

  Lcd.GotoXY(0,6);
  Lcd.Print("TempMin:");

  Lcd.GotoXY(0,3);
  Lcd.Print("LTime:");  
}

void loop() {
  unsigned long currentMillis = millis();

  if(analogRead(button) > 200){
     if(currentMillis - buttonMillis < 300){
        Temp = Temp + 0.05;
     }else{
        Temp = Temp - 0.05;
     }
     buttonMillis = millis();
     delay(200);
     savetime = mtime;
  }

  if(Temp != EEPROM_float_read(0)){
      digitalWrite(testled, HIGH);
      
      if((mtime - savetime) > 5){
          EEPROM_float_write(0, Temp);
      }
  }else{
           digitalWrite(testled, LOW);
           savetime = mtime;  
       }
  
  if(currentMillis - previousMillis > 1000){
    previousMillis = millis();
    mtime++;
    
    char outstr[9];
    double t = Getterm(analogRead(0));
    t = t-4;
    //float h = dht.readHumidity();
    //float t;// = dht.readTemperature();
   
    Lcd.GotoXY(8,2);
    timer(0,mtime).toCharArray(outstr, 9);
    Lcd.Print(outstr);
  
    if (isnan(t)) {
        Lcd.GotoXY(7,0);
        Lcd.Print("Error");
        return;
    }else{
        Lcd.GotoXY(7,0);
        Lcd.Print("Work ");
    }
    
    if(work == true) 
    {
          digitalWrite(boiler, LOW);
    } else {
          digitalWrite(boiler, HIGH);
    }
      
    if(counter == 20) 
    {

      temperature = sumTemperature / 20;
      sumTemperature = 0;
      counter = 0;
      draw();
      
      if(mintemp > temperature){
        mintemp = temperature;
      }
      if(maxtemp<temperature){
        maxtemp = temperature;
      }
      
      if(temperature <= (Temp - gisteresis) and work != true) {
          work = true;
          ltime = timer(1,mtime);
      } 
      
      if(temperature >= (Temp + gisteresis) and work == true) {
          work = false;
          ltime = timer(1,mtime);
      }
    }
    else{
      sumTemperature = (sumTemperature + t);
      counter++;
    }
  
    if(work == true) {
        Lcd.GotoXY(7,1);
        Lcd.Print("On ");
    } else {
        Lcd.GotoXY(7,1);
        Lcd.Print("Off");
    }
  
    dtostrf(temperature,4, 2, outstr);
    Lcd.GotoXY(8,4);
    Lcd.Print(outstr);
  
    Lcd.GotoXY(5,7);
    dtostrf((Temp-0.05),3, 2, outstr);
    Lcd.Print(outstr);
    Lcd.Print("-");
    dtostrf((Temp+0.05),3, 2, outstr);
    Lcd.Print(outstr);


    
    
    Serial.print("ab");
    Serial.print(t);
    Serial.print("bacd");
    Serial.print(temperature);
    Serial.print("dcef");
    Serial.print(work);
    Serial.print("fegi");
    Serial.print(maxtemp);
    Serial.print("ighj");
    Serial.print(mintemp);
    Serial.print("jh");
    
    //dtostrf(freeRam(),3, 0, outstr);
    //Lcd.GotoXY(13,0);
    //Lcd.Print(outstr);  
     
    dtostrf(t,3, 2, outstr);
    Lcd.GotoXY(11,1);
    Lcd.Print(outstr);  
     
    dtostrf(maxtemp,3, 2, outstr);
    Lcd.GotoXY(8,5);
    Lcd.Print(outstr);  
  
    dtostrf(mintemp,3, 2, outstr);
    Lcd.GotoXY(8,6);
    Lcd.Print(outstr);
  
    Lcd.GotoXY(8,3);
    ltime.toCharArray(outstr, 9);
    Lcd.Print(outstr);
    //Serial.println(freeRam());
    //int data[2];
     //data[0] = int(t); // заполняем массив
     //data[1] = int(0);
     //radio.write(&data, sizeof(data)); 
  }
  
}
