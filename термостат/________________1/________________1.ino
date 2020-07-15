#include <dht11.h>
dht11 DHT;
#define DHT11_PIN 14 // AN0 для совместимости с TC1047A
#define RELAY_PIN 2

float sumTemperature = 0.0;
float temperature = 0.0;
float maxTemperature = 22.0;
float minTemperature = 21.0;
int counter = 0;

void setup(){
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  Serial.begin(9600);
  Serial.println("DHT TEST PROGRAM ");
  Serial.print("LIBRARY VERSION: ");
  Serial.println(DHT11LIB_VERSION);
  Serial.println();
  Serial.println("Type,\tstatus,\tHumidity (%),\tTemperature (C)");
}

void loop(){

  int chk;
  Serial.print("DHT11, \t");
  chk = DHT.read(DHT11_PIN);    // READ DATA
  switch (chk){
  case DHTLIB_OK:  
    Serial.print("OK,\t"); 
    break;
  case DHTLIB_ERROR_CHECKSUM: 
    Serial.print("Checksum error,\t"); 
    break;
  case DHTLIB_ERROR_TIMEOUT: 
    Serial.print("Time out error,\t"); 
    break;
  default: 
    Serial.print("Unknown error,\t"); 
    break;
  }
  // DISPLAT DATA
  Serial.print(DHT.humidity,1);
  Serial.print(",\t");
  Serial.println(DHT.temperature,1);

  if(counter > 19) {
    temperature = sumTemperature / 20;
    counter = 0;
    sumTemperature = 0;
    if(temperature <= minTemperature) {
      digitalWrite (RELAY_PIN, HIGH); 
      Serial.println("warming-on");
    }
    if(temperature >= maxTemperature) {
      digitalWrite (RELAY_PIN, LOW);
       Serial.println("warming-off");
    }
    Serial.print("averageTemperature \t");
    Serial.println(temperature);
    Serial.print("\n");
  }
  else  {
    counter ++;
    sumTemperature = (sumTemperature + DHT.temperature);
    Serial.print("counter \t");
    Serial.println(counter);
    Serial.print("sumTemperature \t");
    Serial.println(sumTemperature);
    Serial.print("\n");
  }

  delay(7000);

}
