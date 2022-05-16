//version 2 supports websocket (only) and refactored

#include "ServerUtilsEsp32.v2.h"
#include <Arduino.h>
#include "DateTimeMI.h"
#include "Clock.h"
#ifndef mi_device_h
#include "Device.h"
#endif
#include "DHTDriver.h"
#include "LightSensorDriver.h"




#define DHT_READING_INTV_MS 10000
#define LDR_READING_INTV_MS 400
#define LDR_PIN 33
#define DHT_PIN 34
#define LDR_SAMPLE_SZE 4

//dev only
#define WIFI_ALT_PIN 19 //pin 6 throws runtime error
#define SSID(i) ((i)==0? "yass2l": "Redmi Note 9 Pro")
#define PASS(i) ((i)==0? "yassin123@@2": "01234567")





//caries the state f the system.
struct State {
  public:
  State(){
  }
  int devicesCount ;
  DevicesCollection Devices ;
  Clock clock;
  int current_hum, current_temp;
} mainState;


ServerC* mainServer = new ServerC();
DHTDriver* dhtDriver = new DHTDriver(DHT_READING_INTV_MS,DHT_PIN,[](int hum, int temp){
    mainState.current_hum=hum;
    mainState.current_temp=temp;
    
});



LDRDriver* ldrDriver = new LDRDriver(LDR_READING_INTV_MS,LDR_SAMPLE_SZE,LDR_PIN,[](int value){
    String json = "ldr-update;{\"value\":";
    json = json + String(value)+"}";
    //Serial.printf("texting all %s\n",json.c_str());
  ws.textAll(json);
  String msg = "dht-update;{\"readings\":{\"hum\":";
    msg = msg + String(mainState.current_hum)+",\"temp\":"+
    String(mainState.current_temp)+"},\"error\":null}";
    //Serial.printf("texting all %s\n",json.c_str());
  ws.textAll(msg);
});



