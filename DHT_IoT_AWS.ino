#include "confidential.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <TimeLib.h>
#include "WiFi.h"
#include "time.h"

 
#include "DHT.h"
#define DHTPIN 14     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11   // DHT 11
 
#define AWS_IOT_PUBLISH_TOPIC   "IoT/DHT11"
#define AWS_IOT_SUBSCRIBE_TOPIC "IoT/DHT11"
 
float humidity;
float temp;
int dht_year;
int dht_month;
int dht_day;
int dht_hour;
int dht_minute;
int dht_second;

int buf_size=20;
char dht_time[20];

int dt=20000;


const char* ntpServer = "pool.ntp.org";

//You need to adjust the UTC offset for your timezone in milliseconds. Refer the list of UTC time offsets.
const long  gmtOffset_sec = 32400;

//Change the Daylight offset in milliseconds. If your country observes Daylight saving time set it to 3600. Otherwise, set it to 0
const int   daylightOffset_sec = 0;

 
DHT dht(DHTPIN, DHTTYPE);
 
WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);


void setup()
{
  Serial.begin(9600);
  connectAWS();
  dht.begin();
  
  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
}




 
void loop(){

  if (!client.connected()) {
    reconnect();
  }

  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  //Serial.println(&timeinfo, "%A %B %d %Y %H:%M:%S");

  dht_year=timeinfo.tm_year+1900;
  dht_month=timeinfo.tm_mon+1;
  dht_day=timeinfo.tm_mday;
  dht_hour=timeinfo.tm_hour;
  dht_minute=timeinfo.tm_min;
  dht_second=timeinfo.tm_sec;

  

  
  snprintf(dht_time, buf_size, "%d/%d/%d+%d:%d:%d", dht_year, dht_month, dht_day,dht_hour,dht_minute,dht_second);

  
  humidity=dht.readHumidity();
  temp=dht.readTemperature();

   Serial.print("Time: ");
   Serial.print(dht_time);
   
   Serial.print(" Humidity: ");
   Serial.print(humidity);
   Serial.print(" Temperature: ");
   Serial.println(temp);
   client.loop();
   publishMessage();   
   delay(dt);
 
}



void publishMessage()
{
  StaticJsonDocument<200> doc;  
   
  doc["time"]=dht_time;
  doc["humidity"] = humidity;
  doc["temperature"] = temp;
  
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); 
  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
  
}




 
void messageHandler(char* topic, byte* payload, unsigned int length)
{
  Serial.print("incoming: ");
  Serial.println(topic);
 
  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);
  const char* message = doc["message"];
  Serial.println(message);
}











void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP32")) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("IoT/DHT11");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void connectAWS()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
 
  Serial.println("Connecting to Wi-Fi");
 
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
 
  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);
 
  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.setServer(AWS_IOT_ENDPOINT, 8883);
 
  // Create a message handler
  client.setCallback(messageHandler);
 
  Serial.println("Connecting to AWS IOT");
 
  while (!client.connect(THINGNAME))
  {
    Serial.print(".");
    delay(100);
  }
 
  if (!client.connected())
  {
    Serial.println("AWS IoT Timeout!");
    return;
  }
 
  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
 
  Serial.println("AWS IoT Connected!");
}








 
