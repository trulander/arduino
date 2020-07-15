#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h> // https://github.com/maniacbug/RF24


#include <Wire.h>

#define led 6
long previousMillis = 0;

RF24 radio(9, 10); // CE, CSN

const uint64_t pipe = 0xF0F0F0F0E1LL; // индитификатор передачи, "труба"


void setup(){
  pinMode(led, OUTPUT);
  digitalWrite(led, HIGH); 

  radio.begin();  
  delay(5);  
  //radio.setChannel(100);
  //radio.setDataRate(RF24_250KBPS);     
  //radio.setPALevel(RF24_PA_HIGH);   
  radio.openReadingPipe(1,pipe); // открываем первую трубу с индитификатором "pipe"
  radio.startListening(); // включаем приемник, начинаем слушать трубу
  delay(20);

}



void loop()   
{
  float data[2];
  if(millis() - previousMillis > 1000){
    previousMillis = millis();
    digitalWrite(led, LOW);


    if (radio.available()){ // проверяем не пришло ли чего в буфер.
      bool done = false;
      while (!done)
      {
        done = radio.read(data, sizeof(data)); // читаем данные и указываем сколько байт читать
        digitalWrite(led, HIGH);
      } 
        radio.stopListening();
        radio.startListening();
    }
  }
}
