

#ifndef DATE_TIME_MI_H
#include "DateTimeMI.h"
#endif




//a reused class from horloge 2021
//24-march : switched to using undigned long 
class Clock {
  unsigned long prevMillis;

public:
  DateTimeMI CurrentTime;
  unsigned long millisCounter;

  void Init(DateTimeMI time) {
    Serial.println("assign cuurr time");
    CurrentTime = time;
    Serial.println("call update");
    time.Update();
    Serial.println("assign prev milis");
    prevMillis = 0;
    millisCounter = 0;

  }
  void Update() {
    unsigned long currentMillis = millis();
    unsigned long deltaTime = abs(currentMillis - prevMillis);
    prevMillis = currentMillis;
    millisCounter += deltaTime;
    while (millisCounter >= 1000) {
      millisCounter -= 1000;
      CurrentTime.timestamp++;
      CurrentTime.Update();
      //Serial.println(CurrentTime.timestamp);

    }
  }
  uint64_t getTimeStampMillis() {
    return (((uint64_t)CurrentTime.timestamp) * 1000ull) + ((uint64_t)millisCounter);
  }
  void Set(byte secs, byte mins, byte hours, byte day, byte month, int year) {
    CurrentTime.Update(DateTimeMI::getTime(secs, mins, hours, day, month, year));
  }
  //untested (added on 29-april-2022)
  void Set(uint64_t ts_in_millis) {
    Serial.println("calling CurrentTime.Update");
    CurrentTime.Update((unsigned long)(ts_in_millis / (1000ul)));
    Serial.println("calcul mod");
    millisCounter = ts_in_millis % (1000ul);
  }

  void Increase(int targetProperty, int step) {
    CurrentTime.Update(DateTimeMI::getTime(
      CurrentTime.SecsM + (targetProperty == 3 ? step : 0),
      CurrentTime.MinsM + (targetProperty == 2 ? step : 0),
      CurrentTime.HoursM + (targetProperty == 1 ? step : 0),
      CurrentTime.DayM + (targetProperty == 4 ? step : 0),
      CurrentTime.MonthM + (targetProperty == 5 ? step : 0),
      CurrentTime.YearM + (targetProperty == 6 ? step : 0)));
  }

};




