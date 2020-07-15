#include <avr/sleep.h>


int wakePin = 2;                 // Пин используемый для просыпания (прерывания)
int sleepStatus = 0;             // Переменная для хранения статуса (спим, проснулись) - не используется в коде
int count = 0;                   // Счетчик
int LedPin=13;                   // Светодиод

void wakeUpNow()        // Прерывание сработает после пробуждения
{
    sleep_disable();             // то первое, что нужно сделать после просыпания - выключить спящий режим
    digitalWrite(LedPin, HIGH);  // Включаем светодиод
    sleepStatus = 0;             // В переменную заносим статус бодрствования
}


void setup()
{
  //CLKPR = 1<<CLKPCE;
  //CLKPR = 4;
  pinMode(LedPin, OUTPUT);
  digitalWrite(LedPin, HIGH);                // Включаем светодиод
  pinMode(wakePin, INPUT);
  digitalWrite(wakePin, HIGH);               // Подтягивем ногу к 5.
  Serial.begin(9600);
  attachInterrupt(0, wakeUpNow, FALLING);    // Используем прерывание 0 (pin 2) для выполнения функции wakeUpNow (прерывание вызывается только при смене значения на порту с HIGH на LOW - подтянуть ногу 2 на 5в.)

}


void sleepNow()         // Функция увода ардуины в спячку.
{
  digitalWrite(LedPin, LOW);             // Выключаем светодиод
  
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);   // Здесь устанавливается режим сна

  sleep_enable();                        // Включаем sleep-бит в регистре mcucr. Теперь возможен слип 
  byte adcsra_save = ADCSRA;//запоминаем режим работы ацп
  ADCSRA = 0;  
  
  sleep_cpu();
  attachInterrupt(0,wakeUpNow, LOW);     // Используем прерывание 0 (pin 2) для выполнения функции wakeUpNow при появлении низкого уровня на пине 2
  count = 0;                             // Обнуляем счетчик прошедших секунд
  sleepStatus = 1;                       // В переменную заносим статус сна
  sleep_mode();                          // Здесь устройство перейдет в режим сна!!!
  // -----------------------------------------------ПОСЛЕ ПРОСЫПАНИЯ ВЫПОЛНЕНИЕ КОДА ПРОДОЛЖИТСЯ ОТСЮДА!!!
  
  ADCSRA = adcsra_save;  // восстанавливаем режим работы ацп                       
  //sleep_disable();                       // Первое, что нужно сделать после просыпания - выключить спящий режим
  //detachInterrupt(0);                    // Выключаем прерывание - при нормальном режиме wakeUpNow() не будет вызываться
}

void loop()
{
  // Отображаем информацию о счетчике
  Serial.print("Awake for ");
  Serial.print(count);
  Serial.println("sec");
  count++;
  delay(1000);                           // Ждем секунду
  sleepNow();
}
