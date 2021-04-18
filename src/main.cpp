#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_SSD1306.h>
#include <MORAD_IO.h>
#include <MORAD_utility.h>
#include <DAC.h>
#include <WebSocketsClient.h>
#include <SocketIOclient.h>
#include <ArduinoJson.h>

//******Specific to Motivation Radio module LCD Display********************************
#define OLED_RESET -1 // sharing with reset pin,  to keep the compiler happy
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
//Declare instance of display object, needs to include width, height, and I2C Wire before reset
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//****************Globals*************************************************
const char *ssid = "MonAndToneGlow";
const char *password = "UncagedNY2013";
const String myUsername = "SynthClient";
//Set CH server host and port - local server example
/*
const char host[] = "192.168.1.7";
const int port = 3000;
*/

//CH online server example
const char host[] = "ch-testing.herokuapp.com";
const int port = 80;

//The CH namespace for testing connections
String nSpace = "/hub";

//***********Instances****************************************************
//Init instance of SocketIOclient
SocketIOclient socketIO;

//***********Functions Defs***********************************************
//ESP32-specific function for parsing and printing data.
void hexdump(const void *mem, uint32_t len, uint8_t cols = 16)
{
  const uint8_t *src = (const uint8_t *)mem;
  Serial.printf("\n[HEXDUMP] Address: 0x%08X len: 0x%X (%d)", (ptrdiff_t)src, len, len);
  for (uint32_t i = 0; i < len; i++)
  {
    if (i % cols == 0)
    {
      Serial.printf("\n[0x%08X] 0x%08X: ", (ptrdiff_t)src, i);
    }
    Serial.printf("%02X ", *src);
    src++;
  }
  Serial.printf("\n");
}

//*********************MR Hardware I/O Functions*****************************

void gateoutA(int value)
{
  //you want to constrain the value first (between 0 and 1)
  int triggerVal = constrain(value, 0, 1);
  //then convert 0 to 1 to a float
  float val = float(triggerVal); // values 0 to 1
  //Serial.println("Here is what the gate out will get:");
  //Serial.println(val);
  GATEout(0, (bool)val);
}

void gateoutB(int value)
{
  //you want to constrain the value first (between 0 and 1)
  int triggerVal = constrain(value, 0, 1);
  //then convert 0 to 1 to a float
  float val = float(triggerVal); // values 0 to 1
  //Serial.println("Here is what the gate out will get:");
  //Serial.println(val);
  GATEout(1, (bool)val);
}

void gateoutC(int value)
{
  //you want to constrain the value first (between 0 and 1)
  int triggerVal = constrain(value, 0, 1);
  //then convert 0 to 1 to a float
  float val = float(triggerVal); // values 0 to 1
  //Serial.println("Here is what the gate out will get:");
  //Serial.println(val);
  GATEout(2, (bool)val);
}

void gateoutD(int value)
{
  //you want to constrain the value first (between 0 and 1)
  int triggerVal = constrain(value, 0, 1);
  //then convert 0 to 1 to a float
  float val = float(triggerVal); // values 0 to 1
  //Serial.println("Here is what the gate out will get:");
  //Serial.println(val);
  GATEout(3, (bool)val);
}


void CVoutA(int value)
{
  int constrainedVal = constrain(value, 0, 127);
      //you want to constrain the value first (between 0 and 1)
  float cvValFloat = float(map(constrainedVal, 0, 127, 0, 100)/ 100.0);
  //then convert 0 to 1 to a float
  //Serial.println("Here is what the cv out will get:");
  //Serial.println(cvValFloat);
  CVout(0, (unsigned)(cvValFloat * (DAC_RANGE - 1)));
}

void CVoutB(int value)
{
  int constrainedVal = constrain(value, 0, 127);
  //you want to constrain the value first (between 0 and 1)
  float cvValFloat = float(map(constrainedVal, 0, 127, 0, 100) / 100.0);
  //then convert 0 to 1 to a float
  //Serial.println("Here is what the cv out will get:");
  //Serial.println(cvValFloat);
  CVout(1, (unsigned)(cvValFloat * (DAC_RANGE - 1)));
}

