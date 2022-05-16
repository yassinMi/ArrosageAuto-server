
//in sync with the windows testing version (26-april-2022)
//types used by DataParser and the rest of the app

#ifndef mi_device_h
#include "Device.h"
#endif
#include <string.h>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;

/*struct DevicePostQ  
{
     
};*/

typedef  enum {DevicePost=0,DeviceConfigPut,DeviceStatePut,
TimePut,DevicesListGet,DeviceConfigGet,DeviceStateGet,TimeGet,
DHTStateGet,DeviceDelete } RequestTopic; 
typedef enum{DevicePostQType,DevicePutConfigQType,DevicePutStateQType,TimePutQType} RequestDataType; 
typedef enum{DevicesListRType,DeviceConfigRType,DeviceStateRType,TimeRType,DHTStateRType} ResponseDataType; 

//remember to delete
const char* uint32_to_str(uint32_t n) {
 char * res = new char[12];
 sprintf(res,"%d",n);
 return res;
}

//from app.ino
//IMPORTANT: remember to deallocate the memory 
//mi convert indigned long long number to a 19 sized string
//range: 0->999999999999999999ull anything beyond wont be garanteed to work 
//safe& tested, 
//to support left zeroes trailing you can reaily alter the code to implement that
const char* uint64_to_str(uint64_t l) {
  //the long number is split into two uint32 nmbers that are in range  0-999999999
  //and then are converted to Strings and contactenated
  uint32_t left_p = (uint32_t)((l / 1000000000ull));
  uint32_t right_p = (uint32_t)(l % 1000000000ull);
  char* full = new char[19];
  full[0]='\0'; //todo IMPORTNANT this line is important and is missing int the original implementaion at app.ino
  const char *str_l_p = uint32_to_str(left_p);
  const char *str_r_p = uint32_to_str(right_p);
  String comp_l = String(str_l_p);
  String comp_r = String(str_r_p);
  if (left_p > 0) { //logic whether add thraiing eros to the right section or not
    strcat(full, comp_l.c_str());
    for (int i = 0; i < 9 - comp_r.length();i++) {
      strcat(full, "0");
    }
  }
  strcat(full, comp_r.c_str());
  delete str_l_p;
  delete str_r_p;
  return full;
}

//tested on all range
//returns __UINT64_MAX__ on failre
//range: "0"->"999999999999999999" anything beyond wont be garanteed to work 
//tjb the clnl
const uint64_t uint64_from_str(char* str) {
  //the long number is split into two uint32 nmbers that are in range  0-999999999
  //same aprocach as uint64_to_str but reversed
  //and then are converted to Strings and contactenated
  int str_len = strlen(str);
  if(str_len>18){//assert 
      return __UINT64_MAX__;
  }
  if(str_len<=9){ //case of small uint32 compatible num
      uint32_t small_num=0;
      sscanf(str,"%d",&small_num);
      return (uint64_t) small_num;
  }
  int left_p_len = str_len - 9;

  char* left_p_str , *right_p_str;
  left_p_str = new char[left_p_len+1];
  left_p_str[0] = 0;
  for(int i=0; i<left_p_len;i++){
      left_p_str[i] = str[i];
  }
  left_p_str[left_p_len] = 0;
  //22100000000
  right_p_str = &str[left_p_len];


  uint32_t left_p , right_p ;
  sscanf(left_p_str,"%d",&left_p);
  sscanf(right_p_str,"%d",&right_p);
  /////                    ;
  return ((uint64_t)left_p*1000000000ull) + (uint64_t)right_p ;
}


class jsonBuilder{
    private:
    //doesnt ad qotes to value
    
    public:
    String str ="{";
    bool empty = true;
    void add(const char* key, char * value,bool add_quotes=false){
        if(empty==false){
            str = str + ",";
        }
        str =str + "\"" ;
        str = str+ key ;
        str = str+ "\"" ;
        str = str+ ":" ;
        if(add_quotes)
         str = str+ "\"" ;
        str = str+value ;
         if(add_quotes)
        str = str+ "\"" ;
        empty = false;
    }
    
    
    void add(const char* key, int value){
        char num[20] ;
        sprintf(num,"%d",value);
        add(key,num);
        
    }
    void add(const char* key, bool value){
         add(key,(char*) (value?"true":"false"));
    }
    void end(){
        str = str+ "}";
    }
};
const char * stringifyMode(ConfigMode mode){
    if(mode==manual) return "manual";
    else if(mode==automated) return "automated";
    else if(mode==none) return "none";

}

