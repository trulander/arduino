// Example testing sketch for various DHT humidity/temperature sensors
// Written by ladyada, public domain

#include "DHT.h"

#define DHTPIN 2     // what digital pin we're connected to
#define readpow 0
#define writepow 9
int value;
float velocity;
int counter = 0;
float summ = 0;
float himidity = 0;
int counter_max = 10;


#define DHTTYPE DHT11   // DHT 22  (AM2302), AM2321

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(9600);
  dht.begin();
  himidity = dht.readHumidity();
}


long map_Generic(float x, float in_min, float in_max, long out_min, long out_max){
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
} 

void loop() {


  if(counter == counter_max){
    counter = 0;
    himidity = summ / counter_max;
    summ = 0;
    
    analogWrite(writepow, map_Generic(himidity,0,100,0,255));
    
  }
  
  float h = dht.readHumidity();
  
  if (isnan(h)) {
    Serial.println("Failed to read from DHT sensor!");
    h = 1;
  }
  
  counter = counter + 1;
  summ = summ + h;
  
  
  

  

  Serial.print("himidity = ");
  Serial.print(himidity);
  Serial.print("  ");

  Serial.print("h = ");
  Serial.print(h);
  Serial.print("  ");


  value = analogRead(readpow);
  velocity = value* 5;
  velocity /= 1023;
  Serial.print("velocity = ");
  Serial.println(velocity);
  
  delay(2500);

}
