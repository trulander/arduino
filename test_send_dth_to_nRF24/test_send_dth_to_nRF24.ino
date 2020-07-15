//#include <DHT.h>
  #include <avr/sleep.h>
  #include <avr/power.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h> // https://github.com/maniacbug/RF24
#define DHTPIN 2
//#define DHTTYPE DHT22  
//DHT dht(DHTPIN, DHTTYPE);
  
const uint64_t pipe = 0xF0F0F0F0E1LL; // индитификатор передачи, "труба"
 
RF24 radio(9, 10); // CE, CSN
 
void setup(){
  //delay(1000);
  Serial.begin(9600);
//  dht.begin();
  radio.begin();
  radio.setChannel(100);
  radio.openWritingPipe(pipe); // открываем трубу на передачу.
} 


void loop()   
{
  int data[2];   
  radio.stopListening();
  
  data[0] = 0;//int(dht.readTemperature()); // заполняем массив
  data[1] = 1;//int(dht.readHumidity());
  
  radio.write(data, sizeof(data)); // отправляем данные и указываем сколько байт пакет
   radio.startListening();

  delay(10000);
} 