/////////////uint64*   cha*    bool*      jsonObj*  jsonList*
typedef enum{NumValue,StrValue,BoolValue,JsonObj,JsonLst} jsonValueType;



//only used in JsonList content
class JsonListElement{
    public:
    void * elementValue;
    jsonValueType elementType;  
    int elementIndex;
    char * stringify();
};

class JsonList{
    public:
    ~JsonList(){
        if(elements!=NULL){
            delete elements;
        }
    }
    JsonListElement * elements;
    int count; 
    char * stringify(){
        String *res = new String("[");
        for(int i =0; i<count; i++){
            (*res).concat(elements[i].stringify());
            if(i<count-1)
            (*res).concat(",");

        }
        (*res).concat("]"); //alert append replacments
        return (char*) res->c_str();
    }
};


class jsonKeyValue{
    public:
    ~jsonKeyValue(){
        if(key!=NULL){
            delete key;
        }
        if(value!=NULL){
            delete value;
        }
    }
    char * key;
    void * value;
    jsonValueType valueType;
    char * stringify() const;

};

class JsonObject{
    
    public:
    ~JsonObject(){
        delete keyValueContents;
    }
    jsonKeyValue * keyValueContents;
    int count;
    char * stringify() const;
    static char * stringifyJsonValue(void* value, jsonValueType valueType);
    bool tryGetValue(const char * key, void ** outp,jsonValueType type) const{
        for(int i = 0; i<count ; i++){
            if(strcmp(keyValueContents[i].key,key)==0){
                if(keyValueContents[i].valueType==type){
                    *outp =  keyValueContents[i].value;
                    return true;
                }
            }
        }
        return false;
    }
    bool tryGetString(const char * key, char ** outp) const {
        return tryGetValue(key,(void**) outp,StrValue);
    }
    bool tryGetObject(const char * key, JsonObject ** outp)const{
        return tryGetValue(key,(void**) outp,JsonObj);
    }
    bool tryGetList(const char * key, JsonList ** outp)const{
        return tryGetValue(key,(void**) outp,JsonLst);
    }
    bool tryGetBool(const char * key, bool** outp)const{
        return tryGetValue(key,(void**) outp,BoolValue);
    }
    bool tryGetUint64(const char * key, uint64_t** outp)const{
        return tryGetValue(key,(void**) outp,NumValue);
    }
};





 char*  JsonObject::stringifyJsonValue(void* value, jsonValueType valueType)  {
    char * res ;
    if (valueType == StrValue) {
        //printf("StrValue here\n");
        res = new char[3+ strlen((char*)value)];
        res[0]=0;
        strcat(res, "\"");
        strcat(res, (char*)value);
        strcat(res, "\"");
    }
    else if (valueType == NumValue) {
        res = new char[19]; res[0]=0;
        uint64_t the_num = *((uint64_t*)value);
        strcat(res, uint64_to_str(the_num));
    }
    else if (valueType == BoolValue) {
        res = new char[7]; res[0]=0;
        bool b_val = *((bool*)value);
        strcat(res, b_val ? "true" : "false");
    }
    else if (valueType == JsonObj) {
        char* stringified_obj = ((JsonObject*)value)->stringify();
        res = new char[strlen(stringified_obj)+1]; res[0]=0;
        strcat(res, stringified_obj);
    }
    else if (valueType == JsonLst) {
        char* stringified_lst = ((JsonList*)value)->stringify();
         res = new char[strlen(stringified_lst)+1]; res[0]=0;
        strcat(res, stringified_lst);
    }
    else {
        return (char*)"not yet";
    }



    return res;

}


 char* jsonKeyValue::stringify()  const{
     //printf("jsonKeyValue::stringify here\n");
         char* value_str = JsonObject::stringifyJsonValue(value, valueType);
     char* res = new char[6 + strlen(value_str) + strlen(key)];
     res[0] = '"';
     res[1] = '\0';
     strcat(res, key);
     strcat(res, "\":");
     strcat(res, value_str);
     return res;
 }



 char* JsonObject::stringify() const {
     // printf("JsonObject::stringify here\n");
     String* res = new String("{");
     for (int i = 0; i < count; i++) {
         (*res).concat(keyValueContents[i].stringify());
         if (i < count - 1)
             (*res).concat(",");

     }
     (*res).concat("}"); //alert append replacment
     return (char*)res->c_str();
 }


