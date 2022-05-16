
//added on (30-apr-2022), uses websoket and doesnt support the initial REST api 
#ifndef ServerUtilsEsp32_v2_h
#define ServerUtilsEsp32_v2_h
#include "APITypesRapid.h"
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#endif

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

//application layer abstraction
struct Request_t {
private:

public:
  const char* port;
  RequestTopic req_topic; //independent of the protocol, can be used with this websocket version
  int targetDeviceID;
  char* req_data;
  int action_completion_code; //on http this maps to status codes, on websoket this will depend on req_type to determn a message string
  char* res_data;

};

//typedef std::function<void(Request_t*)> RequestHandler_t ;
typedef void (*RequestHandler_t)(Request_t*) ;



//abstraction
//this version only supports websocket
//members that are not implemented are commented out temporarily
class ServerC {
public:
  
  ServerC (const char * host, const char * port, const char * ssid, const char * pass);
  ServerC (void);
  int StartServer();
  int StartServer(const char * host, const char * port, const char * ssid, const char * pass);
  void setRequestHandler(RequestHandler_t handler);
  void update();
  private:
  void handleMsg(void * arg, uint8_t * data, size_t len);
  void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient * client , AwsEventType type, void *arg, uint8_t * data, size_t len);
  void callHandler(const char* reqest_data, RequestTopic req_type, int target_deviceID_int,int msg_id );
  RequestHandler_t mainRequestHandler;
  const char* host; //host and port are not used and are kept for compatibility with older application code
  const char* port;
  const char* ssid;
  const char* pass;
};






















ServerC::ServerC() {
    ssid = NULL;
    pass = NULL;

}

ServerC::ServerC(const char* host_, const char* port_, const char* ssid_, const char* pass_) {
    host = host_;
    port = port_;
    ssid = ssid_;
    pass = pass_;
}

void ServerC::setRequestHandler(RequestHandler_t reqHndler) {
    mainRequestHandler = reqHndler;
}

//to be called on loop
void ServerC::update() {
    ws.cleanupClients();
};


//sets host and port and starts
int ServerC::StartServer(const char* host, const char* port, const char* ssid, const char* pass) {
    this->host = host;
    this->port = port;
    this->ssid = ssid;
    this->pass = pass;
    return this->StartServer();
}


static char *RequestTopic_lst[] = {"DevicePost","DeviceConfigPut","DeviceStatePut","TimePut","DevicesListGet",
    "DeviceConfigGet","DeviceStateGet","TimeGet","DHTStateGet","DeviceDelete"};
RequestTopic str2RequestTopic(const char * str){
    
    int i=0;
    while(strcmp(RequestTopic_lst[i],str)!=0) {i++;};
    if(i>=10) throw "bad enum string";
    return (RequestTopic) i;
}
const char* RequestTopic2str(RequestTopic topic){
    return RequestTopic_lst[(int)topic] ;
}



void ServerC::callHandler(const char* reqest_data, RequestTopic req_type, int target_deviceID_int, int msg_id) {

    Request_t reqObj = Request_t();
    Serial.println("preparing reqObj..");
    reqObj.req_topic = req_type;
    reqObj.targetDeviceID = target_deviceID_int;
    reqObj.req_data = (char*)reqest_data;
    reqObj.action_completion_code = 200;
    reqObj.res_data = "";
    Serial.println("calling mainRequestHandler hndler..");
    mainRequestHandler(&reqObj);
    printf("handeler returned: \n%s\n",reqObj.res_data);
    char  statusCodeCStr[4] = { 0 };
    sprintf(statusCodeCStr, "%d", reqObj.action_completion_code);
    printf("Response code: %s\n", statusCodeCStr);
   

    String body = String(reqObj.res_data);
    printf("Response:\n'%s'\n", body.c_str());
    String message = String (RequestTopic2str(req_type))+
    "-resp" ";"+String(msg_id)+
    ";"+String(reqObj.action_completion_code)+";"+body;
     printf("Response message: '%s'\n", message.c_str());
    ws.textAll(message );

    Serial.println("done");
}




//'DeviceStatePut;245;{"reqState":"on"};d_id=26&d_c=2'
//'dht;246;;'
//'deleteDevice;2467;;d_id=2'
void parseWsMsg(char*data,int *msg_id, RequestTopic *topic, char ** payload,int *device_id){
char *d = new char[strlen(data)+1];
strcpy(d,data);
char *topic_cstr = d;
char* first_sep = strchr(d,';');
char *id_cstr = first_sep+1;
*(first_sep) = '\0';
char *sec_sep = strchr(id_cstr,';');
char *payload_cstr = sec_sep+1;
*(sec_sep) = '\0';
char *third_sep = strchr(payload_cstr,';');
char *params_cstr = third_sep+1;
*(third_sep) = '\0';
*msg_id = String(id_cstr).toInt() ;
if(String(params_cstr).startsWith("d_id=")){
    *device_id = String(params_cstr+5).toInt(); //assumes that the only param is d_id e.g d_id=26 will result in id being 26 int
}
*payload = payload_cstr;
//*params = params_cstr; //not required now as the only used param is device_id
*topic = str2RequestTopic(topic_cstr);
}

