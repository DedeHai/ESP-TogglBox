
// macros from DateTime.h 
/* Useful Constants */
#define SECS_PER_MIN  (60UL)
#define SECS_PER_HOUR (3600UL)
#define SECS_PER_DAY  (SECS_PER_HOUR * 24L)

/* Useful Macros for getting elapsed time */
#define numberOfSeconds(_time_) (_time_ % SECS_PER_MIN)  
#define numberOfMinutes(_time_) ((_time_ / SECS_PER_MIN) % SECS_PER_MIN) 
#define numberOfHours(_time_) (( _time_% SECS_PER_DAY) / SECS_PER_HOUR)
#define elapsedDays(_time_) ( _time_ / SECS_PER_DAY)  

void printDigits(byte digits){
 // utility function for digital clock display: prints colon and leading 0
 display.print(":");
 if(digits < 10)
   display.print('0');
 display.print(digits,DEC);  
}

void displaytime(long val){  
//int days = elapsedDays(val);
int hours = numberOfHours(val);
int minutes = numberOfMinutes(val);
int seconds = numberOfSeconds(val);

 // digital clock display of current time
// display.print(days,DEC);
if(hours < 10)
  display.print(' ');  
  display.print(hours);  
 printDigits(minutes);
 printDigits(seconds);

 
}


