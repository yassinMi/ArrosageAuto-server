
//esp32 version - the IO wrapper class

#include <Arduino.h>
//wrapper class for io control, there is a Dev verions that is supposed to work with uno, and a win version
class  IObase {
protected:
  virtual int mapPin(int deviceID) = 0;
  virtual void _SetValue(int pin, bool value) = 0;
  virtual void _SetMode(int pin, bool mode) = 0;
public:
  void SetValue(int deviceID, bool state) {
    _SetValue(mapPin(deviceID), state);
    //Serial.write("deviceID : "); Serial.write('0' + deviceID); Serial.println(state ? String(" turned on") : String(" turned off"));
  }
  //mode1= input, 0 = output
  void SetMode(int deviceID, bool mode) {
    _SetMode(mapPin(deviceID), mode);
    
  }
};

const int PinsMapping[]  =  {0,1,2,3,4,5,6,7,8,9,10,11,12,13};//not used for now
//the dev IO currentely used
class IO : public IObase {
  void _SetValue(int pin, bool value) override {  
  if(((int) value) != digitalRead(pin))
  {
    Serial.print("### SET PIN VALUE GPIO#");Serial.print(pin);Serial.println(value?"ON":"OFF"); 

  }
  digitalWrite(pin, value);  
  
  }
  void _SetMode(int pin, bool mode) override { 
  pinMode(pin, mode? INPUT: OUTPUT); 
  Serial.print("### SET PIN MODE GPIO#");Serial.print(pin);Serial.println(mode?"INPUT":"OUTPUT"); 
  }
  int mapPin(int deviceID) override { return deviceID ; }
} IO;