char * JsonListElement::stringify(){
    //printf("JsonListElement::stringify here\n");
        return JsonObject::stringifyJsonValue(elementValue,elementType );
}



class jsonReader{
    public:
    const char * content;
    int c; //the cursor

    //PART
    //requires cursor right at the  " 
    //returns the key size
    //make sure to check -1 return value that means bad format
    //remember to delete
    int readStrLiteral(char ** outpkey){
        int keySize =0;
        do
        {
            keySize++;
            if(*(content+c+1+keySize)=='\0'){
                return -1;
            }
        } while (*(content+c+1+keySize)!='"');
        
        char * key  = new char[keySize+1];
        for(int i=0; i<keySize;i++){
            key [i]= content[c+1+i];
        }
        key[keySize]='\0';
        *outpkey= &key[0];
        return keySize;

    }

    bool isBoolStart(const char* val){
        return (*val =='t')||(*val =='T')||(*val =='f')||(*val =='F');
    }
    bool isNumber(const char* val){
        return ((*val) >=0x30)&&((*val) <0x40);
    }
    //currentely support only uint32
    //reads int number returns it's length, returns -1 if the cursor is not at a digit otherwise this cant fail because it assumes that anything after the digits is accetable
    int readInt(uint64_t * outp ){
        if(isNumber(&content[c])==false) return -1;
        int numSize = 0;
        do
        {
            numSize++;
        } while (isNumber(&content[c+numSize]));
        
        char * numStr = new char [numSize+1];
        for(int i=0; i<numSize;i++){
            numStr [i]= content[c+i];
        }
        numStr[numSize]='\0';  
        //sscanf(numStr,"%d",outp);//todo scanf can do all the work in this function
        *outp = uint64_from_str(numStr);
        
        return numSize;
    }
    //reads and asserts that bool sueqence is valid and returns 4 or 5 (as the boolean value length) returns -1 if none
    int readBool(bool * outp ){
        if(isBoolStart(&content[c])==false) return -1;
        if(strncmp(&content[c],"false",5)==0){
            * outp = false;
            return 5;
        }
        else if(strncmp(&content[c],"true",4)==0){
            * outp = true;
            return 4;
        }
        else{
            return -1; //bad format
        }
    }

    //reads num, str, obj, list, return back data ana type, returns -1 on falure, -2 on bad format (stop the parsing at -2)
    //remember to delete valuePtr
    int readValue(void ** valuePtr, jsonValueType * valueType){
        if(content[c]=='"'){
            //printf("str type\n");
            //case of string value
            char * stringValueStr;
            int stringValueSze = readStrLiteral(&stringValueStr);
            if(stringValueSze==-1){
                printf("fatal 5\n");
                return -1; //ba format
            }
            //moving the cursor forward as promised
            c+=(stringValueSze+2); //2 for the quotes
            //were done now
            *valuePtr =  stringValueStr;
            *valueType = StrValue;
            return 0;
        }
        else if(isBoolStart(&content[c])){
            //printf("bool type\n");
            //case of boolValue
            bool *boolValue = new bool;
            int boolSize = readBool(boolValue);
            if(boolSize==-1) {printf("fatal 6\n"); return -1;}
            c+= boolSize;
            //done
            *valuePtr =  boolValue;
            *valueType = BoolValue;
            return 0;
        }
        else if(isNumber(&content[c])){
            //printf("num type\n");
            //case of numValue
            uint64_t *numValue = new uint64_t;
            int numSize = readInt(numValue);
            if(numSize==-1) {printf("fatal 7\n"); return -1;}
            c+= numSize;
            //done
            *valuePtr =  numValue;
            *valueType = NumValue;
            //printf("num type returned leaving c =%d at %S\n",c,&content[c]);
            return 0;
        }
        else if(content[c]=='['){
            //case of jsonList
            //printf("list type\n");
            *valuePtr =  readNextJsonList();
            if(*valuePtr==NULL){//fatal bad format check
                return -2;
            }
            *valueType = JsonLst;
            return 0;

        }
        else if(content[c]=='{'){
            //printf("obj type\n");
            //case of jsonObj
            *valuePtr =  readNextJsonObj();
            if(*valuePtr==NULL){//fatal bad format check
                return -2;
            }
            //printf("done readNextJsonObj\n");
            *valueType = JsonObj;
            return 0;
        }
        else if ((content[c]==']')||(content[c]=='}')){
            //printf("none type\n");
            return -1;
        }
        else {
            printf("value was expected c=%d at %s\n",c,&content[c]);
            return -2;
        }

    }

