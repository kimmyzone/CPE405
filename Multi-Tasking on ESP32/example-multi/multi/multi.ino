

#include <PubSubClient.h>      //connect and publish to the MQTT broker
#include <WiFiClientSecure.h>
#include <WiFi.h>              //Enables esp32 to connect the local network (pass WiFi)


// share variable
TaskHandle_t t0;
TaskHandle_t t1;

// WiFi
const char* ssid = "Chawevan 2.4G";       
const char* wifi_password = "08768859";   

// MQTT
const char* mqtt_server = "203.150.107.212";  //IP the mqtt broker
const char* mqtt_username = "";  //mqtt username
const char* mqtt_password = "";  //mqtt password
const char* clientID = "";       //mqtt client ID

long lastReconnectAttempt = 0;
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;


//start WiFi and MQTT Client objects
WiFiClient wifiClient;
//1883 is the listener port for the Broker
PubSubClient client(mqtt_server, 1883,wifiClient); 

void setup_wifi() {
  delay(10);
  // we start by connecting to a WiFi network
  Serial.print("Connecting to ");
  Serial.println(ssid);


  WiFi.begin(ssid, wifi_password);
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, HIGH);   //led turn off when can't connect Wifi access point
    Serial.println("turn off led when cannot connect Wifi");
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

   //connect MQTT Broker
   if (client.connect(clientID, mqtt_username, mqtt_password)) {
    Serial.println("set up on loop core1 : Connected to MQTT Broker!");
  }
  else {
    Serial.println("set up on loop core1 : Connection to MQTT Broker failed...");
  }
}


void setup() {
  pinMode(0, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
 
  Serial.begin(115200);
  
  //create a task to handle button gpio 0 (core 0)
  xTaskCreatePinnedToCore(
    tSetButtonFunc,  /* Task function. */
    "Button",     /* name of task. */
    10000,        /* Stack size of task */
    NULL,         /* parameter of the task */
    1,            /* priority of the task */
    &t0,          /* Task handle to keep track of created task */
    1);           /* pin task to core 0 */
  delay(500);

  // create a task to handle led LED_BUILTIN (core 1)
  xTaskCreatePinnedToCore(
    tConnectFunc,     /* Task function. */
    "Connect",        /* name of task. */
    10000,        /* Stack size of task */
    NULL,         /* parameter of the task */
    1,            /* priority of the task */
    &t1,          /* Task handle to keep track of created task */
    0);           /* pin task to core 1 */
  delay(500);
}


void loop() {
  // no coding here
  Serial.print("loop running on core");
  Serial.println(xPortGetCoreID());
  delay(1000);
}


boolean reconnect() {
  Serial.println("reconnect active");
  if (!client.connected() ) {
    Serial.println("connecting...");
    // once connected, publish an announcement...
     if (client.connect(clientID, mqtt_username, mqtt_password)) {
      Serial.println("Connected MQTT broker waiting publish");
      
      // Subscribe
      client.subscribe("#");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" Try again in 5sec");
      
    }
  }
  return client.connected();
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Turn on the LED if 1 is given as the first character.
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   //turn on LED(Note that LOW is the voltage level
  
    // but actually the LED is on; because it has low usage on ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  //turn on LED by making the voltage HIGH
  }

}


// tButtonFunc: check button for push and release events
void tSetButtonFunc(void *params) {
  // localVariables
  bool lastState = false;
  
  Serial.print("tButtonFunc running on core ");
  Serial.println(xPortGetCoreID());

  while (true) {
    
    if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, HIGH);  //turn off led when can't connect Wifi Access point
    Serial.println("turn off led when cannot connect Wifi");
    delay(1000);
   }  
    if(WiFi.status() == WL_CONNECTED){
    Serial.println("loop core0 : Connect WIFI, Turn on led 500 ms ");  
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
    Serial.println("loop core0 : Connect WIFI, Turn off led 500 ms ");  
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500); 
    }
    else if(client.connected()){
    digitalWrite(LED_BUILTIN, LOW);  //turn on led when connect mqtt broker finish
    Serial.println("loop core0 : Connect MQTT, Turn on led ");
    }

  } 
}

// tLedFunc: blinks every 1000ms
void tConnectFunc(void *params) {
  
  // local variable 
  Serial.print("tConnectFunc running on core ");
  Serial.println(xPortGetCoreID());
  Serial.print("Connecting to ");
  Serial.println(ssid);
    // connect WiFi
  setup_wifi();
  
    // Connect MQTT Broker
    
  while (true) {
    if (!client.connected()) {
      
    long now = millis();
    
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      Serial.println("loop core 1 : Counting.. ,not connect");
      Serial.println("Counter is reset : 0");
      Serial.print("Counter Change to : ");
      Serial.println(lastReconnectAttempt);
      // try to reconnect
      if (reconnect()) {
        lastReconnectAttempt = 0;
        Serial.println("loop core 1 : Reconnecting..., turn off LED: ");    // -- led turn off when  can't connect Wifi Access point
      }
    }
  }
    else if (client.connected()) {
    client.loop();
    Serial.println("loop core1 : Connected...");
    Serial.print("IP address:  ");
    Serial.println(WiFi.localIP()); 

  //  Serial.println("loop core1 : Connect MQTT, Turn on led ");          
     client.disconnect();
     delay(50);
  }
  delay(1000);
  }
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    Serial.println("loop core1 : led swap");
    delay(1000);
}
