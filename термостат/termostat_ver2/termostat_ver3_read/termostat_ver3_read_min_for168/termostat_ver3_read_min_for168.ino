//#define BUFFER_SIZE 128
//char buffer[BUFFER_SIZE];

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <EEPROM.h>
#include <Wire.h>
#include <DS3231.h>


#define boiler 4
#define ring 9
#define led_input 6
#define led_watchdog 5

DS3231 clock;
RTCDateTime dt;

RF24 radio(8, 7); // CE, CSN

// Рекомендуют первые 2-4 байта адреса устанавливать в E7 или 18 он проще детектируется чипом
//0xE7E7E7E7E1LL 0xF0F0F0F0E1LL
const uint64_t pipe = 0xE7E7E7E7E1LL; // индитификатор передачи, "труба"
const long ConstADC=1126400;                    // Калибровка встроенного АЦП (встроенный ИОН) по умолчанию 1126400
bool work,alarmState;

float buff_cy,maxtemp_cy,mintemp_cy,temp_on,temp_off,data[3],sumTemperature,temperature,Temp,gisteresis,interval_temp[2],interbuff;

int up_dwn,counter,error,do_error,interval[4],alarm_clock,p_hour_i,p_hour_c;

long previousMillis;





void radio_init(){
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
}

void setup(){
  pinMode(led_input, OUTPUT);
  pinMode(led_watchdog, OUTPUT);
  //digitalWrite(led_input, HIGH);
  //digitalWrite(led_watchdog, HIGH); 
 // pinMode(wakePin, INPUT);
 // digitalWrite(wakePin, HIGH);               // Подтягивем ногу к 5в.
  //attachInterrupt(0, wakeUpNow, FALLING);  
  pinMode(boiler, OUTPUT);
  pinMode(ring, OUTPUT);

  Serial.begin(9600);
  
  delay(500);
  clock.begin();

  radio_init();

  //это для установки новой даты в часах
  //clock.setDateTime(2015, 12, 31, 14, 14, 00);

  //а это для записывания в память базовых значений(актуально для нового контроллера с пустым еепром)
  //EEPROM_float_write(0, 20.8);
  //EEPROM.write(0, 20.00);//temp
  //EEPROM.write(5, 0);//command
  //EEPROM.write(9, 0.05);//gisteresis
  
  //EEPROM_float_write(14, 6);//interval[1] время стартовое
  //EEPROM_float_write(19, 10);//interval[2] время окончания
  //EEPROM_float_write(24, 20);//interval[3] время стартовое второе
  //EEPROM_float_write(29, 21);//interval[4] время окончания второе
  //EEPROM_float_write(35, 1.0);//добавочная температура для первого (может быть отрицательной)
  //EEPROM_float_write(41, -1.8);//добавочная температура для второго (может быть отрицательной)

  Temp = EEPROM_float_read(0);

  gisteresis = EEPROM_float_read(9);
  interval[0] = EEPROM_float_read(14);
  interval[1] = EEPROM_float_read(19);
  interval[2] = EEPROM_float_read(24);
  interval[3] = EEPROM_float_read(29);
  interval_temp[0] = EEPROM_float_read(35);
  interval_temp[1] = EEPROM_float_read(41);

  maxtemp_cy = Temp + gisteresis;
  mintemp_cy = Temp - gisteresis;
  temp_on = Temp;
  temp_off = Temp; 
  up_dwn = 1;//режим выключенного котла
  counter = 0;
  error = 0;
  do_error = 0;
  previousMillis = 0;
  work = false;
  sumTemperature = 0.0;
  temperature = 0.0;
}

void EEPROM_float_write(int addr, float val) // запись в ЕЕПРОМ
{  
  byte *x = (byte *)&val;
  for(byte i = 0; i < 4; i++) EEPROM.write(i+addr, x[i]);
}

float EEPROM_float_read(int addr) // чтение из ЕЕПРОМ
{    
  byte x[4];
  for(byte i = 0; i < 4; i++) x[i] = EEPROM.read(i+addr);
  float *y = (float *)&x;
  return y[0];
}

