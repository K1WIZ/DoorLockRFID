// Wireless RFID Door reader w/ MQTT & Domoticz support 
// Forked from LUIS SANTOS & RICARDO VEIGA https://www.instructables.com/id/Wireless-RFID-Door-Lock-Using-Nodemcu/
// July 30, 2019
//
// Code Revision July 30, 2019 by John Rogers john at wizworks dot net   http://wizworks.net/keyless-entry
//

#include <Wire.h>
#include <SSD1306.h>
#include <MFRC522.h>
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>

#define RST_PIN 20 // RST-PIN for RC522 - RFID - SPI - Module GPIO15 
#define SS_PIN  2  // SDA-PIN for RC522 - RFID - SPI - Module GPIO2
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance

#define Relay 10
#define BlueLed 15
#define GreenLed 0
#define RedLed 3

SSD1306  display(0x3c, 4, 5);

//Wireless name and password
const char* ssid        = "";               // replace with your wireless network name
const char* password    = "";               // replace with your wireless network password

// Remote site information
const char* host        = "";               // IP address of your local server
String url              = "/fobs.txt";      // folder location of the txt file with the RFID cards identification, if on the root of the server
                                            // each line MUST begin with a space!  put empty line at end of file to signify EOF.

// Define MQTT parameters
const char* mqtt_server = "";
const char* mqtt_user   = "";
const char* mqtt_passwd = "";
String clientId         = "";               // MQTT CLIENT ID SHOULD BE UNIQUE!!!
const char* inTopic     = "domoticz/out";   // Leave if using Domoticz, else change for your needs
const char* outTopic    = "domoticz/in";    // Leave if using Domoticz, else change for your needs

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

String tag= "";  // Card read tag declaration

int time_buffer = 5000; // amount of time in miliseconds that the relay will remain open

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");  
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_passwd)) {
      Serial.println("connected");
      // ... and resubscribe
      client.subscribe(inTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(Relay, OUTPUT);
  digitalWrite(Relay,0);
    
  display.init();
  display.flipScreenVertically();
  display.setContrast(255);
  
  Serial.begin(115200);    // Initialize serial communications
  SPI.begin();           // Init SPI bus
  mfrc522.PCD_Init();    // Init MFRC522

  // We start by connecting to a WiFi network

  Serial.println("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  wifi_connected();
  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  leds_off();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  delay(3000);
  
}

void leds_off() {
  analogWrite(BlueLed, 0);   // turn the LED off
  analogWrite(GreenLed, 0);   // turn the LED off
  analogWrite(RedLed, 0);   // turn the LED off
  display.clear();
  display.display();
}

void reject() {
  // Align text vertical/horizontal center
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
  display.setFont(ArialMT_Plain_16);
  display.drawString(display.width()/2, 10+ display.height()/4, "NOT");
  display.drawString(display.width()/2, 30+ display.height()/4, "AUTHORIZED");
  display.display();
  client.publish(outTopic, "{\"command\" : \"addlogmessage\", \"message\" : \"INVALID DOOR TAG PRESENT\" }");  // LOG status to Domoticz when rejected tag is scanned.
  analogWrite(RedLed, 767);   // turn the Red LED on
  delay(2000);
  leds_off(); 
}

void authorize() {
  // Align text vertical/horizontal center
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
  display.setFont(ArialMT_Plain_16);
  display.drawString(display.width()/2, display.height()/2, "AUTHORIZED");
  display.display();
  
  analogWrite(GreenLed, 767);   // turn the Green LED on
  client.publish(outTopic, "{ \"command\": \"switchlight\", \"idx\": 210, \"switchcmd\": \"Off\" }");
  digitalWrite(Relay,1);
  delay(time_buffer);              // wait for a second 
  digitalWrite(Relay,0);
  leds_off(); 
}

void wifi_connected () {
  // Align text vertical/horizontal center
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
  display.setFont(ArialMT_Plain_16);
  display.drawString(display.width()/2, 10+ display.height()/4, "WI-FI");
  display.drawString(display.width()/2, 20+ display.height()/2, "CONNECTED");
  display.display();
  delay(3000);  
}

void connection_failed() {
  // Align text vertical/horizontal center
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
  display.setFont(ArialMT_Plain_16);
  display.drawString(display.width()/2, 10+ display.height()/4, "CONNECTION");
  display.drawString(display.width()/2, 20+ display.height()/2, "FAILED");
  display.display();
  delay(3000);
  leds_off();   
}

// Helper routine to dump a byte array as hex values to Serial
void dump_byte_array(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  int authorized_flag = 0;
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) {   
    delay(50);
    return;
  }
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {   
    delay(50);
    return;
  }

////-------------------------------------------------RFID----------------------------------------------


  // Shows the card ID on the serial console
  tag= "";  // clear tag contents before each read
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
     tag.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     tag.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println();
  tag.toUpperCase();
  Serial.println("Card Read:" + tag);

////-------------------------------------------------SERVER----------------------------------------------

  
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    connection_failed();
    return;
  }
  
  // This will send the request to the server
  //client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: close\r\n\r\n");
  client.print(String("GET ") + url + "\r\n\r\n");

  delay(10);

  // Read all the lines of the reply from server and print them to Serial
  String line;
  while(client.available()){
     line = client.readStringUntil('\n');
     //Serial.println(line);
     //Serial.println(tag);
     
    if(line==tag){
      authorized_flag=1;
    }
  }
  
  if(authorized_flag==1){
    Serial.println("AUTHORIZED");
    authorize();
  }
  else{
    Serial.println("NOT AUTHORIZED");
    reject();
  }
}