    //read th next key and value pair after the cursor and store them in a jsonKeyValue , or NULL if reached end 
    //also moves the curesor forward right one step after the last char of the value
    //returning -2 means fatal bad format error and the parsing should end
    //returning -1 means nohing it's returned when NULL is assigned to the jkv ptr
    //exmpl {"name":"dev1","ID":4,"Conditions":[{"type":"gt","active":false,"p1":15},],"state":true}  //object for test and not related to the app
    //remember to dealocate delete jsonKeyValue
    int readNext(jsonKeyValue ** jkv_ptr){
        //current cursor when calling this should be at a , or } or { otherwise bad format
        //printf("calling readNext with c at %d pointing at %s\n",c,&content[c]);
        if((content[c]!=',')&&(content[c]!='{')){//assert
            //fatal err
            //printf("skip 1, cursor=%d pointing at %s\n",c,(content+c));
            *jkv_ptr= NULL;
            return -1;
        } 
        c++;
        if(content[c]!='"'){
            printf("expected a valid key with qotes c=%d at %s \n",c,&content[c]);
            *jkv_ptr= NULL;
            return -2;
        }
        char * keyStr;
        int keySze = readStrLiteral(&keyStr);
        if(keySze==-1){ 
            printf("couldn't parse key c=%d at %s \n",c,&content[c]);
            *jkv_ptr= NULL;
            return -2;
        } //bad format
        //printf("key:%s\n",keyStr);
        //at this point we have parsed the key, now the value is left (notice that we allow empty string "" as valid key)
        c+=(keySze+2); //putint it at the :
        if(content[c]!=':'){ //assert
            printf("expected a : after key. c=%d at %s \n",c,&content[c]);
            *jkv_ptr= NULL;
            return -2;
        }
        c++;//now it's at whatever the value starts with, can be a $"$ , ${$ , $[$ , any number , $f$ or $t$ for boolean, otherwise bad format
        //printf("crsor at %c\n",content[c]);
        jsonKeyValue *jkv = new jsonKeyValue();
        jkv->key=keyStr;
        int succ = readValue(&jkv->value,&jkv->valueType);
        if(succ==-1){
            *jkv_ptr= NULL;
            return -1;
             //ba format
        }
        else if(succ==-2){
            printf("parsing ended\n");
            *jkv_ptr= NULL;
            return -2;
        }
        *jkv_ptr= jkv;
        return 0;  

    }
    //reuires that the cursor is ecacetly at an opening { of a json obj
    //recursve
    //puts the cursor right after the ending } that corresponds to the opening one
    //returns a ull representation of the object including sub ojects and lists, works recursively
    //returns NULL on fatal bad format, any subsequent parsing should be canceled
    //should never alter the data (content)
    //remember to dealocat (delete JsonObject is enough)
    JsonObject * readNextJsonObj(){
        if(content[c]!='{'){//assert
            //fatal err
            printf("error , cursor=%d pointing at %s while { expected \n",c,(content+c));
            return NULL;
        }
        
        JsonObject *j = new JsonObject();
        j->keyValueContents = new jsonKeyValue[20]; //todo hard coded length
        j->count=0;
        jsonKeyValue * ptr;
        int succ;
        succ = readNext(&ptr);
        while (succ==0)
        {
            j->keyValueContents[j->count++]= *ptr;
            //delete ptr;//fo
            succ =  readNext(&ptr);
        }
        if(succ==-2){
            return NULL;
        }
        if(content[c]!='}'){//assert
            printf("error , cursor=%d pointing at %s while } expected \n",c,(content+c));
            return NULL;
        }
        c++;

        return j;
    }