void CVoutC(int value)
{
  int constrainedVal = constrain(value, 0, 127);
  //you want to constrain the value first (between 0 and 1)
  float cvValFloat = float(map(constrainedVal, 0, 127, 0, 100) / 100.0);
  //then convert 0 to 1 to a float
  //Serial.println("Here is what the cv out will get:");
  //Serial.println(cvValFloat);
  CVout(2, (unsigned)(cvValFloat * (DAC_RANGE - 1)));
}

void CVoutD(int value)
{
  int constrainedVal = constrain(value, 0, 127);
  //you want to constrain the value first (between 0 and 1)
  float cvValFloat = float(map(constrainedVal, 0, 127, 0, 100) / 100.0);
  //then convert 0 to 1 to a float
  //Serial.println("Here is what the cv out will get:");
  //Serial.println(cvValFloat);
  CVout(3, (unsigned)(cvValFloat * (DAC_RANGE - 1)));
}

//***********Collab-Hub Send/Receive Functions*************************

void sendPushEvent(String header, String from, String target)
{
  DynamicJsonDocument outPayloadDoc(1024);
  String emitType = "event";
  JsonArray payloadArray = outPayloadDoc.to<JsonArray>();
  payloadArray.add(emitType);
  JsonObject param_1 = outPayloadDoc.createNestedObject();
  param_1["header"] = header;
  param_1["from"] = from;
  param_1["mode"] = "push";
  param_1["target"] = target;
  String payload;
  serializeJson(outPayloadDoc, payload);
  //MUST add namespace tag and space to the front of any payload in order to emit
  String nSpacePayloadEmit = "/hub, " + payload;

  //The outgoing message format: /namespacetag,  ["event_name",{JSON}]
  socketIO.sendEVENT(nSpacePayloadEmit);
  Serial.println(nSpacePayloadEmit);
}

void sendPushControl(String header, String values, String target)
{
  DynamicJsonDocument outPayloadDoc(1024);
  String emitType = "control";
  JsonArray payloadArray = outPayloadDoc.to<JsonArray>();
  payloadArray.add(emitType);
  JsonObject param_1 = outPayloadDoc.createNestedObject();
  param_1["header"] = header;
  param_1["values"] = values;
  param_1["mode"] = "push";
  param_1["target"] = target;
  String payload;
  serializeJson(outPayloadDoc, payload);
  //MUST add namespace tag and space to the front of any payload in order to emit
  String nSpacePayloadEmit = "/hub, " + payload;

  //The outgoing message format: /namespacetag,  ["event_name",{JSON}]
  socketIO.sendEVENT(nSpacePayloadEmit);
  Serial.println(nSpacePayloadEmit);
}

void sendUsername(String name)
{
  DynamicJsonDocument outPayloadDoc(1024);
  //Structure our outgoing message as an array: first index is event name
  //second is the object payload as JSON

  String emitType = "addUsername";

  JsonArray payloadArray = outPayloadDoc.to<JsonArray>();

  //Add event name to the beginning of the outgoing message
  payloadArray.add(emitType);

  //Then add the object key and member to the array
  JsonObject param_1 = outPayloadDoc.createNestedObject();

  param_1["username"] = name;

  String payload;

  serializeJson(outPayloadDoc, payload);
  //MUST add namespace tag and space to the front of any payload in order to emit
  String nSpacePayloadEmit = "/hub, " + payload;
  delay(500);
  //The outgoing message format: /namespacetag,  ["event_name",{JSON}]
  socketIO.sendEVENT(nSpacePayloadEmit);
  //Serial.println(nSpacePayloadEmit);
}

