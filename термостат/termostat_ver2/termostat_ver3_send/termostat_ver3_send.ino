#include <DHT.h>
#include <avr/sleep.h>
#include  <avr/wdt.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h> // https://github.com/maniacbug/RF24

#define DHTPIN 4
#define DHTTYPE DHT22  

const long ConstADC=1126400;                    // Калибровка встроенного АЦП (встроенный ИОН) по умолчанию 1126400

// Макросы для работы с портами  скорость и место
  #define SetOutput(port,bit)       DDR ## port |= _BV(bit)
  #define SetInput(port,bit)        DDR ## port &= ~_BV(bit)
  #define SetBit(port,bit)          PORT ## port |= _BV(bit)
  #define ClearBit(port,bit)        PORT ## port &= ~_BV(bit)
  #define WritePort(port,bit,value) PORT ## port = (PORT ## port & ~_BV(bit)) | ((value & 1) << bit)
  #define ReadPort(port,bit)        (PIN ## port >> bit) & 1
  #define PullUp(port,bit)          { SetInput(port,bit); SetBit(port,bit); }
  #define Release(port,bit)         { SetInput(port,bit); ClearBit(port,bit); }

#define radiopow1 5
#define radiopow2 6
#define radiopow3 7
//#define wakePin 2
#define led_send 3
#define lowpin 2

DHT dht(DHTPIN, DHTTYPE);

// Рекомендуют первые 2-4 байта адреса устанавливать в E7 или 18 он проще детектируется чипом
//0xE7E7E7E7E1LL 0xF0F0F0F0E1LL
const uint64_t pipe = 0xE7E7E7E7E1LL; // индитификатор передачи, "труба"
 
RF24 radio(9, 10); // CE, CSN

// watchdog interrupt
ISR (WDT_vect) 
{
   wdt_disable();  // disable watchdog
}  // end of WDT_vect
 
void setup(){
  //attachInterrupt(0,wakeUpNow, LOW);
   //Serial.begin(9600);
  dht.begin();
  
  pinMode(radiopow1, OUTPUT);
  pinMode(radiopow2, OUTPUT);
  pinMode(radiopow3, OUTPUT);  
  pinMode(led_send, OUTPUT);
  //pinMode(9, OUTPUT);
  //digitalWrite(led_send, HIGH);
  
} 

void wakeUpNow()
{
    sleep_disable();
}


void sleep_watchdog()//8sec   
{
 
  // disable ADC
  byte adcsra_save = ADCSRA;//запоминаем режим работы ацп
  ADCSRA = 0;  

  // clear various "reset" flags
  MCUSR = 0;     
  // allow changes, disable reset
  WDTCSR = bit (WDCE) | bit (WDE);
  // set interrupt mode and an interval 
  WDTCSR = bit (WDIE) | bit (WDP0) | bit (WDP3);    // set WDIE, and 8 second delay
  wdt_reset();  // pat the dog
  
  set_sleep_mode (SLEEP_MODE_PWR_DOWN);  
  sleep_enable();  

  // turn off brown-out enable in software
//  MCUCR = bit (BODS) | bit (BODSE);
  //MCUCR = bit (BODS);

  sleep_cpu (); 
 // sleep_mode(); 


  // cancel sleep as a precaution
  ADCSRA = adcsra_save;  // восстанавливаем режим работы ацп 
  sleep_disable();
}

// Чтение напряжения питания ----------------------------------------------
long readVcc() {
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = ConstADC / result; // Back-calculate AVcc in mV
  return result;
} 

void loop()   
{
  digitalWrite(led_send, HIGH);
  //digitalWrite(radiopow1, HIGH);
  //digitalWrite(radiopow2, HIGH);
  //digitalWrite(radiopow3, HIGH);  
  PORTD |= 5 << radiopow1;
  PORTD |= 5 << radiopow2;
  PORTD |= 5 << radiopow3;
  delay(220); //на заряд кондеров для nrf24l01

  float data[3];
  
  radio.begin();                          // Включение модуля;
  radio.openWritingPipe(pipe);     // Активация данных для отправки

  dht.begin();
  data[0] = float(dht.readTemperature()); // заполняем массив
  data[1] = float(dht.readHumidity());
  data[2] = ((float)readVcc())/980.0;
  
  delay(20);  
  radio.write(&data, sizeof(data)); // отправляем данные и указываем сколько байт пакет



  PORTD &= ~(5 << radiopow1);
  PORTD &= ~(5 << radiopow2);
  PORTD &= ~(5 << radiopow3);
  //digitalWrite(radiopow1, LOW);
  //digitalWrite(radiopow2, LOW);
  //digitalWrite(radiopow3, LOW);  
  //Serial.println(data[0]);
  //Serial.println(data[1]);
  //Serial.println(data[2]);

  digitalWrite(led_send, LOW);
  sleep_watchdog();
 //delay(3000);
} 
