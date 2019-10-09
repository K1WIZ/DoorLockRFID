

// Wireless RFID Door reader w/ MQTT & Domoticz support 
// Forked from LUIS SANTOS & RICARDO VEIGA https://www.instructables.com/id/Wireless-RFID-Door-Lock-Using-Nodemcu/
// July 30, 2019
//
// Code Revision October 8, 2019 by John Rogers john at wizworks dot net   http://wizworks.net/keyless-entry
//
#include <U8x8lib.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <SSD1306.h>
#include <MFRC522.h>
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>

#define RST_PIN 20 // RST-PIN for RC522 - RFID - SPI - Module GPIO15 
#define SS_PIN  2  // SDA-PIN for RC522 - RFID - SPI - Module GPIO2
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance

#define Relay 10
#define BlueLed 15
#define GreenLed 0
#define RedLed 3

SSD1306  display(0x3c, 4, 5);

// Wireless name and password
const char* ssid        = "";               // replace with your wireless network name
const char* password    = "";               // replace with your wireless network password

// Remote Host running fobcheck
String host = "";
String url = "";

String tag= "";  // Card read tag declaration
int time_buffer = 5000; // amount of time in miliseconds that the relay will remain open

void setup() {
  pinMode(Relay, OUTPUT);
  digitalWrite(Relay,0);
    
  display.init();
  display.flipScreenVertically();
  display.setContrast(255);
  
  Serial.begin(115200);    // Initialize serial communications
  SPI.begin();           // Init SPI bus
  mfrc522.PCD_Init();    // Init MFRC522
  mfrc522.PCD_SetAntennaGain(0xFF);

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
  delay(3000);
  
}

void leds_off() {
  analogWrite(BlueLed, 0);   // turn the LED off
  analogWrite(GreenLed, 0);   // turn the LED off
  analogWrite(RedLed, 0);   // turn the LED off
  display.clear();
  display.display();
}

  //client.publish(outTopic, "{\"command\" : \"addlogmessage\", \"message\" : \"INVALID DOOR TAG PRESENT\" }");  // LOG status to Domoticz when rejected tag is scanned.
 
void foneIn() {
  HTTPClient http;    //Declare object of class HTTPClient
  // Post Tag to Connector
  String postData = "search=" + tag;
  http.begin("http://" + host + url);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int httpCode = http.POST(postData);   //Send the request
  String payload = http.getString();    //Get the response payload
 
  Serial.println(httpCode);   //Print HTTP return code
  Serial.println(payload);    //Print request response payload
 
  http.end();  //Close connection
}

void process() {
  // Align text vertical/horizontal center
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
  display.setFont(ArialMT_Plain_16);
  display.drawString(display.width()/2, display.height()/2, "AUTHORIZED");
  display.display();
  foneIn();
  analogWrite(GreenLed, 767);   // turn the Green LED on
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
//  client.loop();
  //int authorized_flag = 0;
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
  tag.trim();
  tag.toUpperCase();
  Serial.println("Card Read: " + tag);

////-------------------------------------------------SERVER----------------------------------------------

  delay(1000);
  Serial.println("SENDING");
  process();
  tag= "";
}