void loop()   
{
   dt = clock.getDateTime();


  if(millis() - previousMillis > 1000){
    previousMillis = millis();
    do_error++;
    float temp_buff;


    if(Serial.available()) {
      int var1 = Serial.parseInt();
      float var2 = Serial.parseFloat();
      int var3 = Serial.parseInt();
      float var4 = Serial.parseFloat();
      String buff;
      switch (var1) {
        case 2:
          Temp = var2;
          EEPROM_float_write(0, var2);
          break;
        case 3:
          gisteresis = var2;
          EEPROM_float_write(9, var2);
          break;
        case 4:
          interval[0] = var2;
          interval[1] = var3;
          interval_temp[0] = var4;
          EEPROM_float_write(35, var4);//temp
          EEPROM_float_write(14, var2);//interval[1] время стартовое
          EEPROM_float_write(19, var3);//interval[2] время окончания        
          break;
        case 5:
          interval[2] = var2;
          interval[3] = var3;     
          interval_temp[1] = var4;
          EEPROM_float_write(41, var4);   //temp
          EEPROM_float_write(24, var2);//interval[3] время стартовое второе
          EEPROM_float_write(29, var3);//interval[4] время окончания второе        
          break;
        case 7://settings
        
          Serial.print("T2: ");  
          Serial.println(EEPROM_float_read(0));       
                
          Serial.print("G3: ");  
          Serial.println(gisteresis);  
    
          Serial.print("I4 1: ");
          Serial.print(interval[0]); 
          Serial.print(" - "); 
    
          Serial.print(interval[1]); 
          Serial.print(" : ");       
          Serial.println( interval_temp[0]); 
    
          Serial.print("I5 2: ");
          Serial.print(interval[2]); 
          Serial.print(" - "); 
                         
          Serial.print(interval[3]);  
          Serial.print(" : ");       
          Serial.println( interval_temp[1]);
             
          break;                                       
          }
    }    


    if (digitalRead(led_watchdog) == LOW){
      digitalWrite(led_watchdog, HIGH); 
    }else{
      digitalWrite(led_watchdog, LOW);
    }
  
  digitalWrite(led_input, LOW);
  
    if (radio.available()){ // проверяем не пришло ли чего в буфер.
       radio.read(&data, sizeof(data)); // читаем данные и указываем сколько байт читать

        delay(20);
        do_error=0;
        error=0;
        digitalWrite(led_input, HIGH);
        
        if(counter == 4) 
        {
    
          temperature = sumTemperature / 4;
          sumTemperature = 0;
          counter = 0;

          //Начало цикла
          
            //определение рабочего режима остывание или нагрев
             switch (up_dwn) {
              case 0://отключили котел, температура продолжает расти ловим максимальную температуру
                if(buff_cy <= temperature and work != true){//температура продолжает подниматься, хотя котел отключен
                  buff_cy = temperature;
                }else{ 
                  if(buff_cy > temperature and work != true){//температура перестала подниматься после выключения котла
                    maxtemp_cy = buff_cy;
        
                    if(maxtemp_cy > Temp + gisteresis){
                      temp_buff = temp_off - ((maxtemp_cy - (Temp + gisteresis)) / 2);
                      if(temp_buff >= (Temp - (gisteresis*2)) and temp_buff <= (Temp + (gisteresis*2))){
                        temp_off = temp_buff;
                      }else{
                        temp_off = Temp - (gisteresis*2);//устанавливаем минимальную температуру с ограничением
                      }
                    }
        
                    if(maxtemp_cy < Temp){
                      temp_buff = temp_off + ((Temp - maxtemp_cy) / 2);
                      if(temp_buff >= (Temp - (gisteresis*2)) and temp_buff <= (Temp + (gisteresis*2))){
                        temp_off = temp_buff;
                      }else{
                        temp_off = Temp + (gisteresis*2);//устанавливаем максимальную температуру с ограничением
                      }
                    }
                    up_dwn = 1;
                  }
                }                
                break;
              case 1://ждем когда температура снова достигнет температуры включения
                if((temperature <= temp_on || temperature <= Temp - gisteresis) and work != true) {//Сделал проверку на то что котел не будет ждять понижения температуры ниже установленных пределов, это связано с суточными таймерами и изменением температуры
                  if(temperature <= Temp - gisteresis){//увеличиваем скорость корректировки после каких то изменений температуры, смысл в том что температура включения не может быть ниже заданного диапазона поддержания температуры, если это произошло, то мы искусственно уменьшаем это значение
                    temp_on = Temp - gisteresis;
                  }
                    work = true;

                    up_dwn = 2;
                }                 
                break;
              case 2://включили котел, температура продолжает падать ловим  минимальную температуру
                if(buff_cy >= temperature and work == true){ //температура продолжает падать, хотя котел включен
                  buff_cy = temperature;
                }else{
                  if(buff_cy < temperature and work == true){//температура только что начала подниматься после включения котла
                    mintemp_cy = buff_cy;
                    
                    if(mintemp_cy < Temp - gisteresis){
                      temp_buff = temp_on + (((Temp - gisteresis) - mintemp_cy) / 2);
                      if(temp_buff >= (Temp - (gisteresis*2)) and temp_buff <= (Temp + (gisteresis*2))){
                        temp_on = temp_buff;
                      }                      
                    }
        
                    if(mintemp_cy > Temp){
                      temp_buff = temp_on - ((mintemp_cy - Temp) / 2);
                      if(temp_buff >= (Temp - (gisteresis*2)) and temp_buff <= (Temp + (gisteresis*2))){
                        temp_on = temp_buff;
                      }                       
                    } 
                    up_dwn = 3;             
                  }
                }                  
                break;
              case 3://ждем когда температура достигнет температуры выключения
                if((temperature >= temp_off || temperature >= Temp + gisteresis) and work == true) {//Сделал проверку на то что котел не будет ждять повышения температуры выше установленных пределов, это связано с суточными таймерами и изменением температуры
                  if(temperature >= Temp + gisteresis){//увеличиваем скорость корректировки после каких то изменений температуры, смысл в том что температура выключения не может быть выше заданного диапазона поддержания температуры, если это произошло, то мы искусственно уменьшаем это значение
                    temp_off = Temp + gisteresis;
                  }
                    work = false;

                    up_dwn = 0;
                }
                break;                
              default:
                
              break;
             }
          
          //конец цикла
        }
        else{
          sumTemperature = (sumTemperature + data[0]);
          counter++;
        }
     // }
      //radio.stopListening();
      //radio.startListening();
    } 

    if(do_error >= 25){
      do_error=0;
      error++;
      radio_init();
    }

    if(error >= 5){//ошибка чтения данных с датчика больше 125сек
      work = true;//включаем котел
      data[0]=NAN;//выводим температуру в неизвестное
      up_dwn = 3;//переводим рабочий цикл в положение включенного котла
    }
   



    //изменение температуры в зависимости от времени суток
    //делаем правильный суточный таймер, который учитывает переход по времени через сутки и ему неважно в какой момент началась проверка, он проверяет находится ли он в интервале или нет.

    //первый интервал
    //проверка на неправильный интервал
    if(interval[0] > interval[1]){//если время окончания интервала меньше времени начала, то значит интервал в переходе между днямии условно "неправильный", мы к конечному часу прибавляем 24часа и получаем правильный интервал
      p_hour_i = 24;
      if((dt.hour + interval[1]) < 23){//если текущее время в сумме с временем окончания интервала меньше 23, то прибавляем 24 часа и исправляем "неправильный" интервал.
        p_hour_c = 24;
      }else{
        p_hour_c = 0;
      }
    }else{
      p_hour_i=0;
    }
    //проверка вхождения в интервал
    if((dt.hour + p_hour_c) >= interval[0] && (dt.hour + p_hour_c) <= (interval[1] + p_hour_i - 1)){//для компенсации отсутствия минут, я отнимаю 1час у вемени окончания интервала, так как по наступлении последнего часа интервал будет соответствовать еще 59минут, так как мы проверяем только минуты
      if(max(interval_temp[0],0) > 0){//проверяем является ли прибавочная теммпература положительной и прибавляем ее, если отрицательная то вычитаем
        interbuff = EEPROM_float_read(0) + interval_temp[0];
      }else{
        interbuff = EEPROM_float_read(0) + interval_temp[0];
      }
      if(Temp != interbuff){//проверяем изменяли ли мы уже временную температуру или нет
        Temp = interbuff;//задаем новую температуру, она будет временной, так как не записываем ее в еепром.
      }
    }else{
      //второй интервал
      //проверка на неправильный интервал
      if(interval[2] > interval[3]){//если время окончания интервала меньше времени начала, то значит интервал в переходе между днямии условно "неправильный", мы к конечному часу прибавляем 24часа и получаем правильный интервал
        p_hour_i = 24;
        if((dt.hour + interval[3]) < 23){//если текущее время в сумме с временем окончания интервала меньше 23, то прибавляем 24 часа и исправляем "неправильный" интервал.
          p_hour_c = 24;
        }else{
          p_hour_c = 0;
        }
      }else{
        p_hour_i=0;
      }
      //проверка вхождения в интервал
      if((dt.hour + p_hour_c) >= interval[2] && (dt.hour + p_hour_c) <= (interval[3] + p_hour_i - 1)){//для компенсации отсутствия минут, я отнимаю 1час у вемени окончания интервала, так как по наступлении последнего часа интервал будет соответствовать еще 59минут, так как мы проверяем только минуты
        if(max(interval_temp[1],0) > 0){//проверяем является ли прибавочная теммпература положительной и прибавляем ее, если отрицательная то вычитаем
          interbuff = EEPROM_float_read(0) + interval_temp[1];
        }else{
          interbuff = EEPROM_float_read(0) + interval_temp[1];
        }
        if(Temp != interbuff){//проверяем изменяли ли мы уже временную температуру или нет
          Temp = interbuff;//задаем новую температуру, она будет временной, так как не записываем ее в еепром.
        }
      }else{//если все таймеры прошли и показания разнятся, обнуляем температуру до заданной в настройках
        Temp = EEPROM_float_read(0);//обнуляем на сохраненные показания
      }
    }

 

    if(work == true) 
    {
          digitalWrite(boiler, LOW);
    } else {
          digitalWrite(boiler, HIGH);
    }
      
  }
}
