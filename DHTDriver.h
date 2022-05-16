typedef void (*DHTCallbackHandler)(int hum_value, int temp_value);


class DHTDriver {
    public:
    DHTDriver(unsigned long  reading_interval_ms_, int pin_, DHTCallbackHandler cb_  ){
        reading_intv = reading_interval_ms_;
        pin = pin_;
        cb = cb_;
        lastReadTime = 0;
        cc=0;
    }
    void update(){
        unsigned long currMillis = millis();
        if(currMillis-lastReadTime >=reading_intv){
            //Serial.println("intv");
            lastReadTime=(currMillis);
            int curr_hum = 18;
            int curr_temp = 27;
                cb(curr_hum, curr_temp);
            
        }
    }
    private:
    unsigned long lastReadTime;//in millis
    DHTCallbackHandler cb;
    //one reading 
    int reading_intv;
    int pin;
    int cc;
};