    //same return logic as readNextJson 
    //remember to delete JsonList
    JsonList * readNextJsonList(){
        if(content[c]!='['){//assert
            //fatal err
            printf("error , cursor=%d pointing at %s while | expected \n",c,(content+c));
            return NULL;
        }
        c++;
        JsonList * l = new JsonList();
        l->elements = new JsonListElement[20]; //todo hard coded length
        l->count=0;
        void * val_ptr;
        jsonValueType val_type ;

        int succ = readValue(&val_ptr,&val_type);
        //printf("done readValue, c=%d\n",c);
        while (succ==0)
        {
            if(content[c]==',') c++;//jump over the , spiltter
            l->elements[l->count].elementIndex=l->count;
            l->elements[l->count].elementType=val_type;
            l->elements[l->count].elementValue=val_ptr;
            l->count++;
            succ = readValue(&val_ptr,&val_type);
            //printf("done readValue, c=%d\n",c);
        }
        if(succ==-2){
            printf("parsing ended \n",c,(content+c));
            return NULL;
        }
        if(content[c]!=']'){//assert
            printf("error , cursor=%d pointing at %s while ] expected \n",c,(content+c));
            return NULL;
        }
        c++;
        return l;
    }

};









Condition * parseCondition(JsonObject *jo){
    //printf("parsing cond of json %s \n",jo->stringify());
    uint64_t *param1;
    char* type;
    char* targetVar;
    bool succ= jo->tryGetUint64("param1",&param1)&&
    jo->tryGetString("targetVar",&targetVar)&&
    jo->tryGetString("type",&type);
    if(!succ){
        printf("unsec\n");
        return NULL;
    }
    if(*param1>1000||*param1<0){
        printf("range\n");
        return NULL;//accpeted range,;
    }
    Condition *c = new Condition();
    c->param1 = (float) (int)(*param1);
    if(strcmp("temp",targetVar)==0){
        c->targetVariable = temp;
    }
    else if(strcmp("hum",targetVar)==0){
        c->targetVariable = hum;
    }
    else{
        return NULL;
    }
    if(strcmp("gt",type)==0){
        c->type = gt;
    }
    else if(strcmp("lt",type)==0){
        c->type = lt;
    }
    else{
        return NULL;
    }
    return c;
}


AutoOptions * parseAutoOptions(JsonObject *jo){
    uint64_t *startAt, *duration, *repeat;
    printf("parsing jo = %s\n",jo->stringify());
    bool succ= jo->tryGetUint64("startsAt",&startAt)&&
    jo->tryGetUint64("duration",&duration)&&
    jo->tryGetUint64("repeatEvery",&repeat);
    if(!succ){
        printf("unsec\n");
        return NULL;
    }
   
    AutoOptions *opts = new AutoOptions();
    opts->startsAt = *startAt;//todo specify millis vs secs
    opts->repeatEvery = (int) *repeat;
    opts->duration = (int) * duration;
    JsonList *conditions_json;
    if(jo->tryGetList("conditions",&conditions_json)){
        int requiredCC = conditions_json->count;
        Condition * conditions = new Condition[requiredCC ];
        //printf("allocated %d conditions array\n", requiredCC);
        int conditions_cc = 0;
        for(int i =0; i< requiredCC;i++){
            if(conditions_json->elements[i].elementType!=JsonObj) continue;
            Condition * ptr = parseCondition((JsonObject*)(conditions_json->elements[i].elementValue));
            if(ptr!=NULL){
                 conditions[conditions_cc++]=*ptr;
            }
            else{
                printf("condition is NULL\n");
            }
        }
        opts->conditions = conditions;
        opts->conditions_cc = conditions_cc;
    }
    
    else{
        printf("unsec\n");
        return NULL;
    }
    return opts;
    
}