//Processing function for all Socket-related and Collab-Hub events
void socketIOEvent(socketIOmessageType_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case sIOtype_DISCONNECT:
    Serial.printf("[IOc] Disconnected!\n");
    break;
  case sIOtype_CONNECT:
    Serial.printf("[IOc] Connected to url: %s\n", payload);

    // joins the specified namespace on connection
    socketIO.send(sIOtype_CONNECT, nSpace);
    //send username to server
    //sendUsername(myUsername);
    break;
  case sIOtype_EVENT:
  { //this extra set of brackets is used to contain all locally-defined variables inside of this case

    //parsing incoming events, data payloads requires the JSON library
    //make pointer to string as a char first
    char *stringptr = NULL;

    int id = strtol((char *)payload, &stringptr, 10);

    //Since we receive data from a namespace, the first character of the payload array is "/". We need to check that and move the pointer to the first "[" instead

    /* TO FIX: This solution is hardcoded. Need to make a char array searcher that moves pointer X number of indexes to dynamically adjust for any namespace size
    */
    if (payload[0] == 47)
    {
      // /hub, == 5 indexes
      payload += 5;
    }

    //checks to see if there is an ID for an acknowledgment, and if so, it "removes" that ID by moving the pointer to the first character of the incoming JSON message (payload)
    if (id)
    {
      payload = (uint8_t *)stringptr;
    }

    //****************Lets grab the incoming  event name here****************
    //Build memory location for incoming message JSON doc
    DynamicJsonDocument incomingDoc(1024);

    //Deserialize the incoming message
    DeserializationError error = deserializeJson(incomingDoc, payload, length);

    //error handling for the JSON deserialization process
    if (error)
    {
      Serial.print(F("Deserialization of Event JSON failed: "));
      Serial.println(error.c_str());
      return;
    }

    //Pull event name to from JSON document that will just hold incoming event name. We don't know the name of the event/member yet, so we use an array index

    String eventName = incomingDoc[0];
    //Log the event name
    Serial.printf("[IOc] event name: %s\n", eventName.c_str());

    //****************Lets grab the incoming data object here****************
    //Build memory location for incoming data JSON doc
    DynamicJsonDocument eventJSON(1024);
    String eventPayload = incomingDoc[1];
    //Deserialize the second element of the incoming message doc, grabbing the event's data object
    deserializeJson(eventJSON, eventPayload);
    /* For Debugging
    Serial.println("here is the pretty JSON:");
    serializeJsonPretty(incomingDoc[1], Serial);
    Serial.println();
    Serial.println("Here it is a string:");
    Serial.println(eventPayload);
    */

    //***********************Reacting to events and payload data************

    if (eventName == "connection")
    {
      //send username to server
      
      display.println("      Connected!");
      display.println();
      display.display();
      sendUsername(myUsername);
    }
    else if (eventName == "myUsername")
    {
      //extract members from the payload object based on their key
      const char *myUsername = eventJSON["username"];

      //Serial.println("Here is my username: ");
      //Serial.print(myUsername);
      Serial.println("This is my username: ");
      Serial.println(myUsername);

      
      char buf[16];
      char *leader = " ";
      strcpy(buf, leader);
      strcpy(buf + strlen(leader), myUsername);
      display.setTextColor(WHITE, BLACK);
      display.setCursor(0,24);
      display.print((String)buf);
      display.display();
    }
    else if (eventName == "otherUsers")
    {
      //allUsers returns an array. Handle it like this:
      Serial.println("Here are the number of the other connected users: ");
      Serial.println(eventJSON["users"].size());
      Serial.println("Here are the usernames of the other connected users: ");
      for (int i = 0; i < eventJSON["users"].size(); i++)
      {
        Serial.println(eventJSON["users"][i].as<char *>());
      }
    }
    else if (eventName == "event")
    {
      //event from server arrives in this format - {header: <string>, from: <string>}
      const char *eventHeader = eventJSON["header"];
      const char *eventSender = eventJSON["from"];

      Serial.println("This is the latest event header: ");
      Serial.println(eventHeader);
      Serial.println("This is who sent the event: ");
      Serial.println(eventSender);
      Serial.println("Here is the pretty JSON:");
      serializeJsonPretty(eventJSON, Serial);
      Serial.println();

      //sendPushEvent("espEvent", myUsername, "all");
      delay(500);
      //sendPushControl("espControl", "49", "all");
    }
    else if (eventName == "control")
    {
      //control from server arrives in this format - {header: <string>, values: <number> | <number> [] | <string> | <string>[]}
      const char *controlHeader = eventJSON["header"];
      int controlValue = eventJSON["values"][0];
      const char *controlSender = eventJSON["from"];

      Serial.println("This is the latest control header: ");
      Serial.println(controlHeader);
      Serial.println("These are the values: ");
      Serial.println(controlValue);
      Serial.println("This is who sent the control:");
      Serial.println(controlSender);
      Serial.println("Here is the pretty JSON:");
      serializeJsonPretty(eventJSON, Serial);
      Serial.println();

      if (strcmp(controlHeader, "synthGateA") == 0)
      {
        gateoutA(controlValue);
      }
      else if (strcmp(controlHeader, "synthGateB") == 0)
      {
        gateoutB(controlValue);
      }
      else if (strcmp(controlHeader, "synthGateC") == 0)
      {
        gateoutC(controlValue);
      }
      else if (strcmp(controlHeader, "synthGateD") == 0)
      {
        gateoutD(controlValue);
      }
      else if (strcmp(controlHeader, "synthCV_A") == 0)
      {
        CVoutA(controlValue);
      }
      else if (strcmp(controlHeader, "synthCV_B") == 0)
      {
        CVoutB(controlValue);
      }
      else if (strcmp(controlHeader, "synthCV_C") == 0)
      {
        CVoutC(controlValue);
      }
      else if (strcmp(controlHeader, "synthCV_D") == 0)
      {
        CVoutD(controlValue);
      }
    }
  }
  break;
  case sIOtype_ACK:
    Serial.printf("[IOc] get ack: %u\n", length);
    hexdump(payload, length);
    break;
  case sIOtype_ERROR:
    Serial.printf("[IOc] get error: %u\n", length);
    hexdump(payload, length);
    break;
  case sIOtype_BINARY_EVENT:
    Serial.printf("[IOc] get binary: %u\n", length);
    hexdump(payload, length);
    break;
  case sIOtype_BINARY_ACK:
    Serial.printf("[IOc] get binary ack: %u\n", length);
    hexdump(payload, length);
    break;
  }
}

