#define BUFFER_SIZE 128
char buffer[BUFFER_SIZE];

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h> // https://github.com/maniacbug/RF24
//#include <avr/sleep.h>
#include <EEPROM.h>
//#include <PCF8814.h> 
//#include <math.h>
//#include <avr/wdt.h>
#include <Wire.h>
#include <DS3231.h>


#define boiler 4
#define light A0
#define wakePin 2
#define ring 5
#define led 6

DS3231 clock;
RTCDateTime dt;

//PCF8814 Lcd(13,11,10,6); 
//PCF8814 Lcd(7,8,5,6);
RF24 radio(9, 10); // CE, CSN

const uint64_t pipe = 0xF0F0F0F0E1LL; // индитификатор передачи, "труба"
bool work = false;
boolean alarmState = false;
float sumTemperature = 0.0;
float temperature = 0.0;
float Temp;// = 20.8;
float gisteresis;// = 0.05;
float mintemp = 50;
float maxtemp = 0;
float cycle = 0;
float buff_cy,maxtemp_cy,mintemp_cy,temp_on,temp_off;
float data[2];
int up_dwn;
float temp_buff;
int counter = 0;
int error = 0;
int do_error = 0;
String ltime;
long previousMillis = 0;
int command;// = 1;


void setup(){
  pinMode(led, OUTPUT);
  digitalWrite(led, HIGH); 
  pinMode(wakePin, INPUT);
  digitalWrite(wakePin, HIGH);               // Подтягивем ногу к 5.
  //attachInterrupt(0, wakeUpNow, FALLING);  
  pinMode(boiler, OUTPUT);
  pinMode(ring, OUTPUT);

  
  
  //Serial.begin(9600);

  
  delay(500);
  clock.begin();
  //Lcd.Init();
  radio.begin();  
  delay(5);  
  //radio.setChannel(100);
  //radio.setDataRate(RF24_250KBPS);     
  //radio.setPALevel(RF24_PA_HIGH);   
  radio.openReadingPipe(1,pipe); // открываем первую трубу с индитификатором "pipe"
  radio.startListening(); // включаем приемник, начинаем слушать трубу
  delay(20);

  //Lcd.Mirror(1,1);

  //clock.setDateTime(2015, 12, 31, 14, 14, 00);
  
  //EEPROM_float_write(0, 20.8);
  //Serial.println("1");
  //EEPROM.write(0, 20.00);
  //Serial.println("2");
  //EEPROM.write(5, 0.05);
  //Serial.println("3");
  //EEPROM.write(9, 0);
  //  Serial.println("4");
  Temp = EEPROM_float_read(0);
  gisteresis = EEPROM_float_read(9);
  command = EEPROM_float_read(5);

  cycle = 3;
  maxtemp_cy = Temp + gisteresis;
  mintemp_cy = Temp - gisteresis;
  temp_on = Temp;
  temp_off = Temp; 
  up_dwn = 1;//режим выключенного котла
}
/*
// watchdog interrupt
ISR (WDT_vect) 
{
   wdt_disable();  // disable watchdog
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
  MCUCR = bit (BODS) | bit (BODSE);
  MCUCR = bit (BODS); 
  sleep_cpu ();  


  // cancel sleep as a precaution
  ADCSRA = adcsra_save;  // восстанавливаем режим работы ацп 
  sleep_disable();
}
*/


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



