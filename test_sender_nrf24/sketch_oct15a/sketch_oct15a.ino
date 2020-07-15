#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <SPI.h>
#include <Wire.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <stdint.h>

#define chclient 1
#define sleep_bod_disable()

#define RF_PIN 5



volatile int f_wdt = 1;
int counter = 0;

RF24 radio(9,10);


typedef struct{        
  byte identifier;
  int temperature_Sensor;
}

B_t;
B_t clientnf;

void counterHandler()
{

  counter++;
 
  if(counter == 1) {  
   
    counter = 0;

        power_all_enable();

        PORTD |= 5 << RF_PIN;
   
        radio.powerUp();
   
    delay(2);
 
  } else {
   
    enterSleep();
 
  }

}

void enterSleep()
{

  f_wdt = 0;
 
    PORTD &= ~(5 << RF_PIN);
 
    sleep_enable();
    sleep_mode();
    sleep_disable();
    counterHandler();
}

ISR(WDT_vect)
{
  f_wdt = 1;
}

void setupWDT()
{
    MCUSR &= ~(1<<WDRF);
    WDTCSR |= (1<<WDCE) | (1<<WDE);
    WDTCSR = 1<<WDP0 | 1<<WDP3;
    WDTCSR |= _BV(WDIE);
}

void setup()
{

    sleep_bod_disable();
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    setupWDT();
 
    ADCSRA = 0;

    DDRD |= 5 << RF_PIN;
 
  }

void loop()
{

radio.begin();

    radio.setDataRate(RF24_2MBPS);
    radio.setPALevel (RF24_PA_HIGH);
    radio.setAutoAck(false);
    radio.setCRCLength(RF24_CRC_16);
    radio.setChannel(1);
    radio.setRetries(15,15);
    clientnf.identifier = chclient;
    radio.openWritingPipe(1);



delay(20);

  clientnf.identifier = 1;

  radio.openWritingPipe(0xF0F0F0F0E1LL);
  radio.write(&clientnf, sizeof(clientnf));
 
  enterSleep();
}
