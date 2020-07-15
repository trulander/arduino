boolean statuslamp; // состояние лампы: true - включено, false - выключено
long summ = 0;
int curentvol = 0;
int counter = 0;
long soundTime=0; // время 1-го хлопка

void setup() // процедура setup
{

pinMode(2,OUTPUT); // пин 12 со светодиодом будет выходом (англ. «output»)
pinMode(A0,INPUT); // к аналоговому входу A0 подключим датчик (англ. «intput»)

statuslamp=false; // начальное состояние - лампа выключена
Serial.begin(9600); // подключаем монитор порта
}

void loop() // процедура loop
{
  //Serial.println (analogRead(A0)); // выводим значение датчика на монитор
  //long secondSoundTime=millis();
  counter = counter + 1;
  summ = summ + analogRead(A0);
  if(counter >= 300){
    curentvol = summ / 300;
    counter = 0;
    summ = 0;
    
  }


  Serial.println (analogRead(A0)); // выводим значение датчика на монитор
  //Serial.println (curentvol); // выводим значение датчика на монитор

  
  if ( (soundTime + 600 < millis() ) && (analogRead(A0)>curentvol+1)){
    soundTime = millis();
    statuslamp=!statuslamp; // меняем статус лампы при регистрации хлопка
    digitalWrite(13,statuslamp); // переключаем светодиод на выходе 12
  }
  
}
