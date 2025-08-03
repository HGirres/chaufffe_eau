#include <WiFi.h>
#include <MQTTPubSubClient.h>
#include <OneWire.h>
#include <floatToString.h>

#define consigne 50
#define nb_moyenne  10
#define WIFI
#define DEBUG

const char* ssid = "HomeSweetHome";
const char* pass = "maisonbleuegrenouilleverte";

int DS18S20_Pin_1 = 4; //DS18S20 Signal pin on digital 4
int DS18S20_Pin_2 = 16; //DS18S20 Signal pin on digital 16
int DS18S20_Pin_3 = 17; //DS18S20 Signal pin on digital 17
int commande_pin = 23;     // GPIO de commande de la pompe
int commande_led = 22;     // GPIO de commande de la LED
int inter = 21;      // GPIO de commande forcée du moteur

int no_iter = 0;
float t1[nb_moyenne];
float t2[nb_moyenne];
float t3[nb_moyenne];

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


float moyenne_tab(float tab[]) {

  float cumul = 0;
  for (int i = 0; i < nb_moyenne - 1 ; i = i + 1)
  {
    cumul = cumul + tab[i];
  }
  return cumul / nb_moyenne;
}

String FloatToStr(float anumber)
{
  char str[10] = "";
  floatToString(anumber, str, sizeof(str), 2);
  return str;
}

void setup() {
  Serial.begin(115200);
  pinMode(commande_pin, OUTPUT);
  digitalWrite(commande_pin, LOW);
  pinMode(commande_led, OUTPUT);
  digitalWrite(commande_led, LOW);
  pinMode(inter, INPUT_PULLUP);

#ifdef WIFI


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
#endif
}

void loop() {
  //mqtt.update();  // should be called
  static uint32_t prev_ms = millis();
  char message1[10];
  char message2[10];
  char message3[10];


  // publish message

  if (millis() > prev_ms + 500) {

    prev_ms = millis();

    float temp1 = getTemp(ds1);
    float temp2 = getTemp(ds2);
    float temp3 = getTemp(ds3);
    t1[no_iter] = temp1;
    t2[no_iter] = temp2;
    t3[no_iter] = temp3;
    if (no_iter >= nb_moyenne - 1)
    {
      floatToString(temp1, message1, sizeof(message1), 2);
      floatToString(temp2, message2, sizeof(message2), 2);    //Serial.print("Température chauffe eau : ");
      floatToString(temp3, message3, sizeof(message3), 2);   //Serial.println(message1);
#ifdef WIFI
      //Serial.print("connecting to mqtt broker...");
      //while (!mqtt.connect("192.168.1.60", "", "")) {
      //  Serial.print(".");
      //  delay(1000);
      // }

      mqtt.publish("/homeassistant/temperature/t1", FloatToStr(moyenne_tab(t1)));
      mqtt.publish("/homeassistant/temperature/t2", FloatToStr(moyenne_tab(t2)));
      mqtt.publish("/homeassistant/temperature/t3", FloatToStr(moyenne_tab(t3)));

#endif

      Serial.print( message1 );
      Serial.print( ",");
      Serial.print(message2);
      Serial.print(",");
      Serial.println( message3 );
#ifdef DEBUG
      if ( digitalRead(inter) == LOW )
      {
        digitalWrite(commande_pin, HIGH);
        digitalWrite(commande_led, HIGH);
      }
      else
      {
        digitalWrite(commande_pin, LOW);
        digitalWrite(commande_led, LOW);
      }
      Serial.print("Commande_PIN : ");
      Serial.println(digitalRead(commande_pin));
      Serial.print("inter : ");
      Serial.println(digitalRead(inter));

#endif
#ifndef DEBUG
      if (temp1 > consigne) {
        digitalWrite(commande_pin, HIGH);
        digitalWrite(commande_led, HIGH);
        delay(3 * 60 * 1000 );
        digitalWrite(commande_pin, LOW);
        digitalWrite(commande_led, LOW);
      }
#endif
      /*
        if (digitalRead(inter))
        {
        digitalWrite(commande_pin, HIGH);
        delay( 1000 );
        digitalWrite(commande_pin, LOW);
        }
      */
    }
    no_iter = (no_iter + 1) % nb_moyenne;
  }

}
