#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

/*
 * Will accept a JSON in this format (on a mqtt topic)
 * {"p":1,"m":2, "d":10,"i":100}
 * p: output port (0-3)
 * m: mode : 0 - turn off, 1 - turn on, 2 - enable for d seconds
 * d: number of econds to be enabled dor mode 2
 * i: how much we have to wait before beeing able to accept a new mode 2 command
 * 
 * E.g. enable for 10 seconds, but don't accept any other mode2 command before 100 seconds have passed
 */

#define MODE_DISABLE 0
#define MODE_ENABLE 1
#define MODE_TIMER 2

//change this to your actual wifi settings
const char* ssid = "Sergiu";
const char* password = "1234567890";
const char* mqtt_server = "192.168.0.108";

#define TOPIC "control2"

#undef ULONG_MAX
#define ULONG_MAX (LONG_MAX * 2UL + 1UL)

//#define DEBUG 1

//time has to be higher than this to accept a MODE_TIMER commnad on a pin (milliseconds)
unsigned long sinceMode2_pins[4];
unsigned long endTimesMode2[4];

WiFiClient espClient;
PubSubClient client(espClient);

// ArduinoJson
const size_t capacity = JSON_OBJECT_SIZE(4) + 30;
DynamicJsonBuffer jsonBuffer(capacity);
//-----
void setup_wifi() {

  delay(10);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  randomSeed(micros());
}


//orking pin
byte json_p;
//pin mode
byte json_m;
//duration for timer mode
long json_d;
//interval for timer mode
long json_i;

void callback(char* topic, byte* payload, unsigned int length) {
  #ifdef DEBUG
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.println("] ");
  Serial.println((char*)payload);
  #endif

  JsonObject& root = jsonBuffer.parseObject(payload);

  json_p = root["p"];
  json_m = root["m"];
  json_d = root["d"];
  json_i = root["i"];

  setMode(json_p,json_m,json_d,json_i);

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    // Create a random client ID
    String clientId = "Fan1-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      client.subscribe(TOPIC);
    } else {
     
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  // put your setup code here, to run once:
  #ifdef DEBUG
  Serial.begin(9600);
  #endif
  
  pinMode(0, OUTPUT);
  pinMode(2, OUTPUT);
  #ifdef DEBUG
  #else
  pinMode(1, OUTPUT);
  pinMode(3, OUTPUT);
  #endif


  digitalWrite(0,LOW);
  digitalWrite(2,LOW);
  #ifdef DEBUG
  #else
  digitalWrite(1,LOW);
  digitalWrite(3,LOW);;
  #endif
 

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  fakeWatchDog();
  client.loop();
  checkMode2Stop();
}

//will reboot after 1 hour, to make sure everything is fresh
void fakeWatchDog() {
     #ifdef DEBUG
//  Serial.print("uptime: ");Serial.println(millis()/1000);
  #endif
  //one hour
  if (millis() > 3600*1000) {
    #ifdef DEBUG
    Serial.print("Softare reset");
    #endif
    ESP.restart();
  }
}

byte count0 = 0;
void checkMode2Stop() {
  for(count0=0;count0<4;count0++) {
    if (millis() > endTimesMode2[count0] ) {
       #ifdef DEBUG
//      Serial.print("turn off by timer: ");
      #endif
      turnOff(count0);
    }
  }
}

void setMode(byte pin, byte modeSet, long duration, long interval) {

  #ifdef DEBUG
  Serial.print("pin: ");Serial.println(pin);
  Serial.print("mode: ");Serial.println(modeSet);
  Serial.print("duration: ");Serial.println(duration);
  Serial.print("interval: ");Serial.println(interval);
  #endif
  
   if (modeSet == MODE_DISABLE) {
      turnOff(pin);
   }

   if (modeSet == MODE_ENABLE) {
      turnOn(pin);
      endTimesMode2[pin] = ULONG_MAX;
   }

   if (modeSet == MODE_TIMER) {
      if(millis() > sinceMode2_pins[pin]) {
          endTimesMode2[pin] = millis() + duration*1000;
          sinceMode2_pins[pin] = millis() + interval*1000;
          turnOn(pin);
      }
   }
}

void turnOn(byte pin) {   
   #ifdef DEBUG
//  Serial.print("enable: ");Serial.println(pin);
  #endif
    digitalWrite(pin,HIGH);
}

void turnOff(byte pin) { 
    #ifdef DEBUG
//  Serial.print("disable: ");Serial.println(pin);
  #endif
    digitalWrite(pin,LOW);
}