//todo make use of this function at DevicePostQ
//some ommited keys will be filled with default values (manualState=false, mode=manual, ..)
//returns NULL on bad formats
//remember to delete DeviceConfig
DeviceConfig * parseDeviceConfig(const JsonObject *jo){
        char* lbl;
        printf("parsing '%s'", jo->stringify() );
        if (!jo->tryGetString("label", &lbl)) {
            printf("bad config json format label can't be ommited\n");
            return NULL;
        }
        JsonObject* opts_obj;
        AutoOptions * opts;
        bool has_opts = false;
        if(jo->tryGetObject("autoOptions",&opts_obj)){
            opts = parseAutoOptions(opts_obj);
            if(opts!=NULL){
                has_opts = true;
            }
        }
        if(has_opts==false){
            printf("ommited opts, using default..\n");
            opts = new AutoOptions();
        }
        char *c_mode_str;
        ConfigMode c_mode = manual;
        if(jo->tryGetString("mode",&c_mode_str)){
            if(strcmp(c_mode_str,"manual")==0){c_mode=manual;}
            else if(strcmp(c_mode_str,"automated")==0){c_mode=automated;}
            else if(strcmp(c_mode_str,"none")==0){c_mode=none;}
        }
        bool *m_state= new bool;
        *m_state = false;
        jo->tryGetBool("manualState",&m_state);
        DeviceConfig *confg = new DeviceConfig(c_mode,lbl,*opts,*m_state);
        //cleanup
        delete m_state;
        delete opts;
        return confg;
}





//remember to dealocate
const char * stringifyDeviceCompact(Device *device){
    // "{\"ID\":\"1\",\"label\":\"Pomp a eaux ms 1\",\"currentState\":false,\"mode\":\"automated\"},"
    jsonBuilder j;
    Serial.println("# s1 ");
    j.add("ID",device->ID);
    Serial.println("# s2 ");
    j.add("label",(char*)device->label.c_str(),true);
    Serial.println("# s3 ");
    j.add("currentState",(bool)device->currentState);
    Serial.println("# s4 ");
    const char *m = stringifyMode(device->GetConfig()->mode);
     Serial.print("# stringifyMode : ");
     Serial.println(m);
     Serial.println("# s5 ");
    j.add("mode",(char*)m,true);
     Serial.println("# s6 ");
     //delete m;
     Serial.println("# s7 ");
    j.end();
     Serial.println("# s8 ");
    char * res = new char[strlen(j.str.c_str())+1];
     Serial.println("# s9 ");
    strcpy(res,j.str.c_str());
     Serial.println("# s10 ");
    return res;
}
//remember to dealocate
const char * stringifyCondition(Condition cond){
    // "{\"type\": \"gt\",\"param1\":20,\"param2\":20,\"targetVar\":\"temp\"}"
    jsonBuilder j; //todo use the jsonReader in these serialization functions or copy it's code into jsonBuilder
    j.add("type",(char*) (cond.type==gt?"gt":"lt"),true);
    j.add("param1",(int)cond.param1);
    //j.add("param2",(int)cond.param2);
    j.add("targetVar",(char*)(cond.targetVariable==temp?"temp":"hum"),true);
    j.end();
    char * res = new char[strlen(j.str.c_str())+1];
    strcpy(res,j.str.c_str());
    return res;
    
}
//remember to dealocate
const char * stringifyAutoOptions(AutoOptions opts){
    jsonBuilder j;
    const char * st_at =  uint64_to_str(opts.startsAt);
    j.add("startsAt",(char*)st_at,false);
    delete st_at;
    j.add("repeatEvery",opts.repeatEvery);
    j.add("duration",opts.duration);
    String conditionsListStr = "[";
    bool nonempty = false;
    for(int i=0; i<opts.conditions_cc; i++){
        if(nonempty)conditionsListStr.concat(",");
        const char* c =  stringifyCondition(opts.conditions[i]);
        conditionsListStr .concat (c);
        delete c;
        nonempty=  true;
    }
    conditionsListStr.concat("]");
    j.add("conditions",(char *)conditionsListStr.c_str(),false);
    j.end();
    char * res = new char[strlen(j.str.c_str())+1];
    strcpy(res,j.str.c_str());
    return res;
    //string("{\"startsAt\":1654,\"repeatEvery\":3600,\"duration\":60,\"conditions\": [{")
}


