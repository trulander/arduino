#include <DHT.h>

#define DHTTYPE DHT11   // DHT 11


#define DHTPIN A2     // iput dht11
#define cooller A1     //reley cooler
#define himidity 55 // % +/- gisteresis
#define gusteresis 5 // +/- %

DHT dht(DHTPIN, DHTTYPE);

int summ = 0;
int curenthimidity = 0;
int counter = 0;
long lasttimeread = 0;
long lasttimeled = 0;
boolean error = false;
boolean statusled = false;
boolean work = false;


void setup() {
  pinMode(cooller,OUTPUT);
  pinMode(13,OUTPUT);
  
  digitalWrite(13,false);
  delay(100);
  digitalWrite(13,true);
  delay(100);
  digitalWrite(13,false);
  delay(100);
  digitalWrite(13,true);
  delay(100);
  digitalWrite(13,false);
  delay(100);
  digitalWrite(13,true);
  
  delay(500);
  
  dht.begin();  
  
  //Serial.begin(9600);
  //Serial.println("start ok");
  
}

void loop() {

 if ( lasttimeread + 2000 < millis() ){

    lasttimeread = millis();
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    
    if (isnan(h) || isnan(t)) {
      //Serial.println("Failed to read from DHT sensor!");
      work = true;
      error = true;
      h = 100;
    }else{
      error = false;
    }
    

  
    counter = counter +1;
    summ = summ + h;
    
    if(counter >= 5){
      counter = 0;
      curenthimidity = summ / 5;
      summ = 0;
    }
    
    if(curenthimidity >= himidity + gusteresis){
      work = true;
    }

    if(curenthimidity <= himidity - gusteresis){
      work = false;
    }
  
    if(work){
      digitalWrite(cooller,HIGH);
    }else{
      digitalWrite(cooller,LOW);
    }
    
        //Serial.println("Humidity: ");
      //Serial.println(h);
      //Serial.println(summ);
      //Serial.println(curenthimidity);
      //Serial.print("Temperature: ");
      //Serial.print(t);
  }
  
  if ( lasttimeled + 100 < millis() ){
    lasttimeled = millis();
      if(error){
        statusled=!statusled; // меняем статус лампы при регистрации ошибки
        digitalWrite(13,statusled); // переключаем светодиод на выходе 13
      }else{
          digitalWrite(13,false); // переключаем светодиод на выходе 13
      }
  }
}
