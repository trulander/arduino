// ОММЕТР (С)2013 ALEN Studio by Little_Red_Rat

// Омметр на осное ARDUINO
//
// Подключение делителя напряжения к ARDUINO
// Arduino 5V -> R1 10kOm -> Arduino Analog 0 -> R2 -> Arduino GND
#define xt1 A6   
#define yt1 A7  
#define xt2 A4   
#define yt2 A5  
int oldX, oldY, xt, yt;  
byte bytes[4];  

#include <math.h>
#define Temp1Count 3
#include <avr/pgmspace.h>
#include <PCF8814.h> 

PCF8814 Lcd(13,11,10,6); // LCD sets SPI SCLK: 7 pin, SDA: 8 pin, CS: 9 pin. RESET: 6 pin

int hight = 13;
int med = 2;
int analogPin = A0; // Анлоговый вход для считывания напряжения с делителя напряжения

    
uint16_t Temp1[Temp1Count][2] PROGMEM = {{61190.47,-13}, {22098.76,20},{18242.99,23.3}};

float Vout = 0; // Переменная для хранения значения напряжения в средней точки делителя (0-5.0)
float R2 = 0; // Переменная для хранения значения резистора R2

void setup() 
{
  Lcd.Init();
   //Lcd.Inverse(INV_MODE_ON);
  Lcd.Mirror(1,1);
  Lcd.Contrast(20);
  //Lcd.GotoXY(1,4);
  
 // Lcd.PrintF(Text);
  pinMode(13, OUTPUT);
  pinMode(2, OUTPUT);
  Serial.begin(9600); // Подготовка Serial Monitor для вывода информации
} 


void loop() 
{ 
float x,x1,x2,t1,t2,t;  
  char text[15];// = "GRAPH INIT";
  char buf[15];
String tex;  

 pinMode(xt1,OUTPUT);   
  pinMode(xt2,OUTPUT);   
  digitalWrite(xt1,LOW);   
  digitalWrite(xt2,HIGH); //подключаем к пластине Х напряжение +5в и GND  
  digitalWrite(yt1,LOW);  
  digitalWrite(yt2,LOW); //разряжаем емкость  
  pinMode(yt1,INPUT);   
  pinMode(yt2,INPUT);  
  delay(10);  
  xt=analogRead(3); //считываем значение с пластины Х  
    
  pinMode(yt1,OUTPUT);   
  pinMode(yt2,OUTPUT);   
  digitalWrite(yt1,LOW);   
  digitalWrite(yt2,HIGH);  //подключаем к пластине У напряжение +5в и GND  
  digitalWrite(xt1,LOW);  
  digitalWrite(xt2,LOW);  //разряжаем емкость  
  pinMode(xt1,INPUT);   
  pinMode(xt2,INPUT);     
  delay(10);  
  yt=analogRead(0);  //считываем значение с пластины У  
    
   if( xt > 20 && yt > 20 )
   {
    Serial.print(xt,DEC);    
    Serial.print(",");      
    Serial.println(yt,DEC); 
    oldX=xt;
    oldY=yt;
   }   

Vout = (5.0 / 1023.0) * analogRead(analogPin); // Вычисляем напряжение в средней точки делителя (0-5.0)
R2 = 20000 / ((5.0 / Vout) - 1); // Вычисляем сопротивление R2 (10000 это значение R1 10 кОм)

    t1=-13;
    x1=61190.47;
    t2=23.3;
    x2=18242.99;
    t = t1 + (t2-t1) * (R2-x1) / (x2-x1);

//Serial.print("Voltage: "); // 
//Serial.println(Vout); // Напряжения в средней точки делителя (0-5.0) для справки
//Serial.print("R2: "); // 
//Serial.println(R2); // Значение сопротивления R2
//Serial.print("Temp: "); // 
//Serial.println(t); // Значение сопротивления R2
  
  //Lcd.Clear();
  
  tex= String(t);
  tex.toCharArray(buf, 6);
  //dtostrf(t,3,2,buf);
  sprintf(text,"%s %s","Temp:",buf);
  Lcd.Print(text);
  
//  memset(buf,0,sizeof(buf));
//  memset(text,0,sizeof(text));
  //delete[]buf;
  //delete[]text;
  tex= String(Vout);
  tex.toCharArray(buf, 6);
  sprintf(text,"%s %s","Voltage: ",buf);
  Lcd.GotoXY(0,1);
  Lcd.Print(text);

  Lcd.GotoXY(0,0);
if (t > 24)
{
  digitalWrite(13, HIGH);
  digitalWrite(2, LOW); 
}else{
  digitalWrite(2, HIGH);
  digitalWrite(13, LOW);       
}


analogWrite(A6,10);
//tone(8, 15000,1000);
//noTone(8);
delay(1000); // Пауза 1 сек
}
