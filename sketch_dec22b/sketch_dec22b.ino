#include <avr/sleep.h>
#include <DS3232RTC.h> //http://github.com/JChristensen/DS3232RTC
#include <Time.h> //http://www.arduino.cc/playground/Code/Time
#include <Wire.h> //http://arduino.cc/en/Reference/Wire (included with Arduino IDE)

// Pin per l'accensione del modulo RTC
int const pinRTC = A0;

void setup()
{
// Power for RTC from Arduino PIN to RTC VCC
pinMode(pinRTC, OUTPUT);
digitalWrite(pinRTC, HIGH);
Serial.begin(9600);
Serial.println("Startup...");
delay(100);
// Set Alarm2 every minute
RTC.setAlarm(ALM1_EVERY_SECOND, 00, 00, 00, (dowSunday | dowMonday | dowTuesday | dowWednesday | dowThursday | dowFriday | dowSaturday));
//RTC.alarmInterrupt(ALARM_2, true); //assert the INT pin when Alarm2 occurs.
RTC.squareWave(SQWAVE_NONE);
RTC.alarmInterrupt(ALARM_1, true);
delay(100);
}

void wakeUp()
{
  sleep_disable();
}


void loop()
{
// Allow wake up pin to trigger interrupt on low.
attachInterrupt(0, wakeUp, FALLING);
// Enter power down state with ADC and BOD module disabled.
// Wake up when wake up pin is low.
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);   // Здесь устанавливается режим сна

  sleep_enable();                        // Включаем sleep-бит в регистре mcucr. Теперь возможен слип 
  //byte adcsra_save = ADCSRA;//запоминаем режим работы ацп
  //ADCSRA = 0;  
  
  sleep_cpu();
  sleep_mode();                          // Здесь устройство перейдет в режим сна!!!
  // -----------------------------------------------ПОСЛЕ ПРОСЫПАНИЯ ВЫПОЛНЕНИЕ КОДА ПРОДОЛЖИТСЯ ОТСЮДА!!!
  

  //ADCSRA = adcsra_save; 
// Disable external pin interrupt on wake up pin.
//detachInterrupt(0);

if (RTC.alarm(ALARM_2))
{
Serial.println("Alarm fired!");
}
else
{
Serial.println("-");
}

delay (100);

// Turn on VCC on RTC from out pin arduino
digitalWrite(pinRTC, HIGH);
// date and time
tmElements_t tm;
if (RTC.read(tm) == 0) // Se la lettura è avvenuta correttamente tramite I2C
{
Serial.print(tm.Day, DEC);
Serial.print('/');
Serial.print(tm.Month, DEC);
Serial.print('/');
Serial.print(tm.Year, DEC);
Serial.print(' ');
Serial.print(tm.Hour, DEC);
Serial.print(':');
Serial.print(tm.Minute, DEC);
Serial.print(':');
Serial.println(tm.Second, DEC);
}
else
{
Serial.println("Errore RTC");
}
delay(100);
// Spengo il modulo RTC
digitalWrite(pinRTC, LOW);

}
 


