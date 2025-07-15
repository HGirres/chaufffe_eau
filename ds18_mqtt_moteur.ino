#include <WiFi.h>
#include <MQTTPubSubClient.h>
#include <OneWire.h>
#include <floatToString.h>

#define consigne 30

const char* ssid = "HomeSweetHome";
const char* pass = "maisonbleuegrenouilleverte";

int DS18S20_Pin_1 = 4; //DS18S20 Signal pin on digital 4
int DS18S20_Pin_2 = 16; //DS18S20 Signal pin on digital 16
int DS18S20_Pin_3 = 17; //DS18S20 Signal pin on digital 17
int commande_pin = 23;     // GPIO de commande de la pompe
int commande_led = 23;     // GPIO de commande de la LED
int inter = 21;      // GPIO de commande forcée du moteur


OneWire ds1(DS18S20_Pin_1);
OneWire ds2(DS18S20_Pin_2);
OneWire ds3(DS18S20_Pin_3);

WiFiClient client;
MQTTPubSubClient mqtt;

float getTemp(OneWire ds) {
  //returns the temperature from one DS18S20 in DEG Celsius

  byte data[12];
  byte addr[8];

  if ( !ds.search(addr)) {
    //no more sensors on chain, reset search
    ds.reset_search();
    return -1000;
  }

  if ( OneWire::crc8( addr, 7) != addr[7]) {
    Serial.println("CRC is not valid!");
    return -1000;
  }

  if ( addr[0] != 0x10 && addr[0] != 0x28) {
    Serial.print("Device is not recognized");
    return -1000;
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1); // start conversion, with parasite power on at the end

  byte present = ds.reset();
  ds.select(addr);
  ds.write(0xBE); // Read Scratchpad


  for (int i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = ds.read();
  }

  ds.reset_search();

  byte MSB = data[1];
  byte LSB = data[0];

  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  float TemperatureSum = tempRead / 16;

  return TemperatureSum;

}
void setup() {
  Serial.begin(115200);
  pinMode(commande_pin, OUTPUT);
  digitalWrite(commande_pin, LOW);
  pinMode(commande_led, OUTPUT);
  digitalWrite(commande_led, LOW);
  pinMode(inter, INPUT);
  /*

    WiFi.begin(ssid, pass);

    Serial.print("connecting to wifi...");
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(1000);
    }
    Serial.println(" connected!");

    Serial.print("connecting to host...");
    while (!client.connect("192.168.1.60", 1883)) {
      Serial.print(".");
      delay(1000);
    }
    Serial.println(" connected!");

    // initialize mqtt client
    mqtt.begin(client);

    Serial.print("connecting to mqtt broker...");
    while (!mqtt.connect("192.168.1.60", "", "")) {
      Serial.print(".");
      delay(1000);
    }

    Serial.println(" connected!");

    // subscribe callback which is called when every packet has come
    mqtt.subscribe([](const String & topic, const String & payload, const size_t size) {
      Serial.println("mqtt received: " + topic + " - " + payload);
    });

    // subscribe topic and callback which is called when /hello has come
    mqtt.subscribe("/hello", [](const String & payload, const size_t size) {
      Serial.print("/hello ");
      Serial.println(payload);
    });
  */
}

void loop() {
  //mqtt.update();  // should be called
  static uint32_t prev_ms = millis();
  char message1[10];
char message2[10];
char message3[10];
  // publish message

  if (millis() > prev_ms + 5000) {

    prev_ms = millis();

    float temp1 = getTemp(ds1);
    floatToString(temp1, message1, sizeof(message1), 2);
    //Serial.print("Température chauffe eau : ");
    //Serial.println(message1);
    //Serial.print("connecting to mqtt broker...");
    /*   while (!mqtt.connect("192.168.1.60", "", "")) {
         Serial.print(".");
         delay(1000);
       }
    */
    // mqtt.publish("/homeassistant/temperature/t1", message);
    float temp2 = getTemp(ds2);
    floatToString(temp2, message2, sizeof(message2), 2);
    //Serial.print("Température air : ");
    //Serial.println(message2);
    //  mqtt.publish("/homeassistant/temperature/t2", message);
    float temp3 = getTemp(ds3);
    floatToString(temp3, message3, sizeof(message3), 2);
    //Serial.print("Température piscine");
    //Serial.println(message3);
    // mqtt.publish("/homeassistant/temperature/t3", message);
    Serial.print( message1 );
    Serial.print( ",");
    Serial.print(message2);
    Serial.print(",");
    Serial.println( message3 );
    if (temp1 > consigne) {
      digitalWrite(commande_pin, HIGH);
      delay(3 *  1000 );
      digitalWrite(commande_pin, LOW);
    }
    if (digitalRead(inter))
    {
      digitalWrite(commande_pin, HIGH);
      delay( 1000 );
      digitalWrite(commande_pin, LOW);
    }
  }

}
