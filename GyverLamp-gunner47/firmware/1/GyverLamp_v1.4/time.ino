#ifdef USE_NTP

#define RESOLVE_INTERVAL      (5UL * 60UL * 1000UL)                       // интервал проверки подключения к интеренету в миллисекундах (5 минут)
                                                                          // при старте ESP пытается получить точное время от сервера времени в интрнете
                                                                          // эта попытка длится RESOLVE_TIMEOUT
                                                                          // если при этом отсутствует подключение к интернету (но есть WiFi подключение),
                                                                          // модуль будет подвисать на RESOLVE_TIMEOUT каждое срабатывание таймера, т.е., 3 секунды
                                                                          // чтобы избежать этого, будем пытаться узнать состояние подключения 1 раз в RESOLVE_INTERVAL (5 минут)
                                                                          // попытки будут продолжаться до первой успешной синхронизации времени
                                                                          // до этого момента функции будильника работать не будут
                                                                          // интервал последующих синхронизаций времени определяён в NTP_INTERVAL (30 минут)
                                                                          // при ошибках повторной синхронизации времени функции будильника отключаться не будут
#define RESOLVE_TIMEOUT       (1500UL)                                    // таймаут ожидания подключения к интернету в миллисекундах (1,5 секунды)
uint64_t lastResolveTryMoment = 0UL;
bool timeSynched = false;
IPAddress ntpServerIp = {0, 0, 0, 0};
static CHSV dawnColor = CHSV(0, 0, 0);                                    // цвет "рассвета"
static CHSV dawnColorMinus1 = CHSV(0, 0, 0);                              // для большей плавности назначаем каждый новый цвет только 1/10 всех диодов; каждая следующая 1/10 часть будет "оставать" на 1 шаг
static CHSV dawnColorMinus2 = CHSV(0, 0, 0);
static CHSV dawnColorMinus3 = CHSV(0, 0, 0);
static CHSV dawnColorMinus4 = CHSV(0, 0, 0);
static CHSV dawnColorMinus5 = CHSV(0, 0, 0);
static uint8_t dawnCounter = 0;                                           // счётчик первых 10 шагов будильника