struct DevicesListR {
    DevicesCollection* collection;
    //remember to delete
    char* jsonCompact() {
        String devcesListStr = "[";
        Serial.println("# 1 ");
        bool    nonempty = false;
        Serial.println("# 2 ");
        for (int i = 0; i < collection->Count(); i++) {
            if (nonempty) devcesListStr.concat( ",");
            Serial.println("# 2-1 ");
            const char * dc = stringifyDeviceCompact(&(*collection)[i]);
            Serial.println("# 2-2 ");
            devcesListStr .concat(dc);
            Serial.println("# 2-3 ");
            //delete dc;
            nonempty = true;
        }
        Serial.println("# 3 ");
        devcesListStr .concat("]");
        jsonBuilder j;
        Serial.println("# 3-1 ");
        j.add("devices", (char*)devcesListStr.c_str(), false);
        Serial.println("# 3-2 ");
        j.end();
        Serial.println("# 3-3 ");
        Serial.println("# 4 ");
        char* res = new char[strlen(j.str.c_str()) + 1];
        Serial.println("# 5 ");
        strcpy(res, j.str.c_str());
        Serial.println("# 6 ");
        return res;
    }

};

struct DevicePostQ
{
    Device* device;

    //returns true if a minimum of required properties were parsed successfully
    bool parse(const char* deviceJsonStr) {
        jsonReader jr =  jsonReader();
        jr.content = deviceJsonStr;
        jr.c = 0;
         printf("parsing json '%s'\n",jr.content);
        
        JsonObject* j = jr.readNextJsonObj();
        if (j == NULL) {
            printf("bad device json format1 '%s'\n",jr.content);
            return false;
        }
        char* id_str;
        JsonObject* config_obj;
        bool succ = j->tryGetString("ID", &id_str);
        succ &= j->tryGetObject("Config", &config_obj);
        if (!succ) {
            printf("bad device json format2\n");
            return false;
        }
        char* lbl;
        if (!config_obj->tryGetString("label", &lbl)) {
            printf("bad device json format3\n");//todo too much assertions
            return false;
        }
        int id;
        sscanf(id_str, "%d", &id);
        if(id>100||id<0){
            printf("bad device json format, id out of accepted range\n");//todo too much assertions
            return false;
        }
        JsonObject* opts_obj;
        AutoOptions * opts;
        bool has_opts = false;
        if(config_obj->tryGetObject("autoOptions",&opts_obj)){
            opts = parseAutoOptions(opts_obj);
            if(opts!=NULL){
                has_opts = true;
            }
        }
        if(has_opts==false){
            opts = new AutoOptions();
        }
        char *c_mode_str;
        ConfigMode c_mode = manual;
        if(config_obj->tryGetString("mode",&c_mode_str)){
            if(strcmp(c_mode_str,"manual")==0){c_mode=manual;}
            else if(strcmp(c_mode_str,"automated")==0){c_mode=automated;}
        }
        bool *m_state= new bool;
        *m_state = false;
        config_obj->tryGetBool("manualState",&m_state);
        

        DeviceConfig confg = DeviceConfig(c_mode,lbl,*opts,*m_state);
        delete opts;
        

        Device* new_device = new Device(id, String(lbl),confg);
        device= new_device;
        return true;
    }
};

struct DeviceConfigR{
    DeviceConfig *d_conf;
    //remember to delete
    char * json(){
        jsonBuilder j;
        j.add("label",(char *)d_conf->label.c_str(),true);
        j.add("mode",(char *) stringifyMode(d_conf->mode),true);
        j.add("manualState",d_conf->manualState);
        j.add("autoOptions",(char *)stringifyAutoOptions(d_conf->autoOptions),false);
        j.end();
        char * res = new char[strlen(j.str.c_str())+1];
        strcpy(res,j.str.c_str());
        return res;   
    }
};

struct DevicePutConfigQ{
    DeviceConfig *d_conf;
    //remember to dealocate the d_conf
    
