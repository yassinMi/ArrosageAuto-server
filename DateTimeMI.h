#include<Arduino.h>

#define DATE_TIME_MI_H

class DateTimeMI{

   public:
   byte SecsM, MinsM, HoursM, DayM, MonthM,  DayW;
   int YearM;
   unsigned long timestamp;
   DateTimeMI (unsigned long tstamp);
   DateTimeMI ();
   void Update(unsigned long tstamp  );
   void Update();
   String ToString();
   void resolveTime(unsigned long *timep,byte *psec,byte *pmin,byte *phour,byte *pday,byte *pwday,byte *pmonth,int *pyear);
  static unsigned long getTime(byte sec, byte min, byte hour, byte day, byte month, int year );

};
