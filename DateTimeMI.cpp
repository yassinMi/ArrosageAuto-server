#include "DateTimeMI.h"
#include<Arduino.h>


static  byte monthDays[]={31,28,31,30,31,30,31,31,30,31,30,31};
#define LEAP_YEAR(_year) ((_year%4)==0)  // returns true if year has 366 days

// adds 0 to the left if needed , 8 => "08" , 12 => "12" 

String correctDigitMI(int n){  
    String out = (String) n;
    if(n<10) 
    out = "0" + out;
    return out;
}

DateTimeMI::DateTimeMI(unsigned long tstamp=0){
   timestamp = tstamp;
 }
 
 DateTimeMI::DateTimeMI(){
   timestamp = 0;
 }


void DateTimeMI::Update(unsigned long tstamp ){
   timestamp = tstamp;
   resolveTime(&timestamp, &SecsM, &MinsM, &HoursM, &DayM,&DayW,&MonthM, &YearM);
}
void DateTimeMI::Update(){
   resolveTime(&timestamp, &SecsM, &MinsM, &HoursM, &DayM,&DayW,&MonthM, &YearM);
}

String DateTimeMI::ToString(){
     return correctDigitMI(HoursM)+":"+ correctDigitMI(MinsM)+":"+correctDigitMI(SecsM) + "  " + correctDigitMI(DayM)+"/"+correctDigitMI(MonthM);


}


void DateTimeMI::resolveTime(unsigned long *timep,byte *psec,byte *pmin,byte *phour,byte *pday,byte *pwday,byte *pmonth,int *pyear){


// this is a more compact version of the C library localtime function
  unsigned long epoch=*timep;
  byte year;
  byte month, monthLength;
  unsigned long days;
  *psec=epoch%60;
  epoch/=60; // now it is minutes
  *pmin=epoch%60;
  epoch/=60; // now it is hours
  *phour=epoch%24;
  epoch/=24; // now it is days
  *pwday=(epoch+4)%7;
  
  year=70;  
  days=0;
  while((unsigned)(days += (LEAP_YEAR(year) ? 366 : 365)) <= epoch) {
    year++;
  }
  *pyear=year+1900; // *pyear is returned as years from 1900
  days -= LEAP_YEAR(year) ? 366 : 365;
  epoch -= days; // now it is days in this year, starting at 0
  //*pdayofyear=epoch;  // days since jan 1 this year
  days=0;
  month=0;
  monthLength=0;
  for (month=0; month<12; month++) {
    if (month==1) { // february
      if (LEAP_YEAR(year)) {
        monthLength=29;
      } else {
        monthLength=28;
      }
    } else {
      monthLength = monthDays[month];
    }
    
    if (epoch>=monthLength) {
      epoch-=monthLength;
    } else {
        break;
    }
  }
  *pmonth=month+1;  // jan is month 1
  *pday=epoch+1;  // day of month
}


unsigned long DateTimeMI::getTime(byte sec, byte min, byte hour, byte day, byte month, int year ){

    // note year argument is full four digit year (or digits since 2000), i.e.1975, (year 8 is 2008)
  
   int i;
   unsigned long seconds;

   month= month-1;
   if(year < 69) 
      year+= 2000;
    // seconds from 1970 till 1 jan 00:00:00 this year
    seconds= (year-1970)*(60*60*24L*365);

    // add extra days for leap years
    for (i=1970; i<year; i++) {
        if (LEAP_YEAR(i)) {
            seconds+= 60*60*24L;
        }
    }
    // add days for this year
    for (i=0; i<month; i++) {
      if (i==1 && LEAP_YEAR(year)) { 
        seconds+= 60*60*24L*29;
      } else {
        seconds+= 60*60*24L*monthDays[i];
      }
    }

    seconds+= (day-1)*3600*24L;
    seconds+= hour*3600L;
    seconds+= min*60L;
    seconds+= sec;
    return seconds; 
}





