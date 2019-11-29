#include "DHT.h"
#include <MQTT.h>
#include <WiFiNINA.h>
#include <Adafruit_DotStar.h>
#include <SPI.h> 
#include <U8g2lib.h>


#define btnPin 1
#define mosPin 4
#define DHTPIN 0
#define DHTTYPE DHT22
#define NUMPIXELS 87
#define DATAPIN    1
#define CLOCKPIN   2
Adafruit_DotStar strip(NUMPIXELS, DATAPIN, CLOCKPIN, DOTSTAR_BRG);


#define TEMP_READ_DELAY 2000
#define BROKER_IP    "192.168.31.49"
#define DEV_NAME     "mqttdevice"
#define MQTT_USER    "mqtt_user"
#define MQTT_PW      "mqtt_password"
U8G2_SSD1306_128X32_UNIVISION_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);   // Adafruit ESP8266/32u4/ARM Boards + FeatherWing OLED

double setTemperatura = 15;
int tolleranza = 1;

const char ssid[] = "jasmina-caravan";
const char pass[] = "wauholland";
WiFiClient net;
MQTTClient client;

void connect() {
  Serial.print("checking wifi...");
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.print("\nconnecting...");
  if (!client.connect(DEV_NAME, MQTT_USER, MQTT_PW)) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nconnected!");
  client.subscribe("/cantina/settemp");
  client.subscribe("/cantina/setTollerance");
}

void messageReceived(String &topic, String &payload) 
{ 
  Serial.println("arrivato");
  Serial.println(topic);
  if (topic == "/cantina/setTemp"){     //Da sql
    setTemperatura = payload.toDouble();
    Serial.print("Temperature changed to: ");
    Serial.println(payload);
  }
  if (topic == "/cantina/setTollerance"){
    tolleranza = payload.toInt();
  }
}

DHT dht(DHTPIN, DHTTYPE);
double temperature, outputVal, humidity;

unsigned long lastTempUpdate;
bool updateTemperature() {
  if ((millis() - lastTempUpdate) > TEMP_READ_DELAY) {
    humidity = dht.readHumidity();
    temperature = dht.readTemperature();
    //Serial.println("temperatura");
    Serial.println(temperature);
    client.publish("/cantina/temperatura", String(temperature));
    client.publish("/cantina/umidita", String(humidity));
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
  WiFi.begin(ssid, pass);
  dht.begin();
  client.begin(BROKER_IP, 1883, net);
  client.onMessage(messageReceived);
  connect();
  while (!updateTemperature());
  
  u8g2.begin();
  u8g2.setFont(u8g2_font_logisoso28_tf);
//---------Setup Luci-----------
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000L)
  clock_prescale_set(clock_div_1); // Enable 16 MHz on Trinket
#endif
  strip.begin(); // Initialize pins for output
  strip.show();  // Turn all LEDs off ASAP
  
}

int      head  = 14, tail = -10; // Index of first 'on' and 'off' pixels
uint32_t color = 0xb3d9ff,striscia=0xCC6699;      // 'On' color (starts red)
int headStripe=0, tailr=-13;
int h2=28,h3=42,h4=56;
int tail2=15,t3=27,t4=41; 

void loop() {
  updateTemperature();
  if (temperature > setTemperatura + 3) {
    outputVal = 255; 
  } else if (temperature < setTemperatura - 3) {
    outputVal = 0;
  } else {
    outputVal = 128;
  }
  digitalWrite(mosPin, HIGH);
  // analogWrite(mosPin, (int)outputVal);
  if (digitalRead(btnPin) == LOW){
    client.publish("/cantina/Vino", "Vino1");
  }
  client.loop();
  if (!client.connected()) connect();

  //-------Set Luci Led --------



  for(int i=0;i<14;i++)
 strip.setPixelColor(i, striscia); // 'On' pixel at head
 if(headStripe<14) headStripe++;

 
  strip.setPixelColor(head, 0x0066cc);// 'On' pixel at head
  strip.setPixelColor(tail, 0); 
  
  strip.show();                     // Refresh strip
                        // Pause 20 milliseconds (~50 FPS)

   for(int i=0;i<14;i++)
      strip.setPixelColor(i, striscia); // 'On' pixel at head
 
  if(++head >= NUMPIXELS) {         // Increment head index.  Off end of strip?
    head = 14;    } 
    
            
    if(++tail >= NUMPIXELS) tail = -10;

}
