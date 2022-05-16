
//in sync with the windows testing version (26-april-2022)
//string class is altered

#define mi_device_h
#include "IOESP32.h"

typedef enum{ none=0, manual, automated } ConfigMode;
typedef enum{ lt, gt } ConditionType;
typedef enum{ temp, hum } ClimateVariable;
typedef void(fc)(void * device);



struct Condition
{
  private:
  public:
  Condition(){

  }
  Condition(ClimateVariable var_, ConditionType type_, float param_ ){
    type= type_;
    param1 = param_;
    targetVariable = var_;
  }
  ConditionType type;
  ClimateVariable targetVariable;
  float param1;
  //float param2;

  bool Check(int curr_hum, int curr_temp){
    //todo implementation 
    
    float v = (targetVariable==temp)?curr_temp : curr_hum;
    return (type==lt && v < param1 ) || (type==gt && v >= param1 );
  }

};


class AutoOptions {
  private:
  
  public:
  AutoOptions(){
    startsAt = 1648870200000ull;
    duration = 30*60;
    repeatEvery = 24*3600;
    conditions_cc = 1;
    conditions = new Condition[1]{ Condition(temp,gt,50)};
  }
  AutoOptions( uint64_t start, int dur, int rep){
    startsAt = start;
    duration = dur;
    repeatEvery = rep;
    conditions_cc = 0;
    conditions = NULL;
  }
  AutoOptions(uint64_t start, int dur, int rep,Condition * conds, int conds_cc ){
    startsAt = start;
    duration = dur;
    repeatEvery = rep;
    conditions_cc = conds_cc;
    conditions = conds;
  }
  //millis
  uint64_t startsAt;
  //millis
  int duration;
  //millis
  int repeatEvery;
  Condition * conditions;
  int conditions_cc; //added on win version

  const static AutoOptions getDefault(){return AutoOptions(); }
};


struct DeviceConfig
{
private:
  /* data */
public:
DeviceConfig(){
    autoOptions = AutoOptions::getDefault();

}
  DeviceConfig(ConfigMode mode, String lbl, AutoOptions autoOpts, bool mState){
    this->mode = mode;
    label = lbl;
    this->autoOptions = autoOpts;
    manualState = mState;
    
  }
  ConfigMode mode;
  String label;
  AutoOptions autoOptions;
  bool manualState;
}  ;





//provids methods for controling the devices state
class Device {
public:
  
  Device(int id, String lbl){
    ID=id; currentState=false;
    //pinMode(id, OUTPUT);
    Config = DeviceConfig(manual,lbl,AutoOptions::getDefault(),true);
    label = String(lbl);
  }
  //todo copy change on win
  Device(int id, String lbl,DeviceConfig config){
    ID=id; currentState=false;
    //pinMode(id, OUTPUT);
    Config = config;
    label = String(lbl);
  }
  
  Device(){
    ID=0; currentState=false;
    Config = DeviceConfig(manual,String("untitled plant"),AutoOptions::getDefault(),true);
  }
  ~Device(){
    //IO.SetMode(ID, 1);
  }
  void Init(){
    IO.SetMode(ID, 0);
  }
  void setState(bool state){
    if(currentState==state) return;
    currentState=state;
    IO.SetValue(ID,state);
  }
  int ID;// idealy the ID and pin would be two destinct fields with ID being normalized and pin being a generic abstraction to whatever the underlying gpio numbers can be
  
  String label;
  bool currentState;

  //safely update the Config .
  void SetConfig(DeviceConfig newConfig){
    //some pocedure that cleans whatever Config and schduling-related state 
    Config = newConfig;
    //other procedure that may be useful to reset things, this is just me predictng where bugs may show be at adn addressign them 
  }

  //safely update the Config .
  void SetAutoOptions(AutoOptions newOpts){
    Config.autoOptions= newOpts;
  }
  //get config for read only ops
  const DeviceConfig *GetConfig(){ return &Config;}

  //to be called regulrily in loop, this is where most of the device functianalty logic is carried out
  void update(unsigned long long tm);
  public:
  DeviceConfig  Config= DeviceConfig(automated,String("myd"),AutoOptions::getDefault(),true);
  

};



class DevicesCollection {
  private:
  Device *content;
  int itemsCount;

  public:
  DevicesCollection(int capacity){
    itemsCount = capacity;
    content = new Device[capacity];
  }
  DevicesCollection(){
    itemsCount = 0;
  }
  int Count(){
    return itemsCount;
  }
  //fails and returns false if the targed id already exists, true otherwise
  bool Add(Device newDevice){
    printf("adding new device id=%d lbl=%s \n",newDevice.ID,newDevice.GetConfig()->label.c_str());
    if(getDeviceByID(newDevice.ID)!=NULL){
      printf("not added, another device with id=%d exists \n",newDevice.ID);
      return false;
    }
    Device * new_content = new Device[itemsCount+1];
    for(int i=0; i<itemsCount;i++){
       new_content[i]= content[i];
    }
    new_content[itemsCount] = newDevice;

    //delete content;
    content = new_content;
    itemsCount++;
    printf("itemsCount=%d  \n",itemsCount);
    return true;

  }

  bool Remove(int ID){
    Device *target = getDeviceByID(ID);
    if(target==NULL){
      return false;
    }
    printf("deleting device id=%d lbl=%s \n",ID,target->label.c_str());
    target->setState(false);
    
    Device * new_content = new Device[itemsCount-1];
    int j=0;
    for(int i=0; i<itemsCount;i++){
      if(content[i].ID !=ID){
        new_content[j]= content[i];
       j++;
      }
    }
    //delete content;
    content = new_content;
    itemsCount --;
    printf("itemsCount=%d  \n",itemsCount);

    return true;

  }
  //return a pointer to the Device object or NULL otherwise
  Device  *getDeviceByID(int ID_){//this is added here an not cpied from UNO test source
    for(int i=0; i<itemsCount;i++){
      if(content[i].ID==ID_) return &content[i];
    }
    return NULL;
  }
  void UpdateAll(unsigned long long tm){
    for(int i=0; i<itemsCount;i++){
      content[i].update(tm);
    }
  }
  void forEach(fc callback){
    for(int i=0; i<itemsCount;i++){
      callback((void*) (&content[i]));
    }
  }
  //should not be used directely (use others methods instead )
  Device &operator [] (int ix){
    if(ix<itemsCount) return content[ix];
    else {
      printf("fatal: outOfBounds"); //exit(1);
    }
  }






};





void Device::update(unsigned long long tm) {
  if (Config.mode == none) return;
  if (Config.mode == manual) {
    setState(this->Config.manualState);
    return;
  }
  //mode auto
  //uses live updating approach
  uint64_t D = this->Config.autoOptions.repeatEvery*1000;
  uint64_t t = tm; //mainState. Clock. CurrentTime
  uint64_t o = this->Config.autoOptions.startsAt;
  uint64_t filled = this->Config.autoOptions.duration*1000;
  //Serial. println("update :") ;
  int pos = ((uint32_t)(t % ((uint64_t)D))) - (o % D);
  if (pos < 0) pos += D;
  //Serial. write("pos:") ; Serial. println(pos) ;
  //Serial. println() ;
  bool autoState = pos < filled;
  bool conditions_met = true; // true if all conditions are met
  for (int i = 0; i < Config.autoOptions.conditions_cc ;i++) {
    conditions_met = conditions_met && Config.autoOptions.conditions[i].Check(18, 27);
    if (!conditions_met) break;
  }
  if (conditions_met) {
    setState(autoState);
  }
  else {
    setState(false);
  }
}