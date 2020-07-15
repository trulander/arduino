//#define BUFFER_SIZE 128
//char buffer[BUFFER_SIZE];

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <EEPROM.h>
#include <Wire.h>
#include <DS3231.h>


#define boiler 4
//#define wakePin 2
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
bool work;
boolean alarmState;
float sumTemperature;
float temperature;
float Temp;
float gisteresis;
float mintemp;
float maxtemp;
float cycle;
float buff_cy,maxtemp_cy,mintemp_cy,temp_on,temp_off;
float data[3];
int up_dwn;

int counter;
int error;
int do_error;
String ltime;
long previousMillis;
int command;
int interval[4];
float interval_temp[2];
int alarm_clock;
int p_hour_i;//interval hour
int p_hour_c;//current hour
float interbuff;//буферная переменная для расчета температуры по суточному таймеру

void setup(){
  pinMode(led_input, OUTPUT);
  pinMode(led_watchdog, OUTPUT);
  digitalWrite(led_input, HIGH);
  digitalWrite(led_watchdog, HIGH); 
 // pinMode(wakePin, INPUT);
 // digitalWrite(wakePin, HIGH);               // Подтягивем ногу к 5в.
  //attachInterrupt(0, wakeUpNow, FALLING);  
  pinMode(boiler, OUTPUT);
  pinMode(ring, OUTPUT);

  Serial.begin(9600);
  
  delay(500);
  clock.begin();



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
  command = EEPROM_float_read(5);
  gisteresis = EEPROM_float_read(9);
  interval[0] = EEPROM_float_read(14);
  interval[1] = EEPROM_float_read(19);
  interval[2] = EEPROM_float_read(24);
  interval[3] = EEPROM_float_read(29);
  interval_temp[0] = EEPROM_float_read(35);
  interval_temp[1] = EEPROM_float_read(41);
  alarm_clock = 0;
  mintemp = 50;
  maxtemp = 0;
  cycle = 0;
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
  alarmState = false;
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

int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
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
   dt = clock.getDateTime();
   //char outstr[9];
   //int ch_id, packet_len;
   //char *pb; 

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
        case 1:
          command = var2;
          EEPROM_float_write(5, command);
          Serial.print("Out: ");
          Serial.println(command);
          break;
        case 2://записываем в память еепром новое значение температуры, но не присваиваем переменной temp новое значение, оно присвоится на следующей инерации проверкой ниже в суточном таймере
          EEPROM_float_write(0, var2);//дополнительные действия не требуются для корректировки температуры включения\выключения, ниже условие в интервале уже это проверяет
          Serial.print("Temp: ");
          Serial.println(Temp);
          break;
        case 3:
          gisteresis = var2;
          EEPROM_float_write(9, var2);
          Serial.print("Gisteresis: ");
          Serial.println(gisteresis);
          break;
        case 4:
          interval[0] = var2;
          interval[1] = var3;
          interval_temp[0] = var4;
          EEPROM_float_write(35, var4);//temp
          EEPROM_float_write(14, var2);//interval[1] время стартовое
          EEPROM_float_write(19, var3);//interval[2] время окончания        
          Serial.print("Interval 1: ");
          Serial.print(interval[0]); 
          Serial.print(" - "); 
          Serial.print(interval[1]); 
          Serial.print(" : "); 
          Serial.println(interval_temp[0]);
          break;
        case 5:
          interval[2] = var2;
          interval[3] = var3;     
          interval_temp[1] = var4;
          EEPROM_float_write(41, var4);   //temp
          EEPROM_float_write(24, var2);//interval[3] время стартовое второе
          EEPROM_float_write(29, var3);//interval[4] время окончания второе        
          Serial.print("Interval 2: ");
          Serial.print(interval[2]); 
          Serial.print(" - "); 
          Serial.print(interval[3]); 
          Serial.print(" : "); 
          Serial.println(interval_temp[1]);
          break;
        case 6:
          Serial.println("?");
          alarm_clock = 1;
          break;                                        
        default:
          Serial.println("Help:");
          Serial.println("1-output");
          Serial.println("2-temp");
          Serial.println("3-gisteresis");
          Serial.println("4-interval 1 (4,8,10,0.5) 8-10 temp +0.5");
          Serial.println("5-interval 2");
          Serial.println("6");                        
          //Serial.println(var1);
          //Serial.println(var2);
          //Serial.println(var3);
  
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
        noTone(ring);
        digitalWrite(led_input, HIGH);
        
        if(counter == 4) 
        {
    
          temperature = sumTemperature / 4;
          sumTemperature = 0;
          counter = 0;

          //Начало цикла


          //определение максимальной и минимальной температуры
          if(mintemp > temperature){
            mintemp = temperature;
          }
          if(maxtemp<temperature){
            maxtemp = temperature;
          }

          
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
                    ltime = clock.dateFormat("H:i:s", dt);
                    cycle = cycle + 0.5; //1 такт = пол цикла
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
                    ltime = clock.dateFormat("H:i:s", dt);
                    cycle = cycle + 0.5; //1 такт = пол цикла
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
    }

    if(error >= 5){//ошибка чтения данных с датчика больше 125сек
      work = true;//включаем котел
      data[0]=NAN;//выводим температуру в неизвестное
      up_dwn = 3;//переводим рабочий цикл в положение включенного котла
    }
    
   if (data[2] < 3.6 && data[2] > 1) {
      alarmState = !alarmState; 
      if(alarmState){
        tone(ring, 1000);
        delay(5);
        tone(ring, 1150);
        delay(5);
        tone(ring, 1300);
        delay(5);
        tone(ring, 1450);
        delay(5);
        tone(ring, 1600);
        delay(5);
        tone(ring, 1750);
        delay(5);
        tone(ring, 1900);
        delay(5);
        tone(ring, 2050);
        delay(5);
        tone(ring, 2200);
        delay(5);
        tone(ring, 2350);
        delay(5);
        tone(ring, 2500);
        delay(5);
        tone(ring, 2650);
        delay(5);
        tone(ring, 2800);
        delay(5);
        tone(ring, 2950);
        delay(5);
        tone(ring, 3100);
        delay(5);
        tone(ring, 3250);        
        delay(5);
        tone(ring, 3400);
        delay(5);
        tone(ring, 3550);
        delay(5);
        tone(ring, 3700);
        delay(5);
        tone(ring, 3850);
      }else{
        noTone(ring);
      }    
    }else{
        noTone(ring);
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
      //if(max(interval_temp[0],0) > 0){//проверяем является ли прибавочная теммпература положительной и прибавляем ее, если отрицательная то вычитаем
        interbuff = EEPROM_float_read(0) + interval_temp[0];
        if(max(interval_temp[0],0) > 0){//если прибавочная температура отрицательная, то устанавливаем новую температуру включения, если нет, то устанавливаем температуру выключения
          temp_off = interbuff - gisteresis;
        }else{
          temp_on = interbuff + gisteresis;
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
        interbuff = EEPROM_float_read(0) + interval_temp[1];
        if(max(interval_temp[1],0) > 0){//если прибавочная температура отрицательная, то устанавливаем новую температуру включения, если нет, то устанавливаем температуру выключения
          temp_off = interbuff - gisteresis;
        }else{
          temp_on = interbuff + gisteresis;
        }
        if(Temp != interbuff){//проверяем изменяли ли мы уже временную температуру или нет
          Temp = interbuff;//задаем новую температуру, она будет временной, так как не записываем ее в еепром.
        }
      }else{//если все таймеры прошли и показания разнятся, обнуляем температуру до заданной в настройках, это условие справедливо и при ручной смене температуры в настройках, так что дополнительных проверок не требуется
        if(Temp != EEPROM_float_read(0)){
          if(Temp < EEPROM_float_read(0)){//
            temp_off = EEPROM_float_read(0) - gisteresis;
          }else{
            temp_on = EEPROM_float_read(0) + gisteresis;
          }
          Temp = EEPROM_float_read(0);//обнуляем на сохраненные показания
        }
      }
    }

    //alarm_clock
    if(alarm_clock > 0){
      tone(ring, 392, 350);
      delay(350);
      tone(ring, 392, 350);
      delay(350);
      tone(ring, 392, 350);
      delay(350);
      tone(ring, 311, 250);
      delay(250);
      tone(ring, 466, 100);
      delay(100);
      tone(ring, 392, 350);
      delay(350);
      tone(ring, 311, 250);
      delay(250);
      tone(ring, 466, 100);
      delay(100);
      tone(ring, 392, 700);
      delay(700);
      
      tone(ring, 587, 350);
      delay(350);
      tone(ring, 587, 350);
      delay(350);
      tone(ring, 587, 350);
      delay(350);
      tone(ring, 622, 250);
      delay(250);
      tone(ring, 466, 100);
      delay(100);
      tone(ring, 369, 350);
      delay(350);
      tone(ring, 311, 250);
      delay(250);
      tone(ring, 466, 100);
      delay(100);
      tone(ring, 392, 700);
      delay(700);
      
      tone(ring, 784, 350);
      delay(350);
      tone(ring, 392, 250);
      delay(250);
      tone(ring, 392, 100);
      delay(100);
      tone(ring, 784, 350);
      delay(350);
      tone(ring, 739, 250);
      delay(250);
      tone(ring, 698, 100);
      delay(100);
      tone(ring, 659, 100);
      delay(100);
      tone(ring, 622, 100);
      delay(100);
      tone(ring, 659, 450);
      delay(450);
      
      tone(ring, 415, 150);
      delay(150);
      tone(ring, 554, 350);
      delay(350);
      tone(ring, 523, 250);
      delay(250);
      tone(ring, 493, 100);
      delay(100);
      tone(ring, 466, 100);
      delay(100);
      tone(ring, 440, 100);
      delay(100);
      tone(ring, 466, 450);
      delay(450);
      
      tone(ring, 311, 150);
      delay(150);
      tone(ring, 369, 350);
      delay(350);
      tone(ring, 311, 250);
      delay(250);
      tone(ring, 466, 100);
      delay(100);
      tone(ring, 392, 750);
      alarm_clock = 0;
    }
      
    if(command == 3){
      Serial.println("||q||");
      Serial.println("");
      Serial.print("Ram: ");  
      Serial.println(freeRam ()); 

      Serial.print("T(-->): ");  
      Serial.println(EEPROM_float_read(0));       
            
      Serial.print("T(-->+I): ");  
      Serial.println(Temp); 

      Serial.print("T(-><-): ");  
      Serial.println(temperature);      

      Serial.print("T(!): ");  
      Serial.println(data[0]);      

      Serial.print("Gis(+/-): ");  
      Serial.println(gisteresis);  

      Serial.print("T(max): ");  
      Serial.println(maxtemp);  
      
      Serial.print("T(min: ");  
      Serial.println(mintemp);   
      
      Serial.print("T(max->): ");  
      Serial.println(maxtemp_cy);  
     
      Serial.print("T(min->): ");  
      Serial.println(mintemp_cy);   
        
      Serial.print("T(on): ");  
      Serial.println(temp_on);  

      Serial.print("T(off): ");  
      Serial.println(temp_off);     

      Serial.print("T(buf): ");  
      Serial.println(buff_cy);
                 
      Serial.print("C(+): ");  
      Serial.println(counter);  

      Serial.print("C(de): ");  
      Serial.println(do_error);  

      Serial.print("C(e): ");  
      Serial.println(error);  

      Serial.print("C(cy): ");
      Serial.println(cycle); 
       
      Serial.print("Work: ");  
      Serial.println(work);       
 
      Serial.print("Step: ");  
      Serial.println(up_dwn);      

      Serial.print("Time(l): ");  
      Serial.println(ltime);
        
      Serial.print("Time(c): ");  
      Serial.println(clock.dateFormat("H:i:s", dt)); 

      Serial.print("himidity: ");
      Serial.println(data[1]);
      
      Serial.print("vcc(clock): ");
      Serial.println(data[2]); 

      Serial.print("vcc(baze): ");
      Serial.println(((float)readVcc())/1024.0);       

      Serial.print("Inter(1): ");
      Serial.print(interval[0]); 

      Serial.print(" - "); 

      Serial.print(interval[1]); 
      
      Serial.print(" : ");       
      
      Serial.println( interval_temp[0]); 

      Serial.print("Inter(2): ");
      Serial.print(interval[2]); 
      
      Serial.print(" - "); 
                     
      Serial.print(interval[3]);  
      
      Serial.print(" : ");       
      
      Serial.println( interval_temp[1]);   

      Serial.print("p_hour_i: "); 
      Serial.println(p_hour_i); 
      Serial.print("p_hour_c: "); 
      Serial.println(p_hour_c);            
           
      Serial.print("||z||");                                    
      Serial.println(" ");   
    }

    if(work == true) 
    {
          digitalWrite(boiler, LOW);
    } else {
          digitalWrite(boiler, HIGH);
    }
      
  }
}
