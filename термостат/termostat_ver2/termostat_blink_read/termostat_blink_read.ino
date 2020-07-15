//#define BUFFER_SIZE 128
//char buffer[BUFFER_SIZE];

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>



#define radiopow1 5
#define radiopow2 6
#define radiopow3 7


RF24 radio(9, 10); // CE, CSN

// Рекомендуют первые 2-4 байта адреса устанавливать в E7 или 18 он проще детектируется чипом
//0xE7E7E7E7E1LL 0xF0F0F0F0E1LL
const uint64_t pipe = 0xE7E7E7E7E1LL; // индитификатор передачи, "труба"

float data[3];


long previousMillis;

void setup(){

  pinMode(radiopow1, OUTPUT);
  pinMode(radiopow2, OUTPUT);
  pinMode(radiopow3, OUTPUT);    
  PORTD |= 5 << radiopow1;
  PORTD |= 5 << radiopow2;
  PORTD |= 5 << radiopow3;
  delay(500); 
  Serial.begin(9600);





  /*************************                Модуль NRF24                **********************/
  //delay(2000);                          // Пусть зарядится конденсатор
  radio.begin();                          // Включение модуля;
  //radio.setChannel(0);                    // Установка канала вещания;
  //radio.setRetries(15,5);                // Установка интервала и количества попыток "дозвона" до приемника;
  //radio.setDataRate(RF24_1MBPS);        // Установка минимальной скорости; RF24_2MBPS RF24_1MBPS RF24_250KBPS
  //radio.setPALevel(RF24_PA_LOW);          // Установка максимальной мощности;
  //radio.setAutoAck(false);                    // Установка режима подтверждения приема;
  //radio.openWritingPipe(pipe);     // Активация данных для отправки
  radio.openReadingPipe(1,pipe);   // Активация данных для чтения
  radio.startListening();                 // Слушаем эфир.

 
  previousMillis = 0;

}


void loop()   
{


  if(millis() - previousMillis > 1000){
    previousMillis = millis();


  
    if (radio.available()){ // проверяем не пришло ли чего в буфер.
       radio.read(&data, sizeof(data)); // читаем данные и указываем сколько байт читать
       Serial.println(' ');
       Serial.print(data[0]);
       Serial.print(data[1]);
       Serial.print(data[2]);

    } 
       Serial.print('-');
      
  }
}
