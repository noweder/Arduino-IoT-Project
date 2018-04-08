
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"

// Uncomment one of the lines bellow for whatever DHT sensor type you're using!
#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

// Change the credentials below, so your ESP8266 connects to your router
const char* ssid = "AndroidAP";
const char* password = "hamooz1234";

// Change the variable to your Raspberry Pi IP address, so it connects to your MQTT broker
const char* mqtt_server = "192.168.43.211";

// Initializes the espClient. You should change the espClient name if you have multiple ESPs running in your home automation system
WiFiClient espClient;
PubSubClient client(espClient);

// DHT Sensor - GPIO 5 = D1 on ESP-12E NodeMCU board
const int DHTPin = 5;
const int DHTPin2 = 16;

// Lamp - LED - GPIO 4 = D2 on ESP-12E NodeMCU board
const int lamp = 4;
int ledblink=0;
int mod=5;
float difference;
int am=1;
// Initialize DHT sensor.
DHT dht(DHTPin, DHTTYPE);
DHT dht2(DHTPin2, DHTTYPE);

// Timers auxiliar variables
long now = millis();
long lastMeasure = 0;

// Don't change the function below. This functions connects your ESP8266 to your router
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
}

// This functions is executed when some device publishes a message to a topic that your ESP8266 is subscribed to
// Change the function below to add logic to your program, so when a device publishes a message to a topic that 
// your ESP8266 is subscribed you can actually do something
void callback(String topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT

  // If a message is received on the topic room/lamp, you check if the message is either on or off. Turns the lamp GPIO according to the message
  if(topic=="room/lamp2"){
      Serial.print("Changing A/C Temperature Balancing Mode to: ");
      if(messageTemp == "on"){
        am=1;
        Serial.print("AUTO");
      }
      else if(messageTemp == "off"){
        am=0;
        mod=3;
        Serial.print("MANUAL");
      }
  }
  if(am==0){
  if(topic=="room/lamp"){
      Serial.print("Changing A/C Switch to: ");
      if(messageTemp == "on"){
        digitalWrite(lamp, HIGH);
        Serial.print("ON");
      }
      else if(messageTemp == "off"){
        digitalWrite(lamp, LOW);
        Serial.print("OFF");
      }
  }
  }
  Serial.println();
}

// This functions reconnects your ESP8266 to your MQTT broker
// Change the function below if you want to subscribe to more topics with your ESP8266 
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    /*
     YOU MIGHT NEED TO CHANGE THIS LINE, IF YOU'RE HAVING PROBLEMS WITH MQTT MULTIPLE CONNECTIONS
     To change the ESP device ID, you will have to give a new name to the ESP8266.
     Here's how it looks:
       if (client.connect("ESP8266Client")) {
     You can do it like this:
       if (client.connect("ESP1_Office")) {
     Then, for the other ESP:
       if (client.connect("ESP2_Garage")) {
      That should solve your MQTT multiple connections problem
    */
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");  
      // Subscribe or resubscribe to a topic
      // You can subscribe to more topics (to control more LEDs in this example)
      client.subscribe("room/lamp");
      client.subscribe("room/lamp2");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// The setup function sets your ESP GPIOs to Outputs, starts the serial communication at a baud rate of 115200
// Sets your mqtt broker and sets the callback function
// The callback function is what receives messages and actually controls the LEDs
void setup() {
  pinMode(lamp, OUTPUT);
  
  dht.begin();
  
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

}

// For this project, you don't need to change anything in the loop function. Basically it ensures that you ESP is connected to your broker
void loop() {

  if (!client.connected()) {
    reconnect();
  }
  if(!client.loop())
    client.connect("ESP8266Client");

  now = millis();
  // Publishes new temperature and humidity every 10 seconds
  if (now - lastMeasure > 10000) {
    lastMeasure = now;
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float h = dht.readHumidity();
    float h2 = dht2.readHumidity();

    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    float t2 = dht2.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    float f = dht.readTemperature(true);
    float f2 = dht2.readTemperature(true);

    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t) || isnan(f)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    if (isnan(h2) || isnan(t2) || isnan(f2)) {
      Serial.println("Failed to read from DHT2 sensor!");
      return;
    }
    

    // Computes temperature values in Celsius
    float hic = dht.computeHeatIndex(t, h, false);
    float hic2 = dht2.computeHeatIndex(t2, h2, false);

    static char temperatureTemp[7];
    static char temperatureTemp2[7];

    dtostrf(hic, 6, 2, temperatureTemp);
    dtostrf(hic2, 6, 2, temperatureTemp2);

    static char humidityTemp[7];
    static char humidityTemp2[7];
    dtostrf(h, 6, 2, humidityTemp);
    dtostrf(h2, 6, 2, humidityTemp2);

    // Publishes Temperature and Humidity values
    client.publish("room/temperature", temperatureTemp);
    client.publish("room/temperature2", temperatureTemp2);

    client.publish("room/humidity", humidityTemp);
    client.publish("room/humidity2", humidityTemp2);

    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.print(" %\t Temperature: ");
    Serial.print(t);
    Serial.print(" *C ");
    Serial.print(f);
    Serial.print(" *F\t Heat index: ");
    Serial.print(hic);
    Serial.println(" *C ");

    Serial.print("Humidity2: ");
    Serial.print(h2);
    Serial.print(" %\t Temperature2: ");
    Serial.print(t2);
    Serial.print(" *C ");
    Serial.print(f2);
    Serial.print(" *F\t Heat index: ");
    Serial.print(hic2);
    Serial.println(" *C ");
    // Serial.print(hif);
    // Serial.println(" *F");

    if ((t > t2 || t2 > t)&& am == 1){ // Comparing the difference in temperature and powering A/C when difference is more than 2 degrees, also publish the LED again
    difference = t - t2;
    if (difference > 2) {
      Serial.println("A/C COOLING is Active..."); // LED will be Static ON, Equipment room's Temperature is Higher than outside, needs Cooling to Balance
      digitalWrite(lamp, HIGH);
      mod=0;
      client.publish("room/lamp3", "on");
      client.publish("room/lamp4", "Cooling is Active");

      } else if (difference < -2) {
        Serial.println("A/C Heating is Active..."); // LED will be blinking in this case, Equipment room's Temperature is Lower than outside, needs Heating to Balance
        //digitalWrite(lamp, HIGH); //
        mod=1;
        client.publish("room/lamp3", "on"); 
        client.publish("room/lamp4", "Heating is Active");


        } else {
          Serial.println("Temperature is In Balance now... A/C is Inactive...");
          digitalWrite(lamp, LOW);
          mod=2;
          client.publish("room/lamp3", "off"); //turn off light

        }

   
     }
  }

    if(mod==1){
        if (ledblink == 1) {
          digitalWrite(lamp, LOW);
          ledblink=0;
          // turn LED OFF
        } else {
           digitalWrite(lamp, HIGH);  // turn LED ON
           ledblink=1;
        }
    }
        delay (300); // control speed of blinking



   
} 