    bool parse(const char * jsonData){

    Document d;
    Serial.println("parsing");
    d.Parse(jsonData);
    //# label
    Value& l = d["label"];
    String label= l.GetString();
    Serial.println("got lbl");
    Value& m = d["mode"];
    //# mode
    const char * mode_s =  m.GetString();
    Serial.println("got mode");
    ConfigMode mode;
    if(strcmp(mode_s,"manual")==0)mode = manual;
    else if(strcmp(mode_s,"automated")==0) mode = automated;
    else if(strcmp(mode_s,"none")==0) mode = none;
    else{ Serial.println("mode invalid"); return false; }
    Serial.println("got mode2");
    //# manualState
    Value& ms = d["manualState"];
    bool manualState = ms.GetBool();
    Serial.println("got mstate");
    //# autoOptions
    Value& aa = d["autoOptions"];
    Serial.println("got aa");
    uint64_t aa_sa = aa["startsAt"].GetUint64();
    Serial.println("got startsAt");
    int aa_d = aa["duration"].GetInt();
    Serial.println("got dur");
    int aa_re = aa["repeatEvery"].GetInt();
    Serial.println("got re");
    Value& aa_c =aa["conditions"];
    Serial.println("got cond");
    if(aa_c.IsArray()==false) {Serial.println("collecton expected array"); return false;}
    int c_cc=  aa_c.Size();
    Condition * conditions = new Condition[c_cc];
    for (SizeType i = 0; i <c_cc; i++){ // Uses SizeType instead of size_t
        Serial.println("cond cycle");
        Condition new_condition = Condition();
        float param1 = (float) aa_c[i]["param1"].GetInt();
        Serial.println("got param1");
        ClimateVariable targetVariable = (strcmp(aa_c[i]["targetVar"].GetString(),"hum")==0)?hum:temp;
        Serial.println("got targetVariable");
        ConditionType tp = (strcmp(aa_c[i]["type"].GetString(),"lt")==0)?lt:gt;
        Serial.println("got type");
        conditions[i]= Condition(targetVariable,tp,param1);
    }
     Serial.println("don conds loop");

    AutoOptions ao =  AutoOptions(aa_sa,aa_d,aa_re,conditions,c_cc);
     Serial.println("ctor ao ");
    d_conf = new DeviceConfig(mode,label,ao,manualState);
     Serial.println("ctor dc ");


   return true;
    }
};

const char* onOff(bool state){
    if(state) return "on";
    else return "off";
}

struct DeviceStateR{
    bool resState;
    //remember to delete
    char * json(){
        jsonBuilder  j = jsonBuilder();
        j.add("resState",(char *)onOff(this->resState),true);
        j.end();
        char * res = new char [strlen(j.str.c_str())+1];
        strcpy(res,j.str.c_str());

        return res;
    }
};

struct DevicePutStateQ{
    bool reqState;
    //returns false on failure, populates felds otherwise (that applies on all these api types)
     //hacky parsing
    bool parse_json(char * json_str){
        if(strstr(json_str,"reqState")!=NULL){
            if(strstr(json_str,"on")!=NULL){
                reqState=true;
                return true;
            }
            else if(strstr(json_str,"off")!=NULL){ reqState = false; return true;}
            else{ return false;

            }
        }
        else{
        return false;
        }
    }
};

struct TimePutQ{
    unsigned long long t;//ms
    //fast hacky parsing
    bool parse_json(char * json_str){
         //json schema (API 24-march-2022): '{"t":1651254394749,"m":749}' 
          char* t_key_occurrence = strstr(json_str,"\"t\":");

         if(t_key_occurrence==NULL){
             Serial.print("cant find \"t\" in ");
             Serial.println(json_str);
             return false;
         }
         char* t_ms_str = new char[30];//placeholder
         strcpy(t_ms_str,t_key_occurrence+4);
         String s = String(t_ms_str);
         int end = s.indexOf(",");
         s = s.substring(0,end);
         Serial.print("parsed time in mils as str: ");
         Serial.println(s);

         uint64_t t_ms = uint64_from_str((char *)s.c_str());
         t = t_ms;
         return true;
    }
};

struct TimeR{
    unsigned long long t;
};

struct DHTStateR{
    int temp;
    int hum;
    char * error;
};