void ClientReqHandler(Request_t* req) {
    if (req->req_topic == DHTStateGet) {
        Serial.println("DHTStateGet ");
        
        char* body = (char*)"{\"readings\":{\"temp\":34,\"hum\":18}}";
        req->res_data = body;
    }
    else if (req->req_topic == DevicesListGet) {
        Serial.println("DevicesListGet ");
        Serial.println("# ctor dlr ");
        DevicesListR dlr = DevicesListR{ .collection = &mainState.Devices };
        Serial.println("# jsonCompact dlr ");
        req->res_data = dlr.jsonCompact();
        //printf("\n\n\n ok list\n%s\n\n\n",req->res_data);
    }
    else if (req->req_topic == DeviceConfigGet) {
        Serial.println("DeviceConfigGet");
        Device* d = mainState.Devices.getDeviceByID(req->targetDeviceID);
        if (d == NULL) {
            req->action_completion_code = 404;
            Serial.println("err 404 ");
        }
        else {
            req->action_completion_code = 200;
            DeviceConfigR dcr = DeviceConfigR{ .d_conf = (DeviceConfig*)(d->GetConfig()) };
            req->res_data = dcr.json();
            //printf("\n\n\n ok conf\n%s\n\n\n",req->res_data);
        }
    }
    else if (req->req_topic == DeviceConfigPut) {
        Serial.println("DeviceConfigPut ");
        //req->action_completion_code = 200;
        //return;
        printf("Request data:\n%s\n", req->req_data);
        DevicePutConfigQ dpcq = DevicePutConfigQ();
        bool succ = dpcq.parse(req->req_data);
        if (succ == false) {
            req->action_completion_code = 401;
            return;
        }
        else {
            Device* targetPtr = mainState.Devices.getDeviceByID(req->targetDeviceID); 
            if (targetPtr == NULL) {
                Serial.println("404");
                req->action_completion_code = 404; return; //todo this logic should apply on the set of requests that involve targed device
            }
            printf("s1:%s\n", uint64_to_str(dpcq.d_conf->autoOptions.startsAt));
            targetPtr->SetConfig(*(dpcq.d_conf));
            delete dpcq.d_conf;//cleanup
            req->action_completion_code = 200;

        }

    }
    else if (req->req_topic == DeviceStateGet) {
        Serial.println("DeviceStateGet ");
        //res.send({resState: target_device.currentState?"on":"off"})
        //todo this coppied from the node server which implements an older API version
        Device* d = mainState.Devices.getDeviceByID(req->targetDeviceID);
        if (d == NULL) {
            Serial.println("err 404 ");
            req->action_completion_code = 404;
        }
        else {
            req->action_completion_code = 200;
            DeviceStateR dsr = DeviceStateR{ .resState = d->currentState };
            req->res_data = dsr.json();
        }
    }
    else if (req->req_topic == DeviceStatePut) {
        Serial.println("DeviceStatePut ");
        DevicePutStateQ dsq;
        if (!dsq.parse_json(req->req_data)) {
            Serial.println("err 401 ");
            req->action_completion_code = 401;
        }
        else {
            Device* d = mainState.Devices.getDeviceByID(req->targetDeviceID);
            if (d == NULL) {
                Serial.println("err 404 ");
                req->action_completion_code = 404;
            }
            else {
                req->action_completion_code = 200;
                d->Config.manualState = dsq.reqState;
                d->setState(dsq.reqState);
                DeviceStateR dsr = DeviceStateR{ .resState = d->currentState };
                req->res_data = dsr.json();
            }
        }
    }
    else if (req->req_topic == TimeGet) {
        Serial.println("TimeGet ");
        Serial.println(mainState.clock.CurrentTime.timestamp);
    }
    else if (req->req_topic == TimePut) {
        Serial.println("TimePut ");
        TimePutQ tpq = TimePutQ();
        bool succ = tpq.parse_json(req->req_data);
        req->action_completion_code = succ ? 200 : 401;
        Serial.print("set clock to ts(ms): ");
        Serial.println(uint32_to_str(tpq.t));
        mainState.clock.Set(tpq.t);
        Serial.print(mainState.clock.CurrentTime.HoursM);  Serial.print(":");
        Serial.print(mainState.clock.CurrentTime.MinsM);  Serial.print(":");
        Serial.print(mainState.clock.CurrentTime.SecsM);  Serial.print(",");
        Serial.print(mainState.clock.millisCounter);  Serial.print(" ");
        Serial.print(mainState.clock.CurrentTime.DayM);  Serial.print("-");
        Serial.print(mainState.clock.CurrentTime.MonthM);  Serial.print("-");
        Serial.print(mainState.clock.CurrentTime.YearM); 
       
       
        
    }
    else if (req->req_topic == DevicePost) {
        Serial.println("DevicePost ");
        printf("data:\n'%s' \n", req->req_data);

        DevicePostQ dpq = DevicePostQ();
        bool succ = dpq.parse(req->req_data);
        if (succ) {

            bool added = mainState.Devices.Add(*dpq.device);
            dpq.device->Init();//only sets the pin direction and can be called anywhere
            req->action_completion_code = added ? 200 : 401;
            
            delete dpq.device;
        }
        else {
            Serial.println("failled ");
            req->action_completion_code = 401;
        
        }

    }
    else if (req->req_topic == DeviceDelete) {
        Serial.println("DeviceDelete ");
        //turning off down the device
        ///carried out by Remove()
        //removing it
        if (mainState.Devices.Remove(req->targetDeviceID)) {
            req->action_completion_code = 200;
        }
        else {
            req->action_completion_code = 401;
        }


    }

    //printf("done handeling");
}






//################### main setup ######################################################################
void setup() {

    Serial.begin(115200);
    //dev only
    Serial.println("hello");
    pinMode(WIFI_ALT_PIN, INPUT);
    pinMode(2, OUTPUT);
    Serial.println("initializing clock..");
    mainState.clock.Init(* new DateTimeMI());
    Serial.println("setting clock..");
    mainState.clock.Set(0, 0, 3, 24, 3, 2022);
    Serial.println("etting up server..");
    mainServer->setRequestHandler(ClientReqHandler);
    Serial.println("starting server..");
    int wifi_alt = digitalRead(WIFI_ALT_PIN) ? 1 : 0;
    mainServer->StartServer(NULL, NULL, SSID(wifi_alt), PASS(wifi_alt));
    digitalWrite(2, true); delay(200);digitalWrite(2, false);//hardware feedback for wfi connection 
    Serial.println("ready");


}

//########### main loop  ##############################################################################
void loop() {
  
  mainState.clock.Update();  //update clock
  mainState.Devices.UpdateAll(mainState.clock.getTimeStampMillis()); //run the devices logic
  mainServer->update();
  dhtDriver->update();
  ldrDriver->update();

}