void ServerC::handleMsg(void * arg, uint8_t * data, size_t len){
  Serial.println("handeling the data event here");
  Serial.printf("len is %d\n",len);

  AwsFrameInfo *f_inf = (AwsFrameInfo*) arg;
  if(f_inf->final && f_inf->index==0 && f_inf->len==len && f_inf->opcode==AwsFrameType::WS_TEXT ){
    Serial.println("itd a TEXT in one frame");
    ((char*)data)[len]='\0';
    Serial.printf("the text is '%s' \n",(char*)data);

    int msg_id,device_id;
    char * payload_cstr;
    RequestTopic topic;
    parseWsMsg((char*)data,&msg_id,&topic,&payload_cstr,&device_id);
    callHandler(payload_cstr,topic,device_id,msg_id);
    
    
  }
  else{
      Serial.println("itd not a TEXT in one frame");
  }


}


void ServerC::onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient * 
client , AwsEventType type, void *arg, uint8_t * data, size_t len) {
  Serial.println("got an event");
  Serial.println("from : "+ client->remoteIP().toString());
  if(type==WS_EVT_CONNECT){
      Serial.println("the event is connected");
  }
  else if(type==WS_EVT_DISCONNECT){
      Serial.println("the event is dissconnected");
  }
   else if(type==AwsEventType::WS_EVT_DATA){
      Serial.println("the event is Data");
      handleMsg(arg,data,len);
  }
}

int ServerC::StartServer() {
    assert(ssid != NULL && pass != NULL);
    assert(mainRequestHandler != NULL );
    WiFi.mode(WIFI_MODE_STA);
    WiFi.begin(ssid, pass);
    Serial.println("");
    Serial.print("connecting to ");
    Serial.println(ssid);
    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());


    
   /* server.on(F("/devices"), [this]() {
        callHandler(server.arg("plain").c_str(), DevicesListGet, -1);
        });

    server.on(UriBraces("/devices/{}"), HTTP_POST, [this]() {
        String device = server.pathArg(0); int deviceIdInt;
        sscanf(device.c_str(), "%d", &deviceIdInt);
        callHandler(server.arg("plain").c_str(), DevicePost, deviceIdInt);
        });

    server.on(UriBraces("/devices/{}"), HTTP_DELETE, [this]() {
        String device = server.pathArg(0); int deviceIdInt;
        sscanf(device.c_str(), "%d", &deviceIdInt);
        callHandler("", DeviceDelete, deviceIdInt);
        });

    server.on(UriBraces("/devices/{}/config"), HTTP_GET, [this]() {
        String device = server.pathArg(0); int deviceIdInt;
        sscanf(device.c_str(), "%d", &deviceIdInt);
        callHandler("", DeviceConfigGet, deviceIdInt);
        });

    server.on(UriBraces("/devices/{}/config"), HTTP_PUT, [this]() {
        String device = server.pathArg(0); int deviceIdInt;
        sscanf(device.c_str(), "%d", &deviceIdInt);
        callHandler(server.arg("plain").c_str(), DeviceConfigPut, deviceIdInt);
        });

    server.on(UriBraces("/devices/{}/state"), HTTP_GET, [this]() {
        String device = server.pathArg(0); int deviceIdInt;
        sscanf(device.c_str(), "%d", &deviceIdInt);
        callHandler("", DeviceStateGet, deviceIdInt);
        });

    server.on(UriBraces("/devices/{}/state"), HTTP_PUT, [this]() {
        Serial.println("state put");
        String device = server.pathArg(0); int deviceIdInt;
        sscanf(device.c_str(), "%d", &deviceIdInt);
        callHandler(server.arg("plain").c_str(), DeviceStatePut, deviceIdInt);
        });
    //dup put
    server.on(UriBraces("/devices/{}/state"), HTTP_POST, [this]() {
        Serial.println("state put");
        String device = server.pathArg(0); int deviceIdInt;
        sscanf(device.c_str(), "%d", &deviceIdInt);
        callHandler(server.arg("plain").c_str(), DeviceStatePut, deviceIdInt);
        });

    server.on("/dht", HTTP_GET, [this]() {
        callHandler("", DHTStateGet, -1);
        });

    server.on("/time", HTTP_GET, [this]() {
        callHandler("", TimeGet, -1);
        });

    server.on("/time", HTTP_PUT, [this]() {
        callHandler(server.arg("plain").c_str(), TimePut, -1);
        });





    server.on(F("/t26"), []() {
        server.send(200);
        digitalWrite(26, !digitalRead(26));
        //mainRequestHandler(String("t"));
        });
    */



    ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient * 
client , AwsEventType type, void *arg, uint8_t * data, size_t len){
        this->onWsEvent(server, client ,  type,  arg, data,  len);
    });
    server.addHandler(&ws);
    server.on("/yass",HTTP_GET,[](AsyncWebServerRequest *rq){
      rq->send(200,"text/plain","hi, this is HTTP");
    });
    server.begin();
    Serial.println("websocket server started");
};