//***************************Main Code**********************************
void setup() {
  Serial.begin(115200);
  //Keep these pinmodes for every new project. Hardwired to Motivation Radio Hardware

  pinMode(DAC0_CS, OUTPUT);
  pinMode(DAC1_CS, OUTPUT);
  pinMode(ADC_CS, OUTPUT);
  digitalWrite(DAC0_CS, HIGH);
  digitalWrite(DAC1_CS, HIGH);
  digitalWrite(ADC_CS, HIGH);
  pinMode(GATEout_0, OUTPUT);
  pinMode(GATEout_1, OUTPUT);
  pinMode(GATEout_2, OUTPUT);
  pinMode(GATEout_3, OUTPUT);

  pinMode(GATEin_0, INPUT);
  pinMode(GATEin_0, INPUT);
  pinMode(GATEin_0, INPUT);
  pinMode(GATEin_0, INPUT);

  SPI.begin(SCLK, MISO, MOSI, DAC0_CS); // we actually use a CS pin for each DAC
  SPI.setBitOrder(MSBFIRST);
  SPI.setFrequency(2000000); // ADC max clock 2 Mhz

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // initialize with the I2C addr 0x3C (for the 128x32)
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.clearDisplay();
  display.println(" Collab-Hub MR Client");
  display.display();
  delay(5000);
  //Serial.setDebugOutput(true);
  Serial.setDebugOutput(true);

  Serial.println();
  Serial.println();
  Serial.println();
  /*
  for (uint8_t t = 4; t > 0; t--)
  {
    Serial.printf("[SETUP] BOOT WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }
  */
  // disable AP
  if (WiFi.getMode() & WIFI_AP)
  {
    WiFi.softAPdisconnect(true);
  }

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(50);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("WiFi Connection established to ");
  Serial.println(WiFi.SSID());

  socketIO.begin(host, port, "/socket.io/?EIO=4");
  socketIO.onEvent(socketIOEvent);
}

void loop() {
  socketIO.loop();
}