void timeTick()
{
  if (espMode == 1U)
  {
    if (timeTimer.isReady())
    {
      if (!timeSynched)
      {
        if (millis() - lastResolveTryMoment >= RESOLVE_INTERVAL || lastResolveTryMoment == 0)
        {
          resolveNtpServerAddress(ntpServerAddressResolved);              // пытаемся получить IP адрес сервера времени (тест интернет подключения) до тех пор, пока время не будет успешно синхронизировано
          lastResolveTryMoment = millis();
          if (!ntpServerAddressResolved)
          {
            #ifdef GENERAL_DEBUG
            LOG.println(F("Функции будильника отключены до восстановления подключения к интернету"));
            #endif
          }
        }
        if (!ntpServerAddressResolved)
        {
          return;                                                         // если нет интернет подключения, отключаем будильник до тех пор, пока оно не будет восстановлено
        }
      }

      timeSynched = timeSynched || timeClient.update();                   // если время хотя бы один раз было синхронизировано, продолжаем
      if (!timeSynched)                                                   // если время не было синхронизиировано ни разу, отключаем будильник до тех пор, пока оно не будет синхронизировано
      {
        return;
      }

      uint8_t thisDay = timeClient.getDay();
      if (thisDay == 0) thisDay = 7;                                      // воскресенье это 0
      thisDay--;
      thisTime = timeClient.getHours() * 60 + timeClient.getMinutes();
      uint32_t thisFullTime = timeClient.getHours() * 3600 + timeClient.getMinutes() * 60 + timeClient.getSeconds();

      printTime(thisTime, false);

      // проверка рассвета
      if (alarms[thisDay].State &&                                                                                          // день будильника
          thisTime >= (uint16_t)constrain(alarms[thisDay].Time - pgm_read_byte(&dawnOffsets[dawnMode]), 0, (24 * 60)) &&    // позже начала
          thisTime < (alarms[thisDay].Time + DAWN_TIMEOUT))                                                                 // раньше конца + минута
      {
        if (!manualOff)                                                   // будильник не был выключен вручную (из приложения или кнопкой)
        {
          // величина рассвета 0-255
          int32_t dawnPosition = 255 * ((float)(thisFullTime - (alarms[thisDay].Time - pgm_read_byte(&dawnOffsets[dawnMode])) * 60) / (pgm_read_byte(&dawnOffsets[dawnMode]) * 60));
          dawnPosition = constrain(dawnPosition, 0, 255);
          dawnColorMinus5 = dawnCounter > 4 ? dawnColorMinus4 : dawnColorMinus5;
          dawnColorMinus4 = dawnCounter > 3 ? dawnColorMinus3 : dawnColorMinus4;
          dawnColorMinus3 = dawnCounter > 2 ? dawnColorMinus2 : dawnColorMinus3;
          dawnColorMinus2 = dawnCounter > 1 ? dawnColorMinus1 : dawnColorMinus2;
          dawnColorMinus1 = dawnCounter > 0 ? dawnColor : dawnColorMinus1;
          dawnColor = CHSV(map(dawnPosition, 0, 255, 10, 35),
                           map(dawnPosition, 0, 255, 255, 170),
                           map(dawnPosition, 0, 255, 10, DAWN_BRIGHT));
          dawnCounter++;
          // fill_solid(leds, NUM_LEDS, dawnColor);
          for (uint16_t i = 0U; i < NUM_LEDS; i++)
          {
            if (i % 6 == 0) leds[i] = dawnColor;                          // 1я 1/10 диодов: цвет текущего шага
            if (i % 6 == 1) leds[i] = dawnColorMinus1;                    // 2я 1/10 диодов: -1 шаг
            if (i % 6 == 2) leds[i] = dawnColorMinus2;                    // 3я 1/10 диодов: -2 шага
            if (i % 6 == 3) leds[i] = dawnColorMinus3;                    // 3я 1/10 диодов: -3 шага
            if (i % 6 == 4) leds[i] = dawnColorMinus4;                    // 3я 1/10 диодов: -4 шага
            if (i % 6 == 5) leds[i] = dawnColorMinus5;                    // 3я 1/10 диодов: -5 шагов
          }
          FastLED.setBrightness(255);
          delay(1);
          FastLED.show();
          dawnFlag = true;
        }
      }
      else
      {
        // не время будильника (ещё не начался или закончился по времени)
        if (dawnFlag)
        {
          dawnFlag = false;
          FastLED.clear();
          delay(2);
          FastLED.show();
          changePower();                                                  // выключение матрицы или установка яркости текущего эффекта в засисимости от того, была ли включена лампа до срабатывания будильника
        }
        manualOff = false;
        dawnColorMinus1 = CHSV(0, 0, 0);
        dawnColorMinus2 = CHSV(0, 0, 0);
        dawnColorMinus3 = CHSV(0, 0, 0);
        dawnColorMinus4 = CHSV(0, 0, 0);
        dawnColorMinus5 = CHSV(0, 0, 0);
        dawnCounter = 0;
      }
    }
  }
}

void resolveNtpServerAddress(bool &ntpServerAddressResolved)              // функция проверки подключения к интернету
{
  if (ntpServerAddressResolved)
  {
    return;
  }

  WiFi.hostByName(NTP_ADDRESS, ntpServerIp, RESOLVE_TIMEOUT);
  if (ntpServerIp[0] <= 0)
  {
    #ifdef GENERAL_DEBUG
    if (ntpServerAddressResolved)
    {
      LOG.println(F("Подключение к интернету отсутствует"));
    }
    #endif

    ntpServerAddressResolved = false;
  }
  else
  {
    #ifdef GENERAL_DEBUG
    if (!ntpServerAddressResolved)
    {
      LOG.println(F("Подключение к интернету установлено"));
    }
    #endif

    ntpServerAddressResolved = true;
  }
}
#endif
