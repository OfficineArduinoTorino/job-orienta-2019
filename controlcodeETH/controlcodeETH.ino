#include "DHT.h"
#include <MQTT.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <Ethernet.h>

#define mosPin 2
#define DHTPIN 4
#define TEMP_READ_DELAY 3000
#define DHTTYPE DHT22

#define BROKER_IP    "192.168.0.219"
#define DEV_NAME     "mqttdevice"
#define MQTT_USER    "mqtt_user"
#define MQTT_PW      "mqtt_password"
U8G2_SSD1306_128X32_UNIVISION_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);   // Adafruit ESP8266/32u4/ARM Boards + FeatherWing OLED

double setPoint = 18;

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

EthernetClient net;
MQTTClient client;
unsigned long lastMillis = 0;
void connect() {

  Serial.print("\nconnecting...");
  if (!client.connect(DEV_NAME, MQTT_USER, MQTT_PW)) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nconnected!");
  client.subscribe("/cantina1/setpoint");
}

void messageReceived(String &topic, String &payload) {
  setPoint = payload.toDouble();
  Serial.print("setpoint: ");
  Serial.print(setPoint);
}
DHT dht(DHTPIN, DHTTYPE);
double temperature, outputVal, humidity;

unsigned long lastTempUpdate;
bool updateTemperature() {
  if ((millis() - lastTempUpdate) > TEMP_READ_DELAY) {
    humidity = dht.readHumidity(); //leggihum
    temperature = dht.readTemperature(); //leggitemp
    Serial.print(F("\nHumidity: "));
    Serial.print(humidity);
    Serial.print(F("%  Temperature: "));
    Serial.print(temperature);
    Serial.print(F("°C "));
    client.publish("/cantina1/temperatura", String(temperature));
    client.publish("/cantina1/umidita", String(humidity));
    lastTempUpdate = millis();
    u8g2.firstPage();
    do {
      u8g2.setCursor(0, 32);
      u8g2.print(String(temperature, 1));
      u8g2.print((char) 0xb0);
      u8g2.print("C");
    } while ( u8g2.nextPage() );
    return true;
  }
  return false;
}

void setup() {
  Serial.begin(9600);
  Serial.println("Initialize Ethernet with DHCP:");
  Ethernet.init(5);
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // Check for Ethernet hardware present
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    }
    if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable is not connected.");
    }
  } else {
    Serial.print("  DHCP assigned IP ");
    Serial.println(Ethernet.localIP());
  }  dht.begin();
  client.begin(BROKER_IP, 1883, net);
  client.onMessage(messageReceived);
  connect();
  //client.publish("/cantina1/porta", "OPEN");
  while (!updateTemperature());//wait until temp sensor updated
  //if temperature is more than 4 degrees below or above setpoint, OUTPUT will be set to min or max respectively
  u8g2.begin();
  u8g2.setFont(u8g2_font_logisoso28_tf);
}

void loop() {
  updateTemperature();
  if (temperature > setPoint + 1) {
    outputVal = 255;
  } else if (temperature < setPoint - 1) {
    outputVal = 0;
  } else {
    outputVal = 128;
  }
  analogWrite(mosPin, (int)outputVal);
  Serial.println((int)outputVal);
  client.loop();
  if (!client.connected()) connect();
}