void loop()   
{
   dt = clock.getDateTime();
   char outstr[9];
   int ch_id, packet_len;
   char *pb; 

  if(millis() - previousMillis > 1000){
    previousMillis = millis();
    do_error++;

  digitalWrite(led, LOW);
  /*
  if(Serial.available()) {
    int var1 = Serial.parseInt();
    int var2 = Serial.parseInt();
    int var3 = Serial.parseInt();
    int var4 = Serial.parseInt();
    String buff = (String)var2 + '.' + (String)var3 + (String)var4;
    switch (var1) {
      case 0:
        command = var2;
        EEPROM_float_write(5, command);
        Serial.print("Modify output to: ");
        Serial.println(command);
        break;
      case 1:
        Temp = buff.toFloat();;
        EEPROM_float_write(0, buff.toFloat());
        Serial.print("Modify temp to: ");
        Serial.println(Temp);
        break;
      case 2:
        gisteresis = buff.toFloat();;
        EEPROM_float_write(9, buff.toFloat());
        Serial.print("Modify gisteresis to: ");
        Serial.println(gisteresis);
        break;
      default:
        Serial.println("Command error");
        Serial.println("Help:");
        Serial.println("0 - modify output");
        Serial.println("1 - modify temp");
        Serial.println("2 - modify gisteresis");
        Serial.println("Example: 2,0,0,5 - mommand 2, number 0,05");
        Serial.println("Inpute data:");
        Serial.println(var1);
        Serial.println(var2);
        Serial.println(var3);
        Serial.println(var4);
        Serial.println(buff);

        }
  }    
  */


    if (radio.available()){ // проверяем не пришло ли чего в буфер.
      bool done = false;
      while (!done)
      {
        done = radio.read(data, sizeof(data)); // читаем данные и указываем сколько байт читать

        delay(20);
        do_error=0;
        error=0;
        noTone(ring);
        digitalWrite(led, HIGH);
        
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

          
        
          
          if(cycle <= 2)//первый цикл пустой, второй калибровочный - в этом режиме работаем по старой схеме
          {
            if(temperature <= (Temp - gisteresis) and work != true) {
                work = true;
                ltime = clock.dateFormat("H:i:s", dt);
                temp_on = temperature;
                cycle = cycle + 0.5; //1 такт = пол цикла
                up_dwn = 2;
            } 
            
            if(temperature >= (Temp + gisteresis) and work == true) {
                work = false;
                ltime = clock.dateFormat("H:i:s", dt);
                temp_off = temperature;
                cycle = cycle + 0.5; //1 такт = пол цикла
                up_dwn = 0;
            }
          }
          if(cycle == 2)//приравниваем температуры
          {
              maxtemp_cy = maxtemp;
              mintemp_cy = mintemp;
          }
          if(cycle > 2){//послекалебровочные циклы, работают с постоянной корекцией температуры включения\ выключения  котла для поддержание наименьшего гестерезиса
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
                if(temperature <= temp_on and work != true) {
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
                if(temperature >= temp_off and work == true) {
                    work = false;
                    ltime = clock.dateFormat("H:i:s", dt);
                    cycle = cycle + 0.5; //1 такт = пол цикла
                    up_dwn = 0;
                }
                break;                
              default:
                
              break;
             }
          }
          
          //конец цикла
        }
        else{
          sumTemperature = (sumTemperature + data[0]);
          counter++;
        }
      }
      radio.stopListening();
      radio.startListening();
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
    
    if (isnan(data[0])) {
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

    /*      
    if(command == 1){
      Serial.print("ab");      
      Serial.print(data[0]);
      Serial.print("bacd");
      Serial.print(temperature);
      Serial.print("dcef");
      Serial.print(work);
      Serial.print("fegi");
      Serial.print(maxtemp);
      Serial.print("ighj");
      Serial.print(mintemp);
      Serial.print("jhkl");
      Serial.print(data[1]);
      Serial.print("lk");      
      Serial.println(" ");      
    }
    if(command == 2){
      Serial.print("Termostat:");
      if (isnan(data[0])) {
          Serial.println("ERROR");
      }else{
          Serial.println("Ok");
      }
      delay(50);
      
      Serial.print("Boiler: ");
        if(work == true) {
            Serial.println("On");
        } else {
            Serial.println("Off");
        }
      delay(50);
      
      Serial.print("Time: ");
      Serial.println(clock.dateFormat("d-m-Y H:i:s", dt));
      delay(50);

      Serial.print("Time on/off boiler: ");  
      Serial.println(ltime);
      delay(50);
      
      Serial.print("Himidity: ");
      Serial.println(data[1]);
      delay(50);
      
      Serial.print("Temp: ");
      Serial.println(temperature);
      delay(50);
      
      Serial.println(" ");         
    }
    if(command == 3){
      Serial.print("||q||");

      //Serial.print("FreeRam: ");  
      Serial.println(freeRam ()); 
            
      //Serial.print("Temp: ");  
      Serial.println(Temp); 

      //Serial.print("temperature: ");  
      Serial.println(temperature);      

      //Serial.print("curent temperature: ");  
      Serial.println(data[0]);      

      //Serial.print("sumTemperature: ");  
      Serial.println(sumTemperature);

      //Serial.print("gisteresis: ");  
      Serial.println(gisteresis);  

      //Serial.print("maxtemp: ");  
      Serial.println(maxtemp);  
      
      //Serial.print("mintemp: ");  
      Serial.println(mintemp);   
      
      //Serial.print("maxtemp_cy: ");  
      Serial.println(maxtemp_cy);  
     
      //Serial.print("mintemp_cy: ");  
      Serial.println(mintemp_cy);   
        
      //Serial.print("temp_on: ");  
      Serial.println(temp_on);  

      //Serial.print("temp_off: ");  
      Serial.println(temp_off);     

      //Serial.print("buff_cy: ");  
      Serial.println(buff_cy);
                 
      //Serial.print("counter: ");  
      Serial.println(counter);  

      //Serial.print("do_error: ");  
      Serial.println(do_error);  

      //Serial.print("error: ");  
      Serial.println(error);  

      //Serial.print("work: ");  
      Serial.println(work);       
 
      //Serial.print("up_dwn: ");  
      Serial.println(up_dwn);
      
      //Serial.print("cycle: ");  
      Serial.println(cycle);  

      //Serial.print("command: ");  
      Serial.println(command);  
      
      //Serial.print("ltime: ");  
      Serial.println(ltime);
        
      //Serial.print("Time: ");  
      Serial.println(clock.dateFormat("H:i:s", dt)); 
       
      Serial.print("||z||");                                    
      Serial.println(" ");   
    }
    */
    if(work == true) 
    {
          digitalWrite(boiler, LOW);
    } else {
          digitalWrite(boiler, HIGH);
    }
      
  }
}
