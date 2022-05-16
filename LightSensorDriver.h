//simpe wrapper for LDR analog reading and calibraing
typedef void (*LDRCallbackHandler)(int value);

class LDRDriver{
    public:
    LDRDriver(unsigned long  reading_interval_ms_, int sample_size_, int pin_, LDRCallbackHandler cb_  ){
        sample_size = sample_size_;
        reading_intv = reading_interval_ms_;
        pin = pin_;
        cb = cb_;
        sample= new int[sample_size];
        lastReadTime = 0;
        cc=0;
    }
    void update(){
        unsigned long currMillis = millis();
        if(currMillis-lastReadTime >=reading_intv){
            //Serial.println("intv");
            lastReadTime=(currMillis);
            int curr = analogRead(pin);
            sample[cc]=curr;
            cc ++;
            if(cc==sample_size){
                cc=0;
                cb(getValue());
            }
        }
    }
    //returns the mapped average of the sample content, 
    int getValue(){
        int avr = 0;
        for(int i =0; i< sample_size; i++){
            avr+=sample[i];
        }
        avr = avr/sample_size;
        return map_f(avr);
    }
    private:
    unsigned long lastReadTime;//in millis
    int sample_size;
    int *sample;
    LDRCallbackHandler cb;
    //one reading 
    int reading_intv;
    int pin;
    int cc; //counter used both to perform the callback each after sample_cc readings and to save the index of the currnt readin withing the sample
    //calibrates the raw voltage reading, todo lux
    int map_f(int vs){
        if(vs==0) return 0;
        if(vs >=4094) vs = 4093;
        double a = 8.1;
        double b = -1.5;
        double r1 = 10000;
        //vs is in range 0-4095
        double r = (r1*(4095-(double)vs))/(vs);
        double E = pow(10,(log10(r)-a)/b);
        return (int) E;
        return vs;
        return map(vs,0,4095,0,100);
    }
 
};